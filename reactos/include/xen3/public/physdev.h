
#ifndef __XEN_PUBLIC_PHYSDEV_H__
#define __XEN_PUBLIC_PHYSDEV_H__

/* Commands to HYPERVISOR_physdev_op() */
#define PHYSDEVOP_IRQ_UNMASK_NOTIFY     4
#define PHYSDEVOP_IRQ_STATUS_QUERY      5
#define PHYSDEVOP_SET_IOPL              6
#define PHYSDEVOP_SET_IOBITMAP          7
#define PHYSDEVOP_APIC_READ             8
#define PHYSDEVOP_APIC_WRITE            9
#define PHYSDEVOP_ASSIGN_VECTOR         10

typedef struct {
    /* IN */
    u32 irq;                          /*  0 */
    /* OUT */
/* Need to call PHYSDEVOP_IRQ_UNMASK_NOTIFY when the IRQ has been serviced? */
#define PHYSDEVOP_IRQ_NEEDS_UNMASK_NOTIFY (1<<0)
    u32 flags;                        /*  4 */
} PACKED physdevop_irq_status_query_t; /* 8 bytes */

typedef struct {
    /* IN */
    u32 iopl;                         /*  0 */
} PACKED physdevop_set_iopl_t; /* 4 bytes */

typedef struct {
    /* IN */
    memory_t bitmap;                  /*  0 */
    MEMORY_PADDING;
    u32      nr_ports;                /*  8 */
    u32      __pad0;                  /* 12 */
} PACKED physdevop_set_iobitmap_t; /* 16 bytes */

typedef struct {
    /* IN */
    u32 apic;                          /*  0 */
    u32 offset;
    /* IN or OUT */
    u32 value;
} PACKED physdevop_apic_t; 

typedef struct {
    /* IN */
    u32 irq;                          /*  0 */
    /* OUT */
    u32 vector;
} PACKED physdevop_irq_t; 

typedef struct _physdev_op_st 
{
    u32 cmd;                          /*  0 */
    u32 __pad;                        /*  4 */
    union {                           /*  8 */
        physdevop_irq_status_query_t      irq_status_query;
        physdevop_set_iopl_t              set_iopl;
        physdevop_set_iobitmap_t          set_iobitmap;
        physdevop_apic_t                  apic_op;
        physdevop_irq_t                   irq_op;
        u8                                __dummy[32];
    } PACKED u;
} PACKED physdev_op_t; /* 40 bytes */

#endif /* __XEN_PUBLIC_PHYSDEV_H__ */

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
