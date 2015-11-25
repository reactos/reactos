/*
 * File Source Filter
 *
 * Copyright 2003 Robert Shearman
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

#include "quartz_private.h"

static const WCHAR wszOutputPinName[] = { 'O','u','t','p','u','t',0 };

typedef struct AsyncReader
{
    BaseFilter filter;
    IFileSourceFilter IFileSourceFilter_iface;
    IAMFilterMiscFlags IAMFilterMiscFlags_iface;

    IPin * pOutputPin;
    LPOLESTR pszFileName;
    AM_MEDIA_TYPE * pmt;
} AsyncReader;

static inline AsyncReader *impl_from_BaseFilter(BaseFilter *iface)
{
    return CONTAINING_RECORD(iface, AsyncReader, filter);
}

static inline AsyncReader *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, AsyncReader, filter.IBaseFilter_iface);
}

static inline AsyncReader *impl_from_IFileSourceFilter(IFileSourceFilter *iface)
{
    return CONTAINING_RECORD(iface, AsyncReader, IFileSourceFilter_iface);
}

static inline AsyncReader *impl_from_IAMFilterMiscFlags(IAMFilterMiscFlags *iface)
{
    return CONTAINING_RECORD(iface, AsyncReader, IAMFilterMiscFlags_iface);
}

static const IBaseFilterVtbl AsyncReader_Vtbl;
static const IFileSourceFilterVtbl FileSource_Vtbl;
static const IAsyncReaderVtbl FileAsyncReader_Vtbl;
static const IAMFilterMiscFlagsVtbl IAMFilterMiscFlags_Vtbl;

static HRESULT FileAsyncReader_Construct(HANDLE hFile, IBaseFilter * pBaseFilter, LPCRITICAL_SECTION pCritSec, IPin ** ppPin);

static const WCHAR mediatype_name[] = {
    'M', 'e', 'd', 'i', 'a', ' ', 'T', 'y', 'p', 'e', 0 };
static const WCHAR subtype_name[] = {
    'S', 'u', 'b', 't', 'y', 'p', 'e', 0 };
static const WCHAR source_filter_name[] = {
    'S','o','u','r','c','e',' ','F','i','l','t','e','r',0};

static HRESULT process_extensions(HKEY hkeyExtensions, LPCOLESTR pszFileName, GUID * majorType, GUID * minorType, GUID * sourceFilter)
{
    WCHAR *extension;
    LONG l;
    HKEY hsub;
    WCHAR keying[39];
    DWORD size;

    if (!pszFileName)
        return E_POINTER;

    /* Get the part of the name that matters */
    extension = PathFindExtensionW(pszFileName);
    if (*extension != '.')
        return E_FAIL;

    l = RegOpenKeyExW(hkeyExtensions, extension, 0, KEY_READ, &hsub);
    if (l)
        return E_FAIL;

    if (majorType)
    {
        size = sizeof(keying);
        l = RegQueryValueExW(hsub, mediatype_name, NULL, NULL, (LPBYTE)keying, &size);
        if (!l)
            CLSIDFromString(keying, majorType);
    }

    if (minorType)
    {
        size = sizeof(keying);
        if (!l)
            l = RegQueryValueExW(hsub, subtype_name, NULL, NULL, (LPBYTE)keying, &size);
        if (!l)
            CLSIDFromString(keying, minorType);
    }

    if (sourceFilter)
    {
        size = sizeof(keying);
        if (!l)
            l = RegQueryValueExW(hsub, source_filter_name, NULL, NULL, (LPBYTE)keying, &size);
        if (!l)
            CLSIDFromString(keying, sourceFilter);
    }

    RegCloseKey(hsub);

    if (!l)
        return S_OK;
    return E_FAIL;
}

static unsigned char byte_from_hex_char(WCHAR wHex)
{
    switch (tolowerW(wHex))
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return (wHex - '0') & 0xf;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
        return (wHex - 'a' + 10) & 0xf;
    default:
        return 0;
    }
}

static HRESULT process_pattern_string(LPCWSTR wszPatternString, IAsyncReader * pReader)
{
    ULONG ulOffset;
    ULONG ulBytes;
    BYTE * pbMask;
    BYTE * pbValue;
    BYTE * pbFile;
    HRESULT hr = S_OK;
    ULONG strpos;

    TRACE("\t\tPattern string: %s\n", debugstr_w(wszPatternString));
    
    /* format: "offset, bytestocompare, mask, value" */

    ulOffset = strtolW(wszPatternString, NULL, 10);

    if (!(wszPatternString = strchrW(wszPatternString, ',')))
        return E_INVALIDARG;

    wszPatternString++; /* skip ',' */

    ulBytes = strtolW(wszPatternString, NULL, 10);

    pbMask = HeapAlloc(GetProcessHeap(), 0, ulBytes);
    pbValue = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulBytes);
    pbFile = HeapAlloc(GetProcessHeap(), 0, ulBytes);

    /* default mask is match everything */
    memset(pbMask, 0xFF, ulBytes);

    if (!(wszPatternString = strchrW(wszPatternString, ',')))
        hr = E_INVALIDARG;

    if (hr == S_OK)
    {
        wszPatternString++; /* skip ',' */
        while (!isxdigitW(*wszPatternString) && (*wszPatternString != ',')) wszPatternString++;

        for (strpos = 0; isxdigitW(*wszPatternString) && (strpos/2 < ulBytes); wszPatternString++, strpos++)
        {
            if ((strpos % 2) == 1) /* odd numbered position */
                pbMask[strpos / 2] |= byte_from_hex_char(*wszPatternString);
            else
                pbMask[strpos / 2] = byte_from_hex_char(*wszPatternString) << 4;
        }

        if (!(wszPatternString = strchrW(wszPatternString, ',')))
            hr = E_INVALIDARG;
        else
            wszPatternString++; /* skip ',' */
    }

    if (hr == S_OK)
    {
        for ( ; !isxdigitW(*wszPatternString) && (*wszPatternString != ','); wszPatternString++)
            ;

        for (strpos = 0; isxdigitW(*wszPatternString) && (strpos/2 < ulBytes); wszPatternString++, strpos++)
        {
            if ((strpos % 2) == 1) /* odd numbered position */
                pbValue[strpos / 2] |= byte_from_hex_char(*wszPatternString);
            else
                pbValue[strpos / 2] = byte_from_hex_char(*wszPatternString) << 4;
        }
    }

    if (hr == S_OK)
        hr = IAsyncReader_SyncRead(pReader, ulOffset, ulBytes, pbFile);

    if (hr == S_OK)
    {
        ULONG i;
        for (i = 0; i < ulBytes; i++)
            if ((pbFile[i] & pbMask[i]) != pbValue[i])
            {
                hr = S_FALSE;
                break;
            }
    }

    HeapFree(GetProcessHeap(), 0, pbMask);
    HeapFree(GetProcessHeap(), 0, pbValue);
    HeapFree(GetProcessHeap(), 0, pbFile);

    /* if we encountered no errors with this string, and there is a following tuple, then we
     * have to match that as well to succeed */
    if ((hr == S_OK) && (wszPatternString = strchrW(wszPatternString, ',')))
        return process_pattern_string(wszPatternString + 1, pReader);
    else
        return hr;
}

