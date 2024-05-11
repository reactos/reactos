        page    ,132
        title   strcmp.asm - compare two strings
;***
;strcmp.asm - routine to compare two strings (for equal, less, or greater)
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;
;Purpose:
;       STRCMP compares two strings and returns an integer
;       to indicate whether the first is less than the second, the two are
;       equal, or whether the first is greater than the second, respectively.
;       Comparison is done byte by byte on an UNSIGNED basis, which is to
;       say that Null (0) is less than any other character (1-255).
;
;*******************************************************************************

        .xlist
        include cruntime.inc
        .list

page
;***
;strcmp - compare two strings, returning less than, equal to, or greater than
;
;Purpose:
;       Compares two string, determining their lexical order.  Unsigned
;       comparison is used.
;
;       Base Algorithm:
;          int strcmp ( const char *str1, const char *str2 )
;          {
;                  const unsigned char *src1 = (const unsigned char *)str1;
;                  const unsigned char *src2 = (const unsigned char *)str2;
;                  int ret = 0 ;
;
;                  while( ! (ret = *src1 - *src2) && *src2)
;                          ++src1, ++src2;
;
;                  if ( ret < 0 )
;                          ret = -1 ;
;                  else if ( ret > 0 )
;                          ret = 1 ;
;
;                  return( ret );
;          }
;
;Entry:
;       const char * src1 - string for left-hand side of comparison
;       const char * src2 - string for right-hand side of comparison
;
;Exit:
;       EAX < 0, 0, or >0, indicating whether the first string is
;       Less than, Equal to, or Greater than the second string.
;
;Uses:
;       ECX, EDX
;
;Exceptions:
;
;*******************************************************************************

        CODESEG

        public  strcmp
strcmp  proc \
        str1:ptr byte, \
        str2:ptr byte

        OPTION PROLOGUE:NONE, EPILOGUE:NONE

;       .FPO (cdwLocals, cdwParams, cbProlog, cbRegs, fUseBP, cbFrame)
        .FPO    ( 0, 2, 0, 0, 0, 0 )

        mov     edx,[esp + 4]   ; edx = src
        mov     ecx,[esp + 8]   ; ecx = dst

        test    edx,3
        jnz     short dopartial

        align   4
dodwords:
        mov     eax,[edx]

        cmp     al,[ecx]
        jne     short donene
        test    al,al
        jz      short doneeq
        cmp     ah,[ecx + 1]
        jne     short donene
        test    ah,ah
        jz      short doneeq

        shr     eax,16

        cmp     al,[ecx + 2]
        jne     short donene
        test    al,al
        jz      short doneeq
        cmp     ah,[ecx + 3]
        jne     short donene
        add     ecx,4
        add     edx,4
        test    ah,ah
        jnz     short dodwords

        align   4
doneeq:
        xor     eax,eax
        ret

        align   8
donene:
        ; The instructions below should place -1 in eax if src < dst,
        ; and 1 in eax if src > dst.

        sbb     eax,eax
        or      eax,1
        ret

        align   16
dopartial:
        test    edx,1
        jz      short doword

        mov     al,[edx]
        add     edx,1
        cmp     al,[ecx]
        jne     short donene
        add     ecx,1
        test    al,al
        jz      short doneeq

        test    edx,2
        jz      short dodwords


        align   4
doword:
        mov     ax,[edx]
        add     edx,2
        cmp     al,[ecx]
        jne     short donene
        test    al,al
        jz      short doneeq
        cmp     ah,[ecx + 1]
        jne     short donene
        test    ah,ah
        jz      short doneeq
        add     ecx,2
        jmp     short dodwords

strcmp  endp

        end
