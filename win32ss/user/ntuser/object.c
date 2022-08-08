/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          User handle manager
 * FILE:             win32ss/user/ntuser/object.c
 * PROGRAMER:        Copyright (C) 2001 Alexandre Julliard
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserObj);

//int usedHandles=0;
PUSER_HANDLE_TABLE gHandleTable = NULL;

/* Forward declarations */
_Success_(return!=NULL)
static PVOID AllocThreadObject(
    _In_ PDESKTOP pDesk,
    _In_ PTHREADINFO pti,
    _In_ SIZE_T Size,
    _Out_ PVOID* HandleOwner)
{
    PTHROBJHEAD ObjHead;

    UNREFERENCED_PARAMETER(pDesk);

    ASSERT(Size > sizeof(*ObjHead));
    ASSERT(pti != NULL);

    ObjHead = UserHeapAlloc(Size);
    if (!ObjHead)
        return NULL;

    RtlZeroMemory(ObjHead, Size);

    ObjHead->pti = pti;
    IntReferenceThreadInfo(pti);
    *HandleOwner = pti;
    /* It's a thread object, but it still count as one for the process */
    pti->ppi->UserHandleCount++;

    return ObjHead;
}

static void FreeThreadObject(
    _In_ PVOID Object)
{
    PTHROBJHEAD ObjHead = (PTHROBJHEAD)Object;
    PTHREADINFO pti = ObjHead->pti;

    UserHeapFree(ObjHead);

    pti->ppi->UserHandleCount--;
    IntDereferenceThreadInfo(pti);
}

_Success_(return!=NULL)
static PVOID AllocDeskThreadObject(
    _In_ PDESKTOP pDesk,
    _In_ PTHREADINFO pti,
    _In_ SIZE_T Size,
    _Out_ PVOID* HandleOwner)
{
    PTHRDESKHEAD ObjHead;

    ASSERT(Size > sizeof(*ObjHead));
    ASSERT(pti != NULL);

    if (!pDesk)
        pDesk = pti->rpdesk;

    ObjHead = DesktopHeapAlloc(pDesk, Size);
    if (!ObjHead)
        return NULL;

    RtlZeroMemory(ObjHead, Size);

    ObjHead->pSelf = ObjHead;
    ObjHead->rpdesk = pDesk;
    ObjHead->pti = pti;
    IntReferenceThreadInfo(pti);
    *HandleOwner = pti;
    /* It's a thread object, but it still count as one for the process */
    pti->ppi->UserHandleCount++;

    return ObjHead;
}

static void FreeDeskThreadObject(
    _In_ PVOID Object)
{
    PTHRDESKHEAD ObjHead = (PTHRDESKHEAD)Object;
    PDESKTOP pDesk = ObjHead->rpdesk;
    PTHREADINFO pti = ObjHead->pti;

    DesktopHeapFree(pDesk, Object);

    pti->ppi->UserHandleCount--;
    IntDereferenceThreadInfo(pti);
}

_Success_(return!=NULL)
static PVOID AllocDeskProcObject(
    _In_ PDESKTOP pDesk,
    _In_ PTHREADINFO pti,
    _In_ SIZE_T Size,
    _Out_ PVOID* HandleOwner)
{
    PPROCDESKHEAD ObjHead;
    PPROCESSINFO ppi;

    ASSERT(Size > sizeof(*ObjHead));
    ASSERT(pDesk != NULL);
    ASSERT(pti != NULL);

    ObjHead = DesktopHeapAlloc(pDesk, Size);
    if (!ObjHead)
        return NULL;

    RtlZeroMemory(ObjHead, Size);

    ppi = pti->ppi;

    ObjHead->pSelf = ObjHead;
    ObjHead->rpdesk = pDesk;
    ObjHead->hTaskWow = (DWORD_PTR)ppi;
    ppi->UserHandleCount++;
    IntReferenceProcessInfo(ppi);
    *HandleOwner = ppi;

    return ObjHead;
}

