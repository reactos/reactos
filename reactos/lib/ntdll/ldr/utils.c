/* $Id: utils.c,v 1.82 2004/01/31 23:53:45 gvg Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/utils.c
 * PURPOSE:         Process startup for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 *                  Hartmut Birr
 */

/*
 * TODO:
 *      - Handle loading flags correctly
 *      - Handle errors correctly (unload dll's)
 *      - Implement a faster way to find modules (hash table)
 *      - any more ??
 */

/* INCLUDES *****************************************************************/

#include <reactos/config.h>
#include <ddk/ntddk.h>
#include <windows.h>
#include <string.h>
#include <wchar.h>
#include <ntdll/ldr.h>
#include <ntos/minmax.h>

#define LDRP_PROCESS_CREATION_TIME 0x8000000

#ifdef DBG_NTDLL_LDR_UTILS
#define NDEBUG
#endif
#include <ntdll/ntdll.h>

/* GLOBALS *******************************************************************/

typedef struct _TLS_DATA
{
   PVOID StartAddressOfRawData;
   DWORD TlsDataSize;
   DWORD TlsZeroSize;
   PIMAGE_TLS_CALLBACK TlsAddressOfCallBacks;
   PLDR_MODULE Module;
} TLS_DATA, *PTLS_DATA;

static PTLS_DATA LdrpTlsArray = NULL;
static ULONG LdrpTlsCount = 0;
static ULONG LdrpTlsSize = 0;
static HANDLE LdrpKnownDllsDirHandle = NULL;
static UNICODE_STRING LdrpKnownDllPath = {0, 0, NULL};
static PLDR_MODULE LdrpLastModule = NULL;
extern ULONG NtGlobalFlag;
extern PLDR_MODULE ExeModule;

/* PROTOTYPES ****************************************************************/

static NTSTATUS LdrFindEntryForName(PUNICODE_STRING Name, PLDR_MODULE *Module, BOOL Ref);
static PVOID LdrFixupForward(PCHAR ForwardName);
static PVOID LdrGetExportByName(PVOID BaseAddress, PUCHAR SymbolName, USHORT Hint);
static NTSTATUS LdrpLoadModule(IN PWSTR SearchPath OPTIONAL, 
			       IN ULONG LoadFlags,  
			       IN PUNICODE_STRING Name,
			       OUT PLDR_MODULE *Module);
static NTSTATUS LdrpAttachProcess(VOID);
static VOID LdrpDetachProcess(BOOL UnloadAll);

/* FUNCTIONS *****************************************************************/

#ifdef KDBG

VOID
LdrpLoadUserModuleSymbols(PLDR_MODULE LdrModule)
{
  NtSystemDebugControl(
    DebugDbgLoadSymbols,
    (PVOID)LdrModule,
    0,
    NULL,
    0,
    NULL);
}

#endif /* DBG */

static inline LONG LdrpDecrementLoadCount(PLDR_MODULE Module, BOOL Locked)
{
   LONG LoadCount;
   if (!Locked)
     {
       RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
     }
   LoadCount = Module->LoadCount;
   if (Module->LoadCount > 0)
     {
       Module->LoadCount--;
     }
   if (!Locked)
     {
       RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
     }
   return LoadCount;
}

static inline LONG LdrpIncrementLoadCount(PLDR_MODULE Module, BOOL Locked)
{
   LONG LoadCount;
   if (!Locked)
     {
       RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
     }
   LoadCount = Module->LoadCount;
   if (Module->LoadCount >= 0)
     {
       Module->LoadCount++;
     }
   if (!Locked)
     {
       RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
     }
   return LoadCount;
}

static inline VOID LdrpAcquireTlsSlot(PLDR_MODULE Module, ULONG Size, BOOL Locked)
{
   if (!Locked)
     {
       RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
     }
   Module->TlsIndex = (SHORT)LdrpTlsCount;
   LdrpTlsCount++;
   LdrpTlsSize += Size;
   if (!Locked)
     {
       RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
     }
}

static inline VOID LdrpTlsCallback(PLDR_MODULE Module, ULONG dwReason)
{
   PIMAGE_TLS_CALLBACK TlsCallback;
   if (Module->TlsIndex >= 0 && Module->LoadCount == -1)
     {
       TlsCallback = LdrpTlsArray[Module->TlsIndex].TlsAddressOfCallBacks;
       if (TlsCallback)
         {
           while (*TlsCallback)
             {
	       TRACE_LDR("%wZ - Calling tls callback at %x\n",
	                 &Module->BaseDllName, TlsCallback);
	       TlsCallback(Module->BaseAddress, dwReason, NULL);
	       TlsCallback++;
	     }
	 }
     }
}

static BOOL LdrpCallDllEntry(PLDR_MODULE Module, DWORD dwReason, PVOID lpReserved)
{
   if (!(Module->Flags & IMAGE_DLL) ||
       Module->EntryPoint == 0)
     {
       return TRUE;
     }
   LdrpTlsCallback(Module, dwReason);
   return  ((PDLLMAIN_FUNC)Module->EntryPoint)(Module->BaseAddress, dwReason, lpReserved);
}

static NTSTATUS
LdrpInitializeTlsForThread(VOID)
{
   PVOID* TlsPointers;
   PTLS_DATA TlsInfo;
   PVOID TlsData;
   ULONG i;

   DPRINT("LdrpInitializeTlsForThread() called for %wZ\n", &ExeModule->BaseDllName);

   if (LdrpTlsCount > 0)
     {
       TlsPointers = RtlAllocateHeap(RtlGetProcessHeap(),
                                     0,
                                     LdrpTlsCount * sizeof(PVOID) + LdrpTlsSize); 
       if (TlsPointers == NULL)
         {
	   DPRINT1("failed to allocate thread tls data\n");
	   return STATUS_NO_MEMORY;
	 }
       
       TlsData = (PVOID)TlsPointers + LdrpTlsCount * sizeof(PVOID);
       NtCurrentTeb()->ThreadLocalStoragePointer = TlsPointers;

       TlsInfo = LdrpTlsArray;
       for (i = 0; i < LdrpTlsCount; i++, TlsInfo++)
         {
	   TRACE_LDR("Initialize tls data for %wZ\n", &TlsInfo->Module->BaseDllName);
	   TlsPointers[i] = TlsData;
	   if (TlsInfo->TlsDataSize)
	     {
	       memcpy(TlsData, TlsInfo->StartAddressOfRawData, TlsInfo->TlsDataSize);
	       TlsData += TlsInfo->TlsDataSize;
	     }
	   if (TlsInfo->TlsZeroSize)
	     {
	       memset(TlsData, 0, TlsInfo->TlsZeroSize);
	       TlsData += TlsInfo->TlsZeroSize;
	     }
	 }
     }
   DPRINT("LdrpInitializeTlsForThread() done\n");
   return STATUS_SUCCESS;
}

static NTSTATUS
LdrpInitializeTlsForProccess(VOID)
{
   PLIST_ENTRY ModuleListHead;
   PLIST_ENTRY Entry;
   PLDR_MODULE Module;
   PIMAGE_TLS_DIRECTORY TlsDirectory;
   PTLS_DATA TlsData;

   DPRINT("LdrpInitializeTlsForProccess() called for %wZ\n", &ExeModule->BaseDllName);
   
   if (LdrpTlsCount > 0)
     {
       LdrpTlsArray = RtlAllocateHeap(RtlGetProcessHeap(),
                                      0,
                                      LdrpTlsCount * sizeof(TLS_DATA)); 
       if (LdrpTlsArray == NULL)
         {
	   DPRINT1("Failed to allocate global tls data\n");
           return STATUS_NO_MEMORY;
         }
      
       ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
       Entry = ModuleListHead->Flink;
       while (Entry != ModuleListHead)
         {
           Module = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);
	   if (Module->LoadCount == -1 &&
	       Module->TlsIndex >= 0)
             {
               TlsDirectory = (PIMAGE_TLS_DIRECTORY)
		                 RtlImageDirectoryEntryToData(Module->BaseAddress,
                                                              TRUE,
                                                              IMAGE_DIRECTORY_ENTRY_TLS,
                                                              NULL);
               assert(Module->TlsIndex < LdrpTlsCount);
	       TlsData = &LdrpTlsArray[Module->TlsIndex];
	       TlsData->StartAddressOfRawData = (PVOID)TlsDirectory->StartAddressOfRawData;
	       TlsData->TlsDataSize = TlsDirectory->EndAddressOfRawData - TlsDirectory->StartAddressOfRawData;
	       TlsData->TlsZeroSize = TlsDirectory->SizeOfZeroFill;
	       TlsData->TlsAddressOfCallBacks = *TlsDirectory->AddressOfCallBacks;
	       TlsData->Module = Module;
#if 0
               DbgPrint("TLS directory for %wZ\n", &Module->BaseDllName);
	       DbgPrint("StartAddressOfRawData: %x\n", TlsDirectory->StartAddressOfRawData);
	       DbgPrint("EndAddressOfRawData:   %x\n", TlsDirectory->EndAddressOfRawData);
	       DbgPrint("SizeOfRawData:         %d\n", TlsDirectory->EndAddressOfRawData - TlsDirectory->StartAddressOfRawData);
	       DbgPrint("AddressOfIndex:        %x\n", TlsDirectory->AddressOfIndex);
	       DbgPrint("AddressOfCallBacks:    %x (%x)\n", TlsDirectory->AddressOfCallBacks, *TlsDirectory->AddressOfCallBacks);
	       DbgPrint("SizeOfZeroFill:        %d\n", TlsDirectory->SizeOfZeroFill);
	       DbgPrint("Characteristics:       %x\n", TlsDirectory->Characteristics);
#endif
	       /* 
	        * FIXME:
		*   Is this region allways writable ?
		*/
               *(PULONG)TlsDirectory->AddressOfIndex = Module->TlsIndex;
	       CHECKPOINT1;
	     }
	   Entry = Entry->Flink;
	}
    }
  DPRINT("LdrpInitializeTlsForProccess() done\n");
  return STATUS_SUCCESS;
}

VOID
LdrpInitLoader(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING LinkTarget;
  UNICODE_STRING Name;
  HANDLE LinkHandle;
  ULONG Length;
  NTSTATUS Status;

  DPRINT("LdrpInitLoader() called for %wZ\n", &ExeModule->BaseDllName);

  /* Get handle to the 'KnownDlls' directory */
  RtlInitUnicodeString(&Name,
		       L"\\KnownDlls");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = NtOpenDirectoryObject(&LdrpKnownDllsDirHandle,
				 DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
				 &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtOpenDirectoryObject() failed (Status %lx)\n", Status);
      LdrpKnownDllsDirHandle = NULL;
      return;
    }

  /* Allocate target name string */
  LinkTarget.Length = 0;
  LinkTarget.MaximumLength = MAX_PATH * sizeof(WCHAR);
  LinkTarget.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
				      0,
				      MAX_PATH * sizeof(WCHAR));
  if (LinkTarget.Buffer == NULL)
    {
      NtClose(LdrpKnownDllsDirHandle);
      LdrpKnownDllsDirHandle = NULL;
      return;
    }

  RtlInitUnicodeString(&Name,
		       L"KnownDllPath");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENLINK,
			     LdrpKnownDllsDirHandle,
			     NULL);
  Status = NtOpenSymbolicLinkObject(&LinkHandle,
				    SYMBOLIC_LINK_ALL_ACCESS,
				    &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&LinkTarget);
      NtClose(LdrpKnownDllsDirHandle);
      LdrpKnownDllsDirHandle = NULL;
      return;
    }

  Status = NtQuerySymbolicLinkObject(LinkHandle,
				     &LinkTarget,
				     &Length);
  NtClose(LinkHandle);
  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString(&LinkTarget);
      NtClose(LdrpKnownDllsDirHandle);
      LdrpKnownDllsDirHandle = NULL;
    }

  RtlCreateUnicodeString(&LdrpKnownDllPath,
			 LinkTarget.Buffer);

  RtlFreeUnicodeString(&LinkTarget);

  DPRINT("LdrpInitLoader() done\n");
}


