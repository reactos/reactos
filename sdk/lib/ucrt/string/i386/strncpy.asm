        page    ,132
        title   strncpy - copy at most n characters of string
;***
;strncpy.asm - copy at most n characters of string
;
;       Copyright (c) Microsoft Corporation. All rights reserved.
;
;Purpose:
;       defines strncpy() - copy at most n characters of string
;
;*******************************************************************************

        .xlist
        include cruntime.inc
        .list

page
;***
;char *strncpy(dest, source, count) - copy at most n characters
;
;Purpose:
;       Copies count characters from the source string to the
;       destination.  If count is less than the length of source,
;       NO NULL CHARACTER is put onto the end of the copied string.
;       If count is greater than the length of sources, dest is padded
;       with null characters to length count.
;
;       Algorithm:
;       char *
;       strncpy (dest, source, count)
;       char *dest, *source;
;       unsigned count;
;       {
;         char *start = dest;
;
;         while (count && (*dest++ = *source++))
;             count--;
;         if (count)
;             while (--count)
;                 *dest++ = '\0';
;         return(start);
;       }
;
;Entry:
;       char *dest     - pointer to spot to copy source, enough space
;                        is assumed.
;       char *source   - source string for copy
;       unsigned count - characters to copy
;
;Exit:
;       returns dest, with the character copied there.
;
;Uses:
;
;Exceptions:
;
;*******************************************************************************

        CODESEG

        public  strncpy
strncpy proc \
        dest:ptr byte, \
        source:ptr byte, \
        count:dword

        OPTION PROLOGUE:NONE, EPILOGUE:NONE

        .FPO    ( 0, 3, 0, 0, 0, 0 )

        mov     ecx,[esp + 0ch]     ; ecx = count
        push    edi                 ; preserve edi
        test    ecx,ecx
        jz      finish              ; leave if count is zero

        push    esi                 ; preserve edi
        push    ebx                 ; preserve ebx
        mov     ebx,ecx             ; store count for tail loop
        mov     esi,[esp + 14h]     ; esi -> source string
        test    esi,3               ; test if source string is aligned on 32 bits
        mov     edi,[esp + 10h]     ; edi -> dest string
        jnz     short src_misaligned    ; (almost always source is aligned)

        shr     ecx,2               ; convert ecx to dword count
        jnz     main_loop_entrance
        jmp     short copy_tail_loop    ; 0 < count < 4

; simple byte loop until string is aligned

src_misaligned:
        mov     al,byte ptr [esi]   ; copy a byte from source to dest
        add     esi,1
        mov     [edi],al
        add     edi,1
        sub     ecx,1
        jz      fill_tail_end1      ; if count == 0, leave
        test    al,al               ; was last copied byte zero?
        jz      short align_dest    ; if so, go align dest and pad it out
                                    ; with zeros
        test    esi,3               ; esi already aligned ?
        jne     short src_misaligned
        mov     ebx,ecx             ; store count for tail loop
        shr     ecx,2
        jnz     short main_loop_entrance

tail_loop_start:
        and     ebx,3               ; ebx = count_before_main_loop%4
        jz      short fill_tail_end1    ; if ebx == 0 then leave without
                                        ; appending a null byte

; while ( EOS (end-of-string) not found and count > 0 ) copy bytes

copy_tail_loop:
        mov     al,byte ptr [esi]   ; load byte from source
        add     esi,1
        mov     [edi],al            ; store byte to dest
        add     edi,1
        test    al,al               ; EOS found?
        je      short fill_tail_zero_bytes  ; '\0' was already copied
        sub     ebx,1
        jnz     copy_tail_loop
fill_tail_end1:
        mov     eax,[esp + 10h]     ; prepare return value
        pop     ebx
        pop     esi
        pop     edi
        ret

; EOS found. Pad with null characters to length count

align_dest:
        test    edi,3               ; dest string aligned?
        jz      dest_align_loop_end
dest_align_loop:
        mov     [edi],al
        add     edi,1
        sub     ecx,1               ; count == 0?
        jz      fill_tail_end       ; if so, finished
        test    edi,3               ; is edi aligned ?
        jnz     dest_align_loop
dest_align_loop_end:
        mov     ebx,ecx             ; ebx > 0
        shr     ecx,2               ; convert ecx to count of dwords
        jnz     fill_dwords_with_EOS
        ; pad tail bytes
finish_loop:                        ; 0 < ebx < 4
        mov     [edi],al
        add     edi,1
fill_tail_zero_bytes:
        sub     ebx,1
        jnz     finish_loop
        pop     ebx
        pop     esi
finish:
        mov     eax,[esp + 8]       ; return in eax pointer to dest string
        pop     edi
        ret

; copy (source) string to (dest). Also look for end of (source) string

main_loop:                          ; edx contains first dword of source string
        mov     [edi],edx           ; store one more dword
        add     edi,4               ; kick dest pointer
        sub     ecx,1
        jz      tail_loop_start

main_loop_entrance:
        mov     edx,7efefeffh
        mov     eax,dword ptr [esi] ; read 4 bytes (dword)
        add     edx,eax
        xor     eax,-1
        xor     eax,edx
        mov     edx,[esi]           ; it's in cache now
        add     esi,4               ; kick dest pointer
        test    eax,81010100h
        je      short main_loop

        ; may have found zero byte in the dword

        test    dl,dl               ; is it byte 0
        je      short byte_0
        test    dh,dh               ; is it byte 1
        je      short byte_1
        test    edx,00ff0000h       ; is it byte 2
        je      short byte_2
        test    edx,0ff000000h      ; is it byte 3
        jne     short main_loop     ; taken if bits 24-30 are clear and bit
                                    ; 31 is set

; a null character was found, so dest needs to be padded out with null chars
; to count length.

        mov     [edi],edx
        jmp     short fill_with_EOS_dwords

byte_2:
        and     edx,0ffffh          ; fill high 2 bytes with 0
        mov     [edi],edx
        jmp     short fill_with_EOS_dwords

byte_1:
        and     edx,0ffh            ; fill high 3 bytes with 0
        mov     [edi],edx
        jmp     short fill_with_EOS_dwords

byte_0:
        xor     edx,edx             ; fill whole dword with 0
        mov     [edi],edx

; End of string was found. Pad out dest string with dwords of 0

fill_with_EOS_dwords:               ; ecx > 0   (ecx is dword counter)
        add     edi,4
        xor     eax,eax             ; it is instead of ???????????????????
        sub     ecx,1
        jz      fill_tail           ; we filled all dwords

fill_dwords_with_EOS:
        xor     eax,eax
fill_with_EOS_loop:
        mov     [edi],eax
        add     edi,4
        sub     ecx,1
        jnz     short fill_with_EOS_loop
fill_tail:                          ; let's pad tail bytes with zero
        and     ebx,3               ; ebx = ebx % 4
        jnz     finish_loop         ; taken, when there are some tail bytes
fill_tail_end:
        mov     eax,[esp + 10h]
        pop     ebx
        pop     esi
        pop     edi
        ret

strncpy endp
        end



