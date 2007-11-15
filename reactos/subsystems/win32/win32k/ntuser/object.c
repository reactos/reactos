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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */



/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

int usedHandles=0;
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

   DPRINT("handles used %i\n",usedHandles);

   if (ht->freelist)
   {
      entry = ht->freelist;
      ht->freelist = entry->ptr;

      usedHandles++;
      return entry;
   }

   if (ht->nb_handles >= ht->allocated_handles)  /* need to grow the array */
   {
      DPRINT1("Out of user handles!\n");
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

   usedHandles++;

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
   entry->pi = NULL;
   ht->freelist  = entry;

   usedHandles--;

   return ret;
}

static __inline PVOID
UserHandleOwnerByType(USER_OBJECT_TYPE type)
{
    PVOID pi;

    switch (type)
    {
        case otWindow:
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
   entry->pi = UserHandleOwnerByType(type);
   if (++entry->generation >= 0xffff)
      entry->generation = 1;
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

/* free a user handle and return a pointer to the object */
PVOID UserFreeHandle(PUSER_HANDLE_TABLE ht,  HANDLE handle )
{
   PUSER_HANDLE_ENTRY entry;

   if (!(entry = handle_to_entry( ht, handle )))
   {
      SetLastNtError( STATUS_INVALID_HANDLE );
      return NULL;
   }

   return free_user_entry(ht, entry );
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



PVOID FASTCALL
ObmCreateObject(PUSER_HANDLE_TABLE ht, HANDLE* h,USER_OBJECT_TYPE type , ULONG size)
{

   HANDLE hi;
   PUSER_OBJECT_HEADER hdr = UserHeapAlloc(size + sizeof(USER_OBJECT_HEADER));//ExAllocatePool(PagedPool, size + sizeof(USER_OBJECT_HEADER));
   if (!hdr)
      return NULL;


   hi = UserAllocHandle(ht, USER_HEADER_TO_BODY(hdr), type );
   if (!hi)
   {
      //ExFreePool(hdr);
       UserHeapFree(hdr);
      return NULL;
   }

   RtlZeroMemory(hdr, size + sizeof(USER_OBJECT_HEADER));
   hdr->hSelf = hi;
   hdr->RefCount++; //temp hack!

   if (h)
      *h = hi;
   return USER_HEADER_TO_BODY(hdr);
}

BOOL FASTCALL
ObmDeleteObject(HANDLE h, USER_OBJECT_TYPE type )
{
   PUSER_OBJECT_HEADER hdr;
   PVOID body = UserGetObject(gHandleTable, h, type);
   if (!body)
      return FALSE;

   hdr = USER_BODY_TO_HEADER(body);
   ASSERT(hdr->RefCount >= 0);

   hdr->destroyed = TRUE;
   if (hdr->RefCount == 0)
   {
      UserFreeHandle(gHandleTable, h);

      memset(hdr, 0x55, sizeof(USER_OBJECT_HEADER));

      UserHeapFree(hdr);
      //ExFreePool(hdr);
      return TRUE;
   }

//   DPRINT1("info: something not destroyed bcause refs still left, inuse %i\n",usedHandles);
   return FALSE;
}


VOID FASTCALL ObmReferenceObject(PVOID obj)
{
   PUSER_OBJECT_HEADER hdr = USER_BODY_TO_HEADER(obj);

   ASSERT(hdr->RefCount >= 0);

   hdr->RefCount++;
}

HANDLE FASTCALL ObmObjectToHandle(PVOID obj)
{
    PUSER_OBJECT_HEADER hdr = USER_BODY_TO_HEADER(obj);
    return hdr->hSelf;
}


BOOL FASTCALL ObmDereferenceObject2(PVOID obj)
{
   PUSER_OBJECT_HEADER hdr = USER_BODY_TO_HEADER(obj);

   ASSERT(hdr->RefCount >= 1);

   hdr->RefCount--;

   // You can not have a zero here!
   if (!hdr->destroyed && hdr->RefCount == 0) hdr->RefCount++; // BOUNCE!!!!!

   if (hdr->RefCount == 0 && hdr->destroyed)
   {
//      DPRINT1("info: something destroyed bcaise of deref, in use=%i\n",usedHandles);

      UserFreeHandle(gHandleTable, hdr->hSelf);

      memset(hdr, 0x55, sizeof(USER_OBJECT_HEADER));

      UserHeapFree(hdr);
      //ExFreePool(hdr);

      return TRUE;
   }

   return FALSE;
}



BOOL FASTCALL ObmCreateHandleTable()
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
