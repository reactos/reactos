#include "shellprv.h"
#pragma  hdrstop
#include "netview.h"

// drivesx.c
DWORD PathGetClusterSize(LPCTSTR pszPath);

// mtpt.cpp
STDAPI_(void) CMtPt_InvalidateAllMtPts();
STDAPI_(void) CMtPt_InvalidateDriveType(int iDrive);

const TCHAR c_szAutoRun[] = TEXT("AutoRun");

static int rgiDriveType[26] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1
};

// A corresponding array which indicates whether or not
// the drivetype cache has the LFN and ACL bits.

static int rgiHasNetFlags[26] = { 0 };


// get connection information including disconnected drives
//
// in:
//     lpDev    device name "A:" "LPT1:", etc.
//     bConvertClosed
//              if FALSE closed or error drives will be converted to
//              WN_SUCCESS return codes.  if TRUE return not connected
//              and error state values (ie, the caller knows about not
//              connected and error state drives)
//
//     BUGBUG: we need to add cbPath to the output buffer
// out:
//     lpPath   filled with net name if return is WN_SUCCESS (or not connected/error)
// returns:
//     WN_*     error code

DWORD GetConnection(LPCTSTR lpDev, LPTSTR lpPath, UINT cchPath, BOOL bConvertClosed)
{
    DWORD err;
    int iType;

    iType = DriveType(DRIVEID(lpDev));

    if (iType == DRIVE_CDROM)
        return WN_NOT_CONNECTED;

    err = SHWNetGetConnection((LPTSTR)lpDev, lpPath, &cchPath);

    if (!bConvertClosed)
        if (err == WN_CONNECTION_CLOSED || err == WN_DEVICE_ERROR)
            err = WN_SUCCESS;

    return err;
}

// this is called for every drive at init time so it must
// be sure to not trigget things like the phantom B: drive support
//
// in:
//      iDrive  zero based drive number (0 = A, 1 = B)
//
// returns:
//      0       not a net drive
//      1       is a net drive, properly connected
//      2       disconnected/error state connection

int WINAPI IsNetDrive(int iDrive)
{
    DWORD err;
    TCHAR szDrive[4];
    TCHAR szConn[MAX_PATH];     // this really should be WNBD_MAX_LENGTH
                        // but this change would have to be many everywhere

    if ((iDrive >= 0) && (iDrive < 26))
    {
        PathBuildRoot(szDrive, iDrive);

        err = GetConnection(szDrive, szConn, ARRAYSIZE(szConn), TRUE);

        if (err == WN_SUCCESS)
            return 1;

        if (err == WN_CONNECTION_CLOSED || err == WN_DEVICE_ERROR)
            if ((GetLogicalDrives() & (1 << iDrive)) == 0)
                return 2;
    }
    
    return 0;
}

typedef BOOL (WINAPI* PFNISPATHSHARED)(IN LPCTSTR lpPath,IN BOOL fRefresh);

#ifdef UNICODE
#define SHARE_ENTRY "IsPathSharedW"        // Win NT UNICODE
#else // UNICODE
#ifdef WINNT
#define SHARE_ENTRY "IsPathSharedA"        // Win NT Ansi
#else // WINNT
#define SHARE_ENTRY  "IsPathShared"        // Win 95
#endif // WINNT
#endif // UNICODE

HMODULE g_hmodShare = (HMODULE)-1;
PFNISPATHSHARED g_pfnIsPathShared = NULL;

// ask the share provider if this path is shared

BOOL IsShared(LPNCTSTR pszPath, BOOL fUpdateCache)
{
    TCHAR szPath[MAX_PATH];

    // See if we have already tried to load this in this context
    if (g_hmodShare == (HMODULE)-1)
    {
        LONG cb = SIZEOF(szPath);

        g_hmodShare = NULL;     // asume failure

        SHRegQueryValue(HKEY_CLASSES_ROOT, TEXT("Network\\SharingHandler"), szPath, &cb);
        if (szPath[0]) 
        {
            g_hmodShare = LoadLibrary(szPath);
            if (g_hmodShare)
                g_pfnIsPathShared = (PFNISPATHSHARED)GetProcAddress(g_hmodShare, SHARE_ENTRY);
        }
    }

    if (g_pfnIsPathShared)
    {
#ifdef ALIGNMENT_SCENARIO
        ualstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));
        return g_pfnIsPathShared(szPath, fUpdateCache);
