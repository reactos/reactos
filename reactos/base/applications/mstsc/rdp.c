/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - RDP layer
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

#include "todo.h"


extern uint16 g_mcs_userid;
extern char g_username[];
extern char g_codepage[];
extern BOOL g_bitmap_compression;
extern BOOL g_orders;
extern BOOL g_encryption;
extern BOOL g_desktop_save;
extern BOOL g_polygon_ellipse_orders;
extern BOOL g_use_rdp5;
extern uint16 g_server_rdp_version;
extern uint32 g_rdp5_performanceflags;
extern int g_server_depth;
extern int g_width;
extern int g_height;
extern BOOL g_bitmap_cache;
extern BOOL g_bitmap_cache_persist_enable;

uint8 *g_next_packet;
uint32 g_rdp_shareid;

extern RDPCOMP g_mppc_dict;

/* Session Directory support */
extern BOOL g_redirect;
extern char g_redirect_server[64];
extern char g_redirect_domain[16];
extern char g_redirect_password[64];
extern char g_redirect_username[64];
extern char g_redirect_cookie[128];
extern uint32 g_redirect_flags;
/* END Session Directory support */

#ifdef WITH_DEBUG
static uint32 g_packetno;
#endif

#ifdef HAVE_ICONV
static BOOL g_iconv_works = True;
#endif

/* Receive an RDP packet */
static STREAM
rdp_recv(uint8 * type)
{
	static STREAM rdp_s;
	uint16 length, pdu_type;
	uint8 rdpver;

	if ((rdp_s == NULL) || (g_next_packet >= rdp_s->end) || (g_next_packet == NULL))
	{
		rdp_s = sec_recv(&rdpver);
		if (rdp_s == NULL)
			return NULL;
		if (rdpver == 0xff)
		{
			g_next_packet = rdp_s->end;
			*type = 0;
			return rdp_s;
		}
		else if (rdpver != 3)
		{
			/* rdp5_process should move g_next_packet ok */
			rdp5_process(rdp_s);
			*type = 0;
			return rdp_s;
		}

		g_next_packet = rdp_s->p;
	}
	else
	{
		rdp_s->p = g_next_packet;
	}

	in_uint16_le(rdp_s, length);
	/* 32k packets are really 8, keepalive fix */
	if (length == 0x8000)
	{
		g_next_packet += 8;
		*type = 0;
		return rdp_s;
	}
	in_uint16_le(rdp_s, pdu_type);
	in_uint8s(rdp_s, 2);	/* userid */
	*type = pdu_type & 0xf;

#ifdef WITH_DEBUG
	DEBUG(("RDP packet #%d, (type %x)\n", ++g_packetno, *type));
	hexdump(g_next_packet, length);
#endif /*  */

	g_next_packet += length;
	return rdp_s;
}

/* Initialise an RDP data packet */
static STREAM
rdp_init_data(int maxlen)
{
	STREAM s;

	s = sec_init(g_encryption ? SEC_ENCRYPT : 0, maxlen + 18);
	s_push_layer(s, rdp_hdr, 18);

	return s;
}

/* Send an RDP data packet */
static void
rdp_send_data(STREAM s, uint8 data_pdu_type)
{
	uint16 length;

	s_pop_layer(s, rdp_hdr);
	length = s->end - s->p;

	out_uint16_le(s, length);
	out_uint16_le(s, (RDP_PDU_DATA | 0x10));
	out_uint16_le(s, (g_mcs_userid + 1001));

	out_uint32_le(s, g_rdp_shareid);
	out_uint8(s, 0);	/* pad */
	out_uint8(s, 1);	/* streamid */
	out_uint16_le(s, (length - 14));
	out_uint8(s, data_pdu_type);
	out_uint8(s, 0);	/* compress_type */
	out_uint16(s, 0);	/* compress_len */

	sec_send(s, g_encryption ? SEC_ENCRYPT : 0);
}

/* Output a string in Unicode */
void
rdp_out_unistr(STREAM s, char *string, int len)
{
#ifdef HAVE_ICONV
	size_t ibl = strlen(string), obl = len + 2;
	static iconv_t iconv_h = (iconv_t) - 1;
	char *pin = string, *pout = (char *) s->p;

	memset(pout, 0, len + 4);

	if (g_iconv_works)
	{
		if (iconv_h == (iconv_t) - 1)
		{
			size_t i = 1, o = 4;
			if ((iconv_h = iconv_open(WINDOWS_CODEPAGE, g_codepage)) == (iconv_t) - 1)
			{
				warning("rdp_out_unistr: iconv_open[%s -> %s] fail %d\n",
					g_codepage, WINDOWS_CODEPAGE, (int) iconv_h);

				g_iconv_works = False;
				rdp_out_unistr(s, string, len);
				return;
			}
			if (iconv(iconv_h, (ICONV_CONST char **) &pin, &i, &pout, &o) ==
			    (size_t) - 1)
			{
				iconv_close(iconv_h);
				iconv_h = (iconv_t) - 1;
				warning("rdp_out_unistr: iconv(1) fail, errno %d\n", errno);

				g_iconv_works = False;
				rdp_out_unistr(s, string, len);
				return;
			}
			pin = string;
			pout = (char *) s->p;
		}

		if (iconv(iconv_h, (ICONV_CONST char **) &pin, &ibl, &pout, &obl) == (size_t) - 1)
		{
			iconv_close(iconv_h);
			iconv_h = (iconv_t) - 1;
			warning("rdp_out_unistr: iconv(2) fail, errno %d\n", errno);

			g_iconv_works = False;
			rdp_out_unistr(s, string, len);
			return;
		}

		s->p += len + 2;

	}
	else
#endif
	{
		int i = 0, j = 0;

		len += 2;

		while (i < len)
		{
			s->p[i++] = string[j++];
			s->p[i++] = 0;
		}

		s->p += len;
	}
}