static void FreeDeskProcObject(
    _In_ PVOID Object)
{
    PPROCDESKHEAD ObjHead = (PPROCDESKHEAD)Object;
    PDESKTOP pDesk = ObjHead->rpdesk;
    PPROCESSINFO ppi = (PPROCESSINFO)ObjHead->hTaskWow;

    ppi->UserHandleCount--;
    IntDereferenceProcessInfo(ppi);

    DesktopHeapFree(pDesk, Object);
}

_Success_(return!=NULL)
static PVOID AllocProcMarkObject(
    _In_ PDESKTOP pDesk,
    _In_ PTHREADINFO pti,
    _In_ SIZE_T Size,
    _Out_ PVOID* HandleOwner)
{
    PPROCMARKHEAD ObjHead;
    PPROCESSINFO ppi = pti->ppi;

    UNREFERENCED_PARAMETER(pDesk);

    ASSERT(Size > sizeof(*ObjHead));

    ObjHead = UserHeapAlloc(Size);
    if (!ObjHead)
        return NULL;

    RtlZeroMemory(ObjHead, Size);

    ObjHead->ppi = ppi;
    IntReferenceProcessInfo(ppi);
    *HandleOwner = ppi;
    ppi->UserHandleCount++;

    return ObjHead;
}

void FreeProcMarkObject(
    _In_ PVOID Object)
{
    PPROCESSINFO ppi = ((PPROCMARKHEAD)Object)->ppi;

    UserHeapFree(Object);

    ppi->UserHandleCount--;
    IntDereferenceProcessInfo(ppi);
}

_Success_(return!=NULL)
static PVOID AllocSysObject(
    _In_ PDESKTOP pDesk,
    _In_ PTHREADINFO pti,
    _In_ SIZE_T Size,
    _Out_ PVOID* ObjectOwner)
{
    PVOID Object;

    UNREFERENCED_PARAMETER(pDesk);
    UNREFERENCED_PARAMETER(pti);

    ASSERT(Size > sizeof(HEAD));

    Object = UserHeapAlloc(Size);
    if (!Object)
        return NULL;

    *ObjectOwner = NULL;

    RtlZeroMemory(Object, Size);
    return Object;
}

_Success_(return!=NULL)
static PVOID AllocSysObjectCB(
    _In_ PDESKTOP pDesk,
    _In_ PTHREADINFO pti,
    _In_ SIZE_T Size,
    _Out_ PVOID* ObjectOwner)
{
    PVOID Object;

    UNREFERENCED_PARAMETER(pDesk);
    UNREFERENCED_PARAMETER(pti);
    ASSERT(Size > sizeof(HEAD));

    /* Allocate the clipboard data */
    // FIXME: This allocation should be done on the current session pool;
    // however ReactOS' MM doesn't support session pool yet.
    Object = ExAllocatePoolZero(/* SESSION_POOL_MASK | */ PagedPool, Size, USERTAG_CLIPBOARD);
    if (!Object)
    {
        ERR("ExAllocatePoolZero failed. No object created.\n");
        return NULL;
    }

    *ObjectOwner = NULL;
    return Object;
}

static void FreeSysObject(
    _In_ PVOID Object)
{
    UserHeapFree(Object);
}

static void FreeSysObjectCB(
    _In_ PVOID Object)
{
    ExFreePoolWithTag(Object, USERTAG_CLIPBOARD);
}

