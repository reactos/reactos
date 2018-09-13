// Removeable media must be >= this size to be considered "non 3.5/5.25 floppies"

#define WD_MIN_BIG_FLOPPY (10L * 1024L * 1024L)

// Special staticdefine to put data stuff in code segment
#define FORMATSEG _based(_segname("_CODE"))

//
// Defines for devType field
//
#define DEVPB_DEVTYP_525_0360	0
#define DEVPB_DEVTYP_525_1200	1
#define DEVPB_DEVTYP_350_0720	2
#define DEVPB_DEVTYP_350_1440	7
#define DEVPB_DEVTYP_350_2880	9

// Defines for IOCTL
#define IOCTL_FORMAT		0x42
#define IOCTL_SETFLAG		0x47
#define IOCTL_MEDIASSENSE	0x68
#define	IOCTL_GET_DPB 		0x60
#define IOCTL_SET_DPB 		0x40
#define IOCTL_READ		0x61
#define IOCTL_WRITE		0x41

/*
 * Bits of TypeFlags
 */
#define DT_F350 	0x00000001L
#define DT_F525 	0x00000002L
#define DT_FBIG 	0x00000004L
#define DT_DBLDISK	0x00000008L
#define DT_LONGNAME	0x00000010L
#define DT_NOFORMAT	0x00000020L
#define DT_NOMKSYS	0x00000040L
#define DT_NOCOPY	0x00000080L
#define DT_NOCOMP	0x00000100L
#define DT_NOSURFA	0x00000200L

typedef BOOL (WINAPI *DMaint_GetEngineDriveInfoPROC)(LPDWORD lpEngInfArray);
typedef DWORD (WINAPI *DMaint_FormatDrivePROC)(LPFMTINFOSTRUCT lpFmtInfoBuf, DWORD Options, DSKUTILCBPROC lpfnCallBack, LPARAM lRefData);
typedef DWORD (WINAPI *DMaint_GetFormatOptionsPROC)(UINT Drive, LPFMTINFOSTRUCT lpFmtInfoBuf, UINT nSize);

typedef DWORD (WINAPI *DMaint_GetFixOptionsPROC)(LPDRVPARMSTRUCT lpParmBuf);
typedef DWORD (WINAPI *DMaint_FixDrivePROC)(LPDRVPARMSTRUCT lpParmBuf, DWORD Options, DSKUTILCBPROC lpfnCallBack, LPARAM lRefData);
typedef UINT (WINAPI *DMaint_GetFileSysParametersPROC)(UINT Drive, LPDRVPARMSTRUCT lpParmBuf, UINT nSize);

typedef struct _DMAINTINFO
{
	HINSTANCE hInstDMaint;
	DMaint_GetEngineDriveInfoPROC lpfnGetEngineDriveInfo;
	DMaint_FormatDrivePROC lpfnFormatDrive;
	DMaint_GetFormatOptionsPROC lpfnGetFormatOptions;

	DMaint_GetFixOptionsPROC lpfnGetFixOptions;
	DMaint_FixDrivePROC lpfnFixDrive;
	DMaint_GetFileSysParametersPROC lpfnGetFileSysParameters;
} DMAINTINFO, *LPDMAINTINFO;

/*--------------------------------------------------------------------------*/
/*  BIOS Parameter Block Structure -					    */
/*--------------------------------------------------------------------------*/

typedef struct {
    WORD    cbSec;		// Bytes per sector
    BYTE    secPerClus; 	// Sectors per cluster
    WORD    cSecRes;		// Reserved sectors
    BYTE    cFAT;		// FATS
    WORD    cDir;		// Root Directory Entries
    WORD    cSec;		// Total number of sectors in image
    BYTE    bMedia;		// Media descriptor
    WORD    secPerFAT;		// Sectors per FAT
    WORD    secPerTrack;	// Sectors per track
    WORD    cHead;		// Heads
    WORD    cSecHidden; 	// Hidden sectors
    //
    // Extended BPB starts here...
    //
    WORD    cSecHidden_HiWord;	// The high word of no of hidden sectors
    DWORD   cTotalSectors;	// Total sectors, if BPB_cSec is zero
} BPB, * PBPB, * LPBPB;

/*--------------------------------------------------------------------------*/
/*  Device Parameter Block Structure -					    */
/*--------------------------------------------------------------------------*/

#define MAX_SEC_PER_TRACK	64

typedef struct tagDevPB {
    BYTE    SplFunctions;
    BYTE    devType;
    WORD    devAtt;
    WORD    NumCyls;
    BYTE    bMediaType;  /* 0=>1.2MB and 1=>360KB */
    BPB     BPB;
    BYTE    A_BPB_Reserved[6];			 // Unused 6 BPB bytes
    BYTE    TrackLayout[MAX_SEC_PER_TRACK * 4 + 2];
} DevPB, *PDevPB, *LPDevPB;

/* Dialog user dword data struct on each drive property sheet. 
 */
