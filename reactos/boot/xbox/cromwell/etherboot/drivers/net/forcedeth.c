/**************************************************************************
*    forcedeth.c -- Etherboot device driver for the NVIDIA nForce 
*			media access controllers.
*
* Note: This driver is based on the Linux driver that was based on
*      a cleanroom reimplementation which was based on reverse
*      engineered documentation written by Carl-Daniel Hailfinger
*      and Andrew de Quincey. It's neither supported nor endorsed
*      by NVIDIA Corp. Use at your own risk.
*
*    Written 2004 by Timothy Legge <tlegge@rogers.com>
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*    Portions of this code based on:
*		forcedeth: Ethernet driver for NVIDIA nForce media access controllers:
*
*	(C) 2003 Manfred Spraul
*		See Linux Driver for full information
*	
*	Linux Driver Version 0.22, 19 Jan 2004
* 
* 
*    REVISION HISTORY:
*    ================
*    v1.0	01-31-2004	timlegge	Initial port of Linux driver
*    v1.1	02-03-2004	timlegge	Large Clean up, first release 
*    
*    Indent Options: indent -kr -i8
***************************************************************************/

/* to get some global routines like printf */
#include "etherboot.h"
/* to get the interface to the body of the program */
#include "nic.h"
/* to get the PCI support functions, if this is a PCI NIC */
#include "pci.h"
/* Include timer support functions */
#include "timer.h"

#define drv_version "v1.1"
#define drv_date "02-03-2004"

//#define TFTM_DEBUG
#ifdef TFTM_DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;

/* Condensed operations for readability. */
#define virt_to_le32desc(addr)  cpu_to_le32(virt_to_bus(addr))
#define le32desc_to_virt(addr)  bus_to_virt(le32_to_cpu(addr))

unsigned long BASE;
/* NIC specific static variables go here */


/*
 * Hardware access:
 */

#define DEV_NEED_LASTPACKET1	0x0001
#define DEV_IRQMASK_1		0x0002
#define DEV_IRQMASK_2		0x0004
#define DEV_NEED_TIMERIRQ	0x0008

enum {
	NvRegIrqStatus = 0x000,
#define NVREG_IRQSTAT_MIIEVENT	0040
#define NVREG_IRQSTAT_MASK		0x1ff
	NvRegIrqMask = 0x004,
#define NVREG_IRQ_RX			0x0002
#define NVREG_IRQ_RX_NOBUF		0x0004
#define NVREG_IRQ_TX_ERR		0x0008
#define NVREG_IRQ_TX2			0x0010
#define NVREG_IRQ_TIMER			0x0020
#define NVREG_IRQ_LINK			0x0040
#define NVREG_IRQ_TX1			0x0100
#define NVREG_IRQMASK_WANTED_1		0x005f
#define NVREG_IRQMASK_WANTED_2		0x0147
#define NVREG_IRQ_UNKNOWN		(~(NVREG_IRQ_RX|NVREG_IRQ_RX_NOBUF|NVREG_IRQ_TX_ERR|NVREG_IRQ_TX2|NVREG_IRQ_TIMER|NVREG_IRQ_LINK|NVREG_IRQ_TX1))

	NvRegUnknownSetupReg6 = 0x008,
#define NVREG_UNKSETUP6_VAL		3

/*
 * NVREG_POLL_DEFAULT is the interval length of the timer source on the nic
 * NVREG_POLL_DEFAULT=97 would result in an interval length of 1 ms
 */
	NvRegPollingInterval = 0x00c,
#define NVREG_POLL_DEFAULT	970
	NvRegMisc1 = 0x080,
#define NVREG_MISC1_HD		0x02
#define NVREG_MISC1_FORCE	0x3b0f3c

	NvRegTransmitterControl = 0x084,
#define NVREG_XMITCTL_START	0x01
	NvRegTransmitterStatus = 0x088,
#define NVREG_XMITSTAT_BUSY	0x01

	NvRegPacketFilterFlags = 0x8c,
#define NVREG_PFF_ALWAYS	0x7F0008
#define NVREG_PFF_PROMISC	0x80
#define NVREG_PFF_MYADDR	0x20

	NvRegOffloadConfig = 0x90,
#define NVREG_OFFLOAD_HOMEPHY	0x601
#define NVREG_OFFLOAD_NORMAL	0x5ee
	NvRegReceiverControl = 0x094,
#define NVREG_RCVCTL_START	0x01
	NvRegReceiverStatus = 0x98,
#define NVREG_RCVSTAT_BUSY	0x01

	NvRegRandomSeed = 0x9c,
#define NVREG_RNDSEED_MASK	0x00ff
#define NVREG_RNDSEED_FORCE	0x7f00

	NvRegUnknownSetupReg1 = 0xA0,
#define NVREG_UNKSETUP1_VAL	0x16070f
	NvRegUnknownSetupReg2 = 0xA4,
#define NVREG_UNKSETUP2_VAL	0x16
	NvRegMacAddrA = 0xA8,
	NvRegMacAddrB = 0xAC,
	NvRegMulticastAddrA = 0xB0,
#define NVREG_MCASTADDRA_FORCE	0x01
	NvRegMulticastAddrB = 0xB4,
	NvRegMulticastMaskA = 0xB8,
	NvRegMulticastMaskB = 0xBC,

