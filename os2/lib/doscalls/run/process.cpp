/* $Id: process.cpp,v 1.3 2002/05/30 15:11:47 robertk Exp $
*/
/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS OS/2 sub system
 * FILE:             dll/process.cpp
 * PURPOSE:          Kernelservices for OS/2 apps
 * PROGRAMMER:       Robert K. nonvolatil@yahoo.de
 * REVISION HISTORY:
 *    13-03-2002  Created
 */


#define INCL_DOSPROCESS
#include "../../../include/os2.h"
#include <ddk/ntddk.h>


APIRET STDCALL DosSleep(ULONG msec)
{
	NTSTATUS stat;
	TIME Interv;
	Interv.QuadPart= -(10000 * msec);
	stat = NtDelayExecution( TRUE, &Interv );
	return 0;
}


/* $Id: process.cpp,v 1.3 2002/05/30 15:11:47 robertk Exp $ */
/* Terminates the current thread or the current Process.
	Decission is made by action 
	FIXME:	move this code to OS2.EXE */
VOID APIENTRY DosExit(ULONG action, ULONG result)
{
	// decide what to do
	if( action == EXIT_THREAD)
	{
		NtTerminateThread( NULL, result );
	}
	else	// EXIT_PROCESS
	{
		NtTerminateProcess( NULL, result );
	}
}


APIRET STDCALL DosBeep(ULONG freq, ULONG dur)
{
	if( freq<0x25 || freq>0x7FFF )
		return 395;	// ERROR_INVALID_FREQUENCY

	HANDLE	hBeep;
	IO_STATUS_BLOCK ComplStatus;
	//UNICODE_STRING
	OBJECT_ATTRIBUTES oa = {sizeof oa, 0, {8,8,"\\\\.\\Beep"l}, OBJ_CASE_INSENSITIVE};
	NTSTATUS stat;
	stat = NtOpenFile( &hBeep,
				FILE_READ_DATA | FILE_WRITE_DATA,
				&oa,
				&ComplStatus,
				0,	// no sharing
				FILE_OPEN );
	
	if (!NT_SUCCESS(stat))
	{
	}

		   if( ComplStatus-> 
  /*  HANDLE hBeep;
    BEEP_SET_PARAMETERS BeepSetParameters;
    DWORD dwReturned;

    hBeep = Dos32Open("\\\\.\\Beep",
                       FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
    if (hBeep == INVALID_HANDLE_VALUE)
        return FALSE;
*/
    // Set beep data 
  /*  BeepSetParameters.Frequency = dwFreq;
    BeepSetParameters.Duration  = dwDuration;

    DeviceIoControl(hBeep,
                    IOCTL_BEEP_SET,
                    &BeepSetParameters,
                    sizeof(BEEP_SET_PARAMETERS),
                    NULL,
                    0,
                    &dwReturned,
                    NULL);

    CloseHandle (hBeep);

    return TRUE;
*/



	return 0;
}


APIRET STDCALL DosCreateThread(PTID ptid, PFNTHREAD pfn,
                                   ULONG param, ULONG flag, ULONG cbStack)
{
	return 0;
}

/* EOF */
