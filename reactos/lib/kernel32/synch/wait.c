/* $Id: wait.c,v 1.21 2003/03/06 13:01:15 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/synch/wait.c
 * PURPOSE:         Wait functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* FUNCTIONS ****************************************************************/

DWORD STDCALL
WaitForSingleObject(HANDLE hHandle,
		    DWORD dwMilliseconds)
{
   return WaitForSingleObjectEx(hHandle,
				dwMilliseconds,
				FALSE);
}


DWORD STDCALL
WaitForSingleObjectEx(HANDLE hHandle,
                      DWORD  dwMilliseconds,
                      BOOL   bAlertable)
{
  PLARGE_INTEGER TimePtr;
  LARGE_INTEGER Time;
  NTSTATUS Status;

  /* Get real handle */
  switch ((ULONG)hHandle)
    {
      case STD_INPUT_HANDLE:
	hHandle = NtCurrentPeb()->ProcessParameters->hStdInput;
	break;

      case STD_OUTPUT_HANDLE:
	hHandle = NtCurrentPeb()->ProcessParameters->hStdOutput;
	break;

      case STD_ERROR_HANDLE:
	hHandle = NtCurrentPeb()->ProcessParameters->hStdError;
	break;
    }

  /* Check for console handle */
  if (IsConsoleHandle(hHandle))
    {
      if (VerifyConsoleIoHandle(hHandle))
	{
	  DPRINT1("Console handles are not supported yet!\n");
	  SetLastError(ERROR_INVALID_HANDLE);
	  return WAIT_FAILED;
	}
    }

  if (dwMilliseconds == INFINITE)
    {
      TimePtr = NULL;
    }
  else
    {
      Time.QuadPart = -10000 * dwMilliseconds;
      TimePtr = &Time;
    }

  Status = NtWaitForSingleObject(hHandle,
				 (BOOLEAN) bAlertable,
				 TimePtr);
  if (Status == STATUS_TIMEOUT)
    {
      return WAIT_TIMEOUT;
    }
  else if ((Status == WAIT_OBJECT_0) ||
	   (Status == WAIT_ABANDONED_0))
    {
      return Status;
    }

  SetLastErrorByStatus (Status);

  return WAIT_FAILED;
}


DWORD STDCALL
WaitForMultipleObjects(DWORD nCount,
		       CONST HANDLE *lpHandles,
		       BOOL  bWaitAll,
		       DWORD dwMilliseconds)
{
  return WaitForMultipleObjectsEx(nCount,
				  lpHandles,
				  bWaitAll,
				  dwMilliseconds,
				  FALSE);
}


DWORD STDCALL
WaitForMultipleObjectsEx(DWORD nCount,
                         CONST HANDLE *lpHandles,
                         BOOL  bWaitAll,
                         DWORD dwMilliseconds,
                         BOOL  bAlertable)
{
  PLARGE_INTEGER TimePtr;
  LARGE_INTEGER Time;
  PHANDLE HandleBuffer;
  DWORD i;
  NTSTATUS Status;

  DPRINT("nCount %lu\n", nCount);

  HandleBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, nCount * sizeof(HANDLE));
  if (HandleBuffer == NULL)
    {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return WAIT_FAILED;
    }

  for (i = 0; i < nCount; i++)
    {
      switch ((DWORD)lpHandles[i])
	{
	  case STD_INPUT_HANDLE:
	    HandleBuffer[i] = NtCurrentPeb()->ProcessParameters->hStdInput;
	    break;

	  case STD_OUTPUT_HANDLE:
	    HandleBuffer[i] = NtCurrentPeb()->ProcessParameters->hStdOutput;
	    break;

	  case STD_ERROR_HANDLE:
	    HandleBuffer[i] = NtCurrentPeb()->ProcessParameters->hStdError;
	    break;

	  default:
	    HandleBuffer[i] = lpHandles[i];
	    break;
	}

      /* Check for console handle */
      if (IsConsoleHandle(HandleBuffer[i]))
	{
	  if (VerifyConsoleIoHandle(HandleBuffer[i]))
	    {
	      DPRINT1("Console handles are not supported yet!\n");
	      RtlFreeHeap(RtlGetProcessHeap(), 0, HandleBuffer);
	      SetLastError(ERROR_INVALID_HANDLE);
	      return FALSE;
	    }
	}
    }

  if (dwMilliseconds == INFINITE)
    {
      TimePtr = NULL;
    }
  else
    {
      Time.QuadPart = -10000 * dwMilliseconds;
      TimePtr = &Time;
    }

  Status = NtWaitForMultipleObjects (nCount,
				     HandleBuffer,
				     bWaitAll  ? WaitAll : WaitAny,
				     (BOOLEAN)bAlertable,
				     TimePtr);

  RtlFreeHeap(RtlGetProcessHeap(), 0, HandleBuffer);

  if (Status == STATUS_TIMEOUT)
    {
      return WAIT_TIMEOUT;
    }
  else if (((Status >= WAIT_OBJECT_0) &&
	    (Status <= WAIT_OBJECT_0 + nCount - 1)) ||
	   ((Status >= WAIT_ABANDONED_0) &&
	    (Status <= WAIT_ABANDONED_0 + nCount - 1)))
    {
      return Status;
    }

  DPRINT("Status %lx\n", Status);
  SetLastErrorByStatus (Status);

  return WAIT_FAILED;
}


BOOL STDCALL
SignalObjectAndWait(HANDLE hObjectToSignal,
		    HANDLE hObjectToWaitOn,
		    DWORD dwMilliseconds,
		    BOOL bAlertable)
{
  PLARGE_INTEGER TimePtr;
  LARGE_INTEGER Time;
  NTSTATUS Status;

  /* Get real handle */
  switch ((ULONG)hObjectToWaitOn)
    {
      case STD_INPUT_HANDLE:
	hObjectToWaitOn = NtCurrentPeb()->ProcessParameters->hStdInput;
	break;

      case STD_OUTPUT_HANDLE:
	hObjectToWaitOn = NtCurrentPeb()->ProcessParameters->hStdOutput;
	break;

      case STD_ERROR_HANDLE:
	hObjectToWaitOn = NtCurrentPeb()->ProcessParameters->hStdError;
	break;
    }

  /* Check for console handle */
  if (IsConsoleHandle(hObjectToWaitOn))
    {
      if (VerifyConsoleIoHandle(hObjectToWaitOn))
	{
	  DPRINT1("Console handles are not supported yet!\n");
	  SetLastError(ERROR_INVALID_HANDLE);
	  return FALSE;
	}
    }

  if (dwMilliseconds == INFINITE)
    {
      TimePtr = NULL;
    }
  else
    {
      Time.QuadPart = -10000 * dwMilliseconds;
      TimePtr = &Time;
    }

  Status = NtSignalAndWaitForSingleObject (hObjectToSignal,
					   hObjectToWaitOn,
					   TimePtr,
					   (BOOLEAN)bAlertable);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  return TRUE;
}

/* EOF */
