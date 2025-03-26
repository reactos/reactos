#ifndef _FBT_HCI_CMD_STRUCTS_H
#define _FBT_HCI_CMD_STRUCTS_H

// Pack structures to single unsigned char boundries
#pragma pack(push, 1)

// Command Header
typedef struct
{
    unsigned short	OpCode;
    unsigned char	ParameterLength;

} FBT_HCI_CMD_HEADER, *PFBT_HCI_CMD_HEADER;

//  Link control commands
typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		LAP[FBT_HCI_LAP_SIZE];
    unsigned char		InquiryLength;
    unsigned char		NumResponses;

} FBT_HCI_INQUIRY, *PFBT_HCI_INQUIRY;

typedef struct
{
	FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_INQUIRY_CANCEL, *PFBT_HCI_INQUIRY_CANCEL;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		MaxPeriodLength;
    unsigned short		MinPeriodLength;
    unsigned char		LAP[FBT_HCI_LAP_SIZE];
    unsigned char		InquiryLength;
    unsigned char		NumResponses;

} FBT_HCI_PERIODIC_INQUIRY_MODE, *PFBT_HCI_PERIODIC_INQUIRY_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_EXIT_PERIODIC_INQUIRY_MODE, *PFBT_HCI_EXIT_PERIODIC_INQUIRY_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned short		PacketType;
    unsigned char		PageScanRepetitionMode;
    unsigned char		PageScanMode;
    unsigned short		ClockOffset;
    unsigned char		AllowRoleSwitch;

} FBT_HCI_CREATE_CONNECTION, *PFBT_HCI_CREATE_CONNECTION;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;
    unsigned char		Reason;

} FBT_HCI_DISCONNECT, *PFBT_HCI_DISCONNECT;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;
    unsigned short		PacketType;

} FBT_HCI_ADD_SCO_CONNECTION, *PFBT_HCI_ADD_SCO_CONNECTION;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char		Role;

} FBT_HCI_ACCEPT_CONNECTION_REQUEST, *PFBT_HCI_ACCEPT_CONNECTION_REQUEST;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char		Reason;

} FBT_HCI_REJECT_CONNECTION_REQUEST, *PFBT_HCI_REJECT_CONNECTION_REQUEST;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char		LinkKey[FBT_HCI_LINK_KEY_SIZE];

} FBT_HCI_LINK_KEY_REQUEST_REPLY, *PFBT_HCI_LINK_KEY_REQUEST_REPLY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		BD_ADDR[FBT_HCI_BDADDR_SIZE];

} FBT_HCI_LINK_KEY_REQUEST_NEGATIVE_REPLY, *PFBT_HCI_LINK_KEY_REQUEST_NEGATIVE_REPLY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char		PINCodeLength;
    unsigned char		PINCode[FBT_HCI_PIN_CODE_SIZE];

} FBT_HCI_PIN_CODE_REQUEST_REPLY, *PFBT_HCI_PIN_CODE_REQUEST_REPLY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		BD_ADDR[FBT_HCI_BDADDR_SIZE];

} FBT_HCI_PIN_CODE_REQUEST_NEGATIVE_REPLY, *PFBT_HCI_PIN_CODE_REQUEST_NEGATIVE_REPLY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;
    unsigned short		PacketType;

} FBT_HCI_CHANGE_CONNECTION_PACKET_TYPE, *PFBT_HCI_CHANGE_CONNECTION_PACKET_TYPE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_AUTHENTICATION_REQUESTED, *PFBT_HCI_AUTHENTICATION_REQUESTED;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;
    unsigned char		EncryptionEnable;

} FBT_HCI_SET_CONNECTION_ENCRYPTION, *PFBT_HCI_SET_CONNECTION_ENCRYPTION;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_CHANGE_CONNECTION_LINK_KEY, *PFBT_HCI_CHANGE_CONNECTION_LINK_KEY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		KeyFlag;

} FBT_HCI_MASTER_LINK_KEY, *PFBT_HCI_MASTER_LINK_KEY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char		PageScanRepetitionMode;
    unsigned char		PageScanMode;
    unsigned short		ClockOffset;

} FBT_HCI_REMOTE_NAME_REQUEST, *PFBT_HCI_REMOTE_NAME_REQUEST;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_READ_REMOTE_SUPPORTED_FEATURES, *PFBT_HCI_READ_REMOTE_SUPPORTED_FEATURES;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_READ_REMOTE_VERSION_INFORMATION, *PFBT_HCI_READ_REMOTE_VERSION_INFORMATION;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_READ_CLOCK_OFFSET, *PFBT_HCI_READ_CLOCK_OFFSET;


