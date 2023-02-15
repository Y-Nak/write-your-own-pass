; This file is geenreted from `Mem2RegSingleStore.cpp` by running
; `$LLVM15_INSTALL_DIR/bin/clang -O0 -S -emit-llvm Mem2RegSingleBlock.cpp -o Mem2RegSingleBlock.ll`
; NOTE: Attributes are trimmed to make it more readable.

; RUN: opt -load-pass-plugin %plugin_dir/libMem2Reg%plugin_suffix --passes=custom-mem2reg -S %s | FileCheck %s
; CHECK-LABEL: singleBlock
; CHECK-NEXT:   %1 = add nsw i32 0, 10
; CHECK-NEXT:   %2 = add nsw i32 %1, 20
; CHECK-NEXT:   %3 = add nsw i32 %2, 30
; CHECK-NEXT:   ret i32 %3
; CHECK-NEXT: }
define i32 @singleBlock() {
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  store i32 10, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = load i32, ptr %1, align 4
  %5 = add nsw i32 %4, %3
  store i32 %5, ptr %1, align 4
  store i32 20, ptr %2, align 4
  %6 = load i32, ptr %2, align 4
  %7 = load i32, ptr %1, align 4
  %8 = add nsw i32 %7, %6
  store i32 %8, ptr %1, align 4
  store i32 30, ptr %2, align 4
  %9 = load i32, ptr %2, align 4
  %10 = load i32, ptr %1, align 4
  %11 = add nsw i32 %10, %9
  store i32 %11, ptr %1, align 4
  %12 = load i32, ptr %1, align 4
  ret i32 %12
}
