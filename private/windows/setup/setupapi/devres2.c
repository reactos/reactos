/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    devres2.c

Abstract:

    Utility routines for resource matching

Author:

    Jamie Hunter (jamiehun) 9-July-1998

--*/

#include "precomp.h"
#pragma hdrstop

PRDE_LIST
pRDEList_Alloc()
/*++

Routine Description:

    Allocates a list-entry node

Arguments:

Return Value:

   PRDE_LIST entry

--*/
{
    PRDE_LIST Node;

    Node = (PRDE_LIST)MyMalloc(sizeof(RDE_LIST));
    if (Node == NULL) {
        return NULL;
    }
    Node->Prev = Node;
    Node->Next = Node;
    Node->Entry = NULL;
    return Node;
}

VOID
pRDEList_AddHead(
    IN OUT PRDE_LIST pList,
    IN PRDE_LIST Node
                 )
/*++

Routine Description:

    Adds a node to head of list

Arguments:

    pList =  pointer to list
    Node  =  node to add

Return Value:

   none

--*/
{
    MYASSERT(Node);
    MYASSERT(Node->Prev == Node);
    MYASSERT(Node->Next == Node);
    MYASSERT(pList);

    Node->Prev = pList;
    Node->Next = pList->Next;
    Node->Next->Prev = Node;
    pList->Next = Node; // Head
}

VOID
pRDEList_AddTail(
    IN OUT PRDE_LIST pList,
    IN PRDE_LIST Node
                 )
/*++

Routine Description:

    Adds a node to tail of list

Arguments:

    pList =  pointer to list
    Node  =  node to add

Return Value:

   none

--*/
{
    MYASSERT(Node);
    MYASSERT(Node->Prev == Node);
    MYASSERT(Node->Next == Node);
    MYASSERT(pList);

    Node->Next = pList;
    Node->Prev = pList->Prev;
    Node->Prev->Next = Node;
    pList->Prev = Node; // Tail
}

VOID
pRDEList_Remove(
    IN PRDE_LIST Node
                 )
/*++

Routine Description:

    Removes a node from list that node is member of

Arguments:

    Node  =  node to remove

Return Value:

   none

--*/
{
    MYASSERT(Node);

    if (Node->Prev == Node && Node->Next == Node) {
        //
        // already removed
        //
        return;
    }

    Node->Prev->Next = Node->Next;
    Node->Next->Prev = Node->Prev;
    Node->Next = Node;
    Node->Prev = Node;
}

PRDE_LIST
pRDEList_Find(
    IN PRDE_LIST pList,
    IN PRESDES_ENTRY pItem
    )
/*++

Routine Description:

    Looks for pItem in pList

Arguments:

    pList = list to search
    pItem = item to search for

Return Value:

   Node entry, or NULL

--*/
{
    PRDE_LIST Node;

    MYASSERT(pList);
    MYASSERT(pItem);

    Node = pList->Next; // head
    while (Node != pList) {
        if (Node->Entry == pItem) {
            return Node;
        }
        Node = Node->Next;
    }
    return NULL;
}


VOID
pRDEList_Destroy(
    IN PRDE_LIST pList
                 )
/*++

Routine Description:

    Destroy pList and everything on pList

Arguments:

    pList = list to destroy

Return Value:

   Node entry, or NULL

--*/
{
    PRDE_LIST Node,Next;

    if (pList == NULL) {
        return;
    }

    Node = pList; // head
    do
    {
        Next = Node->Next;
        MyFree(Node);       // this will free pList first, and then everything else on same list
        Node = Next;
    }
    while (Node != pList);
}

BOOL
pGetMatchingRange(
    IN ULONG    ulKnownValue,
    IN ULONG    ulKnownLen,
    IN LPBYTE   pData,
    IN RESOURCEID ResType,
    OUT PULONG  pRange,
    OUT PBOOL   pExact,
    OUT PULONG  pFlags
    )
/*++

Routine Description:

    Finds range index for resource inside ResDes

Arguments:

    ulKnownValue - base address
    ulKnownLen - length of resources
    pData/ResType - resource data we're comparing with
    pRange - output range index
    pExact - output true if there is only one range
    pFlags - output flags from matching range

Return Value:

   BOOL if match

--*/
{
    PGENERIC_RESOURCE pGenRes = NULL;
    ULONG ulValue = 0, ulLen = 0, ulEnd = 0, ulFlags = 0, i;

    pGenRes = (PGENERIC_RESOURCE)pData;

    for (i = 0; i < pGenRes->GENERIC_Header.GENERIC_Count; i++) {

        pGetRangeValues(pData, ResType, i, &ulValue, &ulLen, &ulEnd, NULL, &ulFlags);
        if (ulLen != ulKnownLen) {
            continue;
        }

        if ((ulKnownValue >= ulValue) &&
            ((ulKnownValue + ulLen - 1) <= ulEnd)) {

            if (pRange != NULL) {
                *pRange = i;
            }
            //
            // consider exact case
            //
            if (pExact != NULL) {
                if (pGenRes->GENERIC_Header.GENERIC_Count==1 && ulValue == ulKnownValue && (ulKnownValue + ulLen - 1) == ulEnd) {
                    *pExact = TRUE;
                }
            }
            if (pFlags != NULL) {
                //
                // want nearest flags
                //
                *pFlags = ulFlags;
            }

            return TRUE;
        }
    }
    if (pRange != NULL) {
        *pRange = 0;
    }
    return FALSE;
}


