#include <asm.inc>
#if 0
        page    ,132
        title   strnicmp - compare n chars of strings, ignore case
;***
;strnicmp.asm - compare n chars of strings, ignoring case
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;
;Purpose:
;       defines __ascii_strnicmp() - Compares at most n characters of two
;       strings, without regard to case.
;
;*******************************************************************************

        .xlist
        include cruntime.inc
        .list

page
;***
;int __ascii_strnicmp(first, last, count) - compares count char of strings,
;       ignore case
;
;Purpose:
;       Compare the two strings for lexical order.  Stops the comparison
;       when the following occurs: (1) strings differ, (2) the end of the
;       strings is reached, or (3) count characters have been compared.
;       For the purposes of the comparison, upper case characters are
;       converted to lower case.
;
;       Algorithm:
;       int
;       _strncmpi (first, last, count)
;             char *first, *last;
;             unsigned int count;
;             {
;             int f,l;
;             int result = 0;
;
;             if (count) {
;                     do      {
;                             f = tolower(*first);
;                             l = tolower(*last);
;                             first++;
;                             last++;
;                             } while (--count && f && l && f == l);
;                     result = f - l;
;                     }
;             return(result);
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
#endif

        .code

        public  ___ascii_strnicmp
.PROC ___ascii_strnicmp
// Prolog. Original sources used ML's extended PROC feature to autogenerate this.
        push ebp
        mov ebp, esp
        push edi // uses edi esi ebx
        push esi
        push ebx
#define first ebp + 8 // first:ptr byte
#define last ebp + 12 // last:ptr byte
#define count ebp + 16 // count:IWORD

        mov     ecx,[count]     // cx = byte count
        or      ecx,ecx
        jz      toend           // if count = 0, we are done

        mov     esi,[first]     // si = first string
        mov     edi,[last]      // di = last string

        mov     bh,'A'
        mov     bl,'Z'
        mov     dh,'a'-'A'      // add to cap to make lower

        align   4

lupe:
        mov     ah,[esi]        // *first

        or      ah,ah           // see if *first is null

        mov     al,[edi]        // *last

        jz      short eject     //   jump if *first is null

        or      al,al           // see if *last is null
        jz      short eject     //   jump if so

        add     esi,1           // first++
        add     edi,1           // last++

        cmp     ah,bh           // 'A'
        jb      short skip1

        cmp     ah,bl           // 'Z'
        ja      short skip1

        add     ah,dh           // make lower case

skip1:
        cmp     al,bh           // 'A'
        jb      short skip2

        cmp     al,bl           // 'Z'
        ja      short skip2

        add     al,dh           // make lower case

skip2:
        cmp     ah,al           // *first == *last ??
        jne     short differ

        sub     ecx,1
        jnz     short lupe

eject:
        xor     ecx,ecx
        cmp     ah,al           // compare the (possibly) differing bytes
        je      short toend     // both zero; return 0

differ:
        mov     ecx,-1          // assume last is bigger (* can't use 'or' *)
        jb      short toend     // last is, in fact, bigger (return -1)
        neg     ecx             // first is bigger (return 1)

toend:
        mov     eax,ecx

// Epilog. Original sources used ML's extended PROC feature to autogenerate this.
        pop    ebx
        pop    esi
        pop    edi
        pop    ebp

        ret                     // _cdecl return

.ENDP // ___ascii_strnicmp
         end
