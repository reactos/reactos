/*********************************************************************
Registration Wizard

This file houses a set of functions that use TAPI to access
information about installed modems.

11/15/94 - Tracy Ferrier
(c) 1994-95 Microsoft Corporation

Original source: MOS development
**********************************************************************/
#include <windows.h>
#include <mcx.h>
#include <tapi.h>
#include <devcfg.h>

#include "mcm.h"
#include "resource.h"
#include "ATK_RAS.h"

#include  "rw_common.h"


#define pcszDataModem		_T("comm/datamodem")
#define pcszWaitLineCreate	_T("MCMLineCreate")
#define PTSTR(wszID)		GetSz(hInst, wszID)
#define chBackslash		'\\'
#define irgMaxSzs		5
// Globals
static _TCHAR		szStrTable[irgMaxSzs][256];
static INT			iSzTable = 0;

MODEMSTATUS MSDetectModemTAPI(HINSTANCE hInstance);
BOOL DoInstallDialog(HINSTANCE hInstance,int nDialogType);
INT_PTR CALLBACK NoModemDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SetWaitLineCreateEvent(void);
void CenterDlg(HWND hWnd);
PVOID PVReadRegSt(HINSTANCE hInst, WORD wiszKey, WORD wiszVal);
PVOID PVReadReg(HKEY hKeyM, HINSTANCE hInst, WORD wiszKey, WORD wiszVal);
PTSTR GetSz(HINSTANCE hInst, WORD wszID);
void CALLBACK LineCallback(DWORD hDevice, DWORD dwMessage, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);

void CALLBACK CountryLineCallback1(DWORD hDevice, DWORD dwMessage, DWORD dwInstance, DWORD dwParam1,
										  DWORD dwParam2, DWORD dwParam3)
{

}





MODEMSTATUS MSEnsureModemTAPI(HINSTANCE hInstance,HWND hwnd)
/*********************************************************************
Ensures that a modem is installed and that tapi is setup. if not, user
will be prompted to do so.

Returns:
- kMsModemOk
- kMsModemNotFound
- kMsModemTooSlow
**********************************************************************/
{
	MODEMSTATUS			msReturnVal = kMsModemNotFound;
	do
	{
		msReturnVal = MSDetectModemTAPI(hInstance);
		if (msReturnVal != kMsModemOk)
		{	
			INT_PTR dlgReturn;
			int iDialogID = msReturnVal == kMsModemNotFound ? IDD_NOMODEM : IDD_MODEM_TOO_SLOW;
			dlgReturn = DialogBox(hInstance, MAKEINTRESOURCE(iDialogID), hwnd,
				NoModemDlgProc);
			if (!dlgReturn)
			{
				return kMsModemNotFound;
			}
			msReturnVal = MSDetectModemTAPI(hInstance);
		}
	}while (msReturnVal != kMsModemOk);
	return (msReturnVal);
}


MODEMSTATUS MSDetectModemTAPI(HINSTANCE hInstance)
/*********************************************************************
Returns:
- kMsModemOk
- kMsModemNotFound
- kMsModemTooSlow
**********************************************************************/
{
	HLINEAPP 	hLineApp;
	DWORD 		dwAPI;
	DWORD 		dwDevice;
	BOOL		fModem;
	const DWORD cMarvelBpsMin = 2400;
	MODEMSTATUS	msReturnVal = kMsModemNotFound;
	DWORD 		dwIndex = 0;
	#ifdef _LOG_IN_FILE
		RW_DEBUG  <<"\n Inside MSDetectModemTAPI "  << flush;
	#endif

	do
	{
		fModem = FGetDeviceID(hInstance, &hLineApp, &dwAPI, &dwDevice,dwIndex++);
		if (fModem)
		{
			DWORD dwSpeed;
			if (FGetModemSpeed(hInstance, dwDevice,&dwSpeed))
			{
				if (dwSpeed >= cMarvelBpsMin || 0 == dwSpeed )
				{
					msReturnVal = kMsModemOk; 	//modem speed is ok
				}
				else
				{
					msReturnVal = kMsModemTooSlow;
				}
			}
			lineShutdown(hLineApp);
		}
		
		RW_DEBUG << "\n Index:" << dwIndex << "fModem:"<<fModem <<flush;

	}while (fModem == TRUE && msReturnVal != kMsModemOk);
	return msReturnVal;
}



