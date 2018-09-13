//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1998.
//
//  File:       mulprsht.c
//
//  Contents:   Code for multi and single file property sheet page
//
//----------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop

#include "propsht.h"
#include <winbase.h>
#include <shellids.h>
#include "treewkcb.h"   // for FolderSize
#include "fstreex.h"    // for FS_ShowExtension
#include "util.h"       // for GetFileDescription
#include "prshtcpp.h"   // for progress dlg and recursive apply
#include "shlexec.h"    // for SIDKEYNAME 
#include "defext.h"


// drivesx.c
DWORD PathGetClusterSize(LPCTSTR pszPath);
DWORD CALLBACK _CDrives_PropertiesThread(PROPSTUFF *pps);

// version.c
extern void AddVersionPage(LPCTSTR pszFilePath, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

// link.c
extern BOOL AddLinkPage(LPCTSTR szFilePath, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
#ifdef WINNT
// shlexec.c
BOOL ShouldPromptUserLogon(LPTSTR pszExe, BOOL fUserSetting);

// from path.c
BOOL PathIsHighLatency(LPCTSTR pszFile, DWORD dwFileAttr);
#endif
//mdprsht.c
extern HPROPSHEETPAGE AddMountedDrvPage(LPCTSTR szFilePath, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam, BOOL bMounted);

BOOL AddLocationToolTips(HWND hDlg, UINT id, LPCTSTR pszText, HWND *phwndTT);
BOOL _IsBiDiCalendar(void);

const DWORD aGeneralHelpIds[] = {
        IDD_LINE_1,             NO_HELP,
        IDD_LINE_2,             NO_HELP,
        IDD_LINE_3,             NO_HELP,
        IDD_ITEMICON,           IDH_FPROP_GEN_ICON,
        IDD_NAMEEDIT,           IDH_FPROP_GEN_NAME,
        IDC_CHANGEFILETYPE,     IDH_FPROP_GEN_CHANGE,
        IDD_FILETYPE_TXT,       IDH_FPROP_GEN_TYPE,
        IDD_FILETYPE,           IDH_FPROP_GEN_TYPE,
        IDD_OPENSWITH_TXT,      IDH_FPROP_GEN_OPENSWITH,
        IDD_OPENSWITH,          IDH_FPROP_GEN_OPENSWITH,
        IDD_LOCATION_TXT,       IDH_FPROP_GEN_LOCATION,
        IDD_LOCATION,           IDH_FPROP_GEN_LOCATION,
        IDD_FILESIZE_TXT,       IDH_FPROP_GEN_SIZE,
        IDD_FILESIZE,           IDH_FPROP_GEN_SIZE,
        IDD_FILESIZE_COMPRESSED,     IDH_FPROP_GEN_COMPRESSED_SIZE,
        IDD_FILESIZE_COMPRESSED_TXT, IDH_FPROP_GEN_COMPRESSED_SIZE,
        IDD_CONTAINS_TXT,       IDH_FPROP_FOLDER_CONTAINS,
        IDD_CONTAINS,           IDH_FPROP_FOLDER_CONTAINS,
        IDD_CREATED_TXT,        IDH_FPROP_GEN_DATE_CREATED,
        IDD_CREATED,            IDH_FPROP_GEN_DATE_CREATED,              // BUGBUG
        IDD_LASTMODIFIED_TXT,   IDH_FPROP_GEN_LASTCHANGE,
        IDD_LASTMODIFIED,       IDH_FPROP_GEN_LASTCHANGE,
        IDD_LASTACCESSED_TXT,   IDH_FPROP_GEN_LASTACCESS,
        IDD_LASTACCESSED,       IDH_FPROP_GEN_LASTACCESS,
        IDD_ATTR_GROUPBOX,      IDH_COMM_GROUPBOX,
        IDD_READONLY,           IDH_FPROP_GEN_READONLY,
        IDD_HIDDEN,             IDH_FPROP_GEN_HIDDEN,
        IDD_ARCHIVE,            IDH_FPROP_GEN_ARCHIVE,
        IDC_ADVANCED,           IDH_FPROP_GEN_ADVANCED,
#ifdef WINNT
        IDC_DRV_PROPERTIES,     IDH_FPROP_GEN_MOUNTEDPROP,
        IDD_FILETYPE_TARGET,    IDH_FPROP_GEN_MOUNTEDTARGET,
        IDC_DRV_TARGET,         IDH_FPROP_GEN_MOUNTEDTARGET,
#endif
        0, 0
};

const DWORD aMultiPropHelpIds[] = {
        IDD_LINE_1,             NO_HELP,
        IDD_LINE_2,             NO_HELP,
        IDD_ITEMICON,           IDH_FPROP_GEN_ICON,
        IDD_CONTAINS,           IDH_MULTPROP_NAME,
        IDD_FILETYPE_TXT,       IDH_FPROP_GEN_TYPE,
        IDD_FILETYPE,           IDH_FPROP_GEN_TYPE,
        IDD_LOCATION_TXT,       IDH_FPROP_GEN_LOCATION,
        IDD_LOCATION,           IDH_FPROP_GEN_LOCATION,
        IDD_FILESIZE_TXT,       IDH_FPROP_GEN_SIZE,
        IDD_FILESIZE,           IDH_FPROP_GEN_SIZE,
        IDD_FILESIZE_COMPRESSED,     IDH_FPROP_GEN_COMPRESSED_SIZE,
        IDD_FILESIZE_COMPRESSED_TXT, IDH_FPROP_GEN_COMPRESSED_SIZE,
        IDD_ATTR_GROUPBOX,      IDH_COMM_GROUPBOX,
        IDD_READONLY,           IDH_FPROP_GEN_READONLY,
        IDD_HIDDEN,             IDH_FPROP_GEN_HIDDEN,
        IDD_ARCHIVE,            IDH_FPROP_GEN_ARCHIVE,
        IDC_ADVANCED,           IDH_FPROP_GEN_ADVANCED,
        0, 0
};

const DWORD aAdvancedHelpIds[] = {
        IDD_ITEMICON,           NO_HELP,
        IDC_MANAGEFILES_TXT,    NO_HELP,
        IDD_MANAGEFOLDERS_TXT,  NO_HELP,
        IDD_ARCHIVE,            IDH_FPROP_GEN_ARCHIVE,
        IDD_INDEX,              IDH_FPROP_GEN_INDEX,
        IDD_COMPRESS,           IDH_FPROP_GEN_COMPRESSED,
        IDD_ENCRYPT,            IDH_FPROP_GEN_ENCRYPT,
        0, 0
};


void UpdateSizeCount(FILEPROPSHEETPAGE * pfpsp)
{
    TCHAR szNum[32], szNum1[64];
    LPTSTR pszFmt;
    
    pszFmt = ShellConstructMessageString(HINST_THISDLL, 
                                         MAKEINTRESOURCE(pfpsp->fci.cbSize ? IDS_SIZEANDBYTES : IDS_SIZE),
                                         ShortSizeFormat64(pfpsp->fci.cbSize, szNum),
                                         AddCommas64(pfpsp->fci.cbSize, szNum1));
    if (pszFmt)
    {
        SetDlgItemText(pfpsp->hDlg, IDD_FILESIZE, pszFmt);
        LocalFree(pszFmt);
    }


    pszFmt = ShellConstructMessageString(HINST_THISDLL, 
                                         MAKEINTRESOURCE(pfpsp->fci.cbActualSize ? IDS_SIZEANDBYTES : IDS_SIZE),
                                         ShortSizeFormat64(pfpsp->fci.cbActualSize, szNum),
                                         AddCommas64(pfpsp->fci.cbActualSize, szNum1));

    if (pszFmt)
    {
        SetDlgItemText(pfpsp->hDlg, IDD_FILESIZE_COMPRESSED, pszFmt);
        LocalFree(pszFmt);
    }

    pszFmt = ShellConstructMessageString(HINST_THISDLL,
                                         MAKEINTRESOURCE(IDS_NUMFILES),
                                         AddCommas(pfpsp->fci.cFiles, szNum),
                                         AddCommas(pfpsp->fci.cFolders, szNum1));
    if (pszFmt && !pfpsp->fMountedDrive)
    {
        SetDlgItemText(pfpsp->hDlg, IDD_CONTAINS, pszFmt);
        LocalFree(pszFmt);
    }
}


#define ROUND_TO_CLUSER(qw, dwCluster)  ((((qw) + (dwCluster) - 1) / dwCluster) * dwCluster)

BOOL HIDA_FillFindData(HIDA hida, UINT iItem, LPTSTR pszPath, WIN32_FIND_DATA *pfd, BOOL fReturnCompressedSize)
{
    LPITEMIDLIST pidl;
    BOOL fRet = FALSE;      // assume error
    *pszPath = 0;           // assume error

    pidl = HIDA_ILClone(hida, iItem);
    if (pidl)
    {
        if (SHGetPathFromIDList(pidl, pszPath))
        {
            if (pfd)
            {
                HANDLE h = FindFirstFile(pszPath, pfd);
                if (h == INVALID_HANDLE_VALUE)
                {
                    // error, zero the bits
                    ZeroMemory(pfd, SIZEOF(*pfd));
                }
                else
                {
                    FindClose(h);
#ifdef WINNT                    
                    // if the user wants the compressed file size, and compression is supported, then go get it
                    if (fReturnCompressedSize && (pfd->dwFileAttributes & (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE)))
                    {
                        pfd->nFileSizeLow = SHGetCompressedFileSize(pszPath, &pfd->nFileSizeHigh);
                    }
#endif
                }
            }
            fRet = TRUE;
        }
        ILFree(pidl);
    }
    return fRet;
}


DWORD CALLBACK SizeThreadProc(FILEPROPSHEETPAGE *pfpsp)
{
    UINT iItem;
    TCHAR szPath[MAX_PATH];
    // We need to initialize OLE because HIDA_FillFindData causes CoCreateIntance() to
    // be called.  In general we may be going through 3rd party Shell Extns that may want
    // to use CoCreateInstance().
    HRESULT hr = SHCoInitialize();

    pfpsp->fci.cbSize  = 0;
    pfpsp->fci.cbActualSize = 0;
    pfpsp->fci.cFiles = 0;
    pfpsp->fci.cFolders = 0;

    // update the dialog every 1/4 second
    SetTimer(pfpsp->hDlg, IDT_SIZE, 250, NULL);

    for (iItem = 0; HIDA_FillFindData(pfpsp->hida, iItem, szPath, &pfpsp->fci.fd, FALSE) && pfpsp->fci.bContinue; iItem++)
    {
        if (pfpsp->fci.fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            FolderSize(szPath, &pfpsp->fci);
            
            if (pfpsp->fMultipleFiles)
            {
                // for multiple file/folder properties, count myself
                pfpsp->fci.cFolders++;
            }
        }
        else
        {   // file selected
            ULARGE_INTEGER ulSize;
            ULARGE_INTEGER ulSizeOnDisk;
            DWORD dwClusterSize = PathGetClusterSize(szPath);

#ifdef WINNT
            // if compression is supported, we check to see if the file is sparse or compressed
            if (pfpsp->fIsCompressionAvailable && (pfpsp->fci.fd.dwFileAttributes & (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE)))
            {
                ulSizeOnDisk.LowPart = SHGetCompressedFileSize(szPath, &ulSizeOnDisk.HighPart);
            }
            else
#endif
            {
                // not compressed or sparse, so just round to the cluster size
                ulSizeOnDisk.LowPart = pfpsp->fci.fd.nFileSizeLow;
                ulSizeOnDisk.HighPart = pfpsp->fci.fd.nFileSizeHigh;
                ulSizeOnDisk.QuadPart = ROUND_TO_CLUSTER(ulSizeOnDisk.QuadPart, dwClusterSize);
            }

            // add the size in
            ulSize.LowPart = pfpsp->fci.fd.nFileSizeLow;
            ulSize.HighPart = pfpsp->fci.fd.nFileSizeHigh;
            pfpsp->fci.cbSize += ulSize.QuadPart;

            // add the size on disk in
            pfpsp->fci.cbActualSize += ulSizeOnDisk.QuadPart;

            // increment the # of files
            pfpsp->fci.cFiles++;
        }

        // set this so the progress bar knows how much total work there is to do
        pfpsp->ulTotalNumberOfBytes.QuadPart = pfpsp->fci.cbActualSize;
    }

    KillTimer(pfpsp->hDlg, IDT_SIZE);
    // make sure that there is a WM_TIMER message in the queue so we will get the "final" results
    PostMessage(pfpsp->hDlg, WM_TIMER, (WPARAM)IDT_SIZE, (LPARAM)NULL);

    SHCoUninitialize(hr);
    return 0;
}


void CreateSizeThread(FILEPROPSHEETPAGE * pfpsp)
{
    if (pfpsp->fci.bContinue)
    {
        DWORD idThread;

        if (pfpsp->hThread)
        {
            if (WaitForSingleObject(pfpsp->hThread, 0) == WAIT_OBJECT_0)
            {
                // we had a previous thread that already finished
                CloseHandle(pfpsp->hThread);
            }
            else
            {
                // previous size thread still running, so bail
                return;
            }
        }

        pfpsp->hThread = CreateThread(NULL, 0, SizeThreadProc, pfpsp, 0, &idThread);
    }
}


void KillSizeThread(FILEPROPSHEETPAGE * pfpsp)
{
    if (pfpsp)
    {
        // signal the thread to stop
        pfpsp->fci.bContinue = FALSE;

        if (pfpsp->hThread)
        {
            // We will attempt to wait up to 90 seconds (twice the length of the RDR timeout)
            // for the thread to terminate
            if (WaitForSingleObject(pfpsp->hThread, 90000) == WAIT_TIMEOUT)
            {
                ASSERTMSG(FALSE, "KillSizeThread: waited for 90 seconds and the size thread still didn't return!!");
                
                TerminateThread(pfpsp->hThread, (DWORD)-1);
            }

            CloseHandle(pfpsp->hThread);
            pfpsp->hThread = NULL;
        }
    }
}


#ifdef WINNT
DWORD GetVolumeFlags(LPCTSTR pszPath, OUT OPTIONAL LPTSTR pszFileSys, int cchFileSys )
{
    TCHAR szRoot[MAX_PATH];
    DWORD dwVolumeFlags;

    /* Is this mounted point, e.g. c:\ or c:\hostfolder\ */
    if (!PathGetMountPointFromPath(pszPath, szRoot, ARRAYSIZE(szRoot)))
    {
        //no
        lstrcpy(szRoot, pszPath);
        PathStripToRoot(szRoot);
    }
    // GetVolumeInformation requires a trailing backslash.  Append one
    PathAddBackslash(szRoot);

    if( pszFileSys )
        *pszFileSys = 0 ;

    if (GetVolumeInformation(szRoot, NULL, 0, NULL, NULL, &dwVolumeFlags, pszFileSys, cchFileSys))
    {
        return dwVolumeFlags;
    }
    else
    {
        return 0;
    }
}
#endif


//
// This function sets the initial file attributes based on the dwFlagsAND / dwFlagsOR
// for the multiple file case
//
void SetInitialFileAttribs(FILEPROPSHEETPAGE* pfpsp, DWORD dwFlagsAND, DWORD dwFlagsOR)
{
    DWORD dwTriState = dwFlagsAND ^ dwFlagsOR; // this dword now has all the bits that are in the BST_INDETERMINATE state
#ifdef DEBUG
    // the pfpsp struct should have been zero inited, make sure that our ATTRIBUTESTATE 
    // structs are zero inited

    ATTRIBUTESTATE asTemp = {0};

    ASSERT(memcmp(&pfpsp->asInitial, &asTemp, SIZEOF(pfpsp->asInitial)) == 0);
#endif // DEBUG
    
    // set the inital state based on the flags
    if (dwTriState & FILE_ATTRIBUTE_READONLY)
        pfpsp->asInitial.fReadOnly = BST_INDETERMINATE;
    else if (dwFlagsAND & FILE_ATTRIBUTE_READONLY)
        pfpsp->asInitial.fReadOnly = BST_CHECKED;

    if (dwTriState & FILE_ATTRIBUTE_HIDDEN)
        pfpsp->asInitial.fHidden = BST_INDETERMINATE;
    else if (dwFlagsAND & FILE_ATTRIBUTE_HIDDEN)
        pfpsp->asInitial.fHidden = TRUE;

    if (dwTriState & FILE_ATTRIBUTE_ARCHIVE)
        pfpsp->asInitial.fArchive = BST_INDETERMINATE;
    else if (dwFlagsAND & FILE_ATTRIBUTE_ARCHIVE)
        pfpsp->asInitial.fArchive = TRUE;

    if (dwTriState & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED)
        pfpsp->asInitial.fIndex = BST_INDETERMINATE;
    else if (!(dwFlagsAND & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED))
        pfpsp->asInitial.fIndex = TRUE;

    if (dwTriState & FILE_ATTRIBUTE_COMPRESSED)
        pfpsp->asInitial.fCompress = BST_INDETERMINATE;
    else if (dwFlagsAND & FILE_ATTRIBUTE_COMPRESSED)
        pfpsp->asInitial.fCompress = TRUE;

    if (dwTriState & FILE_ATTRIBUTE_ENCRYPTED)
        pfpsp->asInitial.fEncrypt = BST_INDETERMINATE;
    else if (dwFlagsAND & FILE_ATTRIBUTE_ENCRYPTED)
        pfpsp->asInitial.fEncrypt = TRUE;
}


//
// Updates the size fields for single and multiple file property sheets.
//
// NOTE: if you have the the WIN32_FIND_DATA already, then pass it for perf
//
void UpdateSizeField(FILEPROPSHEETPAGE* pfpsp, WIN32_FIND_DATA* pfd)
{
    WIN32_FIND_DATA wfd;

    if (pfpsp->fMultipleFiles)
    {
        // multiple selection case
        // create the size and # of files thread
        CreateSizeThread(pfpsp);
    }
    else
    {
        // if the caller didn't pass pfd, then go get the WIN32_FIND_DATA now
        if (!pfd)
        {
            HANDLE hFind = FindFirstFile(pfpsp->szPath, &wfd);

            if (hFind == INVALID_HANDLE_VALUE)
            {
                // if this failed we should clear out all the values as to not show garbage on the screen.
                ZeroMemory(&wfd, SIZEOF(wfd));
            }
            else
            {
                FindClose(hFind);
            }

            pfd = &wfd;
        }

        if (pfpsp->fMountedDrive)
        {
            // mounted drive case
            SetDateTimeText(pfpsp->hDlg, IDD_CREATED, &pfd->ftCreationTime);
        }
        else if (pfpsp->fIsDirectory)
        {
            // single folder case
            SetDateTimeText(pfpsp->hDlg, IDD_CREATED, &pfd->ftCreationTime);

            // create the size and # of files thread
            CreateSizeThread(pfpsp);
        }
        else
        {
            TCHAR szNum1[MAX_COMMA_AS_K_SIZE];
            TCHAR szNum2[MAX_COMMA_NUMBER_SIZE];
            LPTSTR pszFmt;
            ULARGE_INTEGER ulSize;
            DWORD dwClusterSize = PathGetClusterSize(pfpsp->szPath);

            // fill in the "Size:" field
            ulSize.LowPart  = pfd->nFileSizeLow;
            ulSize.HighPart = pfd->nFileSizeHigh;

            pszFmt = ShellConstructMessageString(HINST_THISDLL, 
                                                 MAKEINTRESOURCE(ulSize.QuadPart ? IDS_SIZEANDBYTES : IDS_SIZE),
                                                 ShortSizeFormat64(ulSize.QuadPart, szNum1),
                                                 AddCommas64(ulSize.QuadPart, szNum2));
            if (pszFmt)
            {
                SetDlgItemText(pfpsp->hDlg, IDD_FILESIZE, pszFmt);
                LocalFree(pszFmt);
            }

            //
            // fill in the "Size on disk:" field
            //
#ifdef WINNT
            if (pfd->dwFileAttributes & (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE))
            {
                // the file is compressed or sparse, so for "size on disk" use the compressed size
                ulSize.LowPart = SHGetCompressedFileSize(pfpsp->szPath, &ulSize.HighPart);
            }
            else
#endif
            {
                // the file isint comrpessed so just round to the cluster size for the "size on disk"
                ulSize.LowPart = pfd->nFileSizeLow;
                ulSize.HighPart = pfd->nFileSizeHigh;
                ulSize.QuadPart = ROUND_TO_CLUSER(ulSize.QuadPart, dwClusterSize);
            }

            pszFmt = ShellConstructMessageString(HINST_THISDLL, 
                                                 MAKEINTRESOURCE(ulSize.QuadPart ? IDS_SIZEANDBYTES : IDS_SIZE),
                                                 ShortSizeFormat64(ulSize.QuadPart, szNum1),
                                                 AddCommas64(ulSize.QuadPart, szNum2));
            if (pszFmt && !pfpsp->fMountedDrive)
            {
                SetDlgItemText(pfpsp->hDlg, IDD_FILESIZE_COMPRESSED, pszFmt);
                LocalFree(pszFmt);
            }

            //
            // BUGBUG (reinerf)
            //
            // we always touch the file in the process of getting its info, so the
            // ftLastAccessTime is always TODAY, which makes this field pretty useless...

            // date and time
            SetDateTimeText(pfpsp->hDlg, IDD_CREATED,      &pfd->ftCreationTime);
            SetDateTimeText(pfpsp->hDlg, IDD_LASTMODIFIED, &pfd->ftLastWriteTime);
#ifdef WINNT
            {
                //  NT's FAT/FAT32 implementation doesn't support last accessed time (gets the date right, but not the time),
                //  so we won't display it
                DWORD dwFlags = FDTF_LONGDATE | FDTF_RELATIVE;

                if((lstrcmpi(pfpsp->szFileSys, TEXT("FAT")) != 0) &&
                   (lstrcmpi(pfpsp->szFileSys, TEXT("FAT32")) != 0))
                {
                    dwFlags |= FDTF_LONGTIME;
                }
            
                SetDateTimeTextEx(pfpsp->hDlg, IDD_LASTACCESSED, &pfd->ftLastAccessTime, dwFlags);
            }
#else
            // on win9x we just use the whole time/date for last accessed
            SetDateTimeText(pfpsp->hDlg, IDD_LASTACCESSED, &pfd->ftLastAccessTime);
#endif // WINNT
        }
    }        
}


//
// Descriptions:
//   This function fills fields of the multiple object property sheet.
//
BOOL InitMultiplePrsht(FILEPROPSHEETPAGE* pfpsp)
{
    SHFILEINFO sfi;
    TCHAR szBuffer[MAX_PATH+1];
    TCHAR szType[MAX_PATH];
    TCHAR szDirPath[MAX_PATH];
    int iItem;
    BOOL fMultipleType = FALSE;
    BOOL fSameLocation = TRUE;
    DWORD dwFlagsOR = 0;                // start all clear
    DWORD dwFlagsAND = (DWORD)-1;       // start all set
#ifdef WINNT
    DWORD dwVolumeFlagsAND = (DWORD)-1; // start all set
#endif

    szDirPath[0] = 0;
    szType[0] = 0;


    // For all the selected files compare their types and get their attribs
    for (iItem = 0; HIDA_FillFindData(pfpsp->hida, iItem, szBuffer, NULL, FALSE); iItem++)
    {
        DWORD dwFileAttributes = GetFileAttributes(szBuffer);

        dwFlagsAND &= dwFileAttributes;
        dwFlagsOR  |= dwFileAttributes;

        // process types only if we haven't already found that there are several types
        if (!fMultipleType)
        {
            SHGetFileInfo((LPTSTR)IDA_GetIDListPtr((LPIDA)GlobalLock(pfpsp->hida), iItem), 0,
                &sfi, SIZEOF(sfi), SHGFI_PIDL|SHGFI_TYPENAME);

            if (szType[0] == TEXT('\0'))
                lstrcpy(szType, sfi.szTypeName);
            else
                fMultipleType = lstrcmp(szType, sfi.szTypeName) != 0;
        }

#ifdef WINNT
        dwVolumeFlagsAND &= GetVolumeFlags(szBuffer, pfpsp->szFileSys, ARRAYSIZE(pfpsp->szFileSys));
#endif
        // check to see if the files are in the same location
        if (fSameLocation)
        {
            PathRemoveFileSpec(szBuffer);

            if (szDirPath[0] == TEXT('\0'))
                lstrcpy(szDirPath, szBuffer);
            else
                fSameLocation = (lstrcmpi(szDirPath, szBuffer) == 0);
        }
    }

#ifdef WINNT
    if ((dwVolumeFlagsAND & FS_FILE_ENCRYPTION) && AllowedToEncrypt())
    {
        // all the files are on volumes that support encryption (eg NTFS)
        pfpsp->fIsEncryptionAvailable = TRUE;
    }
    
    if (dwVolumeFlagsAND & FS_FILE_COMPRESSION)
    {
        pfpsp->fIsCompressionAvailable = TRUE;
    }

    //
    // BUGBUG (reinerf) - we dont have a FS_SUPPORTS_INDEXING so we 
    // use the FILE_SUPPORTS_SPARSE_FILES flag, because native index support
    // appeared first on NTFS5 volumes, at the same time sparse file support
    // was implemented.
    //
    if (dwVolumeFlagsAND & FILE_SUPPORTS_SPARSE_FILES)
    {
        // yup, we are on NTFS5 or greater
        pfpsp->fIsIndexAvailable = TRUE;
    }
#endif // WINNT


    // if any of the files was a directory, then we set this flag
    if (dwFlagsOR & FILE_ATTRIBUTE_DIRECTORY)
    {
        pfpsp->fIsDirectory = TRUE;
    }

    // setup all the flags based on what we found out
    SetInitialFileAttribs(pfpsp, dwFlagsAND, dwFlagsOR);

    // set the current attributes to the same as the initial
    pfpsp->asCurrent = pfpsp->asInitial;

    //
    // now setup all the controls on the dialog based on the attribs
    // that we have
    //

    // check for multiple file types
    if (fMultipleType)
    {
        LoadString(HINST_THISDLL, IDS_MULTIPLETYPES, szBuffer, ARRAYSIZE(szBuffer));
    }
    else
    {
        LoadString(HINST_THISDLL, IDS_ALLOFTYPE, szBuffer, ARRAYSIZE(szBuffer));
        lstrcat(szBuffer, szType);
    }
    SetDlgItemText(pfpsp->hDlg, IDD_FILETYPE, szBuffer);

    if (fSameLocation)
    {
        LoadString(HINST_THISDLL, IDS_ALLIN, szBuffer, ARRAYSIZE(szBuffer));
        lstrcat(szBuffer, szDirPath);
        lstrcpy(pfpsp->szPath, szDirPath);

        AddLocationToolTips(pfpsp->hDlg, IDD_LOCATION, szDirPath, &pfpsp->hwndTip);
    }
    else
    {
        LoadString(HINST_THISDLL, IDS_VARFOLDERS, szBuffer, ARRAYSIZE(szBuffer));
    }
    //Keep Functionality same as NT4 by avoiding PathCompactPath. 
    SetDlgItemText(pfpsp->hDlg, IDD_LOCATION, szBuffer);


    //
    // check the ReadOnly and Hidden checkboxes, they always appear on the general tab
    //
    if (pfpsp->asInitial.fReadOnly == BST_INDETERMINATE)
    {
        SendDlgItemMessage(pfpsp->hDlg, IDD_READONLY, BM_SETSTYLE, BS_AUTO3STATE, 0);
    }
    CheckDlgButton(pfpsp->hDlg, IDD_READONLY, pfpsp->asCurrent.fReadOnly);

    if (pfpsp->asInitial.fHidden == BST_INDETERMINATE)
    {
        SendDlgItemMessage(pfpsp->hDlg, IDD_HIDDEN, BM_SETSTYLE, BS_AUTO3STATE, 0);
    }
    CheckDlgButton(pfpsp->hDlg, IDD_HIDDEN, pfpsp->asCurrent.fHidden);

    // to avoid people making SYSTEM files HIDDEN (SYSTEM HIDDEN files are
    // never show to the user) we don't let people make SYSTEM files HIDDEN
    if (dwFlagsOR & FILE_ATTRIBUTE_SYSTEM)
        EnableWindow(GetDlgItem(pfpsp->hDlg, IDD_HIDDEN), FALSE);

    // Archive is only on the general tab for FAT, otherwise it is under the "Advanced attributes"
    // and FAT volumes dont have the "Advanced attributes" button.
    if (!pfpsp->fIsCompressionAvailable)
    {
        // we are on FAT/FAT32, so get rid of the "Advanced attributes" button, and set the inital Archive state
        DestroyWindow(GetDlgItem(pfpsp->hDlg, IDC_ADVANCED));

        if (pfpsp->asInitial.fArchive == BST_INDETERMINATE)
        {
            SendDlgItemMessage(pfpsp->hDlg, IDD_ARCHIVE, BM_SETSTYLE, BS_AUTO3STATE, 0);
        }
        CheckDlgButton(pfpsp->hDlg, IDD_ARCHIVE, pfpsp->asCurrent.fArchive);
    }
    else
    {
        // if compression is available, then we must be on NTFS
        // get rid of the check box, leave the button
        DestroyWindow(GetDlgItem(pfpsp->hDlg, IDD_ARCHIVE));
    }

    UpdateSizeField(pfpsp, NULL);

    return TRUE;
}

//
// Descriptions:
//   Callback for the property sheet code
//
UINT CALLBACK FilePrshtCallback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    if (uMsg == PSPCB_RELEASE)
    {
        FILEPROPSHEETPAGE * pfpsp = (FILEPROPSHEETPAGE *)ppsp;

        //
        // Make sure thread is dead before we delete it's data...
        //
        KillSizeThread(pfpsp);

        if (pfpsp->hida)
        {
            LocalFree(pfpsp->hida);
            pfpsp->hida = NULL;

            ILFree(pfpsp->pidlFolder);
            pfpsp->pidlFolder = NULL;

            ILFree(pfpsp->pidlTarget);
            pfpsp->pidlFolder = NULL;
        }
    }

    return 1;
}

#ifdef WINNT
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: OpenFileForCompress
//
// DESCRIPTION:
//
//   Opens the file for compression.  It handles the case where a READONLY
//   file is trying to be compressed or uncompressed.  Since read only files
//   cannot be opened for WRITE_DATA, it temporarily resets the file to NOT
//   be READONLY in order to open the file, and then sets it back once the
//   file has been compressed.
//
//   Taken from WinFile module wffile.c without change.  Originally from
//   G. Kimura's compact.c. Now taken from shcompui without change.
//
// ARGUMENTS:
//
//   phFile
//      Address of file handle variable for handle of open file if
//      successful.
//
//   szFile
//      Name string of file to be opened.
//
// RETURNS:
//
//    TRUE  = File successfully opened.  Handle in *phFile.
//    FALSE = File couldn't be opened. *phFile == INVALID_HANDLE_VALUE
//
///////////////////////////////////////////////////////////////////////////////
BOOL OpenFileForCompress(
    PHANDLE phFile,
    LPCTSTR szFile)
{
    HANDLE hAttr;
    BY_HANDLE_FILE_INFORMATION fi;

    //
    //  Try to open the file - READ_DATA | WRITE_DATA.
    //
    if ((*phFile = CreateFile( szFile,
                               FILE_READ_DATA | FILE_WRITE_DATA,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL )) != INVALID_HANDLE_VALUE)
    {
        //
        //  Successfully opened the file.
        //
        return (TRUE);
    }

    if (GetLastError() != ERROR_ACCESS_DENIED)
    {
        return (FALSE);
    }

    //
    //  Try to open the file - READ_ATTRIBUTES | WRITE_ATTRIBUTES.
    //
    if ((hAttr = CreateFile( szFile,
                             FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                             NULL )) == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    //
    //  See if the READONLY attribute is set.
    //
    if ( (!GetFileInformationByHandle(hAttr, &fi)) ||
         (!(fi.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) )
    {
        //
        //  If the file could not be open for some reason other than that
        //  the readonly attribute was set, then fail.
        //
        CloseHandle(hAttr);
        return (FALSE);
    }

    //
    //  Turn OFF the READONLY attribute.
    //
    fi.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
    if (!SetFileAttributes(szFile, fi.dwFileAttributes))
    {
        CloseHandle(hAttr);
        return (FALSE);
    }

    //
    //  Try again to open the file - READ_DATA | WRITE_DATA.
    //
    *phFile = CreateFile( szFile,
                          FILE_READ_DATA | FILE_WRITE_DATA,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                          NULL );

    //
    //  Close the file handle opened for READ_ATTRIBUTE | WRITE_ATTRIBUTE.
    //
    CloseHandle(hAttr);

    //
    //  Make sure the open succeeded.  If it still couldn't be opened with
    //  the readonly attribute turned off, then fail.
    //
    if (*phFile == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    //
    //  Turn the READONLY attribute back ON.
    //
    fi.dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
    if (!SetFileAttributes(szFile, fi.dwFileAttributes))
    {
        CloseHandle(*phFile);
        *phFile = INVALID_HANDLE_VALUE;
        return (FALSE);
    }

    //
    //  Return success.  A valid file handle is in *phFile.
    //
    return (TRUE);
}

//
//  This function encrypts/decrypts a file. If the readonly bit is set, the 
//  function will clear it and encrypt/decrypt and then set the RO bit back
//
//  szPath      a string that has the full path to the file
//  fCompress   TRUE  - compress the file
//              FALSE - decompress the file
//
//
//  return:     TRUE  - the file was sucessfully encryped/decryped
//              FALSE - the file could not be encryped/decryped
//
STDAPI_(BOOL) SHEncryptFile(LPCTSTR pszPath, BOOL fEncrypt)
{
    DWORD dwAttribs = GetFileAttributes(pszPath);
    BOOL fUnsetReadonly = FALSE;
    BOOL bRet = FALSE;


    // We will fail if the file is encryped, so check for that case
    if (dwAttribs & FILE_ATTRIBUTE_READONLY)
    {
        if (SetFileAttributes(pszPath, dwAttribs & ~FILE_ATTRIBUTE_READONLY))
        {
            fUnsetReadonly = TRUE;
        }
    }
    
    if (fEncrypt)
    {
        bRet = EncryptFile(pszPath);
    }
    else
    {
        bRet = DecryptFile(pszPath, 0);
    }

    if (fUnsetReadonly)
    {
        DWORD dwLastError = ERROR_SUCCESS;

        // need to set the readonly bit back, but preserve the lasterror value as well
        if (!bRet)
        {
            dwLastError = GetLastError();
        }

        SetFileAttributes(pszPath, dwAttribs);

        if (!bRet)
        {
            ASSERT(dwLastError != ERROR_SUCCESS);
            SetLastError(dwLastError);
        }
    }

    return bRet;
}

//
//  This function compresses/uncompresses a file.
//
//  szPath      a string that has the full path to the file
//  fCompress   TRUE  - compress the file
//              FALSE - decompress the file
//
//
//  return:     TRUE  - the file was sucessfully compressed/uncompressed
//              FALSE - the file could not be compressed/uncompressed
//
BOOL CompressFile(LPCTSTR pzsPath, BOOL fCompress)
{
    HANDLE hFile;
    USHORT uState;
    ULONG Length;
    DWORD dwAttribs = GetFileAttributes(pzsPath);


    
    if (dwAttribs & FILE_ATTRIBUTE_ENCRYPTED)
    {
        // We will fail to compress/decompress the file if is encryped. We don't want
        // to bother the user w/ error messages in this case (since encryption "takes 
        // presidence" over compression), so we just return success.
        return TRUE;
    }

    if (OpenFileForCompress(&hFile, pzsPath))
    {
        BOOL bRet;

        uState = fCompress ? COMPRESSION_FORMAT_DEFAULT : COMPRESSION_FORMAT_NONE;
        
        bRet = DeviceIoControl(hFile,
                               FSCTL_SET_COMPRESSION,
                               &uState,
                               sizeof(USHORT),
                               NULL,
                               0,
                               &Length,
                               FALSE);
        CloseHandle(hFile);
        return bRet;
    }
    else
    {
        // couldnt get a file handle
        return FALSE;
    }
}
#endif // WINNT


BOOL IsValidFileName(LPCTSTR pszFileName)
{
    LPCTSTR psz = pszFileName;

    if (!pszFileName || !pszFileName[0])
    {
        return FALSE;
    }

    do
    {
        // we are only passed the file name, so its ok to use PIVC_LFN_NAME
        if (!PathIsValidChar(*psz, PIVC_LFN_NAME))    
        {
            // found a non-legal character
            return FALSE;
        }

        psz = CharNext(psz);
    }
    while (*psz);

    // didn't find any illegal characters
    return TRUE;
}


// renames the file, or checks to see if it could be renamed if fCommit == FALSE
BOOL ApplyRename(FILEPROPSHEETPAGE* pfpsp, BOOL fCommit)
{
    LPCTSTR pszOldName;
    TCHAR szNewName[MAX_PATH];
    TCHAR szDir[MAX_PATH];

    ASSERT(pfpsp->fRename);

    Edit_GetText(GetDlgItem(pfpsp->hDlg, IDD_NAMEEDIT), szNewName, ARRAYSIZE(szNewName));

    if (lstrcmp(pfpsp->szInitialName, szNewName) != 0)
    {
        // the name could be changed from C:\foo.txt to C:\FOO.txt, this is
        // technically the same name to PathFileExists, but we should allow it
        // anyway
        BOOL fCaseChange = (lstrcmpi(pfpsp->szInitialName, szNewName) == 0);
        
        // get the dir where the file lives
        lstrcpy(szDir, pfpsp->szPath);
        PathRemoveFileSpec(szDir);

        // find out the old name with the extension (we cant use pfpsp->szInitialName here,
        // because it might not have had the extension depending on the users view|options settings)
        pszOldName = PathFindFileName(pfpsp->szPath);

        if (!pfpsp->fShowExtension)
        {
            // the extension is hidden, so add it to the new path the user typed
            LPCTSTR pszExt = PathFindExtension(pfpsp->szPath);
            if (*pszExt)
            {
                // Note that we can't call PathAddExtension, because it removes the existing extension.
                lstrcatn(szNewName, pszExt, ARRAYSIZE(szNewName));
            }
        }

        // is this a test or is it the real thing? (test needed so we can put up error UI before we get
        // the PSN_LASTCHANCEAPPLY)
        if (fCommit)
        {
            if (SHRenameFileEx(pfpsp->hDlg, NULL, szDir, pszOldName, szNewName, FALSE) != ERROR_SUCCESS)
            {
                // dont need error ui because SHRenameFile takes care of that for us.
                return FALSE;
            }
            else
            {
                SHChangeNotify(SHCNE_RENAMEITEM, SHCNF_FLUSH | SHCNF_PATH, pszOldName, szNewName);
            }
        }
        else
        {
            TCHAR szNewPath[MAX_PATH];
            PathCombine(szNewPath, szDir, szNewName);

            if (!IsValidFileName(szNewName) || (PathFileExists(szNewPath) && !fCaseChange))
            {
                LRESULT lRet;
                lRet = SHRenameFileEx(pfpsp->hDlg, NULL, szDir, pszOldName, szNewName, FALSE);

                if (lRet == ERROR_SUCCESS)
                {
                    // Whoops, I guess we really CAN rename the file (this case can happen if the user
                    // tries to add a whole bunch of .'s to the end of a folder name).
                    
                    // Rename it back so we can succeed when we call this fn. again with fCommit = TRUE;
                    lRet = SHRenameFileEx(NULL, NULL, szDir, szNewName, pszOldName, FALSE);
                    ASSERT(lRet == ERROR_SUCCESS);
                    
                    return TRUE;
                }

                // SHRenameFileEx put up the error UI for us, so just return false.
                return FALSE;
            }
        }
        // we dont bother doing anything if the rename succeeded since we only do renames 
        // if the dialog is about to close (user hit "ok")
    }
    return TRUE;
}


//
// this is the dlg proc for Attribute Errors
//
//   returns
//
//      IDCANCEL                - user clicked abort
//      IDRETRY                 - user clicked retry
//      IDIGNORE                - user clicked ignore
//      IDIGNOREALL             - user clikced ignore all
//
BOOL_PTR CALLBACK FailedApplyAttribDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            RECT rc;
            TCHAR szErrorMsg[MAX_PATH];
            TCHAR szTemplate[MAX_PATH];
            TCHAR szPath[MAX_PATH];
            ATTRIBUTEERROR* pae = (ATTRIBUTEERROR*)lParam;

            lstrcpyn(szPath, pae->pszPath, ARRAYSIZE(szPath));

            // Modify very long path names so that they fit into the message box.
            // get the size of the text boxes
            GetWindowRect(GetDlgItem(hDlg, IDD_NAME), &rc);
            PathCompactPath(NULL, szPath, rc.right - rc.left);

            SetDlgItemText(hDlg, IDD_NAME, szPath);
            
            // Default message if FormatMessage doesn't recognize dwLastError
            LoadString(HINST_THISDLL, IDS_UNKNOWNERROR, szTemplate, ARRAYSIZE(szTemplate));
            wnsprintf(szErrorMsg, ARRAYSIZE(szErrorMsg), szTemplate, pae->dwLastError);

            // Try the system error message
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, pae->dwLastError, 0L, szErrorMsg, ARRAYSIZE(szErrorMsg), NULL);

            SetDlgItemText(hDlg, IDD_ERROR_TXT, szErrorMsg);
            EnableWindow(hDlg, TRUE);
            break;
        }

        case WM_COMMAND:
        {
            WORD wControlID = GET_WM_COMMAND_ID(wParam, lParam);
            switch (wControlID)
            {
                case IDIGNOREALL:   // = 10  (this comes from shell32.rc, the rest come from winuser.h)
                case IDCANCEL:      // = 2
                case IDRETRY:       // = 4
                case IDIGNORE:      // = 5
                    EndDialog(hDlg, wControlID);
                    return TRUE;
                    break;

                default:
                    return FALSE;
            }
            break;
        }
        default :
            return FALSE;
    }
    return FALSE;
}


