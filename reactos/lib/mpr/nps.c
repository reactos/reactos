/*
 * MPR Network Provider Services functions
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
#include "mpr.h"

/*****************************************************************
 *  NPSAuthenticationDialogA [MPR.@]
 */
DWORD WINAPI NPSAuthenticationDialogA( LPAUTHDLGSTRUCTA lpAuthDlgStruct )
{
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSGetProviderHandleA [MPR.@]
 */
DWORD WINAPI NPSGetProviderHandleA( PHPROVIDER phProvider )
{
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSGetProviderNameA [MPR.@]
 */
DWORD WINAPI NPSGetProviderNameA( HPROVIDER hProvider, LPCSTR *lpszProviderName )
{
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSGetSectionNameA [MPR.@]
 */
DWORD WINAPI NPSGetSectionNameA( HPROVIDER hProvider, LPCSTR *lpszSectionName )
{
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSSetExtendedErrorA [MPR.@]
 */
DWORD WINAPI NPSSetExtendedErrorA( DWORD NetSpecificError, LPSTR lpExtendedErrorText )
{
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSSetCustomTextA [MPR.@]
 */
VOID WINAPI NPSSetCustomTextA( LPSTR lpCustomErrorText )
{
}

/*****************************************************************
 *  NPSCopyStringA [MPR.@]
 */
DWORD WINAPI NPSCopyStringA( LPCSTR lpString, LPVOID lpBuffer, LPDWORD lpdwBufferSize )
{
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSDeviceGetNumberA [MPR.@]
 */
DWORD WINAPI NPSDeviceGetNumberA( LPSTR lpLocalName, LPDWORD lpdwNumber, LPDWORD lpdwType )
{
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSDeviceGetStringA [MPR.@]
 */
DWORD WINAPI NPSDeviceGetStringA( DWORD dwNumber, DWORD dwType, LPSTR lpLocalName, LPDWORD lpdwBufferSize )
{
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSNotifyRegisterA [MPR.@]
 */
DWORD WINAPI NPSNotifyRegisterA( enum NOTIFYTYPE NotifyType, NOTIFYCALLBACK pfNotifyCallBack )
{
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSNotifyGetContextA [MPR.@]
 */
LPVOID WINAPI NPSNotifyGetContextA( NOTIFYCALLBACK pfNotifyCallBack )
{
    return NULL;
}

