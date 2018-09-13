//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: openwith.cpp
//
// This file contains the implementation of COpenWithMenu, a context menu handler
//
// History:
//         2-27-98  by ningz
//         3-SEP-98  ZekeL - rewrote ExtAppList() and moved it here
//------------------------------------------------------------------------
#include "shellprv.h"
#include <fsmenu.h>
#include "ids.h"
#include <shlwapi.h>
#include "openwith.h"
#include "uemapp.h"

#define TF_OPENWITHMENU 0x00004000


#define SZOPENWITHLIST                  TEXT("OpenWithList")
#define REGSTR_PATH_EXPLORER_FILEEXTS   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts")
#define _OpenWithListMaxItems()         10

typedef struct _OpenWithList {
    HANDLE hMRU;
    HANDLE hMutex;
    HKEY   hkLM;
    HKEY   hkCU;
} OPENWITHLIST, *POPENWITHLIST;

LONG _CreateListMutex(LPCTSTR pszExt, HANDLE *phMutex)
{
    TCHAR szMutex[MAX_PATH];

    wnsprintf(szMutex, SIZECHARS(szMutex), TEXT("%s%s"), SZOPENWITHLIST, pszExt);
    CharLower(szMutex);

    *phMutex = CreateMutex(CreateAllAccessSecurityAttributes(NULL, NULL, NULL), FALSE, szMutex);

    if (!*phMutex) 
        return GetLastError();

    return NOERROR;
}

inline BOOL _GrabMutex(HANDLE hMutex)
{
    DWORD dwResult = WaitForSingleObject(hMutex, INFINITE);

    return (WAIT_OBJECT_0 == dwResult) ;
}

//
//  OpenWithListClose()
//  Frees up resources allocated by OpenWithListOpen().  No return value.
//
void OpenWithListClose(IN HANDLE hList)
{
    POPENWITHLIST pList = (POPENWITHLIST) hList;  
    TraceMsg(TF_OPENWITHMENU, "[%X] OpenWithListClose() called", pList);
    if (pList) 
    {
        if (pList->hMutex) 
            CloseHandle(pList->hMutex);
        
        if (pList->hkCU) 
            RegCloseKey(pList->hkCU);

        if (pList->hkLM) 
            RegCloseKey(pList->hkLM);

        if (pList->hMRU) 
            FreeMRUList(pList->hMRU);

        LocalFree(pList);
    } 
    
    return;
}

//
//  OpenWithListOpen() 
//  allocates and initializes the state for the openwithlist 
//
HRESULT OpenWithListOpen(IN LPCTSTR pszExt, HANDLE *phList)
{
    LONG lResult;

    if (!pszExt || !*pszExt || !phList) 
        return E_INVALIDARG;

    *phList = NULL;
    
    POPENWITHLIST pList = (POPENWITHLIST) LocalAlloc(LPTR, sizeof(OPENWITHLIST));
    
    if (!pList) 
        return E_OUTOFMEMORY;

    lResult = _CreateListMutex(pszExt, &pList->hMutex);

    if (NOERROR == lResult)
    {
        TCHAR szSubKey[MAX_PATH];
        //  Build up the subkey string.
        wnsprintf(szSubKey, SIZECHARS(szSubKey), TEXT("%s\\%s\\%s"), REGSTR_PATH_EXPLORER_FILEEXTS, pszExt, SZOPENWITHLIST);

        MRUINFO mi = {sizeof(mi), _OpenWithListMaxItems(), 0, HKEY_CURRENT_USER, szSubKey, NULL};

        pList->hMRU = CreateMRUList(&mi);

        //  the MRU list initializes the key a specific way so that
        //  it is a real MRU...
        if (pList->hMRU) 
        {
            lResult = RegOpenKeyEx(HKEY_CURRENT_USER, szSubKey, 0L, 
                MAXIMUM_ALLOWED, &(pList->hkCU));

            if (ERROR_SUCCESS == lResult)
            {
                //  make different subkey for LM
                wnsprintf(szSubKey, SIZECHARS(szSubKey), TEXT("%s\\%s"), pszExt, SZOPENWITHLIST);
                RegOpenKeyEx(HKEY_CLASSES_ROOT, szSubKey, 0L,
                    MAXIMUM_ALLOWED, &(pList->hkLM));
            }
        }
        else //  !pList->hMRU
            lResult = ERROR_BADKEY;
    }

    if (NOERROR != lResult)
    {
        OpenWithListClose((HANDLE)pList);
        TraceMsg(TF_OPENWITHMENU, "OpenWithListOpen() failed on %s, err = %d", pszExt, lResult);
        return HRESULT_FROM_WIN32(lResult);
    }

    TraceMsg(TF_OPENWITHMENU, "[%X] OpenWithListOpen() created on %s", pList, pszExt);
    *phList = (HANDLE) pList;
    return S_OK;
}