HRESULT GetClassMediaFile(IAsyncReader * pReader, LPCOLESTR pszFileName, GUID * majorType, GUID * minorType, GUID * sourceFilter)
{
    HKEY hkeyMediaType = NULL;
    LONG lRet;
    HRESULT hr = S_OK;
    BOOL bFound = FALSE;
    static const WCHAR wszMediaType[] = {'M','e','d','i','a',' ','T','y','p','e',0};

    TRACE("(%p, %s, %p, %p)\n", pReader, debugstr_w(pszFileName), majorType, minorType);

    if(majorType)
        *majorType = GUID_NULL;
    if(minorType)
        *minorType = GUID_NULL;
    if(sourceFilter)
        *sourceFilter = GUID_NULL;

    lRet = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszMediaType, 0, KEY_READ, &hkeyMediaType);
    hr = HRESULT_FROM_WIN32(lRet);

    if (SUCCEEDED(hr))
    {
        DWORD indexMajor;

        for (indexMajor = 0; !bFound; indexMajor++)
        {
            HKEY hkeyMajor;
            WCHAR wszMajorKeyName[CHARS_IN_GUID];
            DWORD dwKeyNameLength = sizeof(wszMajorKeyName) / sizeof(wszMajorKeyName[0]);
            static const WCHAR wszExtensions[] = {'E','x','t','e','n','s','i','o','n','s',0};

            if (RegEnumKeyExW(hkeyMediaType, indexMajor, wszMajorKeyName, &dwKeyNameLength, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
                break;
            if (RegOpenKeyExW(hkeyMediaType, wszMajorKeyName, 0, KEY_READ, &hkeyMajor) != ERROR_SUCCESS)
                break;
            TRACE("%s\n", debugstr_w(wszMajorKeyName));
            if (!strcmpW(wszExtensions, wszMajorKeyName))
            {
                if (process_extensions(hkeyMajor, pszFileName, majorType, minorType, sourceFilter) == S_OK)
                    bFound = TRUE;
            }
            /* We need a reader interface to check bytes */
            else if (pReader)
            {
                DWORD indexMinor;

                for (indexMinor = 0; !bFound; indexMinor++)
                {
                    HKEY hkeyMinor;
                    WCHAR wszMinorKeyName[CHARS_IN_GUID];
                    DWORD dwMinorKeyNameLen = sizeof(wszMinorKeyName) / sizeof(wszMinorKeyName[0]);
                    WCHAR wszSourceFilterKeyName[CHARS_IN_GUID];
                    DWORD dwSourceFilterKeyNameLen = sizeof(wszSourceFilterKeyName);
                    DWORD maxValueLen;
                    DWORD indexValue;

                    if (RegEnumKeyExW(hkeyMajor, indexMinor, wszMinorKeyName, &dwMinorKeyNameLen, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
                        break;

                    if (RegOpenKeyExW(hkeyMajor, wszMinorKeyName, 0, KEY_READ, &hkeyMinor) != ERROR_SUCCESS)
                        break;

                    TRACE("\t%s\n", debugstr_w(wszMinorKeyName));
        
                    if (RegQueryInfoKeyW(hkeyMinor, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &maxValueLen, NULL, NULL) != ERROR_SUCCESS)
                        break;

                    for (indexValue = 0; !bFound; indexValue++)
                    {
                        DWORD dwType;
                        WCHAR wszValueName[14]; /* longest name we should encounter will be "Source Filter" */
                        LPWSTR wszPatternString = HeapAlloc(GetProcessHeap(), 0, maxValueLen);
                        DWORD dwValueNameLen = sizeof(wszValueName) / sizeof(wszValueName[0]); /* remember this is in chars */
                        DWORD dwDataLen = maxValueLen; /* remember this is in bytes */

                        if (RegEnumValueW(hkeyMinor, indexValue, wszValueName, &dwValueNameLen, NULL, &dwType, (LPBYTE)wszPatternString, &dwDataLen) != ERROR_SUCCESS)
                        {
                            HeapFree(GetProcessHeap(), 0, wszPatternString);
                            break;
                        }

                        if (strcmpW(wszValueName, source_filter_name)==0) {
                            HeapFree(GetProcessHeap(), 0, wszPatternString);
                            continue;
                        }

                        /* if it is not the source filter value */
                        if (process_pattern_string(wszPatternString, pReader) == S_OK)
                        {
                            HeapFree(GetProcessHeap(), 0, wszPatternString);
                            if (majorType && FAILED(CLSIDFromString(wszMajorKeyName, majorType)))
                                break;
                            if (minorType && FAILED(CLSIDFromString(wszMinorKeyName, minorType)))
                                break;
                            if (sourceFilter)
                            {
                                /* Look up the source filter key */
                                if (RegQueryValueExW(hkeyMinor, source_filter_name, NULL, NULL, (LPBYTE)wszSourceFilterKeyName, &dwSourceFilterKeyNameLen))
                                    break;
                                if (FAILED(CLSIDFromString(wszSourceFilterKeyName, sourceFilter)))
                                    break;
                            }
                            bFound = TRUE;
                        } else
                            HeapFree(GetProcessHeap(), 0, wszPatternString);
                    }
                    CloseHandle(hkeyMinor);
                }
            }
            CloseHandle(hkeyMajor);
        }
    }
    CloseHandle(hkeyMediaType);

    if (SUCCEEDED(hr) && !bFound)
    {
        ERR("Media class not found\n");
        hr = E_FAIL;
    }
    else if (bFound)
    {
        TRACE("Found file's class:\n");
	if(majorType)
		TRACE("\tmajor = %s\n", qzdebugstr_guid(majorType));
	if(minorType)
		TRACE("\tsubtype = %s\n", qzdebugstr_guid(minorType));
	if(sourceFilter)
		TRACE("\tsource filter = %s\n", qzdebugstr_guid(sourceFilter));
    }

    return hr;
}

static IPin* WINAPI AsyncReader_GetPin(BaseFilter *iface, int pos)
{
    AsyncReader *This = impl_from_BaseFilter(iface);

    if (pos >= 1 || !This->pOutputPin)
        return NULL;

    IPin_AddRef(This->pOutputPin);
    return This->pOutputPin;
}

static LONG WINAPI AsyncReader_GetPinCount(BaseFilter *iface)
{
    AsyncReader *This = impl_from_BaseFilter(iface);

    if (!This->pOutputPin)
        return 0;
    else
        return 1;
}

static const BaseFilterFuncTable BaseFuncTable = {
    AsyncReader_GetPin,
    AsyncReader_GetPinCount
};

HRESULT AsyncReader_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    AsyncReader *pAsyncRead;
    
    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;
    
    pAsyncRead = CoTaskMemAlloc(sizeof(AsyncReader));

    if (!pAsyncRead)
        return E_OUTOFMEMORY;

    BaseFilter_Init(&pAsyncRead->filter, &AsyncReader_Vtbl, &CLSID_AsyncReader, (DWORD_PTR)(__FILE__ ": AsyncReader.csFilter"), &BaseFuncTable);

    pAsyncRead->IFileSourceFilter_iface.lpVtbl = &FileSource_Vtbl;
    pAsyncRead->IAMFilterMiscFlags_iface.lpVtbl = &IAMFilterMiscFlags_Vtbl;
    pAsyncRead->pOutputPin = NULL;

    pAsyncRead->pszFileName = NULL;
    pAsyncRead->pmt = NULL;

    *ppv = pAsyncRead;

    TRACE("-- created at %p\n", pAsyncRead);

    return S_OK;
}

/** IUnknown methods **/

static HRESULT WINAPI AsyncReader_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    AsyncReader *This = impl_from_IBaseFilter(iface);

    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->filter.IBaseFilter_iface;
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppv = &This->filter.IBaseFilter_iface;
    else if (IsEqualIID(riid, &IID_IMediaFilter))
        *ppv = &This->filter.IBaseFilter_iface;
    else if (IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = &This->filter.IBaseFilter_iface;
    else if (IsEqualIID(riid, &IID_IFileSourceFilter))
        *ppv = &This->IFileSourceFilter_iface;
    else if (IsEqualIID(riid, &IID_IAMFilterMiscFlags))
        *ppv = &This->IAMFilterMiscFlags_iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    if (!IsEqualIID(riid, &IID_IPin) && !IsEqualIID(riid, &IID_IMediaSeeking) &&
        !IsEqualIID(riid, &IID_IVideoWindow) && !IsEqualIID(riid, &IID_IBasicAudio))
        FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI AsyncReader_Release(IBaseFilter * iface)
{
    AsyncReader *This = impl_from_IBaseFilter(iface);
    ULONG refCount = InterlockedDecrement(&This->filter.refCount);
    
    TRACE("(%p)->() Release from %d\n", This, refCount + 1);
    
    if (!refCount)
    {
        if (This->pOutputPin)
        {
            IPin *pConnectedTo;
            if(SUCCEEDED(IPin_ConnectedTo(This->pOutputPin, &pConnectedTo)))
            {
                IPin_Disconnect(pConnectedTo);
                IPin_Release(pConnectedTo);
            }
            IPin_Disconnect(This->pOutputPin);
            IPin_Release(This->pOutputPin);
        }
        CoTaskMemFree(This->pszFileName);
        if (This->pmt)
            FreeMediaType(This->pmt);
        BaseFilter_Destroy(&This->filter);
        CoTaskMemFree(This);
        return 0;
    }
    else
        return refCount;
}

/** IMediaFilter methods **/

static HRESULT WINAPI AsyncReader_Stop(IBaseFilter * iface)
{
    AsyncReader *This = impl_from_IBaseFilter(iface);

    TRACE("()\n");

    This->filter.state = State_Stopped;
    
    return S_OK;
}

static HRESULT WINAPI AsyncReader_Pause(IBaseFilter * iface)
{
    AsyncReader *This = impl_from_IBaseFilter(iface);

    TRACE("()\n");

    This->filter.state = State_Paused;

    return S_OK;
}

static HRESULT WINAPI AsyncReader_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    AsyncReader *This = impl_from_IBaseFilter(iface);

    TRACE("(%x%08x)\n", (ULONG)(tStart >> 32), (ULONG)tStart);

    This->filter.state = State_Running;

    return S_OK;
}

/** IBaseFilter methods **/

static HRESULT WINAPI AsyncReader_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    AsyncReader *This = impl_from_IBaseFilter(iface);
    TRACE("(%s, %p)\n", debugstr_w(Id), ppPin);

    if (!Id || !ppPin)
        return E_POINTER;

    if (strcmpW(Id, wszOutputPinName))
    {
        *ppPin = NULL;
        return VFW_E_NOT_FOUND;
    }

    *ppPin = This->pOutputPin;
    IPin_AddRef(*ppPin);
    return S_OK;
}

static const IBaseFilterVtbl AsyncReader_Vtbl =
{
    AsyncReader_QueryInterface,
    BaseFilterImpl_AddRef,
    AsyncReader_Release,
    BaseFilterImpl_GetClassID,
    AsyncReader_Stop,
    AsyncReader_Pause,
    AsyncReader_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    AsyncReader_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

static HRESULT WINAPI FileSource_QueryInterface(IFileSourceFilter * iface, REFIID riid, LPVOID * ppv)
{
    AsyncReader *This = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_QueryInterface(&This->filter.IBaseFilter_iface, riid, ppv);
}

static ULONG WINAPI FileSource_AddRef(IFileSourceFilter * iface)
{
    AsyncReader *This = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI FileSource_Release(IFileSourceFilter * iface)
{
    AsyncReader *This = impl_from_IFileSourceFilter(iface);

    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI FileSource_Load(IFileSourceFilter * iface, LPCOLESTR pszFileName, const AM_MEDIA_TYPE * pmt)
{
    HRESULT hr;
    HANDLE hFile;
    IAsyncReader * pReader = NULL;
    AsyncReader *This = impl_from_IFileSourceFilter(iface);

    TRACE("(%s, %p)\n", debugstr_w(pszFileName), pmt);

    /* open file */
    /* FIXME: check the sharing values that native uses */
    hFile = CreateFileW(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    /* create pin */
    hr = FileAsyncReader_Construct(hFile, &This->filter.IBaseFilter_iface, &This->filter.csFilter, &This->pOutputPin);
    BaseFilterImpl_IncrementPinVersion(&This->filter);

    if (SUCCEEDED(hr))
        hr = IPin_QueryInterface(This->pOutputPin, &IID_IAsyncReader, (LPVOID *)&pReader);

    /* store file name & media type */
    if (SUCCEEDED(hr))
    {
        CoTaskMemFree(This->pszFileName);
        if (This->pmt)
            FreeMediaType(This->pmt);

        This->pszFileName = CoTaskMemAlloc((strlenW(pszFileName) + 1) * sizeof(WCHAR));
        strcpyW(This->pszFileName, pszFileName);

        This->pmt = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        if (!pmt)
        {
            This->pmt->bFixedSizeSamples = TRUE;
            This->pmt->bTemporalCompression = FALSE;
            This->pmt->cbFormat = 0;
            This->pmt->pbFormat = NULL;
            This->pmt->pUnk = NULL;
            This->pmt->lSampleSize = 0;
            This->pmt->formattype = FORMAT_None;
            hr = GetClassMediaFile(pReader, pszFileName, &This->pmt->majortype, &This->pmt->subtype, NULL);
            if (FAILED(hr))
            {
                CoTaskMemFree(This->pmt);
                This->pmt = NULL;
            }
        }
        else
            CopyMediaType(This->pmt, pmt);
    }

    if (pReader)
        IAsyncReader_Release(pReader);

    if (FAILED(hr))
    {
        if (This->pOutputPin)
        {
            IPin_Release(This->pOutputPin);
            This->pOutputPin = NULL;
        }

        CoTaskMemFree(This->pszFileName);
        if (This->pmt)
            FreeMediaType(This->pmt);
        This->pszFileName = NULL;
        This->pmt = NULL;

        CloseHandle(hFile);
    }

    /* FIXME: check return codes */
    return hr;
}

static HRESULT WINAPI FileSource_GetCurFile(IFileSourceFilter * iface, LPOLESTR * ppszFileName, AM_MEDIA_TYPE * pmt)
{
    AsyncReader *This = impl_from_IFileSourceFilter(iface);
    
    TRACE("(%p, %p)\n", ppszFileName, pmt);

    if (!ppszFileName)
        return E_POINTER;

    /* copy file name & media type if available, otherwise clear the outputs */
    if (This->pszFileName)
    {
        *ppszFileName = CoTaskMemAlloc((strlenW(This->pszFileName) + 1) * sizeof(WCHAR));
        strcpyW(*ppszFileName, This->pszFileName);
    }
    else
        *ppszFileName = NULL;

    if (pmt)
    {
        if (This->pmt)
            CopyMediaType(pmt, This->pmt);
        else
            ZeroMemory(pmt, sizeof(*pmt));
    }

    return S_OK;
}

static const IFileSourceFilterVtbl FileSource_Vtbl = 
{
    FileSource_QueryInterface,
    FileSource_AddRef,
    FileSource_Release,
    FileSource_Load,
    FileSource_GetCurFile
};


/* the dwUserData passed back to user */
typedef struct DATAREQUEST
{
    IMediaSample * pSample; /* sample passed to us by user */
    DWORD_PTR dwUserData; /* user data passed to us */
    OVERLAPPED ovl; /* our overlapped structure */
} DATAREQUEST;

typedef struct FileAsyncReader
{
    BaseOutputPin pin;
    IAsyncReader IAsyncReader_iface;

    ALLOCATOR_PROPERTIES allocProps;
    HANDLE hFile;
    BOOL bFlushing;
    /* Why would you need more? Every sample has its own handle */
    LONG queued_number;
    LONG samples;
    LONG oldest_sample;
    CRITICAL_SECTION csList; /* critical section to prevent concurrency issues */
    DATAREQUEST *sample_list;

    /* Have a handle for every sample, and then one more as flushing handle */
    HANDLE *handle_list;
} FileAsyncReader;

static inline FileAsyncReader *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, FileAsyncReader, pin.pin.IPin_iface);
}

static inline FileAsyncReader *impl_from_BasePin(BasePin *iface)
{
    return CONTAINING_RECORD(iface, FileAsyncReader, pin.pin);
}

static inline FileAsyncReader *impl_from_BaseOutputPin(BaseOutputPin *iface)
{
    return CONTAINING_RECORD(iface, FileAsyncReader, pin);
}

static inline BaseOutputPin *impl_BaseOututPin_from_BasePin(BasePin *iface)
{
    return CONTAINING_RECORD(iface, BaseOutputPin, pin);
}

static inline FileAsyncReader *impl_from_IAsyncReader(IAsyncReader *iface)
{
    return CONTAINING_RECORD(iface, FileAsyncReader, IAsyncReader_iface);
}

static HRESULT WINAPI FileAsyncReaderPin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *pmt)
{
    FileAsyncReader *This = impl_from_IPin(iface);
    AM_MEDIA_TYPE *pmt_filter = impl_from_IBaseFilter(This->pin.pin.pinInfo.pFilter)->pmt;

    FIXME("(%p, %p)\n", iface, pmt);

    if (IsEqualGUID(&pmt->majortype, &pmt_filter->majortype) &&
        IsEqualGUID(&pmt->subtype, &pmt_filter->subtype) &&
        IsEqualGUID(&pmt->formattype, &FORMAT_None))
        return S_OK;

    return S_FALSE;
}

static HRESULT WINAPI FileAsyncReaderPin_GetMediaType(BasePin *iface, int iPosition, AM_MEDIA_TYPE *pmt)
{
    FileAsyncReader *This = impl_from_BasePin(iface);
    if (iPosition < 0)
        return E_INVALIDARG;
    if (iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;
    CopyMediaType(pmt, impl_from_IBaseFilter(This->pin.pin.pinInfo.pFilter)->pmt);
    return S_OK;
}

/* overridden pin functions */

static HRESULT WINAPI FileAsyncReaderPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv)
{
    FileAsyncReader *This = impl_from_IPin(iface);
    TRACE("(%s, %p)\n", qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPin))
        *ppv = &This->pin.pin.IPin_iface;
    else if (IsEqualIID(riid, &IID_IAsyncReader))
        *ppv = &This->IAsyncReader_iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }

    if (!IsEqualIID(riid, &IID_IMediaSeeking))
        FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI FileAsyncReaderPin_Release(IPin * iface)
{
    FileAsyncReader *This = impl_from_IPin(iface);
    ULONG refCount = InterlockedDecrement(&This->pin.pin.refCount);
    int x;

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
    {
        CoTaskMemFree(This->sample_list);
        if (This->handle_list)
        {
            for (x = 0; x <= This->samples; ++x)
                CloseHandle(This->handle_list[x]);
            CoTaskMemFree(This->handle_list);
        }
        CloseHandle(This->hFile);
        This->csList.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->csList);
        BaseOutputPin_Destroy(&This->pin);
        return 0;
    }
    return refCount;
}

