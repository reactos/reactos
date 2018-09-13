//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// cdfidl.cpp 
//
//   Cdf id list helper functions.
//
//   History:
//
//       3/19/97  edwardp   Created.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stdinc.h"
#include "cdfidl.h"
#include "xmlutil.h"
#include "winineti.h" // for MAX_CACHE_ENTRY_INFO_SIZE

//
// Helper functions
//


LPTSTR CDFIDL_GetUserName()
{
    static BOOL gunCalled = FALSE;
    static TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH] = TEXT("");

    if (!gunCalled)
    {
        char  szUserNameA[INTERNET_MAX_USER_NAME_LENGTH];
        szUserNameA[0] = 0;
        DWORD size = INTERNET_MAX_USER_NAME_LENGTH;
        GetUserNameA(szUserNameA, &size);
        SHAnsiToTChar(szUserNameA, szUserName, ARRAYSIZE(szUserName));
        gunCalled = TRUE;
    }
    return szUserName;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_Create ***
//
//
// Description:
//     Creates a cdf id list.
//
// Parameters:
//     [In]  pCdfItem - A pointer to cdf item data.
//
// Return:
//     A new cdf id list on success.
//     NULL otherwise.
//
// Comments:
//     This function builds a variable length cdf item id list.  The item id
//     consists of a fixed length initial section followed by two or more null
//     terminated strings.  It has the following form:
//
//         USHORT  cb               - Size in bytes of this cdf item id.
//         WORD    wVersion;        - Version number of this item id structure.
//         DWORD   dwId;            - Used to identify cdf item ids. Set to
//                                    0x0ed1964ed  
//         CDFITEMTYPE cdfItemType  - CDF_Folder, CDF_FolderLink or CDF_Link.
//         LONG    nIndex           - The object model index for this item.
//         TCHAR   szName[1];       - Two or more null terminated strings
//                                    beggining with the name of this item.
//         USHORT  next.cb          - The size of the next item in the list.
//                                    Set to zero to terminate the list.
//
//     It is the callers responsability to free the item id list.  This should
//     be done using the IMalloc returned from SHGetMalloc();
//
////////////////////////////////////////////////////////////////////////////////
PCDFITEMIDLIST
CDFIDL_Create(
    PCDFITEM pCdfItem
)
{
#ifdef UNIX
    TCHAR *pszTempName;
#endif
    ASSERT(pCdfItem);
    ASSERT(pCdfItem->bstrName);
    ASSERT(pCdfItem->bstrURL);

    PCDFITEMIDLIST pcdfidl = NULL;

    //
    // Get the number of chars of the name of the item including the terminating
    // null character.
    //

    USHORT cbName = StrLenW(pCdfItem->bstrName) + 1;

    //
    // Get the number of chars of the URL of the item including the terminating
    // null character.
    //

    USHORT cbURL = StrLenW(pCdfItem->bstrURL) + 1;

    //
    // Calculate the total size of the cdf item id in bytes.  When calculating the size
    // of a cdf item id one TCHAR should be subtracted to account for the TCHAR
    // szName[1] included in the CDFITEMID struct definition.
    //

    USHORT cbItemId = sizeof(CDFITEMID) + (cbName + cbURL) * sizeof(TCHAR) - sizeof(TCHAR);

#ifdef UNIX
    cbItemId = ALIGN4(cbItemId);
#endif /* !UNIX */

    //
    // Item ids must allocated by the shell's IMalloc interface.
    //

    IMalloc* pIMalloc;

    HRESULT hr = SHGetMalloc(&pIMalloc);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIMalloc);

        //
        // An item id *list* must be NULL terminated so an additional USHORT is
        // allocated to hold the terminating NULL.
        //
        pcdfidl = (PCDFITEMIDLIST)pIMalloc->Alloc(cbItemId + sizeof(USHORT));

        if (pcdfidl)
        {
            //
            // NULL terminate the list.
            //

            *((UNALIGNED USHORT*) ( ((LPBYTE)pcdfidl) + cbItemId )) = 0;

#ifdef UNIX
            USHORT cbActaulItemId = sizeof(CDFITEMID) + cbName + cbURL - sizeof(TCHAR);
            if(cbActaulItemId < cbItemId)
                memset((LPBYTE)pcdfidl + cbActaulItemId, 0, cbItemId-cbActaulItemId);
#endif /* UNIX */


            //
            // Fill in the data shared by all cdf item ids.
            //

            pcdfidl->mkid.cb       = cbItemId;
            pcdfidl->mkid.wVersion = CDFITEMID_VERSION;
            pcdfidl->mkid.dwId     = CDFITEMID_ID;

            //
            // Set the data that is specific to this cdf item id.
            //

            pcdfidl->mkid.cdfItemType = pCdfItem->cdfItemType;
            pcdfidl->mkid.nIndex      = pCdfItem->nIndex;

            //
            // REVIEW: Need WSTR to TSTR conversion.
            //

#ifndef UNIX
            SHUnicodeToTChar(pCdfItem->bstrName, pcdfidl->mkid.szName, cbName);
            SHUnicodeToTChar(pCdfItem->bstrURL, pcdfidl->mkid.szName + cbName,
                           cbURL);         
#else
        pszTempName = (LPTSTR)ALIGN4((ULONG)(pcdfidl->mkid.szName));
            SHUnicodeToTChar(pCdfItem->bstrName, pszTempName, cbName);
        pszTempName = (LPTSTR)((ULONG)(pcdfidl->mkid.szName+cbName));
            SHUnicodeToTChar(pCdfItem->bstrURL, pszTempName,
                           cbURL);
#endif

        }
        else
        {
            pcdfidl = NULL;
        }

        pIMalloc->Release();
    }

    ASSERT(CDFIDL_IsValid(pcdfidl) || NULL == pcdfidl);

    return pcdfidl;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_CreateFromXMLElement ***
