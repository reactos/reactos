//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cscentry.h
//
//--------------------------------------------------------------------------

#ifndef __cscentry_h
#define __cscentry_h

#include <comctrlp.h>   // DPA
#include "util.h"       // LocalAllocString, LocalFreeString

///////////////////////////////////////////////////////////////////
// CSCEntry
// 
//
class CSCEntry
{
public:
    CSCEntry(LPCTSTR pszName, REFGUID rguid)
      : m_pszName(NULL), m_Guid(rguid)  { LocalAllocString(&m_pszName, pszName); }
    ~CSCEntry()                         { LocalFreeString(&m_pszName); }

    LPCTSTR Name() const                { return m_pszName; }
    REFGUID Guid() const                { return m_Guid; }

private:
    LPTSTR  m_pszName;                  // E.g. full pathname or sharename
    GUID    m_Guid;                     // GUID used to identify this entry
};

///////////////////////////////////////////////////////////////////
// CSCEntryLog
//
// 
class CSCEntryLog
{
public:
    CSCEntryLog(HKEY hkRoot, LPCTSTR pszSubkey);
    ~CSCEntryLog();

    // Access entries
    CSCEntry* Get(LPCTSTR pszName);
    CSCEntry* Get(REFGUID rguid);

    // Add Entries
    CSCEntry* Add(LPCTSTR pszName);     // Returns existing entry or creates new entry

    // Access Registry
    HKEY OpenKey(LPCTSTR pszSubkey, REGSAM samDesired);
    
private:    
    HKEY m_hkRoot;                      // KEY_ENUMERATE_SUB_KEYS | KEY_CREATE_SUB_KEY
    HDPA m_hdpa;                        // Holds the entry log in memory
    CRITICAL_SECTION m_csDPA;           // Protect access to m_hdpa

    HKEY OpenKeyInternal(LPTSTR pszSubkey, REGSAM samDesired);
    CSCEntry* CreateFromKey(LPTSTR pszSubkey);
    HRESULT ReadRegKeys();              // fills m_hdpa
};

#endif        
