/* $Id: share.c,v 1.4 2001/08/14 21:05:10 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/share.c
 * PURPOSE:         
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID STDCALL
IoUpdateShareAccess(PFILE_OBJECT FileObject,
		    PSHARE_ACCESS ShareAccess)
{
   if ((FileObject->ReadAccess == FALSE) &&
       (FileObject->WriteAccess == FALSE) &&
       (FileObject->DeleteAccess == FALSE))
     {
	return;
     }

   ShareAccess->OpenCount++;

   if (FileObject->ReadAccess == TRUE)
     {
	ShareAccess->Readers++;
     }

   if (FileObject->WriteAccess == TRUE)
     {
	ShareAccess->Writers++;
     }

   if (FileObject->DeleteAccess == TRUE)
     {
	ShareAccess->Deleters++;
     }

   if (FileObject->SharedRead == TRUE)
     {
	ShareAccess->SharedRead++;
     }

   if (FileObject->SharedWrite == TRUE)
     {
	ShareAccess->SharedWrite++;
     }

   if (FileObject->SharedDelete == TRUE)
     {
	ShareAccess->SharedDelete++;
     }
}


NTSTATUS STDCALL
IoCheckShareAccess(ACCESS_MASK DesiredAccess,
		   ULONG DesiredShareAccess,
		   PFILE_OBJECT FileObject,
		   PSHARE_ACCESS ShareAccess,
		   BOOLEAN Update)
{
   BOOLEAN ReadAccess;
   BOOLEAN WriteAccess;
   BOOLEAN DeleteAccess;
   BOOLEAN SharedRead;
   BOOLEAN SharedWrite;
   BOOLEAN SharedDelete;

   ReadAccess = (DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE));
   WriteAccess = (DesiredAccess & (FILE_READ_DATA | FILE_APPEND_DATA));
   DeleteAccess = (DesiredAccess & DELETE);

   FileObject->ReadAccess = ReadAccess;
   FileObject->WriteAccess = WriteAccess;
   FileObject->DeleteAccess = DeleteAccess;

   if (!ReadAccess && !WriteAccess && !DeleteAccess)
     {
	return (STATUS_SUCCESS);
     }

   SharedRead = (DesiredShareAccess & FILE_SHARE_READ);
   SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE);
   SharedDelete = (DesiredShareAccess & FILE_SHARE_DELETE);

   FileObject->SharedRead = SharedRead;
   FileObject->SharedWrite = SharedWrite;
   FileObject->SharedDelete = SharedDelete;

   if (ReadAccess)
     {
	if (ShareAccess->SharedRead < ShareAccess->OpenCount)
	  return (STATUS_SHARING_VIOLATION);
     }

   if (WriteAccess)
     {
	if (ShareAccess->SharedWrite < ShareAccess->OpenCount)
	  return (STATUS_SHARING_VIOLATION);
     }

   if (DeleteAccess)
     {
	if (ShareAccess->SharedDelete < ShareAccess->OpenCount)
	  return (STATUS_SHARING_VIOLATION);
     }

   if (ShareAccess->Readers != 0)
     {
	if (SharedRead == FALSE)
	  return (STATUS_SHARING_VIOLATION);
     }

   if (ShareAccess->Writers != 0)
     {
	if (SharedWrite == FALSE)
	  return (STATUS_SHARING_VIOLATION);
     }

   if (ShareAccess->Deleters != 0)
     {
	if (SharedDelete == FALSE)
	  return (STATUS_SHARING_VIOLATION);
     }

   if (Update == TRUE)
     {
	ShareAccess->OpenCount++;

	if (ReadAccess == TRUE)
	  ShareAccess->Readers++;

	if (WriteAccess == TRUE)
	  ShareAccess->Writers++;

	if (DeleteAccess == TRUE)
	  ShareAccess->Deleters++;

	if (SharedRead == TRUE)
	  ShareAccess->SharedRead++;

	if (SharedWrite == TRUE)
	  ShareAccess->SharedWrite++;

	if (SharedDelete == TRUE)
	  ShareAccess->SharedDelete++;
     }

   return (STATUS_SUCCESS);
}


VOID STDCALL
IoRemoveShareAccess(PFILE_OBJECT FileObject,
		    PSHARE_ACCESS ShareAccess)
{
   if ((FileObject->ReadAccess == FALSE) &&
       (FileObject->WriteAccess == FALSE) &&
       (FileObject->DeleteAccess == FALSE))
     {
	return;
     }

   ShareAccess->OpenCount--;

   if (FileObject->ReadAccess == TRUE)
     {
	ShareAccess->Readers--;
     }

   if (FileObject->WriteAccess == TRUE)
     {
	ShareAccess->Writers--;
     }

   if (FileObject->DeleteAccess == TRUE)
     {
	ShareAccess->Deleters--;
     }

   if (FileObject->SharedRead == TRUE)
     {
	ShareAccess->SharedRead--;
     }

   if (FileObject->SharedWrite == TRUE)
     {
	ShareAccess->SharedWrite--;
     }

   if (FileObject->SharedDelete == TRUE)
     {
	ShareAccess->SharedDelete--;
     }
}


VOID STDCALL
IoSetShareAccess(ACCESS_MASK DesiredAccess,
		 ULONG DesiredShareAccess,
		 PFILE_OBJECT FileObject,
		 PSHARE_ACCESS ShareAccess)
{
   BOOLEAN ReadAccess;
   BOOLEAN WriteAccess;
   BOOLEAN DeleteAccess;
   BOOLEAN SharedRead;
   BOOLEAN SharedWrite;
   BOOLEAN SharedDelete;

   ReadAccess = (DesiredAccess & (FILE_READ_DATA | FILE_EXECUTE));
   WriteAccess = (DesiredAccess & (FILE_READ_DATA | FILE_APPEND_DATA));
   DeleteAccess = (DesiredAccess & DELETE);

   FileObject->ReadAccess = ReadAccess;
   FileObject->WriteAccess = WriteAccess;
   FileObject->DeleteAccess = DeleteAccess;

   if (!ReadAccess && !WriteAccess && !DeleteAccess)
     {
	FileObject->SharedRead = FALSE;
	FileObject->SharedWrite = FALSE;
	FileObject->SharedDelete = FALSE;

	ShareAccess->OpenCount = 0;
	ShareAccess->Readers = 0;
	ShareAccess->Writers = 0;
	ShareAccess->Deleters = 0;

	ShareAccess->SharedRead = 0;
	ShareAccess->SharedWrite = 0;
	ShareAccess->SharedDelete = 0;
     }
   else
     {
	SharedRead = (DesiredShareAccess & FILE_SHARE_READ);
	SharedWrite = (DesiredShareAccess & FILE_SHARE_WRITE);
	SharedDelete = (DesiredShareAccess & FILE_SHARE_DELETE);

	FileObject->SharedRead = SharedRead;
	FileObject->SharedWrite = SharedWrite;
	FileObject->SharedDelete = SharedDelete;

	ShareAccess->OpenCount = 1;
	ShareAccess->Readers = (ReadAccess) ? 1 : 0;
	ShareAccess->Writers = (WriteAccess) ? 1 : 0;
	ShareAccess->Deleters = (DeleteAccess) ? 1 : 0;

	ShareAccess->SharedRead = (SharedRead) ? 1 : 0;
	ShareAccess->SharedWrite = (SharedWrite) ? 1 : 0;
	ShareAccess->SharedDelete = (SharedDelete) ? 1 : 0;
     }
}


VOID STDCALL
IoCheckDesiredAccess(DWORD Unknown0,
		     DWORD Unknown1)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL
IoCheckEaBufferValidity(DWORD Unknown0,
			DWORD Unknown1,
			DWORD Unknown2)
{
   UNIMPLEMENTED;
   return (STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
IoCheckFunctionAccess(DWORD Unknown0,
		      DWORD Unknown1,
		      DWORD Unknown2,
		      DWORD Unknown3,
		      DWORD Unknown4,
		      DWORD Unknown5)
{
   UNIMPLEMENTED;
   return (STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
IoSetInformation (
	IN	PFILE_OBJECT		FileObject,
	IN	FILE_INFORMATION_CLASS	FileInformationClass,
	IN	ULONG			Length,
	OUT	PVOID			FileInformation
	)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}


VOID STDCALL
IoFastQueryNetworkAttributes (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	UNIMPLEMENTED;
}


/* EOF */
