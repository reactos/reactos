#pragma once

extern ULONG gulDebugChannels;

enum _DEBUGCHANNELS
{
    DbgCustom = 1,
    DbgObjects = 2,
    DbgBitBlt = 4,
    DbgXlate = 8,
    DbgModeSwitch = 16,
};

#define DBGENABLE(ch) gulDebugChannels |= (ch);
#define DBGDISABLE(ch) gulDebugChannels &= ~(ch);
#define DPRINTCH(ch) if (gulDebugChannels & (ch)) DbgPrint

#ifdef GDI_DEBUG

#define KeRosDumpStackFrames(Frames, Count) KdSystemDebugControl('DsoR', (PVOID)Frames, Count, NULL, 0, NULL, KernelMode)
NTSYSAPI ULONG APIENTRY RtlWalkFrameChain(OUT PVOID *Callers, IN ULONG Count, IN ULONG Flags);

#define IS_HANDLE_VALID(idx) \
    ((GdiHandleTable->Entries[idx].Type & GDI_ENTRY_BASETYPE_MASK) != 0)

#define GDIDBG_TRACECALLER() \
  DPRINT1("-> called from:\n"); \
  KeRosDumpStackFrames(NULL, 20);
#define GDIDBG_TRACEALLOCATOR(handle) \
  DPRINT1("-> allocated from:\n"); \
  KeRosDumpStackFrames(GDIHandleAllocator[GDI_HANDLE_GET_INDEX(handle)], GDI_STACK_LEVELS);
#define GDIDBG_TRACELOCKER(handle) \
  DPRINT1("-> locked from:\n"); \
  KeRosDumpStackFrames(GDIHandleLocker[GDI_HANDLE_GET_INDEX(handle)], GDI_STACK_LEVELS);
#define GDIDBG_TRACESHARELOCKER(handle) \
  DPRINT1("-> locked from:\n"); \
  KeRosDumpStackFrames(GDIHandleShareLocker[GDI_HANDLE_GET_INDEX(handle)], GDI_STACK_LEVELS);
#define GDIDBG_TRACEDELETER(handle) \
  DPRINT1("-> deleted from:\n"); \
  KeRosDumpStackFrames(GDIHandleDeleter[GDI_HANDLE_GET_INDEX(handle)], GDI_STACK_LEVELS);
#define GDIDBG_CAPTUREALLOCATOR(handle) \
  CaptureStackBackTace((PVOID*)GDIHandleAllocator[GDI_HANDLE_GET_INDEX(handle)], GDI_STACK_LEVELS);
#define GDIDBG_CAPTURELOCKER(handle) \
  CaptureStackBackTace((PVOID*)GDIHandleLocker[GDI_HANDLE_GET_INDEX(handle)], GDI_STACK_LEVELS);
#define GDIDBG_CAPTURESHARELOCKER(handle) \
  CaptureStackBackTace((PVOID*)GDIHandleShareLocker[GDI_HANDLE_GET_INDEX(handle)], GDI_STACK_LEVELS);
#define GDIDBG_CAPTUREDELETER(handle) \
  CaptureStackBackTace((PVOID*)GDIHandleDeleter[GDI_HANDLE_GET_INDEX(handle)], GDI_STACK_LEVELS);
#define GDIDBG_DUMPHANDLETABLE() \
  IntDumpHandleTable(GdiHandleTable)
#define GDIDBG_INITLOOPTRACE() \
  ULONG Attempts = 0;
#define GDIDBG_TRACELOOP(Handle, PrevThread, Thread) \
  if ((++Attempts % 20) == 0) \
  { \
    DPRINT1("[%d] Handle 0x%p Locked by 0x%x (we're 0x%x)\n", Attempts, Handle, PrevThread, Thread); \
  }

#else

#define GDIDBG_TRACECALLER()
#define GDIDBG_TRACEALLOCATOR(index)
#define GDIDBG_TRACELOCKER(index)
#define GDIDBG_TRACESHARELOCKER(index)
#define GDIDBG_CAPTUREALLOCATOR(index)
#define GDIDBG_CAPTURELOCKER(index)
#define GDIDBG_CAPTURESHARELOCKER(index)
#define GDIDBG_CAPTUREDELETER(handle)
#define GDIDBG_DUMPHANDLETABLE()
#define GDIDBG_INITLOOPTRACE()
#define GDIDBG_TRACELOOP(Handle, PrevThread, Thread)
#define GDIDBG_TRACEDELETER(handle)

#endif /* GDI_DEBUG */

