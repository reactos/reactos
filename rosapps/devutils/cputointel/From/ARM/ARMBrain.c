
#include <stdio.h>
#include <stdlib.h> 
#include "ARMBrain.h"
#include "ARM.h"
#include "../../misc.h"

/* retun 
 *         0            : Ok 
 *         1            : unimplemt 
 *         2            : Unkonwn Opcode
 *         3            : unimplement cpu
 *         4            : unknown machine
 */

CPU_INT ARMBrain(  CPU_BYTE *cpu_buffer,
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
        if ((cpuint - (cpuint & GetMaskByte32(cpuARMInit_))) == ConvertBitToByte32(cpuARMInit_))
        {
            retsize = ARM_( outfp, cpu_buffer, cpu_pos, cpu_size,
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
