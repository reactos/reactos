#include <asm.inc>
#if 0
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

        .xlist
        include cruntime.inc
        .list

page
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
#endif

        .code

        public  _strlen

.PROC _strlen
        // buf:ptr byte

        //OPTION PROLOGUE:NONE, EPILOGUE:NONE

        FPO    0, 1, 0, 0, 0, 0

#define string  [esp + 4]

        mov     ecx,string              // ecx -> string
        test    ecx,3                   // test if string is aligned on 32 bits
        je      short main_loop

str_misaligned:
        // simple byte loop until string is aligned
        mov     al,byte ptr [ecx]
        add     ecx,1
        test    al,al
        je      short byte_3
        test    ecx,3
        jne     short str_misaligned

        add     eax,dword ptr 0         // 5 byte nop to align label below

        align   16                      // should be redundant

main_loop:
        mov     eax,dword ptr [ecx]     // read 4 bytes
        mov     edx,HEX(7efefeff)
        add     edx,eax
        xor     eax,-1
        xor     eax,edx
        add     ecx,4
        test    eax,HEX(81010100)
        je      short main_loop
        // found zero byte in the loop
        mov     eax,[ecx - 4]
        test    al,al                   // is it byte 0
        je      short byte_0
        test    ah,ah                   // is it byte 1
        je      short byte_1
        test    eax,HEX(00ff0000)           // is it byte 2
        je      short byte_2
        test    eax,HEX(0ff000000)          // is it byte 3
        je      short byte_3
        jmp     short main_loop         // taken if bits 24-30 are clear and bit
                                        // 31 is set

byte_3:
        lea     eax,[ecx - 1]
        mov     ecx,string
        sub     eax,ecx
        ret
byte_2:
        lea     eax,[ecx - 2]
        mov     ecx,string
        sub     eax,ecx
        ret
byte_1:
        lea     eax,[ecx - 3]
        mov     ecx,string
        sub     eax,ecx
        ret
byte_0:
        lea     eax,[ecx - 4]
        mov     ecx,string
        sub     eax,ecx
        ret

.ENDP // _strlen

        end
