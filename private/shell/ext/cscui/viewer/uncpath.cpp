#include "pch.h"
#pragma hdrstop

#include "uncpath.h"

//
// BUGBUG:  Integrate this (or replace it) with the new CPath stuff
//          in pathstr.cpp.  
//

//-----------------------------------------------------------------------------
// class UNCPath
//-----------------------------------------------------------------------------
//
// UNCPath ctor.
// Initialize a UNCPath object with a UNC path string. 
// (i.e. "\\worf\ntspecs" or "\\worf\ntspecs\myfiles\foo.doc")
//
UNCPath::UNCPath(
    LPCTSTR pszUNCPath
    )
{
    *this = pszUNCPath;
}


//
// Retrieve the UNCPath string formatted as "\\worf\ntspecs" or
// "\\worf\ntspecs\myfiles\foo.doc".  Depends on what the object
// was initialized with.
//
void
UNCPath::GetFullPath(
    CString *pstrPath
    ) const
{
    DBGASSERT((NULL != pstrPath));

    if (m_strServer.IsEmpty() || m_strShare.IsEmpty())
    {
        pstrPath->Empty();
    }
    else
    {
        //
        // Add the basic "\\server\share" part.
        //
        pstrPath->Format(TEXT("\\\\%1\\%2"), (LPCTSTR)m_strServer, (LPCTSTR)m_strShare);

        if (!m_strPath.IsEmpty())
        {
            //
            // There's a path string.
            //
            *pstrPath += m_strPath;
            if (!m_strFile.IsEmpty())
            {
                //
                // There's also a file name.
                //
                *pstrPath += CString(TEXT("\\"));
                *pstrPath += m_strFile;
            }
        }
    }
}

UNCPath& 
UNCPath::operator = (
    LPCTSTR pszUNCPath
    )
{
    m_strServer.Empty();
    m_strShare.Empty();
    m_strPath.Empty();
    m_strFile.Empty();

    if (TEXT('\\') == *pszUNCPath &&
        TEXT('\\') == *(pszUNCPath + 1))
    {
        TCHAR chSaved   = TEXT('\0');
        LPTSTR pszStart = (LPTSTR)pszUNCPath + 2;
        LPTSTR pszEnd;

        //
        // Get the server name.
        //
        for (pszEnd = pszStart; *pszEnd && TEXT('\\') != *pszEnd; pszEnd = CharNext(pszEnd))
            NULL;
            
        SWAP(chSaved, *pszEnd);
        m_strServer = pszStart;
        SWAP(chSaved, *pszEnd);
        if (TEXT('\0') == *pszEnd)
            return *this;

        //
        // Get the share name.
        //
        pszStart = CharNext(pszEnd);
        for (pszEnd = pszStart; *pszEnd && TEXT('\\') != *pszEnd; pszEnd = CharNext(pszEnd))
            NULL;

        SWAP(chSaved, *pszEnd);
        m_strShare = pszStart;
        SWAP(chSaved, *pszEnd);
        if (TEXT('\0') == *pszEnd)
            return *this;

        //
        // Get the path string.
        //
        LPTSTR pszLastSlash = pszStart;
        for (pszStart = pszEnd; *pszEnd; pszEnd = CharNext(pszEnd))
        {
            if (TEXT('\\') == *pszEnd)
                pszLastSlash = pszEnd;
        }
        pszEnd = pszLastSlash;
        SWAP(chSaved, *pszEnd);
        m_strPath = pszStart;
        SWAP(chSaved, *pszEnd);
        if (TEXT('\0') == *pszEnd)
            return *this;

        //
        // Get the filename and extension.
        //
        pszStart = CharNext(pszEnd);
        m_strFile = pszStart;
    }
    return *this;
}


bool 
UNCPath::operator == (
    const UNCPath& rhs
    ) const
{
    return ((0 == m_strServer.CompareNoCase(rhs.m_strServer)) &&
            (0 == m_strShare.CompareNoCase(rhs.m_strShare)) &&
            (0 == m_strPath.CompareNoCase(rhs.m_strPath)) &&
            (0 == m_strFile.CompareNoCase(rhs.m_strFile)));
}

bool 
UNCPath::operator < (
    const UNCPath& rhs
    ) const
{
    CString s1, s2;
    GetFullPath(&s1);
    rhs.GetFullPath(&s2);
    return 0 > s1.CompareNoCase(s2);
}