//
// This function displays the "and error has occured [abort] [retry] [ignore] [ignore all]" message
// If the user hits abort, then we return FALSE so that our caller knows to abort the operation
//
//  returns the id of the button pressed (one of: IDIGNOREALL, IDIGNORE, IDCANCEL, IDRETRY)
//
int FailedApplyAttribsErrorDlg(HWND hWndParent, ATTRIBUTEERROR* pae)
{
    int iRet;

    //  Put up the error message box - ABORT, RETRY, IGNORE, IGNORE ALL.
    iRet = (int)DialogBoxParam(HINST_THISDLL,
                          MAKEINTRESOURCE(DLG_ATTRIBS_ERROR),
                          hWndParent,
                          FailedApplyAttribDlgProc,
                          (LPARAM)pae);
    //
    // if the user hits the ESC key or the little X thingy, then 
    // iRet = 0, so we set iRet = IDCANCEL
    //
    if (!iRet)
    {
        iRet = IDCANCEL;
    }

    return iRet;
}


//
// we check to see if this is a known bad file that we skip applying attribs to
//
BOOL IsBadAttributeFile(LPCTSTR pszFile, FILEPROPSHEETPAGE* pfpsp)
{
    int i;
    LPTSTR pszFileName = PathFindFileName(pszFile);

    const static LPTSTR s_rgszBadFiles[] = {
        {TEXT("pagefile.sys")},
        {TEXT("ntldr")},
        {TEXT("ntdetect.com")},
        {TEXT("explorer.exe")},
        {TEXT("System Volume Information")},
        {TEXT("cmldr")},
        {TEXT("desktop.ini")},
        {TEXT("ntuser.dat")},
        {TEXT("ntuser.dat.log")},
        {TEXT("ntuser.pol")},
        {TEXT("usrclass.dat")},
        {TEXT("usrclass.dat.log")}};
    
    for (i = 0; i < ARRAYSIZE(s_rgszBadFiles); i++)
    {
        if (lstrcmpi(s_rgszBadFiles[i], pszFileName) == 0)
        {
            // this file matched on of the "bad" files that we dont apply attributes to
            return TRUE;
        }
    }

    // ok to muck with this file
    return FALSE;
}


