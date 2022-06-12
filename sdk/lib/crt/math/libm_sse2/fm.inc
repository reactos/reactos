StackAllocate   MACRO size
    if          size ne 0
    sub         rsp, size
    .ALLOCSTACK size
    endif
ENDM

StackDeallocate   MACRO size
    if          size ne 0
    add         rsp, size
    endif
ENDM

SaveReg         MACRO reg64, offset
    mov         QWORD PTR [rsp+offset], reg64
    .SAVEREG    reg64, offset
ENDM

RestoreReg      MACRO reg64, offset
    mov         reg64, QWORD PTR [rsp+offset]
ENDM

SaveXmm         MACRO xmmreg, offset
    movdqa      XMMWORD PTR [offset+rsp], xmmreg
    .SAVEXMM128 xmmreg, offset
ENDM

RestoreXmm      MACRO xmmreg, offset
    movdqa      xmmreg, XMMWORD PTR [offset+rsp]
ENDM

AVXSaveXmm      MACRO xmmreg, offset
    vmovdqa     XMMWORD PTR [offset+rsp], xmmreg
    .SAVEXMM128 xmmreg, offset
ENDM

AVXRestoreXmm   MACRO xmmreg, offset
    vmovdqa     xmmreg, XMMWORD PTR [offset+rsp]
ENDM
