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
#define DOM0_INTERFACE_VERSION   0xAAAA001A

/************************************************************************/

#define DOM0_GETMEMLIST        2
typedef struct {
    /* IN variables. */
    domid_t       domain;             /*  0 */
    u16           __pad0;
    u32           __pad1;
    memory_t      max_pfns;           /*  8 */
    MEMORY_PADDING;
    void         *buffer;             /* 16 */
    MEMORY_PADDING;
    /* OUT variables. */
    memory_t      num_pfns;           /* 24 */
    MEMORY_PADDING;
} PACKED dom0_getmemlist_t; /* 32 bytes */

#define DOM0_SCHEDCTL          6
 /* struct sched_ctl_cmd is from sched-ctl.h   */
typedef struct sched_ctl_cmd dom0_schedctl_t;

#define DOM0_ADJUSTDOM         7
/* struct sched_adjdom_cmd is from sched-ctl.h */
typedef struct sched_adjdom_cmd dom0_adjustdom_t;

#define DOM0_CREATEDOMAIN      8
typedef struct {
    /* IN parameters. */
    memory_t     memory_kb;           /*  0 */
    MEMORY_PADDING;
    u32          cpu;                 /*  8 */
    u32          __pad0;              /* 12 */
    /* IN/OUT parameters. */
    /* If 0, domain is allocated. If non-zero use it unless in use. */
    domid_t      domain;              /* 16 */
    u16          __pad1;
    /* OUT parameters. */
} PACKED dom0_createdomain_t; /* 20 bytes */

#define DOM0_DESTROYDOMAIN     9
typedef struct {
    /* IN variables. */
    domid_t      domain;              /*  0 */
    u16          __pad;
} PACKED dom0_destroydomain_t; /* 4 bytes */

#define DOM0_PAUSEDOMAIN      10
typedef struct {
    /* IN parameters. */
    domid_t domain;                   /*  0 */
    u16     __pad;
} PACKED dom0_pausedomain_t; /* 4 bytes */

#define DOM0_UNPAUSEDOMAIN    11
typedef struct {
    /* IN parameters. */
    domid_t domain;                   /*  0 */
    u16     __pad;
} PACKED dom0_unpausedomain_t; /* 4 bytes */

#define DOM0_GETDOMAININFO    12
typedef struct {
    /* IN variables. */
    domid_t  domain;                  /*  0 */ /* NB. IN/OUT variable. */
    u16     __pad;
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
    u32      flags;                   /*  4 */
    full_execution_context_t *ctxt;   /*  8 */ /* NB. IN/OUT variable. */
    MEMORY_PADDING;
    memory_t tot_pages;               /* 16 */
    MEMORY_PADDING;
    memory_t max_pages;               /* 24 */
    MEMORY_PADDING;
    memory_t shared_info_frame;       /* 32: MFN of shared_info struct */
    MEMORY_PADDING;
    u64      cpu_time;                /* 40 */
} PACKED dom0_getdomaininfo_t; /* 48 bytes */

#define DOM0_BUILDDOMAIN      13
typedef struct {
    /* IN variables. */
    domid_t                 domain;   /*  0 */
    u16                     __pad0;   /*  2 */
    u32                     __pad1;   /*  4 */
    /* IN/OUT parameters */
    full_execution_context_t *ctxt;   /*  8 */
    MEMORY_PADDING;
} PACKED dom0_builddomain_t; /* 16 bytes */

#define DOM0_IOPL             14
typedef struct {
    domid_t domain;                   /*  0 */
    u16     __pad;
    u32     iopl;                     /*  4 */
} PACKED dom0_iopl_t; /* 8 bytes */

#define DOM0_MSR              15
typedef struct {
    /* IN variables. */
    u32 write;                        /*  0 */
    u32 cpu_mask;                     /*  4 */
    u32 msr;                          /*  8 */
    u32 in1;                          /* 12 */
    u32 in2;                          /* 16 */
    /* OUT variables. */
    u32 out1;                         /* 20 */
    u32 out2;                         /* 24 */
} PACKED dom0_msr_t; /* 28 bytes */

#define DOM0_DEBUG            16
typedef struct {
    /* IN variables. */
    domid_t domain;                   /*  0 */
    u8  opcode;                       /*  2 */
    u8  __pad;
    u32 in1;                          /*  4 */
    u32 in2;                          /*  8 */
    u32 in3;                          /* 12 */
    u32 in4;                          /* 16 */
    /* OUT variables. */
    u32 status;                       /* 20 */
    u32 out1;                         /* 24 */
    u32 out2;                         /* 28 */
} PACKED dom0_debug_t; /* 32 bytes */

/*
 * Set clock such that it would read <secs,usecs> after 00:00:00 UTC,
 * 1 January, 1970 if the current system time was <system_time>.
 */
