/*
 * message.c - synchronous message handling
 */
#if 0
#include <linux/pci.h>	/* for scatterlist macros */
#include <linux/usb.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <asm/byteorder.h>
#else
#include "../usb_wrapper.h"
#endif

#include "hcd.h"	/* for usbcore internals */

struct usb_api_data {
	wait_queue_head_t wqh;
	int done;
};

static void usb_api_blocking_completion(struct urb *urb, struct pt_regs *regs)
{
	struct usb_api_data *awd = (struct usb_api_data *)urb->context;

	awd->done = 1;
	wmb();
	wake_up(&awd->wqh);
}

// Starts urb and waits for completion or timeout
static int usb_start_wait_urb(struct urb *urb, int timeout, int* actual_length)
{ 
	DECLARE_WAITQUEUE(wait, current);
	struct usb_api_data awd;
	int status;

	init_waitqueue_head(&awd.wqh); 	
	awd.done = 0;

	set_current_state(TASK_UNINTERRUPTIBLE);
	add_wait_queue(&awd.wqh, &wait);

	urb->context = &awd;
	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status) {
		// something went wrong
		usb_free_urb(urb);
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&awd.wqh, &wait);
		return status;
	}
	
	while (timeout && !awd.done)
	{		
		timeout = schedule_timeout(timeout);
		set_current_state(TASK_UNINTERRUPTIBLE);
		rmb();
	}

	set_current_state(TASK_RUNNING);
	remove_wait_queue(&awd.wqh, &wait);

	if (!timeout && !awd.done) {
		if (urb->status != -EINPROGRESS) {	/* No callback?!! */
			printk(KERN_ERR "usb: raced timeout, "
			    "pipe 0x%x status %d time left %d\n",
			    urb->pipe, urb->status, timeout);
			status = urb->status;
		} else {
			warn("usb_control/bulk_msg: timeout");
			usb_unlink_urb(urb);  // remove urb safely
			status = -ETIMEDOUT;
		}
	} else
		status = urb->status;

	if (actual_length)
		*actual_length = urb->actual_length;

	usb_free_urb(urb);
  	return status;
}

/*-------------------------------------------------------------------*/
// returns status (negative) or length (positive)
int usb_internal_control_msg(struct usb_device *usb_dev, unsigned int pipe, 
			    struct usb_ctrlrequest *cmd,  void *data, int len, int timeout)
{
	struct urb *urb;
	int retv;
	int length;

	urb = usb_alloc_urb(0, GFP_NOIO);
	if (!urb)
		return -ENOMEM;
  
	usb_fill_control_urb(urb, usb_dev, pipe, (unsigned char*)cmd, data, len,
		   usb_api_blocking_completion, 0);

	retv = usb_start_wait_urb(urb, timeout, &length);

	if (retv < 0)
		return retv;
	else
		return length;
}

/**
 *	usb_control_msg - Builds a control urb, sends it off and waits for completion
 *	@dev: pointer to the usb device to send the message to
 *	@pipe: endpoint "pipe" to send the message to
 *	@request: USB message request value
 *	@requesttype: USB message request type value
 *	@value: USB message value
 *	@index: USB message index value
 *	@data: pointer to the data to send
 *	@size: length in bytes of the data to send
 *	@timeout: time in jiffies to wait for the message to complete before
 *		timing out (if 0 the wait is forever)
 *	Context: !in_interrupt ()
 *
 *	This function sends a simple control message to a specified endpoint
 *	and waits for the message to complete, or timeout.
 *	
 *	If successful, it returns the number of bytes transferred, otherwise a negative error number.
 *
 *	Don't use this function from within an interrupt context, like a
 *	bottom half handler.  If you need an asynchronous message, or need to send
 *	a message from within interrupt context, use usb_submit_urb()
 *      If a thread in your driver uses this call, make sure your disconnect()
 *      method can wait for it to complete.  Since you don't have a handle on
 *      the URB used, you can't cancel the request.
 */
int usb_control_msg(struct usb_device *dev, unsigned int pipe, __u8 request, __u8 requesttype,
			 __u16 value, __u16 index, void *data, __u16 size, int timeout)
{
	struct usb_ctrlrequest *dr = kmalloc(sizeof(struct usb_ctrlrequest), GFP_NOIO);
	int ret;
	
	if (!dr)
		return -ENOMEM;

	dr->bRequestType= requesttype;
	dr->bRequest = request;
	dr->wValue = cpu_to_le16p(&value);
	dr->wIndex = cpu_to_le16p(&index);
	dr->wLength = cpu_to_le16p(&size);

	//dbg("usb_control_msg");	

	ret = usb_internal_control_msg(dev, pipe, dr, data, size, timeout);

	kfree(dr);

	return ret;
}


