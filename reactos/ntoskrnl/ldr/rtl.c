/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loader utilities
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/i386/segment.h>
#include <internal/linkage.h>
#include <internal/module.h>
#include <internal/ntoskrnl.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <string.h>
#include <internal/string.h>
#include <internal/symbol.h>
#include <internal/teb.h>
#include <internal/ldr.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

PIMAGE_NT_HEADERS RtlImageNtHeader(PVOID BaseAddress)
{
   PIMAGE_DOS_HEADER DosHeader;
   PIMAGE_NT_HEADERS NTHeaders;
   
   DPRINT("BaseAddress %x\n", BaseAddress);
   DosHeader = (PIMAGE_DOS_HEADER)BaseAddress;
   DPRINT("DosHeader %x\n", DosHeader);
   NTHeaders = (PIMAGE_NT_HEADERS)(BaseAddress + DosHeader->e_lfanew);
   DPRINT("NTHeaders %x\n", NTHeaders);
   if ((DosHeader->e_magic != IMAGE_DOS_MAGIC)
       || (DosHeader->e_lfanew == 0L)
       || (*(PULONG) NTHeaders != IMAGE_PE_MAGIC))
     {
	return(NULL);
     }
   return(NTHeaders);
}
