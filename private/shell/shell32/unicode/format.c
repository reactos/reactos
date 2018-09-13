//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1995.
//
//  File:       format.c
//
//  Contents:   Implements disk formatting from a drive's context menu
//
//  Functions:  SHELL API     SHFormatDrive
//
//              file local    BeginFormat
//              file local    DiableControls
//              file local    EnableControls
//              file local    StuffFormatInfoPtr
//              file local    UnstuffFormatInfoPtr
//              file local    GetFormatInfoPtr
//              file local    FileSysChange
//              file local    FormatCallback
//              file local    FormatDlgProc
//              file local    InitializeFormatDlg
//              file local    LoadFMIFS
//
//  History:    2-13-95   davepl   Created
//             10-15-95   brianau  Added compression setting when fmifs not
//                                 available.
//             10-23-95   brianau  Enabled FMIFS support.  Removed changes
//                                 of 10-15-95.
//
//--------------------------------------------------------------------------

#include "precomp.h"
#pragma  hdrstop

#ifdef WINNT    // Chicago has its own format code

#define WTEXT(quote)         L##quote
#include "oemhard.h"
#include "apithk.h"

#ifdef DEBUG
#define LOAD_STRING(id,buffer)\
{\
    if (0==LoadStringW(HINST_THISDLL, id, buffer, ARRAYSIZE(buffer)))\
        ASSERT(FALSE);\
}
#else
#define LOAD_STRING(id,buffer) LoadStringW(HINST_THISDLL, id, buffer, ARRAYSIZE(buffer))
#endif

const WCHAR cwsz_FAT[]  = WTEXT("FAT");
const WCHAR cwsz_NTFS[] = WTEXT("NTFS");
const WCHAR cwsz_FAT32[] = WTEXT("FAT32");

static DWORD FmtaIds[]={IDOK,               IDH_FORMATDLG_START,
                        IDCANCEL,           IDH_CANCEL,
                        IDC_CAPCOMBO,       IDH_FORMATDLG_CAPACITY,
                        IDC_FSCOMBO,        IDH_FORMATDLG_FILESYS,
                        IDC_ASCOMBO,        IDH_FORMATDLG_ALLOCSIZE,
                        IDC_VLABEL,         IDH_FORMATDLG_LABEL,
                        IDC_GROUPBOX_1,     IDH_COMM_GROUPBOX,
                        IDC_QFCHECK,        IDH_FORMATDLG_QUICKFULL,
                        IDC_ECCHECK,        IDH_FORMATDLG_COMPRESS,
                        IDC_FMTPROGRESS,    IDH_FORMATDLG_PROGRESS,
                        0,0};

static DWORD ChkaIds[]={IDOK,               IDH_CHKDSKDLG_START,
                        IDCANCEL,           IDH_CHKDSKDLG_CANCEL,
                        IDC_GROUPBOX_1,     IDH_COMM_GROUPBOX,
                        IDC_FIXERRORS,      IDH_CHKDSKDLG_FIXERRORS,
                        IDC_RECOVERY,       IDH_CHKDSKDLG_SCAN,
                        IDC_CHKDSKPROGRESS, IDH_CHKDSKDLG_PROGRESS,
                        IDC_PHASE,          -1,
                        0,0};

//
// The following structure encapsulates our calling into the FMIFS.DLL
//
#define HAVE_FMIFS_SUPPORT  1             // Yes, we have FMIFS support.

typedef struct tagFMIFSEntryPoints
{
    HINSTANCE                 hFMIFS_DLL;

#ifdef HAVE_FMIFS_SUPPORT
    PFMIFS_FORMATEX_ROUTINE   FormatEx;
#else
    PFMIFS_FORMAT_ROUTINE   FormatEx;
#endif

    PFMIFS_QSUPMEDIA_ROUTINE  QuerySupportedMedia;

#ifdef HAVE_FMIFS_SUPPORT
    PFMIFS_ENABLECOMP_ROUTINE EnableVolumeCompression;
#else
#endif

    PFMIFS_CHKDSKEX_ROUTINE   ChkDskEx;
} FMIFS, * LPFMIFS;

//
// This structure described the current formatting session
//

typedef struct tagFormatInfo
{
    UINT    drive;                 // 0-based index of drive to format
    UINT    fmtID;                 // Last format ID
    UINT    options;               // options passed to us via the API
    LPFMIFS pFMIFS;                // ptr to FMIFS structure, above
    HWND    hDlg;                  // handle to the format dialog
    BOOL    fIsFloppy;             // TRUE -> its a floppy
    BOOL    fEnableComp;           // Last "Enable Comp" choice from user
    BOOL    fCancelled;            // User cancelled the last format
    BOOL    fShouldCancel;         // User has clicked cancel; pending abort
    BOOL    fWasFAT;               // Was it FAT originally?
    BOOL    fFinishedOK;           // Did format complete sucessfully?
    BOOL    fErrorAlready;         // Did we put up an error dialog already?
    DWORD   dwClusterSize;         // Orig NT cluster size, or last choice
    WCHAR   wszVolName[MAX_PATH];  // Volume Label
    WCHAR   wszWinTitle[MAX_PATH]; // Format dialog window title
    WCHAR   wszDriveName[4];       // Root path to drive (eg: A:\)
    HANDLE  hThread;               // Handle of format thread

    // Array of media types supported by the device
#ifdef WINNT
    // for NT5, we have an expanded list that includes japanese types.
    FMIFS_MEDIA_TYPE rgMedia[IDS_FMT_MEDIA_J21-IDS_FMT_MEDIA_J0];
#else
    FMIFS_MEDIA_TYPE rgMedia[IDS_FMT_MEDIA_11-IDS_FMT_MEDIA_0];
#endif

    // Used to cache the enabled/disabled state of the dialog controls
    BOOL    rgfControlEnabled[DLG_FORMATDISK_NUMCONTROLS];

} FORMATINFO, * LPFORMATINFO;

//
// An enumeration to make the filesystem combo-box code more readble
//

typedef enum tagFILESYSENUM
{
    e_FAT = 0,
    e_NTFS,
    e_FAT32
} FILESYSENUM;

//
// Private WM_USER messages we will use.  For some unknown reason, USER sends
// us a WM_USER during initialization, so I start my private messages at
// WM_USER + 0x0100
//

typedef enum tagPRIVMSGS
{
    PWM_FORMATDONE = WM_USER + 0x0100,
    PWM_CHKDSKDONE
} PRIVMSGS;

#define cdw_Kilobyte (1024)
#define cdw_Megabyte (1024 * 1024)
#define cdw_Gigabyte (1024 * 1024 * 1024)

#define cdwTHREADWAIT (10000)       // Wait 10 seconds for fmt thread to exit
                                    // before asking retry / killing it



