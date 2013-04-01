/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef GALLIUM_RAW_H
#define GALLIUM_RAW_H

/* This is an API for exercising gallium functionality in a
 * platform-neutral fashion.  Whatever platform integration is
 * necessary to implement this interface is orchestrated by the
 * individual target building this entity.
 *
 * For instance, the graw-xlib target includes code to implent these
 * interfaces on top of the X window system.
 *
 * Programs using this interface may additionally benefit from some of
 * the utilities currently in the libgallium.a library, especially
 * those for parsing text representations of TGSI shaders.
 */

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"

struct pipe_context;
struct pipe_screen;
struct pipe_surface;

/* Returns a handle to be used with flush_frontbuffer()/present().
 *
 * Query format support with screen::is_format_supported and usage
 * XXX.
 */
PUBLIC struct pipe_screen *graw_create_window_and_screen( int x,
                                                          int y,
                                                          unsigned width,
                                                          unsigned height,
                                                          enum pipe_format format,
                                                          void **handle);

PUBLIC void graw_set_display_func( void (*func)( void ) );
PUBLIC void graw_main_loop( void );

PUBLIC void *graw_parse_geometry_shader( struct pipe_context *pipe,
                                         const char *text );

PUBLIC void *graw_parse_vertex_shader( struct pipe_context *pipe,
                                       const char *text );

PUBLIC void *graw_parse_fragment_shader( struct pipe_context *pipe,
                                         const char *text );

/* Parse a single command-line option, if any. Options include:
 *
 * -o <filename>
 *
 * If an option has been successfully parsed, argi is updated
 * to point just after the option and return TRUE.
 */
PUBLIC boolean graw_parse_args(int *argi, int argc, char *argv[]);

/* Saves surface contents to a file.
 *
 * If filename is NULL, the filename provided with the `-o' option
 * is used. If the option has not been specified, the surface
 * will not be saved.
 *
 * Returns TRUE if the surface has been saved.
 */
PUBLIC boolean graw_save_surface_to_file(struct pipe_context *pipe,
                                         struct pipe_surface *surface,
                                         const char *filename);

#endif
