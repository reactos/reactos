/* $Id: startup.c,v 1.22 2000/03/22 18:35:51 dwelch Exp $
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
#include <ddk/ntddk.h>
#include <windows.h>
#include <ntdll/ldr.h>
#include <ntdll/rtl.h>
#include <csrss/csrss.h>
#include <ntdll/csr.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* GLOBALS *******************************************************************/

DLL LdrDllListHead;
extern unsigned int _image_base__;

ULONG NtGlobalFlag = 0;


/* FUNCTIONS *****************************************************************/

VOID LdrStartup(VOID)
{
   PEPFUNC EntryPoint;
   PIMAGE_DOS_HEADER PEDosHeader;
   NTSTATUS Status;
   PIMAGE_NT_HEADERS NTHeaders;
   PVOID ImageBase;
   PPEB Peb;
   
   DPRINT("LdrStartup()\n");

   LdrDllListHead.BaseAddress = (PVOID)&_image_base__;
   LdrDllListHead.Prev = &LdrDllListHead;
   LdrDllListHead.Next = &LdrDllListHead;
   LdrDllListHead.SectionHandle = NULL;
   PEDosHeader = (PIMAGE_DOS_HEADER)LdrDllListHead.BaseAddress;
   LdrDllListHead.Headers = (PIMAGE_NT_HEADERS)(LdrDllListHead.BaseAddress +
						PEDosHeader->e_lfanew);
   
   Peb = (PPEB)(PEB_BASE);
   DPRINT("Peb %x\n", Peb);
   ImageBase = Peb->ImageBaseAddress;
   DPRINT("ImageBase %x\n", ImageBase);
   if (ImageBase <= (PVOID)0x1000)
     {
	DPRINT("ImageBase is null\n");
	for(;;);
     }

   NtGlobalFlag = Peb->NtGlobalFlag;

   /*  If MZ header exists  */
   PEDosHeader = (PIMAGE_DOS_HEADER) ImageBase;
   DPRINT("PEDosHeader %x\n", PEDosHeader);
   if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC ||
       PEDosHeader->e_lfanew == 0L ||
       *(PULONG)((PUCHAR)ImageBase + PEDosHeader->e_lfanew) != IMAGE_PE_MAGIC)
     {
	DbgPrint("Image has bad header\n");
	ZwTerminateProcess(NULL, STATUS_UNSUCCESSFUL);
     }

   /* normalize process parameters */
   RtlNormalizeProcessParams (Peb->ProcessParameters);

   NTHeaders = (PIMAGE_NT_HEADERS)(ImageBase + PEDosHeader->e_lfanew);

   /* create process heap */
   Peb->ProcessHeap = RtlCreateHeap(0,
				    (PVOID)HEAP_BASE,
				    NTHeaders->OptionalHeader.SizeOfHeapCommit,
				    NTHeaders->OptionalHeader.SizeOfHeapReserve,
				    NULL,
				    NULL);
   if (Peb->ProcessHeap == 0)
     {
	DbgPrint("Failed to create process heap\n");
	ZwTerminateProcess(NtCurrentProcess(),STATUS_UNSUCCESSFUL);
     }

   EntryPoint = LdrPEStartup((PVOID)ImageBase, NULL);
   if (EntryPoint == NULL)
     {
	DbgPrint("Failed to initialize image\n");
	ZwTerminateProcess(NtCurrentProcess(),STATUS_UNSUCCESSFUL);
     }

   /*
    * Connect to the csrss server
    */
   Status = CsrClientConnectToServer();
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Failed to connect to csrss.exe: expect trouble\n");
//	ZwTerminateProcess(NtCurrentProcess(), Status);
     }
   
   DbgPrint("Transferring control to image at %x\n",EntryPoint);
   Status = EntryPoint(Peb);
   ZwTerminateProcess(NtCurrentProcess(),Status);
}

/* EOF */
