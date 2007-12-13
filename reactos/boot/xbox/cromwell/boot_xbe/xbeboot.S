// Xbox Linux XBE Bootloader
//
//  Copyright (C) 2002 Michael Steil & anonymous
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General 
// Public License for more details.
//
// The latest version of the GPL can be retrieved at:
// http://www.gnu.org/licenses/gpl.html .
//
// Xbox is a trademark or registered trademark of Microsoft Corporation.
// No relationship between the author(s) and Microsoft Corporation exists or is
// implied.

// Constants

// Size of a page on x86
#define PAGE_SIZE			4096
// Win32 read/write protection mode.  x86 has no flag for executability.
#define PAGE_READWRITE			4
// Final address of Linux kernel in RAM
#define KERNEL_ADDRESS			0x00100000
// Minimum physical address at which the kernel can reside temporarily
#define MIN_KERNEL_ADDRESS		KERNEL_ADDRESS
// Memory reserved to the kernel (1 MB at the moment)
#define KERNEL_MAX_SIZE			0x00100000 // 1MB maximum Kernel for Cromwell
// Location of kernel in RAM
#define KERNEL_RAM_ADDRESS		0x03A00000
// Size of memory on a retail XBOX unit
#define RETAIL_RAM_SIZE			0x04000000
// Reserve pages in address space
#define MEM_RESERVE			0x00002000
// Commit physical memory to pages
#define MEM_COMMIT			0x00001000
// CR0 bit to block write-through
#define CR0_DISABLE_WRITE_THROUGH	0x20000000
// CR0 bit to disable caching
#define CR0_DISABLE_CACHE		0x40000000
// CR0 bit to enable paging
#define CR0_ENABLE_PAGING		0x80000000

#define FRAMEBUFFER 0xF0040240
#define TEXTBUFFER (FRAMEBUFFER + 80 * 640 * 4) 


//#define DebugOut

/* located within bootsect space (copied from arch/i386/setup.c) */
/* originally defined mostly in arch/i386/kernel/setup.c and head.S */

PARAM		=	0
SCREEN_INFO	=	PARAM+0
EXT_MEM_K	=	PARAM+2
CL_MAGIC	=	PARAM+0x20
CL_OFFSET	=	PARAM+0x22
ALT_MEM_K	=	PARAM+0x01e0
APM_BIOS_INFO	=	PARAM+0x0040
DRIVE_INFO	=	PARAM+0x0080
SYS_DESC_TABLE	=	PARAM+0x00A0
SETUP_SECTS	=	PARAM+0x01F1
MOUNT_ROOT_RDO	=	PARAM+0x01F2
SYSSIZE		=	PARAM+0x01F4
RAMDISK_FLAGS	=	PARAM+0x01F8
ORIG_ROOT_DEV	=	PARAM+0x01FC
AUX_DEVICE_INFO	=	PARAM+0x01FF

/* located within and following the setup space: */

LOADER_TYPE	=	PARAM+0x0210
LOAD_FLAGS	=	PARAM+0x0211
KERNEL_START	=	PARAM+0x0214
INITRD_START	=	PARAM+0x0218
INITRD_SIZE	=	PARAM+0x021C
COMMAND_LINE	=	PARAM+0x0800
REL_COMMAND_LINE	=	0x0800

PARAM_CURSOR_POS	= PARAM+0x00
PARAM_VIDEO_PAGE	= PARAM+0x04
PARAM_VIDEO_MODE	= PARAM+0x06
PARAM_VIDEO_COLS	= PARAM+0x07
PARAM_VIDEO_EGA_BX	= PARAM+0x0a
PARAM_VIDEO_LINES	= PARAM+0x0e
PARAM_HAVE_VGA		= PARAM+0x0f
PARAM_FONT_POINTS	= PARAM+0x10

PARAM_LFB_WIDTH		= PARAM+0x12
PARAM_LFB_HEIGHT	= PARAM+0x14
PARAM_LFB_DEPTH		= PARAM+0x16
PARAM_LFB_BASE		= PARAM+0x18
PARAM_LFB_SIZE		= PARAM+0x1c
PARAM_LFB_LINELENGTH	= PARAM+0x24
PARAM_LFB_COLORS	= PARAM+0x26
PARAM_VESAPM_SEG	= PARAM+0x2e
PARAM_VESAPM_OFF	= PARAM+0x30
PARAM_LFB_PAGES		= PARAM+0x32


.code32

