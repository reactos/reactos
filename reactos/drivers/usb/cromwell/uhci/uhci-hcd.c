/*
 * Universal Host Controller Interface driver for USB.
 *
 * Maintainer: Johannes Erdfelt <johannes@erdfelt.com>
 *
 * (C) Copyright 1999 Linus Torvalds
 * (C) Copyright 1999-2002 Johannes Erdfelt, johannes@erdfelt.com
 * (C) Copyright 1999 Randy Dunlap
 * (C) Copyright 1999 Georg Acher, acher@in.tum.de
 * (C) Copyright 1999 Deti Fliegl, deti@fliegl.de
 * (C) Copyright 1999 Thomas Sailer, sailer@ife.ee.ethz.ch
 * (C) Copyright 1999 Roman Weissgaerber, weissg@vienna.at
 * (C) Copyright 2000 Yggdrasil Computing, Inc. (port of new PCI interface
 *               support from usb-ohci.c by Adam Richter, adam@yggdrasil.com).
 * (C) Copyright 1999 Gregory P. Smith (from usb-ohci.c)
 *
 * Intel documents this fairly well, and as far as I know there
 * are no royalties or anything like that, but even so there are
 * people who decided that they want to do the same thing in a
 * completely different way.
 *
 * WARNING! The USB documentation is downright evil. Most of it
 * is just crap, written by a committee. You're better off ignoring
 * most of it, the important stuff is:
 *  - the low-level protocol (fairly simple but lots of small details)
 *  - working around the horridness of the rest
 */

#if 0
#include <linux/config.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#endif

//#ifdef CONFIG_USB_DEBUG
#define DEBUG
//#else
//#undef DEBUG
//#endif

#if 0
#include <linux/usb.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#endif

#include "uhci_config.h"
#include "../usb_wrapper.h"
#include "../core/hcd.h"
#include "uhci-hcd.h"

#if 0
#include <linux/pm.h>
#endif

/*
 * Version Information
 */
#define DRIVER_VERSION "v2.1"
#define DRIVER_AUTHOR "Linus 'Frodo Rabbit' Torvalds, Johannes Erdfelt, Randy Dunlap, Georg Acher, Deti Fliegl, Thomas Sailer, Roman Weissgaerber"
#define DRIVER_DESC "USB Universal Host Controller Interface driver"

/*
 * debug = 0, no debugging messages
 * debug = 1, dump failed URB's except for stalls
 * debug = 2, dump all failed URB's (including stalls)
 *            show all queues in /proc/driver/uhci/[pci_addr]
 * debug = 3, show all TD's in URB's when dumping
 */
#ifdef DEBUG
static int debug = 3;
#else
static int debug = 2;
#endif
MODULE_PARM(debug, "i");
MODULE_PARM_DESC(debug, "Debug level");
static char *errbuf;
#define ERRBUF_LEN    (PAGE_SIZE * 8)

#include "uhci-hub.c"
#include "uhci-debug.c"

static kmem_cache_t *uhci_up_cachep;	/* urb_priv */

static int uhci_get_current_frame_number(struct uhci_hcd *uhci);
static int uhci_urb_dequeue(struct usb_hcd *hcd, struct urb *urb);
static void uhci_unlink_generic(struct uhci_hcd *uhci, struct urb *urb);

static void hc_state_transitions(struct uhci_hcd *uhci);

/* If a transfer is still active after this much time, turn off FSBR */
#define IDLE_TIMEOUT	(HZ / 20)	/* 50 ms */
#define FSBR_DELAY	(HZ / 20)	/* 50 ms */

/* When we timeout an idle transfer for FSBR, we'll switch it over to */
/* depth first traversal. We'll do it in groups of this number of TD's */
/* to make sure it doesn't hog all of the bandwidth */
#define DEPTH_INTERVAL 5

/*
 * Technically, updating td->status here is a race, but it's not really a
 * problem. The worst that can happen is that we set the IOC bit again
 * generating a spurious interrupt. We could fix this by creating another
 * QH and leaving the IOC bit always set, but then we would have to play
 * games with the FSBR code to make sure we get the correct order in all
 * the cases. I don't think it's worth the effort
 */
static inline void uhci_set_next_interrupt(struct uhci_hcd *uhci)
{
	unsigned long flags;

	spin_lock_irqsave(&uhci->frame_list_lock, flags);
	uhci->term_td->status |= cpu_to_le32(TD_CTRL_IOC);
	spin_unlock_irqrestore(&uhci->frame_list_lock, flags);
}

static inline void uhci_clear_next_interrupt(struct uhci_hcd *uhci)
{
	unsigned long flags;

	spin_lock_irqsave(&uhci->frame_list_lock, flags);
	uhci->term_td->status &= ~cpu_to_le32(TD_CTRL_IOC);
	spin_unlock_irqrestore(&uhci->frame_list_lock, flags);
}

static inline void uhci_add_complete(struct uhci_hcd *uhci, struct urb *urb)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;
	unsigned long flags;

	spin_lock_irqsave(&uhci->complete_list_lock, flags);
	list_add_tail(&urbp->complete_list, &uhci->complete_list);
	spin_unlock_irqrestore(&uhci->complete_list_lock, flags);
}

static struct uhci_td *uhci_alloc_td(struct uhci_hcd *uhci, struct usb_device *dev)
{
	dma_addr_t dma_handle;
	struct uhci_td *td;

	td = pci_pool_alloc(uhci->td_pool, GFP_ATOMIC, &dma_handle);
	if (!td)
		return NULL;

	td->dma_handle = dma_handle;

	td->link = UHCI_PTR_TERM;
	td->buffer = 0;

	td->frame = -1;
	td->dev = dev;

	INIT_LIST_HEAD(&td->list);
	INIT_LIST_HEAD(&td->fl_list);

	usb_get_dev(dev);

	return td;
}

static inline void uhci_fill_td(struct uhci_td *td, __u32 status,
		__u32 token, __u32 buffer)
{
	td->status = cpu_to_le32(status);
	td->token = cpu_to_le32(token);
	td->buffer = cpu_to_le32(buffer);
}

/*
 * We insert Isochronous URB's directly into the frame list at the beginning
 */
static void uhci_insert_td_frame_list(struct uhci_hcd *uhci, struct uhci_td *td, unsigned framenum)
{
	unsigned long flags;

	framenum %= UHCI_NUMFRAMES;

	spin_lock_irqsave(&uhci->frame_list_lock, flags);

	td->frame = framenum;

	/* Is there a TD already mapped there? */
	if (uhci->fl->frame_cpu[framenum]) {
		struct uhci_td *ftd, *ltd;

		ftd = uhci->fl->frame_cpu[framenum];
		ltd = list_entry(ftd->fl_list.prev, struct uhci_td, fl_list);

		list_add_tail(&td->fl_list, &ftd->fl_list);

		td->link = ltd->link;
		mb();
		ltd->link = cpu_to_le32(td->dma_handle);
	} else {
		td->link = uhci->fl->frame[framenum];
		mb();
		uhci->fl->frame[framenum] = cpu_to_le32(td->dma_handle);
		uhci->fl->frame_cpu[framenum] = td;
	}

	spin_unlock_irqrestore(&uhci->frame_list_lock, flags);
}

static void uhci_remove_td(struct uhci_hcd *uhci, struct uhci_td *td)
{
	unsigned long flags;

	/* If it's not inserted, don't remove it */
	spin_lock_irqsave(&uhci->frame_list_lock, flags);
	if (td->frame == -1 && list_empty(&td->fl_list))
		goto out;

	if (td->frame != -1 && uhci->fl->frame_cpu[td->frame] == td) {
		if (list_empty(&td->fl_list)) {
			uhci->fl->frame[td->frame] = td->link;
			uhci->fl->frame_cpu[td->frame] = NULL;
		} else {
			struct uhci_td *ntd;

			ntd = list_entry(td->fl_list.next, struct uhci_td, fl_list);
			uhci->fl->frame[td->frame] = cpu_to_le32(ntd->dma_handle);
			uhci->fl->frame_cpu[td->frame] = ntd;
		}
	} else {
		struct uhci_td *ptd;

		ptd = list_entry(td->fl_list.prev, struct uhci_td, fl_list);
		ptd->link = td->link;
	}

	mb();
	td->link = UHCI_PTR_TERM;

	list_del_init(&td->fl_list);
	td->frame = -1;

out:
	spin_unlock_irqrestore(&uhci->frame_list_lock, flags);
}

/*
 * Inserts a td into qh list at the top.
 */
static void uhci_insert_tds_in_qh(struct uhci_qh *qh, struct urb *urb, u32 breadth)
{
	struct list_head *tmp, *head;
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;
	struct uhci_td *td, *ptd;

	if (list_empty(&urbp->td_list))
		return;

	head = &urbp->td_list;
	tmp = head->next;

	/* Ordering isn't important here yet since the QH hasn't been */
	/*  inserted into the schedule yet */
	td = list_entry(tmp, struct uhci_td, list);

	/* Add the first TD to the QH element pointer */
	qh->element = cpu_to_le32(td->dma_handle) | breadth;

	ptd = td;

	/* Then link the rest of the TD's */
	tmp = tmp->next;
	while (tmp != head) {
		td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		ptd->link = cpu_to_le32(td->dma_handle) | breadth;

		ptd = td;
	}

	ptd->link = UHCI_PTR_TERM;
}

static void uhci_free_td(struct uhci_hcd *uhci, struct uhci_td *td)
{
	if (!list_empty(&td->list))
		dbg("td %p is still in list!", td);
	if (!list_empty(&td->fl_list))
		dbg("td %p is still in fl_list!", td);

	if (td->dev)
		usb_put_dev(td->dev);

	pci_pool_free(uhci->td_pool, td, td->dma_handle);
}

static struct uhci_qh *uhci_alloc_qh(struct uhci_hcd *uhci, struct usb_device *dev)
{
	dma_addr_t dma_handle;
	struct uhci_qh *qh;

	qh = pci_pool_alloc(uhci->qh_pool, GFP_ATOMIC, &dma_handle);
	if (!qh)
		return NULL;

	qh->dma_handle = dma_handle;

	qh->element = UHCI_PTR_TERM;
	qh->link = UHCI_PTR_TERM;

	qh->dev = dev;
	qh->urbp = NULL;