/* Input a string in Unicode
 *
 * Returns str_len of string
 */
int
rdp_in_unistr(STREAM s, char *string, int uni_len)
{
#ifdef HAVE_ICONV
	size_t ibl = uni_len, obl = uni_len;
	char *pin = (char *) s->p, *pout = string;
	static iconv_t iconv_h = (iconv_t) - 1;

	if (g_iconv_works)
	{
		if (iconv_h == (iconv_t) - 1)
		{
			if ((iconv_h = iconv_open(g_codepage, WINDOWS_CODEPAGE)) == (iconv_t) - 1)
			{
				warning("rdp_in_unistr: iconv_open[%s -> %s] fail %d\n",
					WINDOWS_CODEPAGE, g_codepage, (int) iconv_h);

				g_iconv_works = False;
				return rdp_in_unistr(s, string, uni_len);
			}
		}

		if (iconv(iconv_h, (ICONV_CONST char **) &pin, &ibl, &pout, &obl) == (size_t) - 1)
		{
			iconv_close(iconv_h);
			iconv_h = (iconv_t) - 1;
			warning("rdp_in_unistr: iconv fail, errno %d\n", errno);

			g_iconv_works = False;
			return rdp_in_unistr(s, string, uni_len);
		}

		/* we must update the location of the current STREAM for future reads of s->p */
		s->p += uni_len;

		return pout - string;
	}
	else
#endif
	{
		int i = 0;

		while (i < uni_len / 2)
		{
			in_uint8a(s, &string[i++], 1);
			in_uint8s(s, 1);
		}

		return i - 1;
	}
}


/* Parse a logon info packet */
static void
rdp_send_logon_info(uint32 flags, char *domain, char *user,
		    char *password, char *program, char *directory)
{
	char *ipaddr = tcp_get_address();
	int len_domain = 2 * strlen(domain);
	int len_user = 2 * strlen(user);
	int len_password = 2 * strlen(password);
	int len_program = 2 * strlen(program);
	int len_directory = 2 * strlen(directory);
	int len_ip = 2 * strlen(ipaddr);
	int len_dll = 2 * strlen("C:\\WINNT\\System32\\mstscax.dll");
	int packetlen = 0;
	uint32 sec_flags = g_encryption ? (SEC_LOGON_INFO | SEC_ENCRYPT) : SEC_LOGON_INFO;
	STREAM s;
	//time_t t = time(NULL);
	//time_t tzone;

	if (!g_use_rdp5 || 1 == g_server_rdp_version)
	{
		DEBUG_RDP5(("Sending RDP4-style Logon packet\n"));

		s = sec_init(sec_flags, 18 + len_domain + len_user + len_password
			     + len_program + len_directory + 10);

		out_uint32(s, 0);
		out_uint32_le(s, flags);
		out_uint16_le(s, len_domain);
		out_uint16_le(s, len_user);
		out_uint16_le(s, len_password);
		out_uint16_le(s, len_program);
		out_uint16_le(s, len_directory);
		rdp_out_unistr(s, domain, len_domain);
		rdp_out_unistr(s, user, len_user);
		rdp_out_unistr(s, password, len_password);
		rdp_out_unistr(s, program, len_program);
		rdp_out_unistr(s, directory, len_directory);
	}
	else
	{
#if 0
		flags |= RDP_LOGON_BLOB;
		DEBUG_RDP5(("Sending RDP5-style Logon packet\n"));
		packetlen = 4 +	/* Unknown uint32 */
			4 +	/* flags */
			2 +	/* len_domain */
			2 +	/* len_user */
			(flags & RDP_LOGON_AUTO ? 2 : 0) +	/* len_password */
			(flags & RDP_LOGON_BLOB ? 2 : 0) +	/* Length of BLOB */
			2 +	/* len_program */
			2 +	/* len_directory */
			(0 < len_domain ? len_domain : 2) +	/* domain */
			len_user + (flags & RDP_LOGON_AUTO ? len_password : 0) + 0 +	/* We have no 512 byte BLOB. Perhaps we must? */
			(flags & RDP_LOGON_BLOB && !(flags & RDP_LOGON_AUTO) ? 2 : 0) +	/* After the BLOB is a unknown int16. If there is a BLOB, that is. */
			(0 < len_program ? len_program : 2) + (0 < len_directory ? len_directory : 2) + 2 +	/* Unknown (2) */
			2 +	/* Client ip length */
			len_ip +	/* Client ip */
			2 +	/* DLL string length */
			len_dll +	/* DLL string */
			2 +	/* Unknown */
			2 +	/* Unknown */
			64 +	/* Time zone #0 */
			2 +	/* Unknown */
			64 +	/* Time zone #1 */
			32;	/* Unknown */

		s = sec_init(sec_flags, packetlen);
		DEBUG_RDP5(("Called sec_init with packetlen %d\n", packetlen));

		out_uint32(s, 0);	/* Unknown */
		out_uint32_le(s, flags);
		out_uint16_le(s, len_domain);
		out_uint16_le(s, len_user);
		if (flags & RDP_LOGON_AUTO)
		{
			out_uint16_le(s, len_password);

		}
		if (flags & RDP_LOGON_BLOB && !(flags & RDP_LOGON_AUTO))
		{
			out_uint16_le(s, 0);
		}
		out_uint16_le(s, len_program);
		out_uint16_le(s, len_directory);
		if (0 < len_domain)
			rdp_out_unistr(s, domain, len_domain);
		else
			out_uint16_le(s, 0);
		rdp_out_unistr(s, user, len_user);
		if (flags & RDP_LOGON_AUTO)
		{
			rdp_out_unistr(s, password, len_password);
		}
		if (flags & RDP_LOGON_BLOB && !(flags & RDP_LOGON_AUTO))
		{
			out_uint16_le(s, 0);
		}
		if (0 < len_program)
		{
			rdp_out_unistr(s, program, len_program);

		}
		else
		{
			out_uint16_le(s, 0);
		}
		if (0 < len_directory)
		{
			rdp_out_unistr(s, directory, len_directory);
		}
		else
		{
			out_uint16_le(s, 0);
		}
		out_uint16_le(s, 2);
		out_uint16_le(s, len_ip + 2);	/* Length of client ip */
		rdp_out_unistr(s, ipaddr, len_ip);
		out_uint16_le(s, len_dll + 2);
		rdp_out_unistr(s, "C:\\WINNT\\System32\\mstscax.dll", len_dll);

		tzone = (mktime(gmtime(&t)) - mktime(localtime(&t))) / 60;
		out_uint32_le(s, tzone);

		rdp_out_unistr(s, "GTB, normaltid", 2 * strlen("GTB, normaltid"));
		out_uint8s(s, 62 - 2 * strlen("GTB, normaltid"));

		out_uint32_le(s, 0x0a0000);
		out_uint32_le(s, 0x050000);
		out_uint32_le(s, 3);
		out_uint32_le(s, 0);
		out_uint32_le(s, 0);

		rdp_out_unistr(s, "GTB, sommartid", 2 * strlen("GTB, sommartid"));
		out_uint8s(s, 62 - 2 * strlen("GTB, sommartid"));

		out_uint32_le(s, 0x30000);
		out_uint32_le(s, 0x050000);
		out_uint32_le(s, 2);
		out_uint32(s, 0);
		out_uint32_le(s, 0xffffffc4);
		out_uint32_le(s, 0xfffffffe);
		out_uint32_le(s, g_rdp5_performanceflags);
		out_uint32(s, 0);

#endif
	}
	s_mark_end(s);
	sec_send(s, sec_flags);
}

