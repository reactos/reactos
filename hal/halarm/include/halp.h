/*
 *
 */

#ifndef __INTERNAL_HAL_HAL_H
#define __INTERNAL_HAL_HAL_H

/* WDK Hack */
#define KdComPortInUse          _KdComPortInUse

#define HAL_APC_REQUEST         0
#define HAL_DPC_REQUEST         1

/* Conversion functions */
#define BCD_INT(bcd)            \
    (((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F))
#define INT_BCD(int)            \
    (UCHAR)(((int / 10) << 4) + (int % 10))

/* adapter.c */
PADAPTER_OBJECT STDCALL HalpAllocateAdapterEx(ULONG NumberOfMapRegisters,BOOLEAN IsMaster, BOOLEAN Dma32BitAddresses);

/* bus.c */
VOID NTAPI HalpInitNonBusHandler (VOID);

/* irql.c */
VOID NTAPI HalpInitPICs(VOID);

/* udelay.c */
VOID NTAPI HalpInitializeClock(VOID);

/* pci.c */
VOID HalpInitPciBus (VOID);

/* dma.c */
VOID HalpInitDma (VOID);

/* Non-generic initialization */
VOID HalpInitPhase0 (PLOADER_PARAMETER_BLOCK LoaderBlock);
VOID HalpInitPhase1(VOID);
VOID NTAPI HalpClockInterrupt(VOID);

//
// KD Support
//
VOID
NTAPI
HalpCheckPowerButton(
    VOID
);

VOID
NTAPI
HalpRegisterKdSupportFunctions(
    VOID
);

NTSTATUS
NTAPI
HalpSetupPciDeviceForDebugging(
    IN PVOID LoaderBlock,
    IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice
);

NTSTATUS
NTAPI
HalpReleasePciDeviceForDebugging(
    IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice
);

//
// Memory routines
//
PVOID
NTAPI
HalpMapPhysicalMemory64(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPage
);

VOID
NTAPI
HalpUnmapVirtualAddress(
    IN PVOID VirtualAddress,
    IN ULONG NumberPages
);

/* sysinfo.c */
NTSTATUS
NTAPI
HaliQuerySystemInformation(
    IN HAL_QUERY_INFORMATION_CLASS InformationClass,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer,
    OUT PULONG ReturnedLength
);

NTSTATUS
NTAPI
HaliSetSystemInformation(
    IN HAL_SET_INFORMATION_CLASS InformationClass,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer
);

typedef struct tagHALP_HOOKS
{
  void (*InitPciBus)(ULONG BusNumber, PBUS_HANDLER BusHandler);
} HALP_HOOKS, *PHALP_HOOKS;

extern HALP_HOOKS HalpHooks;
extern KSPIN_LOCK HalpSystemHardwareLock;

#endif /* __INTERNAL_HAL_HAL_H */