	INIT_LIST_HEAD(&qh->list);
	INIT_LIST_HEAD(&qh->remove_list);

	usb_get_dev(dev);

	return qh;
}

static void uhci_free_qh(struct uhci_hcd *uhci, struct uhci_qh *qh)
{
	if (!list_empty(&qh->list))
		dbg("qh %p list not empty!", qh);
	if (!list_empty(&qh->remove_list))
		dbg("qh %p still in remove_list!", qh);

	if (qh->dev)
		usb_put_dev(qh->dev);

	pci_pool_free(uhci->qh_pool, qh, qh->dma_handle);
}

/*
 * Append this urb's qh after the last qh in skelqh->list
 * MUST be called with uhci->frame_list_lock acquired
 *
 * Note that urb_priv.queue_list doesn't have a separate queue head;
 * it's a ring with every element "live".
 */
static void _uhci_insert_qh(struct uhci_hcd *uhci, struct uhci_qh *skelqh, struct urb *urb)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;
	struct list_head *tmp;
	struct uhci_qh *lqh;

	/* Grab the last QH */
	lqh = list_entry(skelqh->list.prev, struct uhci_qh, list);

	/*
	 * Patch this endpoint's URB's QHs to point to the next skelqh:
	 *    skelqh --> ... lqh --> newqh --> next skelqh
	 * Do this first, so the HC always sees the right QH after this one.
	 */
	list_for_each (tmp, &urbp->queue_list) {
		struct urb_priv *turbp =
			list_entry(tmp, struct urb_priv, queue_list);

		turbp->qh->link = lqh->link;
	}
	urbp->qh->link = lqh->link;
	wmb();				/* Ordering is important */

	/*
	 * Patch QHs for previous endpoint's queued URBs?  HC goes
	 * here next, not to the next skelqh it now points to.
	 *
	 *    lqh --> td ... --> qh ... --> td --> qh ... --> td
	 *     |                 |                 |
	 *     v                 v                 v
	 *     +<----------------+-----------------+
	 *     v
	 *    newqh --> td ... --> td
	 *     |
	 *     v
	 *    ...
	 *
	 * The HC could see (and use!) any of these as we write them.
	 */
	if (lqh->urbp) {
		list_for_each (tmp, &lqh->urbp->queue_list) {
			struct urb_priv *turbp =
				list_entry(tmp, struct urb_priv, queue_list);

			turbp->qh->link = cpu_to_le32(urbp->qh->dma_handle) | UHCI_PTR_QH;
		}
	}
	lqh->link = cpu_to_le32(urbp->qh->dma_handle) | UHCI_PTR_QH;

	list_add_tail(&urbp->qh->list, &skelqh->list);
}

static void uhci_insert_qh(struct uhci_hcd *uhci, struct uhci_qh *skelqh, struct urb *urb)
{
	unsigned long flags;

	spin_lock_irqsave(&uhci->frame_list_lock, flags);
	_uhci_insert_qh(uhci, skelqh, urb);
	spin_unlock_irqrestore(&uhci->frame_list_lock, flags);
}

/*
 * Start removal of QH from schedule; it finishes next frame.
 * TDs should be unlinked before this is called.
 */
static void uhci_remove_qh(struct uhci_hcd *uhci, struct uhci_qh *qh)
{
	unsigned long flags;
	struct uhci_qh *pqh;

	if (!qh)
		return;

	qh->urbp = NULL;

	/*
	 * Only go through the hoops if it's actually linked in
	 * Queued QHs are removed in uhci_delete_queued_urb,
	 * since (for queued URBs) the pqh is pointed to the next
	 * QH in the queue, not the next endpoint's QH.
	 */
	spin_lock_irqsave(&uhci->frame_list_lock, flags);
	if (!list_empty(&qh->list)) {
		pqh = list_entry(qh->list.prev, struct uhci_qh, list);

		if (pqh->urbp) {
			struct list_head *head, *tmp;

			head = &pqh->urbp->queue_list;
			tmp = head->next;
			while (head != tmp) {
				struct urb_priv *turbp =
					list_entry(tmp, struct urb_priv, queue_list);

				tmp = tmp->next;

				turbp->qh->link = qh->link;
			}
		}

		pqh->link = qh->link;
		mb();
		/* Leave qh->link in case the HC is on the QH now, it will */
		/* continue the rest of the schedule */
		qh->element = UHCI_PTR_TERM;

		list_del_init(&qh->list);
	}
	spin_unlock_irqrestore(&uhci->frame_list_lock, flags);

	spin_lock_irqsave(&uhci->qh_remove_list_lock, flags);

	/* Check to see if the remove list is empty. Set the IOC bit */
	/* to force an interrupt so we can remove the QH */
	if (list_empty(&uhci->qh_remove_list))
		uhci_set_next_interrupt(uhci);

	list_add(&qh->remove_list, &uhci->qh_remove_list);

	spin_unlock_irqrestore(&uhci->qh_remove_list_lock, flags);
}

static int uhci_fixup_toggle(struct urb *urb, unsigned int toggle)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;
	struct list_head *head, *tmp;

	head = &urbp->td_list;
	tmp = head->next;
	while (head != tmp) {
		struct uhci_td *td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		if (toggle)
			td->token |= cpu_to_le32(TD_TOKEN_TOGGLE);
		else
			td->token &= ~cpu_to_le32(TD_TOKEN_TOGGLE);


		toggle ^= 1;
	}

	return toggle;
}

/* This function will append one URB's QH to another URB's QH. This is for */
/* queuing interrupt, control or bulk transfers */
static void uhci_append_queued_urb(struct uhci_hcd *uhci, struct urb *eurb, struct urb *urb)
{
	struct urb_priv *eurbp, *urbp, *furbp, *lurbp;
	struct list_head *tmp;
	struct uhci_td *lltd;
	unsigned long flags;

	eurbp = eurb->hcpriv;
	urbp = urb->hcpriv;

	spin_lock_irqsave(&uhci->frame_list_lock, flags);

	/* Find the first URB in the queue */
	if (eurbp->queued) {
		struct list_head *head = &eurbp->queue_list;

		tmp = head->next;
		while (tmp != head) {
			struct urb_priv *turbp =
				list_entry(tmp, struct urb_priv, queue_list);

			if (!turbp->queued)
				break;

			tmp = tmp->next;
		}
	} else
		tmp = &eurbp->queue_list;

	furbp = list_entry(tmp, struct urb_priv, queue_list);
	lurbp = list_entry(furbp->queue_list.prev, struct urb_priv, queue_list);

	lltd = list_entry(lurbp->td_list.prev, struct uhci_td, list);

	usb_settoggle(urb->dev, usb_pipeendpoint(urb->pipe), usb_pipeout(urb->pipe),
		uhci_fixup_toggle(urb, uhci_toggle(td_token(lltd)) ^ 1));

	/* All qh's in the queue need to link to the next queue */
	urbp->qh->link = eurbp->qh->link;

	mb();			/* Make sure we flush everything */

	lltd->link = cpu_to_le32(urbp->qh->dma_handle) | UHCI_PTR_QH;

	list_add_tail(&urbp->queue_list, &furbp->queue_list);

	urbp->queued = 1;

	spin_unlock_irqrestore(&uhci->frame_list_lock, flags);
}

static void uhci_delete_queued_urb(struct uhci_hcd *uhci, struct urb *urb)
{
	struct urb_priv *urbp, *nurbp;
	struct list_head *head, *tmp;
	struct urb_priv *purbp;
	struct uhci_td *pltd;
	unsigned int toggle;
	unsigned long flags;

	urbp = urb->hcpriv;

	spin_lock_irqsave(&uhci->frame_list_lock, flags);

	if (list_empty(&urbp->queue_list))
		goto out;

	nurbp = list_entry(urbp->queue_list.next, struct urb_priv, queue_list);

	/* Fix up the toggle for the next URB's */
	if (!urbp->queued)
		/* We just set the toggle in uhci_unlink_generic */
		toggle = usb_gettoggle(urb->dev, usb_pipeendpoint(urb->pipe), usb_pipeout(urb->pipe));
	else {
		/* If we're in the middle of the queue, grab the toggle */
		/*  from the TD previous to us */
		purbp = list_entry(urbp->queue_list.prev, struct urb_priv,
				queue_list);

		pltd = list_entry(purbp->td_list.prev, struct uhci_td, list);

		toggle = uhci_toggle(td_token(pltd)) ^ 1;
	}

	head = &urbp->queue_list;
	tmp = head->next;
	while (head != tmp) {
		struct urb_priv *turbp;

		turbp = list_entry(tmp, struct urb_priv, queue_list);

		tmp = tmp->next;

		if (!turbp->queued)
			break;

		toggle = uhci_fixup_toggle(turbp->urb, toggle);
	}

	usb_settoggle(urb->dev, usb_pipeendpoint(urb->pipe),
		usb_pipeout(urb->pipe), toggle);

	if (!urbp->queued) {
		struct uhci_qh *pqh;

		nurbp->queued = 0;

		/*
		 * Fixup the previous QH's queue to link to the new head
		 * of this queue.
		 */
		pqh = list_entry(urbp->qh->list.prev, struct uhci_qh, list);

		if (pqh->urbp) {
			struct list_head *head, *tmp;

			head = &pqh->urbp->queue_list;
			tmp = head->next;
			while (head != tmp) {
				struct urb_priv *turbp =
					list_entry(tmp, struct urb_priv, queue_list);

				tmp = tmp->next;

				turbp->qh->link = cpu_to_le32(nurbp->qh->dma_handle) | UHCI_PTR_QH;
			}
		}

		pqh->link = cpu_to_le32(nurbp->qh->dma_handle) | UHCI_PTR_QH;

		list_add_tail(&nurbp->qh->list, &urbp->qh->list);
		list_del_init(&urbp->qh->list);
	} else {
		/* We're somewhere in the middle (or end). A bit trickier */
		/*  than the head scenario */
		purbp = list_entry(urbp->queue_list.prev, struct urb_priv,
				queue_list);

		pltd = list_entry(purbp->td_list.prev, struct uhci_td, list);
		if (nurbp->queued)
			pltd->link = cpu_to_le32(nurbp->qh->dma_handle) | UHCI_PTR_QH;
		else
			/* The next URB happens to be the beginning, so */
			/*  we're the last, end the chain */
			pltd->link = UHCI_PTR_TERM;
	}

	list_del_init(&urbp->queue_list);

out:
	spin_unlock_irqrestore(&uhci->frame_list_lock, flags);
}

