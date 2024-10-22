        title   strcat - concatenate (append) one string to another
;***
;strcat.asm - contains strcat() and strcpy() routines
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;
;Purpose:
;       STRCAT concatenates (appends) a copy of the source string to the
;       end of the destination string, returning the destination string.
;
;*******************************************************************************
include ksamd64.inc
        subttl  "strcat"
;***
;char *strcat(dst, src) - concatenate (append) one string to another
;
;Purpose:
;       Concatenates src onto the end of dest.  Assumes enough
;       space in dest.
;
;       Algorithm:
;       char * strcat (char * dst, char * src)
;       {
;           char * cp = dst;
;
;           while( *cp )
;                   ++cp;           /* Find end of dst */
;           while( *cp++ = *src++ )
;                   ;               /* Copy src to end of dst */
;           return( dst );
;       }
;
;Entry:
;       char *dst - string to which "src" is to be appended
;       const char *src - string to be appended to the end of "dst"
;
;Exit:
;       The address of "dst" in EAX
;
;Uses:
;       EAX, ECX
;
;Exceptions:
;
;*******************************************************************************

;***
;char *strcpy(dst, src) - copy one string over another
;
;Purpose:
;       Copies the string src into the spot specified by
;       dest; assumes enough room.
;
;       Algorithm:
;       char * strcpy (char * dst, char * src)
;       {
;           char * cp = dst;
;
;           while( *cp++ = *src++ )
;                   ;               /* Copy src over dst */
;           return( dst );
;       }
;
;Entry:
;       char * dst - string over which "src" is to be copied
;       const char * src - string to be copied over "dst"
;
;Exit:
;       The address of "dst" in EAX
;
;Uses:
;       EAX, ECX
;
;Exceptions:
;*******************************************************************************

public ___entry_from_strcat_in_strcpy
LEAF_ENTRY_ARG2 strcat, _TEXT, dst:ptr byte, src:ptr byte

    OPTION PROLOGUE:NONE, EPILOGUE:NONE

    mov     r11, rcx
    test    cl, 7
    jz      strcat_loop_begin

strcat_copy_head_loop_begin:
    mov     al, [rcx]
    test    al, al
    jz      ___entry_from_strcat_in_strcpy
    inc     rcx
    test    cl, 7
    jnz     strcat_copy_head_loop_begin

strcat_loop_begin:
    mov     rax, [rcx]
    mov     r10, rax
    mov     r9, 7efefefefefefeffh
    add     r9, r10
    xor     r10, -1
    xor     r10, r9
    add     rcx, 8
    mov     r9, 8101010101010100h
    test    r10, r9
    je      strcat_loop_begin
    sub     rcx, 8

strcat_loop_end:
    test    al, al
    jz      ___entry_from_strcat_in_strcpy
    inc     rcx
    test    ah, ah
    jz      ___entry_from_strcat_in_strcpy
    inc     rcx
    shr     rax, 16
    test    al, al
    jz      ___entry_from_strcat_in_strcpy
    inc     rcx
    test    ah, ah
    jz      ___entry_from_strcat_in_strcpy
    inc     rcx
    shr     rax, 16
    test    al, al
    jz      ___entry_from_strcat_in_strcpy
    inc     rcx
    test    ah, ah
    jz      ___entry_from_strcat_in_strcpy
    inc     rcx
    shr     eax, 16
    test    al, al
    jz      ___entry_from_strcat_in_strcpy
    inc     rcx
    test    ah, ah
    jz      ___entry_from_strcat_in_strcpy
    inc     rcx
    jmp     strcat_loop_begin

LEAF_END strcat, _TEXT

LEAF_ENTRY_ARG2 strcpy, _TEXT, dst:ptr byte, src:ptr byte

    OPTION PROLOGUE:NONE, EPILOGUE:NONE

    mov     r11, rcx
strcat_copy:
___entry_from_strcat_in_strcpy=strcat_copy
 ; align the SOURCE so we never page fault
 ; dest pointer alignment not important
    sub     rcx, rdx ; combine pointers
    test    dl, 7
    jz      qword_loop_entrance

copy_head_loop_begin:
    mov     al, [rdx]
    mov     [rdx+rcx], al
    test    al, al
    jz      byte_exit
    inc     rdx
    test    dl, 7
    jnz     copy_head_loop_begin
    jmp     qword_loop_entrance

byte_exit:
    mov     rax, r11
    ret

qword_loop_begin:
    mov     [rdx+rcx], rax
    add     rdx, 8
qword_loop_entrance:
    mov     rax, [rdx]
    mov     r9, 7efefefefefefeffh
    add     r9, rax
    mov     r10, rax
    xor     r10, -1
    xor     r10, r9
    mov     r9, 8101010101010100h
    test    r10, r9
    jz      qword_loop_begin

qword_loop_end:
    test    al, al
    mov     [rdx+rcx], al
    jz      strcat_exit
    inc     rdx
    test    ah, ah
    mov     [rdx+rcx], ah
    jz      strcat_exit
    inc     rdx
    shr     rax, 16
    test    al, al
    mov     [rdx+rcx], al
    jz      strcat_exit
    inc     rdx
    test    ah, ah
    mov     [rdx+rcx], ah
    jz      strcat_exit
    inc     rdx
    shr     rax, 16
    test    al, al
    mov     [rdx+rcx], al
    jz      strcat_exit
    inc     rdx
    test    ah, ah
    mov     [rdx+rcx], ah
    jz      strcat_exit
    inc     rdx
    shr     eax, 16
    test    al, al
    mov     [rdx+rcx], al
    jz      strcat_exit
    inc     rdx
    test    ah, ah
    mov     [rdx+rcx], ah
    jz      strcat_exit
    inc     rdx
    jmp     qword_loop_entrance

strcat_exit:
    mov     rax, r11
    ret

LEAF_END strcpy, _TEXT

    end
