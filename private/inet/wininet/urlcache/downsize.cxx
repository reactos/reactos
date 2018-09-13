/*++
Copyright (c) 1997  Microsoft Corp.

Module Name: downsize.cxx

Abstract:

    Implementation of heuristic pruning and wholesale purge of cache index.

Author:

    Rajeev Dujari (rajeevd) 15-Apr-97

    RajeevD rewrote scoring and pruning algo, Aug-98.

--*/

#include <cache.hxx>

#ifdef BETA_LOGGING
#define SCAVENGER_TRACE
#define TRACE_FACTOR 99
#endif


BOOL // whether memory mapped index file was deleted
URL_CONTAINER::DeleteIndex (void)
{                                        
    BOOL fRetVal = FALSE;
    BOOL fMustUnlock;

    // Get the full path name of the cache directory.
    if (!LockContainer(&fMustUnlock))
        goto exit;

    CHAR szFullPath[MAX_PATH];
    memcpy(szFullPath, _UrlObjStorage->GetFullPathName(),
        _UrlObjStorage->GetFullPathNameLen() + 1);

    if (fMustUnlock)
    {
        UnlockContainer();
        fMustUnlock = FALSE;
    }

    // Delete the cache files not in use (index.dat is open by us)
    CFileMgr::DeleteCache (szFullPath);

    if (!(GetOptions() & INTERNET_CACHE_CONTAINER_NODESKTOPINIT))
        EnableCacheVu(szFullPath);


    if (!LockContainer(&fMustUnlock))
        goto exit;

#ifdef NUKE_CACHE_INDEX_FILE

    // If no handles are actively in use by this process,
    // attempt to shrink the index file.
    if (!AnyFindsInProgress(0) && !GlobalRetrieveUrlCacheEntryFileCount)
    {

        LONGLONG qwLimit = _UrlObjStorage->GetCacheLimit();
        fRetVal = _UrlObjStorage->Reinitialize();
        if (fRetVal)
        {
            _UrlObjStorage->SetCacheLimit (qwLimit);
            _FileManager->Init();
            // BUGBUG: call SetCacheSize with total not deleted by DeleteCache.

        }
    }

#endif

exit:
    if (fMustUnlock)
        UnlockContainer();
    return fRetVal;
}

// Weightings of various score components...
#define IDLETIME_WEIGHT     (60000)
#define EXPIRY_WEIGHT       ( 3000)
#define NUMACCESS_WEIGHT    ( 3000)

