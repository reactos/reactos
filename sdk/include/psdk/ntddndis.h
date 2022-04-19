/*
 * ntddndis.h
 *
 * NDIS device driver interface
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _NTDDNDIS_
#define _NTDDNDIS_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _NDIS_WAN_QUALITY {
	NdisWanRaw,
	NdisWanErrorControl,
	NdisWanReliable
} NDIS_WAN_QUALITY, *PNDIS_WAN_QUALITY;

typedef enum _NDIS_DEVICE_POWER_STATE {
  NdisDeviceStateUnspecified = 0,
  NdisDeviceStateD0,
  NdisDeviceStateD1,
  NdisDeviceStateD2,
  NdisDeviceStateD3,
  NdisDeviceStateMaximum
} NDIS_DEVICE_POWER_STATE, *PNDIS_DEVICE_POWER_STATE;

typedef enum _NDIS_802_11_WEP_STATUS
{
    Ndis802_11WEPEnabled,
    Ndis802_11Encryption1Enabled = Ndis802_11WEPEnabled,
    Ndis802_11WEPDisabled,
    Ndis802_11EncryptionDisabled = Ndis802_11WEPDisabled,
    Ndis802_11WEPKeyAbsent,
    Ndis802_11Encryption1KeyAbsent = Ndis802_11WEPKeyAbsent,
    Ndis802_11WEPNotSupported,
    Ndis802_11EncryptionNotSupported = Ndis802_11WEPNotSupported,
    Ndis802_11Encryption2Enabled,
    Ndis802_11Encryption2KeyAbsent,
    Ndis802_11Encryption3Enabled,
    Ndis802_11Encryption3KeyAbsent
} NDIS_802_11_WEP_STATUS, *PNDIS_802_11_WEP_STATUS,
  NDIS_802_11_ENCRYPTION_STATUS, *PNDIS_802_11_ENCRYPTION_STATUS;

typedef enum _NDIS_802_11_AUTHENTICATION_MODE
{
    Ndis802_11AuthModeOpen,
    Ndis802_11AuthModeShared,
    Ndis802_11AuthModeAutoSwitch,
    Ndis802_11AuthModeWPA,
    Ndis802_11AuthModeWPAPSK,
    Ndis802_11AuthModeWPANone,
    Ndis802_11AuthModeWPA2,
    Ndis802_11AuthModeWPA2PSK,
    Ndis802_11AuthModeMax
} NDIS_802_11_AUTHENTICATION_MODE, *PNDIS_802_11_AUTHENTICATION_MODE;

typedef enum _NDIS_802_11_NETWORK_INFRASTRUCTURE
{
    Ndis802_11IBSS,
    Ndis802_11Infrastructure,
    Ndis802_11AutoUnknown,
    Ndis802_11InfrastructureMax
} NDIS_802_11_NETWORK_INFRASTRUCTURE, *PNDIS_802_11_NETWORK_INFRASTRUCTURE;

typedef enum _NDIS_802_11_NETWORK_TYPE
{
    Ndis802_11FH,
    Ndis802_11DS,
    Ndis802_11OFDM5,
    Ndis802_11OFDM24,
    Ndis802_11Automode,
    Ndis802_11NetworkTypeMax
} NDIS_802_11_NETWORK_TYPE, *PNDIS_802_11_NETWORK_TYPE;

typedef struct _NDIS_OBJECT_HEADER
{
    UCHAR Type;
    UCHAR Revision;
    USHORT Size;
} NDIS_OBJECT_HEADER, *PNDIS_OBJECT_HEADER;

#define NDIS_802_11_LENGTH_SSID  32
#define NDIS_802_11_LENGTH_RATES 8

typedef UCHAR NDIS_802_11_MAC_ADDRESS[6];
typedef LONG NDIS_802_11_RSSI;
typedef UCHAR NDIS_802_11_RATES[NDIS_802_11_LENGTH_RATES];

typedef struct _NDIS_802_11_SSID
{
    ULONG SsidLength;
    UCHAR Ssid[NDIS_802_11_LENGTH_SSID];
} NDIS_802_11_SSID, *PNDIS_802_11_SSID;

typedef struct _NDIS_802_11_CONFIGURATION_FH
{
    ULONG Length;
    ULONG HopPattern;
    ULONG HopSet;
    ULONG DwellTime;
} NDIS_802_11_CONFIGURATION_FH, *PNDIS_802_11_CONFIGURATION_FH;

typedef struct _NDIS_802_11_CONFIGURATION
{
    ULONG Length;
    ULONG BeaconPeriod;
    ULONG ATIMWindow;
    ULONG DSConfig;
    NDIS_802_11_CONFIGURATION_FH FHConfig;
} NDIS_802_11_CONFIGURATION, *PNDIS_802_11_CONFIGURATION;

typedef struct _NDIS_WLAN_BSSID
{
    ULONG Length;
    NDIS_802_11_MAC_ADDRESS MacAddress;
    UCHAR Reserved[2];
    NDIS_802_11_SSID Ssid;
    ULONG Privacy;
    NDIS_802_11_RSSI Rssi;
    NDIS_802_11_NETWORK_TYPE NetworkTypeInUse;
    NDIS_802_11_CONFIGURATION Configuration;
    NDIS_802_11_NETWORK_INFRASTRUCTURE InfrastructureMode;
    NDIS_802_11_RATES SupportedRates;
} NDIS_WLAN_BSSID, *PNDIS_WLAN_BSSID;

typedef struct _NDIS_802_11_BSSID_LIST
{
    ULONG NumberOfItems;
    NDIS_WLAN_BSSID Bssid[1];
} NDIS_802_11_BSSID_LIST, *PNDIS_802_11_BSSID_LIST;

typedef struct _NDIS_802_11_WEP
{
    ULONG Length;
    ULONG KeyIndex;
    ULONG KeyLength;
    UCHAR KeyMaterial[1];
} NDIS_802_11_WEP, *PNDIS_802_11_WEP;

typedef ULONGLONG NDIS_802_11_KEY_RSC;

typedef struct _NDIS_802_11_KEY
{
    ULONG Length;
    ULONG KeyIndex;
    ULONG KeyLength;
    NDIS_802_11_MAC_ADDRESS BSSID;
    NDIS_802_11_KEY_RSC KeyRSC;
    UCHAR KeyMaterial[1];
} NDIS_802_11_KEY, *PNDIS_802_11_KEY;

typedef struct _NDIS_PM_WAKE_UP_CAPABILITIES {
  NDIS_DEVICE_POWER_STATE  MinMagicPacketWakeUp;
  NDIS_DEVICE_POWER_STATE  MinPatternWakeUp;
  NDIS_DEVICE_POWER_STATE  MinLinkChangeWakeUp;
} NDIS_PM_WAKE_UP_CAPABILITIES, *PNDIS_PM_WAKE_UP_CAPABILITIES;

/* NDIS_PNP_CAPABILITIES.Flags constants */
#define NDIS_DEVICE_WAKE_UP_ENABLE                0x00000001
#define NDIS_DEVICE_WAKE_ON_PATTERN_MATCH_ENABLE  0x00000002
#define NDIS_DEVICE_WAKE_ON_MAGIC_PACKET_ENABLE   0x00000004

