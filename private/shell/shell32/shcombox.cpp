// shcombox.cpp : Shared shell comboboxEx methods

#include "shellprv.h"
#include "shcombox.h"
#include "filetype.h"
#include "recdocs.h"
#include "ids.h"

//------------------------------------
//  _AddCbxItemToComboBox: Adds the specified item to a comboboxex window
HRESULT _AddCbxItemToComboBox( IN HWND hwndComboEx, IN PCCBXITEM pItem, IN INT_PTR *pnPosAdded )
{
    ASSERT(hwndComboEx);

    //  Convert to COMBOBOXEXITEM.
    COMBOBOXEXITEM cei;
    cei.mask            = pItem->mask;
    cei.iItem           = pItem->iItem;
    cei.pszText         = (LPTSTR)pItem->szText;
    cei.cchTextMax      = ARRAYSIZE(pItem->szText);
    cei.iImage          = pItem->iImage;
    cei.iSelectedImage  = pItem->iSelectedImage;
    cei.iOverlay        = pItem->iOverlay;
    cei.iIndent         = pItem->iIndent;
    cei.lParam          = pItem->lParam;

    LRESULT nPos = CB_ERR;

    nPos = ::SendMessage( hwndComboEx, CBEM_INSERTITEM, nPos, (LPARAM)&cei );

    if (pnPosAdded)
        *pnPosAdded = nPos;

    return nPos < 0 ? S_FALSE : S_OK;
}

//------------------------------------
//  _AddCbxItemToComboBoxCallback: Adds the specified item to a comboboxex window, and invokes
//  a notification callback function if successful.
HRESULT _AddCbxItemToComboBoxCallback( IN HWND hwndComboEx, IN PCBXITEM pItem, IN ADDCBXITEMCALLBACK pfn, IN LPARAM lParam )
{
    INT_PTR iPos = -1;

    if( pfn && E_ABORT == pfn( CBXCB_ADDING, pItem, lParam ) )
        return E_ABORT;

    HRESULT hr = _AddCbxItemToComboBox( hwndComboEx, pItem, &iPos );
    
    if( pfn && S_OK == hr )
    {
        ((CBXITEM*)pItem)->iItem = iPos;
        pfn( CBXCB_ADDED, pItem, lParam );
    }
    return hr;
}

//------------------------------------
//  _MakeCbxItem() - image icon image list indices unknown
//
HRESULT _MakeCbxItem( 
    OUT CBXITEM* pcbi,
    IN  LPCTSTR pszDisplayName, 
    IN  LPVOID pvData, 
    IN  LPCITEMIDLIST pidlIcon, 
    IN  INT_PTR nPos, 
    IN  int iIndent )
{
    int iImage = -1;
    int iSelectedImage = -1;

    if (pidlIcon)
    {
        _GetPidlIcon(pidlIcon, &iImage, &iSelectedImage);
    }
    return _MakeCbxItemKnownImage( pcbi, pszDisplayName, pvData, iImage, iSelectedImage, nPos, iIndent );
}
   
//------------------------------------
//  _MakeCbxItemKnownImage() - image list indices known
HRESULT _MakeCbxItemKnownImage( 
    OUT CBXITEM* pcbi,
    IN  LPCTSTR pszDisplayName, 
    IN  LPVOID pvData, 
    IN  int iImage, 
    IN  int iSelectedImage, 
    IN  INT_PTR nPos, 
    IN  int iIndent )
{
    ASSERT( pcbi );
    ZeroMemory( pcbi, sizeof(*pcbi) );

    lstrcpyn( pcbi->szText, pszDisplayName, ARRAYSIZE(pcbi->szText) );
    pcbi->lParam = (LPARAM)pvData;
    pcbi->iIndent = iIndent;
    pcbi->iItem = nPos;
    pcbi->mask = (CBEIF_TEXT | CBEIF_INDENT | CBEIF_LPARAM);
    if (-1 != iImage)
    {
        pcbi->mask |= CBEIF_IMAGE;
        pcbi->iImage = iImage;
    }
    if (-1 != iSelectedImage)
    {
        pcbi->mask |= CBEIF_SELECTEDIMAGE;
        pcbi->iSelectedImage = iSelectedImage;
    }
    return S_OK;
}

//------------------------------------
HRESULT _MakeCsidlIconCbxItem(
    OUT CBXITEM* pcbi, 
    LPCTSTR pszDisplayName, 
    LPVOID pvData, 
    int nCsidlIcon, 
    INT_PTR nPos, 
    int iIndent )
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlIcon = ((-1 == nCsidlIcon) ? NULL : SHCloneSpecialIDList(NULL, nCsidlIcon, TRUE));

    hr = _MakeCbxItem( pcbi, pszDisplayName, pvData, pidlIcon, nPos, iIndent );
    ILFree(pidlIcon);

    return hr;
}

//------------------------------------
HRESULT _MakeFileTypeCbxItem( 
    OUT CBXITEM* pcbi, 
    IN  LPCTSTR pszDisplayName, 
    IN  LPCTSTR pszExt, 
    IN  LPCITEMIDLIST pidlIcon, 
    IN  INT_PTR nPos, 
    IN  int iIndent )
{
    HRESULT hr = S_OK;
    LPVOID pvData = NULL;
    BOOL fAllocated = FALSE;

    if (!pidlIcon)
    {
        TCHAR szFileName[MAX_PATH] = TEXT("C:\\notexist");       // This is bogus and that's ok

        StrCatBuff(szFileName, pszExt, ARRAYSIZE(szFileName));
        pidlIcon = SHSimpleIDListFromPath(szFileName);
        fAllocated = TRUE;
    }

    if (EVAL(pidlIcon))
    {
        Str_SetPtr((LPTSTR *)&pvData, pszExt);
        hr = _MakeCbxItem( pcbi, pszDisplayName, (LPVOID)pvData, pidlIcon, nPos, iIndent );
    }

    if (fAllocated)
        ILFree((LPITEMIDLIST) pidlIcon);

    return hr;
}