static const IPinVtbl FileAsyncReaderPin_Vtbl = 
{
    FileAsyncReaderPin_QueryInterface,
    BasePinImpl_AddRef,
    FileAsyncReaderPin_Release,
    BaseOutputPinImpl_Connect,
    BaseOutputPinImpl_ReceiveConnection,
    BasePinImpl_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    FileAsyncReaderPin_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseOutputPinImpl_EndOfStream,
    BaseOutputPinImpl_BeginFlush,
    BaseOutputPinImpl_EndFlush,
    BasePinImpl_NewSegment
};

/* Function called as a helper to IPin_Connect */
/* specific AM_MEDIA_TYPE - it cannot be NULL */
/* this differs from standard OutputPin_AttemptConnection only in that it
 * doesn't need the IMemInputPin interface on the receiving pin */
static HRESULT WINAPI FileAsyncReaderPin_AttemptConnection(BasePin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    BaseOutputPin *This = impl_BaseOututPin_from_BasePin(iface);
    HRESULT hr;

    TRACE("(%p, %p)\n", pReceivePin, pmt);
    dump_AM_MEDIA_TYPE(pmt);

    /* FIXME: call queryacceptproc */

    This->pin.pConnectedTo = pReceivePin;
    IPin_AddRef(pReceivePin);
    CopyMediaType(&This->pin.mtCurrent, pmt);

    hr = IPin_ReceiveConnection(pReceivePin, &iface->IPin_iface, pmt);

    if (FAILED(hr))
    {
        IPin_Release(This->pin.pConnectedTo);
        This->pin.pConnectedTo = NULL;
        FreeMediaType(&This->pin.mtCurrent);
    }

    TRACE(" -- %x\n", hr);
    return hr;
}