typedef struct _NDIS_PNP_CAPABILITIES {
  ULONG  Flags;
  NDIS_PM_WAKE_UP_CAPABILITIES  WakeUpCapabilities;
} NDIS_PNP_CAPABILITIES, *PNDIS_PNP_CAPABILITIES;

/* NDIS driver medium (OID_GEN_MEDIA_SUPPORTED / OID_GEN_MEDIA_IN_USE) */
typedef enum _NDIS_MEDIUM {
  NdisMedium802_3,
  NdisMedium802_5,
  NdisMediumFddi,
  NdisMediumWan,
  NdisMediumLocalTalk,
  NdisMediumDix,
  NdisMediumArcnetRaw,
  NdisMediumArcnet878_2,
  NdisMediumAtm,
  NdisMediumWirelessWan,
  NdisMediumIrda,
  NdisMediumBpc,
  NdisMediumCoWan,
  NdisMedium1394,
  NdisMediumMax
} NDIS_MEDIUM, *PNDIS_MEDIUM;

typedef enum _NDIS_PHYSICAL_MEDIUM
{
    NdisPhysicalMediumUnspecified,
    NdisPhysicalMediumWirelessLan,
    NdisPhysicalMediumCableModem,
    NdisPhysicalMediumPhoneLine,
    NdisPhysicalMediumPowerLine,
    NdisPhysicalMediumDSL,
    NdisPhysicalMediumFibreChannel,
    NdisPhysicalMedium1394,
    NdisPhysicalMediumWirelessWan,
    NdisPhysicalMediumNative802_11,
    NdisPhysicalMediumBluetooth,
    NdisPhysicalMediumInfiniband,
    NdisPhysicalMediumWiMax,
    NdisPhysicalMediumUWB,
    NdisPhysicalMedium802_3,
    NdisPhysicalMedium802_5,
    NdisPhysicalMediumIrda,
    NdisPhysicalMediumWiredWAN,
    NdisPhysicalMediumWiredCoWan,
    NdisPhysicalMediumOther,
    NdisPhysicalMediumMax
} NDIS_PHYSICAL_MEDIUM, *PNDIS_PHYSICAL_MEDIUM;

