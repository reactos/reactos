/* $Id: context.c,v 1.3 2003/06/01 14:59:02 chorns Exp $
*/
/*
*/

#define NTOS_MODE_USER
#include <ntos.h>

#if defined(_M_IX86)
#include <napi/i386/segment.h>
#include <napi/i386/floatsave.h>
#else
#error Unsupported architecture
#endif

NTSTATUS NTAPI RtlRosInitializeContextEx
(
 IN HANDLE ProcessHandle,
 OUT PCONTEXT Context,
 IN PVOID StartAddress,
 IN PUSER_STACK UserStack,
 IN ULONG ParameterCount,
 IN ULONG_PTR * Parameters
)
{
 ULONG nDummy;
 PCHAR pStackBase;
 PCHAR pStackLimit;
 ULONG_PTR nRetAddr = 0xDEADBEEF;
 SIZE_T nParamsSize = ParameterCount * sizeof(ULONG_PTR);
 NTSTATUS nErrCode;

 /* fixed-size stack */
 if(UserStack->FixedStackBase && UserStack->FixedStackLimit)
 {
  pStackBase = UserStack->FixedStackBase;
  pStackLimit = UserStack->FixedStackLimit;
 }
 /* expandable stack */
 else if(UserStack->ExpandableStackBase && UserStack->ExpandableStackLimit)
 {
  pStackBase = UserStack->ExpandableStackBase;
  pStackLimit = UserStack->ExpandableStackLimit;
 }
 /* bad initial stack */
 else
  return STATUS_BAD_INITIAL_STACK;

 /* stack base lower than the limit */
 if(pStackBase <= pStackLimit)
  return STATUS_BAD_INITIAL_STACK;

#if defined(_M_IX86)
 /* Intel x86: all parameters passed on the stack */
 /* too many parameters */
 if((nParamsSize + sizeof(ULONG_PTR)) > (SIZE_T)(pStackBase - pStackLimit))
  return STATUS_STACK_OVERFLOW;

 memset(Context, 0, sizeof(CONTEXT));

 /* initialize the context */
 Context->ContextFlags = CONTEXT_FULL;
 Context->FloatSave.ControlWord = FLOAT_SAVE_CONTROL;
 Context->FloatSave.StatusWord = FLOAT_SAVE_STATUS;
 Context->FloatSave.TagWord = FLOAT_SAVE_TAG;
 Context->FloatSave.DataSelector = FLOAT_SAVE_DATA;
 Context->Eip = (ULONG_PTR)StartAddress;
 Context->SegGs = USER_DS;
 Context->SegFs = TEB_SELECTOR;
 Context->SegEs = USER_DS;
 Context->SegDs = USER_DS;
 Context->SegCs = USER_CS;
 Context->SegSs = USER_DS;
 Context->Esp = (ULONG_PTR)pStackBase - (nParamsSize + sizeof(ULONG_PTR));
 Context->EFlags = ((ULONG_PTR)1 << 1) | ((ULONG_PTR)1 << 9);

 /* write the parameters */
 nErrCode = NtWriteVirtualMemory
 (
  ProcessHandle,
  ((PUCHAR)pStackBase) - nParamsSize,
  Parameters,
  nParamsSize,
  &nDummy
 );

 /* failure */
 if(!NT_SUCCESS(nErrCode)) return nErrCode;

 /* write the return address */
 return NtWriteVirtualMemory
 (
  ProcessHandle,
  ((PUCHAR)pStackBase) - (nParamsSize + sizeof(ULONG_PTR)),
  &nRetAddr,
  sizeof(ULONG_PTR),
  &nDummy
 );

#else
#error Unsupported architecture
#endif
}

/* EOF */
