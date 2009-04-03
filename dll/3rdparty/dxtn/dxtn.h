/*
 * DXTn codec
 * Version:  1.1
 *
 * Copyright (C) 2004  Daniel Borca   All Rights Reserved.
 *
 * this is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * this is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.	
 */


#ifndef DXTN_H_included
#define DXTN_H_included

TAPI int TAPIENTRY
dxt1_rgb_encode (int width, int height, int comps,
		 const void *source, int srcRowStride,
		 void *dest, int destRowStride);

TAPI int TAPIENTRY
dxt1_rgba_encode (int width, int height, int comps,
		  const void *source, int srcRowStride,
		  void *dest, int destRowStride);

TAPI int TAPIENTRY
dxt3_rgba_encode (int width, int height, int comps,
		  const void *source, int srcRowStride,
		  void *dest, int destRowStride);

TAPI int TAPIENTRY
dxt5_rgba_encode (int width, int height, int comps,
		  const void *source, int srcRowStride,
		  void *dest, int destRowStride);

TAPI void TAPIENTRY
dxt1_rgb_decode_1 (const void *texture, int stride /* in pixels */,
		   int i, int j, byte *rgba);

TAPI void TAPIENTRY
dxt1_rgba_decode_1 (const void *texture, int stride /* in pixels */,
		    int i, int j, byte *rgba);

TAPI void TAPIENTRY
dxt3_rgba_decode_1 (const void *texture, int stride /* in pixels */,
		    int i, int j, byte *rgba);

TAPI void TAPIENTRY
dxt5_rgba_decode_1 (const void *texture, int stride /* in pixels */,
		    int i, int j, byte *rgba);

#endif