//  Link policy commands
typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;
    unsigned short		HoldModeMaxInterval;
    unsigned short		HoldModeMinInterval;

} FBT_HCI_HOLD_MODE, *PFBT_HCI_HOLD_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;
    unsigned short		SniffMaxInterval;
    unsigned short		SniffMinInterval;
    unsigned short		SniffAttempt;
    unsigned short		SniffTimeout;

} FBT_HCI_SNIFF_MODE, *PFBT_HCI_SNIFF_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_EXIT_SNIFF_MODE, *PFBT_HCI_EXIT_SNIFF_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;
    unsigned short		BeaconMaxInterval;
    unsigned short		BeaconMinInterval;

} FBT_HCI_PARK_MODE, *PFBT_HCI_PARK_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_EXIT_PARK_MODE, *PFBT_HCI_EXIT_PARK_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;
    unsigned char		Flags;
    unsigned char		ServiceType;
    unsigned long		TokenRate;
    unsigned long		PeakBandwidth;
    unsigned long		Latency;
    unsigned long		DelayVariation;

} FBT_HCI_QOS_SETUP, *PFBT_HCI_QOS_SETUP;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_ROLE_DISCOVERY, *PFBT_HCI_ROLE_DISCOVERY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char		Role;

} FBT_HCI_SWITCH_ROLE, *PFBT_HCI_SWITCH_ROLE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_READ_LINK_POLICY_SETTINGS, *PFBT_HCI_READ_LINK_POLICY_SETTINGS;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;
    unsigned short		LinkPolicySettings;

} FBT_HCI_WRITE_LINK_POLICY_SETTINGS, *PFBT_HCI_WRITE_LINK_POLICY_SETTINGS;


//  Host Controller and Baseband commands
typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		EventMask[8];

} FBT_HCI_SET_EVENT_MASK, *PFBT_HCI_SET_EVENT_MASK;

typedef struct
{
	FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_RESET, *PFBT_HCI_RESET;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		FilterType;
    unsigned char		FilterConditionType;
    unsigned char		Condition[7];

} FBT_HCI_SET_EVENT_FILTER, *PFBT_HCI_SET_EVENT_FILTER;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_FLUSH, *PFBT_HCI_FLUSH;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_PIN_TYPE, *PFBT_HCI_READ_PIN_TYPE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char			PinType;

} FBT_HCI_WRITE_PIN_TYPE, *PFBT_HCI_WRITE_PIN_TYPE;

typedef struct
{
	FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_CREATE_NEW_UNIT_KEY, *PFBT_HCI_CREATE_NEW_UNIT_KEY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char		ReadAllFlag;

} FBT_HCI_READ_STORED_LINK_KEY, *PFBT_HCI_READ_STORED_LINK_KEY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		NumKeysToWrite;
    unsigned char		BD_ADDR[FBT_HCI_VARIABLE_SIZE][FBT_HCI_BDADDR_SIZE];
    unsigned char		LinkKey[FBT_HCI_VARIABLE_SIZE][FBT_HCI_LINK_KEY_SIZE];

} FBT_HCI_WRITE_STORED_LINK_KEY, *PFBT_HCI_WRITE_STORED_LINK_KEY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		BD_ADDR[FBT_HCI_BDADDR_SIZE];
    unsigned char		DeleteAllFlag;

} FBT_HCI_DELETE_STORED_LINK_KEY, *PFBT_HCI_DELETE_STORED_LINK_KEY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		Name[FBT_HCI_NAME_SIZE];

} FBT_HCI_CHANGE_LOCAL_NAME, *PFBT_HCI_CHANGE_LOCAL_NAME;