HRESULT _AddItem(HANDLE hMRU, HKEY hkList, LPCTSTR pszName)
{
    HRESULT hr = S_OK;
    if (hMRU)
    {
        int cItems = EnumMRUList(hMRU, -1, NULL, 0L);

        //  just trim us down to make room...
        while (cItems >= _OpenWithListMaxItems())
            DelMRUString(hMRU, --cItems);
            
        if (0 > AddMRUString(hMRU, pszName))
            hr = E_UNEXPECTED;

    }
    
    return hr;
}

void _DeleteItem(POPENWITHLIST pList, LPCTSTR pszName)
{
    int iItem = FindMRUString(pList->hMRU, pszName, NULL);
    if (0 <= iItem) {
        DelMRUString(pList->hMRU, iItem);
    } 
   
}

STDAPI OpenWithListRegister(DWORD dwFlags, LPCTSTR pszExt, LPCTSTR pszVerb, HKEY hkProgid)
{
    if (!pszExt || !hkProgid)
        return E_INVALIDARG;

    POPENWITHLIST pList;
    //
    //  ----> Peruser entries are stored here
    //  HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts
    //     \.Ext
    //         Application = "foo.exe"
    //         \OpenWithList
    //             MRUList = "ab"
    //             a = "App.exe"
    //             b = "foo.exe"
    //
    //  ----> for permanent entries are stored un HKCR
    //  HKCR
    //     \.Ext
    //         \OpenWithList
    //             \app.exe
    //
    //  ----> and applications or the system can write app association here
    //     \Applications
    //         \APP.EXE
    //             \shell...
    //         \foo.exe
    //             \shell...
    //
    //
    HRESULT hr = OpenWithListOpen(pszExt, (HANDLE *)&pList);
    
    if (SUCCEEDED(hr))
    {
        if (_GrabMutex(pList->hMutex))
        {
            TCHAR szPath[MAX_PATH];
            HRESULT hr = AssocQueryStringByKey(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, hkProgid, pszVerb, szPath, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(szPath)));

            if (SUCCEEDED(hr))
            {
                LPCTSTR pszExe = PathFindFileName(szPath);

                if (IsPathInOpenWithKillList(pszExe))
                    hr = E_ACCESSDENIED;
                else
                    hr = AssocMakeApplicationByKey(ASSOCMAKEF_VERIFY, hkProgid, pszVerb);
            
                if (SUCCEEDED(hr))
                {
                    TraceMsg(TF_OPENWITHMENU, "[%X] OpenWithListRegister() adding %s",pList, pszExe);
                    hr = _AddItem(pList->hMRU, pList->hkCU, pszExe);
                }

                if (FAILED(hr)) 
                    _DeleteItem(pList, pszExe);

            }

            ReleaseMutex(pList->hMutex);
        }
        else
            hr = E_UNEXPECTED;

        OpenWithListClose(pList);
    }


    return hr;
}

STDAPI_(void) OpenWithListSoftRegisterProcess(DWORD dwFlags, LPCTSTR pszExt)
{
    if (!pszExt || !*pszExt)
        return;

    POPENWITHLIST pList;
    
    if (SUCCEEDED(OpenWithListOpen(pszExt, (HANDLE *)&pList)))
    {
        if (_GrabMutex(pList->hMutex))
        {
            TCHAR szApp[MAX_PATH];  
            if (GetModuleFileName(NULL, szApp, SIZECHARS(szApp))
            && !IsPathInOpenWithKillList(szApp))
                _AddItem(pList->hMRU, pList->hkCU, PathFindFileName(szApp));

            ReleaseMutex(pList->hMutex);
        }

        OpenWithListClose(pList);
    }
}



typedef struct _OpenWithItem
{
    LPTSTR  pszFriendly;
    LPTSTR  pszVerb;
    HKEY    hKey;
} OPENWITHITEM, *POPENWITHITEM;

BOOL _GetAppKey(LPCTSTR pszApp, HKEY *phkApp)
{
    ASSERT(pszApp && *pszApp);
    TCHAR szKey[MAX_PATH];
    lstrcpy(szKey, TEXT("Applications\\"));
    StrCatBuff(szKey, pszApp, SIZECHARS(szKey));

    return (NOERROR == RegOpenKeyEx(
        HKEY_CLASSES_ROOT,
        szKey,
        0L,
        MAXIMUM_ALLOWED,
        phkApp));
}