//+-------------------------------------------------------------------------
//
//  Function:   LoadFMIFS
//
//  Synopsis:   Loads FMIFS.DLL and sets up the function entry points for
//              the member functions we are interested in.
//
//  Arguments:  [pFEP] -- Pointer to a FMIFSEntryPoints struc
//
//  Returns:    HRESULT
//
//  History:    2-15-95   davepl   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT LoadFMIFS(LPFMIFS pFMIFS)
{
    HRESULT hr = S_OK;

    //
    // Load the FMIFS DLL and query for the entry points we need
    //

    if (NULL == (pFMIFS->hFMIFS_DLL = LoadLibrary(WTEXT("FMIFS.DLL"))))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

#ifdef HAVE_FMIFS_SUPPORT
    else if (NULL == (pFMIFS->FormatEx = (PFMIFS_FORMATEX_ROUTINE)
                GetProcAddress(pFMIFS->hFMIFS_DLL, "FormatEx")))
#else
    else if (NULL == (pFMIFS->FormatEx = (PFMIFS_FORMAT_ROUTINE)
                GetProcAddress(pFMIFS->hFMIFS_DLL, "Format")))
#endif

    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else if (NULL == (pFMIFS->QuerySupportedMedia = (PFMIFS_QSUPMEDIA_ROUTINE)
                GetProcAddress(pFMIFS->hFMIFS_DLL, "QuerySupportedMedia")))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
#ifdef HAVE_FMIFS_SUPPORT
    else if (NULL == (pFMIFS->EnableVolumeCompression = (PFMIFS_ENABLECOMP_ROUTINE)
                GetProcAddress(pFMIFS->hFMIFS_DLL, "EnableVolumeCompression")))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
#endif
    else if (NULL == (pFMIFS->ChkDskEx = (PFMIFS_CHKDSKEX_ROUTINE)
                GetProcAddress(pFMIFS->hFMIFS_DLL, "ChkdskEx")))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // If anything failed, and we've got the DLL loaded, release the DLL
    //

    if (hr != S_OK && pFMIFS->hFMIFS_DLL)
    {
       FreeLibrary(pFMIFS->hFMIFS_DLL);
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   StuffFormatInfoPtr()
//
//  Synopsis:   Allocates a thread-local index slot for this thread's
//              FORMATINFO pointer, if the index doesn't already exist.
//              In any event, stores the FORMATINFO pointer in the slot
//              and increments the index's usage count.
//
//  Arguments:  [pFormatInfo] -- The pointer to store
//
//  Returns:    HRESULT
//
//  History:    2-20-95   davepl   Created
//
//--------------------------------------------------------------------------

//
// Thread-Local Storage index for our FORMATINFO structure pointer
//

static DWORD g_iTLSFormatInfo = 0;
static LONG  g_cTLSFormatInfo = 0;  // Usage count

HRESULT StuffFormatInfoPtr(LPFORMATINFO pFormatInfo)
{
    HRESULT hr = S_OK;

    //
    // Allocate an index slot for our thread-local FORMATINFO pointer, if one
    // doesn't already exist, then stuff our FORMATINFO ptr at that index.
    //

    ENTERCRITICAL;
    if (0 == g_iTLSFormatInfo)
    {
        if (0xFFFFFFFF == (g_iTLSFormatInfo = TlsAlloc()))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        g_cTLSFormatInfo = 0;
    }
    if (S_OK == hr)
    {
        if (TlsSetValue(g_iTLSFormatInfo, (LPVOID) pFormatInfo))
        {
           g_cTLSFormatInfo++;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    LEAVECRITICAL;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   UnstuffFormatInfoPtr()
//
//  Synopsis:   Decrements the usage count on our thread-local storage
//              index, and if it goes to zero the index is free'd
//
//  Arguments:  [none]
//
//  Returns:    none
//
//  History:    2-20-95   davepl   Created
//
//--------------------------------------------------------------------------

__inline void UnstuffFormatInfoPtr()
{
    ENTERCRITICAL;
    if (0 == --g_cTLSFormatInfo)
    {
        TlsFree(g_iTLSFormatInfo);
        g_iTLSFormatInfo = 0;
    }
    LEAVECRITICAL;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetFormatInfoPtr()
//
//  Synopsis:   Retrieves this threads FORMATINFO ptr by grabbing the
//              thread-local value previously stuff'd
//
//  Arguments:  [none]
//
//  Returns:    The pointer, of course
//
//  History:    2-20-95   davepl   Created
//
//--------------------------------------------------------------------------

__inline LPFORMATINFO GetFormatInfoPtr()
{
    return TlsGetValue(g_iTLSFormatInfo);
}

//+-------------------------------------------------------------------------
//
//  Function:   DisableControls
//
//  Synopsis:   Ghosts all controls except "Cancel", saving their
//              previous state in the FORMATINFO structure
//
//  Arguments:  [pFormatInfo] -- Describes a format dialog session
//
//  History:    2-14-95   davepl   Created
//
//  Notes:      Also changes "Close" button text to read "Cancel"
//
//--------------------------------------------------------------------------

void DisableControls(LPFORMATINFO pFormatInfo)
{
    WCHAR wszCancel[64];
    int i;

    for (i = 0; i < DLG_FORMATDISK_NUMCONTROLS; i++)
    {
        HWND hControl = GetDlgItem(pFormatInfo->hDlg, i + DLG_FORMATDISK_FIRSTCONTROL);

        pFormatInfo->rgfControlEnabled[i] = IsWindowEnabled(hControl);
        EnableWindow(hControl, FALSE);
    }

    EnableWindow(GetDlgItem(pFormatInfo->hDlg, IDOK), FALSE);

    LOAD_STRING( IDS_FMT_CANCEL, wszCancel );
    SetWindowText(GetDlgItem(pFormatInfo->hDlg, IDCANCEL), wszCancel );
}

//+-------------------------------------------------------------------------
//
//  Function:   EnableControls
//
//  Synopsis:   Restores controls to the enabled/disabled state they were
//              before a previous call to DisableControls().
//
//  Arguments:  [pFormatInfo] -- Decribes a format dialog session
//
//  History:    2-14-95   davepl   Created
//
//  Notes:      Undefined behaviour if DisableControls has not been called
//              Also changes "Cancel" button to say "Close"
//              Also sets focus to Cancel button instead of Start button
//
//--------------------------------------------------------------------------

void EnableControls(LPFORMATINFO pFormatInfo)
{
    WCHAR wszClose[64];
    int i;
    HWND hwnd;

    for (i = 0; i < DLG_FORMATDISK_NUMCONTROLS; i++)
    {
        HWND hControl = GetDlgItem(pFormatInfo->hDlg, i + DLG_FORMATDISK_FIRSTCONTROL);
        EnableWindow(hControl, pFormatInfo->rgfControlEnabled[i]);
    }
    hwnd = GetDlgItem(pFormatInfo->hDlg, IDOK);
    EnableWindow(hwnd, TRUE);
    SendMessage(hwnd, BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE,0));
    
    LOAD_STRING( IDS_FMT_CLOSE, wszClose );
    hwnd = GetDlgItem(pFormatInfo->hDlg, IDCANCEL);
    SetWindowText(hwnd, wszClose);
    SendMessage(hwnd, BM_SETSTYLE, BS_DEFPUSHBUTTON, MAKELPARAM(TRUE,0));
    SetFocus(hwnd);
}

//+-------------------------------------------------------------------------
//
//  Function:   SetWindowTitle
//
//  Synopsis:   Sets the format dialog's title to "Format A:" or
//              "Formatting A:" depending on the drive letter and the
//              fInProgress flag.
//
//  Arguments:  [pFormatInfo] -- Carries the drive letter
//              [fInProgress] -- TRUE => currently formatting
//
//  History:    2-14-95   davepl   Created
//
//
//--------------------------------------------------------------------------

void SetWindowTitle(LPFORMATINFO pFormatInfo, BOOL fInProgress)
{
    LOAD_STRING( fInProgress ? IDS_FMT_FORMATTING:IDS_FMT_FORMAT, pFormatInfo->wszWinTitle );

    lstrcat(pFormatInfo->wszWinTitle, pFormatInfo->wszDriveName);
    pFormatInfo->wszDriveName[lstrlen(pFormatInfo->wszDriveName)] = WTEXT('\0');

    SetWindowText(pFormatInfo->hDlg, pFormatInfo->wszWinTitle);
}

//+-------------------------------------------------------------------------
//
//  Function:   FileSysChange
//
//  Synopsis:   Called when a user picks a filesystem in the dialog, this
//              sets the states of the other relevant controls, such as
//              Enable Compression, Allocation Size, etc.
//
//  Arguments:  [n]           -- One of e_FAT, e_NTFS, or e_FAT32
//              [pFormatInfo] -- Current format dialog session
//
//  History:    2-14-95   davepl   Created
//
//--------------------------------------------------------------------------

void FileSysChange(FILESYSENUM n, LPFORMATINFO pFormatInfo)
{
    WCHAR wszTmp[MAX_PATH];
    int i;

    switch(n)
    {
        case e_FAT:
        case e_FAT32:
            // Clean & Diable the Enable Compression option

            SendDlgItemMessage(pFormatInfo->hDlg, IDC_ECCHECK, BM_SETCHECK, FALSE, 0);
            EnableWindow(GetDlgItem(pFormatInfo->hDlg, IDC_ECCHECK), FALSE);

            SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_RESETCONTENT, 0, 0);
            
            LOAD_STRING(IDS_FMT_ALLOC0, wszTmp );
            SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_ADDSTRING, 0, (LPARAM)wszTmp);
            SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_SETCURSEL, 0, 0);

            break;
            
        case e_NTFS:
            EnableWindow(GetDlgItem(pFormatInfo->hDlg, IDC_ECCHECK), TRUE);
            SendDlgItemMessage(pFormatInfo->hDlg, IDC_ECCHECK, BM_SETCHECK, pFormatInfo->fEnableComp, 0);

            // Set up the NTFS Allocation choices, and select the current choice

            SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_RESETCONTENT, 0, 0);

            for ( i = IDS_FMT_ALLOC0 ; i <= IDS_FMT_ALLOC4 ; i++ )
            {
                LOAD_STRING( i, wszTmp );
                SendDlgItemMessage( pFormatInfo->hDlg, IDC_ASCOMBO, CB_ADDSTRING, 0, (LPARAM)wszTmp );
            }

            switch(pFormatInfo->dwClusterSize)
            {
                case 512:
                    SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_SETCURSEL, 1, 0);
                    break;

                case 1024:
                    SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_SETCURSEL, 2, 0);
                    break;

                case 2048:
                    SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_SETCURSEL, 3, 0);
                    break;

                case 4096:
                    SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_SETCURSEL, 4, 0);
                    break;

                default:
                    SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_SETCURSEL, 0, 0);
                    break;

            }

            break;

    } // switch(n)
}
    
//+-------------------------------------------------------------------------
//
//  Function:   InitializeFormatDlg
//
//  Synopsis:   Initializes the format dialog to a default state.  Examines
//              the disk/partition to obtain default values.
//
//  Arguments:  [hDlg]        -- Handle to the format dialog
//              [pFormatInfo] -- Describes current format session
//
//  Returns:    HRESULT
//
//  History:    2-14-95   davepl   Created
//
//--------------------------------------------------------------------------

