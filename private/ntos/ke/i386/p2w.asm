        .286P
_TEXT   SEGMENT  WORD  PUBLIC 'CODE'
_TEXT      ENDS
_DATA   SEGMENT  WORD  PUBLIC 'DATA'
_DATA      ENDS
CONST   SEGMENT  WORD  PUBLIC 'CONST'
CONST      ENDS
_BSS    SEGMENT  WORD  PUBLIC 'BSS'
_BSS      ENDS
    DGROUP GROUP  _DATA, CONST, _BSS
    ASSUME CS:_TEXT, DS:DGROUP, ES:DGROUP, SS:DGROUP
PUBLIC  _p2w
EXTRN   _printf:NEAR

include callconv.inc        ; calling convention macros

_DATA   SEGMENT
s1      db  ' equ 0',0
s2      db  '%hX%04hXH',0ah,0
s3      db  '%hXH',0ah,0
_DATA   ends

_TEXT   segment

;
;   p2w(&ULONG which is value to print)
;
;   if ([bx+2] != 0)
;       printf(bx+2, bx, %x, %04x)
;   else
;       printf(bx, %x)

_p2w    PROC NEAR
; Line 688
        push    bp
        mov     bp, sp
        push    bx
        push    di
        push    si

        push    offset DGROUP:s1
        call    _printf
        add     sp,2

        mov     bx,[bp+4]
        cmp     word ptr [bx+2],0
        jz      p2w10

        push    [bx]
        push    [bx+2]
        push    offset DGROUP:s2
        call    _printf
        add     sp,6
        jmp     p2w20

p2w10:  push    [bx]
        push    offset DGROUP:s3
        call    _printf
        add     sp,4

p2w20:  pop     si
        pop     di
        pop     bx
        leave
        stdRET    _p2w
_p2w    ENDP

_TEXT   ENDS
END
