//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: crl.c
//
//  This files contains code for the cached reclists
//
// History:
//  09-02-93 ScottH     Created
//  01-31-94 ScottH     Moved from cache.c
//
//---------------------------------------------------------------------------

#include "brfprv.h"         // common headers
#include "recact.h"

#include "res.h"

#define CRL_Iterate(atom)       \
            for (atom = Cache_FindFirstKey(&g_cacheCRL);        \
                ATOM_ERR != atom;                               \
                atom = Cache_FindNextKey(&g_cacheCRL, atom))

CACHE g_cacheCRL = {0, 0, 0};        // Reclist cache

#define CRL_EnterCS()    EnterCriticalSection(&g_cacheCRL.cs)
#define CRL_LeaveCS()    LeaveCriticalSection(&g_cacheCRL.cs)


#ifdef DEBUG
/*----------------------------------------------------------
Purpose: Dump a CRL entry
Returns: --
Cond:    --
*/
void PRIVATE CRL_DumpEntry(
    CRL  * pcrl)
    {
    TCHAR sz[MAXBUFLEN];

    ASSERT(pcrl);

    TRACE_MSG(TF_ALWAYS, TEXT("CRL:  Atom %d: %s"), pcrl->atomPath, Atom_GetName(pcrl->atomPath));
    TRACE_MSG(TF_ALWAYS, TEXT("      Outside %d: %s"), pcrl->atomOutside, Atom_GetName(pcrl->atomOutside));
    TRACE_MSG(TF_ALWAYS, TEXT("               Ref [%u]  Use [%u]  %s  %s  %s  %s"), 
        Cache_GetRefCount(&g_cacheCRL, pcrl->atomPath),
        pcrl->ucUse,
        CRL_IsOrphan(pcrl) ? (LPCTSTR) TEXT("Orphan") : (LPCTSTR) TEXT(""),
        IsFlagSet(pcrl->uFlags, CRLF_DIRTY) ? (LPCTSTR) TEXT("Dirty") : (LPCTSTR) TEXT(""),
        IsFlagSet(pcrl->uFlags, CRLF_NUKE) ? (LPCTSTR) TEXT("Nuke") : (LPCTSTR) TEXT(""),
        CRL_IsSubfolderTwin(pcrl) ? (LPCTSTR) TEXT("SubfolderTwin") : (LPCTSTR) TEXT(""));
    TRACE_MSG(TF_ALWAYS, TEXT("               Status: %s"), SzFromIDS(pcrl->idsStatus, sz, ARRAYSIZE(sz)));
    }


void PUBLIC CRL_DumpAll()
    {
    CRL  * pcrl;
    int atom;
    BOOL bDump;

    ENTEREXCLUSIVE()
        {
        bDump = IsFlagSet(g_uDumpFlags, DF_CRL);
        }
    LEAVEEXCLUSIVE()

    if (!bDump)
        return ;

    CRL_Iterate(atom)
        {
        pcrl = Cache_GetPtr(&g_cacheCRL, atom);
        ASSERT(pcrl);
        if (pcrl)
            {
            CRL_DumpEntry(pcrl);
            Cache_DeleteItem(&g_cacheCRL, atom, FALSE, NULL, CRL_Free);    // Decrement count
            }
        }
    }
#endif


/*----------------------------------------------------------
Purpose: Return the resource string ID describing the action to take
Returns: --
Cond:    --
*/
UINT PRIVATE IdsFromRAItem(
    LPRA_ITEM pitem)
    {
    UINT ids;

    ASSERT(IsFlagSet(pitem->mask, RAIF_ACTION));

    switch (pitem->uAction)
        {
    case RAIA_TOOUT:
    case RAIA_TOIN:
    case RAIA_CONFLICT:
    case RAIA_DELETEOUT:
    case RAIA_DELETEIN:
    case RAIA_MERGE:
    case RAIA_SOMETHING:
        ids = IDS_STATE_NeedToUpdate;
        break;

    case RAIA_ORPHAN:
        ids = IDS_STATE_Orphan;
        break;

    case RAIA_DONTDELETE:
    case RAIA_SKIP:
        ASSERT(SI_UNAVAILABLE == pitem->siInside.uState ||
               SI_UNAVAILABLE == pitem->siOutside.uState);
        if (SI_UNAVAILABLE == pitem->siOutside.uState)
            {
            if (SI_UNCHANGED == pitem->siInside.uState)
                {
                ids = IDS_STATE_UptodateInBrf;
                }
            else if (SI_UNAVAILABLE != pitem->siInside.uState)
                {
                ids = IDS_STATE_NeedToUpdate;
                }
            else
                {
                ids = IDS_STATE_Unavailable;
                }
            }
        else
            {
            ASSERT(SI_UNAVAILABLE == pitem->siInside.uState);
            ids = IDS_STATE_Unavailable;
            }
        break;

    case RAIA_NOTHING:
        ids = IDS_STATE_Uptodate;
        break;

    default:
        ASSERT(0);
        ids = 0;
        break;
        }

    return ids;
    }


