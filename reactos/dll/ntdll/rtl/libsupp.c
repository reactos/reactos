/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS NT User-Mode DLL
 * FILE:            lib/ntdll/rtl/libsup.c
 * PURPOSE:         RTL Support Routines
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Gunnar Dalsnes
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>

#define NDEBUG
#include <debug.h>

SIZE_T RtlpAllocDeallocQueryBufferSize = PAGE_SIZE;
PTEB LdrpTopLevelDllBeingLoadedTeb = NULL;
PVOID MmHighestUserAddress = (PVOID)MI_HIGHEST_USER_ADDRESS;

/* FUNCTIONS ***************************************************************/

#ifndef _M_AMD64
// FIXME: Why "Not implemented"???
/*
 * @implemented
 */
ULONG
NTAPI
RtlWalkFrameChain(OUT PVOID *Callers,
                  IN ULONG Count,
                  IN ULONG Flags)
{
    /* Not implemented for user-mode */
    return 0;
}
#endif

BOOLEAN
NTAPI
RtlpCheckForActiveDebugger(VOID)
{
    /* Return the flag in the PEB */
    return NtCurrentPeb()->BeingDebugged;
}

BOOLEAN
NTAPI
RtlpSetInDbgPrint(VOID)
{
    /* Check if it's already set and return TRUE if so */
    if (NtCurrentTeb()->InDbgPrint) return TRUE;

    /* Set it and return */
    NtCurrentTeb()->InDbgPrint = TRUE;
    return FALSE;
}

VOID
NTAPI
RtlpClearInDbgPrint(VOID)
{
    /* Clear the flag */
    NtCurrentTeb()->InDbgPrint = FALSE;
}

KPROCESSOR_MODE
NTAPI
RtlpGetMode()
{
   return UserMode;
}

/*
 * @implemented
 */
PPEB
NTAPI
RtlGetCurrentPeb(VOID)
{
    return NtCurrentPeb();
}

/*
 * @implemented
 */
VOID NTAPI
RtlAcquirePebLock(VOID)
{
   PPEB Peb = NtCurrentPeb ();
   RtlEnterCriticalSection(Peb->FastPebLock);
}

/*
 * @implemented
 */
VOID NTAPI
RtlReleasePebLock(VOID)
{
   PPEB Peb = NtCurrentPeb ();
   RtlLeaveCriticalSection(Peb->FastPebLock);
}

/*
* @implemented
*/
ULONG
NTAPI
RtlGetNtGlobalFlags(VOID)
{
    PPEB pPeb = NtCurrentPeb();
    return pPeb->NtGlobalFlag;
}

NTSTATUS
NTAPI
RtlDeleteHeapLock(IN OUT PHEAP_LOCK Lock)
{
    return RtlDeleteCriticalSection(&Lock->CriticalSection);
}

NTSTATUS
NTAPI
RtlEnterHeapLock(IN OUT PHEAP_LOCK Lock, IN BOOLEAN Exclusive)
{
    UNREFERENCED_PARAMETER(Exclusive);

    return RtlEnterCriticalSection(&Lock->CriticalSection);
}

BOOLEAN
NTAPI
RtlTryEnterHeapLock(IN OUT PHEAP_LOCK Lock, IN BOOLEAN Exclusive)
{
    UNREFERENCED_PARAMETER(Exclusive);

    return RtlTryEnterCriticalSection(&Lock->CriticalSection);
}

NTSTATUS
NTAPI
RtlInitializeHeapLock(IN OUT PHEAP_LOCK *Lock)
{
    return RtlInitializeCriticalSection(&(*Lock)->CriticalSection);
}

NTSTATUS
NTAPI
RtlLeaveHeapLock(IN OUT PHEAP_LOCK Lock)
{
    return RtlLeaveCriticalSection(&Lock->CriticalSection);
}

PVOID
NTAPI
RtlpAllocateMemory(UINT Bytes,
                   ULONG Tag)
{
    UNREFERENCED_PARAMETER(Tag);

    return RtlAllocateHeap(RtlGetProcessHeap(),
                           0,
                           Bytes);
}


VOID
NTAPI
RtlpFreeMemory(PVOID Mem,
               ULONG Tag)
{
    UNREFERENCED_PARAMETER(Tag);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                Mem);
}


#if DBG
VOID FASTCALL
CHECK_PAGED_CODE_RTL(char *file, int line)
{
  /* meaningless in user mode */
}
#endif

