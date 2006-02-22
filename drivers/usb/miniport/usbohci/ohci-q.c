/*
 * OHCI HCD (Host Controller Driver) for USB.
 * 
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2002 David Brownell <dbrownell@users.sourceforge.net>
 * 
 * This file is licenced under the GPL.
 */

static void urb_free_priv (struct ohci_hcd *hc, urb_priv_t *urb_priv)
{
	int		last = urb_priv->length - 1;

	if (last >= 0) {
		int		i;
		struct td	*td;

		for (i = 0; i <= last; i++) {
			td = urb_priv->td [i];
			if (td)
				td_free (hc, td);
		}
	}

	kfree (urb_priv);
}

/*-------------------------------------------------------------------------*/

/*
 * URB goes back to driver, and isn't reissued.
 * It's completely gone from HC data structures.
 * PRECONDITION:  no locks held, irqs blocked  (Giveback can call into HCD.)
 */
static void
finish_urb (struct ohci_hcd *ohci, struct urb *urb, struct pt_regs *regs)
{
	// ASSERT (urb->hcpriv != 0);

	urb_free_priv (ohci, urb->hcpriv);
	urb->hcpriv = NULL;

	spin_lock (&urb->lock);
	if (likely (urb->status == -EINPROGRESS))
		urb->status = 0;
	spin_unlock (&urb->lock);

	// what lock protects these?
	switch (usb_pipetype (urb->pipe)) {
	case PIPE_ISOCHRONOUS:
		hcd_to_bus (&ohci->hcd)->bandwidth_isoc_reqs--;
		break;
	case PIPE_INTERRUPT:
		hcd_to_bus (&ohci->hcd)->bandwidth_int_reqs--;
		break;
	}

#ifdef OHCI_VERBOSE_DEBUG
	urb_print (urb, "RET", usb_pipeout (urb->pipe));
#endif
	usb_hcd_giveback_urb (&ohci->hcd, urb, regs);
}


/*-------------------------------------------------------------------------*
 * ED handling functions
 *-------------------------------------------------------------------------*/  

/* search for the right schedule branch to use for a periodic ed.
 * does some load balancing; returns the branch, or negative errno.
 */
static int balance (struct ohci_hcd *ohci, int interval, int load)
{
	int	i, branch = -ENOSPC;

	/* iso periods can be huge; iso tds specify frame numbers */
	if (interval > NUM_INTS)
		interval = NUM_INTS;

	/* search for the least loaded schedule branch of that period
	 * that has enough bandwidth left unreserved.
	 */
	for (i = 0; i < interval ; i++) {
		if (branch < 0 || ohci->load [branch] > ohci->load [i]) {
#if 1	/* CONFIG_USB_BANDWIDTH */
			int	j;

			/* usb 1.1 says 90% of one frame */
			for (j = i; j < NUM_INTS; j += interval) {
				if ((ohci->load [j] + load) > 900)
					break;
			}
			if (j < NUM_INTS)
				continue;
#endif
			branch = i; 
		}
	}
	return branch;
}

/*-------------------------------------------------------------------------*/

/* both iso and interrupt requests have periods; this routine puts them
 * into the schedule tree in the apppropriate place.  most iso devices use
 * 1msec periods, but that's not required.
 */
static void periodic_link (struct ohci_hcd *ohci, struct ed *ed)
{
	unsigned	i;

	ohci_vdbg (ohci, "link %sed %p branch %d [%dus.], interval %d\n",
		(ed->hwINFO & ED_ISO) ? "iso " : "",
		ed, ed->branch, ed->load, ed->interval);

	for (i = ed->branch; i < NUM_INTS; i += ed->interval) {
		struct ed	**prev = &ohci->periodic [i];
		u32		*prev_p = &ohci->hcca->int_table [i];
		struct ed	*here = *prev;

		/* sorting each branch by period (slow before fast)
		 * lets us share the faster parts of the tree.
		 * (plus maybe: put interrupt eds before iso)
		 */
		while (here && ed != here) {
			if (ed->interval > here->interval)
				break;
			prev = &here->ed_next;
			prev_p = &here->hwNextED;
			here = *prev;
		}
		if (ed != here) {
			ed->ed_next = here;
			if (here)
				ed->hwNextED = *prev_p;
			wmb ();
			*prev = ed;
			*prev_p = cpu_to_le32p (&ed->dma);
		}
		ohci->load [i] += ed->load;
	}
	hcd_to_bus (&ohci->hcd)->bandwidth_allocated += ed->load / ed->interval;
}

/* link an ed into one of the HC chains */