	NvRegTxRingPhysAddr = 0x100,
	NvRegRxRingPhysAddr = 0x104,
	NvRegRingSizes = 0x108,
#define NVREG_RINGSZ_TXSHIFT 0
#define NVREG_RINGSZ_RXSHIFT 16
	NvRegUnknownTransmitterReg = 0x10c,
	NvRegLinkSpeed = 0x110,
#define NVREG_LINKSPEED_FORCE 0x10000
#define NVREG_LINKSPEED_10	10
#define NVREG_LINKSPEED_100	100
#define NVREG_LINKSPEED_1000	1000
	NvRegUnknownSetupReg5 = 0x130,
#define NVREG_UNKSETUP5_BIT31	(1<<31)
	NvRegUnknownSetupReg3 = 0x134,
#define NVREG_UNKSETUP3_VAL1	0x200010
	NvRegTxRxControl = 0x144,
#define NVREG_TXRXCTL_KICK	0x0001
#define NVREG_TXRXCTL_BIT1	0x0002
#define NVREG_TXRXCTL_BIT2	0x0004
#define NVREG_TXRXCTL_IDLE	0x0008
#define NVREG_TXRXCTL_RESET	0x0010
	NvRegMIIStatus = 0x180,
#define NVREG_MIISTAT_ERROR		0x0001
#define NVREG_MIISTAT_LINKCHANGE	0x0008
#define NVREG_MIISTAT_MASK		0x000f
#define NVREG_MIISTAT_MASK2		0x000f
	NvRegUnknownSetupReg4 = 0x184,
#define NVREG_UNKSETUP4_VAL	8

	NvRegAdapterControl = 0x188,
#define NVREG_ADAPTCTL_START	0x02
#define NVREG_ADAPTCTL_LINKUP	0x04
#define NVREG_ADAPTCTL_PHYVALID	0x4000
#define NVREG_ADAPTCTL_RUNNING	0x100000
#define NVREG_ADAPTCTL_PHYSHIFT	24
	NvRegMIISpeed = 0x18c,
#define NVREG_MIISPEED_BIT8	(1<<8)
#define NVREG_MIIDELAY	5
	NvRegMIIControl = 0x190,
#define NVREG_MIICTL_INUSE	0x10000
#define NVREG_MIICTL_WRITE	0x08000
#define NVREG_MIICTL_ADDRSHIFT	5
	NvRegMIIData = 0x194,
	NvRegWakeUpFlags = 0x200,
#define NVREG_WAKEUPFLAGS_VAL		0x7770
#define NVREG_WAKEUPFLAGS_BUSYSHIFT	24
#define NVREG_WAKEUPFLAGS_ENABLESHIFT	16
#define NVREG_WAKEUPFLAGS_D3SHIFT	12
#define NVREG_WAKEUPFLAGS_D2SHIFT	8
#define NVREG_WAKEUPFLAGS_D1SHIFT	4
#define NVREG_WAKEUPFLAGS_D0SHIFT	0
#define NVREG_WAKEUPFLAGS_ACCEPT_MAGPAT		0x01
#define NVREG_WAKEUPFLAGS_ACCEPT_WAKEUPPAT	0x02
#define NVREG_WAKEUPFLAGS_ACCEPT_LINKCHANGE	0x04

	NvRegPatternCRC = 0x204,
	NvRegPatternMask = 0x208,
	NvRegPowerCap = 0x268,
#define NVREG_POWERCAP_D3SUPP	(1<<30)
#define NVREG_POWERCAP_D2SUPP	(1<<26)
#define NVREG_POWERCAP_D1SUPP	(1<<25)
	NvRegPowerState = 0x26c,
#define NVREG_POWERSTATE_POWEREDUP	0x8000
#define NVREG_POWERSTATE_VALID		0x0100
#define NVREG_POWERSTATE_MASK		0x0003
#define NVREG_POWERSTATE_D0		0x0000
#define NVREG_POWERSTATE_D1		0x0001
#define NVREG_POWERSTATE_D2		0x0002
#define NVREG_POWERSTATE_D3		0x0003
};



#define NV_TX_LASTPACKET	(1<<0)
#define NV_TX_RETRYERROR	(1<<3)
#define NV_TX_LASTPACKET1	(1<<8)
#define NV_TX_DEFERRED		(1<<10)
#define NV_TX_CARRIERLOST	(1<<11)
#define NV_TX_LATECOLLISION	(1<<12)
#define NV_TX_UNDERFLOW		(1<<13)
#define NV_TX_ERROR		(1<<14)
#define NV_TX_VALID		(1<<15)