/*=======================================================================
ScoreEntry computes the score for the given url entry.

    The lower the score the more likely is an entry to be
    removed from the cache. Entries with higher scores are
    considered more useful.  Only the relative values matter.

    The components that contribute to the score are as follows...
        idle time since last access
        number of times accessed
        expiry, last-modified, and other sync factors
    They are weighted so that idle time predominates if the
    item has been accessed recently while older items are
    more easily influenced by the other factors.

    IDLE TIME is measured as number of days since last access,
    not rounded to an integer but including a fraction.  Then
    the score decays as 1/(days+1).  To illustrate:

        Elapsed Time    Rel. Score
        ============    ==========
        0                   60
        12 hours            40
        1 day               30
        1.5 days            24
        2 days              20
        5 days              10
        9 days               6
        29 days              2
        30-59 days           1
        60+ days             0

    NUMBER OF TIMES ACCESSED is a predictor of both the likelihood
    the item will ever be accessed again and the frequency of future
    access.  This subscore is scaled by (1 - 1/num).  For example:

        Num         Rel. Score
        ===         ==========
         1               0
         2              10
         4              15
        10              18
        20+             20

    EXPIRY in the future is worth full credit because we need
    not issue if-modified-since requests (except upon refresh.)

    Similarly, an item which is approaching SYNCSTATE_VOLATILE
    gets checked rarely and gets nearly full credit.  Items on
    the way to approaching this state get pro-rated credit.

    An expiry in the past is treated same has no expiry at all.

    An item gets half credit if last-modified-time is set.
    Otherwise any net hit would download  new content so the
    cache entry is of limited value.

    To summarize:

        Expiry  LastMod SyncState   Rel. Score
        ======  ======= =========   ==========
        future  n/a     n/a             14
        other   present static          13
        other   present image            8
        other   present volatile         7
        other   none    n/a              0

    We are agnostic about file size.  Pruning a larger file means
    we reclaim a lot of disk space, but it takes longer to download.
    Small files often waste a lot of disk space on a FAT partition,
    but incur the same fixed cost as downloading a large file.

Arguments:
    pEntry :  pointer to the Url entry.
    CurrentGmtTime : Current GMT time.

Return Value: DWORD score.
=======================================================================*/
DWORD ScoreEntry
(
    URL_FILEMAP_ENTRY* pEntry,
    LONGLONG CurrentGmtTime
)
{
    INET_ASSERT(pEntry->dwSig == SIG_URL);

    // Compute scored based on days since last access.

    // We're adding 15 minutes to the CurrentGmtTime to account for the continual 
    // readjustments to the pc's internal clock; this will handle occasional blips
    // (cases when the gmt is suddenly earlier than the LastAccessedTime, for example)
    CurrentGmtTime += (15*60*FILETIME_SEC);

    LONGLONG IdleTime = CurrentGmtTime - pEntry->LastAccessedTime;

    // In case the Last Accessed Time is later than the GMT, we want to protect against
    // a negative time
    if (IdleTime < 0)
    {
        IdleTime = 0;
    }

    DWORD dwScore = (DWORD) (((LONGLONG) IDLETIME_WEIGHT * FILETIME_DAY)
        / (IdleTime + FILETIME_DAY));

#ifdef UNIX
    {
       /* We don't want to delete items that were just created.
        * On Win32, because the InternetLockRequestFile will hold onto
        * the entries. This will not work on Unix because they use
        * InternetLockRequestFile uses CreateFile, which does not really
        * lock the file on unix, because of the lack of file handles.
        * 
        * So, just like in IE4, we will give a grace period for the cache
        * item.
        */
       #define UNIX_STICKY_SCORE 0L
       if (IdleTime < (1 * 60 * (LONGLONG)10000000))
          return UNIX_STICKY_SCORE;
    }
#endif /* UNIX */

    // Add to score based on number of times accessed.
    DWORD dwAccess = pEntry->NumAccessed;
    if (!dwAccess)
    {
        INET_ASSERT (pEntry->NumAccessed);
        dwAccess = 1;
    }
    dwScore += NUMACCESS_WEIGHT - NUMACCESS_WEIGHT/dwAccess;

    // Add to score based on expiry and syncstate.
    FILETIME ftExpireTime;
    DosTime2FileTime(pEntry->dostExpireTime, &ftExpireTime);
    if (FT2LL(ftExpireTime) > CurrentGmtTime)
        dwScore += EXPIRY_WEIGHT;
    else if (pEntry->LastModifiedTime)
    {
        // Add a bonus for having a last-modified time.
        dwScore += EXPIRY_WEIGHT / 2;

        // Add more as the item approaches auto sync mode.
        INET_ASSERT (pEntry->bSyncState <= SYNCSTATE_STATIC);
        dwScore += (EXPIRY_WEIGHT * pEntry->bSyncState)
            / (2 * (SYNCSTATE_STATIC + 1));
    }

    INET_ASSERT (dwScore <=
        IDLETIME_WEIGHT + EXPIRY_WEIGHT + NUMACCESS_WEIGHT);
    return dwScore;
}


