/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/comm.c
 * PURPOSE:         Comm functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  modified from WINE [ Onno Hovers, (onno@stack.urc.tue.nl) ]
 *					Robert Dickenson (robd@mok.lvcom.com)
 *					Saveliy Tretiakov (saveliyt@mail.ru)
 *					Dmitry Philippov (shedon@mail.ru)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  RDD (30/09/2002) implemented many function bodies to call serial driver.
 *                  KJK (11/02/2003) implemented BuildCommDCB & BuildCommDCBAndTimeouts
 *                  ST  (21/03/2005) implemented GetCommProperties
 *                  ST  (24/03/2005) implemented ClearCommError. Corrected many functions.
 *                  ST  (05/04/2005) implemented CommConfigDialog
 *                  DP  (11/06/2005) implemented GetCommConfig
 *                  DP  (12/06/2005) implemented SetCommConfig
 *
 */

#include <k32.h>
#undef SERIAL_LSRMST_ESCAPE
#undef SERIAL_LSRMST_LSR_DATA
#undef SERIAL_LSRMST_LSR_NODATA
#undef SERIAL_LSRMST_MST
#undef SERIAL_IOC_FCR_FIFO_ENABLE
#undef SERIAL_IOC_FCR_RCVR_RESET
#undef SERIAL_IOC_FCR_XMIT_RESET
#undef SERIAL_IOC_FCR_DMA_MODE
#undef SERIAL_IOC_FCR_RES1
#undef SERIAL_IOC_FCR_RES2
#undef SERIAL_IOC_FCR_RCVR_TRIGGER_LSB
#undef SERIAL_IOC_FCR_RCVR_TRIGGER_MSB
#undef SERIAL_IOC_MCR_DTR
#undef SERIAL_IOC_MCR_RTS
#undef SERIAL_IOC_MCR_OUT1
#undef SERIAL_IOC_MCR_OUT2
#undef SERIAL_IOC_MCR_LOOP
#include <ntddser.h>

#define NDEBUG
#include "../include/debug.h"

/* BUILDCOMMDCB & BUILDCOMMDCBANDTIMEOUTS */

/* TYPES */

/* Pointer to a callback that handles a particular parameter */
typedef BOOL (*COMMDCB_PARAM_CALLBACK)(DCB *, COMMTIMEOUTS *, BOOL *, LPWSTR *);

/* Symbolic flag of any length */
typedef struct _COMMDCB_PARAM_STRFLAG
{
    UNICODE_STRING String;
    ULONG_PTR Value;
} COMMDCB_PARAM_STRFLAG, *PCOMMDCB_PARAM_STRFLAG;

/* One char long symbolic flag */
typedef struct _COMMDCB_PARAM_CHARFLAG
{
    WCHAR Char;
    ULONG_PTR Value;
} COMMDCB_PARAM_CHARFLAG, *PCOMMDCB_PARAM_CHARFLAG;

/* MACROS */

/* Declare a parameter handler */
#define COMMDCB_PARAM_HANDLER(__P__) \
 BOOL COMMDCB_ ## __P__ ## Param \
 ( \
     DCB * Dcb, \
     COMMTIMEOUTS * Timeouts, \
     BOOL *StopBitsSet, \
     LPWSTR *StrTail \
 )

/* UTILITIES */
/*
 Lookup a string flag and return its numerical value. The flags array must be
 sorted - a dichotomycal search is performed
*/
static BOOL
COMMDCB_LookupStrFlag(PUNICODE_STRING Flag,
                      PCOMMDCB_PARAM_STRFLAG Flags,
                      int FlagCount,
                      PULONG_PTR Value)
{
    /* Lower and upper bound for dichotomycal search */
    int nLowerBound = 0;
    int nUpperBound = FlagCount - 1;

    do
    {
        LONG nComparison;
        /* pick the element in the middle of the area of interest as the pivot */
        int nCurFlag = nLowerBound + (nUpperBound - nLowerBound) / 2;

        /* compare the string with the pivot */
        nComparison = RtlCompareUnicodeString(Flag,
                                              &Flags[nCurFlag].String,
                                              TRUE);

        /* string is equal */
        if(nComparison == 0)
        {
            /* return the flag's value */
            *Value = Flags[nCurFlag].Value;

            /* success */
            return TRUE;
        }
        else if(nComparison < 0)
        {
            /*
             * restrict the search to the first half of the current slice, minus the pivot
             */
            nUpperBound = nCurFlag - 1;
        }
        else
        {
            /*
             * restrict the search to the second half of the current slice, minus the pivot
             */
            nLowerBound = nCurFlag + 1;
        }
    } while(nLowerBound <= nUpperBound);

    /* string not found: failure */
    return FALSE;
}

/* PARSERS */
/*
 Find the next character flag and return its numerical value. The flags array
 must be sorted - a dichotomycal search is performed
*/
static BOOL
COMMDCB_ParseCharFlag(LPWSTR *StrTail,
                      PCOMMDCB_PARAM_CHARFLAG Flags,
                      int FlagCount,
                      PULONG_PTR Value)
{
    /* Lower and upper bound for dichotomycal search */
    int nLowerBound = 0;
    int nUpperBound = FlagCount - 1;
    /* get the first character as the flag */
    WCHAR wcFlag = (*StrTail)[0];

    /* premature end of string, or the character is whitespace */
    if(!wcFlag || iswspace(wcFlag))
        return FALSE;

    /* uppercase the character for case-insensitive search */
    wcFlag = towupper(wcFlag);

    /* skip the character flag */
    (*StrTail)++;

    /* see COMMDCB_LookupStrFlag for a description of the algorithm */
    do
    {
        LONG nComparison;
        int nCurFlag = nLowerBound + (nUpperBound - nLowerBound) / 2;

        nComparison = wcFlag - towupper(Flags[nCurFlag].Char);

        if(nComparison == 0)
        {
            *Value = Flags[nCurFlag].Value;

            return TRUE;
        }
        else if(nComparison < 0)
        {
            nUpperBound = nCurFlag - 1;
        }
        else
        {
            nLowerBound = nCurFlag + 1;
        }
    } while(nUpperBound >= nLowerBound);

    /* flag not found: failure */
    return FALSE;
}

