//*******************************************************************************************
//
// Filename : folder.cpp
//	
//				CAB Files Shell Extension
//
// Copyright (c) 1994 - 1997 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************


#include "pch.h"

#include "thisdll.h"
#include "thisguid.h"

#include "folder.h"
#include "enum.h"
#include "icon.h"
#include "menu.h"
#include "dataobj.h"

#include "cabitms.h"

#include "resource.h"

STDAPI StringToStrRet(LPCTSTR pszName, STRRET *pStrRet)
{
#ifdef UNICODE
    pStrRet->uType = STRRET_WSTR;
    return SHStrDup(pszName, &pStrRet->pOleStr);
#else
    pStrRet->uType = STRRET_CSTR;
    lstrcpyn(pStrRet->cStr, pszName, ARRAYSIZE(pStrRet->cStr));
    return NOERROR;
#endif
}

STDMETHODIMP CCabFolder::QueryInterface(REFIID riid, void **ppv)
{
    if (CLSID_CabFolder == riid)
    {
        // yuck - dataobject uses this when loading us from a stream:
        // NOTE: we are doing an AddRef() in this case
        *ppv = (CCabFolder*) this;
        AddRef();
        return S_OK;
    }
    else
    {
        static const QITAB qit[] = {
            QITABENT(CCabFolder, IShellFolder2),
            QITABENTMULTI(CCabFolder, IShellFolder, IShellFolder2),
            QITABENT(CCabFolder, IPersistFolder2),
            QITABENTMULTI(CCabFolder, IPersistFolder, IPersistFolder2),
            QITABENTMULTI(CCabFolder, IPersist, IPersistFolder2),
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }
}


STDMETHODIMP_(ULONG) CCabFolder::AddRef(void)
{
    return(m_cRef.AddRef());
}


STDMETHODIMP_(ULONG) CCabFolder::Release(void)
{
    if (!m_cRef.Release())
    {
        delete this;
        return(0);
    }

    return(m_cRef.GetRef());
}

STDMETHODIMP CCabFolder::ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszDisplayName, ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG *pdwAttributes)
{
    return E_NOTIMPL;
}

//**********************************************************************
//
// Purpose:
//
//      Creates an item enumeration object
//      (an IEnumIDList interface) that can be used to
//      enumerate the contents of a folder.
//
// Parameters:
//
//       HWND hwndOwner       -    handle to the owner window
//       DWORD grFlags        -    flags about which items to include
//       IEnumIDList **ppenumIDList - address that receives IEnumIDList
//                                    interface pointer
//********************************************************************

STDMETHODIMP CCabFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList **ppenumIDList)
{
    HRESULT hres;
    CEnumCabObjs *pce = new CEnumCabObjs(this, grfFlags);
    if (pce)
    {
        hres = pce->QueryInterface(IID_IEnumIDList, (void **)ppenumIDList);
    }
    else
    {
        *ppenumIDList = NULL;
        hres = E_OUTOFMEMORY;
    }
    return hres;
}

STDMETHODIMP CCabFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvObj)
{
    return E_NOTIMPL;
}

STDMETHODIMP CCabFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void ** ppvObj)
{
    return E_NOTIMPL;
}


