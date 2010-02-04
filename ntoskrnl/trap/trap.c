
// create stubs
_NOWARN_PUSH
_NOWARN_MSC(4005)

// 00 DE divide error
#define TRAP_STUB_NAME KiTrap00
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 01 DB reserved
#define TRAP_STUB_NAME KiTrap01
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 02 NMI
#define TRAP_STUB_NAME KiTrap02
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 03 BP brealpoint
#define TRAP_STUB_NAME KiTrap03
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 04 OF overflow
#define TRAP_STUB_NAME KiTrap04
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 05 BR bound range exceeded
#define TRAP_STUB_NAME KiTrap05
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 06 UD undefined opcode
#define TRAP_STUB_NAME KiTrap06
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 07 NM no math coprocessor
#define TRAP_STUB_NAME KiTrap07
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 08 DF double fault
#define TRAP_STUB_NAME KiTrap08
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

// 09 math segment overrun
#define TRAP_STUB_NAME KiTrap09
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 0A TS invalid TSS 
#define TRAP_STUB_NAME KiTrap0A
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

// 0B NP segment not present
#define TRAP_STUB_NAME KiTrap0B
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

// 0C SS stack segment fault 
#define TRAP_STUB_NAME KiTrap0C
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

// 0D GP general protection 
#define TRAP_STUB_NAME KiTrap0D
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

// 0E PF page fault
#define TRAP_STUB_NAME KiTrap0E
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

// 0F reserved
#define TRAP_STUB_NAME KiTrap0F
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 10 MF math fault
#define TRAP_STUB_NAME KiTrap10
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

// 11 AC alignment check
#define TRAP_STUB_NAME KiTrap11
#define TRAP_STUB_FLAGS TRAPF_ERRORCODE
#include <TrapStub.h>

#if 0
// 12 MC machine check
#define TRAP_STUB_NAME KiTrap12
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>
#endif

// 13 XM simd exception
#define TRAP_STUB_NAME KiTrap13
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

#define TRAP_STUB_NAME KiGetTickCount
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

#define TRAP_STUB_NAME KiCallbackReturn
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

#define TRAP_STUB_NAME KiRaiseAssertion
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

#define TRAP_STUB_NAME KiDebugService
#define TRAP_STUB_FLAGS 0
#include <TrapStub.h>

#define TRAP_STUB_NAME KiSystemService
#define TRAP_STUB_FLAGS TRAPF_NOSAVESEG
#include <TrapStub.h>

// 
#define TRAP_STUB_NAME KiFastCallEntry
#define TRAP_STUB_FLAGS TRAPF_FASTSYSCALL
#include <TrapStub.h>

#if 0
#define TRAP_STUB_NAME KiInterruptTemplate
#define TRAP_STUB_FLAGS TRAPF_VECTOR
#include <TrapStub.h>
#endif

_NOWARN_POP


