//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: ibrfstg.c
//
//  This files contains the IBriefcaseStg interface.
//
// History:
//  02-02-94 ScottH     Converted from iface.c
//
//---------------------------------------------------------------------------

#include "brfprv.h"         // common headers
#undef LODWORD              // (because they are redefined by configmg.h)
#undef HIDWORD

#include <brfcasep.h>
#include "recact.h"
#include "res.h"

#include <help.h>

#ifdef WINNT
    // BUGBUG - BobDay - We need some mechanism of determining dock state
#else
    // Needed for dock state determination
#define Not_VxD
#define No_CM_Calls
#include <vmm.h>
#include <configmg.h>
#endif

//---------------------------------------------------------------------------
// BriefStg Class
//---------------------------------------------------------------------------

// An IBriefcaseStg interface instance is created for each
// folder the caller (the Shell) binds to, where the folder
// is known to be inside a briefcase storage.  A briefcase
// storage is the overall storage area (the database) that
// starts at a given folder (called the "briefcase root")
// and extends onwards and below in the file-system.
//
// Internally, the briefcase storage holds the path to the
// folder that this instance is bound to, and it holds a
// cached briefcase structure (CBS), which itself holds a
// reference to the briefcase root.
//
typedef struct _BriefStg
    {
    IBriefcaseStg   bs;
    UINT            cRef;           // reference count
    CBS *           pcbs;           // cached briefcase info
    TCHAR            szFolder[MAX_PATH]; // canonical path
    HBRFCASEITER    hbrfcaseiter;   // handle to iterate briefcases
    DWORD           dwFlags;        // BSTG_* flags
    } BriefStg, * PBRIEFSTG;

// Flags for BriefStg
#define BSTG_SYNCFOLDER     0x00000001      // This folder has a sync copy


//---------------------------------------------------------------------------
// Supporting private code
//---------------------------------------------------------------------------


#ifdef DEBUG
/*----------------------------------------------------------
Purpose: Dump all the cache tables
Returns: --
Cond:    --
*/
void PUBLIC DumpTables()
    {
    Atom_DumpAll();
    CBS_DumpAll();
    CRL_DumpAll();
    }
#endif


/*----------------------------------------------------------
Purpose: Initialize the cache tables
Returns: --
Cond:    --
*/
BOOL PRIVATE InitCacheTables()
    {
    ASSERT(Sync_IsEngineLoaded());

    DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Initialize cache tables")); )

    if (!CBS_Init())
        goto Init_Fail;

    if (!CRL_Init())
        goto Init_Fail;

    return TRUE;

Init_Fail:

    CRL_Term();
    CBS_Term(NULL);
    return FALSE;
    }


/*----------------------------------------------------------
Purpose: Terminate the cache tables
Returns: --
Cond:    --
*/
void PUBLIC TermCacheTables(void)
    {
    ASSERT(Sync_IsEngineLoaded());

    DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Terminate cache tables")); )

    CRL_Term();

    CBS_Term(NULL);
    }


/*----------------------------------------------------------
Purpose: Returns TRUE if the path (a folder) has a sync copy.

Returns: see above
Cond:    --
*/
BOOL PRIVATE HasFolderSyncCopy(
    HBRFCASE hbrf,
    LPCTSTR pszPath)
    {
    ASSERT(pszPath);
    ASSERT(PathIsDirectory(pszPath));

    return (S_OK == Sync_IsTwin(hbrf, pszPath, SF_ISFOLDER) ||
            IsSubfolderTwin(hbrf, pszPath));
    }


