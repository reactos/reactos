/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loaders for PE executables
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>
#include "pe.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS LdrProcessImage(HANDLE SectionHandle, PVOID BaseAddress)
{
   PIMAGE_DOS_HEADER dos_hdr = (PIMAGE_DOS_HEADER)BaseAddress;
   PIMAGE_NT_HEADERS hdr = (PIMAGE_NT_HEADERS)(BaseAddress
					       + dos_hdr->e_lfanew);
   PIMAGE_SECTION_HEADER sections = (PIMAGE_SECTION_HEADER)(BaseAddress 
					    + dos_hdr->e_lfanew
       					    + sizeof(IMAGE_NT_HEADERS));
   
   
   
}

NTSTATUS LdrLoadDriver(PUNICODE_STRING FileName)
/*
 * FUNCTION: Loads a PE executable into the kernel
 * ARGUMENTS:
 *         FileName = Driver to load
 * RETURNS: Status
 */
{
}

NTSTATUS LdrLoadImage(PUNICODE_STRING FileName)
/*
 * FUNCTION: Loads a PE executable into the current process
 * ARGUMENTS:
 *        FileName = File to load
 * RETURNS: Status
 */
{
}
