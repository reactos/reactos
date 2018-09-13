/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:  filemgr.hxx

Abstract:

    Manages cache file & directory creation/deletion.
    
Author:
    Adriaan Canter (adriaanc) 04-02-97

Modifications:
    Ahsan Kabir (akabir) 25-Sept-97 made minor alterations.
    
--*/

#ifndef _FILEMGR_HXX
#define _FILEMGR_HXX

class URL_CONTAINER;

/*-----------------------------------------------------------------------------
    Base file manager class
  ---------------------------------------------------------------------------*/
class CFileMgr
{
public:
    CFileMgr(MEMMAP_FILE* mmFile, DWORD dwOptions);
    ~CFileMgr();

    virtual BOOL  Init();
    virtual DWORD GetDirLen(DWORD nKey);

    virtual DWORD CreateUniqueFile(LPCSTR UrlName, LPTSTR FileName, LPTSTR Extension,
                                    HANDLE *phfHandle);
    
    virtual BOOL  NotifyCommit(DWORD);
    virtual BOOL  Cleanup();    
    static  BOOL  DeleteCache(LPSTR);
    
    virtual BOOL  GetDirIndex(LPSTR, LPDWORD);
    virtual BOOL  GetFilePathFromEntry(URL_FILEMAP_ENTRY*, LPSTR, LPDWORD);

    virtual BOOL  DeleteOneCachedFile (LPSTR lpszFileName, DWORD dostEntry, DWORD nIndex);

    virtual BOOL  CreateDirWithSecureName( LPSTR);

protected:
    DWORD _cbBasePathLen;
    MEMMAP_FILE* _mmFile;
    DWORD   _dwOptions;

    DWORD CreateUniqueFile(LPCSTR UrlName, LPTSTR Path, 
                           LPTSTR FileName, LPTSTR Extension, HANDLE* phfHandle);

    DWORD MakeRandomFileName(LPCSTR UrlName, 
                            LPTSTR FileName, LPTSTR Extension);

    BOOL MapStoreKey(LPSTR szPath, LPDWORD pcbPath, 
                     LPDWORD dwKey, DWORD dwFlag);
    
    BOOL GetStoreDirectory(LPSTR szPath, LPDWORD pcbPath);

    DWORD GetOptions() { return _dwOptions; }

};


/*-----------------------------------------------------------------------------
    Secure file manager class
  ---------------------------------------------------------------------------*/
class CSecFileMgr : public CFileMgr
{
public:
    CSecFileMgr(MEMMAP_FILE* mmFile, DWORD dwOptions);
    ~CSecFileMgr();

    BOOL  Init();
    DWORD GetDirLen(DWORD nKey);

    DWORD CreateUniqueFile(LPCSTR UrlName, LPTSTR FileName, LPTSTR Extension, 
                            HANDLE *phfHandle);
    
    BOOL  NotifyCommit(DWORD);
    BOOL  Cleanup();

    BOOL  GetDirIndex(LPSTR, LPDWORD);
    BOOL  GetFilePathFromEntry(URL_FILEMAP_ENTRY*, LPSTR, LPDWORD);

    BOOL  DeleteOneCachedFile (LPSTR lpszFileName, DWORD dostEntry, DWORD nIndex);

protected:
    BOOL CreateRandomDirName(LPSTR);    
    BOOL CreateAdditionalSubDirectories(DWORD);
    BOOL CreateSubDirectory(DWORD);
    BOOL FindMinFilesSubDir(DWORD&, DWORD&);
};

#endif // _FILEMGR_HXX