static int ed_schedule (struct ohci_hcd *ohci, struct ed *ed)
{	 
	int	branch;

	ed->state = ED_OPER;
	ed->ed_prev = 0;
	ed->ed_next = 0;
	ed->hwNextED = 0;
	wmb ();

	/* we care about rm_list when setting CLE/BLE in case the HC was at
	 * work on some TD when CLE/BLE was turned off, and isn't quiesced
	 * yet.  finish_unlinks() restarts as needed, some upcoming INTR_SF.
	 *
	 * control and bulk EDs are doubly linked (ed_next, ed_prev), but
	 * periodic ones are singly linked (ed_next). that's because the
	 * periodic schedule encodes a tree like figure 3-5 in the ohci
	 * spec:  each qh can have several "previous" nodes, and the tree
	 * doesn't have unused/idle descriptors.
	 */
	switch (ed->type) {
	case PIPE_CONTROL:
		if (ohci->ed_controltail == NULL) {
			writel (ed->dma, &ohci->regs->ed_controlhead);
		} else {
			ohci->ed_controltail->ed_next = ed;
			ohci->ed_controltail->hwNextED = cpu_to_le32 (ed->dma);
		}
		ed->ed_prev = ohci->ed_controltail;
		if (!ohci->ed_controltail && !ohci->ed_rm_list) {
			ohci->hc_control |= OHCI_CTRL_CLE;
			writel (0, &ohci->regs->ed_controlcurrent);
			writel (ohci->hc_control, &ohci->regs->control);
		}
		ohci->ed_controltail = ed;
		break;

	case PIPE_BULK:
		if (ohci->ed_bulktail == NULL) {
			writel (ed->dma, &ohci->regs->ed_bulkhead);
		} else {
			ohci->ed_bulktail->ed_next = ed;
			ohci->ed_bulktail->hwNextED = cpu_to_le32 (ed->dma);
		}
		ed->ed_prev = ohci->ed_bulktail;
		if (!ohci->ed_bulktail && !ohci->ed_rm_list) {
			ohci->hc_control |= OHCI_CTRL_BLE;
			writel (0, &ohci->regs->ed_bulkcurrent);
			writel (ohci->hc_control, &ohci->regs->control);
		}
		ohci->ed_bulktail = ed;
		break;

	// case PIPE_INTERRUPT:
	// case PIPE_ISOCHRONOUS:
	default:
		branch = balance (ohci, ed->interval, ed->load);
		if (branch < 0) {
			ohci_dbg (ohci,
				"ERR %d, interval %d msecs, load %d\n",
				branch, ed->interval, ed->load);
			// FIXME if there are TDs queued, fail them!
			return branch;
		}
		ed->branch = branch;
		periodic_link (ohci, ed);
	}	 	

	/* the HC may not see the schedule updates yet, but if it does
	 * then they'll be properly ordered.
	 */
	return 0;
}

/*-------------------------------------------------------------------------*/

/* scan the periodic table to find and unlink this ED */
static void periodic_unlink (struct ohci_hcd *ohci, struct ed *ed)
{
	int	i;

	for (i = ed->branch; i < NUM_INTS; i += ed->interval) {
		struct ed	*temp;
		struct ed	**prev = &ohci->periodic [i];
		u32		*prev_p = &ohci->hcca->int_table [i];

		while (*prev && (temp = *prev) != ed) {
			prev_p = &temp->hwNextED;
			prev = &temp->ed_next;
		}
		if (*prev) {
			*prev_p = ed->hwNextED;
			*prev = ed->ed_next;
		}
		ohci->load [i] -= ed->load;
	}	
	hcd_to_bus (&ohci->hcd)->bandwidth_allocated -= ed->load / ed->interval;

	ohci_vdbg (ohci, "unlink %sed %p branch %d [%dus.], interval %d\n",
		(ed->hwINFO & ED_ISO) ? "iso " : "",
		ed, ed->branch, ed->load, ed->interval);
}

/* unlink an ed from one of the HC chains. 
 * just the link to the ed is unlinked.
 * the link from the ed still points to another operational ed or 0
 * so the HC can eventually finish the processing of the unlinked ed
 */
