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
 *                  KJK (26/12/2008) reimplemented BuildCommDCB & BuildCommDCBAndTimeouts elsewhere
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
#undef IOCTL_SERIAL_LSRMST_INSERT
#include <ntddser.h>

#define NDEBUG
#include <debug.h>

static const WCHAR lpszSerialUI[] = {
   's','e','r','i','a','l','u','i','.','d','l','l',0 };

/*
 * @implemented
 */
BOOL
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
CommConfigDialogW(LPCWSTR lpszName, HWND hWnd, LPCOMMCONFIG lpCC)
{
	DWORD (WINAPI *drvCommDlgW)(LPCWSTR, HWND, LPCOMMCONFIG);
	HMODULE hSerialuiDll;
	DWORD result;

	//FIXME: Get dll name from registry. (setupapi needed)
	if(!(hSerialuiDll = LoadLibraryW(L"serialui.dll")))
	{
		DPRINT("CommConfigDialogW: serialui.dll not found.\n");
		return FALSE;
	}

	drvCommDlgW = (DWORD (WINAPI *)(LPCWSTR, HWND, LPCOMMCONFIG))
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
		SetLastError(ERROR_INVALID_PARAMETER);
        DPRINT("ERROR: GetCommState() - NULL DCB pointer\n");
		return FALSE;
	}

    if (!DeviceIoControl(hFile, IOCTL_SERIAL_GET_BAUD_RATE,
                         NULL, 0, &BaudRate, sizeof(BaudRate), &dwBytesReturned, NULL) ||
        !DeviceIoControl(hFile, IOCTL_SERIAL_GET_LINE_CONTROL,
                         NULL, 0, &LineControl, sizeof(LineControl), &dwBytesReturned, NULL) ||
        !DeviceIoControl(hFile, IOCTL_SERIAL_GET_HANDFLOW,
                         NULL, 0, &HandFlow, sizeof(HandFlow), &dwBytesReturned, NULL) ||
        !DeviceIoControl(hFile, IOCTL_SERIAL_GET_CHARS,
                         NULL, 0, &SpecialChars, sizeof(SpecialChars), &dwBytesReturned, NULL))
        return FALSE;

    memset(lpDCB, 0, sizeof(*lpDCB));
    lpDCB->DCBlength = sizeof(*lpDCB);

    lpDCB->fBinary = 1;
    lpDCB->fParity = 0;
    lpDCB->BaudRate = BaudRate.BaudRate;

    lpDCB->StopBits = LineControl.StopBits;
    lpDCB->Parity = LineControl.Parity;
    lpDCB->ByteSize = LineControl.WordLength;

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
WINAPI
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
 * @implemented
 */
BOOL
WINAPI
GetDefaultCommConfigW(LPCWSTR lpszName, LPCOMMCONFIG lpCC, LPDWORD lpdwSize)
{
    FARPROC pGetDefaultCommConfig;
    HMODULE hConfigModule;
    DWORD   res = ERROR_INVALID_PARAMETER;

    DPRINT("(%s, %p, %p)  *lpdwSize: %u\n", lpszName, lpCC, lpdwSize, lpdwSize ? *lpdwSize : 0 );
    hConfigModule = LoadLibraryW(lpszSerialUI);

    if (hConfigModule) {
        pGetDefaultCommConfig = GetProcAddress(hConfigModule, "drvGetDefaultCommConfigW");
        if (pGetDefaultCommConfig) {
            res = pGetDefaultCommConfig(lpszName, lpCC, lpdwSize);
        }
        FreeLibrary(hConfigModule);
    }

    if (res) SetLastError(res);
    return (res == ERROR_SUCCESS);
}


/*
 * @implemented
 */
BOOL
WINAPI
GetDefaultCommConfigA(LPCSTR lpszName, LPCOMMCONFIG lpCC, LPDWORD lpdwSize)
{
	BOOL ret = FALSE;
	UNICODE_STRING lpszNameW;

	DPRINT("(%s, %p, %p)  *lpdwSize: %u\n", lpszName, lpCC, lpdwSize, lpdwSize ? *lpdwSize : 0 );
	if(lpszName) RtlCreateUnicodeStringFromAsciiz(&lpszNameW,lpszName);
	else lpszNameW.Buffer = NULL;

	ret = GetDefaultCommConfigW(lpszNameW.Buffer,lpCC,lpdwSize);

	RtlFreeUnicodeString(&lpszNameW);
	return ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
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
WINAPI
SetCommBreak(HANDLE hFile)
{
	DWORD dwBytesReturned;

        return DeviceIoControl(hFile, IOCTL_SERIAL_SET_BREAK_ON, NULL, 0, NULL, 0, &dwBytesReturned, NULL);
}


/*
 * @implemented
 */
BOOL
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
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
 * @implemented
 */
BOOL
WINAPI
SetDefaultCommConfigA(LPCSTR lpszName, LPCOMMCONFIG lpCC, DWORD dwSize)
{
    BOOL r;
    LPWSTR lpDeviceW = NULL;
    DWORD len;

    DPRINT("(%s, %p, %u)\n", lpszName, lpCC, dwSize);

    if (lpszName)
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszName, -1, NULL, 0 );
        lpDeviceW = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszName, -1, lpDeviceW, len );
    }
    r = SetDefaultCommConfigW(lpDeviceW,lpCC,dwSize);
    HeapFree( GetProcessHeap(), 0, lpDeviceW );
    return r;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetDefaultCommConfigW(LPCWSTR lpszName, LPCOMMCONFIG lpCC, DWORD dwSize)
{
    FARPROC pGetDefaultCommConfig;
    HMODULE hConfigModule;
    DWORD   res = ERROR_INVALID_PARAMETER;

    DPRINT("(%s, %p, %p)  *dwSize: %u\n", lpszName, lpCC, dwSize, dwSize ? dwSize : 0 );
    hConfigModule = LoadLibraryW(lpszSerialUI);

    if (hConfigModule) {
        pGetDefaultCommConfig = GetProcAddress(hConfigModule, "drvGetDefaultCommConfigW");
        if (pGetDefaultCommConfig) {
            res = pGetDefaultCommConfig(lpszName, lpCC, &dwSize);
        }
        FreeLibrary(hConfigModule);
    }

    if (res) SetLastError(res);
    return (res == ERROR_SUCCESS);
}


/*
 * @implemented
 */
BOOL
WINAPI
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
WINAPI
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
WINAPI
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
