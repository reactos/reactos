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
#include <winsock2.h>
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

/* holds the whole state of the RDP client */
struct rdpclient
{
	/* channels.c */
	CHANNEL_DEF channel_defs[CHANNEL_MAX_COUNT];
	unsigned int num_channels;

	/* licence.c */
	char * licence_username;
	char licence_hostname[MAX_COMPUTERNAME_LENGTH + 1];
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
	int disconnect_reason;
	unsigned int keylayout;
	int keyboard_type;
	int keyboard_subtype;
	int keyboard_functionkeys;

	int width;
	int height;
	int server_depth;
	BOOL bitmap_compression;
	BOOL bitmap_cache;
	BOOL bitmap_cache_persist_enable;
	BOOL bitmap_cache_precache;
	BOOL encryption;
	BOOL packet_encryption;
	BOOL desktop_save;	/* desktop save order */
	BOOL polygon_ellipse_orders;	/* polygon / ellipse orders */
	BOOL use_rdp5;
	BOOL console_session;
	uint32 rdp5_performanceflags;

	/* Session Directory redirection */
	BOOL redirect;
	wchar_t * redirect_server;
	wchar_t * redirect_domain;
	wchar_t * redirect_password;
	wchar_t * redirect_username;
	char * redirect_cookie;
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
		int current_status;

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
		SOCKET sock;
		struct stream in;
		struct stream out;
		long connection_timeout;
	}
	tcp;
};

#ifndef MAKE_PROTO
#include "proto.h"
#endif
