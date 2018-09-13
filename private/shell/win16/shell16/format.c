//
// format.c : Control Panel Applet for Windows Disk Manager
//

#include "shprv.h"

#include <dskmaint.h>
#include <prsht.h>

#include "util.h"
#include "format.h"
#include <help.h>

//
// Special internal only flag used for "Make Bootable"
//
#define WD_FMT_OPT_SYS	0x8000

#define g_hInstance g_hinst
static  char g_szNULL[] = "";   // c_szNull

static DWORD FORMATSEG FmtaIds[]={DLGCONFOR_START,  IDH_FORMATDLG_START,
				  DLGCONFOR_CANCEL, IDH_CANCEL,
				  DLGCONFOR_CAPCOMB,IDH_FORMATDLG_CAPACITY,
				  DLGCONFOR_FULL,   IDH_FORMATDLG_FULL,
				  DLGCONFOR_QUICK,  IDH_FORMATDLG_QUICK,
				  DLGCONFOR_DOSYS,  IDH_FORMATDLG_DOSYS,
				  DLGCONFOR_PBAR,   0xFFFFFFFFL,
				  DLGCONFOR_STATTXT,0xFFFFFFFFL,
				  DLGCONFOR_LABEL,  IDH_FORMATDLG_LABEL,
				  DLGCONFOR_NOLAB,  IDH_FORMATDLG_NOLAB,
				  DLGCONFOR_MKSYS,  IDH_FORMATDLG_MKSYS,
				  DLGCONFOR_REPORT, IDH_FORMATDLG_REPORT,
				  IDC_TEXT,         IDH_FORMATDLG_CAPACITY,
				  IDC_TEXT_2,       IDH_FORMATDLG_LABEL,
				  IDC_GROUPBOX_1,   IDH_COMM_GROUPBOX,
				  IDC_GROUPBOX_2,   IDH_COMM_GROUPBOX,
				  0,0};

typedef struct _SHFORMATDRIVEINFO
{
	DMAINTINFO sDMaint;
	LPMYFMTINFOSTRUCT lpMyFmtInf;
} SHFORMATDRIVEINFO, FAR *LPSHFORMATDRIVEINFO;


BOOL FAR PASCAL _InitTermDMaint(BOOL bInit, LPDMAINTINFO lpDMaint)
{
	if (!bInit)
	{
		FreeLibrary(lpDMaint->hInstDMaint);
		return(TRUE);
	}

	lpDMaint->hInstDMaint = LoadLibrary("dskmaint.dll");
	if ((UINT)lpDMaint->hInstDMaint < 32)
	{
		return(FALSE);
	}

	(FARPROC)lpDMaint->lpfnGetEngineDriveInfo =
		GetProcAddress(lpDMaint->hInstDMaint, "DMaint_GetEngineDriveInfo");
	(FARPROC)lpDMaint->lpfnFormatDrive =
		GetProcAddress(lpDMaint->hInstDMaint, "DMaint_FormatDrive");
	(FARPROC)lpDMaint->lpfnGetFormatOptions =
		GetProcAddress(lpDMaint->hInstDMaint, "DMaint_GetFormatOptions");

	(FARPROC)lpDMaint->lpfnGetFixOptions =
		GetProcAddress(lpDMaint->hInstDMaint, "DMaint_GetFixOptions");
	(FARPROC)lpDMaint->lpfnFixDrive =
		GetProcAddress(lpDMaint->hInstDMaint, "DMaint_FixDrive");
	(FARPROC)lpDMaint->lpfnGetFileSysParameters =
		GetProcAddress(lpDMaint->hInstDMaint, "DMaint_GetFileSysParameters");

	if (!lpDMaint->lpfnGetEngineDriveInfo
		|| !lpDMaint->lpfnFormatDrive
		|| !lpDMaint->lpfnGetFormatOptions)
	{
		FreeLibrary(lpDMaint->hInstDMaint);
		return(FALSE);
	}

	if (!lpDMaint->lpfnGetFixOptions
		|| !lpDMaint->lpfnFixDrive
		|| !lpDMaint->lpfnGetFileSysParameters)
	{
		FreeLibrary(lpDMaint->hInstDMaint);
		return(FALSE);
	}

	return(TRUE);
}

#pragma optimize("lge",off)

VOID FAR int1(VOID)
{
    _asm {
	int	1
    }
}

/*--------------------------------------------------------------------------
 *
 * int NEAR DeviceParameters(int drive, PDevPB pDevPB, WORD wFunction);
 *
 *  drive	0 based drive number
 *  pDevPB	pointer to device parameter block to set/get
 *  wFunction	0x40 to Set, 0x60 to Get
 *
 *  returns	0 if no error
 *
 *------------------------------------------------------------------------*/

#ifdef OPK2
int NEAR PASCAL DeviceParameters(int drive, LPDevPB lpDevPB, WORD wFunction)
{
    BYTE	DoFunc;
    int 	RetVal;

    DoFunc = lpDevPB->CallForm;

    _asm {
	push	ds
	mov	ax,0x440D	// IOCTL: Generic I/O Control for Block Devices
	lds	dx,lpDevPB
	mov	bx,drive
	inc	bx		// 1-based drive numbers
	mov	cx,wFunction	// Get/Set Device Parameters
	mov	ch,DoFunc
	cmp	ch,0x48
	je	GotForm
	cmp	ch,0x08
	je	GotForm
	mov	ch,0x48 	// Minor Code
	int	0x21
	jnc	SetForm
	mov	ch,0x08 	// Minor Code
	mov	ax,0x440D	// IOCTL: Generic I/O Control for Block Devices
	int	0x21
	jc	GDP_Error
      SetForm:
	mov	DoFunc,ch
	jmp	short GDP_SetRet

      GotForm:
	int	0x21
	jc	GDP_Error
      GDP_SetRet:
	xor	ax, ax		// No error
      GDP_Error:		// Error code in AX
	mov	RetVal,ax
	pop	ds
    }

    lpDevPB->CallForm = DoFunc;
    return(RetVal);
}
#else
int NEAR PASCAL DeviceParameters(int drive, LPDevPB lpDevPB, WORD wFunction)
{
    _asm {
	push	ds
	mov	ax,0x440D	// IOCTL: Generic I/O Control for Block Devices
	lds	dx,lpDevPB
	mov	bx,drive
	inc	bx		// 1-based drive numbers
	mov	cx,wFunction	// Get/Set Device Parameters
	mov	ch,0x08 	// Minor Code
	int	0x21
	jc	GDP_Error
	xor	ax, ax		// No error
      GDP_Error:		// Error code in AX
	pop	ds
    }
    if (0)
	return(0);		/* remove warning, gets otimized out */
}
#endif

