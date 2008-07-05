
/* only for getting the pe struct */
#include <windows.h>
#include <winnt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "any_op.h"
#include "From/ARM/ARM.h"
#include "From/m68k/m68k.h"
#include "From/PPC/PPC.h"


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
 *-------------------------
 * type 0 : auto
 * type 1 : bin
 * type 2 : exe/dll/sys
 */



/* Conveting bit array to a int byte */
CPU_UNINT ConvertBitToByte(CPU_BYTE *bit)
{
    CPU_UNINT Byte = 0;
    CPU_INT t;
    CPU_UNINT size = 15;

    for(t=size;t>=0;t--)
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
    CPU_INT t;
    CPU_UNINT size = 15;

    for(t=size;t>=0;t--)
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
    CPU_INT t;
    CPU_UNINT size = 31;

    for(t=size;t>=0;t--)
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
    CPU_INT t;
    CPU_UNINT size = 31;

    for(t=size;t>=0;t--)
    {
        if (bit[size-t] == 2)
        {
            MaskByte = MaskByte + ( (bit[size-t]-1) <<t);
        }
    }
    return MaskByte;
}



CPU_UNINT GetData32Le(CPU_BYTE *cpu_buffer)
{
    CPU_UNINT cpuint;
    CPU_UNINT split1;
    CPU_UNINT split2;
    CPU_UNINT split3;
    CPU_UNINT split4;

    cpuint = *((CPU_UNINT*) &cpu_buffer[0]);

    split1 = cpu_buffer[0];
    split2 = cpu_buffer[1];
    split3 = cpu_buffer[2];
    split4 = cpu_buffer[3];


    cpuint = split4+(split3 <<8 )+(split2 <<16 )+(split1 <<24 );

    return cpuint;
}

CPU_UNINT GetData32Be(CPU_BYTE *cpu_buffer)
{
    CPU_UNINT cpuint;

    cpuint = *((CPU_UNINT*) &cpu_buffer[0]);

    return cpuint;
}


CPU_INT AllocAny()
{

    if (pMyBrainAnalys== NULL)
    {
        pMyBrainAnalys = (PMYBrainAnalys) malloc(sizeof(MYBrainAnalys));
        if (pMyBrainAnalys==NULL)
        {
            return -1;
        }
        ZeroMemory(pMyBrainAnalys,sizeof(MYBrainAnalys));
        pStartMyBrainAnalys = pMyBrainAnalys;
    }
    else
    {
        PMYBrainAnalys tmp;
        tmp = (PMYBrainAnalys) malloc(sizeof(MYBrainAnalys));
        if (tmp==NULL)
        {
            return -1;
        }
        ZeroMemory(tmp,sizeof(MYBrainAnalys));

        pMyBrainAnalys->ptr_next = (CPU_BYTE*)tmp;
        tmp->ptr_prev= (CPU_BYTE*)pMyBrainAnalys;

        pMyBrainAnalys = tmp;
    }
return 0;
}

CPU_INT FreeAny()
{
  PMYBrainAnalys tmp = NULL;

  if (pMyBrainAnalys == NULL)
  {
      return -1;
  }

  tmp = (PMYBrainAnalys)pMyBrainAnalys->ptr_prev;

  while (pMyBrainAnalys != NULL)
  {
    if (pMyBrainAnalys == NULL)
    {
        break;
    }

    free(pMyBrainAnalys);

    if (pMyBrainAnalys != NULL)
    {
        printf("fail to free memory");
        return -1;
    }

    pMyBrainAnalys = tmp;
  }

  return 0;
}