HRESULT InitializeFormatDlg(LPFORMATINFO pFormatInfo)
{
    HRESULT          hr              = S_OK;
    ULONG            cMedia;
    HWND             hCapacityCombo;
    HWND             hFilesystemCombo;
    HWND             hDlg = pFormatInfo->hDlg;
    WCHAR            wszBuffer[256];

    //
    // Set up some typical default values
    //

    pFormatInfo->fEnableComp       = FALSE;
    pFormatInfo->dwClusterSize     = 0;
    pFormatInfo->fIsFloppy         = TRUE;
    pFormatInfo->fWasFAT           = TRUE;
    pFormatInfo->fFinishedOK       = FALSE;
    pFormatInfo->fErrorAlready     = FALSE;
    pFormatInfo->wszVolName[0]     = WTEXT('\0');

    //
    // Initialize the Quick Format checkbox based on option passed to
    // the SHFormatDrive() API
    //

    SendDlgItemMessage(hDlg, IDC_QFCHECK, BM_SETCHECK,
                       (pFormatInfo->options & SHFMT_OPT_FULL) ? TRUE : FALSE,
                       0);
    //
    // Set the dialog title to indicate which drive we are dealing with
    //

    lstrcpyW(pFormatInfo->wszDriveName, WTEXT("A:\\"));
    ASSERT(pFormatInfo->drive < 26);
    pFormatInfo->wszDriveName[0] += (WCHAR) pFormatInfo->drive;

    SetWindowTitle(pFormatInfo, FALSE);

    //
    // Query the supported media types for the drive in question
    //

    if (FALSE == pFormatInfo->pFMIFS->QuerySupportedMedia(pFormatInfo->wszDriveName,
                                                          pFormatInfo->rgMedia,
                                                          ARRAYSIZE(pFormatInfo->rgMedia),
                                                          &cMedia))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // For each of the formats that the drive can handle, add a selection
    // to the capcity combobox.
    //

    if (S_OK == hr)
    {
        ULONG i,j;
        hCapacityCombo = GetDlgItem(hDlg, IDC_CAPCOMBO);
        hFilesystemCombo = GetDlgItem(hDlg, IDC_FSCOMBO);
        ASSERT(hCapacityCombo && hFilesystemCombo);

        //
        // Strip out weird media types
        //
        j = 0;
        for ( i = 0; i < cMedia; i++)
        {
            if (pFormatInfo->rgMedia[i] != FmMediaF5_160_512 &&
                pFormatInfo->rgMedia[i] != FmMediaF5_180_512 &&
                pFormatInfo->rgMedia[i] != FmMediaF5_320_512 &&
                pFormatInfo->rgMedia[i] != FmMediaF5_320_1024 )
            {
                // Ok, its not a weird one...
                pFormatInfo->rgMedia[j] = pFormatInfo->rgMedia[i];
                j++;
            }
        }
        cMedia = j;

        for (i = 0; i < cMedia; i++)
        {
            //
            // If we find any non-floppy format, clear the fIsFloppy flag
            //

            if (FmMediaFixed == pFormatInfo->rgMedia[i] || FmMediaRemovable == pFormatInfo->rgMedia[i])
            {
                pFormatInfo->fIsFloppy = FALSE;
            }

            //
            // For fixed media we query the size, for floppys we present
            // a set of options supported by the drive
            //

            if (FmMediaFixed == pFormatInfo->rgMedia[i] || (FmMediaRemovable == pFormatInfo->rgMedia[i]))
            {
                DWORD dwSectorsPerCluster,
                      dwBytesPerSector,
                      dwFreeClusters,
                      dwClusters;

                if (GetDiskFreeSpace(pFormatInfo->wszDriveName,
                                     &dwSectorsPerCluster,
                                     &dwBytesPerSector,
                                     &dwFreeClusters,
                                     &dwClusters))
                {
                    __int64 iCapacity = (__int64)dwSectorsPerCluster * 
                                        (__int64)dwBytesPerSector *
                                        (__int64)dwClusters;

                    pFormatInfo->dwClusterSize = dwBytesPerSector * dwSectorsPerCluster;

                    ShortSizeFormat64(iCapacity, wszBuffer);


                    // Add a capacity desciption to the combobox

                    SendMessage(hCapacityCombo,
                                CB_ADDSTRING,
                                0,
                                (LPARAM) wszBuffer);
                }
                else
                {
                    // Couldn't get the free space... prob. not fatal

                    LOAD_STRING( IDS_FMT_CAPUNKNOWN, wszBuffer );
                    SendMessage(hCapacityCombo,
                                CB_ADDSTRING,
                                0,
                                (LPARAM)wszBuffer);

                }

            }
            else // removable media...
            {
                // Add a capacity desciption to the combo
                UINT ids0 = IDS_FMT_MEDIA0 ;  
                        // base ID of sequential media format descriptors
#ifdef WINNT
                if( IsNEC_PC9800() )
                    // use japanese media format descriptors
                    ids0 = IDS_FMT_MEDIA_J0 ;
#endif //WINNT
                LOAD_STRING(ids0 + pFormatInfo->rgMedia[i], wszBuffer );
                SendMessage(hCapacityCombo, CB_ADDSTRING, 0, (LPARAM)wszBuffer);
            }
        }

        SendMessage(hCapacityCombo, CB_SETCURSEL, 0, 0);
    }

    //
    // Add the appropriate filesystem selections to the combobox
    //

    if (S_OK == hr)
    {
        // add the file system types

        SendMessage(hFilesystemCombo, CB_ADDSTRING, 0, (LPARAM)cwsz_FAT );

        if (FALSE == pFormatInfo->fIsFloppy)
        {
            SendMessage(hFilesystemCombo, CB_ADDSTRING, 0, (LPARAM)cwsz_NTFS);

            if (g_bRunOnNT5)
                SendMessage(hFilesystemCombo, CB_ADDSTRING, 0, (LPARAM)cwsz_FAT32);
        }

        // By default, pick FAT (entry 0 in the _nonsorted_ combobox)

        SendMessage(hFilesystemCombo, CB_SETCURSEL, e_FAT, 0);
        FileSysChange(e_FAT, pFormatInfo);
    }

    // If we can determine something other than FAT is being used,
    // select it as the default in the combobox

    if (S_OK == hr)
    {
        UINT olderror = SetErrorMode(SEM_FAILCRITICALERRORS);

        if (GetVolumeInformation(pFormatInfo->wszDriveName,
                                 pFormatInfo->wszVolName, ARRAYSIZE(pFormatInfo->wszVolName),
                                 NULL,
                                 NULL,
                                 NULL,
                                 wszBuffer, ARRAYSIZE(wszBuffer)))
        {
            //
            // If we got a current volume label, stuff it in the edit control
            //

            if (pFormatInfo->wszVolName[0] != WTEXT('\0'))
            {
                SetWindowText(GetDlgItem(pFormatInfo->hDlg, IDC_VLABEL), pFormatInfo->wszVolName);
            }

            if (!pFormatInfo->fIsFloppy)
            {
                if (0 == lstrcmpi(cwsz_NTFS, wszBuffer))
                {
                    SendMessage(hFilesystemCombo, CB_SETCURSEL, e_NTFS, 0);
                    pFormatInfo->fWasFAT = FALSE;
                    FileSysChange(e_NTFS, pFormatInfo);
                }
                else if (0 == lstrcmpi(cwsz_FAT32, wszBuffer))
                {
                    SendMessage(hFilesystemCombo, CB_SETCURSEL, e_FAT32, 0);
                    pFormatInfo->fWasFAT = TRUE;
                    pFormatInfo->dwClusterSize = 0;
                    FileSysChange(e_FAT32, pFormatInfo);
                }
                else // if (0 == lstrcmpi(cwsz_FAT, wszBuffer))
                {
                    SendMessage(hFilesystemCombo, CB_SETCURSEL, e_FAT, 0);
                    pFormatInfo->fWasFAT = TRUE;
                    pFormatInfo->dwClusterSize = 0;
                    FileSysChange(e_FAT, pFormatInfo);
                }
            }
            
            // BUGBUG - What about specialized file-systems?  Don't care for now.
        }
    }

    //
    // If the above failed due to disk not in drive, notify the user
    //

    if (FAILED(hr))
    {
        switch (HRESULT_CODE(hr))
        {
        case ERROR_NOT_READY:
            ShellMessageBox(HINST_THISDLL,
                    hDlg,
                    MAKEINTRESOURCE(IDS_DRIVENOTREADY),
                    NULL,
                    MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK,
                    pFormatInfo->wszDriveName[0]);
            break;

        case ERROR_ACCESS_DENIED:
            ShellMessageBox(HINST_THISDLL,
                    hDlg,
                    MAKEINTRESOURCE(IDS_ACCESSDENIED),
                    NULL,
                    MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK,
                    pFormatInfo->wszDriveName[0]);
            break;

        case ERROR_WRITE_PROTECT:
            ShellMessageBox(HINST_THISDLL,
                    hDlg,
                    MAKEINTRESOURCE(IDS_WRITEPROTECTED),
                    NULL,
                    MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK,
                    pFormatInfo->wszDriveName[0]);
            break;
        }
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   FormatCallback
//
//  Synopsis:   Called from within the FMIFS DLL's Format function, this
//              updates the format dialog's status bar and responds to
//              format completion/error notifications.
//
//  Arguments:  [PacketType]   -- Type of packet (ie: % complete, error, etc)
//              [PacketLength] -- Size, in bytes, of the packet
//              [pPacketData]  -- Pointer to the packet
//
//  Returns:    BOOLEAN continuation value
//
//  History:    2-14-95   davepl   Created
//
//--------------------------------------------------------------------------

BOOLEAN FormatCallback(FMIFS_PACKET_TYPE PacketType,
                       ULONG             PacketLength,
                       PVOID             pPacketData)
{
    UINT iMessageID = IDS_FORMATFAILED;
    BOOL fFailed = FALSE;
    LPFORMATINFO pFormatInfo;

    ASSERT(g_iTLSFormatInfo);

    //
    // Grab the FORMATINFO structure for this thread
    //

    if (NULL == (pFormatInfo = GetFormatInfoPtr()))
    {
        return FALSE;
    }

    //
    // If the user has signalled to abort the format, return
    // FALSE out of here right now
    //

    if (pFormatInfo->fShouldCancel)
    {
        pFormatInfo->fCancelled = TRUE;

        return FALSE;
    }

    //
    // I could table-drive this, but it compiles surprisingly well...
    //

    switch(PacketType)
    {
        case FmIfsIncompatibleFileSystem:
            fFailed    = TRUE;
            iMessageID = IDS_INCOMPATIBLEFS;
            break;

        case FmIfsIncompatibleMedia:
            fFailed    = TRUE;
            iMessageID = IDS_INCOMPATIBLEMEDIA;
            break;

        case FmIfsAccessDenied:
            fFailed    = TRUE;
            iMessageID = IDS_ACCESSDENIED;
            break;

        case FmIfsMediaWriteProtected:
            fFailed    = TRUE;
            iMessageID = IDS_WRITEPROTECTED;
            break;

        case FmIfsCantLock:
            fFailed    = TRUE;
            iMessageID = IDS_CANTLOCK;
            break;

        case FmIfsCantQuickFormat:
            fFailed    = TRUE;
            iMessageID = IDS_CANTQUICKFORMAT;
            break;

        case FmIfsIoError:
            fFailed    = TRUE;
            iMessageID = IDS_IOERROR;
            // FUTURE Consider showing head/track etc where error was
            break;

        case FmIfsBadLabel:
            fFailed    = TRUE;
            iMessageID = IDS_BADLABEL;
            break;

        case FmIfsPercentCompleted:
        {
            FMIFS_PERCENT_COMPLETE_INFORMATION * pPercent =
              (FMIFS_PERCENT_COMPLETE_INFORMATION *) pPacketData;
            
            SendDlgItemMessage(pFormatInfo->hDlg, IDC_FMTPROGRESS,
                               PBM_SETPOS,
                               pPercent->PercentCompleted, 0);
            break;
        }

        case FmIfsFinished:
        {
            //
            // Format is done; check for failure or success
            //

            FMIFS_FINISHED_INFORMATION * pFinishedInfo =
              (FMIFS_FINISHED_INFORMATION *) pPacketData;

            pFormatInfo->fFinishedOK =  pFinishedInfo->Success;

            if (pFinishedInfo->Success)
            {
                //
                // If "Enable Compression" is checked, try to enable filesystem compression
                //

                if (SendDlgItemMessage(pFormatInfo->hDlg, IDC_ECCHECK, BM_GETCHECK, 0, 0))
                {
                    BOOL bStatus = FALSE;
#ifdef HAVE_FMIFS_SUPPORT
                    bStatus = pFormatInfo->pFMIFS->EnableVolumeCompression(pFormatInfo->wszDriveName,
                                                                           COMPRESSION_FORMAT_DEFAULT);
#endif
                    if (FALSE == bStatus)

                    {
                        ShellMessageBox(HINST_THISDLL,
                                        pFormatInfo->hDlg,
                                        MAKEINTRESOURCE(IDS_CANTENABLECOMP),
                                        NULL,
                                        MB_SETFOREGROUND | MB_ICONINFORMATION | MB_OK);
                    }
                }
                //
                // Even though its a quick format, the progress meter should
                // show 100% when the "Format Complete" requester is up
                //

                SendDlgItemMessage(pFormatInfo->hDlg, IDC_FMTPROGRESS,
                                   PBM_SETPOS,
                                   100 /* Percent Complete */, 0);

                // FUTURE Consider showing format stats, ie: ser no, bytes, etc

                ShellMessageBox(HINST_THISDLL,
                                pFormatInfo->hDlg,
                                MAKEINTRESOURCE(IDS_FORMATCOMPLETE),
                                NULL,
                                MB_SETFOREGROUND | MB_ICONINFORMATION | MB_OK);

                //
                // Restore the dialog title, reset progress and flags
                //

                SendDlgItemMessage(pFormatInfo->hDlg, IDC_FMTPROGRESS,
                                   PBM_SETPOS,
                                    0 /* Reset Percent Complete */, 0);

                //
                // Set the focus onto the Close button
                // 

                pFormatInfo->fCancelled = FALSE;
            }
            else
            {
                fFailed = TRUE;
            }
            break;
        }
    }

    //
    // If we received any kind of failure information, put up a final
    // "Format Failed" message. UNLESS we've already put up some nice message
    //

    if (fFailed && !pFormatInfo->fErrorAlready)
    {
        ShellMessageBox(HINST_THISDLL,
                        pFormatInfo->hDlg,
                        MAKEINTRESOURCE(iMessageID),
                        NULL,
                        MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK);
        pFormatInfo->fErrorAlready = TRUE;
    }

    return (BOOLEAN) (fFailed == FALSE);
}

//+-------------------------------------------------------------------------
//
//  Function:   BeginFormat
//
//  Synopsis:   Spun off as its own thread, this ghosts all controls in the
//              dialog except "Cancel", then does the actual format
//
//  Arguments:  [pIn] -- FORMATINFO structure pointer as a void *
//
//  Returns:    HRESULT thread exit code
//
//  History:    2-14-95   davepl   Created
//
//--------------------------------------------------------------------------

DWORD WINAPI BeginFormat(LPVOID pIn)
{
    LPFORMATINFO pFormatInfo = pIn;
    int n;
    FMIFS_MEDIA_TYPE MediaType;
    LPCWSTR pwszFileSystemName;
    BOOLEAN fQuickFormat;
    HRESULT hr = S_OK;
    LPITEMIDLIST pidlFormat;

    //
    // Save the FORAMTINFO ptr for this thread, to be used in the format
    // callback function
    //

    if (S_OK != (hr = StuffFormatInfoPtr(pFormatInfo)))
    {
        PostMessage(pFormatInfo->hDlg, (UINT) PWM_FORMATDONE, 0, 0);
        return (DWORD) hr;
    }

    //
    // Set the window title to indicate format in proress...
    //

    SetWindowTitle(pFormatInfo, TRUE);

    //
    // Determine the user's choice of filesystem
    //

    n = (int) SendDlgItemMessage(pFormatInfo->hDlg, IDC_FSCOMBO, CB_GETCURSEL, 0, 0);

    switch((FILESYSENUM) n)
    {
        case e_FAT:
            pwszFileSystemName = cwsz_FAT;
            break;

        case e_FAT32:
            pwszFileSystemName = cwsz_FAT32;
            break;

        case e_NTFS:
            pwszFileSystemName = cwsz_NTFS;
            break;
    }

    //
    // Determine the user's choice of media formats
    //

    n = (int) SendDlgItemMessage(pFormatInfo->hDlg, IDC_CAPCOMBO, CB_GETCURSEL, 0, 0);
    MediaType = pFormatInfo->rgMedia[n];

    //
    // Get the cluster size.  First selection ("Use Default") yields a zero,
    // while the next 4 select 512, 1024, 2048, or 4096
    //

    n = (int) SendDlgItemMessage(pFormatInfo->hDlg, IDC_ASCOMBO, CB_GETCURSEL, 0, 0);
    pFormatInfo->dwClusterSize = n ? (256 << n) : 0;

    //
    // Quickformatting?
    //

    fQuickFormat = (BOOLEAN) SendDlgItemMessage(pFormatInfo->hDlg, IDC_QFCHECK, BM_GETCHECK, 0, 0);

    //
    // Clear the error state.
    //

    pFormatInfo->fErrorAlready = FALSE;

    //
    //  Tell the shell to get ready...  Announce that the media is no
    //  longer valid (so people who have active views on it will navigate
    //  away) and tell the shell to close its FindFirstChangeNotifications.
    //

    SHILCreateFromPath(pFormatInfo->wszDriveName, &pidlFormat, NULL);
    if (pidlFormat) {
        SHChangeNotify(SHCNE_MEDIAREMOVED, SHCNF_IDLIST | SHCNF_FLUSH, pidlFormat, 0);
        SHChangeNotifySuspendResume(TRUE, pidlFormat, TRUE, 0);
    }

    //
    // Do the format.
    //

#ifdef HAVE_FMIFS_SUPPORT
    pFormatInfo->pFMIFS->FormatEx(pFormatInfo->wszDriveName,
                                  MediaType,
                                  (PWSTR) pwszFileSystemName,
                                  pFormatInfo->wszVolName,
                                  fQuickFormat,
                                  pFormatInfo->dwClusterSize,
                                  FormatCallback);
#else
    pFormatInfo->pFMIFS->FormatEx(pFormatInfo->wszDriveName,
                                  MediaType,
                                  (PWSTR) pwszFileSystemName,
                                  pFormatInfo->wszVolName,
                                  fQuickFormat,
                                  FormatCallback);
#endif

    //
    //  Wake the shell back up.
    //
    if (pidlFormat)
    {
        SHChangeNotifySuspendResume(FALSE, pidlFormat, TRUE, 0);
        ILFree(pidlFormat);
    }

    //
    // See next comment
    //

    InvalidateDriveType(pFormatInfo->drive);

    //
    // Success or failure, we should fire a notification on the disk
    // since we don't really know the state after the format
    //

    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW, (LPVOID) pFormatInfo->wszDriveName, NULL);

    //
    // Release the TLS index
    //

    UnstuffFormatInfoPtr();

    //
    // Post a message back to the DialogProc thread to let it know
    // the format is done.  We post the message since otherwise the
    // DialogProc thread will be too busy waiting for this thread
    // to exit to be able to process the PWM_FORMATDONE message
    // immediately.
    //

    PostMessage(pFormatInfo->hDlg, (UINT) PWM_FORMATDONE, 0, 0);
    return (DWORD) S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   FormatDlgProc
//
//  Synopsis:   DLGPROC for the format dialog
//
//  Arguments:  [hDlg]   -- Typical
//              [wMsg]   -- Typical
//              [wParam] -- Typical
//              [lParam] -- For WM_INIT, carries the FORMATINFO structure
//                          pointer passed to DialogBoxParam() when the
//                          dialog was created.
//
//  History:    2-14-95   davepl   Created
//
//--------------------------------------------------------------------------

BOOL_PTR CALLBACK FormatDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr   = S_OK;
    int     ID   = GET_WM_COMMAND_ID(wParam, lParam);
    int     CMD  = GET_WM_COMMAND_CMD(wParam, lParam);

    // Grab our previously cached pointer to the FORMATINFO struct (see WM_INITDIALOG)

    LPFORMATINFO pFormatInfo = (LPFORMATINFO) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) {

    case PWM_FORMATDONE:
    {
        //
        // Format is done.  Reset the window title and clear the progress meter
        //

        SetWindowTitle(pFormatInfo, FALSE);
        SendDlgItemMessage(pFormatInfo->hDlg, IDC_FMTPROGRESS, PBM_SETPOS, 0 /* Reset Percent Complete */, 0);
        EnableControls(pFormatInfo);

        if (pFormatInfo->fCancelled)
        {
            ShellMessageBox(HINST_THISDLL,
                            pFormatInfo->hDlg,
                            MAKEINTRESOURCE(IDS_FORMATCANCELLED),
                            NULL,
                            MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK);
            pFormatInfo->fCancelled = FALSE;
        }
        CloseHandle(pFormatInfo->hThread);
        pFormatInfo->hThread = NULL;
        break;
    }

    case WM_INITDIALOG:
    {
        //
        // Initialize the dialog and cache the FORMATINFO structure's pointer
        // as our dialog's DWLP_USER data
        //

        pFormatInfo = (LPFORMATINFO) lParam;
        pFormatInfo->hDlg = hDlg;
        if (FAILED(InitializeFormatDlg(pFormatInfo)))
        {
            EndDialog(hDlg, 0);
            return -1;
        }
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        break;
    }

    case WM_DESTROY:
        break;

    case WM_COMMAND:
    {
        switch(CMD)
        {
            //
            // User made a selection in one of the combo boxes
            //

            case CBN_SELCHANGE:

                if (IDC_FSCOMBO == ID)
                {
                    //
                    // User selected a filesystem... update the rest of the dialog
                    // based on this choice
                    //

                    HWND hFilesystemCombo = (HWND) lParam;
                    int n = (WORD) SendMessage(hFilesystemCombo, CB_GETCURSEL, 0, 0);
                    FileSysChange((FILESYSENUM) n, pFormatInfo);
                }

            //
            // Codepath for controls other than combo boxes...
            //

            default:

                switch (GET_WM_COMMAND_ID(wParam, lParam))
                {

                case IDC_ECCHECK:
                    pFormatInfo->fEnableComp = (BOOL) SendMessage((HWND) lParam, BM_GETCHECK, 0, 0);
                    break;

                case IDOK:
                {
                    DWORD   dwThreadID;

                    //
                    // Get user verification for format, break out on CANCEL
                    //

                    if (IDCANCEL == ShellMessageBox(HINST_THISDLL,
                                                    hDlg,
                                                    MAKEINTRESOURCE(IDS_OKTOFORMAT),
                                                    NULL,
                                                    MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OKCANCEL))
                    {
                        break;
                    }

                    DisableControls(pFormatInfo);
                    pFormatInfo->fCancelled = FALSE;
                    pFormatInfo->fShouldCancel = FALSE;
                    GetWindowText(GetDlgItem(pFormatInfo->hDlg, IDC_VLABEL), pFormatInfo->wszVolName, MAX_PATH);
                    pFormatInfo->hThread = CreateThread( NULL,
                                                         0,
                                                         BeginFormat,
                                                         (LPVOID) pFormatInfo,
                                                         0,
                                                         &dwThreadID );
                    break;
                }

                case IDCANCEL:

                    //
                    // If the format thread is running, wait for it.  If not,
                    // exit the dialog
                    //

                    pFormatInfo->fShouldCancel = TRUE;
                    if (pFormatInfo->hThread)
                    {
                        DWORD dwWait;
                        do
                        {
                            dwWait =  WaitForSingleObject(pFormatInfo->hThread, cdwTHREADWAIT);
                        } while ( WAIT_TIMEOUT == dwWait &&
                                  IDRETRY == ShellMessageBox(HINST_THISDLL,
                                                  hDlg,
                                                  MAKEINTRESOURCE(IDS_CANTCANCELFMT),
                                                  NULL,
                                                  MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_RETRYCANCEL));

                        //
                        // If the format doesn't admit to having been killed, it didn't
                        // give up peacefully.  Finish it...
                        // BUGBUG Stack cleanup
                        //

                        if (FALSE == pFormatInfo->fCancelled)
                        {
                            TerminateThread(pFormatInfo->hThread, 0);
                        }

                        CloseHandle(pFormatInfo->hThread);
                        pFormatInfo->hThread = NULL;
                        pFormatInfo->fCancelled = TRUE;
                        EnableControls(pFormatInfo);
                    }
                    else
                    {
                        EndDialog(hDlg, IDCANCEL);
                    }
                    break;
                }
        }

        break;

    } // end WM_COMMAND case

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
                (ULONG_PTR) (LPSTR) FmtaIds);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                        (ULONG_PTR) (LPSTR) FmtaIds);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   SHFormatDrive
