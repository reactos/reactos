/*
 * Copyright 1999, 2000 Juergen Schmied
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

#ifndef __ROS_SHELL_UTILS_H
#define __ROS_SHELL_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

static inline ULONG
Win32DbgPrint(const char *filename, int line, const char *lpFormat, ...)
{
    char szMsg[512];
    char *szMsgStart;
    const char *fname;
    va_list vl;

    fname = strrchr(filename, '\\');
    if (fname == NULL)
    {
        fname = strrchr(filename, '/');
        if (fname != NULL)
            fname++;
    }
    else
        fname++;

    if (fname == NULL)
        fname = filename;

    szMsgStart = szMsg + sprintf(szMsg, "%s:%d: ", fname, line);

    va_start(vl, lpFormat);
    vsprintf(szMsgStart, lpFormat, vl);
    va_end(vl);

    OutputDebugStringA(szMsg);

    /* Return STATUS_SUCCESS, since we are supposed to mimic DbgPrint */
    return 0;
}

#define DbgPrint(fmt, ...) \
    Win32DbgPrint(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
#   define IID_PPV_ARG(Itype, ppType) IID_##Itype, reinterpret_cast<void**>((static_cast<Itype**>(ppType)))
#   define IID_NULL_PPV_ARG(Itype, ppType) IID_##Itype, NULL, reinterpret_cast<void**>((static_cast<Itype**>(ppType)))
#else
#   define IID_PPV_ARG(Itype, ppType) &IID_##Itype, (void**)(ppType)
#   define IID_NULL_PPV_ARG(Itype, ppType) &IID_##Itype, NULL, (void**)(ppType)
#endif

static inline HRESULT HResultFromWin32(DWORD hr)
{
     // HRESULT_FROM_WIN32 will evaluate its parameter twice, this function will not.
    return HRESULT_FROM_WIN32(hr);
}

#if 1

static inline BOOL _ROS_FAILED_HELPER(HRESULT hr, const char* expr, const char* filename, int line)
{
    if (FAILED(hr))
    {
        Win32DbgPrint(filename, line, "Unexpected failure (%s)=%08x.\n", expr, hr);
        return TRUE;
    }
    return FALSE;
}

#define FAILED_UNEXPECTEDLY(hr) _ROS_FAILED_HELPER((hr), #hr, __FILE__, __LINE__)
#else
#define FAILED_UNEXPECTEDLY(hr) FAILED(hr)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

static inline UINT
SHELL_ErrorBoxHelper(HWND hwndOwner, UINT Error)
{
    WCHAR buf[400];
    UINT cch;

    if (!IsWindowVisible(hwndOwner))
        hwndOwner = NULL;
    if (Error == ERROR_SUCCESS)
        Error = ERROR_INTERNAL_ERROR;

    cch = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                         NULL, Error, 0, buf, _countof(buf), NULL);
    if (!cch)
    {
        enum { user32_IDS_ERROR = 2 }; // IDS_ERROR from user32 resource.h ("Error" string)
        cch = LoadStringW(LoadLibraryW(L"USER32"), user32_IDS_ERROR, buf, _countof(buf));
        wsprintfW(buf + cch, L"\n\n%#x (%d)", Error, Error);
    }
    MessageBoxW(hwndOwner, buf, NULL, MB_OK | MB_ICONSTOP);
    return Error;
}
#ifdef __cplusplus
template<class H> static UINT
SHELL_ErrorBox(H hwndOwner, UINT Error = GetLastError())
{
    return SHELL_ErrorBoxHelper(const_cast<HWND>(hwndOwner), Error);
}
#else
#define SHELL_ErrorBox SHELL_ErrorBoxHelper
#endif

