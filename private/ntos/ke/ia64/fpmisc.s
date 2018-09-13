#include "ksia64.h"

//++
//
// VOID
// run_fms (
//    IN ULONGLONG *fpsr,
//    OUT FLOAT128 *fr1, 
//    IN FLOAT128 *fr2, 
//    IN FLOAT128 *fr3, 
//    IN FLOAT128 *fr4
//    )
//
// Routine Description:
//
//    This function runs FMS operation with the specified inputs and FPSR.
//
//--
  LEAF_ENTRY(run_fms)
  alloc r31=ar.pfs,5,2,0,0  // r32, r33, r34, r35, r36, r37, r38

  ARGPTR  (r32)
  ARGPTR  (r33)
  ARGPTR  (r34)
  ARGPTR  (r35)
  ARGPTR  (r36)

  // &fpsr is in r32
  // &fr1 (output) is in r33
  // &fr2 (input) is in r34
  // &fr3 (input) is in r35
  // &fr4 (input) is in r36

  // save old FPSR in r37
  mov r37 = ar40
  nop.i 0;;

  // load new fpsr in r38
  ld8 r38 = [r32];;
  // set new value of FPSR
  mov ar40 = r38
  nop.i 0;;

  // load first input argument into f8
  ldf.fill f8 = [r34]
  // load second input argument into f9
  ldf.fill f9 = [r35]
  nop.i 0;;

  // load third input argument into f10
  ldf.fill f10 = [r36]
  nop.m 0
  nop.i 0;;

  nop.m 0
  (p0) fms.s0 f11 = f8, f9, f10 // f11 = f8 * f9 - f10
  nop.i 0;;

  // store result
  stf.spill [r33] = f11
  // save new FPSR in r38
  mov r38 = ar40
  nop.i 0;;

  // store new fpsr from r38
  st8 [r32] = r38
  // restore FPSR
  mov ar40 = r37
  nop.i 0;;

  nop.m 0
  nop.i 0

  // return
  LEAF_RETURN

  LEAF_EXIT(run_fms)      

//++
//
// VOID
// thmF (
//    IN ULONGLONG *fpsr,
//    OUT FLOAT128 *fr1, 
//    IN FLOAT128 *fr2, 
//    IN FLOAT128 *fr3
//    )
//
// Routine Description:
//
//--
  LEAF_ENTRY(thmF)

  alloc r31=ar.pfs,4,4,0,0  // r32, r33, r34, r35, r36, r37, r38, r39

  ARGPTR  (r32)
  ARGPTR  (r33)
  ARGPTR  (r34)
  ARGPTR  (r35)

  // &fpsr is in r32 
  // &a is in r33 
  // &b is in r34 
  // &div is in r35 (the address of the divide result)

  // save old FPSR in r36
  mov r36 = ar40
  // save predicates in r37
  mov r37 = pr;;

  // load new fpsr in r39
  ld8 r39 = [r32];;
  // set new value of FPSR
  mov ar40 = r39
  nop.i 0;;

  nop.m 0
  // clear predicates
  movl r38 = 0x0000000000000001;;

  nop.m 0
  // load clear predicates from r38
  mov pr = r38,0x1ffff
  nop.i 0;;

  // load a, the first argument, in f6
  ldf.fill f6 = [r33]
  // load b, the second argument, in f7
  ldf.fill f7 = [r34]
  nop.i 0;;

  nop.m 0
  // Step (1)
  // y0 = 1 / b in f8
  frcpa.s0 f8,p2=f6,f7
  nop.i 0;;

  nop.m 0
  // Step (2)
  // e0 = 1 - b * y0 in f9
  (p2) fnma.s1 f9=f7,f8,f1
  nop.i 0

  nop.m 0
  // Step (10)
  // q0 = a * y0 in f10
  (p2) fma.s1 f10=f6,f8,f0
  nop.i 0;;

  nop.m 0
  // Step (3)
  // y1 = y0 + e0 * y0 in f8
  (p2) fma.s1 f8=f9,f8,f8
  nop.i 0

  nop.m 0
  // Step (4)
  // e1 = e0 * e0 in f9
  (p2) fma.s1 f9=f9,f9,f0
  nop.i 0

  nop.m 0
  // Step (11)
  // r0 = a - b * q0 in f11
  (p2) fnma.s1 f11=f7,f10,f6
  nop.i 0;;

  nop.m 0
  // Step (5)
  // y2 = y1 + e1 * y1 in f8
  (p2) fma.s1 f8=f8,f9,f8
  nop.i 0;;

  nop.m 0
  // Step (6)
  // e2 = 1 - b * y2 in f9
  (p2) fnma.s1 f9=f7,f8,f1
  nop.i 0;;

  nop.m 0
  // Step (7)
  // y3 = y2 + e2 * y2 in f8
  (p2) fma.s1 f8=f8,f9,f8
  nop.i 0;;

  nop.m 0
  // Step (8)
  // e3 = 1 - b * y3 in f9
  (p2) fnma.s1 f9=f7,f8,f1
  nop.i 0

  nop.m 0
  // Step (12)
  // q1 = q0 + r0 * y3 in f10
  (p2) fma.s1 f10=f11,f8,f10
  nop.i 0;;

  nop.m 0
  // Step (9)
  // y4 = y3 + e3 * y3 in f8
  (p2) fma.s1 f8=f8,f9,f8
  nop.i 0

  nop.m 0
  // Step (13)
  // r1 = a - b * q1 in f11
  (p2) fnma.s1 f11=f7,f10,f6
  nop.i 0;;

  nop.m 0
  // Step (14)
  // q2 = q1 + r1 * y4 in f8
  (p2) fma.s0 f8=f11,f8,f10
  nop.i 0;;

  // save new FPSR in r39
  mov r39 = ar40;;
  // store new fpsr from r39
  st8 [r32] = r39
  // restore predicates from r37
  mov pr = r37,0x1ffff;;

  // store result
  stf.spill [r35]=f8
  // restore FPSR
  mov ar40 = r36
  // return
  LEAF_RETURN

  LEAF_EXIT(thmF)