VOID
NTAPI
RtlpSetHeapParameters(IN PRTL_HEAP_PARAMETERS Parameters)
{
    PPEB Peb;

    /* Get PEB */
    Peb = RtlGetCurrentPeb();

    /* Apply defaults for non-set parameters */
    if (!Parameters->SegmentCommit) Parameters->SegmentCommit = Peb->HeapSegmentCommit;
    if (!Parameters->SegmentReserve) Parameters->SegmentReserve = Peb->HeapSegmentReserve;
    if (!Parameters->DeCommitFreeBlockThreshold) Parameters->DeCommitFreeBlockThreshold = Peb->HeapDeCommitFreeBlockThreshold;
    if (!Parameters->DeCommitTotalFreeThreshold) Parameters->DeCommitTotalFreeThreshold = Peb->HeapDeCommitTotalFreeThreshold;
}

BOOLEAN
NTAPI
RtlpHandleDpcStackException(IN PEXCEPTION_REGISTRATION_RECORD RegistrationFrame,
                            IN ULONG_PTR RegistrationFrameEnd,
                            IN OUT PULONG_PTR StackLow,
                            IN OUT PULONG_PTR StackHigh)
{
    /* There's no such thing as a DPC stack in user-mode */
    return FALSE;
}

VOID
NTAPI
RtlpCheckLogException(IN PEXCEPTION_RECORD ExceptionRecord,
                      IN PCONTEXT ContextRecord,
                      IN PVOID ContextData,
                      IN ULONG Size)
{
    /* Exception logging is not done in user-mode */
}

BOOLEAN
NTAPI
RtlpCaptureStackLimits(IN ULONG_PTR Ebp,
                       IN ULONG_PTR *StackBegin,
                       IN ULONG_PTR *StackEnd)
{
    /* FIXME: Verify */
    *StackBegin = (ULONG_PTR)NtCurrentTeb()->NtTib.StackLimit;
    *StackEnd = (ULONG_PTR)NtCurrentTeb()->NtTib.StackBase;
    return TRUE;
}

#ifdef _AMD64_
VOID
NTAPI
RtlpGetStackLimits(
    OUT PULONG_PTR LowLimit,
    OUT PULONG_PTR HighLimit)
{
    *LowLimit = (ULONG_PTR)NtCurrentTeb()->NtTib.StackLimit;
    *HighLimit = (ULONG_PTR)NtCurrentTeb()->NtTib.StackBase;
    return;
}
#endif

BOOLEAN
NTAPI
RtlIsThreadWithinLoaderCallout(VOID)
{
    return LdrpTopLevelDllBeingLoadedTeb == NtCurrentTeb();
}

/* RTL Atom Tables ************************************************************/

typedef struct _RTL_ATOM_HANDLE
{
   RTL_HANDLE_TABLE_ENTRY Handle;
   PRTL_ATOM_TABLE_ENTRY AtomEntry;
} RTL_ATOM_HANDLE, *PRTL_ATOM_HANDLE;

NTSTATUS
RtlpInitAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
   RtlInitializeCriticalSection(&AtomTable->CriticalSection);
   return STATUS_SUCCESS;
}


VOID
RtlpDestroyAtomTableLock(PRTL_ATOM_TABLE AtomTable)
{
   RtlDeleteCriticalSection(&AtomTable->CriticalSection);
}


BOOLEAN
RtlpLockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlEnterCriticalSection(&AtomTable->CriticalSection);
   return TRUE;
}


VOID
RtlpUnlockAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlLeaveCriticalSection(&AtomTable->CriticalSection);
}


/* handle functions */

BOOLEAN
RtlpCreateAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlInitializeHandleTable(0xCFFF,
			    sizeof(RTL_ATOM_HANDLE),
			    &AtomTable->RtlHandleTable);

   return TRUE;
}

VOID
RtlpDestroyAtomHandleTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlDestroyHandleTable(&AtomTable->RtlHandleTable);
}

PRTL_ATOM_TABLE
RtlpAllocAtomTable(ULONG Size)
{
   return (PRTL_ATOM_TABLE)RtlAllocateHeap(RtlGetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           Size);
}

VOID
RtlpFreeAtomTable(PRTL_ATOM_TABLE AtomTable)
{
   RtlFreeHeap(RtlGetProcessHeap(),
               0,
               AtomTable);
}

PRTL_ATOM_TABLE_ENTRY
RtlpAllocAtomTableEntry(ULONG Size)
{
   return (PRTL_ATOM_TABLE_ENTRY)RtlAllocateHeap(RtlGetProcessHeap(),
                                                 HEAP_ZERO_MEMORY,
                                                 Size);
}

VOID
RtlpFreeAtomTableEntry(PRTL_ATOM_TABLE_ENTRY Entry)
{
   RtlFreeHeap(RtlGetProcessHeap(),
               0,
               Entry);
}