/*----------------------------------------------------------
Purpose: Gets the outside sync copy and the resource ID to the
         status string that indicates the status between the 
         sync copies.

Returns: --
Cond:    --
*/
void PRIVATE SetPairInfo(
    PCRL pcrl)
    {
    LPCTSTR pszPath = Atom_GetName(pcrl->atomPath);
    LPCTSTR pszName = PathFindFileName(pszPath);

    // Is this an orphan?
    if (CRL_IsOrphan(pcrl))
        {
        // Yes; special case: is this one of the briefcase system files?
        LPCTSTR pszDBName;

        if (IsFlagSet(pcrl->uFlags, CRLF_ISLFNDRIVE))
            pszDBName = g_szDBName;
        else
            pszDBName = g_szDBNameShort;

        if (IsSzEqual(pszName, pszDBName) || 
            IsSzEqual(pszName, c_szDesktopIni))
            {
            // Yes
            pcrl->idsStatus = IDS_STATE_SystemFile;
            }
        // Is this a subfolder twin?  (Only orphans are
        // candidates for being subfolder twins.)
        else if (CRL_IsSubfolderTwin(pcrl))
            {
            // Yes
            ASSERT(PathIsDirectory(pszPath));

            pcrl->idsStatus = IDS_STATE_Subfolder;
            }
        else
            {
            // No
            pcrl->idsStatus = IDS_STATE_Orphan;
            }

        if (Atom_IsValid(pcrl->atomOutside))
            {
            Atom_Delete(pcrl->atomOutside);     // delete the old one
            }
        pcrl->atomOutside = Atom_Add(TEXT(""));
        }
    else
        {
        // No; get the info for this sync copy
        HRESULT hres;
        LPRA_ITEM pitem;
        TCHAR sz[MAXPATHLEN];

        ASSERT(pcrl->lprl);

        hres = RAI_Create(&pitem, Atom_GetName(pcrl->atomBrf), pszPath, 
            pcrl->lprl, pcrl->lpftl);

        if (SUCCEEDED(hres))
            {
            lstrcpy(sz, pitem->siOutside.pszDir);

            // Is this a file?
            if ( !CRL_IsFolder(pcrl) )
                {
                // Yes; atomOutside needs to be a fully qualified path to
                // the outside file/folder--not just the parent folder.
                // That's why we tack on the filename here.
                PathAppend(sz, pszName);
                }

            if (Atom_IsValid(pcrl->atomOutside))
                {
                Atom_Delete(pcrl->atomOutside);     // delete the old one
                }

            pcrl->atomOutside = Atom_Add(sz);
            pcrl->idsStatus = IdsFromRAItem(pitem);
            RAI_Free(pitem);
            }
        }
    }


/*----------------------------------------------------------
Purpose: Determines whether or not a subfolder of a briefcase 
         is the root of a subtree twin.
Returns: --
Cond:    --
*/
BOOL PRIVATE IsSubtreeTwin(HBRFCASE hbrf, LPCTSTR pcszFolder)
{
   BOOL bIsSubtreeTwin = FALSE;
   PFOLDERTWINLIST pftl;

   ASSERT(PathIsDirectory(pcszFolder));

   /* Create a folder twin list for the folder. */

   if (Sync_CreateFolderList(hbrf, pcszFolder, &pftl) == TR_SUCCESS)
   {
      PCFOLDERTWIN pcft;

      /*
       * Look through the folder twin list for any folder twins with the
       * FT_FL_SUBTREE flag set.
       */

      for (pcft = pftl->pcftFirst; pcft; pcft = pcft->pcftNext)
      {
         if (pcft->dwFlags & FT_FL_SUBTREE)
         {
            bIsSubtreeTwin = TRUE;
            break;
         }
      }

      Sync_DestroyFolderList(pftl);
   }

   return(bIsSubtreeTwin);
}