BOOL DoInstallDialog(HINSTANCE hInstance,int nDialogType)
/*********************************************************************
This function puts up the "Install new modem" control panel, and
returns only when the user has dismissed the dialog (either after
installing a new modem, or canceling).
Returns: FALSE if an error prevented the dialog from being displayed.
**********************************************************************/
{
	_TCHAR 				szCmdLine[128];
	STARTUPINFO 		si;
	PROCESS_INFORMATION pi;
	BOOL 				fControlProcessDone = FALSE;
	BOOL 				fProcessStatus;
	//HWND				hwndProcess;

	if(nDialogType == 1)
	{
		LoadString(hInstance,IDS_ADDMODEMCOMMAND,szCmdLine,128);
	}
	else
	{
		_tcscpy(szCmdLine,_T(""));
		LoadString(hInstance,IDS_DIALINGPROPERTIES,szCmdLine,128);
	}
	si.cb = sizeof(STARTUPINFO);
	si.lpReserved = NULL;
	si.lpDesktop = NULL;
	si.lpTitle = NULL;
	si.dwFlags = 0L;
	si.cbReserved2 = 0;
	si.lpReserved2 = NULL;
	fProcessStatus = CreateProcess(NULL,szCmdLine,NULL,NULL,FALSE,
		CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_PROCESS_GROUP | NORMAL_PRIORITY_CLASS,NULL,NULL,&si,&pi);
	if (fProcessStatus == FALSE)
	{
		return FALSE;
	}
	return TRUE;
}

// ======================== NoModemDlgProc ==============================

INT_PTR CALLBACK NoModemDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static fInstallModemDlgStarted = FALSE;
	static fDialingPropertiesStarted = FALSE;

	HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hWnd,GWLP_HINSTANCE);
	switch (uMsg)
	{
		case WM_INITDIALOG:
			CenterDlg(hWnd);
			SetForegroundWindow(hWnd);
			return fTrue;

		case WM_ACTIVATE:
			if (wParam != 0 && fInstallModemDlgStarted == TRUE)
			{
				DoInstallDialog(hInstance,2);
				fDialingPropertiesStarted = TRUE;
				fInstallModemDlgStarted = FALSE;
			}
			
			if (wParam != 0 && fDialingPropertiesStarted == TRUE)
			{
				fDialingPropertiesStarted = FALSE;
				EndDialog(hWnd,fTrue);
			}
			break;
		case WM_CLOSE:
			EndDialog(hWnd, fFalse);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
				{
				case IDCANCEL:
					EndDialog(hWnd, fFalse);
					break;
				case IDYES:
					DoInstallDialog(hInstance,1);
					fInstallModemDlgStarted = TRUE;
					EnableWindow(GetDlgItem(hWnd,IDYES),FALSE);
					break;
				case IDNO:
					EndDialog(hWnd, fFalse);
					return (fTrue);
				}		
			break;
		}

	return (fFalse);
}