//++
//
// VOID
// thmL (
//    IN ULONGLONG *fpsr,
//    OUT FLOAT128 *fr1, 
//    IN FLOAT128 *fr2
//    )
//
// Routine Description:
//
//--
  LEAF_ENTRY(thmL)

  alloc	r31=ar.pfs,3,5,0,0  // r32, r33, r34, r35, r36, r37, r38, r39

  ARGPTR  (r32)
  ARGPTR  (r33)
  ARGPTR  (r34)

  // &fpsr is in r32 
  // &a is in r33 
  // &sqrt is in r34 (the address of the sqrt result)

  // save old FPSR in r35
  mov r35 = ar40
  // save predicates in r36
  mov r36 = pr;;

  // load new fpsr in r38
  ld8 r38 = [r32];;
  // set new value of FPSR
  mov ar40 = r38
  nop.i 0;;

  nop.m 0
  // clear predicates
  movl r37 = 0x0000000000000001;;

  nop.m 0
  // load clear predicates from r37
  mov pr = r37,0x1ffff
  nop.i 0;;

  // load the argument a in f6
  ldf.fill f6 = [r33]
  nop.m 0
  nop.i 0;;

  nop.m 0
  // Step (1)
  // y0 = 1/sqrt(a) in f8
  frsqrta.s0 f8,p2=f6
  nop.i 0;;

  nop.m 0
  // Step (2)
  // load 1/2 in f7; h = 1/2 * a in f9
  (p2) movl r39 = 0x0fffe;;

  (p2) setf.exp f7 = r39
  nop.i 0;;

  nop.m 0
  (p2) fma.s1 f9=f7,f6,f0
  nop.i 0;;

  nop.m 0
  // Step (3)
  // t1 = y0 * y0 in f10
  (p2) fma.s1 f10=f8,f8,f0
  nop.i 0;;

  nop.m 0
  // Step (4)
  // t2 = 1/2 - t1 * h in f10
  (p2) fnma.s1 f10=f10,f9,f7
  nop.i 0;;

  nop.m 0
  // Step (5)
  // y1 = y0 + t2 * y0 in f8
  (p2) fma.s1 f8=f10,f8,f8
  nop.i 0;;

  nop.m 0
  // Step (6)
  // t3 = y1 * h in f10
  (p2) fma.s1 f10=f8,f9,f0
  nop.i 0;;

  nop.m 0
  // Step (7)
  // t4 = 1/2 - t3 * y1 in f10
  (p2) fnma.s1 f10=f10,f8,f7
  nop.i 0;;

  nop.m 0
  // Step (8)
  // y2 = y1 + t4 * y1 in f8
  (p2) fma.s1 f8=f10,f8,f8
  nop.i 0;;

  nop.m 0
  // Step (9)
  // S = a * y2 in f10
  (p2) fma.s1 f10=f6,f8,f0
  nop.i 0;;

  nop.m 0
  // Step (10)
  // t5 = y2 * h in f9
  (p2) fma.s1 f9=f8,f9,f0
  nop.i 0;;

  nop.m 0
  // Step (11)
  // H = 1/2 * y2 in f11
  (p2) fma.s1 f11=f7,f8,f0
  nop.i 0;;

  nop.m 0
  // Step (13)
  // t6 = 1/2 - t5 * y2 in f7
  (p2) fnma.s1 f7=f9,f8,f7
  nop.i 0;;

  nop.m 0
  // Step (12)
  // d = a - S * S in f8
  (p2) fnma.s1 f8=f10,f10,f6
  nop.i 0;;

  nop.m 0
  // Step (14)
  // S1 = S + d * H in f8
  (p2) fma.s1 f8=f8,f11,f10
  nop.i 0;;

  nop.m 0
  // Step (15)
  // H1 = H + t6 * h in f7
  (p2) fma.s1 f7=f11,f7,f11
  nop.i 0;;

  nop.m 0
  // Step (16)
  // d1 = a - S1 * S1 in f6
  (p2) fnma.s1 f6=f8,f8,f6
  nop.i 0;;

  nop.m 0
  // Step (17)
  // R = S1 + d1 * H1 in f8
  (p2) fma.s0 f8=f6,f7,f8
  nop.i 0;;

  // save new FPSR in r38
  mov r38 = ar40;;
  // store new fpsr from r38
  st8 [r32] = r38
  // restore predicates from r36
  mov pr = r36,0x1ffff;;

  // store result
  stf.spill [r34]=f8
  // restore FPSR
  mov ar40 = r35
  // return
  LEAF_RETURN
  
  LEAF_EXIT(thmL)


