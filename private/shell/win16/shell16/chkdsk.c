//
// chkdsk.c : Control Panel Applet for Windows Disk Manager
//

#include "shprv.h"

#include <dskmaint.h>
#include <prsht.h>
#include <regstr.h>
#include <winerror.h>

#include <memory.h>
#include "util.h"
#include "format.h"
#include <help.h>

#ifdef FROSTING
#ifndef REGSTR_PATH_SCANDSKW_SAGESET
#define REGSTR_PATH_SCANDSKW_SAGESET "Software\\Microsoft\\Plus!\\System Agent\\SAGE\\ScanDisk for Windows\\Set%lu"
#endif

#ifndef REGSTR_PATH_CHECKDISKDRIVES
#define REGSTR_PATH_CHECKDISKDRIVES   "DrivesToCheck"
#endif
#endif

//#define DOSETUPCHK 1

#ifdef	DOSETUPCHK
typedef DWORD (WINAPI *DMaint_CheckDriveSetupPROC)(UINT Drive, DWORD Options, DSKUTILCBPROC lpfnCallBack, LPARAM lRefData);
#endif

#define g_hInstance g_hinst
static  char g_szNULL[] = "";   // c_szNull

HIMAGELIST g_himlIconsSmall = NULL;
int	g_cxIcon;
int	g_cyIcon;
HWND	g_ChkWndPar = NULL;
ATOM	g_ChkWndClass = 0;

static DWORD FORMATSEG ChkaIds[]={DLGCHK_START,   IDH_WINDISK_MAIN_START,
				  DLGCHK_CANCEL,  IDH_WINDISK_MAIN_CLOSE,
				  DLGCHK_PBAR,	  0xFFFFFFFFL,
				  DLGCHK_STATTXT, 0xFFFFFFFFL,
				  DLGCHK_STATTXT2,0xFFFFFFFFL,
				  DLGCHK_ADVANCED,IDH_WINDISK_MAIN_ADVANCED,
				  DLGCHK_DRVLIST, IDH_WINDISK_MAIN_LIST,
				  DLGCHK_DOBAD,   IDH_WINDISK_MAIN_THOROUGH,
				  DLGCHK_BADOPT,  IDH_WINDISK_MAIN_OPTIONS,
				  DLGCHK_AUTOFIX, IDH_WINDISK_MAIN_AUTOFIX,
				  DLGCHK_NOBADB,  IDH_WINDISK_MAIN_STANDARD,
				  DLGCHK_DTXT1,   IDH_WINDISK_MAIN_STANDARD,
				  DLGCHK_DTXT2,   IDH_WINDISK_MAIN_THOROUGH,
				  IDC_ICON_1,     0xFFFFFFFFL,
				  IDC_TEXT,       IDH_WINDISK_MAIN_LIST,
				  IDC_GROUPBOX_1, IDH_COMM_GROUPBOX,
				  0,0};

#ifdef OPK2
static DWORD FORMATSEG ChkaIdsSage[]={DLGCHK_START,   IDH_OK,
				      DLGCHK_CANCEL,  IDH_CANCEL,
				      DLGCHK_PBAR,    0xFFFFFFFFL,
				      DLGCHK_STATTXT, 0xFFFFFFFFL,
				      DLGCHK_STATTXT2,0xFFFFFFFFL,
				      DLGCHK_ADVANCED,IDH_WINDISK_MAIN_ADVANCED,
				      DLGCHK_DRVLIST, IDH_WINDISK_MAIN_LIST,
				      DLGCHK_DOBAD,   IDH_WINDISK_MAIN_THOROUGH,
				      DLGCHK_BADOPT,  IDH_WINDISK_MAIN_OPTIONS,
				      DLGCHK_AUTOFIX, IDH_WINDISK_MAIN_AUTOFIX,
				      DLGCHK_NOBADB,  IDH_WINDISK_MAIN_STANDARD,
				      DLGCHK_DTXT1,   IDH_WINDISK_MAIN_STANDARD,
				      DLGCHK_DTXT2,   IDH_WINDISK_MAIN_THOROUGH,
				      IDC_ICON_1,     0xFFFFFFFFL,
				      IDC_TEXT,       IDH_WINDISK_MAIN_LIST,
				      IDC_GROUPBOX_1, IDH_COMM_GROUPBOX,
				      0,0};
#endif

static DWORD FORMATSEG ChkAdvaIds[]={DLGCHKADV_OK,	 IDH_OK,
				     DLGCHKADV_CANCEL,	 IDH_CANCEL,
				     DLGCHKADV_LSTF,	 IDH_WINDISK_ADV_FREE,
				     DLGCHKADV_LSTMF,	 IDH_WINDISK_ADV_CONVERT,
				     DLGCHKADV_XLDEL,	 IDH_WINDISK_ADV_DELETE,
				     DLGCHKADV_XLCPY,	 IDH_WINDISK_ADV_MAKE_COPIES,
				     DLGCHKADV_XLIGN,	 IDH_WINDISK_ADV_IGNORE,
				     DLGCHKADV_CHKDT,	 IDH_WINDISK_ADV_DATE_TIME,
				     DLGCHKADV_CHKNM,	 IDH_WINDISK_ADV_FILENAME,
				     DLGCHKADV_CHKHST,	 IDH_WINDISK_ADV_CHECK_HOST,
				     DLGCHKADV_REPALWAYS,IDH_WINDISK_ADV_ALWAYS,
				     DLGCHKADV_NOREP,	 IDH_WINDISK_ADV_NEVER,
				     DLGCHKADV_REPIFERR, IDH_WINDISK_ADV_ONLY_IF_FOUND,
				     DLGCHKADV_LOGREP,	 IDH_WINDISK_REPLACE_LOG,
				     DLGCHKADV_LOGAPPND, IDH_WINDISK_APPEND_LOG,
				     DLGCHKADV_NOLOG,	 IDH_WINDISK_NO_LOG,
				     IDC_GROUPBOX_1,     IDH_COMM_GROUPBOX,
				     IDC_GROUPBOX_2,     IDH_COMM_GROUPBOX,
				     IDC_GROUPBOX_3,     IDH_COMM_GROUPBOX,
				     IDC_GROUPBOX_4,     IDH_COMM_GROUPBOX,
				     IDC_GROUPBOX_5,	 IDH_COMM_GROUPBOX,
				     0,0};

static DWORD FORMATSEG ChkSAOaIds[]={DLGCHKSAO_OK,	 IDH_OK,
				     DLGCHKSAO_CANCEL,	 IDH_CANCEL,
				     DLGCHKSAO_NOWRTTST, IDH_WINDISK_OPTIONS_NO_WRITE_TEST,
				     DLGCHKSAO_ALLHIDSYS,IDH_WINDISK_OPTIONS_NO_HID_SYS,
				     DLGCHKSAO_DOALL,	 IDH_WINDISK_OPTIONS_SYS_AND_DATA,
				     DLGCHKSAO_NOSYS,	 IDH_WINDISK_OPTIONS_DATA_ONLY,
				     DLGCHKSAO_NODATA,	 IDH_WINDISK_OPTIONS_SYS_ONLY,
				     IDC_GROUPBOX_1,	 IDH_COMM_GROUPBOX,
				     IDC_TEXT,	         0xFFFFFFFFL,
				     0,0};

DWORD WINAPI SHGetFileInfo(LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFO FAR * hfi, UINT cbFileInfo, UINT uFlags);

// convert a file spec to make it look a bit better
// if it is all upper case chars

typedef struct _SHCHECKDISKINFO
{
	DMAINTINFO sDMaint;
	LPMYCHKINFOSTRUCT lpMyChkInf;
} SHCHECKDISKINFO, FAR *LPSHCHECKDISKINFO;


#pragma optimize("lge",off)

BOOL NEAR PASCAL SaveTimeInReg(HKEY hkRoot, LPCSTR pszSubKey,
			       LPCSTR pszValName, WORD ResultCode,
			       BOOL IsSysTime)
{
    DSKTLSYSTEMTIME systime;
    BOOL	    bRet;
    HKEY	    hk;

    _asm {
	// Get the date information

	mov	ah,2ah
	int	21h
	xor	bx,bx

	mov	systime.wYear,cx

	mov	bl,dh
	mov	systime.wMonth,bx

	mov	bl,al
	mov	systime.wDayOfWeek,bx

	mov	bl,dl
	mov	systime.wDay,bx

	// Get the time information

	mov	ah,2ch
	int	21h
	xor	bx,bx

	mov	bl,ch
	mov	systime.wHour,bx

	mov	bl,cl
	mov	systime.wMinute,bx

	mov	bl,dh
	mov	systime.wSecond,bx

	mov	bl,dl
	mov	systime.wMilliseconds,bx
    }

    systime.wResult = ResultCode;

    // Convert from 1/100 second to 1/1000 second

    systime.wMilliseconds *= 10;

    if (RegCreateKey(hkRoot, pszSubKey, &hk) != ERROR_SUCCESS)
    {
	return(FALSE);
    }
    if(IsSysTime)
    {
	bRet = RegSetValueEx(hk, pszValName, 0, REG_BINARY,
			     (LPBYTE)&systime, (sizeof(systime) - 2)) == ERROR_SUCCESS;
    } else {
	bRet = RegSetValueEx(hk, pszValName, 0, REG_BINARY,
			     (LPBYTE)&systime, sizeof(systime)) == ERROR_SUCCESS;
    }
    RegCloseKey(hk);

    return(bRet);
}

//
// Get the basic DBL/DRVSPACE info, in particular, what is the host drive
// (return from function), and what is the extension of the volume CVF on
// the host volume (returned in lpext).
//
WORD NEAR PASCAL GetCompInfo(UINT Drive, LPWORD lpext)
{
    WORD  HostDrive = 0xFFFF;
    WORD  ext;

    _asm {
	mov	ax, 0x4A11
	xor	bx, bx		// 0 = check_version
	int	0x2F		// Is DoubleSpace around?

	or	ax, ax
	jnz	notdouble	// Nope.

	mov	ax, 0x4A11	// DBLSPACE.BIN INT 2F number
	mov	bx, 1		// 1 = GetDriveMap function
	mov	dx, Drive
	int	0x2F		// (bl AND 80h) == DS drive flag
				// (bl AND 7Fh) == host drive

	or	ax, ax		// Success?
	jnz	notdouble	// Nope.

	test	bl, 0x80	// Is the drive compressed?
	jz	notdouble	//    NO

	// We have a DoubleSpace Drive, need to figure out host drive.
	//
	// This is tricky because of the manner in which DRV/DBLSPACE.BIN
	// keeps track of drives.
	//
	// For a swapped CVF, the current drive number of the host
	// drive is returned by the first GetDriveMap call.  But for
	// an unswapped CVF, we must make a second GetDriveMap call
	// on the "host" drive returned by the first call.  But, to
	// distinguish between swapped and unswapped CVFs, we must
	// make both of these calls.  So, we make them, and then check
	// the results.

	mov	cl, bh
	xor	ch, ch
	mov	ext, cx 	// Save the drive's extension
	and	bl, 0x7F	// bl = "host" drive number
	xor	bh, bh
	mov	HostDrive, bx	// Save 1st host drive
	mov	dl, bl		// Set up for query of "host" drive

	mov	ax, 0x4A11	// DBLSPACE.BIN INT 2F number
	mov	bx, 1		// 1 = GetDriveMap function
	int	0x2F		// (bl AND 7Fh) == 2nd host drive

	test	bl, 80h 	// Is the host a hard drive?
	jz	gdiExit 	// if so, it's the real host.

	and	bx, 007Fh	// Otherwise, this thing is swapped
	mov	HostDrive, bx	// with the host.
      gdiExit:
    }

    *lpext = ext;

notdouble:
    return(HostDrive);
}

#pragma optimize("",on)


BOOL NEAR SetDriveTitle(LPMYCHKINFOSTRUCT lpMyChkInf, HWND hwnd)
{
    int i;
    PSTR	       pMsgBuf;
    LPSTR lpszReturn;
    LPSTR lpszTemp;

#ifdef FROSTING
    if (lpMyChkInf->fSageSet)
       return TRUE;
#endif

#define SZTEMPORARY 512
    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,sizeof(char) * SZTEMPORARY);
    if (!pMsgBuf)
        return FALSE;
    
    lpszTemp = (LPSTR)(pMsgBuf + (SZTEMPORARY/2));
    lpszReturn = (LPSTR)pMsgBuf;
    
    // This dialog does not do the split drive title

    if(lpMyChkInf->MyFixOpt & DLGCHK_RO)
    {
        i = IDS_CHKTIT3;
    } else {
        i = IDS_CHKTIT2;
    }
    LoadString(g_hInstance, i, lpszTemp, (SZTEMPORARY/2));

    if((lpMyChkInf->IsSplitDrv) && (!(lpMyChkInf->DoingCompDrv)))
    {
        wsprintf(lpszReturn,lpszTemp,(LPSTR)lpMyChkInf->CompdriveNameStr);
    } else {
        wsprintf(lpszReturn,lpszTemp,(LPSTR)lpMyChkInf->lpwddi->driveNameStr);
    }
    
    SetWindowText(hwnd,lpszReturn);
    LocalFree((HANDLE)pMsgBuf);
    return TRUE;
}

VOID NEAR SetStdChkTitle(LPMYCHKINFOSTRUCT lpMyChkInf, 
			 LPSTR lpszFormat,LPSTR lpszReturn,int cReturn)
{
    if((lpMyChkInf->IsSplitDrv) && (!(lpMyChkInf->DoingCompDrv)))
    {
        char szBuffer[128];
        
	LoadString(g_hInstance, IDS_COMPDISKH, lpszReturn, cReturn);
	wsprintf((LPSTR)szBuffer,(LPSTR)lpszReturn,(LPSTR)lpMyChkInf->CompdriveNameStr);
	wsprintf((LPSTR)lpszReturn,(LPSTR)lpszFormat,(LPSTR)szBuffer);
    } else {
	wsprintf((LPSTR)lpszReturn,(LPSTR)lpszFormat,
                 (LPSTR)lpMyChkInf->lpwddi->driveNameStr);
    }
}

#if 0 // unused
WORD FAR  MyChkdskMessageBoxBuf(LPMYCHKINFOSTRUCT lpMyChkInf,
				LPSTR lpMsgBuf, WORD BoxStyle)
{
    PSTR	      pMsgBuf;
    WORD	      j;

#define SZTYPBUF11     120
#define SZTITBUF11     120
#define SZFMTBUF11     128
#define TOTMSZ11       (SZTYPBUF11+SZTITBUF11+SZFMTBUF11)

#define TypeBuf11 (&(pMsgBuf[0]))
#define TitBuf11  (&(pMsgBuf[SZTYPBUF11]))
#define FmtBuf11  (&(pMsgBuf[SZTYPBUF11+SZTITBUF11]))

    if(lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND))
    {
	switch(BoxStyle & MB_TYPEMASK)
	{
	    case MB_ABORTRETRYIGNORE:
		switch(BoxStyle & MB_DEFMASK)
		{
		    case MB_DEFBUTTON3:
			return(IDIGNORE);
			break;

		    case MB_DEFBUTTON2:
			return(IDRETRY);
			break;

		    case MB_DEFBUTTON1:
		    default:
			return(IDABORT);
			break;
		}
		break;

	    case MB_YESNOCANCEL:
		switch(BoxStyle & MB_DEFMASK)
		{
		    case MB_DEFBUTTON3:
			return(IDCANCEL);
			break;

		    case MB_DEFBUTTON2:
			return(IDNO);
			break;

		    case MB_DEFBUTTON1:
		    default:
			return(IDYES);
			break;
		}
		break;

	    case MB_YESNO:
		if((BoxStyle & MB_DEFMASK) == MB_DEFBUTTON1)
		    return(IDYES);
		else
		    return(IDNO);
		break;

	    case MB_RETRYCANCEL:
		if((BoxStyle & MB_DEFMASK) == MB_DEFBUTTON1)
		    return(IDRETRY);
		else
		    return(IDCANCEL);
		break;

	    case MB_OKCANCEL:
		if((BoxStyle & MB_DEFMASK) == MB_DEFBUTTON1)
		    return(IDOK);
		else
		    return(IDCANCEL);
		break;

	    case MB_OK:
	    default:
		return(IDOK);
		break;
	}
    }
    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ11);
    if(!pMsgBuf)
    {
	// BUG BUG
	return(IDCANCEL);
    }

    LoadString(g_hInstance, IDS_DRIVETITLEC, FmtBuf11, SZFMTBUF11);
    SetStdChkTitle(lpMyChkInf, FmtBuf11, TitBuf11, SZFMTBUF11);
    j = MessageBox(lpMyChkInf->hProgDlgWnd,lpMsgBuf,TitBuf11,BoxStyle);

    LocalFree((HANDLE)pMsgBuf);

    return(j);
}
#endif

WORD FAR  MyChkdskMessageBox(LPMYCHKINFOSTRUCT lpMyChkInf,
			     WORD MsgID, WORD BoxStyle)
{
    PSTR	      pMsgBuf;
    WORD	      j;

#define SZMSGBUF12     256
#define SZTYPBUF12     120
#define SZTITBUF12     120
#define SZFMTBUF12     128
#define TOTMSZ12       (SZMSGBUF12+SZTYPBUF12+SZTITBUF12+SZFMTBUF12)

#define MsgBuf12  (&(pMsgBuf[0]))
//#define TypeBuf12 (&(pMsgBuf[SZMSGBUF12]))
#define TitBuf12  (&(pMsgBuf[SZMSGBUF12+SZTYPBUF12]))
#define FmtBuf12  (&(pMsgBuf[SZMSGBUF12+SZTYPBUF12+SZTITBUF12]))

    if(lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND))
    {
	switch(BoxStyle & MB_TYPEMASK)
	{
	    case MB_ABORTRETRYIGNORE:
		switch(BoxStyle & MB_DEFMASK)
		{
		    case MB_DEFBUTTON3:
			return(IDIGNORE);
			break;

		    case MB_DEFBUTTON2:
			return(IDRETRY);
			break;

		    case MB_DEFBUTTON1:
		    default:
			return(IDABORT);
			break;
		}
		break;

	    case MB_YESNOCANCEL:
		switch(BoxStyle & MB_DEFMASK)
		{
		    case MB_DEFBUTTON3:
			return(IDCANCEL);
			break;

		    case MB_DEFBUTTON2:
			return(IDNO);
			break;

		    case MB_DEFBUTTON1:
		    default:
			return(IDYES);
			break;
		}
		break;

	    case MB_YESNO:
		if((BoxStyle & MB_DEFMASK) == MB_DEFBUTTON1)
		    return(IDYES);
		else
		    return(IDNO);
		break;

	    case MB_RETRYCANCEL:
		if((BoxStyle & MB_DEFMASK) == MB_DEFBUTTON1)
		    return(IDRETRY);
		else
		    return(IDCANCEL);
		break;

	    case MB_OKCANCEL:
		if((BoxStyle & MB_DEFMASK) == MB_DEFBUTTON1)
		    return(IDOK);
		else
		    return(IDCANCEL);
		break;

	    case MB_OK:
	    default:
		return(IDOK);
		break;
	}
    }
    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ12);
    if(!pMsgBuf)
    {
	// BUG BUG
	return(IDCANCEL);
    }

    if((MsgID == IDS_CANTCHKALL) || (MsgID == IDS_NOSEL) || (MsgID == IDS_NOMEM2))
    {
	LoadString(g_hInstance, IDS_CHKTIT, TitBuf12, SZTITBUF12);
    } else {
	LoadString(g_hInstance, IDS_DRIVETITLEC, FmtBuf12, SZFMTBUF12);
        
        SetStdChkTitle(lpMyChkInf, FmtBuf12, TitBuf12, SZTITBUF12);
    }

    LoadString(g_hInstance, MsgID, MsgBuf12, SZMSGBUF12);

    j = MessageBox(lpMyChkInf->hProgDlgWnd,MsgBuf12,TitBuf12,BoxStyle);

    LocalFree((HANDLE)pMsgBuf);

    return(j);
}

#ifdef FROSTING
DWORD NEAR ChkFindHiddenDrives (VOID)
{
    HKEY   hKey;
    DWORD  typ;
    DWORD  sz;
    DWORD  HiddenDrives = 0L;
    char   RegKey[80];

    lstrcpy(RegKey, REGSTR_PATH_POLICIES);
    lstrcat(RegKey, "\\Explorer");

    if(RegOpenKey(HKEY_CURRENT_USER,RegKey,&hKey) == ERROR_SUCCESS)
    {
	sz = sizeof(DWORD);
	if((RegQueryValueEx(hKey,"NoDrives",NULL,&typ,(LPBYTE)&HiddenDrives, &sz) != ERROR_SUCCESS) ||
	   (typ != REG_DWORD)  ||
	   (sz != 4L))
	{
	    HiddenDrives = 0L;
	}
	RegCloseKey(hKey);
    }

    return HiddenDrives;
}

DWORD NEAR ChkFindAllFixedDrives (VOID)
{
    DWORD FixedDrives = 0L;
    DWORD HiddenDrives;
    WORD  i;

    HiddenDrives = ChkFindHiddenDrives();

    for(i = 0; i < 26; i++)
    {
	if(HiddenDrives & (0x00000001L << i))
	    continue;

	if((DriveType(i) == DRIVE_FIXED) || (DriveType(i) == DRIVE_RAMDRIVE))
	{
	    FixedDrives |= (0x00000001L << i);
	}
    }

    return FixedDrives;
}

BOOL NEAR ChkAreAllFixedDrivesSelected (DWORD DrivesToChk)
{
    DWORD FixedDrives;
    WORD  i;

    FixedDrives = ChkFindAllFixedDrives();

    for(i = 0; i < 26; i++)
    {
	if(FixedDrives & (0x00000001L << i))
	{
	    if(!(DrivesToChk & (0x00000001L << i)))
	        return FALSE;
	}
    }

    return TRUE;
}
#endif

#ifndef cbMaxREGPATH
#define cbMaxREGPATH     256
#endif

VOID NEAR GetChkRegOptions(LPMYCHKINFOSTRUCT lpMyChkInf)
{
    DWORD dwi;
    DWORD typ;
    DWORD sz;
    HKEY  hKey;
#ifdef FROSTING
    char  keyname[ cbMaxREGPATH ];
    LONG  rc;
#endif


    lpMyChkInf->RegOptions = MAKELONG((DLGCHK_NOBAD | DLGCHK_XLCPY   |
				       DLGCHK_INTER | DLGCHK_NOCHKDT |
				       DLGCHK_LSTMF | DLGCHK_REP),
				      0);
#ifdef OPK2
#ifdef FROSTING
    if(lpMyChkInf->fSageSet)
	lpMyChkInf->RegOptions |= MAKELONG(0,DLGCHK_REPONLYERR);
#endif
#endif
    lpMyChkInf->NoUnsupDrvs = 0L;
    if(RegOpenKey(HKEY_CURRENT_USER,REGSTR_PATH_CHECKDISK,&hKey) == ERROR_SUCCESS)
    {
#ifndef FROSTING
      sz = sizeof(DWORD);
      if((RegQueryValueEx(hKey,REGSTR_PATH_CHECKDISKSET,NULL,&typ,(LPBYTE)&dwi, &sz) == ERROR_SUCCESS) &&
	 (typ == REG_BINARY)  &&
	 (sz == 4L))
      {
	  lpMyChkInf->RegOptions = dwi;
      }
#endif
      sz = sizeof(DWORD);
      if((RegQueryValueEx(hKey,REGSTR_PATH_CHECKDISKUDRVS,NULL,&typ,(LPBYTE)&dwi, &sz) == ERROR_SUCCESS) &&
	 (typ == REG_BINARY)  &&
	 (sz == 4L))
      {
	  lpMyChkInf->NoUnsupDrvs = dwi;
      }
      RegCloseKey(hKey);
    }

#ifdef FROSTING
    if (lpMyChkInf->idxSettings == (DWORD)0xFFFFFFFF) {
	rc = RegOpenKey(HKEY_CURRENT_USER,REGSTR_PATH_CHECKDISK,&hKey);
    } else {
	lpMyChkInf->DrivesToChk = 0L;
	wsprintf (keyname, REGSTR_PATH_SCANDSKW_SAGESET, lpMyChkInf->idxSettings);
	rc = RegOpenKey(HKEY_LOCAL_MACHINE,keyname,&hKey);
    }

    if(rc == ERROR_SUCCESS)
    {
	sz = sizeof(DWORD);
	rc = RegQueryValueEx(hKey,REGSTR_PATH_CHECKDISKSET,NULL,
			&typ,(LPBYTE)&dwi, &sz);
	if((rc == ERROR_SUCCESS) && (typ == REG_BINARY)  && (sz == 4L))
	{
	    lpMyChkInf->RegOptions = dwi;
	}

	if(lpMyChkInf->idxSettings != (DWORD)0xFFFFFFFF)
	{
	    sz = sizeof(DWORD);
	    rc = RegQueryValueEx(hKey,REGSTR_PATH_CHECKDISKDRIVES,NULL,
			    &typ,(LPBYTE)&dwi, &sz);
	    if((rc == ERROR_SUCCESS) && (typ == REG_BINARY) && (sz == 4L))
	    {
		lpMyChkInf->DrivesToChk = dwi;

		if (lpMyChkInf->DrivesToChk & DTC_ALLFIXEDDRIVES)
		{
		    lpMyChkInf->DrivesToChk &= ~DTC_ALLFIXEDDRIVES;
		    lpMyChkInf->DrivesToChk |= ChkFindAllFixedDrives();
		}
	    }
	}

	RegCloseKey(hKey);
    }

    if((lpMyChkInf->idxSettings != (DWORD)0xFFFFFFFF) && (lpMyChkInf->DrivesToChk == 0L))
    {
	lpMyChkInf->DrivesToChk = ChkFindAllFixedDrives();
    }
#endif

    return;
}

#ifdef FROSTING
VOID NEAR SetChkRegOptions(DWORD options, DWORD NoUnsupDrvs,
			   DWORD DrivesToChk, DWORD idxSettings)
#else
VOID NEAR SetChkRegOptions(DWORD options, DWORD NoUnsupDrvs)
#endif
{
    HKEY  hKey;

#ifndef FROSTING
    if(RegOpenKey(HKEY_CURRENT_USER,REGSTR_PATH_CHECKDISK,&hKey) == ERROR_SUCCESS)
    {
SetIt:
      RegSetValueEx(hKey,REGSTR_PATH_CHECKDISKSET,NULL,REG_BINARY,(LPBYTE)&options,sizeof(DWORD));
      RegSetValueEx(hKey,REGSTR_PATH_CHECKDISKUDRVS,NULL,REG_BINARY,(LPBYTE)&NoUnsupDrvs,sizeof(DWORD));
      RegCloseKey(hKey);
    } else {
      if(RegCreateKey(HKEY_CURRENT_USER,REGSTR_PATH_CHECKDISK,&hKey) == ERROR_SUCCESS)
      {
	  goto SetIt;
      }
    }
#else
    char  keyname[ cbMaxREGPATH ];
    LONG  rc;

    rc = RegCreateKey(HKEY_CURRENT_USER,REGSTR_PATH_CHECKDISK,&hKey);
    if(rc == ERROR_SUCCESS)
    {
	RegSetValueEx(hKey,REGSTR_PATH_CHECKDISKUDRVS,NULL,
		      REG_BINARY,(LPBYTE)&NoUnsupDrvs,sizeof(DWORD));
	RegCloseKey(hKey);
    }

    if (idxSettings == (DWORD)0xFFFFFFFF) {
	rc = RegCreateKey(HKEY_CURRENT_USER,REGSTR_PATH_CHECKDISK,&hKey);
    } else {
	wsprintf(keyname, REGSTR_PATH_SCANDSKW_SAGESET, idxSettings);
	rc = RegCreateKey(HKEY_LOCAL_MACHINE,keyname,&hKey);
    }

    if(rc == ERROR_SUCCESS)
    {
	options &= ~(SHCHK_OPT_SAGESET | SHCHK_OPT_SAGERUN);
	RegSetValueEx(hKey,REGSTR_PATH_CHECKDISKSET,NULL,
		      REG_BINARY,(LPBYTE)&options,sizeof(DWORD));

	if (idxSettings != (DWORD)0xFFFFFFFF)
	{
	    if(ChkAreAllFixedDrivesSelected (DrivesToChk))
	        DrivesToChk |= DTC_ALLFIXEDDRIVES;
	    RegSetValueEx(hKey,REGSTR_PATH_CHECKDISKDRIVES,NULL,
			  REG_BINARY,(LPBYTE)&DrivesToChk,sizeof(DWORD));
	}
	RegCloseKey(hKey);
    }
#endif

    return;
}

#define SZBUFA3       256
#define SZBUFB3       256
#define SZBUFC3       256
#define SZBUFD3       256
#define SZBUFE3       256
#define TOTMSZ3       (SZBUFA3+SZBUFB3+SZBUFC3+SZBUFD3+SZBUFE3)

#define bBuf1 (&(pMsgBuf[0]))
#define bBuf2 (&(pMsgBuf[SZBUFA3]))
#define bBuf3 (&(pMsgBuf[SZBUFA3+SZBUFB3]))
#define bBuf4 (&(pMsgBuf[SZBUFA3+SZBUFB3+SZBUFC3]))
#define bBuf5 (&(pMsgBuf[SZBUFA3+SZBUFB3+SZBUFC3+SZBUFD3]))


