/******************************************************************************
 * dom0_ops.h
 * 
 * Process command requests from domain-0 guest OS.
 * 
 * Copyright (c) 2002-2003, B Dragovic
 * Copyright (c) 2002-2004, K Fraser
 */


#ifndef __XEN_PUBLIC_DOM0_OPS_H__
#define __XEN_PUBLIC_DOM0_OPS_H__

#include "xen.h"
#include "sched_ctl.h"

/*
 * Make sure you increment the interface version whenever you modify this file!
 * This makes sure that old versions of dom0 tools will stop working in a
 * well-defined way (rather than crashing the machine, for instance).
 */
#define DOM0_INTERFACE_VERSION   0xAAAA1004

/************************************************************************/

#define DOM0_GETMEMLIST        2
typedef struct {
    /* IN variables. */
    domid_t       domain;
    memory_t      max_pfns;
    void         *buffer;
    /* OUT variables. */
    memory_t      num_pfns;
} dom0_getmemlist_t;

#define DOM0_SCHEDCTL          6
 /* struct sched_ctl_cmd is from sched-ctl.h   */
typedef struct sched_ctl_cmd dom0_schedctl_t;

#define DOM0_ADJUSTDOM         7
/* struct sched_adjdom_cmd is from sched-ctl.h */
typedef struct sched_adjdom_cmd dom0_adjustdom_t;

#define DOM0_CREATEDOMAIN      8
typedef struct {
    /* IN parameters. */
    memory_t     memory_kb;
    u32          cpu;
    /* IN/OUT parameters. */
    /* If 0, domain is allocated. If non-zero use it unless in use. */
    domid_t      domain;
    /* OUT parameters. */
} dom0_createdomain_t;

#define DOM0_DESTROYDOMAIN     9
typedef struct {
    /* IN variables. */
    domid_t      domain;
} dom0_destroydomain_t;

#define DOM0_PAUSEDOMAIN      10
typedef struct {
    /* IN parameters. */
    domid_t domain;
} dom0_pausedomain_t;

#define DOM0_UNPAUSEDOMAIN    11
typedef struct {
    /* IN parameters. */
    domid_t domain;
} dom0_unpausedomain_t;

#define DOM0_GETDOMAININFO    12
typedef struct {
    /* IN variables. */
    domid_t  domain;                  /* NB. IN/OUT variable. */
    u16      exec_domain;
    /* OUT variables. */
#define DOMFLAGS_DYING     (1<<0) /* Domain is scheduled to die.             */
#define DOMFLAGS_CRASHED   (1<<1) /* Crashed domain; frozen for postmortem.  */
#define DOMFLAGS_SHUTDOWN  (1<<2) /* The guest OS has shut itself down.      */
#define DOMFLAGS_PAUSED    (1<<3) /* Currently paused by control software.   */
#define DOMFLAGS_BLOCKED   (1<<4) /* Currently blocked pending an event.     */
#define DOMFLAGS_RUNNING   (1<<5) /* Domain is currently running.            */
#define DOMFLAGS_CPUMASK      255 /* CPU to which this domain is bound.      */
#define DOMFLAGS_CPUSHIFT       8
#define DOMFLAGS_SHUTDOWNMASK 255 /* DOMFLAGS_SHUTDOWN guest-supplied code.  */
#define DOMFLAGS_SHUTDOWNSHIFT 16
    u32      flags;
    full_execution_context_t *ctxt;   /* NB. IN/OUT variable. */
    memory_t tot_pages;
    memory_t max_pages;
    memory_t shared_info_frame;       /* MFN of shared_info struct */
    u64      cpu_time;
} dom0_getdomaininfo_t;

#define DOM0_SETDOMAININFO      13
typedef struct {
    /* IN variables. */
    domid_t                   domain;
    u16                       exec_domain;
    /* IN/OUT parameters */
    full_execution_context_t *ctxt;
} dom0_setdomaininfo_t;

#define DOM0_MSR              15
typedef struct {
    /* IN variables. */
    u32 write;
    u32 cpu_mask;
    u32 msr;
    u32 in1;
    u32 in2;
    /* OUT variables. */
    u32 out1;
    u32 out2;
} dom0_msr_t;

#define DOM0_DEBUG            16
typedef struct {
    /* IN variables. */
    domid_t domain;
    u8  opcode;
    u32 in1;
    u32 in2;
    u32 in3;
    u32 in4;
    /* OUT variables. */
    u32 status;
    u32 out1;
    u32 out2;
} dom0_debug_t;

/*
 * Set clock such that it would read <secs,usecs> after 00:00:00 UTC,
 * 1 January, 1970 if the current system time was <system_time>.
 */
