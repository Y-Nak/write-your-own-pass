#ifndef CUSTOM_IR_FUNC_INFO_ANALYSIS_H
#define CUSTOM_IR_FUNC_INFO_ANALYSIS_H

#include "llvm/IR/PassManager.h"

#include "AnalysisType.h"

using namespace llvm;

struct FuncInfoAnalysis : AnalysisInfoMixin<FuncInfoAnalysis>
{
public:
    using Result = FuncInfo;

    Result run(Function &F, FunctionAnalysisManager &);
    static bool isRequired() { return true; }

private:
    static AnalysisKey Key;
    friend struct AnalysisInfoMixin<FuncInfoAnalysis>;
};

#endif