/* $Id: wait.c,v 1.28 2004/03/07 18:06:29 arty Exp $
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
#include "../include/debug.h"


/* FUNCTIONS ****************************************************************/

/*
 * Thread that waits for a console handle.  Console handles only fire when
 * they're readable.
 */

DWORD STDCALL WaitForConsoleHandleThread( PVOID ConHandle ) {
    DWORD AmtRead = 0;
    INPUT_RECORD Buffer[1];
    do {
	PeekConsoleInputA( ConHandle, Buffer, 1, &AmtRead );
	if( !AmtRead ) Sleep( 100 );
    } while( AmtRead == 0 );

    return 0;
}

/*
 * Return a waitable object given a console handle
 */
DWORD GetWaiterForConsoleHandle( HANDLE ConHandle, PHANDLE Waitable ) {
    DWORD ThreadId;
    HANDLE WaitableHandle = CreateThread( 0, 
					  0, 
					  WaitForConsoleHandleThread,
					  ConHandle,
					  0,
					  &ThreadId );

    *Waitable = WaitableHandle;

    return WaitableHandle ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
DWORD STDCALL
WaitForSingleObject(HANDLE hHandle,
		    DWORD dwMilliseconds)
{
   return WaitForSingleObjectEx(hHandle,
				dwMilliseconds,
				FALSE);
}


/*
 * @implemented
 */
DWORD STDCALL
WaitForSingleObjectEx(HANDLE hHandle,
                      DWORD  dwMilliseconds,
                      BOOL   bAlertable)
{
  PLARGE_INTEGER TimePtr;
  LARGE_INTEGER Time;
  NTSTATUS Status;
  BOOL CloseWaitHandle = FALSE;

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
	  Status = GetWaiterForConsoleHandle( hHandle, &hHandle );
	  if (!NT_SUCCESS(Status))
	    {
	      SetLastErrorByStatus (Status);
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
      Time.QuadPart = -10000 * (LONGLONG)dwMilliseconds;
      TimePtr = &Time;
    }

  Status = NtWaitForSingleObject(hHandle,
				 (BOOLEAN) bAlertable,
				 TimePtr);

  if (CloseWaitHandle)
    NtClose(hHandle);

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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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
  DWORD i,j;
  NTSTATUS Status;
  PBOOL FreeThisHandle;

  DPRINT("nCount %lu\n", nCount);

  HandleBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, nCount * (sizeof(HANDLE) + sizeof(BOOL)) );
  FreeThisHandle = (PBOOL)(&HandleBuffer[nCount]);

  if (HandleBuffer == NULL)
    {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return WAIT_FAILED;
    }

  for (i = 0; i < nCount; i++)
    {
      FreeThisHandle[i] = FALSE;
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
	      Status = GetWaiterForConsoleHandle( HandleBuffer[i], 
						  &HandleBuffer[i] );
	      if (!NT_SUCCESS(Status))
		{
		  /* We'll leak some handles unless we close the already
		     created handles */
		  for (j = 0; j < i; j++)
		    if (FreeThisHandle[j])
		      NtClose(HandleBuffer[j]);

		  SetLastErrorByStatus (Status);
		  RtlFreeHeap(GetProcessHeap(),0,HandleBuffer);
		  return FALSE;
		}
	      
	      FreeThisHandle[i] = TRUE;
	    }
	}
    }

  if (dwMilliseconds == INFINITE)
    {
      TimePtr = NULL;
    }
  else
    {
      Time.QuadPart = -10000 * (LONGLONG)dwMilliseconds;
      TimePtr = &Time;
    }

  Status = NtWaitForMultipleObjects (nCount,
				     HandleBuffer,
				     bWaitAll  ? WaitAll : WaitAny,
				     (BOOLEAN)bAlertable,
				     TimePtr);

  for (i = 0; i < nCount; i++)
    if (FreeThisHandle[i])
      NtClose(HandleBuffer[i]);

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


/*
 * @implemented
 */
DWORD STDCALL
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
      Time.QuadPart = -10000 * (LONGLONG)dwMilliseconds;
      TimePtr = &Time;
    }

  Status = NtSignalAndWaitForSingleObject (hObjectToSignal,
					   hObjectToWaitOn,
					   (BOOLEAN)bAlertable,
					   TimePtr);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus (Status);
      return FALSE;
    }

  return TRUE;
}

/* EOF */
