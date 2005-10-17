/*
 * OHCI HCD (Host Controller Driver) for USB.
 * 
 * (C) Copyright 1999 Roman Weissgaerber <weissg@vienna.at>
 * (C) Copyright 2000-2002 David Brownell <dbrownell@users.sourceforge.net>
 * 
 * This file is licenced under GPL
 */

/*-------------------------------------------------------------------------*/

/*
 * OHCI Root Hub ... the nonsharable stuff
 *
 * Registers don't need cpu_to_le32, that happens transparently
 */

/* AMD-756 (D2 rev) reports corrupt register contents in some cases.
 * The erratum (#4) description is incorrect.  AMD's workaround waits
 * till some bits (mostly reserved) are clear; ok for all revs.
 */
#define read_roothub(hc, register, mask) ({ \
	u32 temp = readl (&hc->regs->roothub.register); \
	if (temp == -1) \
		disable (hc); \
	else if (hc->flags & OHCI_QUIRK_AMD756) \
		while (temp & mask) \
			temp = readl (&hc->regs->roothub.register); \
	temp; })

static u32 roothub_a (struct ohci_hcd *hc)
	{ return read_roothub (hc, a, 0xfc0fe000); }
static inline u32 roothub_b (struct ohci_hcd *hc)
	{ return readl (&hc->regs->roothub.b); }
static inline u32 roothub_status (struct ohci_hcd *hc)
	{ return readl (&hc->regs->roothub.status); }
static u32 roothub_portstatus (struct ohci_hcd *hc, int i)
	{ return read_roothub (hc, portstatus [i], 0xffe0fce0); }

/*-------------------------------------------------------------------------*/

#define dbg_port(hc,label,num,value) \
	ohci_dbg (hc, \
		"%s roothub.portstatus [%d] " \
		"= 0x%08x%s%s%s%s%s%s%s%s%s%s%s%s\n", \
		label, num, temp, \
		(temp & RH_PS_PRSC) ? " PRSC" : "", \
		(temp & RH_PS_OCIC) ? " OCIC" : "", \
		(temp & RH_PS_PSSC) ? " PSSC" : "", \
		(temp & RH_PS_PESC) ? " PESC" : "", \
		(temp & RH_PS_CSC) ? " CSC" : "", \
 		\
		(temp & RH_PS_LSDA) ? " LSDA" : "", \
		(temp & RH_PS_PPS) ? " PPS" : "", \
		(temp & RH_PS_PRS) ? " PRS" : "", \
		(temp & RH_PS_POCI) ? " POCI" : "", \
		(temp & RH_PS_PSS) ? " PSS" : "", \
 		\
		(temp & RH_PS_PES) ? " PES" : "", \
		(temp & RH_PS_CCS) ? " CCS" : "" \
		);


/*-------------------------------------------------------------------------*/

/* build "status change" packet (one or two bytes) from HC registers */

static int
ohci_hub_status_data (struct usb_hcd *hcd, char *buf)
{
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);
	int		ports, i, changed = 0, length = 1;

	ports = roothub_a (ohci) & RH_A_NDP; 
	if (ports > MAX_ROOT_PORTS) {
		if (ohci->disabled)
			return -ESHUTDOWN;
		ohci_err (ohci, "bogus NDP=%d, rereads as NDP=%d\n",
			ports, readl (&ohci->regs->roothub.a) & RH_A_NDP);
		/* retry later; "should not happen" */
		return 0;
	}

	/* init status */
	if (roothub_status (ohci) & (RH_HS_LPSC | RH_HS_OCIC))
		buf [0] = changed = 1;
	else
		buf [0] = 0;
	if (ports > 7) {
		buf [1] = 0;
		length++;
	}

	/* look at each port */
	for (i = 0; i < ports; i++) {
		u32	status = roothub_portstatus (ohci, i);

		status &= RH_PS_CSC | RH_PS_PESC | RH_PS_PSSC
				| RH_PS_OCIC | RH_PS_PRSC;
		if (status) {
			changed = 1;
			if (i < 7)
			    buf [0] |= 1 << (i + 1);
			else
			    buf [1] |= 1 << (i - 7);
		}
	}
	return changed ? length : 0;
}

/*-------------------------------------------------------------------------*/