//++
//
// VOID
// KiEmulateLoadFloat80(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateLoadFloat80)

  ARGPTR(a0)
  ARGPTR(a1)

  ldfe           ft0 = [a0]       
  ;;
  stf.spill      [a1] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateLoadFloat80) 


//++
//
// VOID
// KiEmulateLoadFloatInt(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateLoadFloatInt)

  ARGPTR(a0)
  ARGPTR(a1)

  ldf8          ft0 = [a0]       
  ;;
  stf.spill      [a1] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateLoadFloatInt) 

//++
//
// VOID
// KiEmulateLoadFloat32(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateLoadFloat32)

  ARGPTR(a0)
  ARGPTR(a1)

  ldfs           ft0 = [a0]       
  ;;
  stf.spill      [a1] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateLoadFloat32) 

//++
//
// VOID
// KiEmulateLoadFloat64(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateLoadFloat64)

  ARGPTR(a0)
  ARGPTR(a1)

  ldfd          ft0 = [a0]       
  ;;
  stf.spill      [a1] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateLoadFloat64) 



//++
//
// VOID
// KiEmulateStoreFloat80(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateStoreFloat80)

  ARGPTR(a0)
  ARGPTR(a1)

  ldf.fill      ft0 = [a1]       
  ;;
  stfe          [a0] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateStoreFloat80) 


//++
//
// VOID
// KiEmulateStoreFloatInt(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateStoreFloatInt)

  ARGPTR(a0)
  ARGPTR(a1)

  ldf.fill      ft0 = [a1]       
  ;;
  stfd          [a0] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateStoreFloatInt) 

//++
//
// VOID
// KiEmulateStoreFloat32(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateStoreFloat32)

  ARGPTR(a0)
  ARGPTR(a1)

  ldf.fill           ft0 = [a1]       
  ;;
  stfs               [a0] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateStoreFloat32) 

//++
//
// VOID
// KiEmulateStoreFloat64(
//    IN PVOID UnalignedAddress, 
//    OUT PVOID FloatData
//    );
//
//-- 

  LEAF_ENTRY(KiEmulateStoreFloat64)

  ARGPTR(a0)
  ARGPTR(a1)

  ldf.fill          ft0 = [a1]       
  ;;
  stfd              [a0] = ft0
       
  LEAF_RETURN
  LEAF_EXIT(KiEmulateStoreFloat64) 