#ifdef __cplusplus
template <typename T>
class CComCreatorCentralInstance
{
private:
    static IUnknown *s_pInstance;

public:
    static HRESULT WINAPI CreateInstance(void *pv, REFIID riid, LPVOID *ppv)
    {
        *ppv = NULL;
        if (pv != NULL)
            return CLASS_E_NOAGGREGATION;
        if (!s_pInstance)
        {
            PVOID pObj;
            HRESULT hr;
            hr = ATL::CComCreator< T >::CreateInstance(NULL, IID_IUnknown, &pObj);
            if (FAILED(hr))
                return hr;
            if (InterlockedCompareExchangePointer((PVOID *)&s_pInstance, pObj, NULL))
                static_cast<IUnknown *>(pObj)->Release();
        }
        return s_pInstance->QueryInterface(riid, ppv);
    }
    static void Term()
    {
        if (s_pInstance)
        {
            s_pInstance->Release();
            s_pInstance = NULL;
        }
    }
};

template <typename T>
IUnknown *CComCreatorCentralInstance<T>::s_pInstance = NULL;

#define DECLARE_CENTRAL_INSTANCE_NOT_AGGREGATABLE(x)                            \
public:                                                                         \
    typedef CComCreatorCentralInstance< ATL::CComObject<x> > _CreatorClass;


template <class Base>
class CComDebugObject : public Base
{
public:
    CComDebugObject(void * = NULL)
    {
#if DEBUG_CCOMOBJECT_CREATION
        DbgPrint("%S, this=%08p\n", __FUNCTION__, static_cast<Base*>(this));
#endif
        _pAtlModule->Lock();
    }

    virtual ~CComDebugObject()
    {
        this->FinalRelease();
        _pAtlModule->Unlock();
    }

    STDMETHOD_(ULONG, AddRef)()
    {
        int rc = this->InternalAddRef();
#if DEBUG_CCOMOBJECT_REFCOUNTING
        DbgPrint("%s, RefCount is now %d(--)!\n", __FUNCTION__, rc);
#endif
        return rc;
    }

    STDMETHOD_(ULONG, Release)()
    {
        int rc = this->InternalRelease();

#if DEBUG_CCOMOBJECT_REFCOUNTING
        DbgPrint("%s, RefCount is now %d(--)!\n", __FUNCTION__, rc);
#endif

        if (rc == 0)
        {
#if DEBUG_CCOMOBJECT_DESTRUCTION
            DbgPrint("%s, RefCount reached 0 Deleting!\n", __FUNCTION__);
#endif
            delete this;
        }
        return rc;
    }

    STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject)
    {
        return this->_InternalQueryInterface(iid, ppvObject);
    }

    static HRESULT WINAPI CreateInstance(CComDebugObject<Base> **pp)
    {
        CComDebugObject<Base>				*newInstance;
        HRESULT								hResult;

        ATLASSERT(pp != NULL);
        if (pp == NULL)
            return E_POINTER;

        hResult = E_OUTOFMEMORY;
        newInstance = NULL;
        ATLTRY(newInstance = new CComDebugObject<Base>());
        if (newInstance != NULL)
        {
            newInstance->SetVoid(NULL);
            newInstance->InternalFinalConstructAddRef();
            hResult = newInstance->_AtlInitialConstruct();
            if (SUCCEEDED(hResult))
                hResult = newInstance->FinalConstruct();
            if (SUCCEEDED(hResult))
                hResult = newInstance->_AtlFinalConstruct();
            newInstance->InternalFinalConstructRelease();
            if (hResult != S_OK)
            {
                delete newInstance;
                newInstance = NULL;
            }
        }
        *pp = newInstance;
        return hResult;
    }
};

#ifdef DEBUG_CCOMOBJECT
#   define _CComObject CComDebugObject
#else
#   define _CComObject CComObject
#endif

template<class T>
ULONG ReleaseCComPtrExpectZeroHelper(const char *file, UINT line, CComPtr<T>& cptr, BOOL forceRelease = FALSE)
{
    ULONG r = 0;
    if (cptr.p != NULL)
    {
        T *raw = cptr.Detach();
        int nrc = r = raw->Release();
        if (nrc > 0)
            Win32DbgPrint(file, line, "WARNING: Unexpected RefCount > 0 (%d)\n", nrc);
        while (nrc > 0 && forceRelease)
        {
            nrc = raw->Release();
        }
    }
    return r;
}
#define ReleaseCComPtrExpectZero(...) ReleaseCComPtrExpectZeroHelper(__FILE__, __LINE__, __VA_ARGS__)

