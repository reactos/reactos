//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: twin.c
//
//  This file contains special twin handling functions.
//
//   (Even though we've moved to a briefcase metaphor,
//    we still refer to twins internally...)
//
// History:
//  08-06-93 ScottH     Transferred from twin code
//
//---------------------------------------------------------------------------


#include "brfprv.h"         // common headers

#include "res.h"
#include "recact.h"


#pragma data_seg(DATASEG_PERINSTANCE)
// BUGBUG: due to a compiler bug, we need to declare this structure
// as a 1-element array because it has pointers to functions in it
// and it is in another datasegment.
VTBLENGINE g_vtblEngine[1] = { { 0 } };    // per-instance v-table
#pragma data_seg()

#define GetFunction(rgtable, name, type)     \
            ((rgtable).##name = (type)GetProcAddress((rgtable).hinst, #name)); \
            ASSERT((rgtable).##name)

#ifdef DEBUG
#define SzTR(tr)    #tr,
#endif

#define MAX_RANGE       0x7fff

// Recitem dwUser values
#define RIU_CHANGED     1
#define RIU_SKIP        2
#define RIU_SHOWSTATUS  3


/*----------------------------------------------------------
Purpose: Compare two structures by folder name
Returns: -1 if <, 0 if ==, 1 if >
Cond:    --
*/
int CALLBACK _export NCompareFolders(
    LPVOID lpv1,
    LPVOID lpv2,
    LPARAM lParam)      // One of: CMP_RECNODES, CMP_FOLDERTWINS
    {
    switch (lParam)
        {
    case CMP_RECNODES:
        return lstrcmpi(((PRECNODE)lpv1)->pcszFolder, ((PRECNODE)lpv2)->pcszFolder);

    case CMP_FOLDERTWINS:
        return lstrcmpi(((PCFOLDERTWIN)lpv1)->pcszOtherFolder, ((PCFOLDERTWIN)lpv2)->pcszOtherFolder);

    default:
        ASSERT(0);      // should never get here
        }
    return 0;
    }


//---------------------------------------------------------------------------
// Choose side functions
//---------------------------------------------------------------------------


#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Dump a CHOOSESIDE structure
Returns: --
Cond:    --
*/
void PRIVATE ChooseSide_Dump(
    PCHOOSESIDE pchside)
    {
    BOOL bDump;
    TCHAR szBuf[MAXMSGLEN];

    ASSERT(pchside);

    #define szDumpLabel     TEXT("             *** ")
    #define szDumpMargin    TEXT("                 ")

    ENTEREXCLUSIVE()
        {
        bDump = IsFlagSet(g_uDumpFlags, DF_CHOOSESIDE);
        }
    LEAVEEXCLUSIVE()

    if (bDump)
        {
        wsprintf(szBuf, TEXT("%s.pszFolder = {%s}\r\n"), (LPTSTR)szDumpLabel, pchside->pszFolder);
        OutputDebugString(szBuf);

        wsprintf(szBuf, TEXT("%s.dwFlags = 0x%lx\r\n"), (LPTSTR)szDumpMargin, pchside->dwFlags);
        OutputDebugString(szBuf);

        wsprintf(szBuf, TEXT("%s.nRank = %ld\r\n"), (LPTSTR)szDumpMargin, pchside->nRank);
        OutputDebugString(szBuf);
        }

    #undef szDumpLabel
    #undef szDumpMargin
    }


/*----------------------------------------------------------
Purpose: Dump a CHOOSESIDE list
Returns: --
Cond:    --
*/
void PUBLIC ChooseSide_DumpList(
    HDSA hdsa)
    {
    BOOL bDump;
    TCHAR szBuf[MAXMSGLEN];

    ASSERT(hdsa);

    #define szDumpLabel     TEXT("Dump CHOOSESIDE list: ")

    ENTEREXCLUSIVE()
        {
        bDump = IsFlagSet(g_uDumpFlags, DF_CHOOSESIDE);
        }
    LEAVEEXCLUSIVE()

    if (bDump)
        {
        int i;
        int cel = DSA_GetItemCount(hdsa);
        PCHOOSESIDE pchside;

        wsprintf(szBuf, TEXT("%s.count = %lu\r\n"), (LPTSTR)szDumpLabel, cel);
        OutputDebugString(szBuf);

        if (NULL != (pchside = DSA_GetItemPtr(hdsa, 0)))
            {
            if (IsFlagSet(pchside->dwFlags, CSF_INSIDE))
                OutputDebugString(TEXT("Rank for inside\r\n"));
            else
                OutputDebugString(TEXT("Rank for outside\r\n"));
            }
            
        for (i = 0; i < cel; i++)
            {
            pchside = DSA_GetItemPtr(hdsa, i);

            ChooseSide_Dump(pchside);
            }
        }

    #undef szDumpLabel
    }

#endif


/*----------------------------------------------------------
Purpose: Initialize an array of CHOOSESIDE elements from a 
         recitem list.  Array is unsorted.

Returns: --

Cond:    The contents of the array are safe as long as the 
         recitem list lives.

*/
void PUBLIC ChooseSide_InitAsFile(
    HDSA hdsa,
    PRECITEM pri)
    {
    CHOOSESIDE chside;
    PRECNODE prn;

    ASSERT(hdsa);
    ASSERT(pri);

    DSA_DeleteAllItems(hdsa);

    // All entries start with these values
    chside.dwFlags = 0;
    chside.nRank = 0;

    // Add each recnode
    for (prn = pri->prnFirst; prn; prn = prn->prnNext)
        {
        chside.htwin = (HTWIN)prn->hObjectTwin;
        chside.hvid = prn->hvid;
        chside.pszFolder = prn->pcszFolder;
        chside.prn = prn;

        DSA_InsertItem(hdsa, 0x7fff, &chside);
        }
    }


/*----------------------------------------------------------
Purpose: Create an array of CHOOSESIDE elements from a recitem
         list.  Array is unsorted.

Returns: standard result

Cond:    The contents of the array are safe as long as the 
         recitem list lives.

*/
HRESULT PUBLIC ChooseSide_CreateAsFile(
    HDSA * phdsa,
    PRECITEM pri)
    {
    HRESULT hres;
    HDSA hdsa;

    ASSERT(phdsa);
    ASSERT(pri);

    hdsa = DSA_Create(sizeof(CHOOSESIDE), (int)pri->ulcNodes);
    if (hdsa)
        {
        ChooseSide_InitAsFile(hdsa, pri);
        hres = NOERROR;
        }
    else
        hres = E_OUTOFMEMORY;

    *phdsa = hdsa;

    return hres;
    }


/*----------------------------------------------------------
Purpose: Create an empty array of CHOOSESIDE elements.

Returns: standard result
Cond:    --
*/
HRESULT PUBLIC ChooseSide_CreateEmpty(
    HDSA * phdsa)
    {
    HRESULT hres;
    HDSA hdsa;

    ASSERT(phdsa);

    hdsa = DSA_Create(sizeof(CHOOSESIDE), 4);
    if (hdsa)
        {
        hres = NOERROR;
        }
    else
        hres = E_OUTOFMEMORY;

    *phdsa = hdsa;

    return hres;
    }


/*----------------------------------------------------------
Purpose: Create an array of CHOOSESIDE elements from a foldertwin
         list.  Array is unsorted.

Returns: standard result

Cond:    The contents of the array are safe as long as the 
         foldertwin list lives.

*/
HRESULT PUBLIC ChooseSide_CreateAsFolder(
    HDSA * phdsa,
    PFOLDERTWINLIST pftl)
    {
    HRESULT hres;
    HDSA hdsa;
    CHOOSESIDE chside;

    ASSERT(pftl);

    hdsa = DSA_Create(sizeof(chside), (int)pftl->ulcItems);
    if (hdsa)
        {
        PCFOLDERTWIN pft;
        LPCTSTR pszFolderLast = NULL;

        // All entries start with these values
        chside.dwFlags = CSF_FOLDER;
        chside.nRank = 0;
        chside.prn = NULL;

        // Special case the source folder
        chside.htwin = (HTWIN)pftl->pcftFirst->hftSrc;
        chside.hvid = pftl->pcftFirst->hvidSrc;
        chside.pszFolder = pftl->pcftFirst->pcszSrcFolder;

        // (Don't care if this fails)
        DSA_InsertItem(hdsa, 0x7fff, &chside);

        // Add the other folders (duplicates skipped)
        for (pft = pftl->pcftFirst; pft; pft = pft->pcftNext)
            {
            // Duplicate?
            if (pszFolderLast && IsSzEqual(pszFolderLast, pft->pcszOtherFolder))
                continue;   // Yes (hack: the engine gives us a sorted list)

            chside.htwin = (HTWIN)pft->hftOther;
            chside.hvid = pft->hvidOther;
            chside.pszFolder = pft->pcszOtherFolder;

            DSA_InsertItem(hdsa, 0x7fff, &chside);

            pszFolderLast = pft->pcszOtherFolder;
            }
        *phdsa = hdsa;
        hres = NOERROR;
        }
    else
        hres = E_OUTOFMEMORY;

    return hres;
    }


/*----------------------------------------------------------
Purpose: Reset the ranks

Returns: --
Cond:    --
*/
void PRIVATE ChooseSide_ResetRanks(
    HDSA hdsa)
    {
    int i;
    int cel;

    ASSERT(hdsa);

    cel = DSA_GetItemCount(hdsa);
    for (i = 0; i < cel; i++)
        {
        PCHOOSESIDE pchside = DSA_GetItemPtr(hdsa, i);

        pchside->nRank = 0;
        }
    }


/*----------------------------------------------------------
Purpose: Determine ranks based on whether each element in the
         array is inside the briefcase.

Returns: --
Cond:    --
*/
void PRIVATE ChooseSide_RankForInside(
    HDSA hdsa,
    LPCTSTR pszBrfPath,      // Root path of briefcase
    LPCTSTR pszFolder)       // If NULL, choose best outside element
    {
    int i;
    int cel;
    int cchLast = 0;
    PCHOOSESIDE pchsideLast;

    ASSERT(hdsa);
    ASSERT(pszBrfPath);
    ASSERT(pszFolder);
    ASSERT(PathIsPrefix(pszBrfPath, pszFolder));

    cel = DSA_GetItemCount(hdsa);
    for (i = 0; i < cel; i++)
        {
        PCHOOSESIDE pchside = DSA_GetItemPtr(hdsa, i);

        DEBUG_CODE( SetFlag(pchside->dwFlags, CSF_INSIDE); )

        // Is this item inside this briefcase?
        if (PathIsPrefix(pszBrfPath, pchside->pszFolder))
            pchside->nRank++;       // Yes

        // Is this item inside this folder?
        if (PathIsPrefix(pszFolder, pchside->pszFolder))
            {
            int cch = lstrlen(pchside->pszFolder);

            pchside->nRank++;       // Yes; even better

            if (0 == cchLast)
                {
                cchLast = cch;
                pchsideLast = pchside;
                }
            else 
                {
                // Is this path deeper than the last prefix-matching path?
                // (the path closer to the top is better)
                if (cch > cchLast)
                    {
                    // Yes; demote this one 
                    pchside->nRank--;
                    }
                else
                    {
                    // No; demote previous one
                    ASSERT(pchsideLast);
                    pchsideLast->nRank--;

                    cchLast = cch;
                    pchsideLast = pchside;
                    }
                }
            }
        }
    }


/*----------------------------------------------------------
Purpose: Determine ranks based on whether each element in the
         array is outside the briefcase.

Returns: --
Cond:    --
*/
void PRIVATE ChooseSide_RankForOutside(
    HDSA hdsa,
    LPCTSTR pszBrfPath)      // Root path of briefcase
    {
    int i;
    int cel;

    ASSERT(hdsa);
    ASSERT(pszBrfPath);

    cel = DSA_GetItemCount(hdsa);
    for (i = 0; i < cel; i++)
        {
        PCHOOSESIDE pchside = DSA_GetItemPtr(hdsa, i);

        DEBUG_CODE( ClearFlag(pchside->dwFlags, CSF_INSIDE); )

        // Is this item NOT in this briefcase?
        if ( !PathIsPrefix(pszBrfPath, pchside->pszFolder) )
            {
            // Yes
            int ndrive = PathGetDriveNumber(pchside->pszFolder);
            int nDriveType = DriveType(ndrive);

            pchside->nRank += 2;

            if (IsFlagClear(pchside->dwFlags, CSF_FOLDER))
                {
                // Is the file unavailable?
                if (RNS_UNAVAILABLE == pchside->prn->rnstate ||
                    FS_COND_UNAVAILABLE == pchside->prn->fsCurrent.fscond)
                    {
                    // Yes; demote
                    pchside->nRank--;
                    }
                }
            else
                {
                // Is the folder unavailable?
                FOLDERTWINSTATUS uStatus;

                Sync_GetFolderTwinStatus((HFOLDERTWIN)pchside->htwin, NULL, 0, 
                    &uStatus);
                if (FTS_UNAVAILABLE == uStatus)
                    {
                    // Yes; demote
                    pchside->nRank--;
                    }
                }

            // Rank on locality of disk (the closer the better)
            if (DRIVE_REMOVABLE == nDriveType || DRIVE_CDROM == nDriveType)
                ;                       // Floppy/removable (do nothing)
            else if (PathIsUNC(pchside->pszFolder) || IsNetDrive(ndrive))
                pchside->nRank++;       // Net
            else
                pchside->nRank += 2;    // Fixed disk
            }
        }
    }


/*----------------------------------------------------------
Purpose: Choose the element with the highest rank.

Returns: TRUE if any element distinguished itself

Cond:    --
*/
BOOL PRIVATE ChooseSide_GetBestRank(
    HDSA hdsa,
    PCHOOSESIDE * ppchside)
    {
    BOOL bRet;
    int i;
    int cel;
    int nRankCur = 0;       // (start at 0 since 0 is not good enough to pass muster)
    DEBUG_CODE( BOOL bDbgDup = FALSE; )

    ASSERT(hdsa);
    ASSERT(ppchside);

    *ppchside = NULL;

    cel = DSA_GetItemCount(hdsa);
    for (i = 0; i < cel; i++)
        {
        PCHOOSESIDE pchside = DSA_GetItemPtr(hdsa, i);

#ifdef DEBUG
        if (0 < nRankCur && nRankCur == pchside->nRank)
            bDbgDup = TRUE;
#endif

        if (nRankCur < pchside->nRank)
            {
            *ppchside = pchside;
            nRankCur = pchside->nRank;

            DEBUG_CODE( bDbgDup = FALSE; )      // Reset
            }
        }

#ifdef DEBUG
    // We shouldn't get duplicate highest ranks
    if (bDbgDup)
        {
        // Dump the chooseside list if there are duplicate highest ranks
        ChooseSide_DumpList(hdsa);
        }
    ASSERT(FALSE == bDbgDup);
#endif

    bRet = 0 < nRankCur;
    ASSERT(bRet && *ppchside || !bRet && !*ppchside);

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Get the best candidate element (inside or outside).
         If the pszFolder is NULL, this function gets the best
         outside path.  

Returns: TRUE if an element was found

Cond:    --
*/
BOOL PUBLIC ChooseSide_GetBest(
    HDSA hdsa,
    LPCTSTR pszBrfPath,      // Root path of briefcase
    LPCTSTR pszFolder,       // If NULL, choose best outside element
    PCHOOSESIDE * ppchside)
    {
    ASSERT(hdsa);
    ASSERT(0 < DSA_GetItemCount(hdsa));
    ASSERT(pszBrfPath);
    ASSERT(ppchside);

    ChooseSide_ResetRanks(hdsa);

    // Are we ranking for inside paths?
    if (pszFolder)
        {
        // Yes; inside wins
        ChooseSide_RankForInside(hdsa, pszBrfPath, pszFolder);
        }
    else
        {
        // No; outside wins
        ChooseSide_RankForOutside(hdsa, pszBrfPath);
        }

    return ChooseSide_GetBestRank(hdsa, ppchside);
    }


/*----------------------------------------------------------
Purpose: Get the next best candidate element (inside or outside).
         ChooseSide_GetBest must have been previously called.

Returns: TRUE if an element was found

Cond:    --
*/
BOOL PUBLIC ChooseSide_GetNextBest(
    HDSA hdsa,
    PCHOOSESIDE * ppchside)
    {
    PCHOOSESIDE pchside;

    ASSERT(hdsa);
    ASSERT(0 < DSA_GetItemCount(hdsa));
    ASSERT(ppchside);

    // Get the best rank and reset it
    ChooseSide_GetBestRank(hdsa, &pchside);
    pchside->nRank = 0;

    // Now get the next best rank
    return ChooseSide_GetBestRank(hdsa, ppchside);
    }


/*----------------------------------------------------------
Purpose: Frees an array of CHOOSESIDE elements.

Returns: --
Cond:    --
*/
void PUBLIC ChooseSide_Free(
    HDSA hdsa)
    {
    if (hdsa)
        {
        DSA_Destroy(hdsa);
        }
    }


//---------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: Determine which node is inside and outside a briefcase.

         This function takes a list of recnodes and determines
         which node is "inside" a briefcase and which one is
         "outside" a briefcase.  "Inside" means the file exists 
         somewhere underneath the briefcase path indicated by 
         atomBrf.  "Outside" is anywhere else (but may be in 
         a different briefcase as well).

Returns: --
Cond:    --
*/
HRESULT PUBLIC Sync_GetNodePair(
    PRECITEM pri,
    LPCTSTR pszBrfPath,
    LPCTSTR pszInsideDir,            // Which folder inside the briefcase to consider
    PRECNODE  * pprnInside,
    PRECNODE  * pprnOutside)     
    {
    HRESULT hres;
    HDSA hdsa;

    ASSERT(pri);
    ASSERT(pszBrfPath);
    ASSERT(pszInsideDir);
    ASSERT(pprnInside);
    ASSERT(pprnOutside);
    ASSERT(PathIsPrefix(pszBrfPath, pszInsideDir));

    hres = ChooseSide_CreateAsFile(&hdsa, pri);
    if (SUCCEEDED(hres))
        {
        PCHOOSESIDE pchside;

        // Get inside folder
        if (ChooseSide_GetBest(hdsa, pszBrfPath, pszInsideDir, &pchside))
            {
            *pprnInside = pchside->prn;
            }
        else
            ASSERT(0);

        // Get outside folder
        if (ChooseSide_GetBest(hdsa, pszBrfPath, NULL, &pchside))
            {
            *pprnOutside = pchside->prn;
            }
        else
            ASSERT(0);

#ifdef DEBUG

        if (IsFlagSet(g_uDumpFlags, DF_PATHS))
            {
            TRACE_MSG(TF_ALWAYS, TEXT("Choosing pairs: %s and %s"), (*pprnInside)->pcszFolder,
                (*pprnOutside)->pcszFolder);
            }

#endif

        ChooseSide_Free(hdsa);
        }
    else
        {
        *pprnInside = NULL;
        *pprnOutside = NULL;
        }
    return hres;
    }



/*----------------------------------------------------------
Purpose: Checks if we've loaded the sync engine
Returns: TRUE if loaded
Cond:    --
*/
BOOL PUBLIC Sync_IsEngineLoaded()
    {
    BOOL bRet;

    ENTEREXCLUSIVE()
        {
        bRet = g_vtblEngine[0].hinst != NULL;
        }
    LEAVEEXCLUSIVE()

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Load the SYNCENG.DLL and initialize the v-table.
Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC Sync_QueryVTable(void)
    {
    BOOL bRet = TRUE;
    HINSTANCE hinst;

    ENTEREXCLUSIVE()
        {
        hinst = g_vtblEngine[0].hinst;
        }
    LEAVEEXCLUSIVE()

    // We want to assure that the engine is loaded the same
    //  number of times that SYNCUI is (by a process).  This prevents
    //  Kernel from nuking the engine prematurely (if a
    //  process is terminated).
    //
    // We go thru these hoops simply because SYNCUI does not
    //  load SYNCENG immediately upon PROCESS_ATTACH.  We wait
    //  until we *really* need to load it the first time.
    //  Once we finally do load it, we need to keep the load
    //  count current.
    //
    // Kernel frees SYNCUI and SYNCENG for us.
    //
    if (NULL == hinst)
        {
        VTBLENGINE vtbl;

        DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Loading %s (cProcess = %d)"),
            (LPCTSTR)c_szEngineDLL, g_cProcesses); )

        ZeroInit(&vtbl, sizeof(VTBLENGINE));

        // Don't be in the critical section when we load the DLL
        // or call GetProcAddress, since our LibMain can block on
        // this critical section.
        ASSERT_NOT_EXCLUSIVE();

        hinst = LoadLibrary(c_szEngineDLL);

        if ( ISVALIDHINSTANCE(hinst) )
            {
            // We are loading for the first time.  Fill the vtable.
            //
            vtbl.hinst = hinst;

            // Get all the function addresses
            //
            GetFunction(vtbl, OpenBriefcase, OPENBRIEFCASEINDIRECT);
            GetFunction(vtbl, SaveBriefcase, SAVEBRIEFCASEINDIRECT);
            GetFunction(vtbl, CloseBriefcase, CLOSEBRIEFCASEINDIRECT);
            GetFunction(vtbl, ClearBriefcaseCache, CLEARBRIEFCASECACHEINDIRECT);
            GetFunction(vtbl, DeleteBriefcase, DELETEBRIEFCASEINDIRECT);
            GetFunction(vtbl, GetOpenBriefcaseInfo, GETOPENBRIEFCASEINFOINDIRECT);
            GetFunction(vtbl, FindFirstBriefcase, FINDFIRSTBRIEFCASEINDIRECT);
            GetFunction(vtbl, FindNextBriefcase, FINDNEXTBRIEFCASEINDIRECT);
            GetFunction(vtbl, FindBriefcaseClose, FINDBRIEFCASECLOSEINDIRECT);

            GetFunction(vtbl, AddObjectTwin, ADDOBJECTTWININDIRECT);
            GetFunction(vtbl, AddFolderTwin, ADDFOLDERTWININDIRECT);
            GetFunction(vtbl, ReleaseTwinHandle, RELEASETWINHANDLEINDIRECT);
            GetFunction(vtbl, DeleteTwin, DELETETWININDIRECT);
            GetFunction(vtbl, GetObjectTwinHandle, GETOBJECTTWINHANDLEINDIRECT);
            GetFunction(vtbl, IsFolderTwin, ISFOLDERTWININDIRECT);
            GetFunction(vtbl, CreateFolderTwinList, CREATEFOLDERTWINLISTINDIRECT);
            GetFunction(vtbl, DestroyFolderTwinList, DESTROYFOLDERTWINLISTINDIRECT);
            GetFunction(vtbl, GetFolderTwinStatus, GETFOLDERTWINSTATUSINDIRECT);
            GetFunction(vtbl, IsOrphanObjectTwin, ISORPHANOBJECTTWININDIRECT);
            GetFunction(vtbl, CountSourceFolderTwins, COUNTSOURCEFOLDERTWINSINDIRECT);
            GetFunction(vtbl, AnyTwins, ANYTWINSINDIRECT);

            GetFunction(vtbl, CreateTwinList, CREATETWINLISTINDIRECT);
            GetFunction(vtbl, DestroyTwinList, DESTROYTWINLISTINDIRECT);
            GetFunction(vtbl, AddTwinToTwinList, ADDTWINTOTWINLISTINDIRECT);
            GetFunction(vtbl, AddAllTwinsToTwinList, ADDALLTWINSTOTWINLISTINDIRECT);
            GetFunction(vtbl, RemoveTwinFromTwinList, REMOVETWINFROMTWINLISTINDIRECT);
            GetFunction(vtbl, RemoveAllTwinsFromTwinList, REMOVEALLTWINSFROMTWINLISTINDIRECT);

            GetFunction(vtbl, CreateRecList, CREATERECLISTINDIRECT);
            GetFunction(vtbl, DestroyRecList, DESTROYRECLISTINDIRECT);
            GetFunction(vtbl, ReconcileItem, RECONCILEITEMINDIRECT);
            GetFunction(vtbl, BeginReconciliation, BEGINRECONCILIATIONINDIRECT);
            GetFunction(vtbl, EndReconciliation, ENDRECONCILIATIONINDIRECT);

            GetFunction(vtbl, IsPathOnVolume, ISPATHONVOLUMEINDIRECT);
            GetFunction(vtbl, GetVolumeDescription, GETVOLUMEDESCRIPTIONINDIRECT);
            }
        else
            {
            bRet = FALSE;
            }

        ENTEREXCLUSIVE()
            {
            g_vtblEngine[0] = vtbl;
            }
        LEAVEEXCLUSIVE()
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Free the engine DLL if it is loaded
Returns: --
Cond:    --
*/
void PUBLIC Sync_ReleaseVTable()
    {
    HINSTANCE hinst;

    ENTEREXCLUSIVE()
        {
        hinst = g_vtblEngine[0].hinst;
        }
    LEAVEEXCLUSIVE()

    if (NULL != hinst)
        {
        DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Freeing %s (cProcess = %d)"),
            (LPCTSTR)c_szEngineDLL, g_cProcesses); )

        // We must call FreeLibrary() on the sync engine even during a
        //  PROCESS_DETACH.  We may be getting detached from a process even
        //  though the process isn't being terminated.  If we don't unload
        //  the sync engine now, it won't be unloaded until the process is
        //  terminated.
        //
        FreeLibrary(hinst);

        ENTEREXCLUSIVE()
            {
            ZeroInit(&g_vtblEngine[0], sizeof(VTBLENGINE));
            }
        LEAVEEXCLUSIVE()
        }

#ifdef DEBUG

    ENTEREXCLUSIVE()
        {
        ASSERT(g_vtblEngine[0].hinst == NULL);
        }
    LEAVEEXCLUSIVE()

#endif
    }


/*----------------------------------------------------------
Purpose: Set the last sync error.
Returns: same twinresult
Cond:    --
*/
TWINRESULT PUBLIC Sync_SetLastError(
    TWINRESULT tr)
    {
    ENTEREXCLUSIVE()
        {
        ASSERTEXCLUSIVE();

        MySetTwinResult(tr);
        }
    LEAVEEXCLUSIVE()

    return tr;
    }


/*----------------------------------------------------------
Purpose: Get the last sync error.
Returns: twinresult
Cond:    --
*/
TWINRESULT PUBLIC Sync_GetLastError()
    {
    TWINRESULT tr;

    ENTEREXCLUSIVE()
        {
        ASSERTEXCLUSIVE();

        tr = MyGetTwinResult();
        }
    LEAVEEXCLUSIVE()

    return tr;
    }


/*----------------------------------------------------------
Purpose: Returns the number of recitems that would require 
         some reconciliation.

Returns: see above
Cond:    --
*/
ULONG PUBLIC CountActionItems(
    PRECLIST prl)
    {
    PRECITEM pri;
    ULONG ulc;

    for (pri = prl->priFirst, ulc = 0; pri; pri = pri->priNext)
        {
        if (IsFileRecItem(pri) &&
            RIU_SKIP != pri->dwUser &&
            RIA_NOTHING != pri->riaction &&
            RIA_BROKEN_MERGE != pri->riaction)
            {
            ulc++;
            pri->dwUser = RIU_SHOWSTATUS;
            }
        }


    return ulc;
    }


/*----------------------------------------------------------
Purpose: Displays appropriate error message on update errors
Returns: --
Cond:    --
*/
void PRIVATE HandleUpdateErrors(
    HWND hwndOwner,
    HRESULT hres,
    UINT uFlags)        // RF_*
    {
    // Is this an update while adding files?
    if (IsFlagSet(uFlags, RF_ONADD))
        {
        // Yes
#pragma data_seg(DATASEG_READONLY)  
        static SETbl const c_rgseUpdateOnAdd[] = {
                // The out of memory message should be handled by caller
                { E_TR_DEST_OPEN_FAILED,    IDS_ERR_ADD_READONLY,    MB_WARNING },
                { E_TR_DEST_WRITE_FAILED,   IDS_ERR_ADD_FULLDISK,    MB_WARNING },
                { E_TR_UNAVAILABLE_VOLUME,  IDS_ERR_ADD_UNAVAIL_VOL, MB_WARNING },
                { E_TR_SRC_OPEN_FAILED,     IDS_ERR_ADD_SOURCE_FILE, MB_WARNING },
                };
#pragma data_seg()

        SEMsgBox(hwndOwner, IDS_CAP_ADD, hres, c_rgseUpdateOnAdd, ARRAYSIZE(c_rgseUpdateOnAdd));
        }
    else
        {
        // No
#pragma data_seg(DATASEG_READONLY)  
        static SETbl const c_rgseUpdate[] = {
                { E_TR_OUT_OF_MEMORY,       IDS_OOM_UPDATE,         MB_ERROR },
                { E_TR_DEST_OPEN_FAILED,    IDS_ERR_READONLY,       MB_INFO },
                { E_TR_DEST_WRITE_FAILED,   IDS_ERR_FULLDISK,       MB_WARNING },
                { E_TR_UNAVAILABLE_VOLUME,  IDS_ERR_UPD_UNAVAIL_VOL,MB_WARNING },
                { E_TR_FILE_CHANGED,        IDS_ERR_FILE_CHANGED,   MB_INFO },
                { E_TR_SRC_OPEN_FAILED,     IDS_ERR_SOURCE_FILE,    MB_WARNING },
                };
#pragma data_seg()

        SEMsgBox(hwndOwner, IDS_CAP_UPDATE, hres, c_rgseUpdate, ARRAYSIZE(c_rgseUpdate));
        }
    }


typedef struct tagPROGPARAM
    {
    HWND hwndProgress;
    WORD wPosMax;
    WORD wPosBase;
    WORD wPosPrev;
    BOOL bSkip;
    } PROGPARAM, * PPROGPARAM;

/*----------------------------------------------------------
Purpose: Status procedure that is called during a single 
         ReconcileItem call.

Returns: varies
Cond:    --
*/
BOOL CALLBACK RecStatusProc(
    RECSTATUSPROCMSG msg,
    LPARAM lParam,
    LPARAM lParamUser)
    {
    BOOL bRet;
    PRECSTATUSUPDATE prsu = (PRECSTATUSUPDATE)lParam;
    PPROGPARAM pprogparam = (PPROGPARAM)lParamUser;
    HWND hwndProgress = pprogparam->hwndProgress;
    WORD wPos;

    bRet = !UpdBar_QueryAbort(hwndProgress);

    switch (msg)
        {
    case RS_BEGIN_COPY:
    case RS_DELTA_COPY:
    case RS_END_COPY:
    case RS_BEGIN_MERGE:
    case RS_DELTA_MERGE:
    case RS_END_MERGE:
#ifdef NEW_REC
    case RS_BEGIN_DELETE:
    case RS_DELTA_DELETE:
    case RS_END_DELETE:
#endif
        TRACE_MSG(TF_PROGRESS, TEXT("Reconcile progress = %lu of %lu"), prsu->ulProgress, prsu->ulScale);
        ASSERT(prsu->ulProgress <= prsu->ulScale);

        if (0 < prsu->ulScale && !pprogparam->bSkip)
            {
            wPos = LOWORD(LODWORD( (((__int64)pprogparam->wPosMax * prsu->ulProgress) / prsu->ulScale) ));

            TRACE_MSG(TF_PROGRESS, TEXT("Max wPos = %u,  new wPos = %u,  old wPos = %u"), 
                pprogparam->wPosMax, wPos, pprogparam->wPosPrev);

            if (wPos > pprogparam->wPosPrev && wPos < pprogparam->wPosMax)
                {
                WORD wPosReal = pprogparam->wPosBase + wPos;

                TRACE_MSG(TF_PROGRESS, TEXT("Setting real position = %u"), wPosReal);

                UpdBar_SetPos(hwndProgress, wPosReal);
                pprogparam->wPosPrev = wPos;
                }
            }
        break;

    default:
        ASSERT(0);
        break;
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Decides the description string while updating.  The
         string is something like "Copying from 'Foo' to 'Bar'"
         or "Merging files in 'Foo' and 'Bar'"

Returns: string in pszBuf

Cond:    --
*/
void PRIVATE DecideDescString(
    LPCTSTR pszBrfPath,
    PRECITEM pri,
    LPTSTR pszBuf,
    int cbBuf,
    LPTSTR pszPathBuf)
    {
    HRESULT hres;
    RA_ITEM * pitem;

    ASSERT(pszBrfPath);
    ASSERT(pri);
    ASSERT(pszBuf);

    hres = RAI_CreateFromRecItem(&pitem, pszBrfPath, pri);
    if (SUCCEEDED(hres))
        {
        UINT ids;
        LPTSTR pszMsg;
        LPCTSTR pszFrom;
        LPCTSTR pszTo;

        lstrcpy(pszPathBuf, pitem->siInside.pszDir);
        PathAppend(pszPathBuf, pitem->pszName);

        switch (pitem->uAction)
            {
        case RAIA_TOOUT:
            ids = IDS_UPDATE_Copy;
            pszFrom = PathFindFileName(pitem->siInside.pszDir);
            pszTo = PathFindFileName(pitem->siOutside.pszDir);
            break;

        case RAIA_TOIN:
            ids = IDS_UPDATE_Copy;
            pszFrom = PathFindFileName(pitem->siOutside.pszDir);
            pszTo = PathFindFileName(pitem->siInside.pszDir);
            break;

        case RAIA_MERGE:
            ids = IDS_UPDATE_Merge;
            // (Arbitrary)
            pszFrom = PathFindFileName(pitem->siInside.pszDir);
            pszTo = PathFindFileName(pitem->siOutside.pszDir);
            break;

        case RAIA_DELETEOUT:
            ids = IDS_UPDATE_Delete;
            pszFrom = PathFindFileName(pitem->siOutside.pszDir);
            pszTo = NULL;
            break;

        case RAIA_DELETEIN:
            ids = IDS_UPDATE_Delete;
            pszFrom = PathFindFileName(pitem->siInside.pszDir);
            pszTo = NULL;
            break;

        default:
            ASSERT(0);
            ids = 0;
            break;
            }

        if (ConstructMessage(&pszMsg, g_hinst, MAKEINTRESOURCE(ids), 
            pszFrom, pszTo))
            {
            lstrcpyn(pszBuf, pszMsg, cbBuf);
            GFree(pszMsg);
            }
        else
            *pszBuf = 0;
        }
    else
        *pszBuf = 0;
    }


/*----------------------------------------------------------
Purpose: Reconcile a given reclist

Returns: standard result
Cond:    --
*/
HRESULT PUBLIC Sync_ReconcileRecList(
    PRECLIST prl,       // ptr to reclist
    LPCTSTR pszBrfPath,
    HWND hwndProgress,
    UINT uFlags)        // RF_*
    {
    HRESULT hres;

    if (prl)
        {
        HWND hwndOwner = GetParent(hwndProgress);
        HWND hwndStatusText = UpdBar_GetStatusWindow(hwndProgress);
        TCHAR szPath[MAX_PATH];
        TCHAR sz[MAXBUFLEN];
        TWINRESULT tr;
        PRECITEM pri;
        PROGPARAM progparam;
        ULONG ulcItems;
        WORD wDelta;

        DEBUG_CODE( Sync_DumpRecList(TR_SUCCESS, prl, TEXT("Updating")); )

        hres = NOERROR;     // assume success

        // Grab the mutex to delay any further calculation in any
        // Briefcase views' secondary threads until we're done 
        // processing here.
        Delay_Own();

        // Determine the range of the progress bar
        UpdBar_SetRange(hwndProgress, MAX_RANGE);

        ulcItems = CountActionItems(prl);
        if (0 < ulcItems)
            wDelta = (WORD)(MAX_RANGE / ulcItems);
        else
            wDelta = 0;

        progparam.hwndProgress = hwndProgress;

        // Start updating
        Sync_BeginRec(prl->hbr);

        ulcItems = 0;           
        for (pri = prl->priFirst; pri; pri = pri->priNext)
            {
            // Did the user explicitly skip this recitem or
            // is this a broken merge?
            if (RIU_SKIP == pri->dwUser ||
                RIA_BROKEN_MERGE == pri->riaction)
                {
                // Yes; don't call ReconcileItem
                continue;
                }

            // Is something going to be done to this recitem?
            if (RIU_SHOWSTATUS == pri->dwUser)
                {
                // Yes; update the name of the file we're updating
                UpdBar_SetName(hwndProgress, pri->pcszName);
                DecideDescString(pszBrfPath, pri, sz, ARRAYSIZE(sz), szPath);
                UpdBar_SetDescription(hwndProgress, sz);

                ASSERT(0 < wDelta);
                progparam.wPosBase = (WORD)(wDelta * ulcItems);
                progparam.wPosMax = wDelta;
                progparam.wPosPrev = 0;
                progparam.bSkip = FALSE;
                }
            else
                {
                progparam.bSkip = TRUE;
                }

            // Call ReconcileItem even for nops, so the recnode states
            // will be updated by the engine.
            tr = Sync_ReconcileItem(pri, RecStatusProc, (LPARAM)&progparam, 
                RI_FL_FEEDBACK_WINDOW_VALID, hwndProgress, hwndStatusText);
            if (TR_SUCCESS != tr &&
                IsFileRecItem(pri))     // ignore folder recitem errors
                {
                // On some conditions, stop updating completely
                hres = HRESULT_FROM_TR(tr);

                switch (hres)
                    {
                case E_TR_OUT_OF_MEMORY:
                case E_TR_RH_LOAD_FAILED: {
                    // Couldn't load the merge handler.  Tell the user but
                    // continue on...
                    int id = MsgBox(hwndOwner, 
                                MAKEINTRESOURCE(IDS_ERR_NO_MERGE_HANDLER), 
                                MAKEINTRESOURCE(IDS_CAP_UPDATE),
                                NULL,
                                MB_WARNING | MB_OKCANCEL,
                                PathGetDisplayName(szPath, sz));

                    if (IDOK == id)
                        break;      // continue updating other files
                    }

                    goto StopUpdating;

                case E_TR_DELETED_TWIN:
                    // Allow the updating to continue.
                    break;

                case E_TR_DEST_OPEN_FAILED:
                case E_TR_FILE_CHANGED:
                    if (IsFlagClear(uFlags, RF_ONADD))
                        {
                        // Allow the updating to continue.  Remember the
                        // latest error for the end.
                        break;
                        }
                    // Fall thru
                    //   |   |
                    //   v   v

                default:
                    goto StopUpdating;
                    }
                }

            // Was something done to this recitem?
            if (RIU_SHOWSTATUS == pri->dwUser)
                {
                // Yes; update the progress bar
                UpdBar_SetPos(hwndProgress, (WORD)(wDelta * ++ulcItems));
                }

            // Check if the Cancel button was pressed
            if (UpdBar_QueryAbort(hwndProgress))
                {
                hres = E_ABORT;
                break;
                }
            }

StopUpdating:
        if (FAILED(hres))
            {
            Sync_DumpRecItem(tr, pri, NULL);
            HandleUpdateErrors(hwndOwner, hres, uFlags);

            if (IsFlagSet(uFlags, RF_ONADD))
                {
                // Hack: since the caller also handles some error messages,
                // return a generic failure code to prevent repeated
                // error messages.
                hres = E_FAIL;
                }
            }
        // Were there any items at all?
        else if (0 == prl->ulcItems)
            {
            // No
            MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_MSG_NoMatchingFiles), 
                MAKEINTRESOURCE(IDS_CAP_UPDATE), NULL, MB_INFO);
            }

        Sync_EndRec(prl->hbr);

        Delay_Release();
        }
    else
        hres = E_INVALIDARG;

    return hres;
    }


/*----------------------------------------------------------
Purpose: Status procedure that is called during a single 
         ReconcileItem call.

Returns: varies
Cond:    --
*/
BOOL CALLBACK CreateRecListProc(
    CREATERECLISTPROCMSG msg,
    LPARAM lParam,
    LPARAM lParamUser)
    {
    return !AbortEvt_Query((PABORTEVT)lParamUser);
    }


/*----------------------------------------------------------
Purpose: Creates a reclist and optionally shows a progress
         bar during the creation.

Returns: standard result
Cond:    --
*/
HRESULT PUBLIC Sync_CreateRecListEx(
    HTWINLIST htl,
    PABORTEVT pabortevt,
    PRECLIST * pprl)
    {
    TWINRESULT tr;

    ASSERT(pprl);

    tr = Sync_CreateRecList(htl, CreateRecListProc, (LPARAM)pabortevt, pprl);
    return HRESULT_FROM_TR(tr);
    }


/*----------------------------------------------------------
Purpose: Return true if the file or folder is a twin.

         There are some cases when this function cannot successfully
         determine this unless the caller first tells it explicitly 
         whether the object is a file or folder.  Otherwise this
         function will attempt to determine this on its own.

Returns: S_OK if it is a twin
         S_FALSE if it is not
         any other is an error

Cond:    --
*/
HRESULT PUBLIC Sync_IsTwin(
    HBRFCASE hbrfcase,
    LPCTSTR pszPath,
    UINT uFlags)        // SF_* flags
    {
    HRESULT hres;
    TWINRESULT tr;

    ASSERT(pszPath);

    // The caller may already know whether this is a twin or not.
    // Remind him.
    if (IsFlagSet(uFlags, SF_ISTWIN))
        return S_OK;
    else if (IsFlagSet(uFlags, SF_ISNOTTWIN))
        return S_FALSE;

    // Is this a folder?
    if (IsFlagSet(uFlags, SF_ISFOLDER) ||
        PathIsDirectory(pszPath))
        {
        // Yes; is it a twin?
        BOOL bRet;
    
        tr = Sync_IsFolder(hbrfcase, pszPath, &bRet);
        if (TR_SUCCESS == tr)
            {
            // Yes/no
            hres = bRet ? S_OK : S_FALSE;
            }
        else
            {
            // Error
            hres = HRESULT_FROM_TR(tr);
            }
        }
    else
        {
        // No
        HOBJECTTWIN hot;
        TCHAR szDir[MAX_PATH];

        lstrcpy(szDir, pszPath);
        PathRemoveFileSpec(szDir);
        tr = Sync_GetObject(hbrfcase, szDir, PathFindFileName(pszPath), &hot);
        if (TR_SUCCESS == tr)
            {
            // Is it a twin?
            if (NULL != hot)
                {
                // Yes
                Sync_ReleaseTwin(hot);
                hres = S_OK;
                }
            else
                {
                // No; (no need to release a null handle)
                hres = S_FALSE;
                }
            }
        else
            {
            // Error
            hres = HRESULT_FROM_TR(tr);
            }
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Create a reclist with everything in it.

Returns: standard result

Cond:    Caller must destroy the reclist
*/
HRESULT PUBLIC Sync_CreateCompleteRecList(
    HBRFCASE hbrf,
    PABORTEVT pabortevt,
    PRECLIST * pprl)
    {
    HRESULT hres = E_OUTOFMEMORY;
    HTWINLIST htl;

    ASSERT(pprl);

    *pprl = NULL;

    if (TR_SUCCESS == Sync_CreateTwinList(hbrf, &htl))
        {
        Sync_AddAllToTwinList(htl);

        hres = Sync_CreateRecListEx(htl, pabortevt, pprl);
        Sync_DestroyTwinList(htl);
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Add a twin to the twinlist given the pathname.  If the
         pathname is not a twin, we don't add it.

Returns: TRUE on success, even when this isn't a twin

Cond:    Caller must destroy the folder list if lplpftl is not NULL
*/
BOOL PUBLIC Sync_AddPathToTwinList(
    HBRFCASE hbrf,
    HTWINLIST htl,
    LPCTSTR lpcszPath,               // Path
    PFOLDERTWINLIST  * lplpftl)  // May be NULL
    {
    BOOL bRet = FALSE;

    ASSERT(lpcszPath);
    ASSERT(htl);

    if (lplpftl)
        *lplpftl = NULL;

    if (lpcszPath)
        {
        if (PathIsDirectory(lpcszPath))
            {
            BOOL fIsTwin = FALSE;
            PFOLDERTWINLIST lpftl;

            // We only want to return false if we couldn't mark something
            //  that should have been marked.  If this isn't a twin,
            //  we still succeed.

            bRet = TRUE;

            Sync_IsFolder(hbrf, lpcszPath, &fIsTwin);
            if (fIsTwin)        // Is this actually twinned?
                {
                // This is a folder twin.  Add to reclist "the folder way".
                //
                if (Sync_CreateFolderList(hbrf, lpcszPath, &lpftl) != TR_SUCCESS)
                    bRet = FALSE;
                else
                    {
                    PCFOLDERTWIN lpcfolder;

                    ASSERT(lpftl->pcftFirst);

                    // BUGBUG: only mark the ones that aren't in other briefcases
                    //
                    lpcfolder = lpftl->pcftFirst;
                    while (lpcfolder)
                        {
                        Sync_AddToTwinList(htl, lpcfolder->hftOther);

                        lpcfolder = lpcfolder->pcftNext;
                        }

                    if (lplpftl)
                        *lplpftl = lpftl;
                    else
                        Sync_DestroyFolderList(lpftl);
                    }
                }
            }
        else
            {
            HOBJECTTWIN hot = NULL;
            TCHAR szDir[MAX_PATH];

            // Add the twins to the reclist "the object way"
            //
            lstrcpy(szDir, lpcszPath);
            PathRemoveFileSpec(szDir);
            Sync_GetObject(hbrf, szDir, PathFindFileName(lpcszPath), &hot);

            if (hot)                // Is this actually a twin?
                {
                // yep
                Sync_AddToTwinList(htl, hot);
                Sync_ReleaseTwin(hot);
                }
            if (lplpftl)
                *lplpftl = NULL;
            bRet = TRUE;
            }
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Asks the user to confirm splitting one or more files.

Returns: IDYES or IDNO
Cond:    --
*/
int PRIVATE ConfirmSplit(
    HWND hwndOwner,
    LPCTSTR pszPath,
    UINT cFiles)
    {
    int idRet;

    ASSERT(pszPath);
    ASSERT(1 <= cFiles);

    // Multiple files?
    if (1 < cFiles)
        {
        // Yes
        idRet = MsgBox(hwndOwner, 
                        MAKEINTRESOURCE(IDS_MSG_ConfirmMultiSplit), 
                        MAKEINTRESOURCE(IDS_CAP_ConfirmMultiSplit), 
                        LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_SPLIT_MULT)), 
                        MB_QUESTION,
                        cFiles);
        }
    else
        {
        // No
        UINT ids;
        UINT idi;
        TCHAR szName[MAX_PATH];

        if (PathIsDirectory(pszPath))
            {
            ids = IDS_MSG_ConfirmFolderSplit;
            idi = IDI_SPLIT_FOLDER;
            }
        else
            {
            ids = IDS_MSG_ConfirmFileSplit;
            idi = IDI_SPLIT_FILE;
            }

        idRet = MsgBox(hwndOwner, 
                        MAKEINTRESOURCE(ids), 
                        MAKEINTRESOURCE(IDS_CAP_ConfirmSplit), 
                        LoadIcon(g_hinst, MAKEINTRESOURCE(idi)), 
                        MB_QUESTION,
                        PathGetDisplayName(pszPath, szName));
        }
    return idRet;
    }


/*----------------------------------------------------------
Purpose: Splits a path from its sync copy.  Private function
         called by Sync_Split.

Returns: standard result
         S_OK if it is split

Cond:    --
*/
HRESULT PRIVATE SplitPath(
    HBRFCASE hbrf,
    LPCTSTR pszPath,
    HWND hwndOwner,
    UINT uFlags)            // SF_* flags
    {
    HRESULT hres;
    TWINRESULT tr;
    TCHAR sz[MAX_PATH];

    if (pszPath)
        {
        // Is the object a folder?
        if (IsFlagSet(uFlags, SF_ISFOLDER) || 
            PathIsDirectory(pszPath))
            {
            // Yup
            BOOL bIsTwin;

            if (IsFlagSet(uFlags, SF_ISTWIN))           // Optimization
                {
                tr = TR_SUCCESS;
                bIsTwin = TRUE;
                }
            else if (IsFlagSet(uFlags, SF_ISNOTTWIN))   // Optimization
                {
                tr = TR_SUCCESS;
                bIsTwin = FALSE;
                }
            else
                {
                tr = Sync_IsFolder(hbrf, pszPath, &bIsTwin);
                }

            // Is this folder a twin?
            if (TR_SUCCESS == tr)
                {
                if (bIsTwin)
                    {
                    // Yes; delete all the twin handles associated with it
                    PFOLDERTWINLIST lpftl;

                    tr = Sync_CreateFolderList(hbrf, pszPath, &lpftl);
                    if (TR_SUCCESS == tr)
                        {
                        PCFOLDERTWIN lpcfolder;

                        ASSERT(lpftl);

                        for (lpcfolder = lpftl->pcftFirst; lpcfolder; 
                            lpcfolder = lpcfolder->pcftNext)
                            {
                            Sync_DeleteTwin(lpcfolder->hftOther);
                            }

                        Sync_DestroyFolderList(lpftl);

                        if (IsFlagClear(uFlags, SF_QUIET))
                            {
                            // Send a notification so it is redrawn.
                            PathNotifyShell(pszPath, NSE_UPDATEITEM, FALSE);
                            }
                        hres = NOERROR;
                        }
                    }
                else if (IsFlagClear(uFlags, SF_QUIET))
                    {
                    // No
                    MsgBox(hwndOwner, 
                            MAKEINTRESOURCE(IDS_MSG_FolderAlreadyOrphan),
                            MAKEINTRESOURCE(IDS_CAP_Split), 
                            LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_SPLIT_FOLDER)), 
                            MB_INFO,
                            PathGetDisplayName(pszPath, sz));

                    hres = S_FALSE;
                    }
                }
            }
        else
            {
            // No; it is a file
            HOBJECTTWIN hot;
            ULONG ulc;

            lstrcpy(sz, pszPath);
            PathRemoveFileSpec(sz);

            // Is this file a twin?
            // (We need the twin handle below, so we cannot take 
            // advantage of SF_ISTWIN or SF_ISNOTTWIN.)
            tr = Sync_GetObject(hbrf, sz, PathFindFileName(pszPath), &hot);

            if (TR_SUCCESS == tr)
                {
                if (hot)
                    {
                    // Yes; is this inside a folder twin?
                    // (If we remove this check, the engine needs to be able
                    // to exclude specific files from a folder twin.)
                    tr = Sync_CountSourceFolders(hot, &ulc);
                    if (TR_SUCCESS == tr)
                        {
                        if (0 < ulc)
                            {
                            // Yes; can't do it
                            if (IsFlagClear(uFlags, SF_QUIET))
                                {
                                UINT rgids[2] = { IDS_ERR_1_CantSplit, IDS_ERR_2_CantSplit };
                                LPTSTR psz;

                                if (FmtString(&psz, IDS_ERR_F_CantSplit, rgids, ARRAYSIZE(rgids)))
                                    {
                                    // This object twin belongs to a folder twin.  We can't
                                    //  allow this action.
                                    MsgBox(hwndOwner, psz, MAKEINTRESOURCE(IDS_CAP_STATUS), 
                                        LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_SPLIT_FILE)), MB_ERROR);
                                    GFree(psz);
                                    }
                                }
                            hres = S_FALSE;
                            }
                        else
                            {
                            // No; delete the twin
                            Sync_DeleteTwin(hot);
            
                            if (IsFlagClear(uFlags, SF_QUIET))
                                {
                                // Send a notification so it's redrawn immediately.
                                PathNotifyShell(pszPath, NSE_UPDATEITEM, FALSE);
                                }
                            hres = NOERROR;
                            }
                        }

                    Sync_ReleaseTwin(hot);
                    }
                else if (IsFlagClear(uFlags, SF_QUIET))
                    {
                    // No
                    MsgBox(hwndOwner, 
                            MAKEINTRESOURCE(IDS_MSG_FileAlreadyOrphan), 
                            MAKEINTRESOURCE(IDS_CAP_Split), 
                            LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_SPLIT_FILE)), 
                            MB_INFO,
                            PathGetDisplayName(pszPath, sz));

                    hres = S_FALSE;
                    }
                }
            }

        if (TR_SUCCESS != tr)
            hres = HRESULT_FROM_TR(tr);
        }
    else
        hres = S_FALSE;

    return hres;
    }