//------------------------------------
HRESULT _MakeResourceAndCsidlStrCbxItem(
    OUT CBXITEM* pcbi, 
    IN  UINT idString, 
    IN  int nCsidlItem, 
    IN  int nCsidlIcon, 
    IN  INT_PTR nPos, 
    IN  int iIndent)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidl = SHCloneSpecialIDList(NULL, nCsidlItem, TRUE);

    if (EVAL(pidl))
    {
        TCHAR szDisplayName[MAX_PATH];   
        TCHAR szPath[MAX_PATH];   
        LPTSTR pszPath = NULL;

        if (!EVAL(LoadString(HINST_THISDLL, idString, szDisplayName, ARRAYSIZE(szDisplayName))))
            szDisplayName[0] = 0;

        SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, SIZECHARS(szPath), NULL);

        Str_SetPtr(&pszPath, szPath);
        hr = _MakeCsidlIconCbxItem( pcbi, szDisplayName, (LPVOID)pszPath, nCsidlIcon, nPos, iIndent);
        ILFree(pidl);
    }

    return hr;
}

//------------------------------------
HRESULT _MakeResourceCbxItem(     
    OUT CBXITEM* pcbi,
    IN  int idString, 
    IN  DWORD dwData, 
    IN  int nCsidlIcon, 
    IN  INT_PTR nPos, 
    IN  int iIndent )
{
    HRESULT hr = E_FAIL;
    TCHAR szDisplayName[MAX_PATH];

    if( EVAL(LoadString(HINST_THISDLL, idString, szDisplayName, ARRAYSIZE(szDisplayName))))
        hr = _MakeCsidlIconCbxItem( pcbi, szDisplayName, (LPVOID) dwData, nCsidlIcon, nPos, iIndent);

    return hr;
}

//------------------------------------
HRESULT _MakeCsidlItemStrCbxItem(
    OUT CBXITEM* pcbi, 
    IN  int nCsidlItem, 
    IN  int nCsidlIcon, 
    IN  INT_PTR nPos, 
    IN  int iIndent)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidl = SHCloneSpecialIDList(NULL, nCsidlItem, TRUE);

    if (EVAL(pidl))
    {
        TCHAR szDisplayName[MAX_PATH];   
        TCHAR szPath[MAX_PATH];   
        LPTSTR pszPath = NULL;

        SHGetNameAndFlags(pidl, SHGDN_NORMAL, szDisplayName, SIZECHARS(szDisplayName), NULL);
        SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, SIZECHARS(szPath), NULL);

        Str_SetPtr(&pszPath, szPath);
        hr = _MakeCsidlIconCbxItem( pcbi, szDisplayName, (LPVOID)pszPath, nCsidlIcon, nPos, iIndent);
        // String is freed in comboboxex
        ILFree(pidl);
    }

    return hr;
}

//------------------------------------
HRESULT _MakeCsidlCbxItem(
    OUT CBXITEM* pcbi, 
    IN  int nCsidlItem, 
    IN  int nCsidlIcon, 
    IN  INT_PTR nPos, 
    IN  int iIndent)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidl = SHCloneSpecialIDList(NULL, nCsidlItem, TRUE);

    if (EVAL(pidl))
    {
        TCHAR szDisplayName[MAX_URL_STRING];

        SHGetNameAndFlags(pidl, SHGDN_NORMAL, szDisplayName, SIZECHARS(szDisplayName), NULL);

        hr = _MakeCsidlIconCbxItem( pcbi, szDisplayName, (LPVOID)pidl, nCsidlIcon, nPos, iIndent);
        ILFree(pidl);
    }

    return hr;
}

//------------------------------------
HRESULT _MakePidlCbxItem( 
    OUT CBXITEM* pcbi, 
    IN  LPITEMIDLIST pidl, 
    IN  LPITEMIDLIST pidlIcon, 
    IN  INT_PTR nPos, 
    IN  int iIndent )
{
    TCHAR szDisplayName[MAX_PATH];

    SHGetNameAndFlags(pidl, SHGDN_NORMAL, szDisplayName, SIZECHARS(szDisplayName), NULL);

    return _MakeCbxItem( pcbi, szDisplayName, (LPVOID) ILClone(pidl), pidlIcon, nPos, iIndent );
}

//------------------------------------
//  Retrieves the system image list indices for the specified ITEMIDLIST
HRESULT _GetPidlIcon(LPCITEMIDLIST pidl, int *piImage, int *piSelectedImage)
{
    HRESULT hr = S_OK;
    IShellFolder *psfParent;
    LPCITEMIDLIST pidlChild;

    hr = SHBindToIDListParent(pidl, IID_IShellFolder, (void **)&psfParent, &pidlChild);
    if (SUCCEEDED(hr))
    {
        ASSERT(g_himlIcons);
        *piImage = SHMapPIDLToSystemImageListIndex(psfParent, pidlChild, piSelectedImage);
        psfParent->Release();
    }
    return hr;
}

//------------------------------------
//  Enumerates children of the indicated special shell item id.
HRESULT _EnumSpecialItemIDs(
    int csidl, 
    DWORD dwSHCONTF, 
    LPFNPIDLENUM_CB pfn, 
    LPVOID pvData)
{
    HRESULT hr;

    if (EVAL(pfn))
    {
        LPITEMIDLIST pidlFolder = SHCloneSpecialIDList(NULL, csidl, TRUE);

        if (EVAL(pidlFolder))
        {
            IShellFolder * psf;

            hr = SHBindToObject(NULL, IID_IShellFolder, pidlFolder, (void **)&psf);
            if (EVAL(SUCCEEDED(hr)))
            {
                IEnumIDList * pEnumIDList;

                hr = psf->EnumObjects(NULL, dwSHCONTF, &pEnumIDList);
                if (EVAL(SUCCEEDED(hr)))
                {
                    LPITEMIDLIST pidl;

                    while (SUCCEEDED(hr) && (S_OK == pEnumIDList->Next(1, &pidl, NULL)))
                    {
                        LPITEMIDLIST pidlFull = ILCombine(pidlFolder, pidl);
                
                        if (EVAL(pidlFull))
                        {
                            hr = pfn(pidlFull, pvData);
                            ILFree(pidlFull);
                        }

                        ILFree(pidl);
                    }

                    pEnumIDList->Release();
                }
                psf->Release();
            }

            ILFree(pidlFolder);
        }
    }

    return S_OK;
}

