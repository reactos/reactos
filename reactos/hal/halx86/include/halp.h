/*
 * 
 */

#ifndef __INTERNAL_HAL_HAL_H
#define __INTERNAL_HAL_HAL_H

#define HAL_APC_REQUEST	    0
#define HAL_DPC_REQUEST	    1

/* display.c */
struct _LOADER_PARAMETER_BLOCK;
VOID FASTCALL HalInitializeDisplay (struct _LOADER_PARAMETER_BLOCK *LoaderBlock);
VOID FASTCALL HalClearDisplay (UCHAR CharAttribute);

/* adapter.c */
PADAPTER_OBJECT STDCALL HalpAllocateAdapterEx(ULONG NumberOfMapRegisters,BOOLEAN IsMaster, BOOLEAN Dma32BitAddresses);
  
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

/* mem.c */
PVOID HalpMapPhysMemory(ULONG PhysAddr, ULONG Size);

/* Non-generic initialization */
VOID HalpInitPhase0 (PLOADER_PARAMETER_BLOCK LoaderBlock);

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
#define Ki386RdTSC(x)		    __asm__ __volatile__("rdtsc\n\t" : "=A" (x.u.LowPart), "=d" (x.u.HighPart));
#define Ki386Rdmsr(msr,val1,val2)   __asm__ __volatile__("rdmsr" : "=a" (val1), "=d" (val2) : "c" (msr))
#define Ki386Wrmsr(msr,val1,val2)   __asm__ __volatile__("wrmsr" : /* no outputs */ : "c" (msr), "a" (val1), "d" (val2))

static inline BYTE Ki386ReadFsByte(ULONG offset)
{
   BYTE b;
   __asm__ __volatile__("movb %%fs:(%1),%0":"=q" (b):"r" (offset));
   return b;
}

static inline VOID Ki386WriteFsByte(ULONG offset, BYTE value)
{
   __asm__ __volatile__("movb %0,%%fs:(%1)"::"r" (value), "r" (offset));
}

#elif defined(_MSC_VER)
#define Ki386SaveFlags(x)	    __asm pushfd  __asm pop x;
#define Ki386RestoreFlags(x)	    __asm push x  __asm popfd;
#define Ki386DisableInterrupts()    __asm cli
#define Ki386EnableInterrupts()	    __asm sti
#define Ki386HaltProcessor()	    __asm hlt
#else
#error Unknown compiler for inline assembler
#endif

typedef struct tagHALP_HOOKS
{
  void (*InitPciBus)(ULONG BusNumber, PBUS_HANDLER BusHandler);
} HALP_HOOKS, *PHALP_HOOKS;

extern HALP_HOOKS HalpHooks;

#endif /* __INTERNAL_HAL_HAL_H */
