/*
 *	Running Object Table
 *
 *      Copyright 2007  Robert Shearman
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

#include <stdarg.h>
#include <string.h>

#include "winerror.h"
#include "windef.h"
#include "winbase.h"

#include "irot.h"

#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpcss);

/* define the structure of the running object table elements */
struct rot_entry
{
    struct list        entry;
    InterfaceData *object; /* marshaled running object*/
    InterfaceData *moniker; /* marshaled moniker that identifies this object */
    MonikerComparisonData *moniker_data; /* moniker comparison data that identifies this object */
    DWORD              cookie; /* cookie identifying this object */
    FILETIME           last_modified;
    LONG               refs;
};

static struct list RunningObjectTable = LIST_INIT(RunningObjectTable);

static CRITICAL_SECTION csRunningObjectTable;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &csRunningObjectTable,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": csRunningObjectTable") }
};
static CRITICAL_SECTION csRunningObjectTable = { &critsect_debug, -1, 0, 0, 0, 0 };

static LONG last_cookie = 1;

static inline void rot_entry_release(struct rot_entry *rot_entry)
{
    if (!InterlockedDecrement(&rot_entry->refs))
    {
        free(rot_entry->object);
        free(rot_entry->moniker);
        free(rot_entry->moniker_data);
        free(rot_entry);
    }
}

HRESULT __cdecl IrotRegister(
    IrotHandle h,
    const MonikerComparisonData *data,
    const InterfaceData *obj,
    const InterfaceData *mk,
    const FILETIME *time,
    DWORD grfFlags,
    IrotCookie *cookie,
    IrotContextHandle *ctxt_handle)
{
    struct rot_entry *rot_entry;
    struct rot_entry *existing_rot_entry;
    HRESULT hr;

    if (grfFlags & ~(ROTFLAGS_REGISTRATIONKEEPSALIVE|ROTFLAGS_ALLOWANYCLIENT))
    {
        WINE_ERR("Invalid grfFlags: 0x%08lx\n", grfFlags & ~(ROTFLAGS_REGISTRATIONKEEPSALIVE|ROTFLAGS_ALLOWANYCLIENT));
        return E_INVALIDARG;
    }

    if (!(rot_entry = calloc(1, sizeof(*rot_entry))))
        return E_OUTOFMEMORY;

    rot_entry->refs = 1;
    rot_entry->object = malloc(FIELD_OFFSET(InterfaceData, abData[obj->ulCntData]));
    if (!rot_entry->object)
    {
        rot_entry_release(rot_entry);
        return E_OUTOFMEMORY;
    }
    rot_entry->object->ulCntData = obj->ulCntData;
    memcpy(&rot_entry->object->abData, obj->abData, obj->ulCntData);

    rot_entry->last_modified = *time;

    rot_entry->moniker = malloc(FIELD_OFFSET(InterfaceData, abData[mk->ulCntData]));
    if (!rot_entry->moniker)
    {
        rot_entry_release(rot_entry);
        return E_OUTOFMEMORY;
    }
    rot_entry->moniker->ulCntData = mk->ulCntData;
    memcpy(&rot_entry->moniker->abData, mk->abData, mk->ulCntData);

    if (!(rot_entry->moniker_data = malloc(FIELD_OFFSET(MonikerComparisonData, abData[data->ulCntData]))))
    {
        rot_entry_release(rot_entry);
        return E_OUTOFMEMORY;
    }
    rot_entry->moniker_data->ulCntData = data->ulCntData;
    memcpy(&rot_entry->moniker_data->abData, data->abData, data->ulCntData);

    EnterCriticalSection(&csRunningObjectTable);

    hr = S_OK;

    LIST_FOR_EACH_ENTRY(existing_rot_entry, &RunningObjectTable, struct rot_entry, entry)
    {
        if ((existing_rot_entry->moniker_data->ulCntData == data->ulCntData) &&
            !memcmp(&data->abData, &existing_rot_entry->moniker_data->abData, data->ulCntData))
        {
            hr = MK_S_MONIKERALREADYREGISTERED;
            WINE_TRACE("moniker already registered with cookie %ld\n", existing_rot_entry->cookie);
            break;
        }
    }

    list_add_tail(&RunningObjectTable, &rot_entry->entry);

    LeaveCriticalSection(&csRunningObjectTable);

    /* gives a registration identifier to the registered object*/
    *cookie = rot_entry->cookie = InterlockedIncrement(&last_cookie);
    *ctxt_handle = rot_entry;

    return hr;
}