static struct urb_priv *uhci_alloc_urb_priv(struct uhci_hcd *uhci, struct urb *urb)
{
	struct urb_priv *urbp;

	urbp = kmem_cache_alloc(uhci_up_cachep, SLAB_ATOMIC);
	if (!urbp) {
		err("uhci_alloc_urb_priv: couldn't allocate memory for urb_priv\n");
		return NULL;
	}

	memset((void *)urbp, 0, sizeof(*urbp));

	urbp->inserttime = jiffies;
	urbp->fsbrtime = jiffies;
	urbp->urb = urb;
	urbp->dev = urb->dev;

	INIT_LIST_HEAD(&urbp->td_list);
	INIT_LIST_HEAD(&urbp->queue_list);
	INIT_LIST_HEAD(&urbp->complete_list);
	INIT_LIST_HEAD(&urbp->urb_list);

	list_add_tail(&urbp->urb_list, &uhci->urb_list);

	urb->hcpriv = urbp;

	return urbp;
}

/*
 * MUST be called with urb->lock acquired
 */
static void uhci_add_td_to_urb(struct urb *urb, struct uhci_td *td)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;

	td->urb = urb;

	list_add_tail(&td->list, &urbp->td_list);
}

/*
 * MUST be called with urb->lock acquired
 */
static void uhci_remove_td_from_urb(struct uhci_td *td)
{
	if (list_empty(&td->list))
		return;

	list_del_init(&td->list);

	td->urb = NULL;
}

/*
 * MUST be called with urb->lock acquired
 */
static void uhci_destroy_urb_priv(struct uhci_hcd *uhci, struct urb *urb)
{
	struct list_head *head, *tmp;
	struct urb_priv *urbp;

	urbp = (struct urb_priv *)urb->hcpriv;
	if (!urbp)
		return;

	if (!list_empty(&urbp->urb_list))
		warn("uhci_destroy_urb_priv: urb %p still on uhci->urb_list or uhci->remove_list", urb);

	if (!list_empty(&urbp->complete_list))
		warn("uhci_destroy_urb_priv: urb %p still on uhci->complete_list", urb);

	head = &urbp->td_list;
	tmp = head->next;
	while (tmp != head) {
		struct uhci_td *td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		uhci_remove_td_from_urb(td);
		uhci_remove_td(uhci, td);
		uhci_free_td(uhci, td);
	}

	urb->hcpriv = NULL;
	kmem_cache_free(uhci_up_cachep, urbp);
}

static void uhci_inc_fsbr(struct uhci_hcd *uhci, struct urb *urb)
{
	unsigned long flags;
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;

	spin_lock_irqsave(&uhci->frame_list_lock, flags);

	if ((!(urb->transfer_flags & URB_NO_FSBR)) && !urbp->fsbr) {
		urbp->fsbr = 1;
		if (!uhci->fsbr++ && !uhci->fsbrtimeout)
			uhci->skel_term_qh->link = cpu_to_le32(uhci->skel_hs_control_qh->dma_handle) | UHCI_PTR_QH;
	}

	spin_unlock_irqrestore(&uhci->frame_list_lock, flags);
}

static void uhci_dec_fsbr(struct uhci_hcd *uhci, struct urb *urb)
{
	unsigned long flags;
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;

	spin_lock_irqsave(&uhci->frame_list_lock, flags);

	if ((!(urb->transfer_flags & URB_NO_FSBR)) && urbp->fsbr) {
		urbp->fsbr = 0;
		if (!--uhci->fsbr)
			uhci->fsbrtimeout = jiffies + FSBR_DELAY;
	}

	spin_unlock_irqrestore(&uhci->frame_list_lock, flags);
}

/*
 * Map status to standard result codes
 *
 * <status> is (td->status & 0xFE0000) [a.k.a. uhci_status_bits(td->status)]
 * <dir_out> is True for output TDs and False for input TDs.
 */
static int uhci_map_status(int status, int dir_out)
{
	if (!status)
		return 0;
	if (status & TD_CTRL_BITSTUFF)			/* Bitstuff error */
		return -EPROTO;
	if (status & TD_CTRL_CRCTIMEO) {		/* CRC/Timeout */
		if (dir_out)
			return -ETIMEDOUT;
		else
			return -EILSEQ;
	}
	if (status & TD_CTRL_NAK)			/* NAK */
		return -ETIMEDOUT;
	if (status & TD_CTRL_BABBLE)			/* Babble */
		return -EOVERFLOW;
	if (status & TD_CTRL_DBUFERR)			/* Buffer error */
		return -ENOSR;
	if (status & TD_CTRL_STALLED)			/* Stalled */
		return -EPIPE;
	if (status & TD_CTRL_ACTIVE)			/* Active */
		return 0;

	return -EINVAL;
}

/*
 * Control transfers
 */
static int uhci_submit_control(struct uhci_hcd *uhci, struct urb *urb, struct urb *eurb)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;
	struct uhci_td *td;
	struct uhci_qh *qh, *skelqh;
	unsigned long destination, status;
	int maxsze = usb_maxpacket(urb->dev, urb->pipe, usb_pipeout(urb->pipe));
	int len = urb->transfer_buffer_length;
	dma_addr_t data = urb->transfer_dma;

	/* The "pipe" thing contains the destination in bits 8--18 */
	destination = (urb->pipe & PIPE_DEVEP_MASK) | USB_PID_SETUP;

	/* 3 errors */
	status = TD_CTRL_ACTIVE | uhci_maxerr(3);
	if (urb->dev->speed == USB_SPEED_LOW)
		status |= TD_CTRL_LS;

	/*
	 * Build the TD for the control request
	 */
	td = uhci_alloc_td(uhci, urb->dev);
	if (!td)
		return -ENOMEM;

	uhci_add_td_to_urb(urb, td);
	uhci_fill_td(td, status, destination | uhci_explen(7),
		urb->setup_dma);

	/*
	 * If direction is "send", change the frame from SETUP (0x2D)
	 * to OUT (0xE1). Else change it from SETUP to IN (0x69).
	 */
	destination ^= (USB_PID_SETUP ^ usb_packetid(urb->pipe));

	if (!(urb->transfer_flags & URB_SHORT_NOT_OK))
		status |= TD_CTRL_SPD;

	/*
	 * Build the DATA TD's
	 */
	while (len > 0) {
		int pktsze = len;

		if (pktsze > maxsze)
			pktsze = maxsze;

		td = uhci_alloc_td(uhci, urb->dev);
		if (!td)
			return -ENOMEM;

		/* Alternate Data0/1 (start with Data1) */
		destination ^= TD_TOKEN_TOGGLE;

		uhci_add_td_to_urb(urb, td);
		uhci_fill_td(td, status, destination | uhci_explen(pktsze - 1),
			data);

		data += pktsze;
		len -= pktsze;
	}

	/*
	 * Build the final TD for control status
	 */
	td = uhci_alloc_td(uhci, urb->dev);
	if (!td)
		return -ENOMEM;

	/*
	 * It's IN if the pipe is an output pipe or we're not expecting
	 * data back.
	 */
	destination &= ~TD_TOKEN_PID_MASK;
	if (usb_pipeout(urb->pipe) || !urb->transfer_buffer_length)
		destination |= USB_PID_IN;
	else
		destination |= USB_PID_OUT;

	destination |= TD_TOKEN_TOGGLE;		/* End in Data1 */

	status &= ~TD_CTRL_SPD;

	uhci_add_td_to_urb(urb, td);
	uhci_fill_td(td, status | TD_CTRL_IOC,
		destination | uhci_explen(UHCI_NULL_DATA_SIZE), 0);

	qh = uhci_alloc_qh(uhci, urb->dev);
	if (!qh)
		return -ENOMEM;

	urbp->qh = qh;
	qh->urbp = urbp;

	uhci_insert_tds_in_qh(qh, urb, UHCI_PTR_BREADTH);

	/* Low speed transfers get a different queue, and won't hog the bus */
	if (urb->dev->speed == USB_SPEED_LOW)
		skelqh = uhci->skel_ls_control_qh;
	else {
		skelqh = uhci->skel_hs_control_qh;
		uhci_inc_fsbr(uhci, urb);
	}

	if (eurb)
		uhci_append_queued_urb(uhci, eurb, urb);
	else
		uhci_insert_qh(uhci, skelqh, urb);

	return -EINPROGRESS;
}

/*
 * If control was short, then end status packet wasn't sent, so this
 * reorganize s so it's sent to finish the transfer.  The original QH is
 * removed from the skel and discarded; all TDs except the last (status)
 * are deleted; the last (status) TD is put on a new QH which is reinserted
 * into the skel.  Since the last TD and urb_priv are reused, the TD->link
 * and urb_priv maintain any queued QHs.
 */
static int usb_control_retrigger_status(struct uhci_hcd *uhci, struct urb *urb)
{
	struct list_head *tmp, *head;
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;

	urbp->short_control_packet = 1;

	/* Create a new QH to avoid pointer overwriting problems */
	uhci_remove_qh(uhci, urbp->qh);

	/* Delete all of the TD's except for the status TD at the end */
	head = &urbp->td_list;
	tmp = head->next;
	while (tmp != head && tmp->next != head) {
		struct uhci_td *td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		uhci_remove_td_from_urb(td);
		uhci_remove_td(uhci, td);
		uhci_free_td(uhci, td);
	}

	urbp->qh = uhci_alloc_qh(uhci, urb->dev);
	if (!urbp->qh) {
		err("unable to allocate new QH for control retrigger");
		return -ENOMEM;
	}

	urbp->qh->urbp = urbp;

	/* One TD, who cares about Breadth first? */
	uhci_insert_tds_in_qh(urbp->qh, urb, UHCI_PTR_DEPTH);

	/* Low speed transfers get a different queue */
	if (urb->dev->speed == USB_SPEED_LOW)
		uhci_insert_qh(uhci, uhci->skel_ls_control_qh, urb);
	else
		uhci_insert_qh(uhci, uhci->skel_hs_control_qh, urb);

	return -EINPROGRESS;
}