/*----------------------------------------------------------
Purpose: Open a folder that belongs to a briefcase storage.
         The pszPath parameter is a folder, which is not necessarily
         the briefcase root.

Returns: NOERROR on success
Cond:    --
*/
HRESULT PRIVATE OpenBriefcaseStorage(
    LPCTSTR pszPath,
    CBS ** ppcbs,
    HWND hwndOwner)
    {
    HRESULT hres;
    UINT uLocality;
    int atomBrf;
    TCHAR szBrfPath[MAX_PATH];
    TCHAR szBrfCanon[MAX_PATH];

    ASSERT(pszPath);
    ASSERT(ppcbs);

    DBG_ENTER_SZ(TEXT("OpenBriefcaseStorage"), pszPath);
    DEBUG_CODE( DEBUG_BREAK(BF_ONOPEN); )

    // Get the root folder of the briefcase storage
    // Get strictly up to the briefcase portion of path
    //
    uLocality = PathGetLocality(pszPath, szBrfPath);
    if (PL_FALSE == uLocality)
        {
        // The only time we get here is if the caller had a legitimate
        // reason to believe this folder was a briefcase storage,
        // but no database exists (yet).  Just continue on as normal,
        // the database will get created later.
        BrfPathCanonicalize(pszPath, szBrfCanon);
        }
    else
        {
        BrfPathCanonicalize(szBrfPath, szBrfCanon);
        }

    // Add this path to the atom list and add it to the
    // cached briefcase structure table.
    // (Reference count decrement happens in CloseBriefcase)
    //
    atomBrf = Atom_Add(szBrfCanon);
    if (atomBrf != ATOM_ERR)
        {
        hres = CBS_Add(ppcbs, atomBrf, hwndOwner);
        }
    else
        {
        *ppcbs = NULL;
        hres = ResultFromScode(E_OUTOFMEMORY);
        }

    DEBUG_CODE( DumpTables(); )

    DBG_EXIT_HRES(TEXT("OpenBriefcaseStorage"), hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: Close a briefcase.

Returns: NOERROR on success
Cond:    --
*/
HRESULT PRIVATE CloseBriefcaseStorage(
    LPCTSTR pszPath)
    {
    int atomBrf;
    TCHAR szBrfPath[MAX_PATH];
    TCHAR szBrfCanon[MAX_PATH];
    UINT uLocality;

    ASSERT(pszPath);
    ASSERT(*pszPath);       // Should not be an emptry string

    DBG_ENTER_SZ(TEXT("CloseBriefcaseStorage"), pszPath);
    DEBUG_CODE( DEBUG_BREAK(BF_ONCLOSE); )

    DEBUG_CODE( DumpTables(); )

    // Save the briefcase and remove it from the cache
    //
    // Get the root folder of the briefcase storage
    // Get strictly up to the briefcase portion of path
    //
    uLocality = PathGetLocality(pszPath, szBrfPath);
    if (PL_FALSE == uLocality)
        {
        // The only time we get here is for a briefcase storage that
        // has no database yet.  Just continue on as normal,
        // the database will get created very soon now.
        BrfPathCanonicalize(pszPath, szBrfCanon);
        }
    else
        {
        BrfPathCanonicalize(szBrfPath, szBrfCanon);
        }

    atomBrf = Atom_Find(szBrfCanon);
    ASSERT(atomBrf != ATOM_ERR);

    CBS_Delete(atomBrf, NULL);

    Atom_Delete(atomBrf);      // for the Add in OpenBriefcaseStorage

    DBG_EXIT_HRES(TEXT("CloseBriefcaseStorage"), NOERROR);

    return NOERROR;
    }


// Confirm button flags
#define CBF_YES         0x0001
#define CBF_NO          0x0002
#define CBF_TOALL       0x0004
#define CBF_CANCEL      0x0008

/*----------------------------------------------------------
Purpose: Checks to see if the given file/folder already exists
          in the given directory.  Prompts the user to confirm
          replacing if this is true.

Returns: TRUE if path exists
         confirm flag settings

Cond:    --
*/
BOOL PRIVATE DoesPathAlreadyExist(
    CBS  * pcbs,
    LPCTSTR pszPathOld,
    LPCTSTR pszPathNew,
    LPUINT puConfirmFlags,  // CBF_*
    UINT uFlags,            // SF_ISFOLDER or SF_ISFILE
    HWND hwndOwner,
    BOOL bMultiDrop)
    {
    BOOL bRet;
    BOOL bIsTwin;

    ASSERT(puConfirmFlags);

    // Retain settings of *puConfirmFlags coming in

    bIsTwin = (S_OK == Sync_IsTwin(pcbs->hbrf, pszPathOld, uFlags));
    if (bIsTwin)
        uFlags |= SF_ISTWIN;
    else
        uFlags |= SF_ISNOTTWIN;

    bRet = (FALSE != PathExists(pszPathOld));

    // Does the path already exist?
    if (!bRet)
        {
        // No; remove it from the database if it is in there so we
        // don't add duplicates.
        Sync_Split(pcbs->hbrf, pszPathOld, 1, hwndOwner, uFlags | SF_QUIET | SF_NOCONFIRM);
        }
    else
        {
        // Yes; has a "to all" previously been specified by the user?
        if (IsFlagSet(*puConfirmFlags, CBF_TOALL))
            {
            // Yes; keep flags as they are

            // (CBF_YES and CBF_NO flags are mutually exclusive)
            ASSERT(IsFlagSet(*puConfirmFlags, CBF_YES) &&
                        IsFlagClear(*puConfirmFlags, CBF_NO | CBF_CANCEL) ||
                   IsFlagSet(*puConfirmFlags, CBF_NO) &&
                        IsFlagClear(*puConfirmFlags, CBF_YES | CBF_CANCEL));
            }
        else
            {
            // No; prompt the user
            UINT uFlagsCRF = bMultiDrop ? CRF_MULTI : CRF_DEFAULT;
            int id = ConfirmReplace_DoModal(hwndOwner, pszPathOld, pszPathNew, uFlagsCRF);

            *puConfirmFlags = 0;

            if (GetKeyState(VK_SHIFT) < 0)
                SetFlag(*puConfirmFlags, CBF_TOALL);

            if (IDYES == id)
                SetFlag(*puConfirmFlags, CBF_YES);
            else if (IDNO == id)
                SetFlag(*puConfirmFlags, CBF_NO);
            else if (IDC_YESTOALL == id)
                SetFlag(*puConfirmFlags, CBF_YES | CBF_TOALL);
            else
                {
                ASSERT(IDCANCEL == id);
                SetFlag(*puConfirmFlags, CBF_CANCEL);
                }
            }

        // Has the user chosen to replace the file?
        if (IsFlagSet(*puConfirmFlags, CBF_YES))
            {
            // Yes; is this an existing twin?
            if (bIsTwin)
                {
                // Yes; delete it from the database before we continue
                Sync_Split(pcbs->hbrf, pszPathOld, 1, hwndOwner, SF_QUIET | SF_NOCONFIRM);
                }

            // Some merge-handlers need the unwanted file to be deleted
            // first because they cannot tell the difference between
            // a newly added file (that is replacing an existing file)
            // and a one-way merge.
            if (!PathIsDirectory(pszPathOld))
                DeleteFile(pszPathOld);
            }
        }
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Add the folder twin to the database, using the default
         *.* wildcard settings.

Returns: standard result
Cond:    --
*/
HRESULT PRIVATE AddDefaultFolderTwin(
    HWND hwndOwner,
    HBRFCASE hbrf,
    HDPA hdpa,               // Return: twin handle in array
    LPCTSTR pszPathFrom,      // Source path
    LPCTSTR pszPathTo)        // Target path
    {
    HRESULT hres;
    int iTwin;

    // First make sure we can add another handle to hdpa (set to zero for now)
    if (DPA_ERR == (iTwin = DPA_InsertPtr(hdpa, DPA_APPEND, (LPVOID)NULL)))
        {
        hres = E_OUTOFMEMORY;
        }
    else
        {
        NEWFOLDERTWIN nft;
        TWINRESULT tr;
        HFOLDERTWIN hft;

        RETRY_BEGIN(FALSE)
            {
            ZeroInit(&nft, NEWFOLDERTWIN);
            nft.ulSize = sizeof(nft);
            nft.pcszFolder1 = pszPathFrom;
            nft.pcszFolder2 = pszPathTo;
            nft.pcszName = c_szAllFiles;
            nft.dwAttributes = OBJECT_TWIN_ATTRIBUTES;
            nft.dwFlags = NFT_FL_SUBTREE;

            // Add the twin
            tr = Sync_AddFolder(hbrf, &nft, &hft);
            hres = HRESULT_FROM_TR(tr);

            if (FAILED(hres))
                {
                DWORD dwError = GetLastError();
                int id;
                extern SETbl const c_rgseInfo[4];

                // Unavailable disk?
                if (ERROR_INVALID_DATA == dwError || ERROR_ACCESS_DENIED == dwError)
                    {
                    // Yes
                    hres = E_TR_UNAVAILABLE_VOLUME;
                    }

                id = SEMsgBox(hwndOwner, IDS_CAP_INFO, hres, c_rgseInfo, ARRAYSIZE(c_rgseInfo));
                if (IDRETRY == id)
                    {
                    // Try the operation again
                    RETRY_SET();
                    }
                }
            }
        RETRY_END()

        if (FAILED(hres))
            {
            DPA_DeletePtr(hdpa, iTwin);
            }
        else
            {
            // Success
            ASSERT(DPA_ERR != iTwin);
            ASSERT(NULL != hft);
            DPA_SetPtr(hdpa, iTwin, hft);
            }
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Create a twin relationship between a folder and
         another folder.

Returns: standard hresult
         handles to created twins in hdpa
         confirm flag settings

Cond:    --
*/
HRESULT PRIVATE CreateTwinOfFolder(
    CBS  * pcbs,
    LPTSTR pszPath,          // Dragged folder path
    LPCTSTR pszDir,          // Location to place twin
    HDPA hdpaTwin,          // array of twin handles
    UINT uFlags,            // AOF_*
    PUINT puConfirmFlags,   // CBF_*
    HWND hwndOwner,
    BOOL bMultiDrop)        // TRUE: more than 1 file/folder was dropped
    {
    HRESULT hres;
    TCHAR szPathB[MAX_PATH];
    LPTSTR pszFile;

    ASSERT(pszPath);
    ASSERT(pszDir);

    pszFile = PathFindFileName(pszPath);

    // Will the path name be too long?
    if (PathsTooLong(pszDir, pszFile))
        {
        // Yes; bail
        MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_ERR_ADDFOLDER_TOOLONG),
               MAKEINTRESOURCE(IDS_CAP_ADD), NULL, MB_ERROR, pszFile);
        hres = E_FAIL;
        }
    // Did the user drag another briefcase root into this briefcase?
    else if (PathIsBriefcase(pszPath))
        {
        // Yes; we don't allow nested briefcases!  Tell the user.
        MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_ERR_CANTADDBRIEFCASE),
               MAKEINTRESOURCE(IDS_CAP_ADD), NULL, MB_WARNING);
        hres = E_FAIL;
        }
    else
        {
        // No; check for an existing folder in the target folder.
        BOOL bExists;

        PathCombine(szPathB, pszDir, pszFile);
        bExists = DoesPathAlreadyExist(pcbs, szPathB, pszPath, puConfirmFlags, SF_ISFOLDER, hwndOwner, bMultiDrop);

        if (!bExists || IsFlagSet(*puConfirmFlags, CBF_YES))
            {
            ASSERT(IsFlagClear(*puConfirmFlags, CBF_NO) &&
                   IsFlagClear(*puConfirmFlags, CBF_CANCEL));

            // Show 'Add Folder' dialog?
            if (IsFlagSet(uFlags, AOF_FILTERPROMPT))
                {
                // Yes
                hres = Info_DoModal(hwndOwner, pszPath, szPathB, hdpaTwin,
                        pcbs);
                }
            else
                {
                // No; just default to *.*
                hres = AddDefaultFolderTwin(hwndOwner, pcbs->hbrf, hdpaTwin,
                        pszPath, szPathB);
                }
            }
        else if (IsFlagSet(*puConfirmFlags, CBF_NO))
            {
            // The user said NO
            ASSERT(IsFlagClear(*puConfirmFlags, CBF_YES) &&
                   IsFlagClear(*puConfirmFlags, CBF_CANCEL));
            hres = NOERROR;
            }
        else
            {
            ASSERT(IsFlagSet(*puConfirmFlags, CBF_CANCEL));
            ASSERT(IsFlagClear(*puConfirmFlags, CBF_YES) &&
                   IsFlagClear(*puConfirmFlags, CBF_NO));
            hres = E_ABORT;
            }
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Create a twin of a file.

Returns: standard result
         twin handle in hdpa
Cond:    --
*/
HRESULT PRIVATE CreateTwinOfFile(
    CBS  * pcbs,
    LPCTSTR pszPath,         // ptr to path to twin
    LPCTSTR pszTargetDir,    // ptr to dest dir
    HDPA hdpa,              // Return: twin handle in array
    UINT uFlags,            // AOF_*
    PUINT puConfirmFlags,   // CBF_*
    HWND hwndOwner,
    BOOL bMultiDrop)        // TRUE: more than 1 file/folder was dropped
    {
    HRESULT hres;
    int iTwin;
    TCHAR szPath[MAX_PATH];
    LPCTSTR pszFile;
    HTWINFAMILY htfam = NULL;

    ASSERT(pszPath);
    ASSERT(pszTargetDir);

    pszFile = PathFindFileName(pszPath);

    // Will the path name be too long?
    if (PathsTooLong(pszTargetDir, pszFile))
        {
        // Yes; bail
        MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_ERR_ADDFILE_TOOLONG),
               MAKEINTRESOURCE(IDS_CAP_ADD), NULL, MB_ERROR, pszFile);
        iTwin = DPA_ERR;
        hres = E_FAIL;
        }
    // First make sure we can add another handle to hdpa (set to zero for now)
    else if (DPA_ERR == (iTwin = DPA_InsertPtr(hdpa, DPA_APPEND, (LPVOID)NULL)))
        {
        hres = E_OUTOFMEMORY;
        }
    else
        {
        BOOL bExists;

        // Confirm the replace if a file with the same name already exists.
        //
        PathCombine(szPath, pszTargetDir, pszFile);
        bExists = DoesPathAlreadyExist(pcbs, szPath, pszPath, puConfirmFlags, SF_ISFILE, hwndOwner, bMultiDrop);

        if (!bExists ||
            IsFlagSet(*puConfirmFlags, CBF_YES))
            {
            NEWOBJECTTWIN not;
            TWINRESULT tr;
            DECLAREHOURGLASS;

            ASSERT(IsFlagClear(*puConfirmFlags, CBF_NO) &&
                   IsFlagClear(*puConfirmFlags, CBF_CANCEL));

            lstrcpy(szPath, pszPath);
            PathRemoveFileSpec(szPath);

            // User has either opted to continue adding this object to the
            // database, or it does not exist in the destination folder.

            RETRY_BEGIN(FALSE)
                {
                ZeroInit(&not, NEWOBJECTTWIN);
                not.ulSize = sizeof(NEWOBJECTTWIN);
                not.pcszFolder1 = szPath;
                not.pcszFolder2 = pszTargetDir;
                not.pcszName = pszFile;

                SetHourglass();
                Sync_Dump(&not, NEWOBJECTTWIN);
                tr = Sync_AddObject(pcbs->hbrf, &not, &htfam);
                ResetHourglass();

                hres = HRESULT_FROM_TR(tr);

                if (FAILED(hres))
                    {
                    DWORD dwError = GetLastError();

                    // Unavailable disk?
                    if (ERROR_INVALID_DATA == dwError || ERROR_ACCESS_DENIED == dwError)
                        {
                        // Yes; ask user to retry/cancel
                        int id = MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_ERR_ADDFILE_UNAVAIL_VOL),
                            MAKEINTRESOURCE(IDS_CAP_ADD), NULL, MB_RETRYCANCEL | MB_ICONWARNING);

                        // Set specific error value
                        hres = E_TR_UNAVAILABLE_VOLUME;

                        if (IDRETRY == id)
                            {
                            RETRY_SET();    // Try again
                            }
                        }
                    }
                }
            RETRY_END()
            }
        else if (IsFlagSet(*puConfirmFlags, CBF_NO))
            {
            // The user said NO
            ASSERT(IsFlagClear(*puConfirmFlags, CBF_YES) &&
                   IsFlagClear(*puConfirmFlags, CBF_CANCEL));
            DPA_DeletePtr(hdpa, iTwin);
            hres = NOERROR;
            }
        else
            {
            ASSERT(IsFlagSet(*puConfirmFlags, CBF_CANCEL));
            ASSERT(IsFlagClear(*puConfirmFlags, CBF_YES) &&
                   IsFlagClear(*puConfirmFlags, CBF_NO));
            hres = E_ABORT;
            }
        }

    if (FAILED(hres))
        {
        if (DPA_ERR != iTwin)
            {
            DPA_DeletePtr(hdpa, iTwin);
            }
        }
    else
        {
        // Success
        ASSERT(DPA_ERR != iTwin);
        if (htfam)
            DPA_SetPtr(hdpa, iTwin, htfam);
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Deletes the new twins
Returns: --
Cond:    --
*/
void PRIVATE DeleteNewTwins(
    CBS  * pcbs,
    HDPA hdpa)
    {
    int iItem;
    int cItems;

    ASSERT(pcbs);
    ASSERT(hdpa);

    cItems = DPA_GetPtrCount(hdpa);
    for (iItem = 0; iItem < cItems; iItem++)
        {
        HTWIN htwin = DPA_FastGetPtr(hdpa, iItem);

        if (htwin)
            Sync_DeleteTwin(htwin);
        }
    }


/*----------------------------------------------------------
Purpose: Releases the twin handles
Returns: --
Cond:    --
*/
void PRIVATE ReleaseNewTwins(
    HDPA hdpa)
    {
    int i;
    int cItems;

    ASSERT(hdpa);

    cItems = DPA_GetPtrCount(hdpa);
    for (i = 0; i < cItems; i++)
        {
        HTWIN htwin = DPA_FastGetPtr(hdpa, i);

        if (htwin)
            Sync_ReleaseTwin(htwin);
        }
    }


/*----------------------------------------------------------
Purpose: Returns the count of nodes that do not have FS_COND_UNAVAILABLE.

Returns: see above
Cond:    --
*/
UINT PRIVATE CountAvailableNodes(
    PRECITEM pri)
    {
    UINT ucNodes = 0;
    PRECNODE prn;

    for (prn = pri->prnFirst; prn; prn = prn->prnNext)
        {
        if (FS_COND_UNAVAILABLE != prn->fsCurrent.fscond)
            {
            ucNodes++;
            }
        }
    return ucNodes;
    }


/*----------------------------------------------------------
Purpose: Returns the count of nodes that require some sort of
         action.

Returns: see above
Cond:    --
*/
UINT PRIVATE CountActionItem(
    PRECLIST prl)
    {
    UINT uc = 0;
    PRECITEM pri;

    for (pri = prl->priFirst; pri; pri = pri->priNext)
        {
        if (RIA_NOTHING != pri->riaction)
            {
            uc++;
            }
        }
    return uc;
    }


/*----------------------------------------------------------
Purpose: Update the twins in the list

Returns:
Cond:    --
*/
HRESULT PRIVATE MassageReclist(
    CBS * pcbs,
    PRECLIST prl,
    LPCTSTR pszInsideDir,
    BOOL bCopyIn,
    HWND hwndOwner)
    {
    HRESULT hres = NOERROR;
    PRECITEM pri;
    BOOL bWarnUser = TRUE;
    PRECNODE prnInside;
    PRECNODE prnOutside;

    // Make sure the direction of the reconciliation coincides
    // with the direction of the user's action.
    for (pri = prl->priFirst; pri; pri = pri->priNext)
        {
        if (RIA_NOTHING != pri->riaction)
            {
            UINT cAvailableNodes = CountAvailableNodes(pri);

            // Is this a wierd multi-edged case (not including
            // Sneakernet)?
            if (2 < cAvailableNodes)
                {
                // Should never get here, but better safe than sorry
                ASSERT(0);
                }
            else
                {
                // No; get the pair of nodes that we just added to the
                // database.
                hres = Sync_GetNodePair(pri, Atom_GetName(pcbs->atomBrf),
                    pszInsideDir, &prnInside, &prnOutside);

                if (SUCCEEDED(hres))
                    {
                    ASSERT(prnInside);
                    ASSERT(prnOutside);

                    if (bCopyIn)
                        {
                        switch (prnOutside->rnstate)
                            {
                        case RNS_UNAVAILABLE:
                        case RNS_DOES_NOT_EXIST:
                        case RNS_DELETED:
                            break;      // leave alone

                        default:
                            // Force the update to be a copy into the briefcase.
                            pri->riaction = RIA_COPY;
                            prnInside->rnaction = RNA_COPY_TO_ME;
                            prnOutside->rnaction = RNA_COPY_FROM_ME;

                            TRACE_MSG(TF_GENERAL, TEXT("Massaging reclist"));
                            break;
                            }
                        }
                    else
                        {
                        switch (prnInside->rnstate)
                            {
                        case RNS_UNAVAILABLE:
                        case RNS_DOES_NOT_EXIST:
                        case RNS_DELETED:
                            break;      // leave alone

                        default:
                            // Force the update to be a copy out of the briefcase.
                            pri->riaction = RIA_COPY;
                            prnInside->rnaction = RNA_COPY_FROM_ME;
                            prnOutside->rnaction = RNA_COPY_TO_ME;

                            TRACE_MSG(TF_GENERAL, TEXT("Massaging reclist"));
                            break;
                            }
                        }
                    }
                else
                    break;      // Error
                }
            }
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Check for more than 2 available nodes in each recitem.
         Remove the associated twin if we find such a case,
         to prevent multiple sync copies.

Returns: S_OK if everything looks ok
         S_FALSE if there were multiple sync copies introduced

Cond:    --
*/
HRESULT PRIVATE VerifyTwins(
    CBS  * pcbs,
    PRECLIST prl,
    LPCTSTR pszTargetDir,
    HWND hwndOwner)
    {
    HRESULT hres = NOERROR;
    PRECITEM pri;
    BOOL bWarnUser = TRUE;
    BOOL bWarnUserFolder = TRUE;
    TCHAR szPath[MAX_PATH];

    // Look thru the reclist and pick out recitems that have more than
    // 2 recnodes that are currently available.

    // Scenarios when this can happen:
    //
    //  1) Foo.txt --> BC
    //     Foo.txt --> BC\Orphan Folder
    //
    //          Expected result: delete BC\Orphan Folder\Foo.txt twin
    //
    //  2) Foo.txt --> BC\Orphan Folder
    //     Orphan Folder --> BC
    //
    //          Expected result: delete BC\Orphan Folder twin
    //
    //  3) Foo.txt --> BC\Orphan Folder
    //     Foo.txt --> BC
    //
    //          Expected result: delete BC\Foo.txt twin
    //

    for (pri = prl->priFirst; pri; pri = pri->priNext)
        {
        UINT cAvailableNodes = CountAvailableNodes(pri);
        PRECNODE prn;

        // Are there more than 2 available nodes?
        if (2 < cAvailableNodes && *pri->pcszName)
            {
            BOOL bLookForFolders = TRUE;

            // FIRST: Look for object twins that are not in folder twins.
            for (prn = pri->prnFirst; prn; prn = prn->prnNext)
                {
                // Is this file here because the file was dragged in?
                if (IsSzEqual(pszTargetDir, prn->pcszFolder))
                    {
                    // Yes; warn the user
                    if (bWarnUser)
                        {
                        MsgBox(hwndOwner,
                               MAKEINTRESOURCE(IDS_ERR_ADDFILE_TOOMANY),
                               MAKEINTRESOURCE(IDS_CAP_ADD),
                               NULL, MB_WARNING, pri->pcszName);

                        if (0 > GetKeyState(VK_SHIFT))
                            {
                            bWarnUser = FALSE;
                            }
                        }

                    // Try to remove the object twin
                    PathCombine(szPath, prn->pcszFolder, pri->pcszName);
                    hres = Sync_Split(pcbs->hbrf, szPath, 1, hwndOwner,
                                        SF_QUIET | SF_NOCONFIRM);

                    TRACE_MSG(TF_GENERAL, TEXT("Deleted object twin for %s"), szPath);
                    ASSERT(FAILED(hres) || S_OK == hres);

                    bLookForFolders = FALSE;
                    break;
                    }
                }


            if (bLookForFolders)
                {
                // SECOND: Look for object twins that exist because of folder
                // twins.
                for (prn = pri->prnFirst; prn; prn = prn->prnNext)
                    {
                    lstrcpy(szPath, prn->pcszFolder);
                    PathRemoveFileSpec(szPath);

                    // Is this file here because it is in a folder that was
                    // dragged in?
                    if (IsSzEqual(pszTargetDir, szPath))
                        {
                        // Yes; warn the user
                        if (bWarnUserFolder && bWarnUser)
                            {
                            MsgBox(hwndOwner,
                                   MAKEINTRESOURCE(IDS_ERR_ADDFOLDER_TOOMANY),
                                   MAKEINTRESOURCE(IDS_CAP_ADD),
                                   NULL, MB_WARNING, PathFindFileName(prn->pcszFolder));

                            // Hack: to prevent showing this messagebox for
                            // every file in this folder, set this flag
                            bWarnUserFolder = FALSE;

                            if (0 > GetKeyState(VK_SHIFT))
                                {
                                bWarnUser = FALSE;
                                }
                            }

                        // Remove the folder twin
                        hres = Sync_Split(pcbs->hbrf, prn->pcszFolder, 1, hwndOwner,
                                            SF_ISFOLDER | SF_QUIET | SF_NOCONFIRM);

                        TRACE_MSG(TF_GENERAL, TEXT("Deleted folder twin for %s"), prn->pcszFolder);

                        ASSERT(FAILED(hres) || !bWarnUserFolder || S_OK == hres);
                        break;
                        }
                    }
                }
            hres = S_FALSE;
            }
        }
    return hres;
    }


#define STATE_VERIFY    0
#define STATE_UPDATE    1
#define STATE_STOP      2

/*----------------------------------------------------------
Purpose: This function updates the new files.  Unlike the general
         update function, this strictly updates file pairs.  All
         other incidental nodes are set to RNA_NOTHING.

         In addition, to be safe, we force the update to always
         perform a copy into the briefcase.

         This function releases the twin handles when it is finished.

Returns: standard result
Cond:    --
*/
HRESULT PRIVATE UpdateNewTwins(
    CBS  * pcbs,
    LPCTSTR pszInsideDir,
    LPCTSTR pszTargetDir,
    BOOL bCopyIn,
    HDPA hdpa,
    HWND hwndOwner)
    {
    HRESULT hres = E_FAIL;
    int iItem;
    int cItems;

    ASSERT(pcbs);
    ASSERT(hdpa);

    cItems = DPA_GetPtrCount(hdpa);
    if (cItems > 0)
        {
        HTWINLIST htl;
        PRECLIST prl;
        TWINRESULT tr;

        tr = Sync_CreateTwinList(pcbs->hbrf, &htl);

        if (TR_SUCCESS != tr)
            {
            hres = HRESULT_FROM_TR(tr);
            }
        else
            {
            HWND hwndProgress;
            UINT nState = STATE_VERIFY;
            DEBUG_CODE( UINT nCount = 0; )

            // State progression is simple:
            //   STATE_VERIFY --> STATE_UPDATE --> STATE_STOP
            // Any questions?

            hwndProgress = UpdBar_Show(hwndOwner, UB_CHECKING, DELAY_UPDBAR);

            for (iItem = 0; iItem < cItems; iItem++)
                {
                HTWIN htwin = DPA_FastGetPtr(hdpa, iItem);

                if (htwin)
                    Sync_AddToTwinList(htl, htwin);
                }

            do
                {
                ASSERT(STATE_VERIFY == nState || STATE_UPDATE == nState);
                ASSERT(2 > nCount++);       // Sanity check for infinite loop

                // Create the reclist
                hres = Sync_CreateRecListEx(htl, UpdBar_GetAbortEvt(hwndProgress), &prl);

                DEBUG_CODE( Sync_DumpRecList(GET_TR(hres), prl, TEXT("Adding new twins")); )

                if (SUCCEEDED(hres))
                    {
                    ASSERT(prl);

                    switch (nState)
                        {
                    case STATE_VERIFY:
                        hres = VerifyTwins(pcbs, prl, pszTargetDir, hwndOwner);
                        if (S_FALSE == hres)
                            nState = STATE_UPDATE;
                        else if (S_OK == hres)
                            goto Update;
                        else
                            nState = STATE_STOP;
                        break;

                    case STATE_UPDATE:
                        // After recreating the reclist, is there anything
                        // that needs updating?
                        if (0 < CountActionItems(prl))
                            {
                            // Yes
Update:
                            UpdBar_SetAvi(hwndProgress, UB_UPDATEAVI);

                            hres = MassageReclist(pcbs, prl, pszInsideDir, bCopyIn, hwndOwner);
                            if (SUCCEEDED(hres))
                                {
                                // Update these files
                                hres = Sync_ReconcileRecList(prl, Atom_GetName(pcbs->atomBrf),
                                    hwndProgress, RF_ONADD);
                                }
                            }

                        nState = STATE_STOP;
                        break;

                    default:
                        ASSERT(0);
                        break;
                        }

                    Sync_DestroyRecList(prl);
                    }

                } while (SUCCEEDED(hres) && STATE_UPDATE == nState);

            Sync_DestroyTwinList(htl);

            UpdBar_Kill(hwndProgress);
            }
        }
    return hres;
    }


#ifdef WINNT
    // BUGBUG - BobDay - WinNT docking state determination code goes here.
#else
// This struct is used to carry stack data across address spaces
// for the configuration manager service call.
typedef struct tagCM_Get_Profile_Param {
    CMAPI    cmApi;
    ULONG    ulIndex;
    PHWPROFILEINFO pHWProfileInfo;
    ULONG    ulFlags;
}   CM_GET_PROFILE_PARAM;

#define HEAP_SHARED     0x04000000      /* put heap in shared memory */
#define CMHEAPSIZE      4096

/*----------------------------------------------------------
Purpose: Service call to get the profile info from the configuration
         manager.

Returns: CONFIGRET
Cond:    --
*/
CONFIGRET _cdecl CM_Get_Hardware_Profile_Info(
    ULONG ulIndex,
    PHWPROFILEINFO pHWProfileInfo,
    ULONG ulFlags)
    {
    CONFIGRET crRetVal = CR_OUT_OF_MEMORY;
    HANDLE hHeap;

    // Allocate our shared heap.  (This is expensive, but this function
    // is rarely called.)
    if (NULL != (hHeap = HeapCreate(HEAP_SHARED, 1, CMHEAPSIZE)))
        {
        DWORD dwRecipients = BSM_VXDS;
        CM_GET_PROFILE_PARAM * pcm;
        DWORD cbVarSize;

        // Allocate CMAPI from our shared heap
        cbVarSize = sizeof(*pHWProfileInfo);

        pcm = (CM_GET_PROFILE_PARAM *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY,
                sizeof(CM_GET_PROFILE_PARAM) + cbVarSize);
        if (NULL != pcm)
            {
            // Package the parameter list
            pcm->cmApi.dwCMAPIRet      = 0;
            pcm->cmApi.dwCMAPIService  = GetVxDServiceOrdinal(_CONFIGMG_Get_Hardware_Profile_Info);
            pcm->cmApi.pCMAPIStack     = (DWORD)(((LPBYTE)pcm) + sizeof(pcm->cmApi));

            pcm->ulIndex = ulIndex;
            pcm->pHWProfileInfo = (PHWPROFILEINFO)(pcm+1);
            pcm->ulFlags = ulFlags;

            // Do the job!
            BroadcastSystemMessage(0, &dwRecipients, WM_DEVICECHANGE, 0x22, (LPARAM)pcm);

            // Return values
            crRetVal = (CONFIGRET)pcm->cmApi.dwCMAPIRet;
            *pHWProfileInfo     = *(pcm->pHWProfileInfo);

            HeapFree(hHeap, 0, pcm);
            }

        HeapDestroy(hHeap);
        }

    return crRetVal;
    }
#endif

/*----------------------------------------------------------
Purpose: Return TRUE if the machine is docked

Returns: See above.
Cond:    --
*/
BOOL PRIVATE IsMachineDocked(void)
    {
#ifdef WINNT
#ifndef BUGBUG_BOBDAY   // On NT we don't know how to determine this yet.
    return TRUE;
#endif
#else
    HWPROFILEINFO hwprofileinfo;
    BOOL bDocked;

    if (CR_SUCCESS == CM_Get_Hardware_Profile_Info((ULONG)-1, &hwprofileinfo, 0))
        {
        bDocked = IsFlagSet(hwprofileinfo.HWPI_dwFlags, CM_HWPI_DOCKED);
        }
    else
        {
        // Error
        bDocked = FALSE;
        }

    DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Machine is %s"), bDocked ? (LPTSTR)TEXT("docked") : (LPTSTR)TEXT("not docked")); )

    return bDocked;
#endif
    }


//---------------------------------------------------------------------------
// IBriefcaseStg member functions
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: IBriefcaseStg::Release

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) BriefStg_Release(
    LPBRIEFCASESTG pstg)
    {
    PBRIEFSTG this = IToClass(BriefStg, bs, pstg);

    DBG_ENTER(TEXT("BriefStg_Release"));

    if (--this->cRef)
        {
        DBG_EXIT_UL(TEXT("BriefStg_Release"), this->cRef);
        return this->cRef;      // Return decremented reference count
        }

    if (this->pcbs)
        {
        // Release this briefcase storage instance
        CloseBriefcaseStorage(this->szFolder);
        }

    if (this->hbrfcaseiter)
        {
        Sync_FindClose(this->hbrfcaseiter);
        }

    GFree(this);

    ENTEREXCLUSIVE()
        {
        DecBriefSemaphore();
        if (IsLastBriefSemaphore())
            {
            CommitIniFile();

            DEBUG_CODE( DumpTables(); )

            TermCacheTables();
            }
        }
    LEAVEEXCLUSIVE()

    DBG_EXIT_UL(TEXT("BriefStg_Release"), 0);

    return 0;
    }


/*----------------------------------------------------------
Purpose: IBriefcaseStg::AddRef

Returns: new reference count
Cond:    --
*/
STDMETHODIMP_(UINT) BriefStg_AddRef(
    LPBRIEFCASESTG pstg)
    {
    PBRIEFSTG this = IToClass(BriefStg, bs, pstg);
    UINT cRef;

    DBG_ENTER(TEXT("BriefStg_AddRef"));

    cRef = ++this->cRef;

    DBG_EXIT_UL(TEXT("BriefStg_AddRef"), cRef);

    return cRef;
    }


/*----------------------------------------------------------
Purpose: IBriefcaseStg::QueryInterface

Returns: standard
Cond:    --
*/
STDMETHODIMP BriefStg_QueryInterface(
    LPBRIEFCASESTG pstg,
    REFIID riid,
    LPVOID * ppvOut)
    {
    PBRIEFSTG this = IToClass(BriefStg, bs, pstg);
    HRESULT hres;

    DBG_ENTER_RIID(TEXT("BriefStg_QueryInterface"), riid);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IBriefcaseStg))
        {
        // We use the bs field as our IUnknown as well
        *ppvOut = &this->bs;
        this->cRef++;
        hres = NOERROR;
        }
    else
        {
        *ppvOut = NULL;
        hres = ResultFromScode(E_NOINTERFACE);
        }

    DBG_EXIT_HRES(TEXT("BriefStg_QueryInterface"), hres);
    return hres;
    }


