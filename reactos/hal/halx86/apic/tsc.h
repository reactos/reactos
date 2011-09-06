

#define NUM_SAMPLES 4
#define MSR_RDTSC 0x10

#ifndef __ASM__

void TscCalibrationISR(void);
extern LARGE_INTEGER HalpCpuClockFrequency;
VOID NTAPI HalpInitializeTsc();


#define KiGetIdtEntry(Pcr, Vector) &((Pcr)->IDT[Vector])

#endif
