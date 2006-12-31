
#include "../misc.h"

CPU_BYTE cpuint_table_Abcd[16]      = {1,1,1,1,2,2,2,1,0,0,0,0,2,2,2,2}; 
CPU_BYTE cpuint_table_Add[16]       = {1,1,0,1,2,2,2,2,2,2,2,2,2,2,2,2};
CPU_BYTE cpuint_table_Addi[16]      = {0,0,0,0,0,1,1,0,2,2,2,2,2,2,2,2};
CPU_BYTE cpuint_table_Addq[16]      = {0,1,0,1,2,2,2,0,2,2,2,2,2,2,2,2};
CPU_BYTE cpuint_table_Addx[16]      = {1,1,0,1,2,2,2,1,2,2,0,0,2,2,2,2};
CPU_BYTE cpuint_table_And[16]       = {1,1,0,0,2,2,2,2,2,2,2,2,2,2,2,2};
CPU_BYTE cpuint_table_Andi[16]      = {0,0,0,0,0,0,1,0,2,2,2,2,2,2,2,2};
CPU_BYTE cpuint_table_AndToCCRF[16] = {0,0,0,0,0,0,1,0,0,0,1,1,1,1,0,0};
CPU_BYTE cpuint_table_AndToCCRS[16] = {0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2};
CPU_BYTE cpuint_table_Asl[16]       = {1,1,1,0,2,2,2,0,2,2,2,0,0,2,2,2};
CPU_BYTE cpuint_table_Asr[16]       = {1,1,1,0,2,2,2,1,2,2,2,0,0,2,2,2};


CPU_BYTE table_Rx[16]     = {0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0};
CPU_BYTE table_RM[16]     = {0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0};
CPU_BYTE table_Ry[16]     = {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1};
CPU_BYTE table_Opmode[16] = {0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0};
CPU_BYTE table_Mode[16]   = {0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0};
CPU_BYTE table_Size[16]   = {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0};