//------------------------------------
HIMAGELIST WINAPI _GetSystemImageListSmallIcons()
{
    if (g_himlIconsSmall == NULL)
        FileIconInit(FALSE);
    return g_himlIconsSmall;
}

//-------------------------------------------------------------------------//
//  Namespace selector combo methods
//-------------------------------------------------------------------------//
#define SFGAO_TEST_DRIVES     (SFGAO_FOLDER | SFGAO_FILESYSTEM)

//------------------------------------
HRESULT _BuildDrivesList(UINT uiFilter, LPCTSTR pszSeparator, LPCTSTR pszEnd, LPTSTR pszString, DWORD cchSize)
{
    TCHAR szDriveList[MAX_PATH];    // Needs to be 'Z'-'A' (26) * 3 + 1 = 79.
    TCHAR szDrive[3] = TEXT("A:");
    UINT nSlot = 0;
    TCHAR chDriveLetter;
    UINT cchSeparator = lstrlen(pszSeparator);
    UINT cchEnd = lstrlen(pszEnd);

    for (chDriveLetter = TEXT('A'); chDriveLetter <= TEXT('Z'); chDriveLetter++)
    {
        UINT uiDriveType;

        szDrive[0] = chDriveLetter;

        uiDriveType = GetDriveType(szDrive);
        if ((1 != uiDriveType) &&          // Make sure it exists.
            ((-1 == uiFilter) ||
            (uiFilter == uiDriveType)))
        {
            if (nSlot) // Do we need to add a separator before we add the next item?
            {
                StrCpy(szDriveList + nSlot, pszSeparator);  // Size guaranteed to not be a problem.
                nSlot += cchSeparator;
            }

            szDriveList[nSlot++] = chDriveLetter; // terminate list.
            StrCpy(szDriveList + nSlot, pszEnd);  // Size guaranteed to not be a problem.
            nSlot += cchEnd;
        }
    }

    StrCpyN(pszString, szDriveList, cchSize);     // Put back into the final location.
    return S_OK;
}

//----------------------------
HRESULT _MakeMyDocsCbxItem( CBXITEM* pItem, OUT LPTSTR pszPath = NULL )
{
    HRESULT      hr = E_FAIL;
    LPITEMIDLIST pidl;

    if( pszPath ) *pszPath = 0;

    if( (pidl = MyDocsIDList())!= NULL ) 
    {
        TCHAR szDisplayName[MAX_PATH],
              szPath[MAX_PATH],
              *pszPathParam = NULL;

        *szPath = 0;
        SHGetNameAndFlags(pidl, SHGDN_NORMAL, szDisplayName, SIZECHARS(szDisplayName), NULL);
        SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, SIZECHARS(szPath), NULL);

        Str_SetPtr(&pszPathParam, szPath); // for item.lParam.
        if( pszPath && *szPath ) 
            StrCpy( pszPath, szPath ); // output to caller

        hr = _MakeCbxItem( pItem, szDisplayName, pszPathParam, pidl, LISTINSERT_LAST, NO_ITEM_INDENT );
        ILFree( pidl );
    }

    return hr;
}

//----------------------------
HRESULT _MakeMyComputerCbxItem( LPCTSTR pszPathPrepend, CBXITEM* pItem )
{
    HRESULT hr;
    TCHAR szPath[MAX_PATH*2],
          *pszDriveList = szPath;
    int   cchDriveList = ARRAYSIZE(szPath);
    TCHAR szDisplayName[MAX_PATH];
    LPITEMIDLIST pidlIcon = SHCloneSpecialIDList(NULL, CSIDL_DRIVES, TRUE);
    LPTSTR pszPath = NULL;
    
    //  Do prepend if caller requested:
    if( pszPathPrepend && *pszPathPrepend )
    {
        StrCpy( szPath, pszPathPrepend );
        StrCat( szPath, TEXT(";") );
        int cch = lstrlen( szPath );
        ASSERT( ARRAYSIZE(szPath) - cch >= MAX_PATH /* still need MAX_PATH for our drive list. */ );
        cchDriveList -= cch;
        pszDriveList += cch;
    }
    
    //  Finish compiling ';'-delimited list of drives.
    _BuildDrivesList((UINT) -1, TEXT(";"), TEXT(":\\"), pszDriveList, cchDriveList);
    Str_SetPtr(&pszPath, szPath);

    //  Get "My Computer" display name.
    SHGetNameAndFlags(pidlIcon, SHGDN_NORMAL, szDisplayName, SIZECHARS(szDisplayName), NULL);
    
    hr = _MakeCbxItem( pItem, szDisplayName, (LPVOID)pszPath, pidlIcon, LISTINSERT_LAST, NO_ITEM_INDENT );
    ILFree(pidlIcon);

    return hr;
}

