  /**
 * @file
 * NetBIOS name service responder
 */

/**
 * @defgroup netbiosns NETBIOS responder
 * @ingroup apps
 *
 * This is an example implementation of a NetBIOS name server.
 * It responds to name queries for a configurable name.
 * Name resolving is not supported.
 *
 * Note that the device doesn't broadcast it's own name so can't
 * detect duplicate names!
 */

/*
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Modifications by Ray Abram to respond to NetBIOS name requests when Incoming name = *
 * - based on code from "https://github.com/esp8266/Arduino/commit/1f7989b31d26d7df9776a08f36d685eae7ac8f99"
 * - with permission to relicense to BSD from original author:
 *   http://www.xpablo.cz/?p=751#more-751
 */

#include "lwip/apps/netbiosns.h"

#if LWIP_IPV4 && LWIP_UDP  /* don't build if not configured for use in lwipopts.h */

#include "lwip/def.h"
#include "lwip/udp.h"
#include "lwip/ip.h"
#include "lwip/netif.h"
#include "lwip/prot/iana.h"

#include <string.h>

/** size of a NetBIOS name */
#define NETBIOS_NAME_LEN 16

/** The Time-To-Live for NetBIOS name responds (in seconds)
 * Default is 300000 seconds (3 days, 11 hours, 20 minutes) */
#define NETBIOS_NAME_TTL 300000u

/** NetBIOS header flags */
#define NETB_HFLAG_RESPONSE           0x8000U
#define NETB_HFLAG_OPCODE             0x7800U
#define NETB_HFLAG_OPCODE_NAME_QUERY  0x0000U
#define NETB_HFLAG_AUTHORATIVE        0x0400U
#define NETB_HFLAG_TRUNCATED          0x0200U
#define NETB_HFLAG_RECURS_DESIRED     0x0100U
#define NETB_HFLAG_RECURS_AVAILABLE   0x0080U
#define NETB_HFLAG_BROADCAST          0x0010U
#define NETB_HFLAG_REPLYCODE          0x0008U
#define NETB_HFLAG_REPLYCODE_NOERROR  0x0000U

/* NetBIOS question types */
#define NETB_QTYPE_NB                 0x0020U
#define NETB_QTYPE_NBSTAT             0x0021U

/** NetBIOS name flags */
#define NETB_NFLAG_UNIQUE             0x8000U
#define NETB_NFLAG_NODETYPE           0x6000U
#define NETB_NFLAG_NODETYPE_HNODE     0x6000U
#define NETB_NFLAG_NODETYPE_MNODE     0x4000U
#define NETB_NFLAG_NODETYPE_PNODE     0x2000U
#define NETB_NFLAG_NODETYPE_BNODE     0x0000U

#define NETB_NFLAG_NAME_IN_CONFLICT   0x0800U /* 1=Yes, 0=No */
#define NETB_NFLAG_NAME_IS_ACTIVE     0x0400U /* 1=Yes, 0=No */
#define NETB_NFLAG_NAME_IS_PERMANENT  0x0200U /* 1=Yes (Name is Permanent Node Name), 0=No */

