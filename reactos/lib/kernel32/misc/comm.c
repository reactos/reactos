/* $Id: comm.c,v 1.7 2003/02/12 00:39:31 hyperion Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/comm.c
 * PURPOSE:         Comm functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  modified from WINE [ Onno Hovers, (onno@stack.urc.tue.nl) ]
 *					Robert Dickenson (robd@mok.lvcom.com)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *					RDD (30/09/2002) implemented many function bodies to call serial driver.
 *                                      KJK (11/02/2003) implemented BuildCommDCB & BuildCommDCBAndTimeouts
 */

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* BUILDCOMMDCB & BUILDCOMMDCBANDTIMEOUTS */

/* TYPES */

/* Pointer to a callback that handles a particular parameter */
typedef BOOL (*COMMDCB_PARAM_CALLBACK)
(
 DCB *,
 COMMTIMEOUTS *,
 BOOL *,
 LPWSTR *
);

/* Symbolic flag of any length */
typedef struct _COMMDCB_PARAM_STRFLAG
{
 UNICODE_STRING String;
 ULONG_PTR Value;
} COMMDCB_PARAM_STRFLAG;

/* One char long symbolic flag */
typedef struct _COMMDCB_PARAM_CHARFLAG
{
 WCHAR Char;
 ULONG_PTR Value;
} COMMDCB_PARAM_CHARFLAG;

/* MACROS */
/* stupid Borland C++ requires this */
#define _L(__S__) L ## __S__

/* Declare a parameter handler */
#define COMMDCB_PARAM_HANDLER(__P__) \
 BOOL COMMDCB_ ## __P__ ## Param \
 ( \
  DCB * Dcb, \
  COMMTIMEOUTS * Timeouts, \
  BOOL * StopBitsSet, \
  LPWSTR * StrTail \
 )

/* UTILITIES */
/*
 Lookup a string flag and return its numerical value. The flags array must be
 sorted - a dichotomycal search is performed
*/
BOOL COMMDCB_LookupStrFlag
(
 UNICODE_STRING * Flag,
 COMMDCB_PARAM_STRFLAG * Flags,
 int FlagCount,
 ULONG_PTR * Value
)
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
  nComparison = RtlCompareUnicodeString
  (
   Flag,
   &Flags[nCurFlag].String,
   TRUE
  );

  /* string is equal */
  if(nComparison == 0)
  {
   /* return the flag's value */
   *Value = Flags[nCurFlag].Value;

   /* success */
   return TRUE;
  }
  /* string is less than */
  else if(nComparison < 0)
  {
   /*
    restrict the search to the first half of the current slice, minus the pivot
   */
   nUpperBound = nCurFlag - 1;

   /* fallthrough */
  }
  /* string is greater than */
  else
  {
   /*
    restrict the search to the second half of the current slice, minus the pivot
   */
   nLowerBound = nCurFlag + 1;

   /* fallthrough */
  }
 }
 /* continue until the slice is empty */
 while(nLowerBound <= nUpperBound);

 /* string not found: failure */
 return FALSE;
}

/* PARSERS */
/*
 Find the next character flag and return its numerical value. The flags array
 must be sorted - a dichotomycal search is performed
*/
BOOL COMMDCB_ParseCharFlag
(
 LPWSTR * StrTail,
 COMMDCB_PARAM_CHARFLAG * Flags,
 int FlagCount,
 ULONG_PTR * Value
)
{
 /* Lower and upper bound for dichotomycal search */
 int nLowerBound = 0;
 int nUpperBound = FlagCount - 1;
 /* get the first character as the flag */
 WCHAR wcFlag = (*StrTail)[0];

 /* premature end of string, or the character is whitespace */
 if(!wcFlag || iswspace(wcFlag))
  /* failure */
  return FALSE;
 
 /* uppercase the character for case-insensitive search */
 wcFlag = towupper(wcFlag);

 /* skip the character flag */
 ++ (*StrTail);

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
 }
 while(nUpperBound >= nLowerBound);

 /* flag not found: failure */
 return FALSE;
}