/*----------------------------------------------------------
Purpose: IBriefcaseStg::Initialize

         Called to initialize a briefcase storage instance.
         The pszFolder indicates the folder we are binding to,
         which is in the briefcase storage (somewhere).

Returns: standard
Cond:    --
*/
STDMETHODIMP BriefStg_Initialize(
    LPBRIEFCASESTG pstg,
    LPCTSTR pszPath,
    HWND hwndOwner)
    {
    PBRIEFSTG this = IToClass(BriefStg, bs, pstg);
    HRESULT hres = ResultFromScode(E_FAIL);

    DBG_ENTER_SZ(TEXT("BriefStg_Initialize"), pszPath);

    ASSERT(pszPath);

    // Only initialize once per interface instance
    //
    if (pszPath && NULL == this->pcbs)
        {
        BOOL bCancel = FALSE;

        RETRY_BEGIN(FALSE)
            {
            // Unavailable disk?
            if (!PathExists(pszPath))
                {
                // Yes; ask user to retry/cancel
                int id = MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_ERR_OPEN_UNAVAIL_VOL),
                    MAKEINTRESOURCE(IDS_CAP_OPEN), NULL, MB_RETRYCANCEL | MB_ICONWARNING);

                if (IDRETRY == id)
                    RETRY_SET();    // Try again
                else
                    bCancel = TRUE;
                }
            }
        RETRY_END()

        if (!bCancel)
            {
            BrfPathCanonicalize(pszPath, this->szFolder);

            if (PathExists(this->szFolder) && !PathIsDirectory(this->szFolder))
                {
                // (Store this as a path to a folder)
                PathRemoveFileSpec(this->szFolder);
                }

            // Open the briefcase storage for this folder
            //
            hres = OpenBriefcaseStorage(this->szFolder, &this->pcbs, hwndOwner);

            if (SUCCEEDED(hres))
                {
                // Is this folder a sync folder?
                if (HasFolderSyncCopy(this->pcbs->hbrf, this->szFolder))
                    {
                    // Yes
                    SetFlag(this->dwFlags, BSTG_SYNCFOLDER);
                    }
                else
                    {
                    // No (or error, in which case we default to no)
                    ClearFlag(this->dwFlags, BSTG_SYNCFOLDER);
                    }
                }

            // Run the wizard?
            if (SUCCEEDED(hres) && IsFlagSet(this->pcbs->uFlags, CBSF_RUNWIZARD))
                {
                // Yes
                RunDLLThread(hwndOwner, TEXT("SYNCUI.DLL,Briefcase_Intro"), SW_SHOW);
                ClearFlag(this->pcbs->uFlags, CBSF_RUNWIZARD);
                }
            }
        }

    hres = MapToOfficialHresult(hres);
    DBG_EXIT_HRES(TEXT("BriefStg_Initialize"), hres);
    return hres;
    }


