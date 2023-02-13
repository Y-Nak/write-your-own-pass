#ifndef CUSTOM_IR_FUNC_INFO_ANALYSIS_H
#define CUSTOM_IR_FUNC_INFO_ANALYSIS_H

#include "llvm/IR/PassManager.h"

using namespace llvm;

struct FuncInfoAnalysis : PassInfoMixin<FuncInfoAnalysis>

{
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &_);
    static bool isRequired() { return true; }
};

#endif