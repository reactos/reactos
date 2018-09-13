/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    filemru.cpp

Abstract:

    This module contains the functions for implementing file mru
    in file open and file save dialog boxes

Revision History:
    01/22/98                arulk                   created
 
--*/
#include "comdlg32.h"
#include <shellapi.h>
#include <shlobj.h>
#include <shsemip.h>
#include <shellp.h>
#include <commctrl.h>
#include <ole2.h>
#include "cdids.h"

#include <coguid.h>
#include <shlguid.h>
#include <shguidp.h>
#include <oleguid.h>

#include <commdlg.h>
#include "filemru.h"

#ifndef ASSERT
#define ASSERT Assert
#endif


#define REGSTR_PATH_FILEMRU     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\OpenSaveMRU\\")
#define REGSTR_PATH_LASTVISITED  TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\LastVisitedMRU\\")

HANDLE CreateMRU(LPCTSTR pszExt, int nMax)
{
    TCHAR szRegPath[256];
    MRUINFO mi =  {
        sizeof(MRUINFO),
        nMax,
        MRU_CACHEWRITE,
        HKEY_CURRENT_USER,
        szRegPath,
        NULL        // NOTE: use default string compare
    };

    //Get the Registry path for the given file type MRU
    lstrcpy(szRegPath,REGSTR_PATH_FILEMRU);
    StrCatBuff(szRegPath, pszExt ? pszExt : TEXT("*"), ARRAYSIZE(szRegPath));
    
    //Call the comctl32 mru implementation to load the MRU from
    //the registry
    return CreateMRUList(&mi);
}

BOOL GetMRUEntry(HANDLE hMRU, int iIndex, LPTSTR lpString, UINT cbSize)
{
     //Check for valid parameters
     if(!lpString || !cbSize || !hMRU)
     { 
         return FALSE;
     }
     
    //Check for valid index
    if (iIndex < 0 || iIndex > EnumMRUList(hMRU, -1, NULL, 0))
    {
        return FALSE;
    }

    if ((EnumMRUList(hMRU, iIndex, lpString, cbSize) > 0 ))
    {
        return TRUE;
    }
    return FALSE;
}

typedef struct {
    HANDLE mru;
    LPTSTR psz;
} EXTMRU, *PEXTMRU;

STDAPI_(int) _FreeExtMru(LPVOID pvItem, LPVOID pvData)
{
    PEXTMRU pem = (PEXTMRU) pvItem;

    if (pem)
    {
        ASSERT(pem->psz);
        ASSERT(pem->mru);

        LocalFree(pem->psz);
        FreeMRUList(pem->mru);

        LocalFree(pem);
        return TRUE;
    }
    return FALSE;
}

STDAPI_(int) _ExtMruFindExt(LPVOID pvFind, LPVOID pvItem, LPARAM pvParam)
{
    ASSERT(pvFind && pvItem);

    return StrCmp(((PEXTMRU)pvItem)->psz, (LPCTSTR)pvFind);
}

PEXTMRU _AllocExtMru(LPCTSTR pszExt, int nMax)
{
    PEXTMRU pem = (PEXTMRU) LocalAlloc(LPTR, SIZEOF(EXTMRU));

    if (pem)
    {
        pem->psz = StrDup(pszExt);
        pem->mru = CreateMRU (pszExt, nMax);

        if (pem->psz && pem->mru)
            return pem;

        _FreeExtMru(pem, NULL);
    }

    return NULL;
}

        
        
HDPA _CreateExtMruDpa(LPCTSTR pszFilter, int nMax, int *pcItems)
{
    //Convert the filter string of form *.c;*.cpp;*.h into form
    // *.c\0*.cpp\0*.h\0. Also count the file types

    LPTSTR pszFree = StrDup(pszFilter);
    *pcItems = 0;

    if (pszFree)
    {
        HDPA hdpa = DPA_Create(4);

        if (hdpa)
        {
            LPTSTR pszNext = pszFree;
            int cItems = 0;
            LPTSTR pszSemi;
            do
            {
                pszSemi = StrChr(pszNext, CHAR_SEMICOLON);

                if (pszSemi) 
                    *pszSemi = CHAR_NULL;
                    
                LPTSTR pszExt = PathFindExtension(pszNext);

                if (*pszExt)
                {
                    //  walk past the dot...
                    pszExt++;

                    //  make sure this extension isnt already in the dpa
                    if (-1 == DPA_Search(hdpa, pszExt, 0, _ExtMruFindExt, NULL, 0))
                    {
                        PEXTMRU pem = _AllocExtMru(pszExt, nMax);
                        if (!pem)
                            break;

                        DPA_SetPtr(hdpa, cItems++,  (void *)pem);
                    }
                }

                //  we only have a next if there was more than one...
                if (pszSemi)
                    pszNext = pszSemi + 1;    
                    
            } while (pszSemi);

            *pcItems = cItems;
        }

        LocalFree(pszFree);
        return hdpa;
    }

    return NULL;
}