/*----------------------------------------------------------
Purpose: Deletes a series of twins from the engine database.
         The user is optionally asked to confirm the action.

         If a file is an orphan, the user is optionally 
         notified.  The user is also optionally notified
         of any errors.

Returns: standard result
         S_OK if anything was deleted

Cond:    --
*/
HRESULT PUBLIC Sync_Split(
    HBRFCASE hbrf,
    LPCTSTR pszList,
    UINT cFiles,
    HWND hwndOwner,
    UINT uFlags)
    {
    HRESULT hres;
    UINT id;
    TCHAR szCanon[MAX_PATH];
    TCHAR sz[MAX_PATH];

    ASSERT(pszList);
    ASSERT(0 < cFiles);

    // Special precondition: is it a single file?
    if (1 == cFiles)
        {
        // Yes; is it a twin?
        BrfPathCanonicalize(pszList, szCanon);
        hres = Sync_IsTwin(hbrf, szCanon, uFlags);
        if (S_FALSE == hres)
            {
            // No; tell the user.  Don't bother confirming the action first.
            if (IsFlagClear(uFlags, SF_QUIET))
                {
                UINT ids;
                UINT idi;

                if (IsFlagSet(uFlags, SF_ISFOLDER) || 
                    PathIsDirectory(szCanon))
                    {
                    ids = IDS_MSG_FolderAlreadyOrphan;
                    idi = IDI_SPLIT_FOLDER;
                    }
                else
                    {
                    ids = IDS_MSG_FileAlreadyOrphan;
                    idi = IDI_SPLIT_FILE;
                    }

                MsgBox(hwndOwner, 
                        MAKEINTRESOURCE(ids), 
                        MAKEINTRESOURCE(IDS_CAP_Split), 
                        LoadIcon(g_hinst, MAKEINTRESOURCE(idi)), 
                        MB_INFO,
                        PathGetDisplayName(szCanon, sz));
                }
            }
        else if (S_OK == hres)
            {
            // Yes
            if (IsFlagClear(uFlags, SF_NOCONFIRM))
                id = ConfirmSplit(hwndOwner, szCanon, 1);
            else
                id = IDYES;

            if (IDYES == id)
                {
                hres = SplitPath(hbrf, szCanon, hwndOwner, uFlags);
                if (IsFlagClear(uFlags, SF_QUIET))
                    {
                    SHChangeNotifyHandleEvents();
                    }
                }
            else
                hres = S_FALSE;
            }
        }

    // Multiselection: ask the user first
    else
        {
        if (IsFlagClear(uFlags, SF_NOCONFIRM))
            id = ConfirmSplit(hwndOwner, pszList, cFiles);
        else
            id = IDYES;

        if (IDYES == id)
            {
            // Remove all the files from the engine database
            LPCTSTR psz;
            UINT i;
            HRESULT hresT;

            hres = S_FALSE;     // assume success but nothing done

            for (i = 0, psz = pszList; i < cFiles; i++)
                {
                // Get dragged file/folder name
                //
                BrfPathCanonicalize(psz, szCanon);

                hresT = SplitPath(hbrf, szCanon, hwndOwner, uFlags);
                if (S_OK == hresT)
                    hres = S_OK;  // (Don't set back to FALSE once it is TRUE)
                else if (FAILED(hresT))
                    {
                    hres = hresT;
                    break;
                    }

                DataObj_NextFile(psz);      // Set psz to next file in list
                }

            if (IsFlagClear(uFlags, SF_QUIET))
                {
                SHChangeNotifyHandleEvents();    // (Do this after the loop)
                }
            }
        else
            hres = S_FALSE;
        }
    return hres;
    }