BOOL FGetModemSpeed(HINSTANCE hInstance, DWORD dwDevice, PDWORD pdwSpeed)
/*********************************************************************
Given a lineDevice ID in the dwDevice parameter, FGetModemSpeed
returns the operating speed of the device represented by that ID.
**********************************************************************/
{
	BOOL		fRet = fFalse;
	PSTR		pvs = NULL;
	DWORD		dwRet;

	*pdwSpeed = 0;	//7/12/94 UmeshM. Init the variable
	Try
		{
		VARSTRING	vs;
		
		vs.dwTotalSize = sizeof(vs);
		dwRet = lineGetDevConfig(dwDevice, &vs, pcszDataModem);
		if (dwRet != 0 && dwRet != LINEERR_STRUCTURETOOSMALL)
			{
			//Dprintf("linegetdevconfig = %X\n", dwRet);
			Leave;
			}

		pvs =(char *)  LocalAlloc(LPTR, vs.dwNeededSize);
		if (!pvs)
			Leave;
			
		((VARSTRING *) pvs)->dwTotalSize = vs.dwNeededSize;
		dwRet = lineGetDevConfig(dwDevice, (LPVARSTRING)  pvs, pcszDataModem);
		if (dwRet != 0)
			{
			//Dprintf("linegetdevconfig = %X\n", dwRet);
			Leave;
			}

			{
			LPDEVCFG pDevCfg = (LPDEVCFG)(pvs + sizeof(VARSTRING));
			COMMCONFIG *pConf = (COMMCONFIG*)(pvs + sizeof(VARSTRING) + sizeof(DEVCFGHDR));
			MODEMSETTINGS *pSet;
			
			pSet = (MODEMSETTINGS*)( ((char*)pConf) + pConf->dwProviderOffset);
			
			//if (FAmIOnline())
			//	*pdwSpeed = pSet->dwNegotiatedDCERate;
			//else
				*pdwSpeed = pConf->dcb.BaudRate;
			fRet = fTrue;
			}
		} // Try
		
	Finally
		{
		if (pvs)
			LocalFree(pvs);
			
		}
		
	return (fRet);
} // FGetModemSpeed()



// ======================== FGetDeviceID ==============================
BOOL FGetDeviceID(HINSTANCE hInstance, HLINEAPP *phLineApp, PDWORD pdwAPI, PDWORD pdwDevice, DWORD dwIndex)
{
	DWORD				dwDevices, iDevice, dwRet;
	LINEEXTENSIONID		ExtID;
	LINEDEVCAPS			dc, *pdc;
	PSTR				pszCL;

	DWORD				dwLocalIndex;
	#ifdef _LOG_IN_FILE
		RW_DEBUG  <<"\n Inside FGetDeviceID "  << flush;
	#endif

	//Assert(phLineApp && pdwDevice && pdwAPI);
	if ((dwRet = lineInitialize(phLineApp, hInstance, (LINECALLBACK) LineCallback, NULL, &dwDevices)) != 0)
		{
			#ifdef _LOG_IN_FILE
				RW_DEBUG  <<"\n lineInitialize Error "  << dwRet << flush;
			#endif
			return (fFalse);
		}

	pszCL = (PSTR) PVReadRegSt(hInstance, iszLoginKey, IDS_CURRENTCOMMDEV);
	*pdwDevice = INVALID_PORTID;
	dc.dwTotalSize = sizeof(LINEDEVCAPS);
	
	dwLocalIndex = 0;
	
	RW_DEBUG << "dwDevices: " << dwDevices << flush;

	for (iDevice = 0; iDevice < dwDevices; iDevice ++)
	{
		DWORD dwAPILowVersion = 0 << 16;
		DWORD dwAPIHighVersion = 3 << 16;
		
		RW_DEBUG << "\n Enter 0" << flush;
	
		dwRet = lineNegotiateAPIVersion(*phLineApp, iDevice,  dwAPILowVersion,dwAPIHighVersion, pdwAPI, &ExtID);
		dwRet |= lineGetDevCaps(*phLineApp, iDevice, *pdwAPI, 0, &dc);
		
		if (dc.dwMediaModes & LINEMEDIAMODE_DATAMODEM && dwRet == 0)
		{
			RW_DEBUG << "\ndwIndex:" <<dwIndex<< " dwLocalIndex:" << dwLocalIndex <<flush;

			if (dwLocalIndex++ == dwIndex)
			{
				RW_DEBUG << "\n Enter 2 iDevice:" << iDevice << flush;

				if (*pdwDevice == INVALID_PORTID)
					*pdwDevice = iDevice;
				
				pdc = (LINEDEVCAPS *) LocalAlloc(LPTR, dc.dwNeededSize);
				if (pdc)
				{
					RW_DEBUG << "\n Enter 3" << flush;

					pdc->dwTotalSize = dc.dwNeededSize;
					dwRet |= lineGetDevCaps(*phLineApp, iDevice, *pdwAPI, 0, pdc);
					if (pdc->dwLineNameSize > 0 && pszCL)
					{
						RW_DEBUG << "\n Enter 3" << flush;
						PSTR	pszLineName;
					
			            pszLineName = (LPSTR)(pdc) + pdc->dwLineNameOffset;
		
			            // Use if specified in registry
			            if (strcmp(pszCL, pszLineName) == 0)
							*pdwDevice = iDevice;
					}
					LocalFree(pdc);
					RW_DEBUG << "\n Exit 3" << flush;
				}
				RW_DEBUG << "\n Exit 2" << flush;
			}
			RW_DEBUG << "\n Exit 1" << flush;
		}
		RW_DEBUG << "\n Exit 0" << flush;
	}
								
	if (*pdwDevice == INVALID_PORTID)
		{ // no data modem found
			#ifdef _LOG_IN_FILE
				RW_DEBUG  <<"\n lineGetDevCaps returned INVALID_PORTID Error"  << flush;
			#endif

			lineShutdown(*phLineApp);
			return (fFalse);
		}
		
	return (fTrue);
} // FGetDeviceID()				