static HRESULT WINAPI FileAsyncReaderPin_DecideBufferSize(BaseOutputPin *iface, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    FileAsyncReader *This = impl_from_BaseOutputPin(iface);
    ALLOCATOR_PROPERTIES actual;

    if (ppropInputRequest->cbAlign && ppropInputRequest->cbAlign != This->allocProps.cbAlign)
        FIXME("Requested Buffer cbAlign mismatch %i,%i\n",This->allocProps.cbAlign, ppropInputRequest->cbAlign);
    if (ppropInputRequest->cbPrefix)
        FIXME("Requested Buffer cbPrefix mismatch %i,%i\n",This->allocProps.cbPrefix, ppropInputRequest->cbPrefix);
    if (ppropInputRequest->cbBuffer)
        FIXME("Requested Buffer cbBuffer mismatch %i,%i\n",This->allocProps.cbBuffer, ppropInputRequest->cbBuffer);
    if (ppropInputRequest->cBuffers)
        FIXME("Requested Buffer cBuffers mismatch %i,%i\n",This->allocProps.cBuffers, ppropInputRequest->cBuffers);

    return IMemAllocator_SetProperties(pAlloc, &This->allocProps, &actual);
}

static const BaseOutputPinFuncTable output_BaseOutputFuncTable = {
    {
        NULL,
        FileAsyncReaderPin_AttemptConnection,
        BasePinImpl_GetMediaTypeVersion,
        FileAsyncReaderPin_GetMediaType
    },
    FileAsyncReaderPin_DecideBufferSize,
    BaseOutputPinImpl_DecideAllocator,
    BaseOutputPinImpl_BreakConnect
};