/* Send a control PDU */
static void
rdp_send_control(uint16 action)
{
	STREAM s;

	s = rdp_init_data(8);

	out_uint16_le(s, action);
	out_uint16(s, 0);	/* userid */
	out_uint32(s, 0);	/* control id */

	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_CONTROL);
}

/* Send a synchronisation PDU */
static void
rdp_send_synchronise(void)
{
	STREAM s;

	s = rdp_init_data(4);

	out_uint16_le(s, 1);	/* type */
	out_uint16_le(s, 1002);

	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_SYNCHRONISE);
}

/* Send a single input event */
void
rdp_send_input(uint32 time, uint16 message_type, uint16 device_flags, uint16 param1, uint16 param2)
{
	STREAM s;

	s = rdp_init_data(16);

	out_uint16_le(s, 1);	/* number of events */
	out_uint16(s, 0);	/* pad */

	out_uint32_le(s, time);
	out_uint16_le(s, message_type);
	out_uint16_le(s, device_flags);
	out_uint16_le(s, param1);
	out_uint16_le(s, param2);

	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_INPUT);
}

/* Send a client window information PDU */
void
rdp_send_client_window_status(int status)
{
	STREAM s;
	static int current_status = 1;

	if (current_status == status)
		return;

	s = rdp_init_data(12);

	out_uint32_le(s, status);

	switch (status)
	{
		case 0:	/* shut the server up */
			break;

		case 1:	/* receive data again */
			out_uint32_le(s, 0);	/* unknown */
			out_uint16_le(s, g_width);
			out_uint16_le(s, g_height);
			break;
	}

	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_CLIENT_WINDOW_STATUS);
	current_status = status;
}

/* Send persistent bitmap cache enumeration PDU's */
static void
rdp_enum_bmpcache2(void)
{
	STREAM s;
	HASH_KEY keylist[BMPCACHE2_NUM_PSTCELLS];
	uint32 num_keys, offset, count, flags;

	offset = 0;
	num_keys = pstcache_enumerate(2, keylist);

	while (offset < num_keys)
	{
		count = MIN(num_keys - offset, 169);

		s = rdp_init_data(24 + count * sizeof(HASH_KEY));

		flags = 0;
		if (offset == 0)
			flags |= PDU_FLAG_FIRST;
		if (num_keys - offset <= 169)
			flags |= PDU_FLAG_LAST;

		/* header */
		out_uint32_le(s, 0);
		out_uint16_le(s, count);
		out_uint16_le(s, 0);
		out_uint16_le(s, 0);
		out_uint16_le(s, 0);
		out_uint16_le(s, 0);
		out_uint16_le(s, num_keys);
		out_uint32_le(s, 0);
		out_uint32_le(s, flags);

		/* list */
		out_uint8a(s, keylist[offset], count * sizeof(HASH_KEY));

		s_mark_end(s);
		rdp_send_data(s, 0x2b);

		offset += 169;
	}
}

