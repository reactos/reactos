
#include <stdio.h>
#include <stdlib.h>
#include "../../misc.h"


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
 *                mode       : if we should run disambler of this binary or
 *                             translate it, Disambler will not calc the
 *                             the row name right so we simple give each
                               row a name. In translations mode we run a
 *                             analys so we getting better optimzing and
 *                             only row name there we need.
 *                             value for mode are :
 *                                                  0 = disambler mode
 *                                                  1 = translate mode intel

 *
 * Return value :
 *               value -1            : unimplement
 *               value  0            : wrong opcode or not vaild opcode
 *               value +1 and higher : who many byte we should add to cpu_pos
 */

CPU_INT ARM_( FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos,
                   CPU_UNINT cpu_size, CPU_UNINT BaseAddress, CPU_UNINT cpuarch)

{
    /*
     * ConvertBitToByte() is perfect to use to get the bit being in use from a bit array
     * GetMaskByte() is perfect if u whant known which bit have been mask out
     * see M68kopcode.c and how it use the ConvertBitToByte()
     */

    fprintf(out,"Line_0x%8x :\n",BaseAddress + cpu_pos);

    printf(";Add unimplement\n");
    return -1;
}
