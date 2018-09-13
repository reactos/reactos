#include "pch.h"
#pragma hdrstop

#include "mapfile.h"

//-----------------------------------------------------------------------------
// MappedFile
//
// A simple encapsulation of opening a mapped file in memory.
// The file is opened with READ access only.
// Client calls Base() to retrieve the base pointer of the mapped file.
//-----------------------------------------------------------------------------
MappedFile::MappedFile(
    VOID
    ) : m_hFile(INVALID_HANDLE_VALUE),
        m_hFileMapping(INVALID_HANDLE_VALUE),
        m_pbBase(NULL),
        m_llSize(0) 
{ 
    DBGTRACE((DM_MAPFILE, DL_HIGH, TEXT("MappedFile::MappedFile")));
}


MappedFile::~MappedFile(
    VOID
    )
{
    DBGTRACE((DM_MAPFILE, DL_HIGH, TEXT("MappedFile::~MappedFile")));
    Close();
}


LONGLONG
MappedFile::Size(
    VOID
    ) const
{
    DBGTRACE((DM_MAPFILE, DL_MID, TEXT("MappedFile::Size")));
    return m_llSize;
}



//
// Open the file.  Caller retrieves the base pointer through the
// Base() member function.
//
HRESULT
MappedFile::Open(
    LPCTSTR pszFile
    )
{
    DBGTRACE((DM_MAPFILE, DL_HIGH, TEXT("MappedFile::Open")));
    DBGPRINT((DM_MAPFILE, DL_HIGH, TEXT("\topening \"%s\""), pszFile));

    HRESULT hr = NO_ERROR;

    m_hFile = CreateFile(pszFile, 
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL);

    if (INVALID_HANDLE_VALUE == m_hFile)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        if ((m_hFileMapping = CreateFileMapping(m_hFile,
                                                NULL,
                                                PAGE_READONLY,
                                                0,
                                                0,
                                                NULL)) == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            m_pbBase = (LPBYTE)MapViewOfFile(m_hFileMapping,
                                             FILE_MAP_READ,
                                             0,
                                             0,
                                             0);
            if (NULL == m_pbBase)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else
            {
                ULARGE_INTEGER liSize;
                liSize.LowPart = GetFileSize(m_hFile, &liSize.HighPart);
                m_llSize = liSize.QuadPart;
            }
        }
    }
    return hr;
}

//
// Close the file mapping and the file.
//
VOID
MappedFile::Close(
    VOID
    )
{
    DBGTRACE((DM_MAPFILE, DL_HIGH, TEXT("MappedFile::Close")));
    if (NULL != m_pbBase)
    {
        UnmapViewOfFile(m_pbBase);
        m_pbBase = NULL;
    }
    if (INVALID_HANDLE_VALUE != m_hFileMapping)
    {
        CloseHandle(m_hFileMapping);
        m_hFileMapping = INVALID_HANDLE_VALUE;
    }
    if (INVALID_HANDLE_VALUE != m_hFile)
    {
        CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
    }
}