/*----------------------------------------------------------
Purpose: Change the recitem action and the two recnodes of
         importance to the specified action.
Returns: --
Cond:    --
*/
void PUBLIC Sync_ChangeRecItemAction(
    PRECITEM pri,
    LPCTSTR pszBrfPath,
    LPCTSTR pszInsideDir,     // Folder inside the briefcase
    UINT riaction)           // One of RAIA_* values to change to
    {
    HRESULT hres;
    PRECNODE prnInside;
    PRECNODE prnOutside;

    // Determine which node is inside the briefcase and which one is
    //  outside.
    //
    hres = Sync_GetNodePair(pri, pszBrfPath, pszInsideDir, &prnInside, &prnOutside);
    if (SUCCEEDED(hres))
        {
        ASSERT(prnInside);
        ASSERT(prnOutside);

        switch(riaction)
            {
        case RAIA_TOIN:
            pri->dwUser = RIU_CHANGED;
            pri->riaction = RIA_COPY;
            prnInside->rnaction = RNA_COPY_TO_ME;
            prnOutside->rnaction = RNA_COPY_FROM_ME;
            break;

        case RAIA_TOOUT:
            pri->dwUser = RIU_CHANGED;
            pri->riaction = RIA_COPY;
            prnInside->rnaction = RNA_COPY_FROM_ME;
            prnOutside->rnaction = RNA_COPY_TO_ME;
            break;

        case RAIA_SKIP:
            pri->dwUser = RIU_SKIP;
            break;

        case RAIA_MERGE:
            pri->dwUser = RIU_CHANGED;
            pri->riaction = RIA_MERGE;
            prnInside->rnaction = RNA_MERGE_ME;
            prnOutside->rnaction = RNA_MERGE_ME;
            break;

#ifdef NEW_REC
        case RAIA_DONTDELETE:
            if (RNA_DELETE_ME == prnInside->rnaction)
                {
                pri->dwUser = RIU_CHANGED;
                pri->riaction = RIA_NOTHING;
                prnInside->rnaction = RNA_NOTHING;
                }
            else if (RNA_DELETE_ME == prnOutside->rnaction)
                {
                pri->dwUser = RIU_CHANGED;
                pri->riaction = RIA_NOTHING;
                prnOutside->rnaction = RNA_NOTHING;
                }
            break;
        
        case RAIA_DELETEIN:
            pri->dwUser = RIU_CHANGED;
            pri->riaction = RIA_DELETE;
            prnInside->rnaction = RNA_DELETE_ME;
            prnOutside->rnaction = RNA_NOTHING;
            break;

        case RAIA_DELETEOUT:
            pri->dwUser = RIU_CHANGED;
            pri->riaction = RIA_DELETE;
            prnInside->rnaction = RNA_NOTHING;
            prnOutside->rnaction = RNA_DELETE_ME;
            break;
#endif

        default:
            // (The other values don't make sense here)
            ASSERT(0);
            break;
            }
        }
    }