static void
ohci_hub_descriptor (
	struct ohci_hcd			*ohci,
	struct usb_hub_descriptor	*desc
) {
	u32		rh = roothub_a (ohci);
	int		ports = rh & RH_A_NDP; 
	u16		temp;

	desc->bDescriptorType = 0x29;
	desc->bPwrOn2PwrGood = (rh & RH_A_POTPGT) >> 24;
	desc->bHubContrCurrent = 0;

	desc->bNbrPorts = ports;
	temp = 1 + (ports / 8);
	desc->bDescLength = 7 + 2 * temp;

	temp = 0;
	if (rh & RH_A_PSM) 		/* per-port power switching? */
	    temp |= 0x0001;
	if (rh & RH_A_NOCP)		/* no overcurrent reporting? */
	    temp |= 0x0010;
	else if (rh & RH_A_OCPM)	/* per-port overcurrent reporting? */
	    temp |= 0x0008;
	desc->wHubCharacteristics = cpu_to_le16 (temp);

	/* two bitmaps:  ports removable, and usb 1.0 legacy PortPwrCtrlMask */
	rh = roothub_b (ohci);
	desc->bitmap [0] = rh & RH_B_DR;
	if (ports > 7) {
		desc->bitmap [1] = (rh & RH_B_DR) >> 8;
		desc->bitmap [2] = desc->bitmap [3] = 0xff;
	} else
		desc->bitmap [1] = 0xff;
}

/*-------------------------------------------------------------------------*/

static int ohci_hub_control (
	struct usb_hcd	*hcd,
	u16		typeReq,
	u16		wValue,
	u16		wIndex,
	char		*buf,
	u16		wLength
) {
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);
	int		ports = hcd_to_bus (hcd)->root_hub->maxchild;
	u32		temp;
	int		retval = 0;

	switch (typeReq) {
	case ClearHubFeature:
		switch (wValue) {
		case C_HUB_OVER_CURRENT:
			writel (RH_HS_OCIC, &ohci->regs->roothub.status);
		case C_HUB_LOCAL_POWER:
			break;
		default:
			goto error;
		}
		break;
	case ClearPortFeature:
		if (!wIndex || wIndex > ports)
			goto error;
		wIndex--;

		switch (wValue) {
		case USB_PORT_FEAT_ENABLE:
			temp = RH_PS_CCS;
			break;
		case USB_PORT_FEAT_C_ENABLE:
			temp = RH_PS_PESC;
			break;
		case USB_PORT_FEAT_SUSPEND:
			temp = RH_PS_POCI;
			break;
		case USB_PORT_FEAT_C_SUSPEND:
			temp = RH_PS_PSSC;
			break;
		case USB_PORT_FEAT_POWER:
			temp = RH_PS_LSDA;
			break;
		case USB_PORT_FEAT_C_CONNECTION:
			temp = RH_PS_CSC;
			break;
		case USB_PORT_FEAT_C_OVER_CURRENT:
			temp = RH_PS_OCIC;
			break;
		case USB_PORT_FEAT_C_RESET:
			temp = RH_PS_PRSC;
			break;
		default:
			goto error;
		}
		writel (temp, &ohci->regs->roothub.portstatus [wIndex]);
		// readl (&ohci->regs->roothub.portstatus [wIndex]);
		break;
	case GetHubDescriptor:
		ohci_hub_descriptor (ohci, (struct usb_hub_descriptor *) buf);
		break;
	case GetHubStatus:
		temp = roothub_status (ohci) & ~(RH_HS_CRWE | RH_HS_DRWE);
		*(u32 *) buf = cpu_to_le32 (temp);
		break;
	case GetPortStatus:
		if (!wIndex || wIndex > ports)
			goto error;
		wIndex--;
		temp = roothub_portstatus (ohci, wIndex);
		*(u32 *) buf = cpu_to_le32 (temp);

#ifndef	OHCI_VERBOSE_DEBUG
	if (*(u16*)(buf+2))	/* only if wPortChange is interesting */
#endif
		dbg_port (ohci, "GetStatus", wIndex + 1, temp);
		break;
	case SetHubFeature:
		switch (wValue) {
		case C_HUB_OVER_CURRENT:
			// FIXME:  this can be cleared, yes?
		case C_HUB_LOCAL_POWER:
			break;
		default:
			goto error;
		}
		break;
	case SetPortFeature:
		if (!wIndex || wIndex > ports)
			goto error;
		wIndex--;
		switch (wValue) {
		case USB_PORT_FEAT_SUSPEND:
			writel (RH_PS_PSS,
				&ohci->regs->roothub.portstatus [wIndex]);
			break;
		case USB_PORT_FEAT_POWER:
			writel (RH_PS_PPS,
				&ohci->regs->roothub.portstatus [wIndex]);
			break;
		case USB_PORT_FEAT_RESET:
			temp = readl (&ohci->regs->roothub.portstatus [wIndex]);
			if (temp & RH_PS_CCS)
				writel (RH_PS_PRS,
				    &ohci->regs->roothub.portstatus [wIndex]);
			break;
		default:
			goto error;
		}
		break;

	default:
error:
		/* "protocol stall" on error */
		retval = -EPIPE;
	}
	return retval;
}

