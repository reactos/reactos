/******************************************************************************
 * blkif.h
 * 
 * Unified block-device I/O interface for Xen guest OSes.
 * 
 * Copyright (c) 2003-2004, Keir Fraser
 */

#ifndef __XEN_PUBLIC_IO_BLKIF_H__
#define __XEN_PUBLIC_IO_BLKIF_H__

#include "ring.h"

#define blkif_vdev_t   u16
#define blkif_sector_t u64

#define BLKIF_OP_READ      0
#define BLKIF_OP_WRITE     1
#define BLKIF_OP_PROBE     2

/* NB. Ring size must be small enough for sizeof(blkif_ring_t) <= PAGE_SIZE. */
#define BLKIF_RING_SIZE        64

/*
 * Maximum scatter/gather segments per request.
 * This is carefully chosen so that sizeof(blkif_ring_t) <= PAGE_SIZE.
 * NB. This could be 12 if the ring indexes weren't stored in the same page.
 */
#define BLKIF_MAX_SEGMENTS_PER_REQUEST 11

typedef struct {
    u8             operation;    /*  0: BLKIF_OP_???                         */
    u8             nr_segments;  /*  1: number of segments                   */
    blkif_vdev_t   device;       /*  2: only for read/write requests         */
    unsigned long  id;           /*  4: private guest value, echoed in resp  */
    blkif_sector_t sector_number;    /* start sector idx on disk (r/w only)  */
    /* @f_a_s[2:0]=last_sect ; @f_a_s[5:3]=first_sect                        */
#ifdef CONFIG_XEN_BLKDEV_GRANT
    /* @f_a_s[:16]= grant reference (16 bits)                                */
#else
    /* @f_a_s[:12]=@frame: machine page frame number.                        */
#endif
    /* @first_sect: first sector in frame to transfer (inclusive).           */
    /* @last_sect: last sector in frame to transfer (inclusive).             */
    unsigned long  frame_and_sects[BLKIF_MAX_SEGMENTS_PER_REQUEST];
} PACKED blkif_request_t;

#define blkif_first_sect(_fas) (((_fas)>>3)&7)
#define blkif_last_sect(_fas)  ((_fas)&7)

#ifdef CONFIG_XEN_BLKDEV_GRANT
#define blkif_gref_from_fas(_fas) ((_fas)>>16)
#endif

typedef struct {
    unsigned long   id;              /* copied from request */
    u8              operation;       /* copied from request */
    s16             status;          /* BLKIF_RSP_???       */
} PACKED blkif_response_t;

#define BLKIF_RSP_ERROR  -1 /* non-specific 'error' */
#define BLKIF_RSP_OKAY    0 /* non-specific 'okay'  */

/*
 * Generate blkif ring structures and types.
 */

DEFINE_RING_TYPES(blkif, blkif_request_t, blkif_response_t);

/*
 * BLKIF_OP_PROBE:
 * The request format for a probe request is constrained as follows:
 *  @operation   == BLKIF_OP_PROBE
 *  @nr_segments == size of probe buffer in pages
 *  @device      == unused (zero)
 *  @id          == any value (echoed in response message)
 *  @sector_num  == unused (zero)
 *  @frame_and_sects == list of page-sized buffers.
 *                       (i.e., @first_sect == 0, @last_sect == 7).
 * 
 * The response is a list of vdisk_t elements copied into the out-of-band
 * probe buffer. On success the response status field contains the number
 * of vdisk_t elements.
 */

#define VDISK_CDROM        0x1
#define VDISK_REMOVABLE    0x2
#define VDISK_READONLY     0x4

typedef struct vdisk {
    blkif_sector_t capacity;     /*  0: Size in terms of 512-byte sectors.   */
    blkif_vdev_t   device;       /*  8: Device number (opaque 16 bit value). */
    u16            info;         /* 10: Device type and flags (VDISK_*).     */
    u16            sector_size;  /* 12: Minimum alignment for requests.      */
    u16            _pad;
} PACKED vdisk_t; /* 16 bytes */

#endif /* __XEN_PUBLIC_IO_BLKIF_H__ */
