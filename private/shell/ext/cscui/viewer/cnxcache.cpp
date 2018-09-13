//////////////////////////////////////////////////////////////////////////////
/*  File: cnxcache.cpp

    Description: Cache of network connection names and mapped drive letters.

        i.e. "\\worf\ntspecs",   'E'
             "\\rastaman\ntwin", 'F'

        Connected but unmapped shares have a letter value of TEXT('\0');

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/20/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include <lmcons.h>
#include <lmerr.h>
#include <winnetwk.h>
#include "cnxcache.h"


const int CnxNameCache::NUM_ENTRIES = 26;  // 1 entry for each ['A'..'Z']

CnxNameCache::CnxNameCache(
    void
    ) throw()
      : m_prgCnx(NULL)
{
    DBGTRACE((TEXT("CnxNameCache::CnxNameCache")));
}

CnxNameCache::~CnxNameCache(
    void
    )
{
    DBGTRACE((TEXT("CnxNameCache::~CnxNameCache")));
    delete[] m_prgCnx;
}

//
// Calling this will cause next call for data to refresh from the system.
//
void
CnxNameCache::Refresh(
    void
    ) throw()
{
    delete[] m_prgCnx;
    m_prgCnx = NULL;
}

void
CnxNameCache::Load(
    void
    )
{
    DBGTRACE((TEXT("CnxNameCache::Load")));
    delete[] m_prgCnx;
    m_prgCnx = NULL;
    m_prgCnx = new CString[NUM_ENTRIES];

    DBGASSERT((TEXT('Z') - TEXT('A') + 1 == NUM_ENTRIES));
    //
    // Get the mapped share name (if it exists) for each of the 26
    // characters in the alphabet.
    //
    TCHAR szRemote[MAX_PATH] = { TEXT('\0') };
    TCHAR szLocal[] = TEXT("?:");
    ULONG cchRemote = ARRAYSIZE(szRemote);
    for (TCHAR ch = TEXT('A'); ch <= TEXT('Z'); ch++)
    {
        szRemote[0] = TEXT('\0');
        szLocal[0]  = ch;
        DWORD dwResult = WNetGetConnection(szLocal, szRemote, &cchRemote);
        if (NERR_Success != dwResult && ERROR_NOT_CONNECTED != dwResult)
        {
            szRemote[0] = TEXT('\0');
            DBGERROR((TEXT("WNetGetConnection failed with error %d"), dwResult));
        }
        m_prgCnx[ch - TEXT('A')] = szRemote;
    }
}

//
// Retrieve the drive letter for a given "\\server\share" string.
// Returns TEXT('\0') if the share isn't in the cache.
//
TCHAR
CnxNameCache::GetShareDriveLetter(
    const CString& strShare
    ) const
{
    if (!Loaded())
        const_cast<CnxNameCache *>(this)->Load();

    if (Loaded())
    {
        for (int i = 0; i < NUM_ENTRIES; i++)
        {
            if (m_prgCnx[i] == strShare)
            {
                //
                // Share found.  Return drive letter.
                //
                return TEXT('A') + i;
            }
        }
    }
    return TEXT('\0');  // Share not found in cache.
}

//
// Retrieve the share name for a particular drive letter.
//
bool
CnxNameCache::GetShareFromDriveLetter(
    TCHAR chDrive,
    CString *pstrShare
    ) const
{
    DBGASSERT((NULL != pstrShare));
    DBGASSERT(( chDrive >= TEXT('A') && chDrive <= TEXT('Z') ));

    if (!Loaded())
        const_cast<CnxNameCache *>(this)->Load();

    if (Loaded())
    {
        pstrShare->Empty();
        *pstrShare = m_prgCnx[chDrive - TEXT('A')];
        return !pstrShare->IsEmpty();
    }
    return false;
}