#define DOM0_SETTIME          17
typedef struct {
    /* IN variables. */
    u32 secs;
    u32 usecs;
    u64 system_time;
} dom0_settime_t;

#define DOM0_GETPAGEFRAMEINFO 18
#define NOTAB 0         /* normal page */
#define L1TAB (1<<28)
#define L2TAB (2<<28)
#define L3TAB (3<<28)
#define L4TAB (4<<28)
#define LPINTAB  (1<<31)
#define XTAB  (0xf<<28) /* invalid page */
#define LTAB_MASK XTAB
#define LTABTYPE_MASK (0x7<<28)

typedef struct {
    /* IN variables. */
    memory_t pfn;          /* Machine page frame number to query.       */
    domid_t domain;        /* To which domain does the frame belong?    */
    /* OUT variables. */
    /* Is the page PINNED to a type? */
    u32 type;              /* see above type defs */
} dom0_getpageframeinfo_t;

/*
 * Read console content from Xen buffer ring.
 */
#define DOM0_READCONSOLE      19
typedef struct {
    memory_t str;
    u32      count;
    u32      cmd;
} dom0_readconsole_t;

/* 
 * Pin Domain to a particular CPU  (use -1 to unpin)
 */
#define DOM0_PINCPUDOMAIN     20
typedef struct {
    /* IN variables. */
    domid_t      domain;
    u16          exec_domain;
    s32          cpu;                 /*  -1 implies unpin */
} dom0_pincpudomain_t;

/* Get trace buffers machine base address */
#define DOM0_TBUFCONTROL       21
typedef struct {
    /* IN variables */
#define DOM0_TBUF_GET_INFO     0
#define DOM0_TBUF_SET_CPU_MASK 1
#define DOM0_TBUF_SET_EVT_MASK 2
    u8 op;
    /* IN/OUT variables */
    unsigned long cpu_mask;
    u32           evt_mask;
    /* OUT variables */
    memory_t mach_addr;
    u32      size;
} dom0_tbufcontrol_t;

/*
 * Get physical information about the host machine
 */
#define DOM0_PHYSINFO         22
typedef struct {
    u32      ht_per_core;
    u32      cores;
    u32      cpu_khz;
    memory_t total_pages;
    memory_t free_pages;
} dom0_physinfo_t;

/* 
 * Allow a domain access to a physical PCI device
 */
#define DOM0_PCIDEV_ACCESS    23
typedef struct {
    /* IN variables. */
    domid_t      domain;
    u32          bus;
    u32          dev;
    u32          func;
    u32          enable;
} dom0_pcidev_access_t;

/*
 * Get the ID of the current scheduler.
 */
#define DOM0_SCHED_ID        24
typedef struct {
    /* OUT variable */
    u32 sched_id;
} dom0_sched_id_t;

/* 
 * Control shadow pagetables operation
 */
#define DOM0_SHADOW_CONTROL  25

#define DOM0_SHADOW_CONTROL_OP_OFF         0
#define DOM0_SHADOW_CONTROL_OP_ENABLE_TEST 1
#define DOM0_SHADOW_CONTROL_OP_ENABLE_LOGDIRTY 2

#define DOM0_SHADOW_CONTROL_OP_FLUSH       10     /* table ops */
#define DOM0_SHADOW_CONTROL_OP_CLEAN       11
#define DOM0_SHADOW_CONTROL_OP_PEEK        12

typedef struct dom0_shadow_control
{
    u32 fault_count;
    u32 dirty_count;
    u32 dirty_net_count;     
    u32 dirty_block_count;     
} dom0_shadow_control_stats_t;

typedef struct {
    /* IN variables. */
    domid_t        domain;
    u32            op;
    unsigned long *dirty_bitmap; /* pointer to locked buffer */
    /* IN/OUT variables. */
    memory_t       pages;        /* size of buffer, updated with actual size */
    /* OUT variables. */
    dom0_shadow_control_stats_t stats;
} dom0_shadow_control_t;

#define DOM0_SETDOMAININITIALMEM   27
typedef struct {
    /* IN variables. */
    domid_t     domain;
    memory_t    initial_memkb;
} dom0_setdomaininitialmem_t;

#define DOM0_SETDOMAINMAXMEM   28
typedef struct {
    /* IN variables. */
    domid_t     domain;
    memory_t    max_memkb;
} dom0_setdomainmaxmem_t;

#define DOM0_GETPAGEFRAMEINFO2 29   /* batched interface */
typedef struct {
    /* IN variables. */
    domid_t  domain;
    memory_t num;
    /* IN/OUT variables. */
    unsigned long *array;
} dom0_getpageframeinfo2_t;

