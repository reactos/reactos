//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: cbs.c
//
//  This files contains code for the cached briefcase structs
//
// History:
//  09-02-93 ScottH     Created
//  01-31-94 ScottH     Moved from cache.c
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////  INCLUDES

#include "brfprv.h"         // common headers
#include "res.h"


CACHE g_cacheCBS = {0, 0, 0};        // Briefcase structure cache

#define CBS_EnterCS()    EnterCriticalSection(&g_cacheCBS.cs)
#define CBS_LeaveCS()    LeaveCriticalSection(&g_cacheCBS.cs)

#pragma data_seg(DATASEG_READONLY)  

SETbl const c_rgseOpenBriefcase[] = {
        { E_TR_OUT_OF_MEMORY,         IDS_OOM_OPENBRIEFCASE,      MB_ERROR },
        { E_OUTOFMEMORY,              IDS_OOM_OPENBRIEFCASE,      MB_ERROR },
        { E_TR_BRIEFCASE_LOCKED,      IDS_ERR_BRIEFCASE_LOCKED,   MB_WARNING },
        { E_TR_BRIEFCASE_OPEN_FAILED, IDS_ERR_OPEN_ACCESS_DENIED, MB_WARNING },
        { E_TR_NEWER_BRIEFCASE,       IDS_ERR_NEWER_BRIEFCASE,    MB_INFO },
        { E_TR_SUBTREE_CYCLE_FOUND,   IDS_ERR_OPEN_SUBTREECYCLE,  MB_WARNING },
        };

#pragma data_seg()


#ifdef DEBUG
void PRIVATE CBS_DumpEntry(
    CBS  * pcbs)
    {
    ASSERT(pcbs);

    TRACE_MSG(TF_ALWAYS, TEXT("CBS:  Atom %d: %s"), pcbs->atomBrf, Atom_GetName(pcbs->atomBrf));
    TRACE_MSG(TF_ALWAYS, TEXT("               Ref [%u]  Hbrf = %lx  "), 
        Cache_GetRefCount(&g_cacheCBS, pcbs->atomBrf),
        pcbs->hbrf);
    }


void PUBLIC CBS_DumpAll()
    {
    CBS  * pcbs;
    int atom;
    BOOL bDump;

    ENTEREXCLUSIVE()
        {
        bDump = IsFlagSet(g_uDumpFlags, DF_CBS);
        }
    LEAVEEXCLUSIVE()

    if (!bDump)
        return ;

    atom = Cache_FindFirstKey(&g_cacheCBS);
    while (atom != ATOM_ERR)
        {
        pcbs = Cache_GetPtr(&g_cacheCBS, atom);
        ASSERT(pcbs);
        if (pcbs)
            {
            CBS_DumpEntry(pcbs);
            CBS_Delete(atom, NULL);         // Decrement count
            }

        atom = Cache_FindNextKey(&g_cacheCBS, atom);
        }
    }
#endif


