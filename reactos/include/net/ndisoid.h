/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        include/net/ndisoid.h
 * PURPOSE:     NDIS Object ID constants
 */
#ifndef __NDISOID_H
#define __NDISOID_H


typedef ULONG NDIS_OID, *PNDIS_OID;

/* Self-contained variable data structure */
typedef struct _NDIS_VAR_DATA_DESC
{
    USHORT  Length;	       /* Number of bytes of data */
    USHORT  MaximumLength; /* Number of bytes available */
    LONG    Offset;        /* Offset of data relative to the descriptor */
} NDIS_VAR_DATA_DESC, *PNDIS_VAR_DATA_DESC;



/* NDIS 4.0 structures */

/* Structure used by TRANSLATE_NAME IOCTL */
typedef struct _NET_PNP_ID
{
    ULONG   ClassId;
    ULONG   Token;
} NET_PNP_ID, *PNET_PNP_ID;


typedef struct _NET_PNP_TRANSLATE_LIST
{
    ULONG       BytesNeeded;
    NET_PNP_ID  IdArray[ANYSIZE_ARRAY];
} NET_PNP_TRANSLATE_LIST, *PNET_PNP_TRANSLATE_LIST;



/* Generel objects */

/* General operational characteristics */

/* Mandatory */
#define OID_GEN_SUPPORTED_LIST                  0x00010101
#define OID_GEN_HARDWARE_STATUS                 0x00010102
#define OID_GEN_MEDIA_SUPPORTED                 0x00010103
#define OID_GEN_MEDIA_IN_USE                    0x00010104
#define OID_GEN_MAXIMUM_LOOKAHEAD               0x00010105
#define OID_GEN_MAXIMUM_FRAME_SIZE              0x00010106
#define OID_GEN_LINK_SPEED                      0x00010107
#define OID_GEN_TRANSMIT_BUFFER_SPACE           0x00010108
#define OID_GEN_RECEIVE_BUFFER_SPACE            0x00010109
#define OID_GEN_TRANSMIT_BLOCK_SIZE             0x0001010A
#define OID_GEN_RECEIVE_BLOCK_SIZE              0x0001010B
#define OID_GEN_VENDOR_ID                       0x0001010C
#define OID_GEN_VENDOR_DESCRIPTION              0x0001010D
#define OID_GEN_CURRENT_PACKET_FILTER           0x0001010E
#define OID_GEN_CURRENT_LOOKAHEAD               0x0001010F
#define OID_GEN_DRIVER_VERSION                  0x00010110
#define OID_GEN_MAXIMUM_TOTAL_SIZE              0x00010111
#define OID_GEN_PROTOCOL_OPTIONS                0x00010112
#define OID_GEN_MAC_OPTIONS                     0x00010113
#define OID_GEN_MEDIA_CONNECT_STATUS            0x00010114
#define OID_GEN_MAXIMUM_SEND_PACKETS            0x00010115
#define OID_GEN_VENDOR_DRIVER_VERSION           0x00010116

/* Optional */
#define	OID_GEN_SUPPORTED_GUIDS                 0x00010117
#define	OID_GEN_NETWORK_LAYER_ADDRESSES         0x00010118
#define OID_GEN_TRANSPORT_HEADER_OFFSET         0x00010119

/* General statistics */

/* Mandatory */
#define OID_GEN_XMIT_OK                         0x00020101
#define OID_GEN_RCV_OK                          0x00020102
#define OID_GEN_XMIT_ERROR                      0x00020103
#define OID_GEN_RCV_ERROR                       0x00020104
#define OID_GEN_RCV_NO_BUFFER                   0x00020105

/* Optional */
#define OID_GEN_DIRECTED_BYTES_XMIT             0x00020201
#define OID_GEN_DIRECTED_FRAMES_XMIT            0x00020202
#define OID_GEN_MULTICAST_BYTES_XMIT            0x00020203
#define OID_GEN_MULTICAST_FRAMES_XMIT           0x00020204
#define OID_GEN_BROADCAST_BYTES_XMIT            0x00020205
#define OID_GEN_BROADCAST_FRAMES_XMIT           0x00020206
#define OID_GEN_DIRECTED_BYTES_RCV              0x00020207
#define OID_GEN_DIRECTED_FRAMES_RCV             0x00020208
#define OID_GEN_MULTICAST_BYTES_RCV             0x00020209
#define OID_GEN_MULTICAST_FRAMES_RCV            0x0002020A
#define OID_GEN_BROADCAST_BYTES_RCV             0x0002020B
#define OID_GEN_BROADCAST_FRAMES_RCV            0x0002020C
#define OID_GEN_RCV_CRC_ERROR                   0x0002020D
#define OID_GEN_TRANSMIT_QUEUE_LENGTH           0x0002020E