typedef ULONG NDIS_OID, *PNDIS_OID;

/* Required Object IDs (OIDs) */
#define OID_GEN_SUPPORTED_LIST            0x00010101
#define OID_GEN_HARDWARE_STATUS           0x00010102
#define OID_GEN_MEDIA_SUPPORTED           0x00010103
#define OID_GEN_MEDIA_IN_USE              0x00010104
#define OID_GEN_MAXIMUM_LOOKAHEAD         0x00010105
#define OID_GEN_MAXIMUM_FRAME_SIZE        0x00010106
#define OID_GEN_LINK_SPEED                0x00010107
#define OID_GEN_TRANSMIT_BUFFER_SPACE     0x00010108
#define OID_GEN_RECEIVE_BUFFER_SPACE      0x00010109
#define OID_GEN_TRANSMIT_BLOCK_SIZE       0x0001010A
#define OID_GEN_RECEIVE_BLOCK_SIZE        0x0001010B
#define OID_GEN_VENDOR_ID                 0x0001010C
#define OID_GEN_VENDOR_DESCRIPTION        0x0001010D
#define OID_GEN_CURRENT_PACKET_FILTER     0x0001010E
#define OID_GEN_CURRENT_LOOKAHEAD         0x0001010F
#define OID_GEN_DRIVER_VERSION            0x00010110
#define OID_GEN_MAXIMUM_TOTAL_SIZE        0x00010111
#define OID_GEN_PROTOCOL_OPTIONS          0x00010112
#define OID_GEN_MAC_OPTIONS               0x00010113
#define OID_GEN_MEDIA_CONNECT_STATUS      0x00010114
#define OID_GEN_MAXIMUM_SEND_PACKETS      0x00010115
#define OID_GEN_VENDOR_DRIVER_VERSION     0x00010116
#define OID_GEN_SUPPORTED_GUIDS           0x00010117
#define OID_GEN_NETWORK_LAYER_ADDRESSES   0x00010118
#define OID_GEN_TRANSPORT_HEADER_OFFSET   0x00010119
#define OID_GEN_MACHINE_NAME              0x0001021A
#define OID_GEN_RNDIS_CONFIG_PARAMETER    0x0001021B
#define OID_GEN_VLAN_ID                   0x0001021C

/* Optional OIDs */
#define OID_GEN_MEDIA_CAPABILITIES        0x00010201
#define OID_GEN_PHYSICAL_MEDIUM           0x00010202

