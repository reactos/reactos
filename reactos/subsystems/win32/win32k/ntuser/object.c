/*
 * Server-side USER handles
 *
 * Copyright (C) 2001 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */



/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

//int usedHandles=0;
PUSER_HANDLE_TABLE gHandleTable = NULL;


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

   DPRINT("handles used %i\n",gpsi->cHandleEntries);

   if (ht->freelist)
   {
      entry = ht->freelist;
      ht->freelist = entry->ptr;

      gpsi->cHandleEntries++;
      return entry;
   }

   if (ht->nb_handles >= ht->allocated_handles)  /* need to grow the array */
   {
/**/
      int i, iFree = 0, iWindow = 0, iMenu = 0, iCursorIcon = 0,
          iHook = 0, iCallProc = 0, iAccel = 0, iMonitor = 0, iTimer = 0;
 /**/
      DPRINT1("Out of user handles! Used -> %i, NM_Handle -> %d\n", gpsi->cHandleEntries, ht->nb_handles);
//#if 0
      for(i = 0; i < ht->nb_handles; i++)
      {
         switch (ht->handles[i].type)
         {
           case otFree: // Should be zero.
            iFree++;
            break;
           case otWindow:
            iWindow++;
            break;
           case otMenu:
            iMenu++;
            break;
           case otCursorIcon:
            iCursorIcon++;
            break;
           case otHook:
            iHook++;
            break;
           case otCallProc:
            iCallProc++;
            break;
           case otAccel:
            iAccel++;
            break;
           case otMonitor:
            iMonitor++;
            break;
           case otTimer:
            iTimer++;
            break;
           default:
            break;
         }
      }
      DPRINT1("Handle Count by Type:\n Free = %d Window = %d Menu = %d CursorIcon = %d Hook = %d\n CallProc = %d Accel = %d Monitor = %d Timer = %d\n",
      iFree, iWindow, iMenu, iCursorIcon, iHook, iCallProc, iAccel, iMonitor, iTimer );
//#endif
      return NULL;
#if 0
      PUSER_HANDLE_ENTRY new_handles;
      /* grow array by 50% (but at minimum 32 entries) */
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
   ret = entry->ptr;
   entry->ptr  = ht->freelist;
   entry->type = 0;
   entry->flags = 0;
   entry->pi = NULL;
   ht->freelist  = entry;

   gpsi->cHandleEntries--;

   return ret;
}

static __inline PVOID
UserHandleOwnerByType(USER_OBJECT_TYPE type)
{
    PVOID pi;

    switch (type)
    {
        case otWindow:
        case otInputContext:
            pi = GetW32ThreadInfo();
            break;

        case otMenu:
        case otCursorIcon:
        case otHook:
        case otCallProc:
        case otAccel:
            pi = GetW32ProcessInfo();
            break;

        case otMonitor:
            pi = NULL; /* System */
            break;

        default:
            pi = NULL;
            break;
    }

    return pi;
}

/* allocate a user handle for a given object */
HANDLE UserAllocHandle(PUSER_HANDLE_TABLE ht, PVOID object, USER_OBJECT_TYPE type )
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

/* return a pointer to a user object from its handle */
PVOID UserGetObject(PUSER_HANDLE_TABLE ht, HANDLE handle, USER_OBJECT_TYPE type )
{
   PUSER_HANDLE_ENTRY entry;

   ASSERT(ht);

   if (!(entry = handle_to_entry(ht, handle )) || entry->type != type)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return NULL;
   }
   return entry->ptr;
}


/* get the full handle (32bit) for a possibly truncated (16bit) handle */
HANDLE get_user_full_handle(PUSER_HANDLE_TABLE ht,  HANDLE handle )
{
   PUSER_HANDLE_ENTRY entry;

   if ((unsigned int)handle >> 16)
      return handle;
   if (!(entry = handle_to_entry(ht, handle )))
      return handle;
   return entry_to_handle( ht, entry );
}


/* same as get_user_object plus set the handle to the full 32-bit value */
void *get_user_object_handle(PUSER_HANDLE_TABLE ht,  HANDLE* handle, USER_OBJECT_TYPE type )
{
   PUSER_HANDLE_ENTRY entry;

   if (!(entry = handle_to_entry(ht, *handle )) || entry->type != type)
      return NULL;
   *handle = entry_to_handle( ht, entry );
   return entry->ptr;
}

