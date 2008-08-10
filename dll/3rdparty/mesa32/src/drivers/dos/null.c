/*
 * Mesa 3-D graphics library
 * Version:  4.1
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


#include <stdlib.h>
#include <sys/segments.h>

#include "video.h"
#include "null.h"


static vl_mode *modes;

#define null_color_precision 8


static void
null_blit_nop (void)
{
}


/* Desc: Attempts to detect NUL, check video modes and create selectors.
 *
 * In  : -
 * Out : mode array
 *
 * Note: -
 */
static vl_mode *
null_init (void)
{
   static int m[][2] = {
      {  320,  200 },
      {  320,  240 },
      {  400,  300 },
      {  512,  384 },
      {  640,  400 },
      {  640,  480 },
      {  800,  600 },
      { 1024,  768 },
      { 1280, 1024 },
      { 1600, 1200 }
   };
   static int b[] = {
      8,
      15,
      16,
      24,
      32
   };
   const unsigned int m_count = sizeof(m) / sizeof(m[0]);
   const unsigned int b_count = sizeof(b) / sizeof(b[0]);

   unsigned int i, j, k;

   if (modes == NULL) {
      modes = malloc(sizeof(vl_mode) * (1 + m_count * b_count));

      if (modes != NULL) {
         for (k = 0, i = 0; i < m_count; i++) {
            for (j = 0; j < b_count; j++, k++) {
               modes[k].xres    = m[i][0];
               modes[k].yres    = m[i][1];
               modes[k].bpp     = b[j];
               modes[k].mode    = 0x4000;
               modes[k].scanlen = m[i][0] * ((b[j] + 7) / 8);
               modes[k].sel     = -1;
               modes[k].gran    = -1;
            }
         }
         modes[k].xres    = -1;
         modes[k].yres    = -1;
         modes[k].bpp     = -1;
         modes[k].mode    = 0xffff;
         modes[k].scanlen = -1;
         modes[k].sel     = -1;
         modes[k].gran    = -1;
      }
   }

   return modes;
}


/* Desc: Frees all resources allocated by NUL init code.
 *
 * In  : -
 * Out : -
 *
 * Note: -
 */
static void
null_fini (void)
{
   if (modes != NULL) {
      free(modes);
      modes = NULL;
   }
}


/* Desc: Attempts to enter specified video mode.
 *
 * In  : ptr to mode structure, refresh rate
 * Out : 0 if success
 *
 * Note: -
 */
static int
null_entermode (vl_mode *p, int refresh, int fbbits)
{
   NUL.blit = null_blit_nop;

   return 0;

   (void)(p && refresh && fbbits); /* silence compiler warning */
}


/* Desc: Restores to the mode prior to first call to null_entermode.
 *
 * In  : -
 * Out : -
 *
 * Note: -
 */
static void
null_restore (void)
{
}


/* Desc: set one palette entry
 *
 * In  : color index, R, G, B
 * Out : -
 *
 * Note: uses integer values
 */
static void
null_setCI_i (int index, int red, int green, int blue)
{
   (void)(index && red && green && blue); /* silence compiler warning */
}


/* Desc: set one palette entry
 *
 * In  : color index, R, G, B
 * Out : -
 *
 * Note: uses normalized values
 */
static void
null_setCI_f (int index, float red, float green, float blue)
{
   float max = (1 << null_color_precision) - 1;

   null_setCI_i(index, (int)(red * max), (int)(green * max), (int)(blue * max));
}


/* Desc: state retrieval
 *
 * In  : parameter name, ptr to storage
 * Out : 0 if request successfully processed
 *
 * Note: -
 */
static int
null_get (int pname, int *params)
{
   switch (pname) {
      default:
         params[0] = params[0]; /* silence compiler warning */
         return -1;
   }
   return 0;
}


/*
 * the driver
 */
vl_driver NUL = {
   null_init,
   null_entermode,
   NULL,
   null_setCI_f,
   null_setCI_i,
   null_get,
   null_restore,
   null_fini
};
