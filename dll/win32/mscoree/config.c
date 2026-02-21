/*
 * Configuration file parsing
 *
 * Copyright 2010 Vincent Povirk
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "ole2.h"
#include "msxml2.h"
#include "mscoree.h"
#include "corhdr.h"
#include "corerror.h"
#include "metahost.h"
#include "cordebug.h"
#include "wine/list.h"
#include "mscoree_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( mscoree );

enum parse_state
{
    STATE_ASSEMBLY_BINDING,
    STATE_ROOT,
    STATE_CONFIGURATION,
    STATE_PROBING,
    STATE_RUNTIME,
    STATE_STARTUP,
    STATE_UNKNOWN
};

typedef struct ConfigFileHandler
{
    ISAXContentHandler ISAXContentHandler_iface;
    ISAXErrorHandler ISAXErrorHandler_iface;
    LONG ref;
    enum parse_state states[16];
    int statenum;
    parsed_config_file *result;
} ConfigFileHandler;

typedef struct
{
    IStream IStream_iface;
    LONG ref;
    HANDLE file;
} ConfigStream;

static inline ConfigStream *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, ConfigStream, IStream_iface);
}

static HRESULT WINAPI ConfigStream_QueryInterface(IStream *iface, REFIID riid, void **ppv)
{
    ConfigStream *This = impl_from_IStream(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IStream))
        *ppv = &This->IStream_iface;
    else
    {
        WARN("Not supported iface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ConfigStream_AddRef(IStream *iface)
{
    ConfigStream *This = impl_from_IStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    return ref;
}

static ULONG WINAPI ConfigStream_Release(IStream *iface)
{
    ConfigStream *This = impl_from_IStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lu\n",This, ref);

    if (!ref)
    {
        CloseHandle(This->file);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI ConfigStream_Read(IStream *iface, void *buf, ULONG size, ULONG *ret_read)
{
    ConfigStream *This = impl_from_IStream(iface);
    DWORD read = 0;

    TRACE("(%p)->(%p %lu %p)\n", This, buf, size, ret_read);

    if (!ReadFile(This->file, buf, size, &read, NULL))
    {
        WARN("error %ld reading file\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (ret_read) *ret_read = read;
    return S_OK;
}

static HRESULT WINAPI ConfigStream_Write(IStream *iface, const void *buf, ULONG size, ULONG *written)
{
    ConfigStream *This = impl_from_IStream(iface);
    TRACE("(%p)->(%p %lu %p)\n", This, buf, size, written);
    return E_FAIL;
}

static HRESULT WINAPI ConfigStream_Seek(IStream *iface, LARGE_INTEGER dlibMove,
                                        DWORD dwOrigin, ULARGE_INTEGER *pNewPos)
{
    ConfigStream *This = impl_from_IStream(iface);
    TRACE("(%p)->(%ld %ld %p)\n", This, dlibMove.u.LowPart, dwOrigin, pNewPos);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigStream_SetSize(IStream *iface, ULARGE_INTEGER libNewSize)
{
    ConfigStream *This = impl_from_IStream(iface);
    TRACE("(%p)->(%ld)\n", This, libNewSize.u.LowPart);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigStream_CopyTo(IStream *iface, IStream *stream, ULARGE_INTEGER size,
                                          ULARGE_INTEGER *read, ULARGE_INTEGER *written)
{
    ConfigStream *This = impl_from_IStream(iface);
    FIXME("(%p)->(%p %ld %p %p)\n", This, stream, size.u.LowPart, read, written);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigStream_Commit(IStream *iface, DWORD flags)
{
    ConfigStream *This = impl_from_IStream(iface);
    FIXME("(%p,%ld)\n", This, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigStream_Revert(IStream *iface)
{
    ConfigStream *This = impl_from_IStream(iface);
    TRACE("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigStream_LockUnlockRegion(IStream *iface, ULARGE_INTEGER libOffset,
                                                    ULARGE_INTEGER cb, DWORD dwLockType)
{
    ConfigStream *This = impl_from_IStream(iface);
    TRACE("(%p,%ld,%ld,%ld)\n", This, libOffset.u.LowPart, cb.u.LowPart, dwLockType);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigStream_Stat(IStream *iface, STATSTG *lpStat, DWORD grfStatFlag)
{
    ConfigStream *This = impl_from_IStream(iface);
    FIXME("(%p,%p,%ld)\n", This, lpStat, grfStatFlag);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConfigStream_Clone(IStream *iface, IStream **ppstm)
{
    ConfigStream *This = impl_from_IStream(iface);
    TRACE("(%p)\n",This);
    return E_NOTIMPL;
}

static const IStreamVtbl ConfigStreamVtbl = {
  ConfigStream_QueryInterface,
  ConfigStream_AddRef,
  ConfigStream_Release,
  ConfigStream_Read,
  ConfigStream_Write,
  ConfigStream_Seek,
  ConfigStream_SetSize,
  ConfigStream_CopyTo,
  ConfigStream_Commit,
  ConfigStream_Revert,
  ConfigStream_LockUnlockRegion,
  ConfigStream_LockUnlockRegion,
  ConfigStream_Stat,
  ConfigStream_Clone
};

HRESULT WINAPI CreateConfigStream(const WCHAR *filename, IStream **stream)
{
    ConfigStream *config_stream;
    HANDLE file;

    TRACE("(%s, %p)\n", debugstr_w(filename), stream);

    if (!stream)
        return COR_E_NULLREFERENCE;

    file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    if (file == INVALID_HANDLE_VALUE)
        return GetLastError() == ERROR_FILE_NOT_FOUND ? COR_E_FILENOTFOUND : E_FAIL;

    config_stream = malloc(sizeof(*config_stream));
    if (!config_stream)
    {
        CloseHandle(file);
        return E_OUTOFMEMORY;
    }

    config_stream->IStream_iface.lpVtbl = &ConfigStreamVtbl;
    config_stream->ref = 1;
    config_stream->file = file;

    *stream = &config_stream->IStream_iface;
    return S_OK;
}

static inline ConfigFileHandler *impl_from_ISAXContentHandler(ISAXContentHandler *iface)
{
    return CONTAINING_RECORD(iface, ConfigFileHandler, ISAXContentHandler_iface);
}

static inline ConfigFileHandler *impl_from_ISAXErrorHandler(ISAXErrorHandler *iface)
{
    return CONTAINING_RECORD(iface, ConfigFileHandler, ISAXErrorHandler_iface);
}

static HRESULT WINAPI ConfigFileHandler_QueryInterface(ISAXContentHandler *iface,
    REFIID riid, void **ppvObject)
{
    ConfigFileHandler *This = impl_from_ISAXContentHandler(iface);

    if (IsEqualGUID(riid, &IID_ISAXContentHandler) || IsEqualGUID(riid, &IID_IUnknown))
        *ppvObject = &This->ISAXContentHandler_iface;
    else if (IsEqualGUID(riid, &IID_ISAXErrorHandler))
        *ppvObject = &This->ISAXErrorHandler_iface;
    else
    {
        WARN("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppvObject);

    return S_OK;
}

static ULONG WINAPI ConfigFileHandler_AddRef(ISAXContentHandler *iface)
{
    ConfigFileHandler *This = impl_from_ISAXContentHandler(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ConfigFileHandler_Release(ISAXContentHandler *iface)
{
    ConfigFileHandler *This = impl_from_ISAXContentHandler(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        free(This);

    return ref;
}

static HRESULT WINAPI ConfigFileHandler_putDocumentLocator(ISAXContentHandler *iface,
    ISAXLocator *pLocator)
{
    return S_OK;
}

static HRESULT WINAPI ConfigFileHandler_startDocument(ISAXContentHandler *iface)
{
    return S_OK;
}

static HRESULT WINAPI ConfigFileHandler_endDocument(ISAXContentHandler *iface)
{
    return S_OK;
}

static HRESULT WINAPI ConfigFileHandler_startPrefixMapping(ISAXContentHandler *iface,
    const WCHAR *pPrefix, int nPrefix, const WCHAR *pUri, int nUri)
{
    return S_OK;
}

static HRESULT WINAPI ConfigFileHandler_endPrefixMapping(ISAXContentHandler *iface,
    const WCHAR *pPrefix, int nPrefix)
{
    return S_OK;
}

static HRESULT parse_startup(ConfigFileHandler *This, ISAXAttributes *pAttr)
{
    static const WCHAR legacy[] = {'u','s','e','L','e','g','a','c','y','V','2','R','u','n','t','i','m','e','A','c','t','i','v','a','t','i','o','n','P','o','l','i','c','y',0};
    static const WCHAR empty[] = {0};
    LPCWSTR value;
    int value_size;
    HRESULT hr;

    hr = ISAXAttributes_getValueFromName(pAttr, empty, 0, legacy, lstrlenW(legacy), &value, &value_size);
    if (SUCCEEDED(hr))
        FIXME("useLegacyV2RuntimeActivationPolicy=%s not implemented\n", debugstr_wn(value, value_size));
    hr = S_OK;

    return hr;
}

static HRESULT parse_probing(ConfigFileHandler *This, ISAXAttributes *pAttr)
{
    static const WCHAR privatePath[] = {'p','r','i','v','a','t','e','P','a','t','h',0};
    static const WCHAR empty[] = {0};
    LPCWSTR value;
    int value_size;
    HRESULT hr;

    hr = ISAXAttributes_getValueFromName(pAttr, empty, 0, privatePath, lstrlenW(privatePath), &value, &value_size);
    if (SUCCEEDED(hr))
    {
        TRACE("%s\n", debugstr_wn(value, value_size));

        This->result->private_path = wcsdup(value);
        if (!This->result->private_path)
            hr = E_OUTOFMEMORY;
    }

    return hr;
}


static HRESULT parse_supported_runtime(ConfigFileHandler *This, ISAXAttributes *pAttr)
{
    static const WCHAR version[] = {'v','e','r','s','i','o','n',0};
    static const WCHAR sku[] = {'s','k','u',0};
    static const WCHAR empty[] = {0};
    LPCWSTR value;
    int value_size;
    HRESULT hr;
    supported_runtime *entry;

    hr = ISAXAttributes_getValueFromName(pAttr, empty, 0, version, lstrlenW(version), &value, &value_size);
    if (SUCCEEDED(hr))
    {
        TRACE("%s\n", debugstr_wn(value, value_size));
        entry = malloc(sizeof(supported_runtime));
        if (entry)
        {
            entry->version = wcsdup(value);
            if (entry->version)
            {
                list_add_tail(&This->result->supported_runtimes, &entry->entry);
            }
            else
            {
                free(entry);
                hr = E_OUTOFMEMORY;
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }
    else
        WARN("Missing version attribute\n");

    if (SUCCEEDED(hr))
    {
        hr = ISAXAttributes_getValueFromName(pAttr, empty, 0, sku, lstrlenW(sku), &value, &value_size);
        if (SUCCEEDED(hr))
            FIXME("sku=%s not implemented\n", debugstr_wn(value, value_size));
        hr = S_OK;
    }

    return hr;
}

static HRESULT WINAPI ConfigFileHandler_startElement(ISAXContentHandler *iface,
    const WCHAR *pNamespaceUri, int nNamespaceUri, const WCHAR *pLocalName,
    int nLocalName, const WCHAR *pQName, int nQName, ISAXAttributes *pAttr)
{
    ConfigFileHandler *This = impl_from_ISAXContentHandler(iface);
    static const WCHAR configuration[] = {'c','o','n','f','i','g','u','r','a','t','i','o','n',0};
    static const WCHAR assemblyBinding[] = {'a','s','s','e','m','b','l','y','B','i','n','d','i','n','g',0};
    static const WCHAR probing[] = {'p','r','o','b','i','n','g',0};
    static const WCHAR runtime[] = {'r','u','n','t','i','m','e',0};
    static const WCHAR startup[] = {'s','t','a','r','t','u','p',0};
    static const WCHAR supportedRuntime[] = {'s','u','p','p','o','r','t','e','d','R','u','n','t','i','m','e',0};

    HRESULT hr = S_OK;

    TRACE("%s %s %s\n", debugstr_wn(pNamespaceUri,nNamespaceUri),
        debugstr_wn(pLocalName,nLocalName), debugstr_wn(pQName,nQName));

    if (This->statenum == ARRAY_SIZE(This->states) - 1)
    {
        ERR("file has too much nesting\n");
        return E_FAIL;
    }

    switch (This->states[This->statenum])
    {
    case STATE_ROOT:
        if (nLocalName == ARRAY_SIZE(configuration) - 1 && wcscmp(pLocalName, configuration) == 0)
        {
            This->states[++This->statenum] = STATE_CONFIGURATION;
            break;
        }
        else
            goto unknown;
    case STATE_CONFIGURATION:
        if (nLocalName == ARRAY_SIZE(startup) - 1 && wcscmp(pLocalName, startup) == 0)
        {
            hr = parse_startup(This, pAttr);
            This->states[++This->statenum] = STATE_STARTUP;
            break;
        }
        else if (nLocalName == ARRAY_SIZE(runtime) - 1 && wcscmp(pLocalName, runtime) == 0)
        {
            This->states[++This->statenum] = STATE_RUNTIME;
            break;
        }
        else
            goto unknown;
    case STATE_RUNTIME:
        if (nLocalName == ARRAY_SIZE(assemblyBinding) - 1 &&
            wcscmp(pLocalName, assemblyBinding) == 0)
        {
            This->states[++This->statenum] = STATE_ASSEMBLY_BINDING;
            break;
        }
        else
            goto unknown;
    case STATE_ASSEMBLY_BINDING:
        if (nLocalName == ARRAY_SIZE(probing) - 1 && wcscmp(pLocalName, probing) == 0)
        {
            hr = parse_probing(This, pAttr);
            This->states[++This->statenum] = STATE_PROBING;
            break;
        }
        else
            goto unknown;
    case STATE_STARTUP:
        if (nLocalName == ARRAY_SIZE(supportedRuntime) - 1 &&
            wcscmp(pLocalName, supportedRuntime) == 0)
        {
            hr = parse_supported_runtime(This, pAttr);
            This->states[++This->statenum] = STATE_UNKNOWN;
            break;
        }
        else
            goto unknown;
    default:
        goto unknown;
    }

    return hr;

unknown:
    TRACE("Unknown element %s in state %u\n", debugstr_wn(pLocalName,nLocalName),
        This->states[This->statenum]);

    This->states[++This->statenum] = STATE_UNKNOWN;

    return S_OK;
}

static HRESULT WINAPI ConfigFileHandler_endElement(ISAXContentHandler *iface,
    const WCHAR *pNamespaceUri, int nNamespaceUri, const WCHAR *pLocalName,
    int nLocalName, const WCHAR *pQName, int nQName)
{
    ConfigFileHandler *This = impl_from_ISAXContentHandler(iface);

    TRACE("%s %s %s\n", debugstr_wn(pNamespaceUri,nNamespaceUri),
        debugstr_wn(pLocalName,nLocalName), debugstr_wn(pQName,nQName));

    if (This->statenum > 0)
    {
        This->statenum--;
    }
    else
    {
        ERR("element end does not match a start\n");
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI ConfigFileHandler_characters(ISAXContentHandler *iface,
    const WCHAR *pChars, int nChars)
{
    TRACE("%s\n", debugstr_wn(pChars,nChars));

    return S_OK;
}

static HRESULT WINAPI ConfigFileHandler_ignorableWhitespace(ISAXContentHandler *iface,
    const WCHAR *pChars, int nChars)
{
    return S_OK;
}

static HRESULT WINAPI ConfigFileHandler_processingInstruction(ISAXContentHandler *iface,
    const WCHAR *pTarget, int nTarget, const WCHAR *pData, int nData)
{
    return S_OK;
}

static HRESULT WINAPI ConfigFileHandler_skippedEntity(ISAXContentHandler *iface,
    const WCHAR * pName, int nName)
{
    TRACE("%s\n", debugstr_wn(pName,nName));
    return S_OK;
}

static const struct ISAXContentHandlerVtbl ConfigFileHandlerVtbl =
{
    ConfigFileHandler_QueryInterface,
    ConfigFileHandler_AddRef,
    ConfigFileHandler_Release,
    ConfigFileHandler_putDocumentLocator,
    ConfigFileHandler_startDocument,
    ConfigFileHandler_endDocument,
    ConfigFileHandler_startPrefixMapping,
    ConfigFileHandler_endPrefixMapping,
    ConfigFileHandler_startElement,
    ConfigFileHandler_endElement,
    ConfigFileHandler_characters,
    ConfigFileHandler_ignorableWhitespace,
    ConfigFileHandler_processingInstruction,
    ConfigFileHandler_skippedEntity
};

static HRESULT WINAPI ConfigFileHandler_Error_QueryInterface(ISAXErrorHandler *iface,
    REFIID riid, void **ppvObject)
{
    ConfigFileHandler *This = impl_from_ISAXErrorHandler(iface);
    return ISAXContentHandler_QueryInterface(&This->ISAXContentHandler_iface, riid, ppvObject);
}

static ULONG WINAPI ConfigFileHandler_Error_AddRef(ISAXErrorHandler *iface)
{
    ConfigFileHandler *This = impl_from_ISAXErrorHandler(iface);
    return ISAXContentHandler_AddRef(&This->ISAXContentHandler_iface);
}

static ULONG WINAPI ConfigFileHandler_Error_Release(ISAXErrorHandler *iface)
{
    ConfigFileHandler *This = impl_from_ISAXErrorHandler(iface);
    return ISAXContentHandler_Release(&This->ISAXContentHandler_iface);
}

static HRESULT WINAPI ConfigFileHandler_error(ISAXErrorHandler *iface,
    ISAXLocator * pLocator, const WCHAR * pErrorMessage, HRESULT hrErrorCode)
{
    WARN("%s,%lx\n", debugstr_w(pErrorMessage), hrErrorCode);
    return S_OK;
}

static HRESULT WINAPI ConfigFileHandler_fatalError(ISAXErrorHandler *iface,
    ISAXLocator * pLocator, const WCHAR * pErrorMessage, HRESULT hrErrorCode)
{
    WARN("%s,%lx\n", debugstr_w(pErrorMessage), hrErrorCode);
    return S_OK;
}

static HRESULT WINAPI ConfigFileHandler_ignorableWarning(ISAXErrorHandler *iface,
    ISAXLocator * pLocator, const WCHAR * pErrorMessage, HRESULT hrErrorCode)
{
    WARN("%s,%lx\n", debugstr_w(pErrorMessage), hrErrorCode);
    return S_OK;
}

static const struct ISAXErrorHandlerVtbl ConfigFileHandlerErrorVtbl =
{
    ConfigFileHandler_Error_QueryInterface,
    ConfigFileHandler_Error_AddRef,
    ConfigFileHandler_Error_Release,
    ConfigFileHandler_error,
    ConfigFileHandler_fatalError,
    ConfigFileHandler_ignorableWarning
};

static void init_config(parsed_config_file *config)
{
    list_init(&config->supported_runtimes);
    config->private_path = NULL;
}

static HRESULT parse_config(VARIANT input, parsed_config_file *result)
{
    ISAXXMLReader *reader;
    ConfigFileHandler *handler;
    HRESULT hr;

    handler = malloc(sizeof(ConfigFileHandler));
    if (!handler)
        return E_OUTOFMEMORY;

    handler->ISAXContentHandler_iface.lpVtbl = &ConfigFileHandlerVtbl;
    handler->ISAXErrorHandler_iface.lpVtbl = &ConfigFileHandlerErrorVtbl;
    handler->ref = 1;
    handler->states[0] = STATE_ROOT;
    handler->statenum = 0;
    handler->result = result;

    hr = CoCreateInstance(&CLSID_SAXXMLReader, NULL, CLSCTX_INPROC_SERVER,
        &IID_ISAXXMLReader, (LPVOID*)&reader);

    if (SUCCEEDED(hr))
    {
        hr = ISAXXMLReader_putContentHandler(reader, &handler->ISAXContentHandler_iface);

        if (SUCCEEDED(hr))
            hr = ISAXXMLReader_putErrorHandler(reader, &handler->ISAXErrorHandler_iface);

        if (SUCCEEDED(hr))
            hr = ISAXXMLReader_parse(reader, input);

        ISAXXMLReader_Release(reader);
    }

    ISAXContentHandler_Release(&handler->ISAXContentHandler_iface);

    return S_OK;
}

HRESULT parse_config_stream(IStream *stream, parsed_config_file *result)
{
    VARIANT var;
    HRESULT hr;
    HRESULT initresult;

    init_config(result);

    initresult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = (IUnknown*)stream;

    hr = parse_config(var, result);

    if (SUCCEEDED(initresult))
        CoUninitialize();

    return hr;
}

HRESULT parse_config_file(LPCWSTR filename, parsed_config_file *result)
{
    HRESULT hr;
    IStream *stream;

    init_config(result);

    hr = CreateConfigStream(filename, &stream);
    if (FAILED(hr))
        return hr;

    hr = parse_config_stream(stream, result);

    IStream_Release(stream);

    return hr;
}

void free_parsed_config_file(parsed_config_file *file)
{
    supported_runtime *cursor, *cursor2;

    LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, &file->supported_runtimes, supported_runtime, entry)
    {
        free(cursor->version);
        list_remove(&cursor->entry);
        free(cursor);
    }

    free(file->private_path);
}
