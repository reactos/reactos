/*
 //----------------------------------------------------------------------------
 //
 // File:     thunks.S
 //
 // Contains: Assembly code for the Sparc. Implements the dynamic vtable stuff
 //           and the tearoff code.
 //
 //----------------------------------------------------------------------------
*/

/* //#include <tearoff.hxx> */
#include "thunks_c.h"

#define this        %i0

#define vtbl        %l0
#define offset      %l1
#define pfn         %l2
#define index       %l3
#define dwMask      %l4
#define dwN	    %l5

/*
 //
 // Here's the layout of the 'this' object in $i0
 //
 // offset  value
 //
 //     0   don't care
 //     4   don't care
 //     8   don't care
 //     12  pvObject1's this
 //     16  pvObject1's function table
 //     20  pvObject2's this
 //     24  pvObject2's function table
 //     28  mask to decide whether to use Object 1 or 2
 //     32  index of method into vtbl
 //  Note:   Look in unixtearoff.cxx for latest structure definition
 //
*/

#define off_pvObject1           12
#define off_pvObjectVtbl1       16
#define off_pvObject2           20
#define off_dwMask              28
#define off_dwN			32

#define objvtblDelta           (off_pvObjectVtbl1 - off_pvObject1)
    
/*
 //----------------------------------------------------------------------------
 // Function:  _GetTearoff
 // 
 // Synopsis:  return the value left in global register (the pthunk pointer)
 //
 // Notes:     This pthunk pointer is left by THUNK_IMPLEMENT_COMARE 
 //            or THUNK_IMPLEMENT_SIMPLE
 //
 //----------------------------------------------------------------------------
*/
    .global _GetTearoff 
    .type   _GetTearoff,2

_GetTearoff:
    retl             
    mov %g2, %o0
 
/*
 //----------------------------------------------------------------------------
 //
 //  Function:  CompleteThunk
 //
 //  Synopsis:  Finishes up the complex and simple thunk handlers
 //
 //  Notes:     Modifies the this pointer and jumps to the corresponding virtual
 //             or non-virtual method.
 //
 //----------------------------------------------------------------------------
*/

#define COMPLETE_THUNK(n)                                                                       \
                                                                                                \
    .global cat(CompleteThunk,n);                                                               \
                                                                                                \
cat(CompleteThunk,n):                                                                           \
    ldsh    [vtbl + 8*(n+1)], offset;                   /* offset = vtbl[n].offset */           \
    lduw    [vtbl + 8*(n+1) + 4], pfn;                  /* pfn = vtbl[n].pfn */                 \
    ldsh    [vtbl + 8*(n+1) + 2], index;                /* index = vtbl[n].realVtblIndex */     \
                                                                                                \
    cmp     index,0;                                    /* if ( index == 0 ) */                 \
    beq       cat(NonVirtual,n);                        /*   goto NonVirtual; */                \
    nop;                                                                                        \
                                                                                                \
/* Virtual */                                                                                   \
    add     this,pfn,this;                              /* this += (vtbl_offset)pfn */          \
    lduw    [this],vtbl;                                /* vtbl = *this */                      \
    sub     this,pfn,this;                              /* this -= (vtbl_offset)pfn */          \
                                                                                                \
    sll     index,3,index;                              /* index *= sizeof(VTBLENTRY) */        \
    add     index,vtbl,vtbl;                            /* vtbl += index */                     \
                                                                                                \
    add     offset,this,this;                           /* this += offset */                    \
    lduw    [vtbl + 4], pfn;                            /* pfn = vtbl->pfn */                   \
    ldsh    [vtbl], offset;                             /* offset = vtbl->offset */             \
                                                                                                \
cat(NonVirtual,n):                                                                              \
    add     offset,this,this;                           /* this += offset; */                   \
    jmp     pfn;                                        /* func(); */                           \
    restore;


/*
 //----------------------------------------------------------------------------
 //
 //  Function:  TearOffCompareThunk
 //
 //  Synopsis:  The "handler" function that handles calls to torn-off interfaces
 //
 //  Notes:     Delegates to methods in the function pointer array held by
 //             the CTearOffThunk class
 //
 //----------------------------------------------------------------------------
*/


#define THUNK_IMPLEMENT_COMPARE(n)                                                                                      \
                                                                                                                        \
    /* No frame */                                                                                                      \
    /* No prologue */                                                                                                   \
                                                                                                                        \
    .global cat(TearoffThunk,n);                                                                                        \
    .type   cat(TearoffThunk,n),2;                                                                                      \
                                                                                                                        \
