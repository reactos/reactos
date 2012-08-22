/**************************************************************************
 *
 * Copyright 2011 Lauri Kasanen
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
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef PP_EXTERNAL_FILTERS_H
#define PP_EXTERNAL_FILTERS_H

#include "postprocess/postprocess.h"

typedef void (*pp_init_func) (struct pp_queue_t *, unsigned int,
                              unsigned int);

struct pp_filter_t
{
   const char *name;            /* Config name */
   unsigned int inner_tmps;     /* Request how many inner temps */
   unsigned int shaders;        /* Request how many shaders */
   unsigned int verts;          /* How many are vertex shaders */
   pp_init_func init;           /* Init function */
   pp_func main;                /* Run function */
};

/*	Order matters. Put new filters in a suitable place. */

static const struct pp_filter_t pp_filters[PP_FILTERS] = {
/*    name			inner	shaders	verts	init			run */
   { "pp_noblue",		0,	2,	1,	pp_noblue_init,		pp_nocolor },
   { "pp_nogreen",		0,	2,	1,	pp_nogreen_init,	pp_nocolor },
   { "pp_nored",		0,	2,	1,	pp_nored_init,		pp_nocolor },
   { "pp_celshade",		0,	2,	1,	pp_celshade_init,	pp_nocolor },
   { "pp_jimenezmlaa",		2,	5,	2,	pp_jimenezmlaa_init,	pp_jimenezmlaa },
   { "pp_jimenezmlaa_color",	2,	5,	2,	pp_jimenezmlaa_init_color, pp_jimenezmlaa_color },
};

#endif
