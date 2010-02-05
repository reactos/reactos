_ONCE

#define DBGTRAP DPRINT1
// #define DBGTRAPENTRY DPRINT1("\n"); DbgDumpCpu(7|DBG_DUMPCPU_TSS); DPRINT1("TrapFrame=%p:\n", TrapFrame); DbgDumpMem(TrapFrame, 0x80)
#define DBGTRAPENTRY DbgDumpCpu(7); DPRINT1("TrapFrame=%p:\n", TrapFrame); DbgDumpMem(TrapFrame, sizeof(KTRAP_FRAME));

// TRAP_STUB_FLAGS TrapStub x-macro flags
// trap type
#define TRAPF_ERRORCODE		1
#define TRAPF_INTERRUPT		2
#define TRAPF_FASTSYSCALL	4
// options
#define TRAPF_NOSAVESEG		0x100
#define TRAPF_NOSAVEFS		0x200
#define TRAPF_SAVENOVOL		0x400
#define TRAPF_NOLOADDS		0x800

#include <trap_asm.h>

extern KINTERRUPT KiInterruptInitialData;

VOID _CDECL KiTrapInit(VOID);
VOID _CDECL KiInterrupt0(VOID);
VOID _CDECL KiInterrupt1(VOID);
VOID _CDECL KiTrap02(VOID);
VOID _CDECL KiTrap08(VOID);
VOID _CDECL KiTrap13(VOID);
VOID _CDECL KiFastCallEntry(VOID);

// temporary
VOID _NORETURN FASTCALL KiTrapReturn(IN PKTRAP_FRAME TrapFrame);
VOID KiExitTrapDebugChecks(IN PKTRAP_FRAME TrapFrame, IN KTRAP_EXIT_SKIP_BITS SkipBits);
VOID KiEnterTrap(IN PKTRAP_FRAME TrapFrame);
VOID KiExitTrap(IN PKTRAP_FRAME TrapFrame, IN UCHAR Skip);
