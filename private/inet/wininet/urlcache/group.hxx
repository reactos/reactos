/*++
Copyright (c) 1998  Microsoft Corporation

Module Name:  group.hxx

Abstract:

    Manages cache group.
    
Author:
    Danpo Zhang (DanpoZ) 02-08-98
--*/


#ifndef _GROUP_
#define _GROUP_

#define GROUP_INFO_TO_ENTRY     1
#define GROUP_ENTRY_TO_INFO     2

class GroupMgr 
{
public:

    GroupMgr();
    ~GroupMgr();

    BOOL    Init(URL_CONTAINER* pContainer);
    DWORD   CreateGroup(DWORD, GROUPID*);
    DWORD   CreateDefaultGroups();
    DWORD   DeleteGroup(GROUPID, DWORD);
    DWORD   GetNextGroup(DWORD*, GROUPID*);
    DWORD   GetGroup(GROUPID, DWORD, INTERNET_CACHE_GROUP_INFOA*, DWORD*);
    DWORD   SetGroup(GROUPID, DWORD, INTERNET_CACHE_GROUP_INFOA*);


protected:
    URL_CONTAINER*  _pContainer; 

private:
    GROUPID ObtainNewGID();
    DWORD   CreateNewPage(DWORD* dwOffset, BOOL fIsFirstPage);
    DWORD   FindRootEntry(GROUP_ENTRY** ppEntryOut, BOOL fCreate);
    DWORD   FindEntry(GROUPID gid, GROUP_ENTRY** ppEntryOut, BOOL fCreate);
    BOOL    IsIndexToNewPage(GROUP_ENTRY* pG) 
            { 
                return (pG->gid == GID_INDEX_TO_NEXT_PAGE) ? TRUE : FALSE; 
            }

    BOOL    Translate(DWORD, INTERNET_CACHE_GROUP_INFOA*, GROUP_ENTRY*, 
                DWORD, DWORD*);

    BOOL    GetHeaderData(DWORD nIdx, LPDWORD pdwData)
            {
                return _pContainer->_UrlObjStorage->GetHeaderData(
                                                        nIdx, pdwData);
            }

    BOOL    SetHeaderData(DWORD nIdx, DWORD dwData)
            {
                return _pContainer->_UrlObjStorage->SetHeaderData(
                                                        nIdx, dwData);
            }

    DWORD   FindDataEntry(LPGROUP_ENTRY, GROUP_DATA_ENTRY**, BOOL);
    VOID    FreeDataEntry(GROUP_DATA_ENTRY*);
    LPGROUP_DATA_ENTRY GetHeadDataEntry(BOOL);


    BOOL    IsLastPage(GROUP_ENTRY*);
    BOOL    IsPageEmpty(GROUP_ENTRY*);

    BOOL    FreeEmptyPages(DWORD);

    DWORD   CreateNewGroupList(DWORD*);
    DWORD   GetOffsetFromList(DWORD, GROUPID, DWORD*); 
    DWORD   AddToGroupList(DWORD, DWORD);
    DWORD   RemoveFromGroupList(DWORD, DWORD, DWORD*);
    void    AdjustUsageOnList(DWORD dwHeadOffset, LONGLONG llDelta);

    DWORD   FindEmptySlotInListPage(DWORD* pdwOffset);
    DWORD   CreateNewListPage(DWORD* pdwOffset, BOOL fIsFirstPage);
    BOOL    IsIndexToNewListPage(LIST_GROUP_ENTRY* p) 
            { 
                return 
                (p->dwGroupOffset == OFFSET_TO_NEXT_PAGE) ? TRUE : FALSE; 
            }


    BOOL    IsGroupOnList(DWORD dwHeadOffset, DWORD dwGroupOffset);
    BOOL    NoMoreStickyEntryOnList(DWORD dwHeadOffset);
    void    AddToFreeList(LIST_GROUP_ENTRY* pFreeListGroup);

friend class URL_CONTAINER;

};

#endif