HRESULT __cdecl IrotRevoke(
    IrotHandle h,
    IrotCookie cookie,
    IrotContextHandle *ctxt_handle,
    PInterfaceData *obj,
    PInterfaceData *mk)
{
    struct rot_entry *rot_entry;

    WINE_TRACE("%ld\n", cookie);

    EnterCriticalSection(&csRunningObjectTable);
    LIST_FOR_EACH_ENTRY(rot_entry, &RunningObjectTable, struct rot_entry, entry)
    {
        if (rot_entry->cookie == cookie)
        {
            HRESULT hr = S_OK;

            list_remove(&rot_entry->entry);
            LeaveCriticalSection(&csRunningObjectTable);

            *obj = MIDL_user_allocate(FIELD_OFFSET(InterfaceData, abData[rot_entry->object->ulCntData]));
            *mk = MIDL_user_allocate(FIELD_OFFSET(InterfaceData, abData[rot_entry->moniker->ulCntData]));
            if (*obj && *mk)
            {
                (*obj)->ulCntData = rot_entry->object->ulCntData;
                memcpy((*obj)->abData, rot_entry->object->abData, (*obj)->ulCntData);
                (*mk)->ulCntData = rot_entry->moniker->ulCntData;
                memcpy((*mk)->abData, rot_entry->moniker->abData, (*mk)->ulCntData);
            }
            else
            {
                MIDL_user_free(*obj);
                MIDL_user_free(*mk);
                hr = E_OUTOFMEMORY;
            }

            rot_entry_release(rot_entry);
            *ctxt_handle = NULL;
            return hr;
        }
    }
    LeaveCriticalSection(&csRunningObjectTable);

    return E_INVALIDARG;
}

HRESULT __cdecl IrotIsRunning(
    IrotHandle h,
    const MonikerComparisonData *data)
{
    const struct rot_entry *rot_entry;
    HRESULT hr = S_FALSE;

    WINE_TRACE("\n");

    EnterCriticalSection(&csRunningObjectTable);

    LIST_FOR_EACH_ENTRY(rot_entry, &RunningObjectTable, const struct rot_entry, entry)
    {
        if ((rot_entry->moniker_data->ulCntData == data->ulCntData) &&
            !memcmp(&data->abData, &rot_entry->moniker_data->abData, data->ulCntData))
        {
            hr = S_OK;
            break;
        }
    }
    LeaveCriticalSection(&csRunningObjectTable);

    return hr;
}

HRESULT __cdecl IrotGetObject(
    IrotHandle h,
    const MonikerComparisonData *moniker_data,
    PInterfaceData *obj,
    IrotCookie *cookie)
{
    const struct rot_entry *rot_entry;

    WINE_TRACE("%p\n", moniker_data);

    *cookie = 0;

    EnterCriticalSection(&csRunningObjectTable);

    LIST_FOR_EACH_ENTRY(rot_entry, &RunningObjectTable, const struct rot_entry, entry)
    {
        HRESULT hr = S_OK;
        if ((rot_entry->moniker_data->ulCntData == moniker_data->ulCntData) &&
            !memcmp(&moniker_data->abData, &rot_entry->moniker_data->abData, moniker_data->ulCntData))
        {
            *obj = MIDL_user_allocate(FIELD_OFFSET(InterfaceData, abData[rot_entry->object->ulCntData]));
            if (*obj)
            {
                (*obj)->ulCntData = rot_entry->object->ulCntData;
                memcpy((*obj)->abData, rot_entry->object->abData, (*obj)->ulCntData);

                *cookie = rot_entry->cookie;
            }
            else
                hr = E_OUTOFMEMORY;

            LeaveCriticalSection(&csRunningObjectTable);

            return hr;
        }
    }

    LeaveCriticalSection(&csRunningObjectTable);

    return MK_E_UNAVAILABLE;
}

