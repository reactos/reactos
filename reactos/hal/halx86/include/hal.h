/*
 * 
 */

#ifndef __INTERNAL_HAL_HAL_H
#define __INTERNAL_HAL_HAL_H

/*
 * FUNCTION: Probes for a BIOS32 extension
 */
VOID Hal_bios32_probe(VOID);

/*
 * FUNCTION: Determines if a a bios32 service is present
 */
BOOLEAN Hal_bios32_is_service_present(ULONG service);

/* display.c */
VOID FASTCALL HalInitializeDisplay (PLOADER_PARAMETER_BLOCK LoaderBlock);
VOID FASTCALL HalClearDisplay (UCHAR CharAttribute);

VOID HalpInitBusHandlers (VOID);

/* irql.c */
VOID HalpInitPICs(VOID);

/* udelay.c */
VOID HalpCalibrateStallExecution(VOID);

/* pci.c */
VOID HalpInitPciBus (VOID);

/* enum.c */
VOID HalpStartEnumerator (VOID);

/*
 * ADAPTER_OBJECT - Track a busmaster DMA adapter and its associated resources
 *
 * NOTES:
 *     - I have not found any documentation on this; if you have any, please 
 *       fix this struct definition
 *     - Some of this is right and some of this is wrong; many of these fields
 *       are unused at this point because X86 doesn't have map registers and
 *       currently that's all ROS supports
 */
struct _ADAPTER_OBJECT {
  INTERFACE_TYPE InterfaceType;
  BOOLEAN Master;
  int Channel;
  PVOID PagePort;
  PVOID CountPort;
  PVOID OffsetPort;
  KSPIN_LOCK SpinLock;
  PVOID Buffer;
  BOOLEAN Inuse;
  ULONG AvailableMapRegisters;
  PVOID MapRegisterBase;
  ULONG AllocatedMapRegisters;
  PWAIT_CONTEXT_BLOCK WaitContextBlock;
  PKDEVICE_QUEUE DeviceQueue;
  BOOLEAN UsesPhysicalMapRegisters;
};

/* sysinfo.c */
NTSTATUS STDCALL
HalpQuerySystemInformation(IN HAL_QUERY_INFORMATION_CLASS InformationClass,
			   IN ULONG BufferSize,
			   IN OUT PVOID Buffer,
			   OUT PULONG ReturnedLength);


/* Non-standard functions */
VOID STDCALL
HalReleaseDisplayOwnership();

BOOLEAN STDCALL
HalQueryDisplayOwnership();


#endif /* __INTERNAL_HAL_HAL_H */