static HRESULT FileAsyncReader_Construct(HANDLE hFile, IBaseFilter * pBaseFilter, LPCRITICAL_SECTION pCritSec, IPin ** ppPin)
{
    PIN_INFO piOutput;
    HRESULT hr;

    *ppPin = NULL;
    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = pBaseFilter;
    strcpyW(piOutput.achName, wszOutputPinName);
    hr = BaseOutputPin_Construct(&FileAsyncReaderPin_Vtbl, sizeof(FileAsyncReader), &piOutput, &output_BaseOutputFuncTable, pCritSec, ppPin);

    if (SUCCEEDED(hr))
    {
        FileAsyncReader *pPinImpl =  (FileAsyncReader *)*ppPin;
        pPinImpl->IAsyncReader_iface.lpVtbl = &FileAsyncReader_Vtbl;
        pPinImpl->hFile = hFile;
        pPinImpl->bFlushing = FALSE;
        pPinImpl->sample_list = NULL;
        pPinImpl->handle_list = NULL;
        pPinImpl->queued_number = 0;
        InitializeCriticalSection(&pPinImpl->csList);
        pPinImpl->csList.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": FileAsyncReader.csList");
    }
    return hr;
}

/* IAsyncReader */

static HRESULT WINAPI FileAsyncReader_QueryInterface(IAsyncReader * iface, REFIID riid, LPVOID * ppv)
{
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    return IPin_QueryInterface(&This->pin.pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI FileAsyncReader_AddRef(IAsyncReader * iface)
{
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    return IPin_AddRef(&This->pin.pin.IPin_iface);
}

static ULONG WINAPI FileAsyncReader_Release(IAsyncReader * iface)
{
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    return IPin_Release(&This->pin.pin.IPin_iface);
}

#define DEF_ALIGNMENT 1

static HRESULT WINAPI FileAsyncReader_RequestAllocator(IAsyncReader * iface, IMemAllocator * pPreferred, ALLOCATOR_PROPERTIES * pProps, IMemAllocator ** ppActual)
{
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    HRESULT hr = S_OK;

    TRACE("(%p, %p, %p)\n", pPreferred, pProps, ppActual);

    if (!pProps->cbAlign || (pProps->cbAlign % DEF_ALIGNMENT) != 0)
        pProps->cbAlign = DEF_ALIGNMENT;

    if (pPreferred)
    {
        hr = IMemAllocator_SetProperties(pPreferred, pProps, pProps);
        /* FIXME: check we are still aligned */
        if (SUCCEEDED(hr))
        {
            IMemAllocator_AddRef(pPreferred);
            *ppActual = pPreferred;
            TRACE("FileAsyncReader_RequestAllocator -- %x\n", hr);
            goto done;
        }
    }

    pPreferred = NULL;

    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC, &IID_IMemAllocator, (LPVOID *)&pPreferred);

    if (SUCCEEDED(hr))
    {
        hr = IMemAllocator_SetProperties(pPreferred, pProps, pProps);
        /* FIXME: check we are still aligned */
        if (SUCCEEDED(hr))
        {
            *ppActual = pPreferred;
            TRACE("FileAsyncReader_RequestAllocator -- %x\n", hr);
        }
    }

done:
    if (SUCCEEDED(hr))
    {
        CoTaskMemFree(This->sample_list);
        if (This->handle_list)
        {
            int x;
            for (x = 0; x <= This->samples; ++x)
                CloseHandle(This->handle_list[x]);
            CoTaskMemFree(This->handle_list);
        }

        This->samples = pProps->cBuffers;
        This->oldest_sample = 0;
        TRACE("Samples: %u\n", This->samples);
        This->sample_list = CoTaskMemAlloc(sizeof(This->sample_list[0]) * pProps->cBuffers);
        This->handle_list = CoTaskMemAlloc(sizeof(HANDLE) * pProps->cBuffers * 2);

        if (This->sample_list && This->handle_list)
        {
            int x;
            ZeroMemory(This->sample_list, sizeof(This->sample_list[0]) * pProps->cBuffers);
            for (x = 0; x < This->samples; ++x)
            {
                This->sample_list[x].ovl.hEvent = This->handle_list[x] = CreateEventW(NULL, 0, 0, NULL);
                if (x + 1 < This->samples)
                    This->handle_list[This->samples + 1 + x] = This->handle_list[x];
            }
            This->handle_list[This->samples] = CreateEventW(NULL, 1, 0, NULL);
            This->allocProps = *pProps;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CoTaskMemFree(This->sample_list);
            CoTaskMemFree(This->handle_list);
            This->samples = 0;
            This->sample_list = NULL;
            This->handle_list = NULL;
        }
    }

    if (FAILED(hr))
    {
        *ppActual = NULL;
        if (pPreferred)
            IMemAllocator_Release(pPreferred);
    }

    TRACE("-- %x\n", hr);
    return hr;
}

/* we could improve the Request/WaitForNext mechanism by allowing out of order samples.
 * however, this would be quite complicated to do and may be a bit error prone */
static HRESULT WINAPI FileAsyncReader_Request(IAsyncReader * iface, IMediaSample * pSample, DWORD_PTR dwUser)
{
    HRESULT hr = S_OK;
    REFERENCE_TIME Start;
    REFERENCE_TIME Stop;
    FileAsyncReader *This = impl_from_IAsyncReader(iface);
    LPBYTE pBuffer = NULL;

    TRACE("(%p, %lx)\n", pSample, dwUser);

    if (!pSample)
        return E_POINTER;

    /* get start and stop positions in bytes */
    if (SUCCEEDED(hr))
        hr = IMediaSample_GetTime(pSample, &Start, &Stop);

    if (SUCCEEDED(hr))
        hr = IMediaSample_GetPointer(pSample, &pBuffer);

    EnterCriticalSection(&This->csList);
    if (This->bFlushing)
    {
        LeaveCriticalSection(&This->csList);
        return VFW_E_WRONG_STATE;
    }

    if (SUCCEEDED(hr))
    {
        DWORD dwLength = (DWORD) BYTES_FROM_MEDIATIME(Stop - Start);
        DATAREQUEST *pDataRq;
        int x;

        /* Try to insert above the waiting sample if possible */
        for (x = This->oldest_sample; x < This->samples; ++x)
        {
            if (!This->sample_list[x].pSample)
                break;
        }

        if (x >= This->samples)
            for (x = 0; x < This->oldest_sample; ++x)
            {
                if (!This->sample_list[x].pSample)
                    break;
            }

        /* There must be a sample we have found */
        assert(x < This->samples);
        ++This->queued_number;

        pDataRq = This->sample_list + x;

        pDataRq->ovl.u.s.Offset = (DWORD) BYTES_FROM_MEDIATIME(Start);
        pDataRq->ovl.u.s.OffsetHigh = (DWORD)(BYTES_FROM_MEDIATIME(Start) >> (sizeof(DWORD) * 8));
        pDataRq->dwUserData = dwUser;

        /* we violate traditional COM rules here by maintaining
         * a reference to the sample, but not calling AddRef, but
         * that's what MSDN says to do */
        pDataRq->pSample = pSample;

        /* this is definitely not how it is implemented on Win9x
         * as they do not support async reads on files, but it is
         * sooo much easier to use this than messing around with threads!
         */
        if (!ReadFile(This->hFile, pBuffer, dwLength, NULL, &pDataRq->ovl))
            hr = HRESULT_FROM_WIN32(GetLastError());

        /* ERROR_IO_PENDING is not actually an error since this is what we want! */
        if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING))
            hr = S_OK;
    }

    LeaveCriticalSection(&This->csList);

    TRACE("-- %x\n", hr);
    return hr;
}

