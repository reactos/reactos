#ifndef _FBT_HCI_OPCODES_H
#define _FBT_HCI_OPCODES_H

// Opcode Group Field (OGF) codes
#define FBT_HCI_OGF_LINK_CONTROL                      			0x01	// Link control group
#define FBT_HCI_OGF_LINK_POLICY                       			0x02	// Link polic group
#define FBT_HCI_OGF_CONTROL_BASEBAND                  			0x03	// Host Controller & Baseband group
#define FBT_HCI_OGF_INFORMATIONAL_PARAMETERS          			0x04	// Information parameters group
#define FBT_HCI_OGF_STATUS_PARAMETERS                 			0x05	// Status parameters group
#define FBT_HCI_OGF_TESTING                           			0x06	// Test group

// Opcode Command Field (OCF) codes
// Link control commands
#define FBT_HCI_OCF_INQUIRY                                   	0x0001
#define FBT_HCI_OCF_INQUIRY_CANCEL                            	0x0002
#define FBT_HCI_OCF_PERIODIC_INQUIRY_MODE                     	0x0003
#define FBT_HCI_OCF_EXIT_PERIODIC_INQUIRY_MODE                	0x0004
#define FBT_HCI_OCF_CREATE_CONNECTION                         	0x0005
#define FBT_HCI_OCF_DISCONNECT                                	0x0006
#define FBT_HCI_OCF_ADD_SCO_CONNECTION                        	0x0007

#define FBT_HCI_OCF_ACCEPT_CONNECTION_REQUEST                 	0x0009
#define FBT_HCI_OCF_REJECT_CONNECTION_REQUEST                 	0x000A
#define FBT_HCI_OCF_LINK_KEY_REQUEST_REPLY                    	0x000B
#define FBT_HCI_OCF_LINK_KEY_REQUEST_NEGATIVE_REPLY           	0x000C
#define FBT_HCI_OCF_PIN_CODE_REQUEST_REPLY                    	0x000D
#define FBT_HCI_OCF_PIN_CODE_REQUEST_NEGATIVE_REPLY           	0x000E
#define FBT_HCI_OCF_CHANGE_CONNECTION_PACKET_TYPE             	0x000F

#define FBT_HCI_OCF_AUTHENTICATION_REQUESTED                  	0x0011
#define FBT_HCI_OCF_SET_CONNECTION_ENCRYPTION                 	0x0013
#define FBT_HCI_OCF_CHANGE_CONNECTION_LINK_KEY                	0x0015
#define FBT_HCI_OCF_MASTER_LINK_KEY                           	0x0017
#define FBT_HCI_OCF_REMOTE_NAME_REQUEST                       	0x0019
#define FBT_HCI_OCF_READ_REMOTE_SUPPORTED_FEATURES            	0x001B
#define FBT_HCI_OCF_READ_REMOTE_VERSION_INFORMATION           	0x001D
#define FBT_HCI_OCF_READ_CLOCK_OFFSET                         	0x001F

// Link policy commands
#define FBT_HCI_OCF_HOLD_MODE                                 	0x0001
#define FBT_HCI_OCF_SNIFF_MODE                                	0x0003
#define FBT_HCI_OCF_EXIT_SNIFF_MODE                           	0x0004
#define FBT_HCI_OCF_PARK_MODE                                 	0x0005
#define FBT_HCI_OCF_EXIT_PARK_MODE                            	0x0006
#define FBT_HCI_OCF_QOS_SETUP                                 	0x0007
#define FBT_HCI_OCF_ROLE_DISCOVERY                            	0x0009
#define FBT_HCI_OCF_SWITCH_ROLE                               	0x000B
#define FBT_HCI_OCF_READ_LINK_POLICY_SETTINGS                 	0x000C
#define FBT_HCI_OCF_WRITE_LINK_POLICY_SETTINGS                	0x000D

