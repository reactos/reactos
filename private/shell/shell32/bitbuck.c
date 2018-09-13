#include "shellprv.h"
#pragma hdrstop

#include "bitbuck.h"
#include "fstreex.h"
#include "copy.h"
#include "mrsw.h"

#include "treewkcb.h"   // for FolderSize

#include "datautil.h"
#include "cscuiext.h"

// mtpt.cpp
STDAPI_(BOOL) CMtPt_IsSecure(int iDrive);

// copy.c
void FOUndo_AddInfo(LPUNDOATOM lpua, LPTSTR lpszSrc, LPTSTR lpszDest, DWORD dwAttributes);
void FOUndo_FileReallyDeleted(LPTSTR lpszFile);
void CALLBACK FOUndo_Release(LPUNDOATOM lpua);
void FOUndo_FileRestored(LPTSTR lpszFile);

// drivesx.c
DWORD PathGetClusterSize(LPCTSTR pszPath);

// bitbcksf.c
int DataObjToFileOpString(IDataObject * pdtobj, LPTSTR * ppszSrc, LPTSTR * ppszDest);


//
// #defines
//
#ifdef DEBUG
int g_idBBCurrentVersion = BITBUCKET_CURRENT_VERSION;
#undef BITBUCKET_CURRENT_VERSION
#define BITBUCKET_CURRENT_VERSION g_idBBCurrentVersion
#endif


//
// per-process global bitbucket data
//
BOOL g_fBBInited = FALSE;                           // have we initialized our global data yet?
BOOL g_bIsProcessExplorer = FALSE;                  // are we the main explorer process? (if so, we persist the state info in the registry)
PBBSYNCOBJECT g_pBitBucket[MAX_BITBUCKETS] = {0};   // our array of bbso's that protect each bucket
HANDLE g_hgcGlobalDirtyCount = INVALID_HANDLE_VALUE;// a global counter to tell us if the global settings have changed and we need to re-read them
LONG g_lProcessDirtyCount = 0;                      // out current dirty count; we compare this to hgcDirtyCount to see if we need to update the settings from the registry
HANDLE g_hgcNumDeleters= INVALID_HANDLE_VALUE;      // a global counter that indicates the total # of people who are currently doing recycle bin file operations
HKEY g_hkBitBucket = NULL;                          // reg key that points to HKLM\Software\Microsoft\Windows\CurrentVersion\Explorer\BitBucket
HKEY g_hkBitBucketPerUser = NULL;                   // reg key that points to HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\BitBucket


//
// prototypes
//
void PersistBBDriveInfo(int idDrive);
LONG CheckFolderSizeAndDeleteability(LPCTSTR pszDir, FOLDERDELETEINFO* pfdi);
BOOL IsFileDeletable(LPCTSTR pszFile);
BOOL CreateRecyclerDirectory(int idDrive);
void PurgeOneBitBucket(HWND hwnd, int idDrive, DWORD dwFlags);
int CountDeletedFilesOnDrive(int idDrive, LPDWORD pdwSize, int iMaxFiles);
BOOL GetBBDriveSettings(int idDrive, ULONGLONG *pcbDiskSpace);
void DeleteOldBBRegInfo(int idDrive);
BOOL IsBitBucketInited(int idDrive);
void FreeBBInfo(PBBSYNCOBJECT pbbso);
BOOL VerifyBBInfoFileHeader(int idDrive);
#ifdef WINNT
SECURITY_DESCRIPTOR* CreateRecycleBinSecurityDescriptor();
#endif


int DriveIDFromBBPath(LPNCTSTR pszPath)
{
    LPCTSTR pszTempPath;
    TCHAR szNetHomeDir[MAX_PATH];
    int idDrive;
#ifdef ALIGNMENT_SCENARIO
    TCHAR szTmp[MAX_PATH];

    //
    // BUGBUG (reinerf): mounted volumes need to be dealt with properly here.
    //
    ualstrcpyn(szTmp, pszPath, ARRAYSIZE(szTmp));
    pszTempPath = szTmp;
#else
    pszTempPath = pszPath;
#endif

    idDrive = PathGetDriveNumber(pszTempPath);

    if ((idDrive == -1) && GetNetHomeDir(szNetHomeDir))
    {
        int iLen = lstrlen(szNetHomeDir);

        // NOTE: we don't want to let you recycle the nethomedir itself, so
        // insure that pszPath is larger than the nethomedir path
        // (neither is trailed with a backslash)
        if ((iLen < lstrlen(pszTempPath)) &&
            (PathCommonPrefix(szNetHomeDir, pszTempPath, NULL) == iLen))
        {
            // this is a subdir of the nethomedir, so we recycle it to the net home server
            // which is drive 26
            return SERVERDRIVE;
        }
    }

    return idDrive;
}


void DriveIDToBBRoot(int idDrive, LPTSTR szPath)
{
    ASSERT(idDrive >= 0);
    
    if (SERVERDRIVE == idDrive) 
    {
        // nethomedir case
        if (!GetNetHomeDir(szPath))
        {
            ASSERT(szPath[0] == 0);
            TraceMsg(TF_BITBUCKET, "BitBucket: Machine does NOT have a NETHOMEDIR");
        }
        else
        {
            // use the nethomedir
            ASSERT(szPath[0] != TEXT('\0'));
        }
    }
    else
    {
        // build up the "C:\" string
        PathBuildRoot(szPath, idDrive);
    }
}

void DriveIDToBBVolumeRoot(int idDrive, LPTSTR szPath)
{
    DriveIDToBBRoot(idDrive, szPath);
    PathStripToRoot(szPath);
    PathAddBackslash(szPath);
}

void DriveIDToBBPath(int idDrive, LPTSTR pszPath)
{
    DriveIDToBBRoot(idDrive, pszPath);

#ifdef WINNT
    // NOTE: always append the SID for the SERVERDRIVE case
    if ((SERVERDRIVE == idDrive) || (CMtPt_IsSecure(idDrive)))
    {
        LPTSTR pszInmate = GetUserSid(NULL);
        if (pszInmate)
        {
            PathAppend(pszPath, TEXT("RECYCLER"));
            PathAppend(pszPath, pszInmate);
            LocalFree((HLOCAL)pszInmate);
            return;
        }
        else
        {
            ASSERTMSG(FALSE, "Bitbuck: unable to get the users SID !!");
        }
    }
#endif
    PathAppend(pszPath, c_szRecycled);
}

//
// converts "c:\recycled\whatver"  to "c"
//          \\nethomedir\share  to "@"
//
// BUGBUG: when the net recycle bin is up, it should convert
//         \\server\share to something as well
//
void DriveIDToBBRegKey(int idDrive, LPTSTR lpszValue)
{
    //
    // BUGBUG (reinerf): mounted volumes need to be dealt with properly here.
    //
    lpszValue[0] = TEXT('A') + (TCHAR)idDrive;

    if (lpszValue[0] == (TEXT('A') + SERVERDRIVE))
    {
        // nethomedir case
        lpszValue[0] = TEXT('@');
    }

    lpszValue[1] = 0;
}


/*
 Network home drive code (from win95 days) is being used to support the recycle bin
 for users with mydocs redirected to a UNC path

 "Drive 26" specifies the network homedir

 This can return NULL = (no net home dir, unknown setup, etc.)
                 or a string ( global ) pointing to the homedir (lfn)
*/
BOOL GetNetHomeDir(LPTSTR pszNetHomeDir)
{
    if (SHGetSpecialFolderPath(NULL, pszNetHomeDir, CSIDL_PERSONAL, FALSE))
    {
        TCHAR szOldBBDir[MAX_PATH];

        if (PathIsUNC(pszNetHomeDir))
        {
            // Remove the trailing backslash (if present)
            // because this string will be passed to PathCommonPrefix()
            PathRemoveBackslash(pszNetHomeDir);
        }
        else
        {
            pszNetHomeDir[0] = TEXT('\0');
        }

        // check to see if the mydocs path has changed
        if (g_pBitBucket[SERVERDRIVE]           &&
            (g_pBitBucket[SERVERDRIVE] != (PBBSYNCOBJECT)-1)   &&
            g_pBitBucket[SERVERDRIVE]->pidl     &&
            SHGetPathFromIDList(g_pBitBucket[SERVERDRIVE]->pidl, szOldBBDir))
        {
            // we should always find "\RECYCLER\" because this is an old recycle bin directory.
            LPTSTR pszTemp = StrRStrI(szOldBBDir, NULL, TEXT("\\RECYCLER\\"));
            
            // cut the string off before the "\RECYCLER\<SID>" part so we can compare it to the current mydocs path
            *pszTemp = TEXT('\0');

            if (lstrcmpi(szOldBBDir, pszNetHomeDir) != 0)
            {   
                if (*pszNetHomeDir)
                {
                    TCHAR szNewBBDir[MAX_PATH];
                    LPITEMIDLIST pidl;
                    WIN32_FIND_DATA fd = {0};

                    // mydocs was redirected to a different UNC path, so update the bbsyncobject for the SERVERDRIVE

                    // copy the new mydocs location and add the "\RECYCLER\<SID>" part back on
                    lstrcpyn(szNewBBDir, pszNetHomeDir, ARRAYSIZE(szNewBBDir));
                    PathAppend(szNewBBDir, pszTemp + 1);

                    // create a simple pidl since "RECYCLER\<SID>" subdirectory might not exist yet
                    fd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
                    lstrcpyn(fd.cFileName, szNewBBDir, ARRAYSIZE(fd.cFileName));
                    
                    if (SUCCEEDED(SHSimpleIDListFromFindData(szNewBBDir, &fd, &pidl)))
                    {
                        LPITEMIDLIST pidlOld;
                        ULARGE_INTEGER ulFreeUser, ulTotal, ulFree;
                        DWORD dwClusterSize;
                        BOOL bUpdateSize = FALSE;

                        if (SHGetDiskFreeSpaceEx(pszNetHomeDir, &ulFreeUser, &ulTotal, &ulFree))
                        {
                            dwClusterSize = PathGetClusterSize(pszNetHomeDir);
                            bUpdateSize = TRUE;
                        }

                        ENTERCRITICAL;
                        // swap in the new pidl
                        pidlOld = g_pBitBucket[SERVERDRIVE]->pidl;
                        g_pBitBucket[SERVERDRIVE]->pidl = pidl;
                        ILFree(pidlOld);

                        // set the cchBBDir
                        g_pBitBucket[SERVERDRIVE]->cchBBDir = lstrlen(szNewBBDir);

                        g_pBitBucket[SERVERDRIVE]->fInited = TRUE;

                        // update the size fields
                        if (bUpdateSize)
                        {
                            ULARGE_INTEGER ulMaxSize;

                            g_pBitBucket[SERVERDRIVE]->dwClusterSize = dwClusterSize;
                            g_pBitBucket[SERVERDRIVE]->qwDiskSize = ulTotal.QuadPart;

                            // we limit the max size of the recycle bin to ~4 gig
                            ulMaxSize.QuadPart = min(((ulTotal.QuadPart / 100) * g_pBitBucket[SERVERDRIVE]->iPercent), (DWORD)-1);
                            ASSERT(ulMaxSize.HighPart == 0);
                            g_pBitBucket[SERVERDRIVE]->cbMaxSize = ulMaxSize.LowPart;
                        }
                        LEAVECRITICAL;
                    }
                }
                else
                {
                    // mydocs was redireced back to a local path, so flag this drive as not inited so we wont do any more
                    // recycle bin operations on it.
                    ENTERCRITICAL;
                    g_pBitBucket[SERVERDRIVE]->fInited = FALSE;
                    LEAVECRITICAL;
                }
            }
            else
            {
                // the mydocs previously to pointed to \\foo\bar, and the user has set it back to that path again.
                // so flag the drive as inited so we can start using it again.
                if (g_pBitBucket[SERVERDRIVE]->fInited == FALSE)
                {
                    ENTERCRITICAL;
                    g_pBitBucket[SERVERDRIVE]->fInited = TRUE;
                    LEAVECRITICAL;
                }
            }
        }
    }
    else
    {
        pszNetHomeDir[0] = TEXT('\0');
    }

    return (BOOL)pszNetHomeDir[0];
}


STDAPI_(BOOL) IsBitBucketableDrive(int idDrive)
{
    BOOL bRet;
    TCHAR szBBRoot[MAX_PATH];
    TCHAR szFileSystem[MAX_PATH];
    
    if ((idDrive < 0) || (g_pBitBucket[idDrive] == (PBBSYNCOBJECT)-1))
    {
        // we dont support recycle bin for the general UNC case or we have 
        // flagged this drive as not having a recycle bin for one reason or another.
        return FALSE;
    }

    if (IsBitBucketInited(idDrive))
    {
        // the struct is allready allocated and inited, so this is a bitbucketable drive
        return TRUE;
    }

    bRet = (RealDriveType(idDrive, FALSE) == DRIVE_FIXED ||
            ((idDrive == SERVERDRIVE) && GetNetHomeDir(szBBRoot)));

    if (bRet && (idDrive != SERVERDRIVE))
    {
        // also check to make sure that the drive isint RAW (unformatted)
        DriveIDToBBRoot(idDrive, szBBRoot);
        
        if (!GetVolumeInformation(szBBRoot, NULL, 0, NULL, NULL, NULL, szFileSystem, ARRAYSIZE(szFileSystem)) ||
            lstrcmpi(szFileSystem, TEXT("RAW")) == 0)
        {
            bRet = FALSE;
        }
        else
        {
            // the drive better be NTFS, FAT or FAT32, else we need to know about it and handle it properly
            ASSERT((lstrcmpi(szFileSystem, TEXT("NTFS")) == 0)  || 
                   (lstrcmpi(szFileSystem, TEXT("FAT")) == 0)   ||
                   (lstrcmpi(szFileSystem, TEXT("FAT32")) == 0));
        }
    }

    return bRet;
}


// c:\recycled => c:\recycled\info2 (the new IE4/NT5/Win98 info file)
__inline void GetBBInfo2FileSpec(LPTSTR pszBBPath, LPTSTR pszInfo)
{
    PathCombine(pszInfo, pszBBPath, c_szInfo2);
}


// c:\recycled => c:\recycled\info (the old win95/NT4 info file)
__inline void GetBBInfoFileSpec(LPTSTR pszBBPath, LPTSTR pszInfo)
{
    PathCombine(pszInfo, pszBBPath, c_szInfo);
}


__inline BOOL IsBitBucketInited(int idDrive)
{
    BOOL bRet;

    // InitBBDriveInfo could fail and we free and set g_pBitBucket[idDrive] = -1. So there
    // is a small window between when we check g_pBitBucket[idDrive] and when we deref 
    // g_pBitBucket[idDrive]->fInited, to protect against g_pBitBucket[idDrive] being freed 
    // in this window we use the crit sec.
    ENTERCRITICAL;
    bRet = (g_pBitBucket[idDrive]                           &&
            (g_pBitBucket[idDrive] != (PBBSYNCOBJECT)-1)    && 
            g_pBitBucket[idDrive]->fInited);
    LEAVECRITICAL;

    return bRet;
}


BOOL RevOldBBInfoFileHeader(HANDLE hFile, LPBBDATAHEADER pbbdh)
{
    // Verify that this is a valid info file
    if (pbbdh->cbDataEntrySize == SIZEOF(BBDATAENTRY)) 
    {
        if (pbbdh->idVersion == BITBUCKET_WIN95_VERSION || pbbdh->idVersion == BITBUCKET_NT4_VERSION)
        {
            DWORD dwBytesWritten;

            // now seek back to 0 and write in the new stuff
            pbbdh->idVersion = BITBUCKET_CURRENT_VERSION;
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN); // go to the beginning
            WriteFile(hFile, (LPBYTE)pbbdh, SIZEOF(BBDATAHEADER), &dwBytesWritten, NULL);
            
            ASSERT(dwBytesWritten == SIZEOF(BBDATAHEADER));
        }

        return (pbbdh->idVersion == BITBUCKET_CURRENT_VERSION);
    }
    return FALSE;
}


//
// We need to update the cCurrent and cFiles in the info file header
// for compat with win98/IE4 machines.
//
BOOL UpdateBBInfoFileHeader(int idDrive)
{
    BBDATAHEADER bbdh = {0, 0, 0, SIZEOF(BBDATAENTRY), 0}; // defaults
    HANDLE hFile;
    BOOL bRet = FALSE; // assume failure;

#ifdef BB_USE_MRSW
    MRSW_EnterRead(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

    // pass 1 for the retry attempt since we are called during shutdown and if another process
    // is using the recycle bin we will hang and get the "End Task" dialog (bad!).
    hFile = OpenBBInfoFile(idDrive, OPENBBINFO_WRITE, 1);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        BBDATAENTRYW bbdew;
        DWORD dwBytesRead;

        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        bRet = ReadFile(hFile, &bbdh, SIZEOF(BBDATAHEADER), &dwBytesRead, NULL);
        
        if (bRet && dwBytesRead == SIZEOF(BBDATAHEADER))
        {
            DWORD dwSize;
            DWORD dwBytesWritten;

            ASSERT(bbdh.idVersion == BITBUCKET_CURRENT_VERSION);

            bbdh.cCurrent = SHGlobalCounterGetValue(g_pBitBucket[idDrive]->hgcNextFileNum);
            bbdh.cFiles = CountDeletedFilesOnDrive(idDrive, &dwSize, 0);
            bbdh.dwSize = dwSize;
            
            SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
            WriteFile(hFile, (LPBYTE)&bbdh, SIZEOF(BBDATAHEADER), &dwBytesWritten, NULL);
            
            ASSERT(dwBytesWritten == SIZEOF(BBDATAHEADER));
            bRet = TRUE;
        }

        ASSERT((g_pBitBucket[idDrive]->fIsUnicode && SIZEOF(BBDATAENTRYW) == bbdh.cbDataEntrySize) ||
               (!g_pBitBucket[idDrive]->fIsUnicode && SIZEOF(BBDATAENTRYA) == bbdh.cbDataEntrySize));

        // Since we dont flag entries that were deleted in the info file as deleted 
        // immeadeately, we need to go through and mark them as such now
        while (ReadNextDataEntry(hFile, &bbdew, TRUE, bbdh.cbDataEntrySize, idDrive)){}

        CloseBBInfoFile(hFile, idDrive);
    }

#ifdef BB_USE_MRSW
    MRSW_LeaveRead(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

    if (!bRet)
    {
        ASSERTMSG(FALSE, "Bitbucket: failed to update drive %d for win98/NT4 compat!!", idDrive);
    }

    return bRet;
}

VOID ResetInfoFileHeader(HANDLE hFile, BOOL fIsUnicode)
{
    DWORD dwBytesWritten;
    BBDATAHEADER bbdh = {BITBUCKET_CURRENT_VERSION,
			 0,
			 0,
			 fIsUnicode ? SIZEOF(BBDATAENTRYW) : SIZEOF(BBDATAENTRYA),
			 0};

    ASSERT(INVALID_HANDLE_VALUE != hFile);

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    WriteFile(hFile, (LPBYTE)&bbdh, SIZEOF(BBDATAHEADER), &dwBytesWritten, NULL);
    ASSERT(dwBytesWritten == SIZEOF(BBDATAHEADER));
    SetEndOfFile(hFile);
}

BOOL CreateInfoFile(idDrive)
{
    TCHAR szBBPath[MAX_PATH];
    TCHAR szInfoFile[MAX_PATH];
    HANDLE hFile;

    DriveIDToBBPath(idDrive, szBBPath);
    GetBBInfo2FileSpec(szBBPath, szInfoFile);

    hFile = OpenBBInfoFile(idDrive, OPENBBINFO_CREATE, 0);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        ResetInfoFileHeader(hFile, SIZEOF(BBDATAENTRY) == SIZEOF(BBDATAENTRYW));
        CloseHandle(hFile);

        // We explicitly call SHChangeNotify so that we can generate a change specifically
        // for the info file. The recycle bin shell folder will then ignore any updates to
        // the info file.
        SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szInfoFile, NULL);

        return TRUE;
    }

    ASSERTMSG(FALSE, "Bitbucket: faild to create file info file: %s !!", szInfoFile);
    return FALSE;
}