template<class T, class R>
HRESULT inline ShellDebugObjectCreator(REFIID riid, R ** ppv)
{
    CComPtr<T>       obj;
    HRESULT          hResult;

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;
    ATLTRY(obj = new CComDebugObject<T>);
    if (obj.p == NULL)
        return E_OUTOFMEMORY;
    hResult = obj->QueryInterface(riid, reinterpret_cast<void **>(ppv));
    if (FAILED(hResult))
        return hResult;
    return S_OK;
}

template<class T>
HRESULT inline ShellObjectCreator(CComPtr<T> &objref)
{
    _CComObject<T> *pobj;
    HRESULT hResult = _CComObject<T>::CreateInstance(&pobj);
    objref = pobj; // AddRef() gets called here
    if (FAILED(hResult))
        return hResult;
    return S_OK;
}

template<class T>
HRESULT inline ShellObjectCreator(REFIID riid, void ** ppv)
{
    _CComObject<T> *pobj;
    HRESULT hResult;

    hResult = _CComObject<T>::CreateInstance(&pobj);
    if (FAILED(hResult))
        return hResult;

    pobj->AddRef(); /* CreateInstance returns object with 0 ref count */

    hResult = pobj->QueryInterface(riid, reinterpret_cast<void **>(ppv));

    pobj->Release(); /* In case of failure the object will be released */

    return hResult;
}

template<class T>
HRESULT inline ShellObjectCreatorInit(REFIID riid, void ** ppv)
{
    _CComObject<T> *pobj;
    HRESULT hResult;

    hResult = _CComObject<T>::CreateInstance(&pobj);
    if (FAILED(hResult))
        return hResult;

    pobj->AddRef(); /* CreateInstance returns object with 0 ref count */

    hResult = pobj->Initialize();

    if (SUCCEEDED(hResult))
        hResult = pobj->QueryInterface(riid, reinterpret_cast<void **>(ppv));

    pobj->Release(); /* In case of failure the object will be released */

    return hResult;
}

template<class T, class T1>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, REFIID riid, void ** ppv)
{
    _CComObject<T> *pobj;
    HRESULT hResult;

    hResult = _CComObject<T>::CreateInstance(&pobj);
    if (FAILED(hResult))
        return hResult;

    pobj->AddRef(); /* CreateInstance returns object with 0 ref count */

    hResult = pobj->Initialize(initArg1);

    if (SUCCEEDED(hResult))
        hResult = pobj->QueryInterface(riid, reinterpret_cast<void **>(ppv));

    pobj->Release(); /* In case of failure the object will be released */

    return hResult;
}

template<class T, class T1, class T2>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, T2 initArg2, REFIID riid, void ** ppv)
{
    _CComObject<T> *pobj;
    HRESULT hResult;

    hResult = _CComObject<T>::CreateInstance(&pobj);
    if (FAILED(hResult))
        return hResult;

    pobj->AddRef(); /* CreateInstance returns object with 0 ref count */

    hResult = pobj->Initialize(initArg1, initArg2);

    if (SUCCEEDED(hResult))
        hResult = pobj->QueryInterface(riid, reinterpret_cast<void **>(ppv));

    pobj->Release(); /* In case of failure the object will be released */

    return hResult;
}

template<class T, class T1, class T2, class T3>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, T2 initArg2, T3 initArg3, REFIID riid, void ** ppv)
{
    _CComObject<T> *pobj;
    HRESULT hResult;

    hResult = _CComObject<T>::CreateInstance(&pobj);
    if (FAILED(hResult))
        return hResult;

    pobj->AddRef(); /* CreateInstance returns object with 0 ref count */

    hResult = pobj->Initialize(initArg1, initArg2, initArg3);

    if (SUCCEEDED(hResult))
        hResult = pobj->QueryInterface(riid, reinterpret_cast<void **>(ppv));

    pobj->Release(); /* In case of failure the object will be released */

    return hResult;
}