#define NV_RX_DESCRIPTORVALID	(1<<0)
#define NV_RX_MISSEDFRAME	(1<<1)
#define NV_RX_SUBSTRACT1	(1<<3)
#define NV_RX_ERROR1		(1<<7)
#define NV_RX_ERROR2		(1<<8)
#define NV_RX_ERROR3		(1<<9)
#define NV_RX_ERROR4		(1<<10)
#define NV_RX_CRCERR		(1<<11)
#define NV_RX_OVERFLOW		(1<<12)
#define NV_RX_FRAMINGERR	(1<<13)
#define NV_RX_ERROR		(1<<14)
#define NV_RX_AVAIL		(1<<15)

/* Miscelaneous hardware related defines: */
#define NV_PCI_REGSZ		0x270

/* various timeout delays: all in usec */
#define NV_TXRX_RESET_DELAY	4
#define NV_TXSTOP_DELAY1	10
#define NV_TXSTOP_DELAY1MAX	500000
#define NV_TXSTOP_DELAY2	100
#define NV_RXSTOP_DELAY1	10
#define NV_RXSTOP_DELAY1MAX	500000
#define NV_RXSTOP_DELAY2	100
#define NV_SETUP5_DELAY		5
#define NV_SETUP5_DELAYMAX	50000
#define NV_POWERUP_DELAY	5
#define NV_POWERUP_DELAYMAX	5000
#define NV_MIIBUSY_DELAY	50
#define NV_MIIPHY_DELAY	10
#define NV_MIIPHY_DELAYMAX	10000

#define NV_WAKEUPPATTERNS	5
#define NV_WAKEUPMASKENTRIES	4

/* General driver defaults */
#define NV_WATCHDOG_TIMEO	(2*HZ)
#define DEFAULT_MTU		1500	/* also maximum supported, at least for now */

#define RX_RING		4
#define TX_RING		2
/* limited to 1 packet until we understand NV_TX_LASTPACKET */
#define TX_LIMIT_STOP	10
#define TX_LIMIT_START	5

/* rx/tx mac addr + type + vlan + align + slack*/
#define RX_NIC_BUFSIZE		(DEFAULT_MTU + 64)
/* even more slack */
#define RX_ALLOC_BUFSIZE	(DEFAULT_MTU + 128)

#define OOM_REFILL	(1+HZ/20)
#define POLL_WAIT	(1+HZ/100)

struct ring_desc {
	u32 PacketBuffer;
	u16 Length;
	u16 Flags;
};


/* Define the TX Descriptor */
static struct ring_desc tx_ring[TX_RING];

/* Create a static buffer of size RX_BUF_SZ for each
TX Descriptor.  All descriptors point to a
part of this buffer */
static unsigned char txb[TX_RING * RX_NIC_BUFSIZE];

/* Define the TX Descriptor */
static struct ring_desc rx_ring[RX_RING];

/* Create a static buffer of size RX_BUF_SZ for each
RX Descriptor   All descriptors point to a
part of this buffer */
static unsigned char rxb[RX_RING * RX_NIC_BUFSIZE];

/* Private Storage for the NIC */
struct forcedeth_private {
	/* General data:
	 * Locking: spin_lock(&np->lock); */
	int in_shutdown;
	u32 linkspeed;
	int duplex;
	int phyaddr;

	/* General data: RO fields */
	u8 *ring_addr;
	u32 orig_mac[2];
	u32 irqmask;
	/* rx specific fields.
	 * Locking: Within irq hander or disable_irq+spin_lock(&np->lock);
	 */
	struct ring_desc *rx_ring;
	unsigned int cur_rx, refill_rx;
	struct sk_buff *rx_skbuff[RX_RING];
	u32 rx_dma[RX_RING];
	unsigned int rx_buf_sz;

	/*
	 * tx specific fields.
	 */
	struct ring_desc *tx_ring;
	unsigned int next_tx, nic_tx;
	struct sk_buff *tx_skbuff[TX_RING];
	u32 tx_dma[TX_RING];
	u16 tx_flags;
} npx;

static struct forcedeth_private *np;

static inline void pci_push(u8 * base)
{
	/* force out pending posted writes */
	readl(base);
}
static int reg_delay(int offset, u32 mask,
		     u32 target, int delay, int delaymax, const char *msg)
{
	u8 *base = (u8 *) BASE;

	pci_push(base);
	do {
		udelay(delay);
		delaymax -= delay;
		if (delaymax < 0) {
			if (msg)
				printf(msg);
			return 1;
		}
	} while ((readl(base + offset) & mask) != target);
	return 0;
}

#define MII_READ	(-1)
#define MII_PHYSID1         0x02	/* PHYS ID 1                   */
#define MII_PHYSID2         0x03	/* PHYS ID 2                   */
#define MII_BMCR            0x00	/* Basic mode control register */
#define MII_BMSR            0x01	/* Basic mode status register  */
#define MII_ADVERTISE       0x04	/* Advertisement control reg   */
#define MII_LPA             0x05	/* Link partner ability reg    */

#define BMSR_ANEGCOMPLETE       0x0020	/* Auto-negotiation complete   */