ULONG
pTryMatch(
    IN OUT PRESDES_ENTRY pKnown,
    IN OUT PRDE_LIST pResList,
    IN OUT PULONG pDepth
    )
/*++

Routine Description:

    Looks for best match of remaining requirements in
    remaining available resources
    returns number of matched requirements

Arguments:

    pKnown - requirements list (what is remaining)
    pResList - list of available resources

Return Value:

   Node entry, or NULL

--*/
{
    ULONG ThisBest = 0;
    ULONG MyBest = 0;
    PRDE_LIST pIterator = NULL;
    PRESDES_ENTRY pRange;
    ULONG  ulValue, ulLen, ulEnd, ulFlags;
    BOOL Exact = FALSE;
    ULONG Best = 0;
    PRDE_LIST pBestRes = NULL;
    BOOL BadMatch = FALSE;
    BOOL Prune = FALSE;
    BOOL NoRemaining = TRUE;

    MYASSERT(pDepth);
    *pDepth = 0;

    if (pKnown == NULL) {
        //
        // end recursion
        //
        return 0;
    }
    pKnown->CrossLink = NULL;

    //
    // we're looking for a match in pResList for pKnown
    // case (1) get a "Best" for if we decide not to match
    // case (2) get a "Best" for exact match, and chose between (1) and (2) if one exists
    // case (3) get a "Best" for each possible range match and choose best between (1) and all (3)
    //

    //
    // consider case(1) - what the results would be if we wasn't able to fit anything in
    //
    //Best = pTryMatch(pKnown->Next,pResList,pDepth);
    //pBestRes = NULL;

    pGetHdrValues(pKnown->ResDesData, pKnown->ResDesType, &ulValue, &ulLen, &ulEnd, &ulFlags);

    //
    // consider case(2) and (3) together
    //
    for(pIterator = pResList->Next;pIterator!=pResList;pIterator = pIterator->Next) {
        //
        // iterate through remaining resources
        //
        pRange = pIterator->Entry;
        if (pRange == NULL) {
            //
            // this has been used
            //
            continue;
        }
        if (pRange->ResDesType != pKnown->ResDesType) {
            //
            // not the kind of resource i'm looking for
            //
            continue;
        }
        NoRemaining = FALSE;
        if(pGetMatchingRange(ulValue, ulLen,pRange->ResDesData, pRange->ResDesType,NULL,&Exact,NULL)) {
            pIterator->Entry = NULL; // eliminate this range
            ThisBest = pTryMatch(pKnown->Next,pResList,pDepth); // match the rest, with us using this resource-range
            pIterator->Entry = pRange;
            if ((ThisBest > Best) || (pBestRes == NULL)) {
                //
                // the current best match (or first match if pBestRes == NULL)
                //
                pKnown->CrossLink = pRange;
                pBestRes = pIterator;
                Best = ThisBest;
                MyBest = 1;
                BadMatch = FALSE;
            } else {
                //
                // need to re-do best match
                //
                BadMatch = TRUE;
            }
            if (Exact || (*pDepth == ThisBest)) {
                //
                // prune - we're either exact, or got a perfect match
                //
                Prune = TRUE;
                goto Final;
            }
        }
    }

    if (NoRemaining) {
        //
        // I have no resources remaining I can use - consider this as good as a match
        // but we need to continue up the tree
        //
        Best = pTryMatch(pKnown->Next,pResList,pDepth); // match the rest, with us using this resource-range
        MyBest = TRUE;
        BadMatch = FALSE;
        goto Final;
    }

    //
    // if we get here we've:
    // (1) found a flexable match, but wasn't able to match everyone above us, or
    // (2) not found any match
    // note that if last best was n with us matching and this best is n+1 without
    // then we don't lose the best
    // consider if overall we'd do any better if we gave up our resources to someone else
    //
    if((pBestRes == NULL) || ((Best+MyBest) < *pDepth)) {
        //
        // if we had a match, only worth checking if we could increase best by more than MyBest
        // note that *pDepth is only valid if pBestRes != NULL
        //
        ThisBest = pTryMatch(pKnown->Next,pResList,pDepth);
        if ((ThisBest > (Best+MyBest)) || (pBestRes == NULL)) {
            //
            // the current best match
            //
            pKnown->CrossLink = NULL;
            pBestRes = NULL;
            Best = ThisBest;
            MyBest = 0;
            BadMatch = FALSE;
        } else {
            //
            // need to re-do best match
            //
            BadMatch = TRUE;
        }
    }

Final:

    if (BadMatch) {
        //
        // We had a bad-match since our last good match
        //
        if (pBestRes) {
            pRange = pBestRes->Entry; // the range we determined was our best bet
            pBestRes->Entry = NULL; // eliminate this range
            Best = pTryMatch(pKnown->Next,pResList,pDepth); // match the rest, with us using this resource-range
            pBestRes->Entry = pRange;
            pKnown->CrossLink = pRange;
            MyBest = 1;
        } else {
            Best = pTryMatch(pKnown->Next,pResList,pDepth); // match the rest, with us not using this resource range
            pKnown->CrossLink = NULL;
            MyBest = 0;
        }
    }

    //
    // if we found a match, we've saved it in pKnown->CrossLink
    // return Best+0 if it's better we don't fit our resource in, Best+1 otherwise
    // returns *pDepth = Best+1 if everyone (me up) fit their resources in
    //

    (*pDepth)++; // increment to include me
    return Best+MyBest; // MyBest = 1 if I managed to match my resource (or no resources left), Best = everyone to the right of me
}