typedef struct
{
	FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_LOCAL_NAME, *PFBT_HCI_READ_LOCAL_NAME;

typedef struct
{
	FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_CONNECTION_ACCEPT_TIMEOUT, *PFBT_HCI_READ_CONNECTION_ACCEPT_TIMEOUT;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnAcceptTimeout;

} FBT_HCI_WRITE_CONNECTION_ACCEPT_TIMEOUT, *PFBT_HCI_WRITE_CONNECTION_ACCEPT_TIMEOUT;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_PAGE_TIMEOUT, *PFBT_HCI_READ_PAGE_TIMEOUT;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		PageTimeout;

} FBT_HCI_WRITE_PAGE_TIMEOUT, *PFBT_HCI_WRITE_PAGE_TIMEOUT;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_SCAN_ENABLE, *PFBT_HCI_READ_SCAN_ENABLE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		ScanEnable;

} FBT_HCI_WRITE_SCAN_ENABLE, *PFBT_HCI_WRITE_SCAN_ENABLE;

typedef struct
{
	FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_PAGE_SCAN_ACTIVITY, *PFBT_HCI_READ_PAGE_SCAN_ACTIVITY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		PageScanInterval;
    unsigned short		PageScanWindow;

} FBT_HCI_WRITE_PAGE_SCAN_ACTIVITY, *PFBT_HCI_WRITE_PAGE_SCAN_ACTIVITY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_INQUIRY_SCAN_ACTIVITY, *PFBT_HCI_READ_INQUIRY_SCAN_ACTIVITY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		InquiryScanInterval;
    unsigned short		InquiryScanWindow;

} FBT_HCI_WRITE_INQUIRY_SCAN_ACTIVITY, *PFBT_HCI_WRITE_INQUIRY_SCAN_ACTIVITY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_AUTHENTICATION_ENABLE, *PFBT_HCI_READ_AUTHENTICATION_ENABLE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		AuthenticationEnable;

} FBT_HCI_WRITE_AUTHENTICATION_ENABLE, *PFBT_HCI_WRITE_AUTHENTICATION_ENABLE;

typedef struct
{
	FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_ENCRYPTION_MODE, *PFBT_HCI_READ_ENCRYPTION_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		EncryptionMode;

} FBT_HCI_WRITE_ENCRYPTION_MODE, *PFBT_HCI_WRITE_ENCRYPTION_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_CLASS_OF_DEVICE, *PFBT_HCI_READ_CLASS_OF_DEVICE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		ClassOfDevice[FBT_HCI_DEVICE_CLASS_SIZE];

} FBT_HCI_WRITE_CLASS_OF_DEVICE, *PFBT_HCI_WRITE_CLASS_OF_DEVICE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_VOICE_SETTING, *PFBT_HCI_READ_VOICE_SETTING;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		VoiceSetting;

} FBT_HCI_WRITE_VOICE_SETTING, *PFBT_HCI_WRITE_VOICE_SETTING;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_READ_AUTOMATIC_FLUSH_TIMEOUT, *PFBT_HCI_READ_AUTOMATIC_FLUSH_TIMEOUT;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;
    unsigned short		FlushTimeout;

} FBT_HCI_WRITE_AUTOMATIC_FLUSH_TIMEOUT, *PFBT_HCI_WRITE_AUTOMATIC_FLUSH_TIMEOUT;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_NUM_BROADCAST_RETRANSMISSIONS, *PFBT_HCI_READ_NUM_BROADCAST_RETRANSMISSIONS;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		NumBroadcastRetran;

} FBT_HCI_WRITE_NUM_BROADCAST_RETRANSMISSIONS, *PFBT_HCI_WRITE_NUM_BROADCAST_RETRANSMISSIONS;

