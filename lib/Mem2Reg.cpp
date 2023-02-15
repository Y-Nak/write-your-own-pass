#include "Mem2Reg.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/IteratedDominanceFrontier.h"

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

    /// This struct holds information for phi node insertion and renaming.
    struct MultiBlockAllocaInfo
    {
        SmallPtrSet<StoreInst *, 32> defInstSet;
        SmallPtrSet<BasicBlock *, 32> defBlockSet;
        SmallPtrSet<LoadInst *, 32> useInstSet;
        SmallPtrSet<PHINode *, 32> phiNodeSet;
        SmallVector<Value *, 8> valueStack;

        void clear()
        {
            defInstSet.clear();
            defBlockSet.clear();
            useInstSet.clear();
            phiNodeSet.clear();
            valueStack.clear();
        }

        StoreInst *isDefInst(Instruction &I)
        {
            if (StoreInst *SI = dyn_cast<StoreInst>(&I))
            {
                if (defInstSet.contains(SI))
                {
                    return SI;
                }
            }

            return nullptr;
        }

        LoadInst *isUseInst(Instruction &I)
        {
            if (LoadInst *LI = dyn_cast<LoadInst>(&I))
            {
                if (useInstSet.contains(LI))
                {
                    return LI;
                }
            }

            return nullptr;
        }

        PHINode *isPhiInst(Instruction &I)
        {
            if (PHINode *PN = dyn_cast<PHINode>(&I))
            {
                if (phiNodeSet.contains(PN))
                {
                    return PN;
                }
            }
            return nullptr;
        }
    };

    struct CustomMem2RegRunner
    {
        DenseMap<AllocaInst *, std::unique_ptr<PromotableAllocaInfo>>
            allocaInfoMap;
        MultiBlockAllocaInfo mbInfo;

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

        bool promoteSingleStoreAlloca(AllocaInst *AI, StoreInst *singleSI, DominatorTree &DT)
        {
            bool isEliminatedAll = true;
            BasicBlock *SIBB = singleSI->getParent();
            Value *storedValue = singleSI->getOperand(0);

            bool isStoreDefined = false;
            // Check the load instruction is used after the store instruction if the load
            // instruction is in the same basic block with the store instruction.
            // If the load instruction is used after the store instruction, we can replace it.
            // Otherwise we have to use `promoteMultiBlockAlloca` later.
            for (Instruction &I : make_early_inc_range(*SIBB))
            {
                if (&I == singleSI)
                {
                    isStoreDefined = true;
                    continue;
                }

                LoadInst *LI = dyn_cast<LoadInst>(&I);
                if (LI && LI->getOperand(0) == AI)
                {
                    if (isStoreDefined)
                    {
                        LI->replaceAllUsesWith(storedValue);
                        LI->eraseFromParent();
                    }
                    else
                        isEliminatedAll = false;
                }
            }

            for (User *U : make_early_inc_range(AI->users()))
            {
                LoadInst *LI = dyn_cast<LoadInst>(U);
                if (!LI)
                    continue;

                // If the load inst is not in the same basic block with the store inst and
                // the load inst is dominated by the store inst, we can replace the load inst.
                // Otherwise we have to use `promoteMultiBlockAlloca` later so that a phi function is properly inserted.
                if (LI->getParent() != SIBB)
                {
                    if (DT.dominates(singleSI, LI))
                    {
                        LI->replaceAllUsesWith(storedValue);
                        LI->eraseFromParent();
                    }
                    else
                        isEliminatedAll = false;
                }
            }

            // If all the load instructions are replaced, we can erase the store instruction and the alloca instruction.
            if (isEliminatedAll)
            {
                singleSI->eraseFromParent();
                AI->eraseFromParent();
            }

            return isEliminatedAll;
        }

        bool promoteSingleBlockAlloca(AllocaInst *AI, BasicBlock *singleBB)
        {
            bool isEliminatedAll = true;
            StoreInst *curSI = nullptr;
            SmallVector<StoreInst *, 8> storesToErase;

            for (Instruction &I : make_early_inc_range(*singleBB))
            {
                LoadInst *LI = dyn_cast<LoadInst>(&I);
                if (LI && LI->getOperand(0) == AI)
                {
                    if (curSI)
                    {
                        LI->replaceAllUsesWith(curSI->getOperand(0));
                        LI->eraseFromParent();
                    }
                    // If the load instruction used before store instruction, we have to use `promoteMultiBlockAlloca` later.
                    else
                        isEliminatedAll = false;
                }

                StoreInst *SI = dyn_cast<StoreInst>(&I);
                if (SI && SI->getOperand(1) == AI)
                {
                    if (curSI)
                        storesToErase.push_back(curSI);
                    curSI = SI;
                }
            }

            for (StoreInst *SI : storesToErase)
                SI->eraseFromParent();

            if (isEliminatedAll)
            {
                AI->eraseFromParent();
                if (curSI)
                {
                    curSI->eraseFromParent();
                }
            }

            return isEliminatedAll;
        }

        /// This function fills phi arguments and replaced `LoadInst` result with proper `Phi` node
        /// while traversing the dominator tree from the root to leafs. The traversing order is DFS.
        /// After the traversing, the function remove the unnecessary `StoreInst`.
        void rename(DomTreeNode *DN)
        {
            BasicBlock *BB = DN->getBlock();
            for (Instruction &I : make_early_inc_range(*BB))
            {
                // Push the stored value to the stack.
                if (auto *SI = mbInfo.isDefInst(I))
                {
                    auto value = SI->getOperand(0);
                    mbInfo.valueStack.push_back(value);
                }

                // Push the phi value to the stack.
                if (auto *PN = mbInfo.isPhiInst(I))
                    mbInfo.valueStack.push_back(PN);

                // Rename the `LoadInst` with the most recent seen value (the top of the stack).
                if (auto *LI = mbInfo.isUseInst(I))
                {
                    if (mbInfo.valueStack.empty())
                        LI->replaceAllUsesWith(PoisonValue::get(LI->getType()));
                    else
                    {
                        auto value = mbInfo.valueStack.back();
                        LI->replaceAllUsesWith(value);
                        LI->eraseFromParent();
                    }
                }
            }

            // Fill the phi node of the successors.
            for (succ_iterator Succ = succ_begin(BB), E = succ_end(BB); Succ != E; ++Succ)
            {
                for (Instruction &I : **Succ)
                {
                    if (auto *PN = mbInfo.isPhiInst(I))
                    {
                        if (mbInfo.valueStack.empty())
                            PN->addIncoming(PoisonValue::get(PN->getType()), BB);
                        else
                            PN->addIncoming(mbInfo.valueStack.back(), BB);
                    }
                }
            }

            // Traverse the children of the current sub dominator tree.
            for (auto &C : DN->children())
            {
                rename(C);
            }

            // Now, there is no users of `StoreInst` result, so it's time to erase them.
            for (Instruction &I : make_early_inc_range(*BB))
            {
                if (auto *SI = mbInfo.isDefInst(I))
                {
                    mbInfo.valueStack.pop_back();
                    SI->eraseFromParent();
                }
                if (auto *PN = mbInfo.isPhiInst(I))
                {
                    mbInfo.valueStack.pop_back();
                }
            }
        }

        void
        promoteMultiBlockAlloca(Function &F, AllocaInst *AI, DominatorTree &DT)
        {
            mbInfo.clear();
            for (User *U : AI->users())
            {
                if (StoreInst *SI = dyn_cast<StoreInst>(U))
                {
                    mbInfo.defInstSet.insert(SI);
                    mbInfo.defBlockSet.insert(SI->getParent());
                }
                else if (LoadInst *LI = dyn_cast<LoadInst>(U))
                {
                    mbInfo.useInstSet.insert(LI);
                }
            }

            // Calculate Iterated Dominance Frontier(IDF).
            // IDF.setDefiningBlocks(DefBlockSet);
            // Initialize IDF calculator.
            ForwardIDFCalculator IDF(DT);
            IDF.setDefiningBlocks(mbInfo.defBlockSet);
            SmallVector<BasicBlock *, 32> PhiInsertionBlocks;
            IDF.calculate(PhiInsertionBlocks);
            // Insert empty Phi blocks to IDF blocks.
            for (auto PB : PhiInsertionBlocks)
            {
                PHINode *PN = PHINode::Create(AI->getAllocatedType(), 0, "", &PB->front());
                mbInfo.phiNodeSet.insert(PN);
            }

            rename(DT.getNode(&F.getEntryBlock()));
            AI->eraseFromParent();
        }

        bool run(Function &F, DominatorTree &DT)
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
                    if (!promoteSingleStoreAlloca(pair.first, pair.second->singleStoreInst, DT))
                        promoteMultiBlockAlloca(F, pair.first, DT);
                }
                // If the alloca is used only in a single basic block,
                // call `PromoteSingleBlockAlloca` to promote it.
                else if (pair.second->singleBlockUse)
                {
                    if (!promoteSingleBlockAlloca(pair.first, pair.second->singleBB))
                        promoteMultiBlockAlloca(F, pair.first, DT);
                }
                else
                {
                    promoteMultiBlockAlloca(F, pair.first, DT);
                }
            }

            return true;
        }
    };
}

PreservedAnalyses
CustomMem2Reg::run(llvm::Function &F, FunctionAnalysisManager &FAM)
{
    auto runner = CustomMem2RegRunner();

    // Obtain Dominator Tree.
    DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);

    auto changed = runner.run(F, DT);
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