typedef struct 
{
  PROPSHEETPAGE psp;
  int	    iDrive;
  UINT	    iType;
  WORD	    status;
  WORD	    DrawFlags;
  HBITMAP   hBigBitmap;
  HWND	    hDriveWindow;
  int	    ThreeDHgt;
  WORD	    MaxComponLen;		// Single name, includes trailing NUL
  WORD	    MaxPathLen; 		// Path, without "X:\", includes trailing NUL
  RECT	    EllipsRect;
  RECT	    BarRect;
  DWORD     DskMFlags;
  DWORD     TypeFlags;
  DWORD     sectorsPerCluster;
  DWORD     bytesPerSector;
  DWORD     bytesPerCluster;
  DWORD     freeClus;
  DWORD     totalClus;
  DWORD     freeBytesLo;
  DWORD     freeBytesHi;
  DWORD     totalBytesLo;
  DWORD     totalBytesHi;
  BYTE	    rootPath[4];
  BYTE	    driveNameStr[120];
} WINDISKDRIVEINFO;
typedef WINDISKDRIVEINFO*      PWINDISKDRIVEINFO;
typedef WINDISKDRIVEINFO NEAR* NPWINDISKDRIVEINFO;
typedef WINDISKDRIVEINFO  * LPWINDISKDRIVEINFO;

//
// This structure contains extra goop of ours for format as well as the
//	format engine's FMTINFOSTRUCT
//
typedef struct tagMYFMTINFOSTRUCT {
    FMTINFOSTRUCT      FmtInf;
    LPWINDISKDRIVEINFO lpwddi;
    LPFATFMTREPORT     lpFmtRep;
    DWORD	       GetFmtOpt;
    DWORD	       OpCmpltRet;
    DWORD	       YldCnt;
    HWND	       hProgDlgWnd;
    UINT	       hTimer;
    BOOL	       FmtCancelBool;
    BOOL	       FmtInProgBool;
    BOOL	       FmtIsActive;
    BYTE	       FmtOpt;
    BYTE	       CurrOpRegion;
    WORD	       LastFmtRslt;
    WORD	       ReqFmtID;
    WORD	       ReqFmtOpt;
} MYFMTINFOSTRUCT;
typedef MYFMTINFOSTRUCT*      PMYFMTINFOSTRUCT;
typedef MYFMTINFOSTRUCT NEAR* NPMYFMTINFOSTRUCT;
typedef MYFMTINFOSTRUCT FAR*  LPMYFMTINFOSTRUCT;


typedef struct Log_
{
	HANDLE		   hSz;
	DWORD		   cchUsed; // Does not include the terminating \0
	DWORD		   cchAlloced;
	BOOL		   fMemWarned;
} LOG, *LPLOG;

VOID SEInitLog(LPLOG lpLog);
VOID SEAddToLog(LPLOG lpLog, LPSTR szNew,LPSTR szPost);
//
// This structure contains extra goop of ours for CHKDSK as well as the
//	FixDrive engine's DRVPARMSTRUCT
//
#define MAXMULTSTRNGS	16
#define MAXMIARGS	5
#define MAXSEARGS	5

typedef struct tagMYCHKINFOSTRUCT {
    DRVPARMSTRUCT      ChkDPms;
    LPWINDISKDRIVEINFO lpwddi;
    LPFATFIXREPORT     lpFixRep;
    LPFIXFATDISP       lpFixFDisp;
    HWND *	       lpTLhwnd;
    DWORD	       OpCmpltRet;
    DWORD	       FixOptions;
    DWORD	       RegOptions;
    DWORD	       NoUnsupDrvs;
    LPARAM	       lParam1;
    LPARAM	       lParam2;
    LPARAM	       lParam3;
    LPARAM	       lParam4;
    LPARAM	       lParam5;
    DWORD	       FixRet;
    DWORD	       DrivesToChk;
    DWORD	       HstDrvsChckd;
    DWORD	       YldCnt;
    HWND	       hProgDlgWnd;
    HWND	       hWndPar; 	// If 0, am top level window
    HICON	       hIcon;
    UINT	       hTimer;
    BOOL	       ChkCancelBool;
    BOOL	       ChkInProgBool;
    BOOL	       SrfInProgBool;
    BOOL	       pTextIsComplt;
    BOOL	       ChkEngCancel;
    BOOL	       IsMultiDrv;
    BOOL	       AlrdyRestartWrn;
    BOOL	       ChkIsActive;
    BOOL	       NoRstrtWarn;
    BOOL	       IsSplitDrv;
    BOOL	       DoingCompDrv;
    BOOL	       DoSARestart;
    BOOL	       DoCHRestart;
    BOOL	       NoParms;
    UINT	       HostDrv;
    UINT	       CompDrv;
    WORD	       CurrOpRegion;
    WORD	       LastChkRslt;
    WORD	       MyFixOpt;
    WORD	       MyFixOpt2;
    BOOL	       IsDrvList;
    WORD	       LstChkdDrv;
    BYTE	       LstChkPcnt;
    BYTE	       RWRstsrtCnt;
    BYTE	       CompdriveNameStr[120];
    WORD	       iErr;
    BOOL	       IsFolder;
    BOOL	       IsRootFolder;
    BOOL	       UseAltDlgTxt;
    BOOL	       UseAltDefBut;
    BOOL	       CancelIsDefault;
    UINT	       AltDefButIndx;
    BOOL	       UseAltCantFix;
    UINT	       AltCantFixTstFlag;
    DWORD	       AltCantFixHID;
    DWORD	       AltCantFixRepHID;
    BOOL	       MultCantDelFiles;
    DWORD	       rgdwArgs[MAXSEARGS];
    DWORD	       MIrgdwArgs[MAXMIARGS];
    DWORD	       MErgdwArgs[MAXMULTSTRNGS][MAXSEARGS];
    WORD	       MltEStrings[MAXMULTSTRNGS];
#define szScratchMax 2048
    char	       szScratch[szScratchMax];
    DWORD	       aIds[20];
    LOG 	       Log;
} MYCHKINFOSTRUCT;
typedef MYCHKINFOSTRUCT*       PMYCHKINFOSTRUCT;
typedef MYCHKINFOSTRUCT NEAR* NPMYCHKINFOSTRUCT;
typedef MYCHKINFOSTRUCT FAR*  LPMYCHKINFOSTRUCT;

