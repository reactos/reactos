
#pragma once

#include <psdk/winnt.h>

#define CONTEXT_i386 0x10000
#define CONTEXT_i486 0x10000

#define CONTEXT_I386_CONTROL   (CONTEXT_i386 | 0x0001) /* SS:SP, CS:IP, FLAGS, BP */
#define CONTEXT_I386_INTEGER   (CONTEXT_i386 | 0x0002) /* AX, BX, CX, DX, SI, DI */
#define CONTEXT_I386_SEGMENTS  (CONTEXT_i386 | 0x0004) /* DS, ES, FS, GS */
#define CONTEXT_I386_FLOATING_POINT  (CONTEXT_i386 | 0x0008) /* 387 state */
#define CONTEXT_I386_DEBUG_REGISTERS (CONTEXT_i386 | 0x0010) /* DB 0-3,6,7 */
#define CONTEXT_I386_EXTENDED_REGISTERS (CONTEXT_i386 | 0x0020)
#define CONTEXT_I386_XSTATE             (CONTEXT_i386 | 0x0040)
#define CONTEXT_I386_FULL (CONTEXT_I386_CONTROL | CONTEXT_I386_INTEGER | CONTEXT_I386_SEGMENTS)
#define CONTEXT_I386_ALL (CONTEXT_I386_FULL | CONTEXT_I386_FLOATING_POINT | CONTEXT_I386_DEBUG_REGISTERS | CONTEXT_I386_EXTENDED_REGISTERS)

typedef WOW64_FLOATING_SAVE_AREA I386_FLOATING_SAVE_AREA;
#ifdef _M_IX86
typedef CONTEXT I386_CONTEXT;
#else
typedef WOW64_CONTEXT I386_CONTEXT;
#endif

#define CONTEXT_AMD64 0x100000

#define CONTEXT_AMD64_CONTROL   (CONTEXT_AMD64 | 0x0001)
#define CONTEXT_AMD64_INTEGER   (CONTEXT_AMD64 | 0x0002)
#define CONTEXT_AMD64_SEGMENTS  (CONTEXT_AMD64 | 0x0004)
#define CONTEXT_AMD64_FLOATING_POINT  (CONTEXT_AMD64 | 0x0008)
#define CONTEXT_AMD64_DEBUG_REGISTERS (CONTEXT_AMD64 | 0x0010)
#define CONTEXT_AMD64_XSTATE          (CONTEXT_AMD64 | 0x0040)
#define CONTEXT_AMD64_FULL (CONTEXT_AMD64_CONTROL | CONTEXT_AMD64_INTEGER | CONTEXT_AMD64_FLOATING_POINT)
#define CONTEXT_AMD64_ALL (CONTEXT_AMD64_CONTROL | CONTEXT_AMD64_INTEGER | CONTEXT_AMD64_SEGMENTS | CONTEXT_AMD64_FLOATING_POINT | CONTEXT_AMD64_DEBUG_REGISTERS)

