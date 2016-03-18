/*
   rdesktop: A Remote Desktop Protocol client.
   Parsing primitives
   Copyright (C) Matthew Chapman 1999-2008
   Copyright 2012 Henrik Andersson <hean01@cendio.se> for Cendio AB

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

/* Parser state */
typedef struct stream
{
	unsigned char *p;
	unsigned char *end;
	unsigned char *data;
	unsigned int size;

	/* Offsets of various headers */
	unsigned char *iso_hdr;
	unsigned char *mcs_hdr;
	unsigned char *sec_hdr;
	unsigned char *rdp_hdr;
	unsigned char *channel_hdr;

}
 *STREAM;

#define s_push_layer(s,h,n)	{ (s)->h = (s)->p; (s)->p += n; }
#define s_pop_layer(s,h)	(s)->p = (s)->h;
#define s_mark_end(s)		(s)->end = (s)->p;
#define s_check(s)		((s)->p <= (s)->end)
#define s_check_rem(s,n)	((s)->p + n <= (s)->end)
#define s_check_end(s)		((s)->p == (s)->end)
#define s_length(s)		((s)->end - (s)->data)
#define s_reset(s)		((s)->end = (s)->p = (s)->data)

#if defined(L_ENDIAN) && !defined(NEED_ALIGN)
#define in_uint16_le(s,v)	{ v = *(uint16 *)((s)->p); (s)->p += 2; }
#define in_uint32_le(s,v)	{ v = *(uint32 *)((s)->p); (s)->p += 4; }
#define out_uint16_le(s,v)	{ *(uint16 *)((s)->p) = v; (s)->p += 2; }
#define out_uint32_le(s,v)	{ *(uint32 *)((s)->p) = v; (s)->p += 4; }

#else
#define in_uint16_le(s,v)	{ v = *((s)->p++); v += *((s)->p++) << 8; }
#define in_uint32_le(s,v)	{ in_uint16_le(s,v) \
				v += *((s)->p++) << 16; v += *((s)->p++) << 24; }
#define out_uint16_le(s,v)	{ *((s)->p++) = (v) & 0xff; *((s)->p++) = ((v) >> 8) & 0xff; }
#define out_uint32_le(s,v)	{ out_uint16_le(s, (v) & 0xffff); out_uint16_le(s, ((v) >> 16) & 0xffff); }
#endif

#if defined(B_ENDIAN) && !defined(NEED_ALIGN)
#define in_uint16_be(s,v)	{ v = *(uint16 *)((s)->p); (s)->p += 2; }
#define in_uint32_be(s,v)	{ v = *(uint32 *)((s)->p); (s)->p += 4; }
#define out_uint16_be(s,v)	{ *(uint16 *)((s)->p) = v; (s)->p += 2; }
#define out_uint32_be(s,v)	{ *(uint32 *)((s)->p) = v; (s)->p += 4; }

#define B_ENDIAN_PREFERRED
#define in_uint16(s,v)		in_uint16_be(s,v)
#define in_uint32(s,v)		in_uint32_be(s,v)
#define out_uint16(s,v)		out_uint16_be(s,v)
#define out_uint32(s,v)		out_uint32_be(s,v)

#else
#define in_uint16_be(s,v)	{ v = *((s)->p++); next_be(s,v); }
#define in_uint32_be(s,v)	{ in_uint16_be(s,v); next_be(s,v); next_be(s,v); }
#define out_uint16_be(s,v)	{ *((s)->p++) = ((v) >> 8) & 0xff; *((s)->p++) = (v) & 0xff; }
#define out_uint32_be(s,v)	{ out_uint16_be(s, ((v) >> 16) & 0xffff); out_uint16_be(s, (v) & 0xffff); }
#endif

#ifndef B_ENDIAN_PREFERRED
#define in_uint16(s,v)		in_uint16_le(s,v)
#define in_uint32(s,v)		in_uint32_le(s,v)
#define out_uint16(s,v)		out_uint16_le(s,v)
#define out_uint32(s,v)		out_uint32_le(s,v)
#endif

#define in_uint8(s,v)		v = *((s)->p++);
#define in_uint8p(s,v,n)	{ v = (s)->p; (s)->p += n; }
#define in_uint8a(s,v,n)	{ memcpy(v,(s)->p,n); (s)->p += n; }
#define in_uint8s(s,n)		(s)->p += n;
#define out_uint8(s,v)		*((s)->p++) = v;
#define out_uint8p(s,v,n)	{ memcpy((s)->p,v,n); (s)->p += n; }
#define out_uint8a(s,v,n)	out_uint8p(s,v,n);
#define out_uint8s(s,n)		{ memset((s)->p,0,n); (s)->p += n; }

#define next_be(s,v)		v = ((v) << 8) + *((s)->p++);