//**********************************************************************
//
// CCabFolder::CompareIDs
//
// Purpose:
//
//      Determines the relative ordering of two file
//      objects or folders, given their item identifier lists
//
// Parameters:
//
//      LPARAM lParam         -    type of comparison
//      LPCITEMIDLIST pidl1   -    address to ITEMIDLIST
//      LPCITEMIDLIST pidl2   -    address to ITEMIDLIST
//
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP CCabFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPCABITEM pit1 = (LPCABITEM)pidl1;
    LPCABITEM pit2 = (LPCABITEM)pidl2;

    short nCmp = 0;

    switch (lParam)
    {
    case CV_COL_NAME:
        break;

    case CV_COL_SIZE:
        if (pit1->dwFileSize < pit2->dwFileSize)
        {
            nCmp = -1;
        }
        else if (pit1->dwFileSize > pit2->dwFileSize)
        {
            nCmp = 1;
        }
        break;

    case CV_COL_TYPE:
        {
            STRRET srName1, srName2;

            GetTypeOf(pit1, &srName1);
            GetTypeOf(pit2, &srName2);

#ifdef UNICODE
            // BUGBUG: check for NULL pOleStr's
            nCmp = (SHORT)lstrcmp(srName1.pOleStr, srName2.pOleStr);
#else  // UNICODE
            nCmp = (SHORT)lstrcmp(srName1.cStr, srName2.cStr);
#endif // UNICODE
            break;
        }

    case CV_COL_MODIFIED:
        if (pit1->uFileDate < pit2->uFileDate)
        {
            nCmp = -1;
        }
        else if (pit1->uFileDate > pit2->uFileDate)
        {
            nCmp = 1;
        }
        else if (pit1->uFileTime < pit2->uFileTime)
        {
            nCmp = -1;
        }
        else if (pit1->uFileTime > pit2->uFileTime)
        {
            nCmp = 1;
        }
        break;

    case CV_COL_PATH:
        if (pit1->cPathChars == 0)
        {
            if (pit2->cPathChars != 0)
            {
                nCmp = -1;
            }
        }
        else if (pit2->cPathChars == 0)
        {
            nCmp = 1;
        }
        else if (pit1->cPathChars <= pit2->cPathChars)
        {
            nCmp = (short) StrCmpN(pit1->szName, pit2->szName, pit1->cPathChars-1);

            if ((nCmp == 0) && (pit1->cPathChars < pit2->cPathChars))
            {
                nCmp = -1;
            }
        }
        else
        {
            nCmp = (short) StrCmpN(pit1->szName, pit2->szName, pit2->cPathChars-1);

            if (nCmp == 0)
            {
                nCmp = 1;
            }
        }
        break;

    default:
        break;
    }

    if (nCmp != 0)
    {
        return ResultFromShort(nCmp);
    }

    return ResultFromShort(lstrcmpi(pit1->szName + pit1->cPathChars, pit2->szName + pit2->cPathChars));
}


//**********************************************************************
//
// CCabFolder::CreateViewObject
//
// Purpose:
//
//      IShellbrowser calls this to create a ShellView
//      object
//
// Parameters:
//
//      HWND   hwndOwner     -
//
//      REFIID riid          -  interface ID
//
//      void ** ppvObj      -  pointer to the Shellview object
//
// Return Value:
//
//      NOERROR
//      E_OUTOFMEMORY
//      E_NOINTERFACE
//
//
// Comments:
//
//      ShellBrowser interface calls this to request the ShellFolder
//      to create a ShellView object
//
//********************************************************************

STDMETHODIMP CCabFolder::CreateViewObject(HWND hwndOwner, REFIID riid, void **ppvObj)
{
    HRESULT hres;

    if (riid == IID_IShellView)
    {
        SFV_CREATE sfvc;
        sfvc.cbSize = sizeof(sfvc);
        sfvc.pshf = this;
        sfvc.psvOuter = NULL;
        sfvc.psfvcb = NULL;

        hres = SHCreateShellFolderView(&sfvc, (IShellView **)ppvObj);
    }
    else
    {
        *ppvObj = NULL;
        hres = E_NOINTERFACE;
    }

    return hres;
}


// **************************************************************************************
//
// CCabFolder::GetAttributesOf
//
// Purpose
//
//			Retrieves attributes of one of more file objects
//
// Parameters:
//			
//    UINT cidl                -    number of file objects
//    LPCITEMIDLIST  *apidl    -    pointer to array of ITEMIDLIST
//    ULONG *rgfInOut          -    array of values that specify file object
//                                  attributes
//
//
// Return Value:
//
//     NOERROR
//
//	Comments
//
// ***************************************************************************************

STDMETHODIMP CCabFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST *apidl, ULONG *rgfInOut)
{
    *rgfInOut &= SFGAO_CANCOPY;
    return NOERROR;
}

// **************************************************************************************
//
// CCabFolder::GetUIObjectOf
//
// Purpose
//
//			Returns an interface that can be used to carry out actions on
//          the specified file objects or folders
//
// Parameters:
//
//        HWND hwndOwner        -    handle of the Owner window
//
//        UINT cidl             -    Number of file objects
//
//        LPCITEMIDLIST *apidl  -    array of file object pidls
//
//        REFIID                -    Identifier of interface to return
//        			
//        UINT * prgfInOut      -    reserved
//
//        void **ppvObj        -    address that receives interface pointer
//
// Return Value:
//
//         E_INVALIDARG
//         E_NOINTERFACE
//         E_OUTOFMEMORY
//
//	Comments
// ***************************************************************************************

