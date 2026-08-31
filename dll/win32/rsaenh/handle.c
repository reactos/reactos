/*
 * dlls/rsaenh/handle.c
 * Support code to manage HANDLE tables.
 *
 * Copyright 1998 Alexandre Julliard
 * Copyright 2002-2004 Mike McCormack for CodeWeavers
 * Copyright 2004 Michael Jung
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "handle.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(handle);

#define HANDLE2INDEX(h) ((h)-1)
#define INDEX2HANDLE(i) ((i)+1)

/******************************************************************************
 *  init_handle_table
 *
 * Initializes the HANDLETABLE structure pointed to by lpTable
 *
 * PARAMS
 *  lpTable [I] Pointer to the HANDLETABLE structure, which is to be initialized.
 *
 * NOTES
 *  You have to call destroy_handle_table when you don't need the table
 *  any more.
 */
void init_handle_table(struct handle_table *lpTable)
{
    TRACE("(lpTable=%p)\n", lpTable);
        
    lpTable->paEntries = NULL;
    lpTable->iEntries = 0;
    lpTable->iFirstFree = 0;
    InitializeCriticalSectionEx(&lpTable->mutex, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    lpTable->mutex.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": HANDLETABLE.mutex");
}

/******************************************************************************
 *  destroy_handle_table
 *
 * Destroys the handle table.
 * 
 * PARAMS
 *  lpTable [I] Pointer to the handle table, which is to be destroyed.
 */
void destroy_handle_table(struct handle_table *lpTable)
{
    TRACE("(lpTable=%p)\n", lpTable);
        
    free(lpTable->paEntries);
    lpTable->mutex.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&lpTable->mutex);
}

/******************************************************************************
 *  is_valid_handle
 *
 * Tests if handle is valid given the specified handle table
 * 
 * PARAMS
 *  lpTable [I] Pointer to the handle table, with respect to which the handle's 
 *              validness is tested.
 *  handle  [I] The handle tested for validness.
 *  dwType  [I] A magic value that identifies the referenced object's type.
 *
 * RETURNS
 *  TRUE,      if handle is valid.
 *  FALSE,     if handle is not valid.
 */
BOOL is_valid_handle(struct handle_table *lpTable, HCRYPTKEY handle, DWORD dwType)
{
    unsigned int index = HANDLE2INDEX(handle);
    BOOL ret = FALSE;

    TRACE("(lpTable=%p, handle=%Id)\n", lpTable, handle);
    
    EnterCriticalSection(&lpTable->mutex);
        
    /* We don't use zero handle values */
    if (!handle) goto exit;
 
    /* Check for index out of table bounds */    
    if (index >= lpTable->iEntries) goto exit;
    
    /* Check if this handle is currently allocated */
    if (!lpTable->paEntries[index].pObject) goto exit;
    
    /* Check if this handle references an object of the correct type. */
    if (lpTable->paEntries[index].pObject->dwType != dwType) goto exit;

    ret = TRUE;
exit:
    LeaveCriticalSection(&lpTable->mutex);
    return ret;
}

/******************************************************************************
 *  grow_handle_table [Internal]
 *
 * Grows the number of entries in the given table by TABLE_SIZE_INCREMENT
 *
 * PARAMS 
 *  lpTable [I] Pointer to the table, which is to be grown
 *
 * RETURNS
 *  TRUE,      if successful
 *  FALSE,     if not successful (out of memory on process heap)
 *
 * NOTES
 *  This is a support function for alloc_handle. Do not call!
 */
static BOOL grow_handle_table(struct handle_table *lpTable)
{
    struct handle_table_entry *newEntries;
    unsigned int i, newIEntries;

    newIEntries = lpTable->iEntries + TABLE_SIZE_INCREMENT;

    newEntries = malloc(sizeof(struct handle_table_entry)*newIEntries);
    if (!newEntries)
        return FALSE;

    if (lpTable->paEntries)
    {
        memcpy(newEntries, lpTable->paEntries, sizeof(struct handle_table_entry)*lpTable->iEntries);
        free(lpTable->paEntries);
    }

    for (i=lpTable->iEntries; i<newIEntries; i++)
    {
        newEntries[i].pObject = NULL;
        newEntries[i].iNextFree = i+1;
    }

    lpTable->paEntries = newEntries;
    lpTable->iEntries = newIEntries;

    return TRUE;
}