DWORD _AppListEnumLM(HKEY hkey, HANDLE hMRU, LPCTSTR pszVerb, POPENWITHITEM pItems, ULONG cItems, DWORD index)
{
    for (DWORD iItem = 0; index < cItems; iItem++) 
    {
        TCHAR sz[MAX_PATH];
        DWORD cch;
        HKEY hkItem;
        
        cch = SIZECHARS(sz);

        if (NOERROR != RegEnumKeyEx(hkey, iItem, sz, &cch, 
            NULL, NULL, NULL, NULL))
            break;

        //  if we already tried this from the MRU or
        //  if the APP key doesnt exist here, then 
        //  just continue on to the next item
        if ((0 <= FindMRUString(hMRU, sz, NULL))
        ||  IsPathInOpenWithKillList(sz)
        ||  !_GetAppKey(sz, &hkItem))
            continue;
            
        ASSERT(hkItem);

        //
        //  this will filter out apps that dont support the specified verb
        //  we reuse sz here.
        if (SUCCEEDED(AssocQueryStringByKey(ASSOCF_VERIFY | ASSOCF_OPEN_BYEXENAME, ASSOCSTR_FRIENDLYAPPNAME, hkItem, pszVerb, sz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(sz)))))
        {
            pItems[index].pszFriendly = StrDup(sz);
            pItems[index].pszVerb = pszVerb ? StrDup(pszVerb) : NULL;
            pItems[index].hKey = hkItem;
            hkItem = NULL;

            index++;

        } 

        if (hkItem)
            RegCloseKey(hkItem);
            
    } //  for

    return index;
}

void _ReleaseListItem(POPENWITHITEM pItem)
{
    if (pItem->pszFriendly) 
        LocalFree(pItem->pszFriendly);

    if (pItem->pszVerb)
        LocalFree(pItem->pszVerb);

    if (pItem->hKey) 
        RegCloseKey(pItem->hKey);

    //  since this is called from only one place, 
    //  we dont actually need to clear the values...
    //  ZeroMemory(pItem, SIZEOF(OPENWITHITEM))
}

//
// Must declare an explicit structure to keep Win64 happy.
// We actually hand out an internal pointer to the rgItems array,
// but occasionally we need to peek at the cItems.
//
typedef struct ITEMSLIST {
    DWORD cItems;
    OPENWITHITEM rgItems[1];
} ITEMSLIST, *PITEMSLIST;

void OpenWithListReleaseList(POPENWITHITEM pItems)
{
    if (pItems)
    {
        PITEMSLIST pList = CONTAINING_RECORD(pItems, ITEMSLIST, rgItems);
        
        DWORD cItems = pList->cItems;
        for (DWORD i = 0; i < cItems; i++)
            _ReleaseListItem(&pList->rgItems[i]);
        LocalFree(pList);
    }
}

DWORD _AllocList(POPENWITHLIST pList, POPENWITHITEM *ppItems)
{
    
    DWORD cResult = (DWORD) EnumMRUList(pList->hMRU, -1, NULL, 0L);

    if (cResult == (DWORD)-1)
        cResult = 0;

    if (pList->hkLM)
    {
        DWORD dw = 0;
        RegQueryInfoKey(
            pList->hkLM,
            NULL,
            NULL,
            NULL,
            &dw,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL
        );
        cResult += dw;        
    }


    if (cResult)
    {
        PITEMSLIST pList = (PITEMSLIST)LocalAlloc(LPTR, FIELD_OFFSET(ITEMSLIST, rgItems[cResult]));

        if(pList)
            *ppItems = &pList->rgItems[0];
        else
            cResult = 0;
    }
    return cResult;    
}

DWORD _AppListEnumMRU(HANDLE hMRU, LPCTSTR pszVerb, POPENWITHITEM pItems, ULONG cItems, DWORD index)
{
    for (int iItem = 0; index < cItems; iItem++) 
    {
        TCHAR sz[MAX_PATH];
        HKEY hkItem;
        
        if (0 > EnumMRUList(hMRU, iItem, sz, SIZECHARS(sz)))
            break;

        //  dont quit if we couldnt get this key...
        if (IsPathInOpenWithKillList(sz) || !_GetAppKey(sz, &hkItem))
            continue;
            
        ASSERT(hkItem);

        //
        //  this will filter out apps that dont support the specified verb
        //  we reuse sz here.
        if (SUCCEEDED(AssocQueryStringByKey(ASSOCF_VERIFY | ASSOCF_OPEN_BYEXENAME, ASSOCSTR_FRIENDLYAPPNAME, hkItem, pszVerb, sz, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(sz)))))
        {
            pItems[index].pszFriendly = StrDup(sz);
            pItems[index].pszVerb = pszVerb ? StrDup(pszVerb) : NULL;
            pItems[index].hKey = hkItem;
            hkItem = NULL;

            index++;

        } 

        if (hkItem)
            RegCloseKey(hkItem);
            
    } //  for

    return index;
}