//
//  Synopsis:   The SHFormatDrive API provides access to the Shell
//              format dialog. This allows apps which want to format disks
//              to bring up the same dialog that the Shell does to do it.
//
//              NOTE that the user can format as many diskettes in the
//              specified drive, or as many times, as he/she wishes to.
//
//  Arguments:  [hwnd]    -- Parent window (Must NOT be NULL)
//              [drive]   -- 0 = A:, 1 = B:, etc.
//              [fmtID]   -- see below
//              [options] -- SHFMT_OPT_FULL    overrised default quickformat
//                           SHFMT_OPT_SYSONLY not support for NT
//
//  Returns:    See Notes
//
//  History:    2-14-95   davepl   Created
//
//  Notes:      BUGBUGs:
//              Should return volume ID, but doesn't yet
//              There's some backdoor magic about passing in the the low
//              DWORD of the last disk's serial number to force the
//              same type of format, still investigating it...
//
//--------------------------------------------------------------------------

DWORD WINAPI SHFormatDrive(
    HWND hwnd,
    UINT drive,
    UINT fmtID,
    UINT options
)
{
    INT_PTR ret;
    FMIFS fmifs;
    FORMATINFO FormatInfo = { (BYTE) drive, fmtID, options, NULL};
    ASSERT(drive < 26);

    //
    // It makes no sense for NT to "SYS" a disk
    //

    if (FormatInfo.options & SHFMT_OPT_SYSONLY)
    {
        return 0;
    }

    //
    // Load the FMIFS DLL and open the Format dialog
    //

    if (S_OK == LoadFMIFS(&fmifs))
    {
        FormatInfo.pFMIFS = &fmifs;

        // BUGBUG nobody uses the return value `ret'
        ret = DialogBoxParam(HINST_THISDLL,
                             MAKEINTRESOURCE(DLG_FORMATDISK),
                             hwnd,
                             FormatDlgProc,
                             (LPARAM) &FormatInfo);
    }
    else
    {
        ASSERT(0 && "Can't load FMIFS.DLL");
        return SHFMT_ERROR;
    }

    //
    // Free the FMIFS library and return a success code the caller
    //

    FreeLibrary(FormatInfo.pFMIFS->hFMIFS_DLL);

    if (FormatInfo.fCancelled)
    {
        return SHFMT_CANCEL;
    }

    if (FormatInfo.fFinishedOK)
    {
        return 0; // BUGBUG FormatInfo.dwSerialNumber;
    }
    else
    {
        return SHFMT_ERROR;
    }
}

