/*
 * Copyright (C) 2016 Austin English
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef __BLUETOOTHAPIS_H
#define __BLUETOOTHAPIS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef ULONGLONG BTH_ADDR;

typedef struct _BLUETOOTH_ADDRESS {
    union {
        BTH_ADDR ullLong;
        BYTE rgBytes[6];
    } DUMMYUNIONNAME;
} BLUETOOTH_ADDRESS_STRUCT;

#define BLUETOOTH_ADDRESS BLUETOOTH_ADDRESS_STRUCT
#define BLUETOOTH_NULL_ADDRESS ((ULONGLONG) 0x0)

#define BLUETOOTH_MAX_NAME_SIZE           (248)
#define BLUETOOTH_MAX_PASSKEY_SIZE        (16)
#define BLUETOOTH_MAX_PASSKEY_BUFFER_SIZE (BLUETOOTH_MAX_PASSKEY_SIZE + 1)

#define BLUETOOTH_SERVICE_DISABLE 0x00
#define BLUETOOTH_SERVICE_ENABLE  0x01
#define BLUETOOTH_SERVICE_MASK    (BLUETOOTH_ENABLE_SERVICE | BLUETOOTH_DISABLE_SERVICE)

typedef struct _BLUETOOTH_FIND_RADIO_PARAMS {
    DWORD dwSize;
} BLUETOOTH_FIND_RADIO_PARAMS;

typedef struct _BLUETOOTH_RADIO_INFO {
    DWORD dwSize;
    BLUETOOTH_ADDRESS address;
    WCHAR szName[BLUETOOTH_MAX_NAME_SIZE];
    ULONG ulClassofDevice;
    USHORT lmpSubversion;
    USHORT manufacturer;
} BLUETOOTH_RADIO_INFO, *PBLUETOOTH_RADIO_INFO;

typedef struct _BLUETOOTH_DEVICE_INFO {
    DWORD dwSize;
    BLUETOOTH_ADDRESS Address;
    ULONG ulClassofDevice;
    BOOL fConnected;
    BOOL fRemembered;
    BOOL fAuthenticated;
    SYSTEMTIME stLastSeen;
    SYSTEMTIME stLastUsed;
    WCHAR szName[BLUETOOTH_MAX_NAME_SIZE];
} BLUETOOTH_DEVICE_INFO, BLUETOOTH_DEVICE_INFO_STRUCT, *PBLUETOOTH_DEVICE_INFO;

typedef struct _BLUETOOTH_DEVICE_SEARCH_PARAMS {
    DWORD dwSize;
    BOOL fReturnAuthenticated;
    BOOL fReturnRemembered;
    BOOL fReturnUnknown;
    BOOL fReturnConnected;
    BOOL fIssueInquiry;
    UCHAR cTimeoutMultiplier;
    HANDLE hRadio;
} BLUETOOTH_DEVICE_SEARCH_PARAMS;

typedef HANDLE HBLUETOOTH_AUTHENTICATION_REGISTRATION;
typedef HANDLE HBLUETOOTH_CONTAINER_ELEMENT;
typedef HANDLE HBLUETOOTH_DEVICE_FIND;
typedef HANDLE HBLUETOOTH_RADIO_FIND;

typedef struct _BLUETOOTH_COD_PAIRS {
    ULONG ulCODMask;
    const WCHAR *pcszDescription;
} BLUETOOTH_COD_PAIRS;

typedef BOOL (WINAPI *PFN_DEVICE_CALLBACK)(void *pvParam, const BLUETOOTH_DEVICE_INFO *pDevice);

typedef struct _BLUETOOTH_SELECT_DEVICE_PARAMS {
    DWORD dwSize;
    ULONG cNumOfClasses;
    BLUETOOTH_COD_PAIRS *prgClassOfDevices;
    WCHAR *pszInfo;
    HWND hwndParent;
    BOOL fForceAuthentication;
    BOOL fShowAuthenticated;
    BOOL fShowRemembered;
    BOOL fShowUnknown;
    BOOL fAddNewDeviceWizard;
    BOOL fSkipServicesPage;
    PFN_DEVICE_CALLBACK pfnDeviceCallback;
    void *pvParam;
    DWORD cNumDevices;
    PBLUETOOTH_DEVICE_INFO pDevices;
} BLUETOOTH_SELECT_DEVICE_PARAMS;

typedef BOOL (WINAPI *PFN_AUTHENTICATION_CALLBACK)(void *, PBLUETOOTH_DEVICE_INFO);

#define BLUETOOTH_DEVICE_INFO BLUETOOTH_DEVICE_INFO_STRUCT

typedef BLUETOOTH_DEVICE_INFO *PBLUETOOTH_DEVICE_INFO;

typedef enum _BLUETOOTH_AUTHENTICATION_METHOD {
    BLUETOOTH_AUTHENTICATION_METHOD_LEGACY = 0x1,
    BLUETOOTH_AUTHENTICATION_METHOD_OOB,
    BLUETOOTH_AUTHENTICATION_METHOD_NUMERIC_COMPARISON,
    BLUETOOTH_AUTHENTICATION_METHOD_PASSKEY_NOTIFICATION,
    BLUETOOTH_AUTHENTICATION_METHOD_PASSKEY,
} BLUETOOTH_AUTHENTICATION_METHOD, *PBLUETOOTH_AUTHENTICATION_METHOD;

typedef enum _BLUETOOTH_IO_CAPABILITY {
    BLUETOOTH_IO_CAPABILITY_DISPLAYONLY = 0x00,
    BLUETOOTH_IO_CAPABILITY_DISPLAYYESNO = 0x01,
    BLUETOOTH_IO_CAPABILITY_KEYBOARDONLY = 0x02,
    BLUETOOTH_IO_CAPABILITY_NOINPUTNOOUTPUT = 0x03,
    BLUETOOTH_IO_CAPABILITY_UNDEFINED = 0xff,
} BLUETOOTH_IO_CAPABILITY;

typedef enum _BLUETOOTH_AUTHENTICATION_REQUIREMENTS{
    BLUETOOTH_MITM_ProtectionNotRequired = 0,
    BLUETOOTH_MITM_ProtectionRequired = 0x1,
    BLUETOOTH_MITM_ProtectionNotRequiredBonding = 0x2,
    BLUETOOTH_MITM_ProtectionRequiredBonding = 0x3,
    BLUETOOTH_MITM_ProtectionNotRequiredGeneralBonding = 0x4,
    BLUETOOTH_MITM_ProtectionRequiredGeneralBonding = 0x5,
    BLUETOOTH_MITM_ProtectionNotDefined = 0xff,
} BLUETOOTH_AUTHENTICATION_REQUIREMENTS;

typedef struct _BLUETOOTH_AUTHENTICATION_CALLBACK_PARAMS {
    BLUETOOTH_DEVICE_INFO deviceInfo;
    BLUETOOTH_AUTHENTICATION_METHOD authenticationMethod;
    BLUETOOTH_IO_CAPABILITY ioCapability;
    BLUETOOTH_AUTHENTICATION_REQUIREMENTS authenticationRequirements;
    union{
        ULONG Numeric_Value;
        ULONG Passkey;
    };
} BLUETOOTH_AUTHENTICATION_CALLBACK_PARAMS, *PBLUETOOTH_AUTHENTICATION_CALLBACK_PARAMS;

typedef BOOL (CALLBACK *PFN_AUTHENTICATION_CALLBACK_EX)(void *, BLUETOOTH_AUTHENTICATION_CALLBACK_PARAMS *);

typedef struct _SDP_ELEMENT_DATA {
    SDP_TYPE type;
    SDP_SPECIFICTYPE specificType;
    union {
        SDP_LARGE_INTEGER_16 int128;
        LONGLONG int64;
        LONG int32;
        SHORT int16;
        CHAR int8;

        SDP_ULARGE_INTEGER_16 uint128;
        ULONGLONG uint64;
        ULONG uint32;
        USHORT uint16;
        UCHAR uint8;

        UCHAR booleanVal;

        GUID uuid128;
        ULONG uuid32;
        USHORT uuid16;

        struct {
            BYTE *value;
            ULONG length;
        } string;

        struct {
            BYTE *value;
            ULONG length;
        } url;

        struct {
            BYTE *value;
            ULONG length;
        } sequence;

        struct {
            BYTE *value;
            ULONG length;
        } alternative;
    } data;
} SDP_ELEMENT_DATA, *PSDP_ELEMENT_DATA;

typedef struct _SDP_STRING_TYPE_DATA {
    USHORT encoding;
    USHORT mibeNum;
    USHORT attributeId;
} SDP_STRING_TYPE_DATA, *PSDP_STRING_TYPE_DATA;

typedef BOOL (CALLBACK *PFN_BLUETOOTH_ENUM_ATTRIBUTES_CALLBACK)(
    ULONG uAttribId,
    BYTE  *pValueStream,
    ULONG cbStreamSize,
    void  *pvParam);

DWORD WINAPI BluetoothAuthenticateDevice(HWND, HANDLE, BLUETOOTH_DEVICE_INFO *, WCHAR *, ULONG);
DWORD WINAPI BluetoothAuthenticateMultipleDevices(HWND, HANDLE, DWORD, BLUETOOTH_DEVICE_INFO *);
BOOL  WINAPI BluetoothDisplayDeviceProperties(HWND, BLUETOOTH_DEVICE_INFO *);
BOOL  WINAPI BluetoothEnableDiscovery(HANDLE, BOOL);
BOOL  WINAPI BluetoothEnableIncomingConnections(HANDLE, BOOL);
DWORD WINAPI BluetoothEnumerateInstalledServices(HANDLE, BLUETOOTH_DEVICE_INFO *, DWORD *, GUID *);
BOOL  WINAPI BluetoothFindDeviceClose(HBLUETOOTH_DEVICE_FIND);
HBLUETOOTH_DEVICE_FIND WINAPI BluetoothFindFirstDevice(BLUETOOTH_DEVICE_SEARCH_PARAMS *, BLUETOOTH_DEVICE_INFO *);
HBLUETOOTH_RADIO_FIND  WINAPI BluetoothFindFirstRadio(BLUETOOTH_FIND_RADIO_PARAMS *, HANDLE *);
BOOL  WINAPI BluetoothFindNextDevice(HBLUETOOTH_DEVICE_FIND, BLUETOOTH_DEVICE_INFO *);
BOOL  WINAPI BluetoothFindNextRadio(HBLUETOOTH_RADIO_FIND, HANDLE *);
BOOL  WINAPI BluetoothFindRadioClose(HBLUETOOTH_RADIO_FIND);
DWORD WINAPI BluetoothGetDeviceInfo(HANDLE, BLUETOOTH_DEVICE_INFO *);
DWORD WINAPI BluetoothGetRadioInfo(HANDLE, PBLUETOOTH_RADIO_INFO);
BOOL  WINAPI BluetoothIsConnectable(HANDLE);
BOOL  WINAPI BluetoothIsDiscoverable(HANDLE);
DWORD WINAPI BluetoothRegisterForAuthentication(const BLUETOOTH_DEVICE_INFO *, HBLUETOOTH_AUTHENTICATION_REGISTRATION *, PFN_AUTHENTICATION_CALLBACK, void  *);
DWORD WINAPI BluetoothRegisterForAuthenticationEx(const BLUETOOTH_DEVICE_INFO *, HBLUETOOTH_AUTHENTICATION_REGISTRATION *, PFN_AUTHENTICATION_CALLBACK_EX, void *);
DWORD WINAPI BluetoothRemoveDevice(BLUETOOTH_ADDRESS *);
#define BluetoothEnumAttributes BluetoothSdpEnumAttributes
BOOL  WINAPI BluetoothSdpEnumAttributes(BYTE  *, ULONG, PFN_BLUETOOTH_ENUM_ATTRIBUTES_CALLBACK, void  *);
DWORD WINAPI BluetoothSdpGetAttributeValue(BYTE *, ULONG, USHORT, PSDP_ELEMENT_DATA);
DWORD WINAPI BluetoothSdpGetContainerElementData(BYTE *, ULONG, HBLUETOOTH_CONTAINER_ELEMENT *, PSDP_ELEMENT_DATA);
DWORD WINAPI BluetoothSdpGetElementData(BYTE *, ULONG, PSDP_ELEMENT_DATA);
DWORD WINAPI BluetoothSdpGetString(BYTE *, ULONG, PSDP_STRING_TYPE_DATA, USHORT, WCHAR *, ULONG *);
BOOL  WINAPI BluetoothSelectDevices(BLUETOOTH_SELECT_DEVICE_PARAMS *);
BOOL  WINAPI BluetoothSelectDevicesFree(BLUETOOTH_SELECT_DEVICE_PARAMS *);
DWORD WINAPI BluetoothSendAuthenticationResponse(HANDLE, BLUETOOTH_DEVICE_INFO *, WCHAR *);
DWORD WINAPI BluetoothSetServiceState(HANDLE, BLUETOOTH_DEVICE_INFO *, GUID *, DWORD);
BOOL  WINAPI BluetoothUnregisterAuthentication(HBLUETOOTH_AUTHENTICATION_REGISTRATION);
DWORD WINAPI BluetoothUpdateDeviceRecord(BLUETOOTH_DEVICE_INFO *);

#ifdef __cplusplus
}
#endif

#endif /* __BLUETOOTHAPIS_H */
