#include "FuncInfoPrinter.h"
#include "FuncInfoAnalysis.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

PreservedAnalyses FuncInfoPrinter::run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM)
{
    auto funcInfo = FAM.getResult<FuncInfoAnalysis>(F);
    errs() << "Name: " << funcInfo.name << "\n";
    errs() << "NArgs: " << funcInfo.nArgs << "\n";
    errs() << "NBlocks: " << funcInfo.nBlocks << "\n";
    errs() << "NInsts: " << funcInfo.nInsts << "\n\n";

    return PreservedAnalyses::all();
}

llvm::PassPluginLibraryInfo
getFuncInfoPrinterPass()
{
    return {LLVM_PLUGIN_API_VERSION, "func-info-printer", LLVM_VERSION_STRING,
            [](PassBuilder &PB)
            {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>)
                    {
                        if (Name == "func-info-printer")
                        {
                            FPM.addPass(FuncInfoPrinter());
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