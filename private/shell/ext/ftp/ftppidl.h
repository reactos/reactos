/*****************************************************************************
 *
 *	ftppidl.h - LPITEMIDLIST management routines
 *
 *****************************************************************************/


#ifndef _FTPPIDL_H
#define _FTPPIDL_H


/****************************************************\
    FTP PIDL to URL functions
\****************************************************/
#ifdef UNICODE
#define UrlCreateFromPidl   UrlCreateFromPidlW
#else // UNICODE
#define UrlCreateFromPidl   UrlCreateFromPidlA
#endif // UNICODE


// Create FTP Pidl
HRESULT CreateFtpPidlFromFtpWirePath(LPCWIRESTR pwFtpWirePath, CWireEncoding * pwe, ULONG *pcchEaten, LPITEMIDLIST * ppidl, BOOL fIsTypeKnown, BOOL fIsDir);
HRESULT CreateFtpPidlFromDisplayPath(LPCWSTR pwzFullPath, CWireEncoding * pwe, ULONG *pcchEaten, LPITEMIDLIST * ppidl, BOOL fIsTypeKnown, BOOL fIsDir);


HRESULT CreateFtpPidlFromUrl(LPCTSTR pszName, CWireEncoding * pwe, ULONG *pcchEaten, LPITEMIDLIST * ppidl, IMalloc * pm, BOOL fHidePassword);
HRESULT CreateFtpPidlFromUrlEx(LPCTSTR pszUrl, CWireEncoding * pwe, ULONG *pcchEaten, LPITEMIDLIST * ppidl, IMalloc * pm, BOOL fHidePassword, BOOL fIsTypeKnown, BOOL fIsDir);
HRESULT CreateFtpPidlFromUrlPathAndPidl(LPCITEMIDLIST pidl, CWireEncoding * pwe, LPCWIRESTR pwFtpWirePath, LPITEMIDLIST * ppidl);

// Get Data from FTP Pidl
HRESULT UrlCreateFromPidlW(LPCITEMIDLIST pidl, DWORD shgno, LPWSTR pwzUrl, DWORD cchSize, DWORD dwFlags, BOOL fHidePassword);
HRESULT UrlCreateFromPidlA(LPCITEMIDLIST pidl, DWORD shgno, LPSTR pszUrl, DWORD cchSize, DWORD dwFlags, BOOL fHidePassword);
HRESULT GetDisplayPathFromPidl(LPCITEMIDLIST pidl, LPWSTR pwzDisplayPath, DWORD cchUrlPathSize, BOOL fDirsOnly);
HRESULT GetWirePathFromPidl(LPCITEMIDLIST pidl, LPWIRESTR pwWirePath, DWORD cchUrlPathSize, BOOL fDirsOnly);



// Functions to work on an entire FTP PIDLs
BOOL FtpPidl_IsValid(LPCITEMIDLIST pidl);
BOOL FtpPidl_IsValidFull(LPCITEMIDLIST pidl);
BOOL FtpPidl_IsValidRelative(LPCITEMIDLIST pidl);
DWORD FtpPidl_GetVersion(LPCITEMIDLIST pidl);
BOOL FtpID_IsServerItemID(LPCITEMIDLIST pidl);
LPCITEMIDLIST FtpID_GetLastIDReferense(LPCITEMIDLIST pidl);

HRESULT FtpPidl_GetServer(LPCITEMIDLIST pidl, LPTSTR pszServer, DWORD cchSize);
BOOL FtpPidl_IsDNSServerName(LPCITEMIDLIST pidl);
HRESULT FtpPidl_GetUserName(LPCITEMIDLIST pidl, LPTSTR pszUserName, DWORD cchSize);
HRESULT FtpPidl_GetPassword(LPCITEMIDLIST pidl, LPTSTR pszPassword, DWORD cchSize, BOOL fIncludingHidenPassword);
HRESULT FtpPidl_GetDownloadTypeStr(LPCITEMIDLIST pidl, LPTSTR pszDownloadType, DWORD cchSize);
DWORD FtpPidl_GetDownloadType(LPCITEMIDLIST pidl);
INTERNET_PORT FtpPidl_GetPortNum(LPCITEMIDLIST pidl);
BOOL FtpPidl_IsDirectory(LPCITEMIDLIST pidl, BOOL fAssumeDirForUnknown);
ULONGLONG FtpPidl_GetFileSize(LPCITEMIDLIST pidl);
HRESULT FtpPidl_SetFileSize(LPCITEMIDLIST pidl, DWORD dwSizeHigh, DWORD dwSizeLow);
DWORD FtpPidl_GetAttributes(LPCITEMIDLIST pidl);
BOOL FtpPidl_HasPath(LPCITEMIDLIST pidl);
HRESULT FtpPidl_SetFileItemType(LPITEMIDLIST pidl, BOOL fIsDir);
HRESULT FtpPidl_GetFileInfo(LPCITEMIDLIST pidl, SHFILEINFO *psfi, DWORD rgf);
HRESULT FtpPidl_GetFileType(LPCITEMIDLIST pidl, LPTSTR pszType, DWORD cchSize);
HRESULT FtpPidl_GetFileTypeStrRet(LPCITEMIDLIST pidl, LPSTRRET pstr);
HRESULT FtpPidl_GetFragment(LPCITEMIDLIST pidl, LPTSTR pszFragment, DWORD cchSize);
HRESULT FtpPidl_SetAttributes(LPCITEMIDLIST pidl, DWORD dwAttribs);

