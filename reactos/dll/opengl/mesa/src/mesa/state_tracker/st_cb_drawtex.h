/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 **************************************************************************/


#ifndef ST_CB_DRAWTEX_H
#define ST_CB_DRAWTEX_H


#include "main/compiler.h"
#include "main/mfeatures.h"

struct dd_function_table;
struct st_context;

#if FEATURE_OES_draw_texture

extern void
st_init_drawtex_functions(struct dd_function_table *functions);

extern void
st_destroy_drawtex(struct st_context *st);

#else

static INLINE void
st_init_drawtex_functions(struct dd_function_table *functions)
{
}

static INLINE void
st_destroy_drawtex(struct st_context *st)
{
}

#endif /* FEATURE_OES_draw_texture */

#endif /* ST_CB_DRAWTEX_H */
