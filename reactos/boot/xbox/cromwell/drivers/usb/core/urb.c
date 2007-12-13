#if 0
#include <linux/config.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/init.h>

#ifdef CONFIG_USB_DEBUG
	#define DEBUG
#else
	#undef DEBUG
#endif
#include <linux/usb.h>
#include "hcd.h"
#else
#include "../usb_wrapper.h"
#include "hcd.h"
#endif

/**
 * usb_init_urb - initializes a urb so that it can be used by a USB driver
 * @urb: pointer to the urb to initialize
 *
 * Initializes a urb so that the USB subsystem can use it properly.
 *
 * If a urb is created with a call to usb_alloc_urb() it is not
 * necessary to call this function.  Only use this if you allocate the
 * space for a struct urb on your own.  If you call this function, be
 * careful when freeing the memory for your urb that it is no longer in
 * use by the USB core.
 *
 * Only use this function if you _really_ understand what you are doing.
 */
void usb_init_urb(struct urb *urb)
{
	if (urb) {
		memset(urb, 0, sizeof(*urb));
		urb->count = (atomic_t)ATOMIC_INIT(1);
		spin_lock_init(&urb->lock);
	}
}

/**
 * usb_alloc_urb - creates a new urb for a USB driver to use
 * @iso_packets: number of iso packets for this urb
 * @mem_flags: the type of memory to allocate, see kmalloc() for a list of
 *	valid options for this.
 *
 * Creates an urb for the USB driver to use, initializes a few internal
 * structures, incrementes the usage counter, and returns a pointer to it.
 *
 * If no memory is available, NULL is returned.
 *
 * If the driver want to use this urb for interrupt, control, or bulk
 * endpoints, pass '0' as the number of iso packets.
 *
 * The driver must call usb_free_urb() when it is finished with the urb.
 */
struct urb *usb_alloc_urb(int iso_packets, int mem_flags)
{
	struct urb *urb;

	urb = (struct urb *)kmalloc(sizeof(struct urb) + 
		iso_packets * sizeof(struct usb_iso_packet_descriptor),
		mem_flags);
	if (!urb) {
		err("alloc_urb: kmalloc failed");
		return NULL;
	}
	usb_init_urb(urb);
	return urb;
}

/**
 * usb_free_urb - frees the memory used by a urb when all users of it are finished
 * @urb: pointer to the urb to free
 *
 * Must be called when a user of a urb is finished with it.  When the last user
 * of the urb calls this function, the memory of the urb is freed.
 *
 * Note: The transfer buffer associated with the urb is not freed, that must be
 * done elsewhere.
 */
void usb_free_urb(struct urb *urb)
{
	if (urb)
		if (atomic_dec_and_test(&urb->count))
		{
			kfree(urb);
		}
}

/**
 * usb_get_urb - increments the reference count of the urb
 * @urb: pointer to the urb to modify
 *
 * This must be  called whenever a urb is transferred from a device driver to a
 * host controller driver.  This allows proper reference counting to happen
 * for urbs.
 *
 * A pointer to the urb with the incremented reference counter is returned.
 */
struct urb * usb_get_urb(struct urb *urb)
{
	if (urb) {
		atomic_inc(&urb->count);
		return urb;
	} else
		return NULL;
}
		
		
/*-------------------------------------------------------------------*/

