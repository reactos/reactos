/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          User handle manager
 * FILE:             subsystems/win32/win32k/ntuser/object.c
 * PROGRAMER:        Copyright (C) 2001 Alexandre Julliard
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserObj);

//int usedHandles=0;
PUSER_HANDLE_TABLE gHandleTable = NULL;

#if DBG

void DbgUserDumpHandleTable()
{
    int HandleCounts[TYPE_CTYPES];
    PPROCESSINFO ppiList;
    int i;
    PWCHAR TypeNames[] = {L"Free",L"Window",L"Menu", L"CursorIcon", L"SMWP", L"Hook", L"ClipBoardData", L"CallProc",
                          L"Accel", L"DDEaccess", L"DDEconv", L"DDExact", L"Monitor", L"KBDlayout", L"KBDfile",
                          L"Event", L"Timer", L"InputContext", L"HidData", L"DeviceInfo", L"TouchInput",L"GestureInfo"};

    ERR("Total handles count: %lu\n", gpsi->cHandleEntries);

    memset(HandleCounts, 0, sizeof(HandleCounts));

    /* First of all count the number of handles per tpe */
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
   int index = (((unsigned int)handle & 0xffff) - FIRST_USER_HANDLE) >> 1;
   if (index < 0 || index >= ht->nb_handles)
      return NULL;
   if (!ht->handles[index].type)
      return NULL;
   generation = (unsigned int)handle >> 16;
   if (generation == ht->handles[index].generation || !generation || generation == 0xffff)
      return &ht->handles[index];
   return NULL;
}

__inline static HANDLE entry_to_handle(PUSER_HANDLE_TABLE ht, PUSER_HANDLE_ENTRY ptr )
{
   int index = ptr - ht->handles;
   return (HANDLE)(((index << 1) + FIRST_USER_HANDLE) + (ptr->generation << 16));
}