//
//  OpenWithListGetList()
//
//  Allocates and initializes a list of all the openwithlist items
//  that are associated with the pszExt, and can shellexecute pszVerb.
//  if the pszVerb is NULL, the default verbs are allowed
//
DWORD OpenWithListGetList(LPCTSTR pszExt, LPCTSTR pszVerb, POPENWITHITEM *ppItems)
{
    POPENWITHLIST pList;
    DWORD cResult = 0;

    if (!pszExt || !ppItems)
        return 0;

    *ppItems = NULL;
    
    if (SUCCEEDED(OpenWithListOpen(pszExt, (HANDLE *)&pList)))
    {
        if (_GrabMutex(pList->hMutex))
        {
            POPENWITHITEM pItems;
            LONG cItems = _AllocList(pList, &pItems);

            if (cItems)
            {
                //  enum the MRU first, and then if there are
                //  any that were under HKCR, them we will add them too.
                cResult = _AppListEnumMRU(pList->hMRU, pszVerb, pItems, cItems, 0);
                if (pList->hkLM)
                    cResult =_AppListEnumLM(pList->hkLM, pList->hMRU, pszVerb, pItems, cItems, cResult);

                if (cResult)
                {
                    PITEMSLIST pList = CONTAINING_RECORD(pItems, ITEMSLIST, rgItems);
                    pList->cItems = cResult;
                    *ppItems = pItems;
                }
                else
                {
                    OpenWithListReleaseList(pItems);    
                }

            }

            ReleaseMutex(pList->hMutex);
        }

        OpenWithListClose((HANDLE)pList);
        
    } 

    return cResult;
}



typedef struct 
{
    TCHAR szMenuText[MAX_PATH];
    int iImage;
    
    DWORD dwFlags;
} OPENWITHMENUITEMINFO, * LPOPENWITHMENUITEMINFO;

// format strings
const TCHAR c_szSShellSCommand[] = TEXT("%s\\Shell\\%s\\Command");
const TCHAR c_szSShell[] = TEXT("%s\\Shell");

class COpenWithMenu : public IContextMenu3, IShellExtInit,IObjectWithSite
{
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    
    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax);
    
    // IContextMenu2
    STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // IContextMenu3
    STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT *lResult);
    
    // IShellExtInit
    STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID);
    
    //IObjectWithSite
    STDMETHOD(SetSite)(IUnknown*);
    STDMETHOD(GetSite)(REFIID,void**);
    
    int                 _cRef;
    HMENU               _hMenu;
    UINT                _idCmdFirst;
    int                 _nItems;
    UINT                _uFlags;
    POPENWITHITEM       _pItems;
    TCHAR               _szPath[MAX_PATH];
    HIMAGELIST          _himlSystemImageList;
    IDataObject        *_pdtobj;
    IShellView2        *_pShellView2;
    LPCITEMIDLIST       _pidlFolder;
    
    COpenWithMenu();
    ~COpenWithMenu();
    
    friend HRESULT COpenWithMenu_CreateInstance(IUnknown* pUnkOuter, REFIID riid, OUT LPVOID *  ppvOut);
    
private:
    //Handle Menu messages submitted to HandleMenuMsg
    void DrawItem(DRAWITEMSTRUCT *lpdi);
    LRESULT MeasureItem(MEASUREITEMSTRUCT *lpmi);
    BOOL InitMenuPopup(HMENU hMenu);
    
    //Internal Helpers
    POPENWITHITEM GetItemData(HMENU hmenu, UINT iItem);
};


COpenWithMenu::COpenWithMenu() : _cRef(1) 
{
    TraceMsg(TF_OPENWITHMENU, "ctor COpenWithMenu %x", this);
}

COpenWithMenu::~COpenWithMenu()
{
    TraceMsg(TF_OPENWITHMENU, "dtor COpenWithMenu %x", this);

    if (_pdtobj)
        _pdtobj->Release();

    if (_pItems)
    {
        OpenWithListReleaseList(_pItems);
    }

    //Safety Net: Release my site in case I manage to get 
    // Released without my site SetSite(NULL) first.
    ATOMICRELEASE(_pShellView2);
}


