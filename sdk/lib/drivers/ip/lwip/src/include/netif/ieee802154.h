/**
 * @file
 * Definitions for IEEE 802.15.4 MAC frames
 */

/*
 * Copyright (c) 2018 Simon Goldschmidt.
 * All rights reserved.
 *
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
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */
#ifndef LWIP_HDR_NETIF_IEEE802154_H
#define LWIP_HDR_NETIF_IEEE802154_H

#include "lwip/opt.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
/** General MAC frame format
 * This shows the full featured header, mainly for documentation.
 * Some fields are omitted or shortened to achieve frame compression.
 */
struct ieee_802154_hdr {
  /** See IEEE_802154_FC_* defines */
  PACK_STRUCT_FIELD(u16_t frame_control);
  /** Sequence number is omitted if IEEE_802154_FC_SEQNO_SUPPR is set in frame_control */
  PACK_STRUCT_FLD_8(u8_t  sequence_number);
  /** Destination PAN ID is omitted if Destination Addressing Mode is 0 */
  PACK_STRUCT_FIELD(u16_t destination_pan_id);
  /** Destination Address is omitted if Destination Addressing Mode is 0 */
  PACK_STRUCT_FLD_8(u8_t destination_address[8]);
  /** Source PAN ID is omitted if Source Addressing Mode is 0
      or if IEEE_802154_FC_PANID_COMPR is set in frame control*/
  PACK_STRUCT_FIELD(u16_t source_pan_id);
  /** Source Address is omitted if Source Addressing Mode is 0 */
  PACK_STRUCT_FLD_8(u8_t source_address[8]);
  /* The rest is variable */
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/* Addressing modes (2 bits) */
#define IEEE_802154_ADDR_MODE_NO_ADDR   0x00 /* PAN ID and address fields are not present */
#define IEEE_802154_ADDR_MODE_RESERVED  0x01 /* Reserved */
#define IEEE_802154_ADDR_MODE_SHORT     0x02 /* Address field contains a short address (16 bit) */
#define IEEE_802154_ADDR_MODE_EXT       0x03 /* Address field contains an extended address (64 bit) */

/* IEEE 802.15.4 Frame Control definitions (2 bytes; see IEEE 802.15.4-2015 ch. 7.2.1) */
#define IEEE_802154_FC_FT_MASK             0x0007 /* bits 0..2: Frame Type */
#define IEEE_802154_FC_FT_BEACON               0x00
#define IEEE_802154_FC_FT_DATA                 0x01
#define IEEE_802154_FC_FT_ACK                  0x02
#define IEEE_802154_FC_FT_MAC_CMD              0x03
#define IEEE_802154_FC_FT_RESERVED             0x04
#define IEEE_802154_FC_FT_MULTIPURPOSE         0x05
#define IEEE_802154_FC_FT_FRAG                 0x06
#define IEEE_802154_FC_FT_EXT                  0x07
#define IEEE_802154_FC_SEC_EN              0x0008 /* bit 3: Security Enabled */
#define IEEE_802154_FC_FRAME_PEND          0x0010 /* bit 4: Frame Pending */
#define IEEE_802154_FC_ACK_REQ             0x0020 /* bit 5: AR (ACK required) */
#define IEEE_802154_FC_PANID_COMPR         0x0040 /* bit 6: PAN ID Compression (src and dst are equal, src PAN ID omitted) */
#define IEEE_802154_FC_RESERVED            0x0080
#define IEEE_802154_FC_SEQNO_SUPPR         0x0100 /* bit 8: Sequence Number Suppression */
#define IEEE_802154_FC_IE_PRESENT          0x0200 /* bit 9: IE Present */
#define IEEE_802154_FC_DST_ADDR_MODE_MASK  0x0c00 /* bits 10..11: Destination Addressing Mode */
#define IEEE_802154_FC_DST_ADDR_MODE_NO_ADDR   (IEEE_802154_ADDR_MODE_NO_ADDR << 10)
#define IEEE_802154_FC_DST_ADDR_MODE_SHORT     (IEEE_802154_ADDR_MODE_SHORT << 10)
#define IEEE_802154_FC_DST_ADDR_MODE_EXT       (IEEE_802154_ADDR_MODE_EXT << 10)
#define IEEE_802154_FC_FRAME_VERSION_MASK  0x3000 /* bits 12..13: Frame Version */
#define IEEE_802154_FC_FRAME_VERSION_GET(x)    (((x) & IEEE_802154_FC_FRAME_VERSION_MASK) >> 12)
#define IEEE_802154_FC_SRC_ADDR_MODE_MASK  0xc000 /* bits 14..15: Source Addressing Mode */
#define IEEE_802154_FC_SRC_ADDR_MODE_SHORT     (IEEE_802154_ADDR_MODE_SHORT << 14)
#define IEEE_802154_FC_SRC_ADDR_MODE_EXT       (IEEE_802154_ADDR_MODE_EXT << 14)

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_NETIF_IEEE802154_H */
