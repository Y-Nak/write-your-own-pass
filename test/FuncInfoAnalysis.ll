; This file is geenreted from `FuncInfoAnalysis.cpp` by running
; `$LLVM15_INSTALL_DIR/bin/clang -O0 -S -emit-llvm FuncInfoAnalysis.cpp -o FuncInfoAnalysis.ll`
; NOTE: Attributes are trimmed to make it more readable.

; RUN: opt -load-pass-plugin %plugin_dir/libFuncInfoAnalysis%plugin_suffix -passes=func-info-analysis -disable-output 2>&1 %s | FileCheck %s
; CHECK: Name: add
; CHECK-NEXT: NArgs: 2
; CHECK-NEXT: NBlocks: 1
; CHECK-NEXT: NInsts: 8
;
; CHECK: Name: sub1
; CHECK-NEXT: NArgs: 1
; CHECK-NEXT: NBlocks: 1
; CHECK-NEXT: NInsts: 5
;
; CHECK: Name: abs
; CHECK-NEXT: NArgs: 1
; CHECK-NEXT: NBlocks: 4
; CHECK-NEXT: NInsts: 15
define i32 @add(i32 %0, i32 %1) {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  store i32 %1, ptr %4, align 4
  %5 = load i32, ptr %3, align 4
  %6 = load i32, ptr %4, align 4
  %7 = add nsw i32 %5, %6
  ret i32 %7
}

define i32 @sub1(i32 %0) {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = sub nsw i32 %3, 1
  ret i32 %4
}

define i32 @abs(i32 %0) {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  %4 = load i32, ptr %3, align 4
  %5 = icmp slt i32 %4, 0
  br i1 %5, label %6, label %9

6:                                                ; preds = %1
  %7 = load i32, ptr %3, align 4
  %8 = sub nsw i32 0, %7
  store i32 %8, ptr %2, align 4
  br label %11

9:                                                ; preds = %1
  %10 = load i32, ptr %3, align 4
  store i32 %10, ptr %2, align 4
  br label %11

11:                                               ; preds = %9, %6
  %12 = load i32, ptr %2, align 4
  ret i32 %12
}