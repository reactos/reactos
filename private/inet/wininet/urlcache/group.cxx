/*++
Copyright (c) 1998  Microsoft Corporation

Module Name:  group.hxx

Abstract:

    Manages cache group.
    
Author:
    Danpo Zhang (DanpoZ) 02-08-98
--*/

#include <cache.hxx>

GroupMgr::GroupMgr()
{
    _pContainer = NULL;
}

GroupMgr::~GroupMgr()
{
    if( _pContainer )
    {
        _pContainer->Release(FALSE);
    }
}

BOOL
GroupMgr::Init(URL_CONTAINER* pCont)
{
    BOOL fRet = TRUE;

    if( pCont )
    {
        _pContainer = pCont;
        _pContainer->AddRef();
    }
    else
    {
        SetLastError(ERROR_INTERNET_INTERNAL_ERROR);
        fRet = FALSE;
    }

    return fRet;
}

DWORD
GroupMgr::CreateGroup(DWORD dwFlags, GROUPID* pGID)
{
    INET_ASSERT(_pContainer);
    INET_ASSERT(pGID);

    BOOL            fMustUnlock;
    DWORD           dwError;
    GROUP_ENTRY*    pGroupEntry = NULL;

    *pGID = 0;

    if( !_pContainer->LockContainer(&fMustUnlock) )
    {
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
        goto exit;    
    }

    if( dwFlags & CACHEGROUP_FLAG_GIDONLY )
    {
        // only needs to return GID, no group needs to be created
        *pGID = ObtainNewGID();
        if( *pGID )
            dwError = ERROR_SUCCESS;
        else
            dwError = ERROR_INTERNET_INTERNAL_ERROR;
            

        goto exit;
    }

    //
    // find the first available entry by using FindEntry()
    // passing gid = 0 means looking for empty entry 
    // passing TRUE means create new page if no entry available
    //
    dwError = FindEntry(0, &pGroupEntry, TRUE );
    if( dwError != ERROR_SUCCESS )
    {
        goto exit;
    }

    // get a new gid
    *pGID = ObtainNewGID();

    if( *pGID )
    {
        // insert gid into the first available entry
        
        // set the sticky bit for non purgable group
        if( dwFlags & CACHEGROUP_FLAG_NONPURGEABLE )
        {
            *pGID = SetStickyBit(*pGID);
        }

        pGroupEntry->gid = *pGID;
        pGroupEntry->dwGroupFlags = dwFlags;
        dwError = ERROR_SUCCESS;
    } 
   
exit: 
    if( fMustUnlock )
    {
        _pContainer->UnlockContainer();
    }

    return dwError;
}


DWORD
GroupMgr::CreateDefaultGroups()
{
    
    INET_ASSERT(_pContainer);

    BOOL            fMustUnlock;
    DWORD           dwError;
    GROUP_ENTRY*    pGroupEntry = NULL;
    DWORD           dwOffsetHead = 0;

    if( !_pContainer->LockContainer(&fMustUnlock) )
    {
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
        goto exit;    
    }

    if(    GetHeaderData( CACHE_HEADER_DATA_ROOTGROUP_OFFSET, &dwOffsetHead)
        && dwOffsetHead )
    {
        BOOL fBadHead = FALSE;

        // dwOffsetHead may point to a page which has not actually mapped in
        if( _pContainer->_UrlObjStorage->IsBadGroupOffset(dwOffsetHead) ) 
        {
            fBadHead = TRUE;
        }
        else
        {
            
            // if offset is too big, invalid
            FILEMAP_ENTRY* pFM = NULL;

            pFM = (FILEMAP_ENTRY*) 
                    (*_pContainer->_UrlObjStorage->GetHeapStart() + 
                    dwOffsetHead - sizeof(FILEMAP_ENTRY) );                                   
            if(pFM->dwSig != SIG_ALLOC || !pFM->nBlocks )
            {
                fBadHead = TRUE;
            }
        }
            
        if( fBadHead )
        {
            // dwOffsetHead is invalid, reset!
            SetHeaderData(CACHE_HEADER_DATA_ROOTGROUP_OFFSET, 0);
        }
    }

    // if already created, just return success
    dwError = FindEntry(CACHEGROUP_ID_BUILTIN_STICKY, &pGroupEntry, FALSE);
    if( dwError == ERROR_SUCCESS )
    {
        goto exit;
    }

    //
    // not found, need to create new default groups
    //
    // find the first available entry by using FindEntry()
    // passing gid = 0 means looking for empty entry 
    // passing TRUE means create new page if no entry available
    //
    dwError = FindEntry(0, &pGroupEntry, TRUE );
    if( dwError != ERROR_SUCCESS )
    {
        goto exit;
    }

    // set the sticky bit for non purgable group
    pGroupEntry->gid = CACHEGROUP_ID_BUILTIN_STICKY;
    pGroupEntry->dwGroupFlags = CACHEGROUP_FLAG_NONPURGEABLE;
    dwError = ERROR_SUCCESS;
   
exit: 
    if( fMustUnlock )
    {
        _pContainer->UnlockContainer();
    }

    return dwError;
}