LRESULT CALLBACK ChkCBProc(UINT msg, LPARAM lRefData, LPARAM lParam1,
			   LPARAM lParam2, LPARAM lParam3,
			   LPARAM lParam4, LPARAM lParam5)
{
    LPMYCHKINFOSTRUCT  lpMyChkInf;
    DLGPROC	       lpfnDlgProc;
    DWORD	       dwi;
    BOOL	       PMRet;
    MSG 	       wmsg;
    WORD	       i;
    WORD	       j;
    PSTR	       pMsgBuf;
    BYTE	       LabBuf[20];
#ifdef FROSTING
    DWORD              LockWrtRestartMax;
#endif


#define SZBUFA4       128
#define SZBUFB4       128
#define SZBUFC4       512
#define TOTMSZ4       (SZBUFC4+SZBUFA4+SZBUFB4)

#define dBuf3 (&(pMsgBuf[0]))
#define dBuf1 (&(pMsgBuf[SZBUFC4]))
#define dBuf2 (&(pMsgBuf[SZBUFC4+SZBUFA4]))

    lpMyChkInf			  = (LPMYCHKINFOSTRUCT)lRefData;
    lpMyChkInf->lpFixFDisp	  = (LPFIXFATDISP)lParam1;
    pMsgBuf			  = 0;
    lpMyChkInf->lParam1 	  = lParam1;
    lpMyChkInf->lParam2 	  = lParam2;
    lpMyChkInf->lParam3 	  = lParam3;
    lpMyChkInf->lParam4 	  = lParam4;
    lpMyChkInf->lParam5 	  = lParam5;
    lpMyChkInf->IsFolder	  = FALSE;
    lpMyChkInf->IsRootFolder	  = FALSE;
    lpMyChkInf->UseAltDlgTxt	  = FALSE;
    lpMyChkInf->UseAltDefBut	  = FALSE;
    lpMyChkInf->CancelIsDefault   = FALSE;
    lpMyChkInf->AltDefButIndx	  = 0;
    lpMyChkInf->UseAltCantFix	  = FALSE;
    lpMyChkInf->AltCantFixTstFlag = 0;
    lpMyChkInf->AltCantFixHID	  = 0xFFFFFFFFL;
    lpMyChkInf->AltCantFixRepHID  = 0xFFFFFFFFL;

    switch(msg)
    {
	case DU_ERRORDETECTED:
	    SEAddToLogRCS(lpMyChkInf,IDL_CRLF,NULL);

	    if((LOWORD(lpMyChkInf->lParam2) != WRITEERROR) &&
	       (LOWORD(lpMyChkInf->lParam2) != READERROR)    )
	    {
		lpMyChkInf->RWRstsrtCnt   = 0;
	    }

	    switch(LOWORD(lpMyChkInf->lParam2)) {

		case ERRLOCKV:	// This will get logged on engine return
		    return(0L);
		    break;

		case WRITEERROR:
		    if(HIWORD(lpMyChkInf->lParam2) & FILCOLL)
		    {
			//
			// We will handle this on DU_ERRORCORRECTED
			//
			lpMyChkInf->FixRet = MAKELONG(0,ERETIGN);
			return(lpMyChkInf->FixRet);
		    }
		    if(HIWORD(lpMyChkInf->lParam3) == 21)
		    {
			lpMyChkInf->iErr = IERR_READERR1;
		    } else if(HIWORD(lpMyChkInf->lParam3) == 19) {
			lpMyChkInf->iErr = IERR_WRITEERR1;
		    } else {
			if(lpMyChkInf->lpwddi->TypeFlags & DT_DBLDISK)
			{
			    if((!(lpMyChkInf->MyFixOpt2 & DLGCHK_NOCHKHST)) &&
			       (!(lpMyChkInf->MyFixOpt & (DLGCHK_NOBAD |
							  DLGCHK_NODATA |
							  DLGCHK_NOSYS)  ))    )
			    {
				lpMyChkInf->iErr = IERR_WRITEERR2;
				if(!(LOWORD(lpMyChkInf->lParam3) & RETRY))
				{
				    lpMyChkInf->RWRstsrtCnt++;
				    if(lpMyChkInf->RWRstsrtCnt > 1)
				    {
					goto DontRestrt1;
				    }
				    lpMyChkInf->FixRet = MAKELONG(0,ERETRETRY);
				    goto LogAutoErr;
				}
DontRestrt1:
				;
			    } else {
				if(HIWORD(lpMyChkInf->lParam2) & ERRDATA)
				{
				    lpMyChkInf->iErr = IERR_WRITEERR6;
				} else {
				    lpMyChkInf->iErr = IERR_WRITEERR5;
				}
			    }
			} else {
			    if(!(lpMyChkInf->MyFixOpt & (DLGCHK_NOBAD |
							 DLGCHK_NODATA |
							 DLGCHK_NOSYS)	))
			    {
				lpMyChkInf->iErr = IERR_WRITEERR2;
				if(!(LOWORD(lpMyChkInf->lParam3) & RETRY))
				{
				    lpMyChkInf->RWRstsrtCnt++;
				    if(lpMyChkInf->RWRstsrtCnt > 1)
				    {
					goto DontRestrt2;
				    }
				    lpMyChkInf->FixRet = MAKELONG(0,ERETRETRY);
				    goto LogAutoErr;
				}
DontRestrt2:
				;
			    } else {
				if(HIWORD(lpMyChkInf->lParam2) & ERRDATA)
				{
				    lpMyChkInf->iErr = IERR_WRITEERR4;
				} else {
				    lpMyChkInf->iErr = IERR_WRITEERR3;
				}
			    }
			}
		    }
		    goto DoNFErrFIgn;
		    break;

		case READERROR:
		    if(HIWORD(lpMyChkInf->lParam3) == 21)
		    {
			lpMyChkInf->iErr = IERR_READERR1;
		    } else {
			if(lpMyChkInf->lpwddi->TypeFlags & DT_DBLDISK)
			{
			    if((!(lpMyChkInf->MyFixOpt2 & DLGCHK_NOCHKHST)) &&
			       (!(lpMyChkInf->MyFixOpt & (DLGCHK_NOBAD |
							  DLGCHK_NODATA |
							  DLGCHK_NOSYS)  ))    )
			    {
				lpMyChkInf->iErr = IERR_READERR2;
				if(!(LOWORD(lpMyChkInf->lParam3) & RETRY))
				{
				    lpMyChkInf->RWRstsrtCnt++;
				    if(lpMyChkInf->RWRstsrtCnt > 1)
				    {
					goto DontRestrt3;
				    }
				    lpMyChkInf->FixRet = MAKELONG(0,ERETRETRY);
				    goto LogAutoErr;
				}
DontRestrt3:
				;
			    } else {
				if(HIWORD(lpMyChkInf->lParam2) & ERRDATA)
				{
				    lpMyChkInf->iErr = IERR_READERR6;
				} else {
				    lpMyChkInf->iErr = IERR_READERR5;
				}
			    }
			} else {
			    if(!(lpMyChkInf->MyFixOpt & (DLGCHK_NOBAD |
							 DLGCHK_NODATA |
							 DLGCHK_NOSYS)	))
			    {
				lpMyChkInf->iErr = IERR_READERR2;
				if(!(LOWORD(lpMyChkInf->lParam3) & RETRY))
				{
				    lpMyChkInf->RWRstsrtCnt++;
				    if(lpMyChkInf->RWRstsrtCnt > 1)
				    {
					goto DontRestrt4;
				    }
				    lpMyChkInf->FixRet = MAKELONG(0,ERETRETRY);
				    goto LogAutoErr;
				}
DontRestrt4:
				;
			    } else {
				if(HIWORD(lpMyChkInf->lParam2) & ERRDATA)
				{
				    lpMyChkInf->iErr = IERR_READERR4;
				} else {
				    lpMyChkInf->iErr = IERR_READERR3;
				}
			    }
			}
		    }
		    goto DoNFErrFIgn;
		    break;

		case MEMORYERROR:
		    if(HIWORD(lpMyChkInf->lParam2) & LOCMEM)
		    {
			lpMyChkInf->UseAltDlgTxt = TRUE;
		    }
		    lpMyChkInf->iErr = IERR_ERRMEM;
DoNFErrFIgn:
		    if(lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND))
		    {
			lpMyChkInf->FixRet = MAKELONG(0,ERETIGN);
			goto LogAutoErr;
		    }
		    // This error is displayed even in non interactive
		    // mode.

		    goto DoNFErr;
		    break;

		case FATERRXLNK:
		    if(lpMyChkInf->MyFixOpt & DLGCHK_XLDEL)
		    {
			lpMyChkInf->UseAltDefBut = TRUE;
			lpMyChkInf->AltDefButIndx = 1;
		    } else if(!(lpMyChkInf->MyFixOpt & DLGCHK_XLCPY)) {
			lpMyChkInf->UseAltDefBut = TRUE;
			lpMyChkInf->AltDefButIndx = 5;
		    }

		    lpMyChkInf->iErr = IERR_FATXLNK;
DoXLDlg:
		    lpfnDlgProc = SEXLDlgProc;
		    i		= IDD_XL_DLG;

		    if((!(lpMyChkInf->MyFixOpt & DLGCHK_INTER)) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			if(lpMyChkInf->MyFixOpt & DLGCHK_XLCPY)
			{
			    lpMyChkInf->FixRet = MAKELONG(0,ERETMKCPY);
			} else if(lpMyChkInf->MyFixOpt & DLGCHK_XLDEL) {
			    lpMyChkInf->FixRet = MAKELONG(0,ERETDELALL);
			} else {
			    lpMyChkInf->FixRet = MAKELONG(0,ERETIGN);
			}
			goto LogAutoErr;
		    }
DoErrDlg:
		    SEAddErrToLog(lpMyChkInf);
		    if(lpfnDlgProc == 0L)
		    {
NoMem:
			SEAddToLogRCS(lpMyChkInf,IDL_NOMEMCAN,NULL);
			MyChkdskMessageBox(lpMyChkInf, IDS_NOMEM2,
					   MB_ICONINFORMATION | MB_OK);
			return(MAKELONG(0,ERETCAN));
		    }
		    i = DialogBoxParam(g_hInstance,
				       MAKEINTRESOURCE(i),
				       lpMyChkInf->hProgDlgWnd,
				       lpfnDlgProc,
				       (LPARAM)lpMyChkInf);
#ifdef FROSTING
DontDoErrDlg:
#endif
		    if(pMsgBuf)
			LocalFree((HANDLE)pMsgBuf);
		    pMsgBuf = 0;
		    if(i == 0xFFFF)
		    {
			goto NoMem;
		    }
		    if(HIWORD(lpMyChkInf->FixRet) == RESTARTWITHCH)
		    {
			lpMyChkInf->DoCHRestart = TRUE;
			lpMyChkInf->FixRet = MAKELONG(0,ERETCAN);
		    }
		    if(HIWORD(lpMyChkInf->FixRet) == RESTARTWITHSA)
		    {
			lpMyChkInf->DoSARestart = TRUE;
			lpMyChkInf->FixRet = MAKELONG(0,ERETCAN);
		    }
LogErrRet:
		    SEAddRetToLog(lpMyChkInf);
		    return(lpMyChkInf->FixRet);
		    break;

		case FATERRRESVAL:
		    if(lpMyChkInf->lParam3 != 0L)
		    {
			lpMyChkInf->UseAltDlgTxt = TRUE;
		    }
		    lpMyChkInf->iErr = IERR_FATRESVAL;
DoNFErrFAfx:
		    if((!(lpMyChkInf->MyFixOpt & DLGCHK_INTER)) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			lpMyChkInf->FixRet = MAKELONG(0,ERETAFIX);
			goto LogAutoErr;
		    }
DoNFErr:
		    lpfnDlgProc = SEDlgProc;
		    i = IDD_SE_DLG;
		    goto DoErrDlg;
		    break;

		case FATERRMISMAT:
		    lpMyChkInf->iErr = IERR_FATFMISMAT;
		    goto DoNFErrFAfx;
		    break;

		case FATERRLSTCLUS:
		    lpMyChkInf->rgdwArgs[0]=((LPFATLOSTCLUSERR)lpMyChkInf->lParam3)->LostClusCnt * lpMyChkInf->ChkDPms.drvprm.FatFS.SecPerClus * lpMyChkInf->ChkDPms.drvprm.FatFS.BytPerSec;
		    lpMyChkInf->rgdwArgs[1]=((LPFATLOSTCLUSERR)lpMyChkInf->lParam3)->LostClusChainCnt;
		    lpMyChkInf->MIrgdwArgs[0]=((LPFATLOSTCLUSERR)lpMyChkInf->lParam3)->LostClusCnt;
		    lpMyChkInf->MIrgdwArgs[1]=((LPFATLOSTCLUSERR)lpMyChkInf->lParam3)->LostClusChainCnt;
		    lpMyChkInf->iErr = IERR_FATLSTCLUS;
DoLstErr:
		    if(lpMyChkInf->MyFixOpt & DLGCHK_LSTMF)
		    {
			lpMyChkInf->UseAltDefBut = TRUE;
			lpMyChkInf->AltDefButIndx = 1;
		    }
		    if((!(lpMyChkInf->MyFixOpt & DLGCHK_INTER)) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			if(lpMyChkInf->MyFixOpt & DLGCHK_LSTMF)
			{
			    lpMyChkInf->FixRet = MAKELONG(0,ERETMKFILS);
			} else {
			    lpMyChkInf->FixRet = MAKELONG(0,ERETFREE);
			}
			goto LogAutoErr;
		    }
		    goto DoNFErr;
		    break;

		case FATERRFILE:
		    if(((LPFATFILEERR)lpMyChkInf->lParam3)->FileAttribute & 0x10)
		    {
			i = IDS_DIR;
			j = IDS_DIRS;
			lpMyChkInf->IsFolder = TRUE;
		    } else {
			i = IDS_FILEM;
			j = IDS_FILEMS;
		    }
		    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ4);
		    if(!pMsgBuf)
		    {
			goto NoMem;
		    }

		    OemToAnsi((((LPFATFILEERR)lpMyChkInf->lParam3)->lpShortFileName),dBuf3);

		    LoadString(g_hInstance, i, dBuf1, SZBUFA4);
		    LoadString(g_hInstance, j, dBuf2, SZBUFB4);

		    lpMyChkInf->rgdwArgs[0]=(DWORD)(((LPFATFILEERR)(lpMyChkInf->lParam3))->lpDirName);
		    if(((LPFATFILEERR)lpMyChkInf->lParam3)->lpLFNFileName == NULL)
			lpMyChkInf->rgdwArgs[1]=(DWORD)(LPSTR)dBuf3;
		    else
			lpMyChkInf->rgdwArgs[1]=(DWORD)(((LPFATFILEERR)lpMyChkInf->lParam3)->lpLFNFileName);
		    lpMyChkInf->rgdwArgs[2]=(DWORD)(LPSTR)dBuf1;
		    lpMyChkInf->rgdwArgs[3]=(DWORD)(LPSTR)dBuf3;
		    lpMyChkInf->rgdwArgs[4]=(DWORD)(LPSTR)dBuf2;

		    for(i = 0; i < MAXMULTSTRNGS; i++)
		    {
			lpMyChkInf->MErgdwArgs[i][0] = (DWORD)(LPSTR)dBuf2;
			lpMyChkInf->MErgdwArgs[i][1] = (DWORD)(LPSTR)dBuf1;
			lpMyChkInf->MErgdwArgs[i][2] = (DWORD)(LPSTR)dBuf3;
			if(((LPFATFILEERR)lpMyChkInf->lParam3)->lpLFNFileName == NULL)
			    lpMyChkInf->MErgdwArgs[i][3]=(DWORD)(LPSTR)dBuf3;
			else
			    lpMyChkInf->MErgdwArgs[i][3]=(DWORD)(((LPFATFILEERR)lpMyChkInf->lParam3)->lpLFNFileName);
		    }
		    i = 0;
		    j = 0;
		    if(HIWORD(lpMyChkInf->lParam2) & ERRINVNM)
		    {
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_INVNM1;
			i++;
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_INVNM2;
			i++;
			lpMyChkInf->MltELogStrings[j] = IDL_FATERRFILE_INVNM;
			j++;
		    }
		    if(HIWORD(lpMyChkInf->lParam2) & ERRINVLFN)
		    {
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_INVLFN1;
			i++;
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_INVLFN2;
			i++;
			lpMyChkInf->MltELogStrings[j] = IDL_FATERRFILE_INVLFN;
			j++;
		    }
		    if(HIWORD(lpMyChkInf->lParam2) & ERRLFNSTR)
		    {
			lpMyChkInf->UseAltDlgTxt = TRUE;

			// we now have to go back and do ALT form of
			// ERRINVNM and ERRINVLFN. NOTE that the LOG does
			// not have an ALT form.

			i = 0;
			if(HIWORD(lpMyChkInf->lParam2) & ERRINVNM)
			{
			    lpMyChkInf->MltEStrings[i] = ALTISTR_FATERRFILE_INVNM1;
			    i++;
			    lpMyChkInf->MltEStrings[i] = ALTISTR_FATERRFILE_INVNM2;
			    i++;
			}
			if(HIWORD(lpMyChkInf->lParam2) & ERRINVLFN)
			{
			    lpMyChkInf->MltEStrings[i] = ALTISTR_FATERRFILE_INVLFN1;
			    i++;
			    lpMyChkInf->MltEStrings[i] = ALTISTR_FATERRFILE_INVLFN2;
			    i++;
			}
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_LFNSTR1;
			i++;
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_LFNSTR2;
			i++;
			lpMyChkInf->MltELogStrings[j] = IDL_FATERRFILE_LFNSTR;
			j++;
		    }
		    if(HIWORD(lpMyChkInf->lParam2) & ERRLFNLEN)
		    {
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_LFNLEN;
			i++;
			lpMyChkInf->MltELogStrings[j] = IDL_FATERRFILE_LFNLEN;
			j++;
		    }
		    if(HIWORD(lpMyChkInf->lParam2) & ERRDEVNM)
		    {
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_DEVNM1;
			i++;
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_DEVNM2;
			i++;
			lpMyChkInf->MltELogStrings[j] = IDL_FATERRFILE_DEVNM;
			j++;
		    }
		    if(HIWORD(lpMyChkInf->lParam2) & ERRSIZE)
		    {
			if(lpMyChkInf->IsFolder)
			{
#ifdef OPK2
			    if(HIWORD(lpMyChkInf->lParam2) & ERRCHNLEN)
			    {
				lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_SIZEDTB1;
				i++;
				lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_SIZEDTB2;
				i++;
				lpMyChkInf->MltELogStrings[j] = IDL_FATERRFILE_SIZETBD;
				j++;
			    } else {
#endif
				lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_SIZED1;
				i++;
				lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_SIZED2;
				i++;
				lpMyChkInf->MltELogStrings[j] = IDL_FATERRFILE_SIZED;
				j++;
#ifdef OPK2
			    }
#endif
			} else {
#ifdef OPK2
			    if(HIWORD(lpMyChkInf->lParam2) & ERRCHNLEN)
			    {
				lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_SIZETB1;
				i++;
				lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_SIZETB2;
				i++;
				lpMyChkInf->MltELogStrings[j] = IDL_FATERRFILE_SIZETB;
				j++;
			    } else {
#endif
				lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_SIZE1;
				i++;
				lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_SIZE2;
				i++;
				lpMyChkInf->MltELogStrings[j] = IDL_FATERRFILE_SIZE;
				j++;
#ifdef OPK2
			    }
#endif
			}
		    }
		    if(HIWORD(lpMyChkInf->lParam2) & ERRDTTM1)
		    {
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_DTTM11;
			i++;
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_DTTM12;
			i++;
			lpMyChkInf->MltELogStrings[j] = IDL_FATERRFILE_DTTM1;
			j++;
		    }
		    if(HIWORD(lpMyChkInf->lParam2) & ERRDTTM2)
		    {
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_DTTM21;
			i++;
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_DTTM22;
			i++;
			lpMyChkInf->MltELogStrings[j] = IDL_FATERRFILE_DTTM2;
			j++;
		    }
		    if(HIWORD(lpMyChkInf->lParam2) & ERRDTTM3)
		    {
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_DTTM31;
			i++;
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRFILE_DTTM32;
			i++;
			lpMyChkInf->MltELogStrings[j] = IDL_FATERRFILE_DTTM3;
			j++;
		    }
		    for(;i < MAXMULTSTRNGS;i++)
			lpMyChkInf->MltEStrings[i] = 0;
		    for(;j < MAXMULTSTRNGS;j++)
			lpMyChkInf->MltELogStrings[j] = 0;
		    lpMyChkInf->iErr = IERR_FATERRFILE;
		    goto DoNFErrFAfx;
		    break;

		case FATERRCIRCC:
		    lpMyChkInf->iErr = IERR_FATCIRCC;
DoFFErr:
		    if(!pMsgBuf)
		    {
		       pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ4);
		       if(!pMsgBuf)
		       {
			   goto NoMem;
		       }
		    }
		    OemToAnsi((((LPFATFILEERR)lpMyChkInf->lParam3)->lpShortFileName),dBuf3);
		    if(((LPFATFILEERR)lpMyChkInf->lParam3)->FileAttribute & 0x10)
			lpMyChkInf->IsFolder = TRUE;
		    if(((LPFATFILEERR)lpMyChkInf->lParam3)->lpLFNFileName == NULL)
			lpMyChkInf->rgdwArgs[0]=(DWORD)(LPSTR)dBuf3;
		    else
			lpMyChkInf->rgdwArgs[0]=(DWORD)(((LPFATFILEERR)lpMyChkInf->lParam3)->lpLFNFileName);
		    lpMyChkInf->rgdwArgs[1]=(DWORD)(((LPFATFILEERR)lpMyChkInf->lParam3)->lpDirName);
		    goto DoNFErrFAfx;
		    break;

		case FATERRINVCLUS:
		    lpMyChkInf->iErr = IERR_FATINVCLUS;
		    goto DoFFErr;
		    break;

		case FATERRVOLLAB:
		    if(!(HIWORD(lpMyChkInf->lParam2) & ISFRST))
		    {
			lpMyChkInf->UseAltDlgTxt = TRUE;

			i = j = 0;
			while(((LPFATFILEERR)lpMyChkInf->lParam3)->lpShortFileName[i] != '\0')
			{
			    if(((LPFATFILEERR)lpMyChkInf->lParam3)->lpShortFileName[i] != '.')
			    {
				LabBuf[j] = ((LPFATFILEERR)lpMyChkInf->lParam3)->lpShortFileName[i];
				j++;
			    }
			    i++;
			}
			LabBuf[j] = '\0';
			OemToAnsi(LabBuf,LabBuf);
		    } else {
			OemToAnsi((((LPFATFILEERR)lpMyChkInf->lParam3)->lpShortFileName),LabBuf);
		    }
		    lpMyChkInf->rgdwArgs[0]=(DWORD)(LPSTR)LabBuf;
		    lpMyChkInf->rgdwArgs[1]=(DWORD)(((LPFATFILEERR)(lpMyChkInf->lParam3))->lpDirName);
		    lpMyChkInf->iErr = IERR_FATERRVOLLAB;
		    goto DoNFErrFAfx;
		    break;

		case FATERRDIR:
		    i = 0;
		    j = 0;
		    if(HIWORD(lpMyChkInf->lParam2) & ERRZRLEN)
		    {
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRDIR_ZRLEN;
			i++;
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRDIR_ZRLENC;
			i++;
			lpMyChkInf->MltELogStrings[j] = IDL_FATERRDIR_ZRLEN;
			j++;
		    } else if(HIWORD(lpMyChkInf->lParam2) & ERRBAD) {
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRDIR_BAD;
			i++;
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRDIR_BADC;
			i++;
			lpMyChkInf->MltELogStrings[j] = IDL_FATERRDIR_BAD;
			j++;
		    } else {
			if(HIWORD(lpMyChkInf->lParam2) & ERRPNOTD)
			{
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRDIR_PNOTD;
			    i++;
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRDIR_PNOTDC;
			    i++;
			    lpMyChkInf->MltELogStrings[j] = IDL_FATERRDIR_PNOTD;
			    j++;
			}
			if(HIWORD(lpMyChkInf->lParam2) & ERRBADENTS)
			{
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRDIR_BDENTS;
			    i++;
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRDIR_BDENTSC;
			    i++;
			    lpMyChkInf->MltELogStrings[j] = IDL_FATERRDIR_BDENTS;
			    j++;
			}
			if(HIWORD(lpMyChkInf->lParam2) & ERRDOTS)
			{
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRDIR_DOTS;
			    i++;
			    lpMyChkInf->MltELogStrings[j] = IDL_FATERRDIR_DOTS;
			    j++;
			}
			if(HIWORD(lpMyChkInf->lParam2) & ERRDUPNM)
			{
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRDIR_DUPNM;
			    i++;
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRDIR_DUPNMC;
			    i++;
			    lpMyChkInf->MltELogStrings[j] = IDL_FATERRDIR_DUPNM;
			    j++;
			}
			// if(HIWORD(lpMyChkInf->lParam2) & ERRLFNSRT)
			// {
			// }
			if(HIWORD(lpMyChkInf->lParam2) & ERRLOSTFILE)
			{
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRDIR_LOSTFIL;
			    i++;
			    lpMyChkInf->MltELogStrings[j] = IDL_FATERRDIR_LOSTFIL;
			    j++;
			}
			if(HIWORD(lpMyChkInf->lParam2) & ERRLFNLST)
			{
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRDIR_LFNLST;
			    i++;
			    lpMyChkInf->MltELogStrings[j] = IDL_FATERRDIR_LFNLST;
			    j++;
			}
		    }
		    for(;i < MAXMULTSTRNGS;i++)
			lpMyChkInf->MltEStrings[i] = 0;
		    for(;j < MAXMULTSTRNGS;j++)
			lpMyChkInf->MltELogStrings[j] = 0;

		    lpMyChkInf->IsFolder = TRUE;
		    if(((LPFATDIRERR)(lpMyChkInf->lParam3))->DirFirstCluster == 0xFFFFFFFF)
		    {
			lpMyChkInf->IsRootFolder = TRUE;
		    }
		    lpfnDlgProc = SEDlgProc;
		    lpMyChkInf->rgdwArgs[0]=(DWORD)(((LPFATDIRERR)(lpMyChkInf->lParam3))->lpDirName);
		    lpMyChkInf->iErr=IERR_FATERRDIR;
		    goto DoNFErrFAfx;
		    break;
#ifdef OPK2
		case FATERRROOTDIR:
		    i = 0;
		    j = 0;
		    if(HIWORD(lpMyChkInf->lParam2) & ERRINVFC)
		    {
			lpMyChkInf->UseAltDlgTxt = TRUE;
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRRTDIR_INVFC;
			i++;
			lpMyChkInf->MltELogStrings[j] = IDL_FATERRRTDIR_INVFC;
			j++;
			goto ChkLkly;
		    } else if(HIWORD(lpMyChkInf->lParam2) & ERRPNOTD) {
			lpMyChkInf->UseAltDlgTxt = TRUE;
			lpMyChkInf->MltEStrings[i] = ISTR_FATERRRTDIR_PNOTD;
			i++;
			lpMyChkInf->MltELogStrings[j] = IDL_FATERRRTDIR_PNOTD;
			j++;
ChkLkly:
			if(lpMyChkInf->lParam3 == 0xFFFFFFFFL)
			{
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRRTDIR_RECRT;
			    lpMyChkInf->MltELogStrings[j] = IDL_FATERRRTDIR_RECRT;
			} else if(HIWORD(lpMyChkInf->lParam2) & ISLIKELYROOT) {
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRRTDIR_FND;
			    lpMyChkInf->MltELogStrings[j] = IDL_FATERRRTDIR_FND;
			} else {
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRRTDIR_MBYFND;
			    lpMyChkInf->MltELogStrings[j] = IDL_FATERRRTDIR_MBYFND;
			}
			i++;
			j++;
		    } else {
			if(HIWORD(lpMyChkInf->lParam2) & ERRINVC)
			{
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRRTDIR_INVC;
			    i++;
			    lpMyChkInf->MltELogStrings[j] = IDL_FATERRRTDIR_INVC;
			    j++;
			}
			if(HIWORD(lpMyChkInf->lParam2) & ERRCIRCC)
			{
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRRTDIR_CIRCC;
			    i++;
			    lpMyChkInf->MltELogStrings[j] = IDL_FATERRRTDIR_CIRCC;
			    j++;
			}
			if(HIWORD(lpMyChkInf->lParam2) & ERRTOOBIG)
			{
			    lpMyChkInf->MltEStrings[i] = ISTR_FATERRRTDIR_TOOBIG;
			    i++;
			    lpMyChkInf->MltELogStrings[j] = IDL_FATERRRTDIR_TOOBIG;
			    j++;
			}
		    }
		    for(;i < MAXMULTSTRNGS;i++)
			lpMyChkInf->MltEStrings[i] = 0;
		    for(;j < MAXMULTSTRNGS;j++)
			lpMyChkInf->MltELogStrings[j] = 0;

		    lpMyChkInf->IsFolder = TRUE;
		    lpMyChkInf->IsRootFolder = TRUE;
		    lpfnDlgProc = SEDlgProc;
		    LabBuf[0] = LOBYTE(lpMyChkInf->lpwddi->iDrive) + 'A';
		    LabBuf[1] = ':';
		    LabBuf[2] = '\\';
		    LabBuf[3] = '\0';
		    OemToAnsi(LabBuf,LabBuf);
		    lpMyChkInf->rgdwArgs[0]=(DWORD)(LPSTR)LabBuf;
		    lpMyChkInf->iErr=IERR_FATERRROOTDIR;
		    goto DoNFErrFAfx;
		    break;

		case FATERRSHDSURF:
		    lpMyChkInf->iErr = IERR_FATERRSHDSURF;
		    if((!(lpMyChkInf->MyFixOpt & DLGCHK_INTER)) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			lpMyChkInf->FixRet = MAKELONG(0,ERETIGN2);
			goto LogAutoErr;
		    }
		    goto DoNFErr;
		    break;

		case FATERRBOOT:
		    lpMyChkInf->iErr = IERR_FATERRBOOT;
		    goto DoNFErrFAfx;
		    break;
#endif
		case FATERRMXPLEN:
		    if(((LPFATFILEERR)lpMyChkInf->lParam3)->FileAttribute & 0x10)
		    {
			lpMyChkInf->IsFolder = TRUE;
			lpMyChkInf->UseAltDlgTxt = TRUE;
		    }
		    if(!pMsgBuf)
		    {
		       pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ4);
		       if(!pMsgBuf)
		       {
			   goto NoMem;
		       }
		    }
		    OemToAnsi((LPSTR)(((LPFATFILEERR)lpMyChkInf->lParam3)->lParam3),dBuf3);
		    lpMyChkInf->rgdwArgs[0]=(DWORD)(((LPFATFILEERR)lpMyChkInf->lParam3)->lParam2);
		    lpMyChkInf->rgdwArgs[1]=(DWORD)(LPSTR)dBuf3;
		    if(lpMyChkInf->lParam2 & MAKELONG(0,ERRRMWRN))
		    {
			lpMyChkInf->iErr=IERR_FATERRMXPLENS;
#ifdef FROSTING
			if(lpMyChkInf->fSageRun)
			{
			    // ProbCnt is never incremented for these errors, if
			    // the problem is ignored.  So there's no need to
			    // increase SilentProbCnt.
			    // lpMyChkInf->SilentProbCnt++; // ignore problem
			    lpMyChkInf->FixRet = MAKELONG(0,ERETIGN);
			    goto DontDoErrDlg;
			}
#endif
		    } else {
			lpMyChkInf->iErr=IERR_FATERRMXPLENL;
		    }
		    goto DoNFErrFAfx;
		    break;

		case FATERRCDLIMIT:
		    lpMyChkInf->IsFolder = TRUE;
		    if(!pMsgBuf)
		    {
		       pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ4);
		       if(!pMsgBuf)
		       {
			   goto NoMem;
		       }
		    }
		    OemToAnsi((((LPFATDIRERR)(lpMyChkInf->lParam3))->lpDirName),dBuf3);
		    lpMyChkInf->rgdwArgs[0]=(DWORD)(LPSTR)dBuf3;
		    lpMyChkInf->iErr=IERR_FATERRCDLIMIT;

#ifdef FROSTING
		    if(lpMyChkInf->fSageRun)
		    {
			// ProbCnt is never incremented for these errors, if
			// the problem is ignored.  So there's no need to
			// increase SilentProbCnt.
			// lpMyChkInf->SilentProbCnt++; // ignore problem
			lpMyChkInf->FixRet = MAKELONG(0,ERETIGN);
			goto DontDoErrDlg;
		    }
#endif

		    if((!(lpMyChkInf->MyFixOpt & DLGCHK_INTER)) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			if(HIWORD(lpMyChkInf->lParam2) & ERRRMWRN)
			{
			    lpMyChkInf->FixRet = MAKELONG(0,ERETIGN);
			} else {
			    lpMyChkInf->FixRet = MAKELONG(0,ERETMVDIR);
			}
			goto LogAutoErr;
		    } else {
			if(!(HIWORD(lpMyChkInf->lParam2) & ERRRMWRN))
			{
			    lpMyChkInf->UseAltDefBut = TRUE;
			    lpMyChkInf->AltDefButIndx = 1;
			}
		    }
		    goto DoNFErr;
		    break;

#ifdef OPK2
		case ERRISBAD2:
		    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ4);
		    if(!pMsgBuf)
		    {
			goto NoMem;
		    }
		    lpMyChkInf->iErr = IERR_ERRISBAD7;
		    if(!(HIWORD(lpMyChkInf->lParam2) & ERRDATA))
		    {
			lpMyChkInf->UseAltDlgTxt = TRUE;
			if(HIWORD(lpMyChkInf->lParam2) & ERRFBOOT)
			{
			    i = IDS_BOOT;
			} else if(HIWORD(lpMyChkInf->lParam2) & ERRFAT) {
			    i = IDS_FAT;
			} else {
			    i = IDS_ROOTD;
			}
			LoadString(g_hInstance, i, dBuf1, SZBUFA4);
		    }
		    lpMyChkInf->rgdwArgs[0] = lpMyChkInf->lParam3;
		    lpMyChkInf->rgdwArgs[1] = lpMyChkInf->lParam5;
		    lpMyChkInf->rgdwArgs[2] = (DWORD)(LPSTR)dBuf1;
		    goto BadErr;
		    break;
