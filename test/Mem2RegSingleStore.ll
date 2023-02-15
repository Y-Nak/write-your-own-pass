; This file is geenreted from `Mem2RegSingleStore.cpp` by running
; `$LLVM15_INSTALL_DIR/bin/clang -O0 -S -emit-llvm Mem2RegSingleStore.cpp -o Mem2RegSingleStore.ll`
; NOTE: Attributes are trimmed to make it more readable.


; RUN: opt -load-pass-plugin %plugin_dir/libMem2Reg%plugin_suffix --passes=custom-mem2reg -S %s | FileCheck %s
; CHECK-LABEL: singleStore
; CHECK-NEXT:   ret i32 10
; CHECK-NEXT: }
define i32 @singleStore(i32 %0) {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  store i32 10, ptr %3, align 4
  %4 = load i32, ptr %3, align 4
  ret i32 %4
}
