        page    ,132
        title   strncat - append n chars of string1 to string2
;***
;strncat.asm - append n chars of string to new string
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;
;Purpose:
;       defines strncat() - appends n characters of string onto
;       end of other string
;
;*******************************************************************************
include ksamd64.inc
        subttl  "strncat"
;***
;char *strncat(front, back, count) - append count chars of back onto front
;
;Purpose:
;       Appends at most count characters of the string back onto the
;       end of front, and ALWAYS terminates with a null character.
;       If count is greater than the length of back, the length of back
;       is used instead.  (Unlike strncpy, this routine does not pad out
;       to count characters).
;
;       Algorithm:
;       char *
;       strncat (front, back, count)
;           char *front, *back;
;           unsigned count;
;       {
;           char *start = front;
;
;           while (*front++)
;               ;
;           front--;
;           while (count--)
;               if (!(*front++ = *back++))
;                   return(start);
;           *front = '\0';
;           return(start);
;       }
;
;Entry:
;       char *   front - string to append onto
;       char *   back  - string to append
;       unsigned count - count of max characters to append
;
;Exit:
;       returns a pointer to string appended onto (front).
;
;Uses:  ECX, EDX
;
;Exceptions:
;
;*******************************************************************************
LEAF_ENTRY_ARG3 strncat, _TEXT, front:ptr byte, back:ptr byte, count:dword
    OPTION PROLOGUE:NONE, EPILOGUE:NONE

    mov     r11, rcx
    or      r8, r8
    jz      byte_exit
    test    cl, 7
    jz      strncat_loop_begin

strncat_copy_head_loop_begin:
    mov     al, [rcx]
    test    al, al
    jz      strncat_copy
    inc     rcx
    test    cl, 7
    jnz     strncat_copy_head_loop_begin

    nop

strncat_loop_begin:
    mov     rax, [rcx]
    mov     r10, rax
    mov     r9, 7efefefefefefeffh
    add     r9, r10
    xor     r10, -1
    xor     r10, r9
    add     rcx, 8
    mov     r9, 8101010101010100h
    test    r10, r9
    je      strncat_loop_begin
    sub     rcx, 8

strncat_loop_end:
    test    al, al
    jz      strncat_copy
    inc     rcx
    test    ah, ah
    jz      strncat_copy
    inc     rcx
    shr     rax, 16
    test    al, al
    jz      strncat_copy
    inc     rcx
    test    ah, ah
    jz      strncat_copy
    inc     rcx
    shr     rax, 16
    test    al, al
    jz      strncat_copy
    inc     rcx
    test    ah, ah
    jz      strncat_copy
    inc     rcx
    shr     eax, 16
    test    al, al
    jz      strncat_copy
    inc     rcx
    test    ah, ah
    jz      strncat_copy
    inc     rcx
    jmp     strncat_loop_begin

strncat_copy:
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
    dec     r8
    jz      byte_null_end
    test    dl, 7
    jnz     copy_head_loop_begin
    jmp     qword_loop_entrance

byte_null_end:
    xor     al, al
    mov     [rdx+rcx], al
byte_exit:
    mov     rax, r11
    ret

    nop

qword_loop_begin:
    mov     [rdx+rcx], rax
    add     rdx, 8
qword_loop_entrance:
    mov     rax, [rdx]
    sub     r8,  8
    jbe     qword_loop_end
    mov     r9, 7efefefefefefeffh
    add     r9, rax
    mov     r10, rax
    xor     r10, -1
    xor     r10, r9
    mov     r9, 8101010101010100h
    test    r10, r9
    jz      qword_loop_begin

qword_loop_end:
    add     r8, 8
    jz      strncat_exit_2

    test    al, al
    mov     [rdx+rcx], al
    jz      strncat_exit
    inc     rdx
    dec     r8
    jz      strncat_exit_2
    test    ah, ah
    mov     [rdx+rcx], ah
    jz      strncat_exit
    inc     rdx
    dec     r8
    jz      strncat_exit_2
    shr     rax, 16
    test    al, al
    mov     [rdx+rcx], al
    jz      strncat_exit
    inc     rdx
    dec     r8
    jz      strncat_exit_2
    test    ah, ah
    mov     [rdx+rcx], ah
    jz      strncat_exit
    inc     rdx
    dec     r8
    jz      strncat_exit_2
    shr     rax, 16
    test    al, al
    mov     [rdx+rcx], al
    jz      strncat_exit
    inc     rdx
    dec     r8
    jz      strncat_exit_2
    test    ah, ah
    mov     [rdx+rcx], ah
    jz      strncat_exit
    inc     rdx
    dec     r8
    jz      strncat_exit_2
    shr     eax, 16
    test    al, al
    mov     [rdx+rcx], al
    jz      strncat_exit
    inc     rdx
    dec     r8
    jz      strncat_exit_2
    test    ah, ah
    mov     [rdx+rcx], ah
    jz      strncat_exit
    inc     rdx
    dec     r8
    jnz     qword_loop_entrance

strncat_exit_2:
    xor     al, al
    mov     [rdx+rcx], al
strncat_exit:
    mov     rax, r11
    ret

LEAF_END strncat, _TEXT

    end