/***************************************************************************
 * NAME                                                         LOCAL
 *      LdrAdjustDllName
 *
 * DESCRIPTION
 *      Adjusts the name of a dll to a fully qualified name.
 *
 * ARGUMENTS
 *      FullDllName:    Pointer to caller supplied storage for the fully
 *                      qualified dll name.
 *      DllName:        Pointer to the dll name.
 *      BaseName:       TRUE:  Only the file name is passed to FullDllName
 *                      FALSE: The full path is preserved in FullDllName
 *
 * RETURN VALUE
 *      None
 *
 * REVISIONS
 *
 * NOTE
 *      A given path is not affected by the adjustment, but the file
 *      name only:
 *        ntdll      --> ntdll.dll
 *        ntdll.     --> ntdll
 *        ntdll.xyz  --> ntdll.xyz
 */
static VOID
LdrAdjustDllName (PUNICODE_STRING FullDllName,
                  PUNICODE_STRING DllName,
                  BOOLEAN BaseName)
{
   WCHAR Buffer[MAX_PATH];
   ULONG Length;
   PWCHAR Extension;
   PWCHAR Pointer;

   Length = DllName->Length / sizeof(WCHAR);

   if (BaseName)
     {
        /* get the base dll name */
        Pointer = DllName->Buffer + Length;
        Extension = Pointer;

        do
          {
             --Pointer;
          }
        while (Pointer >= DllName->Buffer && *Pointer != L'\\' && *Pointer != L'/');

        Pointer++;
        Length = Extension - Pointer;
        memmove (Buffer, Pointer, Length * sizeof(WCHAR));
        Buffer[Length] = L'\0';
     }
   else
     {
        /* get the full dll name */
        memmove (Buffer, DllName->Buffer, DllName->Length);
        Buffer[DllName->Length / sizeof(WCHAR)] = L'\0';
     }

   /* Build the DLL's absolute name */
   Extension = wcsrchr (Buffer, L'.');
   if ((Extension != NULL) && (*Extension == L'.'))
     {
        /* with extension - remove dot if it's the last character */
        if (Buffer[Length - 1] == L'.')
                        Length--;
        Buffer[Length] = 0;
     }
   else
     {
        /* name without extension - assume that it is .dll */
        memmove (Buffer + Length, L".dll", 10);
     }

   RtlCreateUnicodeString(FullDllName, Buffer);
}

PLDR_MODULE
LdrAddModuleEntry(PVOID ImageBase,
		  PIMAGE_NT_HEADERS NTHeaders,
		  PWSTR FullDosName)
{
  PLDR_MODULE Module;

  Module = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof (LDR_MODULE));
  assert(Module);
  memset(Module, 0, sizeof(LDR_MODULE));
  Module->BaseAddress = (PVOID)ImageBase;
  Module->EntryPoint = NTHeaders->OptionalHeader.AddressOfEntryPoint;
  if (Module->EntryPoint != 0)
    Module->EntryPoint += (ULONG)Module->BaseAddress;
  Module->SizeOfImage = NTHeaders->OptionalHeader.SizeOfImage;
  if (NtCurrentPeb()->Ldr->Initialized == TRUE)
    {
      /* loading while app is running */
      Module->LoadCount = 1;
    } else {
      /*
       * loading while app is initializing
       * dll must not be unloaded
       */
      Module->LoadCount = -1;
    }

  Module->Flags = 0;
  Module->TlsIndex = -1;
  Module->CheckSum = NTHeaders->OptionalHeader.CheckSum;
  Module->TimeDateStamp = NTHeaders->FileHeader.TimeDateStamp;

  RtlCreateUnicodeString (&Module->FullDllName,
                          FullDosName);
  RtlCreateUnicodeString (&Module->BaseDllName,
                          wcsrchr(FullDosName, L'\\') + 1);
  DPRINT ("BaseDllName %wZ\n", &Module->BaseDllName);
  
  RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
  InsertTailList(&NtCurrentPeb()->Ldr->InLoadOrderModuleList,
                 &Module->InLoadOrderModuleList);
  RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);

  return(Module);
}


static NTSTATUS
LdrpMapKnownDll(IN PUNICODE_STRING DllName,
		OUT PUNICODE_STRING FullDosName,
		OUT PHANDLE SectionHandle)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  NTSTATUS Status;

  DPRINT("LdrpMapKnownDll() called\n");

  if (LdrpKnownDllsDirHandle == NULL)
    {
      DPRINT("Invalid 'KnownDlls' directory\n");
      return STATUS_UNSUCCESSFUL;
    }

  DPRINT("LdrpKnownDllPath '%wZ'\n", &LdrpKnownDllPath);

  InitializeObjectAttributes(&ObjectAttributes,
			     DllName,
			     OBJ_CASE_INSENSITIVE,
			     LdrpKnownDllsDirHandle,
			     NULL);
  Status = NtOpenSection(SectionHandle,
			 SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE,
			 &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtOpenSection() failed for '%wZ' (Status %lx)\n", DllName, Status);
      return Status;
    }

  FullDosName->Length = LdrpKnownDllPath.Length + DllName->Length + sizeof(WCHAR);
  FullDosName->MaximumLength = FullDosName->Length + sizeof(WCHAR);
  FullDosName->Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
					0,
					FullDosName->MaximumLength);
  if (FullDosName->Buffer == NULL)
    {
      FullDosName->Length = 0;
      FullDosName->MaximumLength = 0;
      return STATUS_SUCCESS;
    }

  wcscpy(FullDosName->Buffer, LdrpKnownDllPath.Buffer);
  wcscat(FullDosName->Buffer, L"\\");
  wcscat(FullDosName->Buffer, DllName->Buffer);

  DPRINT("FullDosName '%wZ'\n", FullDosName);

  DPRINT("LdrpMapKnownDll() done\n");

  return STATUS_SUCCESS;
}


static NTSTATUS
LdrpMapDllImageFile(IN PWSTR SearchPath OPTIONAL,
		    IN PUNICODE_STRING DllName,
		    OUT PUNICODE_STRING FullDosName,
		    OUT PHANDLE SectionHandle)
{
  WCHAR                 SearchPathBuffer[MAX_PATH];
  WCHAR                 DosName[MAX_PATH];
  UNICODE_STRING        FullNtFileName;
  OBJECT_ATTRIBUTES     FileObjectAttributes;
  HANDLE                FileHandle;
  char                  BlockBuffer [1024];
  PIMAGE_DOS_HEADER     DosHeader;
  PIMAGE_NT_HEADERS     NTHeaders;
  PVOID                 ImageBase;
  ULONG                 ImageSize;
  IO_STATUS_BLOCK       IoStatusBlock;
  NTSTATUS Status;

  DPRINT("LdrpMapDllImageFile() called\n");

  if (SearchPath == NULL)
    {
      SearchPath = SearchPathBuffer;
      wcscpy (SearchPathBuffer, SharedUserData->NtSystemRoot);
      wcscat (SearchPathBuffer, L"\\system32;");
      wcscat (SearchPathBuffer, SharedUserData->NtSystemRoot);
      wcscat (SearchPathBuffer, L";.");
    }

  DPRINT("SearchPath %S\n", SearchPath);

  if (RtlDosSearchPath_U (SearchPath,
                          DllName->Buffer,
                          NULL,
                          MAX_PATH,
                          DosName,
                          NULL) == 0)
    return STATUS_DLL_NOT_FOUND;

  DPRINT("DosName %S\n", DosName);

  if (!RtlDosPathNameToNtPathName_U (DosName,
                                     &FullNtFileName,
                                     NULL,
                                     NULL))
    return STATUS_DLL_NOT_FOUND;

  DPRINT("FullNtFileName %wZ\n", &FullNtFileName);

  InitializeObjectAttributes(&FileObjectAttributes,
                             &FullNtFileName,
                             0,
                             NULL,
                             NULL);

  DPRINT("Opening dll \"%wZ\"\n", &FullNtFileName);

  Status = NtOpenFile(&FileHandle,
                      GENERIC_READ|SYNCHRONIZE,
                      &FileObjectAttributes,
                      &IoStatusBlock,
                      0,
                      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Dll open of %wZ failed: Status = 0x%08x\n", 
               &FullNtFileName, Status);
      RtlFreeUnicodeString (&FullNtFileName);
      return Status;
    }
  RtlFreeUnicodeString (&FullNtFileName);

  Status = NtReadFile(FileHandle,
                      NULL,
                      NULL,
                      NULL,
                      &IoStatusBlock,
                      BlockBuffer,
                      sizeof(BlockBuffer),
                      NULL,
                      NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Dll header read failed: Status = 0x%08x\n", Status);
      NtClose(FileHandle);
      return Status;
    }
  /*
   * Overlay DOS and NT headers structures to the 
   * buffer with DLL's header raw data.
   */
  DosHeader = (PIMAGE_DOS_HEADER) BlockBuffer;
  NTHeaders = (PIMAGE_NT_HEADERS) (BlockBuffer + DosHeader->e_lfanew);
  /*
   * Check it is a PE image file.
   */
  if ((DosHeader->e_magic != IMAGE_DOS_MAGIC)
      || (DosHeader->e_lfanew == 0L)
      || (*(PULONG)(NTHeaders) != IMAGE_PE_MAGIC))
    {
      DPRINT("NTDLL format invalid\n");
      NtClose(FileHandle);
      
      return STATUS_UNSUCCESSFUL;
    }
  
  ImageBase = (PVOID) NTHeaders->OptionalHeader.ImageBase;
  ImageSize = NTHeaders->OptionalHeader.SizeOfImage;
  
  DPRINT("ImageBase 0x%08x\n", ImageBase);
  
  /*
   * Create a section for dll.
   */
  Status = NtCreateSection(SectionHandle,
                           SECTION_ALL_ACCESS,
                           NULL,
                           NULL,
                           PAGE_READWRITE,
                           SEC_COMMIT | SEC_IMAGE,
                           FileHandle);
  NtClose(FileHandle);

  if (!NT_SUCCESS(Status))
    {
      DPRINT("NTDLL create section failed: Status = 0x%08x\n", Status);
      return Status;
    }

  RtlCreateUnicodeString(FullDosName,
			 DosName);

  return Status;
}



/***************************************************************************
 * NAME                                                         EXPORTED
 *      LdrLoadDll
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 * @implemented
 */
NTSTATUS STDCALL
LdrLoadDll (IN PWSTR SearchPath OPTIONAL,
            IN ULONG LoadFlags,
            IN PUNICODE_STRING Name,
            OUT PVOID *BaseAddress OPTIONAL)
{
  NTSTATUS              Status;
  PLDR_MODULE           Module;

  TRACE_LDR("LdrLoadDll, loading %wZ%s%S\n", Name, SearchPath ? " from " : "", SearchPath ? SearchPath : L"");

  if (Name == NULL)
    {
      *BaseAddress = NtCurrentPeb()->ImageBaseAddress;
      return STATUS_SUCCESS;
    }

  *BaseAddress = NULL;

  Status = LdrpLoadModule(SearchPath, LoadFlags, Name, &Module);
  if (NT_SUCCESS(Status))
    {
      RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);
      Status = LdrpAttachProcess();
      RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
      if (NT_SUCCESS(Status))
        {
          *BaseAddress = Module->BaseAddress;
        }
   }
  return Status;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      LdrFindEntryForAddress
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 * @implemented
 */
