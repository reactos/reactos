/* 
 * DirectPlay8 Address
 * 
 * Copyright 2004 Raphael Junqueira
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
 *
 */


#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"

#include "wine/debug.h"

#include "dpnet_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpnet);


static BOOL add_component(IDirectPlay8AddressImpl *This, struct component *item)
{
    if(This->comp_count == This->comp_array_size)
    {
        struct component **temp;

        temp = realloc(This->components, sizeof(*temp) * This->comp_array_size * 2);
        if(!temp)
        {
            return FALSE;
        }

        This->comp_array_size *= 2;
        This->components = temp;
    }

    This->components[This->comp_count] = item;
    This->comp_count++;

    return TRUE;
}

static inline IDirectPlay8AddressImpl *impl_from_IDirectPlay8Address(IDirectPlay8Address *iface)
{
    return CONTAINING_RECORD(iface, IDirectPlay8AddressImpl, IDirectPlay8Address_iface);
}

/* IDirectPlay8Address IUnknown parts follow: */
static HRESULT WINAPI IDirectPlay8AddressImpl_QueryInterface(IDirectPlay8Address *iface,
        REFIID riid, void **ppv)
{
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectPlay8Address)) {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return DPN_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectPlay8AddressImpl_AddRef(IDirectPlay8Address *iface)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectPlay8AddressImpl_Release(IDirectPlay8Address *iface)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    if (!ref)
    {
        struct component *entry;
        DWORD i;

        for(i=0; i < This->comp_count; i++)
        {
            entry = This->components[i];

            switch(entry->type)
            {
                case DPNA_DATATYPE_STRING:
                    free(entry->data.string);
                    break;
                case DPNA_DATATYPE_STRING_ANSI:
                    free(entry->data.ansi);
                    break;
                case DPNA_DATATYPE_BINARY:
                    free(entry->data.binary);
                    break;
            }

            free(entry->name);
            free(entry);
        }

        free(This->components);
        free(This);
    }
    return ref;
}

/* returns name of given GUID */
static const char *debugstr_SP(const GUID *id) {
  static const guid_info guids[] = {
    /* CLSIDs */
    GE(CLSID_DP8SP_IPX),
    GE(CLSID_DP8SP_TCPIP),
    GE(CLSID_DP8SP_SERIAL),
    GE(CLSID_DP8SP_MODEM)
  };
  unsigned int i;

  if (!id) return "(null)";

  for (i = 0; i < ARRAY_SIZE(guids); i++) {
    if (IsEqualGUID(id, guids[i].guid))
      return guids[i].name;
  }
  /* if we didn't find it, act like standard debugstr_guid */
  return debugstr_guid(id);
}

/* IDirectPlay8Address Interface follow: */

static HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromURLW(IDirectPlay8Address *iface,
        WCHAR *pwszSourceURL)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %s): stub\n", This, debugstr_w(pwszSourceURL));
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromURLA(IDirectPlay8Address *iface,
       CHAR *pszSourceURL)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %s): stub\n", This, pszSourceURL);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_Duplicate(IDirectPlay8Address *iface,
        IDirectPlay8Address **ppdpaNewAddress)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    IDirectPlay8Address *dup;
    HRESULT hr;

    TRACE("(%p, %p)\n", This, ppdpaNewAddress);

    if(!ppdpaNewAddress)
        return E_POINTER;

    hr = DPNET_CreateDirectPlay8Address(NULL, NULL, &IID_IDirectPlay8Address, (LPVOID*)&dup);
    if(hr == S_OK)
    {
        IDirectPlay8AddressImpl *DupThis = impl_from_IDirectPlay8Address(dup);
        DWORD i;

        DupThis->SP_guid = This->SP_guid;
        DupThis->init    = This->init;

        for(i=0; i < This->comp_count; i++)
        {
            struct component *entry = This->components[i];

            switch (entry->type)
            {
                case DPNA_DATATYPE_DWORD:
                    hr = IDirectPlay8Address_AddComponent(dup, entry->name, &entry->data.value, entry->size, entry->type);
                    break;
                case DPNA_DATATYPE_GUID:
                    hr = IDirectPlay8Address_AddComponent(dup, entry->name, &entry->data.guid, entry->size, entry->type);
                    break;
                case DPNA_DATATYPE_STRING:
                    hr = IDirectPlay8Address_AddComponent(dup, entry->name, entry->data.string, entry->size, entry->type);
                    break;
                case DPNA_DATATYPE_STRING_ANSI:
                    hr = IDirectPlay8Address_AddComponent(dup, entry->name, entry->data.ansi, entry->size, entry->type);
                    break;
                case DPNA_DATATYPE_BINARY:
                    hr = IDirectPlay8Address_AddComponent(dup, entry->name, entry->data.binary, entry->size, entry->type);
                    break;
            }

            if(hr != S_OK)
            {
                IDirectPlay8Address_Release(dup);
                dup = NULL;
                ERR("Failed to copy component: %s - 0x%08lx\n", debugstr_w(entry->name), hr);
                break;
            }
        }

        *ppdpaNewAddress = dup;
    }

    return hr;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetEqual(IDirectPlay8Address *iface,
        IDirectPlay8Address *pdpaAddress)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %p): stub\n", This, pdpaAddress);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_IsEqual(IDirectPlay8Address *iface,
        IDirectPlay8Address *pdpaAddress)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %p): stub\n", This, pdpaAddress);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_Clear(IDirectPlay8Address *iface)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetURLW(IDirectPlay8Address *iface, WCHAR *url, DWORD *length)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    HRESULT hr = DPNERR_BUFFERTOOSMALL;
    int i;
    WCHAR buffer[1024];
    int position = 0;

    TRACE("(%p, %p, %p)\n", This, url, length);

    if(!length || (!url && *length != 0))
        return DPNERR_INVALIDPOINTER;

    for(i=0; i < This->comp_count; i++)
    {
        struct component *entry = This->components[i];

        if (position) buffer[position++] = ';';

        switch(entry->type)
        {
            case DPNA_DATATYPE_GUID:
                position += swprintf( &buffer[position], ARRAY_SIZE(buffer) - position,
                      L"%s=%%7B%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X%%7D",
                      entry->name, entry->data.guid.Data1, entry->data.guid.Data2, entry->data.guid.Data3,
                      entry->data.guid.Data4[0], entry->data.guid.Data4[1], entry->data.guid.Data4[2],
                      entry->data.guid.Data4[3], entry->data.guid.Data4[4], entry->data.guid.Data4[5],
                      entry->data.guid.Data4[6], entry->data.guid.Data4[7] );

                break;
            case DPNA_DATATYPE_STRING:
                position += swprintf(&buffer[position], ARRAY_SIZE(buffer) - position, L"%s=%s", entry->name, entry->data.string);
                break;
            case DPNA_DATATYPE_DWORD:
                position += swprintf(&buffer[position], ARRAY_SIZE(buffer) - position, L"%s=%ld", entry->name, entry->data.value);
                break;
            case DPNA_DATATYPE_STRING_ANSI:
                position += swprintf(&buffer[position], ARRAY_SIZE(buffer) - position, L"%s=%hs", entry->name, entry->data.ansi);
                break;
            case DPNA_DATATYPE_BINARY:
            default:
                FIXME("Unsupported type %ld\n", entry->type);
        }
    }
    buffer[position] = 0;

    if(url && *length >= lstrlenW(buffer) + lstrlenW(DPNA_HEADER) + 1)
    {
        lstrcpyW(url, DPNA_HEADER);
        lstrcatW(url, buffer);
        hr = DPN_OK;
    }

    *length = lstrlenW(buffer) + lstrlenW(DPNA_HEADER) + 1;

    return hr;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetURLA(IDirectPlay8Address *iface, char *url, DWORD *length)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    HRESULT hr;
    WCHAR *buffer = NULL;

    TRACE("(%p, %p %p)\n", This, url, length);

    if(!length || (!url && *length != 0))
        return DPNERR_INVALIDPOINTER;

    if(url && *length)
        buffer = malloc(*length * sizeof(WCHAR));

    hr = IDirectPlay8Address_GetURLW(iface, buffer, length);
    if(hr == DPN_OK)
    {
        DWORD size;
        size = WideCharToMultiByte(CP_ACP, 0, buffer, -1, NULL, 0, NULL, NULL);
        if(size <= *length)
            WideCharToMultiByte(CP_ACP, 0, buffer, -1, url, *length, NULL, NULL);
        else
        {
            *length = size;
            hr = DPNERR_BUFFERTOOSMALL;
        }
    }
    free(buffer);
    return hr;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetSP(IDirectPlay8Address *iface, GUID *pguidSP)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);

    TRACE("(%p, %p)\n", iface, pguidSP);

    if(!pguidSP)
        return DPNERR_INVALIDPOINTER;

    if(!This->init)
        return DPNERR_DOESNOTEXIST;

    *pguidSP = This->SP_guid;
    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetUserData(IDirectPlay8Address *iface,
        void *pvUserData, DWORD *pdwBufferSize)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetSP(IDirectPlay8Address *iface,
        const GUID *const pguidSP)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);

    TRACE("(%p, %s)\n", iface, debugstr_SP(pguidSP));

    if(!pguidSP)
        return DPNERR_INVALIDPOINTER;

    This->init = TRUE;
    This->SP_guid = *pguidSP;

    IDirectPlay8Address_AddComponent(iface, DPNA_KEY_PROVIDER, &This->SP_guid, sizeof(GUID), DPNA_DATATYPE_GUID);

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetUserData(IDirectPlay8Address *iface,
        const void *const pvUserData, const DWORD dwDataSize)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetNumComponents(IDirectPlay8Address *iface,
        DWORD *pdwNumComponents)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    TRACE("(%p): stub\n", This);

    if(!pdwNumComponents)
        return DPNERR_INVALIDPOINTER;

    *pdwNumComponents = This->comp_count;

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetComponentByName(IDirectPlay8Address *iface,
        const WCHAR *const pwszName, void *pvBuffer, DWORD *pdwBufferSize, DWORD *pdwDataType)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    struct component *entry;
    DWORD i;

    TRACE("(%p)->(%s %p %p %p)\n", This, debugstr_w(pwszName), pvBuffer, pdwBufferSize, pdwDataType);

    if(!pwszName || !pdwBufferSize || !pdwDataType || (!pvBuffer && *pdwBufferSize))
        return E_POINTER;

    for(i=0; i < This->comp_count; i++)
    {
        entry = This->components[i];

        if (lstrcmpW(pwszName, entry->name) == 0)
        {
            TRACE("Found %s\n", debugstr_w(pwszName));

            if(*pdwBufferSize < entry->size)
            {
                *pdwBufferSize = entry->size;
                return DPNERR_BUFFERTOOSMALL;
            }

            *pdwBufferSize = entry->size;
            *pdwDataType   = entry->type;

            switch (entry->type)
            {
                case DPNA_DATATYPE_DWORD:
                    memcpy(pvBuffer, &entry->data.value, sizeof(DWORD));
                    break;
                case DPNA_DATATYPE_GUID:
                    memcpy(pvBuffer, &entry->data.guid, sizeof(GUID));
                    break;
                case DPNA_DATATYPE_STRING:
                    memcpy(pvBuffer, entry->data.string, entry->size);
                    break;
                case DPNA_DATATYPE_STRING_ANSI:
                    memcpy(pvBuffer, entry->data.ansi, entry->size);
                    break;
                case DPNA_DATATYPE_BINARY:
                    memcpy(pvBuffer, entry->data.binary, entry->size);
                    break;
            }

            return S_OK;
        }
    }

    return DPNERR_DOESNOTEXIST;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetComponentByIndex(IDirectPlay8Address *iface,
        const DWORD dwComponentID, WCHAR *pwszName, DWORD *pdwNameLen, void *pvBuffer,
        DWORD *pdwBufferSize, DWORD *pdwDataType)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    struct component *entry;
    int namesize;

    TRACE("(%p)->(%lu %p %p %p %p %p)\n", This, dwComponentID, pwszName, pdwNameLen, pvBuffer, pdwBufferSize, pdwDataType);

    if(!pdwNameLen || !pdwBufferSize || !pdwDataType)
    {
        WARN("Invalid buffer (%p, %p, %p)\n", pdwNameLen, pdwBufferSize, pdwDataType);
        return DPNERR_INVALIDPOINTER;
    }

    if(dwComponentID > This->comp_count)
    {
        WARN("dwComponentID out of range\n");
        return DPNERR_DOESNOTEXIST;
    }

    entry = This->components[dwComponentID];

    namesize = lstrlenW(entry->name);
    if(*pdwBufferSize < entry->size || *pdwNameLen < namesize)
    {
        WARN("Buffer too small\n");

        *pdwNameLen = namesize + 1;
        *pdwBufferSize = entry->size;
        *pdwDataType   = entry->type;
        return DPNERR_BUFFERTOOSMALL;
    }

    if(!pwszName || !pvBuffer)
    {
        WARN("Invalid buffer (%p, %p)\n", pwszName, pvBuffer);
        return DPNERR_INVALIDPOINTER;
    }

    lstrcpyW(pwszName, entry->name);

    *pdwNameLen = namesize + 1;
    *pdwBufferSize = entry->size;
    *pdwDataType   = entry->type;

    switch (entry->type)
    {
        case DPNA_DATATYPE_DWORD:
            *(DWORD*)pvBuffer = entry->data.value;
            break;
        case DPNA_DATATYPE_GUID:
             *(GUID*)pvBuffer = entry->data.guid;
            break;
        case DPNA_DATATYPE_STRING:
            memcpy(pvBuffer, entry->data.string, entry->size);
            break;
        case DPNA_DATATYPE_STRING_ANSI:
            memcpy(pvBuffer, entry->data.ansi, entry->size);
            break;
        case DPNA_DATATYPE_BINARY:
            memcpy(pvBuffer, entry->data.binary, entry->size);
            break;
    }

    return S_OK;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_AddComponent(IDirectPlay8Address *iface,
        const WCHAR *const pwszName, const void* const lpvData, const DWORD dwDataSize,
        const DWORD dwDataType)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    struct component *entry;
    BOOL found = FALSE;
    DWORD i;

    TRACE("(%p, %s, %p, %lu, %lx)\n", This, debugstr_w(pwszName), lpvData, dwDataSize, dwDataType);

    if (NULL == lpvData)
        return DPNERR_INVALIDPOINTER;

    switch (dwDataType)
    {
        case DPNA_DATATYPE_DWORD:
            if (sizeof(DWORD) != dwDataSize)
            {
                WARN("Invalid DWORD size, returning DPNERR_INVALIDPARAM\n");
                return DPNERR_INVALIDPARAM;
            }
            break;
        case DPNA_DATATYPE_GUID:
            if (sizeof(GUID) != dwDataSize)
            {
                WARN("Invalid GUID size, returning DPNERR_INVALIDPARAM\n");
                return DPNERR_INVALIDPARAM;
            }
            break;
        case DPNA_DATATYPE_STRING:
            if (((lstrlenW((WCHAR*)lpvData)+1)*sizeof(WCHAR)) != dwDataSize)
            {
                WARN("Invalid STRING size, returning DPNERR_INVALIDPARAM\n");
                return DPNERR_INVALIDPARAM;
            }
            break;
        case DPNA_DATATYPE_STRING_ANSI:
            if ((strlen((const CHAR*)lpvData)+1) != dwDataSize)
            {
                WARN("Invalid ANSI size, returning DPNERR_INVALIDPARAM\n");
                return DPNERR_INVALIDPARAM;
            }
            break;
    }

    for(i=0; i < This->comp_count; i++)
    {
        entry = This->components[i];

        if (lstrcmpW(pwszName, entry->name) == 0)
        {
            TRACE("Found %s\n", debugstr_w(pwszName));
            found = TRUE;

            if(entry->type == DPNA_DATATYPE_STRING_ANSI)
                free(entry->data.ansi);
            else if(entry->type == DPNA_DATATYPE_STRING)
                free(entry->data.string);
            else if(entry->type == DPNA_DATATYPE_BINARY)
                free(entry->data.binary);

            break;
        }
    }

    if(!found)
    {
        /* Create a new one */
        entry = malloc(sizeof(struct component));
        if(!entry)
            return E_OUTOFMEMORY;

        entry->name = wcsdup(pwszName);
        if(!entry->name)
        {
            free(entry);
            return E_OUTOFMEMORY;
        }

        if(!add_component(This, entry))
        {
           free(entry->name);
           free(entry);
           return E_OUTOFMEMORY;
        }
    }

    switch (dwDataType)
    {
        case DPNA_DATATYPE_DWORD:
            entry->data.value = *(DWORD*)lpvData;
            TRACE("(%p, %lu): DWORD Type -> %lu\n", lpvData, dwDataSize, *(const DWORD*) lpvData);
            break;
        case DPNA_DATATYPE_GUID:
            entry->data.guid = *(GUID*)lpvData;
            TRACE("(%p, %lu): GUID Type -> %s\n", lpvData, dwDataSize, debugstr_guid(lpvData));
            break;
        case DPNA_DATATYPE_STRING:
            entry->data.string = wcsdup((WCHAR*)lpvData);
            TRACE("(%p, %lu): STRING Type -> %s\n", lpvData, dwDataSize, debugstr_w((WCHAR*)lpvData));
            break;
        case DPNA_DATATYPE_STRING_ANSI:
            entry->data.ansi = strdup((char*)lpvData);
            TRACE("(%p, %lu): ANSI STRING Type -> %s\n", lpvData, dwDataSize, (const CHAR*) lpvData);
            break;
        case DPNA_DATATYPE_BINARY:
            entry->data.binary = malloc(dwDataSize);
            memcpy(entry->data.binary, lpvData, dwDataSize);
            TRACE("(%p, %lu): BINARY Type\n", lpvData, dwDataSize);
            break;
    }

    entry->type = dwDataType;
    entry->size = dwDataSize;

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetDevice(IDirectPlay8Address *iface, GUID *pDevGuid) {
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetDevice(IDirectPlay8Address *iface,
        const GUID *const devGuid)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %s): stub\n", This, debugstr_guid(devGuid));
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromDirectPlay4Address(IDirectPlay8Address *iface,
        void *pvAddress, DWORD dwDataSize)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static const IDirectPlay8AddressVtbl DirectPlay8Address_Vtbl =
{
    IDirectPlay8AddressImpl_QueryInterface,
    IDirectPlay8AddressImpl_AddRef,
    IDirectPlay8AddressImpl_Release,
    IDirectPlay8AddressImpl_BuildFromURLW,
    IDirectPlay8AddressImpl_BuildFromURLA,
    IDirectPlay8AddressImpl_Duplicate,
    IDirectPlay8AddressImpl_SetEqual,
    IDirectPlay8AddressImpl_IsEqual,
    IDirectPlay8AddressImpl_Clear,
    IDirectPlay8AddressImpl_GetURLW,
    IDirectPlay8AddressImpl_GetURLA,
    IDirectPlay8AddressImpl_GetSP,
    IDirectPlay8AddressImpl_GetUserData,
    IDirectPlay8AddressImpl_SetSP,
    IDirectPlay8AddressImpl_SetUserData,
    IDirectPlay8AddressImpl_GetNumComponents,
    IDirectPlay8AddressImpl_GetComponentByName,
    IDirectPlay8AddressImpl_GetComponentByIndex,
    IDirectPlay8AddressImpl_AddComponent,
    IDirectPlay8AddressImpl_GetDevice,
    IDirectPlay8AddressImpl_SetDevice,
    IDirectPlay8AddressImpl_BuildFromDirectPlay4Address
};

HRESULT DPNET_CreateDirectPlay8Address(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, LPVOID *ppobj)
{
    IDirectPlay8AddressImpl* client;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    client = calloc(1, sizeof(IDirectPlay8AddressImpl));
    if (!client)
        return E_OUTOFMEMORY;

    client->IDirectPlay8Address_iface.lpVtbl = &DirectPlay8Address_Vtbl;
    client->ref = 1;
    client->comp_array_size = 4;
    client->components = malloc(sizeof(*client->components) * client->comp_array_size);
    if(!client->components)
    {
        free(client);
        return E_OUTOFMEMORY;
    }

    ret = IDirectPlay8AddressImpl_QueryInterface(&client->IDirectPlay8Address_iface, riid, ppobj);
    IDirectPlay8AddressImpl_Release(&client->IDirectPlay8Address_iface);

    return ret;
}
