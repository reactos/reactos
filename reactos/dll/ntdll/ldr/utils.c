/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/utils.c
 * PURPOSE:         Process startup for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/*
 * TODO:
 *      - Handle loading flags correctly
 *      - Handle errors correctly (unload dll's)
 *      - Implement a faster way to find modules (hash table)
 *      - any more ??
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

#define LDRP_PROCESS_CREATION_TIME 0x8000000
#define RVA(m, b) ((PVOID)((ULONG_PTR)(b) + (ULONG_PTR)(m)))

/* GLOBALS *******************************************************************/

#ifdef NDEBUG
#if defined(__GNUC__)
#define TRACE_LDR(args...) if (RtlGetNtGlobalFlags() & FLG_SHOW_LDR_SNAPS) { DbgPrint("(LDR:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); }
#elif defined(_MSC_VER)
#define TRACE_LDR(args, ...) if (RtlGetNtGlobalFlags() & FLG_SHOW_LDR_SNAPS) { DbgPrint("(LDR:%s:%d) ",__FILE__,__LINE__); DbgPrint(__VA_ARGS__); }
#endif	/* __GNUC__ */
#else
#define TRACE_LDR(args...) do { DbgPrint("(LDR:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0)
#endif

typedef struct _TLS_DATA
{
   PVOID StartAddressOfRawData;
   DWORD TlsDataSize;
   DWORD TlsZeroSize;
   PIMAGE_TLS_CALLBACK *TlsAddressOfCallBacks;
   PLDR_DATA_TABLE_ENTRY Module;
} TLS_DATA, *PTLS_DATA;

static BOOLEAN LdrpDllShutdownInProgress = FALSE;
static PTLS_DATA LdrpTlsArray = NULL;
static ULONG LdrpTlsCount = 0;
static ULONG LdrpTlsSize = 0;
static HANDLE LdrpKnownDllsDirHandle = NULL;
static UNICODE_STRING LdrpKnownDllPath = {0, 0, NULL};
static PLDR_DATA_TABLE_ENTRY LdrpLastModule = NULL;
extern PLDR_DATA_TABLE_ENTRY ExeModule;

/* PROTOTYPES ****************************************************************/

static NTSTATUS LdrFindEntryForName(PUNICODE_STRING Name, PLDR_DATA_TABLE_ENTRY *Module, BOOLEAN Ref);
static PVOID LdrFixupForward(PCHAR ForwardName);
static PVOID LdrGetExportByName(PVOID BaseAddress, PUCHAR SymbolName, USHORT Hint);
static NTSTATUS LdrpLoadModule(IN PWSTR SearchPath OPTIONAL,
                               IN ULONG LoadFlags,
                               IN PUNICODE_STRING Name,
                               OUT PLDR_DATA_TABLE_ENTRY *Module,
                               OUT PVOID *BaseAddress OPTIONAL);
static NTSTATUS LdrpAttachProcess(VOID);
static VOID LdrpDetachProcess(BOOLEAN UnloadAll);

/* FUNCTIONS *****************************************************************/

#if defined(DBG) || defined(KDBG)

VOID
LdrpLoadUserModuleSymbols(PLDR_DATA_TABLE_ENTRY LdrModule)
{
  NtSystemDebugControl(
    SysDbgQueryVersion,
    (PVOID)LdrModule,
    0,
    NULL,
    0,
    NULL);
}

#endif /* DBG || KDBG */

BOOLEAN
LdrMappedAsDataFile(PVOID *BaseAddress)
{
  if (0 != ((DWORD_PTR) *BaseAddress & (PAGE_SIZE - 1)))
    {
      *BaseAddress = (PVOID) ((DWORD_PTR) *BaseAddress & ~ ((DWORD_PTR) PAGE_SIZE - 1));
      return TRUE;
    }

   return FALSE;
}

static __inline LONG LdrpDecrementLoadCount(PLDR_DATA_TABLE_ENTRY Module, BOOLEAN Locked)
{
   LONG LoadCount;
   if (!Locked)
     {
       RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
     }
   LoadCount = Module->LoadCount;
   if (Module->LoadCount > 0 && Module->LoadCount != 0xFFFF)
     {
       Module->LoadCount--;
     }
   if (!Locked)
     {
       RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
     }
   return LoadCount;
}

static __inline LONG LdrpIncrementLoadCount(PLDR_DATA_TABLE_ENTRY Module, BOOLEAN Locked)
{
   LONG LoadCount;
   if (!Locked)
     {
       RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
     }
   LoadCount = Module->LoadCount;
   if (Module->LoadCount != 0xFFFF)
     {
       Module->LoadCount++;
     }
   if (!Locked)
     {
       RtlLeaveCriticalSection(NtCurrentPeb()->LoaderLock);
     }
   return LoadCount;
}

static __inline VOID LdrpAcquireTlsSlot(PLDR_DATA_TABLE_ENTRY Module, ULONG Size, BOOLEAN Locked)
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

static __inline VOID LdrpTlsCallback(PLDR_DATA_TABLE_ENTRY Module, ULONG dwReason)
{
   PIMAGE_TLS_CALLBACK *TlsCallback;
   if (Module->TlsIndex != 0xFFFF && Module->LoadCount == 0xFFFF)
     {
       TlsCallback = LdrpTlsArray[Module->TlsIndex].TlsAddressOfCallBacks;
       if (TlsCallback)
         {
           while (*TlsCallback)
             {
               TRACE_LDR("%wZ - Calling tls callback at %x\n",
                         &Module->BaseDllName, *TlsCallback);
               (*TlsCallback)(Module->DllBase, dwReason, NULL);
               TlsCallback++;
             }
         }
     }
}

static BOOLEAN LdrpCallDllEntry(PLDR_DATA_TABLE_ENTRY Module, DWORD dwReason, PVOID lpReserved)
{
   if (!(Module->Flags & LDRP_IMAGE_DLL) ||
       Module->EntryPoint == 0)
     {
       return TRUE;
     }
   LdrpTlsCallback(Module, dwReason);
   return  ((PDLLMAIN_FUNC)Module->EntryPoint)(Module->DllBase, dwReason, lpReserved);
}

