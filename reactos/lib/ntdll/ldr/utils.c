/* $Id: utils.c,v 1.67 2003/07/11 13:50:23 royce Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/utils.c
 * PURPOSE:         Process startup for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/*
 * TODO:
 *      - Fix calling of entry points
 *      - Handle loading flags correctly
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


#ifdef DBG_NTDLL_LDR_UTILS
#define NDEBUG
#endif
#include <ntdll/ntdll.h>

/* GLOBALS *******************************************************************/

static HANDLE LdrpKnownDllsDirHandle = NULL;
static UNICODE_STRING LdrpKnownDllPath = {0, 0, NULL};


/* PROTOTYPES ****************************************************************/

static NTSTATUS LdrFindEntryForName(PUNICODE_STRING Name, PLDR_MODULE *Module);
static PVOID LdrFixupForward(PCHAR ForwardName);
static PVOID LdrGetExportByName(PVOID BaseAddress, PUCHAR SymbolName, USHORT Hint);


/* FUNCTIONS *****************************************************************/


#ifdef DBG

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


VOID
LdrpInitLoader(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING LinkTarget;
  UNICODE_STRING Name;
  HANDLE LinkHandle;
  ULONG Length;
  NTSTATUS Status;

  DPRINT("LdrpInitLoader() called\n");

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

   if (BaseName == TRUE)
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
  PLDR_MODULE           Module;
  Module = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof (LDR_MODULE));
  assert(Module);
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

  Module->TlsIndex = 0;
  Module->CheckSum = NTHeaders->OptionalHeader.CheckSum;
  Module->TimeDateStamp = NTHeaders->FileHeader.TimeDateStamp;

  RtlCreateUnicodeString (&Module->FullDllName,
                          FullDosName);
  RtlCreateUnicodeString (&Module->BaseDllName,
                          wcsrchr(FullDosName, L'\\') + 1);
  DPRINT ("BaseDllName %wZ\n", &Module->BaseDllName);
  
  /* FIXME: aquire loader lock */
  InsertTailList(&NtCurrentPeb()->Ldr->InLoadOrderModuleList,
                 &Module->InLoadOrderModuleList);
  InsertTailList(&NtCurrentPeb()->Ldr->InInitializationOrderModuleList,
                 &Module->InInitializationOrderModuleList);
  /* FIXME: release loader lock */

  return(Module);
}


