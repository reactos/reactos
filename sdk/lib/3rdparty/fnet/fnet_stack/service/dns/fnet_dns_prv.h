/**************************************************************************
*
* Copyright 2014-2020 by Andrej Butok. FNET Community.
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
*  DNS Definitions.
*
***************************************************************************/

#ifndef _FNET_DNS_PRV_H_

#define _FNET_DNS_PRV_H_

/************************************************************************
*     Definitions
*************************************************************************/
/* Size limits. */
#define FNET_DNS_NAME_SIZE      (255U)     /* RFC1035:To simplify implementations, the total length of a domain name (i.e.,
                                           * label octets and label length octets) is restricted to 255 octets or less.*/
#define FNET_DNS_MESSAGE_SIZE   (512U)     /* Messages carried by UDP are restricted to 512 bytes (not counting the IP
                                           * or UDP headers).  
                                           * Longer messages (not supported) are truncated and the TC bit is set in
                                           * the header.*/

/********************************************************************/ /*!
* @brief DNS Resorce Record Types
*************************************************************************/
#define    FNET_DNS_TYPE_A      (0x0001U)    /**< @brief IPv4 address. */
#define    FNET_DNS_TYPE_AAAA   (0x001CU)    /**< @brief IPv6 address. */

/************************************************************************
*    DNS header [RFC1035, 4.1.1.]
*************************************************************************
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      ID                       |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    QDCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ANCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    NSCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ARCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
*/
FNET_COMP_PACKED_BEGIN
typedef struct
{
    fnet_uint16_t id FNET_COMP_PACKED;         /* An identifier assigned by the program that
                                                * generates any kind of query. This identifier is copied
                                                * the corresponding reply and can be used by the requester
                                                * to match up replies to outstanding queries. */
    fnet_uint16_t flags FNET_COMP_PACKED;      /* Flags.*/
    fnet_uint16_t qdcount FNET_COMP_PACKED;    /* Count of questions, specifying the number of entries in the question section.*/
    fnet_uint16_t ancount FNET_COMP_PACKED;    /* Count of answers, specifying the number of resource records in the answer section.*/
    fnet_uint16_t nscount FNET_COMP_PACKED;    /* Authority Record Count, specifying the number of name
                                                * server resource records in the authority records section. (“NS” - “name server”).*/
    fnet_uint16_t arcount FNET_COMP_PACKED;    /* Count of additional, specifying the number of
                                                * resource records in the additional records section.*/

} fnet_dns_header_t;
FNET_COMP_PACKED_END

#define FNET_DNS_HEADER_FLAGS_QR    (0x8000U) /* Query (0), Response (1)*/
#define FNET_DNS_HEADER_FLAGS_AA    (0x0400U) /* Authoritative Answer. */
#define FNET_DNS_HEADER_FLAGS_TC    (0x0200U) /* TrunCation. */
#define FNET_DNS_HEADER_FLAGS_RD    (0x0100U) /* Recursion Desired. */
#define FNET_DNS_HEADER_FLAGS_RA    (0x0080U) /* Recursion Available. */

/************************************************************************
*    DNS Question section [RFC1035, 4.1.2.]
*************************************************************************
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    /                     QNAME                     /
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     QTYPE                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     QCLASS                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
*/
FNET_COMP_PACKED_BEGIN
typedef struct
{
    fnet_uint8_t   zero_length FNET_COMP_PACKED;   /* The domain name terminates with the
                                                    * zero length octet for the null label of the root. */
    fnet_uint16_t  qtype FNET_COMP_PACKED;         /* Specifies the type of the query.*/
    fnet_uint16_t  qclass FNET_COMP_PACKED;        /* Specifies the class of the query.*/

} fnet_dns_q_tail_t;
FNET_COMP_PACKED_END

/************************************************************************
*    DNS Resource Record header [RFC1035, 4.1.3.] with message compression
*************************************************************************
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    /                                               /
    /                      NAME                     /
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      TYPE                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     CLASS                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      TTL                      |
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                   RDLENGTH                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
    /                     RDATA                     /
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
*/
FNET_COMP_PACKED_BEGIN
typedef struct
{
    union
    {
        fnet_uint8_t   name_ptr_c[2] FNET_COMP_PACKED; /* A domain name to which this resource record pertains.
                                                        * For compression, it is replaced with a pointer to a prior occurance
                                                        * of the same name */
        fnet_uint16_t  name_ptr FNET_COMP_PACKED;
    } name;
    fnet_uint16_t   type FNET_COMP_PACKED;          /* This field specifies the meaning of the data in the RDATA
                                                    * field.*/
    fnet_uint16_t   rr_class FNET_COMP_PACKED;     /* An unsigned 16 bit integer specifying the number of
                                                    * entries in the question section.*/
    fnet_uint32_t   ttl FNET_COMP_PACKED;           /* Specifies the time interval (in seconds) that the
                                                    * resource record may be
                                                    * cached before it should be discarded.*/
    fnet_uint16_t   rdlength FNET_COMP_PACKED;      /* Length in octets of the RDATA field.*/
    fnet_uint32_t   rdata FNET_COMP_PACKED;         /* The format of this information varies
                                                    * according to the TYPE and CLASS of the resource record.
                                                    * If the TYPE is A and the CLASS is IN,
                                                    * the RDATA field is a 4 octet ARPA Internet address.*/

} fnet_dns_rr_c_header_t;
FNET_COMP_PACKED_END

/* DNS record header */
FNET_COMP_PACKED_BEGIN
typedef struct fnet_dns_rr_header_s
{
    fnet_uint16_t    type    FNET_COMP_PACKED;          /* RR type */
    fnet_uint16_t    rr_class    FNET_COMP_PACKED;      /* RR class */
    fnet_uint32_t    ttl FNET_COMP_PACKED;              /* time to live (in seconds) */
    fnet_uint16_t    rdlength FNET_COMP_PACKED;      /* length of data */
} fnet_dns_rr_header_t;
FNET_COMP_PACKED_END

/* [RFC1035 4.1.4.] In order to reduce the size of messages, the domain system utilizes a
* compression scheme which eliminates the repetition of domain names in a
* message. In this scheme, an entire domain name or a list of labels at
* the end of a domain name is replaced with a pointer to a prior occurance
* of the same name.
* The pointer takes the form of a two octet sequence:
* +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
* | 1  1|                OFFSET                   |
* +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
*/
#define     FNET_DNS_NAME_COMPRESSED_MASK           (0xC0U)       /**< @brief Mark that Domain Name is compressed. */
#define     FNET_DNS_NAME_COMPRESSED_INDEX_MASK     (0x3FFFU)     /**< @brief Index of compressed name. */


#define FNET_DNS_HEADER_CLASS_IN    (0x01U)  /* The Internet.*/


/************************************************************************
* Extract RR name to rr_name. Returns updated next pointer; 0 if error.
* If rr_name or rr_name_size are 0, just skip name.
************************************************************************/
const fnet_uint8_t *_fnet_dns_get_rr_name(char *rr_name, fnet_uint32_t rr_name_size, const fnet_uint8_t *rr, const fnet_uint8_t *packet, fnet_size_t packet_size);

/************************************************************************
* Print DNS resource record name.
************************************************************************/
void _fnet_dns_print_rr_name(const fnet_char_t *prefix, const fnet_char_t *rr_name);

#endif /* _FNET_DNS_PRV_H_*/