//
//
// Description:
//     Creates a cdf item id list from a xml element.
//
// Parameters:
//     [In]  pIXMLElement - A pointer to the xml element.
//     [In]  nIndex       - The index value used to set the cdfidl index field.
//
// Return:
//     A poniter to a new cdf item id list if successful.
//     NULL otherwise.
//
// Comments:
//     The caller is responsible for freeing the returned id list.
//
////////////////////////////////////////////////////////////////////////////////
PCDFITEMIDLIST
CDFIDL_CreateFromXMLElement(
    IXMLElement* pIXMLElement,
    ULONG nIndex
)
{
    ASSERT(pIXMLElement);

    PCDFITEMIDLIST pcdfidl = NULL;

    CDFITEM cdfItem;

    if (cdfItem.bstrName = XML_GetAttribute(pIXMLElement, XML_TITLE))
    {
        if (cdfItem.bstrURL  = XML_GetAttribute(pIXMLElement, XML_HREF))
        {
            cdfItem.nIndex = nIndex;

            if (INDEX_CHANNEL_LINK == nIndex)
            {
                cdfItem.cdfItemType = CDF_FolderLink;
            }
            else
            {
                cdfItem.cdfItemType = XML_IsFolder(pIXMLElement) ? CDF_Folder :
                                                                   CDF_Link;
            }

            pcdfidl = CDFIDL_Create(&cdfItem);

            SysFreeString(cdfItem.bstrURL);
        }

        SysFreeString(cdfItem.bstrName);
    }

    ASSERT(CDFIDL_IsValid(pcdfidl) || NULL == pcdfidl);

    return pcdfidl;
}