/*
 Find the next string flag and return its numerical value. The flags array must
 be sorted - a dichotomycal search is performed
*/
static BOOL
COMMDCB_ParseStrFlag(LPWSTR *StrTail,
                     PCOMMDCB_PARAM_STRFLAG Flags,
                     int FlagCount,
                     PULONG_PTR Value)
{
    LPWSTR pwcNewTail;
    UNICODE_STRING wstrFlag;

    /* scan the string until the first space character or the terminating null */
    for(pwcNewTail = *StrTail;
        pwcNewTail[0] && !iswspace(pwcNewTail[0]);
        pwcNewTail++);

    /* string flag empty */
    if(pwcNewTail == *StrTail)
        return FALSE;

    /* build the UNICODE_STRING description of the string flag */
    wstrFlag.Buffer = *StrTail;
    wstrFlag.Length = (pwcNewTail - *StrTail) * sizeof(WCHAR);
    wstrFlag.MaximumLength = wstrFlag.Length;

    /* skip the string flag */
    *StrTail = pwcNewTail;

    /* lookup the string flag's value and return it */
    return COMMDCB_LookupStrFlag(&wstrFlag, Flags, FlagCount, Value);
}

/*
 Parse a boolean value in the symbolic form on/off
*/
static BOOL
COMMDCB_ParseBool(LPWSTR *StrTail,
                  PBOOL Value)
{
    BOOL bRetVal;
    ULONG_PTR nValue;
    static COMMDCB_PARAM_STRFLAG a_BoolFlags[] = {
       { RTL_CONSTANT_STRING(L"off"), FALSE },
       { RTL_CONSTANT_STRING(L"on"),  TRUE }
    };

    /* try to recognize the next flag as a boolean */
    bRetVal = COMMDCB_ParseStrFlag(StrTail,
                                   a_BoolFlags,
                                   sizeof(a_BoolFlags) / sizeof(a_BoolFlags[0]),
                                   &nValue);


    if(!bRetVal)
        return FALSE;

    /* success */
    *Value = (nValue ? TRUE : FALSE);
    return TRUE;
}

/*
 Parse a decimal integer
*/
static BOOL
COMMDCB_ParseInt(LPWSTR *StrTail,
                 DWORD *Value)
{
    LPWSTR pwcPrevTail = *StrTail;
    DWORD nValue = wcstoul(*StrTail, StrTail, 10);

    /* no character was consumed: failure */
    if(pwcPrevTail == *StrTail)
        return FALSE;

    /* success */
    *Value = nValue;
    return TRUE;
}

/* PARAMETER HANDLERS */
/* baud= */
COMMDCB_PARAM_HANDLER(baud)
{
    DWORD nValue;

    (void)Timeouts;

    /* parse the baudrate */
    if(!COMMDCB_ParseInt(StrTail, &nValue))
        return FALSE;

    switch(nValue)
    {
        /* documented abbreviations */
        case 11:
            Dcb->BaudRate = 110;
            break;
        case 15:
            Dcb->BaudRate = 150;
            break;
        case 30:
            Dcb->BaudRate = 300;
            break;
        case 60:
            Dcb->BaudRate = 600;
            break;
        case 12:
            Dcb->BaudRate = 1200;
            break;
        case 24:
            Dcb->BaudRate = 2400;
            break;
        case 48:
            Dcb->BaudRate = 4800;
            break;
        case 96:
            Dcb->BaudRate = 9600;
            break;
        case 19:
            Dcb->BaudRate = 19200;
            break;

        /* literal value */
        default:
            Dcb->BaudRate = nValue;
            break;
    }

    /* if the stop bits haven't been specified explicitely */
    if(!(*StopBitsSet))
    {
        /* default the stop bits to 2 for 110 baud */
        if(Dcb->BaudRate == 110)
            Dcb->StopBits = TWOSTOPBITS;
        /* else, default the stop bits to 1 */
        else
            Dcb->StopBits = ONESTOPBIT;
    }

    /* success */
    return TRUE;
}

/* data= */
COMMDCB_PARAM_HANDLER(data)
{
    DWORD nValue;

    (void)Timeouts;
    (void)StopBitsSet;

    /* parse the data bits */
    if(!COMMDCB_ParseInt(StrTail, &nValue))
        return FALSE;

    /* value out of range: failure */
    if(nValue < 5 || nValue > 8)
        return FALSE;

    /* success */
    Dcb->ByteSize = (BYTE)nValue;
    return TRUE;
}

/* dtr= */
COMMDCB_PARAM_HANDLER(dtr)
{
    BOOL bRetVal;
    ULONG_PTR nValue;
    static COMMDCB_PARAM_STRFLAG a_DTRFlags[] = {
        { RTL_CONSTANT_STRING(L"hs"),  DTR_CONTROL_HANDSHAKE },
        { RTL_CONSTANT_STRING(L"off"), DTR_CONTROL_DISABLE },
        { RTL_CONSTANT_STRING(L"on"),  DTR_CONTROL_ENABLE }
    };

    (void)Timeouts;
    (void)StopBitsSet;

    /* parse the flag */
    bRetVal = COMMDCB_ParseStrFlag(StrTail,
                                   a_DTRFlags,
                                   sizeof(a_DTRFlags) / sizeof(a_DTRFlags[0]),
                                   &nValue);

    /* failure */
    if(!bRetVal)
        return FALSE;

    /* success */
    Dcb->fDtrControl = nValue;
    return TRUE;
}

/* idsr= */
COMMDCB_PARAM_HANDLER(idsr)
{
    BOOL bValue;

    (void)Timeouts;
    (void)StopBitsSet;

    /* parse the flag */
    if(!COMMDCB_ParseBool(StrTail, &bValue))
       return FALSE;

    /* success */
    Dcb->fDsrSensitivity = bValue;
    return TRUE;
}

