/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: filequeue.c,v 1.1 2002/11/23 01:55:27 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/filequeue.c
 * PURPOSE:         File queue functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#include "usetup.h"
#include "filesup.h"
#include "filequeue.h"


/* INCLUDES *****************************************************************/


typedef struct _QUEUEENTRY
{
  struct _QUEUEENTRY *Prev;
  struct _QUEUEENTRY *Next;

  PWSTR SourceRootPath;
  PWSTR SourcePath;
  PWSTR SourceFilename;
  PWSTR TargetDirectory;
  PWSTR TargetFilename;

} QUEUEENTRY, *PQUEUEENTRY;


typedef struct _FILEQUEUEHEADER
{
  PQUEUEENTRY CopyHead;
  PQUEUEENTRY CopyTail;
  ULONG CopyCount;
} FILEQUEUEHEADER, *PFILEQUEUEHEADER;


/* FUNCTIONS ****************************************************************/

HSPFILEQ
SetupOpenFileQueue(VOID)
{
  PFILEQUEUEHEADER QueueHeader;

  /* Allocate queue header */
  QueueHeader = (PFILEQUEUEHEADER)RtlAllocateHeap(ProcessHeap,
						  0,
						  sizeof(FILEQUEUEHEADER));
  if (QueueHeader == NULL)
    return(NULL);

  /* Initialize queue header */
  RtlZeroMemory(QueueHeader,
		sizeof(FILEQUEUEHEADER));


  return((HSPFILEQ)QueueHeader);
}


BOOL
SetupCloseFileQueue(HSPFILEQ QueueHandle)
{
  PFILEQUEUEHEADER QueueHeader;
  PQUEUEENTRY Entry;

  if (QueueHandle == NULL)
    return(FALSE);

  QueueHeader = (PFILEQUEUEHEADER)QueueHandle;

  /* Delete copy queue */
  Entry = QueueHeader->CopyHead;
  while (Entry != NULL)
    {
      /* Delete all strings */
      if (Entry->SourceRootPath != NULL)
	RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);
      if (Entry->SourcePath != NULL)
	RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);
      if (Entry->SourceFilename != NULL)
	RtlFreeHeap(ProcessHeap, 0, Entry->SourceFilename);
      if (Entry->TargetDirectory != NULL)
	RtlFreeHeap(ProcessHeap, 0, Entry->TargetDirectory);
      if (Entry->TargetFilename != NULL)
	RtlFreeHeap(ProcessHeap, 0, Entry->TargetFilename);

      /* Unlink current queue entry */
      if (Entry->Next != NULL)
      {
	QueueHeader->CopyHead = Entry->Next;
	QueueHeader->CopyHead->Prev = NULL;
      }
      else
      {
	QueueHeader->CopyHead = NULL;
	QueueHeader->CopyTail = NULL;
      }

      /* Delete queue entry */
      RtlFreeHeap(ProcessHeap, 0, Entry);

      /* Get next queue entry */
      Entry = QueueHeader->CopyHead;
    }

  /* Delete queue header */
  RtlFreeHeap(ProcessHeap,
	      0,
	      QueueHeader);

  return(TRUE);
}


BOOL
SetupQueueCopy(HSPFILEQ QueueHandle,
	       PCWSTR SourceRootPath,
	       PCWSTR SourcePath,
	       PCWSTR SourceFilename,
	       PCWSTR TargetDirectory,
	       PCWSTR TargetFilename)
{
  PFILEQUEUEHEADER QueueHeader;
  PQUEUEENTRY Entry;
  ULONG Length;

  if (QueueHandle == NULL ||
      SourceRootPath == NULL ||
      SourceFilename == NULL ||
      TargetDirectory == NULL)
    return(FALSE);

  QueueHeader = (PFILEQUEUEHEADER)QueueHandle;

  /* Allocate new queue entry */
  Entry = (PQUEUEENTRY)RtlAllocateHeap(ProcessHeap,
				       0,
				       sizeof(QUEUEENTRY));
  if (Entry == NULL)
    return(FALSE);

  RtlZeroMemory(Entry,
		sizeof(QUEUEENTRY));

  /* Copy source root path */
  Length = wcslen(SourceRootPath);
  Entry->SourceRootPath = RtlAllocateHeap(ProcessHeap,
					  0,
					  (Length + 1) * sizeof(WCHAR));
  if (Entry->SourceRootPath == NULL)
  {
    RtlFreeHeap(ProcessHeap, 0, Entry);
    return(FALSE);
  }
  wcsncpy(Entry->SourceRootPath, SourceRootPath, Length);
  Entry->SourceRootPath[Length] = (WCHAR)0;

  /* Copy source path */
  if (SourcePath != NULL)
  {
    Length = wcslen(SourcePath);
    Entry->SourcePath = RtlAllocateHeap(ProcessHeap,
					0,
					(Length + 1) * sizeof(WCHAR));
    if (Entry->SourcePath == NULL)
    {
      RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);
      RtlFreeHeap(ProcessHeap, 0, Entry);
      return(FALSE);
    }
    wcsncpy(Entry->SourcePath, SourcePath, Length);
    Entry->SourcePath[Length] = (WCHAR)0;
  }

  /* Copy source file name */
  Length = wcslen(SourceFilename);
  Entry->SourceFilename = RtlAllocateHeap(ProcessHeap,
					  0,
					  (Length + 1) * sizeof(WCHAR));
  if (Entry->SourceFilename == NULL)
  {
    RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);
    RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);
    RtlFreeHeap(ProcessHeap, 0, Entry);
    return(FALSE);
  }
  wcsncpy(Entry->SourceFilename, SourceFilename, Length);
  Entry->SourceFilename[Length] = (WCHAR)0;

  /* Copy target directory */
  Length = wcslen(TargetDirectory);
  if (TargetDirectory[Length] == '\\')
    Length--;
  Entry->TargetDirectory = RtlAllocateHeap(ProcessHeap,
					   0,
					   (Length + 1) * sizeof(WCHAR));
  if (Entry->TargetDirectory == NULL)
  {
    RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);
    RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);
    RtlFreeHeap(ProcessHeap, 0, Entry->SourceFilename);
    RtlFreeHeap(ProcessHeap, 0, Entry);
    return(FALSE);
  }
  wcsncpy(Entry->TargetDirectory, TargetDirectory, Length);
  Entry->TargetDirectory[Length] = (WCHAR)0;

  /* Copy optional target filename */
  if (TargetFilename != NULL)
  {
    Length = wcslen(TargetFilename);
    Entry->TargetFilename = RtlAllocateHeap(ProcessHeap,
					    0,
					    (Length + 1) * sizeof(WCHAR));
    if (Entry->TargetFilename == NULL)
    {
      RtlFreeHeap(ProcessHeap, 0, Entry->SourceRootPath);
      RtlFreeHeap(ProcessHeap, 0, Entry->SourcePath);
      RtlFreeHeap(ProcessHeap, 0, Entry->SourceFilename);
      RtlFreeHeap(ProcessHeap, 0, Entry->TargetDirectory);
      RtlFreeHeap(ProcessHeap, 0, Entry);
      return(FALSE);
    }
    wcsncpy(Entry->TargetFilename, TargetFilename, Length);
    Entry->TargetFilename[Length] = (WCHAR)0;
  }

  /* Append queue entry */
  if (QueueHeader->CopyHead == NULL) // && QueueHeader->CopyTail == NULL)
  {
    Entry->Prev = NULL;
    Entry->Next = NULL;
    QueueHeader->CopyHead = Entry;
    QueueHeader->CopyTail = Entry;
  }
  else
  {
    Entry->Prev = QueueHeader->CopyTail;
    Entry->Next = NULL;
    QueueHeader->CopyTail->Next = Entry;
    QueueHeader->CopyTail = Entry;
  }
  QueueHeader->CopyCount++;

  return(TRUE);
}


