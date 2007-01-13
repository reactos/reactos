

/* 64bits unsigned */
#define CPU_UNINT64 unsigned long long

/* 32bits */
#define CPU_UNINT   unsigned int
#define CPU_INT     int

/* 8bits unsigned */
#define CPU_BYTE    unsigned char

/* Prototypes for misc stuff */
CPU_INT LoadPFileImage(char *infileName, char *outputfileName, CPU_UNINT BaseAddress, char *cpuid, CPU_UNINT type, CPU_INT mode);
CPU_INT PEFileStart( CPU_BYTE *memory, CPU_UNINT pos, CPU_UNINT base, CPU_UNINT size, FILE *outfp, CPU_INT mode);

CPU_UNINT ConvertBitToByte(CPU_BYTE *bit);
CPU_UNINT GetMaskByte(CPU_BYTE *bit);

CPU_UNINT ConvertBitToByte32(CPU_BYTE *bit);
CPU_UNINT GetMaskByte32(CPU_BYTE *bit);