//----------------------------
HRESULT _MakeLocalHardDrivesCbxItem( CBXITEM* pItem )
{
    HRESULT hr = E_FAIL;
    TCHAR szTemplate[MAX_PATH];

    if (LoadString(HINST_THISDLL, IDS_SNS_LOCALHARDDRIVES, szTemplate, ARRAYSIZE(szTemplate)))
    {
        TCHAR szHardDriveList[MAX_PATH];

        hr = _BuildDrivesList(DRIVE_FIXED, TEXT(";"), TEXT(":"), szHardDriveList, ARRAYSIZE(szHardDriveList));
        if (EVAL(SUCCEEDED(hr)))   
        {
            TCHAR szPath[MAX_PATH];

            hr = _BuildDrivesList(DRIVE_FIXED, TEXT(";"), TEXT(":\\"), szPath, ARRAYSIZE(szPath));
            if (EVAL(SUCCEEDED(hr)))   
            {
                TCHAR szFirstDrive[MAX_PATH];
                TCHAR szDisplayName[MAX_PATH];
                LPITEMIDLIST pidlIcon;

                StrCpyN(szFirstDrive, szPath, 4);       // "C:\;D:\;..." will terminate at the first ';'
                wnsprintf(szDisplayName, ARRAYSIZE(szDisplayName), szTemplate, szHardDriveList);
                pidlIcon = ILCreateFromPath(szFirstDrive);
            
                if (EVAL(pidlIcon))
                {
                    LPTSTR pszPath = NULL;
                    Str_SetPtr(&pszPath, szPath);

                    hr = _MakeCbxItem( pItem, szDisplayName, (LPVOID) pszPath, pidlIcon, LISTINSERT_LAST, ITEM_INDENT );
                    ILFree(pidlIcon);
                }
            }
        }
    }

    return hr;
}

//------------------------------------
HRESULT _MakeMappedDrivesCbxItem( CBXITEM* pItem, LPITEMIDLIST pidl)
{
    ULONG ulAttrs = SFGAO_TEST_DRIVES;
    TCHAR szPath[MAX_PATH];
    HRESULT hr = S_OK;

    // User SHGetAttributesOf & Name
    if( SUCCEEDED( (hr = SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, SIZECHARS(szPath), &ulAttrs)) ) )
    {
        if( IsFlagSet(ulAttrs, SFGAO_TEST_DRIVES) )
        {
            TCHAR szDisplayName[MAX_URL_STRING];       
            LPTSTR pszPath = NULL;

            SHGetNameAndFlags(pidl, SHGDN_NORMAL, szDisplayName, SIZECHARS(szDisplayName), NULL);
            Str_SetPtr(&pszPath, szPath);

            hr = _MakeCbxItem( pItem, szDisplayName, (LPVOID)pszPath, pidl, LISTINSERT_LAST, ITEM_INDENT );
        }
        else
            hr = S_FALSE;
    }
    return hr;
}

HRESULT _MakeNethoodDirsCbxItem( CBXITEM* pItem, LPITEMIDLIST pidl )
{
    // Add this if the item is a link to a directory.  This will require
    // dereferencing the link.
    IShellFolder *psf;
    LPCITEMIDLIST pidlLast;
    HRESULT hr = SHBindToIDListParent(pidl, IID_IShellFolder, (void **)&psf, &pidlLast);
    if (SUCCEEDED(hr))
    {
        IShellLink *psl;
        psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&pidlLast, IID_IShellLink, NULL, (void **)&psl);
        if (SUCCEEDED(hr))
        {
            WIN32_FIND_DATA wfd;
            TCHAR szPath[MAX_URL_STRING];

            if (S_OK == psl->GetPath(szPath, ARRAYSIZE(szPath), &wfd, 0))
            {
                if (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
                {
                    LPTSTR pszPath = NULL;
                    Str_SetPtr(&pszPath, szPath);
                    hr = _MakeCbxItemKnownImage( pItem, szPath, (void *) pszPath,
                                                 3, // Folder Icon 
                                                 3, // Folder Icon
                                                 LISTINSERT_LAST, ITEM_INDENT );
                }
            }
            psl->Release();
        }
        psf->Release();
    }

    if( hr != S_OK )
        hr = E_FAIL;

    return hr;
}

//------------------------------------
HRESULT _MakeRecentFolderCbxItem( CBXITEM* pItem )
{
    LPITEMIDLIST pidlIcon = SHCloneSpecialIDList(NULL, CSIDL_RECENT, TRUE);
    HRESULT hr = E_FAIL;

    if (pidlIcon)
    {
        TCHAR szDisplayName[MAX_PATH];
        LPTSTR pszParam = NULL;

        if (SUCCEEDED(SHGetNameAndFlags(pidlIcon, SHGDN_NORMAL, szDisplayName, SIZECHARS(szDisplayName), NULL)))
        {
            // Ok, we are going to build both the item and the list at once, so for now add the item with no path
            // This will allow us to 
            Str_SetPtr(&pszParam, NAMESPACECOMBO_RECENT_PARAM);
            hr = _MakeCbxItem( pItem, szDisplayName, (LPVOID)pszParam, pidlIcon, LISTINSERT_LAST, NO_ITEM_INDENT );
        }
    }
    return hr;
}

//------------------------------------
HRESULT _MakeBrowseForCbxItem( CBXITEM* pItem )
{
    HRESULT hr = E_FAIL;
    TCHAR szDisplayName[MAX_PATH];

    if (EVAL(LoadString(HINST_THISDLL, IDS_SNS_BROWSER_FOR_DIR, szDisplayName, ARRAYSIZE(szDisplayName))))
        hr = _MakeCbxItem( pItem, szDisplayName, (LPVOID) INVALID_HANDLE_VALUE, NULL, LISTINSERT_LAST, NO_ITEM_NOICON_INDENT );

    return hr;
}

//------------------------------------
HRESULT _MakeNetworkPlacesCbxItem( CBXITEM* pItem, LPVOID lParam )
{
    LPITEMIDLIST pidlIcon = SHCloneSpecialIDList(NULL, CSIDL_NETWORK, TRUE);
    HRESULT hr = E_FAIL;

    if (EVAL(pidlIcon))
    {
        TCHAR szTitle[MAX_PATH];

        if (EVAL(LoadString(HINST_THISDLL, IDS_SNS_MYNETWORKPLACES, szTitle, ARRAYSIZE(szTitle))))
            hr  = _MakeCbxItem( pItem, szTitle, lParam, pidlIcon, LISTINSERT_LAST, NO_ITEM_INDENT );
            
        ILFree(pidlIcon);
    }

    return S_OK;
}