////////////////////////////////////////////////////////////////////////////
//
// CHKDSK
//
////////////////////////////////////////////////////////////////////////////

//
// This structure described the current chkdsk session
//

typedef struct tagChkDskInfo
{
    UINT    lastpercent;           // last percentage complete received
    UINT    currentphase;          // current chkdsk phase
    UINT    drive;                 // 0-based index of drive to chkdsk
    LPFMIFS pFMIFS;                // ptr to FMIFS structure, above
    BOOL    fRecovery;             // Attempt to recover bad sectors
    BOOL    fFixErrors;            // Fix filesystem errors as found
    BOOL    fCancelled;            // Was chkdsk terminated early?
    BOOL    fShouldCancel;         // User has clicked cancel; pending abort
    HWND    hDlg;                  // handle to the chkdsk dialog
    HANDLE  hThread;
    BOOL    fNoFinalMsg;           // Do not put up a final failure message
    WCHAR   wszDriveName[MAX_PATH]; // For example, "A:\", or "C:\folder\mountedvolume\"
} CHKDSKINFO, * LPCHKDSKINFO;

//+-------------------------------------------------------------------------
//
//  Function:   StuffChkDskInfoPtr()
//
//  Synopsis:   Allocates a thread-local index slot for this thread's
//              CHKDSKINFO pointer, if the index doesn't already exist.
//              In any event, stores the CHKDSKINFO pointer in the slot
//              and increments the index's usage count.
//
//  Arguments:  [pChkDskInfo] -- The pointer to store
//
//  Returns:    HRESULT
//
//  History:    2-21-95   davepl   Created
//
//--------------------------------------------------------------------------

