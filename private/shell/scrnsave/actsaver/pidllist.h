/////////////////////////////////////////////////////////////////////////////
// PIDLLIST.H
//
// Declaration of CPIDLList.
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     12/09/96    Created
/////////////////////////////////////////////////////////////////////////////
#ifndef __PIDLLIST_H__
#define __PIDLLIST_H__

#include "chanmgr.h"

/////////////////////////////////////////////////////////////////////////////
// CPIDLList
/////////////////////////////////////////////////////////////////////////////
class CPIDLList
{
// Construction/Destruction
public:
    CPIDLList();
    virtual ~CPIDLList();

public:
    int Build(IScreenSaverConfig * pSSConfig, BOOL bDefaultTopLevelURL);
    void Free();

    BOOL Add(LPITEMIDLIST pidl);
    BOOL Add(LPTSTR pszURL);

    static int WINAPI ComparePIDLs(LPVOID pv1, LPVOID pv2, LPARAM lParam);
    static int WINAPI ComparePIDLNames(LPVOID pv1, LPVOID pv2, LPARAM lParam);

    int Count()
    { return ((m_hdpaList != NULL) ? DPA_GetPtrCount(m_hdpaList) : 0); }

    LPITEMIDLIST Item(int iItem)
    { return (LPITEMIDLIST)DPA_GetPtr(m_hdpaList, iItem); }

    int Find(LPITEMIDLIST pidl)
    { return ((m_hdpaList != NULL) ? DPA_Search(m_hdpaList, pidl, 0, CPIDLList::ComparePIDLs, NULL, 0) : -1); }

// Data
protected:
    IShellFolder *  m_psfInternet;
    HDPA            m_hdpaList;
};

/////////////////////////////////////////////////////////////////////////////
// Structs
/////////////////////////////////////////////////////////////////////////////
#pragma pack(1)
struct IDREGITEM
{
    WORD    cb;
    BYTE    bFlags;
    BYTE    bReserved;      // This is to get DWORD alignment
    CLSID   clsid;
};

struct IDLREGITEM
{
    IDREGITEM   idri;
    USHORT      cbNext;
};
#pragma pack()

/////////////////////////////////////////////////////////////////////////////
// Helper functions
/////////////////////////////////////////////////////////////////////////////
HRESULT FindXMLScreenSaverItem(IXMLElement * pXMLElement, CString & strSSURL, BOOL bUseTopLevelURL);
HRESULT GetCDFScreenSaverInfo(LPTSTR pszCDFURL, BOOL bUseTopLevelURL, CString & strSSURL, TASK_TRIGGER * pTaskTrigger);

typedef BOOL (CALLBACK * CHANENUMCALLBACK)(ISubscriptionMgr2 * pSubscriptionMgr2, CHANNELENUMINFO * pci, int nItemNum, BOOL bDefaultTopLevelURL, LPARAM lParam);
int     EnumChannels(CHANENUMCALLBACK pfnec, BOOL bDefaultTopLevelURL, LPARAM lParam);

HRESULT GetPSFInternet(IShellFolder * psfInternet);
BOOL    CreatePIDLFromPath(LPTSTR pszPath, LPITEMIDLIST * ppidl);
BOOL    GetPIDLDisplayName(IShellFolder * psf, LPCITEMIDLIST pidl, LPTSTR pszName, int cbszName, int fType);
LPITEMIDLIST CopyPIDL(LPITEMIDLIST pidl);

enum { PROP_CHANNEL_FLAGS };
extern const WCHAR g_szChannelFlags[];
extern const TCHAR g_szScreenSaverURL[];
extern const TCHAR g_szDesktopINI[];
extern const TCHAR g_szChannel[];
extern const LPCWSTR g_pProps[1];

#endif  // __PIDLLIST_H__