typedef struct DECLSPEC_ALIGN(16) _AMD64_CONTEXT {
    DWORD64 P1Home;          /* 000 */
    DWORD64 P2Home;          /* 008 */
    DWORD64 P3Home;          /* 010 */
    DWORD64 P4Home;          /* 018 */
    DWORD64 P5Home;          /* 020 */
    DWORD64 P6Home;          /* 028 */

    /* Control flags */
    DWORD ContextFlags;      /* 030 */
    DWORD MxCsr;             /* 034 */

    /* Segment */
    WORD SegCs;              /* 038 */
    WORD SegDs;              /* 03a */
    WORD SegEs;              /* 03c */
    WORD SegFs;              /* 03e */
    WORD SegGs;              /* 040 */
    WORD SegSs;              /* 042 */
    DWORD EFlags;            /* 044 */

    /* Debug */
    DWORD64 Dr0;             /* 048 */
    DWORD64 Dr1;             /* 050 */
    DWORD64 Dr2;             /* 058 */
    DWORD64 Dr3;             /* 060 */
    DWORD64 Dr6;             /* 068 */
    DWORD64 Dr7;             /* 070 */

    /* Integer */
    DWORD64 Rax;             /* 078 */
    DWORD64 Rcx;             /* 080 */
    DWORD64 Rdx;             /* 088 */
    DWORD64 Rbx;             /* 090 */
    DWORD64 Rsp;             /* 098 */
    DWORD64 Rbp;             /* 0a0 */
    DWORD64 Rsi;             /* 0a8 */
    DWORD64 Rdi;             /* 0b0 */
    DWORD64 R8;              /* 0b8 */
    DWORD64 R9;              /* 0c0 */
    DWORD64 R10;             /* 0c8 */
    DWORD64 R11;             /* 0d0 */
    DWORD64 R12;             /* 0d8 */
    DWORD64 R13;             /* 0e0 */
    DWORD64 R14;             /* 0e8 */
    DWORD64 R15;             /* 0f0 */

    /* Counter */
    DWORD64 Rip;             /* 0f8 */

    /* Floating point */
    union {
        XMM_SAVE_AREA32 FltSave;  /* 100 */
        struct {
            M128A Header[2];      /* 100 */
            M128A Legacy[8];      /* 120 */
            M128A Xmm0;           /* 1a0 */
            M128A Xmm1;           /* 1b0 */
            M128A Xmm2;           /* 1c0 */
            M128A Xmm3;           /* 1d0 */
            M128A Xmm4;           /* 1e0 */
            M128A Xmm5;           /* 1f0 */
            M128A Xmm6;           /* 200 */
            M128A Xmm7;           /* 210 */
            M128A Xmm8;           /* 220 */
            M128A Xmm9;           /* 230 */
            M128A Xmm10;          /* 240 */
            M128A Xmm11;          /* 250 */
            M128A Xmm12;          /* 260 */
            M128A Xmm13;          /* 270 */
            M128A Xmm14;          /* 280 */
            M128A Xmm15;          /* 290 */
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

    /* Vector */
    M128A VectorRegister[26];     /* 300 */
    DWORD64 VectorControl;        /* 4a0 */

    /* Debug control */
    DWORD64 DebugControl;         /* 4a8 */
    DWORD64 LastBranchToRip;      /* 4b0 */
    DWORD64 LastBranchFromRip;    /* 4b8 */
    DWORD64 LastExceptionToRip;   /* 4c0 */
    DWORD64 LastExceptionFromRip; /* 4c8 */
} AMD64_CONTEXT;

typedef struct _YMMCONTEXT
{
    M128A Ymm0;
    M128A Ymm1;
    M128A Ymm2;
    M128A Ymm3;
    M128A Ymm4;
    M128A Ymm5;
    M128A Ymm6;
    M128A Ymm7;
    M128A Ymm8;
    M128A Ymm9;
    M128A Ymm10;
    M128A Ymm11;
    M128A Ymm12;
    M128A Ymm13;
    M128A Ymm14;
    M128A Ymm15;
}
YMMCONTEXT, *PYMMCONTEXT;

typedef struct _XSTATE
{
    ULONG64 Mask;
    ULONG64 CompactionMask;
    ULONG64 Reserved[6];
    YMMCONTEXT YmmContext;
} XSTATE, *PXSTATE;

#define CONTEXT_UNWOUND_TO_CALL     0x20000000

#define CONTEXT_EXCEPTION_ACTIVE 0x8000000
#define CONTEXT_SERVICE_ACTIVE 0x10000000
#define CONTEXT_EXCEPTION_REQUEST 0x40000000
#define CONTEXT_EXCEPTION_REPORTING 0x80000000

// HACK!
#ifdef __ROS_LONG64__
#define ReadAcquire(x) ReadAcquire((volatile long*)(x))
#define WriteRelease(x, y) WriteRelease((volatile long*)(x), (y))
#endif