/** NetBIOS message header */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct netbios_hdr {
  PACK_STRUCT_FIELD(u16_t trans_id);
  PACK_STRUCT_FIELD(u16_t flags);
  PACK_STRUCT_FIELD(u16_t questions);
  PACK_STRUCT_FIELD(u16_t answerRRs);
  PACK_STRUCT_FIELD(u16_t authorityRRs);
  PACK_STRUCT_FIELD(u16_t additionalRRs);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/** NetBIOS message question part */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct netbios_question_hdr {
  PACK_STRUCT_FLD_8(u8_t  nametype);
  PACK_STRUCT_FLD_8(u8_t  encname[(NETBIOS_NAME_LEN * 2) + 1]);
  PACK_STRUCT_FIELD(u16_t type);
  PACK_STRUCT_FIELD(u16_t cls);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/** NetBIOS message name part */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct netbios_name_hdr {
  PACK_STRUCT_FLD_8(u8_t  nametype);
  PACK_STRUCT_FLD_8(u8_t  encname[(NETBIOS_NAME_LEN * 2) + 1]);
  PACK_STRUCT_FIELD(u16_t type);
  PACK_STRUCT_FIELD(u16_t cls);
  PACK_STRUCT_FIELD(u32_t ttl);
  PACK_STRUCT_FIELD(u16_t datalen);
  PACK_STRUCT_FIELD(u16_t flags);
  PACK_STRUCT_FLD_S(ip4_addr_p_t addr);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/** NetBIOS message */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct netbios_resp {
  struct netbios_hdr      resp_hdr;
  struct netbios_name_hdr resp_name;
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/** The NBNS Structure Responds to a Name Query */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct netbios_answer {
  struct netbios_hdr      answer_hdr;
  /** the length of the next string */
  PACK_STRUCT_FIELD(u8_t  name_size);
  /** WARNING!!! this item may be of a different length (we use this struct for transmission) */
  PACK_STRUCT_FLD_8(u8_t  query_name[(NETBIOS_NAME_LEN * 2) + 1]);
  PACK_STRUCT_FIELD(u16_t packet_type);
  PACK_STRUCT_FIELD(u16_t cls);
  PACK_STRUCT_FIELD(u32_t ttl);
  PACK_STRUCT_FIELD(u16_t data_length);
#define OFFSETOF_STRUCT_NETBIOS_ANSWER_NUMBER_OF_NAMES 56
  /** number of names */
  PACK_STRUCT_FLD_8(u8_t  number_of_names);
  /** node name */
  PACK_STRUCT_FLD_8(u8_t  answer_name[NETBIOS_NAME_LEN]);
  /** node flags */
  PACK_STRUCT_FIELD(u16_t answer_name_flags);
  /** Unit ID */
  PACK_STRUCT_FLD_8(u8_t  unit_id[6]);
  /** Jumpers */
  PACK_STRUCT_FLD_8(u8_t  jumpers);
  /** Test result */
  PACK_STRUCT_FLD_8(u8_t  test_result);
  /** Version number */
  PACK_STRUCT_FIELD(u16_t version_number);
  /** Period of statistics */
  PACK_STRUCT_FIELD(u16_t period_of_statistics);
  /** Statistics */
  PACK_STRUCT_FIELD(u16_t number_of_crcs);
  /** Statistics */
  PACK_STRUCT_FIELD(u16_t number_of_alignment_errors);
  /** Statistics */
  PACK_STRUCT_FIELD(u16_t number_of_collisions);
  /** Statistics */
  PACK_STRUCT_FIELD(u16_t number_of_send_aborts);
  /** Statistics */
  PACK_STRUCT_FIELD(u32_t number_of_good_sends);
  /** Statistics */
  PACK_STRUCT_FIELD(u32_t number_of_good_receives);
  /** Statistics */
  PACK_STRUCT_FIELD(u16_t number_of_retransmits);
  /** Statistics */
  PACK_STRUCT_FIELD(u16_t number_of_no_resource_condition);
  /** Statistics */
  PACK_STRUCT_FIELD(u16_t number_of_free_command_blocks);
  /** Statistics */
  PACK_STRUCT_FIELD(u16_t total_number_of_command_blocks);
  /** Statistics */
  PACK_STRUCT_FIELD(u16_t max_total_number_of_command_blocks);
  /** Statistics */
  PACK_STRUCT_FIELD(u16_t number_of_pending_sessions);
  /** Statistics */
  PACK_STRUCT_FIELD(u16_t max_number_of_pending_sessions);
  /** Statistics */
  PACK_STRUCT_FIELD(u16_t max_total_sessions_possible);
  /** Statistics */
  PACK_STRUCT_FIELD(u16_t session_data_packet_size);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

#ifdef NETBIOS_LWIP_NAME
#define NETBIOS_LOCAL_NAME NETBIOS_LWIP_NAME
#else
static char netbiosns_local_name[NETBIOS_NAME_LEN];
#define NETBIOS_LOCAL_NAME netbiosns_local_name
#endif

static struct udp_pcb *netbiosns_pcb;

/** Decode a NetBIOS name (from packet to string) */
static int
netbiosns_name_decode(char *name_enc, char *name_dec, int name_dec_len)
{
  char *pname;
  char  cname;
  char  cnbname;
  int   idx = 0;

  LWIP_UNUSED_ARG(name_dec_len);

  /* Start decoding netbios name. */
  pname  = name_enc;
  for (;;) {
    /* Every two characters of the first level-encoded name
     * turn into one character in the decoded name. */
    cname = *pname;
    if (cname == '\0') {
      break;  /* no more characters */
    }
    if (cname == '.') {
      break;  /* scope ID follows */
    }
    if (!lwip_isupper(cname)) {
      /* Not legal. */
      return -1;
    }
    cname -= 'A';
    cnbname = cname << 4;
    pname++;

    cname = *pname;
    if (!lwip_isupper(cname)) {
      /* Not legal. */
      return -1;
    }
    cname -= 'A';
    cnbname |= cname;
    pname++;

    /* Do we have room to store the character? */
    if (idx < NETBIOS_NAME_LEN) {
      /* Yes - store the character. */
      name_dec[idx++] = (cnbname != ' ' ? cnbname : '\0');
    }
  }

  return 0;
}

#if 0 /* function currently unused */
/** Encode a NetBIOS name (from string to packet) - currently unused because
    we don't ask for names. */
static int
netbiosns_name_encode(char *name_enc, char *name_dec, int name_dec_len)
{
  char         *pname;
  char          cname;
  unsigned char ucname;
  int           idx = 0;

  /* Start encoding netbios name. */
  pname = name_enc;

  for (;;) {
    /* Every two characters of the first level-encoded name
     * turn into one character in the decoded name. */
    cname = *pname;
    if (cname == '\0') {
      break;  /* no more characters */
    }
    if (cname == '.') {
      break;  /* scope ID follows */
    }
    if ((cname < 'A' || cname > 'Z') && (cname < '0' || cname > '9')) {
      /* Not legal. */
      return -1;
    }

    /* Do we have room to store the character? */
    if (idx >= name_dec_len) {
      return -1;
    }

    /* Yes - store the character. */
    ucname = cname;
    name_dec[idx++] = ('A' + ((ucname >> 4) & 0x0F));
    name_dec[idx++] = ('A' + ( ucname     & 0x0F));
    pname++;
  }

  /* Fill with "space" coding */
  for (; idx < name_dec_len - 1;) {
    name_dec[idx++] = 'C';
    name_dec[idx++] = 'A';
  }

  /* Terminate string */
  name_dec[idx] = '\0';

  return 0;
}
#endif /* 0 */

/** NetBIOS Name service recv callback */
static void
netbiosns_recv(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  LWIP_UNUSED_ARG(arg);

  /* if packet is valid */
  if (p != NULL) {
    char   netbios_name[NETBIOS_NAME_LEN + 1];
    struct netbios_hdr          *netbios_hdr          = (struct netbios_hdr *)p->payload;
    struct netbios_question_hdr *netbios_question_hdr = (struct netbios_question_hdr *)(netbios_hdr + 1);

    /* is the packet long enough (we need the header in one piece) */
    if (p->len < (sizeof(struct netbios_hdr) + sizeof(struct netbios_question_hdr))) {
      /* packet too short */
      pbuf_free(p);
      return;
    }
    /* we only answer if we got a default interface */
    if (netif_default != NULL) {
      /* @todo: do we need to check answerRRs/authorityRRs/additionalRRs? */
      /* if the packet is a NetBIOS name query question */
      if (((netbios_hdr->flags & PP_NTOHS(NETB_HFLAG_OPCODE)) == PP_NTOHS(NETB_HFLAG_OPCODE_NAME_QUERY)) &&
          ((netbios_hdr->flags & PP_NTOHS(NETB_HFLAG_RESPONSE)) == 0) &&
          (netbios_hdr->questions == PP_NTOHS(1))) {
        /* decode the NetBIOS name */
        netbiosns_name_decode((char *)(netbios_question_hdr->encname), netbios_name, sizeof(netbios_name));
        /* check the request type */
        if (netbios_question_hdr->type == PP_HTONS(NETB_QTYPE_NB)) {
          /* if the packet is for us */
          if (lwip_strnicmp(netbios_name, NETBIOS_LOCAL_NAME, sizeof(NETBIOS_LOCAL_NAME)) == 0) {
            struct pbuf *q;
            struct netbios_resp *resp;

            q = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct netbios_resp), PBUF_RAM);
            if (q != NULL) {
              resp = (struct netbios_resp *)q->payload;

              /* prepare NetBIOS header response */
              resp->resp_hdr.trans_id      = netbios_hdr->trans_id;
              resp->resp_hdr.flags         = PP_HTONS(NETB_HFLAG_RESPONSE |
                                                      NETB_HFLAG_OPCODE_NAME_QUERY |
                                                      NETB_HFLAG_AUTHORATIVE |
                                                      NETB_HFLAG_RECURS_DESIRED);
              resp->resp_hdr.questions     = 0;
              resp->resp_hdr.answerRRs     = PP_HTONS(1);
              resp->resp_hdr.authorityRRs  = 0;
              resp->resp_hdr.additionalRRs = 0;

              /* prepare NetBIOS header datas */
              MEMCPY( resp->resp_name.encname, netbios_question_hdr->encname, sizeof(netbios_question_hdr->encname));
              resp->resp_name.nametype     = netbios_question_hdr->nametype;
              resp->resp_name.type         = netbios_question_hdr->type;
              resp->resp_name.cls          = netbios_question_hdr->cls;
              resp->resp_name.ttl          = PP_HTONL(NETBIOS_NAME_TTL);
              resp->resp_name.datalen      = PP_HTONS(sizeof(resp->resp_name.flags) + sizeof(resp->resp_name.addr));
              resp->resp_name.flags        = PP_HTONS(NETB_NFLAG_NODETYPE_BNODE);
              ip4_addr_copy(resp->resp_name.addr, *netif_ip4_addr(netif_default));

              /* send the NetBIOS response */
              udp_sendto(upcb, q, addr, port);

              /* free the "reference" pbuf */
              pbuf_free(q);
            }
          }
#if LWIP_NETBIOS_RESPOND_NAME_QUERY
        } else if (netbios_question_hdr->type == PP_HTONS(NETB_QTYPE_NBSTAT)) {
          /* if the packet is for us or general query */
          if (!lwip_strnicmp(netbios_name, NETBIOS_LOCAL_NAME, sizeof(NETBIOS_LOCAL_NAME)) ||
              !lwip_strnicmp(netbios_name, "*", sizeof(NETBIOS_LOCAL_NAME))) {
            /* general query - ask for our IP address */
            struct pbuf *q;
            struct netbios_answer *resp;

            q = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct netbios_answer), PBUF_RAM);
            if (q != NULL) {
              /* buffer to which a response is compiled */
              resp = (struct netbios_answer *) q->payload;

              /* Init response to zero, especially the statistics fields */
              memset(resp, 0, sizeof(*resp));

              /* copy the query to the response ID */
              resp->answer_hdr.trans_id        = netbios_hdr->trans_id;
              /* acknowledgment of termination */
              resp->answer_hdr.flags           = PP_HTONS(NETB_HFLAG_RESPONSE | NETB_HFLAG_OPCODE_NAME_QUERY | NETB_HFLAG_AUTHORATIVE);
              /* resp->answer_hdr.questions       = PP_HTONS(0); done by memset() */
              /* serial number of the answer */
              resp->answer_hdr.answerRRs       = PP_HTONS(1);
              /* resp->answer_hdr.authorityRRs    = PP_HTONS(0); done by memset() */
              /* resp->answer_hdr.additionalRRs   = PP_HTONS(0); done by memset() */
              /* we will copy the length of the station name */
              resp->name_size                  = netbios_question_hdr->nametype;
              /* we will copy the queried name */
              MEMCPY(resp->query_name, netbios_question_hdr->encname, (NETBIOS_NAME_LEN * 2) + 1);
              /* NBSTAT */
              resp->packet_type                = PP_HTONS(0x21);
              /* Internet name */
              resp->cls                        = PP_HTONS(1);
              /* resp->ttl                        = PP_HTONL(0); done by memset() */
              resp->data_length                = PP_HTONS(sizeof(struct netbios_answer) - offsetof(struct netbios_answer, number_of_names));
              resp->number_of_names            = 1;

              /* make windows see us as workstation, not as a server */
              memset(resp->answer_name, 0x20, NETBIOS_NAME_LEN - 1);
              /* strlen is checked to be < NETBIOS_NAME_LEN during initialization */
              MEMCPY(resp->answer_name, NETBIOS_LOCAL_NAME, strlen(NETBIOS_LOCAL_NAME));

              /* b-node, unique, active */
              resp->answer_name_flags          = PP_HTONS(NETB_NFLAG_NAME_IS_ACTIVE);

              /* Set responder netif MAC address */
              SMEMCPY(resp->unit_id, ip_current_input_netif()->hwaddr, sizeof(resp->unit_id));

              udp_sendto(upcb, q, addr, port);
              pbuf_free(q);
            }
          }
#endif /* LWIP_NETBIOS_RESPOND_NAME_QUERY */
        }
      }
    }
    /* free the pbuf */
    pbuf_free(p);
  }
}

/**
 * @ingroup netbiosns
 * Init netbios responder
 */
void
netbiosns_init(void)
{
  /* LWIP_ASSERT_CORE_LOCKED(); is checked by udp_new() */
#ifdef NETBIOS_LWIP_NAME
  LWIP_ASSERT("NetBIOS name is too long!", strlen(NETBIOS_LWIP_NAME) < NETBIOS_NAME_LEN);
#endif

  netbiosns_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
  if (netbiosns_pcb != NULL) {
    /* we have to be allowed to send broadcast packets! */
    ip_set_option(netbiosns_pcb, SOF_BROADCAST);
    udp_bind(netbiosns_pcb, IP_ANY_TYPE, LWIP_IANA_PORT_NETBIOS);
    udp_recv(netbiosns_pcb, netbiosns_recv, netbiosns_pcb);
  }
}

#ifndef NETBIOS_LWIP_NAME
/**
 * @ingroup netbiosns
 * Set netbios name. ATTENTION: the hostname must be less than 15 characters!
 *                              the NetBIOS name spec says the name MUST be upper case, so incoming name is forced into uppercase :-)
 */
void
netbiosns_set_name(const char *hostname)
{
  size_t i;
  size_t copy_len = strlen(hostname);
  LWIP_ASSERT_CORE_LOCKED();
  LWIP_ASSERT("NetBIOS name is too long!", copy_len < NETBIOS_NAME_LEN);
  if (copy_len >= NETBIOS_NAME_LEN) {
    copy_len = NETBIOS_NAME_LEN - 1;
  }

  /* make name into upper case */
  for (i = 0; i < copy_len; i++ ) {
    netbiosns_local_name[i] = (char)lwip_toupper(hostname[i]);
  }
  netbiosns_local_name[copy_len] = '\0';
}
#endif /* NETBIOS_LWIP_NAME */

/**
 * @ingroup netbiosns
 * Stop netbios responder
 */
void
netbiosns_stop(void)
{
  LWIP_ASSERT_CORE_LOCKED();
  if (netbiosns_pcb != NULL) {
    udp_remove(netbiosns_pcb);
    netbiosns_pcb = NULL;
  }
}

#endif /* LWIP_IPV4 && LWIP_UDP */
