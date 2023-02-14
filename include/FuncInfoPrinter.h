#ifndef CUSTOM_IR_FUNC_INFO_PRINTER_H
#define CUSTOM_IR_FUNC_INFO_PRINTER_H

#include "llvm/IR/PassManager.h"

using namespace llvm;

struct FuncInfoPrinter : PassInfoMixin<FuncInfoPrinter>
{
    PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
    static bool isRequired() { return true; }
};

#endif