static void ed_deschedule (struct ohci_hcd *ohci, struct ed *ed) 
{
	ed->hwINFO |= ED_SKIP;

	switch (ed->type) {
	case PIPE_CONTROL:
		if (ed->ed_prev == NULL) {
			if (!ed->hwNextED) {
				ohci->hc_control &= ~OHCI_CTRL_CLE;
				writel (ohci->hc_control, &ohci->regs->control);
				writel (0, &ohci->regs->ed_controlcurrent);
				// post those pci writes
				(void) readl (&ohci->regs->control);
			}
			writel (le32_to_cpup (&ed->hwNextED),
				&ohci->regs->ed_controlhead);
		} else {
			ed->ed_prev->ed_next = ed->ed_next;
			ed->ed_prev->hwNextED = ed->hwNextED;
		}
		if (ohci->ed_controltail == ed) {
			ohci->ed_controltail = ed->ed_prev;
			if (ohci->ed_controltail)
				ohci->ed_controltail->ed_next = 0;
		} else if (ed->ed_next) {
			ed->ed_next->ed_prev = ed->ed_prev;
		}
		break;

	case PIPE_BULK:
		if (ed->ed_prev == NULL) {
			if (!ed->hwNextED) {
				ohci->hc_control &= ~OHCI_CTRL_BLE;
				writel (ohci->hc_control, &ohci->regs->control);
				writel (0, &ohci->regs->ed_bulkcurrent);
				// post those pci writes
				(void) readl (&ohci->regs->control);
			}
			writel (le32_to_cpup (&ed->hwNextED),
				&ohci->regs->ed_bulkhead);
		} else {
			ed->ed_prev->ed_next = ed->ed_next;
			ed->ed_prev->hwNextED = ed->hwNextED;
		}
		if (ohci->ed_bulktail == ed) {
			ohci->ed_bulktail = ed->ed_prev;
			if (ohci->ed_bulktail)
				ohci->ed_bulktail->ed_next = 0;
		} else if (ed->ed_next) {
			ed->ed_next->ed_prev = ed->ed_prev;
		}
		break;

	// case PIPE_INTERRUPT:
	// case PIPE_ISOCHRONOUS:
	default:
		periodic_unlink (ohci, ed);
		break;
	}

	/* NOTE: Except for a couple of exceptionally clean unlink cases
	 * (like unlinking the only c/b ED, with no TDs) HCs may still be
	 * caching this operational ED (or its address).  Safe unlinking
	 * involves not marking it ED_IDLE till INTR_SF; we always do that
	 * if td_list isn't empty.  Otherwise the race is small; but ...
	 */
	if (ed->state == ED_OPER) {
		ed->state = ED_IDLE;
		ed->hwINFO &= ~(ED_SKIP | ED_DEQUEUE);
		ed->hwHeadP &= ~ED_H;
		wmb ();
	}
}


/*-------------------------------------------------------------------------*/

/* get and maybe (re)init an endpoint. init _should_ be done only as part
 * of usb_set_configuration() or usb_set_interface() ... but the USB stack
 * isn't very stateful, so we re-init whenever the HC isn't looking.
 */
static struct ed *ed_get (
	struct ohci_hcd		*ohci,
	struct usb_device	*udev,
	unsigned int		pipe,
	int			interval
) {
	int			is_out = !usb_pipein (pipe);
	int			type = usb_pipetype (pipe);
	struct hcd_dev		*dev = (struct hcd_dev *) udev->hcpriv;
	struct ed		*ed; 
	unsigned		ep;
	unsigned long		flags;

	ep = usb_pipeendpoint (pipe) << 1;
	if (type != PIPE_CONTROL && is_out)
		ep |= 1;

	spin_lock_irqsave (&ohci->lock, flags);

	if (!(ed = dev->ep [ep])) {
		struct td	*td;

		ed = ed_alloc (ohci, SLAB_ATOMIC);
		if (!ed) {
			/* out of memory */
			goto done;
		}
		dev->ep [ep] = ed;

  		/* dummy td; end of td list for ed */
		td = td_alloc (ohci, SLAB_ATOMIC);
 		if (!td) {
			/* out of memory */
			ed_free (ohci, ed);
			ed = 0;
			goto done;
		}
		ed->dummy = td;
		ed->hwTailP = cpu_to_le32 (td->td_dma);
		ed->hwHeadP = ed->hwTailP;	/* ED_C, ED_H zeroed */
		ed->state = ED_IDLE;
		ed->type = type;
	}

	/* NOTE: only ep0 currently needs this "re"init logic, during
	 * enumeration (after set_address, or if ep0 maxpacket >8).
	 */
  	if (ed->state == ED_IDLE) {
		u32	info;

		info = usb_pipedevice (pipe);
		info |= (ep >> 1) << 7;
		info |= usb_maxpacket (udev, pipe, is_out) << 16;
		info = cpu_to_le32 (info);
		if (udev->speed == USB_SPEED_LOW)
			info |= ED_LOWSPEED;
		/* only control transfers store pids in tds */
		if (type != PIPE_CONTROL) {
			info |= is_out ? ED_OUT : ED_IN;
			if (type != PIPE_BULK) {
				/* periodic transfers... */
				if (type == PIPE_ISOCHRONOUS)
					info |= ED_ISO;
				else if (interval > 32)	/* iso can be bigger */
					interval = 32;
				ed->interval = interval;
				ed->load = usb_calc_bus_time (
					udev->speed, !is_out,
					type == PIPE_ISOCHRONOUS,
					usb_maxpacket (udev, pipe, is_out))
						/ 1000;
			}
		}
		ed->hwINFO = info;
	}

done:
	spin_unlock_irqrestore (&ohci->lock, flags);
	return ed; 
}