ULONG
pMergeResDesDataLists(
    IN OUT PRESDES_ENTRY pKnown,
    IN OUT PRESDES_ENTRY pTest,
    OUT PULONG pMatchCount
    )
/*++

Routine Description:

    Map entries in pKnown into pTest
    as best as possible

Arguments:

    pKnown - list of known values
    pTest - list of ranges
    pMatchCount - set to be number of resources matched

Return Value:

    NO_LC_MATCH  if no correlation (not a single known matches)
    LC_MATCH_SUPERSET if at least one known matches, but some known's don't
    LC_MATCH_SUBSET if all known matches, but there are some range-items unmatched
    LC_MATCH if all known matches and all range-items match
    ORDERED_LC_MATCH if match, and match is in order

    pKnown->CrossLink entries point to matching entries in pTest
    pTest->CrossLink entries point to matching entries in pKnown

--*/
{
    PRDE_LIST pResList = NULL;
    PRDE_LIST Node;
    PRESDES_ENTRY pKnownEntry;
    PRESDES_ENTRY pTestEntry;
    ULONG Success = NO_LC_MATCH;
    ULONG Depth = 0;
    BOOL SomeKnownMatched = FALSE;
    BOOL SomeKnownUnMatched = FALSE;
    BOOL SomeTestMatched = FALSE;
    BOOL SomeTestUnMatched = FALSE;
    BOOL Ordered = TRUE;
    ULONG MatchCount = 0;

    if (pKnown == NULL) {
        goto Final;
    }

    //
    // reset
    //
    for(pKnownEntry = pKnown; pKnownEntry != NULL ;pKnownEntry = pKnownEntry->Next) {
        pKnownEntry->CrossLink = NULL;
    }

    for(pTestEntry = pTest; pTestEntry != NULL ;pTestEntry = pTestEntry->Next) {
        pTestEntry->CrossLink = NULL;
    }

    pResList = pRDEList_Alloc();

    if (pResList == NULL) {
        goto Final;
    }

    //
    // make all resources available
    // this gives us a work list without destroying original list
    //
    for(pTestEntry = pTest; pTestEntry != NULL ;pTestEntry = pTestEntry->Next) {
        Node = pRDEList_Alloc();

        if (Node == NULL) {
            goto Final;
        }
        Node->Entry = pTestEntry;
        pRDEList_AddTail(pResList,Node);
    }

    MatchCount = pTryMatch(pKnown,pResList,&Depth);

    if (MatchCount ==0) {
        //
        // no match
        //
        goto Final;
    }

    //
    // pKnown now has it's Cross-Link's set to determine success of this match
    //
    // consider NO_LC_MATCH, LC_MATCH_SUPERSET and ORDERED_LC_MATCH cases
    //

    pKnownEntry = pKnown;
    pTestEntry = pTest;

    while (pKnownEntry) {
        if (pKnownEntry->CrossLink == NULL) {
            SomeKnownUnMatched = TRUE;
        } else {
            SomeKnownMatched = TRUE;    // we have at least one matched
            pKnownEntry->CrossLink->CrossLink = pKnownEntry; // cross-link test entries
            if (pKnownEntry->CrossLink != pTestEntry) {
                Ordered = FALSE;        // ordered compare lost
            } else {
                pTestEntry = pTestEntry->Next; // goto next test for ordered
            }
        }
        pKnownEntry = pKnownEntry->Next;
    }
    if (Ordered && pTestEntry != NULL) {
        Ordered = FALSE;
    }

    if (SomeKnownUnMatched) {
        if (SomeKnownMatched) {
            Success = LC_MATCH_SUPERSET;
        }
        goto Final;
    }

    if (Ordered) {
        Success = ORDERED_LC_MATCH;
        goto Final;
    }

    //
    // consider between LC_MATCH_SUBSET and LC_MATCH
    //
    pTestEntry = pTest;

    while (pTestEntry) {
        if (pTestEntry->CrossLink == NULL) {
            //
            // the first NULL CrossLink entry makes Known a Subset of Test
            Success = LC_MATCH_SUBSET;
            goto Final;
        }
        pTestEntry = pTestEntry->Next;
    }
    //
    // if we get here, there is an exact match
    //
    Success = LC_MATCH;

  Final:
    pRDEList_Destroy(pResList);

    if (pMatchCount != NULL) {
        *pMatchCount = MatchCount;
    }

    return Success;
}

