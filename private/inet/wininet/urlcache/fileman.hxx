#ifndef _FILEMAN_HXX
#define _FILEMAN_HXX

class URL_CONTAINER;

class CFileManager
{

private:
    const DWORD nMaxDirs;

    MEMMAP_FILE *mmFile;
    URL_CONTAINER *urlContainer;
    BOOL fUtilizeSubDirs;

    VOID Init();
    VOID AllocateDirTableEntry(DWORD);
    VOID ReAllocateDirTableEntry(DWORD);
    
    VOID CreateAdditionalSubDirectories(DWORD);
    VOID CreateSubDirectory(DWORD);
    VOID SetFileCount(DWORD, DWORD);
    DWORD GetFileCount(DWORD);
    DWORD FindMinFilesSubDir(DWORD&);
public:
    CFileManager(URL_CONTAINER*, MEMMAP_FILE*);
    ~CFileManager();

    VOID  ReInitialize();
    DWORD CreateUniqueFile(LPSTR, LPSTR, LPSTR, LPSTR);
    VOID  NotifyCommit(LPSTR);
    VOID  NotifyDelete(LPSTR);
    VOID  DeleteCacheFiles();
};


#endif // _FILEMAN_HXX