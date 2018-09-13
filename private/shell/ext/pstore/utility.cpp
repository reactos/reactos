

#include <windows.h>
#include <shlobj.h>

#include "pstore.h"

#include "utility.h"
#include "enumid.h"

LPCITEMIDLIST
SearchPidlByType(
    LPCITEMIDLIST pidl,
    DWORD dwPidlType
    )
/*++

    This function searches a pidl, looking for an entry of the type specified
    by the dwPidlType argument.

    On success, the return value in non-NULL.

--*/
{
    if(pidl == NULL)
        return NULL;

    //
    // travel through pidls in list, then suck out the type and compare
    //

    LPCITEMIDLIST pidlTemp = pidl;
    LPCITEMIDLIST pidlResult = NULL;

    while(pidlTemp->mkid.cb)
    {
        if(GetPidlType(pidlTemp) == dwPidlType) {
            pidlResult = pidlTemp;
            break;
        }

        pidlTemp = GetPidlNextItem(pidlTemp);
    }

    return pidlResult;
}

DWORD
GetLastPidlType(
    LPCITEMIDLIST pidl
    )
/*++

    This function traverses the items in the specified pidl until the end of
    the list, returning the type value associated with the last valid entry.

--*/
{
    if(pidl == NULL)
        return 0;

    //
    // travel to last pidl in list, then suck out the type
    //

    LPCITEMIDLIST pidlTemp = pidl;
    LPCITEMIDLIST pidlLast = pidlTemp;

    while(pidlTemp->mkid.cb)
    {
        pidlLast = pidlTemp;
        pidlTemp = GetPidlNextItem(pidlTemp);
    }

    return GetPidlType(pidlLast);
}

PST_KEY
GetLastPidlKeyType(
    LPCITEMIDLIST pidl
    )
/*++

    This function traverses the items in the specified pidl until the end of
    the list, returning the key type (PST_KEY) value associated with the last
    valid entry.

--*/
{
    if(pidl == NULL)
        return 0;

    //
    // travel to last pidl in list, then suck out the type
    //

    LPCITEMIDLIST pidlTemp = pidl;
    LPCITEMIDLIST pidlLast = pidlTemp;

    while(pidlTemp->mkid.cb)
    {
        pidlLast = pidlTemp;
        pidlTemp = GetPidlNextItem(pidlTemp);
    }

    return GetPidlKeyType(pidlLast);
}


GUID *
GetLastPidlGuid(
    LPCITEMIDLIST pidl
    )
/*++

    This function traverses the items in the specified pidl until the end of
    the list, returning a pointer to the GUID data associated with the last
    valid entry.

    The caller should make a copy of the data if it is to be used persistently.

--*/
{
    if(pidl == NULL)
        return 0;

    //
    // travel to last pidl in list, then suck out the guid
    //

    LPCITEMIDLIST pidlTemp = pidl;
    LPCITEMIDLIST pidlLast = pidlTemp;

    while(pidlTemp->mkid.cb)
    {
        pidlLast = pidlTemp;
        pidlTemp = GetPidlNextItem(pidlTemp);
    }

    return GetPidlGuid(pidlLast);
}

LPCWSTR
GetLastPidlText(
    LPCITEMIDLIST pidl
    )
/*++

    This function traverses the items in the specified pidl until the end of
    the list, returning a pointer to the text data associated with the last
    valid entry.

    The caller should make a copy of the data if it is to be used persistently.

--*/
{
    if(pidl == NULL)
        return 0;

    //
    // travel to last pidl in list, then suck out the guid
    //

    LPCITEMIDLIST pidlTemp = pidl;
    LPCITEMIDLIST pidlLast = pidlTemp;

    while(pidlTemp->mkid.cb)
    {
        pidlLast = pidlTemp;
        pidlTemp = GetPidlNextItem(pidlTemp);
    }

    return GetPidlText(pidlLast);
}



LPCWSTR
GetPidlText(
    LPCITEMIDLIST pidl
    )
/*++

    This helper routine returns the display text associated with the specified
    pidl.

    The caller should make a copy of the string if the string is for persistent
    use.

--*/
{
    LPPIDL_CONTENT pidlContent = (LPPIDL_CONTENT)&(pidl->mkid.abID);

    return (LPCWSTR)(pidlContent + 1);
}


GUID *
GetPidlGuid(
    LPCITEMIDLIST pidl
    )
/*++

    This helper routine is called by IShellFolder::CompareIDs()
    to get the GUID identifiers associated with the specified pidl.

    The caller should make a copy of the output buffer pointer if the item is
    for persistent use.

--*/
{
    LPPIDL_CONTENT pidlContent = (LPPIDL_CONTENT)&(pidl->mkid.abID);

    return &(pidlContent->guid);
}