/*----------------------------------------------------------
Purpose: Save and close the briefcase.
Returns: --
Cond:    
         This function is serialized by the caller (Cache_Term or
         Cache_DeleteItem).
*/
void CALLBACK CBS_Free(
    LPVOID lpv,
    HWND hwndOwner)
    {
    HBRFCASE hbrf;
    CBS  * pcbs = (CBS  *)lpv;
    CRL  * pcrl;
    int atomPath = pcbs->atomBrf;
    int atom;
    TWINRESULT tr1;
    TWINRESULT tr2;
    DECLAREHOURGLASS;

    hbrf = pcbs->hbrf;

    // Save the briefcase with the same name it was opened
    //
    DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Saving and closing Briefcase %s (0x%lx)"), 
        Atom_GetName(atomPath), hbrf); )

    // Search thru the CRL cache for entries 
    //  sharing the same partial path as this briefcase
    //  and nuke them.
    //
    atom = Cache_FindFirstKey(&g_cacheCRL);
    while (atom != ATOM_ERR)
        {
        pcrl = Cache_GetPtr(&g_cacheCRL, atom);
        ASSERT(pcrl);

        if (pcrl)
            {
            if (hbrf == pcrl->hbrf)
                {
                // This atomKey belongs to this briefcase.  Nuke it.
                //
                DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  Nuking CRL %d"), atom); )
                CRL_Nuke(atom);
                }
#ifdef DEBUG
            else
                DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  NOT Nuking CRL %d"), atom); )
#endif

            Cache_DeleteItem(&g_cacheCRL, atom, FALSE, hwndOwner, CRL_Free);     // Decrement count
            }

        atom = Cache_FindNextKey(&g_cacheCRL, atom);
        }

    // Save the briefcase.  We normally (re)specify the database
    //  pathname to handle the rename case.  However, if the
    //  move bit has been set, then we use the NULL parameter
    //  (save under current name) because we will depend on the
    //  shell to move the database.
    //
    ASSERT(Sync_IsEngineLoaded());

    // First check if the disk is available.  If it isn't, Windows will
    // blue-screen because we cannot close the database file.  So before
    // that happens, bring up a friendlier retry messagebox.
    RETRY_BEGIN(FALSE)
        {
        // Is disk unavailable?
        if ( !PathExists(Atom_GetName(atomPath)) )
            {
            // Yes; ask user to retry/cancel
            int id = MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_ERR_CLOSE_UNAVAIL_VOL),
                MAKEINTRESOURCE(IDS_CAP_SAVE), NULL, MB_RETRYCANCEL | MB_ICONWARNING);
            if (IDRETRY == id)
                RETRY_SET();
            }
        }
    RETRY_END()

    SetHourglass();
    tr1 = Sync_SaveBriefcase(pcbs->hbrf);
    tr2 = Sync_CloseBriefcase(pcbs->hbrf);
    if (TR_SUCCESS != tr1 || TR_SUCCESS != tr2)
        {
        DWORD dwError = GetLastError();

        switch (dwError)
            {
        case ERROR_ACCESS_DENIED:
            MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_ERR_SAVE_UNAVAIL_VOL), 
                MAKEINTRESOURCE(IDS_CAP_SAVE), NULL, MB_ERROR);
            break;
            
        default:
            if (TR_BRIEFCASE_WRITE_FAILED == tr1 || TR_BRIEFCASE_WRITE_FAILED == tr2)
                {
                LPTSTR psz;

                static UINT rgids[2] = { IDS_ERR_1_FullDiskSave, IDS_ERR_2_FullDiskSave };

                if (FmtString(&psz, IDS_ERR_F_FullDiskSave, rgids, ARRAYSIZE(rgids)))
                    {
                    MsgBox(hwndOwner, psz, MAKEINTRESOURCE(IDS_CAP_SAVE), NULL, MB_ERROR);
                    GFree(psz);
                    }
                }
            break;
            }
        }
    ResetHourglass();

    AbortEvt_Free(pcbs->pabortevt);

    SharedFree(&pcbs);
    }


