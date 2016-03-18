/*
 * Copyright 2007-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef HDAC_REGS_H
#define HDAC_REGS_H

#ifndef __REACTOS__
#include <SupportDefs.h>
#endif

/* Controller register definitions */
#define HDAC_GLOBAL_CAP					0x00	// 16bits, GCAP
#define GLOBAL_CAP_OUTPUT_STREAMS(cap)	(((cap) >> 12) & 15)
#define GLOBAL_CAP_INPUT_STREAMS(cap)	(((cap) >> 8) & 15)
#define GLOBAL_CAP_BIDIR_STREAMS(cap)	(((cap) >> 3) & 15)
#define GLOBAL_CAP_NUM_SDO(cap)			((((cap) >> 1) & 3) ? (cap & 6) : 1)
#define GLOBAL_CAP_64BIT(cap)			(((cap) & 1) != 0)

#define HDAC_VERSION_MINOR				0x02	// 8bits, VMIN
#define HDAC_VERSION_MAJOR				0x03	// 8bits, VMAJ

#define HDAC_GLOBAL_CONTROL				0x08	// 32bits, GCTL
#define GLOBAL_CONTROL_UNSOLICITED		(1 << 8)
	// accept unsolicited responses
#define GLOBAL_CONTROL_FLUSH			(1 << 1)
#define GLOBAL_CONTROL_RESET			(1 << 0)

#define HDAC_WAKE_ENABLE				0x0c	// 16bits, WAKEEN
#define HDAC_WAKE_ENABLE_MASK			0x7fff
#define HDAC_STATE_STATUS				0x0e	// 16bits, STATESTS

#define HDAC_INTR_CONTROL				0x20	// 32bits, INTCTL
#define INTR_CONTROL_GLOBAL_ENABLE		(1U << 31)
#define INTR_CONTROL_CONTROLLER_ENABLE	(1 << 30)

#define HDAC_INTR_STATUS				0x24	// 32bits, INTSTS
#define INTR_STATUS_GLOBAL				(1U << 31)
#define INTR_STATUS_CONTROLLER			(1 << 30)
#define INTR_STATUS_STREAM_MASK			0x3fffffff

#define HDAC_CORB_BASE_LOWER			0x40	// 32bits, CORBLBASE
#define HDAC_CORB_BASE_UPPER			0x44	// 32bits, CORBUBASE
#define HDAC_CORB_WRITE_POS				0x48	// 16bits, CORBWP
#define HDAC_CORB_WRITE_POS_MASK		0xff

#define HDAC_CORB_READ_POS				0x4a	// 16bits, CORBRP
#define CORB_READ_POS_RESET				(1 << 15)

#define HDAC_CORB_CONTROL				0x4c	// 8bits, CORBCTL
#define HDAC_CORB_CONTROL_MASK			0x3
#define CORB_CONTROL_RUN				(1 << 1)
#define CORB_CONTROL_MEMORY_ERROR_INTR	(1 << 0)

#define HDAC_CORB_STATUS				0x4d	// 8bits, CORBSTS
#define CORB_STATUS_MEMORY_ERROR		(1 << 0)

#define HDAC_CORB_SIZE					0x4e	// 8bits, CORBSIZE
#define HDAC_CORB_SIZE_MASK				0x3
#define CORB_SIZE_CAP_2_ENTRIES			(1 << 4)
#define CORB_SIZE_CAP_16_ENTRIES		(1 << 5)
#define CORB_SIZE_CAP_256_ENTRIES		(1 << 6)
#define CORB_SIZE_2_ENTRIES				0x00	// 8 byte
#define CORB_SIZE_16_ENTRIES			0x01	// 64 byte
#define CORB_SIZE_256_ENTRIES			0x02	// 1024 byte

#define HDAC_RIRB_BASE_LOWER			0x50	// 32bits, RIRBLBASE
#define HDAC_RIRB_BASE_UPPER			0x54	// 32bits, RIRBUBASE

#define HDAC_RIRB_WRITE_POS				0x58	// 16bits, RIRBWP
#define RIRB_WRITE_POS_RESET			(1 << 15)

#define HDAC_RESPONSE_INTR_COUNT		0x5a	// 16bits, RINTCNT
#define HDAC_RESPONSE_INTR_COUNT_MASK	0xff

#define HDAC_RIRB_CONTROL				0x5c	// 8bits, RIRBCTL
#define HDAC_RIRB_CONTROL_MASK			0x7
#define RIRB_CONTROL_OVERRUN_INTR		(1 << 2)
#define RIRB_CONTROL_DMA_ENABLE			(1 << 1)
#define RIRB_CONTROL_RESPONSE_INTR		(1 << 0)

#define HDAC_RIRB_STATUS				0x5d	// 8bits, RIRBSTS
#define RIRB_STATUS_OVERRUN				(1 << 2)
#define RIRB_STATUS_RESPONSE			(1 << 0)

