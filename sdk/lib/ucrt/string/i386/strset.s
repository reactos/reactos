#include <asm.inc>
#if 0
        page    ,132
        title   strset - set all characters of string to character
;***
;strset.asm - sets all charcaters of string to given character
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;
;Purpose:
;       defines _strset() - sets all of the characters in a string (except
;       the '\0') equal to a given character.
;
;*******************************************************************************

        .xlist
        include cruntime.inc
        .list

page
;***
;char *_strset(string, val) - sets all of string to val
;
;Purpose:
;       Sets all of characters in string (except the terminating '/0'
;       character) equal to val.
;
;       Algorithm:
;       char *
;       _strset (string, val)
;             char *string;
;             char val;
;             {
;             char *start = string;
;
;             while (*string)
;                     *string++ = val;
;             return(start);
;             }
;
;Entry:
;       char *string - string to modify
;       char val - value to fill string with
;
;Exit:
;       returns string -- now filled with val's
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************
#endif

        .code

        public  __strset
.PROC __strset
// Prolog. Original sources used ML's extended PROC feature to autogenerate this.
        push ebp
        mov ebp, esp
        push edi // uses edi
#define string ebp + 8 // string:ptr byte
#define val ebp + 12 // val:byte


        mov     edi,[string]    // di = string
        mov     edx,edi         // dx=string addr; save return value

        xor     eax,eax         // ax = 0
        or      ecx,-1          // cx = -1
repne   scasb                   // scan string & count bytes
        add     ecx,2           // cx=-strlen
        neg     ecx             // cx=strlen
        mov     al,[val]        // al = byte value to store
        mov     edi,edx         // di=string addr
rep     stosb

        mov     eax,edx         // return value: string addr

// Epilog. Original sources used ML's extended PROC feature to autogenerate this.
        pop     edi
        pop     ebp

        ret                     // _cdecl return

.ENDP // __strset
        end
