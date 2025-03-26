#include <asm.inc>
#if 0
        page    ,132
        title   strncmp.asm - compare two strings
;***
;strcmp.asm - routine to compare two strings (for equal, less, or greater)
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;
;Purpose:
;       strncmp compares two strings and returns an integer
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
;strncmp - compare two strings, returning less than, equal to, or greater than
;
;Purpose:
;       Compares two string, determining their lexical order.  Unsigned
;       comparison is used.
;
;       Algorithm:
;          int strncmp (
;              const char *first,
;              const char *last,
;              size_t      count
;          )
;          {
;              size_t x;
;              int ret = 0 ;
;
;              for (x = 0; x < count; x++)
;              {
;                  if (*first == 0 || *first != *last)
;                  {
;                      int ret = (*(unsigned char *)first - *(unsigned char *)last);
;                      if ( ret < 0 )
;                          ret = -1 ;
;                      else if ( ret > 0 )
;                          ret = 1 ;
;                      return ret;
;                  }
;                  ++first;
;                  ++last;
;              }
;
;              return 0;
;          }
;
;Entry:
;       const char * first - string for left-hand side of comparison
;       const char * last  - string for right-hand side of comparison
;       size_t count       - maximum number of characters to compare; if
;                             strings equal to that point consider them equal
;
;Exit:
;       EAX < 0, 0, or >0, indicating whether the first string is
;       Less than, Equal to, or Greater than the second string.
;
;       Note: For compatibility with other versions of this routine
;       the returned value is limited to {-1, 0, 1}.
;
;Uses:
;       ECX, EDX
;
;Exceptions:
;
;*******************************************************************************
#endif

        .code

#define CHAR_TYPE BYTE
#define CHAR_PTR BYTE PTR
#define CHAR_SIZE 1 // = sizeof CHAR_TYPE

#define BLK_TYPE DWORD
#define BLK_PTR DWORD PTR
#define BLK_SIZE 4 // = sizeof BLK_TYPE
#define BLK_CHARS 4 // = BLK_SIZE / CHAR_SIZE

PAGE_SIZE = HEX(1000)
PAGE_MASK = PAGE_SIZE - 1       // mask for offset in MM page
PAGE_SAFE_BLK = PAGE_SIZE - BLK_SIZE // maximum offset for safe block compare

    public  _strncmp
.PROC _strncmp
        // uses ebx esi, \
        // str1:ptr byte, \
        // str2:ptr byte, \
        // count:IWORD

    //OPTION PROLOGUE:NONE, EPILOGUE:NONE

    push      ebx
    push      esi

//   .FPO (cdwLocals, cdwParams, cbProlog, cbRegs, fUseBP, cbFrame)
    FPO      0, 3, ($ - _strncmp), 2, 0, 0

    mov       ecx,[esp + 12]   // ecx = str1
    mov       edx,[esp + 16]   // edx = str2
    mov       ebx,[esp + 20]   // ebx = count

// Check for a limit of zero characters.
    test      ebx, HEX(0FFFFFFFF)
    jz        return_equal

    sub       ecx, edx

    test      edx, (BLK_SIZE - 1)
    jz        dword_loop_begin

comp_head_loop_begin:
    movzx     eax, CHAR_PTR[ecx+edx]
    cmp       al, CHAR_PTR[edx]
    jnz       return_not_equal

    test      eax, eax
    jz        return_equal

    inc       edx

    sub       ebx, 1
    jbe       return_equal

    test      dl, (BLK_SIZE - 1)
    jnz       comp_head_loop_begin

dword_loop_begin:
    lea       eax, [ecx+edx]
    and       eax, PAGE_MASK
    cmp       eax, PAGE_SAFE_BLK
    ja        comp_head_loop_begin

    mov       eax, BLK_PTR[ecx+edx]
    cmp       eax, BLK_PTR[edx]
    jne       comp_head_loop_begin

    sub       ebx, BLK_CHARS
    jbe       return_equal

    lea       esi, [eax+HEX(0fefefeff)]
    add       edx, BLK_SIZE
    not       eax
    and       eax, esi
    test      eax, HEX(80808080)
    jz        dword_loop_begin

return_equal:
    xor       eax, eax
    pop       esi
    pop       ebx
    ret

    align     16

return_not_equal:
    sbb       eax, eax  // AX=-1, CY=1 AX=0, CY=0
    or        eax, 1
    pop       esi
    pop       ebx
    ret

.ENDP // _strncmp

    end