/////////////////////////////////////////////////////  PRIVATE FUNCTIONS


#ifdef DEBUG
/*----------------------------------------------------------
Purpose: Dumps the contents of the given twin structure to
         to debug out
Returns: --
Cond:    --
*/
void PUBLIC Sync_FnDump(
    LPVOID lpvBuf,
    UINT cbBuf)
    {
    int bDump;

    #define szDumpTwin  TEXT("Dump TWIN: ")
    #define szDumpSp    TEXT("           ")

    ENTEREXCLUSIVE()
        {
        bDump = IsFlagSet(g_uDumpFlags, DF_CREATETWIN);
        }
    LEAVEEXCLUSIVE()

    if (!bDump)
        return ;

    if (cbBuf == sizeof(NEWOBJECTTWIN))
        {
        PNEWOBJECTTWIN lpnot = (PNEWOBJECTTWIN)lpvBuf;

        TRACE_MSG(TF_ALWAYS, TEXT("%s.Folder1 = {%s}\r\n%s.Folder2 = {%s}\r\n%s.Name = {%s}\r\n"),
            (LPTSTR)szDumpTwin, lpnot->pcszFolder1,
            (LPTSTR)szDumpSp, lpnot->pcszFolder2,
            (LPTSTR)szDumpSp, lpnot->pcszName);
        }
    else if (cbBuf == sizeof(NEWFOLDERTWIN))
        {
        PNEWFOLDERTWIN lpnft = (PNEWFOLDERTWIN)lpvBuf;

        TRACE_MSG(TF_ALWAYS, TEXT("%s.Folder1 = {%s}\r\n%s.Folder2 = {%s}\r\n%s.Name = {%s}\r\n%s.dwFlags = 0x%04lx\r\n"),
            (LPTSTR)szDumpTwin, lpnft->pcszFolder1,
            (LPTSTR)szDumpSp, lpnft->pcszFolder2,
            (LPTSTR)szDumpSp, lpnft->pcszName,
            (LPTSTR)szDumpSp, (DWORD)lpnft->dwFlags);
        }
    }