/*----------------------------------------------------------
Purpose: Determines whether or not a path is a subfolder of a subtree twin in a
         briefcase.
Returns: --
Cond:    --
*/
BOOL PUBLIC IsSubfolderTwin(HBRFCASE hbrf, LPCTSTR pcszPath)
{
   BOOL bIsSubfolderTwin = FALSE;
   TCHAR szBrfRoot[MAXPATHLEN];

   if (PathIsDirectory(pcszPath) &&
       PathGetLocality(pcszPath, szBrfRoot) == PL_INSIDE)
   {
      int ncchBrfRootLen;
      TCHAR szParent[MAXPATHLEN];

      ASSERT(PathIsPrefix(szBrfRoot, pcszPath));

      ncchBrfRootLen = lstrlen(szBrfRoot);

      ASSERT(lstrlen(pcszPath) < ARRAYSIZE(szParent));
      lstrcpy(szParent, pcszPath);

      /*
       * Keep whacking off the last path component until we find a parent
       * subtree twin root, or we hit the briefcase root.
       */

      while (! bIsSubfolderTwin &&
             PathRemoveFileSpec(szParent) &&
             lstrlen(szParent) > ncchBrfRootLen)
      {
         BOOL bIsFolderTwin;

         if (Sync_IsFolder(hbrf, szParent, &bIsFolderTwin) == TR_SUCCESS &&
             bIsFolderTwin)
         {
            bIsSubfolderTwin = IsSubtreeTwin(hbrf, szParent);

#ifdef DEBUG
            TRACE_MSG(TF_CACHE, TEXT("CACHE  Found subfolder twin %s with parent subtree twin root %s."),
                      pcszPath,
                      szParent);
#endif
         }
      }
   }

   return(bIsSubfolderTwin);
}


/*----------------------------------------------------------
Purpose: Sets the bSubfolderTwin member of a CRL.
Returns: --
Cond:    The lprl and lpftl members of the CRL must be filled in before calling
         this function.
*/
void PRIVATE SetSubfolderTwinFlag(PCRL pcrl)
    {
    if (! pcrl->lprl && ! pcrl->lpftl)
        {
        if (IsSubfolderTwin(pcrl->hbrf, Atom_GetName(pcrl->atomPath)))
            SetFlag(pcrl->uFlags, CRLF_SUBFOLDERTWIN);
        else
            ClearFlag(pcrl->uFlags, CRLF_SUBFOLDERTWIN);
        }
    else
        {
        ClearFlag(pcrl->uFlags, CRLF_SUBFOLDERTWIN);
        }
    }


/*----------------------------------------------------------
Purpose: Free the reclist
Returns: --

Cond:    hwndOwner is not used, so it is okay for all CRL_ routines
         to pass NULL as hwndOwner.

         This function is serialized by the caller (Cache_Term or
         Cache_DeleteItem).
*/
void CALLBACK CRL_Free(
    LPVOID lpv,
    HWND hwndOwner)
    {
    CRL  * pcrl = (CRL  *)lpv;

    ASSERT(Sync_IsEngineLoaded());

    DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  Destroying CRL for %s (0x%lx)"), 
        Atom_GetName(pcrl->atomPath), pcrl->hbrf); )
   
    if (Atom_IsValid(pcrl->atomOutside))
        Atom_Delete(pcrl->atomOutside);

    if (Atom_IsValid(pcrl->atomBrf))
        Atom_Delete(pcrl->atomBrf);

    if (pcrl->lprl)
        Sync_DestroyRecList(pcrl->lprl);

    if (pcrl->lpftl)
        Sync_DestroyFolderList(pcrl->lpftl);

    // The CRL does not own pabortevt, leave it alone

    SharedFree(&pcrl);
    }