STDMETHODIMP CCabFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST *apidl,
                                       REFIID riid, UINT *prgfInOut,  void **ppvObj)
{
    IUnknown *pObj = NULL;

    if (riid == IID_IExtractIcon)
    {
        if (cidl != 1)
        {
            return(E_INVALIDARG);
        }

        LPCABITEM pci = (LPCABITEM)*apidl;
        pObj = (IUnknown *)(IExtractIcon *)(new CCabItemIcon(pci->szName));
    }
    else if (riid == IID_IContextMenu)
    {
        if (cidl < 1)
        {
            return(E_INVALIDARG);
        }

        pObj = (IUnknown *)(IContextMenu *)(new CCabItemMenu(hwndOwner, this,
            (LPCABITEM *)apidl, cidl));
    }
    else if (riid == IID_IDataObject)
    {
        if (cidl < 1)
        {
            return(E_INVALIDARG);
        }

        pObj = (IUnknown *)(IDataObject *)(new CCabObj(hwndOwner, this,
            (LPCABITEM *)apidl, cidl));
    }
    else
    {
        return E_NOINTERFACE;
    }

    if (!pObj)
    {
        return E_OUTOFMEMORY;
    }

    pObj->AddRef();
    HRESULT hres = pObj->QueryInterface(riid, ppvObj);
    pObj->Release();

    return hres;
}

//*****************************************************************************
//
// CCabFolder::GetDisplayNameOf
//
// Purpose:
//        Retrieves the display name for the specified file object or
//        subfolder.
//
//
// Parameters:
//
//        LPCITEMIDLIST    pidl    -    pidl of the file object
//        DWORD  dwReserved        -    Value of the type of display name to
//                                      return
//        LPSTRRET  lpName         -    address holding the name returned
//
//
// Comments:
//
//*****************************************************************************


STDMETHODIMP CCabFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD dwReserved, LPSTRRET lpName)
{
    LPCABITEM pit = (LPCABITEM)pidl;

    GetNameOf(pit, lpName);

    return NOERROR;
}

STDMETHODIMP CCabFolder::SetNameOf(HWND hwndOwner,  LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD dwRes, LPITEMIDLIST *ppidlOut)
{
    return E_NOTIMPL;
}

struct _CVCOLINFO
{
    UINT iColumn;
    UINT iTitle;
    UINT cchCol;
    UINT iFmt;
} s_aCVColInfo[] = {
    {CV_COL_NAME,     IDS_CV_COL_NAME,     20, LVCFMT_LEFT},
    {CV_COL_SIZE,     IDS_CV_COL_SIZE,     10, LVCFMT_RIGHT},
    {CV_COL_TYPE,     IDS_CV_COL_TYPE,     20, LVCFMT_LEFT},
    {CV_COL_MODIFIED, IDS_CV_COL_MODIFIED, 20, LVCFMT_LEFT},
    {CV_COL_PATH,     IDS_CV_COL_PATH,     30, LVCFMT_LEFT},
};

STDMETHODIMP CCabFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *psd)
{
    LPCABITEM pit = (LPCABITEM)pidl;
    TCHAR szTemp[MAX_PATH];

    if (iColumn >= CV_COL_MAX)
    {
        return E_NOTIMPL;
    }

    psd->str.uType = STRRET_CSTR;
    psd->str.cStr[0] = '\0';

    if (!pit)
    {
        TCHAR szTitle[MAX_PATH];
        LoadString(g_ThisDll.GetInstance(), s_aCVColInfo[iColumn].iTitle, szTitle, ARRAYSIZE(szTitle));
        StringToStrRet(szTitle, &(psd->str));
        psd->fmt = s_aCVColInfo[iColumn].iFmt;
        psd->cxChar = s_aCVColInfo[iColumn].cchCol;
        return S_OK;
    }

    switch (iColumn)
    {
    case CV_COL_NAME:
        GetNameOf(pit, &psd->str);
        break;

    case CV_COL_PATH:
        GetPathOf(pit, &psd->str);
        break;

    case CV_COL_SIZE:
        {
            ULARGE_INTEGER ullSize = {pit->dwFileSize, 0};
            StrFormatKBSize(ullSize.QuadPart, szTemp, ARRAYSIZE(szTemp));
            StringToStrRet(szTemp, &(psd->str));
            break;
        }

    case CV_COL_TYPE:
        GetTypeOf(pit, &psd->str);
        break;

    case CV_COL_MODIFIED:
        {
            FILETIME ft, uft;
            DosDateTimeToFileTime(pit->uFileDate, pit->uFileTime, &ft);
            LocalFileTimeToFileTime(&ft, &uft);         // Apply timezone
            SHFormatDateTime(&uft, NULL, szTemp, ARRAYSIZE(szTemp));
            StringToStrRet(szTemp, &(psd->str));
        }
        break;
    }

    return S_OK;
}


// *** IPersist methods ***

STDMETHODIMP CCabFolder::GetClassID(CLSID *pclsid)
{
    *pclsid = CLSID_CabFolder;
    return NOERROR;
}


// IPersistFolder

