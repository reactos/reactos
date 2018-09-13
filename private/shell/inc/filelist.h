//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

typedef struct
{
    DWORD dwAttribs;
    LPITEMIDLIST pidl;
    PSTR pszName;
} FILELIST_ITEM;
typedef FILELIST_ITEM *PFILELIST_ITEM;

typedef BOOL (*PFN_FOLDER_ENUM_CALLBACK)(LPSHELLFOLDER psf, LPITEMIDLIST pidlFolder, LPITEMIDLIST pidlItem, LPVOID pv);
BOOL Folder_Enum(LPITEMIDLIST pidlFolder, PFN_FOLDER_ENUM_CALLBACK pfn, PVOID pv);

void FileList_Destroy(HDPA hdpa);
BOOL FileList_Create(LPITEMIDLIST pidlFolder, HDPA *phdpa, PINT pcItems);
BOOL FileList_Sort(HDPA hdpaFLI);

BOOL FileList_CreateItem(IShellFolder *psf, LPITEMIDLIST pidl, PFILELIST_ITEM *ppfli);
void FileList_DestroyItem(PFILELIST_ITEM pfli);
BOOL Sz_AllocCopy(LPCTSTR pszSrc, LPSTR *ppszDst);