HRESULT FtpPidl_GetWireName(LPCITEMIDLIST pidl, LPWIRESTR pwName, DWORD cchSize);
HRESULT FtpPidl_GetDisplayName(LPCITEMIDLIST pidl, LPWSTR pwzName, DWORD cchSize);
LPCWSTR FtpPidl_GetFileDisplayName(LPCITEMIDLIST pidl);
LPCWIRESTR FtpPidl_GetFileWireName(LPCITEMIDLIST pidl);
LPCWIRESTR FtpPidl_GetLastItemWireName(LPCITEMIDLIST pidl);
LPCWSTR FtpPidl_GetLastItemDisplayName(LPCITEMIDLIST pidl);
LPCWSTR FtpPidl_GetLastFileDisplayName(LPCITEMIDLIST pidl);
BOOL FtpPidl_IsAnonymous(LPCITEMIDLIST pidl);

HRESULT FtpPidl_ReplacePath(LPCITEMIDLIST pidlServer, LPCITEMIDLIST pidlFtpPath, LPITEMIDLIST * ppidlOut);


#define FILEATTRIB_DIRSOFTLINK (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)
#define FtpPidl_IsDirSoftLink(pidl)        (FILEATTRIB_DIRSOFTLINK == (FILEATTRIB_DIRSOFTLINK & FtpPidl_GetAttributes(pidl)))
#define FtpPidl_IsSoftLink(pidl)        (FILE_ATTRIBUTE_REPARSE_POINT & FtpPidl_GetAttributes(pidl))


// WIN32_FIND_DATA normally stores the dates/times in a time zone independent (UTC)
// way, but FTP doesn't.  This requires conversions of dates when transfering
// from one to another.
FILETIME FtpPidl_GetFileTime(LPCITEMIDLIST pidl);   // Return value is UTC
FILETIME FtpPidl_GetFTPFileTime(LPCITEMIDLIST pidl);    // Return value is in Local Time Zone.
void FtpItemID_SetFileTime(LPCITEMIDLIST pidl, FILETIME fileTime);   // fileTime is in UTC
HRESULT Win32FindDataFromPidl(LPCITEMIDLIST pidl, LPWIN32_FIND_DATA pwfd, BOOL fFullPath, BOOL fInDisplayFormat);
HRESULT FtpPidl_SetFileTime(LPCITEMIDLIST pidl, FILETIME ftTimeDate);   // ftTimeDate In UTC

HRESULT FtpPidl_InsertVirtualRoot(LPCITEMIDLIST pidlVirtualRoot, LPCITEMIDLIST pidlFtpPath, LPITEMIDLIST * ppidl);

BOOL IsFtpPidlQuestionable(LPCITEMIDLIST pidl);

#define FtpPidl_DirChoose(pidl, dir, file)  (FtpPidl_IsDirectory(pidl, TRUE) ? dir : file)

LPITEMIDLIST ILCloneFirstItemID(LPITEMIDLIST pidl);


/****************************************************\
    FTP Individual ServerID/ItemID functions
\****************************************************/

// Ftp ServerID Helper Functions
HRESULT FtpServerID_GetServer(LPCITEMIDLIST pidl, LPTSTR szServer, DWORD cchSize);
BOOL FtpServerID_ServerStrCmp(LPCITEMIDLIST pidl, LPCTSTR pszServer);
HRESULT FtpServerID_SetHiddenPassword(LPITEMIDLIST pidl, LPCTSTR pszPassword);
DWORD FtpServerID_GetTypeID(LPCITEMIDLIST pidl);
INTERNET_PORT FtpServerID_GetPortNum(LPCITEMIDLIST pidl);
HRESULT FtpServerID_Create(LPCTSTR pszServer, LPCTSTR pszUserName, LPCTSTR pszPassword, 
                     DWORD dwFlags, INTERNET_PORT ipPortNum, LPITEMIDLIST * ppidl, IMalloc *pm, BOOL fHidePassword);


