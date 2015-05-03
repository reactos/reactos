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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winnls.h>
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

    virtual ~CComCriticalSection()
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
};

extern CAtlBaseModule _AtlBaseModule;

}; // namespace ATL