/*-------------------------------------------------------------------------*/

/* request unlinking of an endpoint from an operational HC.
 * put the ep on the rm_list
 * real work is done at the next start frame (SF) hardware interrupt
 */
static void start_urb_unlink (struct ohci_hcd *ohci, struct ed *ed)
{    
	ed->hwINFO |= ED_DEQUEUE;
	ed->state = ED_UNLINK;
	ed_deschedule (ohci, ed);

	/* SF interrupt might get delayed; record the frame counter value that
	 * indicates when the HC isn't looking at it, so concurrent unlinks
	 * behave.  frame_no wraps every 2^16 msec, and changes right before
	 * SF is triggered.
	 */
	ed->tick = le16_to_cpu (ohci->hcca->frame_no) + 1;

	/* rm_list is just singly linked, for simplicity */
	ed->ed_next = ohci->ed_rm_list;
	ed->ed_prev = 0;
	ohci->ed_rm_list = ed;

	/* enable SOF interrupt */
	if (!ohci->sleeping) {
		writel (OHCI_INTR_SF, &ohci->regs->intrstatus);
		writel (OHCI_INTR_SF, &ohci->regs->intrenable);
		// flush those pci writes
		(void) readl (&ohci->regs->control);
	}
}

/*-------------------------------------------------------------------------*
 * TD handling functions
 *-------------------------------------------------------------------------*/

/* enqueue next TD for this URB (OHCI spec 5.2.8.2) */

static void
td_fill (struct ohci_hcd *ohci, u32 info,
	dma_addr_t data, int len,
	struct urb *urb, int index)
{
	struct td		*td, *td_pt;
	struct urb_priv		*urb_priv = urb->hcpriv;
	int			is_iso = info & TD_ISO;
	int			hash;

	// ASSERT (index < urb_priv->length);

	/* aim for only one interrupt per urb.  mostly applies to control
	 * and iso; other urbs rarely need more than one TD per urb.
	 * this way, only final tds (or ones with an error) cause IRQs.
	 * at least immediately; use DI=6 in case any control request is
	 * tempted to die part way through.
	 *
	 * NOTE: could delay interrupts even for the last TD, and get fewer
	 * interrupts ... increasing per-urb latency by sharing interrupts.
	 * Drivers that queue bulk urbs may request that behavior.
	 */
	if (index != (urb_priv->length - 1)
			|| (urb->transfer_flags & URB_NO_INTERRUPT))
		info |= TD_DI_SET (6);

	/* use this td as the next dummy */
	td_pt = urb_priv->td [index];

	/* fill the old dummy TD */
	td = urb_priv->td [index] = urb_priv->ed->dummy;
	urb_priv->ed->dummy = td_pt;

	td->ed = urb_priv->ed;
	td->next_dl_td = NULL;
	td->index = index;
	td->urb = urb; 
	td->data_dma = data;
	if (!len)
		data = 0;

	td->hwINFO = cpu_to_le32 (info);
	if (is_iso) {
		td->hwCBP = cpu_to_le32 (data & 0xFFFFF000);
		td->hwPSW [0] = cpu_to_le16 ((data & 0x0FFF) | 0xE000);
		td->ed->last_iso = info & 0xffff;
	} else {
		td->hwCBP = cpu_to_le32 (data); 
	}			
	if (data)
		td->hwBE = cpu_to_le32 (data + len - 1);
	else
		td->hwBE = 0;
	td->hwNextTD = cpu_to_le32 (td_pt->td_dma);

	/* append to queue */
	list_add_tail (&td->td_list, &td->ed->td_list);

	/* hash it for later reverse mapping */
	hash = TD_HASH_FUNC (td->td_dma);
	td->td_hash = ohci->td_hash [hash];
	ohci->td_hash [hash] = td;

