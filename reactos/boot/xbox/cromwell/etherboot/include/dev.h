#ifndef DEV_H
#define DEV_H

#include "isa.h"
#include "pci.h"

/* Need to check the packing of this struct if Etherboot is ported */
struct dev_id
{
	unsigned short	vendor_id;
	unsigned short	device_id;
	unsigned char	bus_type;
#define	PCI_BUS_TYPE	1
#define	ISA_BUS_TYPE	2
};

/* Dont use sizeof, that will include the padding */
#define	DEV_ID_SIZE	8


struct pci_probe_state 
{
#ifdef CONFIG_PCI
	struct pci_device dev;
	int advance;
#else
	int dummy;
#endif
};
struct isa_probe_state
{
#ifdef CONFIG_ISA
	const struct isa_driver *driver;
	int advance;
#else
	int dummy;
#endif
};

union probe_state
{
	struct pci_probe_state pci;
	struct isa_probe_state isa;
};

struct dev
{
	void		(*disable)P((struct dev *));
	struct dev_id	devid;	/* device ID string (sent to DHCP server) */
	int		index;  /* Index of next device on this controller to probe */
	int		type;		/* Type of device I am probing for */
	int		how_probe;	/* First, next or awake */
	int 		to_probe;	/* Flavor of device I am probing */
	int		failsafe;	/* Failsafe probe requested */
	int		type_index;	/* Index of this device (within type) */
#define	PROBE_NONE 0
#define PROBE_PCI  1
#define PROBE_ISA  2
	union probe_state state;
};


#define NIC_DRIVER    0
#define DISK_DRIVER   1
#define FLOPPY_DRIVER 2

#define BRIDGE_DRIVER 1000

#define PROBE_FIRST  (-1)
#define PROBE_NEXT   0
#define PROBE_AWAKE  1		/* After calling disable bring up the same device */

/* The probe result codes are selected
 * to allow them to be fed back into the probe
 * routine and get a successful probe.
 */
#define PROBE_FAILED PROBE_FIRST
#define PROBE_WORKED  PROBE_NEXT

extern int probe(struct dev *dev);
extern void disable(struct dev *dev);

/* Boot option values
 * option & BOOT_TYPE_MASK should equal a driver for probing
 */

#define BOOT_NIC      0x0	/* Boot from a nic */
#define BOOT_DISK     0x1	/* Boot from disk */
#define BOOT_FLOPPY   0x2	/* Boot from a floppy */

#define BOOT_NOTHING  0x3	/* Last valid boot choice */

/* Do magic failsafe boot processing */
#define BOOT_FAILSAFE 0x8

#define BOOT_BITS       4
#define BOOT_MASK      ((1 << (BOOT_BITS)) - 1)
#define BOOT_TYPE_MASK ((1 << (BOOT_BITS - 1)) - 1)

#define MAX_BOOT_ENTRIES 3

#define	BOOT_ALL_VALUE	(1<<BOOT_FIRST|1<<BOOT_SECOND|1<<BOOT_THIRD)

/* These could be customised for different languages perhaps */
#if	BOOT_ALL_VALUE&(1<<BOOT_DISK)
#define	BOOT_DISK_PROMPT	"(D)isk "
#else
#define	BOOT_DISK_PROMPT
#endif

#if	BOOT_ALL_VALUE&(1<<BOOT_FLOPPY)
#define	BOOT_FLOPPY_PROMPT	"(F)loppy "
#else
#define	BOOT_FLOPPY_PROMPT
#endif

#define	ASK_PROMPT	\
	"Boot from (N)etwork " BOOT_DISK_PROMPT BOOT_FLOPPY_PROMPT "or (Q)uit? "

#define	ANS_NETWORK	'N'
#define ANS_DISK	'D'
#define ANS_FLOPPY	'F'
#define	ANS_QUIT	'Q'
#define ANS_DEFAULT	'\n'

#endif /* DEV_H */