/* Send an (empty) font information PDU */
static void
rdp_send_fonts(uint16 seq)
{
	STREAM s;

	s = rdp_init_data(8);

	out_uint16(s, 0);	/* number of fonts */
	out_uint16_le(s, 0);	/* pad? */
	out_uint16_le(s, seq);	/* unknown */
	out_uint16_le(s, 0x32);	/* entry size */

	s_mark_end(s);
	rdp_send_data(s, RDP_DATA_PDU_FONT2);
}

/* Output general capability set */
static void
rdp_out_general_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_GENERAL);
	out_uint16_le(s, RDP_CAPLEN_GENERAL);

	out_uint16_le(s, 1);	/* OS major type */
	out_uint16_le(s, 3);	/* OS minor type */
	out_uint16_le(s, 0x200);	/* Protocol version */
	out_uint16(s, 0);	/* Pad */
	out_uint16(s, 0);	/* Compression types */
	out_uint16_le(s, g_use_rdp5 ? 0x40d : 0);
	/* Pad, according to T.128. 0x40d seems to 
	   trigger
	   the server to start sending RDP5 packets. 
	   However, the value is 0x1d04 with W2KTSK and
	   NT4MS. Hmm.. Anyway, thankyou, Microsoft,
	   for sending such information in a padding 
	   field.. */
	out_uint16(s, 0);	/* Update capability */
	out_uint16(s, 0);	/* Remote unshare capability */
	out_uint16(s, 0);	/* Compression level */
	out_uint16(s, 0);	/* Pad */
}

/* Output bitmap capability set */
static void
rdp_out_bitmap_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_BITMAP);
	out_uint16_le(s, RDP_CAPLEN_BITMAP);

	out_uint16_le(s, g_server_depth);	/* Preferred colour depth */
	out_uint16_le(s, 1);	/* Receive 1 BPP */
	out_uint16_le(s, 1);	/* Receive 4 BPP */
	out_uint16_le(s, 1);	/* Receive 8 BPP */
	out_uint16_le(s, 800);	/* Desktop width */
	out_uint16_le(s, 600);	/* Desktop height */
	out_uint16(s, 0);	/* Pad */
	out_uint16(s, 1);	/* Allow resize */
	out_uint16_le(s, g_bitmap_compression ? 1 : 0);	/* Support compression */
	out_uint16(s, 0);	/* Unknown */
	out_uint16_le(s, 1);	/* Unknown */
	out_uint16(s, 0);	/* Pad */
}

/* Output order capability set */
static void
rdp_out_order_caps(STREAM s)
{
	uint8 order_caps[32];

	memset(order_caps, 0, 32);
	order_caps[0] = 1;	/* dest blt */
	order_caps[1] = 1;	/* pat blt */
	order_caps[2] = 1;	/* screen blt */
	order_caps[3] = (g_bitmap_cache ? 1 : 0);	/* memblt */
	order_caps[4] = 0;	/* triblt */
	order_caps[8] = 1;	/* line */
	order_caps[9] = 1;	/* line */
	order_caps[10] = 1;	/* rect */
	order_caps[11] = (g_desktop_save ? 1 : 0);	/* desksave */
	order_caps[13] = 1;	/* memblt */
	order_caps[14] = 1;	/* triblt */
	order_caps[20] = (g_polygon_ellipse_orders ? 1 : 0);	/* polygon */
	order_caps[21] = (g_polygon_ellipse_orders ? 1 : 0);	/* polygon2 */
	order_caps[22] = 1;	/* polyline */
	order_caps[25] = (g_polygon_ellipse_orders ? 1 : 0);	/* ellipse */
	order_caps[26] = (g_polygon_ellipse_orders ? 1 : 0);	/* ellipse2 */
	order_caps[27] = 1;	/* text2 */
	out_uint16_le(s, RDP_CAPSET_ORDER);
	out_uint16_le(s, RDP_CAPLEN_ORDER);

	out_uint8s(s, 20);	/* Terminal desc, pad */
	out_uint16_le(s, 1);	/* Cache X granularity */
	out_uint16_le(s, 20);	/* Cache Y granularity */
	out_uint16(s, 0);	/* Pad */
	out_uint16_le(s, 1);	/* Max order level */
	out_uint16_le(s, 0x147);	/* Number of fonts */
	out_uint16_le(s, 0x2a);	/* Capability flags */
	out_uint8p(s, order_caps, 32);	/* Orders supported */
	out_uint16_le(s, 0x6a1);	/* Text capability flags */
	out_uint8s(s, 6);	/* Pad */
	out_uint32_le(s, g_desktop_save == False ? 0 : 0x38400);	/* Desktop cache size */
	out_uint32(s, 0);	/* Unknown */
	out_uint32_le(s, 0x4e4);	/* Unknown */
}

/* Output bitmap cache capability set */
static void
rdp_out_bmpcache_caps(STREAM s)
{
	int Bpp;
	out_uint16_le(s, RDP_CAPSET_BMPCACHE);
	out_uint16_le(s, RDP_CAPLEN_BMPCACHE);

	Bpp = (g_server_depth + 7) / 8;	/* bytes per pixel */
	out_uint8s(s, 24);	/* unused */
	out_uint16_le(s, 0x258);	/* entries */
	out_uint16_le(s, 0x100 * Bpp);	/* max cell size */
	out_uint16_le(s, 0x12c);	/* entries */
	out_uint16_le(s, 0x400 * Bpp);	/* max cell size */
	out_uint16_le(s, 0x106);	/* entries */
	out_uint16_le(s, 0x1000 * Bpp);	/* max cell size */
}

