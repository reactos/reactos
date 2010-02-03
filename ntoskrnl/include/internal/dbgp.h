#pragma once
#include <cpu.h>

// private
extern ULONG (_CDECL *DbgPrintf)(PCSTR fmt, ...);
extern ULONG (_CDECL *DbgPrintfv)(PCSTR fmt, va_list ap);
extern ULONG (NTAPI *DbgPrintExwp)
	(IN PCCH Prefix, IN ULONG ComponentId, IN ULONG Level, IN PCCH Format, IN va_list ap, IN BOOLEAN HandleBreakpoint);

void _CDECL DbgDumpMem(void *p, int cnt);		// dump memory
void _CDECL DbgDumpCpu(int flags);				// dump cpu
#define DBG_DUMPCPU_GP CPUREGSAVE_GP			// general purpose regs
#define DBG_DUMPCPU_SEG CPUREGSAVE_SEG			// segments
#define DBG_DUMPCPU_C CPUREGSAVE_C				// control
#define DBG_DUMPCPU_IDT 0x1000
#define DBG_DUMPCPU_GDT	0x2000
#define DBG_DUMPCPU_LDT 0x4000
#define DBG_DUMPCPU_TSS 0x8000

ULONG _CDECL DbgPrintfSer1(PCSTR fmt, ...);
ULONG _CDECL DbgPrintfvSer1(PCSTR fmt, va_list ap);
ULONG NTAPI DbgPrintExwpSer1(IN PCCH Prefix, IN ULONG ComponentId, IN ULONG Level, IN PCCH Format, IN va_list ap, IN BOOLEAN HandleBreakpoint);

#define RtlpSetInDbgPrint() FALSE
#define RtlpClearInDbgPrint()

extern PULONG KdComponentTable[104];
extern ULONG KdComponentTableSize;
extern ULONG Kd_WIN2000_Mask;

// exported by ntoskrnl.exe, declared in wdm.h
NTSYSAPI ULONG NTAPI vDbgPrintExWithPrefix(PCCH Prefix, ULONG ComponentId, ULONG Level, PCCH Format, va_list arglist);
NTSYSAPI ULONG NTAPI vDbgPrintEx(ULONG ComponentId, ULONG Level, PCCH Format, va_list arglist);
ULONG _CDECL DbgPrint(PCSTR Format, ...);
NTSYSAPI ULONG _CDECL DbgPrintEx(ULONG ComponentId, ULONG Level, PCSTR Format, ...);
NTSYSAPI ULONG _CDECL DbgPrintReturnControlC(PCCH Format, ...);
NTSYSAPI NTSTATUS NTAPI DbgQueryDebugFilterState(ULONG ComponentId, ULONG Level);
NTSYSAPI NTSTATUS NTAPI DbgSetDebugFilterState(ULONG ComponentId, ULONG Level, BOOLEAN State);

#if 0
DbgBreakPoint
DbgBreakPointWithStatus
DbgLoadImageSymbols
DbgPrompt
DbgQueryDebugFilterState
DbgSetDebugFilterState
DbgCommandString			// ros only
#endif