static const struct
{
    PVOID   (*ObjectAlloc)(PDESKTOP, PTHREADINFO, SIZE_T, PVOID*);
    BOOLEAN (*ObjectDestroy)(PVOID);
    void    (*ObjectFree)(PVOID);
} ObjectCallbacks[TYPE_CTYPES] =
{
    { NULL,                     NULL,                       NULL },                 /* TYPE_FREE */
    { AllocDeskThreadObject,    co_UserDestroyWindow,       FreeDeskThreadObject }, /* TYPE_WINDOW */
    { AllocDeskProcObject,      UserDestroyMenuObject,      FreeDeskProcObject },   /* TYPE_MENU */
    { AllocProcMarkObject,      IntDestroyCurIconObject,    FreeCurIconObject },    /* TYPE_CURSOR */
    { AllocSysObject,           /*UserSetWindowPosCleanup*/NULL, FreeSysObject },   /* TYPE_SETWINDOWPOS */
    { AllocDeskThreadObject,    IntRemoveHook,              FreeDeskThreadObject }, /* TYPE_HOOK */
    { AllocSysObjectCB,         /*UserClipDataCleanup*/NULL,FreeSysObjectCB },      /* TYPE_CLIPDATA */
    { AllocDeskProcObject,      DestroyCallProc,            FreeDeskProcObject },   /* TYPE_CALLPROC */
    { AllocProcMarkObject,      UserDestroyAccelTable,      FreeProcMarkObject },   /* TYPE_ACCELTABLE */
    { NULL,                     NULL,                       NULL },                 /* TYPE_DDEACCESS */
    { NULL,                     NULL,                       NULL },                 /* TYPE_DDECONV */
    { NULL,                     NULL,                       NULL },                 /* TYPE_DDEXACT */
    { AllocSysObject,           /*UserMonitorCleanup*/NULL, FreeSysObject },        /* TYPE_MONITOR */
    { AllocSysObject,           /*UserKbdLayoutCleanup*/NULL,FreeSysObject },       /* TYPE_KBDLAYOUT */
    { AllocSysObject,           /*UserKbdFileCleanup*/NULL, FreeSysObject },        /* TYPE_KBDFILE */
    { AllocThreadObject,        IntRemoveEvent,             FreeThreadObject },     /* TYPE_WINEVENTHOOK */
    { AllocSysObject,           /*UserTimerCleanup*/NULL,   FreeSysObject },        /* TYPE_TIMER */
    { AllocInputContextObject,  UserDestroyInputContext,    UserFreeInputContext }, /* TYPE_INPUTCONTEXT */
    { NULL,                     NULL,                       NULL },                 /* TYPE_HIDDATA */
    { NULL,                     NULL,                       NULL },                 /* TYPE_DEVICEINFO */
    { NULL,                     NULL,                       NULL },                 /* TYPE_TOUCHINPUTINFO */
    { NULL,                     NULL,                       NULL },                 /* TYPE_GESTUREINFOOBJ */
};

#if DBG

void DbgUserDumpHandleTable(VOID)
{
    int HandleCounts[TYPE_CTYPES];
    PPROCESSINFO ppiList;
    int i;
    PWCHAR TypeNames[] = {L"Free",L"Window",L"Menu", L"CursorIcon", L"SMWP", L"Hook", L"ClipBoardData", L"CallProc",
                          L"Accel", L"DDEaccess", L"DDEconv", L"DDExact", L"Monitor", L"KBDlayout", L"KBDfile",
                          L"Event", L"Timer", L"InputContext", L"HidData", L"DeviceInfo", L"TouchInput",L"GestureInfo"};

    ERR("Total handles count: %lu\n", gpsi->cHandleEntries);

    memset(HandleCounts, 0, sizeof(HandleCounts));

    /* First of all count the number of handles per type */
    ppiList = gppiList;
    while (ppiList)
    {
        ERR("Process %s (%p) handles count: %d\n\t", ppiList->peProcess->ImageFileName, ppiList->peProcess->UniqueProcessId, ppiList->UserHandleCount);

        for (i = 1 ;i < TYPE_CTYPES; i++)
        {
            HandleCounts[i] += ppiList->DbgHandleCount[i];

            DbgPrint("%S: %lu, ", TypeNames[i], ppiList->DbgHandleCount[i]);
            if (i % 6 == 0)
                DbgPrint("\n\t");
        }
        DbgPrint("\n");

        ppiList = ppiList->ppiNext;
    }

    /* Print total type counts */
    ERR("Total handles of the running processes: \n\t");
    for (i = 1 ;i < TYPE_CTYPES; i++)
    {
        DbgPrint("%S: %d, ", TypeNames[i], HandleCounts[i]);
        if (i % 6 == 0)
            DbgPrint("\n\t");
    }
    DbgPrint("\n");

    /* Now count the handle counts that are allocated from the handle table */
    memset(HandleCounts, 0, sizeof(HandleCounts));
    for (i = 0; i < gHandleTable->nb_handles; i++)
         HandleCounts[gHandleTable->handles[i].type]++;

    ERR("Total handles count allocated: \n\t");
    for (i = 1 ;i < TYPE_CTYPES; i++)
    {
        DbgPrint("%S: %d, ", TypeNames[i], HandleCounts[i]);
        if (i % 6 == 0)
            DbgPrint("\n\t");
    }
    DbgPrint("\n");
}

