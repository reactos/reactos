/*
 * Copyright (C) 2005 Mike McCormack
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

#ifndef _CFGMGR32_H_
#define _CFGMGR32_H_

typedef DWORD CONFIGRET;
typedef HANDLE HMACHINE;
typedef HMACHINE *PHMACHINE;

#define CR_SUCCESS       0x00000000
#define CR_INVALID_DATA  0x0000001F
#define CR_ACCESS_DENIED 0x00000033


CONFIGRET WINAPI CM_Connect_MachineA( PCSTR, PHMACHINE );
CONFIGRET WINAPI CM_Connect_MachineW( PCWSTR, PHMACHINE );
#define     CM_Connect_Machine WINELIB_NAME_AW(CM_Connect_Machine)

CONFIGRET WINAPI CM_Disconnect_Machine( HMACHINE );

CONFIGRET WINAPI CM_Get_Device_ID_ListA( PCSTR, PCHAR, ULONG, ULONG );
CONFIGRET WINAPI CM_Get_Device_ID_ListW( PCWSTR, PWCHAR, ULONG, ULONG );
#define     CM_Get_Device_ID_List WINELIB_NAME_AW(CM_Get_Device_ID_List)
CONFIGRET WINAPI CM_Get_Device_ID_List_ExA( PCSTR, PCHAR, ULONG, ULONG, HMACHINE );
CONFIGRET WINAPI CM_Get_Device_ID_List_ExW( PCWSTR, PWCHAR, ULONG, ULONG, HMACHINE );
#define     CM_Get_Device_ID_List_Ex WINELIB_NAME_AW(CM_Get_Device_ID_List_Ex)
CONFIGRET WINAPI CM_Get_Device_ID_List_SizeA( PULONG, PCSTR, ULONG );
CONFIGRET WINAPI CM_Get_Device_ID_List_SizeW( PULONG, PCWSTR, ULONG );
#define     CM_Get_Device_ID_List_Size WINELIB_NAME_AW(CM_Get_Device_ID_List_Size)
CONFIGRET WINAPI CM_Get_Device_ID_List_Size_ExA( PULONG, PCSTR, ULONG, HMACHINE );
CONFIGRET WINAPI CM_Get_Device_ID_List_Size_ExW( PULONG, PCWSTR, ULONG, HMACHINE );
#define     CM_Get_Device_ID_List_Size_Ex WINELIB_NAME_AW(CM_Get_Device_ID_List_Size_Ex)

#endif /* _CFGMGR32_H_ */
