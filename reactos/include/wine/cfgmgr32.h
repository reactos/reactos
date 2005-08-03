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

/* cfgmgr32 doesn't use the normal convention, it adds an underscore before A/W */
#ifdef __WINESRC__
# define DECL_WINELIB_CFGMGR32_TYPE_AW(type)  /* nothing */
#else   /* __WINESRC__ */
# define DECL_WINELIB_CFGMGR32_TYPE_AW(type)  typedef WINELIB_NAME_AW(type##_) type;
#endif  /* __WINESRC__ */

typedef DWORD CONFIGRET;
typedef HANDLE HMACHINE;
typedef HMACHINE *PHMACHINE;
typedef DWORD DEVINST;
typedef DEVINST *PDEVINST;
typedef ULONG REGDISPOSITION;

typedef CHAR  *DEVINSTID_A;
typedef WCHAR *DEVINSTID_W;
DECL_WINELIB_CFGMGR32_TYPE_AW(DEVINSTID)

#define CR_SUCCESS              0x00000000
#define CR_OUT_OF_MEMORY        0x00000002
#define CR_INVALID_POINTER      0x00000003
#define CR_INVALID_FLAG         0x00000004
#define CR_INVALID_DEVNODE      0x00000005
#define CR_INVALID_DEVINST      CR_INVALID_DEVNODE
#define CR_NO_SUCH_DEVNODE      0x0000000D
#define CR_NO_SUCH_DEVINST      CR_NO_SUCH_DEVNODE
#define CR_FAILURE              0x00000013
#define CR_BUFFER_SMALL         0x0000001A
#define CR_REGISTRY_ERROR       0x0000001D
#define CR_INVALID_DEVICE_ID    0x0000001E
#define CR_INVALID_DATA         0x0000001F
#define CR_NO_SUCH_VALUE        0x00000025
#define CR_NO_SUCH_REGISTRY_KEY 0x0000002E
#define CR_INVALID_MACHINENAME  0x0000002F
#define CR_ACCESS_DENIED        0x00000033
#define CR_CALL_NOT_IMPLEMENTED 0x00000034
#define CR_INVALID_PROPERTY     0x00000035

#define MAX_CLASS_NAME_LEN  32
#define MAX_GUID_STRING_LEN 39
#define MAX_PROFILE_LEN     80
#define MAX_DEVICE_ID_LEN      200
#define MAX_DEVNODE_ID_LEN     MAX_DEVICE_ID_LEN

/* Disposition values for CM_Open_Class_Key[_Ex] */
#define RegDisposition_OpenAlways   0x00000000
#define RegDisposition_OpenExisting 0x00000001
#define RegDisposition_Bits         0x00000001

/* ulFlags for CM_Open_Class_Key[_Ex] */
#define CM_OPEN_CLASS_KEY_INSTALLER 0x00000000
#define CM_OPEN_CLASS_KEY_INTERFACE 0x00000001
#define CM_OPEN_CLASS_KEY_BITS      0x00000001

/* ulFlags for CM_Locate_DevNode[_Ex] */
#define CM_LOCATE_DEVNODE_NORMAL       0x00000000
#define CM_LOCATE_DEVNODE_PHANTOM      0x00000001
#define CM_LOCATE_DEVNODE_CANCELREMOVE 0x00000002
#define CM_LOCATE_DEVNODE_NOVALIDATION 0x00000004
#define CM_LOCATE_DEVNODE_BITS         0x00000007

#define CM_LOCATE_DEVINST_NORMAL       CM_LOCATE_DEVNODE_NORMAL
#define CM_LOCATE_DEVINST_PHANTOM      CM_LOCATE_DEVNODE_PHANTOM
#define CM_LOCATE_DEVINST_CANCELREMOVE CM_LOCATE_DEVNODE_CANCELREMOVE
#define CM_LOCATE_DEVINST_NOVALIDATION CM_LOCATE_DEVNODE_NOVALIDATION
#define CM_LOCATE_DEVINST_BITS         CM_LOCATE_DEVNODE_BITS

/* ulFlags for CM_Set_DevNode_Problem[_Ex] */
#define CM_SET_DEVNODE_PROBLEM_NORMAL   0x00000000
#define CM_SET_DEVNODE_PROBLEM_OVERRIDE 0x00000001
#define CM_SET_DEVNODE_PROBLEM_BITS     0x00000001

