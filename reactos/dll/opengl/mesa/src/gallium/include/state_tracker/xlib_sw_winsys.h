#ifndef XLIB_SW_WINSYS_H
#define XLIB_SW_WINSYS_H

#include "state_tracker/sw_winsys.h"
#include <X11/Xlib.h>


struct pipe_screen;
struct pipe_resource;

/* This is what the xlib software winsys expects to find in the
 * "private" field of flush_frontbuffers().
 *
 * Xlib-based state trackers somehow need to know this.
 */
struct xlib_drawable {
   Visual *visual;
   int depth;
   Drawable drawable;
};


/* This is the public interface to the ws/xlib module.  Why isn't it
 * being defined in that directory?
 */
struct sw_winsys *xlib_create_sw_winsys( Display *display );


#endif
