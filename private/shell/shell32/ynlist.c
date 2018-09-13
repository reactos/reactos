#include "shellprv.h"

#define YNLIST_ALLOC    (2 * MAX_PATH * sizeof(TCHAR))

//
// Constructor - creates a YesNoList
//
void CreateYesNoList(PYNLIST pynl)
{
    ZeroMemory(pynl, sizeof(*pynl));
}

//
// Destructor - frees and destroys a YesNoList
//
void DestroyYesNoList(PYNLIST pynl)
{
    if (pynl->dlYes.pszzList)
        GlobalFree(pynl->dlYes.pszzList);
    if (pynl->dlNo.pszzList)
        GlobalFree(pynl->dlNo.pszzList);
    ZeroMemory(pynl, sizeof(*pynl));
}

//
// IsPathOfItem - determine if pszPath is on the path to pszItem
//
BOOL IsPathOfItem(LPCTSTR pszPath, LPCTSTR pszItem)
{
    //
    // Validate pszPath is the first
    // substring of pszItem.
    //
    while (*pszPath)
    {
        if (*pszPath != *pszItem)
        {
            return FALSE;
        }

        pszPath++;
        pszItem++;
    }

    //
    // pszPath is the path if pszItem is empty (exact match),
    // or pszItem is a directory separator.
    //
    return (*pszItem == TEXT('\\')) || (*pszItem == TEXT('\0'));
}

//
// IsInDirList - determines if DIRLIST contains
// the path to pszItem.
//
BOOL IsInDirList(PDIRLIST pdl, LPCTSTR pszItem)
{
    LPTSTR pszzList;

    //
    // Quick check for everything flag.
    //
    if (pdl->fEverythingInList)
        return TRUE;

    //
    // Quick check for empty list.
    //
    if (pdl->pszzList == NULL)
    {
        return FALSE;
    }

    //
    // Compare against each string in the szz list.
    //
    pszzList = pdl->pszzList;
    while (*pszzList)
    {
        //
        // If pszList is the beginning of the path to pszItem,
        // the item is in the list.
        //
        if (IsPathOfItem(pszzList, pszItem))
        {
            return TRUE;
        }

        pszzList += lstrlen(pszzList) + 1;
    }

    //
    // Couldn't find it.
    //
    return FALSE;
}

//
// IsInYesList - determine if an item is in the
// yes list of a YesNoList.
//
BOOL IsInYesList(PYNLIST pynl, LPCTSTR pszItem)
{
    //
    // Call helper function.
    //
    return IsInDirList(&pynl->dlYes, pszItem);
}

//
// IsInNoList - determine if an item is in the
// no list of a YesNoList.
//
BOOL IsInNoList(PYNLIST pynl, LPCTSTR pszItem)
{
    //
    // Call helper function.
    //
    return IsInDirList(&pynl->dlNo, pszItem);
}

//
// AddToDirList - adds an item to a dir list if necessary.
//
void AddToDirList(PDIRLIST pdl, LPCTSTR pszItem)
{
    UINT cchItem;

    //
    // Is the item already in the list?
    //
    if (IsInDirList(pdl, pszItem))
    {
        return;
    }

    //
    // Is the list empty?
    //
    if (pdl->pszzList == NULL)
    {
        pdl->pszzList = (LPTSTR)GlobalAlloc(GPTR, YNLIST_ALLOC);

        if (pdl->pszzList == NULL)
        {
            return;
        }

        pdl->cbAlloc = YNLIST_ALLOC;
        pdl->cchUsed = 1;
        ASSERT(pdl->pszzList[0] == TEXT('\0'));
    }

    //
    // Get the string length,
    // verify it can be added with
    // at most one additional alloc.
    //
    cchItem = lstrlen(pszItem) + 1;
    if (CbFromCch(cchItem) >= YNLIST_ALLOC)
    {
        return;
    }

    //
    // Do we need to allocate more space?
    //
    if (CbFromCch(cchItem) > pdl->cbAlloc - CbFromCch(pdl->cchUsed))
    {
        LPTSTR pszzNew;

        pszzNew = (LPTSTR)GlobalReAlloc(pdl->pszzList, pdl->cbAlloc + YNLIST_ALLOC, GMEM_MOVEABLE|GMEM_ZEROINIT);

        if (pszzNew == NULL)
        {
            return;
        }
        pdl->pszzList = pszzNew;

        pdl->cbAlloc += YNLIST_ALLOC;
    }

    //
    // Add the item.
    //
    lstrcpy(&(pdl->pszzList[pdl->cchUsed - 1]), pszItem);
    pdl->cchUsed += cchItem;

    //
    // Add the second NULL terminator
    // (GlobalReAlloc can't guarantee zeromeminit)
    //
    pdl->pszzList[pdl->cchUsed - 1] = TEXT('\0');
}

//
// Adds an item to the Yes list.
//
void AddToYesList(PYNLIST pynl, LPCTSTR pszItem)
{
    //
    // Call helper function.
    //
    AddToDirList(&pynl->dlYes, pszItem);
}

//
// Adds an item to the No list.
//
void AddToNoList(PYNLIST pynl, LPCTSTR pszItem)
{
    //
    // Call helper function.
    //
    AddToDirList(&pynl->dlNo, pszItem);
}

//
// SetYesToAll - puts everything in the yes list.
//
void SetYesToAll(PYNLIST pynl)
{
    pynl->dlYes.fEverythingInList = TRUE;
}