	/* HC might read the TD (or cachelines) right away ... */
	wmb ();
	td->ed->hwTailP = td->hwNextTD;
}

/*-------------------------------------------------------------------------*/

/* Prepare all TDs of a transfer, and queue them onto the ED.
 * Caller guarantees HC is active.
 * Usually the ED is already on the schedule, so TDs might be
 * processed as soon as they're queued.
 */
static void td_submit_urb (
	struct ohci_hcd	*ohci,
	struct urb	*urb
) {
	struct urb_priv	*urb_priv = urb->hcpriv;
	dma_addr_t	data;
	int		data_len = urb->transfer_buffer_length;
	int		cnt = 0;
	u32		info = 0;
	int		is_out = usb_pipeout (urb->pipe);

	/* OHCI handles the bulk/interrupt data toggles itself.  We just
	 * use the device toggle bits for resetting, and rely on the fact
	 * that resetting toggle is meaningless if the endpoint is active.
	 */
  	if (!usb_gettoggle (urb->dev, usb_pipeendpoint (urb->pipe), is_out)) {
		usb_settoggle (urb->dev, usb_pipeendpoint (urb->pipe),
			is_out, 1);
		urb_priv->ed->hwHeadP &= ~ED_C;
	}

	urb_priv->td_cnt = 0;

	if (data_len)
		data = urb->transfer_dma;
	else
		data = 0;

	/* NOTE:  TD_CC is set so we can tell which TDs the HC processed by
	 * using TD_CC_GET, as well as by seeing them on the done list.
	 * (CC = NotAccessed ... 0x0F, or 0x0E in PSWs for ISO.)
	 */
	switch (urb_priv->ed->type) {

	/* Bulk and interrupt are identical except for where in the schedule
	 * their EDs live.
	 */
	case PIPE_INTERRUPT:
		/* ... and periodic urbs have extra accounting */
		hcd_to_bus (&ohci->hcd)->bandwidth_int_reqs++;
		/* FALLTHROUGH */
	case PIPE_BULK:
		info = is_out
			? TD_T_TOGGLE | TD_CC | TD_DP_OUT
			: TD_T_TOGGLE | TD_CC | TD_DP_IN;
		/* TDs _could_ transfer up to 8K each */
		while (data_len > 4096) {
			td_fill (ohci, info, data, 4096, urb, cnt);
			data += 4096;
			data_len -= 4096;
			cnt++;
		}
		/* maybe avoid ED halt on final TD short read */
		if (!(urb->transfer_flags & URB_SHORT_NOT_OK))
			info |= TD_R;
		td_fill (ohci, info, data, data_len, urb, cnt);
		cnt++;
		if ((urb->transfer_flags & URB_ZERO_PACKET)
				&& cnt < urb_priv->length) {
			td_fill (ohci, info, 0, 0, urb, cnt);
			cnt++;
		}
		/* maybe kickstart bulk list */
		if (urb_priv->ed->type == PIPE_BULK) {
			wmb ();
			writel (OHCI_BLF, &ohci->regs->cmdstatus);
		}
		break;

	/* control manages DATA0/DATA1 toggle per-request; SETUP resets it,
	 * any DATA phase works normally, and the STATUS ack is special.
	 */
	case PIPE_CONTROL:
		info = TD_CC | TD_DP_SETUP | TD_T_DATA0;
		td_fill (ohci, info, urb->setup_dma, 8, urb, cnt++);
		if (data_len > 0) {
			info = TD_CC | TD_R | TD_T_DATA1;
			info |= is_out ? TD_DP_OUT : TD_DP_IN;
			/* NOTE:  mishandles transfers >8K, some >4K */
			td_fill (ohci, info, data, data_len, urb, cnt++);
		}
		info = is_out
			? TD_CC | TD_DP_IN | TD_T_DATA1
			: TD_CC | TD_DP_OUT | TD_T_DATA1;
		td_fill (ohci, info, data, 0, urb, cnt++);
		/* maybe kickstart control list */
		wmb ();
		writel (OHCI_CLF, &ohci->regs->cmdstatus);
		break;

	/* ISO has no retransmit, so no toggle; and it uses special TDs.
	 * Each TD could handle multiple consecutive frames (interval 1);
	 * we could often reduce the number of TDs here.
	 */
	case PIPE_ISOCHRONOUS:
		for (cnt = 0; cnt < urb->number_of_packets; cnt++) {
			int	frame = urb->start_frame;

			// FIXME scheduling should handle frame counter
			// roll-around ... exotic case (and OHCI has
			// a 2^16 iso range, vs other HCs max of 2^10)
			frame += cnt * urb->interval;
			frame &= 0xffff;
			td_fill (ohci, TD_CC | TD_ISO | frame,
				data + urb->iso_frame_desc [cnt].offset,
				urb->iso_frame_desc [cnt].length, urb, cnt);
		}
		hcd_to_bus (&ohci->hcd)->bandwidth_isoc_reqs++;
		break;
	}
	// ASSERT (urb_priv->length == cnt);
}