/*
 Find the next string flag and return its numerical value. The flags array must
 be sorted - a dichotomycal search is performed
*/
BOOL COMMDCB_ParseStrFlag
(
 LPWSTR * StrTail,
 COMMDCB_PARAM_STRFLAG * Flags,
 int FlagCount,
 ULONG_PTR * Value
)
{
 LPWSTR pwcNewTail = *StrTail;
 UNICODE_STRING wstrFlag;

 /* scan the string until the first space character or the terminating null */
 while(pwcNewTail[0] && !iswspace(pwcNewTail[0]))
  ++ pwcNewTail;

 /* string flag empty */
 if(pwcNewTail == *StrTail)
  /* failure */
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
BOOL COMMDCB_ParseBool(LPWSTR * StrTail, BOOL * Value)
{
 BOOL bRetVal;
 ULONG_PTR nValue;
 static COMMDCB_PARAM_STRFLAG a_BoolFlags[] =
 {
  { UNICODE_STRING_INITIALIZER(L"off"), FALSE },
  { UNICODE_STRING_INITIALIZER(L"on"),  TRUE }
 };

 /* try to recognize the next flag as a boolean */
 bRetVal = COMMDCB_ParseStrFlag
 (
  StrTail,
  a_BoolFlags,
  sizeof(a_BoolFlags) / sizeof(a_BoolFlags[0]),
  &nValue
 );

 /* failure */
 if(!bRetVal) return FALSE;

 /* success */
 *Value = nValue ? TRUE : FALSE;
 return TRUE;
}

/*
 Parse a decimal integer
*/
BOOL COMMDCB_ParseInt(LPWSTR * StrTail, DWORD * Value)
{
 LPWSTR pwcPrevTail = *StrTail;
 DWORD nValue = wcstoul(*StrTail, StrTail, 10);
 
 /* no character was consumed: failure */
 if(pwcPrevTail == *StrTail) return FALSE;

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
  /* failure */
  return FALSE;

 switch(nValue)
 {
  /* documented abbreviations */
  case 11: Dcb->BaudRate = 110; break;
  case 15: Dcb->BaudRate = 150; break;
  case 30: Dcb->BaudRate = 300; break;
  case 60: Dcb->BaudRate = 600; break;
  case 12: Dcb->BaudRate = 1200; break;
  case 24: Dcb->BaudRate = 2400; break;
  case 48: Dcb->BaudRate = 4800; break;
  case 96: Dcb->BaudRate = 9600; break;
  case 19: Dcb->BaudRate = 19200; break;
  /* literal value */
  default: Dcb->BaudRate = nValue; break;
 }

 /* if the stop bits haven't been specified explicitely */
 if(!(*StopBitsSet))
 {
  /* default the stop bits to 2 for 110 baud */
  if(Dcb->BaudRate == 110) Dcb->StopBits = TWOSTOPBITS;
  /* else, default the stop bits to 1 */  
  else Dcb->StopBits = ONESTOPBIT;
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
  /* failure */
  return FALSE;

 /* value out of range: failure */
 if(nValue < 5 || nValue > 8) return FALSE;
  
 /* success */
 Dcb->ByteSize = nValue;
 return TRUE;
}

/* dtr= */
COMMDCB_PARAM_HANDLER(dtr)
{
 BOOL bRetVal;
 ULONG_PTR nValue;
 static COMMDCB_PARAM_STRFLAG a_DTRFlags[] =
 {
  { UNICODE_STRING_INITIALIZER(L"hs"),  DTR_CONTROL_HANDSHAKE },
  { UNICODE_STRING_INITIALIZER(L"off"), DTR_CONTROL_DISABLE },
  { UNICODE_STRING_INITIALIZER(L"on"),  DTR_CONTROL_ENABLE }
 };

 (void)Timeouts;
 (void)StopBitsSet;

 /* parse the flag */
 bRetVal = COMMDCB_ParseStrFlag
 (
  StrTail,
  a_DTRFlags,
  sizeof(a_DTRFlags) / sizeof(a_DTRFlags[0]),
  &nValue
 );

 /* failure */
 if(!bRetVal) return FALSE;

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
  /* failure */
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
  /* failure */
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
  /* failure */
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
 static COMMDCB_PARAM_CHARFLAG a_ParityFlags[] =
 {
  { L'e', EVENPARITY },
  { L'm', MARKPARITY },
  { L'n', NOPARITY },
  { L'o', ODDPARITY },
  { L's', SPACEPARITY }
 };

 (void)Timeouts;
 (void)StopBitsSet;

 /* parse the flag */
 bRetVal = COMMDCB_ParseCharFlag
 (
  StrTail,
  a_ParityFlags,
  sizeof(a_ParityFlags) / sizeof(a_ParityFlags[0]),
  &nValue
 );

 /* failure */
 if(!bRetVal) return FALSE;

 /* success */
 Dcb->Parity = nValue;
 return TRUE;
}

/* rts= */
COMMDCB_PARAM_HANDLER(rts)
{
 DWORD nRetVal;
 ULONG_PTR nValue;
 static COMMDCB_PARAM_STRFLAG a_RTSFlags[] =
 {
  { UNICODE_STRING_INITIALIZER(L"hs"),  RTS_CONTROL_HANDSHAKE },
  { UNICODE_STRING_INITIALIZER(L"off"), RTS_CONTROL_DISABLE },
  { UNICODE_STRING_INITIALIZER(L"on"),  RTS_CONTROL_ENABLE },
  { UNICODE_STRING_INITIALIZER(L"tg"),  RTS_CONTROL_TOGGLE }
 };

 (void)Timeouts;
 (void)StopBitsSet;

 /* parse the flag */
 nRetVal = COMMDCB_ParseStrFlag
 (
  StrTail,
  a_RTSFlags,
  sizeof(a_RTSFlags) / sizeof(a_RTSFlags[0]),
  &nValue
 );

 /* failure */
 if(!nRetVal) return FALSE;

 /* success */
 Dcb->fRtsControl = nValue;
 return TRUE;
}

/* stop= */
COMMDCB_PARAM_HANDLER(stop)
{
 BOOL bRetVal;
 ULONG_PTR nValue;
 static COMMDCB_PARAM_STRFLAG a_StopFlags[] =
 {
  { UNICODE_STRING_INITIALIZER(L"1"),   ONESTOPBIT },
  { UNICODE_STRING_INITIALIZER(L"1.5"), ONE5STOPBITS },
  { UNICODE_STRING_INITIALIZER(L"2"),   TWOSTOPBITS }
 };

 (void)Timeouts;

 /* parse the flag */
 bRetVal = COMMDCB_ParseStrFlag
 (
  StrTail,
  a_StopFlags,
  sizeof(a_StopFlags) / sizeof(a_StopFlags[0]),
  &nValue
 );

 /* failure */
 if(!bRetVal) return FALSE;

 /* tell the baud= handler that the stop bits have been specified explicitely */
 *StopBitsSet = TRUE;

 /* success */
 Dcb->StopBits = nValue;
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
  /* failure */
  return FALSE;

 /* for BuildCommDCB(), Timeouts is NULL */
 if(Timeouts)
 {
  /* why? no idea. All values taken from Windows 2000 with experimentation */
  Timeouts->ReadIntervalTimeout = 0;
  Timeouts->ReadTotalTimeoutMultiplier = 0;
  Timeouts->ReadTotalTimeoutConstant = 0;
  Timeouts->WriteTotalTimeoutMultiplier = 0;

  /* timeout */
  if(bValue) Timeouts->WriteTotalTimeoutConstant = 60000;
  /* no timeout */
  else Timeouts->WriteTotalTimeoutConstant = 0;
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
  /* failure */
  return FALSE;

 /* XON/XOFF */
 if(bValue) Dcb->fInX = Dcb->fOutX = TRUE;
 /* no XON/XOFF */
 else Dcb->fInX = Dcb->fOutX = FALSE;

 /* success */
 return TRUE;
}

/* FUNCTIONS */
#define COMMDCB_PARAM(__P__) \
 { \
  UNICODE_STRING_INITIALIZER(_L(#__P__)), \
  (ULONG_PTR)&COMMDCB_ ## __P__ ## Param \
 }

WINBOOL
STDCALL
BuildCommDCBAndTimeoutsW
(
 LPCWSTR lpDef,
 LPDCB lpDCB,
 LPCOMMTIMEOUTS lpCommTimeouts
)
{
 /* tell the baud= handler that the stop bits should be defaulted */
 BOOL bStopBitsSet = FALSE;

 /* parameter validation */
 if(lpDCB->DCBlength != sizeof(DCB)) goto InvalidParam;

 /* set defaults */
 lpDCB->StopBits = ONESTOPBIT;

 /*
  The documentation for MODE says that data= defaults to 7, but BuildCommDCB
  doesn't seem to set it
 */
 /* lpDCB->ByteSize = 7; */

 /* skip COMx[n] */
 if
 (
  lpDef[0] &&
  towupper(lpDef[0]) == L'C' &&
  lpDef[1] &&
  towupper(lpDef[1]) == L'O' &&
  lpDef[2] &&
  towupper(lpDef[2]) == L'M'
 )
 {
  DWORD nDummy;

  /* skip "COM" */
  lpDef += 3;

  /* premature end of string */
  if(!lpDef[0]) goto InvalidParam;

  /* skip "x" */
  if(!COMMDCB_ParseInt((LPWSTR *)&lpDef, &nDummy)) goto InvalidParam;

  /* skip ":" */
  if(lpDef[0] == L':') ++ lpDef;
 }

 /* skip leading whitespace */
 while(lpDef[0] && iswspace(lpDef[0])) ++ lpDef;

 /* repeat until the end of the string */
 while(lpDef[0])
 {
  static COMMDCB_PARAM_STRFLAG a_Params[] =
  {
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
  while(lpDef[0] && lpDef[0] != L'=') ++ lpDef;

  /* premature end of string */
  if(!lpDef[0]) goto InvalidParam;

  /* build the parameter's UNICODE_STRING */
  wstrParam.Buffer = pwcPrevTail;
  wstrParam.Length = (lpDef - pwcPrevTail) * sizeof(WCHAR);
  wstrParam.MaximumLength = wstrParam.Length;

  /* skip the "=" */
  ++ lpDef;

  /* lookup the callback for the parameter */
  bRetVal = COMMDCB_LookupStrFlag
  (
   &wstrParam,
   a_Params,
   sizeof(a_Params) / sizeof(a_Params[0]),
   (ULONG_PTR *)&pCallback
  );

  /* invalid parameter */
  if(!bRetVal) goto InvalidParam;

  /* call the callback to parse the parameter's argument */
  if(!pCallback(lpDCB, lpCommTimeouts, &bStopBitsSet, (LPWSTR *)&lpDef))
   /* failure */
   goto InvalidParam;

  /* skip trailing whitespace */
  while(lpDef[0] && iswspace(lpDef[0])) ++ lpDef;
 }

 /* success */
 return TRUE;

InvalidParam:
 /* failure */
 SetLastError(ERROR_INVALID_PARAMETER);
 return FALSE;
}


WINBOOL
STDCALL
BuildCommDCBAndTimeoutsA(LPCSTR lpDef, LPDCB lpDCB,	LPCOMMTIMEOUTS lpCommTimeouts)
{
 NTSTATUS nErrCode;
 WINBOOL bRetVal;
 ANSI_STRING strDef;
 UNICODE_STRING wstrDef;

 RtlInitAnsiString(&strDef, (LPSTR)lpDef);
 
 nErrCode = RtlAnsiStringToUnicodeString(&wstrDef, &strDef, TRUE);

 if(!NT_SUCCESS(nErrCode))
 {
  SetLastErrorByStatus(nErrCode);
  return FALSE;
 }
 
 bRetVal = BuildCommDCBAndTimeoutsW(wstrDef.Buffer, lpDCB, lpCommTimeouts);

 RtlFreeUnicodeString(&wstrDef);
 
 return bRetVal;
}

WINBOOL
STDCALL
BuildCommDCBA(LPCSTR lpDef, LPDCB lpDCB)
{
 return BuildCommDCBAndTimeoutsA(lpDef, lpDCB, NULL);
}

WINBOOL
STDCALL
BuildCommDCBW(LPCWSTR lpDef, LPDCB lpDCB)
{
 return BuildCommDCBAndTimeoutsW(lpDef, lpDCB, NULL);
}

WINBOOL
STDCALL
ClearCommBreak(HANDLE hFile)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;

	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
    result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_BREAK_OFF, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
	return TRUE;
}

WINBOOL
STDCALL
ClearCommError(HANDLE hFile, LPDWORD lpErrors, LPCOMSTAT lpStat)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;

	if (hFile == INVALID_HANDLE_VALUE) {
		//SetLastError(CE_MODE);
		return FALSE;
	}
	if (lpErrors == NULL) {
        DPRINT("ERROR: GetCommState() - NULL Errors pointer\n");
		return FALSE;
	}
//	*lpErrors = CE_BREAK;
//	*lpErrors = CE_FRAME;
//	*lpErrors = CE_IOE;
//	*lpErrors = CE_MODE;
//	*lpErrors = CE_OVERRUN;
//	*lpErrors = CE_RXOVER;
//	*lpErrors = CE_RXPARITY;
//	*lpErrors = CE_TXFULL;
/*
CE_BREAK The hardware detected a break condition. 
CE_FRAME The hardware detected a framing error. 
CE_IOE An I/O error occurred during communications with the device. 
CE_MODE The requested mode is not supported, or the hFile parameter is invalid. If this value is specified, it is the only valid error. 
CE_OVERRUN A character-buffer overrun has occurred. The next character is lost. 
CE_RXOVER An input buffer overflow has occurred. There is either no room in the input buffer, or a character was received after the end-of-file (EOF) character. 
CE_RXPARITY The hardware detected a parity error. 
CE_TXFULL The application tried to transmit a character, but the output buffer was full. 
 */
    result = DeviceIoControl(hFile, IOCTL_SERIAL_RESET_DEVICE, NULL, 0, NULL, 0, &dwBytesReturned, NULL);

	if (lpStat != NULL) {
		lpStat->fCtsHold = 0;
		lpStat->fDsrHold = 0;
		lpStat->fRlsdHold = 0;
		lpStat->fXoffHold = 0;
		lpStat->fXoffSent = 0;
		lpStat->fEof = 0;
		lpStat->fTxim = 0;
		lpStat->cbInQue = 0;
		lpStat->cbOutQue = 0;
	}
	return TRUE;
}

WINBOOL
STDCALL
CommConfigDialogA(LPCSTR lpszName, HWND hWnd, LPCOMMCONFIG lpCC)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
CommConfigDialogW(LPCWSTR lpszName, HWND hWnd, LPCOMMCONFIG lpCC)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
EscapeCommFunction(HANDLE hFile, DWORD dwFunc)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;

	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
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
	return TRUE;
}

WINBOOL
STDCALL
GetCommConfig(HANDLE hCommDev, LPCOMMCONFIG lpCC, LPDWORD lpdwSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
GetCommMask(HANDLE hFile, LPDWORD lpEvtMask)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;

	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
    result = DeviceIoControl(hFile, IOCTL_SERIAL_GET_WAIT_MASK, 
		NULL, 0, lpEvtMask, sizeof(DWORD), &dwBytesReturned, NULL);
	return TRUE;
}

WINBOOL
STDCALL
GetCommModemStatus(HANDLE hFile, LPDWORD lpModemStat)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;

	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	result = DeviceIoControl(hFile, IOCTL_SERIAL_GET_MODEMSTATUS,
		NULL, 0, lpModemStat, sizeof(DWORD), &dwBytesReturned, NULL);
	return TRUE;
}

WINBOOL
STDCALL
GetCommProperties(HANDLE hFile, LPCOMMPROP lpCommProp)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
GetCommState(HANDLE hFile, LPDCB lpDCB)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;

	SERIAL_BAUD_RATE BaudRate;
	SERIAL_HANDFLOW HandFlow;
	SERIAL_CHARS SpecialChars;
	SERIAL_LINE_CONTROL LineControl;

    DPRINT("GetCommState(%d, %p)\n", hFile, lpDCB);

	if (hFile == INVALID_HANDLE_VALUE) {
        DPRINT("ERROR: GetCommState() - INVALID_HANDLE_VALUE\n");
		return FALSE;
	}
	if (lpDCB == NULL) {
        DPRINT("ERROR: GetCommState() - NULL DCB pointer\n");
		return FALSE;
	}
	if (lpDCB->DCBlength != sizeof(DCB)) {
        DPRINT("ERROR: GetCommState() - Invalid DCB size\n");
		return FALSE;
	}

//    DPRINT("    GetCommState() CALLING DeviceIoControl\n");
//    result = DeviceIoControl(hFile, IOCTL_SERIAL_GET_COMMSTATUS, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
//    DPRINT("    GetCommState() DeviceIoControl returned %d\n", result);

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
    lpDCB->XonLim = HandFlow.XonLimit;
    lpDCB->XoffLim = HandFlow.XoffLimit;

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

WINBOOL
STDCALL
GetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;

	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	if (lpCommTimeouts == NULL) {
		return FALSE;
	}
	result = DeviceIoControl(hFile, IOCTL_SERIAL_GET_TIMEOUTS,
							 NULL, 0, 
							 lpCommTimeouts, sizeof(COMMTIMEOUTS), 
							 &dwBytesReturned, NULL);
	return TRUE;
}

WINBOOL
STDCALL
GetDefaultCommConfigW(LPCWSTR lpszName, LPCOMMCONFIG lpCC, LPDWORD lpdwSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
GetDefaultCommConfigA(LPCSTR lpszName, LPCOMMCONFIG lpCC, LPDWORD lpdwSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
PurgeComm(HANDLE hFile, DWORD dwFlags)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;

	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
    result = DeviceIoControl(hFile, IOCTL_SERIAL_PURGE, 
		&dwFlags, sizeof(DWORD), NULL, 0, &dwBytesReturned, NULL);
	return TRUE;
}

WINBOOL
STDCALL
SetCommBreak(HANDLE hFile)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;

	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
    result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_BREAK_ON, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
	return TRUE;
}

WINBOOL
STDCALL
SetCommConfig(HANDLE hCommDev, LPCOMMCONFIG lpCC, DWORD dwSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
SetCommMask(HANDLE hFile, DWORD dwEvtMask)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;

	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
    result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_WAIT_MASK, 
		&dwEvtMask, sizeof(DWORD), NULL, 0, &dwBytesReturned, NULL);
	return TRUE;
}