#endif

		case ERRISBAD:
		    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ4);
		    if(!pMsgBuf)
		    {
			goto NoMem;
		    }
		    if((lpMyChkInf->lpwddi->TypeFlags & DT_DBLDISK) &&
		       (lpMyChkInf->MyFixOpt2 & DLGCHK_NOCHKHST)      )
		    {
			if(HIWORD(lpMyChkInf->lParam2) & ERRDATA)
			{
			    lpMyChkInf->iErr = IERR_ERRISBAD2;
			} else {
			    lpMyChkInf->iErr = IERR_ERRISBAD1;
			}
		    } else {
			if(!(HIWORD(lpMyChkInf->lParam2) & ERRDATA))
			{
			    lpMyChkInf->iErr = IERR_ERRISBAD3;
			} else {
			    if(lpMyChkInf->lpwddi->TypeFlags & DT_DBLDISK)
			    {
				lpMyChkInf->iErr = IERR_ERRISBAD4;
			    } else {
				lpMyChkInf->iErr = IERR_ERRISBAD5;
			    }
			}
		    }
		    if((HIWORD(lpMyChkInf->lParam2) & ERRDATA) &&
		       (lpMyChkInf->lParam5 == 0L)		 )
		    {
			lpMyChkInf->iErr = IERR_ERRISBAD6;
		    } else if((HIWORD(lpMyChkInf->lParam2) & ERRDATA) &&
			      (lpMyChkInf->lParam5 != 0L)		) {

			if(HIWORD(lpMyChkInf->lParam2) & ISADIR)
			{
			    lpMyChkInf->IsFolder = TRUE;
			    i = IDS_DIR;
			} else {
			    i = IDS_FILEM;
			}
			LoadString(g_hInstance, i, dBuf1, SZBUFA4);
			if(HIWORD(lpMyChkInf->lParam2) & FULLDISK)
			{
			    lpMyChkInf->UseAltCantFix = TRUE;
			    lpMyChkInf->AltCantFixTstFlag = FULLDISK;
			    lpMyChkInf->AltCantFixHID = IDH_WASTE_FREEING_DISK_SPACE;
			    lpMyChkInf->AltCantFixRepHID = IDH_WINDISK_ISBAD_NO_FREE_CLUSTER;
			}
		    } else if(!(HIWORD(lpMyChkInf->lParam2) & ERRDATA)) {

			if(HIWORD(lpMyChkInf->lParam2) & ERRFBOOT)
			{
			    i = IDS_BOOT;
			} else if(HIWORD(lpMyChkInf->lParam2) & ERRFAT) {
			    i = IDS_FAT;
			} else {
			    i = IDS_ROOTD;
			}
			LoadString(g_hInstance, i, dBuf1, SZBUFA4);
		    }
		    lpMyChkInf->rgdwArgs[0] = lpMyChkInf->lParam3;
		    lpMyChkInf->rgdwArgs[1] = lpMyChkInf->lParam5;
		    lpMyChkInf->rgdwArgs[2] = (DWORD)(LPSTR)dBuf1;
BadErr:
		    if((!(lpMyChkInf->MyFixOpt & DLGCHK_INTER)) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
AutoBad:
			if(!(HIWORD(lpMyChkInf->lParam2) & ERRDATA))
			{
			    // error in system area
			    lpMyChkInf->FixRet = MAKELONG(0,ERETIGN2);
			}
			if(!(HIWORD(lpMyChkInf->lParam2) & RECOV))
			{
			    // Bad sector is uncorrectable (unmovable file)
			    lpMyChkInf->FixRet = MAKELONG(0,ERETIGN2);
			}
			if(LOWORD(lpMyChkInf->lParam2) == ERRISNTBAD)
			{
			    // Do NOT clear bad marks
			    //	except under USER control

			    lpMyChkInf->FixRet = MAKELONG(0,ERETIGN2);
#ifdef OPK2
			} else if(LOWORD(lpMyChkInf->lParam2) == ERRISBAD2) {
			    lpMyChkInf->FixRet = MAKELONG(0,ERETIGN2);
#endif
			} else {
			    lpMyChkInf->FixRet = MAKELONG(0,ERETMRKBAD);
			}
LogAutoErr:
			SEAddErrToLog(lpMyChkInf);
			if(pMsgBuf)
			    LocalFree((HANDLE)pMsgBuf);
			pMsgBuf = 0;
			goto LogErrRet;
		    } else {
			if((LOWORD(lpMyChkInf->lParam2) == ERRISNTBAD) &&
			   (!(lpMyChkInf->MyFixOpt2 & DLGCHK_DOBADISNTBAD)))
			{
			    goto AutoBad;
			}
		    }
		    goto DoNFErr;
		    break;

		case ERRISNTBAD:
		    lpMyChkInf->rgdwArgs[0] = lpMyChkInf->lParam3;
		    lpMyChkInf->iErr = IERR_ERRISNTBAD;
		    goto BadErr;
		    break;

		case ERRCANTDEL:
		    lpMyChkInf->rgdwArgs[0] = lpMyChkInf->lParam3;
		    lpMyChkInf->iErr = IERR_ERRCANTDEL;
DoNFErrFIgn2Int:
		    if((!(lpMyChkInf->MyFixOpt & DLGCHK_INTER)) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			lpMyChkInf->FixRet = MAKELONG(0,ERETIGN2);
			goto LogAutoErr;
		    }
		    goto DoNFErr;
		    break;

		case DDERRMOUNT:
		    lpMyChkInf->CancelIsDefault = TRUE;
		    lpMyChkInf->rgdwArgs[0]=(DWORD)(LPSTR)(lpMyChkInf->lpwddi->driveNameStr);
		    lpMyChkInf->iErr = IERR_DDERRMOUNT;
		    goto DoNFErrFIgn2Int;
		    break;

		case DDERRSIZE1:
		    lpMyChkInf->iErr = IERR_DDERRSIZE1;
DoNFErrFIgn2:
		    if(lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND))
		    {
			lpMyChkInf->FixRet = MAKELONG(0,ERETIGN2);
			goto LogAutoErr;
		    }
		    // This error is displayed even in non interactive
		    // mode.

		    goto DoNFErr;
		    break;

		case DDERRFRAG:
		    lpMyChkInf->iErr = IERR_DDERRFRAG;
		    goto DoNFErrFIgn2;
		    break;

		case DDERRALIGN:
		    lpMyChkInf->iErr = IERR_DDERRALIGN;
		    goto DoNFErrFIgn2;
		    break;

		case DDERRNOXLCHK:
		    lpMyChkInf->iErr = IERR_DDERRNOXLCHK;
#ifdef FROSTING
		    if (lpMyChkInf->fSageRun)
		    {
			lpMyChkInf->SilentProbCnt++; // ignore problem
			lpMyChkInf->FixRet = MAKELONG(0,ERETIGN2);
			goto DontDoErrDlg;
		    }
#endif
		    goto DoNFErrFIgn2Int;
		    break;

		case DDERRUNSUP:
		    lpMyChkInf->iErr = IERR_DDERRUNSUP;
		    dwi = 0x00000001L << lpMyChkInf->lpwddi->iDrive;
#ifdef FROSTING
                    if (lpMyChkInf->fSageRun)
		    {
			lpMyChkInf->SilentProbCnt++; // ignore this problem
			lpMyChkInf->FixRet = MAKELONG(0,ERETIGN2);
			goto DontDoErrDlg;
		    }
#endif
		    if((lpMyChkInf->NoUnsupDrvs & dwi) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			lpMyChkInf->FixRet = MAKELONG(0,ERETIGN2);
			goto LogAutoErr;
		    }
#ifdef OPK2
		    // Do not do this warning again if already done (restarts)

		    if(lpMyChkInf->Done3PtyCompWrn)
		    {
			lpMyChkInf->FixRet = MAKELONG(0,ERETIGN2);
			goto LogAutoErr;
		    } else {
			lpMyChkInf->Done3PtyCompWrn = TRUE;
		    }
#endif
		    // This error is displayed even in non interactive
		    // mode.

		    goto DoNFErr;
		    break;

		case DDERRCVFNM:
		    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ4);
		    if(!pMsgBuf)
		    {
			goto NoMem;
		    }
		    LoadString(g_hInstance, IDS_DBLSPACE, dBuf1, SZBUFA4);
		    LoadString(g_hInstance, IDS_DRVSPACE, dBuf2, SZBUFB4);
		    if(HIWORD(lpMyChkInf->lParam2) & CHNGTONEW)
		    {
			lpMyChkInf->rgdwArgs[0]=(DWORD)((LPSTR)dBuf2);
			lpMyChkInf->rgdwArgs[1]=(DWORD)((LPSTR)dBuf1);
		    } else {
			lpMyChkInf->rgdwArgs[0]=(DWORD)((LPSTR)dBuf1);
			lpMyChkInf->rgdwArgs[1]=(DWORD)((LPSTR)dBuf2);
		    }
		    lpMyChkInf->iErr = IERR_DDERRCVFNM;
		    goto DoNFErrFAfx;
		    break;

		case DDERRSIG:
		    lpMyChkInf->iErr = IERR_DDERRSIG;
		    goto DoNFErrFAfx;
		    break;

		case DDERRBOOT:
		    lpMyChkInf->iErr = IERR_DDERRBOOT;
		    goto DoNFErrFAfx;
		    break;

		case DDERRMDBPB:
		    lpMyChkInf->iErr = IERR_DDERRMDBPB;
		    goto DoNFErrFAfx;
		    break;

		case DDERRSIZE2:
		    if(lpMyChkInf->MyFixOpt2 & DLGCHK_NOCHKHST)
		    {
			lpMyChkInf->iErr = IERR_DDERRSIZE2A;
		    } else {
			lpMyChkInf->iErr = IERR_DDERRSIZE2B;
		    }
		    goto DoNFErrFAfx;
		    break;

		case DDERRMDFAT:
		    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ4);
		    if(!pMsgBuf)
		    {
			goto NoMem;
		    }
		    lpMyChkInf->rgdwArgs[0]=(DWORD)(((LPXLNKFILE)(lpMyChkInf->lParam3))->FileName);
		    if(lpMyChkInf->rgdwArgs[0] == 0L)
		    {
			lpMyChkInf->UseAltDlgTxt = TRUE;
		    } else {
			if(((LPXLNKFILE)(lpMyChkInf->lParam3))->FileAttributes & 0x10)
			{
			    lpMyChkInf->IsFolder = TRUE;
			    i = IDS_DIR;
			} else {
			    i = IDS_FILEM;
			}
			LoadString(g_hInstance, i, dBuf1, SZBUFA4);
			lpMyChkInf->rgdwArgs[1]=(DWORD)(LPSTR)dBuf1;
		    }
		    lpMyChkInf->iErr = IERR_DDERRMDFAT;
		    goto DoNFErrFAfx;
		    break;

		case DDERRLSTSQZ:
		    lpMyChkInf->rgdwArgs[0]=MAKELONG(LOWORD(lpMyChkInf->lParam3),0);
		    lpMyChkInf->iErr = IERR_DDERRLSTSQZ;
		    goto DoLstErr;
		    break;

		case DDERRXLSQZ:
		    lpMyChkInf->UseAltDlgTxt = TRUE;
		    if(lpMyChkInf->MyFixOpt & DLGCHK_XLDEL)
		    {
			lpMyChkInf->UseAltDefBut = TRUE;
			lpMyChkInf->AltDefButIndx = 1;
		    } else if(!(lpMyChkInf->MyFixOpt & DLGCHK_XLCPY)) {
			lpMyChkInf->UseAltDefBut = TRUE;
			lpMyChkInf->AltDefButIndx = 2;
		    }
		    lpMyChkInf->iErr = IERR_DDERRXLSQZ;
		    goto DoXLDlg;
		    break;

		default:
		    SEAddToLogRCS(lpMyChkInf,IDL_ERUNKNO,NULL);
		    return(0L);
		    break;
	    }
	    break;

	case DU_ERRORCORRECTED:
	    if(LOWORD(lpMyChkInf->lParam4) == FULLCORR)
	    {
		SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECFULLCORR);
		goto DoYld;
	    } else if(LOWORD(lpMyChkInf->lParam4) == CANTFIX) {
		SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECCANTFIX);
		goto DoYld;
	    } else if(LOWORD(lpMyChkInf->lParam4) == NOCORR) {
		SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECNOCORR);
	    } else if(LOWORD(lpMyChkInf->lParam4) == PCORROK) {
		SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECPCORROK);
	    } else if(LOWORD(lpMyChkInf->lParam4) == PCORRBAD) {
		SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECPCORRBAD);
	    }

	    if(HIWORD(lpMyChkInf->lParam4) & OTHERWRT)
	    {
		SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECOTHERWRT);
		if(lpMyChkInf->MyFixOpt & DLGCHK_INTER)
		{
		    lpMyChkInf->iErr = IERR_ECORROTHWRT;
		    lpMyChkInf->AlrdyRestartWrn = TRUE;
		    goto DoErr;
		} else {
		    goto DoYld;
		}
	    }

	    if(HIWORD(lpMyChkInf->lParam4) & NOMEM)
	    {
		SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECNOMEM);
		lpMyChkInf->iErr = IERR_ECORRMEM;
		goto DoErr;
	    }
	    if(HIWORD(lpMyChkInf->lParam4) & UNEXP)
	    {
		SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECUNEXP);
		lpMyChkInf->iErr = IERR_ECORRUNEXP;
		goto DoErr;
	    }

	    switch(LOWORD(lpMyChkInf->lParam2)) {

		case FATERRRESVAL:
		case FATERRMISMAT:
		case FATERRCIRCC:
		case FATERRINVCLUS:
		    if(HIWORD(lpMyChkInf->lParam4) & DISKERR)
		    {
			goto DskErr;
		    }
		    goto HmmmErr;
		    break;

		case FATERRCDLIMIT:
		case FATERRMXPLEN:
		    if(HIWORD(lpMyChkInf->lParam4) & DISKERR)
		    {
			goto DskErr;
		    }
		    if(HIWORD(lpMyChkInf->lParam4) & FILCRT)
		    {
			pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ4);
			if(!pMsgBuf)
			{
			    goto NoMem2;
			}
			if(LOWORD(lpMyChkInf->lParam2) == FATERRMXPLEN)
			{
			    OemToAnsi((((LPFATFILEERR)lpMyChkInf->lParam3)->lpShortFileName),dBuf3);
			    if(((LPXLNKFILE)(lpMyChkInf->lParam3))->FileAttributes & 0x10)
			    {
				lpMyChkInf->IsFolder = TRUE;
				i = IDS_DIR;
			    } else {
				i = IDS_FILEM;
			    }
			    if(((LPFATFILEERR)lpMyChkInf->lParam3)->lpLFNFileName == NULL)
				lpMyChkInf->rgdwArgs[1]=(DWORD)(LPSTR)dBuf3;
			    else
				lpMyChkInf->rgdwArgs[1]=(DWORD)(((LPFATFILEERR)lpMyChkInf->lParam3)->lpLFNFileName);
			} else {
			    lpMyChkInf->IsFolder = TRUE;
			    i = IDS_DIR;
			    lpMyChkInf->rgdwArgs[1]=((LPFATDIRERR)(lpMyChkInf->lParam3))->lParam2;
			}
			LoadString(g_hInstance, i, dBuf1, SZBUFA4);
			lpMyChkInf->rgdwArgs[0]=(DWORD)(LPSTR)dBuf1;
			if(HIWORD(lpMyChkInf->lParam4) & FILCOLL)
			{
			    SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECFILCOL);
			    lpMyChkInf->iErr = IERR_ECORRFILCOL;
			} else {
			    SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECFILCRT);
			    lpMyChkInf->iErr = IERR_ECORRFILCRT;
			}
		    } else {
			SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECUNEXP);
			lpMyChkInf->iErr = IERR_ECORRUNEXP;
		    }
		    goto DoErr;
		    break;

#ifdef OPK2
		case FATERRROOTDIR:
#endif
		case FATERRDIR:
		case DDERRXLSQZ:
		case FATERRXLNK:
		    if(HIWORD(lpMyChkInf->lParam4) & CLUSALLO)
		    {
			SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECCLUSA);
			lpMyChkInf->iErr = IERR_ECORRCLUSA;
			goto DoErr;
		    } else if(HIWORD(lpMyChkInf->lParam4) & DISKERR) {
DskErr:
			SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECDISKE);
			lpMyChkInf->iErr = IERR_ECORRDISK;
			if(lpMyChkInf->MyFixOpt & (DLGCHK_NOBAD |
						   DLGCHK_NODATA |
						   DLGCHK_NOSYS)  )
			{
			    lpMyChkInf->UseAltDlgTxt = TRUE;
			}
			goto DoErr;
		    }

		    // NOTE FALL THROUGH

#ifdef OPK2
		case FATERRBOOT:
#endif
		case DDERRCVFNM:
		case DDERRSIZE2:
		case DDERRSIG:
		case DDERRBOOT:
		case DDERRMDBPB:
		case DDERRMDFAT:
		case DDERRLSTSQZ:
		case FATERRVOLLAB:
		case FATERRFILE:
		case FATERRLSTCLUS:
		    if(HIWORD(lpMyChkInf->lParam4) & DISKERR)
		    {
			goto DskErr;
		    }
		    if(HIWORD(lpMyChkInf->lParam4) & FILCRT)
		    {
			lpMyChkInf->UseAltDlgTxt = TRUE;
			if(HIWORD(lpMyChkInf->lParam4) & FILCOLL)
			{
			    SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECFILCOL);
			    lpMyChkInf->iErr = IERR_ECORRFILCOL;
			} else {
			    SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECFILCRT);
			    lpMyChkInf->iErr = IERR_ECORRFILCRT;
			}
		    } else {
HmmmErr:
			SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECUNEXP);
			lpMyChkInf->iErr = IERR_ECORRUNEXP;
		    }
DoErr:
		    if(lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND))
		    {
			if(pMsgBuf)
			    LocalFree((HANDLE)pMsgBuf);
			pMsgBuf = 0;
			goto DoYld;
		    }
		    lpfnDlgProc = SEDlgProc;
		    i = IDD_SE_DLG;
		    i = DialogBoxParam(g_hInstance,
				       MAKEINTRESOURCE(i),
				       lpMyChkInf->hProgDlgWnd,
				       lpfnDlgProc,
				       (LPARAM)lpMyChkInf);
		    if(pMsgBuf)
			LocalFree((HANDLE)pMsgBuf);
		    pMsgBuf = 0;
		    if(i == 0xFFFF)
		    {
NoMem2:
			SEAddToLogRCS(lpMyChkInf,IDL_NOMEMCAN,NULL);
			MyChkdskMessageBox(lpMyChkInf, IDS_NOMEM2,
					   MB_ICONINFORMATION | MB_OK);
			return(1L);
		    }
		    if(HIWORD(lpMyChkInf->FixRet) == ERETCAN)
			return(1L);
		    goto DoYld;
		    break;

		case ERRISBAD:
BadErr2:
		    if(HIWORD(lpMyChkInf->lParam4) & CLUSALLO)
		    {
			SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECCLUSA);
			lpMyChkInf->iErr = IERR_ECORRCLUSA;
			goto DoErr;
		    } else if(HIWORD(lpMyChkInf->lParam4) & DISKERR) {
			SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECDISKE);
			lpMyChkInf->iErr = IERR_ECORRDISK;
			lpMyChkInf->UseAltDlgTxt = TRUE;
			goto DoErr;
		    }
		    goto DoYld;
		    break;

		case ERRISNTBAD:
		    if(lpMyChkInf->MyFixOpt & DLGCHK_INTER)
		    {
			goto BadErr2;
		    }
		    goto DoYld;
		    break;

		case ERRCANTDEL:	// These are all CANTFIX
		case DDERRSIZE1:
		case DDERRMOUNT:
		case DDERRFRAG:
		case DDERRALIGN:
		case DDERRUNSUP:
		case DDERRNOXLCHK:
#ifdef OPK2
		case ERRISBAD2:
#endif
		    SEAddToLogRCS(lpMyChkInf,IDL_ECPRE,IDL_ECCANTFIX);

		    // Note fall through

#ifdef OPK2
		case FATERRSHDSURF:
#endif
		case ERRLOCKV:	    // This will get logged on engine return
		case MEMORYERROR:   // Rest are all "retry ignore or cancel"
		case READERROR:
		case WRITEERROR:
		    goto DoYld;
		    break;

		default:
		    return(0L);
		    break;
	    }
	    goto DoYld;
	    break;

	case DU_OPCOMPLETE:
	    lpMyChkInf->lpFixRep = (LPFATFIXREPORT)lpMyChkInf->lParam2;
	    lpMyChkInf->OpCmpltRet = lpMyChkInf->lParam3;

#ifdef FROSTING
	    if(lpMyChkInf->lpFixRep != 0L)
	    {
		if(lpMyChkInf->lpFixRep->ProbCnt >= lpMyChkInf->SilentProbCnt)
		    lpMyChkInf->lpFixRep->ProbCnt -= lpMyChkInf->SilentProbCnt;
		else
		    lpMyChkInf->lpFixRep->ProbCnt = 0L;
	    }
	    lpMyChkInf->SilentProbCnt = 0;	// Re-init to 0 for next drive
#endif

	    switch(HIWORD(lpMyChkInf->OpCmpltRet))
	    {
		case ERR_FSUNCORRECTED:
		case ERR_FSCORRECTED:
		case NOERROR:
		    if(lpMyChkInf->lpFixRep != 0L)
		    {
			if(lpMyChkInf->lpFixRep->ProbCnt == 0L)
			    i = IDL_NOERROR;
			else if(lpMyChkInf->MyFixOpt & DLGCHK_RO)
			    i = IDL_NONEFIXED;
			else if(lpMyChkInf->lpFixRep->ProbCnt == lpMyChkInf->lpFixRep->ProbFixedCnt)
			    i = IDL_ALLFIXED;
			else
			    i = IDL_SOMEFIXED;
			SEAddToLogRCS(lpMyChkInf,i,NULL);
		    }
		    break;

		// case ERR_OSERR:
		// case ERR_NOTWRITABLE:
		// case ERR_NOTSUPPORTED:
		// case ERR_INSUFMEM:
		// case ERR_EXCLVIOLATION:
		// case ERR_LOCKVIOLATION:
		// case ERR_FSACTIVE:
		// case ERR_FSERR:
		// case ERR_BADOPTIONS:
		// case OPCANCEL:
		default:
		    break;
	    }

	    if(!(lpMyChkInf->MyFixOpt2 & DLGCHK_NOWND))
	    {
		SendMessage(lpMyChkInf->hProgDlgWnd, WM_APP+3, 0, 0L);
	    }
	    return(0L);
	    break;

	case DU_INITENGINE:
	    if(!(lpMyChkInf->MyFixOpt2 & DLGCHK_NOWND))
	    {
		SendMessage(lpMyChkInf->hProgDlgWnd, WM_APP+4, TRUE, 0L);
	    }
	    return(0L);
	    break;

	case DU_ENGINERESTART:
#ifdef FROSTING
	    if(lpMyChkInf->fSageRun)
		LockWrtRestartMax = CHKLOCKRESTARTLIM_SAGE;
	    else
		LockWrtRestartMax = CHKLOCKRESTARTLIM;
#else
#define LockWrtRestartMax CHKLOCKRESTARTLIM
#endif

	    if((HIWORD(lpMyChkInf->lParam2) & OTHERWRT) 	&&
	       (lpMyChkInf->NoRstrtWarn == FALSE)		&&
	       (lpMyChkInf->lpFixFDisp->LockWrtRestartCnt >= LockWrtRestartMax) &&
	       ((lpMyChkInf->lpFixFDisp->LockWrtRestartCnt % LockWrtRestartMax) == 0) )
	    {
		SEAddToLogRCS(lpMyChkInf,IDL_RSTLOCKLIM,NULL);
#ifdef FROSTING
		if(lpMyChkInf->fSageRun)	// If /SAGERUN, just cancel.
		{
		    lpMyChkInf->ChkCancelBool = TRUE;
		    lpMyChkInf->fShouldRerun = TRUE;
		    return(1L);
		}
#endif
		switch(MyChkdskMessageBox(lpMyChkInf, IDS_LOCKRSTART,
					  MB_ICONQUESTION | MB_YESNOCANCEL))
		{
		    case IDNO:
			lpMyChkInf->NoRstrtWarn = TRUE;
			break;

		    case IDCANCEL:
			lpMyChkInf->ChkCancelBool = TRUE;
			return(1L);
			break;

		    case IDYES:
		    default:
			break;
		}
	    }
	    if((HIWORD(lpMyChkInf->lParam2) & UNFIXEDERRS) &&
	       (HIWORD(lpMyChkInf->lParam2) & OTHERWRT)    &&
	       (!lpMyChkInf->AlrdyRestartWrn)		     )
	    {
		SEAddToLogRCS(lpMyChkInf,IDL_RSTUNFERR,NULL);
#ifdef FROSTING
		if(!lpMyChkInf->fSageRun)
		{
#endif
		    switch(MyChkdskMessageBox(lpMyChkInf, IDS_ERRRSTART,
					      MB_ICONQUESTION | MB_OKCANCEL))
		    {
			case IDCANCEL:
			    lpMyChkInf->ChkCancelBool = TRUE;
			    return(1L);
			    break;

			case IDOK:
			default:
			    break;
		    }
#ifdef FROSTING
		}
#endif
	    }
	    lpMyChkInf->AlrdyRestartWrn = FALSE;
#ifdef OPK2
	    if(HIWORD(lpMyChkInf->lParam2) & DOSURFAN)
	    {
		lpMyChkInf->MyFixOpt &= ~DLGCHK_NOBAD;
		SendMessage(GetDlgItem(lpMyChkInf->hProgDlgWnd,DLGCHK_NOBADB),BM_SETCHECK,0,0);
		SendMessage(GetDlgItem(lpMyChkInf->hProgDlgWnd,DLGCHK_DOBAD),BM_SETCHECK,1,0);
		UpdateWindow(GetDlgItem(lpMyChkInf->hProgDlgWnd,DLGCHK_NOBADB));
		UpdateWindow(GetDlgItem(lpMyChkInf->hProgDlgWnd,DLGCHK_DOBAD));
	    }
#endif
	    // NOTE FALL THROUGH

	case DU_YIELD:
	case DU_ENGINESTART:
	case DU_OPUPDATE:
	    if(lpMyChkInf->ChkIsActive)
	    {
		lpMyChkInf->lpFixFDisp->Options &= ~FDO_LOWPRIORITY;
	    } else {
		lpMyChkInf->lpFixFDisp->Options |= FDO_LOWPRIORITY;
	    }
	    if((lpMyChkInf->hTimer == 0)		 &&
	       (!(lpMyChkInf->MyFixOpt2 & DLGCHK_NOWND)) &&
	       (lpMyChkInf->hProgDlgWnd != 0)		    )
	    {
		lpMyChkInf->hTimer = SetTimer(lpMyChkInf->hProgDlgWnd,1,500,NULL);
	    }
	    if(!(lpMyChkInf->MyFixOpt2 & DLGCHK_NOWND))
	    {
		SendMessage(lpMyChkInf->hProgDlgWnd, WM_APP+4, TRUE, 0L);
	    }
DoYld:
#ifdef OPK2
	    if(!(lpMyChkInf->MyFixOpt2 & DLGCHK_NOWND))
	    {
		SendMessage(lpMyChkInf->hProgDlgWnd, WM_APP+5, 0, 0L);
	    }
DoYld2:
#endif

#define YLDCNTDIVH	 25L
#define YLDCNTDIVL	 50L

	    lpMyChkInf->YldCnt++;
	    if(lpMyChkInf->MyFixOpt2 & DLGCHK_NOWND)
	    {
		Yield();
	    } else {
		if(lpMyChkInf->hTimer != 0)
		{
		    if(lpMyChkInf->ChkIsActive)
		    {
			if((lpMyChkInf->YldCnt % YLDCNTDIVH) == 0)
			{
			    goto DoWait;
			}
		    } else {
			if((lpMyChkInf->YldCnt % YLDCNTDIVL) == 0)
			{
DoWait:
			    WaitMessage();
			}
		    }
		}
		PMRet = PeekMessage((LPMSG)&wmsg, NULL, 0, 0, PM_REMOVE);
		if(PMRet)
		{
		    if(lpMyChkInf->ChkIsActive &&
		       (wmsg.message == WM_KEYDOWN))
		    {
			//
			// Because this Message Loop is not USERS dialog
			//  message loop we must do all the keyboard API
			//  mapping ourselves. Mouse works OK.
			//
			// Only thing that's enabled is the CANCEL button,
			//  so all of ESC RETURN and SPACE will cancel.
			//
			if((wmsg.wParam == VK_ESCAPE) ||
			   (wmsg.wParam == VK_SPACE)  ||
			   (wmsg.wParam == VK_RETURN)	 )
			{
			    lpMyChkInf->ChkCancelBool = TRUE;
			    SendDlgItemMessage(lpMyChkInf->hProgDlgWnd,
					       DLGCHK_CANCEL,BM_SETSTATE,
					       TRUE,0L);
			}
		    }
		    TranslateMessage((LPMSG)&wmsg);
		    DispatchMessage((LPMSG)&wmsg);

		    if((wmsg.message == WM_KEYDOWN)    ||
		       (wmsg.message == WM_KEYUP)      ||
		       (wmsg.message == WM_SYSKEYDOWN) ||
		       (wmsg.message == WM_SYSKEYUP)	 )
		    {
			goto DoWait;
		    }
#ifdef OPK2
		    goto DoYld2;
#else
		    goto DoYld;
#endif
		}
	    }
	    if(lpMyChkInf->ChkCancelBool)
		return(1L);
	    else
		return(0L);
	    break;

	default:
	    return(0L);
	    break;

    }
}

#ifdef DOSETUPCHK

