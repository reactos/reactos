/*
* PROJECT:     ReactOS ATL
* LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
* PURPOSE:     ATL File implementation
* COPYRIGHT:   Copyright 2019 Mark Jansen (mark.jansen@reactos.org)
*/

#pragma once

#include <atlbase.h>

namespace ATL
{

//class CAtlFile:   TODO
//    public CHandle
//{
//};


//class CAtlTemporaryFile   TODO
//{
//};



class CAtlFileMappingBase
{
private:
    void* m_pData;
    SIZE_T m_nMappingSize;
    HANDLE m_hMapping;
    ULARGE_INTEGER m_nOffset;
    DWORD m_dwViewDesiredAccess;

public:
    CAtlFileMappingBase() throw()
        :m_pData(NULL)
        ,m_nMappingSize(0)
        ,m_hMapping(NULL)
        ,m_dwViewDesiredAccess(0)
    {
        m_nOffset.QuadPart = 0;
    }

    ~CAtlFileMappingBase() throw()
    {
        Unmap();
    }

    CAtlFileMappingBase(CAtlFileMappingBase& orig)
    {
        HRESULT hr;

        m_pData = NULL;
        m_nMappingSize = 0;
        m_hMapping = NULL;
        m_dwViewDesiredAccess = 0;
        m_nOffset.QuadPart = 0;

        hr = CopyFrom(orig);
        if (FAILED(hr))
            AtlThrow(hr);
    }

    CAtlFileMappingBase& operator=(CAtlFileMappingBase& orig)
    {
        HRESULT hr;

        hr = CopyFrom(orig);
        if (FAILED(hr))
            AtlThrow(hr);

        return *this;
    }

    HRESULT CopyFrom(CAtlFileMappingBase& orig) throw()
    {
        HRESULT hr = S_OK;

        if (&orig == this)
            return S_OK;

        ATLASSERT(m_pData == NULL);
        ATLASSERT(m_hMapping == NULL);
        ATLASSERT(orig.m_pData != NULL);

        m_nMappingSize = orig.m_nMappingSize;
        m_nOffset.QuadPart = orig.m_nOffset.QuadPart;
        m_dwViewDesiredAccess = orig.m_dwViewDesiredAccess;

        if (::DuplicateHandle(GetCurrentProcess(), orig.m_hMapping, GetCurrentProcess(), &m_hMapping, 0, TRUE, DUPLICATE_SAME_ACCESS))
        {
            m_pData = ::MapViewOfFile(m_hMapping, m_dwViewDesiredAccess, m_nOffset.HighPart, m_nOffset.LowPart, m_nMappingSize);
            if (!m_pData)
            {
                hr = AtlHresultFromLastError();
                ::CloseHandle(m_hMapping);
                m_hMapping = NULL;
            }
        }
        else
        {
            hr = AtlHresultFromLastError();
        }

        return hr;
    }

    HRESULT MapFile(
        HANDLE hFile,
        SIZE_T nMappingSize = 0,
        ULONGLONG nOffset = 0,
        DWORD dwMappingProtection = PAGE_READONLY,
        DWORD dwViewDesiredAccess = FILE_MAP_READ) throw()
    {
        HRESULT hr = S_OK;
        ULARGE_INTEGER FileSize;

        ATLASSERT(hFile != INVALID_HANDLE_VALUE);
        ATLASSERT(m_pData == NULL);
        ATLASSERT(m_hMapping == NULL);

        FileSize.LowPart = ::GetFileSize(hFile, &FileSize.HighPart);
        FileSize.QuadPart = nMappingSize > FileSize.QuadPart ? nMappingSize : FileSize.QuadPart;

        m_hMapping = ::CreateFileMapping(hFile, NULL, dwMappingProtection, FileSize.HighPart, FileSize.LowPart, 0);
        if (m_hMapping)
        {
            m_nMappingSize = nMappingSize == 0 ? (SIZE_T)(FileSize.QuadPart - nOffset) : nMappingSize;
            m_nOffset.QuadPart = nOffset;
            m_dwViewDesiredAccess = dwViewDesiredAccess;

            m_pData = ::MapViewOfFile(m_hMapping, m_dwViewDesiredAccess, m_nOffset.HighPart, m_nOffset.LowPart, m_nMappingSize);
            if (!m_pData)
            {
                hr = AtlHresultFromLastError();
                ::CloseHandle(m_hMapping);
                m_hMapping = NULL;
            }
        }
        else
        {
            hr = AtlHresultFromLastError();
        }

        return hr;
    }