/* Required statistics OIDs */
#define OID_GEN_XMIT_OK                   0x00020101
#define OID_GEN_RCV_OK                    0x00020102
#define OID_GEN_XMIT_ERROR                0x00020103
#define OID_GEN_RCV_ERROR                 0x00020104
#define OID_GEN_RCV_NO_BUFFER             0x00020105

/* Optional statistics OIDs */
#define OID_GEN_DIRECTED_BYTES_XMIT       0x00020201
#define OID_GEN_DIRECTED_FRAMES_XMIT      0x00020202
#define OID_GEN_MULTICAST_BYTES_XMIT      0x00020203
#define OID_GEN_MULTICAST_FRAMES_XMIT     0x00020204
#define OID_GEN_BROADCAST_BYTES_XMIT      0x00020205
#define OID_GEN_BROADCAST_FRAMES_XMIT     0x00020206
#define OID_GEN_DIRECTED_BYTES_RCV        0x00020207
#define OID_GEN_DIRECTED_FRAMES_RCV       0x00020208
#define OID_GEN_MULTICAST_BYTES_RCV       0x00020209
#define OID_GEN_MULTICAST_FRAMES_RCV      0x0002020A
#define OID_GEN_BROADCAST_BYTES_RCV       0x0002020B
#define OID_GEN_BROADCAST_FRAMES_RCV      0x0002020C
#define OID_GEN_RCV_CRC_ERROR             0x0002020D
#define OID_GEN_TRANSMIT_QUEUE_LENGTH     0x0002020E
#define OID_GEN_GET_TIME_CAPS             0x0002020F
#define OID_GEN_GET_NETCARD_TIME          0x00020210
#define OID_GEN_NETCARD_LOAD              0x00020211
#define OID_GEN_DEVICE_PROFILE            0x00020212
#define OID_GEN_INIT_TIME_MS              0x00020213
#define OID_GEN_RESET_COUNTS              0x00020214
#define OID_GEN_MEDIA_SENSE_COUNTS        0x00020215
#define OID_GEN_FRIENDLY_NAME             0x00020216
#define OID_GEN_MINIPORT_INFO             0x00020217
#define OID_GEN_RESET_VERIFY_PARAMETERS   0x00020218

/* IEEE 802.3 (Ethernet) OIDs */
#define NDIS_802_3_MAC_OPTION_PRIORITY    0x00000001

#define OID_802_3_PERMANENT_ADDRESS       0x01010101
#define OID_802_3_CURRENT_ADDRESS         0x01010102
#define OID_802_3_MULTICAST_LIST          0x01010103
#define OID_802_3_MAXIMUM_LIST_SIZE       0x01010104
#define OID_802_3_MAC_OPTIONS             0x01010105
#define OID_802_3_RCV_ERROR_ALIGNMENT     0x01020101
#define OID_802_3_XMIT_ONE_COLLISION      0x01020102
#define OID_802_3_XMIT_MORE_COLLISIONS    0x01020103
#define OID_802_3_XMIT_DEFERRED           0x01020201
#define OID_802_3_XMIT_MAX_COLLISIONS     0x01020202
#define OID_802_3_RCV_OVERRUN             0x01020203
#define OID_802_3_XMIT_UNDERRUN           0x01020204
#define OID_802_3_XMIT_HEARTBEAT_FAILURE  0x01020205
#define OID_802_3_XMIT_TIMES_CRS_LOST     0x01020206
#define OID_802_3_XMIT_LATE_COLLISIONS    0x01020207

