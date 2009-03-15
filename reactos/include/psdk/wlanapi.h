#ifndef _WLANAPI_H
#define _WLANAPI_H

#include <windot11.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Defines */
#define WLAN_MAX_PHY_INDEX 64
#define WLAN_MAX_NAME_LENGTH 256

/* Enumerations */

typedef enum _WLAN_CONNECTION_MODE {
    wlan_connection_mode_profile = 0,
    wlan_connection_mode_temporary_profile,
    wlan_connection_mode_discovery_secure,
    wlan_connection_mode_discovery_unsecure,
    wlan_connection_mode_auto,
    wlan_connection_mode_invalid
} WLAN_CONNECTION_MODE, *PWLAN_CONNECTION_MODE;

#if defined(__midl) || defined(__WIDL__)
typedef [v1_enum] enum _WLAN_INTERFACE_STATE {
#else
typedef enum _WLAN_INTERFACE_STATE {
#endif
    wlan_interface_state_not_ready = 0,
    wlan_interface_state_connected,
    wlan_interface_state_ad_hoc_network_formed,
    wlan_interface_state_disconnecting,
    wlan_interface_state_disconnected,
    wlan_interface_state_associating,
    wlan_interface_state_discovering,
    wlan_interface_state_authenticating
} WLAN_INTERFACE_STATE;

typedef enum _WLAN_INTERFACE_TYPE {
    wlan_interface_type_emulated_802_11 = 0,
    wlan_interface_type_native_802_11,
    wlan_interface_type_invalid
} WLAN_INTERFACE_TYPE, *PWLAN_INTERFACE_TYPE;

/* Types */
typedef DWORD WLAN_REASON_CODE, *PWLAN_REASON_CODE;
typedef ULONG WLAN_SIGNAL_QUALITY, *PWLAN_SIGNAL_QUALITY; 

typedef struct _DOT11_NETWORK {
    DOT11_SSID dot11Ssid;
    DOT11_BSS_TYPE dot11BssType;
} DOT11_NETWORK, *PDOT11_NETWORK;

typedef struct _DOT11_NETWORK_LIST {
    DWORD dwNumberOfItems;
    DWORD dwIndex;
#if defined(__midl) || defined(__WIDL__)
    [size_is(dwNumberOfItems)] DOT11_NETWORK Network[];
#else
    DOT11_NETWORK Network[1];
#endif
} DOT11_NETWORK_LIST, *PDOT11_NETWORK_LIST;

typedef struct _WLAN_INTERFACE_INFO {
    GUID InterfaceGuid;
    WCHAR strInterfaceDescription[256];
    WLAN_INTERFACE_STATE isState;
} WLAN_INTERFACE_INFO, *PWLAN_INTERFACE_INFO;

typedef struct _WLAN_INTERFACE_INFO_LIST {
    DWORD dwNumberOfItems;
    DWORD dwIndex;
#if defined(__midl) || defined(__WIDL__)
    [unique, size_is(dwNumberOfItems)] WLAN_INTERFACE_INFO InterfaceInfo[*];
#else
    WLAN_INTERFACE_INFO InterfaceInfo[1];
#endif
} WLAN_INTERFACE_INFO_LIST, *PWLAN_INTERFACE_INFO_LIST;

typedef struct _WLAN_INTERFACE_CAPABILITY {
    WLAN_INTERFACE_TYPE interfaceType;
    BOOL bDot11DSupported;
    DWORD dwMaxDesiredSsidListSize;
    DWORD dwMaxDesiredBssidListSize;
    DWORD dwNumberOfSupportedPhys;
    /* enum32 */ long dot11PhyTypes[WLAN_MAX_PHY_INDEX];
} WLAN_INTERFACE_CAPABILITY, *PWLAN_INTERFACE_CAPABILITY;

typedef struct _WLAN_RAW_DATA {
    DWORD dwDataSize;
#if defined(__midl) || defined(__WIDL__)
    [size_is(dwDataSize)] BYTE DataBlob[];
#else
    BYTE DataBlob[1];
#endif
} WLAN_RAW_DATA, *PWLAN_RAW_DATA;

typedef struct _WLAN_PROFILE_INFO {
    WCHAR strProfileName[256];
    DWORD dwFlags;
} WLAN_PROFILE_INFO, *PWLAN_PROFILE_INFO;

typedef struct _WLAN_PROFILE_INFO_LIST {
    DWORD dwNumberOfItems;
    DWORD dwIndex;
#if defined(__midl) || defined(__WIDL__)
    [size_is(dwNumberOfItems)] WLAN_PROFILE_INFO ProfileInfo[];
#else
    WLAN_PROFILE_INFO ProfileInfo[1];
#endif
} WLAN_PROFILE_INFO_LIST, *PWLAN_PROFILE_INFO_LIST;

typedef struct _WLAN_AVAILABLE_NETWORK {
    WCHAR strProfileName[WLAN_MAX_NAME_LENGTH];
    DOT11_SSID dot11Ssid;
    DOT11_BSS_TYPE dot11BssType;
    ULONG uNumberOfBssids;
    BOOL bNetworkConnectable;
    WLAN_REASON_CODE wlanNotConnectableReason;
    ULONG uNumberOfPhyTypes;
    DOT11_PHY_TYPE dot11PhyTypes[8];
    BOOL bMorePhyTypes;
    WLAN_SIGNAL_QUALITY wlanSignalQuality;
    BOOL bSecurityEnabled;
    DOT11_AUTH_ALGORITHM dot11DefaultAuthAlgorithm;
    DOT11_CIPHER_ALGORITHM dot11DefaultCipherAlgorithm;
    DWORD dwFlags;
    DWORD dwReserved;
} WLAN_AVAILABLE_NETWORK, *PWLAN_AVAILABLE_NETWORK;

typedef struct _WLAN_AVAILABLE_NETWORK_LIST {
    DWORD dwNumberOfItems;
    DWORD dwIndex;
#if defined(__midl) || defined(__WIDL__)
    [size_is(dwNumberOfItems)] WLAN_AVAILABLE_NETWORK Network[];
#else
    WLAN_AVAILABLE_NETWORK Network[1];
#endif
} WLAN_AVAILABLE_NETWORK_LIST ,*PWLAN_AVAILABLE_NETWORK_LIST;

typedef struct _WLAN_CONNECTION_PARAMETERS {
    WLAN_CONNECTION_MODE wlanConnectionMode;
    [string] wchar_t * strProfile;
    PDOT11_SSID pDot11Ssid;
    PDOT11_BSSID_LIST pDesiredBssidList;
    DOT11_BSS_TYPE dot11BssType;
    DWORD dwFlags;
} WLAN_CONNECTION_PARAMETERS, *PWLAN_CONNECTION_PARAMETERS;


/* FIXME */

typedef struct _struct_8 {
    DWORD elem_1;
    DWORD elem_2;
    GUID elem_3;
    DWORD elem_4;
    [size_is(elem_4)] LPBYTE elem_5;
} struct_8 ;

typedef struct struct_12 {
    BYTE elem_1;
    DWORD elem_2;
    DWORD elem_3;
} struct_12 ;

typedef struct _struct_9 {
    struct_12 elem_1;
    DWORD elem_2;
} struct_9 ;

typedef struct struct_C {
    ULONGLONG elem_1;
    ULONGLONG elem_2;
    GUID elem_3;
    GUID elem_4;
    DWORD elem_5;
    GUID elem_6;
    DWORD elem_7;
    DWORD elem_8;
    ULONGLONG elem_9;
    [size_is(elem_7)] BYTE elem_10[];
} struct_C ;

typedef struct struct_D {
    GUID elem_1;
    GUID elem_2;
    DWORD elem_3;
    ULONGLONG elem_4;
    [size_is(elem_3)] BYTE elem_5[];
} struct_D ;

typedef struct struct_E {
    /* enum16 */ short elem_1;
    DWORD elem_2;
    WCHAR elem_3[256];
    WCHAR elem_4[1024];
} struct_E ;

/* Functions */
#if !defined(__midl) && !defined(__WIDL__)
PVOID WINAPI WlanAllocateMemory(DWORD dwSize);
VOID WINAPI WlanFreeMemory(PVOID pMemory);
#endif

#ifdef __cplusplus
}
#endif


#endif  // _WLANAPI_H
