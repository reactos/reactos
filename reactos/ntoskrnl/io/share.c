/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: share.c,v 1.6 2002/07/24 17:49:31 ekohl Exp $
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
IoCheckShareAccess(IN ACCESS_MASK DesiredAccess,
		   IN ULONG DesiredShareAccess,
		   IN PFILE_OBJECT FileObject,
		   IN PSHARE_ACCESS ShareAccess,
		   IN BOOLEAN Update)
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
      return(STATUS_SUCCESS);
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
	return(STATUS_SHARING_VIOLATION);
    }

  if (WriteAccess)
    {
      if (ShareAccess->SharedWrite < ShareAccess->OpenCount)
	return(STATUS_SHARING_VIOLATION);
    }

  if (DeleteAccess)
    {
      if (ShareAccess->SharedDelete < ShareAccess->OpenCount)
	return(STATUS_SHARING_VIOLATION);
    }

  if (ShareAccess->Readers != 0)
    {
      if (SharedRead == FALSE)
	return(STATUS_SHARING_VIOLATION);
    }

  if (ShareAccess->Writers != 0)
    {
      if (SharedWrite == FALSE)
	return(STATUS_SHARING_VIOLATION);
    }

  if (ShareAccess->Deleters != 0)
    {
      if (SharedDelete == FALSE)
	return(STATUS_SHARING_VIOLATION);
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

  return(STATUS_SUCCESS);
}


VOID STDCALL
IoRemoveShareAccess(IN PFILE_OBJECT FileObject,
		    IN PSHARE_ACCESS ShareAccess)
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
IoSetShareAccess(IN ACCESS_MASK DesiredAccess,
		 IN ULONG DesiredShareAccess,
		 IN PFILE_OBJECT FileObject,
		 OUT PSHARE_ACCESS ShareAccess)
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


NTSTATUS STDCALL
IoCheckDesiredAccess(IN OUT PACCESS_MASK DesiredAccess,
		     IN ACCESS_MASK GrantedAccess)
{
  RtlMapGenericMask(DesiredAccess,
		    IoFileObjectType->Mapping);
  if ((*DesiredAccess & GrantedAccess) != GrantedAccess)
    return(STATUS_ACCESS_DENIED);

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
IoCheckEaBufferValidity(IN PFILE_FULL_EA_INFORMATION EaBuffer,
			IN ULONG EaLength,
			OUT PULONG ErrorOffset)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
IoCheckFunctionAccess(IN ACCESS_MASK GrantedAccess,
		      IN UCHAR MajorFunction,
		      IN UCHAR MinorFunction,
		      IN ULONG IoControlCode,
		      IN PFILE_INFORMATION_CLASS FileInformationClass OPTIONAL,
		      IN PFS_INFORMATION_CLASS FsInformationClass OPTIONAL)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
IoSetInformation(IN PFILE_OBJECT FileObject,
		 IN FILE_INFORMATION_CLASS FileInformationClass,
		 IN ULONG Length,
		 OUT PVOID FileInformation)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


BOOLEAN STDCALL
IoFastQueryNetworkAttributes(IN POBJECT_ATTRIBUTES ObjectAttributes,
			     IN ACCESS_MASK DesiredAccess,
			     IN ULONG OpenOptions,
			     OUT PIO_STATUS_BLOCK IoStatus,
			     OUT PFILE_NETWORK_OPEN_INFORMATION Buffer)
{
  UNIMPLEMENTED;
  return(FALSE);
}

/* EOF */