#define DOM0_SETTIME          17
typedef struct {
    /* IN variables. */
    u32 secs;                         /*  0 */
    u32 usecs;                        /*  4 */
    u64 system_time;                  /*  8 */
} PACKED dom0_settime_t; /* 16 bytes */

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
    memory_t pfn;          /*  0: Machine page frame number to query.       */
    MEMORY_PADDING;
    domid_t domain;        /*  8: To which domain does the frame belong?    */
    u16     __pad;
    /* OUT variables. */
    /* Is the page PINNED to a type? */
    u32 type;              /* 12: see above type defs */
} PACKED dom0_getpageframeinfo_t; /* 16 bytes */

/*
 * Read console content from Xen buffer ring.
 */
#define DOM0_READCONSOLE      19
typedef struct {
    memory_t str;                     /*  0 */
    MEMORY_PADDING;
    u32      count;                   /*  8 */
    u32      cmd;                     /* 12 */
} PACKED dom0_readconsole_t; /* 16 bytes */

/* 
 * Pin Domain to a particular CPU  (use -1 to unpin)
 */
#define DOM0_PINCPUDOMAIN     20
typedef struct {
    /* IN variables. */
    domid_t      domain;              /*  0 */
    u16          __pad;
    s32          cpu;                 /*  4: -1 implies unpin */
} PACKED dom0_pincpudomain_t; /* 8 bytes */

/* Get trace buffers machine base address */
#define DOM0_GETTBUFS         21
typedef struct {
    /* OUT variables */
    memory_t mach_addr;   /*  0: location of the trace buffers       */
    MEMORY_PADDING;
    u32      size;        /*  8: size of each trace buffer, in bytes */
} PACKED dom0_gettbufs_t; /* 12 bytes */

/*
 * Get physical information about the host machine
 */
#define DOM0_PHYSINFO         22
typedef struct {
    u32      ht_per_core;             /*  0 */
    u32      cores;                   /*  4 */
    u32      cpu_khz;                 /*  8 */
    u32      __pad;                   /* 12 */
    memory_t total_pages;             /* 16 */
    MEMORY_PADDING;
    memory_t free_pages;              /* 24 */
    MEMORY_PADDING;
} PACKED dom0_physinfo_t; /* 32 bytes */

/* 
 * Allow a domain access to a physical PCI device
 */
#define DOM0_PCIDEV_ACCESS    23
typedef struct {
    /* IN variables. */
    domid_t      domain;              /*  0 */
    u16          __pad;
    u32          bus;                 /*  4 */
    u32          dev;                 /*  8 */
    u32          func;                /* 12 */
    u32          enable;              /* 16 */
} PACKED dom0_pcidev_access_t; /* 20 bytes */

/*
 * Get the ID of the current scheduler.
 */
#define DOM0_SCHED_ID        24
typedef struct {
    /* OUT variable */
    u32 sched_id;                     /*  0 */
} PACKED dom0_sched_id_t; /* 4 bytes */

/* 
 * Control shadow pagetables operation
 */
#define DOM0_SHADOW_CONTROL  25

#define DOM0_SHADOW_CONTROL_OP_OFF         0
#define DOM0_SHADOW_CONTROL_OP_ENABLE_TEST 1
#define DOM0_SHADOW_CONTROL_OP_ENABLE_LOGDIRTY 2
#define DOM0_SHADOW_CONTROL_OP_ENABLE_TRANSLATE 3
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
    domid_t        domain;            /*  0 */
    u16            __pad;
    u32            op;                /*  4 */
    unsigned long *dirty_bitmap;      /*  8: pointer to locked buffer */
    MEMORY_PADDING;
    /* IN/OUT variables. */
    memory_t       pages;  /* 16: size of buffer, updated with actual size */
    MEMORY_PADDING;
    /* OUT variables. */
    dom0_shadow_control_stats_t stats;
} PACKED dom0_shadow_control_t;

#define DOM0_SETDOMAININITIALMEM   27
typedef struct {
    /* IN variables. */
    domid_t     domain;               /*  0 */
    u16         __pad0;
    u32         __pad1;
    memory_t    initial_memkb;        /*  8 */
    MEMORY_PADDING;
} PACKED dom0_setdomaininitialmem_t; /* 16 bytes */

#define DOM0_SETDOMAINMAXMEM   28
typedef struct {
    /* IN variables. */
    domid_t     domain;               /*  0 */
    u16         __pad0;
    u32         __pad1;
    memory_t    max_memkb;            /*  8 */
    MEMORY_PADDING;
} PACKED dom0_setdomainmaxmem_t; /* 16 bytes */

#define DOM0_GETPAGEFRAMEINFO2 29   /* batched interface */
typedef struct {
    /* IN variables. */
    domid_t  domain;                  /*  0 */
    u16      __pad0;
    u32      __pad1;
    memory_t num;                     /*  8 */
    MEMORY_PADDING;
    /* IN/OUT variables. */
    unsigned long *array;             /* 16 */
    MEMORY_PADDING;
} PACKED dom0_getpageframeinfo2_t; /* 24 bytes */