.text
// This includes a minimal XBE header. It's not complete, i.e. software that wants
// to decode the header, such as "xbedump" (or probably Microsoft's certification
// tools) are likely to fail, but it is a valid header for the Xbox kernel.
#include "xbe.S"


.org 0x1000
section_1_start:		// this should be 0x11000
// kernel thunk table
MmAllocateContiguousMemoryEx:
	.long	0x80000000 + 166
MmGetPhysicalAddress:
	.long	0x80000000 + 173
NtAllocateVirtualMemory:
	.long	0x80000000 + 184

	.long	0		// end of table

.org 0x1080
cromwellstart:
	.long 0x3000		// Start of the ROM image
cromwellsize:	
	.long 0x00060000	// Size  of the ROM image   (this Value will be overwritten by Imagebld)

.org 0x1100
entry:				// this should be 0x11100

.globl _start
.intel_syntax noprefix
_start:




    #ifdef DebugOut
    mov ebx, 0		// x position
    mov ecx, 0		// y position

    lea esi, text1 
    mov eax, MmAllocateContiguousMemoryEx
    call strhexout

    lea esi, text2
    mov eax, MmGetPhysicalAddress
    call strhexout

    lea esi, text3
    mov eax, NtAllocateVirtualMemory
    call strhexout 
  
    lea esi, text_kernel
    mov eax, offset kernel
    call strhexout 
    #endif

// 1. use MmAllocateContiguousMemoryEx to allocate physical memory
//    somewhere between 1 and 32 MB for the kernel
    push ebx
    push ecx

        push	PAGE_READWRITE
        push	0
        push	RETAIL_RAM_SIZE / 2 - 1
	push	MIN_KERNEL_ADDRESS + (KERNEL_MAX_SIZE) 
        push	(KERNEL_MAX_SIZE)
        call	dword ptr [MmAllocateContiguousMemoryEx]
        mov	[kernelram], eax

    pop ecx
    pop ebx

    #ifdef DebugOut
    push eax
    lea esi, text_kernelram
    call strhexout
    pop eax		// kernelram
    #endif
    
    
// 3. MmGetPhysicalAddress that memory
    push ebx
    push ecx

        push	dword ptr [kernelram]
        call	dword ptr [MmGetPhysicalAddress]
        mov	[phys_kernelram], eax

    pop ecx
    pop ebx

    #ifdef DebugOut
    lea esi, text_phys_kernelram
    call strhexout 
    #endif

// 2. put kernel there
    
    // UWEI
    push ecx
    mov esi, offset kernel
    mov edi, dword ptr [kernelram]
    mov ecx, [cromwellsize]    
    rep movsb
    pop ecx

// 4. use MmAllocateContiguousMemoryEx to allocate 4096 bytes of memory
//	  for the boot loader between 16 MB and 32 MB (virtual memory should
//	  be unallocated there)
    push ebx
    push ecx

        push	PAGE_READWRITE
        push	16
        push	RETAIL_RAM_SIZE / 2
        push	RETAIL_RAM_SIZE / 4
        push	PAGE_SIZE
        call	dword ptr [MmAllocateContiguousMemoryEx]
        mov	[bootloaderram], eax

    pop ecx
    pop ebx

    #ifdef DebugOut
    lea esi, text_bootloaderram
    call strhexout 
    #endif


// 5. use MmGetPhysicalAddress on that
    push ebx
    push ecx

        push	dword ptr [bootloaderram]
        call	dword ptr [MmGetPhysicalAddress]
        mov	[phys_bootloaderram],  eax

    pop ecx
    pop ebx

    #ifdef DebugOut
    lea esi, text_phys_bootloaderram
    call strhexout 
    #endif

// 6. use NtAllocateVirtualMemory to allocate virtual memory at that virtual address
//	NtAllocateVirtualMemory(
//		(PVOID*) &phys_bootloaderram,
//              (ULONG) 0,
//		(PULONG) PAGE_SIZE,    *****BUG***** POINTER!!
//		(ULONG) MEM_RESERVE | MEM_COMMIT,
//		(ULONG) PAGE_READWRITE);

    push ebx
    push ecx

        push	PAGE_READWRITE
        push	MEM_RESERVE + MEM_COMMIT        // sum = 3000
        push	offset ptr_PAGE_SIZE
        push	0 // 0 is used when the caller determines address
        push	offset phys_bootloaderram
        call	dword ptr [NtAllocateVirtualMemory]


    pop ecx
    pop ebx

    #ifdef DebugOut
    lea esi, text_allocvirt
    call strhexout 
    #endif


