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

/* CMOS Registers and Ports */
#define CMOS_CONTROL_PORT       (PUCHAR)0x70
#define CMOS_DATA_PORT          (PUCHAR)0x71
#define RTC_REGISTER_A          0x0A
#define RTC_REGISTER_B          0x0B
#define RTC_REG_A_UIP           0x80
#define RTC_REGISTER_CENTURY    0x32

/* Conversion functions */
#define BCD_INT(bcd)            \
    (((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F))
#define INT_BCD(int)            \
    (UCHAR)(((int / 10) << 4) + (int % 10))

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
VOID NTAPI HalpInitNonBusHandler (VOID);

/* irql.c */
VOID NTAPI HalpInitPICs(VOID);

/* udelay.c */
VOID HalpCalibrateStallExecution(VOID);

VOID NTAPI HalpInitializeClock(VOID);

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
