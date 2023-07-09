#include "ksia64.h"

//++
//
// VOID
//++
//
// VOID
// KeFlushCurrentTb (
//    )
//
// Routine Description:
//
//    This function flushes the entire translation buffer.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
// Algorithm:
//
//--

        LEAF_ENTRY(KeFlushCurrentTb)

        ptc.e   r0

        ;;
        srlz.d
        ;;

        LEAF_RETURN

        LEAF_EXIT(KeFlushCurrentTb)
//++
//
// VOID
// KiPurgeTranslationCache (
//    ULONGLONG Base,
//    ULONGLONG Stride1,
//    ULONGLONG Stride2,
//    ULONGLONG Count1,
//    ULONGLONG Count2
//    )
//
// Routine Description:
//
//    This function flushes the entire translation cache from the current processor.
//
// Arguments:
//
//    r32 - base
//    r33 - stride1
//    r34 - stride2
//    r35 - count1
//    r36 - count2
//      
// Return Value:
//
//    None.
//
// Algorithm:
//
//    for (i = 0; i < count1; i++) {
//        for (j = 0; j < count2; j++) {
//            ptc.e addr;
//            addr += stride2;
//        }
//        addr += stride1;
//    }
//
//--
        LEAF_ENTRY(KiPurgeTranslationCache)
        
        alloc	r38=5, 4, 1, 0				    // M
	mov	r37=b0					    // I
	adds	sp=-32, sp				    // I

	rsm     1 << PSR_I                                  // M, disable interrupts 
	cmp.ge	p14,p15=r0, r35				    // M
	mov.i	r39=ar.lc;;				    // I

	ld8.nta	r3=[sp]					    // M
	nop.f	 0                                          // F    
  (p14)	br.cond.dpnt.few $L150#;;			    // B

  $L148:
	cmp.ge	p14,p15=r0, r36				    // M
	adds	r31=-1, r36				    // I
  (p14)	br.cond.dpnt.few $L153#;;			    // B

	nop.m	 0                                          // M
	mov.i	ar.lc=r31				    // I
	nop.b	 0                                          // B

  $L151:
        ptc.e   r32                                         // M, purge a TC entry
	nop.f	 0                                          // F
	nop.b    0;;                                        //

        add	r32=r32, r34				    // M
	nop.i	 0                                          // I 
	br.cloop.dptk.many $L151#;;			    // B

$L153:
	adds	r35=-1, r35				    // M
	add	r32=r32, r33;;				    // I
	cmp.ltu	p14,p15=r0, r35				    // I

	nop.m	 0                                          // M
	nop.f	 0                                          // F
  (p14)	br.cond.dptk.few $L148#;;			    // B

$L150:
        srlz.i                                              // M
        ;;
        ssm     1 << PSR_I                                  // M
        nop.i   0                                           // I

	adds	sp=16, sp				    // M
	mov	b0=r37;;				    // I
	mov.i	ar.pfs=r38;;				    // I

	nop.m	0                                           // M, enable interrupt
	mov.i	ar.lc=r39				    // I
	br.ret.sptk.few b0;;				    // B


        LEAF_EXIT(KiPurgeTranslationCache)
