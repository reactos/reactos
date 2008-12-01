/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/find.c
 * PURPOSE:         Find functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32file);

/*
 * @implemented
 */
BOOL WINAPI
FindCloseChangeNotification (HANDLE hChangeHandle)
{
   NTSTATUS Status = NtClose(hChangeHandle);
   if(!NT_SUCCESS(Status))
   {
     SetLastErrorByStatus(Status);
     return FALSE;
   }

   return TRUE;
}


/*
 * @implemented
 */
HANDLE
WINAPI
FindFirstChangeNotificationA (
	LPCSTR	lpPathName,
	BOOL	bWatchSubtree,
	DWORD	dwNotifyFilter
	)
{
	PWCHAR PathNameW;

   if (!(PathNameW = FilenameA2W(lpPathName, FALSE)))
      return INVALID_HANDLE_VALUE;

   return FindFirstChangeNotificationW (PathNameW ,
	                                        bWatchSubtree,
	                                        dwNotifyFilter);
}


/*
 * @implemented
 */
HANDLE
WINAPI
FindFirstChangeNotificationW (
	LPCWSTR	lpPathName,
	BOOL	bWatchSubtree,
	DWORD	dwNotifyFilter
	)
{
   NTSTATUS Status;
   UNICODE_STRING NtPathU;
   IO_STATUS_BLOCK IoStatus;
   OBJECT_ATTRIBUTES ObjectAttributes;
   HANDLE hDir;

   if (!RtlDosPathNameToNtPathName_U (lpPathName,
                                          &NtPathU,
                                          NULL,
                                          NULL))
   {
      SetLastErrorByStatus(STATUS_OBJECT_PATH_SYNTAX_BAD);
      return INVALID_HANDLE_VALUE;
   }



   InitializeObjectAttributes (&ObjectAttributes,
                               &NtPathU,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

   Status = NtOpenFile (&hDir,
                        SYNCHRONIZE|FILE_LIST_DIRECTORY,
                        &ObjectAttributes,
                        &IoStatus,
                        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                        FILE_DIRECTORY_FILE);

   /*
   FIXME: I think we should use FILE_OPEN_FOR_BACKUP_INTENT. See M$ Q188321
   -Gunnar
   */

   RtlFreeHeap(RtlGetProcessHeap(),
               0,
               NtPathU.Buffer);



   if (!NT_SUCCESS(Status))
   {
      SetLastErrorByStatus(Status);
      return INVALID_HANDLE_VALUE;
   }

   Status = NtNotifyChangeDirectoryFile(hDir,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &IoStatus,
                                        NULL,//Buffer,
                                        0,//BufferLength,
                                        dwNotifyFilter,
                                        (BOOLEAN)bWatchSubtree);
   if (!NT_SUCCESS(Status))
   {
      SetLastErrorByStatus(Status);
      return INVALID_HANDLE_VALUE;
   }

   return hDir;
}


/*
 * @implemented
 */
BOOL
WINAPI
FindNextChangeNotification (
	HANDLE	hChangeHandle
	)
{
   IO_STATUS_BLOCK IoStatus;
   NTSTATUS Status;

   Status = NtNotifyChangeDirectoryFile(hChangeHandle,
      NULL,
      NULL,
      NULL,
      &IoStatus,
      NULL,//Buffer,
      0,//BufferLength,
      FILE_NOTIFY_CHANGE_SECURITY,//meaningless/ignored for subsequent calls, but must contain a valid flag
      0 //meaningless/ignored for subsequent calls
      );

   if (!NT_SUCCESS(Status))
   {
      SetLastErrorByStatus(Status);
      return FALSE;
   }

   return TRUE;
}


extern VOID
(WINAPI ApcRoutine)(PVOID ApcContext,
      struct _IO_STATUS_BLOCK* IoStatusBlock,
      ULONG Reserved);


/*
 * @implemented
 */
BOOL
WINAPI
ReadDirectoryChangesW(
    HANDLE hDirectory,
    LPVOID lpBuffer OPTIONAL,
    DWORD nBufferLength,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter,
    LPDWORD lpBytesReturned, /* undefined for asych. operations */
    LPOVERLAPPED lpOverlapped OPTIONAL,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine /* OPTIONAL???????? */
    )
{
   NTSTATUS Status;
   IO_STATUS_BLOCK IoStatus;

   if (lpOverlapped )
      lpOverlapped->Internal = STATUS_PENDING;

   Status = NtNotifyChangeDirectoryFile(
      hDirectory,
      lpOverlapped ? lpOverlapped->hEvent : NULL,
      lpCompletionRoutine ? ApcRoutine : NULL, /* ApcRoutine OPTIONAL???? */
      lpCompletionRoutine, /* ApcContext */
      lpOverlapped ? (PIO_STATUS_BLOCK)lpOverlapped : &IoStatus,
      lpBuffer,
      nBufferLength,
      dwNotifyFilter,
      (BOOLEAN)bWatchSubtree
      );

   if (!NT_SUCCESS(Status))
   {
      SetLastErrorByStatus(Status);
      return FALSE;
   }


   /* NOTE: lpBytesReturned is undefined for asynch. operations */
   *lpBytesReturned = IoStatus.Information;

   return TRUE;
}





/* EOF */