STDAPI COpenWithMenu_CreateInstance(IUnknown* pUnkOuter, REFIID riid, OUT LPVOID *  ppvOut)
{
    HRESULT hr = E_FAIL;
    
    TraceMsg(TF_OPENWITHMENU, "COpenWithMenu_CreateInstance()");
    *ppvOut = NULL;                     

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    COpenWithMenu * powm = new COpenWithMenu();
    if (!powm)
        return E_OUTOFMEMORY;
    
    hr = powm->QueryInterface(riid, (LPVOID *)ppvOut);

    powm->Release();
    
    return hr;
}


HRESULT COpenWithMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IContextMenu) || 
        IsEqualIID(riid, IID_IContextMenu2) ||  
        IsEqualIID(riid, IID_IContextMenu3))
    {
        *ppvObj = SAFECAST(this, IContextMenu3 *);
    }
    else if (IsEqualIID(riid, IID_IShellExtInit))
    {
        *ppvObj = SAFECAST(this, IShellExtInit *);
    }
    else if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = SAFECAST(this, IObjectWithSite *);
    }
    else 
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return NOERROR;
}

ULONG COpenWithMenu::AddRef()
{
    _cRef++;
    TraceMsg(TF_OPENWITHMENU, "COpenWithMenu::AddRef = %x", _cRef);
    return _cRef;
}

ULONG COpenWithMenu::Release()
{
    _cRef--;
    TraceMsg(TF_OPENWITHMENU, "COpenWithMenu::Release = %x", _cRef);
    
    if (_cRef > 0)
        return _cRef;
    
    delete this;
    return 0;
}

/*
    Purpose:
        Add verb to extension app list
*/
VOID AddVerbItem(IQueryAssociations *pqa, HKEY hkeyClass, LPCTSTR pszExt, LPCTSTR pszVerb)
{
    WCHAR wsz[MAX_PATH];
    WCHAR wszVerb[MAX_PATH];

    // HackHack: we don't want to put msohtmed.exe in openwithlist
    if (pszVerb)
        SHTCharToUnicode(pszVerb, wszVerb, SIZECHARS(wszVerb));
        
    if (SUCCEEDED(pqa->GetString(0, ASSOCSTR_EXECUTABLE, pszVerb ? wszVerb : NULL, wsz,(LPDWORD)MAKEINTRESOURCE(SIZECHARS(wsz))))
    && (StrStrIW(wsz, L"msohtmed")))
        return;

    OpenWithListRegister(0, pszExt, pszVerb, hkeyClass);
}

/*
    Purpose:
        Add Open/Edit/Default verb to extension app list
*/
HRESULT AddVerbItems(LPCTSTR pszExt)
{
    IQueryAssociations *pqa;
    HRESULT hr = E_FAIL;
    
    if (SUCCEEDED(AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (LPVOID *)&pqa)))
    {
        HKEY hkeyClass;
        WCHAR wszExt[MAX_PATH];
        SHTCharToUnicode(pszExt, wszExt, SIZECHARS(wszExt));
        
        if (SUCCEEDED(pqa->Init(0, wszExt, NULL, NULL))
        && (SUCCEEDED(pqa->GetKey(0, ASSOCKEY_SHELLEXECCLASS, NULL, &hkeyClass))))
        {
            AddVerbItem(pqa, hkeyClass, pszExt, NULL);
            AddVerbItem(pqa, hkeyClass, pszExt, c_szOpen);
            AddVerbItem(pqa, hkeyClass, pszExt, c_szEdit);
            RegCloseKey(hkeyClass);
            hr = S_OK;
        }
        pqa->Release();
    }
    return hr;
}


