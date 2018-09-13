#include "pch.h"
#pragma hdrstop

#include "except.h"


CExceptionString::CExceptionString(
    LPCTSTR pszText
    ) : m_pszText(NULL)
{
    m_pszText = Dup(pszText);
}


CExceptionString::CExceptionString(
    const CExceptionString& rhs
    ) : m_pszText(NULL)
{
    *this = rhs;
}


CExceptionString& 
CExceptionString::operator = (
    const CExceptionString& rhs
    )
{
    if (this != &rhs)
    {
        delete[] m_pszText;
        m_pszText = Dup(rhs.m_pszText);
    }
    return *this;
}

LPTSTR
CExceptionString::Dup(
    LPCTSTR psz
    )
{
    LPTSTR pszNew = new TCHAR[lstrlen(psz) + 1];
    lstrcpy(pszNew, psz);
    return pszNew;
}    


#if DBG
//
// Don't include these strings in free builds.  Since the strings
// aren't localized, they're intended for debug output only.
//
LPCTSTR CMemoryException::m_pszReasons[] = { TEXT("alloc"), 
                                             TEXT("overflow"),
                                             TEXT("index"), 
                                             TEXT("range"),
                                             TEXT("pointer") };

LPCTSTR CFileException::m_pszReasons[] = { TEXT("create"), 
                                           TEXT("read"),
                                           TEXT("write"),
                                           TEXT("diskfull"),
                                           TEXT("access"),
                                           TEXT("device") };

LPCTSTR CSyncException::m_pszReasons[] = { TEXT("create"), 
                                           TEXT("timeout"), 
                                           TEXT("abandoned") };

LPCTSTR CSyncException::m_pszObjects[] = { TEXT("mutex"),  
                                           TEXT("critsect"), 
                                           TEXT("semaphore"), 
                                           TEXT("event"), 
                                           TEXT("thread"), 
                                           TEXT("process") };

LPCTSTR CResourceException::m_pszReasons[] = { TEXT("accelerator"), 
                                               TEXT("anicursor"), 
                                               TEXT("aniicon"),
                                               TEXT("bitmap"),
                                               TEXT("cursor"),
                                               TEXT("dialog"),
                                               TEXT("font"),
                                               TEXT("fontdir"),
                                               TEXT("group_cursor"),
                                               TEXT("group_icon"),
                                               TEXT("icon"),
                                               TEXT("menu"),
                                               TEXT("messagetable"),
                                               TEXT("rcdata"),
                                               TEXT("string"),
                                               TEXT("version") };

LPCTSTR 
CResourceException::ReasonText(
    void
    ) const
{
    static TCHAR szMsg[MAX_PATH];
    wsprintf(szMsg, TEXT("%s (id: %d)"), m_pszReasons[Type()], m_uResId );
    return szMsg;
}

#endif // DBG

