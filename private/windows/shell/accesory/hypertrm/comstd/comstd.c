/*  File: D:\WACKER\comstd\comstd.c (Created: 08-Dec-1993)
 *
 *  Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *  All rights reserved
 *
 *  $Revision: 3 $
 *  $Date: 9/24/99 5:01p $
 */
#include <windows.h>
#pragma hdrstop

#define     DEBUGSTR
#define DEBUG_CHARDUMP

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\mc.h>
#include <tdll\sf.h>
#include <tdll\timers.h>
#include <tdll\com.h>
#include <tdll\comdev.h>
#include "comstd.hh"
#if defined(INCL_WINSOCK)
#include <comwsock\comwsock.hh>
#endif  // defined(INCL_WINSOCK)
#include <tdll\assert.h>
#include <tdll\statusbr.h>
#include <tdll\com.hh>
#include "rc_id.h"


#if defined(DEBUG_CHARDUMP)
    #include <stdio.h>
	FILE *pfDbgR = NULL;
    FILE *pfDbgC = NULL;
#endif

BOOL WINAPI ComStdEntry(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);
BOOL WINAPI _CRT_INIT(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);
static void ComstdSettingsToDCB(ST_STDCOM_SETTINGS *pstSettings, DCB *pstDcb);
static void ComstdDCBToSettings(DCB *pstDcb, ST_STDCOM_SETTINGS *pstSettings);
static void DeviceBreakTimerProc(void *pvData, long ulSince); //mrw:6/15/95

HINSTANCE hinstDLL = (HINSTANCE)0;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *      very temporary - mrw
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int GetAutoDetect(ST_STDCOM *pstPrivate)
    {
    return pstPrivate->stWorkSettings.fAutoDetect;
    }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  ComStdEntry
 *
 * DESCRIPTION:
 *  Currently, just initializes the C-Runtime library but may be used
 *  for other things later.
 *
 * ARGUMENTS:
 *  hInstDll    - Instance of this DLL
 *  fdwReason   - Why this entry point is called
 *  lpReserved  - reserved
 *
 * RETURNS:
 *  BOOL
 *
 */