DWORD NEAR GetDriveSubType(LPWINDISKDRIVEINFO lpwddi)
{
    DWORD      retval = 0L;
    BOOL       DDFlg = FALSE;
    BOOL       LFNFlg = FALSE;
    BYTE       FNm[128];
    BYTE       DNm[] = {"A:\\"};
    int        iDrive;		/* in line asm won't accept lpwddi-> */
    WORD       fnsz;		/* in line asm won't accept lpwddi-> */
    WORD       fpsz;		/* in line asm won't accept lpwddi-> */
    DevPB      dpbDiskParms;	/* Device Parameters */

#ifdef OPK2
    dpbDiskParms.CallForm = 0;
#endif
    iDrive = lpwddi->iDrive;
    switch (lpwddi->iType)
    {
	case DRIVE_REMOVABLE:
	    if(lpwddi->status == 0)
#ifdef OPK2
		dpbDiskParms.DevPrm.ExtDevPB.SplFunctions = 1;	// Get current disk parms
#else
		dpbDiskParms.SplFunctions = 1;	// Get current disk parms
#endif
	    else
#ifdef OPK2
		dpbDiskParms.DevPrm.ExtDevPB.SplFunctions = 0;	// Get GENERIC parms
#else
		dpbDiskParms.SplFunctions = 0;	// Get GENERIC parms
#endif
	    if ((lpwddi->status == 0) &&
		((lpwddi->totalBytesHi != 0L)		    ||
		 (lpwddi->totalBytesLo >= WD_MIN_BIG_FLOPPY)  ))

	    {
	       retval |= DT_FBIG;
	    }
	    else if(!DeviceParameters(iDrive, &dpbDiskParms, IOCTL_GET_DPB))
	    {
#ifdef OPK2
		switch (dpbDiskParms.DevPrm.ExtDevPB.devType) {
#else
		switch (dpbDiskParms.devType) {
#endif
		    case DEVPB_DEVTYP_350_0720:
		    case DEVPB_DEVTYP_350_1440:
		    case DEVPB_DEVTYP_350_2880:
			retval |= DT_F350;
			break;

		    case DEVPB_DEVTYP_525_1200:
		    case DEVPB_DEVTYP_525_0360:
			retval |= DT_F525;
			break;

		    default:
			break;
		}
	    }
	    else
	    {
	       ;	// Not setting any of DT_F350, DT_F525, DT_FBIG
			// results in "generic" removable
	    }
	    goto IsDDLFN;
	    break;

	case DRIVE_RAMDRIVE:
	case DRIVE_FIXED:
	    goto IsDDLFN;
	    break;

	case DRIVE_REMOTE:
	case DRIVE_CDROM:

	    // NOTE FALL THROUGH

	default:
IsDDLFN:
	    DNm[0] += iDrive;
	    _asm
	    {
		push	ds
		push	si
		push	di
		push	ss
		pop	ds
		push	ss
		pop	es
		lea	dx,DNm
		lea	di,FNm
		mov	cx,128		// sizeof(FNm)
		xor	bx,bx		// zero output value if weirdness occurs
		mov	ax,0x71A0	// GetVolumeInformation
		stc
		int	0x21
		jnc	SetVal
		xor	bx,bx
		mov	cx,8 + 1 + 3 + 1
		mov	dx,67
	      SetVal:
		mov	[fnsz],cx
		mov	[fpsz],dx
		test	bx,FS_VOL_IS_COMPRESSED
		jz	NotComp
		mov	[DDFlg],1
	      NotComp:
		test	bx,FS_VOL_SUPPORTS_LONG_NAMES
		jz	NotLFN
		mov	[LFNFlg],1
	      NotLFN:
		pop	di
		pop	si
		pop	ds
	    }
	    lpwddi->MaxComponLen = fnsz;
	    lpwddi->MaxPathLen = fpsz;
	    if (DDFlg)
	    {
	       retval |= DT_DBLDISK;
	    }
	    if (LFNFlg)
	    {
	       retval |= DT_LONGNAME;
	    }
	    break;
    }
    return(retval);
}


BOOL NEAR PASCAL GetVolumeLabel(UINT drive, LPSTR buff, UINT szbuf)
{
    WORD    DTASvSeg;
    WORD    DTASvOff;
    UINT    prvEMd;
    BOOL    myret = 0;
    BYTE    SrchSpec[] = "x:*.*";
    BYTE    SrchBuf[60];

    prvEMd = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    buff[0] = '\0';
    if(drive >= 26)
	return(myret);
    if(szbuf < 12)
	return(myret);

    SrchSpec[0] = (BYTE)drive + (BYTE)'A';
    _asm
    {
	push	ds
	push	es
	mov	ax,2F00h
	int	21h
	mov	DTASvSeg,es
	mov	DTASvOff,bx
	push	ss
	pop	ds
	lea	dx,SrchBuf
	mov	ax,1A00h
	int	21h
	lea	dx,SrchSpec
	mov	cx,0008h
	mov	ax,4E00h
	int	21h
	jc	NoLab
	mov	myret,1
	lea	si,SrchBuf
	lea	si,[si+30]
	les	di,buff
	cld

      GetVol:
	lodsb
	or	al,al
	jz	VolDn
	cmp	al,'.'
	je	GetVol
	stosb
	jmp	short GetVol

      VolDn:
	stosb
      NoLab:
	mov	ds,DTASvSeg
	mov	dx,DTASvOff
	mov	ax,1A00h
	int	21h
	pop	es
	pop	ds
    }
    SetErrorMode(prvEMd);
    return(myret);
}

#pragma optimize("",on)

typedef struct _HFILEINFO
{
        HICON       hIcon;                      // out icon
        int i;
        DWORD       dwAttributes;               // in/out SFGAO_ flags
        char        szDisplayName[260];         // out display name (or path)
        char        szTypeName[80];             // out
} HFILEINFO;
DWORD WINAPI SHGetFileInfo(LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFO FAR * hfi, UINT cbFileInfo, UINT uFlags);
DWORD WINAPI SHGetFileInfoA(LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFO FAR * lpshfi, UINT cbFileInfo, UINT uFlags);


VOID FAR PASCAL InitDrvInfo(HWND hwnd, LPWINDISKDRIVEINFO lpwddi, LPDMAINTINFO lpDMaint)
{
    SHFILEINFO shfi;
    DWORD      DMFlags[26];
    UINT       prvEMd;
    char       Drv[] = "A:\\";

    prvEMd = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    lpwddi->rootPath[0]      = 'A' + lpwddi->iDrive;
    lpwddi->rootPath[1]      = ':';
    lpwddi->rootPath[2]      = '\\';
    lpwddi->rootPath[3]      = 0;
    lpwddi->DrawFlags	     = 0;
    lpwddi->status	     = 0xFFFE;	// Status "unknown"
    lpwddi->freeBytesHi      = 0L;
    lpwddi->freeBytesLo      = 0L;
    lpwddi->totalBytesLo     = 0L;
    lpwddi->totalBytesHi     = 0L;
    lpwddi->hDriveWindow = hwnd;
    lpwddi->iType	 = DriveType(lpwddi->iDrive);
    lpwddi->TypeFlags	 = GetDriveSubType(lpwddi);
    lpDMaint->lpfnGetEngineDriveInfo((LPDWORD)&DMFlags);
    lpwddi->DskMFlags	 = DMFlags[lpwddi->iDrive];
    lpwddi->hBigBitmap	 = 0;

    Drv[0] = (char)lpwddi->iDrive + 'A';
    {
        SHFILEINFO hfi;
        SHGetFileInfo(Drv, 0L, &hfi, sizeof(shfi),
                     SHGFI_DISPLAYNAME);
    }

    if(!SHGetFileInfo(Drv, 0L, (SHFILEINFO FAR *)&shfi, sizeof(shfi),
		      SHGFI_DISPLAYNAME))

    {
	lpwddi->driveNameStr[0]  = 0;
    } else {
	lstrcpy(lpwddi->driveNameStr,(LPSTR)&(shfi.szDisplayName[0]));
    }
    SetErrorMode(prvEMd);
    return;
}



WORD NEAR MyFormatSysMessageBox(LPMYFMTINFOSTRUCT lpMyFmtInf,
				LPCSTR lpMsgBuf, WORD BoxStyle, BOOL isSys)
{
    WORD	      j;
    PSTR	      pMsgBuf;
    
#define SZMSGBUF2      256
#define SZTYPBUF2      80
#define SZTITBUF2      80
#define SZFMTBUF2      128
#define TOTMSZ2        (SZMSGBUF2+SZTYPBUF2+SZTITBUF2+SZFMTBUF2)

#define MsgBuf2  (&(pMsgBuf[0]))
#define TypeBuf2 (&(pMsgBuf[SZMSGBUF2]))
#define TitBuf2  (&(pMsgBuf[SZMSGBUF2+SZTYPBUF2]))
#define FmtBuf2  (&(pMsgBuf[SZMSGBUF2+SZTYPBUF2+SZTITBUF2]))

    pMsgBuf = (PSTR)LocalAlloc(LPTR,TOTMSZ2);
    if(!pMsgBuf)
    {
	return(IDCANCEL);
    }

    LoadString(g_hInstance, IDS_DRIVETITLEF, FmtBuf2, SZFMTBUF2);

    wsprintf((LPSTR)TitBuf2,(LPSTR)FmtBuf2,
	     (LPSTR)lpMyFmtInf->lpwddi->driveNameStr);

    if (!HIWORD(lpMsgBuf)) {
        WORD MsgID = LOWORD(lpMsgBuf);
        LoadString(g_hInstance, MsgID, MsgBuf2, SZMSGBUF2);
        lpMsgBuf = MsgBuf2;
    }

    j = MessageBox(lpMyFmtInf->hProgDlgWnd,lpMsgBuf,TitBuf2,BoxStyle);

    LocalFree((HANDLE)pMsgBuf);

    return(j);
}


LRESULT CALLBACK FmtCBProc(UINT msg, LPARAM lRefData, LPARAM lParam1,
			   LPARAM lParam2, LPARAM lParam3,
			   LPARAM lParam4, LPARAM lParam5)
{
    LPMYFMTINFOSTRUCT lpMyFmtInf;
    LPFMTINFOSTRUCT   lpFmtInf;
    BOOL	      PMRet;
    MSG 	      wmsg;
    WORD	      i,j;
    HWND	      hCmb;
    PSTR	      pMsgBuf;

#define SZMSGBUF4      256
#define SZFMTBUF4      256
#define TOTMSZ4        (SZMSGBUF4+SZFMTBUF4)

#define MsgBuf4  (&(pMsgBuf[0]))
#define FmtBuf4  (&(pMsgBuf[SZMSGBUF4]))

    lpMyFmtInf	= (LPMYFMTINFOSTRUCT)lRefData;
    lpFmtInf	= (LPFMTINFOSTRUCT)lParam1;

    switch(msg)
    {
	case DU_INITENGINE:
	    SendMessage(lpMyFmtInf->hProgDlgWnd, WM_APP+4, TRUE, 0L);
	    return(0L);
	    break;

	case DU_ERRORCORRECTED:
//	      switch (LOWORD(lParam2))
//	      {
//		  case ERRLOCKV:
//		      goto DoYld;
//		      break;
//
//		  default:
//		      goto DoYld;
//		      break;
//	      }
	    goto DoYld;
	    break;

	case DU_ERRORDETECTED:
	    if(HIWORD(lParam2) & WRTPROT)
	    {
		i = IDS_WRTPROT;
		goto DoRetryCanMsg;
	    } else if(HIWORD(lParam2) & NOTRDY) {
		i = IDS_NOTRDY;
DoRetryCanMsg:
		j = MB_ICONINFORMATION | MB_RETRYCANCEL;
		i = MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(i),j,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
		if(i == IDCANCEL)
		    return(MAKELONG(0,ERETCAN2));
		else
		    return(MAKELONG(0,ERETRETRY));

	    } else {
		switch (LOWORD(lParam2))
		{
		    case ERRLOCKV:

			// BUG BUG
			// lparam3 == 0xFFFFFFFFL if special format lock,
			// else is file name pointer
			//	(0 if none (insuff mem?)).

			return(MAKELONG(0,ERETCAN2));
			break;

		    case ERRINVFMT:
			if(!(HIWORD(lParam2) & RECOV))
			{
			    MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(IDS_INVFMT),MB_ICONINFORMATION | MB_OK,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			    return(MAKELONG(0,ERETCAN2));
			}
			pMsgBuf = (PSTR)LocalAlloc(LPTR,TOTMSZ4);
			if(!pMsgBuf)
			{
			    MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(IDS_NOMEMF),MB_ICONINFORMATION | MB_OK,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			    return(MAKELONG(0,ERETCAN2));
			}

			for(j = 0; j < lpMyFmtInf->FmtInf.PhysFmtCnt; j++)
			{
			    if(lpMyFmtInf->FmtInf.PhysFmtIDList[j] == lpMyFmtInf->FmtInf.PhysFmtID)
				break;
			}
			LoadString(g_hInstance, IDS_INVFMTREC, FmtBuf4, SZFMTBUF4);
			wsprintf(MsgBuf4,FmtBuf4,(LPSTR)&(lpMyFmtInf->FmtInf.PhysFmtNmList[j]));

			i = MyFormatSysMessageBox(lpMyFmtInf,MsgBuf4,MB_ICONQUESTION | MB_OKCANCEL,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			if(i == IDCANCEL)
			{
			    LocalFree((HANDLE)pMsgBuf);
			    return(MAKELONG(0,ERETCAN2));
			} else {
			    lpMyFmtInf->LastFmtRslt = lpMyFmtInf->FmtInf.PhysFmtID;
			    hCmb = GetDlgItem(lpMyFmtInf->hProgDlgWnd, DLGCONFOR_CAPCOMB);

			    for(i = 0; ; i++)
			    {
				if(SendMessage(hCmb, CB_GETLBTEXT,i,
					       (LPARAM)(LPSTR)FmtBuf4) == CB_ERR)
				    break;

				if(lstrcmpi(FmtBuf4,
					    (LPSTR)&(lpMyFmtInf->FmtInf.PhysFmtNmList[j]))
				   == 0)
				{
				    SendMessage(hCmb, CB_SETCURSEL, i, (LONG)NULL);
				    SendMessage(hCmb,EM_SETSEL,0,MAKELONG(0,0x7FFF));
				    break;
				}
			    }
			    LocalFree((HANDLE)pMsgBuf);
			    return(MAKELONG(0,ERETRETRY));
			}
			break;

		    case ERRNOQUICK:
			if(HIWORD(lParam2) & RECOV)
			{
			    if(lpMyFmtInf->lpwddi->iType == DRIVE_REMOVABLE)
			    {
				i = IDS_NOQCKRECREM;
			    } else {
				i = IDS_NOQCKREC;
			    }
			    j = MB_ICONQUESTION | MB_OKCANCEL;
			} else {
			    if(lpMyFmtInf->lpwddi->iType == DRIVE_REMOVABLE)
			    {
				i = IDS_NOQCKREM;
			    } else {
				i = IDS_NOQCK;
			    }
			    j = MB_ICONINFORMATION | MB_OK;
			}
			i = MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(i),j,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			if(!(HIWORD(lParam2) & RECOV) || (i == IDCANCEL))
			{
			    return(MAKELONG(0,ERETCAN2));
			} else {
			    CheckRadioButton(lpMyFmtInf->hProgDlgWnd, DLGCONFOR_FULL, DLGCONFOR_DOSYS, DLGCONFOR_FULL);
			    lpMyFmtInf->FmtOpt &= ~DLGCONFOR_QCK;
			    return(MAKELONG(0,ERETRETRY));
			}
			break;

		    case ERRVOLLABEL:
			//
			// BUG BUG Check for special BADCHRS case....
			//

			if(HIWORD(lParam2) & RECOV)
			    i = IDS_BADVOLREC;
			else
			    i = IDS_BADVOL;
			goto DoOKCanMsg;
			break;

		    case ERRFAT:
			if(lpMyFmtInf->FmtInf.Options & FD_BOOTONLY)
			    i = IDS_FATERRSYS;
			else
			    i = IDS_FATERR;
			goto DoOKMsg;
			break;

		    case ERRFBOOT:
			if(lpMyFmtInf->FmtInf.Options & FD_BOOTONLY)
			    i = IDS_BOOTERRSYS;
			else
			    i = IDS_BOOTERR;
			goto DoOKMsg;
			break;

		    case ERRROOTD:
			i = IDS_ROOTDERR;
			goto DoOKMsg;
			break;

		    case ERROSAREA:
			if(HIWORD(lParam2) & RECOV)
			    i = IDS_OSAREC;
			else
			    i = IDS_OSA;
			goto DoOKCanMsg;
			break;

		    case ERRDATA:
			if(HIWORD(lParam2) & RECOV)
			    i = IDS_DATAERRREC;
			else
			    i = IDS_DATAERR;
			goto DoOKCanMsg;
			break;

		    case ERRTOS:
			if(HIWORD(lParam2) & RECOV)
			    i = IDS_TOSREC;
			else
			    i = IDS_TOS;
			if(lpMyFmtInf->FmtInf.Options & FD_BOOTONLY)
			{
			    if(HIWORD(lParam2) & DISKFULL)
				i = IDS_FULLDISK;
			    else if(HIWORD(lParam2) & OSFILESPACE)
				i = IDS_NOSYSFILES;
			}
			goto DoOKCanMsg;
			break;

		    case ERRNOOS:
			if(HIWORD(lParam2) & RECOV)
			    i = IDS_NOSREC;
			else
			    i = IDS_NOS;
DoOKCanMsg:
			if(HIWORD(lParam2) & RECOV)
			    j = MB_ICONQUESTION | MB_OKCANCEL;
			else
			    j = MB_ICONINFORMATION | MB_OK;
			i = MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(i),j,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			if(!(HIWORD(lParam2) & RECOV) || (i == IDCANCEL))
			    return(MAKELONG(0,ERETCAN2));
			else
			    return(MAKELONG(0,ERETIGN));
			break;

		    case ERRNOUFOR:
		    case ERRBADUFOR:
		    case ERRDSKWRT:
		    default:
			if(lpMyFmtInf->FmtInf.Options & FD_BOOTONLY)
			    i = IDS_UNKERRSYS;
			else
			    i = IDS_UNKERR;
DoOKMsg:
			MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(i),MB_ICONINFORMATION | MB_OK,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			return(MAKELONG(0,ERETCAN2));
			break;

		}
	    }
	    break;

	case DU_OPCOMPLETE:
	    lpMyFmtInf->lpFmtRep = (LPFATFMTREPORT)lParam2;
	    lpMyFmtInf->OpCmpltRet = lParam3;
	    SendMessage(lpMyFmtInf->hProgDlgWnd, WM_APP+3, 0, 0L);
	    return(0L);
	    break;

	case DU_OPUPDATE:
	case DU_ENGINESTART:
	case DU_YIELD:
	    if(lpMyFmtInf->FmtIsActive)
		lpMyFmtInf->FmtInf.Options &= ~FDO_LOWPRIORITY;
	    else
		lpMyFmtInf->FmtInf.Options |= FDO_LOWPRIORITY;

	    if(lpMyFmtInf->hTimer == 0)
	    {
		lpMyFmtInf->hTimer = SetTimer(lpMyFmtInf->hProgDlgWnd,1,750,NULL);
	    }
	    SendMessage(lpMyFmtInf->hProgDlgWnd, WM_APP+4, TRUE, 0L);
DoYld:

#define YLDCNTDIVH	 7L
#define YLDCNTDIVL	 2L

	    lpMyFmtInf->YldCnt++;
	    if(lpMyFmtInf->hTimer != 0)
	    {
		if(lpMyFmtInf->FmtIsActive)
		{
		    if((lpMyFmtInf->YldCnt % YLDCNTDIVH) == 0)
		    {
			goto DoWait;
		    }
		} else {
//		      if((lpMyFmtInf->YldCnt % YLDCNTDIVL) == 0)
//		      {
DoWait:
			WaitMessage();
//		      }
		}
	    }
DoYld2:
	    PMRet = PeekMessage((LPMSG)&wmsg, NULL, 0, 0, PM_REMOVE);
	    if(PMRet)
	    {
		if(lpMyFmtInf->FmtIsActive &&
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
			lpMyFmtInf->FmtCancelBool = TRUE;
			SendDlgItemMessage(lpMyFmtInf->hProgDlgWnd,
					   DLGCONFOR_CANCEL,BM_SETSTATE,
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
		    goto DoWait;
		}
		if(lpMyFmtInf->FmtIsActive)
		{
		    goto DoYld;
		} else {
		    goto DoYld2;
		}
	    }
	    if(lpMyFmtInf->FmtCancelBool)
		return(1L);
	    else
		return(0L);
	    break;

	default:
	    return(0L);
	    break;

    }
}

#ifdef OPK2
void FAR PASCAL BeutifyNumberOutput(HWND hwnd, HDC hDC, WORD dxNum1, int id, int AltID, DWORD dwInput, DWORD dwInput2);
    
BOOL WINAPI FmtRepDlgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPSHFORMATDRIVEINFO lpSHFmtInfo;
    LPMYFMTINFOSTRUCT lpMyFmtInf;
    LPFATFMTREPORT    lpFmtRep;
    DWORD	      dwi;
    HFONT	      hTxtFnt;
    HFONT	      hFnt;
    HWND	      hCtrl;
    HDC 	      hDC;
    WORD	      dxNum;
    BYTE	      Buf1[128];
    BYTE	      Buf2[128];
    BYTE	      Buf3[128];
    WORD	      j;

    switch (msg) {

	case  WM_INITDIALOG:
	    lpSHFmtInfo = (LPSHFORMATDRIVEINFO)lParam;
	    lpMyFmtInf = lpSHFmtInfo->lpMyFmtInf;
	    lpFmtRep = lpMyFmtInf->lpFmtRep;

	    GetWindowText(hwnd,Buf1,sizeof(Buf1));

	    wsprintf((LPSTR)Buf3,(LPSTR)Buf1,(LPSTR)lpMyFmtInf->lpwddi->driveNameStr);

	    SetWindowText(hwnd,Buf3);

	    hCtrl = GetDlgItem(hwnd,DLGFORREP_TOT);
	    hTxtFnt = (HFONT)SendMessage(hCtrl,WM_GETFONT,0,0L);
	    if(!hTxtFnt)
	       hTxtFnt = GetStockObject(SYSTEM_FONT);
	    hDC = GetDC(hCtrl);
	    hFnt = SelectObject(hDC,hTxtFnt);
	    dxNum = LOWORD(GetTextExtent(hDC,(LPSTR)"1,000,000,000",13));

	    if((lpFmtRep->TotDiskSzByte == 0L) &&
	       ((lpFmtRep->TotDiskSzK != 0L) || (lpFmtRep->TotDiskSzM != 0L)))
	    {
		if(lpFmtRep->TotDiskSzK == 0L)
		{
		    j = IDS_REP_TOTM;
		    dwi = lpFmtRep->TotDiskSzM;
		} else {
		    j = IDS_REP_TOTK;
		    dwi = lpFmtRep->TotDiskSzK;
		}
	    } else {
		j = 0;
		dwi = lpFmtRep->TotDiskSzByte;
	    }
	    BeutifyNumberOutput(hwnd, hDC, dxNum, DLGFORREP_TOT, j, dwi, 0);

	    BeutifyNumberOutput(hwnd, hDC, dxNum, DLGFORREP_SYS, 0, lpFmtRep->SysSzByte, 0);

	    if((lpFmtRep->BadSzByte == 0L) &&
	       ((lpFmtRep->BadSzK != 0L) || (lpFmtRep->BadSzM != 0L)))
	    {
		if(lpFmtRep->BadSzK == 0L)
		{
		    j = IDS_REP_BADM;
		    dwi = lpFmtRep->BadSzM;
		} else {
		    j = IDS_REP_BADK;
		    dwi = lpFmtRep->BadSzK;
		}
	    } else {
		j = 0;
		dwi = lpFmtRep->BadSzByte;
	    }
	    BeutifyNumberOutput(hwnd, hDC, dxNum, DLGFORREP_BAD, j, dwi, 0);

	    if((lpFmtRep->AvailSzByte == 0L) &&
	       ((lpFmtRep->AvailSzK != 0L) || (lpFmtRep->AvailSzM != 0L)))
	    {
		if(lpFmtRep->AvailSzK == 0L)
		{
		    j = IDS_REP_AVAILM;
		    dwi = lpFmtRep->AvailSzM;
		} else {
		    j = IDS_REP_AVAILK;
		    dwi = lpFmtRep->AvailSzK;
		}
	    } else {
		j = 0;
		dwi = lpFmtRep->AvailSzByte;
	    }
	    BeutifyNumberOutput(hwnd, hDC, dxNum, DLGFORREP_AVAIL, j, dwi, 0);

	    BeutifyNumberOutput(hwnd, hDC, dxNum, DLGFORREP_BCLUS, 0, lpFmtRep->BytesPerClus, 0);
	    BeutifyNumberOutput(hwnd, hDC, dxNum, DLGFORREP_TCLUS, 0, lpFmtRep->TotDataClus, 0);

            // ARGH!  This is annoying
	    GetDlgItemText(hwnd,DLGFORREP_SER,Buf1,sizeof(Buf1));
	    wsprintf((LPSTR)Buf2,(LPSTR)"%04X-%04X",HIWORD(lpFmtRep->SerialNumber),LOWORD(lpFmtRep->SerialNumber));
	    j = lstrlen(Buf2);
	    while(LOWORD(GetTextExtent(hDC,(LPSTR)Buf2,j)) < (dxNum - 2))
	    {
                hmemcpy(Buf2+1, Buf2, j+1);
		Buf2[0] = ' ';
                j++;
	    }
	    wsprintf((LPSTR)Buf3,(LPSTR)Buf1,(LPSTR)Buf2);
	    SetDlgItemText(hwnd,DLGFORREP_SER,Buf3);

	    SelectObject(hDC,hFnt);
	    ReleaseDC(hCtrl,hDC);

	    return(TRUE);
	    break;

	case WM_COMMAND:
	    switch  (wParam) {

		case DLGFORREP_CLOSE:
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
    return(FALSE);
}
#else
void FAR PASCAL SomeRandomMagic(HWND hwnd, HDC hDC, WORD dxNum1, int id, DWORD dwInput, DWORD dwInput2);
    
BOOL WINAPI FmtRepDlgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPSHFORMATDRIVEINFO lpSHFmtInfo;
    LPMYFMTINFOSTRUCT lpMyFmtInf;
    LPFATFMTREPORT    lpFmtRep;
    HFONT	      hTxtFnt;
    HFONT	      hFnt;
    HWND	      hCtrl;
    HDC 	      hDC;
    WORD	      dxNum;
    BYTE	      Buf1[128];
    BYTE	      Buf2[128];
    BYTE	      Buf3[128];
    WORD	      j;

    switch (msg) {

	case  WM_INITDIALOG:
	    lpSHFmtInfo = (LPSHFORMATDRIVEINFO)lParam;
	    lpMyFmtInf = lpSHFmtInfo->lpMyFmtInf;
	    lpFmtRep = lpMyFmtInf->lpFmtRep;

	    GetWindowText(hwnd,Buf1,sizeof(Buf1));

	    wsprintf((LPSTR)Buf3,(LPSTR)Buf1,(LPSTR)lpMyFmtInf->lpwddi->driveNameStr);

	    SetWindowText(hwnd,Buf3);

	    hCtrl = GetDlgItem(hwnd,DLGFORREP_TOT);
	    hTxtFnt = (HFONT)SendMessage(hCtrl,WM_GETFONT,0,0L);
	    if(!hTxtFnt)
	       hTxtFnt = GetStockObject(SYSTEM_FONT);
	    hDC = GetDC(hCtrl);
	    hFnt = SelectObject(hDC,hTxtFnt);
	    dxNum = LOWORD(GetTextExtent(hDC,(LPSTR)"1,000,000,000",13));

            SomeRandomMagic(hwnd, hDC, dxNum, DLGFORREP_TOT, lpFmtRep->TotDiskSzByte, 0);
            SomeRandomMagic(hwnd, hDC, dxNum, DLGFORREP_SYS, lpFmtRep->SysSzByte, 0);
            SomeRandomMagic(hwnd, hDC, dxNum, DLGFORREP_BAD, lpFmtRep->BadSzByte, 0);
            SomeRandomMagic(hwnd, hDC, dxNum, DLGFORREP_AVAIL, lpFmtRep->AvailSzByte, 0);
            SomeRandomMagic(hwnd, hDC, dxNum, DLGFORREP_BCLUS, lpFmtRep->BytesPerClus, 0);
            SomeRandomMagic(hwnd, hDC, dxNum, DLGFORREP_TCLUS, lpFmtRep->TotDataClus, 0);

            // ARGH!  This is annoying
	    GetDlgItemText(hwnd,DLGFORREP_SER,Buf1,sizeof(Buf1));
	    wsprintf((LPSTR)Buf2,(LPSTR)"%04X-%04X",HIWORD(lpFmtRep->SerialNumber),LOWORD(lpFmtRep->SerialNumber));
	    j = lstrlen(Buf2);
	    while(LOWORD(GetTextExtent(hDC,(LPSTR)Buf2,j)) < (dxNum - 2))
	    {
                hmemcpy(Buf2+1, Buf2, j+1);
		Buf2[0] = ' ';
                j++;
	    }
	    wsprintf((LPSTR)Buf3,(LPSTR)Buf1,(LPSTR)Buf2);
	    SetDlgItemText(hwnd,DLGFORREP_SER,Buf3);

	    SelectObject(hDC,hFnt);
	    ReleaseDC(hCtrl,hDC);

	    return(TRUE);
	    break;

	case WM_COMMAND:
	    switch  (wParam) {

		case DLGFORREP_CLOSE:
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
    return(FALSE);
}
#endif

BOOL WINAPI FmtDlgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WORD	      i,j;
    DWORD	      dwi;
    DWORD	      dwj;
    HWND	      hCmb;
    LPSHFORMATDRIVEINFO lpSHFmtInfo;
    LPMYFMTINFOSTRUCT lpMyFmtInf;
    PSTR	      pMsgBuf;
    WORD	      CurrSelFmtID;

#define SZTYPBUF5      80
#define SZTITBUF5      80
#define SZFMTBUF5      80
#define TOTMSZ5        (SZTYPBUF5+SZTITBUF5+SZFMTBUF5)

#define TypeBuf5 (&(pMsgBuf[0]))
#define TitBuf5  (&(pMsgBuf[SZTYPBUF5]))
#define FmtBuf5  (&(pMsgBuf[SZTYPBUF5+SZTITBUF5]))

    lpSHFmtInfo = (LPSHFORMATDRIVEINFO)GetWindowLong(hwnd,DWL_USER);
    if (lpSHFmtInfo)
    {
	lpMyFmtInf = lpSHFmtInfo->lpMyFmtInf;
    }

    switch (msg) {

	case  WM_INITDIALOG:
	    SetWindowLong(hwnd,DWL_USER,lParam);
	    lpSHFmtInfo = (LPSHFORMATDRIVEINFO)lParam;
	    lpMyFmtInf = lpSHFmtInfo->lpMyFmtInf;
	    lpMyFmtInf->LastFmtRslt = LASTFMTRSLT_CAN;

	    pMsgBuf = (PSTR)LocalAlloc(LPTR,TOTMSZ5);
	    if(!pMsgBuf)
	    {
		MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(IDS_NOMEMF),MB_ICONINFORMATION | MB_OK,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
		EndDialog(hwnd, IDCANCEL);
	    }

	    // Init progress bar and status text

	    SendDlgItemMessage(hwnd, DLGCONFOR_PBAR, PBM_SETRANGE, 0, MAKELONG(0, 100));
	    SendDlgItemMessage(hwnd, DLGCONFOR_PBAR, PBM_SETPOS, 0, 0L);

	    SetDlgItemText(hwnd, DLGCONFOR_STATTXT, g_szNULL);

	    // turn this style bit, PBS_SHOWPERCENT, on and off to
	    // enable/disable the progress bar.
	    //
	    //	dwi = GetWindowLong(GetDlgItem(hwnd,DLGCONFOR_PBAR),GWL_STYLE);
	    //	dwi |= PBS_SHOWPERCENT; or dwi &= ~PBS_SHOWPERCENT;
	    //	SetWindowLong(GetDlgItem(hwnd,DLGCONFOR_PBAR),GWL_STYLE,dwi);
	    //
	    // When off, also do SendDlgItemMessage(..PBM_SETPOS, 0, 0L)
	    // to set the bar to "empty".

	    // LONG volume labels currently not supported
	    // if((lpMyFmtInf->lpwddi->TypeFlags) & DT_LONGNAME)
	    // {
	    //	   SendDlgItemMessage(hwnd, DLGCONFOR_LABEL, EM_LIMITTEXT, (MAXFNAMELEN-1), 0L);
	    // } else {
		SendDlgItemMessage(hwnd, DLGCONFOR_LABEL, EM_LIMITTEXT, 11, 0L);
	    // }

	    switch (lpMyFmtInf->lpwddi->iType)
	    {
		case DRIVE_REMOTE:
		case DRIVE_CDROM:
		    SetDlgItemText(hwnd, DLGCONFOR_LABEL, g_szNULL);
		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_LABEL),FALSE);
		    CheckDlgButton(hwnd, DLGCONFOR_NOLAB, TRUE);
		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_NOLAB),FALSE);
		    break;

		case DRIVE_RAMDRIVE:
		case DRIVE_REMOVABLE:
		case DRIVE_FIXED:
		default:
		    GetVolumeLabel(lpMyFmtInf->FmtInf.Drive,lpMyFmtInf->FmtInf.VolLabel,sizeof(lpMyFmtInf->FmtInf.VolLabel));
		    OemToAnsi(lpMyFmtInf->FmtInf.VolLabel,FmtBuf5);
		    SetDlgItemText(hwnd, DLGCONFOR_LABEL, FmtBuf5);
		    SendDlgItemMessage(hwnd, DLGCONFOR_LABEL, EM_SETMODIFY, FALSE, 0L);
		    break;
	    }

	    GetWindowText(hwnd,FmtBuf5,SZFMTBUF5);
	    wsprintf((LPSTR)TitBuf5,(LPSTR)FmtBuf5,(LPSTR)lpMyFmtInf->lpwddi->driveNameStr);
	    SetWindowText(hwnd,TitBuf5);

	    // Init DLGCONFOR_CAPCOMB

	    hCmb = GetDlgItem(hwnd, DLGCONFOR_CAPCOMB);
	    SendMessage(hCmb, CB_RESETCONTENT, 0, 0L);
	    j = 0xFFFF;
	    for(i = 0; i < lpMyFmtInf->FmtInf.PhysFmtCnt; i++)
	    {
		SendMessage(hCmb, CB_INSERTSTRING,0xFFFF,(LPARAM)(LPSTR)&lpMyFmtInf->FmtInf.PhysFmtNmList[i]);
		SendMessage(hCmb, CB_SETITEMDATA,i,(LPARAM)(DWORD)i);
		if(lpMyFmtInf->ReqFmtID != SHFMT_ID_DEFAULT)
		{
		    if(lpMyFmtInf->FmtInf.PhysFmtIDList[i] == lpMyFmtInf->ReqFmtID)
			j = i;
		} else {
		    if(lpMyFmtInf->FmtInf.PhysFmtIDList[i] == lpMyFmtInf->FmtInf.DefPhysFmtID)
			j = i;
		}
	    }
	    if(j == 0xFFFF)
	    {
		if(lpMyFmtInf->ReqFmtID == SHFMT_ID_DEFAULT)
		{
		    j = 0;
		} else {
		    for(i = 0; i < lpMyFmtInf->FmtInf.PhysFmtCnt; i++)
		    {
			if(lpMyFmtInf->FmtInf.PhysFmtIDList[i] == lpMyFmtInf->FmtInf.DefPhysFmtID)
			    j = i;
		    }
		    if(j == 0xFFFF)
			j = 0;
		}
	    }
	    SendMessage(hCmb, CB_SETCURSEL, j, (LONG)NULL);
	    SetFocus(hCmb);
	    SendMessage(hCmb,EM_SETSEL,0,MAKELONG(0,0x7FFF));

	    if(lpMyFmtInf->ReqFmtOpt & SHFMT_OPT_SYSONLY)
            {
                // Try to initialize properly to just do a sys of the
                // disk
	        lpMyFmtInf->FmtOpt |= DLGCONFOR_SYSONLY;
            }

	    if(lpMyFmtInf->FmtOpt & DLGCONFOR_SYSONLY)
	    {
		if(!(lpMyFmtInf->FmtInf.Options & FD_BOOT))
		{
		    lpMyFmtInf->FmtOpt &= ~DLGCONFOR_SYSONLY;
		}
	    }

	    if(lpMyFmtInf->FmtInf.Options & FD_FSONLY)
	    {
		if(!(lpMyFmtInf->FmtOpt & DLGCONFOR_SYSONLY))
		{
		    if(lpMyFmtInf->ReqFmtOpt & SHFMT_OPT_FULL)
			CheckRadioButton(hwnd, DLGCONFOR_FULL, DLGCONFOR_DOSYS, DLGCONFOR_FULL);
		    else
			CheckRadioButton(hwnd, DLGCONFOR_FULL, DLGCONFOR_DOSYS, DLGCONFOR_QUICK);
		}
		if((!(lpMyFmtInf->FmtInf.Options & FD_LOWLEV)) && (!(lpMyFmtInf->FmtInf.Options & FD_VERIFY)))
		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_FULL),FALSE);
	    } else {
		if(!(lpMyFmtInf->FmtOpt & DLGCONFOR_SYSONLY))
		{
		    CheckRadioButton(hwnd, DLGCONFOR_FULL, DLGCONFOR_DOSYS, DLGCONFOR_FULL);
		}
		EnableWindow(GetDlgItem(hwnd,DLGCONFOR_QUICK),FALSE);
	    }
	    if(lpMyFmtInf->FmtOpt & DLGCONFOR_SYSONLY)
	    {
		CheckRadioButton(hwnd, DLGCONFOR_FULL, DLGCONFOR_DOSYS, DLGCONFOR_DOSYS);
		CheckDlgButton(hwnd, DLGCONFOR_MKSYS, TRUE);
		EnableWindow(GetDlgItem(hwnd,DLGCONFOR_MKSYS),FALSE);
		EnableWindow(GetDlgItem(hwnd,DLGCONFOR_LABEL),FALSE);
		EnableWindow(GetDlgItem(hwnd,DLGCONFOR_NOLAB),FALSE);
		EnableWindow(GetDlgItem(hwnd,DLGCONFOR_CAPCOMB),FALSE);
		CheckDlgButton(hwnd, DLGCONFOR_REPORT, FALSE);
		EnableWindow(GetDlgItem(hwnd,DLGCONFOR_REPORT),FALSE);
	    } else {
		CheckDlgButton(hwnd, DLGCONFOR_MKSYS, FALSE);
		CheckDlgButton(hwnd, DLGCONFOR_REPORT, TRUE);
	    }
	    if(!(lpMyFmtInf->FmtInf.Options & FD_BOOT))
	    {
		EnableWindow(GetDlgItem(hwnd,DLGCONFOR_DOSYS),FALSE);
		EnableWindow(GetDlgItem(hwnd,DLGCONFOR_MKSYS),FALSE);
	    }
	    LocalFree((HANDLE)pMsgBuf);
	    return(FALSE);	// We set the focus to the proper control...
	    break;

	case WM_ACTIVATE:
	    if(wParam == WA_INACTIVE)
		lpMyFmtInf->FmtIsActive = FALSE;
	    else
		lpMyFmtInf->FmtIsActive = TRUE;
	    return(FALSE);
	    break;

	case WM_COMMAND:
	    switch  (wParam) {

		case DLGCONFOR_DOSYS:
		case DLGCONFOR_FULL:
		case DLGCONFOR_QUICK:
		    if(lpMyFmtInf->FmtInProgBool)
		    {
			return(TRUE);
		    }

		    if (IsDlgButtonChecked(hwnd, wParam))
		    {
			// Do nothing if not changin states
			return(TRUE);
		    }

		    if(wParam == DLGCONFOR_DOSYS)
		    {
			CheckDlgButton(hwnd, DLGCONFOR_MKSYS, TRUE);
			CheckDlgButton(hwnd, DLGCONFOR_REPORT, FALSE);
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_MKSYS),FALSE);
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_LABEL),FALSE);
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_NOLAB),FALSE);
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_CAPCOMB),FALSE);
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_REPORT),FALSE);

			// Save the current label text for use if we go back
			// to a normal format

			pMsgBuf = (PSTR)LocalAlloc(LPTR,TOTMSZ5);
			if(!pMsgBuf)
			{
			    MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(IDS_NOMEMF),MB_ICONINFORMATION | MB_OK,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			    EndDialog(hwnd, IDCANCEL);
			} else {
			    GetDlgItemText(hwnd, DLGCONFOR_LABEL, FmtBuf5,
					   SZFMTBUF5);

			    AnsiToOem(FmtBuf5,lpMyFmtInf->FmtInf.VolLabel);
			    LocalFree((HANDLE)pMsgBuf);
			}
			SetDlgItemText(hwnd, DLGCONFOR_LABEL, g_szNULL);
			SendDlgItemMessage(hwnd, DLGCONFOR_LABEL, EM_SETMODIFY, FALSE, 0L);
		    } else {
			if (IsDlgButtonChecked(hwnd, DLGCONFOR_DOSYS))
			{
			    // If we are coming from a DOSYS, restore the
			    // label the user had typed and turn the
			    // report option back on

			    pMsgBuf = (PSTR)LocalAlloc(LPTR,TOTMSZ5);
			    if(!pMsgBuf)
			    {
				MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(IDS_NOMEMF),MB_ICONINFORMATION | MB_OK,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
				EndDialog(hwnd, IDCANCEL);
			    } else {
				OemToAnsi(lpMyFmtInf->FmtInf.VolLabel,FmtBuf5);
				SetDlgItemText(hwnd, DLGCONFOR_LABEL, FmtBuf5);
				LocalFree((HANDLE)pMsgBuf);
			    }
			    CheckDlgButton(hwnd, DLGCONFOR_REPORT, TRUE);
			}

			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_MKSYS),TRUE);
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_LABEL),TRUE);
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_NOLAB),TRUE);
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_CAPCOMB),TRUE);
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_REPORT),TRUE);
		    }

		    CheckRadioButton(hwnd, DLGCONFOR_FULL, DLGCONFOR_DOSYS, wParam);

		    return(TRUE);
		    break;

		case DLGCONFOR_NOLAB:
		    if(lpMyFmtInf->FmtInProgBool)
		    {
			return(TRUE);
		    }
		    if(IsDlgButtonChecked(hwnd, DLGCONFOR_DOSYS))
		    {
			return(TRUE);
		    }
		    CheckDlgButton(hwnd, wParam, !IsDlgButtonChecked(hwnd, wParam));
		    i = LOWORD(SendDlgItemMessage(hwnd, DLGCONFOR_LABEL, EM_GETMODIFY, 0, 0L));
		    if(IsDlgButtonChecked(hwnd, DLGCONFOR_NOLAB))
		    {
			SetDlgItemText(hwnd, DLGCONFOR_LABEL, g_szNULL);
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_LABEL),FALSE);
		    } else {
			pMsgBuf = (PSTR)LocalAlloc(LPTR,TOTMSZ5);
			if(!pMsgBuf)
			{
			    MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(IDS_NOMEMF),MB_ICONINFORMATION | MB_OK,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			    EndDialog(hwnd, IDCANCEL);
			} else {
			    OemToAnsi(lpMyFmtInf->FmtInf.VolLabel,FmtBuf5);
			    SetDlgItemText(hwnd, DLGCONFOR_LABEL, FmtBuf5);
			    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_LABEL),TRUE);
			    LocalFree((HANDLE)pMsgBuf);
			}
		    }
		    SendDlgItemMessage(hwnd, DLGCONFOR_LABEL, EM_SETMODIFY, i, 0L);
		    return(TRUE);
		    break;

		case DLGCONFOR_REPORT:
		case DLGCONFOR_MKSYS:
		    if(lpMyFmtInf->FmtInProgBool)
		    {
			return(TRUE);
		    }
		    if(IsDlgButtonChecked(hwnd, DLGCONFOR_DOSYS))
		    {
			return(TRUE);
		    }
		    CheckDlgButton(hwnd, wParam, !IsDlgButtonChecked(hwnd, wParam));
		    return(TRUE);
		    break;

		case DLGCONFOR_START:
		    if(lpMyFmtInf->FmtInProgBool)
		    {
			return(TRUE);
		    }
		    lpMyFmtInf->CurrOpRegion = 0xFF;
		    lpMyFmtInf->FmtOpt = 0;
		    if(IsDlgButtonChecked(hwnd, DLGCONFOR_DOSYS))
			lpMyFmtInf->FmtOpt |= DLGCONFOR_SYSONLY;
		    if(IsDlgButtonChecked(hwnd, DLGCONFOR_QUICK))
			lpMyFmtInf->FmtOpt |= DLGCONFOR_QCK;
		    if(IsDlgButtonChecked(hwnd, DLGCONFOR_MKSYS))
			lpMyFmtInf->FmtOpt |= DLGCONFOR_SYS;
		    if(IsDlgButtonChecked(hwnd, DLGCONFOR_REPORT))
			lpMyFmtInf->FmtOpt |= DLGCONFOR_REP;
		    if(IsDlgButtonChecked(hwnd, DLGCONFOR_NOLAB))
		    {
			lpMyFmtInf->FmtOpt |= DLGCONFOR_NOV;
		    }
		    hCmb = GetDlgItem(hwnd, DLGCONFOR_CAPCOMB);
		    i = (WORD)SendMessage(hCmb, CB_GETCURSEL, 0, 0L);
		    if(i == CB_ERR)
		    {
			MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(IDS_NOMEMF),MB_ICONINFORMATION | MB_OK,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			return(TRUE);
		    }
		    i = (WORD)SendMessage(hCmb, CB_GETITEMDATA, i, 0L);

		    if(lpMyFmtInf->FmtInf.Options & FD_VOLLABEL)
		    {
			if(lpMyFmtInf->FmtOpt & DLGCONFOR_NOV)
			{
			    dwi = FD_NOVOLLABEL;
			} else if(SendDlgItemMessage(hwnd, DLGCONFOR_LABEL,
						     EM_GETMODIFY, 0, 0L)) {

			    pMsgBuf = (PSTR)LocalAlloc(LPTR,TOTMSZ5);
			    if(!pMsgBuf)
			    {
				MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(IDS_NOMEMF),MB_ICONINFORMATION | MB_OK,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
				EndDialog(hwnd, IDCANCEL);
			    } else {
				GetDlgItemText(hwnd, DLGCONFOR_LABEL, FmtBuf5,
					       SZFMTBUF5);

				AnsiToOem(FmtBuf5,lpMyFmtInf->FmtInf.VolLabel);
				LocalFree((HANDLE)pMsgBuf);
			    }
			    dwi = FD_ISVOLLABEL;
			} else {
			    dwi = FD_ISVOLLABEL;
			}
		    } else {
			dwi = 0L;
		    }
		    if(lpMyFmtInf->FmtOpt & DLGCONFOR_SYSONLY)
		    {
			dwi = FD_BOOTONLY;
		    } else if(lpMyFmtInf->FmtOpt & DLGCONFOR_QCK) {
			dwi |= FD_FSONLY;
		    } else {
			if(lpMyFmtInf->FmtInf.Options & FD_LOWLEV)
			    dwi |= FD_LOWLEV;
			else
			    dwi |= FD_VERIFY;
		    }
		    if(!(lpMyFmtInf->FmtOpt & DLGCONFOR_SYSONLY))
		    {
			if(lpMyFmtInf->FmtOpt & DLGCONFOR_SYS)
			    dwi |= FD_BOOT;
		    }

		    lpMyFmtInf->LastFmtRslt = CurrSelFmtID =
		    lpMyFmtInf->FmtInf.PhysFmtID =
		    lpMyFmtInf->FmtInf.PhysFmtIDList[i];

		    if(!(lpMyFmtInf->FmtOpt & DLGCONFOR_SYSONLY))
		    {
			if(lpMyFmtInf->FmtInf.Options & FD_GETCONFIRM)
			{
			    i = MyFormatSysMessageBox(lpMyFmtInf,
						      MAKEINTRESOURCE(IDS_FCONFIRM),
						      MB_ICONSTOP |
							MB_OKCANCEL |
							MB_DEFBUTTON2,
						      lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			    if(i == IDCANCEL)
			    {
				return(TRUE);
			    }
			}
		    }
		    lpMyFmtInf->hProgDlgWnd = hwnd;

		    lpMyFmtInf->FmtCancelBool = FALSE;
		    lpMyFmtInf->FmtInProgBool = TRUE;

		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_CAPCOMB),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_FULL),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_QUICK),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_DOSYS),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_MKSYS),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_REPORT),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_NOLAB),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_LABEL),FALSE);
		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_START),FALSE);

		    pMsgBuf = (PSTR)LocalAlloc(LPTR,TOTMSZ5);
		    if(pMsgBuf)
		    {
			LoadString(g_hInstance, IDS_F_INIT, FmtBuf5, SZFMTBUF5);
			SetDlgItemText(hwnd, DLGCONFOR_STATTXT, FmtBuf5);

			LoadString(g_hInstance, IDS_CANCEL, FmtBuf5, SZFMTBUF5);
			SetDlgItemText(hwnd, DLGCONFOR_CANCEL, FmtBuf5);
			LocalFree((HANDLE)pMsgBuf);
		    } else {
			MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(IDS_NOMEMF),MB_ICONINFORMATION | MB_OK,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			goto ReEnable;
		    }

		    SendMessage(hwnd,DM_SETDEFID,DLGCONFOR_CANCEL,0L);
		    SetFocus(GetDlgItem(hwnd,DLGCONFOR_CANCEL));
