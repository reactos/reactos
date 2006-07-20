/*
   rdesktop: A Remote Desktop Protocol client.
   Master include file
   Copyright (C) Matthew Chapman 1999-2005

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <cchannel.h>

#if 0
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/time.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif
#include <limits.h>		/* PATH_MAX */

/* FIXME FIXME */
#include <windows.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
/* FIXME FIXME */
#endif

// TODO
#include <openssl/rc4.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/x509v3.h>

#define VERSION "1.4.1"

#ifdef WITH_DEBUG
#define DEBUG(args)	printf args;
#else
#define DEBUG(args)
#endif

#ifdef WITH_DEBUG_KBD
#define DEBUG_KBD(args) printf args;
#else
#define DEBUG_KBD(args)
#endif

#ifdef WITH_DEBUG_RDP5
#define DEBUG_RDP5(args) printf args;
#else
#define DEBUG_RDP5(args)
#endif

#ifdef WITH_DEBUG_CLIPBOARD
#define DEBUG_CLIPBOARD(args) printf args;
#else
#define DEBUG_CLIPBOARD(args)
#endif

#ifdef WITH_DEBUG_CHANNEL
#define DEBUG_CHANNEL(args) printf args;
#else
#define DEBUG_CHANNEL(args)
#endif

#define STRNCPY(dst,src,n)	{ strncpy(dst,src,n-1); dst[n-1] = 0; }

#ifndef MIN
#define MIN(x,y)		(((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y)		(((x) > (y)) ? (x) : (y))
#endif

/* timeval macros */
#ifndef timerisset
#define timerisset(tvp)\
         ((tvp)->tv_sec || (tvp)->tv_usec)
#endif
#ifndef timercmp
#define timercmp(tvp, uvp, cmp)\
        ((tvp)->tv_sec cmp (uvp)->tv_sec ||\
        (tvp)->tv_sec == (uvp)->tv_sec &&\
        (tvp)->tv_usec cmp (uvp)->tv_usec)
#endif
#ifndef timerclear
#define timerclear(tvp)\
        ((tvp)->tv_sec = (tvp)->tv_usec = 0)
#endif

/* If configure does not define the endianess, try
   to find it out */
#if !defined(L_ENDIAN) && !defined(B_ENDIAN)
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define L_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define B_ENDIAN
#else
#error Unknown endianness. Edit rdesktop.h.
#endif
#endif /* B_ENDIAN, L_ENDIAN from configure */

/* No need for alignment on x86 and amd64 */
#if !defined(NEED_ALIGN)
#if !(defined(__x86__) || defined(__x86_64__) || \
      defined(__AMD64__) || defined(_M_IX86) || \
      defined(__i386__))
#define NEED_ALIGN
#endif
#endif

struct rdpclient;
typedef struct rdpclient RDPCLIENT;

#include "parse.h"
#include "constants.h"
#include "types.h"
#include "orders.h"

#if 0
/* Used to store incoming io request, until they are ready to be completed */
/* using a linked list ensures that they are processed in the right order, */
/* if multiple ios are being done on the same fd */
struct async_iorequest
{
	uint32 fd, major, minor, offset, device, id, length, partial_len;
	long timeout,		/* Total timeout */
	  itv_timeout;		/* Interval timeout (between serial characters) */
	uint8 *buffer;
	DEVICE_FNS *fns;

	struct async_iorequest *next;	/* next element in list */
};
#endif

struct bmpcache_entry
{
	HBITMAP bitmap;
	sint16 previous;
	sint16 next;
};

#if 0
typedef struct _seamless_group
{
	Window wnd;
	unsigned long id;
	unsigned int refcnt;
} seamless_group;
typedef struct _seamless_window
{
	Window wnd;
	unsigned long id;
	unsigned long behind;
	seamless_group *group;
	int xoffset, yoffset;
	int width, height;
	int state;		/* normal/minimized/maximized. */
	unsigned int desktop;
	struct timeval *position_timer;

	BOOL outstanding_position;
	unsigned int outpos_serial;
	int outpos_xoffset, outpos_yoffset;
	int outpos_width, outpos_height;

	struct _seamless_window *next;
} seamless_window;
#endif

/* holds the whole state of the RDP client */
struct rdpclient
{
	/* channels.c */
#define MAX_CHANNELS			6
	VCHANNEL channels[MAX_CHANNELS];
	unsigned int num_channels;

	/* licence.c */
	BOOL licence_issued;

