#pragma once

typedef struct _USER_REFERENCE_ENTRY
{
   SINGLE_LIST_ENTRY Entry;
   PVOID obj;
} USER_REFERENCE_ENTRY, *PUSER_REFERENCE_ENTRY;

extern PUSER_HANDLE_TABLE gHandleTable;
VOID FASTCALL UserReferenceObject(PVOID obj);
PVOID FASTCALL UserReferenceObjectByHandle(HANDLE handle, HANDLE_TYPE type);
BOOL FASTCALL UserDereferenceObject(PVOID obj);
PVOID FASTCALL UserCreateObject(PUSER_HANDLE_TABLE ht, struct _DESKTOP* pDesktop, PTHREADINFO pti, HANDLE* h,HANDLE_TYPE type , ULONG size);
BOOL FASTCALL UserDeleteObject(HANDLE h, HANDLE_TYPE type );
PVOID UserGetObject(PUSER_HANDLE_TABLE ht, HANDLE handle, HANDLE_TYPE type );
PVOID UserGetObjectNoErr(PUSER_HANDLE_TABLE, HANDLE, HANDLE_TYPE);
BOOL FASTCALL UserCreateHandleTable(VOID);
BOOL FASTCALL UserObjectInDestroy(HANDLE);
void DbgUserDumpHandleTable();
PVOID FASTCALL ValidateHandle(HANDLE handle, HANDLE_TYPE type);
BOOLEAN UserDestroyObjectsForOwner(PUSER_HANDLE_TABLE Table, PVOID Owner);
BOOL FASTCALL UserMarkObjectDestroy(PVOID);
PVOID FASTCALL UserAssignmentLock(PVOID *ppvObj, PVOID pvNew);
PVOID FASTCALL UserAssignmentUnlock(PVOID *ppvObj);

static __inline VOID
UserRefObjectCo(PVOID obj, PUSER_REFERENCE_ENTRY UserReferenceEntry)
{
    PTHREADINFO W32Thread;

    W32Thread = PsGetCurrentThreadWin32Thread();
    ASSERT(W32Thread != NULL);
    ASSERT(UserReferenceEntry != NULL);
    UserReferenceEntry->obj = obj;
    UserReferenceObject(obj);
    PushEntryList(&W32Thread->ReferencesList, &UserReferenceEntry->Entry);
#if DBG
    W32Thread->cRefObjectCo++;
#endif
}

static __inline VOID
UserDerefObjectCo(PVOID obj)
{
    PTHREADINFO W32Thread;
    PSINGLE_LIST_ENTRY ReferenceEntry;
    PUSER_REFERENCE_ENTRY UserReferenceEntry;

    ASSERT(obj != NULL);
    W32Thread = PsGetCurrentThreadWin32Thread();
    ASSERT(W32Thread != NULL);
    ReferenceEntry = PopEntryList(&W32Thread->ReferencesList);
    ASSERT(ReferenceEntry != NULL);
    UserReferenceEntry = CONTAINING_RECORD(ReferenceEntry, USER_REFERENCE_ENTRY, Entry);
    ASSERT(UserReferenceEntry != NULL);

    ASSERT(obj == UserReferenceEntry->obj);
    UserDereferenceObject(obj);
#if DBG
    W32Thread->cRefObjectCo--;
#endif
}

void FreeProcMarkObject(_In_ PVOID Object);

/* EOF */
