/* $Id: startup.c,v 1.32 2000/10/08 16:32:52 dwelch Exp $
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
#include <napi/shared_data.h>

#define NDEBUG
#include <ntdll/ntdll.h>


VOID RtlInitializeHeapManager (VOID);

/* GLOBALS *******************************************************************/


extern unsigned int _image_base__;

static CRITICAL_SECTION PebLock;
static CRITICAL_SECTION LoaderLock;

ULONG NtGlobalFlag = 0;


/* FUNCTIONS *****************************************************************/

VOID STDCALL
LdrInitializeThunk (ULONG Unknown1,
                    ULONG Unknown2,
                    ULONG Unknown3,
                    ULONG Unknown4)
{
   PIMAGE_NT_HEADERS NTHeaders;
   PEPFUNC EntryPoint;
   PIMAGE_DOS_HEADER PEDosHeader;
   NTSTATUS Status;
   PVOID ImageBase;
   PPEB Peb;
   PLDR_MODULE NtModule;  // ntdll
   PLDR_MODULE ExeModule; // executable
   PKUSER_SHARED_DATA SharedUserData = 
	(PKUSER_SHARED_DATA)USER_SHARED_DATA_BASE;
   WCHAR FullNtDllPath[MAX_PATH];

   DPRINT("LdrInitializeThunk()\n");

   Peb = (PPEB)(PEB_BASE);
   DPRINT("Peb %x\n", Peb);
   ImageBase = Peb->ImageBaseAddress;
   DPRINT("ImageBase %x\n", ImageBase);
   if (ImageBase <= (PVOID)0x1000)
     {
	DPRINT("ImageBase is null\n");
	ZwTerminateProcess(NtCurrentProcess(), STATUS_UNSUCCESSFUL);
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
	ZwTerminateProcess(NtCurrentProcess(), STATUS_UNSUCCESSFUL);
     }

   /* normalize process parameters */
   RtlNormalizeProcessParams (Peb->ProcessParameters);

#if 0
   /* initialize NLS data */
   RtlInitNlsTables (Peb->AnsiCodePageData,
                     Peb->OemCodePageData,
                     Peb->UnicodeCaseTableData,
                     &TranslationTable);
   RtlResetRtlTranslations (&TranslationTable);
#endif

   NTHeaders = (PIMAGE_NT_HEADERS)(ImageBase + PEDosHeader->e_lfanew);

   /* create process heap */
   RtlInitializeHeapManager();
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

   /* initalize peb lock support */
   RtlInitializeCriticalSection (&PebLock);
   Peb->FastPebLock = &PebLock;
   Peb->FastPebLockRoutine = (PPEBLOCKROUTINE)RtlEnterCriticalSection;
   Peb->FastPebUnlockRoutine = (PPEBLOCKROUTINE)RtlLeaveCriticalSection;

   /* initalize loader lock */
   RtlInitializeCriticalSection (&LoaderLock);
   Peb->LoaderLock = &LoaderLock;

   /* create loader information */
   Peb->Ldr = (PPEB_LDR_DATA)RtlAllocateHeap (Peb->ProcessHeap,
					      0,
					      sizeof(PEB_LDR_DATA));
   if (Peb->Ldr == NULL)
     {
	DbgPrint("Failed to create loader data\n");
	ZwTerminateProcess(NtCurrentProcess(),STATUS_UNSUCCESSFUL);
     }
   Peb->Ldr->Length = sizeof(PEB_LDR_DATA);
   Peb->Ldr->Initialized = FALSE;
   Peb->Ldr->SsHandle = NULL;
   InitializeListHead(&Peb->Ldr->InLoadOrderModuleList);
   InitializeListHead(&Peb->Ldr->InMemoryOrderModuleList);
   InitializeListHead(&Peb->Ldr->InInitializationOrderModuleList);

   /* build full ntdll path */
   wcscpy (FullNtDllPath, SharedUserData->NtSystemRoot);
   wcscat (FullNtDllPath, L"\\system32\\ntdll.dll");

   /* add entry for ntdll */
   NtModule = (PLDR_MODULE)RtlAllocateHeap (Peb->ProcessHeap,
					    0,
					    sizeof(LDR_MODULE));
   if (NtModule == NULL)
     {
	DbgPrint("Failed to create loader module entry (NTDLL)\n");
	ZwTerminateProcess(NtCurrentProcess(),STATUS_UNSUCCESSFUL);
     }
   memset(NtModule, 0, sizeof(LDR_MODULE));

   NtModule->BaseAddress = (PVOID)&_image_base__;
   NtModule->EntryPoint = 0; /* no entry point */
   RtlCreateUnicodeString (&NtModule->FullDllName,
			   FullNtDllPath);
   RtlCreateUnicodeString (&NtModule->BaseDllName,
			   L"ntdll.dll");
   NtModule->Flags = 0;
   NtModule->LoadCount = -1; /* don't unload */
   NtModule->TlsIndex = 0;
   NtModule->SectionHandle = NULL;
   NtModule->CheckSum = 0;

   NTHeaders = RtlImageNtHeader (NtModule->BaseAddress);
   NtModule->SizeOfImage = NTHeaders->OptionalHeader.SizeOfImage;
   NtModule->TimeDateStamp = NTHeaders->FileHeader.TimeDateStamp;

   InsertTailList(&Peb->Ldr->InLoadOrderModuleList,
		  &NtModule->InLoadOrderModuleList);
   InsertTailList(&Peb->Ldr->InInitializationOrderModuleList,
		  &NtModule->InInitializationOrderModuleList);


   /* add entry for executable (becomes first list entry) */
   ExeModule = (PLDR_MODULE)RtlAllocateHeap (Peb->ProcessHeap,
					     0,
					     sizeof(LDR_MODULE));
   if (ExeModule == NULL)
     {
	DbgPrint("Failed to create loader module infomation\n");
	ZwTerminateProcess(NtCurrentProcess(),STATUS_UNSUCCESSFUL);
     }
   ExeModule->BaseAddress = Peb->ImageBaseAddress;

   if ((Peb->ProcessParameters != NULL) &&
       (Peb->ProcessParameters->ImagePathName.Length != 0))
     {
	RtlCreateUnicodeString (&ExeModule->FullDllName,
			        Peb->ProcessParameters->ImagePathName.Buffer);
	RtlCreateUnicodeString (&ExeModule->BaseDllName,
				wcsrchr(ExeModule->FullDllName.Buffer, L'\\') + 1);
     }
   else
     {
	/* FIXME(???): smss.exe doesn't have a process parameter block */
	RtlCreateUnicodeString (&ExeModule->BaseDllName,
				L"smss.exe");
	RtlCreateUnicodeString (&ExeModule->FullDllName,
				L"C:\\reactos\\system32\\smss.exe");
     }

   ExeModule->Flags = 0;
   ExeModule->LoadCount = -1; /* don't unload */
   ExeModule->TlsIndex = 0;
   ExeModule->SectionHandle = NULL;
   ExeModule->CheckSum = 0;

   NTHeaders = RtlImageNtHeader (ExeModule->BaseAddress);
   ExeModule->SizeOfImage = NTHeaders->OptionalHeader.SizeOfImage;
   ExeModule->TimeDateStamp = NTHeaders->FileHeader.TimeDateStamp;

   InsertHeadList(&Peb->Ldr->InLoadOrderModuleList,
		  &ExeModule->InLoadOrderModuleList);

   EntryPoint = LdrPEStartup((PVOID)ImageBase, NULL);
   ExeModule->EntryPoint = (ULONG)EntryPoint;

   /* all required dlls are loaded now */
   Peb->Ldr->Initialized = TRUE;

   if (EntryPoint == NULL)
     {
	DbgPrint("Failed to initialize image\n");
	ZwTerminateProcess(NtCurrentProcess(),STATUS_UNSUCCESSFUL);
     }

   DbgPrint("Transferring control to image at %x\n",EntryPoint);
   Status = EntryPoint(Peb);
   ZwTerminateProcess(NtCurrentProcess(),Status);
}

/* EOF */