/**
 *	usb_bulk_msg - Builds a bulk urb, sends it off and waits for completion
 *	@usb_dev: pointer to the usb device to send the message to
 *	@pipe: endpoint "pipe" to send the message to
 *	@data: pointer to the data to send
 *	@len: length in bytes of the data to send
 *	@actual_length: pointer to a location to put the actual length transferred in bytes
 *	@timeout: time in jiffies to wait for the message to complete before
 *		timing out (if 0 the wait is forever)
 *	Context: !in_interrupt ()
 *
 *	This function sends a simple bulk message to a specified endpoint
 *	and waits for the message to complete, or timeout.
 *	
 *	If successful, it returns 0, otherwise a negative error number.
 *	The number of actual bytes transferred will be stored in the 
 *	actual_length paramater.
 *
 *	Don't use this function from within an interrupt context, like a
 *	bottom half handler.  If you need an asynchronous message, or need to
 *	send a message from within interrupt context, use usb_submit_urb()
 *      If a thread in your driver uses this call, make sure your disconnect()
 *      method can wait for it to complete.  Since you don't have a handle on
 *      the URB used, you can't cancel the request.
 */
int usb_bulk_msg(struct usb_device *usb_dev, unsigned int pipe, 
			void *data, int len, int *actual_length, int timeout)
{
	struct urb *urb;

	if (len < 0)
		return -EINVAL;

	urb=usb_alloc_urb(0, GFP_KERNEL);
	if (!urb)
		return -ENOMEM;

	usb_fill_bulk_urb(urb, usb_dev, pipe, data, len,
		    usb_api_blocking_completion, 0);

	return usb_start_wait_urb(urb,timeout,actual_length);
}

/*-------------------------------------------------------------------*/
//#warning "Scatter-gather stuff disabled"
#if 0
static void sg_clean (struct usb_sg_request *io)
{
	if (io->urbs) {
		while (io->entries--)
			usb_free_urb (io->urbs [io->entries]);
		kfree (io->urbs);
		io->urbs = 0;
	}
	if (io->dev->dev.dma_mask != 0)
		usb_buffer_unmap_sg (io->dev, io->pipe, io->sg, io->nents);
	io->dev = 0;
}

static void sg_complete (struct urb *urb, struct pt_regs *regs)
{
	struct usb_sg_request	*io = (struct usb_sg_request *) urb->context;
	unsigned long		flags;

	spin_lock_irqsave (&io->lock, flags);

	/* In 2.5 we require hcds' endpoint queues not to progress after fault
	 * reports, until the completion callback (this!) returns.  That lets
	 * device driver code (like this routine) unlink queued urbs first,
	 * if it needs to, since the HC won't work on them at all.  So it's
	 * not possible for page N+1 to overwrite page N, and so on.
	 *
	 * That's only for "hard" faults; "soft" faults (unlinks) sometimes
	 * complete before the HCD can get requests away from hardware,
	 * though never during cleanup after a hard fault.
	 */
	if (io->status
			&& (io->status != -ECONNRESET
				|| urb->status != -ECONNRESET)
			&& urb->actual_length) {
		dev_err (io->dev->bus->controller,
			"dev %s ep%d%s scatterlist error %d/%d\n",
			io->dev->devpath,
			usb_pipeendpoint (urb->pipe),
			usb_pipein (urb->pipe) ? "in" : "out",
			urb->status, io->status);
		// BUG ();
	}

	if (urb->status && urb->status != -ECONNRESET) {
		int		i, found, status;

		io->status = urb->status;

		/* the previous urbs, and this one, completed already.
		 * unlink the later ones so they won't rx/tx bad data,
		 *
		 * FIXME don't bother unlinking urbs that haven't yet been
		 * submitted; those non-error cases shouldn't be syslogged
		 */
		for (i = 0, found = 0; i < io->entries; i++) {
			if (found) {
				status = usb_unlink_urb (io->urbs [i]);
				if (status && status != -EINPROGRESS)
					err ("sg_complete, unlink --> %d",
							status);
			} else if (urb == io->urbs [i])
				found = 1;
		}
	}

	/* on the last completion, signal usb_sg_wait() */
	io->bytes += urb->actual_length;
	io->count--;
	if (!io->count)
		complete (&io->complete);

	spin_unlock_irqrestore (&io->lock, flags);
}