/* IEEE 802.11 (WLAN) OIDs */
#define OID_802_11_BSSID                        0x0D010101
#define OID_802_11_SSID                         0x0D010102
#define OID_802_11_NETWORK_TYPES_SUPPORTED      0x0D010203
#define OID_802_11_NETWORK_TYPE_IN_USE          0x0D010204
#define OID_802_11_TX_POWER_LEVEL               0x0D010205
#define OID_802_11_RSSI                         0x0D010206
#define OID_802_11_RSSI_TRIGGER                 0x0D010207
#define OID_802_11_INFRASTRUCTURE_MODE          0x0D010108
#define OID_802_11_FRAGMENTATION_THRESHOLD      0x0D010209
#define OID_802_11_RTS_THRESHOLD                0x0D01020A
#define OID_802_11_NUMBER_OF_ANTENNAS           0x0D01020B
#define OID_802_11_RX_ANTENNA_SELECTED          0x0D01020C
#define OID_802_11_TX_ANTENNA_SELECTED          0x0D01020D
#define OID_802_11_SUPPORTED_RATES              0x0D01020E
#define OID_802_11_DESIRED_RATES                0x0D010210
#define OID_802_11_CONFIGURATION                0x0D010211
#define OID_802_11_STATISTICS                   0x0D020212
#define OID_802_11_ADD_WEP                      0x0D010113
#define OID_802_11_REMOVE_WEP                   0x0D010114
#define OID_802_11_DISASSOCIATE                 0x0D010115
#define OID_802_11_POWER_MODE                   0x0D010216
#define OID_802_11_BSSID_LIST                   0x0D010217
#define OID_802_11_AUTHENTICATION_MODE          0x0D010118
#define OID_802_11_PRIVACY_FILTER               0x0D010119
#define OID_802_11_BSSID_LIST_SCAN              0x0D01011A
#define OID_802_11_WEP_STATUS                   0x0D01011B
/* New name is supposed to better reflect the extended set of encryption status */
#define OID_802_11_ENCRYPTION_STATUS            OID_802_11_WEP_STATUS
#define OID_802_11_RELOAD_DEFAULTS              0x0D01011C
/* Allows key mapping and default keys */
#define OID_802_11_ADD_KEY                      0x0D01011D
#define OID_802_11_REMOVE_KEY                   0x0D01011E
#define OID_802_11_ASSOCIATION_INFORMATION      0x0D01011F
#define OID_802_11_TEST                         0x0D010120
#define OID_802_11_MEDIA_STREAM_MODE            0x0D010121
#define OID_802_11_CAPABILITY                   0x0D010122
#define OID_802_11_PMKID                        0x0D010123
#define OID_802_11_NON_BCAST_SSID_LIST          0x0D010124
#define OID_802_11_RADIO_STATUS                 0x0D010125

/* PnP and Power Management (PM) OIDs */
#define OID_PNP_CAPABILITIES                    0xFD010100
#define OID_PNP_SET_POWER                       0xFD010101
#define OID_PNP_QUERY_POWER                     0xFD010102
#define OID_PNP_ADD_WAKE_UP_PATTERN             0xFD010103
#define OID_PNP_REMOVE_WAKE_UP_PATTERN          0xFD010104
#define OID_PNP_WAKE_UP_PATTERN_LIST            0xFD010105
#define OID_PNP_ENABLE_WAKE_UP                  0xFD010106

/* Optional PnP/PM statistics OIDs */
#define OID_PNP_WAKE_UP_OK                      0xFD020200
#define OID_PNP_WAKE_UP_ERROR                   0xFD020201

#define NDIS_PNP_WAKE_UP_MAGIC_PACKET           0x00000001
#define NDIS_PNP_WAKE_UP_PATTERN_MATCH          0x00000002
#define NDIS_PNP_WAKE_UP_LINK_CHANGE            0x00000004

/* TCP and IP OIDs */
#define OID_TCP_TASK_OFFLOAD                    0xFC010201
#define OID_TCP_TASK_IPSEC_ADD_SA               0xFC010202
#define OID_TCP_TASK_IPSEC_DELETE_SA            0xFC010203
#define OID_TCP_SAN_SUPPORT                     0xFC010204
#define OID_TCP_TASK_IPSEC_ADD_UDPESP_SA        0xFC010205
#define OID_TCP_TASK_IPSEC_DELETE_UDPESP_SA     0xFC010206
#define OID_TCP4_OFFLOAD_STATS                  0xFC010207
#define OID_TCP6_OFFLOAD_STATS                  0xFC010208
#define OID_IP4_OFFLOAD_STATS                   0xFC010209
#define OID_IP6_OFFLOAD_STATS                   0xFC01020A