/* Ethernet objects */

/* Ethernet operational characteristics */

/* Mandatory */
#define OID_802_3_PERMANENT_ADDRESS             0x01010101
#define OID_802_3_CURRENT_ADDRESS               0x01010102
#define OID_802_3_MULTICAST_LIST                0x01010103
#define OID_802_3_MAXIMUM_LIST_SIZE             0x01010104

/* Optional */
#define OID_802_3_MAC_OPTIONS                   0x01010105

/* Ethernet statistics */

/* Mandatory */
#define OID_802_3_RCV_ERROR_ALIGNMENT           0x01020101
#define OID_802_3_XMIT_ONE_COLLISION            0x01020102
#define OID_802_3_XMIT_MORE_COLLISIONS          0x01020103

/* Optional */
#define OID_802_3_XMIT_DEFERRED                 0x01020201
#define OID_802_3_XMIT_MAX_COLLISIONS           0x01020202
#define OID_802_3_RCV_OVERRUN                   0x01020203
#define OID_802_3_XMIT_UNDERRUN                 0x01020204
#define OID_802_3_XMIT_HEARTBEAT_FAILURE        0x01020205
#define OID_802_3_XMIT_TIMES_CRS_LOST           0x01020206
#define OID_802_3_XMIT_LATE_COLLISIONS          0x01020207


/* Token Ring objects */

/* Token Ring operational characteristics */

/* Mandatory */
#define OID_802_5_PERMANENT_ADDRESS             0x02010101
#define OID_802_5_CURRENT_ADDRESS               0x02010102
#define OID_802_5_CURRENT_FUNCTIONAL            0x02010103
#define OID_802_5_CURRENT_GROUP                 0x02010104
#define OID_802_5_LAST_OPEN_STATUS              0x02010105
#define OID_802_5_CURRENT_RING_STATUS           0x02010106
#define OID_802_5_CURRENT_RING_STATE            0x02010107

/* Token Ring statistics */

/* Mandatory */
#define OID_802_5_LINE_ERRORS                   0x02020101
#define OID_802_5_LOST_FRAMES                   0x02020102
#define OID_802_5_BURST_ERRORS                  0x02020201
#define OID_802_5_AC_ERRORS                     0x02020202
#define OID_802_5_ABORT_DELIMETERS              0x02020203
#define OID_802_5_FRAME_COPIED_ERRORS           0x02020204
#define OID_802_5_FREQUENCY_ERRORS              0x02020205
#define OID_802_5_TOKEN_ERRORS                  0x02020206
#define OID_802_5_INTERNAL_ERRORS               0x02020207


/* FDDI objects */

/* FDDI operational characteristics */

/* Mandatory */
#define OID_FDDI_LONG_PERMANENT_ADDR            0x03010101
#define OID_FDDI_LONG_CURRENT_ADDR              0x03010102
#define OID_FDDI_LONG_MULTICAST_LIST            0x03010103
#define OID_FDDI_LONG_MAX_LIST_SIZE             0x03010104
#define OID_FDDI_SHORT_PERMANENT_ADDR           0x03010105
#define OID_FDDI_SHORT_CURRENT_ADDR             0x03010106
#define OID_FDDI_SHORT_MULTICAST_LIST           0x03010107
#define OID_FDDI_SHORT_MAX_LIST_SIZE            0x03010108

/* FDDI statistics */

/* Mandatory */
#define OID_FDDI_ATTACHMENT_TYPE                0x03020101
#define OID_FDDI_UPSTREAM_NODE_LONG             0x03020102
#define OID_FDDI_DOWNSTREAM_NODE_LONG           0x03020103
#define OID_FDDI_FRAME_ERRORS                   0x03020104
#define OID_FDDI_FRAMES_LOST                    0x03020105
#define OID_FDDI_RING_MGT_STATE                 0x03020106
#define OID_FDDI_LCT_FAILURES                   0x03020107
#define OID_FDDI_LEM_REJECTS                    0x03020108
#define OID_FDDI_LCONNECTION_STATE              0x03020109


