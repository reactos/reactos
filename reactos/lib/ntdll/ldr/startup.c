/* $Id: startup.c,v 1.16 2000/01/27 08:56:48 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/startup.c
 * PURPOSE:         Process startup for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/* INCLUDES *****************************************************************/

#include <reactos/config.h>
#define WIN32_NO_STATUS
#define WIN32_NO_PEHDR
#include <windows.h>
#include <ddk/ntddk.h>
#include <pe.h>
#include <string.h>
#include <wchar.h>
#include <ntdll/ldr.h>
#include <ntdll/rtl.h>

//#define DBG_NTDLL_LDR_STARTUP
#ifndef DBG_NTDLL_LDR_STARTUP
#define NDEBUG
#endif
#include <ntdll/ntdll.h>

/* GLOBALS *******************************************************************/

DLL LdrDllListHead;
extern unsigned int _image_base__;
extern HANDLE __ProcessHeap;


/* FUNCTIONS *****************************************************************/


/*   LdrStartup
 * FUNCTION:
 *   Handles Process Startup Activities.
 * ARGUMENTS:
 *   DWORD    ImageBase  The base address of the process image
 */
VOID LdrStartup(PPEB Peb,
		HANDLE SectionHandle,
		DWORD ImageBase,
		HANDLE NTDllSectionHandle)
{
   PEPFUNC EntryPoint;
   PIMAGE_DOS_HEADER PEDosHeader;
   NTSTATUS Status;
   PIMAGE_NT_HEADERS NTHeaders;

   DPRINT("LdrStartup(Peb %x SectionHandle %x, ImageBase %x, "
	   "NTDllSectionHandle %x )\n",
	   Peb, SectionHandle, ImageBase, NTDllSectionHandle);

   LdrDllListHead.BaseAddress = (PVOID)&_image_base__;
   LdrDllListHead.Prev = &LdrDllListHead;
   LdrDllListHead.Next = &LdrDllListHead;
   LdrDllListHead.SectionHandle = NTDllSectionHandle;
   PEDosHeader = (PIMAGE_DOS_HEADER)LdrDllListHead.BaseAddress;
   LdrDllListHead.Headers = (PIMAGE_NT_HEADERS)(LdrDllListHead.BaseAddress +
						PEDosHeader->e_lfanew);

  /*  If MZ header exists  */
  PEDosHeader = (PIMAGE_DOS_HEADER) ImageBase;
   DPRINT("PEDosHeader %x\n", PEDosHeader);
  if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC ||
      PEDosHeader->e_lfanew == 0L ||
      *(PULONG)((PUCHAR)ImageBase + PEDosHeader->e_lfanew) != IMAGE_PE_MAGIC)
     {
	DbgPrint("Image has bad header\n");
	ZwTerminateProcess(NULL,
			   STATUS_UNSUCCESSFUL);
     }

   NTHeaders = (PIMAGE_NT_HEADERS)(ImageBase + PEDosHeader->e_lfanew);
   __ProcessHeap = RtlCreateHeap(0,
				 (PVOID)HEAP_BASE,
				 NTHeaders->OptionalHeader.SizeOfHeapCommit,
				 NTHeaders->OptionalHeader.SizeOfHeapReserve,
				 NULL,
				 NULL);
   EntryPoint = LdrPEStartup((PVOID)ImageBase, SectionHandle);

   if (EntryPoint == NULL)
     {
	DbgPrint("Failed to initialize image\n");
	ZwTerminateProcess(NtCurrentProcess(),STATUS_UNSUCCESSFUL);
     }
   
   /*
    * 
    */
   Status = CsrConnectToServer();
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Failed to connect to csrss.exe: expect trouble\n");
     }
   
//   DbgPrint("Transferring control to image at %x\n",EntryPoint);
   Status = EntryPoint(Peb);
   ZwTerminateProcess(NtCurrentProcess(),Status);
}

/* EOF */
