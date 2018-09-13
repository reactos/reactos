//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: info.c
//
//  This files contains dialog code for the Info property sheet
//
// History:
//  08-06-93 ScottH     Transferred from twin code
//
//---------------------------------------------------------------------------


#include "brfprv.h"         // common headers
#include <brfcasep.h>

#include "res.h"
#ifdef WINNT
#include <help.h>
#else
#include "..\..\..\win\core\inc\help.h"   // help IDs
#endif

//---------------------------------------------------------------------------
// INFO dialog struct
//---------------------------------------------------------------------------

// State flags for the INFO dialog
#define IS_ALLTYPES         0x0001
#define IS_INCLUDESUBS      0x0002
#define IS_DENYAPPLY        0x0004
#define IS_CHANGED          0x0008
#define IS_LAST_INCLUDESUBS 0x0010

typedef struct tagINFO
    {
    HWND    hwnd;               // dialog handle
    PPAGEDATA ppagedata;
    PINFODATA pinfodata;
    int     cselPrev;           // previous count of selections
    
    LPTSTR   pszExtListPrev;     // alloc: last saved settings
    UINT    uState;
    BOOL    bInit;
    
    } INFO,  * PINFO;


// Struct for CHANGETWINPROC callback
typedef struct tagCHANGEDATA
    {
    HBRFCASE    hbrf;
    HFOLDERTWIN hft;

    HDPA        hdpaTwins;
    int         idpaTwin;
    HDPA        hdpaFolders;
    int         idpaStart;

    UINT        uState;

    } CHANGEDATA, * PCHANGEDATA;

typedef HRESULT (CALLBACK * CHANGETWINPROC)(PNEWFOLDERTWIN, TWINRESULT, PCHANGEDATA);


// Struct for Info_AddTwins
typedef struct tagADDTWINSDATA
    {
    CHANGETWINPROC pfnCallback;
    HDPA hdpaSortedFolders;
    int idpaStart;
    } ADDTWINSDATA, * PADDTWINSDATA;


#define MAX_EXT_LEN     6       // Length for "*.ext"

#pragma data_seg(DATASEG_READONLY)

static TCHAR const c_szAllFilesExt[] = TEXT(".*");

#pragma data_seg()

// Helper macros

#define Info_StandAlone(this)       ((this)->pinfodata->bStandAlone)

#define Info_GetPtr(hwnd)           (PINFO)GetWindowLongPtr(hwnd, DWLP_USER)
#define Info_SetPtr(hwnd, lp)       (PINFO)SetWindowLongPtr(hwnd, DWLP_USER, (LRESULT)(lp))


#pragma data_seg(DATASEG_READONLY)  
SETbl const c_rgseInfo[4] = {       // change in ibrfstg.c too
        { E_TR_OUT_OF_MEMORY, IDS_OOM_ADDFOLDER, MB_ERROR },
        { E_OUTOFMEMORY, IDS_OOM_ADDFOLDER, MB_ERROR },
        { E_TR_UNAVAILABLE_VOLUME, IDS_ERR_ADDFOLDER_UNAVAIL_VOL, MB_RETRYCANCEL | MB_ICONWARNING },
        { E_TR_SUBTREE_CYCLE_FOUND, IDS_ERR_ADD_SUBTREECYCLE, MB_WARNING },
        };
#pragma data_seg()


//---------------------------------------------------------------------------
// Info dialog functions
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: Searches for an occurrence of the given extension
         in the folder twin list.

Returns: TRUE if the extension was found
Cond:    --
*/
BOOL PRIVATE FindExtension(
    PFOLDERTWINLIST pftl,
    LPCTSTR pszExt)
    {
    PCFOLDERTWIN pcft;
    
    for (pcft = pftl->pcftFirst; pcft; pcft = pcft->pcftNext)
        {
        if (IsSzEqual(pszExt, pcft->pcszName))
            {
            return TRUE;       // Found a match!
            }
        }
    return FALSE;
    }


/*----------------------------------------------------------
Purpose: Disable all the controls.  Remove any selections.
Returns: --
Cond:    --
*/
void PRIVATE Info_DisableAll(
    PINFO this)
    {
    ASSERT(!Info_StandAlone(this));
    
    // Remove selections
    //
    ListBox_ResetContent(GetDlgItem(this->hwnd, IDC_LBINTYPES));
    Button_SetCheck(GetDlgItem(this->hwnd, IDC_RBINALL), 0);
    Button_SetCheck(GetDlgItem(this->hwnd, IDC_RBINSELECTED), 0);
    Button_SetCheck(GetDlgItem(this->hwnd, IDC_CHININCLUDE), 0);
    
    // Disable the controls
    //
    Button_Enable(GetDlgItem(this->hwnd, IDC_RBINALL), FALSE);
    Button_Enable(GetDlgItem(this->hwnd, IDC_RBINSELECTED), FALSE);
    
    ListBox_Enable(GetDlgItem(this->hwnd, IDC_LBINTYPES), FALSE);
    
    Button_Enable(GetDlgItem(this->hwnd, IDC_CHININCLUDE), FALSE);
    }