BOOL LoadMRU(LPCTSTR pszFilter, HWND hwndCombo, int nMax)
{   
    //Check if valid filter string is passed
    if (!pszFilter || !pszFilter[0] || nMax <= 0)
    {
        return FALSE;
    }
    
    //First reset the hwndCombo
    SendMessage(hwndCombo, CB_RESETCONTENT, (WPARAM)0L, (LPARAM)0L);

    int cDPAItems;   
    HDPA hdpa = _CreateExtMruDpa(pszFilter, nMax, &cDPAItems);

    if (hdpa)
    {
        TCHAR szFile[MAX_PATH];
        //Set the comboboxex item values
        COMBOBOXEXITEM  cbexItem = {0};
        cbexItem.mask = CBEIF_TEXT;                 // This combobox displays only text
        cbexItem.iItem = -1;                        // Always insert the item at the end
        cbexItem.pszText = szFile;                  // This buffer contains the string
        cbexItem.cchTextMax = ARRAYSIZE(szFile);    // Size of the buffer

        //Now load the hwndcombo with file list from MRU.
        //We use a kind of round robin algorithm for filling
        //the mru. We start with first MRU and try to fill the combobox
        //with one string from each mru. Until we have filled the required
        //strings or we have exhausted all strings in the mrus

        for (int j = 0; nMax > 0; j++)
        {
            //Variable used for checking whether we are able to load atlease one string
            //during the loop
            BOOL fCouldLoadAtleastOne = FALSE;

            for (int i = 0; i < cDPAItems && nMax > 0; i++)
            {
                PEXTMRU pem = (PEXTMRU)DPA_FastGetPtr(hdpa, i);

                if (pem && GetMRUEntry(pem->mru, j, szFile, SIZECHARS(szFile)))
                {
                    SendMessage(hwndCombo, CBEM_INSERTITEM, (WPARAM)0, (LPARAM)(LPVOID)&cbexItem);
                    nMax--;
                    fCouldLoadAtleastOne = TRUE;
                }
            }

            //Check for possible infinite loop
            if(!fCouldLoadAtleastOne)
            {
                //We couldn't load string from any of the MRU's so there's no point
                //in continuing this loop further. This is the max number of strings 
                // we can load for this user, for this filter type.
                break;
            }
        }

        DPA_DestroyCallback(hdpa, _FreeExtMru, NULL);
    }
    
    return TRUE;
}

//This function adds the selected file into the MRU of the appropriate file MRU's
//This functions also takes care of MultiFile Select case in which the file selected
//will  c:\winnt\file1.c\0file2.c\0file3.c\0. Refer GetOpenFileName documentation for 
// how the multifile is returned.

BOOL AddToMRU(LPOPENFILENAME lpOFN)
{
    TCHAR szDir[MAX_PATH];
    TCHAR szFile[MAX_PATH];
    LPTSTR  lpFile;
    LPTSTR  lpExt;
    BOOL fAddToStar =  TRUE;
    HANDLE hMRUStar;

    //Check if we have valid file name
    if (!lpOFN->lpstrFile)
        return FALSE;

    hMRUStar = CreateMRU(szStar, 10);   //File MRU For *.* file extension

    //Copy the Directory for the selected file
    lstrcpyn(szDir, lpOFN->lpstrFile, lpOFN->nFileOffset);

    //point to the first file
    lpFile = lpOFN->lpstrFile + lpOFN->nFileOffset;

    do
    {
        // BUGBUG: perf, if there are multiple files  of the same extension type,
        // don't keep re-creating the mru.
        lpExt = PathGetExtension(lpFile, NULL,0);
        HANDLE hMRU = CreateMRU(lpExt, 10);
        if (hMRU)
        {
            PathCombine(szFile, szDir, lpFile);
            AddMRUString(hMRU, szFile);
            if((lstrcmpi(lpExt, szStar)) && hMRUStar)
            {
                //Add to the *.* file mru also
                AddMRUString(hMRUStar, szFile);
            }

            FreeMRUList(hMRU);
        }
        lpFile = lpFile + lstrlen(lpFile) + 1;
    } while (((lpOFN->Flags & OFN_ALLOWMULTISELECT)) && (*lpFile != CHAR_NULL));

    //Free the * file mru
    if (hMRUStar)
    {
        FreeMRUList(hMRUStar);
    }
   return TRUE;
}





