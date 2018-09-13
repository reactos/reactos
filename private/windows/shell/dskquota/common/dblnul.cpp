#include "pch.h"
#pragma hdrstop

#include "dblnul.h"

//
// Use of LMEM_ZEROINIT keeps us from having to add nul terminators.
//

bool
DblNulTermList::AddString(
    LPCTSTR psz,            // String to copy.
    int cch                 // Length of psz in chars (excl nul term).
    )
{
    while((m_cchAlloc - m_cchUsed) < (cch + 2))
    {
        if (!Grow())
            return false;
    }
    DBGASSERT((NULL != m_psz));
    if (NULL != m_psz)
    {
        lstrcpy(m_psz + m_cchUsed, psz);
        m_cchUsed += cch + 1;
        m_cStrings++;
        return true;
    }
    return false;
}



bool
DblNulTermList::Grow(
    void
    )
{
    DBGASSERT((NULL != m_psz));
    DBGASSERT((m_cchGrow > 0));
    int cb = (m_cchAlloc + m_cchGrow) * sizeof(TCHAR);
    LPTSTR p = new TCHAR[cb];
    if (NULL != p && NULL != m_psz)
    {
        ZeroMemory(p, cb);
        CopyMemory(p, m_psz, m_cchUsed * sizeof(TCHAR));
        delete[] m_psz;
        m_psz = p;
    }
    if (NULL != m_psz)
    {
        m_cchAlloc += m_cchGrow;
    }

    return NULL != m_psz;
}


#if DBG
void 
DblNulTermList::Dump(
    void
    ) const
{
    DBGERROR((TEXT("Dumping nul term list iter -------------")));
    DblNulTermListIter iter = CreateIterator();
    LPCTSTR psz;
    while(iter.Next(&psz))
        DBGERROR((TEXT("%s"), psz ? psz : TEXT("<null>")));
}

#endif


bool
DblNulTermListIter::Next(
    LPCTSTR *ppszItem
    )
{
    if (*m_pszCurrent)
    {
        *ppszItem = m_pszCurrent;
        m_pszCurrent += lstrlen(m_pszCurrent) + 1;
        return true;
    }
    return false;
}