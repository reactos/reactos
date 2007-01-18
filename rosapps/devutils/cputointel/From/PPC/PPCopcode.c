
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

/* only for ppc */
#define PPC_GetBitArray6toA(opcode) (((opcode & 0x3) << 3) | ((opcode & 0xE000) >> 13))




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

    opcode = GetData32Le(cpu_buffer);

    BaseAddress +=cpu_pos;

    /* own translatons langues */
    if (AllocAny()!=0)  /* alloc memory for pMyBrainAnalys */
    {
        return -1;
    }
    pMyBrainAnalys->op = OP_ANY_mov;
    pMyBrainAnalys->type= 8 + 16; /* 8 dst reg, 16 imm */
    pMyBrainAnalys->src_size = 16;
    pMyBrainAnalys->src = PPC_GetBitArrayBto31(opcode);
    pMyBrainAnalys->dst = PPC_GetBitArray6toA(opcode);
    pMyBrainAnalys->memAdr=BaseAddress;

    return 4;
}