#define HDAC_RIRB_SIZE					0x5e	// 8bits, RIRBSIZE
#define HDAC_RIRB_SIZE_MASK				0x3
#define RIRB_SIZE_CAP_2_ENTRIES			(1 << 4)
#define RIRB_SIZE_CAP_16_ENTRIES		(1 << 5)
#define RIRB_SIZE_CAP_256_ENTRIES		(1 << 6)
#define RIRB_SIZE_2_ENTRIES				0x00
#define RIRB_SIZE_16_ENTRIES			0x01
#define RIRB_SIZE_256_ENTRIES			0x02

#define HDAC_DMA_POSITION_BASE_LOWER	0x70	// 32bits, DPLBASE
#define HDAC_DMA_POSITION_BASE_UPPER	0x74	// 32bits, DPUBASE
#define DMA_POSITION_ENABLED			1

/* Stream Descriptor Registers */
#define HDAC_STREAM_BASE				0x80
#define HDAC_STREAM_SIZE				0x20

#define HDAC_STREAM_CONTROL0			0x00	// 8bits, CTL0
#define CONTROL0_RESET					(1 << 0)
#define CONTROL0_RUN					(1 << 1)
#define CONTROL0_BUFFER_COMPLETED_INTR	(1 << 2)
#define CONTROL0_FIFO_ERROR_INTR		(1 << 3)
#define CONTROL0_DESCRIPTOR_ERROR_INTR	(1 << 4)
#define HDAC_STREAM_CONTROL1			0x01	// 8bits, CTL1
#define HDAC_STREAM_CONTROL2			0x02	// 8bits, CTL2
#define CONTROL2_STREAM_MASK			0xf0
#define CONTROL2_STREAM_SHIFT			4
#define CONTROL2_BIDIR					(1 << 3)
#define CONTROL2_TRAFFIC_PRIORITY		(1 << 2)
#define CONTROL2_STRIPE_SDO_MASK		0x03
#define HDAC_STREAM_STATUS				0x03	// 8bits, STS
#define STATUS_BUFFER_COMPLETED			(1 << 2)
#define STATUS_FIFO_ERROR				(1 << 3)
#define STATUS_DESCRIPTOR_ERROR			(1 << 4)
#define STATUS_FIFO_READY				(1 << 5)
#define HDAC_STREAM_POSITION			0x04	// 32bits, LPIB
#define HDAC_STREAM_BUFFER_SIZE			0x08	// 32bits, CBL
#define HDAC_STREAM_LAST_VALID			0x0c	// 16bits, LVI
#define HDAC_STREAM_FIFO_SIZE			0x10	// 16bits, FIFOS
#define HDAC_STREAM_FORMAT				0x12	// 16bits, FMT
#define FORMAT_8BIT						(0 << 4)
#define FORMAT_16BIT					(1 << 4)
#define FORMAT_20BIT					(2 << 4)
#define FORMAT_24BIT					(3 << 4)
#define FORMAT_32BIT					(4 << 4)
#define FORMAT_44_1_BASE_RATE			(1 << 14)
#define FORMAT_MULTIPLY_RATE_SHIFT		11
#define FORMAT_DIVIDE_RATE_SHIFT		8
#define HDAC_STREAM_BUFFERS_BASE_LOWER	0x18	// 32bits, BDPL
#define HDAC_STREAM_BUFFERS_BASE_UPPER	0x1c	// 32bits, BDPU

/* PCI space register definitions */
#define PCI_HDA_TCSEL					0x44
#define PCI_HDA_TCSEL_MASK				0xf8

#define ATI_HDA_MISC_CNTR2				0x42
#define ATI_HDA_MISC_CNTR2_MASK   		0xf8
#define ATI_HDA_ENABLE_SNOOP      		0x02
#define NVIDIA_HDA_OSTRM_COH			0x4c
#define NVIDIA_HDA_ISTRM_COH			0x4d
#define NVIDIA_HDA_ENABLE_COHBIT		0x01
#define NVIDIA_HDA_TRANSREG				0x4e
#define NVIDIA_HDA_TRANSREG_MASK		0xf0
#define NVIDIA_HDA_ENABLE_COHBITS		0x0f
#define INTEL_SCH_HDA_DEVC				0x78
#define INTEL_SCH_HDA_DEVC_SNOOP		0x800


#ifndef __REACTOS__
typedef uint32 corb_t;
typedef struct {
	uint32 response;
	uint32 flags;
} rirb_t;
#endif

#define RESPONSE_FLAGS_CODEC_MASK		0x0000000f
#define RESPONSE_FLAGS_UNSOLICITED		(1 << 4)

#ifndef __REACTOS__
typedef struct {
	uint32	lower;
	uint32	upper;
	uint32	length;
	uint32	ioc;
} bdl_entry_t;
#endif

#endif /* HDAC_REGS_H */