#define CM_SET_DEVINST_PROBLEM_NORMAL   CM_SET_DEVNODE_PROBLEM_NORMAL
#define CM_SET_DEVINST_PROBLEM_OVERRIDE CM_SET_DEVNODE_PROBLEM_OVERRIDE
#define CM_SET_DEVINST_PROBLEM_BITS     CM_SET_DEVNODE_PROBLEM_BITS

/* Properties for CM_Get/Set_DevNode_Registry_Property[_Ex]A/W */
#define CM_DRP_DEVICEDESC                  0x00000001
#define CM_DRP_HARDWAREID                  0x00000002
#define CM_DRP_COMPATIBLEIDS               0x00000003
#define CM_DRP_UNUSED0                     0x00000004
#define CM_DRP_SERVICE                     0x00000005
#define CM_DRP_UNUSED1                     0x00000006
#define CM_DRP_UNUSED2                     0x00000007
#define CM_DRP_CLASS                       0x00000008
#define CM_DRP_CLASSGUID                   0x00000009
#define CM_DRP_DRIVER                      0x0000000A
#define CM_DRP_CONFIGFLAGS                 0x0000000B
#define CM_DRP_MFG                         0x0000000C
#define CM_DRP_FRIENDLYNAME                0x0000000D
#define CM_DRP_LOCATION_INFORMATION        0x0000000E
#define CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME 0x0000000F
#define CM_DRP_CAPABILITIES                0x00000010
#define CM_DRP_UI_NUMBER                   0x00000011
#define CM_DRP_UPPERFILTERS                0x00000012
#define CM_DRP_LOWERFILTERS                0x00000013
#define CM_DRP_BUSTYPEGUID                 0x00000014
#define CM_DRP_LEGACYBUSTYPE               0x00000015
#define CM_DRP_BUSNUMBER                   0x00000016
#define CM_DRP_ENUMERATOR_NAME             0x00000017

#define CM_DRP_MIN                         0x00000001
#define CM_DRP_MAX                         0x00000017



CONFIGRET WINAPI CM_Connect_MachineA( PCSTR, PHMACHINE );
CONFIGRET WINAPI CM_Connect_MachineW( PCWSTR, PHMACHINE );
#define     CM_Connect_Machine WINELIB_NAME_AW(CM_Connect_Machine)

CONFIGRET WINAPI CM_Disconnect_Machine( HMACHINE );
CONFIGRET WINAPI CM_Enumerate_Classes( ULONG, LPGUID, ULONG );
CONFIGRET WINAPI CM_Enumerate_Classes_Ex( ULONG, LPGUID, ULONG, HMACHINE );
CONFIGRET WINAPI CM_Get_Child( PDEVINST, DEVINST, ULONG );
CONFIGRET WINAPI CM_Get_Child_Ex( PDEVINST, DEVINST, ULONG, HMACHINE );
CONFIGRET WINAPI CM_Get_Depth( PULONG, DEVINST, ULONG );
CONFIGRET WINAPI CM_Get_Depth_Ex( PULONG, DEVINST, ULONG, HMACHINE );
CONFIGRET WINAPI CM_Get_DevNode_Registry_PropertyA( DEVINST, ULONG, PULONG, PVOID, PULONG, ULONG );
CONFIGRET WINAPI CM_Get_DevNode_Registry_PropertyW( DEVINST, ULONG, PULONG, PVOID, PULONG, ULONG );
#define     CM_Get_DevNode_Registry_Property WINELIB_NAME_AW(CM_Get_DevNode_Registry_Property)
CONFIGRET WINAPI CM_Get_DevNode_Registry_Property_ExA( DEVINST, ULONG, PULONG, PVOID, PULONG, ULONG, HMACHINE );
CONFIGRET WINAPI CM_Get_DevNode_Registry_Property_ExW( DEVINST, ULONG, PULONG, PVOID, PULONG, ULONG, HMACHINE );
#define     CM_Get_DevNode_Registry_Property_Ex WINELIB_NAME_AW(CM_Get_DevNode_Registry_Property_Ex)
CONFIGRET WINAPI CM_Get_DevNode_Status( PULONG, PULONG, DEVINST, ULONG );
CONFIGRET WINAPI CM_Get_DevNode_Status_Ex( PULONG, PULONG, DEVINST, ULONG, HMACHINE );
CONFIGRET WINAPI CM_Get_Device_IDA( DEVINST, PCHAR, ULONG, ULONG );
CONFIGRET WINAPI CM_Get_Device_IDW( DEVINST, PWCHAR, ULONG, ULONG );
#define     CM_Get_Device_ID WINELIB_NAME_AW(CM_Get_Device_ID)
CONFIGRET WINAPI CM_Get_Device_ID_ExW( DEVINST, PWCHAR, ULONG, ULONG, HMACHINE );
CONFIGRET WINAPI CM_Get_Device_ID_ExA( DEVINST, PCHAR, ULONG, ULONG, HMACHINE );
#define     CM_Get_Device_ID_Ex WINELIB_NAME_AW(CM_Get_Device_ID_Ex)
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
CONFIGRET WINAPI CM_Get_Device_ID_Size( PULONG, DEVINST, ULONG );
CONFIGRET WINAPI CM_Get_Device_ID_Size_Ex( PULONG, DEVINST, ULONG, HMACHINE );
CONFIGRET WINAPI CM_Get_Global_State( PULONG, ULONG );
CONFIGRET WINAPI CM_Get_Global_State_Ex( PULONG, ULONG, HMACHINE );
CONFIGRET WINAPI CM_Get_Parent( PDEVINST, DEVINST, ULONG );
CONFIGRET WINAPI CM_Get_Parent_Ex( PDEVINST, DEVINST, ULONG, HMACHINE );
CONFIGRET WINAPI CM_Get_Sibling( PDEVINST, DEVINST, ULONG );
CONFIGRET WINAPI CM_Get_Sibling_Ex( PDEVINST, DEVINST, ULONG, HMACHINE );
WORD WINAPI CM_Get_Version( VOID );
WORD WINAPI CM_Get_Version_Ex( HMACHINE );