/**
 * usb_sg_init - initializes scatterlist-based bulk/interrupt I/O request
 * @io: request block being initialized.  until usb_sg_wait() returns,
 *	treat this as a pointer to an opaque block of memory,
 * @dev: the usb device that will send or receive the data
 * @pipe: endpoint "pipe" used to transfer the data
 * @period: polling rate for interrupt endpoints, in frames or
 * 	(for high speed endpoints) microframes; ignored for bulk
 * @sg: scatterlist entries
 * @nents: how many entries in the scatterlist
 * @length: how many bytes to send from the scatterlist, or zero to
 * 	send every byte identified in the list.
 * @mem_flags: SLAB_* flags affecting memory allocations in this call
 *
 * Returns zero for success, else a negative errno value.  This initializes a
 * scatter/gather request, allocating resources such as I/O mappings and urb
 * memory (except maybe memory used by USB controller drivers).
 *
 * The request must be issued using usb_sg_wait(), which waits for the I/O to
 * complete (or to be canceled) and then cleans up all resources allocated by
 * usb_sg_init().
 *
 * The request may be canceled with usb_sg_cancel(), either before or after
 * usb_sg_wait() is called.
 */
int usb_sg_init (
	struct usb_sg_request	*io,
	struct usb_device	*dev,
	unsigned		pipe, 
	unsigned		period,
	struct scatterlist	*sg,
	int			nents,
	size_t			length,
	int			mem_flags
)
{
	int			i;
	int			urb_flags;
	int			dma;

	if (!io || !dev || !sg
			|| usb_pipecontrol (pipe)
			|| usb_pipeisoc (pipe)
			|| nents <= 0)
		return -EINVAL;

	spin_lock_init (&io->lock);
	io->dev = dev;
	io->pipe = pipe;
	io->sg = sg;
	io->nents = nents;

	/* not all host controllers use DMA (like the mainstream pci ones);
	 * they can use PIO (sl811) or be software over another transport.
	 */
	dma = (dev->dev.dma_mask != 0);
	if (dma)
		io->entries = usb_buffer_map_sg (dev, pipe, sg, nents);
	else
		io->entries = nents;

	/* initialize all the urbs we'll use */
	if (io->entries <= 0)
		return io->entries;

	io->count = 0;
	io->urbs = kmalloc (io->entries * sizeof *io->urbs, mem_flags);
	if (!io->urbs)
		goto nomem;

	urb_flags = URB_ASYNC_UNLINK | URB_NO_DMA_MAP | URB_NO_INTERRUPT;
	if (usb_pipein (pipe))
		urb_flags |= URB_SHORT_NOT_OK;

	for (i = 0; i < io->entries; i++, io->count = i) {
		unsigned		len;

		io->urbs [i] = usb_alloc_urb (0, mem_flags);
		if (!io->urbs [i]) {
			io->entries = i;
			goto nomem;
		}

		io->urbs [i]->dev = dev;
		io->urbs [i]->pipe = pipe;
		io->urbs [i]->interval = period;
		io->urbs [i]->transfer_flags = urb_flags;

		io->urbs [i]->complete = sg_complete;
		io->urbs [i]->context = io;
		io->urbs [i]->status = -EINPROGRESS;
		io->urbs [i]->actual_length = 0;

		if (dma) {
			/* hc may use _only_ transfer_dma */
			io->urbs [i]->transfer_dma = sg_dma_address (sg + i);
			len = sg_dma_len (sg + i);
		} else {
			/* hc may use _only_ transfer_buffer */
			io->urbs [i]->transfer_buffer =
				page_address (sg [i].page) + sg [i].offset;
			len = sg [i].length;
		}

		if (length) {
			len = min_t (unsigned, len, length);
			length -= len;
			if (length == 0)
				io->entries = i + 1;
		}
		io->urbs [i]->transfer_buffer_length = len;
	}
	io->urbs [--i]->transfer_flags &= ~URB_NO_INTERRUPT;

	/* transaction state */
	io->status = 0;
	io->bytes = 0;
	init_completion (&io->complete);
	return 0;

nomem:
	sg_clean (io);
	return -ENOMEM;
}