WINBOOL
STDCALL
SetCommState(HANDLE	hFile, LPDCB lpDCB)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;

	SERIAL_BAUD_RATE BaudRate;
	SERIAL_HANDFLOW HandFlow;
	SERIAL_CHARS SpecialChars;
	SERIAL_LINE_CONTROL LineControl;

    DPRINT("SetCommState(%d, %p) - ENTERED\n", hFile, lpDCB);

	if (hFile == INVALID_HANDLE_VALUE) {
        DPRINT("SetCommState() - ERROR: INVALID_HANDLE_VALUE\n");
		return FALSE;
	}
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

WINBOOL
STDCALL
SetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;
	SERIAL_TIMEOUTS Timeouts;

	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	if (lpCommTimeouts == NULL) {
		return FALSE;
	}
	Timeouts.ReadIntervalTimeout = lpCommTimeouts->ReadIntervalTimeout;
	Timeouts.ReadTotalTimeoutMultiplier = lpCommTimeouts->ReadTotalTimeoutMultiplier;
	Timeouts.ReadTotalTimeoutConstant = lpCommTimeouts->ReadTotalTimeoutConstant;
	Timeouts.WriteTotalTimeoutMultiplier = lpCommTimeouts->WriteTotalTimeoutMultiplier;
	Timeouts.WriteTotalTimeoutConstant = lpCommTimeouts->WriteTotalTimeoutConstant;
	result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_TIMEOUTS,
		&Timeouts, sizeof(Timeouts), NULL, 0, &dwBytesReturned, NULL);
	return TRUE;
}

