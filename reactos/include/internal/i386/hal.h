/*
 * 
 */

#ifndef __INTERNAL_HAL_HAL_H
#define __INTERNAL_HAL_HAL_H

#include <internal/service.h>
#include <internal/ntoskrnl.h>

typedef struct
{
   unsigned short previous_task;
   unsigned short reserved1;
   unsigned long esp0;
   unsigned short ss0;
   unsigned short reserved2;
   unsigned long esp1;
   unsigned short ss1;
   unsigned short reserved3;
   unsigned long esp2;
   unsigned short ss2;
   unsigned short reserved4;
   unsigned long cr3;
   unsigned long eip;
   unsigned long eflags;
   unsigned long eax;
   unsigned long ecx;
   unsigned long edx;
   unsigned long ebx;
   unsigned long esp;
   unsigned long ebp;
   unsigned long esi;
   unsigned long edi;
   unsigned short es;
   unsigned short reserved5;
   unsigned short cs;
   unsigned short reserved6;
   unsigned short ss;
   unsigned short reserved7;
   unsigned short ds;
   unsigned short reserved8;
   unsigned short fs;
   unsigned short reserved9;
   unsigned short gs;
   unsigned short reserved10;
   unsigned short ldt;
   unsigned short reserved11;
   unsigned short trap;
   unsigned short iomap_base;

   unsigned short nr;
   PVOID KernelStackBase;
   PVOID SavedKernelEsp;
   PVOID SavedKernelStackBase;
   
   unsigned char io_bitmap[1];
} hal_thread_state;


/*
 * FUNCTION: Probes for a PCI bus
 * RETURNS: True if found
 */
BOOL HalPciProbe(void);

/*
 * FUNCTION: Probes for a BIOS32 extension
 */
VOID Hal_bios32_probe(VOID);

/*
 * FUNCTION: Determines if a a bios32 service is present
 */
BOOLEAN Hal_bios32_is_service_present(ULONG service);

VOID HalInitializeDisplay (boot_param *bp);
VOID HalResetDisplay (VOID);

VOID
HalpInitBusHandlers (VOID);

ULONG
STDCALL
HalpGetSystemInterruptVector (
	PVOID	BusHandler,
	ULONG BusInterruptLevel,
	ULONG BusInterruptVector,
	PKIRQL Irql,
	PKAFFINITY Affinity
	);


#endif /* __INTERNAL_HAL_HAL_H */
