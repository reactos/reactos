/*
 * TGA file handling
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


#ifndef TGA_H_included
#define TGA_H_included

#define TGA_OK          0
#define TGA_ERR_MEM    -2
#define TGA_ERR_CREATE -3
#define TGA_ERR_OPEN   -4
#define TGA_ERR_READ   -5
#define TGA_ERR_WRITE  -6
#define TGA_ERR_FORMAT -7

int tga_read_32 (const char *filename, int *w, int *h, void **p);
int tga_write (const char *filename, int width, int height, void *data, int bpp);

#endif
