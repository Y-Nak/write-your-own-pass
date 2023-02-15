; This file is geenreted from `Mem2RegSingleStore.cpp` by running
; `$LLVM15_INSTALL_DIR/bin/clang -O0 -S -emit-llvm Mem2RegMultiBlock.cpp -o Mem2RegMultiBlock.ll`
; NOTE: Attributes are trimmed to make it more readable.

; RUN: opt -load-pass-plugin %plugin_dir/libMem2Reg%plugin_suffix --passes=custom-mem2reg -S %s | FileCheck %s
; CHECK-LABEL: test1
define i32 @test1(i1 %0) {
  ; CHECK: zext  
  ; CHECK-NEXT: trunc
  ; CHECK-NEXT: br i1 {{%[0-9]+}}, label %[[THEN_BB:[0-9]+]], label %[[ELSE_BB:[0-9]+]]
  %2 = alloca i8, align 1
  %3 = alloca i32, align 4
  %4 = zext i1 %0 to i8
  store i8 %4, ptr %2, align 1
  store i32 1, ptr %3, align 4
  %5 = load i8, ptr %2, align 1
  %6 = trunc i8 %5 to i1
  br i1 %6, label %7, label %8

; CHECK: [[THEN_BB]]:
7:                                                ; preds = %1
  ; CHECK-NEXT: br label %[[MERGE_BB:[0-9]+]]
  store i32 2, ptr %3, align 4
  br label %9

; CHECK: [[ELSE_BB]]:
8:                                                ; preds = %1
  ; CHECK-NEXT: br label %[[MERGE_BB]]
  store i32 3, ptr %3, align 4
  br label %9

; CHECK: [[MERGE_BB]]:
9:                                                ; preds = %8, %7
  ; CHECK-NEXT: [[RET:%[0-9]+]] = phi i32 [ 2, %[[THEN_BB]] ], [ 3, %[[ELSE_BB]] ]
  ; CHECK-NEXT: ret i32 [[RET]]
  %10 = load i32, ptr %3, align 4
  ret i32 %10
}

define i32 @test2(i1 zeroext %0) #0 {
  %2 = alloca i8, align 1
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = zext i1 %0 to i8
  store i8 %5, ptr %2, align 1
  store i32 10, ptr %3, align 4
  store i32 0, ptr %4, align 4
  br label %6

6:                                                ; preds = %24, %1
  %7 = load i32, ptr %4, align 4
  %8 = icmp slt i32 %7, 10
  br i1 %8, label %9, label %27

9:                                                ; preds = %6
  %10 = load i32, ptr %4, align 4
  %11 = load i32, ptr %3, align 4
  %12 = add nsw i32 %11, %10
  store i32 %12, ptr %3, align 4
  %13 = load i8, ptr %2, align 1
  %14 = trunc i8 %13 to i1
  br i1 %14, label %15, label %18

15:                                               ; preds = %9
  %16 = load i32, ptr %3, align 4
  %17 = add nsw i32 %16, 100
  store i32 %17, ptr %3, align 4
  br label %21

18:                                               ; preds = %9
  %19 = load i32, ptr %4, align 4
  %20 = add nsw i32 %19, 1
  store i32 %20, ptr %4, align 4
  br label %24

21:                                               ; preds = %15
  %22 = load i32, ptr %3, align 4
  %23 = add nsw i32 %22, 1
  store i32 %23, ptr %3, align 4
  br label %24

24:                                               ; preds = %21, %18
  %25 = load i32, ptr %4, align 4
  %26 = add nsw i32 %25, 1
  store i32 %26, ptr %4, align 4
  br label %6, !llvm.loop !5

27:                                               ; preds = %6
  %28 = load i32, ptr %3, align 4
  ret i32 %28
}

attributes #0 = { mustprogress noinline nounwind optnone ssp uwtable "frame-pointer"="non-leaf" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="apple-m1" "target-features"="+aes,+crc,+crypto,+dotprod,+fp-armv8,+fp16fml,+fullfp16,+lse,+neon,+ras,+rcpc,+rdm,+sha2,+sha3,+sm4,+v8.5a,+zcm,+zcz" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"uwtable", i32 2}
!3 = !{i32 7, !"frame-pointer", i32 1}
!4 = !{!"Homebrew clang version 15.0.7"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.mustprogress"}