HRESULT __cdecl IrotNoteChangeTime(
    IrotHandle h,
    IrotCookie cookie,
    const FILETIME *last_modified_time)
{
    struct rot_entry *rot_entry;

    WINE_TRACE("%ld %p\n", cookie, last_modified_time);

    EnterCriticalSection(&csRunningObjectTable);
    LIST_FOR_EACH_ENTRY(rot_entry, &RunningObjectTable, struct rot_entry, entry)
    {
        if (rot_entry->cookie == cookie)
        {
            rot_entry->last_modified = *last_modified_time;
            LeaveCriticalSection(&csRunningObjectTable);
            return S_OK;
        }
    }
    LeaveCriticalSection(&csRunningObjectTable);

    return E_INVALIDARG;
}

HRESULT __cdecl IrotGetTimeOfLastChange(
    IrotHandle h,
    const MonikerComparisonData *moniker_data,
    FILETIME *time)
{
    const struct rot_entry *rot_entry;
    HRESULT hr = MK_E_UNAVAILABLE;

    WINE_TRACE("%p\n", moniker_data);

    memset(time, 0, sizeof(*time));

    EnterCriticalSection(&csRunningObjectTable);
    LIST_FOR_EACH_ENTRY(rot_entry, &RunningObjectTable, const struct rot_entry, entry)
    {
        if ((rot_entry->moniker_data->ulCntData == moniker_data->ulCntData) &&
            !memcmp(&moniker_data->abData, &rot_entry->moniker_data->abData, moniker_data->ulCntData))
        {
            *time = rot_entry->last_modified;
            hr = S_OK;
            break;
        }
    }
    LeaveCriticalSection(&csRunningObjectTable);

    return hr;
}

HRESULT __cdecl IrotEnumRunning(
    IrotHandle h,
    PInterfaceList *list)
{
    const struct rot_entry *rot_entry;
    HRESULT hr = S_OK;
    ULONG moniker_count = 0;
    ULONG i = 0;

    WINE_TRACE("\n");

    EnterCriticalSection(&csRunningObjectTable);

    LIST_FOR_EACH_ENTRY( rot_entry, &RunningObjectTable, const struct rot_entry, entry )
        moniker_count++;

    *list = MIDL_user_allocate(FIELD_OFFSET(InterfaceList, interfaces[moniker_count]));
    if (*list)
    {
        (*list)->size = moniker_count;
        LIST_FOR_EACH_ENTRY( rot_entry, &RunningObjectTable, const struct rot_entry, entry )
        {
            (*list)->interfaces[i] = MIDL_user_allocate(FIELD_OFFSET(InterfaceData, abData[rot_entry->moniker->ulCntData]));
            if (!(*list)->interfaces[i])
            {
                ULONG end = i - 1;
                for (i = 0; i < end; i++)
                    MIDL_user_free((*list)->interfaces[i]);
                MIDL_user_free(*list);
                hr = E_OUTOFMEMORY;
                break;
            }
            (*list)->interfaces[i]->ulCntData = rot_entry->moniker->ulCntData;
            memcpy((*list)->interfaces[i]->abData, rot_entry->moniker->abData, rot_entry->moniker->ulCntData);
            i++;
        }
    }
    else
        hr = E_OUTOFMEMORY;

    LeaveCriticalSection(&csRunningObjectTable);

    return hr;
}

void __RPC_USER IrotContextHandle_rundown(IrotContextHandle ctxt_handle)
{
    struct rot_entry *rot_entry = ctxt_handle;
    EnterCriticalSection(&csRunningObjectTable);
    list_remove(&rot_entry->entry);
    LeaveCriticalSection(&csRunningObjectTable);
    rot_entry_release(rot_entry);
}

void * __RPC_USER MIDL_user_allocate(SIZE_T size)
{
    return I_RpcAllocate(size);
}

void __RPC_USER MIDL_user_free(void * p)
{
    I_RpcFree(p);
}
