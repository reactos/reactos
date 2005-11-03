/*
 *  ReactOS kernel
 *  Copyright (C) 2003 Mike Nordell
 *  Based on multiboot.S (no copyright note present), but so heavily
 *  modified that it bears close to no resemblance to the original work.
 *
 *  MSVC compatible combination of plain C and inline assembler to:
 *  1 Relocated all the sections in the kernel - something I feel the
 *    bootloader should have done, but multiboot being just a "raw image"
 *    loader, it unfortunately had to be done here - in-place.
 *  2 Set up page directories and stuff.
 *  3 Load IDT, GDT and turn on paging, making us execute at the intended
 *    target address (as if the image was PE-loaded and parsed into that addr.)
 *  4 Call _main, and let the rest of the startup run...
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

//
// TODO: Fix the MP parts
//

/* INCLUDES ******************************************************************/

#pragma hdrstop

#include <ddk/ntddk.h>
#include <ddk/status.h>
#include <internal/i386/segment.h>
#include <internal/i386/fpu.h>
#include <internal/ps.h>
#include <ddk/defines.h>
#include <pe.h>

#include <roscfg.h>
#include <internal/ntoskrnl.h>
#include <internal/i386/segment.h>
#include <internal/ps.h>
#include <internal/ldr.h>


// some notes:
// The MSVC linker (by defult) emits no special .bss section, but uses the data
// section with a rawsize smaller than virtualsize. The "slack" is BSS.


//////////////////////////////////////////////////////////////////
// Some macros we need

// some stuff straight from freeloaders multiboot.h
#define MULTIBOOT_HEADER_MAGIC (0x1BADB002)
#define MULTIBOOT_HEADER_FLAGS (0x00010003)

#define TARGET_LOAD_ADDR  0x00200000
#define BASE_TO_PHYS_DIST (KERNEL_BASE - TARGET_LOAD_ADDR)

#define V2P(x)   (x - BASE_TO_PHYS_DIST)


#ifdef MP

#define AP_MAGIC (0x12481020)

#endif /* MP */



void  initialize_page_directory(void);

void* relocate_pointer_log_to_phys(const void* p)
{
	// DON'T CALL this function until relocation of .data and/or .rdata,
	// is completed - but still be sure that we have not yet enabled paging!
	return (void*)((DWORD)p - BASE_TO_PHYS_DIST);
}



#ifdef _DEBUG

// Macro to emit one character to Bochs debug-port (0x9e).
// We need to do it this way, since at this point of the startup, obviously
// we have neither HAL nor DbgPrint support.
#define BOCHS_OUT_CHAR(c1)      __asm push eax __asm push edx __asm mov dx, 0xe9 __asm mov al, c1 __asm out dx, al __asm pop edx __asm pop eax

void boch_out_hex_digit(unsigned char ch1)
{
	if (ch1 <= 9) { ch1 += '0'; } else { ch1 += 'a' - 10; }
	BOCHS_OUT_CHAR(ch1)
}

void bochs_dump_hex(DWORD p)
{
	unsigned char ch3 = (unsigned char)((p >> 28) & 0x0f);
	unsigned char cl3 = (unsigned char)((p >> 24) & 0x0f);
	unsigned char ch2 = (unsigned char)((p >> 20) & 0x0f);
	unsigned char cl2 = (unsigned char)((p >> 16) & 0x0f);
	unsigned char ch1 = (unsigned char)((p >> 12) & 0x0f);
	unsigned char cl1 = (unsigned char)((p >>  8) & 0x0f);
	unsigned char ch0 = (unsigned char)((p >>  4) & 0x0f);
	unsigned char cl0 = (unsigned char)((p >>  0) & 0x0f);
	BOCHS_OUT_CHAR('0') BOCHS_OUT_CHAR('x')
	boch_out_hex_digit(ch3);
	boch_out_hex_digit(cl3);
	boch_out_hex_digit(ch2);
	boch_out_hex_digit(cl2);
	boch_out_hex_digit(ch1);
	boch_out_hex_digit(cl1);
	boch_out_hex_digit(ch0);
	boch_out_hex_digit(cl0);
	BOCHS_OUT_CHAR('\n')
}

