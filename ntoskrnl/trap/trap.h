_ONCE

#define TRAPF_NOSAVESEG		1
#define TRAPF_NOSAVEFS		2
#define TRAPF_SAVENOVOL		4
#define TRAPF_VECTOR		0x100
#define TRAPF_ERRORCODE		0x200
#define TRAPF_SYSENTER		0x400

extern KPCR const *TrapPcr;

#include <trap_asm.h>