/* octs= */
COMMDCB_PARAM_HANDLER(octs)
{
    BOOL bValue;

    (void)Timeouts;
    (void)StopBitsSet;

    /* parse the flag */
    if(!COMMDCB_ParseBool(StrTail, &bValue))
        return FALSE;

    /* success */
    Dcb->fOutxCtsFlow = bValue;
    return TRUE;
}

/* odsr= */
COMMDCB_PARAM_HANDLER(odsr)
{
    BOOL bValue;

    (void)Timeouts;
    (void)StopBitsSet;

    /* parse the flag */
    if(!COMMDCB_ParseBool(StrTail, &bValue))
        return FALSE;

    /* success */
    Dcb->fOutxDsrFlow = bValue;
    return TRUE;
}

/* parity= */
COMMDCB_PARAM_HANDLER(parity)
{
    BOOL bRetVal;
    ULONG_PTR nValue;
    static COMMDCB_PARAM_CHARFLAG a_ParityFlags[] = {
        { L'e', EVENPARITY },
        { L'm', MARKPARITY },
        { L'n', NOPARITY },
        { L'o', ODDPARITY },
        { L's', SPACEPARITY }
    };

    (void)Timeouts;
    (void)StopBitsSet;

    /* parse the flag */
    bRetVal = COMMDCB_ParseCharFlag(StrTail,
                                    a_ParityFlags,
                                    sizeof(a_ParityFlags) / sizeof(a_ParityFlags[0]),
                                    &nValue);

    /* failure */
    if(!bRetVal)
        return FALSE;

    /* success */
    Dcb->Parity = (BYTE)nValue;
    return TRUE;
}

/* rts= */
COMMDCB_PARAM_HANDLER(rts)
{
    DWORD nRetVal;
    ULONG_PTR nValue;
    static COMMDCB_PARAM_STRFLAG a_RTSFlags[] = {
        { RTL_CONSTANT_STRING(L"hs"),  RTS_CONTROL_HANDSHAKE },
        { RTL_CONSTANT_STRING(L"off"), RTS_CONTROL_DISABLE },
        { RTL_CONSTANT_STRING(L"on"),  RTS_CONTROL_ENABLE },
        { RTL_CONSTANT_STRING(L"tg"),  RTS_CONTROL_TOGGLE }
    };

    (void)Timeouts;
    (void)StopBitsSet;

    /* parse the flag */
    nRetVal = COMMDCB_ParseStrFlag(StrTail,
                                   a_RTSFlags,
                                   sizeof(a_RTSFlags) / sizeof(a_RTSFlags[0]),
                                   &nValue);

    /* failure */
    if(!nRetVal)
        return FALSE;

    /* success */
    Dcb->fRtsControl = nValue;
    return TRUE;
}

/* stop= */
COMMDCB_PARAM_HANDLER(stop)
{
    BOOL bRetVal;
    ULONG_PTR nValue;
    static COMMDCB_PARAM_STRFLAG a_StopFlags[] = {
        { RTL_CONSTANT_STRING(L"1"),   ONESTOPBIT },
        { RTL_CONSTANT_STRING(L"1.5"), ONE5STOPBITS },
        { RTL_CONSTANT_STRING(L"2"),   TWOSTOPBITS }
    };

    (void)Timeouts;

    /* parse the flag */
    bRetVal = COMMDCB_ParseStrFlag(StrTail,
                                   a_StopFlags,
                                   sizeof(a_StopFlags) / sizeof(a_StopFlags[0]),
                                   &nValue);

    /* failure */
    if(!bRetVal)
        return FALSE;

    /* tell the baud= handler that the stop bits have been specified explicitely */
    *StopBitsSet = TRUE;

    /* success */
    Dcb->StopBits = (BYTE)nValue;
    return TRUE;
}

/* to= */
COMMDCB_PARAM_HANDLER(to)
{
    BOOL bValue;

    (void)Dcb;
    (void)StopBitsSet;

    /* parse the flag */
    if(!COMMDCB_ParseBool(StrTail, &bValue))
        return FALSE;

    /* for BuildCommDCB(), Timeouts is NULL */
    if(Timeouts)
    {
        /* why? no idea. All values taken from Windows 2000 with experimentation */
        Timeouts->ReadIntervalTimeout = 0;
        Timeouts->ReadTotalTimeoutMultiplier = 0;
        Timeouts->ReadTotalTimeoutConstant = 0;
        Timeouts->WriteTotalTimeoutMultiplier = 0;

        if(bValue)
        {
            /* timeout */
            Timeouts->WriteTotalTimeoutConstant = 60000;
        }
        else
        {
            /* no timeout */
            Timeouts->WriteTotalTimeoutConstant = 0;
        }
    }

    /* success */
    return TRUE;
}

/* xon= */
COMMDCB_PARAM_HANDLER(xon)
{
    BOOL bValue;

    (void)Timeouts;
    (void)StopBitsSet;

    /* parse the flag */
    if(!COMMDCB_ParseBool(StrTail, &bValue))
        return FALSE;

    if(bValue)
    {
        /* XON/XOFF */
        Dcb->fInX = Dcb->fOutX = TRUE;
    }
    else
    {
        /* no XON/XOFF */
        Dcb->fInX = Dcb->fOutX = FALSE;
    }

    /* success */
    return TRUE;
}

