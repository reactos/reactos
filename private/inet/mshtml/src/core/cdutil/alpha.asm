 //----------------------------------------------------------------------------
 //
 // File:     alpha.asm
 //
 // Contains: Assembly code for the Alpha. Implements the dynamic vtable stuff
 //           and the tearoff code.
 //
 //----------------------------------------------------------------------------

.globl DynLinkFunc
.globl CallTornOffMethod

#include <dvtbl.h>

offsetof_pvObject         = 12  // Must be kept in sync with the source code
offsetof_apfn             = 16  // Ditto


rIndex    = $t0
rThis     = $a0

rTemp     = $t1
rTmp2     = $t2
rpFnTable = $t3

.align 3

 // Ensure that DynLinkFunc stays exactly how we expect
.set noreorder
.set nomacro

 //----------------------------------------------------------------------------
 //
 //  Function:  DynLinkFunc
 //
 //  Synopsis:  The code that is put into the dynamic vtable thunks
 //
 //  Notes:     The various 0x7000 constants are replaced on the fly with
 //             appropriate values by InitDynamicVtable.
 //
 //             NOTE: If the MSB of the low word of the offset is 1, then
 //             the high word of the offset must have 1 added to it, since
 //             these instructions sign extend the low word and add it to
 //             the high word.
 //
 //----------------------------------------------------------------------------
.ent DynLinkFunc

DynLinkFunc:

    lda  rIndex, 0x7000($zero) // Store the index (replace 7000 w/ index)
    ldl  rTemp,  0x0(rThis)    // Move vtable pointer into reg
    ldah rTemp,  0x7000(rTemp) // Add high word of offset to vtable ptr
    ldl  rTemp,  0x7000(rTemp) // Add (sign extended) low word of offset to the
                               //   vtable ptr & get the "handler" function ptr
    jmp  (rTemp)               // Call the "handler" function
                               // The offset is the offset between g_pvtbl
                               //   and g_apfnThunkers, computed and replaced
                               //   by InitDynamicVtable.
.end DynLinkFunc

.set macro
.set reorder

 //----------------------------------------------------------------------------
 //
 //  Function:  CallTornOffMethod
 //
 //  Synopsis:  The "handler" function that handles calls to torn-off interfaces
 //
 //  Notes:     Delegates to methods in the function pointer array held by
 //             the CTearOffThunk class
 //
 //----------------------------------------------------------------------------

.align 3


.ent CallTornOffMethod

CallTornOffMethod:

    // Value in rIndex was stored by DynLinkFunc

    ldl    rpFnTable, offsetof_apfn(rThis)          // Get table of functions

    // NOTE: Uncomment the following line if DVTBL_OFFSET_TEAROFF is nonzero.
    // subl   rIndex, DVTBL_OFFSET_TEAROFF, rIndex  // Adjust thunk index

    subl   rIndex, NUMBER_CTIUNKNOWN_METHODS, rTemp // Are we calling IUnknown?
    ldl    rTmp2,  offsetof_pvObject(rThis)         // Get object's this ptr
    s4addl rIndex, rpFnTable, rpFnTable             // Get the entry in table
    cmovge rTemp,  rTmp2, rThis                     // Store new this ptr if
                                                    //   not calling IUnknown
    ldl    rTemp, 0x0(rpFnTable)                    // Get the fn pointer
    jmp    (rTemp)                                  // Call function

.end CallTornOffMethod
