/*
 * Copyright 2007-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _HDA_H_
#define _HDA_H_

#ifndef __REACTOS__
#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>
#include <PCI_x86.h>

#include <string.h>
#include <stdlib.h>

#ifndef HAIKU_TARGET_PLATFORM_HAIKU
#	define DEVFS_PATH_FORMAT	"audio/multi/hda/%lu"
#	include <multi_audio.h>
#else
#	define DEVFS_PATH_FORMAT	"audio/hmulti/hda/%lu"
#	include <hmulti_audio.h>
#endif
#endif

#include "hda_controller_defs.h"
#include "hda_codec_defs.h"

#define MAX_CARDS				4

/* values for the class_sub field for class_base = 0x04 (multimedia device) */
#ifndef __HAIKU__
#	define PCI_hd_audio			3
#endif

#define HDA_MAX_AUDIO_GROUPS	15
#define HDA_MAX_CODECS			15
#define HDA_MAX_STREAMS			16
#define MAX_CODEC_RESPONSES		16
#define MAX_CODEC_UNSOL_RESPONSES 16
#define MAX_INPUTS				32
#define MAX_IO_WIDGETS			8
#define MAX_ASSOCIATIONS		16
#define MAX_ASSOCIATION_PINS	16

#define STREAM_MAX_BUFFERS	10
#define STREAM_MIN_BUFFERS	2


enum {
	STREAM_PLAYBACK,
	STREAM_RECORD
};

struct hda_codec;
struct hda_stream;
struct hda_multi;

/*!	This structure describes a single HDA compliant
	controller. It contains a list of available streams
	for use by the codecs contained, and the messaging queue
	(verb/response) buffers for communication.
*/
#ifndef __REACTOS__
struct hda_controller {
	struct pci_info	pci_info;
	int32			opened;
	const char*		devfs_path;

	area_id			regs_area;
	vuint8*			regs;
	uint32			irq;
	bool			msi;
	bool			dma_snooping;

	uint16			codec_status;
	uint32			num_input_streams;
	uint32			num_output_streams;
	uint32			num_bidir_streams;

	uint32			corb_length;
	uint32			rirb_length;
	uint32			rirb_read_pos;
	uint32			corb_write_pos;
	area_id			corb_rirb_pos_area;
	corb_t*			corb;
	rirb_t*			rirb;
	uint32*			stream_positions;

	hda_codec*		codecs[HDA_MAX_CODECS + 1];
	hda_codec*		active_codec;
	uint32			num_codecs;

	hda_stream*		streams[HDA_MAX_STREAMS];
	sem_id			buffer_ready_sem;

	uint8 Read8(uint32 reg)
	{
		return *(regs + reg);
	}

	uint16 Read16(uint32 reg)
	{
		return *(vuint16*)(regs + reg);
	}

	uint32 Read32(uint32 reg)
	{
		return *(vuint32*)(regs + reg);
	}

	void Write8(uint32 reg, uint8 value)
	{
		*(regs + reg) = value;
	}

	void Write16(uint32 reg, uint16 value)
	{
		*(vuint16*)(regs + reg) = value;
	}

	void Write32(uint32 reg, uint32 value)
	{
		*(vuint32*)(regs + reg) = value;
	}

	void ReadModifyWrite8(uint32 reg, uint8 mask, uint8 value)
	{
		uint8 temp = Read8(reg);
		temp &= ~mask;
		temp |= value;
		Write8(reg, temp);
	}

	void ReadModifyWrite16(uint32 reg, uint16 mask, uint16 value)
	{
		uint16 temp = Read16(reg);
		temp &= ~mask;
		temp |= value;
		Write16(reg, temp);
	}

	void ReadModifyWrite32(uint32 reg, uint32 mask, uint32 value)
	{
		uint32 temp = Read32(reg);
		temp &= ~mask;
		temp |= value;
		Write32(reg, temp);
	}
};

/*!	This structure describes a single stream of audio data,
	which is can have multiple channels (for stereo or better).
*/
struct hda_stream {
	uint32		id;					/* HDA controller stream # */
	uint32		offset;				/* HDA I/O/B descriptor offset */
	bool		running;
	spinlock	lock;				/* Write lock */
	uint32		type;

	hda_controller* controller;

	uint32		pin_widget;			/* PIN Widget ID */
	uint32		io_widgets[MAX_IO_WIDGETS];	/* Input/Output Converter Widget ID */
	uint32		num_io_widgets;

	uint32		sample_rate;
	uint32		sample_format;

	uint32		num_buffers;
	uint32		num_channels;
	uint32		buffer_length;	/* size of buffer in samples */
	uint32		buffer_size;	/* actual (aligned) size of buffer in bytes */
	uint32		sample_size;
	uint8*		buffers[STREAM_MAX_BUFFERS];
					/* Virtual addresses for buffer */
	phys_addr_t	physical_buffers[STREAM_MAX_BUFFERS];
					/* Physical addresses for buffer */

	volatile bigtime_t	real_time;
	volatile uint64		frames_count;
	uint32				last_link_frame_position;
	volatile int32		buffer_cycle;

	uint32		rate, bps;			/* Samplerate & bits per sample */

	area_id		buffer_area;
	area_id		buffer_descriptors_area;
	phys_addr_t	physical_buffer_descriptors;	/* BDL physical address */

	int32		incorrect_position_count;
	bool		use_dma_position;

	uint8 Read8(uint32 reg)
	{
		return controller->Read8(HDAC_STREAM_BASE + offset + reg);
	}

	uint16 Read16(uint32 reg)
	{
		return controller->Read16(HDAC_STREAM_BASE + offset + reg);
	}

	uint32 Read32(uint32 reg)
	{
		return controller->Read32(HDAC_STREAM_BASE + offset + reg);
	}

