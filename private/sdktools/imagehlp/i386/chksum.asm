	title  "Compute Checksum"

;/*++
;
; Copyright (c) 1992  Microsoft Corporation
;
; Module Name:
;
;    chksum.asm
;
; Abstract:
;
;    This module implements a fucntion to compute the checksum of a buffer.
;
; Author:
;
;    David N. Cutler (davec) 27-Jan-1992
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
;--*/

        .386
        .model  small,c

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include callconv.inc
        include ks386.inc
        .list

        .code

;++
;
; USHORT
; ChkSum(
;   IN ULONG cksum,
;   IN PUSHORT buf,
;   IN ULONG len
;   )
;
; Routine Description:
;
;    This function computes the checksum of the specified buffer.
;
; Arguments:
;
;    cksum - Suppiles the initial checksum value.
;
;    buf - Supplies a pointer to the buffer that is checksumed.
;
;    len - Supplies the of the buffer in words.
;
; Return Value:
;
;    The computed checksum is returned as the function value.
;
;--

cksum   equ     8                       ; stack offset to initial checksum
buf     equ     12                      ; stack offset to source address
len     equ     16                      ; stack offset to length in words

cPublicProc ChkSum,3

	push	esi                     ; save nonvolatile register
        mov     ecx,[esp + len]         ; get length in words
        mov     esi,[esp + buf]         ; get source address
        mov     eax,[esp + cksum]       ; get initial checksum
        shl     ecx,1                   ; convert to length in bytes
        jz      cks80                   ; if z set, no words to checksum

;
; Compute checksum in cascading order of block size until 128 byte blocks
; are all that is left, then loop on 128-bute blocks.
;

        test    esi,02h                 ; check if source dword aligned
        jz      short cks10             ; if z set, source is dword aligned
        sub     edx,edx                 ; get initial word for alignment
        mov     dx,[esi + 0]            ;
        add     eax,edx                 ; update partial checkcum
        adc     eax,0                   ; add carry
        add     esi,2                   ; update source address
        sub     ecx,2                   ; reduce length in bytes
cks10:  mov     edx,ecx                 ; isolate residual bytes
        and     edx,07h                 ;
        sub     ecx,edx                 ; subtract residual bytes
        jz      cks60                   ; if z set, no 8-byte blocks
        test    ecx,08h                 ; test if initial 8-byte block
        jz      short cks20             ; if z set, no initial 8-byte block
        add     eax,[esi + 0]           ; compute 8-byte checksum
        adc     eax,[esi + 4]           ;
        adc     eax,0                   ; add carry
        add     esi,8                   ; update source address
        sub     ecx,8                   ; reduce length of checksum
        jz      cks60                   ; if z set, end of 8-byte blocks
cks20:  test    ecx,010h                ; test if initial 16-byte block
        jz      short cks30             ; if z set, no initial 16-byte block
        add     eax,[esi + 0]           ; compute 16-byte checksum
        adc     eax,[esi + 4]           ;
        adc     eax,[esi + 8]           ;
        adc     eax,[esi + 12]          ;
        adc     eax,0                   ; add carry
        add     esi,16                  ; update source address
        sub     ecx,16                  ; reduce length of checksum
        jz      cks60                   ; if z set, end of 8-byte blocks
cks30:  test    ecx,020h                ; test if initial 32-byte block
        jz      short cks40             ; if z set, no initial 32-byte block
        add     eax,[esi + 0]           ; compute 32-byte checksum
        adc     eax,[esi + 4]           ;
        adc     eax,[esi + 8]           ;
        adc     eax,[esi + 12]          ;
        adc     eax,[esi + 16]          ;
        adc     eax,[esi + 20]          ;
        adc     eax,[esi + 24]          ;
        adc     eax,[esi + 28]          ;
        adc     eax,0                   ; add carry
        add     esi,32                  ; update source address
        sub     ecx,32                  ; reduce length of checksum
        jz      cks60                   ; if z set, end of 8-byte blocks
