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

// Defines for GetVolumeInfo FileSysFlags return
#define FS_CASE_IS_PRESERVED		0x0002
#define FS_VOL_IS_COMPRESSED		0x8000
#define FS_VOL_SUPPORTS_LONG_NAMES	0x4000

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
} DMAINTINFO, FAR *LPDMAINTINFO;

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
} BPB, NEAR * PBPB, FAR * LPBPB;

#ifdef OPK2
typedef struct	BIGFATBPB { /* */
	BPB	oldBPB;
	WORD	BGBPB_BigSectorsPerFat;   /* BigFAT Fat sectors 	     */
	WORD	BGBPB_BigSectorsPerFatHi; /* High word of BigFAT Fat sectrs  */
	WORD	BGBPB_ExtFlags; 	  /* Other flags		     */
	WORD	BGBPB_FS_Version;	  /* File system version	     */
	WORD	BGBPB_RootDirStrtClus;	  /* Starting cluster of root directory */
	WORD	BGBPB_RootDirStrtClusHi;
	WORD	BGBPB_FSInfoSec;	  /* Sector number in the reserved   */
					  /* area where the BIGFATBOOTFSINFO */
					  /* structure is. If this is >=     */
					  /* oldBPB.BPB_ReservedSectors or   */
					  /* == 0 there is no FSInfoSec      */
	WORD	BGBPB_BkUpBootSec;	  /* Sector number in the reserved   */
					  /* area where there is a backup    */
					  /* copy of all of the boot sectors */
					  /* If this is >=		     */
					  /* oldBPB.BPB_ReservedSectors or   */
					  /* == 0 there is no backup copy.   */
	WORD	BGBPB_Reserved[6];	  /* Reserved for future expansion   */
} BIGFATBPB, *PBIGFATBPB;
#endif

/*--------------------------------------------------------------------------*/
/*  Device Parameter Block Structure -					    */
/*--------------------------------------------------------------------------*/

#define MAX_SEC_PER_TRACK	64

#ifdef OPK2
typedef struct tagODEVPB {
    BYTE    SplFunctions;
    BYTE    devType;
    WORD    devAtt;
    WORD    NumCyls;
    BYTE    bMediaType;  /* 0=>1.2MB and 1=>360KB */
    BPB     BPB;
    BYTE    A_BPB_Reserved[6];			 // Unused 6 BPB bytes
    BYTE    TrackLayout[MAX_SEC_PER_TRACK * 4 + 2];
} ODEVPB;

#define OLDTRACKLAYOUT_OFFSET	(7+sizeof(BPB)+6)  // Offset of tracklayout

typedef struct tagEDEVPB {
    BYTE      SplFunctions;
    BYTE      devType;
    WORD      devAtt;
    WORD      NumCyls;
    BYTE      bMediaType;  /* 0=>1.2MB and 1=>360KB */
    BIGFATBPB BPB;
    BYTE      reserved1[32];
    BYTE      TrackLayout[MAX_SEC_PER_TRACK * 4 + 2];
} EDEVPB;

#define EXTTRACKLAYOUT_OFFSET	   (7+sizeof(BIGFATBPB)+32)  // Offset of tracklayout
							     // in a Device Parameter Block */
//
// NOTE that it is pretty irrelevant whether you use NonExtDevPB or ExtDevPB
//	until you get up to the new FAT32 fields in the BPB and/or the
//	TrackLayout area as the structures are identical andinterchangeable
//	up to that point.
//
typedef struct tagDEVPB {
	union	{
		    ODEVPB NonExtDevPB;
		    EDEVPB ExtDevPB;
		} DevPrm;
	BYTE	CallForm;
} DevPB, NEAR *PDevPB, FAR *LPDevPB;
#else
typedef struct tagDevPB {
    BYTE    SplFunctions;
    BYTE    devType;
    WORD    devAtt;
    WORD    NumCyls;
    BYTE    bMediaType;  /* 0=>1.2MB and 1=>360KB */
    BPB     BPB;
    BYTE    A_BPB_Reserved[6];			 // Unused 6 BPB bytes
    BYTE    TrackLayout[MAX_SEC_PER_TRACK * 4 + 2];
} DevPB, NEAR *PDevPB, FAR *LPDevPB;
#endif

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
typedef WINDISKDRIVEINFO FAR  * LPWINDISKDRIVEINFO;

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