/* LocalTalk objects */

/* LocalTalk operational characteristics */

/* Mandatory */
#define OID_LTALK_CURRENT_NODE_ID               0x05010102

/* LocalTalk statistics */

/* Mandatory */
#define OID_LTALK_IN_BROADCASTS	                0x05020101
#define OID_LTALK_IN_LENGTH_ERRORS              0x05020102

/* Optional */
#define OID_LTALK_OUT_NO_HANDLERS               0x05020201
#define OID_LTALK_COLLISIONS                    0x05020202
#define OID_LTALK_DEFERS                        0x05020203
#define OID_LTALK_NO_DATA_ERRORS                0x05020204
#define OID_LTALK_RANDOM_CTS_ERRORS             0x05020205
#define OID_LTALK_FCS_ERRORS                    0x05020206


/* ARCNET objects */

/* ARCNET operational characteristics */

/* Mandatory */
#define OID_ARCNET_PERMANENT_ADDRESS            0x06010101
#define OID_ARCNET_CURRENT_ADDRESS              0x06010102

/* ARCNET statistics */

/* Optional */
#define OID_ARCNET_RECONFIGURATIONS             0x06020201


/* WAN objects */

/* Mandatory */
#define OID_WAN_PERMANENT_ADDRESS               0x04010101
#define OID_WAN_CURRENT_ADDRESS                 0x04010102
#define OID_WAN_QUALITY_OF_SERVICE              0x04010103
#define OID_WAN_PROTOCOL_TYPE                   0x04010104
#define OID_WAN_MEDIUM_SUBTYPE                  0x04010105
#define OID_WAN_HEADER_FORMAT                   0x04010106
#define OID_WAN_GET_INFO                        0x04010107
#define OID_WAN_SET_LINK_INFO                   0x04010108
#define OID_WAN_GET_LINK_INFO                   0x04010109
#define OID_WAN_LINE_COUNT                      0x0401010A
#define OID_WAN_PROTOCOL_CAPS                   0x0401010B
#define OID_WAN_GET_BRIDGE_INFO                 0x0401020A
#define OID_WAN_SET_BRIDGE_INFO                 0x0401020B

/* Optional */
#define OID_WAN_GET_COMP_INFO                   0x0401020C
#define OID_WAN_SET_COMP_INFO                   0x0401020D
#define OID_WAN_GET_STATS_INFO                  0x0401020E


/* TAPI objects */

/* Madatory */
#define OID_TAPI_ANSWER                         0x07030102
#define OID_TAPI_CLOSE                          0x07030103
#define OID_TAPI_CLOSE_CALL                     0x07030104
#define OID_TAPI_CONDITIONAL_MEDIA_DETECTION    0x07030105
#define OID_TAPI_DROP                           0x07030109
#define OID_TAPI_GET_ADDRESS_CAPS               0x0703010A
#define OID_TAPI_GET_ADDRESS_ID                 0x0703010B
#define OID_TAPI_GET_ADDRESS_STATUS             0x0703010C
#define OID_TAPI_GET_CALL_ADDRESS_ID            0x0703010D
#define OID_TAPI_GET_CALL_INFO                  0x0703010E
#define OID_TAPI_GET_CALL_STATUS                0x0703010F
#define OID_TAPI_GET_DEV_CAPS                   0x07030110
#define OID_TAPI_GET_DEV_CONFIG                 0x07030111
#define OID_TAPI_GET_ID                         0x07030113
#define OID_TAPI_GET_LINE_DEV_STATUS            0x07030114
#define OID_TAPI_MAKE_CALL                      0x07030115
#define OID_TAPI_OPEN                           0x07030117
#define OID_TAPI_PROVIDER_INITIALIZE            0x07030118
#define OID_TAPI_PROVIDER_SHUTDOWN              0x07030119
#define OID_TAPI_SET_APP_SPECIFIC               0x0703011D
#define OID_TAPI_SET_CALL_PARAMS                0x0703011E
#define OID_TAPI_SET_DEFAULT_MEDIA_DETECTION    0x0703011F
#define OID_TAPI_SET_MEDIA_MODE                 0x07030121
#define OID_TAPI_SET_STATUS_MESSAGES            0x07030122