typedef struct
{
	FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_HOLD_MODE_ACTIVITY, *PFBT_HCI_READ_HOLD_MODE_ACTIVITY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		HoldModeActivity;

} FBT_HCI_WRITE_HOLD_MODE_ACTIVITY, *PFBT_HCI_WRITE_HOLD_MODE_ACTIVITY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;
    unsigned char		Type;

} FBT_HCI_READ_TRANSMIT_POWER_LEVEL, *PFBT_HCI_READ_TRANSMIT_POWER_LEVEL;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_SCO_FLOW_CONTROL_ENABLE, *PFBT_HCI_READ_SCO_FLOW_CONTROL_ENABLE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		ScoFlowControlEnable;

} FBT_HCI_WRITE_SCO_FLOW_CONTROL_ENABLE, *PFBT_HCI_WRITE_SCO_FLOW_CONTROL_ENABLE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		FlowControlEnable;

} FBT_HCI_SET_HOST_CONTROLLER_TO_HOST_FLOW_CONTROL, *PFBT_HCI_SET_HOST_CONTROLLER_TO_HOST_FLOW_CONTROL;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		AclDataPacketLength;
    unsigned char		ScoDataPacketLength;
    unsigned short		TotalNumAclDataPackets;
    unsigned short		TotalNumScoDataPackets;

} FBT_HCI_HOST_BUFFER_SIZE, *PFBT_HCI_HOST_BUFFER_SIZE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		NumberOfHandles;
    unsigned short		ConnectionHandle[FBT_HCI_VARIABLE_SIZE];
    unsigned short		HostNumOfCompletedPackets[FBT_HCI_VARIABLE_SIZE];

} FBT_HCI_HOST_NUMBER_OF_COMPLETED_PACKETS, *PFBT_HCI_HOST_NUMBER_OF_COMPLETED_PACKETS;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_READ_LINK_SUPERVISION_TIMEOUT, *PFBT_HCI_READ_LINK_SUPERVISION_TIMEOUT;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;
    unsigned short		LinkSupervisionTimeout;

} FBT_HCI_WRITE_LINK_SUPERVISION_TIMEOUT, *PFBT_HCI_WRITE_LINK_SUPERVISION_TIMEOUT;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_NUMBER_OF_SUPPORTED_IAC, *PFBT_HCI_READ_NUMBER_OF_SUPPORTED_IAC;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_CURRENT_IAC_LAP, *PFBT_HCI_READ_CURRENT_IAC_LAP;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		NumCurrentIac;
    unsigned char		IacLap[FBT_HCI_VARIABLE_SIZE][FBT_HCI_LAP_SIZE];

} FBT_HCI_WRITE_CURRENT_IAC_LAP, *PFBT_HCI_WRITE_CURRENT_IAC_LAP;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_PAGE_SCAN_PERIOD_MODE, *PFBT_HCI_READ_PAGE_SCAN_PERIOD_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		PageScanPeriodMode;

} FBT_HCI_WRITE_PAGE_SCAN_PERIOD_MODE, *PFBT_HCI_WRITE_PAGE_SCAN_PERIOD_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_PAGE_SCAN_MODE, *PFBT_HCI_READ_PAGE_SCAN_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		PageScanMode;

} FBT_HCI_WRITE_PAGE_SCAN_MODE, *PFBT_HCI_WRITE_PAGE_SCAN_MODE;


//  Informational parameters
typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_LOCAL_VERSION_INFORMATION, *PFBT_HCI_READ_LOCAL_VERSION_INFORMATION;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_LOCAL_SUPPORTED_FEATURES, *PFBT_HCI_READ_LOCAL_SUPPORTED_FEATURES;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_BUFFER_SIZE, *PFBT_HCI_READ_BUFFER_SIZE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_COUNTRY_CODE, *PFBT_HCI_READ_COUNTRY_CODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_BD_ADDR, *PFBT_HCI_READ_BD_ADDR;


//  Status parameter commands

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_READ_FAILED_CONTACT_COUNTER, *PFBT_HCI_READ_FAILED_CONTACT_COUNTER;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_RESET_FAILED_CONTACT_COUNTER, *PFBT_HCI_RESET_FAILED_CONTACT_COUNTER;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_GET_LINK_QUALITY, *PFBT_HCI_GET_LINK_QUALITY;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned short		ConnectionHandle;

} FBT_HCI_READ_RSSI, *PFBT_HCI_READ_RSSI;


//  Testing commands
typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_READ_LOOPBACK_MODE, *PFBT_HCI_READ_LOOPBACK_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;
    unsigned char		LoopbackMode;

} FBT_HCI_WRITE_LOOPBACK_MODE, *PFBT_HCI_WRITE_LOOPBACK_MODE;

typedef struct
{
    FBT_HCI_CMD_HEADER	CommandHeader;

} FBT_HCI_ENABLE_DEVICE_UNDER_TEST_MODE, *PFBT_HCI_ENABLE_DEVICE_UNDER_TEST_MODE;

#pragma pack(pop)

#endif // _FBT_HCI_CMD_STRUCTS_H