static void bochs_out_string(const char* s /* logical address! */)
{
	s = relocate_pointer_log_to_phys(s);
	__asm
	{
		pushad
		mov dx, 0xe9
		mov ebx, s
L1:
		cmp byte ptr[ebx], 0
		je end
		mov al, [ebx]
		out dx, al
		inc ebx
		jmp L1
end:
		popad
	}
}

#else

#define BOCHS_OUT_CHAR(c1)
#define bochs_dump_hex(VAL)
#define bochs_out_string(STR)

#endif	// _DEBUG


//////////////////////////////////////////////////////////////////

typedef char kernel_page_t[4096];

// Use 4096 (pagesize) more bytes that actually needed for each *_holder,
// to be able to make sure that the other stuff is page aligned.
// No other way to do this portably... :-(
//
// TODO: Consider allocating just one large block of BSS memory here, align
// just the first pointer, and then get the other ones just as offsets from
// this (now-aligned) pointer. That way we could get away with wasting just
// one page of memory, instead of 4 (like 16KB would matter... but still)
static kernel_page_t*  startup_pagedirectory_holder[1024 * 2];
static kernel_page_t*  lowmem_pagetable_holder[1024 * 2];
static kernel_page_t*  kernel_pagetable_holder[32*1024 + 1];
static __int32         kpcr_pagetable_holder[4096/4 * 2];

#ifdef MP
char apic_pagetable[4096];
#endif /* MP */

__int32 unmap_me[4096/4];
__int32 unmap_me2[4096/4];
__int32 unmap_me3[4096/4];

__int32 init_stack[3*4096/4];
int     init_stack_top;


__int32 trap_stack[3*4096/4];
int     trap_stack_top;



void _main();
// lie a bit about types - since C is basically typeless anyway, it
// doesn't really matter what type we say it is here...
extern int KiGdtDescriptor;
extern int KiIdtDescriptor;



	/*
	 * This is called by the realmode loader, with protected mode
	 * enabled, paging disabled and the segment registers pointing
	 * a 4Gb, 32-bit segment starting at zero.
	 *
	 *    EAX = Multiboot magic or application processor magic
	 *
	 *    EBX = Points to a structure in lowmem with data from the
	 *    loader
	 */
#pragma intrinsic(memset)


// We need to implement this ourself, to be able to get to it by short call's
void our_memmove(void* pDest, const void* pSrc, DWORD size)
{
	      char* pD = (char*)pDest;
	const char* pS = (char*)pSrc;
	if (pDest < pSrc)
	{
		while (size--)
		{
			*pD++ = *pS++;
		}
	}
	else if (pSrc < pDest)
	{
		while (size--)
		{
			pD[size] = pS[size];
		}
	}
}
void dummy_placeholder(void)
{
	// NOTE: This function MUST be placed JUST AFTER MultibootStub in memory.
	// Yes, it's BEFORE it in this file, but linkorder.txt fixes this for us.
}