#ifdef WINNT
//---------------------------------------------------------------------------
//  GetNT4BBAcl() - Creates a ACL structure for allowing access for 
//                  only the current user,the administrators group, or the system.
//                  Returns a pointer to an access control list 
//                  structure in the local heap; it can be
//                  free'd with LocalFree.
//
// !! HACKHACK !! - This code was basically taken right out of NT4 so that we can
//                  compare against the old NT4 recycle bin ACL. The new helper function
//                  GetShellSecurityDescriptor puts the ACE's in a different order
//                  than this function, and so we memcmp the ACL against botht this
//                  one and the new win2k one.
//---------------------------------------------------------------------------
PACL GetNT4BBAcl()
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    PACL         pAcl = NULL;
    PTOKEN_USER  pUser = NULL;
    PSID         psidSystem = NULL;
    PSID         psidAdmin = NULL;
    DWORD        cbAcl;
    DWORD        aceIndex;
    ACE_HEADER * lpAceHeader;
    UINT         nCnt = 2;  // inheritable; so two ACE's for each user
    BOOL         bSuccess = FALSE;


    //
    // Get the USER token so we can grab its SID for the DACL.
    //
    pUser = GetUserToken(NULL);
    if (!pUser)
    {
        TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to get user.  Error = %d", GetLastError());
        goto Exit;
    }

    //
    // Get the system sid
    //
    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
         TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to initialize system sid.  Error = %d", GetLastError());
         goto Exit;
    }


    //
    // Get the Admin sid
    //

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, &psidAdmin)) {
         TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to initialize admin sid.  Error = %d", GetLastError());
         goto Exit;
    }


    //
    // Allocate space for the DACL
    //
    cbAcl = SIZEOF(ACL) +
            (nCnt * GetLengthSid(pUser->User.Sid)) +
            (nCnt * GetLengthSid(psidSystem)) +
            (nCnt * GetLengthSid(psidAdmin)) +
            (nCnt * 3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));

    pAcl = (PACL)LocalAlloc(LPTR, cbAcl);
    if (!pAcl) {
        TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to allocate acl.  Error = %d", GetLastError());
        goto Exit;
    }

    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to initialize acl.  Error = %d", GetLastError());
        goto Exit;
    }

    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //
    aceIndex = 0;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, pUser->User.Sid)) {
        TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError());
        goto Exit;
    }

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSystem)) {
        TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError());
        goto Exit;
    }

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidAdmin)) {
        TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError());
        goto Exit;
    }

    //
    // Now the inheritable ACEs
    //

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, pUser->User.Sid)) {
        TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError());
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to get ace (%d).  Error = %d", aceIndex, GetLastError());
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
        TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError());
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to get ace (%d).  Error = %d", aceIndex, GetLastError());
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    aceIndex++;
    if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
        TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to add ace (%d).  Error = %d", aceIndex, GetLastError());
        goto Exit;
    }

    if (!GetAce(pAcl, aceIndex, &lpAceHeader)) {
        TraceMsg(TF_BITBUCKET, "GetNT4BBAcl: Failed to get ace (%d).  Error = %d", aceIndex, GetLastError());
        goto Exit;
    }

    lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

    bSuccess = TRUE;
    
Exit:
    if (pUser)
        LocalFree(pUser);

    if (psidSystem)
        FreeSid(psidSystem);

    if (psidAdmin)
        FreeSid(psidAdmin);

    if (!bSuccess && pAcl)
    {
        LocalFree(pAcl);
        pAcl = NULL;
    }

    return pAcl;
}


//
// this checks to make sure that the users recycle bin directory is properly acl'ed
//
BOOL CheckRecycleBinAcls(idDrive)
{
    BOOL bIsSecure = TRUE;
    TCHAR szBBPath[MAX_PATH];
    PSECURITY_DESCRIPTOR psdCurrent;
    PSID psidOwner;
    PACL pdaclCurrent;

    if ((idDrive == SERVERDRIVE) || !CMtPt_IsSecure(idDrive))
    {
        // either redirected mydocs case (no security) or its not an NTFS drive, so no ACL's to check
        return TRUE;
    }

    DriveIDToBBPath(idDrive, szBBPath);

    if (GetNamedSecurityInfo(szBBPath,
                             SE_FILE_OBJECT,
                             DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION,
                             &psidOwner,
                             NULL,
                             &pdaclCurrent,
                             NULL,
                             &psdCurrent) == ERROR_SUCCESS)
    {
        PTOKEN_USER pUser = GetUserToken(NULL);
        
        if (pUser)
        {
            if (!EqualSid(psidOwner, pUser->User.Sid))
            {
                // the user is not the owner of the dir, check to see if the owner is the Administrators group or the System
                // (we consider the directory to be secure if the owner is either of these two)
                SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
                PSID psidAdministrators = NULL;
                PSID psidSystem = NULL;

                if (AllocateAndInitializeSid(&sia, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psidAdministrators) && 
                    AllocateAndInitializeSid(&sia, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &psidSystem))
                {
                    if (!EqualSid(psidOwner, psidAdministrators) && !EqualSid(psidOwner, psidSystem))
                    {
                        // directory is not owned by the user, or the Administrators group or the system, we thus consider it unsecure.
                        TraceMsg(TF_BITBUCKET, "CheckRecycleBinAcls: dir %s has possibly unsecure owner!", szBBPath);
                        bIsSecure = FALSE;
                    }

                    if (psidAdministrators)
                        FreeSid(psidAdministrators);

                    if (psidSystem)
                        FreeSid(psidSystem);
                }
                else
                {
                    TraceMsg(TF_BITBUCKET, "CheckRecycleBinAcls: AllocateAndInitializeSid failed, assuming %s is unsecure", szBBPath);
                    bIsSecure = FALSE;
                }
            }

            if (bIsSecure)
            {
                // directory owner checked out ok, lets see if the acl is what we expect...
                SECURITY_DESCRIPTOR* psdRecycle = CreateRecycleBinSecurityDescriptor();

                if (psdRecycle)
                {
                    // BUGBUG (reinerf) - lame that all we do is check the size and do a memcmp, but that's all the aclui code does
                    // so we are no worse
                    if ((psdRecycle->Dacl->AclSize != pdaclCurrent->AclSize) ||
                        (memcmp(psdRecycle->Dacl, pdaclCurrent, pdaclCurrent->AclSize) != 0))
                    {
                        // acl sizes were different or they didn't memcmp, so check against the old NT4 style acl
                        // (in NT4 we added the ACE's in a different order which causes the memcmp to fail, even 
                        // though the ACL is equivilant)
                        PACL pAclNT4 = GetNT4BBAcl();

                        if (pAclNT4)
                        {
                            // do the same lame size / memcmp check
                            if ((pAclNT4->AclSize != pdaclCurrent->AclSize) ||
                                (memcmp(pAclNT4, pdaclCurrent, pdaclCurrent->AclSize) != 0))
                            {
                                // acl sizes were different or they didn't memcmp, so assume the dir is unsecure
                                bIsSecure = FALSE;
                            }

                            LocalFree(pAclNT4);
                        }
                        else
                        {
                            TraceMsg(TF_BITBUCKET, "CheckRecycleBinAcls: GetNT4BBSecurityAttributes failed, assuming %s is unsecure", szBBPath);
                            bIsSecure = FALSE;
                        }
                    }

                    LocalFree(psdRecycle);
                }
                else
                {
                    TraceMsg(TF_BITBUCKET, "CheckRecycleBinAcls: CreateRecycleBinSecurityDescriptor failed, assuming %s is unsecure", szBBPath);
                    bIsSecure = FALSE;
                }

            }

            LocalFree(pUser);
        }
        else
        {
            // couldnt' get the users sid, so assume the dir is unsecure
            TraceMsg(TF_BITBUCKET, "CheckRecycleBinAcls: failed to get the users sid, assuming %s is unsecure", szBBPath);
            bIsSecure = FALSE;
        }

        LocalFree(psdCurrent);
    }
    else
    {
        // GetNamedSecurityInfo failed, assume the dir is unsecure
        TraceMsg(TF_BITBUCKET, "CheckRecycleBinAcls: GetNamedSecurityInfo failed, assuming %s is unsecure", szBBPath);
        bIsSecure = FALSE;
    }
    
    if (!bIsSecure)
    {
        TCHAR szDriveName[MAX_PATH];

        DriveIDToBBRoot(idDrive, szDriveName);

        if (ShellMessageBox(HINST_THISDLL, 
                            NULL,
                            MAKEINTRESOURCE(IDS_RECYCLEBININVALIDFORMAT),
                            MAKEINTRESOURCE(IDS_WASTEBASKET),
                            MB_YESNO | MB_ICONEXCLAMATION | MB_SETFOREGROUND,
			    szDriveName) == IDYES)
        {
            TCHAR szBBPathToNuke[MAX_PATH+1];
            SHFILEOPSTRUCT fo = {NULL,
                                 FO_DELETE,
                                 szBBPathToNuke,
                                 NULL,
                                 FOF_NOCONFIRMATION | FOF_SILENT,
                                 FALSE,
                                 NULL,
                                 NULL};

            lstrcpyn(szBBPathToNuke, szBBPath, MAX_PATH);
            szBBPathToNuke[lstrlen(szBBPathToNuke) + 1] = TEXT('\0'); // double null terminate

            // try to nuke the old recycle bin for this drive
            if (SHFileOperation(&fo) == ERROR_SUCCESS)
            {
                // now create the new secure one
                bIsSecure = CreateRecyclerDirectory(idDrive);

                if (bIsSecure)
                {
                    // we re-created the recycle bin dir, so we need to reset the info file..
                    VerifyBBInfoFileHeader(idDrive);
                }
            }
        }
    }
   
    return bIsSecure;
}
#endif