/*----------------------------------------------------------
Purpose: Initialize the labels for our formatted radio buttons
Returns: --
Cond:    --
*/
void PRIVATE Info_InitLabels(
    PINFO this)
    {
    HWND hwnd = this->hwnd;
    HWND hwndST = GetDlgItem(hwnd, IDC_CHININCLUDE);
    TCHAR sz[MAXMSGLEN];
    TCHAR szFmt[MAXBUFLEN];
    LPCTSTR pszPath = Atom_GetName(this->ppagedata->atomPath);
    LPTSTR pszFile;

    pszFile = PathFindFileName(pszPath);

    // Set static label
    //
    GetWindowText(hwndST, szFmt, ARRAYSIZE(szFmt));
    wsprintf(sz, szFmt, pszFile);
    SetWindowText(hwndST, sz);

    if (Info_StandAlone(this))
        {
        // Set title ("Create Twin of %s")
        //
        GetWindowText(hwnd, szFmt, ARRAYSIZE(szFmt));
        wsprintf(sz, szFmt, pszFile);
        SetWindowText(hwnd, sz);
        }
    }


/*----------------------------------------------------------
Purpose: Queries the registry for all the legal extensions that
         are registered.  These extensions are returned as a 
         space-separated list in buffer.

Returns: --
Cond:    Caller must GFree *ppszBuffer
*/
void PRIVATE GetExtensionList(
    LPTSTR * ppszBuffer)
    {
    HKEY hkRoot;

    *ppszBuffer = NULL;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hkRoot))
        {
        DWORD dwIndex;
        TCHAR szExt[MAX_PATH];
        
        // Enumerate this key
        for (dwIndex = 0;
                ERROR_SUCCESS == RegEnumKey(hkRoot, dwIndex, szExt, ARRAYSIZE(szExt));
                dwIndex++)
            {
            // Did we get a node that is an extension AND
            // is it a legal MS-DOS extension?
            if (TEXT('.') == *szExt &&
                4 >= lstrlen(szExt))
                {
                // Yes; add this extension to our list
                lstrcat(szExt, TEXT(" "));
                if (FALSE == GCatString(ppszBuffer, szExt))
                    {
                    // Uh oh, something bad happened
                    break;
                    }
                }
            }
        RegCloseKey(hkRoot);
        }
    }


/*----------------------------------------------------------
Purpose: Fill the file types listbox
Returns: --
Cond:    --
*/
void PRIVATE Info_FillTypesList(
    PINFO this)
    {
    HWND hwndCtl = GetDlgItem(this->hwnd, IDC_LBINTYPES);
    LPTSTR pszExtList;

    GetExtensionList(&pszExtList);
    if (pszExtList)
        {
        int nTabWidth;
        TCHAR szExt[MAXBUFLEN];
        LPTSTR psz;
        LPTSTR pszT;
        UINT uLen;
        SHFILEINFO sfi;

        nTabWidth = 30;
        ListBox_SetTabStops(hwndCtl, 1, &nTabWidth);

        for (psz = pszExtList; *psz; psz = CharNext(psz))
            {
            // Skip any leading white-space 
            for (; TEXT(' ') == *psz; psz = CharNext(psz))
                ;

            if (0 == *psz)
                {
                break;  // End of string
                }
            
            // Skip to next white-space (or null)
            for (pszT = psz; TEXT(' ') < *pszT; pszT = CharNext(pszT))
                {
                // (This will also stop at null)
                }

            // (GetExtensionList should only get max 3 char extensions)
            uLen = (UINT)(pszT - psz);
            ASSERT(ARRAYSIZE(szExt) > uLen);

            lstrcpyn(szExt, psz, uLen+1);
            CharUpper(szExt);
            SHGetFileInfo(szExt, 0, &sfi, sizeof(sfi), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);

            // Although this forces the format for international versions,
            // it makes extraction much much easier.
            lstrcat(szExt, TEXT("\t("));
            lstrcat(szExt, sfi.szTypeName);
            lstrcat(szExt, TEXT(")"));
            ListBox_AddString(hwndCtl, szExt);

            psz = pszT;     // To next extension
            }
        
        GFree(pszExtList);
        }
    }