LRESULT CALLBACK ChkSetupCBProc(UINT msg, LPARAM lRefData, LPARAM lParam1,
				LPARAM lParam2, LPARAM lParam3,
				LPARAM lParam4, LPARAM lParam5)
{
    LPMYCHKINFOSTRUCT  lpMyChkInf;
    BOOL	       PMRet;
    MSG 	       wmsg;


    lpMyChkInf			  = (LPMYCHKINFOSTRUCT)lRefData;
    lpMyChkInf->lpFixFDisp	  = (LPFIXFATDISP)lParam1;
    lpMyChkInf->lParam1 	  = lParam1;
    lpMyChkInf->lParam2 	  = lParam2;
    lpMyChkInf->lParam3 	  = lParam3;
    lpMyChkInf->lParam4 	  = lParam4;
    lpMyChkInf->lParam5 	  = lParam5;
    lpMyChkInf->IsFolder	  = FALSE;
    lpMyChkInf->IsRootFolder	  = FALSE;
    lpMyChkInf->UseAltDlgTxt	  = FALSE;
    lpMyChkInf->UseAltDefBut	  = FALSE;
    lpMyChkInf->CancelIsDefault   = FALSE;
    lpMyChkInf->AltDefButIndx	  = 0;
    lpMyChkInf->UseAltCantFix	  = FALSE;
    lpMyChkInf->AltCantFixTstFlag = 0;
    lpMyChkInf->AltCantFixHID	  = 0xFFFFFFFFL;
    lpMyChkInf->AltCantFixRepHID  = 0xFFFFFFFFL;

    switch(msg)
    {
	case DU_ERRORDETECTED:
	    switch(LOWORD(lpMyChkInf->lParam2)) {

		case FATERRXLNK:
		    MessageBox(lpMyChkInf->hProgDlgWnd,
			       "Setup Check detected a Cross Link.",
			       "ScanDisk for SETUP",
			       MB_ICONINFORMATION | MB_OK);
		    return(0L);
		    break;

		default:
		    return(0L);
		    break;
	    }
	    break;

	case DU_OPCOMPLETE:
	    return(0L);
	    break;

	case DU_INITENGINE:
	    SendMessage(lpMyChkInf->hProgDlgWnd, WM_APP+4, TRUE, lpMyChkInf->lParam1);
	    return(0L);
	    break;

	case DU_YIELD:
	case DU_OPUPDATE:
	    SendMessage(lpMyChkInf->hProgDlgWnd, WM_APP+4, TRUE, lpMyChkInf->lParam1);
DoYld:
#ifdef OPK2
	    SendMessage(lpMyChkInf->hProgDlgWnd, WM_APP+5, 0, 0L);
#endif
	    PMRet = PeekMessage((LPMSG)&wmsg, NULL, 0, 0, PM_REMOVE);
	    if(PMRet)
	    {
		if(lpMyChkInf->ChkIsActive &&
		   (wmsg.message == WM_KEYDOWN))
		{
		    //
		    // Because this Message Loop is not USERS dialog
		    //	message loop we must do all the keyboard API
		    //	mapping ourselves. Mouse works OK.
		    //
		    // Only thing that's enabled is the CANCEL button,
		    //	so all of ESC RETURN and SPACE will cancel.
		    //
		    if((wmsg.wParam == VK_ESCAPE) ||
		       (wmsg.wParam == VK_SPACE)  ||
		       (wmsg.wParam == VK_RETURN)    )
		    {
			lpMyChkInf->ChkCancelBool = TRUE;
			SendDlgItemMessage(lpMyChkInf->hProgDlgWnd,
					   DLGCHK_CANCEL,BM_SETSTATE,
					   TRUE,0L);
		    }
		}
		TranslateMessage((LPMSG)&wmsg);
		DispatchMessage((LPMSG)&wmsg);

		if((wmsg.message == WM_KEYDOWN)    ||
		   (wmsg.message == WM_KEYUP)	   ||
		   (wmsg.message == WM_SYSKEYDOWN) ||
		   (wmsg.message == WM_SYSKEYUP)     )
		{
		    WaitMessage();
		}
		goto DoYld;
	    }
	    if(lpMyChkInf->ChkCancelBool)
		return(1L);
	    else
		return(0L);
	    break;

	default:
	    return(0L);
	    break;
    }
}

#endif // DOSETUPCHK

BOOL WINAPI ChkSADlgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPMYCHKINFOSTRUCT lpMyChkInf;

    lpMyChkInf = (LPMYCHKINFOSTRUCT)GetWindowLong(hwnd,DWL_USER);

    switch (msg) {

	case  WM_INITDIALOG:
	    SetWindowLong(hwnd,DWL_USER,lParam);
	    lpMyChkInf = (LPMYCHKINFOSTRUCT)lParam;

	    if(lpMyChkInf->MyFixOpt & DLGCHK_NOSYS)
		CheckRadioButton(hwnd, DLGCHKSAO_DOALL, DLGCHKSAO_NODATA, DLGCHKSAO_NOSYS);
	    else if(lpMyChkInf->MyFixOpt & DLGCHK_NODATA)
		CheckRadioButton(hwnd, DLGCHKSAO_DOALL, DLGCHKSAO_NODATA, DLGCHKSAO_NODATA);
	    else
		CheckRadioButton(hwnd, DLGCHKSAO_DOALL, DLGCHKSAO_NODATA, DLGCHKSAO_DOALL);

	    if(lpMyChkInf->MyFixOpt & DLGCHK_NOWRTTST)
		CheckDlgButton(hwnd, DLGCHKSAO_NOWRTTST , TRUE);
	    else
		CheckDlgButton(hwnd, DLGCHKSAO_NOWRTTST , FALSE);

	    if(lpMyChkInf->MyFixOpt & DLGCHK_ALLHIDSYS)
		CheckDlgButton(hwnd, DLGCHKSAO_ALLHIDSYS , TRUE);
	    else
		CheckDlgButton(hwnd, DLGCHKSAO_ALLHIDSYS , FALSE);

	    return(TRUE);
	    break;

	case WM_COMMAND:
	    switch  (wParam) {

		case DLGCHKSAO_OK:
		    if(IsDlgButtonChecked(hwnd, DLGCHKSAO_ALLHIDSYS))
			lpMyChkInf->MyFixOpt |= DLGCHK_ALLHIDSYS;
		    else
			lpMyChkInf->MyFixOpt &= ~DLGCHK_ALLHIDSYS;

		    if(IsDlgButtonChecked(hwnd, DLGCHKSAO_NOWRTTST))
			lpMyChkInf->MyFixOpt |= DLGCHK_NOWRTTST;
		    else
			lpMyChkInf->MyFixOpt &= ~DLGCHK_NOWRTTST;

		    lpMyChkInf->MyFixOpt &= ~(DLGCHK_NOSYS | DLGCHK_NODATA);
		    if(IsDlgButtonChecked(hwnd, DLGCHKSAO_NOSYS))
			lpMyChkInf->MyFixOpt |= DLGCHK_NOSYS;
		    else if(IsDlgButtonChecked(hwnd, DLGCHKSAO_NODATA))
			lpMyChkInf->MyFixOpt |= DLGCHK_NODATA;

		    EndDialog(hwnd, IDOK);
		    return(TRUE);
		    break;

		case DLGCHKSAO_CANCEL:
		    EndDialog(hwnd, IDCANCEL);
		    return(TRUE);
		    break;

		case DLGCHKSAO_DOALL:
		case DLGCHKSAO_NOSYS:
		case DLGCHKSAO_NODATA:
		    CheckRadioButton(hwnd, DLGCHKSAO_DOALL, DLGCHKSAO_NODATA, wParam);
		    return(TRUE);
		    break;

		case DLGCHKSAO_NOWRTTST:
		case DLGCHKSAO_ALLHIDSYS:
		    CheckDlgButton(hwnd, wParam, !IsDlgButtonChecked(hwnd, wParam));
		    return(TRUE);
		    break;

		default:
		    return(FALSE);

	    }
	    break;

	case WM_HELP:
	    WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
		    (DWORD) (LPSTR) ChkSAOaIds);
	    return(TRUE);
	    break;

	case WM_CONTEXTMENU:
	    WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
			    (DWORD) (LPSTR) ChkSAOaIds);
	    return(TRUE);
	    break;

	default:
	    return(FALSE);
	    break;
    }
    return(FALSE);
}

BOOL WINAPI ChkAdvDlgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPMYCHKINFOSTRUCT lpMyChkInf;
    PSTR	      pMsgBuf;

#define SZBUFA2       128
#define SZBUFB2       128
#define SZBUFC2       128
#define TOTMSZ2       (SZBUFA2+SZBUFB2+SZBUFC2)

#define aBuf1 (&(pMsgBuf[0]))
#define aBuf2 (&(pMsgBuf[SZBUFA2]))
#define aBuf3 (&(pMsgBuf[SZBUFA2+SZBUFB2]))

    lpMyChkInf = (LPMYCHKINFOSTRUCT)GetWindowLong(hwnd,DWL_USER);

    switch (msg) {

	case  WM_INITDIALOG:
	    SetWindowLong(hwnd,DWL_USER,lParam);
	    lpMyChkInf = (LPMYCHKINFOSTRUCT)lParam;

	    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ2);
	    if(!pMsgBuf)
	    {
		MyChkdskMessageBox(lpMyChkInf, IDS_NOMEM2,
				   MB_ICONINFORMATION | MB_OK);
		EndDialog(hwnd, IDCANCEL);
		return(TRUE);
	    }

	    if(lpMyChkInf->MyFixOpt & DLGCHK_XLDEL)
		CheckRadioButton(hwnd, DLGCHKADV_XLDEL, DLGCHKADV_XLIGN, DLGCHKADV_XLDEL);
	    else if(lpMyChkInf->MyFixOpt & DLGCHK_XLCPY)
		CheckRadioButton(hwnd, DLGCHKADV_XLDEL, DLGCHKADV_XLIGN, DLGCHKADV_XLCPY);
	    else
		CheckRadioButton(hwnd, DLGCHKADV_XLDEL, DLGCHKADV_XLIGN, DLGCHKADV_XLIGN);

	    if(lpMyChkInf->MyFixOpt & DLGCHK_LSTMF)
		CheckRadioButton(hwnd, DLGCHKADV_LSTF, DLGCHKADV_LSTMF, DLGCHKADV_LSTMF);
	    else
		CheckRadioButton(hwnd, DLGCHKADV_LSTF, DLGCHKADV_LSTMF, DLGCHKADV_LSTF);

	    if(lpMyChkInf->MyFixOpt & DLGCHK_NOCHKDT)
		CheckDlgButton(hwnd, DLGCHKADV_CHKDT , FALSE);
	    else
		CheckDlgButton(hwnd, DLGCHKADV_CHKDT , TRUE);

	    if(lpMyChkInf->MyFixOpt2 & DLGCHK_NOCHKHST)
		CheckDlgButton(hwnd, DLGCHKADV_CHKHST , FALSE);
	    else
		CheckDlgButton(hwnd, DLGCHKADV_CHKHST , TRUE);

	    if(lpMyChkInf->MyFixOpt & DLGCHK_NOCHKNM)
		CheckDlgButton(hwnd, DLGCHKADV_CHKNM , FALSE);
	    else
		CheckDlgButton(hwnd, DLGCHKADV_CHKNM , TRUE);

	    if(lpMyChkInf->MyFixOpt & DLGCHK_REP)
	    {
		if(lpMyChkInf->MyFixOpt2 & DLGCHK_REPONLYERR)
		{
		    CheckRadioButton(hwnd, DLGCHKADV_REPALWAYS, DLGCHKADV_REPIFERR, DLGCHKADV_REPIFERR);
		} else {
		    CheckRadioButton(hwnd, DLGCHKADV_REPALWAYS, DLGCHKADV_REPIFERR, DLGCHKADV_REPALWAYS);
		}
	    } else {
		CheckRadioButton(hwnd, DLGCHKADV_REPALWAYS, DLGCHKADV_REPIFERR, DLGCHKADV_NOREP);
	    }

	    if(lpMyChkInf->MyFixOpt2 & DLGCHK_NOLOG)
	    {
		CheckRadioButton(hwnd, DLGCHKADV_LOGREP, DLGCHKADV_NOLOG, DLGCHKADV_NOLOG);
	    } else {
		if(lpMyChkInf->MyFixOpt2 & DLGCHK_LOGAPPEND)
		{
		    CheckRadioButton(hwnd, DLGCHKADV_LOGREP, DLGCHKADV_NOLOG, DLGCHKADV_LOGAPPND);
		} else {
		    CheckRadioButton(hwnd, DLGCHKADV_LOGREP, DLGCHKADV_NOLOG, DLGCHKADV_LOGREP);
		}
	    }

	    return(TRUE);
	    break;

	case WM_COMMAND:
	    switch  (wParam) {

		case DLGCHKADV_OK:
		    if(IsDlgButtonChecked(hwnd, DLGCHKADV_CHKDT))
			lpMyChkInf->MyFixOpt &= ~DLGCHK_NOCHKDT;
		    else
			lpMyChkInf->MyFixOpt |= DLGCHK_NOCHKDT;

		    if(IsDlgButtonChecked(hwnd, DLGCHKADV_CHKHST))
			lpMyChkInf->MyFixOpt2 &= ~DLGCHK_NOCHKHST;
		    else
			lpMyChkInf->MyFixOpt2 |= DLGCHK_NOCHKHST;

		    if(IsDlgButtonChecked(hwnd, DLGCHKADV_CHKNM))
			lpMyChkInf->MyFixOpt &= ~DLGCHK_NOCHKNM;
		    else
			lpMyChkInf->MyFixOpt |= DLGCHK_NOCHKNM;

		    if(IsDlgButtonChecked(hwnd, DLGCHKADV_LSTMF))
			lpMyChkInf->MyFixOpt |= DLGCHK_LSTMF;
		    else
			lpMyChkInf->MyFixOpt &= ~DLGCHK_LSTMF;

		    lpMyChkInf->MyFixOpt &= ~(DLGCHK_XLDEL | DLGCHK_XLCPY);
		    if(IsDlgButtonChecked(hwnd, DLGCHKADV_XLDEL))
			lpMyChkInf->MyFixOpt |= DLGCHK_XLDEL;
		    else if(IsDlgButtonChecked(hwnd, DLGCHKADV_XLCPY))
			lpMyChkInf->MyFixOpt |= DLGCHK_XLCPY;

		    lpMyChkInf->MyFixOpt2 &= ~(DLGCHK_NOLOG | DLGCHK_LOGAPPEND);
		    if(IsDlgButtonChecked(hwnd, DLGCHKADV_NOLOG))
			lpMyChkInf->MyFixOpt2 |= DLGCHK_NOLOG;
		    else if(IsDlgButtonChecked(hwnd, DLGCHKADV_LOGAPPND))
			lpMyChkInf->MyFixOpt2 |= DLGCHK_LOGAPPEND;

		    lpMyChkInf->MyFixOpt &= ~(DLGCHK_REP);
		    lpMyChkInf->MyFixOpt2 &= ~(DLGCHK_REPONLYERR);
		    if(!IsDlgButtonChecked(hwnd, DLGCHKADV_NOREP))
		    {
			lpMyChkInf->MyFixOpt |= DLGCHK_REP;
			if(IsDlgButtonChecked(hwnd, DLGCHKADV_REPIFERR))
			{
			    lpMyChkInf->MyFixOpt2 |= DLGCHK_REPONLYERR;
			}
		    }
		    EndDialog(hwnd, IDOK);
		    return(TRUE);
		    break;

		case DLGCHKADV_CANCEL:
		    EndDialog(hwnd, IDCANCEL);
		    return(TRUE);
		    break;

		case DLGCHKADV_LSTF:
		case DLGCHKADV_LSTMF:
		    CheckRadioButton(hwnd, DLGCHKADV_LSTF, DLGCHKADV_LSTMF, wParam);
		    return(TRUE);
		    break;

		case DLGCHKADV_XLDEL:
		case DLGCHKADV_XLCPY:
		case DLGCHKADV_XLIGN:
		    CheckRadioButton(hwnd, DLGCHKADV_XLDEL, DLGCHKADV_XLIGN, wParam);
		    return(TRUE);
		    break;

		case DLGCHKADV_LOGREP:
		case DLGCHKADV_LOGAPPND:
		case DLGCHKADV_NOLOG:
		    CheckRadioButton(hwnd, DLGCHKADV_LOGREP, DLGCHKADV_NOLOG, wParam);
		    return(TRUE);
		    break;

		case DLGCHKADV_REPALWAYS:
		case DLGCHKADV_NOREP:
		case DLGCHKADV_REPIFERR:
		    CheckRadioButton(hwnd, DLGCHKADV_REPALWAYS, DLGCHKADV_REPIFERR, wParam);
		    return(TRUE);
		    break;

		case DLGCHKADV_CHKDT:
		case DLGCHKADV_CHKNM:
		case DLGCHKADV_CHKHST:
		    CheckDlgButton(hwnd, wParam, !IsDlgButtonChecked(hwnd, wParam));
		    return(TRUE);
		    break;

		default:
		    return(FALSE);

	    }
	    break;

	case WM_HELP:
	    WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
		    (DWORD) (LPSTR) ChkAdvaIds);
	    return(TRUE);
	    break;

	case WM_CONTEXTMENU:
	    WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
			    (DWORD) (LPSTR) ChkAdvaIds);
	    return(TRUE);
	    break;

	default:
	    return(FALSE);
	    break;
    }
    return(FALSE);
}

#ifdef OPK2

// this right aligns the numbers to dxNum1 width and pretty's it up 
void FAR PASCAL BeutifyNumberOutput(HWND hwnd, HDC hDC,
				    WORD dxNum1, int id, int AltSRcID,
				    DWORD dwInput, DWORD dwInput2)
{
    char szBuf1[128];
    char szBuf2[128];
    char szBuf3[128];
    char szBuf4[128];
    DWORD pdw[2] = { (DWORD)(LPSTR)szBuf2, (DWORD)(LPSTR)szBuf4};
    int j;
    
    if(AltSRcID)
    {
	LoadString(g_hInstance, AltSRcID,szBuf1,sizeof(szBuf1));
    } else {
	GetDlgItemText(hwnd,id,szBuf1,sizeof(szBuf1));
    }
    AddCommas(dwInput,szBuf2,sizeof(szBuf2),FALSE);
    j = lstrlen(szBuf2);
    while(LOWORD(GetTextExtent(hDC,(LPSTR)szBuf2,j)) < (dxNum1 - 2))
    {
        hmemcpy(szBuf2+1, szBuf2, j+1);
        szBuf2[0] = ' ';
        j++;
    }
    
    AddCommas(dwInput2,szBuf4,sizeof(szBuf4),FALSE);
    
    FormatMessage( FORMAT_MESSAGE_FROM_STRING, // | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                  szBuf1, 0, 0, szBuf3, sizeof(szBuf3), pdw);
    //wsprintf(szBuf3,szBuf1,(LPSTR)szBuf2,(LPSTR)szBuf4);
    SetDlgItemText(hwnd,id,szBuf3);
}

void ChkRepInit(LPMYCHKINFOSTRUCT lpMyChkInf, HWND hwnd)
{
    LPFATFIXREPORT    lpChkRep;
    DWORD	      dwi;
    HFONT	      hTxtFnt;
    HFONT	      hFnt;
    HWND	      hCtrl;
    HDC 	      hDC;
    WORD	      dxNum1;
    WORD	      i;
    char	      Buf1[128];
    char	      Buf2[128];
    
    lpChkRep = lpMyChkInf->lpFixRep;

    GetWindowText(hwnd,Buf1,sizeof(Buf1));
    SetStdChkTitle(lpMyChkInf, Buf1, Buf2, sizeof(Buf2));
    SetWindowText(hwnd,Buf2);

    if(lpChkRep->ProbCnt == 0L)
        i = IDS_NOERROR;
    else if(lpMyChkInf->MyFixOpt & DLGCHK_RO)
        i = IDS_NONEFIXED;
    else if(lpChkRep->ProbCnt == lpChkRep->ProbFixedCnt)
        i = IDS_ALLFIXED;
    else
        i = IDS_SOMEFIXED;
    LoadString(g_hInstance, i, Buf2, sizeof(Buf2));
    SetDlgItemText(hwnd,DLGCHKREP_ESTAT,Buf2);

    hCtrl = GetDlgItem(hwnd,DLGCHKREP_TOT);
    hTxtFnt = (HFONT)SendMessage(hCtrl,WM_GETFONT,0,0L);
    if(!hTxtFnt)
        hTxtFnt = GetStockObject(SYSTEM_FONT);
    hDC = GetDC(hCtrl);
    hFnt = SelectObject(hDC,hTxtFnt);
    dxNum1 = LOWORD(GetTextExtent(hDC,(LPSTR)"100,000,000,000",15));

    if((lpChkRep->TotDiskSzByte == 0L) &&
       ((lpChkRep->TotDiskSzK != 0L) || (lpChkRep->TotDiskSzM != 0L)))
    {
	if(lpChkRep->TotDiskSzK == 0L)
	{
	    i = IDS_REP_TOTM;
	    dwi = lpChkRep->TotDiskSzM;
	} else {
	    i = IDS_REP_TOTK;
	    dwi = lpChkRep->TotDiskSzK;
	}
    } else {
	i = 0;
	dwi = lpChkRep->TotDiskSzByte;
    }
    BeutifyNumberOutput(hwnd, hDC, dxNum1, DLGCHKREP_TOT, i, dwi, 0);

    if((lpChkRep->AvailSzByte == 0L) &&
       ((lpChkRep->AvailSzK != 0L) || (lpChkRep->AvailSzM != 0L)))
    {
	if(lpChkRep->AvailSzK == 0L)
	{
	    i = IDS_REP_AVAILM;
	    dwi = lpChkRep->AvailSzM;
	} else {
	    i = IDS_REP_AVAILK;
	    dwi = lpChkRep->AvailSzK;
	}
    } else {
	i = 0;
	dwi = lpChkRep->AvailSzByte;
    }
    BeutifyNumberOutput(hwnd, hDC, dxNum1, DLGCHKREP_AVAIL, i, dwi, 0);

    if((lpChkRep->BadSzDataByte == 0L) &&
       ((lpChkRep->BadSzDataK != 0L) || (lpChkRep->BadSzDataM != 0L)))
    {
	if(lpChkRep->BadSzDataK == 0L)
	{
	    i = IDS_REP_BADM;
	    dwi = lpChkRep->BadSzDataM;
	} else {
	    i = IDS_REP_BADK;
	    dwi = lpChkRep->BadSzDataK;
	}
    } else {
	i = 0;
	dwi = lpChkRep->BadSzDataByte;
    }
    BeutifyNumberOutput(hwnd, hDC, dxNum1, DLGCHKREP_BAD, i, dwi, 0);

    if((lpChkRep->DirSzByte == 0L) &&
       ((lpChkRep->DirSzK != 0L) || (lpChkRep->DirSzM != 0L)))
    {
	if(lpChkRep->DirSzK == 0L)
	{
	    i = IDS_REP_DIRM;
	    dwi = lpChkRep->DirSzM;
	} else {
	    i = IDS_REP_DIRK;
	    dwi = lpChkRep->DirSzK;
	}
    } else {
	i = 0;
	dwi = lpChkRep->DirSzByte;
    }
    BeutifyNumberOutput(hwnd, hDC, dxNum1, DLGCHKREP_DIR, i, dwi, lpChkRep->DirFileCnt);

    if((lpChkRep->UserSzByte == 0L) &&
       ((lpChkRep->UserSzK != 0L) || (lpChkRep->UserSzM != 0L)))
    {
	if(lpChkRep->UserSzK == 0L)
	{
	    i = IDS_REP_USERM;
	    dwi = lpChkRep->UserSzM;
	} else {
	    i = IDS_REP_USERK;
	    dwi = lpChkRep->UserSzK;
	}
    } else {
	i = 0;
	dwi = lpChkRep->UserSzByte;
    }
    BeutifyNumberOutput(hwnd, hDC, dxNum1, DLGCHKREP_USER, i, dwi, lpChkRep->UserFileCnt);

    if((lpChkRep->HidSzByte == 0L) &&
       ((lpChkRep->HidSzK != 0L) || (lpChkRep->HidSzM != 0L)))
    {
	if(lpChkRep->HidSzK == 0L)
	{
	    i = IDS_REP_HIDM;
	    dwi = lpChkRep->HidSzM;
	} else {
	    i = IDS_REP_HIDK;
	    dwi = lpChkRep->HidSzK;
	}
    } else {
	i = 0;
	dwi = lpChkRep->HidSzByte;
    }
    BeutifyNumberOutput(hwnd, hDC, dxNum1, DLGCHKREP_HID, i,dwi, lpChkRep->HidFileCnt);

    BeutifyNumberOutput(hwnd, hDC, dxNum1, DLGCHKREP_BCLUS, 0,lpChkRep->BytesPerClus, 0);
    BeutifyNumberOutput(hwnd, hDC, dxNum1, DLGCHKREP_TCLUS, 0,lpChkRep->TotDataClus, 0);
    BeutifyNumberOutput(hwnd, hDC, dxNum1, DLGCHKREP_ACLUS, 0,lpChkRep->AvailDataClus, 0);

    SelectObject(hDC,hFnt);
    ReleaseDC(hCtrl,hDC);
}
#else
// this right aligns the numbers to dxNum1 width and pretty's it up 
void FAR PASCAL SomeRandomMagic(HWND hwnd, HDC hDC, WORD dxNum1, int id, DWORD dwInput, DWORD dwInput2)
{
    char szBuf1[128];
    char szBuf2[128];
    char szBuf3[128];
    char szBuf4[128];
    DWORD pdw[2] = { (DWORD)(LPSTR)szBuf2, (DWORD)(LPSTR)szBuf4};
    int j;
    
    GetDlgItemText(hwnd,id,szBuf1,sizeof(szBuf1));
    AddCommas(dwInput,szBuf2,sizeof(szBuf2),FALSE);
    j = lstrlen(szBuf2);
    while(LOWORD(GetTextExtent(hDC,(LPSTR)szBuf2,j)) < (dxNum1 - 2))
    {
        hmemcpy(szBuf2+1, szBuf2, j+1);
        szBuf2[0] = ' ';
        j++;
    }
    
    AddCommas(dwInput2,szBuf4,sizeof(szBuf4),FALSE);
    
    FormatMessage( FORMAT_MESSAGE_FROM_STRING, // | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                  szBuf1, 0, 0, szBuf3, sizeof(szBuf3), pdw);
    //wsprintf(szBuf3,szBuf1,(LPSTR)szBuf2,(LPSTR)szBuf4);
    SetDlgItemText(hwnd,id,szBuf3);
}

void ChkRepInit(LPMYCHKINFOSTRUCT lpMyChkInf, HWND hwnd)
{
    LPFATFIXREPORT    lpChkRep;
    HFONT	      hTxtFnt;
    HFONT	      hFnt;
    HWND	      hCtrl;
    HDC 	      hDC;
    WORD	      dxNum1;
    WORD i;
    char Buf1[128];
    char Buf2[128];
    
    lpChkRep = lpMyChkInf->lpFixRep;

    GetWindowText(hwnd,Buf1,sizeof(Buf1));
    SetStdChkTitle(lpMyChkInf, Buf1, Buf2, sizeof(Buf2));
    SetWindowText(hwnd,Buf2);

    if(lpChkRep->ProbCnt == 0L)
        i = IDS_NOERROR;
    else if(lpMyChkInf->MyFixOpt & DLGCHK_RO)
        i = IDS_NONEFIXED;
    else if(lpChkRep->ProbCnt == lpChkRep->ProbFixedCnt)
        i = IDS_ALLFIXED;
    else
        i = IDS_SOMEFIXED;
    LoadString(g_hInstance, i, Buf2, sizeof(Buf2));
    SetDlgItemText(hwnd,DLGCHKREP_ESTAT,Buf2);

    hCtrl = GetDlgItem(hwnd,DLGCHKREP_TOT);
    hTxtFnt = (HFONT)SendMessage(hCtrl,WM_GETFONT,0,0L);
    if(!hTxtFnt)
        hTxtFnt = GetStockObject(SYSTEM_FONT);
    hDC = GetDC(hCtrl);
    hFnt = SelectObject(hDC,hTxtFnt);
    dxNum1 = LOWORD(GetTextExtent(hDC,(LPSTR)"100,000,000,000",15));

    SomeRandomMagic(hwnd, hDC, dxNum1, DLGCHKREP_TOT, lpChkRep->TotDiskSzByte, 0);
    SomeRandomMagic(hwnd, hDC, dxNum1, DLGCHKREP_AVAIL, lpChkRep->AvailSzByte, 0);
    SomeRandomMagic(hwnd, hDC, dxNum1, DLGCHKREP_BAD, lpChkRep->BadSzDataByte, 0);
    SomeRandomMagic(hwnd, hDC, dxNum1, DLGCHKREP_BCLUS, lpChkRep->BytesPerClus, 0);
    SomeRandomMagic(hwnd, hDC, dxNum1, DLGCHKREP_TCLUS, lpChkRep->TotDataClus, 0);
    SomeRandomMagic(hwnd, hDC, dxNum1, DLGCHKREP_ACLUS, lpChkRep->AvailDataClus, 0);
    SomeRandomMagic(hwnd, hDC, dxNum1, DLGCHKREP_DIR, lpChkRep->DirSzByte, lpChkRep->DirFileCnt);
    SomeRandomMagic(hwnd, hDC, dxNum1, DLGCHKREP_USER, lpChkRep->UserSzByte, lpChkRep->UserFileCnt);
    SomeRandomMagic(hwnd, hDC, dxNum1, DLGCHKREP_HID, lpChkRep->HidSzByte, lpChkRep->HidFileCnt);

    SelectObject(hDC,hFnt);
    ReleaseDC(hCtrl,hDC);
}
#endif

BOOL WINAPI ChkRepDlgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {

	case  WM_INITDIALOG:
            ChkRepInit((LPMYCHKINFOSTRUCT)lParam, hwnd);
	    return(TRUE);

	case WM_COMMAND:
	    switch  (wParam) {

		case DLGCHKREP_CLOSE:
		    EndDialog(hwnd, IDOK);
		    return(TRUE);
		    break;

		default:
		    return(FALSE);

	    }
	    break;

	default:
	    return(FALSE);
	    break;
    }
}

#define MAX_DRIVELIST_STRING_LEN	(64+4)
#define MINIDRIVE_MARGIN	4
#define DRIVELIST_BORDER	3

BOOL NEAR MyDrawItem(HWND hwnd, WORD wParam, LPDRAWITEMSTRUCT lpdis)
{
    HDC hdc;
    RECT rc;
    char szText[MAX_DRIVELIST_STRING_LEN];
    int xString, yString, xMiniDrive, dyString;
    SIZE siz;

    if(lpdis->CtlID != DLGCHK_DRVLIST)
	return(FALSE);

    hdc = lpdis->hDC;
    rc = lpdis->rcItem;

    SendMessage(lpdis->hwndItem,LB_GETTEXT,lpdis->itemID,(LPARAM)(LPSTR)szText);

    xMiniDrive = rc.left + DRIVELIST_BORDER;
    rc.left = xString = xMiniDrive + g_cxIcon + MINIDRIVE_MARGIN;
    GetTextExtentPoint(hdc, szText, lstrlen(szText), &siz);

    dyString = siz.cy;
    rc.right = rc.left + siz.cx;
    rc.left--;
    rc.right++;

    if(lpdis->itemAction != ODA_FOCUS)
    {
	yString = rc.top + (rc.bottom - rc.top - dyString)/2;

	SetBkColor(hdc, GetSysColor((lpdis->itemState & ODS_SELECTED) ?
			COLOR_HIGHLIGHT : COLOR_WINDOW));
	SetTextColor(hdc, GetSysColor((lpdis->itemState & ODS_SELECTED) ?
			COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

	ExtTextOut(hdc, xString, yString, ETO_OPAQUE,
		    &rc, szText, lstrlen(szText), NULL);

	ImageList_Draw(g_himlIconsSmall,
		       (int)HIWORD(lpdis->itemData),
                       hdc, xMiniDrive,
		       rc.top + (rc.bottom - rc.top - g_cyIcon)/2,
                       (lpdis->itemState & ODS_SELECTED) ? (ILD_SELECTED | ILD_FOCUS) : ILD_NORMAL);
    }

    if (lpdis->itemAction == ODA_FOCUS ||
	(lpdis->itemState & ODS_FOCUS))
    {
	DrawFocusRect(hdc, &rc);
    }
    return(TRUE);
}

//---------------------------------------------------------------------------
// Returns TRUE if the given string is a UNC path.
//
// TRUE
//	"\\foo\bar"
//	"\\foo"		<- careful
//	"\\"
// FALSE
//	"\foo"
//	"foo"
BOOL NEAR MyPathIsUNC(LPSTR lpsz)
{
    if (lpsz[0] == '\\' && lpsz[1] == '\\')
	return TRUE;
    else
	return FALSE;
}

VOID NEAR MakeThisDriveVisible(HWND hwndDlg, LPMYCHKINFOSTRUCT lpMyChkInf)
{
    HWND  hDrv;
    WORD  ThisDrv;
    int   i;
    int   cnt;

    if(!lpMyChkInf->IsDrvList)
	return;

    hDrv = GetDlgItem(hwndDlg,DLGCHK_DRVLIST);

    if((lpMyChkInf->IsSplitDrv) && (!(lpMyChkInf->DoingCompDrv)))
    {
	ThisDrv = lpMyChkInf->CompDrv;
    } else {
	ThisDrv = lpMyChkInf->lpwddi->iDrive;
    }
    cnt = (int)SendMessage(hDrv,LB_GETCOUNT,0,0L);
    for(i = 0; i < cnt; i++)
    {
	if(LOWORD(SendMessage(hDrv,LB_GETITEMDATA,i,0L)) == ThisDrv)
	    break;
    }
    if(i < cnt)
	SendMessage(hDrv,LB_SETTOPINDEX,i,0L);
    return;
}

BOOL NEAR InitDriveList(HWND hwndDlg, LPSHCHECKDISKINFO  lpSHChkInfo)
{
    SHFILEINFO	      shfi;
    LPMYCHKINFOSTRUCT lpMyChkInf;
    DWORD	      EngFlags[26];
#ifndef FROSTING
    DWORD	      typ;
    DWORD	      sz;
#endif
    int 	      ThisDrv = 0;
#ifndef FROSTING
    HKEY	      hKey;
#endif
    BOOL	      LastDrvValid;
    HWND	      hDrv;
    int 	      i;
    int 	      j;
    int 	      iIndex;
    BOOL	      FndValidDrv = FALSE;
    char	      Drv[] = "A:\\";
#ifndef FROSTING
    char	      RegKey[80];
#endif

    lpMyChkInf = lpSHChkInfo->lpMyChkInf;

    lpSHChkInfo->sDMaint.lpfnGetEngineDriveInfo((LPDWORD)&EngFlags);

#ifdef FROSTING
    lpMyChkInf->HiddenDrives = ChkFindHiddenDrives();
#else
    lpMyChkInf->HiddenDrives = 0L;
    lstrcpy(RegKey, REGSTR_PATH_POLICIES);
    lstrcat(RegKey, "\\Explorer");

    if(RegOpenKey(HKEY_CURRENT_USER,RegKey,&hKey) == ERROR_SUCCESS)
    {
	sz = sizeof(DWORD);
	if((RegQueryValueEx(hKey,"NoDrives",NULL,&typ,(LPBYTE)&(lpMyChkInf->HiddenDrives), &sz) != ERROR_SUCCESS) ||
	   (typ != REG_DWORD)  ||
	   (sz != 4L))
	{
	    lpMyChkInf->HiddenDrives = 0L;
	}
	RegCloseKey(hKey);
    }
#endif
    hDrv = GetDlgItem(hwndDlg,DLGCHK_DRVLIST);
    lpMyChkInf->IsDrvList = FALSE;

    if(!g_himlIconsSmall)
    {
	g_cxIcon = GetSystemMetrics(SM_CXSMICON);
	g_cyIcon = GetSystemMetrics(SM_CYSMICON);
        g_himlIconsSmall = ImageList_Create(g_cxIcon, g_cyIcon, ILC_SHARED|ILC_MASK, 0, 26);
	if(!g_himlIconsSmall)
	    return(FALSE);

	// set the bk colors to COLOR_WINDOW since this is what will
	// be used most of the time as the bk for these lists
	// this avoids having to do ROPs when drawing, thus making it fast

	ImageList_SetBkColor(g_himlIconsSmall, GetSysColor(COLOR_WINDOW));

	for (i = 0; i < 26; i++)
	{
	    if(lpMyChkInf->HiddenDrives & (0x00000001L << i))
		goto SkipDrive;

	    switch(DriveType(i))
	    {
		case DRIVE_REMOVABLE:
		case DRIVE_RAMDRIVE:
		case DRIVE_FIXED:
		    //
		    // Check for special exclusions (JOIN SUBST ASSIGN INTERLINK)
		    //
		    if(!(EngFlags[i] & FS_ISFIXABLE))
			goto InvDrive;
		    Drv[0] = (char)i + 'A';
		    if(!SHGetFileInfo(Drv, 0L, &shfi, sizeof(shfi),
				      SHGFI_ICON |
				      SHGFI_SMALLICON))
		    {
Problem:
			ImageList_Destroy(g_himlIconsSmall);
			g_himlIconsSmall = NULL;
			return(FALSE);
		    }
		    if(shfi.hIcon)
		    {
			j = ImageList_AddIcon(g_himlIconsSmall, shfi.hIcon);
			DestroyIcon(shfi.hIcon);
		    } else {
			goto Problem;
		    }
		    break;

		case DRIVE_REMOTE:
		case DRIVE_CDROM:
		default:		// probably 0, invalid drive
InvDrive:
		    break;
	    }
SkipDrive:
	    ;
	}
    }

    // Make sure the height of list box items is enough to do the ICON

    i = (int)SendMessage(hDrv,LB_GETITEMHEIGHT,0,0L);
    if(i < g_cyIcon)
    {
	SendMessage(hDrv,LB_SETITEMHEIGHT,0,MAKELPARAM(g_cyIcon,0));
    }

    if(lpMyChkInf->DrivesToChk == 0L)
    {
	if(lpMyChkInf->IsSplitDrv)
	{
	    ThisDrv = lpMyChkInf->CompDrv;
	} else {
	    ThisDrv = lpMyChkInf->lpwddi->iDrive;
	}
	iIndex = 0;
	for(i = 0; i < ThisDrv; i++)
	{
	    if(lpMyChkInf->HiddenDrives & (0x00000001L << i))
		goto SkipDrive2;

	    switch(DriveType(i))
	    {
		case DRIVE_REMOVABLE:
		case DRIVE_RAMDRIVE:
		case DRIVE_FIXED:
		    //
		    // Check for special exclusions (JOIN SUBST ASSIGN INTERLINK)
		    //
		    if(!(EngFlags[i] & FS_ISFIXABLE))
			goto InvDrive2;
		    iIndex++;
		    break;

		case DRIVE_REMOTE:
		case DRIVE_CDROM:
		default:		// probably 0, invalid drive
InvDrive2:
		    break;
	    }
SkipDrive2:
	    ;
	}
	goto NextDrv;
    }

    iIndex = 0;
NextDrv:

    if(lpMyChkInf->HiddenDrives & (0x00000001L << ThisDrv))
    {
	LastDrvValid = FALSE;
	goto NextDrv2;
    }

    switch(DriveType(ThisDrv))
    {
	case DRIVE_REMOVABLE:
	case DRIVE_RAMDRIVE:
	case DRIVE_FIXED:
	    //
	    // Check for special exclusions (JOIN SUBST ASSIGN INTERLINK)
	    //
	    if(!(EngFlags[ThisDrv] & FS_ISFIXABLE))
		goto InvDrive3;
	    LastDrvValid = TRUE;
	    break;

	case DRIVE_REMOTE:
	case DRIVE_CDROM:
	default:		// probably 0, invalid drive
InvDrive3:
	    LastDrvValid = FALSE;
	    goto NextDrv2;
	    break;
    }
    FndValidDrv = TRUE;

    // iIndex = the imagelist index for the ICON of this drive, ThisDrv is the
    // 0 based drive number.

    Drv[0] = (char)ThisDrv + 'A';
    if(!SHGetFileInfo(Drv, 0L, &shfi, sizeof(shfi),
		      SHGFI_DISPLAYNAME))

    {
	return(FALSE);
    }

    i = (int)SendMessage(hDrv,LB_GETCOUNT,0,0L);
    SendMessage(hDrv,LB_ADDSTRING,0,(LPARAM)(LPSTR)&(shfi.szDisplayName[0]));
    SendMessage(hDrv,LB_SETITEMDATA,(WPARAM)i,MAKELPARAM(ThisDrv,iIndex));

    if(lpMyChkInf->DrivesToChk == 0L)
    {
SelIt:
	SendMessage(hDrv,LB_SETSEL,TRUE,MAKELPARAM(iIndex,0));
    } else {
	if(lpMyChkInf->DrivesToChk & (0x00000001L << ThisDrv))
	    goto SelIt;
    }

NextDrv2:
    if(lpMyChkInf->DrivesToChk != 0L)
    {
NextDrv2a:
	ThisDrv++;
	if(lpMyChkInf->MyFixOpt & SHCHK_OPT_DRVLISTONLY)
	{
	    if(!(lpMyChkInf->DrivesToChk & (0x00000001L << ThisDrv)))
		goto NextDrv2a;
	}
	if(ThisDrv >= 26)
	{
	    if(FndValidDrv)
	    {
		lpMyChkInf->IsDrvList = TRUE;
		MakeThisDriveVisible(hwndDlg,lpMyChkInf);
		if(lpMyChkInf->MyFixOpt & SHCHK_OPT_DRVLISTONLY)
		    EnableWindow(hDrv,FALSE);
		else
		    EnableWindow(hDrv,TRUE);
	    } else {
		EnableWindow(hDrv,FALSE);
	    }
	    return(FndValidDrv);
	}
	if(LastDrvValid)
	    iIndex++;
	goto NextDrv;
    } else {
	MakeThisDriveVisible(hwndDlg,lpMyChkInf);
	EnableWindow(hDrv,FALSE);
	return(FndValidDrv);
    }
}

#ifdef FROSTING
VOID NEAR SetMultiDrvRslt(LPMYCHKINFOSTRUCT lpMyChkInf)
{
    // Note that the following is irrelevant, in terms of the exit code,
    // if lpMyChkInf->fShouldRerun is TRUE

    if(lpMyChkInf->IsMultiDrv)
    {
	switch (lpMyChkInf->LastChkRslt)
	{
	    case LASTCHKRSLT_SMNOTFIX:
		lpMyChkInf->MultLastChkRslt = lpMyChkInf->LastChkRslt;
		break;

	    case LASTCHKRSLT_ERR:
		if(lpMyChkInf->MultLastChkRslt != LASTCHKRSLT_SMNOTFIX)
		{
		    lpMyChkInf->MultLastChkRslt = lpMyChkInf->LastChkRslt;
		}
		break;

	    case LASTCHKRSLT_ALLFIXED:
		if((lpMyChkInf->MultLastChkRslt != LASTCHKRSLT_SMNOTFIX) &&
		   (lpMyChkInf->MultLastChkRslt != LASTCHKRSLT_ERR)	   )
		{
		    lpMyChkInf->MultLastChkRslt = lpMyChkInf->LastChkRslt;
		}
		break;

	    case LASTCHKRSLT_NOERROR:
		if((lpMyChkInf->MultLastChkRslt != LASTCHKRSLT_SMNOTFIX) &&
		   (lpMyChkInf->MultLastChkRslt != LASTCHKRSLT_ERR)	 &&
		   (lpMyChkInf->MultLastChkRslt != LASTCHKRSLT_ALLFIXED)   )
		{
		    lpMyChkInf->MultLastChkRslt = lpMyChkInf->LastChkRslt;
		}
		break;

	    // case LASTCHKRSLT_CAN:
	    default:
		// Do not change lpMyChkInf->MultLastChkRslt
		break;
	}
    } else {
	lpMyChkInf->MultLastChkRslt = lpMyChkInf->LastChkRslt;
    }
    return;
}
#endif

BOOL WINAPI ChkDlgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPSHCHECKDISKINFO  lpSHChkInfo;
    LPMYCHKINFOSTRUCT  lpMyChkInf;
    DWORD	       dwi;
    DWORD	       dwj;
    HMENU	       hMenu;
    WORD	       i;
    PSTR	       pMsgBuf;
    HWND	       hDrv;
    BOOL	       DoingLBInit;
#ifdef FROSTING
    RECT               rItm, rBar;
    POINT              pt;
    int                dy;	// RECTs are made of ints
#endif

#define SZMSGBUF10     256
#define SZTYPBUF10     120
#define SZTITBUF10     120
#define SZFMTBUF10     128
#define TOTMSZ10       (SZMSGBUF10+SZTYPBUF10+SZTITBUF10+SZFMTBUF10)

#define MsgBuf10  (&(pMsgBuf[0]))
#define TypeBuf10 (&(pMsgBuf[SZMSGBUF10]))
#define TitBuf10  (&(pMsgBuf[SZMSGBUF10+SZTYPBUF10]))
#define FmtBuf10  (&(pMsgBuf[SZMSGBUF10+SZTYPBUF10+SZTITBUF10]))

    lpSHChkInfo = (LPSHCHECKDISKINFO)GetWindowLong(hwnd,DWL_USER);
    if (lpSHChkInfo)
    {
	lpMyChkInf = lpSHChkInfo->lpMyChkInf;
    }

    DoingLBInit = FALSE;

    switch (msg) {

	case  WM_INITDIALOG:
	    DoingLBInit = TRUE;
	    SetWindowLong(hwnd,DWL_USER,lParam);
	    lpSHChkInfo = (LPSHCHECKDISKINFO)lParam;
	    lpMyChkInf = lpSHChkInfo->lpMyChkInf;
	    lpMyChkInf->hProgDlgWnd = hwnd;
	    if(lpMyChkInf->lpTLhwnd)
		*(lpMyChkInf->lpTLhwnd) = hwnd;
#ifdef FROSTING
	    lpMyChkInf->MultLastChkRslt =
#endif
	    lpMyChkInf->LastChkRslt  = LASTCHKRSLT_CAN;
	    lpMyChkInf->HstDrvsChckd = 0L;
	    lpMyChkInf->IsSplitDrv   = FALSE;
	    lpMyChkInf->DoingCompDrv = FALSE;
	    lpMyChkInf->IsFirstDrv   = TRUE;
	    if(lpMyChkInf->hWndPar == 0)
	    {
		lpMyChkInf->hIcon = LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_CHKICON));
		if(lpMyChkInf->hIcon)
		{
		    SendMessage(hwnd,WM_SETICON,1,MAKELONG((WORD)lpMyChkInf->hIcon,0));
		}
		hMenu = GetSystemMenu(hwnd,FALSE);
		if(hMenu)
		{
		    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ10);
		    if(pMsgBuf)
		    {
			LoadString(g_hInstance, IDS_CHELP, FmtBuf10, SZFMTBUF10);
			InsertMenu(hMenu,SC_CLOSE,
				   MF_BYCOMMAND | MF_STRING | MF_ENABLED,
				   DLGCHK_CHELP,FmtBuf10);
			LoadString(g_hInstance, IDS_ABOUT, FmtBuf10, SZFMTBUF10);
			InsertMenu(hMenu,SC_CLOSE,
				   MF_BYCOMMAND | MF_STRING | MF_ENABLED,
				   DLGCHK_ABOUT,FmtBuf10);
			InsertMenu(hMenu,SC_CLOSE,
				   MF_BYCOMMAND | MF_SEPARATOR,
				   NULL,NULL);
			LocalFree((HANDLE)pMsgBuf);
		    }
		    EnableMenuItem(hMenu,SC_SIZE,MF_BYCOMMAND | MF_GRAYED);
		    EnableMenuItem(hMenu,SC_MAXIMIZE,MF_BYCOMMAND | MF_GRAYED);
		}
	    }
#ifdef OPK2
	    lpMyChkInf->hAniIcon1 = (HICON)LOWORD(SendDlgItemMessage(hwnd, IDC_ICON_1, STM_GETIMAGE, IMAGE_ICON, 0L));
	    lpMyChkInf->hCurrAniIcon = lpMyChkInf->hAniIcon1;
	    lpMyChkInf->hAniIcon2 = LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_CHKDLGICN2));
	    lpMyChkInf->hAniIcon3 = LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_CHKDLGICN3));