#ifdef WINNT // encryption stuff only supported on NT
//
// This is the encryption warning callback dlg proc
//
BOOL_PTR CALLBACK EncryptionWarningDlgProc(HWND hDlgWarning, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LPCTSTR pszPath = (LPCTSTR)lParam;
            
            SetWindowPtr(hDlgWarning, DWLP_USER, (void*) pszPath);

            // set the initial state of the radio buttons
            CheckDlgButton(hDlgWarning, IDC_ENCRYPT_PARENTFOLDER, TRUE);
            break;
        }

        case WM_COMMAND:
        {
            if ((LOWORD(wParam) == IDOK) && (IsDlgButtonChecked(hDlgWarning, IDC_ENCRYPT_PARENTFOLDER) == BST_CHECKED))
            {
                LPTSTR pszPath = (LPTSTR) GetWindowPtr(hDlgWarning, DWLP_USER);

                if (pszPath)
                {
                    LPITEMIDLIST pidl = ILCreateFromPath(pszPath);

                    if (pidl)
                    {
                        SHChangeNotifySuspendResume(TRUE, pidl, TRUE, 0);
                    }

RetryEncryptParentFolder:
                    if (!EncryptFile(pszPath))
                    {
                        ATTRIBUTEERROR ae = {pszPath, GetLastError()};

                        if (FailedApplyAttribsErrorDlg(hDlgWarning, &ae) == IDRETRY)
                        {
                            goto RetryEncryptParentFolder;
                        }
                    }

                    if (pidl)
                    {
                        SHChangeNotifySuspendResume(FALSE, pidl, TRUE, 0);
                        ILFree(pidl);
                    }
                }
            }
            break;
        }
    }

    // we want the MessageBoxCheckExDlgProc have a crack at everything as well,
    // so return false here
    return FALSE;
}