//
// The following define determines the number of OTHERWRT DU_ENGINERESTARTs
//   it takes before we do a IDS_LOCKRSTART warning
//
#define CHKLOCKRESTARTLIM	10L

// FORMAT.C
VOID InitDrvInfo(HWND hwnd, LPWINDISKDRIVEINFO lpwddi, LPDMAINTINFO lpDMaint);
BOOL _InitTermDMaint(BOOL bInit, LPDMAINTINFO lpDMaint);
int  DeviceParameters(int drive, LPDevPB lpDevPB, WORD wFunction);
BOOL GetVolumeLabel(UINT drive, LPSTR buff, UINT szbuf);

// SEDLG.C
BOOL_PTR WINAPI SEDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL_PTR WINAPI SEXLDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);_

// CHKDSK.C
WORD MyChkdskMessageBoxBuf(LPMYCHKINFOSTRUCT lpMyChkInf,
				LPSTR lpMsgBuf, WORD BoxStyle);
WORD MyChkdskMessageBox(LPMYCHKINFOSTRUCT lpMyChkInf,
			     WORD MsgID, WORD BoxStyle);

// Command ID's
//
#define DLGCONFOR_START		IDOK
#define DLGCONFOR_CANCEL	IDCANCEL
#define DLGCONFOR_FIRST		0x0020
#define DLGCONFOR_CAPCOMB	(DLGCONFOR_FIRST + 0x0000)
#define DLGCONFOR_FULL		(DLGCONFOR_FIRST + 0x0001)
#define DLGCONFOR_QUICK		(DLGCONFOR_FIRST + 0x0002)
#define DLGCONFOR_DOSYS		(DLGCONFOR_FIRST + 0x0003)
#define DLGCONFOR_PBAR		(DLGCONFOR_FIRST + 0x0004)
#define DLGCONFOR_STATTXT	(DLGCONFOR_FIRST + 0x0005)
#define DLGCONFOR_LABEL		(DLGCONFOR_FIRST + 0x0006)
#define DLGCONFOR_NOLAB		(DLGCONFOR_FIRST + 0x0007)
#define DLGCONFOR_MKSYS		(DLGCONFOR_FIRST + 0x0008)
#define DLGCONFOR_REPORT	(DLGCONFOR_FIRST + 0x0009)

#define DLGCONFOR_QCK		0x0001
#define DLGCONFOR_SYS		0x0002
#define DLGCONFOR_REP		0x0004
#define DLGCONFOR_NOV		0x0008
#define DLGCONFOR_SYSONLY	0x0010

#define DLGFORREP_CLOSE	IDCANCEL
#define DLGFORREP_FIRST	0x0020
#define DLGFORREP_TOT	(DLGFORREP_FIRST + 0x0000)
#define DLGFORREP_SYS	(DLGFORREP_FIRST + 0x0001)
#define DLGFORREP_BAD	(DLGFORREP_FIRST + 0x0002)
#define DLGFORREP_AVAIL	(DLGFORREP_FIRST + 0x0003)
#define DLGFORREP_BCLUS	(DLGFORREP_FIRST + 0x0004)
#define DLGFORREP_TCLUS	(DLGFORREP_FIRST + 0x0005)
#define DLGFORREP_SER	(DLGFORREP_FIRST + 0x0006)

#define LASTFMTRSLT_CAN	0xfffe
#define LASTFMTRSLT_ERR	0xffff

#define DLGCHKADV_OK		IDOK
#define DLGCHKADV_CANCEL	IDCANCEL
#define DLGCHKADV_FIRST		0x0100
#define DLGCHKADV_LSTF		(DLGCHKADV_FIRST + 0x0000)
#define DLGCHKADV_LSTMF 	(DLGCHKADV_FIRST + 0x0001)
#define DLGCHKADV_XLDEL 	(DLGCHKADV_FIRST + 0x0002)
#define DLGCHKADV_XLCPY 	(DLGCHKADV_FIRST + 0x0003)
#define DLGCHKADV_XLIGN 	(DLGCHKADV_FIRST + 0x0004)
#define DLGCHKADV_CHKDT 	(DLGCHKADV_FIRST + 0x0005)
#define DLGCHKADV_CHKNM 	(DLGCHKADV_FIRST + 0x0006)
#define DLGCHKADV_CHKHST	(DLGCHKADV_FIRST + 0x0007)
#define DLGCHKADV_REPALWAYS	(DLGCHKADV_FIRST + 0x0008)
#define DLGCHKADV_NOREP 	(DLGCHKADV_FIRST + 0x0009)
#define DLGCHKADV_REPIFERR	(DLGCHKADV_FIRST + 0x000a)
#define DLGCHKADV_LOGREP	(DLGCHKADV_FIRST + 0x000b)
#define DLGCHKADV_LOGAPPND	(DLGCHKADV_FIRST + 0x000c)
#define DLGCHKADV_NOLOG 	(DLGCHKADV_FIRST + 0x000d)

