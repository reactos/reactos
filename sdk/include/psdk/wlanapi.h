#ifndef _WLANAPI_H
#define _WLANAPI_H

#include <l2cmn.h>
#include <windot11.h>
#include <eaptypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Defines */
#define WLAN_API_VERSION_1_0 0x00000001
#define WLAN_API_VERSION_2_0 0x00000002

#define WLAN_MAX_PHY_INDEX 64
#define WLAN_MAX_NAME_LENGTH 256

#define WLAN_AVAILABLE_NETWORK_INCLUDE_ALL_ADHOC_PROFILES         0x00000001
#define WLAN_AVAILABLE_NETWORK_INCLUDE_ALL_MANUAL_HIDDEN_PROFILES 0x00000002

#define WLAN_AVAILABLE_NETWORK_CONNECTED                          0x00000001
#define WLAN_AVAILABLE_NETWORK_HAS_PROFILE                        0x00000002
#define WLAN_AVAILABLE_NETWORK_CONSOLE_USER_PROFILE               0x00000004
#define WLAN_AVAILABLE_NETWORK_INTERWORKING_SUPPORTED             0x00000008
#define WLAN_AVAILABLE_NETWORK_HOTSPOT2_ENABLED                   0x00000010
#define WLAN_AVAILABLE_NETWORK_ANQP_SUPPORTED                     0x00000020
#define WLAN_AVAILABLE_NETWORK_HOTSPOT2_DOMAIN                    0x00000040
#define WLAN_AVAILABLE_NETWORK_HOTSPOT2_ROAMING                   0x00000080
#define WLAN_AVAILABLE_NETWORK_AUTO_CONNECT_FAILED                0x00000100

/* Enumerations */