/**
 * usb_submit_urb - issue an asynchronous transfer request for an endpoint
 * @urb: pointer to the urb describing the request
 * @mem_flags: the type of memory to allocate, see kmalloc() for a list
 *	of valid options for this.
 *
 * This submits a transfer request, and transfers control of the URB
 * describing that request to the USB subsystem.  Request completion will
 * be indicated later, asynchronously, by calling the completion handler.
 * The three types of completion are success, error, and unlink
 * (also called "request cancellation").
 * URBs may be submitted in interrupt context.
 *
 * The caller must have correctly initialized the URB before submitting
 * it.  Functions such as usb_fill_bulk_urb() and usb_fill_control_urb() are
 * available to ensure that most fields are correctly initialized, for
 * the particular kind of transfer, although they will not initialize
 * any transfer flags.
 *
 * Successful submissions return 0; otherwise this routine returns a
 * negative error number.  If the submission is successful, the complete()
 * callback from the urb will be called exactly once, when the USB core and
 * host controller driver are finished with the urb.  When the completion
 * function is called, control of the URB is returned to the device
 * driver which issued the request.  The completion handler may then
 * immediately free or reuse that URB.
 *
 * For control endpoints, the synchronous usb_control_msg() call is
 * often used (in non-interrupt context) instead of this call.
 * That is often used through convenience wrappers, for the requests
 * that are standardized in the USB 2.0 specification.  For bulk
 * endpoints, a synchronous usb_bulk_msg() call is available.
 *
 * Request Queuing:
 *
 * URBs may be submitted to endpoints before previous ones complete, to
 * minimize the impact of interrupt latencies and system overhead on data
 * throughput.  This is required for continuous isochronous data streams,
 * and may also be required for some kinds of interrupt transfers. Such
 * queueing also maximizes bandwidth utilization by letting USB controllers
 * start work on later requests before driver software has finished the
 * completion processing for earlier requests.
 *
 * Bulk and Isochronous URBs may always be queued.  At this writing, all
 * mainstream host controller drivers support queueing for control and
 * interrupt transfer requests.
 *
 * Reserved Bandwidth Transfers:
 *
 * Periodic transfers (interrupt or isochronous) are performed repeatedly,
 * using the interval specified in the urb.  Submitting the first urb to
 * the endpoint reserves the bandwidth necessary to make those transfers.
 * If the USB subsystem can't allocate sufficient bandwidth to perform
 * the periodic request, submitting such a periodic request should fail.
 *
 * Device drivers must explicitly request that repetition, by ensuring that
 * some URB is always on the endpoint's queue (except possibly for short
 * periods during completion callacks).  When there is no longer an urb
 * queued, the endpoint's bandwidth reservation is canceled.  This means
 * drivers can use their completion handlers to ensure they keep bandwidth
 * they need, by reinitializing and resubmitting the just-completed urb
 * until the driver longer needs that periodic bandwidth.
 *
 * Memory Flags:
 *
 * The general rules for how to decide which mem_flags to use
 * are the same as for kmalloc.  There are four
 * different possible values; GFP_KERNEL, GFP_NOFS, GFP_NOIO and
 * GFP_ATOMIC.
 *
 * GFP_NOFS is not ever used, as it has not been implemented yet.
 *
 * GFP_ATOMIC is used when
 *   (a) you are inside a completion handler, an interrupt, bottom half,
 *       tasklet or timer, or
 *   (b) you are holding a spinlock or rwlock (does not apply to
 *       semaphores), or
 *   (c) current->state != TASK_RUNNING, this is the case only after
 *       you've changed it.
 * 
 * GFP_NOIO is used in the block io path and error handling of storage
 * devices.
 *
 * All other situations use GFP_KERNEL.
 *
 * Some more specific rules for mem_flags can be inferred, such as
 *  (1) start_xmit, timeout, and receive methods of network drivers must
 *      use GFP_ATOMIC (they are called with a spinlock held);
 *  (2) queuecommand methods of scsi drivers must use GFP_ATOMIC (also
 *      called with a spinlock held);
 *  (3) If you use a kernel thread with a network driver you must use
 *      GFP_NOIO, unless (b) or (c) apply;
 *  (4) after you have done a down() you can use GFP_KERNEL, unless (b) or (c)
 *      apply or your are in a storage driver's block io path;
 *  (5) USB probe and disconnect can use GFP_KERNEL unless (b) or (c) apply; and
 *  (6) changing firmware on a running storage or net device uses
 *      GFP_NOIO, unless b) or c) apply
 *
 */
