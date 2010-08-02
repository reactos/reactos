/*
 * ReactOS AMD PCNet Driver
 *
 * Copyright (C) 2003 Vizzini <vizzini@plasmic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * PURPOSE:
 *     PCI configuration constants
 * REVISIONS:
 *     01-Sept-2003 vizzini - Created
 */

#pragma once

/* PCI Config Space Offset Definitions */
#define PCI_PCIID    0x0        /* pci id - query 32 bits */
#define PCI_VENID    0x0        /* vendor ID */
#define PCI_DEVID    0x2        /* device ID */
#define PCI_COMMAND  0x4        /* command register */
#define PCI_STATUS   0x6        /* status register */
#define PCI_REVID    0x8        /* revision ID */
#define PCI_PIR      0x9        /* programming interface register */
#define PCI_SCR      0xa        /* sub-class register */
#define PCI_BCR      0xb        /* base-class register */
#define PCI_LTR      0xd        /* latency timer register */
#define PCI_HTR      0xe        /* header type register */
#define PCI_IOBAR    0x10       /* i/o base address register */
#define PCI_MMBAR    0x14       /* i/o memory-mapped base address register */
#define PCI_ERBAR    0x30       /* expansion rom base address register */
#define PCI_ILR      0x3c       /* interrupt line register */
#define PCI_IPR      0x3d       /* interrupt pin register */
#define PCI_MINGNT   0x3e       /* min_gnt register */
#define PCI_MAXLAT   0x3f       /* max_lat register */

/* PCI Command Register Bits */
#define PCI_IOEN     0x1        /* i/o space access enable */
#define PCI_MEMEN    0x2        /* memory space access enable */
#define PCI_BMEN     0x4        /* bus master enable */
#define PCI_SCYCEN   0x8        /* special cycle enable */
#define PCI_MWIEN    0X10       /* memory write and invalidate cycle enable */
#define PCI_VGASNOOP 0x20       /* vga palette snoop */
#define PCI_PERREN   0x40       /* parity error response enable */
#define PCI_ADSTEP   0x80       /* address/data stepping */
#define PCI_SERREN   0x100      /* signalled error enable */
#define PCI_FBTBEN   0X200      /* fast back-to-back enable */

/* PCI Status Register Bits */
#define PCI_FBTBC    0x80       /* fast back-to-back capable */
#define PCI_DATAPERR 0x100      /* data parity error detected */
#define PCI_DEVSEL1  0x200      /* device select timing lsb */
#define PCI_DEVSEL2  0x400      /* device select timing msb */
#define PCI_STABORT  0x800      /* send target abort */
#define PCI_RTABORT  0x1000     /* received target abort */
#define PCI_SERR     0x2000     /* signalled error */
#define PCI_PERR     0x4000     /* parity error */