//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_CreateFolderPidl ***
//
//
// Description: creates a special folder pidl
//     
//
// Parameters:
//     [In]  pcdfidl - Pointer to the cdf id list to be created from
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
PCDFITEMIDLIST
CDFIDL_CreateFolderPidl(
    PCDFITEMIDLIST pcdfidl
)
{
    ASSERT(CDFIDL_IsValid(pcdfidl));

    PCDFITEMIDLIST pcdfidlRet = (PCDFITEMIDLIST)ILClone((LPITEMIDLIST)pcdfidl);

    if (pcdfidlRet)
    {
        ((PCDFITEMID)pcdfidlRet)->nIndex = INDEX_CHANNEL_LINK;
        ((PCDFITEMID)pcdfidlRet)->cdfItemType = CDF_FolderLink; //CDF_Link instead?
    }
    ASSERT(CDFIDL_IsValid(pcdfidlRet));
   
    return pcdfidlRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_Free ***
//
//
// Description:
//     Free the given cdf item id list.
//
// Parameters:
//     [In]  pcdfidl - Pointer to the cdf id list to be freed.
//
// Return:
//     No return value.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
void
CDFIDL_Free(
    PCDFITEMIDLIST pcdfidl
)
{
    ASSERT(CDFIDL_IsValid(pcdfidl));

    IMalloc *pIMalloc;

    if (SUCCEEDED(SHGetMalloc(&pIMalloc)))
    {
        ASSERT(pIMalloc);
        ASSERT(pIMalloc->DidAlloc(pcdfidl));
        
        pIMalloc->Free(pcdfidl);

        pIMalloc->Release();
    }
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_GetDisplayName ***
//
//
// Description:
//     Gets the name stored in the given cdf item id list.
//
// Parameters:
//     [In]  pcdfidl - A pointer to a cdf item id list.
//     [Out] pName - A pointer to a STRRET structure.  STRRET has the following
//                   structure:
//                       UINT uType             - STRRET_CSTR, _OFFSET or _WSTR
//                       union {
//                           LPWSTR pOleStr;
//                           UINT   uOffset;
//                           char   cStr[MAX_PATH];
//                       }
//
// Return:
//     S_OK on success.  E_Fail otherwise.
//
// Comments:
//     This function returns the name as an offset of the string from the start
//     of the cdf item id list.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT
CDFIDL_GetDisplayName(
    PCDFITEMIDLIST pcdfidl,
    LPSTRRET pName
)
{
    ASSERT(CDFIDL_IsValid(pcdfidl));
    ASSERT(pName);

#ifdef UNICODE
#ifdef SHDOCVW_UNICODE     //open this when shdocvw becomes unicode
    IMalloc* pIMalloc;

    HRESULT hr = SHGetMalloc(&pIMalloc);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIMalloc);
        pName->uType = STRRET_WSTR;
        LPTSTR pszName = CDFIDL_GetName(pcdfidl);

        pName->pOleStr = (LPWSTR)pIMalloc->Alloc(ARRAYSIZE(pszName));
        if (pName->pOleStr)
            lstrcpyW(pName->pOleStr, pszName);
        pIMalloc->Release();
    }
#else
    pName->uType = STRRET_CSTR;
    LPTSTR pszName = CDFIDL_GetName(pcdfidl);

    SHTCharToAnsi(pszName, pName->cStr, ARRAYSIZE(pName->cStr));
#endif
#else
    pName->uType = STRRET_OFFSET;
    pName->uOffset = (UINT)((LPBYTE)CDFIDL_GetName(pcdfidl) - (LPBYTE)pcdfidl);
#endif
    return S_OK;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_GetName ***
//
//
// Description:
//     Gets a pointer to the URL stored in the given cdf item id list.
//
// Parameters:
//     [In]  pcdfidl - A pointer to a cdf item id list.
//
// Return:
//     A LPTSTR to the Name stored in the pidl.
//
// Comments:
//     This function returns a pointer to the Name in the cdf item id list.
//     The pointer is valid for the life of the item id list.  The caller is
//     resposible for maintaining the item id list and for not using the
//     the returned pointer after the id list is freed.
//
//     The name returned is the name of the last item in the list. 
//
////////////////////////////////////////////////////////////////////////////////
LPTSTR
CDFIDL_GetName(
    PCDFITEMIDLIST pcdfidl
)
{
    ASSERT(CDFIDL_IsValid(pcdfidl));

    pcdfidl = (PCDFITEMIDLIST)ILFindLastID((LPITEMIDLIST)pcdfidl);
    
    return CDFIDL_GetNameId(&pcdfidl->mkid);
}

LPTSTR
CDFIDL_GetNameId(
    PCDFITEMID pcdfid
)
{
    ASSERT(pcdfid);

#ifndef UNIX
    return pcdfid->szName;
#else
    return (LPTSTR)(ALIGN4((ULONG)pcdfid->szName));
#endif
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_GetURL ***
//
//
// Description:
//     Gets a pointer to the URL stored in the given cdf item id list.
//
// Parameters:
//     [In]  pcdfidl - A pointer to a cdf item id list.
//
// Return:
//     A LPTSTR to the URL value for the given pcdfidl.
//
// Comments:
//     This function returns a pointer to the URL in the cdf item id list.  The
//     pointer is valid for the life of the item id list.  The caller is
//     resposible for maintaining the item id list and for not using the
//     the returned pointer after the id list is freed. 
//
//     The URL returned is the URL of the last item in the list.
//
////////////////////////////////////////////////////////////////////////////////
LPTSTR
CDFIDL_GetURL(
    PCDFITEMIDLIST pcdfidl
)
{
    ASSERT(CDFIDL_IsValid(pcdfidl));

    //
    // Get the first string after the name.
    //

    LPTSTR szURL = CDFIDL_GetName(pcdfidl);

    while (*szURL++);

    return szURL;
}

LPTSTR
CDFIDL_GetURLId(
    PCDFITEMID pcdfid
)
{
    ASSERT(pcdfid);

    //
    // Get the first string after the name.
    //

    LPTSTR szURL = CDFIDL_GetNameId(pcdfid);

    while (*szURL++);

    return szURL;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_GetIndex ***
//
//
// Description:
//     Returns the index item of the given cdf id list.
//
// Parameters:
//     [In]  pcdfidl - Pointer to the cdf item id list.
//
// Return:
//     Returns the index item of the given id list.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
ULONG
CDFIDL_GetIndex(
    PCDFITEMIDLIST pcdfidl
)
{
    pcdfidl = (PCDFITEMIDLIST)ILFindLastID((LPITEMIDLIST)pcdfidl);

    return CDFIDL_GetIndexId(&pcdfidl->mkid);
}

ULONG
CDFIDL_GetIndexId(
    PCDFITEMID pcdfid
)
{
    return pcdfid->nIndex;
}

#define ASTR_HISTORY_PREFIX TEXT("Visited: ")

//
// Looks the URL up in the cache to see if the user has ever read this url
//
// REVIEW
// REVIEW - Should probably use IUrlStorage instead of constructing the 
// REVIEW
// history cache URL on the fly
//
BOOL
CDFIDL_IsUnreadURL(
    LPTSTR szUrl
)
{
    DWORD dwLen;
    
    //
    //  Canonicalize the input url.
    // 
    TCHAR szCanonicalizedUrl[INTERNET_MAX_URL_LENGTH];
    dwLen = INTERNET_MAX_URL_LENGTH;
    InternetCanonicalizeUrl(szUrl, szCanonicalizedUrl, &dwLen, 0);
    
    //
    // Build a string that is the URL prefixed with VISITED: and the UserName
    //
    TCHAR szVisited[
        INTERNET_MAX_USER_NAME_LENGTH+
        1+
        INTERNET_MAX_URL_LENGTH+
        ARRAYSIZE(ASTR_HISTORY_PREFIX)];
 
    StrCpy(szVisited, ASTR_HISTORY_PREFIX);
    StrCatN(szVisited, CDFIDL_GetUserName(), ARRAYSIZE(szVisited));
    int len = StrLen(szVisited);
    StrCpyN(szVisited + len++, TEXT("@"), ARRAYSIZE(szVisited) - len);
    //len++;        //bug, this will introduce a null char...
    StrCpyN(szVisited + len++, szCanonicalizedUrl, ARRAYSIZE(szVisited) - len);          

    // Check for trailing slash and eliminate, copied from shdocvw\urlhist.cpp
    LPTSTR pszT = CharPrev(szVisited, szVisited + lstrlen(szVisited));
    if (*pszT == TEXT('/'))
    {
        ASSERT(lstrlen(pszT) == 1);
        *pszT = 0;
    }

    //
    // If the VISITED: entry does not exist in the cache assume url is unread
    //
#ifndef UNIX
    BYTE visitedCEI[MAX_CACHE_ENTRY_INFO_SIZE];
    LPINTERNET_CACHE_ENTRY_INFO pVisitedCEI = (LPINTERNET_CACHE_ENTRY_INFO)visitedCEI;
#else
    union
    {
        double align8;
        BYTE visitedCEI[MAX_CACHE_ENTRY_INFO_SIZE];
    } alignedvisitedCEI;
    LPINTERNET_CACHE_ENTRY_INFO pVisitedCEI = (LPINTERNET_CACHE_ENTRY_INFO)&alignedvisitedCEI;
#endif /* UNIX */
    dwLen = MAX_CACHE_ENTRY_INFO_SIZE;

    if (GetUrlCacheEntryInfo(szVisited, pVisitedCEI, &dwLen) == FALSE)
    {
        return TRUE;
    }
    else
    {
        //
        // URL has been visited, but it still may be unread if the page has
        // been placed in the cache by the infodelivery mechanism
        //
#ifndef UNIX
        BYTE urlCEI[MAX_CACHE_ENTRY_INFO_SIZE];
        LPINTERNET_CACHE_ENTRY_INFO pUrlCEI = (LPINTERNET_CACHE_ENTRY_INFO)urlCEI;
#else
        union
        {
            double align8;
            BYTE urlCEI[MAX_CACHE_ENTRY_INFO_SIZE];
        } alignedurlCEI;
        LPINTERNET_CACHE_ENTRY_INFO pUrlCEI = (LPINTERNET_CACHE_ENTRY_INFO)&alignedurlCEI;
#endif /* UNIX */

        dwLen = MAX_CACHE_ENTRY_INFO_SIZE;

        if (GetUrlCacheEntryInfo(szCanonicalizedUrl, pUrlCEI, &dwLen) == FALSE)
        {
            return FALSE; // no url cache entry but url was visited so mark read
        }
        else
        {
            //
            // If the url has been modified after the time of the visited 
            // record then url is unread
            //
            if (CompareFileTime(&pUrlCEI->LastModifiedTime,
                                &pVisitedCEI->LastModifiedTime) > 0)
            {
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
    }
}

//
// Looks the URL up in the cache. TRUE if it is and FALSE otherwise
//
BOOL
CDFIDL_IsCachedURL(
    LPWSTR wszUrl
)
{

    BOOL  fCached;
    TCHAR szUrlT[INTERNET_MAX_URL_LENGTH];


    //
    //  Canonicalize the input url.
    // 
    
    if (SHUnicodeToTChar(wszUrl, szUrlT, ARRAYSIZE(szUrlT)))
    {
        URL_COMPONENTS uc;
 
        memset(&uc, 0, sizeof(uc));
        uc.dwStructSize = sizeof(URL_COMPONENTS);
        uc.dwSchemeLength = 1;
        if (InternetCrackUrl(szUrlT, 0, 0, &uc))
        {
            // BUGBUG: zekel should look at this
            TCHAR *pchLoc = StrChr(szUrlT, TEXT('#'));
            if (pchLoc)
                *pchLoc = TEXT('\0');
        

            fCached = GetUrlCacheEntryInfoEx(szUrlT, NULL, NULL, NULL, NULL, NULL, 0);
            if(fCached)
            {
                return TRUE;
            }
            else
            {
                TCHAR szCanonicalizedUrlT[INTERNET_MAX_URL_LENGTH];
                DWORD dwLen = INTERNET_MAX_URL_LENGTH;
                InternetCanonicalizeUrl(szUrlT, szCanonicalizedUrlT, &dwLen, 0);

                fCached = GetUrlCacheEntryInfoEx(szCanonicalizedUrlT, NULL, NULL, NULL, NULL, NULL, 0);

                if(fCached)
                    return TRUE;
            }
        }
    }
    
    return FALSE;


}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_GetAttributes ***
//
//
// Description:
//     Returns the attributes item of the given cdf item id list.
//
// Parameters:
//     [In]  pIXMLElementCollectionparent - The containing element collection.
//     [In]  pcdfidl                      - A pointer to the cdf item id list.
//     [In]  fAttributesFilter            - Determines which flags to bother 
//                                          looking at
//
// Return:
//     The attributes of the given id list.
//     Zero on failure.  Note: Zero is a valid attribute value.
//
// Comments:
//     The attribute flags returned by this function can be used directly as a
//     return value by IShellFolder->GetAttributesOf().
//
////////////////////////////////////////////////////////////////////////////////
ULONG
CDFIDL_GetAttributes(
    IXMLElementCollection* pIXMLElementCollectionParent,
    PCDFITEMIDLIST pcdfidl,
    ULONG fAttributesFilter
)
{
    ASSERT(pIXMLElementCollectionParent);
    ASSERT(CDFIDL_IsValid(pcdfidl));
    ASSERT(ILIsEmpty(_ILNext((LPITEMIDLIST)pcdfidl)));

    //
    // REVIEW:  Need to properly determine shell attributes of cdf items.
    //

    ULONG uRet;

    if (CDFIDL_IsFolderId(&pcdfidl->mkid))
    {
        uRet = SFGAO_FOLDER | SFGAO_CANLINK;

        //  If we weren't asked for HASSUBFOLDER don't bother looking for it
        //  This should be a win in non tree views (ie. std open mode)
        if ((SFGAO_HASSUBFOLDER & fAttributesFilter) &&
            pIXMLElementCollectionParent && 
            XML_ChildContainsFolder(pIXMLElementCollectionParent,
                                    CDFIDL_GetIndex(pcdfidl)))
        {
            uRet |= SFGAO_HASSUBFOLDER;
        }
    }
    else
    {
        uRet = SFGAO_CANLINK;
        //  If we weren't asked for NEWCONTENT don't bother looking for it
        //  This will be a win in non channel pane views.
        //  BUGBUG: Can't test for SFGAO_NEWCONTENT since shell is never
        //  expressing interest in it!
        if (/*(SFGAO_NEWCONTENT & fAttributeFilter) && */
            CDFIDL_IsUnreadURL(CDFIDL_GetURL(pcdfidl)))
        {
            uRet |= SFGAO_NEWCONTENT;
        }
    }

    return uRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_Compare ***
//
//
// Description:
//     Compares two cdf item id lists
//
// Parameters:
//     [In]  pcdfidl1 - A pointer to the first item id list to compare.
//     [In]  pcdfidl2 - A pointer to the second item id list to compare.
//
// Return:
//     -1 if item 1 comes before item 2.
//      0 if the items are equal.
//      1 if item 2 comes before item 1.
//
// Comments:
//      Sort Order:
//          1) Use the CompareId result of the first items in the lists.
//          2) If 1) returns 0.  Compare the next two items in the lists.
//          3) If both list are empty. They are equal.
//          4) The shorter id list comes first.
//
////////////////////////////////////////////////////////////////////////////////
SHORT
CDFIDL_Compare(
    PCDFITEMIDLIST pcdfidl1,
    PCDFITEMIDLIST pcdfidl2
)
{
    ASSERT(CDFIDL_IsValid(pcdfidl1));
    ASSERT(CDFIDL_IsValid(pcdfidl2));

    SHORT sRet;

    sRet = CDFIDL_CompareId(&pcdfidl1->mkid, &pcdfidl2->mkid);

    if (0 == sRet)
    {
        if (!ILIsEmpty(_ILNext(pcdfidl1)) && !ILIsEmpty(_ILNext(pcdfidl2)))
        {
            sRet = CDFIDL_Compare((PCDFITEMIDLIST)_ILNext(pcdfidl1), 
                                  (PCDFITEMIDLIST)_ILNext(pcdfidl2));
        }
        else if(!ILIsEmpty(_ILNext(pcdfidl1)) && ILIsEmpty(_ILNext(pcdfidl2)))
        {
            sRet = 1;
        }
        else if (ILIsEmpty(_ILNext(pcdfidl1)) && !ILIsEmpty(_ILNext(pcdfidl2)))
        {
            sRet = -1;
        }
    }

    return sRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_CompareId ***
//
//
// Description:
//     Compares two item cdf item ids.
//
// Parameters:
//     [In]  pcdfid1 - A pointer to the first item id to compare.
//     [In]  pcdfid2 - A pointer to the second item id to compare.
//
// Return:
//     -1 if item 1 comes before item 2.
//      0 if the items are the same.
//      1 if item 2 comes before item 1.
//
// Comments:
//     Sort Order:
//         1) CDF_FolderLink (Essentially an URL for the current folder). These
//            have an index of -1.
//         2) Everything else accoring to its order in the CDF.  These have
//            a zero-based index.
//         3) Non CDF items  (should't have any).
//
////////////////////////////////////////////////////////////////////////////////
SHORT
CDFIDL_CompareId(
    PCDFITEMID pcdfid1,
    PCDFITEMID pcdfid2
)
{
    ASSERT(CDFIDL_IsValidId(pcdfid1));
    ASSERT(CDFIDL_IsValidId(pcdfid2));

    SHORT sRet;

    if (pcdfid1->nIndex < pcdfid2->nIndex)
    {
        sRet = -1;
    }
    else if (pcdfid1->nIndex > pcdfid2->nIndex)
    {
        sRet = 1;
    }
    else
    {
        sRet =  (short) CompareString(LOCALE_USER_DEFAULT, 0, CDFIDL_GetNameId(pcdfid1),
                                      -1, CDFIDL_GetNameId(pcdfid2), -1);

        //
        // Note: CompareString returns 1 if S1 comes before S2, 2 if S1 is equal
        // to S2, 3 if S2 comes before S1 and 0 on error.
        //

        sRet = sRet ? sRet - 2 : 0;

        if (0 == sRet)
        {
            //
            // If the URLs aren't equal just pick one at random.
            //

            sRet = !StrEql(CDFIDL_GetURLId(pcdfid1), CDFIDL_GetURLId(pcdfid2));
            
            ASSERT((pcdfid1->cb == pcdfid2->cb) || 0 != sRet);
            ASSERT((pcdfid1->cdfItemType == pcdfid1->cdfItemType) || 0 != sRet);
        }
    }

    return sRet;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_IsValid ***
//
//
// Description:
//     Determines if the given pcdfidl is valid.
//
// Parameters:
//     pcdfid - A pointer to the cdf id to check.
//
// Return:
//     TRUE if the id is a cdf id.
//     FALSE otherwise.
//
// Comments:
//     An empty list is not valid.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
CDFIDL_IsValid(
    PCDFITEMIDLIST pcdfidl
)
{
    BOOL bRet;

    if (pcdfidl && (pcdfidl->mkid.cb > 0))
    {
        bRet = TRUE;

        while (pcdfidl->mkid.cb && bRet)
        {
            bRet = CDFIDL_IsValidId(&pcdfidl->mkid);
            pcdfidl = (PCDFITEMIDLIST)_ILNext((LPITEMIDLIST)pcdfidl);
        }
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}


//
// Inline helper functions.
//

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_IsValidId ***
//
//
// Description:
//     Inline function that returns TRUE if the given id is a pointer to a cdf
//     item id.
//
// Parameters:
//     pcdfid - A pointer to the cdf id to check.
//
// Return:
//     TRUE if the id is a cdf id.
//     FALSE otherwise.
//
// Comments:
//     This function is not completely safe.  If the first word pointed to 
//     is large but the memory block pointed to is smaller than a 8 bytes an
//     access violation will occur.  Also, if the first word is large enough and
//     the second DWORD is equal to CDFITEM_ID but the item isn't a cdf id a
//     false positive will occur.
//
////////////////////////////////////////////////////////////////////////////////
BOOL
CDFIDL_IsValidId(
    PCDFITEMID pcdfid
)
{
    ASSERT(pcdfid);

    return (pcdfid->cb >= (sizeof(CDFITEMID) +  sizeof(TCHAR)) && 
            pcdfid->dwId == CDFITEMID_ID                       &&
            CDFIDL_IsValidSize(pcdfid)                         &&
            CDFIDL_IsValidType(pcdfid)                         &&
            CDFIDL_IsValidIndex(pcdfid)                        &&
            CDFIDL_IsValidStrings(pcdfid)                         );
}

inline
BOOL
CDFIDL_IsValidSize(
    PCDFITEMID pcdfid
)
{
    int cbName = (StrLen(CDFIDL_GetNameId(pcdfid)) + 1) * 
                 sizeof(TCHAR);

    int cbURL  = (StrLen(CDFIDL_GetURLId(pcdfid)) + 1) *
                 sizeof(TCHAR);

#ifndef UNIX
    return (sizeof(CDFITEMID) - sizeof(TCHAR) + cbName + cbURL == pcdfid->cb);
#else
    return ((ALIGN4(sizeof(CDFITEMID) - sizeof(TCHAR) + cbName + cbURL)) == pcdfid->cb);
#endif /* !UNIX */
}

inline
BOOL
CDFIDL_IsValidType(
    PCDFITEMID pcdfid
)
{
    return ((CDF_Folder     == (CDFITEMTYPE)pcdfid->cdfItemType) ||
            (CDF_Link       == (CDFITEMTYPE)pcdfid->cdfItemType) ||
            (CDF_FolderLink == (CDFITEMTYPE)pcdfid->cdfItemType)   );
}

inline
BOOL
CDFIDL_IsValidIndex(
    PCDFITEMID pcdfid
)
{
    return (  pcdfid->nIndex >= 0
            || 
              (INDEX_CHANNEL_LINK == pcdfid->nIndex &&
              CDF_FolderLink == (CDFITEMTYPE)pcdfid->cdfItemType));
}

inline
BOOL
CDFIDL_IsValidStrings(
    PCDFITEMID pcdfid
)
{
    //
    // REVIEW: Validate pidl strings.
    //

    return TRUE;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_IsFolder ***
//
//
// Description:
//     Inline function that returns TRUE if the given cdfidl is a folder as far
//     as the shel is concerned.
//
// Parameters:
//     pcdfidl - The cdf item id list to check.
//
// Return:
//     TRUE if the cdf item id list is a folder.
//
// Comments:
//
//
////////////////////////////////////////////////////////////////////////////////
BOOL
CDFIDL_IsFolder(
    PCDFITEMIDLIST pcdfidl
)
{
    ASSERT(CDFIDL_IsValid(pcdfidl));

    pcdfidl = (PCDFITEMIDLIST)ILFindLastID((LPITEMIDLIST)pcdfidl);

    return CDFIDL_IsFolderId(&pcdfidl->mkid);
}

BOOL
CDFIDL_IsFolderId(
    PCDFITEMID pcdfid
)
{
    ASSERT(CDFIDL_IsValidId(pcdfid));

    return (CDF_Folder == (CDFITEMTYPE)pcdfid->cdfItemType);
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
//
// *** CDFIDL_NonCdfGetName ***
//
//
// Description:
//     Gets the name stored in the given non-cdf item id list.
//
// Parameters:
//     [In]  pcdfidl - A pointer to a cdf item id list.  Can be NULL.
//     [Out] pName - A pointer to a STRRET structure.  STRRET has the following
//                   structure:
//                       UINT uType             - STRRET_CSTR, _OFFSET or _WSTR
//                       union {
//                           LPWSTR pOleStr;
//                           UINT   uOffset;
//                           char   cStr[MAX_PATH];
//                       }
//
// Return:
//     S_OK on success.  E_Fail otherwise.
//
// Comments:
//     This function returns the name as a cString in the STRRET structure.
//
//     ILGetDisplayName returns the full path.  This function strips out the
//     filename sans extension.
//
////////////////////////////////////////////////////////////////////////////////
#if 0
HRESULT
CDFIDL_NonCdfGetDisplayName(
    LPCITEMIDLIST pidl,
    LPSTRRET pName
)
{
    ASSERT(pName);

    HRESULT hr;

    //
    // REVIEW:  Hack to get the name of a shell pidl.
    //

    if (ILGetDisplayName(pidl, pName->cStr))
    {
        TCHAR* p1 = pName->cStr;
        TCHAR* p2 = p1;

        while (*p1++);                          // Go to the end. 
        while (*--p1 != TEXT('\\'));            // Back to last backslash.
        while (TEXT('.') != (*p2++ = *++p1));   // Copy the name.
        *--p2 = TEXT('\0');                     // NULL terminate.

        pName->uType = STRRET_CSTR;

        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}
#endif