cat(TearoffThunk,n):                                                                                                    \
    save    %sp,-64,%sp;                                                                                                \
    mov     this, %g2;                                  /* save this -> %g2      */                                     \
    mov     n,  dwN;                                                                                                    \
    stw     dwN, [this+off_dwN];                                                                                        \
    lduw    [this+off_dwMask], dwMask;                  /* dwMask = this->dwMask */                                     \
    srl     dwMask, n, dwMask;                          /* dwMask >>= n; */                                             \
    btst    1, dwMask;                                  /* if (! (dwMask & 1 )) */                                      \
    be        cat(Object1_,n);                          /*   goto Object1_n; */                                         \
    nop;                                                                                                                \
                                                                                                                        \
    add     off_pvObject2-off_pvObject1, this, this;    /* this += &pvObject2 - &pvObject1; */                          \
                                                        /* This boosts the following 2 instructions from pvObject1 */   \
                                                        /* and apfnVtblObject1 to the corresponding 2nd values */       \
cat(Object1_,n):                                                                                                        \
    lduw    [this + off_pvObject1 + objvtblDelta], vtbl;/* vtbl = *(this + offset(TEAROFF_THUNK, apfnVtblObject1) */    \
    lduw    [this + off_pvObject1], this;               /* this += offset(TEAROFF_THUNK, pvObject1 ) */                 \
                                                                                                                        \
    COMPLETE_THUNK(n)

/*
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
*/

#define THUNK_IMPLEMENT_SIMPLE(n)                                                               \
                                                                                                \
    /* No frame */                                                                              \
    /* No prologue */                                                                           \
                                                                                                \
    .global cat(TearoffThunk,n);                                                                \
    .type   cat(TearoffThunk,n),2;                                                              \
                                                                                                \
cat(TearoffThunk,n):                                                                            \
    save    %sp,-64,%sp;                                                                        \
    mov     this, %g2;                                                                          \
    mov     n, dwN;                                                                             \
    stw     dwN, [this + off_dwN];                                                              \
    lduw    [this + off_pvObject1 + objvtblDelta], vtbl;/* vtbl = this->apfnVtblObject1 */      \
    lduw    [this + off_pvObject1], this;               /* this = this->pvObject1 */            \
                                                                                                \
    COMPLETE_THUNK(n)


/*
//
//      Define IUnknown thunks (0 - 2) only used by unixtearoff.cxx (simple)
//      Define the thunks from 3 to 15 (compare thunks)
//      Define the thunks from 16 onwards (simple thunks)
//
*/

THUNK_IMPLEMENT_SIMPLE(0)
THUNK_IMPLEMENT_SIMPLE(1)
THUNK_IMPLEMENT_SIMPLE(2)

THUNK_ARRAY_3_TO_15(IMPLEMENT_COMPARE)

THUNK_ARRAY_16_AND_UP(IMPLEMENT_SIMPLE)


/*
 //----------------------------------------------------------------------------
 //
 //  Function:  CMethodThunk::doThunk implementations
 //
 //  Synopsis:  The thunk that calls class method pointers correctly.
 //
 //  Notes:  The CMethodThunk struct has at offsets:
 //
 //                 0 : this
 //                 4 : vtbl method pointer
 //
 //          Also, we call CompleteThunk0 to complete our call.  However,
 //          it always skips 8 bytes to get to the first vtable method so
 //          we substract 8 from the vtbl method poner before calling in.
 //
 //----------------------------------------------------------------------------
*/

    .global doThunk
    .type   doThunk,2
               
    .global __0fMCMethodThunkHdoThunkPve
    .type   __0fMCMethodThunkHdoThunkPve,2

    .global __0fMCMethodThunkHdoThunkie
    .type   __0fMCMethodThunkHdoThunkie,2

    .global __0fMCMethodThunkHdoThunkv
    .type   __0fMCMethodThunkHdoThunkv,2

    .global __0fMCMethodThunkHdoThunk6F_GUIDe
    .type   __0fMCMethodThunkHdoThunk6F_GUIDe,2

    .global __0fMCMethodThunkHdoThunkR6QTextContextEvent
    .type   __0fMCMethodThunkHdoThunkR6QTextContextEvent,2

doThunk:
__0fMCMethodThunkHdoThunkR6QTextContextEvent:
__0fMCMethodThunkHdoThunkie:
__0fMCMethodThunkHdoThunkv:
__0fMCMethodThunkHdoThunk6F_GUIDe:
__0fMCMethodThunkHdoThunkPve:
    save    %sp,-64,%sp
    lduw    [this +4], vtbl    /* vtbl = this->pfnMethod */
    lduw    [this], this       /* this = this->pObject */

    ba      CompleteThunk0
    dec     8, vtbl            /* vtbl = vtbl - sizeof(VTABLE_ENTRY) */


