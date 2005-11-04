
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


/* xfonts.c -- glXUseXFont() for Mesa written by
 * Copyright (C) 1995 Thorsten.Ohl @ Physik.TH-Darmstadt.de
 */

#ifdef __VMS
#include <GL/vms_x_fix.h>
#endif

#include "glxheader.h"
#include "context.h"
#include "imports.h"
#include "xfonts.h"


/* Some debugging info.  */

#ifdef DEBUG
#undef _R
#undef _G
#undef _B
#include <ctype.h>

int debug_xfonts = 0;

static void
dump_char_struct(XCharStruct * ch, char *prefix)
{
   printf("%slbearing = %d, rbearing = %d, width = %d\n",
	  prefix, ch->lbearing, ch->rbearing, ch->width);
   printf("%sascent = %d, descent = %d, attributes = %u\n",
	  prefix, ch->ascent, ch->descent, (unsigned int) ch->attributes);
}

static void
dump_font_struct(XFontStruct * font)
{
   printf("ascent = %d, descent = %d\n", font->ascent, font->descent);
   printf("char_or_byte2 = (%u,%u)\n",
	  font->min_char_or_byte2, font->max_char_or_byte2);
   printf("byte1 = (%u,%u)\n", font->min_byte1, font->max_byte1);
   printf("all_chars_exist = %s\n", font->all_chars_exist ? "True" : "False");
   printf("default_char = %c (\\%03o)\n",
	  (char) (isprint(font->default_char) ? font->default_char : ' '),
	  font->default_char);
   dump_char_struct(&font->min_bounds, "min> ");
   dump_char_struct(&font->max_bounds, "max> ");
#if 0
   for (c = font->min_char_or_byte2; c <= font->max_char_or_byte2; c++) {
      char prefix[8];
      sprintf(prefix, "%d> ", c);
      dump_char_struct(&font->per_char[c], prefix);
   }
#endif
}

static void
dump_bitmap(unsigned int width, unsigned int height, GLubyte * bitmap)
{
   unsigned int x, y;

   printf("    ");
   for (x = 0; x < 8 * width; x++)
      printf("%o", 7 - (x % 8));
   putchar('\n');
   for (y = 0; y < height; y++) {
      printf("%3o:", y);
      for (x = 0; x < 8 * width; x++)
	 putchar((bitmap[width * (height - y - 1) + x / 8] & (1 << (7 - (x %
									 8))))
		 ? '*' : '.');
      printf("   ");
      for (x = 0; x < width; x++)
	 printf("0x%02x, ", bitmap[width * (height - y - 1) + x]);
      putchar('\n');
   }
}
#endif /* DEBUG */


/* Implementation.  */

/* Fill a BITMAP with a character C from thew current font
   in the graphics context GC.  WIDTH is the width in bytes
   and HEIGHT is the height in bits.

   Note that the generated bitmaps must be used with

        glPixelStorei (GL_UNPACK_SWAP_BYTES, GL_FALSE);
        glPixelStorei (GL_UNPACK_LSB_FIRST, GL_FALSE);
        glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei (GL_UNPACK_SKIP_ROWS, 0);
        glPixelStorei (GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

   Possible optimizations:

     * use only one reusable pixmap with the maximum dimensions.
     * draw the entire font into a single pixmap (careful with
       proportional fonts!).
*/


/*
 * Generate OpenGL-compatible bitmap.
 */
