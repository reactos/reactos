;
; COPYRIGHT:       See COPYING in the top level directory
; PROJECT:         ReactOS kernel
; FILE:            ntoskrnl/hal/x86/mpsboot.c
; PURPOSE:         Bootstrap code for application processors
; PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
; UPDATE HISTORY:
;                  Created 12/04/2001
;

;
; Memory map at this stage is:
;     0x2000  Location of our stack
;     0x3000  Startup code for the APs (this code)
;

;
; Magic value to be put in EAX when multiboot.S is called as part of the
; application processor initialization process
;
AP_MAGIC    equ 12481020h


X86_CR4_PAE equ 00000020h

;
; Segment selectors
;
%define KERNEL_CS     (0x8)
%define KERNEL_DS     (0x10)

section .text

global _APstart
global _APend

; 16 bit code
BITS 16

_APstart:
	cli		; Just in case

  xor   ax, ax
	mov		ds, ax
	mov		ss, ax

  mov   eax, 3000h + APgdt - _APstart
	lgdt  [eax]
	
  mov	eax, [2004h]	  ; Set the page directory
  mov   cr3, eax  
  
  mov	eax, [200ch]
  cmp	eax,0
  je	NoPae
  
  mov	eax,cr4
  or	eax,X86_CR4_PAE
  mov	cr4,eax
  
NoPae:  

  mov   eax, cr0
  or    eax, 80010001h    ; Turn on protected mode, paging and write protection
  mov   cr0, eax

  db    0eah
  dw    3000h + flush - _APstart, KERNEL_CS

; 32 bit code
BITS 32

flush:
  mov   ax, KERNEL_DS
  mov		ds, ax
  mov		es, ax
  mov		fs, ax
  mov		gs, ax
  mov		ss, ax

  ; Setup a stack for the AP
  mov   eax, 2000h
  mov   eax, [eax]
  mov   esp, eax

  ; Jump to start of the kernel with AP magic in ecx
  mov      ecx, AP_MAGIC
  mov	   eax,[2008h]
  jmp      eax

  ; Never get here


; Temporary GDT descriptor for the APs

APgdt:
; Limit
  dw  (3*8)-1
; Base
  dd	3000h + gdt - _APstart

gdt:
  dw	0x0       ; Null descriptor
  dw	0x0
  dw	0x0
  dw	0x0

  dw	0xffff    ; Kernel code descriptor
  dw	0x0000
  dw	0x9a00
  dw	0x00cf

  dw	0xffff    ;  Kernel data descriptor
  dw	0x0000
  dw	0x9200
  dw	0x00cf

_APend:
