
#ifndef __ANY_OP_H__
#define __ANY_OP_H__

#define OP_ANY_mov  0x00000000
#define OP_ANY_ret  0x00000001

typedef struct _BrainAnalys
{
    CPU_UNINT op;  /*  one tranlator for any cpu type set our own opcode */
    CPU_INT  type; /*  1 = source are memmory, 2 source are register */
                   /*  4 = dest are memmory,   8 dest are register */
                   /*  16 = source are imm                          */

    CPU_INT src_size; /* who many bits are src not vaild for reg*/
    CPU_INT dst_size; /* who many bits are dst not vaild for reg*/

    CPU_UNINT64 src;
    CPU_UNINT64 dst;

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

CPU_INT ConvertToIntelProcess( FILE *outfp, CPU_INT eax, CPU_INT ebp,
                               CPU_INT edx, CPU_INT esp, 
                               PMYBrainAnalys pMystart, 
                               PMYBrainAnalys pMyend, CPU_INT regbits,
                               CPU_INT HowManyRegInUse);

#endif
