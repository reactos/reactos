/*
 *  FreeLoader - rs232.c
 *
 *  Copyright (C) 2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <stdio.h>

#include "rs232.h"


HANDLE	hPortHandle = NULL;


BOOL Rs232OpenPortWin32(TCHAR* CommPort)
{
	TCHAR	PortName[MAX_PATH];
	DWORD	ErrorCode;

	// First check and make sure they don't already have the
	// OBD2 connection open. We don't want to open things twice.
	if (hPortHandle != NULL)
	{
		_tprintf(TEXT("Port handle not NULL. Must be already open. Returning FALSE...\n"));
		return FALSE;
	}

	_stprintf(PortName, TEXT("\\\\.\\%s"), CommPort);

	hPortHandle = CreateFile(PortName,
							GENERIC_READ|GENERIC_WRITE,
							0,
							0,
							OPEN_EXISTING,
							0,
							0);

	if (hPortHandle == INVALID_HANDLE_VALUE)
	{
		hPortHandle = NULL;
		ErrorCode = GetLastError();

		_tprintf(TEXT("CreateFile(\"%s\") failed. GetLastError() = %lu.\n"), PortName, ErrorCode);

		return FALSE;
	}

	return TRUE;
}

BOOL Rs232ClosePortWin32(VOID)
{
	HANDLE	hTempPortHandle = hPortHandle;

	hPortHandle = NULL;

	if (hTempPortHandle == NULL)
	{
		return FALSE;
	}

	return CloseHandle(hTempPortHandle);
}

// DeviceControlString
//   [in] Pointer to a null-terminated string that specifies device-control information.
//   The string must have the same form as the mode command's command-line arguments.
//
//   For example, the following string specifies a baud rate of 1200, no parity, 8 data bits, and 1 stop bit: 
//   "baud=1200 parity=N data=8 stop=1"
//
//   The following string specifies a baud rate of 115200, no parity, 8 data bits, and 1 stop bit:
//   "115200,n,8,1"
//
//   The device name is ignored if it is included in the string, but it must specify a valid device, as follows: 
//   "COM1: baud=1200 parity=N data=8 stop=1"
//
//   For further information on mode command syntax, refer to the end-user documentation for your operating system. 
BOOL Rs232ConfigurePortWin32(TCHAR* DeviceControlString)
{
	DCB		dcb;
	DWORD	ErrorCode;

	/*if (!GetCommState(hPortHandle, &dcb))
	{
		ErrorCode = GetLastError();

		_tprintf(TEXT("GetCommState() failed. GetLastError() = %lu.\n"), ErrorCode);

		return FALSE;
	}

	dcb.BaudRate = BaudRate;
	dcb.ByteSize = DataBits;
	dcb.Parity = Parity;
	dcb.StopBits = StopBits;
	dcb.fBinary = TRUE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fParity = (Parity == NOPARITY) ? FALSE : TRUE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fNull = FALSE;
	dcb.fAbortOnError = TRUE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = FALSE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxCtsFlow = FALSE;*/


	memset(&dcb, 0, sizeof(DCB));
	dcb.DCBlength = sizeof(dcb);
	if (!BuildCommDCB(DeviceControlString, &dcb))
	{
		ErrorCode = GetLastError();

		_tprintf(TEXT("BuildCommDCB() failed. GetLastError() = %lu.\n"), ErrorCode);

		return FALSE;
	}

	if (!SetCommState(hPortHandle, &dcb))
	{
		ErrorCode = GetLastError();

		_tprintf(TEXT("SetCommState() failed. GetLastError() = %lu.\n"), ErrorCode);

		return FALSE;
	}

	// Set the timeouts
	if (!Rs232SetCommunicationTimeoutsWin32(MAXDWORD, MAXDWORD, 1000, 0, 0))
	{
		return FALSE;
	}

	return TRUE;
}

