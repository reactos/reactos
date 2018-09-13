//*******************************************************************************************
//
// Filename : Folder.h
//	
//				Definitions of CCabFolder and CCabItemList
//
// Copyright (c) 1994 - 1997 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#ifndef _CABFOLD_H_
#define _CABFOLD_H_

enum _CV_COLS
{
    CV_COL_NAME = 0,
    CV_COL_SIZE,
    CV_COL_TYPE,
    CV_COL_MODIFIED,
    CV_COL_PATH,
    CV_COL_MAX,
} ;

typedef struct _CABITEM
{
    WORD wSize;
    DWORD dwFileSize;
    USHORT uFileDate;
    USHORT uFileTime;
    USHORT uFileAttribs;
    USHORT cPathChars;
    TCHAR szName[1];
} CABITEM, *LPCABITEM;

class CCabItemList
{
public:
    CCabItemList(UINT uStep) {m_uStep=uStep;}
    CCabItemList() {CCabItemList(8);}
    ~CCabItemList() {CleanList();}
    
    enum
    {
        State_UnInit,
            State_Init,
            State_OutOfMem,
    };
    
    UINT GetState();
    
    LPCABITEM operator[](UINT nIndex)
    {
        return((LPCABITEM)DPA_GetPtr(m_dpaList, nIndex));
    }
    UINT GetCount() {return(GetState()==State_Init ? DPA_GetPtrCount(m_dpaList) : 0);}
    LPCABITEM* GetArray() {return(GetState()==State_Init ? (LPCABITEM*) DPA_GetPtrPtr(m_dpaList) : NULL);}
    
    BOOL InitList();
    
    BOOL AddItems(LPCABITEM *apit, UINT cpit);
    BOOL AddItem(LPCTSTR pszName, DWORD dwFileSize,
        UINT uFileDate, UINT uFileTime, UINT uFileAttribs);
    
    int FindInList(LPCTSTR pszName, DWORD dwFileSize,
        UINT uFileDate, UINT uFileTime, UINT uFileAttribs);
    BOOL IsInList(LPCTSTR pszName, DWORD dwFileSize,
        UINT uFileDate, UINT uFileTime, UINT uFileAttribs)
    {
        return(FindInList(pszName, dwFileSize, uFileDate, uFileTime, uFileAttribs) >= 0);
    }
    
    
private:
    BOOL StoreItem(LPITEMIDLIST pidl);
    void CleanList();
    
private:
    UINT m_uStep;
    HDPA m_dpaList;
} ;

class CCabFolder : public IPersistFolder2, public IShellFolder2
{
public:
    CCabFolder() : m_pidlHere(0), m_lItems(1024/sizeof(void *)) {}
    ~CCabFolder()
    {
        if (m_pidlHere)
        {
            ILFree(m_pidlHere);
        }
    }
    
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(
        REFIID riid, 
        void ** ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // *** IParseDisplayName method ***
    STDMETHODIMP ParseDisplayName(
        HWND hwndOwner,
        LPBC pbc, 
        LPOLESTR lpszDisplayName,
        ULONG * pchEaten, 
        LPITEMIDLIST * ppidl,
        ULONG *pdwAttributes);
    
    // *** IOleContainer methods ***
    STDMETHODIMP EnumObjects(
        HWND hwndOwner, 
        DWORD grfFlags,
        LPENUMIDLIST * ppenumIDList);
    
    // *** IShellFolder methods ***
    STDMETHODIMP BindToObject(
        LPCITEMIDLIST pidl, 
        LPBC pbc,
        REFIID riid, 
        void ** ppvObj);
    STDMETHODIMP BindToStorage(
        LPCITEMIDLIST pidl, 
        LPBC pbc,
        REFIID riid, 
        void ** ppvObj);
    STDMETHODIMP CompareIDs(
        LPARAM lParam, 
        LPCITEMIDLIST pidl1,
        LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject(
        HWND hwndOwner, 
        REFIID riid,
        void ** ppvObj);
    STDMETHODIMP GetAttributesOf(
        UINT cidl, 
        LPCITEMIDLIST * apidl,
        ULONG * rgfInOut);
    STDMETHODIMP GetUIObjectOf(
        HWND hwndOwner, 
        UINT cidl, 
        LPCITEMIDLIST * apidl, 
        REFIID riid, 
        UINT * prgfInOut, 
        void ** ppvObj);
    STDMETHODIMP GetDisplayNameOf(
        LPCITEMIDLIST pidl, 
        DWORD dwReserved, 
        LPSTRRET lpName);
    STDMETHODIMP SetNameOf(
        HWND hwndOwner, 
        LPCITEMIDLIST pidl,
        LPCOLESTR lpszName, 
        DWORD dwReserved,
        LPITEMIDLIST * ppidlOut);
    
    // IShellFolder2
    STDMETHODIMP GetDefaultSearchGUID(GUID *pguid) { return E_NOTIMPL; };
    STDMETHODIMP EnumSearches(IEnumExtraSearch **ppenum) { return E_NOTIMPL; };
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay) { return E_NOTIMPL; };
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags) { return E_NOTIMPL; };
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv) { return E_NOTIMPL; };
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd);
    STDMETHODIMP MapColumnToSCID(UINT iCol, SHCOLUMNID *pscid) { return E_NOTIMPL; };
    
    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // IPersistFolder
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // IPersistFolder2
    STDMETHODIMP GetCurFolder(LPITEMIDLIST *ppidl);
    
public:
    static LPITEMIDLIST CreateIDList(LPCTSTR pszName, DWORD dwFileSize,
        UINT uFileDate, UINT uFileTime, UINT uFileAttribs);
    static void GetNameOf(LPCABITEM pit, LPSTRRET lpName);
    static void GetPathOf(LPCABITEM pit, LPSTRRET lpName);
    static void GetTypeOf(LPCABITEM pit, LPSTRRET lpName);
    
    BOOL GetPath(LPTSTR szPath);
    
private:
    static void CALLBACK EnumToList(LPCTSTR pszFile, DWORD dwSize, UINT date,
        UINT time, UINT attribs, LPARAM lParam);
    
    HRESULT InitItems();
    
private:
    CRefDll m_cRefDll;
    
    CRefCount m_cRef;
    
    LPITEMIDLIST m_pidlHere;		// maintains the current pidl
    
    CCabItemList m_lItems;
    
    friend class CEnumCabObjs;
} ;

#endif // _CABFOLD_H_
