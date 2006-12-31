
#include <stdio.h>
#include <stdlib.h> 
#include "m68k.h"
#include "misc.h"


CPU_INT M68k_Abcd(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress)
{
    fprintf(out,"Line_0x%8x :\n",BaseAddress + cpu_pos);

    printf(";Abcd unimplement\n");
    return -1;
}

CPU_INT M68k_Add(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress)
{
    CPU_INT opmode;
    CPU_INT mode;
    CPU_INT Rx;
    CPU_INT Ry;
    //CPU_INT cpuint;
            
    opmode = ConvertBitToByte(M68k_Opmode);
    mode = ConvertBitToByte(M68k_Mode);
    Rx = ConvertBitToByte(M68k_Rx);
    Ry = ConvertBitToByte(M68k_Ry);

    fprintf(out,"Line_0x%8x :\n",BaseAddress + cpu_pos);

    if (opmode == 0x00)
    {
        /* <ea> + Dn -> Dn */  
        printf(";Add unimplement of  \"<ea> + Dn -> Dn\" \n");
        
    }

    if (opmode == 0x01)
    {
        /* <ea> + Dn -> Dn */
        printf(";Add unimplement of \"<ea> + Dn -> Dn\" \n");
    }

    if (opmode == 0x02)
    {
        /* <ea> + Dn -> Dn */
        printf(";Add unimplement of \"<ea> + Dn -> Dn\" \n");
    }

    if (opmode == 0x03)
    {
        /* <ea> + An -> An */
        printf(";Add unimplement of \"<ea> + An -> An\" \n");
    }
    
    if (opmode == 0x04)
    {
        /* Dn + <ea> -> <ea> */
        printf(";Add unimplement of \"Dn + <ea> -> <ea>\" \n");
    }

    if (opmode == 0x05)
    {
        /* Dn + <ea> -> <ea> */
        printf(";Add unimplement of \"Dn + <ea> -> <ea>\" \n");
    }

    if (opmode == 0x06)
    {
        /* Dn + <ea> -> <ea> */
        printf(";Add unimplement of \"Dn + <ea> -> <ea>\" \n");
    }

    if (opmode == 0x07)
    {
        /* <ea> + An -> An */
        printf(";Add unimplement of \"<ea> + An -> An\" \n");
    }


    

    
    return -1;
}

CPU_INT M68k_Addi(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress)
{
    fprintf(out,"Line_0x%8x :\n",BaseAddress + cpu_pos);

    printf(";Addi unimplement\n");
    return -1;
}

CPU_INT M68k_Addq(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress)
{
    fprintf(out,"Line_0x%8x :\n",BaseAddress + cpu_pos);

    printf(";Addq unimplement\n");
    return -1;
}

CPU_INT M68k_Addx(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress)
{
    fprintf(out,"Line_0x%8x :\n",BaseAddress + cpu_pos);

    printf(";Addx unimplement\n");
    return -1;
}

CPU_INT M68k_And(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress)
{
    fprintf(out,"Line_0x%8x :\n",BaseAddress + cpu_pos);

    printf(";And unimplement\n");
    return -1;
}

CPU_INT M68k_Andi(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress)
{
    fprintf(out,"Line_0x%8x :\n",BaseAddress + cpu_pos);

    printf(";Andi unimplement\n");
    return -1;
}

CPU_INT M68k_AndToCCR(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress)
{
    fprintf(out,"Line_0x%8x :\n",BaseAddress + cpu_pos);

    printf(";AndToCCR unimplement\n");
    return -1;
}

CPU_INT M68k_Asl(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress)
{
    fprintf(out,"Line_0x%8x :\n",BaseAddress + cpu_pos);

    printf(";Asl unimplement\n");
    return -1;
}

CPU_INT M68k_Asr(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress)
{
    fprintf(out,"Line_0x%8x :\n",BaseAddress + cpu_pos);

    printf(";Asr unimplement\n");
    return -1;
}