/*=======================================================================
WalkLeakList attempts to delete files that we couldn't delete earlier.
========================================================================*/
BOOL URL_CONTAINER::WalkLeakList (void)
{
    BOOL fMustUnlock;
    LockContainer(&fMustUnlock);

    // Set loop variables to head of list.
    DWORD dwPrevOffset = OffsetFromPointer(_UrlObjStorage->GetPtrToLeakListOffset());
    DWORD dwCurrOffset, dwFirstItemOffset;
    _UrlObjStorage->GetHeaderData(CACHE_HEADER_DATA_ROOT_LEAK_OFFSET,
                                  &dwCurrOffset);

    // Validate offset and block signature.
    URL_FILEMAP_ENTRY* pEntry = (URL_FILEMAP_ENTRY*) PointerFromOffset (dwCurrOffset);
    if (_UrlObjStorage->IsBadOffset (dwCurrOffset)
        || pEntry->dwSig != SIG_LEAK)
    {
        INET_ASSERT(dwCurrOffset==0);
        
        // Replace the bad link with a terminator.
        _UrlObjStorage->SetHeaderData(CACHE_HEADER_DATA_ROOT_LEAK_OFFSET, 
                                      0);
        return fMustUnlock;
    }
    dwFirstItemOffset = dwCurrOffset;
    
    while (1)
    {
        // Extract full path of the file.
        // and attempt to delete the file.
        DWORD cb;
        TCHAR szFile[MAX_PATH];

        if (_FileManager->GetFilePathFromEntry(pEntry, szFile, &(cb = MAX_PATH))
            &&
            (!pEntry->NumReferences)
            &&
            (_FileManager->DeleteOneCachedFile
                (szFile, pEntry->dostFileCreationTime, pEntry->DirIndex)))
        {
            // Adjust cache usage.
            _UrlObjStorage->AdjustCacheSize(-RealFileSize(pEntry->dwFileSize));

            // Remove this item from the list.
            LPDWORD pdwPrev = (LPDWORD) PointerFromOffset (dwPrevOffset);
            *pdwPrev = pEntry->dwNextLeak;

            if (dwFirstItemOffset==dwCurrOffset)
            {
                _UrlObjStorage->SetHeaderData(CACHE_HEADER_DATA_ROOT_LEAK_OFFSET,
                                  pEntry->dwNextLeak);
            }
            if( dwCurrOffset != pEntry->dwNextLeak )
            {
                dwCurrOffset = pEntry->dwNextLeak;
            }
            else
            {
                // we have a circular list, break now
                // Replace the bad link with a terminator.
                dwCurrOffset = 0;
                LPDWORD pdwPrev = (LPDWORD) PointerFromOffset (dwPrevOffset);
                *pdwPrev = 0;
            }

            _UrlObjStorage->FreeEntry(pEntry);
        }
        else
        {
            // We don't have permission to delete this entry
            dwPrevOffset = OffsetFromPointer (&pEntry->dwNextLeak);
            if( dwCurrOffset != pEntry->dwNextLeak )
            {
                dwCurrOffset = pEntry->dwNextLeak;
            }
            else
            {
                // we have a circular list, break now
                // Replace the bad link with a terminator.
                dwCurrOffset = 0;
                LPDWORD pdwPrev = (LPDWORD) PointerFromOffset (dwPrevOffset);
                *pdwPrev = 0;
            }
        }
    
        // If the shutdown event signalled, call it quits.
        // Also, if we've reached the end of the list, quit
        if (InDllCleanup || (dwCurrOffset==0))
            break;

        pEntry = (URL_FILEMAP_ENTRY*) PointerFromOffset (dwCurrOffset);
        if (_UrlObjStorage->IsBadOffset (dwCurrOffset)
            || pEntry->dwSig != SIG_LEAK)
        {
            // Replace the bad link with a terminator.
            INET_ASSERT (FALSE);
            LPDWORD pdwPrev = (LPDWORD) PointerFromOffset (dwPrevOffset);
            *pdwPrev = 0;
            break;
        }
        pEntry->NumReferences++;

        // Relinquish the lock and time slice so other threads don't get starved.
        if (fMustUnlock)
        {
            UnlockContainer();
            fMustUnlock = FALSE;
        }

        SuspendCAP();
        Sleep (0);
        ResumeCAP();

        LockContainer(&fMustUnlock);
        _UrlObjStorage->GetHeaderData(CACHE_HEADER_DATA_ROOT_LEAK_OFFSET,
                                  &dwFirstItemOffset);
        pEntry = (URL_FILEMAP_ENTRY*) PointerFromOffset (dwCurrOffset);
        pEntry->NumReferences--;
    }

    return fMustUnlock;
}