static NTSTATUS
LdrpMapKnownDll(IN PUNICODE_STRING DllName,
		OUT PUNICODE_STRING FullDosName,
		OUT PHANDLE SectionHandle)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING ObjectDirName;
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

  Status = ZwOpenFile(&FileHandle,
                      FILE_ALL_ACCESS,
                      &FileObjectAttributes,
                      NULL,
                      0,
                      0);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Dll open of %wZ failed: Status = 0x%08x\n", 
               &FullNtFileName, Status);
      RtlFreeUnicodeString (&FullNtFileName);
      return Status;
    }
  RtlFreeUnicodeString (&FullNtFileName);

  Status = ZwReadFile(FileHandle,
                      0,
                      0,
                      0,
                      0,
                      BlockBuffer,
                      sizeof(BlockBuffer),
                      0,
                      0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Dll header read failed: Status = 0x%08x\n", Status);
      ZwClose(FileHandle);
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
      ZwClose(FileHandle);
      
      return STATUS_UNSUCCESSFUL;
    }
  
  ImageBase = (PVOID) NTHeaders->OptionalHeader.ImageBase;
  ImageSize = NTHeaders->OptionalHeader.SizeOfImage;
  
  DPRINT("ImageBase 0x%08x\n", ImageBase);
  
  /*
   * Create a section for dll.
   */
  Status = ZwCreateSection(SectionHandle,
                           SECTION_ALL_ACCESS,
                           NULL,
                           NULL,
                           PAGE_READWRITE,
                           SEC_COMMIT | SEC_IMAGE,
                           FileHandle);
  ZwClose(FileHandle);

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
  UNICODE_STRING        FullDosName;
  UNICODE_STRING        AdjustedName;
  NTSTATUS              Status;
  PIMAGE_NT_HEADERS     NTHeaders;
  ULONG                 ViewSize;
  PVOID                 ImageBase;
  HANDLE                SectionHandle;
  PDLLMAIN_FUNC         Entrypoint = NULL;
  PLDR_MODULE           Module;


  if (Name == NULL)
    {
      *BaseAddress = NtCurrentPeb()->ImageBaseAddress;
      return STATUS_SUCCESS;
    }

  *BaseAddress = NULL;

  DPRINT("LdrLoadDll(Name \"%wZ\" BaseAddress %x)\n",
         Name, BaseAddress);

  /* adjust the full dll name */
  LdrAdjustDllName (&AdjustedName,
                    Name,
                    FALSE);
  DPRINT("AdjustedName: %wZ\n", &AdjustedName);

  /*
   * Test if dll is already loaded.
   */
  if (LdrFindEntryForName(&AdjustedName, &Module) == STATUS_SUCCESS)
    {
      DPRINT("DLL %wZ already loaded.\n", &AdjustedName);
      if (Module->LoadCount != -1)
        Module->LoadCount++;
      *BaseAddress = Module->BaseAddress;
      return STATUS_SUCCESS;
    }
  DPRINT("Loading \"%wZ\"\n", Name);

  /* Open or create dll image section */
  Status = LdrpMapKnownDll(&AdjustedName,
			   &FullDosName,
			   &SectionHandle);
  if (!NT_SUCCESS(Status))
    {
      Status = LdrpMapDllImageFile(SearchPath,
				   &AdjustedName,
				   &FullDosName,
				   &SectionHandle);
    }

  RtlFreeUnicodeString(&AdjustedName);

  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to create or open dll section (Status %lx)\n", Status);
      return Status;
    }

  /*
   * Map the dll into the process.
   */
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
      DbgPrint("NTDLL.LDR: map view of section failed (Status %x)\n", Status);
      RtlFreeUnicodeString(&FullDosName);
      NtClose(SectionHandle);
      return(Status);
    }

  /* Get and check the NT headers */
  NTHeaders = RtlImageNtHeader(ImageBase);
  if (NTHeaders == NULL)
    {
      DPRINT1("RtlImageNtHeaders() failed\n");
      RtlFreeUnicodeString(&FullDosName);
      return STATUS_UNSUCCESSFUL;
    }

  /* relocate dll and fixup import table */
  if ((NTHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL) ==
      IMAGE_FILE_DLL)
    {
      Entrypoint =
        (PDLLMAIN_FUNC) LdrPEStartup(ImageBase, SectionHandle, &Module,
                                     FullDosName.Buffer);
      if (Entrypoint == NULL)
        {
          RtlFreeUnicodeString(&FullDosName);
          return(STATUS_UNSUCCESSFUL);
        }
    }

  RtlFreeUnicodeString(&FullDosName);

#ifdef DBG

  LdrpLoadUserModuleSymbols(Module);