#define DLGCHKSAO_OK		IDOK
#define DLGCHKSAO_CANCEL	IDCANCEL
#define DLGCHKSAO_FIRST 	0x0120
#define DLGCHKSAO_NOWRTTST	(DLGCHKSAO_FIRST + 0x0000)
#define DLGCHKSAO_ALLHIDSYS	(DLGCHKSAO_FIRST + 0x0001)
#define DLGCHKSAO_DOALL 	(DLGCHKSAO_FIRST + 0x0002)
#define DLGCHKSAO_NOSYS 	(DLGCHKSAO_FIRST + 0x0003)
#define DLGCHKSAO_NODATA	(DLGCHKSAO_FIRST + 0x0004)

#define DLGCHKREP_CLOSE		IDCANCEL
#define DLGCHKREP_FIRST 	0x0140
#define DLGCHKREP_TOT		(DLGCHKREP_FIRST + 0x0000)
#define DLGCHKREP_AVAIL		(DLGCHKREP_FIRST + 0x0001)
#define DLGCHKREP_BAD		(DLGCHKREP_FIRST + 0x0002)
#define DLGCHKREP_BCLUS		(DLGCHKREP_FIRST + 0x0003)
#define DLGCHKREP_TCLUS		(DLGCHKREP_FIRST + 0x0004)
#define DLGCHKREP_ACLUS		(DLGCHKREP_FIRST + 0x0005)
#define DLGCHKREP_DIR		(DLGCHKREP_FIRST + 0x0006)
#define DLGCHKREP_USER		(DLGCHKREP_FIRST + 0x0007)
#define DLGCHKREP_HID		(DLGCHKREP_FIRST + 0x0008)
#define DLGCHKREP_ESTAT 	(DLGCHKREP_FIRST + 0x0009)

#define DLGCHK_START		IDOK
#define DLGCHK_CANCEL		IDCANCEL
#define DLGCHK_FIRST		0x0160
#define DLGCHK_PBAR		(DLGCHK_FIRST + 0x0000)
#define DLGCHK_STATTXT		(DLGCHK_FIRST + 0x0001)
#define DLGCHK_ADVANCED 	(DLGCHK_FIRST + 0x0002)
#define DLGCHK_DRVLIST		(DLGCHK_FIRST + 0x0003)
#define DLGCHK_DOBAD		(DLGCHK_FIRST + 0x0004)
#define DLGCHK_BADOPT		(DLGCHK_FIRST + 0x0005)
#define DLGCHK_AUTOFIX		(DLGCHK_FIRST + 0x0006)
#define DLGCHK_NOBADB		(DLGCHK_FIRST + 0x0007)
#define DLGCHK_DTXT1		(DLGCHK_FIRST + 0x0008)
#define DLGCHK_DTXT2		(DLGCHK_FIRST + 0x0009)
#define DLGCHK_CHELP		(DLGCHK_FIRST + 0x000a)

//
// These are bit defines for a WORD (MyFixOpt) in DLG_CHKDSK that correspond
// to the check boxes in DLG_CHKDSK and DLG_CHKDSKADVOPT
//
#define DLGCHK_REP		0x0001
#define DLGCHK_RO		0x0002	 // No check box for this...
#define DLGCHK_NOSYS		0x0004
#define DLGCHK_NODATA		0x0008
#define DLGCHK_NOBAD		0x0010
#define DLGCHK_LSTMF		0x0020
#define DLGCHK_NOCHKNM		0x0040
#define DLGCHK_NOCHKDT		0x0080
#define DLGCHK_INTER		0x0100
#define DLGCHK_XLCPY		0x0200
#define DLGCHK_XLDEL		0x0400
#define DLGCHK_ALLHIDSYS	0x0800
#define DLGCHK_NOWRTTST		0x1000
#define DLGCHK_DEFOPTIONS	0x2000	 // No check box for this...
#define DLGCHK_DRVLISTONLY	0x4000	 // No check box for this...
#define DLGCHK_AUTO		0x8000	 // No check box for this...
// These are bit defines for WORD (MyFixOpt2)
#define DLGCHK_EXCLU		0x0001	 // No check box for this...
#define DLGCHK_FLWRT		0x0002	 // No check box for this...
#define DLGCHK_MKOLDFS		0x0004	 // No check box for this...
#define DLGCHK_PROGONLY 	0x0008	 // No check box for this...
#define DLGCHK_NOWND		0x0010	 // No check box for this...
#define DLGCHK_NOCHKHST 	0x0020
#define DLGCHK_REPONLYERR	0x0040
#define DLGCHK_NOLOG		0x0080
#define DLGCHK_LOGAPPEND	0x0100

// Defines for LastChkRslt special values
#define LASTCHKRSLT_SMNOTFIX 0xFFFD
#define LASTCHKRSLT_CAN 0xFFFE
#define LASTCHKRSLT_ERR 0xFFFF


// Resource ID's
//

#define DLG_FORMAT_FIRST	400
#define IDS_FORMAT_FIRST	0x2800