/*----------------------------------------------------------
Purpose: Return English form of RIA_ flags
Returns:
Cond:    --
*/
LPTSTR PRIVATE LpszFromItemAction(
    ULONG riaction)
    {
    switch (riaction)
        {
    DEBUG_CASE_STRING( RIA_NOTHING );
    DEBUG_CASE_STRING( RIA_COPY );
    DEBUG_CASE_STRING( RIA_MERGE );
    DEBUG_CASE_STRING( RIA_BROKEN_MERGE );

#ifdef NEW_REC
    DEBUG_CASE_STRING( RIA_DELETE );
#endif

    default:        return TEXT("RIA unknown");
        }
    }


/*----------------------------------------------------------
Purpose: Return English form of RNA_ flags
Returns:
Cond:    --
*/
LPTSTR PRIVATE LpszFromNodeAction(
    ULONG rnaction)
    {
    switch (rnaction)
        {
    DEBUG_CASE_STRING( RNA_NOTHING );
    DEBUG_CASE_STRING( RNA_COPY_TO_ME );
    DEBUG_CASE_STRING( RNA_COPY_FROM_ME );
    DEBUG_CASE_STRING( RNA_MERGE_ME );

#ifdef NEW_REC
    DEBUG_CASE_STRING( RNA_DELETE_ME );
#endif

    default:    return TEXT("RNA unknown");
        }
    }