template<class T, class T1, class T2, class T3, class T4>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, T2 initArg2, T3 initArg3, T4 initArg4, REFIID riid, void ** ppv)
{
    _CComObject<T> *pobj;
    HRESULT hResult;

    hResult = _CComObject<T>::CreateInstance(&pobj);
    if (FAILED(hResult))
        return hResult;

    pobj->AddRef(); /* CreateInstance returns object with 0 ref count */

    hResult = pobj->Initialize(initArg1, initArg2, initArg3, initArg4);

    if (SUCCEEDED(hResult))
        hResult = pobj->QueryInterface(riid, reinterpret_cast<void **>(ppv));

    pobj->Release(); /* In case of failure the object will be released */

    return hResult;
}

template<class T, class T1, class T2, class T3, class T4, class T5>
HRESULT inline ShellObjectCreatorInit(T1 initArg1, T2 initArg2, T3 initArg3, T4 initArg4, T5 initArg5, REFIID riid, void ** ppv)
{
    _CComObject<T> *pobj;
    HRESULT hResult;

    hResult = _CComObject<T>::CreateInstance(&pobj);
    if (FAILED(hResult))
        return hResult;

    pobj->AddRef(); /* CreateInstance returns object with 0 ref count */

    hResult = pobj->Initialize(initArg1, initArg2, initArg3, initArg4, initArg5);

    if (SUCCEEDED(hResult))
        hResult = pobj->QueryInterface(riid, reinterpret_cast<void **>(ppv));

    pobj->Release(); /* In case of failure the object will be released */

    return hResult;
}

template<class P, class R> static HRESULT SHILClone(P pidl, R *ppOut)
{
    R r = *ppOut = (R)ILClone((PIDLIST_RELATIVE)pidl);
    return r ? S_OK : E_OUTOFMEMORY;
}

template<class B, class R> static HRESULT SHILCombine(B base, PCUIDLIST_RELATIVE sub, R *ppOut)
{
    R r = *ppOut = (R)ILCombine((PCIDLIST_ABSOLUTE)base, sub);
    return r ? S_OK : E_OUTOFMEMORY;
}

static inline bool StrIsNullOrEmpty(LPCSTR str) { return !str || !*str; }
static inline bool StrIsNullOrEmpty(LPCWSTR str) { return !str || !*str; }

HRESULT inline SHSetStrRet(LPSTRRET pStrRet, LPCSTR pstrValue)
{
    pStrRet->uType = STRRET_CSTR;
    strcpy(pStrRet->cStr, pstrValue);
    return S_OK;
}

HRESULT inline SHSetStrRet(LPSTRRET pStrRet, LPCWSTR pwstrValue)
{
    SIZE_T cchr = wcslen(pwstrValue);
    LPWSTR buffer = static_cast<LPWSTR>(CoTaskMemAlloc((cchr + 1) * sizeof(WCHAR)));
    if (buffer == NULL)
        return E_OUTOFMEMORY;

    pStrRet->uType = STRRET_WSTR;
    pStrRet->pOleStr = buffer;
    wcscpy(buffer, pwstrValue);
    return S_OK;
}

HRESULT inline SHSetStrRet(LPSTRRET pStrRet, HINSTANCE hInstance, DWORD resId)
{
    WCHAR Buffer[MAX_PATH];

    if (!LoadStringW(hInstance, resId, Buffer, MAX_PATH))
        return E_FAIL;

    return SHSetStrRet(pStrRet, Buffer);
}

static inline void DbgDumpMenuInternal(HMENU hmenu, char* padding, int padlevel)
{
    WCHAR label[128];
    int i;
    int count = GetMenuItemCount(hmenu);

    padding[padlevel] = '.';
    padding[padlevel + 1] = '.';
    padding[padlevel + 2] = 0;

    for (i = 0; i < count; i++)
    {
        MENUITEMINFOW mii = { 0 };

        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_SUBMENU | MIIM_STATE | MIIM_ID;
        mii.dwTypeData = label;
        mii.cch = _countof(label);

        GetMenuItemInfoW(hmenu, i, TRUE, &mii);

        if (mii.fType & MFT_BITMAP)
            DbgPrint("%s%2d - %08x: BITMAP %08p (state=%d, has submenu=%s)\n", padding, i, mii.wID, mii.hbmpItem, mii.fState, mii.hSubMenu ? "TRUE" : "FALSE");
        else if (mii.fType & MFT_SEPARATOR)
            DbgPrint("%s%2d - %08x ---SEPARATOR---\n", padding, i, mii.wID);
        else
            DbgPrint("%s%2d - %08x: %S (state=%d, has submenu=%s)\n", padding, i, mii.wID, mii.dwTypeData, mii.fState, mii.hSubMenu ? "TRUE" : "FALSE");

        if (mii.hSubMenu)
            DbgDumpMenuInternal(mii.hSubMenu, padding, padlevel + 2);

    }

    padding[padlevel] = 0;
}