static int uhci_result_control(struct uhci_hcd *uhci, struct urb *urb)
{
	struct list_head *tmp, *head;
	struct urb_priv *urbp = urb->hcpriv;
	struct uhci_td *td;
	unsigned int status;
	int ret = 0;

	if (list_empty(&urbp->td_list))
		return -EINVAL;

	head = &urbp->td_list;

	if (urbp->short_control_packet) {
		tmp = head->prev;
		goto status_phase;
	}

	tmp = head->next;
	td = list_entry(tmp, struct uhci_td, list);

	/* The first TD is the SETUP phase, check the status, but skip */
	/*  the count */
	status = uhci_status_bits(td_status(td));
	if (status & TD_CTRL_ACTIVE)
		return -EINPROGRESS;

	if (status)
		goto td_error;

	urb->actual_length = 0;

	/* The rest of the TD's (but the last) are data */
	tmp = tmp->next;
	while (tmp != head && tmp->next != head) {
		td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		status = uhci_status_bits(td_status(td));
		if (status & TD_CTRL_ACTIVE)
			return -EINPROGRESS;

		urb->actual_length += uhci_actual_length(td_status(td));

		if (status)
			goto td_error;

		/* Check to see if we received a short packet */
		if (uhci_actual_length(td_status(td)) < uhci_expected_length(td_token(td))) {
			if (urb->transfer_flags & URB_SHORT_NOT_OK) {
				ret = -EREMOTEIO;
				goto err;
			}

			if (uhci_packetid(td_token(td)) == USB_PID_IN)
				return usb_control_retrigger_status(uhci, urb);
			else
				return 0;
		}
	}

status_phase:
	td = list_entry(tmp, struct uhci_td, list);

	/* Control status phase */
	status = td_status(td);

#ifdef I_HAVE_BUGGY_APC_BACKUPS
	/* APC BackUPS Pro kludge */
	/* It tries to send all of the descriptor instead of the amount */
	/*  we requested */
	if (status & TD_CTRL_IOC &&	/* IOC is masked out by uhci_status_bits */
	    status & TD_CTRL_ACTIVE &&
	    status & TD_CTRL_NAK)
		return 0;
#endif

	if (status & TD_CTRL_ACTIVE)
		return -EINPROGRESS;

	if (uhci_status_bits(status))
		goto td_error;

	return 0;

td_error:
	ret = uhci_map_status(status, uhci_packetout(td_token(td)));

err:
	if ((debug == 1 && ret != -EPIPE) || debug > 1) {
		/* Some debugging code */
		dbg("uhci_result_control() failed with status %x", status);

		if (errbuf) {
			/* Print the chain for debugging purposes */
			uhci_show_qh(urbp->qh, errbuf, ERRBUF_LEN, 0);

			lprintk(errbuf);
		}
	}

	return ret;
}

/*
 * Common submit for bulk and interrupt
 */
static int uhci_submit_common(struct uhci_hcd *uhci, struct urb *urb, struct urb *eurb, struct uhci_qh *skelqh)
{
	struct uhci_td *td;
	struct uhci_qh *qh;
	unsigned long destination, status;
	int maxsze = usb_maxpacket(urb->dev, urb->pipe, usb_pipeout(urb->pipe));
	int len = urb->transfer_buffer_length;
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;
	dma_addr_t data = urb->transfer_dma;

	if (len < 0)
		return -EINVAL;

	/* The "pipe" thing contains the destination in bits 8--18 */
	destination = (urb->pipe & PIPE_DEVEP_MASK) | usb_packetid(urb->pipe);

	status = uhci_maxerr(3) | TD_CTRL_ACTIVE;
	if (urb->dev->speed == USB_SPEED_LOW)
		status |= TD_CTRL_LS;
	if (!(urb->transfer_flags & URB_SHORT_NOT_OK))
		status |= TD_CTRL_SPD;

	/*
	 * Build the DATA TD's
	 */
	do {	/* Allow zero length packets */
		int pktsze = len;

		if (pktsze > maxsze)
			pktsze = maxsze;

		td = uhci_alloc_td(uhci, urb->dev);
		if (!td)
			return -ENOMEM;

		uhci_add_td_to_urb(urb, td);
		uhci_fill_td(td, status, destination | uhci_explen(pktsze - 1) |
			(usb_gettoggle(urb->dev, usb_pipeendpoint(urb->pipe),
			 usb_pipeout(urb->pipe)) << TD_TOKEN_TOGGLE_SHIFT),
			data);

		data += pktsze;
		len -= maxsze;

		usb_dotoggle(urb->dev, usb_pipeendpoint(urb->pipe),
			usb_pipeout(urb->pipe));
	} while (len > 0);

	/*
	 * URB_ZERO_PACKET means adding a 0-length packet, if direction
	 * is OUT and the transfer_length was an exact multiple of maxsze,
	 * hence (len = transfer_length - N * maxsze) == 0
	 * however, if transfer_length == 0, the zero packet was already
	 * prepared above.
	 */
	if (usb_pipeout(urb->pipe) && (urb->transfer_flags & URB_ZERO_PACKET) &&
	    !len && urb->transfer_buffer_length) {
		td = uhci_alloc_td(uhci, urb->dev);
		if (!td)
			return -ENOMEM;

		uhci_add_td_to_urb(urb, td);
		uhci_fill_td(td, status, destination | uhci_explen(UHCI_NULL_DATA_SIZE) |
			(usb_gettoggle(urb->dev, usb_pipeendpoint(urb->pipe),
			 usb_pipeout(urb->pipe)) << TD_TOKEN_TOGGLE_SHIFT),
			data);

		usb_dotoggle(urb->dev, usb_pipeendpoint(urb->pipe),
			usb_pipeout(urb->pipe));
	}

	/* Set the flag on the last packet */
	td->status |= cpu_to_le32(TD_CTRL_IOC);

	qh = uhci_alloc_qh(uhci, urb->dev);
	if (!qh)
		return -ENOMEM;

	urbp->qh = qh;
	qh->urbp = urbp;

	/* Always breadth first */
	uhci_insert_tds_in_qh(qh, urb, UHCI_PTR_BREADTH);

	if (eurb)
		uhci_append_queued_urb(uhci, eurb, urb);
	else
		uhci_insert_qh(uhci, skelqh, urb);

	return -EINPROGRESS;
}

/*
 * Common result for bulk and interrupt
 */
static int uhci_result_common(struct uhci_hcd *uhci, struct urb *urb)
{
	struct list_head *tmp, *head;
	struct urb_priv *urbp = urb->hcpriv;
	struct uhci_td *td;
	unsigned int status = 0;
	int ret = 0;

	urb->actual_length = 0;

	head = &urbp->td_list;
	tmp = head->next;
	while (tmp != head) {
		td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		status = uhci_status_bits(td_status(td));
		if (status & TD_CTRL_ACTIVE)
			return -EINPROGRESS;

		urb->actual_length += uhci_actual_length(td_status(td));

		if (status)
			goto td_error;

		if (uhci_actual_length(td_status(td)) < uhci_expected_length(td_token(td))) {
			if (urb->transfer_flags & URB_SHORT_NOT_OK) {
				ret = -EREMOTEIO;
				goto err;
			} else
				return 0;
		}
	}

	return 0;

td_error:
	ret = uhci_map_status(status, uhci_packetout(td_token(td)));
	if (ret == -EPIPE)
		/* endpoint has stalled - mark it halted */
		usb_endpoint_halt(urb->dev, uhci_endpoint(td_token(td)),
	    			uhci_packetout(td_token(td)));

err:
	/*
	 * Enable this chunk of code if you want to see some more debugging.
	 * But be careful, it has the tendancy to starve out khubd and prevent
	 * disconnects from happening successfully if you have a slow debug
	 * log interface (like a serial console.
	 */
#if 0
	if ((debug == 1 && ret != -EPIPE) || debug > 1) {
		/* Some debugging code */
		dbg("uhci_result_common() failed with status %x", status);

		if (errbuf) {
			/* Print the chain for debugging purposes */
			uhci_show_qh(urbp->qh, errbuf, ERRBUF_LEN, 0);

			lprintk(errbuf);
		}
	}
#endif
	return ret;
}

static inline int uhci_submit_bulk(struct uhci_hcd *uhci, struct urb *urb, struct urb *eurb)
{
	int ret;

	/* Can't have low speed bulk transfers */
	if (urb->dev->speed == USB_SPEED_LOW)
		return -EINVAL;

	ret = uhci_submit_common(uhci, urb, eurb, uhci->skel_bulk_qh);
	if (ret == -EINPROGRESS)
		uhci_inc_fsbr(uhci, urb);

	return ret;
}

static inline int uhci_submit_interrupt(struct uhci_hcd *uhci, struct urb *urb, struct urb *eurb)
{
	/* USB 1.1 interrupt transfers only involve one packet per interval;
	 * that's the uhci_submit_common() "breadth first" policy.  Drivers
	 * can submit urbs of any length, but longer ones might need many
	 * intervals to complete.
	 */
	return uhci_submit_common(uhci, urb, eurb, uhci->skelqh[__interval_to_skel(urb->interval)]);
}

/*
 * Bulk and interrupt use common result
 */
#define uhci_result_bulk uhci_result_common
#define uhci_result_interrupt uhci_result_common

/*
 * Isochronous transfers
 */
static int isochronous_find_limits(struct uhci_hcd *uhci, struct urb *urb, unsigned int *start, unsigned int *end)
{
	struct urb *last_urb = NULL;
	struct list_head *tmp, *head;
	int ret = 0;

	head = &uhci->urb_list;
	tmp = head->next;
	while (tmp != head) {
		struct urb_priv *up = list_entry(tmp, struct urb_priv, urb_list);
		struct urb *u = up->urb;

		tmp = tmp->next;

		/* look for pending URB's with identical pipe handle */
		if ((urb->pipe == u->pipe) && (urb->dev == u->dev) &&
		    (u->status == -EINPROGRESS) && (u != urb)) {
			if (!last_urb)
				*start = u->start_frame;
			last_urb = u;
		}
	}

	if (last_urb) {
		*end = (last_urb->start_frame + last_urb->number_of_packets *
				last_urb->interval) & (UHCI_NUMFRAMES-1);
		ret = 0;
	} else
		ret = -1;	/* no previous urb found */

	return ret;
}