// Ftp ItemID Creation Functions
HRESULT FtpItemID_CreateFake(LPCWSTR pwzDisplayName, LPCWIRESTR pwWireName, BOOL fTypeKnown, BOOL fIsFile, BOOL fIsFragment, LPITEMIDLIST * ppidl);
HRESULT FtpItemID_CreateReal(const LPFTP_FIND_DATA pwfd, LPCWSTR pwzDisplayName, LPITEMIDLIST * ppidl);


// Ftp ItemID Helper Functions
HRESULT FtpItemID_CreateWithNewName(LPCITEMIDLIST pidl, LPCWSTR pwzDisplayName, LPCWIRESTR pwWireName, LPITEMIDLIST * ppidlOut);
HRESULT FtpItemID_GetDisplayName(LPCITEMIDLIST pidl, LPWSTR pwzName, DWORD cchSize);
HRESULT FtpItemID_GetWireName(LPCITEMIDLIST pidl, LPWIRESTR pszName, DWORD cchSize);
HRESULT FtpItemID_GetFragment(LPCITEMIDLIST pidl, LPTSTR pszName, DWORD cchSize);
HRESULT FtpItemID_GetNameA(LPCITEMIDLIST pidl, LPSTR pszName, DWORD cchSize);
BOOL FtpItemID_IsFragment(LPCITEMIDLIST pidl);
BOOL FtpItemID_IsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
BOOL FtpPidl_IsPathEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
BOOL FtpItemID_IsParent(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild);
LPCITEMIDLIST FtpItemID_FindDifference(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild);

DWORD FtpItemID_GetAttributes(LPCITEMIDLIST pidl);
DWORD FtpItemID_SetDirAttribute(LPCITEMIDLIST pidl);
HRESULT FtpItemID_SetAttributes(LPCITEMIDLIST pidl, DWORD dwFileAttributes);
DWORD FtpItemID_GetUNIXPermissions(LPCITEMIDLIST pidl);
HRESULT FtpItemID_SetUNIXPermissions(LPCITEMIDLIST pidl, DWORD dwFileAttributes);

LPCWIRESTR FtpItemID_GetWireNameReference(LPCITEMIDLIST pidl);
LPCWSTR FtpItemID_GetDisplayNameReference(LPCITEMIDLIST pidl);

ULONGLONG FtpItemID_GetFileSize(LPCITEMIDLIST pidl);
void FtpItemID_SetFileSize(LPCITEMIDLIST pidl, ULARGE_INTEGER uliFileSize);
DWORD FtpItemID_GetFileSizeLo(LPCITEMIDLIST pidl);
DWORD FtpItemID_GetFileSizeHi(LPCITEMIDLIST pidl);

DWORD FtpItemID_GetCompatFlags(LPCITEMIDLIST pidl);
HRESULT FtpItemID_SetCompatFlags(LPCITEMIDLIST pidl, DWORD dwCompatFlags);


BOOL FtpItemID_IsDirectory(LPCITEMIDLIST pidl, BOOL fAssumeDirForUnknown);

// Flags for FtpItemID dwCompatFlags
#define COMPAT_WEBBASEDDIR       0x00000001

// Flags for dwCompFlags
#define FCMP_NORMAL             0x00000000
#define FCMP_GROUPDIRS          0x00000001
#define FCMP_CASEINSENSE        0x00000002

HRESULT FtpItemID_CompareIDs(LPARAM ici, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, DWORD dwCompFlags);
int FtpItemID_CompareIDsInt(LPARAM ici, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, DWORD dwCompFlags);



LPITEMIDLIST FtpCloneServerID(LPCITEMIDLIST pidl);
HRESULT PurgeSessionKey(void);

// NOT USED
//HRESULT CreateFtpPidlFromFindData(LPCTSTR pszBaseUrl, const LPWIN32_FIND_DATA pwfd, LPITEMIDLIST * ppidl, IMalloc * pm);
//HRESULT UrlGetFileNameFromPidl(LPCITEMIDLIST pidl, LPTSTR pszFileName, DWORD cchSize);
//HRESULT FtpServerID_CopyHiddenPassword(LPCITEMIDLIST pidlSrc, LPITEMIDLIST pidlDest);


#endif // _FTPPIDL_H