__inline static PUSER_HANDLE_ENTRY alloc_user_entry(PUSER_HANDLE_TABLE ht)
{
   PUSER_HANDLE_ENTRY entry;
   PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();
   TRACE("handles used %lu\n", gpsi->cHandleEntries);

   if (ht->freelist)
   {
      entry = ht->freelist;
      ht->freelist = entry->ptr;

      gpsi->cHandleEntries++;
      ppi->UserHandleCount++;
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
   ppi->UserHandleCount++;

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
   PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();
   void *ret;

#if DBG
   ppi->DbgHandleCount[entry->type]--;
#endif

   ret = entry->ptr;
   entry->ptr  = ht->freelist;
   entry->type = 0;
   entry->flags = 0;
   entry->pi = NULL;
   ht->freelist  = entry;

   gpsi->cHandleEntries--;
   ppi->UserHandleCount--;

   return ret;
}

static __inline PVOID
UserHandleOwnerByType(HANDLE_TYPE type)
{
    PVOID pi;

    switch (type)
    {
        case TYPE_WINDOW:
        case TYPE_INPUTCONTEXT:
            pi = GetW32ThreadInfo();
            break;

        case TYPE_MENU:
        case TYPE_CURSOR:
        case TYPE_HOOK:
        case TYPE_CALLPROC:
        case TYPE_ACCELTABLE:
        case TYPE_SETWINDOWPOS:
            pi = GetW32ProcessInfo();
            break;

        case TYPE_MONITOR:
            pi = NULL; /* System */
            break;

        default:
            pi = NULL;
            break;
    }

    return pi;
}

/* allocate a user handle for a given object */
HANDLE UserAllocHandle(PUSER_HANDLE_TABLE ht, PVOID object, HANDLE_TYPE type )
{
   PUSER_HANDLE_ENTRY entry = alloc_user_entry(ht);
   if (!entry)
      return 0;
   entry->ptr  = object;
   entry->type = type;
   entry->flags = 0;
   entry->pi = UserHandleOwnerByType(type);
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

   if ((unsigned int)handle >> 16)
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
   PPROCESSINFO ppi;
   BOOL dt;
   PDESKTOP rpdesk = pDesktop;

   /* We could get the desktop for the new object from the pti however this is 
    * not always the case for example when creating a new desktop window for 
    * the desktop thread*/

   if (!pti) pti = GetW32ThreadInfo();
   if (!pDesktop) rpdesk = pti->rpdesk;
   ppi = pti->ppi;

   switch (type)
   {
      case TYPE_WINDOW:
      case TYPE_MENU:
      case TYPE_HOOK:
      case TYPE_CALLPROC:
      case TYPE_INPUTCONTEXT:
         Object = DesktopHeapAlloc(rpdesk, size);
         dt = TRUE;
         break;

      default:
         Object = UserHeapAlloc(size);
         dt = FALSE;
         break;
   }

   if (!Object)
      return NULL;


   hi = UserAllocHandle(ht, Object, type );
   if (!hi)
   {
      if (dt)
         DesktopHeapFree(rpdesk, Object);
      else
         UserHeapFree(Object);
      return NULL;
   }

#if DBG
   ppi->DbgHandleCount[type]++;
#endif

   RtlZeroMemory(Object, size);

   switch (type)
   {
        case TYPE_WINDOW:
        case TYPE_HOOK:
        case TYPE_INPUTCONTEXT:
            ((PTHRDESKHEAD)Object)->rpdesk = rpdesk;
            ((PTHRDESKHEAD)Object)->pSelf = Object;
        case TYPE_WINEVENTHOOK:
            ((PTHROBJHEAD)Object)->pti = pti;
            break;

        case TYPE_MENU:
        case TYPE_CALLPROC:
            ((PPROCDESKHEAD)Object)->rpdesk = rpdesk;
            ((PPROCDESKHEAD)Object)->pSelf = Object;
            break;

        case TYPE_CURSOR:
            ((PPROCMARKHEAD)Object)->ppi = ppi;
            break;

        default:
            break;
   }
   /* Now set default headers. */
   ((PHEAD)Object)->h = hi;
   ((PHEAD)Object)->cLockObj = 2; // We need this, because we create 2 refs: handle and pointer!

   if (h)
      *h = hi;
   return Object;
}


BOOL
FASTCALL
UserDereferenceObject(PVOID object)
{
  PUSER_HANDLE_ENTRY entry;
  HANDLE_TYPE type;

  ASSERT(((PHEAD)object)->cLockObj >= 1);

  if ((INT)--((PHEAD)object)->cLockObj <= 0)
  {
     entry = handle_to_entry(gHandleTable, ((PHEAD)object)->h );

     if (!entry)
     {
        ERR("Warning! Dereference Object without ENTRY! Obj -> %p\n", object);
        return FALSE;
     }
     TRACE("Warning! Dereference to zero! Obj -> %p\n", object);

     ((PHEAD)object)->cLockObj = 0;

     if (!(entry->flags & HANDLEENTRY_INDESTROY))
        return TRUE;

     type = entry->type;
     free_user_entry(gHandleTable, entry );

     switch (type)
     {
        case TYPE_WINDOW:
        case TYPE_MENU:
        case TYPE_HOOK:
        case TYPE_CALLPROC:
        case TYPE_INPUTCONTEXT:
           return DesktopHeapFree(((PTHRDESKHEAD)object)->rpdesk, object);

        default:
           return UserHeapFree(object);
     }
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

   return UserFreeHandle(gHandleTable, h);
}

VOID
FASTCALL
UserReferenceObject(PVOID obj)
{
   ASSERT(((PHEAD)obj)->cLockObj >= 0);

   ((PHEAD)obj)->cLockObj++;
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

VOID
FASTCALL
UserSetObjectOwner(PVOID obj, HANDLE_TYPE type, PVOID owner)
{
    PUSER_HANDLE_ENTRY entry = handle_to_entry(gHandleTable, ((PHEAD)obj)->h );
    PPROCESSINFO ppi, oldppi;
    
    /* This must be called with a valid object */
    ASSERT(entry);
    
    /* For now, only supported for CursorIcon object */
    switch(type)
    {
        case TYPE_CURSOR:
            ppi = (PPROCESSINFO)owner;
            entry->pi = ppi;
            oldppi = ((PPROCMARKHEAD)obj)->ppi;
            ((PPROCMARKHEAD)obj)->ppi = ppi;
            break;
        default:
            ASSERT(FALSE);
            return;
    }

    oldppi->UserHandleCount--;
    ppi->UserHandleCount++;
#if DBG
    oldppi->DbgHandleCount[type]--;
    ppi->DbgHandleCount[type]++;
#endif
}


HANDLE FASTCALL ValidateHandleNoErr(HANDLE handle, HANDLE_TYPE type)
{
   if (handle) return (PWND)UserGetObjectNoErr(gHandleTable, handle, type);
   return NULL;
}

PVOID FASTCALL ValidateHandle(HANDLE handle, HANDLE_TYPE type)
{
  PVOID pObj;
  DWORD dwError = 0;
  if (handle) 
  {
      pObj = UserGetObjectNoErr(gHandleTable, handle, type);
      if (!pObj)
      {
          switch (type)
          {  
              case TYPE_WINDOW:
                  dwError = ERROR_INVALID_WINDOW_HANDLE;
                  break;
              case TYPE_MENU:
                  dwError = ERROR_INVALID_MENU_HANDLE;
                  break;
              case TYPE_CURSOR:
                  dwError = ERROR_INVALID_CURSOR_HANDLE;
                  break;
              case TYPE_SETWINDOWPOS:
                  dwError = ERROR_INVALID_DWP_HANDLE;
                  break;
              case TYPE_HOOK:
                  dwError = ERROR_INVALID_HOOK_HANDLE;
                  break;
              case TYPE_ACCELTABLE:
                  dwError = ERROR_INVALID_ACCEL_HANDLE;
                  break;
              default:
                  dwError = ERROR_INVALID_HANDLE;
                  break;
          }
          EngSetLastError(dwError);
          return NULL;
      }
      return pObj;
  }
  return NULL;
}
      
/*
 * NtUserValidateHandleSecure W2k3 has one argument.
 *
 * Status
 *    @implemented
 */

BOOL
APIENTRY
NtUserValidateHandleSecure(
   HANDLE handle,
   BOOL Restricted)
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