//------------------------------------
HRESULT _EnumRecentAndGeneratePath( BOOL fAddEntries, LPFNRECENTENUM_CB pfn, LPVOID pvParam )
{
    DWORD dwMax;
    HANDLE hEnum = CreateSharedRecentMRUList(TEXT("Folder"), &dwMax, SRMLF_COMPNAME);
    ASSERT(pfn);

    if (EVAL(hEnum))
    {
        LPITEMIDLIST pidlRecent = SHCloneSpecialIDList(NULL, CSIDL_RECENT, TRUE);
        if (pidlRecent)
        {
            int nResult = 1;

            for (int nIndex = 0; (nIndex < (LONG)dwMax) && (-1 != nResult); nIndex++)
            {
                LPITEMIDLIST pidlItem = NULL;

                nResult = EnumSharedRecentMRUList(hEnum, nIndex, NULL, &pidlItem);
                if (pidlItem)
                {
                    ASSERT(-1 != nResult);  // I assume this means error.

                    IShellFolder *psf;
                    if (SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidlRecent, (void **)&psf)))
                    {
                        IShellLink *psl;
                        if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&pidlItem, IID_IShellLink, NULL, (void **)&psl)))
                        {
                            //  reuse szPath here
                            TCHAR szPath[MAX_PATH];
                            if (SUCCEEDED(psl->GetPath(szPath, ARRAYSIZE(szPath), NULL, 0)))
                                pfn(szPath, fAddEntries, pvParam);
                    
                            psl->Release();
                        }
                        psf->Release();
                    }
                    ILFree(pidlItem);
                }
                else
                    ASSERT(-1 == nResult);
            }

            ILFree(pidlRecent);
        }

        FreeMRUList(hEnum);
    }

    return S_OK;
}

//  Enum callback param
typedef struct
{
    HWND                hwndComboBox;
    ADDCBXITEMCALLBACK  pfn; 
    LPARAM              lParam;
} ENUMITEMPARAM;

//------------------------------------
HRESULT _PopulateMappedDrivesCB( LPITEMIDLIST pidl, LPVOID pv ) 
{ 
    CBXITEM item;
    ENUMITEMPARAM* peip = (ENUMITEMPARAM*)pv;

    HRESULT hr = _MakeMappedDrivesCbxItem( &item, pidl);
    if( S_OK == hr )
    {
        item.iID = CSIDL_DRIVES;
        hr = _AddCbxItemToComboBoxCallback( peip->hwndComboBox, &item, peip->pfn, peip->lParam );
        item.iID = -1;
    }
    return hr;    
};

//------------------------------------
HRESULT _PopulateRecentPathEntriesCB( LPCTSTR pszPath, BOOL fAddEntries, LPVOID pv )
{
    HRESULT hr = S_OK;
    ENUMITEMPARAM* peip = (ENUMITEMPARAM*)pv;

    if( fAddEntries )
    {
        LPTSTR psz = NULL;
        Str_SetPtr(&psz, pszPath);

        CBXITEM item;
        hr = _MakeCbxItemKnownImage( &item, pszPath, (LPVOID)psz,
                                     3, 3, LISTINSERT_LAST, ITEM_INDENT );
        if( S_OK == hr )
        {
            item.iID = CSIDL_DRIVES;
            hr = _AddCbxItemToComboBoxCallback( peip->hwndComboBox, &item, peip->pfn, peip->lParam );
            item.iID = -1;
        }
    }
    return hr;
}

//------------------------------------
HRESULT _PopulateNethoodDirsCB( LPITEMIDLIST pidl, LPVOID pv ) 
{ 
    CBXITEM item;
    ENUMITEMPARAM* peip = (ENUMITEMPARAM*)pv;

    HRESULT hr = _MakeNethoodDirsCbxItem( &item, pidl );
    if( S_OK == hr )
    {
        item.iID = CSIDL_NETHOOD;
        hr = _AddCbxItemToComboBoxCallback( (HWND)peip->hwndComboBox, &item, peip->pfn, peip->lParam );
        item.iID = -1;
    }
    return hr;    
};