NTSTATUS STDCALL
LdrFindEntryForAddress(PVOID Address,
                       PLDR_MODULE *Module)
{
  PLIST_ENTRY ModuleListHead;
  PLIST_ENTRY Entry;
  PLDR_MODULE ModulePtr;

  DPRINT("LdrFindEntryForAddress(Address %p)\n", Address);

  if (NtCurrentPeb()->Ldr == NULL)
    return(STATUS_NO_MORE_ENTRIES);

  RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);
  ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
  Entry = ModuleListHead->Flink;
  if (Entry == ModuleListHead)
    {
      RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
      return(STATUS_NO_MORE_ENTRIES);
    }

  while (Entry != ModuleListHead)
    {
      ModulePtr = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);

      DPRINT("Scanning %wZ at %p\n", &ModulePtr->BaseDllName, ModulePtr->BaseAddress);

      if ((Address >= ModulePtr->BaseAddress) &&
          (Address <= (ModulePtr->BaseAddress + ModulePtr->SizeOfImage)))
        {
          *Module = ModulePtr;
	  RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
          return(STATUS_SUCCESS);
        }

      Entry = Entry->Flink;
    }

  DPRINT("Failed to find module entry.\n");

  RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
  return(STATUS_NO_MORE_ENTRIES);
}


/***************************************************************************
 * NAME                                                         LOCAL
 *      LdrFindEntryForName
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */
static NTSTATUS
LdrFindEntryForName(PUNICODE_STRING Name,
                    PLDR_MODULE *Module,
		    BOOL Ref)
{
  PLIST_ENTRY ModuleListHead;
  PLIST_ENTRY Entry;
  PLDR_MODULE ModulePtr;
  BOOLEAN ContainsPath;
  UNICODE_STRING AdjustedName;
  unsigned i;

  DPRINT("LdrFindEntryForName(Name %wZ)\n", Name);

  if (NtCurrentPeb()->Ldr == NULL)
    return(STATUS_NO_MORE_ENTRIES);

  RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);
  ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
  Entry = ModuleListHead->Flink;
  if (Entry == ModuleListHead)
    {
      RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
      return(STATUS_NO_MORE_ENTRIES);
    }

  // NULL is the current process
  if (Name == NULL)
    {
      *Module = ExeModule;
      RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
      return(STATUS_SUCCESS);
    }

  LdrAdjustDllName (&AdjustedName, Name, FALSE);

  ContainsPath = (AdjustedName.Length >= 2 * sizeof(WCHAR) && L':' == AdjustedName.Buffer[1]);
  for (i = 0; ! ContainsPath && i < AdjustedName.Length / sizeof(WCHAR); i++)
    {
      ContainsPath = L'\\' == AdjustedName.Buffer[i] ||
                     L'/' == AdjustedName.Buffer[i];
    }
  
  if (LdrpLastModule)
    {
      if ((! ContainsPath &&
           0 == RtlCompareUnicodeString(&LdrpLastModule->BaseDllName, &AdjustedName, TRUE)) ||
          (ContainsPath &&
           0 == RtlCompareUnicodeString(&LdrpLastModule->FullDllName, &AdjustedName, TRUE)))
        {
          *Module = LdrpLastModule;
	  if (Ref && (*Module)->LoadCount != -1)
	    {
              (*Module)->LoadCount++;
	    }
          RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
	  RtlFreeUnicodeString(&AdjustedName);
          return(STATUS_SUCCESS);
        }
    }
  while (Entry != ModuleListHead)
    {
      ModulePtr = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);

      DPRINT("Scanning %wZ %wZ\n", &ModulePtr->BaseDllName, &AdjustedName);

      if ((! ContainsPath &&
           0 == RtlCompareUnicodeString(&ModulePtr->BaseDllName, &AdjustedName, TRUE)) ||
          (ContainsPath &&
           0 == RtlCompareUnicodeString(&ModulePtr->FullDllName, &AdjustedName, TRUE)))
        {
          *Module = LdrpLastModule = ModulePtr;
	  if (Ref && ModulePtr->LoadCount != -1)
	    {
              ModulePtr->LoadCount++;
            }
          RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
	  RtlFreeUnicodeString(&AdjustedName);
          return(STATUS_SUCCESS);
        }

      Entry = Entry->Flink;
    }

  DPRINT("Failed to find dll %wZ\n", Name);
  RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
  RtlFreeUnicodeString(&AdjustedName);
  return(STATUS_NO_MORE_ENTRIES);
}

/**********************************************************************
 * NAME                                                         LOCAL
 *      LdrFixupForward
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */
static PVOID
LdrFixupForward(PCHAR ForwardName)
{
   CHAR NameBuffer[128];
   UNICODE_STRING DllName;
   NTSTATUS Status;
   PCHAR p;
   PLDR_MODULE Module;
   PVOID BaseAddress;

   strcpy(NameBuffer, ForwardName);
   p = strchr(NameBuffer, '.');
   if (p != NULL)
     {
        *p = 0;

        DPRINT("Dll: %s  Function: %s\n", NameBuffer, p+1);
        RtlCreateUnicodeStringFromAsciiz (&DllName,
                                          NameBuffer);

        Status = LdrFindEntryForName (&DllName, &Module, FALSE);
	/* FIXME:
	 *   The caller (or the image) is responsible for loading of the dll, where the function is forwarded.
         */
        if (!NT_SUCCESS(Status))
          {
             Status = LdrLoadDll(NULL,
                                 LDRP_PROCESS_CREATION_TIME,
                                 &DllName,
                                 &BaseAddress);
	     if (NT_SUCCESS(Status))
	       {
	         Status = LdrFindEntryForName (&DllName, &Module, FALSE);
	       }
	  }
        RtlFreeUnicodeString (&DllName);
        if (!NT_SUCCESS(Status))
          {
            DPRINT1("LdrFixupForward: failed to load %s\n", NameBuffer);
            return NULL;
          }

        DPRINT("BaseAddress: %p\n", Module->BaseAddress);
        
        return LdrGetExportByName(Module->BaseAddress, p+1, -1);
     }

   return NULL;
}


/**********************************************************************
 * NAME                                                         LOCAL
 *      LdrGetExportByOrdinal
 *      
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */
static PVOID
LdrGetExportByOrdinal (
        PVOID   BaseAddress,	
        ULONG   Ordinal
        )
{
        PIMAGE_EXPORT_DIRECTORY ExportDir;
        ULONG                   ExportDirSize;
        PDWORD                  * ExFunctions;
        USHORT                  * ExOrdinals;
        PVOID                   Function;

        ExportDir = (PIMAGE_EXPORT_DIRECTORY)
                RtlImageDirectoryEntryToData (BaseAddress,
                                              TRUE,
                                              IMAGE_DIRECTORY_ENTRY_EXPORT,
                                              &ExportDirSize);


        ExOrdinals = (USHORT *)
                RVA(
                        BaseAddress,
                        ExportDir->AddressOfNameOrdinals
                        );
        ExFunctions = (PDWORD *)
                RVA(
                        BaseAddress,
                        ExportDir->AddressOfFunctions
                        );
        DPRINT(
                "LdrGetExportByOrdinal(Ordinal %d) = %x\n",
                Ordinal,
                RVA(BaseAddress, ExFunctions[Ordinal - ExportDir->Base] )
                );

        Function = RVA(BaseAddress, ExFunctions[Ordinal - ExportDir->Base] );

        if (((ULONG)Function >= (ULONG)ExportDir) &&
            ((ULONG)Function < (ULONG)ExportDir + (ULONG)ExportDirSize))
          {
             DPRINT("Forward: %s\n", (PCHAR)Function);
             Function = LdrFixupForward((PCHAR)Function);
          }

        return Function;
}


/**********************************************************************
 * NAME                                                         LOCAL
 *      LdrGetExportByName
 *      
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *  AddressOfNames and AddressOfNameOrdinals are paralell tables, 
 *  both with NumberOfNames entries.
 *
 */
static PVOID
LdrGetExportByName(PVOID BaseAddress,
                   PUCHAR SymbolName,
                   WORD Hint)
{
   PIMAGE_EXPORT_DIRECTORY      ExportDir;
   PDWORD                       * ExFunctions;
   PDWORD                       * ExNames;
   USHORT                       * ExOrdinals;
   ULONG                        i;
   PVOID                        ExName;
   ULONG                        Ordinal;
   PVOID                        Function;
   LONG minn, maxn;
   ULONG ExportDirSize;
   
   DPRINT("LdrGetExportByName %x %s %hu\n", BaseAddress, SymbolName, Hint);

   ExportDir = (PIMAGE_EXPORT_DIRECTORY)
     RtlImageDirectoryEntryToData(BaseAddress,
                                  TRUE,
                                  IMAGE_DIRECTORY_ENTRY_EXPORT,
                                  &ExportDirSize);
   if (ExportDir == NULL)
     {
        DPRINT1("LdrGetExportByName(): no export directory!\n");
        return NULL;
     }


   //The symbol names may be missing entirely
   if (ExportDir->AddressOfNames == 0)
   {
      DPRINT("LdrGetExportByName(): symbol names missing entirely\n");  
      return NULL;
   }

   /*
    * Get header pointers
    */
   ExNames = (PDWORD *)RVA(BaseAddress,
                           ExportDir->AddressOfNames);
   ExOrdinals = (USHORT *)RVA(BaseAddress,
                              ExportDir->AddressOfNameOrdinals);
   ExFunctions = (PDWORD *)RVA(BaseAddress,
                               ExportDir->AddressOfFunctions);
   
   /*
    * Check the hint first
    */
   if (Hint < ExportDir->NumberOfNames)
     {
        ExName = RVA(BaseAddress, ExNames[Hint]);
        if (strcmp(ExName, SymbolName) == 0)
          {
             Ordinal = ExOrdinals[Hint];
             Function = RVA(BaseAddress, ExFunctions[Ordinal]);
             if (((ULONG)Function >= (ULONG)ExportDir) &&
                 ((ULONG)Function < (ULONG)ExportDir + (ULONG)ExportDirSize))
               {
                  DPRINT("Forward: %s\n", (PCHAR)Function);
                  Function = LdrFixupForward((PCHAR)Function);
		  if (Function == NULL)
		    {
                      DPRINT1("LdrGetExportByName(): failed to find %s\n",SymbolName);
		    } 
		  return Function;
               }
             if (Function != NULL)
               return Function;
          }
     }
   
   /*
    * Try a binary search first
    */
   minn = 0;
   maxn = ExportDir->NumberOfNames - 1;
   while (minn <= maxn)
     {
        LONG mid;
        LONG res;

        mid = (minn + maxn) / 2;

        ExName = RVA(BaseAddress, ExNames[mid]);
        res = strcmp(ExName, SymbolName);
        if (res == 0)
          {
             Ordinal = ExOrdinals[mid];
             Function = RVA(BaseAddress, ExFunctions[Ordinal]);
             if (((ULONG)Function >= (ULONG)ExportDir) &&
                 ((ULONG)Function < (ULONG)ExportDir + (ULONG)ExportDirSize))
               {
                  DPRINT("Forward: %s\n", (PCHAR)Function);
                  Function = LdrFixupForward((PCHAR)Function);
		  if (Function == NULL)
		    {
                      DPRINT1("LdrGetExportByName(): failed to find %s\n",SymbolName);
		    } 
		  return Function;
               }
             if (Function != NULL)
               return Function;
          }
        else if (minn == maxn)
          {
             DPRINT("LdrGetExportByName(): binary search failed\n");
             break;
          }
        else if (res > 0)
          {
             maxn = mid - 1;
          }
        else
          {
             minn = mid + 1;
          }
     }
   
   /*
    * Fall back on a linear search
    */
   DPRINT("LdrGetExportByName(): Falling back on a linear search of export table\n");
   for (i = 0; i < ExportDir->NumberOfNames; i++)
     {
        ExName = RVA(BaseAddress, ExNames[i]);
        if (strcmp(ExName,SymbolName) == 0)
          {
             Ordinal = ExOrdinals[i];
             Function = RVA(BaseAddress, ExFunctions[Ordinal]);
             DPRINT("%x %x %x\n", Function, ExportDir, ExportDir + ExportDirSize);
             if (((ULONG)Function >= (ULONG)ExportDir) &&
                 ((ULONG)Function < (ULONG)ExportDir + (ULONG)ExportDirSize))
               {
                  DPRINT("Forward: %s\n", (PCHAR)Function);
                  Function = LdrFixupForward((PCHAR)Function);
	       }
	     if (Function == NULL)
	       {
	         break;
	       }
	     return Function;
          }
     }
   DPRINT1("LdrGetExportByName(): failed to find %s\n",SymbolName);
   return (PVOID)NULL;
}