cks40:  test    ecx,040h                ; test if initial 64-byte block
        jz      cks50                   ; if z set, no initial 64-byte block
        add     eax,[esi + 0]           ; compute 64-byte checksum
        adc     eax,[esi + 4]           ;
        adc     eax,[esi + 8]           ;
        adc     eax,[esi + 12]          ;
        adc     eax,[esi + 16]          ;
        adc     eax,[esi + 20]          ;
        adc     eax,[esi + 24]          ;
        adc     eax,[esi + 28]          ;
        adc     eax,[esi + 32]          ;
        adc     eax,[esi + 36]          ;
        adc     eax,[esi + 40]          ;
        adc     eax,[esi + 44]          ;
        adc     eax,[esi + 48]          ;
        adc     eax,[esi + 52]          ;
        adc     eax,[esi + 56]          ;
        adc     eax,[esi + 60]          ;
        adc     eax,0                   ; add carry
        add     esi,64                  ; update source address
        sub     ecx,64                  ; reduce length of checksum
        jz      short cks60             ; if z set, end of 8-byte blocks
cks50:  add     eax,[esi + 0]           ; compute 64-byte checksum
        adc     eax,[esi + 4]           ;
        adc     eax,[esi + 8]           ;
        adc     eax,[esi + 12]          ;
        adc     eax,[esi + 16]          ;
        adc     eax,[esi + 20]          ;
        adc     eax,[esi + 24]          ;
        adc     eax,[esi + 28]          ;
        adc     eax,[esi + 32]          ;
        adc     eax,[esi + 36]          ;
        adc     eax,[esi + 40]          ;
        adc     eax,[esi + 44]          ;
        adc     eax,[esi + 48]          ;
        adc     eax,[esi + 52]          ;
        adc     eax,[esi + 56]          ;
        adc     eax,[esi + 60]          ;
        adc     eax,[esi + 64]          ;
        adc     eax,[esi + 68]          ;
        adc     eax,[esi + 72]          ;
        adc     eax,[esi + 76]          ;
        adc     eax,[esi + 80]          ;
        adc     eax,[esi + 84]          ;
        adc     eax,[esi + 88]          ;
        adc     eax,[esi + 92]          ;
        adc     eax,[esi + 96]          ;
        adc     eax,[esi + 100]         ;
        adc     eax,[esi + 104]         ;
        adc     eax,[esi + 108]         ;
        adc     eax,[esi + 112]         ;
        adc     eax,[esi + 116]         ;
        adc     eax,[esi + 120]         ;
        adc     eax,[esi + 124]         ;
        adc     eax,0                   ; add carry
        add     esi,128                 ; update source address
        sub     ecx,128                 ; reduce length of checksum
        jnz     short cks50             ; if z clear, not end of 8-byte blocks

;
; Compute checksum on 2-byte blocks.
;

cks60:  test    edx,edx                 ; check if any 2-byte blocks
        jz      short cks80             ; if z set, no 2-byte blocks
cks70:  sub     ecx,ecx                 ; load 2-byte block
        mov     cx,[esi + 0]            ;
        add     eax,ecx                 ; compue 2-byte checksum
        adc     eax,0                   ;
        add     esi,2                   ; update source address
        sub     edx,2                   ; reduce length of checksum
        jnz     short cks70             ; if z clear, more 2-bytes blocks

;
; Fold 32-but checksum into 16-bits
;

cks80:  mov     edx,eax                 ; copy checksum value
        shr     edx,16                  ; isolate high order bits
        and     eax,0ffffh              ; isolate low order bits
        add     eax,edx                 ; sum high and low order bits
        mov     edx,eax                 ; isolate possible carry
        shr     edx,16                  ;
        add     eax,edx                 ; add carry
        and     eax,0ffffh              ; clear possible carry bit
	pop     esi                     ; restore nonvolatile register
        stdRET  ChkSum

stdENDP ChkSum

	end

