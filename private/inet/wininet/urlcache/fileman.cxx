#include <cache.hxx>


/*-----------------------------------------------------------------------------
CFileManager::CFileManager
----------------------------------------------------------------------------*/
CFileManager::CFileManager(URL_CONTAINER* _urlContainer, MEMMAP_FILE* _mmFile) 
: nMaxDirs(DEFAULT_MAX_DIRS)
{
    // Set pointer to the associated container 
    // object and memory mapped file object
    urlContainer = _urlContainer;

    // Only create subdirectories for the CONTENT cache.   
    fUtilizeSubDirs = !strnicmp((LPSTR) urlContainer->GetCachePrefix(), 
        CONTENT_PREFIX, strlen(urlContainer->GetCachePrefix()));
    mmFile = _mmFile;
 
    // If the number of directories contained in the memory
    // mapped file is 0 it means it has been reinitialized.
    // Create the default number of directories.
    if (fUtilizeSubDirs && mmFile->GetDirCount() == 0)
        Init();
}

/*-----------------------------------------------------------------------------
CFileManager::~CFileManager
----------------------------------------------------------------------------*/
CFileManager::~CFileManager()
{}


/*-----------------------------------------------------------------------------
VOID CFileManager::Init
----------------------------------------------------------------------------*/
VOID CFileManager::Init()
{
    // Create the directories.
    CreateAdditionalSubDirectories(DEFAULT_DIR_TABLE_GROW_SIZE);
}


/*-----------------------------------------------------------------------------
VOID CreateAdditionalSubDirectories
----------------------------------------------------------------------------*/
VOID CFileManager::CreateAdditionalSubDirectories(DWORD nAdditionalDirs)
{
    DWORD nTotalDirs;
    DWORD nDirCount = mmFile->GetDirCount();

    if ((nTotalDirs = nAdditionalDirs + nDirCount) > nMaxDirs)
        return;
            
    mmFile->SetDirCount(nDirCount + nAdditionalDirs);
    
    for (DWORD i = nDirCount; i < nTotalDirs; i++)
    {
        // Actual directory names (cache1, cache2, etc) are
        // offset by the array indexes (0, 1, 2) by 1.
        // ie, dwDirArray[0] corresponds to cache1, etc.
        CreateSubDirectory(i+1);
        SetFileCount(i, 0);
    }
}

/*-----------------------------------------------------------------------------
VOID CreateSubDirectory(DWORD nIdx)
----------------------------------------------------------------------------*/
VOID CFileManager::CreateSubDirectory(DWORD nIdx)
{
    CHAR szCacheDir[MAX_PATH];
    CHAR szSubDir[MAX_PATH];

    strcpy(szCacheDir, mmFile->GetFullPathName());
    strcat(szCacheDir, "\\cache");
    wsprintf (szSubDir, "%d", nIdx);
    strcat(szCacheDir, szSubDir);
    CreateDirectory(szCacheDir, NULL);
    // TODO: set hidden attribute?
}



/*-----------------------------------------------------------------------------
VOID CFileManager::SetFileCount(DWORD nDir, DWORD nFiles)
----------------------------------------------------------------------------*/
VOID CFileManager::SetFileCount(DWORD nDir, DWORD nFiles)
{
    // BUGBUG - add asserts.
    LPDWORD dwDirArray = mmFile->GetDirArray();

    dwDirArray[nDir] = nFiles;
}

/*-----------------------------------------------------------------------------
DWORD CFileManager::GetFileCount(DWORD nDir)
----------------------------------------------------------------------------*/
DWORD CFileManager::GetFileCount(DWORD nDir)
{
    LPDWORD dwDirArray = mmFile->GetDirArray();
    DWORD dwFileCount = dwDirArray[nDir];
    return dwFileCount;
}

/*-----------------------------------------------------------------------------
DWORD CFileManager::FindMinFilesSubDir
----------------------------------------------------------------------------*/
DWORD CFileManager::FindMinFilesSubDir(DWORD& nMinFileDir)
{
    DWORD   nDirCount = mmFile->GetDirCount();
    LPDWORD dwDirArray = mmFile->GetDirArray();
    
    nMinFileDir = 0;

    DWORD nMinFiles = dwDirArray[0];
    for (DWORD i = 0; i < nDirCount - 1; i++)
    {
        if (dwDirArray[i+1] < nMinFiles)
        {
            nMinFiles = dwDirArray[i+1];
            nMinFileDir = i + 1;
        }
    }
    return dwDirArray[nMinFileDir];
}