/**********************************************************************
 * NAME                                                         LOCAL
 *      LdrPerformRelocations
 *      
 * DESCRIPTION
 *      Relocate a DLL's memory image.
 *      
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */
static NTSTATUS LdrPerformRelocations (PIMAGE_NT_HEADERS        NTHeaders,
                                       PVOID                    ImageBase)
{
  USHORT                        NumberOfEntries;
  PUSHORT                       pValue16;
  ULONG                 RelocationRVA;
  ULONG                 Delta32;
  ULONG                 Offset;
  PULONG                        pValue32;
  PRELOCATION_DIRECTORY RelocationDir;
  PRELOCATION_ENTRY     RelocationBlock;
  int                   i;
  PIMAGE_DATA_DIRECTORY RelocationDDir;
  ULONG OldProtect;
  ULONG OldProtect2;
  NTSTATUS Status;
  PIMAGE_SECTION_HEADER Sections;
  ULONG MaxExtend;

  if (NTHeaders->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
    {
      return STATUS_UNSUCCESSFUL;
    }

  Sections = 
    (PIMAGE_SECTION_HEADER)((PVOID)NTHeaders + sizeof(IMAGE_NT_HEADERS));
  MaxExtend = 0;
  for (i = 0; i < NTHeaders->FileHeader.NumberOfSections; i++)
    {
      if (!(Sections[i].Characteristics & IMAGE_SECTION_NOLOAD))
        {
          ULONG Extend;
          Extend = 
            (ULONG)(Sections[i].VirtualAddress + Sections[i].Misc.VirtualSize);
          MaxExtend = max(MaxExtend, Extend);
        }
    }
  
  RelocationDDir = 
    &NTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
  RelocationRVA = RelocationDDir->VirtualAddress;

  if (RelocationRVA)
    {
      RelocationDir = 
        (PRELOCATION_DIRECTORY)((PCHAR)ImageBase + RelocationRVA);

      while (RelocationDir->SizeOfBlock)
        {
          if (RelocationDir->VirtualAddress > MaxExtend)
            {
              RelocationRVA += RelocationDir->SizeOfBlock;
              RelocationDir = 
                (PRELOCATION_DIRECTORY) (ImageBase + RelocationRVA);
              continue;
            }

          Delta32 = (ULONG)(ImageBase - NTHeaders->OptionalHeader.ImageBase);
          RelocationBlock = 
            (PRELOCATION_ENTRY) (RelocationRVA + ImageBase + 
                                 sizeof (RELOCATION_DIRECTORY));          
          NumberOfEntries = 
            RelocationDir->SizeOfBlock - sizeof (RELOCATION_DIRECTORY);
          NumberOfEntries = NumberOfEntries / sizeof (RELOCATION_ENTRY);

          Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                          ImageBase + 
                                          RelocationDir->VirtualAddress,
                                          PAGE_SIZE,
                                          PAGE_READWRITE,
                                          &OldProtect);
          if (!NT_SUCCESS(Status))
            {
              DPRINT1("Failed to unprotect relocation target.\n");
              return(Status);
            }

          if (RelocationDir->VirtualAddress + PAGE_SIZE < MaxExtend)
            {
          	  Status = NtProtectVirtualMemory(NtCurrentProcess(),
          					  ImageBase + 
          					  RelocationDir->VirtualAddress + PAGE_SIZE,
          					  PAGE_SIZE,
          					  PAGE_READWRITE,
          					  &OldProtect2);
          	  if (!NT_SUCCESS(Status))
          	    {
          	      DPRINT1("Failed to unprotect relocation target (2).\n");
                  NtProtectVirtualMemory(NtCurrentProcess(),
                                        ImageBase + 
                                        RelocationDir->VirtualAddress,
                                        PAGE_SIZE,
                                        OldProtect,
                                        &OldProtect);
          	      return(Status);
          	    }
              }
                
          for (i = 0; i < NumberOfEntries; i++)
            {
              Offset = (RelocationBlock[i].TypeOffset & 0xfff);
              Offset += (ULONG)(RelocationDir->VirtualAddress + ImageBase);

              /*
               * What kind of relocations should we perform
               * for the current entry?
               */
              switch (RelocationBlock[i].TypeOffset >> 12)
                {
                case TYPE_RELOC_ABSOLUTE:
                  break;
                  
                case TYPE_RELOC_HIGH:
                  pValue16 = (PUSHORT)Offset;
                  *pValue16 += Delta32 >> 16;
                  break;
                  
                case TYPE_RELOC_LOW:
                  pValue16 = (PUSHORT)Offset;
                  *pValue16 += Delta32 & 0xffff;
                  break;
                  
                case TYPE_RELOC_HIGHLOW:
                  pValue32 = (PULONG)Offset;
                  *pValue32 += Delta32;
                  break;
                          
                case TYPE_RELOC_HIGHADJ:
                  /* FIXME: do the highadjust fixup  */
                  DPRINT("TYPE_RELOC_HIGHADJ fixup not implemented, sorry\n");
                  return(STATUS_UNSUCCESSFUL);
                  
                default:
                  DPRINT("unexpected fixup type\n");
                  return STATUS_UNSUCCESSFUL;
                }             
            }

          Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                          ImageBase + 
                                          RelocationDir->VirtualAddress,
                                          PAGE_SIZE,
                                          OldProtect,
                                          &OldProtect);
          if (!NT_SUCCESS(Status))
            {
              DPRINT1("Failed to protect relocation target.\n");
              return(Status);
            }

          if (RelocationDir->VirtualAddress + PAGE_SIZE < MaxExtend)
            {
          	  Status = NtProtectVirtualMemory(NtCurrentProcess(),
          					  ImageBase + 
          					  RelocationDir->VirtualAddress + PAGE_SIZE,
          					  PAGE_SIZE,
          					  OldProtect2,
          					  &OldProtect2);
          	  if (!NT_SUCCESS(Status))
          	    {
          	      DPRINT1("Failed to protect relocation target2.\n");
          	      return(Status);
          	    }
            }

          RelocationRVA += RelocationDir->SizeOfBlock;
          RelocationDir = 
            (PRELOCATION_DIRECTORY) (ImageBase + RelocationRVA);
        }
    }
  return STATUS_SUCCESS;
}
 
static NTSTATUS 
LdrpGetOrLoadModule(PWCHAR SerachPath,
		    PCHAR Name, 
		    PLDR_MODULE* Module, 
		    BOOL Load)
{
   UNICODE_STRING DllName;
   NTSTATUS Status;

   DPRINT("LdrpGetOrLoadModule() called for %s\n", Name);

   RtlCreateUnicodeStringFromAsciiz (&DllName, Name);
	   
   Status = LdrFindEntryForName (&DllName, Module, Load);
   if (Load && !NT_SUCCESS(Status))
     {
       Status = LdrpLoadModule(SerachPath, 
	                       NtCurrentPeb()->Ldr->Initialized ? 0 : LDRP_PROCESS_CREATION_TIME, 
			       &DllName, 
			       Module);
       if (NT_SUCCESS(Status))
         {
	   Status = LdrFindEntryForName (&DllName, Module, FALSE);
	 }
       if (!NT_SUCCESS(Status))
         {
           DPRINT1("failed to load %wZ\n", &DllName);
         }
     }
   RtlFreeUnicodeString (&DllName);
   return Status;
}

static NTSTATUS
LdrpProcessImportDirectory(PLDR_MODULE Module, 
			   PLDR_MODULE ImportedModule,
			   PCHAR ImportedName)
{
   PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;
   NTSTATUS Status;
   PVOID* ImportAddressList;
   PULONG FunctionNameList;
   DWORD pName;
   WORD pHint;
   PVOID IATBase;
   ULONG OldProtect;
   ULONG Ordinal;
   ULONG IATSize;
   PCHAR Name;

   DPRINT("LdrpProcessImportDirectory(%x '%wZ', %x '%wZ', %x '%s')\n",
          Module, &Module->BaseDllName, ImportedModule, 
	  &ImportedModule->BaseDllName, ImportedName, ImportedName);

   ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)
                              RtlImageDirectoryEntryToData(Module->BaseAddress, 
			                                   TRUE, 
							   IMAGE_DIRECTORY_ENTRY_IMPORT, 
							   NULL);
   if (ImportModuleDirectory == NULL)
     {
       return STATUS_UNSUCCESSFUL;
     }

   while (ImportModuleDirectory->dwRVAModuleName)
     {
       Name = (PCHAR)Module->BaseAddress + ImportModuleDirectory->dwRVAModuleName;
       if (0 == _stricmp(Name, ImportedName))
         {

           /* Get the import address list. */
           ImportAddressList = (PVOID *)(Module->BaseAddress + ImportModuleDirectory->dwRVAFunctionAddressList);

           /* Get the list of functions to import. */
           if (ImportModuleDirectory->dwRVAFunctionNameList != 0)
             {
               FunctionNameList = (PULONG) (Module->BaseAddress + ImportModuleDirectory->dwRVAFunctionNameList);
             }
           else
             {
               FunctionNameList = (PULONG)(Module->BaseAddress + ImportModuleDirectory->dwRVAFunctionAddressList);
             }

           /* Get the size of IAT. */
           IATSize = 0;
           while (FunctionNameList[IATSize] != 0L)
             {
               IATSize++;
             }

          /* Unprotect the region we are about to write into. */
          IATBase = (PVOID)ImportAddressList;
          Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                          IATBase,
                                          IATSize * sizeof(PVOID*),
                                          PAGE_READWRITE,
                                          &OldProtect);
          if (!NT_SUCCESS(Status))
            {
              DPRINT1("Failed to unprotect IAT.\n");
              return(Status);
            }
         
          /* Walk through function list and fixup addresses. */
         
          while (*FunctionNameList != 0L)
            {
              if ((*FunctionNameList) & 0x80000000)
                {
                  Ordinal = (*FunctionNameList) & 0x7fffffff;
                  *ImportAddressList = LdrGetExportByOrdinal(ImportedModule->BaseAddress, Ordinal);
                }
              else
                {
                  pName = (DWORD) (Module->BaseAddress + *FunctionNameList + 2);
                  pHint = *(PWORD)(Module->BaseAddress + *FunctionNameList);

                  *ImportAddressList = LdrGetExportByName(ImportedModule->BaseAddress, (PUCHAR)pName, pHint);
                  if ((*ImportAddressList) == NULL)
                    {
                      DPRINT1("Failed to import %s\n", pName);
                      return STATUS_UNSUCCESSFUL;
                    }
                }
              ImportAddressList++;
              FunctionNameList++;
           }

           /* Protect the region we are about to write into. */
           Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                           IATBase,
                                           IATSize * sizeof(PVOID*),
                                           OldProtect,
                                           &OldProtect);
           if (!NT_SUCCESS(Status))
             {
               DPRINT1("Failed to protect IAT.\n");
               return(Status);
             }
	 }
       ImportModuleDirectory++;
     }

   return STATUS_SUCCESS;
}

