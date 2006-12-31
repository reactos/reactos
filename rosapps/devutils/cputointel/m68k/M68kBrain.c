
#include <stdio.h>
#include <stdlib.h> 
#include "M68kBrain.h"
#include "m68k.h"
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

CPU_INT M68KBrain(char *infileName, char *outputfileName, CPU_UNINT BaseAddress, CPU_UNINT cpuarch)
{
    FILE *infp;
    FILE *outfp;
    CPU_BYTE *cpu_buffer;   
    CPU_UNINT cpu_pos = 0;
    CPU_UNINT cpu_oldpos;
    CPU_UNINT cpu_size=0;
    CPU_INT cpuint;
    CPU_INT retcode = 0;
    CPU_INT retsize;

    /* Open file for read */
    if (!(infp = fopen(infileName,"RB")))
    {
        printf("Can not open file %s\n",infileName);
        return 3;
    }

    /* Open file for write */
    if (!(outfp = fopen(outputfileName,"WB")))
    {
        printf("Can not open file %s\n",outputfileName);
        return 4;
    }

    /* Load the binary file to a memory buffer */
    fseek(infp,0,SEEK_END);
    if (!ferror(infp))
    {
        printf("error can not seek in the read file");
        fclose(infp);
        fclose(outfp);        
        return 5;
    }
    
    /* get the memory size buffer */
    cpu_size = ftell(infp);
    if (!ferror(infp))
    {
        printf("error can not get file size of the read file");
        fclose(infp);
        fclose(outfp);        
        return 6;
    }

    if (cpu_size==0)
    {
        printf("error file size is Zero lenght of the read file");
        fclose(infp);
        fclose(outfp);        
        return 7;
    }

    /* alloc memory now */
    if (!(cpu_buffer = (unsigned char *) malloc(cpu_size)))
    {
        printf("error can not alloc %uld size for memory buffer",cpu_size);
        fclose(infp);
        fclose(outfp);
        return 8;
    }

    /* read from the file now in one sweep */
    fread(cpu_buffer,1,cpu_size,infp);
    if (!ferror(infp))
    {
        printf("error can not read file ");
        fclose(infp);
        fclose(outfp);        
        return 9;
    }
    fclose(infp);

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
