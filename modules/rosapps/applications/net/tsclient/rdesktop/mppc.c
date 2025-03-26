/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - RDP decompression
   Copyright (C) Matthew Chapman 1999-2005

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>

#include "rdesktop.h"

/* mppc decompression                       */
/* http://www.faqs.org/rfcs/rfc2118.html    */

/* Contacts:                                */

/* hifn contact mentioned in the faq is     */
/* Robert Friend rfriend at hifn dot com    */

/* if you have questions regarding MPPC     */
/* our contact is                           */
/* Guus Dhaeze  GDhaeze at hifn dot com     */

/* Licensing:                               */

/* decompression is alright as long as we   */
/* don't compress data                      */

/* Algorithm: */

/* as the rfc states the algorithm seems to */
/* be LZ77 with a sliding buffer            */
/* that is empty at init.                   */

/* the algorithm is called LZS and is       */
/* patented for another couple of years.    */

/* more information is available in         */
/* https://web.archive.org/web/20231203135154/https://www.ietf.org/ietf-ftp/ietf/IPR/hifn-ipr-draft-friend-tls-lzs-compression.txt */

int
mppc_expand(RDPCLIENT * This, uint8 * data, uint32 clen, uint8 ctype, uint32 * roff, uint32 * rlen)
{
	int k, walker_len = 0, walker;
	uint32 i = 0;
	int next_offset, match_off;
	int match_len;
	int old_offset, match_bits;
	BOOL big = ctype & RDP_MPPC_BIG ? True : False;

	uint8 *dict = This->mppc_dict.hist;

	if ((ctype & RDP_MPPC_COMPRESSED) == 0)
	{
		*roff = 0;
		*rlen = clen;
		return 0;
	}

	if ((ctype & RDP_MPPC_RESET) != 0)
	{
		This->mppc_dict.roff = 0;
	}

	if ((ctype & RDP_MPPC_FLUSH) != 0)
	{
		memset(dict, 0, RDP_MPPC_DICT_SIZE);
		This->mppc_dict.roff = 0;
	}

	*roff = 0;
	*rlen = 0;

	walker = This->mppc_dict.roff;

	next_offset = walker;
	old_offset = next_offset;
	*roff = old_offset;
	if (clen == 0)
		return 0;
	clen += i;

	do
	{
		if (walker_len == 0)
		{
			if (i >= clen)
				break;
			walker = data[i++] << 24;
			walker_len = 8;
		}
		if (walker >= 0)
		{
			if (walker_len < 8)
			{
				if (i >= clen)
				{
					if (walker != 0)
						return -1;
					break;
				}
				walker |= (data[i++] & 0xff) << (24 - walker_len);
				walker_len += 8;
			}
			if (next_offset >= RDP_MPPC_DICT_SIZE)
				return -1;
			dict[next_offset++] = (((uint32) walker) >> ((uint32) 24));
			walker <<= 8;
			walker_len -= 8;
			continue;
		}
		walker <<= 1;
		/* fetch next 8-bits */
		if (--walker_len == 0)
		{
			if (i >= clen)
				return -1;
			walker = data[i++] << 24;
			walker_len = 8;
		}
		/* literal decoding */
		if (walker >= 0)
		{
			if (walker_len < 8)
			{
				if (i >= clen)
					return -1;
				walker |= (data[i++] & 0xff) << (24 - walker_len);
				walker_len += 8;
			}
			if (next_offset >= RDP_MPPC_DICT_SIZE)
				return -1;
			dict[next_offset++] = (uint8) (walker >> 24 | 0x80);
			walker <<= 8;
			walker_len -= 8;
			continue;
		}

		/* decode offset  */
		/* length pair    */
		walker <<= 1;
		if (--walker_len < (big ? 3 : 2))
		{
			if (i >= clen)
				return -1;
			walker |= (data[i++] & 0xff) << (24 - walker_len);
			walker_len += 8;
		}

		if (big)
		{
			/* offset decoding where offset len is:
			   -63: 11111 followed by the lower 6 bits of the value
			   64-319: 11110 followed by the lower 8 bits of the value ( value - 64 )
			   320-2367: 1110 followed by lower 11 bits of the value ( value - 320 )
			   2368-65535: 110 followed by lower 16 bits of the value ( value - 2368 )
			 */
			switch (((uint32) walker) >> ((uint32) 29))
			{
				case 7:	/* - 63 */
					for (; walker_len < 9; walker_len += 8)
					{
						if (i >= clen)
							return -1;
						walker |= (data[i++] & 0xff) << (24 - walker_len);
					}
					walker <<= 3;
					match_off = ((uint32) walker) >> ((uint32) 26);
					walker <<= 6;
					walker_len -= 9;
					break;

				case 6:	/* 64 - 319 */
					for (; walker_len < 11; walker_len += 8)
					{
						if (i >= clen)
							return -1;
						walker |= (data[i++] & 0xff) << (24 - walker_len);
					}

					walker <<= 3;
					match_off = (((uint32) walker) >> ((uint32) 24)) + 64;
					walker <<= 8;
					walker_len -= 11;
					break;

				case 5:
				case 4:	/* 320 - 2367 */
					for (; walker_len < 13; walker_len += 8)
					{
						if (i >= clen)
							return -1;
						walker |= (data[i++] & 0xff) << (24 - walker_len);
					}

					walker <<= 2;
					match_off = (((uint32) walker) >> ((uint32) 21)) + 320;
					walker <<= 11;
					walker_len -= 13;
					break;

				default:	/* 2368 - 65535 */
					for (; walker_len < 17; walker_len += 8)
					{
						if (i >= clen)
							return -1;
						walker |= (data[i++] & 0xff) << (24 - walker_len);
					}

					walker <<= 1;
					match_off = (((uint32) walker) >> ((uint32) 16)) + 2368;
					walker <<= 16;
					walker_len -= 17;
					break;
			}
		}
		else
		{
			/* offset decoding where offset len is:
			   -63: 1111 followed by the lower 6 bits of the value
			   64-319: 1110 followed by the lower 8 bits of the value ( value - 64 )
			   320-8191: 110 followed by the lower 13 bits of the value ( value - 320 )
			 */
			switch (((uint32) walker) >> ((uint32) 30))
			{
				case 3:	/* - 63 */
					if (walker_len < 8)
					{
						if (i >= clen)
							return -1;
						walker |= (data[i++] & 0xff) << (24 - walker_len);
						walker_len += 8;
					}
					walker <<= 2;
					match_off = ((uint32) walker) >> ((uint32) 26);
					walker <<= 6;
					walker_len -= 8;
					break;

				case 2:	/* 64 - 319 */
					for (; walker_len < 10; walker_len += 8)
					{
						if (i >= clen)
							return -1;
						walker |= (data[i++] & 0xff) << (24 - walker_len);
					}

					walker <<= 2;
					match_off = (((uint32) walker) >> ((uint32) 24)) + 64;
					walker <<= 8;
					walker_len -= 10;
					break;

				default:	/* 320 - 8191 */
					for (; walker_len < 14; walker_len += 8)
					{
						if (i >= clen)
							return -1;
						walker |= (data[i++] & 0xff) << (24 - walker_len);
					}

					match_off = (walker >> 18) + 320;
					walker <<= 14;
					walker_len -= 14;
					break;
			}
		}
		if (walker_len == 0)
		{
			if (i >= clen)
				return -1;
			walker = data[i++] << 24;
			walker_len = 8;
		}

		/* decode length of match */
		match_len = 0;
		if (walker >= 0)
		{		/* special case - length of 3 is in bit 0 */
			match_len = 3;
			walker <<= 1;
			walker_len--;
		}
		else
		{
			/* this is how it works len of:
			   4-7: 10 followed by 2 bits of the value
			   8-15: 110 followed by 3 bits of the value
			   16-31: 1110 followed by 4 bits of the value
			   32-63: .... and so forth
			   64-127:
			   128-255:
			   256-511:
			   512-1023:
			   1024-2047:
			   2048-4095:
			   4096-8191:

			   i.e. 4097 is encoded as: 111111111110 000000000001
			   meaning 4096 + 1...
			 */
			match_bits = big ? 14 : 11;	/* 11 or 14 bits of value at most */
			do
			{
				walker <<= 1;
				if (--walker_len == 0)
				{
					if (i >= clen)
						return -1;
					walker = data[i++] << 24;
					walker_len = 8;
				}
				if (walker >= 0)
					break;
				if (--match_bits == 0)
				{
					return -1;
				}
			}
			while (1);
			match_len = (big ? 16 : 13) - match_bits;
			walker <<= 1;
			if (--walker_len < match_len)
			{
				for (; walker_len < match_len; walker_len += 8)
				{
					if (i >= clen)
					{
						return -1;
					}
					walker |= (data[i++] & 0xff) << (24 - walker_len);
				}
			}

			match_bits = match_len;
			match_len =
				((walker >> (32 - match_bits)) & (~(-1 << match_bits))) | (1 <<
											   match_bits);
			walker <<= match_bits;
			walker_len -= match_bits;
		}
		if (next_offset + match_len >= RDP_MPPC_DICT_SIZE)
		{
			return -1;
		}
		/* memory areas can overlap - meaning we can't use memXXX functions */
		k = (next_offset - match_off) & (big ? 65535 : 8191);
		do
		{
			dict[next_offset++] = dict[k++];
		}
		while (--match_len != 0);
	}
	while (1);

	/* store history offset */
	This->mppc_dict.roff = next_offset;

	*roff = old_offset;
	*rlen = next_offset - old_offset;

	return 0;
}