VOID
RtlpFreeAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry)
{
   PRTL_HANDLE_TABLE_ENTRY RtlHandleEntry;

   if (RtlIsValidIndexHandle(&AtomTable->RtlHandleTable,
                             (ULONG)Entry->HandleIndex,
                             &RtlHandleEntry))
   {
      RtlFreeHandle(&AtomTable->RtlHandleTable,
                    RtlHandleEntry);
   }
}

BOOLEAN
RtlpCreateAtomHandle(PRTL_ATOM_TABLE AtomTable, PRTL_ATOM_TABLE_ENTRY Entry)
{
   ULONG HandleIndex;
   PRTL_HANDLE_TABLE_ENTRY RtlHandle;

   RtlHandle = RtlAllocateHandle(&AtomTable->RtlHandleTable,
                                 &HandleIndex);
   if (RtlHandle != NULL)
   {
      PRTL_ATOM_HANDLE AtomHandle = (PRTL_ATOM_HANDLE)RtlHandle;

      /* FIXME - Handle Indexes >= 0xC000 ?! */
      if (HandleIndex < 0xC000)
      {
         Entry->HandleIndex = (USHORT)HandleIndex;
         Entry->Atom = 0xC000 + (USHORT)HandleIndex;

         AtomHandle->AtomEntry = Entry;
         AtomHandle->Handle.Flags = RTL_HANDLE_VALID;

         return TRUE;
      }
      else
      {
         /* set the valid flag, otherwise RtlFreeHandle will fail! */
         AtomHandle->Handle.Flags = RTL_HANDLE_VALID;

         RtlFreeHandle(&AtomTable->RtlHandleTable,
                       RtlHandle);
      }
   }

   return FALSE;
}

PRTL_ATOM_TABLE_ENTRY
RtlpGetAtomEntry(PRTL_ATOM_TABLE AtomTable, ULONG Index)
{
   PRTL_HANDLE_TABLE_ENTRY RtlHandle;

   if (RtlIsValidIndexHandle(&AtomTable->RtlHandleTable,
                             Index,
                             &RtlHandle))
   {
      PRTL_ATOM_HANDLE AtomHandle = (PRTL_ATOM_HANDLE)RtlHandle;

      return AtomHandle->AtomEntry;
   }

   return NULL;
}


/*
 * Ldr Resource support code
 */

IMAGE_RESOURCE_DIRECTORY *find_entry_by_name( IMAGE_RESOURCE_DIRECTORY *dir,
                                              LPCWSTR name, void *root,
                                              int want_dir );
IMAGE_RESOURCE_DIRECTORY *find_entry_by_id( IMAGE_RESOURCE_DIRECTORY *dir,
                                            WORD id, void *root, int want_dir );
IMAGE_RESOURCE_DIRECTORY *find_first_entry( IMAGE_RESOURCE_DIRECTORY *dir,
                                            void *root, int want_dir );
int push_language( USHORT *list, ULONG pos, WORD lang );

/**********************************************************************
 *  find_entry
 *
 * Find a resource entry
 */
