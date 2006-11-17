/*
 * 
 */

#ifndef __INTERNAL_HAL_HAL_H
#define __INTERNAL_HAL_HAL_H

/* Temporary hack */
#define KPCR_BASE   0xFF000000

/* WDK Hack */
#define KdComPortInUse _KdComPortInUse

#define HAL_APC_REQUEST	    0
#define HAL_DPC_REQUEST	    1

//
// Kernel Debugger Port Definition
//
typedef struct _KD_PORT_INFORMATION
{
    ULONG ComPort;
    ULONG BaudRate;
    ULONG BaseAddress;
} KD_PORT_INFORMATION, *PKD_PORT_INFORMATION;
/* adapter.c */
PADAPTER_OBJECT STDCALL HalpAllocateAdapterEx(ULONG NumberOfMapRegisters,BOOLEAN IsMaster, BOOLEAN Dma32BitAddresses);
  
/* bus.c */
VOID HalpInitBusHandlers (VOID);

/* irql.c */
VOID NTAPI HalpInitPICs(VOID);

/* udelay.c */
VOID HalpCalibrateStallExecution(VOID);

/* pci.c */
VOID HalpInitPciBus (VOID);

/* enum.c */
VOID HalpStartEnumerator (VOID);

/* dma.c */
VOID HalpInitDma (VOID);

/* mem.c */
PVOID HalpMapPhysMemory(ULONG PhysAddr, ULONG Size);

/* Non-generic initialization */
VOID HalpInitPhase0 (PLOADER_PARAMETER_BLOCK LoaderBlock);
VOID HalpInitPhase1(VOID);
VOID NTAPI HalpClockInterrupt(VOID);

/* sysinfo.c */
NTSTATUS STDCALL
HalpQuerySystemInformation(IN HAL_QUERY_INFORMATION_CLASS InformationClass,
			   IN ULONG BufferSize,
			   IN OUT PVOID Buffer,
			   OUT PULONG ReturnedLength);

typedef struct tagHALP_HOOKS
{
  void (*InitPciBus)(ULONG BusNumber, PBUS_HANDLER BusHandler);
} HALP_HOOKS, *PHALP_HOOKS;

extern HALP_HOOKS HalpHooks;

#endif /* __INTERNAL_HAL_HAL_H */