//
// This function warns the user that they are encrypting a file that is not in and encrypted
// folder. Most editors (MS word included), do a "safe-save" where they rename the file being
// edited, and then save the new modified version out, and then delete the old original. This 
// causes an encrypted document that is NOT in an encrypted folder to become decrypted so we
// warn the user here.
//
// returns:
//          TRUE  - the user hit "ok" (either compress just the file, or the parent folder as well)
//          FALSE - the user hit "cancel"
//
int WarnUserAboutDecryptedParentFolder(LPCTSTR pszPath, HWND hWndParent)
{
    int iRet = IDOK; // assume everything is okidokey
    TCHAR szParentFolder[MAX_PATH];
    DWORD dwAttribs;

    // check for the root case (no parent), or the directory case
    if (PathIsRoot(pszPath) || PathIsDirectory(pszPath))
        return TRUE;

    // first check to see if the parent folder is encrypted
    lstrcpyn(szParentFolder, pszPath, ARRAYSIZE(szParentFolder));
    PathRemoveFileSpec(szParentFolder);

    dwAttribs = GetFileAttributes(szParentFolder);
    
    if ((dwAttribs != (DWORD)-1) && !(dwAttribs & FILE_ATTRIBUTE_ENCRYPTED) && !PathIsRoot(szParentFolder))
    {
        // the parent folder is NOT encrypted and the parent folder isin't the root, so warn the user
        iRet = SHMessageBoxCheckEx(hWndParent, HINST_THISDLL, MAKEINTRESOURCE(DLG_ENCRYPTWARNING), EncryptionWarningDlgProc,
                                  (LPVOID)szParentFolder, IDOK, TEXT("EncryptionWarning"));
    }

    return (iRet == IDOK);
}


#endif // WINNT


//
// Sets attributes of a file based on the info in pfpsp
//
//  szFilename  -  the name of the file to compress
// 
//  pfpsp       -  the filepropsheetpage info
//
//  hWndParent  -  Parent hwnd in case we need to put up some ui
//
//  pbSomethingChanged - pointer to a bool that says whether or not something actually was
//                       changed during the operation.
//                       TRUE  - we applied at leaset one attribute
//                       FALSE - we didnt change anything (either an error or all the attribs already matched)
// 
//  return value: TRUE  - the operation was sucessful
//                FALSE - there was an error and the user hit cancel to abort the operation 
//
//
// NOTE:    the caller of this function must take care of generating the SHChangeNotifies so that
//          we dont end up blindly sending them for every file in a dir (the caller will send
//          one for just that dir). That is why we have the pbSomethingChanged variable.
//
STDAPI_(BOOL) ApplyFileAttributes(LPCTSTR pszPath, FILEPROPSHEETPAGE* pfpsp, HWND hWndParent, BOOL* pbSomethingChanged)
{
    DWORD dwInitialAttributes;
    DWORD dwNewAttributes = 0;
    DWORD dwLastError = ERROR_SUCCESS;
    BOOL ShouldCallSetFileAttributes = FALSE;
    BOOL bAlreadyWarnedUserAboutEncryption = FALSE;
    BOOL  bIsSuperHidden = FALSE;
    LPITEMIDLIST pidl = NULL;
 
    // assume nothing changed to start with
    *pbSomethingChanged = 0;
    
    if ((pfpsp->fRecursive || pfpsp->fMultipleFiles) && IsBadAttributeFile(pszPath, pfpsp))
    {
        // we are doing a recursive operation or a multiple file operation, so we skip files
        // that we dont want to to mess with because they will ususally give error dialogs
        if (pfpsp->pProgressDlg)
        {
            // since we are skipping this file, we subtract its size from both
            // ulTotal and ulCompleted. This will make sure the progress bar isint
            // messed up by files like pagefile.sys who are huge but get "compressed"
            // in milliseconds.
            ULARGE_INTEGER ulTemp;

            ulTemp.LowPart = pfpsp->fd.nFileSizeLow;
            ulTemp.HighPart = pfpsp->fd.nFileSizeHigh;

            // guard against underflow
            if (pfpsp->ulNumberOfBytesDone.QuadPart < ulTemp.QuadPart)
            {
                pfpsp->ulNumberOfBytesDone.QuadPart = 0;
            }
            else
            {
                pfpsp->ulNumberOfBytesDone.QuadPart -= ulTemp.QuadPart;
            }

            pfpsp->ulTotalNumberOfBytes.QuadPart -= ulTemp.QuadPart;

            UpdateProgressBar(pfpsp);
        }

        // return telling the user everying is okidokey
        return TRUE;
    }

RetryApplyAttribs:
    dwInitialAttributes = GetFileAttributes(pszPath);

    if (dwInitialAttributes == -1)
    {
        // we were unable to get the file attribues, doh!
        dwLastError = GetLastError();
        goto RaiseErrorMsg;
    }

    if (pfpsp->pProgressDlg)
    {
        // update the progress dialog file name
        SetProgressDlgPath(pfpsp, pszPath, TRUE);
    }
        
    //
    // we only allow attribs that SetFileAttributes can handle
    //
    dwNewAttributes = (dwInitialAttributes & (FILE_ATTRIBUTE_READONLY               | 
                                              FILE_ATTRIBUTE_HIDDEN                 | 
                                              FILE_ATTRIBUTE_ARCHIVE                |
                                              FILE_ATTRIBUTE_OFFLINE                |
                                              FILE_ATTRIBUTE_SYSTEM                 |
                                              FILE_ATTRIBUTE_TEMPORARY              |
                                              FILE_ATTRIBUTE_NOT_CONTENT_INDEXED));

    bIsSuperHidden = ((dwNewAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) == (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN));

    if (pfpsp->asInitial.fReadOnly != pfpsp->asCurrent.fReadOnly)
    {
        BOOL bCanChangeReadOnly = TRUE;
        
        // Don't change read only on folders with desktop ini
        if ((dwNewAttributes & FILE_ATTRIBUTE_READONLY) && PathIsDirectory(pszPath))
        {
            TCHAR szDIPath[MAX_PATH];

            PathCombine(szDIPath, pszPath, TEXT("desktop.ini"));
            bCanChangeReadOnly = !PathFileExistsAndAttributes(szDIPath, NULL);
        }
        
        if (bCanChangeReadOnly)
        {
            if (pfpsp->asCurrent.fReadOnly)
                dwNewAttributes |= FILE_ATTRIBUTE_READONLY;
            else
                dwNewAttributes &= ~FILE_ATTRIBUTE_READONLY;

            ShouldCallSetFileAttributes = TRUE;
        }
    }

    //
    // don't allow setting of hidden on system files, as this will make them disappear for good.
    //
    if (pfpsp->asInitial.fHidden != pfpsp->asCurrent.fHidden && !(dwNewAttributes & FILE_ATTRIBUTE_SYSTEM))
    {
        if (pfpsp->asCurrent.fHidden)
            dwNewAttributes |= FILE_ATTRIBUTE_HIDDEN;
        else
            dwNewAttributes &= ~FILE_ATTRIBUTE_HIDDEN;
            
        ShouldCallSetFileAttributes = TRUE;
    }
    
    if (pfpsp->asInitial.fArchive != pfpsp->asCurrent.fArchive)
    {
        if (pfpsp->asCurrent.fArchive)
            dwNewAttributes |= FILE_ATTRIBUTE_ARCHIVE;
        else
            dwNewAttributes &= ~FILE_ATTRIBUTE_ARCHIVE;
        
        ShouldCallSetFileAttributes = TRUE;
    }

    if (pfpsp->asInitial.fIndex != pfpsp->asCurrent.fIndex)
    {
        if (pfpsp->asCurrent.fIndex)
            dwNewAttributes &= ~FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
        else
            dwNewAttributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
        
        ShouldCallSetFileAttributes = TRUE;
    }

    // did something change that we need to call SetFileAttributes for?
    if (ShouldCallSetFileAttributes)
    {
        if (SetFileAttributes(pszPath, dwNewAttributes))
        {
            // success! set fSomethingChanged so we know to send out
            // a changenotify
            *pbSomethingChanged = TRUE;
        }
        else
        {
            // get the last error value now so we know why it failed
            dwLastError = GetLastError();
            goto RaiseErrorMsg;
        }
    }

#ifdef WINNT

    // We need to be careful about the order we compress/encrypt in since these
    // operations are mutually exclusive. 
    // We therefore do the uncompressing/decrypting first
    if ((pfpsp->asInitial.fCompress != pfpsp->asCurrent.fCompress) && 
        (pfpsp->asCurrent.fCompress == BST_UNCHECKED))
    {
        if (!CompressFile(pszPath, FALSE))
        {
            // get the last error value now so we know why it failed
            dwLastError = GetLastError();
            goto RaiseErrorMsg;
        }
        else
        {
            // success
            *pbSomethingChanged = TRUE;
        }
    }

    if ((pfpsp->asInitial.fEncrypt != pfpsp->asCurrent.fEncrypt) && 
        (pfpsp->asCurrent.fEncrypt == BST_UNCHECKED))
    {
        BOOL fSucceeded = SHEncryptFile(pszPath, FALSE); // try to decrypt the file
        
        if (!fSucceeded)
        {
            // get the last error value now so we know why it failed
            dwLastError = GetLastError();

            if (ERROR_SHARING_VIOLATION == dwLastError)
            {
                // Encrypt/Decrypt needs exclusive access to the file, this is a problem if we 
                // initiate encrypt for a folder from Explorer, then most probably the folder will
                // be opened.  We don't do "SHChangeNotifySuspendResume" right away for perf reasons,
                // we wait for it to fail and then we try again. (stephstm)

                ASSERT(pidl == NULL);
                pidl = ILCreateFromPath(pszPath);

                if (pidl)
                {
                    SHChangeNotifySuspendResume(TRUE, pidl, TRUE, 0);
                }

                // retry to decrypt after the suspend
                fSucceeded = SHEncryptFile(pszPath, FALSE);

                if (!fSucceeded)
                {
                    // get the last error value now so we know why it failed
                    dwLastError = GetLastError();
                }
            }
        }

        if (fSucceeded)
        {
            // success
            *pbSomethingChanged = TRUE;
            dwLastError = ERROR_SUCCESS;
        }
        else
        {
            ASSERT(dwLastError != ERROR_SUCCESS);
            goto RaiseErrorMsg;
        }
    }

    // now check for encrypt/compress
    if ((pfpsp->asInitial.fCompress != pfpsp->asCurrent.fCompress) &&
        (pfpsp->asCurrent.fCompress == BST_CHECKED))
    {
        if (!CompressFile(pszPath, TRUE))
        {
            // get the last error value now so we know why it failed
            dwLastError = GetLastError();
            goto RaiseErrorMsg;
        }
        else
        {
            // success
            *pbSomethingChanged = TRUE;
        }
    }

    if ((pfpsp->asInitial.fEncrypt != pfpsp->asCurrent.fEncrypt) &&
        (pfpsp->asCurrent.fEncrypt == BST_CHECKED))
    {
        BOOL fSucceeded;

        // only prompt for encrypting the parent folder on non-recursive operations
        if (!pfpsp->fRecursive && !WarnUserAboutDecryptedParentFolder(pszPath, hWndParent))
        {
            // user cancled the operation
            return FALSE;
        }

        fSucceeded = SHEncryptFile(pszPath, TRUE); // try to encrypt the file
        
        if (!fSucceeded)
        {
            // get the last error value now so we know why it failed
            dwLastError = GetLastError();

            if (ERROR_SHARING_VIOLATION == dwLastError)
            {
                // Encrypt/Decrypt needs exclusive access to the file, this is a problem if we 
                // initiate encrypt for a folder from Explorer, then most probably the folder will
                // be opened.  We don't do "SHChangeNotifySuspendResume" right away for perf reasons,
                // we wait for it to fail and then we try again. (stephstm)

                ASSERT(pidl == NULL);
                pidl = ILCreateFromPath(pszPath);

                if (pidl)
                {
                    SHChangeNotifySuspendResume(TRUE, pidl, TRUE, 0);
                }

                // retry to encrypt after the suspend
                fSucceeded = SHEncryptFile(pszPath, TRUE);

                if (!fSucceeded)
                {
                    // get the last error value now so we know why it failed
                    dwLastError = GetLastError();
                }
            }
        }

        if (fSucceeded)
        {
            // success
            *pbSomethingChanged = TRUE;
            dwLastError = ERROR_SUCCESS;
        }
        else
        {
            ASSERT(dwLastError != ERROR_SUCCESS);
            goto RaiseErrorMsg;
        }
    }

RaiseErrorMsg:

    if (pidl)
    {
        SHChangeNotifySuspendResume(FALSE, pidl, TRUE, 0);
        ILFree(pidl);
        pidl = NULL;  // need to reset this in case the user retries the operation
    }
#else

RaiseErrorMsg:

#endif // WINNT

    // if we are ignoring all errors or we dont have an hwnd to use as a parent, 
    // then dont show any error msgs.
    if (pfpsp->fIgnoreAllErrors || !hWndParent)
        dwLastError = ERROR_SUCCESS;

    // If kernel threw up an error dialog (such as "the disk is write proctected")
    // and the user hit "abort" then return false to avoid a second error dialog
    if (dwLastError == ERROR_REQUEST_ABORTED)
        return FALSE;

    // put up the error dlg if necessary, but not for super hidden files
    if (dwLastError != ERROR_SUCCESS)
    {
        // !PathIsRoot is required, since the root path (eg c:\) is superhidden by default even after formatting a drive,
        // why the filesystem thinks that the root should be +s +r after a format is a freaking mystery to me...
        if (bIsSuperHidden && !ShowSuperHidden() && !PathIsRoot(pszPath))
        {
            dwLastError = ERROR_SUCCESS;
        }
        else
        {
            int iRet;
            ATTRIBUTEERROR ae;

            ae.pszPath = pszPath;
            ae.dwLastError = dwLastError;

            iRet = FailedApplyAttribsErrorDlg(hWndParent, &ae);

            switch (iRet)
            {
                case IDRETRY:
                    // we clear out dwError and try again
                    dwLastError = ERROR_SUCCESS;
                    goto RetryApplyAttribs;
                    break;

                case IDIGNOREALL:
                    pfpsp->fIgnoreAllErrors = TRUE;
                    // fall through
                case IDIGNORE:
                    dwLastError = ERROR_SUCCESS;
                    // fall through
                case IDCANCEL:
                default:
                    break;
            }
        }
    }

    // update the progress bar
    if (pfpsp->pProgressDlg)
    {
        ULARGE_INTEGER ulTemp;

        // it is the callers responsibility to make sure that pfpsp->fd is filled with
        // the proper information for the file we are applying attributes to.
        ulTemp.LowPart = pfpsp->fd.nFileSizeLow;
        ulTemp.HighPart = pfpsp->fd.nFileSizeHigh;

        pfpsp->ulNumberOfBytesDone.QuadPart += ulTemp.QuadPart;

        UpdateProgressBar(pfpsp);
    }

    return (dwLastError == ERROR_SUCCESS) ? TRUE : FALSE;
}


