//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       pathstr.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "pathstr.h"



CPath::CPath(
    LPCTSTR pszRoot, 
    LPCTSTR pszDir, 
    LPCTSTR pszFile, 
    LPCTSTR pszExt
    )
{
    if (pszDir)
        SetPath(pszDir);
    if (pszRoot)
        SetRoot(pszRoot);
    if (pszFile)
        SetFileSpec(pszFile);
    if (pszExt)
        SetExtension(pszExt);
}


CPath::CPath(
    const CPath& rhs
    ) : CString(rhs)
{

}

    
CPath& 
CPath::operator = (
    const CPath& rhs
    )
{
    if (this != &rhs)
    {
        CString::operator = (rhs);
    }
    return *this;
}


CPath& 
CPath::operator = (
    LPCTSTR rhs
    )
{
    CString::operator = (rhs);
    return *this;
}


void
CPath::AddBackslash(
    void
    )
{
    ::PathAddBackslash(GetBuffer(MAX(MAX_PATH, Length() + 2)));
    ReleaseBuffer();
}

void
CPath::RemoveBackslash(
    void
    )
{
    ::PathRemoveBackslash(GetBuffer());
    ReleaseBuffer();
}

bool 
CPath::GetRoot(
    CPath *pOut
    ) const
{
    CPath temp(*this);
    temp.StripToRoot();
    *pOut = temp;
    return 0 < pOut->Length();
}

bool 
CPath::GetPath(
    CPath *pOut
    ) const
{
    CPath temp(*this);
    temp.RemoveFileSpec();
    *pOut = temp;
    return 0 < pOut->Length();
}

bool
CPath::GetDirectory(
    CPath *pOut
    ) const
{
    if (GetPath(pOut))
        pOut->RemoveRoot();

    return 0 < pOut->Length();
}

bool 
CPath::GetExtension(
    CPath *pOut
    ) const
{
    *pOut = ::PathFindExtension(*this);
    return 0 < pOut->Length();
}


bool 
CPath::GetFileSpec(
    CPath *pOut
    ) const
{
    *pOut = ::PathFindFileName(*this);
    return 0 < pOut->Length();
}
    

bool 
CPath::Append(
    LPCTSTR psz
    )
{
    bool bResult = boolify(::PathAppend(GetBuffer(MAX(MAX_PATH, Length() + lstrlen(psz) + 3)), psz));
    ReleaseBuffer();
    return bResult;
}

bool 
CPath::BuildRoot(
    int iDrive
    )
{
    Empty();
    bool bResult = NULL != ::PathBuildRoot(GetBuffer(5), iDrive);
    ReleaseBuffer();
    return bResult;
}


bool 
CPath::Canonicalize(
    void
    )
{
    CString strTemp(*this);
    bool bResult = boolify(::PathCanonicalize(GetBuffer(MAX(MAX_PATH, Size())), strTemp));
    ReleaseBuffer();
    return bResult;
}



bool 
CPath::Compact(
    HDC hdc, 
    int cxPixels
    )
{
    bool bResult = boolify(::PathCompactPath(hdc, GetBuffer(), cxPixels));
    ReleaseBuffer();
    return bResult;
}


bool 
CPath::CommonPrefix(
    LPCTSTR pszPath1, 
    LPCTSTR pszPath2
    )
{
    Empty();
    ::PathCommonPrefix(pszPath1, 
                       pszPath2, 
                       GetBuffer(MAX(MAX_PATH, (MAX(lstrlen(pszPath1), lstrlen(pszPath2)) + 1))));
    ReleaseBuffer();
    return 0 < Length();
}


void
CPath::QuoteSpaces(
    void
    )
{
    ::PathQuoteSpaces(GetBuffer(MAX(MAX_PATH, Length() + 3)));
    ReleaseBuffer();
}

void 
CPath::UnquoteSpaces(
    void
    )
{
    ::PathUnquoteSpaces(GetBuffer());
    ReleaseBuffer();
}

void 
CPath::RemoveBlanks(
    void
    )
{
    ::PathRemoveBlanks(GetBuffer());
    ReleaseBuffer();
}

void
CPath::RemoveExtension(
    void
    )
{
    PathRemoveExtension(GetBuffer());
    ReleaseBuffer();
}

void
CPath::RemoveFileSpec(
    void
    )
{
    ::PathRemoveFileSpec(GetBuffer());
    ReleaseBuffer();
}

void
CPath::RemoveRoot(
    void
    )
{
    LPTSTR psz = ::PathSkipRoot(*this);
    if (psz)
    {
        CPath temp(psz);
        *this = temp; 
    }
}


void
CPath::RemovePath(
    void
    )
{
    CPath temp;
    GetFileSpec(&temp);
    *this = temp;
}


void
CPath::StripToRoot(
    void
    )
{
    ::PathStripToRoot(GetBuffer());
    ReleaseBuffer();
}


void 
CPath::SetRoot(
    LPCTSTR pszRoot
    )
{
    CPath strTemp(*this);
    strTemp.RemoveRoot();
    *this = pszRoot;
    Append(strTemp);
}

void
CPath::SetPath(
    LPCTSTR pszPath
    )
{
    CPath strTemp(*this);
    *this = pszPath;

    strTemp.RemovePath();
    Append(strTemp);
}

void
CPath::SetDirectory(
    LPCTSTR pszDir
    )
{
    CPath path;
    GetPath(&path);
    path.StripToRoot();
    path.AddBackslash();
    path.Append(pszDir);
    SetPath(path);
}


void
CPath::SetFileSpec(
    LPCTSTR pszFileSpec
    )
{
    RemoveFileSpec();
    Append(pszFileSpec);
}

void
CPath::SetExtension(
    LPCTSTR pszExt
    )
{
    ::PathRenameExtension(GetBuffer(MAX(MAX_PATH, Length() + lstrlen(pszExt) + 2)), pszExt);
    ReleaseBuffer();
}


CPathIter::CPathIter(
    const CPath& path
    ) : m_path(path),
        m_pszCurrent((LPTSTR)m_path.Cstr())
{
    //
    // Skip over leading whitespace and backslashes.
    //
    while(*m_pszCurrent &&
          (TEXT('\\') == *m_pszCurrent ||
           TEXT(' ') == *m_pszCurrent  ||
           TEXT('\t') == *m_pszCurrent ||
           TEXT('\n') == *m_pszCurrent))
    {
        m_pszCurrent++;
    }
}


bool
CPathIter::Next(
    CPath *pOut
    )
{
    DBGASSERT((NULL != pOut));

    LPCTSTR pszStart = m_pszCurrent;
    if (NULL == pszStart || TEXT('\0') == *pszStart)
        return false;

    TCHAR chTemp = TEXT('\0');
    m_pszCurrent = ::PathFindNextComponent(pszStart);
    if (NULL != m_pszCurrent && *m_pszCurrent)
        SWAP(*(m_pszCurrent - 1), chTemp);

    *pOut = pszStart;

    if (TEXT('\0') != chTemp)
        SWAP(*(m_pszCurrent - 1), chTemp);

    return true;
}


void
CPathIter::Reset(
    void
    )
{
    m_pszCurrent = (LPTSTR)m_path.Cstr();
}    