NTSTATUS LdrpAdjustImportDirectory(PLDR_MODULE Module, 
				   PLDR_MODULE ImportedModule,
				   PUCHAR ImportedName)
{
   PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;
   NTSTATUS Status;
   PVOID* ImportAddressList;
   PVOID Start;
   PVOID End;
   PULONG FunctionNameList;
   PVOID IATBase;
   ULONG OldProtect;
   ULONG Offset;
   ULONG IATSize;
   PIMAGE_NT_HEADERS NTHeaders;
   PCHAR Name;

   DPRINT("LdrpAdjustImportDirectory(Module %x '%wZ', %x '%wZ', %x '%s')\n",
          Module, &Module->BaseDllName, ImportedModule, &ImportedModule->BaseDllName, ImportedName);

   ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)
                              RtlImageDirectoryEntryToData(Module->BaseAddress, 
			                                   TRUE, 
							   IMAGE_DIRECTORY_ENTRY_IMPORT, 
							   NULL);
   if (ImportModuleDirectory == NULL)
     {
       return STATUS_UNSUCCESSFUL;
     }

   while (ImportModuleDirectory->dwRVAModuleName)
     {
       Name = (PCHAR)Module->BaseAddress + ImportModuleDirectory->dwRVAModuleName;
       if (0 == _stricmp(Name, ImportedName))
         {

           /* Get the import address list. */
           ImportAddressList = (PVOID *)(Module->BaseAddress + ImportModuleDirectory->dwRVAFunctionAddressList);

           /* Get the list of functions to import. */
           if (ImportModuleDirectory->dwRVAFunctionNameList != 0)
             {
               FunctionNameList = (PULONG) (Module->BaseAddress + ImportModuleDirectory->dwRVAFunctionNameList);
             }
           else
             {
               FunctionNameList = (PULONG)(Module->BaseAddress + ImportModuleDirectory->dwRVAFunctionAddressList);
             }

           /* Get the size of IAT. */
           IATSize = 0;
           while (FunctionNameList[IATSize] != 0L)
             {
               IATSize++;
             }

          /* Unprotect the region we are about to write into. */
          IATBase = (PVOID)ImportAddressList;
          Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                          IATBase,
                                          IATSize * sizeof(PVOID*),
                                          PAGE_READWRITE,
                                          &OldProtect);
          if (!NT_SUCCESS(Status))
            {
              DPRINT1("Failed to unprotect IAT.\n");
              return(Status);
            }
         
          NTHeaders = RtlImageNtHeader (ImportedModule->BaseAddress);
          Start = (PVOID)NTHeaders->OptionalHeader.ImageBase;
          End = Start + ImportedModule->SizeOfImage;
          Offset = ImportedModule->BaseAddress - Start;

          /* Walk through function list and fixup addresses. */
          while (*FunctionNameList != 0L)
            {
              if (*ImportAddressList >= Start && *ImportAddressList < End)
                 {
	           (*ImportAddressList) += Offset;
	         }
              ImportAddressList++;
              FunctionNameList++;
            }

            /* Protect the region we are about to write into. */
            Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                            IATBase,
                                            IATSize * sizeof(PVOID*),
                                            OldProtect,
                                            &OldProtect);
            if (!NT_SUCCESS(Status))
              {
                DPRINT1("Failed to protect IAT.\n");
                return(Status);
              }
	 }
       ImportModuleDirectory++;
     }
   return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME                                                         LOCAL
 *      LdrFixupImports
 *      
 * DESCRIPTION
 *      Compute the entry point for every symbol the DLL imports
 *      from other modules.
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 */
static NTSTATUS 
LdrFixupImports(IN PWSTR SearchPath OPTIONAL,
		IN PLDR_MODULE Module)
{
   PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;
   PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectoryCurrent;
   PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundImportDescriptor;
   PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundImportDescriptorCurrent;
   PIMAGE_TLS_DIRECTORY TlsDirectory;
   ULONG TlsSize;
   NTSTATUS Status;
   PLDR_MODULE ImportedModule;
   PCHAR ImportedName;
   
   DPRINT("LdrFixupImports(SearchPath %x, Module %x)\n", SearchPath, Module);
   
   /* Check for tls data */
   TlsDirectory = (PIMAGE_TLS_DIRECTORY)
                     RtlImageDirectoryEntryToData(Module->BaseAddress, 
			                          TRUE, 
						  IMAGE_DIRECTORY_ENTRY_TLS, 
						  NULL);
   if (TlsDirectory)
     {
       TlsSize = TlsDirectory->EndAddressOfRawData 
	           - TlsDirectory->StartAddressOfRawData
		   + TlsDirectory->SizeOfZeroFill;
       if (TlsSize > 0 &&
	   NtCurrentPeb()->Ldr->Initialized)
         {
           TRACE_LDR("Trying to load dynamicly %wZ which contains a tls directory\n",
	             &Module->BaseDllName);
	   return STATUS_UNSUCCESSFUL;
	 }
     }
   /*
    * Process each import module.
    */
   ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)
                              RtlImageDirectoryEntryToData(Module->BaseAddress, 
			                                   TRUE, 
							   IMAGE_DIRECTORY_ENTRY_IMPORT, 
							   NULL);

   BoundImportDescriptor = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)
                              RtlImageDirectoryEntryToData(Module->BaseAddress, 
			                                   TRUE, 
							   IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT, 
							   NULL);

   if (BoundImportDescriptor != NULL && ImportModuleDirectory == NULL)
     {
       DPRINT1("%wZ has only a bound import directory\n", &Module->BaseDllName);
       return STATUS_UNSUCCESSFUL;
     }
   if (BoundImportDescriptor)
     {
       DPRINT("BoundImportDescriptor %x\n", BoundImportDescriptor);

       BoundImportDescriptorCurrent = BoundImportDescriptor;
       while (BoundImportDescriptorCurrent->OffsetModuleName)
         {
	   ImportedName = (PCHAR)BoundImportDescriptor + BoundImportDescriptorCurrent->OffsetModuleName;
	   TRACE_LDR("%wZ bound to %s\n", &Module->BaseDllName, ImportedName);
	   Status = LdrpGetOrLoadModule(SearchPath, ImportedName, &ImportedModule, TRUE);
	   if (!NT_SUCCESS(Status))
	     {
	       DPRINT1("failed to load %s\n", ImportedName);
	       return Status;
	     }
	   if (Module == ImportedModule)
	     {
	       LdrpDecrementLoadCount(Module, FALSE);
	     }
	   if (ImportedModule->TimeDateStamp != BoundImportDescriptorCurrent->TimeDateStamp)
	     {
	       TRACE_LDR("%wZ has stale binding to %wZ\n", 
		         &Module->BaseDllName, &ImportedModule->BaseDllName);
	       Status = LdrpProcessImportDirectory(Module, ImportedModule, ImportedName);
	       if (!NT_SUCCESS(Status))
	         {
		   DPRINT1("failed to import %s\n", ImportedName);
		   return Status;
		 }
	     }
	   else
	     {
	       BOOL WrongForwarder;
	       WrongForwarder = FALSE;
	       if (ImportedModule->Flags & IMAGE_NOT_AT_BASE)
	         {
	           TRACE_LDR("%wZ has stale binding to %s\n", 
		             &Module->BaseDllName, ImportedName);
		 }
	       else
	         {
	           TRACE_LDR("%wZ has correct binding to %wZ\n", 
		           &Module->BaseDllName, &ImportedModule->BaseDllName);
		 }
	       if (BoundImportDescriptorCurrent->NumberOfModuleForwarderRefs)
	         {
	           PIMAGE_BOUND_FORWARDER_REF BoundForwarderRef;
	           ULONG i;
	           PLDR_MODULE ForwarderModule;
		   PUCHAR ForwarderName;

                   BoundForwarderRef = (PIMAGE_BOUND_FORWARDER_REF)(BoundImportDescriptorCurrent + 1);
	           for (i = 0; i < BoundImportDescriptorCurrent->NumberOfModuleForwarderRefs; i++, BoundForwarderRef++)
	             {
		       ForwarderName = (PCHAR)BoundImportDescriptor + BoundForwarderRef->OffsetModuleName;
		       TRACE_LDR("%wZ bound to %s via forwardes from %s\n",
			         &Module->BaseDllName, ForwarderName, ImportedName);
		       Status = LdrpGetOrLoadModule(SearchPath, ForwarderName, &ForwarderModule, TRUE);
		       if (!NT_SUCCESS(Status))
		         {
	                   DPRINT1("failed to load %s\n", ForwarderName);
			   return Status;
			 }
	               if (Module == ImportedModule)
	                 {
	                   LdrpDecrementLoadCount(Module, FALSE);
	                 }
		       if (ForwarderModule->TimeDateStamp != BoundForwarderRef->TimeDateStamp ||
			   ForwarderModule->Flags & IMAGE_NOT_AT_BASE)
		         {
			   TRACE_LDR("%wZ has stale binding to %s\n",
			             &Module->BaseDllName, ForwarderName);
			   WrongForwarder = TRUE;
			 }
		       else
		         {
			   TRACE_LDR("%wZ has correct binding to %s\n",
			             &Module->BaseDllName, ForwarderName);
			 }
		     }
		 }
	       if (WrongForwarder ||
	           ImportedModule->Flags & IMAGE_NOT_AT_BASE)
		 {
                   Status = LdrpProcessImportDirectory(Module, ImportedModule, ImportedName);
	           if (!NT_SUCCESS(Status))
	             {
		       DPRINT1("failed to import %s\n", ImportedName);
		       return Status;
		     }
		 }
	       else if (ImportedModule->Flags & IMAGE_NOT_AT_BASE)
		 {
		   TRACE_LDR("Adjust imports for %s from %wZ\n",
		             ImportedName, &Module->BaseDllName);
                   Status = LdrpAdjustImportDirectory(Module, ImportedModule, ImportedName);
		   if (!NT_SUCCESS(Status))
		   {
		     DPRINT1("failed to adjust import entries for %s\n", ImportedName);
		     return Status;
		   }
		 }
	       else if (WrongForwarder)
		 {
		   /*
		    * FIXME:
		    *   Update only forwarders
		    */
		   TRACE_LDR("Stale BIND %s from %wZ\n", 
		             ImportedName, &Module->BaseDllName);
                   Status = LdrpProcessImportDirectory(Module, ImportedModule, ImportedName);
	           if (!NT_SUCCESS(Status))
	             {
		       DPRINT1("faild to import %s\n", ImportedName);
		       return Status;
		     }
		 }
	       else
		 {
		   /* nothing to do */
		 }
	     }
           BoundImportDescriptorCurrent += BoundImportDescriptorCurrent->NumberOfModuleForwarderRefs + 1;
	 }
     }
   else if (ImportModuleDirectory)
     {
       DPRINT("ImportModuleDirectory %x\n", ImportModuleDirectory);

       ImportModuleDirectoryCurrent = ImportModuleDirectory;
       while (ImportModuleDirectoryCurrent->dwRVAModuleName)
         {
	   ImportedName = (PCHAR)Module->BaseAddress + ImportModuleDirectoryCurrent->dwRVAModuleName;
	   TRACE_LDR("%wZ imports functions from %s\n", &Module->BaseDllName, ImportedName);

           Status = LdrpGetOrLoadModule(SearchPath, ImportedName, &ImportedModule, TRUE);
           if (!NT_SUCCESS(Status))
             {
               DPRINT1("failed to load %s\n", ImportedName);
               return Status;
             }
	   if (Module == ImportedModule)
	     {
	       LdrpDecrementLoadCount(Module, FALSE);
	     }
	   TRACE_LDR("Initializing imports for %wZ from %s\n",
	             &Module->BaseDllName, ImportedName);
           Status = LdrpProcessImportDirectory(Module, ImportedModule, ImportedName);
	   if (!NT_SUCCESS(Status))
	     {
	       DPRINT1("failed to import %s\n", ImportedName);
	       return Status;
	     }
	   ImportModuleDirectoryCurrent++;
	 }
     }
   if (TlsDirectory && TlsSize > 0)
     {
       LdrpAcquireTlsSlot(Module, TlsSize, FALSE);
     }

   return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME
 *      LdrPEStartup
 *
 * DESCRIPTION
 *      1. Relocate, if needed the EXE.
 *      2. Fixup any imported symbol.
 *      3. Compute the EXE's entry point.
 *
 * ARGUMENTS
 *      ImageBase
 *              Address at which the EXE's image
 *              is loaded.
 *              
 *      SectionHandle
 *              Handle of the section that contains
 *              the EXE's image.
 *
 * RETURN VALUE
 *      NULL on error; otherwise the entry point
 *      to call for initializing the DLL.
 *
 * REVISIONS
 *
 * NOTE
 *      04.01.2004 hb Previous this function was used for all images (dll + exe).
 *                    Currently the function is only used for the exe.
 */
