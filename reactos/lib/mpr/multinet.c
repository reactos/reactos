/*
 * MPR Multinet functions
 *
 * Copyright 1999 Ulrich Weigand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>


/*****************************************************************
 *     MultinetGetConnectionPerformanceA [MPR.@]
 *
 * RETURNS
 *    Success: NO_ERROR
 *    Failure: ERROR_NOT_SUPPORTED, ERROR_NOT_CONNECTED,
 *             ERROR_NO_NET_OR_BAD_PATH, ERROR_BAD_DEVICE,
 *             ERROR_BAD_NET_NAME, ERROR_INVALID_PARAMETER,
 *             ERROR_NO_NETWORK, ERROR_EXTENDED_ERROR
 */
DWORD WINAPI MultinetGetConnectionPerformanceA(
	LPNETRESOURCEA lpNetResource,
	LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct )
{
    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *     MultinetGetConnectionPerformanceW [MPR.@]
 */
DWORD WINAPI MultinetGetConnectionPerformanceW(
	LPNETRESOURCEW lpNetResource,
	LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct )
{
    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  MultinetGetErrorTextA [MPR.@]
 */
DWORD WINAPI MultinetGetErrorTextA( DWORD x, DWORD y, DWORD z )
{
      return 0;
}

/*****************************************************************
 *  MultinetGetErrorTextW [MPR.@]
 */
DWORD WINAPI MultinetGetErrorTextW( DWORD x, DWORD y, DWORD z )
{
    return 0;
}

