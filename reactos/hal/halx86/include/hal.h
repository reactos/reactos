/*
 * 
 */

#ifndef __INTERNAL_HAL_HAL_H
#define __INTERNAL_HAL_HAL_H


/* display.c */
VOID FASTCALL HalInitializeDisplay (PLOADER_PARAMETER_BLOCK LoaderBlock);
VOID FASTCALL HalClearDisplay (UCHAR CharAttribute);

/* bus.c */
VOID HalpInitBusHandlers (VOID);

/* irql.c */
VOID HalpInitPICs(VOID);

/* udelay.c */
VOID HalpCalibrateStallExecution(VOID);

/* pci.c */
VOID HalpInitPciBus (VOID);

/* enum.c */
VOID HalpStartEnumerator (VOID);

/* dma.c */
VOID HalpInitDma (VOID);

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
  KDEVICE_QUEUE DeviceQueue;
  BOOLEAN ScatterGather;

  /*
   * 18/07/04: Added these members. It's propably not the exact place where
   * this should be stored, but I can't find better one. I haven't checked
   * how Windows handles this.
   * -- Filip Navara
   */
  BOOLEAN DemandMode;
  BOOLEAN AutoInitialize;
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

#if defined(__GNUC__)
#define Ki386SaveFlags(x)	    __asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */)
#define Ki386RestoreFlags(x)	    __asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory")
#define Ki386DisableInterrupts()    __asm__ __volatile__("cli\n\t")
#define Ki386EnableInterrupts()	    __asm__ __volatile__("sti\n\t")
#define Ki386HaltProcessor()	    __asm__ __volatile__("hlt\n\t")
#elif defined(_MSC_VER)
#define Ki386SaveFlags(x)	    __asm pushfd  __asm pop x;
#define Ki386RestoreFlags(x)	    __asm push x  __asm popfd;
#define Ki386DisableInterrupts()    __asm cli
#define Ki386EnableInterrupts()	    __asm sti
#define Ki386HaltProcessor()	    __asm hlt
#else
#error Unknown compiler for inline assembler
#endif





#endif /* __INTERNAL_HAL_HAL_H */