/*----------------------------------------------------------
Purpose: Add an object or objects to the briefcase storage.
         This function does the real work for BriefStg_AddObject.

Returns: standard result
         NOERROR if the object(s) were added
         S_FALSE if the object(s) should be handled by the caller

Cond:    --
*/
HRESULT PRIVATE BriefStg_AddObjectPrivate(
    LPBRIEFCASESTG pstg,
    LPDATAOBJECT pdtobj,
    LPCTSTR pszFolderEx,         // optional (may be NULL)
    UINT uFlags,                // One of AOF_*
    HWND hwndOwner)
    {
    PBRIEFSTG this = IToClass(BriefStg, bs, pstg);
    HRESULT hres;
    LPTSTR pszList;
    LPTSTR psz;
    UINT i;
    UINT cFiles;
    TCHAR szCanon[MAX_PATH];
    HDPA hdpa;
    LPCTSTR pszTarget;
    BOOL bMultiFiles;
#pragma data_seg(DATASEG_READONLY)
    static SETbl const c_rgseAdd[] = {
            { E_OUTOFMEMORY,        IDS_OOM_ADD,    MB_ERROR },
            { E_TR_OUT_OF_MEMORY,   IDS_OOM_ADD,    MB_ERROR },
            };
#pragma data_seg()

    ASSERT(pdtobj);

    // Verify that the folder of this briefcase storage is actually inside
    // a briefcase.  (szCanon is used as a dummy here.)
    ASSERT( !PathExists(this->szFolder) || PL_FALSE != PathGetLocality(this->szFolder, szCanon) );

    // Get list of files to add
    hres = DataObj_QueryFileList(pdtobj, &pszList, &cFiles);
    if (SUCCEEDED(hres))
        {
        // Grab the mutex to delay any further calculation in any
        // Briefcase views' secondary threads until we're done
        // processing here.
        Delay_Own();

        // Does the caller want to create sync copies of objects that are
        // already in the briefcase to some other folder?  (Sneakernet)
        if (NULL != pszFolderEx)
            {
            // Yes
            pszTarget = pszFolderEx;
            }
        else
            {
            // No
            pszTarget = this->szFolder;

            // Are the entities already in this briefcase?
            //
            // Based on the success return value of DataObj_QueryFileList,
            // we can tell if the entities are already within a briefcase.
            // Because of the nature of the shell, we assume the file
            // list contains entities which all exist in the same folder,
            // so we consider it an "all or nothing" sort of indicator.
            // If the entities are indeed in a briefcase, we compare the
            // roots of the source and destination briefcases, and BLOCK
            // the addition if they are the same.
            //
            if (S_OK == hres)
                {
                // They are in *a* briefcase.  Which one?
                DataObj_QueryBriefPath(pdtobj, szCanon);
                if (IsSzEqual(szCanon, Atom_GetName(this->pcbs->atomBrf)))
                    {
                    // This same one!  Don't do anything.
                    // BUGBUG: display message box
                    hres = ResultFromScode(E_FAIL);
                    goto Error1;
                    }
                }
            }

        bMultiFiles = (1 < cFiles);

        // Create the temporary DPA list
        if (NULL == (hdpa = DPA_Create(cFiles)))
            {
            hres = ResultFromScode(E_OUTOFMEMORY);
            }
        else
            {
            UINT uConfirmFlags = 0;

            // Add all the objects to the briefcase storage
            for (i = 0, psz = pszList; i < cFiles; i++)
                {
                // Get file/folder name that was dropped
                BrfPathCanonicalize(psz, szCanon);

                if (PathIsDirectory(szCanon))
                    {
                    hres = CreateTwinOfFolder(this->pcbs, szCanon, pszTarget,
                                              hdpa, uFlags, &uConfirmFlags,
                                              hwndOwner, bMultiFiles);
                    }
                else
                    {
                    hres = CreateTwinOfFile(this->pcbs, szCanon, pszTarget,
                                            hdpa, uFlags, &uConfirmFlags,
                                            hwndOwner, bMultiFiles);
                    }

                if (FAILED(hres))
                    {
                    // An error occurred while attempting to add a twin
                    break;
                    }

                DataObj_NextFile(psz);      // Set psz to next file in list
                }

            if (FAILED(hres))
                {
                // Delete the twins that were added.
                DeleteNewTwins(this->pcbs, hdpa);
                }
            else
                {
                // Update these new twins
                hres = UpdateNewTwins(this->pcbs, this->szFolder, pszTarget, (NULL == pszFolderEx), hdpa, hwndOwner);
                }

            ReleaseNewTwins(hdpa);
            DPA_Destroy(hdpa);
            }
Error1:
        DataObj_FreeList(pszList);

        Delay_Release();
        }

    if (FAILED(hres))
        {
        SEMsgBox(hwndOwner, IDS_CAP_ADD, hres, c_rgseAdd, ARRAYSIZE(c_rgseAdd));
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: IBriefcaseStg::AddObject

         Add an object to the briefcase storage.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP BriefStg_AddObject(
    LPBRIEFCASESTG pstg,
    LPDATAOBJECT pdtobj,
    LPCTSTR pszFolderEx,        // optional
    UINT uFlags,
    HWND hwndOwner)
    {
    PBRIEFSTG this = IToClass(BriefStg, bs, pstg);
    HRESULT hres = NOERROR;
    LPCTSTR pszFolder;
    UINT ids;
    DEBUG_CODE( TCHAR szDbg[MAX_PATH]; )

    DBG_ENTER_DTOBJ(TEXT("BriefStg_AddObject"), pdtobj, szDbg);

    ASSERT(pdtobj);
    ASSERT(this->pcbs);

    // Is this sneakernet?
    // Is this folder a sync folder?
    if (pszFolderEx)
        {
        // Yes; is the source a sync folder already?
        if (HasFolderSyncCopy(this->pcbs->hbrf, pszFolderEx))
            {
            // Yes; don't allow other sync copies into (or out of) it
            ids = IDS_ERR_ADD_SYNCFOLDER;
            pszFolder = PathFindFileName(pszFolderEx);
            hres = E_FAIL;
            }
        // Is the source folder a sync folder already?
        else if (IsFlagSet(this->dwFlags, BSTG_SYNCFOLDER))
            {
            // Yes; don't allow other sync copies into (or out of) it
            ids = IDS_ERR_ADD_SYNCFOLDER_SRC;
            pszFolder = PathFindFileName(this->szFolder);
            hres = E_FAIL;
            }
        }
    else if (IsFlagSet(this->dwFlags, BSTG_SYNCFOLDER))
        {
        // Yes; don't allow other sync copies into (or out of) it
        ids = IDS_ERR_ADD_SYNCFOLDER;
        pszFolder = PathFindFileName(this->szFolder);
        hres = E_FAIL;
        }

    if (SUCCEEDED(hres))
        {
        hres = BriefStg_AddObjectPrivate(pstg, pdtobj, pszFolderEx, uFlags, hwndOwner);
        }
    else
        {
        MsgBox(hwndOwner,
                MAKEINTRESOURCE(ids),
                MAKEINTRESOURCE(IDS_CAP_ADD),
                NULL,
                MB_WARNING,
                pszFolder);
        }

    DEBUG_CODE( DumpTables(); )
    hres = MapToOfficialHresult(hres);
    DBG_EXIT_HRES(TEXT("BriefStg_AddObject"), hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: Removes an object or objects from the briefcase storage.

Returns: standard hresult
Cond:    --
*/
HRESULT PRIVATE ReleaseObject(
    CBS * pcbs,
    LPDATAOBJECT pdtobj,
    HWND hwndOwner)
    {
    HRESULT hres;
    LPTSTR pszList;
    UINT cFiles;

    ASSERT(pdtobj);

    hres = DataObj_QueryFileList(pdtobj, &pszList, &cFiles);
    if (SUCCEEDED(hres))
        {
        RETRY_BEGIN(FALSE)
            {
            hres = Sync_Split(pcbs->hbrf, pszList, cFiles, hwndOwner, 0);

            // Unavailable disk?
            if (E_TR_UNAVAILABLE_VOLUME == hres)
                {
                // Yes; ask user to retry/cancel
                int id = MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_ERR_UNAVAIL_VOL),
                    MAKEINTRESOURCE(IDS_CAP_Split), NULL, MB_RETRYCANCEL | MB_ICONWARNING);

                if (IDRETRY == id)
                    RETRY_SET();    // Try again
                }
            }
        RETRY_END()

        DataObj_FreeList(pszList);
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: IBriefcaseStg::ReleaseObject

         Release an object from the briefcase storage.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP BriefStg_ReleaseObject(
    LPBRIEFCASESTG pstg,
    LPDATAOBJECT pdtobj,
    HWND hwndOwner)
    {
    PBRIEFSTG this = IToClass(BriefStg, bs, pstg);
    HRESULT hres;
    DEBUG_CODE( TCHAR szDbg[MAX_PATH]; )

    DBG_ENTER_DTOBJ(TEXT("BriefStg_ReleaseObject"), pdtobj, szDbg);

    ASSERT(pdtobj);
    ASSERT(this->pcbs);

    hres = ReleaseObject(this->pcbs, pdtobj, hwndOwner);

    DEBUG_CODE( DumpTables(); )
    hres = MapToOfficialHresult(hres);
    DBG_EXIT_HRES(TEXT("BriefStg_ReleaseObject"), hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: IBriefcaseStg::UpdateObject

         Update an object in the briefcase storage.

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP BriefStg_UpdateObject(
    LPBRIEFCASESTG pstg,
    LPDATAOBJECT pdtobj,
    HWND hwndOwner)
    {
    PBRIEFSTG this = IToClass(BriefStg, bs, pstg);
    HRESULT hres;
    TCHAR szPath[MAX_PATH];
    DEBUG_CODE( TCHAR szDbg[MAX_PATH]; )

    DBG_ENTER_DTOBJ(TEXT("BriefStg_UpdateObject"), pdtobj, szDbg);

    ASSERT(pdtobj);
    ASSERT(this->pcbs);

    // Determine whether this is an Update Selection or Update All.
    hres = DataObj_QueryPath(pdtobj, szPath);
    if (SUCCEEDED(hres))
        {
        // Is this a briefcase root?
        if (PathIsBriefcase(szPath))
            {
            // Yes; do an Update All
            hres = Upd_DoModal(hwndOwner, this->pcbs, NULL, 0, UF_ALL);
            }
        else
            {
            // No; do an Update Selection
            LPTSTR pszList;
            UINT cFiles;
            hres = DataObj_QueryFileList(pdtobj, &pszList, &cFiles);
            if (SUCCEEDED(hres))
                {
                hres = Upd_DoModal(hwndOwner, this->pcbs, pszList, cFiles, UF_SELECTION);
                DataObj_FreeList(pszList);
                }
            }
        }

    DEBUG_CODE( DumpTables(); )
    hres = MapToOfficialHresult(hres);
    DBG_EXIT_HRES(TEXT("BriefStg_UpdateObject"), hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: Update a briefcase based on events

Returns: standard hresult
Cond:    --
*/
HRESULT PRIVATE BriefStg_UpdateOnEvent(
    LPBRIEFCASESTG pstg,
    UINT uEvent,
    HWND hwndOwner)
    {
    PBRIEFSTG this = IToClass(BriefStg, bs, pstg);
    HRESULT hres = NOERROR;

    DBG_ENTER(TEXT("BriefStg_UpdateOnEvent"));

    switch (uEvent)
        {
    case UOE_CONFIGCHANGED:
    case UOE_QUERYCHANGECONFIG:
        // Is the machine docked?
        if (IsMachineDocked())
            {
            // Yes; does the user want to update?
            TCHAR sz[MAX_PATH];
            int ids = (UOE_CONFIGCHANGED == uEvent) ? IDS_MSG_UpdateOnDock : IDS_MSG_UpdateBeforeUndock;
            LPCTSTR pszBrf = Atom_GetName(this->pcbs->atomBrf);
            int id = MsgBox(hwndOwner,
                                MAKEINTRESOURCE(ids),
                                MAKEINTRESOURCE(IDS_CAP_UPDATE),
                                LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_UPDATE_DOCK)),
                                MB_QUESTION,
                                PathGetDisplayName(pszBrf, sz));

            if (IDYES == id)
                {
                // Yes; do an Update All
                hres = Upd_DoModal(hwndOwner, this->pcbs, NULL, 0, UF_ALL);
                }
            }
        break;

    default:
        hres = ResultFromScode(E_INVALIDARG);
        break;
        }

    DEBUG_CODE( DumpTables(); )
    hres = MapToOfficialHresult(hres);
    DBG_EXIT_HRES(TEXT("BriefStg_UpdateOnEvent"), hres);

    return hres;
    }

/*----------------------------------------------------------
Purpose: IBriefcaseStg::Notify

         Marks the path dirty in the briefcase storage cache.
         (The path may not exist in the cache, in which case this
         function does nothing.)

Returns: S_OK to force a refresh
         S_FALSE to not force a refresh

Cond:    --
*/
STDMETHODIMP BriefStg_Notify(
    LPBRIEFCASESTG pstg,
    LPCTSTR pszPath,         // may be NULL
    LONG lEvent,            // one of NOE_ flags
    UINT * puFlags,         // returned NF_ flags
    HWND hwndOwner)
    {
    PBRIEFSTG this = IToClass(BriefStg, bs, pstg);
    HRESULT hres = ResultFromScode(E_OUTOFMEMORY);
    TCHAR szCanon[MAX_PATH];
    int atom;

    DBG_ENTER_SZ(TEXT("BriefStg_Notify"), pszPath);

    ASSERT(this->pcbs);
    ASSERT(puFlags);

    DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Received event %lx for %s"), lEvent, Dbg_SafeStr(pszPath)); )

    *puFlags = 0;

    // Dirty the entire cache?
    if (NOE_DIRTYALL == lEvent)
        {
        // Yes
        TRACE_MSG(TF_GENERAL, TEXT("Marking everything"));

        CRL_DirtyAll(this->pcbs->atomBrf);
        Sync_ClearBriefcaseCache(this->pcbs->hbrf);
        hres = NOERROR;
        }
    else if (pszPath && 0 < lEvent)
        {
        // No
        BrfPathCanonicalize(pszPath, szCanon);
        atom = Atom_Add(szCanon);
        if (ATOM_ERR != atom)
            {
            int atomCab = Atom_Add(this->szFolder);
            if (ATOM_ERR != atomCab)
                {
                // There are two actions we must determine: what gets marked dirty?
                // and does this specific window get forcibly refreshed?
                BOOL bRefresh;
                BOOL bMarked;

                bMarked = CRL_Dirty(atom, atomCab, lEvent, &bRefresh);
                hres = NOERROR;

                if (bMarked)
                    {
                    SetFlag(*puFlags, NF_ITEMMARKED);
                    }
                if (bRefresh)
                    {
#if 0
                    SetFlag(*puFlags, NF_REDRAWWINDOW);
#endif
                    }

#ifdef DEBUG
                if (bMarked && bRefresh)
                    {
                    TRACE_MSG(TF_GENERAL, TEXT("Marked and forcing refresh of window on %s"), (LPTSTR)this->szFolder);
                    }
                else if (bMarked)
                    {
                    TRACE_MSG(TF_GENERAL, TEXT("Marked"));
                    }
#endif

                Atom_Delete(atomCab);
                }
            Atom_Delete(atom);
            }
        }

    DBG_EXIT_HRES(TEXT("BriefStg_Notify"), hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: Gets special info (status and origin) of a path.
Returns: --
Cond:    --
*/
HRESULT PRIVATE BriefStg_GetSpecialInfoOf(
    PBRIEFSTG this,
    LPCTSTR pszName,
    UINT uFlag,
    LPTSTR pszBuf,
    int cchBuf)
    {
    HRESULT hres = E_OUTOFMEMORY;
    TCHAR szPath[MAX_PATH];
    TCHAR szCanon[MAX_PATH];
    int atom;

    ASSERT(this);
    ASSERT(pszName);
    ASSERT(pszBuf);
    ASSERT(this->pcbs);

    *pszBuf = TEXT('\0');

    // Would the path be too long if combined?
    if (PathsTooLong(this->szFolder, pszName))
        {
        // Yes
        hres = E_FAIL;
        }
    else
        {
        PathCombine(szPath, this->szFolder, pszName);
        BrfPathCanonicalize(szPath, szCanon);
        atom = Atom_Add(szCanon);
        if (ATOM_ERR != atom)
            {
            CRL * pcrl;

            // The first CRL_Get call will get the reclist from the cache
            // or get a fresh reclist if the dirty bit is set.  If the cache
            // item doesn't exist, add it.  We add orphans to the cache too
            // but they have no reclist.

            // Does the cached item already exist?
            hres = CRL_Get(atom, &pcrl);
            if (FAILED(hres))
                {
                // No; add it
                hres = CRL_Add(this->pcbs, atom);
                if (SUCCEEDED(hres))
                    {
                    // Do another 'get' to offset the CRL_Delete at the end of
                    // this function.  This will leave this new reclist in the
                    // cache upon exit.  (We don't want to create a new reclist
                    // everytime this functin is called.)  It will all get
                    // cleaned up when the CBS is freed.
                    //
                    hres = CRL_Get(atom, &pcrl);
                    }
                }

            ASSERT(FAILED(hres) || pcrl);

            // Do we have a cache reclist entry to work with?
            if (pcrl)
                {
                // Yes
                if (GEI_ORIGIN == uFlag)
                    {
                    lstrcpyn(pszBuf, Atom_GetName(pcrl->atomOutside), cchBuf);
                    PathRemoveFileSpec(pszBuf);
                    }
                else
                    {
                    ASSERT(GEI_STATUS == uFlag);
                    SzFromIDS(pcrl->idsStatus, pszBuf, cchBuf);
                    }

                CRL_Delete(atom);   // Decrement count
                }
            Atom_Delete(atom);
            }
        }
    return hres;
    }


/*----------------------------------------------------------
Purpose: IBriefcaseStg::GetExtraInfo

Returns: standard hresult
Cond:    --
*/
STDMETHODIMP BriefStg_GetExtraInfo(
    LPBRIEFCASESTG pstg,
    LPCTSTR pszName,
    UINT uInfo,
    WPARAM wParam,
    LPARAM lParam)
    {
    PBRIEFSTG this = IToClass(BriefStg, bs, pstg);
    HRESULT hres;

    DBG_ENTER_SZ(TEXT("BriefStg_GetExtraInfo"), pszName);

    ASSERT(this->pcbs);

    switch (uInfo)
        {
    case GEI_ORIGIN:
    case GEI_STATUS: {
        LPTSTR pszBuf = (LPTSTR)lParam;
        int cchBuf = (int)wParam;

        ASSERT(pszName);
        ASSERT(pszBuf);

        hres = BriefStg_GetSpecialInfoOf(this, pszName, uInfo, pszBuf, cchBuf);
        }
        break;

    case GEI_DELAYHANDLE: {
        HANDLE * phMutex = (HANDLE *)lParam;

        ASSERT(phMutex);

        *phMutex = g_hMutexDelay;
        hres = NOERROR;
        }
        break;

    case GEI_ROOT: {
        LPTSTR pszBuf = (LPTSTR)lParam;
        int cchBuf = (int)wParam;

        ASSERT(pszBuf);

        lstrcpyn(pszBuf, Atom_GetName(this->pcbs->atomBrf), cchBuf);

#ifdef DEBUG

        if (IsFlagSet(g_uDumpFlags, DF_PATHS))
            {
            TRACE_MSG(TF_ALWAYS, TEXT("Root is \"%s\""), pszBuf);
            }

#endif
        hres = NOERROR;
        }
        break;

    case GEI_DATABASENAME: {
        LPTSTR pszBuf = (LPTSTR)lParam;
        int cchBuf = (int)wParam;
        LPCTSTR pszDBName;

        ASSERT(pszBuf);

        if (IsFlagSet(this->pcbs->uFlags, CBSF_LFNDRIVE))
            pszDBName = g_szDBName;
        else
            pszDBName = g_szDBNameShort;

        lstrcpyn(pszBuf, pszDBName, cchBuf);

        hres = NOERROR;
        }
        break;

    default:
        hres = E_INVALIDARG;
        break;
        }

    DBG_EXIT_HRES(TEXT("BriefStg_GetExtraInfo"), hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: IBriefcaseStg::FindFirst

         Returns the location of the root of the first briefcase storage
         in the system.

Returns: S_OK if a briefcase was found
         S_FALSE to end enumeration
Cond:    --
*/
STDMETHODIMP BriefStg_FindFirst(
    LPBRIEFCASESTG pstg,
    LPTSTR pszName,
    int cchMaxName)
    {
    PBRIEFSTG this = IToClass(BriefStg, bs, pstg);
    HRESULT hres;
    TWINRESULT tr;
    BRFCASEINFO bcinfo;

    DBG_ENTER(TEXT("BriefStg_FindFirst"));

    ASSERT(pszName);

    bcinfo.ulSize = sizeof(bcinfo);
    tr = Sync_FindFirst(&this->hbrfcaseiter, &bcinfo);
    switch (tr)
        {
    case TR_OUT_OF_MEMORY:
        hres = ResultFromScode(E_OUTOFMEMORY);
        break;

    case TR_SUCCESS:
        hres = ResultFromScode(S_OK);
        lstrcpyn(pszName, bcinfo.rgchDatabasePath, cchMaxName);
        break;

    case TR_NO_MORE:
        hres = ResultFromScode(S_FALSE);
        break;

    default:
        hres = ResultFromScode(E_FAIL);
        break;
        }

    DBG_EXIT_HRES(TEXT("BriefStg_FindFirst"), hres);

    return hres;
    }


/*----------------------------------------------------------
Purpose: IBriefcaseStg::FindNext

         Returns the location of the root of the next briefcase storage
         in the system.

Returns: S_OK if a briefcase was found
         S_FALSE to end enumeration
Cond:    --
*/
STDMETHODIMP BriefStg_FindNext(
    LPBRIEFCASESTG pstg,
    LPTSTR pszName,
    int cchMaxName)
    {
    PBRIEFSTG this = IToClass(BriefStg, bs, pstg);
    HRESULT hres;
    TWINRESULT tr;
    BRFCASEINFO bcinfo;

    DBG_ENTER(TEXT("BriefStg_FindNext"));

    ASSERT(pszName);

    bcinfo.ulSize = sizeof(bcinfo);
    tr = Sync_FindNext(this->hbrfcaseiter, &bcinfo);
    switch (tr)
        {
    case TR_OUT_OF_MEMORY:
        hres = ResultFromScode(E_OUTOFMEMORY);
        break;

    case TR_SUCCESS:
        hres = ResultFromScode(S_OK);
        lstrcpyn(pszName, bcinfo.rgchDatabasePath, cchMaxName);
        break;

    case TR_NO_MORE:
        hres = ResultFromScode(S_FALSE);
        break;

    default:
        hres = ResultFromScode(E_FAIL);
        break;
        }

    DBG_EXIT_HRES(TEXT("BriefStg_FindNext"), hres);

    return hres;
    }


//---------------------------------------------------------------------------
// BriefStg class : Vtables
//---------------------------------------------------------------------------

#pragma data_seg(DATASEG_READONLY)

IBriefcaseStgVtbl c_BriefStg_BSVtbl =
    {
    BriefStg_QueryInterface,
    BriefStg_AddRef,
    BriefStg_Release,
    BriefStg_Initialize,
    BriefStg_AddObject,
    BriefStg_ReleaseObject,
    BriefStg_UpdateObject,
    BriefStg_UpdateOnEvent,
    BriefStg_GetExtraInfo,
    BriefStg_Notify,
    BriefStg_FindFirst,
    BriefStg_FindNext,
    };

#pragma data_seg()


/*----------------------------------------------------------
Purpose: This function is called back from within
         IClassFactory::CreateInstance() of the default class
         factory object, which is created by SHCreateClassObject.

Returns: standard
Cond:    --
*/
HRESULT CALLBACK BriefStg_CreateInstance(
    LPUNKNOWN punkOuter,        // Should be NULL for us
    REFIID riid,
    LPVOID * ppvOut)
    {
    HRESULT hres = E_FAIL;
    PBRIEFSTG this;

    DBG_ENTER_RIID(TEXT("BriefStg_CreateInstance"), riid);

    // Briefcase storage does not support aggregation.
    //
    if (punkOuter)
        {
        hres = CLASS_E_NOAGGREGATION;
        *ppvOut = NULL;
        goto Leave;
        }

    this = GAlloc(sizeof(*this));
    if (!this)
        {
        hres = E_OUTOFMEMORY;
        *ppvOut = NULL;
        goto Leave;
        }
    this->bs.lpVtbl = &c_BriefStg_BSVtbl;
    this->cRef = 1;
    this->pcbs = NULL;
    this->dwFlags = 0;

    // Load the engine if it hasn't already been loaded
    // (this only returns FALSE if something went wrong)
    if (Sync_QueryVTable())
        {
        ENTEREXCLUSIVE()
            {
            // The decrement is in BriefStg_Release()
            IncBriefSemaphore();
            if (IsFirstBriefSemaphore())
                {
                ProcessIniFile();   // Load settings first

                // Initialize cache
                if (InitCacheTables())
                    hres = NOERROR;
                else
                    hres = E_OUTOFMEMORY;
                }
            else
                {
                hres = NOERROR;
                }
            }
        LEAVEEXCLUSIVE()
        }

    if (SUCCEEDED(hres))
        {
        // Note that the Release member will free the object, if
        // QueryInterface failed.
        //
        hres = this->bs.lpVtbl->QueryInterface(&this->bs, riid, ppvOut);
        this->bs.lpVtbl->Release(&this->bs);
        }
    else
        {
        *ppvOut = NULL;
        }

Leave:
    DBG_EXIT_HRES(TEXT("BriefStg_CreateInstance"), hres);

    return hres;        // S_OK or E_NOINTERFACE
    }
