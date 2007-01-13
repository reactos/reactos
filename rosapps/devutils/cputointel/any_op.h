

#define OP_ANY_mov  0x00000000


typedef struct _BrainAnalys
{
    CPU_UNINT op;  /*  one tranlator for any cpu type set our own opcode */
    CPU_INT  type; /*  0 = source are memmory, 1 source are register */
                   /*  2 = dest are memmory,   4 dest are register */
                   /*  8 = source are imm                          */
    CPU_INT src_size; /* who many bits are src not vaild for reg*/
    CPU_INT dst_size; /* who many bits are dst not vaild for reg*/

    CPU_UNINT64 src;
    CPU_UNINT64 dst;

    /* try translate the Adress to a name */
    CPU_BYTE* ptr_next; /* hook next one */
    CPU_BYTE* ptr_prev; /* hook previus one */
} MYBrainAnalys, *PMYBrainAnalys;

extern PMYBrainAnalys pMyBrainAnalys;