/*----------------------------------------------------------
Purpose: Return English form of RNS_ flags
Returns:
Cond:    --
*/
LPTSTR PRIVATE LpszFromNodeState(
    ULONG rnstate)
    {
    switch (rnstate)
        {
#ifdef NEW_REC
    DEBUG_CASE_STRING( RNS_NEVER_RECONCILED );
#endif

    DEBUG_CASE_STRING( RNS_UNAVAILABLE );
    DEBUG_CASE_STRING( RNS_DOES_NOT_EXIST );
    DEBUG_CASE_STRING( RNS_DELETED );
    DEBUG_CASE_STRING( RNS_NOT_RECONCILED );
    DEBUG_CASE_STRING( RNS_UP_TO_DATE );
    DEBUG_CASE_STRING( RNS_CHANGED );

    default: return TEXT("RNS unknown");
        }
    }


/*----------------------------------------------------------
Purpose: Dump the RECNODE
Returns:
Cond:    --
*/
void PUBLIC Sync_DumpRecNode(
    TWINRESULT tr,
    PRECNODE lprn)
    {
    BOOL bDump;
    TCHAR szBuf[MAXMSGLEN];

    #define szDumpLabel     TEXT("\tDump RECNODE: ")
    #define szDumpMargin    TEXT("\t              ")

    ENTEREXCLUSIVE()
        {
        bDump = IsFlagSet(g_uDumpFlags, DF_RECNODE);
        }
    LEAVEEXCLUSIVE()

    if (!bDump || lprn == NULL || tr == TR_OUT_OF_MEMORY || tr == TR_INVALID_PARAMETER)
        return ;

    wsprintf(szBuf, TEXT("%s.Folder = {%s}\r\n"), (LPTSTR)szDumpLabel, lprn->pcszFolder);
    OutputDebugString(szBuf);

    wsprintf(szBuf, TEXT("%s.hObjectTwin = %lx\r\n"), (LPTSTR)szDumpMargin, lprn->hObjectTwin);
    OutputDebugString(szBuf);

    wsprintf(szBuf, TEXT("%s.rnstate = %s\r\n"), (LPTSTR)szDumpMargin, LpszFromNodeState(lprn->rnstate));
    OutputDebugString(szBuf);

    wsprintf(szBuf, TEXT("%s.rnaction = %s\r\n"), (LPTSTR)szDumpMargin, LpszFromNodeAction(lprn->rnaction));
    OutputDebugString(szBuf);
    OutputDebugString(TEXT("\r\n"));

    #undef szDumpLabel
    #undef szDumpMargin
    }