//------------------------------------
HRESULT WINAPI _PopulateNamespaceCombo( 
    IN HWND hwndComboBoxEx,
    IN OPTIONAL ADDCBXITEMCALLBACK pfn,
    IN OPTIONAL LPARAM lParam )
{
    ASSERT(hwndComboBoxEx);

    CBXITEM          item;
    HRESULT          hr;
    ENUMITEMPARAM    eip;
    TCHAR            szMyDocsPath[MAX_PATH];

    eip.hwndComboBox = hwndComboBoxEx;
    eip.pfn          = pfn;
    eip.lParam       = lParam;
    *szMyDocsPath    = 0;

    ::SendMessage( hwndComboBoxEx, CB_RESETCONTENT, 0, 0L );

    // My Documents
    if( SUCCEEDED( _MakeMyDocsCbxItem( &item, szMyDocsPath ) ) )
    {
        item.iID = CSIDL_PERSONAL;
        if( E_ABORT == (hr = _AddCbxItemToComboBoxCallback( hwndComboBoxEx, &item, pfn, lParam )) )
            return hr;
        item.iID = -1;

        //  If My Docs has been redirected to a remote share, we'll want to prepend its path
        //  to our My Computer path list; otherwise it'll be missed.
        if( !PathIsNetworkPath( szMyDocsPath ) )
            *szMyDocsPath = 0; // it's local, so blow it off.
    }
    
    // Desktop
    if( SUCCEEDED( _MakeCsidlItemStrCbxItem( &item, 
                        CSIDL_DESKTOPDIRECTORY, 
                        CSIDL_DESKTOP, 
                        LISTINSERT_LAST, 
                        NO_ITEM_INDENT )) )
    {
        item.iID = CSIDL_DESKTOP;
        if( E_ABORT == (hr = _AddCbxItemToComboBoxCallback( hwndComboBoxEx, &item, pfn, lParam )) )
            return hr;
        item.iID = -1; 
    }

    //  My Computer and children
    if( SUCCEEDED( _MakeMyComputerCbxItem( szMyDocsPath /* see above */, &item ) ) )
    {
        if( SUCCEEDED( (hr = _AddCbxItemToComboBoxCallback( hwndComboBoxEx, &item, pfn, lParam )) ) )
        {
            //  Local hard drives
            if( SUCCEEDED( _MakeLocalHardDrivesCbxItem( &item ) ) )
            {
                item.iID = CBX_CSIDL_LOCALDRIVES;
                hr = _AddCbxItemToComboBoxCallback( hwndComboBoxEx, &item, pfn, lParam );
                item.iID = -1;

                if( SUCCEEDED(hr) )
                    hr = _EnumSpecialItemIDs( CSIDL_DRIVES, (SHCONTF_FOLDERS|SHCONTF_NONFOLDERS), 
                                              _PopulateMappedDrivesCB, &eip );
            }
        }
        if( E_ABORT == hr )
            return hr;
    }

#ifdef _POPULATE_NETHOOD_ITEMS_
    // My Network Places and nethood dirs...
    if( SUCCEEDED( _MakeNetworkPlacesCbxItem( &item, NULL ) ) )
    {
        item.iID = CSIDL_NETWORK;
        hr = _AddCbxItemToComboBoxCallback( hwndComboBoxEx, &item, pfn, lParam );
        item.iID = -1;

        if( SUCCEEDED( hr ) )
            hr = _EnumSpecialItemIDs(CSIDL_NETHOOD, (SHCONTF_FOLDERS|SHCONTF_NONFOLDERS|SHCONTF_INCLUDEHIDDEN), 
                                     _PopulateNethoodDirsCB, &eip );
        if( E_ABORT == hr )
            return hr;
    }
#endif //_POPULATE_NETHOOD_ITEMS_

#ifdef _POPULATE_RECENT_ITEMS_
    //  Recent folder and paths...
    if( SUCCEEDED( _MakeRecentFolderCbxItem( &item ) ) )
    {
        if( SUCCEEDED( (hr = _AddCbxItemToComboBoxCallback( hwndComboBoxEx, &item, pfn, lParam )) ) )
            _EnumRecentAndGeneratePath( TRUE, _PopulateRecentPathEntriesCB, hwndComboBoxEx );

        if( E_ABORT == hr )
            return hr;
    }
#endif //_POPULATE_RECENT_ITEMS_

    //  Browse...
    if( SUCCEEDED( _MakeBrowseForCbxItem( &item ) ) )
    {
        if( E_ABORT == (hr = _AddCbxItemToComboBoxCallback( hwndComboBoxEx, &item, pfn, lParam )) )
            return hr;
    }

    return hr;
}

//------------------------------------
LONG_PTR _GetNamespaceComboItemText(
    IN HWND hwndComboBox, 
    IN INT_PTR iItem,
    IN BOOL fPath, 
    OUT LPTSTR psz, 
    IN int cch)
{
    //  Devnote: Keep logic synched with internal version
    //  _GetCurItemTextAndIndex()

    // Share code with get_String and the save user input code.
    TCHAR szItemName[MAX_PATH];

    // We don't trust the comboex to handle the edit text properly so try to compensate...
    INT_PTR iCurSel = ::SendMessage( hwndComboBox, CB_GETCURSEL, 0, 0L );
    ::GetWindowText(hwndComboBox, psz, cch);

    if ( CB_ERR != iItem )
    {
        if (::SendMessage(hwndComboBox, CB_GETLBTEXT, (WPARAM)iItem, (LPARAM)szItemName) == CB_ERR)
            szItemName[0]=0;

        LPCTSTR pszString = (LPCTSTR)::SendMessage(hwndComboBox, CB_GETITEMDATA, iItem, 0);
        if (!EVAL(pszString) || ((void *)pszString == INVALID_HANDLE_VALUE) ||
           (iItem == iCurSel && lstrcmp(psz, szItemName) != 0 /* combo edit/combo dropdown mismatch!*/))
            iItem = CB_ERR;
        else
            StrCpyN(psz, fPath? pszString : szItemName, cch);
    }
    return iItem;
}

//------------------------------------
LONG_PTR _GetNamespaceComboSelItemText( HWND hwndComboBox, BOOL fPath, LPTSTR psz, int cch )
{
    //  Devnote: Keep synched with internal version
    //  _GetNamespaceComboSelItemText()
    LRESULT iSel = ::SendMessage( hwndComboBox, CB_GETCURSEL, 0, 0);
    return _GetNamespaceComboItemText( hwndComboBox, iSel, fPath, psz, cch );
}

//------------------------------------
LRESULT _DeleteNamespaceComboItem( LPNMHDR pnmh )
{
    PNMCOMBOBOXEX pnmce = (PNMCOMBOBOXEX)pnmh;
    if (pnmce->ceItem.lParam && (INVALID_HANDLE_VALUE != (void *)pnmce->ceItem.lParam))
        Str_SetPtr((LPTSTR *)&(pnmce->ceItem.lParam), NULL);
    return 1L;
}

//-------------------------------------------------------------------------//
//  File Associations selector combo methods
//-------------------------------------------------------------------------//

//  Hack alert - from filetype.cpp:
extern BOOL ExtToTypeNameAndId( LPCTSTR pszExt, LPTSTR pszDesc, DWORD *pdwDesc, LPTSTR pszId, DWORD *pdwId );

