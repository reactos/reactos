/*
   rdesktop: A Remote Desktop Protocol client.
   Common data types
   Copyright (C) Matthew Chapman 1999-2008
   Copyright 2014 Henrik Andersson <hean01@cendio.se> for Cendio AB

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

typedef int RD_BOOL;

#ifndef True
#define True  (1)
#define False (0)
#endif

typedef unsigned char uint8;
typedef signed char sint8;
typedef unsigned short uint16;
typedef signed short sint16;
typedef unsigned int uint32;
typedef signed int sint32;

typedef void *RD_HBITMAP;
typedef void *RD_HGLYPH;
typedef void *RD_HCOLOURMAP;
typedef void *RD_HCURSOR;


typedef enum _RDP_VERSION
{
	RDP_V4 = 4,
	RDP_V5 = 5,
	RDP_V6 = 6
} RDP_VERSION;


typedef struct _RD_POINT
{
	sint16 x, y;
}
RD_POINT;

typedef struct _COLOURENTRY
{
	uint8 red;
	uint8 green;
	uint8 blue;

}
COLOURENTRY;

typedef struct _COLOURMAP
{
	uint16 ncolours;
	COLOURENTRY *colours;

}
COLOURMAP;

typedef struct _BOUNDS
{
	sint16 left;
	sint16 top;
	sint16 right;
	sint16 bottom;

}
BOUNDS;

typedef struct _PEN
{
	uint8 style;
	uint8 width;
	uint32 colour;

}
PEN;

/* this is whats in the brush cache */
typedef struct _BRUSHDATA
{
	uint32 colour_code;
	uint32 data_size;
	uint8 *data;
}
BRUSHDATA;

typedef struct _BRUSH
{
	uint8 xorigin;
	uint8 yorigin;
	uint8 style;
	uint8 pattern[8];
	BRUSHDATA *bd;
}
BRUSH;

typedef struct _FONTGLYPH
{
	sint16 offset;
	sint16 baseline;
	uint16 width;
	uint16 height;
	RD_HBITMAP pixmap;

}
FONTGLYPH;

typedef struct _DATABLOB
{
	void *data;
	int size;

}
DATABLOB;

typedef struct _key_translation
{
	/* For normal scancode translations */
	uint8 scancode;
	uint16 modifiers;
	/* For sequences. If keysym is nonzero, the fields above are not used. */
	uint32 seq_keysym;	/* Really KeySym */
	struct _key_translation *next;
}
key_translation;

typedef struct _key_translation_entry
{
	key_translation *tr;
	/* The full KeySym for this entry, not KEYMAP_MASKed */
	uint32 keysym;
	/* This will be non-NULL if there has been a hash collision */
	struct _key_translation_entry *next;
}
key_translation_entry;

typedef struct _VCHANNEL
{
	uint16 mcs_id;
	char name[8];
	uint32 flags;
	struct stream in;
	void (*process) (STREAM);
}
VCHANNEL;

/* PSTCACHE */
typedef uint8 HASH_KEY[8];

/* Header for an entry in the persistent bitmap cache file */
typedef struct _PSTCACHE_CELLHEADER
{
	HASH_KEY key;
	uint8 width, height;
	uint16 length;
	uint32 stamp;
}
CELLHEADER;

#define MAX_CBSIZE 256

/* RDPSND */
typedef struct _RD_WAVEFORMATEX
{
	uint16 wFormatTag;
	uint16 nChannels;
	uint32 nSamplesPerSec;
	uint32 nAvgBytesPerSec;
	uint16 nBlockAlign;
	uint16 wBitsPerSample;
	uint16 cbSize;
	uint8 cb[MAX_CBSIZE];
} RD_WAVEFORMATEX;

typedef struct _RDPCOMP
{
	uint32 roff;
	uint8 hist[RDP_MPPC_DICT_SIZE];
	struct stream ns;
}
RDPCOMP;

/* RDPDR */
typedef uint32 RD_NTSTATUS;
typedef uint32 RD_NTHANDLE;

typedef struct _DEVICE_FNS
{
	RD_NTSTATUS(*create) (uint32 device, uint32 desired_access, uint32 share_mode,
			      uint32 create_disposition, uint32 flags_and_attributes,
			      char *filename, RD_NTHANDLE * handle);
	RD_NTSTATUS(*close) (RD_NTHANDLE handle);
	RD_NTSTATUS(*read) (RD_NTHANDLE handle, uint8 * data, uint32 length, uint32 offset,
			    uint32 * result);
	RD_NTSTATUS(*write) (RD_NTHANDLE handle, uint8 * data, uint32 length, uint32 offset,
			     uint32 * result);
	RD_NTSTATUS(*device_control) (RD_NTHANDLE handle, uint32 request, STREAM in, STREAM out);
}
DEVICE_FNS;


typedef struct rdpdr_device_info
{
	uint32 device_type;
	RD_NTHANDLE handle;
	char name[8];
	char *local_path;
	void *pdevice_data;
}
RDPDR_DEVICE;

typedef struct rdpdr_serial_device_info
{
	int dtr;
	int rts;
	uint32 control, xonoff, onlimit, offlimit;
	uint32 baud_rate,
		queue_in_size,
		queue_out_size,
		wait_mask,
		read_interval_timeout,
		read_total_timeout_multiplier,
		read_total_timeout_constant,
		write_total_timeout_multiplier, write_total_timeout_constant, posix_wait_mask;
	uint8 stop_bits, parity, word_length;
	uint8 chars[6];
	struct termios *ptermios, *pold_termios;
	int event_txempty, event_cts, event_dsr, event_rlsd, event_pending;
}
SERIAL_DEVICE;

typedef struct rdpdr_parallel_device_info
{
	char *driver, *printer;
	uint32 queue_in_size,
		queue_out_size,
		wait_mask,
		read_interval_timeout,
		read_total_timeout_multiplier,
		read_total_timeout_constant,
		write_total_timeout_multiplier,
		write_total_timeout_constant, posix_wait_mask, bloblen;
	uint8 *blob;
}
PARALLEL_DEVICE;

typedef struct rdpdr_printer_info
{
	FILE *printer_fp;
	char *driver, *printer;
	uint32 bloblen;
	uint8 *blob;
	RD_BOOL default_printer;
}
PRINTER;

typedef struct notify_data
{
	time_t modify_time;
	time_t status_time;
	time_t total_time;
	unsigned int num_entries;
}
NOTIFY;

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

typedef struct fileinfo
{
	uint32 device_id, flags_and_attributes, accessmask;
	char path[PATH_MAX];
	DIR *pdir;
	struct dirent *pdirent;
	char pattern[PATH_MAX];
	RD_BOOL delete_on_close;
	NOTIFY notify;
	uint32 info_class;
}
FILEINFO;

typedef RD_BOOL(*str_handle_lines_t) (const char *line, void *data);
