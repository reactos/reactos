
#ifndef SHFOLDER_H
#define SHFOLDER_H


class CShellFolder : public IShellFolder, public IPersistFolder
{
friend class CShellView;

protected:
    LONG m_ObjRefCount;

public:
    CShellFolder(CShellFolder*, LPCITEMIDLIST);
    ~CShellFolder();

    //
    // IUnknown methods
    //

    STDMETHOD (QueryInterface) (REFIID riid, LPVOID * ppvObj);
    STDMETHOD_ (ULONG, AddRef) (void);
    STDMETHOD_ (ULONG, Release) (void);

    //
    // IPersist methods
    //

    STDMETHODIMP GetClassID(LPCLSID);

    //
    // IPersistFolder methods
    //

    STDMETHODIMP Initialize(LPCITEMIDLIST);

    //
    // IShellFolder methods
    //

    STDMETHOD (ParseDisplayName) (HWND, LPBC, LPOLESTR, LPDWORD, LPITEMIDLIST*, LPDWORD);
    STDMETHOD (EnumObjects) (HWND, DWORD, LPENUMIDLIST*);
    STDMETHOD (BindToObject) (LPCITEMIDLIST, LPBC, REFIID, LPVOID*);
    STDMETHOD (BindToStorage) (LPCITEMIDLIST, LPBC, REFIID, LPVOID*);
    STDMETHOD (CompareIDs) (LPARAM, LPCITEMIDLIST, LPCITEMIDLIST);
    STDMETHOD (CreateViewObject) (HWND, REFIID, LPVOID* );
    STDMETHOD (GetAttributesOf) (UINT, LPCITEMIDLIST*, ULONG *);
    STDMETHOD (GetUIObjectOf) (HWND, UINT, LPCITEMIDLIST*, REFIID, LPUINT, LPVOID*);
    STDMETHOD (GetDisplayNameOf) (LPCITEMIDLIST, DWORD, LPSTRRET);
    STDMETHOD (SetNameOf) (HWND, LPCITEMIDLIST, LPCOLESTR, DWORD, LPITEMIDLIST*);

private:
    LPITEMIDLIST m_pidl;
    CShellFolder *m_pSFParent;
    LPMALLOC m_pMalloc;

    //
    // utility functions
    //

	BOOL HasSubFolders(LPCITEMIDLIST pidl);
    BOOL GetPidlFullText(LPCITEMIDLIST, LPTSTR, DWORD);
};

#endif   // SHFOLDER_H