	void Write8(uint32 reg, uint8 value)
	{
		*(controller->regs + HDAC_STREAM_BASE + offset + reg) = value;
	}

	void Write16(uint32 reg, uint16 value)
	{
		*(vuint16*)(controller->regs + HDAC_STREAM_BASE + offset + reg) = value;
	}

	void Write32(uint32 reg, uint32 value)
	{
		*(vuint32*)(controller->regs + HDAC_STREAM_BASE + offset + reg) = value;
	}
};

struct hda_widget {
	uint32			node_id;

	uint32			num_inputs;
	int32			active_input;
	uint32			inputs[MAX_INPUTS];
	uint32			flags;

	hda_widget_type	type;
	uint32			pm;

	struct {
		uint32		audio;
		uint32		output_amplifier;
		uint32		input_amplifier;
	} capabilities;

	union {
		struct {
			uint32	formats;
			uint32	rates;
		} io;
		struct {
		} mixer;
		struct {
			uint32	capabilities;
			uint32	config;
		} pin;
	} d;
};

struct hda_association {
	uint32	index;
	bool	enabled;
	uint32 	pin_count;
	uint32 	pins[MAX_ASSOCIATION_PINS];
};
#endif

#define WIDGET_FLAG_OUTPUT_PATH	0x01
#define WIDGET_FLAG_INPUT_PATH	0x02
#define WIDGET_FLAG_WIDGET_PATH	0x04

/*!	This structure describes a single Audio Function Group. An AFG
	is a group of audio widgets which can be used to configure multiple
	streams of audio either from the HDA Link to an output device (= playback)
	or from an input device to the HDA link (= recording).
*/
#ifndef __REACTOS__
struct hda_audio_group {
	hda_codec*		codec;
	hda_widget		widget;

	/* Multi Audio API data */
	hda_stream*		playback_stream;
	hda_stream*		record_stream;

	uint32			widget_start;
	uint32			widget_count;

	uint32			association_count;
	uint32			gpio;

	hda_widget*		widgets;
	hda_association		associations[MAX_ASSOCIATIONS];

	hda_multi*		multi;
};

/*!	This structure describes a single codec module in the
	HDA compliant device. This is a discrete component, which
	can contain both Audio Function Groups, Modem Function Groups,
	and other customized (vendor specific) Function Groups.

	NOTE: ATM, only Audio Function Groups are supported.
*/
struct hda_codec {
	uint16		vendor_id;
	uint16		product_id;
	uint8		major;
	uint8		minor;
	uint8		revision;
	uint8		stepping;
	uint8		addr;

	uint32		quirks;

	sem_id		response_sem;
	uint32		responses[MAX_CODEC_RESPONSES];
	uint32		response_count;

	sem_id		unsol_response_sem;
	thread_id	unsol_response_thread;
	uint32		unsol_responses[MAX_CODEC_UNSOL_RESPONSES];
	uint32		unsol_response_read, unsol_response_write;

	hda_audio_group* audio_groups[HDA_MAX_AUDIO_GROUPS];
	uint32		num_audio_groups;

	struct hda_controller* controller;
};


#define MULTI_CONTROL_FIRSTID	1024
#define MULTI_CONTROL_MASTERID	0
#define MULTI_MAX_CONTROLS 128
#define MULTI_MAX_CHANNELS 128

struct hda_multi_mixer_control {
	hda_multi	*multi;
	int32 	nid;
	int32 type;
	bool input;
	uint32 mute;
	uint32 gain;
	uint32 capabilities;
	int32 index;
	multi_mix_control	mix_control;
};


struct hda_multi {
	hda_audio_group *group;
	hda_multi_mixer_control controls[MULTI_MAX_CONTROLS];
	uint32 control_count;

	multi_channel_info chans[MULTI_MAX_CHANNELS];
	uint32 output_channel_count;
	uint32 input_channel_count;
	uint32 output_bus_channel_count;
	uint32 input_bus_channel_count;
	uint32 aux_bus_channel_count;
};


/* driver.c */
extern device_hooks gDriverHooks;
extern pci_module_info* gPci;
extern pci_x86_module_info* gPCIx86Module;
extern hda_controller gCards[MAX_CARDS];
extern uint32 gNumCards;

/* hda_codec.c */
const char* get_widget_location(uint32 location);
hda_widget* hda_audio_group_get_widget(hda_audio_group* audioGroup, uint32 nodeID);

status_t hda_audio_group_get_widgets(hda_audio_group* audioGroup,
	hda_stream* stream);
hda_codec* hda_codec_new(hda_controller* controller, uint32 cad);
void hda_codec_delete(hda_codec* codec);

/* hda_multi_audio.c */
status_t multi_audio_control(void* cookie, uint32 op, void* arg, size_t length);

/* hda_controller.c: Basic controller support */
status_t hda_hw_init(hda_controller* controller);
void hda_hw_stop(hda_controller* controller);
void hda_hw_uninit(hda_controller* controller);
status_t hda_send_verbs(hda_codec* codec, corb_t* verbs, uint32* responses,
	uint32 count);
status_t hda_verb_write(hda_codec* codec, uint32 nid, uint32 vid, uint16 payload);
status_t hda_verb_read(hda_codec* codec, uint32 nid, uint32 vid, uint32 *response);

/* hda_controller.c: Stream support */
hda_stream* hda_stream_new(hda_audio_group* audioGroup, int type);
void hda_stream_delete(hda_stream* stream);
status_t hda_stream_setup_buffers(hda_audio_group* audioGroup,
	hda_stream* stream, const char* desc);
status_t hda_stream_start(hda_controller* controller, hda_stream* stream);
status_t hda_stream_stop(hda_controller* controller, hda_stream* stream);
#endif

#endif	/* _HDA_H_ */
