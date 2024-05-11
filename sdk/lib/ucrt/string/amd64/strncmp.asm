        page    ,132
        title   strncmp - compare first n chars of two strings
;***
;strncmp.asm - compare first n characters of two strings
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;
;Purpose:
;       defines strncmp() - compare first n characters of two strings
;       for ordinal order.
;
;*******************************************************************************
include ksamd64.inc
        subttl  "strncmp"
;***
;int strncmp(first, last, count) - compare first count chars of strings
;
;Purpose:
;       Compares two strings for ordinal order.  The comparison stops
;       after: (1) a difference between the strings is found, (2) the end
;       of the strings is reached, or (3) count characters have been
;       compared.
;
;       Algorithm:
;       int
;       strncmp (first, last, count)
;             char *first, *last;
;             unsigned count;
;             {
;             if (!count)
;                     return(0);
;             while (--count && *first && *first == *last)
;                     {
;                     first++;
;                     last++;
;                     }
;             return(*first - *last);
;             }
;
;Entry:
;       char *first, *last - strings to compare
;       unsigned count - maximum number of characters to compare
;
;Exit:
;       returns <0 if first < last
;       returns 0 if first == last
;       returns >0 if first > last
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

LEAF_ENTRY_ARG3 strncmp, _TEXT, str1:ptr byte, str2:ptr byte, count:dword

    OPTION PROLOGUE:NONE, EPILOGUE:NONE

; rcx = first
; rdx = last
; r8 = count

    sub       rdx, rcx

    test      r8, r8
    jz        return_equal

    test      ecx, 7
    jz        qword_loop_enter

comp_head_loop_begin:
    movzx     eax, byte ptr[rcx]
    cmp       al, byte ptr[rdx+rcx]
    jne       return_not_equal

    inc       rcx

    dec       r8
    jz        return_equal

    test      al, al
    jz        return_equal

    test      rcx, 7
    jnz       comp_head_loop_begin

qword_loop_enter:
    mov       r11, 08080808080808080h
    mov       r10, 0fefefefefefefeffh

qword_loop_begin:
    lea       eax, [rdx+rcx]
    and       eax, 0fffh
    cmp       eax, 0ff8h
    ja        comp_head_loop_begin

    mov       rax, qword ptr[rcx]
    cmp       rax, qword ptr[rdx+rcx]
    jne       comp_head_loop_begin

    add       rcx, 8

    sub       r8, 8
    jbe       return_equal

    lea       r9, [r10+rax]
    not       rax
    and       rax, r9
    test      rax, r11  ; 8080808080808080h

    jz        qword_loop_begin

return_equal:
    xor       eax, eax
    ret

;    align     16

return_not_equal:
    sbb       rax, rax  ; AX=-1, CY=1 AX=0, CY=0
    or        rax, 1
    ret

LEAF_END strncmp, _TEXT
        end
