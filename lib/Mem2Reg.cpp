#include "Mem2Reg.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/BasicBlock.h"

// `dbgs() << ...` is quite useful for debugging.
#include "llvm/Support/Debug.h"

using namespace llvm;

namespace
{

    struct PromotableAllocaInfo
    {
        /// True If the uses of the alloca is limited to a single basic block.
        bool singleBlockUse;
        /// True if the alloca is used only by a single store.
        bool singleStore;

        /// If hte alloca is used only by a single store, this holds the store inst.
        /// Otherwise, this is null.
        StoreInst *singleStoreInst;
        /// If the alloca is used only in a single basic block, this holds the basic block.
        /// Otherwise, this is null.
        BasicBlock *singleBB;

        PromotableAllocaInfo(bool singleBlockUse, bool singleStore, StoreInst *singleStoreInst, BasicBlock *singleBB)
            : singleBlockUse(singleBlockUse), singleStore(singleStore), singleStoreInst(singleStoreInst), singleBB(singleBB) {}
    };

    struct CustomMem2RegRunner
    {
        DenseMap<AllocaInst *, std::unique_ptr<PromotableAllocaInfo>>
            allocaInfoMap;

        void collectPromotableAlloca(Function &F)
        {
            BasicBlock &Entry = F.getEntryBlock();
            for (BasicBlock::iterator I = Entry.begin(), E = --Entry.end(); I != E; ++I)
            {
                // If the inst is not alloca, skip it.
                auto AI = dyn_cast<AllocaInst>(I);
                if (!AI)
                    continue;

                collectPromotableAllocaImpl(AI);
            }
        }

        /// This function analyze alloca inst to check whether it is promotable.
        /// Then if the inst is promotable, it stores its information to `allocaInfoMap`.
        void collectPromotableAllocaImpl(AllocaInst *AI)
        {
            bool singleBlockUse = true;
            size_t nStore = 0;
            StoreInst *singleStoreInst = nullptr;
            BasicBlock *userBB = nullptr;

            // In this loop we analyze three things:
            // 1. Whether the alloca is promotable.
            // 2. Whether the alloca is used in a single basic block
            // 3. Whether the alloca is used only by a single store.
            for (User *U : AI->users())
            {
                Instruction *userInst = cast<Instruction>(U);
                if (LoadInst *LI = dyn_cast<LoadInst>(userInst))
                {
                    if (LI->isVolatile() || LI->getType() != AI->getAllocatedType())
                        return;
                }
                else if (StoreInst *SI = dyn_cast<StoreInst>(userInst))
                {
                    if (SI->isVolatile() || SI->getValueOperand()->getType() != AI->getAllocatedType())
                        return;
                    if (nStore == 0)
                        singleStoreInst = SI;
                    ++nStore;
                }
                else
                    return;

                auto instBB = userInst->getParent();
                if (!userBB)
                    userBB = instBB;
                else if (singleBlockUse && userBB != instBB)
                    singleBlockUse = false;
            }
            singleStoreInst = nStore == 1 ? singleStoreInst : nullptr;
            BasicBlock *singleBB = singleBlockUse ? userBB : nullptr;

            allocaInfoMap[AI] = std::make_unique<PromotableAllocaInfo>(singleBlockUse, nStore == 1, singleStoreInst, singleBB);
        }

        void promoteSingleStoreAlloca(AllocaInst *AI, StoreInst *singleSI)
        {
            Value *storedValue = singleSI->getOperand(0);
            // We have to use `make_early_inc_range` here
            // because we are going to delete the load in the loop.
            for (User *U : make_early_inc_range(AI->users()))
            {
                if (LoadInst *LI = dyn_cast<LoadInst>(U))
                {
                    // NOTE: This will cause bug if `singleSI` doesn't dominate `LI`.
                    // We will see how to fix it later when we introduce `DominatorTree`.
                    LI->replaceAllUsesWith(storedValue);
                    LI->eraseFromParent();
                }
            }

            singleSI->eraseFromParent();
            AI->eraseFromParent();
        }

        void promoteSingleBlockAlloca(AllocaInst *AI, BasicBlock *singleBB)
        {
            Value *curRepVal = nullptr;
            SmallVector<StoreInst *, 8> storesToErase;

            for (Instruction &I : make_early_inc_range(*singleBB))
            {
                LoadInst *LI = dyn_cast<LoadInst>(&I);
                if (LI && LI->getOperand(0) == AI)
                {
                    if (!curRepVal)
                        // NOTE: This is not sufficient if there is backedge from successor of `singleBB` to `singleBB`.
                        // We'll address this later when we introduce `DominatorTree`.
                        LI->replaceAllUsesWith(PoisonValue::get(LI->getType()));
                    else
                        LI->replaceAllUsesWith(curRepVal);
                    LI->eraseFromParent();
                    continue;
                }

                StoreInst *SI = dyn_cast<StoreInst>(&I);
                if (SI && SI->getOperand(1) == AI)
                {
                    curRepVal = SI->getOperand(0);
                    storesToErase.push_back(SI);
                }
            }

            for (StoreInst *SI : storesToErase)
                SI->eraseFromParent();

            AI->eraseFromParent();
        }

        bool run(Function &F)
        {
            collectPromotableAlloca(F);
            if (allocaInfoMap.empty())
                return false;

            // Promote the alloca insts that is used only by a single store.
            for (auto &pair : allocaInfoMap)
            {
                // If the alloca is used only by a single store and (maybe) multiple loads,
                // call `PromoteSingleStoreAlloca` to promote it.
                if (pair.second->singleStore)
                {
                    promoteSingleStoreAlloca(pair.first, pair.second->singleStoreInst);
                }
                // If the alloca is used only in a single basic block,
                // call `PromoteSingleBlockAlloca` to promote it.
                else if (pair.second->singleBlockUse)
                {
                    promoteSingleBlockAlloca(pair.first, pair.second->singleBB);
                }
            }

            return true;
        }
    };
}

PreservedAnalyses CustomMem2Reg::run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM)
{
    auto runner = CustomMem2RegRunner();
    auto changed = runner.run(F);
    if (changed)
    {
        // Mem2Reg pass doesn't change a control flow graph if it removes some instructions.
        // So it's better to specify `CFGAnalyses` here as a preserved set.
        auto PM = PreservedAnalyses();
        PM.preserveSet<CFGAnalyses>();
        return PM;
    }

    return PreservedAnalyses::all();
}

llvm::PassPluginLibraryInfo
getFuncInfoPrinterPass()
{
    return {LLVM_PLUGIN_API_VERSION, "custom-mem2reg", LLVM_VERSION_STRING,
            [](PassBuilder &PB)
            {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>)
                    {
                        if (Name == "custom-mem2reg")
                        {
                            FPM.addPass(CustomMem2Reg());
                            return true;
                        }
                        else
                        {
                            return false;
                        }
                    });
            }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo()
{
    return getFuncInfoPrinterPass();
}