PEPFUNC LdrPEStartup (PVOID  ImageBase,
                      HANDLE SectionHandle,
                      PLDR_MODULE* Module,
                      PWSTR FullDosName)
{
   NTSTATUS             Status;
   PEPFUNC              EntryPoint = NULL;
   PIMAGE_DOS_HEADER    DosHeader;
   PIMAGE_NT_HEADERS    NTHeaders;
   PLDR_MODULE tmpModule;

   DPRINT("LdrPEStartup(ImageBase %x SectionHandle %x)\n",
           ImageBase, (ULONG)SectionHandle);

   /*
    * Overlay DOS and WNT headers structures
    * to the DLL's image.
    */
   DosHeader = (PIMAGE_DOS_HEADER) ImageBase;
   NTHeaders = (PIMAGE_NT_HEADERS) (ImageBase + DosHeader->e_lfanew);

   /*
    * If the base address is different from the
    * one the DLL is actually loaded, perform any
    * relocation.
    */
   if (ImageBase != (PVOID) NTHeaders->OptionalHeader.ImageBase)
     {
       DPRINT("LDR: Performing relocations\n");
       Status = LdrPerformRelocations(NTHeaders, ImageBase);
       if (!NT_SUCCESS(Status))
         {
           DPRINT1("LdrPerformRelocations() failed\n");
           return NULL;
         }
     }

   if (Module != NULL)
     {
       *Module = LdrAddModuleEntry(ImageBase, NTHeaders, FullDosName);
       (*Module)->SectionHandle = SectionHandle;
     }
   else
     {
       Module = &tmpModule;
       Status = LdrFindEntryForAddress(ImageBase, Module);
       if (!NT_SUCCESS(Status))
         {
	   return NULL;
	 }
     }
   
   if (ImageBase != (PVOID) NTHeaders->OptionalHeader.ImageBase)
     {
       (*Module)->Flags |= IMAGE_NOT_AT_BASE;
     }

   /*
    * If the DLL's imports symbols from other
    * modules, fixup the imported calls entry points.
    */
   DPRINT("About to fixup imports\n");
   Status = LdrFixupImports(NULL, *Module);
   if (!NT_SUCCESS(Status))
     {
       DPRINT1("LdrFixupImports() failed for %wZ\n", &(*Module)->BaseDllName);
       return NULL;
     }
   DPRINT("Fixup done\n");
   RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);
   Status = LdrpInitializeTlsForProccess();
   if (NT_SUCCESS(Status))
     {
       Status = LdrpAttachProcess();
     }
   if (NT_SUCCESS(Status))
     {
       LdrpTlsCallback(*Module, DLL_PROCESS_ATTACH);
     }


   RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
   if (!NT_SUCCESS(Status))
     {
       CHECKPOINT1;
       return NULL;
     }

   /*
    * Compute the DLL's entry point's address.
    */
   DPRINT("ImageBase = %x\n",(ULONG)ImageBase);
   DPRINT("AddressOfEntryPoint = %x\n",(ULONG)NTHeaders->OptionalHeader.AddressOfEntryPoint);
   if (NTHeaders->OptionalHeader.AddressOfEntryPoint != 0)
     {
        EntryPoint = (PEPFUNC) (ImageBase
                           + NTHeaders->OptionalHeader.AddressOfEntryPoint);
     }
   DPRINT("LdrPEStartup() = %x\n",EntryPoint);
   return EntryPoint;
}

static NTSTATUS 
LdrpLoadModule(IN PWSTR SearchPath OPTIONAL,
	       IN ULONG LoadFlags,
	       IN PUNICODE_STRING Name,
	       PLDR_MODULE *Module)
{
    UNICODE_STRING AdjustedName;
    UNICODE_STRING FullDosName;
    NTSTATUS Status;
    PLDR_MODULE tmpModule;
    HANDLE SectionHandle;
    ULONG ViewSize;
    PVOID ImageBase;
    PIMAGE_NT_HEADERS NtHeaders;

    if (Module == NULL)
      {
        Module = &tmpModule;
      }
    /* adjust the full dll name */
    LdrAdjustDllName(&AdjustedName, Name, FALSE);

    DPRINT("%wZ\n", &AdjustedName);

    /* Test if dll is already loaded */
    Status = LdrFindEntryForName(&AdjustedName, Module, TRUE);
    if (NT_SUCCESS(Status))
      {
        RtlFreeUnicodeString(&AdjustedName);
      }
    else
      {
        /* Open or create dll image section */
        Status = LdrpMapKnownDll(&AdjustedName, &FullDosName, &SectionHandle);
	if (!NT_SUCCESS(Status))
	  {
            Status = LdrpMapDllImageFile(SearchPath, &AdjustedName, &FullDosName, &SectionHandle);
	  }
      	if (!NT_SUCCESS(Status))
	  {
            DPRINT1("Failed to create or open dll section of '%wZ' (Status %lx)\n", &AdjustedName, Status);
            RtlFreeUnicodeString(&AdjustedName);
            RtlFreeUnicodeString(&FullDosName);
            return Status;
	  }
        RtlFreeUnicodeString(&AdjustedName);
	/* Map the dll into the process */
	ViewSize = 0;
	ImageBase = 0;
        Status = NtMapViewOfSection(SectionHandle,
                                    NtCurrentProcess(),
                                    &ImageBase,
                                    0,
                                    0,
                                    NULL,
                                    &ViewSize,
                                    0,
                                    MEM_COMMIT,
                                    PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
          {
            DPRINT1("map view of section failed (Status %x)\n", Status);
            RtlFreeUnicodeString(&FullDosName);
            NtClose(SectionHandle);
            return(Status);
          }
        /* Get and check the NT headers */
        NtHeaders = RtlImageNtHeader(ImageBase);
        if (NtHeaders == NULL)
          {
            DPRINT1("RtlImageNtHeaders() failed\n");
            NtUnmapViewOfSection (NtCurrentProcess (), ImageBase);
            NtClose (SectionHandle);
            RtlFreeUnicodeString(&FullDosName);
            return STATUS_UNSUCCESSFUL;
          }
        /* If the base address is different from the
         * one the DLL is actually loaded, perform any
         * relocation. */
        if (ImageBase != (PVOID) NtHeaders->OptionalHeader.ImageBase)
          {
            DPRINT1("Performing relocations (%x -> %x)\n",
              NtHeaders->OptionalHeader.ImageBase, ImageBase);
            Status = LdrPerformRelocations(NtHeaders, ImageBase);
            if (!NT_SUCCESS(Status))
              {
                DPRINT1("LdrPerformRelocations() failed\n");
	        NtUnmapViewOfSection (NtCurrentProcess (), ImageBase);
                NtClose (SectionHandle);
                RtlFreeUnicodeString(&FullDosName);
                return STATUS_UNSUCCESSFUL;
	      }
          }
        *Module = LdrAddModuleEntry(ImageBase, NtHeaders, FullDosName.Buffer);
        (*Module)->SectionHandle = SectionHandle;
        if (ImageBase != (PVOID) NtHeaders->OptionalHeader.ImageBase)
          {
	    (*Module)->Flags |= IMAGE_NOT_AT_BASE;
	  }
        if (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL)
          {
	    (*Module)->Flags |= IMAGE_DLL;
	  }
        /* fixup the imported calls entry points */
        Status = LdrFixupImports(SearchPath, *Module);
        if (!NT_SUCCESS(Status))
          {
	    DPRINT1("LdrFixupImports failed for %wZ, status=%x\n", &(*Module)->BaseDllName, Status);
	    return Status;
	  }
#ifdef KDBG
        LdrpLoadUserModuleSymbols(*Module);
#endif
        RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);
        InsertTailList(&NtCurrentPeb()->Ldr->InInitializationOrderModuleList, 
	               &(*Module)->InInitializationOrderModuleList);
        RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);
      }
    return STATUS_SUCCESS;
}

static NTSTATUS 
LdrpUnloadModule(PLDR_MODULE Module, 
		 BOOL Unload)
{
   PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;
   PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundImportDescriptor;
   PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundImportDescriptorCurrent;
   PCHAR ImportedName;
   PLDR_MODULE ImportedModule;
   NTSTATUS Status;
   LONG LoadCount;


   if (Unload)
     {
       RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);
     }

   LoadCount = LdrpDecrementLoadCount(Module, Unload);

   TRACE_LDR("Unload %wZ, LoadCount %d\n", &Module->BaseDllName, LoadCount);
   
   if (LoadCount == 0)
     {
       /* ?????????????????? */
       CHECKPOINT1;
     }
   else if (LoadCount == 1)
     {
       BoundImportDescriptor = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)
                                 RtlImageDirectoryEntryToData(Module->BaseAddress, 
		                                              TRUE, 
							      IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT, 
							      NULL);
       if (BoundImportDescriptor)
        {
	  /* dereferencing all imported modules, use the bound import descriptor */
          BoundImportDescriptorCurrent = BoundImportDescriptor;
          while (BoundImportDescriptorCurrent->OffsetModuleName)
            {
	      ImportedName = (PCHAR)BoundImportDescriptor + BoundImportDescriptorCurrent->OffsetModuleName;
	      TRACE_LDR("%wZ trys to unload %s\n", &Module->BaseDllName, ImportedName);
              Status = LdrpGetOrLoadModule(NULL, ImportedName, &ImportedModule, FALSE);
	      if (!NT_SUCCESS(Status))
	        {
		  DPRINT1("unable to found imported modul %s\n", ImportedName);
	        }
	      else
	        {
		  if (Module != ImportedModule)
		    {
                      Status = LdrpUnloadModule(ImportedModule, FALSE);
                      if (!NT_SUCCESS(Status))
		        {
			  DPRINT1("unable to unload %s\n", ImportedName);
			}
		    }
		}
	      BoundImportDescriptorCurrent++;
	    }
         }
       else 
         {
           ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)
                                      RtlImageDirectoryEntryToData(Module->BaseAddress, 
			                                           TRUE, 
							           IMAGE_DIRECTORY_ENTRY_IMPORT, 
							           NULL);
           if (ImportModuleDirectory)
	     {
	       /* dereferencing all imported modules, use the import descriptor */
	       while (ImportModuleDirectory->dwRVAModuleName)
	         {
		   ImportedName = (PCHAR)Module->BaseAddress + ImportModuleDirectory->dwRVAModuleName;
		   TRACE_LDR("%wZ trys to unload %s\n", &Module->BaseDllName, ImportedName);
                   Status = LdrpGetOrLoadModule(NULL, ImportedName, &ImportedModule, FALSE);
		   if (!NT_SUCCESS(Status))
	             {
		       DPRINT1("unable to found imported modul %s\n", ImportedName);
	             }
	           else
	             {
		       if (Module != ImportedModule)
		         {
			   Status = LdrpUnloadModule(ImportedModule, FALSE);
		           if (!NT_SUCCESS(Status))
		             {
			       DPRINT1("unable to unload %s\n", ImportedName);
		             }
			 }
		     }
		   ImportModuleDirectory++;
		 }
	     }
         }
     }

   if (Unload)
     {
       LdrpDetachProcess(FALSE);
       RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);
     }
   return STATUS_SUCCESS;

}

