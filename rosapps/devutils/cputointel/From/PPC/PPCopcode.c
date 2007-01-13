
#include <stdio.h>
#include <stdlib.h> 
#include "PPC.h"
#include "../../misc.h"


/* cpuDummyInit_Add
 * Input param : 
 *               out         : The file pointer that we write to (the output file to intel asm) 
 *               cpu_buffer  : The memory buffer we have our binary code that we whant convert
 *               cpu_pos     : Current positions in the cpu_buffer 
 *               cpu_size    : The memory size of the cpu_buffer
 *               BaseAddress : The base address you whant the binay file should run from 
 *               cpuarch     : if it exists diffent cpu from a manufactor like pentium,
 *                             pentinum-mmx so on, use this flag to specify which type 
 *                             of cpu you whant or do not use it if it does not exists
 *                             other or any sub model.
 *
 * Return value :
 *               value -1            : unimplement 
 *               value  0            : wrong opcode or not vaild opcode
 *               value +1 and higher : who many byte we should add to cpu_pos
 */
 
CPU_INT PPC_Addx( FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos,
                   CPU_UNINT cpu_size, CPU_UNINT BaseAddress, CPU_UNINT cpuarch,
                   CPU_INT mode)

{
    /* 
     * ConvertBitToByte() is perfect to use to get the bit being in use from a bit array
     * GetMaskByte() is perfect if u whant known which bit have been mask out 
     * see M68kopcode.c and how it use the ConvertBitToByte()
     */

    fprintf(out,"Line_0x%8x :\n",BaseAddress + cpu_pos);

    printf(";Add unimplement\n");
    return -1;
}
 //                                                          stb
                                                             
 // li  %r3, 0  : op    00          00         60           38
 // li = ld
//                   0000 0000   0000 0000  0100 0000    0011 1000


CPU_INT PPC_Ld( FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos,
                   CPU_UNINT cpu_size, CPU_UNINT BaseAddress, CPU_UNINT cpuarch,
                   CPU_INT mode)
{
    CPU_UNINT formA;
    CPU_UNINT formD;
    CPU_UNINT formDS;
    CPU_UNINT opcode;

    opcode = GetData32Le(cpu_buffer);
    formA =  (opcode & ConvertBitToByte32(PPC_A)) >> 13;
    formD =  (opcode & ConvertBitToByte32(PPC_D)) >> 10;
    formDS = (opcode & ConvertBitToByte32(PPC_ds)) >> 15;

    if (mode==0)
    {
        fprintf(out,"Line_0x%08x:\n",BaseAddress + cpu_pos);
        fprintf(out,"li %%r%d,%d\n",formA, formDS);
    }

    printf(";not full implement \n");
    return 4;
}

