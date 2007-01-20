
#include <stdio.h>
#include <stdlib.h> 
#include "IA32Brain.h"
#include "IA32.h"
#include "../../any_op.h"
#include "../../misc.h"




/* 
 * DummyBrain is example how you create you own cpu brain to translate from 
 * cpu to intel assembler, I have not add DummyBrain to the loader it is not
 * need it in our example. When you write you own brain, it must be setup in
 * misc.c function LoadPFileImage and PEFileStart, PEFileStart maybe does not
 * need the brain you have writen so you do not need setup it there then.
 *
 * input param: 
 *         cpu_buffer   : the memory buffer with loaded program we whant translate
 *         cpu_pos      : the positions in the cpu_buffer 
 *         cpu_size     : the alloced memory size of the cpu_buffer
 *         BaseAddress  : the virtual memory address we setup to use.
 *         cpuarch      : the sub arch for the brain, example if it exists more one
 *                        cpu with same desgin but few other opcode or extend opcode
 *         outfp        : the output file pointer
 *
 *           mode       : if we should run disambler of this binary or
 *                        translate it, Disambler will not calc the
 *                        the row name right so we simple give each
                          row a name. In translations mode we run a 
 *                        analys so we getting better optimzing and 
 *                        only row name there we need.
 *                        value for mode are :
 *                                             0 = disambler mode
 *                                             1 = translate mode intel
 *
 * return value
 *         0            : Ok 
 *         1            : unimplemt 
 *         2            : Unkonwn Opcode
 *         3            : unimplement cpu
 *         4            : unknown machine
 */

CPU_INT IA32Brain(  CPU_BYTE *cpu_buffer,
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

        /* use the GetData32Be or GetData32Le
           to read from the memory the
           Le is for small endian and the
           Be is for big endian
           the 32 is how many bits we should read 
         */
        cpuint = GetData32Be(&cpu_buffer[cpu_pos]);
    
        /* Add */
        if ((cpuint - (cpuint & GetMaskByte(cpuIA32Init_Add))) == ConvertBitToByte(cpuIA32Init_Add))
        {
            retsize = IA32_Add( outfp, cpu_buffer, cpu_pos, cpu_size,
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
