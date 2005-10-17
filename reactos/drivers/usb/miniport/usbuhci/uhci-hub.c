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
 */

static __u8 root_hub_hub_des[] =
{
	0x09,			/*  __u8  bLength; */
	0x29,			/*  __u8  bDescriptorType; Hub-descriptor */
	0x02,			/*  __u8  bNbrPorts; */
	0x00,			/* __u16  wHubCharacteristics; */
	0x00,
	0x01,			/*  __u8  bPwrOn2pwrGood; 2ms */
	0x00,			/*  __u8  bHubContrCurrent; 0 mA */
	0x00,			/*  __u8  DeviceRemovable; *** 7 Ports max *** */
	0xff			/*  __u8  PortPwrCtrlMask; *** 7 ports max *** */
};

static int uhci_hub_status_data(struct usb_hcd *hcd, char *buf)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	unsigned int io_addr = uhci->io_addr;
	int i, len = 1;

	*buf = 0;
	for (i = 0; i < uhci->rh_numports; i++) {
		*buf |= ((inw(io_addr + USBPORTSC1 + i * 2) & 0xa) > 0 ? (1 << (i + 1)) : 0);
		len = (i + 1) / 8 + 1;
	}

	return !!*buf;
}

#define OK(x)			len = (x); break

#define CLR_RH_PORTSTAT(x) \
	status = inw(io_addr + USBPORTSC1 + 2 * (wIndex-1)); \
	status = (status & 0xfff5) & ~(x); \
	outw(status, io_addr + USBPORTSC1 + 2 * (wIndex-1))

#define SET_RH_PORTSTAT(x) \
	status = inw(io_addr + USBPORTSC1 + 2 * (wIndex-1)); \
	status = (status & 0xfff5) | (x); \
	outw(status, io_addr + USBPORTSC1 + 2 * (wIndex-1))


/* size of returned buffer is part of USB spec */
static int uhci_hub_control(struct usb_hcd *hcd, u16 typeReq, u16 wValue,
			u16 wIndex, char *buf, u16 wLength)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	int i, status, retval = 0, len = 0;
	unsigned int io_addr = uhci->io_addr;
	__u16 cstatus;
	char c_p_r[8];

	for (i = 0; i < 8; i++)
		c_p_r[i] = 0;

	switch (typeReq) {
		/* Request Destination:
		   without flags: Device,
		   RH_INTERFACE: interface,
		   RH_ENDPOINT: endpoint,
		   RH_CLASS means HUB here,
		   RH_OTHER | RH_CLASS  almost ever means HUB_PORT here
		*/

	case GetHubStatus:
		*(__u32 *)buf = cpu_to_le32(0);
		OK(4);		/* hub power */
	case GetPortStatus:
		status = inw(io_addr + USBPORTSC1 + 2 * (wIndex - 1));
		cstatus = ((status & USBPORTSC_CSC) >> (1 - 0)) |
			((status & USBPORTSC_PEC) >> (3 - 1)) |
			(c_p_r[wIndex - 1] << (0 + 4));
			status = (status & USBPORTSC_CCS) |
			((status & USBPORTSC_PE) >> (2 - 1)) |
			((status & USBPORTSC_SUSP) >> (12 - 2)) |
			((status & USBPORTSC_PR) >> (9 - 4)) |
			(1 << 8) |      /* power on */
			((status & USBPORTSC_LSDA) << (-8 + 9));

		*(__u16 *)buf = cpu_to_le16(status);
		*(__u16 *)(buf + 2) = cpu_to_le16(cstatus);
		OK(4);
	case SetHubFeature:
		switch (wValue) {
		case C_HUB_OVER_CURRENT:
		case C_HUB_LOCAL_POWER:
			break;
		default:
			goto err;
		}
		break;
	case ClearHubFeature:
		switch (wValue) {
		case C_HUB_OVER_CURRENT:
			OK(0);	/* hub power over current */
		default:
			goto err;
		}
		break;
	case SetPortFeature:
		if (!wIndex || wIndex > uhci->rh_numports)
			goto err;

		switch (wValue) {
		case USB_PORT_FEAT_SUSPEND:
			SET_RH_PORTSTAT(USBPORTSC_SUSP);
			OK(0);
		case USB_PORT_FEAT_RESET:
			SET_RH_PORTSTAT(USBPORTSC_PR);
			mdelay(50);	/* USB v1.1 7.1.7.3 */
			c_p_r[wIndex - 1] = 1;
			CLR_RH_PORTSTAT(USBPORTSC_PR);
			udelay(10);
			SET_RH_PORTSTAT(USBPORTSC_PE);
			mdelay(10);
			SET_RH_PORTSTAT(0xa);
			OK(0);
		case USB_PORT_FEAT_POWER:
			OK(0); /* port power ** */
		case USB_PORT_FEAT_ENABLE:
			SET_RH_PORTSTAT(USBPORTSC_PE);
			OK(0);
		default:
			goto err;
		}
		break;
	case ClearPortFeature:
		if (!wIndex || wIndex > uhci->rh_numports)
			goto err;

		switch (wValue) {
		case USB_PORT_FEAT_ENABLE:
			CLR_RH_PORTSTAT(USBPORTSC_PE);
			OK(0);
		case USB_PORT_FEAT_C_ENABLE:
			SET_RH_PORTSTAT(USBPORTSC_PEC);
			OK(0);
		case USB_PORT_FEAT_SUSPEND:
			CLR_RH_PORTSTAT(USBPORTSC_SUSP);
			OK(0);
		case USB_PORT_FEAT_C_SUSPEND:
			/*** WR_RH_PORTSTAT(RH_PS_PSSC); */
			OK(0);
		case USB_PORT_FEAT_POWER:
			OK(0);	/* port power */
		case USB_PORT_FEAT_C_CONNECTION:
			SET_RH_PORTSTAT(USBPORTSC_CSC);
			OK(0);
		case USB_PORT_FEAT_C_OVER_CURRENT:
			OK(0);	/* port power over current */
		case USB_PORT_FEAT_C_RESET:
			c_p_r[wIndex - 1] = 0;
			OK(0);
		default:
			goto err;
		}
		break;
	case GetHubDescriptor:
		len = min_t(unsigned int, wLength,
			  min_t(unsigned int, sizeof(root_hub_hub_des), wLength));
		memcpy(buf, root_hub_hub_des, len);
		if (len > 2)
			buf[2] = uhci->rh_numports;
		OK(len);
	default:
err:
		retval = -EPIPE;
	}

	return retval;
}

