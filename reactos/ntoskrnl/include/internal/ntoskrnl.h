/*
 * Various useful prototypes
 */

#ifndef __INCLUDE_INTERNAL_NTOSKRNL_H
#define __INCLUDE_INTERNAL_NTOSKRNL_H

#ifndef __ASM__

#include <ddk/ntddk.h>

#include <stdarg.h>

/*
 * Use these to place a function in a specific section of the executable
 */
#define PLACE_IN_SECTION(s) __attribute__((section (s)))
#define INIT_FUNCTION (PLACE_IN_SECTION("init"))
#define PAGE_LOCKED_FUNCTION (PLACE_IN_SECTION("pagelk"))
#define PAGE_UNLOCKED_FUNCTION (PLACE_IN_SECTION("pagepo"))

/*
 * Maximum size of the kmalloc area (this is totally arbitary)
 */
#define NONPAGED_POOL_SIZE   (4*1024*1024)

/*
 * Defines a descriptor as it appears in the processor tables
 */
typedef struct _DESCRIPTOR
{
  ULONG a;
  ULONG b;
} __attribute__ ((packed)) IDT_DESCRIPTOR, GDT_DESCRIPTOR;

extern IDT_DESCRIPTOR KiIdt[256];
//extern GDT_DESCRIPTOR KiGdt[256];


VOID NtInitializeEventImplementation(VOID);
VOID NtInit(VOID);

/*
 * Initalization functions (called once by main())
 */
VOID MmInitSystem(ULONG Phase, PLOADER_PARAMETER_BLOCK LoaderBlock, ULONG LastKernelAddress);
VOID IoInit(VOID);
VOID ObInit(VOID);
VOID PsInit(VOID);
VOID CmInitializeRegistry(VOID);
VOID CmInitializeRegistry2(VOID);
VOID CmShutdownRegistry(VOID);
VOID CmImportHive(PCHAR);
VOID KdInitSystem(ULONG Reserved, PLOADER_PARAMETER_BLOCK LoaderBlock);

NTSTATUS RtlpInitNlsSections(ULONG Mod1Start,
			     ULONG Mod1End,
			     ULONG Mod2Start,
			     ULONG Mod2End,
			     ULONG Mod3Start,
			     ULONG Mod3End);

#endif /* __ASM__ */

/*
 * 
 */
#define MM_STACK_SIZE             (3*4096)

#endif /* INCLUDE_INTERNAL_NTOSKRNL_H */