static int isochronous_find_start(struct uhci_hcd *uhci, struct urb *urb)
{
	int limits;
	unsigned int start = 0, end = 0;

	if (urb->number_of_packets > 900)	/* 900? Why? */
		return -EFBIG;

	limits = isochronous_find_limits(uhci, urb, &start, &end);

	if (urb->transfer_flags & URB_ISO_ASAP) {
		if (limits) {
			int curframe;

			curframe = uhci_get_current_frame_number(uhci) % UHCI_NUMFRAMES;
			urb->start_frame = (curframe + 10) % UHCI_NUMFRAMES;
		} else
			urb->start_frame = end;
	} else {
		urb->start_frame %= UHCI_NUMFRAMES;
		/* FIXME: Sanity check */
	}

	return 0;
}

/*
 * Isochronous transfers
 */
static int uhci_submit_isochronous(struct uhci_hcd *uhci, struct urb *urb)
{
	struct uhci_td *td;
	int i, ret, frame;
	int status, destination;

	status = TD_CTRL_ACTIVE | TD_CTRL_IOS;
	destination = (urb->pipe & PIPE_DEVEP_MASK) | usb_packetid(urb->pipe);

	ret = isochronous_find_start(uhci, urb);
	if (ret)
		return ret;

	frame = urb->start_frame;
	for (i = 0; i < urb->number_of_packets; i++, frame += urb->interval) {
		if (!urb->iso_frame_desc[i].length)
			continue;

		td = uhci_alloc_td(uhci, urb->dev);
		if (!td)
			return -ENOMEM;

		uhci_add_td_to_urb(urb, td);
		uhci_fill_td(td, status, destination | uhci_explen(urb->iso_frame_desc[i].length - 1),
			urb->transfer_dma + urb->iso_frame_desc[i].offset);

		if (i + 1 >= urb->number_of_packets)
			td->status |= cpu_to_le32(TD_CTRL_IOC);

		uhci_insert_td_frame_list(uhci, td, frame);
	}

	return -EINPROGRESS;
}

static int uhci_result_isochronous(struct uhci_hcd *uhci, struct urb *urb)
{
	struct list_head *tmp, *head;
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;
	int status;
	int i, ret = 0;

	urb->actual_length = 0;

	i = 0;
	head = &urbp->td_list;
	tmp = head->next;
	while (tmp != head) {
		struct uhci_td *td = list_entry(tmp, struct uhci_td, list);
		int actlength;

		tmp = tmp->next;

		if (td_status(td) & TD_CTRL_ACTIVE)
			return -EINPROGRESS;

		actlength = uhci_actual_length(td_status(td));
		urb->iso_frame_desc[i].actual_length = actlength;
		urb->actual_length += actlength;

		status = uhci_map_status(uhci_status_bits(td_status(td)), usb_pipeout(urb->pipe));
		urb->iso_frame_desc[i].status = status;
		if (status) {
			urb->error_count++;
			ret = status;
		}

		i++;
	}

	return ret;
}

/*
 * MUST be called with uhci->urb_list_lock acquired
 */
static struct urb *uhci_find_urb_ep(struct uhci_hcd *uhci, struct urb *urb)
{
	struct list_head *tmp, *head;

	/* We don't match Isoc transfers since they are special */
	if (usb_pipeisoc(urb->pipe))
		return NULL;

	head = &uhci->urb_list;
	tmp = head->next;
	while (tmp != head) {
		struct urb_priv *up = list_entry(tmp, struct urb_priv, urb_list);
		struct urb *u = up->urb;

		tmp = tmp->next;

		if (u->dev == urb->dev && u->status == -EINPROGRESS) {
			/* For control, ignore the direction */
			if (usb_pipecontrol(urb->pipe) &&
			    (u->pipe & ~USB_DIR_IN) == (urb->pipe & ~USB_DIR_IN))
				return u;
			else if (u->pipe == urb->pipe)
				return u;
		}
	}

	return NULL;
}

static int uhci_urb_enqueue(struct usb_hcd *hcd, struct urb *urb, int mem_flags)
{
	int ret = -EINVAL;
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	unsigned long flags;
	struct urb *eurb;
	int bustime;

	spin_lock_irqsave(&uhci->urb_list_lock, flags);

	eurb = uhci_find_urb_ep(uhci, urb);

	if (!uhci_alloc_urb_priv(uhci, urb)) {
		spin_unlock_irqrestore(&uhci->urb_list_lock, flags);
		return -ENOMEM;
	}

	switch (usb_pipetype(urb->pipe)) {
	case PIPE_CONTROL:
		ret = uhci_submit_control(uhci, urb, eurb);
		break;
	case PIPE_INTERRUPT:
		if (!eurb) {
			bustime = usb_check_bandwidth(urb->dev, urb);
			if (bustime < 0)
				ret = bustime;
			else {
				ret = uhci_submit_interrupt(uhci, urb, eurb);
				if (ret == -EINPROGRESS)
					usb_claim_bandwidth(urb->dev, urb, bustime, 0);
			}
		} else {	/* inherit from parent */
			urb->bandwidth = eurb->bandwidth;
			ret = uhci_submit_interrupt(uhci, urb, eurb);
		}
		break;
	case PIPE_BULK:
		ret = uhci_submit_bulk(uhci, urb, eurb);
		break;
	case PIPE_ISOCHRONOUS:
		bustime = usb_check_bandwidth(urb->dev, urb);
		if (bustime < 0) {
			ret = bustime;
			break;
		}

		ret = uhci_submit_isochronous(uhci, urb);
		if (ret == -EINPROGRESS)
			usb_claim_bandwidth(urb->dev, urb, bustime, 1);
		break;
	}

	if (ret != -EINPROGRESS) {
		/* Submit failed, so delete it from the urb_list */
		struct urb_priv *urbp = urb->hcpriv;

		list_del_init(&urbp->urb_list);
		spin_unlock_irqrestore(&uhci->urb_list_lock, flags);
		uhci_destroy_urb_priv (uhci, urb);

		return ret;
	}

	spin_unlock_irqrestore(&uhci->urb_list_lock, flags);

	return 0;
}

/*
 * Return the result of a transfer
 *
 * MUST be called with urb_list_lock acquired
 */
static void uhci_transfer_result(struct uhci_hcd *uhci, struct urb *urb)
{
	int ret = -EINVAL;
	unsigned long flags;
	struct urb_priv *urbp;

	spin_lock_irqsave(&urb->lock, flags);

	urbp = (struct urb_priv *)urb->hcpriv;

	if (urb->status != -EINPROGRESS) {
		info("uhci_transfer_result: called for URB %p not in flight?", urb);
		goto out;
	}

	switch (usb_pipetype(urb->pipe)) {
	case PIPE_CONTROL:
		ret = uhci_result_control(uhci, urb);
		break;
	case PIPE_INTERRUPT:
		ret = uhci_result_interrupt(uhci, urb);
		break;
	case PIPE_BULK:
		ret = uhci_result_bulk(uhci, urb);
		break;
	case PIPE_ISOCHRONOUS:
		ret = uhci_result_isochronous(uhci, urb);
		break;
	}

	urbp->status = ret;

	if (ret == -EINPROGRESS)
		goto out;

	switch (usb_pipetype(urb->pipe)) {
	case PIPE_CONTROL:
	case PIPE_BULK:
	case PIPE_ISOCHRONOUS:
		/* Release bandwidth for Interrupt or Isoc. transfers */
		/* Spinlock needed ? */
		if (urb->bandwidth)
			usb_release_bandwidth(urb->dev, urb, 1);
		uhci_unlink_generic(uhci, urb);
		break;
	case PIPE_INTERRUPT:
		/* Release bandwidth for Interrupt or Isoc. transfers */
		/* Make sure we don't release if we have a queued URB */
		spin_lock(&uhci->frame_list_lock);
		/* Spinlock needed ? */
		if (list_empty(&urbp->queue_list) && urb->bandwidth)
			usb_release_bandwidth(urb->dev, urb, 0);
		else
			/* bandwidth was passed on to queued URB, */
			/* so don't let usb_unlink_urb() release it */
			urb->bandwidth = 0;
		spin_unlock(&uhci->frame_list_lock);
		uhci_unlink_generic(uhci, urb);
		break;
	default:
		info("uhci_transfer_result: unknown pipe type %d for urb %p\n",
			usb_pipetype(urb->pipe), urb);
	}

	/* Remove it from uhci->urb_list */
	list_del_init(&urbp->urb_list);

	uhci_add_complete(uhci, urb);

out:
	spin_unlock_irqrestore(&urb->lock, flags);
}

/*
 * MUST be called with urb->lock acquired
 */
static void uhci_unlink_generic(struct uhci_hcd *uhci, struct urb *urb)
{
	struct list_head *head, *tmp;
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;
	int prevactive = 1;

	/* We can get called when urbp allocation fails, so check */
	if (!urbp)
		return;

	uhci_dec_fsbr(uhci, urb);	/* Safe since it checks */

	/*
	 * Now we need to find out what the last successful toggle was
	 * so we can update the local data toggle for the next transfer
	 *
	 * There's 3 way's the last successful completed TD is found:
	 *
	 * 1) The TD is NOT active and the actual length < expected length
	 * 2) The TD is NOT active and it's the last TD in the chain
	 * 3) The TD is active and the previous TD is NOT active
	 *
	 * Control and Isochronous ignore the toggle, so this is safe
	 * for all types
	 */
	head = &urbp->td_list;
	tmp = head->next;
	while (tmp != head) {
		struct uhci_td *td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		if (!(td_status(td) & TD_CTRL_ACTIVE) &&
		    (uhci_actual_length(td_status(td)) < uhci_expected_length(td_token(td)) ||
		    tmp == head))
			usb_settoggle(urb->dev, uhci_endpoint(td_token(td)),
				uhci_packetout(td_token(td)),
				uhci_toggle(td_token(td)) ^ 1);
		else if ((td_status(td) & TD_CTRL_ACTIVE) && !prevactive)
			usb_settoggle(urb->dev, uhci_endpoint(td_token(td)),
				uhci_packetout(td_token(td)),
				uhci_toggle(td_token(td)));

		prevactive = td_status(td) & TD_CTRL_ACTIVE;
	}

	uhci_delete_queued_urb(uhci, urb);

	/* The interrupt loop will reclaim the QH's */
	uhci_remove_qh(uhci, urbp->qh);
	urbp->qh = NULL;
}