BOOL AddLocationToolTips(HWND hDlg, UINT id, LPCTSTR pszText, HWND *phwnd)
{
    if (*phwnd == NULL)
    {
        *phwnd = CreateWindow(TOOLTIPS_CLASS,
                              c_szNULL,
                              WS_POPUP | TTS_NOPREFIX,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              hDlg,
                              NULL,
                              HINST_THISDLL,
                              NULL);
        if (*phwnd)
        {
            TOOLINFO ti;

            ti.cbSize = SIZEOF(ti);
            ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
            ti.hwnd = hDlg;
            ti.uId = (UINT_PTR)GetDlgItem(hDlg, id);
            ti.lpszText = (LPTSTR)pszText;  // const -> non const
            ti.hinst = HINST_THISDLL;
            SendMessage(*phwnd, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
        }
    }
    return BOOLFROMPTR(*phwnd);
}


void UpdateTriStateCheckboxes(FILEPROPSHEETPAGE* pfpsp)
{
    // we turn off tristate after applying attibs for those things that were tri-state
    // initially but are not anymore since we sucessfully applied the attributes
    
    if(pfpsp->hDlg)
    {
        if (pfpsp->asInitial.fReadOnly == BST_INDETERMINATE && pfpsp->asCurrent.fReadOnly != BST_INDETERMINATE)
        {
            SendDlgItemMessage(pfpsp->hDlg, IDD_READONLY, BM_SETSTYLE, BS_AUTOCHECKBOX, 0);
        }
        
        if (pfpsp->asInitial.fHidden == BST_INDETERMINATE && pfpsp->asCurrent.fHidden != BST_INDETERMINATE)
        {
            SendDlgItemMessage(pfpsp->hDlg, IDD_HIDDEN, BM_SETSTYLE, BS_AUTOCHECKBOX, 0);
        }
        
        // Archive is only on the general tab for files on FAT/FAT32 volumes
        if (!pfpsp->fIsCompressionAvailable && pfpsp->asInitial.fArchive == BST_INDETERMINATE && pfpsp->asCurrent.fArchive != BST_INDETERMINATE)
        {
            SendDlgItemMessage(pfpsp->hDlg, IDD_ARCHIVE, BM_SETSTYLE, BS_AUTOCHECKBOX, 0);
        }
    }
}

//
// This applies the attributes to the selected files (multiple file case)
//
// return value:
//      TRUE    We sucessfully applied all the attributes
//      FALSE   The user hit cancel, and we stoped
//
BOOL ApplyMultipleFileAttributes(FILEPROPSHEETPAGE* pfpsp)
{
    int iItem;
    TCHAR szPath[MAX_PATH];
    BOOL bRet = TRUE;
    BOOL bSomethingChanged = FALSE;

    // create the progress dialog
    CreateAttributeProgressDlg(pfpsp);

    // make sure that HIDA_FillFindDatat returns the compressed size, else our progress est will be way off
    for (iItem = 0; HIDA_FillFindData(pfpsp->hida, iItem, szPath, &pfpsp->fd, TRUE); iItem++)
    {
        if (HasUserCanceledAttributeProgressDlg(pfpsp))
        {
            // the user hit cancel on the progress dlg, so stop
            bRet = FALSE;
            break;
        }

        if (pfpsp->fRecursive && (pfpsp->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // apply attribs to the subfolders
            bRet = ApplyRecursiveFolderAttribs(szPath, pfpsp);

            // send out a notification for the whole dir, regardless if the user hit cancel since
            // something could have changed 
            SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, szPath, NULL);
        }
        else
        {
            HWND hwndParent = NULL;
            
            // if we have a progress hwnd, try to use it as our parent. This will fail 
            // if the progress dialog isn't being displayed yet.
            IUnknown_GetWindow((IUnknown*)pfpsp->pProgressDlg, &hwndParent);

            if (!hwndParent)
            {
                // the progress dlg isint here yet, so use the property page hwnd
                hwndParent = GetParent(pfpsp->hDlg);
            }

            // apply the attribs to this item only
            bRet = ApplyFileAttributes(szPath, pfpsp, hwndParent, &bSomethingChanged);

            if (bSomethingChanged)
            {
                // something changed, so send out a notification for that file
                SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, szPath, NULL);
            }
        }
    }

    // destroy the progress dialog
    DestroyAttributeProgressDlg(pfpsp);

    if (bRet)
    {
        // since we just sucessfully applied attribs, reset any tri-state checkboxes as necessary
        UpdateTriStateCheckboxes(pfpsp);

        // the user did NOT hit cancel, so update the prop sheet to reflect the new attribs
        pfpsp->asInitial = pfpsp->asCurrent;
    }

    // flush any change-notifications we generated
    SHChangeNotifyHandleEvents();

    return bRet;
}


STDAPI_(BOOL) ApplySingleFileAttributes(FILEPROPSHEETPAGE* pfpsp)
{
    BOOL bRet = TRUE;
    BOOL bSomethingChanged = FALSE;

    if (!pfpsp->fRecursive)
    {
        bRet = ApplyFileAttributes(pfpsp->szPath, pfpsp, GetParent(pfpsp->hDlg), &bSomethingChanged);
        
        if (bSomethingChanged)
        {
            // something changed, so generate a notification for the item
            SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, pfpsp->szPath, NULL);
        }
    }
    else
    {
        // We only should be doing a recursive operation if we have a directory!
        ASSERT(pfpsp->fIsDirectory);

        CreateAttributeProgressDlg(pfpsp);

        // apply attribs to this folder & sub files/folders
        bRet = ApplyRecursiveFolderAttribs(pfpsp->szPath, pfpsp);
        
        // send out a notification for the whole dir, regardless of the return value since
        // something could have changed even if the user hit cancel
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH, pfpsp->szPath, NULL);
        
        DestroyAttributeProgressDlg(pfpsp);
    }
       
    if (bRet)
    {
        // since we just sucessfully applied attribs, reset any tri-state checkboxes as necessary
        UpdateTriStateCheckboxes(pfpsp);

        // the user did NOT hit cancel, so update the prop sheet to reflect the new attribs
        pfpsp->asInitial = pfpsp->asCurrent;

        // BUGBUG (reinerf) need to update the size fields (eg file was just compressed)
    }

    // handle any events we may have generated
    SHChangeNotifyHandleEvents();

    return bRet;
}