	/* mcs.c */
	uint16 mcs_userid;

	/* mppc.c */
	RDPCOMP mppc_dict;

	/* pstcache.c */
	int pstcache_fd[8];
	int pstcache_Bpp;
	BOOL pstcache_enumerated;

	/* rdesktop.c */
	char title[64];
	char username[64];
	char hostname[16];
	char keymapname[MAX_PATH];
	unsigned int keylayout;
	int keyboard_type;
	int keyboard_subtype;
	int keyboard_functionkeys;

	int width;
	int height;
	int xpos;
	int ypos;
	int pos;			/* 0 position unspecified,
					   1 specified,
					   2 xpos neg,
					   4 ypos neg  */

	int server_depth;
	int win_button_size; /* If zero, disable single app mode */
	BOOL bitmap_compression;
	BOOL sendmotion;
	BOOL bitmap_cache;
	BOOL bitmap_cache_persist_enable;
	BOOL bitmap_cache_precache;
	BOOL encryption;
	BOOL packet_encryption;
	BOOL desktop_save;	/* desktop save order */
	BOOL polygon_ellipse_orders;	/* polygon / ellipse orders */
	BOOL fullscreen;
	BOOL grab_keyboard;
	BOOL hide_decorations;
	BOOL use_rdp5;
	BOOL rdpclip;
	BOOL console_session;
	BOOL numlock_sync;
	BOOL lspci_enabled ;
	BOOL owncolmap;
	BOOL ownbackstore;	/* We can't rely on external BackingStore */
	BOOL seamless_rdp;
	uint32 embed_wnd;
	uint32 rdp5_performanceflags;

	/* Session Directory redirection */
	BOOL redirect;
	char redirect_server[64];
	char redirect_domain[16];
	char redirect_password[64];
	char redirect_username[64];
	char redirect_cookie[128];
	uint32 redirect_flags;

	/* rdp.c */
	uint8 *next_packet;
	uint32 rdp_shareid;

	/* secure.c */
	uint16 server_rdp_version;

	/* tcp.c */
	int tcp_port_rdp;

	/* cache.c */
	struct cache_
	{
		struct bmpcache_entry bmpcache[3][0xa00];
		HBITMAP volatile_bc[3];

		int bmpcache_lru[3];
		int bmpcache_mru[3];
		int bmpcache_count[3];

		FONTGLYPH fontcache[12][256];
		DATABLOB textcache[256];
		uint8 deskcache[0x38400 * 4];
		HCURSOR cursorcache[0x20];
	}
	cache;

	/* licence.c */
	struct licence_
	{
		uint8 key[16];
		uint8 sign_key[16];
	}
	licence;

	/* orders.c */
	struct orders_
	{
		RDP_ORDER_STATE order_state;
	}
	orders;

	/* rdp.c */
	struct rdp_
	{
		int ignore_;
#if WITH_DEBUG
		uint32 packetno;
#endif

#ifdef HAVE_ICONV
		BOOL iconv_works;
#endif
	}
	rdp;

	/* secure.c */
	struct secure_
	{
		int rc4_key_len;
		RC4_KEY rc4_decrypt_key;
		RC4_KEY rc4_encrypt_key;
		RSA *server_public_key;
		uint32 server_public_key_len;

		uint8 sign_key[16];
		uint8 decrypt_key[16];
		uint8 encrypt_key[16];
		uint8 decrypt_update_key[16];
		uint8 encrypt_update_key[16];
		uint8 crypted_random[SEC_MAX_MODULUS_SIZE];

		/* These values must be available to reset state - Session Directory */
		int encrypt_use_count;
		int decrypt_use_count;
	}
	secure;

	/* tcp.c */
	struct tcp_
	{
		int sock;
		struct stream in;
		struct stream out;
	}
	tcp;

#if 0
	/* Public fields */
	/* disk.c */
#define	MAX_OPEN_FILES	0x100
	FILEINFO fileinfo[MAX_OPEN_FILES];
	BOOL notify_stamp;

	/* ewmhints.c */
	Atom net_wm_state_atom, net_wm_desktop_atom;

	/* rdpdr.c */
	/* If select() times out, the request for the device with handle min_timeout_fd is aborted */
	NTHANDLE min_timeout_fd;
	uint32 num_devices;

	/* Table with information about rdpdr devices */
	RDPDR_DEVICE rdpdr_device[RDPDR_MAX_DEVICES];
	char *rdpdr_clientname;

	struct async_iorequest *iorequest;