//----------------------------------------
HRESULT _AddFileTypes( HWND hwndComboBox, ADDCBXITEMCALLBACK pfn, LPARAM lParam )
{
    HRESULT hr = S_OK;
    DWORD dwSubKey = 0;
    TCHAR szDesc[MAX_PATH];
    TCHAR szClassesKey[MAX_PATH];   // string containing the classes key
    TCHAR szId[MAX_PATH];
    TCHAR szShellCommandValue[MAX_PATH];
    DWORD dwName;
    DWORD dwId;
    DWORD dwClassesKey;
    DWORD dwAttributes;
    BOOL bRC1;
    BOOL fPastTypes = FALSE;
    LONG err;
    HKEY hkeyFT = NULL;
    BOOL bAbort = FALSE;

    // Enumerate extensions from registry to get file types
    dwClassesKey = ARRAYSIZE(szClassesKey);
    while( !bAbort && SHEnumKeyEx(HKEY_CLASSES_ROOT, dwSubKey, szClassesKey, &dwClassesKey) != ERROR_NO_MORE_ITEMS)
    {
        szId[0] = TEXT('\0');
        dwAttributes = 0;

        if(*szClassesKey == TEXT('.'))  // find the file type identifier and description from the extension
        {
            dwName = SIZEOF(szDesc);
            dwId = SIZEOF(szId);
            bRC1 = ExtToTypeNameAndId(szClassesKey, szDesc, &dwName, szId, &dwId);

            fPastTypes = TRUE;  // We are in the types now.
            if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, szId, NULL, KEY_READ, &hkeyFT))
            {
                dwAttributes = GetFileTypeAttributes(hkeyFT);
                if(!(dwAttributes & FTA_Exclude))
                    dwAttributes |= FTA_HasExtension;

                if(!bRC1)
                {
                    // see if there is a HKEY_CLASSES_ROOT\[.Ext]\Shell\Open\Command value
                    err = SIZEOF(szShellCommandValue);
                    err = SHGetValue(hkeyFT, c_szShellOpenCommand, NULL, NULL, (void *)szShellCommandValue, (LPDWORD)&err);
                    if (err != ERROR_SUCCESS || !(*szShellCommandValue))
                        dwAttributes = FTA_Exclude;
                    else
                        dwAttributes |= FTA_ExtShellOpen;
                }
            }
        }
        else
        {
            if (fPastTypes)
                break;          // We are done because we passed the types.

            RegOpenKeyEx(HKEY_CLASSES_ROOT, szClassesKey, NULL, KEY_READ, &hkeyFT);
            if((dwAttributes = GetFileTypeAttributes(hkeyFT)) & FTA_Show)
            {
                StrCpyN(szId, szClassesKey, ARRAYSIZE(szId));
                dwName = SIZEOF(szDesc);
                err = SHQueryValueEx(hkeyFT, NULL, NULL, NULL, (void *)szDesc, &dwName);
                if(err != ERROR_SUCCESS || !*szDesc)
                    StrCpyN(szDesc, szClassesKey, ARRAYSIZE(szDesc));
                *szClassesKey = TEXT('\0');
            }
        }

        //TraceMsg(TF_ALWAYS, "FT RegEnum HKCR szClassKey=%s szId=%s dwAttributes=%d",       szClassesKey, szId, dwAttributes);
        if((!(dwAttributes & FTA_Exclude)) && ((dwAttributes & FTA_Show) || (dwAttributes & FTA_HasExtension) || (dwAttributes & FTA_ExtShellOpen)))
        {
            if( E_ABORT == _AddFileType( hwndComboBox, szDesc, szClassesKey, NULL, NO_ITEM_INDENT, pfn, lParam ) )
            {
                hr = E_ABORT; //  report failure only on abort.
                bAbort = TRUE;
            }
        }
                
        if (hkeyFT)
        {
            RegCloseKey(hkeyFT);
            hkeyFT = NULL;
        }

        dwSubKey++;
        dwClassesKey = ARRAYSIZE(szClassesKey);
    }

    if (hkeyFT) // we may not release the last one because of the break above
    {
        RegCloseKey(hkeyFT);
        hkeyFT = NULL;
    }
    
    TCHAR szFolder[MAX_PATH];
    if(!bAbort && EVAL(LoadString(HINST_THISDLL, IDS_FOLDERTYPENAME, szFolder, ARRAYSIZE(szFolder))))
    {
        LPITEMIDLIST pidlIcon = SHCloneSpecialIDList(NULL, CSIDL_RECENT, TRUE);

        if (EVAL(pidlIcon))
        {
            if( E_ABORT == _AddFileType( hwndComboBox, szFolder, TEXT("."), pidlIcon, NO_ITEM_INDENT, pfn, lParam ) )
            {
                hr = E_ABORT; //  report failure only on abort.
                bAbort = TRUE;
            }
            ILFree(pidlIcon);
        }
    }

    return hr;
}

