
.macro TEXTAREA
    .section text, "rx"
    .align 2
.endm

.macro NESTED_ENTRY Name
    .global &Name
    .align 2
    .func &Name
    &Name:
.endm

.macro PROLOG_END Name
    prolog_&Name:
.endm

.macro ENTRY_END Name
    end_&Name:
   .endfunc
.endm