/**
 * usb_sg_wait - synchronously execute scatter/gather request
 * @io: request block handle, as initialized with usb_sg_init().
 * 	some fields become accessible when this call returns.
 * Context: !in_interrupt ()
 *
 * This function blocks until the specified I/O operation completes.  It
 * leverages the grouping of the related I/O requests to get good transfer
 * rates, by queueing the requests.  At higher speeds, such queuing can
 * significantly improve USB throughput.
 *
 * There are three kinds of completion for this function.
 * (1) success, where io->status is zero.  The number of io->bytes
 *     transferred is as requested.
 * (2) error, where io->status is a negative errno value.  The number
 *     of io->bytes transferred before the error is usually less
 *     than requested, and can be nonzero.
 * (3) cancelation, a type of error with status -ECONNRESET that
 *     is initiated by usb_sg_cancel().
 *
 * When this function returns, all memory allocated through usb_sg_init() or
 * this call will have been freed.  The request block parameter may still be
 * passed to usb_sg_cancel(), or it may be freed.  It could also be
 * reinitialized and then reused.
 *
 * Data Transfer Rates:
 *
 * Bulk transfers are valid for full or high speed endpoints.
 * The best full speed data rate is 19 packets of 64 bytes each
 * per frame, or 1216 bytes per millisecond.
 * The best high speed data rate is 13 packets of 512 bytes each
 * per microframe, or 52 KBytes per millisecond.
 *
 * The reason to use interrupt transfers through this API would most likely
 * be to reserve high speed bandwidth, where up to 24 KBytes per millisecond
 * could be transferred.  That capability is less useful for low or full
 * speed interrupt endpoints, which allow at most one packet per millisecond,
 * of at most 8 or 64 bytes (respectively).
 */
void usb_sg_wait (struct usb_sg_request *io)
{
	int		i;
	unsigned long	flags;

	/* queue the urbs.  */
	spin_lock_irqsave (&io->lock, flags);
	for (i = 0; i < io->entries && !io->status; i++) {
		int	retval;

		retval = usb_submit_urb (io->urbs [i], SLAB_ATOMIC);

		/* after we submit, let completions or cancelations fire;
		 * we handshake using io->status.
		 */
		spin_unlock_irqrestore (&io->lock, flags);
		switch (retval) {
			/* maybe we retrying will recover */
		case -ENXIO:	// hc didn't queue this one
		case -EAGAIN:
		case -ENOMEM:
			retval = 0;
			i--;
			// FIXME:  should it usb_sg_cancel() on INTERRUPT?
			yield ();
			break;

			/* no error? continue immediately.
			 *
			 * NOTE: to work better with UHCI (4K I/O buffer may
			 * need 3K of TDs) it may be good to limit how many
			 * URBs are queued at once; N milliseconds?
			 */
		case 0:
			cpu_relax ();
			break;

			/* fail any uncompleted urbs */
		default:
			io->urbs [i]->status = retval;
			dbg ("usb_sg_msg, submit --> %d", retval);
			usb_sg_cancel (io);
		}
		spin_lock_irqsave (&io->lock, flags);
		if (retval && io->status == -ECONNRESET)
			io->status = retval;
	}
	spin_unlock_irqrestore (&io->lock, flags);

	/* OK, yes, this could be packaged as non-blocking.
	 * So could the submit loop above ... but it's easier to
	 * solve neither problem than to solve both!
	 */
	wait_for_completion (&io->complete);

	sg_clean (io);
}

/**
 * usb_sg_cancel - stop scatter/gather i/o issued by usb_sg_wait()
 * @io: request block, initialized with usb_sg_init()
 *
 * This stops a request after it has been started by usb_sg_wait().
 * It can also prevents one initialized by usb_sg_init() from starting,
 * so that call just frees resources allocated to the request.
 */
void usb_sg_cancel (struct usb_sg_request *io)
{
	unsigned long	flags;

	spin_lock_irqsave (&io->lock, flags);

	/* shut everything down, if it didn't already */
	if (!io->status) {
		int	i;

		io->status = -ECONNRESET;
		for (i = 0; i < io->entries; i++) {
			int	retval;

			if (!io->urbs [i]->dev)
				continue;
			retval = usb_unlink_urb (io->urbs [i]);
			if (retval && retval != -EINPROGRESS)
				warn ("usb_sg_cancel, unlink --> %d", retval);
			// FIXME don't warn on "not yet submitted" error
		}
	}
	spin_unlock_irqrestore (&io->lock, flags);
}
#endif
/*-------------------------------------------------------------------*/

/**
 * usb_get_descriptor - issues a generic GET_DESCRIPTOR request
 * @dev: the device whose descriptor is being retrieved
 * @type: the descriptor type (USB_DT_*)
 * @index: the number of the descriptor
 * @buf: where to put the descriptor
 * @size: how big is "buf"?
 * Context: !in_interrupt ()
 *
 * Gets a USB descriptor.  Convenience functions exist to simplify
 * getting some types of descriptors.  Use
 * usb_get_device_descriptor() for USB_DT_DEVICE,
 * and usb_get_string() or usb_string() for USB_DT_STRING.
 * Configuration descriptors (USB_DT_CONFIG) are part of the device
 * structure, at least for the current configuration.
 * In addition to a number of USB-standard descriptors, some
 * devices also use class-specific or vendor-specific descriptors.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Returns the number of bytes received on success, or else the status code
 * returned by the underlying usb_control_msg() call.
 */
