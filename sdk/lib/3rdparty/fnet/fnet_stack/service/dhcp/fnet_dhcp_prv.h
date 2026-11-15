/**************************************************************************
*
* Copyright 2017 by Andrey Butok. FNET Community.
*
***************************************************************************
*
*  Licensed under the Apache License, Version 2.0 (the "License"); you may
*  not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
*  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
***************************************************************************
*
*  DHCP private definitions.
*
***************************************************************************/

#ifndef _FNET_DHCP_PRV_H_

#define _FNET_DHCP_PRV_H_

#if FNET_CFG_DHCP_CLN || FNET_CFG_DHCP_SRV

/************************************************************************
*     Definitions
*************************************************************************/
#define FNET_DHCP_OP_BOOTREQUEST            (1U)        /* RFC: The 'op' field of each DHCP message sent from a client to a server contains BOOTREQUEST.*/
#define FNET_DHCP_OP_BOOTREPLY              (2U)        /* RFC: BOOTREPLY is used in the 'op' field of each DHCP message sent from a server to a client.*/
#define FNET_DHCP_FLAGS_BROADCAST           (0x8000U)   /* RFC2131:  A server or relay agent sending or relaying a DHCP message directly
                                                        to a DHCP client (i.e., not to a relay agent specified in the
                                                        'giaddr' field) SHOULD examine the BROADCAST bit in the 'flags'
                                                        field.  If this bit is set to 1, the DHCP message SHOULD be sent as
                                                        an IP broadcast using an IP broadcast address (preferably 0xffffffff)
                                                        as the IP destination address and the link-layer broadcast address as
                                                        the link-layer destination address.  If the BROADCAST bit is cleared
                                                        to 0, the message SHOULD be sent as an IP unicast to the IP address
                                                        specified in the 'yiaddr' field and the link-layer address specified
                                                        in the 'chaddr' field.  If unicasting is not possible, the message
                                                        MAY be sent as an IP broadcast using an IP broadcast address
                                                        (preferably 0xffffffff) as the IP destination address and the link-
                                                        layer broadcast address as the link-layer destination address.*/

#define FNET_DHCP_HTYPE_ETHERNET            (1U)       /* Hardware address type, see ARP section in "Assigned Numbers" RFC; e.g., ’1’ = 10mb ethernet..*/


/************************************************************************
*     DHCP Options. [RFC 2132] definitions
*************************************************************************/
#define FNET_DHCP_OPTION_PAD                    (0U)     /* Pad option.*/
#define FNET_DHCP_OPTION_SUBNETMASK             (1U)     /* Subnet mask.*/
#define FNET_DHCP_OPTION_SUBNETMASK_LENGTH      (4U)

#define FNET_DHCP_OPTION_ROUTER                 (3U)     /* Router option (gateway).*/
#define FNET_DHCP_OPTION_ROUTER_LENGTH_MIN      (4U)
#define FNET_DHCP_OPTION_DNS                    (6U)     /* Domain Name Server option. */
#define FNET_DHCP_OPTION_DNS_LENGTH_MIN         (4U)     /* The minimum length for this option is 4 octets, 
                                                         * and the length MUST always be a multiple of 4. */