HRESULT COpenWithMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    MENUITEMINFO mii;
    LPTSTR pszExt;
    TCHAR szOpenWithMenu[80];
    
    _idCmdFirst = idCmdFirst;
    _uFlags = uFlags;
    
    if (SUCCEEDED(PathFromDataObject(_pdtobj, _szPath, ARRAYSIZE(_szPath))))
    {
        // No openwith context menu for executables.
        if (PathIsExe(_szPath))
            return NOERROR;
            
        pszExt = PathFindExtension(_szPath);
        if (pszExt && *pszExt)
        {

            // Add Open/Edit/Default verb to extension app list
            if (SUCCEEDED(AddVerbItems(pszExt)))
            {
                // Do this only if AddVerbItems succeeded; otherwise,
                // we would create an empty MRU for a nonexisting class,
                // causing the class to spring into existence and cause
                // the "Open With" dialog to think we are overriding
                // rather than creating new.
                // get extension app list
                _nItems = OpenWithListGetList(pszExt, NULL, &_pItems);
            }
        }
    }

    // For known file type(there is at least one verb under its progid), 
    // if there is only one item in its openwithlist, don't show open with sub menu
    if (1 == _nItems)
    {
        TCHAR szExe[MAX_PATH];
        DWORD cch = ARRAYSIZE(szExe);

        if (SUCCEEDED(AssocQueryString(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, pszExt, NULL, szExe, &cch)))
        {
            TCHAR szItem[MAX_PATH];
            cch = ARRAYSIZE(szItem);
            
            if (SUCCEEDED(AssocQueryStringByKey(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, _pItems[0].hKey, _pItems[0].pszVerb, szItem, &cch))
            && 0 == StrCmpI(szExe, szItem))
            {

                OpenWithListReleaseList(_pItems);

                _pItems = NULL;
                _nItems = 0;
            }
        }
    }

    LoadString(g_hinst, (_nItems ? IDS_OPENWITH : IDS_OPENWITHNEW), szOpenWithMenu, ARRAYSIZE(szOpenWithMenu));
    
    if (_nItems)
    {
        _hMenu = CreatePopupMenu();
        
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID|MIIM_TYPE|MIIM_DATA;
        mii.wID = idCmdFirst+1;
        mii.fType = MFT_STRING;
        mii.dwTypeData = szOpenWithMenu;
        mii.dwItemData = 0;
    
        InsertMenuItem(_hMenu,0,TRUE,&mii);
    
        mii.fMask = MIIM_ID|MIIM_SUBMENU|MIIM_TYPE;
        mii.fType = MFT_STRING;
        mii.wID = idCmdFirst;
        mii.hSubMenu = _hMenu;
        mii.dwTypeData = szOpenWithMenu;
    
        InsertMenuItem(hmenu,indexMenu,TRUE,&mii);

        return ResultFromShort(_nItems + 2);
    }
    else
    {
        _hMenu = hmenu;
        
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID|MIIM_TYPE|MIIM_DATA;
        mii.fType = MFT_STRING;
        mii.wID = idCmdFirst;
        mii.dwTypeData = szOpenWithMenu;
        mii.dwItemData = 0;
        
        InsertMenuItem(hmenu,indexMenu,TRUE,&mii);

        return ResultFromShort(1);
    }
}

HRESULT COpenWithMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hres = E_OUTOFMEMORY;
    CMINVOKECOMMANDINFOEX ici;
    LPVOID pvFree;

    //  maybe these two routines should be collapsed into one?
    if ((IS_INTRESOURCE(pici->lpVerb) || 0 == lstrcmpiA(pici->lpVerb, "openas"))
    && SUCCEEDED(ICI2ICIX(pici, &ici, &pvFree)))
    {
        SHELLEXECUTEINFO ei = {0};

        if (SUCCEEDED(ICIX2SEI(&ici, &ei)))
        {
            POPENWITHITEM pItem;
            ei.lpFile = _szPath;

            if (IS_INTRESOURCE(pici->lpVerb))
                pItem = GetItemData(_hMenu, LOWORD(pici->lpVerb));
            else
                pItem = NULL;

            if (pItem)
            {
                //  if pitem is there, this means that we are using
                //  something that was in the openwith list MRU.
                
                ei.lpVerb = pItem->pszVerb;
                ei.hkeyClass = pItem->hKey;

                //  make sure to update the MRU
                OpenWithListRegister(0, PathFindExtension(_szPath), pItem->pszVerb, pItem->hKey);
            }
            else
            {   
                // use the "Unknown" key so we get the openwith prompt
                RegOpenKeyEx(HKEY_CLASSES_ROOT, TEXT("Unknown"), 0L, MAXIMUM_ALLOWED, &ei.hkeyClass);

                if (!(_uFlags & CMF_DEFAULTONLY))
                {
                    // defview sets CFM_DEFAULTONLY when the user is double-clicking. We check it
                    // here since we want do NOT want to query the class store if the user explicitly
                    // right-clicked on the menu and choo   se openwith.

                    // pop up open with dialog without querying class store
                    ei.fMask |= SEE_MASK_NOQUERYCLASSSTORE;
                }
            }

            //  if we got the key then we are good to go!
            if (ei.hkeyClass)
            {
                ei.fMask |= SEE_MASK_CLASSKEY;

#ifdef WINNT
                // Shrink the shell since the user is about to run an application.
                ShrinkWorkingSet();
#endif

                if (FALSE != ShellExecuteEx(&ei)) 
                {
                    hres = NOERROR;
#ifdef WINNT
                    // Shrink the shell since the user just ran an application.
                    ShrinkWorkingSet();
#endif
                    if (UEMIsLoaded())
                    {
                        // note that we already got a UIBL_DOTASSOC (from
                        // OpenAs_RunDLL or whatever it is that 'Unknown'
                        // runs).  so the Uassist analysis app will have to
                        // subtract it off
                        UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_RUNASSOC, UIBL_DOTNOASSOC);
                    }
                }
                else
                {
                    hres = E_FAIL;
                }

                // Close the Unknown key if we opened it
                if (!pItem && ei.hkeyClass)
                {
                    RegCloseKey(ei.hkeyClass);
                }
            }

        }

        if (pvFree)
            LocalFree(pvFree);
    }        


    return hres;
}