/*----------------------------------------------------------
Purpose: Create a reclist and (optional) folder twin list for a path.
          
Returns: standard result
         S_OK if the item is a twin
         S_FALSE if the item is an orphan

Cond:    --
*/
HRESULT PRIVATE CreatePathLists(
    HBRFCASE hbrf,
    PABORTEVT pabortevt,
    int atomPath,
    PRECLIST  * lplprl,
    PFOLDERTWINLIST  * lplpftl)
    {
    HRESULT hres;
    LPCTSTR pszPath = Atom_GetName(atomPath);

    ASSERT(pszPath);
    ASSERT(hbrf);
    ASSERT(lplprl);
    ASSERT(lplpftl);

    *lplprl = NULL;
    *lplpftl = NULL;

    // Two routes.  
    //
    //  1) If the path is to the root of a briefcase,
    //     create a complete reclist.
    //
    //  2) Otherwise create a reclist for the individual file or folder
    //
    // Hack: a quick way of telling if atomPath is a briefcase
    // root is by looking for it in the CBS cache.

    // Is this the root of a briefcase?
    if (CBS_Get(atomPath))
        {
        // Yes
        CBS_Delete(atomPath, NULL);       // Decrement count
        hres = Sync_CreateCompleteRecList(hbrf, pabortevt, lplprl);
        }
    else
        {
        // No; is this a twin?
        hres = Sync_IsTwin(hbrf, pszPath, 0);
        if (S_OK == hres)
            {
            // Yes; create a reclist (and an optional folder twin list).
            HTWINLIST htl;

            hres = E_OUTOFMEMORY;   // assume error

            if (Sync_CreateTwinList(hbrf, &htl) == TR_SUCCESS)
                {
                if (Sync_AddPathToTwinList(hbrf, htl, pszPath, lplpftl))
                    {
                    hres = Sync_CreateRecListEx(htl, pabortevt, lplprl);

                    if (SUCCEEDED(hres))
                        {
                        // The object may have been implicitly deleted
                        // in CreateRecList.  Check again.
                        hres = Sync_IsTwin(hbrf, pszPath, 0);
                        }
                    }
                Sync_DestroyTwinList(htl);
                }
            }
        }

    if (FAILED(hres))
        {
        // Cleanup on failure
        //
        if (*lplpftl)
            Sync_DestroyFolderList(*lplpftl);

        *lplprl = NULL;
        *lplpftl = NULL;
        }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Add a CRL entry for the atomPath to the cache.  This 
         consists of creating the reclist (and folder twin list 
         possibly).

         If the atomPath is already in the cache, this function
         increments the reference count of the item and calls
         CRL_Replace.

Returns: standard result

Cond:    Must call CRL_Delete for every call to this function.

         IMPORTANT: Some portions of code call PathIsDirectory,
         which will fail if atomPath does not exist.

*/
HRESULT PUBLIC CRL_Add(
    PCBS pcbs,
    int atomPath)
    {
    HRESULT hres = E_OUTOFMEMORY;
    PRECLIST lprl = NULL;
    PFOLDERTWINLIST lpftl = NULL;
    CRL  * pcrl;

    ASSERT(pcbs);

    CRL_EnterCS();
        {
        // Caller wants to add.  If it already exists, we simply return
        //  the existing entry.
        //
        // This CRL_Get increments the count (if it succeeds)
        pcrl = Cache_GetPtr(&g_cacheCRL, atomPath);

        // Is the item in the cache?
        if (pcrl)
            {
            // Yes; attempt to get fresh contents
            hres = CRL_Replace(atomPath);
            }
        else
            {
            // No; the entry is not in the cache.  Add it.
            LPCTSTR pszPath = Atom_GetName(atomPath);

            ASSERT(pszPath);

            DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  Adding CRL for %s (0x%lx)"), 
                pszPath, pcbs->hbrf); )

            // Leave the critical section while we do expensive calculation
            CRL_LeaveCS();
                {
                hres = CreatePathLists(pcbs->hbrf, pcbs->pabortevt,
                    atomPath, &lprl, &lpftl);
                }
            CRL_EnterCS();

            if (FAILED(hres))
                goto Fail;
            else
                {
                LPCTSTR pszBrf;

                // Allocate using commctrl's Alloc, so the structure will be in
                // shared heap space across processes.
                pcrl = SharedAllocType(CRL);
                if (!pcrl)
                    {
                    hres = E_OUTOFMEMORY;
                    goto Fail;
                    }

                pcrl->atomPath = atomPath;

                pcrl->hbrf = pcbs->hbrf;
                
                pszBrf = Atom_GetName(pcbs->atomBrf);
                pcrl->atomBrf = Atom_Add(pszBrf);

                pcrl->pabortevt = pcbs->pabortevt;
                pcrl->lpftl = lpftl;
                pcrl->lprl = lprl;
                pcrl->ucUse = 0;

                pcrl->uFlags = 0;       // reset
                SetSubfolderTwinFlag(pcrl);
                if (S_FALSE == hres)
                    SetFlag(pcrl->uFlags, CRLF_ORPHAN);

                if (PathIsDirectory(Atom_GetName(atomPath)))
                    SetFlag(pcrl->uFlags, CRLF_ISFOLDER);

                if (IsFlagSet(pcbs->uFlags, CBSF_LFNDRIVE))
                    SetFlag(pcrl->uFlags, CRLF_ISLFNDRIVE);

                SetPairInfo(pcrl);

                // This Cache_AddItem does the increment count
                if ( !Cache_AddItem(&g_cacheCRL, atomPath, (LPVOID)pcrl) )
                    {
                    // Failed
                    Atom_Delete(pcrl->atomBrf);
                    Atom_Delete(pcrl->atomOutside);
                    hres = E_OUTOFMEMORY;
                    goto Fail;
                    }
                }
            }
        }
    CRL_LeaveCS();

    return hres;
    
