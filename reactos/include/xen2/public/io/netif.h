/******************************************************************************
 * netif.h
 * 
 * Unified network-device I/O interface for Xen guest OSes.
 * 
 * Copyright (c) 2003-2004, Keir Fraser
 */

#ifndef __XEN_PUBLIC_IO_NETIF_H__
#define __XEN_PUBLIC_IO_NETIF_H__

typedef struct {
    memory_t addr;   /*  0: Machine address of packet.  */
    MEMORY_PADDING;
    u16      id;     /*  8: Echoed in response message. */
    u16      size;   /* 10: Packet size in bytes.       */
} PACKED netif_tx_request_t; /* 12 bytes */

typedef struct {
    u16      id;     /*  0 */
    s8       status; /*  2 */
    u8       __pad;  /*  3 */
} PACKED netif_tx_response_t; /* 4 bytes */

typedef struct {
    u16       id;    /*  0: Echoed in response message.        */
} PACKED netif_rx_request_t; /* 2 bytes */

typedef struct {
    memory_t addr;   /*  0: Machine address of packet.              */
    MEMORY_PADDING;
    u16      id;     /*  8:  */
    s16      status; /* 10: -ve: BLKIF_RSP_* ; +ve: Rx'ed pkt size. */
} PACKED netif_rx_response_t; /* 12 bytes */

/*
 * We use a special capitalised type name because it is _essential_ that all 
 * arithmetic on indexes is done on an integer type of the correct size.
 */
typedef u32 NETIF_RING_IDX;

/*
 * Ring indexes are 'free running'. That is, they are not stored modulo the
 * size of the ring buffer. The following macros convert a free-running counter
 * into a value that can directly index a ring-buffer array.
 */
#define MASK_NETIF_RX_IDX(_i) ((_i)&(NETIF_RX_RING_SIZE-1))
#define MASK_NETIF_TX_IDX(_i) ((_i)&(NETIF_TX_RING_SIZE-1))

#define NETIF_TX_RING_SIZE 256
#define NETIF_RX_RING_SIZE 256

/* This structure must fit in a memory page. */
typedef struct {
    /*
     * Frontend places packets into ring at tx_req_prod.
     * Frontend receives event when tx_resp_prod passes tx_event.
     * 'req_cons' is a shadow of the backend's request consumer -- the frontend
     * may use it to determine if all queued packets have been seen by the
     * backend.
     */
    NETIF_RING_IDX req_prod;       /*  0 */
    NETIF_RING_IDX req_cons;       /*  4 */
    NETIF_RING_IDX resp_prod;      /*  8 */
    NETIF_RING_IDX event;          /* 12 */
    union {                        /* 16 */
        netif_tx_request_t  req;
        netif_tx_response_t resp;
    } PACKED ring[NETIF_TX_RING_SIZE];
} PACKED netif_tx_interface_t;

/* This structure must fit in a memory page. */
typedef struct {
    /*
     * Frontend places empty buffers into ring at rx_req_prod.
     * Frontend receives event when rx_resp_prod passes rx_event.
     */
    NETIF_RING_IDX req_prod;       /*  0 */
    NETIF_RING_IDX resp_prod;      /*  4 */
    NETIF_RING_IDX event;          /*  8 */
    union {                        /* 12 */
        netif_rx_request_t  req;
        netif_rx_response_t resp;
    } PACKED ring[NETIF_RX_RING_SIZE];
} PACKED netif_rx_interface_t;

/* Descriptor status values */
#define NETIF_RSP_DROPPED         -2
#define NETIF_RSP_ERROR           -1
#define NETIF_RSP_OKAY             0

#endif
