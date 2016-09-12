

#define NUM_SAMPLES 4
#define MSR_RDTSC 0x10

#ifndef __ASM__

void __cdecl TscCalibrationISR(void);
extern LARGE_INTEGER HalpCpuClockFrequency;
VOID NTAPI HalpInitializeTsc(void);

#ifdef _M_AMD64
#define KiGetIdtEntry(Pcr, Vector) &((Pcr)->IdtBase[Vector])
#else
#define KiGetIdtEntry(Pcr, Vector) &((Pcr)->IDT[Vector])
#endif

#endif