HRESULT COpenWithMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

HRESULT COpenWithMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg,wParam,lParam,NULL);
}

HRESULT COpenWithMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam,LRESULT *lResult)
{
    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
        {
            InitMenuPopup(_hMenu);
        }
        break;
    /*    
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT * pdi = (DRAWITEMSTRUCT *)lParam;
            
            DrawItem(pdi);
        }
        break;
        
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT *pmi = (MEASUREITEMSTRUCT *)lParam;
            
            MeasureItem(pmi);
            
        }
        break;
    case WM_MENUCHAR:
        {
            int c = GetMenuItemCount(_hMenu);
            for (int i = 0; i < c; i++) 
            {
                LPOPENWITHMENUITEMINFO lpomi = GetItemData(_hMenu, i);
                if(lpomi && _MenuCharMatch(lpomi->szMenuText,(TCHAR)LOWORD(wParam),FALSE))
                {
                    _lpomiLast = lpomi;
                    if(lResult) *lResult = MAKELONG(i,MNC_EXECUTE);
                    return S_OK;
                }
            }
            if(lResult) *lResult = MAKELONG(0,MNC_IGNORE);
            return S_FALSE;
            
        }
        
    */
    }
    
    return NOERROR;
}

HRESULT COpenWithMenu::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    if (_pdtobj)
        _pdtobj->Release();
    
    
    _pidlFolder = pidlFolder;
    _pdtobj = pdtobj;
    
    
    if (_pdtobj)
        _pdtobj->AddRef();
    
    return NOERROR;
}


void COpenWithMenu::DrawItem(DRAWITEMSTRUCT *lpdi)
{
/*
    if ((lpdi->itemAction & ODA_SELECT) || (lpdi->itemAction & ODA_DRAWENTIRE))
    {
        DWORD dwRop;
        int x, y;
        SIZE sz;
        LPOPENWITHMENUITEMINFO lpomi = (LPOPENWITHMENUITEMINFO)lpdi->itemData;
        
        // Draw the image (if there is one).
        
        GetTextExtentPoint(lpdi->hDC, lpomi->szMenuText, lstrlen(lpomi->szMenuText), &sz);
        
        if (lpdi->itemState & ODS_SELECTED)
        {
            SetBkColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHT));
            SetTextColor(lpdi->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            // REVIEW HACK - keep track of the last selected item.
            _lpomiLast = lpomi;
            dwRop = SRCSTENCIL;
            FillRect(lpdi->hDC,&lpdi->rcItem,GetSysColorBrush(COLOR_HIGHLIGHT));
        }
        else
        {
            dwRop = SRCAND;
            SetTextColor(lpdi->hDC, GetSysColor(COLOR_MENUTEXT));
            FillRect(lpdi->hDC,&lpdi->rcItem,GetSysColorBrush(COLOR_MENU));
        }
        
        RECT rc = lpdi->rcItem;
        rc.left += +2*CXIMAGEGAP+g_cxSmIcon;
        
        
        DrawText(lpdi->hDC,lpomi->szMenuText,lstrlen(lpomi->szMenuText),
            &rc,DT_SINGLELINE|DT_VCENTER);
        if (lpomi->iImage != -1)
        {
            x = lpdi->rcItem.left+CXIMAGEGAP;
            y = (lpdi->rcItem.bottom+lpdi->rcItem.top-g_cySmIcon)/2;
            ImageList_Draw(g_himlSysSmall, lpomi->iImage, lpdi->hDC, x, y, ILD_TRANSPARENT);
        } 
        else 
        {
            x = lpdi->rcItem.left+CXIMAGEGAP;
            y = (lpdi->rcItem.bottom+lpdi->rcItem.top-g_cySmIcon)/2;
        }
    }
*/
}