/*=======================================================================
IsUrlEntryExemptFromScavenging filters out items exempt from scavenging.

Returns BOOL: TRUE indicating the item should not be scavenged.
========================================================================*/
BOOL URL_CONTAINER::IsUrlEntryExemptFromScavenging
(
    HASH_ITEM* pItem,
    URL_FILEMAP_ENTRY* pEntry,
    DWORD dwFilter,
    LONGLONG qwGmtTime,
    GroupMgr* pgm
)
{
    // If entry points to a store directory, ignore it.
    if ((pEntry->DirIndex == INSTALLED_DIRECTORY_KEY)
    || (pEntry->CacheEntryType & EDITED_CACHE_ENTRY))
    {
        return TRUE;
    }

    // If filter==0, trash everything, son.
    if (dwFilter==0)
    {
        return FALSE;
    }
    
    // If entry type excluded by filter, ignore it.
    if (pEntry->CacheEntryType & dwFilter)
        return TRUE;

    // If not deleting all entries, check for exemption from scavenging.
    if( pEntry->CacheEntryType & STICKY_CACHE_ENTRY)
    {
        // sticky + exemptDelta == 0 means sticky forever
        // because item must belong to non-purgeable group
        // or the cache entry type would not have sticky bit.
        if( !(pEntry->dwExemptDelta) )
            return TRUE;

        // sticky group == sticky forever! no exempt delta
        // needs to be looked.

        if( pEntry->dwGroupOffset )
        {
            if( pItem->HasMultiGroup() )
            {
                // multiple group
                // if there are other sticky groups attached to
                // this url entry, leave this entry alone
                if(!pgm->NoMoreStickyEntryOnList(pEntry->dwGroupOffset))
                    return TRUE;
            }
            else
            {
                // single group
                // if the group attached to this url entry is
                // sticky, leave this entry alone
                GROUP_ENTRY* pGroupEntry = NULL;
                pGroupEntry = _UrlObjStorage->ValidateGroupOffset
                    (pEntry->dwGroupOffset, pItem);
                if(pGroupEntry && IsStickyGroup(pGroupEntry->gid) )
                    return TRUE;
            }
        }

        // Skip over the item if it's within its exemption period.
        // FILETIME units are 100-ns ticks, exempt delta in seconds.

        LONGLONG qwExemptDelta = FILETIME_SEC * pEntry->dwExemptDelta;
        if (qwGmtTime < pEntry->LastAccessedTime + qwExemptDelta)
            return TRUE;
    }

    return FALSE;
}

/*=======================================================================
ScavengeItem deletes a cache entry and yields with the lock unowned.

Returns BOOL: FALSE if dll shutdown has been signalled.
========================================================================*/
BOOL URL_CONTAINER::ScavengeItem (HASH_ITEM* pItem, BOOL* pfMustUnlock)
{
    DeleteUrlEntry (HashGetEntry (pItem), pItem, SIG_DELETE);

    // If the shutdown event signalled, call it quits.
    if (InDllCleanup)
        return FALSE;

    // Relinquish the lock and time slice so other threads don't get starved.
    if (*pfMustUnlock)
    {
        UnlockContainer();
        *pfMustUnlock = FALSE;
    }

    SuspendCAP();
    Sleep (0);
    ResumeCAP();

    LockContainer(pfMustUnlock);
    return TRUE;
}


#define NUM_SCORE_ITEMS 100

//=======================================================================
#define FIND_MIN 1
#define FIND_MAX 0

PRIVATE SCORE_ITEM* FindMinOrMaxScoreItem
    (SCORE_ITEM* pScore, DWORD cScore, DWORD MinOrMax)
{
    INET_ASSERT (cScore);
    INET_ASSERT (MinOrMax == FIND_MIN || MinOrMax == FIND_MAX);

    SCORE_ITEM* pRet = pScore;
    DWORD dwScore = pScore->dwScore;

    for (DWORD iScore=1; iScore<cScore; iScore++)
    {
        pScore++;
        if ((dwScore < pScore->dwScore ? TRUE : FALSE) ^ MinOrMax)
        {
            pRet = pScore;
            dwScore = pScore->dwScore;
        }
    }

    return pRet;
}

//=======================================================================
PRIVATE void SwapScoreItems (SCORE_ITEM *p1, SCORE_ITEM *p2)
{
    SCORE_ITEM t;
    memcpy (&t, p1, sizeof(SCORE_ITEM));
    memcpy (p1, p2, sizeof(SCORE_ITEM));
    memcpy (p2, &t, sizeof(SCORE_ITEM));
}

//=======================================================================
PRIVATE void SortScoreItems (SCORE_ITEM* pScore, DWORD cScore)
{

    while (cScore > 1)
    {
        SCORE_ITEM *pMax =
            FindMinOrMaxScoreItem (pScore, cScore--, FIND_MAX);
        SwapScoreItems (pScore + cScore, pMax);
    }
}

//=======================================================================
void URL_CONTAINER::ScavengerDebugSpew
    (SCORE_ITEM* pScoreItem, LONGLONG* pqwDeleted)
{
    HASH_ITEM* pItem = (HASH_ITEM*)
        (*_UrlObjStorage->GetHeapStart() + pScoreItem->dwItemOffset);
    if (pScoreItem->dwHashValue == pItem->GetValue()
        && pScoreItem->dwHashOffset == pItem->dwOffset)
    {
        URL_FILEMAP_ENTRY* pEntry = HashGetEntry (pItem);
        char szBuf[1024];
        LPSTR pszOp;

        if (!pqwDeleted)
            pszOp = "IGNORE";
        else
        {
            pszOp = "DELETE";
            *pqwDeleted += RealFileSize (pEntry->dwFileSize);
        }

        wsprintf (szBuf, "%s %05d ", pszOp, pScoreItem->dwScore);
        OutputDebugString (szBuf);
        if (pqwDeleted)
        {
            wsprintf (szBuf, "%02d%% ", (*pqwDeleted * 100) / GetCacheLimit());
            OutputDebugString (szBuf);
        }
        PrintFileTimeInInternetFormat ((FILETIME*)
            &pEntry->LastAccessedTime , szBuf, sizeof(szBuf));
        OutputDebugString (szBuf);
        wsprintf (szBuf, " %s\n", ((LPSTR) pEntry) + pEntry->UrlNameOffset);
        OutputDebugString (szBuf);
    }
}