static NTSTATUS
LdrpInitializeTlsForThread(VOID)
{
   PVOID* TlsPointers;
   PTLS_DATA TlsInfo;
   PVOID TlsData;
   ULONG i;
   PTEB Teb = NtCurrentTeb();

   DPRINT("LdrpInitializeTlsForThread() called for %wZ\n", &ExeModule->BaseDllName);

   Teb->StaticUnicodeString.Length = 0;
   Teb->StaticUnicodeString.MaximumLength = sizeof(Teb->StaticUnicodeBuffer);
   Teb->StaticUnicodeString.Buffer = Teb->StaticUnicodeBuffer;

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

       TlsData = (PVOID)((ULONG_PTR)TlsPointers + LdrpTlsCount * sizeof(PVOID));
       Teb->ThreadLocalStoragePointer = TlsPointers;

       TlsInfo = LdrpTlsArray;
       for (i = 0; i < LdrpTlsCount; i++, TlsInfo++)
         {
           TRACE_LDR("Initialize tls data for %wZ\n", &TlsInfo->Module->BaseDllName);
           TlsPointers[i] = TlsData;
           if (TlsInfo->TlsDataSize)
             {
               memcpy(TlsData, TlsInfo->StartAddressOfRawData, TlsInfo->TlsDataSize);
               TlsData = (PVOID)((ULONG_PTR)TlsData + TlsInfo->TlsDataSize);
             }
           if (TlsInfo->TlsZeroSize)
             {
               memset(TlsData, 0, TlsInfo->TlsZeroSize);
               TlsData = (PVOID)((ULONG_PTR)TlsData + TlsInfo->TlsZeroSize);
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
   PLDR_DATA_TABLE_ENTRY Module;
   PIMAGE_TLS_DIRECTORY TlsDirectory;
   PTLS_DATA TlsData;
   ULONG Size;

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
           Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
           if (Module->LoadCount == 0xFFFF &&
               Module->TlsIndex != 0xFFFF)
             {
               TlsDirectory = (PIMAGE_TLS_DIRECTORY)
                                 RtlImageDirectoryEntryToData(Module->DllBase,
                                                              TRUE,
                                                              IMAGE_DIRECTORY_ENTRY_TLS,
                                                              &Size);
               ASSERT(Module->TlsIndex < LdrpTlsCount);
               TlsData = &LdrpTlsArray[Module->TlsIndex];
               TlsData->StartAddressOfRawData = (PVOID)TlsDirectory->StartAddressOfRawData;
               TlsData->TlsDataSize = TlsDirectory->EndAddressOfRawData - TlsDirectory->StartAddressOfRawData;
               TlsData->TlsZeroSize = TlsDirectory->SizeOfZeroFill;
               if (TlsDirectory->AddressOfCallBacks)
                 TlsData->TlsAddressOfCallBacks = (PIMAGE_TLS_CALLBACK *)TlsDirectory->AddressOfCallBacks;
               else
                 TlsData->TlsAddressOfCallBacks = NULL;
               TlsData->Module = Module;
#if 0
               DbgPrint("TLS directory for %wZ\n", &Module->BaseDllName);
               DbgPrint("StartAddressOfRawData: %x\n", TlsDirectory->StartAddressOfRawData);
               DbgPrint("EndAddressOfRawData:   %x\n", TlsDirectory->EndAddressOfRawData);
               DbgPrint("SizeOfRawData:         %d\n", TlsDirectory->EndAddressOfRawData - TlsDirectory->StartAddressOfRawData);
               DbgPrint("AddressOfIndex:        %x\n", TlsDirectory->AddressOfIndex);
               DbgPrint("AddressOfCallBacks:    %x\n", TlsDirectory->AddressOfCallBacks);
               DbgPrint("SizeOfZeroFill:        %d\n", TlsDirectory->SizeOfZeroFill);
               DbgPrint("Characteristics:       %x\n", TlsDirectory->Characteristics);
#endif
               /*
                * FIXME:
                *   Is this region allways writable ?
                */
               *(PULONG)TlsDirectory->AddressOfIndex = Module->TlsIndex;
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
                             OBJ_CASE_INSENSITIVE,
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

PLDR_DATA_TABLE_ENTRY
LdrAddModuleEntry(PVOID ImageBase,
                  PIMAGE_NT_HEADERS NTHeaders,
                  PWSTR FullDosName)
{
  PLDR_DATA_TABLE_ENTRY Module;

  Module = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof (LDR_DATA_TABLE_ENTRY));
  ASSERT(Module);
  memset(Module, 0, sizeof(LDR_DATA_TABLE_ENTRY));
  Module->DllBase = (PVOID)ImageBase;
  Module->EntryPoint = (PVOID)NTHeaders->OptionalHeader.AddressOfEntryPoint;
  if (Module->EntryPoint != 0)
    Module->EntryPoint = (PVOID)((ULONG_PTR)Module->EntryPoint + (ULONG_PTR)Module->DllBase);
  Module->SizeOfImage = LdrpGetResidentSize(NTHeaders);
  if (NtCurrentPeb()->Ldr->Initialized == TRUE)
    {
      /* loading while app is running */
      Module->LoadCount = 1;
    } else {
      /*
       * loading while app is initializing
       * dll must not be unloaded
       */
      Module->LoadCount = 0xFFFF;
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
                 &Module->InLoadOrderLinks);
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
      DPRINT("NtOpenSection() failed for '%wZ' (Status 0x%08lx)\n", DllName, Status);
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
                    IN BOOLEAN MapAsDataFile,
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
  IO_STATUS_BLOCK       IoStatusBlock;
  NTSTATUS              Status;
  ULONG                 len;

  DPRINT("LdrpMapDllImageFile() called\n");

  if (SearchPath == NULL)
    {
      /* get application running path */

      wcscpy (SearchPathBuffer, NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer);

      len = wcslen (SearchPathBuffer);

      while (len && SearchPathBuffer[len - 1] != L'\\')
           len--;

      if (len) SearchPathBuffer[len-1] = L'\0';

      wcscat (SearchPathBuffer, L";");

      wcscat (SearchPathBuffer, SharedUserData->NtSystemRoot);
      wcscat (SearchPathBuffer, L"\\system32;");
      wcscat (SearchPathBuffer, SharedUserData->NtSystemRoot);
      wcscat (SearchPathBuffer, L";.");

      SearchPath = SearchPathBuffer;
    }

  if (RtlDosSearchPath_U (SearchPath,
                          DllName->Buffer,
                          NULL,
                          MAX_PATH,
                          DosName,
                          NULL) == 0)
    return STATUS_DLL_NOT_FOUND;

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
                      FILE_SHARE_READ,
                      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Dll open of %wZ failed: Status = 0x%08lx\n",
               &FullNtFileName, Status);
      RtlFreeHeap (RtlGetProcessHeap (),
                   0,
                   FullNtFileName.Buffer);
      return Status;
    }
  RtlFreeHeap (RtlGetProcessHeap (),
               0,
               FullNtFileName.Buffer);

  if (!MapAsDataFile)
    {

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
          DPRINT("Dll header read failed: Status = 0x%08lx\n", Status);
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
      if ((DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
          || (DosHeader->e_lfanew == 0L)
          || (*(PULONG)(NTHeaders) != IMAGE_NT_SIGNATURE))
        {
          DPRINT("NTDLL format invalid\n");
          NtClose(FileHandle);

          return STATUS_UNSUCCESSFUL;
        }
    }

  /*
   * Create a section for dll.
   */
  Status = NtCreateSection(SectionHandle,
                           SECTION_ALL_ACCESS,
                           NULL,
                           NULL,
                           PAGE_READONLY,
                           MapAsDataFile ? SEC_COMMIT : SEC_IMAGE,
                           FileHandle);
  NtClose(FileHandle);

  if (!NT_SUCCESS(Status))
    {
      DPRINT("NTDLL create section failed: Status = 0x%08lx\n", Status);
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
NTSTATUS NTAPI
LdrLoadDll (IN PWSTR SearchPath OPTIONAL,
            IN PULONG LoadFlags OPTIONAL,
            IN PUNICODE_STRING Name,
            OUT PVOID *BaseAddress /* also known as HMODULE*, and PHANDLE 'DllHandle' */)
{
  NTSTATUS              Status;
  PLDR_DATA_TABLE_ENTRY Module;

  PPEB Peb = NtCurrentPeb();

  TRACE_LDR("LdrLoadDll, loading %wZ%s%S\n",
            Name,
            SearchPath ? L" from " : L"",
            SearchPath ? SearchPath : L"");

  Status = LdrpLoadModule(SearchPath, LoadFlags ? *LoadFlags : 0, Name, &Module, BaseAddress);

  if (NT_SUCCESS(Status) &&
      (!LoadFlags || 0 == (*LoadFlags & LOAD_LIBRARY_AS_DATAFILE)))
    {
      if (!(Module->Flags & LDRP_PROCESS_ATTACH_CALLED))
        {
          RtlEnterCriticalSection(Peb->LoaderLock);
          Status = LdrpAttachProcess();
          RtlLeaveCriticalSection(Peb->LoaderLock);
        }
    }

 if ((!Module) && (NT_SUCCESS(Status)))
    return Status;

  *BaseAddress = NT_SUCCESS(Status) ? Module->DllBase : NULL;

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
NTSTATUS NTAPI
LdrFindEntryForAddress(PVOID Address,
                       PLDR_DATA_TABLE_ENTRY *Module)
{
  PLIST_ENTRY ModuleListHead;
  PLIST_ENTRY Entry;
  PLDR_DATA_TABLE_ENTRY ModulePtr;

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
      ModulePtr = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

      DPRINT("Scanning %wZ at %p\n", &ModulePtr->BaseDllName, ModulePtr->DllBase);

      if ((Address >= ModulePtr->DllBase) &&
          ((ULONG_PTR)Address <= ((ULONG_PTR)ModulePtr->DllBase + ModulePtr->SizeOfImage)))
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
                    PLDR_DATA_TABLE_ENTRY *Module,
                    BOOLEAN Ref)
{
  PLIST_ENTRY ModuleListHead;
  PLIST_ENTRY Entry;
  PLDR_DATA_TABLE_ENTRY ModulePtr;
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
          if (Ref && (*Module)->LoadCount != 0xFFFF)
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
      ModulePtr = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

      DPRINT("Scanning %wZ %wZ\n", &ModulePtr->BaseDllName, &AdjustedName);

      if ((! ContainsPath &&
           0 == RtlCompareUnicodeString(&ModulePtr->BaseDllName, &AdjustedName, TRUE)) ||
          (ContainsPath &&
           0 == RtlCompareUnicodeString(&ModulePtr->FullDllName, &AdjustedName, TRUE)))
        {
          *Module = LdrpLastModule = ModulePtr;
          if (Ref && ModulePtr->LoadCount != 0xFFFF)
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
   PLDR_DATA_TABLE_ENTRY Module;
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
             ULONG Flags = LDRP_PROCESS_CREATION_TIME;
             Status = LdrLoadDll(NULL,
                                 &Flags,
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

        DPRINT("BaseAddress: %p\n", Module->DllBase);

        return LdrGetExportByName(Module->DllBase, (PUCHAR)(p+1), -1);
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
        PVOID                   Function;

        ExportDir = (PIMAGE_EXPORT_DIRECTORY)
                RtlImageDirectoryEntryToData (BaseAddress,
                                              TRUE,
                                              IMAGE_DIRECTORY_ENTRY_EXPORT,
                                              &ExportDirSize);


        ExFunctions = (PDWORD *)
                RVA(
                        BaseAddress,
                        ExportDir->AddressOfFunctions
                        );
        DPRINT(
                "LdrGetExportByOrdinal(Ordinal %lu) = %p\n",
                Ordinal,
                RVA(BaseAddress, ExFunctions[Ordinal - ExportDir->Base] )
                );

        Function = (0 != ExFunctions[Ordinal - ExportDir->Base]
                    ? RVA(BaseAddress, ExFunctions[Ordinal - ExportDir->Base] )
                    : NULL);

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
   PVOID                        ExName;
   ULONG                        Ordinal;
   PVOID                        Function;
   LONG minn, maxn;
   ULONG ExportDirSize;

   DPRINT("LdrGetExportByName %p %s %hu\n", BaseAddress, SymbolName, Hint);

   ExportDir = (PIMAGE_EXPORT_DIRECTORY)
     RtlImageDirectoryEntryToData(BaseAddress,
                                  TRUE,
                                  IMAGE_DIRECTORY_ENTRY_EXPORT,
                                  &ExportDirSize);
   if (ExportDir == NULL)
     {
        DPRINT1("LdrGetExportByName(): no export directory, "
                "can't lookup %s/%hu!\n", SymbolName, Hint);
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
        if (strcmp(ExName, (PCHAR)SymbolName) == 0)
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
    * Binary search
    */
   minn = 0;
   maxn = ExportDir->NumberOfNames - 1;
   while (minn <= maxn)
     {
        LONG mid;
        LONG res;

        mid = (minn + maxn) / 2;

        ExName = RVA(BaseAddress, ExNames[mid]);
        res = strcmp(ExName, (PCHAR)SymbolName);
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

   DPRINT("LdrGetExportByName(): failed to find %s\n",SymbolName);
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
static NTSTATUS
LdrPerformRelocations(PIMAGE_NT_HEADERS NTHeaders,
                      PVOID ImageBase)
{
  PIMAGE_DATA_DIRECTORY RelocationDDir;
  PIMAGE_BASE_RELOCATION RelocationDir, RelocationEnd;
  ULONG Count, ProtectSize, OldProtect, OldProtect2;
  PVOID Page, ProtectPage, ProtectPage2;
  PUSHORT TypeOffset;
  ULONG_PTR Delta;
  NTSTATUS Status;

  if (NTHeaders->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)
    {
      return STATUS_SUCCESS;
    }

  RelocationDDir =
    &NTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

  if (RelocationDDir->VirtualAddress == 0 || RelocationDDir->Size == 0)
    {
      return STATUS_SUCCESS;
    }

  ProtectSize = PAGE_SIZE;
  Delta = (ULONG_PTR)ImageBase - NTHeaders->OptionalHeader.ImageBase;
  RelocationDir = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)ImageBase +
                  RelocationDDir->VirtualAddress);
  RelocationEnd = (PIMAGE_BASE_RELOCATION)((ULONG_PTR)ImageBase +
                  RelocationDDir->VirtualAddress + RelocationDDir->Size);

  while (RelocationDir < RelocationEnd &&
         RelocationDir->SizeOfBlock > 0)
    {
      Count = (RelocationDir->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) /
              sizeof(USHORT);
      Page = (PVOID)((ULONG_PTR)ImageBase + (ULONG_PTR)RelocationDir->VirtualAddress);
      TypeOffset = (PUSHORT)(RelocationDir + 1);

      /* Unprotect the page(s) we're about to relocate. */
      ProtectPage = Page;
      Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                      &ProtectPage,
                                      &ProtectSize,
                                      PAGE_READWRITE,
                                      &OldProtect);
      if (!NT_SUCCESS(Status))
        {
          DPRINT1("Failed to unprotect relocation target.\n");
          return Status;
        }

      if (RelocationDir->VirtualAddress + PAGE_SIZE <
          NTHeaders->OptionalHeader.SizeOfImage)
        {
          ProtectPage2 = (PVOID)((ULONG_PTR)ProtectPage + PAGE_SIZE);
          Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                          &ProtectPage2,
                                          &ProtectSize,
                                          PAGE_READWRITE,
                                          &OldProtect2);
          if (!NT_SUCCESS(Status))
            {
              DPRINT1("Failed to unprotect relocation target (2).\n");
              NtProtectVirtualMemory(NtCurrentProcess(),
                                     &ProtectPage,
                                     &ProtectSize,
                                     OldProtect,
                                     &OldProtect);
              return Status;
            }
        }
      else
        {
          ProtectPage2 = NULL;
        }

      RelocationDir = LdrProcessRelocationBlock((ULONG_PTR)Page,
                                                Count,
                                                TypeOffset,
                                                Delta);
      if (RelocationDir == NULL)
        return STATUS_UNSUCCESSFUL;

      /* Restore old page protection. */
      NtProtectVirtualMemory(NtCurrentProcess(),
                             &ProtectPage,
                             &ProtectSize,
                             OldProtect,
                             &OldProtect);

      if (ProtectPage2 != NULL)
        {
          NtProtectVirtualMemory(NtCurrentProcess(),
                                 &ProtectPage2,
                                 &ProtectSize,
                                 OldProtect2,
                                 &OldProtect2);
        }
    }

  return STATUS_SUCCESS;
}

static NTSTATUS
LdrpGetOrLoadModule(PWCHAR SearchPath,
                    PCHAR Name,
                    PLDR_DATA_TABLE_ENTRY* Module,
                    BOOLEAN Load)
{
   ANSI_STRING AnsiDllName;
   UNICODE_STRING DllName;
   NTSTATUS Status;

   DPRINT("LdrpGetOrLoadModule() called for %s\n", Name);

   RtlInitAnsiString(&AnsiDllName, Name);
   Status = RtlAnsiStringToUnicodeString(&DllName, &AnsiDllName, TRUE);
   if (!NT_SUCCESS(Status))
     {
       return Status;
     }

   Status = LdrFindEntryForName (&DllName, Module, Load);
   if (Load && !NT_SUCCESS(Status))
     {
       Status = LdrpLoadModule(SearchPath,
                               NtCurrentPeb()->Ldr->Initialized ? 0 : LDRP_PROCESS_CREATION_TIME,
                               &DllName,
                               Module,
                               NULL);
       if (NT_SUCCESS(Status))
         {
           Status = LdrFindEntryForName (&DllName, Module, FALSE);
         }
       if (!NT_SUCCESS(Status))
         {
           ULONG ErrorResponse;
           ULONG_PTR ErrorParameter = (ULONG_PTR)&DllName;

           DPRINT1("failed to load %wZ\n", &DllName);
           NtRaiseHardError(STATUS_DLL_NOT_FOUND,
                            1,
                            1,
                            &ErrorParameter,
                            OptionOk,
                            &ErrorResponse);
         }
     }
   RtlFreeUnicodeString (&DllName);
   return Status;
}

void
RtlpRaiseImportNotFound(CHAR *FuncName, ULONG Ordinal, PUNICODE_STRING DllName)
{
    ULONG ErrorResponse;
    ULONG_PTR ErrorParameters[2];
    ANSI_STRING ProcNameAnsi;
    UNICODE_STRING ProcName;
    CHAR Buffer[8];

    if (!FuncName)
    {
        _snprintf(Buffer, 8, "# %ld", Ordinal);
        FuncName = Buffer;
    }

    RtlInitAnsiString(&ProcNameAnsi, FuncName);
    RtlAnsiStringToUnicodeString(&ProcName, &ProcNameAnsi, TRUE);
    ErrorParameters[0] = (ULONG_PTR)&ProcName;
    ErrorParameters[1] = (ULONG_PTR)DllName;
    NtRaiseHardError(STATUS_ENTRYPOINT_NOT_FOUND,
                     2,
                     3,
                     ErrorParameters,
                     OptionOk,
                     &ErrorResponse);
    RtlFreeUnicodeString(&ProcName);
}

static NTSTATUS
LdrpProcessImportDirectoryEntry(PLDR_DATA_TABLE_ENTRY Module,
                                PLDR_DATA_TABLE_ENTRY ImportedModule,
                                PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory)
{
   NTSTATUS Status;
   PVOID* ImportAddressList;
   PULONG FunctionNameList;
   PVOID IATBase;
   ULONG OldProtect;
   ULONG Ordinal;
   ULONG IATSize;

   if (ImportModuleDirectory == NULL || ImportModuleDirectory->Name == 0)
     {
       return STATUS_UNSUCCESSFUL;
     }

   /* Get the import address list. */
   ImportAddressList = (PVOID *)((ULONG_PTR)Module->DllBase + (ULONG_PTR)ImportModuleDirectory->FirstThunk);

   /* Get the list of functions to import. */
   if (ImportModuleDirectory->OriginalFirstThunk != 0)
     {
       FunctionNameList = (PULONG) ((ULONG_PTR)Module->DllBase + (ULONG_PTR)ImportModuleDirectory->OriginalFirstThunk);
     }
   else
     {
       FunctionNameList = (PULONG)((ULONG_PTR)Module->DllBase + (ULONG_PTR)ImportModuleDirectory->FirstThunk);
     }

   /* Get the size of IAT. */
   IATSize = 0;
   while (FunctionNameList[IATSize] != 0L)
     {
       IATSize++;
     }

   /* Unprotect the region we are about to write into. */
   IATBase = (PVOID)ImportAddressList;
   IATSize *= sizeof(PVOID*);
   Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                   &IATBase,
                                   &IATSize,
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
           *ImportAddressList = LdrGetExportByOrdinal(ImportedModule->DllBase, Ordinal);
           if ((*ImportAddressList) == NULL)
             {
               DPRINT1("Failed to import #%ld from %wZ\n", Ordinal, &ImportedModule->FullDllName);
               RtlpRaiseImportNotFound(NULL, Ordinal, &ImportedModule->FullDllName);
               return STATUS_ENTRYPOINT_NOT_FOUND;
             }
         }
       else
         {
           IMAGE_IMPORT_BY_NAME *pe_name;
           pe_name = RVA(Module->DllBase, *FunctionNameList);
           *ImportAddressList = LdrGetExportByName(ImportedModule->DllBase, pe_name->Name, pe_name->Hint);
           if ((*ImportAddressList) == NULL)
             {
               DPRINT1("Failed to import %s from %wZ\n", pe_name->Name, &ImportedModule->FullDllName);
               RtlpRaiseImportNotFound((CHAR*)pe_name->Name, 0, &ImportedModule->FullDllName);
               return STATUS_ENTRYPOINT_NOT_FOUND;
             }
         }
       ImportAddressList++;
       FunctionNameList++;
     }

   /* Protect the region we are about to write into. */
   Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                   &IATBase,
                                   &IATSize,
                                   OldProtect,
                                   &OldProtect);
   if (!NT_SUCCESS(Status))
     {
       DPRINT1("Failed to protect IAT.\n");
       return(Status);
     }

   return STATUS_SUCCESS;
}

static NTSTATUS
LdrpProcessImportDirectory(
   PLDR_DATA_TABLE_ENTRY Module,
   PLDR_DATA_TABLE_ENTRY ImportedModule,
   PCHAR ImportedName)
{
   NTSTATUS Status;
   PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory;
   PCHAR Name;
   ULONG Size;

   DPRINT("LdrpProcessImportDirectory(%p '%wZ', '%s')\n",
          Module, &Module->BaseDllName, ImportedName);


   ImportModuleDirectory = (PIMAGE_IMPORT_DESCRIPTOR)
                             RtlImageDirectoryEntryToData(Module->DllBase,
                                                          TRUE,
                                                          IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                          &Size);
   if (ImportModuleDirectory == NULL)
     {
       return STATUS_UNSUCCESSFUL;
     }

   while (ImportModuleDirectory->Name)
     {
       Name = (PCHAR)Module->DllBase + ImportModuleDirectory->Name;
       if (0 == _stricmp(Name, ImportedName))
         {
           Status = LdrpProcessImportDirectoryEntry(Module,
                                                    ImportedModule,
                                                    ImportModuleDirectory);
           if (!NT_SUCCESS(Status))
             {
               return Status;
             }
         }
       ImportModuleDirectory++;
     }


   return STATUS_SUCCESS;
}


static NTSTATUS
LdrpAdjustImportDirectory(PLDR_DATA_TABLE_ENTRY Module,
                          PLDR_DATA_TABLE_ENTRY ImportedModule,
                          PCHAR ImportedName)
{
   PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory;
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
   ULONG Size;

   DPRINT("LdrpAdjustImportDirectory(Module %p '%wZ', %p '%wZ', '%s')\n",
          Module, &Module->BaseDllName, ImportedModule, &ImportedModule->BaseDllName, ImportedName);

   ImportModuleDirectory = (PIMAGE_IMPORT_DESCRIPTOR)
                              RtlImageDirectoryEntryToData(Module->DllBase,
                                                           TRUE,
                                                           IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                           &Size);
   if (ImportModuleDirectory == NULL)
     {
       return STATUS_UNSUCCESSFUL;
     }

   while (ImportModuleDirectory->Name)
     {
       Name = (PCHAR)Module->DllBase + ImportModuleDirectory->Name;
       if (0 == _stricmp(Name, (PCHAR)ImportedName))
         {

           /* Get the import address list. */
           ImportAddressList = (PVOID *)((ULONG_PTR)Module->DllBase + (ULONG_PTR)ImportModuleDirectory->FirstThunk);

           /* Get the list of functions to import. */
           if (ImportModuleDirectory->OriginalFirstThunk != 0)
             {
               FunctionNameList = (PULONG) ((ULONG_PTR)Module->DllBase + (ULONG_PTR)ImportModuleDirectory->OriginalFirstThunk);
             }
           else
             {
               FunctionNameList = (PULONG)((ULONG_PTR)Module->DllBase + (ULONG_PTR)ImportModuleDirectory->FirstThunk);
             }

           /* Get the size of IAT. */
           IATSize = 0;
           while (FunctionNameList[IATSize] != 0L)
             {
               IATSize++;
             }

           /* Unprotect the region we are about to write into. */
           IATBase = (PVOID)ImportAddressList;
           IATSize *= sizeof(PVOID*);
           Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                           &IATBase,
                                           &IATSize,
                                           PAGE_READWRITE,
                                           &OldProtect);
           if (!NT_SUCCESS(Status))
             {
               DPRINT1("Failed to unprotect IAT.\n");
               return(Status);
             }

           NTHeaders = RtlImageNtHeader (ImportedModule->DllBase);
           Start = (PVOID)NTHeaders->OptionalHeader.ImageBase;
           End = (PVOID)((ULONG_PTR)Start + ImportedModule->SizeOfImage);
           Offset = (ULONG)((ULONG_PTR)ImportedModule->DllBase - (ULONG_PTR)Start);

           /* Walk through function list and fixup addresses. */
           while (*FunctionNameList != 0L)
             {
               if (*ImportAddressList >= Start && *ImportAddressList < End)
                 {
                   (*ImportAddressList) = (PVOID)((ULONG_PTR)(*ImportAddressList) + Offset);
                 }
               ImportAddressList++;
               FunctionNameList++;
             }

           /* Protect the region we are about to write into. */
           Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                           &IATBase,
                                           &IATSize,
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
                IN PLDR_DATA_TABLE_ENTRY Module)
{
   PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory;
   PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectoryCurrent;
   PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundImportDescriptor;
   PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundImportDescriptorCurrent;
   PIMAGE_TLS_DIRECTORY TlsDirectory;
   ULONG TlsSize = 0;
   NTSTATUS Status;
   PLDR_DATA_TABLE_ENTRY ImportedModule;
   PCHAR ImportedName;
   ULONG Size;

   DPRINT("LdrFixupImports(SearchPath %S, Module %p)\n", SearchPath, Module);

   /* Check for tls data */
   TlsDirectory = (PIMAGE_TLS_DIRECTORY)
                     RtlImageDirectoryEntryToData(Module->DllBase,
                                                  TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_TLS,
                                                  &Size);
   if (TlsDirectory)
     {
       TlsSize = TlsDirectory->EndAddressOfRawData
                   - TlsDirectory->StartAddressOfRawData
                   + TlsDirectory->SizeOfZeroFill;

       if (TlsSize > 0 && NtCurrentPeb()->Ldr->Initialized)
         {
           TRACE_LDR("Trying to dynamically load %wZ which contains a TLS directory\n",
                     &Module->BaseDllName);
           TlsDirectory = NULL;
         }
     }

   /*
    * Process each import module.
    */
   ImportModuleDirectory = (PIMAGE_IMPORT_DESCRIPTOR)
                              RtlImageDirectoryEntryToData(Module->DllBase,
                                                           TRUE,
                                                           IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                           &Size);

   BoundImportDescriptor = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)
                              RtlImageDirectoryEntryToData(Module->DllBase,
                                                           TRUE,
                                                           IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,
                                                           &Size);

   if (BoundImportDescriptor != NULL && ImportModuleDirectory == NULL)
     {
       DPRINT1("%wZ has only a bound import directory\n", &Module->BaseDllName);
       return STATUS_UNSUCCESSFUL;
     }
   if (BoundImportDescriptor)
     {
       DPRINT("BoundImportDescriptor %p\n", BoundImportDescriptor);

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
               BOOLEAN WrongForwarder;
               WrongForwarder = FALSE;
               if (ImportedModule->Flags & LDRP_IMAGE_NOT_AT_BASE)
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
                   PLDR_DATA_TABLE_ENTRY ForwarderModule;
                   PCHAR ForwarderName;

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
                           ForwarderModule->Flags & LDRP_IMAGE_NOT_AT_BASE)
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
                   ImportedModule->Flags & LDRP_IMAGE_NOT_AT_BASE)
                 {
                   Status = LdrpProcessImportDirectory(Module, ImportedModule, ImportedName);
                   if (!NT_SUCCESS(Status))
                     {
                       DPRINT1("failed to import %s\n", ImportedName);
                       return Status;
                     }
                 }
               else if (ImportedModule->Flags & LDRP_IMAGE_NOT_AT_BASE)
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
       DPRINT("ImportModuleDirectory %p\n", ImportModuleDirectory);

       ImportModuleDirectoryCurrent = ImportModuleDirectory;
       while (ImportModuleDirectoryCurrent->Name)
         {
           ImportedName = (PCHAR)Module->DllBase + ImportModuleDirectoryCurrent->Name;
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
           Status = LdrpProcessImportDirectoryEntry(Module, ImportedModule, ImportModuleDirectoryCurrent);
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
                      PLDR_DATA_TABLE_ENTRY* Module,
                      PWSTR FullDosName)
{
   NTSTATUS             Status;
   PEPFUNC              EntryPoint = NULL;
   PIMAGE_DOS_HEADER    DosHeader;
   PIMAGE_NT_HEADERS    NTHeaders;
   PLDR_DATA_TABLE_ENTRY tmpModule;

   DPRINT("LdrPEStartup(ImageBase %p SectionHandle %p)\n",
           ImageBase, SectionHandle);

   /*
    * Overlay DOS and WNT headers structures
    * to the DLL's image.
    */
   DosHeader = (PIMAGE_DOS_HEADER) ImageBase;
   NTHeaders = (PIMAGE_NT_HEADERS) ((ULONG_PTR)ImageBase + DosHeader->e_lfanew);

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
       (*Module)->SectionPointer = SectionHandle;
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
       (*Module)->Flags |= LDRP_IMAGE_NOT_AT_BASE;
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
       return NULL;
     }

   /*
    * Compute the DLL's entry point's address.
    */
   DPRINT("ImageBase = %p\n", ImageBase);
   DPRINT("AddressOfEntryPoint = 0x%lx\n",(ULONG)NTHeaders->OptionalHeader.AddressOfEntryPoint);
   if (NTHeaders->OptionalHeader.AddressOfEntryPoint != 0)
     {
        EntryPoint = (PEPFUNC) ((ULONG_PTR)ImageBase
                           + NTHeaders->OptionalHeader.AddressOfEntryPoint);
     }
   DPRINT("LdrPEStartup() = %p\n",EntryPoint);
   return EntryPoint;
}

static NTSTATUS
LdrpLoadModule(IN PWSTR SearchPath OPTIONAL,
               IN ULONG LoadFlags,
               IN PUNICODE_STRING Name,
               PLDR_DATA_TABLE_ENTRY *Module,
               PVOID *BaseAddress OPTIONAL)
{
    UNICODE_STRING AdjustedName;
    UNICODE_STRING FullDosName;
    NTSTATUS Status;
    PLDR_DATA_TABLE_ENTRY tmpModule;
    HANDLE SectionHandle;
    SIZE_T ViewSize;
    PVOID ImageBase;
    PIMAGE_NT_HEADERS NtHeaders;
    BOOLEAN MappedAsDataFile;
    PVOID ArbitraryUserPointer;

    if (Module == NULL)
      {
        Module = &tmpModule;
      }
    /* adjust the full dll name */
    LdrAdjustDllName(&AdjustedName, Name, FALSE);

    DPRINT("%wZ\n", &AdjustedName);

    MappedAsDataFile = FALSE;
    /* Test if dll is already loaded */
    Status = LdrFindEntryForName(&AdjustedName, Module, TRUE);
    if (NT_SUCCESS(Status))
      {
        RtlFreeUnicodeString(&AdjustedName);
        if (NULL != BaseAddress)
          {
            *BaseAddress = (*Module)->DllBase;
          }
      }
    else
      {
        /* Open or create dll image section */
        Status = LdrpMapKnownDll(&AdjustedName, &FullDosName, &SectionHandle);
        if (!NT_SUCCESS(Status))
          {
            MappedAsDataFile = (0 != (LoadFlags & LOAD_LIBRARY_AS_DATAFILE));
            Status = LdrpMapDllImageFile(SearchPath, &AdjustedName, &FullDosName,
                                         MappedAsDataFile, &SectionHandle);
          }
        if (!NT_SUCCESS(Status))
          {
            DPRINT1("Failed to create or open dll section of '%wZ' (Status %lx)\n", &AdjustedName, Status);
            RtlFreeUnicodeString(&AdjustedName);
            return Status;
          }
        RtlFreeUnicodeString(&AdjustedName);
        /* Map the dll into the process */
        ViewSize = 0;
        ImageBase = 0;
        ArbitraryUserPointer = NtCurrentTeb()->Tib.ArbitraryUserPointer;
        NtCurrentTeb()->Tib.ArbitraryUserPointer = FullDosName.Buffer;
        Status = NtMapViewOfSection(SectionHandle,
                                    NtCurrentProcess(),
                                    &ImageBase,
                                    0,
                                    0,
                                    NULL,
                                    &ViewSize,
                                    ViewShare,
                                    0,
                                    PAGE_READONLY);
        NtCurrentTeb()->Tib.ArbitraryUserPointer = ArbitraryUserPointer;
        if (!NT_SUCCESS(Status))
          {
            DPRINT1("map view of section failed (Status 0x%08lx)\n", Status);
            RtlFreeUnicodeString(&FullDosName);
            NtClose(SectionHandle);
            return(Status);
          }
        if (NULL != BaseAddress)
          {
            *BaseAddress = ImageBase;
          }
        if (!MappedAsDataFile)
          {
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
          }
        DPRINT("Mapped %wZ at %x\n", &FullDosName, ImageBase);
        if (MappedAsDataFile)
          {
            ASSERT(NULL != BaseAddress);
            if (NULL != BaseAddress)
              {
                *BaseAddress = (PVOID) ((char *) *BaseAddress + 1);
              }
            *Module = NULL;
            RtlFreeUnicodeString(&FullDosName);
            NtClose(SectionHandle);
            return STATUS_SUCCESS;
          }
        /* If the base address is different from the
         * one the DLL is actually loaded, perform any
         * relocation. */
        if (ImageBase != (PVOID) NtHeaders->OptionalHeader.ImageBase)
          {
            DPRINT1("Relocating (%lx -> %p) %wZ\n",
              NtHeaders->OptionalHeader.ImageBase, ImageBase, &FullDosName);
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
        (*Module)->SectionPointer = SectionHandle;
        if (ImageBase != (PVOID) NtHeaders->OptionalHeader.ImageBase)
          {
            (*Module)->Flags |= LDRP_IMAGE_NOT_AT_BASE;
          }
        if (NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL)
          {
            (*Module)->Flags |= LDRP_IMAGE_DLL;
          }
        /* fixup the imported calls entry points */
        Status = LdrFixupImports(SearchPath, *Module);
        if (!NT_SUCCESS(Status))
          {
            DPRINT1("LdrFixupImports failed for %wZ, status=%x\n", &(*Module)->BaseDllName, Status);
            return Status;
          }
#if defined(DBG) || defined(KDBG)
        LdrpLoadUserModuleSymbols(*Module);
#endif /* DBG || KDBG */
        RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);
        InsertTailList(&NtCurrentPeb()->Ldr->InInitializationOrderModuleList,
                       &(*Module)->InInitializationOrderModuleList);
        RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);
      }
    return STATUS_SUCCESS;
}

static NTSTATUS
LdrpUnloadModule(PLDR_DATA_TABLE_ENTRY Module,
                 BOOLEAN Unload)
{
   PIMAGE_IMPORT_DESCRIPTOR ImportModuleDirectory;
   PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundImportDescriptor;
   PIMAGE_BOUND_IMPORT_DESCRIPTOR BoundImportDescriptorCurrent;
   PCHAR ImportedName;
   PLDR_DATA_TABLE_ENTRY ImportedModule;
   NTSTATUS Status;
   LONG LoadCount;
   ULONG Size;

   if (Unload)
     {
       RtlEnterCriticalSection(NtCurrentPeb()->LoaderLock);
     }

   LoadCount = LdrpDecrementLoadCount(Module, Unload);

   TRACE_LDR("Unload %wZ, LoadCount %d\n", &Module->BaseDllName, LoadCount);

   if (LoadCount == 0)
     {
       /* ?????????????????? */
     }
   else if (!(Module->Flags & LDRP_STATIC_LINK) && LoadCount == 1)
     {
       BoundImportDescriptor = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)
                                 RtlImageDirectoryEntryToData(Module->DllBase,
                                                              TRUE,
                                                              IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,
                                                              &Size);
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
           ImportModuleDirectory = (PIMAGE_IMPORT_DESCRIPTOR)
                                      RtlImageDirectoryEntryToData(Module->DllBase,
                                                                   TRUE,
                                                                   IMAGE_DIRECTORY_ENTRY_IMPORT,
                                                                   &Size);
           if (ImportModuleDirectory)
             {
               /* dereferencing all imported modules, use the import descriptor */
               while (ImportModuleDirectory->Name)
                 {
                   ImportedName = (PCHAR)Module->DllBase + ImportModuleDirectory->Name;
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
       if (!(Module->Flags & LDRP_STATIC_LINK))
         {
           LdrpDetachProcess(FALSE);
         }

       RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);
     }
   return STATUS_SUCCESS;

}

/*
 * @implemented
 */
NTSTATUS NTAPI
LdrUnloadDll (IN PVOID BaseAddress)
{
   PLDR_DATA_TABLE_ENTRY Module;
   NTSTATUS Status;

   if (BaseAddress == NULL)
     return STATUS_SUCCESS;

   if (LdrMappedAsDataFile(&BaseAddress))
     {
       Status = NtUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
     }
   else
     {
       Status = LdrFindEntryForAddress(BaseAddress, &Module);
       if (NT_SUCCESS(Status))
         {
           TRACE_LDR("LdrUnloadDll, , unloading %wZ\n", &Module->BaseDllName);
           Status = LdrpUnloadModule(Module, TRUE);
         }
     }

   return Status;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
LdrDisableThreadCalloutsForDll(IN PVOID BaseAddress)
{
    PLIST_ENTRY ModuleListHead;
    PLIST_ENTRY Entry;
    PLDR_DATA_TABLE_ENTRY Module;
    NTSTATUS Status;

    DPRINT("LdrDisableThreadCalloutsForDll (BaseAddress %p)\n", BaseAddress);

    Status = STATUS_DLL_NOT_FOUND;
    RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
    ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Entry = ModuleListHead->Flink;
    while (Entry != ModuleListHead)
      {
        Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        DPRINT("BaseDllName %wZ BaseAddress %p\n", &Module->BaseDllName, Module->DllBase);

        if (Module->DllBase == BaseAddress)
          {
            if (Module->TlsIndex == 0xFFFF)
              {
                Module->Flags |= LDRP_DONT_CALL_FOR_THREADS;
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
NTSTATUS NTAPI
LdrGetDllHandle(IN PWSTR DllPath OPTIONAL,
                IN PULONG DllCharacteristics,
                IN PUNICODE_STRING DllName,
                OUT PVOID *DllHandle)
{
    PLDR_DATA_TABLE_ENTRY Module;
    NTSTATUS Status;

    TRACE_LDR("LdrGetDllHandle, searching for %wZ from %S\n",
               DllName, DllPath ? DllPath : L"");

    /* NULL is the current executable */
    if (DllName == NULL)
      {
        *DllHandle = ExeModule->DllBase;
        DPRINT("BaseAddress 0x%lx\n", *DllHandle);
        return STATUS_SUCCESS;
      }

    Status = LdrFindEntryForName(DllName, &Module, FALSE);
    if (NT_SUCCESS(Status))
      {
        *DllHandle = Module->DllBase;
        return STATUS_SUCCESS;
      }

    DPRINT("Failed to find dll %wZ\n", DllName);
    *DllHandle = NULL;
    return STATUS_DLL_NOT_FOUND;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
LdrAddRefDll(IN ULONG Flags,
             IN PVOID BaseAddress)
{
    PLIST_ENTRY ModuleListHead;
    PLIST_ENTRY Entry;
    PLDR_DATA_TABLE_ENTRY Module;
    NTSTATUS Status;

    if (Flags & ~(LDR_PIN_MODULE))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = STATUS_DLL_NOT_FOUND;
    RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
    ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Entry = ModuleListHead->Flink;
    while (Entry != ModuleListHead)
    {
        Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if (Module->DllBase == BaseAddress)
        {
            if (Flags & LDR_PIN_MODULE)
            {
                Module->Flags |= LDRP_STATIC_LINK;
            }
            else
            {
                LdrpIncrementLoadCount(Module,
                                       FALSE);
            }
            Status = STATUS_SUCCESS;
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
PVOID NTAPI
RtlPcToFileHeader(IN PVOID PcValue,
                  PVOID* BaseOfImage)
{
    PLIST_ENTRY ModuleListHead;
    PLIST_ENTRY Entry;
    PLDR_DATA_TABLE_ENTRY Module;
    PVOID ImageBase = NULL;

    RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);
    ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Entry = ModuleListHead->Flink;
    while (Entry != ModuleListHead)
      {
        Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        if ((ULONG_PTR)PcValue >= (ULONG_PTR)Module->DllBase &&
            (ULONG_PTR)PcValue < (ULONG_PTR)Module->DllBase + Module->SizeOfImage)
          {
            ImageBase = Module->DllBase;
            break;
          }
        Entry = Entry->Flink;
      }
    RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);

    *BaseOfImage = ImageBase;
    return ImageBase;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
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

   DPRINT("LdrGetProcedureAddress (BaseAddress %p Name %Z Ordinal %lu ProcedureAddress %p)\n",
          BaseAddress, Name, Ordinal, ProcedureAddress);

   if (Name && Name->Length)
     {
       /* by name */
       *ProcedureAddress = LdrGetExportByName(BaseAddress, (PUCHAR)Name->Buffer, 0xffff);
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
       DPRINT("LdrGetProcedureAddress: Can't resolve symbol @%lu\n", Ordinal);
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
LdrpDetachProcess(BOOLEAN UnloadAll)
{
   PLIST_ENTRY ModuleListHead;
   PLIST_ENTRY Entry;
   PLDR_DATA_TABLE_ENTRY Module;
   static ULONG CallingCount = 0;

   DPRINT("LdrpDetachProcess() called for %wZ\n",
           &ExeModule->BaseDllName);

   if (UnloadAll)
     LdrpDllShutdownInProgress = TRUE;

   CallingCount++;

   ModuleListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
   Entry = ModuleListHead->Blink;
   while (Entry != ModuleListHead)
     {
       Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InInitializationOrderModuleList);
       if (((UnloadAll && Module->LoadCount == 0xFFFF) || Module->LoadCount == 0) &&
           Module->Flags & LDRP_ENTRY_PROCESSED &&
           !(Module->Flags & LDRP_UNLOAD_IN_PROGRESS))
         {
           Module->Flags |= LDRP_UNLOAD_IN_PROGRESS;
           if (Module == LdrpLastModule)
             {
               LdrpLastModule = NULL;
             }
           if (Module->Flags & LDRP_PROCESS_ATTACH_CALLED)
             {
               TRACE_LDR("Unload %wZ - Calling entry point at %x\n",
                         &Module->BaseDllName, Module->EntryPoint);
               LdrpCallDllEntry(Module, DLL_PROCESS_DETACH, (PVOID)(Module->LoadCount == 0xFFFF ? 1 : 0));
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
           Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InInitializationOrderModuleList);
           Entry = Entry->Blink;
           if (Module->Flags & LDRP_UNLOAD_IN_PROGRESS &&
               ((UnloadAll && Module->LoadCount != 0xFFFF) || Module->LoadCount == 0))
             {
               /* remove the module entry from the list */
               RemoveEntryList (&Module->InLoadOrderLinks);
               RemoveEntryList (&Module->InInitializationOrderModuleList);

               NtUnmapViewOfSection (NtCurrentProcess (), Module->DllBase);
               NtClose (Module->SectionPointer);

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
   PLDR_DATA_TABLE_ENTRY Module;
   BOOLEAN Result;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("LdrpAttachProcess() called for %wZ\n",
          &ExeModule->BaseDllName);

   ModuleListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
   Entry = ModuleListHead->Flink;
   while (Entry != ModuleListHead)
     {
       Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InInitializationOrderModuleList);
       if (!(Module->Flags & (LDRP_LOAD_IN_PROGRESS|LDRP_UNLOAD_IN_PROGRESS|LDRP_ENTRY_PROCESSED)))
         {
           Module->Flags |= LDRP_LOAD_IN_PROGRESS;
           TRACE_LDR("%wZ loaded - Calling init routine at %x for process attaching\n",
                     &Module->BaseDllName, Module->EntryPoint);
           Result = LdrpCallDllEntry(Module, DLL_PROCESS_ATTACH, (PVOID)(Module->LoadCount == 0xFFFF ? 1 : 0));
           if (!Result)
             {
               Status = STATUS_DLL_INIT_FAILED;
               break;
             }
           if (Module->Flags & LDRP_IMAGE_DLL && Module->EntryPoint != 0)
             {
               Module->Flags |= LDRP_PROCESS_ATTACH_CALLED|LDRP_ENTRY_PROCESSED;
             }
           else
             {
               Module->Flags |= LDRP_ENTRY_PROCESSED;
             }
           Module->Flags &= ~LDRP_LOAD_IN_PROGRESS;
         }
       Entry = Entry->Flink;
     }

   DPRINT("LdrpAttachProcess() done\n");

   return Status;
}

/*
 * @implemented
 */
BOOLEAN NTAPI
RtlDllShutdownInProgress (VOID)
{
  return LdrpDllShutdownInProgress;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
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
  PLDR_DATA_TABLE_ENTRY Module;
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
          Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InInitializationOrderModuleList);
          if (Module->Flags & LDRP_PROCESS_ATTACH_CALLED &&
              !(Module->Flags & LDRP_DONT_CALL_FOR_THREADS) &&
              !(Module->Flags & LDRP_UNLOAD_IN_PROGRESS))
            {
              TRACE_LDR("%wZ - Calling entry point at %x for thread attaching\n",
                        &Module->BaseDllName, Module->EntryPoint);
              LdrpCallDllEntry(Module, DLL_THREAD_ATTACH, NULL);
            }
          Entry = Entry->Flink;
        }

      Entry = NtCurrentPeb()->Ldr->InLoadOrderModuleList.Flink;
      Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
      LdrpTlsCallback(Module, DLL_THREAD_ATTACH);
    }

  RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);

  DPRINT("LdrpAttachThread() done\n");

  return Status;
}


/*
 * @implemented
 */
NTSTATUS NTAPI
LdrShutdownThread (VOID)
{
   PLIST_ENTRY ModuleListHead;
   PLIST_ENTRY Entry;
   PLDR_DATA_TABLE_ENTRY Module;

   DPRINT("LdrShutdownThread() called for %wZ\n",
          &ExeModule->BaseDllName);

   RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);

   ModuleListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
   Entry = ModuleListHead->Blink;
   while (Entry != ModuleListHead)
     {
       Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InInitializationOrderModuleList);

       if (Module->Flags & LDRP_PROCESS_ATTACH_CALLED &&
           !(Module->Flags & LDRP_DONT_CALL_FOR_THREADS) &&
           !(Module->Flags & LDRP_UNLOAD_IN_PROGRESS))
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
NTSTATUS NTAPI
LdrQueryProcessModuleInformation(IN PRTL_PROCESS_MODULES ModuleInformation OPTIONAL,
                                 IN ULONG Size OPTIONAL,
                                 OUT PULONG ReturnedSize)
{
  PLIST_ENTRY ModuleListHead;
  PLIST_ENTRY Entry;
  PLDR_DATA_TABLE_ENTRY Module;
  PRTL_PROCESS_MODULE_INFORMATION ModulePtr = NULL;
  NTSTATUS Status = STATUS_SUCCESS;
  ULONG UsedSize = sizeof(ULONG);
  ANSI_STRING AnsiString;
  PCHAR p;

  DPRINT("LdrQueryProcessModuleInformation() called\n");
 // FIXME: This code is ultra-duplicated. see lib\rtl\dbgbuffer.c
  RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);

  if (ModuleInformation == NULL || Size == 0)
    {
      Status = STATUS_INFO_LENGTH_MISMATCH;
    }
  else
    {
      ModuleInformation->NumberOfModules = 0;
      ModulePtr = &ModuleInformation->Modules[0];
      Status = STATUS_SUCCESS;
    }

  ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
  Entry = ModuleListHead->Flink;

  while (Entry != ModuleListHead)
    {
      Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

      DPRINT("  Module %wZ\n",
             &Module->FullDllName);

      if (UsedSize > Size)
        {
          Status = STATUS_INFO_LENGTH_MISMATCH;
        }
      else if (ModuleInformation != NULL)
        {
          ModulePtr->Section = 0;
          ModulePtr->MappedBase = NULL;      // FIXME: ??
          ModulePtr->ImageBase        = Module->DllBase;
          ModulePtr->ImageSize        = Module->SizeOfImage;
          ModulePtr->Flags       = Module->Flags;
          ModulePtr->LoadOrderIndex       = 0;      // FIXME:  ??
          ModulePtr->InitOrderIndex     = 0;      // FIXME: ??
          ModulePtr->LoadCount   = Module->LoadCount;

          AnsiString.Length        = 0;
          AnsiString.MaximumLength = 256;
          AnsiString.Buffer        = ModulePtr->FullPathName;
          RtlUnicodeStringToAnsiString(&AnsiString,
                                       &Module->FullDllName,
                                       FALSE);

          p = strrchr(ModulePtr->FullPathName, '\\');
          if (p != NULL)
            ModulePtr->OffsetToFileName = p - ModulePtr->FullPathName + 1;
          else
            ModulePtr->OffsetToFileName = 0;

          ModulePtr++;
          ModuleInformation->NumberOfModules++;
        }
      UsedSize += sizeof(RTL_PROCESS_MODULE_INFORMATION);

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

/*
 * Compute size of an image as it is actually present in virt memory
 * (i.e. excluding NEVER_LOAD sections)
 */
ULONG
LdrpGetResidentSize(PIMAGE_NT_HEADERS NTHeaders)
{
  PIMAGE_SECTION_HEADER SectionHeader;
  unsigned SectionIndex;
  ULONG ResidentSize;

  SectionHeader = (PIMAGE_SECTION_HEADER)((char *) &NTHeaders->OptionalHeader
                                          + NTHeaders->FileHeader.SizeOfOptionalHeader);
  ResidentSize = 0;
  for (SectionIndex = 0; SectionIndex < NTHeaders->FileHeader.NumberOfSections; SectionIndex++)
    {
      if (0 == (SectionHeader->Characteristics & IMAGE_SCN_LNK_REMOVE)
          && ResidentSize < SectionHeader->VirtualAddress + SectionHeader->Misc.VirtualSize)
        {
          ResidentSize = SectionHeader->VirtualAddress + SectionHeader->Misc.VirtualSize;
        }
      SectionHeader++;
    }

  return ResidentSize;
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
NTSTATUS NTAPI
LdrVerifyImageMatchesChecksum (IN HANDLE FileHandle,
                               ULONG Unknown1,
                               ULONG Unknown2,
                               ULONG Unknown3)
{
  FILE_STANDARD_INFORMATION FileInfo;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE SectionHandle;
  SIZE_T ViewSize;
  PVOID BaseAddress;
  BOOLEAN Result;
  NTSTATUS Status;

  DPRINT ("LdrVerifyImageMatchesChecksum() called\n");

  Status = NtCreateSection (&SectionHandle,
                            SECTION_MAP_READ,
                            NULL,
                            NULL,
                            PAGE_READONLY,
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
                               PAGE_READONLY);
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
NTSTATUS NTAPI
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
  if (KeyInfo == NULL)
    {
      NtClose (KeyHandle);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

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
          return STATUS_INSUFFICIENT_RESOURCES;
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


PIMAGE_BASE_RELOCATION NTAPI
LdrProcessRelocationBlock(IN ULONG_PTR Address,
			  IN ULONG Count,
			  IN PUSHORT TypeOffset,
			  IN LONG_PTR Delta)
{
  SHORT Offset;
  USHORT Type;
  USHORT i;
  PUSHORT ShortPtr;
  PULONG LongPtr;

  for (i = 0; i < Count; i++)
    {
      Offset = *TypeOffset & 0xFFF;
      Type = *TypeOffset >> 12;

      switch (Type)
        {
          case IMAGE_REL_BASED_ABSOLUTE:
            break;

          case IMAGE_REL_BASED_HIGH:
            ShortPtr = (PUSHORT)((ULONG_PTR)Address + Offset);
            *ShortPtr += HIWORD(Delta);
            break;

          case IMAGE_REL_BASED_LOW:
            ShortPtr = (PUSHORT)((ULONG_PTR)Address + Offset);
            *ShortPtr += LOWORD(Delta);
            break;

          case IMAGE_REL_BASED_HIGHLOW:
            LongPtr = (PULONG)((ULONG_PTR)Address + Offset);
            *LongPtr += Delta;
            break;

          case IMAGE_REL_BASED_HIGHADJ:
          case IMAGE_REL_BASED_MIPS_JMPADDR:
          default:
            DPRINT1("Unknown/unsupported fixup type %hu.\n", Type);
            return NULL;
        }

      TypeOffset++;
    }

  return (PIMAGE_BASE_RELOCATION)TypeOffset;
}

NTSTATUS
NTAPI
LdrLockLoaderLock(IN ULONG Flags,
                  OUT PULONG Disposition OPTIONAL,
                  OUT PULONG Cookie OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
LdrUnlockLoaderLock(IN ULONG Flags,
                    IN ULONG Cookie OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
NTAPI
LdrUnloadAlternateResourceModule(IN PVOID BaseAddress)
{
    UNIMPLEMENTED;
    return FALSE;
}