#define SCANDISKLOGFILENAME "C:\\SCANDISK.LOG"

typedef struct Log_
{
	HANDLE		   hSz;
	DWORD		   cchUsed; // Does not include the terminating \0
	DWORD		   cchAlloced;
	BOOL		   fMemWarned;
	BYTE		   LogFileName[20];
} LOG, FAR *LPLOG;

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
    HWND FAR *	       lpTLhwnd;
    DWORD	       OpCmpltRet;
    DWORD	       FixOptions;
    DWORD	       RegOptions;
    DWORD	       NoUnsupDrvs;
    DWORD	       SALstPos;
    LPARAM	       lParam1;
    LPARAM	       lParam2;
    LPARAM	       lParam3;
    LPARAM	       lParam4;
    LPARAM	       lParam5;
    DWORD	       FixRet;
    DWORD	       DrivesToChk;
    DWORD	       HstDrvsChckd;
    DWORD	       HiddenDrives;
    DWORD	       YldCnt;
    HWND	       hProgDlgWnd;
    HWND	       hWndPar; 	// If 0, am top level window
    HICON	       hIcon;
#ifdef OPK2
    HICON	       hCurrAniIcon;
    HICON	       hAniIcon1;
    HICON	       hAniIcon2;
    HICON	       hAniIcon3;
    BOOL	       Done3PtyCompWrn;
#endif
    UINT	       hTimer;
    BOOL	       ChkCancelBool;
    BOOL	       ChkInProgBool;
    BOOL	       SrfInProgBool;
    BOOL	       pTextIsComplt;
    BOOL	       ChkEngCancel;
    BOOL	       IsMultiDrv;
    BOOL	       IsFirstDrv;
    BOOL	       AlrdyRestartWrn;
    BOOL	       ChkIsActive;
    BOOL	       NoRstrtWarn;
    BOOL	       IsSplitDrv;
    BOOL	       DoingCompDrv;
    BOOL	       DoSARestart;
    BOOL	       DoCHRestart;
    BOOL	       NoParms;
    BOOL	       ShowMinimized;
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
    WORD	       MltELogStrings[MAXMULTSTRNGS];
#define szScratchMax 4096
    char	       szScratch[szScratchMax];
    DWORD	       aIds[20];
    LOG 	       Log;
#ifdef FROSTING
    DWORD              idxSettings;
    BOOL               fSageSet;
    BOOL               fSageRun;
    BOOL               fShouldRerun;
    WORD	       MultLastChkRslt;
    DWORD              SilentProbCnt;
#endif
} MYCHKINFOSTRUCT;
typedef MYCHKINFOSTRUCT*       PMYCHKINFOSTRUCT;
typedef MYCHKINFOSTRUCT NEAR* NPMYCHKINFOSTRUCT;
typedef MYCHKINFOSTRUCT FAR*  LPMYCHKINFOSTRUCT;

//
// The following define determines the number of OTHERWRT DU_ENGINERESTARTs
//   it takes before we do a IDS_LOCKRSTART warning
//
#define CHKLOCKRESTARTLIM	10L
#ifdef FROSTING
#define CHKLOCKRESTARTLIM_SAGE	100L
#endif

#ifdef FROSTING
//
// The following defines a flag which is set in the DrivesToChk field, which
//   indicates that all non-removable drives should be checked, in addition to
//   any removable drives which are indicated to be checked in the remaining
//   bits in the DrivesToChk field.
//
#define DTC_ALLFIXEDDRIVES  0x80000000L
#endif

// FORMAT.C
VOID FAR  PASCAL InitDrvInfo(HWND hwnd, LPWINDISKDRIVEINFO lpwddi, LPDMAINTINFO lpDMaint);
BOOL FAR  PASCAL _InitTermDMaint(BOOL bInit, LPDMAINTINFO lpDMaint);
int  NEAR PASCAL DeviceParameters(int drive, LPDevPB lpDevPB, WORD wFunction);
BOOL NEAR PASCAL GetVolumeLabel(UINT drive, LPSTR buff, UINT szbuf);

