/*
 * 
 */

#ifndef __INTERNAL_HAL_HAL_H
#define __INTERNAL_HAL_HAL_H

#define NTOSAPI extern
#include <ntoskrnl.h>
#include <string.h>
#include <bus.h>
#include <mps.h>

/* We need to override these and use DECL_IMPORT to get the right _imp_Xxx
   symbol names */
extern DECL_IMPORT PHAL_DISPATCH_TABLE HalDispatchTable;
extern DECL_IMPORT PHAL_PRIVATE_DISPATCH_TABLE HalPrivateDispatchTable;


/*
 * FUNCTION: Probes for a BIOS32 extension
 */
VOID Hal_bios32_probe(VOID);

/*
 * FUNCTION: Determines if a a bios32 service is present
 */
BOOLEAN Hal_bios32_is_service_present(ULONG service);

VOID HalInitializeDisplay (PLOADER_PARAMETER_BLOCK LoaderBlock);
VOID HalResetDisplay (VOID);

VOID HalpInitBusHandlers (VOID);

/* irql.c */
VOID HalpInitPICs(VOID);

/* udelay.c */
VOID HalpCalibrateStallExecution(VOID);

/* pci.c */
VOID HalpInitPciBus (VOID);

/* enum.c */
VOID HalpStartEnumerator (VOID);

#endif /* __INTERNAL_HAL_HAL_H */
