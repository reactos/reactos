/***************************************************************************
 *                        lzx.h - LZX decompression routines               *
 *                           -------------------                           *
 *                                                                         *
 *  maintainer: Jed Wing <jedwin@ugcs.caltech.edu>                         *
 *  source:     modified lzx.c from cabextract v0.5                        *
 *  notes:      This file was taken from cabextract v0.5, which was,       *
 *              itself, a modified version of the lzx decompression code   *
 *              from unlzx.                                                *
 ***************************************************************************/

/***************************************************************************
 *
 *   Copyright(C) Stuart Caie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 ***************************************************************************/

#ifndef INCLUDED_LZX_H
#define INCLUDED_LZX_H

/* return codes */
#define DECR_OK           (0)
#define DECR_DATAFORMAT   (1)
#define DECR_ILLEGALDATA  (2)
#define DECR_NOMEMORY     (3)

/* opaque state structure */
struct LZXstate;

/* create an lzx state object */
struct LZXstate *LZXinit(int window);

/* destroy an lzx state object */
void LZXteardown(struct LZXstate *pState);

/* reset an lzx stream */
int LZXreset(struct LZXstate *pState);

/* decompress an LZX compressed block */
int LZXdecompress(struct LZXstate *pState,
                  unsigned char *inpos,
                  unsigned char *outpos,
                  int inlen,
                  int outlen);

#endif /* INCLUDED_LZX_H */
