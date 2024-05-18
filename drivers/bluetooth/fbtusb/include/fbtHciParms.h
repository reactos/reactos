#ifndef _FBT_HCI_PARAMETERS_H
#define _FBT_HCI_PARAMETERS_H

// HCI header types
#define FBT_HCI_SYNC_HCI_COMMAND_PACKET		0x01
#define FBT_HCI_SYNC_ACL_DATA_PACKET        0x02
#define FBT_HCI_SYNC_SCO_DATA_PACKET        0x03
#define FBT_HCI_SYNC_HCI_EVENT_PACKET       0x04

// Packet types for use in CreateConnection
#define FBT_HCI_PACKET_TYPE_DM1				0x0008	// 1 time slot, 1-18 bytes of data, FEC encoded
#define FBT_HCI_PACKET_TYPE_DH1				0x0010	// 1 time slot, 1-28 bytes of data, not FEC encoded

#define FBT_HCI_PACKET_TYPE_DM3				0x0400	// 3 time slots, 2-123 bytes of data, FEC encoded
#define FBT_HCI_PACKET_TYPE_DH3				0x0800	// 3 time slots, 2-185 bytes of data, not FEC encoded

#define FBT_HCI_PACKET_TYPE_DM5				0x4000	// 5 time slots, 2-226 bytes of data, FEC encoded
#define FBT_HCI_PACKET_TYPE_DH5				0x8000	// 3 time slots, 2-341 bytes of data, not FEC encoded

#define FBT_HCI_PACKET_TYPE_ALL				(FBT_HCI_PACKET_TYPE_DM1|FBT_HCI_PACKET_TYPE_DH1|FBT_HCI_PACKET_TYPE_DM3|FBT_HCI_PACKET_TYPE_DH3|FBT_HCI_PACKET_TYPE_DM5|FBT_HCI_PACKET_TYPE_DH5)

// LAP codes for use in Inquiry
#define FBT_HCI_LAP_GIAC					0x9E8B33
#define FBT_HCI_LAP_LIAC					0x9E8B00

// Link Types
#define FBT_HCI_LINK_TYPE_SCO          		0x00
#define FBT_HCI_LINK_TYPE_ACL          		0x01

// Roles
#define FBT_HCI_ROLE_MASTER					0x00
#define FBT_HCI_ROLE_SLAVE					0x01

// Event Filters
#define FBT_HCI_FILTER_NONE					0x00
#define FBT_HCI_FILTER_INQUIRY_RESULT		0x01
#define FBT_HCI_FILTER_CONNECTION_SETUP		0x02

#define FBT_HCI_FILTER_ALL					0x00
#define FBT_HCI_FILTER_CLASS_OF_DEVICE		0x01
#define FBT_HCI_FILTER_BDADDR				0x02

// Data packet parameters
#define FBT_HCI_PACKET_BOUNDARY_FRAGMENT	0x01
#define FBT_HCI_PACKET_BOUNDARY_START		0x02

#define FBT_HCI_BROADCAST_POINT_TO_POINT	0x00
#define FBT_HCI_BROADCAST_ACTIVE_SLAVE		0x01
#define FBT_HCI_BROADCAST_PARKED_SLAVE		0x02

#endif // _FBT_HCI_PARAMETERS_H
