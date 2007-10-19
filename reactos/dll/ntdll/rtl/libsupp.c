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

/* FUNCTIONS ***************************************************************/

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

BOOLEAN
NTAPI
RtlpCheckForActiveDebugger(BOOLEAN Type)
{
    return (NtCurrentPeb()->BeingDebugged);
}

BOOLEAN
NTAPI
RtlpSetInDbgPrint(IN BOOLEAN NewValue)
{
    /* If we're setting it to false, do it and return */
    if (NewValue == FALSE)
    {
        NtCurrentTeb()->InDbgPrint = FALSE;
        return FALSE;
    }

    /* Setting to true; check if it's not already */
    if (NtCurrentTeb()->InDbgPrint) return TRUE;

    /* Set it and return */
    NtCurrentTeb()->InDbgPrint = TRUE;
    return FALSE;
}

KPROCESSOR_MODE
NTAPI
RtlpGetMode()
{
   return UserMode;
}

PPEB
NTAPI
RtlpCurrentPeb(VOID)
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
   Peb->FastPebLockRoutine (Peb->FastPebLock);
}

/*
 * @implemented
 */
VOID NTAPI
RtlReleasePebLock(VOID)
{
   PPEB Peb = NtCurrentPeb ();
   Peb->FastPebUnlockRoutine (Peb->FastPebLock);
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
RtlDeleteHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    return RtlDeleteCriticalSection(CriticalSection);
}

NTSTATUS
NTAPI
RtlEnterHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    return RtlEnterCriticalSection(CriticalSection);
}

NTSTATUS
NTAPI
RtlInitializeHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
     return RtlInitializeCriticalSection(CriticalSection);
}

NTSTATUS
NTAPI
RtlLeaveHeapLock(
    PRTL_CRITICAL_SECTION CriticalSection)
{
    return RtlLeaveCriticalSection(CriticalSection );
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


#ifdef DBG
VOID FASTCALL
CHECK_PAGED_CODE_RTL(char *file, int line)
{
  /* meaningless in user mode */
}
#endif

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
    *StackBegin = (ULONG_PTR)NtCurrentTeb()->Tib.StackLimit;
    *StackEnd = (ULONG_PTR)NtCurrentTeb()->Tib.StackBase;
    return TRUE;
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