// This one is needed, since the boot loader hasn't relocated us
__declspec(naked)
void MultibootStub()
{
	__asm
	{
		jmp		_multiboot_entry
		// This sucks, I know...
#define EMIT_DWORD(x) __asm __emit ((x) >> 0) & 0xff __asm _emit ((x) >> 8) & 0xff __asm _emit ((x) >> 16) & 0xff __asm _emit ((x) >> 24) & 0xff
		ALIGN 4
		EMIT_DWORD(MULTIBOOT_HEADER_MAGIC)
		EMIT_DWORD(MULTIBOOT_HEADER_FLAGS)
		EMIT_DWORD(-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS))
		EMIT_DWORD(TARGET_LOAD_ADDR + 0x0400 + 0x04)
		// Now just make something up, since there is no way we can know, beforehand,
		// where any BSS data is...
		EMIT_DWORD((TARGET_LOAD_ADDR))
		EMIT_DWORD((TARGET_LOAD_ADDR + 1*1024*1024)) /* assume ntoskrnel.exe is < 1MB! */
		EMIT_DWORD((TARGET_LOAD_ADDR + 2*1024*1024)) /* just to have something, let's say BSS is 1MB too */
		/* This is *REALLY* ugly! If MultibootStub is *EVER* at   */
		/* any other offset, this will crash like crazy! */
		/* 0x0400 is the file alignment of the binary (ntoskrnl.exe) */
		EMIT_DWORD((TARGET_LOAD_ADDR + 0x0400))	// entry_addr

_multiboot_entry:

		cld		// just for good measure
	}

	{
		/* Save the multiboot or application processor magic */
		DWORD saved_eax;
		DWORD saved_ebx;
		__asm	mov	saved_eax, eax
		__asm	mov	saved_ebx, ebx

//		bochs_out_string("MultibootStub()\n");

		// OK, time to relocate the brute-loaded image in-place...
		// If we don't watch it, we will overwrite ourselves here - imagine
		// the fireworks! :-) That's why the function dummy_placeholder()
		// MUST be placed JUST JUST AFTER this function.

		{
		PIMAGE_NT_HEADERS     NtHeader = RtlImageNtHeader((PVOID)TARGET_LOAD_ADDR);
		PIMAGE_SECTION_HEADER Section  = IMAGE_FIRST_SECTION(NtHeader);
		const int count = NtHeader->FileHeader.NumberOfSections;
		int i;
		Section += count - 1;	// make it point to the last section
		// NOTE: We MUST walk the sections "backwards".
		for (i = count-1; i >= 0; --i, --Section)
		{
			DWORD dwSrc = TARGET_LOAD_ADDR + Section->PointerToRawData;
			DWORD dwDst = TARGET_LOAD_ADDR + Section->VirtualAddress;
			DWORD dwSiz = Section->SizeOfRawData;
			const char* pEndThisFunc;

			if (dwSrc == dwDst)
			{
				continue;
			}

//bochs_out_string("MultibootStub: relocating section\n");

			if (Section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
			{
				// can't get a pointer to a label from plain C :-(
				__asm mov	pEndThisFunc, offset dummy_placeholder
				pEndThisFunc -= BASE_TO_PHYS_DIST;
				if (dwDst < (DWORD)pEndThisFunc)
				{
					// We must not move the code from under our feet!
					// This can only happen in the code segment - the first segment
					DWORD diff = (DWORD)pEndThisFunc - dwDst;
					dwDst += diff;
					dwSrc += diff;
					dwSiz -= diff;
				}
			}

			// obviously we must use memmove, since memory can overlap
			our_memmove((void*)dwDst, (void*)dwSrc, dwSiz);

			// While at it, we might as well zero any uninitialized data in the section...
			if (Section->SizeOfRawData < Section->Misc.VirtualSize)
			{
				memset((char*)(Section->VirtualAddress + Section->SizeOfRawData + TARGET_LOAD_ADDR),
					   0,
					   Section->Misc.VirtualSize - Section->SizeOfRawData);
			}
		}

		// Now all sections are relocated to their intended in-memory layout,
		// but we are still running int the low TARGET_LOAD_ADDR memory.

		{
			// Time to jump to the real startup, the entry-point function.
			// We must do this using assembler, since both eax and ebx are assumed
			// to hold some magic values.
			typedef VOID (STDCALL* pfn_t)(PPEB);
			pfn_t pfn = (pfn_t)(NtHeader->OptionalHeader.AddressOfEntryPoint + TARGET_LOAD_ADDR);
#if 1
			__asm	mov eax, saved_eax
			__asm	mov ebx, saved_ebx
			__asm	mov ecx, pfn
			__asm	jmp ecx
#else
			__asm	mov ebx, saved_ebx
			(*pfn)((PPEB)saved_eax);
#endif
		}
		}
	}
}