static HRESULT WINAPI FileAsyncReader_WaitForNext(IAsyncReader * iface, DWORD dwTimeout, IMediaSample ** ppSample, DWORD_PTR * pdwUser)
{
    HRESULT hr = S_OK;
    FileAsyncReader *This = impl_from_IAsyncReader(iface);
    DWORD buffer = ~0;

    TRACE("(%u, %p, %p)\n", dwTimeout, ppSample, pdwUser);

    *ppSample = NULL;
    *pdwUser = 0;

    EnterCriticalSection(&This->csList);
    if (!This->bFlushing)
    {
        LONG oldest = This->oldest_sample;

        if (!This->queued_number)
        {
            /* It could be that nothing is queued right now, but that can be fixed */
            WARN("Called without samples in queue and not flushing!!\n");
        }
        LeaveCriticalSection(&This->csList);

        /* wait for an object to read, or time out */
        buffer = WaitForMultipleObjectsEx(This->samples+1, This->handle_list + oldest, FALSE, dwTimeout, TRUE);

        EnterCriticalSection(&This->csList);
        if (buffer <= This->samples)
        {
            /* Re-scale the buffer back to normal */
            buffer += oldest;

            /* Uh oh, we overshot the flusher handle, renormalize it back to 0..Samples-1 */
            if (buffer > This->samples)
                buffer -= This->samples + 1;
            assert(buffer <= This->samples);
        }

        if (buffer >= This->samples)
        {
            if (buffer != This->samples)
            {
                FIXME("Returned: %u (%08x)\n", buffer, GetLastError());
                hr = VFW_E_TIMEOUT;
            }
            else
                hr = VFW_E_WRONG_STATE;
            buffer = ~0;
        }
        else
            --This->queued_number;
    }

    if (This->bFlushing && buffer == ~0)
    {
        for (buffer = 0; buffer < This->samples; ++buffer)
        {
            if (This->sample_list[buffer].pSample)
            {
                ResetEvent(This->handle_list[buffer]);
                break;
            }
        }
        if (buffer == This->samples)
        {
            assert(!This->queued_number);
            hr = VFW_E_TIMEOUT;
        }
        else
        {
            --This->queued_number;
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        REFERENCE_TIME rtStart, rtStop;
        REFERENCE_TIME rtSampleStart, rtSampleStop;
        DATAREQUEST *pDataRq = This->sample_list + buffer;
        DWORD dwBytes = 0;

        /* get any errors */
        if (!This->bFlushing && !GetOverlappedResult(This->hFile, &pDataRq->ovl, &dwBytes, FALSE))
            hr = HRESULT_FROM_WIN32(GetLastError());

        /* Return the sample no matter what so it can be destroyed */
        *ppSample = pDataRq->pSample;
        *pdwUser = pDataRq->dwUserData;

        if (This->bFlushing)
            hr = VFW_E_WRONG_STATE;

        if (FAILED(hr))
            dwBytes = 0;

        /* Set the time on the sample */
        IMediaSample_SetActualDataLength(pDataRq->pSample, dwBytes);

        rtStart = (DWORD64)pDataRq->ovl.u.s.Offset + ((DWORD64)pDataRq->ovl.u.s.OffsetHigh << 32);
        rtStart = MEDIATIME_FROM_BYTES(rtStart);
        rtStop = rtStart + MEDIATIME_FROM_BYTES(dwBytes);

        IMediaSample_GetTime(pDataRq->pSample, &rtSampleStart, &rtSampleStop);
        assert(rtStart == rtSampleStart);
        assert(rtStop <= rtSampleStop);

        IMediaSample_SetTime(pDataRq->pSample, &rtStart, &rtStop);
        assert(rtStart == rtSampleStart);
        if (hr == S_OK)
            assert(rtStop == rtSampleStop);
        else
            assert(rtStop == rtStart);

        This->sample_list[buffer].pSample = NULL;
        assert(This->oldest_sample < This->samples);

        if (buffer == This->oldest_sample)
        {
            LONG x;
            for (x = This->oldest_sample + 1; x < This->samples; ++x)
                if (This->sample_list[x].pSample)
                    break;
            if (x >= This->samples)
                for (x = 0; x < This->oldest_sample; ++x)
                    if (This->sample_list[x].pSample)
                        break;
            if (This->oldest_sample == x)
                /* No samples found, reset to 0 */
                x = 0;
            This->oldest_sample = x;
        }
    }
    LeaveCriticalSection(&This->csList);

    TRACE("-- %x\n", hr);
    return hr;
}

static HRESULT WINAPI FileAsyncReader_SyncRead(IAsyncReader * iface, LONGLONG llPosition, LONG lLength, BYTE * pBuffer);

static HRESULT WINAPI FileAsyncReader_SyncReadAligned(IAsyncReader * iface, IMediaSample * pSample)
{
    BYTE * pBuffer;
    REFERENCE_TIME tStart;
    REFERENCE_TIME tStop;
    HRESULT hr;

    TRACE("(%p)\n", pSample);

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);

    if (SUCCEEDED(hr))
        hr = IMediaSample_GetPointer(pSample, &pBuffer);

    if (SUCCEEDED(hr))
        hr = FileAsyncReader_SyncRead(iface, 
            BYTES_FROM_MEDIATIME(tStart),
            (LONG) BYTES_FROM_MEDIATIME(tStop - tStart),
            pBuffer);

    TRACE("-- %x\n", hr);
    return hr;
}

