        page    ,132
        title   strspn - search for init substring of chars from control str
;***
;strspn.asm - find length of initial substring of chars from a control string
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;
;Purpose:
;       defines strspn() - finds the length of the initial substring of
;       a string consisting entirely of characters from a control string.
;
;       defines strcspn()- finds the length of the initial substring of
;       a string consisting entirely of characters not in a control string.
;
;       defines strpbrk()- finds the index of the first character in a string
;       that is not in a control string
;
;*******************************************************************************

        .xlist
        include cruntime.inc
        .list

page
;***
;int strspn(string, control) - find init substring of control chars
;
;Purpose:
;       Finds the index of the first character in string that does belong
;       to the set of characters specified by control.  This is
;       equivalent to the length of the initial substring of string that
;       consists entirely of characters from control.  The '\0' character
;       that terminates control is not considered in the matching process.
;
;       Algorithm:
;       int
;       strspn (string, control)
;               unsigned char *string, *control;
;       {
;               unsigned char map[32];
;               int count;
;
;               for (count = 0; count < 32; count++)
;                       map[count] = 0;
;               while (*control)
;               {
;                       map[*control >> 3] |= (1 << (*control & 7));
;                       control++;
;               }
;               if (*string)
;               {
;                       while (map[*string >> 3] & (1 << (*string & 7)))
;                       {
;                               count++;
;                               string++;
;                       }
;                       return(count);
;               }
;               return(0);
;       }
;
;Entry:
;       char *string - string to search
;       char *control - string containing characters not to search for
;
;Exit:
;       returns index of first char in string not in control
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

;***
;int strcspn(string, control) - search for init substring w/o control chars
;
;Purpose:
;       returns the index of the first character in string that belongs
;       to the set of characters specified by control.  This is equivalent
;       to the length of the length of the initial substring of string
;       composed entirely of characters not in control.  Null chars not
;       considered.
;
;       Algorithm:
;       int
;       strcspn (string, control)
;               unsigned char *string, *control;
;       {
;               unsigned char map[32];
;               int count;
;
;               for (count = 0; count < 32; count++)
;                       map[count] = 0;
;               while (*control)
;               {
;                       map[*control >> 3] |= (1 << (*control & 7));
;                       control++;
;               }
;               map[0] |= 1;
;               while (!(map[*string >> 3] & (1 << (*string & 7))))
;               {
;                       count++;
;                       string++;
;               }
;               return(count);
;       }
;
;Entry:
;       char *string - string to search
;       char *control - set of characters not allowed in init substring
;
;Exit:
;       returns the index of the first char in string
;       that is in the set of characters specified by control.
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

;***
;char *strpbrk(string, control) - scans string for a character from control
;
;Purpose:
;       Finds the first occurence in string of any character from
;       the control string.
;
;       Algorithm:
;       char *
;       strpbrk (string, control)
;               unsigned char *string, *control;
;       {
;               unsigned char map[32];
;               int count;
;
;               for (count = 0; count < 32; count++)
;                       map[count] = 0;
;               while (*control)
;               {
;                       map[*control >> 3] |= (1 << (*control & 7));
;                       control++;
;               }
;               while (*string)
;               {
;                       if (map[*string >> 3] & (1 << (*string & 7)))
;                               return(string);
;                       string++;
;               }
;               return(NULL);
;       }
;
;Entry:
;       char *string - string to search in
;       char *control - string containing characters to search for
;
;Exit:
;       returns a pointer to the first character from control found
;       in string.
;       returns NULL if string and control have no characters in common.
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************


ifdef SSTRCSPN

    _STRSPN_ equ <strcspn>

elseifdef SSTRPBRK

    _STRSPN_ equ <strpbrk>

else  ; SSTRCSPN

; Default is to build strspn()

    SSTRSPN equ 1
    _STRSPN_ equ <strspn>

endif  ; SSTRCSPN

% public  _STRSPN_

    CODESEG

_STRSPN_ proc \
        uses esi, \
        string:ptr byte, \
        control:ptr byte

; create and zero out char bit map

        xor     eax,eax
        push    eax             ; 32
        push    eax
        push    eax
        push    eax             ; 128
        push    eax
        push    eax
        push    eax
        push    eax             ; 256

map     equ     [esp]

; Set control char bits in map

        mov     edx,control     ; si = control string

        align   @WordSize
lab listnext                    ; init char bit map
        mov     al,[edx]
        or      al,al
        jz      short listdone
        add     edx,1
        bts     map,eax
        jmp     short listnext

lab listdone

; Loop through comparing source string with control bits

        mov     esi,string      ; si = string

_ifnd   SSTRPBRK <or     ecx,-1> ; set ecx to -1

        align   @WordSize
lab dstnext

_ifnd   SSTRPBRK <add    ecx,1>

        mov     al,[esi]
        or      al,al
        jz      short dstdone
        add     esi,1
        bt      map, eax

ifdef SSTRSPN
        jc      short dstnext   ; strspn: found char, continue
elseifdef SSTRCSPN
        jnc     short dstnext   ; strcspn: did not find char, continue
elseifdef SSTRPBRK
        jnc     short dstnext   ; strpbrk: did not find char, continue
        lea     eax,[esi - 1]   ; found char, return address of it
endif  ; SSTRSPN

; Return code

lab dstdone

_ifnd   SSTRPBRK <mov   eax,ecx> ; strspn/strcspn: return index

        add     esp,32

        ret                     ; _cdecl return

_STRSPN_ endp
         end
