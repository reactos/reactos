#ifndef _UAPI_LINUX_VIRTIO_RING_H
#define _UAPI_LINUX_VIRTIO_RING_H
/* An interface for efficient virtio implementation, currently for use by KVM
* and lguest, but hopefully others soon.  Do NOT change this since it will
* break existing servers and clients.
*
* This header is BSD licensed so anyone can use the definitions to implement
* compatible drivers/servers.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of IBM nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*
* Copyright Rusty Russell IBM Corporation 2007. */

#include "linux/types.h"
#include "linux/virtio_types.h"

/* We support indirect buffer descriptors */
#define VIRTIO_RING_F_INDIRECT_DESC	28

/* The Guest publishes the used index for which it expects an interrupt
* at the end of the avail ring. Host should ignore the avail->flags field. */
/* The Host publishes the avail index for which it expects a kick
* at the end of the used ring. Guest should ignore the used->flags field. */
#define VIRTIO_RING_F_EVENT_IDX		29

void vring_transport_features(VirtIODevice *vdev, u64 *features);
unsigned long vring_size(unsigned int num, unsigned long align, bool packed);

#endif /* _UAPI_LINUX_VIRTIO_RING_H */
