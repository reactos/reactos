/*****************************************************************************\
    FILE: isf.h

    DESCRIPTION:
        This is a base class that implements the default behavior of IShellFolder.
\*****************************************************************************/

#ifndef _DEFAULT_ISHELLFOLDER_H
#define _DEFAULT_ISHELLFOLDER_H

#include "cowsite.h"


/*****************************************************************************\
    CLASS: CBaseFolder

    DESCRIPTION:
        The stuff that tracks the state of a folder.

    The cBusy field tracks how many sub-objects have been created
    (e.g., IEnumIDList) which still contain references to this
    folder's identity.  You cannot change the folder's identity
    (via IPersistFolder::Initialize) while there are outstanding
    subobjects.

    The number of cBusy's never exceeds the number of cRef's, because
    each subobject that requires the folder identity must retain a
    reference to the folder itself.  That way, the folder won't be
    Release()d while the identity is still needed.

    Name Space description (for m_pidlComplete & m_nIDOffsetToOurNameSpaceRoot):
    The name space is provided by the shell to describe resources for the user.
    This class is a base implementation so users can create their own name space
    that is rooted in the shell's name space.  A PIDL is a list of ItemID, each of
    which represent one level in the name space.  The list provides a path thru
    the name space to a specific item.  Example:
    [Desktop][My Computer][C:\][Dir1][Dir2][File.htm][#goto_description_secion]
    [Desktop][The Internet][ftp://server/][Dir1][Dir2][file.txt]
    [Desktop][My Computer][PrivateNS lvl1][lvl2][lvl3]...
    (Public Name Space)   (Private Name Space)
    [GNS Level1][GNS Levl2][Pri LVL1][P LVL2][P LVL3]...

    In the example immediately above, this CBaseFolder can create a name space
    under "My Computer" that has 3 levels (lvl1, lvl2, lvl3).  An instance of this
    COM object will be positioned at one level of the sub name space (lvl1, lvl2, or lvl3).

    m_pidlComplete - is the list of ItemIDs from the base [Desktop] to the current location
                maybe lvl2.
    m_nIDOffsetToOurNameSpaceRoot - is the number of bytes of m_pidlComplete that you need
                to skip to get to the first ItemID in the private name space (which
                is the name space owned by this class).
\*****************************************************************************/

class CBaseFolder       : public IShellFolder2
                        , public IPersistFolder3
                        , public CObjectWithSite
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IShellFolder ***
    virtual STDMETHODIMP ParseDisplayName(HWND hwndOwner, LPBC pbcReserved, LPOLESTR lpszDisplayName,
                                            ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);
    virtual STDMETHODIMP EnumObjects(HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppenumIDList);
    virtual STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvOut);
    virtual STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    virtual STDMETHODIMP CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID * ppvOut);
    virtual STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl, ULONG * rgfInOut);
    virtual STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut);
    virtual STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    virtual STDMETHODIMP SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD uFlags, LPITEMIDLIST * ppidlOut);

    // *** IShellFolder2 ***
    virtual STDMETHODIMP GetDefaultSearchGUID(GUID *pguid) {return E_NOTIMPL;};
    virtual STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum) {return E_NOTIMPL;};
    virtual STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay) {return E_NOTIMPL;};
    virtual STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags) {return E_NOTIMPL;};
    virtual STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv) {return E_NOTIMPL;};
    virtual STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd) {return E_NOTIMPL;};
    virtual STDMETHODIMP MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid) {return E_NOTIMPL;};

    // *** IPersist ***
    virtual STDMETHODIMP GetClassID(LPCLSID lpClassID);

    // *** IPersistFolder ***
    virtual STDMETHODIMP Initialize(LPCITEMIDLIST pidl);
    
    // *** IPersistFolder2 ***
    virtual STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);

    // *** IPersistFolder3 ***
    virtual STDMETHODIMP InitializeEx(IBindCtx *pbc, LPCITEMIDLIST pidlRoot, const PERSIST_FOLDER_TARGET_INFO *ppfti);
    virtual STDMETHODIMP GetFolderTargetInfo(PERSIST_FOLDER_TARGET_INFO *ppfti);

public:
    CBaseFolder(LPCLSID pClassID);
    virtual ~CBaseFolder(void);

    // Public Member Functions
    virtual HRESULT _GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, LPVOID * ppvOut, BOOL fFromCreateViewObject);
    virtual HRESULT _Initialize(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlAliasRoot, int nBytesToPrivate);
    virtual HRESULT _CreateShellView(HWND hwndOwner, void ** ppvObj) = 0;       // PURE
    virtual HRESULT _CreateShellView(HWND hwndOwner, void ** ppvObj, LONG lEvents, FOLDERVIEWMODE fvm, IShellFolderViewCB * psfvCallBack, 
                            LPCITEMIDLIST pidl, LPFNVIEWCALLBACK pfnCallback);

    LPCITEMIDLIST GetPublicTargetPidlReference(void) { return m_pidl;};
    LPITEMIDLIST GetPublicTargetPidlClone(void) { return ILClone(GetPublicTargetPidlReference());};
    LPCITEMIDLIST GetPublicRootPidlReference(void) { return (m_pidlRoot ? m_pidlRoot : m_pidl);};
    LPITEMIDLIST GetPublicRootPidlClone(void) { return ILClone(GetPublicRootPidlReference());};
    LPCITEMIDLIST GetPrivatePidlReference(void);
    LPITEMIDLIST GetPrivatePidlClone(void) { return ILClone(GetPrivatePidlReference());};
    LPITEMIDLIST GetPublicPidlRootIDClone(void);
    LPITEMIDLIST CreateFullPublicPidlFromRelative(LPCITEMIDLIST pidlPrivateSubPidl);
    LPITEMIDLIST CreateFullPrivatePidl(LPCITEMIDLIST pidlPrivateSubPidl);
    LPITEMIDLIST CreateFullPublicPidl(LPCITEMIDLIST pidlPrivatePidl);

    LPCITEMIDLIST GetFolderPidl(void) { return m_pidlRoot;};

protected:
    int                     m_cRef;

    int GetPidlByteOffset(void) { return m_nIDOffsetToPrivate;};

private:
    LPITEMIDLIST            m_pidl;                 // Public Pidl - Complete list of IDs from very base of NameSpace to this name space and into this name space to the point of being rooted.
    LPITEMIDLIST            m_pidlRoot;             // Pidl of Folder Shortcut.
    int                     m_nIDOffsetToPrivate;   // number of bytes from the start of m_pidlComplete to the first ItemID in our name space.
    LPCLSID                 m_pClassID;             // My CLSID
};

#endif // _DEFAULT_ISHELLFOLDER_H