int usb_submit_urb(struct urb *urb, int mem_flags)
{
	int			pipe, temp, max;
	struct usb_device	*dev;
	struct usb_operations	*op;
	int			is_out;
//	printk("sub dev %p bus %p num %i op %p sub %p\n",
//	       urb->dev, urb->dev->bus,urb->dev->devnum,urb->dev->bus->op, urb->dev->bus->op->submit_urb);
	if (!urb || urb->hcpriv || !urb->complete)
		return -EINVAL;
	if (!(dev = urb->dev) ||
	    (dev->state < USB_STATE_DEFAULT) ||
	    (!dev->bus) || (dev->devnum <= 0))
		return -ENODEV;
	if (!(op = dev->bus->op) || !op->submit_urb)
		return -ENODEV;

	urb->status = -EINPROGRESS;
	urb->actual_length = 0;
	urb->bandwidth = 0;

	/* Lots of sanity checks, so HCDs can rely on clean data
	 * and don't need to duplicate tests
	 */
	pipe = urb->pipe;
	temp = usb_pipetype (pipe);
	is_out = usb_pipeout (pipe);

	if (!usb_pipecontrol (pipe) && dev->state < USB_STATE_CONFIGURED)
		return -ENODEV;

	/* (actually HCDs may need to duplicate this, endpoint might yet
	 * stall due to queued bulk/intr transactions that complete after
	 * we check)
	 */
	if (usb_endpoint_halted (dev, usb_pipeendpoint (pipe), is_out))
		return -EPIPE;

	/* FIXME there should be a sharable lock protecting us against
	 * config/altsetting changes and disconnects, kicking in here.
	 * (here == before maxpacket, and eventually endpoint type,
	 * checks get made.)
	 */

	max = usb_maxpacket (dev, pipe, is_out);
	if (max <= 0) {
		dbg ("%s: bogus endpoint %d-%s on usb-%s-%s (bad maxpacket %d)",
			__FUNCTION__,
			usb_pipeendpoint (pipe), is_out ? "OUT" : "IN",
			dev->bus->bus_name, dev->devpath,
			max);
		return -EMSGSIZE;
	}

	/* periodic transfers limit size per frame/uframe,
	 * but drivers only control those sizes for ISO.
	 * while we're checking, initialize return status.
	 */
	if (temp == PIPE_ISOCHRONOUS) {
		int	n, len;

		/* "high bandwidth" mode, 1-3 packets/uframe? */
		if (dev->speed == USB_SPEED_HIGH) {
			int	mult = 1 + ((max >> 11) & 0x03);
			max &= 0x03ff;
			max *= mult;
		}

		if (urb->number_of_packets <= 0)		    
			return -EINVAL;
		for (n = 0; n < urb->number_of_packets; n++) {
			len = urb->iso_frame_desc [n].length;
			if (len < 0 || len > max) 
				return -EMSGSIZE;
			urb->iso_frame_desc [n].status = -EXDEV;
			urb->iso_frame_desc [n].actual_length = 0;
		}
	}

	/* the I/O buffer must be mapped/unmapped, except when length=0 */
	if (urb->transfer_buffer_length < 0)
		return -EMSGSIZE;

#ifdef DEBUG
	/* stuff that drivers shouldn't do, but which shouldn't
	 * cause problems in HCDs if they get it wrong.
	 */
	{
	unsigned int	orig_flags = urb->transfer_flags;
	unsigned int	allowed;

	/* enforce simple/standard policy */
	allowed = URB_ASYNC_UNLINK;	// affects later unlinks
	allowed |= URB_NO_DMA_MAP;
	allowed |= URB_NO_INTERRUPT;
	switch (temp) {
	case PIPE_BULK:
		if (is_out)
			allowed |= URB_ZERO_PACKET;
		/* FALLTHROUGH */
	case PIPE_CONTROL:
		allowed |= URB_NO_FSBR;	/* only affects UHCI */
		/* FALLTHROUGH */
	default:			/* all non-iso endpoints */
		if (!is_out)
			allowed |= URB_SHORT_NOT_OK;
		break;
	case PIPE_ISOCHRONOUS:
		allowed |= URB_ISO_ASAP;
		break;
	}
	urb->transfer_flags &= allowed;

	/* fail if submitter gave bogus flags */
	if (urb->transfer_flags != orig_flags) {
		err ("BOGUS urb flags, %x --> %x",
			orig_flags, urb->transfer_flags);
		return -EINVAL;
	}
	}
#endif
	/*
	 * Force periodic transfer intervals to be legal values that are
	 * a power of two (so HCDs don't need to).
	 *
	 * FIXME want bus->{intr,iso}_sched_horizon values here.  Each HC
	 * supports different values... this uses EHCI/UHCI defaults (and
	 * EHCI can use smaller non-default values).
	 */
	switch (temp) {
	case PIPE_ISOCHRONOUS:
	case PIPE_INTERRUPT:
		/* too small? */
		if (urb->interval <= 0)
			return -EINVAL;
		/* too big? */
		switch (dev->speed) {
		case USB_SPEED_HIGH:	/* units are microframes */
			// NOTE usb handles 2^15
			if (urb->interval > (1024 * 8))
				urb->interval = 1024 * 8;
			temp = 1024 * 8;
			break;
		case USB_SPEED_FULL:	/* units are frames/msec */
		case USB_SPEED_LOW:
			if (temp == PIPE_INTERRUPT) {
				if (urb->interval > 255)
					return -EINVAL;
				// NOTE ohci only handles up to 32
				temp = 128;
			} else {
				if (urb->interval > 1024)
					urb->interval = 1024;
				// NOTE usb and ohci handle up to 2^15
				temp = 1024;
			}
			break;
		default:
			return -EINVAL;
		}
		/* power of two? */
		while (temp > urb->interval)
			temp >>= 1;
		urb->interval = temp;
	}

	return op->submit_urb (urb, mem_flags);
}

/*-------------------------------------------------------------------*/

/**
 * usb_unlink_urb - abort/cancel a transfer request for an endpoint
 * @urb: pointer to urb describing a previously submitted request
 *
 * This routine cancels an in-progress request.  The requests's
 * completion handler will be called with a status code indicating
 * that the request has been canceled, and that control of the URB
 * has been returned to that device driver.
 *
 * When the URB_ASYNC_UNLINK transfer flag for the URB is clear, this
 * request is synchronous.  Success is indicated by returning zero,
 * at which time the urb will have been unlinked,
 * and the completion function will see status -ENOENT.  Failure is
 * indicated by any other return value.  This mode may not be used
 * when unlinking an urb from an interrupt context, such as a bottom
 * half or a completion handler,
 *
 * When the URB_ASYNC_UNLINK transfer flag for the URB is set, this
 * request is asynchronous.  Success is indicated by returning -EINPROGRESS,
 * at which time the urb will normally not have been unlinked,
 * and the completion function will see status -ECONNRESET.  Failure is
 * indicated by any other return value.
 */
int usb_unlink_urb(struct urb *urb)
{
	if (urb && urb->dev && urb->dev->bus && urb->dev->bus->op)
		return urb->dev->bus->op->unlink_urb(urb);
	else
		return -ENODEV;
}

EXPORT_SYMBOL(usb_init_urb);
EXPORT_SYMBOL(usb_alloc_urb);
EXPORT_SYMBOL(usb_free_urb);
EXPORT_SYMBOL(usb_get_urb);
EXPORT_SYMBOL(usb_submit_urb);
EXPORT_SYMBOL(usb_unlink_urb);

