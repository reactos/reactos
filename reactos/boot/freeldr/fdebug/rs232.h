/*
 *  FreeLoader - rs232.h
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


#ifndef __RS232_H
#define __RS232_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//////////////////////////////////////////////////////////////////////////////////////////
//
// Function prototypes for RS232 communication under Win32
//
//////////////////////////////////////////////////////////////////////////////////////////
BOOL	Rs232OpenPortWin32(TCHAR* CommPort);
BOOL	Rs232ClosePortWin32(VOID);
BOOL	Rs232ConfigurePortWin32(TCHAR* DeviceControlString);
BOOL	Rs232SetCommunicationTimeoutsWin32(DWORD ReadIntervalTimeout, DWORD ReadTotalTimeoutMultiplier, DWORD ReadTotalTimeoutConstant, DWORD WriteTotalTimeoutMultiplier, DWORD WriteTotalTimeoutConstant);
BOOL	Rs232ReadByteWin32(BYTE* DataByte);
BOOL	Rs232WriteByteWin32(BYTE DataByte);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // !defined(__RS232_H)
