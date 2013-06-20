/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/



#include "util/u_math.h"


/** 2^x, for x in [-1.0, 1.0) */
float pow2_table[POW2_TABLE_SIZE];


static void
init_pow2_table(void)
{
   int i;
   for (i = 0; i < POW2_TABLE_SIZE; i++)
      pow2_table[i] = (float) pow(2.0, (i - POW2_TABLE_OFFSET) / POW2_TABLE_SCALE);
}


/** log2(x), for x in [1.0, 2.0) */
float log2_table[LOG2_TABLE_SIZE];


static void 
init_log2_table(void)
{
   unsigned i;
   for (i = 0; i < LOG2_TABLE_SIZE; i++)
      log2_table[i] = (float) log2(1.0 + i * (1.0 / LOG2_TABLE_SCALE));
}


/**
 * One time init for math utilities.
 */
void
util_init_math(void)
{
   static boolean initialized = FALSE;
   if (!initialized) {
      init_pow2_table();
      init_log2_table();
      initialized = TRUE;
   }
}


