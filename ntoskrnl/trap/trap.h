_ONCE

// TRAP_STUB_FLAGS TrapStub x-macro flags
// trap type
#define TRAPF_ERRORCODE		1
#define TRAPF_INTERRUPT		2
#define TRAPF_FASTSYSCALL	4
// options
#define TRAPF_NOSAVESEG		0x100
#define TRAPF_SAVEFS		0x200
#define TRAPF_SAVENOVOL		0x400
#define TRAPF_NOLOADDS		0x800

#include <trap_asm.h>