#endif

PUSER_HANDLE_ENTRY handle_to_entry(PUSER_HANDLE_TABLE ht, HANDLE handle )
{
   unsigned short generation;
   int index = (LOWORD(handle) - FIRST_USER_HANDLE) >> 1;
   if (index < 0 || index >= ht->nb_handles)
      return NULL;
   if (!ht->handles[index].type)
      return NULL;
   generation = HIWORD(handle);
   if (generation == ht->handles[index].generation || !generation || generation == 0xffff)
      return &ht->handles[index];
   return NULL;
}

__inline static HANDLE entry_to_handle(PUSER_HANDLE_TABLE ht, PUSER_HANDLE_ENTRY ptr )
{
   int index = ptr - ht->handles;
   return (HANDLE)((((INT_PTR)index << 1) + FIRST_USER_HANDLE) + (ptr->generation << 16));
}

__inline static PUSER_HANDLE_ENTRY alloc_user_entry(PUSER_HANDLE_TABLE ht)
{
   PUSER_HANDLE_ENTRY entry;
   TRACE("handles used %lu\n", gpsi->cHandleEntries);

   if (ht->freelist)
   {
      entry = ht->freelist;
      ht->freelist = entry->ptr;

      gpsi->cHandleEntries++;
      return entry;
   }

   if (ht->nb_handles >= ht->allocated_handles)  /* Need to grow the array */
   {
       ERR("Out of user handles! Used -> %lu, NM_Handle -> %d\n", gpsi->cHandleEntries, ht->nb_handles);

#if DBG
       DbgUserDumpHandleTable();
#endif

      return NULL;
#if 0
      PUSER_HANDLE_ENTRY new_handles;
      /* Grow array by 50% (but at minimum 32 entries) */
      int growth = max( 32, ht->allocated_handles / 2 );
      int new_size = min( ht->allocated_handles + growth, (LAST_USER_HANDLE-FIRST_USER_HANDLE+1) >> 1 );
      if (new_size <= ht->allocated_handles)
         return NULL;
      if (!(new_handles = UserHeapReAlloc( ht->handles, new_size * sizeof(*ht->handles) )))
         return NULL;
      ht->handles = new_handles;
      ht->allocated_handles = new_size;
#endif
   }

   entry = &ht->handles[ht->nb_handles++];

   entry->generation = 1;

   gpsi->cHandleEntries++;

   return entry;
}

VOID UserInitHandleTable(PUSER_HANDLE_TABLE ht, PVOID mem, ULONG bytes)
{
   ht->freelist = NULL;
   ht->handles = mem;

   ht->nb_handles = 0;
   ht->allocated_handles = bytes / sizeof(USER_HANDLE_ENTRY);
}


__inline static void *free_user_entry(PUSER_HANDLE_TABLE ht, PUSER_HANDLE_ENTRY entry)
{
   void *ret;

#if DBG
   {
       PPROCESSINFO ppi;
       switch (entry->type)
       {
           case TYPE_WINDOW:
           case TYPE_HOOK:
           case TYPE_WINEVENTHOOK:
               ppi = ((PTHREADINFO)entry->pi)->ppi;
               break;
           case TYPE_MENU:
           case TYPE_CURSOR:
           case TYPE_CALLPROC:
           case TYPE_ACCELTABLE:
               ppi = entry->pi;
               break;
           default:
               ppi = NULL;
       }
       if (ppi)
           ppi->DbgHandleCount[entry->type]--;
   }
#endif

   ret = entry->ptr;
   entry->ptr  = ht->freelist;
   entry->type = 0;
   entry->flags = 0;
   entry->pi = NULL;
   ht->freelist  = entry;

   gpsi->cHandleEntries--;

   return ret;
}

