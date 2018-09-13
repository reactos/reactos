#include "diskcopy.h"
#include "ids.h"
#include "..\..\inc\help.h"

// SHChangeNotifySuspendResume
#include <shlobjp.h>

#define WM_DONE_WITH_FORMAT     (WM_USER + 100)

#ifndef WINNT
//
// NT has multiple disk formats handled by FMIFS.DLL
//
#ifdef  DBCS
// changed from NEC_98 to DBCS.
// 3mode FD will use this on non NEC and BOOTSEC doesn't
// suffer from this anyway.
#define SEC_SIZE        1024
#else
#define SEC_SIZE        512
#endif

#define DEVPB_DEVATT_DMF 0x0800
//
// The low BYTE of Options is the lock sub-type
//
#define LFS_OPT_EXCLUSIVE        0      // Only owner of lock can read or write
#define LFS_OPT_ALLOWOPEN        1      // See below option definitions
#define LFS_OPT_ALLOWRDBLKWRT    2      // Requires LFS_OPT_ALLOWOPEN first
#define LFS_OPT_BLKALL           3      // Requires LFS_OPT_ALLOWRDBLKWRT first
//
// The HIGH WORD of Options is the lock type modifyer for LFS_OPT_ALLOWOPEN
//
// NOTE that this is actually a flag field as opposed to a value setting.
//
#define LFS_OPT_ALLOWRDFLWRT 0x0000     // Allow others to read, fail writes
#define LFS_OPT_ALLOWRDWRT   0x0001     // Allow others to read and write
#define LFS_OPT_ALLOWMMACT   0x0000     // Allow memory mapped file activity
#define LFS_OPT_NOMMACT      0x0002     // Fail memory mapped file activity
#define LFS_OPT_FORMAT       0x0004     // Mount default FSD


#define IOCTL_FORMAT            0x0842
#define IOCTL_GET_DPB           0x0860
#define IOCTL_SET_DPB           0x0840
#define IOCTL_READ              0x0861
#define IOCTL_WRITE             0x0841
#define IOCTL_LOCK              0x084A
#define IOCTL_UNLOCK            0x086A

/* Media descriptor values for different floppy drives */
// NOTE: these are not all unique!
#define  MEDIA_160      0xFE    /* 160KB */
#define  MEDIA_320      0xFF    /* 320KB */
#define  MEDIA_180      0xFC    /* 180KB */
#define  MEDIA_360      0xFD    /* 360KB */
#define  MEDIA_1200     0xF9    /* 1.2MB */
#define  MEDIA_720      0xF9    /* 720KB */
#ifdef NEC_98
#define  MEDIA_1250     0xFE    /* 1.25MB(1024bps) */
#endif
#define  MEDIA_1440     0xF0    /* 1.44M */
#define  MEDIA_2880     0xF0    /* 2.88M */


/* DriveIOCTL error codes */
#define SECNOTFOUND         0x1B
#define CRCERROR            0x17
#define GENERALERROR        0x1F

#pragma pack(1)

/*--------------------------------------------------------------------------*/
/*  BIOS Parameter Block Structure -                                        */
/*--------------------------------------------------------------------------*/
typedef struct
{
    WORD    cbSec;          /* Bytes per sector                 */
    BYTE    secPerClus;     /* Sectors per cluster              */
    WORD    cSecRes;        /* Reserved sectors                 */
    BYTE    cFAT;           /* FATS                             */
    WORD    cDir;           /* Root Directory Entries           */
    WORD    cSec;           /* Total number of sectors in image */
    BYTE    bMedia;         /* Media descriptor                 */
    WORD    secPerFAT;      /* Sectors per FAT                  */
    WORD    secPerTrack;    /* Sectors per track                */
    WORD    cHead;          /* Heads                            */
    WORD    cSecHidden;     /* Hidden sectors                   */
} BPB, *PBPB;

/*--------------------------------------------------------------------------*/
/*  Drive Parameter Block Structure -                                       */
/*--------------------------------------------------------------------------*/
typedef struct
{
    BYTE    drive;
    BYTE    unit;
    WORD    sector_size;
    BYTE    cluster_mask;
    BYTE    cluster_shift;
    WORD    first_FAT;
    BYTE    FAT_count;
    WORD    root_entries;
    WORD    first_sector;
    WORD    max_cluster;
    BYTE    FAT_size;
    WORD    dir_sector;
    LONG    reserved1;
    BYTE    media;
    BYTE    first_access;
    BYTE    reserved2[4];
    WORD    next_free;
    WORD    free_cnt;
    BYTE    DOS4_Extra;
} DPB, *PDPB;

#define MAX_SEC_PER_TRACK       40

/*--------------------------------------------------------------------------*/
/*  Device Parameter Block Structure -                                      */
/*--------------------------------------------------------------------------*/
typedef struct
{
    BYTE    SplFunctions;
    BYTE    devType;
    WORD    devAtt;     // see dskmaint for what these are
    WORD    NumCyls;
    BYTE    bMediaType;  /* 0=>1.2MB and 1=>360KB */
    BPB     BPB;
    BYTE    reserved3[MAX_SEC_PER_TRACK * 4 + 2];
} DEVPB, *PDEVPB;

#define TRACKLAYOUT_OFFSET      (7+31)  /* Offset of tracklayout
                                         * in a Device Parameter Block */

typedef struct
{
    BYTE    jump[3];        /* 3 byte jump */
    BYTE    label[8];       /* OEM name and version */
    BPB     BPB;            /* BPB */
    BYTE    bootdrive;      /* INT 13h indicator for boot device */
    BYTE    dontcare[SEC_SIZE-12-3-sizeof(BPB)];
    BYTE    phydrv;
    WORD    signature;
} BOOTSEC;

#pragma pack()

#endif  // ndef WINNT

// DISKINFO Struct
// Revisions:	02/04/98 dsheldon - added bDestInserted

typedef struct
{
    int     nSrcDrive;
    int     nDestDrive;
    UINT    nCylinderSize;
    UINT    nCylinders;
    UINT    nHeads;
    UINT    nSectorsPerTrack;
    UINT    nSectorSize;
#ifndef WINNT
    PDEVPB  pTrackLayout;           /* DEVPB with the track layout */

    LPBYTE  pCopyBuffer;
    DWORD   dwCopyBufferSize;
#else
    BOOL    bNotifiedWriting;
#endif
    BOOL    bFormatTried;

    HWND    hdlg;
    HANDLE  hThread;
    BOOL    bUserAbort;
    DWORD   dwError;

	BOOL	bDestInserted;

} DISKINFO, *PDISKINFO;

int ErrorMessageBox(DISKINFO* pdi, UINT uFlags);
void SetStatusText(DISKINFO* pdi, int id);
BOOL PromptInsertDisk(DISKINFO *pdi, LPCTSTR lpsz, BOOL fAutoCheck);

#ifndef WINNT

extern BOOL DriveIOCTL(int iDrive, int cmd, void *pv);
BOOL LockDrive(int iDrive, BOOL fLock, WORD wPermissions);

const TCHAR c_szVWIN32[] = TEXT("\\\\.\\vwin32");

// in:
//      iDrive  0 based drive number
//
// returns:
//      TRUE    success
//      FALSE   failure