////////////////////////////////////////////////////////////////////////////
//
//  Last Visited MRU Implementation
//     All Strings stored in the registry are stored in unicode format.
//
////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////
//  CreateLastVisitedItem
////////////////////////////////////////////////////////////////////////////
LPBYTE CreateLastVisitedItem(LPCWSTR wszModule, LPCWSTR wszPath, DWORD *pcbOut)
{
    LPBYTE pitem = NULL;
    DWORD cbLen1, cbLen2;
    cbLen1 = CbFromCchW(lstrlenW(wszModule)+1);
    cbLen2 = CbFromCchW(lstrlenW(wszPath)+1);

    pitem = (LPBYTE) LocalAlloc(LPTR, cbLen1+cbLen2);

    if (pitem)
    {
        memcpy(pitem, wszModule, cbLen1);
        memcpy(pitem+cbLen1, wszPath, cbLen2);
        *pcbOut = cbLen1+cbLen2;
    }

    return pitem;
}

////////////////////////////////////////////////////////////////////////////
//  LastVisitedCompareProc
////////////////////////////////////////////////////////////////////////////
int cdecl LastVisitedCompareProc(const void *p1, const void *p2, size_t cb)
{
    return lstrcmpiW((LPWSTR)p1,(LPWSTR)p2);
}

////////////////////////////////////////////////////////////////////////////
//  AddToLastVisitedMRU
//  Store all the strings in the registry as unicode strings
////////////////////////////////////////////////////////////////////////////
BOOL AddToLastVisitedMRU(LPCTSTR pszFile, int nFileOffset)
{
    BOOL bRet = FALSE;

    MRUDATAINFO mi =
    {
        SIZEOF(MRUDATAINFO),
        MAX_MRU,
        MRU_BINARY | MRU_CACHEWRITE,
        HKEY_CURRENT_USER,
        REGSTR_PATH_LASTVISITED,
        LastVisitedCompareProc
    };

    HANDLE hMRU =  CreateMRUList((MRUINFO *)&mi);

    if (hMRU)
    {

        WCHAR  wszDir[MAX_PATH];
        WCHAR  wszModulePath[MAX_PATH];
        WCHAR* pszModuleName;
        DWORD  cbSize;
    
        //Get the module name
        GetModuleFileNameW(GetModuleHandle(NULL), wszModulePath, ARRAYSIZE(wszModulePath));
        pszModuleName = PathFindFileNameW(wszModulePath);


        int i = FindMRUData(hMRU, (LPVOID)pszModuleName, CbFromCchW(lstrlenW(pszModuleName)+1), NULL);

        if (i >= 0 )
        {
            DelMRUString(hMRU, i);
        }

        //Get the Directoy from file.
        SHTCharToUnicode(pszFile,wszDir, MAX_PATH); 
        wszDir[nFileOffset - 1] = CHAR_NULL;

        LPBYTE pitem = CreateLastVisitedItem(pszModuleName, wszDir,&cbSize);
        
        if (pitem)
        {
            AddMRUData(hMRU, pitem, cbSize);
            bRet = TRUE;
            LocalFree(pitem);
        }

        FreeMRUList(hMRU);
    }
    return bRet;
}

////////////////////////////////////////////////////////////////////////////
//  GetPathFromLastVisitedMRU
////////////////////////////////////////////////////////////////////////////

BOOL GetPathFromLastVisitedMRU(LPTSTR pszDir, DWORD cchDir)
{
    BOOL bRet = FALSE;

    MRUDATAINFO mi =
    {
        SIZEOF(MRUDATAINFO),
        MAX_MRU,
        MRU_BINARY | MRU_CACHEWRITE,
        HKEY_CURRENT_USER,
        REGSTR_PATH_LASTVISITED,
        LastVisitedCompareProc
    };

    pszDir[0] = 0;

    HANDLE hMRU =  CreateMRUList((MRUINFO *)&mi);
    
    if (hMRU)
    {

        WCHAR  wszModulePath[MAX_PATH];
        WCHAR* pszModuleName;
    
        //Get the module name
        GetModuleFileNameW(GetModuleHandle(NULL), wszModulePath, ARRAYSIZE(wszModulePath));
        pszModuleName = PathFindFileNameW(wszModulePath);

        int i = FindMRUData(hMRU, (LPVOID)pszModuleName, CbFromCchW(lstrlenW(pszModuleName)+1), NULL);

        if (i >= 0)
        {
            BYTE buf[CbFromCchW(2*MAX_PATH)];

            if (-1 != EnumMRUList(hMRU, i, buf, SIZEOF(buf)))
            {
                LPWSTR lpDir = (LPWSTR)((LPBYTE)buf + CbFromCchW(lstrlenW((LPWSTR)buf) +1));
                SHUnicodeToTChar(lpDir, pszDir, cchDir);
                bRet = TRUE;
            }
        }
        FreeMRUList(hMRU);
    }
    return bRet;
}