#define IDS_DRIVETITLEF (IDS_FORMAT_FIRST + 0x0000)
#define IDS_WRTPROT	(IDS_FORMAT_FIRST + 0x0001)
#define IDS_NOTRDY	(IDS_FORMAT_FIRST + 0x0002)
#define IDS_INVFMT	(IDS_FORMAT_FIRST + 0x0003)
#define IDS_INVFMTREC	(IDS_FORMAT_FIRST + 0x0004)
#define IDS_NOQCKREC	(IDS_FORMAT_FIRST + 0x0005)
#define IDS_NOQCK	(IDS_FORMAT_FIRST + 0x0006)
#define IDS_BADVOLREC	(IDS_FORMAT_FIRST + 0x0007)
#define IDS_BADVOL	(IDS_FORMAT_FIRST + 0x0008)
#define IDS_FATERRSYS	(IDS_FORMAT_FIRST + 0x0009)
#define IDS_FATERR	(IDS_FORMAT_FIRST + 0x000a)
#define IDS_BOOTERRSYS	(IDS_FORMAT_FIRST + 0x000b)
#define IDS_BOOTERR	(IDS_FORMAT_FIRST + 0x000c)
#define IDS_ROOTDERR	(IDS_FORMAT_FIRST + 0x000d)
#define IDS_OSAREC	(IDS_FORMAT_FIRST + 0x000e)
#define IDS_OSA 	(IDS_FORMAT_FIRST + 0x000f)
#define IDS_DATAERRREC	(IDS_FORMAT_FIRST + 0x0010)
#define IDS_DATAERR	(IDS_FORMAT_FIRST + 0x0011)
#define IDS_TOSREC	(IDS_FORMAT_FIRST + 0x0012)
#define IDS_TOS 	(IDS_FORMAT_FIRST + 0x0013)
#define IDS_FULLDISK	(IDS_FORMAT_FIRST + 0x0014)
#define IDS_NOSYSFILES	(IDS_FORMAT_FIRST + 0x0015)
#define IDS_NOSREC	(IDS_FORMAT_FIRST + 0x0016)
#define IDS_NOS 	(IDS_FORMAT_FIRST + 0x0017)
#define IDS_UNKERRSYS	(IDS_FORMAT_FIRST + 0x0018)
#define IDS_UNKERR	(IDS_FORMAT_FIRST + 0x0019)
#define IDS_F_INIT	(IDS_FORMAT_FIRST + 0x001a)
#define IDS_CANCEL	(IDS_FORMAT_FIRST + 0x001b)
#define IDS_F_LOWFMT	(IDS_FORMAT_FIRST + 0x001c)
#define IDS_F_VERIFY	(IDS_FORMAT_FIRST + 0x001d)
#define IDS_F_FSFMT	(IDS_FORMAT_FIRST + 0x001e)
#define IDS_F_TSYS	(IDS_FORMAT_FIRST + 0x001f)
#define IDS_F_GETLABEL	(IDS_FORMAT_FIRST + 0x0020)
#define IDS_F_SHTDOWN	(IDS_FORMAT_FIRST + 0x0021)
#define IDS_FORMAT_NAME (IDS_FORMAT_FIRST + 0x0023)
#define IDS_NOFORMATSYS (IDS_FORMAT_FIRST + 0x0024)
#define IDS_GENDISKPROB (IDS_FORMAT_FIRST + 0x0025)
#define IDS_FCONFIRM	(IDS_FORMAT_FIRST + 0x0026)
#define IDS_NOFORMATSYSW (IDS_FORMAT_FIRST + 0x0027)
#define IDS_NOFORMATSYSP (IDS_FORMAT_FIRST + 0x0028)
#define IDS_NOFORMATSYSH (IDS_FORMAT_FIRST + 0x0029)
#define IDS_GENDISKPROBA (IDS_FORMAT_FIRST + 0x002a)
#define IDS_GENDISKPROBI (IDS_FORMAT_FIRST + 0x002b)
#define IDS_GENDISKPROBC (IDS_FORMAT_FIRST + 0x002c)
#define IDS_GENDISKPROBL (IDS_FORMAT_FIRST + 0x002d)
#define IDS_FDOSURF	(IDS_FORMAT_FIRST + 0x002e)
#define IDS_NOMEMF	(IDS_FORMAT_FIRST + 0x002f)
#define IDS_FBADOPT	(IDS_FORMAT_FIRST + 0x0030)
#define IDS_FNOTSUP	(IDS_FORMAT_FIRST + 0x0031)
#define IDS_FLOCK	(IDS_FORMAT_FIRST + 0x0032)
#define IDS_DRIVETITLEC (IDS_FORMAT_FIRST + 0x0033)
#define IDS_BOOT	(IDS_FORMAT_FIRST + 0x0036)
#define IDS_FAT 	(IDS_FORMAT_FIRST + 0x0037)
#define IDS_ROOTD	(IDS_FORMAT_FIRST + 0x0038)
#define IDS_DIR 	(IDS_FORMAT_FIRST + 0x003e)
#define IDS_C_INIT	IDS_F_INIT
#define IDS_C_FAT	(IDS_FORMAT_FIRST + 0x003f)
#define IDS_C_DIR	(IDS_FORMAT_FIRST + 0x0040)
#define IDS_C_FILEDIR	(IDS_FORMAT_FIRST + 0x0041)
#define IDS_C_LOSTCLUS	(IDS_FORMAT_FIRST + 0x0042)
#define IDS_B_INIT	(IDS_FORMAT_FIRST + 0x0043)
#define IDS_B_UNMOV	(IDS_FORMAT_FIRST + 0x0044)
#define IDS_B_SYS	(IDS_FORMAT_FIRST + 0x0045)
#define IDS_B_DATA	(IDS_FORMAT_FIRST + 0x0046)
#define IDS_C_SHTDOWN	IDS_F_SHTDOWN
#define IDS_CANTCHK	(IDS_FORMAT_FIRST + 0x004f)
#define IDS_CANTCHKALL	(IDS_FORMAT_FIRST + 0x0050)
#define IDS_NOSEL	(IDS_FORMAT_FIRST + 0x0051)
#define IDS_NOMEM2	(IDS_FORMAT_FIRST + 0x0052)
#define IDS_CHKTIT	(IDS_FORMAT_FIRST + 0x0053)
#define IDS_FILEM	(IDS_FORMAT_FIRST + 0x0055)
#define IDS_CHKTIT2	(IDS_FORMAT_FIRST + 0x0056)
#define IDS_CHKTIT3	(IDS_FORMAT_FIRST + 0x0057)
#define IDS_DD_HEAD	(IDS_FORMAT_FIRST + 0x0059)
#define IDS_DD_STRUC	(IDS_FORMAT_FIRST + 0x005a)
#define IDS_DD_FAT	(IDS_FORMAT_FIRST + 0x005b)
#define IDS_DD_SIG	(IDS_FORMAT_FIRST + 0x005c)
#define IDS_DD_BOOT	(IDS_FORMAT_FIRST + 0x005d)
#define IDS_XLNOFIL	(IDS_FORMAT_FIRST + 0x005e)
#define IDS_NOERROR	(IDS_FORMAT_FIRST + 0x005f)
#define IDS_ALLFIXED	(IDS_FORMAT_FIRST + 0x0060)
#define IDS_SOMEFIXED	(IDS_FORMAT_FIRST + 0x0061)
#define IDS_NONEFIXED	(IDS_FORMAT_FIRST + 0x0062)
#define IDS_LOCKRSTART	(IDS_FORMAT_FIRST + 0x0063)
#define IDS_UNEXP3	(IDS_FORMAT_FIRST + 0x0064)
#define IDS_UNSUP	(IDS_FORMAT_FIRST + 0x0065)
#define IDS_CANTWRT	(IDS_FORMAT_FIRST + 0x0066)
#define IDS_SERDISK	(IDS_FORMAT_FIRST + 0x0067)
#define IDS_SERFS	(IDS_FORMAT_FIRST + 0x0068)
#define IDS_ACTIVE	(IDS_FORMAT_FIRST + 0x0069)
#define IDS_LOCKVIOL	(IDS_FORMAT_FIRST + 0x006a)
#define IDS_NOMEM3	(IDS_FORMAT_FIRST + 0x006b)
#define IDS_CANTDELROOT (IDS_FORMAT_FIRST + 0x006c)
#define IDS_LOCKVIOL2	(IDS_FORMAT_FIRST + 0x006d)
#define IDS_INVALID	(IDS_FORMAT_FIRST + 0x006f)
#define IDS_CANTCHKR	(IDS_FORMAT_FIRST + 0x0070)
#define IDS_COMPDISKH	(IDS_FORMAT_FIRST + 0x0071)
#define IDS_UNSUPR	(IDS_FORMAT_FIRST + 0x0072)
#define IDS_NODSKMNT	(IDS_FORMAT_FIRST + 0x0073)
#define IDS_CXL_DIRDEL	(IDS_FORMAT_FIRST + 0x0074)
#define IDS_CXL_DIRTIT	(IDS_FORMAT_FIRST + 0x0075)
#define IDS_CANTDELF	(IDS_FORMAT_FIRST + 0x0076)
#define IDS_CANTDELD	(IDS_FORMAT_FIRST + 0x0077)
#define IDS_DIRDEL	(IDS_FORMAT_FIRST + 0x0078)
#define IDS_FILEMS	(IDS_FORMAT_FIRST + 0x0079)
#define IDS_DIRS	(IDS_FORMAT_FIRST + 0x007a)
#define IDS_CANTFIX	(IDS_FORMAT_FIRST + 0x007b)
#define IDS_DBLSPACE	(IDS_FORMAT_FIRST + 0x007c)
#define IDS_DRVSPACE	(IDS_FORMAT_FIRST + 0x007d)
#define IDS_ERRRSTART	(IDS_FORMAT_FIRST + 0x007e)
#define IDS_C_COMPLETE	(IDS_FORMAT_FIRST + 0x007f)