#endif
	    if(lpMyChkInf->DrivesToChk != 0L)
	    {
		// Set IsMultiDrv flag

		dwi = 0x00000001L;
		dwj = 0L;
		for(i = 0; i < 26; i++)
		{
		    if(lpMyChkInf->DrivesToChk & dwi)
		    {
			dwj++;
		    }
		    dwi = dwi << 1;
		}
		if(dwj > 1)
		    lpMyChkInf->IsMultiDrv = TRUE;
		else
		    lpMyChkInf->IsMultiDrv = FALSE;

		// Set lpMyChkInf->LstChkdDrv to the first drive
		// specified in DrivesToChk

		dwi = 0x00000001L;
		for(i = 0; i < 26; i++)
		{
		    if(lpMyChkInf->DrivesToChk & dwi)
		    {
			lpMyChkInf->LstChkdDrv = i;
			goto InitNext;
		    }
		    dwi = dwi << 1;
		}
NoDrvsToChk:
#ifdef FROSTING
		if(!lpMyChkInf->fSageRun)
		{
#endif
		MyChkdskMessageBox(lpMyChkInf, IDS_CANTCHKALL,
				   MB_ICONINFORMATION | MB_OK);
#ifdef FROSTING
		}
#endif
NoDrvsToChkNN:
		lpMyChkInf->HstDrvsChckd = 0L;
		lpMyChkInf->IsSplitDrv	 = FALSE;
		lpMyChkInf->DoingCompDrv = FALSE;
		EndDialog(hwnd, IDCANCEL);
		return(TRUE);

		// Init lpMyChkInf->lpwddi to the lpMyChkInf->LstChkdDrv drive
InitNext:
		lpMyChkInf->lpwddi->iDrive = lpMyChkInf->LstChkdDrv;
		InitDrvInfo(0, lpMyChkInf->lpwddi, &lpSHChkInfo->sDMaint);
		if(lpMyChkInf->lpwddi->TypeFlags & DT_DBLDISK)
		{
		    lstrcpy(lpMyChkInf->CompdriveNameStr,lpMyChkInf->lpwddi->driveNameStr);
		}
		if((lpMyChkInf->lpwddi->TypeFlags & DT_DBLDISK) &&
		   (!(lpMyChkInf->MyFixOpt2 & DLGCHK_NOCHKHST))   )
		{
		    lpMyChkInf->CompDrv = lpMyChkInf->lpwddi->iDrive;
		    lpMyChkInf->HostDrv = GetCompInfo(lpMyChkInf->CompDrv,&i);
		    if(lpMyChkInf->HostDrv != 0xFFFF)
		    {
			lpMyChkInf->IsSplitDrv	 = TRUE;
			lpMyChkInf->DoingCompDrv = FALSE;
			lpMyChkInf->lpwddi->iDrive = lpMyChkInf->HostDrv;
			InitDrvInfo(0, lpMyChkInf->lpwddi, &lpSHChkInfo->sDMaint);
		    }
		}

		// Init for CHKDSK

		i = lpSHChkInfo->sDMaint.lpfnGetFileSysParameters(lpMyChkInf->lpwddi->iDrive,
						&(lpMyChkInf->ChkDPms),
						sizeof(lpMyChkInf->ChkDPms));
		switch(i) {
		    case FS_FAT:
		    case FS_DDFAT:
		    case FS_LFNFAT:
		    case FS_DDLFNFAT:
			lpMyChkInf->NoParms = FALSE;
			break;

		    default:
			lpMyChkInf->NoParms = TRUE;
CantChkDsk:
			lpMyChkInf->IsSplitDrv = FALSE;

			switch (lpMyChkInf->lpwddi->iType)
			{
			    case DRIVE_REMOVABLE:
#ifdef FROSTING
				if(lpMyChkInf->fSageRun)
				{
				    lpMyChkInf->fShouldRerun = TRUE;
				    goto NoComplainCantChk;
				}
#endif

				i = IDS_CANTCHKR;
				break;

			    case DRIVE_RAMDRIVE:
			    case DRIVE_FIXED:
#ifdef FROSTING
				if(lpMyChkInf->fSageRun)
				{
				    lpMyChkInf->fShouldRerun = TRUE;
				    goto NoComplainCantChk;
				}
#endif
				i = IDS_CANTCHK;
				break;

			    case DRIVE_CDROM:
			    case DRIVE_REMOTE:
			    default:
#ifdef FROSTING
				if (lpMyChkInf->fSageRun)
				{
				    goto NoComplainCantChk;
				}
#endif
				i = IDS_INVALID;
				break;
			}
			MyChkdskMessageBox(lpMyChkInf, i,
					   MB_ICONINFORMATION | MB_OK);
#ifdef FROSTING
NoComplainCantChk:
#endif
			lpMyChkInf->LastChkRslt = LASTCHKRSLT_ERR;
#ifdef FROSTING
			SetMultiDrvRslt(lpMyChkInf);
#endif

			// Set lpMyChkInf->LstChkdDrv to the next drive
			// specified in DrivesToChk

			dwi = 0x00000001L << (lpMyChkInf->LstChkdDrv + 1);
			for(i = lpMyChkInf->LstChkdDrv + 1; i < 26; i++)
			{
			    if(lpMyChkInf->DrivesToChk & dwi)
			    {
				lpMyChkInf->LstChkdDrv = i;
				goto InitNext;
			    }
			    dwi = dwi << 1;
			}
			if(lpMyChkInf->IsMultiDrv)
			    goto NoDrvsToChk;
			else
			    goto NoDrvsToChkNN;

			break;
		}

		dwi = lpSHChkInfo->sDMaint.lpfnGetFixOptions(&(lpMyChkInf->ChkDPms));

		if(dwi & (FSINVALID | FSDISALLOWED))
		    goto CantChkDsk;

		lpMyChkInf->FixOptions = dwi;
	    } else {
		lpMyChkInf->IsMultiDrv = FALSE;
	    }

	    // Init drive list to include all drives and select
	    // the ones specified by DrivesToChk or
	    // init drive list to only include the one drive
	    // specified by lpMyChkInf->lpwddi->iDrive or include
	    // only the drives specified by DrivesToChk

	    if(!InitDriveList(hwnd,lpSHChkInfo))
	    {
NoMem:
		MyChkdskMessageBox(lpMyChkInf, IDS_NOMEM2,
				   MB_ICONINFORMATION | MB_OK);
		EndDialog(hwnd, IDCANCEL);
		return(TRUE);
	    }

	    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ10);
	    if(!pMsgBuf)
	    {
		goto NoMem;
	    }

	    // Init progress bar and status text

#ifdef FROSTING
	    if (lpMyChkInf->fSageSet)
	    {
		ShowWindow(GetDlgItem(hwnd,DLGCHK_PBAR), SW_HIDE);
		GetWindowRect(GetDlgItem(hwnd,DLGCHK_PBAR), &rBar);

		GetWindowRect(GetDlgItem(hwnd,DLGCHK_START), &rItm);
		pt.x = rItm.left;
		pt.y = rBar.top;
		ScreenToClient(hwnd, &pt);
		MoveWindow(GetDlgItem(hwnd,DLGCHK_START), pt.x, pt.y,
			    rItm.right-rItm.left, rItm.bottom-rItm.top, TRUE);

		dy = rItm.top - rBar.top;

		GetWindowRect(GetDlgItem(hwnd,DLGCHK_CANCEL), &rItm);
		pt.x = rItm.left;
		pt.y = rBar.top;
		ScreenToClient(hwnd, &pt);
		MoveWindow(GetDlgItem(hwnd,DLGCHK_CANCEL), pt.x, pt.y,
			    rItm.right-rItm.left, rItm.bottom-rItm.top, TRUE);

		GetWindowRect(GetDlgItem(hwnd,DLGCHK_ADVANCED), &rItm);
		pt.x = rItm.left;
		pt.y = rBar.top;
		ScreenToClient(hwnd,&pt);
		MoveWindow(GetDlgItem(hwnd,DLGCHK_ADVANCED), pt.x, pt.y,
			    rItm.right-rItm.left, rItm.bottom-rItm.top, TRUE);

		LoadString(g_hInstance,IDS_OK,MsgBuf10,SZMSGBUF10);
		SetDlgItemText(hwnd, DLGCHK_START, MsgBuf10);

		LoadString(g_hInstance,IDS_CANCEL,MsgBuf10,SZMSGBUF10);
		SetDlgItemText(hwnd, DLGCHK_CANCEL, MsgBuf10);

		LoadString(g_hInstance,IDS_SAGETITLE,MsgBuf10,SZMSGBUF10);
		SetWindowText(hwnd, MsgBuf10);

		GetWindowRect(hwnd, &rItm);
		SetWindowPos(hwnd, NULL, 0, 0,
			     rItm.right-rItm.left,
		             rItm.bottom-rItm.top -dy,
			     SWP_NOMOVE|SWP_NOACTIVATE);
	    }
#endif
	    SendDlgItemMessage(hwnd, DLGCHK_PBAR, PBM_SETRANGE, 0, MAKELONG(0, 100));
	    SendDlgItemMessage(hwnd, DLGCHK_PBAR, PBM_SETPOS, 0, 0L);

	    SetDlgItemText(hwnd, DLGCHK_STATTXT, g_szNULL);
	    SetDlgItemText(hwnd, DLGCHK_STATTXT2, g_szNULL);
	    lpMyChkInf->pTextIsComplt = FALSE;

	    // turn this style bit, PBS_SHOWPERCENT, on and off to
	    // enable/disable the progress bar.
	    //
	    //	dwi = GetWindowLong(GetDlgItem(hwnd,DLGCHK_PBAR),GWL_STYLE);
	    //	dwi |= PBS_SHOWPERCENT; or dwi &= ~PBS_SHOWPERCENT;
	    //	SetWindowLong(GetDlgItem(hwnd,DLGCHK_PBAR),GWL_STYLE,dwi);
	    //
	    // When off, also do SendDlgItemMessage(..PBM_SETPOS, 0, 0L)
	    // to set the bar to "empty".

	    // Add drive letter and type to title
            SetDriveTitle(lpMyChkInf, hwnd);

	    if(lpMyChkInf->MyFixOpt & DLGCHK_NOBAD)
	    {
		SendMessage(GetDlgItem(hwnd,DLGCHK_DOBAD),BM_SETCHECK,0,0);
		SendMessage(GetDlgItem(hwnd,DLGCHK_NOBADB),BM_SETCHECK,1,0);
		EnableWindow(GetDlgItem(hwnd,DLGCHK_BADOPT),FALSE);
	    } else {
		SendMessage(GetDlgItem(hwnd,DLGCHK_NOBADB),BM_SETCHECK,0,0);
		SendMessage(GetDlgItem(hwnd,DLGCHK_DOBAD),BM_SETCHECK,1,0);
		EnableWindow(GetDlgItem(hwnd,DLGCHK_BADOPT),TRUE);
	    }

	    if(lpMyChkInf->MyFixOpt & DLGCHK_RO)
	    {
		lpMyChkInf->MyFixOpt |= DLGCHK_INTER;
		CheckDlgButton(hwnd, DLGCHK_AUTOFIX, FALSE);
		EnableWindow(GetDlgItem(hwnd,DLGCHK_AUTOFIX),FALSE);
	    } else {
		if(lpMyChkInf->MyFixOpt & DLGCHK_INTER)
		{
		    CheckDlgButton(hwnd, DLGCHK_AUTOFIX, FALSE);
		} else {
		    CheckDlgButton(hwnd, DLGCHK_AUTOFIX, TRUE);
		}
	    }
	    if(lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND))
	    {
		EnableWindow(GetDlgItem(hwnd,DLGCHK_ADVANCED),FALSE);
		if(lpMyChkInf->hWndPar == 0)
		{
		    hMenu = GetSystemMenu(hwnd,FALSE);
		    if(hMenu)
		    {
			EnableMenuItem(hMenu,DLGCHK_CHELP,MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu,DLGCHK_ABOUT,MF_BYCOMMAND | MF_GRAYED);
		    }
		}
		EnableWindow(GetDlgItem(hwnd,DLGCHK_NOBADB),FALSE);
		EnableWindow(GetDlgItem(hwnd,DLGCHK_DOBAD),FALSE);
		EnableWindow(GetDlgItem(hwnd,DLGCHK_DTXT1),FALSE);
		EnableWindow(GetDlgItem(hwnd,DLGCHK_BADOPT),FALSE);
		EnableWindow(GetDlgItem(hwnd,DLGCHK_DTXT2),FALSE);
		EnableWindow(GetDlgItem(hwnd,DLGCHK_AUTOFIX),FALSE);
	    }
	    SetFocus(GetDlgItem(hwnd,DLGCHK_START));
	    if(lpMyChkInf->ShowMinimized)
	    {
		SendMessage(hwnd,WM_SYSCOMMAND,SC_MINIMIZE,0L);
	    }
#ifdef FROSTING
	    if((lpMyChkInf->MyFixOpt & DLGCHK_AUTO) || (lpMyChkInf->fSageRun))
#else
	    if(lpMyChkInf->MyFixOpt & DLGCHK_AUTO)