#define FNET_DHCP_OPTION_HOSTNAME               (12U)    /* Hostname */
#define FNET_DHCP_OPTION_HOSTNAME_LENGTH        (32U)
#define FNET_DHCP_OPTION_FILELENGTH             (13U)    /* File Length */
#define FNET_DHCP_OPTION_FILELENGTH_LENGTH      (2U)
#define FNET_DHCP_OPTION_ROOTPATH               (17U)    /* Rootpath.*/
#define FNET_DHCP_OPTION_ROOTPATH_LENGTH        (32U)
#define FNET_DHCP_OPTION_BROADCAST              (28U)    /* Broadcast Address option.  */
#define FNET_DHCP_OPTION_BROADCAST_LENGTH       (4U)
#define FNET_DHCP_OPTION_REQ_ADDRESS            (50U)    /* Requested IP address.*/
#define FNET_DHCP_OPTION_REQ_ADDRESS_LENGTH     (4U)
#define FNET_DHCP_OPTION_LEASE                  (51U)    /* IP Address lease time (seconds).*/
#define FNET_DHCP_OPTION_LEASE_LENGTH           (4U)
#define FNET_DHCP_OPTION_OVERLOAD               (52U)    /* Option overload.*/
#define FNET_DHCP_OPTION_OVERLOAD_LENGTH        (1U)
#define FNET_DHCP_OPTION_OVERLOAD_NONE          (0U)
#define FNET_DHCP_OPTION_OVERLOAD_FILE          (1U)     /* The 'file' field is used to hold options.*/
#define FNET_DHCP_OPTION_OVERLOAD_SNAME         (2U)     /* The 'sname' field is used to hold options.*/
#define FNET_DHCP_OPTION_OVERLOAD_BOTH          (3U)     /* Both fields are used to hold options.*/
#define FNET_DHCP_OPTION_MSG_TYPE               (53U)    /* DHCP Message Type.*/
#define FNET_DHCP_OPTION_MSG_TYPE_LENGTH        (1U)
#define FNET_DHCP_OPTION_MSG_TYPE_DISCOVER      (1U)
#define FNET_DHCP_OPTION_MSG_TYPE_OFFER         (2U)
#define FNET_DHCP_OPTION_MSG_TYPE_REQUEST       (3U)
#define FNET_DHCP_OPTION_MSG_TYPE_DECLINE       (4U)
#define FNET_DHCP_OPTION_MSG_TYPE_ACK           (5U)
#define FNET_DHCP_OPTION_MSG_TYPE_NAK           (6U)
#define FNET_DHCP_OPTION_MSG_TYPE_RELEASE       (7U)
#define FNET_DHCP_OPTION_MSG_TYPE_INFORM        (8U)
#define FNET_DHCP_OPTION_SERVER_ID              (54U)    /* Server Identifier (ip address).*/
#define FNET_DHCP_OPTION_SERVER_ID_LENGTH       (4U)
#define FNET_DHCP_OPTION_PARAMETER_REQ_LIST     (55U)    /* Parameter Request List. */
#define FNET_DHCP_OPTION_MESSAGE_SIZE           (57U)    /* Maximum DHCP Message Size. The minimum legal value is 576.*/
#define FNET_DHCP_OPTION_MESSAGE_SIZE_LENGTH    (2U)
#define FNET_DHCP_OPTION_T1                     (58U)    /* Renewal (T1) Time Value. It specifies the time interval from address assignment until the client transitions to the RENEWING state.*/
#define FNET_DHCP_OPTION_T1_LENGTH              (4U)
#define FNET_DHCP_OPTION_T2                     (59U)    /* Rebinding (T2) Time Value. It specifies the time interval from address assignment until the client transitions to the REBINDING state.*/
#define FNET_DHCP_OPTION_T2_LENGTH              (4U)
#define FNET_DHCP_OPTION_CLIENT_ID              (61U)    /* Client-identifier.*/
#define FNET_DHCP_OPTION_CLIENT_ID_LENGTH       (sizeof(fnet_mac_addr_t)+1U)
#define FNET_DHCP_OPTION_END                    (255U)   /* End option. */

#define FNET_DHCP_OPTIONS_LENGTH    (312U)     /* [RFC2131, 2] A DHCP client must be prepared to receive DHCP messages 
                                                *   with an 'options' field of at least length 312 octets.*/

/************************************************************************
*    DHCP header [RFC2131, 2]
*************************************************************************
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     op (1)    |   htype (1)   |   hlen (1)    |   hops (1)    |
   +---------------+---------------+---------------+---------------+
   |                            xid (4) - Transaction ID                           |
   +-------------------------------+-------------------------------+
   |           secs (2)            |           flags (2)           |
   +-------------------------------+-------------------------------+
   |                          ciaddr  (4) - client IP addr         |
   +---------------------------------------------------------------+
   |                          yiaddr  (4) - your (client) IP addr  |
   +---------------------------------------------------------------+
   |                          siaddr  (4) - server IP addr         |
   +---------------------------------------------------------------+
   |                          giaddr  (4) - relay aget IP addr     |
   +---------------------------------------------------------------+
   |                                                               |
   |                          chaddr  (16) - client HW adress.     |
   |                                                               |
   |                                                               |
   +---------------------------------------------------------------+
   |                                                               |
   |                          sname   (64) - server host name      |
   +---------------------------------------------------------------+
   |                                                               |
   |                          file    (128) - boot file name       |
   +---------------------------------------------------------------+
   |                          DHCP magic cookie    (4)             |
   +---------------------------------------------------------------+
   |                                                               |
   |                          options (variable)                   |
   +---------------------------------------------------------------+
   | opt end = 0xFF|
   +---------------+*/