    HRESULT MapSharedMem(
        SIZE_T nMappingSize,
        LPCTSTR szName,
        BOOL* pbAlreadyExisted = NULL,
        LPSECURITY_ATTRIBUTES lpsa = NULL,
        DWORD dwMappingProtection = PAGE_READWRITE,
        DWORD dwViewDesiredAccess = FILE_MAP_ALL_ACCESS) throw()
    {
        HRESULT hr = S_OK;
        ULARGE_INTEGER Size;

        ATLASSERT(nMappingSize > 0);
        ATLASSERT(szName != NULL);
        ATLASSERT(m_pData == NULL);
        ATLASSERT(m_hMapping == NULL);

        m_nMappingSize = nMappingSize;
        m_dwViewDesiredAccess = dwViewDesiredAccess;
        m_nOffset.QuadPart = 0;
        Size.QuadPart = nMappingSize;

        m_hMapping = ::CreateFileMapping(NULL, lpsa, dwMappingProtection, Size.HighPart, Size.LowPart, szName);
        if (m_hMapping != NULL)
        {
            if (pbAlreadyExisted)
                *pbAlreadyExisted = GetLastError() == ERROR_ALREADY_EXISTS;

            m_pData = ::MapViewOfFile(m_hMapping, dwViewDesiredAccess, m_nOffset.HighPart, m_nOffset.LowPart, m_nMappingSize);
            if (!m_pData)
            {
                hr = AtlHresultFromLastError();
                ::CloseHandle(m_hMapping);
                m_hMapping = NULL;
            }
        }
        else
        {
            hr = AtlHresultFromLastError();
        }

        return hr;
    }

    HRESULT OpenMapping(
        LPCTSTR szName,
        SIZE_T nMappingSize,
        ULONGLONG nOffset = 0,
        DWORD dwViewDesiredAccess = FILE_MAP_ALL_ACCESS) throw()
    {
        HRESULT hr = S_OK;

        ATLASSERT(szName != NULL);
        ATLASSERT(m_pData == NULL);
        ATLASSERT(m_hMapping == NULL);

        m_nMappingSize = nMappingSize;
        m_dwViewDesiredAccess = dwViewDesiredAccess;
        m_nOffset.QuadPart = nOffset;

        m_hMapping = ::OpenFileMapping(m_dwViewDesiredAccess, FALSE, szName);
        if (m_hMapping)
        {
            m_pData = ::MapViewOfFile(m_hMapping, dwViewDesiredAccess, m_nOffset.HighPart, m_nOffset.LowPart, m_nMappingSize);
            if (!m_pData)
            {
                hr = AtlHresultFromLastError();
                ::CloseHandle(m_hMapping);
                m_hMapping = NULL;
            }
        }
        else
        {
            hr = AtlHresultFromLastError();
        }

        return hr;
    }

    HRESULT Unmap() throw()
    {
        HRESULT hr = S_OK;

        if (m_pData)
        {
            if (!::UnmapViewOfFile(m_pData))
                hr = AtlHresultFromLastError();

            m_pData = NULL;
        }
        if (m_hMapping)
        {
            // If we already had an error, do not overwrite it
            if (!::CloseHandle(m_hMapping) && SUCCEEDED(hr))
                hr = AtlHresultFromLastError();

            m_hMapping = NULL;
        }

        return hr;
    }

    void* GetData() const throw()
    {
        return m_pData;
    }

    HANDLE GetHandle() throw ()
    {
        return m_hMapping;
    }

    SIZE_T GetMappingSize() throw()
    {
        return m_nMappingSize;
    }

};


template <typename T = char>
class CAtlFileMapping:
    public CAtlFileMappingBase
{
public:
    operator T*() const throw()
    {
        return reinterpret_cast<T*>(GetData());
    }
};


}