#endif
	    {
		PostMessage(hwnd,WM_COMMAND,DLGCHK_START,0L);
	    }
	    LocalFree((HANDLE)pMsgBuf);
	    return(FALSE);
	    break;

	case WM_DRAWITEM:
	    return(MyDrawItem(hwnd,wParam,(LPDRAWITEMSTRUCT)lParam));
	    break;

	case WM_ACTIVATE:
	    if(wParam == WA_INACTIVE)
	    {
		lpMyChkInf->ChkIsActive = FALSE;
	    } else {
		lpMyChkInf->ChkIsActive = TRUE;
		InvalidateRect(GetDlgItem(hwnd,DLGCHK_PBAR),NULL,TRUE);

		if((!lpMyChkInf->ChkInProgBool) &&
		   (lpMyChkInf->NoParms)	   )
		{
		    //
		    // We just got re-activated and we are in the "couldn't
		    //	get drive parameters" case. Mr. USER was hopefully
		    //	off fixing things so we could run, try fetching
		    //	drive parms again (silently).
		    //
		    i = lpSHChkInfo->sDMaint.lpfnGetFileSysParameters(lpMyChkInf->lpwddi->iDrive,
						    &(lpMyChkInf->ChkDPms),
						    sizeof(lpMyChkInf->ChkDPms));
		    switch(i) {
			case FS_FAT:
			case FS_DDFAT:
			case FS_LFNFAT:
			case FS_DDLFNFAT:
			    lpMyChkInf->NoParms = FALSE;

			    dwi = lpSHChkInfo->sDMaint.lpfnGetFixOptions(&(lpMyChkInf->ChkDPms));

			    if(dwi & (FSINVALID | FSDISALLOWED))
			    {
				lpMyChkInf->NoParms = TRUE;
			    } else {
				lpMyChkInf->FixOptions = dwi;

                                if (!SetDriveTitle(lpMyChkInf, hwnd)) {
				    lpMyChkInf->NoParms = TRUE;
				}
			    }
			    break;

			default:
			    lpMyChkInf->NoParms = TRUE;
			    break;
		    }
		}
	    }
	    return(FALSE);
	    break;

	case WM_SYSCOMMAND:
	    switch  (wParam) {

		case DLGCHK_CHELP:
		    if((lpMyChkInf->ChkInProgBool) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			return(TRUE);
		    }
		    SendMessage(hwnd,WM_SYSCOMMAND,SC_CONTEXTHELP,0L);
		    return(TRUE);
		    break;

		case DLGCHK_ABOUT:
		    if((lpMyChkInf->ChkInProgBool) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			return(TRUE);
		    }

		    if(!lpMyChkInf->hIcon)
		    {
			lpMyChkInf->hIcon = LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_CHKICON));
		    }

		    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ10);
		    if(pMsgBuf)
		    {
			LoadString(g_hInstance,IDS_CHKTITABOUT,TitBuf10,SZTITBUF10);
			ShellAbout(hwnd,TitBuf10,NULL,lpMyChkInf->hIcon);
			LocalFree((HANDLE)pMsgBuf);
		    } else {
			MyChkdskMessageBox(lpMyChkInf, IDS_NOMEM2,
					   MB_ICONINFORMATION | MB_OK);
		    }
		    return(TRUE);
		    break;

		default:
		    return(FALSE);
	    }
	    break;

	case WM_COMMAND:
	    switch  (wParam) {

		case DLGCHK_DRVLIST:
		    hDrv = (HWND)LOWORD(lParam);
		    switch(HIWORD(lParam))
		    {
			case LBN_SELCHANGE:
			    if(DoingLBInit || (!(lpMyChkInf->IsDrvList)) ||
			       (lpMyChkInf->MyFixOpt & SHCHK_OPT_DRVLISTONLY)  )
			    {
				return(FALSE);
			    }
			    if(lpMyChkInf->pTextIsComplt)
			    {
				SetDlgItemText(hwnd, DLGCHK_STATTXT, g_szNULL);
				SetDlgItemText(hwnd, DLGCHK_STATTXT2, g_szNULL);
				lpMyChkInf->pTextIsComplt = FALSE;
			    }
			    lpMyChkInf->DrivesToChk = 0L;
			    dwi = SendMessage(hDrv,LB_GETCOUNT,0,0L);
			    for(i = 0; i < LOWORD(dwi); i++)
			    {
				dwj = SendMessage(hDrv,LB_GETSEL,i,0L);
				if(dwj && (dwj != LB_ERR))
				{
				    dwj = SendMessage(hDrv,LB_GETITEMDATA,i,0L);
				    if(dwj != LB_ERR)
				    {
					dwj = 0x00000001L << LOWORD(dwj);
					lpMyChkInf->DrivesToChk |= dwj;
				    }
				}
			    }
			    if(lpMyChkInf->DrivesToChk != 0L)
			    {
				dwi = 0x00000001L;
				dwj = 0L;
				for(i = 0; i < 26; i++)
				{
				    if(lpMyChkInf->DrivesToChk & dwi)
				    {
					dwj++;
				    }
				    dwi = dwi << 1;
				}
				if(dwj > 1)
				    lpMyChkInf->IsMultiDrv = TRUE;
				else
				    lpMyChkInf->IsMultiDrv = FALSE;

				dwi = 0x00000001L;
				for(i = 0; i < 26; i++)
				{
				    if(lpMyChkInf->DrivesToChk & dwi)
				    {
					lpMyChkInf->LstChkdDrv = i;
					lpMyChkInf->IsFirstDrv = TRUE;
					goto InitNext3;
				    }
				    dwi = dwi << 1;
				}
NoDrvsToChk3:
#ifdef FROSTING
				if(!lpMyChkInf->fSageRun)
				{
#endif
				MyChkdskMessageBox(lpMyChkInf, IDS_CANTCHKALL,
						   MB_ICONINFORMATION | MB_OK);
#ifdef FROSTING
				}
#endif
NoDrvsToChkNN3:
				LocalFree((HANDLE)pMsgBuf);
				lpMyChkInf->HstDrvsChckd = 0L;
				lpMyChkInf->IsSplitDrv	 = FALSE;
				lpMyChkInf->DoingCompDrv = FALSE;
				return(TRUE);

				// Init lpMyChkInf->lpwddi to the lpMyChkInf->LstChkdDrv drive
InitNext3:
				lpMyChkInf->IsSplitDrv	 = FALSE;
				lpMyChkInf->DoingCompDrv = FALSE;
				lpMyChkInf->HstDrvsChckd = 0L;
				lpMyChkInf->lpwddi->iDrive = lpMyChkInf->LstChkdDrv;
				InitDrvInfo(0, lpMyChkInf->lpwddi, &lpSHChkInfo->sDMaint);
				if(lpMyChkInf->lpwddi->TypeFlags & DT_DBLDISK)
				{
				    lstrcpy(lpMyChkInf->CompdriveNameStr,lpMyChkInf->lpwddi->driveNameStr);
				}
				if((lpMyChkInf->lpwddi->TypeFlags & DT_DBLDISK) &&
				   (!(lpMyChkInf->MyFixOpt2 & DLGCHK_NOCHKHST))   )
				{
				    lpMyChkInf->CompDrv = lpMyChkInf->lpwddi->iDrive;
				    lpMyChkInf->HostDrv = GetCompInfo(lpMyChkInf->CompDrv,&i);
				    if(lpMyChkInf->HostDrv != 0xFFFF)
				    {
					lpMyChkInf->IsSplitDrv	 = TRUE;
					lpMyChkInf->DoingCompDrv = FALSE;
					lpMyChkInf->lpwddi->iDrive = lpMyChkInf->HostDrv;
					InitDrvInfo(0, lpMyChkInf->lpwddi, &lpSHChkInfo->sDMaint);
				    }
				}

				// Init for CHKDSK

				i = lpSHChkInfo->sDMaint.lpfnGetFileSysParameters(lpMyChkInf->lpwddi->iDrive,
								&(lpMyChkInf->ChkDPms),
								sizeof(lpMyChkInf->ChkDPms));
				switch(i) {
				    case FS_FAT:
				    case FS_DDFAT:
				    case FS_LFNFAT:
				    case FS_DDLFNFAT:
					lpMyChkInf->NoParms = FALSE;
					break;

				    default:
					lpMyChkInf->NoParms = TRUE;
CantChkDsk3:
					lpMyChkInf->IsSplitDrv = FALSE;
					switch (lpMyChkInf->lpwddi->iType)
					{
					    case DRIVE_REMOVABLE:
#ifdef FROSTING
						if (lpMyChkInf->fSageRun)
						{
						    lpMyChkInf->fShouldRerun = TRUE;
						    goto CantChkDsk4;
						}
#endif
						i = IDS_CANTCHKR;
						break;

					    case DRIVE_RAMDRIVE:
					    case DRIVE_FIXED:
#ifdef FROSTING
						if (lpMyChkInf->fSageRun)
						{
						    lpMyChkInf->fShouldRerun = TRUE;
						    goto CantChkDsk4;
						}
#endif
						i = IDS_CANTCHK;
						break;

					    case DRIVE_CDROM:
					    case DRIVE_REMOTE:
					    default:
#ifdef FROSTING
						if (lpMyChkInf->fSageRun)
						{
						    goto CantChkDsk4;
						}
#endif
						i = IDS_INVALID;
						break;
					}
					MyChkdskMessageBox(lpMyChkInf, i,
							   MB_ICONINFORMATION | MB_OK);

#ifdef FROSTING
CantChkDsk4:
#endif

					// Set lpMyChkInf->LstChkdDrv to the next drive
					// specified in DrivesToChk

					dwi = 0x00000001L << (lpMyChkInf->LstChkdDrv + 1);
					for(i = lpMyChkInf->LstChkdDrv + 1; i < 26; i++)
					{
					    if(lpMyChkInf->DrivesToChk & dwi)
					    {
						lpMyChkInf->LstChkdDrv = i;
						goto InitNext3;
					    }
					    dwi = dwi << 1;
					}
					if(lpMyChkInf->IsMultiDrv)
					    goto NoDrvsToChk3;
					else
					    goto NoDrvsToChkNN3;
					break;
				}

				dwi = lpSHChkInfo->sDMaint.lpfnGetFixOptions(&(lpMyChkInf->ChkDPms));

				if(dwi & (FSINVALID | FSDISALLOWED))
				    goto CantChkDsk3;

				lpMyChkInf->FixOptions = dwi;

				//
				// Don't want to do this here because it will
				// scroll the list box back to the first drive
				//
				// MakeThisDriveVisible(hwnd,lpMyChkInf);

				// re-init title stuff
                                if (!SetDriveTitle(lpMyChkInf, hwnd)) {
				    // BUG BUG
				    EndDialog(hwnd, IDCANCEL);
				    return(TRUE);
				}
			    } else {
				lpMyChkInf->LstChkdDrv = 0;
				lpMyChkInf->IsMultiDrv = FALSE;
			    }
			    break;

			default:
			    break;
		    }
		    return(FALSE);
		    break;

		case DLGCHK_START:
		    if(lpMyChkInf->ChkInProgBool)
		    {
			return(TRUE);
		    }
		    if(lpMyChkInf->IsDrvList && (lpMyChkInf->DrivesToChk == 0L))
		    {
			MyChkdskMessageBox(lpMyChkInf, IDS_NOSEL,
					   MB_ICONINFORMATION | MB_OK);
			return(TRUE);
		    }

		    if(lpMyChkInf->NoParms)
		    {
			i = lpSHChkInfo->sDMaint.lpfnGetFileSysParameters(lpMyChkInf->lpwddi->iDrive,
							&(lpMyChkInf->ChkDPms),
							sizeof(lpMyChkInf->ChkDPms));
			switch(i) {
			    case FS_FAT:
			    case FS_DDFAT:
			    case FS_LFNFAT:
			    case FS_DDLFNFAT:
				lpMyChkInf->NoParms = FALSE;

				dwi = lpSHChkInfo->sDMaint.lpfnGetFixOptions(&(lpMyChkInf->ChkDPms));

				if(dwi & (FSINVALID | FSDISALLOWED))
				    goto CantChkDsk5;

				lpMyChkInf->FixOptions = dwi;

				// re-init title stuff
                                if (!SetDriveTitle(lpMyChkInf, hwnd)) {
				    // BUG BUG
				    EndDialog(hwnd, IDCANCEL);
				    return(TRUE);
                                }
				break;

			    default:
				lpMyChkInf->NoParms = TRUE;
CantChkDsk5:
				lpMyChkInf->DoCHRestart = FALSE;
				lpMyChkInf->DoSARestart = FALSE;
				if(lpMyChkInf->IsSplitDrv)
				{
				    SendDlgItemMessage(hwnd, DLGCHK_PBAR, PBM_SETPOS, 0, 0L);
				    dwj = GetWindowLong(GetDlgItem(hwnd,DLGCHK_PBAR),GWL_STYLE);
				    dwj &= ~PBS_SHOWPERCENT;
				    SetWindowLong(GetDlgItem(hwnd,DLGCHK_PBAR),GWL_STYLE,dwj);
				    InvalidateRect(GetDlgItem(hwnd,DLGCHK_PBAR),NULL,TRUE);
				}
				lpMyChkInf->IsSplitDrv = FALSE;
				switch (lpMyChkInf->lpwddi->iType)
				{
				    case DRIVE_REMOVABLE:
#ifdef FROSTING
					if(lpMyChkInf->fSageRun)
					{
					    lpMyChkInf->fShouldRerun = TRUE;
					    goto CantChkDsk6;
					}
#endif
					i = IDS_CANTCHKR;
					break;

				    case DRIVE_RAMDRIVE:
				    case DRIVE_FIXED:
#ifdef FROSTING
					if(lpMyChkInf->fSageRun)
					{
					    lpMyChkInf->fShouldRerun = TRUE;
					    goto CantChkDsk6;
					}
#endif
					i = IDS_CANTCHK;
					break;

				    case DRIVE_CDROM:
				    case DRIVE_REMOTE:
				    default:
#ifdef FROSTING
					if(lpMyChkInf->fSageRun)
					    goto CantChkDsk6;
#endif
					i = IDS_INVALID;
					break;
				}
				MyChkdskMessageBox(lpMyChkInf, i,
						   MB_ICONINFORMATION | MB_OK);
#ifdef FROSTING
CantChkDsk6:
#endif
				lpMyChkInf->LastChkRslt = LASTCHKRSLT_ERR;
#ifdef FROSTING
				SetMultiDrvRslt(lpMyChkInf);
#endif
				return(TRUE);
				break;
			}
		    }

		    MakeThisDriveVisible(hwnd,lpMyChkInf);

		    lpMyChkInf->CurrOpRegion = 0xFFFF;

		    dwj = lpSHChkInfo->sDMaint.lpfnGetFixOptions(&(lpMyChkInf->ChkDPms));

		    dwi = 0L;

		    if(IsDlgButtonChecked(hwnd, DLGCHK_NOBADB))
		    {
			lpMyChkInf->MyFixOpt |= DLGCHK_NOBAD;
		    } else {
			lpMyChkInf->MyFixOpt &= ~DLGCHK_NOBAD;
		    }
		    if(IsDlgButtonChecked(hwnd, DLGCHK_AUTOFIX))
		    {
			lpMyChkInf->MyFixOpt &= ~DLGCHK_INTER;
		    } else {
			lpMyChkInf->MyFixOpt |= DLGCHK_INTER;
		    }

		    if(lpMyChkInf->MyFixOpt & DLGCHK_NOBAD)
		    {
			dwi |= FDOS_NOSRFANAL;
		    } else {
#ifdef OPK2
			if(lpMyChkInf->MyFixOpt2 & DLGCHK_DOBADISNTBAD)
			    dwi |= FDOSFAT_RETESTBAD;
#endif
			if(!(lpMyChkInf->MyFixOpt & DLGCHK_NOWRTTST))
			    dwi |= FDOS_WRTTST;

			if(lpMyChkInf->MyFixOpt & DLGCHK_ALLHIDSYS)
			    dwi |= FDOSFAT_NMHISSYS;

			if(lpMyChkInf->MyFixOpt & DLGCHK_NODATA)
			{
			    dwi |= FDOSFAT_NODATATST;
			} else if(lpMyChkInf->MyFixOpt & DLGCHK_NOSYS) {
			    dwi |= FDOSFAT_NOSYSTST;
			}
		    }

		    if(lpMyChkInf->MyFixOpt & DLGCHK_RO)
		    {
			dwi |= FDO_NOFIX;
		    } else {
			if(lpMyChkInf->ChkDPms.drvprm.FatFS.FSFlags & FSFF_DMFFLOPPY)
			{
			    switch(MyChkdskMessageBox(lpMyChkInf, IDS_ISDMF,
						      MB_ICONQUESTION |
						      MB_YESNO |
						      MB_DEFBUTTON2))
			    {
				case IDYES:
				    dwi |= FDO_NOFIX;
				    break;

				case IDNO:
				default:
				    return(TRUE);
				    break;
			    }
			}
		    }

		    if(lpMyChkInf->MyFixOpt & DLGCHK_LSTMF)
			dwi |= FDOFAT_LSTMKFILE;

		    if(lpMyChkInf->MyFixOpt & DLGCHK_XLCPY)
		    {
			dwi |= FDOFAT_XLNKCPY;
		    } else if(lpMyChkInf->MyFixOpt & DLGCHK_XLDEL) {
			dwi |= FDOFAT_XLNKDEL;
		    }
		    if(lpMyChkInf->MyFixOpt & DLGCHK_NOCHKNM)
			dwi |= FDOFAT_NOCHKNM;

		    if(lpMyChkInf->MyFixOpt & DLGCHK_NOCHKDT)
			dwi |= FDOFAT_NOCHKDT;

		    if(lpMyChkInf->MyFixOpt2 & DLGCHK_EXCLU)
		    {
			dwi |= FDO_LOCKEXCLUSIVE;
		    } else if(lpMyChkInf->MyFixOpt2 & DLGCHK_FLWRT) {
			dwi |= FDO_LOCKFAILWRT;
		    } else {
			dwi |= FDO_LOCKBLKWRT;
		    }

		    if(lpMyChkInf->MyFixOpt2 & DLGCHK_MKOLDFS)
		    {
			dwi |= FDOFAT_MKOLDFS;

			// Do not record settings in registry when running
			// in this mode because this mode auto-changes some
			// settings and we don't want those settings recorded
			// if they are different than the ones the user has set.

		    } else {
			SetChkRegOptions(MAKELONG((lpMyChkInf->MyFixOpt &  (~(SHCHK_OPT_DEFOPTIONS |
									      SHCHK_OPT_DRVLISTONLY |
									      SHCHK_OPT_AUTO))),
						  (lpMyChkInf->MyFixOpt2 & (~(SHCHK_OPT_MKOLDFS |
									      SHCHK_OPT_PROGONLY |
									      SHCHK_OPT_NOWND)))),
#ifdef FROSTING
					 lpMyChkInf->NoUnsupDrvs,
					 lpMyChkInf->DrivesToChk,
					 lpMyChkInf->idxSettings);
#else
					 lpMyChkInf->NoUnsupDrvs);
#endif
		    }

#ifdef FROSTING
		    // If all we wanted was to set settings in the
		    // registry, then we just finished.

		    if (lpMyChkInf->fSageSet)
		    {
			EndDialog(hwnd, IDOK);
			return (TRUE);
		    }
#endif

		    lpMyChkInf->ChkCancelBool = FALSE;
		    lpMyChkInf->SrfInProgBool = FALSE;
		    lpMyChkInf->AlrdyRestartWrn = FALSE;
#ifdef OPK2
		    lpMyChkInf->Done3PtyCompWrn = FALSE;
#endif
		    lpMyChkInf->ChkEngCancel	= FALSE;
		    lpMyChkInf->ChkInProgBool = TRUE;
		    lpMyChkInf->RWRstsrtCnt   = 0;

		    if(lpMyChkInf->IsSplitDrv)
		    {
			if(!(lpMyChkInf->DoingCompDrv))
			    lpMyChkInf->NoRstrtWarn   = FALSE;
		    } else {
			lpMyChkInf->NoRstrtWarn   = FALSE;
		    }
		    EnableWindow(GetDlgItem(hwnd,DLGCHK_ADVANCED),FALSE);
		    if(lpMyChkInf->hWndPar == 0)
		    {
			hMenu = GetSystemMenu(hwnd,FALSE);
			if(hMenu)
			{
			    EnableMenuItem(hMenu,DLGCHK_CHELP,MF_BYCOMMAND | MF_GRAYED);
			    EnableMenuItem(hMenu,DLGCHK_ABOUT,MF_BYCOMMAND | MF_GRAYED);
			}
		    }
		    EnableWindow(GetDlgItem(hwnd,DLGCHK_START),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCHK_DRVLIST),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCHK_NOBADB),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCHK_DTXT1),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCHK_DOBAD),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCHK_DTXT2),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCHK_AUTOFIX),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCHK_BADOPT),FALSE);

		    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ10);
		    if(pMsgBuf)
		    {
			LoadString(g_hInstance, IDS_C_INIT, FmtBuf10, SZFMTBUF10);
			SetDlgItemText(hwnd, DLGCHK_STATTXT, FmtBuf10);
			SetDlgItemText(hwnd, DLGCHK_STATTXT2, g_szNULL);
			lpMyChkInf->pTextIsComplt = FALSE;

			LoadString(g_hInstance, IDS_CANCEL, FmtBuf10, SZFMTBUF10);
			SetDlgItemText(hwnd, DLGCHK_CANCEL, FmtBuf10);
			LocalFree((HANDLE)pMsgBuf);
		    } else {
			MyChkdskMessageBox(lpMyChkInf, IDS_NOMEM2,
					   MB_ICONINFORMATION | MB_OK);
		    }

		    SendMessage(hwnd,DM_SETDEFID,DLGCHK_CANCEL,0L);
#ifdef FROSTING
		    // Do not want to do this SetFocus is we are not the active
		    // app.

		    if(lpMyChkInf->ChkIsActive)
		    {
			SetFocus(GetDlgItem(hwnd,DLGCHK_CANCEL));
		    }
#else
		    SetFocus(GetDlgItem(hwnd,DLGCHK_CANCEL));
#endif

		    if(lpMyChkInf->IsSplitDrv)
		    {
			if(!(lpMyChkInf->DoingCompDrv))
			    goto InitPBar;
		    } else {
InitPBar:
			SendDlgItemMessage(hwnd, DLGCHK_PBAR, PBM_SETPOS, 0, 0L);
			dwj = GetWindowLong(GetDlgItem(hwnd,DLGCHK_PBAR),GWL_STYLE);
			dwj |= PBS_SHOWPERCENT;
			SetWindowLong(GetDlgItem(hwnd,DLGCHK_PBAR),GWL_STYLE,dwj);
			InvalidateRect(GetDlgItem(hwnd,DLGCHK_PBAR),NULL,TRUE);
		    }

		    lpMyChkInf->DoSARestart = FALSE;
		    lpMyChkInf->DoCHRestart = FALSE;

#ifdef DOSETUPCHK
		    {
			DMaint_CheckDriveSetupPROC lpfnCheckDriveSetup;


			(FARPROC)lpfnCheckDriveSetup =
				GetProcAddress(lpSHChkInfo->sDMaint.hInstDMaint,
					       "DMaint_CheckDriveSetup");

			dwi = lpfnCheckDriveSetup(lpMyChkInf->lpwddi->iDrive,
						  dwi,
						  ChkSetupCBProc,
						  (LPARAM)lpMyChkInf);
		    }
#else
		    SEInitLog(lpMyChkInf);
		    SEAddToLogStart(lpMyChkInf,lpMyChkInf->IsFirstDrv);

		    dwi = lpSHChkInfo->sDMaint.lpfnFixDrive(&(lpMyChkInf->ChkDPms),
					  dwi,
					  ChkCBProc,
					  (LPARAM)lpMyChkInf);
#endif

		    lpMyChkInf->AlrdyRestartWrn = FALSE;
		    lpMyChkInf->ChkInProgBool = FALSE;
#ifdef OPK2
		    lpMyChkInf->Done3PtyCompWrn = FALSE;
		    if(lpMyChkInf->hAniIcon1 != NULL)
		    {
			SendDlgItemMessage(hwnd, IDC_ICON_1, STM_SETIMAGE, IMAGE_ICON, MAKELONG((WORD)lpMyChkInf->hAniIcon1,0));
			lpMyChkInf->hCurrAniIcon = lpMyChkInf->hAniIcon1;
			InvalidateRect(GetDlgItem(hwnd,IDC_ICON_1),NULL,TRUE);
			UpdateWindow(GetDlgItem(hwnd,IDC_ICON_1));
		    }