//
// this verifies the info file header infomation
//
BOOL VerifyBBInfoFileHeader(int idDrive)
{
    BBDATAHEADER bbdh = {0, 0, 0, SIZEOF(BBDATAENTRY), 0}; // defaults
    HANDLE hFile;
    TCHAR szBBPath[MAX_PATH];
    TCHAR szInfo[MAX_PATH];
    BOOL fSuccess = FALSE;

    // check for the the old win95 INFO file
    DriveIDToBBPath(idDrive, szBBPath);

    GetBBInfoFileSpec(szBBPath, szInfo);

    hFile = CreateFile(szInfo,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_RANDOM_ACCESS,
                       NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwBytesRead;

        ReadFile(hFile, &bbdh, SIZEOF(BBDATAHEADER), &dwBytesRead, NULL);
        if (dwBytesRead == SIZEOF(BBDATAHEADER)) 
        {
            TraceMsg(TF_BITBUCKET, "Bitbucket: migrating info in old database file %s", szInfo);
            fSuccess = RevOldBBInfoFileHeader(hFile, &bbdh);
        }

        CloseHandle(hFile);

        if (fSuccess) 
        {
            // rename from INFO -> INFO2
            TCHAR szInfoNew[MAX_PATH];

            GetBBInfo2FileSpec(szBBPath, szInfoNew);
            TraceMsg(TF_BITBUCKET, "Bitbucket: renaming %s to %s !!", szInfo, szInfoNew);
            SHMoveFile(szInfo, szInfoNew, SHCNE_RENAMEITEM);
        }
        else
        {
            goto bad_info_file;
        }
    }

    // Failed to open or rev the old info file. Next, we check for the existance of the new info2 file
    // to see if the drive has a bitbucket format that is greater than what we can handle
    if (!fSuccess)
    {
        // we are going to be mucking with the info files, so we need to take this.
#ifdef BB_USE_MRSW
        MRSW_EnterRead(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

        hFile = OpenBBInfoFile(idDrive, OPENBBINFO_READ, 0);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD dwBytesRead;

            SetFilePointer(hFile, 0, NULL, FILE_BEGIN); // go to the beginning
            ReadFile(hFile, &bbdh, SIZEOF(BBDATAHEADER), &dwBytesRead, NULL);
            CloseBBInfoFile(hFile, idDrive);
#ifdef BB_USE_MRSW
            MRSW_LeaveRead(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

            if (dwBytesRead != SIZEOF(BBDATAHEADER) ||
		bbdh.idVersion > BITBUCKET_CURRENT_VERSION ||
		(bbdh.cbDataEntrySize != SIZEOF(BBDATAENTRYA) && bbdh.cbDataEntrySize != SIZEOF(BBDATAENTRYW)))
            {
		TCHAR szDriveName[MAX_PATH];

                // either we had a corrupt win95 info file, or an info2 file whose version is greater than ours
                // so we just empy the recycle bin.
bad_info_file:
		// since we failed to read the existing header, assume the native format
		g_pBitBucket[idDrive]->fIsUnicode = (SIZEOF(BBDATAENTRY) == SIZEOF(BBDATAENTRYW));

		// find out which drive it is that is corrupt
		DriveIDToBBRoot(idDrive, szDriveName);

                if (ShellMessageBox(HINST_THISDLL, 
                                    NULL,
                                    MAKEINTRESOURCE(IDS_RECYCLEBININVALIDFORMAT),
                                    MAKEINTRESOURCE(IDS_WASTEBASKET),
                                    MB_YESNO | MB_ICONEXCLAMATION | MB_SETFOREGROUND,
				    szDriveName) == IDYES)
                {
                    // nuke this bucket since it is hosed
                    PurgeOneBitBucket(NULL, idDrive, SHERB_NOCONFIRMATION);
                    return TRUE;
                }

		// If the user hit "no", we reset the header but leave all the entries alone.
		// We do our best to get cbDataEntrySize right, and if the rest of the info file
		// isint corrupted, then there is a good chance that this will work.
#ifdef BB_USE_MRSW
		MRSW_EnterWrite(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW
		hFile = OpenBBInfoFile(idDrive, OPENBBINFO_WRITE, 0);

		if (hFile != INVALID_HANDLE_VALUE)
		{
		    DWORD dwBytesWritten;

		    bbdh.idVersion = BITBUCKET_CURRENT_VERSION;

		    if (bbdh.cbDataEntrySize != SIZEOF(BBDATAENTRYW) &&
			bbdh.cbDataEntrySize != SIZEOF(BBDATAENTRYA))
		    {
			// assume the native data entry size
			bbdh.cbDataEntrySize = SIZEOF(BBDATAENTRY);
		    }

		    g_pBitBucket[idDrive]->fIsUnicode = (bbdh.cbDataEntrySize == SIZEOF(BBDATAENTRYW));
		    
		    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		    WriteFile(hFile, (LPBYTE)&bbdh, SIZEOF(BBDATAHEADER), &dwBytesWritten, NULL);
		    ASSERT(dwBytesWritten == SIZEOF(BBDATAHEADER));
		    
		    CloseBBInfoFile(hFile, idDrive);
		    fSuccess = TRUE;
		}
		else
		{
		    // we are so screwed!
		    fSuccess = FALSE;
		}

#ifdef BB_USE_MRSW
		MRSW_LeaveWrite(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

            }
            else if (bbdh.idVersion != BITBUCKET_CURRENT_VERSION)
            {
                // old info2 information (is this case even possible?)
                fSuccess = RevOldBBInfoFileHeader(hFile, &bbdh);
            }
            else
            {
                // the header info is current
                fSuccess = TRUE;
            }
        }
        else
        {
#ifdef BB_USE_MRSW
            MRSW_LeaveRead(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

            // brand spanking new drive, so go create the info file now.
            fSuccess = CreateInfoFile(idDrive);
        }
    }

    // get the only relevant thing in the header, whether it is unicode or not 
    g_pBitBucket[idDrive]->fIsUnicode = (bbdh.cbDataEntrySize == SIZEOF(BBDATAENTRYW));

    return fSuccess;
}


LONG FindInitialNextFileNum(idDrive)
{
    TCHAR szFile[MAX_PATH];
    TCHAR szBBFileSpec[MAX_PATH];
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    int iRet = 0;

    lstrcpy(szFile, TEXT("D*.*"));

    DriveIDToBBPath(idDrive, szBBFileSpec);
    PathAppend(szBBFileSpec, szFile);

    hFind = FindFirstFile(szBBFileSpec, &fd);

    // clear this out as we will reuse it
    szFile[0] = TEXT('\0');

    if (hFind == INVALID_HANDLE_VALUE)
        return 0;

    do
    {
        if (PathIsDotOrDotDot(fd.cFileName) || lstrcmpi(fd.cFileName, c_szDesktopIni) == 0)
        {
            continue;
        }
        else
        {
            int iCurrent = BBPathToIndex(fd.cFileName);

            if (iCurrent > iRet)
            {
                iRet = iCurrent;
            }
        }
    
    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);

    ASSERT(iRet >= 0);
    
    return (LONG)iRet;
}


BOOL InitBBDriveInfo(int idDrive)
{
    TCHAR szName[MAX_PATH];
    DWORD dwDisp;
    LONG lInitialCount = 0;
    
    // build up the string "BitBucket.<drive letter>"
    lstrcpy(szName, TEXT("BitBucket.")); 
    DriveIDToBBRegKey(idDrive, &szName[10]);

#ifdef BB_USE_MRSW
    g_pBitBucket[idDrive]->pmrsw = MRSW_Create(szName); // MRSW.BitBucket.<drive letter>
#endif // BB_USE_MRSW

    lstrcpy(&szName[11], TEXT(".DirtyCount"));
    g_pBitBucket[idDrive]->hgcDirtyCount = SHGlobalCounterCreateNamed(szName, 0); // BitBucket.<drive letter>.DirtyCount

    if (g_pBitBucket[idDrive]->hgcDirtyCount == INVALID_HANDLE_VALUE)
    {
        ASSERTMSG(FALSE, "BitBucket: failed to create hgcDirtyCount for drive %d !!", idDrive);
        return FALSE;
    }

    // set our dirty count to the current value, since we are going to read the stuff from the registry next
    g_pBitBucket[idDrive]->lCurrentDirtyCount = SHGlobalCounterGetValue(g_pBitBucket[idDrive]->hgcDirtyCount);

    // now create the subkey for this drive
    DriveIDToBBRegKey(idDrive, szName);

    // the per-user key is volatile since we only use this for temporary bookeeping (eg need to purge / compact).
    // the exception to this rule is the SERVERDRIVE case, because this is the users "My Documents" so we let it 
    // and we also need to store the path under that key (it has to roam with the user)
    if (RegCreateKeyEx(g_hkBitBucketPerUser, 
                       szName,
                       0,
                       NULL,
                       (SERVERDRIVE == idDrive) ? REG_OPTION_NON_VOLATILE : REG_OPTION_VOLATILE,
                       KEY_SET_VALUE | KEY_QUERY_VALUE,
                       NULL,
                       &g_pBitBucket[idDrive]->hkeyPerUser,
                       &dwDisp) != ERROR_SUCCESS)
    {
        ASSERTMSG(FALSE, "BitBucket: Could not create HKCU BitBucket registry key for drive %s", szName);
        g_pBitBucket[idDrive]->hkeyPerUser = NULL;
        return FALSE;
    }

    if (RegCreateKeyEx(g_hkBitBucket, 
                       szName,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       MAXIMUM_ALLOWED,
                       NULL,
                       &g_pBitBucket[idDrive]->hkey,
                       &dwDisp) != ERROR_SUCCESS)
    {
        TraceMsg(TF_BITBUCKET, "BitBucket: Could not create HKLM BitBucket registry key for drive %s, falling back to HKLM global key! ", szName);
        if (RegOpenKeyEx(g_hkBitBucket,
                         NULL,
                         0,
                         MAXIMUM_ALLOWED,
                         &g_pBitBucket[idDrive]->hkey) != ERROR_SUCCESS)
        {
            ASSERTMSG(FALSE, "BitBucket: Could not duplicate HKLM Global Bitbucket key!");
            return FALSE;
        }
    }

    // load the rest of the settings (hgcNextFileNum, fIsUnicode, iPercent, cbMaxSize, dwClusterSize, and fNukeOnDelete)
    return GetBBDriveSettings(idDrive, NULL);
}


BOOL AllocBBDriveInfo(int idDrive)
{
    TCHAR szBBPath[MAX_PATH];
    LPITEMIDLIST pidl;
    BOOL bRet = FALSE; // assume failure

    DriveIDToBBPath(idDrive, szBBPath);
    pidl = ILCreateFromPath(szBBPath);

    if (!pidl && !PathFileExists(szBBPath))
    {
        if (CreateRecyclerDirectory(idDrive))
        {
            pidl = ILCreateFromPath(szBBPath);
        }
    }

    if (pidl)
    {
        PBBSYNCOBJECT pbbso = LocalAlloc(LPTR, SIZEOF(BBSYNCOBJECT));

        if (pbbso)
        {
            if (SHInterlockedCompareExchange(&g_pBitBucket[idDrive], pbbso, NULL))
            {
                DWORD dwInitialTickCount;
                BOOL bKeepWaiting = TRUE;

                // Some other thread beat us to creating this bitbucket.
                // We can't return until that thread has inited the bitbucket
                // since some of the members might not be valid yet.
                LocalFree(pbbso);
                ILFree(pidl);

                dwInitialTickCount = GetTickCount();

                do
                {
                    if (g_pBitBucket[idDrive] == (PBBSYNCOBJECT)-1)
                    {
                        // this volume is flagged as not being recycleable for some reason...
                        break;
                    }

                    // Spin until the bitbucket struct is inited
                    Sleep(0);
                    
                    bKeepWaiting = !IsBitBucketInited(idDrive);

                    // we should never spin more than ~15 seconds
                    if (GetTickCount() <= (dwInitialTickCount + (15 * 1000)))
                    {
                        ASSERTMSG(FALSE, "AllocBBDriveInfo: other thread took longer that 15 seconds to init a bitbucket?!?");
                        break;
                    }

                } while (bKeepWaiting);

                return ((g_pBitBucket[idDrive] != NULL) && 
                        (g_pBitBucket[idDrive] != (PBBSYNCOBJECT)-1));
            }

            ASSERT(g_pBitBucket[idDrive] && (g_pBitBucket[idDrive] != (PBBSYNCOBJECT)-1));
            g_pBitBucket[idDrive]->pidl = pidl;
            g_pBitBucket[idDrive]->cchBBDir = lstrlen(szBBPath);


            if (InitBBDriveInfo(idDrive))
            {
                // Success!!
                g_pBitBucket[idDrive]->fInited = TRUE;
                bRet = TRUE;
            }
            else
            {
                // we failed for some weird reason
                TraceMsg(TF_WARNING, "Bitbucket: InitBBDriveInfo() failed on drive %d", idDrive);
                ILFree(pidl);
                
                ENTERCRITICAL;
                // take the critical section to protect people who call IsBitBucketInited()
                FreeBBInfo(g_pBitBucket[idDrive]);

                if (idDrive == SERVERDRIVE)
                {
                    // We set it to null in the serverdrive case so we will always retry. This allows 
                    // the user to re-direct and try to recycle on a new location.
                    g_pBitBucket[idDrive] = NULL;
                }
                else
                {
                    // set it to -1 here so we dont try any future recycle operations on this volume
                    g_pBitBucket[idDrive] = (PBBSYNCOBJECT)-1;
                }
                LEAVECRITICAL;
            }
        }
        else
        {
            ASSERTMSG(FALSE, "Bitbucket: failed to allocate a BBSYNCOBJECT for drive %d", idDrive);
            ILFree(pidl);
        }
    }
    else
    {
        ASSERTMSG(FALSE, "Bitbucket: failed to create pidl for path %s !!", szBBPath);
    }

    return bRet;
}


BOOL InitBBGlobals()
{
    if (!g_fBBInited)
    {
        // Save this now beceause at shutdown the desktop window will already be gone,
        // so we need to find out if we are the main explorer process now.
        if (!g_bIsProcessExplorer)
        {
            g_bIsProcessExplorer = IsWindowInProcess(GetShellWindow());
        }

        // do we have our global hkey that points to HKLM\Software\Microsoft\Windows\CurrentVersion\BitBucket yet?
        if (!g_hkBitBucket)
        {
            DWORD dwDisp = 0;
            HKEY hkExplorer = SHGetExplorerHkey(HKEY_LOCAL_MACHINE, TRUE);
        
            if (RegCreateKeyEx(hkExplorer,
                               TEXT("BitBucket"),
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               MAXIMUM_ALLOWED, NULL,
                               &g_hkBitBucket,
                               &dwDisp) != ERROR_SUCCESS)
            {
                ASSERTMSG(FALSE, "Bitbucket: Could not create g_hkBitBucket!");
                return FALSE;
            }

            ASSERTMSG(dwDisp == REG_OPENED_EXISTING_KEY, "Bitbucket: g_hkBitBucket was re-created, regsvr32 on shell32 must have failed!");
        }

        // do we have our global hkey that points to HKCU\Software\Microsoft\Windows\CurrentVersion\BitBucket yet?
        if (!g_hkBitBucketPerUser)
        {
            DWORD dwDisp = 0;
            HKEY hkExplorer = SHGetExplorerHkey(HKEY_CURRENT_USER, TRUE);
        
            if (RegCreateKeyEx(hkExplorer,
                               TEXT("BitBucket"),
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               MAXIMUM_ALLOWED, NULL,
                               &g_hkBitBucketPerUser,
                               &dwDisp) != ERROR_SUCCESS)
            {
                ASSERTMSG(FALSE, "Bitbucket: Could not create g_hkBitBucketPerUser!");
                return FALSE;
            }
        }

        // have we initialized the global settings dirty counter yet
        if (g_hgcGlobalDirtyCount == INVALID_HANDLE_VALUE)
        {
            g_hgcGlobalDirtyCount = SHGlobalCounterCreateNamed(TEXT("BitBucket.GlobalDirtyCount"), 0);

            if (g_hgcGlobalDirtyCount == INVALID_HANDLE_VALUE)
            {
                ASSERTMSG(FALSE, "Bitbucket: failed to create g_hgcGlobalDirtyCount!");
                return FALSE;
            }

            g_lProcessDirtyCount = SHGlobalCounterGetValue(g_hgcGlobalDirtyCount);
        }

        // have we initialized the global # of people doing recycle bin file operations?
        if (g_hgcNumDeleters == INVALID_HANDLE_VALUE)
        {
            g_hgcNumDeleters = SHGlobalCounterCreateNamed(TEXT("BitBucket.NumDeleters"), 0);

            if (g_hgcGlobalDirtyCount == INVALID_HANDLE_VALUE)
            {
                ASSERTMSG(FALSE, "Bitbucket: failed to create g_hgcGlobalDirtyCount!");
                return FALSE;
            }
        }

        // we inited everything!!
        g_fBBInited = TRUE;
    }

    return g_fBBInited;
}


void FreeBBInfo(PBBSYNCOBJECT pbbso)
{
#ifdef BB_USE_MRSW
    if (pbbso->pmrsw)
        MRSW_Destroy(pbbso->pmrsw);
#endif // BB_USE_MRSW
    if (pbbso->hgcNextFileNum)
        CloseHandle(pbbso->hgcNextFileNum);

    if (pbbso->hgcDirtyCount)
        CloseHandle(pbbso->hgcDirtyCount);

    if (pbbso->hkey)
        RegCloseKey(pbbso->hkey);
    
    if (pbbso->hkeyPerUser)
        RegCloseKey(pbbso->hkeyPerUser);

    LocalFree(pbbso);
}


//
// This function is exported from shell32 so that explorer can call us during WM_ENDSESSION
// and we can go save a bunch of state and free all the semaphores. 
STDAPI_(void) SaveRecycleBinInfo()
{
    if (g_bIsProcessExplorer)
    {
        LONG lGlobalDirtyCount;
        BOOL bGlobalUpdate = FALSE; // did global settings change?
        int i;

        // We are going to persist the info to the registry, so check to see if we need to 
        // update our info now
        lGlobalDirtyCount = SHGlobalCounterGetValue(g_hgcGlobalDirtyCount);
        if (g_lProcessDirtyCount < lGlobalDirtyCount)
        {
            g_lProcessDirtyCount = lGlobalDirtyCount;
            RefreshAllBBDriveSettings();
            bGlobalUpdate = TRUE;
        }

        for (i = 0; i < MAX_BITBUCKETS ; i++)
        {
            if (IsBitBucketInited(i))
            {
                LONG lBucketDirtyCount = SHGlobalCounterGetValue(g_pBitBucket[i]->hgcDirtyCount);

                // if we didnt do a global update, check this bucket specifically to see if it is dirty
                // and we need to update it
                if (!bGlobalUpdate && g_pBitBucket[i]->lCurrentDirtyCount < lBucketDirtyCount)
                {
                    g_pBitBucket[i]->lCurrentDirtyCount = lBucketDirtyCount;
                    RefreshBBDriveSettings(i);
                }

                // save all of the volume serial # and whether the drive is unicode to the registry
                PersistBBDriveInfo(i);

                // we also update the header for win98/IE4 compat
                UpdateBBInfoFileHeader(i);
            }
        }
    }
}


void BitBucket_Terminate()
{
    int i;

    // free the global recycle bin structs
    for (i = 0; i < MAX_BITBUCKETS ; i++)
    {
        if ((g_pBitBucket[i]) && (g_pBitBucket[i] != (PBBSYNCOBJECT)-1))
        {
            ENTERCRITICAL;
            FreeBBInfo(g_pBitBucket[i]);
            g_pBitBucket[i] = NULL;
            LEAVECRITICAL;
        }
    }

    if (g_hgcGlobalDirtyCount != INVALID_HANDLE_VALUE)
        CloseHandle(g_hgcGlobalDirtyCount);

    if (g_hgcNumDeleters != INVALID_HANDLE_VALUE)
        CloseHandle(g_hgcNumDeleters);

    if (g_hkBitBucketPerUser != NULL)
	RegCloseKey(g_hkBitBucketPerUser);

    if (g_hkBitBucket != NULL)
        RegCloseKey(g_hkBitBucket);
}

//
// refreshes g_pBitBucket with new global settings
//
BOOL RefreshAllBBDriveSettings()
{
    int i;

    // since global settings changes affect all the drives, update all the drives
    for (i = 0; i < MAX_BITBUCKETS; i++)
    {
        if ((g_pBitBucket[i]) && (g_pBitBucket[i] != (PBBSYNCOBJECT)-1))
        {
            RefreshBBDriveSettings(i);
        }
    }
    
    return TRUE;
}


BOOL RefreshBBDriveSettings(int idDrive)
{
    HKEY hkey;
    ULARGE_INTEGER ulMaxSize;
    BOOL fUseGlobalSettings = TRUE;
    DWORD dwSize = SIZEOF(fUseGlobalSettings);

    ASSERT(g_pBitBucket[idDrive] && (g_pBitBucket[idDrive] != (PBBSYNCOBJECT)-1));

    RegQueryValueEx(g_hkBitBucket, TEXT("UseGlobalSettings"), NULL, NULL, (LPBYTE)&fUseGlobalSettings, &dwSize);
    
    if (fUseGlobalSettings)
    {
        hkey = g_hkBitBucket;
    }
    else
    {
        hkey = g_pBitBucket[idDrive]->hkey;
    }

    // re-read the iPercent and fNukeOnDelete 
    //
    // note: the RegQueryValueEx calls may fail if we are not using global settings and the user has not 
    //       tweaked the settings for this drive yet or shutdown explorer so that they were properly persisted
    dwSize = SIZEOF(g_pBitBucket[idDrive]->iPercent);
    RegQueryValueEx(hkey, TEXT("Percent"), NULL, NULL, (LPBYTE)&g_pBitBucket[idDrive]->iPercent, &dwSize);

    dwSize = SIZEOF(g_pBitBucket[idDrive]->fNukeOnDelete);
    RegQueryValueEx(hkey, TEXT("NukeOnDelete"), NULL, NULL, (LPBYTE)&g_pBitBucket[idDrive]->fNukeOnDelete, &dwSize);

    // re-calculate cbMaxSize based on the new iPercent
    ulMaxSize.QuadPart = min((g_pBitBucket[idDrive]->qwDiskSize / 100) * g_pBitBucket[idDrive]->iPercent, (DWORD)-1);
    ASSERT(ulMaxSize.HighPart == 0);
    g_pBitBucket[idDrive]->cbMaxSize = ulMaxSize.LowPart;

    // since we just refreshed the settings from the registry, we are now up to date
    g_pBitBucket[idDrive]->lCurrentDirtyCount = SHGlobalCounterGetValue(g_pBitBucket[idDrive]->hgcDirtyCount);

    return TRUE;
}


//
// this function is used to compact the bitbucked INFO files.
//
// we do a lazy delete (just mark the entries as deleted) and when we hit a
// certain number of bogus entries in the info file, we need to go through and clean up the
// garbage and compact the file.
//
DWORD CALLBACK CompactBBInfoFileThread(void *pData)
{
    int idDrive = PtrToLong(pData);
    HANDLE hFile;

    //
    // BUGBUG (reinerf) - as an optimization, we might want to check here to see
    // if someone is waiting to empty the bitbucket since if we are going to empty
    // this bucket there is no point in wasting time compacting it.
    //

    // we are about to start writing, so grab the MRSW for write access
#ifdef BB_USE_MRSW
    MRSW_EnterWrite(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

    hFile = OpenBBInfoFile(idDrive, OPENBBINFO_WRITE, 0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        // work in chunks of 10
        BBDATAENTRYW bbdewArray[10]; // use a unicode array, but it might end up holding BBDATAENTRYA stucts
        LPBBDATAENTRYW pbbdew = bbdewArray;
        int iNumEntries = 0;
        DWORD dwDataEntrySize = g_pBitBucket[idDrive]->fIsUnicode ? SIZEOF(BBDATAENTRYW) : SIZEOF(BBDATAENTRYA);
        DWORD dwWritePos = 0;
        DWORD dwReadPos = 0;
        DWORD dwBytesWritten;

        // save off the inital write pos
        dwWritePos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

        while (ReadNextDataEntry(hFile, pbbdew, TRUE, dwDataEntrySize, idDrive))
        {
            ASSERT(!IsDeletedEntry(pbbdew));

            iNumEntries++;

            // do we have 10 entries yet?
            if (iNumEntries == 10)
            {
                iNumEntries = 0;

                TraceMsg(TF_BITBUCKET, "Bitbucket: Compacting drive %d: dwRead = %d, dwWrite = %d, writing 10 entries", idDrive, dwReadPos, dwWritePos);

                // save where we are for reading
                dwReadPos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

                // then go to where we are for writing
                SetFilePointer(hFile, dwWritePos, NULL, FILE_BEGIN);

                // write it out
                if (!WriteFile(hFile, (LPBYTE)bbdewArray, dwDataEntrySize * 10, &dwBytesWritten, NULL) || dwBytesWritten != (dwDataEntrySize * 10))
                {
                    // we're in big trouble if this happens.
                    // bail completely so that at worst, we only have a few bad records.
                    // if we keep trying to write from this point, but the write point is
                    // screwed, we'll nuke all the records
                    ASSERTMSG(FALSE, "Bitbucket: we were compacting drive %d and it is totally screwed!! Iiiieeee!", idDrive);
                    break;
                }

                // sucess! move our write pos to the end of were we finished writing
                dwWritePos += (dwDataEntrySize * 10);
                
                // go back to were we left off reading
                SetFilePointer(hFile, dwReadPos, NULL, FILE_BEGIN);

                // reset our lparray pointer
                pbbdew = bbdewArray;
            }
            else
            {
                // dont have 10 entries yet, so keep going
                pbbdew = (LPBBDATAENTRYW)((LPBYTE)pbbdew + dwDataEntrySize);
            }
        }

        TraceMsg(TF_BITBUCKET, "Bitbucket: Compacting drive %d: dwRead = %d, dwWrite = %d, writing last %d entries", idDrive, dwReadPos, dwWritePos, iNumEntries);

        // write whatever we have left over
        SetFilePointer(hFile, dwWritePos, NULL, FILE_BEGIN);
        WriteFile(hFile, (LPBYTE)bbdewArray, dwDataEntrySize * iNumEntries, &dwBytesWritten, NULL);
        ASSERT(dwBytesWritten == (dwDataEntrySize * iNumEntries));
        SetEndOfFile(hFile);
        CloseBBInfoFile(hFile, idDrive);
    }

    // we are done writing, so release MRSW 
#ifdef BB_USE_MRSW
    MRSW_LeaveWrite(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

    return 0;
}

void CompactBBInfoFile(int idDrive)
{
    HANDLE hThread;
    DWORD idThread;

    // try to spin up a background thread to do the work for us
    hThread = CreateThread(NULL, 0, CompactBBInfoFileThread, (LPVOID)idDrive, 0, &idThread);

    if (hThread)
    {
        // let the background thread do the work
        CloseHandle(hThread);
    }
    else
    {
        TraceMsg(TF_BITBUCKET, "BBCompact - failed to create backgound thread! Doing work on this thread");
        CompactBBInfoFileThread((LPVOID)idDrive);
    }
}


void GetDeletedFileName(LPTSTR pszFileName, int idDrive, int iIndex, LPNTSTR pszOriginal)
{
    //
    // use lowercase 'a' since when we call CFSFolder_CreateIDForItem it creates a pidl
    // whose filname has a lower-case second character, and therefore the filenames will
    // match and we get a perf win during all the CBitBucket_SF_CompareID calls
    //
    TCHAR c = TEXT('a') + idDrive;

    if (c == (TEXT('a') + SERVERDRIVE))
    {
        c = TEXT('@');
    }

#ifdef ALIGNMENT_SCENARIO
    {
        TCHAR szTmp[MAX_PATH];
        ualstrcpyn(szTmp, pszOriginal, ARRAYSIZE(szTmp));
        wsprintf(pszFileName, TEXT("D%c%d%s"), c, iIndex, PathFindExtension(szTmp));
    }
#else
    wsprintf(pszFileName, TEXT("D%c%d%s"), c, iIndex, PathFindExtension(pszOriginal));
#endif
}

void UpdateIcon(BOOL fFull)
{
    LONG    cbData;
    DWORD   dwType;
    HKEY    hkeyCLSID = NULL;
    HKEY    hkeyUserCLSID = NULL;
    TCHAR   szTemp[MAX_PATH];
    TCHAR   szNewValue[MAX_PATH];
    TCHAR   szValue[MAX_PATH];

    TraceMsg(TF_BITBUCKET, "BitBucket: UpdateIcon %s", fFull ? TEXT("Full") : TEXT("Empty"));

    szValue[0] = 0;
    szNewValue[0] = 0;

    // get the HKCR CLSID key (HKCR\CLSID\CLSID_RecycleBin\DefaultIcon)
    if (FAILED(SHRegGetCLSIDKey(&CLSID_RecycleBin, c_szDefaultIcon, FALSE, FALSE, &hkeyCLSID)))
        goto error;

    // get the per-user CLSID
    // (HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\CLSID\CLSID_RecycleBin\DefaultIcon)
    if (FAILED(SHRegGetCLSIDKey(&CLSID_RecycleBin, c_szDefaultIcon, TRUE, FALSE, &hkeyUserCLSID)))
    {
        // it most likely failed because the reg key dosent exist, so create it now
        if (FAILED(SHRegGetCLSIDKey(&CLSID_RecycleBin, c_szDefaultIcon, TRUE, TRUE, &hkeyUserCLSID)))
            goto error;

        // now that we created it, lets copy the stuff from HKLM there
        
        // get the local machine default icon
        cbData = SIZEOF(szTemp);
        if (RegQueryValueEx(hkeyCLSID, NULL, 0, &dwType, (LPBYTE)szTemp, &cbData) != ERROR_SUCCESS)
            goto error;

        // set the per-user default icon
        RegSetValueEx(hkeyUserCLSID, NULL, 0, dwType, (LPBYTE)szTemp, (lstrlen(szTemp) + 1) * SIZEOF(TCHAR));
        
        // get the local machine full icon
        cbData = SIZEOF(szTemp);
        if (RegQueryValueEx(hkeyCLSID, TEXT("Full"), 0, &dwType, (LPBYTE)szTemp, &cbData) != ERROR_SUCCESS)
            goto error;

        // set the per-user full icon
        RegSetValueEx(hkeyUserCLSID, TEXT("Full"), 0, dwType, (LPBYTE)szTemp, (lstrlen(szTemp) + 1) * SIZEOF(TCHAR));

        // get the local machine empty icon
        cbData = SIZEOF(szTemp);
        if (RegQueryValueEx(hkeyCLSID, TEXT("Empty"), 0, &dwType, (LPBYTE)szTemp, &cbData) != ERROR_SUCCESS)
            goto error;

        // set the per-user empty icon
        RegSetValueEx(hkeyUserCLSID, TEXT("Empty"), 0, dwType, (LPBYTE)szTemp, (lstrlen(szTemp) + 1) * SIZEOF(TCHAR));
    }

    // try the per user first, if we dont find it, then copy the information from HKCR\CLSID\etc...
    // to the per-user location
    cbData = SIZEOF(szTemp);
    if (RegQueryValueEx(hkeyUserCLSID, NULL, 0, &dwType, (LPBYTE)szTemp, &cbData) != ERROR_SUCCESS)
    {
        // get the local machine default icon
        cbData = SIZEOF(szTemp);
        if (RegQueryValueEx(hkeyCLSID, NULL, 0, &dwType, (LPBYTE)szTemp, &cbData) != ERROR_SUCCESS)
            goto error;

        // set the per-user default icon
        RegSetValueEx(hkeyUserCLSID, NULL, 0, dwType, (LPBYTE)szTemp, (lstrlen(szTemp) + 1) * SIZEOF(TCHAR));
    }
    lstrcpy(szValue, szTemp);

    cbData = SIZEOF(szTemp);
    if (RegQueryValueEx(hkeyUserCLSID, fFull ? TEXT("Full") : TEXT("Empty"), 0, &dwType, (LPBYTE)szTemp, &cbData) != ERROR_SUCCESS)
    {
        cbData = SIZEOF(szTemp);
        if (RegQueryValueEx(hkeyCLSID, fFull ? TEXT("Full") : TEXT("Empty"), 0, &dwType, (LPBYTE)szTemp, &cbData) != ERROR_SUCCESS)
            goto error;

        // set the per-user full/empty icon
        RegSetValueEx(hkeyUserCLSID, fFull ? TEXT("Full") : TEXT("Empty"), 0, dwType, (LPBYTE)szTemp, (lstrlen(szTemp) + 1) * SIZEOF(TCHAR));
    }
    lstrcpy(szNewValue, szTemp);

    if (lstrcmpi(szNewValue, szValue) != 0)
    {
        LPTSTR szIconIndex;


//  99/01/05 #257290 vtan: Match the expanded strings but copy the
//  REG_EXPAND_SZ strings.

        cbData = SIZEOF(szTemp);
        TW32(RegQueryValueEx(hkeyUserCLSID, fFull ? TEXT("Full") : TEXT("Empty"), 0, &dwType, (LPBYTE)szTemp, &cbData));
        // we always update the per user default icon, because recycle bins are per user on NTFS
        RegSetValueEx(hkeyUserCLSID, NULL, 0, dwType, (LPBYTE)szTemp, (lstrlen(szTemp) + 1) * SIZEOF(TCHAR));

        szIconIndex = StrRChr(szValue, NULL, TEXT(','));

        if (szIconIndex)  
        {
            int id;
            int iNum = StrToInt(szIconIndex + 1);
            LPTSTR pszDllName;

            *szIconIndex = 0; // end szValue after the dll name
            pszDllName = PathFindFileName(szValue);

            // ..and tell anyone viewing this image index to update
            id = LookupIconIndex(pszDllName, iNum, 0);
            SHUpdateImage(pszDllName, iNum, 0, id);
            SHChangeNotifyHandleEvents();
        }
    }

error:
    if (hkeyCLSID)
        RegCloseKey(hkeyCLSID);
    
    if (hkeyUserCLSID)
        RegCloseKey(hkeyUserCLSID);
}


//
// this loads the settings for this drive.  it obeys the "use global" bit
//
BOOL GetBBDriveSettings(int idDrive, ULONGLONG *pcbDiskSpace)
{
    TCHAR szDrive[MAX_PATH];
    TCHAR szName[MAX_PATH];
    TCHAR szVolume[MAX_PATH];
    TCHAR szPath[MAX_PATH];
    DWORD dwSizePath = ARRAYSIZE(szPath);
    ULARGE_INTEGER ulFreeUser, ulTotal, ulFree;
    DWORD dwSize1, dwSize2;
    DWORD dwSerialNumber, dwSerialNumberFromRegistry;
    LONG lInitialCount;
    BOOL fUseGlobalSettings = TRUE;
    BOOL bHaveCachedRegInfo = FALSE;
    BOOL bRet = TRUE;
    HKEY hkey;

    // Get volume root since we are going to call GetVolumeInformation()
    DriveIDToBBVolumeRoot(idDrive, szVolume);

    DriveIDToBBPath(idDrive, szDrive);
    GetBBInfo2FileSpec(szDrive, szName);

    if (idDrive == SERVERDRIVE)
    {
        // in the SERVERDRIVE case everything is under HKCU, so use the per-user key
        hkey = g_pBitBucket[idDrive]->hkeyPerUser;
    }
    else
    {
        hkey = g_pBitBucket[idDrive]->hkey;
    }

    // first we need to check to see we have cached registry info for this drive, or if this 
    // is a new drive
    dwSize1 = SIZEOF(dwSerialNumber);

    if (PathFileExists(szName)                                  &&
        (RegQueryValueEx(hkey,
                         TEXT("VolumeSerialNumber"),
                         NULL,
                         NULL,
                         (LPBYTE)&dwSerialNumberFromRegistry,
                         &dwSize1) == ERROR_SUCCESS)            &&
        GetVolumeInformation(szVolume,
                             NULL,
                             0,
                             &dwSerialNumber,
                             NULL,
                             NULL,
                             NULL,
                             0)                                 &&
        (dwSerialNumber == dwSerialNumberFromRegistry))
    {
        // we were able to read the drive serial number and it matched the regsitry, so
        // assume that the cached reg info is valid
        bHaveCachedRegInfo = TRUE;
    }
    
    // do some extra checks in the SERVERDRIVE case to make sure that the path matches in addition to the volume serial number.
    // (eg nethomedir could be on the same volume but a different path)
    if (bHaveCachedRegInfo && (SERVERDRIVE == idDrive))
    {
        if ((RegQueryValueEx(hkey, TEXT("Path"), NULL, NULL, (LPBYTE) szPath, &dwSizePath) != ERROR_SUCCESS) ||
            (lstrcmpi(szPath, szDrive) != 0))
        {
            // couldn't read the path or it didnt match, so no we can't use the cacehed info
            bHaveCachedRegInfo = FALSE;
        }
    }

 
    if (!bHaveCachedRegInfo)
    {
        TraceMsg(TF_BITBUCKET, "Bitbucket: new drive %s detected!!!", szDrive);
        // this is a new volume, so delete any old registry info we had
        DeleteOldBBRegInfo(idDrive);
        
        // And also migrate the win95 info if it exists
        // NOTE: this also fills in the g_pBitBucket[idDrive]->fIsUnicode
        VerifyBBInfoFileHeader(idDrive);
    }
    else
    {
        // set g_pBitBucket[idDrive]->fIsUnicode based on the registry info
        dwSize1 = SIZEOF(g_pBitBucket[idDrive]->fIsUnicode);
        if (RegQueryValueEx(hkey, TEXT("IsUnicode"), NULL, NULL, (LPBYTE)&g_pBitBucket[idDrive]->fIsUnicode, &dwSize1) != ERROR_SUCCESS)
        {
            ASSERTMSG(FALSE, "Bitbucket: IsUnicode missing from registry for drive %s !!", szDrive);
            
            // instead, try to get this out of the header
            VerifyBBInfoFileHeader(idDrive);
        }
    }

#ifdef WINNT
    // we need to check to make sure that the Recycle Bin folder is properly secured
    if (!CheckRecycleBinAcls(idDrive))
    {
        // we return false if this fails (meaning we detected an unsecure directory and were unable to 
        // fix it or the user didnt want to fix it). This will effectively disable all recycle bin operations
        // on this volume for this session.
        return FALSE;
    }
#endif

    // calculate the next file num index
    lInitialCount = FindInitialNextFileNum(idDrive);

    // create the hgcNextFileNume global counter
    ASSERT(lInitialCount >= 0);
    lstrcpy(szName, TEXT("BitBucket.")); 
    DriveIDToBBRegKey(idDrive, &szName[10]);
    lstrcpy(&szName[11], TEXT(".NextFileNum"));
    g_pBitBucket[idDrive]->hgcNextFileNum = SHGlobalCounterCreateNamed(szName, lInitialCount); // BitBucket.<drive letter>.NextFileNum

    if (g_pBitBucket[idDrive]->hgcNextFileNum == INVALID_HANDLE_VALUE)
    {
        ASSERTMSG(FALSE, "BitBucket: failed to create hgcNextFileNum for drive %s !!", szDrive);
        return FALSE;
    }

    dwSize1 = SIZEOF(fUseGlobalSettings);
    RegQueryValueEx(g_hkBitBucket, TEXT("UseGlobalSettings"), NULL, NULL, (LPBYTE)&fUseGlobalSettings, &dwSize1);

    if (fUseGlobalSettings)
    {
        // use the global settings for iPercent and fNukeOnDelete
        hkey = g_hkBitBucket;
    }
    else
    {
        // use the per-bitbucket settings for iPercent and fNukeOnDelete
        hkey = g_pBitBucket[idDrive]->hkey;
    }

retry:
    dwSize1 = SIZEOF(g_pBitBucket[idDrive]->iPercent);
    dwSize2 = SIZEOF(g_pBitBucket[idDrive]->fNukeOnDelete);

    if (RegQueryValueEx(hkey, TEXT("Percent"), NULL, NULL, (LPBYTE)&g_pBitBucket[idDrive]->iPercent, &dwSize1) != ERROR_SUCCESS ||
        RegQueryValueEx(hkey, TEXT("NukeOnDelete"), NULL, NULL, (LPBYTE)&g_pBitBucket[idDrive]->fNukeOnDelete, &dwSize2) != ERROR_SUCCESS)
    {
        if (hkey == g_hkBitBucket)
        {
            ASSERTMSG(FALSE, "Missing global bitbucket data: run regsvr32 on shell32.dll !!");
            return FALSE;
        }
        else
        {
            // we are missing the per-bitbuckt information, so fall back to the global stuff
            hkey = g_hkBitBucket;
            goto retry;
        }
    }

    // we call SHGetDiskFreeSpaceEx so we can respect quotas on NTFS
    DriveIDToBBRoot(idDrive, szDrive);
    if (SHGetDiskFreeSpaceEx(szDrive, &ulFreeUser, &ulTotal, &ulFree))
    {
        ULARGE_INTEGER ulMaxSize;

        g_pBitBucket[idDrive]->dwClusterSize = PathGetClusterSize(szDrive);
        g_pBitBucket[idDrive]->qwDiskSize = ulTotal.QuadPart;

        // we limit the max size of the recycle bin to ~4 gig
        ulMaxSize.QuadPart = min(((ulTotal.QuadPart / 100) * g_pBitBucket[idDrive]->iPercent), (DWORD)-1);
        ASSERT(ulMaxSize.HighPart == 0);
        g_pBitBucket[idDrive]->cbMaxSize = ulMaxSize.LowPart;

        if (pcbDiskSpace)
        {
            // set the total disk size as well
            *pcbDiskSpace = ulTotal.QuadPart;
        }
    }
    else
    {
        // REVIEW: (chrispi 11/20/98) where did this code come from?  should it be removed?
        if (idDrive == SERVERDRIVE)
        {
            g_pBitBucket[idDrive]->dwClusterSize = 2048;
            g_pBitBucket[idDrive]->cbMaxSize = ((0x7FFFFFFF / 1024) * ((1024 * g_pBitBucket[idDrive]->iPercent) / 100));
            g_pBitBucket[idDrive]->qwDiskSize = (ULONGLONG)((DWORD) -1);

            if (pcbDiskSpace)
            {
                *pcbDiskSpace = 0x7FFFFFFF;
            }
        }
        else
        {
            ASSERTMSG(FALSE, "Bitbucket: SHGetDiskFreeSpaceEx failed on %s !!", szDrive);
            
            g_pBitBucket[idDrive]->cbMaxSize = 0;
            g_pBitBucket[idDrive]->dwClusterSize = 0;
            g_pBitBucket[idDrive]->qwDiskSize = 0;

            if (pcbDiskSpace)
            {
                *pcbDiskSpace = 0;
            }
        }
    }

    TraceMsg(TF_BITBUCKET,
             "GetBBDriveSettings: Drive %s, fIsUnicode=%d, iPercent=%d, cbMaxSize=%d, fNukeOnDelete=%d, NextFileNum=%d",
             szDrive,
             g_pBitBucket[idDrive]->fIsUnicode,
             g_pBitBucket[idDrive]->iPercent,
             g_pBitBucket[idDrive]->cbMaxSize,
             g_pBitBucket[idDrive]->fNukeOnDelete,
             SHGlobalCounterGetValue(g_pBitBucket[idDrive]->hgcNextFileNum));

    return TRUE;
}


//
// cleans up old iPercent and fNukeOnDelete registry keys when we dectect a new drive
//
void DeleteOldBBRegInfo(idDrive)
{
    RegDeleteValue(g_pBitBucket[idDrive]->hkey, TEXT("Percent"));
    RegDeleteValue(g_pBitBucket[idDrive]->hkey, TEXT("NukeOnDelete"));
    RegDeleteValue(g_pBitBucket[idDrive]->hkey, TEXT("IsUnicode"));
}


//
// This gets called when explorer exits to persist the volume serial # and
// whether the drive is unicode for the specified drive.
//
void PersistBBDriveInfo(int idDrive)
{
    TCHAR szVolume[MAX_PATH];
    DWORD dwSerialNumber;
    HKEY hkey;

    if (SERVERDRIVE == idDrive)
    {
        TCHAR szPath[MAX_PATH];

        // in the SERVERDRIVE case everything is under HKCU, so use the per-user key
        hkey = g_pBitBucket[idDrive]->hkeyPerUser;

        DriveIDToBBPath(idDrive, szPath);
        RegSetValueEx(hkey, TEXT("Path"), 0, REG_SZ, (LPBYTE) szPath, SIZEOF(TCHAR) * (lstrlen(szPath) + 1));
    }
    else
    {
        hkey = g_pBitBucket[idDrive]->hkey;
    }

    DriveIDToBBVolumeRoot(idDrive, szVolume);

    // write out the volume serial # so we can detect when a new drive comes along and give it the default settings
    // NOTE: we will fail to write out the volume serial # if we are a normal user and HKLM is locked down. Oh well.
    if (GetVolumeInformation(szVolume, NULL, 0, &dwSerialNumber, NULL, NULL, NULL, 0))
    {
        RegSetValueEx(hkey, TEXT("VolumeSerialNumber"), 0, REG_DWORD, (LPBYTE)&dwSerialNumber, SIZEOF(dwSerialNumber));
    }

    // save off fIsUnicode as well
    RegSetValueEx(hkey, TEXT("IsUnicode"), 0, REG_DWORD, (LPBYTE)&g_pBitBucket[idDrive]->fIsUnicode, SIZEOF(g_pBitBucket[idDrive]->fIsUnicode));
}


//
// This is what gets called when the user tweaks the drive settings for all the drives (the global settings)
//
BOOL PersistGlobalSettings(BOOL fUseGlobalSettings, BOOL fNukeOnDelete, int iPercent)
{
    ASSERT(g_hkBitBucket);

    if (RegSetValueEx(g_hkBitBucket, TEXT("Percent"), 0, REG_DWORD, (LPBYTE)&iPercent, SIZEOF(iPercent)) != ERROR_SUCCESS ||
        RegSetValueEx(g_hkBitBucket, TEXT("NukeOnDelete"), 0, REG_DWORD, (LPBYTE)&fNukeOnDelete, SIZEOF(fNukeOnDelete)) != ERROR_SUCCESS ||
        RegSetValueEx(g_hkBitBucket, TEXT("UseGlobalSettings"), 0, REG_DWORD, (LPBYTE)&fUseGlobalSettings, SIZEOF(fUseGlobalSettings)) != ERROR_SUCCESS)        
    {
         TraceMsg(TF_BITBUCKET, "Bitbucket: failed to update global bitbucket data in the registry!!");
         return FALSE;
    }

    // since we just updated the global drive settings, we need to increment the dirty count and set our own
    g_lProcessDirtyCount = SHGlobalCounterIncrement(g_hgcGlobalDirtyCount);

    return TRUE;
}

//
// This is what gets called when the user tweaks the drive settings for a drive via the
// Recycle Bin property sheet page. The only thing we care about is the % slider and the
// "Do not move files to the recycle bin" settings.
// 
BOOL PersistBBDriveSettings(int idDrive, int iPercent, BOOL fNukeOnDelete)
{
    if (RegSetValueEx(g_pBitBucket[idDrive]->hkey, TEXT("Percent"), 0, REG_DWORD, (LPBYTE)&iPercent, SIZEOF(iPercent)) != ERROR_SUCCESS ||
        RegSetValueEx(g_pBitBucket[idDrive]->hkey, TEXT("NukeOnDelete"), 0, REG_DWORD, (LPBYTE)&fNukeOnDelete, SIZEOF(fNukeOnDelete)) != ERROR_SUCCESS)
    {
        TraceMsg(TF_BITBUCKET, "Bitbucket: unable to persist drive settings for drive %d", idDrive);
        return FALSE;
    }

    // since we just updated the drive settings, we need to increment the dirty count for this drive
    g_pBitBucket[idDrive]->lCurrentDirtyCount = SHGlobalCounterIncrement(g_pBitBucket[idDrive]->hgcDirtyCount);

    return TRUE;
}


//
// walks the multi-string pszSrc and sets up the undo info
//
void BBCheckRestoredFiles(LPTSTR pszSrc)
{
    LPTSTR pszTemp;

    if (pszSrc && IsFileInBitBucket(pszSrc)) 
    {
        pszTemp = pszSrc;

        while (*pszTemp) 
        {
            FOUndo_FileRestored(pszTemp);
            pszTemp += (lstrlen(pszTemp) + 1);
        }

        SHUpdateRecycleBinIcon();
    }
}


//
// This is the quick and efficent way to tell if the Recycle Bin is empty or not
//
STDAPI_(BOOL) IsRecycleBinEmpty()
{
    int i;
    
    for (i = 0; i < MAX_BITBUCKETS; i++) 
    {
        if (CountDeletedFilesOnDrive(i, NULL, 1))
            return FALSE;
    }

    return TRUE;
}


//
// Finds out how many files are deleted on this drive, and optionally the total size of those files.
// Also, stop counting if the total # of files gets above iMaxFiles.
//
// NOTE: if you pass iMaxFiles = 0, then we ignore the parameter and count up all the files/sizes
// 
int CountDeletedFilesOnDrive(int idDrive, LPDWORD pdwSize, int iMaxFiles)
{
    int cFiles = 0;
    HANDLE hFile;
    WIN32_FIND_DATA wfd;
    TCHAR szBBPath[MAX_PATH];
    TCHAR szBBFileSpec[MAX_PATH];

    if (pdwSize)
        *pdwSize = 0;
    
    if (!IsBitBucketableDrive(idDrive))
        return 0;

    DriveIDToBBPath(idDrive, szBBPath);
    PathCombine(szBBFileSpec, szBBPath, TEXT("D*.*"));

    hFile = FindFirstFile(szBBFileSpec, &wfd);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    do
    {
        if (PathIsDotOrDotDot(wfd.cFileName) || lstrcmpi(wfd.cFileName, c_szDesktopIni) == 0)
        {
            continue;
        }

        cFiles++;

        if (pdwSize)
        {
            if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                FOLDERCONTENTSINFO fci = {0};
                TCHAR szDir[MAX_PATH];
                fci.bContinue = TRUE;

                // BUGBUG (reinerf) - for perf we should try to avoid
                // calling FolderSize here. Perhaps we could encode the size
                // as part of the extension?
                lstrcpyn(szDir, szBBPath, ARRAYSIZE(szDir));
                PathAppend(szDir, wfd.cFileName);
                FolderSize(szDir, &fci);
                *pdwSize += (DWORD)(fci.cbSize);
            }
            else
            {
                // simple file case
                *pdwSize += wfd.nFileSizeLow;
            }
        }

        if (iMaxFiles && (cFiles > iMaxFiles))
            break;

    } while (FindNextFile(hFile, &wfd));

    FindClose(hFile);

    return cFiles;
}


//
// Returns the number of files in the Recycle Bin, and optionally the drive id
// if there's only one file, and optionally the total size of all the stuff.
//
// We also stop counting if iMaxFiles is nonzero and we find more that that many
// files. This helps perf by having a cutoff point where we use a generic error
// message instead of the exact # of files. If iMaxFiles is zero, we give the true
// count of files.
//
// NOTE: don't use this if you just want to check to see if the recycle bin is 
// empty or full!! Use IsRecycleBinEmpty() instead
//
int BBTotalCount(LPINT pidDrive, LPDWORD pdwSize, int iMaxFiles)
{
    int i;
    int idDrive;
    int nFiles = 0;
    DWORD dwSize;

    if (pdwSize)
        *pdwSize = 0;

    for (i = 0; i < MAX_BITBUCKETS; i++) 
    {
        int nFilesOld = nFiles;

        nFiles += CountDeletedFilesOnDrive(i, pdwSize ? &dwSize : NULL, iMaxFiles - nFilesOld);
     
        if (pdwSize)
            *pdwSize += dwSize;
        
        if (nFilesOld == 0 && nFiles == 1)
        {
            // if just one file, set the drive id
            idDrive = i;
        }

        if (iMaxFiles && (nFiles > iMaxFiles))
            break;
    }

    if (pidDrive)
        *pidDrive = (nFiles == 1) ? idDrive : 0;

    return nFiles;
}


//
// gets the number of files and and size of the bitbucket for the given drive
//
SHSTDAPI SHQueryRecycleBin(LPCTSTR pszRootPath, LPSHQUERYRBINFO pSHQueryInfo)
{
    DWORD dwSize = 0;
    DWORD dwNumItems = 0;
    int  idDrive;

    // since this fn is exported, we need to check to see if we need to 
    // init our global data first
    if (!InitBBGlobals())
    {
        return E_OUTOFMEMORY;
    }

    if (!pSHQueryInfo                                   ||
        (pSHQueryInfo->cbSize < sizeof(SHQUERYRBINFO))  ||
        !pszRootPath                                    ||
        (pszRootPath[0] == TEXT('\0')))
    {
        return E_INVALIDARG;
    }

    idDrive = DriveIDFromBBPath(pszRootPath);

    if (MakeBitBucket(idDrive))
    {
        dwNumItems = CountDeletedFilesOnDrive(idDrive, &dwSize, 0);
    }

    pSHQueryInfo->i64Size = (__int64)dwSize;
    pSHQueryInfo->i64NumItems = (__int64)dwNumItems;

    return S_OK;
}

#ifdef UNICODE
SHSTDAPI SHQueryRecycleBinA(LPCSTR pszRootPath, LPSHQUERYRBINFO pSHQueryRBInfo)
{
    WCHAR wszPath[MAX_PATH];

    SHAnsiToUnicode(pszRootPath, wszPath, ARRAYSIZE(wszPath));
    return SHQueryRecycleBin(wszPath, pSHQueryRBInfo);
}
#else
SHSTDAPI SHQueryRecycleBinW(LPCWSTR pwszRootPath, LPSHQUERYRBINFO pSHQueryRBInfo)
{
    CHAR szPath[MAX_PATH];

    SHUnicodeToAnsi(pwszRootPath, szPath, ARRAYSIZE(szPath));
    return SHQueryRecycleBin(szPath, pSHQueryRBInfo);
}
#endif


//
// Empty the given drive or all drives
//
SHSTDAPI SHEmptyRecycleBin(HWND hWnd, LPCTSTR pszRootPath, DWORD dwFlags)
{
    // since this fn is exported, we need to check to see if we need to 
    // init our global data first
    if (!InitBBGlobals())
    {
        // this could happen in low memory situations, we have no choice but
        // to abort the empty
        return E_OUTOFMEMORY;
    }

    if ((pszRootPath == NULL) || (*pszRootPath == TEXT('\0')))
    {
        BBPurgeAll(NULL, hWnd, dwFlags);
    }
    else
    {
        int idDrive;

        idDrive = DriveIDFromBBPath(pszRootPath);

        // note: we include MAX_DRIVES(26) which is SERVERDRIVE case!
        if ((idDrive < 0) || (idDrive > MAX_DRIVES))
        {
            return (E_INVALIDARG);
        }

        if (MakeBitBucket(idDrive))
        {
            PurgeOneBitBucket(hWnd, idDrive, dwFlags);
        }
    }

    return S_OK;
}


#ifdef UNICODE
SHSTDAPI SHEmptyRecycleBinA(HWND hWnd, LPCSTR pszRootPath, DWORD dwFlags)
{
    WCHAR wszPath[MAX_PATH];

    SHAnsiToUnicode(pszRootPath, wszPath, ARRAYSIZE(wszPath));
    return SHEmptyRecycleBin(hWnd, wszPath, dwFlags);
}
#else
SHSTDAPI SHEmptyRecycleBinW(HWND hWnd, LPCWSTR pwszRootPath, DWORD dwFlags)
{
    CHAR szPath[MAX_PATH];

    SHUnicodeToAnsi(pwszRootPath, szPath, ARRAYSIZE(szPath));
    return SHEmptyRecycleBin(hWnd, szPath, dwFlags);
}
#endif


void MarkBBPurgeAllTime(BOOL bStart)
{
    TCHAR szText[64];
    
    if (g_dwStopWatchMode == 0xffffffff)
        g_dwStopWatchMode = StopWatchMode();    // Since the stopwatch funcs live in shdocvw, delay this call so we don't load shdocvw until we need to

    if (g_dwStopWatchMode)
    {
        lstrcpy((LPTSTR)szText, TEXT("Shell Empty Recycle"));
        if (bStart)
        {
            lstrcat((LPTSTR)szText, TEXT(": Start"));
            StopWatch_Start(SWID_BITBUCKET, (LPCTSTR)szText, SPMODE_SHELL | SPMODE_DEBUGOUT);
        }
        else
        {
            lstrcat((LPTSTR)szText, TEXT(": Stop"));
            StopWatch_Stop(SWID_BITBUCKET, (LPCTSTR)szText, SPMODE_SHELL | SPMODE_DEBUGOUT);
        }
    }
}


void BBPurgeAll(CBitBucket *this, HWND hwndOwner, DWORD dwFlags)
{
    TCHAR szPath[MAX_PATH * 2 + 3]; // null space and double null termination
    int nFiles;
    int idDrive;
    BOOL fConfirmed;
    SHFILEOPSTRUCT sFileOp ={hwndOwner,
                             FO_DELETE,
                             szPath,
                             NULL,
                             FOF_NOCONFIRMATION | FOF_SIMPLEPROGRESS,
                             FALSE,
                             NULL,
                             MAKEINTRESOURCE(IDS_BB_EMPTYINGWASTEBASKET)};

    // check to see if we need to init our global data first
    if (!InitBBGlobals())
    {
        // this could happen in low memory situations, we have no choice but
        // to fail the empty
        return;
    }

    if (g_dwStopWatchMode)   // If the shell perf mode is enabled, time the empty operation
    {
        MarkBBPurgeAllTime(TRUE);
    }

    fConfirmed = (dwFlags & SHERB_NOCONFIRMATION);

    if (!fConfirmed) 
    {
        // find out how many files we have...
        BBDATAENTRYW bbdew;
        TCHAR szSrcName[MAX_PATH];

        WIN32_FIND_DATA fd;
        LPTSTR pszSrc;
        CONFIRM_DATA cd = {CONFIRM_DELETE_FILE | CONFIRM_DELETE_FOLDER | CONFIRM_PROGRAM_FILE | CONFIRM_MULTIPLE, 0};

        nFiles = BBTotalCount(&idDrive, NULL, MAX_EMPTY_FILES);

        if (!nFiles)
        {
            if (g_dwStopWatchMode)
            {
                MarkBBPurgeAllTime(FALSE);
            }
            return;   // no files to delete
        }

        // first do the confirmation thing
        fd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

        // We have to call IsBitBucketInited() here since we could be in BBPurgeAll as a result
        // of a corrupt bitbucket. In this case, the g_pBitBucket[idDrive] has not been inited and
        // therefore we can't use it yet
        if (nFiles == 1 && IsBitBucketInited(idDrive)) 
        {
            HANDLE hFile;
            BOOL fSuccess;

#ifdef BB_USE_MRSW
            MRSW_EnterRead(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

            hFile = OpenBBInfoFile(idDrive, OPENBBINFO_READ, 0);
            fSuccess = (hFile != INVALID_HANDLE_VALUE) && ReadNextDataEntry(hFile, &bbdew, TRUE, g_pBitBucket[idDrive]->fIsUnicode ? SIZEOF(BBDATAENTRYW) : SIZEOF(BBDATAENTRYA), idDrive);

            if (hFile)
            {
                CloseBBInfoFile(hFile, idDrive);
            }

#ifdef BB_USE_MRSW
            MRSW_LeaveRead(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

            if (!fSuccess)
            {
                if (g_dwStopWatchMode)
                {
                    MarkBBPurgeAllTime(FALSE);
                }
                return; // no files to delete
            }

            if (g_pBitBucket[idDrive]->fIsUnicode)
            {
#ifdef UNICODE
                // this is a unicode drive, so use the unicode original name
                pszSrc = bbdew.szOriginal;
#else
                WideCharToMultiByte(CP_ACP, 0,
                                    bbdew.szOriginal, -1,
                                    szSrcName, ARRAYSIZE(szSrcName),
                                    NULL, NULL);
                pszSrc = szSrcName;
#endif
            }
            else
            {
                LPBBDATAENTRYA pbbdea = (LPBBDATAENTRYA)&bbdew;
#ifdef UNICODE
                MultiByteToWideChar(CP_ACP, 0,
                                    pbbdea->szOriginal, -1,
                                    szSrcName, ARRAYSIZE(szSrcName));
                pszSrc = szSrcName;
#else
                pszSrc = pbbdea->szOriginal;
#endif
            }
        }
        else
        {
            // If we haven't inited this bucket yet or there are greater than MAX_EMPTY_FILES,
            // then use the generic empty message
            if (nFiles == 1 || nFiles > MAX_EMPTY_FILES)
            {
                // counting up the total # of files in the bitbucket scales as
                // the # of files (duh!). This can get pretty expensive, so if there
                // are more than MAX_EMPTY_FILES in the bin, we just give a generic
                // error message
                
                // set this so ConfirmFileOp knows to use the generic message
                nFiles = -1;
            }
            
            pszSrc = (LPTSTR)c_szNULL;
        }
        
        if (ConfirmFileOp(hwndOwner, NULL, &cd, nFiles, 0, CONFIRM_DELETE_FILE | CONFIRM_WASTEBASKET_PURGE, pszSrc, &fd, NULL, &fd, NULL) == IDYES)
        {
            fConfirmed = TRUE;
        }
    }

    if (fConfirmed)
    {
        DECLAREWAITCURSOR;
        SetWaitCursor();
        
        if (dwFlags & SHERB_NOPROGRESSUI)
        {
            sFileOp.fFlags |= FOF_SILENT;
        }

        for (idDrive = 0; (idDrive < MAX_BITBUCKETS) && !sFileOp.fAnyOperationsAborted; idDrive++)
        {
            if (MakeBitBucket(idDrive))
            {
                HANDLE hFile;
                
                // nuke all the BB files (d*.*)
                DriveIDToBBPath(idDrive, szPath);
                PathAppend(szPath, c_szDStarDotStar);
                szPath[lstrlen(szPath) + 1] = TEXT('\0'); // double null terminate

                // turn off redraw for now.
                ShellFolderView_SetRedraw(hwndOwner, FALSE);

                // since we are nuking this bucket, block for write
#ifdef BB_USE_MRSW
                MRSW_EnterWrite(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

                hFile = OpenBBInfoFile(idDrive, OPENBBINFO_WRITE, 0);

                if (INVALID_HANDLE_VALUE != hFile)
                {
                    // now do the actual delete.
                    if (SHFileOperation(&sFileOp) || sFileOp.fAnyOperationsAborted) 
                    {
                        TraceMsg(TF_BITBUCKET, "Bitbucket: emptying bucket on %s failed or user aborted", szPath);

                        // NOTE: the info file may point to some files that have been deleted,
                        // it will be cleaned up later
                    }
                    else
                    {
                        // reset the info file since we deleted in while emptying
                        ResetInfoFileHeader(hFile, g_pBitBucket[idDrive]->fIsUnicode);
                    }

                    // we always reset the desktop.ini
                    CreateRecyclerDirectory(idDrive);

                    CloseBBInfoFile(hFile, idDrive);
                }

                // unblock for write
#ifdef BB_USE_MRSW
                MRSW_LeaveWrite(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

                ShellFolderView_SetRedraw(hwndOwner, TRUE);
            }
        }

        if (!(dwFlags & SHERB_NOSOUND))
        {
            SHPlaySound(TEXT("EmptyRecycleBin"));
        }

        SHUpdateRecycleBinIcon();
        ResetWaitCursor();
    }
    
    if (g_dwStopWatchMode)
    {
        MarkBBPurgeAllTime(FALSE);
    }
}


BOOL BBNukeFile(LPCTSTR pszPath, DWORD dwAttribs)
{
    if (Win32DeleteFile(pszPath))
    {
        FOUndo_FileReallyDeleted((LPTSTR)pszPath);
        return TRUE;
    }

    return FALSE;
}


BOOL BBNukeFolder(LPCTSTR pszDir)
{
    TCHAR szPath[MAX_PATH];
    BOOL fRet;

    if (PathCombine(szPath, pszDir, c_szStarDotStar))
    {
        WIN32_FIND_DATA fd;
        HANDLE hfind = FindFirstFile(szPath, &fd);
        if (hfind != INVALID_HANDLE_VALUE)
        {
            do
            {
                LPTSTR pszFile = fd.cAlternateFileName[0] ? fd.cAlternateFileName : fd.cFileName;

                if (pszFile[0] != TEXT('.'))
                {
                    // use the short path name so that we
                    // don't have to worry about max path
                    PathCombine(szPath, pszDir, pszFile);

                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        // even if this fails, we keep going.
                        // we want to delete as much as possible
                        BBNukeFolder(szPath);
                    }
                    else
                    {
                        BBNukeFile(szPath, fd.dwFileAttributes);
                    }
                }
            } while (FindNextFile(hfind, &fd));

            FindClose(hfind);
        }
    }
    
    fRet = Win32RemoveDirectory(pszDir);
    
    // if everything was successful, we need to notify any undo stuff about this
    if (fRet)
    {
        FOUndo_FileReallyDeleted((LPTSTR)szPath);
    }

    return fRet;
}


BOOL BBNuke(LPCTSTR pszPath)
{
    BOOL fRet = FALSE;
    // verify that the file exists
    DWORD dwAttribs = GetFileAttributes(pszPath);

    TraceMsg(TF_BITBUCKET, "Bitbucket: BBNuke called on %s ", pszPath);
    
    if (dwAttribs != (UINT)-1)
    {
        // this was a directory, we need to recurse in and delete everything inside
        if (dwAttribs & FILE_ATTRIBUTE_DIRECTORY)
        {
            fRet = BBNukeFolder(pszPath);
        }
        else
        {
            fRet = BBNukeFile(pszPath, dwAttribs);
        }
    }

    return fRet;
}


DWORD PurgeBBFiles(int idDrive)
{
    DWORD dwCurrentSize;

    CountDeletedFilesOnDrive(idDrive, &dwCurrentSize, 0);

    if (dwCurrentSize > g_pBitBucket[idDrive]->cbMaxSize)
    {
        TCHAR szBBPath[MAX_PATH];
        BBDATAENTRYW bbdew;
        HANDLE hFile;
        DWORD dwDataEntrySize = g_pBitBucket[idDrive]->fIsUnicode ? SIZEOF(BBDATAENTRYW) : SIZEOF(BBDATAENTRYA);

        DriveIDToBBPath(idDrive, szBBPath);

#ifdef BB_USE_MRSW
        MRSW_EnterRead(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

        hFile = OpenBBInfoFile(idDrive, OPENBBINFO_WRITE, 0);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            // while we're too big, find something to delete
            while (dwCurrentSize > g_pBitBucket[idDrive]->cbMaxSize && ReadNextDataEntry(hFile, &bbdew, TRUE, dwDataEntrySize, idDrive))
            {
                TCHAR szOriginalFileName[MAX_PATH];
                TCHAR szPath[MAX_PATH];
                TCHAR szDeletedFile[MAX_PATH];

                // BUGBUG (reinerf) - need a warning that stuff is going to be deleted when the
                //                    user changes the % size slider and stuff gets nuked.
                
                if (g_pBitBucket[idDrive]->fIsUnicode)
                {
                    SHUnicodeToTChar(bbdew.szOriginal, szOriginalFileName, ARRAYSIZE(szOriginalFileName));
                }
                else
                {
                    LPBBDATAENTRYA pbbdea = (LPBBDATAENTRYA)&bbdew;

                    SHAnsiToTChar(pbbdea->szOriginal, szOriginalFileName, ARRAYSIZE(szOriginalFileName));
                }

                GetDeletedFileName(szDeletedFile, idDrive, bbdew.iIndex, szOriginalFileName);
                PathCombine(szPath, szBBPath, szDeletedFile);
                BBNuke(szPath);
                NukeFileInfoBeforePoint(hFile, &bbdew, dwDataEntrySize);

                // subtract the size of what we just nuked
                dwCurrentSize -= bbdew.dwSize;

                TraceMsg(TF_BITBUCKET, "Bitbucket: purging drive %d, curent size = %d, max size = %d", idDrive, dwCurrentSize, g_pBitBucket[idDrive]->cbMaxSize);
            }

            CloseBBInfoFile(hFile, idDrive);
        }
        
#ifdef BB_USE_MRSW
        MRSW_LeaveRead(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW
    }

    return dwCurrentSize;
}


// converts C:\RECYCLED\Dc19.foo to 19
int BBPathToIndex(LPCTSTR pszPath)
{
    TCHAR szFullPath[MAX_PATH];
    LPTSTR pszFileName;

    lstrcpyn(szFullPath, pszPath, SIZECHARS(szFullPath));

    pszFileName = PathFindFileName(szFullPath);

    if (!lstrcmpi(pszFileName, c_szInfo) ||
        !lstrcmpi(pszFileName, c_szInfo2) ||
        !lstrcmpi(pszFileName, c_szDesktopIni) ||
        !lstrcmpi(pszFileName, c_szRecycled))
    {
        return -1;
    }

    PathRemoveExtension(pszFileName);

    if (((pszFileName[0] == TEXT('D')) || (pszFileName[0] == TEXT('d'))) &&
        ISDIGIT(pszFileName[2]))
    {
        return StrToInt(&pszFileName[2]);
    }
    else
    {
        return -1;
    }
}


BOOL ReadNextDataEntry(HANDLE hFile, LPBBDATAENTRYW pbbdew, BOOL fSkipDeleted, DWORD dwDataEntrySize, int idDrive)
{
    TCHAR szOriginalFileName[MAX_PATH];
    TCHAR szDeleteFileName[MAX_PATH];
    TCHAR szOldPath[MAX_PATH];

    DWORD dwBytesRead;

TryAgain:
    if (ReadFile(hFile, pbbdew, dwDataEntrySize, &dwBytesRead, NULL) && dwBytesRead == dwDataEntrySize)
    {
        if (fSkipDeleted && IsDeletedEntry(pbbdew))
        {
            goto TryAgain;
        }

        // calculate the original file name
        if (dwDataEntrySize == SIZEOF(BBDATAENTRYW))
        {
            ASSERT(g_pBitBucket[idDrive]->fIsUnicode);
            SHUnicodeToTChar(pbbdew->szOriginal, szOriginalFileName, ARRAYSIZE(szOriginalFileName));
        }
        else
        {
            LPBBDATAENTRYA pbbdea = (LPBBDATAENTRYA)pbbdew;

            ASSERT(!g_pBitBucket[idDrive]->fIsUnicode);
            SHAnsiToTChar(pbbdea->szOriginal, szOriginalFileName, ARRAYSIZE(szOriginalFileName));
        }

        // We check for a drive that has had its letter changed since this record was added.
        // In this case, we want to restore the files that were deleted on this volume to this volume.
        if (pbbdew->idDrive != idDrive)
        {
            TCHAR szNewPath[MAX_PATH];

            DriveIDToBBPath(idDrive, szOldPath);
            lstrcpy(szNewPath, szOldPath);

            GetDeletedFileName(szDeleteFileName, pbbdew->idDrive, pbbdew->iIndex, szOriginalFileName);
            PathAppend(szOldPath, szDeleteFileName);

            GetDeletedFileName(szDeleteFileName, idDrive, pbbdew->iIndex, szOriginalFileName);
            PathAppend(szNewPath, szDeleteFileName);

            TraceMsg(TF_BITBUCKET, "Bitbucket: found entry %s corospoinding to old drive letter, whacking it to be on drive %d !!", szOldPath, idDrive);

            // we need to rename the file from d?0.txt to d<idDrive>0.txt
            if (!Win32MoveFile(szOldPath, szNewPath, GetFileAttributes(szOldPath) & FILE_ATTRIBUTE_DIRECTORY))
            {
                ASSERTMSG(FALSE, "Bitbucket: failed to rename %s to %s, getlasterror = %d", szOldPath, szNewPath, GetLastError());
                goto DeleteEntry;
            }

            // whack the rest of the information about this entry to match the new drive ID
            pbbdew->idDrive = idDrive;
            pbbdew->szShortName[0] = 'A' + (CHAR)idDrive;
            if (g_pBitBucket[idDrive]->fIsUnicode)
            {
                // for unicode volumes we need to whack the first letter of the long name as well
                pbbdew->szOriginal[0] = L'A' + (WCHAR)idDrive;
            }
        }
        else
        {
            //
            // Starting with NT5, when we delete or restore items, we dont bother updating the info file.
            // So we need to make sure that the entry we have has not been restored or really nuked.
            //
            DriveIDToBBPath(idDrive, szOldPath);
            GetDeletedFileName(szDeleteFileName, pbbdew->idDrive, pbbdew->iIndex, szOriginalFileName);
            PathAppend(szOldPath, szDeleteFileName);

            if (!PathFileExists(szOldPath))
            {
DeleteEntry:
                // this entry is really deleted, so mark it as such now
                NukeFileInfoBeforePoint(hFile, pbbdew, dwDataEntrySize);
        
                if (fSkipDeleted)
                {
                    goto TryAgain;
                }
            }
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//
// the file pointer is RIGHT AFTER the entry that we want to delete.
//
// back the file pointer up one record and mark it deleted
//
void NukeFileInfoBeforePoint(HANDLE hFile, LPBBDATAENTRYW pbbdew, DWORD dwDataEntrySize)
{
    DWORD dwBytesWritten;
    LONG lPos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

    ASSERT((DWORD)lPos >= dwDataEntrySize + SIZEOF(BBDATAHEADER));
    
    if ((DWORD)lPos >= dwDataEntrySize + SIZEOF(BBDATAHEADER))
    {
        // found the entry.. back up the file pointer to the beginning
        // of this record and mark it as deleted
        lPos -= dwDataEntrySize;
        SetFilePointer(hFile, lPos, NULL, FILE_BEGIN);
        
        MarkEntryDeleted(pbbdew);
        
        if (WriteFile(hFile, (LPBYTE)pbbdew, dwDataEntrySize, &dwBytesWritten, NULL))
        {
            ASSERT(dwDataEntrySize == dwBytesWritten);
        }
        else
        {
            TraceMsg(TF_BITBUCKET, "Bitbucket: couldn't nuke file info");
            // move the file pointer back to where it was when we entered this function
            SetFilePointer(hFile, lPos + dwDataEntrySize, NULL, FILE_BEGIN);
        }
    }
}


//
// This closes the hFile and sends out an SHCNE_UPDATEITEM for the info file on
// drive idDrive
//
void CloseBBInfoFile(HANDLE hFile, int idDrive)
{
    TCHAR szInfoFile[MAX_PATH];

    ASSERT(hFile != INVALID_HANDLE_VALUE);

    DriveIDToBBPath(idDrive, szInfoFile);
    PathAppend(szInfoFile, c_szInfo2);

    CloseHandle(hFile);

    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, szInfoFile, NULL);
}

// One half second (500 ms = 0.5 s)
#define BBINFO_OPEN_RETRY_PERIOD        500
// Retry 30 times (at least 20 s)
#define BBINFO_OPEN_MAX_RETRIES         40

//
// This opens up a handle to the bitbucket info file.
// 
// NOTE: use CloseBBInfoFile so that we generate the proper 
//       SHChangeNotify event for the info file.
//
HANDLE OpenBBInfoFile(int idDrive, DWORD dwFlags, int iRetryCount)
{
    HANDLE hFile;
    TCHAR szBBPath[MAX_PATH];
    TCHAR szInfo[MAX_PATH];
    int nAttempts = 0;
    DWORD dwLastErr;
    DECLAREWAITCURSOR;

    if ((iRetryCount == 0) || (iRetryCount > BBINFO_OPEN_MAX_RETRIES))
    {
        // zero retry count means that the caller wants the max # of retries
        iRetryCount = BBINFO_OPEN_MAX_RETRIES;
    }

    DriveIDToBBPath(idDrive, szBBPath);
    GetBBInfo2FileSpec(szBBPath, szInfo);
    // If we are hitting a sharing violation, retry many times
    do
    {
        nAttempts++;
        hFile = CreateFile(szInfo,
                           GENERIC_READ | ((OPENBBINFO_WRITE & dwFlags) ? GENERIC_WRITE : 0),
                           (OPENBBINFO_WRITE & dwFlags) ? 0 : FILE_SHARE_READ,
                           NULL,
                           (OPENBBINFO_CREATE & dwFlags) ? OPEN_ALWAYS : OPEN_EXISTING,
                           FILE_ATTRIBUTE_HIDDEN | FILE_FLAG_RANDOM_ACCESS,
                           NULL);
        if (INVALID_HANDLE_VALUE != hFile)
        {
            // success!
            break;
        }

        dwLastErr = GetLastError();
        if (ERROR_SHARING_VIOLATION != dwLastErr)
        {
            break;
        }

        TraceMsg(TF_BITBUCKET, "Bitbucket: sharing violation on info file (retry %d)", nAttempts - 1);

        if (nAttempts < iRetryCount)
        {
            SetWaitCursor();
            Sleep(BBINFO_OPEN_RETRY_PERIOD);
            ResetWaitCursor();
        }

    } while (nAttempts < iRetryCount);
    
    if (hFile == INVALID_HANDLE_VALUE)
    {
        TraceMsg(TF_BITBUCKET, "Bitbucket: could not open a handle to %s - error 0x%08x!!", szInfo, dwLastErr);
        return INVALID_HANDLE_VALUE;
    }

    // set the file pointer to just after the dataheader
    SetFilePointer(hFile, SIZEOF(BBDATAHEADER), NULL, FILE_BEGIN);

    return hFile;
}


void BBAddDeletedFileInfo(LPTSTR pszOriginal, LPCTSTR pszShortName, int iIndex, int idDrive, DWORD dwSize)
{
    BBDATAENTRYW bbdew;
    DWORD dwBytesWritten;
    SYSTEMTIME st;
    HANDLE hFile;
    BOOL fSuccess = FALSE; // assume failure

    if (g_pBitBucket[idDrive]->fIsUnicode)
    {
#ifdef UNICODE
        // Create a BBDATAENTRYW from a unicode name
        lstrcpy(bbdew.szOriginal, pszOriginal);
        
        if (!DoesStringRoundTrip(pszOriginal, bbdew.szShortName, ARRAYSIZE(bbdew.szShortName)))
        {
            SHUnicodeToAnsi(pszShortName, bbdew.szShortName, ARRAYSIZE(bbdew.szShortName));
        }
#else
        // Create a BBDATAENTRYW from an ansi name
        lstrcpy(bbdew.szShortName, pszOriginal);

        SHAnsiToUnicode(pszOriginal, bbdew.szOriginal, ARRAYSIZE(bbdew.szOriginal));
#endif
    }
    else
    {
        LPBBDATAENTRYA pbbdea = (LPBBDATAENTRYA)&bbdew;

#ifdef UNICODE
        // Create a BBDATAENTRYA from a unicode name
        if (!DoesStringRoundTrip(pszOriginal, pbbdea->szOriginal, ARRAYSIZE(pbbdea->szOriginal)))
        {
            SHUnicodeToAnsi(pszShortName, pbbdea->szOriginal, ARRAYSIZE(pbbdea->szOriginal));
        }
#else
        // Create a BBDATAENTRYA from an ansi name
        lstrcpy(pbbdea->szOriginal, pszOriginal);
#endif
    }

    bbdew.iIndex = iIndex;
    bbdew.idDrive = idDrive;

    // round up to nearest cluster
    bbdew.dwSize = ROUND_TO_CLUSTER(dwSize, g_pBitBucket[idDrive]->dwClusterSize);

    GetSystemTime(&st);             // Get time of deletion
    SystemTimeToFileTime(&st, &bbdew.ft);

    // we are adding an new record to the end, so even though we
    // are going to write to the file, since it is at the end we shouldnt
    // need to block anyone. therefore, we only enter for read here.
#ifdef BB_USE_MRSW
    MRSW_EnterRead(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

    // now write it to the file
    hFile = OpenBBInfoFile(idDrive, OPENBBINFO_WRITE, 0);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwDataEntrySize = g_pBitBucket[idDrive]->fIsUnicode ? SIZEOF(BBDATAENTRYW) : SIZEOF(BBDATAENTRYA);


        // BUGBUG (reinerf) - two threads could both come through this code and get the same seek position and 
        // thus the second would not be appending to the file, he would be deleteing the first's changes.
        SetFilePointer(hFile, 0, NULL, FILE_END);

        if (WriteFile(hFile, (LPSTR)&bbdew, dwDataEntrySize, &dwBytesWritten, NULL) && dwDataEntrySize == dwBytesWritten)
        {
            fSuccess = TRUE;
        }

        CloseBBInfoFile(hFile, idDrive);
    }

#ifdef BB_USE_MRSW
    MRSW_LeaveRead(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

    if (!fSuccess)
    {
        TCHAR szBBPath[MAX_PATH];
        TCHAR szFileName[MAX_PATH];
        
        ASSERTMSG(FALSE, "BitBucket: failed to record deleted file %s , have to nuke it!!", bbdew.szOriginal);

        // get the recycled dir and tack on the file name
        DriveIDToBBPath(idDrive, szBBPath);
        GetDeletedFileName(szFileName, idDrive, iIndex, pszOriginal);
        PathAppend(szBBPath, szFileName);

        // now delete it
        BBNuke(szBBPath);
    }
    else
    {
        BOOL bPurge = TRUE;
        
        // since we sucessfully deleted a file, we set this so at the end of the last SHFileOperation call on this drive
        // we will go back and make sure that there isint too much stuff in the bucket.
        RegSetValueEx(g_pBitBucket[idDrive]->hkeyPerUser, TEXT("NeedToPurge"), 0, REG_DWORD, (LPBYTE)&bPurge, SIZEOF(bPurge));
    }
}

#ifdef WINNT
//
// creates the proper SECURITY_DESCRIPTOR for securing the recycle bin
//
// NOTE: if return value is non-null, the caller must LocalFree it
//
SECURITY_DESCRIPTOR* CreateRecycleBinSecurityDescriptor()
{
    SHELL_USER_PERMISSION supLocalUser;
    SHELL_USER_PERMISSION supSystem;
    SHELL_USER_PERMISSION supAdministrators;
    PSHELL_USER_PERMISSION aPerms[3] = {&supLocalUser, &supSystem, &supAdministrators};

    // we want the current user to have full control
    supLocalUser.susID = susCurrentUser;
    supLocalUser.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supLocalUser.dwAccessMask = FILE_ALL_ACCESS;
    supLocalUser.fInherit = TRUE;
    supLocalUser.dwInheritMask = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    supLocalUser.dwInheritAccessMask = GENERIC_ALL;

    // we want the SYSTEM to have full control
    supSystem.susID = susSystem;
    supSystem.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supSystem.dwAccessMask = FILE_ALL_ACCESS;
    supSystem.fInherit = TRUE;
    supSystem.dwInheritMask = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    supSystem.dwInheritAccessMask = GENERIC_ALL;

    // we want the Administrators to have full control
    supAdministrators.susID = susAdministrators;
    supAdministrators.dwAccessType = ACCESS_ALLOWED_ACE_TYPE;
    supAdministrators.dwAccessMask = FILE_ALL_ACCESS;
    supAdministrators.fInherit = TRUE;
    supAdministrators.dwInheritMask = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
    supAdministrators.dwInheritAccessMask = GENERIC_ALL;

    return GetShellSecurityDescriptor(aPerms, ARRAYSIZE(aPerms));
}


//
// Creates the secure recycle bin directory (eg one with ACL's that protect it)
// for recycle bins on NTFS volumes.
//
BOOL CreateSecureRecyclerDirectory(LPCTSTR pszPath)
{
    BOOL fSuccess = FALSE;      // assume failure
    SECURITY_DESCRIPTOR* psd = CreateRecycleBinSecurityDescriptor();

    if (psd)
    {
        DWORD cbSA = GetSecurityDescriptorLength(psd);
        SECURITY_DESCRIPTOR* psdSelfRelative;

        psdSelfRelative = LocalAlloc(LPTR, cbSA);

        if (psdSelfRelative)
        {
            if (MakeSelfRelativeSD(psd, psdSelfRelative, &cbSA))
            {
                SECURITY_ATTRIBUTES sa;

                // Build the security attributes structure
                sa.nLength = SIZEOF(SECURITY_ATTRIBUTES);
                sa.lpSecurityDescriptor = psdSelfRelative;
                sa.bInheritHandle = FALSE;

                fSuccess = (SHCreateDirectoryEx(NULL, pszPath, &sa) == ERROR_SUCCESS);
            }

            LocalFree(psdSelfRelative);
        }

        LocalFree(psd);
    }

    return fSuccess;
}
#endif // WINNT


BOOL CreateRecyclerDirectory(int idDrive)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szRoot[MAX_PATH];
    BOOL    bResult = FALSE;
    BOOL    bExists;

    // BUGBUG (reinerf) - we neeed to check for FAT/FAT32 drives that
    // have been upgraded to NTFS and migrate the recycle bin info over
    
    DriveIDToBBPath(idDrive, szPath);
    DriveIDToBBRoot(idDrive, szRoot);

    bExists = PathIsDirectory(szPath);

    if (!bExists)
    {
#ifdef WINNT
        if (CMtPt_IsSecure(idDrive)) // BUGBUG (reinerf) - really need a PathIsSecure(szPath) check instead, that works on UNC's too.
        {
            bExists = CreateSecureRecyclerDirectory(szPath);
        }
        else
#endif // WINNT
        {
            bExists = (SHCreateDirectoryEx(NULL, szPath, NULL) == ERROR_SUCCESS);
        }
    }

    if (bExists) 
    {
        PathAppend(szPath, c_szDesktopIni);
        // CLSID_RecycleBin
        WritePrivateProfileString(c_szClassInfo, c_szCLSID, TEXT("{645FF040-5081-101B-9F08-00AA002F954E}"), szPath);
        SetFileAttributes(szPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);   // desktop.ini

        PathRemoveFileSpec(szPath);
        // Hide all of the directories along the way until we hit the BB root
        do
        {
            SetFileAttributes(szPath, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);

            PathRemoveFileSpec(szPath);
        } while (0 != lstrcmpi(szPath, szRoot));

        // everything's set.  let's add it in
        // try to load the and initalize 
        bResult = TRUE;
    }

    return bResult;
}


//
// this sets up the bitbucket directory and allocs the internal structures
//
BOOL MakeBitBucket(int idDrive)
{
    BOOL bRet = FALSE;

    if ((idDrive >= 0)  &&  // dont support recycling for the general UNC case
        IsBitBucketableDrive(idDrive))
    {
        if (IsBitBucketInited(idDrive))
        {
            LONG lBucketDirtyCount = SHGlobalCounterGetValue(g_pBitBucket[idDrive]->hgcDirtyCount);
            LONG lGlobalDirtyCount = SHGlobalCounterGetValue(g_hgcGlobalDirtyCount);

            // check to see if we need to refresh the settings for this bucket
            if (lGlobalDirtyCount > g_lProcessDirtyCount)
            {
		// global settings change, so refresh all buckets
                g_lProcessDirtyCount = lGlobalDirtyCount;
                RefreshAllBBDriveSettings();
            }
            else if (lBucketDirtyCount > g_pBitBucket[idDrive]->lCurrentDirtyCount)
            {
		// just this buckets settings changed, so refresh only this one
                g_pBitBucket[idDrive]->lCurrentDirtyCount = lBucketDirtyCount;
                RefreshBBDriveSettings(idDrive);
            }
            
            bRet = TRUE;
        }
        else
        {
            bRet = AllocBBDriveInfo(idDrive);
        }
    }

    return bRet;
}


// Tells if a file will *likely* be recycled...
// this could be wrong if:
//
//      * disk is full
//      * file is really a folder
//      * file greater than the allocated size for the recycle directory
//      * file is in use or no ACLS to move or delete it
//
BOOL BBWillRecycle(LPCTSTR pszFile, INT* piRet)
{
    INT iRet = BBDELETE_SUCCESS;
    int idDrive = DriveIDFromBBPath(pszFile);

    // MakeBitBucket will ensure that the global & per-bucket settings we have are current
    if (!MakeBitBucket(idDrive) || g_pBitBucket[idDrive]->fNukeOnDelete || (g_pBitBucket[idDrive]->iPercent == 0))
    {
        iRet = BBDELETE_FORCE_NUKE;
    }
    else if (SERVERDRIVE == idDrive)
    {
        // Check to see if the serverdrive is offline (don't recycle while offline to prevent
        // synchronization conflicts when coming back online):
        TCHAR szVolume[MAX_PATH];
        LONG lStatus;
        DriveIDToBBVolumeRoot(idDrive, szVolume);

        lStatus = GetOfflineShareStatus(szVolume);
        if ((CSC_SHARESTATUS_OFFLINE == lStatus) || (CSC_SHARESTATUS_SERVERBACK == lStatus))
        {
            iRet = BBDELETE_NUKE_OFFLINE;
        }
    }

    if (piRet)
    {
        *piRet = iRet;
    }
    return (BBDELETE_SUCCESS == iRet);
}


//
// This is called at the end of the last pending SHFileOperation that involves deletes.
// We wait till there arent any more people deleteing before we go try to compact the info
// file or purge entries and make the bitbucket respect its cbMaxSize.
//
void CheckCompactAndPurge()
{
    int i;
    TCHAR szBBKey[MAX_PATH];
    HKEY hkBBPerUser;

    for (i = 0; i < MAX_BITBUCKETS ; i++)
    {
        DriveIDToBBRegKey(i, szBBKey);
        
        // NOTE: these function needs to manually construct the key because it would like to avoid calling MakeBitBucket()
        // for drives that it has yet to look at (this is a performance optimization)
        if (RegOpenKeyEx(g_hkBitBucketPerUser, szBBKey, 0, KEY_QUERY_VALUE |  KEY_SET_VALUE, &hkBBPerUser) == ERROR_SUCCESS)
        {
            BOOL bCompact = FALSE;
            BOOL bPurge = FALSE;
            DWORD dwSize;

            dwSize = SIZEOF(bCompact);
            if (RegQueryValueEx(hkBBPerUser, TEXT("NeedToCompact"), NULL, NULL, (LPBYTE)&bCompact, &dwSize) == ERROR_SUCCESS && bCompact == TRUE)
            {
                // reset this key so no one else tries to compact this bitbucket
                RegDeleteValue(hkBBPerUser, TEXT("NeedToCompact"));
            }

            dwSize = SIZEOF(bPurge);
            if (RegQueryValueEx(hkBBPerUser, TEXT("NeedToPurge"), NULL, NULL, (LPBYTE)&bPurge, &dwSize) == ERROR_SUCCESS && bPurge == TRUE)
            {
                // reset this key so no one else tries to purge this bitbucket
                RegDeleteValue(hkBBPerUser, TEXT("NeedToPurge"));
            }
  
            if (MakeBitBucket(i))
            {
                if (bCompact)
                {
                    TraceMsg(TF_BITBUCKET, "Bitbucket: compacting drive %d",i);
                    CompactBBInfoFile(i);
                }

                if (bPurge)
                {
                    TraceMsg(TF_BITBUCKET, "Bitbucket: purging drive %d", i);
                    PurgeBBFiles(i);
                }
            }

            RegCloseKey(hkBBPerUser);
        }
    }

    SHUpdateRecycleBinIcon();
    SHChangeNotify(0, SHCNF_FLUSH | SHCNF_FLUSHNOWAIT, NULL, NULL);
}


//
// BBDeleteFile
//
// return:
//
//      TRUE    - The file/folder was sucessfully moved to the recycle bin. We set lpiReturn = BBDELETE_SUCCESS for this case.
//
//      FALSE   - The file/folder could not be moved to the recycle bin
//                In this case, the piRet value tells WHY the file/folder could not be recycled:
//                
//                BBDELETE_FORCE_NUKE       - User has "delete file immeadately" set, or % max size=0, or we failed to 
//                                            create the recycler directory.
//
//                BBDELETE_CANNOT_DELETE    - The file/folder is non-deletable because a file under it cannot be deleted.
//                                            This is an NT only case, and it could be caused by acls or the fact that the
//                                            folder or a file in it is currently in use.
//
//                BBDELETE_SIZE_TOO_BIG     - The file/folder is bigger than the max allowable size of the recycle bin.
//
//                BBDELETE_PATH_TOO_LONG    - The path would be too long ( > MAX_PATH), if the file were to be moved to the
//                                            the recycle bin directory at the root of the drive
//
//                BBDELETE_UNKNOWN_ERROR    - Some other error occured, GetLastError() should explain why we failed.
//                            
//
BOOL BBDeleteFile(LPTSTR pszFile, INT* piRet, LPUNDOATOM lpua, BOOL fIsDir, WIN32_FIND_DATA *pfd)
{
    int iRet;
    TCHAR szBitBucket[MAX_PATH];
    TCHAR szFileName[MAX_PATH];
    TCHAR szShortFileName[MAX_PATH];
    ULARGE_INTEGER ulSize;
    DWORD dwLastError;
    int iIndex;
    int idDrive = DriveIDFromBBPath(pszFile);

    // check to see if we need to init our global data first
    if (!InitBBGlobals())
    {
        // this could happen in low memory situations, we have no choice but
        // to really nuke the file
        *piRet = BBDELETE_FORCE_NUKE;
        return FALSE;
    }

    if (!BBWillRecycle(pszFile, piRet))
    {
        // We failed to create the recycler directory on the volume, or
        // this is the case where the user has "delete files immeadately" set, or 
        // % max size=0, etc.
        return FALSE;
    }

    if (fIsDir) 
    {
        DWORD dwError;
        FOLDERDELETEINFO fdi = {0};

        fdi.cchBBDir = g_pBitBucket[idDrive]->cchBBDir;

        dwError = CheckFolderSizeAndDeleteability(pszFile, &fdi);
        
        if (dwError != ERROR_SUCCESS) 
        {
            // if CheckFolderSizeAndDeleteability can fail if a file cant be recycled.
            // In this case, it appends the name of the file to pszFile, so we know who 
            // the undeletable file is. 
            if ((dwError == ERROR_FILENAME_EXCED_RANGE) ||
                (dwError == ERROR_BUFFER_OVERFLOW))
            {
                // it failed because a new path would be to long after being moveed under the "C:\recycler\sid" directory
                *piRet = BBDELETE_PATH_TOO_LONG;
            }
            else
            {
                // must be a non-deletable directory, so set piRet = BBDELETE_CANNOT_DELETE so our caller
                // can detect this case, also pass the name of the non-deletable file back out so we can give
                // a better error message to the user
                *piRet = BBDELETE_CANNOT_DELETE;
                ASSERT(*fdi.szPath);
                lstrcpy(pszFile, fdi.szNonDeletableFile);
            }

            return FALSE;
        }

        ulSize.QuadPart = fdi.cbSize;
    }
    else 
    {
        if (!IsFileDeletable(pszFile))
        {
            // We set piRet = BBDELETE_CANNOT_DELETE so our caller can detect
            // that this file cant be recycled.
            *piRet = BBDELETE_CANNOT_DELETE;
            return FALSE;
        }

        ulSize.LowPart  = pfd->nFileSizeLow;
        ulSize.HighPart = pfd->nFileSizeHigh;
    }

    // check to make sure it's not bigger than the allowed wastebasket..
    if (ulSize.HighPart || g_pBitBucket[idDrive]->cbMaxSize <= ulSize.LowPart) 
    {
        // we set piRet = BBDELETE_SIZE_TOO_BIG so our caller can 
        // detect the "file/folder too big" case
        *piRet = BBDELETE_SIZE_TOO_BIG;

        return FALSE;
    }

    // Before we move the file we save off the "short" name. This is in case we have 
    // a unicode path and we need the ansi short path in case a win95 machine later tries to
    // restore this file. We can't do this later because GetShortPathName relies on the 
    // file actually exising.
    if (!GetShortPathName(pszFile, szShortFileName, ARRAYSIZE(szShortFileName)))
    {
        ASSERTMSG(FALSE, "Bitbucket: BBDeleteFile failed to get a short name for %s !", pszFile);
        lstrcpyn(szShortFileName, pszFile, ARRAYSIZE(szShortFileName));
    }

TryMoveAgain:

    // get the target name and move it
    iIndex = SHGlobalCounterIncrement(g_pBitBucket[idDrive]->hgcNextFileNum);
    GetDeletedFileName(szFileName, idDrive, iIndex, pszFile);
    DriveIDToBBPath(idDrive, szBitBucket);

    if (PathAppend(szBitBucket, szFileName))
    {
        iRet = SHMoveFile(pszFile, szBitBucket, fIsDir ? SHCNE_RMDIR : SHCNE_DELETE);

        // do GetLastError here so that we don't get the last error from the PathFileExists
        dwLastError = (iRet ? ERROR_SUCCESS : GetLastError());

        if (!iRet) 
        {
            if (ERROR_ALREADY_EXISTS == dwLastError)
            {
                TraceMsg(TF_BITBUCKET, "Bitbucket: BBDeleteFile found a file of the same name (%s) - skipping", szBitBucket);
                // generate a new filename and retry
                goto TryMoveAgain;
            }
            else 
            {
                // is our recycled directory still there?
                TCHAR szTemp[MAX_PATH];
                SHGetPathFromIDList(g_pBitBucket[idDrive]->pidl, szTemp);
                // if it already exists or there was some error in creating it, bail
                // else try again
                if (!PathIsDirectory(szTemp) && CreateRecyclerDirectory(idDrive))
                {
                    // if we did just re-create the directory, we need to reset the info
                    // file or the drive will get corrupted.
                    VerifyBBInfoFileHeader(idDrive);
                    goto TryMoveAgain;
                }
            }
        }
        else 
        {
            // success!
            BBAddDeletedFileInfo(pszFile, szShortFileName, iIndex, idDrive, ulSize.LowPart);
    
            if (lpua)
                FOUndo_AddInfo(lpua, pszFile, szBitBucket, 0);

            *piRet = BBDELETE_SUCCESS;
            return TRUE;
        }
    }
    else
    {
        // if path append fails, it probably means that the path is too long
        *piRet = BBDELETE_PATH_TOO_LONG;
        return FALSE;
    }

    // set back the correct last error
    SetLastError(dwLastError);
    
    // something bad happened, we dont know what it is
    *piRet = BBDELETE_UNKNOWN_ERROR;

    return FALSE;
}


// Basically it understands how we the trash is layed out which is fine
// as we are in the bitbucket code file... So we skip the first 3
// characters for the root of the name: c:\ and we truncate off the
// last part of the name and the rest should match our deathrow name...
BOOL IsFileInBitBucket(LPCTSTR pszPath)
{
    TCHAR szPath[MAX_PATH];
    int idDrive = DriveIDFromBBPath(pszPath);

    if (idDrive != -1)
    {
        if (IsBitBucketableDrive(idDrive))
        {
            DriveIDToBBPath(idDrive, szPath);

            return(PathCommonPrefix(szPath, pszPath, NULL) == lstrlen(szPath));
        }
    }

    return FALSE;
}


//
// This is called by the copy engine when a user selects "undo".
//
// NOTE: takes two multistrings (seperated/double null terminated file lists)
void UndoBBFileDelete(LPCTSTR lpszOriginal, LPCTSTR lpszDelFile)
{
    SHFILEOPSTRUCT sFileOp = {NULL,
                              FO_MOVE,
                              lpszDelFile,
                              lpszOriginal,
                              FOF_NOCONFIRMATION | FOF_MULTIDESTFILES | FOF_SIMPLEPROGRESS};

    // BUGBUG: we need to find out specific information about which
    // file ops were aborted
    SHFileOperation(&sFileOp);

    SHUpdateRecycleBinIcon();
}


STDAPI_(void) SHUpdateRecycleBinIcon()
{
    UpdateIcon(!IsRecycleBinEmpty());
}


void PurgeOneBitBucket(HWND hwnd, int idDrive, DWORD dwFlags)
{
    TCHAR szPath[MAX_PATH];
    HANDLE hFile;
    SHFILEOPSTRUCT sFileOp = {hwnd,
                              FO_DELETE,
                              szPath,
                              NULL,
                              FOF_SIMPLEPROGRESS,
                              FALSE,
                              NULL,
                              MAKEINTRESOURCE(IDS_BB_EMPTYINGWASTEBASKET)};

    ASSERT(g_pBitBucket[idDrive] && (g_pBitBucket[idDrive] != (PBBSYNCOBJECT)-1));

    if (dwFlags & SHERB_NOCONFIRMATION)
    {
        sFileOp.fFlags |= FOF_NOCONFIRMATION;
    }

    if (dwFlags & SHERB_NOPROGRESSUI)
    {
        sFileOp.fFlags |= FOF_SILENT;
    }

    DriveIDToBBPath(idDrive, szPath);
    PathAppend(szPath, c_szDStarDotStar);
    szPath[lstrlen(szPath) + 1] = TEXT('\0'); // double null terminate

#ifdef BB_USE_MRSW
    MRSW_EnterWrite(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW
    hFile = OpenBBInfoFile(idDrive, OPENBBINFO_WRITE, 0);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        // now do the actual delete.
        if (SHFileOperation(&sFileOp) || sFileOp.fAnyOperationsAborted)
        {
            TraceMsg(TF_BITBUCKET, "Bitbucket: emptying bucket on %s failed", szPath);

            // NOTE: the info file may point to some files that have been deleted,
            // it will be cleaned up later
        }
        else
        {
            // reset the info file since we just emptied this bucket.
            ResetInfoFileHeader(hFile, g_pBitBucket[idDrive]->fIsUnicode);
        }

        // we always re-create the desktop.ini
        CreateRecyclerDirectory(idDrive);

        CloseBBInfoFile(hFile, idDrive);
    }
    
#ifdef BB_USE_MRSW
    MRSW_LeaveWrite(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

    SHUpdateRecycleBinIcon();
}


// This function checks to see if an local NT directory is delete-able
//
// returns:
//      TRUE   yes, the dir can be nuked
//      FALSE  for UNC dirs or dirs on network drives, or
//             if the user does not have enough privlidges
//             to delete the file (acls).
//
// NOTE: this code is largely stolen from the RemoveDirectoryW API (windows\base\client\dir.c). if
//       you think that there is a bug in it, then diff it against the DeleteFileW and see it something
//       there changed.
//
// also sets the last error to explain why
//
BOOL IsDirectoryDeletable(LPCTSTR pszDir)
{
#ifdef WINNT
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_DISPOSITION_INFORMATION Disposition;
    FILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    DWORD dwAttributes;
    BOOL fChangedAttribs = FALSE;

    // return false for any network drives (allow UNC)
    if (IsNetDrive(PathGetDriveNumber(pszDir)))
    {
        return FALSE;
    }

    if (PathIsUNC(pszDir) && PathIsDirectoryEmpty(pszDir))
    {
        // HACKACK - (reinerf) the rdr sucks and will nuke the file when we call
        // NtSetInformationFile to set the deleted bit on an empty directory even
        // though we pass READ_CONTROL and we still have a handle to the object.
        // So, to work around this, we just assume we can always delete empty
        // directories (ha!)
        return TRUE;
    }

    // check to see if the dir is readonly
    dwAttributes = GetFileAttributes(pszDir);
    if ((dwAttributes != -1) && (dwAttributes & FILE_ATTRIBUTE_READONLY))
    {
        fChangedAttribs = TRUE;

        if (!SetFileAttributes(pszDir, dwAttributes & ~FILE_ATTRIBUTE_READONLY))
        {
            return FALSE;
        }
    }

    TranslationStatus = RtlDosPathNameToNtPathName_U(pszDir,
                                                     &FileName,
                                                     NULL,
                                                     &RelativeName);
    if (!TranslationStatus)
    {
        if (fChangedAttribs)
        {
            // set the attribs back
            SetFileAttributes(pszDir, dwAttributes);
        }

        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    FreeBuffer = FileName.Buffer;

    if (RelativeName.RelativeName.Length)
    {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
    }
    else
    {
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes(&Obja,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               NULL);

    if (IsOS(OS_NT5))
    {
        //
        // on NT5 we need to do some extra repars point munging
        //

        //
        // Open the directory for delete access.
        // Inhibit the reparse behavior using FILE_OPEN_REPARSE_POINT.
        //
        Status = NtOpenFile(&Handle,
                            DELETE | SYNCHRONIZE | FILE_READ_ATTRIBUTES | READ_CONTROL,
                            &Obja,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT);

        if (!NT_SUCCESS(Status))
        {
            //
            // Back level file systems may not support reparse points and thus not
            // support symbolic links.
            // We infer this is the case when the Status is STATUS_INVALID_PARAMETER.
            //

            if (Status == STATUS_INVALID_PARAMETER)
            {
                //   
                // Re-open not inhibiting the reparse behavior and not needing to read the attributes.
                //
                Status = NtOpenFile(&Handle,
                                    DELETE | SYNCHRONIZE | READ_CONTROL,
                                    &Obja,
                                    &IoStatusBlock,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);

                if (!NT_SUCCESS(Status))
                {
                    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

                    if (fChangedAttribs)
                    {
                        // set the attribs back
                        SetFileAttributes(pszDir, dwAttributes);
                    }

                    SetLastError(RtlNtStatusToDosError(Status));
                    return FALSE;
                }
            }
            else
            {
                RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

                if (fChangedAttribs)
                {
                    // set the attribs back
                    SetFileAttributes(pszDir, dwAttributes);
                }
                
                SetLastError(RtlNtStatusToDosError(Status));
                return FALSE;
            }
        }
        else
        {
            //
            // If we found a reparse point that is not a name grafting operation,
            // either a symbolic link or a mount point, we re-open without 
            // inhibiting the reparse behavior.
            //
            Status = NtQueryInformationFile(Handle,
                                            &IoStatusBlock,
                                            (PVOID) &FileTagInformation,
                                            sizeof(FileTagInformation),
                                            FileAttributeTagInformation);
        
            if (!NT_SUCCESS(Status))
            {
                //
                // Not all File Systems implement all information classes.
                // The value STATUS_INVALID_PARAMETER is returned when a non-supported
                // information class is requested to a back-level File System. As all the
                // parameters to NtQueryInformationFile are correct, we can infer that
                // we found a back-level system.
                //
                // If FileAttributeTagInformation is not implemented, we assume that
                // the file at hand is not a reparse point.
                //

                if ((Status != STATUS_NOT_IMPLEMENTED) && (Status != STATUS_INVALID_PARAMETER))
                {
                    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                    NtClose(Handle);

                    if (fChangedAttribs)
                    {
                        // set the attribs back
                        SetFileAttributes(pszDir, dwAttributes);
                    }

                    SetLastError(RtlNtStatusToDosError(Status));
                    return FALSE;
                }
            }

            if (NT_SUCCESS(Status) && (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
            {
                if (FileTagInformation.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
                {
                    //
                    // We want to make sure that we return FALSE for mounted volumes. This will cause BBDeleteFile
                    // to return BBDELETE_CANNOT_DELETE so that we will actuall delete the mountpoint and not try to
                    // move the mount point to the recycle bin or walk into it.
                    //
                    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                    NtClose(Handle);
    
                    if (fChangedAttribs)
                    {
                         // set the attribs back
                        SetFileAttributes(pszDir, dwAttributes);
                    }

                    // hmmm... lets pull ERROR_NOT_A_REPARSE_POINT out of our butt and return that error code!
                    SetLastError(ERROR_NOT_A_REPARSE_POINT);
                    return FALSE;
                }
            }
        
            if (NT_SUCCESS(Status) && (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
            {
                //
                // Re-open without inhibiting the reparse behavior and not needing to 
                // read the attributes.
                //
                NtClose(Handle);
                Status = NtOpenFile(&Handle,
                                    DELETE | SYNCHRONIZE | READ_CONTROL,
                                    &Obja,
                                    &IoStatusBlock,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);

                if (!NT_SUCCESS(Status))
                {
                    //
                    // When the FS Filter is absent, delete it any way.
                    //
                    if (Status == STATUS_IO_REPARSE_TAG_NOT_HANDLED)
                    {
                        //
                        // We re-open (possible 3rd open) for delete access inhibiting the reparse behavior.
                        //
                        Status = NtOpenFile(&Handle,
                                            DELETE | SYNCHRONIZE | READ_CONTROL,
                                            &Obja,
                                            &IoStatusBlock,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT);
                    }

                    if (!NT_SUCCESS(Status))
                    {
                        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                        
                        if (fChangedAttribs)
                        {
                            // set the attribs back
                            SetFileAttributes(pszDir, dwAttributes);
                        }

                        SetLastError(RtlNtStatusToDosError(Status));
                        return FALSE;
                    }
                }
            }
        }
    }
    else
    {
        //
        // This is the code to open the file handle as it shipped in NT4, we dont need to do any of
        // the reparse point stuff since they are not supported on NT4
        //

        // Open the directory for delete access
        Status = NtOpenFile(&Handle,
                            DELETE | SYNCHRONIZE | READ_CONTROL,
                            &Obja,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);
    }

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

    //
    // Attempt to set the delete bit
    //
#undef DeleteFile
    Disposition.DeleteFile = TRUE;

    Status = NtSetInformationFile(Handle,
                                  &IoStatusBlock,
                                  &Disposition,
                                  sizeof(Disposition),
                                  FileDispositionInformation);

    if (NT_SUCCESS(Status)) 
    {
        //
        // yep, we were able to set the bit, now unset it so its not delted!
        //
        Disposition.DeleteFile = FALSE;
        Status = NtSetInformationFile(Handle,
                                      &IoStatusBlock,
                                      &Disposition,
                                      sizeof(Disposition),
                                      FileDispositionInformation);
        NtClose(Handle);
        
        if (fChangedAttribs)
        {
            // set the attribs back
            SetFileAttributes(pszDir, dwAttributes);
        }
        return TRUE;
    }
    else
    {
        //
        // nope couldnt set the del bit. can't be deleted
        //
        TraceMsg(TF_BITBUCKET, "IsDirectoryDeletable: NtSetInformationFile failed, status=0x%08x", Status);

        NtClose(Handle);

        if (fChangedAttribs)
        {
             // set the attribs back
            SetFileAttributes(pszDir, dwAttributes);
        }

        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
#endif // WINNT
    return TRUE;
}


// This function checks to see if a local NT file is delete-able
//
// returns:
//      TRUE   yes, the file can be nuked
//      FALSE  for UNC files or files on network drives
//      FALSE  if the file is in use
//
// NOTE: this code is largely stolen from the DeleteFileW API (windows\base\client\filemisc.c). if
//       you think that there is a bug in it, then diff it against the DeleteFileW and see it something
//       there changed.
//
// also sets the last error to explain why
//
BOOL IsFileDeletable(LPCTSTR pszFile)
{
#ifdef WINNT
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_DISPOSITION_INFORMATION Disposition;
    FILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;
    BOOLEAN fIsSymbolicLink = FALSE;
    DWORD dwAttributes;
    BOOL fChangedAttribs = FALSE;

    // return false for any network drives
    if (IsNetDrive(PathGetDriveNumber(pszFile)))
    {
        return FALSE;
    }

    // check to see if the file is readonly or system
    dwAttributes = GetFileAttributes(pszFile);
    if (dwAttributes != -1)
    {
        if (dwAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
        {
            fChangedAttribs = TRUE;

            if (!SetFileAttributes(pszFile, dwAttributes & ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
            {
                return FALSE;
            }
        }
    }

    TranslationStatus = RtlDosPathNameToNtPathName_U(pszFile,
                                                     &FileName,
                                                     NULL,
                                                     &RelativeName);

    if (!TranslationStatus)
    {
        if (fChangedAttribs)
        {
             // set the attribs back
            SetFileAttributes(pszFile, dwAttributes);
        }

        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    FreeBuffer = FileName.Buffer;

    if (RelativeName.RelativeName.Length)
    {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
    }
    else
    {
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes(&Obja,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               NULL);

    if (IsOS(OS_NT5))
    {
        //
        // on NT5 we need to do some extra repars point munging
        //

        //
        // Open the file for delete access
        //
        Status = NtOpenFile(&Handle,
                            (ACCESS_MASK)DELETE | FILE_READ_ATTRIBUTES | READ_CONTROL,
                            &Obja,
                            &IoStatusBlock,
                            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT);

        if (!NT_SUCCESS(Status))
        {
            //
            // Back level file systems may not support reparse points and thus not
            // support symbolic links.
            // We infer this is the case when the Status is STATUS_INVALID_PARAMETER.
            //

            if (Status == STATUS_INVALID_PARAMETER)
            {
                //
                // Open without inhibiting the reparse behavior and not needing to
                // read the attributes.
                //

                Status = NtOpenFile(&Handle,
                                    (ACCESS_MASK)DELETE | READ_CONTROL,
                                    &Obja,
                                    &IoStatusBlock,
                                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT);

                if (!NT_SUCCESS(Status))
                {
                    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    
                    if (fChangedAttribs)
                    {
                         // set the attribs back
                        SetFileAttributes(pszFile, dwAttributes);
                    }

                    SetLastError(RtlNtStatusToDosError(Status));
                    return FALSE;
                }
            }
            else
            {
                RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    
                if (fChangedAttribs)
                {
                     // set the attribs back
                    SetFileAttributes(pszFile, dwAttributes);
                }

                SetLastError(RtlNtStatusToDosError(Status));
                return FALSE;
            }
        }
        else
        {
            //
            // If we found a reparse point that is not a symbolic link, we re-open
            // without inhibiting the reparse behavior.
            //
            Status = NtQueryInformationFile(Handle,
                                            &IoStatusBlock,
                                            (PVOID) &FileTagInformation,
                                            sizeof(FileTagInformation),
                                            FileAttributeTagInformation);
            if (!NT_SUCCESS(Status))
            {
                //
                // Not all File Systems implement all information classes.
                // The value STATUS_INVALID_PARAMETER is returned when a non-supported
                // information class is requested to a back-level File System. As all the
                // parameters to NtQueryInformationFile are correct, we can infer that
                // we found a back-level system.
                //
                // If FileAttributeTagInformation is not implemented, we assume that
                // the file at hand is not a reparse point.
                //

                if ((Status != STATUS_NOT_IMPLEMENTED) && (Status != STATUS_INVALID_PARAMETER))
                {
                    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
                    NtClose(Handle);
    
                    if (fChangedAttribs)
                    {
                         // set the attribs back
                        SetFileAttributes(pszFile, dwAttributes);
                    }

                    SetLastError(RtlNtStatusToDosError(Status));
                    return FALSE;
                }
            }

            if (NT_SUCCESS(Status) && (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
            {
                if (FileTagInformation.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
                {
                    fIsSymbolicLink = TRUE;
                }
            }

            if (NT_SUCCESS(Status)                                                 &&
                (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
                !fIsSymbolicLink)
            {
                //
                // Re-open without inhibiting the reparse behavior and not needing to
                // read the attributes.
                //

                NtClose(Handle);
                Status = NtOpenFile(&Handle,
                                    (ACCESS_MASK)DELETE | READ_CONTROL,
                                    &Obja,
                                    &IoStatusBlock,
                                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT);

                if (!NT_SUCCESS(Status))
                {
                    //
                    // When the FS Filter is absent, delete it any way.
                    //

                    if (Status == STATUS_IO_REPARSE_TAG_NOT_HANDLED)
                    {
                        //
                        // We re-open (possible 3rd open) for delete access inhibiting the reparse behavior.
                        //

                        Status = NtOpenFile(&Handle,
                                            (ACCESS_MASK)DELETE | READ_CONTROL,
                                            &Obja,
                                            &IoStatusBlock,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                            FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT);
                    }

                    if (!NT_SUCCESS(Status))
                    {
                        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    
                        if (fChangedAttribs)
                        {
                             // set the attribs back
                            SetFileAttributes(pszFile, dwAttributes);
                        }

                        SetLastError(RtlNtStatusToDosError(Status));
                        return FALSE;
                    }
                }
            }
        }
    }
    else
    {
        //
        // This is the code to open the file handle as it shipped in NT4, we dont need to do any of
        // the reparse point stuff since they are not supported on NT4
        //

        //
        // Open the file for delete access
        //
        Status = NtOpenFile(&Handle,
                            (ACCESS_MASK)DELETE | READ_CONTROL,
                            &Obja,
                            &IoStatusBlock,
                            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT);

        if (!NT_SUCCESS(Status)) 
        {
            RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

            if (fChangedAttribs)
            {
                 // set the attribs back
                SetFileAttributes((LPCTSTR) pszFile, dwAttributes);
            }
            
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }
    }


    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

    //
    // Attempt to set the delete bit
    //
#undef DeleteFile
    Disposition.DeleteFile = TRUE;

    Status = NtSetInformationFile(Handle,
                                  &IoStatusBlock,
                                  &Disposition,
                                  sizeof(Disposition),
                                  FileDispositionInformation);

    if (NT_SUCCESS(Status)) 
    {
        //
        // yep, we were able to set the bit, now unset it so its not delted!
        //
        Disposition.DeleteFile = FALSE;
        Status = NtSetInformationFile(Handle,
                                      &IoStatusBlock,
                                      &Disposition,
                                      sizeof(Disposition),
                                      FileDispositionInformation);
        NtClose(Handle);
        
        if (fChangedAttribs)
        {
            // set the attribs back
            SetFileAttributes(pszFile, dwAttributes);
        }
        return TRUE;
    }
    else
    {
        //
        // nope couldnt set the del bit. can't be deleted
        //
        TraceMsg(TF_BITBUCKET, "IsFileDeletable: NtSetInformationFile failed, status=0x%08x", Status);

        NtClose(Handle);

        if (fChangedAttribs)
        {
             // set the attribs back
            SetFileAttributes(pszFile, dwAttributes);
        }

        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }
#endif // WINNT

    //
    // on win9x, we just return true
    //
    return TRUE;
}


// This function adds up the sizes of the files in pszDir, and
// also makes sure that all those files are "delete-able"
//
// return:  ERROR_SUCCESS - everything is fine, all the files in the dir are deleteable
//          else          - the dir cant be deleted because something inside is non-deletable
//
// NOTE: other in-out params are in the pfdi
//
LONG CheckFolderSizeAndDeleteability(LPCTSTR pszDir, FOLDERDELETEINFO* pfdi)
{
    LONG lRet = ERROR_SUCCESS;  // keep stack to a minimum as this is a recursive function!
    BOOL bHasChildren = FALSE;

    // do the root specific processing
    if (!pfdi->bProcessedRoot)
    {
        // since the the destination folder could be something like "DC100000.oldext", calculate how many characters are
        // going to be in the new destination directory: "C:\recycler\sid" + "\" + "DC100000.oldext" == the new root directory length

        // BUGBUG (reinerf) - if the counter is above 1000000, then what? (eg filename wil be DC1000000.txt) this is all wrong...
        pfdi->cchDelta = (pfdi->cchBBDir + 1 + 8 + 1 + lstrlen(PathFindExtension(pszDir))) - lstrlen(pszDir);

        // set this so that we only do the above processing for the root folder
        pfdi->bProcessedRoot = TRUE;
    }

    if (PathCombine(pfdi->szPath, pszDir, c_szStarDotStar))
    {
        HANDLE hfind = FindFirstFile(pfdi->szPath, &pfdi->fd);

        // remove the *.* we added, because we will be recycling this buffer
        PathRemoveFileSpec(pfdi->szPath);

        if (hfind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!PathIsDotOrDotDot(pfdi->fd.cFileName))
                {
                    bHasChildren = TRUE;

                    // append the subfile/subfolder to the parent path
                    if (!PathAppend(pfdi->szPath, pfdi->fd.cFileName))
                    {
                        // PathAppend failed, try to append the short name
                        if (!pfdi->fd.cAlternateFileName[0] || !PathAppend(pfdi->szPath, pfdi->fd.cAlternateFileName))
                        {
                            // no alternate name or we failed to append that as well, assume we failed because the path is too long
                            lRet = ERROR_FILENAME_EXCED_RANGE;
                            
                            // pass back the name of the non-deleteable file/folder in pfdi->szNonDeletableFile
                            lstrcpyn(pfdi->szNonDeletableFile, pfdi->szPath, ARRAYSIZE(pfdi->szNonDeletableFile));
                        }
                    }

                    if (lRet == ERROR_SUCCESS)
                    {
                        // we have to check to see if the path will exceed MAX_PATH if we were to move this file to the recycle
                        // bin (C:\Recycler\<sid>). the increase in path length due to the recycle bin dir could be enough to 
                        // put us over MAX_PATH and we would have problems later.
                        if ((lstrlen(pfdi->szPath) + pfdi->cchDelta + 1) > MAX_PATH) // +1 for NULL
                        {
                            TraceMsg(TF_BITBUCKET, "CheckFolderSizeAndDeleteability: path '%s' would exceed MAX_PATH if moved to the recycle bin!", pfdi->szPath);
                            lRet = ERROR_FILENAME_EXCED_RANGE;

                            // pass back the name of the non-deleteable file/folder in pfdi->szNonDeletableFile
                            lstrcpyn(pfdi->szNonDeletableFile, pfdi->szPath, ARRAYSIZE(pfdi->szNonDeletableFile));
                        }
                        else
                        {
                            if (pfdi->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                            {
                                // its a directory, so recurse
                                lRet = CheckFolderSizeAndDeleteability(pfdi->szPath, pfdi);
                            }
                            else 
                            {
                                // its a file.
                                ULARGE_INTEGER ulTemp;

                                if (!IsFileDeletable(pfdi->szPath))
                                {
                                    // we cant delete this file, find out why
                                    lRet = GetLastError();
                                    ASSERT(lRet != ERROR_SUCCESS);
  
                                    // pass back the name of the non-deleteable file in pfdi->szNonDeletableFile
                                    lstrcpyn(pfdi->szNonDeletableFile, pfdi->szPath, ARRAYSIZE(pfdi->szNonDeletableFile));
                                }

                                ulTemp.LowPart  = pfdi->fd.nFileSizeLow;
                                ulTemp.HighPart = pfdi->fd.nFileSizeHigh;
                                pfdi->cbSize += ulTemp.QuadPart;
                            }
                        }
                    }

                    // remove the file/dir name we appended
                    PathRemoveFileSpec(pfdi->szPath);
                }

            } while ((lRet == ERROR_SUCCESS) && FindNextFile(hfind, &pfdi->fd));
            
            FindClose(hfind);
            
            // if this dir has no children, see if we can simply delete it
            if (!bHasChildren && !IsDirectoryDeletable(pszDir))
            {
                lRet = GetLastError();
                ASSERT(lRet != ERROR_SUCCESS);

                // pass back the name of the non-deleteable file in pfdi->szNonDeletableFile
                lstrcpyn(pfdi->szNonDeletableFile, pszDir, ARRAYSIZE(pfdi->szNonDeletableFile));
            }
        }
        else
        {
            // if FindFirstFile fails, check to see if the directory itself is deleteable
            if (!IsDirectoryDeletable(pszDir))
            {
                lRet = GetLastError();
                ASSERT(lRet != ERROR_SUCCESS);

                // pass back the name of the non-deleteable file in pfdi->szNonDeletableFile
                lstrcpyn(pfdi->szNonDeletableFile, pszDir, ARRAYSIZE(pfdi->szNonDeletableFile));
            }
        }
    }
    else
    {
        // if PathCombine fails, assume its because the path is too long
        lRet = ERROR_FILENAME_EXCED_RANGE;

        // pass back the name of the non-deleteable file in pfdi->szNonDeletableFile
        lstrcpyn(pfdi->szNonDeletableFile, pszDir, ARRAYSIZE(pfdi->szNonDeletableFile));
    }

    return lRet;
}