/*-------------------------------------------------------------------------*
 * Done List handling functions
 *-------------------------------------------------------------------------*/

/* calculate transfer length/status and update the urb
 * PRECONDITION:  irqsafe (only for urb->status locking)
 */
static void td_done (struct ohci_hcd *ohci, struct urb *urb, struct td *td)
{
	u32	tdINFO = le32_to_cpup (&td->hwINFO);
	int	cc = 0;

	list_del (&td->td_list);

	/* ISO ... drivers see per-TD length/status */
  	if (tdINFO & TD_ISO) {
 		u16	tdPSW = le16_to_cpu (td->hwPSW [0]);
		int	dlen = 0;

		/* NOTE:  assumes FC in tdINFO == 0 (and MAXPSW == 1) */

 		cc = (tdPSW >> 12) & 0xF;
  		if (tdINFO & TD_CC)	/* hc didn't touch? */
			return;

		if (usb_pipeout (urb->pipe))
			dlen = urb->iso_frame_desc [td->index].length;
		else {
			/* short reads are always OK for ISO */
			if (cc == TD_DATAUNDERRUN)
				cc = TD_CC_NOERROR;
			dlen = tdPSW & 0x3ff;
		}
		urb->actual_length += dlen;
		urb->iso_frame_desc [td->index].actual_length = dlen;
		urb->iso_frame_desc [td->index].status = cc_to_error [cc];

		if (cc != TD_CC_NOERROR)
			ohci_vdbg (ohci,
				"urb %p iso td %p (%d) len %d cc %d\n",
				urb, td, 1 + td->index, dlen, cc);

	/* BULK, INT, CONTROL ... drivers see aggregate length/status,
	 * except that "setup" bytes aren't counted and "short" transfers
	 * might not be reported as errors.
	 */
	} else {
		int	type = usb_pipetype (urb->pipe);
		u32	tdBE = le32_to_cpup (&td->hwBE);

  		cc = TD_CC_GET (tdINFO);

		/* control endpoints only have soft stalls */
  		if (type != PIPE_CONTROL && cc == TD_CC_STALL)
			usb_endpoint_halt (urb->dev,
				usb_pipeendpoint (urb->pipe),
				usb_pipeout (urb->pipe));

		/* update packet status if needed (short is normally ok) */
		if (cc == TD_DATAUNDERRUN
				&& !(urb->transfer_flags & URB_SHORT_NOT_OK))
			cc = TD_CC_NOERROR;
		if (cc != TD_CC_NOERROR && cc < 0x0E) {
			spin_lock (&urb->lock);
			if (urb->status == -EINPROGRESS)
				urb->status = cc_to_error [cc];
			spin_unlock (&urb->lock);
		}

		/* count all non-empty packets except control SETUP packet */
		if ((type != PIPE_CONTROL || td->index != 0) && tdBE != 0) {
			if (td->hwCBP == 0)
				urb->actual_length += tdBE - td->data_dma + 1;
			else
				urb->actual_length +=
					  le32_to_cpup (&td->hwCBP)
					- td->data_dma;
		}

		if (cc != TD_CC_NOERROR && cc < 0x0E)
			ohci_vdbg (ohci,
				"urb %p td %p (%d) cc %d, len=%d/%d\n",
				urb, td, 1 + td->index, cc,
				urb->actual_length,
				urb->transfer_buffer_length);
  	}
}

/*-------------------------------------------------------------------------*/