	/* rdpsndXXX.c */
#ifdef WITH_RDPSND
	int dsp_fd;
	BOOL dsp_busy;
#endif

#if 0
	/* xwin.c */
	Display *display;
	BOOL enable_compose;
	BOOL Unobscured;		/* used for screenblt */
	Time last_gesturetime;
	Window wnd;
#endif

	/* Private fields */
	/* FIXME: it's not pretty to spill private fields this way. Use opaque pointers */
	/* cliprdr.c */
	struct cliprdr_
	{
		VCHANNEL *channel;
		uint8 *last_formats;
		uint32 last_formats_length;
	}
	cliprdr;

	struct ewmhints_
	{
		Atom state_maximized_vert_atom, state_maximized_horz_atom,
			state_hidden_atom, name_atom, utf8_string_atom,
			state_skip_taskbar_atom, state_skip_pager_atom, state_modal_atom;
	}
	ewmhints;

	/* rdpdr.c */
	struct rdpdr_
	{
		VCHANNEL *channel;
	}
	rdpdr;

	/* rdpsnd.c */
	struct rdpsnd_
	{
		VCHANNEL *channel;

		BOOL device_open;

#define MAX_FORMATS		10
		WAVEFORMATEX formats[MAX_FORMATS];

		unsigned int format_count;
		unsigned int current_format;
	}
	rdpsnd;

	/* seamless.c */
	struct seamless_
	{
		VCHANNEL *channel;
		unsigned int serial;
	}
	seamless;

	/* xclip.c */
	struct xclip_
	{
#define MAX_TARGETS 8

		/* Mode of operation.
		   - Auto: Look at both PRIMARY and CLIPBOARD and use the most recent.
		   - Non-auto: Look at just CLIPBOARD. */
		BOOL auto_mode;
		/* Atoms of the two X selections we're dealing with: CLIPBOARD (explicit-copy) and PRIMARY (selection-copy) */
		Atom clipboard_atom, primary_atom;
		/* Atom of the TARGETS clipboard target */
		Atom targets_atom;
		/* Atom of the TIMESTAMP clipboard target */
		Atom timestamp_atom;
		/* Atom _RDESKTOP_CLIPBOARD_TARGET which is used as the 'property' argument in
		   XConvertSelection calls: This is the property of our window into which
		   XConvertSelection will store the received clipboard data. */
		Atom rdesktop_clipboard_target_atom;
		/* Atoms _RDESKTOP_PRIMARY_TIMESTAMP_TARGET and _RDESKTOP_CLIPBOARD_TIMESTAMP_TARGET
		   are used to store the timestamps for when a window got ownership of the selections.
		   We use these to determine which is more recent and should be used. */
		Atom rdesktop_primary_timestamp_target_atom, rdesktop_clipboard_timestamp_target_atom;
		/* Storage for timestamps since we get them in two separate notifications. */
		Time primary_timestamp, clipboard_timestamp;
		/* Clipboard target for getting a list of native Windows clipboard formats. The
		   presence of this target indicates that the selection owner is another rdesktop. */
		Atom rdesktop_clipboard_formats_atom;
		/* The clipboard target (X jargon for "clipboard format") for rdesktop-to-rdesktop
		   interchange of Windows native clipboard data. The requestor must supply the
		   desired native Windows clipboard format in the associated property. */
		Atom rdesktop_native_atom;
		/* Local copy of the list of native Windows clipboard formats. */
		uint8 *formats_data;
		uint32 formats_data_length;
		/* We need to know when another rdesktop process gets or loses ownership of a
		   selection. Without XFixes we do this by touching a property on the root window
		   which will generate PropertyNotify notifications. */
		Atom rdesktop_selection_notify_atom;
		/* State variables that indicate if we're currently probing the targets of the
		   selection owner. reprobe_selections indicate that the ownership changed in
		   the middle of the current probe so it should be restarted. */
		BOOL probing_selections, reprobe_selections;
		/* Atoms _RDESKTOP_PRIMARY_OWNER and _RDESKTOP_CLIPBOARD_OWNER. Used as properties
		   on the root window to indicate which selections that are owned by rdesktop. */
		Atom rdesktop_primary_owner_atom, rdesktop_clipboard_owner_atom;
		Atom format_string_atom, format_utf8_string_atom, format_unicode_atom;
		/* Atom of the INCR clipboard type (see ICCCM on "INCR Properties") */
		Atom incr_atom;
		/* Stores the last "selection request" (= another X client requesting clipboard data from us).
		   To satisfy such a request, we request the clipboard data from the RDP server.
		   When we receive the response from the RDP server (asynchronously), this variable gives us
		   the context to proceed. */
		XSelectionRequestEvent selection_request;
		/* Denotes we have a pending selection request. */
		Bool has_selection_request;
		/* Stores the clipboard format (CF_TEXT, CF_UNICODETEXT etc.) requested in the last
		   CLIPDR_DATA_REQUEST (= the RDP server requesting clipboard data from us).
		   When we receive this data from whatever X client offering it, this variable gives us
		   the context to proceed.
		 */
		int rdp_clipboard_request_format;
		/* Array of offered clipboard targets that will be sent to fellow X clients upon a TARGETS request. */
		Atom targets[MAX_TARGETS];
		int num_targets;
		/* Denotes that an rdesktop (not this rdesktop) is owning the selection,
		   allowing us to interchange Windows native clipboard data directly. */
		BOOL rdesktop_is_selection_owner;
		/* Time when we acquired the selection. */
		Time acquire_time;