/*
 * Request memory range (@pfn, @pfn+@nr_pfns-1) to have type @type.
 * On x86, @type is an architecture-defined MTRR memory type.
 * On success, returns the MTRR that was used (@reg) and a handle that can
 * be passed to DOM0_DEL_MEMTYPE to accurately tear down the new setting.
 * (x86-specific).
 */
#define DOM0_ADD_MEMTYPE         31
typedef struct {
    /* IN variables. */
    memory_t pfn;
    memory_t nr_pfns;
    u32      type;
    /* OUT variables. */
    u32      handle;
    u32      reg;
} dom0_add_memtype_t;

/*
 * Tear down an existing memory-range type. If @handle is remembered then it
 * should be passed in to accurately tear down the correct setting (in case
 * of overlapping memory regions with differing types). If it is not known
 * then @handle should be set to zero. In all cases @reg must be set.
 * (x86-specific).
 */
#define DOM0_DEL_MEMTYPE         32
typedef struct {
    /* IN variables. */
    u32      handle;
    u32      reg;
} dom0_del_memtype_t;

/* Read current type of an MTRR (x86-specific). */
#define DOM0_READ_MEMTYPE        33
typedef struct {
    /* IN variables. */
    u32      reg;
    /* OUT variables. */
    memory_t pfn;
    memory_t nr_pfns;
    u32      type;
} dom0_read_memtype_t;

/* Interface for controlling Xen software performance counters. */
#define DOM0_PERFCCONTROL        34
/* Sub-operations: */
#define DOM0_PERFCCONTROL_OP_RESET 1   /* Reset all counters to zero. */
#define DOM0_PERFCCONTROL_OP_QUERY 2   /* Get perfctr information. */
typedef struct {
    u8      name[80];               /*  name of perf counter */
    u32     nr_vals;                /* number of values for this counter */
    u32     vals[64];               /* array of values */
} dom0_perfc_desc_t;
typedef struct {
    /* IN variables. */
    u32            op;                /*  DOM0_PERFCCONTROL_OP_??? */
    /* OUT variables. */
    u32            nr_counters;       /*  number of counters */
    dom0_perfc_desc_t *desc;          /*  counter information (or NULL) */
} dom0_perfccontrol_t;

#define DOM0_MICROCODE           35
typedef struct {
    /* IN variables. */
    void   *data;                     /* Pointer to microcode data */
    u32     length;                   /* Length of microcode data. */
} dom0_microcode_t;

#define DOM0_IOPORT_PERMISSION   36
typedef struct {
    domid_t domain;                   /* domain to be affected */
    u16     first_port;               /* first port int range */
    u16     nr_ports;                 /* size of port range */
    u16     allow_access;             /* allow or deny access to range? */
} dom0_ioport_permission_t;

typedef struct {
    u32 cmd;
    u32 interface_version; /* DOM0_INTERFACE_VERSION */
    union {
        dom0_createdomain_t      createdomain;
        dom0_pausedomain_t       pausedomain;
        dom0_unpausedomain_t     unpausedomain;
        dom0_destroydomain_t     destroydomain;
        dom0_getmemlist_t        getmemlist;
        dom0_schedctl_t          schedctl;
        dom0_adjustdom_t         adjustdom;
        dom0_setdomaininfo_t     setdomaininfo;
        dom0_getdomaininfo_t     getdomaininfo;
        dom0_getpageframeinfo_t  getpageframeinfo;
        dom0_msr_t               msr;
        dom0_debug_t             debug;
        dom0_settime_t           settime;
        dom0_readconsole_t       readconsole;
        dom0_pincpudomain_t      pincpudomain;
        dom0_tbufcontrol_t       tbufcontrol;
        dom0_physinfo_t          physinfo;
        dom0_pcidev_access_t     pcidev_access;
        dom0_sched_id_t          sched_id;
        dom0_shadow_control_t    shadow_control;
        dom0_setdomaininitialmem_t setdomaininitialmem;
        dom0_setdomainmaxmem_t   setdomainmaxmem;
        dom0_getpageframeinfo2_t getpageframeinfo2;
        dom0_add_memtype_t       add_memtype;
        dom0_del_memtype_t       del_memtype;
        dom0_read_memtype_t      read_memtype;
        dom0_perfccontrol_t      perfccontrol;
        dom0_microcode_t         microcode;
        dom0_ioport_permission_t ioport_permission;
    } u;
} dom0_op_t;

#endif /* __XEN_PUBLIC_DOM0_OPS_H__ */