FNET_COMP_PACKED_BEGIN
typedef struct
{
    fnet_uint8_t    op FNET_COMP_PACKED;        /* Message op code / message type:
                                                 *   1 = BOOTREQUEST, 2 = BOOTREPLY */
    fnet_uint8_t    htype FNET_COMP_PACKED;     /* Hardware address type, see ARP section in "Assigned
                                                 *   Numbers" RFC; e.g., '1' = 10mb ethernet.*/
    fnet_uint8_t    hlen FNET_COMP_PACKED;      /* Hardware address length (e.g.  '6' for 10mb ethernet).*/
    fnet_uint8_t    hops FNET_COMP_PACKED;      /* Client sets to zero, optionally used by relay agents
                                                 *   when booting via a relay agent.*/
    fnet_uint32_t   xid FNET_COMP_PACKED;       /* Transaction ID, a random number chosen by the
                                                 *   client, used by the client and server to associate
                                                 *   messages and responses between a client and a server.*/
    fnet_uint16_t   secs FNET_COMP_PACKED;      /* Filled in by client, seconds elapsed since client
                                                 *   began address acquisition or renewal process.*/
    fnet_uint16_t   flags FNET_COMP_PACKED;     /* Flags.*/
    fnet_ip4_addr_t ciaddr FNET_COMP_PACKED;    /* Client IP address; only filled in if client is in BOUND,
                                                 *   RENEW or REBINDING state and can respond to ARP requests.*/
    fnet_ip4_addr_t yiaddr FNET_COMP_PACKED;    /* Your (client) IP address.*/
    fnet_ip4_addr_t siaddr FNET_COMP_PACKED;    /* IP address of next server to use in bootstrap;
                                                 *   returned in DHCPOFFER, DHCPACK by server.*/
    fnet_ip4_addr_t giaddr FNET_COMP_PACKED;    /* Relay agent IP address, used in booting via a relay agent.*/
    fnet_uint8_t    chaddr[16] FNET_COMP_PACKED;/* Client hardware address.*/
    fnet_uint8_t    sname[64] FNET_COMP_PACKED; /* Optional server host name, null terminated string.*/
    fnet_uint8_t    file[128] FNET_COMP_PACKED; /* Boot file name, null terminated string; "generic"
                                                 *   name or null in DHCPDISCOVER, fully qualified
                                                 *   directory-path name in DHCPOFFER.*/
    fnet_uint8_t    magic_cookie[4] FNET_COMP_PACKED;   /* DHCP magic cookie */
    fnet_uint8_t    options[FNET_DHCP_OPTIONS_LENGTH] FNET_COMP_PACKED; /* Optional parameters field. See [RFC2132].*/
} fnet_dhcp_header_t;
FNET_COMP_PACKED_END

#if defined(__cplusplus)
extern "C" {
#endif

/* DHCP magic cookie */
extern const fnet_uint8_t fnet_dhcp_magic_cookie[4];

fnet_uint8_t *_fnet_dhcp_add_option(fnet_uint8_t *option_buffer, fnet_size_t option_buffer_size, fnet_uint8_t option_code, fnet_uint8_t option_length,  const void *option_value);

#if FNET_CFG_DEBUG_TRACE_DHCP_SRV && FNET_CFG_DEBUG_TRACE
void _fnet_dhcp_trace(const fnet_char_t *str, fnet_dhcp_header_t *header);
#else
#define _fnet_dhcp_trace(str, header)    do{}while(0)
#endif

#if defined(__cplusplus)
}
#endif

#endif /* FNET_CFG_DHCP_CLN || FNET_CFG_DHCP_SRV */

#endif /* _FNET_DHCP_PRV_H_ */