/******************************************************************************
 *  alloc_handle
 *
 * Allocates a new handle to the specified object in a given handle table.
 *
 * PARAMS
 *  lpTable  [I] Pointer to the handle table, from which the new handle is 
 *               allocated.
 *  lpObject [I] Pointer to the object, for which a handle shall be allocated.
 *  lpHandle [O] Pointer to a handle variable, into which the handle value will
 *               be stored. If not successful, this will be 
 *               INVALID_HANDLE_VALUE
 * RETURNS
 *  TRUE,      if successful
 *  FALSE,     if not successful (no free handle)
 */
static BOOL alloc_handle(struct handle_table *lpTable, OBJECTHDR *lpObject, HCRYPTKEY *lpHandle)
{
    BOOL ret = FALSE;

    TRACE("(lpTable=%p, lpObject=%p, lpHandle=%p)\n", lpTable, lpObject, lpHandle);
        
    EnterCriticalSection(&lpTable->mutex);
    if (lpTable->iFirstFree >= lpTable->iEntries) 
        if (!grow_handle_table(lpTable))
        {
            *lpHandle = (HCRYPTKEY)INVALID_HANDLE_VALUE;
            goto exit;
        }

    *lpHandle = INDEX2HANDLE(lpTable->iFirstFree);
    
    lpTable->paEntries[lpTable->iFirstFree].pObject = lpObject;
    lpTable->iFirstFree = lpTable->paEntries[lpTable->iFirstFree].iNextFree;
    InterlockedIncrement(&lpObject->refcount);

    ret = TRUE;
exit:
    LeaveCriticalSection(&lpTable->mutex);
    return ret;
}

/******************************************************************************
 *  release_handle
 *
 * Releases resources occupied by the specified handle in the given table.
 * The reference count of the handled object is decremented. If it becomes
 * zero and if the 'destructor' function pointer member is non NULL, the
 * destructor function will be called. Note that release_handle does not 
 * release resources other than the handle itself. If this is wanted, do it
 * in the destructor function.
 *
 * PARAMS
 *  lpTable [I] Pointer to the handle table, from which a handle is to be 
 *              released.
 *  handle  [I] The handle, which is to be released
 *  dwType  [I] Identifier for the type of the object, for which a handle is
 *              to be released.
 *
 * RETURNS
 *  TRUE,      if successful
 *  FALSE,     if not successful (invalid handle)
 */
BOOL release_handle(struct handle_table *lpTable, HCRYPTKEY handle, DWORD dwType)
{
    unsigned int index = HANDLE2INDEX(handle);
    OBJECTHDR *pObject;
    BOOL ret = FALSE;

    TRACE("(lpTable=%p, handle=%Id)\n", lpTable, handle);
    
    EnterCriticalSection(&lpTable->mutex);
    
    if (!is_valid_handle(lpTable, handle, dwType))
        goto exit;

    pObject = lpTable->paEntries[index].pObject;
    if (InterlockedDecrement(&pObject->refcount) == 0)
    {
        TRACE("destroying handle %Id\n", handle);
        if (pObject->destructor)
            pObject->destructor(pObject);
    }

    lpTable->paEntries[index].pObject = NULL;
    lpTable->paEntries[index].iNextFree = lpTable->iFirstFree;
    lpTable->iFirstFree = index;

    ret = TRUE;
exit:
    LeaveCriticalSection(&lpTable->mutex);
    return ret;
}

