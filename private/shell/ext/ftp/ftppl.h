/*****************************************************************************
 *	ftppl.h
 *****************************************************************************/

#ifndef _FTPPIDLLIST_H
#define _FTPPIDLLIST_H


#include "ftplist.h"

typedef HRESULT (CALLBACK *LPFNPROCESSITEMCB)(LPVOID pfnProcessItemCB, HINTERNET hint, LPCITEMIDLIST pidlFull, BOOL * pfValidhinst, LPVOID pvData);

HRESULT EnumFolder(LPFNPROCESSITEMCB pfnProcessItemCB, HINTERNET hint, LPCITEMIDLIST pidlFull, CWireEncoding * pwe, BOOL * pfValidhinst, LPVOID pvData);
int RecursiveProcessPidl(LPVOID pvPidl, LPVOID pvInetEnum);


/*****************************************************************************
 *
 *	CFtpPidlList
 *
 *****************************************************************************/

class CFtpPidlList      : public IUnknown
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

public:
    CFtpPidlList();
    ~CFtpPidlList(void);

    // Public Member Functions
    HRESULT CompareAndDeletePidl(LPCITEMIDLIST pidl);
    HRESULT ReplacePidl(LPCITEMIDLIST pidlSrc, LPCITEMIDLIST pidlDest);
    HRESULT InsertSorted(LPCITEMIDLIST pidl);
    HRESULT RecursiveEnum(LPCITEMIDLIST pidlRoot, LPFNPROCESSITEMCB pfnProcessItemCB, HINTERNET hint, LPVOID pvData);
    void Enum(PFNDPAENUMCALLBACK pfn, LPVOID pv) { m_pfl->Enum(pfn, pv); };
    void DeletePidl(LPITEMIDLIST pidl) { m_pfl->DeleteItemPtr((LPVOID) pidl);  ILFree(pidl); };
    void Delete(int nIndex);
    int GetCount(void)  { return m_pfl->GetCount(); };
    int FindPidlIndex(LPCITEMIDLIST pidlToFind, BOOL fCaseInsensitive);
    LPITEMIDLIST GetPidl(UINT ipv) { return (LPITEMIDLIST) m_pfl->GetItemPtr(ipv); };   // TODO: Rename GetPidlReference()
    LPITEMIDLIST FindPidl(LPCITEMIDLIST pidlToFind, BOOL fCaseInsensitive);
    BOOL AreAllFolders(void);
    BOOL AreAllFiles(void);

    LPCITEMIDLIST * GetPidlList(void);
    void FreePidlList(LPCITEMIDLIST * ppidl);
    void TraceDump(LPCITEMIDLIST pidl, LPCTSTR pszCaller);

    void UseCachedDirListings(BOOL fUseCachedDirListings);

    // Friend Functions
    static int CFtpPidlList::ComparePidlName(LPVOID pvPidl1, LPVOID pvPidl2, LPARAM lParam);
    friend HRESULT CFtpPidlList_Create(int cpidl, LPCITEMIDLIST rgpidl[], CFtpPidlList ** ppfl);
    friend HRESULT _EnumFolderPrep(HINTERNET hint, LPCITEMIDLIST pidlFull, CFtpPidlList * pPidlList, CWireEncoding * pwe, LPITEMIDLIST * ppidlCurrFtpPath);

protected:
    // Private Member Variables
    int                     m_cRef;

    CFtpList *              m_pfl;
    CWireEncoding *         m_pwe;          // We don't hold a ref, so we assume the object will outlive us.
    DWORD                   m_dwInetFlags;  // What flags do we want to set for enumeration?

    // Private Member Functions
    HRESULT _Fill(int cpidl, LPCITEMIDLIST rgpidl[]);
    void AssertSorted(void);
};


#endif // _FTPPIDLLIST_H
