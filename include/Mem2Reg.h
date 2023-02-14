#ifndef CUSTOM_IR_MEM2REG_H
#define CUSTOM_IR_MEM2REG_H

#include "llvm/IR/PassManager.h"

using namespace llvm;

struct CustomMem2Reg : PassInfoMixin<CustomMem2Reg>
{
    PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
    static bool isRequired() { return true; }
};

#endif