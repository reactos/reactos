#pragma once

typedef struct _USER_REFERENCE_ENTRY
{
   SINGLE_LIST_ENTRY Entry;
   PVOID obj;
} USER_REFERENCE_ENTRY, *PUSER_REFERENCE_ENTRY;

#define USER_ASSERT(exp,file,line) \
    if (!(exp)) {RtlAssert(#exp,(PVOID)file,line,"");}

static __inline VOID
UserAssertLastRef(PVOID obj, const char *file, int line)
{
    PTHREADINFO W32Thread;
    PSINGLE_LIST_ENTRY ReferenceEntry;
    PUSER_REFERENCE_ENTRY UserReferenceEntry;

    USER_ASSERT(obj != NULL, file, line);
    W32Thread = PsGetCurrentThreadWin32Thread();
    USER_ASSERT(W32Thread != NULL, file, line);
    ReferenceEntry = W32Thread->ReferencesList.Next;
    USER_ASSERT(ReferenceEntry != NULL, file, line);
    UserReferenceEntry = CONTAINING_RECORD(ReferenceEntry, USER_REFERENCE_ENTRY, Entry);
    USER_ASSERT(UserReferenceEntry != NULL, file, line);
    USER_ASSERT(obj == UserReferenceEntry->obj, file, line);
}
#define ASSERT_LAST_REF(_obj_) UserAssertLastRef(_obj,__FILE__,__LINE__)

#undef USER_ASSERT

extern PUSER_HANDLE_TABLE gHandleTable;
VOID FASTCALL UserReferenceObject(PVOID obj);
PVOID FASTCALL UserReferenceObjectByHandle(HANDLE handle, USER_OBJECT_TYPE type);
BOOL FASTCALL UserDereferenceObject(PVOID obj);
PVOID FASTCALL UserCreateObject(PUSER_HANDLE_TABLE ht, struct _DESKTOP* pDesktop, HANDLE* h,USER_OBJECT_TYPE type , ULONG size);
BOOL FASTCALL UserDeleteObject(HANDLE h, USER_OBJECT_TYPE type );
PVOID UserGetObject(PUSER_HANDLE_TABLE ht, HANDLE handle, USER_OBJECT_TYPE type );
BOOL FASTCALL UserCreateHandleTable(VOID);

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
}

/* EOF */