//
// this function sets the string that tells the user what attributes they are about to
// apply
//
BOOL SetAttributePromptText(HWND hDlgRecurse, FILEPROPSHEETPAGE* pfpsp)
{
    TCHAR szAttribsToApply[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    int iLength;

    szAttribsToApply[0] = TEXT('\0');
 
    if (pfpsp->asInitial.fReadOnly != pfpsp->asCurrent.fReadOnly)
    {
        if (pfpsp->asCurrent.fReadOnly)
            LoadString(HINST_THISDLL, IDS_READONLY, szTemp, ARRAYSIZE(szTemp)); 
        else
            LoadString(HINST_THISDLL, IDS_NOTREADONLY, szTemp, ARRAYSIZE(szTemp)); 

        StrCatBuff(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
    }

    if (pfpsp->asInitial.fHidden != pfpsp->asCurrent.fHidden)
    {
        if (pfpsp->asCurrent.fHidden)
            LoadString(HINST_THISDLL, IDS_HIDE, szTemp, ARRAYSIZE(szTemp)); 
        else
            LoadString(HINST_THISDLL, IDS_UNHIDE, szTemp, ARRAYSIZE(szTemp)); 

        StrCatBuff(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
    }
    
    if (pfpsp->asInitial.fArchive != pfpsp->asCurrent.fArchive)
    {
        if (pfpsp->asCurrent.fArchive)
            LoadString(HINST_THISDLL, IDS_ARCHIVE, szTemp, ARRAYSIZE(szTemp)); 
        else
            LoadString(HINST_THISDLL, IDS_UNARCHIVE, szTemp, ARRAYSIZE(szTemp)); 

        StrCatBuff(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
    }

    if (pfpsp->asInitial.fIndex != pfpsp->asCurrent.fIndex)
    {
        if (pfpsp->asCurrent.fIndex)
            LoadString(HINST_THISDLL, IDS_INDEX, szTemp, ARRAYSIZE(szTemp)); 
        else
            LoadString(HINST_THISDLL, IDS_DISABLEINDEX, szTemp, ARRAYSIZE(szTemp)); 

        StrCatBuff(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
    }

    if (pfpsp->asInitial.fCompress != pfpsp->asCurrent.fCompress)
    {
        if (pfpsp->asCurrent.fCompress)
            LoadString(HINST_THISDLL, IDS_COMPRESS, szTemp, ARRAYSIZE(szTemp)); 
        else
            LoadString(HINST_THISDLL, IDS_UNCOMPRESS, szTemp, ARRAYSIZE(szTemp)); 

        StrCatBuff(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
    }

    if (pfpsp->asInitial.fEncrypt != pfpsp->asCurrent.fEncrypt)
    {
        if (pfpsp->asCurrent.fEncrypt)
            LoadString(HINST_THISDLL, IDS_ENCRYPT, szTemp, ARRAYSIZE(szTemp)); 
        else
            LoadString(HINST_THISDLL, IDS_DECRYPT, szTemp, ARRAYSIZE(szTemp)); 

        StrCatBuff(szAttribsToApply, szTemp, ARRAYSIZE(szAttribsToApply));
    }

    if (!*szAttribsToApply)
    {
        // nothing changed bail 
        return FALSE;
    }
   
    // remove the trailing ", "
    iLength = lstrlen(szAttribsToApply);
    ASSERT(iLength >= 3);
    lstrcpy(&szAttribsToApply[iLength - 2], TEXT("\0"));

    SetDlgItemText(hDlgRecurse, IDD_ATTRIBSTOAPPLY, szAttribsToApply);
    return TRUE;
}


//
// This dlg proc is for the prompt to ask the user if they want to have their changes apply
// to only the directories, or all files/folders within the directories.
//
BOOL_PTR CALLBACK RecursivePromptDlgProc(HWND hDlgRecurse, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    FILEPROPSHEETPAGE* pfpsp = (FILEPROPSHEETPAGE *)GetWindowLongPtr(hDlgRecurse, DWLP_USER);
    
    switch (uMessage)
    {
        case WM_INITDIALOG:
        {
            TCHAR szFolderText[MAX_PATH];
            TCHAR szFormatString[MAX_PATH];
            TCHAR szDlgText[MAX_PATH];

            SetWindowLongPtr(hDlgRecurse, DWLP_USER, lParam);
            pfpsp = (FILEPROPSHEETPAGE *)lParam;

            // set the initial state of the radio button
            CheckDlgButton(hDlgRecurse, IDD_NOTRECURSIVE, TRUE);
            
            // set the IDD_ATTRIBSTOAPPLY based on what attribs we are applying
            if (!SetAttributePromptText(hDlgRecurse, pfpsp))
            { 
                // we should not get here because we check for the no attribs
                // to apply earlier
                ASSERT(FALSE);

                EndDialog(hDlgRecurse, TRUE);
            }

            // load either "this folder" or "the selected items"
            LoadString(HINST_THISDLL, pfpsp->fMultipleFiles ? IDS_THESELECTEDITEMS : IDS_THISFOLDER, szFolderText, ARRAYSIZE(szFolderText)); 
            
            // set the IDD_RECURSIVE_TXT text to have "this folder" or "the selected items"
            GetDlgItemText(hDlgRecurse, IDD_RECURSIVE_TXT, szFormatString, ARRAYSIZE(szFormatString));
            wsprintf(szDlgText, szFormatString, szFolderText);
            SetDlgItemText(hDlgRecurse, IDD_RECURSIVE_TXT, szDlgText);

            // set the IDD_NOTRECURSIVE raido button text to have "this folder" or "the selected items"
            GetDlgItemText(hDlgRecurse, IDD_NOTRECURSIVE, szFormatString, ARRAYSIZE(szFormatString));
            wsprintf(szDlgText, szFormatString, szFolderText);
            SetDlgItemText(hDlgRecurse, IDD_NOTRECURSIVE, szDlgText);

            // set the IDD_RECURSIVE raido button text to have "this folder" or "the selected items"
            GetDlgItemText(hDlgRecurse, IDD_RECURSIVE, szFormatString, ARRAYSIZE(szFormatString));
            wsprintf(szDlgText, szFormatString, szFolderText);
            SetDlgItemText(hDlgRecurse, IDD_RECURSIVE, szDlgText);

            return TRUE;
        }

        case WM_COMMAND:
        {
            WORD wID = GET_WM_COMMAND_ID(wParam, lParam);
            switch (wID)
            {
                case IDOK:
                    pfpsp->fRecursive = (IsDlgButtonChecked(hDlgRecurse, IDD_RECURSIVE) == BST_CHECKED);
                    // fall through
                case IDCANCEL:
                    EndDialog(hDlgRecurse, (wID == IDCANCEL) ? FALSE : TRUE);
                    break;
            }
        }

        default:
            return FALSE;
    }
}


//
// This wndproc handles the "Advanced Attributes..." button on the general tab for
//
// return - FALSE:  the user hit cancle
//          TRUE:   the user hit ok
//
BOOL_PTR CALLBACK AdvancedFileAttribsDlgProc(HWND hDlgAttribs, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    FILEPROPSHEETPAGE* pfpsp = (FILEPROPSHEETPAGE *)GetWindowLongPtr(hDlgAttribs, DWLP_USER);
    
    switch (uMessage) 
    {
        case WM_INITDIALOG:
        {   
            TCHAR szFolderText[MAX_PATH];
            TCHAR szDlgText[MAX_PATH];
            TCHAR szFormatString[MAX_PATH];

            SetWindowLongPtr(hDlgAttribs, DWLP_USER, lParam);
            pfpsp = (FILEPROPSHEETPAGE *)lParam;

            //
            // set the initial state of the checkboxes
            //

            // BUGBUG (reinerf) - should have a helper fn. for this
            if (pfpsp->asInitial.fArchive == BST_INDETERMINATE)
            {
                SendDlgItemMessage(hDlgAttribs, IDD_ARCHIVE, BM_SETSTYLE, BS_AUTO3STATE, 0);
            }
            CheckDlgButton(hDlgAttribs, IDD_ARCHIVE, pfpsp->asCurrent.fArchive);

            if (pfpsp->asInitial.fIndex == BST_INDETERMINATE)
            {
                SendDlgItemMessage(hDlgAttribs, IDD_INDEX, BM_SETSTYLE, BS_AUTO3STATE, 0);
            }
            CheckDlgButton(hDlgAttribs, IDD_INDEX, pfpsp->asCurrent.fIndex);

            if (pfpsp->asInitial.fCompress == BST_INDETERMINATE)
            {
                SendDlgItemMessage(hDlgAttribs, IDD_COMPRESS, BM_SETSTYLE, BS_AUTO3STATE, 0);
            }
            CheckDlgButton(hDlgAttribs, IDD_COMPRESS, pfpsp->asCurrent.fCompress);

            if (pfpsp->asInitial.fEncrypt == BST_INDETERMINATE)
            {
                SendDlgItemMessage(hDlgAttribs, IDD_ENCRYPT, BM_SETSTYLE, BS_AUTO3STATE, 0);
            }
            CheckDlgButton(hDlgAttribs, IDD_ENCRYPT, pfpsp->asCurrent.fEncrypt);

            // assert that compression and encryption are mutually exclusive
            ASSERT(!((pfpsp->asCurrent.fCompress == BST_CHECKED) && (pfpsp->asCurrent.fEncrypt == BST_CHECKED)));
            
            // gray the index checkbox based on whether or not it is supported on this volume
            EnableWindow(GetDlgItem(hDlgAttribs, IDD_INDEX), pfpsp->fIsIndexAvailable);

            // load either "this folder" or "the selected items"
            LoadString(HINST_THISDLL, pfpsp->fMultipleFiles ? IDS_THESELECTEDITEMS : IDS_THISFOLDER, szFolderText, ARRAYSIZE(szFolderText)); 
            
            // set the IDC_MANAGEFILES_TXT text to have "this folder" or "the selected items"
            GetDlgItemText(hDlgAttribs, IDC_MANAGEFILES_TXT, szFormatString, ARRAYSIZE(szFormatString));
            wsprintf(szDlgText, szFormatString, szFolderText);
            SetDlgItemText(hDlgAttribs, IDC_MANAGEFILES_TXT, szDlgText);
            return TRUE;
        }

        case WM_HELP:
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPTSTR)aAdvancedHelpIds);
            break;

        case WM_CONTEXTMENU:
            if ((int)SendMessage(hDlgAttribs, WM_NCHITTEST, 0, lParam) != HTCLIENT)
            {
                // not in our client area, so don't process it
                return FALSE;
            }
            WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)aAdvancedHelpIds);
            break;

        case WM_COMMAND:
        {
            WORD wControlID = GET_WM_COMMAND_ID(wParam, lParam);

            switch (wControlID) 
            {
            case IDD_COMPRESS:
                // encrypt and compress are mutually exclusive
                if (IsDlgButtonChecked(hDlgAttribs, IDD_COMPRESS) == BST_CHECKED)
                {
                    // the user checked compress so uncheck the encrypt checkbox
                    CheckDlgButton(hDlgAttribs, IDD_ENCRYPT, BST_UNCHECKED);
                }
                break;

            case IDD_ENCRYPT:
                // encrypt and compress are mutually exclusive
                if (IsDlgButtonChecked(hDlgAttribs, IDD_ENCRYPT) == BST_CHECKED)
                {
                    // the user checked encrypt, so uncheck the compression checkbox
                    CheckDlgButton(hDlgAttribs, IDD_COMPRESS, BST_UNCHECKED);
                }
                break;


            case IDOK:
            {
                pfpsp->asCurrent.fArchive = IsDlgButtonChecked(hDlgAttribs, IDD_ARCHIVE);
                if (pfpsp->asCurrent.fArchive == BST_INDETERMINATE)
                {
                    // if its indeterminate, it better had been indeterminate to start with
                    ASSERT(pfpsp->asInitial.fArchive == BST_INDETERMINATE);
                }
                                    
                pfpsp->asCurrent.fIndex = IsDlgButtonChecked(hDlgAttribs, IDD_INDEX);
                if (pfpsp->asCurrent.fIndex == BST_INDETERMINATE)
                {
                    // if its indeterminate, it better had been indeterminate to start with
                    ASSERT(pfpsp->asInitial.fIndex == BST_INDETERMINATE);
                }

                pfpsp->asCurrent.fCompress = IsDlgButtonChecked(hDlgAttribs, IDD_COMPRESS);
                if (pfpsp->asCurrent.fCompress == BST_INDETERMINATE)
                {
                    // if its indeterminate, it better had been indeterminate to start with
                    ASSERT(pfpsp->asInitial.fCompress == BST_INDETERMINATE);
                }

                pfpsp->asCurrent.fEncrypt = IsDlgButtonChecked(hDlgAttribs, IDD_ENCRYPT);
                if (pfpsp->asCurrent.fEncrypt == BST_INDETERMINATE)
                {
                    // if its indeterminate, it better had been indeterminate to start with
                    ASSERT(pfpsp->asInitial.fEncrypt == BST_INDETERMINATE);
                }
            }
            // fall through...

            case IDCANCEL:
                ReplaceDlgIcon(hDlgAttribs, IDD_ITEMICON, NULL);
            
                EndDialog(hDlgAttribs, (wControlID == IDCANCEL) ? FALSE : TRUE);
                break;
            }
        }

        default:
            return FALSE;
    }

    return TRUE;         
}


//
// Descriptions:
//   This is the dialog procedure for multiple object property sheet.
//
BOOL_PTR CALLBACK MultiplePrshtDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    FILEPROPSHEETPAGE * pfpsp = (FILEPROPSHEETPAGE *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage) {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pfpsp = (FILEPROPSHEETPAGE *)lParam;
        pfpsp->hDlg = hDlg;
            
        InitMultiplePrsht(pfpsp);
        break;

    case WM_HELP:
        WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPVOID)aMultiPropHelpIds);
        break;

    case WM_CONTEXTMENU:
        if ((int)SendMessage(hDlg, WM_NCHITTEST, 0, lParam) != HTCLIENT)
        {
            // not in our client area, so don't process it
            return FALSE;
        }
        WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)aMultiPropHelpIds);
        break;

    case WM_TIMER:
        UpdateSizeCount(pfpsp);
        break;

    case WM_DESTROY:
        // Careful!  pfpsp can be NULL in low memory situations
        if (pfpsp)
        {
            KillSizeThread(pfpsp);
            ASSERT(!IsWindow(pfpsp->hwndTip));
        }
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) 
        {
        case IDD_READONLY:
        case IDD_HIDDEN:
        case IDD_ARCHIVE:
            break;

        case IDC_ADVANCED:
            // the dialog box returns fase if the user hit cancel, and true if they hit ok,
            // so if they cancelled, return immediately and don't send the PSM_CHANGED message
            // because nothing actually changed
            if (!DialogBoxParam(HINST_THISDLL, 
                                MAKEINTRESOURCE(pfpsp->fIsDirectory ? DLG_FOLDERATTRIBS : DLG_FILEATTRIBS),
                                hDlg,
                                AdvancedFileAttribsDlgProc,
                                (LPARAM)pfpsp))
            {
                // the user has cancled
                return TRUE;
            }
            break;

        default:
            return TRUE;
        }

        // check to see if we need to enable the Apply button
        if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
        {
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) 
        {
            case PSN_APPLY:
            {
                BOOL bRet = TRUE;

                //
                // Get the final state of the checkboxes
                //

                pfpsp->asCurrent.fReadOnly = IsDlgButtonChecked(hDlg, IDD_READONLY);
                if (pfpsp->asCurrent.fReadOnly == BST_INDETERMINATE)
                {
                    // if its indeterminate, it better had been indeterminate to start with
                    ASSERT(pfpsp->asInitial.fReadOnly == BST_INDETERMINATE);
                }
                
                pfpsp->asCurrent.fHidden = IsDlgButtonChecked(hDlg, IDD_HIDDEN);
                if (pfpsp->asCurrent.fHidden == BST_INDETERMINATE)
                {
                    // if its indeterminate, it better had been indeterminate to start with
                    ASSERT(pfpsp->asInitial.fHidden == BST_INDETERMINATE);
                }

                if (!pfpsp->fIsCompressionAvailable)
                {
                    // at least one of the files is on FAT, so the Archive checkbox is on the general page
                    pfpsp->asCurrent.fArchive = IsDlgButtonChecked(hDlg, IDD_ARCHIVE);
                    if (pfpsp->asCurrent.fArchive == BST_INDETERMINATE)
                    {
                        // if its indeterminate, it better had been indeterminate to start with
                        ASSERT(pfpsp->asInitial.fArchive == BST_INDETERMINATE);
                    }
                }

                // check to see if the user actually changed something, if they didnt, then 
                // we dont have to apply anything
                if (memcmp(&pfpsp->asInitial, &pfpsp->asCurrent, SIZEOF(pfpsp->asInitial)) != 0)
                {
                    // NOTE: We dont check to see if all the dirs are empty, that would be too expensive.
                    // We only do that in the single file case.
                    if (pfpsp->fIsDirectory)
                    {
                        // check to see if the user wants to apply the attribs recursively or not
                        bRet = (int)DialogBoxParam(HINST_THISDLL, 
                                              MAKEINTRESOURCE(DLG_ATTRIBS_RECURSIVE),
                                              hDlg,
                                              RecursivePromptDlgProc,
                                              (LPARAM)pfpsp);
                    }
            
                    if (bRet)
                        bRet = ApplyMultipleFileAttributes(pfpsp);

                    if (!bRet)
                    {
                        // the user hit cancel, so we return true to prevent the property sheet form closeing
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }
                    else
                    {
                        // update the size / last accessed time
                        UpdateSizeField(pfpsp, NULL);
                    }
                }
                break;
            }
            // fall through

            default:
                return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}



// in:
//      hdlg
//      id      text control id
//      pftUTC  UTC time time to be set
//
void SetDateTimeText(HWND hdlg, int id, const FILETIME *pftUTC )
{
    SetDateTimeTextEx( hdlg, id, pftUTC, FDTF_LONGDATE | FDTF_LONGTIME | FDTF_RELATIVE ) ;
}

void SetDateTimeTextEx(HWND hdlg, int id, const FILETIME *pftUTC, DWORD dwFlags )
{
    if (!IsNullTime(pftUTC))
    {
        TCHAR szBuf[64];
        LCID locale = GetUserDefaultLCID();        

        if ((PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC) || 
            (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_HEBREW))
        {
            HWND hWnd = GetDlgItem(hdlg, id);
            DWORD dwExStyle = GetWindowLong(hdlg, GWL_EXSTYLE);
            if ((BOOLIFY(dwExStyle & WS_EX_RTLREADING)) != (BOOLIFY(dwExStyle & RTL_MIRRORED_WINDOW)))
                dwFlags |= FDTF_RTLDATE;
            else
                dwFlags |= FDTF_LTRDATE;
        }

        SHFormatDateTime(pftUTC, &dwFlags, szBuf, SIZECHARS(szBuf));
        SetDlgItemText(hdlg, id, szBuf);
    }
}


//
// Descriptions:
//   Detects BiDi Calender 
//
BOOL _IsBiDiCalendar(void)
{
    TCHAR chCalendar[32];
    CALTYPE defCalendar;
    BOOL fBiDiCalender = FALSE;
    LCID locale = GetUserDefaultLCID();

    //
    // Let's verify the calendar type whether it's gregorian or not.
    if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_ICALENDARTYPE,
                      (TCHAR *) &chCalendar[0],
                      ARRAYSIZE(chCalendar)))
    {
        defCalendar = StrToInt((TCHAR *)&chCalendar[0]);
        
        if ((defCalendar == CAL_HIJRI) ||
            (defCalendar == CAL_HEBREW) ||
            ((defCalendar == CAL_GREGORIAN) && 
            ((PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC) || 
            (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_HEBREW))) ||            
            (defCalendar == CAL_GREGORIAN_ARABIC) ||
            (defCalendar == CAL_GREGORIAN_XLIT_ENGLISH) ||     
            (defCalendar == CAL_GREGORIAN_XLIT_FRENCH))
        {
            fBiDiCalender = TRUE;
        }
    }

    return fBiDiCalender;
}

// Set the friendly display name into control uId.
BOOL SetPidlToWindow(HWND hwnd, UINT uId, LPITEMIDLIST pidl)
{
    BOOL fRes = FALSE;
    LPCITEMIDLIST pidlItem;
    IShellFolder* psf;
    if (SUCCEEDED(SHBindToParent(pidl, &IID_IShellFolder, (void**)&psf, &pidlItem)))
    {
        STRRET str;

        // SHGDN_FORADDRESSBAR | SHGDN_FORPARSING because we want:
        // c:\winnt\.... and http://weird, but not ::{GUID} or Folder.{GUID}
        if (SUCCEEDED(psf->lpVtbl->GetDisplayNameOf(psf, pidlItem, SHGDN_FORADDRESSBAR | SHGDN_FORPARSING, &str)))
        {
            TCHAR szPath[MAX_PATH];
            if (SUCCEEDED(StrRetToBuf(&str, pidl, szPath, ARRAYSIZE(szPath))))
            {
                SetDlgItemText(hwnd, IDD_TARGET, szPath);
                fRes = TRUE;
            }
        }
        psf->lpVtbl->Release(psf);
    }

    return fRes;
}