#endif /* DBG */

  /* initialize dll */
  if ((NTHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL) ==
      IMAGE_FILE_DLL)
    {
      if (Module->EntryPoint != 0)
        {
          Entrypoint = (PDLLMAIN_FUNC)Module->EntryPoint;
          
          DPRINT("Calling entry point at 0x%08x\n", Entrypoint);
          if (FALSE == Entrypoint(Module->BaseAddress,
                                  DLL_PROCESS_ATTACH,
                                  NULL))
            {
	      /* Do this as a DPRINT1 for now, until clean up and fail implemented */
              DPRINT1("NTDLL.LDR: DLL \"%wZ\" failed to initialize\n",
                     &Module->BaseDllName);
              /* FIXME: should clean up and fail */
            }
          else
            {
              DPRINT("NTDLL.LDR: DLL \"%wZ\" initialized successfully\n",
                     &Module->BaseDllName);
            }
        }
      else
        {
          DPRINT("NTDLL.LDR: Entrypoint is NULL for \"%wZ\"\n",
                 &Module->BaseDllName);
        }
    }

  *BaseAddress = Module->BaseAddress;
  return STATUS_SUCCESS;
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

  DPRINT("NTDLL.LdrFindEntryForAddress(Address %p)\n", Address);

  if (NtCurrentPeb()->Ldr == NULL)
    return(STATUS_NO_MORE_ENTRIES);

  ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
  Entry = ModuleListHead->Flink;
  if (Entry == ModuleListHead)
    return(STATUS_NO_MORE_ENTRIES);

  while (Entry != ModuleListHead)
    {
      ModulePtr = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);

      DPRINT("Scanning %wZ at %p\n", &ModulePtr->BaseDllName, ModulePtr->BaseAddress);

      if ((Address >= ModulePtr->BaseAddress) &&
          (Address <= (ModulePtr->BaseAddress + ModulePtr->SizeOfImage)))
        {
          *Module = ModulePtr;
          return(STATUS_SUCCESS);
        }

      Entry = Entry->Flink;
    }

  DPRINT("Failed to find module entry.\n");

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
                    PLDR_MODULE *Module)
{
  PLIST_ENTRY ModuleListHead;
  PLIST_ENTRY Entry;
  PLDR_MODULE ModulePtr;
  BOOLEAN ContainsPath;
  unsigned i;

  DPRINT("NTDLL.LdrFindEntryForName(Name %wZ)\n", Name);

  if (NtCurrentPeb()->Ldr == NULL)
    return(STATUS_NO_MORE_ENTRIES);

  ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
  Entry = ModuleListHead->Flink;
  if (Entry == ModuleListHead)
    return(STATUS_NO_MORE_ENTRIES);

  // NULL is the current process
  if (Name == NULL)
    {
      *Module = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);
      return(STATUS_SUCCESS);
    }

  ContainsPath = (2 <= Name->Length && L':' == Name->Buffer[1]);
  for (i = 0; ! ContainsPath && i < Name->Length; i++)
    {
    ContainsPath = L'\\' == Name->Buffer[i] ||
                   L'/' == Name->Buffer[i];
    }
  while (Entry != ModuleListHead)
    {
      ModulePtr = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);

      DPRINT("Scanning %wZ %wZ\n", &ModulePtr->BaseDllName, Name);

      if ((! ContainsPath &&
           0 == RtlCompareUnicodeString(&ModulePtr->BaseDllName, Name, TRUE)) ||
          (ContainsPath &&
           0 == RtlCompareUnicodeString(&ModulePtr->FullDllName, Name, TRUE)))
        {
          *Module = ModulePtr;
          return(STATUS_SUCCESS);
        }

      Entry = Entry->Flink;
    }

  DPRINT("Failed to find dll %wZ\n", Name);

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
   UNICODE_STRING FunctionName;
   NTSTATUS Status;
   PCHAR p;
   PVOID BaseAddress;

   strcpy(NameBuffer, ForwardName);
   p = strchr(NameBuffer, '.');
   if (p != NULL)
     {
        *p = 0;

        DPRINT("Dll: %s  Function: %s\n", NameBuffer, p+1);
        RtlCreateUnicodeStringFromAsciiz (&DllName,
                                          NameBuffer);

        Status = LdrGetDllHandle (0, 0, &DllName, &BaseAddress);
        if (!NT_SUCCESS(Status))
          {
             Status = LdrLoadDll(NULL,
                                 0,
                                 &DllName,
                                 &BaseAddress);
             if (!NT_SUCCESS(Status))
               {
                  DbgPrint("LdrFixupForward: failed to load %wZ\n", &DllName);
                  RtlFreeUnicodeString (&DllName);
                  return NULL;
               }
          }

        RtlFreeUnicodeString (&DllName);
        DPRINT("BaseAddress: %p\n", BaseAddress);
        
        return LdrGetExportByName(BaseAddress, p+1, -1);
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
        PDWORD                  * ExFunctions;
        USHORT                  * ExOrdinals;

        ExportDir = (PIMAGE_EXPORT_DIRECTORY)
                RtlImageDirectoryEntryToData (BaseAddress,
                                              TRUE,
                                              IMAGE_DIRECTORY_ENTRY_EXPORT,
                                              NULL);


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
        DbgPrint(
                "LdrGetExportByOrdinal(Ordinal %d) = %x\n",
                Ordinal,
                RVA(BaseAddress, ExFunctions[Ordinal - ExportDir->Base] )
                );
        return(RVA(BaseAddress, ExFunctions[Ordinal - ExportDir->Base] ));
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
   ULONG minn, maxn;
   ULONG ExportDirSize;
   
   DPRINT("LdrGetExportByName %x %s %hu\n", BaseAddress, SymbolName, Hint);

   ExportDir = (PIMAGE_EXPORT_DIRECTORY)
     RtlImageDirectoryEntryToData(BaseAddress,
                                  TRUE,
                                  IMAGE_DIRECTORY_ENTRY_EXPORT,
                                  &ExportDirSize);
   if (ExportDir == NULL)
     {
        DbgPrint("LdrGetExportByName(): no export directory!\n");
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
               }
             if (Function != NULL)
               return Function;
          }
     }
   
   /*
    * Try a binary search first
    */
   minn = 0;
   maxn = ExportDir->NumberOfNames;
   while (minn <= maxn)
     {
        ULONG mid;
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
             return Function;
          }
     }
   DbgPrint("LdrGetExportByName(): failed to find %s\n",SymbolName);
   return NULL;
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
  ULONG LastOffset;

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
static NTSTATUS LdrFixupImports(PIMAGE_NT_HEADERS       NTHeaders,
                                PVOID                   ImageBase)
{
   PIMAGE_IMPORT_MODULE_DIRECTORY ImportModuleDirectory;
   ULONG Ordinal;
   PVOID BaseAddress;
   NTSTATUS Status;
   ULONG IATSize;
   
   DPRINT("LdrFixupImports(NTHeaders %x, ImageBase %x)\n", NTHeaders, 
           ImageBase);
   
   /*
    * Process each import module.
    */
   ImportModuleDirectory = (PIMAGE_IMPORT_MODULE_DIRECTORY)(
                               ImageBase + NTHeaders->OptionalHeader
                                 .DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
                             .VirtualAddress);
   DPRINT("ImportModuleDirectory %x\n", ImportModuleDirectory);

   while (ImportModuleDirectory->dwRVAModuleName)
     {
        PVOID   * ImportAddressList;
        PULONG  FunctionNameList;
        UNICODE_STRING DllName;
        DWORD   pName;
        WORD    pHint;
        PVOID   IATBase;
        ULONG   OldProtect;

        DPRINT("ImportModule->Directory->dwRVAModuleName %s\n",
               (PCHAR)(ImageBase + ImportModuleDirectory->dwRVAModuleName));

        RtlCreateUnicodeStringFromAsciiz (&DllName,
                  (PCHAR)(ImageBase + ImportModuleDirectory->dwRVAModuleName));

        Status = LdrGetDllHandle (0, 0, &DllName, &BaseAddress);
        if (!NT_SUCCESS(Status))
          {
             Status = LdrLoadDll(NULL,
                                 0,
                                 &DllName,
                                 &BaseAddress);
             RtlFreeUnicodeString (&DllName);
             if (!NT_SUCCESS(Status))
               {
                  DbgPrint("LdrFixupImports:failed to load %s\n"
                        ,(PCHAR)(ImageBase 
                                + ImportModuleDirectory->dwRVAModuleName));

                  return Status;
               }
          }

        /*
         * Get the import address list.
         */
        ImportAddressList = (PVOID *)(ImageBase
                        + ImportModuleDirectory->dwRVAFunctionAddressList);
        
        /*
         * Get the list of functions to import.
         */
        if (ImportModuleDirectory->dwRVAFunctionNameList != 0)
          {
             FunctionNameList = (PULONG) (
                                          ImageBase
                                          + ImportModuleDirectory->dwRVAFunctionNameList
                                          );
          }
        else
          {
             FunctionNameList = 
               (PULONG)(ImageBase 
                        + ImportModuleDirectory->dwRVAFunctionAddressList);
          }

        /*
         * Get the size of IAT.
         */
        IATSize = 0;
        while (FunctionNameList[IATSize] != 0L)
          {
            IATSize++;
          }

        /*
         * Unprotect the region we are about to write into.
         */
        IATBase = (PVOID)ImportAddressList;
        Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                        IATBase,
                                        IATSize * sizeof(PVOID*),
                                        PAGE_READWRITE,
                                        &OldProtect);
        if (!NT_SUCCESS(Status))
          {
            DbgPrint("LDR: Failed to unprotect IAT.\n");
            return(Status);
          }

        /*
         * Walk through function list and fixup addresses.
         */
        while (*FunctionNameList != 0L)
          {
             if ((*FunctionNameList) & 0x80000000)
               {
                  Ordinal = (*FunctionNameList) & 0x7fffffff;
                  *ImportAddressList = 
                    LdrGetExportByOrdinal(BaseAddress,
                                          Ordinal);
               }
             else
               {
                  pName = (DWORD) (ImageBase + *FunctionNameList + 2);
                  pHint = *(PWORD)(ImageBase + *FunctionNameList);

                  *ImportAddressList = 
                    LdrGetExportByName(BaseAddress, (PUCHAR)pName, pHint);
                  if ((*ImportAddressList) == NULL)
                    {
                       DbgPrint("Failed to import %s\n", pName);
                       return STATUS_UNSUCCESSFUL;
                    }
               }
             ImportAddressList++;
             FunctionNameList++;
          }

        /*
         * Protect the region we are about to write into.
         */
        Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                        IATBase,
                                        IATSize * sizeof(PVOID*),
                                        OldProtect,
                                        &OldProtect);
        if (!NT_SUCCESS(Status))
          {
            DbgPrint("LDR: Failed to protect IAT.\n");
            return(Status);
          }

        ImportModuleDirectory++;
     }
   return STATUS_SUCCESS;
}


