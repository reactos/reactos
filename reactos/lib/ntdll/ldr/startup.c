/* $Id$
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
#include <user32/callback.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <ntdll/ntdll.h>


VOID RtlInitializeHeapManager (VOID);
VOID LdrpInitLoader(VOID);

/* GLOBALS *******************************************************************/


extern unsigned int _image_base__;

static CRITICAL_SECTION PebLock;
static CRITICAL_SECTION LoaderLock;
static RTL_BITMAP TlsBitMap;
PLDR_MODULE ExeModule;

NTSTATUS LdrpAttachThread (VOID);


#define VALUE_BUFFER_SIZE 256

BOOL FASTCALL
ReadCompatibilitySetting(HANDLE Key, LPWSTR Value, PKEY_VALUE_PARTIAL_INFORMATION ValueInfo, DWORD *Buffer)
{
	UNICODE_STRING ValueName;
	NTSTATUS Status;
	ULONG Length;

	RtlInitUnicodeString(&ValueName, Value);
	Status = NtQueryValueKey(Key,
		&ValueName,
		KeyValuePartialInformation,
		ValueInfo,
		VALUE_BUFFER_SIZE,
		&Length);

	if (!NT_SUCCESS(Status) || (ValueInfo->Type != REG_DWORD))
	{
		RtlFreeUnicodeString(&ValueName);
		return FALSE;
	}
	RtlCopyMemory(Buffer, &ValueInfo->Data[0], sizeof(DWORD));
	RtlFreeUnicodeString(&ValueName);
	return TRUE;
}

VOID FASTCALL
LoadImageFileExecutionOptions(PPEB Peb)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Value = 0;
    UNICODE_STRING ValueString;
    WCHAR ValueBuffer[64];
    ULONG ValueSize;

    if (Peb->ProcessParameters &&
        Peb->ProcessParameters->ImagePathName.Length > 0)
      {
        /* global flag */
        Status = LdrQueryImageFileExecutionOptions (&Peb->ProcessParameters->ImagePathName,
				                    L"GlobalFlag",
				                    REG_SZ,
						    (PVOID)ValueBuffer,
						    sizeof(ValueBuffer),
						    &ValueSize);
        if (NT_SUCCESS(Status))
          {
            ValueString.Buffer = ValueBuffer + 1;
	    ValueString.Length = ValueSize - 2 * sizeof(WCHAR);
	    ValueString.MaximumLength = sizeof(ValueBuffer);
	    RtlUnicodeStringToInteger(&ValueString, 16, &Value);
            Peb->NtGlobalFlag |= Value;
	    DPRINT("GlobalFlag: Key='%S', Value=%08x\n", ValueBuffer, Value);
	  }
        /*
	 * FIXME:
	 *   read more options 
         */
      }
}


	    
	