/*----------------------------------------------------------
Purpose: Set the selection of the dialog controls
Returns: --
Cond:    --
*/
void PRIVATE Info_SetSelections(
    PINFO this)
    {
    HWND hwndLB = GetDlgItem(this->hwnd, IDC_LBINTYPES);
    int idBtn;
    int cItems = ListBox_GetCount(hwndLB);

    ListBox_SetSel(hwndLB, FALSE, -1);  // deselect everything

    // Is this the 'Add Folder' dialog?
    if (Info_StandAlone(this))
        {
        // Yes; default to *.* settings
        SetFlag(this->uState, IS_ALLTYPES);
        SetFlag(this->uState, IS_INCLUDESUBS);
        }
    else
        {
        // No; query what the selections are
        TCHAR szExt[MAXBUFLEN];
        PFOLDERTWINLIST pftl;
        PCFOLDERTWIN pcft;
        int cItems;
        int i;
        BOOL bStarDotStar;
        LPTSTR psz;

        if (S_OK == PageData_Query(this->ppagedata, this->hwnd, NULL, &pftl))
            {
            // Determine the selections in the listbox
            szExt[0] = TEXT('*');
            
            cItems = ListBox_GetCount(hwndLB);
            for (i = 0; i < cItems; i++)
                {
                // Extract the extension (it will be the first part of the
                // string)
                ListBox_GetText(hwndLB, i, &szExt[1]);
                for (psz = szExt; *psz && TEXT('\t') != *psz; psz = CharNext(psz))
                    ;
                ASSERT(TEXT('\t') == *psz);
                *psz = 0;           // null terminate after the extension

                // Is this extension in the folder twin list?
                if (FindExtension(pftl, szExt))
                    {
                    // Yes; select the entry
                    ListBox_SetSel(hwndLB, TRUE, i);
                    }
                }

            ListBox_SetTopIndex(hwndLB, 0);
            this->cselPrev = ListBox_GetSelCount(hwndLB);
            
            // Determine the Include Subdirectories checkbox setting
            //
            bStarDotStar = FALSE;
            ClearFlag(this->uState, IS_INCLUDESUBS);
            for (pcft = pftl->pcftFirst; pcft; pcft = pcft->pcftNext)
                {
                if (IsFlagSet(pcft->dwFlags, FT_FL_SUBTREE))
                    SetFlag(this->uState, IS_INCLUDESUBS);
                
                if (IsSzEqual(pcft->pcszName, c_szAllFiles))
                    bStarDotStar = TRUE;
                }
            
            // Set the default radio button choice, and disable listbox 
            // if necessary.  The default radio choice will be IDC_RBINALL, 
            // unless there are selections in the listbox AND there is no 
            // *.* occurrence in the folder twin list.
            //
            if (0 == this->cselPrev || bStarDotStar)
                SetFlag(this->uState, IS_ALLTYPES);
            else
                ClearFlag(this->uState, IS_ALLTYPES);
            }
        else
            {
            // An error occurred or this is an orphan.  Bail early.
            return;
            }
        }

    if (IsFlagSet(this->uState, IS_INCLUDESUBS))
        SetFlag(this->uState, IS_LAST_INCLUDESUBS);
    else
        ClearFlag(this->uState, IS_LAST_INCLUDESUBS);
        
    // Set the control settings 
    Button_SetCheck(GetDlgItem(this->hwnd, IDC_CHININCLUDE), IsFlagSet(this->uState, IS_INCLUDESUBS));

    ListBox_Enable(hwndLB, IsFlagClear(this->uState, IS_ALLTYPES));
    idBtn =  IsFlagSet(this->uState, IS_ALLTYPES) ? IDC_RBINALL : IDC_RBINSELECTED;
    CheckRadioButton(this->hwnd, IDC_RBINALL, IDC_RBINSELECTED, idBtn);
    
    // If listbox is empty, disable Selected Types radio button
    if (0 == cItems)
        {
        Button_Enable(GetDlgItem(this->hwnd, IDC_RBINSELECTED), FALSE);
        }
    }