// 7. put a copy of the boot loader at both #4 and #6's memory

// Copy to #4
	cld
	mov	esi, offset blentry
	mov	edi, [bootloaderram]
	mov	ecx, PAGE_SIZE / 4
	rep 	movsb
	

// Copy to #6
// The phys_bootloaderram looks strange, but it's correct.  It was made into
// a virtual address by NtAllocateVirtualMemory.

	mov	esi, offset blentry
	mov	edi, [phys_bootloaderram]
	mov	ecx, PAGE_SIZE / 4
	rep 	movsb
	

//	mov	ebp, dword ptr [phys_pm_kernelram]	// kernel
        mov	esi, dword ptr [phys_kernelram]		// kernel information structure

	mov	edx, [phys_bootloaderram]; // we'll need it again later
	mov	ecx, edx;
	sub	ecx, offset blentry;

	// construct jmp to new CS
	mov	eax, ecx
	add	eax, offset newloc
	mov	ebx, ecx
	add	ebx, offset ptr_newloc
	mov	[ebx], eax
	add	ebx, 0x80000000 // change physical page, too
	mov	[ebx], eax

	jmp	edx;

// this is the bootloader code

// ecx = ("blentry" in real memory) - (blentry in original memory)
// ebp = phys_pm_kernelram
// esi = phys_kernelram
blentry:
	cli;
        // turn off paging
	mov	eax, cr0
	and	eax, 0xFFFFFFFF - CR0_ENABLE_PAGING
	mov	cr0, eax

	xor	eax, eax // flush the TLB
	mov	cr3, eax
// setup
	// put position if gdt into gdt_48_base
	mov	eax, ecx
	add	eax, offset bootloader_gdt
	mov	ebx, ecx
	add	ebx, offset gdt_48_base
	mov	[ebx], eax

// load new gdt/ldt/idt
	mov	eax, ecx
	add	eax, offset idt_48
	lidt	[eax]
	mov	eax, ecx
	add	eax, offset gdt_48
	lgdt	[eax]
	xor	eax, eax
	lldt	ax

// initialize segment registers
	mov	ax, 0x0010
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax
	mov	ss, ax


// jmp to new location
	.byte	0xea //  = ljmp
ptr_newloc:
	.long	0
	.word	0x0008

newloc:
 	// copy the Cromwell to it's new position
	mov	edi, KERNEL_RAM_ADDRESS 
	mov	ecx, KERNEL_MAX_SIZE 
	rep 	movsb

	
// Kill Interrupt Handler  
   
   	cli
 
	
// Interrupt Handler Now Dead      
        
	//jmp newflash
	// Jump into Cromwell & start Kernel
	.byte	0xea //  = ljmp


	.long	KERNEL_RAM_ADDRESS  // Basically, jump to the first Byte in the Cromwell image...
	.word	0x0008

newflash:
// flash the LEDs
// use this as debug code to check whether control reaches a certain point
#define LED_RED_C0	0x80
#define LED_RED_C1	0x40
#define LED_RED_C2	0x20
#define LED_RED_C3	0x10
#define LED_GREEN_C0	0x08
#define LED_GREEN_C1	0x04
#define LED_GREEN_C2	0x02
#define LED_GREEN_C3	0x01
	mov dx, 0xc004
	mov al, 0x20
	out dx, al
	mov dx, 0xc008
	mov al, 8
	out dx, al
	mov dx, 0xc006
	mov al, LED_RED_C0+LED_GREEN_C1+LED_RED_C2+LED_GREEN_C2
	out dx, al
	mov dx, 0xc000
	in ax, dx
	out dx, al
	mov dx, 0xc002
	mov al, 0x1a
	out dx, al

	mov eax, 1000000
l2:
	dec eax
	jne l2

	mov dx, 0xc004
	mov al, 0x20
	out dx, al
	mov dx, 0xc008
	mov al, 7
	out dx, al
	mov dx, 0xc006
	mov al, 1
	out dx, al
	mov dx, 0xc000
	in ax, dx
	out dx, al
	mov dx, 0xc002
	mov al, 0x1a
	out dx, al
lx:
	jmp lx

	
// taken from linux/arch/i386/boot/boot.S
// SS entry was added
.align 16
bootloader_gdt:
	// 0000 (dummy)
	.word	0, 0, 0, 0		// dummy

