/*
 * Copyright (c) 2007 by Aleksey Bragin
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __OHCI_H__
#define __OHCI_H__

typedef struct _OHCI_DEV
{
    HCD     hcd_interf;
} OHCI_DEV, *POHCI_DEV;

typedef struct _OHCI_DEVICE_EXTENSION
{
    DEVEXT_HEADER		dev_ext_hdr;
    PDEVICE_OBJECT     	pdev_obj;
    PDRIVER_OBJECT  	pdrvr_obj;
    POHCI_DEV 			ohci;

    ULONG               pci_addr;	// bus number | slot number | funciton number
    UHCI_INTERRUPT   	res_interrupt;

    PKINTERRUPT         ohci_int;
    KDPC                ohci_dpc;
} OHCI_DEVICE_EXTENSION, *POHCI_DEVICE_EXTENSION;


#endif /* __OHCI_H__ */
