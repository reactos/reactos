 //----------------------------------------------------------------------------
 //
 // File:     thunks.s
 //
 // Contains: Assembly code for the Alpha. Implements the dynamic vtable stuff
 //           and the tearoff code.
 //
 // NOTE!!: This file (after preprocessing) is what would normally be passed
 //         to ASAXP.EXE, the Alpha assembler.  Unfortunately that causes the
 //         assembler to GPF.  Instead the preprocessed version is hand-edited
 //         to replace all semicolons with newlines, remove all unnecessary
 //         spaces and replace all macros.
 //
 //----------------------------------------------------------------------------


#include "ksia64.h"


//
// These must be kept in sync with TEAROFF_THUNK structure offsets
//

#define offsetof_papfnVtblThis		0x00
#define offsetof_ulRef				0x08
#define offsetof_apIID				0x10		// pointer is 8 byte aligned
#define offsetof_pvObject1			0x18
#define offsetof_apfnVtblObject1	0x20
#define offsetof_pvObject2			0x28
#define offsetof_apfnVtblObject2	0x30
#define offsetof_dwMask				0x38
#define offsetof_n					0x3C
#define offsetof_apVtblPropDesc		0x40

#define ptr_width                   0x8



 //----------------------------------------------------------------------------
 //
 //  Function:  TearOffCompareThunk
 //
 //  Synopsis:  The "handler" function that handles calls to torn-off interfaces
 //
 //  Notes:     Delegates to methods in the function pointer array held by
 //             the CTearOffThunk class
 //
 //  In:		Pointer to TEAROFF_THUNK
 //----------------------------------------------------------------------------


#define THUNK_C_1(n)    LEAF_ENTRY(TearoffThunk##n)
#define THUNK_C_2(n)		add		t3 = offsetof_dwMask, a0		        /* pointer to mask */
#define THUNK_C_3(n)        add     t7 = r0, a0                             /* store the tearoff ptr in t7 */
#define THUNK_C_4(n)        add     t8 = offsetof_n, a0                     /* pointer to vtbl index */
#define THUNK_C_5(n)		;;
#define THUNK_C_6(n)		ld4		t4 = [t3]                               /* get mask */
#define THUNK_C_7(n)		add		t5 = offsetof_pvObject2, a0             /* pointer to pvObject2 */
#define THUNK_C_8(n)		add     t6 = offsetof_pvObject1, a0             /* pointer to pvObject1 */
#define THUNK_C_9(n)		;;
#define THUNK_C_10(n)       mov     t9 = n
#define THUNK_C_11(n)		tbit.nz	pt0, pt1 = t4, n                        /* see if we are using pvObject2 */ 
#define THUNK_C_12(n)       ;;
#define THUNK_C_13(n)       st8     [t8] = t9                               /* save the vtbl index */
#define THUNK_C_14(n) (pt0) mov     t3 = t5                                 /* pvObject2 */
#define THUNK_C_15(n) (pt1) mov     t3 = t6                                 /* pvObject1 */
#define THUNK_C_16(n)       ;;
#define THUNK_C_17(n)       ld8     a0 = [t3]                               /* this->_pvObject goes to param 1*/
#define THUNK_C_18(n)       add     t4 = ptr_width, t3       
#define THUNK_C_19(n)       shl     t9 = t9, 3                              /* multiply index by ptr_width */
#define THUNK_C_20(n)       ;;
#define THUNK_C_21(n)       ld8     t3 = [t4]                               /* ptr to apfnVtblObj */
#define THUNK_C_22(n)       ;;
#define THUNK_C_23(n)       add     t5 = t9, t3                             /* increment by index * ptr size */
#define THUNK_C_24(n)       ;;
#define THUNK_C_25(n)       mov     bt0 = t5
#define THUNK_C_26(n)       br.cond.sptk    bt0                             /* jump to the function */
#define THUNK_C_27(n)   LEAF_EXIT(TearoffThunk##n)
#define THUNK_C_28(n)
#define THUNK_C_29(n)
#define THUNK_C_30(n)


 //----------------------------------------------------------------------------
 //
 //  Function:  GetTearoff
 //
 //  Synopsis:  This function returns the tearoff thunk pointer stored in
 //             the temp register t7. This should be called first thing from
 //             the C++ functions that handles calls to torn-off interfaces
 //
 //----------------------------------------------------------------------------

#define THUNK_GT_1		LEAF_ENTRY(_GetTearoff)
#define THUNK_GT_2			add		v0 = r0, t7       /* place tearoff ptr in return reg */
#define THUNK_GT_3		LEAF_EXIT(_GetTearoff)
#define THUNK_GT_4
#define THUNK_GT_5
#define THUNK_GT_6
#define THUNK_GT_7
#define THUNK_GT_8
#define THUNK_GT_9


//
//      Define the thunks from 3 to 15 (these are compare thunks)
//

#include "..\thunks_c.h"

 //----------------------------------------------------------------------------
 //
 //  Function:  CallTearOffSimpleThunk
 //
 //  Synopsis:  The "handler" function that handles calls to torn-off interfaces
 //
 //  Notes:     Delegates to methods in the function pointer array held by
 //             the CTearOffThunk class
 //
 //----------------------------------------------------------------------------



#define THUNK_S_1(n)    LEAF_ENTRY(TearoffThunk##n)
#define THUNK_S_2(n)        mov     t7 = a0                                 /* Save tearoff in t7 */
#define THUNK_S_3(n)        mov     t9 = n                                  /* We need n in a register */
#define THUNK_S_4(n)        add     t3 = offsetof_pvObject1, a0             /* this->pObject */ 
#define THUNK_S_5(n)        ;;
#define THUNK_S_6(n)        ld8     t3 = [t3]                               /* Get pObject */
#define THUNK_S_7(n)        add     t4 = offsetof_apfnVtblObject1, a0       /* this->apfnVtblObject */
#define THUNK_S_8(n)        add     t5 = offsetof_n, a0                     /* this->n */
#define THUNK_S_9(n)        ;;
#define THUNK_S_10(n)       ld8     t4 = [t4]                               /* apfnVtblObject */
#define THUNK_S_11(n)       shl     t6 = t9, 3                              /* multiply n by ptr_width */
#define THUNK_S_12(n)       mov     a0 = t3                                 /* pObject goes to param 1 */
#define THUNK_S_13(n)       ;;
#define THUNK_S_14(n)       st8     [t5] = t9                               /* Save the index */
#define THUNK_S_15(n)       add     t4 = t6, t4                             /* get function ptr */
#define THUNK_S_16(n)       ;;
#define THUNK_S_17(n)       mov     bt0 = t4
#define THUNK_S_18(n)       br.cond.sptk    bt0                             /* jump to the function */
#define THUNK_S_19(n)    LEAF_EXIT(TearoffThunk##n)
#define THUNK_S_20(n)


//
//      Define the thunks from 16 onwards (these are simple thunks)
//

#include "..\thunks_s.h"

// .set macro
// .set reorder