/* Optional */
#define OID_TAPI_ACCEPT                         0x07030101
#define OID_TAPI_CONFIG_DIALOG                  0x07030106
#define OID_TAPI_DEV_SPECIFIC                   0x07030107
#define OID_TAPI_DIAL                           0x07030108
#define OID_TAPI_GET_EXTENSION_ID               0x07030112
#define OID_TAPI_NEGOTIATE_EXT_VERSION          0x07030116
#define OID_TAPI_SET_DEV_CONFIG                 0x07030120
#define OID_TAPI_SECURE_CALL                    0x0703011A
#define OID_TAPI_SELECT_EXT_VERSION             0x0703011B
#define OID_TAPI_SEND_USER_USER_INFO            0x0703011C


/* Wireless objects */

/* Mandatory */
#define OID_WW_GEN_NETWORK_TYPES_SUPPORTED      0x09010101
#define OID_WW_GEN_NETWORK_TYPE_IN_USE          0x09010102
#define OID_WW_GEN_HEADER_FORMATS_SUPPORTED     0x09010103
#define OID_WW_GEN_HEADER_FORMAT_IN_USE         0x09010104
#define OID_WW_GEN_INDICATION_REQUEST           0x09010105
#define OID_WW_GEN_DEVICE_INFO                  0x09010106
#define OID_WW_GEN_OPERATION_MODE               0x09010107
#define OID_WW_GEN_LOCK_STATUS                  0x09010108
#define OID_WW_GEN_DISABLE_TRANSMITTER          0x09010109
#define OID_WW_GEN_NETWORK_ID                   0x0901010A
#define OID_WW_GEN_PERMANENT_ADDRESS            0x0901010B
#define OID_WW_GEN_CURRENT_ADDRESS              0x0901010C
#define OID_WW_GEN_SUSPEND_DRIVER               0x0901010D
#define OID_WW_GEN_BASESTATION_ID               0x0901010E
#define OID_WW_GEN_CHANNEL_ID                   0x0901010F
#define OID_WW_GEN_ENCRYPTION_SUPPORTED         0x09010110
#define OID_WW_GEN_ENCRYPTION_IN_USE            0x09010111
#define OID_WW_GEN_ENCRYPTION_STATE             0x09010112
#define OID_WW_GEN_CHANNEL_QUALITY              0x09010113
#define OID_WW_GEN_REGISTRATION_STATUS          0x09010114
#define OID_WW_GEN_RADIO_LINK_SPEED             0x09010115
#define OID_WW_GEN_LATENCY                      0x09010116
#define OID_WW_GEN_BATTERY_LEVEL                0x09010117
#define OID_WW_GEN_EXTERNAL_POWER               0x09010118

/* Optional */
#define OID_WW_GEN_PING_ADDRESS	                0x09010201
#define OID_WW_GEN_RSSI                         0x09010202
#define OID_WW_GEN_SIM_STATUS                   0x09010203
#define OID_WW_GEN_ENABLE_SIM_PIN               0x09010204
#define OID_WW_GEN_CHANGE_SIM_PIN               0x09010205
#define OID_WW_GEN_SIM_PUK                      0x09010206
#define OID_WW_GEN_SIM_EXCEPTION                0x09010207

/* Metricom OIDs */
#define OID_WW_MET_FUNCTION                     0x09190101

/* DataTac OIDs */
#define OID_WW_TAC_COMPRESSION                  0x09150101

#define OID_WW_TAC_SET_CONFIG                   0x09150102
#define OID_WW_TAC_GET_STATUS                   0x09150103
#define OID_WW_TAC_USER_HEADER                  0x09150104

/* Ardis OIDs */

#define OID_WW_ARD_SNDCP                        0x09110101
#define OID_WW_ARD_TMLY_MSG	                    0x09110102
#define OID_WW_ARD_DATAGRAM                     0x09110103

/* CDPD OIDs */

#define OID_WW_CDPD_CIRCUIT_SWITCHED            0x090D010E
#define	OID_WW_CDPD_TEI                         0x090D010F
#define	OID_WW_CDPD_RSSI                        0x090D0110

#define OID_WW_CDPD_CS_SERVICE_PREFERENCE       0x090D0111
#define OID_WW_CDPD_CS_SERVICE_STATUS           0x090D0112
#define OID_WW_CDPD_CS_INFO	                    0x090D0113
#define OID_WW_CDPD_CS_SUSPEND                  0x090D0114
#define OID_WW_CDPD_CS_DEFAULT_DIAL_CODE        0x090D0115
#define OID_WW_CDPD_CS_CALLBACK	                0x090D0116
#define OID_WW_CDPD_CS_SID_LIST	                0x090D0117
#define OID_WW_CDPD_CS_CONFIGURATION            0x090D0118