/* Output bitmap cache v2 capability set */
static void
rdp_out_bmpcache2_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_BMPCACHE2);
	out_uint16_le(s, RDP_CAPLEN_BMPCACHE2);

	out_uint16_le(s, g_bitmap_cache_persist_enable ? 2 : 0);	/* version */

	out_uint16_be(s, 3);	/* number of caches in this set */

	/* max cell size for cache 0 is 16x16, 1 = 32x32, 2 = 64x64, etc */
	out_uint32_le(s, BMPCACHE2_C0_CELLS);
	out_uint32_le(s, BMPCACHE2_C1_CELLS);
	if (pstcache_init(2))
	{
		out_uint32_le(s, BMPCACHE2_NUM_PSTCELLS | BMPCACHE2_FLAG_PERSIST);
	}
	else
	{
		out_uint32_le(s, BMPCACHE2_C2_CELLS);
	}
	out_uint8s(s, 20);	/* other bitmap caches not used */
}

/* Output control capability set */
static void
rdp_out_control_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_CONTROL);
	out_uint16_le(s, RDP_CAPLEN_CONTROL);

	out_uint16(s, 0);	/* Control capabilities */
	out_uint16(s, 0);	/* Remote detach */
	out_uint16_le(s, 2);	/* Control interest */
	out_uint16_le(s, 2);	/* Detach interest */
}

/* Output activation capability set */
static void
rdp_out_activate_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_ACTIVATE);
	out_uint16_le(s, RDP_CAPLEN_ACTIVATE);

	out_uint16(s, 0);	/* Help key */
	out_uint16(s, 0);	/* Help index key */
	out_uint16(s, 0);	/* Extended help key */
	out_uint16(s, 0);	/* Window activate */
}

/* Output pointer capability set */
static void
rdp_out_pointer_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_POINTER);
	out_uint16_le(s, RDP_CAPLEN_POINTER);

	out_uint16(s, 0);	/* Colour pointer */
	out_uint16_le(s, 20);	/* Cache size */
}

/* Output share capability set */
static void
rdp_out_share_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_SHARE);
	out_uint16_le(s, RDP_CAPLEN_SHARE);

	out_uint16(s, 0);	/* userid */
	out_uint16(s, 0);	/* pad */
}

/* Output colour cache capability set */
static void
rdp_out_colcache_caps(STREAM s)
{
	out_uint16_le(s, RDP_CAPSET_COLCACHE);
	out_uint16_le(s, RDP_CAPLEN_COLCACHE);

	out_uint16_le(s, 6);	/* cache size */
	out_uint16(s, 0);	/* pad */
}

static uint8 caps_0x0d[] = {
	0x01, 0x00, 0x00, 0x00, 0x09, 0x04, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
};

static uint8 caps_0x0c[] = { 0x01, 0x00, 0x00, 0x00 };

static uint8 caps_0x0e[] = { 0x01, 0x00, 0x00, 0x00 };

static uint8 caps_0x10[] = {
	0xFE, 0x00, 0x04, 0x00, 0xFE, 0x00, 0x04, 0x00,
	0xFE, 0x00, 0x08, 0x00, 0xFE, 0x00, 0x08, 0x00,
	0xFE, 0x00, 0x10, 0x00, 0xFE, 0x00, 0x20, 0x00,
	0xFE, 0x00, 0x40, 0x00, 0xFE, 0x00, 0x80, 0x00,
	0xFE, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x08,
	0x00, 0x01, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00
};

/* Output unknown capability sets */
static void
rdp_out_unknown_caps(STREAM s, uint16 id, uint16 length, uint8 * caps)
{
	out_uint16_le(s, id);
	out_uint16_le(s, length);

	out_uint8p(s, caps, length - 4);
}

#define RDP5_FLAG 0x0030
/* Send a confirm active PDU */
static void
rdp_send_confirm_active(void)
{
	STREAM s;
	uint32 sec_flags = g_encryption ? (RDP5_FLAG | SEC_ENCRYPT) : RDP5_FLAG;
	uint16 caplen =
		RDP_CAPLEN_GENERAL + RDP_CAPLEN_BITMAP + RDP_CAPLEN_ORDER +
		RDP_CAPLEN_BMPCACHE + RDP_CAPLEN_COLCACHE +
		RDP_CAPLEN_ACTIVATE + RDP_CAPLEN_CONTROL +
		RDP_CAPLEN_POINTER + RDP_CAPLEN_SHARE +
		0x58 + 0x08 + 0x08 + 0x34 /* unknown caps */  +
		4 /* w2k fix, why? */ ;

	s = sec_init(sec_flags, 6 + 14 + caplen + sizeof(RDP_SOURCE));

	out_uint16_le(s, 2 + 14 + caplen + sizeof(RDP_SOURCE));
	out_uint16_le(s, (RDP_PDU_CONFIRM_ACTIVE | 0x10));	/* Version 1 */
	out_uint16_le(s, (g_mcs_userid + 1001));

	out_uint32_le(s, g_rdp_shareid);
	out_uint16_le(s, 0x3ea);	/* userid */
	out_uint16_le(s, sizeof(RDP_SOURCE));
	out_uint16_le(s, caplen);

	out_uint8p(s, RDP_SOURCE, sizeof(RDP_SOURCE));
	out_uint16_le(s, 0xd);	/* num_caps */
	out_uint8s(s, 2);	/* pad */

	rdp_out_general_caps(s);
	rdp_out_bitmap_caps(s);
	rdp_out_order_caps(s);
	g_use_rdp5 ? rdp_out_bmpcache2_caps(s) : rdp_out_bmpcache_caps(s);
	rdp_out_colcache_caps(s);
	rdp_out_activate_caps(s);
	rdp_out_control_caps(s);
	rdp_out_pointer_caps(s);
	rdp_out_share_caps(s);

	rdp_out_unknown_caps(s, 0x0d, 0x58, caps_0x0d);	/* international? */
	rdp_out_unknown_caps(s, 0x0c, 0x08, caps_0x0c);
	rdp_out_unknown_caps(s, 0x0e, 0x08, caps_0x0e);
	rdp_out_unknown_caps(s, 0x10, 0x34, caps_0x10);	/* glyph cache? */

	s_mark_end(s);
	sec_send(s, sec_flags);
}