static __inline void DbgDumpMenu(HMENU hmenu)
{
    char padding[128];
    DbgDumpMenuInternal(hmenu, padding, 0);
}


static inline
void DumpIdList(LPCITEMIDLIST pcidl)
{
    DbgPrint("Begin IDList Dump\n");

    for (; pcidl != NULL; pcidl = ILGetNext(pcidl))
    {
        int i;
        int cb = pcidl->mkid.cb;
        BYTE * sh = (BYTE*) &(pcidl->mkid);
        if (cb == 0) // ITEMIDLISTs are terminatedwith a null SHITEMID.
            break;
        DbgPrint("Begin SHITEMID (cb=%d)\n", cb);
        if ((cb & 3) != 0)
            DbgPrint(" - WARNING: cb is not a multiple of 4\n");
        for (i = 0; (i + 4) <= cb; i += 4)
        {
            DbgPrint(" - abID[%08x]: %02x %02x %02x %02x\n",
                     i,
                     sh[i + 0],
                     sh[i + 1],
                     sh[i + 2],
                     sh[i + 3]);
        }
        if (i < cb)
        {
            cb -= i;
            if (cb == 3)
            {
                DbgPrint(" - abID[%08x]: %02x %02x %02x --\n",
                         i,
                         sh[i + 0],
                         sh[i + 1],
                         sh[i + 2]);
            }
            else if (cb == 2)
            {
                DbgPrint(" - abID[%08x]: %02x %02x -- --\n",
                         i,
                         sh[i + 0],
                         sh[i + 1]);
            }
            else if (cb == 1)
            {
                DbgPrint(" - abID[%08x]: %02x -- -- --\n",
                         i,
                         sh[i + 0]);
            }
        }
        DbgPrint("End SHITEMID\n");
    }
    DbgPrint("End IDList Dump.\n");
}

template <HRESULT (WINAPI *InitFunc)(void*), void (WINAPI *UninitFunc)()>
struct CCoInitBase
{
    HRESULT hr;
    CCoInitBase() : hr(InitFunc(NULL)) { }
    ~CCoInitBase()
    {
        if (SUCCEEDED(hr))
            UninitFunc();
    }
};
typedef CCoInitBase<CoInitialize, CoUninitialize> CCoInit;
typedef CCoInitBase<OleInitialize, OleUninitialize> COleInit;

#endif /* __cplusplus */

#define S_LESSTHAN 0xffff
#define S_EQUAL S_OK
#define S_GREATERTHAN S_FALSE
#define MAKE_COMPARE_HRESULT(x) ((x)>0 ? S_GREATERTHAN : ((x)<0 ? S_LESSTHAN : S_EQUAL))

#define SEE_CMIC_COMMON_BASICFLAGS (SEE_MASK_NOASYNC | SEE_MASK_ASYNCOK | SEE_MASK_UNICODE | \
                                    SEE_MASK_NO_CONSOLE | SEE_MASK_FLAG_NO_UI | SEE_MASK_FLAG_SEPVDM | \
                                    SEE_MASK_FLAG_LOG_USAGE | SEE_MASK_NOZONECHECKS)
#define SEE_CMIC_COMMON_FLAGS      (SEE_CMIC_COMMON_BASICFLAGS | SEE_MASK_HOTKEY | SEE_MASK_ICON | \
                                    SEE_MASK_HASLINKNAME | SEE_MASK_HASTITLE)

static inline BOOL ILIsSingle(LPCITEMIDLIST pidl)
{
    return pidl == ILFindLastID(pidl);
}