/*----------------------------------------------------------
Purpose: Actually opens the briefcase and adds the briefcase
         handle to the given CBS struct.

Returns: standard hresult
Cond:    --
*/
HRESULT PRIVATE OpenTheBriefcase(
    LPCTSTR pszDatPath,
    int atomPath,
    CBS * pcbs,
    HWND hwndOwner)
    {
    HRESULT hres;
    TWINRESULT tr;
    BOOL bRet = FALSE;
    DWORD dwFlags = OB_FL_OPEN_DATABASE | OB_FL_TRANSLATE_DB_FOLDER | OB_FL_ALLOW_UI;
    int nDrive;
    int nDriveType;

    // Determine if we want to record the existence of this briefcase.
    // We don't care about briefcases on remote or floppy drives.
    nDrive = PathGetDriveNumber(pszDatPath);

    // Record this briefcase?
    nDriveType = DriveType(nDrive);
    if (DRIVE_CDROM != nDriveType && DRIVE_REMOVABLE != nDriveType && 
        DRIVE_RAMDRIVE != nDriveType &&
        !PathIsUNC(pszDatPath) && !IsNetDrive(nDrive))
        {
        // Yes
        SetFlag(dwFlags, OB_FL_LIST_DATABASE);

        TRACE_MSG(TF_GENERAL, TEXT("Remembering briefcase %s"), pszDatPath);
        }

    RETRY_BEGIN(FALSE)
        {
        tr = Sync_OpenBriefcase(pszDatPath, dwFlags, GetDesktopWindow(), &pcbs->hbrf);
        hres = HRESULT_FROM_TR(tr);

        // Unavailable disk?
        if (FAILED(hres))
            {
            DWORD dwError = GetLastError();

            if (ERROR_INVALID_DATA == dwError || ERROR_ACCESS_DENIED == dwError)
                {
                // Yes; ask user to retry/cancel
                int id = MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_ERR_OPEN_UNAVAIL_VOL),
                    MAKEINTRESOURCE(IDS_CAP_OPEN), NULL, MB_RETRYCANCEL | MB_ICONWARNING);

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

    if (SUCCEEDED(hres))
        {
        if (!Cache_AddItem(&g_cacheCBS, atomPath, (LPVOID)pcbs))
            {
            Sync_CloseBriefcase(pcbs->hbrf);
            hres = ResultFromScode(E_OUTOFMEMORY);
            }
        else
            {
            TCHAR szTmp[MAXPATHLEN]; 
            BOOL bRunWizard;

            lstrcpy(szTmp, pszDatPath);
            PathRemoveFileSpec(szTmp);
            PathAppend(szTmp, c_szDesktopIni);
            bRunWizard = GetPrivateProfileInt(STRINI_CLASSINFO, c_szRunWizard, 0, szTmp);

            // Run the wizard?
            if (bRunWizard)
                {
                // Yes; set a flag for the wizard
                SetFlag(pcbs->uFlags, CBSF_RUNWIZARD);

                // Delete the .ini entry
                WritePrivateProfileString(STRINI_CLASSINFO, c_szRunWizard, NULL, szTmp);
                }

            TRACE_MSG(TF_GENERAL, TEXT("Opened Briefcase %s (0x%lx)"), pszDatPath, pcbs->hbrf);
            }
        }
    return hres;
    }


/*----------------------------------------------------------
Purpose: This function handles the case when the engine fails 
         to open the database because the database file is
         corrupt.

Returns: standard hresult
Cond:    --
*/
HRESULT PRIVATE HandleCorruptDatabase(
    CBS * pcbs,
    int atomPath,
    LPCTSTR pszDatPath,      // Path of database file
    HWND hwndOwner)
    {
    TCHAR szTemplate[MAXPATHLEN];
    TCHAR szNewFile[MAXPATHLEN];
    LPTSTR pszNewPath = szTemplate;
    LPCTSTR pszPath = Atom_GetName(atomPath);
    LPTSTR psz;
    DWORD dwAttr;

    static UINT rgids[2] = { IDS_ERR_1_CorruptDB, IDS_ERR_2_CorruptDB };

    ASSERT(pszPath);

    // Create the new database name
    //
    SzFromIDS(IDS_BOGUSDBTEMPLATE, szTemplate, ARRAYSIZE(szTemplate));
    PathMakeUniqueName(szNewFile, ARRAYSIZE(szNewFile), TEXT("badbc.dat"), szTemplate,
        pszPath);
    lstrcpy(pszNewPath, pszPath);
    PathAppend(pszNewPath, szNewFile);

    // Move the database
    //
    MoveFile(pszDatPath, pszNewPath);

    // Unhide the corrupt database 
    //
    dwAttr = GetFileAttributes(pszNewPath);
    if (dwAttr != 0xFFFFFFFF)
        {
        ClearFlag(dwAttr, FILE_ATTRIBUTE_HIDDEN);
        SetFileAttributes(pszNewPath, dwAttr);
        }

    if (FmtString(&psz, IDS_ERR_F_CorruptDB, rgids, ARRAYSIZE(rgids)))
        {
        MsgBox(hwndOwner, psz, MAKEINTRESOURCE(IDS_CAP_OPEN), NULL, MB_ERROR);
        GFree(psz);
        }
    DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Renaming corrupt database to %s"), pszNewPath); )

    // Retry opening...
    //
    return OpenTheBriefcase(pszDatPath, atomPath, pcbs, hwndOwner);
    }


/*----------------------------------------------------------
Purpose: Add the atomPath to the cache.  We open the briefcase
         database if it needs opening.  If atomPath is already
         in the cache, simply return the pointer to the entry.

Returns: standard hresult

Cond:    Must call CBS_Delete for every call to this function
*/
HRESULT PUBLIC CBS_Add(
    PCBS * ppcbs,
    int atomPath,
    HWND hwndOwner)
    {
    HRESULT hres = NOERROR;
    TCHAR szDatPath[MAXPATHLEN];
    CBS  * pcbs;
    
    CBS_EnterCS();
        {
        pcbs = Cache_GetPtr(&g_cacheCBS, atomPath);
        if (NULL == pcbs)
            {
            // Allocate using commctrl's Alloc, so the structure will be in
            // shared heap space across processes.
            pcbs = SharedAllocType(CBS);
            if (NULL == pcbs)
                {
                hres = ResultFromScode(E_OUTOFMEMORY);
                }
            else
                {
                LPCTSTR pszPath = Atom_GetName(atomPath);
                LPCTSTR pszDBName;

                ASSERT(pszPath);

                pcbs->atomBrf = atomPath;
                pcbs->uFlags = 0;

                // Create an abort event object simply so we can programmatically
                // cancel a createreclist call in the worker thread.  This
                // would happen if the user closed the briefcase during
                // CreateRecList.  

                // (it is ok if this fails)
                AbortEvt_Create(&pcbs->pabortevt, AEF_SHARED);
                    
                DEBUG_CODE( TRACE_MSG(TF_GENERAL, TEXT("Opening Briefcase %s..."), pszPath); )

                if (IsLFNDrive(pszPath))
                    {
                    pszDBName = g_szDBName;
                    SetFlag(pcbs->uFlags, CBSF_LFNDRIVE);
                    }
                else
                    pszDBName = g_szDBNameShort;

                if (PathsTooLong(pszPath, pszDBName))
                    {
                    MsgBox(hwndOwner, MAKEINTRESOURCE(IDS_ERR_OPEN_TOOLONG), 
                           MAKEINTRESOURCE(IDS_CAP_OPEN), NULL, MB_ERROR);
                    hres = E_FAIL;
                    }
                else
                    {
                    PathCombine(szDatPath, pszPath, pszDBName);
                    hres = OpenTheBriefcase(szDatPath, atomPath, pcbs, hwndOwner);
                    if (FAILED(hres))
                        {
                        DEBUG_CODE( TRACE_MSG(TF_ERROR, TEXT("Open failed.  Error is %s"), SzFromTR(GET_TR(hres))); )

                        SEMsgBox(hwndOwner, IDS_CAP_OPEN, hres, c_rgseOpenBriefcase, ARRAYSIZE(c_rgseOpenBriefcase));

                        // Is this a corrupt briefcase?
                        if (E_TR_CORRUPT_BRIEFCASE == hres)
                            {
                            // Yes; try to create a new database
                            hres = HandleCorruptDatabase(pcbs, atomPath, szDatPath, hwndOwner);
                            }
                        }
                    }
                }
            }

        // Did something fail above?
        if (FAILED(hres))
            {
            // Yes; cleanup
            if (pcbs)
                {
                if (pcbs->hbrf)
                    Sync_CloseBriefcase(pcbs->hbrf);

                SharedFree(&pcbs);
                }
            }

        *ppcbs = pcbs;
        }
    CBS_LeaveCS();

    return hres;
    }