#ifdef OPK2
		    if((lpMyFmtInf->FmtInf.Options & FD_GETCONFIRM) &&
		       (!(lpMyFmtInf->FmtOpt & DLGCONFOR_SYSONLY))    )
		    {
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_CANCEL),FALSE);
		    }
#endif
		    SendDlgItemMessage(hwnd, DLGCONFOR_PBAR, PBM_SETPOS, 0, 0L);
		    dwj = GetWindowLong(GetDlgItem(hwnd,DLGCONFOR_PBAR),GWL_STYLE);
		    dwj |= PBS_SHOWPERCENT;
		    SetWindowLong(GetDlgItem(hwnd,DLGCONFOR_PBAR),GWL_STYLE,dwj);
		    InvalidateRect(GetDlgItem(hwnd,DLGCONFOR_PBAR),NULL,TRUE);

		    dwi = lpSHFmtInfo->sDMaint.lpfnFormatDrive(&(lpMyFmtInf->FmtInf),
					     dwi,
					     FmtCBProc,
					     (LPARAM)lpMyFmtInf);


		    lpMyFmtInf->FmtInProgBool = FALSE;
		    if(lpMyFmtInf->hTimer != 0)
		    {
			KillTimer(lpMyFmtInf->hProgDlgWnd,lpMyFmtInf->hTimer);
			lpMyFmtInf->hTimer = 0;
		    }
		    switch (HIWORD(dwi))
		    {
			case ERR_BADOPTIONS:	// This error occurs before
						//  any call backs are made
			    i = IDS_FBADOPT;
			    goto DoErr;
			    break;

			case ERR_NOTSUPPORTED:	// This error occurs before
						//  any call backs are made
			    i = IDS_FNOTSUP;
			    goto DoErr;
			    break;

			case ERR_OSNOTFOUND:	// These errors may be
			case ERR_OSERR: 	//  correctable
			case ERR_FATAL:
			    // No error messages for these, this is handled
			    // in the call back.

			    lpMyFmtInf->LastFmtRslt = LASTFMTRSLT_ERR;
			    break;

			case ERR_FSACTIVE:	// These errors may go away
			case ERR_LOCKVIOLATION: //  if it is tried again.
			    MyFormatSysMessageBox(lpMyFmtInf,
						  MAKEINTRESOURCE(IDS_FLOCK),
						  MB_ICONINFORMATION | MB_OK,
						  lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			    lpMyFmtInf->LastFmtRslt = LASTFMTRSLT_ERR;
			    goto ReEnable;
			    break;

			case ERR_INSUFMEM:
			    i = IDS_NOMEMF;
DoErr:
			    MyFormatSysMessageBox(lpMyFmtInf,
						  MAKEINTRESOURCE(i),
						  MB_ICONINFORMATION | MB_OK,
						  lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			    lpMyFmtInf->LastFmtRslt = LASTFMTRSLT_ERR;
			    break;

			case OPCANCEL:
			    lpMyFmtInf->LastFmtRslt = LASTFMTRSLT_CAN;
			    break;

			case ERR_NONFATAL:
			case NOERROR:
			    // leave lpMyFmtInf->LastFmtRslt set as
			    // set above or by ERRINVFMT

			    if((lpMyFmtInf->FmtInf.Options & FD_GETCONFIRM) &&
			       (!(lpMyFmtInf->FmtOpt & (DLGCONFOR_QCK | DLGCONFOR_SYSONLY))) )
			    {
				MyFormatSysMessageBox(lpMyFmtInf,
						      MAKEINTRESOURCE(IDS_FDOSURF),
						      MB_ICONINFORMATION | MB_OK,
						      lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);

				WinHelp(hwnd, "windows.hlp>Proc4", HELP_CONTEXT, IDH_DISK_PHYSICAL);
			    }
			    break;

			default:
			    lpMyFmtInf->LastFmtRslt = LASTFMTRSLT_ERR;
			    break;		// just continue
		    }

		    SendDlgItemMessage(hwnd,DLGCONFOR_CANCEL,BM_SETSTATE,
				       FALSE,0L);

		    pMsgBuf = (PSTR)LocalAlloc(LPTR,TOTMSZ5);
		    if(pMsgBuf)
		    {
			LoadString(g_hInstance, IDS_CLOSE, FmtBuf5, SZFMTBUF5);
			SetDlgItemText(hwnd, DLGCONFOR_CANCEL, FmtBuf5);
			LocalFree((HANDLE)pMsgBuf);
		    } else {
			MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(IDS_NOMEMF),MB_ICONINFORMATION | MB_OK,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			goto ReEnable;
		    }

		    // Refresh the drive display to show the new state

		    if(lpMyFmtInf->lpwddi->hDriveWindow)
			PostMessage(lpMyFmtInf->lpwddi->hDriveWindow,WM_APP+2,0,0L);

		    //
		    // Re-Get the format options for this drive. The
		    //	call to DMaint_FormatDrive has scribbled all over
		    //	the FmtInf structure.
		    //
		    dwi = lpSHFmtInfo->sDMaint.lpfnGetFormatOptions(lpMyFmtInf->lpwddi->iDrive,&(lpMyFmtInf->FmtInf),sizeof(lpMyFmtInf->FmtInf));

		    GetVolumeLabel(lpMyFmtInf->lpwddi->iDrive,lpMyFmtInf->FmtInf.VolLabel,sizeof(lpMyFmtInf->FmtInf.VolLabel));

ReEnable:
		    SendMessage(hwnd,DM_SETDEFID,DLGCONFOR_START,0L);
		    SetFocus(GetDlgItem(hwnd,DLGCONFOR_START));

		    SetDlgItemText(hwnd, DLGCONFOR_STATTXT, g_szNULL);

		    SendDlgItemMessage(hwnd, DLGCONFOR_PBAR, PBM_SETPOS, 0, 0L);
		    dwj = GetWindowLong(GetDlgItem(hwnd,DLGCONFOR_PBAR),GWL_STYLE);
		    dwj &= ~PBS_SHOWPERCENT;
		    SetWindowLong(GetDlgItem(hwnd,DLGCONFOR_PBAR),GWL_STYLE,dwj);
		    InvalidateRect(GetDlgItem(hwnd,DLGCONFOR_PBAR),NULL,TRUE);

		    if(!(lpMyFmtInf->FmtOpt & DLGCONFOR_SYSONLY))
		    {
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_CAPCOMB),TRUE);
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_REPORT),TRUE);
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_NOLAB),TRUE);
		    }
		    if(lpMyFmtInf->FmtInf.Options & FD_FSONLY)
		    {
			if((lpMyFmtInf->FmtInf.Options & FD_LOWLEV) || (lpMyFmtInf->FmtInf.Options & FD_VERIFY))
			    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_FULL),TRUE);
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_QUICK),TRUE);
		    } else {
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_FULL),TRUE);
		    }
		    if(lpMyFmtInf->FmtInf.Options & FD_BOOT)
		    {
			EnableWindow(GetDlgItem(hwnd,DLGCONFOR_DOSYS),TRUE);
			if(!(lpMyFmtInf->FmtOpt & DLGCONFOR_SYSONLY))
			{
			    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_MKSYS),TRUE);
			}
		    }
		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_START),TRUE);
