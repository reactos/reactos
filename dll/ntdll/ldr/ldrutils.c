/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode Library
 * FILE:            dll/ntdll/ldr/ldrutils.c
 * PURPOSE:         Internal Loader Utility Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PLDR_DATA_TABLE_ENTRY LdrpLoadedDllHandleCache;

#define LDR_GET_HASH_ENTRY(x) (RtlUpcaseUnicodeChar((x)) & (LDR_HASH_TABLE_ENTRIES - 1))

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
LdrpCallDllEntry(PDLLMAIN_FUNC EntryPoint,
                 PVOID BaseAddress,
                 ULONG Reason,
                 PVOID Context)
{
    /* Call the entry */
    return EntryPoint(BaseAddress, Reason, Context);
}

VOID
NTAPI
LdrpTlsCallback(PVOID BaseAddress, ULONG Reason)
{
    PIMAGE_TLS_DIRECTORY TlsDirectory;
    PIMAGE_TLS_CALLBACK *Array, Callback;
    ULONG Size;

    /* Get the TLS Directory */
    TlsDirectory = RtlImageDirectoryEntryToData(BaseAddress,
                                                TRUE,
                                                IMAGE_DIRECTORY_ENTRY_TLS,
                                                &Size);

    /* Protect against invalid pointers */
    _SEH2_TRY
    {
        /* Make sure it's valid and we have an array */
        if (TlsDirectory && (Array = (PIMAGE_TLS_CALLBACK *)TlsDirectory->AddressOfCallBacks))
        {
            /* Display debug */
            if (ShowSnaps)
            {
                DPRINT1("LDR: Tls Callbacks Found. Imagebase %p Tls %p CallBacks %p\n",
                        BaseAddress, TlsDirectory, Array);
            }

            /* Loop the array */
            while (*Array)
            {
                /* Get the TLS Entrypoint */
                Callback = *Array++;

                /* Display debug */
                if (ShowSnaps)
                {
                    DPRINT1("LDR: Calling Tls Callback Imagebase %p Function %p\n",
                            BaseAddress, Callback);
                }

                /* Call it */
                LdrpCallDllEntry((PDLLMAIN_FUNC)Callback, BaseAddress, Reason, NULL);
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Do nothing */
    }
    _SEH2_END;
}

PVOID
NTAPI
LdrpFetchAddressOfEntryPoint(PVOID ImageBase)
{
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG_PTR EntryPoint;

    /* Get entry point offset from NT headers */
    NtHeaders = RtlImageNtHeader(ImageBase);
    EntryPoint = NtHeaders->OptionalHeader.AddressOfEntryPoint;

    /* If it's 0 - return so */
    if (!EntryPoint) return NULL;

    /* Add image base */
    EntryPoint += (ULONG_PTR)ImageBase;

    /* Return calculated pointer */
    return (PVOID)EntryPoint;
}

NTSTATUS
NTAPI
LdrpMapDll(IN PWSTR SearchPath OPTIONAL,
           IN PWSTR DllPath2,
           IN PWSTR DllName OPTIONAL,
           IN PULONG DllCharacteristics,
           IN BOOLEAN Static,
           IN BOOLEAN Redirect,
           OUT PLDR_DATA_TABLE_ENTRY *DataTableEntry)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

PLDR_DATA_TABLE_ENTRY
NTAPI
LdrpAllocateDataTableEntry(IN PVOID BaseAddress)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;
    PIMAGE_NT_HEADERS NtHeader = RtlImageNtHeader(BaseAddress);

    /* Make sure the header is valid */
    if (NtHeader)
    {
        /* Allocate an entry */
        LdrEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   sizeof(LDR_DATA_TABLE_ENTRY));

        /* Make sure we got one */
        if (LdrEntry)
        {
            /* Set it up */
            LdrEntry->DllBase = BaseAddress;
            LdrEntry->SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
            LdrEntry->TimeDateStamp = NtHeader->FileHeader.TimeDateStamp;
        }
    }

    /* Return the entry */
    return LdrEntry;
}

