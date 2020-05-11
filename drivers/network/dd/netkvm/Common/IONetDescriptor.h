/*
 * This file contains common guest/host definition, related
 * to VirtIO network adapter
 *
 * Copyright (c) 2008-2017 Red Hat, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met :
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and / or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of their contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef IONETDESCRIPTOR_H
#define IONETDESCRIPTOR_H

#pragma pack (push)
#pragma pack (1)
/* This is the first element of the scatter-gather list.  If you don't
 * specify GSO or CSUM features, you can simply ignore the header. */
typedef struct _tagvirtio_net_hdr
{
#define VIRTIO_NET_HDR_F_NEEDS_CSUM 1   // Use csum_start, csum_offset
#define VIRTIO_NET_HDR_F_DATA_VALID 2   // Host checked checksum, no need to recheck
    u8 flags;
#define VIRTIO_NET_HDR_GSO_NONE     0   // Not a GSO frame
#define VIRTIO_NET_HDR_GSO_TCPV4    1   // GSO frame, IPv4 TCP (TSO)
#define VIRTIO_NET_HDR_GSO_UDP      3   // GSO frame, IPv4 UDP (UFO)
#define VIRTIO_NET_HDR_GSO_TCPV6    4   // GSO frame, IPv6 TCP
#define VIRTIO_NET_HDR_GSO_ECN      0x80    // TCP has ECN set
    u8 gso_type;
    u16 hdr_len;                        // Ethernet + IP + tcp/udp hdrs
    u16 gso_size;                       // Bytes to append to gso_hdr_len per frame
    u16 csum_start;                     // Position to start checksumming from
    u16 csum_offset;                    // Offset after that to place checksum
}virtio_net_hdr_basic;

typedef struct _tagvirtio_net_hdr_ext
{
    virtio_net_hdr_basic BasicHeader;
    u16 nBuffers;
}virtio_net_hdr_ext;

/*
 * Control virtqueue data structures
 *
 * The control virtqueue expects a header in the first sg entry
 * and an ack/status response in the last entry.  Data for the
 * command goes in between.
 */
typedef struct tag_virtio_net_ctrl_hdr {
    u8 class_of_command;
    u8 cmd;
}virtio_net_ctrl_hdr;

typedef u8 virtio_net_ctrl_ack;

#define VIRTIO_NET_OK     0
#define VIRTIO_NET_ERR    1

/*
 * Control the RX mode, ie. promiscuous, allmulti, etc...
 * All commands require an "out" sg entry containing a 1 byte
 * state value, zero = disable, non-zero = enable.  Commands
 * 0 and 1 are supported with the VIRTIO_NET_F_CTRL_RX feature.
 * Commands 2-5 are added with VIRTIO_NET_F_CTRL_RX_EXTRA.
 */
#define VIRTIO_NET_CTRL_RX_MODE    0
 #define VIRTIO_NET_CTRL_RX_MODE_PROMISC      0
 #define VIRTIO_NET_CTRL_RX_MODE_ALLMULTI     1
 #define VIRTIO_NET_CTRL_RX_MODE_ALLUNI       2
 #define VIRTIO_NET_CTRL_RX_MODE_NOMULTI      3
 #define VIRTIO_NET_CTRL_RX_MODE_NOUNI        4
 #define VIRTIO_NET_CTRL_RX_MODE_NOBCAST      5

/*
 * Control the MAC filter table.
 *
 * The MAC filter table is managed by the hypervisor, the guest should
 * assume the size is infinite.  Filtering should be considered
 * non-perfect, ie. based on hypervisor resources, the guest may
 * received packets from sources not specified in the filter list.
 *
 * In addition to the class/cmd header, the TABLE_SET command requires
 * two out scatterlists.  Each contains a 4 byte count of entries followed
 * by a concatenated byte stream of the ETH_ALEN MAC addresses.  The
 * first sg list contains unicast addresses, the second is for multicast.
 * This functionality is present if the VIRTIO_NET_F_CTRL_RX feature
 * is available.
 */
#define ETH_ALEN    6

struct virtio_net_ctrl_mac {
    u32 entries;
    // follows
    //u8 macs[][ETH_ALEN];
};
#define VIRTIO_NET_CTRL_MAC                  1
  #define VIRTIO_NET_CTRL_MAC_TABLE_SET        0

/*
 * Control VLAN filtering
 *
 * The VLAN filter table is controlled via a simple ADD/DEL interface.
 * VLAN IDs not added may be filterd by the hypervisor.  Del is the
 * opposite of add.  Both commands expect an out entry containing a 2
 * byte VLAN ID.  VLAN filtering is available with the
 * VIRTIO_NET_F_CTRL_VLAN feature bit.
 */
#define VIRTIO_NET_CTRL_VLAN                 2
  #define VIRTIO_NET_CTRL_VLAN_ADD             0
  #define VIRTIO_NET_CTRL_VLAN_DEL             1


#pragma pack (pop)

#endif