/* return the next user handle after 'handle' that is of a given type */
PVOID UserGetNextHandle(PUSER_HANDLE_TABLE ht, HANDLE* handle, USER_OBJECT_TYPE type )
{
   PUSER_HANDLE_ENTRY entry;

   if (!*handle)
      entry = ht->handles;
   else
   {
      int index = (((unsigned int)*handle & 0xffff) - FIRST_USER_HANDLE) >> 1;
      if (index < 0 || index >= ht->nb_handles)
         return NULL;
      entry = ht->handles + index + 1;  /* start from the next one */
   }
   while (entry < ht->handles + ht->nb_handles)
   {
      if (!type || entry->type == type)
      {
         *handle = entry_to_handle(ht, entry );
         return entry->ptr;
      }
      entry++;
   }
   return NULL;
}

BOOL FASTCALL UserCreateHandleTable(VOID)
{

   PVOID mem;

   //FIXME: dont alloc all at once! must be mapped into umode also...
   //mem = ExAllocatePool(PagedPool, sizeof(USER_HANDLE_ENTRY) * 1024*2);
   mem = UserHeapAlloc(sizeof(USER_HANDLE_ENTRY) * 1024*2);
   if (!mem)
   {
      DPRINT1("Failed creating handle table\n");
      return FALSE;
   }

   gHandleTable = UserHeapAlloc(sizeof(USER_HANDLE_TABLE));
   if (gHandleTable == NULL)
   {
       UserHeapFree(mem);
       DPRINT1("Failed creating handle table\n");
       return FALSE;
   }

   //FIXME: make auto growable
   UserInitHandleTable(gHandleTable, mem, sizeof(USER_HANDLE_ENTRY) * 1024*2);

   return TRUE;
}

//
// New
//
PVOID
FASTCALL
UserCreateObject( PUSER_HANDLE_TABLE ht,
                  PDESKTOP pDesktop,
                  HANDLE* h,
                  USER_OBJECT_TYPE type,
                  ULONG size)
{
   HANDLE hi;
   PVOID Object;
   PTHREADINFO pti;
   PPROCESSINFO ppi;
   BOOL dt;
   PDESKTOP rpdesk = pDesktop;

   pti = GetW32ThreadInfo();
   ppi = pti->ppi;
   if (!pDesktop) rpdesk = pti->rpdesk;

   switch (type)
   {
//      case otWindow:
//      case otMenu:
//      case otHook:
//      case otCallProc:
      case otInputContext:
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

   RtlZeroMemory(Object, size);

   switch (type)
   {
        case otWindow:
        case otHook:
        case otInputContext:
            ((PTHRDESKHEAD)Object)->rpdesk = rpdesk;
            ((PTHRDESKHEAD)Object)->pSelf = Object;
        case otEvent:
            ((PTHROBJHEAD)Object)->pti = pti;
            break;

        case otMenu:
        case otCallProc:
            ((PPROCDESKHEAD)Object)->rpdesk = rpdesk;
            ((PPROCDESKHEAD)Object)->pSelf = Object;            
            break;

        case otCursorIcon:
            ((PPROCMARKHEAD)Object)->ppi = ppi;
            break;

        default:
            break;
   }
   /* Now set default headers. */
   ((PHEAD)Object)->h = hi;
   ((PHEAD)Object)->cLockObj = 2; // we need this, because we create 2 refs: handle and pointer!

   if (h)
      *h = hi;
   return Object;
}


BOOL
FASTCALL
UserDereferenceObject(PVOID object)
{
  PUSER_HANDLE_ENTRY entry;
  USER_OBJECT_TYPE type;

  ASSERT(((PHEAD)object)->cLockObj >= 1);

  if ((INT)--((PHEAD)object)->cLockObj <= 0)
  {
     entry = handle_to_entry(gHandleTable, ((PHEAD)object)->h );

     DPRINT("warning! Dereference to zero! Obj -> 0x%x\n", object);

     ((PHEAD)object)->cLockObj = 0;

     if (!(entry->flags & HANDLEENTRY_INDESTROY))
        return TRUE;

     type = entry->type;
     free_user_entry(gHandleTable, entry );

     switch (type)
     {
//        case otWindow:
//        case otMenu:
//        case otHook:
//        case otCallProc:
        case otInputContext:
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
UserDeleteObject(HANDLE h, USER_OBJECT_TYPE type )
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
UserReferenceObjectByHandle(HANDLE handle, USER_OBJECT_TYPE type)
{
    PVOID object;

    object = UserGetObject(gHandleTable, handle, type);
    if (object)
    {
       UserReferenceObject(object);
    }
    return object;
}