#if defined(__midl) || defined(__WIDL__)
typedef [v1_enum] enum _WLAN_OPCODE_VALUE_TYPE {
#else
typedef enum _WLAN_OPCODE_VALUE_TYPE {
#endif
    wlan_opcode_value_type_query_only = 0,
    wlan_opcode_value_type_set_by_group_policy,
    wlan_opcode_value_type_set_by_user,
    wlan_opcode_value_type_invalid
} WLAN_OPCODE_VALUE_TYPE; /* HACK: WIDL is broken    , *PWLAN_OPCODE_VALUE_TYPE; */

typedef enum _WLAN_SECURABLE_OBJECT {
    wlan_secure_permit_list = 0,
    wlan_secure_deny_list,
    wlan_secure_ac_enabled,
    wlan_secure_bc_scan_enabled,
    wlan_secure_bss_type,
    wlan_secure_show_denied,
    wlan_secure_interface_properties,
    wlan_secure_ihv_control,
    wlan_secure_all_user_profiles_order,
    wlan_secure_add_new_all_user_profiles,
    wlan_secure_add_new_per_user_profiles,
    wlan_secure_media_streaming_mode_enabled,
    wlan_secure_current_operation_mode,
    WLAN_SECURABLE_OBJECT_COUNT
} WLAN_SECURABLE_OBJECT, *PWLAN_SECURABLE_OBJECT;

typedef enum _WLAN_CONNECTION_MODE {
    wlan_connection_mode_profile = 0,
    wlan_connection_mode_temporary_profile,
    wlan_connection_mode_discovery_secure,
    wlan_connection_mode_discovery_unsecure,
    wlan_connection_mode_auto,
    wlan_connection_mode_invalid
} WLAN_CONNECTION_MODE, *PWLAN_CONNECTION_MODE;

#if defined(__midl) || defined(__WIDL__)
typedef [v1_enum] enum _WLAN_IHV_CONTROL_TYPE {
#else
typedef enum _WLAN_IHV_CONTROL_TYPE {
#endif
    wlan_ihv_control_type_service,
    wlan_ihv_control_type_driver
} WLAN_IHV_CONTROL_TYPE;  /* HACK , *PWLAN_IHV_CONTROL_TYPE; */

#if defined(__midl) || defined(__WIDL__)
typedef [v1_enum] enum _WLAN_INTF_OPCODE {
#else
typedef enum _WLAN_INTF_OPCODE {
#endif
    wlan_intf_opcode_autoconf_start                              = 0x000000000,
    wlan_intf_opcode_autoconf_enabled,
    wlan_intf_opcode_background_scan_enabled,
    wlan_intf_opcode_media_streaming_mode,
    wlan_intf_opcode_radio_state,
    wlan_intf_opcode_bss_type,
    wlan_intf_opcode_interface_state,
    wlan_intf_opcode_current_connection,
    wlan_intf_opcode_channel_number,
    wlan_intf_opcode_supported_infrastructure_auth_cipher_pairs,
    wlan_intf_opcode_supported_adhoc_auth_cipher_pairs,
    wlan_intf_opcode_supported_country_or_region_string_list,
    wlan_intf_opcode_current_operation_mode,
    wlan_intf_opcode_supported_safe_mode,
    wlan_intf_opcode_certified_safe_mode,
    wlan_intf_opcode_hosted_network_capable,
    wlan_intf_opcode_management_frame_protection_capable,
    wlan_intf_opcode_autoconf_end                                = 0x0fffffff,
    wlan_intf_opcode_msm_start                                   = 0x10000100,
    wlan_intf_opcode_statistics,
    wlan_intf_opcode_rssi,
    wlan_intf_opcode_msm_end                                     = 0x1fffffff,
    wlan_intf_opcode_security_start                              = 0x20010000,
    wlan_intf_opcode_security_end                                = 0x2fffffff,
    wlan_intf_opcode_ihv_start                                   = 0x30000000,
    wlan_intf_opcode_ihv_end                                     = 0x3fffffff
} WLAN_INTF_OPCODE; /* HACK: WIDL is broken    , *PWLAN_INTF_OPCODE; */

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
    [size_is(dwNumberOfItems)] WLAN_AVAILABLE_NETWORK Network[*];
#else
    WLAN_AVAILABLE_NETWORK Network[1];
#endif
} WLAN_AVAILABLE_NETWORK_LIST ,*PWLAN_AVAILABLE_NETWORK_LIST;

typedef struct _WLAN_CONNECTION_PARAMETERS {
    WLAN_CONNECTION_MODE wlanConnectionMode;
#if defined(__midl) || defined(__WIDL__)
    [string] LPCWSTR strProfile;
#else
    LPCWSTR strProfile;
#endif
    PDOT11_SSID pDot11Ssid;
    PDOT11_BSSID_LIST pDesiredBssidList;
    DOT11_BSS_TYPE dot11BssType;
    DWORD dwFlags;
} WLAN_CONNECTION_PARAMETERS, *PWLAN_CONNECTION_PARAMETERS;

typedef L2_NOTIFICATION_DATA WLAN_NOTIFICATION_DATA, *PWLAN_NOTIFICATION_DATA;

typedef void (__stdcall *WLAN_NOTIFICATION_CALLBACK) (PWLAN_NOTIFICATION_DATA, PVOID);

/* Functions */
#if !defined(__midl) && !defined(__WIDL__)
PVOID WINAPI WlanAllocateMemory(DWORD dwSize);
VOID WINAPI WlanFreeMemory(PVOID pMemory);
DWORD WINAPI WlanOpenHandle(IN DWORD dwClientVersion, PVOID pReserved, OUT DWORD *pdwNegotiatedVersion, OUT HANDLE *phClientHandle);
DWORD WINAPI WlanCloseHandle(IN HANDLE hClientHandle, PVOID pReserved);
DWORD WINAPI WlanConnect(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, IN const PWLAN_CONNECTION_PARAMETERS pConnectionParameters, PVOID pReserved);
DWORD WINAPI WlanDisconnect(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, PVOID pReserved);
DWORD WINAPI WlanEnumInterfaces(IN HANDLE hClientHandle, PVOID pReserved, OUT PWLAN_INTERFACE_INFO_LIST *ppInterfaceList);
DWORD WINAPI WlanGetAvailableNetworkList(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, IN DWORD dwFlags, PVOID pReserved, OUT PWLAN_AVAILABLE_NETWORK_LIST *ppAvailableNetworkList);
DWORD WINAPI WlanGetInterfaceCapability(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, PVOID pReserved, OUT PWLAN_INTERFACE_CAPABILITY *ppCapability);
DWORD WINAPI WlanDeleteProfile(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, IN LPCWSTR strProfileName, PVOID pReserved);
DWORD WINAPI WlanGetProfile(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, IN LPCWSTR strProfileName, PVOID pReserved, OUT LPWSTR *pstrProfileXml, DWORD *pdwFlags, PDWORD pdwGrantedAccess);
DWORD WINAPI WlanGetProfileCustomUserData(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, IN LPCWSTR strProfileName, PVOID pReserved, OUT DWORD *pdwDataSize, OUT PBYTE *ppData);
DWORD WINAPI WlanGetProfileList(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, PVOID pReserved, OUT PWLAN_PROFILE_INFO_LIST *ppProfileList);
DWORD WINAPI WlanIhvControl(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, IN WLAN_IHV_CONTROL_TYPE Type, IN DWORD dwInBufferSize, IN PVOID pInBuffer, IN DWORD dwOutBufferSize, PVOID pOutBuffer, OUT PDWORD pdwBytesReturned);
DWORD WINAPI WlanQueryInterface(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, IN WLAN_INTF_OPCODE OpCode, PVOID pReserved, OUT PDWORD pdwDataSize, OUT PVOID *ppData, WLAN_OPCODE_VALUE_TYPE *pWlanOpcodeValueType);
DWORD WINAPI WlanReasonCodeToString(IN DWORD dwReasonCode, IN DWORD dwBufferSize, IN PWCHAR pStringBuffer, PVOID pReserved);
DWORD WINAPI WlanRegisterNotification(IN HANDLE hClientHandle,IN DWORD dwNotifSource, IN BOOL bIgnoreDuplicate, WLAN_NOTIFICATION_CALLBACK funcCallback, PVOID pCallbackContext, PVOID pReserved, PDWORD pdwPrevNotifSource);
DWORD WINAPI WlanRenameProfile(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, IN LPCWSTR strOldProfileName, IN LPCWSTR strNewProfileName, PVOID pReserved);
DWORD WINAPI WlanSetProfile(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, IN DWORD dwFlags, IN LPCWSTR strProfileXml, LPCWSTR strAllUserProfileSecurity, IN BOOL bOverwrite, PVOID pReserved, OUT DWORD *pdwReasonCode);
DWORD WINAPI WlanSetProfileCustomUserData(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, IN LPCWSTR strProfileName, IN DWORD dwDataSize, IN const PBYTE pData, PVOID pReserved);
DWORD WINAPI WlanSetProfileEapUserData(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, IN LPCWSTR strProfileName, IN EAP_METHOD_TYPE eapType, IN DWORD dwFlags, IN DWORD dwEapUserDataSize, IN const LPBYTE pbEapUserData, PVOID pReserved);
DWORD WINAPI WlanSetProfileList(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, DWORD dwItems, IN LPCWSTR *strProfileNames, PVOID pReserved);
DWORD WINAPI WlanSetSecuritySettings(IN HANDLE hClientHandle, IN WLAN_SECURABLE_OBJECT SecurableObject, IN LPCWSTR strModifiedSDDL);
DWORD WINAPI WlanScan(IN HANDLE hClientHandle, IN const GUID *pInterfaceGuid, IN PDOT11_SSID pDot11Ssid, IN PWLAN_RAW_DATA pIeData, PVOID pReserved);
#endif

#ifdef __cplusplus
}
#endif


#endif  // _WLANAPI_H