ULONG
pCompareLogConf(
    IN LOG_CONF KnownLogConf,
    IN LOG_CONF TestLogConf,
    IN HMACHINE hMachine,
    OUT PULONG pMatchCount
    )

/*++

Routine Description:

    This routine compares two log confs and returns info about how well
    they match.
    This simply uses the pMergeResDesDataLists function to get match status

Arguments:

   KnownLogConf = First log conf to compare (fixed values)
   TestConfType = Second log conf to compare (range values)
   hMachine = Machine to compare on
   pMatchCount = number of resources matched

Return Value:

    As pMergeResDesDataLists

--*/

{
    PRESDES_ENTRY pKnownResList = NULL, pTestResList = NULL;
    ULONG Status;

    //
    // Retrieve the resources for each log conf
    //

    if (!pGetResDesDataList(KnownLogConf, &pKnownResList, TRUE,hMachine)) {
        return NO_LC_MATCH;
    }

    if (!pGetResDesDataList(TestLogConf, &pTestResList, TRUE,hMachine)) {
        pDeleteResDesDataList(pKnownResList);
        return NO_LC_MATCH;
    }

    Status = pMergeResDesDataLists(pKnownResList,pTestResList,pMatchCount);

    pDeleteResDesDataList(pKnownResList);
    pDeleteResDesDataList(pTestResList);

    return Status;
}

BOOL
pFindMatchingAllocConfig(
    IN  LPDMPROP_DATA lpdmpd
    )
{
    HWND     hDlg = lpdmpd->hDlg;
    ULONG    ulBasicLC = 0;
    ULONG    ulBasicCount = 0;
    BOOL     bFoundCorrectLC = FALSE;
    LOG_CONF LogConf;
    ULONG    lastMatchStatus = NO_LC_MATCH, bestMatchStatus = NO_LC_MATCH;
    UINT lastMatchCount;
    UINT bestMatchCount = 0;
    HMACHINE hMachine;

    hMachine = pGetMachine(lpdmpd);

    lpdmpd->MatchingLC = (LOG_CONF)0;
    lpdmpd->MatchingLCType = BASIC_LOG_CONF;

    //
    // Load the values associated with the allocated config in the list box,
    // but associate each with the resource requirements descriptor that it
    // originated from. To do this, we have to match the allocated config
    // with the basic/filtered config it is based on.
    //
    // NOTE: if we got here, then we know that an known config of some kind
    // exists (passed in as param) and that at least one basic/filtered config
    // exists. Further more, we know that the combobox has already been
    // filled in with a list of any basic/filtered configs and the lc handle
    // associated with them.
    //

    ulBasicCount = (ULONG)SendDlgItemMessage(hDlg,IDC_DEVRES_LOGCONFIGLIST,CB_GETCOUNT,
                                                        (WPARAM)0,(LPARAM)0);
    if (ulBasicCount == (ULONG)LB_ERR) {
        return FALSE;
    }

    for (ulBasicLC = 0 ; ulBasicLC < ulBasicCount; ulBasicLC++) {
        //
        // Retrieve the log conf handle
        //

        LogConf = (LOG_CONF)SendDlgItemMessage(hDlg, IDC_DEVRES_LOGCONFIGLIST,
                                               CB_GETITEMDATA, ulBasicLC, 0L);

        if (LogConf != 0) {
            //
            // Determine how good a match this requirements list is.
            //

            lastMatchStatus = pCompareLogConf(lpdmpd->CurrentLC, LogConf,hMachine,&lastMatchCount);
            if ((lastMatchStatus > bestMatchStatus)
                ||  ((bestMatchStatus == lastMatchStatus) && lastMatchCount > bestMatchCount)) {
                bestMatchCount = lastMatchCount;
                bestMatchStatus =lastMatchStatus;
                lpdmpd->MatchingLC = LogConf;
            }
        }
    }

    if (bestMatchStatus == NO_LC_MATCH || bestMatchStatus == LC_MATCH_SUBSET) {
        //
        // this doesn't follow any valid config
        //
        return FALSE;
    }

    lpdmpd->dwFlags &= ~(DMPROP_FLAG_PARTIAL_MATCH|DMPROP_FLAG_MATCH_OUT_OF_ORDER);
    if (bestMatchStatus != ORDERED_LC_MATCH) {
        //
        // If match status isn't ORDERED_LC_MATCH, then ordering of the resource descriptors
        // didn't match up.  Set a flag to indicate this, so that later on we'll know to handle
        // this specially.
        //
        lpdmpd->dwFlags |= DMPROP_FLAG_MATCH_OUT_OF_ORDER;
    } else if (bestMatchStatus < LC_MATCH) {
        //
        // match is partial
        //
        lpdmpd->dwFlags |= DMPROP_FLAG_PARTIAL_MATCH;
    }
    return TRUE;

} // LoadMatchingAllocConfig

