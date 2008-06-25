
//
// @name GENERATE_INT_HANDLER
//
// This macro creates an unexpected interrupt handler.
//
// @param None.
//
// @remark None.
//
.macro GENERATE_ARM_STUB Name
.global &Name
.func &Name
&Name:
    b .
.endfunc
.endm
