
#include <stdio.h>
#include <stdlib.h> 
#include "PPCBrain.h"
#include "PPC.h"
#include "../misc.h"

/* retun 
 * 0 = Ok 
 * 1 = unimplemt 
 * 2 = Unkonwn Opcode 
 * 3 = can not open read file
 * 4 = can not open write file
 * 5 = can not seek to end of read file
 * 6 = can not get the file size of the read file
 * 7 = read file size is Zero
 * 8 = can not alloc memory
 * 9 = can not read file
 */

CPU_INT PPCBrain(    CPU_BYTE *cpu_buffer,
                     CPU_UNINT cpu_pos,
                     CPU_UNINT cpu_size,
                     CPU_UNINT BaseAddress,
                     CPU_UNINT cpuarch,
                     FILE *outfp)
{
    CPU_UNINT cpu_oldpos;
    CPU_INT cpuint;
    CPU_INT retcode = 0;
    CPU_INT retsize;


    /* now we start the process */
    while (cpu_pos<cpu_size)
    {
        cpu_oldpos = cpu_pos;

        cpuint = cpu_buffer[cpu_pos];
    
        /* Add */
        if ((cpuint - (cpuint & GetMaskByte32(cpuPPCInit_Addx))) == ConvertBitToByte32(cpuPPCInit_Addx))
        {
            retsize = PPC_Addx( outfp, cpu_buffer, cpu_pos, cpu_size,
                                 BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }
    
        /* Found all Opcode and breakout and return no error found */
        if (cpu_pos >=cpu_size)
        {
            break;
        }

        /* Check if we have found a cpu opcode */
        if (cpu_oldpos == cpu_pos)
        {            
            if (retcode == 0)
            {              
                /* no unimplement error where found so we return a msg for unknown opcode */
                printf("Unkonwn Opcode found at 0x%8x opcode 0x%2x\n",cpu_oldpos+BaseAddress,(unsigned int)cpu_buffer[cpu_oldpos]);                
                retcode = 2;
            }
        }

        /* Erorro Found ? */
        if (retcode!=0)
        {
            /* Erorro Found break and return the error code */
            break;
        }
    }
    return retcode;    
}