/*----------------------------------------------------------
Purpose: Dump the RECITEM
Returns:
Cond:    --
*/
void PUBLIC Sync_DumpRecItem(
    TWINRESULT tr,
    PRECITEM lpri,
    LPCTSTR pszMsg)
    {
    BOOL bDump;
    PRECNODE lprn;
    TCHAR szBuf[MAXMSGLEN];

    #define szDumpLabel     TEXT("Dump RECITEM: ")
    #define szDumpMargin    TEXT("              ")

    ENTEREXCLUSIVE()
        {
        bDump = IsFlagSet(g_uDumpFlags, DF_RECITEM);
        }
    LEAVEEXCLUSIVE()

    if (!bDump || lpri == NULL || tr == TR_OUT_OF_MEMORY || tr == TR_INVALID_PARAMETER)
        return ;

    if (pszMsg)
        TRACE_MSG(TF_ALWAYS, pszMsg);

    wsprintf(szBuf, TEXT("tr = %s\r\n"), (LPTSTR)SzFromTR(tr));
    OutputDebugString(szBuf);

    wsprintf(szBuf, TEXT("%s.Name = {%s}\r\n"), (LPTSTR)szDumpLabel, lpri->pcszName);
    OutputDebugString(szBuf);

    wsprintf(szBuf, TEXT("%s.hTwinFamily = %lx\r\n"), (LPTSTR)szDumpMargin, lpri->hTwinFamily);
    OutputDebugString(szBuf);

    wsprintf(szBuf, TEXT("%s.ulcNodes = %lu\r\n"), (LPTSTR)szDumpMargin, lpri->ulcNodes);
    OutputDebugString(szBuf);

    wsprintf(szBuf, TEXT("%s.riaction = %s\r\n"), (LPTSTR)szDumpMargin, LpszFromItemAction(lpri->riaction));
    OutputDebugString(szBuf);

    lprn = lpri->prnFirst;
    while (lprn)
        {
        Sync_DumpRecNode(tr, lprn);
        lprn = lprn->prnNext;
        }

    #undef szDumpLabel
    #undef szDumpMargin
    }


