/*
 * Various useful prototypes
 */

#ifndef __INCLUDE_INTERNAL_NTOSKRNL_H
#define __INCLUDE_INTERNAL_NTOSKRNL_H

#ifndef __ASM__

#include <stdarg.h>
#define NTOS_MODE_KERNEL
#include <ntos.h>

#include "internal/ke.h"

/*
 * Use these to place a function in a specific section of the executable
 */
#define PLACE_IN_SECTION(s)	__attribute__((section (s)))
#define INIT_FUNCTION		PLACE_IN_SECTION("init")
#define PAGE_LOCKED_FUNCTION	PLACE_IN_SECTION("pagelk")
#define PAGE_UNLOCKED_FUNCTION	PLACE_IN_SECTION("pagepo")

#include <pshpack1.h>
/*
 * Defines a descriptor as it appears in the processor tables
 */
typedef struct _DESCRIPTOR
{
  ULONG a;
  ULONG b;
} IDT_DESCRIPTOR, GDT_DESCRIPTOR;

#include <poppack.h>

extern IDT_DESCRIPTOR KiIdt[256];
//extern GDT_DESCRIPTOR KiGdt[256];


VOID ExpInitializeEventImplementation(VOID);
VOID ExpInitializeEventImplementation(VOID);
VOID ExpInitializeEventPairImplementation(VOID);
VOID ExpInitializeSemaphoreImplementation(VOID);
VOID ExpInitializeMutantImplementation(VOID);
VOID ExpInitializeTimerImplementation(VOID);
VOID ExpInitializeProfileImplementation(VOID);
/*
 * Initalization functions (called once by main())
 */
VOID MmInitSystem(ULONG Phase, PLOADER_PARAMETER_BLOCK LoaderBlock, ULONG LastKernelAddress);
VOID IoInit(VOID);
VOID IoInit2(BOOLEAN BootLog);
VOID STDCALL IoInit3(VOID);
VOID ObInit(VOID);
VOID PsInit(VOID);
VOID CmInitializeRegistry(VOID);
VOID STDCALL CmInitHives(BOOLEAN SetupBoot);
VOID CmInit2(PCHAR CommandLine);
VOID CmShutdownRegistry(VOID);
BOOLEAN CmImportSystemHive(PCHAR ChunkBase, ULONG ChunkSize);
BOOLEAN CmImportHardwareHive(PCHAR ChunkBase, ULONG ChunkSize);
VOID KdInitSystem(ULONG Reserved, PLOADER_PARAMETER_BLOCK LoaderBlock);

#endif /* __ASM__ */

/*
 * 
 */
#define MM_STACK_SIZE             (12*4096)

#endif /* INCLUDE_INTERNAL_NTOSKRNL_H */
