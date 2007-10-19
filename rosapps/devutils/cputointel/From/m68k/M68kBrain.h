
#include "../../misc.h"

CPU_BYTE cpuM68kInit_Abcd[16]      = {1,1,1,1,2,2,2,1,0,0,0,0,2,2,2,2};
CPU_BYTE cpuM68kInit_Add[16]       = {1,1,0,1,2,2,2,2,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Addi[16]      = {0,0,0,0,0,1,1,0,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Addq[16]      = {0,1,0,1,2,2,2,0,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Addx[16]      = {1,1,0,1,2,2,2,1,2,2,0,0,2,2,2,2};
CPU_BYTE cpuM68kInit_And[16]       = {1,1,0,0,2,2,2,2,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Andi[16]      = {0,0,0,0,0,0,1,0,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_AndToCCRF[16] = {0,0,0,0,0,0,1,0,0,0,1,1,1,1,0,0};
CPU_BYTE cpuM68kInit_AndToCCRS[16] = {0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Asl[16]       = {1,1,1,0,2,2,2,0,2,2,2,0,0,2,2,2};
CPU_BYTE cpuM68kInit_Asr[16]       = {1,1,1,0,2,2,2,1,2,2,2,0,0,2,2,2};

CPU_BYTE cpuM68kInit_Bhi[16]       = {0,1,1,0,0,0,1,0,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Bls[16]       = {0,1,1,0,0,0,1,1,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Bcc[16]       = {0,1,1,0,0,1,0,0,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Bcs[16]       = {0,1,1,0,0,1,0,1,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Bne[16]       = {0,1,1,0,0,1,1,0,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Beq[16]       = {0,1,1,0,0,1,1,1,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Bvc[16]       = {0,1,1,0,1,0,0,0,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Bvs[16]       = {0,1,1,0,1,0,0,1,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Bpl[16]       = {0,1,1,0,1,0,1,0,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Bmi[16]       = {0,1,1,0,1,0,1,1,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Bge[16]       = {0,1,1,0,1,1,0,0,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Blt[16]       = {0,1,1,0,1,1,0,1,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Bgt[16]       = {0,1,1,0,1,1,1,0,2,2,2,2,2,2,2,2};
CPU_BYTE cpuM68kInit_Ble[16]       = {0,1,1,0,1,1,1,1,2,2,2,2,2,2,2,2};


CPU_BYTE M68k_Rx[16]     = {0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0};
CPU_BYTE M68k_RM[16]     = {0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0};
CPU_BYTE M68k_Ry[16]     = {0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1};
CPU_BYTE M68k_Opmode[16] = {0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0};
CPU_BYTE M68k_Mode[16]   = {0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0};
CPU_BYTE M68k_Size[16]   = {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0};