static int uhci_urb_dequeue(struct usb_hcd *hcd, struct urb *urb)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	unsigned long flags;
	struct urb_priv *urbp = urb->hcpriv;

	/* If this is an interrupt URB that is being killed in urb->complete, */
	/* then just set its status and return */
	if (!urbp) {
	  urb->status = -ECONNRESET;
	  return 0;
	}

	spin_lock_irqsave(&uhci->urb_list_lock, flags);

	list_del_init(&urbp->urb_list);

	uhci_unlink_generic(uhci, urb);

	spin_lock(&uhci->urb_remove_list_lock);

	/* If we're the first, set the next interrupt bit */
	if (list_empty(&uhci->urb_remove_list))
		uhci_set_next_interrupt(uhci);
	list_add(&urbp->urb_list, &uhci->urb_remove_list);

	spin_unlock(&uhci->urb_remove_list_lock);
	spin_unlock_irqrestore(&uhci->urb_list_lock, flags);
	return 0;
}

static int uhci_fsbr_timeout(struct uhci_hcd *uhci, struct urb *urb)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;
	struct list_head *head, *tmp;
	int count = 0;

	uhci_dec_fsbr(uhci, urb);

	urbp->fsbr_timeout = 1;

	/*
	 * Ideally we would want to fix qh->element as well, but it's
	 * read/write by the HC, so that can introduce a race. It's not
	 * really worth the hassle
	 */

	head = &urbp->td_list;
	tmp = head->next;
	while (tmp != head) {
		struct uhci_td *td = list_entry(tmp, struct uhci_td, list);

		tmp = tmp->next;

		/*
		 * Make sure we don't do the last one (since it'll have the
		 * TERM bit set) as well as we skip every so many TD's to
		 * make sure it doesn't hog the bandwidth
		 */
		if (tmp != head && (count % DEPTH_INTERVAL) == (DEPTH_INTERVAL - 1))
			td->link |= UHCI_PTR_DEPTH;

		count++;
	}

	return 0;
}

/*
 * uhci_get_current_frame_number()
 *
 * returns the current frame number for a USB bus/controller.
 */
static int uhci_get_current_frame_number(struct uhci_hcd *uhci)
{
	return inw(uhci->io_addr + USBFRNUM);
}

static int init_stall_timer(struct usb_hcd *hcd);

static void stall_callback(unsigned long ptr)
{
	struct usb_hcd *hcd = (struct usb_hcd *)ptr;
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	struct list_head list, *tmp, *head;
	unsigned long flags;

	INIT_LIST_HEAD(&list);

	spin_lock_irqsave(&uhci->urb_list_lock, flags);
	head = &uhci->urb_list;
	tmp = head->next;
	while (tmp != head) {
		struct urb_priv *up = list_entry(tmp, struct urb_priv, urb_list);
		struct urb *u = up->urb;

		tmp = tmp->next;

		spin_lock(&u->lock);

		/* Check if the FSBR timed out */
		if (up->fsbr && !up->fsbr_timeout && time_after_eq(jiffies, up->fsbrtime + IDLE_TIMEOUT))
			uhci_fsbr_timeout(uhci, u);

		/* Check if the URB timed out */
		if (u->timeout && time_after_eq(jiffies, up->inserttime + u->timeout))
			list_move_tail(&up->urb_list, &list);

		spin_unlock(&u->lock);
	}
	spin_unlock_irqrestore(&uhci->urb_list_lock, flags);

	head = &list;
	tmp = head->next;
	while (tmp != head) {
		struct urb_priv *up = list_entry(tmp, struct urb_priv, urb_list);
		struct urb *u = up->urb;

		tmp = tmp->next;

		uhci_urb_dequeue(hcd, u);
	}

	/* Really disable FSBR */
	if (!uhci->fsbr && uhci->fsbrtimeout && time_after_eq(jiffies, uhci->fsbrtimeout)) {
		uhci->fsbrtimeout = 0;
		uhci->skel_term_qh->link = UHCI_PTR_TERM;
	}

	/* Poll for and perform state transitions */
	hc_state_transitions(uhci);

	init_stall_timer(hcd);
}

static int init_stall_timer(struct usb_hcd *hcd)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);

	init_timer(&uhci->stall_timer);
	uhci->stall_timer.function = stall_callback;
	uhci->stall_timer.data = (unsigned long)hcd;
	uhci->stall_timer.expires = jiffies + (HZ / 10);
	add_timer(&uhci->stall_timer);

	return 0;
}

static void uhci_free_pending_qhs(struct uhci_hcd *uhci)
{
	struct list_head *tmp, *head;
	unsigned long flags;

	spin_lock_irqsave(&uhci->qh_remove_list_lock, flags);
	head = &uhci->qh_remove_list;
	tmp = head->next;
	while (tmp != head) {
		struct uhci_qh *qh = list_entry(tmp, struct uhci_qh, remove_list);

		tmp = tmp->next;

		list_del_init(&qh->remove_list);

		uhci_free_qh(uhci, qh);
	}
	spin_unlock_irqrestore(&uhci->qh_remove_list_lock, flags);
}

static void uhci_finish_urb(struct usb_hcd *hcd, struct urb *urb, struct pt_regs *regs)
{
	struct urb_priv *urbp = (struct urb_priv *)urb->hcpriv;
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	int status;
	unsigned long flags;

	spin_lock_irqsave(&urb->lock, flags);
	status = urbp->status;
	uhci_destroy_urb_priv(uhci, urb);

 	if (urb->status != -ENOENT && urb->status != -ECONNRESET)
		urb->status = status;
	spin_unlock_irqrestore(&urb->lock, flags);

	usb_hcd_giveback_urb(hcd, urb, regs);
}

static void uhci_finish_completion(struct usb_hcd *hcd, struct pt_regs *regs)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	struct list_head *tmp, *head;
	unsigned long flags;

	spin_lock_irqsave(&uhci->complete_list_lock, flags);
	head = &uhci->complete_list;
	tmp = head->next;
	while (tmp != head) {
		struct urb_priv *urbp = list_entry(tmp, struct urb_priv, complete_list);
		struct urb *urb = urbp->urb;

		list_del_init(&urbp->complete_list);
		spin_unlock_irqrestore(&uhci->complete_list_lock, flags);

		uhci_finish_urb(hcd, urb, regs);

		spin_lock_irqsave(&uhci->complete_list_lock, flags);
		head = &uhci->complete_list;
		tmp = head->next;
	}
	spin_unlock_irqrestore(&uhci->complete_list_lock, flags);
}

static void uhci_remove_pending_qhs(struct uhci_hcd *uhci)
{
	struct list_head *tmp, *head;
	unsigned long flags;

	spin_lock_irqsave(&uhci->urb_remove_list_lock, flags);
	head = &uhci->urb_remove_list;
	tmp = head->next;
	while (tmp != head) {
		struct urb_priv *urbp = list_entry(tmp, struct urb_priv, urb_list);
		struct urb *urb = urbp->urb;

		tmp = tmp->next;

		list_del_init(&urbp->urb_list);

		urbp->status = urb->status = -ECONNRESET;

		uhci_add_complete(uhci, urb);
	}
	spin_unlock_irqrestore(&uhci->urb_remove_list_lock, flags);
}

static int uhci_irq(struct usb_hcd *hcd, struct pt_regs *regs)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	unsigned int io_addr = uhci->io_addr;
	unsigned short status;
	struct list_head *tmp, *head;

	/*
	 * Read the interrupt status, and write it back to clear the
	 * interrupt cause
	 */
	status = inw(io_addr + USBSTS);
	if (!status)	/* shared interrupt, not mine */
		return 0;
	outw(status, io_addr + USBSTS);		/* Clear it */

	if (status & ~(USBSTS_USBINT | USBSTS_ERROR | USBSTS_RD)) {
		if (status & USBSTS_HSE)
			err("%x: host system error, PCI problems?", io_addr);
		if (status & USBSTS_HCPE)
			err("%x: host controller process error. something bad happened", io_addr);
		if ((status & USBSTS_HCH) && uhci->state > 0) {
			err("%x: host controller halted. very bad", io_addr);
			/* FIXME: Reset the controller, fix the offending TD */
		}
	}

	if (status & USBSTS_RD)
		uhci->resume_detect = 1;

	uhci_free_pending_qhs(uhci);

	uhci_remove_pending_qhs(uhci);

	uhci_clear_next_interrupt(uhci);

	/* Walk the list of pending URB's to see which ones completed */
	spin_lock(&uhci->urb_list_lock);
	head = &uhci->urb_list;
	tmp = head->next;
	while (tmp != head) {
		struct urb_priv *urbp = list_entry(tmp, struct urb_priv, urb_list);
		struct urb *urb = urbp->urb;

		tmp = tmp->next;

		/* Checks the status and does all of the magic necessary */
		uhci_transfer_result(uhci, urb);
	}
	spin_unlock(&uhci->urb_list_lock);

	uhci_finish_completion(hcd, regs);
	
	return 0;
}

static void reset_hc(struct uhci_hcd *uhci)
{
	unsigned int io_addr = uhci->io_addr;

	/* Global reset for 50ms */
	uhci->state = UHCI_RESET;
	outw(USBCMD_GRESET, io_addr + USBCMD);
	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout((HZ*50+999) / 1000);
	set_current_state(TASK_RUNNING);
	outw(0, io_addr + USBCMD);

	/* Another 10ms delay */
	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout((HZ*10+999) / 1000);
	set_current_state(TASK_RUNNING);
	uhci->resume_detect = 0;
}

static void suspend_hc(struct uhci_hcd *uhci)
{
	unsigned int io_addr = uhci->io_addr;

	dbg("%x: suspend_hc", io_addr);
	uhci->state = UHCI_SUSPENDED;
	uhci->resume_detect = 0;
	outw(USBCMD_EGSM, io_addr + USBCMD);
}