/* Process a general capability set */
static void
rdp_process_general_caps(STREAM s)
{
	uint16 pad2octetsB;	/* rdp5 flags? */

	in_uint8s(s, 10);
	in_uint16_le(s, pad2octetsB);

	if (!pad2octetsB)
		g_use_rdp5 = False;
}

/* Process a bitmap capability set */
static void
rdp_process_bitmap_caps(STREAM s)
{
	uint16 width, height, depth;

	in_uint16_le(s, depth);
	in_uint8s(s, 6);

	in_uint16_le(s, width);
	in_uint16_le(s, height);

	DEBUG(("setting desktop size and depth to: %dx%dx%d\n", width, height, depth));

	/*
	 * The server may limit depth and change the size of the desktop (for
	 * example when shadowing another session).
	 */
	if (g_server_depth != depth)
	{
		warning("Remote desktop does not support colour depth %d; falling back to %d\n",
			g_server_depth, depth);
		g_server_depth = depth;
	}
	if (g_width != width || g_height != height)
	{
		warning("Remote desktop changed from %dx%d to %dx%d.\n", g_width, g_height,
			width, height);
		g_width = width;
		g_height = height;
		ui_resize_window();
	}
}

/* Process server capabilities */
static void
rdp_process_server_caps(STREAM s, uint16 length)
{
	int n;
	uint8 *next, *start;
	uint16 ncapsets, capset_type, capset_length;

	start = s->p;

	in_uint16_le(s, ncapsets);
	in_uint8s(s, 2);	/* pad */

	for (n = 0; n < ncapsets; n++)
	{
		if (s->p > start + length)
			return;

		in_uint16_le(s, capset_type);
		in_uint16_le(s, capset_length);

		next = s->p + capset_length - 4;

		switch (capset_type)
		{
			case RDP_CAPSET_GENERAL:
				rdp_process_general_caps(s);
				break;

			case RDP_CAPSET_BITMAP:
				rdp_process_bitmap_caps(s);
				break;
		}

		s->p = next;
	}
}

/* Respond to a demand active PDU */
static void
process_demand_active(STREAM s)
{
	uint8 type;
	uint16 len_src_descriptor, len_combined_caps;

	in_uint32_le(s, g_rdp_shareid);
	in_uint16_le(s, len_src_descriptor);
	in_uint16_le(s, len_combined_caps);
	in_uint8s(s, len_src_descriptor);

	DEBUG(("DEMAND_ACTIVE(id=0x%x)\n", g_rdp_shareid));
	rdp_process_server_caps(s, len_combined_caps);

	rdp_send_confirm_active();
	rdp_send_synchronise();
	rdp_send_control(RDP_CTL_COOPERATE);
	rdp_send_control(RDP_CTL_REQUEST_CONTROL);
	rdp_recv(&type);	/* RDP_PDU_SYNCHRONIZE */
	rdp_recv(&type);	/* RDP_CTL_COOPERATE */
	rdp_recv(&type);	/* RDP_CTL_GRANT_CONTROL */
	rdp_send_input(0, RDP_INPUT_SYNCHRONIZE, 0, ui_get_numlock_state(read_keyboard_state()), 0);

	if (g_use_rdp5)
	{
		rdp_enum_bmpcache2();
		rdp_send_fonts(3);
	}
	else
	{
		rdp_send_fonts(1);
		rdp_send_fonts(2);
	}

	rdp_recv(&type);	/* RDP_PDU_UNKNOWN 0x28 (Fonts?) */
	reset_order_state();
}

/* Process a colour pointer PDU */
void
process_colour_pointer_pdu(STREAM s)
{
	uint16 x, y, width, height, cache_idx, masklen, datalen;
	uint8 *mask, *data;
	HCURSOR cursor;

	in_uint16_le(s, cache_idx);
	in_uint16_le(s, x);
	in_uint16_le(s, y);
	in_uint16_le(s, width);
	in_uint16_le(s, height);
	in_uint16_le(s, masklen);
	in_uint16_le(s, datalen);
	in_uint8p(s, data, datalen);
	in_uint8p(s, mask, masklen);
	cursor = ui_create_cursor(x, y, width, height, mask, data);
	ui_set_cursor(cursor);
	cache_put_cursor(cache_idx, cursor);
}

/* Process a cached pointer PDU */
void
process_cached_pointer_pdu(STREAM s)
{
	uint16 cache_idx;

	in_uint16_le(s, cache_idx);
	ui_set_cursor(cache_get_cursor(cache_idx));
}