DWORD
GroupMgr::DeleteGroup(GROUPID gid, DWORD dwFlags)
{
    INET_ASSERT(_pContainer);
    INET_ASSERT(gid);

    BOOL                fMustUnlock;
    DWORD               dwError;
    GROUP_ENTRY*        pGroupEntry = NULL;
    GROUP_DATA_ENTRY*   pData = NULL;
    DWORD               hUrlFindHandle = 0;
    URL_FILEMAP_ENTRY*  pUrlEntry = 0;
    DWORD               dwFindFilter;
    HASH_ITEM*          pItem = NULL; 


    if( !_pContainer->LockContainer(&fMustUnlock) )
    {
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
        goto exit;    
    }


    // find the first available entry
    dwError = FindEntry(gid, &pGroupEntry, FALSE);
    if( dwError != ERROR_SUCCESS )
    {
        goto exit;
    }
     

    // Look for all the url associated with this group
    // mark the groupid to 0
    hUrlFindHandle = _pContainer->GetInitialFindHandle();       

    // set up find filter (do not care about cookie/history)
    dwFindFilter = URLCACHE_FIND_DEFAULT_FILTER 
                    & ~COOKIE_CACHE_ENTRY 
                    & ~URLHISTORY_CACHE_ENTRY;
    
    //
    // loop find all url belongs to this group
    // WARNING: this can be slow!
    //
    do 
    {
        // next url in this group
        pUrlEntry = (URL_FILEMAP_ENTRY*)
                        _pContainer->_UrlObjStorage->FindNextEntry( 
                                &hUrlFindHandle, dwFindFilter, gid); 

        if( pUrlEntry )
        {
            INET_ASSERT(hUrlFindHandle);
            pItem = (HASH_ITEM*)(
                    (LPBYTE) *_pContainer->_UrlObjStorage->GetHeapStart() +
                    hUrlFindHandle );

            if( pItem->HasMultiGroup() )
            {
                //
                // examing the group list and remove this group
                // from the list
                //
                DWORD       dwNewHeaderOffset       = pUrlEntry->dwGroupOffset;
                DWORD       dwGroupEntryOffset      = PtrDiff32(pGroupEntry, *_pContainer->_UrlObjStorage->GetHeapStart());

                //
                // find the to be deleted group entry in the list
                // of groups associated with this url, we need to
                // fix this by removing the to be dead group from 
                // the list
                //
                DWORD Error = RemoveFromGroupList(
                    pUrlEntry->dwGroupOffset, 
                    dwGroupEntryOffset,
                    &dwNewHeaderOffset 
                );
                    
            
                //
                // found the entry and head offset has been changed
                //
                if( Error == ERROR_SUCCESS && 
                    dwNewHeaderOffset != pUrlEntry->dwGroupOffset )
                {
                    pUrlEntry->dwGroupOffset = dwNewHeaderOffset;
               
                    // 
                    // no more group associated with this url 
                    // let's update the hash flags 
                    //
                    if( !dwNewHeaderOffset )
                    {
                        pItem->ClearMultGroup();
                        pItem->ClearGroup();
                    }
                }

                // sticky bit
                if(!pUrlEntry->dwExemptDelta && IsStickyGroup(gid) )
                {
                    //
                    // unset sticky bit for this url IFF 
                    // 1) we are about to delete the last group of this url
                    // 2) there is no more sticky group associated with this
                    //    url other than the to be deleted group
                    //
                    if( !pUrlEntry->dwGroupOffset ||
                        (  pUrlEntry->dwGroupOffset &&
                           NoMoreStickyEntryOnList(pUrlEntry->dwGroupOffset)))
                    {
                    
                        _pContainer->UpdateStickness(
                            pUrlEntry,
                            URLCACHE_OP_UNSET_STICKY,
                            hUrlFindHandle        
                        );
                    }
                }
            }
            else
            {
                //
                // do not move the url entry now, so we just
                // need to reset the GroupOffset and re-exam the
                // stick bit
                //
                pUrlEntry->dwGroupOffset = 0;

                // sticky bit
                if(!pUrlEntry->dwExemptDelta && IsStickyGroup(gid) )
                {

                    _pContainer->UpdateStickness(
                        pUrlEntry,
                        URLCACHE_OP_UNSET_STICKY,
                        hUrlFindHandle        
                    );
                }

            }


            if( dwFlags & CACHEGROUP_FLAG_FLUSHURL_ONDELETE)
            {
                //
                // Container's DeleteUrlEntry method takes two 
                // param, the url entry and hash item.
                // The hUrlFindHandle actually contains the
                // offset of the Hash Item, so we can get 
                // the hash item from there. 
                //

                // if this url belongs to other groups, 
                // do not delete it
                if( !pItem->HasMultiGroup() )
                {
                    _pContainer->DeleteUrlEntry(pUrlEntry, pItem, SIG_DELETE);
                }
            }

        } // find next url
    } while( pUrlEntry);
                    
    // if data entry exists, we should free them as well 
    if( pGroupEntry->dwGroupNameOffset )
    {
        dwError = FindDataEntry(pGroupEntry, &pData, FALSE); 
        if( dwError == ERROR_SUCCESS )
        {
            FreeDataEntry(pData);
        }
    }

    memset(pGroupEntry, 0, sizeof(GROUP_ENTRY) );
    dwError = ERROR_SUCCESS;

exit: 
    if( fMustUnlock )
    {
        _pContainer->UnlockContainer();
    }

    return dwError;
}



DWORD
GroupMgr::GetGroup(
    GROUPID                             gid, 
    DWORD                               dwAttrib, 
    INTERNET_CACHE_GROUP_INFOA*         pOutGroupInfo, 
    DWORD*                              pdwOutGroupInfoSize
)
{
    INET_ASSERT(_pContainer);
    INET_ASSERT(gid && pOutGroupInfo && pdwOutGroupInfoSize);

    BOOL            fMustUnlock;
    DWORD           dwError;
    GROUP_ENTRY*    pGroupEntry = NULL;

    if( !_pContainer->LockContainer(&fMustUnlock) )
    {
        dwError = ERROR_INTERNET_INTERNAL_ERROR; 
        goto exit;    
    }

    
    *pdwOutGroupInfoSize = 0;

    // find the entry
    dwError = FindEntry(gid, &pGroupEntry, FALSE); 
    if( dwError != ERROR_SUCCESS )
    {
        goto exit;
    }

    // init out param
    memset(pOutGroupInfo, 0, sizeof(INTERNET_CACHE_GROUP_INFOA) );

    // copy over GROUP_ENTRY -> GROUP_INFO
    Translate(
            dwAttrib,
            pOutGroupInfo, 
            pGroupEntry, 
            GROUP_ENTRY_TO_INFO, 
            pdwOutGroupInfoSize 
    ); 
    dwError = ERROR_SUCCESS; 

exit: 
    if( fMustUnlock )
    {
        _pContainer->UnlockContainer();
    }

    return dwError;
}