static inline PCUIDLIST_ABSOLUTE HIDA_GetPIDLFolder(CIDA const* pida)
{
    return (PCUIDLIST_ABSOLUTE)(((LPBYTE)pida) + (pida)->aoffset[0]);
}

static inline PCUIDLIST_RELATIVE HIDA_GetPIDLItem(CIDA const* pida, SIZE_T i)
{
    return (PCUIDLIST_RELATIVE)(((LPBYTE)pida) + (pida)->aoffset[i + 1]);
}


#ifdef __cplusplus

#if defined(CMIC_MASK_UNICODE) && defined(SEE_MASK_UNICODE)
static inline bool IsUnicode(const CMINVOKECOMMANDINFOEX &ici)
{
    const UINT minsize = FIELD_OFFSET(CMINVOKECOMMANDINFOEX, ptInvoke);
    return (ici.fMask & CMIC_MASK_UNICODE) && ici.cbSize >= minsize;
}

static inline bool IsUnicode(const CMINVOKECOMMANDINFO &ici)
{
    return IsUnicode(*(CMINVOKECOMMANDINFOEX*)&ici);
}
#endif // CMIC_MASK_UNICODE

DECLSPEC_SELECTANY CLIPFORMAT g_cfHIDA = NULL;
DECLSPEC_SELECTANY CLIPFORMAT g_cfShellIdListOffsets = NULL;

// Allow to use the HIDA from an IDataObject without copying it
struct CDataObjectHIDA
{
private:
    STGMEDIUM m_medium;
    CIDA* m_cida;
    HRESULT m_hr;

public:
    explicit CDataObjectHIDA(IDataObject* pDataObject)
        : m_cida(nullptr)
    {
        m_hr = CreateCIDA(pDataObject, &m_cida, m_medium);
    }

    ~CDataObjectHIDA()
    {
        DestroyCIDA(m_cida, m_medium);
    }

    static void DestroyCIDA(CIDA *pcida, STGMEDIUM &medium)
    {
        if (pcida)
            ::GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);
    }

    static HRESULT CreateCIDA(IDataObject* pDataObject, CIDA **ppcida, STGMEDIUM &medium)
    {
        *ppcida = NULL;
        medium.pUnkForRelease = NULL;
        if (g_cfHIDA == NULL)
            g_cfHIDA = (CLIPFORMAT)RegisterClipboardFormatW(CFSTR_SHELLIDLISTW);
        
        FORMATETC fmt = { g_cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        HRESULT hr = pDataObject->GetData(&fmt, &medium);
        if (SUCCEEDED(hr))
        {
            *ppcida = (CIDA*)::GlobalLock(medium.hGlobal);
            if (*ppcida)
                return S_OK;
            ReleaseStgMedium(&medium);
            hr = E_UNEXPECTED;
        }
        medium.tymed = TYMED_NULL;
        return hr;
    }

    HRESULT hr() const
    {
        return m_hr;
    }

    operator bool() const
    {
        return m_cida != nullptr;
    }

    operator const CIDA* () const
    {
        return m_cida;
    }

    const CIDA* operator->() const
    {
        return m_cida;
    }
};