// TMN: TODO: Convert this to the extent possible to plain C code
// Due to the "magic" above, we enter this function with all kernel sections
// properly relocated wrt. offsets from start-of-mem. But, we are still running
// without paging, meaning that the address that is to be KERNEL_BASE+xyz is
// currently still TARGET_LOAD_ADDR+xyz.
// We get aways with a few of the functions call here since they are near calls
// (PC-relative), but don't even _think_ about calling any other functions
// until we have turned on paging!
VOID STDCALL
NtProcessStartup(
	IN	PPEB	Peb
	)
{
	DWORD saved_ebx;
	DWORD saved_eax;
	__asm mov saved_ebx, ebx
	__asm mov saved_eax, eax

	bochs_out_string("NtProcessStartup: Just entered\n");

#ifdef MP
	if (saved_eax != AP_MAGIC)
	{
#endif /* MP */

		bochs_out_string("NtProcessStartup: Calling initialize_page_directory()\n");

		initialize_page_directory();	// Initialize the page directory

		bochs_out_string("NtProcessStartup: Page directory initialized\n");

#ifdef  MP

		__asm
		{
			/*
			 * Initialize the page table that maps the APIC register address space
			 */
			 
			/* 
			 * FIXME: APIC register address space can be non-standard so do the 
			 * mapping later 
			 */
			mov		esi, V2P(apic_pagetable)
			mov		edi, 0
			mov		eax, 0xFEC0001B
			mov		[esi+edi], eax
			mov		edi, 0x800
			mov		eax, 0xFEE0001B
			mov		[esi+edi], eax
		}
	}

#endif /* MP */

	{
		bochs_out_string("NtProcessStartup: Enabling paging...\n");

		/*
		 * Enable paging and set write protect
		 * bit 31: PG, bit 16: WP
		 */
		__asm	mov		eax, cr0
		__asm	or		eax, 0x80010000
		__asm	mov		cr0, eax

		bochs_out_string("NtProcessStartup: Paging enabled!\n");
		bochs_out_string("NtProcessStartup: But we're still at the \"low\" address\n");

		/*
		 * Do an absolute jump because we now want to execute above KERNEL_BASE
		 */
		__asm	mov		eax, offset l2_
		__asm	jmp		eax
	}

l2_:

bochs_out_string("We have now left \"low\" memory, and is flying at an altitude of...\n");
bochs_out_string("OK, we're not flying, we're just executing above KERNEL_BASE\n");

	/*
	 * Load the GDTR and IDTR with new tables located above
	 * KERNEL_BASE
	 */

#ifdef _DEBUG
	{
		DWORD val = (DWORD)&KiGdtDescriptor;
		bochs_out_string("&KiGdtDescriptor: ");
		bochs_dump_hex(val);

		val = (DWORD)&KiIdtDescriptor;
		bochs_out_string("&KiIdtDescriptor: ");
		bochs_dump_hex(val);
	}
#endif

bochs_out_string("Loading GDT and IDT...\n");

	/* FIXME: Application processors should have their own GDT/IDT */
	__asm	lgdt	KiGdtDescriptor
	__asm	lidt	KiIdtDescriptor

bochs_out_string("GDT and IDT loaded\n");

	__asm
	{
		/*
		 * Reload the data segment registers 
		 */
		mov		eax, KERNEL_DS
		mov		ds, ax
		mov		es, ax
		mov		gs, ax
		mov		ss, ax
		mov		eax, 0
		mov		fs, ax
	}

bochs_out_string("NtProcessStartup: segment registers loaded\n");

#ifdef MP

	if (saved_eax == AP_MAGIC)
	{
		__asm
		{
			/*
			 * This is an application processor executing
			 */

			/*
			 * Initialize EFLAGS
			 */
			push	0
			popfd

			/*
			 * Call the application processor initialization code
			 */
			push	0
			push	offset l7_
			push	KERNEL_CS
			push	KiSystemStartup
			retf

			/*
			 * Catch illegal returns from KiSystemStartup
			 */
l7_:
			pop		eax
		}

		KeBugCheck(0);

		for (;;)
			;	/*forever */
	}

#endif /* MP */

bochs_out_string("Loading fs with PCR_SELECTOR\n");

	/* Load the PCR selector */
	__asm	mov		eax, PCR_SELECTOR
	__asm	mov		fs, ax

bochs_out_string("Loading esp with init_stack_top    : "); bochs_dump_hex((DWORD)&init_stack_top);
bochs_out_string("Just for interest, init_stack is at: "); bochs_dump_hex((DWORD)init_stack);
bochs_out_string("Meaing the init_stack in bytes is  : "); bochs_dump_hex((DWORD)&init_stack_top - (DWORD)init_stack);


	/* Load the initial kernel stack */
	__asm	mov		esp, offset init_stack_top

bochs_out_string("Loaded esp with init_stack_top\n");

	/*
	 * Initialize EFLAGS
	 */
	__asm	push	0
	__asm	popfd

bochs_out_string("Loaded eflags\n");

		/*
		 * Call the main kernel initialization
		 */
bochs_out_string("TMN: Calling _main...\n");

	__asm
	{
		xor		ebp,ebp
		push	ebx
		push	edx
		push	offset l5_
		push	KERNEL_CS
		push	offset _main
		retf
	
	/*
	 * Catch illegal returns from main, try bug checking the system,
	 * if that fails then loop forever.
	 */
l5_:
		pop		eax
		pop		eax

	} // end of __asm block

bochs_out_string("TMN: Back from _main ?! Let's crash!\n");

	KeBugCheck(0);

	for (;;)
		;	/*forever */
}


