//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       strret.cpp
//
//--------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
/*  File: strret.cpp

    Description: Class to handle STRRET objects used in returning strings
        from the Windows Shell.  Main function is to properly clean up any
        dynamic storage.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "strret.h"


StrRet::StrRet(
    LPITEMIDLIST pidl,
    UINT type
    ) : m_pMalloc(NULL)
{
    uType = type;
    pOleStr = NULL;
    SHGetMalloc(&m_pMalloc);
    m_pidl = pidl;
}


StrRet::~StrRet(
    VOID
    )
{
    FreeOleStr();

    if (NULL != m_pMalloc)
    {
        m_pMalloc->Release();
    }
}


StrRet::StrRet(const StrRet& rhs)
{
    *this = rhs;
}

    

StrRet& StrRet::operator = (const StrRet& rhs)
{
    if (this != &rhs)
    {
        if (NULL != m_pMalloc)
            m_pMalloc->Release();

        m_pMalloc = rhs.m_pMalloc;
        if (NULL != m_pMalloc)
            m_pMalloc->AddRef();
        
        m_pidl = rhs.m_pidl;
        switch(rhs.uType)
        {
            case STRRET_WSTR:
                CopyOleStr(rhs.pOleStr);
                break;

            case STRRET_CSTR:
                lstrcpynA(cStr, rhs.cStr, ARRAYSIZE(cStr));
                break;

            case STRRET_OFFSET:
                uOffset = rhs.uOffset;
                break;
        }
    }
    return *this;
}

VOID
StrRet::FreeOleStr(
    VOID
    )
{
    if (NULL != m_pMalloc &&
        STRRET_WSTR == uType && 
        NULL != pOleStr)
    {
        m_pMalloc->Free(pOleStr);
        pOleStr = NULL;
    }
}

VOID
StrRet::CopyOleStr(
    LPCWSTR pszOle
    )
{
    FreeOleStr();
    pOleStr = new WCHAR[lstrlenW(pszOle) + 1];
    if (NULL != pOleStr)
    {
        lstrcpyW(pOleStr, pszOle);
    }
}


LPTSTR 
StrRet::GetString(
    LPTSTR pszStr, 
    INT cchStr
    ) const
{
    switch(uType)
    {
        case STRRET_WSTR:
#ifndef UNICODE
            WideCharToMultiByte(CP_ACP, 0, pOleStr, -1, pszStr, cchStr, NULL, NULL);
#else
            lstrcpyn(pszStr, pOleStr, cchStr);
#endif                                    
            break;

        case STRRET_CSTR:
#ifndef UNICODE
            lstrcpyn(pszStr, cStr, cchStr);
#else
            MultiByteToWideChar(CP_ACP, 0, cStr, -1, pszStr, cchStr);
#endif
            break;

        case STRRET_OFFSET:
            lstrcpyn(pszStr, (LPCTSTR)((LPBYTE)m_pidl + uOffset), cchStr);
            break;
    }
    return pszStr;
}


