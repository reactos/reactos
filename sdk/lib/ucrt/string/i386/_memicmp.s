#include <asm.inc>
#if 0
        page        ,132
        title        memicmp - compare blocks of memory, ignore case
;***
;memicmp.asm - compare memory, ignore case
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;
;Purpose:
;       defines __ascii_memicmp() - compare two blocks of memory for lexical
;       order. Case is ignored in the comparison.
;
;*******************************************************************************

        .xlist
        include cruntime.inc
        .list

page
;***
;int __ascii_memicmp(first, last, count) - compare two blocks of memory, ignore case
;
;Purpose:
;       Compares count bytes of the two blocks of memory stored at first
;       and last.  The characters are converted to lowercase before
;       comparing (not permanently), so case is ignored in the search.
;
;       Algorithm:
;       int
;       _memicmp (first, last, count)
;               char *first, *last;
;               unsigned count;
;               {
;               if (!count)
;                       return(0);
;               while (--count && tolower(*first) == tolower(*last))
;                       {
;                       first++;
;                       last++;
;                       }
;               return(tolower(*first) - tolower(*last));
;               }
;
;Entry:
;       char *first, *last - memory buffers to compare
;       unsigned count - maximum length to compare
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

        public  ___ascii_memicmp
.PROC ___ascii_memicmp

// Prolog. Original sources used ML's extended PROC feature to autogenerate this.
        push ebp
        mov ebp, esp
        push edi
        push esi
        push ebx
#define first ebp + 8
#define last ebp + 12
#define count ebp + 16

        mov     ecx,[count]     // cx = count
        or      ecx,ecx
        jz      short toend     // if count=0, nothing to do

        mov     esi,[first]     // si = first
        mov     edi,[last]      // di = last

        // C locale

        mov     bh,'A'
        mov     bl,'Z'
        mov     dh,'a'-'A'      // add to cap to make lower

        align   4

lupe:
        mov     ah,[esi]        // ah = *first
        add     esi,1           // first++
        mov     al,[edi]        // al = *last
        add     edi,1           // last++

        cmp     ah,al           // test for equality BEFORE converting case
        je      short dolupe

        cmp     ah,bh           // ah < 'A' ??
        jb      short skip1

        cmp     ah,bl           // ah > 'Z' ??
        ja      short skip1

        add     ah,dh           // make lower case

skip1:
        cmp     al,bh           // al < 'A' ??
        jb      short skip2

        cmp     al,bl           // al > 'Z' ??
        ja      short skip2

        add     al,dh           // make lower case

skip2:
        cmp     ah,al           // *first == *last ??
        jne     short differ    // nope, found mismatched chars

dolupe:
        sub     ecx,1
        jnz     short lupe

        jmp     short toend     // cx = 0, return 0

differ:
        mov     ecx,-1          // assume last is bigger
                                // *** can't use "or ecx,-1" due to flags ***
        jb      short toend     // last is, in fact, bigger (return -1)
        neg     ecx             // first is bigger (return 1)

toend:
        mov     eax,ecx         // move return value to ax

// Epilog. Original sources used ML's extended PROC feature to autogenerate this.
        pop    ebx
        pop    esi
        pop    edi
        pop    ebp

        ret                     // _cdecl return

.ENDP // ___ascii_memicmp
        end