#define DLG_FORMATREPORT	400
#define DLG_FORMAT		401

#define DLG_CHKDSKSAOPT 	408
#define DLG_CHKDSKADVOPT	409
#define DLG_CHKDSKREPORT	410
#define DLG_CHKDSK		411
#define DLG_CHKDSKTL		412

#define IDI_CHKICON		430
#define IDI_CHKDLGICON		431

// SE--Simple Error Four checkboxen
#define ID_SE_FIRST	0x1000
#define IDD_SE_DLG	0x1000

#define IDC_SE_TXT	0x1001
#define IDC_SE_BUT1	0x1002
#define IDC_SE_BUT2	0x1003
#define IDC_SE_BUT3	0x1004
#define IDC_SE_BUT4	0x1005
#define IDC_SE_CBUTTONS 0x04
#define IDC_SE_OK	0x1006
#define IDC_SE_CANCEL	IDCANCEL	// Needs to be this so close cookie
					//	in title bar works
#define IDC_SE_CANCELA	0x1007
#define IDC_SE_MOREINFO 0x1008
#define IDC_SE_HELP	0x1009

#define IDD_XL_DLG	0x1100
#define IDC_XL_LIST	0x1101
#define IDC_XL_TXT1	0x1102
#define IDC_XL_TXT2	0x1103
#define IDC_XL_BUT1	0x1104	// 6 buttons starting here

