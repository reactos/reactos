/****************************** Module Header ******************************\
* Module Name: accrare.c
*
* Copyright (c) 1985-95, Microsoft Corporation
*
* History:
* 12-18-95 a-jimhar 	Created based on rare.c
\***************************************************************************/


#include "Access.h"

#define REGSTR_PATH_SERIALKEYS  __TEXT("Control Panel\\Accessibility\\SerialKeys")
#define REGSTR_VAL_ACTIVEPORT   __TEXT("ActivePort")
#define REGSTR_VAL_BAUDRATE     __TEXT("BaudRate")
#define REGSTR_VAL_FLAGS        __TEXT("Flags")
#define REGSTR_VAL_PORTSTATE    __TEXT("PortState")

#define PMAP_STICKYKEYS            __TEXT("Control Panel\\Accessibility\\StickyKeys")
#define PMAP_KEYBOARDRESPONSE  __TEXT("Control Panel\\Accessibility\\Keyboard Response")
#define PMAP_MOUSEKEYS		   __TEXT("Control Panel\\Accessibility\\MouseKeys")
#define PMAP_TOGGLEKEYS 	   __TEXT("Control Panel\\Accessibility\\ToggleKeys")
#define PMAP_TIMEOUT		   __TEXT("Control Panel\\Accessibility\\TimeOut")
#define PMAP_SOUNDSENTRY	   __TEXT("Control Panel\\Accessibility\\SoundSentry")
#define PMAP_SHOWSOUNDS 	   __TEXT("Control Panel\\Accessibility\\ShowSounds")

#define ISACCESSFLAGSET(s, flag) ((s).dwFlags & flag)

#define SK_SPI_INITUSER -1

typedef int (*PSKEY_SPI)(
	UINT uAction, 
	UINT uParam, 
	LPSERIALKEYS lpvParam, 
	BOOL fuWinIni);


/****************************************************************************/

BOOL AccessSKeySystemParametersInfo(
	UINT uAction, 
	UINT uParam, 
	LPSERIALKEYS psk, 
	BOOL fu)
{
	BOOL fRet = FALSE;
    static PSKEY_SPI s_pSKEY_SystemParametersInfo =NULL;
    static BOOL s_fSKeySPIAttemptedLoad = FALSE;
#ifdef UNICODE
	static BOOL s_fMustConvert = FALSE;
	CHAR szActivePort[MAX_PATH]; 
	CHAR szPort[MAX_PATH]; 
	PWSTR pszActivePort; 
	PWSTR pszPort; 
#endif


	if (NULL == s_pSKEY_SystemParametersInfo && !s_fSKeySPIAttemptedLoad)
	{
		BOOL fRc = FALSE;
		HINSTANCE hinst = LoadLibrary(__TEXT("SKDLL.DLL"));

		if (NULL != hinst) 
		{

#ifdef UNICODE

			s_pSKEY_SystemParametersInfo = (PSKEY_SPI)GetProcAddress(
				(HMODULE)hinst, "SKEY_SystemParametersInfoW");
			if (NULL == s_pSKEY_SystemParametersInfo) 
			{
				s_pSKEY_SystemParametersInfo = (PSKEY_SPI)GetProcAddress(
					(HMODULE)hinst, "SKEY_SystemParametersInfo");
				s_fMustConvert = TRUE;
			}

#else
			s_pSKEY_SystemParametersInfo = (PSKEY_SPI)GetProcAddress(
				(HMODULE)hinst, "SKEY_SystemParametersInfo");

#endif // UNICODE

			// We don't bother calling FreeLibrary(hinst), the library will be freed
			// when the app terminates
		}
        s_fSKeySPIAttemptedLoad = TRUE;
	}

    if (NULL != s_pSKEY_SystemParametersInfo)
    {

#ifdef UNICODE

		if (s_fMustConvert) 
		{
            memset(szActivePort, 0, sizeof(szActivePort));
            memset(szPort, 0, sizeof(szPort));

	        pszActivePort = psk->lpszActivePort; 
            pszPort = psk->lpszPort; 

			if (NULL != psk->lpszActivePort)
			{
				psk->lpszActivePort = (PTSTR)szActivePort;
			}

			if (NULL != psk->lpszPort)
			{
				psk->lpszPort = (PTSTR)szPort; 
			}

            if (SPI_SETSERIALKEYS == uAction)
			{
				if (NULL != psk->lpszActivePort)
				{
					WideCharToMultiByte(
						CP_ACP, 0, pszActivePort, -1, 
						(PCHAR)psk->lpszActivePort, MAX_PATH, NULL, NULL);
				}
				if (NULL != psk->lpszPort)
				{
					WideCharToMultiByte(
						CP_ACP, 0, pszPort, -1, 
						(PCHAR)psk->lpszPort, MAX_PATH, NULL, NULL);
				}
			}
		}
#endif // UNICODE

		fRet = (BOOL)(*s_pSKEY_SystemParametersInfo)(
    		uAction, 
			uParam, 
			psk, 
			fu);
#ifdef UNICODE

		if (s_fMustConvert && SPI_GETSERIALKEYS == uAction) 
		{

			if (NULL != psk->lpszActivePort)
			{
				MultiByteToWideChar(
					CP_ACP, 0, (PCHAR)psk->lpszActivePort, -1,
					pszActivePort, MAX_PATH);
			}
			if (NULL != psk->lpszPort)
			{
				MultiByteToWideChar(
					CP_ACP, 0, (PCHAR)psk->lpszPort, -1,
					pszPort, MAX_PATH);
			}
		}
		if (NULL != psk->lpszActivePort)
		{
			psk->lpszActivePort = pszActivePort;
		}

		if (NULL != psk->lpszPort)
		{
			psk->lpszPort = pszPort; 
		}

#endif // UNICODE

    }
    return(fRet);
}