static void
fill_bitmap(Display * dpy, Window win, GC gc,
	    unsigned int width, unsigned int height,
	    int x0, int y0, unsigned int c, GLubyte * bitmap)
{
   XImage *image;
   unsigned int x, y;
   Pixmap pixmap;
   XChar2b char2b;

   pixmap = XCreatePixmap(dpy, win, 8 * width, height, 1);
   XSetForeground(dpy, gc, 0);
   XFillRectangle(dpy, pixmap, gc, 0, 0, 8 * width, height);
   XSetForeground(dpy, gc, 1);

   char2b.byte1 = (c >> 8) & 0xff;
   char2b.byte2 = (c & 0xff);

   XDrawString16(dpy, pixmap, gc, x0, y0, &char2b, 1);

   image = XGetImage(dpy, pixmap, 0, 0, 8 * width, height, 1, XYPixmap);
   if (image) {
      /* Fill the bitmap (X11 and OpenGL are upside down wrt each other).  */
      for (y = 0; y < height; y++)
	 for (x = 0; x < 8 * width; x++)
	    if (XGetPixel(image, x, y))
	       bitmap[width * (height - y - 1) + x / 8] |=
		  (1 << (7 - (x % 8)));
      XDestroyImage(image);
   }

   XFreePixmap(dpy, pixmap);
}

/*
 * determine if a given glyph is valid and return the
 * corresponding XCharStruct.
 */
static XCharStruct *
isvalid(XFontStruct * fs, unsigned int which)
{
   unsigned int rows, pages;
   unsigned int byte1 = 0, byte2 = 0;
   int i, valid = 1;

   rows = fs->max_byte1 - fs->min_byte1 + 1;
   pages = fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1;

   if (rows == 1) {
      /* "linear" fonts */
      if ((fs->min_char_or_byte2 > which) || (fs->max_char_or_byte2 < which))
	 valid = 0;
   }
   else {
      /* "matrix" fonts */
      byte2 = which & 0xff;
      byte1 = which >> 8;
      if ((fs->min_char_or_byte2 > byte2) ||
	  (fs->max_char_or_byte2 < byte2) ||
	  (fs->min_byte1 > byte1) || (fs->max_byte1 < byte1))
	 valid = 0;
   }

   if (valid) {
      if (fs->per_char) {
	 if (rows == 1) {
	    /* "linear" fonts */
	    return (fs->per_char + (which - fs->min_char_or_byte2));
	 }
	 else {
	    /* "matrix" fonts */
	    i = ((byte1 - fs->min_byte1) * pages) +
	       (byte2 - fs->min_char_or_byte2);
	    return (fs->per_char + i);
	 }
      }
      else {
	 return (&fs->min_bounds);
      }
   }
   return (NULL);
}


void
Fake_glXUseXFont(Font font, int first, int count, int listbase)
{
   Display *dpy;
   Window win;
   Pixmap pixmap;
   GC gc;
   XGCValues values;
   unsigned long valuemask;
   XFontStruct *fs;
   GLint swapbytes, lsbfirst, rowlength;
   GLint skiprows, skippixels, alignment;
   unsigned int max_width, max_height, max_bm_width, max_bm_height;
   GLubyte *bm;
   int i;

   dpy = glXGetCurrentDisplay();
   if (!dpy)
      return;			/* I guess glXMakeCurrent wasn't called */
   win = RootWindow(dpy, DefaultScreen(dpy));

   fs = XQueryFont(dpy, font);
   if (!fs) {
      _mesa_error(NULL, GL_INVALID_VALUE,
		  "Couldn't get font structure information");
      return;
   }

   /* Allocate a bitmap that can fit all characters.  */
   max_width = fs->max_bounds.rbearing - fs->min_bounds.lbearing;
   max_height = fs->max_bounds.ascent + fs->max_bounds.descent;
   max_bm_width = (max_width + 7) / 8;
   max_bm_height = max_height;

   bm = (GLubyte *) MALLOC((max_bm_width * max_bm_height) * sizeof(GLubyte));
   if (!bm) {
      XFreeFontInfo(NULL, fs, 1);
      _mesa_error(NULL, GL_OUT_OF_MEMORY,
		  "Couldn't allocate bitmap in glXUseXFont()");
      return;
   }

#if 0
   /* get the page info */
   pages = fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1;
   firstchar = (fs->min_byte1 << 8) + fs->min_char_or_byte2;
   lastchar = (fs->max_byte1 << 8) + fs->max_char_or_byte2;
   rows = fs->max_byte1 - fs->min_byte1 + 1;
   unsigned int first_char, last_char, pages, rows;
#endif

   /* Save the current packing mode for bitmaps.  */
   glGetIntegerv(GL_UNPACK_SWAP_BYTES, &swapbytes);
   glGetIntegerv(GL_UNPACK_LSB_FIRST, &lsbfirst);
   glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowlength);
   glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skiprows);
   glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skippixels);
   glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);

   /* Enforce a standard packing mode which is compatible with
      fill_bitmap() from above.  This is actually the default mode,
      except for the (non)alignment.  */
   glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
   glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
   glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   pixmap = XCreatePixmap(dpy, win, 10, 10, 1);
   values.foreground = BlackPixel(dpy, DefaultScreen(dpy));
   values.background = WhitePixel(dpy, DefaultScreen(dpy));
   values.font = fs->fid;
   valuemask = GCForeground | GCBackground | GCFont;
   gc = XCreateGC(dpy, pixmap, valuemask, &values);
   XFreePixmap(dpy, pixmap);