#ifndef SCAVENGER_TRACE
#define ScavengerTrace(x,y,z) { }
#else
#define ScavengerTrace(dwFactor, pScoreItem, pdwDel) \
    if (dwFactor==TRACE_FACTOR) {ScavengerDebugSpew(pScoreItem, pdwDel);}
#endif

/*========================================================================*/
DWORD URL_CONTAINER::FixupHandler (DWORD dwFactor, DWORD dwFilter)
{
    LOCK_CACHE();
    
    if (!g_pfnFixup)
    {
        // This is the first time we needed the handler; initialize.
        char szDll[MAX_PATH + 80];
        DWORD cbDll = sizeof(szDll);
        
        // Look up the fixup handler for the highest cache version installed.
        REGISTRY_OBJ roCache (HKEY_LOCAL_MACHINE, OLD_CACHE_KEY);
        if (ERROR_SUCCESS != roCache.GetStatus())
            goto err;
        if (ERROR_SUCCESS != roCache.GetValue (g_szFixup, (LPBYTE) szDll, &cbDll))
            goto err;
            
        LPSTR pszEntryPoint;

        // The dll name and entry point are delimited by a comma; tokenize.
        pszEntryPoint = StrChr (szDll, TEXT(','));
        if (!pszEntryPoint)
            goto err;
        *pszEntryPoint++ = 0;
        
        g_hFixup = LoadLibrary (szDll);
        if (!g_hFixup)
            goto err;
                
        g_pfnFixup = (PFN_FIXUP) GetProcAddress (g_hFixup, pszEntryPoint);
        if (!g_pfnFixup)
        {
            FreeLibrary (g_hFixup);
            goto err;
        }
    }

    UNLOCK_CACHE();
    
    return (*g_pfnFixup)
        (ENTRY_VERSION_CURRENT, _CachePath, _CachePrefix, 
        &InDllCleanup, dwFactor, dwFilter, NULL);

err:
    // We couldn't locate async fixup handler; fail gracefully.
    g_szFixup[0] = 0;
    UNLOCK_CACHE();
    return ERROR_INTERNET_INTERNAL_ERROR;
}