NTSTATUS find_entry( PVOID BaseAddress, LDR_RESOURCE_INFO *info,
                     ULONG level, void **ret, int want_dir )
{
    ULONG size;
    void *root;
    IMAGE_RESOURCE_DIRECTORY *resdirptr;
    USHORT list[9];  /* list of languages to try */
    int i, pos = 0;
    LCID user_lcid, system_lcid;

    root = RtlImageDirectoryEntryToData( BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &size );
    if (!root) return STATUS_RESOURCE_DATA_NOT_FOUND;
    if (size < sizeof(*resdirptr)) return STATUS_RESOURCE_DATA_NOT_FOUND;
    resdirptr = root;

    if (!level--) goto done;
    if (!(*ret = find_entry_by_name( resdirptr, (LPCWSTR)info->Type, root, want_dir || level )))
        return STATUS_RESOURCE_TYPE_NOT_FOUND;
    if (!level--) return STATUS_SUCCESS;

    resdirptr = *ret;
    if (!(*ret = find_entry_by_name( resdirptr, (LPCWSTR)info->Name, root, want_dir || level )))
        return STATUS_RESOURCE_NAME_NOT_FOUND;
    if (!level--) return STATUS_SUCCESS;
    if (level) return STATUS_INVALID_PARAMETER;  /* level > 3 */

    /* 1. specified language */
    pos = push_language( list, pos, info->Language );

    /* 2. specified language with neutral sublanguage */
    pos = push_language( list, pos, MAKELANGID( PRIMARYLANGID(info->Language), SUBLANG_NEUTRAL ) );

    /* 3. neutral language with neutral sublanguage */
    pos = push_language( list, pos, MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ) );

    /* if no explicitly specified language, try some defaults */
    if (PRIMARYLANGID(info->Language) == LANG_NEUTRAL)
    {
        /* user defaults, unless SYS_DEFAULT sublanguage specified  */
        if (SUBLANGID(info->Language) != SUBLANG_SYS_DEFAULT)
        {
            /* 4. current thread locale language */
            pos = push_language( list, pos, LANGIDFROMLCID(NtCurrentTeb()->CurrentLocale) );

            if (NT_SUCCESS(NtQueryDefaultLocale(TRUE, &user_lcid)))
            {
                /* 5. user locale language */
                pos = push_language( list, pos, LANGIDFROMLCID(user_lcid) );

                /* 6. user locale language with neutral sublanguage  */
                pos = push_language( list, pos, MAKELANGID( PRIMARYLANGID(user_lcid), SUBLANG_NEUTRAL ) );
            }
        }

        /* now system defaults */

        if (NT_SUCCESS(NtQueryDefaultLocale(FALSE, &system_lcid)))
        {
            /* 7. system locale language */
            pos = push_language( list, pos, LANGIDFROMLCID( system_lcid ) );

            /* 8. system locale language with neutral sublanguage */
            pos = push_language( list, pos, MAKELANGID( PRIMARYLANGID(system_lcid), SUBLANG_NEUTRAL ) );
        }

        /* 9. English */
        pos = push_language( list, pos, MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ) );
    }

    resdirptr = *ret;
    for (i = 0; i < pos; i++)
        if ((*ret = find_entry_by_id( resdirptr, list[i], root, want_dir ))) return STATUS_SUCCESS;

    /* if no explicitly specified language, return the first entry */
    if (PRIMARYLANGID(info->Language) == LANG_NEUTRAL)
    {
        if ((*ret = find_first_entry( resdirptr, root, want_dir ))) return STATUS_SUCCESS;
    }
    return STATUS_RESOURCE_LANG_NOT_FOUND;

done:
    *ret = resdirptr;
    return STATUS_SUCCESS;
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
 * @unimplemented
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDosApplyFileIsolationRedirection_Ustr(IN ULONG Flags,
                                         IN PUNICODE_STRING OriginalName,
                                         IN PUNICODE_STRING Extension,
                                         IN OUT PUNICODE_STRING StaticString,
                                         IN OUT PUNICODE_STRING DynamicString,
                                         IN OUT PUNICODE_STRING *NewName,
                                         IN PULONG NewFlags,
                                         IN PSIZE_T FileNameSize,
                                         IN PSIZE_T RequiredLength)
{
    return STATUS_SXS_KEY_NOT_FOUND;
}

/*
 * @implemented
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64EnableFsRedirection(IN BOOLEAN Wow64FsEnableRedirection)
{
    /* This is what Windows returns on x86 */
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64EnableFsRedirectionEx(IN PVOID Wow64FsEnableRedirection,
                              OUT PVOID *OldFsRedirectionLevel)
{
    /* This is what Windows returns on x86 */
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlComputeImportTableHash(IN HANDLE FileHandle,
                          OUT PCHAR Hash,
                          IN ULONG ImporTTableHashSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlpSafeCopyMemory(
   _Out_writes_bytes_all_(Length) VOID UNALIGNED *Destination,
   _In_reads_bytes_(Length) CONST VOID UNALIGNED *Source,
   _In_ SIZE_T Length)
{
    _SEH2_TRY
    {
        RtlCopyMemory(Destination, Source, Length);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

/* FIXME: code duplication with kernel32/client/time.c */
ULONG
NTAPI
RtlGetTickCount(VOID)
{
    ULARGE_INTEGER TickCount;

#ifdef _WIN64
    TickCount.QuadPart = *((volatile ULONG64*)&SharedUserData->TickCount);
#else
    while (TRUE)
    {
        TickCount.HighPart = (ULONG)SharedUserData->TickCount.High1Time;
        TickCount.LowPart = SharedUserData->TickCount.LowPart;

        if (TickCount.HighPart == (ULONG)SharedUserData->TickCount.High2Time)
            break;

        YieldProcessor();
    }
#endif

    return (ULONG)((UInt32x32To64(TickCount.LowPart,
                                  SharedUserData->TickCountMultiplier) >> 24) +
                    UInt32x32To64((TickCount.HighPart << 8) & 0xFFFFFFFF,
                                  SharedUserData->TickCountMultiplier));
}

/* EOF */