//
// Thread-Local Storage index for our CHKDSKINFO structure pointer
//

static DWORD g_iTLSChkDskInfo = 0;
static LONG  g_cTLSChkDskInfo = 0;  // Usage count

HRESULT StuffChkDskInfoPtr(LPCHKDSKINFO pChkDskInfo)
{
    HRESULT hr = S_OK;

    //
    // Allocate an index slot for our thread-local CHKDSKINFO pointer, if one
    // doesn't already exist, then stuff our CHKDSKINFO ptr at that index.
    //

    ENTERCRITICAL;
    if (0 == g_iTLSChkDskInfo)
    {
        if (0 == (g_iTLSChkDskInfo = TlsAlloc()))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        g_cTLSChkDskInfo = 0;
    }
    if (S_OK == hr)
    {
        if (TlsSetValue(g_iTLSChkDskInfo, (LPVOID) pChkDskInfo))
        {
           g_cTLSChkDskInfo++;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    LEAVECRITICAL;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   UnstuffChkDskInfoPtr()
//
//  Synopsis:   Decrements the usage count on our thread-local storage
//              index, and if it goes to zero the index is free'd
//
//  Arguments:  [none]
//
//  Returns:    none
//
//  History:    2-21-95   davepl   Created
//
//--------------------------------------------------------------------------

__inline void UnstuffChkDskInfoPtr()
{
    ENTERCRITICAL;
    if (0 == --g_cTLSChkDskInfo)
    {
        TlsFree(g_iTLSChkDskInfo);
        g_iTLSChkDskInfo = 0;
    }
    LEAVECRITICAL;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetChkDskInfoPtr()
//
//  Synopsis:   Retrieves this threads CHKDSKINFO ptr by grabbing the
//              thread-local value previously stuff'd
//
//  Arguments:  [none]
//
//  Returns:    The pointer, of course
//
//  History:    2-21-95   davepl   Created
//
//--------------------------------------------------------------------------

__inline LPCHKDSKINFO GetChkDskInfoPtr()
{
    return TlsGetValue(g_iTLSChkDskInfo);
}

//+-------------------------------------------------------------------------
//
//  Function:   DisableChkDskControls
//
//  Synopsis:   Ghosts all controls except "Cancel", saving their
//              previous state in the CHKDSKINFO structure
//
//  Arguments:  [pChkDskInfo] -- Describes a ChkDsk dialog session
//
//  History:    2-21-95   davepl   Created
//
//  Notes:      Also changes "Close" button text to read "Cancel"
//
//--------------------------------------------------------------------------

__inline void DisableChkDskControls(LPCHKDSKINFO pChkDskInfo)
{
    //
    // BUGBUG (DavePl) We disable CANCEL because CHKDSK does not
    // allow interruption at the filesystem level.
    //

    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDC_FIXERRORS), FALSE);
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDC_RECOVERY), FALSE);
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDOK), FALSE);
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDCANCEL), FALSE);
    // SetWindowText(GetDlgItem(pChkDskInfo->hDlg, IDCANCEL), cwsz_Cancel);
}

//+-------------------------------------------------------------------------
//
//  Function:   EnableChkDskControls
//
//  Synopsis:   Restores controls to the enabled/disabled state they were
//              before a previous call to DisableControls().
//
//  Arguments:  [pChkDskInfo] -- Decribes a chkdsk dialog session
//
//  History:    2-21-95   davepl   Created
//
//--------------------------------------------------------------------------

__inline void EnableChkDskControls(LPCHKDSKINFO pChkDskInfo)
{
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDC_FIXERRORS), TRUE);
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDC_RECOVERY), TRUE);
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDOK), TRUE);
    EnableWindow(GetDlgItem(pChkDskInfo->hDlg, IDCANCEL), TRUE);
    // SetWindowText(GetDlgItem(pChkDskInfo->hDlg, IDCANCEL), cwsz_Close);

    // Erase the current phase text

    SetWindowText(GetDlgItem(pChkDskInfo->hDlg, IDC_PHASE), WTEXT(""));
    pChkDskInfo->lastpercent = 101;
    pChkDskInfo->currentphase = 0;

}

//+-------------------------------------------------------------------------
//
//  Function:   SetChkDskWindowTitle
//
//  Synopsis:   Sets the chkdsk dialog's title to "Check Disk A:" or
//              "Checking Disk A:" depending on the drive letter and the
//              fInProgress flag.
//
//  Arguments:  [pChkDskInfo] -- Carries the drive letter
//              [fInProgress] -- TRUE => currently checking the disk
//
//  History:    2-21-95   davepl   Created
//
//
//--------------------------------------------------------------------------

void SetChkDskWindowTitle(LPCHKDSKINFO pChkDskInfo, BOOL fInProgress)
{
    WCHAR swzTmp[MAX_PATH + 50];

    LOAD_STRING( fInProgress ? IDS_CHKINPROGRESS:IDS_CHKDISK, swzTmp );
    lstrcat(swzTmp, pChkDskInfo->wszDriveName);

    SetWindowText(pChkDskInfo->hDlg, swzTmp);
}


//+-------------------------------------------------------------------------
//
//  Function:   ChkDskCallback
//
//  Synopsis:   Called from within the FMIFS DLL's ChkDsk function, this
//              updates the ChkDsk dialog's status bar and responds to
//              chkdsk completion/error notifications.
//
//  Arguments:  [PacketType]   -- Type of packet (ie: % complete, error, etc)
//              [PacketLength] -- Size, in bytes, of the packet
//              [pPacketData]  -- Pointer to the packet
//
//  Returns:    BOOLEAN continuation value
//
//  History:    2-21-95   davepl   Created
//
//--------------------------------------------------------------------------

BOOLEAN ChkDskCallback(FMIFS_PACKET_TYPE PacketType,
                       ULONG             PacketLength,
                       PVOID             pPacketData)
{
    UINT iMessageID = IDS_CHKDSKFAILED;
    BOOL fFailed = FALSE;
    LPCHKDSKINFO pChkDskInfo;

    ASSERT(g_iTLSChkDskInfo);

    //
    // Grab the CHKDSKINFO structure for this thread
    //

    if (NULL == (pChkDskInfo = GetChkDskInfoPtr()))
    {
        return FALSE;
    }

    //
    // If the user has signalled to abort the ChkDsk, return
    // FALSE out of here right now
    //

    if (pChkDskInfo->fShouldCancel)
    {
        pChkDskInfo->fCancelled = TRUE;
        return FALSE;
    }

    switch(PacketType)
    {
        case FmIfsAccessDenied:
            fFailed    = TRUE;
            iMessageID = IDS_CHKACCESSDENIED;
            break;

        case FmIfsCheckOnReboot:
        {
            FMIFS_CHECKONREBOOT_INFORMATION * pRebootInfo =
               (FMIFS_CHECKONREBOOT_INFORMATION *) pPacketData;

            // Check to see whether or not the user wants to schedule this
            // chkdsk for the next reboot, since the drive cannot be locked
            // right now.

            if (IDYES == ShellMessageBox(HINST_THISDLL,
                                         pChkDskInfo->hDlg,
                                         MAKEINTRESOURCE(IDS_CHKONREBOOT),
                                         NULL,
                                         MB_SETFOREGROUND | MB_ICONINFORMATION | MB_YESNO))
            {
                // Yes, have FMIFS schedule an autochk for us

                pRebootInfo->QueryResult = TRUE;
                pChkDskInfo->fNoFinalMsg = TRUE;
            }
            else
            {
                // Nope, just fail out with "cant lock drive"

                fFailed = TRUE;
                iMessageID = IDS_CHKDSKFAILED;
            }
            break;
        }

        case FmIfsMediaWriteProtected:
            fFailed    = TRUE;
            iMessageID = IDS_WRITEPROTECTED;
            break;

        /*
            This case will show up with a checkonreboot also, so don't do the
            same error twice

        case FmIfsCantLock:
            fFailed    = TRUE;
            iMessageID = IDS_CANTLOCK;
            break;
        */

        case FmIfsIoError:
            fFailed    = TRUE;
            iMessageID = IDS_IOERROR;
            // FUTURE Consider showing head/track etc where error was
            break;

        case FmIfsPercentCompleted:
        {
            FMIFS_PERCENT_COMPLETE_INFORMATION * pPercent =
              (FMIFS_PERCENT_COMPLETE_INFORMATION *) pPacketData;
            SendMessage(GetDlgItem(pChkDskInfo->hDlg, IDC_CHKDSKPROGRESS),
                        PBM_SETPOS,
                        pPercent->PercentCompleted, 0);

            if (pPercent->PercentCompleted < pChkDskInfo->lastpercent)
            {
                //
                // If this % complete is less than the last one seen,
                // we have completed a phase of the chkdsk and should
                // advance to the next one.
                //

                WCHAR wszTmp[100];
                WCHAR wszFormat[100];

                LOAD_STRING( IDS_CHKPHASE, wszFormat );

                wsprintf(wszTmp, wszFormat, ++(pChkDskInfo->currentphase));

                SetWindowText(GetDlgItem(pChkDskInfo->hDlg, IDC_PHASE), wszTmp);
            }
            pChkDskInfo->lastpercent = pPercent->PercentCompleted;
            break;
        }

        case FmIfsFinished:
        {
            //
            // ChkDsk is done; check for failure or success
            //

            FMIFS_FINISHED_INFORMATION * pFinishedInfo =
              (FMIFS_FINISHED_INFORMATION *) pPacketData;

            //
            // ChkDskEx now return the proper success value
            //
            if (pFinishedInfo->Success)
            {
                //
                // Since we're done, force the progress gauge to 100%, so we
                // don't sit here looking stupid if the chkdsk code misled us
                //

                SendMessage(GetDlgItem(pChkDskInfo->hDlg, IDC_CHKDSKPROGRESS),
                            PBM_SETPOS,
                            100 /* Percent Complete */, 0);

                ShellMessageBox(HINST_THISDLL,
                                pChkDskInfo->hDlg,
                                MAKEINTRESOURCE(IDS_CHKDSKCOMPLETE),
                                NULL,
                                MB_SETFOREGROUND | MB_ICONINFORMATION | MB_OK);

                SetWindowText(GetDlgItem(pChkDskInfo->hDlg, IDC_PHASE), WTEXT(""));

                SendMessage(GetDlgItem(pChkDskInfo->hDlg, IDC_CHKDSKPROGRESS),
                            PBM_SETPOS,
                            0 /* Reset Percent Complete */, 0);
            }
            else
            {
                iMessageID = IDS_CHKDSKFAILED;
                fFailed = TRUE;
            }
            break;
        }
    }

    //
    // If we received any kind of failure information, put up a final
    // "ChkDsk Failed" message.
    //

    if (fFailed && FALSE == pChkDskInfo->fNoFinalMsg)
    {
        pChkDskInfo->fNoFinalMsg = TRUE;

        ShellMessageBox(HINST_THISDLL,
                        pChkDskInfo->hDlg,
                        MAKEINTRESOURCE(iMessageID),
                        NULL,
                        MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK);

    }

    return (BOOLEAN) (fFailed == FALSE);
}