BOOL DriveIOCTL(int iDrive, int cmd, void *pv)
{
    DWORD reg[7];
    DWORD cbBytes;
    HANDLE h;
    BOOL bRet;

    reg[0] = iDrive + 1;    // make 1 based drive number
    reg[1] = (DWORD)pv;     // out buffer
    reg[2] = cmd;           // device specific command code
    reg[3] = 0x440D;        // generic read ioctl
    reg[6] = 0x0001;        // flags, assume error (carry)

    h = CreateFile(c_szVWIN32, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (h != INVALID_HANDLE_VALUE)
    {
        DeviceIoControl(h, 1, &reg, sizeof(reg), &reg, sizeof(reg), &cbBytes, 0);
        CloseHandle(h);
    }

    bRet = !(reg[6] & 0x0001);

#ifdef DEBUG
    // all we ever get is access denied
    if (!bRet) {
        DebugMsg(DM_TRACE, TEXT("IOCtl error %x, %d"), reg[3], GetLastError());
        //SetLastError(reg[3]);
    }
#endif

    return bRet;
}


BOOL ReadWriteSector(void *pBuf, UINT nFunc, UINT nDrive, UINT nCylinder, UINT wHead, UINT wCount)
{
#pragma pack(1)
typedef struct
{
    BYTE        bSplFn;
    WORD        wHead;
    WORD        nCylinder;
    WORD        wStSector;
    WORD        wCount;
    LPBYTE      pBuf;
    WORD        wSel;
} RW_PARMBLOCK;
#pragma pack()
    RW_PARMBLOCK rwp;

    rwp.bSplFn = 0;
    rwp.wHead = wHead;
    rwp.nCylinder = nCylinder;
    rwp.wStSector = 0;
    rwp.wCount = wCount;
    rwp.pBuf = pBuf;

    _asm mov    rwp.wSel, ds

    return DriveIOCTL(nDrive, nFunc, &rwp);
}



// This reads the boot sector of a floppy and returns a ptr to
// the BIOS PARAMETER BLOCK in the Boot sector.
// BUGBUG: boot sector sizes != 512 will puke

BOOL GetBootBPB(int nDrive, PBPB pBPB)
{
    BOOTSEC Boot;
    if (ReadWriteSector(&Boot, IOCTL_READ, nDrive, 0, 0, 1) &&
        (Boot.jump[0] == 0xEB || Boot.jump[0] == 0xE9))
    {
        *pBPB = Boot.BPB;
        return TRUE;
    }
    return FALSE;
}

// Gets get the BPB of the Physical Drive.

BOOL GetBPB(int nDrive, PDEVPB pDevicePB, BYTE bDisk)
{
    /* All fields in pDevicePB must be initialized to zero. */
    memset(pDevicePB, 0, sizeof(DEVPB));
    pDevicePB->SplFunctions = bDisk;
    /* Spl Function field must be set to get parameters */
    Assert(pDevicePB->SplFunctions == 0);
    return DriveIOCTL(nDrive, IOCTL_GET_DPB, pDevicePB);
}


/* Checks whether the two BPB are compatible for the purpose of performing
 * the diskcopy operation.
 */

BOOL CheckBPBCompatibility(PDEVPB pSrc, PDEVPB pDst)
{
#ifndef NEC_98
  /* Let us compare the media byte */
  if (pSrc->BPB.bMedia == 0xF9)
    {
      /* If the source and dest have the same number of sectors,
       * or if srce is 720KB and Dest is 1.44MB floppy drive,
       * thnigs are kosher.
       */
      if ((pSrc->BPB.cSec == pDst->BPB.cSec) ||
         ((pSrc->BPB.secPerTrack == 9) && (pDst->BPB.bMedia == 0xF0)))
          return TRUE;
    }
  else
#endif
    {
      /* If they have the same media byte */
      if ((pSrc->BPB.bMedia == pDst->BPB.bMedia) &&
          (pSrc->BPB.cbSec  == pDst->BPB.cbSec) && // bytes per sector are the same
          (pSrc->BPB.cSec   == pDst->BPB.cSec))    // total sectors on drive are the same
                return TRUE; /* They are compatible */
#ifndef NEC_98
      else if
            /* srce is 160KB and dest is 320KB drive */
            (((pSrc->BPB.bMedia == MEDIA_160) && (pDst->BPB.bMedia == MEDIA_320)) ||
            /* or if srce is 180KB and dest is 360KB drive */
            ((pSrc->BPB.bMedia == MEDIA_180) && (pDst->BPB.bMedia == MEDIA_360)) ||
            /* or if srce is 1.44MB and dest is 2.88MB drive */
            ((pSrc->BPB.bMedia == MEDIA_1440) && (pDst->BPB.bMedia == MEDIA_2880)
            && ((pSrc->devType == 7) || (pSrc->devType == 9))
            &&  (pDst->devType == 9)) ||
            /* or if srce is 360KB and dest is 1.2MB drive */
            ((pSrc->BPB.bMedia == MEDIA_360) && (pDst->BPB.secPerTrack == 15)))
                return TRUE; /* They are compatible */
#endif
    }

  /* All other combinations are currently incompatible. */
  return FALSE;
}


PDEVPB BuildDEVPB(PDEVPB pDEVPB)
{
    PDEVPB pNewDEVPB = LocalAlloc(LPTR, TRACKLAYOUT_OFFSET + 2 + pDEVPB->BPB.secPerTrack * 4);
    if (pNewDEVPB)
    {
        WORD wTrackNumber, *pData;

        memcpy(pNewDEVPB, pDEVPB, TRACKLAYOUT_OFFSET);

        pData = (WORD *)((LPBYTE)pNewDEVPB + TRACKLAYOUT_OFFSET);
        *pData++ = pDEVPB->BPB.secPerTrack;

        for (wTrackNumber = 1;  wTrackNumber <= pDEVPB->BPB.secPerTrack; wTrackNumber++)
        {
            *pData++ = wTrackNumber;
            *pData++ = pDEVPB->BPB.cbSec;
        }
    }
    return pNewDEVPB;
}



/* Saves a copy of the drive parameters block and
 * Checks if the BPB of Drive and BPB of disk are different and if
 * so, modifies the drive parameter block accordingly.
 */

BOOL ModifyDeviceParams(int nDrive, PDEVPB pdpbParams, PDEVPB *ppSaveDevPB,
        PBPB pDriveBPB, PBPB pMediaBPB)
{
    PDEVPB pNewDPB;

    *ppSaveDevPB = BuildDEVPB(pdpbParams);
    if (!*ppSaveDevPB)
        return FALSE;

    /* Check if the Disk and Drive have the same parameters */
    //if (pMediaBPB->bMedia != pDriveBPB->bMedia)
    //{
      /* They are not equal; So, it must be a 360KB floppy in a 1.2MB drive
       * or a 720KB floppy in a 1.44MB drive kind of situation!.
       * So, modify the DriveParameterBlock's BPB.
       */
    // copy these always because sometimes (liek 2.88) bMedia is the same on
    // both when the media are different
    pdpbParams->BPB = *pMediaBPB;
    DebugMsg(DM_TRACE, TEXT("BPB = %x %x %x %x\n    %x %x %x %x\n    %x %x %x \n"),
             (DWORD)pMediaBPB->cbSec, (DWORD)pMediaBPB->secPerClus, (DWORD)pMediaBPB->cSecRes,
             (DWORD)pMediaBPB->cFAT,
             (DWORD)pMediaBPB->cDir, (DWORD)pMediaBPB->cSec, (DWORD)pMediaBPB->bMedia,
             (DWORD)pMediaBPB->secPerFAT,
             (DWORD)pMediaBPB->secPerTrack, (DWORD)pMediaBPB->cHead, (DWORD)pMediaBPB->cSecHidden);
    //}

    // Build a DPB with TrackLayout
    pNewDPB = BuildDEVPB(pdpbParams);
    if (!pNewDPB)
    {
        LocalFree(*ppSaveDevPB);
        *ppSaveDevPB = NULL;
        return FALSE;
    }

    pNewDPB->SplFunctions = 4;  /* To Set parameters */

    // REVIEW: special case, may not be needed on win95
    if (pMediaBPB->bMedia == MEDIA_360)
    {
        pNewDPB->NumCyls = 40;
        pNewDPB->bMediaType = 1;
    }

    DriveIOCTL(nDrive, IOCTL_SET_DPB, pNewDPB);

    LocalFree(pNewDPB);

    return TRUE;
}

/* This calls IOCTL format if DOS ver >= 3.2; Else calls BIOS.
 *
 *  Returns : 0 if no error
 *          > 0 if tolerable error (resuling in bad sectors);
 *          -1  if fatal error (Format has to be aborted);
 */

int FormatTrack(UINT nDisk, UINT nCylinder, UINT wHead)
{
#pragma pack(1)
typedef struct
{
    BYTE   bSpl;
    WORD   wHead;
    WORD   nCylinder;
} FORMATPARAMS;
#pragma pack()

    FORMATPARAMS fp;
//    int iErrCode;

    DebugMsg(DM_TRACE, TEXT("Format %d %d"), wHead, nCylinder);

    fp.bSpl = 0;
    fp.wHead = wHead;
    fp.nCylinder = nCylinder;

    if (DriveIOCTL(nDisk, IOCTL_FORMAT, &fp))
        return 0;       // success
    else
    {
        DebugMsg(DM_ERROR, TEXT("FormatTrack failed %d"), GetLastError());
        return -1;      // fatial error
    }

#if 0
    // BUGBUG: need extended error
    switch (iErrCode) {
    case NOERROR:
    case CRCERROR:
    case SECNOTFOUND:
    case GENERALERROR:
        return iErrCode;
    default:
        return -1;
    }
#endif
}

#define FORMAT_RETRY -1
#define FORMAT_ERROR 0
#define FORMAT_SUCCESS 1



int FormatAllTracks(PDISKINFO pdi, UINT nStartCylinder, UINT nStartHead)
{
    int iErrCode;
    BOOL bRetValue = FORMAT_SUCCESS;

    LockDrive(pdi->nDestDrive, TRUE, LFS_OPT_FORMAT);
    SetStatusText(pdi, IDS_FORMATTINGDEST);

    pdi->pTrackLayout->SplFunctions = 5;
    DriveIOCTL(pdi->nDestDrive, IOCTL_SET_DPB, pdi->pTrackLayout);

    // Format tracks one by one, checking if the user has "Aborted"
    // after each track is formatted; DlgProgreeProc() will set the global
    // bUserAbort, if the user has aborted;

    while (nStartCylinder < pdi->nCylinders)
    {
        /* Has the user aborted? */
        if (pdi->bUserAbort)
        {
            bRetValue = FORMAT_ERROR;
            break;
        }

Retry:
        /* If no message is pending, go ahead and format one track */
        if ((iErrCode = FormatTrack(pdi->nDestDrive, nStartCylinder, nStartHead)))
        {
            /* Check if it is a fatal error */
            if (iErrCode == -1)
            {
                pdi->dwError = IDS_ERROR_FORMAT;
                LockDrive(pdi->nDestDrive, FALSE, LFS_OPT_FORMAT);
                LockDrive(pdi->nDestDrive, FALSE, 0);
                if (ErrorMessageBox(pdi, MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY) {

                    LockDrive(pdi->nDestDrive, TRUE, 0);
                    LockDrive(pdi->nDestDrive, TRUE, LFS_OPT_FORMAT);
                    pdi->dwError = 0;
                    goto Retry;
                }
                bRetValue = FORMAT_ERROR;
                break;
            }
        }
        if (++nStartHead >= pdi->nHeads)
        {
            nStartHead = 0;
            nStartCylinder++;
        }
    }

    pdi->pTrackLayout->SplFunctions = 4;
    DriveIOCTL(pdi->nDestDrive, IOCTL_SET_DPB, pdi->pTrackLayout);

    LockDrive(pdi->nDestDrive, FALSE, 0);
    return bRetValue;
}



BOOL AllocCopyDiskBuffers(PDISKINFO pdi)
{
  // now, lets try to allocate a buffer for the whole disk, and
  // if that fails try smaller

  pdi->dwCopyBufferSize = pdi->nCylinderSize * pdi->nCylinders;

  // we will try down to 8 cylinders worth, less than that means
  // there will be too much disk swapping so don't bother

  do {
        pdi->pCopyBuffer = GlobalAlloc(GPTR, pdi->dwCopyBufferSize);
        if (pdi->pCopyBuffer)
            return TRUE;

        // reduce request and try again
        DebugMsg(DM_TRACE, TEXT("Failed alloc, trying smaller size"));
        pdi->dwCopyBufferSize /= 2;

  } while (pdi->dwCopyBufferSize > (8 * pdi->nCylinderSize));

  DebugMsg(DM_ERROR, TEXT("Failed to alloc copy buffers"));

  return FALSE;
}



// BOOL             bWrite;             TRUE for Write, FALSE for Read

int ReadWriteCylinder(PDISKINFO pdi, LPBYTE pBuf, BOOL bWrite, UINT nCylinder)
{
    UINT nHead;

    DebugMsg(DM_TRACE, TEXT("%s Cylinder %d"), bWrite ? TEXT("Write") : TEXT("Read"), nCylinder);

    /* Perform the operation for all the heads for a given cylinder */
    for (nHead = 0; nHead < pdi->nHeads; nHead++, pBuf += pdi->nSectorsPerTrack * pdi->nSectorSize)
    {

Retry:
        if (bWrite)
        {
            if (!ReadWriteSector(pBuf, IOCTL_WRITE,
                pdi->nDestDrive, nCylinder, nHead, pdi->nSectorsPerTrack))
            {
                // that didn't work, try formatting
                if (!pdi->bFormatTried)
                {
                    DebugMsg(DM_ERROR, TEXT("ReadWriteCylinder() write failed, trying format"));

                    pdi->bFormatTried = TRUE;

                    switch (FormatAllTracks(pdi, nCylinder, nHead)) {
                    case FORMAT_ERROR:
                        return -1;  /* Failure or user cancel */

                    case FORMAT_SUCCESS:
                        break;
                    }
                    SetStatusText(pdi, IDS_WRITING);

                    if (!ReadWriteSector(pBuf, IOCTL_WRITE,
                                         pdi->nDestDrive, nCylinder, nHead, pdi->nSectorsPerTrack)) {
                        pdi->dwError = IDS_ERROR_WRITE;
                        goto PromptRetry;
                    }
                }
                else
                   return -1;
            }
        }
        else
        {
            if (!ReadWriteSector(pBuf, IOCTL_READ,
                pdi->nSrcDrive, nCylinder, nHead, pdi->nSectorsPerTrack))
            {
                pdi->dwError = IDS_ERROR_READ;

PromptRetry:
                if (ErrorMessageBox(pdi, MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY) {
                    pdi->dwError = 0;
                    goto Retry;
                }
                DebugMsg(DM_ERROR, TEXT("RWS Failed %d %d %d"), nCylinder, nHead, pdi->nSectorsPerTrack);
                return -1;
            }
        }
    }
    return 0;
}


// BOOL bWrite  TRUE for Write, FALSE for Read
//
// reads or writes as many cylinders as possible using whats in
// pCopyBuffer
//
// returns:
//      the next cylinder to be read.
//

int ReadWriteMaxPossible(PDISKINFO pdi, BOOL bWrite, UINT nStartCylinder)
{
    LPBYTE pBuf;

    SetStatusText(pdi, bWrite ? IDS_WRITING : IDS_READING);

    pdi->bFormatTried = FALSE;

    // buffer needs to be multiple of cyl size
    Assert((pdi->dwCopyBufferSize % pdi->nCylinderSize) == 0);

    /* We will read a cylinder only if we can read the entire cylinder. */

    for (pBuf = pdi->pCopyBuffer;
         pBuf < (pdi->pCopyBuffer + pdi->dwCopyBufferSize);
         pBuf += pdi->nCylinderSize)
    {
        if (pdi->bUserAbort)
            return -1;

        if (ReadWriteCylinder(pdi, pBuf, bWrite, nStartCylinder))
        {
            DebugMsg(DM_ERROR, TEXT("ReadWriteCylinder failed"));
            return -1;
        }

        nStartCylinder++;

        SendDlgItemMessage(pdi->hdlg, IDD_PROBAR, PBM_DELTAPOS, 1, 0);

        /* Have we read/written all the cylinders? */
        if (nStartCylinder >= pdi->nCylinders)
            break;
    }
    return nStartCylinder;
}


void RestoreDPB(int nDisk, PDEVPB pDEVPB)
{
    if (pDEVPB)
    {
        pDEVPB->SplFunctions = 4;
        DriveIOCTL(nDisk, IOCTL_SET_DPB, pDEVPB);
        LocalFree(pDEVPB);
    }
}

BOOL LockDrive(int iDrive, BOOL fLock, WORD wPermissions)
{
    int idCmd = fLock ? IOCTL_LOCK : IOCTL_UNLOCK;
    DWORD reg[7];
    BOOL bRet;
    DWORD cbBytes;
    HANDLE h;

    reg[0] = MAKELONG(iDrive + 1, LFS_OPT_EXCLUSIVE);    // make 1 based drive number and lock level
    reg[1] = MAKELONG(wPermissions, wPermissions);     // permissions
    reg[2] = idCmd;           // device specific command code
    reg[3] = 0x440D;        // generic read ioctl
    reg[6] = 0x0001;        // flags, assume error (carry)

    h = CreateFile(c_szVWIN32, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (h != INVALID_HANDLE_VALUE)
    {
        DeviceIoControl(h, 1, &reg, sizeof(reg), &reg, sizeof(reg), &cbBytes, 0);
        CloseHandle(h);
    }

    bRet = !(reg[6] & 0x0001);
    DebugMsg(DM_TRACE, TEXT("LockDrive bRet = %d"), bRet);
    return bRet;
}

DWORD CALLBACK CopyDiskThreadProc(DISKINFO *pdi)
{
    int rc = -1;
    UINT nCylinder, nNextCylinder;
    BPB BootBPB;        /* Boot Drive's BPB (taken from Boot sector) */
#ifdef DBCS //NEC_98
// we need this for 3mode as well.
//
    BPB DstBootBPB;     /* Destination BPB */
#endif
    DEVPB dpbSrcParams, dpbDstParams;
    PDEVPB pSaveSrcParams, pSaveDstParams;
    BOOL bSingleDrive = pdi->nSrcDrive == pdi->nDestDrive;
    HWND hwndProgress;

    EnableWindow(GetDlgItem(pdi->hdlg, IDD_FROM), FALSE);
    EnableWindow(GetDlgItem(pdi->hdlg, IDD_TO), FALSE);

    DebugMsg(DM_TRACE, TEXT("CopyDisk %d -> %d"), pdi->nSrcDrive, pdi->nDestDrive);

    hwndProgress = GetDlgItem(pdi->hdlg, IDD_PROBAR);
    // src and dest the same, special case this.
    if (!PromptInsertDisk(pdi, bSingleDrive ? MAKEINTRESOURCE(IDS_INSERTSRC) : MAKEINTRESOURCE(IDS_INSERTSRCDEST), TRUE)) {
        goto Failure;
    }

    // lock the drive.  do it here after the prompt so that the user
    // has a chance to view the disk and make sure it's right
    LockDrive(pdi->nSrcDrive, TRUE, 0);
    if (pdi->nSrcDrive != pdi->nDestDrive)
        LockDrive(pdi->nDestDrive, TRUE, 0);


    /* Get the BiosParameterBlock of source drive */
    if (!GetBPB(pdi->nSrcDrive, &dpbSrcParams, 1))
    {
        DebugMsg(DM_ERROR, TEXT("Bad source disk"));
        goto BadSourceDisk;
    }

    /* Get the BiosParameterBlock of the Source Diskette */
    if (!GetBootBPB(pdi->nSrcDrive, &BootBPB))
    {
        DebugMsg(DM_ERROR, TEXT("Bad source disk boot sector"));
BadSourceDisk:
        pdi->dwError = IDS_SRCDISKBAD;
        ShellMessageBox(g_hinst, pdi->hdlg, MAKEINTRESOURCE(IDS_SRCDISKBAD), NULL, MB_ICONHAND | MB_OK);
        goto Failure;
    }

#if defined(DBCS) && !defined(NEC_98)
    // Reject 1024 b/sec in case 3 mode FDD.

    if (BootBPB.cbSec == 1024)
    {
        pdi->dwError = IDS_SRCDISK1024;
        ShellMessageBox(g_hinst, pdi->hdlg, MAKEINTRESOURCE(IDS_SRCDISK1024), NULL, MB_ICONERROR | MB_OK);
        goto Failure;
    }
#endif

    if (dpbSrcParams.devAtt & DEVPB_DEVATT_DMF) {
        pdi->dwError = IDS_SRCDISKDMF;
        ShellMessageBox(g_hinst, pdi->hdlg, MAKEINTRESOURCE(IDS_SRCDISKDMF), NULL, MB_ICONERROR | MB_OK);
        goto Failure;
    }

    /* Get the BPB and DPB for the Destination drive also; */
    if (!bSingleDrive)
    {
#ifdef NEC_98
    /* Since NEC_98 3.5" FD drive can handle both 1.25MB/1.21MB media and 1.44
       MB media, we should check actaul medias for both src and dest are
       completely same. */
        if (!GetBootBPB(pdi->nDestDrive, &DstBootBPB))
#else
        if (!GetBPB(pdi->nDestDrive, &dpbDstParams, 0))
#endif
        {
            DebugMsg(DM_ERROR, TEXT("Bad dest disk"));
            pdi->dwError = IDS_DSTDISKBAD;
            ShellMessageBox(g_hinst, pdi->hdlg, MAKEINTRESOURCE(IDS_DSTDISKBAD), NULL, MB_ICONHAND | MB_OK);
            goto Failure;
        }
#ifdef  NEC_98
      /* Set ACTUAL BPB, not device default BPB due to above reason */
        dpbSrcParams.BPB = BootBPB;
        dpbDstParams.BPB = DstBootBPB;
#endif

      /* Compare BPB of source and Dest to see if they are compatible */
        if (!(CheckBPBCompatibility(&dpbSrcParams, &dpbDstParams)))
        {
            DebugMsg(DM_ERROR, TEXT("disks don't match"));
            pdi->dwError = IDS_COPYSRCDESTINCOMPAT;
            ShellMessageBox(g_hinst, pdi->hdlg, MAKEINTRESOURCE(IDS_COPYSRCDESTINCOMPAT), NULL, MB_ICONHAND | MB_OK);
            goto Failure;
        }
    }

    if (!ModifyDeviceParams(pdi->nSrcDrive, &dpbSrcParams, &pSaveSrcParams, &dpbSrcParams.BPB, &BootBPB))
    {
        DebugMsg(DM_ERROR, TEXT("can't set device params for source"));
        goto Failure;
    }

    if (!bSingleDrive)
    {
        if (!ModifyDeviceParams(pdi->nDestDrive, &dpbDstParams, &pSaveDstParams, &dpbDstParams.BPB, &BootBPB))
        {
            DebugMsg(DM_ERROR, TEXT("can't set device params for dest"));
            RestoreDPB(pdi->nSrcDrive, pSaveSrcParams);
            goto Failure;
        }
    }

    pdi->nCylinderSize    = BootBPB.secPerTrack * BootBPB.cbSec * BootBPB.cHead;
    pdi->nCylinders       = BootBPB.cSec / (BootBPB.secPerTrack * BootBPB.cHead);
    pdi->nHeads           = BootBPB.cHead;
    pdi->nSectorsPerTrack = BootBPB.secPerTrack;
    pdi->nSectorSize      = BootBPB.cbSec;

    if (!pdi->nCylinderSize || !pdi->nCylinders) {
        pdi->dwError = IDS_ERROR_GENERAL;
        ErrorMessageBox(pdi, MB_OK | MB_ICONERROR);
        goto Failure;
    }

    PostMessage(hwndProgress, PBM_SETRANGE, 0, MAKELONG(0, (WORD)pdi->nCylinders * 2));

    // In case we need to format the destination diskette, we need to know the
    // track layout; So, build a DPB with the required track layout

    pdi->pTrackLayout = BuildDEVPB(&dpbSrcParams);
    if (!pdi->pTrackLayout)
        goto Failure0;

    /* The following is required to format a 360KB floppy in a 1.2MB
     * drive of NCR PC916 machine; We do formatting, if the destination
     * floppy is an unformatted one;
     * Fix for Bug #6894 --01-10-90-- SANKAR --
     */
    if (pdi->pTrackLayout->BPB.bMedia == MEDIA_360)
    {
        pdi->pTrackLayout->NumCyls = 40;
        pdi->pTrackLayout->bMediaType = 1;
    }

    /* We wish we could do the following allocation at the begining of this
     * function, but we can not do so, because we need di
     * and we just got it;
     */
    if (!AllocCopyDiskBuffers(pdi))
    {
        // ShellMessageBox(g_hinst, pdi->hdlg, MAKEINTRESOURCE(IDS_REASONS+DE_INSMEM), NULL, MB_ICONHAND | MB_OK);
        goto Failure0;
    }

    for (nCylinder = 0; nCylinder < pdi->nCylinders; nCylinder = nNextCylinder)
    {
        // Do not prompt for the first time, because the Source diskette is
        // already in the drive.

        if (bSingleDrive && (nCylinder > 0))
        {
            LockDrive(pdi->nSrcDrive, FALSE, 0);
            if (!PromptInsertDisk(pdi, MAKEINTRESOURCE(IDS_INSERTSRC), FALSE)) {
                pdi->bUserAbort = TRUE;
                goto Failure0;
            }
            LockDrive(pdi->nSrcDrive, TRUE, 0);
        }

        // Read in the current cylinders

        rc = ReadWriteMaxPossible(pdi, FALSE, nCylinder);
        if (rc < 0)
            break;
        else
            nNextCylinder = rc;

        // If this is a single drive system, ask the user to insert
        // the destination diskette.

        if (bSingleDrive)
        {
            LockDrive(pdi->nSrcDrive, FALSE, 0);
            if (!PromptInsertDisk(pdi, MAKEINTRESOURCE(IDS_INSERTDEST), FALSE)) {
                pdi->bUserAbort = TRUE;
                goto Failure0;
            }
            LockDrive(pdi->nSrcDrive, TRUE, 0);
#ifdef DBCS // NEC_98
// we need this for 3mode as well.
//
            /* Get destination media BPB */
            if (!GetBootBPB(pdi->nSrcDrive, &DstBootBPB))
            {
                DebugMsg(DM_ERROR, "Bad dest disk");
                pdi->dwError = IDS_DSTDISKBAD;
                ShellMessageBox(g_hinst, pdi->hdlg, MAKEINTRESOURCE(IDS_DSTDISKBAD), NULL, MB_ICONHAND | MB_OK);
                rc = -1;
                break;
            }
            dpbDstParams.BPB = DstBootBPB;
            /* Compare BPB of source and Dest to see if they are compatible */
            if (!(CheckBPBCompatibility(&dpbSrcParams, &dpbDstParams)))
            {
                DebugMsg(DM_ERROR, "disks don't match");
                pdi->dwError = IDS_COPYSRCDESTINCOMPAT;
                ShellMessageBox(g_hinst, pdi->hdlg, MAKEINTRESOURCE(IDS_COPYSRCDESTINCOMPAT), NULL, MB_ICONHAND | MB_OK);
                rc = -1;
                break;
            }
#endif
        }

        // Write out the current cylinders
        rc = ReadWriteMaxPossible(pdi, TRUE, nCylinder);
        if (rc < 0)
            break;
    }

    if (pdi->pCopyBuffer)
    {
        GlobalFree(pdi->pCopyBuffer);
        pdi->pCopyBuffer = NULL;
    }

Failure0:

    // Reset the Source drive parameters to the same as old
    RestoreDPB(pdi->nSrcDrive, pSaveSrcParams);

    if (!bSingleDrive)
        RestoreDPB(pdi->nDestDrive, pSaveDstParams);

    if (pdi->pTrackLayout)
    {
        LocalFree(pdi->pTrackLayout);
        pdi->pTrackLayout = NULL;
    }

Failure:

    PostMessage(pdi->hdlg, WM_DONE_WITH_FORMAT, 0, 0);

    return rc;
}
#else



typedef struct _fmifs {
    HANDLE hDll;
    PFMIFS_DISKCOPY_ROUTINE DiskCopy;
} FMIFS;
typedef FMIFS *PFMIFS;


BOOL LoadFMIFS(PFMIFS pFMIFS)
{
    //
    // Load the FMIFS DLL and query for the entry points we need
    //

    pFMIFS->hDll = LoadLibrary(TEXT("FMIFS.DLL"));

    if (NULL == pFMIFS->hDll)
        return FALSE;

    pFMIFS->DiskCopy = (PFMIFS_DISKCOPY_ROUTINE)GetProcAddress(pFMIFS->hDll,
                                                               "DiskCopy");

    if (NULL == pFMIFS->DiskCopy)
    {
        FreeLibrary(pFMIFS->hDll);
        pFMIFS->hDll = (HANDLE)0;
        return FALSE;
    }

    return TRUE;
}

void UnloadFMIFS(PFMIFS pFMIFS)
{
    FreeLibrary(pFMIFS->hDll);
    pFMIFS->hDll = NULL;
    pFMIFS->DiskCopy = NULL;
}

BOOL FMIFSDriveIdIsFloppy(int iDrive, BOOL *pfIsFloppy)
{
    FMIFS fmifs;
    BOOL fResult = FALSE;
    WCHAR szDriveString[8] = L"\\\\.\\a:";
    HANDLE DeviceHandle;
    DWORD FsFlags, BytesReturned;
    DISK_GEOMETRY Geometry;

    *pfIsFloppy = FALSE;
    if (iDrive < 0)
        return TRUE;

    // Get drive letter from 0-based number
    szDriveString[4] = (WCHAR) ((int) L'a' + iDrive);

    // Open the device using WIN32 API.
    //
    DeviceHandle = CreateFile( szDriveString,
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_NO_BUFFERING,
                               NULL );

    if( DeviceHandle == INVALID_HANDLE_VALUE ) {

        // This drive doesn't exist, or I can't open it.
        //
        return FALSE;
    }

    if( DeviceIoControl( DeviceHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY,
                          NULL, 0, &Geometry, sizeof( Geometry ), &BytesReturned, 0 ) ) 
    {
        *pfIsFloppy = (Geometry.MediaType != Unknown) &&
                   (Geometry.MediaType != RemovableMedia) &&
                   (Geometry.MediaType != FixedMedia);
    }

    CloseHandle( DeviceHandle );
    return TRUE;
}

//
// Thread-Local Storage index for our DISKINFO structure pointer
//

static DWORD g_iTLSDiskInfo = 0;
static LONG  g_cTLSDiskInfo = 0;  // Usage count

__inline void UnstuffDiskInfoPtr()
{
    if (InterlockedDecrement(&g_cTLSDiskInfo) == 0)
        TlsFree(g_iTLSDiskInfo);
}

BOOL StuffDiskInfoPtr(PDISKINFO pDiskInfo)
{
    //
    // Allocate an index slot for our thread-local DISKINFO pointer, if one
    // doesn't already exist, then stuff our DISKINFO ptr at that index.
    //

    if (0 == g_iTLSDiskInfo)
    {
        if (0xFFFFFFFF == (g_iTLSDiskInfo = TlsAlloc()))
        {
            return FALSE;
        }
        g_cTLSDiskInfo = 0;
    }

    InterlockedIncrement(&g_cTLSDiskInfo);

    if (!TlsSetValue(g_iTLSDiskInfo, (LPVOID) pDiskInfo))
    {
       UnstuffDiskInfoPtr();
       return FALSE;
    }

    return TRUE;
}

__inline PDISKINFO GetDiskInfoPtr()
{
    return TlsGetValue(g_iTLSDiskInfo);
}


// DriveNumFromDriveLetterW: Return a drive number given a pointer to
//  a unicode drive letter.
// 02/03/98: dsheldon created
int DriveNumFromDriveLetterW(wchar_t* pwchDrive)
{
	Assert(pwchDrive != NULL);

	return ( ((int) *pwchDrive) - ((int) L'A') );
}

/*
 Function: CopyDiskCallback

 Return Value:
		TRUE - Normally, TRUE should be returned if the Disk Copy procedure should
		 continue after CopyDiskCallback returns. Note the HACK below, however!
		FALSE - Normally, this indicates that the Disk Copy procedure should be
		 cancelled.


		!HACKHACK!

		The low-level Disk Copy procedure that invokes this callback is also used
		by the command-line DiskCopy utility. That utility's implementation of the
		callback always returns TRUE. For this reason, the low-level Disk Copy
		procedure will interpret TRUE as CANCEL when it is returned from callbacks
		that display a message box and allow the user to possibly RETRY an operation.
		Therefore, return TRUE after handling such messages to tell the Disk Copy
		procedure to abort, and return FALSE to tell Disk Copy to retry.

		TRUE still means 'continue' when returned from PercentComplete or Disk Insertion
		messages.

 Revision:
		02/03/98: dsheldon - modified code to handle retry/cancel for bad media,
			write protected media, and disk being yanked out of drive during copy

*/

BOOLEAN CopyDiskCallback( FMIFS_PACKET_TYPE PacketType, DWORD PacketLength, PVOID PacketData)
{
    PDISKINFO pdi = GetDiskInfoPtr();
    int iDisk;

    // Quit if told to do so..
    if (pdi->bUserAbort)
       return FALSE;

    switch (PacketType) {
        case FmIfsPercentCompleted:
          {
            DWORD dwPercent = ((PFMIFS_PERCENT_COMPLETE_INFORMATION)
                                            PacketData)->PercentCompleted;

            //
            // Hokey method of determining "writing"
            //
            if (dwPercent > 50 && !pdi->bNotifiedWriting)
            {
                pdi->bNotifiedWriting = TRUE;
                SetStatusText(pdi, IDS_WRITING);
            }

            SendDlgItemMessage(pdi->hdlg, IDD_PROBAR, PBM_SETPOS, dwPercent,0);
            break;
          }
        case FmIfsInsertDisk:

            switch(((PFMIFS_INSERT_DISK_INFORMATION)PacketData)->DiskType) {
                case DISK_TYPE_SOURCE:
                case DISK_TYPE_GENERIC:
                    iDisk = IDS_INSERTSRC;
                    break;

                case DISK_TYPE_TARGET:
                    iDisk = IDS_INSERTDEST;
					pdi->bDestInserted = TRUE;
                    break;
                case DISK_TYPE_SOURCE_AND_TARGET:
                    iDisk = IDS_INSERTSRCDEST;
                    break;
            }
            if (!PromptInsertDisk(pdi, MAKEINTRESOURCE(iDisk), FALSE)) {
                pdi->bUserAbort = TRUE;
                return FALSE;
            }

            break;

        case FmIfsFormattingDestination:
            pdi->bNotifiedWriting = FALSE;      // Reset so we get Writing later
            SetStatusText(pdi, IDS_FORMATTINGDEST);
            break;

        case FmIfsIncompatibleFileSystem:
        case FmIfsIncompatibleMedia:
            pdi->dwError = IDS_COPYSRCDESTINCOMPAT;
            if (ErrorMessageBox(pdi, MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
            {
                pdi->dwError = 0;
				return FALSE;	//Indicates RETRY - see HACK in function header
            }
            else
            {
                return TRUE;
            }
            break;

        case FmIfsMediaWriteProtected:
            pdi->dwError = IDS_DSTDISKBAD;
            if (ErrorMessageBox(pdi, MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
            {
                pdi->dwError = 0;
				return FALSE;	//Indicates RETRY - see HACK in function header
            }
            else
            {
                return TRUE;
            }
            break;

        case FmIfsCantLock:
            // BUGBUG - BobDay - We should do something for this!
            pdi->dwError = IDS_ERROR_GENERAL;
            ErrorMessageBox(pdi, MB_OK | MB_ICONERROR);
            return FALSE;

        case FmIfsAccessDenied:
			pdi->dwError = IDS_SRCDISKBAD;
			ErrorMessageBox(pdi, MB_OK | MB_ICONERROR);
			return FALSE;

        case FmIfsBadLabel:
        case FmIfsCantQuickFormat:
            pdi->dwError = IDS_ERROR_GENERAL;
            ErrorMessageBox(pdi, MB_OK | MB_ICONERROR);
            return FALSE;

        case FmIfsIoError:
            switch(((PFMIFS_IO_ERROR_INFORMATION)PacketData)->DiskType) {
                case DISK_TYPE_SOURCE:
                    pdi->dwError = IDS_SRCDISKBAD;
                    break;
                case DISK_TYPE_TARGET:
                    pdi->dwError = IDS_DSTDISKBAD;
                    break;
                default:
                    // BUGBUG - BobDay - We should never get this!!
                    pdi->dwError = IDS_ERROR_GENERAL;
                    break;
            }

            if (ErrorMessageBox(pdi, MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
            {
                pdi->dwError = 0;
				return FALSE;	//Indicates RETRY - see HACK in function header
            }
            else
            {
                return TRUE;
            }
            break;

		case FmIfsNoMediaInDevice:
			{
				// Note that we get a pointer to the unicode
				// drive letter in the PacketData argument

				// If the drives are the same, determine if we are
				// reading or writing with the "dest inserted" flag
				if (pdi->nSrcDrive == pdi->nDestDrive)
				{
					if (pdi->bDestInserted)
						pdi->dwError = IDS_ERROR_WRITE;
					else
						pdi->dwError = IDS_ERROR_READ;
				}
				else
				{
					// Otherwise, use the drive letter to determine this
					// ...Check if we're reading or writing
					int nDrive = DriveNumFromDriveLetterW(
						(wchar_t*) PacketData);

					Assert ((nDrive == pdi->nSrcDrive) || 
						(nDrive == pdi->nDestDrive));

					// Check if the source or dest disk was removed and set
					// error accordingly
					
					if (nDrive == pdi->nDestDrive)
						pdi->dwError = IDS_ERROR_WRITE;
					else
						pdi->dwError = IDS_ERROR_READ;
				}
				
				if (ErrorMessageBox(pdi, MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
				{
					pdi->dwError = 0;

					// Note that FALSE is returned here to indicate RETRY
					// See HACK in the function header for explanation.
					return FALSE;
				}
				else
				{
					return TRUE;
				}
			}
			break;
				

        case FmIfsFinished:
            if (((PFMIFS_FINISHED_INFORMATION)PacketData)->Success)
            {
                pdi->dwError = 0;
            }
            else
            {
                pdi->dwError = IDS_ERROR_GENERAL;
            }
            break;

        default:
            break;
    }
    return TRUE;
}


// nDrive == 0-based drive number (a: == 0)
LPITEMIDLIST GetDrivePidl(HWND hwnd, int nDrive)
{
    IShellFolder* psfDesktop = NULL;
    LPITEMIDLIST pidl = NULL;

    if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
    {
        ULONG cchEaten;
        static WCHAR szDriveString[4] = L"a:\\";

        // Get drive letter from 0-based number
        szDriveString[0] = (WCHAR) ((int) L'a' + nDrive);

        if (FAILED(psfDesktop->lpVtbl->ParseDisplayName(psfDesktop, hwnd, 
            0, szDriveString, &cchEaten, &pidl, NULL)))
        {
            pidl = NULL;
        }

        psfDesktop->lpVtbl->Release(psfDesktop);
    }    

    return pidl;
}

DWORD CALLBACK CopyDiskThreadProc(DISKINFO *pdi)
{
    FMIFS fmifs;
    LPITEMIDLIST pidlSrc = NULL;
    LPITEMIDLIST pidlDest = NULL;
    HWND hwndProgress = GetDlgItem(pdi->hdlg, IDD_PROBAR);

    // Disable change notifications for the src drive
    pidlSrc = GetDrivePidl(pdi->hdlg, pdi->nSrcDrive);
    if (NULL != pidlSrc)
    {
        SHChangeNotifySuspendResume(TRUE, pidlSrc, TRUE, 0);
    }

    if (pdi->nSrcDrive != pdi->nDestDrive)
    {
        // Do the same for the dest drive since they're different
        pidlDest = GetDrivePidl(pdi->hdlg, pdi->nDestDrive);

        if (NULL != pidlDest)
        {
            SHChangeNotifySuspendResume(TRUE, pidlDest, TRUE, 0);
        }
    }

    // Change notifications are disabled; do the copy
    EnableWindow(GetDlgItem(pdi->hdlg, IDD_FROM), FALSE);
    EnableWindow(GetDlgItem(pdi->hdlg, IDD_TO), FALSE);

    PostMessage(hwndProgress, PBM_SETRANGE, 0, MAKELONG(0, 100));

    pdi->bFormatTried = FALSE;
    pdi->bNotifiedWriting = FALSE;
    pdi->dwError = 0;
	pdi->bDestInserted = FALSE;

    if (StuffDiskInfoPtr(pdi) && LoadFMIFS(&fmifs))
    {
        TCHAR szSource[3];
        TCHAR szDestination[3];

        //
        // Now copy the disk
        //
        szSource[0] = TEXT('A') + pdi->nSrcDrive;
        szSource[1] = TEXT(':');
        szSource[2] = 0;

        szDestination[0] = TEXT('A') + pdi->nDestDrive;
        szDestination[1] = TEXT(':');
        szDestination[2] = 0;

        SetStatusText(pdi, IDS_READING);

        fmifs.DiskCopy(szSource, szDestination, FALSE, CopyDiskCallback);

        UnstuffDiskInfoPtr();
        UnloadFMIFS(&fmifs);
    }

    PostMessage(pdi->hdlg, WM_DONE_WITH_FORMAT, 0, 0);

    // Resume any shell notifications we've suspended and free
    // our pidls (and send updatedir notifications while we're at
    // it)
    if (NULL != pidlSrc)
    {
        SHChangeNotifySuspendResume(FALSE, pidlSrc, TRUE, 0);
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidlSrc, NULL);
        ILFree(pidlSrc);
        pidlSrc = NULL;
    }

    if (NULL != pidlDest)
    {
        Assert(pdi->nSrcDrive != pdi->nDestDrive);
        SHChangeNotifySuspendResume(FALSE, pidlDest, TRUE, 0);
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_IDLIST, pidlDest, NULL);
        ILFree(pidlDest);
        pidlDest = NULL;
    }

    return 0;
}
#endif

BOOL DriveIdIsFloppy(int iDrive)
{
#ifdef WINNT
    BOOL fIsFloppy;
    if (FMIFSDriveIdIsFloppy(iDrive, &fIsFloppy))
        return fIsFloppy;
#endif
    return ((IsRemovableDrive(iDrive) && !IsCDRomDrive(iDrive)));
}

int ErrorMessageBox(DISKINFO* pdi, UINT uFlags)
{
    if (!pdi->bUserAbort && pdi->dwError) {
        TCHAR szTemp[1024];
        DWORD dwLastError = GetLastError();

        DebugMsg(DM_TRACE, TEXT("ERROR %d %d"), pdi->dwError, dwLastError);
        if (dwLastError) {
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwLastError, 0, szTemp, sizeof(szTemp), NULL);
        } else {
            LoadString(g_hinst, (int)pdi->dwError, szTemp, sizeof(szTemp));
        }

        // if the user didn't abort and it didn't complete normally, post an error box
        return ShellMessageBox(g_hinst, pdi->hdlg, szTemp, NULL, uFlags);
    } else
        return -1;
}

void SetStatusText(DISKINFO* pdi, int id)
{
    TCHAR szMsg[128];
    LoadString(g_hinst, id, szMsg, sizeof(szMsg));
    SendDlgItemMessage(pdi->hdlg, IDD_STATUS, WM_SETTEXT, 0, (LPARAM)szMsg);
}

BOOL PromptInsertDisk(DISKINFO *pdi, LPCTSTR lpsz, BOOL fAutoCheck)
{
    if (fAutoCheck)
        goto AutoCheckBegin;

    for (;;) {
        DWORD dwLastErrorSrc = 0;
        DWORD dwLastErrorDest = 0 ;
        TCHAR szPath[4];

        if (ShellMessageBox(g_hinst, pdi->hdlg, lpsz, NULL, MB_OKCANCEL | MB_ICONINFORMATION) != IDOK) {
            pdi->bUserAbort = TRUE;
            return FALSE;
        }

    AutoCheckBegin:
        szPath[0] = TEXT('A') + pdi->nSrcDrive;
        szPath[1] = TEXT(':');
        szPath[2] = TEXT('\\');
        szPath[3] = 0;

        // make sure both disks are in
        if (GetFileAttributes(szPath) == (UINT)-1)
            dwLastErrorDest = GetLastError();

        if (pdi->nDestDrive != pdi->nSrcDrive) {
            szPath[0] = TEXT('A') + pdi->nDestDrive;
            if (GetFileAttributes(szPath) == (UINT)-1)
                dwLastErrorDest = GetLastError();
        }

        if (dwLastErrorDest != ERROR_NOT_READY &&
            dwLastErrorSrc != ERROR_NOT_READY)
            break;
    }

    return TRUE;
}


HICON GetDriveInfo(int nDrive, LPTSTR pszName)
{
    SHFILEINFO shfi;
    TCHAR szRoot[4];

    PathBuildRoot(szRoot, nDrive);

    if (SHGetFileInfo(szRoot, FILE_ATTRIBUTE_DIRECTORY, &shfi, sizeof(shfi),
        SHGFI_ICON | SHGFI_SMALLICON | SHGFI_DISPLAYNAME)) //  | SHGFI_USEFILEATTRIBUTES
    {
        lstrcpy(pszName, shfi.szDisplayName);
        return shfi.hIcon;
    }
    else
    {
        lstrcpy(pszName, szRoot);
        return NULL;
    }
}

int AddDriveToListView(HWND hwndLV, int nDrive, int nDefaultDrive)
{
    TCHAR szDriveName[64];
    LV_ITEM item;
    HICON hicon = GetDriveInfo(nDrive, szDriveName);
    HIMAGELIST himlSmall = ListView_GetImageList(hwndLV, LVSIL_SMALL);

    Assert(himlSmall);
    if (hicon)
    {
        item.iImage = ImageList_AddIcon(himlSmall, hicon);
        DestroyIcon(hicon);
    }
    else
        item.iImage = 0;

    item.mask = nDrive == nDefaultDrive ?
        LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE :
        LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;

    item.stateMask = item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.iItem = 26;     // add at end
    item.iSubItem = 0;

    item.pszText = szDriveName;
    item.lParam = (LPARAM)nDrive;

    return ListView_InsertItem(hwndLV, &item);
}

int GetSelectedDrive(HWND hwndLV)
{
    LV_ITEM item;
    item.iItem = ListView_GetNextItem(hwndLV, -1, LVNI_SELECTED);
    if (item.iItem >= 0)
    {
        item.mask = LVIF_PARAM;
        item.iSubItem = 0;
        ListView_GetItem(hwndLV, &item);
        return (int)item.lParam;
    }

    // implicitly selected the 0th item
    ListView_SetItemState(hwndLV, 0, LVIS_SELECTED, LVIS_SELECTED);
    return 0;
}

void InitSingleColListView(HWND hwndLV)
{
    LV_COLUMN col = {LVCF_FMT | LVCF_WIDTH, LVCFMT_LEFT};
    RECT rc;

    GetClientRect(hwndLV, &rc);
    col.cx = rc.right;
    //  - GetSystemMetrics(SM_CXVSCROLL)
    //        - GetSystemMetrics(SM_CXSMICON)
    //        - 2 * GetSystemMetrics(SM_CXEDGE);
    ListView_InsertColumn(hwndLV, 0, &col);
}

#define g_cxSmIcon  GetSystemMetrics(SM_CXSMICON)

void CopyDiskInitDlg(HWND hDlg, DISKINFO *pdi)
{
    int iDrive;
    HWND hwndFrom = GetDlgItem(hDlg, IDD_FROM);
    HWND hwndTo   = GetDlgItem(hDlg, IDD_TO);
    HIMAGELIST himl;

    SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pdi);

    SendMessage(hDlg, WM_SETICON, 0, (LPARAM)LoadImage(GetWindowInstance(hDlg), MAKEINTRESOURCE(IDI_DISKCOPY), IMAGE_ICON, 16, 16, 0));
    SendMessage(hDlg, WM_SETICON, 1, (LPARAM)LoadIcon(GetWindowInstance(hDlg), MAKEINTRESOURCE(IDI_DISKCOPY)));

    pdi->hdlg = hDlg;

    InitSingleColListView(hwndFrom);
    InitSingleColListView(hwndTo);

    himl = ImageList_Create(g_cxSmIcon, g_cxSmIcon, ILC_MASK, 1, 4);
    if (himl)
    {
        // NOTE: only one of these is not marked LVS_SHAREIMAGELIST
        // so it will only be destroyed once

        ListView_SetImageList(hwndFrom, himl, LVSIL_SMALL);
        ListView_SetImageList(hwndTo, himl, LVSIL_SMALL);
    }

    for (iDrive = 0; iDrive < 26; iDrive++)
    {
        if (DriveIdIsFloppy(iDrive))
        {
            AddDriveToListView(hwndFrom, iDrive, pdi->nSrcDrive);
            AddDriveToListView(hwndTo, iDrive, pdi->nDestDrive);
        }
    }
}


DWORD _inline WaitForThreadDeath(HWND hThread)
{
    MSG msg;
    DWORD result = WAIT_FAILED;

    if (hThread) {
        while(TRUE) {
            result = MsgWaitForMultipleObjects(1, &hThread, FALSE, 5000, QS_SENDMESSAGE);
            switch (result) {
            default:
            case WAIT_OBJECT_0:
            case WAIT_FAILED:
                return result;

            case WAIT_TIMEOUT:
                TerminateThread(hThread, (DWORD)-1);
                return result;

            case WAIT_OBJECT_0 + 1:
                PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
                break;
            }
        }
    }

    return(result);
}

void SetCancelButtonText(HWND hDlg, int id)
{
    TCHAR szText[80];
    LoadString(g_hinst, id, szText, sizeof(szText));
    SetDlgItemText(hDlg, IDCANCEL, szText);
}

void DoneWithFormat(DISKINFO* pdi)
{
    int id;

    EnableWindow(GetDlgItem(pdi->hdlg, IDD_FROM), TRUE);
    EnableWindow(GetDlgItem(pdi->hdlg, IDD_TO), TRUE);

#ifndef WINNT
    // unlock the drives
    LockDrive(pdi->nSrcDrive, FALSE, 0 );
    if (pdi->nSrcDrive != pdi->nDestDrive)
        LockDrive(pdi->nDestDrive, FALSE, 0);
#endif

    SendDlgItemMessage(pdi->hdlg, IDD_PROBAR, PBM_SETPOS, 0, 0);
    EnableWindow(GetDlgItem(pdi->hdlg, IDOK), TRUE);

    CloseHandle(pdi->hThread);
    SetCancelButtonText(pdi->hdlg, IDS_CLOSE);
    pdi->hThread = NULL;

    if (pdi->bUserAbort) {
        id = IDS_COPYABORTED;
    } else {
        switch (pdi->dwError) {
        case 0:
            id = IDS_COPYCOMPLETED;
            break;

        default:
            id = IDS_COPYFAILED;
            break;
        }
    }
    SetStatusText(pdi, id);
    SetCancelButtonText(pdi->hdlg, IDS_CLOSE);

    // reset variables
    pdi->dwError = 0;
    pdi->bUserAbort = 0;
}


#pragma data_seg(".text")
const static DWORD aCopyDiskHelpIDs[] = {  // Context Help IDs
    IDOK,         IDH_DISKCOPY_START,
    IDD_FROM,     IDH_DISKCOPY_FROM,
    IDD_TO,       IDH_DISKCOPY_TO,
    IDD_STATUS,   NO_HELP,
    IDD_PROBAR,   NO_HELP,

    0, 0
};
#pragma data_seg()

INT_PTR CALLBACK CopyDiskDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DISKINFO *pdi = (DISKINFO *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {
    case WM_INITDIALOG:
        CopyDiskInitDlg(hDlg, (DISKINFO *)lParam);
        break;

    case WM_DONE_WITH_FORMAT:
        DoneWithFormat(pdi);
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (DWORD_PTR)(LPTSTR) aCopyDiskHelpIDs);
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID) aCopyDiskHelpIDs);
        return TRUE;

   case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDCANCEL:
            // if there's az hThread that means we're in copy mode, abort
            // from that, otherwise, it means quit the dialog completely
            if (pdi->hThread)
            {
                pdi->bUserAbort = TRUE;

                // do a Msgwaitformultiple so that we don't
                // get blocked with them sending us a message
                if (WaitForThreadDeath(pdi->hThread) == WAIT_TIMEOUT)
                    DoneWithFormat(pdi);
                CloseHandle(pdi->hThread);
                pdi->hThread = NULL;
            }
            else
                EndDialog(hDlg, IDCANCEL);
            break;

        case IDOK:
            {
            DWORD idThread;

            SetLastError(0);
            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);

            // set cancel button to "Cancel"
            SetCancelButtonText(hDlg, IDS_CANCEL);

            pdi->nSrcDrive  = GetSelectedDrive(GetDlgItem(hDlg, IDD_FROM));
            pdi->nDestDrive = GetSelectedDrive(GetDlgItem(hDlg, IDD_TO));

            pdi->bUserAbort = FALSE;

            SendDlgItemMessage(hDlg, IDD_PROBAR, PBM_SETPOS, 0, 0);
            SendDlgItemMessage(pdi->hdlg, IDD_STATUS, WM_SETTEXT, 0, 0);

            Assert(pdi->hThread == NULL);

            pdi->hThread = CreateThread(NULL, 0, CopyDiskThreadProc, pdi, 0, &idThread);
            }
            break;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

int SHCopyDisk(HWND hwnd, int nSrcDrive, int nDestDrive, DWORD dwFlags)
{
    DISKINFO di;
    memset(&di, 0, sizeof(di));

    di.nSrcDrive = nSrcDrive;
    di.nDestDrive = nDestDrive;

    return (int)DialogBoxParam(g_hinst, MAKEINTRESOURCE(DLG_DISKCOPYPROGRESS), hwnd, CopyDiskDlgProc, (LPARAM)&di);
}
