/*
 * 
 */

#ifndef __INTERNAL_HAL_HAL_H
#define __INTERNAL_HAL_HAL_H

#include <ddk/service.h>
#include <internal/ntoskrnl.h>

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

VOID HalInitializeDisplay (PLOADER_PARAMETER_BLOCK LoaderBlock);
VOID HalResetDisplay (VOID);

VOID HalpInitBusHandlers (VOID);

/* udelay.c */
VOID HalpCalibrateStallExecution(VOID);

#endif /* __INTERNAL_HAL_HAL_H */
