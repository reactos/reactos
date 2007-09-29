/*        This code illustrates a sample implementation
 *                 of the Arcfour algorithm
 *         Copyright (c) April 29, 1997 Kalle Kaukonen.
 *                    All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that this copyright
 * notice and disclaimer are retained.
 *
 * THIS SOFTWARE IS PROVIDED BY KALLE KAUKONEN AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL KALLE
 * KAUKONEN OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fitz-base.h"
#include "fitz-stream.h"

void
fz_arc4init(fz_arc4 *arc4, unsigned char *key, unsigned keylen)
{
	unsigned int t, u;
	unsigned int keyindex;
	unsigned int stateindex;
	unsigned char *state;
	unsigned int counter;

	state = arc4->state;

	arc4->x = 0;
	arc4->y = 0;

	for (counter = 0; counter < 256; counter++) {
		state[counter] = counter;
	}

	keyindex = 0;
	stateindex = 0;

	for (counter = 0; counter < 256; counter++) {
		t = state[counter];
		stateindex = (stateindex + key[keyindex] + t) & 0xff;
		u = state[stateindex];

		state[stateindex] = t;
		state[counter] = u;

		if (++keyindex >= keylen) {
			keyindex = 0;
		}
	}
}

unsigned char
fz_arc4next(fz_arc4 *arc4)
{
	unsigned int x;
	unsigned int y;
	unsigned int sx, sy;
	unsigned char *state;

	state = arc4->state;

	x = (arc4->x + 1) & 0xff;
	sx = state[x];
	y = (sx + arc4->y) & 0xff;
	sy = state[y];

	arc4->x = x;
	arc4->y = y;

	state[y] = sx;
	state[x] = sy;

	return state[(sx + sy) & 0xff];
}

void
fz_arc4encrypt(fz_arc4 *arc4, unsigned char *dest, unsigned char *src, unsigned len)
{
	unsigned int i;
	for (i = 0; i < len; i++) {
		unsigned char x;
		x = fz_arc4next(arc4);
		dest[i] = src[i] ^ x;
	}
}

