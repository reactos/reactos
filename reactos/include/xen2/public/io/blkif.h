/******************************************************************************
 * blkif.h
 * 
 * Unified block-device I/O interface for Xen guest OSes.
 * 
 * Copyright (c) 2003-2004, Keir Fraser
 */

#ifndef __XEN_PUBLIC_IO_BLKIF_H__
#define __XEN_PUBLIC_IO_BLKIF_H__

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
    /* @f_a_s[2:0]=last_sect ; @f_a_s[5:3]=first_sect ; @f_a_s[:12]=frame.   */
    /* @first_sect: first sector in frame to transfer (inclusive).           */
    /* @last_sect: last sector in frame to transfer (inclusive).             */
    /* @frame: machine page frame number.                                    */
    unsigned long  frame_and_sects[BLKIF_MAX_SEGMENTS_PER_REQUEST];
} PACKED blkif_request_t;

#define blkif_first_sect(_fas) (((_fas)>>3)&7)
#define blkif_last_sect(_fas)  ((_fas)&7)

typedef struct {
    unsigned long   id;              /* copied from request */
    u8              operation;       /* copied from request */
    s16             status;          /* BLKIF_RSP_???       */
} PACKED blkif_response_t;

#define BLKIF_RSP_ERROR  -1 /* non-specific 'error' */
#define BLKIF_RSP_OKAY    0 /* non-specific 'okay'  */

/*
 * We use a special capitalised type name because it is _essential_ that all 
 * arithmetic on indexes is done on an integer type of the correct size.
 */
typedef u32 BLKIF_RING_IDX;

/*
 * Ring indexes are 'free running'. That is, they are not stored modulo the
 * size of the ring buffer. The following macro converts a free-running counter
 * into a value that can directly index a ring-buffer array.
 */
#define MASK_BLKIF_IDX(_i) ((_i)&(BLKIF_RING_SIZE-1))

typedef struct {
    BLKIF_RING_IDX req_prod;  /*  0: Request producer. Updated by front-end. */
    BLKIF_RING_IDX resp_prod; /*  4: Response producer. Updated by back-end. */
    union {                   /*  8 */
        blkif_request_t  req;
        blkif_response_t resp;
    } PACKED ring[BLKIF_RING_SIZE];
} PACKED blkif_ring_t;


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

/* XXX SMH: Type values below are chosen to match ide_xxx in Linux ide.h. */
#define VDISK_TYPE_FLOPPY  0x00
#define VDISK_TYPE_TAPE    0x01
#define VDISK_TYPE_CDROM   0x05
#define VDISK_TYPE_OPTICAL 0x07
#define VDISK_TYPE_DISK    0x20 

#define VDISK_TYPE_MASK    0x3F
#define VDISK_TYPE(_x)     ((_x) & VDISK_TYPE_MASK) 

/* The top two bits of the type field encode various flags. */
#define VDISK_FLAG_RO      0x40
#define VDISK_FLAG_VIRT    0x80
#define VDISK_READONLY(_x) ((_x) & VDISK_FLAG_RO)
#define VDISK_VIRTUAL(_x)  ((_x) & VDISK_FLAG_VIRT) 

typedef struct {
    blkif_sector_t capacity;     /*  0: Size in terms of 512-byte sectors.   */
    blkif_vdev_t   device;       /*  8: Device number (opaque 16 bit value). */
    u16            info;         /* 10: Device type and flags (VDISK_*).     */
} PACKED vdisk_t; /* 12 bytes */

#endif /* __XEN_PUBLIC_IO_BLKIF_H__ */
