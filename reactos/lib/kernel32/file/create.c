/* $Id: create.c,v 1.29 2002/09/08 10:22:41 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/create.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *		    GetTempFileName is modified from WINE [ Alexandre Juiliard ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  Removed use of SearchPath (not used by Windows)
 *                  18/08/2002: CreateFileW mess cleaned up (KJK::Hyperion)
 *                  24/08/2002: removed superfluous DPRINTs (KJK::Hyperion)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
/* please FIXME: ddk/ntddk.h should be enough */
#include <ddk/iodef.h>
#include <ntdll/rtl.h>
#include <windows.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>


/* FUNCTIONS ****************************************************************/

HANDLE STDCALL CreateFileA (LPCSTR			lpFileName,
			    DWORD			dwDesiredAccess,
			    DWORD			dwShareMode,
			    LPSECURITY_ATTRIBUTES	lpSecurityAttributes,
			    DWORD			dwCreationDisposition,
			    DWORD			dwFlagsAndAttributes,
			    HANDLE			hTemplateFile)
{
   UNICODE_STRING FileNameU;
   ANSI_STRING FileName;
   HANDLE FileHandle;
   
   DPRINT("CreateFileA(lpFileName %s)\n",lpFileName);
   
   RtlInitAnsiString (&FileName,
		      (LPSTR)lpFileName);
   
   /* convert ansi (or oem) string to unicode */
   if (bIsFileApiAnsi)
     RtlAnsiStringToUnicodeString (&FileNameU,
				   &FileName,
				   TRUE);
   else
     RtlOemStringToUnicodeString (&FileNameU,
				  &FileName,
				  TRUE);

   FileHandle = CreateFileW (FileNameU.Buffer,
			     dwDesiredAccess,
			     dwShareMode,
			     lpSecurityAttributes,
			     dwCreationDisposition,
			     dwFlagsAndAttributes,
			     hTemplateFile);
   
   RtlFreeHeap (RtlGetProcessHeap (),
		0,
		FileNameU.Buffer);
   
   return FileHandle;
}


HANDLE STDCALL CreateFileW (LPCWSTR			lpFileName,
			    DWORD			dwDesiredAccess,
			    DWORD			dwShareMode,
			    LPSECURITY_ATTRIBUTES	lpSecurityAttributes,
			    DWORD			dwCreationDisposition,
			    DWORD			dwFlagsAndAttributes,
			    HANDLE			hTemplateFile)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK IoStatusBlock;
   UNICODE_STRING NtPathU;
   HANDLE FileHandle;
   NTSTATUS Status;
   ULONG Flags = 0;

   DPRINT("CreateFileW(lpFileName %S)\n",lpFileName);

   if(hTemplateFile != NULL)
   {
    /* FIXME */
    DPRINT("Template file feature not supported yet\n");
    SetLastError(ERROR_NOT_SUPPORTED);
    return INVALID_HANDLE_VALUE;
   }

   /* validate & translate the creation disposition */
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
	dwCreationDisposition = FILE_OPEN_IF;
	break;

      case TRUNCATE_EXISTING:
	dwCreationDisposition = FILE_OVERWRITE;
        break;
      
      default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return (INVALID_HANDLE_VALUE);
     }

   /* validate & translate the flags */
   if (dwFlagsAndAttributes & FILE_FLAG_OVERLAPPED)
   {
    DPRINT("Overlapped I/O not supported\n");
    SetLastError(ERROR_NOT_SUPPORTED);
    return INVALID_HANDLE_VALUE;
   }
   else
     Flags |= FILE_SYNCHRONOUS_IO_ALERT;
   
   /* validate & translate the filename */
   if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFileName,
				      &NtPathU,
				      NULL,
				      NULL))
   {
     DPRINT("Invalid path\n");
     SetLastError(ERROR_BAD_PATHNAME);
     return INVALID_HANDLE_VALUE;
   }
   
   DPRINT("NtPathU \'%S\'\n", NtPathU.Buffer);

   /* translate the flags that need no validation */
   if(dwFlagsAndAttributes & FILE_FLAG_WRITE_THROUGH)
    Flags |= FILE_WRITE_THROUGH;

   if(dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)
    Flags |= FILE_NO_INTERMEDIATE_BUFFERING;

   if(dwFlagsAndAttributes & FILE_FLAG_RANDOM_ACCESS)
    Flags |= FILE_RANDOM_ACCESS;
   
   if(dwFlagsAndAttributes & FILE_FLAG_SEQUENTIAL_SCAN)
    Flags |= FILE_SEQUENTIAL_ONLY;
   
   if(dwFlagsAndAttributes & FILE_FLAG_DELETE_ON_CLOSE)
    Flags |= FILE_DELETE_ON_CLOSE;
   
   if(dwFlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS)
   {
    if(dwDesiredAccess & GENERIC_ALL)
      Flags |= FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_FOR_RECOVERY;
    else
    {
      if(dwDesiredAccess & GENERIC_READ)
        Flags |= FILE_OPEN_FOR_BACKUP_INTENT;
      
      if(dwDesiredAccess & GENERIC_WRITE)
        Flags |= FILE_OPEN_FOR_RECOVERY;
    }
   }
   else
    Flags |= FILE_NON_DIRECTORY_FILE;

   /* FILE_FLAG_POSIX_SEMANTICS is handled later */

#if 0
   /* FIXME: Win32 constants to be defined */
   if(dwFlagsAndAttributes & FILE_FLAG_OPEN_REPARSE_POINT)
    Flags |= FILE_OPEN_REPARSE_POINT;
   
   if(dwFlagsAndAttributes & FILE_FLAG_OPEN_NO_RECALL)
    Flags |= FILE_OPEN_NO_RECALL;
#endif

   /* translate the desired access */
   if (dwDesiredAccess & GENERIC_ALL)
     dwDesiredAccess |= FILE_ALL_ACCESS;
   else
   {
     if (dwDesiredAccess & GENERIC_READ)
       dwDesiredAccess |= FILE_GENERIC_READ;
     
     if (dwDesiredAccess & GENERIC_WRITE)
       dwDesiredAccess |= FILE_GENERIC_WRITE;
     
     if (dwDesiredAccess & GENERIC_EXECUTE)
       dwDesiredAccess |= FILE_GENERIC_EXECUTE;
   }

   /* build the object attributes */
   InitializeObjectAttributes(
    &ObjectAttributes,
    &NtPathU,
    0,
    NULL,
    NULL
   );

   if (lpSecurityAttributes)
   {
      if(lpSecurityAttributes->bInheritHandle)
         ObjectAttributes.Attributes |= OBJ_INHERIT;

      ObjectAttributes.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
   }
   
   if(!(dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS))
    ObjectAttributes.Attributes |= OBJ_CASE_INSENSITIVE;

   /* perform the call */
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

   RtlFreeUnicodeString(&NtPathU);

   /* error */
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus (Status);
	return INVALID_HANDLE_VALUE;
     }
   
   switch(IoStatusBlock.Information)
   {
    case FILE_OPENED:
    case FILE_CREATED:
     SetLastError(ERROR_ALREADY_EXISTS);
     break;
    
    default:
   }
   
   return FileHandle;
}

/* EOF */