/*
 * @implemented
 */
NTSTATUS STDCALL
LdrUnloadDll (IN PVOID BaseAddress)
{
   PLDR_MODULE Module;
   NTSTATUS Status;

   if (BaseAddress == NULL)
     return STATUS_SUCCESS;

   Status = LdrFindEntryForAddress(BaseAddress, &Module);
   if (NT_SUCCESS(Status))
     {
       TRACE_LDR("LdrUnloadDll, , unloading %wZ\n", &Module->BaseDllName);
       Status = LdrpUnloadModule(Module, TRUE);
     }
   return Status;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
LdrDisableThreadCalloutsForDll(IN PVOID BaseAddress)
{
    PLIST_ENTRY ModuleListHead;
    PLIST_ENTRY Entry;
    PLDR_MODULE Module;
    NTSTATUS Status;

    DPRINT("LdrDisableThreadCalloutsForDll (BaseAddress %x)\n", BaseAddress);

    Status = STATUS_DLL_NOT_FOUND;
    RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
    ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Entry = ModuleListHead->Flink;
    while (Entry != ModuleListHead) 
      {
        Module = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);

        DPRINT("BaseDllName %wZ BaseAddress %x\n", &Module->BaseDllName, Module->BaseAddress);

        if (Module->BaseAddress == BaseAddress) 
	  {
            if (Module->TlsIndex == -1) 
	      {
                Module->Flags |= DONT_CALL_FOR_THREAD;
                Status = STATUS_SUCCESS;
              }
            break;
          }
        Entry = Entry->Flink;
      }
    RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
LdrGetDllHandle(IN PWCHAR Path OPTIONAL,
                IN ULONG Unknown2,
                IN PUNICODE_STRING DllName,
                OUT PVOID* BaseAddress)
{
    PLDR_MODULE Module;
    NTSTATUS Status;

    TRACE_LDR("LdrGetDllHandle, searching for %wZ from %S\n", DllName, Path ? Path : L"");

    /* NULL is the current executable */
    if (DllName == NULL) 
      {
        *BaseAddress = ExeModule->BaseAddress;
        DPRINT("BaseAddress %x\n", *BaseAddress);
        return STATUS_SUCCESS;
      }

    Status = LdrFindEntryForName(DllName, &Module, FALSE);
    if (NT_SUCCESS(Status))
      {
        *BaseAddress = Module->BaseAddress;
        return STATUS_SUCCESS;
      }

    DPRINT("Failed to find dll %wZ\n", DllName);
    *BaseAddress = NULL;
    return STATUS_DLL_NOT_FOUND;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LdrGetProcedureAddress (IN PVOID BaseAddress,
                        IN PANSI_STRING Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress)
{
   if (Name && Name->Length)
     {
       TRACE_LDR("LdrGetProcedureAddress by NAME - %Z\n", Name);
     }
   else
     {
       TRACE_LDR("LdrGetProcedureAddress by ORDINAL - %d\n", Ordinal);
     }

   DPRINT("LdrGetProcedureAddress (BaseAddress %x Name %Z Ordinal %lu ProcedureAddress %x)\n",
          BaseAddress, Name, Ordinal, ProcedureAddress);

   if (Name && Name->Length)
     {
       /* by name */
       *ProcedureAddress = LdrGetExportByName(BaseAddress, Name->Buffer, 0xffff);
       if (*ProcedureAddress != NULL)
	 {
           return STATUS_SUCCESS;
         }
       DPRINT("LdrGetProcedureAddress: Can't resolve symbol '%Z'\n", Name);
     }
   else
     {
       /* by ordinal */
       Ordinal &= 0x0000FFFF;
       *ProcedureAddress = LdrGetExportByOrdinal(BaseAddress, (WORD)Ordinal);
       if (*ProcedureAddress)
         {
           return STATUS_SUCCESS;
         }
       DPRINT("LdrGetProcedureAddress: Can't resolve symbol @%d\n", Ordinal);
     }
   return STATUS_PROCEDURE_NOT_FOUND;
}

/**********************************************************************
 * NAME                                                         LOCAL
 *      LdrpDetachProcess
 *      
 * DESCRIPTION
 *      Unload dll's which are no longer referenced from others dll's
 *
 * ARGUMENTS
 *      none
 *
 * RETURN VALUE
 *      none
 *
 * REVISIONS
 *
 * NOTE
 *      The loader lock must be held on enty. 
 */
static VOID 
LdrpDetachProcess(BOOL UnloadAll)
{
   PLIST_ENTRY ModuleListHead;
   PLIST_ENTRY Entry;
   PLDR_MODULE Module;
   static ULONG CallingCount = 0;

   DPRINT("LdrpDetachProcess() called for %wZ\n",
           &ExeModule->BaseDllName);

   CallingCount++;

   ModuleListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
   Entry = ModuleListHead->Blink;
   while (Entry != ModuleListHead)
     {
       Module = CONTAINING_RECORD(Entry, LDR_MODULE, InInitializationOrderModuleList);
       if (((UnloadAll && Module->LoadCount <= 0) || Module->LoadCount == 0) &&
	   Module->Flags & ENTRY_PROCESSED &&
	   !(Module->Flags & UNLOAD_IN_PROGRESS))
         {
           Module->Flags |= UNLOAD_IN_PROGRESS;
	   if (Module == LdrpLastModule)
	     {
	       LdrpLastModule = NULL;
	     }
           if (Module->Flags & PROCESS_ATTACH_CALLED)
	     {
	       TRACE_LDR("Unload %wZ - Calling entry point at %x\n",
		         &Module->BaseDllName, Module->EntryPoint);
	       LdrpCallDllEntry(Module, DLL_PROCESS_DETACH, (PVOID)(Module->LoadCount == -1 ? 1 : 0));
	     }
	   else
	     {
               TRACE_LDR("Unload %wZ\n", &Module->BaseDllName);
	     }
	   Entry = ModuleListHead->Blink;
	 }
       else
         {
	   Entry = Entry->Blink;
	 }
     }
      
   if (CallingCount == 1)
     {
       Entry = ModuleListHead->Blink;
       while (Entry != ModuleListHead)
         {
           Module = CONTAINING_RECORD(Entry, LDR_MODULE, InInitializationOrderModuleList);
	   Entry = Entry->Blink;
	   if (Module->Flags & UNLOAD_IN_PROGRESS &&
	       ((UnloadAll && Module->LoadCount >= 0) || Module->LoadCount == 0))
	     {
               /* remove the module entry from the list */
               RemoveEntryList (&Module->InLoadOrderModuleList)
               RemoveEntryList (&Module->InInitializationOrderModuleList);

	       NtUnmapViewOfSection (NtCurrentProcess (), Module->BaseAddress);
               NtClose (Module->SectionHandle);

	       TRACE_LDR("%wZ unloaded\n", &Module->BaseDllName);

               RtlFreeUnicodeString (&Module->FullDllName);
               RtlFreeUnicodeString (&Module->BaseDllName);

               RtlFreeHeap (RtlGetProcessHeap (), 0, Module);
	     }
         }
     }
   CallingCount--;
   DPRINT("LdrpDetachProcess() done\n");
}

/**********************************************************************
 * NAME                                                         LOCAL
 *      LdrpAttachProcess
 *      
 * DESCRIPTION
 *      Initialize all dll's which are prepered for loading
 *
 * ARGUMENTS
 *      none
 *
 * RETURN VALUE
 *      status
 *
 * REVISIONS
 *
 * NOTE
 *      The loader lock must be held on entry.
 *
 */
static NTSTATUS
LdrpAttachProcess(VOID)
{
   PLIST_ENTRY ModuleListHead;
   PLIST_ENTRY Entry;
   PLDR_MODULE Module;
   BOOL Result;
   NTSTATUS Status;

   DPRINT("LdrpAttachProcess() called for %wZ\n",
          &ExeModule->BaseDllName);

   ModuleListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
   Entry = ModuleListHead->Flink;
   while (Entry != ModuleListHead)
     {
       Module = CONTAINING_RECORD(Entry, LDR_MODULE, InInitializationOrderModuleList);
       if (!(Module->Flags & (LOAD_IN_PROGRESS|UNLOAD_IN_PROGRESS|ENTRY_PROCESSED)))
	 {
	   Module->Flags |= LOAD_IN_PROGRESS;
           TRACE_LDR("%wZ loaded - Calling init routine at %x for process attaching\n",
	             &Module->BaseDllName, Module->EntryPoint);
	   Result = LdrpCallDllEntry(Module, DLL_PROCESS_ATTACH, (PVOID)(Module->LoadCount == -1 ? 1 : 0));
           if (!Result)
             {
	       Status = STATUS_DLL_INIT_FAILED;
	       break;
	     }
	   if (Module->Flags & IMAGE_DLL && Module->EntryPoint != 0)
	     {
	       Module->Flags |= PROCESS_ATTACH_CALLED|ENTRY_PROCESSED;
	     }
	   else
	     {
	       Module->Flags |= ENTRY_PROCESSED;
	     }
           Module->Flags &= ~LOAD_IN_PROGRESS;
	 }
       Entry = Entry->Flink;
     }

   DPRINT("LdrpAttachProcess() done\n");

   return Status;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
LdrShutdownProcess (VOID)
{
   LdrpDetachProcess(TRUE);
   return STATUS_SUCCESS;
}

/*
 * @implemented
 */

NTSTATUS 
LdrpAttachThread (VOID)
{
   PLIST_ENTRY ModuleListHead;
   PLIST_ENTRY Entry;
   PLDR_MODULE Module;
   NTSTATUS Status;

   DPRINT("LdrpAttachThread() called for %wZ\n",
          &ExeModule->BaseDllName);

   RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);

   Status = LdrpInitializeTlsForThread();

   if (NT_SUCCESS(Status))
     {

       ModuleListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
       Entry = ModuleListHead->Flink;

       while (Entry != ModuleListHead)
         {
           Module = CONTAINING_RECORD(Entry, LDR_MODULE, InInitializationOrderModuleList);
	   if (Module->Flags & PROCESS_ATTACH_CALLED &&
	       !(Module->Flags & DONT_CALL_FOR_THREAD) &&
               !(Module->Flags & UNLOAD_IN_PROGRESS))
	     {
               TRACE_LDR("%wZ - Calling entry point at %x for thread attaching\n", 
		         &Module->BaseDllName, Module->EntryPoint);
	       LdrpCallDllEntry(Module, DLL_THREAD_ATTACH, NULL);
             }
           Entry = Entry->Flink;
         }

       Entry = NtCurrentPeb()->Ldr->InLoadOrderModuleList.Flink;
       Module = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);
       LdrpTlsCallback(Module, DLL_THREAD_ATTACH);
     }

   RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);

   DPRINT("LdrpAttachThread() done\n");

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LdrShutdownThread (VOID)
{
   PLIST_ENTRY ModuleListHead;
   PLIST_ENTRY Entry;
   PLDR_MODULE Module;

   DPRINT("LdrShutdownThread() called for %wZ\n",
          &ExeModule->BaseDllName);

   RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);

   ModuleListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
   Entry = ModuleListHead->Blink;
   while (Entry != ModuleListHead)
     {
       Module = CONTAINING_RECORD(Entry, LDR_MODULE, InInitializationOrderModuleList);

       if (Module->Flags & PROCESS_ATTACH_CALLED &&
	   !(Module->Flags & DONT_CALL_FOR_THREAD) &&
           !(Module->Flags & UNLOAD_IN_PROGRESS))
         {
           TRACE_LDR("%wZ - Calling entry point at %x for thread detaching\n", 
		     &Module->BaseDllName, Module->EntryPoint);
           LdrpCallDllEntry(Module, DLL_THREAD_DETACH, NULL);
         }
       Entry = Entry->Blink;
     }

   RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);

   if (LdrpTlsArray)
     {
       RtlFreeHeap (RtlGetProcessHeap(),  0, NtCurrentTeb()->ThreadLocalStoragePointer);
     }

   DPRINT("LdrShutdownThread() done\n");

   return STATUS_SUCCESS;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      LdrQueryProcessModuleInformation
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 * @implemented
 */