/*=======================================================================
Routine Description:

Arguments:
    Factor : amount of free space to make. Factor of 25 means delete
        sufficient files to make CacheSize <= .75 * CacheLimit.

The index does not maintain a list of items sorted by score because the cost
of scavenging would be amortized across update operations, which are
performed on a foreground thread.  Such a list would be doubly linked because
updating an item would change its score and probably change its rank.  If the
items were directly linked together, this would likely result in touching two
other random pages on update.  A lookaside list would be a better approach
but would still require hitting another page on update or increasing the size
of the lookup hash table.  Furthermore, the ranking would need to be strictly
LRU, or else we would have to a 16-bit score in the entry and an updated item
might not go to the head of the list and require some traversal.

The scavenger thread scores items on the fly.  It attempts to avoid a full
enumeration of the cache and sorting of the scores.  Instead, it attempts to
track items that fall below a cutoff score and delete the lowest-scoring
among this set, possibly before completing the enumeration.  Specifically, it
starts by enumerating 100 items and sorting them.  Since the rank of the
items is uniformly distributed, by definition, the score of the 10th lowest
item is an estimate of the 10th percentile.  Of course, deleting 10% of the
items in the cache is no guarantee 10% of disk usage will be reclaimed.
However hitting the low-water mark of 90% of cache quota is not a strict goal
and will probably get the cache under the quota.  Even if not, the scavenger
will be invoked again on the next update and establish a higher cutoff.

If this cutoff score proves to be too low, then it's possible the scavenger
will enumerate the entire cache without bringing it below quota, in which
case it will be restarted by the next cache update, probably with a higher
threshhold.  On the other hand, if the cutoff score is too high, then we
might end the enumeration early and delete some items in the 20th or even
30th percentiles.  The latter outcome seems better since we never promised to
be perfect anyway, so we bias the algorithm by picking the 20th lowest
item for the threshhold score.

Once the cutoff score is established, the enumeration continues.  The list is
not kept sorted.  If all of the items in the 100-item list are below the
cutoff, then the lowest-scoring one is deleted.  Otherwise the highest-scoring
item is merely removed from the list and forgotten.  The the next item in the
enumeration is added to the list.  If the enumeration completes without
reaching the target usage, then the lowest-scoring item is deleted until the
list is empty, even those items that fall above the cutoff, which after all was
too low.

After each file deletion, the scavenger thread yields without holding the
container lock.  Otherwise another thread wanting to acquire the lock would
block, wake up the scavenger thread, and switch back after the scavenger
unlocked.

Return Value: ERROR_SUCCESS
========================================================================*/
DWORD URL_CONTAINER::CleanupUrls(DWORD dwFactor, DWORD dwFilter)
{
    DWORD Error = ERROR_SUCCESS;

    // dwFactor must be between 1 and 100 inclusive.
    INET_ASSERT (dwFactor >= 1 && dwFactor <= 100);

    // If an uplevel fixup handler is installed, delegate.

    if (g_szFixup[0])
        return FixupHandler (dwFactor, dwFilter);
    
    // Special case purging entire container.
    BOOL fPurge = (dwFactor == 100 && dwFilter == 0);
    if (fPurge && DeleteIndex())
        return ERROR_SUCCESS;

    // First loop through the leaked files and try to delete them.
    BOOL fMustUnlock = WalkLeakList();

    // before index get nuked, we need to send out last notification
    // about the whole cache gets deleted
    DWORD dwHWnd = 0;
    DWORD dwUMsg = 0;
    DWORD dwNotifFilter = 0;

    GroupMgr gm;
    if( !gm.Init(this) )
    {
        INET_ASSERT(FALSE);
    }

    BOOL fLowDiskSpace = FALSE;
    
    _UrlObjStorage->GetHeaderData(
            CACHE_HEADER_DATA_NOTIFICATION_FILTER, &dwNotifFilter);
    if( dwNotifFilter & CACHE_NOTIFY_DELETE_ALL)
    {
        _UrlObjStorage->GetHeaderData( CACHE_HEADER_DATA_NOTIFICATION_HWND, &dwHWnd);
        _UrlObjStorage->GetHeaderData( CACHE_HEADER_DATA_NOTIFICATION_MESG, &dwUMsg);
    }

    // Calculate usage goal.
    LONGLONG qwQuota = _UrlObjStorage->GetCacheLimit();
    LONGLONG qwGoal = (qwQuota * (100 - dwFactor)) / 100;
    LONGLONG qwGmtTime;
    GetCurrentGmtTime ((FILETIME*) &qwGmtTime);

    DWORDLONG dlAvail = 0;
    if (GetDiskInfo(_CachePath, NULL, &dlAvail, NULL)
        &&
        (BOOL)(dlAvail <= (DWORDLONG)GlobalDiskUsageLowerBound))
    {
        fLowDiskSpace = TRUE;
        // We'll set the goal even lower, if the disk space falls below the 4 GIG threshold
        // qwResult contains how much disk space would be available with the current goal
        LONGLONG qwResult = dlAvail + (_UrlObjStorage->GetCacheSize() - qwGoal);
        if (qwResult < (LONGLONG)GlobalDiskUsageLowerBound)
        {
            qwGoal = _UrlObjStorage->GetCacheSize() - ((LONGLONG)(GlobalDiskUsageLowerBound - dlAvail));

            // At the very least, we'll preserve 128K (about three pages)
            if (qwGoal<(LONGLONG)(128*1024))
            {
                qwGoal = (LONGLONG)(128*1024);
            }
        }
    }

#ifdef SCAVENGER_TRACE

    // If we are simulating a scavenging, we accumulate the number
    // of bytes, adjusted for cluster slop, that would be reclaimed
    // if this were for real.  By setting the usage target to 0, we
    // also stress the scavenger to see how well it selects items
    // in edge cases where we just can't seem to delete enough.

    LONGLONG qwDeleted = 0;
    if (dwFactor == TRACE_FACTOR)
        qwGoal = 0;

#endif

    SCORE_ITEM ScoreList[NUM_SCORE_ITEMS];
    DWORD cScore = 0;  // number of valid entries in score list

    DWORD dwCutoffScore = 0;

    DWORD dwEnum = GetInitialFindHandle();

    // The loop code below is organized in two parts.
    // Part 1 - enum the cache to add another item to the list.
    // Part 2 - remove an item from a list, by throwing out a
    //   a high-scoring item or deleting a low-scoring item.

    // The looping occurs in 3 phases.
    // A. Do part 1 only until there are 100 items or enum is complete.
    // B. Do part 1 and part 2 until the enum is complete.
    // C. Do part 2 only until the list is empty.
    // Note that it's possible to skip directly from phase A to C.

    while (1) // until goal is met or score list is empty
    {

        // PART 1 OF LOOP: Enumerate another item from the cache.

        HASH_ITEM* pItem = HashGetNextItem
            (_UrlObjStorage, *_UrlObjStorage->GetHeapStart(), &dwEnum, fPurge);

        if (pItem)
        {
            // Validate offset.
            if (_UrlObjStorage->IsBadOffset (pItem->dwOffset))
            {
                pItem->MarkFree();
                continue;
            }

            // Get the signature.
            FILEMAP_ENTRY* pBlock = (FILEMAP_ENTRY*)
                (((LPBYTE) *_UrlObjStorage->GetHeapStart()) + pItem->dwOffset);

            if (pBlock->dwSig != SIG_URL)
            {
                if (fPurge && (pBlock->dwSig == SIG_REDIR))
                    _UrlObjStorage->FreeEntry (pBlock);
                else
                    INET_ASSERT (pBlock->dwSig == SIG_LEAK );

                pItem->MarkFree();
                continue;
            }

            // Filter out items exempt from scavenging.
            URL_FILEMAP_ENTRY* pEntry = (URL_FILEMAP_ENTRY*) pBlock;

            // The entry should not be from an uplevel cache, or
            // we ought to be deferring to its scavenger.
            INET_ASSERT (!(pEntry->bVerCreate & ENTRY_VERSION_NONCOMPAT_MASK));
            
            if (IsUrlEntryExemptFromScavenging
                (pItem, pEntry, dwFilter, qwGmtTime, &gm))
            {
#ifdef SCAVENGER_TRACE
                if (dwFactor == TRACE_FACTOR)
                {
                    char szBuf[1024];
                    wsprintf (szBuf, "EXEMPT %s\n",
                        ((LPSTR) pEntry) + pEntry->UrlNameOffset);
                    OutputDebugString (szBuf);
                }
#endif
                continue;
            }

            // If we are deleting all items, no need to score.
            if (dwFactor==100)
            {
                if (ScavengeItem (pItem, &fMustUnlock))
                    continue;
                else
                    goto done;
            }

            // If we've fallen below the 4MB threshold, we won't exempt anything from 
            // scavenging.

            // Otherwise, we look at the size of the item. If its size is greater than
            // whatever 90% of the cache quota is (arbitrary), then we won't scavenge it
            // for this session. 

            // For all other instances, we won't scavenge items we've seen in the past
            // ten minutes.

            if (!fLowDiskSpace)
            {
                if (((LONGLONG)pEntry->dwFileSize > (LONGLONG)((LONGLONG)(qwQuota * (LONGLONG)9)/(LONGLONG)10))
                    && (dwdwSessionStartTime < pEntry->LastAccessedTime))
                    continue;
                
                if (qwGmtTime < (pEntry->LastAccessedTime + (LONGLONG)(GlobalScavengeFileLifeTime*FILETIME_SEC)))
                    continue;
            }
            
            // Otherwise score the entry.
            SCORE_ITEM* pScoreItem = ScoreList + cScore;

            pScoreItem->dwScore = ScoreEntry (pEntry, qwGmtTime);

#ifdef UNIX
            if (!pScoreItem->dwScore)
               continue;
#endif /* UNIX */

            // Add to the list.
            pScoreItem->dwItemOffset =          // 64BIT
                (DWORD) ((LPBYTE) pItem - *_UrlObjStorage->GetHeapStart());
            pScoreItem->dwHashValue  = pItem->GetValue();
            pScoreItem->dwHashOffset = pItem->dwOffset;

             // Check if list is full.
            if (++cScore != NUM_SCORE_ITEMS)
                continue;

            if (!dwCutoffScore)
            {
                // Establish a cutoff score.
                SortScoreItems (ScoreList, cScore);
                DWORD nIndex; // of item used as cutoff

                switch (dwFactor)
                {
                    case DEFAULT_CLEANUP_FACTOR:
#ifdef SCAVENGER_TRACE
                    case TRACE_FACTOR:
#endif
                        nIndex = NUM_SCORE_ITEMS / 5;
                        break;

                    default:
                        nIndex = (NUM_SCORE_ITEMS * dwFactor) / 100;
                        break;
                }

                dwCutoffScore = ScoreList[nIndex].dwScore;
            }
        } // end if (pItem)

        // PART 2 OF LOOP: remove an item from the list

        // If enumeration complete and list is empty, then
        // break out of the infinite loop.
        if (!cScore)
            break;

        SCORE_ITEM *pScoreItem;

        // Is the score list full?
        if (cScore == NUM_SCORE_ITEMS)
        {
            // Find the highest scoring item.
            pScoreItem = FindMinOrMaxScoreItem
                (ScoreList, NUM_SCORE_ITEMS, FIND_MAX);
            if (pScoreItem->dwScore > dwCutoffScore)
            {
                ScavengerTrace (dwFactor, pScoreItem, NULL);

                // Some of the items are above the cutoff score.
                // Remove the highest-scoring item from the list
                // by swapping it to the end and reducing count.
                cScore--;
                SwapScoreItems (pScoreItem, ScoreList + cScore);
                continue;
            }
        }

        // Either the score list isn't full or all of the items
        // are below the cutoff score.  Delete lowest scoring item.
        pScoreItem = FindMinOrMaxScoreItem (ScoreList, cScore, FIND_MIN);

        // We yield the lock between deletes, so do some sanity
        // checking before attemptint to delete the item.
        pItem = (HASH_ITEM*)
            (*_UrlObjStorage->GetHeapStart() + pScoreItem->dwItemOffset);
        if (pScoreItem->dwHashValue == pItem->GetValue()
            && pScoreItem->dwHashOffset == pItem->dwOffset)
        {
            ScavengerTrace (dwFactor, pScoreItem, &qwDeleted);

            if (!ScavengeItem (pItem, &fMustUnlock))
                goto done;

            // If we met our goal, call it quits.
            if (dwFactor != 100 && _UrlObjStorage->GetCacheSize() < qwGoal)
                break;
        }

        // Remove the lowest-scoring item from the list.
        cScore--;
        SwapScoreItems (pScoreItem, ScoreList + cScore);

    } // end while (1)

    if( dwHWnd && dwUMsg && IsWindow((HWND)dwHWnd) )
    {
        PostMessage(
            (HWND)dwHWnd,
            (UINT)dwUMsg,
            (WPARAM)CACHE_NOTIFY_DELETE_ALL,
            (LPARAM)0
        );
    }

done:
    if (fMustUnlock)
        UnlockContainer();
    return ERROR_SUCCESS;
}