#define IDD_XL_CANT	0x1200
#define IDC_XL_CTXT	0x1201
#define IDC_GROUPBOX_1	0x1202
#define IDC_GROUPBOX_2	0x1203
#define IDC_GROUPBOX_3	0x1204
#define IDC_GROUPBOX_4	0x1205
#define IDC_GROUPBOX_5	0x1206
#define IDC_ICON_1	0x1207
#define IDC_TEXT	0x1208
#define IDC_TEXT_2	0x1209

#define TITLE(n)     (n)
#define IDC_SE_CTXT 0x04
#define DLTXT1(n)    (n+0x01)
#define DLTXT2(n)    (n+0x02)
#define DLTXT3(n)    (n+0x03)
#define DLTXT4(n)    (n+0x04)
#define BUTT1(n)     (n+0x05)
#define BUTT2(n)     (n+0x06)
#define BUTT3(n)     (n+0x07)
#define BUTT4(n)     (n+0x08)
#define IDC_SE_CMORT 0x04
#define MORETXT1(n)  (n+0x09)
#define MORETXT2(n)  (n+0x0a)
#define MORETXT3(n)  (n+0x0b)
#define MORETXT4(n)  (n+0x0c)
#define MORETIT(n)   (n+0x0d)
#define ALTDLTXT1(n) (n+0x0e)
#define ALTDLTXT2(n) (n+0x0f)
#define ALTDLTXT3(n) (n+0x10)
#define ALTDLTXT4(n) (n+0x11)
#define ALTBUTT1(n)  (n+0x12)
#define ALTBUTT2(n)  (n+0x13)
#define ALTBUTT3(n)  (n+0x14)
#define ALTBUTT4(n)  (n+0x15)

#define diSpec (8)

#define IERR_FATERRDIR		0
#define ISTR_FATERRDIR		0x3000
#define ISTR_FATERRDIR_DOTS	0x3030
#define ISTR_FATERRDIR_LFNLST	0x3031
#define ISTR_FATERRDIR_ZRLEN	0x3032
#define ISTR_FATERRDIR_ZRLENC	0x3033
#define ISTR_FATERRDIR_LOSTFIL	0x3034
#define ISTR_FATERRDIR_DUPNM	0x3035
#define ISTR_FATERRDIR_DUPNMC	0x3036
#define ISTR_FATERRDIR_BAD	0x3037
#define ISTR_FATERRDIR_BADC	0x3038
#define ISTR_FATERRDIR_PNOTD	0x3039
#define ISTR_FATERRDIR_PNOTDC	0x303a

#define IERR_FATLSTCLUS 	1
#define ISTR_FATLSTCLUS 	0x3080

#define IERR_FATCIRCC		2
#define ISTR_FATCIRCC		0x3100

#define IERR_FATINVCLUS 	3
#define ISTR_FATINVCLUS 	0x3180

#define IERR_FATRESVAL		4
#define ISTR_FATRESVAL		0x3200

#define IERR_FATFMISMAT 	5
#define ISTR_FATFMISMAT 	0x3280

#define IERR_FATERRFILE 	6
#define ISTR_FATERRFILE 	0x3300
#define ISTR_FATERRFILE_INVLFN1 0x3330
#define ISTR_FATERRFILE_INVLFN2 0x3331
#define ISTR_FATERRFILE_INVNM1	0x3332
#define ISTR_FATERRFILE_INVNM2	0x3333
#define ISTR_FATERRFILE_LFNSTR1 0x3334
#define ISTR_FATERRFILE_LFNSTR2 0x3335
#define ISTR_FATERRFILE_LFNLEN	0x3336
#define ISTR_FATERRFILE_DEVNM1	0x3337
#define ISTR_FATERRFILE_DEVNM2	0x3338
#define ISTR_FATERRFILE_SIZE1	0x3339
#define ISTR_FATERRFILE_SIZE2	0x333a
#define ISTR_FATERRFILE_SIZED1	0x333b
#define ISTR_FATERRFILE_SIZED2	0x333c
#define ISTR_FATERRFILE_DTTM11	0x333d
#define ISTR_FATERRFILE_DTTM12	0x333e
#define ISTR_FATERRFILE_DTTM21	0x333f
#define ISTR_FATERRFILE_DTTM22	0x3340
#define ISTR_FATERRFILE_DTTM31	0x3341
#define ISTR_FATERRFILE_DTTM32	0x3342

#define IERR_FATERRVOLLAB	7
#define ISTR_FATERRVOLLAB	0x3380