static HRESULT WINAPI FileAsyncReader_SyncRead(IAsyncReader * iface, LONGLONG llPosition, LONG lLength, BYTE * pBuffer)
{
    OVERLAPPED ovl;
    HRESULT hr = S_OK;
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    TRACE("(%x%08x, %d, %p)\n", (ULONG)(llPosition >> 32), (ULONG)llPosition, lLength, pBuffer);

    ZeroMemory(&ovl, sizeof(ovl));

    ovl.hEvent = CreateEventW(NULL, 0, 0, NULL);
    /* NOTE: llPosition is the actual byte position to start reading from */
    ovl.u.s.Offset = (DWORD) llPosition;
    ovl.u.s.OffsetHigh = (DWORD) (llPosition >> (sizeof(DWORD) * 8));

    if (!ReadFile(This->hFile, pBuffer, lLength, NULL, &ovl))
        hr = HRESULT_FROM_WIN32(GetLastError());

    if (hr == HRESULT_FROM_WIN32(ERROR_IO_PENDING))
        hr = S_OK;

    if (SUCCEEDED(hr))
    {
        DWORD dwBytesRead;

        if (!GetOverlappedResult(This->hFile, &ovl, &dwBytesRead, TRUE))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }

    CloseHandle(ovl.hEvent);

    TRACE("-- %x\n", hr);
    return hr;
}

