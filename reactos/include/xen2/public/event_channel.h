/******************************************************************************
 * event_channel.h
 * 
 * Event channels between domains.
 * 
 * Copyright (c) 2003-2004, K A Fraser.
 */

#ifndef __XEN_PUBLIC_EVENT_CHANNEL_H__
#define __XEN_PUBLIC_EVENT_CHANNEL_H__

/*
 * EVTCHNOP_alloc_unbound: Allocate a fresh local port and prepare
 * it for binding to <dom>.
 */
#define EVTCHNOP_alloc_unbound    6
typedef struct {
    /* IN parameters */
    domid_t dom;                      /*  0 */
    u16     __pad;
    /* OUT parameters */
    u32     port;                     /*  4 */
} PACKED evtchn_alloc_unbound_t; /* 8 bytes */

/*
 * EVTCHNOP_bind_interdomain: Construct an interdomain event channel between
 * <dom1> and <dom2>. Either <port1> or <port2> may be wildcarded by setting to
 * zero. On successful return both <port1> and <port2> are filled in and
 * <dom1,port1> is fully bound to <dom2,port2>.
 * 
 * NOTES:
 *  1. A wildcarded port is allocated from the relevant domain's free list
 *     (i.e., some port that was previously EVTCHNSTAT_closed). However, if the
 *     remote port pair is already fully bound then a port is not allocated,
 *     and instead the existing local port is returned to the caller.
 *  2. If the caller is unprivileged then <dom1> must be DOMID_SELF.
 *  3. If the caller is unprivileged and <dom2,port2> is EVTCHNSTAT_closed
 *     then <dom2> must be DOMID_SELF.
 *  4. If either port is already bound then it must be bound to the other
 *     specified domain and port (if not wildcarded).
 *  5. If either port is awaiting binding (EVTCHNSTAT_unbound) then it must
 *     be awaiting binding to the other domain, and the other port pair must
 *     be closed or unbound.
 */
#define EVTCHNOP_bind_interdomain 0
typedef struct {
    /* IN parameters. */
    domid_t dom1, dom2;               /*  0,  2 */
    /* IN/OUT parameters. */
    u32     port1, port2;             /*  4,  8 */
} PACKED evtchn_bind_interdomain_t; /* 12 bytes */

/*
 * EVTCHNOP_bind_virq: Bind a local event channel to IRQ <irq>.
 * NOTES:
 *  1. A virtual IRQ may be bound to at most one event channel per domain.
 */
#define EVTCHNOP_bind_virq        1
typedef struct {
    /* IN parameters. */
    u32 virq;                         /*  0 */
    /* OUT parameters. */
    u32 port;                         /*  4 */
} PACKED evtchn_bind_virq_t; /* 8 bytes */

/*
 * EVTCHNOP_bind_pirq: Bind a local event channel to IRQ <irq>.
 * NOTES:
 *  1. A physical IRQ may be bound to at most one event channel per domain.
 *  2. Only a sufficiently-privileged domain may bind to a physical IRQ.
 */
#define EVTCHNOP_bind_pirq        2
typedef struct {
    /* IN parameters. */
    u32 pirq;                         /*  0 */
#define BIND_PIRQ__WILL_SHARE 1
    u32 flags; /* BIND_PIRQ__* */     /*  4 */
    /* OUT parameters. */
    u32 port;                         /*  8 */
} PACKED evtchn_bind_pirq_t; /* 12 bytes */

/*
 * EVTCHNOP_close: Close the communication channel which has an endpoint at
 * <dom, port>. If the channel is interdomain then the remote end is placed in
 * the unbound state (EVTCHNSTAT_unbound), awaiting a new connection.
 * NOTES:
 *  1. <dom> may be specified as DOMID_SELF.
 *  2. Only a sufficiently-privileged domain may close an event channel
 *     for which <dom> is not DOMID_SELF.
 */
#define EVTCHNOP_close            3
typedef struct {
    /* IN parameters. */
    domid_t dom;                      /*  0 */
    u16     __pad;
    u32     port;                     /*  4 */
    /* No OUT parameters. */
} PACKED evtchn_close_t; /* 8 bytes */

/*
 * EVTCHNOP_send: Send an event to the remote end of the channel whose local
 * endpoint is <DOMID_SELF, local_port>.
 */
#define EVTCHNOP_send             4
typedef struct {
    /* IN parameters. */
    u32     local_port;               /*  0 */
    /* No OUT parameters. */
} PACKED evtchn_send_t; /* 4 bytes */

/*
 * EVTCHNOP_status: Get the current status of the communication channel which
 * has an endpoint at <dom, port>.
 * NOTES:
 *  1. <dom> may be specified as DOMID_SELF.
 *  2. Only a sufficiently-privileged domain may obtain the status of an event
 *     channel for which <dom> is not DOMID_SELF.
 */
#define EVTCHNOP_status           5
typedef struct {
    /* IN parameters */
    domid_t dom;                      /*  0 */
    u16     __pad;
    u32     port;                     /*  4 */
    /* OUT parameters */
#define EVTCHNSTAT_closed       0  /* Channel is not in use.                 */
#define EVTCHNSTAT_unbound      1  /* Channel is waiting interdom connection.*/
#define EVTCHNSTAT_interdomain  2  /* Channel is connected to remote domain. */
#define EVTCHNSTAT_pirq         3  /* Channel is bound to a phys IRQ line.   */
#define EVTCHNSTAT_virq         4  /* Channel is bound to a virtual IRQ line */
    u32     status;                   /*  8 */
    union {                           /* 12 */
        struct {
            domid_t dom;                              /* 12 */
        } PACKED unbound; /* EVTCHNSTAT_unbound */
        struct {
            domid_t dom;                              /* 12 */
            u16     __pad;
            u32     port;                             /* 16 */
        } PACKED interdomain; /* EVTCHNSTAT_interdomain */
        u32 pirq;      /* EVTCHNSTAT_pirq        */   /* 12 */
        u32 virq;      /* EVTCHNSTAT_virq        */   /* 12 */
    } PACKED u;
} PACKED evtchn_status_t; /* 20 bytes */

typedef struct {
    u32 cmd; /* EVTCHNOP_* */         /*  0 */
    u32 __reserved;                   /*  4 */
    union {                           /*  8 */
        evtchn_alloc_unbound_t    alloc_unbound;
        evtchn_bind_interdomain_t bind_interdomain;
        evtchn_bind_virq_t        bind_virq;
        evtchn_bind_pirq_t        bind_pirq;
        evtchn_close_t            close;
        evtchn_send_t             send;
        evtchn_status_t           status;
        u8                        __dummy[24];
    } PACKED u;
} PACKED evtchn_op_t; /* 32 bytes */

#endif /* __XEN_PUBLIC_EVENT_CHANNEL_H__ */