#define IERR_FATERRMXPLENL	8
#define ISTR_FATERRMXPLENL	0x3400

#define IERR_FATERRMXPLENS	9
#define ISTR_FATERRMXPLENS	0x3480

#define IERR_FATERRCDLIMIT	10
#define ISTR_FATERRCDLIMIT	0x3500

#define IERR_DDERRSIZE1 	11
#define ISTR_DDERRSIZE1 	0x3580

#define IERR_DDERRFRAG		12
#define ISTR_DDERRFRAG		0x3600

#define IERR_DDERRALIGN 	13
#define ISTR_DDERRALIGN 	0x3680

#define IERR_DDERRNOXLCHK	14
#define ISTR_DDERRNOXLCHK	0x3700

#define IERR_DDERRUNSUP 	15
#define ISTR_DDERRUNSUP 	0x3780

#define IERR_DDERRCVFNM 	16
#define ISTR_DDERRCVFNM 	0x3800

#define IERR_DDERRSIG		17
#define ISTR_DDERRSIG		0x3880

#define IERR_DDERRBOOT		18
#define ISTR_DDERRBOOT		0x3900

#define IERR_DDERRMDBPB 	19
#define ISTR_DDERRMDBPB 	0x3980

#define IERR_DDERRSIZE2A	20
#define ISTR_DDERRSIZE2A	0x3a00

#define IERR_DDERRSIZE2B	21
#define ISTR_DDERRSIZE2B	0x3a80

#define IERR_DDERRMDFAT 	22
#define ISTR_DDERRMDFAT 	0x3b00

#define IERR_DDERRLSTSQZ	23
#define ISTR_DDERRLSTSQZ	0x3b80

#define IERR_ERRISNTBAD 	24
#define ISTR_ERRISNTBAD 	0x3c00

#define RESTARTWITHSA		0xFFFF
#define RESTARTWITHCH		0xFFFE

#define IERR_ERRISBAD1		25
#define ISTR_ERRISBAD1		0x3c80

#define IERR_ERRISBAD2		26
#define ISTR_ERRISBAD2		0x3d00

#define IERR_ERRISBAD3		27
#define ISTR_ERRISBAD3		0x3d80

#define IERR_ERRISBAD4		28
#define ISTR_ERRISBAD4		0x3e00

#define IERR_ERRISBAD5		29
#define ISTR_ERRISBAD5		0x3e80

#define IERR_ERRISBAD6		30
#define ISTR_ERRISBAD6		0x3f00

#define IERR_ERRMEM		31
#define ISTR_ERRMEM		0x3f80

#define IERR_ERRCANTDEL 	32
#define ISTR_ERRCANTDEL 	0x4000

#define IERR_DDERRMOUNT 	33
#define ISTR_DDERRMOUNT 	0x4080

#define IERR_READERR1		34
#define ISTR_READERR1		0x4100

#define IERR_READERR2		35
#define ISTR_READERR2		0x4180

#define IERR_READERR3		36
#define ISTR_READERR3		0x4200

#define IERR_READERR4		37
#define ISTR_READERR4		0x4280

#define IERR_READERR5		38
#define ISTR_READERR5		0x4300

#define IERR_READERR6		39
#define ISTR_READERR6		0x4380

#define IERR_WRITEERR1		40
#define ISTR_WRITEERR1		0x4400

#define IERR_WRITEERR2		41
#define ISTR_WRITEERR2		0x4480

#define IERR_WRITEERR3		42
#define ISTR_WRITEERR3		0x4500

#define IERR_WRITEERR4		43
#define ISTR_WRITEERR4		0x4580

#define IERR_WRITEERR5		44
#define ISTR_WRITEERR5		0x4600

#define IERR_WRITEERR6		45
#define ISTR_WRITEERR6		0x4680

#define IERR_ECORRDISK		46
#define ISTR_ECORRDISK		0x4700

#define IERR_ECORRMEM		47
#define ISTR_ECORRMEM		0x4780

#define IERR_ECORRFILCOL	48
#define ISTR_ECORRFILCOL	0x4800

#define IERR_ECORRUNEXP 	49
#define ISTR_ECORRUNEXP 	0x4880

#define IERR_ECORRCLUSA 	50
#define ISTR_ECORRCLUSA 	0x4900

#define IERR_ECORRFILCRT	51
#define ISTR_ECORRFILCRT	0x4980

#define IERR_ECORROTHWRT	52
#define ISTR_ECORROTHWRT	0x4a00

#define ISTR_SE_IGNORE          0x5800
#define ISTR_SE_DIRDEL          0x5801
#define ISTR_XL_TITLE           0x5802
#define ISTR_SEADDLOGNOMEM      0x5803
#define ISTR_SE_REPAIR          0x5804
#define ISTR_SE_FILDEL          0x5805
#define ISTR_SE_FILTRNC         0x5806
#define ISTR_SE_DIRTRNC         0x5807
#define ISTR_SE_CONTB           0x5808
#define ISTR_XL_ALTTXT1         0x5809
#define ISTR_XL_ALTTXT2A        0x580a
#define ISTR_XL_ALTTXT2B        0x580b
#define ISTR_XL_ALTBUTTREP	ISTR_SE_REPAIR
#define ISTR_XL_ALTBUTTDEL      0x580c