// Members
//  ReadIntervalTimeout 
//   Specifies the maximum time, in milliseconds, allowed to elapse between the arrival of two characters on the communications line. During a ReadFile operation, the time period begins when the first character is received. If the interval between the arrival of any two characters exceeds this amount, the ReadFile operation is completed and any buffered data is returned. A value of zero indicates that interval time-outs are not used. 
//   A value of MAXDWORD, combined with zero values for both the ReadTotalTimeoutConstant and ReadTotalTimeoutMultiplier members, specifies that the read operation is to return immediately with the characters that have already been received, even if no characters have been received. 
// 
//  ReadTotalTimeoutMultiplier 
//   Specifies the multiplier, in milliseconds, used to calculate the total time-out period for read operations. For each read operation, this value is multiplied by the requested number of bytes to be read. 
//  ReadTotalTimeoutConstant 
//   Specifies the constant, in milliseconds, used to calculate the total time-out period for read operations. For each read operation, this value is added to the product of the ReadTotalTimeoutMultiplier member and the requested number of bytes. 
//   A value of zero for both the ReadTotalTimeoutMultiplier and ReadTotalTimeoutConstant members indicates that total time-outs are not used for read operations. 
// 
//  WriteTotalTimeoutMultiplier 
//   Specifies the multiplier, in milliseconds, used to calculate the total time-out period for write operations. For each write operation, this value is multiplied by the number of bytes to be written. 
//  WriteTotalTimeoutConstant 
//   Specifies the constant, in milliseconds, used to calculate the total time-out period for write operations. For each write operation, this value is added to the product of the WriteTotalTimeoutMultiplier member and the number of bytes to be written. 
//   A value of zero for both the WriteTotalTimeoutMultiplier and WriteTotalTimeoutConstant members indicates that total time-outs are not used for write operations. 
// 
// Remarks
//  If an application sets ReadIntervalTimeout and ReadTotalTimeoutMultiplier to MAXDWORD and sets ReadTotalTimeoutConstant to a value greater than zero and less than MAXDWORD, one of the following occurs when the ReadFile function is called: 
// 
//   If there are any characters in the input buffer, ReadFile returns immediately with the characters in the buffer. 
//   If there are no characters in the input buffer, ReadFile waits until a character arrives and then returns immediately. 
//   If no character arrives within the time specified by ReadTotalTimeoutConstant, ReadFile times out. 
BOOL Rs232SetCommunicationTimeoutsWin32(DWORD ReadIntervalTimeout, DWORD ReadTotalTimeoutMultiplier, DWORD ReadTotalTimeoutConstant, DWORD WriteTotalTimeoutMultiplier, DWORD WriteTotalTimeoutConstant)
{
	COMMTIMEOUTS	ct;
	DWORD			ErrorCode;

	if (!GetCommTimeouts(hPortHandle, &ct))
	{
		ErrorCode = GetLastError();

		_tprintf(TEXT("GetCommTimeouts() failed. GetLastError() = %lu.\n"), ErrorCode);

		return FALSE;
	}

	ct.ReadIntervalTimeout = ReadIntervalTimeout;
	ct.ReadTotalTimeoutConstant = ReadTotalTimeoutConstant;
	ct.ReadTotalTimeoutMultiplier = ReadTotalTimeoutMultiplier;
	ct.WriteTotalTimeoutConstant = WriteTotalTimeoutConstant;
	ct.WriteTotalTimeoutMultiplier = WriteTotalTimeoutMultiplier;

	if (!SetCommTimeouts(hPortHandle, &ct))
	{
		ErrorCode = GetLastError();

		_tprintf(TEXT("SetCommTimeouts() failed. GetLastError() = %lu.\n"), ErrorCode);

		return FALSE;
	}

	return TRUE;
}

BOOL Rs232ReadByteWin32(BYTE* DataByte)
{
	DWORD	BytesRead = 0;
	DWORD	ErrorCode;

	// If ReadFile() fails then report error
	if (!ReadFile(hPortHandle, DataByte, 1, &BytesRead, NULL))
	{
		ErrorCode = GetLastError();

		_tprintf(TEXT("ReadFile() failed. GetLastError() = %lu.\n"), ErrorCode);

		return FALSE;
	}

	// If ReadFile() succeeds, but BytesRead isn't 1
	// then a timeout occurred.
	if (BytesRead != 1)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL Rs232WriteByteWin32(BYTE DataByte)
{
	DWORD	BytesWritten = 0;
	BOOL	Success;
	DWORD	ErrorCode;

	Success = WriteFile(hPortHandle, &DataByte, 1, &BytesWritten, NULL);

	if (!Success || BytesWritten != 1)
	{
		ErrorCode = GetLastError();

		_tprintf(TEXT("WriteFile() failed. GetLastError() = %lu.\n"), ErrorCode);

		return FALSE;
	}

	return TRUE;
}
