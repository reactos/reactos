#ifndef _DRISW_API_H_
#define _DRISW_API_H_

#include "pipe/p_compiler.h"

struct pipe_screen;
struct dri_drawable;

/**
 * This callback struct is intended for the winsys to call the loader.
 */
struct drisw_loader_funcs
{
   void (*put_image) (struct dri_drawable *dri_drawable,
                      void *data, unsigned width, unsigned height);
};

/**
 * Implemented by the drisw target.
 */
struct pipe_screen * drisw_create_screen(struct drisw_loader_funcs *lf);

#endif
