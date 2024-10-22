        page    ,132
        title   strlen - return the length of a null-terminated string
;***
;strlen.asm - contains strlen() routine
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;
;Purpose:
;       strlen returns the length of a null-terminated string,
;       not including the null byte itself.
;
;*******************************************************************************
include ksamd64.inc
        subttl  "strlen"
;***
;strlen - return the length of a null-terminated string
;
;Purpose:
;       Finds the length in bytes of the given string, not including
;       the final null character.
;
;       Algorithm:
;       int strlen (const char * str)
;       {
;           int length = 0;
;
;           while( *str++ )
;                   ++length;
;
;           return( length );
;       }
;
;Entry:
;       const char * str - string whose length is to be computed
;
;Exit:
;       EAX = length of the string "str", exclusive of the final null byte
;
;Uses:
;       EAX, ECX, EDX
;
;Exceptions:
;
;*******************************************************************************

LEAF_ENTRY_ARG1 strlen, _TEXT, buf:ptr byte

    OPTION PROLOGUE:NONE, EPILOGUE:NONE

    mov   rax, rcx
    neg   rcx          ; for later
    test  rax, 7
    jz    main_loop_entry

byte 066h, 090h

byte_loop_begin:
    mov   dl, [rax]
    inc   rax
    test  dl, dl
    jz    return_byte_7
    test  al, 7
    jnz   byte_loop_begin

main_loop_entry:
    mov   r8, 7efefefefefefeffh
    mov   r11, 8101010101010100h

main_loop_begin:
    mov   rdx, [rax]

    mov   r9,  r8
    add   rax, 8
    add   r9,  rdx
    not   rdx
    xor   rdx, r9
    and   rdx, r11
    je    main_loop_begin

main_loop_end:

    mov   rdx, [rax-8]

    test  dl, dl
    jz    return_byte_0
    test  dh, dh
    jz    return_byte_1
    shr   rdx, 16
    test  dl, dl
    jz    return_byte_2
    test  dh, dh
    jz    return_byte_3
    shr   rdx, 16
    test  dl, dl
    jz    return_byte_4
    test  dh, dh
    jz    return_byte_5
    shr   edx, 16
    test  dl, dl
    jz    return_byte_6
    test  dh, dh
    jnz   main_loop_begin

return_byte_7:
    lea   rax, [rax+rcx-1]
    ret
return_byte_6:
    lea   rax, [rax+rcx-2]
    ret
return_byte_5:
    lea   rax, [rax+rcx-3]
    ret
return_byte_4:
    lea   rax, [rax+rcx-4]
    ret
return_byte_3:
    lea   rax, [rax+rcx-5]
    ret
return_byte_2:
    lea   rax, [rax+rcx-6]
    ret
return_byte_1:
    lea   rax, [rax+rcx-7]
    ret
return_byte_0:
    lea   rax, [rax+rcx-8]
    ret

LEAF_END strlen, _TEXT
    end
