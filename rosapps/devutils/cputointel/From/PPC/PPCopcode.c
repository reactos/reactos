
#include <stdio.h>
#include <stdlib.h> 
#include "PPC.h"
#include "../../misc.h"
#include "../../any_op.h"

/* reg r0-r31 
  r3 = eax
 */

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

/* Get Dest register */
#define PPC_GetBitArraySrcReg(opcode) (((opcode & 0x3) << 3) | ((opcode & 0xE000) >> 13))

/* Get Source register */
CPU_UNINT PPC_GetBitArrayBto31xx(CPU_UNINT opcode)
{
    CPU_INT x1;

   /* FIXME make it to a macro
    * not tested to 100% yet */
    x1 = ((opcode & 0x1F00)>>8);
    return  x1;
}


CPU_UNINT PPC_GetBitArrayBto31(CPU_UNINT opcode)
{
    CPU_INT x1;
   /* FIXME make it to a macro
    * not tested to 100% yet */
   x1 = ((opcode & 0xFFFF0000)>>16);
    return  x1;
}


CPU_INT PPC_Blr( FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos,
                   CPU_UNINT cpu_size, CPU_UNINT BaseAddress, CPU_UNINT cpuarch)
{

    BaseAddress +=cpu_pos;

    /* own translatons langues */
    if (AllocAny()!=0)  /* alloc memory for pMyBrainAnalys */
    {
       return -1;
    }
    pMyBrainAnalys->op = OP_ANY_ret;
    pMyBrainAnalys->memAdr=BaseAddress;

    return 4;
}


CPU_INT PPC_Li( FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos,
                   CPU_UNINT cpu_size, CPU_UNINT BaseAddress, CPU_UNINT cpuarch)
{
    CPU_UNINT opcode;

    opcode = GetData32Le(&cpu_buffer[cpu_pos]);

    BaseAddress +=cpu_pos;

    /* own translatons langues */
    if (AllocAny()!=0)  /* alloc memory for pMyBrainAnalys */
    {
        return -1;
    }
    pMyBrainAnalys->op = OP_ANY_mov;
    pMyBrainAnalys->type= 8 + 16; /* 8 dst reg, 16 imm */
    pMyBrainAnalys->src_size = 16;
    pMyBrainAnalys->src = PPC_GetBitArraySrcReg(opcode);
    pMyBrainAnalys->dst = PPC_GetBitArrayBto31(opcode);
    pMyBrainAnalys->memAdr=BaseAddress;

    return 4;
}


CPU_INT PPC_mr( FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos,
                   CPU_UNINT cpu_size, CPU_UNINT BaseAddress, CPU_UNINT cpuarch)
{
    CPU_UNINT opcode;

    opcode = GetData32Le(&cpu_buffer[cpu_pos]);

    BaseAddress +=cpu_pos;

    /* own translatons langues */
    if (AllocAny()!=0)  /* alloc memory for pMyBrainAnalys */
    {
        return -1;
    }
    pMyBrainAnalys->op = OP_ANY_mov;
    pMyBrainAnalys->type= 2 + 8; /* 8 dst reg, 2 src reg */
    pMyBrainAnalys->src_size = 32;
    pMyBrainAnalys->src = PPC_GetBitArraySrcReg(opcode);
    pMyBrainAnalys->dst = PPC_GetBitArrayBto31xx(opcode);
    pMyBrainAnalys->memAdr=BaseAddress;

    return 4;
}


CPU_INT PPC_Stw( FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos,
                  CPU_UNINT cpu_size, CPU_UNINT BaseAddress, CPU_UNINT cpuarch)
{
    /* r1 store at -0x20(r1) */

    CPU_UNINT opcode;
    CPU_SHORT tmp = 0;

    opcode = GetData32Le(&cpu_buffer[cpu_pos]);

    BaseAddress +=cpu_pos;

    /* own translatons langues */
    if (AllocAny()!=0)  /* alloc memory for pMyBrainAnalys */
    {
        return -1;
    }

    tmp =  _byteswap_ushort( ((CPU_SHORT)((opcode >> 16) & 0xffff)));

    pMyBrainAnalys->op = OP_ANY_mov;
    pMyBrainAnalys->type= 2 + 64;
    pMyBrainAnalys->src_size = 32;
    pMyBrainAnalys->dst_size = 32;
    pMyBrainAnalys->src = PPC_GetBitArraySrcReg(opcode);
    pMyBrainAnalys->dst = PPC_GetBitArrayBto31xx(opcode);
    pMyBrainAnalys-> dst_extra = tmp;
    pMyBrainAnalys->memAdr=BaseAddress;

    return 4;
}

CPU_INT PPC_Stwu( FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos,
                  CPU_UNINT cpu_size, CPU_UNINT BaseAddress, CPU_UNINT cpuarch)
{
    /* r1 store at -0x20(r1) */

    CPU_UNINT opcode;
    CPU_INT DstReg;
    CPU_SHORT tmp = 0;

    opcode = GetData32Le(&cpu_buffer[cpu_pos]);

    DstReg = PPC_GetBitArrayBto31xx(opcode);
    if (DstReg == 0)
    {
        return 0;
    }

    BaseAddress +=cpu_pos;

    /* own translatons langues */
    if (AllocAny()!=0)  /* alloc memory for pMyBrainAnalys */
    {
        return -1;
    }

    tmp =  _byteswap_ushort( ((CPU_SHORT)((opcode >> 16) & 0xffff)));

    pMyBrainAnalys->op = OP_ANY_mov;
    pMyBrainAnalys->type= 2 + 64 + 128;
    pMyBrainAnalys->src_size = 32;
    pMyBrainAnalys->dst_size = 32;
    pMyBrainAnalys->src = PPC_GetBitArraySrcReg(opcode);
    pMyBrainAnalys->dst = DstReg;
    pMyBrainAnalys-> dst_extra = tmp;
    pMyBrainAnalys->memAdr=BaseAddress;

    return 4;
}