/* Link partner ability register. */
#define LPA_SLCT                0x001f	/* Same as advertise selector  */
#define LPA_10HALF              0x0020	/* Can do 10mbps half-duplex   */
#define LPA_10FULL              0x0040	/* Can do 10mbps full-duplex   */
#define LPA_100HALF             0x0080	/* Can do 100mbps half-duplex  */
#define LPA_100FULL             0x0100	/* Can do 100mbps full-duplex  */
#define LPA_100BASE4            0x0200	/* Can do 100mbps 4k packets   */
#define LPA_RESV                0x1c00	/* Unused...                   */
#define LPA_RFAULT              0x2000	/* Link partner faulted        */
#define LPA_LPACK               0x4000	/* Link partner acked us       */
#define LPA_NPAGE               0x8000	/* Next page bit               */

/* mii_rw: read/write a register on the PHY.
 *
 * Caller must guarantee serialization
 */
static int mii_rw(struct nic *nic __unused, int addr, int miireg,
		  int value)
{
	u8 *base = (u8 *) BASE;
	int was_running;
	u32 reg;
	int retval;

	writel(NVREG_MIISTAT_MASK, base + NvRegMIIStatus);
	was_running = 0;
	reg = readl(base + NvRegAdapterControl);
	if (reg & NVREG_ADAPTCTL_RUNNING) {
		was_running = 1;
		writel(reg & ~NVREG_ADAPTCTL_RUNNING,
		       base + NvRegAdapterControl);
	}
	reg = readl(base + NvRegMIIControl);
	if (reg & NVREG_MIICTL_INUSE) {
		writel(NVREG_MIICTL_INUSE, base + NvRegMIIControl);
		udelay(NV_MIIBUSY_DELAY);
	}

	reg =
	    NVREG_MIICTL_INUSE | (addr << NVREG_MIICTL_ADDRSHIFT) | miireg;
	if (value != MII_READ) {
		writel(value, base + NvRegMIIData);
		reg |= NVREG_MIICTL_WRITE;
	}
	writel(reg, base + NvRegMIIControl);

	if (reg_delay(NvRegMIIControl, NVREG_MIICTL_INUSE, 0,
		      NV_MIIPHY_DELAY, NV_MIIPHY_DELAYMAX, NULL)) {
		dprintf(("mii_rw of reg %d at PHY %d timed out.\n",
			 miireg, addr));
		retval = -1;
	} else if (value != MII_READ) {
		/* it was a write operation - fewer failures are detectable */
		dprintf(("mii_rw wrote 0x%x to reg %d at PHY %d\n",
			 value, miireg, addr));
		retval = 0;
	} else if (readl(base + NvRegMIIStatus) & NVREG_MIISTAT_ERROR) {
		dprintf(("mii_rw of reg %d at PHY %d failed.\n",
			 miireg, addr));
		retval = -1;
	} else {
		/* FIXME: why is that required? */
		udelay(50);
		retval = readl(base + NvRegMIIData);
		dprintf(("mii_rw read from reg %d at PHY %d: 0x%x.\n",
			 miireg, addr, retval));
	}
	if (was_running) {
		reg = readl(base + NvRegAdapterControl);
		writel(reg | NVREG_ADAPTCTL_RUNNING,
		       base + NvRegAdapterControl);
	}
	return retval;
}

static void start_rx(struct nic *nic __unused)
{
	u8 *base = (u8 *) BASE;

	dprintf(("start_rx\n"));
	/* Already running? Stop it. */
	if (readl(base + NvRegReceiverControl) & NVREG_RCVCTL_START) {
		writel(0, base + NvRegReceiverControl);
		pci_push(base);
	}
	writel(np->linkspeed, base + NvRegLinkSpeed);
	pci_push(base);
	writel(NVREG_RCVCTL_START, base + NvRegReceiverControl);
	pci_push(base);
}

static void stop_rx(void)
{
	u8 *base = (u8 *) BASE;

	dprintf(("stop_rx\n"));
	writel(0, base + NvRegReceiverControl);
	reg_delay(NvRegReceiverStatus, NVREG_RCVSTAT_BUSY, 0,
		  NV_RXSTOP_DELAY1, NV_RXSTOP_DELAY1MAX,
		  "stop_rx: ReceiverStatus remained busy");

	udelay(NV_RXSTOP_DELAY2);
	writel(0, base + NvRegLinkSpeed);
}

static void start_tx(struct nic *nic __unused)
{
	u8 *base = (u8 *) BASE;

	dprintf(("start_tx\n"));
	writel(NVREG_XMITCTL_START, base + NvRegTransmitterControl);
	pci_push(base);
}

static void stop_tx(void)
{
	u8 *base = (u8 *) BASE;

	dprintf(("stop_tx\n"));
	writel(0, base + NvRegTransmitterControl);
	reg_delay(NvRegTransmitterStatus, NVREG_XMITSTAT_BUSY, 0,
		  NV_TXSTOP_DELAY1, NV_TXSTOP_DELAY1MAX,
		  "stop_tx: TransmitterStatus remained busy");

	udelay(NV_TXSTOP_DELAY2);
	writel(0, base + NvRegUnknownTransmitterReg);
}