DWORD
GetPidlType(
    LPCITEMIDLIST pidl
    )
/*++

    This function returns the type value associated with the specified pidl.

--*/
{
    if(pidl == NULL)
        return 0;

    LPPIDL_CONTENT pidlContent = (LPPIDL_CONTENT)&(pidl->mkid.abID);

    return pidlContent->dwType;
}

PST_KEY
GetPidlKeyType(
    LPCITEMIDLIST pidl
    )
/*++

    This function returns the key type associated with the specified pidl.

--*/
{
    if(pidl == NULL)
        return 0;

    LPPIDL_CONTENT pidlContent = (LPPIDL_CONTENT)&(pidl->mkid.abID);

    return pidlContent->KeyType;
}




LPCITEMIDLIST
GetPidlNextItem(
    LPCITEMIDLIST pidl
    )
/*++

    This function examines the specified pidl and returns to the caller
    a pointer to the next pidl entry.

--*/
{
    if(pidl == NULL)
        return NULL;

    return (LPCITEMIDLIST) (LPBYTE)(((LPBYTE)pidl) + pidl->mkid.cb);
}

UINT
GetPidlSize(
    LPCITEMIDLIST pidl
    )
/*++

    This function gets the total size associated with the specified pidl.
    This accounts for all items, item data, and the terminal entry.

--*/
{
    if(pidl == NULL)
        return 0;

    UINT cbTotal = 0;
    LPCITEMIDLIST pidlTemp = pidl;

    while(pidlTemp->mkid.cb)
    {
        cbTotal += pidlTemp->mkid.cb;
        pidlTemp = GetPidlNextItem(pidlTemp);
    }

    //
    // Requires a 16 bit zero value for the NULL terminator
    //

    cbTotal += 2 * sizeof(BYTE);

    return cbTotal;
}

LPITEMIDLIST
CopyPidl(
    LPMALLOC pMalloc,
    LPCITEMIDLIST pidlSource
    )
/*++

    This function copies the specified pidl to new storage allocated by the
    specified allocation interface.

    On success, the return value is non-NULL and points to the copy of the pidl.

--*/
{
    LPITEMIDLIST pidlTarget = NULL;
    UINT cbSource = 0;

    if(NULL == pidlSource)
        return NULL;

    //
    // Allocate the new pidl
    //

    cbSource = GetPidlSize(pidlSource);

    pidlTarget = (LPITEMIDLIST) pMalloc->Alloc(cbSource);
    if(pidlTarget == NULL)
        return NULL;

    // Copy the source to the target
    CopyMemory(pidlTarget, pidlSource, cbSource);

    return pidlTarget;
}

LPITEMIDLIST
CopyCatPidl(
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2
    )
/*++

    This function allocated sufficient storage for a copy of:
    the two specified pidls, catanated together.

    On success, the return value is non-NULL and points to the copy of the pidl.

--*/
{
    LPMALLOC pMalloc;
    LPITEMIDLIST pidlTarget;
    UINT cbSource1;
    UINT cbSource2;


    if( NOERROR != SHGetMalloc(&pMalloc) )
        return NULL;

    //
    // Allocate the new pidl
    //

    cbSource1 = GetPidlSize(pidl1);
    cbSource2 = GetPidlSize(pidl2);

    pidlTarget = (LPITEMIDLIST) pMalloc->Alloc(cbSource1 + cbSource2);

    if(pidlTarget != NULL) {


        //
        // Copy first pidl source to the target
        //

        if( cbSource1 )
            CopyMemory(pidlTarget, pidl1, cbSource1);
        else {

            //
            // no source pidl: insure zero termination for search.
            //

            ZeroMemory(pidlTarget, cbSource2);
        }

        //
        // find the null terminator
        //

        if( cbSource2 ) {
            LPCITEMIDLIST pidlTemp = pidlTarget;

            while(pidlTemp->mkid.cb)
            {
                pidlTemp = GetPidlNextItem(pidlTemp);
            }

            //
            // Copy second pidl source to target.
            //

            CopyMemory((LPBYTE)pidlTemp, pidl2, cbSource2);
        }
    }

    pMalloc->Release();

    return pidlTarget;
}

VOID
FreePidl(
    LPITEMIDLIST pidl
    )
{
    LPMALLOC pMalloc;

    if( NOERROR != SHGetMalloc(&pMalloc) )
        return;

    pMalloc->Free(pidl);
    pMalloc->Release();

}