//----------------------------------------
HRESULT _AddFileType( 
    HWND hwndComboBox, 
    LPCTSTR pszDisplayName, 
    LPCTSTR pszExt, 
    LPCITEMIDLIST pidlIcon, 
    int iIndent,
    ADDCBXITEMCALLBACK pfn,
    LPARAM lParam )
{
    HRESULT hr = S_OK;
    LRESULT nIndex = CB_ERR;
    BOOL    bExists = FALSE;

    LRESULT lRet = ::SendMessage( hwndComboBox, CB_FINDSTRINGEXACT, (WPARAM)0, (LPARAM) pszDisplayName );
    nIndex = lRet;
    
    // Is the string already in the list?
    if (CB_ERR != nIndex)
    {
        // Yes, so we want to combine our extension with the current extension or extension list
        // and erase the old one.  Then we can continue to add it below.
        LPTSTR pszOldExt = NULL;

        lRet = SendMessage( hwndComboBox, CB_GETITEMDATA, nIndex, 0 );
        if( !(0L == lRet || CB_ERR == lRet) )
        {
            pszOldExt = (LPTSTR)lRet;
            UINT cchLen = lstrlen(pszOldExt) + 1 + lstrlen(pszExt) + 1;

            //Note! We use ReAlloc because that's what Str_SetPtr uses.
            LPTSTR pszNewExt = (LPTSTR)ReAlloc(pszOldExt, sizeof(TCHAR) * cchLen);
            if (pszNewExt)
            {
                StrCat(pszNewExt, TEXT(";"));
                StrCat(pszNewExt, pszExt);
                lRet = ::SendMessage( hwndComboBox, CB_SETITEMDATA, (WPARAM)nIndex, (LPARAM)pszNewExt );
            }
            bExists = TRUE;
        }
    }

    if( !bExists )
    {
        // No, so we can add it.
        TCHAR szString[MAX_URL_STRING];
        INT_PTR nPos = 0;
        INT_PTR nLast = CB_ERR;
        
        lRet = ::SendMessage( hwndComboBox, CB_GETCOUNT, 0, 0 );

        if( lRet == CB_ERR )
            return E_FAIL;

        nLast = lRet - 1;
        *szString = 0;

        lRet = ::SendMessage( hwndComboBox, CB_GETLBTEXT, (WPARAM)nLast, (LPARAM)szString );

        if( lRet == CB_ERR )
            return E_FAIL;

        // Base case, does his the new string need to be inserted into the end?
        if ((-1 == nLast) || (0 > StrCmp(szString, pszDisplayName)))
        {
            // Yes, so add it to the end.
            CBXITEM item;
            hr = _MakeFileTypeCbxItem( &item, pszDisplayName, pszExt, pidlIcon, (nLast + 1), iIndent );
            if( SUCCEEDED( hr ) )
                hr = _AddCbxItemToComboBoxCallback( hwndComboBox, &item, pfn, lParam );
        }
        else
        {
#ifdef DEBUG
            INT_PTR nCycleDetector = nLast + 5;
#endif // DEBUG
            BOOL bDisplayName = TRUE;
            do
            {
                //  Determine ordered insertion point:
                INT_PTR nTest = nPos + ((nLast - nPos) / 2);
                bDisplayName = CB_ERR != ::SendMessage( hwndComboBox, CB_GETLBTEXT, (WPARAM)nTest, (LPARAM)szString );

                if( bDisplayName )
                {
                    // Does the string need to before nTest?
                    if (0 > StrCmp(pszDisplayName, szString))
                        nLast = nTest;  // Yes
                    else
                    {
                        if (nPos == nTest)
                            nPos++;
                        else
                            nPos = nTest;  // No
                    }

#ifdef DEBUG
                    ASSERT(nCycleDetector);   // Make sure we converge.
                    nCycleDetector--;
#endif // DEBUG
                }

            } while( bDisplayName && nLast - nPos );
            
            if( bDisplayName )
            {
                CBXITEM item;
                hr = _MakeFileTypeCbxItem( &item, pszDisplayName, pszExt, pidlIcon, nPos, iIndent );
                if( SUCCEEDED( hr ) )
                    hr = _AddCbxItemToComboBoxCallback( hwndComboBox, &item, pfn, lParam );
            }
        }
    }

    return hr;
}

//------------------------------------
HRESULT _PopulateFileAssocCombo( HWND hwndComboBoxEx, ADDCBXITEMCALLBACK pfn, LPARAM lParam )
{
    HRESULT hr = S_OK;
    ASSERT(hwndComboBoxEx);

    ::SendMessage( hwndComboBoxEx, CB_RESETCONTENT, 0, 0L );

    // Recent Folder
    if( E_ABORT == (hr = _AddFileTypes( hwndComboBoxEx, pfn, lParam )) )
        return hr;

    
    // Now add this to the top of the list.
    CBXITEM item;
    hr = _MakeResourceCbxItem( &item, IDS_SNS_ALL_FILE_TYPES, FILEASSOCIATIONSID_ALLFILETYPES, 
                               -1, LISTINSERT_FIRST, NO_ITEM_NOICON_INDENT );
    if( SUCCEEDED( hr ) )
    {
        if( E_ABORT == (hr = _AddCbxItemToComboBoxCallback( hwndComboBoxEx, &item, pfn, lParam )) )
            return hr;
    }

    return hr;
}

//------------------------------------
LPVOID _getFileAssocComboData( HWND hwndComboBox )
{
    LRESULT nSelected = ::SendMessage( hwndComboBox, CB_GETCURSEL, 0, 0);

    if (-1 == nSelected)
        return NULL;

    return (LPVOID) ::SendMessage( hwndComboBox, CB_GETITEMDATA, nSelected, 0);
}

//------------------------------------
DWORD _getFileAssocComboID( HWND hwndComboBox )
{
    DWORD dwID = 0;
    LPVOID pvData = _getFileAssocComboData( hwndComboBox );

    // Is this an ID?
    if (pvData && ((DWORD_PTR)pvData <= FILEASSOCIATIONSID_MAX))
    {
        // Yes, so let's get it.
        dwID = PtrToUlong(pvData);
    }

    return dwID;
}


//------------------------------------
LONG _GetFileAssocComboSelItemText( IN HWND hwndComboBox, OUT LPTSTR *ppszText )
{
    ASSERT( hwndComboBox );
    *ppszText = 0;

    int nSel;
    if( (nSel = (LONG)::SendMessage( hwndComboBox, CB_GETCURSEL, 0, 0L )) >= 0 )
    {
        DWORD dwID = _getFileAssocComboID( hwndComboBox );

        if (dwID > FILEASSOCIATIONSID_FILE_PATH)
            *ppszText = StrDup(TEXT(".*"));
        else
            *ppszText = StrDup((LPCTSTR)_getFileAssocComboData( hwndComboBox ) );
    }
    return nSel;
}


//------------------------------------
LRESULT _DeleteFileAssocComboItem( IN LPNMHDR pnmh )
{
    PNMCOMBOBOXEX pnmce = (PNMCOMBOBOXEX)pnmh;
    if (pnmce->ceItem.lParam)
    {
        // Is this a pidl?
        if ((pnmce->ceItem.lParam) > FILEASSOCIATIONSID_MAX)
        {
            // Yes, so let's free it.
            Str_SetPtr((LPTSTR *)&pnmce->ceItem.lParam, NULL);
        }
    }
    return 1L;
}
