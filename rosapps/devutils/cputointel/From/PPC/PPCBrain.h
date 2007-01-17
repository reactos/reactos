
#include "../../misc.h"


/* example how setup a opcode, this opcode is 16bit long (taken from M68K) 
 * 0 and 1 mean normal bit, 2 mean mask bit the bit that are determent diffent 
 * thing in the opcode, example which reg so on, it can be etither 0 or 1 in 
 * the opcode. but a opcode have also normal bit that is always been set to 
 * same. thuse bit are always 0 or 1
 */

CPU_BYTE cpuPPCInit_Blr[32]  = {0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,1,1,0};


CPU_BYTE cpuPPCInit_Ld[32]  = {0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,1,1,1,0,0,0};
CPU_BYTE cpuPPCInit_Ldu[32] = {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,1,1,1,0,0,0};


/* mask */
CPU_BYTE PPC_D[32]     = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0};
CPU_BYTE PPC_A[32]     = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0};
CPU_BYTE PPC_ds[32]    = {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/* bit index
                          3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
                          1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
*/