BOOL
SetupCommitFileQueue(HSPFILEQ QueueHandle,
		     PCWSTR TargetRootPath,
		     PCWSTR TargetPath,
		     PSP_FILE_CALLBACK MsgHandler,
		     PVOID Context)
{
  PFILEQUEUEHEADER QueueHeader;
  PQUEUEENTRY Entry;
  NTSTATUS Status;

  WCHAR FileSrcPath[MAX_PATH];
  WCHAR FileDstPath[MAX_PATH];

  if (QueueHandle == NULL)
    return(FALSE);

  QueueHeader = (PFILEQUEUEHEADER)QueueHandle;

  MsgHandler(Context,
	     SPFILENOTIFY_STARTQUEUE,
	     NULL,
	     NULL);

  MsgHandler(Context,
	     SPFILENOTIFY_STARTSUBQUEUE,
	     (PVOID)FILEOP_COPY,
	     (PVOID)QueueHeader->CopyCount);

  /* Commit copy queue */
  Entry = QueueHeader->CopyHead;
  while (Entry != NULL)
  {
    /* Build the full source path */
    wcscpy(FileSrcPath, Entry->SourceRootPath);
    if (Entry->SourcePath != NULL)
      wcscat(FileSrcPath, Entry->SourcePath);
    wcscat(FileSrcPath, L"\\");
    wcscat(FileSrcPath, Entry->SourceFilename);

    /* Build the full target path */
    wcscpy(FileDstPath, TargetRootPath);
    if (Entry->TargetDirectory[0] == L'\\')
    {
      wcscat(FileDstPath, Entry->TargetDirectory);
    }
    else
    {
      if (TargetPath != NULL)
      {
	if (TargetPath[0] != L'\\')
	  wcscat(FileDstPath, L"\\");
	wcscat(FileDstPath, TargetPath);
      }
      wcscat(FileDstPath, L"\\");
      wcscat(FileDstPath, Entry->TargetDirectory);
    }
    wcscat(FileDstPath, L"\\");
    if (Entry->TargetFilename != NULL)
      wcscat(FileDstPath, Entry->TargetFilename);
    else
      wcscat(FileDstPath, Entry->SourceFilename);

    /* FIXME: Do it! */
    DPRINT("'%S' ==> '%S'\n",
	   FileSrcPath,
	   FileDstPath);

    MsgHandler(Context,
	       SPFILENOTIFY_STARTCOPY,
	       (PVOID)Entry->SourceFilename,
	       (PVOID)FILEOP_COPY);

    /* Copy the file */
    Status = SetupCopyFile(FileSrcPath, FileDstPath);
    if (!NT_SUCCESS(Status))
    {
      MsgHandler(Context,
		 SPFILENOTIFY_COPYERROR,
		 (PVOID)Entry->SourceFilename,
		 (PVOID)FILEOP_COPY);

    }
    else
    {
      MsgHandler(Context,
		 SPFILENOTIFY_ENDCOPY,
		 (PVOID)Entry->SourceFilename,
		 (PVOID)FILEOP_COPY);
    }

    Entry = Entry->Next;
  }

  MsgHandler(Context,
	     SPFILENOTIFY_ENDSUBQUEUE,
	     (PVOID)FILEOP_COPY,
	     NULL);

  MsgHandler(Context,
	     SPFILENOTIFY_ENDQUEUE,
	     NULL,
	     NULL);

  return(TRUE);
}

/* EOF */