Fail:
    // Cleanup on failure
    //
    if (lprl)
        Sync_DestroyRecList(lprl);
    if (lpftl)
        Sync_DestroyFolderList(lpftl);
    SharedFree(&pcrl);
    CRL_LeaveCS();

    DEBUG_MSG(TF_ERROR, TEXT("SyncUI   CRL_Add failed!"));
    return hres;
    }


/*----------------------------------------------------------
Purpose: Decrement the reference count and the use count to 
         the reclist cache entry.

         If the reference count == 0, the entry is deleted.
         
Returns: --
Cond:    --
*/
void PUBLIC CRL_Delete(
    int atomPath)
    {
    CRL  * pcrl;

    CRL_EnterCS();
        {
        // Decrement the use count
        //
        pcrl = Cache_GetPtr(&g_cacheCRL, atomPath);
        if (pcrl)
            {
            DEBUG_CODE( LPCTSTR pszPath = Atom_GetName(atomPath); )

            if (pcrl->ucUse > 0)
                pcrl->ucUse--;

            if (IsFlagSet(pcrl->uFlags, CRLF_NUKE))
                {
                if (pcrl->ucUse == 0)
                    {
                    DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  Nuking late CRL %s..."), 
                        pszPath); )

                    // A nuke was deferred.  Now we can really do it.
                    //
                    Cache_DeleteItem(&g_cacheCRL, atomPath, TRUE, NULL, CRL_Free);
                    goto Done;
                    }
#ifdef DEBUG
                else
                    {
                    TRACE_MSG(TF_CACHE, TEXT("CACHE  Deferring nuke CRL %s..."), 
                        pszPath);
                    }
#endif
                }
            Cache_DeleteItem(&g_cacheCRL, atomPath, FALSE, NULL, CRL_Free);    // Decrement for Cache_GetPtr

            // The real delete...
            Cache_DeleteItem(&g_cacheCRL, atomPath, FALSE, NULL, CRL_Free);
            }
Done:;
        }
    CRL_LeaveCS();
    }


/*----------------------------------------------------------
Purpose: Nuke the cache entry if the use count is 0.  Otherwise,
         set the nuke bit, and this entry will get nuked on the
         next CRL_Get when the use count is 0.

Returns: --
Cond:    --
*/
void PUBLIC CRL_Nuke(
    int atomPath)
    {
    CRL  * pcrl;

    CRL_EnterCS();
        {
        // Check the use count
        //
        pcrl = Cache_GetPtr(&g_cacheCRL, atomPath);
        if (pcrl)
            {
            if (pcrl->ucUse > 0)
                {
                DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  Marking to nuke CRL for %s (0x%lx)"), 
                    Atom_GetName(atomPath), pcrl->hbrf); )

                SetFlag(pcrl->uFlags, CRLF_NUKE);
                Cache_DeleteItem(&g_cacheCRL, atomPath, FALSE, NULL, CRL_Free);    // Decrement for Cache_GetPtr
                }
            else
                {
                DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  Nuking CRL for %s (0x%lx)"), 
                    Atom_GetName(atomPath), pcrl->hbrf); )

                Cache_DeleteItem(&g_cacheCRL, atomPath, TRUE, NULL, CRL_Free);
                }
            }
        }
    CRL_LeaveCS();
    }


