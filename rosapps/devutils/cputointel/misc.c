
#include <stdio.h>
#include "misc.h"


/* Conveting bit array to a int byte */
CPU_UNINT ConvertBitToByte(CPU_BYTE *bit)
{
    CPU_UNINT Byte = 0;
    CPU_UNINT t;
    CPU_UNINT size = 15;

    for(t=size;t>0;t--)
    {
        if (bit[size-t] != 2) 
            Byte = Byte + (bit[size-t]<<t);
    }
    return Byte;
}

/* Conveting bit array mask to a int byte mask */
CPU_UNINT GetMaskByte(CPU_BYTE *bit)
{
    CPU_UNINT MaskByte = 0;
    CPU_UNINT t;
    CPU_UNINT size = 15;

    for(t=size;t>0;t--)
    {
        if (bit[size-t] == 2) 
        {            
            MaskByte = MaskByte + ( (bit[size-t]-1) <<t);
        }
    }
    return MaskByte;
}

/* Conveting bit array to a int byte */
CPU_UNINT ConvertBitToByte32(CPU_BYTE *bit)
{
    CPU_UNINT Byte = 0;
    CPU_UNINT t;
    CPU_UNINT size = 31;

    for(t=size;t>0;t--)
    {
        if (bit[size-t] != 2) 
            Byte = Byte + (bit[size-t]<<t);
    }
    return Byte;
}

/* Conveting bit array mask to a int byte mask */
CPU_UNINT GetMaskByte32(CPU_BYTE *bit)
{
    CPU_UNINT MaskByte = 0;
    CPU_UNINT t;
    CPU_UNINT size = 31;

    for(t=size;t>0;t--)
    {
        if (bit[size-t] == 2) 
        {            
            MaskByte = MaskByte + ( (bit[size-t]-1) <<t);
        }
    }
    return MaskByte;
}


