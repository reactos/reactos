.intel_syntax noprefix

.globl @_MOV_DD_SWP@8
.globl @_MOV_DW_SWP@8
.globl @_MOV_SWP_DW2DD@8

.func @_MOV_DD_SWP@8, @_MOV_DD_SWP@8
@_MOV_DD_SWP@8:
    mov   eax,[edx]
    bswap eax
    mov   [ecx],eax
    ret
.endfunc

.func @_MOV_DW_SWP@8, @_MOV_DW_SWP@8
@_MOV_DW_SWP@8:
    mov   ax,[edx]
    rol   ax,8
    mov   [ecx],ax
    ret
.endfunc

.func @_MOV_SWP_DW2DD@8, @_MOV_SWP_DW2DD@8
@_MOV_SWP_DW2DD@8:
    xor   eax,eax
    mov   ax,[edx]
    rol   ax,8
    mov   [ecx],eax
    ret
.endfunc