// SEDLG.C
BOOL WINAPI SEDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI SEXLDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define LOGCHUNK 1024L

VOID FAR   SEInitLog(LPMYCHKINFOSTRUCT lpMyChkInf);
VOID FAR   SEAddToLogStart(LPMYCHKINFOSTRUCT lpMyChkInf, BOOL FullHeader);
VOID FAR   SEAddToLogRCS(LPMYCHKINFOSTRUCT lpMyChkInf, UINT RCId, UINT PostRCId);
VOID FAR   SEAddToLog(LPMYCHKINFOSTRUCT lpMyChkInf, LPSTR szNew, LPSTR szPost);
VOID FAR   SERecordLog(LPMYCHKINFOSTRUCT lpMyChkInf,BOOL MltiDrvAppnd);
VOID FAR   SEAddErrToLog(LPMYCHKINFOSTRUCT lpMyChkInf);
VOID FAR   SEAddRetToLog(LPMYCHKINFOSTRUCT lpMyChkInf);


// CHKDSK.C
WORD FAR  MyChkdskMessageBoxBuf(LPMYCHKINFOSTRUCT lpMyChkInf,
				LPSTR lpMsgBuf, WORD BoxStyle);
WORD FAR  MyChkdskMessageBox(LPMYCHKINFOSTRUCT lpMyChkInf,
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
#define DLGCHK_ABOUT		(DLGCHK_FIRST + 0x000b)
#define DLGCHK_STATTXT2 	(DLGCHK_FIRST + 0x000c)

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
#define DLGCHK_MINIMIZED	0x0200	 // No check box for this...
#define DLGCHK_DOBADISNTBAD	0x0400	 // Currently No check box for this...

// Defines for LastChkRslt special values
#define LASTCHKRSLT_NOERROR  0x0000
#define LASTCHKRSLT_ALLFIXED 0x0001
#define LASTCHKRSLT_SMNOTFIX 0xFFFD
#define LASTCHKRSLT_CAN      0xFFFE
#define LASTCHKRSLT_ERR      0xFFFF


// Resource ID's
//
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
#define IDS_NAME	(IDS_FORMAT_FIRST + 0x0023)
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
#define IDS_B_DATANUM	(IDS_FORMAT_FIRST + 0x0047)
#define IDS_C_SHTDOWN	IDS_F_SHTDOWN
#ifdef OPK2
#define IDS_C_BOOT	(IDS_FORMAT_FIRST + 0x0048)
#define IDS_B_SYSNUM	(IDS_FORMAT_FIRST + 0x0049)
#endif
#define IDS_CANTCHK	(IDS_FORMAT_FIRST + 0x004f)
#define IDS_CANTCHKALL	(IDS_FORMAT_FIRST + 0x0050)
#define IDS_NOSEL	(IDS_FORMAT_FIRST + 0x0051)
#define IDS_NOMEM2	(IDS_FORMAT_FIRST + 0x0052)
#define IDS_CHKTIT	(IDS_FORMAT_FIRST + 0x0053)
#define IDS_FILEM	(IDS_FORMAT_FIRST + 0x0055)
#define IDS_CHKTIT2	(IDS_FORMAT_FIRST + 0x0056)
#define IDS_CHKTIT3	(IDS_FORMAT_FIRST + 0x0057)
#define IDS_CLOSEM	(IDS_FORMAT_FIRST + 0x0058)
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
#define IDS_NOQCKRECREM (IDS_FORMAT_FIRST + 0x0080)
#define IDS_NOQCKREM	(IDS_FORMAT_FIRST + 0x0081)
#define IDS_ISDMF	(IDS_FORMAT_FIRST + 0x0082)
#define IDS_CHELP	(IDS_FORMAT_FIRST + 0x0083)
#define IDS_ABOUT	(IDS_FORMAT_FIRST + 0x0084)
#define IDS_SEADDLOGNOMEM (IDS_FORMAT_FIRST + 0x0085)
#define IDS_SELOGNOWRITE (IDS_FORMAT_FIRST + 0x0086)
#define IDS_CHKTITABOUT (IDS_FORMAT_FIRST + 0x0087)
#define IDS_NORMWARN	(IDS_FORMAT_FIRST + 0x0088)

#ifdef FROSTING
#define IDS_OK          (IDS_FORMAT_FIRST + 0x0089)
#define IDS_SAGETITLE   (IDS_FORMAT_FIRST + 0x008a)
#endif

#define IDL_ENGBADOPT	(IDS_FORMAT_FIRST + 0x00a0)
#define IDL_ENGNOTSUPP	(IDS_FORMAT_FIRST + 0x00a1)
#define IDL_ENGCANTWRT	(IDS_FORMAT_FIRST + 0x00a2)
#define IDL_ENGOS	(IDS_FORMAT_FIRST + 0x00a3)
#define IDL_ENGFS	(IDS_FORMAT_FIRST + 0x00a4)
#define IDL_ENGACTIVE	(IDS_FORMAT_FIRST + 0x00a5)
#define IDL_ENGLOCK	(IDS_FORMAT_FIRST + 0x00a6)
#define IDL_ENGMEM	(IDS_FORMAT_FIRST + 0x00a7)
#define IDL_ENGCANCEL	(IDS_FORMAT_FIRST + 0x00a8)
#define IDL_ENGCHKHSTRST (IDS_FORMAT_FIRST + 0x00a9)
#define IDL_ENGSARST	(IDS_FORMAT_FIRST + 0x00aa)
#define IDL_ENGUXP	(IDS_FORMAT_FIRST + 0x00ab)
#define IDL_TRAILER	(IDS_FORMAT_FIRST + 0x00ac)
#define IDL_NOERROR	(IDS_FORMAT_FIRST + 0x00ad)
#define IDL_ALLFIXED	(IDS_FORMAT_FIRST + 0x00ae)
#define IDL_SOMEFIXED	(IDS_FORMAT_FIRST + 0x00af)
#define IDL_NONEFIXED	(IDS_FORMAT_FIRST + 0x00b0)
#define IDL_TITLE1	(IDS_FORMAT_FIRST + 0x00b1)
#define IDL_TITLE2	(IDS_FORMAT_FIRST + 0x00b2)
#define IDL_TITLE3	(IDS_FORMAT_FIRST + 0x00b3)
#define IDL_COMPDISKH	(IDS_FORMAT_FIRST + 0x00b4)
#define IDL_TITLE4	(IDS_FORMAT_FIRST + 0x00b5)
#define IDL_OPTST	(IDS_FORMAT_FIRST + 0x00b6)
#define IDL_OPTPRE	(IDS_FORMAT_FIRST + 0x00b7)
#define IDL_OPTCDT	(IDS_FORMAT_FIRST + 0x00b8)
#define IDL_OPTCFN	(IDS_FORMAT_FIRST + 0x00b9)
#define IDL_OPTTH	(IDS_FORMAT_FIRST + 0x00ba)
#define IDL_OPTNWRT	(IDS_FORMAT_FIRST + 0x00bb)
#define IDL_OPTSYSO	(IDS_FORMAT_FIRST + 0x00bc)
#define IDL_OPTDTAO	(IDS_FORMAT_FIRST + 0x00bd)
#define IDL_OPTHDSYS	(IDS_FORMAT_FIRST + 0x00be)
#define IDL_CRLF	(IDS_FORMAT_FIRST + 0x00bf)
#define IDL_OPTCHST	(IDS_FORMAT_FIRST + 0x00c0)
#define IDL_OPTAUTOFIX	(IDS_FORMAT_FIRST + 0x00c1)
#define IDL_ECFULLCORR	(IDS_FORMAT_FIRST + 0x00c2)
#define IDL_ECCANTFIX	(IDS_FORMAT_FIRST + 0x00c3)
#define IDL_ECOTHERWRT	(IDS_FORMAT_FIRST + 0x00c4)
#define IDL_ECNOMEM	(IDS_FORMAT_FIRST + 0x00c5)
#define IDL_ECUNEXP	(IDS_FORMAT_FIRST + 0x00c6)
#define IDL_ECDISKE	(IDS_FORMAT_FIRST + 0x00c7)
#define IDL_NOMEMCAN	(IDS_FORMAT_FIRST + 0x00c8)
#define IDL_ECFILCOL	(IDS_FORMAT_FIRST + 0x00c9)
#define IDL_ECFILCRT	(IDS_FORMAT_FIRST + 0x00ca)
#define IDL_ECCLUSA	(IDS_FORMAT_FIRST + 0x00cb)
#define IDL_RSTLOCKLIM	(IDS_FORMAT_FIRST + 0x00cc)
#define IDL_RSTUNFERR	(IDS_FORMAT_FIRST + 0x00cd)
#define IDL_ECNOCORR	(IDS_FORMAT_FIRST + 0x00ce)
#define IDL_ECPCORROK	(IDS_FORMAT_FIRST + 0x00cf)
#define IDL_ECPCORRBAD	(IDS_FORMAT_FIRST + 0x00d0)
#define IDL_ECPRE	(IDS_FORMAT_FIRST + 0x00d1)
#define IDL_ERUNKNO	(IDS_FORMAT_FIRST + 0x00d2)
#define IDL_RSPRE	(IDS_FORMAT_FIRST + 0x00d3)
#define IDL_RSREP	(IDS_FORMAT_FIRST + 0x00d4)
#define IDL_RSIGN	(IDS_FORMAT_FIRST + 0x00d5)
#define IDL_RSDEL	(IDS_FORMAT_FIRST + 0x00d6)
#define IDL_RSTRNC	(IDS_FORMAT_FIRST + 0x00d7)
#define IDL_RSCAN	(IDS_FORMAT_FIRST + 0x00d8)
#define IDL_RSUNK	(IDS_FORMAT_FIRST + 0x00d9)
#define IDL_RSRMDF	(IDS_FORMAT_FIRST + 0x00da)
#define IDL_RSRMDBPB	(IDS_FORMAT_FIRST + 0x00db)
#define IDL_RSFLMDF	(IDS_FORMAT_FIRST + 0x00dc)
#define IDL_RSKLMDF	(IDS_FORMAT_FIRST + 0x00dd)
#define IDL_RSFLC	(IDS_FORMAT_FIRST + 0x00de)
#define IDL_RSKLC	(IDS_FORMAT_FIRST + 0x00df)
#define IDL_RSREPFF	(IDS_FORMAT_FIRST + 0x00e0)
#define IDL_RSREPVL	(IDS_FORMAT_FIRST + 0x00e1)
#define IDL_RSDELVL	(IDS_FORMAT_FIRST + 0x00e2)
#define IDL_RSMVRT	(IDS_FORMAT_FIRST + 0x00e3)
#define IDL_RSRETBD	(IDS_FORMAT_FIRST + 0x00e4)
#define IDL_RSUNMARK	(IDS_FORMAT_FIRST + 0x00e5)
#define IDL_RSDELXL	(IDS_FORMAT_FIRST + 0x00e6)
#define IDL_RSDELXLQ	(IDS_FORMAT_FIRST + 0x00e7)
#define IDL_RSMCXL	(IDS_FORMAT_FIRST + 0x00e8)
#define IDL_RSTNCXL	(IDS_FORMAT_FIRST + 0x00e9)
#define IDL_RSSODXL	(IDS_FORMAT_FIRST + 0x00ea)
#define IDL_RSSOTXL	(IDS_FORMAT_FIRST + 0x00eb)
#define IDL_RSRETMM	(IDS_FORMAT_FIRST + 0x00ec)
#define IDL_RSRETRD	(IDS_FORMAT_FIRST + 0x00ed)
#define IDL_RSRETWR	(IDS_FORMAT_FIRST + 0x00ee)
#define IDL_TITLE1A	(IDS_FORMAT_FIRST + 0x00ef)
#define IDL_ERFATXL	(IDS_FORMAT_FIRST + 0x00f0)
#define IDL_ERDDXLQ	(IDS_FORMAT_FIRST + 0x00f1)
#define IDL_XLLIST	(IDS_FORMAT_FIRST + 0x00f2)
#define IDL_XLFILE	(IDS_FORMAT_FIRST + 0x00f3)
#define IDL_XLFOLD	(IDS_FORMAT_FIRST + 0x00f4)
#define IDL_XLFRAG	(IDS_FORMAT_FIRST + 0x00f5)
#define IDL_XLUNMO	(IDS_FORMAT_FIRST + 0x00f6)
#define IDL_FATERRDIR_DOTS    (IDS_FORMAT_FIRST + 0x00f7)
#define IDL_FATERRDIR_LFNLST  (IDS_FORMAT_FIRST + 0x00f8)
#define IDL_FATERRDIR_ZRLEN   (IDS_FORMAT_FIRST + 0x00f9)
#define IDL_FATERRDIR_LOSTFIL (IDS_FORMAT_FIRST + 0x00fa)
#define IDL_FATERRDIR_DUPNM   (IDS_FORMAT_FIRST + 0x00fb)
#define IDL_FATERRDIR_BAD     (IDS_FORMAT_FIRST + 0x00fc)
#define IDL_FATERRDIR_PNOTD   (IDS_FORMAT_FIRST + 0x00fd)
#define IDL_FATERRDIR_BDENTS  (IDS_FORMAT_FIRST + 0x00fe)
#define IDL_FATERRFILE_INVLFN (IDS_FORMAT_FIRST + 0x00ff)
#define IDL_FATERRFILE_INVNM  (IDS_FORMAT_FIRST + 0x0100)
#define IDL_FATERRFILE_LFNSTR (IDS_FORMAT_FIRST + 0x0101)
#define IDL_FATERRFILE_LFNLEN (IDS_FORMAT_FIRST + 0x0102)
#define IDL_FATERRFILE_DEVNM  (IDS_FORMAT_FIRST + 0x0103)
#define IDL_FATERRFILE_SIZE   (IDS_FORMAT_FIRST + 0x0104)
#define IDL_FATERRFILE_SIZED  (IDS_FORMAT_FIRST + 0x0105)
#define IDL_FATERRFILE_DTTM1  (IDS_FORMAT_FIRST + 0x0106)
#define IDL_FATERRFILE_DTTM2  (IDS_FORMAT_FIRST + 0x0107)
#define IDL_FATERRFILE_DTTM3  (IDS_FORMAT_FIRST + 0x0108)
#define IDL_FATERRVOLLABALT   (IDS_FORMAT_FIRST + 0x0109)
#define IDL_FATERRMXPLENLALT  (IDS_FORMAT_FIRST + 0x010a)
#define IDL_FATERRMXPLENSALT  (IDS_FORMAT_FIRST + 0x010b)
#define IDL_DDERRMDFATALT     (IDS_FORMAT_FIRST + 0x010c)
#ifdef OPK2
#define IDL_RSDOSURF	       (IDS_FORMAT_FIRST + 0x010d)
#define IDL_FATERRFILE_SIZETB  (IDS_FORMAT_FIRST + 0x010e)
#define IDL_FATERRFILE_SIZETBD (IDS_FORMAT_FIRST + 0x010f)
//
// 0x0110 - 0x0147 taken for base error log messages below
//
#define IDL_FATERRRTDIR_INVFC  (IDS_FORMAT_FIRST + 0x0180)
#define IDL_FATERRRTDIR_PNOTD  (IDS_FORMAT_FIRST + 0x0181)
#define IDL_FATERRRTDIR_RECRT  (IDS_FORMAT_FIRST + 0x0182)
#define IDL_FATERRRTDIR_FND    (IDS_FORMAT_FIRST + 0x0183)
#define IDL_FATERRRTDIR_MBYFND (IDS_FORMAT_FIRST + 0x0184)
#define IDL_FATERRRTDIR_INVC   (IDS_FORMAT_FIRST + 0x0185)
#define IDL_FATERRRTDIR_CIRCC  (IDS_FORMAT_FIRST + 0x0186)
#define IDL_FATERRRTDIR_TOOBIG (IDS_FORMAT_FIRST + 0x0187)
#endif

//
#ifdef OPK2
// Following defines up to 56 IDs starting here (110-147 inclusive)
#else
// Following defines up to 53 IDs starting here (110-144 inclusive)
#endif
//
#define IDL_ER_FIRST	(IDS_FORMAT_FIRST + 0x0110)


#define DLG_FORMATREPORT	400
#define DLG_FORMAT		401

#define DLG_CHKDSKSAOPT 	408
#define DLG_CHKDSKADVOPT	409
#define DLG_CHKDSKREPORT	410
#define DLG_CHKDSK		411
#define DLG_CHKDSKTL		412

#define IDI_CHKICON		430
#define IDI_CHKDLGICON		431
#ifdef OPK2
#define IDI_CHKDLGICN2		432
#define IDI_CHKDLGICN3		433
#endif

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
#define ISTR_FATERRDIR_BDENTS	0x303b
#define ISTR_FATERRDIR_BDENTSC	0x303c

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
#define ALTISTR_FATERRFILE_INVLFN1 0x3343
#define ALTISTR_FATERRFILE_INVLFN2 0x3344
#define ALTISTR_FATERRFILE_INVNM1  0x3345
#define ALTISTR_FATERRFILE_INVNM2  0x3346
#ifdef OPK2
#define ISTR_FATERRFILE_SIZETB1  0x3347
#define ISTR_FATERRFILE_SIZETB2  0x3348
#define ISTR_FATERRFILE_SIZEDTB1 0x3349
#define ISTR_FATERRFILE_SIZEDTB2 0x334a
#endif


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
#ifdef OPK2
#define IERR_FATERRBOOT 	53
#define ISTR_FATERRBOOT 	0x4a80

#define IERR_FATERRSHDSURF	54
#define ISTR_FATERRSHDSURF	0x4b00

#define IERR_FATERRROOTDIR	55
#define ISTR_FATERRROOTDIR	0x4b80
#define ISTR_FATERRRTDIR_INVFC	0x4bb1
#define ISTR_FATERRRTDIR_PNOTD	0x4bb2
#define ISTR_FATERRRTDIR_INVC	0x4bb3
#define ISTR_FATERRRTDIR_CIRCC	0x4bb4
#define ISTR_FATERRRTDIR_TOOBIG 0x4bb5
#define ISTR_FATERRRTDIR_FND	0x4bb6
#define ISTR_FATERRRTDIR_MBYFND 0x4bb7
#define ISTR_FATERRRTDIR_RECRT	0x4bb8

#define IERR_ERRISBAD7		56
#define ISTR_ERRISBAD7		0x4c00

#define IDS_REP_TOTM		0x6001
#define IDS_REP_BADM		0x6002
#define IDS_REP_DIRM		0x6003
#define IDS_REP_HIDM		0x6004
#define IDS_REP_USERM		0x6005
#define IDS_REP_AVAILM		0x6006
#define IDS_REP_TOTK		0x6007
#define IDS_REP_BADK		0x6008
#define IDS_REP_DIRK		0x6009
#define IDS_REP_HIDK		0x600a
#define IDS_REP_USERK		0x600b
#define IDS_REP_AVAILK		0x600c
#endif

//
// Following two special values are used only for the logging
//
#define IERR_FATXLNK		0xFFFE
#define IERR_DDERRXLSQZ 	0xFFFF

#define ISTR_SE_IGNORE		0x1800
#define ISTR_SE_DIRDEL		0x1801
#define ISTR_XL_TITLE		0x1802
#define ISTR_SE_REPAIR		0x1803
#define ISTR_SE_FILDEL		0x1804
#define ISTR_SE_FILTRNC 	0x1805
#define ISTR_SE_DIRTRNC 	0x1806
#define ISTR_SE_CONTB		0x1807
#define ISTR_XL_ALTTXT1 	0x1808
#define ISTR_XL_ALTTXT2A	0x1809
#define ISTR_XL_ALTTXT2B	0x180a
#define ISTR_XL_ALTBUTTREP	ISTR_SE_REPAIR
#define ISTR_XL_ALTBUTTDEL	0x180b