NTSTATUS STDCALL
LdrQueryProcessModuleInformation(IN PMODULE_INFORMATION ModuleInformation OPTIONAL,
                                 IN ULONG Size OPTIONAL,
                                 OUT PULONG ReturnedSize)
{
  PLIST_ENTRY ModuleListHead;
  PLIST_ENTRY Entry;
  PLDR_MODULE Module;
  PMODULE_ENTRY ModulePtr = NULL;
  NTSTATUS Status = STATUS_SUCCESS;
  ULONG UsedSize = sizeof(ULONG);
  ANSI_STRING AnsiString;
  PCHAR p;

  DPRINT("LdrQueryProcessModuleInformation() called\n");

  RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);

  if (ModuleInformation == NULL || Size == 0)
    {
      Status = STATUS_INFO_LENGTH_MISMATCH;
    }
  else
    {
      ModuleInformation->ModuleCount = 0;
      ModulePtr = &ModuleInformation->ModuleEntry[0];
      Status = STATUS_SUCCESS;
    }

  ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
  Entry = ModuleListHead->Flink;

  while (Entry != ModuleListHead)
    {
      Module = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);

      DPRINT("  Module %wZ\n",
             &Module->FullDllName);

      if (UsedSize > Size)
        {
          Status = STATUS_INFO_LENGTH_MISMATCH;
        }
      else if (ModuleInformation != NULL)
        {
          ModulePtr->Unknown0 = 0;      // FIXME: ??
          ModulePtr->Unknown1 = 0;      // FIXME: ??
          ModulePtr->BaseAddress = Module->BaseAddress;
          ModulePtr->SizeOfImage = Module->SizeOfImage;
          ModulePtr->Flags = Module->Flags;
          ModulePtr->Unknown2 = 0;      // FIXME: load order index ??
          ModulePtr->Unknown3 = 0;      // FIXME: ??
          ModulePtr->LoadCount = Module->LoadCount;

          AnsiString.Length = 0;
          AnsiString.MaximumLength = 256;
          AnsiString.Buffer = ModulePtr->ModuleName;
          RtlUnicodeStringToAnsiString(&AnsiString,
                                       &Module->FullDllName,
                                       FALSE);
          p = strrchr(ModulePtr->ModuleName, '\\');
          if (p != NULL)
            ModulePtr->PathLength = p - ModulePtr->ModuleName + 1;
          else
            ModulePtr->PathLength = 0;

          ModulePtr++;
          ModuleInformation->ModuleCount++;
        }
      UsedSize += sizeof(MODULE_ENTRY);

      Entry = Entry->Flink;
    }

  RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);

  if (ReturnedSize != 0)
    *ReturnedSize = UsedSize;

  DPRINT("LdrQueryProcessModuleInformation() done\n");

  return(Status);
}


static BOOLEAN
LdrpCheckImageChecksum (IN PVOID BaseAddress,
			IN ULONG ImageSize)
{
  PIMAGE_NT_HEADERS Header;
  PUSHORT Ptr;
  ULONG Sum;
  ULONG CalcSum;
  ULONG HeaderSum;
  ULONG i;

  Header = RtlImageNtHeader (BaseAddress);
  if (Header == NULL)
    return FALSE;

  HeaderSum = Header->OptionalHeader.CheckSum;
  if (HeaderSum == 0)
    return TRUE;

   Sum = 0;
   Ptr = (PUSHORT) BaseAddress;
   for (i = 0; i < ImageSize / sizeof (USHORT); i++)
     {
      Sum += (ULONG)*Ptr;
      if (HIWORD(Sum) != 0)
	{
	  Sum = LOWORD(Sum) + HIWORD(Sum);
	}
      Ptr++;
     }

  if (ImageSize & 1)
    {
      Sum += (ULONG)*((PUCHAR)Ptr);
      if (HIWORD(Sum) != 0)
	{
	  Sum = LOWORD(Sum) + HIWORD(Sum);
	}
    }

  CalcSum = (USHORT)(LOWORD(Sum) + HIWORD(Sum));

  /* Subtract image checksum from calculated checksum. */
  /* fix low word of checksum */
  if (LOWORD(CalcSum) >= LOWORD(HeaderSum))
    {
      CalcSum -= LOWORD(HeaderSum);
    }
  else
    {
      CalcSum = ((LOWORD(CalcSum) - LOWORD(HeaderSum)) & 0xFFFF) - 1;
    }

   /* fix high word of checksum */
  if (LOWORD(CalcSum) >= HIWORD(HeaderSum))
    {
      CalcSum -= HIWORD(HeaderSum);
    }
  else
    {
      CalcSum = ((LOWORD(CalcSum) - HIWORD(HeaderSum)) & 0xFFFF) - 1;
    }

  /* add file length */
  CalcSum += ImageSize;

  return (BOOLEAN)(CalcSum == HeaderSum);
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      LdrVerifyImageMatchesChecksum
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 * @implemented
 */
NTSTATUS STDCALL
LdrVerifyImageMatchesChecksum (IN HANDLE FileHandle,
			       ULONG Unknown1,
			       ULONG Unknown2,
			       ULONG Unknown3)
{
  FILE_STANDARD_INFORMATION FileInfo;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE SectionHandle;
  ULONG ViewSize;
  PVOID BaseAddress;
  BOOLEAN Result;
  NTSTATUS Status;

  DPRINT ("LdrVerifyImageMatchesChecksum() called\n");

  Status = NtCreateSection (&SectionHandle,
			    SECTION_MAP_EXECUTE,
			    NULL,
			    NULL,
			    PAGE_EXECUTE,
			    SEC_COMMIT,
			    FileHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("NtCreateSection() failed (Status %lx)\n", Status);
      return Status;
    }

  ViewSize = 0;
  BaseAddress = NULL;
  Status = NtMapViewOfSection (SectionHandle,
			       NtCurrentProcess (),
			       &BaseAddress,
			       0,
			       0,
			       NULL,
			       &ViewSize,
			       ViewShare,
			       0,
			       PAGE_EXECUTE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("NtMapViewOfSection() failed (Status %lx)\n", Status);
      NtClose (SectionHandle);
      return Status;
    }

  Status = NtQueryInformationFile (FileHandle,
				   &IoStatusBlock,
				   &FileInfo,
				   sizeof (FILE_STANDARD_INFORMATION),
				   FileStandardInformation);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("NtMapViewOfSection() failed (Status %lx)\n", Status);
      NtUnmapViewOfSection (NtCurrentProcess (),
			    BaseAddress);
      NtClose (SectionHandle);
      return Status;
    }

  Result = LdrpCheckImageChecksum (BaseAddress,
				   FileInfo.EndOfFile.u.LowPart);
  if (Result == FALSE)
    {
      Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
    }

  NtUnmapViewOfSection (NtCurrentProcess (),
			BaseAddress);

  NtClose (SectionHandle);

  return Status;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      LdrQueryImageFileExecutionOptions
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *
 * @implemented
 */
NTSTATUS STDCALL
LdrQueryImageFileExecutionOptions (IN PUNICODE_STRING SubKey,
				   IN PCWSTR ValueName,
				   IN ULONG Type,
				   OUT PVOID Buffer,
				   IN ULONG BufferSize,
				   OUT PULONG ReturnedLength OPTIONAL)
{
  PKEY_VALUE_PARTIAL_INFORMATION KeyInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING ValueNameString;
  UNICODE_STRING KeyName;
  WCHAR NameBuffer[256];
  HANDLE KeyHandle;
  ULONG KeyInfoSize;
  ULONG ResultSize;
  PWCHAR Ptr;
  NTSTATUS Status;

  wcscpy (NameBuffer,
	  L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\");
  Ptr = wcsrchr (SubKey->Buffer, L'\\');
  if (Ptr == NULL)
    {
      Ptr = SubKey->Buffer;
    }
  else
    {
      Ptr++;
    }
  wcscat (NameBuffer, Ptr);
  RtlInitUnicodeString (&KeyName,
			NameBuffer);

  InitializeObjectAttributes (&ObjectAttributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);

  Status = NtOpenKey (&KeyHandle,
		      KEY_READ,
		      &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT ("NtOpenKey() failed (Status %lx)\n", Status);
      return Status;
    }

  KeyInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 32;
  KeyInfo = RtlAllocateHeap (RtlGetProcessHeap(),
			     HEAP_ZERO_MEMORY,
			     KeyInfoSize);

  RtlInitUnicodeString (&ValueNameString,
			(PWSTR)ValueName);
  Status = NtQueryValueKey (KeyHandle,
			    &ValueNameString,
			    KeyValuePartialInformation,
			    KeyInfo,
			    KeyInfoSize,
			    &ResultSize);
  if (Status == STATUS_BUFFER_OVERFLOW)
    {
      KeyInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + KeyInfo->DataLength;
      RtlFreeHeap (RtlGetProcessHeap(),
		   0,
		   KeyInfo);
      KeyInfo = RtlAllocateHeap (RtlGetProcessHeap(),
				 HEAP_ZERO_MEMORY,
				 KeyInfoSize);
      if (KeyInfo == NULL)
	{
	  NtClose (KeyHandle);
	  return Status;
	}

      Status = NtQueryValueKey (KeyHandle,
				&ValueNameString,
				KeyValuePartialInformation,
				KeyInfo,
				KeyInfoSize,
				&ResultSize);
    }
  NtClose (KeyHandle);

  if (!NT_SUCCESS(Status))
    {
      if (KeyInfo != NULL)
	{
	  RtlFreeHeap (RtlGetProcessHeap(),
		       0,
		       KeyInfo);
	}
      return Status;
    }

  if (KeyInfo->Type != Type)
    {
      RtlFreeHeap (RtlGetProcessHeap(),
		   0,
		   KeyInfo);
      return STATUS_OBJECT_TYPE_MISMATCH;
    }

  ResultSize = BufferSize;
  if (ResultSize < KeyInfo->DataLength)
    {
      Status = STATUS_BUFFER_OVERFLOW;
    }
  else
    {
      ResultSize = KeyInfo->DataLength;
    }
  RtlCopyMemory (Buffer,
		 &KeyInfo->Data,
		 ResultSize);

  RtlFreeHeap (RtlGetProcessHeap(),
	       0,
	       KeyInfo);

  if (ReturnedLength != NULL)
    {
      *ReturnedLength = ResultSize;
    }

  return Status;
}

/* EOF */