static HRESULT WINAPI FileAsyncReader_Length(IAsyncReader * iface, LONGLONG * pTotal, LONGLONG * pAvailable)
{
    DWORD dwSizeLow;
    DWORD dwSizeHigh;
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    TRACE("(%p, %p)\n", pTotal, pAvailable);

    if (((dwSizeLow = GetFileSize(This->hFile, &dwSizeHigh)) == -1) &&
        (GetLastError() != NO_ERROR))
        return HRESULT_FROM_WIN32(GetLastError());

    *pTotal = (LONGLONG)dwSizeLow | (LONGLONG)dwSizeHigh << (sizeof(DWORD) * 8);

    *pAvailable = *pTotal;

    return S_OK;
}

static HRESULT WINAPI FileAsyncReader_BeginFlush(IAsyncReader * iface)
{
    FileAsyncReader *This = impl_from_IAsyncReader(iface);

    TRACE("()\n");

    EnterCriticalSection(&This->csList);
    This->bFlushing = TRUE;
    CancelIo(This->hFile);
    SetEvent(This->handle_list[This->samples]);
    LeaveCriticalSection(&This->csList);

    return S_OK;
}

static HRESULT WINAPI FileAsyncReader_EndFlush(IAsyncReader * iface)
{
    FileAsyncReader *This = impl_from_IAsyncReader(iface);
    int x;

    TRACE("()\n");

    EnterCriticalSection(&This->csList);
    ResetEvent(This->handle_list[This->samples]);
    This->bFlushing = FALSE;
    for (x = 0; x < This->samples; ++x)
        assert(!This->sample_list[x].pSample);

    LeaveCriticalSection(&This->csList);

    return S_OK;
}

static const IAsyncReaderVtbl FileAsyncReader_Vtbl = 
{
    FileAsyncReader_QueryInterface,
    FileAsyncReader_AddRef,
    FileAsyncReader_Release,
    FileAsyncReader_RequestAllocator,
    FileAsyncReader_Request,
    FileAsyncReader_WaitForNext,
    FileAsyncReader_SyncReadAligned,
    FileAsyncReader_SyncRead,
    FileAsyncReader_Length,
    FileAsyncReader_BeginFlush,
    FileAsyncReader_EndFlush,
};


static HRESULT WINAPI AMFilterMiscFlags_QueryInterface(IAMFilterMiscFlags *iface, REFIID riid, void **ppv) {
    AsyncReader *This = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_QueryInterface((IUnknown*)This, riid, ppv);
}

static ULONG WINAPI AMFilterMiscFlags_AddRef(IAMFilterMiscFlags *iface) {
    AsyncReader *This = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_AddRef((IUnknown*)This);
}

static ULONG WINAPI AMFilterMiscFlags_Release(IAMFilterMiscFlags *iface) {
    AsyncReader *This = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_Release((IUnknown*)This);
}

static ULONG WINAPI AMFilterMiscFlags_GetMiscFlags(IAMFilterMiscFlags *iface) {
    return AM_FILTER_MISC_FLAGS_IS_SOURCE;
}

static const IAMFilterMiscFlagsVtbl IAMFilterMiscFlags_Vtbl = {
    AMFilterMiscFlags_QueryInterface,
    AMFilterMiscFlags_AddRef,
    AMFilterMiscFlags_Release,
    AMFilterMiscFlags_GetMiscFlags
};