int usb_get_descriptor(struct usb_device *dev, unsigned char type, unsigned char index, void *buf, int size)
{
	int i = 5;
	int result;
	
	memset(buf,0,size);	// Make sure we parse really received data

	while (i--) {
		/* retries if the returned length was 0; flakey device */
		if ((result = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
				    USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
				    (type << 8) + index, 0, buf, size,
				    HZ * USB_CTRL_GET_TIMEOUT)) > 0
				|| result == -EPIPE)
			break;
	}
	return result;
}

/**
 * usb_get_string - gets a string descriptor
 * @dev: the device whose string descriptor is being retrieved
 * @langid: code for language chosen (from string descriptor zero)
 * @index: the number of the descriptor
 * @buf: where to put the string
 * @size: how big is "buf"?
 * Context: !in_interrupt ()
 *
 * Retrieves a string, encoded using UTF-16LE (Unicode, 16 bits per character,
 * in little-endian byte order).
 * The usb_string() function will often be a convenient way to turn
 * these strings into kernel-printable form.
 *
 * Strings may be referenced in device, configuration, interface, or other
 * descriptors, and could also be used in vendor-specific ways.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Returns the number of bytes received on success, or else the status code
 * returned by the underlying usb_control_msg() call.
 */
int usb_get_string(struct usb_device *dev, unsigned short langid, unsigned char index, void *buf, int size)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
		USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
		(USB_DT_STRING << 8) + index, langid, buf, size,
		HZ * USB_CTRL_GET_TIMEOUT);
}

/**
 * usb_get_device_descriptor - (re)reads the device descriptor
 * @dev: the device whose device descriptor is being updated
 * Context: !in_interrupt ()
 *
 * Updates the copy of the device descriptor stored in the device structure,
 * which dedicates space for this purpose.  Note that several fields are
 * converted to the host CPU's byte order:  the USB version (bcdUSB), and
 * vendors product and version fields (idVendor, idProduct, and bcdDevice).
 * That lets device drivers compare against non-byteswapped constants.
 *
 * There's normally no need to use this call, although some devices
 * will change their descriptors after events like updating firmware.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Returns the number of bytes received on success, or else the status code
 * returned by the underlying usb_control_msg() call.
 */
int usb_get_device_descriptor(struct usb_device *dev)
{
	int ret = usb_get_descriptor(dev, USB_DT_DEVICE, 0, &dev->descriptor,
				     sizeof(dev->descriptor));
	if (ret >= 0) {
		le16_to_cpus(&dev->descriptor.bcdUSB);
		le16_to_cpus(&dev->descriptor.idVendor);
		le16_to_cpus(&dev->descriptor.idProduct);
		le16_to_cpus(&dev->descriptor.bcdDevice);
	}
	return ret;
}

/**
 * usb_get_status - issues a GET_STATUS call
 * @dev: the device whose status is being checked
 * @type: USB_RECIP_*; for device, interface, or endpoint
 * @target: zero (for device), else interface or endpoint number
 * @data: pointer to two bytes of bitmap data
 * Context: !in_interrupt ()
 *
 * Returns device, interface, or endpoint status.  Normally only of
 * interest to see if the device is self powered, or has enabled the
 * remote wakeup facility; or whether a bulk or interrupt endpoint
 * is halted ("stalled").
 *
 * Bits in these status bitmaps are set using the SET_FEATURE request,
 * and cleared using the CLEAR_FEATURE request.  The usb_clear_halt()
 * function should be used to clear halt ("stall") status.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Returns the number of bytes received on success, or else the status code
 * returned by the underlying usb_control_msg() call.
 */
int usb_get_status(struct usb_device *dev, int type, int target, void *data)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
		USB_REQ_GET_STATUS, USB_DIR_IN | type, 0, target, data, 2,
		HZ * USB_CTRL_GET_TIMEOUT);
}