//
// Descriptions:
//   This function fills fields of the "general" dialog box (a page of
//  a property sheet) with attributes of the associated file.
//
BOOL InitSingleFilePrsht(FILEPROPSHEETPAGE * pfpsp)
{
    HANDLE hfind;
    TCHAR szBuffer[MAX_PATH];
    SHFILEINFO sfi;
    int cchMax;

    // fd is filled in with info from the pidl, but this
    // does not contain all the date/time information so hit the disk here.
    hfind = FindFirstFile(pfpsp->szPath, &pfpsp->fd);
    ASSERT(hfind != INVALID_HANDLE_VALUE);
    if (hfind == INVALID_HANDLE_VALUE)
    {
        // if this failed we should clear out some values as to not show garbage on the screen.
        ZeroMemory(&pfpsp->fd, SIZEOF(pfpsp->fd));
    }
    else
    {
        FindClose(hfind);
    }

    // get info about the file.
    SHGetFileInfo(pfpsp->szPath, pfpsp->fd.dwFileAttributes, &sfi, SIZEOF(sfi),
        SHGFI_ICON|SHGFI_LARGEICON|
        SHGFI_DISPLAYNAME|
        SHGFI_TYPENAME | SHGFI_ADDOVERLAYS);

    // .ani cursor hack!
    if (lstrcmpi(PathFindExtension(pfpsp->szPath), TEXT(".ani")) == 0)
    {
        HICON hIcon = (HICON)LoadImage(NULL, pfpsp->szPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
        if (hIcon)
        {
            if (sfi.hIcon)
                DestroyIcon(sfi.hIcon);

            sfi.hIcon = hIcon;
        }
    }

    // icon
    ReplaceDlgIcon(pfpsp->hDlg, IDD_ITEMICON, sfi.hIcon);

    // set the initial rename state
    pfpsp->fRename = FALSE;

#ifdef WINNT
    // set the file type
    if (pfpsp->fMountedDrive)
    {
        TCHAR szVolumeGUID[MAX_PATH];
        TCHAR szVolumeLabel[MAX_PATH];

        //Borrow szVolumeGUID
        LoadString(HINST_THISDLL, IDS_MOUNTEDVOLUME, szVolumeGUID, ARRAYSIZE(szVolumeGUID));

        //BUGBUG: should have "Mounted local disk" or "Mounted CD_ROM"...
        SetDlgItemText(pfpsp->hDlg, IDD_FILETYPE, szVolumeGUID);

        //use szVolumeLabel temporarily
        lstrcpy(szVolumeLabel, pfpsp->szPath);
        PathAddBackslash(szVolumeLabel);
        GetVolumeNameForVolumeMountPoint(szVolumeLabel, szVolumeGUID, ARRAYSIZE(szVolumeGUID));

        if (!GetVolumeInformation(szVolumeGUID, szVolumeLabel, ARRAYSIZE(szVolumeLabel),
            NULL, NULL, NULL, pfpsp->szFileSys, ARRAYSIZE(pfpsp->szFileSys)))
        {
            EnableWindow(GetDlgItem(pfpsp->hDlg, IDC_DRV_PROPERTIES), FALSE);
            *szVolumeLabel = 0;
        }

        if (!(*szVolumeLabel))
            LoadString(HINST_THISDLL, IDS_UNLABELEDVOLUME, szVolumeLabel, ARRAYSIZE(szVolumeLabel));        

        SetDlgItemText(pfpsp->hDlg, IDC_DRV_TARGET, szVolumeLabel);
    }
    else
#endif
    {
        SetDlgItemText(pfpsp->hDlg, IDD_FILETYPE, sfi.szTypeName);
    }


    // save off the initial short filename, and set the "Name" edit box
    lstrcpy(pfpsp->szInitialName, sfi.szDisplayName);
    SetDlgItemText(pfpsp->hDlg, IDD_NAMEEDIT, sfi.szDisplayName);

    // use a strcmp to see if we are showing the extension
    if (lstrcmpi(sfi.szDisplayName, PathFindFileName(pfpsp->szPath)) == 0)
    {
        // since the strings are the same, we must be showing the extension
        pfpsp->fShowExtension = TRUE;
    }

    // limit the text length in the "Name" edit box to (MAX_PATH - 1 (for null) - directory length - 1 (for '\') - extension length (if extension is not shown) - 1 b/c the copy engine is lame (only supports paths that are MAX_PATH-2 !!!)
    lstrcpy(szBuffer, pfpsp->szPath);
    PathRemoveFileSpec(szBuffer);
    cchMax = MAX_PATH - 1 - lstrlen(szBuffer) - 1 - (pfpsp->fShowExtension ? 0 : lstrlen(PathFindExtension(pfpsp->szPath))) - 1;
    Edit_LimitText(GetDlgItem(pfpsp->hDlg, IDD_NAMEEDIT), cchMax);

    // Are we a folder shortcut?
    if (pfpsp->fFolderShortcut)
    {
        // Yes; Then we need to populate folder shortcut specific controls.
        IShellLink *psl;
        pfpsp->pidlFolder = ILCreateFromPath(pfpsp->szPath);
        if (pfpsp->pidlFolder)
        {
            if (SUCCEEDED(SHGetUIObjectFromFullPIDL(pfpsp->pidlFolder, NULL, IID_PPV_ARG(IShellLink, &psl)))
            {
                // Populate the Target
                TCHAR sz[INFOTIPSIZE];
                if (SUCCEEDED(psl->lpVtbl->GetIDList(psl, &pfpsp->pidlTarget)))
                {
                    if (SetPidlToWindow(pfpsp->hDlg, IDD_TARGET, pfpsp->pidlTarget))
                    {
                        pfpsp->fValidateEdit = FALSE;     // Set this to false because we already have a pidl 
                                                          // and don't need to validate.
                    }
                }

                // And description
                if (SUCCEEDED(psl->lpVtbl->GetDescription(psl, sz, ARRAYSIZE(sz))))
                {
                    SetDlgItemText(pfpsp->hDlg, IDD_COMMENT, sz);
                }

                psl->lpVtbl->Release(psl);
            }
        }

        SetDateTimeText(pfpsp->hDlg, IDD_CREATED, &pfpsp->fd.ftCreationTime);
    }
    else
    {
        // set the initial attributes
        SetInitialFileAttribs(pfpsp, pfpsp->fd.dwFileAttributes, pfpsp->fd.dwFileAttributes);

        // set the current attributes to the same as the initial
        pfpsp->asCurrent = pfpsp->asInitial;

        CheckDlgButton(pfpsp->hDlg, IDD_READONLY, pfpsp->asInitial.fReadOnly);
        CheckDlgButton(pfpsp->hDlg, IDD_HIDDEN, pfpsp->asInitial.fHidden);

        // Disable renaming the file if requested
        if (pfpsp->fDisableRename)
        {
            EnableWindow(GetDlgItem(pfpsp->hDlg, IDD_NAMEEDIT), FALSE);
        }

        if (pfpsp->fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
        {
            // to avoid people making SYSTEM files HIDDEN (superhidden files are
            // not show to the user by default) we don't let people make SYSTEM files HIDDEN
            EnableWindow(GetDlgItem(pfpsp->hDlg, IDD_HIDDEN), FALSE);
        }

        // Archive is only on the general tab for FAT, otherwise it is under the "Advanced attributes"
        // and FAT volumes dont have the "Advanced attributes" button.
        if (!pfpsp->fIsCompressionAvailable)
        {
            // we are on FAT/FAT32, so get rid of the "Advanced attributes" button, and set the inital Archive state
            DestroyWindow(GetDlgItem(pfpsp->hDlg, IDC_ADVANCED));
            CheckDlgButton(pfpsp->hDlg, IDD_ARCHIVE, pfpsp->asInitial.fArchive);
        }
        else
        {
            // if compression is available, then we must be on NTFS
            DestroyWindow(GetDlgItem(pfpsp->hDlg, IDD_ARCHIVE));
        }

        UpdateSizeField(pfpsp, &pfpsp->fd);

        if (!(pfpsp->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // Check to see if the target file is a lnk, because if it is a lnk then 
            // we need to display the type information for the target, not the lnk itself.
            if (PathIsShortcut(pfpsp->szPath))
            {
                pfpsp->fIsLink = TRUE;
            }

            if (!(GetFileAttributes(pfpsp->szPath) & FILE_ATTRIBUTE_OFFLINE))
            {
                 UpdateOpensWithInfo(pfpsp);
            }
            else
            {
                 EnableWindow(GetDlgItem(pfpsp->hDlg, IDC_FT_PROP_CHANGEOPENSWITH), FALSE);
            }
        }

        // get the full path to the folder that contains this file.
        lstrcpyn(szBuffer, pfpsp->szPath, ARRAYSIZE(szBuffer));
        PathRemoveFileSpec(szBuffer);

        AddLocationToolTips(pfpsp->hDlg, IDD_LOCATION, szBuffer, &pfpsp->hwndTip);

        // Keep Functionality same as NT4 by avoiding PathCompactPath. 
        SetDlgItemText(pfpsp->hDlg, IDD_LOCATION, szBuffer);
    }

    return TRUE;
}

#ifdef WINNT
BOOL ShowMountedVolumeProperties(LPCTSTR pszMountedVolume, HWND hwndParent)
{
    IMountedVolume* pMountedVolume;
    IDataObject* pDataObj;
    HRESULT hres;

    hres = SHCoCreateInstance(NULL, &CLSID_MountedVolume, NULL, &IID_IMountedVolume,
        &pMountedVolume);

    if (SUCCEEDED(hres))
    {
        TCHAR szPathSlash[MAX_PATH];
        lstrcpy(szPathSlash, pszMountedVolume);
        PathAddBackslash(szPathSlash);

        ASSERT(pMountedVolume);

        hres = pMountedVolume->lpVtbl->Initialize(pMountedVolume, szPathSlash);
    
        if (SUCCEEDED(hres))
        {
            hres = pMountedVolume->lpVtbl->QueryInterface(pMountedVolume, &IID_IDataObject, &pDataObj);

            if (SUCCEEDED(hres))
            {
                PROPSTUFF * pps = (PROPSTUFF *)LocalAlloc(LPTR, SIZEOF(PROPSTUFF));
                if (pps)
                {
                    pps->lpStartAddress = _CDrives_PropertiesThread;
                    pps->pdtobj = pDataObj;

                    EnableWindow(hwndParent, FALSE);

                    _CDrives_PropertiesThread(pps);

                    EnableWindow(hwndParent, TRUE);

                    LocalFree(pps);
                }

                pDataObj->lpVtbl->Release(pDataObj);
            }
            pMountedVolume->lpVtbl->Release(pMountedVolume);
        }
    }

    return SUCCEEDED(hres);
}
#endif

#ifdef FOLDERSHORTCUT_EDITABLETARGET
BOOL SetFolderShortcutInfo(HWND hDlg, FILEPROPSHEETPAGE* pfpsp)
{
    BOOL fSuccess = FALSE;
    IShellLink* psl;
    ASSERT(pfpsp->pidlFolder);
   
    if (SUCCEEDED(SHGetUIObjectFromFullPIDL(pfpsp->pidlFolder, NULL, IID_PPV_ARG(IShellLink, &psl)))
    {
        IPersistFile* ppf;
        TCHAR sz[INFOTIPSIZE];
        Edit_GetText(GetDlgItem(pfpsp->hDlg, IDD_COMMENT), sz, ARRAYSIZE(sz));

        psl->lpVtbl->SetDescription(psl, sz);

        if (pfpsp->fValidateEdit)
        {
            IShellFolder* psf;
            if (SUCCEEDED(SHGetDesktopFolder(&psf)))
            {
                TCHAR szPath[MAX_PATH];
                LPITEMIDLIST pidlDest;
                ULONG chEat = 0;
                Edit_GetText(GetDlgItem(pfpsp->hDlg, IDD_TARGET), sz, ARRAYSIZE(sz));

                if (PathCanonicalize(szPath, sz))
                {
                    DWORD dwAttrib = SFGAO_FOLDER | SFGAO_VALIDATE;
                    if (SUCCEEDED(psf->lpVtbl->ParseDisplayName(psf, NULL, NULL, szPath, &chEat, &pidlDest, &dwAttrib)))
                    {
                        if ((dwAttrib & SFGAO_FOLDER) == SFGAO_FOLDER)
                        {
                            ILFree(pfpsp->pidlTarget);
                            pfpsp->pidlTarget = pidlDest;
                            fSuccess = TRUE;
                        }
                        else
                        {
                            ILFree(pidlDest);
                        }
                    }
                }
                psf->lpVtbl->Release(psf);
            }
        }
        else
        {
            fSuccess = TRUE;
        }
     


        if (fSuccess)
        {
            psl->lpVtbl->SetIDList(psl, pfpsp->pidlTarget);
            if (SUCCEEDED(psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (void**)&ppf)))
            {
                fSuccess = (S_OK == ppf->lpVtbl->Save(ppf, pfpsp->szPath, TRUE));
                SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, pfpsp->szPath, NULL);
                ppf->lpVtbl->Release(ppf);
            }
        }

        psl->lpVtbl->Release(psl);
    }

    return fSuccess;
}
#endif

//
// Descriptions:
//   This is the dialog procedure for the "general" page of a property sheet.
//
BOOL_PTR CALLBACK SingleFilePrshtDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    FILEPROPSHEETPAGE* pfpsp = (FILEPROPSHEETPAGE *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMessage) 
    {
        case WM_INITDIALOG:
            // REVIEW, we should store more state info here, for example
            // the hIcon being displayed and the FILEINFO pointer, not just
            // the file name ptr
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
            pfpsp = (FILEPROPSHEETPAGE *)lParam;
            pfpsp->hDlg = hDlg;
            
            InitSingleFilePrsht(pfpsp);

            // We set this to signal that we are done processing the WM_INITDIALOG. 
            // This is needed because we set the text of the "Name" edit box and unless
            // he knows that this is being set for the first time, he thinks that someone is doing a rename. 
            pfpsp->fWMInitFinshed = TRUE;
            break;

        case WM_DESTROY:
            // Careful!  pfpsp can be NULL in low memory situations
            if (pfpsp)
            {
                ReplaceDlgIcon(pfpsp->hDlg, IDD_ITEMICON, NULL);

                CleanupOpensWithInfo(pfpsp);

                KillSizeThread(pfpsp);
                ASSERT(!IsWindow(pfpsp->hwndTip));
            }
            break;

        case WM_TIMER:
            if (!pfpsp->fMountedDrive)
                UpdateSizeCount(pfpsp);
            break;
        
        case WM_HELP:
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP, (ULONG_PTR)(LPVOID)aGeneralHelpIds);
            break;

        case WM_CONTEXTMENU:
            if ((int)SendMessage(hDlg, WM_NCHITTEST, 0, lParam) != HTCLIENT)
            {
                // not in our client area, so don't process it
                return FALSE;
            }
            WinHelp((HWND)wParam, NULL, HELP_CONTEXTMENU, (ULONG_PTR)(LPVOID)aGeneralHelpIds);
            break;


        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            {
                case IDD_READONLY:
                case IDD_HIDDEN:
                case IDD_ARCHIVE:
                    break;

#ifdef FOLDERSHORTCUT_EDITABLETARGET
                case IDD_TARGET:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                    {
                        // someone typed in the target. 
                        
                        //Set the apply button
                        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);

                        // Do a verification on apply.
                        pfpsp->fValidateEdit = TRUE;
                    }
                    break;

                case IDD_COMMENT:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                    {
                        // Set the apply.
                        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                    }
                    break;

#endif

                case IDD_NAMEEDIT:
                    // we need to check the pfpsp->fWMInitFinshed to make sure that we are done processing the WM_INITDIALOG, 
                    // because during init we set the initial IDD_NAMEEDIT text which generates a EN_CHANGE message. 
                    if (!pfpsp->fRename && GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE && pfpsp->fWMInitFinshed)
                    {
                        pfpsp->fRename = TRUE;
                        // 
                        // HACKHACK (reinerf)
                        //
                        // since the user has renamed the file, we need to disable the apply button. We only
                        // allow "ok" or "cancel" after the name has changed. This prevents us from renaming
                        // the file out from under other property sheet extensions that cache the original
                        // name away
                        PropSheet_DisableApply(GetParent(pfpsp->hDlg));
                    }
                    break;

                case IDC_CHANGEFILETYPE:
                    {
                        // Bring up the "Open With" dialog
                        OPENASINFO oai;

                        if (pfpsp->fIsLink && pfpsp->szLinkTarget[0])
                        {
                            // if we have a link we want to re-associate the link target, NOT .lnk files!
                            oai.pcszFile = pfpsp->szLinkTarget;
                        }
                        else
                        {
#ifdef DEBUG
                            LPTSTR pszExt = PathFindExtension(pfpsp->szPath);
                            
                            // reality check...
                            ASSERT((lstrcmpi(pszExt, TEXT(".exe")) != 0) &&
                                   (lstrcmpi(pszExt, TEXT(".lnk")) != 0));
#endif // DEBUG
                            oai.pcszFile = pfpsp->szPath;
                        }

                        oai.pcszClass = NULL;
                        oai.dwInFlags = (OAIF_REGISTER_EXT | OAIF_FORCE_REGISTRATION); // we want the association to be made

                        if (SUCCEEDED(OpenAsDialog(GetParent(hDlg), &oai)))
                        {
                            // we changed the association so update the "Opens with:" text. Clear out szLinkTarget to force
                            // the update to happen
                            pfpsp->szLinkTarget[0] = TEXT('\0');
                            UpdateOpensWithInfo(pfpsp);
                        }
                    }
                    break;

                case IDC_ADVANCED:
                    // the dialog box returns fase if the user hit cancel, and true if they hit ok,
                    // so if they cancelled, return immediately and don't send the PSM_CHANGED message
                    // because nothing actually changed
                    if (!DialogBoxParam(HINST_THISDLL, 
                                        MAKEINTRESOURCE(pfpsp->fIsDirectory ? DLG_FOLDERATTRIBS : DLG_FILEATTRIBS),
                                        hDlg,
                                        AdvancedFileAttribsDlgProc,
                                        (LPARAM)pfpsp))
                    {
                        // the user has canceled
                        return TRUE;
                    }
                    break;

#ifdef WINNT
                case IDC_DRV_PROPERTIES:
                    {
                        ASSERT(pfpsp->fMountedDrive);

                        ShowMountedVolumeProperties(pfpsp->szPath, hDlg);

                        break;
                    }
#endif

#ifdef FOLDERSHORTCUT_EDITABLETARGET
                case IDD_BROWSE:
                    {
                        // Display the BrowseForFolder dialog.
                        TCHAR szTitle[MAX_PATH];
                        TCHAR szAltPath[MAX_PATH];
                        LPITEMIDLIST pidlFull;
                        BROWSEINFO bi = {0};

                        // BUGBUG (lamadio): Implement a filter to filter things we can create folder
                        // shortcuts to. Not enough time for this rev 6.5.99

                        LoadString(HINST_THISDLL, IDS_BROWSEFORFS, szTitle, ARRAYSIZE(szTitle));

                        bi.hwndOwner    = hDlg;
                        bi.pidlRoot     = NULL;
                        bi.pszDisplayName = szAltPath;
                        bi.lpszTitle    = szTitle;
                        bi.ulFlags      =  BIF_USENEWUI | BIF_EDITBOX;
                        pidlFull = SHBrowseForFolder(&bi);
                        if (pidlFull)
                        {
                            ILFree(pfpsp->pidlTarget);
                            pfpsp->pidlTarget = pidlFull;

                            if (SetPidlToWindow(hDlg, IDD_TARGET, pfpsp->pidlTarget))
                            {
                                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                                pfpsp->fValidateEdit = FALSE;
                            }
                        }
                    }
                    break;
#endif

                default:
                    return TRUE;
            }

            // check to see if we need to enable the Apply button
            if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
            {
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR *)lParam)->code) 
            {
                case PSN_APPLY:
                    // check to see if we could apply the name change.  Note that this 
                    // does not actually apply the change until PSN_LASTCHANCEAPPLY
                    pfpsp->fCanRename = TRUE;
                    if (pfpsp->fRename && !ApplyRename(pfpsp, FALSE))
                    {
                        // can't change the name so don't let the dialog close
                        pfpsp->fCanRename = FALSE;
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }

                    if (pfpsp->fFolderShortcut)
                    {
#ifdef FOLDERSHORTCUT_EDITABLETARGET
                        if (!SetFolderShortcutInfo(hDlg, pfpsp))
                        {
                            // Display that we could not create because blah, blah, blah
                            ShellMessageBox(HINST_THISDLL, hDlg, MAKEINTRESOURCE(IDS_FOLDERSHORTCUT_ERR),
                                MAKEINTRESOURCE(IDS_FOLDERSHORTCUT_ERR_TITLE), MB_OK | MB_ICONSTOP);

                            // Reset the Folder info.
                            SetPidlToWindow(hDlg, IDD_TARGET, pfpsp->pidlTarget);

                            // Don't close the dialog.
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                            return TRUE;
                        }
#endif
                    }
                    else
                    {
                        pfpsp->asCurrent.fReadOnly = (IsDlgButtonChecked(hDlg, IDD_READONLY) == BST_CHECKED);

                        pfpsp->asCurrent.fHidden = (IsDlgButtonChecked(hDlg, IDD_HIDDEN) == BST_CHECKED);

                        // Archive is on the general page for FAT volumes
                        if (!pfpsp->fIsCompressionAvailable)
                            pfpsp->asCurrent.fArchive = (IsDlgButtonChecked(hDlg, IDD_ARCHIVE) == BST_CHECKED);

                        // check to see if the user actually changed something, if they didnt, then 
                        // we dont have to apply anything
                        if (memcmp(&pfpsp->asInitial, &pfpsp->asCurrent, SIZEOF(pfpsp->asInitial)) != 0)
                        {
                            BOOL bRet = TRUE;

                            // Check to see if the user wants to apply the attribs recursively or not. If the
                            // directory is empty, dont bother to ask, since there is nothing to recurse into
                            if (pfpsp->fIsDirectory && !PathIsDirectoryEmpty(pfpsp->szPath))
                            {
                                bRet = (int)DialogBoxParam(HINST_THISDLL, 
                                                      MAKEINTRESOURCE(DLG_ATTRIBS_RECURSIVE),
                                                      hDlg,
                                                      RecursivePromptDlgProc,
                                                      (LPARAM)pfpsp);
                            }

                            if (bRet)
                                bRet = ApplySingleFileAttributes(pfpsp);

                            if (!bRet)
                            {
                                // the user hit cancel, so we return true to prevent the property sheet from closing
                                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                                return TRUE;
                            }
                            else
                            {
                                // update the size / last accessed time
                                UpdateSizeField(pfpsp, NULL);
                            }
                        }
                    }
                    break;

                case PSN_SETACTIVE:
                    if (pfpsp->fIsLink)
                    {
                        // If this is a link, each time we get set active we need to check to see
                        // if the user applied changes on the link tab that would affect the 
                        // "Opens With:" info.
                        UpdateOpensWithInfo(pfpsp);
                    }
                    break;

                case PSN_QUERYINITIALFOCUS:
                    // Special hack:  We do not want initial focus on the "Rename" or "Change" controls, since 
                    // if the user hit something by accident they would start renaming/modifying the assoc. So 
                    // we set the focus to the "Read-only" control since it is present on all dialogs that use
                    // this wndproc (file, folder, and mounted drive)
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LPARAM)GetDlgItem(hDlg, IDD_READONLY));
                    return TRUE;

                case PSN_LASTCHANCEAPPLY:
                    //
                    // HACKHACK (reinerf)
                    //
                    // I hacked PSN_LASTCHANCEAPPLY into the prsht code so we can get a notification after
                    // every other app has applied, then we can go and do the rename.
                    //
                    // strangely, PSN_LASTCHANCEAPPLY is called even if PSN_APPY returns TRUE.
                    //
                    // we can now safely rename the file, since all the other tabs have
                    // applied their stuff.
                    if (pfpsp->fRename && pfpsp->fCanRename)
                    {
                        // dont bother to check the return value since this is the last-chance,
                        // so the dialog is ending shortly after this
                        ApplyRename(pfpsp, TRUE);
                    }
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

