/******************************************************************************
 * Generic scheduler control interface.
 *
 * Mark Williamson, (C) 2004 Intel Research Cambridge
 */

#ifndef __XEN_PUBLIC_SCHED_CTL_H__
#define __XEN_PUBLIC_SCHED_CTL_H__

/* Scheduler types. */
#define SCHED_BVT      0

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
    u32     sched_id;
    u32     direction;
    domid_t domain;
    union {
        struct bvt_adjdom {
            u32 mcu_adv;            /* mcu advance: inverse of weight */
            u32 warpback;           /* warp? */
            s32 warpvalue;          /* warp value */
            long long warpl;        /* warp limit */
            long long warpu;        /* unwarp time requirement */
        } bvt;
    } u;
};

#endif /* __XEN_PUBLIC_SCHED_CTL_H__ */