LRESULT COpenWithMenu::MeasureItem(MEASUREITEMSTRUCT *lpmi)
{
    LRESULT lres = FALSE;
/*    
    LPOPENWITHMENUITEMINFO lpomi = (LPOPENWITHMENUITEMINFO)lpmi->itemData;
    if (lpomi)
    {
        // Get the rough height of an item so we can work out when to break the
        // menu. User should really do this for us but that would be useful.
        HDC hdc = GetDC(NULL);
        if (hdc)
        {
            // REVIEW cache out the menu font?
            NONCLIENTMETRICS ncm;
            ncm.cbSize = SIZEOF(NONCLIENTMETRICS);
            if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, SIZEOF(ncm), &ncm, FALSE))
            {
                HFONT hfont = CreateFontIndirect(&ncm.lfMenuFont);
                if (hfont)
                {
                    SIZE sz;
                    HFONT hfontOld = (HFONT)SelectObject(hdc, hfont);
                    GetTextExtentPoint(hdc, lpomi->szMenuText, lstrlen(lpomi->szMenuText), &sz);
                    lpmi->itemHeight = max (g_cySmIcon+CXIMAGEGAP/2, ncm.iMenuHeight);
                    lpmi->itemWidth = g_cxSmIcon + 2*CXIMAGEGAP + sz.cx;
                    //lpmi->itemWidth = 2*CXIMAGEGAP + sz.cx;
                    SelectObject(hdc, hfontOld);
                    DeleteObject(hfont);
                    lres = TRUE;
                }
            }
            ReleaseDC(NULL, hdc);
        }
    }
    else
    {
        TraceMsg(TF_OPENWITHMENU, TEXT("fm_mi: Filemenu is invalid."));
    }
*/    
    return lres;
}

BOOL COpenWithMenu::InitMenuPopup(HMENU hmenu)
{
    TCHAR szMenuText[80];
    MENUITEMINFO mii;
    
    TraceMsg(TF_OPENWITHMENU, "COpenWithMenu::InitMenuPopup");

    if (!_nItems || !_pItems)
        return FALSE;

    if (GetItemData(hmenu, 0))  // already initialized.
        return FALSE;

    // remove the place holder.
    DeleteMenu(hmenu,0,MF_BYPOSITION);

    // add app's in mru list to context menu
    for (int i = 0; i < _nItems; i++)
    {
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID|MIIM_TYPE|MIIM_DATA;
        mii.wID = _idCmdFirst + i;
        mii.fType = MFT_STRING;
        mii.dwTypeData = _pItems[i].pszFriendly;
        mii.dwItemData = (DWORD_PTR)&_pItems[i];

        InsertMenuItem(hmenu,GetMenuItemCount(hmenu),TRUE,&mii);
    }

    // add seperator
    AppendMenu(hmenu,MF_SEPARATOR,0,NULL); 

    // add "&Browse..."
    LoadString(g_hinst, IDS_OPENWITHBROWSE, szMenuText, ARRAYSIZE(szMenuText));
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID|MIIM_TYPE|MIIM_DATA;
    mii.wID = _idCmdFirst + _nItems + 1;
    mii.fType = MFT_STRING;
    mii.dwTypeData = szMenuText;
    mii.dwItemData = 0;

    InsertMenuItem(hmenu,GetMenuItemCount(hmenu),TRUE,&mii);

    return TRUE;
}



POPENWITHITEM COpenWithMenu::GetItemData(HMENU hmenu, UINT iItem)
{
    MENUITEMINFO mii;
    
    mii.cbSize = SIZEOF(MENUITEMINFO);
    mii.fMask = MIIM_DATA | MIIM_STATE;
    mii.cch = 0;     // just in case...
    
    if (GetMenuItemInfo(hmenu, iItem, TRUE, &mii))
        return (POPENWITHITEM)mii.dwItemData;
    
    return NULL;
}

HRESULT COpenWithMenu::SetSite(IUnknown* pUnk)
{
    ATOMICRELEASE(_pShellView2);
    TraceMsg(TF_OPENWITHMENU, "COpenWithMenu::SetSite = 0x%x", pUnk);
    
    if(pUnk)
        return pUnk->QueryInterface(IID_IShellView2,(void**)&_pShellView2);

    return NOERROR;
}

HRESULT COpenWithMenu::GetSite(REFIID riid,void** ppvObj)
{
    if(_pShellView2)
        return _pShellView2->QueryInterface(riid,ppvObj);
    else
    {
        ASSERT(ppvObj != NULL);
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}
