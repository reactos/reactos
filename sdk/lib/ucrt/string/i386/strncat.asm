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

        .xlist
        include cruntime.inc
        .list

page
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

    CODESEG

    public  strncat
strncat proc
;   front:ptr byte,
;   back:ptr byte,
;   count:IWORD

        .FPO    ( 0, 3, 0, 0, 0, 0 )

        mov     ecx,[esp + 0ch]     ; ecx = count
        push    edi                 ; preserve edi
        test    ecx,ecx
        jz      finish              ; leave if count is zero

        mov     edi,[esp + 8]       ; edi -> front string
        push    esi                 ; preserve esi
        test    edi,3               ; is string aligned on dword (4 bytes)
        push    ebx                 ; preserve ebx
        je      short find_end_of_front_string_loop

        ; simple byte loop until string is aligned

front_misaligned:
        mov     al,byte ptr [edi]
        add     edi,1
        test    al,al
        je      short start_byte_3
        test    edi,3
        jne     short front_misaligned

find_end_of_front_string_loop:
        mov     eax,dword ptr [edi] ; read dword (4 bytes)
        mov     edx,7efefeffh
        add     edx,eax
        xor     eax,-1
        xor     eax,edx
        add     edi,4
        test    eax,81010100h
        je      short find_end_of_front_string_loop

; found zero byte in the loop
        mov     eax,[edi - 4]
        test    al,al               ; is it byte 0
        je      short start_byte_0
        test    ah,ah               ; is it byte 1
        je      short start_byte_1
        test    eax,00ff0000h       ; is it byte 2
        je      short start_byte_2
        test    eax,0ff000000h      ; is it byte 3
        jne     short find_end_of_front_string_loop
                                    ; taken if bits 24-30 are clear and bit
                                    ; 31 is set
start_byte_3:
        sub     edi,1
        jmp     short copy_start
start_byte_2:
        sub     edi,2
        jmp     short copy_start
start_byte_1:
        sub     edi,3
        jmp     short copy_start
start_byte_0:
        sub     edi,4

; edi now points to the end of front string.

copy_start:
        mov     esi,[esp + 14h]     ; esi -> back string
        test    esi,3               ; is back string is dword aligned?
        jnz     back_misaligned

        mov     ebx,ecx             ; store count for tail loop

        shr     ecx,2
        jnz     short main_loop_entrance
        jmp     short tail_loop_start   ; 0 < counter < 4

; simple byte loop until back string is aligned

back_misaligned:
        mov     dl,byte ptr [esi]
        add     esi,1
        test    dl,dl
        je      short byte_0
        mov     [edi],dl
        add     edi,1
        sub     ecx,1
        jz      empty_counter
        test    esi,3
        jne     short back_misaligned
        mov     ebx,ecx             ; store count for tail loop
        shr     ecx,2               ; convert ecx to dword count
        jnz     short main_loop_entrance

tail_loop_start:
        mov     ecx,ebx
        and     ecx,3               ; ecx = count of leftover bytes after the
                                    ; dwords have been concatenated
        jz      empty_counter

tail_loop:
        mov     dl,byte ptr [esi]
        add     esi,1
        mov     [edi],dl
        add     edi,1
        test    dl,dl
        je      short finish1       ; '\0' was already copied
        sub     ecx,1
        jnz     tail_loop

empty_counter:
        mov     [edi],cl            ; cl=0;
finish1:
        pop     ebx
        pop     esi
finish:
        mov     eax,[esp + 8]       ; return in eax pointer to front string
        pop     edi
        ret                         ; _cdecl return


byte_0:
        mov     [edi],dl
        mov     eax,[esp + 10h]     ; return in eax pointer to front string
        pop     ebx
        pop     esi
        pop     edi
        ret                         ; _cdecl return


main_loop:                          ; edx contains first dword of back string
        mov     [edi],edx           ; store one more dword
        add     edi,4               ; kick pointer to front string

        sub     ecx,1
        jz      tail_loop_start
main_loop_entrance:
        mov     edx,7efefeffh
        mov     eax,dword ptr [esi] ; read 4 bytes

        add     edx,eax
        xor     eax,-1

        xor     eax,edx
        mov     edx,[esi]           ; it's in cache now

        add     esi,4               ; kick pointer to back string
        test    eax,81010100h

        je      short main_loop

; may be found zero byte in the loop
        test    dl,dl               ; is it byte 0
        je      short byte_0
        test    dh,dh               ; is it byte 1
        je      short byte_1
        test    edx,00ff0000h       ; is it byte 2
        je      short byte_2
        test    edx,0ff000000h      ; is it byte 3
        jne short main_loop         ; taken if bits 24-30 are clear and bit
                                    ; 31 is set
byte_3:
        mov     [edi],edx
        mov     eax,[esp + 10h]     ; return in eax pointer to front string
        pop     ebx
        pop     esi
        pop     edi
        ret                         ; _cdecl return

byte_2:
        mov     [edi],dx
        xor     edx,edx
        mov     eax,[esp + 10h]     ; return in eax pointer to front string
        mov     [edi + 2],dl
        pop     ebx
        pop     esi
        pop     edi
        ret                         ; _cdecl return

byte_1:
        mov     [edi],dx
        mov     eax,[esp + 10h]     ; return in eax pointer to front string
        pop     ebx
        pop     esi
        pop     edi
        ret                         ; _cdecl return

strncat endp

        end