static void txrx_reset(struct nic *nic __unused)
{
	u8 *base = (u8 *) BASE;

	dprintf(("txrx_reset\n"));
	writel(NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET,
	       base + NvRegTxRxControl);
	pci_push(base);
	udelay(NV_TXRX_RESET_DELAY);
	writel(NVREG_TXRXCTL_BIT2, base + NvRegTxRxControl);
	pci_push(base);
}

/*
 * alloc_rx: fill rx ring entries.
 * Return 1 if the allocations for the skbs failed and the
 * rx engine is without Available descriptors
 */
static int alloc_rx(struct nic *nic __unused)
{
	unsigned int refill_rx = np->refill_rx;
	int i;
	//while (np->cur_rx != refill_rx) {
	for (i = 0; i < RX_RING; i++) {
		//int nr = refill_rx % RX_RING;
		rx_ring[i].PacketBuffer =
		    virt_to_le32desc(&rxb[i * RX_NIC_BUFSIZE]);
		rx_ring[i].Length = cpu_to_le16(RX_NIC_BUFSIZE);
		wmb();
		rx_ring[i].Flags = cpu_to_le16(NV_RX_AVAIL);
		/*      printf("alloc_rx: Packet  %d marked as Available\n",
		   refill_rx); */
		refill_rx++;
	}
	np->refill_rx = refill_rx;
	if (np->cur_rx - refill_rx == RX_RING)
		return 1;
	return 0;
}

static int update_linkspeed(struct nic *nic)
{
	int adv, lpa, newdup;
	u32 newls;
	adv = mii_rw(nic, np->phyaddr, MII_ADVERTISE, MII_READ);
	lpa = mii_rw(nic, np->phyaddr, MII_LPA, MII_READ);
	dprintf(("update_linkspeed: PHY advertises 0x%hX, lpa 0x%hX.\n",
		 adv, lpa));

	/* FIXME: handle parallel detection properly, handle gigabit ethernet */
	lpa = lpa & adv;
	if (lpa & LPA_100FULL) {
		newls = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_100;
		newdup = 1;
	} else if (lpa & LPA_100HALF) {
		newls = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_100;
		newdup = 0;
	} else if (lpa & LPA_10FULL) {
		newls = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_10;
		newdup = 1;
	} else if (lpa & LPA_10HALF) {
		newls = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_10;
		newdup = 0;
	} else {
		printf("bad ability %hX - falling back to 10HD.\n", lpa);
		newls = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_10;
		newdup = 0;
	}
	if (np->duplex != newdup || np->linkspeed != newls) {
		np->duplex = newdup;
		np->linkspeed = newls;
		return 1;
	}
	return 0;
}



static int init_ring(struct nic *nic)
{
	int i;

	np->next_tx = np->nic_tx = 0;
	for (i = 0; i < TX_RING; i++) {
		tx_ring[i].Flags = 0;
	}

	np->cur_rx = 0;
	np->refill_rx = 0;
	for (i = 0; i < RX_RING; i++) {
		rx_ring[i].Flags = 0;
	}
	return alloc_rx(nic);
}

static void set_multicast(struct nic *nic)
{

	u8 *base = (u8 *) BASE;
	u32 addr[2];
	u32 mask[2];
	u32 pff;
	u32 alwaysOff[2];
	u32 alwaysOn[2];

	memset(addr, 0, sizeof(addr));
	memset(mask, 0, sizeof(mask));

	pff = NVREG_PFF_MYADDR;

	alwaysOn[0] = alwaysOn[1] = alwaysOff[0] = alwaysOff[1] = 0;

	addr[0] = alwaysOn[0];
	addr[1] = alwaysOn[1];
	mask[0] = alwaysOn[0] | alwaysOff[0];
	mask[1] = alwaysOn[1] | alwaysOff[1];

	addr[0] |= NVREG_MCASTADDRA_FORCE;
	pff |= NVREG_PFF_ALWAYS;
	stop_rx();
	writel(addr[0], base + NvRegMulticastAddrA);
	writel(addr[1], base + NvRegMulticastAddrB);
	writel(mask[0], base + NvRegMulticastMaskA);
	writel(mask[1], base + NvRegMulticastMaskB);
	writel(pff, base + NvRegPacketFilterFlags);
	start_rx(nic);
}

