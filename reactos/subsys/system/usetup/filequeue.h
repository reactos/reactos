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
/* $Id: filequeue.h,v 1.1 2002/11/23 01:55:27 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/filequeue.h
 * PURPOSE:         File queue functions
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __FILEQUEUE_H__
#define __FILEQUEUE_H__


#define SPFILENOTIFY_STARTQUEUE       0x1
#define SPFILENOTIFY_ENDQUEUE         0x2
#define SPFILENOTIFY_STARTSUBQUEUE    0x3
#define SPFILENOTIFY_ENDSUBQUEUE      0x4

#define SPFILENOTIFY_STARTCOPY        0xb
#define SPFILENOTIFY_ENDCOPY          0xc
#define SPFILENOTIFY_COPYERROR        0xd

#define FILEOP_COPY                   0x0
#define FILEOP_RENAME                 0x1
#define FILEOP_DELETE                 0x2
#define FILEOP_BACKUP                 0x3

#define FILEOP_ABORT                  0x0
#define FILEOP_DOIT                   0x1
#define FILEOP_SKIP                   0x2
#define FILEOP_RETRY                  FILEOP_DOIT
#define FILEOP_NEWPATH                0x4


/* TYPES ********************************************************************/

typedef PVOID HSPFILEQ;

typedef ULONG (*PSP_FILE_CALLBACK)(PVOID Context,
				   ULONG Notification,
				   PVOID Param1,
				   PVOID Param2);


/* FUNCTIONS ****************************************************************/

HSPFILEQ
SetupOpenFileQueue(VOID);

BOOL
SetupCloseFileQueue(HSPFILEQ QueueHandle);

BOOL
SetupQueueCopy(HSPFILEQ QueueHandle,
	       PCWSTR SourceRootPath,
	       PCWSTR SourcePath,
	       PCWSTR SourceFilename,
	       PCWSTR TargetDirectory,
	       PCWSTR TargetFilename);

BOOL
SetupCommitFileQueue(HSPFILEQ QueueHandle,
		     PCWSTR TargetRootPath,
		     PCWSTR TargetPath,
		     PSP_FILE_CALLBACK MsgHandler,
		     PVOID Context);

#endif /* __FILEQUEUE_H__ */

/* EOF */
