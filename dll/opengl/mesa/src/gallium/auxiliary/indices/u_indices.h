/*
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef U_INDICES_H
#define U_INDICES_H

#include "pipe/p_compiler.h"

#define PV_FIRST      0
#define PV_LAST       1
#define PV_COUNT      2

typedef void (*u_translate_func)( const void *in,
                                  unsigned nr,
                                  void *out );

typedef void (*u_generate_func)( unsigned nr,
                                 void *out );


/* Return codes describe the translate/generate operation.  Caller may
 * be able to reuse translated indices under some circumstances.
 */
#define U_TRANSLATE_ERROR  -1
#define U_TRANSLATE_NORMAL  1
#define U_TRANSLATE_MEMCPY  2
#define U_GENERATE_LINEAR   3
#define U_GENERATE_REUSABLE 4
#define U_GENERATE_ONE_OFF  5


void u_index_init( void );

int u_index_translator( unsigned hw_mask,
                        unsigned prim,
                        unsigned in_index_size,
                        unsigned nr,
                        unsigned in_pv,   /* API */
                        unsigned out_pv,  /* hardware */
                        unsigned *out_prim,
                        unsigned *out_index_size,
                        unsigned *out_nr,
                        u_translate_func *out_translate );

/* Note that even when generating it is necessary to know what the
 * API's PV is, as the indices generated will depend on whether it is
 * the same as hardware or not, and in the case of triangle strips,
 * whether it is first or last.
 */
int u_index_generator( unsigned hw_mask,
                       unsigned prim,
                       unsigned start,
                       unsigned nr,
                       unsigned in_pv,   /* API */
                       unsigned out_pv,  /* hardware */
                       unsigned *out_prim,
                       unsigned *out_index_size,
                       unsigned *out_nr,
                       u_generate_func *out_generate );


void u_unfilled_init( void );

int u_unfilled_translator( unsigned prim,
                           unsigned in_index_size,
                           unsigned nr,
                           unsigned unfilled_mode,
                           unsigned *out_prim,
                           unsigned *out_index_size,
                           unsigned *out_nr,
                           u_translate_func *out_translate );

int u_unfilled_generator( unsigned prim,
                          unsigned start,
                          unsigned nr,
                          unsigned unfilled_mode,
                          unsigned *out_prim,
                          unsigned *out_index_size,
                          unsigned *out_nr,
                          u_generate_func *out_generate );




#endif