WINBOOL
STDCALL
SetDefaultCommConfigA(LPCSTR lpszName, LPCOMMCONFIG lpCC, DWORD dwSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
SetDefaultCommConfigW(LPCWSTR lpszName, LPCOMMCONFIG lpCC, DWORD dwSize)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
SetupComm(HANDLE hFile, DWORD dwInQueue, DWORD dwOutQueue)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;
	SERIAL_QUEUE_SIZE QueueSize;

	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
    QueueSize.InSize = dwInQueue;
    QueueSize.OutSize = dwOutQueue;
	result = DeviceIoControl(hFile, IOCTL_SERIAL_SET_QUEUE_SIZE,
		&QueueSize, sizeof(QueueSize), NULL, 0, &dwBytesReturned, NULL);
	return TRUE;
}

WINBOOL
STDCALL
TransmitCommChar(HANDLE hFile, char cChar)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;

	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	result = DeviceIoControl(hFile, IOCTL_SERIAL_IMMEDIATE_CHAR,
		&cChar, sizeof(cChar), NULL, 0, &dwBytesReturned, NULL);
	return TRUE;
}

WINBOOL
STDCALL
WaitCommEvent(HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped)
{
	WINBOOL result = FALSE;
	DWORD dwBytesReturned;

	if (hFile == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	if (lpEvtMask == NULL) {
		return FALSE;
	}
	result = DeviceIoControl(hFile, IOCTL_SERIAL_WAIT_ON_MASK,
		NULL, 0, lpEvtMask, sizeof(DWORD), &dwBytesReturned, lpOverlapped);
	return TRUE;
}

/* EOF */