/* allocate a user handle for a given object */
HANDLE UserAllocHandle(
    _Inout_ PUSER_HANDLE_TABLE ht,
    _In_ PVOID object,
    _In_ HANDLE_TYPE type,
    _In_ PVOID HandleOwner)
{
   PUSER_HANDLE_ENTRY entry = alloc_user_entry(ht);
   if (!entry)
      return 0;
   entry->ptr  = object;
   entry->type = type;
   entry->flags = 0;
   entry->pi = HandleOwner;
   if (++entry->generation >= 0xffff)
      entry->generation = 1;

   /* We have created a handle, which is a reference! */
   UserReferenceObject(object);

   return entry_to_handle(ht, entry );
}

/* return a pointer to a user object from its handle without setting an error */
PVOID UserGetObjectNoErr(PUSER_HANDLE_TABLE ht, HANDLE handle, HANDLE_TYPE type )
{
   PUSER_HANDLE_ENTRY entry;

   ASSERT(ht);

   if (!(entry = handle_to_entry(ht, handle )) || entry->type != type)
   {
      return NULL;
   }
   return entry->ptr;
}

/* return a pointer to a user object from its handle */
PVOID UserGetObject(PUSER_HANDLE_TABLE ht, HANDLE handle, HANDLE_TYPE type )
{
   PUSER_HANDLE_ENTRY entry;

   ASSERT(ht);

   if (!(entry = handle_to_entry(ht, handle )) || entry->type != type)
   {
      EngSetLastError(ERROR_INVALID_HANDLE);
      return NULL;
   }
   return entry->ptr;
}


/* Get the full handle (32bit) for a possibly truncated (16bit) handle */
HANDLE get_user_full_handle(PUSER_HANDLE_TABLE ht,  HANDLE handle )
{
   PUSER_HANDLE_ENTRY entry;

   if ((ULONG_PTR)handle >> 16)
      return handle;
   if (!(entry = handle_to_entry(ht, handle )))
      return handle;
   return entry_to_handle( ht, entry );
}


/* Same as get_user_object plus set the handle to the full 32-bit value */
void *get_user_object_handle(PUSER_HANDLE_TABLE ht,  HANDLE* handle, HANDLE_TYPE type )
{
   PUSER_HANDLE_ENTRY entry;

   if (!(entry = handle_to_entry(ht, *handle )) || entry->type != type)
      return NULL;
   *handle = entry_to_handle( ht, entry );
   return entry->ptr;
}



BOOL FASTCALL UserCreateHandleTable(VOID)
{
   PVOID mem;
   INT HandleCount = 1024 * 4;

   // FIXME: Don't alloc all at once! Must be mapped into umode also...
   mem = UserHeapAlloc(sizeof(USER_HANDLE_ENTRY) * HandleCount);
   if (!mem)
   {
      ERR("Failed creating handle table\n");
      return FALSE;
   }

   gHandleTable = UserHeapAlloc(sizeof(USER_HANDLE_TABLE));
   if (gHandleTable == NULL)
   {
       UserHeapFree(mem);
       ERR("Failed creating handle table\n");
       return FALSE;
   }

   // FIXME: Make auto growable
   UserInitHandleTable(gHandleTable, mem, sizeof(USER_HANDLE_ENTRY) * HandleCount);

   return TRUE;
}

