/* $ ld $
*/
/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS OS/2 sub system
 * FILE:             dll/doscalls.c
 * PURPOSE:          Kernelservices for OS/2 apps
 * PROGRAMMER:       Robert K. robertk@mok.lvcm.com
 * REVISION HISTORY:
 *    13-03-2002  Created
 */

#include <ddk/ntddk.h>
#include "doscalls.h"


/*  Process variables. This section conains
	per process variables that are used for caching or
	other stuff. */
DWORD PROC_Pid;			// contains the current processes pid. (or is it also in PEB)



/* Implementation of the system calls */
APIRET APIENTRY Dos32Sleep(ULONG msec)
{
	NTSTATUS stat;
	TIME Interv;
	Interv.QuadPart= -(10000 * msec);
	stat = NtDelayExecution( TRUE, &Interv );
	return 0;
}

APIRET APIENTRY Dos32CreateThread(PTID ptid, PFNTHREAD pfn,
                                   ULONG param, ULONG flag, ULONG cbStack)
{
	return 0;
}


APIRET APIENTRY  Dos32Open(PSZ    pszFileName,  PHFILE pHf,
                            PULONG pulAction,  ULONG  cbFile,
                            ULONG  ulAttribute,  ULONG  fsOpenFlags,
                            ULONG  fsOpenMode,  PVOID reserved )  //ULONGPEAOP2 peaop2)
{
/*	NTAPI
ZwCreateFile(
OUT PHANDLE FileHandle,
IN ACCESS_MASK DesiredAccess,
IN POBJECT_ATTRIBUTES ObjectAttributes,
OUT PIO_STATUS_BLOCK IoStatusBlock,
IN PLARGE_INTEGER AllocationSize OPTIONAL,
IN ULONG FileAttributes,
IN ULONG ShareAccess,
IN ULONG CreateDisposition,
IN ULONG CreateOptions,
IN PVOID EaBuffer OPTIONAL,
IN ULONG EaLength
);*/
	return 0;
}


/* close a Handle. seems finished */
APIRET APIENTRY  Dos32Close(HFILE hFile)
{
	NTSTATUS   nErrCode;
	nErrCode = NtClose( (HANDLE)hFile );
	switch( nErrCode )
	{
	case STATUS_SUCCESS:
		return NO_ERROR;
	case STATUS_INVALID_HANDLE:
		return ERROR_INVALID_HANDLE;
	case STATUS_HANDLE_NOT_CLOSABLE:
		return ERROR_FILE_NOT_FOUND;
	}
	return nErrCode;
}




APIRET APIENTRY  Dos32Read(HFILE hFile, PVOID pBuffer,
                            ULONG cbRead, PULONG pcbActual)
{
	NTSTATUS        nErrCode;
	IO_STATUS_BLOCK isbStatus;
	// read data from file 
	nErrCode = NtReadFile( (HANDLE)hFile,	NULL,	NULL,	NULL,
							&isbStatus,		pBuffer,	cbRead,
							NULL,		NULL	);
	// contains the # bytes actually read.
	*pcbActual = isbStatus.Information;
	switch(nErrCode)
	{
	case STATUS_INVALID_HANDLE:
		return ERROR_INVALID_HANDLE;
		// FIXME: complete this
	}
	return NO_ERROR;
}

/* Generic write to a stream given by hFile */
APIRET APIENTRY  Dos32Write(HFILE hFile, PVOID pBuffer,
                             ULONG cbWrite, PULONG pcbActual)
{ 
	NTSTATUS        nErrCode;
	IO_STATUS_BLOCK StatusBlk;
	nErrCode = NtWriteFile( (HANDLE)hFile, NULL, NULL, NULL,
							&StatusBlk, pBuffer, cbWrite, 0, NULL );
	*pcbActual = StatusBlk.Information;
	// do an errorcode translation   FIXME: correct
	switch(nErrCode)
	{
	case STATUS_SUCCESS:
	case STATUS_PENDING:
	case STATUS_ACCESS_DENIED:
	case STATUS_INVALID_HANDLE:
	case STATUS_FILE_LOCK_CONFLICT:
		return 0;
	}
	return 0;
}
   


/*******************************************/
/* DosDevIOCtl performs control functions  */
/* on a device specified by an opened      */
/* device handle.                          */
/*******************************************/
/*HFILE     hDevice;         Device handle returned by DosOpen, or a standard (open) device handle. */
/*ULONG     category;        Device category. */
/*ULONG     function;        Device-specific function code. */
/*PVOID     pParams;         Address of the command-specific argument list. */
/*ULONG     cbParmLenMax;    Length, in bytes, of pParams. */
/*PULONG    pcbParmLen;      Pointer to the length of parameters. */
/*PVOID     pData;           Address of the data area. */
/*ULONG     cbDataLenMax;    Length, in bytes, of pData. */
/*PULONG    pcbDataLen;      Pointer to the length of data. */
/*APIRET    ulrc;            Return Code. 
 
 ulrc (APIRET) - returns 
    Return Code. 

    DosDevIOCtl returns one of the following values: 

      0         NO_ERROR 
      1         ERROR_INVALID_FUNCTION 
      6         ERROR_INVALID_HANDLE 
      15        ERROR_INVALID_DRIVE 
      31        ERROR_GEN_FAILURE 
      87        ERROR_INVALID_PARAMETER 
      111       ERROR_BUFFER_OVERFLOW 
      115       ERROR_PROTECTION_VIOLATION 
      117       ERROR_INVALID_CATEGORY 
      119       ERROR_BAD_DRIVER_LEVEL 
      163       ERROR_UNCERTAIN_MEDIA 
      165       ERROR_MONITORS_NOT_SUPPORTED 
 
*/
APIRET APIENTRY Dos32DevIOCtl(HFILE hDevice, ULONG category, ULONG function,
        PVOID pParams,ULONG cbParmLenMax,PULONG pcbParmLen,
        PVOID pData,ULONG cbDataLenMax,PULONG pcbDataLen)
{
	return 0;
}


 
APIRET APIENTRY Dos32Beep(ULONG freq, ULONG dur)
{
	if( freq<0x25 || freq>0x7FFF )
		return 395;	// ERROR_INVALID_FREQUENCY

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

/* Terminates the current thread or the current Process.
	Decission is made by action 
	FIXME:	move this code to OS2.EXE */
VOID APIENTRY Dos32Exit(ULONG action, ULONG result)
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



BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
	case 1://DLL_PROCESS_ATTACH:
	case 2://DLL_THREAD_ATTACH:
	case 3://DLL_THREAD_DETACH:
	case 0://DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}

