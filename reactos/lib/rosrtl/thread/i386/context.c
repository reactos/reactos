/* $Id$
*/
/*
*/

#include <string.h>

#define NTOS_MODE_USER
#include <ntos.h>

#include <napi/i386/segment.h>
#include <napi/i386/floatsave.h>

#include <rosrtl/thread.h>

NTSTATUS NTAPI
RtlRosInitializeContext
(
 IN HANDLE ProcessHandle,
 OUT PCONTEXT Context,
 IN PVOID StartAddress,
 IN PINITIAL_TEB InitialTeb,
 IN ULONG ParameterCount,
 IN ULONG_PTR * Parameters
)
{
 static PVOID s_pRetAddr = (PVOID)0xDEADBEEF;

 ULONG nDummy;
 SIZE_T nParamsSize = ParameterCount * sizeof(ULONG_PTR);
 NTSTATUS nErrCode;
 PVOID pStackBase;
 PVOID pStackLimit;

 /* Intel x86: linear top-down stack, all parameters passed on the stack */
 /* get the stack base and limit */
 nErrCode = RtlpRosGetStackLimits(InitialTeb, &pStackBase, &pStackLimit);

 /* failure */
 if(!NT_SUCCESS(nErrCode)) return nErrCode;

 /* validate the stack */
 nErrCode = RtlpRosValidateTopDownUserStack(pStackBase, pStackLimit);

 /* failure */
 if(!NT_SUCCESS(nErrCode)) return nErrCode;

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
  &s_pRetAddr,
  sizeof(s_pRetAddr),
  &nDummy
 );
}

/* EOF */