/**************************************************************************
RESET - Reset the NIC to prepare for use
***************************************************************************/
static int forcedeth_reset(struct nic *nic)
{
	u8 *base = (u8 *) BASE;
	int ret, oom, i;
	ret = 0;
	dprintf(("forcedeth: open\n"));

	/* 1) erase previous misconfiguration */
	/* 4.1-1: stop adapter: ignored, 4.3 seems to be overkill */
	writel(NVREG_MCASTADDRA_FORCE, base + NvRegMulticastAddrA);
	writel(0, base + NvRegMulticastAddrB);
	writel(0, base + NvRegMulticastMaskA);
	writel(0, base + NvRegMulticastMaskB);
	writel(0, base + NvRegPacketFilterFlags);
	writel(0, base + NvRegAdapterControl);
	writel(0, base + NvRegLinkSpeed);
	writel(0, base + NvRegUnknownTransmitterReg);
	txrx_reset(nic);
	writel(0, base + NvRegUnknownSetupReg6);

	/* 2) initialize descriptor rings */
	np->in_shutdown = 0;
	oom = init_ring(nic);

	/* 3) set mac address */
	{
		u32 mac[2];

		mac[0] =
		    (nic->node_addr[0] << 0) + (nic->node_addr[1] << 8) +
		    (nic->node_addr[2] << 16) + (nic->node_addr[3] << 24);
		mac[1] =
		    (nic->node_addr[4] << 0) + (nic->node_addr[5] << 8);

		writel(mac[0], base + NvRegMacAddrA);
		writel(mac[1], base + NvRegMacAddrB);
	}

	/* 4) continue setup */
	np->linkspeed = NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_10;
	np->duplex = 0;
	writel(NVREG_UNKSETUP3_VAL1, base + NvRegUnknownSetupReg3);
	writel(0, base + NvRegTxRxControl);
	pci_push(base);
	writel(NVREG_TXRXCTL_BIT1, base + NvRegTxRxControl);

	reg_delay(NvRegUnknownSetupReg5, NVREG_UNKSETUP5_BIT31,
		  NVREG_UNKSETUP5_BIT31, NV_SETUP5_DELAY,
		  NV_SETUP5_DELAYMAX,
		  "open: SetupReg5, Bit 31 remained off\n");
	writel(0, base + NvRegUnknownSetupReg4);

	/* 5) Find a suitable PHY */
	writel(NVREG_MIISPEED_BIT8 | NVREG_MIIDELAY, base + NvRegMIISpeed);
	for (i = 1; i < 32; i++) {
		int id1, id2;

		id1 = mii_rw(nic, i, MII_PHYSID1, MII_READ);
		if (id1 < 0)
			continue;
		id2 = mii_rw(nic, i, MII_PHYSID2, MII_READ);
		if (id2 < 0)
			continue;
		dprintf(("open: Found PHY %04x:%04x at address %d.\n",
			 id1, id2, i));
		np->phyaddr = i;

		update_linkspeed(nic);

		break;
	}
	if (i == 32) {
		printf("open: failing due to lack of suitable PHY.\n");
		ret = -1;
		goto out_drain;
	}

	printf("%d-Mbs Link, %s-Duplex\n",
	       np->linkspeed & NVREG_LINKSPEED_10 ? 10 : 100,
	       np->duplex ? "Full" : "Half");
	/* 6) continue setup */
	writel(NVREG_MISC1_FORCE | (np->duplex ? 0 : NVREG_MISC1_HD),
	       base + NvRegMisc1);
	writel(readl(base + NvRegTransmitterStatus),
	       base + NvRegTransmitterStatus);
	writel(NVREG_PFF_ALWAYS, base + NvRegPacketFilterFlags);
	writel(NVREG_OFFLOAD_NORMAL, base + NvRegOffloadConfig);

	writel(readl(base + NvRegReceiverStatus),
	       base + NvRegReceiverStatus);

	/* FIXME: I cheated and used the calculator to get a random number */
	i = 75963081;
	writel(NVREG_RNDSEED_FORCE | (i & NVREG_RNDSEED_MASK),
	       base + NvRegRandomSeed);
	writel(NVREG_UNKSETUP1_VAL, base + NvRegUnknownSetupReg1);
	writel(NVREG_UNKSETUP2_VAL, base + NvRegUnknownSetupReg2);
	writel(NVREG_POLL_DEFAULT, base + NvRegPollingInterval);
	writel(NVREG_UNKSETUP6_VAL, base + NvRegUnknownSetupReg6);
	writel((np->
		phyaddr << NVREG_ADAPTCTL_PHYSHIFT) |
	       NVREG_ADAPTCTL_PHYVALID, base + NvRegAdapterControl);
	writel(NVREG_UNKSETUP4_VAL, base + NvRegUnknownSetupReg4);
	writel(NVREG_WAKEUPFLAGS_VAL, base + NvRegWakeUpFlags);

	/* 7) start packet processing */
	writel((u32) virt_to_le32desc(&rx_ring[0]),
	       base + NvRegRxRingPhysAddr);
	writel((u32) virt_to_le32desc(&tx_ring[0]),
	       base + NvRegTxRingPhysAddr);


	writel(((RX_RING - 1) << NVREG_RINGSZ_RXSHIFT) +
	       ((TX_RING - 1) << NVREG_RINGSZ_TXSHIFT),
	       base + NvRegRingSizes);

	i = readl(base + NvRegPowerState);
	if ((i & NVREG_POWERSTATE_POWEREDUP) == 0) {
		writel(NVREG_POWERSTATE_POWEREDUP | i,
		       base + NvRegPowerState);
	}
	pci_push(base);
	udelay(10);
	writel(readl(base + NvRegPowerState) | NVREG_POWERSTATE_VALID,
	       base + NvRegPowerState);
	writel(NVREG_ADAPTCTL_RUNNING, base + NvRegAdapterControl);

	writel(0, base + NvRegIrqMask);
	pci_push(base);
	writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);
	pci_push(base);
	writel(NVREG_MIISTAT_MASK2, base + NvRegMIIStatus);
	writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);
	pci_push(base);