/* New NDIS 6 offload OIDs */
#define OID_TCP_OFFLOAD_CURRENT_CONFIG                     0xFC01020B /* NDIS 5 handled. Query only */
#define OID_TCP_OFFLOAD_PARAMETERS                         0xFC01020C /* Set only */
#define OID_TCP_OFFLOAD_HARDWARE_CAPABILITIES              0xFC01020D /* Query only */
#define OID_TCP_CONNECTION_OFFLOAD_CURRENT_CONFIG          0xFC01020E /* Query only */
#define OID_TCP_CONNECTION_OFFLOAD_HARDWARE_CAPABILITIES   0xFC01020F /* Query only */
#define OID_OFFLOAD_ENCAPSULATION                          0x0101010A

/* Obsolete FFP defines */
#define OID_FFP_SUPPORT                         0xFC010210
#define OID_FFP_FLUSH                           0xFC010211
#define OID_FFP_CONTROL                         0xFC010212
#define OID_FFP_PARAMS                          0xFC010213
#define OID_FFP_DATA                            0xFC010214

#define OID_FFP_DRIVER_STATS                    0xFC020210
#define OID_FFP_ADAPTER_STATS                   0xFC020211

/* OID_GEN_MINIPORT_INFO constants */
#define NDIS_MINIPORT_BUS_MASTER                      0x00000001
#define NDIS_MINIPORT_WDM_DRIVER                      0x00000002
#define NDIS_MINIPORT_SG_LIST                         0x00000004
#define NDIS_MINIPORT_SUPPORTS_MEDIA_QUERY            0x00000008
#define NDIS_MINIPORT_INDICATES_PACKETS               0x00000010
#define NDIS_MINIPORT_IGNORE_PACKET_QUEUE             0x00000020
#define NDIS_MINIPORT_IGNORE_REQUEST_QUEUE            0x00000040
#define NDIS_MINIPORT_IGNORE_TOKEN_RING_ERRORS        0x00000080
#define NDIS_MINIPORT_INTERMEDIATE_DRIVER             0x00000100
#define NDIS_MINIPORT_IS_NDIS_5                       0x00000200
#define NDIS_MINIPORT_IS_CO                           0x00000400
#define NDIS_MINIPORT_DESERIALIZE                     0x00000800
#define NDIS_MINIPORT_REQUIRES_MEDIA_POLLING          0x00001000
#define NDIS_MINIPORT_SUPPORTS_MEDIA_SENSE            0x00002000
#define NDIS_MINIPORT_NETBOOT_CARD                    0x00004000
#define NDIS_MINIPORT_PM_SUPPORTED                    0x00008000
#define NDIS_MINIPORT_SUPPORTS_MAC_ADDRESS_OVERWRITE  0x00010000
#define NDIS_MINIPORT_USES_SAFE_BUFFER_APIS           0x00020000
#define NDIS_MINIPORT_HIDDEN                          0x00040000
#define NDIS_MINIPORT_SWENUM                          0x00080000
#define NDIS_MINIPORT_SURPRISE_REMOVE_OK              0x00100000
#define NDIS_MINIPORT_NO_HALT_ON_SUSPEND              0x00200000
#define NDIS_MINIPORT_HARDWARE_DEVICE                 0x00400000
#define NDIS_MINIPORT_SUPPORTS_CANCEL_SEND_PACKETS    0x00800000
#define NDIS_MINIPORT_64BITS_DMA                      0x01000000

/* Full duplex driver */
#define NDIS_MAC_OPTION_FULL_DUPLEX                      0x00000010 /* Deprecated flag */

