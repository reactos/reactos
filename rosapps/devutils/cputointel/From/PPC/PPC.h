
#include "../../misc.h"

CPU_INT PPCBrain(    CPU_BYTE *cpu_buffer,
                     CPU_UNINT cpu_pos,
                     CPU_UNINT cpu_size,
                     CPU_UNINT BaseAddress,
                     CPU_UNINT cpuarch,
                     FILE *outfp,
                     CPU_INT mode);


/* here we put the prototype for the opcode api that brain need we show a example for it */
CPU_INT PPC_Addx(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress, CPU_UNINT cpuarch, CPU_INT mode);


CPU_INT PPC_Ld( FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos,
                   CPU_UNINT cpu_size, CPU_UNINT BaseAddress, CPU_UNINT cpuarch,
                   CPU_INT mode);

/* Export comment thing see m68k for example 
 * in dummy we do not show it, for it is diffent for each cpu
 */

extern CPU_BYTE PPC_D[32];
extern CPU_BYTE PPC_A[32];
extern CPU_BYTE PPC_ds[32];
