#ifndef _FBT_HCI_EVENT_STRUCTS_H
#define _FBT_HCI_EVENT_STRUCTS_H

// Pack structures to single unsigned char boundries
#pragma pack(push, 1)

typedef struct
{
    unsigned char EventCode;
    unsigned char ParameterLength;

} FBT_HCI_EVENT_HEADER, *PFBT_HCI_EVENT_HEADER;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned char			NumResponses;

} FBT_HCI_INQUIRY_COMPLETE, *PFBT_HCI_INQUIRY_COMPLETE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			NumResponses;
    unsigned char			BD_ADDR[FBT_HCI_VARIABLE_SIZE][FBT_HCI_BDADDR_SIZE];
    unsigned char			PageScanRepetitionMode[FBT_HCI_VARIABLE_SIZE];
    unsigned char			PageScanPeriodMode[FBT_HCI_VARIABLE_SIZE];
    unsigned char			PageScanMode[FBT_HCI_VARIABLE_SIZE];
    unsigned char			ClassOfDevice[FBT_HCI_VARIABLE_SIZE][FBT_HCI_DEVICE_CLASS_SIZE];
    unsigned short			ClockOffset[FBT_HCI_VARIABLE_SIZE];

} FBT_HCI_INQUIRY_RESULT, *PFBT_HCI_INQUIRY_RESULT;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned short			ConnectionHandle;
    unsigned char			BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char			LinkType;
    unsigned char			EncryptionMode;

} FBT_HCI_CONNECTION_COMPLETE, *PFBT_HCI_CONNECTION_COMPLETE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned long			ClassOfDevice[FBT_HCI_DEVICE_CLASS_SIZE];
    unsigned char			LinkType;

} FBT_HCI_CONNECTION_REQUEST, *PFBT_HCI_CONNECTION_REQUEST;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned short			ConnectionHandle;
    unsigned char			Reason;

} FBT_HCI_DISCONNECTION_COMPLETE, *PFBT_HCI_DISCONNECTION_COMPLETE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned short			ConnectionHandle;

} FBT_HCI_AUTHENTICATION_COMPLETE, *PFBT_HCI_AUTHENTICATION_COMPLETE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned char			BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char			RemoteName[FBT_HCI_NAME_SIZE];

} FBT_HCI_REMOTE_NAME_REQUEST_COMPLETE, *PFBT_HCI_REMOTE_NAME_REQUEST_COMPLETE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned short			ConnectionHandle;
    unsigned char			EncryptionEnable;

} FBT_HCI_ENCRYPTION_CHANGE, *PFBT_HCI_ENCRYPTION_CHANGE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned short			ConnectionHandle;

} FBT_HCI_CHANGE_CONNECTION_LINK_KEY_COMPLETE, *PFBT_HCI_CHANGE_CONNECTION_LINK_KEY_COMPLETE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned short			ConnectionHandle;
    unsigned char			KeyFlag;

} FBT_HCI_MASTER_LINK_KEY_COMPLETE, *PFBT_HCI_MASTER_LINK_KEY_COMPLETE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned short			ConnectionHandle;
    unsigned char			LmpFeatures[8];

} FBT_HCI_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE, *PFBT_HCI_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned short			ConnectionHandle;
    unsigned char			LmpVersion;
    unsigned short			ManufacturerName;
    unsigned short			LmpSubversion;

} FBT_HCI_READ_REMOTE_VERSION_INFORMATION_COMPLETE, *PFBT_HCI_READ_REMOTE_VERSION_INFORMATION_COMPLETE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned short			ConnectionHandle;
    unsigned char			Flags;
    unsigned char			ServiceType;
    unsigned long			TokenRate;
    unsigned long			PeakBandwidth;
    unsigned long			Latency;
    unsigned long			DelayVariation;

} FBT_HCI_QOS_SETUP_COMPLETE, *PFBT_HCI_QOS_SETUP_COMPLETE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			NumHCICommandPackets;
    unsigned short			OpCode;
    unsigned char			Parameters[FBT_HCI_VARIABLE_SIZE];

} FBT_HCI_COMMAND_COMPLETE, *PFBT_HCI_COMMAND_COMPLETE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned char			NumHCICommandPackets;
    unsigned short			OpCode;

} FBT_HCI_COMMAND_STATUS, *PFBT_HCI_COMMAND_STATUS;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			HardwareCode;

} FBT_HCI_HARDWARE_ERROR, *PFBT_HCI_HARDWARE_ERROR;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned short			ConnectionHandle;

} FBT_HCI_FLUSH_OCCURRED, *PFBT_HCI_FLUSH_OCCURRED;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned char			BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char			NewRole;

} FBT_HCI_ROLE_CHANGE, *PFBT_HCI_ROLE_CHANGE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			NumberOfHandles;
    unsigned short			ConnectionHandle[FBT_HCI_VARIABLE_SIZE];
    unsigned short			NumberOfCompletedPackets[FBT_HCI_VARIABLE_SIZE];

} FBT_HCI_NUMBER_OF_COMPLETED_PACKETS, *PFBT_HCI_NUMBER_OF_COMPLETED_PACKETS;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned short			ConnectionHandle;
    unsigned char			CurrentMode;
    unsigned short			Interval;

} FBT_HCI_MODE_CHANGE, *PFBT_HCI_MODE_CHANGE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			NumKeys;
    unsigned char			BD_ADDR[FBT_HCI_VARIABLE_SIZE][FBT_HCI_BDADDR_SIZE];
    unsigned char			LinkKey[FBT_HCI_VARIABLE_SIZE][FBT_HCI_LINK_KEY_SIZE];

} FBT_HCI_RETURN_LINK_KEYS, *PFBT_HCI_RETURN_LINK_KEYS;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			BD_ADDR[FBT_HCI_BDADDR_SIZE];

} FBT_HCI_PIN_CODE_REQUEST, *PFBT_HCI_PIN_CODE_REQUEST;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			BD_ADDR[FBT_HCI_BDADDR_SIZE];

} FBT_HCI_LINK_KEY_REQUEST, *PFBT_HCI_LINK_KEY_REQUEST;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char			LinkKey[FBT_HCI_LINK_KEY_SIZE];

} FBT_HCI_LINK_KEY_NOTIFICATION, *PFBT_HCI_LINK_KEY_NOTIFICATION;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			HCICommandPacket[FBT_HCI_CMD_MAX_SIZE];

} FBT_HCI_LOOPBACK_COMMAND, *PFBT_HCI_LOOPBACK_COMMAND;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			LinkType;

} FBT_HCI_DATA_BUFFER_OVERFLOW, *PFBT_HCI_DATA_BUFFER_OVERFLOW;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned short			ConnectionHandle;
    unsigned char			LmpMaxSlots;

} FBT_HCI_MAX_SLOTS_CHANGE, *PFBT_HCI_MAX_SLOTS_CHANGE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned short			ConnectionHandle;
    unsigned short			ClockOffset;

} FBT_HCI_READ_CLOCK_OFFSET_COMPLETE, *PFBT_HCI_READ_CLOCK_OFFSET_COMPLETE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			Status;
    unsigned short			ConnectionHandle;
    unsigned short			PacketType;

} FBT_HCI_CONNECTION_PACKET_TYPE_CHANGED, *PFBT_HCI_CONNECTION_PACKET_TYPE_CHANGED;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned short			ConnectionHandle;

} FBT_HCI_QOS_VIOLATION, *PFBT_HCI_QOS_VIOLATION;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char			PageScanMode;

} FBT_HCI_PAGE_SCAN_MODE_CHANGE, *PFBT_HCI_PAGE_SCAN_MODE_CHANGE;

typedef struct
{
    FBT_HCI_EVENT_HEADER	EventHeader;
    unsigned char			BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char			PageScanRepetitionMode;

} FBT_HCI_PAGE_SCAN_REPETITION_MODE_CHANGE, *PFBT_HCI_PAGE_SCAN_REPETITION_MODE_CHANGE;

typedef struct
{
	unsigned char			Status;
	unsigned char			HCIVersion;
	unsigned short			HCIRevision;
	unsigned char			LMPVersion;
	unsigned short			Manufacturer;
	unsigned short			LMPSubVersion;

} FBT_HCI_READ_LOCAL_VERSION_INFORMATION_COMPLETE;

// Data Packet Structure
typedef struct
{
    unsigned short  ConnectionHandle:	12;
    unsigned short  PacketBoundary:		2;
    unsigned short  Broadcast:			2;
    unsigned short  DataLength;
    unsigned char	Data[1];

} FBT_HCI_DATA_PACKET, *PFBT_HCI_DATA_PACKET;

#pragma pack(pop)

#endif // _FBT_HCI_EVENT_STRUCTS_H