#define DOM0_SETDOMAINVMASSIST   30
typedef struct {
    /* IN variables. */
    domid_t      domain;              /*  0 */
    u16          __pad0;
    u32          cmd;                 /*  4: vm_assist cmd */
    u32          type;                /*  8: vm_assist cmd */
} PACKED dom0_setdomainvmassist_t; /* 12 bytes */

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
    memory_t pfn;                     /*  0 */
    MEMORY_PADDING;
    memory_t nr_pfns;                 /*  8 */
    MEMORY_PADDING;
    u32      type;                    /* 16 */
    u32      __pad0;
    /* OUT variables. */
    u32      handle;                  /* 24 */
    u32      reg;                     /* 28 */
} PACKED dom0_add_memtype_t; /* 32 bytes */

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
    u32      handle;                  /*  0 */
    u32      reg;                     /*  4 */
} PACKED dom0_del_memtype_t; /* 8 bytes */

/* Read current type of an MTRR (x86-specific). */
#define DOM0_READ_MEMTYPE        33
typedef struct {
    /* IN variables. */
    u32      reg;                     /*  0 */
    u32      __pad0;
    /* OUT variables. */
    memory_t pfn;                     /*  8 */
    MEMORY_PADDING;
    memory_t nr_pfns;                 /* 16 */
    MEMORY_PADDING;
    u32      type;                    /* 24 */
    u32      __pad1;
} PACKED dom0_read_memtype_t; /* 32 bytes */

/* Interface for controlling Xen software performance counters. */
#define DOM0_PERFCCONTROL        34
/* Sub-operations: */
#define DOM0_PERFCCONTROL_OP_RESET 1   /* Reset all counters to zero. */
#define DOM0_PERFCCONTROL_OP_QUERY 2   /* Get perfctr information. */
typedef struct {
    u8      name[80];               /*  0: name of perf counter */
    u32     nr_vals;                /* 80: number of values for this counter */
    u32     vals[64];               /* 84: array of values */
} PACKED dom0_perfc_desc_t; /* 340 bytes */
typedef struct {
    /* IN variables. */
    u32            op;                /*  0: DOM0_PERFCCONTROL_OP_??? */
    /* OUT variables. */
    u32            nr_counters;       /*  4: number of counters */
    dom0_perfc_desc_t *desc;          /*  8: counter information (or NULL) */
    MEMORY_PADDING;
} PACKED dom0_perfccontrol_t; /* 16 bytes */

#define DOM0_MICROCODE           35
typedef struct {
    /* IN variables. */
    void   *data;                     /* 0: Pointer to microcode data */
    MEMORY_PADDING;
    u32     length;                   /* 8: Length of microcode data. */
    u32     _pad0;
} PACKED dom0_microcode_t; /* 16 bytes */

typedef struct {
    u32 cmd;                          /* 0 */
    u32 interface_version;            /* 4 */ /* DOM0_INTERFACE_VERSION */
    union {                           /* 8 */
	u32                      dummy[18]; /* 72 bytes */
        dom0_createdomain_t      createdomain;
        dom0_pausedomain_t       pausedomain;
        dom0_unpausedomain_t     unpausedomain;
        dom0_destroydomain_t     destroydomain;
        dom0_getmemlist_t        getmemlist;
        dom0_schedctl_t          schedctl;
        dom0_adjustdom_t         adjustdom;
        dom0_builddomain_t       builddomain;
        dom0_getdomaininfo_t     getdomaininfo;
        dom0_getpageframeinfo_t  getpageframeinfo;
        dom0_iopl_t              iopl;
	dom0_msr_t               msr;
	dom0_debug_t             debug;
	dom0_settime_t           settime;
	dom0_readconsole_t	 readconsole;
	dom0_pincpudomain_t      pincpudomain;
        dom0_gettbufs_t          gettbufs;
        dom0_physinfo_t          physinfo;
        dom0_pcidev_access_t     pcidev_access;
        dom0_sched_id_t          sched_id;
	dom0_shadow_control_t    shadow_control;
	dom0_setdomaininitialmem_t setdomaininitialmem;
	dom0_setdomainmaxmem_t   setdomainmaxmem;
	dom0_getpageframeinfo2_t getpageframeinfo2;
	dom0_setdomainvmassist_t setdomainvmassist;
        dom0_add_memtype_t       add_memtype;
        dom0_del_memtype_t       del_memtype;
        dom0_read_memtype_t      read_memtype;
        dom0_perfccontrol_t      perfccontrol;
        dom0_microcode_t         microcode;
    } PACKED u;
} PACKED dom0_op_t; /* 80 bytes */

#endif /* __XEN_PUBLIC_DOM0_OPS_H__ */
