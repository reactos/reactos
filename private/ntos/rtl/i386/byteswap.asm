	title  "Byte Swap Functions"
;++
;
; Copyright (c) 1997  Microsoft Corporation
;
; Module Name:
;
;    movemem.asm
;
; Abstract:
;
;    This module implements functions to perform byte swapping operations.
;
;
; Author:
;
;    Forrest Foltz (forrestf) 12-Dec-1997
;
; Environment:
;
;    User or Kernel mode.
;
; Revision History:
;
;--
.486p
	.xlist
include ks386.inc
include callconv.inc            ; calling convention macros
	.list 

;
; Alignment for functions in this module
;

CODE_ALIGNMENT macro
    align   16
endm


_TEXT$00   SEGMENT PARA PUBLIC 'CODE'
	ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

	PAGE 
	SUBTTL "RtlUshortByteSwap"
;++
;
; USHORT
; RtlUshortByteSwap(
;    IN USHORT Source
;    )
; 
; /*++
; 
; Routine Description:
; 
;    The RtlfUshortByteSwap function exchanges bytes 0 and 1 of Source
;    and returns the resulting USHORT.
; 
; Arguments:
; 
;    (cx) Source - 16-bit value to byteswap.
; 
; Return Value:
; 
;    Swapped 16-bit value.
;
;--

CODE_ALIGNMENT
				
cPublicFastCall RtlUshortByteSwap  ,1
cPublicFpo 0, 0

	mov     ah, cl
	mov     al, ch
	fstRET  RtlUshortByteSwap

fstENDP RtlUshortByteSwap


	PAGE 
	SUBTTL "RtlUlongByteSwap"
;++
;
; ULONG
; RtlUlongByteSwap(
;    IN ULONG Source
;    )
; 
; /*++
; 
; Routine Description:
; 
;    The RtlUlongByteSwap function exchanges byte pairs 0:3 and 1:2 of
;    Source and returns the the resulting ULONG.
; 
; Arguments:
; 
;    (ecx) Source - 32-bit value to byteswap.
; 
; Return Value:
; 
;    Swapped 32-bit value.
;
;--

CODE_ALIGNMENT
				
cPublicFastCall RtlUlongByteSwap  ,1
cPublicFpo 0, 0

	mov     eax, ecx
	bswap   eax
	fstRET  RtlUlongByteSwap

fstENDP RtlUlongByteSwap


	PAGE 
	SUBTTL "RtlUlonglongByteSwap"
;++
;
; ULONG
; RtlUlonglongByteSwap(
;    IN ULONGLONG Source
;    )
; 
; /*++
; 
; Routine Description:
; 
;    The RtlUlonglongByteSwap function exchanges byte pairs 0:7, 1:6, 2:5,
;    and 3:4 of Source and returns the resulting ULONGLONG.
; 
; Arguments:
; 
;    (edx:ecx) Source - 64-bit value to byteswap.
; 
; Return Value:
; 
;    Swapped 64-bit value.
;
;--

CODE_ALIGNMENT
				
cPublicFastCall RtlUlonglongByteSwap  ,1
cPublicFpo 0, 0

	mov     edx, eax
	mov     eax, ecx
	bswap   edx
	bswap   eax
	fstRET  RtlUlonglongByteSwap

fstENDP RtlUlonglongByteSwap

_TEXT$00   ends
	end
