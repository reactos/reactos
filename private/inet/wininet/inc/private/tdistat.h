/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-1993          **/
/********************************************************************/
/* :ts=4 */

//** TDISTAT.H - TDI status code definitions.
//
// This file contains the TDI status code definitions.

#ifndef NT

#define TDI_SUCCESS             0           // Success
#define TDI_NO_RESOURCES        1           // No resources.
#define TDI_ADDR_IN_USE         2           // Address already in use.
#define TDI_BAD_ADDR            3           // Address given is bad.
#define TDI_NO_FREE_ADDR        4           // No addresses available.
#define TDI_ADDR_INVALID        6           // Address object is invalid.
#define TDI_ADDR_DELETED        7           // Address object was deleted.
#define TDI_BUFFER_OVERFLOW     9           // Buffer overflowed.
#define TDI_BAD_EVENT_TYPE      10          // Bad event type.
#define TDI_BAD_OPTION          11          // Bad option or length.
#define TDI_CONN_REFUSED        14          // Connection was refused.
#define TDI_INVALID_CONNECTION  15          // Invalid connection.
#define TDI_ALREADY_ASSOCIATED  16          // Connection already associated.
#define TDI_NOT_ASSOCIATED      17          // Connection not associated.
#define TDI_CONNECTION_ACTIVE   18          // Connection is still active.
#define TDI_CONNECTION_ABORTED  19          // Connection was aborted.
#define TDI_CONNECTION_RESET    20          // Connection was reset.
#define TDI_TIMED_OUT           21          // Connection timed out.
#define TDI_GRACEFUL_DISC       22          // Received a graceful disconnect.
#define TDI_NOT_ACCEPTED        23          // Data not accepted.
#define TDI_MORE_PROCESSING     24          // More processing required.
#define TDI_INVALID_STATE       25          // TCB in an invalid state.
#define TDI_INVALID_PARAMETER   26          // An invalid parameter.
#define TDI_DEST_NET_UNREACH    27          // Destination net is unreachable.
#define TDI_DEST_HOST_UNREACH   28          // Dest. host is unreachable.
#define TDI_DEST_UNREACHABLE    TDI_DEST_HOST_UNREACH
#define TDI_DEST_PROT_UNREACH   29          // Destination protocol is
                                            // unreachable.
#define TDI_DEST_PORT_UNREACH   30          // Dest. port is unreachable.
#define TDI_INVALID_QUERY       31          // Invalid query type specified.
#define TDI_REQ_ABORTED         32          // Request was aborted for some
                                            // reason.
#define TDI_BUFFER_TOO_SMALL    33          // Buffer was too small.
#define TDI_CANCELLED           34          // The request was cancelled.
#define	TDI_BUFFER_TOO_BIG		35			// Send buffer was too big.
#define TDI_ITEM_NOT_FOUND      36          // Item not found.
#define TDI_INVALID_REQUEST     0xfe        // Invalid request.
#define TDI_PENDING             0xff        // Pending

#else

//
// Map to NT STATUS codes.
//
#define TDI_SUCCESS             STATUS_SUCCESS
#define TDI_NO_RESOURCES        STATUS_INSUFFICIENT_RESOURCES
#define TDI_ADDR_IN_USE         STATUS_ADDRESS_ALREADY_EXISTS
#define TDI_BAD_ADDR            STATUS_INVALID_ADDRESS_COMPONENT
#define TDI_NO_FREE_ADDR        STATUS_TOO_MANY_ADDRESSES
#define TDI_ADDR_INVALID        STATUS_INVALID_ADDRESS
#define TDI_ADDR_DELETED        STATUS_ADDRESS_CLOSED
#define TDI_BUFFER_OVERFLOW     STATUS_BUFFER_OVERFLOW
#define TDI_BAD_EVENT_TYPE      STATUS_INVALID_PARAMETER
#define TDI_BAD_OPTION          STATUS_INVALID_PARAMETER
#define TDI_CONN_REFUSED        STATUS_CONNECTION_REFUSED
#define TDI_INVALID_CONNECTION  STATUS_CONNECTION_INVALID
#define TDI_ALREADY_ASSOCIATED  STATUS_ADDRESS_ALREADY_ASSOCIATED
#define TDI_NOT_ASSOCIATED      STATUS_ADDRESS_NOT_ASSOCIATED
#define TDI_CONNECTION_ACTIVE   STATUS_CONNECTION_ACTIVE
#define TDI_CONNECTION_ABORTED  STATUS_CONNECTION_ABORTED
#define TDI_CONNECTION_RESET    STATUS_CONNECTION_RESET
#define TDI_TIMED_OUT           STATUS_IO_TIMEOUT
#define TDI_GRACEFUL_DISC       STATUS_GRACEFUL_DISCONNECT
#define TDI_NOT_ACCEPTED        STATUS_DATA_NOT_ACCEPTED
#define TDI_MORE_PROCESSING     STATUS_MORE_PROCESSING_REQUIRED
#define TDI_INVALID_STATE       STATUS_INVALID_DEVICE_STATE
#define TDI_INVALID_PARAMETER   STATUS_INVALID_PARAMETER
#define TDI_DEST_NET_UNREACH    STATUS_NETWORK_UNREACHABLE
#define TDI_DEST_HOST_UNREACH   STATUS_HOST_UNREACHABLE
#define TDI_DEST_UNREACHABLE    TDI_DEST_HOST_UNREACH
#define TDI_DEST_PROT_UNREACH   STATUS_PROTOCOL_UNREACHABLE
#define TDI_DEST_PORT_UNREACH   STATUS_PORT_UNREACHABLE
#define TDI_INVALID_QUERY       STATUS_INVALID_DEVICE_REQUEST
#define TDI_REQ_ABORTED         STATUS_REQUEST_ABORTED
#define TDI_BUFFER_TOO_SMALL    STATUS_BUFFER_TOO_SMALL
#define TDI_CANCELLED           STATUS_CANCELLED
#define	TDI_BUFFER_TOO_BIG		STATUS_INVALID_BUFFER_SIZE
#define TDI_INVALID_REQUEST     STATUS_INVALID_DEVICE_REQUEST
#define TDI_PENDING             STATUS_PENDING
#define TDI_ITEM_NOT_FOUND      STATUS_OBJECT_NAME_NOT_FOUND


#endif  // NT

#define	TDI_OPTION_EOL				0

#define	TDI_ADDRESS_OPTION_REUSE	1
#define	TDI_ADDRESS_OPTION_DHCP		2