VOID
NTAPI
LdrpInsertMemoryTableEntry(IN PLDR_DATA_TABLE_ENTRY LdrEntry)
{
    PPEB_LDR_DATA PebData = NtCurrentPeb()->Ldr;
    ULONG i;

    /* Get the Hash entry */
    i = LDR_GET_HASH_ENTRY(LdrEntry->BaseDllName.Buffer[0]);

    InsertTailList(&LdrpHashTable[i], &LdrEntry->HashLinks);
    InsertTailList(&PebData->InLoadOrderModuleList, &LdrEntry->InLoadOrderLinks);
    InsertTailList(&PebData->InMemoryOrderModuleList, &LdrEntry->InMemoryOrderModuleList);
}

BOOLEAN
NTAPI
LdrpCheckForLoadedDllHandle(IN PVOID Base,
                            OUT PLDR_DATA_TABLE_ENTRY *LdrEntry)
{
    PLDR_DATA_TABLE_ENTRY Current;
    PLIST_ENTRY ListHead, Next;

    /* Check the cache first */
    if (LdrpLoadedDllHandleCache && LdrpLoadedDllHandleCache->DllBase == Base)
    {
        /* We got lucky, return the cached entry */
        *LdrEntry = LdrpLoadedDllHandleCache;
        return TRUE;
    }

    /* Time for a lookup */
    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    Next = ListHead->Flink;
    while(Next != ListHead)
    {
        /* Get the current entry */
        Current =  CONTAINING_RECORD(Next,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /* Make sure it's not unloading and check for a match */
        if ((Current->InMemoryOrderModuleList.Flink) && (Base == Current->DllBase))
        {
            /* Save in cache */
            LdrpLoadedDllHandleCache = Current;

            /* Return it */
            *LdrEntry = Current;
            return TRUE;
        }

        /* Move to the next one */
        Next = Next->Flink;
    }

    /* Nothing found */
    return FALSE;
}

BOOLEAN
NTAPI
LdrpCheckForLoadedDll(IN PWSTR DllPath,
                      IN PUNICODE_STRING DllName,
                      IN BOOLEAN Flag,
                      IN BOOLEAN RedirectedDll,
                      OUT PLDR_DATA_TABLE_ENTRY *LdrEntry)
{
    ULONG HashIndex;
    PLIST_ENTRY ListHead, ListEntry;
    PLDR_DATA_TABLE_ENTRY CurEntry;
    BOOLEAN FullPath = FALSE;
    PWCHAR wc;
    WCHAR NameBuf[266];
    UNICODE_STRING FullDllName, NtPathName;
    ULONG Length;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE FileHandle, SectionHandle;
    IO_STATUS_BLOCK Iosb;
    PVOID ViewBase = NULL;
    SIZE_T ViewSize = 0;
    PIMAGE_NT_HEADERS NtHeader, NtHeader2;
DPRINT1("LdrpCheckForLoadedDll('%S' '%wZ' %d %d %p)\n", DllPath, DllName, Flag, RedirectedDll, LdrEntry);
    /* Check if a dll name was provided */
    if (!DllName->Buffer || !DllName->Buffer[0]) return FALSE;

    /* Look in the hash table if flag was set */
lookinhash:
    if (Flag)
    {
        /* Get hash index */
        HashIndex = LDR_GET_HASH_ENTRY(DllName->Buffer[0]);

        /* Traverse that list */
        ListHead = &LdrpHashTable[HashIndex];
        ListEntry = ListHead->Flink;
        while (ListEntry != ListHead)
        {
            /* Get the current entry */
            CurEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, HashLinks);

            /* Check base name of that module */
            if (RtlEqualUnicodeString(DllName, &CurEntry->BaseDllName, TRUE))
            {
                /* It matches, return it */
                *LdrEntry = CurEntry;
                return TRUE;
            }

            /* Advance to the next entry */
            ListEntry = ListEntry->Flink;
        }

        /* Module was not found, return failure */
        return FALSE;
    }

    /* Check if there is a full path in this DLL */
    wc = DllName->Buffer;
    while (*wc)
    {
        /* Check for a slash in the current position*/
        if (*wc == L'\\' || *wc == L'/')
        {
            /* Found the slash, so dll name contains path */
            FullPath = TRUE;

            /* Setup full dll name string */
            FullDllName.Buffer = NameBuf;

            Length = RtlDosSearchPath_U(DllPath ? DllPath : LdrpDefaultPath.Buffer,
                                        DllName->Buffer,
                                        NULL,
                                        sizeof(NameBuf) - sizeof(UNICODE_NULL),
                                        FullDllName.Buffer,
                                        NULL);

            /* Check if that was successful */
            if (!Length || Length > sizeof(NameBuf) - sizeof(UNICODE_NULL))
            {
                if (ShowSnaps)
                {
                    DPRINT1("LDR: LdrpCheckForLoadedDll - Unable To Locate %ws: 0x%08x\n",
                        DllName->Buffer, Length);
                }

                /* Return failure */
                return FALSE;
            }

            /* Full dll name is found */
            FullDllName.Length = Length;
            FullDllName.MaximumLength = FullDllName.Length + sizeof(UNICODE_NULL);
            break;
        }

        wc++;
    }

    /* Go check the hash table */
    if (!FullPath)
    {
        Flag = TRUE;
        goto lookinhash;
    }

    /* Now go through the InLoadOrder module list */
    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    ListEntry = ListHead->Flink;

    while (ListEntry != ListHead)
    {
        /* Get the containing record of the current entry and advance to the next one */
        CurEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        ListEntry = ListEntry->Flink;

        /* Check if it's already being unloaded */
        if (!CurEntry->InMemoryOrderModuleList.Flink) continue;

        /* Check if name matches */
        if (RtlEqualUnicodeString(&FullDllName,
                                  &CurEntry->FullDllName,
                                  TRUE))
        {
            /* Found it */
            *LdrEntry = CurEntry;

            /* Find activation context */
            Status = RtlFindActivationContextSectionString(0, NULL, ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, DllName, NULL);
            if (!NT_SUCCESS(Status))
                return FALSE;
            else
                return TRUE;
        }
    }

    /* The DLL was not found in the load order modules list. Perform a complex check */

    /* Convert given path to NT path */
    if (!RtlDosPathNameToNtPathName_U(FullDllName.Buffer,
                                      &NtPathName,
                                      NULL,
                                      NULL))
    {
        /* Fail if conversion failed */
        return FALSE;
    }

    /* Initialize object attributes and open it */
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        SYNCHRONIZE | FILE_EXECUTE,
                        &ObjectAttributes,
                        &Iosb,
                        FILE_SHARE_READ | FILE_SHARE_DELETE,
                        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

    /* Free NT path name */
    RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);

    /* If opening the file failed - return failure */
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Create a section for this file */
    Status = NtCreateSection(&SectionHandle,
                             SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_MAP_WRITE,
                             NULL,
                             NULL,
                             PAGE_EXECUTE,
                             SEC_COMMIT,
                             FileHandle);

    /* Close file handle */
    NtClose(FileHandle);

    /* If creating section failed - return failure */
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Map view of this section */
    Status = ZwMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                &ViewBase,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_EXECUTE);
    /* Close section handle */
    NtClose(SectionHandle);

    /* If section mapping failed - return failure */
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Get pointer to the NT header of this section */
    NtHeader = RtlImageNtHeader(ViewBase);
    if (!NtHeader)
    {
        /* Unmap the section and fail */
        NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);
        return FALSE;
    }

    /* Go through the list of modules */
    ListHead = &NtCurrentPeb()->Ldr->InLoadOrderModuleList;
    ListEntry = ListHead->Flink;

    while (ListEntry != ListHead)
    {
        CurEntry = CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        ListEntry = ListEntry->Flink;

        /* Check if it's already being unloaded */
        if (!CurEntry->InMemoryOrderModuleList.Flink) continue;

        _SEH2_TRY
        {
            /* Check if timedate stamp and sizes match */
            if (CurEntry->TimeDateStamp == NtHeader->FileHeader.TimeDateStamp &&
                CurEntry->SizeOfImage == NtHeader->OptionalHeader.SizeOfImage)
            {
                /* Time, date and size match. Let's compare their headers */
                NtHeader2 = RtlImageNtHeader(CurEntry->DllBase);

                if (RtlCompareMemory(NtHeader2, NtHeader, sizeof(IMAGE_NT_HEADERS)))
                {
                    /* Headers match too! Finally ask the kernel to compare mapped files */
                    Status = ZwAreMappedFilesTheSame(CurEntry->DllBase, ViewBase);

                    if (!NT_SUCCESS(Status))
                    {
                        _SEH2_YIELD(continue;)
                    }
                    else
                    {
                        /* This is our entry! */
                        *LdrEntry = CurEntry;

                        /* Unmap the section */
                        NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);

                        _SEH2_YIELD(return TRUE;)
                    }
                }
            }
        }
        _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(break;)
        }
        _SEH2_END;
    }

    /* Unmap the section */
    NtUnmapViewOfSection(NtCurrentProcess(), ViewBase);

    return FALSE;
}