/**********************************************************************
 * NAME
 *      LdrPEStartup
 *
 * DESCRIPTION
 *      1. Map the DLL's sections into memory.
 *      2. Relocate, if needed the DLL.
 *      3. Fixup any imported symbol.
 *      4. Compute the DLL's entry point.
 *
 * ARGUMENTS
 *      ImageBase
 *              Address at which the DLL's image
 *              is loaded.
 *              
 *      SectionHandle
 *              Handle of the section that contains
 *              the DLL's image.
 *
 * RETURN VALUE
 *      NULL on error; otherwise the entry point
 *      to call for initializing the DLL.
 *
 * REVISIONS
 *
 * NOTE
 *
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
       DbgPrint("LDR: Performing relocations\n");
       Status = LdrPerformRelocations(NTHeaders, ImageBase);
       if (!NT_SUCCESS(Status))
         {
           DbgPrint("LdrPerformRelocations() failed\n");
           return NULL;
         }
     }

   if (Module != NULL)
     {
       *Module = LdrAddModuleEntry(ImageBase, NTHeaders, FullDosName);
     }

   /*
    * If the DLL's imports symbols from other
    * modules, fixup the imported calls entry points.
    */
   if (NTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
       .VirtualAddress != 0)
     {
        DPRINT("About to fixup imports\n");
        Status = LdrFixupImports(NTHeaders, ImageBase);
        if (!NT_SUCCESS(Status))
          {
             DbgPrint("LdrFixupImports() failed\n");
             return NULL;
          }
        DPRINT("Fixup done\n");
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


/*
 * @implemented
 */
NTSTATUS STDCALL
LdrUnloadDll (IN PVOID BaseAddress)
{
   PIMAGE_NT_HEADERS NtHeaders;
   PDLLMAIN_FUNC Entrypoint;
   PLIST_ENTRY ModuleListHead;
   PLIST_ENTRY Entry;
   PLDR_MODULE Module;
   NTSTATUS Status;

   if (BaseAddress == NULL)
     return STATUS_SUCCESS;

   ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
   Entry = ModuleListHead->Flink;

   while (Entry != ModuleListHead)
     {
        Module = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);
        if (Module->BaseAddress == BaseAddress)
          {
             if (Module->LoadCount == -1)
               {
                  /* never unload this dll */
                  return STATUS_SUCCESS;
               }
             else if (Module->LoadCount > 1)
               {
                  Module->LoadCount--;
                  return STATUS_SUCCESS;
               }

             NtHeaders = RtlImageNtHeader (Module->BaseAddress);
             if ((NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DLL) == IMAGE_FILE_DLL)
               {
                  if (Module->EntryPoint != 0)
                    {
                       Entrypoint = (PDLLMAIN_FUNC)Module->EntryPoint;
                       DPRINT("Calling entry point at 0x%08x\n", Entrypoint);
                       Entrypoint(Module->BaseAddress,
                                  DLL_PROCESS_DETACH,
                                  NULL);
                    }
                  else
                    {
                       DPRINT("NTDLL.LDR: Entrypoint is NULL for \n");
                    }
               }
             Status = ZwUnmapViewOfSection (NtCurrentProcess (),
                                            Module->BaseAddress);
             ZwClose (Module->SectionHandle);

             /* remove the module entry from the list */
             RtlFreeUnicodeString (&Module->FullDllName);
             RtlFreeUnicodeString (&Module->BaseDllName);
             RemoveEntryList (Entry);
             RtlFreeHeap (RtlGetProcessHeap (), 0, Module);

             return Status;
          }

        Entry = Entry->Flink;
     }

   DPRINT("NTDLL.LDR: Dll not found\n")

   return STATUS_UNSUCCESSFUL;
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
    ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Entry = ModuleListHead->Flink;
    while (Entry != ModuleListHead) {
        Module = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);

        DPRINT("BaseDllName %wZ BaseAddress %x\n", &Module->BaseDllName, Module->BaseAddress);

        if (Module->BaseAddress == BaseAddress) {
            if (Module->TlsIndex == 0) {
                Module->Flags |= 0x00040000;
                Status = STATUS_SUCCESS;
            }
            return Status;
        }
        Entry = Entry->Flink;
    }
    return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LdrGetDllHandle(IN ULONG Unknown1,
                IN ULONG Unknown2,
                IN PUNICODE_STRING DllName,
                OUT PVOID* BaseAddress)
{
    UNICODE_STRING FullDllName;
    PLIST_ENTRY ModuleListHead;
    PLIST_ENTRY Entry;
    PLDR_MODULE Module;

    DPRINT("LdrGetDllHandle (Unknown1 %x Unknown2 %x DllName %wZ BaseAddress %p)\n",
           Unknown1, Unknown2, DllName, BaseAddress);

    /* NULL is the current executable */
    if (DllName == NULL) {
        *BaseAddress = NtCurrentPeb()->ImageBaseAddress;
        DPRINT("BaseAddress %x\n", *BaseAddress);
        return STATUS_SUCCESS;
    }
    LdrAdjustDllName(&FullDllName, DllName, TRUE);

    DPRINT("FullDllName %wZ\n", &FullDllName);

    ModuleListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Entry = ModuleListHead->Flink;
    while (Entry != ModuleListHead) {
        Module = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);

        DPRINT("EntryPoint %x\n", Module->EntryPoint);
        DPRINT("Comparing %wZ and %wZ\n", &Module->BaseDllName, &FullDllName);

        if (!RtlCompareUnicodeString(&Module->BaseDllName, &FullDllName, TRUE)) {
             RtlFreeUnicodeString(&FullDllName);
             *BaseAddress = Module->BaseAddress;
             DPRINT("BaseAddress %x\n", *BaseAddress);
             return STATUS_SUCCESS;
        }
        Entry = Entry->Flink;
    }

    DPRINT("Failed to find dll %wZ\n", &FullDllName);

    RtlFreeUnicodeString(&FullDllName);
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
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   PUSHORT OrdinalPtr;
   PULONG NamePtr;
   PULONG AddressPtr;
   ULONG i = 0;

   DPRINT("LdrGetProcedureAddress (BaseAddress %x Name %Z Ordinal %lu ProcedureAddress %x)\n",
          BaseAddress, Name, Ordinal, ProcedureAddress);

   /* Get the pointer to the export directory */
   ExportDir = (PIMAGE_EXPORT_DIRECTORY)
                RtlImageDirectoryEntryToData (BaseAddress,
                                              TRUE,
                                              IMAGE_DIRECTORY_ENTRY_EXPORT,
                                              &i);

   DPRINT("ExportDir %x i %lu\n", ExportDir, i);

   if (!ExportDir || !i || !ProcedureAddress)
     {
        return STATUS_INVALID_PARAMETER;
     }

   AddressPtr = (PULONG)((ULONG)BaseAddress + (ULONG)ExportDir->AddressOfFunctions);
   if (Name && Name->Length)
     {
        /* by name */
        OrdinalPtr = (PUSHORT)((ULONG)BaseAddress + (ULONG)ExportDir->AddressOfNameOrdinals);
        NamePtr = (PULONG)((ULONG)BaseAddress + (ULONG)ExportDir->AddressOfNames);
        for( i = 0; i < ExportDir->NumberOfNames; i++, NamePtr++, OrdinalPtr++)
          {
             if (!_strnicmp(Name->Buffer, (char*)(BaseAddress + *NamePtr), Name->Length))
               {
                  *ProcedureAddress = (PVOID)((ULONG)BaseAddress + (ULONG)AddressPtr[*OrdinalPtr]);
                  return STATUS_SUCCESS;
               }
          }
        DPRINT("LdrGetProcedureAddress: Can't resolve symbol '%Z'\n", Name);
     }
   else
     {
        /* by ordinal */
        Ordinal &= 0x0000FFFF;
        if (Ordinal - ExportDir->Base < ExportDir->NumberOfFunctions)
          {
             *ProcedureAddress = (PVOID)((ULONG)BaseAddress + (ULONG)AddressPtr[Ordinal - ExportDir->Base]);
             return STATUS_SUCCESS;
          }
        DPRINT("LdrGetProcedureAddress: Can't resolve symbol @%d\n", Ordinal);
  }

   return STATUS_PROCEDURE_NOT_FOUND;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LdrShutdownProcess (VOID)
{
   PLIST_ENTRY ModuleListHead;
   PLIST_ENTRY Entry;
   PLDR_MODULE Module;

   DPRINT("LdrShutdownProcess() called\n");

   RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);

   ModuleListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
   Entry = ModuleListHead->Blink;

   while (Entry != ModuleListHead)
     {
        Module = CONTAINING_RECORD(Entry, LDR_MODULE, InInitializationOrderModuleList);

        DPRINT("  Unloading %wZ\n",
               &Module->BaseDllName);
        // PJS: only detach from static dlls, they should FreeLibrary() any dlls that
        // they loaded dynamically, and when the last reference is gone, that lib will
        // be detached.  
        if (Module->EntryPoint != 0 && Module->LoadCount == -1)
          {
             PDLLMAIN_FUNC Entrypoint = (PDLLMAIN_FUNC)Module->EntryPoint;

             DPRINT("Calling entry point at 0x%08x\n", Entrypoint);
             Entrypoint (Module->BaseAddress,
                         DLL_PROCESS_DETACH,
                         NULL);
          }

        Entry = Entry->Blink;
     }

   RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);

   DPRINT("LdrShutdownProcess() done\n");

   return STATUS_SUCCESS;
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

   DPRINT("LdrShutdownThread() called\n");

   RtlEnterCriticalSection (NtCurrentPeb()->LoaderLock);

   ModuleListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
   Entry = ModuleListHead->Blink;

   while (Entry != ModuleListHead)
     {
        Module = CONTAINING_RECORD(Entry, LDR_MODULE, InInitializationOrderModuleList);

        DPRINT("  Unloading %wZ\n",
               &Module->BaseDllName);

        if (Module->EntryPoint != 0)
          {
             PDLLMAIN_FUNC Entrypoint = (PDLLMAIN_FUNC)Module->EntryPoint;

             DPRINT("Calling entry point at 0x%08x\n", Entrypoint);
             Entrypoint (Module->BaseAddress,
                         DLL_THREAD_DETACH,
                         NULL);
          }

        Entry = Entry->Blink;
     }

   RtlLeaveCriticalSection (NtCurrentPeb()->LoaderLock);

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

/* EOF */
