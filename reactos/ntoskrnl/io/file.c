/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/io/file.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS ZwQueryInformationFile(HANDLE FileHandle,
				PIO_STATUS_BLOCK IoStatusBlock,
				PVOID FileInformation,
				ULONG Length,
				FILE_INFORMATION_CLASS FileInformationClass)
{
   UNIMPLEMENTED;
}

NTSTATUS ZwSetInformationFile(HANDLE FileHandle,
			      PIO_STATUS_BLOCK IoStatusBlock,
			      PVOID FileInformation,
			      ULONG Length,
			      FILE_INFORMATION_CLASS FileInformationClass)
{
   UNIMPLEMENTED;
}

PGENERIC_MAPPING IoGetFileObjectGenericMapping()
{
   UNIMPLEMENTED;
}
