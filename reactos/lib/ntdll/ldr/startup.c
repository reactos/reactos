/* $Id: startup.c,v 1.15 2000/01/18 12:04:31 ekohl Exp $
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

#undef DBG_NTDLL_LDR_STARTUP
#ifndef DBG_NTDLL_LDR_STARTUP
#define NDEBUG
#endif
#include <ntdll/ntdll.h>

/* GLOBALS *******************************************************************/

DLL LdrDllListHead;
extern unsigned int _image_base__;
extern HANDLE __ProcessHeap;


/* FUNCTIONS *****************************************************************/

NTSTATUS LdrMapNTDllForProcess (HANDLE	ProcessHandle,
				PHANDLE	PtrNTDllSectionHandle)
{
   ULONG InitialViewSize;
   NTSTATUS Status;
   HANDLE NTDllSectionHandle;
   PVOID ImageBase;
   PIMAGE_NT_HEADERS NTHeaders;
   PIMAGE_DOS_HEADER PEDosHeader;

   DPRINT("LdrMapNTDllForProcess(ProcessHandle %x)\n",ProcessHandle);

   PEDosHeader = (PIMAGE_DOS_HEADER)LdrDllListHead.BaseAddress;
   NTHeaders = (PIMAGE_NT_HEADERS)(LdrDllListHead.BaseAddress +
				   PEDosHeader->e_lfanew);

   NTDllSectionHandle = LdrDllListHead.SectionHandle;
   InitialViewSize = PEDosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS) 
     + sizeof(IMAGE_SECTION_HEADER) * NTHeaders->FileHeader.NumberOfSections;
   ImageBase = LdrDllListHead.BaseAddress;
   DPRINT("Mapping at %x\n",ImageBase);
   Status = ZwMapViewOfSection(NTDllSectionHandle,
			       ProcessHandle,
			       (PVOID *)&ImageBase,
			       0,
			       InitialViewSize,
			       NULL,
			       &InitialViewSize,
			       0,
			       MEM_COMMIT,
			       PAGE_READWRITE);
   LdrMapSections(ProcessHandle,
		  ImageBase,
		  NTDllSectionHandle,
		  NTHeaders);
   *PtrNTDllSectionHandle = NTDllSectionHandle;
   return(STATUS_SUCCESS);
}

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
  if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC ||
      PEDosHeader->e_lfanew == 0L ||
      *(PULONG)((PUCHAR)ImageBase + PEDosHeader->e_lfanew) != IMAGE_PE_MAGIC)
     {
	DbgPrint("Image has bad header\n");
	ZwTerminateProcess(NULL,STATUS_UNSUCCESSFUL);
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
