
#ifdef _MSC_VER

// Disabling spammy warnings when compiling with /W4 or /Wall
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4201) // nonstandard extension used
#pragma warning(disable:4265) // class has virtual functions, but destructor is not virtual
#pragma warning(disable:4365) // signed/unsigned mismatch
#pragma warning(disable:4514) // unreferenced inline function
#pragma warning(disable:4710) // function was not inlined
#pragma warning(disable:4820) // padding added
#pragma warning(disable:4946) // reinterpret_cast between related classes

// Disable some warnings in headers only
#pragma warning(push)
#pragma warning(disable:4244) // possible loss of data
#pragma warning(disable:4512) // assignment operator could not be gernerated
#endif

#define USE_SYSTEM_MENUDESKBAR 0
#define USE_SYSTEM_MENUSITE 0
#define USE_SYSTEM_MENUBAND 0

#define WRAP_MENUDESKBAR 0
#define WRAP_MENUSITE 0
#define WRAP_MENUBAND 0
#define WRAP_TRAYPRIV 0

#define MERGE_FOLDERS 0

#include <stdio.h>
#include <tchar.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <winnls.h>
#include <wincon.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlwapi.h>
#include <shlguid_undoc.h>
#include <uxtheme.h>
#include <strsafe.h>

#include <atlbase.h>
#include <atlcom.h>
#include <wine/debug.h>

#if _MSC_VER
// Restore warnings
#pragma warning(pop)
#endif

#define shell32_hInstance 0
#define SMC_EXEC 4
extern "C" INT WINAPI Shell_GetCachedImageIndex(LPCWSTR szPath, INT nIndex, UINT bSimulateDoc);

extern "C" HRESULT WINAPI CStartMenu_Constructor(REFIID riid, void **ppv);
extern "C" HRESULT WINAPI CMenuDeskBar_Constructor(REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuSite_Constructor(REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuBand_Constructor(REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuDeskBar_Wrapper(IDeskBar * db, REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuSite_Wrapper(IBandSite * bs, REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMenuBand_Wrapper(IShellMenu * sm, REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CMergedFolder_Constructor(IShellFolder* userLocal, IShellFolder* allUsers, REFIID riid, LPVOID *ppv);
extern "C" HRESULT WINAPI CStartMenuSite_Wrapper(ITrayPriv * trayPriv, REFIID riid, LPVOID *ppv);

static __inline ULONG
Win32DbgPrint(const char *filename, int line, const char *lpFormat, ...)
{
    char szMsg[512];
    char *szMsgStart;
    const char *fname;
    va_list vl;
    ULONG uRet;

    fname = strrchr(filename, '\\');
    if (fname == NULL)
    {
        fname = strrchr(filename, '/');
    }

    if (fname == NULL)
        fname = filename;
    else
        fname++;

    szMsgStart = szMsg + sprintf(szMsg, "[%10lu] %s:%d: ", GetTickCount(), fname, line);

    va_start(vl, lpFormat);
    uRet = (ULONG) vsprintf(szMsgStart, lpFormat, vl);
    va_end(vl);

    OutputDebugStringA(szMsg);

    return uRet;
}

#define DbgPrint(fmt, ...) \
    Win32DbgPrint(__FILE__, __LINE__, fmt, ##__VA_ARGS__)

#if 1
#define FAILED_UNEXPECTEDLY(hr) (FAILED(hr) && (DbgPrint("Unexpected failure %08x.\n", hr), TRUE))
#else
#define FAILED_UNEXPECTEDLY(hr) FAILED(hr)
#endif


template <class Base>
class CComDebugObject : public Base
{
public:
    CComDebugObject(void * = NULL)
    {
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
        DbgPrint("RefCount is now %d(++)!\n", rc);
        return rc;
    }

    STDMETHOD_(ULONG, Release)()
    {
        ULONG								newRefCount;

        newRefCount = this->InternalRelease();
        DbgPrint("RefCount is now %d(--)!\n", newRefCount);
        if (newRefCount == 0)
            delete this;
        return newRefCount;
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
        ATLTRY(newInstance = new CComDebugObject<Base>())
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