CONFIGRET WINAPI CM_Locate_DevNodeA(PDEVINST, DEVINSTID_A, ULONG);
CONFIGRET WINAPI CM_Locate_DevNodeW(PDEVINST, DEVINSTID_W, ULONG);
#define     CM_Locate_DevNode WINELIB_NAME_AW(CM_Locate_DevNode)
CONFIGRET WINAPI CM_Locate_DevNode_ExA(PDEVINST, DEVINSTID_A, ULONG, HMACHINE);
CONFIGRET WINAPI CM_Locate_DevNode_ExW(PDEVINST, DEVINSTID_W, ULONG, HMACHINE);
#define     CM_Locate_DevNode_Ex WINELIB_NAME_AW(CM_Locate_DevNode_Ex)

CONFIGRET WINAPI CM_Open_Class_KeyA(LPGUID, LPCSTR, REGSAM, REGDISPOSITION, PHKEY, ULONG);
CONFIGRET WINAPI CM_Open_Class_KeyW(LPGUID, LPCWSTR, REGSAM, REGDISPOSITION, PHKEY, ULONG);
#define     CM_Open_Class_Key WINELIB_NAME_AW(CM_Open_Class_Key)
CONFIGRET WINAPI CM_Open_Class_Key_ExA(LPGUID, LPCSTR, REGSAM, REGDISPOSITION, PHKEY, ULONG, HMACHINE);
CONFIGRET WINAPI CM_Open_Class_Key_ExW(LPGUID, LPCWSTR, REGSAM, REGDISPOSITION, PHKEY, ULONG, HMACHINE);
#define     CM_Open_Class_Key_Ex WINELIB_NAME_AW(CM_Open_Class_Key_Ex)

CONFIGRET WINAPI CM_Set_DevNode_Problem( DEVINST, ULONG, ULONG );
CONFIGRET WINAPI CM_Set_DevNode_Problem_Ex( DEVINST, ULONG, ULONG, HMACHINE );
CONFIGRET WINAPI CM_Set_DevNode_Registry_PropertyA( DEVINST, ULONG, PCVOID, ULONG, ULONG );
CONFIGRET WINAPI CM_Set_DevNode_Registry_PropertyW( DEVINST, ULONG, PCVOID, ULONG, ULONG );
#define     CM_Set_DevNode_Registry_Property WINELIB_NAME_AW(CM_Set_DevNode_Registry_Property)
CONFIGRET WINAPI CM_Set_DevNode_Registry_Property_ExA( DEVINST, ULONG, PCVOID, ULONG, ULONG, HMACHINE );
CONFIGRET WINAPI CM_Set_DevNode_Registry_Property_ExW( DEVINST, ULONG, PCVOID, ULONG, ULONG, HMACHINE );
#define     CM_Set_DevNode_Registry_Property_Ex WINELIB_NAME_AW(CM_Set_DevNode_Registry_Property_Ex)

#endif /* _CFGMGR32_H_ */