#ifdef OPK2
		    EnableWindow(GetDlgItem(hwnd,DLGCONFOR_CANCEL),TRUE);
#endif
		    if(!(lpMyFmtInf->FmtOpt & DLGCONFOR_SYSONLY))
		    {
			if(!IsDlgButtonChecked(hwnd, DLGCONFOR_NOLAB))
			{
			    pMsgBuf = (PSTR)LocalAlloc(LPTR,TOTMSZ5);
			    if(!pMsgBuf)
			    {
				MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(IDS_NOMEMF),MB_ICONINFORMATION | MB_OK,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
				EndDialog(hwnd, IDCANCEL);
			    } else {
				OemToAnsi(lpMyFmtInf->FmtInf.VolLabel,FmtBuf5);
				SetDlgItemText(hwnd, DLGCONFOR_LABEL, FmtBuf5);
				EnableWindow(GetDlgItem(hwnd,DLGCONFOR_LABEL),TRUE);
				LocalFree((HANDLE)pMsgBuf);
			    }
			} else {
			    SetDlgItemText(hwnd, DLGCONFOR_LABEL, g_szNULL);
			}
		    } else {
			pMsgBuf = (PSTR)LocalAlloc(LPTR,TOTMSZ5);
			if(!pMsgBuf)
			{
			    MyFormatSysMessageBox(lpMyFmtInf,MAKEINTRESOURCE(IDS_NOMEMF),MB_ICONINFORMATION | MB_OK,lpMyFmtInf->ReqFmtOpt & WD_FMT_OPT_SYS);
			    EndDialog(hwnd, IDCANCEL);
			} else {
			    OemToAnsi(lpMyFmtInf->FmtInf.VolLabel,FmtBuf5);
			    SetDlgItemText(hwnd, DLGCONFOR_LABEL, FmtBuf5);
			    LocalFree((HANDLE)pMsgBuf);
			}
		    }

		    // Re-Init DLGCONFOR_CAPCOMB

		    hCmb = GetDlgItem(hwnd, DLGCONFOR_CAPCOMB);
		    SendMessage(hCmb, CB_RESETCONTENT, 0, 0L);
		    j = 0xFFFF;
		    for(i = 0; i < lpMyFmtInf->FmtInf.PhysFmtCnt; i++)
		    {
			SendMessage(hCmb, CB_INSERTSTRING,0xFFFF,(LPARAM)(LPSTR)&lpMyFmtInf->FmtInf.PhysFmtNmList[i]);
			SendMessage(hCmb, CB_SETITEMDATA,i,(LPARAM)(DWORD)i);
			if(CurrSelFmtID == lpMyFmtInf->FmtInf.PhysFmtIDList[i])
			    j = i;
			if(j == 0xFFFF)
			{
			    if(lpMyFmtInf->FmtInf.PhysFmtIDList[i] == lpMyFmtInf->FmtInf.DefPhysFmtID)
				j = i;
			}
		    }
		    if(j == 0xFFFF)
			j = 0;
		    SendMessage(hCmb, CB_SETCURSEL, j, (LONG)NULL);
		    SendMessage(hCmb,EM_SETSEL,0,MAKELONG(0,0x7FFF));
		    SendDlgItemMessage(hwnd, DLGCONFOR_LABEL, EM_SETMODIFY, FALSE, 0L);
		    return(TRUE);
		    break;

		case DLGCONFOR_CANCEL:
		    if(lpMyFmtInf->FmtInProgBool)
		    {
			lpMyFmtInf->FmtCancelBool = TRUE;
		    } else {
			EndDialog(hwnd, IDCANCEL);
		    }
		    return(TRUE);
		    break;

		case DLGCONFOR_CAPCOMB:
		    if(IsDlgButtonChecked(hwnd, DLGCONFOR_DOSYS))
		    {
			return(TRUE);
		    }

		    // NOTE FALL THROUGH

		default:
		    return(FALSE);

	    }
	    break;

	case WM_APP+3:	       // Format complete
	    if(lpMyFmtInf->FmtOpt & DLGCONFOR_REP)
	    {
		switch(HIWORD(lpMyFmtInf->OpCmpltRet))
		{
		    case NOERROR:
		    case ERR_NONFATAL:
			DialogBoxParam(g_hInstance,
				       MAKEINTRESOURCE(DLG_FORMATREPORT),
				       hwnd,
				       FmtRepDlgWndProc,
				       (LPARAM)lpSHFmtInfo);
			break;

		    // case ERR_OSNOTFOUND:
		    // case ERR_OSERR:
		    // case ERR_NOTSUPPORTED:
		    // case ERR_INSUFMEM:
		    // case ERR_LOCKVIOLATION:
		    // case ERR_FSACTIVE:
		    // case ERR_BADOPTIONS:
		    // case ERR_FATAL:
		    // case OPCANCEL:
		    default:
			break;
		}
	    }
	    return(TRUE);
	    break;

	case WM_APP+4:	       // Format progress update
	    if(wParam)
	    {
		switch(lpMyFmtInf->FmtInf.CurrOpRegion)
		{
		    case FOP_INIT:
			i = IDS_F_INIT;
			goto SetStTxt;
			break;

		    case FOP_LOWFMT:
			i = IDS_F_LOWFMT;
			goto SetStTxt;
			break;

		    case FOP_VERIFY:
			i = IDS_F_VERIFY;
			goto SetStTxt;
			break;

		    case FOP_FSFMT:
			i = IDS_F_FSFMT;
			goto SetStTxt;
			break;

		    case FOP_TSYS:
			i = IDS_F_TSYS;
			goto SetStTxt;
			break;

		    case FOP_GETLABEL:
			i = IDS_F_GETLABEL;
			goto SetStTxt;
			break;

		    case FOP_SHTDOWN:
			i = IDS_F_SHTDOWN;
SetStTxt:
			if(lpMyFmtInf->CurrOpRegion != lpMyFmtInf->FmtInf.CurrOpRegion)
			{
			    pMsgBuf = (PSTR)LocalAlloc(LPTR,TOTMSZ5);
			    if(pMsgBuf)
			    {
				LoadString(g_hInstance, i, FmtBuf5, SZFMTBUF5);
				SetDlgItemText(hwnd, DLGCONFOR_STATTXT, FmtBuf5);
				LocalFree((HANDLE)pMsgBuf);
			    }
#ifdef OPK2
			    if((lpMyFmtInf->FmtInf.Options & FD_GETCONFIRM) &&
			       (lpMyFmtInf->CurrOpRegion == FOP_FSFMT)	      )
			    {
				EnableWindow(GetDlgItem(hwnd,DLGCONFOR_CANCEL),TRUE);
				if(lpMyFmtInf->FmtIsActive)
				{
				    SetFocus(GetDlgItem(hwnd,DLGCONFOR_CANCEL));
				}
			    }
#endif
			    lpMyFmtInf->CurrOpRegion = lpMyFmtInf->FmtInf.CurrOpRegion;
			}
			break;

		    default:
			break;		// Leave status unchanged
		}
	    }
	    SendDlgItemMessage(hwnd, DLGCONFOR_PBAR, PBM_SETPOS, lpMyFmtInf->FmtInf.TotalPcntCmplt, 0L);
	    return(TRUE);
	    break;

	case WM_HELP:
	    WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
		    (DWORD) (LPSTR) FmtaIds);
	    return(TRUE);
	    break;

	case WM_CONTEXTMENU:
	    WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
			    (DWORD) (LPSTR) FmtaIds);
	    return(TRUE);
	    break;

	default:
	    return(FALSE);
	    break;
    }
    return(FALSE);
}