/******************************************************************************
 *  lookup_handle
 *
 * Returns the object identified by the handle in the given handle table
 *
 * PARAMS
 *  lpTable    [I] Pointer to the handle table, in which the handle is looked up.
 *  handle     [I] The handle, which is to be looked up
 *  lplpObject [O] Pointer to the variable, into which the pointer to the
 *                 object looked up is copied.
 * RETURNS
 *  TRUE,      if successful
 *  FALSE,     if not successful (invalid handle)
 */
BOOL lookup_handle(struct handle_table *lpTable, HCRYPTKEY handle, DWORD dwType, OBJECTHDR **lplpObject)
{
    BOOL ret = FALSE;

    TRACE("(lpTable=%p, handle=%Id, lplpObject=%p)\n", lpTable, handle, lplpObject);
    
    EnterCriticalSection(&lpTable->mutex);
    if (!is_valid_handle(lpTable, handle, dwType)) 
    {
        *lplpObject = NULL;
        goto exit;
    }
    *lplpObject = lpTable->paEntries[HANDLE2INDEX(handle)].pObject;

    ret = TRUE;
exit:
    LeaveCriticalSection(&lpTable->mutex);
    return ret;
}

/******************************************************************************
 *  copy_handle
 *
 * Copies a handle. Increments the reference count of the object referenced
 * by the handle.
 *
 * PARAMS
 *  lpTable [I] Pointer to the handle table, which holds the handle to be copied.
 *  handle  [I] The handle to be copied.
 *  copy    [O] Pointer to a handle variable, where the copied handle is put.
 *
 * RETURNS
 *  TRUE,      if successful
 *  FALSE,     if not successful (invalid handle or out of memory)
 */
BOOL copy_handle(struct handle_table *lpTable, HCRYPTKEY handle, DWORD dwType, HCRYPTKEY *copy)
{
    OBJECTHDR *pObject;
    BOOL ret;

    TRACE("(lpTable=%p, handle=%Id, copy=%p)\n", lpTable, handle, copy);

    EnterCriticalSection(&lpTable->mutex);
    if (!lookup_handle(lpTable, handle, dwType, &pObject)) 
    {
        *copy = (HCRYPTKEY)INVALID_HANDLE_VALUE;
        LeaveCriticalSection(&lpTable->mutex);
        return FALSE;
    }

    ret = alloc_handle(lpTable, pObject, copy);
    LeaveCriticalSection(&lpTable->mutex);
    return ret;
}

/******************************************************************************
 *  new_object
 *
 * Allocates a new object of size cbSize on the current process's heap.
 * Initializes the object header using the destructor and dwType params.
 * Allocates a handle to the object in the handle table pointed to by lpTable.
 * Returns a pointer to the created object in ppObject.
 * Returns a handle to the created object.
 *
 * PARAMS
 *  lpTable    [I] Pointer to the handle table, from which a handle is to be 
 *              allocated.
 *  cbSize     [I] Size of the object to be allocated in bytes.
 *  dwType     [I] Object type; will be copied to the object header.
 *  destructor [I] Function pointer to a destructor function. Will be called
 *                 once the object's reference count gets zero.
 *  ppObject   [O] Pointer to a pointer variable, where a pointer to the newly
 *                 created object will be stored. You may set this to NULL.
 *
 * RETURNS
 *  INVALID_HANDLE_VALUE,        if something went wrong.
 *  a handle to the new object,  if successful. 
 */
HCRYPTKEY new_object(struct handle_table *lpTable, size_t cbSize, DWORD dwType, DESTRUCTOR destructor,
                        OBJECTHDR **ppObject)
{
    OBJECTHDR *pObject;
    HCRYPTKEY hObject;

    if (ppObject)
        *ppObject = NULL;

    pObject = malloc(cbSize);
    if (!pObject)
        return (HCRYPTKEY)INVALID_HANDLE_VALUE;

    pObject->dwType = dwType;
    pObject->refcount = 0;
    pObject->destructor = destructor;

    if (!alloc_handle(lpTable, pObject, &hObject))
        free(pObject);
    else
        if (ppObject)
            *ppObject = pObject;

    return hObject;
}