#else        
        return g_pfnIsPathShared(pszPath, fUpdateCache);
#endif
    }

    return FALSE;
}

// invalidate the DriveType cache for one entry, or all
void WINAPI InvalidateDriveType(int iDrive)
{
    if (iDrive < 0)
    {
        //
        //  invalidate all drives
        //
#ifndef DEBUG
        RegDeleteKey(HKEY_CLASSES_ROOT, c_szAutoRun);
#endif

        // Clear these values under the critical section, so any pair will
        // always be in sync

        CMtPt_InvalidateAllMtPts();

    }
    else if (iDrive < 26)
    {
        TCHAR szDrive[10];
        SHFILEINFO sfi;
        int iIcon=0;

        //
        //  invalidate a single drive, if the icon for a drive changes
        //  send a notify so links can update..  Handle the case where
        //  the drive was a unavailable net drive...
        //
        if  ((rgiDriveType[iDrive] != -1) &&
            ((rgiDriveType[iDrive] & ~DRIVE_TYPE) != ~DRIVE_TYPE) &&
            (((rgiDriveType[iDrive] & DRIVE_TYPE) >= DRIVE_REMOVABLE)
                || (rgiDriveType[iDrive] & DRIVE_NETUNAVAIL)))
        {
            PathBuildRoot(szDrive, iDrive);
            SHGetFileInfo(szDrive, 0, &sfi, SIZEOF(sfi), SHGFI_SYSICONINDEX);
            iIcon = sfi.iIcon;
        }

        // Clear these values under the critical section, so they'll always
        // be in sync with each other

        CMtPt_InvalidateDriveType(iDrive);

        if (iIcon != 0)
        {
            SHGetFileInfo(szDrive, 0, &sfi, SIZEOF(sfi), SHGFI_SYSICONINDEX);

            if (iIcon != sfi.iIcon &&
                (rgiDriveType[iDrive] & ~DRIVE_TYPE) != ~DRIVE_TYPE &&
                (rgiDriveType[iDrive] & DRIVE_TYPE) >= DRIVE_REMOVABLE)
            {
                TraceMsg(TF_PATH, "InvalidateDriveType: sending icon change for %s (%d => %d)", szDrive, iIcon, sfi.iIcon);
                SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_DWORD, (LPCVOID)IntToPtr( iIcon ), NULL);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#ifdef TRACK_DEFAULT_DRIVE
// we may want to instance this data in the future
// so keep it in this structure

typedef struct {
    PHASHITEM rphiPaths[27];    // [26] is for the no default drive case
    int iDefaultDrive;
} CURRENTDIR_DATA, *LPCURRENTDIR_DATA;

CURRENTDIR_DATA cdd = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -1
};

// returns:
//      the default drive 0 = A, 1 = B, 2 = C... or
//      -1 if the default is a UNC type name (no drive associated)
//      if no default was previously set the
//      window drive is used as the default

int WINAPI GetDefaultDrive(void)
{
    return cdd.iDefaultDrive;
}

// in:
//      iDrive number 0 = A, 1 = B, 2 = C to set as default
//      -1 for no default drive (UNC name is default)
//
// returns:
//      previous default drive
//
int WINAPI SetDefaultDrive(int iDrive)
{
    int iOldDrive;

    iOldDrive = GetDefaultDrive();

    if (iDrive >= 0 && iDrive < 26)
        cdd.iDefaultDrive = iDrive;
    else
        cdd.iDefaultDrive = -1;

    return iOldDrive;
}

// in:
//      iDrive  0 = A, 1 = B, ... or
//      -1 for UNC type path
// returns:
//      lpPath  fully qualified path (ANSI string)
//
void WINAPI GetDefaultDirectory(int iDrive, LPTSTR lpPath)
{
    if (iDrive < 0 || iDrive >= 26)
        iDrive = 26;

    if (cdd.rphiPaths[iDrive])
        GetHashItemName(NULL, cdd.rphiPaths[iDrive], lpPath, MAX_PATH);
    else
        PathBuildRoot(lpPath, iDrive);
}

// in:
//      lpPath   fully qualified path (ANSI string)
// note:
//      this does not set the default drive, if you
//      need to change that it must be done explicitly
//
int WINAPI SetDefaultDirectory(LPCTSTR lpPath)
{
    PHASHITEM phiPath;
    int iDrive;

    iDrive = DRIVEID(lpPath);

    if (iDrive < 0 || iDrive >= 26)
        iDrive = 26;

    phiPath = AddHashItem(NULL, lpPath);
    if (phiPath) {
        if (cdd.rphiPaths[iDrive])
            DeleteHashItem(NULL, cdd.rphiPaths[iDrive]);
        cdd.rphiPaths[iDrive] = phiPath;
        return TRUE;
    }
    return FALSE;
}
#endif


#ifdef WINNT

#define ROUND_TO_CLUSER(qw, dwCluster)  ((((qw) + (dwCluster) - 1) / dwCluster) * dwCluster)

//
// GetCompresedFileSize is NT only, so we only implement the SHGetCompressedFileSizeW
// version. This will return the size of the file on disk rounded to the cluster size.
//
STDAPI_(DWORD) SHGetCompressedFileSizeW(LPCWSTR pszFileName, LPDWORD pFileSizeHigh)
{
    DWORD dwClusterSize;
    ULARGE_INTEGER ulSizeOnDisk;

    if (!pszFileName || !pszFileName[0])
    {
        ASSERT(FALSE);
        *pFileSizeHigh = 0;
        return 0;
    }

    dwClusterSize = PathGetClusterSize(pszFileName);

    ulSizeOnDisk.LowPart = GetCompressedFileSizeW(pszFileName, &ulSizeOnDisk.HighPart);

    if ((ulSizeOnDisk.LowPart == (DWORD)-1) && (GetLastError() != NO_ERROR))
    {
        WIN32_FILE_ATTRIBUTE_DATA fad;

        TraceMsg(TF_WARNING, "GetCompressedFileSize failed on %s (lasterror = %x)", pszFileName, GetLastError());

        if (GetFileAttributesExW(pszFileName, GetFileExInfoStandard, &fad))
        {
            // use the normal size, but round it to the cluster size
            ulSizeOnDisk.LowPart = fad.nFileSizeLow;
            ulSizeOnDisk.HighPart = fad.nFileSizeHigh;
            
            ROUND_TO_CLUSER(ulSizeOnDisk.QuadPart, dwClusterSize);
        }
        else
        {
            // since both GetCompressedFileSize and GetFileAttributesEx failed, we
            // just return zero
            ulSizeOnDisk.QuadPart = 0;
        }
    }

    // for files < one cluster, GetCompressedFileSize returns real size so we need
    // to round it up to one cluster
    if (ulSizeOnDisk.QuadPart < dwClusterSize)
    {
        ulSizeOnDisk.QuadPart = dwClusterSize;
    }

    *pFileSizeHigh = ulSizeOnDisk.HighPart;
    return ulSizeOnDisk.LowPart;
}
#endif




// if we are not on NT, we need a fn pointer so we can try to getprocaddress GetDiskFreeSpaceExA.
#ifndef WINNT

// Make sure this is perinstance
#pragma data_seg(DATASEG_PERINSTANCE)

typedef BOOL (*PFNGETDISKFREESPACEEXA)(LPCSTR lpDirectoryName,
                                       PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                       PULARGE_INTEGER lpTotalNumberOfBytes,
                                       PULARGE_INTEGER lpTotalNumberOfFreeBytes);

PFNGETDISKFREESPACEEXA g_pfnGetDiskFreeSpaceExA = (PFNGETDISKFREESPACEEXA)-1;
#pragma data_seg()
#endif // not WINNT



STDAPI_(BOOL) SHGetDiskFreeSpaceEx(LPCTSTR pszDirectoryName,
                                   PULARGE_INTEGER pulFreeBytesAvailableToCaller,
                                   PULARGE_INTEGER pulTotalNumberOfBytes,
                                   PULARGE_INTEGER pulTotalNumberOfFreeBytes)
{
#ifdef WINNT
    // on NT just call the real API
    return GetDiskFreeSpaceEx(pszDirectoryName, pulFreeBytesAvailableToCaller, pulTotalNumberOfBytes, pulTotalNumberOfFreeBytes);
#else
    // this case is implicitly ANSI, since we are not running on NT. We will
    // be call the ANSI GetDiskFreeSpaceEx if it exists. If it does not exist
    // (meaning we are running on win95 gold), we call the old GetDiskFreeSpace and
    // do the math.
    if (g_pfnGetDiskFreeSpaceExA == (PFNGETDISKFREESPACEEXA) -1)
    {
        HMODULE hmod = GetModuleHandle(TEXT("KERNEL32"));
        if (hmod)
            g_pfnGetDiskFreeSpaceExA = (PFNGETDISKFREESPACEEXA)GetProcAddress(hmod, "GetDiskFreeSpaceExA");
        else
            g_pfnGetDiskFreeSpaceExA = NULL;
    }

    if (g_pfnGetDiskFreeSpaceExA)
    {
        // found the real API, so use it
        return g_pfnGetDiskFreeSpaceExA(pszDirectoryName, pulFreeBytesAvailableToCaller, pulTotalNumberOfBytes, pulTotalNumberOfFreeBytes);
    }
    else
    {
        // could not find a GetDiskFreeSpaceExA in kernel32, must be on 
        // win95 gold (not OSR2 or later...), need to do the math
        DWORD dwSectorsPerCluster;
        DWORD dwBytesPerSector;
        DWORD dwNumberOfFreeClusters;
        DWORD dwTotalNumberOfClusters;

        if (GetDiskFreeSpaceA(pszDirectoryName, &dwSectorsPerCluster, &dwBytesPerSector, &dwNumberOfFreeClusters, &dwTotalNumberOfClusters))
        {
            pulTotalNumberOfBytes->QuadPart = ((ULONGLONG)dwTotalNumberOfClusters) * ((ULONGLONG)dwSectorsPerCluster) * ((ULONGLONG)dwBytesPerSector);
            pulTotalNumberOfFreeBytes->QuadPart = ((ULONGLONG)dwNumberOfFreeClusters) * ((ULONGLONG)dwSectorsPerCluster) * ((ULONGLONG)dwBytesPerSector);
            // we are on win95, so NO quotas
            pulFreeBytesAvailableToCaller->QuadPart = pulTotalNumberOfFreeBytes->QuadPart;
            return TRUE;
        }
        return FALSE;
    }
#endif
}

#ifdef UNICODE
BOOL SHGetDiskFreeSpaceExA(LPCSTR pszDirectoryName,
                           PULARGE_INTEGER pulFreeBytesAvailableToCaller,
                           PULARGE_INTEGER pulTotalNumberOfBytes,
                           PULARGE_INTEGER pulTotalNumberOfFreeBytes
                            )
{
    WCHAR wszDirectoryName[MAX_PATH];

    MultiByteToWideChar(CP_ACP, 0, pszDirectoryName, -1, wszDirectoryName, SIZECHARS(wszDirectoryName));
    return SHGetDiskFreeSpaceEx(wszDirectoryName, pulFreeBytesAvailableToCaller, pulTotalNumberOfBytes, pulTotalNumberOfFreeBytes);
}
#else

BOOL SHGetDiskFreeSpaceExW(LPCWSTR pszDirectoryName,
                           PULARGE_INTEGER pulFreeBytesAvailableToCaller,
                           PULARGE_INTEGER pulTotalNumberOfBytes,
                           PULARGE_INTEGER pulTotalNumberOfFreeBytes
                           )
{
    CHAR aszDirectoryName[MAX_PATH];

    SHUnicodeToAnsi(pszDirectoryName, aszDirectoryName, SIZECHARS(aszDirectoryName));
    return SHGetDiskFreeSpaceEx(aszDirectoryName, pulFreeBytesAvailableToCaller, pulTotalNumberOfBytes, pulTotalNumberOfFreeBytes); 
}
#endif