//
// Exported API to format dialog.
//
DWORD WINAPI SHFormatDrive(HWND hwnd, UINT drive, UINT fmtID, UINT options)
{
    SHFORMATDRIVEINFO sSHFmtInfo;
    LPMYFMTINFOSTRUCT lpMyFmtInf;
    DWORD	      mret = SHFMT_ERROR;
    DWORD	      dwi;
    WORD	      i;

    if (!_InitTermDMaint(TRUE, &sSHFmtInfo.sDMaint))
    {
	ShellMessageBox(g_hInstance, hwnd,
			MAKEINTRESOURCE(IDS_NODSKMNT),
			MAKEINTRESOURCE(IDS_NAME),
			MB_ICONINFORMATION | MB_OK);
	goto Error0;
    }

    lpMyFmtInf = (LPMYFMTINFOSTRUCT)(NPMYFMTINFOSTRUCT)
		 LocalAlloc(LPTR,sizeof(MYFMTINFOSTRUCT));
    if(LOWORD(lpMyFmtInf) == 0)
    {
	ShellMessageBox(g_hInstance, hwnd,
			MAKEINTRESOURCE(IDS_NOMEM2),
			MAKEINTRESOURCE(IDS_NAME),
			MB_ICONINFORMATION | MB_OK);
	goto Error1;
    }
    sSHFmtInfo.lpMyFmtInf = lpMyFmtInf;

    lpMyFmtInf->lpwddi = (LPWINDISKDRIVEINFO)(NPWINDISKDRIVEINFO)
			 LocalAlloc(LPTR,sizeof(WINDISKDRIVEINFO));
    if(LOWORD(lpMyFmtInf->lpwddi) == 0)
    {
	ShellMessageBox(g_hInstance, hwnd,
			MAKEINTRESOURCE(IDS_NOMEM2),
			MAKEINTRESOURCE(IDS_NAME),
			MB_ICONINFORMATION | MB_OK);
	goto Error2;
    }

    lpMyFmtInf->lpwddi->iDrive = drive;

    InitDrvInfo(0, lpMyFmtInf->lpwddi, &sSHFmtInfo.sDMaint);

    dwi = sSHFmtInfo.sDMaint.lpfnGetFormatOptions(lpMyFmtInf->lpwddi->iDrive,
	&(lpMyFmtInf->FmtInf),sizeof(lpMyFmtInf->FmtInf));

    switch (HIWORD(dwi))
    {
	case ERR_NOTFULLSUPP:
	    if(LOWORD(dwi) & NOFORMAT)
	    {
		if(!(LOWORD(dwi) & FSONLY))
		    goto GenNotSupErr;
	    }
	    // NOTE fall through
	case NOERROR:
	    lpMyFmtInf->FmtCancelBool = FALSE;
	    lpMyFmtInf->FmtInProgBool = FALSE;
	    lpMyFmtInf->ReqFmtID = fmtID;
	    lpMyFmtInf->ReqFmtOpt = options & (~WD_FMT_OPT_SYS);
	    DialogBoxParam(g_hInstance,
			   MAKEINTRESOURCE(DLG_FORMAT),
			   hwnd,
			   FmtDlgWndProc,
			   (LPARAM)(LPSTR)&sSHFmtInfo);
	    if(lpMyFmtInf->LastFmtRslt == LASTFMTRSLT_ERR)
		mret = SHFMT_ERROR;
	    else if(lpMyFmtInf->LastFmtRslt == LASTFMTRSLT_CAN)
		mret = SHFMT_CANCEL;
	    else
		mret = MAKELONG(lpMyFmtInf->LastFmtRslt,0);
	    break;

	case ERR_ISSYSDRIVE:
	    if(LOWORD(dwi) & ISWINDRV)
	    {
		i = IDS_NOFORMATSYSW;
	    } else if(LOWORD(dwi) & ISPAGINGDRV) {
		i = IDS_NOFORMATSYSP;
	    } else if(LOWORD(dwi) & ISCVFHOSTDRV) {
		i = IDS_NOFORMATSYSH;
	    } else {
		i = IDS_NOFORMATSYS;
	    }
	    ShellMessageBox(g_hInstance, hwnd,
		MAKEINTRESOURCE(i), MAKEINTRESOURCE(IDS_NAME),
		MB_ICONINFORMATION | MB_OK);
	    mret = SHFMT_NOFORMAT;
	    break;

	case ERR_NOTSUPPORTED:
	default:
	    if(LOWORD(dwi) & ISLOCKED)
	    {
		i = IDS_GENDISKPROBL;
	    } else if(LOWORD(dwi) & ISALIASDRV) {
		i = IDS_GENDISKPROBA;
	    } else if(LOWORD(dwi) & NOFMTIOCTL) {
		i = IDS_GENDISKPROBI;
	    } else if(LOWORD(dwi) & ISCOMPDRV) {
		i = IDS_GENDISKPROBC;
	    } else {
GenNotSupErr:
		i = IDS_GENDISKPROB;
	    }
	    ShellMessageBox(g_hInstance, hwnd,
		MAKEINTRESOURCE(i), MAKEINTRESOURCE(IDS_NAME),
		MB_ICONINFORMATION | MB_OK);
	    mret = SHFMT_NOFORMAT;
	    break;
    }

    LocalFree((HANDLE)LOWORD(lpMyFmtInf->lpwddi));
Error2:;
    LocalFree((HANDLE)LOWORD(lpMyFmtInf));
Error1:;
    _InitTermDMaint(FALSE, &sSHFmtInfo.sDMaint);
Error0:;
    return(mret);
}