STDMETHODIMP CCabFolder::Initialize(LPCITEMIDLIST pidl)
{
    if (m_pidlHere)
    {
        ILFree(m_pidlHere);
    }

    m_pidlHere = ILClone(pidl); // copy the pidl
    return m_pidlHere ? S_OK : E_OUTOFMEMORY;
}

HRESULT CCabFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    if (m_pidlHere)
    {
        *ppidl = ILClone(m_pidlHere);
        return *ppidl ? NOERROR : E_OUTOFMEMORY;
    }

    *ppidl = NULL;
    return S_FALSE; // success but empty
}


//*****************************************************************************
//
// CCabFolder::CreateIDList
//
// Purpose:
//
//    Creates an item identifier list for the objects in the namespace
//
//
//*****************************************************************************

LPITEMIDLIST CCabFolder::CreateIDList(LPCTSTR pszName, DWORD dwFileSize,
                                      UINT uFileDate, UINT uFileTime, UINT uFileAttribs)
{
    // We'll assume no name is longer than MAX_PATH
    // Note the terminating NULL is already in the sizeof(CABITEM)
    BYTE bBuf[sizeof(CABITEM) + (sizeof(TCHAR) * MAX_PATH) + sizeof(WORD)];
    LPCABITEM pci = (LPCABITEM)bBuf;

    UINT uNameLen = lstrlen(pszName);
    if (uNameLen >= MAX_PATH)
    {
        uNameLen = MAX_PATH;
    }

    pci->wSize = (WORD)(sizeof(CABITEM) + (sizeof(TCHAR) * uNameLen));
    pci->dwFileSize = dwFileSize;
    pci->uFileDate = (USHORT)uFileDate;
    pci->uFileTime = (USHORT)uFileTime;
    pci->uFileAttribs = (USHORT)uFileAttribs & (FILE_ATTRIBUTE_READONLY|
        FILE_ATTRIBUTE_HIDDEN  |
        FILE_ATTRIBUTE_SYSTEM  |
        FILE_ATTRIBUTE_ARCHIVE);
    lstrcpyn(pci->szName, pszName, uNameLen+1);
    pci->cPathChars = 0;
    LPCTSTR psz = pszName;
    while (*psz)
    {
        if ((*psz == TEXT(':')) || (*psz == TEXT('/')) || (*psz == TEXT('\\')))
        {
            pci->cPathChars = (USHORT)(psz - pszName) + 1;
        }

        psz = CharNext(psz);
    }

    // Terminate the IDList
    *(WORD *)(((LPSTR)pci)+pci->wSize) = 0;

    return(ILClone((LPCITEMIDLIST)pci));
}

//*****************************************************************************
//
// CCabFolder::GetPath
//
// Purpose:
//
//        Get the Path for the current pidl
//
// Parameters:
//
//        LPSTR szPath        -    return pointer for path string
//
// Comments:
//
//*****************************************************************************

BOOL CCabFolder::GetPath(LPTSTR szPath)
{
    if (!m_pidlHere || !SHGetPathFromIDList(m_pidlHere, szPath))
    {
        *szPath = TEXT('\0');
        return FALSE;
    }

#ifdef UNICODE
    // NOTE: we use GetShortPathName() to avoid losing characters during the
    // UNICODE->ANSI->UNICODE roundtrip while calling FDICopy()
    // NOTE: It is valid for GetShortPathName()'s src and dest pointers to be the same

    // If this fails, we'll just ignore the error and try to use the long path name
    GetShortPathName(szPath, szPath, MAX_PATH);
#endif // UNICODE

    return(TRUE);
}


void CCabFolder::GetNameOf(LPCABITEM pit, LPSTRRET lpName)
{
    lpName->uType = STRRET_CSTR;
    lpName->cStr[0] = '\0';

    SHFILEINFO sfi;
    if (SHGetFileInfo(pit->szName + pit->cPathChars, 0, &sfi, sizeof(sfi),
        SHGFI_USEFILEATTRIBUTES | SHGFI_DISPLAYNAME))
    {
        StringToStrRet(sfi.szDisplayName, lpName);
    }
}


void CCabFolder::GetPathOf(LPCABITEM pit, LPSTRRET lpName)
{
    TCHAR szPath[MAX_PATH];
    lstrcpy(szPath, pit->szName);
    szPath[pit->cPathChars] = TEXT('\0');
    StringToStrRet(szPath, lpName);
}


void CCabFolder::GetTypeOf(LPCABITEM pit, LPSTRRET lpName)
{
    lpName->uType = STRRET_CSTR;
    lpName->cStr[0] = '\0';

    SHFILEINFO sfi;

    if (SHGetFileInfo(pit->szName + pit->cPathChars, 0, &sfi, sizeof(sfi),
        SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME))
    {
        StringToStrRet(sfi.szTypeName, lpName);
    }
}