#define NDIS_MAC_OPTION_EOTX_INDICATION                  0x00000020
#define NDIS_MAC_OPTION_8021P_PRIORITY                   0x00000040
#define NDIS_MAC_OPTION_SUPPORTS_MAC_ADRESS_OVERWRITE    0x00000080
#define NDIS_MAC_OPTION_RECEIVE_AT_DPC                   0x00000100
#define NDIS_MAC_OPTION_8021Q_VLAN                       0x00000200
#define NDIS_MAC_OPTION_RESERVED                         0x80000000

#define IOCTL_NDIS_QUERY_GLOBAL_STATS CTL_CODE(FILE_DEVICE_PHYSICAL_NETCARD, \
                                               0,                            \
                                               METHOD_OUT_DIRECT,            \
                                               FILE_ANY_ACCESS)

/* Hardware status codes (OID_GEN_HARDWARE_STATUS) */
typedef enum _NDIS_HARDWARE_STATUS {
  NdisHardwareStatusReady,
  NdisHardwareStatusInitializing,
  NdisHardwareStatusReset,
  NdisHardwareStatusClosing,
  NdisHardwareStatusNotReady
} NDIS_HARDWARE_STATUS, *PNDIS_HARDWARE_STATUS;

/* OID_GEN_GET_TIME_CAPS */
typedef struct _GEN_GET_TIME_CAPS {
  ULONG Flags;
  ULONG ClockPrecision;
} GEN_GET_TIME_CAPS, *PGEN_GET_TIME_CAPS;

/* OID_GEN_GET_NETCARD_TIME */
typedef struct _GEN_GET_NETCARD_TIME {
  ULONGLONG ReadTime;
} GEN_GET_NETCARD_TIME, *PGEN_GET_NETCARD_TIME;

/* State of the LAN media (OID_GEN_MEDIA_CONNECT_STATUS) */
typedef enum _NDIS_MEDIA_STATE {
  NdisMediaStateConnected,
  NdisMediaStateDisconnected
} NDIS_MEDIA_STATE, *PNDIS_MEDIA_STATE;

#ifndef _NDIS_
typedef int NDIS_STATUS, *PNDIS_STATUS;
#endif

/* OID_GEN_SUPPORTED_GUIDS */
typedef struct _NDIS_GUID {
  GUID Guid;
  union {
    NDIS_OID Oid;
    NDIS_STATUS Status;
  } u;
  ULONG Size;
  ULONG Flags;
} NDIS_GUID, *PNDIS_GUID;

typedef struct _NDIS_PM_PACKET_PATTERN {
  ULONG Priority;
  ULONG Reserved;
  ULONG MaskSize;
  ULONG PatternOffset;
  ULONG PatternSize;
  ULONG PatternFlags;
} NDIS_PM_PACKET_PATTERN, *PNDIS_PM_PACKET_PATTERN;

/* OID_GEN_NETWORK_LAYER_ADDRESSES */
typedef struct _NETWORK_ADDRESS {
  USHORT AddressLength;
  USHORT AddressType;
  UCHAR Address[1];
} NETWORK_ADDRESS, *PNETWORK_ADDRESS;

typedef struct _NETWORK_ADDRESS_LIST {
  LONG AddressCount;
  USHORT AddressType;
  NETWORK_ADDRESS Address[1];
} NETWORK_ADDRESS_LIST, *PNETWORK_ADDRESS_LIST;

/* OID_GEN_TRANSPORT_HEADER_OFFSET */
typedef struct _TRANSPORT_HEADER_OFFSET {
  USHORT ProtocolType;
  USHORT HeaderOffset;
} TRANSPORT_HEADER_OFFSET, *PTRANSPORT_HEADER_OFFSET;

/* OID_GEN_CO_LINK_SPEED / OID_GEN_CO_MINIMUM_LINK_SPEED */
typedef struct _NDIS_CO_LINK_SPEED {
  ULONG Outbound;
  ULONG Inbound;
} NDIS_CO_LINK_SPEED, *PNDIS_CO_LINK_SPEED;

#ifdef __cplusplus
}
#endif

#endif /* _NTDDNDIS_ */
