
#include <stdio.h>
#include <stdlib.h> 
#include "M68kBrain.h"
#include "m68k.h"
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
 * return value
 *         0            : Ok 
 *         1            : unimplemt 
 *         2            : Unkonwn Opcode
 *         3            : unimplement cpu
 *         4            : unknown machine
 */

CPU_INT M68KBrain(   CPU_BYTE *cpu_buffer,
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
    
        /* Abcd */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Abcd))) == ConvertBitToByte(cpuM68kInit_Abcd))
        {
            retsize = M68k_Abcd( outfp, cpu_buffer, cpu_pos, cpu_size,
                                 BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }
        /* Add */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Add))) == ConvertBitToByte(cpuM68kInit_Add))
        {
            retsize = M68k_Add( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }
        /* Addi */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Addi))) == ConvertBitToByte(cpuM68kInit_Addi))
        {
            retsize = M68k_Addi( outfp, cpu_buffer, cpu_pos, cpu_size,
                                 BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }
        /* Addq */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Addq))) == ConvertBitToByte(cpuM68kInit_Addq))
        {
            retsize = M68k_Addq( outfp, cpu_buffer, cpu_pos, cpu_size,
                                 BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }
        /* Addx */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Addx))) == ConvertBitToByte(cpuM68kInit_Addx))
        {
            retsize = M68k_Addx( outfp, cpu_buffer, cpu_pos, cpu_size,
                                 BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }
        /* And */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_And))) == ConvertBitToByte(cpuM68kInit_And))
        {
            retsize = M68k_Add( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }
        /* Andi */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Andi))) == ConvertBitToByte(cpuM68kInit_Andi))
        {
            retsize = M68k_Andi( outfp, cpu_buffer, cpu_pos, cpu_size,
                                 BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }
        /* AndToCCR */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_AndToCCRF))) == ConvertBitToByte(cpuM68kInit_AndToCCRF))
        {            
            cpuint = cpu_buffer[cpu_pos+1];
            if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_AndToCCRS))) == ConvertBitToByte(cpuM68kInit_AndToCCRS))
            {
                cpu_pos++;
                retsize = M68k_AndToCCR( outfp, cpu_buffer, cpu_pos, cpu_size,
                                         BaseAddress, cpuarch);
                if (retsize<0)
                    retcode = 1;
                else
                    cpu_pos += retsize;
            }
            else
            {
                cpuint = cpu_buffer[cpu_pos];
            }
        }

        /* Bhi */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Bhi))) == ConvertBitToByte(cpuM68kInit_Bhi))
        {
            retsize = M68k_Bhi( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }

        /* Bls */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Bls))) == ConvertBitToByte(cpuM68kInit_Bls))
        {
            retsize = M68k_Bls( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }

        /* Bcc */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Bcc))) == ConvertBitToByte(cpuM68kInit_Bcc))
        {
            retsize = M68k_Bcc( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }

        /* Bcs */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Bcs))) == ConvertBitToByte(cpuM68kInit_Bcs))
        {
            retsize = M68k_Bcs( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }

        /* Bne */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Bne))) == ConvertBitToByte(cpuM68kInit_Bne))
        {
            retsize = M68k_Bne( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }

        /* Beq */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Beq))) == ConvertBitToByte(cpuM68kInit_Beq))
        {
            retsize = M68k_Beq( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }

        /* Bvc */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Bvc))) == ConvertBitToByte(cpuM68kInit_Bvc))
        {
            retsize = M68k_Bvc( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }

        /* Bvs */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Bvs))) == ConvertBitToByte(cpuM68kInit_Bvs))
        {
            retsize = M68k_Bvs( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }

        /* Bpl */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Bpl))) == ConvertBitToByte(cpuM68kInit_Bpl))
        {
            retsize = M68k_Bpl( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }

        /* Bmi */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Bmi))) == ConvertBitToByte(cpuM68kInit_Bmi))
        {
            retsize = M68k_Bmi( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }

        /* Bge */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Bge))) == ConvertBitToByte(cpuM68kInit_Bge))
        {
            retsize = M68k_Bge( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }

        /* Blt */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Blt))) == ConvertBitToByte(cpuM68kInit_Blt))
        {
            retsize = M68k_Blt( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }

        /* Bgt */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Bgt))) == ConvertBitToByte(cpuM68kInit_Bgt))
        {
            retsize = M68k_Bgt( outfp, cpu_buffer, cpu_pos, cpu_size,
                                BaseAddress, cpuarch);
            if (retsize<0)
                 retcode = 1;
            else
                 cpu_pos += retsize;
        }

        /* Ble */
        if ((cpuint - (cpuint & GetMaskByte(cpuM68kInit_Ble))) == ConvertBitToByte(cpuM68kInit_Ble))
        {
            retsize = M68k_Ble( outfp, cpu_buffer, cpu_pos, cpu_size,
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