/*
	writel(np->irqmask, base + NvRegIrqMask);
*/
	writel(NVREG_MCASTADDRA_FORCE, base + NvRegMulticastAddrA);
	writel(0, base + NvRegMulticastAddrB);
	writel(0, base + NvRegMulticastMaskA);
	writel(0, base + NvRegMulticastMaskB);
	writel(NVREG_PFF_ALWAYS | NVREG_PFF_MYADDR,
	       base + NvRegPacketFilterFlags);

	set_multicast(nic);
	//start_rx(nic);
	start_tx(nic);

	if (!
	    (mii_rw(nic, np->phyaddr, MII_BMSR, MII_READ) &
	     BMSR_ANEGCOMPLETE)) {
		printf("no link during initialization.\n");
	}

	udelay(10000);
      out_drain:
	return ret;
}

//extern void hex_dump(const char *data, const unsigned int len);

/**************************************************************************
POLL - Wait for a frame
***************************************************************************/
static int forcedeth_poll(struct nic *nic)
{
	/* return true if there's an ethernet packet ready to read */
	/* nic->packet should contain data on return */
	/* nic->packetlen should contain length of data */

	struct ring_desc *prd;
	int len;
	int i;

	i = np->cur_rx % RX_RING;
	prd = &rx_ring[i];

	if (prd->Flags & cpu_to_le16(NV_RX_DESCRIPTORVALID)) {
		/* got a valid packet - forward it to the network core */
		len = cpu_to_le16(prd->Length);
		nic->packetlen = len;
		//hex_dump(rxb + (i * RX_NIC_BUFSIZE), len);
		memcpy(nic->packet, rxb +
		       (i * RX_NIC_BUFSIZE), nic->packetlen);

		wmb();
		np->cur_rx++;
		alloc_rx(nic);
		return 1;
	}
	return 0;		/* initially as this is called to flush the input */
}


/**************************************************************************
TRANSMIT - Transmit a frame
***************************************************************************/
static void forcedeth_transmit(struct nic *nic, const char *d,	/* Destination */
			       unsigned int t,	/* Type */
			       unsigned int s,	/* size */
			       const char *p)
{				/* Packet */
	/* send the packet to destination */
	u8 *ptxb;
	u16 nstype;
	//u16 status;
	u8 *base = (u8 *) BASE;
	int nr = np->next_tx % TX_RING;

	/* point to the current txb incase multiple tx_rings are used */
	ptxb = txb + (nr * RX_NIC_BUFSIZE);
	//np->tx_skbuff[nr] = ptxb;

	/* copy the packet to ring buffer */
	memcpy(ptxb, d, ETH_ALEN);	/* dst */
	memcpy(ptxb + ETH_ALEN, nic->node_addr, ETH_ALEN);	/* src */
	nstype = htons((u16) t);	/* type */
	memcpy(ptxb + 2 * ETH_ALEN, (u8 *) & nstype, 2);	/* type */
	memcpy(ptxb + ETH_HLEN, p, s);

	s += ETH_HLEN;
	while (s < ETH_ZLEN)	/* pad to min length */
		ptxb[s++] = '\0';

	tx_ring[nr].PacketBuffer = (u32) virt_to_le32desc(ptxb);
	tx_ring[nr].Length = cpu_to_le16(s - 1);

	wmb();
	tx_ring[nr].Flags = np->tx_flags;

	writel(NVREG_TXRXCTL_KICK, base + NvRegTxRxControl);
	pci_push(base);
	tx_ring[nr].Flags = np->tx_flags;
	np->next_tx++;
}

/**************************************************************************
DISABLE - Turn off ethernet interface
***************************************************************************/
static void forcedeth_disable(struct dev *dev __unused)
{
	/* put the card in its initial state */
	/* This function serves 3 purposes.
	 * This disables DMA and interrupts so we don't receive
	 *  unexpected packets or interrupts from the card after
	 *  etherboot has finished. 
	 * This frees resources so etherboot may use
	 *  this driver on another interface
	 * This allows etherboot to reinitialize the interface
	 *  if something is something goes wrong.
	 */
	u8 *base = (u8 *) BASE;
	np->in_shutdown = 1;
	stop_tx();
	stop_rx();

	/* disable interrupts on the nic or we will lock up */
	writel(0, base + NvRegIrqMask);
	pci_push(base);
	dprintf(("Irqmask is zero again\n"));

	/* specia op:o write back the misordered MAC address - otherwise
	 * the next probe_nic would see a wrong address.
	 */
	writel(np->orig_mac[0], base + NvRegMacAddrA);
	writel(np->orig_mac[1], base + NvRegMacAddrB);
}