//+-------------------------------------------------------------------------
//
//  Function:   BeginChkDsk
//
//  Synopsis:   Spun off as its own thread, this ghosts all controls in the
//              dialog except "Cancel", then does the actual ChkDsk
//
//  Arguments:  [pIn] -- CHKDSKINFO structure pointer as a void *
//
//  Returns:    HRESULT thread exit code
//
//  History:    2-21-95   davepl   Created
//
//--------------------------------------------------------------------------

DWORD WINAPI BeginChkDsk(LPVOID pIn)
{
    LPCHKDSKINFO pChkDskInfo = pIn;
    HRESULT hr = S_OK;
    WCHAR swzFileSystem[MAX_PATH];

    //
    // Save the CHKDSKINFO ptr for this thread, to be used in the ChkDsk
    // callback function
    //

    if (S_OK != (hr = StuffChkDskInfoPtr(pChkDskInfo)))
    {
        PostMessage(pChkDskInfo->hDlg, (UINT) PWM_CHKDSKDONE, 0, 0);
        return (DWORD) hr;
    }

    //
    // Get the filesystem in use on the device
    //

    if (FALSE == GetVolumeInformationW(pChkDskInfo->wszDriveName,
                                       NULL, 0,
                                       NULL, NULL, NULL,
                                       swzFileSystem, MAX_PATH))
    {
        PostMessage(pChkDskInfo->hDlg, (UINT) PWM_CHKDSKDONE, 0, 0);
        return (HRESULT_FROM_WIN32(GetLastError()));
    }

    //
    // Set the window title to indicate ChkDsk in proress...
    //

    SetChkDskWindowTitle(pChkDskInfo, TRUE);

    pChkDskInfo->fNoFinalMsg = FALSE;

    //
    // Should we try data recovery?
    //

    pChkDskInfo->fRecovery = (BOOLEAN) SendMessage(GetDlgItem(pChkDskInfo->hDlg, IDC_RECOVERY), BM_GETCHECK, 0, 0);

    //
    // Should we fix filesystem errors?
    //

    pChkDskInfo->fFixErrors = (BOOLEAN) SendMessage(GetDlgItem(pChkDskInfo->hDlg, IDC_FIXERRORS), BM_GETCHECK, 0, 0);

    //
    // Do the ChkDsk.
    //
    {
        TCHAR szVolumeGUID[50]; // 50: from doc

        FMIFS_CHKDSKEX_PARAM param = {0};

        param.Major = 1;
        param.Minor = 0;
        param.Flags = pChkDskInfo->fRecovery ? FMIFS_CHKDSK_RECOVER : 0;

        GetVolumeNameForVolumeMountPoint(pChkDskInfo->wszDriveName,
            szVolumeGUID, ARRAYSIZE(szVolumeGUID));

        // the backslash at the end means check for fragmentation.
        PathRemoveBackslash(szVolumeGUID);

        pChkDskInfo->pFMIFS->ChkDskEx(szVolumeGUID,
                                swzFileSystem,
                                (BOOLEAN) pChkDskInfo->fFixErrors,
                                &param,
                                ChkDskCallback);            /* Callback fn   */

    }
    //
    // Release the TLS index
    //

    UnstuffChkDskInfoPtr();

    //
    // Post a message back to the DialogProc thread to let it know
    // the chkdsk is done.  We post the message since otherwise the
    // DialogProc thread will be too busy waiting for this thread
    // to exit to be able to process the PWM_CHKDSKDONE message
    // immediately.
    //

    PostMessage(pChkDskInfo->hDlg, (UINT) PWM_CHKDSKDONE, 0, 0);
    return (DWORD) S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   ChkDskDlgProc
//
//  Synopsis:   DLGPROC for the chkdsk dialog
//
//  Arguments:  [hDlg]   -- Typical
//              [wMsg]   -- Typical
//              [wParam] -- Typical
//              [lParam] -- For WM_INIT, carries the CHKDSKINFO structure
//                          pointer passed to DialogBoxParam() when the
//                          dialog was created.
//
//  History:    2-22-95   davepl   Created
//
//--------------------------------------------------------------------------

BOOL_PTR CALLBACK ChkDskDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr   = S_OK;
    int     ID   = GET_WM_COMMAND_ID(wParam, lParam);

    // Grab our previously cached pointer to the CHKDSKINFO struct (see WM_INITDIALOG)

    LPCHKDSKINFO pChkDskInfo = (LPCHKDSKINFO) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) {

    case PWM_CHKDSKDONE:
    {
        //
        // chdsk is done.  Reset the window title and clear the progress meter
        //

        SetChkDskWindowTitle(pChkDskInfo, FALSE);

        SendMessage(GetDlgItem(pChkDskInfo->hDlg, IDC_CHKDSKPROGRESS),
                    PBM_SETPOS,
                    0 /* Reset Percent Complete */, 0);

        EnableChkDskControls(pChkDskInfo);

        if (pChkDskInfo->fCancelled)
        {
            ShellMessageBox(HINST_THISDLL,
                            pChkDskInfo->hDlg,
                            MAKEINTRESOURCE(IDS_CHKDSKCANCELLED),
                            NULL,
                            MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_OK);
        }
        CloseHandle(pChkDskInfo->hThread);
        pChkDskInfo->hThread = NULL;
        EndDialog(hDlg, 0);
        break;
    }

    case WM_INITDIALOG:
    {
        //
        // Initialize the dialog and cache the CHKDSKINFO structure's pointer
        // as our dialog's DWLP_USER data
        //

        pChkDskInfo = (LPCHKDSKINFO) lParam;
        pChkDskInfo->hDlg = hDlg;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        //
        // Set the dialog title to indicate which drive we are dealing with
        //

        // Are we dealing with a drive mounted on a folder?
        if (-1 == pChkDskInfo->drive)
        {
            // Yes, we should be on NT5 (or better)
            ASSERT(g_bRunOnNT5);
        }
        else
        {
            // No
            lstrcpyW(pChkDskInfo->wszDriveName, WTEXT("A:\\"));
            ASSERT(pChkDskInfo->drive < 26);
            pChkDskInfo->wszDriveName[0] += (WCHAR) pChkDskInfo->drive;
            SetChkDskWindowTitle(pChkDskInfo, FALSE);
        }

        break;
    }

    case WM_DESTROY:
        break;

    case WM_COMMAND:
    {
        switch (ID)
        {
            case IDC_FIXERRORS:
                pChkDskInfo->fFixErrors = (BOOL) SendMessage((HWND) lParam, BM_GETCHECK, 0, 0);
                break;

            case IDC_RECOVERY:
                pChkDskInfo->fRecovery = (BOOL) SendMessage((HWND) lParam, BM_GETCHECK, 0, 0);
                break;

            case IDOK:
            {
                DWORD   dwThreadID;

                //
                // Get user verification for chkdsk, break out on CANCEL
                //

                DisableChkDskControls(pChkDskInfo);
                pChkDskInfo->fShouldCancel = FALSE;
                pChkDskInfo->fCancelled    = FALSE;
                pChkDskInfo->hThread = CreateThread( NULL,
                                                     0,
                                                     BeginChkDsk,
                                                     (LPVOID) pChkDskInfo,
                                                     0,
                                                     &dwThreadID );
                break;
            }

            case IDCANCEL:

                //
                // If the chdsk thread is running, wait for it.  If not,
                // exit the dialog
                //

                pChkDskInfo->fShouldCancel = TRUE;
                if (pChkDskInfo->hThread)
                {
                    DWORD dwWait;
                    do
                    {
                        dwWait =  WaitForSingleObject(pChkDskInfo->hThread, cdwTHREADWAIT);
                    } while ( WAIT_TIMEOUT == dwWait &&
                              IDRETRY == ShellMessageBox(HINST_THISDLL,
                                              hDlg,
                                              MAKEINTRESOURCE(IDS_CANTCANCELCHKDSK),
                                              NULL,
                                              MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_RETRYCANCEL));

                    //
                    // If the chkdsk doesn't admit to having been killed, it didn't
                    // give up peacefully.  Finish it...
                    // BUGBUG Stack cleanup
                    //

                    if (FALSE == pChkDskInfo->fCancelled)
                    {
                        TerminateThread(pChkDskInfo->hThread, 0);
                    }

                    CloseHandle(pChkDskInfo->hThread);
                    pChkDskInfo->hThread = NULL;
                    pChkDskInfo->fCancelled = TRUE;
                    EnableChkDskControls(pChkDskInfo);
                }
                else
                {
                    EndDialog(hDlg, IDCANCEL);
                }
                break;
            }
            break;
        }
        case WM_HELP:
            WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
                    (ULONG_PTR) (LPSTR) ChkaIds);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                            (ULONG_PTR) (LPSTR) ChkaIds);
            break;
        default:
            return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function:   SHChkDskDrive