BOOL FASTCALL
LoadCompatibilitySettings(PPEB Peb)
{
	NTSTATUS Status;
	HANDLE UserKey = NULL;
	HANDLE KeyHandle;
	HANDLE SubKeyHandle;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING KeyName;
	UNICODE_STRING ValueName;
	UCHAR ValueBuffer[VALUE_BUFFER_SIZE];
	PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
	ULONG Length;
	DWORD MajorVersion, MinorVersion, BuildNumber, PlatformId,
		  SPMajorVersion, SPMinorVersion= 0;

	if(Peb->ProcessParameters &&
		(Peb->ProcessParameters->ImagePathName.Length > 0))
	{
		Status = RtlOpenCurrentUser(KEY_READ,
				    &UserKey);
		if (!NT_SUCCESS(Status))
		{
			return FALSE;
		}

		RtlRosInitUnicodeStringFromLiteral(&KeyName, 
			L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers");

		InitializeObjectAttributes(&ObjectAttributes,
			&KeyName,
			OBJ_CASE_INSENSITIVE,
			UserKey,
			NULL);

		Status = NtOpenKey(&KeyHandle,
					KEY_QUERY_VALUE,
					&ObjectAttributes);

		if (!NT_SUCCESS(Status))
		{
			if (UserKey) NtClose(UserKey);
			return FALSE;
		}

		/* query version name for application */
		ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
		Status = NtQueryValueKey(KeyHandle,
			&Peb->ProcessParameters->ImagePathName,
			KeyValuePartialInformation,
			ValueBuffer,
			VALUE_BUFFER_SIZE,
			&Length);

		if (!NT_SUCCESS(Status) || (ValueInfo->Type != REG_SZ))
		{
			NtClose(KeyHandle);
			if (UserKey) NtClose(UserKey);
			return FALSE;
		}

		ValueName.Length = ValueInfo->DataLength;
		ValueName.MaximumLength = ValueInfo->DataLength;
		ValueName.Buffer = (PWSTR)ValueInfo->Data;

		/* load version info */
		InitializeObjectAttributes(&ObjectAttributes,
			&ValueName,
			OBJ_CASE_INSENSITIVE,
			KeyHandle,
			NULL);

		Status = NtOpenKey(&SubKeyHandle,
					KEY_QUERY_VALUE,
					&ObjectAttributes);

		if (!NT_SUCCESS(Status))
		{
			NtClose(KeyHandle);
			if (UserKey) NtClose(UserKey);
			return FALSE;
		}

		DPRINT("Loading version information for: %wZ\n", &ValueName);

		/* read settings from registry */
		if(!ReadCompatibilitySetting(SubKeyHandle, L"MajorVersion", ValueInfo, &MajorVersion))
			goto finish;
		if(!ReadCompatibilitySetting(SubKeyHandle, L"MinorVersion", ValueInfo, &MinorVersion))
			goto finish;
		if(!ReadCompatibilitySetting(SubKeyHandle, L"BuildNumber", ValueInfo, &BuildNumber))
			goto finish;
		if(!ReadCompatibilitySetting(SubKeyHandle, L"PlatformId", ValueInfo, &PlatformId))
			goto finish;

		/* now assign the settings */
		Peb->OSMajorVersion = (ULONG)MajorVersion;
		Peb->OSMinorVersion = (ULONG)MinorVersion;
		Peb->OSBuildNumber = (USHORT)BuildNumber;
		Peb->OSPlatformId = (ULONG)PlatformId;

		/* optional service pack version numbers */
		if(ReadCompatibilitySetting(SubKeyHandle, L"SPMajorVersion", ValueInfo, &SPMajorVersion))
			Peb->SPMajorVersion = (UCHAR)SPMajorVersion;
		if(ReadCompatibilitySetting(SubKeyHandle, L"SPMinorVersion", ValueInfo, &SPMinorVersion))
			Peb->SPMinorVersion = (UCHAR)SPMinorVersion;

finish:
		/* we're finished */
		NtClose(SubKeyHandle);
		NtClose(KeyHandle);
		if (UserKey) NtClose(UserKey);
		return TRUE;
	}
	return FALSE;
}


/* FUNCTIONS *****************************************************************/

VOID STDCALL
__true_LdrInitializeThunk (ULONG Unknown1,
                    ULONG Unknown2,
                    ULONG Unknown3,
                    ULONG Unknown4)
{
   PIMAGE_NT_HEADERS NTHeaders;
   PEPFUNC EntryPoint;
   PIMAGE_DOS_HEADER PEDosHeader;
   PVOID ImageBase;
   PPEB Peb;
   PLDR_MODULE NtModule;  // ntdll
   NLSTABLEINFO NlsTable;
   WCHAR FullNtDllPath[MAX_PATH];
   SYSTEM_BASIC_INFORMATION SystemInformation;
   NTSTATUS Status;

   DPRINT("LdrInitializeThunk()\n");
   if (NtCurrentPeb()->Ldr == NULL || NtCurrentPeb()->Ldr->Initialized == FALSE)
     {
       Peb = (PPEB)(PEB_BASE);
       DPRINT("Peb %x\n", Peb);
       ImageBase = Peb->ImageBaseAddress;
       DPRINT("ImageBase %x\n", ImageBase);
       if (ImageBase <= (PVOID)0x1000)
         {
           DPRINT("ImageBase is null\n");
           ZwTerminateProcess(NtCurrentProcess(), STATUS_UNSUCCESSFUL);
         }

       /*  If MZ header exists  */
       PEDosHeader = (PIMAGE_DOS_HEADER) ImageBase;
       DPRINT("PEDosHeader %x\n", PEDosHeader);

       if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC ||
           PEDosHeader->e_lfanew == 0L ||
           *(PULONG)((PUCHAR)ImageBase + PEDosHeader->e_lfanew) != IMAGE_PE_MAGIC)
         {
           DPRINT1("Image has bad header\n");
           ZwTerminateProcess(NtCurrentProcess(), STATUS_UNSUCCESSFUL);
         }

       /* normalize process parameters */
       RtlNormalizeProcessParams (Peb->ProcessParameters);

       /* Initialize NLS data */
       RtlInitNlsTables (Peb->AnsiCodePageData,
                         Peb->OemCodePageData,
                         Peb->UnicodeCaseTableData,
                         &NlsTable);
       RtlResetRtlTranslations (&NlsTable);

       NTHeaders = (PIMAGE_NT_HEADERS)(ImageBase + PEDosHeader->e_lfanew);
      

       /* Get number of processors */
       Status = ZwQuerySystemInformation(SystemBasicInformation,
	                                 &SystemInformation,
					 sizeof(SYSTEM_BASIC_INFORMATION),
					 NULL);
       if (!NT_SUCCESS(Status))
         {
	   ZwTerminateProcess(NtCurrentProcess(), Status);
	 }

       Peb->NumberOfProcessors = SystemInformation.NumberProcessors;
       /* create process heap */
       RtlInitializeHeapManager();
       Peb->ProcessHeap = RtlCreateHeap(HEAP_GROWABLE,
                                        NULL,
                                        NTHeaders->OptionalHeader.SizeOfHeapReserve,
                                        NTHeaders->OptionalHeader.SizeOfHeapCommit,
                                        NULL,
                                        NULL);
       if (Peb->ProcessHeap == 0)
         {
           DPRINT1("Failed to create process heap\n");
           ZwTerminateProcess(NtCurrentProcess(),STATUS_UNSUCCESSFUL);
         }

       /* Initialize Critical Section Data */
       RtlpInitDeferedCriticalSection();
             
       /* initalize peb lock support */
       RtlInitializeCriticalSection (&PebLock);
       Peb->FastPebLock = &PebLock;
       Peb->FastPebLockRoutine = (PPEBLOCKROUTINE)RtlEnterCriticalSection;
       Peb->FastPebUnlockRoutine = (PPEBLOCKROUTINE)RtlLeaveCriticalSection;

       /* initialize tls bitmap */
       RtlInitializeBitMap (&TlsBitMap,
                            Peb->TlsBitmapBits,
                            TLS_MINIMUM_AVAILABLE);
       Peb->TlsBitmap = &TlsBitMap;
       Peb->TlsExpansionCounter = TLS_MINIMUM_AVAILABLE;

       /* Initialize table of callbacks for the kernel. */
       Peb->KernelCallbackTable = 
         RtlAllocateHeap(RtlGetProcessHeap(),
                         0,
                         sizeof(PVOID) * (USER32_CALLBACK_MAXIMUM + 1));
       
       /* initalize loader lock */
       RtlInitializeCriticalSection (&LoaderLock);
       Peb->LoaderLock = &LoaderLock;

       /* create loader information */
       Peb->Ldr = (PPEB_LDR_DATA)RtlAllocateHeap (Peb->ProcessHeap,
                                                  0,
                                                  sizeof(PEB_LDR_DATA));
       if (Peb->Ldr == NULL)
         {
           DPRINT1("Failed to create loader data\n");
           ZwTerminateProcess(NtCurrentProcess(),STATUS_UNSUCCESSFUL);
         }
       Peb->Ldr->Length = sizeof(PEB_LDR_DATA);
       Peb->Ldr->Initialized = FALSE;
       Peb->Ldr->SsHandle = NULL;
       InitializeListHead(&Peb->Ldr->InLoadOrderModuleList);
       InitializeListHead(&Peb->Ldr->InMemoryOrderModuleList);
       InitializeListHead(&Peb->Ldr->InInitializationOrderModuleList);

       /* Load compatibility settings */
       LoadCompatibilitySettings(Peb);

       /* Load execution options */
       LoadImageFileExecutionOptions(Peb);

       /* build full ntdll path */
       wcscpy (FullNtDllPath, SharedUserData->NtSystemRoot);
       wcscat (FullNtDllPath, L"\\system32\\ntdll.dll");

       /* add entry for ntdll */
       NtModule = (PLDR_MODULE)RtlAllocateHeap (Peb->ProcessHeap,
                                                0,
                                                sizeof(LDR_MODULE));
       if (NtModule == NULL)
         {
           DPRINT1("Failed to create loader module entry (NTDLL)\n");
           ZwTerminateProcess(NtCurrentProcess(),STATUS_UNSUCCESSFUL);
	 }
       memset(NtModule, 0, sizeof(LDR_MODULE));

       NtModule->BaseAddress = (PVOID)&_image_base__;
       NtModule->EntryPoint = 0; /* no entry point */
       RtlCreateUnicodeString (&NtModule->FullDllName,
                               FullNtDllPath);
       RtlCreateUnicodeString (&NtModule->BaseDllName,
                               L"ntdll.dll");
       NtModule->Flags = IMAGE_DLL|ENTRY_PROCESSED;

       NtModule->LoadCount = -1; /* don't unload */
       NtModule->TlsIndex = -1;
       NtModule->SectionHandle = NULL;
       NtModule->CheckSum = 0;

       NTHeaders = RtlImageNtHeader (NtModule->BaseAddress);
       NtModule->SizeOfImage = NTHeaders->OptionalHeader.SizeOfImage;
       NtModule->TimeDateStamp = NTHeaders->FileHeader.TimeDateStamp;

       InsertTailList(&Peb->Ldr->InLoadOrderModuleList,
                      &NtModule->InLoadOrderModuleList);
       InsertTailList(&Peb->Ldr->InInitializationOrderModuleList,
                      &NtModule->InInitializationOrderModuleList);

#if defined(DBG) || defined(KDBG)

       LdrpLoadUserModuleSymbols(NtModule);

#endif /* DBG || KDBG */

       /* add entry for executable (becomes first list entry) */
       ExeModule = (PLDR_MODULE)RtlAllocateHeap (Peb->ProcessHeap,
                                                 0,
                                                 sizeof(LDR_MODULE));
       if (ExeModule == NULL)
         {
           DPRINT1("Failed to create loader module infomation\n");
           ZwTerminateProcess(NtCurrentProcess(),STATUS_UNSUCCESSFUL);
         }
       ExeModule->BaseAddress = Peb->ImageBaseAddress;

       if ((Peb->ProcessParameters == NULL) ||
           (Peb->ProcessParameters->ImagePathName.Length == 0))
         {
           DPRINT1("Failed to access the process parameter block\n");
           ZwTerminateProcess(NtCurrentProcess(),STATUS_UNSUCCESSFUL);
         }

       RtlCreateUnicodeString(&ExeModule->FullDllName,
                              Peb->ProcessParameters->ImagePathName.Buffer);
       RtlCreateUnicodeString(&ExeModule->BaseDllName,
                              wcsrchr(ExeModule->FullDllName.Buffer, L'\\') + 1);

       DPRINT("BaseDllName '%wZ'  FullDllName '%wZ'\n",
              &ExeModule->BaseDllName,
              &ExeModule->FullDllName);

       ExeModule->Flags = ENTRY_PROCESSED;
       ExeModule->LoadCount = -1; /* don't unload */
       ExeModule->TlsIndex = -1;
       ExeModule->SectionHandle = NULL;
       ExeModule->CheckSum = 0;

       NTHeaders = RtlImageNtHeader (ExeModule->BaseAddress);
       ExeModule->SizeOfImage = NTHeaders->OptionalHeader.SizeOfImage;
       ExeModule->TimeDateStamp = NTHeaders->FileHeader.TimeDateStamp;

       InsertHeadList(&Peb->Ldr->InLoadOrderModuleList,
                      &ExeModule->InLoadOrderModuleList);

       LdrpInitLoader();

#if defined(DBG) || defined(KDBG)

       LdrpLoadUserModuleSymbols(ExeModule);

#endif /* DBG || KDBG */

       EntryPoint = LdrPEStartup((PVOID)ImageBase, NULL, NULL, NULL);
       ExeModule->EntryPoint = (ULONG)EntryPoint;

       /* all required dlls are loaded now */
       Peb->Ldr->Initialized = TRUE;

       /* Check before returning that we can run the image safely. */
       if (EntryPoint == NULL)
         {
           DPRINT1("Failed to initialize image\n");
           ZwTerminateProcess(NtCurrentProcess(),STATUS_UNSUCCESSFUL);
         }
     }
   /* attach the thread */
   RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);
   LdrpAttachThread();
   RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
}

/* EOF */
