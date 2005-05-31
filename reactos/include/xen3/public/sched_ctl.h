/******************************************************************************
 * Generic scheduler control interface.
 *
 * Mark Williamson, (C) 2004 Intel Research Cambridge
 */

#ifndef __XEN_PUBLIC_SCHED_CTL_H__
#define __XEN_PUBLIC_SCHED_CTL_H__

/* Scheduler types. */
#define SCHED_BVT      0
#define SCHED_SEDF     4

/* Set or get info? */
#define SCHED_INFO_PUT 0
#define SCHED_INFO_GET 1

/*
 * Generic scheduler control command - used to adjust system-wide scheduler
 * parameters
 */
struct sched_ctl_cmd
{
    u32 sched_id;
    u32 direction;
    union {
        struct bvt_ctl {
            u32 ctx_allow;
        } bvt;
    } u;
};

struct sched_adjdom_cmd
{
    u32     sched_id;                 /*  0 */
    u32     direction;                /*  4 */
    domid_t domain;                   /*  8 */
    u16     __pad0;
    u32     __pad1;
    union {                           /* 16 */
        struct bvt_adjdom
        {
            u32 mcu_adv;            /* 16: mcu advance: inverse of weight */
            u32 warpback;           /* 20: warp? */
            s32 warpvalue;          /* 24: warp value */
            long long warpl;        /* 32: warp limit */
            long long warpu;        /* 40: unwarp time requirement */
        } PACKED bvt;
        
	struct sedf_adjdom
        {
            u64 period;     /* 16 */
            u64 slice;      /* 24 */
            u64 latency;    /* 32 */
            u16 extratime;  /* 36 */
	    u16 weight;     /* 38 */
        } PACKED sedf;

    } PACKED u;
} PACKED; /* 40 bytes */

#endif /* __XEN_PUBLIC_SCHED_CTL_H__ */