/*-----------------------------------------------------------------------------
VOID CFileManager::NotifyCommit
----------------------------------------------------------------------------*/
VOID CFileManager::NotifyCommit(LPSTR szFileName)
{
    if (!fUtilizeSubDirs)
        return;

    LPDWORD dwDirArray = mmFile->GetDirArray();

    // szFileName should look like cache0\foo.bar
    CHAR* ptr = szFileName + sizeof("cache") - 1;
    DWORD nDir = atol(ptr) - 1;
    dwDirArray[nDir]++;
}

/*-----------------------------------------------------------------------------
VOID CFileManager::NotifyDelete
----------------------------------------------------------------------------*/
VOID CFileManager::NotifyDelete(LPSTR szFileName)
{
    if (!fUtilizeSubDirs)
        return;

    LPDWORD dwDirArray = mmFile->GetDirArray();

    // szFileName should look like 0\foo.bar
    CHAR* ptr = szFileName + sizeof("cache") - 1;
    DWORD nDir = atol(ptr) - 1;
    
    if (dwDirArray[nDir] > 0)
        dwDirArray[nDir]--;

}

/*-----------------------------------------------------------------------------
DWORD CFileManager::CreateUniqueFile(LPSTR szUrl, LPSTR szCachePath, 
                                    LPSTR szFileName, LPSTR szFileExtension)
----------------------------------------------------------------------------*/
DWORD CFileManager::CreateUniqueFile(LPSTR szUrl, LPSTR szCachePath, 
                                   LPSTR szFileName, LPSTR szFileExtension)
{
    if (!fUtilizeSubDirs)
        return ::CreateUniqueFile(szUrl, szCachePath, szFileName, szFileExtension);
    else
    {
        // Find the subdirectory with the minimum # of files
        DWORD nDir;
        DWORD nDirCount = mmFile->GetDirCount();
        if (FindMinFilesSubDir(nDir) >= MAX_FILES_PER_CACHE_DIRECTORY && nDirCount < nMaxDirs)
        {
            CreateAdditionalSubDirectories(DEFAULT_DIR_TABLE_GROW_SIZE);
            FindMinFilesSubDir(nDir);
        }

        CHAR szSubDirPath[MAX_PATH];
        CHAR szDirName[MAX_PATH];

        nDir++;

        wsprintf (szDirName, "%d", nDir);
        strcpy(szSubDirPath, szCachePath);
        strcat(szSubDirPath, "cache");
        strcat(szSubDirPath, szDirName);
        strcat(szSubDirPath, "\\");

        return ::CreateUniqueFile(szUrl, szSubDirPath, szFileName, szFileExtension);
    }
}

/*-----------------------------------------------------------------------------
DWORD CFileManager::DeleteCacheFiles()
----------------------------------------------------------------------------*/
VOID CFileManager::DeleteCacheFiles()
{
    if (!fUtilizeSubDirs)
        DeleteCachedFilesInDir(mmFile->GetFullPathName(), TRUE, TRUE);
    else
    {
        CHAR szDir[MAX_PATH];
        CHAR szSubDir[MAX_PATH];

        DWORD nDirCount = mmFile->GetDirCount();
        LPDWORD dwDirArray = mmFile->GetDirArray();
    
        for (DWORD i = 0; i < nDirCount; i++)
        {
            strcpy(szDir, mmFile->GetFullPathName());
            itoa(i + 1, szSubDir, 10);
            strcat(szDir, "cache");
            strcat(szDir, szSubDir);
            DeleteCachedFilesInDir(szDir, TRUE, TRUE);
            RemoveDirectory(szDir);
            dwDirArray[i] = 0;
        }
        mmFile->SetDirCount(0);
    }
}


/*-----------------------------------------------------------------------------
DWORD CFileManager::ReInitialize
----------------------------------------------------------------------------*/
VOID CFileManager::ReInitialize()
{
    Init();
}