//
// New
//
PVOID
FASTCALL
UserCreateObject( PUSER_HANDLE_TABLE ht,
                  PDESKTOP pDesktop,
                  PTHREADINFO pti,
                  HANDLE* h,
                  HANDLE_TYPE type,
                  ULONG size)
{
   HANDLE hi;
   PVOID Object;
   PVOID ObjectOwner;

   /* Some sanity checks. Other checks will be made in the allocator */
   ASSERT(type < TYPE_CTYPES);
   ASSERT(type != TYPE_FREE);
   ASSERT(ht != NULL);

   /* Allocate the object */
   ASSERT(ObjectCallbacks[type].ObjectAlloc != NULL);
   Object = ObjectCallbacks[type].ObjectAlloc(pDesktop, pti, size, &ObjectOwner);
   if (!Object)
   {
       ERR("User object allocation failed. Out of memory!\n");
       return NULL;
   }

   hi = UserAllocHandle(ht, Object, type, ObjectOwner);
   if (hi == NULL)
   {
       ERR("Out of user handles!\n");
       ObjectCallbacks[type].ObjectFree(Object);
       return NULL;
   }

#if DBG
   if (pti)
       pti->ppi->DbgHandleCount[type]++;
#endif

   /* Give this object its identity. */
   ((PHEAD)Object)->h = hi;

   /* The caller will get a locked object.
    * Note: with the reference from the handle, that makes two */
   UserReferenceObject(Object);

   if (h)
      *h = hi;
   return Object;
}

// Win: HMMarkObjectDestroy
BOOL
FASTCALL
UserMarkObjectDestroy(PVOID Object)
{
    PUSER_HANDLE_ENTRY entry;
    PHEAD ObjHead = Object;

    entry = handle_to_entry(gHandleTable, ObjHead->h);

    ASSERT(entry != NULL);

    entry->flags |= HANDLEENTRY_DESTROY;

    if (ObjHead->cLockObj > 1)
    {
        entry->flags &= ~HANDLEENTRY_INDESTROY;
        TRACE("Count %d\n",ObjHead->cLockObj);
        return FALSE;
    }

    return TRUE;
}

BOOL
FASTCALL
UserDereferenceObject(PVOID Object)
{
    PHEAD ObjHead = Object;

    ASSERT(ObjHead->cLockObj >= 1);
    ASSERT(ObjHead->cLockObj < 0x10000);

    if (--ObjHead->cLockObj == 0)
    {
        PUSER_HANDLE_ENTRY entry;
        HANDLE_TYPE type;

        entry = handle_to_entry(gHandleTable, ObjHead->h);

        ASSERT(entry != NULL);
        /* The entry should be marked as in deletion */
        ASSERT(entry->flags & HANDLEENTRY_INDESTROY);

        type = entry->type;
        ASSERT(type != TYPE_FREE);
        ASSERT(type < TYPE_CTYPES);

        /* We can now get rid of everything */
        free_user_entry(gHandleTable, entry );

#if 0
        /* Call the object destructor */
        ASSERT(ObjectCallbacks[type].ObjectCleanup != NULL);
        ObjectCallbacks[type].ObjectCleanup(Object);
#endif

        /* And free it */
        ASSERT(ObjectCallbacks[type].ObjectFree != NULL);
        ObjectCallbacks[type].ObjectFree(Object);

        return TRUE;
    }
    return FALSE;
}

BOOL
FASTCALL
UserFreeHandle(PUSER_HANDLE_TABLE ht,  HANDLE handle )
{
  PUSER_HANDLE_ENTRY entry;

  if (!(entry = handle_to_entry( ht, handle )))
  {
     SetLastNtError( STATUS_INVALID_HANDLE );
     return FALSE;
  }

  entry->flags = HANDLEENTRY_INDESTROY;

  return UserDereferenceObject(entry->ptr);
}

BOOL
FASTCALL
UserObjectInDestroy(HANDLE h)
{
  PUSER_HANDLE_ENTRY entry;

  if (!(entry = handle_to_entry( gHandleTable, h )))
  {
     SetLastNtError( STATUS_INVALID_HANDLE );
     return TRUE;
  }
  return (entry->flags & HANDLEENTRY_INDESTROY);
}

