#include <windows.h>
#include <GdiPlusPrivate.h>

#include "regtests.h"

BOOL
ReturnTrue()
{
  return TRUE;
}

static BOOL MyRtlFreeHeapCalled = FALSE;

VOID STDCALL
MyRtlFreeHeap(ULONG a1, ULONG a2, ULONG a3)
{
  MyRtlFreeHeapCalled = TRUE;
}

extern VOID STDCALL
RtlFreeHeap(ULONG a1, ULONG a2, ULONG a3);

HOOK Hooks[] =
{
  {"RtlFreeHeap", MyRtlFreeHeap}
};

static int
RunTest(char *Buffer)
{
  _SetHooks(Hooks);
  RtlFreeHeap(0,0,0);
  FAIL_IF_FALSE(MyRtlFreeHeapCalled, "RtlFreeHeap() must be called.");

  FAIL_IF_FALSE(ReturnTrue(), "ReturnTrue() must always return TRUE.");
  return TS_OK;
}

DISPATCHER(Test_1Test, "Test 1")