#ifdef DEBUG
   if (debug_xfonts)
      dump_font_struct(fs);
#endif

   for (i = 0; i < count; i++) {
      unsigned int width, height, bm_width, bm_height;
      GLfloat x0, y0, dx, dy;
      XCharStruct *ch;
      int x, y;
      unsigned int c = first + i;
      int list = listbase + i;
      int valid;

      /* check on index validity and get the bounds */
      ch = isvalid(fs, c);
      if (!ch) {
	 ch = &fs->max_bounds;
	 valid = 0;
      }
      else {
	 valid = 1;
      }

#ifdef DEBUG
      if (debug_xfonts) {
	 char s[7];
	 sprintf(s, isprint(c) ? "%c> " : "\\%03o> ", c);
	 dump_char_struct(ch, s);
      }
#endif

      /* glBitmap()' parameters:
         straight from the glXUseXFont(3) manpage.  */
      width = ch->rbearing - ch->lbearing;
      height = ch->ascent + ch->descent;
      x0 = -ch->lbearing;
      y0 = ch->descent - 0;	/* XXX used to subtract 1 here */
      /* but that caused a conformace failure */
      dx = ch->width;
      dy = 0;

      /* X11's starting point.  */
      x = -ch->lbearing;
      y = ch->ascent;

      /* Round the width to a multiple of eight.  We will use this also
         for the pixmap for capturing the X11 font.  This is slightly
         inefficient, but it makes the OpenGL part real easy.  */
      bm_width = (width + 7) / 8;
      bm_height = height;

      glNewList(list, GL_COMPILE);
      if (valid && (bm_width > 0) && (bm_height > 0)) {

	 MEMSET(bm, '\0', bm_width * bm_height);
	 fill_bitmap(dpy, win, gc, bm_width, bm_height, x, y, c, bm);

	 glBitmap(width, height, x0, y0, dx, dy, bm);
#ifdef DEBUG
	 if (debug_xfonts) {
	    printf("width/height = %u/%u\n", width, height);
	    printf("bm_width/bm_height = %u/%u\n", bm_width, bm_height);
	    dump_bitmap(bm_width, bm_height, bm);
	 }
#endif
      }
      else {
	 glBitmap(0, 0, 0.0, 0.0, dx, dy, NULL);
      }
      glEndList();
   }

   FREE(bm);
   XFreeFontInfo(NULL, fs, 1);
   XFreeGC(dpy, gc);

   /* Restore saved packing modes.  */
   glPixelStorei(GL_UNPACK_SWAP_BYTES, swapbytes);
   glPixelStorei(GL_UNPACK_LSB_FIRST, lsbfirst);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, rowlength);
   glPixelStorei(GL_UNPACK_SKIP_ROWS, skiprows);
   glPixelStorei(GL_UNPACK_SKIP_PIXELS, skippixels);
   glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
}