/*----------------------------------------------------------
Purpose: Get the selected extensions in the listbox
         and place them as a list in *ppszExtList. 

         .* is placed in the buffer if the Select All radio button
         is chosen instead.

Returns: TRUE on success

Cond:    The caller must GFree *ppszExtList
*/
BOOL PRIVATE Info_GetSelections(
    PINFO this,
    LPTSTR * ppszExtList)
    {
    BOOL bRet = FALSE;

    *ppszExtList = NULL;

    // Did user choose the All Types radio button?
    if (IsFlagSet(this->uState, IS_ALLTYPES))
        {
        // Yes; store the .* extension
        bRet = GSetString(ppszExtList, c_szAllFilesExt);
        }
    else
        {
        // No; user selected a bunch of wildcards to filter
        LPINT pisel;
        TCHAR szExt[MAXBUFLEN];
        int csel;
        int isel;
        HWND hwndCtl = GetDlgItem(this->hwnd, IDC_LBINTYPES);
        
        // Allocate memory for the selection buffer
        csel = ListBox_GetSelCount(hwndCtl);
        pisel = GAllocArray(int, csel);
        if (pisel)
            {
            // Get the selected extensions from the listbox
            LPTSTR psz;

            if (0 < csel)
                {
                ListBox_GetSelItems(hwndCtl, csel, pisel);
                for (isel = 0; isel < csel; isel++)
                    {
                    // Extract the extension (it will be the first part of the string)
                    ListBox_GetText(hwndCtl, pisel[isel], szExt);
                    for (psz = szExt; *psz && TEXT('\t') != *psz; psz = CharNext(psz))
                        ;
                    ASSERT(TEXT('\t') == *psz);
                    *psz = 0;

                    if (FALSE == GCatString(ppszExtList, szExt))
                        {
                        break;
                        }
                    }

                if (isel == csel)
                    {
                    bRet = TRUE;    // Success
                    }
                else
                    {
                    GFree(*ppszExtList);
                    }
                }
            GFree(pisel);
            }
        }
        
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Create a sorted DPA version of the folder twin list

Returns: hdpa
         NULL on OOM
Cond:    --
*/
HDPA PRIVATE CreateSortedFolderDPA(
    PFOLDERTWINLIST pftl)
    {
    HDPA hdpa;

    ASSERT(pftl);

    hdpa = DPA_Create(8);
    if (hdpa)
        {
        PCFOLDERTWIN pcft;

        for (pcft = pftl->pcftFirst; pcft; pcft = pcft->pcftNext)
            {
            // Use the dwUser field as a deletion flag
            ((PFOLDERTWIN)pcft)->dwUser = FALSE;
            
            if (DPA_ERR == DPA_InsertPtr(hdpa, DPA_APPEND, (LPVOID)pcft))
                {
                DPA_Destroy(hdpa);
                return NULL;
                }
            }
        DPA_Sort(hdpa, NCompareFolders, CMP_FOLDERTWINS);
        }

    return hdpa;
    }
    

/*----------------------------------------------------------
Purpose: Process callback after adding a folder twin

Returns: standard result
Cond:    --
*/
HRESULT CALLBACK ChangeTwinProc(
    PNEWFOLDERTWIN pnft,
    TWINRESULT tr,
    PCHANGEDATA pcd)
    {
    HRESULT hres = NOERROR;

    // Is this a duplicate twin?
    if (TR_DUPLICATE_TWIN == tr)
        {
        // Yes; there's a wierd case to deal with.  It's possible that the 
        // only thing the user did was check/uncheck the Include Subdirs 
        // checkbox.  If this is true, then we delete the old twin and add 
        // a new twin (with same filespec as before) with the flags set 
        // differently.
        PCFOLDERTWIN pcft;
        HDPA hdpaFolders = pcd->hdpaFolders;
        int cdpa = DPA_GetPtrCount(hdpaFolders);
        int idpa;
        BOOL bOldInclude;

        // Find the correct pcfolder.  We will either tag it or
        // we will delete it right now and re-add the new twin.
        for (idpa = pcd->idpaStart; idpa < cdpa; idpa++)
            {
            pcft = DPA_FastGetPtr(hdpaFolders, idpa);
        
            if (IsSzEqual(pcft->pcszName, pnft->pcszName))
                break;      // found it!
            }
        ASSERT(idpa < cdpa);
        
        // Tag the twin to save from impending doom...
        ((PFOLDERTWIN)(DWORD_PTR)pcft)->dwUser = TRUE;
        
        // Has the Include Subfolders checkbox setting changed?
        bOldInclude = IsFlagSet(pcft->dwFlags, FT_FL_SUBTREE);
        if (bOldInclude ^ IsFlagSet(pcd->uState, IS_INCLUDESUBS))
            {
            // Yes; delete the twin anyway and add the new one.
            HFOLDERTWIN hft;

            DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Deleting old folder twin")); )
            Sync_DeleteTwin(pcft->hftOther);

            // Add the new folder twin to the database
            tr = Sync_AddFolder(pcd->hbrf, pnft, &hft);
            if (TR_SUCCESS != tr)
                {
                // Adding the new twin failed
                DPA_DeletePtr(pcd->hdpaTwins, pcd->idpaTwin);
                hres = HRESULT_FROM_TR(tr);
                }
            else 
                {
                // Set the new twin handle in the pcd->hdpaTwins list
                DPA_SetPtr(pcd->hdpaTwins, pcd->idpaTwin, (LPVOID)hft);

                DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Adding new folder twin")); )
                DEBUG_CODE( Sync_Dump(pnft, NEWFOLDERTWIN); )
                }
            }
        else
            {
            // No; this isn't new, so don't add to list
            DPA_DeletePtr(pcd->hdpaTwins, pcd->idpaTwin);
            }
        }
    else if (tr != TR_SUCCESS)
        {
        // Sync_AddFolder failed
        DPA_DeletePtr(pcd->hdpaTwins, pcd->idpaTwin);
        hres = HRESULT_FROM_TR(tr);
        }
    else
        {
        // Sync_AddFolder succeeded
        DPA_SetPtr(pcd->hdpaTwins, pcd->idpaTwin, (LPVOID)pcd->hft);

        DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Adding new folder twin")); )
        DEBUG_CODE( Sync_Dump(pnft, NEWFOLDERTWIN); )
        }
    return hres;
    }


/*----------------------------------------------------------
Purpose: Add folder twins based on the list of extensions

Returns: standard result
Cond:    --
*/
HRESULT PRIVATE Info_AddTwins(
    PINFO this,
    PNEWFOLDERTWIN pnft,
    PADDTWINSDATA patd,         // May be NULL
    LPTSTR pszExtList)           // This function writes in this buffer
    {
    HRESULT hres = NOERROR;
    CHANGEDATA cd;
    HDPA hdpa;
    int  idpa;
    TCHAR szWildcard[MAX_EXT_LEN];
    LPTSTR psz;
    LPTSTR pszT;
    TCHAR ch;

    hdpa = this->pinfodata->hdpaTwins;

    cd.hbrf = PageData_GetHbrf(this->ppagedata);
    cd.hdpaTwins = hdpa;
    if (patd)
        {
        cd.hdpaFolders = patd->hdpaSortedFolders;
        cd.idpaStart = patd->idpaStart;
        }
    cd.uState = this->uState;

    pnft->pcszName = szWildcard;
    szWildcard[0] = TEXT('*');

    for (psz = pszExtList; *psz; )
        {
        TWINRESULT tr;
        HFOLDERTWIN hft = NULL;

        // Find the beginning of the next extension for the next iteration
        for (pszT = CharNext(psz); *pszT && TEXT('.') != *pszT; pszT = CharNext(pszT))
            ;
        ch = *pszT;
        *pszT = 0;      // Temporary assignment

        // Copy the extension into the name string
        lstrcpy(&szWildcard[1], psz);

        *pszT = ch;
        psz = pszT;
        
        // First make sure we can add another handle to hdpaTwins
        if (DPA_ERR == (idpa = DPA_InsertPtr(hdpa, DPA_APPEND, (LPVOID)hft)))
            {
            hres = ResultFromScode(E_OUTOFMEMORY);
            break;      // Failed
            }
    
        // Add the folder twin to the database
        tr = Sync_AddFolder(cd.hbrf, pnft, &hft);

        if (patd)
            {
            cd.idpaTwin = idpa;
            cd.hft = hft;

            ASSERT(patd->pfnCallback);
            if ( FAILED((hres = patd->pfnCallback(pnft, tr, &cd))) )
                {
                break;
                }
            }
        else if (TR_SUCCESS != tr)
            {
            // Sync_AddFolder failed
            DPA_DeletePtr(hdpa, idpa);
            hres = HRESULT_FROM_TR(tr);
            break;
            }
        else
            {
            // Sync_AddFolder succeeded
            DPA_SetPtr(hdpa, idpa, (LPVOID)hft);

            DEBUG_CODE( Sync_Dump(pnft, NEWFOLDERTWIN); )
            }
        }
    return hres;
    }


/*----------------------------------------------------------
Purpose: Add the folder twin to the database

Returns: standard result
Cond:    --
*/
HRESULT PRIVATE Info_CommitStandAlone(
    PINFO this)
    {
    HRESULT hres;
    NEWFOLDERTWIN nft;
    LPTSTR pszExtList;

    RETRY_BEGIN(FALSE)
        {
        ZeroInit(&nft, NEWFOLDERTWIN);
        nft.ulSize = sizeof(nft);
        nft.pcszFolder1 = Atom_GetName(this->ppagedata->atomPath);
        nft.pcszFolder2 = Atom_GetName(this->pinfodata->atomTo);
        // nft.pcszName is set in Info_AddTwins()
        nft.dwAttributes = OBJECT_TWIN_ATTRIBUTES;
        nft.dwFlags = IsFlagSet(this->uState, IS_INCLUDESUBS) ? NFT_FL_SUBTREE : 0;

        // Create an extension list based on the dialog settings
        if (!Info_GetSelections(this, &pszExtList))
            {
            // Failed
            hres = ResultFromScode(E_OUTOFMEMORY); 
            }
        else
            {
            // Add the twins
            hres = Info_AddTwins(this, &nft, NULL, pszExtList);
            GFree(pszExtList);
            }

        if (SUCCEEDED(hres))
            {
            // Since the engine does not create folders if the folder is empty,
            // we will create the folder now (whether it is empty or not).
            // If the folder already exists, CreateDirectory will fail.
            // Big deal.
            CreateDirectory(nft.pcszFolder2, NULL);
            PathNotifyShell(nft.pcszFolder2, NSE_MKDIR, FALSE);
            }
        else
            {
            DWORD dwError = GetLastError();
            int id;

            // Unavailable disk?
            if (ERROR_INVALID_DATA == dwError || ERROR_ACCESS_DENIED == dwError)
                {
                // Yes
                hres = E_TR_UNAVAILABLE_VOLUME;
                }

            id = SEMsgBox(this->hwnd, IDS_CAP_INFO, hres, c_rgseInfo, ARRAYSIZE(c_rgseInfo));
            if (IDRETRY == id)
                {
                // Try the operation again
                RETRY_SET();
                }
            }
        }
    RETRY_END()

    return hres;
    }


/*----------------------------------------------------------
Purpose: Commit the user changes to the database.  We delete
         all old hFolderTwins, and add new ones.

Returns: standard result
Cond:    --
*/
HRESULT PRIVATE Info_CommitChange(
    PINFO this)
    {
    HRESULT hres;
    PFOLDERTWINLIST pftl;

    hres = PageData_Query(this->ppagedata, this->hwnd, NULL, &pftl);
    if (S_FALSE == hres)
        {
        // The folder has become an orphan right under our nose.
        // Don't do anything.
        Info_DisableAll(this);
        }
    else if (S_OK == hres)
        {
        LPCTSTR pszPath = Atom_GetName(this->ppagedata->atomPath);
        ADDTWINSDATA atd;
        DECLAREHOURGLASS;

        SetHourglass();

        atd.pfnCallback = ChangeTwinProc;

        // Create a sorted DPA based on the folder twin list
        atd.hdpaSortedFolders = CreateSortedFolderDPA(pftl);
        if (atd.hdpaSortedFolders)
            {
            // Create an extension list based on the dialog settings
            LPTSTR pszExtList = NULL;

            if (Info_GetSelections(this, &pszExtList))
                {
                NEWFOLDERTWIN nft;
                PCFOLDERTWIN pcft;
                PCFOLDERTWIN pcftLast;
                int idpa;
                int cdpa;

                // Now add new folder twins.  Iterate thru atd.hdpaSortedFolders.  
                // For each unique folder twin in this list, we add a new twin, 
                // using the old lpcszFolder as the lpcszFolder2 field in our 
                // NEWFOLDERTWIN structure.
                //
                ZeroInit(&nft, NEWFOLDERTWIN);
                nft.ulSize = sizeof(NEWFOLDERTWIN);
                nft.pcszFolder1 = pszPath;
                // nft.pcszFolder2 is set in loop below
                // nft.pcszName is set in Info_AddTwins()
                nft.dwAttributes = OBJECT_TWIN_ATTRIBUTES;
                nft.dwFlags = IsFlagSet(this->uState, IS_INCLUDESUBS) ? NFT_FL_SUBTREE : 0;
                
                // Iterate thru existing folder twins.  Act on each unique one.
                cdpa = DPA_GetPtrCount(atd.hdpaSortedFolders);
                pcftLast = NULL;
                for (idpa = 0; idpa < cdpa; idpa++)
                    {
                    pcft = DPA_FastGetPtr(atd.hdpaSortedFolders, idpa);
                    
                    // Unique?
                    if (pcftLast && pcft->pcszOtherFolder == pcftLast->pcszOtherFolder)
                        {
                        // No; skip to next one
                        continue;
                        }
                    
                    // This is a unique folder.  Add it using the extensions in 
                    // pszExtList.
                    atd.idpaStart = idpa;
                    nft.pcszFolder2 = pcft->pcszOtherFolder;

                    hres = Info_AddTwins(this, &nft, &atd, pszExtList);
                    if (FAILED(hres))
                        {
                        goto Cleanup;
                        }
                    pcftLast = pcft;
                    }
                
                // Delete any old twins
                for (pcft = pftl->pcftFirst; pcft; pcft = pcft->pcftNext)
                    {
                    // Is it okay to delete this twin?
                    if (pcft->hftOther && FALSE == pcft->dwUser)
                        {
                        // Yes
                        TRACE_MSG(TF_GENERAL, TEXT("Deleting folder twin with extension '%s'"), pcft->pcszName);
                        Sync_DeleteTwin(pcft->hftOther);
                        }
                    }

Cleanup:
                GFree(pszExtList);
                }
            DPA_Destroy(atd.hdpaSortedFolders);
            }
        
        ResetHourglass();

        // Notify the shell of the change
        PathNotifyShell(pszPath, NSE_UPDATEITEM, FALSE);

        // Throw out the last saved settings and reset
        GFree(this->pszExtListPrev);
        Info_GetSelections(this, &this->pszExtListPrev);

        this->ppagedata->bRecalc = TRUE;

        if (FAILED(hres))
            {
#pragma data_seg(DATASEG_READONLY)  
            static SETbl const c_rgseInfoChange[] = {
                    { E_TR_OUT_OF_MEMORY, IDS_OOM_CHANGETYPES, MB_ERROR },
                    { E_OUTOFMEMORY, IDS_OOM_CHANGETYPES, MB_ERROR },
                    { E_TR_SUBTREE_CYCLE_FOUND, IDS_ERR_ADD_SUBTREECYCLE, MB_WARNING },
                    };
#pragma data_seg()

            SEMsgBox(this->hwnd, IDS_CAP_INFO, hres, c_rgseInfoChange, ARRAYSIZE(c_rgseInfoChange));
            }
        }
    return hres;
    }


/*----------------------------------------------------------
Purpose: Info WM_INITDIALOG Handler
Returns: 
Cond:    --
*/
BOOL PRIVATE Info_OnInitDialog(
    PINFO this,
    HWND hwndFocus,
    LPARAM lParam)              // LPPROPSHEETINFO
    {
    this->ppagedata = (PPAGEDATA)((LPPROPSHEETPAGE)lParam)->lParam;
    this->pinfodata = (PINFODATA)this->ppagedata->lParam;

    // Set the text of the controls
    Info_InitLabels(this);
    
    // Fill listbox and set the control selections
    Info_FillTypesList(this);
    if (Info_StandAlone(this))
        {
        Info_SetSelections(this);
        Info_GetSelections(this, &this->pszExtListPrev);
        }

    this->bInit = TRUE;

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: PSN_APPLY handler

Returns: FALSE if everything is OK
         TRUE to have the property sheet switch to this page to 
            correct something.
Cond:    --
*/
BOOL PRIVATE Info_OnApply(
    PINFO this)
    {
    BOOL bRet;
    LPTSTR pszExtList;

    ASSERT(!Info_StandAlone(this));

    Info_GetSelections(this, &pszExtList);

    // Deny the apply?
    if (IsFlagSet(this->uState, IS_DENYAPPLY))
        {
        // Yes; don't let the apply go thru
        MsgBox(this->hwnd, MAKEINTRESOURCE(IDS_MSG_SPECIFYTYPE), 
               MAKEINTRESOURCE(IDS_CAP_INFO), NULL, MB_ERROR);
        bRet = PSNRET_INVALID;
        }
    // Have any settings changed?
    else if (pszExtList && this->pszExtListPrev &&
        // (Assume extensions are always listed in same order)
        IsSzEqual(this->pszExtListPrev, pszExtList) &&
        IsFlagSet(this->uState, IS_INCLUDESUBS) == IsFlagSet(this->uState, IS_LAST_INCLUDESUBS))
        {
        // No
        bRet = PSNRET_NOERROR;
        }
    else
        {
        // Yes; commit the changes
        Info_CommitChange(this);

        // Sync up the current/previous state
        if (IsFlagSet(this->uState, IS_INCLUDESUBS))
            SetFlag(this->uState, IS_LAST_INCLUDESUBS);
        else
            ClearFlag(this->uState, IS_LAST_INCLUDESUBS);

        bRet = PSNRET_NOERROR;
        }

    GFree(pszExtList);
    ClearFlag(this->uState, IS_CHANGED);

    return bRet;
    }


/*----------------------------------------------------------
Purpose: PSN_SETACTIVE handler
Returns: --
Cond:    --
*/
void PRIVATE Info_OnSetActive(
    PINFO this)
    {
    HWND hwnd = this->hwnd;

    // Cause the page to be painted right away 
    SetWindowRedraw(hwnd, TRUE);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    if (this->bInit)
        {
        PageData_Init(this->ppagedata, GetParent(hwnd));
        this->bInit = FALSE;

        Info_SetSelections(this);
        Info_GetSelections(this, &this->pszExtListPrev);
        }

    // Is this data still valid?
    else if (S_FALSE == PageData_Query(this->ppagedata, this->hwnd, NULL, NULL))
        {
        // No; the folder has become an orphan
        Info_DisableAll(this);
        }
    }


/*----------------------------------------------------------
Purpose: WM_NOTIFY handler
Returns: varies
Cond:    --
*/
LRESULT PRIVATE Info_OnNotify(
    PINFO this,
    int idFrom,
    NMHDR  * lpnmhdr)
    {
    LRESULT lRet = 0;

    switch (lpnmhdr->code)
        {
    case PSN_SETACTIVE:
        Info_OnSetActive(this);
        break;

    case PSN_APPLY:
        lRet = Info_OnApply(this);
        break;

    default:
        break;
        }

    return lRet;
    }


/*----------------------------------------------------------
Purpose: Determines whether to keep from leaving this sheet.
         For the stand-alone ('Add Folder') dialog, this 
         function enables or disables the OK button.

Returns: --
Cond:    --
*/
void PRIVATE Info_DenyKill(
    PINFO this,
    BOOL bDeny)
    {
    if (Info_StandAlone(this))
        {
        Button_Enable(GetDlgItem(this->hwnd, IDOK), !bDeny);
        }
    else
        {
        if (bDeny)
            SetFlag(this->uState, IS_DENYAPPLY);
        else
            ClearFlag(this->uState, IS_DENYAPPLY);
        }
    }


/*----------------------------------------------------------
Purpose: Enable the Apply button 

Returns: --
Cond:    --
*/
void PRIVATE Info_HandleChange(
    PINFO this)
    {
    if (IsFlagClear(this->uState, IS_CHANGED) && !Info_StandAlone(this))
        {
        SetFlag(this->uState, IS_CHANGED);
        PropSheet_Changed(GetParent(this->hwnd), this->hwnd);
        }
    }


/*----------------------------------------------------------
Purpose: Info WM_COMMAND Handler
Returns: --
Cond:    --
*/
VOID PRIVATE Info_OnCommand(
    PINFO this,
    int id,
    HWND hwndCtl,
    UINT uNotifyCode)
    {
    HWND hwnd = this->hwnd;

    switch (id)
        {
    case IDC_RBINALL:
        Info_DenyKill(this, FALSE);
        
        // fall thru

    case IDC_RBINSELECTED:
        // Disable/enable listbox depending on which radio button
        //  is marked.
        //
        if (IDC_RBINALL == id)
            SetFlag(this->uState, IS_ALLTYPES);
        else
            ClearFlag(this->uState, IS_ALLTYPES);
        ListBox_Enable(GetDlgItem(hwnd, IDC_LBINTYPES), IsFlagClear(this->uState, IS_ALLTYPES));
        
        if (IDC_RBINSELECTED == id &&
            0 == ListBox_GetSelCount(GetDlgItem(hwnd, IDC_LBINTYPES)))
            {
            Info_DenyKill(this, TRUE);
            }

        Info_HandleChange(this);
        break;
    
    case IDC_LBINTYPES:
        if (uNotifyCode == LBN_SELCHANGE)
            {
            // Disable/enable OK button based on number of selections
            //  in listbox.
            //
            int csel = ListBox_GetSelCount(GetDlgItem(hwnd, IDC_LBINTYPES));
            
            if (csel == 0)
                Info_DenyKill(this, TRUE);
            else if (csel != this->cselPrev && this->cselPrev == 0)
                Info_DenyKill(this, FALSE);
            this->cselPrev = csel;

            Info_HandleChange(this);
            }

        break;

    case IDC_CHININCLUDE:
        if (FALSE != Button_GetCheck(GetDlgItem(hwnd, IDC_CHININCLUDE)))
            SetFlag(this->uState, IS_INCLUDESUBS);
        else
            ClearFlag(this->uState, IS_INCLUDESUBS);
        Info_HandleChange(this);
        break;
    
    case IDOK:
        if (FAILED(Info_CommitStandAlone(this)))
            EndDialog(hwnd, -1);
    
        // Fall thru
        //  |    |
        //  v    v
    
    case IDCANCEL:
        if (Info_StandAlone(this))
            EndDialog(hwnd, id);
        break;
        }
    }


/*----------------------------------------------------------
Purpose: Handle WM_DESTROY
Returns: --
Cond:    --
*/
void PRIVATE Info_OnDestroy(
    PINFO this)
    {
    GFree(this->pszExtListPrev);
    }


/////////////////////////////////////////////////////  PRIVATE FUNCTIONS


static BOOL s_bInfoRecurse = FALSE;

LRESULT INLINE Info_DefProc(
    HWND hDlg, 
    UINT msg,
    WPARAM wParam,
    LPARAM lParam) 
    {
    ENTEREXCLUSIVE()
        {
        s_bInfoRecurse = TRUE;
        }
    LEAVEEXCLUSIVE()

    return DefDlgProc(hDlg, msg, wParam, lParam); 
    }


/*----------------------------------------------------------
Purpose: Real Create Folder Twin dialog proc
Returns: varies
Cond:    --
*/
LRESULT Info_DlgProc(
    PINFO this,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
    {
#pragma data_seg(DATASEG_READONLY)
    const static DWORD rgHelpIDs[] = {
        IDC_RBINALL,        IDH_BFC_FILTER_TYPE,
        IDC_RBINSELECTED,   IDH_BFC_FILTER_TYPE,
        IDC_LBINTYPES,      IDH_BFC_FILTER_TYPE,
        IDC_CHININCLUDE,    IDH_BFC_FILTER_INCLUDE,
        IDC_GBIN,           IDH_BFC_FILTER_TYPE,
        0, 0 };
#pragma data_seg()

    switch (message)
        {
        HANDLE_MSG(this, WM_INITDIALOG, Info_OnInitDialog);
        HANDLE_MSG(this, WM_COMMAND, Info_OnCommand);
        HANDLE_MSG(this, WM_NOTIFY, Info_OnNotify);
        HANDLE_MSG(this, WM_DESTROY, Info_OnDestroy);

    case WM_HELP:
        WinHelp(((LPHELPINFO)lParam)->hItemHandle, c_szWinHelpFile, HELP_WM_HELP, (DWORD_PTR)(LPVOID)rgHelpIDs);
        return 0;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, c_szWinHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID)rgHelpIDs);
        return 0;

    default:
        return Info_DefProc(this->hwnd, message, wParam, lParam);
        }
    }


/*----------------------------------------------------------
Purpose: Create Folder Twin Dialog Wrapper
Returns: varies
Cond:    --
*/
INT_PTR _export CALLBACK Info_WrapperProc(
    HWND hDlg,          // std params
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
    {
    PINFO this;
    
    // Cool windowsx.h dialog technique.  For full explanation, see
    //  WINDOWSX.TXT.  This supports multiple-instancing of dialogs.
    //
    ENTEREXCLUSIVE()
        {
        if (s_bInfoRecurse)
            {
            s_bInfoRecurse = FALSE;
            LEAVEEXCLUSIVE()
            return FALSE;
            }
        }
    LEAVEEXCLUSIVE()

    this = Info_GetPtr(hDlg);
    if (this == NULL)
        {
        if (message == WM_INITDIALOG)
            {
            this = GAlloc(sizeof(*this));
            if (!this)
                {
                MsgBox(hDlg, MAKEINTRESOURCE(IDS_OOM_INFO), MAKEINTRESOURCE(IDS_CAP_INFO), 
                       NULL, MB_ERROR);
                EndDialog(hDlg, IDCANCEL);
                return Info_DefProc(hDlg, message, wParam, lParam);
                }
            this->hwnd = hDlg;
            Info_SetPtr(hDlg, this);
            }
        else
            {
            return Info_DefProc(hDlg, message, wParam, lParam);
            }
        }
    
    if (message == WM_DESTROY)
        {
        Info_DlgProc(this, message, wParam, lParam);
        GFree(this);
        Info_SetPtr(hDlg, NULL);
        return 0;
        }
    
    return SetDlgMsgResult(hDlg, message, Info_DlgProc(this, message, wParam, lParam));
    }



/////////////////////////////////////////////////////  PUBLIC FUNCTIONS


/*----------------------------------------------------------
Purpose: Entry point to invoke dialog

Returns: standard hresult
Cond:    --
*/
HRESULT PUBLIC Info_DoModal(
    HWND hwndOwner,
    LPCTSTR pszPathFrom,      // Source path
    LPCTSTR pszPathTo,        // Target path
    HDPA hdpaTwin,
    PCBS pcbs)
    {
    HRESULT hres;
    PROPSHEETPAGE psp;
    PAGEDATA pagedata;
    INFODATA infodata;
    
    // (Use the source path for the atomPath because the target path
    // does not exist yet.)
    pagedata.atomPath = Atom_Add(pszPathFrom);
    if (ATOM_ERR != pagedata.atomPath)
        {
        infodata.atomTo = Atom_Add(pszPathTo);
        if (ATOM_ERR != infodata.atomTo)
            {
            INT_PTR nRet;

            pagedata.pcbs = pcbs;
            pagedata.lParam = (LPARAM)&infodata;

            infodata.hdpaTwins = hdpaTwin;
            infodata.bStandAlone = TRUE;

            // Fake up a propsheetinfo struct for the dialog box
            psp.lParam = (LPARAM)&pagedata;        // this is all we care about
        
            nRet = DoModal(hwndOwner, Info_WrapperProc, IDD_INFOCREATE, (LPARAM)(LPVOID)&psp);
            Atom_Delete(infodata.atomTo);

            switch (nRet)
                {
            case IDOK:      hres = NOERROR;         break;
            case IDCANCEL:  hres = E_ABORT;         break;
            default:        hres = E_OUTOFMEMORY;   break;
                }
            }
        else
            {
            hres = E_OUTOFMEMORY;
            }
        Atom_Delete(pagedata.atomPath);
        }
    else
        {
        hres = E_OUTOFMEMORY;
        }
    return hres;
    }
