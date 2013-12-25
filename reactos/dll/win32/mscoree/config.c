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

#include "mscoree_private.h"

#include <ole2.h>
#include <shlwapi.h>
#include <initguid.h>
#include <msxml2.h>

enum parse_state
{
    STATE_ROOT,
    STATE_CONFIGURATION,
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
    if (IsEqualGUID(riid, &IID_ISAXContentHandler) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObject = iface;
    }
    else
    {
        WARN("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ISAXContentHandler_AddRef(iface);

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
        HeapFree(GetProcessHeap(), 0, This);

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
        entry = HeapAlloc(GetProcessHeap(), 0, sizeof(supported_runtime));
        if (entry)
        {
            entry->version = HeapAlloc(GetProcessHeap(), 0, (value_size + 1) * sizeof(WCHAR));
            if (entry->version)
            {
                lstrcpyW(entry->version, value);
                list_add_tail(&This->result->supported_runtimes, &entry->entry);
            }
            else
            {
                HeapFree(GetProcessHeap(), 0, entry);
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
    static const WCHAR startup[] = {'s','t','a','r','t','u','p',0};
    static const WCHAR supportedRuntime[] = {'s','u','p','p','o','r','t','e','d','R','u','n','t','i','m','e',0};
    HRESULT hr = S_OK;

    TRACE("%s %s %s\n", debugstr_wn(pNamespaceUri,nNamespaceUri),
        debugstr_wn(pLocalName,nLocalName), debugstr_wn(pQName,nQName));

    if (This->statenum == sizeof(This->states) / sizeof(This->states[0]) - 1)
    {
        ERR("file has too much nesting\n");
        return E_FAIL;
    }

    switch (This->states[This->statenum])
    {
    case STATE_ROOT:
        if (nLocalName == sizeof(configuration)/sizeof(WCHAR)-1 &&
            lstrcmpW(pLocalName, configuration) == 0)
        {
            This->states[++This->statenum] = STATE_CONFIGURATION;
            break;
        }
        else
            goto unknown;
    case STATE_CONFIGURATION:
        if (nLocalName == sizeof(startup)/sizeof(WCHAR)-1 &&
            lstrcmpW(pLocalName, startup) == 0)
        {
            hr = parse_startup(This, pAttr);
            This->states[++This->statenum] = STATE_STARTUP;
            break;
        }
        else
            goto unknown;
    case STATE_STARTUP:
        if (nLocalName == sizeof(supportedRuntime)/sizeof(WCHAR)-1 &&
            lstrcmpW(pLocalName, supportedRuntime) == 0)
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
    FIXME("Unknown element %s in state %u\n", debugstr_wn(pLocalName,nLocalName),
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
    if (IsEqualGUID(riid, &IID_ISAXErrorHandler) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObject = iface;
    }
    else
    {
        WARN("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ISAXErrorHandler_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI ConfigFileHandler_Error_AddRef(ISAXErrorHandler *iface)
{
    ConfigFileHandler *This = impl_from_ISAXErrorHandler(iface);
    return IUnknown_AddRef((IUnknown*)This);
}

static ULONG WINAPI ConfigFileHandler_Error_Release(ISAXErrorHandler *iface)
{
    ConfigFileHandler *This = impl_from_ISAXErrorHandler(iface);
    return IUnknown_Release((IUnknown*)This);
}

static HRESULT WINAPI ConfigFileHandler_error(ISAXErrorHandler *iface,
    ISAXLocator * pLocator, const WCHAR * pErrorMessage, HRESULT hrErrorCode)
{
    WARN("%s,%x\n", debugstr_w(pErrorMessage), hrErrorCode);
    return S_OK;
}

static HRESULT WINAPI ConfigFileHandler_fatalError(ISAXErrorHandler *iface,
    ISAXLocator * pLocator, const WCHAR * pErrorMessage, HRESULT hrErrorCode)
{
    WARN("%s,%x\n", debugstr_w(pErrorMessage), hrErrorCode);
    return S_OK;
}

static HRESULT WINAPI ConfigFileHandler_ignorableWarning(ISAXErrorHandler *iface,
    ISAXLocator * pLocator, const WCHAR * pErrorMessage, HRESULT hrErrorCode)
{
    WARN("%s,%x\n", debugstr_w(pErrorMessage), hrErrorCode);
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
}

static HRESULT parse_config(VARIANT input, parsed_config_file *result)
{
    ISAXXMLReader *reader;
    ConfigFileHandler *handler;
    HRESULT hr;

    handler = HeapAlloc(GetProcessHeap(), 0, sizeof(ConfigFileHandler));
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

HRESULT parse_config_file(LPCWSTR filename, parsed_config_file *result)
{
    IStream *stream;
    VARIANT var;
    HRESULT hr;
    HRESULT initresult;

    init_config(result);

    initresult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = SHCreateStreamOnFileW(filename, STGM_SHARE_DENY_WRITE | STGM_READ | STGM_FAILIFTHERE, &stream);

    if (SUCCEEDED(hr))
    {
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown*)stream;

        hr = parse_config(var, result);

        IStream_Release(stream);
    }

    if (SUCCEEDED(initresult))
        CoUninitialize();

    return hr;
}

void free_parsed_config_file(parsed_config_file *file)
{
    supported_runtime *cursor, *cursor2;

    LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, &file->supported_runtimes, supported_runtime, entry)
    {
        HeapFree(GetProcessHeap(), 0, cursor->version);
        list_remove(&cursor->entry);
        HeapFree(GetProcessHeap(), 0, cursor);
    }
}
