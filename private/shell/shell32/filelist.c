//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
#include "shellprv.h"
#pragma  hdrstop

//----------------------------------------------------------------------------
typedef LPITEMIDLIST PIDL;

//----------------------------------------------------------------------------
typedef struct
{
    // IShellLink *psl;    
    // IPersistFile *ppf;
    HDPA hdpaFLI;    
} ENUM_INFO;
typedef ENUM_INFO *PENUM_INFO;

//----------------------------------------------------------------------------
__inline UINT Sz_Cb(LPCTSTR psz)
{
    // NB We allocate an extra char in case we need to add an '&'.
    return (lstrlen(psz)+2)*sizeof(TCHAR);
}

//----------------------------------------------------------------------------
__inline static BOOL LAlloc(UINT cb, PVOID *ppv)
{
    *ppv = (PVOID*)LocalAlloc(LPTR, cb);
    return *ppv ? TRUE : FALSE;
}

//----------------------------------------------------------------------------
__inline static BOOL LFree(PVOID pv)
{
    return LocalFree(pv) ? FALSE : TRUE;
}

//----------------------------------------------------------------------------
BOOL Sz_AllocCopy(LPCTSTR pszSrc, LPSTR *ppszDst)
{
    BOOL fRet = FALSE;
    ASSERT(pszSrc && ppszDst);
    
    if (LAlloc(Sz_Cb(pszSrc), ppszDst))
    {
        lstrcpy(*ppszDst, pszSrc);
        fRet = TRUE;
    }

    return fRet;
}

//----------------------------------------------------------------------------
// Create a folder enum item.
BOOL FileList_CreateItem(IShellFolder *psf, PIDL pidl, PFILELIST_ITEM *ppfli)
{
    ASSERT(psf && pidl && ppfli);
    
    if (LAlloc(sizeof(FILELIST_ITEM), ppfli))
    {
        STRRET str;
        TCHAR szName[MAX_PATH];
        
        if (SUCCEEDED(psf->lpVtbl->GetDisplayNameOf(psf, pidl, SHGDN_NORMAL, &str)) &&
            SUCCEEDED(StrRetToBuf(&str, pidl, szName, ARRAYSIZE(szName))))
        {
            PSTR pszName;
            if (Sz_AllocCopy(szName, &pszName))
            {
                DWORD dwAttribs = SFGAO_FOLDER | SFGAO_FILESYSTEM;
                if (SUCCEEDED(psf->lpVtbl->GetAttributesOf(psf, 1, &pidl, &dwAttribs)))
                {
                    (*ppfli)->pszName = pszName;
                    (*ppfli)->pidl = pidl;
                    (*ppfli)->dwAttribs = dwAttribs;
                    return TRUE;
                }
                ASSERT(FALSE);
                LFree(pszName);
            }
        }
        ASSERT(FALSE);
        LFree(*ppfli);
        *ppfli = NULL;
    }

    return FALSE;
}

BOOL Folder_Enum(PIDL pidlFolder, PFN_FOLDER_ENUM_CALLBACK pfn, PVOID pv)
{
    BOOL fRet = FALSE;

    IShellFolder *psfDesktop;
    if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
    {
        IShellFolder *psf;
        if (SUCCEEDED(psfDesktop->lpVtbl->BindToObject(psfDesktop, pidlFolder, NULL, &IID_IShellFolder, &psf)))
        {
            LPENUMIDLIST penum;
            if (SUCCEEDED(psf->lpVtbl->EnumObjects(psf, NULL, SHCONTF_FOLDERS|SHCONTF_NONFOLDERS, &penum)))
            {
                PIDL pidlItem;
                UINT celt;
                while (penum->lpVtbl->Next(penum, 1, &pidlItem, &celt)==NOERROR && celt==1)
                {
                    // NB Callback function is responsible for nuking pidlItem if it
                    // doesn't need it.
                    if (!(*pfn)(psf, pidlFolder, pidlItem, pv))
                        break;
                }
                fRet = TRUE;
                penum->lpVtbl->Release(penum);
            }
            psf->lpVtbl->Release(psf);
        }
        psfDesktop->lpVtbl->Release(psfDesktop);
    }

    return fRet;
}

//----------------------------------------------------------------------------
BOOL FolderEnum_Callback(LPSHELLFOLDER psf, PIDL pidlFolder, PIDL pidlItem, LPVOID pv)
{
    PENUM_INFO pei = (PENUM_INFO)pv;
    PFILELIST_ITEM pfli;
    Assert (psf && pidlFolder && pidlItem && pei && pei->hdpaFLI);

    if (FileList_CreateItem(psf, pidlItem, &pfli))
        DPA_AppendPtr(pei->hdpaFLI, pfli);
    
    return TRUE;
}

//----------------------------------------------------------------------------
BOOL FileList_Create(PIDL pidlFolder, HDPA *phdpa, PINT pcItems)
{
    BOOL fRet = FALSE;
    ENUM_INFO ei;
    
    ASSERT(pidlFolder && phdpa);

    if (pcItems)
        *pcItems = 0;
    *phdpa = DPA_Create(0);
    if (*phdpa)
    {
        ei.hdpaFLI = *phdpa;
        if (ei.hdpaFLI)
        {
            if (Folder_Enum(pidlFolder, FolderEnum_Callback, &ei))
            {
                if (pcItems)
                    *pcItems = DPA_GetPtrCount(*phdpa);
                fRet = TRUE;
            }
        }
    }
    
    return fRet;
}

//---------------------------------------------------------------------------
// Simplified version of the file info comparison function.
int CALLBACK FileList_ItemCompare(PFILELIST_ITEM pfli1, PFILELIST_ITEM pfli2, LPARAM lParam)
{
    // Directories come first, then files and links.
    if ((pfli1->dwAttribs & SFGAO_FOLDER) > (pfli2->dwAttribs & SFGAO_FOLDER))
        return -1;
    else if ((pfli1->dwAttribs & SFGAO_FOLDER) < (pfli2->dwAttribs & SFGAO_FOLDER))
        return 1;

    return lstrcmpi(pfli1->pszName, pfli2->pszName);
}

//---------------------------------------------------------------------------
// Sort the list alphabetically.
BOOL FileList_Sort(HDPA hdpaFLI)
{
    return DPA_Sort(hdpaFLI, FileList_ItemCompare, 0);
}

//---------------------------------------------------------------------------
void FileList_DestroyItem(PFILELIST_ITEM pfli)
{
    // NB The pidl's are used by the menu itself, it will clean them up as 
    // needed so we don't have to do it here.
    if (pfli->pszName)
        LFree(pfli->pszName);
}

//---------------------------------------------------------------------------
void FileList_Destroy(HDPA hdpa)
{
    int cItems = DPA_GetPtrCount(hdpa);
    int i;
    for (i=0; i<cItems; i++)
    {
        PFILELIST_ITEM pfli = DPA_GetPtr(hdpa, i);
        FileList_DestroyItem(pfli);
    }
    DPA_Destroy(hdpa);
}


