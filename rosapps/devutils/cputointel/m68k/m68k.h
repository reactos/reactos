
#include "../misc.h"

CPU_INT M68k_Abcd(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress);
CPU_INT M68k_Add(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress);
CPU_INT M68k_Addi(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress);
CPU_INT M68k_Addq(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress);
CPU_INT M68k_Addx(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress);
CPU_INT M68k_And(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress);
CPU_INT M68k_Andi(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress);
CPU_INT M68k_AndToCCR(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress);
CPU_INT M68k_Asl(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress);
CPU_INT M68k_Asr(FILE *out, CPU_BYTE * cpu_buffer, CPU_UNINT cpu_pos, CPU_UNINT cpu_size, CPU_UNINT BaseAddress);

extern CPU_BYTE M68k_Rx[16];
extern CPU_BYTE M68k_RM[16];
extern CPU_BYTE M68k_Ry[16];
extern CPU_BYTE M68k_Opmode[16];
extern CPU_BYTE M68k_Mode[16];
extern CPU_BYTE M68k_Size[16];