// hub-only!! ... and only exported for reset/reinit path.
// otherwise used internally, when setting up a config
void usb_set_maxpacket(struct usb_device *dev)
{
	int i, b;

	/* NOTE:  affects all endpoints _except_ ep0 */
	for (i=0; i<dev->actconfig->desc.bNumInterfaces; i++) {
		struct usb_interface *ifp = dev->actconfig->interface + i;
		struct usb_host_interface *as = ifp->altsetting + ifp->act_altsetting;
		struct usb_host_endpoint *ep = as->endpoint;
		int e;

		for (e=0; e<as->desc.bNumEndpoints; e++) {
			struct usb_endpoint_descriptor	*d;
			d = &ep [e].desc;
			b = d->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
			if ((d->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
				USB_ENDPOINT_XFER_CONTROL) {	/* Control => bidirectional */
				dev->epmaxpacketout[b] = d->wMaxPacketSize;
				dev->epmaxpacketin [b] = d->wMaxPacketSize;
				}
			else if (usb_endpoint_out(d->bEndpointAddress)) {
				if (d->wMaxPacketSize > dev->epmaxpacketout[b])
					dev->epmaxpacketout[b] = d->wMaxPacketSize;
			}
			else {
				if (d->wMaxPacketSize > dev->epmaxpacketin [b])
					dev->epmaxpacketin [b] = d->wMaxPacketSize;
			}
		}
	}
}

/**
 * usb_clear_halt - tells device to clear endpoint halt/stall condition
 * @dev: device whose endpoint is halted
 * @pipe: endpoint "pipe" being cleared
 * Context: !in_interrupt ()
 *
 * This is used to clear halt conditions for bulk and interrupt endpoints,
 * as reported by URB completion status.  Endpoints that are halted are
 * sometimes referred to as being "stalled".  Such endpoints are unable
 * to transmit or receive data until the halt status is cleared.  Any URBs
 * queued for such an endpoint should normally be unlinked by the driver
 * before clearing the halt condition, as described in sections 5.7.5
 * and 5.8.5 of the USB 2.0 spec.
 *
 * Note that control and isochronous endpoints don't halt, although control
 * endpoints report "protocol stall" (for unsupported requests) using the
 * same status code used to report a true stall.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Returns zero on success, or else the status code returned by the
 * underlying usb_control_msg() call.
 */
int usb_clear_halt(struct usb_device *dev, int pipe)
{
	int result;
	int endp = usb_pipeendpoint(pipe);
	
	if (usb_pipein (pipe))
		endp |= USB_DIR_IN;

	/* we don't care if it wasn't halted first. in fact some devices
	 * (like some ibmcam model 1 units) seem to expect hosts to make
	 * this request for iso endpoints, which can't halt!
	 */
	result = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
		USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT, 0, endp, NULL, 0,
		HZ * USB_CTRL_SET_TIMEOUT);

	/* don't un-halt or force to DATA0 except on success */
	if (result < 0)
		return result;

	/* NOTE:  seems like Microsoft and Apple don't bother verifying
	 * the clear "took", so some devices could lock up if you check...
	 * such as the Hagiwara FlashGate DUAL.  So we won't bother.
	 *
	 * NOTE:  make sure the logic here doesn't diverge much from
	 * the copy in usb-storage, for as long as we need two copies.
	 */

	/* toggle was reset by the clear, then ep was reactivated */
	usb_settoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe), 0);
	usb_endpoint_running(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe));

	return 0;
}

/**
 * usb_set_interface - Makes a particular alternate setting be current
 * @dev: the device whose interface is being updated
 * @interface: the interface being updated
 * @alternate: the setting being chosen.
 * Context: !in_interrupt ()
 *
 * This is used to enable data transfers on interfaces that may not
 * be enabled by default.  Not all devices support such configurability.
 * Only the driver bound to an interface may change its setting.
 *
 * Within any given configuration, each interface may have several
 * alternative settings.  These are often used to control levels of
 * bandwidth consumption.  For example, the default setting for a high
 * speed interrupt endpoint may not send more than 64 bytes per microframe,
 * while interrupt transfers of up to 3KBytes per microframe are legal.
 * Also, isochronous endpoints may never be part of an
 * interface's default setting.  To access such bandwidth, alternate
 * interface settings must be made current.
 *
 * Note that in the Linux USB subsystem, bandwidth associated with
 * an endpoint in a given alternate setting is not reserved until an URB
 * is submitted that needs that bandwidth.  Some other operating systems
 * allocate bandwidth early, when a configuration is chosen.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 * Also, drivers must not change altsettings while urbs are scheduled for
 * endpoints in that interface; all such urbs must first be completed
 * (perhaps forced by unlinking).
 *
 * Returns zero on success, or else the status code returned by the
 * underlying usb_control_msg() call.
 */