//
//  Synopsis:   Displays the "Check Disk" dialog and calls chkdsk on the
//              drive in question, as appropriate
//
//  Arguments:  [hwnd]    -- Parent window (Must NOT be NULL)
//              [drive]   -- 0 = A:, 1 = B:, etc.
//
//  Returns:    HRESULT
//
//  History:    2-14-95   davepl   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

static BOOL fChkdskActive[26];

DWORD WINAPI SHChkDskDrive(
    HWND hwnd,
    UINT drive
)
{
    INT_PTR ret = S_OK;
    FMIFS fmifs;

    //
    // We use a last percentage-complete value of 101, to guarantee that the
    // next one received will be less, indicating next (first) phase
    //

    CHKDSKINFO ChkDskInfo = { 101, 0, drive, NULL, FALSE, FALSE, FALSE, FALSE };

    //
    // Cheap semaphore to prevent multiple chkdsks of the same drive
    //

    ENTERCRITICAL;
    if (fChkdskActive[drive])
    {
        LEAVECRITICAL;
        return (DWORD)E_FAIL;
    }
    fChkdskActive[drive] = 1;
    LEAVECRITICAL;

    //
    // Load the FMIFS DLL and open the ChkDsk dialog
    //

    if (S_OK == LoadFMIFS(&fmifs))
    {
        ChkDskInfo.pFMIFS = &fmifs;

        ret = DialogBoxParam(HINST_THISDLL,
                             MAKEINTRESOURCE(DLG_CHKDSK),
                             hwnd,
                             ChkDskDlgProc,
                             (LPARAM) &ChkDskInfo);

        if (-1 == ret)
        {
            // BUGBUG nobody uses the return value `ret'
            ret = GetLastError();
        }

    }
    else
    {
        ASSERT(0 && "Can't load FMIFS.DLL");
        fChkdskActive[drive] = 0;
        return (DWORD) E_OUTOFMEMORY;
    }

    //
    // Free the FMIFS library and return a success code the caller
    //

    FreeLibrary(ChkDskInfo.pFMIFS->hFMIFS_DLL);

    fChkdskActive[drive] = 0;
    return (DWORD) S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   SHChkDskDriveEx
//
//  Synopsis:   Same as SHChkDskDrive but takes a path rather than a drive int ID
//              Call this fct for both path and drive int ID to be protected
//              against chkdsk'ing the same drive simultaneously
//
//  Arguments:  [hwnd]     -- Parent window (Must NOT be NULL)
//              [pszDrive] -- INTRESOURCE: string if mounted on folder, drive
//                            number if mounted on drive letter (0 based)
//
//--------------------------------------------------------------------------

#define GET_INTRESOURCE(r) (LOWORD((UINT_PTR)(r)))

static HDPA hpdaChkdskActive = NULL;

STDAPI_(DWORD) SHChkDskDriveEx(
    HWND hwnd, LPWSTR pszDrive
)
{
    INT_PTR ret = S_OK;
    FMIFS fmifs;
    HRESULT hres = S_OK;
    WCHAR szUniqueID[50]; // 50: size of VolumeGUID, which can fit "A:\\" too

    //
    // We use a last percentage-complete value of 101, to guarantee that the
    // next one received will be less, indicating next (first) phase
    //
    CHKDSKINFO ChkDskInfo = { 101, 0, -1, NULL, FALSE, FALSE, FALSE, FALSE };

    lstrcpyn(ChkDskInfo.wszDriveName, pszDrive, ARRAYSIZE(ChkDskInfo.wszDriveName));
    PathAddBackslash(ChkDskInfo.wszDriveName);

    //
    // Prevent multiple chkdsks of the same drive
    //

    if (g_bRunOnNT5)
    {
        GetVolumeNameForVolumeMountPoint(ChkDskInfo.wszDriveName, szUniqueID, 
            ARRAYSIZE(szUniqueID));        
    }
    else
    {
        StrCpyNW(szUniqueID, pszDrive, ARRAYSIZE(szUniqueID));
    }

    ENTERCRITICAL;

    if (!hpdaChkdskActive)
    {
        hpdaChkdskActive = DPA_Create(1);
    }

    if (hpdaChkdskActive)
    {
        int n = DPA_GetPtrCount(hpdaChkdskActive);
        int i = 0;

        // Go through the DPA of currently chkdsk'ed volumes, and check if we're already
        // processing this volume
        for (; i < n; ++i)
        {
            LPWSTR pszUniqueID = (LPWSTR)DPA_GetPtr(hpdaChkdskActive, i);

            if (pszUniqueID)
            {
                if (!lstrcmpi(szUniqueID, pszUniqueID))
                {
                    // we're already chkdsk'ing this drive
                    hres = (DWORD)E_FAIL;
                    break;
                }
            }
        }

        // Looks like we're currently not chkdsk'ing this volume, add it to the DPA of currently
        // chkdsk'ed volumes
        if (S_OK == hres)
        {
            LPWSTR pszUniqueID = (LPWSTR)LocalAlloc(LPTR, 50 * sizeof(TCHAR)); // 50: from doc

            lstrcpyn(pszUniqueID, szUniqueID, 50 * sizeof(TCHAR));

            if (pszUniqueID)
            {
                if (-1 == DPA_AppendPtr(hpdaChkdskActive, pszUniqueID))
                {
                     LocalFree((HLOCAL)pszUniqueID);

                     // if can't allocate room to store a pointer, pretty useless to go on
                     hres = (DWORD)E_FAIL;
                }
            }
        }
    }

    LEAVECRITICAL;

    //
    // Load the FMIFS DLL and open the ChkDsk dialog
    //

    if (S_OK == hres)
    {
        if (S_OK == LoadFMIFS(&fmifs))
        {
            ChkDskInfo.pFMIFS = &fmifs;

            ret = DialogBoxParam(HINST_THISDLL,
                                 MAKEINTRESOURCE(DLG_CHKDSK),
                                 hwnd,
                                 ChkDskDlgProc,
                                 (LPARAM) &ChkDskInfo);

            if (-1 == ret)
            {
                // BUGBUG nobody uses the return value `ret'
                ret = GetLastError();
            }
        }
        else
        {
            ASSERT(0 && "Can't load FMIFS.DLL");
            hres = E_OUTOFMEMORY;
        }

        //
        // Free the FMIFS library and return a success code the caller
        //
        if (E_OUTOFMEMORY != hres)
        {
            FreeLibrary(ChkDskInfo.pFMIFS->hFMIFS_DLL);
        }

        // We're finsih for this volume, remove from the list of currently processed volumes
        ENTERCRITICAL;
        if (hpdaChkdskActive)
        {
            int n = DPA_GetPtrCount(hpdaChkdskActive);
            int i = 0;

            for (i = 0; i < n; ++i)
            {
                LPWSTR pszUniqueID = (LPWSTR)DPA_GetPtr(hpdaChkdskActive, i);

                if (pszUniqueID)
                {
                    if (!lstrcmpi(szUniqueID, pszUniqueID))
                    {
                        LocalFree((HLOCAL)pszUniqueID);

                        DPA_DeletePtr(hpdaChkdskActive, i);
                        break;
                    }
                }
            }
        }
        LEAVECRITICAL;
    }

    // If the DPA is empty delete it
    ENTERCRITICAL;
    if (hpdaChkdskActive && !DPA_GetPtrCount(hpdaChkdskActive))
    {
        DPA_Destroy(hpdaChkdskActive);
        hpdaChkdskActive = NULL;
    }
    LEAVECRITICAL;

    return (DWORD) hres;
}

#endif // WINNT