DWORD
GroupMgr::SetGroup(
    GROUPID                             gid, 
    DWORD                               dwAttrib, 
    INTERNET_CACHE_GROUP_INFOA*         pGroupInfo 
)
{
    INET_ASSERT(_pContainer);
    INET_ASSERT(pGroupInfo && gid);

    BOOL  fMustUnlock;
    DWORD dwError;
    GROUP_ENTRY* pGroupEntry;

    if( !_pContainer->LockContainer(&fMustUnlock) )
    {
        dwError = GetLastError();
        goto Cleanup;    
    }

    pGroupEntry = NULL;

    INET_ASSERT(pGroupInfo);

    if( dwAttrib & ~(CACHEGROUP_READWRITE_MASK) ) 
    {
        //
        // read only fields are being requested
        //
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if( (dwAttrib & CACHEGROUP_ATTRIBUTE_GROUPNAME) &&
        (strlen(pGroupInfo->szGroupName) >= GROUPNAME_MAX_LENGTH ) ) 
    {
        //
        // name too long, exceed the buffer limit 
        //
        dwError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // find the entry
    dwError = FindEntry(gid, &pGroupEntry, FALSE);
    if( dwError != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    // copy over GROUP_INFO -> GROUP_ENTRY
    Translate(
            dwAttrib,
            pGroupInfo, 
            pGroupEntry, 
            GROUP_INFO_TO_ENTRY, 
            0 
    ); 
    dwError = ERROR_SUCCESS;
    
Cleanup:
    if( fMustUnlock )
    {
        _pContainer->UnlockContainer();
    }

    return dwError; 
}


DWORD
GroupMgr::GetNextGroup(
    DWORD*                          pdwLastItemOffset, 
    GROUPID*                        pOutGroupId
)
{
    INET_ASSERT(_pContainer);
    INET_ASSERT(pOutGroupId);

    BOOL            fMustUnlock;
    BOOL            fEndOfGroups;
    GROUP_ENTRY*    pGroupEntry;
    DWORD           dwNewOffset;
    DWORD           dwError;

    if( !_pContainer->LockContainer(&fMustUnlock) )
    {
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
        fEndOfGroups = TRUE;
        goto Cleanup;    
    }

    pGroupEntry = NULL;
    dwNewOffset = 0;
    fEndOfGroups = FALSE;

    if( *pdwLastItemOffset == 0 )
    {
        // get root
        dwError = FindRootEntry(&pGroupEntry, FALSE );
        if( dwError != ERROR_SUCCESS )
        {
            //
            // new find and we can not get the root entry
            // this means there are no group at all. 
            //
            fEndOfGroups = TRUE;
            goto Cleanup;
        }
    } // IF: no previous offset, this is a new Find 

    else if( *pdwLastItemOffset == OFFSET_NO_MORE_GROUP )
    {
        // this group of search has completed already
        fEndOfGroups = TRUE;
        dwError = ERROR_FILE_NOT_FOUND;
        goto Cleanup;

    } // ELSE IF: previous FindNext has already reached the end of the groups 

    else
    {
        //
        // use the offset to jump to the last returned item's entry  
        //
        pGroupEntry = (GROUP_ENTRY*) 
            (*_pContainer->_UrlObjStorage->GetHeapStart() + *pdwLastItemOffset);                                   
        //
        // one step forward 
        //
        INET_ASSERT(pGroupEntry);                      // can't be null
        INET_ASSERT( !IsIndexToNewPage(pGroupEntry) ); // can't be index item
        pGroupEntry++;

    } // ELSE: walk to the item which has been returned by previous FindNext()


    // loop for next entry 
    while(pGroupEntry)
    {
        //
        // if this entry is the last one of the page
        // it contains offset pointing to the next page
        //
        if( IsIndexToNewPage(pGroupEntry) )
        {
            //
            // BUGBUG
            // we currently use dwFlags to indicating if
            // this is pointing to the next offset
            //
            if( pGroupEntry->dwGroupFlags )
            {
                //
                // walk to next page
                //
                pGroupEntry = (GROUP_ENTRY*)
                        (   *_pContainer->_UrlObjStorage->GetHeapStart() 
                          + pGroupEntry->dwGroupFlags );                                   
            } // IF: index entry point to next page

            else
            {
                //
                // we are done 
                //
                fEndOfGroups = TRUE;
                dwError = ERROR_FILE_NOT_FOUND;
                break; 

            } // ELSE: index page contains nothing (this is the last page)

        } // special case: current entry is the index(point to next page)


        // 
        // using gid to test if the entry is empty, if not, 
        // walk to the next entry  
        //
        if( !pGroupEntry->gid )
        {
            pGroupEntry++;
        } 
        else
        {
            break;    
        }

    } // while(pGroupEntry)
    

Cleanup:
    // update LastItemOffset
    if( !fEndOfGroups )
    {
        LPBYTE      lpbBase = *_pContainer->_UrlObjStorage->GetHeapStart(); 
        dwNewOffset = PtrDiff32(pGroupEntry, lpbBase);
        *pdwLastItemOffset = dwNewOffset;

        // copy over GROUP_ENTRY -> GROUP_INFO
        *pOutGroupId = pGroupEntry->gid;
        dwError = ERROR_SUCCESS;

    } // IF:  find the item

    else
    {
        *pdwLastItemOffset = OFFSET_NO_MORE_GROUP;
        dwError = ERROR_FILE_NOT_FOUND;
    } // ELSE: not find 

    if( fMustUnlock )
    {
        _pContainer->UnlockContainer();
    }

    return dwError;
}


DWORD
GroupMgr::FindRootEntry(
    GROUP_ENTRY** ppOut,        // OUT: first empty entry
    BOOL fCreate                // allocate new page if needed
)
{
    INET_ASSERT(ppOut);
    *ppOut = NULL;
    
    GROUPS_ALLOC_FILEMAP_ENTRY* pPage = NULL;
    DWORD                       dwError;
    DWORD                       dwOffsetToRootEntry = 0;

    // get base offset 
    if( GetHeaderData( CACHE_HEADER_DATA_ROOTGROUP_OFFSET, &dwOffsetToRootEntry))
    {
        if( !dwOffsetToRootEntry && fCreate )
        {
            dwError = CreateNewPage(&dwOffsetToRootEntry, TRUE);

            if( dwError != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
        } 
        else if( !dwOffsetToRootEntry && !fCreate )
        {
            //
            // there is no offset infomation on the mem file 
            // however, the flag says do not create a new page
            // failure is the only option here
            //
            dwError = ERROR_FILE_NOT_FOUND;
            goto Cleanup;
        } 

    } // IF: retrieve base offset

    else
    {
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup; 

    } // ELSE: failed to get base offset
    

    // 
    // At this point, we should either:
    //  1. retrieved valid dwOffsetToRootEntry or 
    //  2. get the new dwOffsetToRootEntry via CreateNewPage() call  
    //
    INET_ASSERT( dwOffsetToRootEntry );
    *ppOut =  (GROUP_ENTRY*) 
        ( *_pContainer->_UrlObjStorage->GetHeapStart() + dwOffsetToRootEntry);                                   
    dwError = ERROR_SUCCESS;

Cleanup:
    return dwError; 
}



DWORD
GroupMgr::FindEntry(
    GROUPID         gid,          // gid, 0 means find first empty seat
    GROUP_ENTRY**   ppOut,        // OUT: entry with gid specified
    BOOL            fCreate       // allocate new page if needed 
                                  // (applied for searching empty seat only)
)
{
    INET_ASSERT(ppOut);

    // fCreate can only be associated with gid == 0
    INET_ASSERT( (fCreate && !gid ) || (!fCreate && gid ) );

    GROUP_ENTRY*    pGroupEntry = NULL;
    DWORD           dwError;

    // get Root Entry
    dwError = FindRootEntry(&pGroupEntry, fCreate);
    if( dwError != ERROR_SUCCESS )
    {
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    } // failed to get the root entry


    INET_ASSERT(pGroupEntry); // pGroupEntry should be available now

    while(1)
    {
        // special case for end of this page
        if( IsIndexToNewPage(pGroupEntry) )
        {
            //
            // BUGBUG
            // we currently use the dwFlags to indicating
            // if this is pointing to the next offset
            //
            if( pGroupEntry->dwGroupFlags )
            {
                // walk to next page
                pGroupEntry = (GROUP_ENTRY*)
                        (   *_pContainer->_UrlObjStorage->GetHeapStart()
                           + pGroupEntry->dwGroupFlags );

            } // IF: index entry points to next page
    
            else if( fCreate)
            {
//////////////////////////////////////////////////////////////////
// BEGIN WARNING: The file might be grown and remapped, so all  //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////

                DWORD dwOffsetToFirstEntry = 0;
                LPBYTE  lpbBase = NULL;

                // remember the old offset for pGroupEntry
                DWORD_PTR dwpEntryOffset = PtrDifference(pGroupEntry, *_pContainer->_UrlObjStorage->GetHeapStart());

                // create new page!
                dwError = CreateNewPage(&dwOffsetToFirstEntry, FALSE);
                if( dwError != ERROR_SUCCESS )
                {
                    goto Cleanup;
                }

                // recalculate pGroupEntry using the offset remembered 
                lpbBase = *_pContainer->_UrlObjStorage->GetHeapStart(); 
                pGroupEntry = (GROUP_ENTRY*)(lpbBase + dwpEntryOffset);

//////////////////////////////////////////////////////////////////
// END WARNING: The file might be grown and remapped, so all    //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////

                //
                // pGroupEntry currently is the index item, insert 
                // the offset of the first item to the newly created page
                //
                pGroupEntry->dwGroupFlags = dwOffsetToFirstEntry;

                // walk to the new page 
                pGroupEntry = (GROUP_ENTRY*)(lpbBase + dwOffsetToFirstEntry);


            } // ELSE IF: index entry not point to new page, fCreate is
              //          set, a new page is being created  

            else
            {
                // this is the end of all groups, item still not found, 
                dwError = ERROR_FILE_NOT_FOUND;
                break;

            } // ELSE: index entry not point to new page, fCreate not set

        } // IF: this entry is an index entry


        //
        // now pGroupEntry must point to a normal group entry 
        //
        INET_ASSERT( !IsIndexToNewPage(pGroupEntry) );

        if( pGroupEntry->gid != gid )
        {
            // not found, walk to next entry
            pGroupEntry++;
        } 
        else
        {
            // found entry
            dwError = ERROR_SUCCESS;
            break;    
        }

    } // WHILE: (loop over all page)

    
Cleanup:
    if( dwError == ERROR_SUCCESS )
    {
        *ppOut = pGroupEntry;
    }
    else
    {
        *ppOut = NULL;
    }

    return dwError;
}

DWORD
GroupMgr::CreateNewPage(DWORD* dwOffsetToFirstEntry, BOOL fIsFirstPage)
{
    DWORD                           dwError;
    GROUPS_ALLOC_FILEMAP_ENTRY*     pPage = NULL;
    DWORD cbSize = sizeof(GROUPS_ALLOC_FILEMAP_ENTRY);

    pPage = (GROUPS_ALLOC_FILEMAP_ENTRY*)
            _pContainer->_UrlObjStorage->AllocateEntry(cbSize);


    if( pPage )
    {
        // clean up allocated page
        cbSize = PAGE_SIZE_FOR_GROUPS;    
        memset(pPage->pGroupBlock, 0, cbSize );

        // calculate the group base offset 
        LPBYTE lpbBase = *_pContainer->_UrlObjStorage->GetHeapStart(); 

        *dwOffsetToFirstEntry = PtrDiff32(pPage->pGroupBlock, lpbBase);

        //
        // mark the last entry as index to next page
        // (gid == GID_INDEX_TO_NEXT_PAGE) is the mark, 
        // the actual offset is stored at dwGroupFlags field
        //
        GROUP_ENTRY*    pEnd = (GROUP_ENTRY*) pPage->pGroupBlock;
        pEnd = pEnd + (GROUPS_PER_PAGE - 1);
        pEnd->gid = GID_INDEX_TO_NEXT_PAGE;

        if( fIsFirstPage )
        {
            //
            // for first page, we would have to set the offset 
            // back to the CacheHeader 
            //
            if( !SetHeaderData( 
                    CACHE_HEADER_DATA_ROOTGROUP_OFFSET, *dwOffsetToFirstEntry))
            {
                // free allocated page
                _pContainer->_UrlObjStorage->FreeEntry(pPage);
        
                // set error and go
                *dwOffsetToFirstEntry = 0;
                dwError = ERROR_INTERNET_INTERNAL_ERROR;
                goto Cleanup;

            } // IF: failed to set the offset 
        }

        // return the offset to the first entry of the new page
        dwError = ERROR_SUCCESS;

    } // IF: Allocate new page succeed

    else
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
    } // ELSE: failed to allocate new page

Cleanup:
    return dwError;
}



GROUPID
GroupMgr::ObtainNewGID()
{
    SYSTEMTIME  st;
    DWORD   dwC[2] = {0, 0};
    GROUPID gid = 0;

    // get counter from index file
    if( GetHeaderData(CACHE_HEADER_DATA_GID_LOW,  &dwC[0]) &&
        GetHeaderData(CACHE_HEADER_DATA_GID_HIGH, &dwC[1]) )
    {
        if( !dwC[0] && !dwC[1] )
        {
            // need to get the current system time
            GetSystemTime( &st );
            SystemTimeToFileTime(&st, (FILETIME*)dwC);

        } // IF: counter not initialized 

        else
        {
            // increment
            if( dwC[0] != 0xffffffff )
            {
                dwC[0] ++;
            }
            else
            {
                dwC[0] = 0;
                dwC[1] ++;
            }
        } // ELSE: counter initialized

        // send data back to cache
        if( SetHeaderData(CACHE_HEADER_DATA_GID_LOW,  dwC[0] ) &&
            SetHeaderData(CACHE_HEADER_DATA_GID_HIGH, dwC[1] ) ) 
        {
            //memcpy(&gid, dwC, sizeof(GROUPID) );
            gid = *((GROUPID *)dwC); 
        } 
    } 
    
    // apply the mask to newly created gid
    // the first 4 bits are reserved (one bit is used for stickness)  
    return (gid & GID_MASK); 
}


BOOL
GroupMgr::Translate(
    DWORD                           dwAttrib,
    INTERNET_CACHE_GROUP_INFOA*     pGroupInfo,
    GROUP_ENTRY*                    pGroupEntry, 
    DWORD                           dwFlag,
    DWORD*                          pdwSize                           
) 
{
    INET_ASSERT(pGroupInfo && pGroupEntry);
    BOOL fRet = TRUE;
    GROUP_DATA_ENTRY*   pData = NULL;
    DWORD               dwError;

    if( dwFlag == GROUP_ENTRY_TO_INFO )
    {
        INET_ASSERT(pdwSize);

        // clear
        memset(pGroupInfo, 0, sizeof(INTERNET_CACHE_GROUP_INFOA) );
        *pdwSize = 0;

        // basic entries 
        if( dwAttrib & CACHEGROUP_ATTRIBUTE_BASIC )
        {
            pGroupInfo->dwGroupSize  = sizeof(INTERNET_CACHE_GROUP_INFOA);
            pGroupInfo->dwGroupFlags = pGroupEntry->dwGroupFlags;
            pGroupInfo->dwGroupType  = pGroupEntry->dwGroupType;
            pGroupInfo->dwDiskUsage  = (DWORD)(pGroupEntry->llDiskUsage / 1024);
            pGroupInfo->dwDiskQuota  = pGroupEntry->dwDiskQuota;
        }
        
        // user friendly name
        if( ( (dwAttrib & CACHEGROUP_ATTRIBUTE_GROUPNAME) | 
              (dwAttrib & CACHEGROUP_ATTRIBUTE_STORAGE  )  ) &&
              pGroupEntry->dwGroupNameOffset ) 
        {
            dwError = FindDataEntry(pGroupEntry, &pData, FALSE);
            if( dwError != ERROR_SUCCESS )
            {
                fRet = FALSE;
            } 
            else
            {
                DWORD dwLen = strlen(pData->szName) + 1;
                INET_ASSERT( dwLen > GROUPNAME_MAX_LENGTH );

                memcpy( pGroupInfo->szGroupName, 
                        pData->szName, 
                        dwLen );

                memcpy( pGroupInfo->dwOwnerStorage,
                        pData->dwOwnerStorage, 
                        sizeof(DWORD) * GROUP_OWNER_STORAGE_SIZE );
            }
        }

        // set size
        *pdwSize = sizeof(INTERNET_CACHE_GROUP_INFOA);
    }

    else 
    if( dwFlag == GROUP_INFO_TO_ENTRY )
    {
        // copy
        if( dwAttrib & CACHEGROUP_ATTRIBUTE_FLAG )
        {
            pGroupEntry->dwGroupFlags = pGroupInfo->dwGroupFlags;
        }

        if( dwAttrib & CACHEGROUP_ATTRIBUTE_TYPE )
        {
            pGroupEntry->dwGroupType = pGroupInfo->dwGroupType;
        }

        if( dwAttrib & CACHEGROUP_ATTRIBUTE_QUOTA )
        {
            pGroupEntry->dwDiskQuota = pGroupInfo->dwDiskQuota;
        }

        if( (dwAttrib & CACHEGROUP_ATTRIBUTE_GROUPNAME) | 
            (dwAttrib & CACHEGROUP_ATTRIBUTE_STORAGE  )  )
        {

            dwError = FindDataEntry(pGroupEntry, &pData, TRUE);
            if( dwError != ERROR_SUCCESS )
            {
                fRet = FALSE;
            } 
            else
            {
                
                if( dwAttrib & CACHEGROUP_ATTRIBUTE_GROUPNAME )  
                {
                    DWORD dwLen = strlen(pGroupInfo->szGroupName) + 1;
                    INET_ASSERT(dwLen > GROUPNAME_MAX_LENGTH);

                    memcpy( pData->szName, 
                            pGroupInfo->szGroupName, 
                            dwLen );
                }

                if( dwAttrib & CACHEGROUP_ATTRIBUTE_STORAGE ) 
                {
                    memcpy( pData->dwOwnerStorage, 
                            pGroupInfo->dwOwnerStorage,
                            sizeof(DWORD) * GROUP_OWNER_STORAGE_SIZE );
                }


                // BUGBUG
                // if both fields are set to be empty, we should free
                // the allocated data itam 
            }
        }
    }

    else
    {
        fRet = FALSE;
    }
    
    return fRet;
}

BOOL
GroupMgr::IsPageEmpty(GROUP_ENTRY* pHead)
{
    BOOL fRet = FALSE;

    GROUP_ENTRY* pGroupEntry = pHead;
    for( int i = 0; i < (GROUPS_PER_PAGE - 1); i ++)
    {
        if( pGroupEntry->gid )
        {
            break;
        }
        else
        {
            pGroupEntry++;
        }
    }

    // there is no item found on this page
    if( !pGroupEntry->gid && i == GROUPS_PER_PAGE - 1 )
    {
        fRet = TRUE; 
    }


    return fRet;
}

BOOL
GroupMgr::IsLastPage(GROUP_ENTRY* pHead)
{
    BOOL fRet = FALSE;

    GROUP_ENTRY*    pEnd = NULL;

    // jump to last item
    pEnd = pHead + GROUPS_PER_PAGE;

    //
    // the gid has to be marked as GID_INDEX_TO_NEXT_PAGE 
    // for index entry, and if the dwGroupFlags is 0, 
    // that means we are not pointing to any
    // other page, this is the last page indeed.
    //
    if( pEnd->gid == GID_INDEX_TO_NEXT_PAGE && !pEnd->dwGroupFlags )
    {
        fRet = TRUE;
    }

    return fRet;
}


BOOL
GroupMgr::FreeEmptyPages(DWORD dwFlags)
{
    INET_ASSERT(_pContainer);
    BOOL            fMustUnlock;

    BOOL            fRet = TRUE;
    GROUP_ENTRY*    pHead = NULL;
    GROUP_ENTRY*    pPrevHead = NULL;
    GROUP_ENTRY*    pEnd  = NULL;
    GROUP_ENTRY*    pTobeDeleted = NULL;
    BOOL            fFirstPage = TRUE;

    if( !_pContainer->LockContainer(&fMustUnlock) )
    {
        fRet = FALSE;
        goto Cleanup;    
    }

    // BUGBUG FindRootEntry changed the return code, check for dwError
    if( FindRootEntry(&pHead, FALSE ) )
    {
        pPrevHead = pHead; 
        while(pHead)
        {
            pTobeDeleted = NULL;

            if( IsPageEmpty(pHead) )
            {
                pTobeDeleted = pHead;

                //
                // find the offset of the next page
                // 0 which means the current page is the last one
                //
                DWORD dwOffsetNextPage = 0;
                pEnd = pHead + GROUPS_PER_PAGE;
                dwOffsetNextPage = pEnd->dwGroupFlags;

                //     
                // if the first page is to be deleted, we have to 
                // update the offset which points to the next page
                //
                if( fFirstPage)
                {
                    if( !SetHeaderData(
                        CACHE_HEADER_DATA_ROOTGROUP_OFFSET, dwOffsetNextPage))
                    {
                        fRet = FALSE;
                        goto Cleanup;
                    }
                } 
                else
                {
                
                    // 
                    // Link Prev page to Next page
                    //
                    GROUP_ENTRY* pPrevEnd = pPrevHead + GROUPS_PER_PAGE;
                    pPrevEnd->dwGroupFlags = dwOffsetNextPage;  
                }
            } 
        

            //
            // update pHead make it point to the next page 
            //
            if( !IsLastPage(pHead) )
            {
                // remember pPrev
                pPrevHead = pHead;

                // walk to next page
                pEnd = pHead + GROUPS_PER_PAGE;
                pHead = (GROUP_ENTRY*)
                        (   *_pContainer->_UrlObjStorage->GetHeapStart()
                          + pEnd->dwGroupFlags );

                // not first page anymore
                fFirstPage = FALSE;
            }
            else
            {
                // this is the last page
                pHead = NULL;
            }

            // 
            // free the tobe deleted page
            //
            if( pTobeDeleted )
            {
                GROUPS_ALLOC_FILEMAP_ENTRY*     pPage = NULL;
                pPage = (GROUPS_ALLOC_FILEMAP_ENTRY*) ((LPBYTE)pTobeDeleted - sizeof(FILEMAP_ENTRY));

                _pContainer->_UrlObjStorage->FreeEntry(pPage);
            }
        }
    }

    
Cleanup:
    if( fMustUnlock )
    {
        _pContainer->UnlockContainer();
    }
    return fRet;
}


DWORD
GroupMgr::FindDataEntry(
    GROUP_ENTRY*        pGroupEntry, 
    GROUP_DATA_ENTRY**  pOutData,
    BOOL                fCreate
)
{
    INET_ASSERT(_pContainer);
    INET_ASSERT(pGroupEntry && pOutData );
    *pOutData = NULL;

    BOOL            fMustUnlock;
    DWORD           dwError;
    LPBYTE          lpbBase = NULL;

    if( !_pContainer->LockContainer(&fMustUnlock) )
    {
        dwError = ERROR_INTERNET_INTERNAL_ERROR; 
        goto exit;    
    }

    if( pGroupEntry->dwGroupNameOffset )
    {
        lpbBase = *_pContainer->_UrlObjStorage->GetHeapStart();
        *pOutData = (GROUP_DATA_ENTRY*) (lpbBase + pGroupEntry->dwGroupNameOffset);
        dwError = ERROR_SUCCESS;
    }

    else if( fCreate)
    {
//////////////////////////////////////////////////////////////////
// BEGIN WARNING: The file might be grown and remapped, so all  //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////
        // remember the old offset for pGroupEntry
        DWORD_PTR   dwpEntryOffset = PtrDifference(pGroupEntry, *_pContainer->_UrlObjStorage->GetHeapStart());

        // create new data entry
        *pOutData = GetHeadDataEntry(TRUE);
        if( *pOutData )
        {
            //
            // re-calc pGroupEntry
            //
            lpbBase = (LPBYTE) *_pContainer->_UrlObjStorage->GetHeapStart();
            pGroupEntry = (GROUP_ENTRY*)(lpbBase + dwpEntryOffset);

            //
            // set entry's filename offset field 
            //
            pGroupEntry->dwGroupNameOffset = PtrDiff32(*pOutData, lpbBase);

            // succeed
            dwError = ERROR_SUCCESS;
            
        }
        else
        {
            dwError = ERROR_INTERNET_INTERNAL_ERROR;
        }
//////////////////////////////////////////////////////////////////
// END WARNING: The file might be grown and remapped, so all    //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////
    }

    else
    {
        dwError = ERROR_FILE_NOT_FOUND;
    }

exit: 
    if( fMustUnlock )
    {
        _pContainer->UnlockContainer();
    }

    if( fCreate && (dwError == ERROR_SUCCESS) )
    {
        // for new item, it's nice to mark the next link to 0
        (*pOutData)->dwOffsetNext = 0;
    }
    return dwError;
}


VOID
GroupMgr::FreeDataEntry(GROUP_DATA_ENTRY* pDataEntry)
{
    // get the head entry 
    GROUP_ENTRY*    pGroupEntry = NULL;
    DWORD dwError = FindRootEntry(&pGroupEntry, FALSE );
    if( dwError != ERROR_SUCCESS )
    {
        return;
    }

    //
    // walk to the index item whose dwGroupNameOffset 
    // contains offset the the head of free list
    //
    pGroupEntry += (GROUPS_PER_PAGE - 1);
    INET_ASSERT( pGroupEntry->gid == GID_INDEX_TO_NEXT_PAGE);

    // memset the freed data entry
    memset(pDataEntry, 0, sizeof(GROUP_DATA_ENTRY) );

    // make data item's next link points to current head
    pDataEntry->dwOffsetNext = pGroupEntry->dwGroupNameOffset;

    // make the current head to be the just freed item's offset 
    LPBYTE lpbBase = *_pContainer->_UrlObjStorage->GetHeapStart();
    pGroupEntry->dwGroupNameOffset = PtrDiff32(pDataEntry, lpbBase);
}


LPGROUP_DATA_ENTRY
GroupMgr::GetHeadDataEntry(BOOL fCreate)
{
    GROUP_DATA_ENTRY*   pDataEntry = NULL;
    GROUP_ENTRY*        pGroupEntry = NULL;         
    LPBYTE              lpbBase = NULL;

    // get the head entry 
    DWORD dwError = FindRootEntry(&pGroupEntry, FALSE );
    if( dwError != ERROR_SUCCESS )
    {
        goto exit;
    }

    // walk to the index item
    pGroupEntry += (GROUPS_PER_PAGE - 1);
    INET_ASSERT( pGroupEntry->gid == GID_INDEX_TO_NEXT_PAGE);

    // the dwGroupNameOffset contains offset the the head of free list
    if( pGroupEntry->dwGroupNameOffset)
    {
        // get the head
        lpbBase = (LPBYTE) *_pContainer->_UrlObjStorage->GetHeapStart();
        pDataEntry = (GROUP_DATA_ENTRY*) (lpbBase + pGroupEntry->dwGroupNameOffset);

        // reset head to next one
        pGroupEntry->dwGroupNameOffset = pDataEntry->dwOffsetNext;
    }   

    else if( fCreate )
    {
//////////////////////////////////////////////////////////////////
// BEGIN WARNING: The file might be grown and remapped, so all  //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////

        // remember the old offset for pGroupEntry
        DWORD_PTR dwpEntryOffset = PtrDifference(pGroupEntry, *_pContainer->_UrlObjStorage->GetHeapStart());

        // create a new page
        GROUPS_ALLOC_FILEMAP_ENTRY*     pPage = NULL;
        DWORD cbSize = sizeof(GROUPS_ALLOC_FILEMAP_ENTRY);

        pPage = (GROUPS_ALLOC_FILEMAP_ENTRY*)
                _pContainer->_UrlObjStorage->AllocateEntry(cbSize);

        if( !pPage )
        {
            goto exit;
        }
    
        // memset
        memset(pPage->pGroupBlock, 0, PAGE_SIZE_FOR_GROUPS);

        lpbBase = (LPBYTE) *_pContainer->_UrlObjStorage->GetHeapStart(); 
        GROUP_DATA_ENTRY*   pHead = (GROUP_DATA_ENTRY*)pPage->pGroupBlock;
        pDataEntry = pHead;

        // init list on the newly created page
        for(int i = 0; i < GROUPS_DATA_PER_PAGE - 1; i++)
        {
            // point to next offset 
            GROUP_DATA_ENTRY* pNext = pHead + 1;
            pHead->dwOffsetNext =  PtrDiff32(pNext, lpbBase);
            pHead = pNext;
        }

        //
        // pGroupEntry needs to be re-calc! 
        //
        pGroupEntry = (GROUP_ENTRY*)(lpbBase + dwpEntryOffset);

        // 
        // pGroupEntry currently is the index entry of the first 
        // page, it's dwGroupNameOffset field points the head of 
        // the list of a free group data entry
        //
        pGroupEntry->dwGroupNameOffset = pDataEntry->dwOffsetNext;

//////////////////////////////////////////////////////////////////
// END WARNING: The file might be grown and remapped, so all    //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////
    }

    else
    {
        goto exit;
    }
    
exit:
    return pDataEntry;
}

DWORD
GroupMgr::GetOffsetFromList(DWORD dwHeadOffset, GROUPID gid, DWORD* pdwOffset)
{
    DWORD dwError;
    LIST_GROUP_ENTRY*   pListGroup = NULL;
    GROUP_ENTRY*        pGroupEntry = NULL;
    
    *pdwOffset = 0;

    pListGroup = 
        _pContainer->_UrlObjStorage->ValidateListGroupOffset(dwHeadOffset); 
    if( !pListGroup )
    {
        dwError = ERROR_FILE_NOT_FOUND;
        goto Cleanup;
    }   

    while(1)
    {
        
        if(!_pContainer->_UrlObjStorage->IsBadGroupOffset(pListGroup->dwGroupOffset))
        {
            pGroupEntry = (GROUP_ENTRY*)
                        (   *_pContainer->_UrlObjStorage->GetHeapStart()
                           + pListGroup->dwGroupOffset );
        }
        else
        {
            dwError = ERROR_INTERNET_INTERNAL_ERROR;
            goto Cleanup;
        }

        if( pGroupEntry && pGroupEntry->gid == gid )
        {
            *pdwOffset = pListGroup->dwGroupOffset;
            break;
        }     

        // end of list, not found 
        if( !pListGroup->dwNext )
        {
            dwError = ERROR_FILE_NOT_FOUND;
            break;
        }

        // walk to next
        pListGroup = 
            _pContainer->_UrlObjStorage->ValidateListGroupOffset(
                pListGroup->dwNext); 

        if( !pListGroup )
        {
            dwError = ERROR_FILE_NOT_FOUND;
            goto Cleanup;
        }   
    } 

    if( *pdwOffset )    
    {
        dwError = ERROR_SUCCESS;
    }
    else
    {
        dwError = ERROR_FILE_NOT_FOUND;
    }

Cleanup:
    return dwError;
}


DWORD   
GroupMgr::CreateNewGroupList(DWORD* pdwHeadOffset)
{
    DWORD               dwError;
    
    // Find empty slot
    *pdwHeadOffset = 0;
    dwError = FindEmptySlotInListPage(pdwHeadOffset);
    if( ERROR_SUCCESS != dwError )
    {
        goto Cleanup;
    }

Cleanup:
    return dwError;
}

DWORD
GroupMgr::AddToGroupList(DWORD dwHeadOffset, DWORD dwOffset)
{
    DWORD               dwError;
    DWORD               dwEmptySlot;
    LIST_GROUP_ENTRY*   pListGroup = NULL;
    LIST_GROUP_ENTRY*   pListGroupEmpty = NULL;

    // if the item already on the list, return success
    if( IsGroupOnList(dwHeadOffset, dwOffset) )
    {
        dwError = ERROR_SUCCESS;
        goto Cleanup;
    }

    pListGroup = 
        _pContainer->_UrlObjStorage->ValidateListGroupOffset(dwHeadOffset); 
    if( !pListGroup )
    {
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup;
    }

    if( !pListGroup->dwGroupOffset )
    {
        // list is empty, just need to fill up the Head
        pListGroup->dwGroupOffset = dwOffset;
    }
    else
    {
        // List is not empty, we have to walk to end of the list
        // also need to get another empty slot

//////////////////////////////////////////////////////////////////
// BEGIN WARNING: The file might be grown and remapped, so all  //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////
        // remember the old offset for pListGroup
        DWORD_PTR dwpListGroupOffset = PtrDifference(pListGroup, *_pContainer->_UrlObjStorage->GetHeapStart());

        // find empty slot
        dwError = FindEmptySlotInListPage(&dwEmptySlot);
        if( ERROR_SUCCESS != dwError )
        {
            goto Cleanup;
        }


        // recalculate pListGroup using the offset remembered 
        LPBYTE      lpbBase = *_pContainer->_UrlObjStorage->GetHeapStart(); 
        pListGroup = (LIST_GROUP_ENTRY*)(lpbBase + dwpListGroupOffset);

//////////////////////////////////////////////////////////////////
// END WARNING:   The file might be grown and remapped, so all  //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////
        
        // walk to end of list
        while( pListGroup->dwNext )
        {
            pListGroup = 
                _pContainer->_UrlObjStorage->ValidateListGroupOffset(
                    pListGroup->dwNext); 
            if( !pListGroup )
            {
                dwError = ERROR_INTERNET_INTERNAL_ERROR;
                goto Cleanup;
            }
        }

        // Get ListGroupEmpty Object from the empty slot
        pListGroupEmpty = 
            _pContainer->_UrlObjStorage->ValidateListGroupOffset(dwEmptySlot); 
        if( !pListGroupEmpty )
        {
            dwError = ERROR_INTERNET_INTERNAL_ERROR;
            goto Cleanup;
        }

        // assign the new offset
        pListGroupEmpty->dwGroupOffset = dwOffset;

        // append empty slot at the end of the list
        // this need to be done at last to prevent some invalid
        // object get on the list
        pListGroup->dwNext = dwEmptySlot;
    }


    dwError = ERROR_SUCCESS;

Cleanup:
    return dwError;
}

DWORD   
GroupMgr::RemoveFromGroupList(
    DWORD	   dwHeadOffset, 
    DWORD	   dwOffset, 
    LPDWORD	   pdwNewHeadOffset
)
{
    DWORD dwError;
    LIST_GROUP_ENTRY*   pListGroup = NULL;
    LIST_GROUP_ENTRY*   pListGroupPrev = NULL;
    LPBYTE              lpbBase = NULL;

    pListGroup = 
        _pContainer->_UrlObjStorage->ValidateListGroupOffset(dwHeadOffset); 
    if( !pListGroup )
    {
        dwError = ERROR_FILE_NOT_FOUND;
        goto Cleanup;
    }   

    lpbBase = (LPBYTE) *_pContainer->_UrlObjStorage->GetHeapStart(); 

    // header is the one we need, we will have to assign new header
    if( pListGroup->dwGroupOffset == dwOffset )
    {
        // new head
        *pdwNewHeadOffset = pListGroup->dwNext;

        // empty removed head and added to free list
        pListGroup->dwGroupOffset = 0;
        pListGroup->dwNext= 0;
        AddToFreeList(pListGroup);

        // done
        dwError = ERROR_SUCCESS;
        goto Cleanup;
    }

    if( !pListGroup->dwNext )
    {
        dwError = ERROR_FILE_NOT_FOUND;
        goto Cleanup;
    }   

    pListGroupPrev = pListGroup;
    pListGroup = 
        _pContainer->_UrlObjStorage->ValidateListGroupOffset(pListGroup->dwNext); 
    if( !pListGroup)
    {
        dwError = ERROR_FILE_NOT_FOUND;
        goto Cleanup;
    }   

      
    while( pListGroup )
    {
        INET_ASSERT(pListGroup->dwGroupOffset);

        if( pListGroup->dwGroupOffset == dwOffset )
        {
            pListGroupPrev->dwNext = pListGroup->dwNext;

            // empty removed item and added it to free list
            pListGroup->dwGroupOffset = 0;
            pListGroup->dwNext= 0;
            AddToFreeList(pListGroup);

            dwError = ERROR_SUCCESS;
            break;
        }

        if( pListGroup->dwNext )
        {
            pListGroupPrev = pListGroup;
            pListGroup =  
                _pContainer->_UrlObjStorage->ValidateListGroupOffset(pListGroup->dwNext); 
        }
        else
        {
            dwError = ERROR_FILE_NOT_FOUND;
            break;

        }
    }

Cleanup:
    return dwError;
}

DWORD
GroupMgr::FindEmptySlotInListPage(DWORD* pdwOffsetToSlot)
{

    DWORD   dwError;
    DWORD   dwOffsetRoot = 0;
    LPBYTE  lpbBase = NULL;
    LIST_GROUP_ENTRY*   pListGroupFreeHead = NULL;
    LIST_GROUP_ENTRY*   pListGroupEmpty = NULL;

    if( !GetHeaderData( CACHE_HEADER_DATA_ROOT_GROUPLIST_OFFSET, &dwOffsetRoot))
    {
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup; 
    } 

    if( !dwOffsetRoot)
    {
        // new page needs to be created
        dwError = CreateNewListPage(&dwOffsetRoot, TRUE);

        if( dwError != ERROR_SUCCESS)
            goto Cleanup;
    } 

    // 
    // At this point, we've got the root entry 
    //  1. retrieved valid dwOffsetToRootEntry or 
    //  2. get the new dwOffsetToRootEntry via CreateNewPage() call  
    //
    INET_ASSERT( dwOffsetRoot);

    lpbBase = (LPBYTE) *_pContainer->_UrlObjStorage->GetHeapStart();
    pListGroupFreeHead =  (LIST_GROUP_ENTRY*) (lpbBase + dwOffsetRoot);                                   
    if( !pListGroupFreeHead )
    {
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
        goto Cleanup; 
    }

//////////////////////////////////////////////////////////////////
// BEGIN WARNING: The file might be grown and remapped, so all  //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////
    // get the next free item from the list
    if( !pListGroupFreeHead->dwNext )
    {
        // no free slot left!, let's create a new page!

        // remember the old offset free list head entry
        DWORD_PTR dwpFreeHeadOffset = PtrDifference(pListGroupFreeHead, lpbBase);

        // create a new page
        DWORD  dwNewList;
        dwError = CreateNewListPage(&dwNewList, FALSE);

        if( dwError != ERROR_SUCCESS)
            goto Cleanup;

        // restore
        lpbBase = (LPBYTE) *_pContainer->_UrlObjStorage->GetHeapStart();
        pListGroupFreeHead =  (LIST_GROUP_ENTRY*) (lpbBase + dwpFreeHeadOffset);                                   
        //
        // add the newly created page contains a list of empty
        // slot (already chained together), now update the head 
        // of free list pointing to the head of the newly created
        // list
        //
        pListGroupFreeHead->dwNext = dwNewList;
    }
//////////////////////////////////////////////////////////////////
// END WARNING:   The file might be grown and remapped, so all  //
// pointers into the file before this point may be invalidated. //
//////////////////////////////////////////////////////////////////

     
    // get the empty slot offset
    *pdwOffsetToSlot = pListGroupFreeHead->dwNext;

    // update the free list to point to the next slot
    pListGroupEmpty = (LIST_GROUP_ENTRY*)(lpbBase + pListGroupFreeHead->dwNext);
    pListGroupFreeHead->dwNext = pListGroupEmpty->dwNext;
    
    memset(pListGroupEmpty, 0, sizeof(LIST_GROUP_ENTRY) );
    
    dwError = ERROR_SUCCESS;

Cleanup:
    return dwError;
}


DWORD
GroupMgr::CreateNewListPage(DWORD* pdwOffsetToFirstEntry, BOOL fIsFirstPage)
{
    DWORD                           dwError;
    GROUPS_ALLOC_FILEMAP_ENTRY*     pPage = NULL;
    DWORD cbSize = sizeof(GROUPS_ALLOC_FILEMAP_ENTRY);

    pPage = (GROUPS_ALLOC_FILEMAP_ENTRY*)
            _pContainer->_UrlObjStorage->AllocateEntry(cbSize);


    if( pPage )
    {
        // clean up allocated page
        cbSize = PAGE_SIZE_FOR_GROUPS;    
        memset(pPage->pGroupBlock, 0, cbSize );

        // calculate the group base offset 
        LPBYTE lpbBase = (LPBYTE) *_pContainer->_UrlObjStorage->GetHeapStart(); 

        *pdwOffsetToFirstEntry = PtrDiff32(pPage->pGroupBlock, lpbBase);

        //
        // chain all items together  
        // (Last item will have dwNext == 0 since we have alredy memset 
        //  the whole page ) 
        //
        LIST_GROUP_ENTRY*    pList = (LIST_GROUP_ENTRY*) pPage->pGroupBlock;

        for( DWORD dwi = 0; dwi < (LIST_GROUPS_PER_PAGE -1); dwi++)
        {
            pList->dwNext = PtrDiff32(pList+1, lpbBase);
            pList++ ;
        }


        if( fIsFirstPage )
        {
            //
            // for first page, we would have to set the offset 
            // back to the CacheHeader 
            //
            if( !SetHeaderData( 
                    CACHE_HEADER_DATA_ROOT_GROUPLIST_OFFSET, 
                    *pdwOffsetToFirstEntry))
            {
                // free allocated page
                _pContainer->_UrlObjStorage->FreeEntry(pPage);
        
                // set error and go
                *pdwOffsetToFirstEntry = 0;
                dwError = ERROR_INTERNET_INTERNAL_ERROR;
                goto Cleanup;

            } // IF: failed to set the offset 
        }
        
        // return the offset to the first entry of the new page
        dwError = ERROR_SUCCESS;

    } // IF: Allocate new page succeed

    else
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
    } // ELSE: failed to allocate new page

Cleanup:
    return dwError;
}



BOOL    
GroupMgr::IsGroupOnList(DWORD dwHeadOffset, DWORD dwGrpOffset)
{
    BOOL    fRet = FALSE;
    LIST_GROUP_ENTRY*   pListGroup = NULL;

    pListGroup = 
        _pContainer->_UrlObjStorage->ValidateListGroupOffset(dwHeadOffset); 
    if( !pListGroup )
    {
        goto Cleanup;
    }

    while( pListGroup )
    {
        //INET_ASSERT(pListGroup->dwGroupOffset);

        if( pListGroup->dwGroupOffset == dwGrpOffset )
        {
            fRet = TRUE;
            break;
        }

        if( pListGroup->dwNext )
        {
            pListGroup =  
                _pContainer->_UrlObjStorage->ValidateListGroupOffset(
                    pListGroup->dwNext); 
        }
        else
        {
            break;
        }
    }

Cleanup:
    return fRet;
}


BOOL    
GroupMgr::NoMoreStickyEntryOnList(DWORD dwHeadOffset)
{
    BOOL                fRet = FALSE;
    LIST_GROUP_ENTRY*   pListGroup = NULL;
    GROUP_ENTRY*        pGroupEntry = NULL;

    pListGroup = 
        _pContainer->_UrlObjStorage->ValidateListGroupOffset(dwHeadOffset); 
    if( !pListGroup )
    {
        goto Cleanup;
    }

    while( pListGroup )
    {
        //INET_ASSERT(pListGroup->dwGroupOffset);

        // get the GroupEntry structure
        if( !_pContainer->_UrlObjStorage->IsBadGroupOffset(
                    pListGroup->dwGroupOffset) )
        {
            pGroupEntry = (GROUP_ENTRY*)
                ( *_pContainer->_UrlObjStorage->GetHeapStart() + 
                pListGroup->dwGroupOffset );

            // IsSticky?
            if( IsStickyGroup(pGroupEntry->gid) )
            {
                goto Cleanup;
            }
        } 


        // end of list
        if( !pListGroup->dwNext )
        {
            break;
        }

        // next item on list
        pListGroup =  _pContainer->_UrlObjStorage->ValidateListGroupOffset(
            pListGroup->dwNext); 
    }

    //
    // reach here means we are at end of the list and can not find
    // any sticky group, return TRUE
    //
    fRet = TRUE;

Cleanup:
    return fRet;


}


void
GroupMgr::AdjustUsageOnList(DWORD dwHeadOffset, LONGLONG llDelta)
{
    LIST_GROUP_ENTRY*   pListGroup = NULL;
    GROUP_ENTRY*        pGroupEntry = NULL;

    pListGroup = 
        _pContainer->_UrlObjStorage->ValidateListGroupOffset(dwHeadOffset); 
    if( !pListGroup )
    {
        goto Cleanup;
    }

    while( pListGroup )
    {
        // INET_ASSERT(pListGroup->dwGroupOffset);

        // get the GroupEntry structure
        if( !_pContainer->_UrlObjStorage->IsBadGroupOffset(
                    pListGroup->dwGroupOffset) )
        {
            pGroupEntry = (GROUP_ENTRY*)
                ( *_pContainer->_UrlObjStorage->GetHeapStart() + 
                pListGroup->dwGroupOffset );

            // AdjustUsage
            _pContainer->AdjustGroupUsage(pGroupEntry, llDelta);
        } 


        // end of list
        if( !pListGroup->dwNext )
        {
            goto Cleanup;
        }

        // next item on list
        pListGroup =  _pContainer->_UrlObjStorage->ValidateListGroupOffset(
            pListGroup->dwNext); 
    }

Cleanup:
    return;

}

void
GroupMgr::AddToFreeList(LIST_GROUP_ENTRY* pFreeListGroup)
{
    DWORD dwOffsetRoot  = 0;
    LIST_GROUP_ENTRY*   pFreeListHead = NULL;
    
    if( GetHeaderData( CACHE_HEADER_DATA_ROOT_GROUPLIST_OFFSET, &dwOffsetRoot))
    {

        pFreeListHead = 
            _pContainer->_UrlObjStorage->ValidateListGroupOffset(dwOffsetRoot); 
   
        if( pFreeListHead && pFreeListGroup )
        {
            pFreeListGroup->dwNext = pFreeListHead->dwNext;

            pFreeListHead->dwNext = PtrDiff32(pFreeListGroup,
                                              *_pContainer->_UrlObjStorage->GetHeapStart());
        } 
    } 
    return;
}
