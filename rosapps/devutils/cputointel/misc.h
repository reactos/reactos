
#define CPU_UNINT unsigned int
#define CPU_INT int

#define CPU_BYTE unsigned char

/* Prototypes for misc stuff */


/* Convert Bit index to int */
CPU_INT LoadPFileImage(char *infileName, char *outputfileName, CPU_UNINT BaseAddress, char *cpuid, CPU_UNINT type);
CPU_INT PEFileStart( CPU_BYTE *memory, CPU_UNINT pos, CPU_UNINT base,  CPU_UNINT size);

CPU_UNINT ConvertBitToByte(CPU_BYTE *bit);
CPU_UNINT GetMaskByte(CPU_BYTE *bit);

CPU_UNINT ConvertBitToByte32(CPU_BYTE *bit);
CPU_UNINT GetMaskByte32(CPU_BYTE *bit);