/* FUNCTIONS */
#define COMMDCB_PARAM(__P__) \
 { \
  RTL_CONSTANT_STRING(L""UNICODIZE(#__P__ )), \
  (ULONG_PTR)&COMMDCB_ ## __P__ ## Param \
 }

/*
 * @implemented
 */
BOOL
STDCALL
BuildCommDCBAndTimeoutsW(LPCWSTR lpDef,
                         LPDCB lpDCB,
                         LPCOMMTIMEOUTS lpCommTimeouts)
{
    /* tell the baud= handler that the stop bits should be defaulted */
    BOOL bStopBitsSet = FALSE;

    /* parameter validation */
    if(lpDCB->DCBlength != sizeof(DCB))
        goto InvalidParam;

    /* set defaults */
    lpDCB->StopBits = ONESTOPBIT;

    /*
     * The documentation for MODE says that data= defaults to 7, but BuildCommDCB
     * doesn't seem to set it
     */
    /* lpDCB->ByteSize = 7; */

    /* skip COMx[n] */
    if(lpDef[0] &&
       towupper(lpDef[0]) == L'C' &&
       lpDef[1] &&
       towupper(lpDef[1]) == L'O' &&
       lpDef[2] &&
       towupper(lpDef[2]) == L'M')
    {
        DWORD nDummy;

        /* skip "COM" */
        lpDef += 3;

        /* premature end of string */
        if(!lpDef[0])
            goto InvalidParam;

        /* skip "x" */
        if(!COMMDCB_ParseInt((LPWSTR *)&lpDef, &nDummy))
            goto InvalidParam;

        /* skip ":" */
        if(lpDef[0] == L':')
            lpDef++;
    }

    /* skip leading whitespace */
    while(lpDef[0] && iswspace(lpDef[0]))
        lpDef++;

    /* repeat until the end of the string */
    while(lpDef[0])
    {
        static COMMDCB_PARAM_STRFLAG a_Params[] = {
            COMMDCB_PARAM(baud),
            COMMDCB_PARAM(data),
            COMMDCB_PARAM(dtr),
            COMMDCB_PARAM(idsr),
            COMMDCB_PARAM(octs),
            COMMDCB_PARAM(odsr),
            COMMDCB_PARAM(parity),
            COMMDCB_PARAM(rts),
            COMMDCB_PARAM(stop),
            COMMDCB_PARAM(to),
            COMMDCB_PARAM(xon)
        };
        BOOL bRetVal;
        COMMDCB_PARAM_CALLBACK pCallback;
        UNICODE_STRING wstrParam;
        LPWSTR pwcPrevTail = (LPWSTR)lpDef;

        /* get the parameter */
        while(lpDef[0] && lpDef[0] != L'=')
            lpDef++;

        /* premature end of string */
        if(!lpDef[0])
            goto InvalidParam;

        /* build the parameter's UNICODE_STRING */
        wstrParam.Buffer = pwcPrevTail;
        wstrParam.Length = (lpDef - pwcPrevTail) * sizeof(WCHAR);
        wstrParam.MaximumLength = wstrParam.Length;

        /* skip the "=" */
        lpDef++;

        /* lookup the callback for the parameter */
        bRetVal = COMMDCB_LookupStrFlag(&wstrParam,
                                        a_Params,
                                        sizeof(a_Params) / sizeof(a_Params[0]),
                                        (ULONG_PTR *)&pCallback);

        /* invalid parameter */
        if(!bRetVal)
            goto InvalidParam;

        /* call the callback to parse the parameter's argument */
        if(!pCallback(lpDCB, lpCommTimeouts, &bStopBitsSet, (LPWSTR *)&lpDef))
            goto InvalidParam;

        /* skip trailing whitespace */
        while(lpDef[0] && iswspace(lpDef[0]))
            lpDef++;
    }

    /* success */
    return TRUE;

InvalidParam:
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
BuildCommDCBAndTimeoutsA(LPCSTR lpDef,
                         LPDCB lpDCB,
                         LPCOMMTIMEOUTS lpCommTimeouts)
{
    NTSTATUS Status;
    BOOL bRetVal;
    ANSI_STRING strDef;
    UNICODE_STRING wstrDef;

    RtlInitAnsiString(&strDef, (LPSTR)lpDef);

    Status = RtlAnsiStringToUnicodeString(&wstrDef, &strDef, TRUE);

    if(!NT_SUCCESS(Status))
    {
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    bRetVal = BuildCommDCBAndTimeoutsW(wstrDef.Buffer, lpDCB, lpCommTimeouts);

    RtlFreeUnicodeString(&wstrDef);

    return bRetVal;
}

/*
 * @implemented
 */
BOOL
STDCALL
BuildCommDCBA(LPCSTR lpDef, LPDCB lpDCB)
{
    return BuildCommDCBAndTimeoutsA(lpDef, lpDCB, NULL);
}


/*
 * @implemented
 */
BOOL
STDCALL
BuildCommDCBW(LPCWSTR lpDef, LPDCB lpDCB)
{
    return BuildCommDCBAndTimeoutsW(lpDef, lpDCB, NULL);
}


/*
 * @implemented
 */
BOOL
STDCALL
ClearCommBreak(HANDLE hFile)
{
    DWORD dwBytesReturned;
    return DeviceIoControl(hFile, IOCTL_SERIAL_SET_BREAK_OFF,
                        NULL, 0, NULL, 0, &dwBytesReturned, NULL);
}


/*
 * @implemented
 */
BOOL
STDCALL
ClearCommError(HANDLE hFile, LPDWORD lpErrors, LPCOMSTAT lpComStat)
{
	BOOL status = FALSE;
	DWORD dwBytesReturned;
        SERIAL_STATUS SerialStatus;

        status = DeviceIoControl(hFile, IOCTL_SERIAL_GET_COMMSTATUS, NULL, 0,
                        &SerialStatus, sizeof(SERIAL_STATUS), &dwBytesReturned, NULL);

        if(!NT_SUCCESS(status))
        {
            return status;
        }

        if(lpErrors)
        {
            *lpErrors = 0;
            if(SerialStatus.Errors & SERIAL_ERROR_BREAK)
                *lpErrors |= CE_BREAK;
            if(SerialStatus.Errors & SERIAL_ERROR_FRAMING)
                *lpErrors |= CE_FRAME;
            if(SerialStatus.Errors & SERIAL_ERROR_OVERRUN)
                *lpErrors |= CE_OVERRUN;
            if(SerialStatus.Errors & SERIAL_ERROR_QUEUEOVERRUN )
                *lpErrors |= CE_RXOVER;
            if(SerialStatus.Errors & SERIAL_ERROR_PARITY)
                *lpErrors |= CE_RXPARITY;
        }

	if (lpComStat)
        {
            ZeroMemory(lpComStat, sizeof(COMSTAT));

            if(SerialStatus.HoldReasons & SERIAL_TX_WAITING_FOR_CTS)
                lpComStat->fCtsHold = TRUE;
            if(SerialStatus.HoldReasons & SERIAL_TX_WAITING_FOR_DSR)
                lpComStat->fDsrHold = TRUE;
            if(SerialStatus.HoldReasons & SERIAL_TX_WAITING_FOR_DCD)
                lpComStat->fRlsdHold = TRUE;
            if(SerialStatus.HoldReasons & SERIAL_TX_WAITING_FOR_XON)
                lpComStat->fXoffHold = TRUE;
            if(SerialStatus.HoldReasons & SERIAL_TX_WAITING_XOFF_SENT)
                lpComStat->fXoffSent = TRUE;

            if(SerialStatus.EofReceived)
                lpComStat->fEof = TRUE;

            if(SerialStatus.WaitForImmediate)
                lpComStat->fTxim = TRUE;

            lpComStat->cbInQue = SerialStatus.AmountInInQueue;
            lpComStat->cbOutQue = SerialStatus.AmountInOutQueue;
	}
	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
CommConfigDialogA(LPCSTR lpszName, HWND hWnd, LPCOMMCONFIG lpCC)
{
	PWCHAR NameW;
	DWORD result;

	/* don't use the static thread buffer so operations in serialui
	   don't overwrite the string */
	if(!(NameW = FilenameA2W(lpszName, TRUE)))
	{
		return FALSE;
	}

	result = CommConfigDialogW(NameW, hWnd, lpCC);

	RtlFreeHeap(RtlGetProcessHeap(), 0, NameW);

	return result;
}


/*
 * @implemented
 */
BOOL
STDCALL
CommConfigDialogW(LPCWSTR lpszName, HWND hWnd, LPCOMMCONFIG lpCC)
{
	DWORD (STDCALL *drvCommDlgW)(LPCWSTR, HWND, LPCOMMCONFIG);
	HMODULE hSerialuiDll;
	DWORD result;

	//FIXME: Get dll name from registry. (setupapi needed)
	if(!(hSerialuiDll = LoadLibraryW(L"serialui.dll")))
	{
		DPRINT("CommConfigDialogW: serialui.dll not found.\n");
		return FALSE;
	}

	drvCommDlgW = (DWORD (STDCALL *)(LPCWSTR, HWND, LPCOMMCONFIG))
					GetProcAddress(hSerialuiDll, "drvCommConfigDialogW");

	if(!drvCommDlgW)
	{
		DPRINT("CommConfigDialogW: serialui does not export drvCommConfigDialogW\n");
		FreeLibrary(hSerialuiDll);
		return FALSE;
	}

	result = drvCommDlgW(lpszName, hWnd, lpCC);
	SetLastError(result);
	FreeLibrary(hSerialuiDll);
	
	return (result == ERROR_SUCCESS ? TRUE : FALSE);
}


/*
 * @implemented
 */
BOOL
STDCALL
EscapeCommFunction(HANDLE hFile, DWORD dwFunc)
{
	BOOL result = FALSE;
	DWORD dwBytesReturned;

	switch (dwFunc) {
    case CLRDTR: // Clears the DTR (data-terminal-ready) signal.
        result = DeviceIoControl(hFile, IOCTL_SERIAL_CLR_DTR, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
		break;
    case CLRRTS: // Clears the RTS (request-to-send) signal.
        result = DeviceIoControl(hFile, IOCTL_SERIAL_CLR_RTS, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
		break;
    case SETDTR: // Sends the DTR (data-terminal-ready) signal.
        result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_DTR, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
		break;
    case SETRTS: // Sends the RTS (request-to-send) signal.
        result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_RTS, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
		break;
    case SETXOFF: // Causes transmission to act as if an XOFF character has been received.
        result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_XOFF, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
		break;
    case SETXON: // Causes transmission to act as if an XON character has been received.
        result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_XON, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
		break;
    case SETBREAK: // Suspends character transmission and places the transmission line in a break state until the ClearCommBreak function is called (or EscapeCommFunction is called with the CLRBREAK extended function code). The SETBREAK extended function code is identical to the SetCommBreak function. Note that this extended function does not flush data that has not been transmitted.
        result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_BREAK_ON, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
		break;
    case CLRBREAK: // Restores character transmission and places the transmission line in a nonbreak state. The CLRBREAK extended function code is identical to the ClearCommBreak function.
        result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_BREAK_OFF, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
		break;
	default:
        DPRINT("EscapeCommFunction() WARNING: unknown function code\n");
    	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		break;
	}
	return result;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetCommConfig(HANDLE hCommDev, LPCOMMCONFIG lpCC, LPDWORD lpdwSize)
{
	BOOL ReturnValue = FALSE;
	LPCOMMPROP lpComPort;

	DPRINT("GetCommConfig(%d, %p, %p)\n", hCommDev, lpCC, lpdwSize);

	lpComPort = RtlAllocateHeap( hProcessHeap,
		HEAP_ZERO_MEMORY,
		sizeof(COMMPROP) + 0x100 );

	if(NULL == lpComPort) {
		DPRINT("GetCommConfig() - ERROR_NOT_ENOUGH_MEMORY\n");
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	if( (NULL == lpdwSize) 
		|| (NULL == lpCC) ) {
			DPRINT("GetCommConfig() - invalid parameter\n");
			SetLastError(ERROR_INVALID_PARAMETER);
			ReturnValue = FALSE;
	}
	else
	{
		lpComPort->wPacketLength = sizeof(COMMPROP) + 0x100;
		lpComPort->dwProvSpec1 = COMMPROP_INITIALIZED;
		ReturnValue = GetCommProperties(hCommDev, lpComPort);
		if( ReturnValue )
		{
			lpCC->dwSize = sizeof(COMMCONFIG);
			lpCC->wVersion = 1;
			lpCC->wReserved = 0;
			lpCC->dwProviderSubType = lpComPort->dwProvSubType;
			lpCC->dwProviderSize = lpComPort->dwProvSpec2;
			if( 0 == lpComPort->dwProvSpec2 ) {
				lpCC->dwProviderOffset = 0;
			} else {
				lpCC->dwProviderOffset = (ULONG_PTR)&lpCC->wcProviderData[0] - (ULONG_PTR)lpCC;
			}
			if( (lpCC->dwProviderSize+lpCC->dwSize) > *lpdwSize ) {
				DPRINT("GetCommConfig() - ERROR_INSUFFICIENT_BUFFER\n");
				SetLastError(ERROR_INSUFFICIENT_BUFFER);
				ReturnValue = FALSE;
			} else {
				RtlCopyMemory(lpCC->wcProviderData, lpComPort->wcProvChar, lpCC->dwProviderSize);
				ReturnValue = GetCommState(hCommDev, &lpCC->dcb);
			}
			*lpdwSize = lpCC->dwSize+lpCC->dwProviderSize;
		}
	}

	RtlFreeHeap(hProcessHeap, 0, lpComPort);
	return (ReturnValue);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetCommMask(HANDLE hFile, LPDWORD lpEvtMask)
{
	DWORD dwBytesReturned;
        return DeviceIoControl(hFile, IOCTL_SERIAL_GET_WAIT_MASK,
		NULL, 0, lpEvtMask, sizeof(DWORD), &dwBytesReturned, NULL);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetCommModemStatus(HANDLE hFile, LPDWORD lpModemStat)
{
	DWORD dwBytesReturned;

	return DeviceIoControl(hFile, IOCTL_SERIAL_GET_MODEMSTATUS,
		NULL, 0, lpModemStat, sizeof(DWORD), &dwBytesReturned, NULL);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetCommProperties(HANDLE hFile, LPCOMMPROP lpCommProp)
{
	DWORD dwBytesReturned;
	return DeviceIoControl(hFile, IOCTL_SERIAL_GET_PROPERTIES, 0, 0,
		lpCommProp, sizeof(COMMPROP), &dwBytesReturned, 0);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetCommState(HANDLE hFile, LPDCB lpDCB)
{
	BOOL result = FALSE;
	DWORD dwBytesReturned;

	SERIAL_BAUD_RATE BaudRate;
	SERIAL_HANDFLOW HandFlow;
	SERIAL_CHARS SpecialChars;
	SERIAL_LINE_CONTROL LineControl;

    DPRINT("GetCommState(%d, %p)\n", hFile, lpDCB);

	if (lpDCB == NULL) {
        DPRINT("ERROR: GetCommState() - NULL DCB pointer\n");
		return FALSE;
	}

	lpDCB->DCBlength = sizeof(DCB);

	/* FIXME: need to fill following fields (1 bit):
	 * fBinary: binary mode, no EOF check
	 * fParity: enable parity checking
	 * fOutX  : XON/XOFF out flow control
	 * fInX   : XON/XOFF in flow control
	 */

	result = DeviceIoControl(hFile, IOCTL_SERIAL_GET_BAUD_RATE,
			 NULL, 0, &BaudRate, sizeof(BaudRate),&dwBytesReturned, NULL);
	if (!NT_SUCCESS(result)) {
        DPRINT("ERROR: GetCommState() - DeviceIoControl(IOCTL_SERIAL_GET_BAUD_RATE) Failed.\n");
		return FALSE;
	}
    lpDCB->BaudRate = BaudRate.BaudRate;

	result = DeviceIoControl(hFile, IOCTL_SERIAL_GET_HANDFLOW,
			NULL, 0, &HandFlow, sizeof(HandFlow), &dwBytesReturned, NULL);
	if (!NT_SUCCESS(result)) {
        DPRINT("ERROR: GetCommState() - DeviceIoControl(IOCTL_SERIAL_GET_HANDFLOW) Failed.\n");
		return FALSE;
	}
	if (HandFlow.ControlHandShake & SERIAL_CTS_HANDSHAKE) {
    	lpDCB->fOutxCtsFlow = 1;
	}
    if (HandFlow.ControlHandShake & SERIAL_DSR_HANDSHAKE) {
    	lpDCB->fOutxDsrFlow = 1;
	}
    if (HandFlow.ControlHandShake & SERIAL_DTR_CONTROL) {
    	lpDCB->fDtrControl = 1;
	}
    if (HandFlow.ControlHandShake & SERIAL_DTR_HANDSHAKE) {
    	lpDCB->fDtrControl = 2;
	}
    if (HandFlow.ControlHandShake & SERIAL_RTS_CONTROL) {
    	lpDCB->fRtsControl = 1;
	}
    if (HandFlow.ControlHandShake & SERIAL_RTS_HANDSHAKE) {
    	lpDCB->fRtsControl = 2;
	}
    if (HandFlow.ControlHandShake & SERIAL_DSR_SENSITIVITY) {
    	lpDCB->fDsrSensitivity = 1;
	}
    if (HandFlow.ControlHandShake & SERIAL_ERROR_ABORT) {
    	lpDCB->fAbortOnError = 1;
	}

    if (HandFlow.FlowReplace & SERIAL_ERROR_CHAR) {
    	lpDCB->fErrorChar = 1;
	}
    if (HandFlow.FlowReplace & SERIAL_NULL_STRIPPING) {
    	lpDCB->fNull = 1;
	}
    if (HandFlow.FlowReplace & SERIAL_XOFF_CONTINUE) {
    	lpDCB->fTXContinueOnXoff = 1;
	}
    lpDCB->XonLim = (WORD)HandFlow.XonLimit;
    lpDCB->XoffLim = (WORD)HandFlow.XoffLimit;

	result = DeviceIoControl(hFile, IOCTL_SERIAL_GET_CHARS,
			NULL, 0, &SpecialChars, sizeof(SpecialChars), &dwBytesReturned, NULL);
	if (!NT_SUCCESS(result)) {
        DPRINT("ERROR: GetCommState() - DeviceIoControl(IOCTL_SERIAL_GET_CHARS) Failed.\n");
		return FALSE;
	}

    lpDCB->EofChar = SpecialChars.EofChar;
    lpDCB->ErrorChar = SpecialChars.ErrorChar;
    // = SpecialChars.BreakChar;
    lpDCB->EvtChar = SpecialChars.EventChar;
    lpDCB->XonChar = SpecialChars.XonChar;
    lpDCB->XoffChar = SpecialChars.XoffChar;

	result = DeviceIoControl(hFile, IOCTL_SERIAL_GET_LINE_CONTROL,
			NULL, 0, &LineControl, sizeof(LineControl), &dwBytesReturned, NULL);
	if (!NT_SUCCESS(result)) {
        DPRINT("ERROR: GetCommState() - DeviceIoControl(IOCTL_SERIAL_GET_LINE_CONTROL) Failed.\n");
		return FALSE;
	}
	lpDCB->StopBits = LineControl.StopBits;
	lpDCB->Parity = LineControl.Parity;
	lpDCB->ByteSize = LineControl.WordLength;
    DPRINT("GetCommState() - COMPLETED SUCCESSFULLY\n");
	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts)
{
	DWORD dwBytesReturned;

	if (lpCommTimeouts == NULL) {
		return FALSE;
	}

	return DeviceIoControl(hFile, IOCTL_SERIAL_GET_TIMEOUTS,
							 NULL, 0,
							 lpCommTimeouts, sizeof(COMMTIMEOUTS),
							 &dwBytesReturned, NULL);
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetDefaultCommConfigW(LPCWSTR lpszName, LPCOMMCONFIG lpCC, LPDWORD lpdwSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetDefaultCommConfigA(LPCSTR lpszName, LPCOMMCONFIG lpCC, LPDWORD lpdwSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
PurgeComm(HANDLE hFile, DWORD dwFlags)
{
	DWORD dwBytesReturned;

        return DeviceIoControl(hFile, IOCTL_SERIAL_PURGE,
		&dwFlags, sizeof(DWORD), NULL, 0, &dwBytesReturned, NULL);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetCommBreak(HANDLE hFile)
{
	DWORD dwBytesReturned;

        return DeviceIoControl(hFile, IOCTL_SERIAL_SET_BREAK_ON, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetCommConfig(HANDLE hCommDev, LPCOMMCONFIG lpCC, DWORD dwSize)
{
	BOOL ReturnValue = FALSE;

	DPRINT("SetCommConfig(%d, %p, %d)\n", hCommDev, lpCC, dwSize);

	if(NULL == lpCC)
	{
		DPRINT("SetCommConfig() - invalid parameter\n");
		SetLastError(ERROR_INVALID_PARAMETER);
		ReturnValue = FALSE;
	}
	else
	{
		ReturnValue = SetCommState(hCommDev, &lpCC->dcb);
	}

	return ReturnValue;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetCommMask(HANDLE hFile, DWORD dwEvtMask)
{
	DWORD dwBytesReturned;

        return DeviceIoControl(hFile, IOCTL_SERIAL_SET_WAIT_MASK,
		&dwEvtMask, sizeof(DWORD), NULL, 0, &dwBytesReturned, NULL);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetCommState(HANDLE	hFile, LPDCB lpDCB)
{
	BOOL result = FALSE;
	DWORD dwBytesReturned;

	SERIAL_BAUD_RATE BaudRate;
	SERIAL_HANDFLOW HandFlow;
	SERIAL_CHARS SpecialChars;
	SERIAL_LINE_CONTROL LineControl;

    DPRINT("SetCommState(%d, %p) - ENTERED\n", hFile, lpDCB);

	if (lpDCB == NULL) {
        DPRINT("SetCommState() - ERROR: NULL DCB pointer passed\n");
		return FALSE;
	}

	BaudRate.BaudRate = lpDCB->BaudRate;
	result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_BAUD_RATE,
		&BaudRate, sizeof(BaudRate), NULL, 0, &dwBytesReturned, NULL);
	if (!NT_SUCCESS(result)) {
        DPRINT("ERROR: SetCommState() - DeviceIoControl(IOCTL_SERIAL_SET_BAUD_RATE) Failed.\n");
		return FALSE;
	}
/*
#define SERIAL_DTR_MASK           ((ULONG)0x03)
#define SERIAL_DTR_CONTROL        ((ULONG)0x01)
#define SERIAL_DTR_HANDSHAKE      ((ULONG)0x02)
#define SERIAL_CTS_HANDSHAKE      ((ULONG)0x08)
#define SERIAL_DSR_HANDSHAKE      ((ULONG)0x10)
#define SERIAL_DCD_HANDSHAKE      ((ULONG)0x20)
#define SERIAL_OUT_HANDSHAKEMASK  ((ULONG)0x38)
#define SERIAL_DSR_SENSITIVITY    ((ULONG)0x40)
#define SERIAL_ERROR_ABORT        ((ULONG)0x80000000)
#define SERIAL_CONTROL_INVALID    ((ULONG)0x7fffff84)
 */
    HandFlow.ControlHandShake = 0;

	if (lpDCB->fOutxCtsFlow) {
        HandFlow.ControlHandShake |= SERIAL_CTS_HANDSHAKE;
	}
	if (lpDCB->fOutxDsrFlow) {
        HandFlow.ControlHandShake |= SERIAL_DSR_HANDSHAKE;
	}
	if (lpDCB->fDtrControl) {
        HandFlow.ControlHandShake |= SERIAL_DTR_CONTROL;
	}
	if (lpDCB->fDtrControl) {
        HandFlow.ControlHandShake |= SERIAL_DTR_HANDSHAKE;
	}
	if (lpDCB->fRtsControl) {
        HandFlow.ControlHandShake |= SERIAL_RTS_CONTROL;
	}
	if (lpDCB->fRtsControl) {
        HandFlow.ControlHandShake |= SERIAL_RTS_HANDSHAKE;
	}
	if (lpDCB->fDsrSensitivity) {
        HandFlow.ControlHandShake |= SERIAL_DSR_SENSITIVITY;
	}
	if (lpDCB->fAbortOnError) {
        HandFlow.ControlHandShake |= SERIAL_ERROR_ABORT;
	}
/*
#define SERIAL_AUTO_TRANSMIT      ((ULONG)0x01)
#define SERIAL_AUTO_RECEIVE       ((ULONG)0x02)
#define SERIAL_ERROR_CHAR         ((ULONG)0x04)
#define SERIAL_NULL_STRIPPING     ((ULONG)0x08)
#define SERIAL_BREAK_CHAR         ((ULONG)0x10)
#define SERIAL_RTS_MASK           ((ULONG)0xc0)
#define SERIAL_RTS_CONTROL        ((ULONG)0x40)
#define SERIAL_RTS_HANDSHAKE      ((ULONG)0x80)
#define SERIAL_TRANSMIT_TOGGLE    ((ULONG)0xc0)
#define SERIAL_XOFF_CONTINUE      ((ULONG)0x80000000)
#define SERIAL_FLOW_INVALID       ((ULONG)0x7fffff20)
 */
    HandFlow.FlowReplace = 0;
	if (lpDCB->fErrorChar) {
        HandFlow.FlowReplace |= SERIAL_ERROR_CHAR;
	}
	if (lpDCB->fNull) {
        HandFlow.FlowReplace |= SERIAL_NULL_STRIPPING;
	}
	if (lpDCB->fTXContinueOnXoff) {
        HandFlow.FlowReplace |= SERIAL_XOFF_CONTINUE;
	}
    HandFlow.XonLimit = lpDCB->XonLim;
    HandFlow.XoffLimit = lpDCB->XoffLim;
	result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_HANDFLOW,
		&HandFlow, sizeof(HandFlow), NULL, 0, &dwBytesReturned, NULL);
	if (!NT_SUCCESS(result)) {
        DPRINT("ERROR: SetCommState() - DeviceIoControl(IOCTL_SERIAL_SET_HANDFLOW) Failed.\n");
		return FALSE;
	}

    SpecialChars.EofChar = lpDCB->EofChar;
    SpecialChars.ErrorChar = lpDCB->ErrorChar;
    SpecialChars.BreakChar = 0;
    SpecialChars.EventChar = lpDCB->EvtChar;
    SpecialChars.XonChar = lpDCB->XonChar;
    SpecialChars.XoffChar = lpDCB->XoffChar;
	result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_CHARS,
		&SpecialChars, sizeof(SpecialChars), NULL, 0, &dwBytesReturned, NULL);
	if (!NT_SUCCESS(result)) {
        DPRINT("ERROR: SetCommState() - DeviceIoControl(IOCTL_SERIAL_SET_CHARS) Failed.\n");
		return FALSE;
	}

	LineControl.StopBits = lpDCB->StopBits;
	LineControl.Parity = lpDCB->Parity;
	LineControl.WordLength = lpDCB->ByteSize;
	result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_LINE_CONTROL,
		&LineControl, sizeof(LineControl), NULL, 0, &dwBytesReturned, NULL);
	if (!NT_SUCCESS(result)) {
        DPRINT("ERROR: SetCommState() - DeviceIoControl(IOCTL_SERIAL_SET_LINE_CONTROL) Failed.\n");
		return FALSE;
	}

    DPRINT("SetCommState() - COMPLETED SUCCESSFULLY\n");
	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts)
{
	DWORD dwBytesReturned;
	SERIAL_TIMEOUTS Timeouts;

	if (lpCommTimeouts == NULL) {
		return FALSE;
	}
	Timeouts.ReadIntervalTimeout = lpCommTimeouts->ReadIntervalTimeout;
	Timeouts.ReadTotalTimeoutMultiplier = lpCommTimeouts->ReadTotalTimeoutMultiplier;
	Timeouts.ReadTotalTimeoutConstant = lpCommTimeouts->ReadTotalTimeoutConstant;
	Timeouts.WriteTotalTimeoutMultiplier = lpCommTimeouts->WriteTotalTimeoutMultiplier;
	Timeouts.WriteTotalTimeoutConstant = lpCommTimeouts->WriteTotalTimeoutConstant;

        return DeviceIoControl(hFile, IOCTL_SERIAL_SET_TIMEOUTS,
		&Timeouts, sizeof(Timeouts), NULL, 0, &dwBytesReturned, NULL);
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetDefaultCommConfigA(LPCSTR lpszName, LPCOMMCONFIG lpCC, DWORD dwSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetDefaultCommConfigW(LPCWSTR lpszName, LPCOMMCONFIG lpCC, DWORD dwSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetupComm(HANDLE hFile, DWORD dwInQueue, DWORD dwOutQueue)
{
	DWORD dwBytesReturned;
	SERIAL_QUEUE_SIZE QueueSize;

    QueueSize.InSize = dwInQueue;
    QueueSize.OutSize = dwOutQueue;
    return DeviceIoControl(hFile, IOCTL_SERIAL_SET_QUEUE_SIZE,
		&QueueSize, sizeof(QueueSize), NULL, 0, &dwBytesReturned, NULL);
}


/*
 * @implemented
 */
BOOL
STDCALL
TransmitCommChar(HANDLE hFile, char cChar)
{
	DWORD dwBytesReturned;
	return DeviceIoControl(hFile, IOCTL_SERIAL_IMMEDIATE_CHAR,
		&cChar, sizeof(cChar), NULL, 0, &dwBytesReturned, NULL);
}


/*
 * @implemented
 */
BOOL
STDCALL
WaitCommEvent(HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped)
{
	DWORD dwBytesReturned;

	if (lpEvtMask == NULL) {
		return FALSE;
	}

	return DeviceIoControl(hFile, IOCTL_SERIAL_WAIT_ON_MASK,
		NULL, 0, lpEvtMask, sizeof(DWORD), &dwBytesReturned, lpOverlapped);
}

/* EOF */
