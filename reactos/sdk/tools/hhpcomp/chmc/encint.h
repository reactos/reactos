/*

  Copyright (C) 2010 Alex Andreotti <alex.andreotti@gmail.com>

  This file is part of chmc.

  chmc is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  chmc is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with chmc.  If not, see <http://www.gnu.org/licenses/>.

*/
#ifndef CHMC_ENCINT_H
#define CHMC_ENCINT_H

// 0x7f 127
// 0x3fff 16383
// 0x1fffff 2097151
// 0xfffffff 268435455
static inline int chmc_encint_len ( const UInt32 val ) {
	int len;

	// FIXME should support 64 bit?
	if ( val > 0xfffffffUL )
		len = 0; // overflow
	else if ( val > 0x1fffffUL )
		len = 4;
	else if ( val > 0x3fffUL )
		len = 3;
	else if ( val > 0x7fUL )
		len = 2;
	else
		len = 1;

	return len;
}

static inline int chmc_encint ( const UInt32 val, UChar *out ) {
	int len;
	UInt32 a;
	UChar *p, *l;

	// FIXME should support 64 bit?
	if ( ! out || val > 0xfffffffUL )
		return 0; // FIXME can't handle, overflow

	if ( val > 0x1fffffUL )
		len = 4;
	else if ( val > 0x3fffUL )
		len = 3;
	else if ( val > 0x7fUL )
		len = 2;
	else
		len = 1;

	a = val;
	l = p = out + (len - 1);

	while ( p >= out ) {
		*p = (a & 0x7fUL);
		if ( p < l )
			*p |= 0x80UL;
		p--;
		a >>= 7;
	}

	return len;
}

static inline int chmc_decint ( const UChar *in, UInt32 *value ) {
	int len;

	len = 0;
	*value = 0;

	while ( (in[len] & 0x80) && (len < 3) ) {
		*value <<= 7;
		*value |= in[len] & 0x7f;
		len++;
	}
	*value <<= 7;
	*value |= in[len] & 0x7f;
	len++;

	return len;
}

#endif /* CHMC_ENCINT_H */