BOOL
pGetMatchingResDes(
    IN ULONG      ulKnownValue,
    IN ULONG      ulKnownLen,
    IN ULONG      ulKnownEnd,
    IN RESOURCEID ResType,
    IN LOG_CONF   MatchingLogConf,
    OUT PRES_DES  pMatchingResDes,
    IN HMACHINE   hMachine
    )
/*++

Routine Description:

    BUGBUG!!! (jamiehun) this is obsolete, and should be removed from code-path

    This returns a res des that matches the specified values.

Arguments:

    ulKnownValue    Starting resource value to match against

    ulKnownLen      Length of resource to match against

    ResType         Type of resource to match against

    MatchnigLogConf Log conf to retreive potential matching res des from

    pMatchingResDes Supplies a pointer that on return contains a matching
                    res des if any.

Return Value:

   None.

--*/

{
    CONFIGRET   Status = CR_SUCCESS;
    RESOURCEID  Res;
    RES_DES     ResDes, ResDesTemp;
    ULONG       ulSize, ulValue = 0, ulLen = 0, ulEnd = 0, ulFlags = 0, i;
    PGENERIC_RESOURCE   pGenRes;
    BOOL        bMatch = FALSE;
    LPBYTE      pData = NULL;


    //
    // The MatchingLogConf is a requirements list. Loop through each res des
    // in the matching log conf until we find a res des that matches the
    // known res des values.
    //
    Status = CM_Get_Next_Res_Des_Ex(&ResDes, MatchingLogConf, ResType, &Res, 0,hMachine);

    while (Status == CR_SUCCESS) {
        //
        // Get res des data
        //
        if (CM_Get_Res_Des_Data_Size_Ex(&ulSize, ResDes, 0,hMachine) != CR_SUCCESS) {
            CM_Free_Res_Des_Handle(ResDes);
            break;
        }

        pData = MyMalloc(ulSize);
        if (pData == NULL) {
            CM_Free_Res_Des_Handle(ResDes);
            break;
        }

        if (CM_Get_Res_Des_Data_Ex(ResDes, pData, ulSize, 0,hMachine) != CR_SUCCESS) {
            CM_Free_Res_Des_Handle(ResDes);
            MyFree(pData);
            break;
        }

        if(pGetMatchingRange(ulKnownValue,ulKnownLen,pData,ResType,NULL,NULL,NULL)) {
            *pMatchingResDes = ResDes;
            bMatch = TRUE;
            MyFree(pData);
            goto MatchFound;
        }

        //
        // Get next res des in log conf
        //
        ResDesTemp = ResDes;
        Status = CM_Get_Next_Res_Des_Ex(&ResDes, ResDesTemp,
                                     ResType, &Res, 0,hMachine);

        CM_Free_Res_Des_Handle(ResDesTemp);
        MyFree(pData);
    }

    MatchFound:

    return bMatch;

} // GetMatchingResDes



//
// BUGBUG!!! conflict suppression hack 5/25/99 jamiehun (Win2k pre RC1)
// this stuff needs to be fixed proper
//

PTSTR
pGetRegString(
    IN HKEY hKey,
    IN PCTSTR regval
    )
/*++

Routine Description:

    Obtain and return a registry string allocated by MyMalloc
    return NULL if we couldn't retrieve string

Arguments:

    hKey - key to retrieve string from
    regval - value to retrieve

Return Value:

    copy of registry string, may be free'd with MyFree

--*/
{
    DWORD dwSize;
    DWORD dwType;
    PTSTR pSz;
    LONG res;

    dwType = 0;
    dwSize = 0;
    res = RegQueryValueEx(hKey,regval,NULL,&dwType,(PBYTE)NULL,&dwSize);
    if (res != ERROR_SUCCESS) {
        return NULL;
    }
    if (dwType != REG_SZ) {
        return NULL;
    }
    pSz = MyMalloc(dwSize);
    if (pSz == NULL) {
        return NULL;
    }
    res = RegQueryValueEx(hKey,regval,NULL,&dwType,(PBYTE)pSz,&dwSize);
    if (res != ERROR_SUCCESS) {
        MyFree(pSz);
        return NULL;
    }
    return pSz;
}

VOID
pFillCETags(
    IN PCONFLICT_EXCEPTIONS pExceptions,
    IN PCE_TAGS pTags,
    PTSTR pSz
    )