static void wakeup_hc(struct uhci_hcd *uhci)
{
	unsigned int io_addr = uhci->io_addr;

	switch (uhci->state) {
		case UHCI_SUSPENDED:		/* Start the resume */
			dbg("%x: wakeup_hc", io_addr);

			/* Global resume for >= 20ms */
			outw(USBCMD_FGR | USBCMD_EGSM, io_addr + USBCMD);
			uhci->state = UHCI_RESUMING_1;
			uhci->state_end = jiffies + (20*HZ+999) / 1000;
			break;

		case UHCI_RESUMING_1:		/* End global resume */
			uhci->state = UHCI_RESUMING_2;
			outw(0, io_addr + USBCMD);
			/* Falls through */

		case UHCI_RESUMING_2:		/* Wait for EOP to be sent */
			if (inw(io_addr + USBCMD) & USBCMD_FGR)
				break;

			/* Run for at least 1 second, and
			 * mark it configured with a 64-byte max packet */
			uhci->state = UHCI_RUNNING_GRACE;
			uhci->state_end = jiffies + HZ;
			outw(USBCMD_RS | USBCMD_CF | USBCMD_MAXP,
					io_addr + USBCMD);
			break;

		case UHCI_RUNNING_GRACE:	/* Now allowed to suspend */
			uhci->state = UHCI_RUNNING;
			break;

		default:
			break;
	}
}

static int ports_active(struct uhci_hcd *uhci)
{
	unsigned int io_addr = uhci->io_addr;
	int connection = 0;
	int i;

	for (i = 0; i < uhci->rh_numports; i++)
		connection |= (inw(io_addr + USBPORTSC1 + i * 2) & USBPORTSC_CCS);

	return connection;
}

static int suspend_allowed(struct uhci_hcd *uhci)
{
	unsigned int io_addr = uhci->io_addr;
	int i;

	if (!uhci->hcd.pdev ||
	     uhci->hcd.pdev->vendor != PCI_VENDOR_ID_INTEL ||
	     uhci->hcd.pdev->device != PCI_DEVICE_ID_INTEL_82371AB_2)
		return 1;

	/* This is a 82371AB/EB/MB USB controller which has a bug that
	 * causes false resume indications if any port has an
	 * over current condition.  To prevent problems, we will not
	 * allow a global suspend if any ports are OC.
	 *
	 * Some motherboards using the 82371AB/EB/MB (but not the USB portion)
	 * appear to hardwire the over current inputs active to disable
	 * the USB ports.
	 */

	/* check for over current condition on any port */
	for (i = 0; i < uhci->rh_numports; i++) {
		if (inw(io_addr + USBPORTSC1 + i * 2) & USBPORTSC_OC)
			return 0;
	}

	return 1;
}

static void hc_state_transitions(struct uhci_hcd *uhci)
{
	switch (uhci->state) {
		case UHCI_RUNNING:

			/* global suspend if nothing connected for 1 second */
			if (!ports_active(uhci) && suspend_allowed(uhci)) {
				uhci->state = UHCI_SUSPENDING_GRACE;
				uhci->state_end = jiffies + HZ;
			}
			break;

		case UHCI_SUSPENDING_GRACE:
			if (ports_active(uhci))
				uhci->state = UHCI_RUNNING;
			else if (time_after_eq(jiffies, uhci->state_end))
				suspend_hc(uhci);
			break;

		case UHCI_SUSPENDED:

			/* wakeup if requested by a device */
			if (uhci->resume_detect)
				wakeup_hc(uhci);
			break;

		case UHCI_RESUMING_1:
		case UHCI_RESUMING_2:
		case UHCI_RUNNING_GRACE:
			if (time_after_eq(jiffies, uhci->state_end))
				wakeup_hc(uhci);
			break;

		default:
			break;
	}
}

static void start_hc(struct uhci_hcd *uhci)
{
	unsigned int io_addr = uhci->io_addr;
	int timeout = 1000;

	/*
	 * Reset the HC - this will force us to get a
	 * new notification of any already connected
	 * ports due to the virtual disconnect that it
	 * implies.
	 */
	outw(USBCMD_HCRESET, io_addr + USBCMD);
	while (inw(io_addr + USBCMD) & USBCMD_HCRESET) {
		if (!--timeout) {
			printk(KERN_ERR "uhci: USBCMD_HCRESET timed out!\n");
			break;
		}
	}

	/* Turn on all interrupts */
	outw(USBINTR_TIMEOUT | USBINTR_RESUME | USBINTR_IOC | USBINTR_SP,
		io_addr + USBINTR);

	/* Start at frame 0 */
	outw(0, io_addr + USBFRNUM);
	outl(uhci->fl->dma_handle, io_addr + USBFLBASEADD);

	/* Run and mark it configured with a 64-byte max packet */
	uhci->state = UHCI_RUNNING_GRACE;
	uhci->state_end = jiffies + HZ;
	outw(USBCMD_RS | USBCMD_CF | USBCMD_MAXP, io_addr + USBCMD);

        uhci->hcd.state = USB_STATE_READY;
}

/*
 * De-allocate all resources..
 */
static void release_uhci(struct uhci_hcd *uhci)
{
	int i;

	for (i = 0; i < UHCI_NUM_SKELQH; i++)
		if (uhci->skelqh[i]) {
			uhci_free_qh(uhci, uhci->skelqh[i]);
			uhci->skelqh[i] = NULL;
		}

	if (uhci->term_td) {
		uhci_free_td(uhci, uhci->term_td);
		uhci->term_td = NULL;
	}

	if (uhci->qh_pool) {
		pci_pool_destroy(uhci->qh_pool);
		uhci->qh_pool = NULL;
	}

	if (uhci->td_pool) {
		pci_pool_destroy(uhci->td_pool);
		uhci->td_pool = NULL;
	}

	if (uhci->fl) {
		pci_free_consistent(uhci->hcd.pdev, sizeof(*uhci->fl), uhci->fl, uhci->fl->dma_handle);
		uhci->fl = NULL;
	}

#ifdef CONFIG_PROC_FS
	if (uhci->proc_entry) {
		remove_proc_entry(uhci->hcd.self.bus_name, uhci_proc_root);
		uhci->proc_entry = NULL;
	}
#endif
}

/*
 * Allocate a frame list, and then setup the skeleton
 *
 * The hardware doesn't really know any difference
 * in the queues, but the order does matter for the
 * protocols higher up. The order is:
 *
 *  - any isochronous events handled before any
 *    of the queues. We don't do that here, because
 *    we'll create the actual TD entries on demand.
 *  - The first queue is the interrupt queue.
 *  - The second queue is the control queue, split into low and high speed
 *  - The third queue is bulk queue.
 *  - The fourth queue is the bandwidth reclamation queue, which loops back
 *    to the high speed control queue.
 */
static int __devinit uhci_start(struct usb_hcd *hcd)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	int retval = -EBUSY;
	int i, port;
	unsigned io_size;
	dma_addr_t dma_handle;
	struct usb_device *udev;
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *ent;
#endif

	uhci->io_addr = (unsigned long) hcd->regs;
	io_size = pci_resource_len(hcd->pdev, hcd->region);

#ifdef CONFIG_PROC_FS
	ent = create_proc_entry(hcd->self.bus_name, S_IFREG|S_IRUGO|S_IWUSR, uhci_proc_root);
	if (!ent) {
		err("couldn't create uhci proc entry");
		retval = -ENOMEM;
		goto err_create_proc_entry;
	}

	ent->data = uhci;
	ent->proc_fops = &uhci_proc_operations;
	ent->size = 0;
	uhci->proc_entry = ent;