		/* Denotes that an INCR ("chunked") transfer is in progress. */
		int waiting_for_INCR;
		/* Denotes the target format of the ongoing INCR ("chunked") transfer. */
		Atom incr_target;
		/* Buffers an INCR transfer. */
		uint8 *clip_buffer;
		/* Denotes the size of clip_buffer. */
		uint32 clip_buflen;
	}
	xclip;

	/* xkeymap.c */
	struct xkeymap_
	{
#define KEYMAP_SIZE 0xffff+1
		BOOL keymap_loaded;
		key_translation *keymap[KEYMAP_SIZE];
		int min_keycode;
		uint16 remote_modifier_state;
		uint16 saved_remote_modifier_state;
	}
	xkeymap;

#if 0
	/* xwin.c */
	struct xwin_
	{
		int x_socket;
		Screen *screen;

		/* SeamlessRDP support */
		seamless_window *seamless_windows;
		unsigned long seamless_focused;
		BOOL seamless_started;	/* Server end is up and running */
		BOOL seamless_active;	/* We are currently in seamless mode */
		BOOL seamless_hidden;	/* Desktop is hidden on server */

		GC gc;
		GC create_bitmap_gc;
		GC create_glyph_gc;
		XRectangle clip_rectangle;
		Visual *visual;
		/* Color depth of the X11 visual of our window (e.g. 24 for True Color R8G8B visual).
		   This may be 32 for R8G8B8 visuals, and then the rest of the bits are undefined
		   as far as we're concerned. */
		int depth;
		/* Bits-per-Pixel of the pixmaps we'll be using to draw on our window.
		   This may be larger than depth, in which case some of the bits would
		   be kept solely for alignment (e.g. 32bpp pixmaps on a 24bpp visual). */
		int bpp;
		XIM IM;
		XIC IC;
		XModifierKeymap *mod_map;
		Cursor current_cursor;
		HCURSOR null_cursor;
		Atom protocol_atom, kill_atom;
		BOOL focused;
		BOOL mouse_in_wnd;
		/* Indicates that:
		   1) visual has 15, 16 or 24 depth and the same color channel masks
			  as its RDP equivalent (implies X server is LE),
		   2) host is LE
		   This will trigger an optimization whose real value is questionable.
		*/
		BOOL compatible_arch;
		/* Indicates whether RDP's bitmaps and our XImages have the same
		   binary format. If so, we can avoid an expensive translation.
		   Note that this can be true when compatible_arch is false,
		   e.g.:

			 RDP(LE) <-> host(BE) <-> X-Server(LE)

		   ('host' is the machine running rdesktop; the host simply memcpy's
			so its endianess doesn't matter)
		 */
		BOOL no_translate_image;

		/* endianness */
		BOOL host_be;
		BOOL xserver_be;
		int red_shift_r, blue_shift_r, green_shift_r;
		int red_shift_l, blue_shift_l, green_shift_l;

		/* software backing store */
		Pixmap backstore;

		/* Moving in single app mode */
		BOOL moving_wnd;
		int move_x_offset;
		int move_y_offset;
		BOOL using_full_workarea;

		/* colour maps */
		Colormap xcolmap;
		uint32 *colmap;

		XErrorHandler old_error_handler;
	}
	xwin;
#endif
#endif
};

#ifndef MAKE_PROTO
#include "proto.h"
#endif