static inline struct td *
ed_halted (struct ohci_hcd *ohci, struct td *td, int cc, struct td *rev)
{
  	struct urb		*urb = td->urb;
	struct ed		*ed = td->ed;
	struct list_head	*tmp = td->td_list.next;
	u32			toggle = ed->hwHeadP & ED_C;

	/* clear ed halt; this is the td that caused it, but keep it inactive
	 * until its urb->complete() has a chance to clean up.
	 */
	ed->hwINFO |= ED_SKIP;
	wmb ();
	ed->hwHeadP &= ~ED_H; 

	/* put any later tds from this urb onto the donelist, after 'td',
	 * order won't matter here: no errors, and nothing was transferred.
	 * also patch the ed so it looks as if those tds completed normally.
	 */
	while (tmp != &ed->td_list) {
		struct td	*next;
		u32		info;

		next = list_entry (tmp, struct td, td_list);
		tmp = next->td_list.next;

		if (next->urb != urb)
			break;

		/* NOTE: if multi-td control DATA segments get supported,
		 * this urb had one of them, this td wasn't the last td
		 * in that segment (TD_R clear), this ed halted because
		 * of a short read, _and_ URB_SHORT_NOT_OK is clear ...
		 * then we need to leave the control STATUS packet queued
		 * and clear ED_SKIP.
		 */
		info = next->hwINFO;
		info |= cpu_to_le32 (TD_DONE);
		info &= ~cpu_to_le32 (TD_CC);
		next->hwINFO = info;

		next->next_dl_td = rev;	
		rev = next;

		if (ed->hwTailP == cpu_to_le32 (next->td_dma))
			ed->hwTailP = next->hwNextTD;
		ed->hwHeadP = next->hwNextTD | toggle;
	}

	/* help for troubleshooting:  report anything that
	 * looks odd ... that doesn't include protocol stalls
	 * (or maybe some other things)
	 */
	if (cc != TD_CC_STALL || !usb_pipecontrol (urb->pipe))
		ohci_dbg (ohci,
			"urb %p path %s ep%d%s %08x cc %d --> status %d\n",
			urb, urb->dev->devpath,
			usb_pipeendpoint (urb->pipe),
			usb_pipein (urb->pipe) ? "in" : "out",
			le32_to_cpu (td->hwINFO),
			cc, cc_to_error [cc]);

	return rev;
}

/* replies to the request have to be on a FIFO basis so
 * we unreverse the hc-reversed done-list
 */
static struct td *dl_reverse_done_list (struct ohci_hcd *ohci)
{
	u32		td_dma;
	struct td	*td_rev = NULL;
	struct td	*td = NULL;
  	unsigned long	flags;

  	spin_lock_irqsave (&ohci->lock, flags);
	td_dma = le32_to_cpup (&ohci->hcca->done_head);
	ohci->hcca->done_head = 0;

	/* get TD from hc's singly linked list, and
	 * prepend to ours.  ed->td_list changes later.
	 */
	while (td_dma) {		
	    	int		cc;

		td = dma_to_td (ohci, td_dma);
		if (!td) {
			ohci_err (ohci, "bad entry %8x\n", td_dma);
			break;
		}

		td->hwINFO |= cpu_to_le32 (TD_DONE);
		cc = TD_CC_GET (le32_to_cpup (&td->hwINFO));

		/* Non-iso endpoints can halt on error; un-halt,
		 * and dequeue any other TDs from this urb.
		 * No other TD could have caused the halt.
		 */
		if (cc != TD_CC_NOERROR && (td->ed->hwHeadP & ED_H))
			td_rev = ed_halted (ohci, td, cc, td_rev);

		td->next_dl_td = td_rev;	
		td_rev = td;
		td_dma = le32_to_cpup (&td->hwNextTD);
	}	
	spin_unlock_irqrestore (&ohci->lock, flags);
	return td_rev;
}

/*-------------------------------------------------------------------------*/

/* wrap-aware logic stolen from <linux/jiffies.h> */
#define tick_before(t1,t2) ((((s16)(t1))-((s16)(t2))) < 0)