/* Process a system pointer PDU */
void
process_system_pointer_pdu(STREAM s)
{
	uint16 system_pointer_type;

	in_uint16(s, system_pointer_type);
	switch (system_pointer_type)
	{
		case RDP_NULL_POINTER:
			ui_set_null_cursor();
			break;

		default:
			unimpl("System pointer message 0x%x\n", system_pointer_type);
	}
}

/* Process a pointer PDU */
static void
process_pointer_pdu(STREAM s)
{
	uint16 message_type;
	uint16 x, y;

	in_uint16_le(s, message_type);
	in_uint8s(s, 2);	/* pad */

	switch (message_type)
	{
		case RDP_POINTER_MOVE:
			in_uint16_le(s, x);
			in_uint16_le(s, y);
			if (s_check(s))
				ui_move_pointer(x, y);
			break;

		case RDP_POINTER_COLOR:
			process_colour_pointer_pdu(s);
			break;

		case RDP_POINTER_CACHED:
			process_cached_pointer_pdu(s);
			break;

		case RDP_POINTER_SYSTEM:
			process_system_pointer_pdu(s);
			break;

		default:
			unimpl("Pointer message 0x%x\n", message_type);
	}
}

/* Process bitmap updates */
void
process_bitmap_updates(STREAM s)
{
	uint16 num_updates;
	uint16 left, top, right, bottom, width, height;
	uint16 cx, cy, bpp, Bpp, compress, bufsize, size;
	uint8 *data, *bmpdata;
	int i;

	in_uint16_le(s, num_updates);

	for (i = 0; i < num_updates; i++)
	{
		in_uint16_le(s, left);
		in_uint16_le(s, top);
		in_uint16_le(s, right);
		in_uint16_le(s, bottom);
		in_uint16_le(s, width);
		in_uint16_le(s, height);
		in_uint16_le(s, bpp);
		Bpp = (bpp + 7) / 8;
		in_uint16_le(s, compress);
		in_uint16_le(s, bufsize);

		cx = right - left + 1;
		cy = bottom - top + 1;

		DEBUG(("BITMAP_UPDATE(l=%d,t=%d,r=%d,b=%d,w=%d,h=%d,Bpp=%d,cmp=%d)\n",
		       left, top, right, bottom, width, height, Bpp, compress));

		if (!compress)
		{
			int y;
			bmpdata = (uint8 *) xmalloc(width * height * Bpp);
			for (y = 0; y < height; y++)
			{
				in_uint8a(s, &bmpdata[(height - y - 1) * (width * Bpp)],
					  width * Bpp);
			}
			ui_paint_bitmap(left, top, cx, cy, width, height, bmpdata);
			xfree(bmpdata);
			continue;
		}


		if (compress & 0x400)
		{
			size = bufsize;
		}
		else
		{
			in_uint8s(s, 2);	/* pad */
			in_uint16_le(s, size);
			in_uint8s(s, 4);	/* line_size, final_size */
		}
		in_uint8p(s, data, size);
		bmpdata = (uint8 *) xmalloc(width * height * Bpp);
		if (bitmap_decompress(bmpdata, width, height, data, size, Bpp))
		{
			ui_paint_bitmap(left, top, cx, cy, width, height, bmpdata);
		}
		else
		{
			DEBUG_RDP5(("Failed to decompress data\n"));
		}

		xfree(bmpdata);
	}
}

/* Process a palette update */
void
process_palette(STREAM s)
{
	COLOURENTRY *entry;
	COLOURMAP map;
	RD_HCOLOURMAP hmap;
	int i;

	in_uint8s(s, 2);	/* pad */
	in_uint16_le(s, map.ncolours);
	in_uint8s(s, 2);	/* pad */

	map.colours = (COLOURENTRY *) xmalloc(sizeof(COLOURENTRY) * map.ncolours);

	DEBUG(("PALETTE(c=%d)\n", map.ncolours));

	for (i = 0; i < map.ncolours; i++)
	{
		entry = &map.colours[i];
		in_uint8(s, entry->red);
		in_uint8(s, entry->green);
		in_uint8(s, entry->blue);
	}

	hmap = ui_create_colourmap(&map);
	ui_set_colourmap(hmap);

	xfree(map.colours);
}

/* Process an update PDU */
static void
process_update_pdu(STREAM s)
{
	uint16 update_type, count;

	in_uint16_le(s, update_type);

	ui_begin_update();
	switch (update_type)
	{
		case RDP_UPDATE_ORDERS:
			in_uint8s(s, 2);	/* pad */
			in_uint16_le(s, count);
			in_uint8s(s, 2);	/* pad */
			process_orders(s, count);
			break;

		case RDP_UPDATE_BITMAP:
			process_bitmap_updates(s);
			break;

		case RDP_UPDATE_PALETTE:
			process_palette(s);
			break;

		case RDP_UPDATE_SYNCHRONIZE:
			break;

		default:
			unimpl("update %d\n", update_type);
	}
	ui_end_update();
}

/* Process a disconnect PDU */
void
process_disconnect_pdu(STREAM s, uint32 * ext_disc_reason)
{
	in_uint32_le(s, *ext_disc_reason);

	DEBUG(("Received disconnect PDU\n"));
}