#endif
		    if(lpMyChkInf->hTimer != 0)
		    {
			KillTimer(lpMyChkInf->hProgDlgWnd,lpMyChkInf->hTimer);
			lpMyChkInf->hTimer = 0;
		    }
		    switch (HIWORD(dwi))
		    {
			// These errors occurs before
			//  any call backs are made

			case ERR_BADOPTIONS:
			    lpMyChkInf->DoSARestart = FALSE;
			    lpMyChkInf->DoCHRestart = FALSE;
			    SEAddToLogRCS(lpMyChkInf,IDL_ENGBADOPT,NULL);
			    i = IDS_UNEXP3;
			    goto DoErr2;
			    break;

			case ERR_NOTSUPPORTED:
			    lpMyChkInf->DoCHRestart = FALSE;
			    lpMyChkInf->DoSARestart = FALSE;
			    SEAddToLogRCS(lpMyChkInf,IDL_ENGNOTSUPP,NULL);
			    switch (lpMyChkInf->lpwddi->iType)
			    {
				case DRIVE_REMOVABLE:
				    i = IDS_UNSUPR;
				    break;

				case DRIVE_RAMDRIVE:
				case DRIVE_FIXED:
				    i = IDS_UNSUP;
				    break;

				case DRIVE_CDROM:
				case DRIVE_REMOTE:
				default:
#ifdef FROSTING
				    if(lpMyChkInf->fSageRun)
				    {
					goto DoErr4;
				    }
#endif
				    i = IDS_INVALID;
				    break;
			    }
			    goto DoErr3;
			    break;

			// These errors may be
			//  correctable

			case ERR_NOTWRITABLE:
			    lpMyChkInf->DoCHRestart = FALSE;
			    lpMyChkInf->DoSARestart = FALSE;
			    SEAddToLogRCS(lpMyChkInf,IDL_ENGCANTWRT,NULL);
			    i = IDS_CANTWRT;
			    goto DoErr2;
			    break;

			case ERR_OSERR:
			    lpMyChkInf->DoCHRestart = FALSE;
			    lpMyChkInf->DoSARestart = FALSE;
			    SEAddToLogRCS(lpMyChkInf,IDL_ENGOS,NULL);
			    i = IDS_SERDISK;
			    goto DoErr2;
			    break;

			// These errors may go away
			//  if it is tried again.

			case ERR_FSERR:
			    lpMyChkInf->DoCHRestart = FALSE;
			    lpMyChkInf->DoSARestart = FALSE;
			    SEAddToLogRCS(lpMyChkInf,IDL_ENGFS,NULL);
			    i = IDS_SERFS;
			    goto DoErr2;
			    break;

			case ERR_FSACTIVE:
			    lpMyChkInf->DoCHRestart = FALSE;
			    lpMyChkInf->DoSARestart = FALSE;
			    SEAddToLogRCS(lpMyChkInf,IDL_ENGACTIVE,NULL);
			    i = IDS_ACTIVE;
			    goto DoErr2;
			    break;

			case ERR_LOCKVIOLATION:
			    lpMyChkInf->DoCHRestart = FALSE;
			    lpMyChkInf->DoSARestart = FALSE;
			    SEAddToLogRCS(lpMyChkInf,IDL_ENGLOCK,NULL);
#ifdef FROSTING
			    if(lpMyChkInf->fSageRun)
			    {
			        lpMyChkInf->fShouldRerun = TRUE;
				goto DoErr4;
			    }
#endif
			    if(lpMyChkInf->MyFixOpt2 & DLGCHK_EXCLU)
				i = IDS_LOCKVIOL2;
			    else
				i = IDS_LOCKVIOL;
DoErr3:
			    MyChkdskMessageBox(lpMyChkInf, i,
					       MB_ICONINFORMATION | MB_OK);
#ifdef FROSTING
DoErr4:
#endif
			    lpMyChkInf->LastChkRslt = LASTCHKRSLT_ERR;
#ifdef FROSTING
			    SetMultiDrvRslt(lpMyChkInf);
#endif
			    if(lpMyChkInf->IsSplitDrv)
			    {
				lpMyChkInf->IsSplitDrv = FALSE;
				lpMyChkInf->DoingCompDrv = FALSE;
			    }
			    lpMyChkInf->NoParms = TRUE;
			    break;

			case ERR_INSUFMEM:
			    lpMyChkInf->DoCHRestart = FALSE;
			    lpMyChkInf->DoSARestart = FALSE;
			    SEAddToLogRCS(lpMyChkInf,IDL_ENGMEM,NULL);
			    i = IDS_NOMEM3;
DoErr2:
			    MyChkdskMessageBox(lpMyChkInf, i,
					       MB_ICONINFORMATION | MB_OK);
			    lpMyChkInf->LastChkRslt = LASTCHKRSLT_ERR;
#ifdef FROSTING
			    SetMultiDrvRslt(lpMyChkInf);
#endif
			    if(lpMyChkInf->IsSplitDrv)
			    {
				lpMyChkInf->IsSplitDrv = FALSE;
				lpMyChkInf->DoingCompDrv = FALSE;
			    }
			    break;

			case OPCANCEL:
			    if((!lpMyChkInf->DoCHRestart) &&
			       (!lpMyChkInf->DoSARestart)   )
			    {
				SEAddToLogRCS(lpMyChkInf,IDL_ENGCANCEL,NULL);
				lpMyChkInf->ChkEngCancel = TRUE;
			    }
			    lpMyChkInf->LastChkRslt = LASTCHKRSLT_CAN;
#ifdef FROSTING
			    SetMultiDrvRslt(lpMyChkInf);
#endif
			    if(lpMyChkInf->IsSplitDrv)
			    {
				lpMyChkInf->IsSplitDrv = FALSE;
				lpMyChkInf->DoingCompDrv = FALSE;
			    }
			    if(lpMyChkInf->DoCHRestart)
			    {
				SEAddToLogRCS(lpMyChkInf,IDL_ENGCHKHSTRST,NULL);
				lpMyChkInf->MyFixOpt2 &= ~DLGCHK_NOCHKHST;
				goto TurnSA2;
			    }
			    if(lpMyChkInf->DoSARestart)
			    {
				SEAddToLogRCS(lpMyChkInf,IDL_ENGSARST,NULL);
TurnSA2:
				lpMyChkInf->MyFixOpt &= ~(DLGCHK_NOBAD | DLGCHK_NODATA | DLGCHK_NOSYS);
			    }
			    break;

			case ERR_FSUNCORRECTED:
			    lpMyChkInf->DoSARestart = FALSE;
			    lpMyChkInf->DoCHRestart = FALSE;
			    lpMyChkInf->LastChkRslt = LASTCHKRSLT_SMNOTFIX;
#ifdef FROSTING
			    SetMultiDrvRslt(lpMyChkInf);
#endif
			    i = DTRESULTPROB;
			    goto RecordLstRun;
			    break;

			case NOERROR:
			    lpMyChkInf->DoCHRestart = FALSE;
			    lpMyChkInf->DoSARestart = FALSE;
			    lpMyChkInf->LastChkRslt = LASTCHKRSLT_NOERROR;
#ifdef FROSTING
			    SetMultiDrvRslt(lpMyChkInf);
#endif
			    i = DTRESULTOK;
			    goto RecordLstRun;
			    break;

			case ERR_FSCORRECTED:
			    lpMyChkInf->DoCHRestart = FALSE;
			    lpMyChkInf->DoSARestart = FALSE;
			    //
			    // The SHCheckDrive return is not sensitive
			    // to DLGCHK_RO, the registry is. This is
			    // because the registry does not record that
			    // this was a DLGCHK_RO run and nothing
			    // was actually fixed.
			    //
			    lpMyChkInf->LastChkRslt = LASTCHKRSLT_ALLFIXED;
#ifdef FROSTING
			    SetMultiDrvRslt(lpMyChkInf);
#endif
			    if(lpMyChkInf->MyFixOpt & DLGCHK_RO)
			    {
				i = DTRESULTPROB;
			    } else {
				i = DTRESULTFIX;
			    }
RecordLstRun:
			    if(lpMyChkInf->lpwddi->iType != DRIVE_REMOVABLE)
			    {
				char szDrive[] = "A";

				szDrive[0] += lpMyChkInf->lpwddi->iDrive;

				SaveTimeInReg(HKEY_LOCAL_MACHINE,
					      REGSTR_PATH_LASTCHECK,
					      szDrive,i,TRUE);

				SaveTimeInReg(HKEY_LOCAL_MACHINE,
					      REGSTR_PATH_CHKLASTCHECK,
					      szDrive,i,FALSE);

				if((!(lpMyChkInf->MyFixOpt & DLGCHK_NOBAD)) &&
				   (!(lpMyChkInf->MyFixOpt & DLGCHK_NODATA))  )
				{
				    SaveTimeInReg(HKEY_LOCAL_MACHINE,
						  REGSTR_PATH_CHKLASTSURFAN,
						  szDrive,i,FALSE);
				}
			    }
			    break;

			default:
			    lpMyChkInf->DoCHRestart = FALSE;
			    lpMyChkInf->DoSARestart = FALSE;
			    lpMyChkInf->LastChkRslt = LASTCHKRSLT_ERR;
#ifdef FROSTING
			    SetMultiDrvRslt(lpMyChkInf);
#endif
			    SEAddToLogRCS(lpMyChkInf,IDL_ENGUXP,NULL);
			    break;		// just continue
		    }
		    SEAddToLogRCS(lpMyChkInf,IDL_TRAILER,NULL);
		    if(lpMyChkInf->IsFirstDrv)
		    {
			SERecordLog(lpMyChkInf,FALSE);
		    } else {
			SERecordLog(lpMyChkInf,TRUE);
		    }

		    if(!(lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			EnableWindow(GetDlgItem(hwnd,DLGCHK_ADVANCED),TRUE);
			if(lpMyChkInf->hWndPar == 0)
			{
			    hMenu = GetSystemMenu(hwnd,FALSE);
			    if(hMenu)
			    {
				EnableMenuItem(hMenu,DLGCHK_CHELP,MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hMenu,DLGCHK_ABOUT,MF_BYCOMMAND | MF_ENABLED);
			    }
			}
			EnableWindow(GetDlgItem(hwnd,DLGCHK_NOBADB),TRUE);
			EnableWindow(GetDlgItem(hwnd,DLGCHK_DTXT1),TRUE);
			EnableWindow(GetDlgItem(hwnd,DLGCHK_DOBAD),TRUE);
			EnableWindow(GetDlgItem(hwnd,DLGCHK_DTXT2),TRUE);
			if(!(lpMyChkInf->MyFixOpt & DLGCHK_RO))
			{
			    EnableWindow(GetDlgItem(hwnd,DLGCHK_AUTOFIX),TRUE);
			}
			if(lpMyChkInf->MyFixOpt & DLGCHK_NOBAD)
			{
			    SendMessage(GetDlgItem(hwnd,DLGCHK_DOBAD),BM_SETCHECK,0,0);
			    SendMessage(GetDlgItem(hwnd,DLGCHK_NOBADB),BM_SETCHECK,1,0);
			} else {
			    SendMessage(GetDlgItem(hwnd,DLGCHK_NOBADB),BM_SETCHECK,0,0);
			    SendMessage(GetDlgItem(hwnd,DLGCHK_DOBAD),BM_SETCHECK,1,0);
			    EnableWindow(GetDlgItem(hwnd,DLGCHK_BADOPT),TRUE);
			}
		    }
		    EnableWindow(GetDlgItem(hwnd,DLGCHK_START),TRUE);
		    if((lpMyChkInf->IsDrvList) &&
		       (!(lpMyChkInf->MyFixOpt & SHCHK_OPT_DRVLISTONLY)) )
			EnableWindow(GetDlgItem(hwnd,DLGCHK_DRVLIST),TRUE);

		    SendDlgItemMessage(hwnd,DLGCHK_CANCEL,BM_SETSTATE,
				       FALSE,0L);
		    //
		    // Note that we do NOT DM_SETDEFID and setfocus back
		    //	to the start button. CHKDSK is a "do once" sort
		    //	of thing.
		    //

		    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ10);
		    if(pMsgBuf)
		    {
			LoadString(g_hInstance, IDS_CLOSEM, FmtBuf10, SZFMTBUF10);
			SetDlgItemText(hwnd, DLGCHK_CANCEL, FmtBuf10);

			LoadString(g_hInstance, IDS_C_COMPLETE, FmtBuf10, SZFMTBUF10);
			SetDlgItemText(hwnd, DLGCHK_STATTXT, FmtBuf10);
			SetDlgItemText(hwnd, DLGCHK_STATTXT2, g_szNULL);
			lpMyChkInf->pTextIsComplt = TRUE;
			LocalFree((HANDLE)pMsgBuf);
		    } else {
			MyChkdskMessageBox(lpMyChkInf, IDS_NOMEM2,
					   MB_ICONINFORMATION | MB_OK);
			SetDlgItemText(hwnd, DLGCHK_STATTXT, g_szNULL);
			SetDlgItemText(hwnd, DLGCHK_STATTXT2, g_szNULL);
			lpMyChkInf->pTextIsComplt = FALSE;
		    }

		    if(lpMyChkInf->IsSplitDrv)
		    {
			if(lpMyChkInf->DoingCompDrv)
			{
			    goto InitPBar2;
			}
		    } else {
InitPBar2:
			SendDlgItemMessage(hwnd, DLGCHK_PBAR, PBM_SETPOS, 0, 0L);
			dwj = GetWindowLong(GetDlgItem(hwnd,DLGCHK_PBAR),GWL_STYLE);
			dwj &= ~PBS_SHOWPERCENT;
			SetWindowLong(GetDlgItem(hwnd,DLGCHK_PBAR),GWL_STYLE,dwj);
			InvalidateRect(GetDlgItem(hwnd,DLGCHK_PBAR),NULL,TRUE);
		    }

		    if(lpMyChkInf->IsSplitDrv)
		    {
			if(!(lpMyChkInf->DoingCompDrv))
			{
			    lpMyChkInf->HstDrvsChckd |= 0x00000001L << lpMyChkInf->HostDrv;
			    lpMyChkInf->DoingCompDrv = TRUE;
			    lpMyChkInf->lpwddi->iDrive = lpMyChkInf->CompDrv;
			    InitDrvInfo(0, lpMyChkInf->lpwddi, &lpSHChkInfo->sDMaint);
			    lpMyChkInf->IsFirstDrv = FALSE;
			    goto InitNext2a;
			}
			lpMyChkInf->IsSplitDrv = FALSE;
			lpMyChkInf->DoingCompDrv = FALSE;
		    }

		    // Refresh the drive display to show the new state

		    if(lpMyChkInf->lpwddi->hDriveWindow)
			PostMessage(lpMyChkInf->lpwddi->hDriveWindow,WM_APP+2,0,0L);

		    if((lpMyChkInf->DoCHRestart) ||
		       (lpMyChkInf->DoSARestart)   )
		    {
			goto InitNext2;
		    }
		    if((lpMyChkInf->IsMultiDrv)  &&
		       (lpMyChkInf->ChkEngCancel)  )
		    {
			goto CancelChk2;
		    }

		    if(lpMyChkInf->IsDrvList)
		    {
			// Set lpMyChkInf->LstChkdDrv to the next drive
			// specified in DrivesToChk

			dwi = 0x00000001L << (lpMyChkInf->LstChkdDrv + 1);
			for(i = lpMyChkInf->LstChkdDrv + 1; i < 26; i++)
			{
			    if(lpMyChkInf->DrivesToChk & dwi)
			    {
				lpMyChkInf->LstChkdDrv = i;
InitNext2:
				lpMyChkInf->IsFirstDrv = FALSE;
				lpMyChkInf->IsSplitDrv	 = FALSE;
				lpMyChkInf->DoingCompDrv = FALSE;
				lpMyChkInf->lpwddi->iDrive = lpMyChkInf->LstChkdDrv;
				InitDrvInfo(0, lpMyChkInf->lpwddi, &lpSHChkInfo->sDMaint);
				if(lpMyChkInf->lpwddi->TypeFlags & DT_DBLDISK)
				{
				    lstrcpy(lpMyChkInf->CompdriveNameStr,lpMyChkInf->lpwddi->driveNameStr);
				}
				if((lpMyChkInf->lpwddi->TypeFlags & DT_DBLDISK) &&
				   (!(lpMyChkInf->MyFixOpt2 & DLGCHK_NOCHKHST))   )
				{
				    lpMyChkInf->CompDrv = lpMyChkInf->lpwddi->iDrive;
				    lpMyChkInf->HostDrv = GetCompInfo(lpMyChkInf->CompDrv,&i);
				    if(lpMyChkInf->HostDrv != 0xFFFF)
				    {
					dwi = 0x00000001L << lpMyChkInf->HostDrv;
					if(lpMyChkInf->HstDrvsChckd & dwi)
					    goto InitNext2a;

					if((lpMyChkInf->DrivesToChk & dwi) &&
					   (lpMyChkInf->HostDrv <= lpMyChkInf->LstChkdDrv))
					    goto InitNext2a;

					lpMyChkInf->IsSplitDrv	 = TRUE;
					lpMyChkInf->DoingCompDrv = FALSE;
					lpMyChkInf->lpwddi->iDrive = lpMyChkInf->HostDrv;
					InitDrvInfo(0, lpMyChkInf->lpwddi, &lpSHChkInfo->sDMaint);
				    }
				}

				// Init for CHKDSK
InitNext2a:
				i = lpSHChkInfo->sDMaint.lpfnGetFileSysParameters(lpMyChkInf->lpwddi->iDrive,
								&(lpMyChkInf->ChkDPms),
								sizeof(lpMyChkInf->ChkDPms));
				switch(i) {
				    case FS_FAT:
				    case FS_DDFAT:
				    case FS_LFNFAT:
				    case FS_DDLFNFAT:
					lpMyChkInf->NoParms = FALSE;
					break;

				    default:
					lpMyChkInf->NoParms = TRUE;
CantChkDsk2:
					lpMyChkInf->DoCHRestart = FALSE;
					lpMyChkInf->DoSARestart = FALSE;
					if(lpMyChkInf->IsSplitDrv)
					{
					    SendDlgItemMessage(hwnd, DLGCHK_PBAR, PBM_SETPOS, 0, 0L);
					    dwj = GetWindowLong(GetDlgItem(hwnd,DLGCHK_PBAR),GWL_STYLE);
					    dwj &= ~PBS_SHOWPERCENT;
					    SetWindowLong(GetDlgItem(hwnd,DLGCHK_PBAR),GWL_STYLE,dwj);
					    InvalidateRect(GetDlgItem(hwnd,DLGCHK_PBAR),NULL,TRUE);
					}
					lpMyChkInf->IsSplitDrv = FALSE;
					switch (lpMyChkInf->lpwddi->iType)
					{
					    case DRIVE_REMOVABLE:
#ifdef FROSTING
						if(lpMyChkInf->fSageRun)
						{
						    lpMyChkInf->fShouldRerun = TRUE;
						    goto CantChkDsk2point5;
						}
#endif
						i = IDS_CANTCHKR;
						break;

					    case DRIVE_RAMDRIVE:
					    case DRIVE_FIXED:
#ifdef FROSTING
						if(lpMyChkInf->fSageRun)
						{
						    lpMyChkInf->fShouldRerun = TRUE;
						    goto CantChkDsk2point5;
						}
#endif
						i = IDS_CANTCHK;
						break;

					    case DRIVE_CDROM:
					    case DRIVE_REMOTE:
					    default:
#ifdef FROSTING
						if(lpMyChkInf->fSageRun)
						{
						    goto CantChkDsk2point5;
						}
#endif
						i = IDS_INVALID;
						break;
					}
					MyChkdskMessageBox(lpMyChkInf, i,
							   MB_ICONINFORMATION | MB_OK);
#ifdef FROSTING
CantChkDsk2point5:
#endif
					lpMyChkInf->LastChkRslt = LASTCHKRSLT_ERR;
#ifdef FROSTING
					SetMultiDrvRslt(lpMyChkInf);
#endif

					// Set lpMyChkInf->LstChkdDrv to the next drive
					// specified in DrivesToChk

					dwi = 0x00000001L << (lpMyChkInf->LstChkdDrv + 1);
					for(i = lpMyChkInf->LstChkdDrv + 1; i < 26; i++)
					{
					    if(lpMyChkInf->DrivesToChk & dwi)
					    {
						lpMyChkInf->LstChkdDrv = i;
						goto InitNext2;
					    }
					    dwi = dwi << 1;
					}
					if(lpMyChkInf->MyFixOpt & DLGCHK_AUTO)
					{
					    EndDialog(hwnd, IDCANCEL);
					}
#ifdef FROSTING
					if(lpMyChkInf->fSageRun)
					{
					    EndDialog(hwnd, IDOK);
					}
#endif
					lpMyChkInf->HstDrvsChckd = 0L;
					lpMyChkInf->IsSplitDrv	 = FALSE;
					lpMyChkInf->DoingCompDrv = FALSE;
					lpMyChkInf->IsFirstDrv = TRUE;
					goto NoDrvsToChk2;
					break;
				}

				dwi = lpSHChkInfo->sDMaint.lpfnGetFixOptions(&(lpMyChkInf->ChkDPms));

				if(dwi & (FSINVALID | FSDISALLOWED))
				    goto CantChkDsk2;

				lpMyChkInf->FixOptions = dwi;

				MakeThisDriveVisible(hwnd,lpMyChkInf);

				// re-init title stuff
                                if (!SetDriveTitle(lpMyChkInf, hwnd)) {
				    // BUG BUG
				    EndDialog(hwnd, IDCANCEL);
				    return(TRUE);
                                }
                                
				if((!lpMyChkInf->ChkEngCancel) &&
				   ((lpMyChkInf->MyFixOpt & DLGCHK_AUTO) ||
#ifdef FROSTING
				    (lpMyChkInf->fSageRun) ||
#endif
				    (lpMyChkInf->IsMultiDrv) ||
				    ((lpMyChkInf->IsSplitDrv) && (lpMyChkInf->DoingCompDrv)) ||
				    (lpMyChkInf->DoCHRestart) ||
				    (lpMyChkInf->DoSARestart)	) )
				{
				    lpMyChkInf->DoCHRestart = FALSE;
				    lpMyChkInf->DoSARestart = FALSE;
				    PostMessage(hwnd,WM_COMMAND,DLGCHK_START,0L);
				} else {
				    lpMyChkInf->IsFirstDrv = TRUE;
				}
				goto NoDrvsToChk2;
			    }
			    dwi = dwi << 1;
			}
			//
			// Here if no more drives to check. If we're AUTO
			// we're done, else we want to reset the selected
			// drive back to the FIRST drive.
			//
CancelChk2:
			lpMyChkInf->HstDrvsChckd = 0L;
			lpMyChkInf->IsSplitDrv	 = FALSE;
			lpMyChkInf->DoingCompDrv = FALSE;
			if(lpMyChkInf->MyFixOpt & DLGCHK_AUTO)
			{
			    EndDialog(hwnd, IDCANCEL);
			}
#ifdef FROSTING
			if(lpMyChkInf->fSageRun)
			{
			    EndDialog(hwnd, IDOK);
			}
#endif

			dwi = 0x00000001L;
			for(i = 0; i < 26; i++)
			{
			    if(lpMyChkInf->DrivesToChk & dwi)
			    {
				lpMyChkInf->LstChkdDrv = i;
				//
				// Want to set this flag so we'll stop
				// at this point and wait for the USER
				// to push the START button again (see
				// if around above PostMessage) at which
				// point this flag will get cleared.
				//
				lpMyChkInf->ChkEngCancel = TRUE;
				goto InitNext2;
			    }
			    dwi = dwi << 1;
			}
		    }
NoDrvsToChk2:
		    return(TRUE);
		    break;

		case DLGCHK_CANCEL:
		    if(lpMyChkInf->ChkInProgBool)
		    {
			lpMyChkInf->ChkCancelBool = TRUE;
		    } else {
			EndDialog(hwnd, IDCANCEL);
		    }
		    return(TRUE);
		    break;

		case DLGCHK_NOBADB:
		    if((lpMyChkInf->ChkInProgBool) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			return(TRUE);
		    }
		    SendMessage(GetDlgItem(hwnd,DLGCHK_DOBAD),BM_SETCHECK,0,0);
		    SendMessage(GetDlgItem(hwnd,DLGCHK_NOBADB),BM_SETCHECK,1,0);
		    EnableWindow(GetDlgItem(hwnd,DLGCHK_BADOPT),FALSE);
		    return(TRUE);
		    break;

		case DLGCHK_DOBAD:
		    if((lpMyChkInf->ChkInProgBool) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			return(TRUE);
		    }
		    SendMessage(GetDlgItem(hwnd,DLGCHK_NOBADB),BM_SETCHECK,0,0);
		    SendMessage(GetDlgItem(hwnd,DLGCHK_DOBAD),BM_SETCHECK,1,0);
		    EnableWindow(GetDlgItem(hwnd,DLGCHK_BADOPT),TRUE);
		    return(TRUE);
		    break;

		case DLGCHK_BADOPT:
		    if((lpMyChkInf->ChkInProgBool) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			return(TRUE);
		    }
		    DialogBoxParam(g_hInstance,
				   MAKEINTRESOURCE(DLG_CHKDSKSAOPT),
				   hwnd,
				   ChkSADlgWndProc,
				   (LPARAM)lpMyChkInf);
		    return(TRUE);
		    break;

		case DLGCHK_AUTOFIX:
		    if((lpMyChkInf->ChkInProgBool) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			return(TRUE);
		    }
		    CheckDlgButton(hwnd, wParam, !IsDlgButtonChecked(hwnd, wParam));
		    return(TRUE);
		    break;

		case DLGCHK_ADVANCED:
		    if((lpMyChkInf->ChkInProgBool) ||
		       (lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND)))
		    {
			return(TRUE);
		    }

		    i = lpMyChkInf->MyFixOpt2 & DLGCHK_NOCHKHST;

		    DialogBoxParam(g_hInstance,
				   MAKEINTRESOURCE(DLG_CHKDSKADVOPT),
				   hwnd,
				   ChkAdvDlgWndProc,
				   (LPARAM)lpMyChkInf);

		    if(((lpMyChkInf->MyFixOpt2 & DLGCHK_NOCHKHST) && (i == 0))	 ||
		       ((!(lpMyChkInf->MyFixOpt2 & DLGCHK_NOCHKHST)) && (i != 0))  )
		    {
			//
			// Ooops DLGCHK_NOCHKHST changed state
			//  Need to resetup the drive selection
			//
			if(!lpMyChkInf->IsDrvList)
			{
			    lpMyChkInf->LstChkdDrv = lpMyChkInf->lpwddi->iDrive;
			}
			// lpMyChkInf->LstChkdDrv is set to the
			// drive we are interested in.

			goto InitNext2;
		    }
		    return(TRUE);
		    break;

		default:
		    return(FALSE);

	    }
	    break;

	case WM_APP+3:	       // Chkdsk complete
	    if(lpMyChkInf->MyFixOpt & DLGCHK_REP)
	    {
		//
		// We don't want to "dual report" on a CHKHST unless
		// errors were uncorrected on the host drive.
		//
		if((lpMyChkInf->IsSplitDrv) &&
		   (!(lpMyChkInf->DoingCompDrv)) )
		{
		    if((HIWORD(lpMyChkInf->OpCmpltRet) == ERR_FSCORRECTED) ||
		       (HIWORD(lpMyChkInf->OpCmpltRet) == NOERROR)	     )
		    {
			goto ChkReboot;
		    }
		}

		if(lpMyChkInf->MyFixOpt2 & DLGCHK_REPONLYERR)
		{
#ifdef FROSTING
		    if(lpMyChkInf->lpFixRep != 0L && lpMyChkInf->lpFixRep->ProbCnt != 0L)
		    {
#endif
			switch(HIWORD(lpMyChkInf->OpCmpltRet))
			{
			    case ERR_FSUNCORRECTED:
			    case ERR_FSCORRECTED:
				goto DoReport;
				break;

			    // case NOERROR:
			    // case ERR_OSERR:
			    // case ERR_NOTWRITABLE:
			    // case ERR_NOTSUPPORTED:
			    // case ERR_INSUFMEM:
			    // case ERR_EXCLVIOLATION:
			    // case ERR_LOCKVIOLATION:
			    // case ERR_FSACTIVE:
			    // case ERR_FSERR:
			    // case ERR_BADOPTIONS:
			    // case OPCANCEL:
			    default:
				break;
			}
#ifdef FROSTING
		    }
#endif
		} else {
		    switch(HIWORD(lpMyChkInf->OpCmpltRet))
		    {
			case ERR_FSUNCORRECTED:
			case ERR_FSCORRECTED:
			case NOERROR:
DoReport:
			    if((lpMyChkInf->lpFixRep != 0L) &&
			       (!(lpMyChkInf->MyFixOpt2 & (DLGCHK_PROGONLY | DLGCHK_NOWND))) )
			    {
				DialogBoxParam(g_hInstance,
					       MAKEINTRESOURCE(DLG_CHKDSKREPORT),
					       hwnd,
					       ChkRepDlgWndProc,
					       (LPARAM)lpMyChkInf);
			    }
			    break;

			// case ERR_OSERR:
			// case ERR_NOTWRITABLE:
			// case ERR_NOTSUPPORTED:
			// case ERR_INSUFMEM:
			// case ERR_EXCLVIOLATION:
			// case ERR_LOCKVIOLATION:
			// case ERR_FSACTIVE:
			// case ERR_FSERR:
			// case ERR_BADOPTIONS:
			// case OPCANCEL:
			default:
			    break;
		    }
		}
	    }
ChkReboot:
	    if((lpMyChkInf->lpFixRep != 0L) &&
#ifdef FROSTING
	       (lpMyChkInf->lpFixRep->ProbCnt != 0) &&
#endif
	       (lpMyChkInf->lpFixRep->Flags & NORMSHUTDOWN))
	    {
		MyChkdskMessageBox(lpMyChkInf,IDS_NORMWARN,MB_ICONINFORMATION | MB_OK);
	    }
	    return(TRUE);
	    break;

	case WM_APP+4:	       // ChkDsk progress update
	    if(wParam)
	    {
#ifdef DOSETUPCHK
		switch(HIWORD(lpMyChkInf->lParam1))
#else
		switch(lpMyChkInf->lpFixFDisp->CurrOpRegion)
#endif
		{

		    case FOP_INIT:
			i = IDS_C_INIT;
			goto SetStTxt;
			break;

		    case FOP_FAT:
			i = IDS_C_FAT;
			goto SetStTxt;
			break;

		    case FOP_DIR:
			i = IDS_C_DIR;
			goto SetStTxt;
			break;

		    case FOP_FILDIR:
			i = IDS_C_FILEDIR;
			goto SetStTxt;
			break;

		    case FOP_LOSTCLUS:
			i = IDS_C_LOSTCLUS;
			goto SetStTxt;
			break;

		    case FSOP_INIT:
			lpMyChkInf->SrfInProgBool = TRUE;
			i = IDS_B_INIT;
			goto SetStTxt;
			break;

		    case FSOP_SETUNMOV:
			i = IDS_B_UNMOV;
			goto SetStTxt;
			break;

		    case FSOP_SYSTEM:
			i = IDS_B_SYS;
			goto SetStTxt;
			break;

		    case FSOP_DATA:
			i = IDS_B_DATA;
			goto SetStTxt;
			break;

		    case FOP_DDHEAD:
			i = IDS_DD_HEAD;
			goto SetStTxt;
			break;

		    case FOP_DDSTRUC:
			i = IDS_DD_STRUC;
			goto SetStTxt;
			break;

		    case FOP_DDFAT:
			i = IDS_DD_FAT;
			goto SetStTxt;
			break;

		    case FOP_DDSIG:
			i = IDS_DD_SIG;
			goto SetStTxt;
			break;

		    case FOP_DDBOOT:
			i = IDS_DD_BOOT;
			goto SetStTxt;
			break;
#ifdef OPK2
		    case FOP_BOOT:
			i = IDS_C_BOOT;
			goto SetStTxt;
			break;
#endif
#ifdef DOSETUPCHK
		    case FOP_SCANDIR:
			if(lpMyChkInf->CurrOpRegion != (BYTE)HIWORD(lpMyChkInf->lParam1))
			{
			    SetDlgItemText(hwnd, DLGCHK_STATTXT, "Scanning directories...");
			    lpMyChkInf->pTextIsComplt = FALSE;
			    lpMyChkInf->CurrOpRegion = (BYTE)HIWORD(lpMyChkInf->lParam1);
			}
			break;
#endif

		    case FOP_SHTDOWN:
			i = IDS_C_SHTDOWN;
SetStTxt:
#ifdef DOSETUPCHK
			if(lpMyChkInf->CurrOpRegion != (BYTE)HIWORD(lpMyChkInf->lParam1))
#else
			if(lpMyChkInf->CurrOpRegion != lpMyChkInf->lpFixFDisp->CurrOpRegion)
#endif
			{
			    pMsgBuf = (PSTR)LocalAlloc(LMEM_FIXED,TOTMSZ10);
			    if(pMsgBuf)
			    {
				LoadString(g_hInstance, i, FmtBuf10, SZFMTBUF10);
				SetDlgItemText(hwnd, DLGCHK_STATTXT, FmtBuf10);
				if(i == IDS_B_DATA)
				{
				    DWORD args[2];

				    args[0] = lpMyChkInf->lpFixFDisp->SurfAnProgCurr;
				    args[1] = lpMyChkInf->lpFixFDisp->SurfAnProgTot;
				    lpMyChkInf->SALstPos = lpMyChkInf->lpFixFDisp->SurfAnProgCurr;
				    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
						  (LPVOID)g_hInstance,
						  IDS_B_DATANUM,
						  0,
						  (LPSTR)lpMyChkInf->szScratch,
						  (DWORD)sizeof(lpMyChkInf->szScratch),
						  (LPDWORD)&(args));
				    SetDlgItemText(hwnd, DLGCHK_STATTXT2, lpMyChkInf->szScratch);
#ifdef OPK2
				} else if(i == IDS_B_SYS) {

				    DWORD args[2];

				    args[0] = lpMyChkInf->lpFixFDisp->SurfAnProgCurr;
				    args[1] = lpMyChkInf->lpFixFDisp->SurfAnProgTot;
				    lpMyChkInf->SALstPos = lpMyChkInf->lpFixFDisp->SurfAnProgCurr;
				    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
						  (LPVOID)g_hInstance,
						  IDS_B_SYSNUM,
						  0,
						  (LPSTR)lpMyChkInf->szScratch,
						  (DWORD)sizeof(lpMyChkInf->szScratch),
						  (LPDWORD)&(args));
				    SetDlgItemText(hwnd, DLGCHK_STATTXT2, lpMyChkInf->szScratch);
#endif
				} else {
				    SetDlgItemText(hwnd, DLGCHK_STATTXT2, g_szNULL);
				}
				lpMyChkInf->pTextIsComplt = FALSE;
				LocalFree((HANDLE)pMsgBuf);
			    } else {
				MyChkdskMessageBox(lpMyChkInf, IDS_NOMEM2,
						   MB_ICONINFORMATION | MB_OK);
			    }
#ifdef DOSETUPCHK
			    lpMyChkInf->CurrOpRegion = (BYTE)HIWORD(lpMyChkInf->lParam1);
#else
			    lpMyChkInf->CurrOpRegion = lpMyChkInf->lpFixFDisp->CurrOpRegion;
#endif
			}
			if((lpMyChkInf->CurrOpRegion == FSOP_DATA) &&
			   (lpMyChkInf->SALstPos != lpMyChkInf->lpFixFDisp->SurfAnProgCurr) )
			{
			   DWORD args[2];

			   args[0] = lpMyChkInf->lpFixFDisp->SurfAnProgCurr;
			   args[1] = lpMyChkInf->lpFixFDisp->SurfAnProgTot;
			   lpMyChkInf->SALstPos = lpMyChkInf->lpFixFDisp->SurfAnProgCurr;
			   FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
					 (LPVOID)g_hInstance,
					 IDS_B_DATANUM,
					 0,
					 (LPSTR)lpMyChkInf->szScratch,
					 (DWORD)sizeof(lpMyChkInf->szScratch),
					 (LPDWORD)&(args));
			   SetDlgItemText(hwnd, DLGCHK_STATTXT2, lpMyChkInf->szScratch);
#ifdef OPK2
			} else if((lpMyChkInf->CurrOpRegion == FSOP_SYSTEM) &&
				  (lpMyChkInf->SALstPos != lpMyChkInf->lpFixFDisp->SurfAnProgCurr) ) {

			   DWORD args[2];

			   args[0] = lpMyChkInf->lpFixFDisp->SurfAnProgCurr;
			   args[1] = lpMyChkInf->lpFixFDisp->SurfAnProgTot;
			   lpMyChkInf->SALstPos = lpMyChkInf->lpFixFDisp->SurfAnProgCurr;
			   FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
					 (LPVOID)g_hInstance,
					 IDS_B_SYSNUM,
					 0,
					 (LPSTR)lpMyChkInf->szScratch,
					 (DWORD)sizeof(lpMyChkInf->szScratch),
					 (LPDWORD)&(args));
			   SetDlgItemText(hwnd, DLGCHK_STATTXT2, lpMyChkInf->szScratch);
#endif
			}
			break;

		    default:
			break;		// Leave status unchanged
		}
	    }
	    if(lpMyChkInf->IsSplitDrv)
	    {
		if(lpMyChkInf->DoingCompDrv)
		{
#ifdef DOSETUPCHK
		    i = (WORD)(50 + (LOWORD(lpMyChkInf->lParam1) / 2));
#else
		    i = (WORD)(50 + (lpMyChkInf->lpFixFDisp->TotalPcntCmplt / 2));
#endif
		} else {
#ifdef DOSETUPCHK
		    i = (WORD)(LOWORD(lpMyChkInf->lParam1) / 2);
#else
		    i = (WORD)(lpMyChkInf->lpFixFDisp->TotalPcntCmplt / 2);
#endif
		}
		if(i < (WORD)lpMyChkInf->LstChkPcnt)
		{
		    InvalidateRect(GetDlgItem(hwnd,DLGCHK_PBAR),NULL,TRUE);
		}
		SendDlgItemMessage(hwnd, DLGCHK_PBAR, PBM_SETPOS, i, 0L);
		lpMyChkInf->LstChkPcnt = LOBYTE(i);
	    } else {
#ifdef DOSETUPCHK
		if((BYTE)LOWORD(lpMyChkInf->lParam1) < lpMyChkInf->LstChkPcnt)
#else
		if(lpMyChkInf->lpFixFDisp->TotalPcntCmplt < lpMyChkInf->LstChkPcnt)
#endif
		{
		    InvalidateRect(GetDlgItem(hwnd,DLGCHK_PBAR),NULL,TRUE);
		}
#ifdef DOSETUPCHK
		SendDlgItemMessage(hwnd, DLGCHK_PBAR, PBM_SETPOS, LOWORD(lpMyChkInf->lParam1), 0L);
		lpMyChkInf->LstChkPcnt = (BYTE)LOWORD(lpMyChkInf->lParam1);
#else
		SendDlgItemMessage(hwnd, DLGCHK_PBAR, PBM_SETPOS, lpMyChkInf->lpFixFDisp->TotalPcntCmplt, 0L);
		lpMyChkInf->LstChkPcnt = lpMyChkInf->lpFixFDisp->TotalPcntCmplt;
#endif
	    }
	    UpdateWindow(GetDlgItem(hwnd,DLGCHK_PBAR));
	    return(TRUE);
	    break;

#ifdef OPK2
	case WM_APP+5:
	    if((lpMyChkInf->hAniIcon1 != NULL) &&
	       (lpMyChkInf->hAniIcon2 != NULL) &&
	       (lpMyChkInf->hAniIcon3 != NULL)	 )
	    {
		if(lpMyChkInf->hCurrAniIcon == lpMyChkInf->hAniIcon1)
		{
		    lpMyChkInf->hCurrAniIcon = lpMyChkInf->hAniIcon2;
		} else if(lpMyChkInf->hCurrAniIcon == lpMyChkInf->hAniIcon2) {
		    lpMyChkInf->hCurrAniIcon = lpMyChkInf->hAniIcon3;
		} else {
		    lpMyChkInf->hCurrAniIcon = lpMyChkInf->hAniIcon1;
		}
		SendDlgItemMessage(hwnd, IDC_ICON_1, STM_SETIMAGE, IMAGE_ICON, MAKELONG((WORD)lpMyChkInf->hCurrAniIcon,0));
		InvalidateRect(GetDlgItem(hwnd,IDC_ICON_1),NULL,TRUE);
		UpdateWindow(GetDlgItem(hwnd,IDC_ICON_1));
	    }
	    return(TRUE);
	    break;
#endif

	case WM_HELP:
#ifdef OPK2
	    if(lpMyChkInf->fSageSet)
	    {
		WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
			(DWORD) (LPSTR) ChkaIdsSage);
	    } else {
#endif
		WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
			(DWORD) (LPSTR) ChkaIds);
#ifdef OPK2
	    }
#endif
	    return(TRUE);
	    break;

	case WM_CONTEXTMENU:
#ifdef OPK2
	    if(lpMyChkInf->fSageSet)
	    {
		WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
				(DWORD) (LPSTR) ChkaIdsSage);
	    } else {
#endif
		WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
				(DWORD) (LPSTR) ChkaIds);
#ifdef OPK2
	    }
#endif
	    return(TRUE);
	    break;

	case WM_DESTROY:
	    if(g_himlIconsSmall) {
		ImageList_Destroy(g_himlIconsSmall);
		g_himlIconsSmall = NULL;
	    }

	    // NOTE FALL THROUGH

	default:
	    return(FALSE);
	    break;
    }
    return(FALSE);
}

