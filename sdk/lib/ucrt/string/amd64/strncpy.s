#include <asm.inc>
#include <ksamd64.inc>
.code64
#if 0
        page    ,132
        title   strncpy - copy at most n characters of string
;***
;strncpy.asm - copy at most n characters of string
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;
;Purpose:
;       defines strncpy() - copy at most n characters of string
;
;*******************************************************************************
; Look at strncat.asm for this file
include ksamd64.inc
        subttl  "strncpy"
#endif

LEAF_ENTRY_ARG3 strncpy, _TEXT, dst_ptr_byte, src_ptr_byte, count_dword
//OPTION PROLOGUE:NONE, EPILOGUE:NONE

// align the SOURCE so we never page fault
// dest pointer alignment not important

    mov     r11, rcx
    or      r8, r8
    jz      strncpy_exit
    sub     rcx, rdx // combine pointers
    test    dl, 7
    jz      qword_loop_entrance

copy_head_loop_begin:
    mov     al, [rdx]
    test    al, al
    mov     [rdx+rcx], al
    jz      filler
    inc     rdx
    dec     r8
    jz      strncpy_exit
    test    dl, 7
    jnz     copy_head_loop_begin
    jmp     qword_loop_entrance

strncpy_exit:
    mov     rax, r11
    ret

qword_loop_begin:
    mov     [rdx+rcx], rax
    add     rdx, 8
qword_loop_entrance:
    mov     rax, [rdx]
    sub     r8,  8
    jbe     qword_loop_end
    mov     r9, HEX(7efefefefefefeff)
    add     r9, rax
    mov     r10, rax
    xor     r10, -1
    xor     r10, r9
    mov     r9, HEX(8101010101010100)
    test    r10, r9
    jz      qword_loop_begin

qword_loop_end:
    add     r8, 8
    jz      strncpy_exit_2

    test    al, al
    mov     [rdx+rcx], al
    jz      filler
    inc     rdx
    dec     r8
    jz      strncpy_exit_2
    test    ah, ah
    mov     [rdx+rcx], ah
    jz      filler
    inc     rdx
    dec     r8
    jz      strncpy_exit_2
    shr     rax, 16
    test    al, al
    mov     [rdx+rcx], al
    jz      filler
    inc     rdx
    dec     r8
    jz      strncpy_exit_2
    test    ah, ah
    mov     [rdx+rcx], ah
    jz      filler
    inc     rdx
    dec     r8
    jz      strncpy_exit_2
    shr     rax, 16
    test    al, al
    mov     [rdx+rcx], al
    jz      filler
    inc     rdx
    dec     r8
    jz      strncpy_exit_2
    test    ah, ah
    mov     [rdx+rcx], ah
    jz      filler
    inc     rdx
    dec     r8
    jz      strncpy_exit_2
    shr     eax, 16
    test    al, al
    mov     [rdx+rcx], al
    jz      filler
    inc     rdx
    dec     r8
    jz      strncpy_exit_2
    test    ah, ah
    mov     [rdx+rcx], ah
    jz      filler
    inc     rdx
    dec     r8
    jnz     qword_loop_entrance

strncpy_exit_2:
    mov     rax, r11
    ret

//this is really just memset
filler:
    add     rcx, rdx
    xor     rdx, rdx
    cmp     r8, 16
    jb      tail // a quickie
aligner1:
    test    cl, 7
    jz      aligned
    inc     rcx
    mov     [rcx], dl
    dec     r8
    jmp     aligner1
aligned:
    sub     r8, 32
    jb      tail_8_enter
loop32:
    mov     [rcx], rdx
    mov     [rcx+8], rdx
    mov     [rcx+16], rdx
    mov     [rcx+24], rdx
    add     rcx, 32
    sub     r8, 32
    jae     loop32

tail_8_enter:
    add     r8, 32   // get back the value
tail_8_begin:
    sub     r8, 8
    jb      tail_enter
    mov     [rcx], rdx
    add     rcx, 8
    jmp     tail_8_begin

tail_enter:
    add     r8, 8   // get back the value
tail:
    sub     r8, 1
    jb      tail_finish
    mov     [rcx], dl
    inc     rcx
    jmp     tail
tail_finish:
    mov     rax, r11
    ret

LEAF_END strncpy, _TEXT

    end
