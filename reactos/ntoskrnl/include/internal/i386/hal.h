/*
 * 
 */

#ifndef __INTERNAL_HAL_HAL_H
#define __INTERNAL_HAL_HAL_H

#include <ddk/service.h>
#include <internal/ntoskrnl.h>

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

struct _ADAPTER_OBJECT {
  int Channel;
  PVOID PagePort;
  PVOID CountPort;
  PVOID OffsetPort;
  KSPIN_LOCK SpinLock;
  PVOID Buffer;
  BOOLEAN Inuse;
};

#endif /* __INTERNAL_HAL_HAL_H */