BOOL
FASTCALL
UserDeleteObject(HANDLE h, HANDLE_TYPE type )
{
   PVOID body = UserGetObject(gHandleTable, h, type);

   if (!body) return FALSE;

   ASSERT( ((PHEAD)body)->cLockObj >= 1);
   ASSERT( ((PHEAD)body)->cLockObj < 0x10000);

   return UserFreeHandle(gHandleTable, h);
}

VOID
FASTCALL
UserReferenceObject(PVOID obj)
{
   PHEAD ObjHead = obj;
   ASSERT(ObjHead->cLockObj < 0x10000);

   ObjHead->cLockObj++;
}

PVOID
FASTCALL
UserReferenceObjectByHandle(HANDLE handle, HANDLE_TYPE type)
{
    PVOID object;

    object = UserGetObject(gHandleTable, handle, type);
    if (object)
    {
       UserReferenceObject(object);
    }
    return object;
}

BOOLEAN
UserDestroyObjectsForOwner(PUSER_HANDLE_TABLE Table, PVOID Owner)
{
    int i;
    PUSER_HANDLE_ENTRY Entry;
    BOOLEAN Ret = TRUE;

    /* Sweep the whole handle table */
    for (i = 0; i < Table->allocated_handles; i++)
    {
        Entry = &Table->handles[i];

        if (Entry->pi != Owner)
            continue;

        /* Do not destroy if it's already been done */
        if (Entry->flags & HANDLEENTRY_INDESTROY)
            continue;

        /* Call destructor */
        if (!ObjectCallbacks[Entry->type].ObjectDestroy(Entry->ptr))
        {
            ERR("Failed destructing object %p, type %u.\n", Entry->ptr, Entry->type);
            /* Don't return immediately, we must continue destroying the other objects */
            Ret = FALSE;
        }
    }

    return Ret;
}

/*
 *
 * Status
 *    @implemented
 */

BOOL
APIENTRY
NtUserValidateHandleSecure(
   HANDLE handle)
{
   UINT uType;
   PPROCESSINFO ppi;
   PUSER_HANDLE_ENTRY entry;

   DECLARE_RETURN(BOOL);
   UserEnterExclusive();

   if (!(entry = handle_to_entry(gHandleTable, handle )))
   {
      EngSetLastError(ERROR_INVALID_HANDLE);
      RETURN( FALSE);
   }
   uType = entry->type;
   switch (uType)
   {
       case TYPE_WINDOW:
       case TYPE_INPUTCONTEXT:
          ppi = ((PTHREADINFO)entry->pi)->ppi;
          break;
       case TYPE_MENU:
       case TYPE_ACCELTABLE:
       case TYPE_CURSOR:
       case TYPE_HOOK:
       case TYPE_CALLPROC:
       case TYPE_SETWINDOWPOS:
          ppi = entry->pi;
          break;
       default:
          ppi = NULL;
          break;
   }

   if (!ppi) RETURN( FALSE);

   // Same process job returns TRUE.
   if (gptiCurrent->ppi->pW32Job == ppi->pW32Job) RETURN( TRUE);

   RETURN( FALSE);

CLEANUP:
   UserLeave();
   END_CLEANUP;
}

// Win: HMAssignmentLock
PVOID FASTCALL UserAssignmentLock(PVOID *ppvObj, PVOID pvNew)
{
    PVOID pvOld = *ppvObj;
    *ppvObj = pvNew;

    if (pvOld && pvOld == pvNew)
        return pvOld;

    if (pvNew)
        UserReferenceObject(pvNew);

    if (pvOld)
    {
        if (UserDereferenceObject(pvOld))
            pvOld = NULL;
    }

    return pvOld;
}

// Win: HMAssignmentUnlock
PVOID FASTCALL UserAssignmentUnlock(PVOID *ppvObj)
{
    PVOID pvOld = *ppvObj;
    *ppvObj = NULL;

    if (pvOld)
    {
        if (UserDereferenceObject(pvOld))
            pvOld = NULL;
    }

    return pvOld;
}
