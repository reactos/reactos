/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/startup.c
 * PURPOSE:         Process startup for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/* INCLUDES *****************************************************************/

#define WIN32_NO_PEHDR
#include <windows.h>
#include <ddk/ntddk.h>
#include <pe.h>
#include <string.h>
#include <wchar.h>
#include <ntdll/ldr.h>
#include <ntdll/rtl.h>

//#define NDEBUG
#include <ntdll/ntdll.h>

/* GLOBALS *******************************************************************/

DLL LdrDllListHead;
extern unsigned int _image_base__;

/* FUNCTIONS *****************************************************************/

/*   LdrStartup
 * FUNCTION:
 *   Handles Process Startup Activities.
 * ARGUMENTS:
 *   DWORD    ImageBase  The base address of the process image
 */
VOID LdrStartup(HANDLE SectionHandle, DWORD ImageBase)
{
   PEPFUNC EntryPoint;
   PIMAGE_DOS_HEADER PEDosHeader;
   NTSTATUS Status;
   PIMAGE_NT_HEADERS NTHeaders;
   
   DPRINT("LdrStartup(ImageBase %x, SectionHandle %x)\n",ImageBase,
	   SectionHandle);
   
   DPRINT("&_image_base__ %x\n",&_image_base__);
   LdrDllListHead.BaseAddress = (PVOID)&_image_base__;
   LdrDllListHead.Prev = &LdrDllListHead;
   LdrDllListHead.Next = &LdrDllListHead;
   LdrDllListHead.SectionHandle = SectionHandle;
   PEDosHeader = (PIMAGE_DOS_HEADER)LdrDllListHead.BaseAddress;
   LdrDllListHead.Headers = (PIMAGE_NT_HEADERS)(LdrDllListHead.BaseAddress +
						PEDosHeader->e_lfanew);
   
  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ImageBase;
  if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC ||
      PEDosHeader->e_lfanew == 0L ||
      *(PULONG)((PUCHAR)ImageBase + PEDosHeader->e_lfanew) != IMAGE_PE_MAGIC)
     {
	DPRINT("Image has bad header\n");
	ZwTerminateProcess(NULL,STATUS_UNSUCCESSFUL);
     }

   NTHeaders = (PIMAGE_NT_HEADERS)(ImageBase + PEDosHeader->e_lfanew);
   __RtlInitHeap((PVOID)HEAP_BASE,
		 NTHeaders->OptionalHeader.SizeOfHeapCommit, 
		 NTHeaders->OptionalHeader.SizeOfHeapReserve);
   EntryPoint = LdrPEStartup((PVOID)ImageBase, SectionHandle);

   if (EntryPoint == NULL)
     {
	DPRINT("Failed to initialize image\n");
	ZwTerminateProcess(NULL,STATUS_UNSUCCESSFUL);
     }
   
   DPRINT("Transferring control to image at %x\n",EntryPoint);
   Status = EntryPoint();
   ZwTerminateProcess(NtCurrentProcess(),Status);
}