//
// Exported API to check drive dialog.
//
DWORD WINAPI SHCheckDrive(HWND hwnd, DWORD options, DWORD DrvList, HWND FAR* lpTLhwnd)
{
    SHCHECKDISKINFO   sSHChkInfo;
    LPMYCHKINFOSTRUCT lpMyChkInf;
    WNDCLASS	      wc;
    DWORD	      mret = SHCHK_ERROR;
    DWORD	      dwi;
    DWORD	      dwj;
    WORD	      i;

    if(lpTLhwnd)
	*lpTLhwnd = 0;

    if (!_InitTermDMaint(TRUE, &sSHChkInfo.sDMaint))
    {
	mret = SHCHK_ERRORINIT;
	goto Error0;
    }

    lpMyChkInf = (LPMYCHKINFOSTRUCT)(NPMYCHKINFOSTRUCT)
		 LocalAlloc(LPTR,sizeof(MYCHKINFOSTRUCT));
    if(LOWORD(lpMyChkInf) == 0)
    {
	mret = SHCHK_ERRORMEM;
	goto Error1;
    }
    sSHChkInfo.lpMyChkInf = lpMyChkInf;

    lpMyChkInf->lpwddi = (LPWINDISKDRIVEINFO)(NPWINDISKDRIVEINFO)
			 LocalAlloc(LPTR,sizeof(WINDISKDRIVEINFO));
    if(LOWORD(lpMyChkInf->lpwddi) == 0)
    {
	mret = SHCHK_ERRORMEM;
	goto Error2;
    }

#ifdef FROSTING
    lpMyChkInf->fSageSet = (options & SHCHK_OPT_SAGESET) ? TRUE : FALSE;
    lpMyChkInf->fSageRun = (options & SHCHK_OPT_SAGERUN) ? TRUE : FALSE;
    lpMyChkInf->fShouldRerun = FALSE;
    lpMyChkInf->SilentProbCnt = 0;

    if(lpMyChkInf->fSageSet || lpMyChkInf->fSageRun)
    {
	lpMyChkInf->idxSettings = DrvList;

	GetChkRegOptions (lpMyChkInf);

	DrvList = lpMyChkInf->DrivesToChk;
	options &= (SHCHK_OPT_MINIMIZED | SHCHK_OPT_SAGESET | SHCHK_OPT_SAGERUN);
	options |= lpMyChkInf->RegOptions &
		   (~(SHCHK_OPT_MINIMIZED | SHCHK_OPT_SAGESET | SHCHK_OPT_SAGERUN));
    }
    else
    {
	lpMyChkInf->idxSettings = (DWORD)0xFFFFFFFF;
    }
#endif

#ifdef FROSTING
    if(!lpMyChkInf->fSageSet && !lpMyChkInf->fSageRun)
#endif
    {
	if(DrvList == 0L)
	{
	    mret = SHCHK_NOCHK;
	    goto ChkDone;
	}
    }

    lpMyChkInf->DrivesToChk = DrvList;
    lpMyChkInf->lpTLhwnd    = lpTLhwnd;

    lpMyChkInf->hProgDlgWnd = hwnd;
    lpMyChkInf->hWndPar     = hwnd;

    if(lpMyChkInf->hWndPar == 0)
    {
	if(g_ChkWndClass == NULL)
	{
	    wc.style = CS_GLOBALCLASS;
	    wc.lpfnWndProc = DefDlgProc;
	    wc.cbClsExtra = 0;
	    wc.cbWndExtra = DLGWINDOWEXTRA;
	    wc.hInstance = g_hInstance;
	    wc.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_CHKICON));
	    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
	    wc.lpszMenuName =  NULL;
	    wc.lpszClassName = "ScanDskWDlgClass";

	    g_ChkWndClass = RegisterClass(&wc);
	}
    }

    lpMyChkInf->MyFixOpt  = LOWORD(options);
    lpMyChkInf->MyFixOpt2 = HIWORD(options);

#ifdef FROSTING
    if(!lpMyChkInf->fSageSet && !lpMyChkInf->fSageRun)
#endif
    {
	GetChkRegOptions(lpMyChkInf);
    }

    if(options & SHCHK_OPT_DEFOPTIONS)
    {
      lpMyChkInf->MyFixOpt = (lpMyChkInf->MyFixOpt & (WORD)(SHCHK_OPT_DEFOPTIONS |
							    SHCHK_OPT_DRVLISTONLY |
							    SHCHK_OPT_AUTO))	     |
			     (LOWORD(lpMyChkInf->RegOptions) & (WORD)(~(SHCHK_OPT_DEFOPTIONS |
									SHCHK_OPT_DRVLISTONLY |
									SHCHK_OPT_AUTO)));
      lpMyChkInf->MyFixOpt2 = HIWORD(lpMyChkInf->RegOptions) & ~(DLGCHK_MKOLDFS |
								 DLGCHK_PROGONLY |
								 DLGCHK_NOWND |
								 DLGCHK_MINIMIZED);
      //
      // There is currently no UI for these in advanced, we need to make sure
      // these bits are off to that if these bits are set in the registry
      // (there was UI for these in one of the betas of Win95) we will ignore
      // them and turn them off when we re-record the registry options.
      // This needs to be removed if these bits are ever defined as valid
      // again.
      //
      lpMyChkInf->MyFixOpt2 &= ~(DLGCHK_EXCLU | DLGCHK_FLWRT);
    }

    if(options & SHCHK_OPT_MKOLDFS)
    {
	// NOTE that this triggers some automatic option changes. This is
	// why we don't want the registry options set when we are running
	// in this mode.

	lpMyChkInf->MyFixOpt2 |= DLGCHK_MKOLDFS;
	lpMyChkInf->MyFixOpt  &= ~(DLGCHK_RO | DLGCHK_REP);
	lpMyChkInf->MyFixOpt  |= DLGCHK_NOBAD;
    }
    if(options & SHCHK_OPT_RO)
    {
	lpMyChkInf->MyFixOpt  |= DLGCHK_RO;
    } else {
	lpMyChkInf->MyFixOpt  &= ~(DLGCHK_RO);
    }

    if(options & SHCHK_OPT_PROGONLY)
    {
	lpMyChkInf->MyFixOpt2 |= DLGCHK_PROGONLY;
	// Below is handled at the DoReport: label above
	// lpMyChkInf->MyFixOpt  &= ~(DLGCHK_REP);
    }

    if(options & SHCHK_OPT_NOWND)
    {
	lpMyChkInf->MyFixOpt2 |= DLGCHK_NOWND;
	lpMyChkInf->MyFixOpt2 &= ~DLGCHK_PROGONLY;
	lpMyChkInf->MyFixOpt  |= DLGCHK_AUTO;
	// Below is handled at the DoReport: label above
	// lpMyChkInf->MyFixOpt  &= ~(DLGCHK_REP);
    }
    if(options & SHCHK_OPT_MINIMIZED)
    {
	lpMyChkInf->ShowMinimized = TRUE;
    }

#if SHCHK_OPT_REP	     != DLGCHK_REP
    ERROR defines do not match
#endif
#if SHCHK_OPT_RO	     != DLGCHK_RO
    ERROR defines do not match
#endif
#if SHCHK_OPT_NOSYS     != DLGCHK_NOSYS
    ERROR defines do not match
#endif
#if SHCHK_OPT_NODATA    != DLGCHK_NODATA
    ERROR defines do not match
#endif
#if SHCHK_OPT_NOBAD     != DLGCHK_NOBAD
    ERROR defines do not match
#endif
#if SHCHK_OPT_LSTMF     != DLGCHK_LSTMF
    ERROR defines do not match
#endif
#if SHCHK_OPT_NOCHKNM   != DLGCHK_NOCHKNM
    ERROR defines do not match
#endif
#if SHCHK_OPT_NOCHKDT   != DLGCHK_NOCHKDT
    ERROR defines do not match
#endif
#if SHCHK_OPT_INTER     != DLGCHK_INTER
    ERROR defines do not match
#endif
#if SHCHK_OPT_XLCPY     != DLGCHK_XLCPY
    ERROR defines do not match
#endif
#if SHCHK_OPT_XLDEL     != DLGCHK_XLDEL
    ERROR defines do not match
#endif
#if SHCHK_OPT_ALLHIDSYS != DLGCHK_ALLHIDSYS
    ERROR defines do not match
#endif
#if SHCHK_OPT_NOWRTTST  != DLGCHK_NOWRTTST
    ERROR defines do not match
#endif
#if SHCHK_OPT_AUTO  != DLGCHK_AUTO
    ERROR defines do not match
#endif
#if SHCHK_OPT_DEFOPTIONS  != DLGCHK_DEFOPTIONS
    ERROR defines do not match
#endif
#if SHCHK_OPT_DRVLISTONLY != DLGCHK_DRVLISTONLY
    ERROR defines do not match
#endif
#if (SHCHK_OPT_EXCLULOCK >> 16) != DLGCHK_EXCLU
    ERROR defines do not match
#endif
#if (SHCHK_OPT_FLWRTLOCK >> 16) != DLGCHK_FLWRT
    ERROR defines do not match
#endif
#if (SHCHK_OPT_MKOLDFS >> 16) != DLGCHK_MKOLDFS
    ERROR defines do not match
#endif
#if (SHCHK_OPT_PROGONLY >> 16) != DLGCHK_PROGONLY
    ERROR defines do not match
#endif
#if (SHCHK_OPT_NOWND >> 16) != DLGCHK_NOWND
    ERROR defines do not match
#endif
#if (SHCHK_OPT_NOCHKHST >> 16) != DLGCHK_NOCHKHST
    ERROR defines do not match
#endif
#if (SHCHK_OPT_MINIMIZED >> 16) != DLGCHK_MINIMIZED
    ERROR defines do not match
#endif
#if (SHCHK_OPT_REPONLYERR >> 16) != DLGCHK_REPONLYERR
    ERROR defines do not match
#endif
#if (SHCHK_OPT_NOLOG >> 16) != DLGCHK_NOLOG
    ERROR defines do not match
#endif
#if (SHCHK_OPT_LOGAPPEND >> 16) != DLGCHK_LOGAPPEND
    ERROR defines do not match
#endif

    if(lpMyChkInf->MyFixOpt2 & DLGCHK_NOWND)
    {
	// Basically we need to do all the steps that WM_INITDIALOG and
	// WM_COMMAND,DLGCHK_START do in ChkDlgWndProc

	lpMyChkInf->HstDrvsChckd = 0L;
	lpMyChkInf->hProgDlgWnd = 0;
	lpMyChkInf->LastChkRslt = LASTCHKRSLT_ERR;
#ifdef FROSTING
	SetMultiDrvRslt(lpMyChkInf);
#endif

	if(lpMyChkInf->DrivesToChk == 0L)
	{
	    goto ChkDoneA;
	}

	// Set IsMultiDrv

	dwi = 0x00000001L;
	dwj = 0L;
	for(i = 0; i < 26; i++)
	{
	    if(lpMyChkInf->DrivesToChk & dwi)
	    {
		dwj++;
	    }
	    dwi = dwi << 1;
	}
	if(dwj > 1)
	    lpMyChkInf->IsMultiDrv = TRUE;
	else
	    lpMyChkInf->IsMultiDrv = FALSE;

	// Set lpMyChkInf->LstChkdDrv to the first drive
	// specified in DrivesToChk

	dwi = 0x00000001L;
	for(i = 0; i < 26; i++)
	{
	    if(lpMyChkInf->DrivesToChk & dwi)
	    {
		lpMyChkInf->LstChkdDrv = i;
		lpMyChkInf->IsFirstDrv = TRUE;
		goto InitNext;
	    }
	    dwi = dwi << 1;
	}
	goto ChkDoneA;
InitNext:
	lpMyChkInf->IsSplitDrv	 = FALSE;
	lpMyChkInf->DoingCompDrv = FALSE;
	lpMyChkInf->lpwddi->iDrive = lpMyChkInf->LstChkdDrv;
	InitDrvInfo(0, lpMyChkInf->lpwddi, &sSHChkInfo.sDMaint);

	if((lpMyChkInf->lpwddi->TypeFlags & DT_DBLDISK) &&
	   (!(lpMyChkInf->MyFixOpt2 & DLGCHK_NOCHKHST))   )
	{
	    lpMyChkInf->CompDrv = lpMyChkInf->lpwddi->iDrive;
	    lpMyChkInf->HostDrv = GetCompInfo(lpMyChkInf->CompDrv,&i);
	    if(lpMyChkInf->HostDrv != 0xFFFF)
	    {
		dwi = 0x00000001L << lpMyChkInf->HostDrv;
		if(lpMyChkInf->HstDrvsChckd & dwi)
		    goto InitNexta;

		if((lpMyChkInf->DrivesToChk & dwi) &&
		   (lpMyChkInf->HostDrv <= lpMyChkInf->LstChkdDrv))
		    goto InitNexta;

		lpMyChkInf->IsSplitDrv	 = TRUE;
		lpMyChkInf->DoingCompDrv = FALSE;
		lpMyChkInf->lpwddi->iDrive = lpMyChkInf->HostDrv;
		InitDrvInfo(0, lpMyChkInf->lpwddi, &sSHChkInfo.sDMaint);
	    }
	}
InitNexta:

	// Init for CHKDSK

	i = sSHChkInfo.sDMaint.lpfnGetFileSysParameters(lpMyChkInf->lpwddi->iDrive,
					&(lpMyChkInf->ChkDPms),
					sizeof(lpMyChkInf->ChkDPms));
	switch(i) {
	    case FS_FAT:
	    case FS_DDFAT:
	    case FS_LFNFAT:
	    case FS_DDLFNFAT:
		lpMyChkInf->NoParms = FALSE;
		break;

	    default:
		lpMyChkInf->NoParms = TRUE;
GetNextDrive:
		if((lpMyChkInf->IsMultiDrv)  &&
		   (lpMyChkInf->ChkEngCancel)  )
		{
		    goto ChkDoneA;
		}

		// Set lpMyChkInf->LstChkdDrv to the next drive
		// specified in DrivesToChk

		dwi = 0x00000001L << (lpMyChkInf->LstChkdDrv + 1);
		for(i = lpMyChkInf->LstChkdDrv + 1; i < 26; i++)
		{
		    if(lpMyChkInf->DrivesToChk & dwi)
		    {
			lpMyChkInf->LstChkdDrv = i;
			goto InitNext;
		    }
		    dwi = dwi << 1;
		}
		goto ChkDoneA;
	}

	dwj = sSHChkInfo.sDMaint.lpfnGetFixOptions(&(lpMyChkInf->ChkDPms));

	if(dwj & (FSINVALID | FSDISALLOWED))
	{
	    goto GetNextDrive;
	}

	lpMyChkInf->FixOptions = dwj;

	lpMyChkInf->CurrOpRegion = 0xFFFF;

	dwi = 0L;

	// Below is handled at the DoReport: label above
	// lpMyChkInf->MyFixOpt &= ~DLGCHK_REP;

	if(lpMyChkInf->MyFixOpt & DLGCHK_NOBAD)
	{
	    dwi |= FDOS_NOSRFANAL;
	} else {
	    if(!(lpMyChkInf->MyFixOpt & DLGCHK_NOWRTTST))
		dwi |= FDOS_WRTTST;

	    if(lpMyChkInf->MyFixOpt & DLGCHK_ALLHIDSYS)
		dwi |= FDOSFAT_NMHISSYS;

	    if(lpMyChkInf->MyFixOpt & DLGCHK_NODATA)
	    {
		dwi |= FDOSFAT_NODATATST;
	    } else if(lpMyChkInf->MyFixOpt & DLGCHK_NOSYS) {
		dwi |= FDOSFAT_NOSYSTST;
	    }
	}

	if(lpMyChkInf->MyFixOpt & DLGCHK_RO)
	{
	    dwi |= FDO_NOFIX;
	} else {
	    if(lpMyChkInf->ChkDPms.drvprm.FatFS.FSFlags & FSFF_DMFFLOPPY)
	    {
		dwi |= FDO_NOFIX;
	    }
	}

	if(lpMyChkInf->MyFixOpt & DLGCHK_LSTMF)
	    dwi |= FDOFAT_LSTMKFILE;

	if(lpMyChkInf->MyFixOpt & DLGCHK_XLCPY)
	{
	    dwi |= FDOFAT_XLNKCPY;
	} else if(lpMyChkInf->MyFixOpt & DLGCHK_XLDEL) {
	    dwi |= FDOFAT_XLNKDEL;
	}
	if(lpMyChkInf->MyFixOpt & DLGCHK_NOCHKNM)
	    dwi |= FDOFAT_NOCHKNM;

	if(lpMyChkInf->MyFixOpt & DLGCHK_NOCHKDT)
	    dwi |= FDOFAT_NOCHKDT;

	if(lpMyChkInf->MyFixOpt2 & DLGCHK_EXCLU)
	{
	    dwi |= FDO_LOCKEXCLUSIVE;
	} else if(lpMyChkInf->MyFixOpt2 & DLGCHK_FLWRT) {
	    dwi |= FDO_LOCKFAILWRT;
	} else {
	    dwi |= FDO_LOCKBLKWRT;
	}

	if(lpMyChkInf->MyFixOpt2 & DLGCHK_MKOLDFS)
	{
	    dwi |= FDOFAT_MKOLDFS;

	    // Do not record settings in registry when running in this
	    // mode because this mode auto-changes some settings and
	    // we don't want those settings recorded if they are different
	    // than the ones the user has set.

	} else {
	    SetChkRegOptions(MAKELONG((lpMyChkInf->MyFixOpt &  (~(SHCHK_OPT_DEFOPTIONS |
								  SHCHK_OPT_DRVLISTONLY |
								  SHCHK_OPT_AUTO))),
				      (lpMyChkInf->MyFixOpt2 & (~(SHCHK_OPT_MKOLDFS)))),
#ifdef FROSTING
				      lpMyChkInf->NoUnsupDrvs,
				      lpMyChkInf->DrivesToChk,
				      lpMyChkInf->idxSettings);
#else
				      lpMyChkInf->NoUnsupDrvs);
#endif
	}

	lpMyChkInf->ChkCancelBool = FALSE;
	lpMyChkInf->SrfInProgBool = FALSE;
	lpMyChkInf->AlrdyRestartWrn = FALSE;
#ifdef OPK2
	lpMyChkInf->Done3PtyCompWrn = FALSE;
#endif
	lpMyChkInf->ChkInProgBool = TRUE;
	lpMyChkInf->ChkEngCancel  = FALSE;
	lpMyChkInf->RWRstsrtCnt   = 0;
	lpMyChkInf->LstChkPcnt	  = 0;
	lpMyChkInf->NoRstrtWarn   = FALSE;
	lpMyChkInf->DoSARestart   = FALSE;
	lpMyChkInf->DoCHRestart   = FALSE;

	SEInitLog(lpMyChkInf);
	SEAddToLogStart(lpMyChkInf,lpMyChkInf->IsFirstDrv);

	dwi = sSHChkInfo.sDMaint.lpfnFixDrive(&(lpMyChkInf->ChkDPms),
			      dwi,
			      ChkCBProc,
			      (LPARAM)lpMyChkInf);

	lpMyChkInf->ChkInProgBool = FALSE;
	lpMyChkInf->AlrdyRestartWrn = FALSE;
#ifdef OPK2
	lpMyChkInf->Done3PtyCompWrn = FALSE;
	if(lpMyChkInf->hAniIcon1 != NULL)
	{
	    SendDlgItemMessage(hwnd, IDC_ICON_1, STM_SETIMAGE, IMAGE_ICON, MAKELONG((WORD)lpMyChkInf->hAniIcon1,0));
	    lpMyChkInf->hCurrAniIcon = lpMyChkInf->hAniIcon1;
	    InvalidateRect(GetDlgItem(hwnd,IDC_ICON_1),NULL,TRUE);
	    UpdateWindow(GetDlgItem(hwnd,IDC_ICON_1));
	}
#endif

	switch (HIWORD(dwi))
	{
	    // These errors occurs before
	    //	any call backs are made

	    case ERR_BADOPTIONS:
		lpMyChkInf->DoCHRestart = FALSE;
		lpMyChkInf->DoSARestart = FALSE;
		SEAddToLogRCS(lpMyChkInf,IDL_ENGBADOPT,NULL);
		i = IDS_UNEXP3;
		goto DoErr2;
		break;

	    case ERR_NOTSUPPORTED:
		lpMyChkInf->DoCHRestart = FALSE;
		lpMyChkInf->DoSARestart = FALSE;
		SEAddToLogRCS(lpMyChkInf,IDL_ENGNOTSUPP,NULL);
		switch (lpMyChkInf->lpwddi->iType)
		{
		    case DRIVE_REMOVABLE:
			i = IDS_UNSUPR;
			break;

		    case DRIVE_RAMDRIVE:
		    case DRIVE_FIXED:
			i = IDS_UNSUP;
			break;

		    case DRIVE_CDROM:
		    case DRIVE_REMOTE:
		    default:
			i = IDS_INVALID;
			break;
		}
		goto DoErr2;
		break;

	    // These errors may be
	    //	correctable

	    case ERR_NOTWRITABLE:
		lpMyChkInf->DoCHRestart = FALSE;
		lpMyChkInf->DoSARestart = FALSE;
		SEAddToLogRCS(lpMyChkInf,IDL_ENGCANTWRT,NULL);
		i = IDS_CANTWRT;
		goto DoErr2;
		break;

	    case ERR_OSERR:
		lpMyChkInf->DoCHRestart = FALSE;
		lpMyChkInf->DoSARestart = FALSE;
		SEAddToLogRCS(lpMyChkInf,IDL_ENGOS,NULL);
		i = IDS_SERDISK;
		goto DoErr2;
		break;

	    // These errors may go away
	    //	if it is tried again.

	    case ERR_FSERR:
		lpMyChkInf->DoCHRestart = FALSE;
		lpMyChkInf->DoSARestart = FALSE;
		SEAddToLogRCS(lpMyChkInf,IDL_ENGFS,NULL);
		i = IDS_SERFS;
		goto DoErr2;
		break;

	    case ERR_FSACTIVE:
		lpMyChkInf->DoCHRestart = FALSE;
		lpMyChkInf->DoSARestart = FALSE;
		SEAddToLogRCS(lpMyChkInf,IDL_ENGACTIVE,NULL);
		i = IDS_ACTIVE;
		goto DoErr2;
		break;

	    case ERR_LOCKVIOLATION:
		lpMyChkInf->DoCHRestart = FALSE;
		lpMyChkInf->DoSARestart = FALSE;
		SEAddToLogRCS(lpMyChkInf,IDL_ENGLOCK,NULL);
		if(lpMyChkInf->MyFixOpt2 & DLGCHK_EXCLU)
		    i = IDS_LOCKVIOL2;
		else
		    i = IDS_LOCKVIOL;
		goto DoErr2;
		break;

	    case ERR_INSUFMEM:
		lpMyChkInf->DoCHRestart = FALSE;
		lpMyChkInf->DoSARestart = FALSE;
		SEAddToLogRCS(lpMyChkInf,IDL_ENGMEM,NULL);
		i = IDS_NOMEM3;
DoErr2:
		lpMyChkInf->LastChkRslt = LASTCHKRSLT_ERR;
#ifdef FROSTING
		SetMultiDrvRslt(lpMyChkInf);
#endif
		break;

	    case OPCANCEL:
		if((!lpMyChkInf->DoCHRestart) &&
		   (!lpMyChkInf->DoSARestart)	)
		{
		    SEAddToLogRCS(lpMyChkInf,IDL_ENGCANCEL,NULL);
		    lpMyChkInf->ChkEngCancel = TRUE;
		}
		lpMyChkInf->LastChkRslt = LASTCHKRSLT_CAN;
#ifdef FROSTING
		SetMultiDrvRslt(lpMyChkInf);
#endif
		if(lpMyChkInf->DoCHRestart)
		{
		   SEAddToLogRCS(lpMyChkInf,IDL_ENGCHKHSTRST,NULL);
		   lpMyChkInf->MyFixOpt2 &= ~DLGCHK_NOCHKHST;
		   goto TurnSA;
		}
		lpMyChkInf->DoCHRestart = FALSE;
		if(lpMyChkInf->DoSARestart)
		{
		    SEAddToLogRCS(lpMyChkInf,IDL_ENGSARST,NULL);
TurnSA:
		    lpMyChkInf->MyFixOpt &= ~(DLGCHK_NOBAD | DLGCHK_NODATA | DLGCHK_NOSYS);
		    lpMyChkInf->DoCHRestart = FALSE;
		    lpMyChkInf->DoSARestart = FALSE;
		    lpMyChkInf->IsFirstDrv = FALSE;
		    goto InitNext;
		}
		break;

	    case ERR_FSUNCORRECTED:
		lpMyChkInf->DoCHRestart = FALSE;
		lpMyChkInf->DoSARestart = FALSE;
		lpMyChkInf->LastChkRslt = LASTCHKRSLT_SMNOTFIX;
#ifdef FROSTING
		SetMultiDrvRslt(lpMyChkInf);
#endif
		i = DTRESULTPROB;
		goto RecordLstRun;
		break;

	    case NOERROR:
		lpMyChkInf->DoCHRestart = FALSE;
		lpMyChkInf->DoSARestart = FALSE;
		lpMyChkInf->LastChkRslt = LASTCHKRSLT_NOERROR;
#ifdef FROSTING
		SetMultiDrvRslt(lpMyChkInf);
#endif
		i = DTRESULTOK;
		goto RecordLstRun;
		break;

	    case ERR_FSCORRECTED:
		lpMyChkInf->DoCHRestart = FALSE;
		lpMyChkInf->DoSARestart = FALSE;
		lpMyChkInf->LastChkRslt = LASTCHKRSLT_ALLFIXED;
#ifdef FROSTING
		SetMultiDrvRslt(lpMyChkInf);
#endif
		if(lpMyChkInf->MyFixOpt & DLGCHK_RO)
		{
		    i = DTRESULTPROB;
		} else {
		    i = DTRESULTFIX;
		}
RecordLstRun:
		if(lpMyChkInf->lpwddi->iType != DRIVE_REMOVABLE)
		{
		    char szDrive[] = "A";

		    szDrive[0] += lpMyChkInf->lpwddi->iDrive;

		    SaveTimeInReg(HKEY_LOCAL_MACHINE,
				  REGSTR_PATH_LASTCHECK,
				  szDrive,i,TRUE);

		    SaveTimeInReg(HKEY_LOCAL_MACHINE,
				  REGSTR_PATH_CHKLASTCHECK,
				  szDrive,i,FALSE);

		    if((!(lpMyChkInf->MyFixOpt & DLGCHK_NOBAD)) &&
		       (!(lpMyChkInf->MyFixOpt & DLGCHK_NODATA))  )
		    {
			SaveTimeInReg(HKEY_LOCAL_MACHINE,
				      REGSTR_PATH_CHKLASTSURFAN,
				      szDrive,i,FALSE);
		    }
		}
		break;

	    default:
		lpMyChkInf->DoCHRestart = FALSE;
		lpMyChkInf->DoSARestart = FALSE;
		lpMyChkInf->LastChkRslt = LASTCHKRSLT_ERR;
#ifdef FROSTING
		SetMultiDrvRslt(lpMyChkInf);
#endif
		SEAddToLogRCS(lpMyChkInf,IDL_ENGUXP,NULL);
		break;		    // just continue
	}

	SEAddToLogRCS(lpMyChkInf,IDL_TRAILER,NULL);
	if(lpMyChkInf->IsFirstDrv)
	{
	    SERecordLog(lpMyChkInf,FALSE);
	} else {
	    SERecordLog(lpMyChkInf,TRUE);
	}

	if(lpMyChkInf->IsSplitDrv)
	{
	    if(!(lpMyChkInf->DoingCompDrv))
	    {
		lpMyChkInf->HstDrvsChckd |= 0x00000001L << lpMyChkInf->HostDrv;
		lpMyChkInf->DoingCompDrv = TRUE;
		lpMyChkInf->lpwddi->iDrive = lpMyChkInf->CompDrv;
		InitDrvInfo(0, lpMyChkInf->lpwddi, &sSHChkInfo.sDMaint);
		lpMyChkInf->IsFirstDrv = FALSE;
		goto InitNexta;
	    }
	    lpMyChkInf->IsSplitDrv = FALSE;
	    lpMyChkInf->DoingCompDrv = FALSE;
	}
	lpMyChkInf->IsFirstDrv = FALSE;
	goto GetNextDrive;
    } else {

	if((lpMyChkInf->hWndPar == 0) && (g_ChkWndClass != NULL))
	{
	    DialogBoxParam(g_hInstance,
			   MAKEINTRESOURCE(DLG_CHKDSKTL),
			   hwnd,
			   ChkDlgWndProc,
			   (LPARAM)(LPSTR)&sSHChkInfo);
	} else {
	    lpMyChkInf->ShowMinimized = FALSE;
	    DialogBoxParam(g_hInstance,
			   MAKEINTRESOURCE(DLG_CHKDSK),
			   hwnd,
			   ChkDlgWndProc,
			   (LPARAM)(LPSTR)&sSHChkInfo);
	}
    }

    lpMyChkInf->hProgDlgWnd = hwnd;

ChkDoneA:

#ifdef FROSTING
    if(lpMyChkInf->fShouldRerun == TRUE)
        mret = SHCHK_RERUN;
    else if(lpMyChkInf->MultLastChkRslt == LASTCHKRSLT_ERR)
	mret = SHCHK_ERROR;
    else if(lpMyChkInf->MultLastChkRslt == LASTCHKRSLT_CAN)
	mret = SHCHK_CANCEL;
    else if(lpMyChkInf->MultLastChkRslt == LASTCHKRSLT_SMNOTFIX)
	mret = SHCHK_SMNOTFIX;
    else if(lpMyChkInf->MultLastChkRslt == LASTCHKRSLT_ALLFIXED)
	mret = SHCHK_ALLFIXED;
    else if(lpMyChkInf->MultLastChkRslt == LASTCHKRSLT_NOERROR)
	mret = SHCHK_NOERROR;
#else
    if(lpMyChkInf->LastChkRslt == LASTCHKRSLT_ERR)
	mret = SHCHK_ERROR;
    else if(lpMyChkInf->LastChkRslt == LASTCHKRSLT_CAN)
	mret = SHCHK_CANCEL;
    else if(lpMyChkInf->LastChkRslt == LASTCHKRSLT_SMNOTFIX)
	mret = SHCHK_SMNOTFIX;
    else if(lpMyChkInf->LastChkRslt == LASTCHKRSLT_ALLFIXED)
	mret = SHCHK_ALLFIXED;
    else if(lpMyChkInf->LastChkRslt == LASTCHKRSLT_NOERROR)
	mret = SHCHK_NOERROR;
#endif
    else
	mret = 0L;

ChkDone:
    LocalFree((HANDLE)LOWORD(lpMyChkInf->lpwddi));
Error2:;
    if(lpMyChkInf->hIcon)
	DestroyIcon(lpMyChkInf->hIcon);
#ifdef OPK2
    if(lpMyChkInf->hAniIcon2)
	DestroyIcon(lpMyChkInf->hAniIcon2);
    if(lpMyChkInf->hAniIcon3)
	DestroyIcon(lpMyChkInf->hAniIcon3);
#endif
    LocalFree((HANDLE)LOWORD(lpMyChkInf));
Error1:;
    _InitTermDMaint(FALSE, &sSHChkInfo.sDMaint);
Error0:;
    return(mret);
}


//
// I guess I will only allow a single drive to be checked at this time
//
void WINAPI _RunDLLCheckDrive(HWND hwndStub, HINSTANCE hAppInstance,
	LPSTR lpszCmdLine, int nCmdShow)
{
	DWORD dwi;
	int   iDrive;
	int   i;

	if(lstrcmpi(lpszCmdLine+1, ":\\") != 0)  // !PathIsRoot(lpszCmdLine))
	{
	    return;
        }

	iDrive = *lpszCmdLine - 'A';
	if (iDrive >= 26)
	{
		return;
	}

	if(g_ChkWndPar)
	{
	    // First, validate that this hwnd is in fact still valid.
	    // This deals with the abnormal termination case.

	    if(IsWindow(g_ChkWndPar))
	    {
		if (IsIconic(g_ChkWndPar))
		{
		    ShowWindow(g_ChkWndPar, SW_RESTORE);
		} else {
		    SetActiveWindow(GetLastActivePopup(g_ChkWndPar));
		}
		goto AllDone;
	    } else {
		g_ChkWndPar = NULL;
	    }
	}

	g_ChkWndPar = hwndStub;

	dwi = SHCheckDrive(hwndStub, SHCHK_OPT_DEFOPTIONS, 1L << (DWORD)iDrive,NULL);

	if(dwi == SHCHK_ERRORINIT)
	{
	    i = IDS_NODSKMNT;
	    goto DoErr;
	} else if(dwi == SHCHK_ERRORMEM) {
	    i = IDS_NOMEM2;
DoErr:
	    ShellMessageBox(g_hInstance, hwndStub,
			    MAKEINTRESOURCE(i),
			    MAKEINTRESOURCE(IDS_CHKTIT),
			    MB_ICONINFORMATION | MB_OK);
	}
	g_ChkWndPar = NULL;
AllDone:
	;
}