/**
 * is_valid_ether_addr - Determine if the given Ethernet address is valid
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Check that the Ethernet address (MAC) is not 00:00:00:00:00:00, is not
 * a multicast address, and is not FF:FF:FF:FF:FF:FF.  The multicast
 * and FF:FF:... tests are combined into the single test "!(addr[0]&1)".
 *
 * Return true if the address is valid.
 */
static inline int is_valid_ether_addr( u8 *addr ) {
        const char zaddr[6] = {0,};
                                                                                
        return !(addr[0]&1) && memcmp( addr, zaddr, 6);
}

/**************************************************************************
PROBE - Look for an adapter, this routine's visible to the outside
***************************************************************************/
#define IORESOURCE_MEM 0x00000200
#define board_found 1
#define valid_link 0
static int forcedeth_probe(struct dev *dev, struct pci_device *pci)
{
	struct nic *nic = (struct nic *) dev;
	unsigned long addr;
	int sz;
	u8 *base;

	if (pci->ioaddr == 0)
		return 0;

	printf("forcedeth.c: Found %s, vendor=0x%hX, device=0x%hX\n",
	       pci->name, pci->vendor, pci->dev_id);

	/* point to private storage */
	np = &npx;

	adjust_pci_device(pci);

	addr = pci_bar_start(pci, PCI_BASE_ADDRESS_0);
	sz = pci_bar_size(pci, PCI_BASE_ADDRESS_0);

	/* BASE is used throughout to address the card */
	BASE = (unsigned long) ioremap(addr, sz);
	if (!BASE)
		return 0;
	//rx_ring[0] = rx_ring;
	//tx_ring[0] = tx_ring; 

	/* read the mac address */
	base = (u8 *) BASE;
	np->orig_mac[0] = readl(base + NvRegMacAddrA);
	np->orig_mac[1] = readl(base + NvRegMacAddrB);

	nic->node_addr[0] = (np->orig_mac[1] >> 8) & 0xff;
	nic->node_addr[1] = (np->orig_mac[1] >> 0) & 0xff;
	nic->node_addr[2] = (np->orig_mac[0] >> 24) & 0xff;
	nic->node_addr[3] = (np->orig_mac[0] >> 16) & 0xff;
	nic->node_addr[4] = (np->orig_mac[0] >> 8) & 0xff;
	nic->node_addr[5] = (np->orig_mac[0] >> 0) & 0xff;

	if (!is_valid_ether_addr(nic->node_addr)) {
		/*
		 * Bad mac address. At least one bios sets the mac address
		 * to 01:23:45:67:89:ab
		 */
		printf("Invalid Mac address detected: %!\n",
		       nic->node_addr);
		printf("Please complain to your hardware vendor.\n");
		return 0;
	}
	printf("%s: MAC Address %!, ", pci->name, nic->node_addr);

	np->tx_flags =
	    cpu_to_le16(NV_TX_LASTPACKET | NV_TX_LASTPACKET1 |
			NV_TX_VALID);
	switch (pci->dev_id) {
	case 0x01C3:		// nforce
		np->irqmask = NVREG_IRQMASK_WANTED_2;
		np->irqmask |= NVREG_IRQ_TIMER;
		break;
	case 0x0066:		// nforce2
		np->tx_flags |= cpu_to_le16(NV_TX_LASTPACKET1);
		np->irqmask = NVREG_IRQMASK_WANTED_2;
		np->irqmask |= NVREG_IRQ_TIMER;
		break;
	case 0x00D6:		// nforce3
		np->tx_flags |= cpu_to_le16(NV_TX_LASTPACKET1);
		np->irqmask = NVREG_IRQMASK_WANTED_2;
		np->irqmask |= NVREG_IRQ_TIMER;

	}
	dprintf(("%s: forcedeth.c: subsystem: %hX:%hX bound to %s\n",
		 pci->name, pci->vendor, pci->dev_id, pci->name));

	forcedeth_reset(nic);
//      if (board_found && valid_link)
	/* point to NIC specific routines */
	dev->disable = forcedeth_disable;
	nic->poll = forcedeth_poll;
	nic->transmit = forcedeth_transmit;
	return 1;
//      }
	/* else */
}

static struct pci_id forcedeth_nics[] = {
	PCI_ROM(0x10de, 0x01C3, "nforce", "nForce Ethernet Controller"),
	PCI_ROM(0x10de, 0x0066, "nforce2", "nForce2 Ethernet Controller"),
	PCI_ROM(0x10de, 0x00D6, "nforce3", "nForce3 Ethernet Controller"),
};

struct pci_driver forcedeth_driver __pci_driver = {
	.type = NIC_DRIVER,
	.name = "forcedeth",
	.probe = forcedeth_probe,
	.ids = forcedeth_nics,
	.id_count = sizeof(forcedeth_nics) / sizeof(forcedeth_nics[0]),
	.class = 0,
};

