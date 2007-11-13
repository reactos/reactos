/*
 * Mesa 3-D graphics library
 * Version:  4.0
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * DOS/DJGPP device driver for Mesa
 *
 *  Author: Daniel Borca
 *  Email : dborca@users.sourceforge.net
 *  Web   : http://www.geocities.com/dborca
 */


#ifndef INTERNAL_H_included
#define INTERNAL_H_included

#include "../main/mtypes.h"


/*
 * general purpose defines, etc.
 */
#ifndef FALSE
#define FALSE 0
#define TRUE !FALSE
#endif

#define __PACKED__ __attribute__((packed))

typedef unsigned char word8;
typedef unsigned short word16;
typedef unsigned long word32;

#define _16_ *(word16 *)&
#define _32_ *(word32 *)&

typedef void (*BLTFUNC) (void);


/*
 * video mode structure
 */
typedef struct vl_mode {
   int xres, yres;
   int bpp;

   int mode;
   int scanlen;

   int sel;
   int gran;
} vl_mode;


/*
 * video driver structure
 */
typedef struct {
   vl_mode *(*init) (void);
   int (*entermode) (vl_mode *p, int refresh, int fbbits);
   void (*blit) (void);
   void (*setCI_f) (int index, float red, float green, float blue);
   void (*setCI_i) (int index, int red, int green, int blue);
   int (*get) (int pname, int *params);
   void (*restore) (void);
   void (*fini) (void);
} vl_driver;


/*
 * memory mapping
 */
int _create_linear_mapping (unsigned long *linear, unsigned long physaddr, int size);
void _remove_linear_mapping (unsigned long *linear);
int _create_selector (int *segment, unsigned long base, int size);
void _remove_selector (int *segment);


/*
 * system routines
 */
int _can_mmx (void);


#endif