/*++

Routine Description:

    parse a list of tags into CE_TAGS structure
    adding the strings into the string table en-route
    note that this structure will be flexable and allow ',' or ';' seperator
    however when used in Exceptions string, we've already eliminated any ';'

    format is:
    <tag>,<tag>,<tag> or <tag>;<tag>;<tag>

Arguments:

    pExceptions - context information
    pTags - tag structure to fill in

Return Value:

    none

--*/
{
    static CE_TAGS DummyEntry = { -1 }; // if we write a new string, negative size count means this isn't a devnode entry

    MYASSERT(pTags->nTags == 0);

    while(pSz[0] && pTags->nTags < MAX_CE_TAGS) {
        if(pSz[0]==TEXT(',')||pSz[0]==TEXT(';')||pSz[0]<=TEXT(' ')) {
            pSz++;
        } else {
            PTSTR pOldSz = pSz;
            PTSTR pLastSpace = NULL;
            LONG id;
            while (pSz[0] && pSz[0]!=TEXT(';')&& pSz[0]!=TEXT(',')) {
                if (pSz[0]<=TEXT(' ')) {
                    if (pLastSpace==NULL) {
                        pLastSpace = pSz;
                    }
                } else {
                    pLastSpace = NULL;
                }
                pSz++;
            }
            //
            // pSz points to '\0', ';' or ','
            // pLastSpace points to any trailing WS
            // pOldSz points to start of string
            //
            if(pLastSpace==NULL) {
                pLastSpace = pSz;
            }
            if (pSz[0]) {
                pSz++;
            }
            pLastSpace[0]=TEXT('\0');
            //
            // pSz points to next string, pOldSz points to this string
            // add string to string table, place in list of tags
            //
            id = StringTableAddStringEx(pExceptions->ceTagMap,pOldSz,STRTAB_CASE_INSENSITIVE|STRTAB_BUFFER_WRITEABLE,&DummyEntry,sizeof(DummyEntry));
            if (id>=0) {
                pTags->Tag[pTags->nTags++] = id;
            }
        }
    }
}

PCE_ENTRY
pScanConflictExceptionEntry(
    IN PCONFLICT_EXCEPTIONS pExceptions,
    PTSTR pSz
    )
/*++

Routine Description:

    obtains conflict exception info from string

    format is:
    (1)  <tags>          - always ignore tag for any type of conflict
    (2)  <rt>:<tags>     - ignore tag for <rt> resource type
    (3)  <rt>@x:<tags>   - IRQ/DMA - specfic
    (4)  <rt>@x-y:<tags> - IO/MEM - range

    <tags>  are a comma-sep list of tags <tag>,<tag>,<tag>

Arguments:

    pExceptions - context information
    pSz - string to parse

Return Value:

    CE_ENTRY structure if this is a valid descriptor

--*/
{
    PTSTR brk;
    PCE_ENTRY pEntry;
    TCHAR rt[5];
    int c;

    while (pSz[0] && pSz[0]<=TEXT(' ')) {
        pSz++;
    }
    if (!pSz[0]) {
        return NULL;
    }

    pEntry = MyMalloc(sizeof(CE_ENTRY));

    if (pEntry == NULL) {
        return NULL;
    }
    ZeroMemory(pEntry,sizeof(CE_ENTRY));

    brk = _tcschr(pSz,TEXT(':'));

    if(!brk) {
        //
        // treat as tags only
        //
        pEntry->resType = ResType_None;
    } else {
        //
        // determine resource type
        //
        for(c=0;_istalpha(pSz[0]) && c<(sizeof(rt)/sizeof(TCHAR)-1);c++,pSz++) {
            rt[c] = _totupper(pSz[0]);
        }
        rt[c] = 0;
        while (pSz[0] && pSz[0]<=TEXT(' ')) {
            pSz++;
        }
        if (pSz[0]!=TEXT(':') && pSz[0]!=TEXT('@')) {
            MyFree(pEntry);
            return NULL;
        } else if (lstrcmp(rt,CE_RES_IO)==0) {
            pEntry->resType = ResType_IO;
        } else if (lstrcmp(rt,CE_RES_MEM)==0) {
            pEntry->resType = ResType_Mem;
        } else if (lstrcmp(rt,CE_RES_IRQ)==0) {
            pEntry->resType = ResType_IRQ;
        } else if (lstrcmp(rt,CE_RES_DMA)==0) {
            pEntry->resType = ResType_DMA;
        } else {
            MyFree(pEntry);
            return NULL;
        }
        if (pSz[0]!=TEXT('@')) {
            //
            // no range follows
            //
            pEntry->resStart = (ULONG)0;
            pEntry->resEnd = (ULONG)(-1);
        } else {
            //
            // @x[-y]:
            //
            ULONG x;
            ULONG y;
            PTSTR i;

            pSz++; // past @

            while (pSz[0] && pSz[0]<=TEXT(' ')) {
                pSz++;
            }
            i = pSz;
            x = _tcstoul(pSz,&i,0);
            if (i==pSz) {
                MyFree(pEntry);
                return NULL;
            }
            pSz = i;
            while (pSz[0] && pSz[0]<=TEXT(' ')) {
                pSz++;
            }
            if (pSz[0]==TEXT('-')) {
                //
                // -y
                //
                pSz++;
                while (pSz[0] && pSz[0]<=TEXT(' ')) {
                    pSz++;
                }
                i = pSz;
                y = _tcstoul(pSz,&i,0);
                if (i==pSz || y<x) {
                    MyFree(pEntry);
                    return NULL;
                }
                pSz = i;
                while (pSz[0] && pSz[0]<=TEXT(' ')) {
                    pSz++;
                }
            } else {
                y = x;
            }
            pEntry->resStart = x;
            pEntry->resEnd = y;
        }
        if (pSz[0] != TEXT(':')) {
            MyFree(pEntry);
            return NULL;
        }
        pSz ++; // skip past colon
    }
    //
    // at this point, expect a list of tags
    // each tag terminated by a comma
    //
    pFillCETags(pExceptions,&pEntry->tags,pSz);
    if (!pEntry->tags.nTags) {
        MyFree(pEntry);
        return NULL;
    }
    return pEntry;
}