/* there are some urbs/eds to unlink; called in_irq(), with HCD locked */
static void
finish_unlinks (struct ohci_hcd *ohci, u16 tick, struct pt_regs *regs)
{
	struct ed	*ed, **last;

rescan_all:
	for (last = &ohci->ed_rm_list, ed = *last; ed != NULL; ed = *last) {
		struct list_head	*entry, *tmp;
		int			completed, modified;
		u32			*prev;

		/* only take off EDs that the HC isn't using, accounting for
		 * frame counter wraps.
		 */
		if (tick_before (tick, ed->tick) && !ohci->disabled) {
			last = &ed->ed_next;
			continue;
		}

		/* reentrancy:  if we drop the schedule lock, someone might
		 * have modified this list.  normally it's just prepending
		 * entries (which we'd ignore), but paranoia won't hurt.
		 */
		*last = ed->ed_next;
		ed->ed_next = 0;
		modified = 0;

		/* unlink urbs as requested, but rescan the list after
		 * we call a completion since it might have unlinked
		 * another (earlier) urb
		 */
rescan_this:
		completed = 0;
		prev = &ed->hwHeadP;
		list_for_each_safe (entry, tmp, &ed->td_list) {
			struct td	*td;
			struct urb	*urb;
			urb_priv_t	*urb_priv;
			u32		savebits;

			td = list_entry (entry, struct td, td_list);
			urb = td->urb;
			urb_priv = td->urb->hcpriv;

			if (urb_priv->state != URB_DEL) {
				prev = &td->hwNextTD;
				continue;
			}

			/* patch pointers hc uses ... tail, if we're removing
			 * an otherwise active td, and whatever td pointer
			 * points to this td
			 */
			if (ed->hwTailP == cpu_to_le32 (td->td_dma))
				ed->hwTailP = td->hwNextTD;
			savebits = *prev & ~cpu_to_le32 (TD_MASK);
			*prev = td->hwNextTD | savebits;

			/* HC may have partly processed this TD */
			td_done (ohci, urb, td);
			urb_priv->td_cnt++;

			/* if URB is done, clean up */
			if (urb_priv->td_cnt == urb_priv->length) {
				modified = completed = 1;
				spin_unlock (&ohci->lock);
				finish_urb (ohci, urb, regs);
				spin_lock (&ohci->lock);
			}
		}
		if (completed && !list_empty (&ed->td_list))
			goto rescan_this;

		/* ED's now officially unlinked, hc doesn't see */
		ed->state = ED_IDLE;
		ed->hwINFO &= ~(ED_SKIP | ED_DEQUEUE);
		ed->hwHeadP &= ~ED_H;
		ed->hwNextED = 0;

		/* but if there's work queued, reschedule */
		if (!list_empty (&ed->td_list)) {
			if (!ohci->disabled && !ohci->sleeping)
				ed_schedule (ohci, ed);
		}

		if (modified)
			goto rescan_all;
   	}

	/* maybe reenable control and bulk lists */ 
	if (!ohci->disabled && !ohci->ed_rm_list) {
		u32	command = 0, control = 0;

		if (ohci->ed_controltail) {
			command |= OHCI_CLF;
			if (!(ohci->hc_control & OHCI_CTRL_CLE)) {
				control |= OHCI_CTRL_CLE;
				writel (0, &ohci->regs->ed_controlcurrent);
			}
		}
		if (ohci->ed_bulktail) {
			command |= OHCI_BLF;
			if (!(ohci->hc_control & OHCI_CTRL_BLE)) {
				control |= OHCI_CTRL_BLE;
				writel (0, &ohci->regs->ed_bulkcurrent);
			}
		}
		
		/* CLE/BLE to enable, CLF/BLF to (maybe) kickstart */
		if (control) {
			ohci->hc_control |= control;
 			writel (ohci->hc_control, &ohci->regs->control);   
 		}
		if (command)
			writel (command, &ohci->regs->cmdstatus);   
 	}
}



/*-------------------------------------------------------------------------*/

/*
 * Process normal completions (error or success) and clean the schedules.
 *
 * This is the main path for handing urbs back to drivers.  The only other
 * path is finish_unlinks(), which unlinks URBs using ed_rm_list, instead of
 * scanning the (re-reversed) donelist as this does.
 */
static void
dl_done_list (struct ohci_hcd *ohci, struct td *td, struct pt_regs *regs)
{
	unsigned long	flags;

  	spin_lock_irqsave (&ohci->lock, flags);
  	while (td) {
		struct td	*td_next = td->next_dl_td;
		struct urb	*urb = td->urb;
		urb_priv_t	*urb_priv = urb->hcpriv;
		struct ed	*ed = td->ed;

		/* update URB's length and status from TD */
   		td_done (ohci, urb, td);
  		urb_priv->td_cnt++;

		/* If all this urb's TDs are done, call complete() */
  		if (urb_priv->td_cnt == urb_priv->length) {
     			spin_unlock (&ohci->lock);
  			finish_urb (ohci, urb, regs);
  			spin_lock (&ohci->lock);
  		}

		/* clean schedule:  unlink EDs that are no longer busy */
		if (list_empty (&ed->td_list))
			ed_deschedule (ohci, ed);
		/* ... reenabling halted EDs only after fault cleanup */
		else if (!(ed->hwINFO & ED_DEQUEUE)) {
			td = list_entry (ed->td_list.next, struct td, td_list);
			if (!(td->hwINFO & TD_DONE))
				ed->hwINFO &= ~ED_SKIP;
		}

    		td = td_next;
  	}  
	spin_unlock_irqrestore (&ohci->lock, flags);
}