//	// 0008 (TSS?)
//	.word	0, 0, 0, 0		// unused

	// 0008 (code segment)
	.word	0xFFFF			// 4Gb - (0x100000*0x1000 = 4Gb)
	.word	0			// base address = 0
	.word	0x9A00			// code read/exec
	.word	0x00CF			// granularity = 4096, 386
					//  (+5th nibble of limit)

	// 0010 (data segment)
	.word	0xFFFF			// 4Gb - (0x100000*0x1000 = 4Gb)
	.word	0			// base address = 0
	.word	0x9200			// data read/write
	.word	0x00CF			// granularity = 4096, 386
					//  (+5th nibble of limit)

.align 8
idt_48:
	.word	0			// idt limit = 0
	.word	0, 0			// idt base = 0L
.align 8
gdt_48:
	.word	0x8000			// GDT limit=2048, 256 entries
gdt_48_base:
	.long	0			// GDT base



    
//    call flash
//infinite:
//    jmp infinite

   
strhexout:
    push eax
    call strout
    pop eax
    call hexout32
cr:
    mov al, 0xa
    jmp chrout
    
strout:
// esi: text
// ebx: x position (will be updated)
// ecx: y position (will be updated)
    mov eax, 0
    lodsb			// char
    test al,al
    je charend
    call chrout
    jmp strout
charend:
    ret
    
hexout32:
    push eax
    shr eax, 16
    call hexout16
    pop eax
    and eax, 0xFFFF
hexout16:
    push eax
    mov al, ah
    call hexout8
    pop eax
hexout8:
    push eax
    shr al, 4
    call hexout4
    pop eax
    and al, 0xF
hexout4:
    cmp al, 10
    jb hexoutd
    add al, 'A' - 10
    jmp chrout
hexoutd:
    add al, '0'
chrout:
// eax: character
// ebx: x position (will be updated)
// ecx: y position (will be updated)
    cmp al, 0x0a	// lf?
    jne nlf
    mov ebx, 0
    inc ecx
    ret
nlf:
    pusha
    and eax, 0xff	// avoid font overflow
    shl eax, 4		// index to character
    lea esi, font[eax]

    mov eax, ebx	// x position
    shl eax, 5		// index to screen line
    lea edi, TEXTBUFFER[eax]

    mov eax, ecx	// y position
    mov ebx, 0xA000
    mul ebx
    add edi, eax

    mov ecx, 16
fl1:
    lodsb
    push ecx
    mov ecx, 8
fl2:
    shl al, 1
    push eax
    mov eax, 0
    jnc f2
    dec eax
f2:
    stosd
    pop eax
    loop fl2
    add edi, 640*4 - 8*4
    pop ecx
    loop fl1
    popa
// update cursor position
    inc ebx
    cmp ebx, 80
    jb no1
    mov bx, 0
    inc cx
no1:
    ret

text1:
	.ascii "MmAllocateContiguousMemoryEx: 0x"
        .byte 0
text2:
	.ascii "MmGetPhysicalAddress: 0x"
        .byte 0
text3:
	.ascii "NtAllocateVirtualMemory: 0x"
        .byte 0
text_kernel:
	.ascii "kernel: 0x"
        .byte 0
text_kernelram:
	.ascii "kernelram: 0x"
        .byte 0
text_phys_kernelram:
	.ascii "phys_kernelram: 0x"
        .byte 0
text_bootloaderram:
	.ascii "bootloaderram: 0x"
        .byte 0
text_phys_bootloaderram:
	.ascii "phys_bootloaderram: 0x"
        .byte 0
text_allocvirt:
        .ascii "NtAllocateVirtualMemory(...) = 0x"
        .byte 0
text_setup_sects:
        .ascii "setup_sects: 0x"
        .byte 0

//text_phys_pm_kernelram:
    //.ascii "phys_pm_kernelram: 0x"
    //.byte 0
        
ptr_PAGE_SIZE:
    .long PAGE_SIZE

my_command_line:
    .ascii "devfs=mount"
    .byte 0
    
// Variables

.align 4

// Virtual address of the kernel's allocated physical memory
kernelram:
	.long	0
// Physical address of kernelram
phys_kernelram:
	.long	0
// Virtual address of the bootloaders's allocated physical memory
bootloaderram:
	.long	0
// Physical address of bootloader _and_ virtual address of a second copy
phys_bootloaderram:
	.long	0
//phys_pm_kernelram:
//	.long	0

#include "font.S"

//.align 4096.
.org 0x3000
kernel:
