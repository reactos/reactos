/*********************************************************************
Registration Wizard

regutil.cpp

Standard RegWizard utility functions 

12/7/94 - Tracy Ferrier
(c) 1994-96 Microsoft Corporation

  Modification History:
  5/26/99 : The Regsitration Version number should have the OS build number 
  The build number should be tahen from HKLM/SW/Ms/Windows NT/CurrentBuildNumber
  eg 3.0.nnnn 
**********************************************************************/

#include <Windows.h>
#include <winnt.h>
#include "regutil.h"
#include "resource.h"
#include <stdio.h>
#include "rwwin95.h"
#include "sysinv.h"
#include "version.h"
#include "rw_common.h"



void EncryptBlock (PTBYTE lpbBlock, DWORD dwBlockSize );
void DecryptBlock (PTBYTE lpbBlock, DWORD dwBlockSize );
#define BITS_PER_BYTE		8
WORD vrgwRot[] = {2,3,2,1,4,2,1,1};

_TUCHAR g_rgchCredits[] = {
#include "credits.h"
};
static  HFONT   shBoldFont;
static  HFONT   shNormalFont;

void InitDlgNormalBoldFont()
{
	shBoldFont   = NULL;
	shNormalFont = NULL;
}
void DeleteDlgNormalBoldFont()
{
	int iInd;
	if(shBoldFont) {
		DeleteObject(shBoldFont);
		RW_DEBUG << "\n In Delete Bold Font:[" << shBoldFont << flush;
	}
	if(shNormalFont){
		DeleteObject(shNormalFont);
		RW_DEBUG << "\n In Delete Normal Font:[" << shNormalFont<< flush;
	}

	shBoldFont   = NULL;
	shNormalFont = NULL;

		
}
// Modified on 04/07/98 
// Previously a font was created for all of the controls, when ever the function is called. 
// Now 2 fonts Normal and Bold is created only once and is used by all the controls calling this fn
//
//
HFONT NormalizeDlgItemFont(HWND hwndDlg,int idControl, int iMakeBold)
/*********************************************************************
This function removes any BOLD weight attribute attached to the
dialog control specified by hwndDlg and idControl.
**********************************************************************/
{
	// This is needed only when running under NT. Under
	// Win95, the dialog text is unbolded by default.
	
	HFONT hfont;
	hfont = NULL;
	
	//if(iMakeBold != RWZ_MAKE_BOLD){
	//	return hfont;
	//}
		if (Windows95OrGreater())
	{

		hfont = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0L);
		if (hfont != NULL){
			LOGFONT lFont;
			if (!GetObject(hfont, sizeof(LOGFONT), (LPTSTR)&lFont))
			{
				hfont = NULL;
			}
			else
			{
				if(iMakeBold == RWZ_MAKE_BOLD){
						lFont.lfWeight = FW_BOLD;
						// Create Bold Font
						if(shBoldFont==NULL){
							shBoldFont = CreateFontIndirect((LPLOGFONT)&lFont);
							RW_DEBUG << "\n\tIn Create Bold Font:[" << shBoldFont << flush;
						}
						hfont = shBoldFont;
				}else {
					lFont.lfWeight = FW_NORMAL;
						// Create Normal Font
						if(shNormalFont==NULL){
							shNormalFont = CreateFontIndirect((LPLOGFONT)&lFont);
							RW_DEBUG << "\n\tIn Create Normal Font:[" << shNormalFont <<  flush;
						}
						hfont = shNormalFont;
				}

							
				if (hfont != NULL)
				{			
				
					SendDlgItemMessage(hwndDlg, idControl, WM_SETFONT,(WPARAM)hfont , 0L);
				}
			}
		}
	}
	
	return hfont;
}


//
//
// Old implementation of NormalizeDlgItemFont(),this fn was used till 
// 4/7/98
// 
void NormalizeDlgItemFont1(HWND hwndDlg,int idControl, int iMakeBold)
/*********************************************************************
This function removes any BOLD weight attribute attached to the
dialog control specified by hwndDlg and idControl.
**********************************************************************/
{
	// This is needed only when running under NT. Under
	// Win95, the dialog text is unbolded by default.
	if (!Windows95OrGreater())
	//if (Windows95OrGreater())
	{
		HFONT hfont = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0L);
		if (hfont != NULL)
		{
			LOGFONT lFont;
			if (!GetObject(hfont, sizeof(LOGFONT), (LPTSTR)&lFont))
			{
				hfont = NULL;
			}
			else
			{
				if(iMakeBold == RWZ_MAKE_BOLD){
						lFont.lfWeight = FW_BOLD;
				}else {
					lFont.lfWeight = FW_NORMAL;
				}
			
				hfont = CreateFontIndirect((LPLOGFONT)&lFont);
				if (hfont != NULL)
				{
					SendDlgItemMessage(hwndDlg, idControl, WM_SETFONT,(WPARAM)hfont, 0L);
				}
			}
		}
	}
}


void ReplaceDialogText(HWND hwndDlg,int idControl,LPTSTR szText)
/*********************************************************************
For the dialog control indicated by hwndDlg and idControl, this
function replaces the first occurrence of '%s' with the text pointed
to by szText.
**********************************************************************/
{
	_TCHAR szCurrentText[512];
	_TCHAR szTextBuffer[512];
	LRESULT textLen = SendDlgItemMessage(hwndDlg, idControl, WM_GETTEXT, (WPARAM) 512, (LPARAM) szCurrentText);
	_stprintf(szTextBuffer,szCurrentText,szText);
	SendDlgItemMessage(hwndDlg,idControl,WM_SETTEXT,0,(LPARAM) szTextBuffer);
}



void UpgradeDlg(HWND hwndDlg)
/*********************************************************************
Applies an RegWizard defined set of "upgrades" to the dialog whose
handle is given in the hwndDlg parameter.
**********************************************************************/
{
	// Turn SS_BLACKFRAME line into SS_ETCHEDFRAME
	HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwndDlg,GWLP_HINSTANCE);
	HWND hwndEtchedLine = GetDlgItem(hwndDlg,IDC_ETCHEDLINE);
	if (hwndEtchedLine)
	{
		SetWindowLongPtr(hwndEtchedLine,GWL_STYLE,SS_ETCHEDFRAME | WS_VISIBLE | WS_CHILD);
	}
	//CenterDlg(hwndDlg);// We now set the DS_CENTER window style instead

	if (Windows95OrGreater())
	{
		HICON hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_REGWIZ));
		SendMessage(hwndDlg,WM_SETICON,(WPARAM) TRUE,(LPARAM) hIcon);
	}
}


void LoadAndCombineString(HINSTANCE hInstance,LPCTSTR szReplaceWith,int idReplacementString,LPTSTR szString)
/*********************************************************************
This function replaces the first occurrence of "%s" in szTarget with 
the string specified by the idReplacementString resource ID, and 
returns the resulting string in the szString parameter.
**********************************************************************/
{
	_TCHAR szTarget[256];
	LoadString(hInstance,idReplacementString,szTarget,256);
	_stprintf(szString,szTarget,szReplaceWith);
}



 void StripCharFromString(LPTSTR szInString, LPTSTR szOutString, _TCHAR charToStrip)
 /***********************************************************************
 Strips the given character from szInString, and returns the result in
 szOutString.
 ************************************************************************/
 {
 	while (1)
	{
		if (*szInString != charToStrip)
		{
		  //*szOutString++ = *szInString;
			_tcscpy(szOutString,szInString);
			szOutString = _tcsinc(szOutString);
		}
		//if (*szInString++ == NULL) return;
		if (*szInString == NULL) return;
		szInString = _tcsinc(szInString);
	};
 }



BOOL GetIndexedRegKeyValue(HINSTANCE hInstance, int enumIndex, LPTSTR szBaseKey,int valueStrID, LPTSTR szValue)
/*********************************************************************
Looks for a subkey, under the Registration Database key given in the
szBaseKey parameter, of the form "0000", "0001", etc.  The numerical
equivalent of the subkey is determined by the index value given in
the enumIndex parameter.  The value attached to the valueName
specified in the string resource whose ID is given in valueStrID will
be returned in szValue.

Returns: FALSE if the key specified is not found. 
**********************************************************************/
{
	_TCHAR szRegKey[256];
	_stprintf(szRegKey,_T("%s\\%04i"),szBaseKey,enumIndex);

	BOOL returnVal = FALSE;
	HKEY hKey;
	LONG regStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,szRegKey,0,KEY_READ,&hKey);
	if (regStatus == ERROR_SUCCESS)
	{
		_TCHAR szValueName[128];
		LoadString(hInstance,valueStrID,szValueName,128);
		unsigned long infoSize = 255;
		regStatus = RegQueryValueEx(hKey,szValueName,NULL,0,(LPBYTE) szValue,&infoSize);
		if (regStatus == ERROR_SUCCESS)
		{
			returnVal = TRUE;
		}
		RegCloseKey(hKey);
	}
	return returnVal;
}



BOOL FileExists(LPTSTR szPathName)
/***************************************************************************
Returns TRUE if the file specified by the given pathname actually exists.
****************************************************************************/
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	HANDLE fileHandle = CreateFile(szPathName,GENERIC_READ,0,&sa,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

	BOOL retValue;
	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		retValue = FALSE;
	}
	else
	{
		retValue = TRUE;
		CloseHandle(fileHandle);
	}
	return retValue;
}


void UppercaseString(LPTSTR sz)
/*********************************************************************
Converts the given string to uppercase, in place.
**********************************************************************/
{
	if (sz)
	{
		while (*sz)
		{
			*sz = _totupper(*sz);
			//sz++;
			sz = _tcsinc(sz);
		}
	}
}

LONG GetResNumber(HINSTANCE hInstance, int iStrResID)
/*********************************************************************
Loads the string whose ID is given by the iStrResID parameter, and
returns the numerical equivalent of the string's value.
**********************************************************************/
{
	_TCHAR szRes[256];
	LoadString(hInstance,iStrResID,szRes,255);
	LONG lResNum = _ttol(szRes);
	return lResNum;
}


BOOL Windows95OrGreater( void )
/*********************************************************************
Returns TRUE if the current operating system is Windows 4.0 or better.
**********************************************************************/
{
	LONG lPlatform, lMajorVersion,lMinorVersion,lBuildNumber;
	GetWindowsVersion(&lPlatform,&lMajorVersion,&lMinorVersion,&lBuildNumber);
	return lMajorVersion >= 4 ? TRUE : FALSE;
}


void DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, int xStart,int yStart, 
	int xWidth, int yWidth, int xSrc, int ySrc, COLORREF cTransparentColor)
/*********************************************************************
Draws the given bitmap, treating the color given by the 
cTransparentColor parameter as transparent.
**********************************************************************/
{
	BITMAP     bm;
	COLORREF   cColor;
	HBITMAP    bmAndBack, bmAndObject, bmAndMem, bmSave;
	HBITMAP    bmBackOld, bmObjectOld, bmMemOld, bmSaveOld;
	HDC        hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
	POINT      ptSize,ptBmSize;

	hdcTemp = CreateCompatibleDC(hdc);
	SelectObject(hdcTemp, hBitmap);   // Select the bitmap
	GetObject(hBitmap, sizeof(BITMAP), (LPTSTR)&bm);
	ptSize.x = xWidth;            // Get width of bitmap
	ptSize.y = yWidth;           // Get height of bitmap
	ptBmSize.x = bm.bmWidth;            // Get width of bitmap
	ptBmSize.y = bm.bmHeight;           // Get height of bitmap
	DPtoLP(hdcTemp, &ptSize, 1);      // Convert from device
										// to logical points
	// Create some DCs to hold temporary data.
	hdcBack   = CreateCompatibleDC(hdc);
	hdcObject = CreateCompatibleDC(hdc);
	hdcMem    = CreateCompatibleDC(hdc);
	hdcSave   = CreateCompatibleDC(hdc);
   
	// Create a bitmap for each DC. DCs are required for a number of GDI functions.
	// Monochrome DC
	bmAndBack   = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);
	
	// Monochrome DC
	bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);
	bmAndMem    = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
	bmSave      = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
   
	// Each DC must select a bitmap object to store pixel data.
	bmBackOld   = (HBITMAP) SelectObject(hdcBack, bmAndBack);
	bmObjectOld = (HBITMAP) SelectObject(hdcObject, bmAndObject);
	bmMemOld    = (HBITMAP) SelectObject(hdcMem, bmAndMem);
	bmSaveOld   = (HBITMAP) SelectObject(hdcSave, bmSave);
	
	// Set proper mapping mode.
	SetMapMode(hdcTemp, GetMapMode(hdc));
	
	// Save the bitmap sent here, because it will be overwritten.
	BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, xSrc, ySrc, SRCCOPY);
   
	// Set the background color of the source DC to the color.
	// contained in the parts of the bitmap that should be transparent
	cColor = SetBkColor(hdcTemp, cTransparentColor);
  
  	// Create the object mask for the bitmap by performing a BitBlt
	// from the source bitmap to a monochrome bitmap.
	BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, xSrc, ySrc,SRCCOPY);

	// Set the background color of the source DC back to the original color.
	SetBkColor(hdcTemp, cColor);
   
	// Create the inverse of the object mask.
	BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0,NOTSRCCOPY);
   
	// Copy the background of the main DC to the destination.
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, xStart, yStart,SRCCOPY);
   
	// Mask out the places where the bitmap will be placed.
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND);
   
	// Mask out the transparent colored pixels on the bitmap.
	BitBlt(hdcTemp, xSrc, ySrc, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);
	
	// XOR the bitmap with the background on the destination DC.
	BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, xSrc, ySrc, SRCPAINT);
   
	// Copy the destination to the screen.
	BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, 0, 0,SRCCOPY);
   
	// Place the original bitmap back into the bitmap sent here.
	BitBlt(hdcTemp, xSrc, ySrc, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);
   
	// Delete the memory bitmaps.
	DeleteObject(SelectObject(hdcBack, bmBackOld));
	DeleteObject(SelectObject(hdcObject, bmObjectOld));
	DeleteObject(SelectObject(hdcMem, bmMemOld));
	DeleteObject(SelectObject(hdcSave, bmSaveOld));

	// Delete the memory DCs.
	DeleteDC(hdcMem);
	DeleteDC(hdcBack);
	DeleteDC(hdcObject);
	DeleteDC(hdcSave);
	DeleteDC(hdcTemp);
}


BOOL GetSystemLanguageInfo(LPTSTR lpszLanguage, DWORD dwBufferSize,LANGID* lpLangID)
/*********************************************************************
Returns, in the buffer pointed to by the lpszLanguage parameter, a
string describing the current system language setting.  The
corresponding language ID is returned in the buffer pointed to by
lpLangID (pass NULL for lpLangID if you don't need this value).
**********************************************************************/
{
	LANGID langID = GetSystemDefaultLangID();
	if (lpLangID) *lpLangID = langID;
	DWORD dwRet = VerLanguageName(langID,lpszLanguage,dwBufferSize);
	return dwRet == 0 ? FALSE : TRUE;
}


void GetRegWizardVersionString(HINSTANCE hInstance, LPTSTR lpszVersion)
/*********************************************************************
Returns a string representing the current release version of RegWizard
**********************************************************************/
{
	TCHAR  czBuildNo[24];
	DWORD  dwStatus;
	TCHAR  uszRegKey[128];
	HKEY  hKey; 
	DWORD dwBuildNo,dwInfoSize, dwValueType;

	dwBuildNo = 0;
	uszRegKey[0] = 0;
	LoadString(hInstance, IDS_REREGISTER_OS2, uszRegKey, sizeof(uszRegKey));
	
	czBuildNo[0] =0;
	dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, uszRegKey, 0, KEY_READ, &hKey);
	if(dwStatus == ERROR_SUCCESS) {
		LoadString(hInstance, IDS_OSBUILDNUMBER, uszRegKey, sizeof(uszRegKey));
		dwInfoSize = sizeof(czBuildNo);
		RegQueryValueEx(hKey, uszRegKey, NULL, &dwValueType, (LPBYTE)czBuildNo, &dwInfoSize);
		RegCloseKey(hKey);
	}
	wsprintf(lpszVersion,_T("%i.%i.%s"),rmj,rmm,czBuildNo);
	
}


void RegWizardInfo(HWND hwndDlg)
/*********************************************************************
**********************************************************************/
{
	if (GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_SHIFT) < 0)
	{
		static BOOL fDecrypt = FALSE;
		long iLen = _tcslen((LPTSTR)g_rgchCredits);
		if (fDecrypt == FALSE)
		{
			fDecrypt = TRUE;
			DecryptBlock((PTBYTE) g_rgchCredits,iLen);
			g_rgchCredits[iLen] = 0;
		}
		_TCHAR rgchVersion[60];
		HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwndDlg,GWLP_HINSTANCE);
		GetRegWizardVersionString(hInstance, rgchVersion);

		_TCHAR rgchInfo[340];
		_TCHAR szAbout[256];
    

		LoadString(hInstance,IDD_MICROSOFT_ABOUT_MSG,szAbout,256);
		wsprintf(rgchInfo,szAbout,rgchVersion,0xA9,g_rgchCredits);
		LoadString(hInstance,IDD_MICROSOFT_ABOUT_CAPTION,szAbout,256);
		MessageBox(hwndDlg,rgchInfo,szAbout,MB_OK | MB_ICONINFORMATION);
	}
}

void DecryptBlock (PTBYTE lpbBlock, DWORD dwBlockSize )
/**************************************************************
EncryptBlock decrypts a data block that was encrypted by the
EncryptBlock function.
 
- lpbBlock: a pointer to the block to be decrypted.
- dwBlockSize: the size, in bytes, of the given block.

Returns: nothing.
**************************************************************/
{
	for (DWORD x = 0;x < dwBlockSize;x += 2)
	{
		if ((x + 1) < dwBlockSize)
		{
			lpbBlock[x + 1] = (TBYTE)(lpbBlock[x] ^ lpbBlock[x + 1]);
		}
		WORD wRot = vrgwRot[x & 0x07];
		lpbBlock[x] = (TBYTE) ((lpbBlock[x] >> wRot) + (lpbBlock[x] << (BITS_PER_BYTE - wRot)));

	} 
}



UINT GetRegKeyValue32 ( HKEY hRootKey, LPTSTR const cszcSubKey, LPTSTR const cszcValueName,
	 PDWORD pdwType, PTBYTE pbData, UINT cbData )
/**********************************************************************
Determines the value associated with the specified Registration
Database key and value name.

Returns:
	The cb of the key data if successful, 0 otherwise.
Notes:
	If hRootKey is NULL, HKEY_CLASSES_ROOT is used for the root
***********************************************************************/
{
	HKEY hSubKey;
	LONG lErr;
	DWORD cbSize = (DWORD)cbData;

	if (hRootKey == NULL)
		hRootKey = HKEY_CLASSES_ROOT;

	lErr = RegOpenKeyEx(hRootKey, cszcSubKey, 0, KEY_READ, &hSubKey);
	if (lErr != ERROR_SUCCESS)
	{
		pdwType[0] = NULL;
		return 0;	/* Return 0 if the key doesn't exist */
	}

	lErr = RegQueryValueEx(hSubKey, (LPTSTR)cszcValueName, NULL, pdwType, (LPBYTE)pbData,
						   &cbSize);
	RegCloseKey(hSubKey);
	if (lErr != ERROR_SUCCESS)
	{
		pdwType[0] = NULL;
		return 0;	/* Return 0 if the value doesn't exist */
	}

	return (UINT)cbSize;
}


BOOL FSetDialogTabOrder(HWND hwndDlg,LPTSTR szTabOrder)
/*********************************************************************
FSetDialogTabOrder sets the tabbing order of all controls in a dialog.
Optionally, the control that should initially have the focus can be
set at the same time.

hwndDlg: handle to the dialog containing the controls.
szTabOrder: a string containing the dlg ID's of all controls in the
dialog, separated by commas.  The order of the ID's in the string
determines what the new tabbing order will be.  If any ID is 
immediately followed with 'F', the corresponding control will be
given the initial input focus.
Example: "1001,1002,105F,72,1006,1005,1007,1008"

Returns:
- TRUE: Tabbing order successfully set
- FALSE: one or more ID's in szTabOrder did not correspond to a valid
			control in the given dialog.
**********************************************************************/
{
	if (szTabOrder == NULL || szTabOrder[0] == NULL) return FALSE;

	LPTSTR sz = szTabOrder;
	int iCtrl1 = 0;
	int iCtrl2 = 0;
	while (*sz)
	{
		LPTSTR szEnd;
		iCtrl1 = iCtrl2;
		iCtrl2 = _tcstol(sz,&szEnd,10);
		HWND hwndCtrl1 = GetDlgItem(hwndDlg, iCtrl1);
		HWND hwndCtrl2 = GetDlgItem(hwndDlg, iCtrl2);
		if (!hwndCtrl2) return FALSE;

		if (*szEnd == _T('F') || *szEnd == _T('f'))
		{
			if (hwndCtrl2) SetFocus(hwndCtrl2);
			//szEnd++;
			szEnd = _tcsinc(szEnd);
		}
		if (hwndCtrl1 && hwndCtrl2)
		{
			SetWindowPos(hwndCtrl2,hwndCtrl1,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
		}
		//sz = *szEnd == NULL ? szEnd : szEnd+1;
		sz = (*szEnd == NULL) ? szEnd : _tcsinc(szEnd);
	}
	return TRUE;
}


BOOL FResSetDialogTabOrder(HWND hwndDlg, UINT wResStringID)
/*********************************************************************
Same as FSetDialogTabOrder, except that instead of passing a pointer
to a string, the wResStringID parameter specifies a string in the 
resource string table.  

Returns:
If the string represented by wResStringID cannot be loaded, or if one
or more of the ID's in the loaded string do not correspond to a valid
dialog control, FALSE will be returned as the function result.
**********************************************************************/
{
	_TCHAR rgchTabOrder[256];
	HINSTANCE hInstance = (HINSTANCE) GetWindowLongPtr(hwndDlg,GWLP_HINSTANCE);
	if (LoadString(hInstance,wResStringID,rgchTabOrder,256) > 0)
	{
		return FSetDialogTabOrder(hwndDlg,rgchTabOrder);
	}
	else
	{
		return FALSE;
	}
}