//*****************************************************************************
//
// CCabFolder::EnumToList
//
// Purpose:
//
//       This notify callback is called by the FDI routines. It adds the
//       file object from the cab file to the list.
//
// Parameters:
//
//
//
// Comments:
//
//*****************************************************************************


void CALLBACK CCabFolder::EnumToList(LPCTSTR pszFile, DWORD dwSize, UINT date,
                                     UINT time, UINT attribs, LPARAM lParam)
{
    CCabFolder *pThis = (CCabFolder *)lParam;

    pThis->m_lItems.AddItem(pszFile, dwSize, date, time, attribs);
}


HRESULT CCabFolder::InitItems()
{
    switch (m_lItems.GetState())
    {
    case CCabItemList::State_Init:
        return NOERROR;

    case CCabItemList::State_OutOfMem:
        return E_OUTOFMEMORY;

    case CCabItemList::State_UnInit:
    default:
        break;
    }

    // Force the list to initialize
    m_lItems.InitList();

    TCHAR szHere[MAX_PATH];

    // the m_pidl has been set to current dir
    // get the path to the current directory
    if (!GetPath(szHere))
    {
        return(E_UNEXPECTED);
    }

    CCabItems ciHere(szHere);

    if (!ciHere.EnumItems(EnumToList, (LPARAM)this))
    {
        return(E_UNEXPECTED);
    }

    return NOERROR;
}

HRESULT CabFolder_CreateInstance(REFIID riid, void **ppvObj)
{
    HRESULT hres;

    *ppvObj = NULL;

    HINSTANCE hCabinetDll = LoadLibrary(TEXT("CABINET.DLL"));
    if (hCabinetDll)
    {
        FreeLibrary(hCabinetDll);

        CCabFolder *pfolder = new CCabFolder;
        if (pfolder)
            hres = pfolder->QueryInterface(riid, ppvObj);
        else
            hres = E_OUTOFMEMORY;
    }
    else
        hres = E_UNEXPECTED;

    return hres;
}


UINT CCabItemList::GetState()
{
    if (m_uStep == 0)
    {
        if (m_dpaList)
        {
            return(State_Init);
        }

        return(State_OutOfMem);
    }

    return(State_UnInit);
}


BOOL CCabItemList::StoreItem(LPITEMIDLIST pidl)
{
    if (pidl)
    {
        if (InitList() && DPA_InsertPtr(m_dpaList, 0x7fff, (LPSTR)pidl)>=0)
        {
            return(TRUE);
        }

        ILFree(pidl);
    }

    CleanList();
    return FALSE;
}


BOOL CCabItemList::AddItems(LPCABITEM *apit, UINT cpit)
{
    for (UINT i=0; i<cpit; ++i)
    {
        if (!StoreItem(ILClone((LPCITEMIDLIST)apit[i])))
        {
            return FALSE;
        }
    }

    return(TRUE);
}


BOOL CCabItemList::AddItem(LPCTSTR pszName, DWORD dwFileSize,
                           UINT uFileDate, UINT uFileTime, UINT uFileAttribs)
{
    return(StoreItem(CCabFolder::CreateIDList(pszName, dwFileSize, uFileDate, uFileTime,
        uFileAttribs)));
}


int CCabItemList::FindInList(LPCTSTR pszName, DWORD dwFileSize,
                             UINT uFileDate, UINT uFileTime, UINT uFileAttribs)
{
    // TODO: Linear search for now; binary later
    for (int i=DPA_GetPtrCount(m_dpaList)-1; i>=0; --i)
    {
        if (lstrcmpi(pszName, (*this)[i]->szName) == 0)
        {
            break;
        }
    }

    return(i);
}


BOOL CCabItemList::InitList()
{
    switch (GetState())
    {
    case State_Init:
        return(TRUE);

    case State_OutOfMem:
        return FALSE;

    case State_UnInit:
    default:
        m_dpaList = DPA_Create(m_uStep);
        m_uStep = 0;

        return(InitList());
    }
}


void CCabItemList::CleanList()
{
    if (m_uStep != 0)
    {
        m_dpaList = NULL;
        m_uStep = 0;
        return;
    }

    if (!m_dpaList)
    {
        return;
    }

    for (int i=DPA_GetPtrCount(m_dpaList)-1; i>=0; --i)
    {
        ILFree((LPITEMIDLIST)((*this)[i]));
    }

    DPA_Destroy(m_dpaList);
    m_dpaList = NULL;
}
