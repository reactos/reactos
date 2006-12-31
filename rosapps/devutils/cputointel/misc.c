
#include <stdio.h>
#include "misc.h"


/* Conveting bit array to a int byte */
CPU_UNINT ConvertBitToByte(CPU_BYTE *bit)
{
    CPU_UNINT Byte = 0;
    CPU_UNINT t;

    for(t=15;t>0;t--)
    {
        if (bit[15-t] != 2) 
            Byte = Byte + (bit[15-t]<<t);
    }
    return Byte;
}

/* Conveting bit array mask to a int byte mask */
CPU_UNINT GetMaskByte(CPU_BYTE *bit)
{
    CPU_UNINT MaskByte = 0;
    CPU_UNINT t;

    for(t=15;t>0;t--)
    {
        if (bit[15-t] == 2) 
        {            
            MaskByte = MaskByte + ( (bit[15-t]-1) <<t);
        }
    }
    return MaskByte;
}