// Host controller & baseband commands
#define FBT_HCI_OCF_SET_EVENT_MASK                            	0x0001
#define FBT_HCI_OCF_RESET                                     	0x0003
#define FBT_HCI_OCF_SET_EVENT_FILTER                          	0x0005
#define FBT_HCI_OCF_FLUSH                                     	0x0008
#define FBT_HCI_OCF_READ_PIN_TYPE                             	0x0009
#define FBT_HCI_OCF_WRITE_PIN_TYPE                            	0x000A
#define FBT_HCI_OCF_CREATE_NEW_UNIT_KEY                       	0x000B
#define FBT_HCI_OCF_READ_STORED_LINK_KEY                      	0x000D
#define FBT_HCI_OCF_WRITE_STORED_LINK_KEY                     	0x0011
#define FBT_HCI_OCF_DELETE_STORED_LINK_KEY                    	0x0012
#define FBT_HCI_OCF_CHANGE_LOCAL_NAME                         	0x0013
#define FBT_HCI_OCF_READ_LOCAL_NAME                           	0x0014
#define FBT_HCI_OCF_READ_CONNECTION_ACCEPT_TIMEOUT            	0x0015
#define FBT_HCI_OCF_WRITE_CONNECTION_ACCEPT_TIMEOUT			  	0x0016
#define FBT_HCI_OCF_READ_PAGE_TIMEOUT                         	0x0017
#define FBT_HCI_OCF_WRITE_PAGE_TIMEOUT                        	0x0018
#define FBT_HCI_OCF_READ_SCAN_ENABLE                          	0x0019
#define FBT_HCI_OCF_WRITE_SCAN_ENABLE                         	0x001A
#define FBT_HCI_OCF_READ_PAGE_SCAN_ACTIVITY                   	0x001B
#define FBT_HCI_OCF_WRITE_PAGE_SCAN_ACTIVITY                  	0x001C
#define FBT_HCI_OCF_READ_INQUIRY_SCAN_ACTIVITY                	0x001D
#define FBT_HCI_OCF_WRITE_INQUIRY_SCAN_ACTIVITY               	0x001E
#define FBT_HCI_OCF_READ_AUTHENTICATION_ENABLE                	0x001F
#define FBT_HCI_OCF_WRITE_AUTHENTICATION_ENABLE               	0x0020
#define FBT_HCI_OCF_READ_ENCRYPTION_MODE                      	0x0021
#define FBT_HCI_OCF_WRITE_ENCRYPTION_MODE                     	0x0022
#define FBT_HCI_OCF_READ_CLASS_OF_DEVICE                      	0x0023
#define FBT_HCI_OCF_WRITE_CLASS_OF_DEVICE                     	0x0024
#define FBT_HCI_OCF_READ_VOICE_SETTING                        	0x0025
#define FBT_HCI_OCF_WRITE_VOICE_SETTING                       	0x0026
#define FBT_HCI_OCF_READ_AUTOMATIC_FLUSH_TIMEOUT              	0x0027
#define FBT_HCI_OCF_WRITE_AUTOMATIC_FLUSH_TIMEOUT             	0x0028
#define FBT_HCI_OCF_READ_NUM_BROADCAST_RETRANSMISSIONS        	0x0029
#define FBT_HCI_OCF_WRITE_NUM_BROADCAST_RETRANSMISSIONS       	0x002A
#define FBT_HCI_OCF_READ_HOLD_MODE_ACTIVITY                   	0x002B
#define FBT_HCI_OCF_WRITE_HOLD_MODE_ACTIVITY                  	0x002C
#define FBT_HCI_OCF_READ_TRANSMIT_POWER_LEVEL                 	0x002D
#define FBT_HCI_OCF_READ_SCO_FLOW_CONTROL_ENABLE              	0x002E
#define FBT_HCI_OCF_WRITE_SCO_FLOW_CONTROL_ENABLE             	0x002F
#define FBT_HCI_OCF_SET_HOST_CONTROLLER_TO_HOST_FLOW_CONTROL  	0x0031
#define FBT_HCI_OCF_HOST_BUFFER_SIZE                          	0x0033
#define FBT_HCI_OCF_HOST_NUMBER_OF_COMPLETED_PACKETS          	0x0035
#define FBT_HCI_OCF_READ_LINK_SUPERVISION_TIMEOUT             	0x0036
#define FBT_HCI_OCF_WRITE_LINK_SUPERVISION_TIMEOUT            	0x0037
#define FBT_HCI_OCF_READ_NUMBER_OF_SUPPORTED_IAC              	0x0038
#define FBT_HCI_OCF_READ_CURRENT_IAC_LAP                      	0x0039
#define FBT_HCI_OCF_WRITE_CURRENT_IAC_LAP                     	0x003A
#define FBT_HCI_OCF_READ_PAGE_SCAN_PERIOD_MODE                	0x003B
#define FBT_HCI_OCF_WRITE_PAGE_SCAN_PERIOD_MODE               	0x003C
#define FBT_HCI_OCF_READ_PAGE_SCAN_MODE                       	0x003D
#define FBT_HCI_OCF_WRITE_PAGE_SCAN_MODE                      	0x003E

// Informational parameter commands
#define FBT_HCI_OCF_READ_LOCAL_VERSION_INFORMATION            	0x0001
#define FBT_HCI_OCF_LOCAL_SUPPPROTED_FEATURES                 	0x0003
#define FBT_HCI_OCF_READ_BUFFER_SIZE                          	0x0005
#define FBT_HCI_OCF_READ_COUNTRY_CODE                         	0x0007
#define FBT_HCI_OCF_READ_BD_ADDR                              	0x0009

// Status parameters commands
#define FBT_HCI_OCF_READ_FAILED_CONTACT_COUNTER               	0x0001
#define FBT_HCI_OCF_RESET_FAILED_CONTACT_COUNTER              	0x0002
#define FBT_HCI_OCF_GET_LINK_QUALITY                          	0x0003
#define FBT_HCI_OCF_READ_RSSI                                 	0x0005

// Test commands
#define FBT_HCI_OCF_READ_LOOPBACK_MODE                        	0x0001
#define FBT_HCI_OCF_WRITE_LOOPBACK_MODE                       	0x0002
#define FBT_HCI_OCF_ENABLE_DEVICE_UNDER_TEST_MODE             	0x0003

#define FBT_HCI_OGF_FROM_COMMAND(cmd)							(cmd>>10)
#define FBT_HCI_OCF_FROM_COMMAND(cmd)							(cmd&0x3FF)

#endif // _FBT_HCI_OPCODES_H