int usb_set_interface(struct usb_device *dev, int interface, int alternate)
{
	struct usb_interface *iface;
	struct usb_host_interface *iface_as;
	int i, ret;
	void (*disable)(struct usb_device *, int) = dev->bus->op->disable;

	iface = usb_ifnum_to_if(dev, interface);
	if (!iface) {
		warn("selecting invalid interface %d", interface);
		return -EINVAL;
	}

	/* 9.4.10 says devices don't need this, if the interface
	   only has one alternate setting */
	if (iface->num_altsetting == 1) {
		dbg("ignoring set_interface for dev %d, iface %d, alt %d",
			dev->devnum, interface, alternate);
		return 0;
	}

	if (alternate < 0 || alternate >= iface->num_altsetting)
		return -EINVAL;

	if ((ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				   USB_REQ_SET_INTERFACE, USB_RECIP_INTERFACE,
				   iface->altsetting[alternate]
				   	.desc.bAlternateSetting,
				   interface, NULL, 0, HZ * 5)) < 0)
		return ret;

	/* FIXME drivers shouldn't need to replicate/bugfix the logic here
	 * when they implement async or easily-killable versions of this or
	 * other "should-be-internal" functions (like clear_halt).
	 * should hcd+usbcore postprocess control requests?
	 */

	/* prevent submissions using previous endpoint settings */
	iface_as = iface->altsetting + iface->act_altsetting;
	for (i = 0; i < iface_as->desc.bNumEndpoints; i++) {
		u8	ep = iface_as->endpoint [i].desc.bEndpointAddress;
		int	out = !(ep & USB_DIR_IN);

		/* clear out hcd state, then usbcore state */
		if (disable)
			disable (dev, ep);
		ep &= USB_ENDPOINT_NUMBER_MASK;
		(out ? dev->epmaxpacketout : dev->epmaxpacketin ) [ep] = 0;
	}
	iface->act_altsetting = alternate;

	/* 9.1.1.5: reset toggles for all endpoints affected by this iface-as
	 *
	 * Note:
	 * Despite EP0 is always present in all interfaces/AS, the list of
	 * endpoints from the descriptor does not contain EP0. Due to its
	 * omnipresence one might expect EP0 being considered "affected" by
	 * any SetInterface request and hence assume toggles need to be reset.
	 * However, EP0 toggles are re-synced for every individual transfer
	 * during the SETUP stage - hence EP0 toggles are "don't care" here.
	 * (Likewise, EP0 never "halts" on well designed devices.)
	 */

	iface_as = &iface->altsetting[alternate];
	for (i = 0; i < iface_as->desc.bNumEndpoints; i++) {
		u8	ep = iface_as->endpoint[i].desc.bEndpointAddress;
		int	out = !(ep & USB_DIR_IN);

		ep &= USB_ENDPOINT_NUMBER_MASK;
		usb_settoggle (dev, ep, out, 0);
		(out ? dev->epmaxpacketout : dev->epmaxpacketin) [ep]
			= iface_as->endpoint [i].desc.wMaxPacketSize;
		usb_endpoint_running (dev, ep, out);
	}

	return 0;
}

/**
 * usb_set_configuration - Makes a particular device setting be current
 * @dev: the device whose configuration is being updated
 * @configuration: the configuration being chosen.
 * Context: !in_interrupt ()
 *
 * This is used to enable non-default device modes.  Not all devices
 * support this kind of configurability.  By default, configuration
 * zero is selected after enumeration; many devices only have a single
 * configuration.
 *
 * USB devices may support one or more configurations, which affect
 * power consumption and the functionality available.  For example,
 * the default configuration is limited to using 100mA of bus power,
 * so that when certain device functionality requires more power,
 * and the device is bus powered, that functionality will be in some
 * non-default device configuration.  Other device modes may also be
 * reflected as configuration options, such as whether two ISDN
 * channels are presented as independent 64Kb/s interfaces or as one
 * bonded 128Kb/s interface.
 *
 * Note that USB has an additional level of device configurability,
 * associated with interfaces.  That configurability is accessed using
 * usb_set_interface().
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Returns zero on success, or else the status code returned by the
 * underlying usb_control_msg() call.
 */
