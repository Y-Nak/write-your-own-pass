#include "FuncInfoAnalysis.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

AnalysisKey FuncInfoAnalysis::Key;

FuncInfoAnalysis::Result FuncInfoAnalysis::run(Function &F, FunctionAnalysisManager &)
{
    auto name = F.getName();
    auto nArgs = F.arg_size();
    auto nBlocks = F.size();
    auto nInsts = F.getInstructionCount();

    return FuncInfoAnalysis::Result(name, nArgs, nBlocks, nInsts);
}

llvm::PassPluginLibraryInfo
getFuncInfoAnalysisPass()
{
    return {LLVM_PLUGIN_API_VERSION, "func-info-analysis", LLVM_VERSION_STRING,
            [](PassBuilder &PB)
            {
                PB.registerAnalysisRegistrationCallback(
                    [](FunctionAnalysisManager &FAM)
                    {
                        FAM.registerPass([&]
                                         { return FuncInfoAnalysis(); });
                    });
            }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo()
{
    return getFuncInfoAnalysisPass();
}