void initialize_page_directory(void)
{
	/*
	 * Initialize the page directory
	 */

	// First convert the pointers from the virtual address the compiler generated
	// code thinks we are at, to the currently active physical address we actually
	// got loaded into by the loader. At this point we have been relocated, so
	// that there is a 1:1 mapping between KERNEL_BASE+n and TARGET_LOAD_ADDR+n.

	kernel_page_t** startup_pagedirectory = startup_pagedirectory_holder;
	kernel_page_t** lowmem_pagetable      = lowmem_pagetable_holder;
	kernel_page_t** kernel_pagetable      = kernel_pagetable_holder;
	__int32*        kpcr_pagetable        = kpcr_pagetable_holder;

	bochs_out_string("startup_pagedirectory before reloc: ");
	bochs_dump_hex((DWORD)startup_pagedirectory);

	startup_pagedirectory = (kernel_page_t**)relocate_pointer_log_to_phys(startup_pagedirectory);
	lowmem_pagetable      = (kernel_page_t**)relocate_pointer_log_to_phys(lowmem_pagetable);
	kernel_pagetable      = (kernel_page_t**)relocate_pointer_log_to_phys(kernel_pagetable);
	kpcr_pagetable        = (__int32*)       relocate_pointer_log_to_phys(kpcr_pagetable);

	bochs_out_string("startup_pagedirectory after reloc : ");
	bochs_dump_hex((DWORD)startup_pagedirectory);

	// Now align the pointers to PAGE_SIZE...
	startup_pagedirectory = (kernel_page_t**)(((ULONG_PTR)startup_pagedirectory + 4095) & ~4095);
	lowmem_pagetable      = (kernel_page_t**)(((ULONG_PTR)lowmem_pagetable      + 4095) & ~4095);
	kernel_pagetable      = (kernel_page_t**)(((ULONG_PTR)kernel_pagetable      + 4095) & ~4095);
	kpcr_pagetable        = (__int32* )      (((ULONG_PTR)kpcr_pagetable        + 4095) & ~4095);

#ifdef _DEBUG
	bochs_out_string("startup_pagedirectory aligned     : ");
	bochs_dump_hex((DWORD)startup_pagedirectory);
#endif

	// Ugly macros, I know...
#define DEST(PAGE)   startup_pagedirectory[(PAGE) + 0xc00 / 4]
#define SRC(PAGE)    (kernel_page_t*)((char*)kernel_pagetable + (PAGE)*4096 + 0x7)

	startup_pagedirectory[0] = (kernel_page_t*)((char*)lowmem_pagetable + 0x7);

	{
		unsigned int i;
		for (i=0; i<32; ++i)
		{
			DEST(i) = SRC(i);
		}
	}

	DEST( 64) = (kernel_page_t*)((char*)lowmem_pagetable      + 0x7);
	DEST(192) = (kernel_page_t*)((char*)startup_pagedirectory + 0x7);
#ifdef MP
	DEST(251) = (kernel_page_t*)((char*)apic_pagetable        + 0x7);
#endif /* MP */
	DEST(252) = (kernel_page_t*)((char*)kpcr_pagetable        + 0x7);


	{
		unsigned int i;
		/* Initialize the page table that maps low memory */
		for (i=0; i<1024; ++i) {
			lowmem_pagetable[i] = (kernel_page_t*)(i*4096 + 7);
		}

		/* Initialize the page table that maps kernel memory */
		for (i=0; i<6144/4 /* 1536 pages = 6MB */; ++i) {
			kernel_pagetable[i] = (kernel_page_t*)(i*4096 + TARGET_LOAD_ADDR + 0x7);
		}

		/* Initialize the page table that maps the initial KPCR (at FF000000) */
		kpcr_pagetable[0] = 0x1007;
	}

	/*
	 * Set up the PDBR
	 */	
	__asm	mov		eax, startup_pagedirectory
	__asm	mov		cr3, eax
}