#endif

	/* Reset here so we don't get any interrupts from an old setup */
	/*  or broken setup */
	reset_hc(uhci);

	uhci->fsbr = 0;
	uhci->fsbrtimeout = 0;

	spin_lock_init(&uhci->qh_remove_list_lock);
	INIT_LIST_HEAD(&uhci->qh_remove_list);

	spin_lock_init(&uhci->urb_remove_list_lock);
	INIT_LIST_HEAD(&uhci->urb_remove_list);

	spin_lock_init(&uhci->urb_list_lock);
	INIT_LIST_HEAD(&uhci->urb_list);

	spin_lock_init(&uhci->complete_list_lock);
	INIT_LIST_HEAD(&uhci->complete_list);

	spin_lock_init(&uhci->frame_list_lock);

	uhci->fl = pci_alloc_consistent(hcd->pdev, sizeof(*uhci->fl), &dma_handle);
	if (!uhci->fl) {
		err("unable to allocate consistent memory for frame list");
		goto err_alloc_fl;
	}

	memset((void *)uhci->fl, 0, sizeof(*uhci->fl));

	uhci->fl->dma_handle = dma_handle;

	uhci->td_pool = pci_pool_create("uhci_td", hcd->pdev,
		sizeof(struct uhci_td), 16, 0);
	if (!uhci->td_pool) {
		err("unable to create td pci_pool");
		goto err_create_td_pool;
	}

	uhci->qh_pool = pci_pool_create("uhci_qh", hcd->pdev,
		sizeof(struct uhci_qh), 16, 0);
	if (!uhci->qh_pool) {
		err("unable to create qh pci_pool");
		goto err_create_qh_pool;
	}

	/* Initialize the root hub */

	/* UHCI specs says devices must have 2 ports, but goes on to say */
	/*  they may have more but give no way to determine how many they */
	/*  have. However, according to the UHCI spec, Bit 7 is always set */
	/*  to 1. So we try to use this to our advantage */
	for (port = 0; port < (io_size - 0x10) / 2; port++) {
		unsigned int portstatus;

		portstatus = inw(uhci->io_addr + 0x10 + (port * 2));
		if (!(portstatus & 0x0080))
			break;
	}
	if (debug)
		info("detected %d ports", port);

	/* This is experimental so anything less than 2 or greater than 8 is */
	/*  something weird and we'll ignore it */
	if (port < 2 || port > 8) {
		info("port count misdetected? forcing to 2 ports");
		port = 2;
	}

	uhci->rh_numports = port;

	hcd->self.root_hub = udev = usb_alloc_dev(NULL, &hcd->self);
	if (!udev) {
		err("unable to allocate root hub");
		goto err_alloc_root_hub;
	}
	hcd->pdev->bus = (struct pci_bus  *)udev; /* Fix bus pointer for initial device */

	uhci->term_td = uhci_alloc_td(uhci, udev);
	if (!uhci->term_td) {
		err("unable to allocate terminating TD");
		goto err_alloc_term_td;
	}

	for (i = 0; i < UHCI_NUM_SKELQH; i++) {
		uhci->skelqh[i] = uhci_alloc_qh(uhci, udev);
		if (!uhci->skelqh[i]) {
			err("unable to allocate QH %d", i);
			goto err_alloc_skelqh;
		}
	}

	/*
	 * 8 Interrupt queues; link int2 to int1, int4 to int2, etc
	 * then link int1 to control and control to bulk
	 */
	uhci->skel_int128_qh->link = cpu_to_le32(uhci->skel_int64_qh->dma_handle) | UHCI_PTR_QH;
	uhci->skel_int64_qh->link = cpu_to_le32(uhci->skel_int32_qh->dma_handle) | UHCI_PTR_QH;
	uhci->skel_int32_qh->link = cpu_to_le32(uhci->skel_int16_qh->dma_handle) | UHCI_PTR_QH;
	uhci->skel_int16_qh->link = cpu_to_le32(uhci->skel_int8_qh->dma_handle) | UHCI_PTR_QH;
	uhci->skel_int8_qh->link = cpu_to_le32(uhci->skel_int4_qh->dma_handle) | UHCI_PTR_QH;
	uhci->skel_int4_qh->link = cpu_to_le32(uhci->skel_int2_qh->dma_handle) | UHCI_PTR_QH;
	uhci->skel_int2_qh->link = cpu_to_le32(uhci->skel_int1_qh->dma_handle) | UHCI_PTR_QH;
	uhci->skel_int1_qh->link = cpu_to_le32(uhci->skel_ls_control_qh->dma_handle) | UHCI_PTR_QH;

	uhci->skel_ls_control_qh->link = cpu_to_le32(uhci->skel_hs_control_qh->dma_handle) | UHCI_PTR_QH;
	uhci->skel_hs_control_qh->link = cpu_to_le32(uhci->skel_bulk_qh->dma_handle) | UHCI_PTR_QH;
	uhci->skel_bulk_qh->link = cpu_to_le32(uhci->skel_term_qh->dma_handle) | UHCI_PTR_QH;

	/* This dummy TD is to work around a bug in Intel PIIX controllers */
	uhci_fill_td(uhci->term_td, 0, (UHCI_NULL_DATA_SIZE << 21) |
		(0x7f << TD_TOKEN_DEVADDR_SHIFT) | USB_PID_IN, 0);
	uhci->term_td->link = cpu_to_le32(uhci->term_td->dma_handle);

	uhci->skel_term_qh->link = UHCI_PTR_TERM;
	uhci->skel_term_qh->element = cpu_to_le32(uhci->term_td->dma_handle);

	/*
	 * Fill the frame list: make all entries point to
	 * the proper interrupt queue.
	 *
	 * This is probably silly, but it's a simple way to
	 * scatter the interrupt queues in a way that gives
	 * us a reasonable dynamic range for irq latencies.
	 */
	for (i = 0; i < UHCI_NUMFRAMES; i++) {
		int irq = 0;

		if (i & 1) {
			irq++;
			if (i & 2) {
				irq++;
				if (i & 4) {
					irq++;
					if (i & 8) {
						irq++;
						if (i & 16) {
							irq++;
							if (i & 32) {
								irq++;
								if (i & 64)
									irq++;
							}
						}
					}
				}
			}
		}

		/* Only place we don't use the frame list routines */
		uhci->fl->frame[i] = cpu_to_le32(uhci->skelqh[7 - irq]->dma_handle);
	}

	start_hc(uhci);

	init_stall_timer(hcd);

	/* disable legacy emulation */
	pci_write_config_word(hcd->pdev, USBLEGSUP, USBLEGSUP_DEFAULT);

	usb_connect(udev);
	udev->speed = USB_SPEED_FULL;

	if (usb_register_root_hub(udev, &hcd->pdev->dev) != 0) {
		err("unable to start root hub");
		retval = -ENOMEM;
		goto err_start_root_hub;
	}

	return 0;

/*
 * error exits:
 */
err_start_root_hub:
	reset_hc(uhci);

	del_timer_sync(&uhci->stall_timer);

err_alloc_skelqh:
	for (i = 0; i < UHCI_NUM_SKELQH; i++)
		if (uhci->skelqh[i]) {
			uhci_free_qh(uhci, uhci->skelqh[i]);
			uhci->skelqh[i] = NULL;
		}

	uhci_free_td(uhci, uhci->term_td);
	uhci->term_td = NULL;

err_alloc_term_td:
	usb_put_dev(udev);
	hcd->self.root_hub = NULL;

err_alloc_root_hub:
	pci_pool_destroy(uhci->qh_pool);
	uhci->qh_pool = NULL;

err_create_qh_pool:
	pci_pool_destroy(uhci->td_pool);
	uhci->td_pool = NULL;

err_create_td_pool:
	pci_free_consistent(hcd->pdev, sizeof(*uhci->fl), uhci->fl, uhci->fl->dma_handle);
	uhci->fl = NULL;

err_alloc_fl:
#ifdef CONFIG_PROC_FS
	remove_proc_entry(hcd->self.bus_name, uhci_proc_root);
	uhci->proc_entry = NULL;

err_create_proc_entry:
#endif

	return retval;
}

static void uhci_stop(struct usb_hcd *hcd)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);

	del_timer_sync(&uhci->stall_timer);

	/*
	 * At this point, we're guaranteed that no new connects can be made
	 * to this bus since there are no more parents
	 */
	uhci_free_pending_qhs(uhci);
	uhci_remove_pending_qhs(uhci);

	reset_hc(uhci);

	uhci_free_pending_qhs(uhci);

	release_uhci(uhci);
}

#ifdef CONFIG_PM
static int uhci_suspend(struct usb_hcd *hcd, u32 state)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);

	/* Don't try to suspend broken motherboards, reset instead */
	if (suspend_allowed(uhci))
		suspend_hc(uhci);
	else
		reset_hc(uhci);
	return 0;
}

static int uhci_resume(struct usb_hcd *hcd)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);

	pci_set_master(uhci->hcd.pdev);

	if (uhci->state == UHCI_SUSPENDED)
		uhci->resume_detect = 1;
	else {
		reset_hc(uhci);
		start_hc(uhci);
	}
	uhci->hcd.state = USB_STATE_READY;
	return 0;
}
#endif

static struct usb_hcd *uhci_hcd_alloc(void)
{
	struct uhci_hcd *uhci;

	uhci = (struct uhci_hcd *)kmalloc(sizeof(*uhci), GFP_KERNEL);
	if (!uhci)
		return NULL;

	memset(uhci, 0, sizeof(*uhci));
	return &uhci->hcd;
}

static void uhci_hcd_free(struct usb_hcd *hcd)
{
	kfree(hcd_to_uhci(hcd));
}

static int uhci_hcd_get_frame_number(struct usb_hcd *hcd)
{
	return uhci_get_current_frame_number(hcd_to_uhci(hcd));
}

static const char hcd_name[] = "uhci-hcd";

static struct hc_driver uhci_driver = {
	.description =		hcd_name,

	/* Generic hardware linkage */
	.irq =			uhci_irq,
	.flags =		HCD_USB11,

	/* Basic lifecycle operations */
	.start =		uhci_start,
#ifdef CONFIG_PM
	.suspend =		uhci_suspend,
	.resume =		uhci_resume,
#endif
	.stop =			uhci_stop,

	.hcd_alloc =		uhci_hcd_alloc,
	.hcd_free =		uhci_hcd_free,

	.urb_enqueue =		uhci_urb_enqueue,
	.urb_dequeue =		uhci_urb_dequeue,

	.get_frame_number =	uhci_hcd_get_frame_number,

	.hub_status_data =	uhci_hub_status_data,
	.hub_control =		uhci_hub_control,
};

const struct pci_device_id __devinitdata uhci_pci_ids[] = { {

	/* handle any USB UHCI controller */
	.class = 		((PCI_CLASS_SERIAL_USB << 8) | 0x00),
	.class_mask = 	~0,
	.driver_data =	(unsigned long) &uhci_driver,

	/* no matter who makes it */
	.vendor =	PCI_ANY_ID,
	.device =	PCI_ANY_ID,
	.subvendor =	PCI_ANY_ID,
	.subdevice =	PCI_ANY_ID,

	}, { /* end: all zeroes */ }
};

MODULE_DEVICE_TABLE(pci, uhci_pci_ids);

struct pci_driver uhci_pci_driver = {
	.name =		(char *)hcd_name,
	.id_table =	uhci_pci_ids,

	.probe =	usb_hcd_pci_probe,
	.remove =	usb_hcd_pci_remove,

#ifdef	CONFIG_PM
	.suspend =	usb_hcd_pci_suspend,
	.resume =	usb_hcd_pci_resume,
#endif	/* PM */
};

int __init uhci_hcd_init(void)
{
	int retval = -ENOMEM;

	info(DRIVER_DESC " " DRIVER_VERSION);

	if (usb_disabled())
		return -ENODEV;

	if (debug) {
		errbuf = kmalloc(ERRBUF_LEN, GFP_KERNEL);
		if (!errbuf)
			goto errbuf_failed;
	}

#ifdef CONFIG_PROC_FS
	uhci_proc_root = create_proc_entry("driver/uhci", S_IFDIR, 0);
	if (!uhci_proc_root)
		goto proc_failed;
#endif

	uhci_up_cachep = kmem_cache_create("uhci_urb_priv",
		sizeof(struct urb_priv), 0, 0, NULL, NULL);
	if (!uhci_up_cachep)
		goto up_failed;

	retval = pci_module_init(&uhci_pci_driver);
	if (retval)
		goto init_failed;

	return 0;

init_failed:
	if (kmem_cache_destroy(uhci_up_cachep))
		printk(KERN_INFO "uhci: not all urb_priv's were freed\n");

up_failed:

#ifdef CONFIG_PROC_FS
	remove_proc_entry("driver/uhci", 0);

proc_failed:
#endif
	if (errbuf)
		kfree(errbuf);

errbuf_failed:

	return retval;
}

void __exit uhci_hcd_cleanup(void)
{
	pci_unregister_driver(&uhci_pci_driver);

	if (kmem_cache_destroy(uhci_up_cachep))
		printk(KERN_INFO "uhci: not all urb_priv's were freed\n");

#ifdef CONFIG_PROC_FS
	remove_proc_entry("driver/uhci", 0);
#endif

	if (errbuf)
		kfree(errbuf);
}

/*module_init(uhci_hcd_init);*/
module_exit(uhci_hcd_cleanup);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