//
// Descriptions:
//   This function creates a property sheet object for the "general" page
//  which shows file system attributes.
//
// Arguments:
//  hDrop           -- specifies the file(s)
//  lpfnAddPage     -- Specifies the callback function.
//  lParam          -- Specifies the lParam to be passed to the callback.
//
// Returns:
//  TRUE if it added any pages
//
// History:
//  12-31-92 SatoNa Created
//
HRESULT FileSystem_AddPages(LPVOID lp, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    LPDATAOBJECT pdtobj = lp;
    HIDA hida;
    BOOL bPageCreated = FALSE;
    HPROPSHEETPAGE hpage;
    FILEPROPSHEETPAGE fpsp;
    STGMEDIUM medium;
    HRESULT hres = NOERROR;
    FORMATETC fmte = {g_cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (FAILED(pdtobj->lpVtbl->GetData(pdtobj, &fmte, &medium)))
        return NOERROR;

    // zero init fpsp
    ZeroMemory(&fpsp, SIZEOF(fpsp));

    fpsp.psp.dwSize      = SIZEOF(fpsp);        // extra data
    fpsp.psp.dwFlags     = PSP_USECALLBACK;
    fpsp.psp.hInstance   = HINST_THISDLL;
    fpsp.psp.pfnCallback = FilePrshtCallback;
    fpsp.fci.bContinue   = TRUE;


    hida = (HIDA)medium.hGlobal;
    ASSERT(hida);
    if (hida)
    {
        fpsp.hida = (HIDA)LocalAlloc( LPTR, (ULONG) GlobalSize((HGLOBAL)hida ));
        if (fpsp.hida)
        {
            CopyMemory( (LPVOID)fpsp.hida, (LPVOID)hida, GlobalSize( (HGLOBAL)hida ) );
        }
    }
    else
    {
        fpsp.hida = NULL;
    }


    if (HIDA_GetCount(hida) == 1)       // single file?
    {   
        // get most of the data we will need (the date/time stuff is not filled in)
        if (HIDA_FillFindData(hida, 0, fpsp.szPath, &fpsp.fci.fd, FALSE))
        {
#ifdef WINNT            
            TCHAR szPathSlash[MAX_PATH];
#endif // WINNT                
            LPITEMIDLIST pidl = HIDA_ILClone(hida, 0);

            if (pidl)
            {
                //  disable renaming here.
                DWORD dwAttrs = SFGAO_CANRENAME;
                if (SUCCEEDED(SHGetNameAndFlags(pidl, 0, NULL, 0, &dwAttrs))
                && !(dwAttrs & SFGAO_CANRENAME))
                {
                    fpsp.fDisableRename = TRUE;
                }
                ILFree(pidl);
            }

            fpsp.psp.pfnDlgProc = SingleFilePrshtDlgProc;

            if (fpsp.fci.fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                UINT uRes;
                fpsp.fIsDirectory = TRUE;
#ifdef WINNT
                // If we are on NT check for HostFolder (folder mounting a volume)
                {
                    //GetVolumeNameFromMountPoint Succeeds then the give path is a mount point
                    TCHAR szVolumeName[MAX_PATH];

                    //Make sure the path ends with a backslash. otherwise the following api wont work
                    lstrcpy(szPathSlash, fpsp.szPath);
                    PathAddBackslash(szPathSlash);
            
                    // Is this a mounted volume at this folder?
                    // this fct will return FALSE if not on NT5 and higher
                    if (GetVolumeNameForVolumeMountPoint(szPathSlash, szVolumeName, ARRAYSIZE(szVolumeName)))
                    {
                        // Yes; show the Mounted Drive Propertysheet instead of normal
                        // folder property sheet
                        // fpsp.fMountedDrive also means NT5 or higher, because this fct will fail otherwise
                        fpsp.fMountedDrive = TRUE;
                    }
                }
#endif // WINNT      
                
                // check to see if it's a folder shortcut
                if (!fpsp.fMountedDrive)
                {
                    // Folder and a shortcut? Must be a folder shortcut!
                    if (PathIsShortcut(fpsp.szPath))
                    {
                        fpsp.fFolderShortcut = TRUE;
                    }
                }

                uRes = DLG_FOLDERPROP;

                if (fpsp.fMountedDrive)
                    uRes = DLG_MOUNTEDDRV_GENERAL;
                else if (fpsp.fFolderShortcut)
                    uRes = DLG_FOLDERSHORTCUTPROP;

                // Load appropriate dialog template
                fpsp.psp.pszTemplate = MAKEINTRESOURCE(uRes);
            }
            else
            {
                // BUGBUG should FONT folder just say !SFGAO_CANRENAME
                if (!fpsp.fDisableRename)
                {
                    TCHAR szFontsFolder[MAX_PATH];

                    // If this is the font folder, then don't allow renaming the files.

                    if (SUCCEEDED(SHGetSpecialFolderPath(NULL, szFontsFolder, CSIDL_FONTS, FALSE)))
                    {
                        if (StrNCmpI(szFontsFolder, fpsp.szPath, lstrlen(szFontsFolder)) == 0)
                        {
                            fpsp.fDisableRename = TRUE;
                        }
                    }
                }

                fpsp.psp.pszTemplate = MAKEINTRESOURCE(DLG_FILEPROP);
            }
       
#ifdef WINNT // file-based compression / encyrption / prompt userlogon / content indexing only supported on NTFS
            {
                DWORD dwVolumeFlags = GetVolumeFlags(fpsp.szPath, 
                                                     fpsp.szFileSys, 
                                                     ARRAYSIZE(fpsp.szFileSys));

                // test for file-based compression.
                if (dwVolumeFlags & FS_FILE_COMPRESSION)
                {
                    // filesystem supports compression
                    fpsp.fIsCompressionAvailable = TRUE;
                }

                // test for file-based encryption.
                if ((dwVolumeFlags & FS_FILE_ENCRYPTION) && AllowedToEncrypt())
                {
                    // filesystem supports encryption
                    fpsp.fIsEncryptionAvailable = TRUE;
                }
                
                //
                // BUGBUG (reinerf) - we dont have a FS_SUPPORTS_INDEXING so we 
                // use the FILE_SUPPORTS_SPARSE_FILES flag, because native index support
                // appeared first on NTFS5 volumes, at the same time sparse file support
                // was implemented.
                //
                if (dwVolumeFlags & FILE_SUPPORTS_SPARSE_FILES)
                {
                    // yup, we are on NTFS5 or greater
                    fpsp.fIsIndexAvailable = TRUE;
                }

                // check to see if we have a .exe and we need to prompt for user logon
                fpsp.fIsExe = PathIsBinaryExe(fpsp.szPath);
            }
#endif // WINNT

            hpage = CreatePropertySheetPage(&fpsp.psp);

            if (hpage)
            {
                if (lpfnAddPage(hpage, lParam))
                {
                    bPageCreated = TRUE;
                    if (AddLinkPage(fpsp.szPath, lpfnAddPage, lParam))
                        hres = ResultFromShort(2);  // set second page default!

                    AddVersionPage(fpsp.szPath, lpfnAddPage, lParam);
                }
                else
                {
                    DestroyPropertySheetPage(hpage);
                }
            }
        }
    }
    else
    {
        // we have multiple files
        fpsp.fMultipleFiles = TRUE;

        //
        // Create a property sheet page for multiple files.
        //
        fpsp.psp.pszTemplate = MAKEINTRESOURCE(DLG_FILEMULTPROP);
        fpsp.psp.pfnDlgProc  = MultiplePrshtDlgProc;

        hpage = CreatePropertySheetPage(&fpsp.psp);
        if (hpage)
        {
            if (!lpfnAddPage(hpage, lParam))
            {
                DestroyPropertySheetPage(hpage);
            }
            else
            {
                bPageCreated = TRUE;
            }
        }
    }

    ReleaseStgMedium(&medium);

    if (!bPageCreated && fpsp.hida)
    {
        LocalFree( (HLOCAL)fpsp.hida );
        ILFree(fpsp.pidlFolder);
        ILFree(fpsp.pidlTarget);
    }

    return hres;
}


#ifdef DEBUG
//
// Type checking
//
const LPFNADDPAGES s_pfnFileSystem_AddPages = FileSystem_AddPages;
#endif
