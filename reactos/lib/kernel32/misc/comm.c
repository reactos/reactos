/* $Id: comm.c,v 1.5 2002/10/03 18:26:53 robd Exp $
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
 */

#include <ddk/ntddk.h>
#include <ddk/ntddser.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wchar.h>
#include <string.h>

//#define NDEBUG
#define DBG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>


WINBOOL
STDCALL
BuildCommDCBA(LPCSTR lpDef, LPDCB lpDCB)
{
	if (lpDCB == NULL) {
        DPRINT("ERROR: BuildCommDCBA() - NULL DCB pointer\n");
		return FALSE;
	}
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
BuildCommDCBW(LPCWSTR lpDef, LPDCB lpDCB)
{
	if (lpDCB == NULL) {
        DPRINT("ERROR: BuildCommDCBW() - NULL DCB pointer\n");
		return FALSE;
	}
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
BuildCommDCBAndTimeoutsA(LPCSTR lpDef, LPDCB lpDCB,	LPCOMMTIMEOUTS lpCommTimeouts)
{
	if (lpDCB == NULL) {
        DPRINT("ERROR: BuildCommDCBAndTimeoutsA() - NULL DCB pointer\n");
		return FALSE;
	}
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
BuildCommDCBAndTimeoutsW(LPCWSTR lpDef, LPDCB lpDCB, LPCOMMTIMEOUTS lpCommTimeouts)
{
	if (lpDCB == NULL) {
        DPRINT("ERROR: BuildCommDCBAndTimeoutsW() - NULL DCB pointer\n");
		return FALSE;
	}
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
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