/*----------------------------------------------------------
Purpose: Replace the atomPath in the cache.  This consists of 
         creating the reclist (and folder twin list possibly)
         and replacing the contents of pcrl.  
         
         The pcrl pointer remains unchanged.

         The reference and use-counts remain unchanged.

Returns: standard result 

Cond:    
         IMPORTANT: Some portions of code call PathIsDirectory,
         which will fail if atomPath does not exist.

*/
HRESULT PUBLIC CRL_Replace(
    int atomPath)
    {
    HRESULT hres;
    CRL * pcrl;

    CRL_EnterCS();
        {
        pcrl = Cache_GetPtr(&g_cacheCRL, atomPath);

        // Does the item exist?
        if (pcrl)
            {
            DEBUG_CODE( LPCTSTR pszPath = Atom_GetName(atomPath); )
            ASSERT(pszPath);
            DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  Replacing CRL for %s (0x%lx)"), 
                pszPath, pcrl->hbrf); )

            // Yes; mark it dirty and call CRL_Get on it.
            SetFlag(pcrl->uFlags, CRLF_DIRTY);

            // Note: pay attention to the difference between Cache_Delete
            //  and CRL_Delete.  Cache_Delete must match Cache_Add/Get and
            //  CRL_Delete must match CRL_Add/Get.
            //
            Cache_DeleteItem(&g_cacheCRL, atomPath, FALSE, NULL, CRL_Free);    // Decrement count for Cache_GetPtr

            hres = CRL_Get(atomPath, &pcrl);  // This does the replace

            CRL_Delete(atomPath);             // Decrement count for CRL_Get
            }
        else
            {
            hres = E_FAIL;
            }
        }
    CRL_LeaveCS();

    return hres;
    }


/*----------------------------------------------------------
Purpose: Get the reclist from the cache.  If the cache item exists
          and is marked dirty and the use count is 0, then recreate 
          the reclist.

         If the nuke bit is set, then the entry is nuked and this
          function returns NULL.

Returns: standard result
         Ptr to cache entry.  

Cond:    Must call CRL_Delete for every call to CRL_Get

         IMPORTANT: Some portions of code call PathIsDirectory,
         which will fail if atomPath does not exist.

*/
HRESULT PUBLIC CRL_Get(
    int atomPath,
    PCRL * ppcrl)
    {
    HRESULT hres;
    PCRL pcrl;
    PRECLIST lprl = NULL;
    PFOLDERTWINLIST lpftl = NULL;

    CRL_EnterCS();
        {
        // Don't need to decrement the reference count in this 
        //  function -- this is Get!
        //
        pcrl = Cache_GetPtr(&g_cacheCRL, atomPath);     
        if (pcrl)
            {
            HBRFCASE hbrf = pcrl->hbrf;

            // Is this item pending a nuke?
            if (IsFlagSet(pcrl->uFlags, CRLF_NUKE))
                {
                // Yes; return NULL as if it were already nuked.
                DEBUG_CODE( LPCTSTR pszPath = Atom_GetName(atomPath); )

                DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  Attempt to get deferred nuke CRL %s (0x%lx)..."), 
                    pszPath, hbrf); )

                // (Decrement counter for Cache_GetPtr to keep count even,
                // since we are returning NULL.)
                Cache_DeleteItem(&g_cacheCRL, atomPath, FALSE, NULL, CRL_Free);     
                pcrl = NULL;
                hres = E_FAIL;
                }

            // Is this item tagged dirty and the use-count is 0?
            else if (IsFlagSet(pcrl->uFlags, CRLF_DIRTY) && pcrl->ucUse == 0)
                {
                // Yes; we are free to re-create the reclist.
                LPCTSTR pszPath = Atom_GetName(atomPath);

                ASSERT(pszPath);

                DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  Getting clean CRL %s (0x%lx)..."), 
                    pszPath, hbrf); )

                // Since we'll be leaving the critical section below,
                // temporarily increase the use count to keep the pcrl
                // from getting nuked from under us.
                pcrl->ucUse++;

                // Replace the contents of the cache entry
                // Leave the critical section while we do expensive calculation
                CRL_LeaveCS();
                    {
                    hres = CreatePathLists(hbrf, pcrl->pabortevt, atomPath,
                        &lprl, &lpftl);
                    }
                CRL_EnterCS();

                // Decrement use count
                pcrl->ucUse--;

                if (FAILED(hres))
                    {
                    DEBUG_CODE( DEBUG_MSG(TF_ERROR, TEXT("SyncUI   CRL_Get failed in cleaning dirty entry!")); )

                    // Still return pcrl since it exists
                    hres = NOERROR;
                    }
                else
                    {
                    // Put the new lists in the cache entry
                    if (pcrl->lprl)
                        {
                        Sync_DestroyRecList(pcrl->lprl);
                        }
                    pcrl->lprl = lprl;

                    if (pcrl->lpftl)
                        {
                        Sync_DestroyFolderList(pcrl->lpftl);
                        }
                    pcrl->lpftl = lpftl;

                    if (S_FALSE == hres)
                        SetFlag(pcrl->uFlags, CRLF_ORPHAN);
                    else
                        {
                        ASSERT(S_OK == hres);
                        ClearFlag(pcrl->uFlags, CRLF_ORPHAN);
                        }

                    ClearFlag(pcrl->uFlags, CRLF_DIRTY);
                    SetSubfolderTwinFlag(pcrl);

                    SetPairInfo(pcrl);
                    hres = NOERROR;
                    }
                }
            else
                {
#ifdef DEBUG
                LPCTSTR pszPath = Atom_GetName(atomPath);

                ASSERT(pszPath);

                if (IsFlagSet(pcrl->uFlags, CRLF_DIRTY))
                    {
                    TRACE_MSG(TF_CACHE, TEXT("CACHE  Getting dirty CRL %s (0x%lx)..."), 
                        pszPath, hbrf);
                    }
                else
                    {
                    TRACE_MSG(TF_CACHE, TEXT("CACHE  Getting CRL %s (0x%lx)..."), 
                        pszPath, hbrf);
                    }
#endif
                hres = NOERROR;
                }
            }
        else
            hres = E_FAIL;

        // Now increment the use count
        //
        if (pcrl)
            pcrl->ucUse++;

        *ppcrl = pcrl;

        ASSERT((FAILED(hres) && !*ppcrl) || 
               (SUCCEEDED(hres) && *ppcrl));
        }
    CRL_LeaveCS();

    return hres;
    }