/***************************************************************************\
* FixupAndRetrySystemParametersInfo
*
* Used by access but not implemented by NT's SPI:
*
* SPI_GETKEYBOARDPREF
* SPI_SETKEYBOARDPREF
*
* SPI_GETHIGHCONTRAST
* SPI_SETHIGHCONTRAST
*
* SPI_GETSERIALKEYS
* SPI_SETSERIALKEYS
*
*
* History:
* 12-18-95 a-jimhar 	Created, derived from xxxSystemParametersInfo
* 01-22-95 a-jimhar 	Removed old code that worked around NT bugs
*
* On NT this function fixes the parameters and calls SystemParametersInfo
*
\***************************************************************************/

static BOOL FixupAndRetrySystemParametersInfo(
	UINT wFlag,
	DWORD wParam,
	PVOID lParam,
	UINT flags)	// we ignoring this flag 
				// could add support for SPIF_UPDATEINIFILE and SPIF_SENDCHANGE
{
	BOOL fRet = FALSE;
	BOOL fCallSpi = FALSE;
	BOOL fChanged = FALSE;

	if (NULL != (PVOID)lParam)
	{
		switch (wFlag) {


		// Fake support
		case SPI_GETKEYBOARDPREF:
			{
				*(PBOOL) lParam = FALSE;
				fRet = TRUE;
				fCallSpi = FALSE;
			}
			break;

		case SPI_GETSERIALKEYS:
            {
		        LPSERIALKEYS psk = (LPSERIALKEYS)lParam;

			    if (NULL != psk &&
			       (sizeof(*psk) == psk->cbSize || 0 == psk->cbSize))
			    {
					fRet = AccessSKeySystemParametersInfo(
						wFlag, 
						0, 
						psk, 
						TRUE);
                }
                fCallSpi = FALSE;
            }
			break;

		case SPI_SETSERIALKEYS:
            {
		        LPSERIALKEYS psk = (LPSERIALKEYS)lParam;

			    if (NULL != psk &&
			       (sizeof(*psk) == psk->cbSize || 0 == psk->cbSize))
			    {
					fRet = AccessSKeySystemParametersInfo(
						wFlag, 
						0, 
						psk, 
						TRUE);
	    			fChanged = TRUE;
                }
		        fCallSpi = FALSE;
            }
			break;

		default:
			// This function is only for fix-up and second chance calls.
			// We didn't fix anything, don't call SPI.
			fCallSpi = FALSE;
			fRet = FALSE;
			break;
		}
	}

	if (fCallSpi)
	{
		fRet = SystemParametersInfo(wFlag, wParam, lParam, flags);
	}
	else if (fChanged && (flags & SPIF_SENDCHANGE))
	{
        DWORD_PTR dwResult;

        SendMessageTimeout(
			HWND_BROADCAST, 
			WM_WININICHANGE, 
			wFlag, 
			(LONG_PTR)NULL,
            SMTO_NORMAL, 
			100, 
			&dwResult);
	}
	return(fRet);
}

/***************************************************************************\
* AccessSystemParametersInfo
*
* History:
* 12-18-95 a-jimhar 	Created.
\***************************************************************************/

BOOL AccessSystemParametersInfo(
	UINT wFlag,
	DWORD wParam,
	PVOID lParam,
	UINT flags)
{
	BOOL fRet;

	// first give the system SPI a chance

	fRet = SystemParametersInfo(wFlag, wParam, lParam, flags);

	if (!fRet && g_fWinNT)
	{
		// the system SPI failed, fixup the params and try again

		fRet = FixupAndRetrySystemParametersInfo(wFlag, wParam, lParam, flags);
	}

	return(fRet);
}