/* Pinpoint OIDs */

#define OID_WW_PIN_LOC_AUTHORIZE                0x09090101
#define OID_WW_PIN_LAST_LOCATION                0x09090102
#define OID_WW_PIN_LOC_FIX                      0x09090103

/* Mobitex OIDs */
#define OID_WW_MBX_SUBADDR                      0x09050101
/*  OID 0x09050102 is reserved and may not be used */
#define OID_WW_MBX_FLEXLIST	                    0x09050103
#define OID_WW_MBX_GROUPLIST                    0x09050104
#define OID_WW_MBX_TRAFFIC_AREA	                0x09050105
#define OID_WW_MBX_LIVE_DIE	                    0x09050106
#define OID_WW_MBX_TEMP_DEFAULTLIST             0x09050107


/* Connection-oriented objects */

/* Connection-oriented operational characteristics */

/* Mandatory */
#define OID_GEN_CO_SUPPORTED_LIST               OID_GEN_SUPPORTED_LIST
#define OID_GEN_CO_HARDWARE_STATUS              OID_GEN_HARDWARE_STATUS
#define OID_GEN_CO_MEDIA_SUPPORTED              OID_GEN_MEDIA_SUPPORTED
#define OID_GEN_CO_MEDIA_IN_USE	                OID_GEN_MEDIA_IN_USE
#define OID_GEN_CO_LINK_SPEED                   OID_GEN_LINK_SPEED
#define OID_GEN_CO_VENDOR_ID                    OID_GEN_VENDOR_ID
#define OID_GEN_CO_VENDOR_DESCRIPTION           OID_GEN_VENDOR_DESCRIPTION
#define OID_GEN_CO_DRIVER_VERSION               OID_GEN_DRIVER_VERSION
#define OID_GEN_CO_PROTOCOL_OPTIONS	            OID_GEN_PROTOCOL_OPTIONS
#define OID_GEN_CO_MAC_OPTIONS                  OID_GEN_MAC_OPTIONS
#define OID_GEN_CO_MEDIA_CONNECT_STATUS	        OID_GEN_MEDIA_CONNECT_STATUS
#define OID_GEN_CO_VENDOR_DRIVER_VERSION        OID_GEN_VENDOR_DRIVER_VERSION

/* Optional */
#define	OID_GEN_CO_SUPPORTED_GUIDS              OID_GEN_SUPPORTED_GUIDS
#define OID_GEN_CO_GET_TIME_CAPS                OID_GEN_GET_TIME_CAPS
#define OID_GEN_CO_GET_NETCARD_TIME	            OID_GEN_GET_NETCARD_TIME
#define OID_GEN_CO_MINIMUM_LINK_SPEED           0x00020120

/* Connection-oriented statistics */

#define	OID_GEN_CO_XMIT_PDUS_OK	                OID_GEN_XMIT_OK
#define	OID_GEN_CO_RCV_PDUS_OK                  OID_GEN_RCV_OK
#define	OID_GEN_CO_XMIT_PDUS_ERROR              OID_GEN_XMIT_ERROR
#define	OID_GEN_CO_RCV_PDUS_ERROR               OID_GEN_RCV_ERROR
#define	OID_GEN_CO_RCV_PDUS_NO_BUFFER           OID_GEN_RCV_NO_BUFFER
#define	OID_GEN_CO_RCV_CRC_ERROR                OID_GEN_RCV_CRC_ERROR
#define OID_GEN_CO_TRANSMIT_QUEUE_LENGTH        OID_GEN_TRANSMIT_QUEUE_LENGTH
#define	OID_GEN_CO_BYTES_XMIT                   OID_GEN_DIRECTED_BYTES_XMIT
#define OID_GEN_CO_BYTES_RCV                    OID_GEN_DIRECTED_BYTES_RCV
#define	OID_GEN_CO_NETCARD_LOAD	                OID_GEN_NETCARD_LOAD
#define	OID_GEN_CO_DEVICE_PROFILE               OID_GEN_DEVICE_PROFILE
#define	OID_GEN_CO_BYTES_XMIT_OUTSTANDING       0x00020221

#endif /* __NDISOID_H */

/* EOF */