/*----------------------------------------------------------
Purpose: Dump the RECLIST
Returns:
Cond:    --
*/
void PUBLIC Sync_DumpRecList(
    TWINRESULT tr,
    PRECLIST lprl,
    LPCTSTR pszMsg)
    {
    BOOL bDump;
    PRECITEM lpri;
    TCHAR szBuf[MAXMSGLEN];

    #define szDumpLabel   TEXT("Dump RECLIST: ")

    ENTEREXCLUSIVE()
        {
        bDump = IsFlagSet(g_uDumpFlags, DF_RECLIST);
        }
    LEAVEEXCLUSIVE()

    if (!bDump)
        return ;

    if (pszMsg)
        TRACE_MSG(TF_ALWAYS, pszMsg);

    // Note we only dump on TR_SUCCESS
    //
    wsprintf(szBuf, TEXT("tr = %s\r\n"), (LPTSTR)SzFromTR(tr));
    OutputDebugString(szBuf);

    if (lprl == NULL || tr == TR_OUT_OF_MEMORY || tr == TR_INVALID_PARAMETER)
        return ;

    wsprintf(szBuf, TEXT("%s.ulcItems = %lu\r\n"), (LPTSTR)szDumpLabel, lprl->ulcItems);
    OutputDebugString(szBuf);

    lpri = lprl->priFirst;
    while (lpri)
        {
        Sync_DumpRecItem(TR_SUCCESS, lpri, NULL);
        lpri = lpri->priNext;
        }

    #undef szDumpLabel
    }


/*----------------------------------------------------------
Purpose: Dump the FOLDERTWIN
Returns: --
Cond:    --
*/
void PUBLIC Sync_DumpFolderTwin(
    PCFOLDERTWIN pft)
    {
    BOOL bDump;
    TCHAR szBuf[MAXMSGLEN];

    #define szDumpLabel      TEXT("Dump FOLDERTWIN: ")
    #define szDumpMargin     TEXT("                 ")

    ENTEREXCLUSIVE()
        {
        bDump = IsFlagSet(g_uDumpFlags, DF_FOLDERTWIN);
        }
    LEAVEEXCLUSIVE()

    if (!bDump || pft == NULL)
        return ;

    wsprintf(szBuf, TEXT("%s.Name = {%s}\r\n"), (LPTSTR)szDumpLabel, pft->pcszName);
    OutputDebugString(szBuf);

    wsprintf(szBuf, TEXT("%s.pszSrcFolder = {%s}\r\n"), (LPTSTR)szDumpMargin, pft->pcszSrcFolder);
    OutputDebugString(szBuf);

    wsprintf(szBuf, TEXT("%s.pszOtherFolder = {%s}\r\n"), (LPTSTR)szDumpMargin, pft->pcszOtherFolder);
    OutputDebugString(szBuf);

    wsprintf(szBuf, TEXT("%s.dwFlags = %lx\r\n"), (LPTSTR)szDumpMargin, pft->dwFlags);
    OutputDebugString(szBuf);

    wsprintf(szBuf, TEXT("%s.dwUser = %lx\r\n"), (LPTSTR)szDumpMargin, pft->dwUser);
    OutputDebugString(szBuf);

    #undef szDumpLabel
    #undef szDumpMargin
    }


/*----------------------------------------------------------
Purpose: Dump the FOLDERTWINLIST
Returns: --
Cond:    --
*/
void PUBLIC Sync_DumpFolderTwinList(
    PFOLDERTWINLIST pftl,
    LPCTSTR pszMsg)
    {
    BOOL bDump;
    PCFOLDERTWIN pft;
    TCHAR szBuf[MAXMSGLEN];

    #define szDumpLabel   TEXT("Dump FOLDERTWINLIST: ")

    ENTEREXCLUSIVE()
        {
        bDump = IsFlagSet(g_uDumpFlags, DF_FOLDERTWIN);
        }
    LEAVEEXCLUSIVE()

    if (!bDump)
        return ;

    if (pszMsg)
        TRACE_MSG(TF_ALWAYS, pszMsg);

    if (pftl == NULL)
        return ;

    wsprintf(szBuf, TEXT("%s.ulcItems = %lu\r\n"), (LPTSTR)szDumpLabel, pftl->ulcItems);
    OutputDebugString(szBuf);

    for (pft = pftl->pcftFirst; pft; pft = pft->pcftNext)
        {
        Sync_DumpFolderTwin(pft);
        }

    #undef szDumpLabel
    }



#endif