inline
HRESULT DataObject_GetData(IDataObject* pDataObject, CLIPFORMAT clipformat, PVOID pBuffer, SIZE_T dwBufferSize)
{
    FORMATETC fmt = { clipformat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM medium = { TYMED_NULL };

    HRESULT hr = pDataObject->GetData(&fmt, &medium);
    if (SUCCEEDED(hr))
    {
        LPVOID blob = GlobalLock(medium.hGlobal);
        if (blob)
        {
            SIZE_T size = GlobalSize(medium.hGlobal);
            if (size <= dwBufferSize)
            {
                CopyMemory(pBuffer, blob, size);
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            GlobalUnlock(medium.hGlobal);
        }
        else
        {
            hr = STG_E_INVALIDHANDLE;
        }

        ReleaseStgMedium(&medium);
    }
    return hr;
}

inline
HRESULT DataObject_SetData(IDataObject* pDataObject, CLIPFORMAT clipformat, PVOID pBuffer, SIZE_T dwBufferSize)
{
    STGMEDIUM medium = { TYMED_HGLOBAL };

    medium.hGlobal = GlobalAlloc(GHND, dwBufferSize);
    if (!medium.hGlobal)
        return E_OUTOFMEMORY;

    HRESULT hr = E_UNEXPECTED;
    LPVOID blob = GlobalLock(medium.hGlobal);
    if (blob)
    {
        CopyMemory(blob, pBuffer, dwBufferSize);
        GlobalUnlock(medium.hGlobal);

        FORMATETC etc = { clipformat, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        hr = pDataObject->SetData(&etc, &medium, TRUE);
    }

    if (FAILED(hr))
        GlobalFree(medium.hGlobal);

    return hr;
}


inline HRESULT
DataObject_GetOffset(IDataObject *pDataObject, POINT *point)
{
    if (g_cfShellIdListOffsets == NULL)
    {
        g_cfShellIdListOffsets = (CLIPFORMAT)RegisterClipboardFormatW(CFSTR_SHELLIDLISTOFFSETW);
    }

    point->x = point->y = 0;

    return DataObject_GetData(pDataObject, g_cfShellIdListOffsets, point, sizeof(point[0]));
}

inline HRESULT
DataObject_SetOffset(IDataObject* pDataObject, POINT* point)
{
    if (g_cfShellIdListOffsets == NULL)
    {
        g_cfShellIdListOffsets = (CLIPFORMAT)RegisterClipboardFormatW(CFSTR_SHELLIDLISTOFFSETW);
    }

    return DataObject_SetData(pDataObject, g_cfShellIdListOffsets, point, sizeof(point[0]));
}

#endif // __cplusplus

#ifdef __cplusplus
struct SHELL_GetSettingImpl
{
    SHELLSTATE ss;
    SHELL_GetSettingImpl(DWORD ssf) { SHGetSetSettings(&ss, ssf, FALSE); }
    const SHELLSTATE* operator ->() { return &ss; }
};
#define SHELL_GetSetting(ssf, field) ( SHELL_GetSettingImpl(ssf)->field )
#else
#define SHELL_GetSetting(pss, ssf, field) ( SHGetSetSettings((pss), (ssf), FALSE), (pss)->field )
#endif

static inline void DumpIdListOneLine(LPCITEMIDLIST pidl)
{
    char buf[1024], *data, drive = 0;
    for (UINT depth = 0, type; ; pidl = ILGetNext(pidl), ++depth)
    {
        if (!pidl || !pidl->mkid.cb)
        {
            if (!depth)
            {
                wsprintfA(buf, "%p [] (%s)\n", pidl, pidl ? "Empty/Desktop" : "NULL");
                OutputDebugStringA(buf);
            }
            break;
        }
        else if (!depth)
        {
            wsprintfA(buf, "%p", pidl);
            OutputDebugStringA(buf);
        }
        type = pidl->mkid.abID[0] & 0x7f;
        data = (char*)&pidl->mkid.abID[0];
        if (depth == 0 && type == 0x1f && pidl->mkid.cb == 20 && *(UINT*)(&data[2]) == 0x20D04FE0)
        {
            wsprintfA(buf, " [%.2x ThisPC?]", type); /* "?" because we did not check the full GUID */
        }
        else if (depth == 1 && type >= 0x20 && type < 0x30 && type != 0x2E && pidl->mkid.cb > 4)
        {
            drive = data[1];
            wsprintfA(buf, " [%.2x %c: %ub]", type, drive, pidl->mkid.cb);
        }
        else if (depth >= 2 && drive && (type & 0x70) == 0x30) /* PT_FS */
        {
            if (type & 4)
                wsprintfA(buf, " [%.2x FS %.256ls %ub]", type, data + 12, pidl->mkid.cb);
            else
                wsprintfA(buf, " [%.2x FS %.256hs %ub]", type, data + 12, pidl->mkid.cb);
        }
        else
        {
            wsprintfA(buf, " [%.2x ? %ub]", type, pidl->mkid.cb);
        }
        OutputDebugStringA(buf);
    }
    OutputDebugStringA("\n");
}

#endif /* __ROS_SHELL_UTILS_H */
