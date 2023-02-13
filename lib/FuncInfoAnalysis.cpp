#include "FuncInfoAnalysis.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

PreservedAnalyses FuncInfoAnalysis::run(Function &F, FunctionAnalysisManager &_)
{
    errs() << "Name: " << F.getName() << "\n";
    errs() << "NArgs: " << F.arg_size() << "\n";
    errs() << "NBlocks: " << F.size() << "\n";
    errs() << "NInsts: " << F.getInstructionCount() << "\n\n";
    return PreservedAnalyses::all();
}

llvm::PassPluginLibraryInfo
getFuncInfoAnalysisPass()
{
    return {LLVM_PLUGIN_API_VERSION, "func-info-analysis", LLVM_VERSION_STRING,
            [](PassBuilder &PB)
            {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>)
                    {
                        if (Name == "func-info-analysis")
                        {
                            FPM.addPass(FuncInfoAnalysis());
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
    return getFuncInfoAnalysisPass();
}