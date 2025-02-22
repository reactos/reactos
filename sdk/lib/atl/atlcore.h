/*
 * ReactOS ATL
 *
 * Copyright 2009 Andrew Hill <ash77@reactos.org>
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

#pragma once

#include <malloc.h>

#ifdef __REACTOS__
    #define WIN32_NO_STATUS
    #define _INC_WINDOWS
    #define COM_NO_WINDOWS_H
    #include <stdarg.h>
    #include <windef.h>
    #include <winbase.h>
    #include <winreg.h>
    #include <winnls.h>
#else
    #include <windows.h>
#endif
#include <ole2.h>
#include <olectl.h>
#include <crtdbg.h>

#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif // ATLASSERT

namespace ATL
{

class CComCriticalSection
{
public:
    CRITICAL_SECTION m_sec;
public:
    CComCriticalSection()
    {
        memset(&m_sec, 0, sizeof(CRITICAL_SECTION));
    }

    ~CComCriticalSection()
    {
    }

    HRESULT Lock()
    {
        EnterCriticalSection(&m_sec);
        return S_OK;
    }

    HRESULT Unlock()
    {
        LeaveCriticalSection(&m_sec);
        return S_OK;
    }

    HRESULT Init()
    {
        InitializeCriticalSection(&m_sec);
        return S_OK;
    }

    HRESULT Term()
    {
        DeleteCriticalSection(&m_sec);
        return S_OK;
    }
};

class CComFakeCriticalSection
{
public:
    HRESULT Lock()
    {
        return S_OK;
    }

    HRESULT Unlock()
    {
        return S_OK;
    }

    HRESULT Init()
    {
        return S_OK;
    }

    HRESULT Term()
    {
        return S_OK;
    }
};

class CComAutoCriticalSection : public CComCriticalSection
{
public:
    CComAutoCriticalSection()
    {
        HRESULT hResult __MINGW_ATTRIB_UNUSED;

        hResult = CComCriticalSection::Init();
        ATLASSERT(SUCCEEDED(hResult));
    }
    ~CComAutoCriticalSection()
    {
        CComCriticalSection::Term();
    }
};

class CComSafeDeleteCriticalSection : public CComCriticalSection
{
private:
    bool m_bInitialized;
public:
    CComSafeDeleteCriticalSection()
    {
        m_bInitialized = false;
    }

    ~CComSafeDeleteCriticalSection()
    {
        Term();
    }

    HRESULT Lock()
    {
        ATLASSERT(m_bInitialized);
        return CComCriticalSection::Lock();
    }

    HRESULT Init()
    {
        HRESULT hResult;

        ATLASSERT(!m_bInitialized);
        hResult = CComCriticalSection::Init();
        if (SUCCEEDED(hResult))
            m_bInitialized = true;
        return hResult;
    }

    HRESULT Term()
    {
        if (!m_bInitialized)
            return S_OK;
        m_bInitialized = false;
        return CComCriticalSection::Term();
    }
};

class CComAutoDeleteCriticalSection : public CComSafeDeleteCriticalSection
{
private:
    // CComAutoDeleteCriticalSection::Term should never be called
    HRESULT Term();
};

struct _ATL_BASE_MODULE70
{
    UINT cbSize;
    HINSTANCE m_hInst;
    HINSTANCE m_hInstResource;
    bool m_bNT5orWin98;
    DWORD dwAtlBuildVer;
    GUID *pguidVer;
    CRITICAL_SECTION m_csResource;
#ifdef NOTYET
    CSimpleArray<HINSTANCE> m_rgResourceInstance;
#endif
};
typedef _ATL_BASE_MODULE70 _ATL_BASE_MODULE;

class CAtlBaseModule : public _ATL_BASE_MODULE
{
public :
    static bool m_bInitFailed;
public:
    CAtlBaseModule()
    {
        cbSize = sizeof(_ATL_BASE_MODULE);
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)this, &m_hInst);
        m_hInstResource = m_hInst;
    }

    HINSTANCE GetModuleInstance()
    {
        return m_hInst;
    }

    HINSTANCE GetResourceInstance()
    {
        return m_hInstResource;
    }

    HINSTANCE SetResourceInstance(HINSTANCE hInst)
    {
        return static_cast< HINSTANCE >(InterlockedExchangePointer((void**)&m_hInstResource, hInst));
    }

    HINSTANCE GetHInstanceAt(int i);
};

__declspec(selectany) CAtlBaseModule _AtlBaseModule;
__declspec(selectany) bool CAtlBaseModule::m_bInitFailed = false;


///
// String Resource helper functions
//
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4200)
#endif
struct ATLSTRINGRESOURCEIMAGE
{
    WORD nLength;
    WCHAR achString[];
};
#ifdef _MSC_VER
#pragma warning(pop)
#endif

inline const ATLSTRINGRESOURCEIMAGE* _AtlGetStringResourceImage(
    _In_ HINSTANCE hInstance,
    _In_ HRSRC hResource,
    _In_ UINT id)
{
    const ATLSTRINGRESOURCEIMAGE* pImage;
    const ATLSTRINGRESOURCEIMAGE* pImageEnd;
    ULONG nResourceSize;
    HGLOBAL hGlobal;
    UINT iIndex;

    hGlobal = ::LoadResource(hInstance, hResource);
    if (hGlobal == NULL) return NULL;

    pImage = (const ATLSTRINGRESOURCEIMAGE*)::LockResource(hGlobal);
    if (pImage == NULL) return NULL;

    nResourceSize = ::SizeofResource(hInstance, hResource);
    pImageEnd = (const ATLSTRINGRESOURCEIMAGE*)(LPBYTE(pImage) + nResourceSize);
    iIndex = id & 0x000f;

    while ((iIndex > 0) && (pImage < pImageEnd))
    {
        pImage = (const ATLSTRINGRESOURCEIMAGE*)(LPBYTE(pImage) + (sizeof(ATLSTRINGRESOURCEIMAGE) + (pImage->nLength * sizeof(WCHAR))));
        iIndex--;
    }

    if (pImage >= pImageEnd) return NULL;
    if (pImage->nLength == 0) return NULL;

    return pImage;
}

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage(
    _In_ HINSTANCE hInstance,
    _In_ UINT id) noexcept
{
    HRSRC hResource;
    hResource = ::FindResourceW(hInstance, MAKEINTRESOURCEW((((id >> 4) + 1) & static_cast<WORD>(~0))), (LPWSTR)RT_STRING);
    if (hResource == NULL) return NULL;
    return _AtlGetStringResourceImage(hInstance, hResource, id);
}

inline const ATLSTRINGRESOURCEIMAGE* AtlGetStringResourceImage(
    _In_ HINSTANCE hInstance,
    _In_ UINT id,
    _In_ WORD wLanguage)
{
    HRSRC hResource;
    hResource = ::FindResourceExW(hInstance, (LPWSTR)RT_STRING, MAKEINTRESOURCEW((((id >> 4) + 1) & static_cast<WORD>(~0))), wLanguage);
    if (hResource == NULL) return NULL;
    return _AtlGetStringResourceImage(hInstance, hResource, id);
}

inline HINSTANCE AtlFindStringResourceInstance(
    UINT nID,
    WORD wLanguage = 0)
{
    const ATLSTRINGRESOURCEIMAGE* strRes = NULL;
    HINSTANCE hInst = _AtlBaseModule.GetHInstanceAt(0);

    for (int i = 1; hInst != NULL && strRes == NULL; hInst = _AtlBaseModule.GetHInstanceAt(i++))
    {
        strRes = AtlGetStringResourceImage(hInst, nID, wLanguage);
        if (strRes != NULL) return hInst;
    }

    return NULL;
}

}; // namespace ATL