PCONFLICT_EXCEPTIONS pLoadConflictExceptions(
    IN LPDMPROP_DATA lpdmpd
    )
/*++

Routine Description:

    Load the string "ResourcePickerExceptions" if any
    create a context structure for eliminating false conflicts
    this is one huge hack.

Arguments:

    lpdmpd - Context data

Return Value:

    CONFLICT_EXCEPTIONS structure, if "active" contains a string table and a list of resource exceptions

--*/
{
    PCONFLICT_EXCEPTIONS pExceptions;
    PCE_ENTRY pEntry;
    BOOL bStatus;
    HKEY hDevRegKey;
    PTSTR pSz;
    PTSTR pScanSz;
    PTSTR pOldSz;
    PCE_TAGS pTags;

    //
    // we always create the structure, so we will avoid looking for registry info every time
    //
    pExceptions = MyMalloc(sizeof(CONFLICT_EXCEPTIONS));
    if (pExceptions == NULL) {
        return NULL;
    }
    ZeroMemory(pExceptions,sizeof(CONFLICT_EXCEPTIONS));

    hDevRegKey = SetupDiOpenDevRegKey(lpdmpd->hDevInfo,lpdmpd->lpdi,DICS_FLAG_GLOBAL,0,DIREG_DRV,KEY_READ);
    if (hDevRegKey == INVALID_HANDLE_VALUE) {
        //
        // can't find key, no conflict elimination
        //
        return pExceptions;
    }
    pSz = pGetRegString(hDevRegKey,REGSTR_VAL_RESOURCE_PICKER_EXCEPTIONS);
    RegCloseKey(hDevRegKey);
    if(pSz == NULL) {
        //
        // can't find key, no conflict elimination
        //
        return pExceptions;
    }

    //
    // now parse the string creating our context to eliminate false conflicts
    //

    pExceptions->ceTagMap = StringTableInitializeEx(sizeof(CE_TAGS),0);
    if (pExceptions->ceTagMap == NULL) {
        MyFree(pSz);
        return pExceptions;
    }

    pScanSz = pSz;

    while (pScanSz[0]) {
        if (pScanSz[0] == TEXT(';')) {
            pScanSz ++;
        } else {
            pOldSz = pScanSz;
            while (pScanSz[0] && pScanSz[0] != TEXT(';')) {
                pScanSz++;
            }
            if (pScanSz[0]) {
                pScanSz[0] = 0;
                pScanSz++;
            }
            pEntry = pScanConflictExceptionEntry(pExceptions,pOldSz);
            if (pEntry) {
                pEntry->Next = pExceptions->exceptions;
                pExceptions->exceptions = pEntry;
            }
        }
    }

    MyFree(pSz);
    return pExceptions;
}

VOID pFreeConflictExceptions(
    IN PCONFLICT_EXCEPTIONS pExceptions
    )
/*++

Routine Description:

    Releases memory used by PCONFLICT_EXCEPTIONS

Arguments:

    pExceptions     structure to release

Return Value:

   None.

--*/
{
    //
    // free the list
    //
    while (pExceptions->exceptions) {
        PCE_ENTRY pEntry = pExceptions->exceptions;
        pExceptions->exceptions = pEntry->Next;
        MyFree(pEntry);
    }
    //
    // free the string table
    //
    if (pExceptions->ceTagMap) {
        StringTableDestroy(pExceptions->ceTagMap);
    }
    MyFree(pExceptions);
}

BOOL pIsConflictException(
    IN LPDMPROP_DATA lpdmpd,
    IN PCONFLICT_EXCEPTIONS pExceptions,
    IN DEVINST devConflict,
    IN PCTSTR resDesc,
    IN RESOURCEID resType,
    IN ULONG resValue,
    IN ULONG resLength
    )
