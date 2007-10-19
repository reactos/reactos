
#include "../../misc.h"


/* example how setup a opcode, this opcode is 16bit long (taken from M68K)
 * 0 and 1 mean normal bit, 2 mean mask bit the bit that are determent diffent
 * thing in the opcode, example which reg so on, it can be etither 0 or 1 in
 * the opcode. but a opcode have also normal bit that is always been set to
 * same. thuse bit are always 0 or 1
 */

/* FIXME RA should be 0 in stwu */
CPU_BYTE cpuPPCInit_Blr[32]   = {0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0, 0,1,0,0,1,1, 1,0};
/* addi */
CPU_BYTE cpuPPCInit_Li[32]    = {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0,0,0,0,0, 0,0,1,1,1,0, 2,2};
CPU_BYTE cpuPPCInit_stw[32]   = {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 1,0,0,1,0,0, 2,2};
CPU_BYTE cpuPPCInit_stwu[32]  = {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 1,0,0,1,0,1, 2,2};
CPU_BYTE cpuPPCInit_mr[32]    = {0,1,1,1,1,0,0,0,0,0,0,0,1,0,1,1,2,2,2,2,2,2,2,2, 0,1,1,1,1,1, 2,2};
CPU_BYTE cpuPPCInit_lwz[32]   = {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 1,0,0,0,0,0, 2,2};



/* mask */
/*
 * no mask  we implement function getting the reg right
 */

/* bit index
                          3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
                          1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
*/