/* Process data PDU */
static BOOL
process_data_pdu(STREAM s, uint32 * ext_disc_reason)
{
	uint8 data_pdu_type;
	uint8 ctype;
	uint16 clen;
	uint32 len;

	uint32 roff, rlen;

	struct stream *ns = &(g_mppc_dict.ns);

	in_uint8s(s, 6);	/* shareid, pad, streamid */
	in_uint16(s, len);
	in_uint8(s, data_pdu_type);
	in_uint8(s, ctype);
	in_uint16(s, clen);
	clen -= 18;

	if (ctype & RDP_MPPC_COMPRESSED)
	{
		if (len > RDP_MPPC_DICT_SIZE)
			error("error decompressed packet size exceeds max\n");
		if (mppc_expand(s->p, clen, ctype, &roff, &rlen) == -1)
			error("error while decompressing packet\n");

		/* len -= 18; */

		/* allocate memory and copy the uncompressed data into the temporary stream */
		ns->data = (uint8 *) xrealloc(ns->data, rlen);

		memcpy((ns->data), (unsigned char *) (g_mppc_dict.hist + roff), rlen);

		ns->size = rlen;
		ns->end = (ns->data + ns->size);
		ns->p = ns->data;
		ns->rdp_hdr = ns->p;

		s = ns;
	}

	switch (data_pdu_type)
	{
		case RDP_DATA_PDU_UPDATE:
			process_update_pdu(s);
			break;

		case RDP_DATA_PDU_CONTROL:
			DEBUG(("Received Control PDU\n"));
			break;

		case RDP_DATA_PDU_SYNCHRONISE:
			DEBUG(("Received Sync PDU\n"));
			break;

		case RDP_DATA_PDU_POINTER:
			process_pointer_pdu(s);
			break;

		case RDP_DATA_PDU_BELL:
			ui_bell();
			break;

		case RDP_DATA_PDU_LOGON:
			DEBUG(("Received Logon PDU\n"));
			/* User logged on */
			break;

		case RDP_DATA_PDU_DISCONNECT:
			process_disconnect_pdu(s, ext_disc_reason);
			return True;

		default:
			unimpl("data PDU %d\n", data_pdu_type);
	}
	return False;
}

/* Process redirect PDU from Session Directory */
static BOOL
process_redirect_pdu(STREAM s /*, uint32 * ext_disc_reason */ )
{
	uint32 len;

	/* these 2 bytes are unknown, seem to be zeros */
	in_uint8s(s, 2);

	/* read connection flags */
	in_uint32_le(s, g_redirect_flags);

	/* read length of ip string */
	in_uint32_le(s, len);

	/* read ip string */
	rdp_in_unistr(s, g_redirect_server, len);

	/* read length of cookie string */
	in_uint32_le(s, len);

	/* read cookie string (plain ASCII) */
	in_uint8a(s, g_redirect_cookie, len);
	g_redirect_cookie[len] = 0;

	/* read length of username string */
	in_uint32_le(s, len);

	/* read username string */
	rdp_in_unistr(s, g_redirect_username, len);

	/* read length of domain string */
	in_uint32_le(s, len);

	/* read domain string */
	rdp_in_unistr(s, g_redirect_domain, len);

	/* read length of password string */
	in_uint32_le(s, len);

	/* read password string */
	rdp_in_unistr(s, g_redirect_password, len);

	g_redirect = True;

	return True;
}

/* Process incoming packets */
/* nevers gets out of here till app is done */
void
rdp_main_loop(BOOL * deactivated, uint32 * ext_disc_reason)
{
	while (rdp_loop(deactivated, ext_disc_reason))
		;
}

/* used in uiports and rdp_main_loop, processes the rdp packets waiting */
BOOL
rdp_loop(BOOL * deactivated, uint32 * ext_disc_reason)
{
	uint8 type;
	BOOL disc = False;	/* True when a disconnect PDU was received */
	BOOL cont = True;
	STREAM s;

	while (cont)
	{
		s = rdp_recv(&type);
		if (s == NULL)
			return False;
		switch (type)
		{
			case RDP_PDU_DEMAND_ACTIVE:
				process_demand_active(s);
				*deactivated = False;
				break;
			case RDP_PDU_DEACTIVATE:
				DEBUG(("RDP_PDU_DEACTIVATE\n"));
				*deactivated = True;
				break;
			case RDP_PDU_REDIRECT:
				return process_redirect_pdu(s);
				break;
			case RDP_PDU_DATA:
				disc = process_data_pdu(s, ext_disc_reason);
				break;
			case 0:
				break;
			default:
				unimpl("PDU %d\n", type);
		}
		if (disc)
			return False;
		cont = g_next_packet < s->end;
	}
	return True;
}

/* Establish a connection up to the RDP layer */
BOOL
rdp_connect(char *server, uint32 flags, char *domain, char *password,
	    char *command, char *directory)
{
	if (!sec_connect(server, g_username))
		return False;

	rdp_send_logon_info(flags, domain, g_username, password, command, directory);
	return True;
}

/* Establish a reconnection up to the RDP layer */
BOOL
rdp_reconnect(char *server, uint32 flags, char *domain, char *password,
	      char *command, char *directory, char *cookie)
{
	if (!sec_reconnect(server))
		return False;

	rdp_send_logon_info(flags, domain, g_username, password, command, directory);
	return True;
}

/* Called during redirection to reset the state to support redirection */
void
rdp_reset_state(void)
{
	g_next_packet = NULL;	/* reset the packet information */
	g_rdp_shareid = 0;
	sec_reset_state();
}

/* Disconnect from the RDP layer */
void
rdp_disconnect(void)
{
	sec_disconnect();
}
