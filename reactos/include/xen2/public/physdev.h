/* -*-  Mode:C; c-basic-offset:4; tab-width:4 -*-
 ****************************************************************************
 * (c) 2004 - Rolf Neugebauer - Intel Research Cambridge
 * (c) 2004 - Keir Fraser - University of Cambridge
 ****************************************************************************
 * Description: Interface for domains to access physical devices on the PCI bus
 */

#ifndef __XEN_PUBLIC_PHYSDEV_H__
#define __XEN_PUBLIC_PHYSDEV_H__

/* Commands to HYPERVISOR_physdev_op() */
#define PHYSDEVOP_PCI_CFGREG_READ       0
#define PHYSDEVOP_PCI_CFGREG_WRITE      1
#define PHYSDEVOP_PCI_INITIALISE_DEVICE 2
#define PHYSDEVOP_PCI_PROBE_ROOT_BUSES  3
#define PHYSDEVOP_IRQ_UNMASK_NOTIFY     4
#define PHYSDEVOP_IRQ_STATUS_QUERY      5

/* Read from PCI configuration space. */
typedef struct {
    /* IN */
    u32 bus;                          /*  0 */
    u32 dev;                          /*  4 */
    u32 func;                         /*  8 */
    u32 reg;                          /* 12 */
    u32 len;                          /* 16 */
    /* OUT */
    u32 value;                        /* 20 */
} PACKED physdevop_pci_cfgreg_read_t; /* 24 bytes */

/* Write to PCI configuration space. */
typedef struct {
    /* IN */
    u32 bus;                          /*  0 */
    u32 dev;                          /*  4 */
    u32 func;                         /*  8 */
    u32 reg;                          /* 12 */
    u32 len;                          /* 16 */
    u32 value;                        /* 20 */
} PACKED physdevop_pci_cfgreg_write_t; /* 24 bytes */

/* Do final initialisation of a PCI device (e.g., last-moment IRQ routing). */
typedef struct {
    /* IN */
    u32 bus;                          /*  0 */
    u32 dev;                          /*  4 */
    u32 func;                         /*  8 */
} PACKED physdevop_pci_initialise_device_t; /* 12 bytes */

/* Find the root buses for subsequent scanning. */
typedef struct {
    /* OUT */
    u32 busmask[256/32];              /*  0 */
} PACKED physdevop_pci_probe_root_buses_t; /* 32 bytes */

typedef struct {
    /* IN */
    u32 irq;                          /*  0 */
    /* OUT */
/* Need to call PHYSDEVOP_IRQ_UNMASK_NOTIFY when the IRQ has been serviced? */
#define PHYSDEVOP_IRQ_NEEDS_UNMASK_NOTIFY (1<<0)
    u32 flags;                        /*  4 */
} PACKED physdevop_irq_status_query_t; /* 8 bytes */

typedef struct _physdev_op_st 
{
    u32 cmd;                          /*  0 */
    u32 __pad;                        /*  4 */
    union {                           /*  8 */
        physdevop_pci_cfgreg_read_t       pci_cfgreg_read;
        physdevop_pci_cfgreg_write_t      pci_cfgreg_write;
        physdevop_pci_initialise_device_t pci_initialise_device;
        physdevop_pci_probe_root_buses_t  pci_probe_root_buses;
        physdevop_irq_status_query_t      irq_status_query;
        u8                                __dummy[32];
    } PACKED u;
} PACKED physdev_op_t; /* 40 bytes */

#endif /* __XEN_PUBLIC_PHYSDEV_H__ */