VOID CacheScavenger(LPVOID Parameter)
/*++

Routine Description:

    This function is the main  function for the cache management scavenger
    thread. This function performs verious time critical operations.

Arguments:

    NONE.

Return Value:

    NONE

--*/
{
    DWORD Error;
    DWORD WaitStatus;

    //StartCAP();

    // Set a global to indicate the thread is no longer suspended.
    LOCK_CACHE();
    if (!InDllCleanup)
    {
        UNLOCK_CACHE();

        // Why aren't we locking in this case?
        
        // Attempt to reduce the cache usage below the quota.
        GlobalUrlContainers->CleanupUrls (NULL, DEFAULT_CLEANUP_FACTOR, 0);

        // Clear a global to indicate the scavenger thread has exited.
        InterlockedDecrement(&GlobalScavengerRunning);

        LOCK_CACHE();
    }
    UNLOCK_CACHE();

    //StopCAP();
}


void LaunchScavenger (void)
{
#ifdef unix
    INET_ASSERT(!g_ReadOnlyCaches);
#endif /* unix */

    LOCK_CACHE(); 

    // only if scavenger is not already running.
    if (!InterlockedIncrement(&GlobalScavengerRunning))
    {
        // don't fire off new thread. Just queue scavenger as work item for
        // thread pool
        SHQueueUserWorkItem((LPTHREAD_START_ROUTINE)CacheScavenger,
                            NULL,
                            0,
                            (DWORD_PTR)0,
                            (DWORD_PTR *)NULL,
                            NULL,
                            0
                            );
    }
    else
    {
        InterlockedDecrement(&GlobalScavengerRunning);
    }
    
    UNLOCK_CACHE();
}