BOOL WINAPI ComStdEntry(HINSTANCE hInst, DWORD fdwReason, LPVOID lpReserved)
    {
    hinstDLL = hInst;
    return _CRT_INIT(hInst, fdwReason, lpReserved);
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: DeviceInitialize
 *
 * DESCRIPTION:
 *  Called whenever the driver is being loaded
 *
 * ARGUMENTS:
 *  hCom               -- A copy of the com handle. Can be used in the
 *                          driver code to call com services
 *  usInterfaceVersion -- A version number identifying the version of the
 *                          driver interface
 *  ppvDriverData      -- A place to put the pointer to our private data.
 *                          This value will be passed back to us in all
 *                          subsequent calls.
 *
 * RETURNS:
 *  COM_OK if all is hunky dory
 *  COM_DEVICE_VERSION_ERROR if HA/Win expects a different interface version.
 *  COM_NOT_ENOUGH_MEMORY
 *  COM_DEVICE_ERROR if anything else goes wrong
 */
int WINAPI DeviceInitialize(HCOM hCom,
    unsigned nInterfaceVersion,
    void **ppvDriverData)
    {
    int        iRetVal = COM_OK;
    int        ix;
    ST_STDCOM *pstPrivate = NULL;

    //              Check version number and compatibility

    if (nInterfaceVersion != COM_VERSION)
        {
        // This error is reported by Com Routines. We cannot report errors
        // until after DeviceInitialize has completed.
        return COM_DEVICE_VERSION_ERROR;
        }

    if (*ppvDriverData)
        {
        pstPrivate = *ppvDriverData;
        }
    else
        {
        // Allocate our private storage structure
        if ((pstPrivate = malloc(sizeof *pstPrivate)) == NULL)
            return COM_NOT_ENOUGH_MEMORY;
        *ppvDriverData = pstPrivate;

        // These members are common to both com drivers
        //
        pstPrivate->hCom = hCom;
        pstPrivate->fNotifyRcv = TRUE;
        pstPrivate->dwEventMask = 0;
        pstPrivate->fSending = FALSE;
        pstPrivate->lSndTimer = 0L;
        pstPrivate->lSndLimit = 0L;
        pstPrivate->lSndStuck = 0L;
        pstPrivate->hwndEvents = (HWND)0;
        pstPrivate->nRBufrSize = SIZE_INQ;
        pstPrivate->pbBufrStart = NULL;
        pstPrivate->fHaltThread = TRUE;

        InitializeCriticalSection(&pstPrivate->csect);
        for (ix = 0; ix < EVENT_COUNT; ++ix)
            {
            pstPrivate->ahEvent[ix] = CreateEvent((LPSECURITY_ATTRIBUTES)0,
                TRUE, FALSE, NULL);
            if (!pstPrivate->ahEvent[ix])
                {
                iRetVal = COM_FAILED;
                break;
                }
            }
        }

    // These members are specific to the stdcom driver
    //
    pstPrivate->bLastMdmStat = 0;
    pstPrivate->pbSndBufr = NULL;
    pstPrivate->nParityErrors = 0;
    pstPrivate->nFramingErrors = 0;
    pstPrivate->nOverrunErrors = 0;
    pstPrivate->nOverflowErrors = 0;

    pstPrivate->hWinComm = INVALID_HANDLE_VALUE;
    pstPrivate->fBreakSignalOn = FALSE;
    // Setup up reasonable default device values in case this type of
    //  device has not been used in a session before
    pstPrivate->stWorkSettings.lBaud          = 2400L;
    //pstPrivate->stWorkSettings.lBaud          = 9600L;
    pstPrivate->stWorkSettings.nDataBits      = 8;
    pstPrivate->stWorkSettings.nStopBits      = ONESTOPBIT;
    pstPrivate->stWorkSettings.nParity        = NOPARITY;
    pstPrivate->stWorkSettings.afHandshake    = HANDSHAKE_RCV_RTS |
        HANDSHAKE_SND_CTS;
    pstPrivate->stWorkSettings.chXON          = 0x11;
    pstPrivate->stWorkSettings.chXOFF         = 0x13;
    pstPrivate->stWorkSettings.nBreakDuration = 750;
    pstPrivate->stWorkSettings.fAutoDetect    = TRUE;
    pstPrivate->stFileSettings = pstPrivate->stWorkSettings;

    pstPrivate->fADRunning = FALSE;
    pstPrivate->nADTotal = 0;
    pstPrivate->nADMix = 0;
    pstPrivate->nAD7o1 = 0;
    pstPrivate->nADHighBits = 0;
    pstPrivate->nADBestGuess = AD_DONT_KNOW;
    pstPrivate->chADLastChar = '\0';
    pstPrivate->fADToggleParity = FALSE;
    pstPrivate->fADReconfigure = FALSE;

    pstPrivate->hComstdThread = NULL;

    if (iRetVal != COM_OK)
        {
        if (pstPrivate)
			{
            free(pstPrivate);
			pstPrivate = NULL;
			}
        }

    return iRetVal;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: DeviceClose
 *
 * DESCRIPTION:
 *  Called when HA/Win is done with this driver and is about to release .DLL
 *
 * ARGUMENTS:
 *  pstPrivate -- Pointer to our private data structure
 *
 * RETURNS:
 *  COM_OK
 */
int WINAPI DeviceClose(ST_STDCOM *pstPrivate)
    {
    int ix;

    // Driver is about to be let go, do any cleanup
    // Port should have been deactivated before we are called, but
    //  check anyway.
    PortDeactivate(pstPrivate);

    for (ix = 0; ix < EVENT_COUNT; ++ix)
        {
        CloseHandle(pstPrivate->ahEvent[ix]);
        }
    DeleteCriticalSection(&pstPrivate->csect);
    // Free our private data area
    free(pstPrivate);
	pstPrivate = NULL;

    return COM_OK;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  ComLoadStdcomDriver
 *
 * DESCRIPTION:
 *  Loads the COM handle with pointers to the stdcom driver functions
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *  COM_OK		if successful
 *	COM_FAILED	otherwise
 *
 * AUTHOR:
 * mcc 12/26/95
 */
int ComLoadStdcomDriver(HCOM pstCom)
	{
	int	iRetVal = COM_OK;

	if ( !pstCom )
		return COM_FAILED;

	pstCom->pfPortActivate   = PortActivate;
	pstCom->pfPortDeactivate = PortDeactivate;
	pstCom->pfPortConnected  = PortConnected;
	pstCom->pfRcvRefill 	 = RcvRefill;
	pstCom->pfRcvClear		 = RcvClear;
	pstCom->pfSndBufrSend	 = SndBufrSend;
	pstCom->pfSndBufrIsBusy  = SndBufrIsBusy;
	pstCom->pfSndBufrClear	 = SndBufrClear;
	pstCom->pfSndBufrQuery	 = SndBufrQuery;
	pstCom->pfDeviceSpecial	 = DeviceSpecial;
	pstCom->pfPortConfigure	 = PortConfigure;
    pstCom->pfDeviceDialog   = DeviceDialog;

	return iRetVal;
	}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: DeviceDialog
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
/*lint ARGSUSED*/
int WINAPI DeviceDialog(ST_STDCOM *pstPrivate, HWND hwndParent)
    {
    int iRetValue = COM_OK;
    COMMCONFIG stCC;
    TCHAR szPortName[COM_MAX_PORT_NAME];
    int nLen;

    memset(&stCC, 0, sizeof(stCC));
    stCC.dwSize = sizeof(stCC);
    stCC.wVersion = 1;
    stCC.dwProviderSubType = PST_RS232;
    nLen = sizeof(szPortName);
    ComGetPortName(pstPrivate->hCom, szPortName, &nLen);
    ComstdSettingsToDCB(&pstPrivate->stWorkSettings, &stCC.dcb);

    if (CommConfigDialog(szPortName, hwndParent, &stCC))
        {
        ComstdDCBToSettings(&stCC.dcb, &pstPrivate->stWorkSettings);
        }
    else
        {
        iRetValue = COM_CANCELLED;
		//DbgShowLastError();
        }

    return iRetValue;
    }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: DeviceGetCommon
 *
 * DESCRIPTION:
 *  Gets user settings common to all drivers
 *
 * ARGUMENTS:
 *  pstPrivate -- Our private data structure
 *  pstcommon  -- Pointer to structure of type ST_COMMON to be filled in
 *                  with the desired settings
 *
 * RETURNS:
 *  Always returns COM_OK
 */
int WINAPI DeviceGetCommon(ST_STDCOM *pstPrivate, ST_COMMON *pstCommon)
    {
    pstCommon->afItem = (COM_BAUD |
             COM_DATABITS |
             COM_STOPBITS |
                         COM_PARITY |
                         COM_AUTO);
    pstCommon->lBaud           = pstPrivate->stWorkSettings.lBaud;
    pstCommon->nDataBits   = pstPrivate->stWorkSettings.nDataBits;
    pstCommon->nStopBits   = pstPrivate->stWorkSettings.nStopBits;
    pstCommon->nParity         = pstPrivate->stWorkSettings.nParity;
    pstCommon->fAutoDetect = pstPrivate->stWorkSettings.fAutoDetect;

    return COM_OK;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: DeviceSetCommon
 *
 * DESCRIPTION:
 *  Passes common user settings to driver for use and storage.
 *
 * ARGUMENTS:
 *  pstPrivate
 *  pstCommon  -- Structure containing common user settings to be used
 *                by driver
 *
 * RETURNS:
 *  Always returns COM_OK
 */
int WINAPI DeviceSetCommon(ST_STDCOM *pstPrivate, ST_COMMON *pstCommon)
    {
    if (bittest(pstCommon->afItem, COM_BAUD))
        pstPrivate->stWorkSettings.lBaud     = pstCommon->lBaud;
    if (bittest(pstCommon->afItem, COM_DATABITS))
        pstPrivate->stWorkSettings.nDataBits = pstCommon->nDataBits;
    if (bittest(pstCommon->afItem, COM_STOPBITS))
        pstPrivate->stWorkSettings.nStopBits = pstCommon->nStopBits;
    if (bittest(pstCommon->afItem, COM_PARITY))
        pstPrivate->stWorkSettings.nParity   = pstCommon->nParity;
    if (bittest(pstCommon->afItem, COM_AUTO))
        pstPrivate->stWorkSettings.fAutoDetect = pstCommon->fAutoDetect;

    return COM_OK;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: DeviceSpecial
 *
 * DESCRIPTION:
 *  The means for others to control any special features in this driver
 *  that are not supported by all drivers.
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *  COM_NOT_SUPPORTED if the instruction string was not recognized
 *  otherwise depends on instruction string
 */
/*ARGSUSED*/
int WINAPI DeviceSpecial(ST_STDCOM *pstPrivate,
    const TCHAR *pszInstructions,
    TCHAR *pszResult,
    int   nBufrSize)
    {

    int           iRetVal = COM_NOT_SUPPORTED;
    HSESSION      hSession;
#if 0           //* do port of this odd fellow later
    unsigned      usMask = 0;
    unsigned long ulSetVal;
    TCHAR        *pszEnd;
    TCHAR         achInstructions[100]; // John: decide how you want to handle
    TCHAR        *pszToken = achInstructions;
    int           iIndex;
    TCHAR         szResult[100];

    static TCHAR *apszItems[] =
    {
    "HANDSHAKE_RCV_X",
    "HANDSHAKE_RCV_DTR",
    "HANDSHAKE_RCV_RTS",
    "HANDSHAKE_SND_X",
    "HANDSHAKE_SND_CTS",
    "HANDSHAKE_SND_DSR",
    "HANDSHAKE_SND_DCD",
    "XON_CHAR",
    "XOFF_CHAR",
    "BREAK_DURATION",
    "CTS_STATUS",
    "DSR_STATUS",
    "DCD_STATUS",
    "DTR_STATE",
    "RTS_STATE",
    "MODIFIED",     // remove when real temporary settings are in
    NULL
    };

    // supported instruction strings:
    // "Set xxx=vv"
    // "Query xxx"

    if (!pszInstructions || !*pszInstructions)
        return COM_FAILED;
    if (sizeof(achInstructions) < (size_t)(lstrlen(pszInstructions) + 1))
        return COM_NOT_SUPPORTED;

    strcpy(achInstructions, pszInstructions);

    if (pszResult)
        *pszResult = '\0';

    pszToken = strtok(achInstructions, " ");
    if (!pszToken)
        return COM_NOT_SUPPORTED;

    if (lstrcmpi(pszToken, "SET") == 0)
        {
        iRetVal = COM_OK;
        pszToken = strtok(NULL, " =");
        if (!pszToken)
            pszToken = "";

        // Look up the item to set
        for (iIndex = 0; apszItems[iIndex]; ++iIndex)
            if (lstrcmpi(pszToken, apszItems[iIndex]) == 0)
                break;

        // Isolate the new value to be set
        pszToken = strtok(NULL, "\n");

        if (pszToken && *pszToken)
            {
            // Several items take numeric values
            ulSetVal = strtoul(pszToken, &pszEnd, 0);

            switch(iIndex)
                {
            case 0: // RCV_X
                usMask = HANDSHAKE_RCV_X;
                break;

            case 1: // RCV_DTR
                usMask = HANDSHAKE_RCV_DTR;
                break;

            case 2: // RCV_RTS
                usMask = HANDSHAKE_RCV_RTS;
                break;

            case 3: // SND_X
                usMask = HANDSHAKE_SND_X;
                break;

            case 4: // SND_CTS
                usMask = HANDSHAKE_SND_CTS;
                break;

            case 5: // SND_DSR
                usMask = HANDSHAKE_SND_DSR;
                break;

            case 6: // SND_DCD
                usMask = HANDSHAKE_SND_DCD;
                break;

            case 7: // XON_CHAR
                if (!*pszEnd && ulSetVal <= UCHAR_MAX)
                    pstPrivate->stWorkSettings.chXON = (TCHAR)ulSetVal;
                else
                    iRetVal = COM_FAILED;
                break;

            case 8: // XOFF_CHAR
                if (!*pszEnd && ulSetVal <= UCHAR_MAX)
                    pstPrivate->stWorkSettings.chXOFF = (TCHAR)ulSetVal;
                else
                    iRetVal = COM_FAILED;
                break;

            case 9: // BREAK_DURATION
                if (!*pszEnd && ulSetVal <= USHRT_MAX)
                    pstPrivate->stWorkSettings.nBreakDuration = (USHORT)ulSetVal;
                else
                    iRetVal = COM_FAILED;
                break;

            case 13: // DTR_STATE
                if (pstPrivate->hWinComm != INVALID_HANDLE_VALUE)
                    {
                    switch (ulSetVal)
                        {
                    case 0:
                        EscapeCommFunction(pstPrivate->hWinComm, CLRDTR);
                        break;

                    case 1:
                        EscapeCommFunction(pstPrivate->hWinComm, SETDTR);
                        break;

                    default:
                        iRetVal = COM_FAILED;
                        break;
                        }
                    }
                else
                    iRetVal = COM_PORT_NOT_OPEN;

                break;

            case 14: // RTS_STATE
                if (pstPrivate->hWinComm != INVALID_HANDLE_VALUE)
                    {
                    switch (ulSetVal)
                        {
                    case 0:
                        EscapeCommFunction(pstPrivate->hWinComm, CLRRTS);
                        break;

                    case 1:
                        EscapeCommFunction(pstPrivate->hWinComm, SETRTS);
                        break;

                    default:
                        iRetVal = COM_FAILED;
                        break;
                        }
                    }
                else
                    iRetVal = COM_PORT_NOT_OPEN;

                break;

                // TODO: remove when real temp settings are implemented
            case 15: // MODIFIED
                break;

            default:  // Who was that masked man?
                iRetVal = COM_FAILED;
                break;
                }

            if (usMask != 0)
                {
                // Must have been a handshake setting
                if (strcmp(pszToken, "1") == 0)
                    bitset(pstPrivate->stWorkSettings.afHandshake, usMask);
                else if (strcmp(pszToken, "0") == 0)
                    bitclear(pstPrivate->stWorkSettings.afHandshake,usMask);
                else
                    {
                    iRetVal = COM_FAILED;
                    }
                }
            }
        else    // if (pszToken && *pszToken)
            {
            iRetVal = COM_NOT_SUPPORTED;
            }
        }
    else if (lstrcmpi(pszToken, "QUERY") == 0)
        {
        iRetVal = COM_OK;
        pszToken = strtok(NULL, "\n");
        szResult[0] = '\0';

        // Look up the item to query
        for (iIndex = 0; apszItems[iIndex]; ++iIndex)
            if (lstrcmpi(pszToken, apszItems[iIndex]) == 0)
                break;

        if (*pszToken)
            {
            switch(iIndex)
                {
            case 0: // RCV_X
                usMask = HANDSHAKE_RCV_X;
                break;

            case 1: // RCV_DTR
                usMask = HANDSHAKE_RCV_DTR;
                break;

            case 2: // RCV_RTS
                usMask = HANDSHAKE_RCV_RTS;
                break;

            case 3: // SND_X
                usMask = HANDSHAKE_SND_X;
                break;

            case 4: // SND_CTS
                usMask = HANDSHAKE_SND_CTS;
                break;

            case 5: // SND_DSR
                usMask = HANDSHAKE_SND_DSR;
                break;

            case 6: // SND_DCD
                usMask = HANDSHAKE_SND_DCD;
                break;

            case 7: // XON_CHAR
                wsprintf(szResult, "%u", pstPrivate->stWorkSettings.chXON);
                break;

            case 8: // XOFF_CHAR
                wsprintf(szResult, "%u", pstPrivate->stWorkSettings.chXOFF);
                break;

            case 9: // BREAK_DURATION
                wsprintf(szResult, "%u", pstPrivate->stWorkSettings.nBreakDuration);
                break;

            case 10: // CTS_STATUS
                strcpy(szResult, bittest(*pbMdmStat, MDMSTAT_CTS) ? "1" : "0");
                break;

            case 11: // DSR_STATUS
                strcpy(szResult, bittest(*pbMdmStat, MDMSTAT_DSR) ? "1" : "0");
                break;

            case 12: // DCD_STATUS
                strcpy(szResult, bittest(*pbMdmStat, MDMSTAT_DCD) ? "1" : "0");
                break;

            case 15: // MODIFIED
                strcpy(szResult, "0");
                break;

            default:  // Who was that masked man?
                iRetVal = COM_FAILED;
                break;
                }

            if (usMask != 0)
                {
                // Must have been a handshake setting
                strcpy(szResult,
                    bittest(pstPrivate->stWorkSettings.afHandshake, usMask) ? "1" : "0");
                }

            if (szResult[0])
                {
                if (!pszResult || strlen(szResult) > uiBufrSize)
                    iRetVal = COM_FAILED;
                else
                    strcpy(pszResult, szResult);
                }
            }
        }
    else if (lstrcmpi(pszToken, "SEND") == 0)
        {
        pszToken = strtok(NULL, "\n");
        if (lstrcmpi(pszToken, "BREAK") == 0)
            {
            if (pstPrivate->hWinComm != INVALID_HANDLE_VALUE && !pstPrivate->fBreakSignalOn)
                {
                SndBufrClear(pstPrivate);
                SetCommBreak(pstPrivate->hWinComm);
                ComGetSession(pstPrivate->hCom, &hSession);

                if (TimerCreate(mGetTimerMuxHdl(hSession),
                    &pstPrivate->hTmrBreak, pstPrivate->stWorkSettings.nBreakDuration,
                    MakeProcInstance((FARPROC)DeviceBreakTimerProc, hinstDLL),
                    (DWORD)pstPrivate) != TIMER_OK)
                    {
                    //* DeviceReportError(pstPrivate, SID_ERR_NOTIMER, 0, TRUE);
                    iRetVal = COM_DEVICE_ERROR;
                    }

                pstPrivate->fBreakSignalOn = TRUE;
                iRetVal = COM_OK;
                }
            }
        }

#endif
    // Implement only the Break function.  All other comm functions handled
    // through TAPI. - mrw:6/15/95
    //
    if (_stricmp(pszInstructions, "Send Break") == 0)
        {
        if (pstPrivate->hWinComm != INVALID_HANDLE_VALUE && !pstPrivate->fBreakSignalOn)
            {
            SndBufrClear(pstPrivate);
            SetCommBreak(pstPrivate->hWinComm);
            ComGetSession(pstPrivate->hCom, &hSession);

            if (TimerCreate(sessQueryTimerMux(hSession),
                    &pstPrivate->hTmrBreak, pstPrivate->stWorkSettings.nBreakDuration,
                    DeviceBreakTimerProc,
                    pstPrivate) != TIMER_OK)
                {
                //* DeviceReportError(pstPrivate, SID_ERR_NOTIMER, 0, TRUE);
                iRetVal = COM_DEVICE_ERROR;
                }

            pstPrivate->fBreakSignalOn = TRUE;
            iRetVal = COM_OK;
            }
        }

    return iRetVal;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  DeviceLoadHdl
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *  pstPrivate  -- driver data structure
 *
 * RETURNS:
 *
 */
int WINAPI DeviceLoadHdl(ST_STDCOM *pstPrivate, SF_HANDLE sfHdl)
    {
    unsigned long ul;

    // Load comm settings from the session file. If we connect via TAPI,
    // several of these settings will be inherited from TAPI and these
    // values will not be used.
    ul = sizeof(pstPrivate->stWorkSettings.lBaud);
    sfGetSessionItem(sfHdl, SFID_COMSTD_BAUD, &ul,
            &pstPrivate->stWorkSettings.lBaud);

    ul = sizeof(pstPrivate->stWorkSettings.nDataBits);
    sfGetSessionItem(sfHdl, SFID_COMSTD_DATABITS, &ul,
            &pstPrivate->stWorkSettings.nDataBits);

    ul = sizeof(pstPrivate->stWorkSettings.nStopBits);
    sfGetSessionItem(sfHdl, SFID_COMSTD_STOPBITS, &ul,
            &pstPrivate->stWorkSettings.nStopBits);

    ul = sizeof(pstPrivate->stWorkSettings.nParity);
    sfGetSessionItem(sfHdl, SFID_COMSTD_PARITY, &ul,
            &pstPrivate->stWorkSettings.nParity);

    ul = sizeof(pstPrivate->stWorkSettings.afHandshake);
    sfGetSessionItem(sfHdl, SFID_COMSTD_HANDSHAKING, &ul,
            &pstPrivate->stWorkSettings.afHandshake);

    ul = sizeof(pstPrivate->stWorkSettings.fAutoDetect);
    sfGetSessionItem(sfHdl, SFID_COMSTD_AUTODETECT, &ul,
        &pstPrivate->stWorkSettings.fAutoDetect);

    return SF_OK;
    }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  DeviceSaveHdl
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *  pstPrivate  -- driver data structure
 *
 * RETURNS:
 *
 */
int WINAPI DeviceSaveHdl(ST_STDCOM *pstPrivate, SF_HANDLE sfHdl)
    {
    // Save settings in session file space. Many of these values may be
    // overwritten by TAPI settings but are used by direct connect.
    sfPutSessionItem(sfHdl, SFID_COMSTD_BAUD,
            sizeof(pstPrivate->stWorkSettings.lBaud),
            &pstPrivate->stWorkSettings.lBaud);

    sfPutSessionItem(sfHdl, SFID_COMSTD_DATABITS,
            sizeof(pstPrivate->stWorkSettings.nDataBits),
            &pstPrivate->stWorkSettings.nDataBits);

    sfPutSessionItem(sfHdl, SFID_COMSTD_STOPBITS,
            sizeof(pstPrivate->stWorkSettings.nStopBits),
            &pstPrivate->stWorkSettings.nStopBits);

    sfPutSessionItem(sfHdl, SFID_COMSTD_PARITY,
            sizeof(pstPrivate->stWorkSettings.nParity),
            &pstPrivate->stWorkSettings.nParity);

    sfPutSessionItem(sfHdl, SFID_COMSTD_HANDSHAKING,
            sizeof(pstPrivate->stWorkSettings.afHandshake),
            &pstPrivate->stWorkSettings.afHandshake);

    sfPutSessionItem(sfHdl, SFID_COMSTD_AUTODETECT,
        sizeof(pstPrivate->stWorkSettings.fAutoDetect),
        &pstPrivate->stWorkSettings.fAutoDetect);

    return SF_OK;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: PortActivate
 *
 * DESCRIPTION:
 *  Called to activate the port and open it for use
 *
 * ARGUMENTS:
 *  pstPrivate  -- driver data structure
 *  pszPortName -- the name of the port to activate
 *
 * RETURNS:
 *  COM_OK if port is successfully activated
 *  COM_NOT_ENOUGH_MEMORY if there in insufficient memory for data storage
 *  COM_NOT_FOUND if named port cannot be opened
 *  COM_DEVICE_ERROR if API errors are encountered
 */
int WINAPI PortActivate(ST_STDCOM *pstPrivate,
    TCHAR *pszPortName,
    DWORD_PTR dwMediaHdl)
    {
    TCHAR           szFullPortName[_MAX_PATH];
    int             iRetVal = COM_OK;
    ST_COM_CONTROL *pstComCntrl;
    DWORD           dwThreadID;

    // Make sure we can get enough memory for buffers before opening device
    pstPrivate->pbBufrStart = malloc((size_t)pstPrivate->nRBufrSize);

    if (pstPrivate->pbBufrStart == NULL)
        {
        iRetVal = COM_NOT_ENOUGH_MEMORY;
        //* DeviceReportError(pstPrivate, SID_ERR_NOMEM, 0, TRUE);
        goto checkout;
        }

    pstPrivate->pbSndBufr = malloc((size_t) SIZE_OUTQ);
    if (pstPrivate->pbSndBufr == 0)
        {
        iRetVal = COM_NOT_ENOUGH_MEMORY;
        free(pstPrivate->pbBufrStart);
        pstPrivate->pbBufrStart = 0;
        goto checkout;
        }

    pstPrivate->pbBufrEnd = pstPrivate->pbBufrStart + pstPrivate->nRBufrSize;
    pstPrivate->pbReadEnd = pstPrivate->pbBufrStart;
    pstPrivate->pbComStart = pstPrivate->pbComEnd = pstPrivate->pbBufrStart;
    pstPrivate->fBufrEmpty = TRUE;

#if defined(DEBUG_CHARDUMP)
	if (!pfDbgR)
		pfDbgR = fopen("comreads.dbg", "wt");
	fprintf(pfDbgR, "Port opened, internal buffer = 0x%p to 0x%p\n",
			pstPrivate->pbBufrStart, pstPrivate->pbBufrEnd - 1);
	if (!pfDbgC)
		pfDbgC = fopen("comused.dbg", "wt");
	fprintf(pfDbgC, "Port opened, internal buffer = 0x%p to 0x%p\n",
			pstPrivate->pbBufrStart, pstPrivate->pbBufrEnd - 1);
#endif

    pstPrivate->dwSndOffset = 0;
    pstPrivate->dwBytesToSend = 0;

    if (dwMediaHdl)
        {
        pstPrivate->hWinComm = (HANDLE)dwMediaHdl;

        if (PortExtractSettings(pstPrivate) != COM_OK)
            iRetVal = COM_DEVICE_ERROR;
        }

    else
        {
        // Win32 internally maps ports COM1 to COM9 to
        // \\.\COMx. We need to add this for ports COMxx,
        // and for special com devices in the registry.
        //
        lstrcpy(szFullPortName, TEXT("\\\\.\\"));
        lstrcat(szFullPortName, pszPortName);
        pstPrivate->hWinComm = CreateFile(szFullPortName,
                   GENERIC_READ | GENERIC_WRITE,
                   0,
                   (LPSECURITY_ATTRIBUTES)0,
                   OPEN_EXISTING,
                   FILE_FLAG_OVERLAPPED,
                   (HANDLE)0);


        if (pstPrivate->hWinComm == INVALID_HANDLE_VALUE)
            {
            //* Figure out which errors to report specifically
            iRetVal = COM_NOT_FOUND;
            }
        }

    if (iRetVal == COM_OK)
        {
		OSVERSIONINFO osInfo;
		osInfo.dwOSVersionInfoSize = sizeof(osInfo);

		if (GetVersionEx(&osInfo))
			{	
			// Major bug in Win95 - If you call SetupComm() for a standard
			// comm handle (not one given to us by TAPI) the WriteFile
            // call fails and locks the system. - mrw:2/29/96
			//
			if (osInfo.dwPlatformId	== VER_PLATFORM_WIN32_NT)
				{
				if (SetupComm(pstPrivate->hWinComm, 8192, 8192) == FALSE)
					assert(0);
				}
			}
        }

    if (iRetVal == COM_OK)
        {
        iRetVal = PortConfigure(pstPrivate);
        }

    if (iRetVal == COM_OK)
        {
        pstComCntrl = (ST_COM_CONTROL *)pstPrivate->hCom;
        pstComCntrl->puchRBData =
            pstComCntrl->puchRBDataLimit =
            pstPrivate->pbBufrStart;

        pstPrivate->dwEventMask = EV_ERR | EV_RLSD;
        pstPrivate->fNotifyRcv = TRUE;
        pstPrivate->fBufrEmpty = TRUE;

        if (!SetCommMask(pstPrivate->hWinComm, pstPrivate->dwEventMask))
            iRetVal = COM_DEVICE_ERROR;

        // Clear error counts on new connection
        pstPrivate->nParityErrors = 0;
        pstPrivate->nFramingErrors = 0;
        pstPrivate->nOverrunErrors = 0;
        pstPrivate->nOverflowErrors = 0;

        // Start thread to handle Reading, Writing (& 'rithmetic) & events
        pstPrivate->fHaltThread = FALSE;
        DBG_THREAD("DBG_THREAD: Calling CreateThread\r\n",0,0,0,0,0);
        pstPrivate->hComstdThread = CreateThread((LPSECURITY_ATTRIBUTES)0,
                    2000, ComstdThread, pstPrivate, 0, &dwThreadID);

        if (pstPrivate->hComstdThread)
            {
            SetThreadPriority(pstPrivate->hComstdThread,
                    THREAD_PRIORITY_ABOVE_NORMAL);
                    //THREAD_PRIORITY_TIME_CRITICAL); // - mrw:7/8/96
            }

        DBG_THREAD("DBG_THREAD: CreateThread returned %08X\r\n",
            pstPrivate->hComstdThread,0,0,0,0);
        }

checkout:
    if (iRetVal != COM_OK)
        PortDeactivate(pstPrivate);

    return iRetVal;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: PortDeactivate
 *
 * DESCRIPTION:
 *  Deactivates and closes an open port
 *
 * ARGUMENTS:
 *  pstPrivate -- Driver data structure
 *
 * RETURNS:
 *  COM_OK
 */
int WINAPI PortDeactivate(ST_STDCOM *pstPrivate)
    {
    int iRetVal = COM_OK;

    if (pstPrivate->hComstdThread)
        {
        // Halt the thread by setting a flag for the thread to detect and then
        // forcing WaitCommEvent to return by changing the event mask
        DBG_THREAD("DBG_THREAD: Shutting down comstd thread\r\n", 0,0,0,0,0);
        pstPrivate->fHaltThread = TRUE;
        SetCommMask(pstPrivate->hWinComm, pstPrivate->dwEventMask);
        PurgeComm(pstPrivate->hWinComm,
            PURGE_TXABORT | PURGE_RXABORT);  // Abort any calls in progress

        // thread should exit now, it's handle will signal when it has exited
        WaitForSingleObject(pstPrivate->hComstdThread, 5000);
        CloseHandle(pstPrivate->hComstdThread);
        pstPrivate->hComstdThread = NULL;
        DBG_THREAD("DBG_THREAD: Comstd thread has shut down\r\n", 0,0,0,0,0);
        }

    if (pstPrivate->pbBufrStart)
        {
        free(pstPrivate->pbBufrStart);
        pstPrivate->pbBufrStart = NULL;
        }

    if (pstPrivate->pbSndBufr)
        {
        free(pstPrivate->pbSndBufr);
        pstPrivate->pbSndBufr = 0;
        }

    if (pstPrivate->hWinComm != INVALID_HANDLE_VALUE)
        {
        //* As of 2/9/94, this PurgeComm call caused the program to hang
        //   or reboot
        // PurgeComm(pstPrivate->hWinComm,
        //        PURGE_TXABORT | PURGE_RXABORT);  // Flush transmit queue

        CloseHandle(pstPrivate->hWinComm);
        }

    pstPrivate->hWinComm = INVALID_HANDLE_VALUE;

    return iRetVal;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: PortConfigure
 *
 * DESCRIPTION:
 *  Configures an open port with the current set of user settings
 *
 * ARGUMENTS:
 *  pstPrivate -- The driver data structure
 *
 * RETURNS:
 *  COM_OK if port is configured successfully
 *  COM_DEVICE_ERROR if API errors are encountered
 *  COM_DEVICE_INVALID_SETTING if some user settings are not valid
 */
int WINAPI PortConfigure(ST_STDCOM *pstPrivate)
    {
    int          iRetVal = COM_OK;
    unsigned     uOverrides = 0;
    DWORD        dwError;
    DWORD        dwStructSize;
    DCB         *pstDcb;
    COMMCONFIG   stCommConfig;
    COMMTIMEOUTS stCT;

    dwStructSize = sizeof(stCommConfig);
    stCommConfig.dwSize = sizeof(stCommConfig);

    if (!GetCommConfig(pstPrivate->hWinComm, &stCommConfig, &dwStructSize))
        {
        //* DeviceReportError(pstPrivate, SID_ERR_WINDRIVER, 0, TRUE);
        iRetVal = COM_DEVICE_ERROR;
        }
    else
        {
        pstDcb = &stCommConfig.dcb;
        ComstdSettingsToDCB(&pstPrivate->stWorkSettings, pstDcb);

        // Check for overrides
        ComQueryOverride(pstPrivate->hCom, &uOverrides);

        if (bittest(uOverrides, COM_OVERRIDE_8BIT))
            {
            pstDcb->ByteSize = 8;
            pstDcb->Parity = NOPARITY;
            }

        // If we need to receive all 256 chars., we need to override
        //   XON/XOFF during sending since it will strip XON & XOFF from
        //   the incoming stream if enabled
        if (bittest(uOverrides, COM_OVERRIDE_RCVALL))
            pstDcb->fOutX = 0;

        stCommConfig.dwSize = sizeof(stCommConfig);

        if (!SetCommConfig(pstPrivate->hWinComm, &stCommConfig,
            dwStructSize))
            {
            dwError = GetLastError();

            //* Use GetLastError to figure out what went wrong, but
            //*  docs don't specify which error to check for.

            // At this point SOME setting in the DCB is bad but there is
            // no way to find out which. Since the baud rate is a likely
            // candidate. Try reissuing the command with a common baud
            // rate to see if the problem goes away
            //
            pstDcb->BaudRate = 1200;

            if (!SetCommConfig(pstPrivate->hWinComm, &stCommConfig,
                sizeof(stCommConfig)))
                {
                // If its still no good them some other setting is bad
                //* DeviceReportError(pstPrivate, SID_ERR_BADSETTING, 0, TRUE);
                }
            else
                {
                // Changing baud rate to 1200 worked, so the user's baud
                // rate must be what the driver is refusing
                //* DeviceReportError(pstPrivate, SID_ERR_BADBAUD, 0, TRUE);
                }
            iRetVal = COM_DEVICE_INVALID_SETTING;
            }
        else
            {
            stCT.ReadIntervalTimeout = 10;
            stCT.ReadTotalTimeoutMultiplier = 0;
            stCT.ReadTotalTimeoutConstant = 0;
            stCT.WriteTotalTimeoutMultiplier = 0;
            stCT.WriteTotalTimeoutConstant = 5000;
            if (!SetCommTimeouts(pstPrivate->hWinComm, &stCT))
                {
                assert(FALSE);
                iRetVal = COM_DEVICE_INVALID_SETTING;
                }
            else
                {
                }
            }
        }
    return iRetVal;
    }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  PortExtactSettings
 *
 * DESCRIPTION:
 *  Extracts current Com settings from the Windows Com driver. This is
 *  needed when we are passed an existing Com handle by something like TAPI
 *
 * ARGUMENTS:
 *  pstPrivate -- The driver data structure
 *
 * RETURNS:
 *  COM_OK if port is configured successfully
 *  COM_DEVICE_ERROR if API errors are encountered
 */
int PortExtractSettings(ST_STDCOM *pstPrivate)
    {
    int        iRetVal;
    DWORD      dwError;
    DWORD      dwSize;
    COMMCONFIG stCommConfig;

    dwSize = sizeof(stCommConfig);
    if (!GetCommConfig(pstPrivate->hWinComm, &stCommConfig, &dwSize))
        {
        dwError = GetLastError();
        //* DeviceReportError(pstPrivate, SID_ERR_WINDRIVER, 0, TRUE);
        iRetVal = COM_DEVICE_ERROR;
        }
    else
        {
        // Unload appropriate values from DCB to our settings structure
        ComstdDCBToSettings(&stCommConfig.dcb, &pstPrivate->stWorkSettings);

        // Don't leave autodetect on if user has already set something
        // other than 8N1
        DBG_AD("DBG_AD: fAutoDetect = %d\r\n",
            pstPrivate->stWorkSettings.fAutoDetect, 0,0,0,0);
        if (pstPrivate->stWorkSettings.fAutoDetect)
            {
            if (pstPrivate->stWorkSettings.nDataBits != 8 ||
                    pstPrivate->stWorkSettings.nParity != NOPARITY ||
                    pstPrivate->stWorkSettings.nStopBits != ONESTOPBIT)
                {
                DBG_AD("DBG_AD: Turning fAutoDetect off due to non 8N1\r\n",
                    0,0,0,0,0);
                pstPrivate->stWorkSettings.fAutoDetect = FALSE;
                }
            }
        iRetVal = COM_OK;
        }
    return iRetVal;
    }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: PortConnected
 *
 * DESCRIPTION:
 *  Determines whether the driver is currently connected to a host system.
 *  In the case of this driver, the presence of the carrier signal determines
 *  when we are connected.
 *
 * ARGUMENTS:
 *  pstPrivate -- Our private data structure
 *
 * RETURNS:
 *  TRUE if carrier is present
 *  FALSE if carrier is off
 */
int WINAPI PortConnected(ST_STDCOM *pstPrivate)
    {
    int   iRetVal = FALSE;
    DWORD dwModemStat;

    if (GetCommModemStatus(pstPrivate->hWinComm, &dwModemStat))
        iRetVal = bittest(dwModemStat, MS_RLSD_ON);
    return iRetVal;
    }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: RcvRefill
 *
 * DESCRIPTION:
 *  Called when the receive buffer is empty to refill it. This routine
 *  should attempt to refill the buffer and return the first character.
 *  It is important that this function be implemented efficiently.
 *
 * ARGUMENTS:
 *  pstPrivate -- the driver data structure
 *
 * RETURNS:
 *  TRUE if data is put in the receive buffer
 *  FALSE if there is no new incoming data
 */
int WINAPI RcvRefill(ST_STDCOM *pstPrivate)
    {
    int fRetVal = FALSE;
    ST_COM_CONTROL *pstComCntrl;

    EnterCriticalSection(&pstPrivate->csect);

    pstPrivate->pbComStart = (pstPrivate->pbComEnd == pstPrivate->pbBufrEnd) ?
        pstPrivate->pbBufrStart : pstPrivate->pbComEnd;
    pstPrivate->pbComEnd = (pstPrivate->pbReadEnd >= pstPrivate->pbComStart) ?
        pstPrivate->pbReadEnd : pstPrivate->pbBufrEnd;
    DBG_READ("DBG_READ: Refill ComStart==%x, ComEnd==%x (ReadEnd==%x)\r\n",
        pstPrivate->pbComStart, pstPrivate->pbComEnd,
        pstPrivate->pbReadEnd, 0,0);
    if (pstPrivate->fBufrFull)
        {
        DBG_READ("DBG_READ: Refill Signalling EVENT_READ\r\n", 0,0,0,0,0);
        SetEvent(pstPrivate->ahEvent[EVENT_READ]);
        }
    if (pstPrivate->pbComStart == pstPrivate->pbComEnd)
        {
        DBG_READ("DBG_READ: Refill setting fBufrEmpty = TRUE\r\n", 0,0,0,0,0);
        pstPrivate->fBufrEmpty = TRUE;
        ComNotify(pstPrivate->hCom, NODATA);
        }
    else
        {
        pstComCntrl = (ST_COM_CONTROL *)pstPrivate->hCom;
        pstComCntrl->puchRBData = pstPrivate->pbComStart;
        pstComCntrl->puchRBDataLimit = pstPrivate->pbComEnd;

#if defined(DEBUG_CHARDUMP)
		{
        int iAvail;
        int iCnt;

        iAvail = (int)(pstComCntrl->puchRBDataLimit - pstComCntrl->puchRBData);
		fprintf(pfDbgC,
			"Consumed -- %d bytes 0x%p to 0x%p:",
				iAvail,
                pstComCntrl->puchRBData,
				pstComCntrl->puchRBDataLimit - 1);
		for (iCnt = 0; iCnt < iAvail; ++iCnt)
			{
			if ((iCnt % 16) == 0)
				fputs("\n", pfDbgC);
			fprintf(pfDbgC, "%02X ", pstComCntrl->puchRBData[iCnt]);
			}
		fputs("\n", pfDbgC);
		}
#endif

        // If this com driver were being used to make the connection, we
        //  would have to check here to see whether we were connected before
        //  we called AutoDetect. Since TAPI takes care of making the
        //  connection for this app, we can just start auto detecting
        //  whenever we get control

        if (pstPrivate->stWorkSettings.fAutoDetect)
            AutoDetectAnalyze(pstPrivate,
                (int)(pstPrivate->pbComEnd - pstPrivate->pbComStart),
                pstPrivate->pbComStart);
        fRetVal = TRUE;
        }

    LeaveCriticalSection(&pstPrivate->csect);
    return fRetVal;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: RcvClear
 *
 * DESCRIPTION:
 *  Clears the receiver of all received data.
 *
 * ARGUMENTS:
 *  hCom -- a comm handle returned by an earlier call to ComCreateHandle
 *
 * RETURNS:
 *  COM_OK if data is cleared
 *  COM_DEVICE_ERROR if Windows com device driver returns an error
 */
int WINAPI RcvClear(ST_STDCOM *pstPrivate)
    {
    int iRetVal = COM_OK;
    ST_COM_CONTROL *pstComCntrl = (ST_COM_CONTROL *)pstPrivate->hCom;

    EnterCriticalSection(&pstPrivate->csect);

    // Set buffer pointers to clear out any data we might have queued
    pstComCntrl->puchRBData = pstComCntrl->puchRBDataLimit =
        pstPrivate->pbBufrStart;
    pstPrivate->pbReadEnd = pstPrivate->pbBufrStart;
    pstPrivate->pbComStart = pstPrivate->pbComEnd = pstPrivate->pbBufrStart;

    if (!PurgeComm(pstPrivate->hWinComm, PURGE_RXCLEAR | PURGE_RXABORT))
        iRetVal = COM_DEVICE_ERROR;

    LeaveCriticalSection(&pstPrivate->csect);
    return iRetVal;
    }



//          Buffered send routines


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: SndBufrSend
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int WINAPI SndBufrSend(ST_STDCOM *pstPrivate, void *pvBufr, int  nSize)
    {
    int  iRetVal = COM_OK;
    DWORD dwBytesWritten;
    DWORD dwError;

    assert(pvBufr != (void *)0);
    assert(nSize <= SIZE_OUTQ);

    if (nSize > 0)
        {
        // If Auto Detection is on, we may need to manually alter the
        // parity of the output
        if (pstPrivate->stWorkSettings.fAutoDetect)
            AutoDetectOutput(pstPrivate, pvBufr, nSize);

        ComNotify(pstPrivate->hCom, SEND_STARTED);

		// We had a bug that caused the session to stop displaying new characters when
		// you used auto-connect to start typing to a modem on Win 95. I tracked it down
		// to this point in the code by moving a debug trace statement around. Put it
		// just before this EnterCriticalSection and the bug goes away. Put it just after
		// and the bug comes back. I discovered that replacing the DbgOutStr with a Sleep(0)
		// had the same effect. This is a cheap fix but seems to work. We may want to
		// spend the time to figure out exactly what is going on sometime in the future.
		//jkh 9/9/98
		Sleep(0);

        EnterCriticalSection(&pstPrivate->csect);

        assert(pstPrivate->dwBytesToSend == 0);
        assert(pstPrivate->dwSndOffset == 0);
		MemCopy(pstPrivate->pbSndBufr, (BYTE*) pvBufr, nSize);
        pstPrivate->dwBytesToSend = nSize;
        pstPrivate->dwSndOffset = 0;

        pstPrivate->stWriteOv.Offset = pstPrivate->stWriteOv.OffsetHigh = 0;
        pstPrivate->stWriteOv.hEvent = pstPrivate->ahEvent[EVENT_WRITE];

        DBG_WRITE("DBG_WRITE: %d WriteFile nSize==%d 0x%x\r\n", GetTickCount(),nSize,pstPrivate->hWinComm,0,0);
        // jmh:01-12-96 When the OVERLAPPED structure is passed to WriteFile,
        // there is character loss. Thorough investigation indicates a problem
        // within Win32 comm. Documentation says behavior is undefined when
        // this structure is not passed, but it works.
        if (WriteFile(pstPrivate->hWinComm, pstPrivate->pbSndBufr, (DWORD)nSize,
            &dwBytesWritten, &pstPrivate->stWriteOv)) // mrw:12/6/95 restored stWriteOv
            {
            assert(dwBytesWritten == (DWORD)nSize);
            DBG_WRITE("DBG_WRITE: %d WriteFile completed synchronously\r\n",GetTickCount(),0,0,0,0);
            LeaveCriticalSection(&pstPrivate->csect);
            ComNotify(pstPrivate->hCom, SEND_DONE);
            EnterCriticalSection(&pstPrivate->csect);
            pstPrivate->dwBytesToSend = 0;
            }
        else
            {
            dwError = GetLastError();
            if (dwError == ERROR_IO_PENDING)
                {
                pstPrivate->fSending = TRUE;
                }
            else
                {
                iRetVal = COM_FAILED;
                DBG_WRITE("DBG_WRITE: %d WriteFile failed %d 0x%x\r\n", GetTickCount(),dwError,pstPrivate->hWinComm,0,0);
                }
            }
        LeaveCriticalSection(&pstPrivate->csect);
        }

    return iRetVal;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: SndBufrIsBusy
 *
 * DESCRIPTION:
 *  Determines whether the driver is available to transmit a buffer of
 *  data or not.
 *
 * ARGUMENTS:
 *  pstPrivate -- address of com driver's data structure
 *
 * RETURNS:
 *  COM_OK   if data can be transmitted
 *  COM_BUSY if driver is still working on a previous buffer
 */
int WINAPI SndBufrIsBusy(ST_STDCOM *pstPrivate)
    {
    int  iRetVal = COM_OK;

    EnterCriticalSection(&pstPrivate->csect);

    if (pstPrivate->fBreakSignalOn || pstPrivate->fSending)
        {
        iRetVal = COM_BUSY;
        }

    LeaveCriticalSection(&pstPrivate->csect);

    return iRetVal;
    }



/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: SndBufrQuery
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int WINAPI SndBufrQuery(ST_STDCOM *pstPrivate,
    unsigned *pafStatus,
    long *plHandshakeDelay)
    {
    int     iRetVal = COM_OK;
    DWORD   dwErrors;
    COMSTAT stComStat;

    assert(pafStatus != NULL);

    *pafStatus = 0;

    //* temporary
    if (!SndBufrIsBusy(pstPrivate))
        {
        // If no send is in progress, return clear status
        *pafStatus = 0;
        if (plHandshakeDelay)
            *plHandshakeDelay = 0L;
        }
    else
        {
        if (ClearCommError(pstPrivate->hWinComm, &dwErrors, &stComStat))
            {
            if (stComStat.fXoffHold)
                bitset(*pafStatus, COMSB_WAIT_XON);
            if (stComStat.fCtsHold)
                bitset(*pafStatus, COMSB_WAIT_CTS);
            if (stComStat.fDsrHold)
                bitset(*pafStatus, COMSB_WAIT_DSR);
            if (stComStat.fRlsdHold)
                bitset(*pafStatus, COMSB_WAIT_DCD);
            if (stComStat.fXoffSent)
                bitset(*pafStatus, COMSB_WAIT_BUSY);

            if (*pafStatus && pstPrivate->lSndStuck == -1L)
                pstPrivate->lSndStuck = (long)startinterval();

            if (plHandshakeDelay)
                *plHandshakeDelay =
                (pstPrivate->lSndStuck == -1L ?
                0L : (long)interval(pstPrivate->lSndStuck));
            }
        }

    return iRetVal;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: SndBufrClear
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int WINAPI SndBufrClear(ST_STDCOM *pstPrivate)
    {
    int iRetVal = COM_OK;

    EnterCriticalSection(&pstPrivate->csect);
    if (SndBufrIsBusy(pstPrivate))
        {
        if (!PurgeComm(pstPrivate->hWinComm, PURGE_TXCLEAR | PURGE_TXABORT))
            iRetVal = COM_DEVICE_ERROR;
        }
    LeaveCriticalSection(&pstPrivate->csect);

    return iRetVal;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComstdThread
 *
 * DESCRIPTION:
 *	This thread services three events, reads, writes and rithmatic, uh
 *	no, I mean comm events.  It uses overlapped I/O to accomplish this
 *	task which simplifies the task since thread contention between the
 *	different events is eliminated.
 *
 * ARGUMENTS:
 *	pvData - pointer to private comm handle
 *
 * RETURNS:
 *	Eventually
 *
 */
DWORD WINAPI ComstdThread(void *pvData)
    {
    ST_STDCOM *pstPrivate =  (ST_STDCOM *)pvData;
    DWORD      dwResult;
    DWORD      dwError;
    DWORD      dwBytes;
    DWORD      dwComEvent = 0;
    long       lBytesRead;
    long       lReadSize;
    BYTE      *pbReadFrom;
    OVERLAPPED stReadOv;
    OVERLAPPED stEventOv;
    COMSTAT    stComStat;
#if defined(DEBUG_CHARDUMP)
	int        iCnt;
#endif

    DBG_THREAD("DBG_THREAD: ComstdThread starting\r\n",0,0,0,0,0);
    EnterCriticalSection(&pstPrivate->csect);

    // Set Read event to signaled to get the first Read operation going
    //
    pstPrivate->fBufrFull = TRUE;
    SetEvent(pstPrivate->ahEvent[EVENT_READ]);

    // Set ComEvent event to signaled to to get the first WaitCommEvent
    // started
    //
    SetEvent(pstPrivate->ahEvent[EVENT_COMEVENT]);

    // Clear any set state left by a previous connection.
    //
    ResetEvent(pstPrivate->ahEvent[EVENT_WRITE]);

    for ( ; ; )
        {
        LeaveCriticalSection(&pstPrivate->csect);
        DBG_THREAD("DBG_THREAD: Waiting\r\n", 0,0,0,0,0);

        dwResult = WaitForMultipleObjects(EVENT_COUNT, pstPrivate->ahEvent,
            FALSE, INFINITE);

        DBG_THREAD("DBG_THREAD: WaitForMultipleObjects returned %d\r\n",
            dwResult,0,0,0,0);

        EnterCriticalSection(&pstPrivate->csect);

        // To get this thread to exit, the deactivate routine forces a
        // fake com event by calling SetCommMask
        //
        if (pstPrivate->fHaltThread)
            {
            LeaveCriticalSection(&pstPrivate->csect);
            DBG_THREAD("DBG_THREAD: ComStd exiting thread\r\n",0,0,0,0,0);
            ExitThread(0);
            }

        switch (dwResult)
            {
        case WAIT_OBJECT_0 + EVENT_READ:
            if (pstPrivate->fBufrFull)
                {
                DBG_READ("DBG_READ: Thread -- fBufrFull = FALSE\r\n",
                    0,0,0,0,0);

                pstPrivate->fBufrFull = FALSE;
                }
            else
                {
                if (GetOverlappedResult(pstPrivate->hWinComm, &stReadOv,
                    (DWORD *)&lBytesRead, FALSE))
                    {

                    pstPrivate->pbReadEnd += lBytesRead;
#if defined(DEBUG_CHARDUMP)
					if (lBytesRead > 0)
						{
						fprintf(pfDbgR,
							"Overlapped Read -- %d bytes 0x%p to 0x%p:",
								lBytesRead, pbReadFrom,
								pstPrivate->pbReadEnd - 1);
						for (iCnt = 0; iCnt < lBytesRead; ++iCnt)
							{
							if ((iCnt % 16) == 0)
								fputs("\n", pfDbgR);
							fprintf(pfDbgR, "%02X ", pbReadFrom[iCnt]);
							}
						fputs("\n", pfDbgR);
						}
#endif

                    if (pstPrivate->pbReadEnd >= pstPrivate->pbBufrEnd)
                        pstPrivate->pbReadEnd = pstPrivate->pbBufrStart;

                    DBG_READ("DBG_READ: Thread -- got %ld, ReadEnd==%x\r\n",
                        lBytesRead, pstPrivate->pbReadEnd,0,0,0);

                    if (pstPrivate->fBufrEmpty)
                        {
                        DBG_READ("DBG_READ: Thread -- fBufrEmpty = FALSE\r\n",
                            0,0,0,0,0);

                        pstPrivate->fBufrEmpty = FALSE;
                        LeaveCriticalSection(&pstPrivate->csect);
                        ComNotify(pstPrivate->hCom, DATA_RECEIVED);
                        EnterCriticalSection(&pstPrivate->csect);
                        }
                    }

                else
					{
					switch (GetLastError())
						{
					case ERROR_OPERATION_ABORTED:
						// Operations can be aborted by calls to PurgeComm()
						// Allow setup for another read request.
						// mrw:12/14/95
						//
						break;

					default:
						// Com is failing for some reason.  Exit thread
						// so that we don't tie-up resources.
						//
	                    DBG_EVENTS("DBG_EVENTS: GetOverlappedResult "
                            "failed!\r\n",0,0,0,0,0);

						LeaveCriticalSection(&pstPrivate->csect);
						ExitThread(0);
						}
					}
                }

            // Do reads until we fill the buffer or we get an overlapped read
			//
            for ( ; ; )
                {
				// Check for wrap around in circular buffer
				//
                pbReadFrom = (pstPrivate->pbReadEnd >= pstPrivate->pbBufrEnd) ?
                    pstPrivate->pbBufrStart : pstPrivate->pbReadEnd;

#if 0   // mrw:10/7/96 - enabled shiva fix for NT 4.0 Service Pack
        // Enabled for NT 4.0 release. Per Microsoft, leave this bug
        // in, so US and international versions are identical. It was
        // found between US and international releases.
        //
                // This was causing bad packets in Zmodem transfers when
                // using Shiva's LanRover, which appeared at baud rates
                // of 57600 or higher, and using TCP/IP to connect to the
                // LanRover. lReadSize in this code would not leave an
                // unused byte at the end of the buffer if pbComStart was
                // pointing to the beginning of the buffer.
                // - jmh 07-31-96
                lReadSize = (pbReadFrom < pstPrivate->pbComStart) ?
                    (pstPrivate->pbComStart - pbReadFrom - 1) :
                    (pstPrivate->pbBufrEnd - pbReadFrom);
#else
                // Determine the extent to which we're allowed to fill the
                // buffer. pbComStart points to the start of where the buffer
                // is "reserved", waiting to be emptied from. We make sure we
                // leave the byte before pbComStart empty. - jmh 07-31-96
                //
                if (pbReadFrom < pstPrivate->pbComStart)
                    lReadSize = (long)(pstPrivate->pbComStart - pbReadFrom - 1);
                else
                    {
                    lReadSize = (long)(pstPrivate->pbBufrEnd - pbReadFrom);
                    // The circular buffer code was written so that the address
                    // pointed to by pbBufrEnd is equated with pbBufrStart. We
                    // also need to make sure that if we've just calculated
                    // that we can read to the end of the buffer (aka the
                    // *start* of the buffer), and pbComStart is pointing to
                    // the start of the buffer, there's still an empty byte
                    // before pbComStart. - jmh 07-31-96
                    //
                    if (pstPrivate->pbComStart == pstPrivate->pbBufrStart)
                        lReadSize -= 1;
                    }
#endif

                if (lReadSize > MAX_READSIZE)
                    lReadSize = MAX_READSIZE;

                if (!lReadSize)
                    {
                    DBG_READ("DBG_READ: Thread -- fBufrFull = TRUE, "
                        "unsignalling EVENT_READ\r\n",0,0,0,0,0);

                    pstPrivate->fBufrFull = TRUE;
                    ResetEvent(pstPrivate->ahEvent[EVENT_READ]);
                    break;
                    }
                else
                    {
                    // Set up to do an overlapped read. From what I can make
                    // of the documenation, this may or may not complete
                    // immediately. So, to be safe, I will code it to expect
                    // either result.
                    //
                    stReadOv.Offset = stReadOv.OffsetHigh = 0;
                    stReadOv.hEvent = pstPrivate->ahEvent[EVENT_READ];

                    DBG_READ("DBG_READ: Thread -- ReadFile started, "
                        "ReadFrom==%x, ReadSize==%ld\r\n",
                        pbReadFrom, lReadSize, 0,0,0);

					// ReadFile resets the read event semaphore
					//
                    if (ReadFile(pstPrivate->hWinComm, pbReadFrom,
                        (DWORD)lReadSize, (DWORD *)&lBytesRead,
                        &stReadOv))
                        {
                        pstPrivate->pbReadEnd += lBytesRead;

#if defined(DEBUG_CHARDUMP)
						fprintf(pfDbgR,
							"Overlapped Read -- %d bytes 0x%p to 0x%p:",
								lBytesRead, pbReadFrom,
								pstPrivate->pbReadEnd - 1);
						for (iCnt = 0; iCnt < lBytesRead; ++iCnt)
							{
							if ((iCnt % 16) == 0)
								fputs("\n", pfDbgR);
							fprintf(pfDbgR, "%02X ", pbReadFrom[iCnt]);
							}
						fputs("\n", pfDbgR);
#endif

                        if (pstPrivate->pbReadEnd >= pstPrivate->pbBufrEnd)
                            pstPrivate->pbReadEnd = pstPrivate->pbBufrStart;

                        DBG_READ("DBG_READ: Thread -- ReadFile completed "
                            "synchronously, lBytesRead==%ld, ReadEnd==%x\r\n",
                            lBytesRead, pstPrivate->pbReadEnd,0,0,0);

                        if (pstPrivate->fBufrEmpty)
                            {
                            DBG_READ("DBG_READ: Thread -- fBufrEmpty = "
                                "FALSE\r\n", 0,0,0,0,0);

                            pstPrivate->fBufrEmpty = FALSE;
                            LeaveCriticalSection(&pstPrivate->csect);
                            ComNotify(pstPrivate->hCom, DATA_RECEIVED);
                            EnterCriticalSection(&pstPrivate->csect);
                            }
                        }

					else
						{
						switch (GetLastError())
							{
						case ERROR_IO_PENDING:
							break;

						case ERROR_OPERATION_ABORTED:
							// PurgeComm can do this.  Setup for another read.
							// mrw:12/14/95
							// But clear errors or the read may fail from
                            // now to eternity! We're in an infinite for-loop,
                            // after all. jmh:06-12-96
                            ClearCommError(pstPrivate->hWinComm, &dwError, &stComStat);
							continue;

						default:
							// Com is failing for some reason.  Exit thread
							// so that we don't tie-up resources.
							//
		                    DBG_READ("DBG_READ: ReadFile failed!\r\n",
                                0,0,0,0,0);

							LeaveCriticalSection(&pstPrivate->csect);
							ExitThread(0);
							}

						break;  // Come back when event signals
						}
                    }
                }
            break;

        case WAIT_OBJECT_0 + EVENT_WRITE:
            if (GetOverlappedResult(pstPrivate->hWinComm,
                &pstPrivate->stWriteOv, &dwBytes, FALSE) == FALSE)
                {
                dwError = GetLastError();
                DBG_WRITE("DBG_WRITE: %d Overlapped WriteFile failed: errno=%d\n",
                    GetTickCount(), dwError, 0, 0, 0);
                }
            else if (dwBytes < pstPrivate->dwBytesToSend && dwBytes != 0)
                {
                // ResetEvent(pstPrivate->ahEvent[EVENT_WRITE]);

                DBG_WRITE("DBG_WRITE: %d Write result -- dwBytes==%d\r\n",
                    GetTickCount(),dwBytes,0,0,0);

                // There's more to write. Seems kinda silly, but WriteFile
                // will return a success code, and dwBytes will show
                // there's still stuff to write. So we make another call
                // to WriteFile for what's remaining. Perhaps the write
                // timeout is too short. This happens for slower baud rates
                //
                pstPrivate->dwBytesToSend -= dwBytes;
                pstPrivate->dwSndOffset += dwBytes;

                pstPrivate->stWriteOv.Offset = pstPrivate->stWriteOv.OffsetHigh = 0;
                pstPrivate->stWriteOv.hEvent = pstPrivate->ahEvent[EVENT_WRITE];

                DBG_WRITE("DBG_WRITE: %d WriteFile(2) nSize==%d 0x%x\r\n",
                    GetTickCount(), pstPrivate->dwBytesToSend, pstPrivate->hWinComm, 0, 0);
                if (WriteFile(pstPrivate->hWinComm,
                    &pstPrivate->pbSndBufr[pstPrivate->dwSndOffset],
                    pstPrivate->dwBytesToSend,
                    &dwBytes, &pstPrivate->stWriteOv))
                    {
                    assert(dwBytes == pstPrivate->dwBytesToSend);
                    DBG_WRITE("DBG_WRITE: %d WriteFile(2) completed synchronously\r\n", GetTickCount(),0,0,0,0);
                    }
                else
                    {
                    dwError = GetLastError();
                    if (dwError == ERROR_IO_PENDING)
                        {
                        break;  // This is what we expect
                        }
                    else
                        {
                        DBG_WRITE("DBG_WRITE: %d WriteFile(2) failed %d 0x%x\r\n", GetTickCount(),dwError,pstPrivate->hWinComm,0,0);
                        }
                    }
                }
            else
                {
                // The write semaphore must be reset after the call to
                // GetOverlappedResult, because it checks the semaphore
                // to see if there's an outstanding write call. jmh 01-10-96
                //
                ResetEvent(pstPrivate->ahEvent[EVENT_WRITE]);
                }

            DBG_WRITE("DBG_WRITE: %d Write result -- dwBytes==%d\r\n",
                GetTickCount(),dwBytes,0,0,0);

            pstPrivate->dwBytesToSend = 0;
            pstPrivate->dwSndOffset = 0;

            pstPrivate->fSending = FALSE;
            LeaveCriticalSection(&pstPrivate->csect);
            ComNotify(pstPrivate->hCom, SEND_DONE);
            EnterCriticalSection(&pstPrivate->csect);
            break;

        case WAIT_OBJECT_0 + EVENT_COMEVENT:
            // WaitCommEvent is returning an event flag
            //
            ResetEvent(pstPrivate->ahEvent[EVENT_COMEVENT]);

            switch (dwComEvent)
                {
            case EV_ERR:
                ClearCommError(pstPrivate->hWinComm, &dwError, &stComStat);

                DBG_EVENTS("DBG_EVENTS: EV_ERR dwError==%x\r\n",
                    dwError,0,0,0,0);

                //* need code here to record errors, handle HHS stuck etc.
                break;

            case EV_RLSD: // receive-line-signal-detect changed state.
                LeaveCriticalSection(&pstPrivate->csect);
                ComNotify(pstPrivate->hCom, CONNECT);
                EnterCriticalSection(&pstPrivate->csect);
                DBG_EVENTS("DBG_EVENTS: EV_RLSD\r\n", 0,0,0,0,0);
                break;

            default:
                DBG_EVENTS("DBG_EVENTS: EV_??? (dwComEvent==%x)\r\n",
                    dwComEvent,0,0,0,0);
                break;
                }

            // Start up another overlapped WaitCommEvent to get the
            // next event
            //
            stEventOv.Offset = stEventOv.OffsetHigh = (DWORD)0;
            stEventOv.hEvent = pstPrivate->ahEvent[EVENT_COMEVENT];

            if (WaitCommEvent(pstPrivate->hWinComm, &dwComEvent, &stEventOv))
                {
                // Call completed synchronously, re-signal our event object
                //
                DBG_EVENTS("DBG_EVENTS: WaitCommEvent completed "
                    "synchronously\r\n",0,0,0,0,0);

                SetEvent(pstPrivate->ahEvent[EVENT_COMEVENT]);
                }

            else
                {
				switch (GetLastError())
					{
				case ERROR_IO_PENDING:
                    break;

				case ERROR_OPERATION_ABORTED:
                    // Not sure this can happen but we'll code it like
                    // the read. - mrw:12/14/95
                    //
                    DBG_EVENTS("DBG_EVENTS: WaitCommEvent - "
                        "ERROR_OPERATION_ABORTED\r\n",0,0,0,0,0);

                    SetEvent(pstPrivate->ahEvent[EVENT_COMEVENT]);
					break;

				default:
					// Com is failing for some reason.  Exit thread
					// so that we don't tie-up resources.
					//
                    DBG_EVENTS("DBG_EVENTS: WaitCommEvent failed!\r\n",
                        0,0,0,0,0);

					LeaveCriticalSection(&pstPrivate->csect);
					ExitThread(0);
					}
                }
            break;

        default:
            break;
            }
        }

    return (DWORD)0;
    }


/* --- AUTO DETECT ROUTINES --- */

static int Nibble[] = {0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0}; // 1=odd, 0=even
#define OddBits(b) (Nibble[(b) / 16] ^ Nibble[(b) % 16])

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  AutoDetectAnalyze
 *
 * DESCRIPTION:
 *  Analyzes incoming data to determine the char size, parity type and
 *  stop bits
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
void AutoDetectAnalyze(ST_STDCOM *pstPrivate, int nBytes, char *pchBufr)
    {
    char *pchScan = pchBufr;
    char *pszMsg;
    int fForceTo7Bits = FALSE;
    int iCnt = nBytes;

    if (!pstPrivate->fADRunning)
        {
        AutoDetectStart(pstPrivate);

        // This was a temporary fix I used while debugging a 7E1 problem that I decided
        // to leave in because it may help in some situations and shouldn't hurt.
        // In my case, when a GVC Fax 11400 V.42bis/MNP5 modem was installed as
        // "Standard Modem", an initial 8N1 CRLF from the modem negotiation got through
        // even when connecting to a 7E1 host. This made auto detect decide the whole
        // connection was 8N1. I fixed it with this little patch. Once I reinstalled the
        // modem as itself, it worked without the patch. This is not a rigourous fix
        // because there is no guarantee that there will be only two extraneous characters
        // or that they will be read all by themselves -- but this will fix the problems
        // in some typical cases and will do no harm.   jkh 9/9/98
        if (iCnt <= 2)
                return;
        }

    if (pstPrivate->nFramingErrors > 0)
        {
        DBG_AD("DBG_AD: Got Framing Errors: shutting down\r\n", 0,0,0,0,0);
        AutoDetectStop(pstPrivate);
        // MessageBox(NULL,
        //            "Would use Wizard code here. Either wrong baud rate "
        //            "is set or unusual settings have been encountered. "
        //            "Finished code may be able to handle some cases included here.",
        //             "Auto Detection Wizard", MB_OK);
        return;
        }

    pstPrivate->nADTotal += iCnt;

    // for each byte, determine whether the lower 7 bits contain an odd
    //  number of 1 bits, then determine whether the byte would be a valid
    //  7e1 character.
    while (iCnt--)
        {
        if (OddBits(*pchScan & 0x7F))
            ++pstPrivate->nADMix;
        if (OddBits(*pchScan))
            ++pstPrivate->nAD7o1;
        if (*pchScan & 0x80)
            ++pstPrivate->nADHighBits;
        ++pchScan;
        }

    // See whether we can make any decision with what we've got
    if (pstPrivate->nADMix > 0 && pstPrivate->nADMix < pstPrivate->nADTotal)
        {
        // We now have both kinds of characters: those with an even and
        //  an odd number of bits in the lower 7 bits - so we can make
        //  a guess.
        if (pstPrivate->nAD7o1 == pstPrivate->nADTotal)
            pstPrivate->nADBestGuess = AD_7O1;
        else if (pstPrivate->nAD7o1 == 0)
            pstPrivate->nADBestGuess = AD_7E1;
        else
            pstPrivate->nADBestGuess = AD_8N1;
        }

    DBG_AD("DBG_AD: Cnt=%3d, Mix=%3d, 7o1=%3d, HB=%3d BG=%d\r\n",
        pstPrivate->nADTotal, pstPrivate->nADMix,
        pstPrivate->nAD7o1,   pstPrivate->nADHighBits,
        pstPrivate->nADBestGuess);

    // See whether we've checked a sufficient sample to determine settings
    if (pstPrivate->nADBestGuess != AD_8N1 &&
        (pstPrivate->nADTotal < MIN_AD_TOTAL ||
         pstPrivate->nADMix < MIN_AD_MIX ||
         (pstPrivate->nADTotal - pstPrivate->nADMix) < MIN_AD_MIX))
        {
        // Data sample is insufficient to draw a conclusion.
        // For now, let the data display as 7-bit data and wait for more
        fForceTo7Bits = TRUE;
        }
    else
        {
        // We have enough data to make a decision
        if (pstPrivate->nAD7o1 == 0)
            {
            // Data is 7-even-1
            pstPrivate->stWorkSettings.nDataBits = 7;
            pstPrivate->stWorkSettings.nParity = EVENPARITY;
            fForceTo7Bits = TRUE;
            pstPrivate->fADReconfigure = TRUE;
            pszMsg = "Establishing settings of 7-Even-1";
            }
        else if (pstPrivate->nAD7o1 == pstPrivate->nADTotal)
            {
            // Data is 7-odd-1
            pstPrivate->stWorkSettings.nDataBits = 7;
            pstPrivate->stWorkSettings.nParity = ODDPARITY;
            fForceTo7Bits = TRUE;
            pstPrivate->fADReconfigure = TRUE;
            pszMsg = "Establishing settings of 7-Odd-1";
            }
        else
            {
            // Data is most likely 8-none-1. But if the high bit was
            //  set on all the received data, it may have been 7-mark-1 or
            //  some other odd setting
            pstPrivate->stWorkSettings.nDataBits = 8;
            pstPrivate->stWorkSettings.nParity = NOPARITY;
            if (pstPrivate->nADHighBits == pstPrivate->nADTotal)
                pszMsg = "Settings are either 8-none-1 or something quite "
                "odd like 7-mark-one. A wizard would pop up here"
                "asking the user if the data looked correct and"
                "offering suggestions if it did not.";
            else
                pszMsg = "Establishing settings of 8-none-1";
            }

        // Decision has been made, so turn auto detect off
        DBG_AD("DBG_AD: %s\r\n", pszMsg, 0,0,0,0);
        AutoDetectStop(pstPrivate);
        if (pstPrivate->fADReconfigure)
            {
            DBG_AD("DBG_AD: Reconfiguring port\r\n", 0,0,0,0,0);
            PortConfigure(pstPrivate);
            }
        // MessageBox(NULL, pszMsg, "Auto Detection Done", MB_OK);
        }

    if (fForceTo7Bits)
        {
        while (nBytes--)
            {
            *pchBufr = (char)(*pchBufr & 0x7F);
            ++pchBufr;
            }
        }
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  AutoDetectOutput
 *
 * DESCRIPTION:
 *  Checks state of auto detection and alters outgoing characters to
 *  reflect the best guess of their parity status.
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
void AutoDetectOutput(ST_STDCOM *pstPrivate, void *pvBufr, int nSize)
    {
    char *pch  = (char *)pvBufr;

    if (!pstPrivate->fADRunning)
        AutoDetectStart(pstPrivate);

    switch (pstPrivate->nADBestGuess)
        {
    case AD_8N1:
        // Do nothing
        break;

    case AD_7E1:
        // Make output look like 7e1
        DBG_AD("DBG_AD: Converting %d output char(s) to 7E1\r\n",
            nSize, 0,0,0,0);
        while (nSize--)
            {
            if (OddBits(*pch & 0x7F))
                *pch |= 0x80;
            ++pch;
            }
        break;

    case AD_7O1:
        // Make output look like 7o1
        DBG_AD("DBG_AD: Converting %d output char(s) to 7O1\r\n",
            nSize, 0,0,0,0);
        while (nSize--)
            {
            if (!OddBits(*pch & 0x7F))
                *pch |= 0x80;
            ++pch;
            }
        break;

    case AD_DONT_KNOW:
        // As long as the same single character is being sent
        //  out repeatedly, toggle the parity bit every other time

        if (nSize != 1)
            {
            pstPrivate->chADLastChar = '\0';
            pstPrivate->fADToggleParity = FALSE;
            }
        else
            {
            if (*pch != pstPrivate->chADLastChar)
                {
                pstPrivate->chADLastChar = *pch;
                pstPrivate->fADToggleParity = FALSE;
                }
            else
                {
                if (pstPrivate->fADToggleParity)
                    *pch = (*pch ^ (char)0x80);
                pstPrivate->fADToggleParity = !pstPrivate->fADToggleParity;
                }
            }
        break;

    default:
        assert(FALSE);
        break;
        }
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  AutoDetectStart
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
void AutoDetectStart(ST_STDCOM *pstPrivate)
    {
    DBG_AD("DBG_AD: AutoDetectStart\r\n", 0,0,0,0,0);
    pstPrivate->nADTotal = 0;
    pstPrivate->nADMix = 0;
    pstPrivate->nAD7o1 = 0;
    pstPrivate->nADHighBits = 0;
    pstPrivate->nADBestGuess = AD_DONT_KNOW;
    pstPrivate->fADRunning = TRUE;
    pstPrivate->chADLastChar = '\0';
    pstPrivate->fADToggleParity = FALSE;
    pstPrivate->fADReconfigure = FALSE;
    pstPrivate->nFramingErrors = 0;
    }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  AutoDetectStop
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
void AutoDetectStop(ST_STDCOM *pstPrivate)
    {
    HSESSION hSession;

    DBG_AD("DBG_AD: AutoDetectStop\r\n", 0,0,0,0,0);
    pstPrivate->stWorkSettings.fAutoDetect = FALSE;
    pstPrivate->fADRunning = FALSE;

    ComGetSession(pstPrivate->hCom, &hSession);

    PostMessage(sessQueryHwndStatusbar(hSession),
        SBR_NTFY_REFRESH, (WPARAM)SBR_COM_PART_NO, 0);
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  ComstdGetAutoDetectResults
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
int ComstdGetAutoDetectResults(void *pvData, BYTE *bByteSize,
    BYTE *bParity, BYTE *bStopBits)
    {
    ST_STDCOM *pstPrivate =  (ST_STDCOM *)pvData;

    assert(bByteSize);
    assert(bParity);
    assert(bStopBits);

    if (pstPrivate->fADReconfigure)
        {
        *bByteSize = (BYTE)pstPrivate->stWorkSettings.nDataBits;
        *bParity   = (BYTE)pstPrivate->stWorkSettings.nParity;
        *bStopBits = (BYTE)pstPrivate->stWorkSettings.nStopBits;
        }
    DBG_AD("DBG_AD: ComstdGetAutoDetectResults returning %d\r\n",
        pstPrivate->fADReconfigure, 0,0,0,0);
    DBG_AD("   (bits = %d, parity = %d, stops = %d)\r\n",
        *bByteSize, *bParity, *bStopBits, 0,0);
    return pstPrivate->fADReconfigure;
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      ComstdSettingsToDCB
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
static void ComstdSettingsToDCB(ST_STDCOM_SETTINGS *pstSettings, DCB *pstDcb)
    {
    unsigned         afHandshake;

    afHandshake = pstSettings->afHandshake;

    // fill in device control block
    pstDcb->BaudRate = (DWORD)pstSettings->lBaud;
    pstDcb->fBinary = 1;
    pstDcb->fParity = 1;
    pstDcb->fOutxCtsFlow = (BYTE)((bittest(afHandshake, HANDSHAKE_SND_CTS)) ? 1 : 0);
    pstDcb->fOutxDsrFlow = (BYTE)(bittest(afHandshake, HANDSHAKE_SND_DSR) ? 1 : 0);
    pstDcb->fDtrControl = bittest(afHandshake, HANDSHAKE_RCV_DTR) ?
        DTR_CONTROL_HANDSHAKE : DTR_CONTROL_ENABLE;
    pstDcb->fDsrSensitivity = 0;
    pstDcb->fTXContinueOnXoff = TRUE;
    pstDcb->fOutX = (BYTE)(bittest(afHandshake, HANDSHAKE_SND_X) ? 1 :0);
    pstDcb->fInX =  (BYTE)(bittest(afHandshake, HANDSHAKE_RCV_X) ? 1 :0);
    pstDcb->fErrorChar = 0;
    pstDcb->fNull = 0;
    pstDcb->fRtsControl = bittest(afHandshake, HANDSHAKE_RCV_RTS) ?
        RTS_CONTROL_HANDSHAKE : RTS_CONTROL_ENABLE;
    pstDcb->fAbortOnError = 1;      // so we can count all errors
    pstDcb->XonLim = 80;
    pstDcb->XoffLim = 200;
    pstDcb->ByteSize = (BYTE)pstSettings->nDataBits;
    pstDcb->Parity   = (BYTE)pstSettings->nParity;
    pstDcb->StopBits = (BYTE)pstSettings->nStopBits;
    pstDcb->XonChar = pstSettings->chXON;
    pstDcb->XoffChar = pstSettings->chXOFF;

    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *      ComstdDCBToSettings
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
static void ComstdDCBToSettings(DCB *pstDcb, ST_STDCOM_SETTINGS *pstSettings)
    {
    pstSettings->lBaud = (long)pstDcb->BaudRate;
    pstSettings->afHandshake = 0;
    if (pstDcb->fOutxCtsFlow)
        bitset(pstSettings->afHandshake, HANDSHAKE_SND_CTS);
    if (pstDcb->fOutxDsrFlow)
        bitset(pstSettings->afHandshake, HANDSHAKE_SND_DSR);
    if (pstDcb->fDtrControl == DTR_CONTROL_HANDSHAKE)
        bitset(pstSettings->afHandshake, HANDSHAKE_RCV_DTR);
    if (pstDcb->fOutX)
        bitset(pstSettings->afHandshake, HANDSHAKE_SND_X);
    if (pstDcb->fInX)
        bitset(pstSettings->afHandshake, HANDSHAKE_RCV_X);
    if (pstDcb->fRtsControl == RTS_CONTROL_HANDSHAKE)
        bitset(pstSettings->afHandshake, HANDSHAKE_RCV_RTS);
    pstSettings->nDataBits = pstDcb->ByteSize;
    pstSettings->nParity = pstDcb->Parity;
    pstSettings->nStopBits = pstDcb->StopBits;
    pstSettings->chXON = pstDcb->XonChar;
    pstSettings->chXOFF = pstDcb->XoffChar;
    }

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: DeviceBreakTimerProc
 *
 * DESCRIPTION:
 *  Called when the break timer goes off. A timer is started whenever we
 *  set the break signal on. It goes off after the break signal duration.
 *  This function clears the break signal and destroys the timer
 *
 * ARGUMENTS:
 *  dwData  -- A value stored when the timer is created. Contains pstPrivate
 *
 * RETURNS:
 *
 */
static void DeviceBreakTimerProc(void *pvData, long ulSince)
    {
    ST_STDCOM *pstPrivate = (ST_STDCOM *)pvData;

    TimerDestroy(&pstPrivate->hTmrBreak);       // this is a one-shot op
    ClearCommBreak(pstPrivate->hWinComm);    // have Win comm driver do it
    pstPrivate->fBreakSignalOn = FALSE;
    }

#if 0
void StdcomRecordErrors(ST_STDCOM *pstPrivate, int iErrorBits)
    {
    if (bittest(iErrorBits, CE_FRAME | CE_OVERRUN | CE_RXOVER | CE_RXPARITY))
        {
        if (bittest(iErrorBits, CE_FRAME))
            ++pstPrivate->nFramingErrors;

        if (bittest(iErrorBits, CE_OVERRUN))
            ++pstPrivate->nOverrunErrors;

        if (bittest(iErrorBits, CE_RXOVER))
            ++pstPrivate->nOverflowErrors;

        if (bittest(iErrorBits, CE_RXPARITY))
            ++pstPrivate->nParityErrors;
        }
    }


/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: DeviceReportError
 *
 * DESCRIPTION:
 *
 *
 * ARGUMENTS:
 *
 *
 * RETURNS:
 *
 */
void DeviceReportError(ST_STDCOM *pstPrivate, UINT uiStringID,
    LPSTR pszOptInfo, BOOL fFirstOnly)
    {
    CHAR szFmtString[250];
    CHAR szErrString[250];

    if (LoadString(hinstDLL, uiStringID, szFmtString, sizeof(szFmtString) / sizeof(TCHAR)) > 0)
        {
        wsprintf(szErrString, szFmtString, pszOptInfo);
        ComReportError(pstPrivate->hCom, 0, szErrString, fFirstOnly);
        }
    }


#endif