void CenterDlg(HWND hWnd)
{
	HWND	hwndOwner;
	RECT	rcOwner, rcDlg, rc;
	
	if (((hwndOwner = GetParent(hWnd)) == NULL) || IsIconic(hwndOwner) || !IsWindowVisible(hWnd))
		hwndOwner = GetDesktopWindow();
	GetWindowRect(hwndOwner, &rcOwner);
	GetWindowRect(hWnd, &rcDlg);
	CopyRect(&rc, &rcOwner);

	
	// Offset the owner and dialog box rectangles so that
	// right and bottom values represent the width and
	// height, and then offset the owner again to discard
	// space taken up by the dialog box.

	OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
	OffsetRect(&rc, -rc.left, -rc.top);
	OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

	// The new position is the sum of half the remaining
	// space and the owner's original position.

	SetWindowPos(hWnd,HWND_TOP, rcOwner.left + (rc.right / 2),
		rcOwner.top + (rc.bottom / 2),	0, 0, SWP_NOSIZE);
} // CenterDlg


// ======================== PVReadRegSt ==============================

PVOID PVReadRegSt(HINSTANCE hInst, WORD wiszKey, WORD wiszVal)
{
	return (PVReadReg(HKEY_CURRENT_USER, hInst, wiszKey, wiszVal));
} // PVReadRegSt()

// ======================== PVReadReg ==============================

// returns pointer to struct. REMEMBER to free the pointer(LocalFree)

PVOID PVReadReg(HKEY hKeyM, HINSTANCE hInst, WORD wiszKey, WORD wiszVal)
{
	HKEY	hKey = NULL;
	PVOID	pVal = NULL;
	DWORD	dwCb;
	BOOL	fRead = fFalse;
		

	Try
		{
		if (RegOpenKey(hKeyM, PTSTR(wiszKey), &hKey) != ERROR_SUCCESS)
			Leave;
			
		if (!(RegQueryValueEx(hKey, PTSTR(wiszVal), NULL, NULL, NULL, &dwCb) == ERROR_SUCCESS && dwCb))
			Leave;
			
		pVal = LocalAlloc(LPTR, dwCb);
		if (pVal == NULL)
			Leave;
				
		if (RegQueryValueEx(hKey, PTSTR(wiszVal), NULL,
			NULL,(LPBYTE ) pVal, &dwCb) != ERROR_SUCCESS)
			Leave;
		fRead = fTrue;
		}
		
	Finally
		{
		RegCloseKey(hKey);
		if (fRead == fFalse)
			{
				if(pVal != NULL)
				{
					LocalFree(pVal);
					pVal = NULL;
				}
			}
		}
		
	return (pVal);
} // PVReadReg()


// BEWARE uses static variable.
PTSTR GetSz(HINSTANCE hInst, WORD wszID)
{
	PTSTR	psz = szStrTable[iSzTable];
	
	iSzTable ++;
	if (iSzTable >= irgMaxSzs)
		iSzTable = 0;
		
	if (!LoadString(hInst, wszID, psz, 256))
		{	// now u could return a error but then everybody will have to check
			// the return value
		//AssertGLE(0);
		*psz = 0;
		}
		
	return (psz);
} // GetSz()


