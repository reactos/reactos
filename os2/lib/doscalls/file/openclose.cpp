/* $Id: openclose.cpp,v 1.1 2002/07/23 13:00:11 robertk Exp $
*/
/*
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS OS/2 sub system
 * FILE:             dll/doscalls.c
 * PURPOSE:          Kernelservices for OS/2 apps
 * PROGRAMMER:       Robert K. nonvolatil@yahoo.de
 * REVISION HISTORY:
 *    13-03-2002  Created
 */


#define INCL_DOSFILEMGR
#include "../../../include/os2.h"
#include <ddk/ntddk.h>



APIRET STDCALL  Dos32Open(PSZ    pszFileName,  PHFILE pHf,
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



	   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK IoStatusBlock;
   UNICODE_STRING NtPathU;
   HANDLE FileHandle;
   NTSTATUS Status;
   ULONG Flags = 0;

   switch (dwCreationDisposition)
     {
      case CREATE_NEW:
	dwCreationDisposition = FILE_CREATE;
	break;
	
      case CREATE_ALWAYS:
	dwCreationDisposition = FILE_OVERWRITE_IF;
	break;
	
      case OPEN_EXISTING:
	dwCreationDisposition = FILE_OPEN;
	break;
	
      case OPEN_ALWAYS:
	dwCreationDisposition = OPEN_ALWAYS;
	break;

      case TRUNCATE_EXISTING:
	dwCreationDisposition = FILE_OVERWRITE;
     }
   
   DPRINT("CreateFileW(lpFileName %S)\n",lpFileName);
   
   if (dwDesiredAccess & GENERIC_READ)
     dwDesiredAccess |= FILE_GENERIC_READ;
   
   if (dwDesiredAccess & GENERIC_WRITE)
     dwDesiredAccess |= FILE_GENERIC_WRITE;
   
   if (!(dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED))
     {
	Flags |= FILE_SYNCHRONOUS_IO_ALERT;
     }
   
   if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFileName,
				      &NtPathU,
				      NULL,
				      NULL))
     return INVALID_HANDLE_VALUE;
   
   DPRINT("NtPathU \'%S\'\n", NtPathU.Buffer);
   
   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = NULL;
   ObjectAttributes.ObjectName = &NtPathU;
   ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;
   
   Status = NtCreateFile (&FileHandle,
			  dwDesiredAccess,
			  &ObjectAttributes,
			  &IoStatusBlock,
			  NULL,
			  dwFlagsAndAttributes,
			  dwShareMode,
			  dwCreationDisposition,
			  Flags,
			  NULL,
			  0);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus (Status);
	return INVALID_HANDLE_VALUE;
     }
   
   return FileHandle;

	return 0;
}


/* close a Handle. seems finished */
APIRET STDCALL  Dos32Close(HFILE hFile)
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




APIRET STDCALL  Dos32Read(HFILE hFile, PVOID pBuffer,
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
APIRET STDCALL  Dos32Write(HFILE hFile, PVOID pBuffer,
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
   


