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


; Function Attrs: mustprogress noinline nounwind optnone ssp uwtable
; CHECK-LABEL: singleStore2
; CHECK: br i1 {{%[0-9]+}}, label %[[THEN_BB:[0-9]+]], label %[[ELSE_BB:[0-9]+]]
; CHECK: phi i32 [ 2, %[[THEN_BB]] ], [ poison, %[[ELSE_BB]]
define i32 @singleStore2(i1 %0) {
  %2 = alloca i8, align 1
  %3 = alloca i32, align 4
  %4 = zext i1 %0 to i8
  store i8 %4, ptr %2, align 1
  %5 = load i8, ptr %2, align 1
  %6 = trunc i8 %5 to i1
  br i1 %6, label %7, label %8

7:                                                ; preds = %1
  store i32 2, ptr %3, align 4
  br label %9

8:                                                ; preds = %1
  br label %9

9:                                                ; preds = %8, %7
  %10 = load i32, ptr %3, align 4
  ret i32 %10
}