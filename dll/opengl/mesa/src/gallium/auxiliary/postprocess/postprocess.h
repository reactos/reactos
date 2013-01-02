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

#ifndef POSTPROCESS_H
#define POSTPROCESS_H

#include "postprocess/pp_program.h"

#define PP_FILTERS 6            /* Increment this if you add filters */
#define PP_MAX_PASSES 6

struct pp_queue_t;              /* Forward definition */

/* Less typing later on */
typedef void (*pp_func) (struct pp_queue_t *, struct pipe_resource *,
                         struct pipe_resource *, unsigned int);
/**
*	The main post-processing queue.
*/
struct pp_queue_t
{
   pp_func *pp_queue;           /* An array of pp_funcs */
   unsigned int n_filters;      /* Number of enabled filters */

   struct pipe_resource *tmp[2];        /* Two temp FBOs for the queue */
   struct pipe_resource *inner_tmp[3];  /* Three for filter use */

   unsigned int n_tmp, n_inner_tmp;

   struct pipe_resource *depth; /* depth of original input */
   struct pipe_resource *stencil;       /* stencil shared by inner_tmps */

   struct pipe_surface *tmps[2], *inner_tmps[3], *stencils;

   void ***shaders;             /* Shaders in TGSI form */
   unsigned int *verts;
   struct program *p;

   bool fbos_init;
};

/* Main functions */

struct pp_queue_t *pp_init(struct pipe_screen *, const unsigned int *);
void pp_run(struct pp_queue_t *, struct pipe_resource *,
            struct pipe_resource *, struct pipe_resource *);
void pp_free(struct pp_queue_t *);
void pp_free_fbos(struct pp_queue_t *);
void pp_debug(const char *, ...);
struct program *pp_init_prog(struct pp_queue_t *, struct pipe_screen *);
void pp_init_fbos(struct pp_queue_t *, unsigned int, unsigned int);

/* The filters */

void pp_nocolor(struct pp_queue_t *, struct pipe_resource *,
                struct pipe_resource *, unsigned int);

void pp_jimenezmlaa(struct pp_queue_t *, struct pipe_resource *,
                    struct pipe_resource *, unsigned int);
void pp_jimenezmlaa_color(struct pp_queue_t *, struct pipe_resource *,
                          struct pipe_resource *, unsigned int);

/* The filter init functions */

void pp_celshade_init(struct pp_queue_t *, unsigned int, unsigned int);

void pp_nored_init(struct pp_queue_t *, unsigned int, unsigned int);
void pp_nogreen_init(struct pp_queue_t *, unsigned int, unsigned int);
void pp_noblue_init(struct pp_queue_t *, unsigned int, unsigned int);

void pp_jimenezmlaa_init(struct pp_queue_t *, unsigned int, unsigned int);
void pp_jimenezmlaa_init_color(struct pp_queue_t *, unsigned int,
                               unsigned int);

#endif