/*----------------------------------------------------------
Purpose: Mark any related CRLs dirty.  Which CRLs get marked dirty
         depends on the lEvent.

         In addition, *pbRefresh is set to TRUE if the window that
         is calling this function should refresh itself.

         (N.b. in the following rules, we never refresh the immediate
         folder unless explicitly stated, since the shell will do this
         automatically.  E.g., if C:\Bar\Foo.txt received NOE_DIRTYITEM,
         the shell will automatically repaint C:\Bar.)

         Rules:

         NOE_CREATE     Cause:   A file is created.
                        Dirty?   Any CRLs whose atomInside or atomOutside
                                 are parents or equal to atomPath.
                        Refresh? Only in windows of parent folders on either
                                 side OR
                                 in immediate window on the briefcase side 
                                 if atomPath was created on the outside.

         NOE_CREATEFOLDER  same as above

         NOE_DELETE     Cause:   A file or folder was deleted.
                        Dirty?   Any CRLs whose atomInside or atomOutside
                                 are parents or equal to atomPath.
                                 Delete CRL if atomPath matches atomInside
                        Refresh? Only in windows of parent folders on either
                                 side OR
                                 in immediate window on the briefcase side 
                                 if atomPath was deleted on the outside.

         NOE_DELETEFOLDER  same as above

         NOE_RENAME     Cause:   A file or folder was renamed or moved.
                        Dirty?   Any CRLs whose atomInside or atomOutside
                                 are parents or equal to atomPath.
                                 Rename CRL (and related database entry) if
                                 atomPath is inside briefcase
                        Refresh? Only in windows of parent folders on either
                                 side OR
                                 in immediate window on the briefcase side
                                 if atomPath is renamed on the outside.

         NOE_RENAMEFOLDER  same as above

         NOE_DIRTY      Cause:   Varies.  Typically something needs refreshed.
                        Dirty?   Any CRLs whose atomInside or atomOutside
                                 are parents or equal to atomPath.
                                 If atomPath is folder, any CRLs which are
                                 children of atomPath.
                        Refresh? Only in windows of parent folders on either
                                 side OR
                                 in immediate window on the briefcase side
                                 if atomPath is updated on the outside.

Returns: FALSE if nothing was marked
         TRUE if something was marked
Cond:    --
*/
BOOL PUBLIC CRL_Dirty(
    int atomPath,
    int atomCabFolder,      // path of open cabinet window
    LONG lEvent,
    LPBOOL pbRefresh)       // return TRUE to refresh cabinet window
    {
    BOOL bRet = FALSE;
    CRL  * pcrl;
    int atom;
    int atomParent;

    ASSERT(pbRefresh);

    *pbRefresh = FALSE;

    CRL_EnterCS();
        {
        BOOL bIsFolder;

        // Get the atomParent of the atomPath
        TCHAR szParent[MAXPATHLEN];
        
        lstrcpy(szParent, Atom_GetName(atomPath));
        PathRemoveFileSpec(szParent);
        if (0 == *szParent)         // skip blank parent paths 
            goto Done;
        atomParent = Atom_Add(szParent);
        if (ATOM_ERR == atomParent)
            goto Done;

        // Is the path that is being updated a folder?
        pcrl = Cache_GetPtr(&g_cacheCRL, atomPath);
        if (pcrl)
            {
            bIsFolder = CRL_IsFolder(pcrl);
            Cache_DeleteItem(&g_cacheCRL, atomPath, FALSE, NULL, CRL_Free);    // Decrement count
            }
        else
            {
            bIsFolder = FALSE;
            }

        CRL_Iterate(atom)
            {
            pcrl = Cache_GetPtr(&g_cacheCRL, atom);
            ASSERT(pcrl);
            if (pcrl)
                {
                // Is CRL a parent or equal of atomPath?
                if (Atom_IsParentOf(atom, atomPath))
                    {
                    // Yes; mark it
                    DEBUG_CODE( LPCTSTR pszDbg = Atom_GetName(atom); )
                    DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  Tagging CRL for %s (0x%lx)"), 
                        pszDbg, pcrl->hbrf); )
    
                    SetFlag(pcrl->uFlags, CRLF_DIRTY);
                    bRet = TRUE;

                    // Refresh this window?
                    // (Only if the cabinet folder is > than an immediate
                    // parent folder.)
                    *pbRefresh = Atom_IsParentOf(atomCabFolder, atom) &&
                                 atomCabFolder != atomParent;

                    switch (lEvent)
                        {
                    case NOE_DELETE:
                    case NOE_DELETEFOLDER:
                        // Is this CRL the item being deleted?
                        if (pcrl->atomPath == atomPath) 
                            {
                            // Yes; delete the CRL
                            CRL_Delete(atom);
                            }
                        break;

                    case NOE_RENAME:
                    case NOE_RENAMEFOLDER:
                        // Is this CRL being renamed (inside briefcase only)?
                        if (pcrl->atomPath == atomPath)
                            {
                            // Yes; mark it for renaming
                            // BUGBUG
                            }
                        break;

                    case NOE_DIRTY:
                    case NOE_DIRTYFOLDER:
                        // Is the atomPath a folder and this CRL a child?
                        if (bIsFolder && Atom_IsChildOf(pcrl->atomPath, atomPath))
                            {
                            // Yes; mark it
                            DEBUG_CODE( LPCTSTR pszDbg = Atom_GetName(atom); )
                            DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  Tagging CRL for %s (0x%lx)"), 
                                pszDbg, pcrl->hbrf); )
            
                            SetFlag(pcrl->uFlags, CRLF_DIRTY);
                            bRet = TRUE;
                            }
                        break;
                        }
                    }

                // Is CRL's atomOutside a parent or equal of atomPath?
                if (Atom_IsParentOf(pcrl->atomOutside, atomPath))
                    {
                    // Yes; mark it
                    DEBUG_CODE( LPCTSTR pszDbg = Atom_GetName(atom); )
                    DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  Tagging CRL for %s (0x%lx)"), 
                        pszDbg, pcrl->hbrf); )
    
                    SetFlag(pcrl->uFlags, CRLF_DIRTY);
                    bRet = TRUE;

                    // Refresh this window?
                    *pbRefresh = Atom_IsParentOf(atomCabFolder, atom);
                    }

                Cache_DeleteItem(&g_cacheCRL, atom, FALSE, NULL, CRL_Free);    // Decrement count
                }
            }
Done:;
        }
    CRL_LeaveCS();
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Mark the entire cache dirty
Returns: --
Cond:    --
*/
void PUBLIC CRL_DirtyAll(
    int atomBrf)
    {
    CRL_EnterCS();
        {
        CRL  * pcrl;
        int atom;

        CRL_Iterate(atom)
            {
            pcrl = Cache_GetPtr(&g_cacheCRL, atom);
            ASSERT(pcrl);
            if (pcrl && pcrl->atomBrf == atomBrf)
                {
                DEBUG_CODE( LPCTSTR pszDbg = Atom_GetName(atom); )
            
                DEBUG_CODE( TRACE_MSG(TF_CACHE, TEXT("CACHE  Tagging CRL for %s (0x%lx)"), pszDbg, 
                    pcrl->hbrf); )
    
                SetFlag(pcrl->uFlags, CRLF_DIRTY);
                Cache_DeleteItem(&g_cacheCRL, atom, FALSE, NULL, CRL_Free);    // Decrement count
                }
            }
        }
    CRL_LeaveCS();
    }