NTSTATUS
NTAPI
LdrpGetProcedureAddress(IN PVOID BaseAddress,
                        IN PANSI_STRING Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress,
                        IN BOOLEAN ExecuteInit)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UCHAR ImportBuffer[64];
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    IMAGE_THUNK_DATA Thunk;
    PVOID ImageBase;
    PIMAGE_IMPORT_BY_NAME ImportName = NULL;
    PIMAGE_EXPORT_DIRECTORY ExportDir;
    ULONG ExportDirSize;
    PLIST_ENTRY Entry;

    /* Show debug message */
    if (ShowSnaps) DPRINT1("LDR: LdrGetProcedureAddress by ");

    /* Check if we got a name */
    if (Name)
    {
        /* Show debug message */
        if (ShowSnaps) DPRINT1("NAME - %s\n", Name->Buffer);

        /* Make sure it's not too long */
        if ((Name->Length + sizeof(CHAR) + sizeof(USHORT)) > MAXLONG)
        {
            /* Won't have enough space to add the hint */
            return STATUS_NAME_TOO_LONG;
        }

        /* Check if our buffer is large enough */
        if (Name->Length >= (sizeof(ImportBuffer) - sizeof(CHAR)))
        {
            /* Allocate from heap, plus 2 bytes for the Hint */
            ImportName = RtlAllocateHeap(RtlGetProcessHeap(),
                0,
                Name->Length + sizeof(CHAR) +
                sizeof(USHORT));
        }
        else
        {
            /* Use our internal buffer */
            ImportName = (PIMAGE_IMPORT_BY_NAME)ImportBuffer;
        }

        /* Clear the hint */
        ImportName->Hint = 0;

        /* Copy the name and null-terminate it */
        RtlMoveMemory(&ImportName->Name, Name->Buffer, Name->Length);
        ImportName->Name[Name->Length + 1] = 0;

        /* Clear the high bit */
        ImageBase = ImportName;
        Thunk.u1.AddressOfData = 0;
    }
    else
    {
        /* Do it by ordinal */
        ImageBase = NULL;

        /* Show debug message */
        if (ShowSnaps) DPRINT1("ORDINAL - %lx\n", Ordinal);

        if (Ordinal)
        {
            Thunk.u1.Ordinal = Ordinal | IMAGE_ORDINAL_FLAG;
        }
        else
        {
            /* No ordinal */
            DPRINT1("No ordinal and no name\n");
            return STATUS_INVALID_PARAMETER;
        }
    }

    /* Acquire lock unless we are initting */
    if (!LdrpInLdrInit) RtlEnterCriticalSection(&LdrpLoaderLock);

    _SEH2_TRY
    {
        /* Try to find the loaded DLL */
        if (!LdrpCheckForLoadedDllHandle(BaseAddress, &LdrEntry))
        {
            /* Invalid base */
            DPRINT1("Invalid base address\n");
            Status = STATUS_DLL_NOT_FOUND;
            _SEH2_YIELD(goto Quickie;)
        }

        /* Get the pointer to the export directory */
        ExportDir = RtlImageDirectoryEntryToData(LdrEntry->DllBase,
                                                 TRUE,
                                                 IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                 &ExportDirSize);

        if (!ExportDir)
        {
            DPRINT1("Image has no exports\n");
            Status = STATUS_PROCEDURE_NOT_FOUND;
            _SEH2_YIELD(goto Quickie;)
        }

        /* Now get the thunk */
        Status = LdrpSnapThunk(LdrEntry->DllBase,
                               ImageBase,
                               &Thunk,
                               &Thunk,
                               ExportDir,
                               ExportDirSize,
                               FALSE,
                               NULL);

        /* Finally, see if we're supposed to run the init routines */
        if (ExecuteInit)
        {
            /*
            * It's possible a forwarded entry had us load the DLL. In that case,
            * then we will call its DllMain. Use the last loaded DLL for this.
            */
            Entry = NtCurrentPeb()->Ldr->InInitializationOrderModuleList.Blink;
            LdrEntry = CONTAINING_RECORD(Entry,
                                         LDR_DATA_TABLE_ENTRY,
                                         InInitializationOrderModuleList);

            /* Make sure we didn't process it yet*/
            if (!(LdrEntry->Flags & LDRP_ENTRY_PROCESSED))
            {
                /* Call the init routine */
                _SEH2_TRY
                {
                    Status = LdrpRunInitializeRoutines(NULL);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    /* Get the exception code */
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
        }

        /* Make sure we're OK till here */
        if (NT_SUCCESS(Status))
        {
            /* Return the address */
            *ProcedureAddress = (PVOID)Thunk.u1.Function;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Just ignore exceptions */
    }
    _SEH2_END;

Quickie:
    /* Cleanup */
    if (ImportName && (ImportName != (PIMAGE_IMPORT_BY_NAME)ImportBuffer))
    {
        /* We allocated from heap, free it */
        RtlFreeHeap(RtlGetProcessHeap(), 0, ImportName);
    }

    /* Release the CS if we entered it */
    if (!LdrpInLdrInit) RtlLeaveCriticalSection(&LdrpLoaderLock);

    /* We're done */
    return Status;
}

NTSTATUS
NTAPI
LdrpLoadDll(IN BOOLEAN Redirected,
            IN PWSTR DllPath OPTIONAL,
            IN PULONG DllCharacteristics OPTIONAL,
            IN PUNICODE_STRING DllName,
            OUT PVOID *BaseAddress,
            IN BOOLEAN CallInit)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

ULONG
NTAPI
LdrpClearLoadInProgress()
{
    PLIST_ENTRY ListHead;
    PLIST_ENTRY Entry;
    PLDR_DATA_TABLE_ENTRY Module;
    ULONG ModulesCount = 0;

    /* Traverse the init list */
    ListHead = &NtCurrentPeb()->Ldr->InInitializationOrderModuleList;
    Entry = ListHead->Flink;

    while (Entry != ListHead)
    {
        Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InInitializationOrderModuleList);

        /* Clear load in progress flag */
        Module->Flags &= ~LDRP_LOAD_IN_PROGRESS;

        /* Increase counter for modules with entry point count but not processed yet */
        if (Module->EntryPoint &&
            !(Module->Flags & LDRP_ENTRY_PROCESSED)) ModulesCount++;

        /* Advance to the next entry */
        Entry = Entry->Flink;
    }

    return ModulesCount;
}

/* EOF */
