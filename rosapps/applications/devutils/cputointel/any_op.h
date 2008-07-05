
#ifndef __ANY_OP_H__
#define __ANY_OP_H__

#define OP_ANY_mov  0x00000000
#define OP_ANY_ret  0x00000001

/* We are using same abi as PPC
 * eax = register 3
 * edx = register 4
 * esp = register 1
 * ebp = register 31

 * ecx = 8
 * ebx = 9
 * esi = 10
 * edi = 11
 * mmx/sse/fpu 0 = 12
 * mmx/sse/fpu 1 = 14
 * mmx/sse/fpu 2 = 16
 * mmx/sse/fpu 3 = 18
 * mmx/sse/fpu 4 = 20
 * mmx/sse/fpu 5 = 22
 * mmx/sse/fpu 6 = 24
 * mmx/sse/fpu 7 = 28
 */

typedef struct _BrainAnalys
{
    CPU_UNINT op;  /*  one tranlator for any cpu type set our own opcode */
    CPU_INT  type; /*  1 = source are memmory, 2 source are register */
                   /*  4 = dest are memmory,   8 dest are register */
                   /*  16 = source are imm                          */
                   /*  32 =  soucre -xx(r1) or [eax-xx] */
                   /*  64 =  dest -xx(r1) or [eax-xx] */
                   /*  128 =  update form the src be update with dest */

    CPU_INT src_size; /* who many bits are src not vaild for reg*/
    CPU_INT dst_size; /* who many bits are dst not vaild for reg*/

    CPU_UNINT64 src;
    CPU_UNINT64 dst;

    CPU_INT src_extra; /* if type == 32 are set */
    CPU_INT dst_extra; /* if type == 32 are set */

    CPU_UNINT memAdr; /* where are we in the current memory pos + baseaddress */

    CPU_INT row; /* 0 = no row,
                  * 1 = row is bcc (conditions),
                  * 2 = row is jsr (Call)
                  */

    /* try translate the Adress to a name */
    CPU_BYTE* ptr_next; /* hook next one */
    CPU_BYTE* ptr_prev; /* hook previus one */
} MYBrainAnalys, *PMYBrainAnalys;

extern PMYBrainAnalys pMyBrainAnalys;     /* current working address */
extern PMYBrainAnalys pStartMyBrainAnalys; /* start address */

CPU_INT ConvertToIA32Process( FILE *outfp,
                               PMYBrainAnalys pMystart,
                               PMYBrainAnalys pMyend, CPU_INT regbits,
                               CPU_INT HowManyRegInUse,
                               CPU_INT *RegTableCount);

CPU_INT ConvertToPPCProcess( FILE *outfp,
                               PMYBrainAnalys pMystart,
                               PMYBrainAnalys pMyend, CPU_INT regbits,
                               CPU_INT HowManyRegInUse,
                               CPU_INT *RegTableCount);

#endif