int usb_set_configuration(struct usb_device *dev, int configuration)
{
	int i, ret;
	struct usb_host_config *cp = NULL;
	void (*disable)(struct usb_device *, int) = dev->bus->op->disable;
	
	for (i=0; i<dev->descriptor.bNumConfigurations; i++) {
		if (dev->config[i].desc.bConfigurationValue == configuration) {
			cp = &dev->config[i];
			break;
		}
	}
	if ((!cp && configuration != 0) || (cp && configuration == 0)) {
		warn("selecting invalid configuration %d", configuration);
		return -EINVAL;
	}

	/* if it's already configured, clear out old state first. */
	if (dev->state != USB_STATE_ADDRESS && disable) {
		for (i = 1 /* skip ep0 */; i < 15; i++) {
			disable (dev, i);
			disable (dev, USB_DIR_IN | i);
		}
	}
	dev->toggle[0] = dev->toggle[1] = 0;
	dev->halted[0] = dev->halted[1] = 0;
	dev->state = USB_STATE_ADDRESS;

	if ((ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
			USB_REQ_SET_CONFIGURATION, 0, configuration, 0,
			NULL, 0, HZ * USB_CTRL_SET_TIMEOUT)) < 0)
		return ret;
	if (configuration)
		dev->state = USB_STATE_CONFIGURED;
	dev->actconfig = cp;

	/* reset more hc/hcd endpoint state */
	usb_set_maxpacket(dev);

	return 0;
}


/**
 * usb_string - returns ISO 8859-1 version of a string descriptor
 * @dev: the device whose string descriptor is being retrieved
 * @index: the number of the descriptor
 * @buf: where to put the string
 * @size: how big is "buf"?
 * Context: !in_interrupt ()
 * 
 * This converts the UTF-16LE encoded strings returned by devices, from
 * usb_get_string_descriptor(), to null-terminated ISO-8859-1 encoded ones
 * that are more usable in most kernel contexts.  Note that all characters
 * in the chosen descriptor that can't be encoded using ISO-8859-1
 * are converted to the question mark ("?") character, and this function
 * chooses strings in the first language supported by the device.
 *
 * The ASCII (or, redundantly, "US-ASCII") character set is the seven-bit
 * subset of ISO 8859-1. ISO-8859-1 is the eight-bit subset of Unicode,
 * and is appropriate for use many uses of English and several other
 * Western European languages.  (But it doesn't include the "Euro" symbol.)
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Returns length of the string (>= 0) or usb_control_msg status (< 0).
 */
int usb_string(struct usb_device *dev, int index, char *buf, size_t size)
{
	unsigned char *tbuf;
	int err, len;
	unsigned int u, idx;

	if (size <= 0 || !buf || !index)
		return -EINVAL;
	buf[0] = 0;
	tbuf = kmalloc(256, GFP_KERNEL);
	if (!tbuf)
		return -ENOMEM;

	/* get langid for strings if it's not yet known */
	if (!dev->have_langid) {
		err = usb_get_string(dev, 0, 0, tbuf, 4);
		if (err < 0) {
			err("error getting string descriptor 0 (error=%d)", err);
			goto errout;
		} else if (tbuf[0] < 4) {
			err("string descriptor 0 too short");
			err = -EINVAL;
			goto errout;
		} else {
			dev->have_langid = -1;
			dev->string_langid = tbuf[2] | (tbuf[3]<< 8);
				/* always use the first langid listed */
			dbg("USB device number %d default language ID 0x%x",
				dev->devnum, dev->string_langid);
		}
	}

	/*
	 * ask for the length of the string 
	 */

	err = usb_get_string(dev, dev->string_langid, index, tbuf, 2);
	if(err<2)
		goto errout;
	len=tbuf[0];	
	
	err = usb_get_string(dev, dev->string_langid, index, tbuf, len);
	if (err < 0)
		goto errout;

	size--;		/* leave room for trailing NULL char in output buffer */
	for (idx = 0, u = 2; u < err; u += 2) {
		if (idx >= size)
			break;
		if (tbuf[u+1])			/* high byte */
			buf[idx++] = '?';  /* non ISO-8859-1 character */
		else
			buf[idx++] = tbuf[u];
	}
	buf[idx] = 0;
	err = idx;

 errout:
	kfree(tbuf);
	return err;
}

// synchronous request completion model
EXPORT_SYMBOL(usb_control_msg);
EXPORT_SYMBOL(usb_bulk_msg);

EXPORT_SYMBOL(usb_sg_init);
EXPORT_SYMBOL(usb_sg_cancel);
EXPORT_SYMBOL(usb_sg_wait);

// synchronous control message convenience routines
EXPORT_SYMBOL(usb_get_descriptor);
EXPORT_SYMBOL(usb_get_device_descriptor);
EXPORT_SYMBOL(usb_get_status);
EXPORT_SYMBOL(usb_get_string);
EXPORT_SYMBOL(usb_string);
EXPORT_SYMBOL(usb_clear_halt);
EXPORT_SYMBOL(usb_set_configuration);
EXPORT_SYMBOL(usb_set_interface);

