
#include "../../misc.h"


/* example how setup a opcode, this opcode is 16bit long (taken from M68K)
 * 0 and 1 mean normal bit, 2 mean mask bit the bit that are determent diffent
 * thing in the opcode, example which reg so on, it can be etither 0 or 1 in
 * the opcode. but a opcode have also normal bit that is always been set to
 * same. thuse bit are always 0 or 1
 */
CPU_BYTE cpuARMInit_[32] = {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2};

