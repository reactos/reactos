#ifndef __CONTROL_FOLDER__
#define __CONTROL_FOLDER__

#include "general.h"

// forward declaration
class CControlFolder;
class CControlItem;

HRESULT CControlFolderEnum_CreateInstance(
                                      LPITEMIDLIST pidl,
                                      UINT shcontf,
                                      LPENUMIDLIST *ppeidl);

HRESULT CControlItem_CreateInstance(
                               CControlFolder *pCFolder,
                               UINT cidl, 
                               LPCITEMIDLIST *ppidl, 
                               REFIID riid, 
                               void **ppvOut);

HRESULT ControlFolderView_Command(HWND hwnd, UINT uID);

UINT MergeMenuHierarchy(
                    HMENU hmenuDst, 
                    HMENU hmenuSrc, 
                    UINT idcMin, 
                    UINT idcMax);

///////////////////////////////////////////////////////////////////////////////
// CControlFolder declaration

class CControlFolder : public IShellFolder, 
                       public IPersistFolder,
                       public IContextMenu
{
    // CControlFolder interfaces
    friend CControlItem;
    friend HRESULT ControlFolderView_CreateInstance(CControlFolder *pHCFolder, LPCITEMIDLIST pidl, void **ppvOut);
    friend HRESULT ControlFolderView_DidDragDrop(
                                            HWND hwnd, 
                                            IDataObject *pdo, 
                                            DWORD dwEffect);
        
public:
    CControlFolder();

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID,void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
   
    // IShellFolder methods
    STDMETHODIMP ParseDisplayName(
                             HWND hwndOwner, 
                             LPBC pbcReserved,
			                 LPOLESTR lpszDisplayName, 
                             ULONG *pchEaten,
			                 LPITEMIDLIST *ppidl, 
                             ULONG *pdwAttributes);

    STDMETHODIMP EnumObjects(
                        HWND hwndOwner, 
                        DWORD grfFlags,
			            LPENUMIDLIST *ppenumIDList);

    STDMETHODIMP BindToObject(
                          LPCITEMIDLIST pidl, 
                          LPBC pbcReserved,
			              REFIID riid, 
                          void **ppvOut);

    STDMETHODIMP BindToStorage(
                          LPCITEMIDLIST pidl, 
                          LPBC pbcReserved,
			              REFIID riid, 
                          void **ppvObj);

    STDMETHODIMP CompareIDs(
                        LPARAM lParam, 
                        LPCITEMIDLIST pidl1, 
                        LPCITEMIDLIST pidl2);

    STDMETHODIMP CreateViewObject(
                             HWND hwndOwner, 
                             REFIID riid, 
                             void **ppvOut);

    STDMETHODIMP GetAttributesOf(
                            UINT cidl, 
                            LPCITEMIDLIST *apidl,
			                ULONG *rgfInOut);

    STDMETHODIMP GetUIObjectOf(
                          HWND hwndOwner, 
                          UINT cidl, 
                          LPCITEMIDLIST *apidl,
			              REFIID riid, 
                          UINT *prgfInOut, 
                          void **ppvOut);

    STDMETHODIMP GetDisplayNameOf(
                          LPCITEMIDLIST pidl, 
                          DWORD uFlags, 
                          LPSTRRET lpName);

    STDMETHODIMP SetNameOf(
                      HWND hwndOwner, 
                      LPCITEMIDLIST pidl,
			          LPCOLESTR lpszName, 
                      DWORD uFlags, 
                      LPITEMIDLIST *ppidlOut);

    // IShellIcon Methods 
    STDMETHODIMP GetIconOf(LPCITEMIDLIST pidl, UINT flags, LPINT lpIconIndex);

    // IPersist Methods 
    STDMETHODIMP GetClassID(LPCLSID lpClassID);

    // IPersistFolder Methods
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IContextMenu Methods -- This handles the background context menus
    STDMETHODIMP QueryContextMenu(
                              HMENU hmenu, 
                              UINT indexMenu, 
                              UINT idCmdFirst,
                              UINT idCmdLast, 
                              UINT uFlags);

    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO lpici);

    STDMETHODIMP GetCommandString(
                              UINT_PTR idCmd, 
                              UINT uType,
                              UINT *pwReserved,
                              LPTSTR pszName, 
                              UINT cchMax);

protected:
    ~CControlFolder();

    UINT            m_cRef;
    LPITEMIDLIST    m_pidl;
    LPMALLOC        m_pMalloc;
};

#endif