void SetWaitLineCreateEvent(void)
{
	HANDLE hEvent;

	hEvent = OpenEvent(SYNCHRONIZE, fFalse, pcszWaitLineCreate);
	if (hEvent)
		{
		SetEvent(hEvent);
		CloseHandle(hEvent);
		}
}

void CALLBACK LineCallback(DWORD hDevice, DWORD dwMessage, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{
	switch (dwMessage)
	{
		case LINE_LINEDEVSTATE:
		{
			if (dwParam1 == LINEDEVSTATE_REINIT)
			{
				SetWaitLineCreateEvent();
			}
		}
			break;
	}
} // LineCallback

#define  MAX_RAS_DEVICES   10
#define  SZ_BUF_RET_SZ     256
//
// iModemIndex : Index of Modem Nane required, should be starting from 1 to MAX_RAS_DEVICES
//
//
TCHAR * GetModemDeviceInformation(HINSTANCE hInstance, int iModemIndex)
{

	static  TCHAR  szRetValue[MAX_RAS_DEVICES][SZ_BUF_RET_SZ]
		={	_T(""),_T(""),_T(""),
			_T(""),_T(""),_T(""),
			_T(""),_T(""),_T(""),
			_T("")};
	static  TCHAR szNoModem[] = _T("NOMODEM");
	static  int iFirstTimeCall=0;
	static  int nNoofModemDevice = 0;
		
	RASDEVINFO  *pdi,*pDevInfo;

	DWORD dwSize=0,dwNum=0,dwRet;
	int iEntries;
	int i;

	if( iModemIndex < 1 ) {
		return szNoModem;
	}
	if(iFirstTimeCall == 0 )
	{
		iFirstTimeCall = 1;
		if( ATK_IsRasDllOk() == RAS_DLL_LOADED)
		{
			dwRet = ATK_RasEnumDevices(NULL,&dwSize,&dwNum);
			iEntries = dwSize /sizeof(RASDEVINFO);
		
			pDevInfo = (LPRASDEVINFO)LocalAlloc(LPTR,dwSize);
			
			pdi = pDevInfo;
			
			for(i=0; i< iEntries;i++) {
				pdi->dwSize = sizeof(RASDEVINFO);
				pdi++;
			}
			dwRet = ATK_RasEnumDevices(pdi,&dwSize,&dwNum);
			if(dwRet == 0)
			{
			
				for(i=0; i< iEntries;i++)
				{
					#ifdef _LOG_IN_FILE
						RW_DEBUG << "\n Device Name:"<< ConvertToANSIString(pdi->szDeviceName) << flush;
						RW_DEBUG << "\n Device Type:"<< ConvertToANSIString(pdi->szDeviceType) << flush;
					#endif

						if( !_tcscmp(pdi->szDeviceType, RASDT_Modem) )
						{
							_tcscpy(szRetValue[nNoofModemDevice],pdi->szDeviceName);
							nNoofModemDevice++;
							//i=iEntries+1;
						}
					pdi++;
				}
			}
			else
			{
				switch(dwRet)
				{
					case ERROR_BUFFER_TOO_SMALL:
						RW_DEBUG <<"\n ERROR_BUFFER_TOO_SMALL"<< flush;
						break;
					case ERROR_NOT_ENOUGH_MEMORY:
						RW_DEBUG <<"\n ERROR_NOT_ENOUGH_MEMORY"<< flush;
						break;
					case ERROR_INVALID_PARAMETER:
						RW_DEBUG <<"\n ERROR_INVALID_PARAMETER"<< flush;
						break;
					case ERROR_INVALID_USER_BUFFER :
						RW_DEBUG <<"\n ERROR_INVALID_USER_BUFFER"<< flush;
						break;
					default:
						RW_DEBUG <<"\n UNKNOWN_ERROR"<< flush;
						break;
				}

			}
			LocalFree(pDevInfo);
		}
		else
		{
			return szNoModem;
		}	
	}

	// Return the  modem device name
	if( iModemIndex  > nNoofModemDevice ) {
			return szNoModem;
	}else {
			return &szRetValue[iModemIndex-1][0];
	}
		
	
}