/*++

Routine Description:

    Load the string "ResourcePickerExceptions" if any
    create a context structure for eliminating false conflicts
    this is one huge hack.

Arguments:

    lpdmpd - Context data
    pExceptions - Cache of information
    devConflict - DEVINST that's shown to be conflicting with us, -1 if "unavailable" (tag = *)
    resType - type of resource that we tested
    resValue - value of resource that we tested
    resLength - length of resource that we tested

Return Value:

    TRUE if this is an exception

--*/
{
    HMACHINE hMachine;
    TCHAR DevNodeName[MAX_DEVNODE_ID_LEN];
    CE_TAGS tags;
    PCE_ENTRY pEntry;
    LONG tagent;
    LONG n,m;
    ULONG resEnd = resValue+(resLength-1);
    PTSTR pSz;
    HKEY hKey;

    //
    // if we're not doing any exceptions, get out ASAP
    //
    if (pExceptions->exceptions == NULL) {
        return FALSE;
    }

    hMachine = pGetMachine(lpdmpd);
    //
    // handle "reserved" case first
    //
    if (devConflict != -1) {
        //
        // obtain device instance string
        //
        if(CM_Get_Device_ID_Ex(devConflict,DevNodeName,MAX_DEVNODE_ID_LEN,0,hMachine)!=CR_SUCCESS) {
            devConflict = -1;
        }
    }
    if (devConflict == -1) {
        if (resDesc && resDesc[0]) {
            lstrcpy(DevNodeName,resDesc);
        } else {
            lstrcpy(DevNodeName,CE_TAG_RESERVED);
        }
    } else {
    }
    //
    // is this a brand-new devnodename ?
    //
    tags.nTags = 0;
    tagent = StringTableLookUpStringEx(pExceptions->ceTagMap,DevNodeName,STRTAB_CASE_INSENSITIVE|STRTAB_BUFFER_WRITEABLE,&tags,sizeof(tags));
    if(tagent<0 || tags.nTags<0) {
        //
        // this particular devnode hasn't been processed before, ouch time
        //
        ZeroMemory(&tags,sizeof(tags)); // default reserved case
        if (devConflict != -1) {
            //
            // we need to get regkey for this devnode
            // I could do this via setupapi, or cfgmgr
            // for efficiency, I'm going latter route
            //
            if(CM_Open_DevNode_Key_Ex(devConflict,
                     KEY_READ,
                     0,
                     RegDisposition_OpenExisting,
                     &hKey,
                     CM_REGISTRY_SOFTWARE,
                     hMachine) == CR_SUCCESS) {

                pSz = pGetRegString(hKey,REGSTR_VAL_RESOURCE_PICKER_TAGS);
                RegCloseKey(hKey);

                if (pSz) {
                    //
                    // now fill in tags
                    //
                    pFillCETags(pExceptions,&tags,pSz);
                    MyFree(pSz);
                }
            }
        }
        //
        // now write this back into the string table
        // this time, non-negative nTags indicates we've processed this once
        // we will re-write the extra-data
        //
        tagent = StringTableAddStringEx(pExceptions->ceTagMap,DevNodeName,STRTAB_CASE_INSENSITIVE|STRTAB_BUFFER_WRITEABLE|STRTAB_NEW_EXTRADATA,&tags,sizeof(tags));
    }
    if (tagent<0) {
        //
        // if this happens, we have other problems (out of memory)
        //
        return FALSE;
    }

    //
    // go through our list of exceptions
    // an exception that mentions tagent, or any of tags associated with tagent (in tags) is rejected
    // the policy in this routine is make this an exception if we can
    //

    for(pEntry=pExceptions->exceptions;pEntry;pEntry=pEntry->Next) {

        if (pEntry->resType != ResType_None) {
            //
            // we need to validate the resource
            //
            if (pEntry->resType != resType ||
                    pEntry->resStart > resValue ||
                    pEntry->resEnd < resEnd) {
                continue;
            }
        }
        for (n=0;n<pEntry->tags.nTags;n++) {
            if (pEntry->tags.Tag[n] == tagent) {
                MYTRACE(TEXT("Eliminated false conflict with %s type=%u, start=0x%08x, len=0x%08x\n"),DevNodeName,resType,resValue,resLength);
                return TRUE;    // hit (devnode itself, where devnode may also be "*")
            }
            for (m=0;m<tags.nTags;m++) {
                if (pEntry->tags.Tag[n] == tags.Tag[m]) {
                    MYTRACE(TEXT("Eliminated false conflict with %s (via tag %s) type=%u, start=0x%08x, len=0x%08x\n"),DevNodeName,pStringTableStringFromId(pExceptions->ceTagMap,tags.Tag[m]),resType,resValue,resLength);
                    return TRUE;    // hit on one of associated tags
                }
            }
        }
    }

    return FALSE;
}


