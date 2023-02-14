#ifndef CUSTOM_IR_ANALYSIS_TYPE_H
#define CUSTOM_IR_ANALYSIS_TYPE_H

using namespace llvm;

struct FuncInfo
{
    StringRef name;
    size_t nArgs;
    size_t nBlocks;
    size_t nInsts;

    FuncInfo(StringRef name, size_t nArgs, size_t nBlocks, size_t nInsts)
        : name(name), nArgs(nArgs), nBlocks(nBlocks), nInsts(nInsts) {}
};

#endif