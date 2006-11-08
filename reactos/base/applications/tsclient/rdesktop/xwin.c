/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   User interface services - X Window System
   Copyright (C) Matthew Chapman 1999-2005

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <strings.h>
#include "rdesktop.h"
#include "xproto.h"

/* We can't include Xproto.h because of conflicting defines for BOOL */
#define X_ConfigureWindow              12

/* MWM decorations */
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define PROP_MOTIF_WM_HINTS_ELEMENTS    5
typedef struct
{
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long inputMode;
	unsigned long status;
}
PropMotifWmHints;

typedef struct
{
	uint32 red;
	uint32 green;
	uint32 blue;
}
PixelColour;

#define ON_ALL_SEAMLESS_WINDOWS(func, args) \
        do { \
                seamless_window *sw; \
                XRectangle rect; \
		if (!This->xwin.seamless_windows) break; \
                for (sw = This->xwin.seamless_windows; sw; sw = sw->next) { \
                    rect.x = This->xwin.clip_rectangle.x - sw->xoffset; \
                    rect.y = This->xwin.clip_rectangle.y - sw->yoffset; \
                    rect.width = This->xwin.clip_rectangle.width; \
                    rect.height = This->xwin.clip_rectangle.height; \
                    XSetClipRectangles(This->display, This->xwin.gc, 0, 0, &rect, 1, YXBanded); \
                    func args; \
                } \
                XSetClipRectangles(This->display, This->xwin.gc, 0, 0, &This->xwin.clip_rectangle, 1, YXBanded); \
        } while (0)

static void
seamless_XFillPolygon(RDPCLIENT * This, Drawable d, XPoint * points, int npoints, int xoffset, int yoffset)
{
	points[0].x -= xoffset;
	points[0].y -= yoffset;
	XFillPolygon(This->display, d, This->xwin.gc, points, npoints, Complex, CoordModePrevious);
	points[0].x += xoffset;
	points[0].y += yoffset;
}

static void
seamless_XDrawLines(RDPCLIENT * This, Drawable d, XPoint * points, int npoints, int xoffset, int yoffset)
{
	points[0].x -= xoffset;
	points[0].y -= yoffset;
	XDrawLines(This->display, d, This->xwin.gc, points, npoints, CoordModePrevious);
	points[0].x += xoffset;
	points[0].y += yoffset;
}

#define FILL_RECTANGLE(x,y,cx,cy)\
{ \
	XFillRectangle(This->display, This->wnd, This->xwin.gc, x, y, cx, cy); \
        ON_ALL_SEAMLESS_WINDOWS(XFillRectangle, (This->display, sw->wnd, This->xwin.gc, x-sw->xoffset, y-sw->yoffset, cx, cy)); \
	if (This->ownbackstore) \
		XFillRectangle(This->display, This->xwin.backstore, This->xwin.gc, x, y, cx, cy); \
}

#define FILL_RECTANGLE_BACKSTORE(x,y,cx,cy)\
{ \
	XFillRectangle(This->display, This->ownbackstore ? This->xwin.backstore : This->wnd, This->xwin.gc, x, y, cx, cy); \
}

#define FILL_POLYGON(p,np)\
{ \
	XFillPolygon(This->display, This->wnd, This->xwin.gc, p, np, Complex, CoordModePrevious); \
	if (This->ownbackstore) \
		XFillPolygon(This->display, This->xwin.backstore, This->xwin.gc, p, np, Complex, CoordModePrevious); \
	ON_ALL_SEAMLESS_WINDOWS(seamless_XFillPolygon, (This, sw->wnd, p, np, sw->xoffset, sw->yoffset)); \
}

#define DRAW_ELLIPSE(x,y,cx,cy,m)\
{ \
	switch (m) \
	{ \
		case 0:	/* Outline */ \
			XDrawArc(This->display, This->wnd, This->xwin.gc, x, y, cx, cy, 0, 360*64); \
                        ON_ALL_SEAMLESS_WINDOWS(XDrawArc, (This->display, sw->wnd, This->xwin.gc, x-sw->xoffset, y-sw->yoffset, cx, cy, 0, 360*64)); \
			if (This->ownbackstore) \
				XDrawArc(This->display, This->xwin.backstore, This->xwin.gc, x, y, cx, cy, 0, 360*64); \
			break; \
		case 1: /* Filled */ \
			XFillArc(This->display, This->wnd, This->xwin.gc, x, y, cx, cy, 0, 360*64); \
                        ON_ALL_SEAMLESS_WINDOWS(XCopyArea, (This->display, This->ownbackstore ? This->xwin.backstore : This->wnd, sw->wnd, This->xwin.gc, \
							    x, y, cx, cy, x-sw->xoffset, y-sw->yoffset)); \
			if (This->ownbackstore) \
				XFillArc(This->display, This->xwin.backstore, This->xwin.gc, x, y, cx, cy, 0, 360*64); \
			break; \
	} \
}

#define TRANSLATE(col)		( This->server_depth != 8 ? translate_colour(This, col) : This->owncolmap ? col : This->xwin.colmap[col] )
#define SET_FOREGROUND(col)	XSetForeground(This->display, This->xwin.gc, TRANSLATE(col));
#define SET_BACKGROUND(col)	XSetBackground(This->display, This->xwin.gc, TRANSLATE(col));

static const int rop2_map[] = {
	GXclear,		/* 0 */
	GXnor,			/* DPon */
	GXandInverted,		/* DPna */
	GXcopyInverted,		/* Pn */
	GXandReverse,		/* PDna */
	GXinvert,		/* Dn */
	GXxor,			/* DPx */
	GXnand,			/* DPan */
	GXand,			/* DPa */
	GXequiv,		/* DPxn */
	GXnoop,			/* D */
	GXorInverted,		/* DPno */
	GXcopy,			/* P */
	GXorReverse,		/* PDno */
	GXor,			/* DPo */
	GXset			/* 1 */
};

#define SET_FUNCTION(rop2)	{ if (rop2 != ROP2_COPY) XSetFunction(This->display, This->xwin.gc, rop2_map[rop2]); }
#define RESET_FUNCTION(rop2)	{ if (rop2 != ROP2_COPY) XSetFunction(This->display, This->xwin.gc, GXcopy); }

static seamless_window *
sw_get_window_by_id(RDPCLIENT * This, unsigned long id)
{
	seamless_window *sw;
	for (sw = This->xwin.seamless_windows; sw; sw = sw->next)
	{
		if (sw->id == id)
			return sw;
	}
	return NULL;
}


static seamless_window *
sw_get_window_by_wnd(RDPCLIENT * This, Window wnd)
{
	seamless_window *sw;
	for (sw = This->xwin.seamless_windows; sw; sw = sw->next)
	{
		if (sw->wnd == wnd)
			return sw;
	}
	return NULL;
}


static void
sw_remove_window(RDPCLIENT * This, seamless_window * win)
{
	seamless_window *sw, **prevnext = &This->xwin.seamless_windows;
	for (sw = This->xwin.seamless_windows; sw; sw = sw->next)
	{
		if (sw == win)
		{
			*prevnext = sw->next;
			sw->group->refcnt--;
			if (sw->group->refcnt == 0)
			{
				XDestroyWindow(This->display, sw->group->wnd);
				xfree(sw->group);
			}
			xfree(sw->position_timer);
			xfree(sw);
			return;
		}
		prevnext = &sw->next;
	}
	return;
}


/* Move all windows except wnd to new desktop */
static void
sw_all_to_desktop(RDPCLIENT * This, Window wnd, unsigned int desktop)
{
	seamless_window *sw;
	for (sw = This->xwin.seamless_windows; sw; sw = sw->next)
	{
		if (sw->wnd == wnd)
			continue;
		if (sw->desktop != desktop)
		{
			ewmh_move_to_desktop(This, sw->wnd, desktop);
			sw->desktop = desktop;
		}
	}
}


/* Send our position */
static void
sw_update_position(RDPCLIENT * This, seamless_window * sw)
{
	XWindowAttributes wa;
	int x, y;
	Window child_return;
	unsigned int serial;

	XGetWindowAttributes(This->display, sw->wnd, &wa);
	XTranslateCoordinates(This->display, sw->wnd, wa.root,
			      -wa.border_width, -wa.border_width, &x, &y, &child_return);

	serial = seamless_send_position(This, sw->id, x, y, wa.width, wa.height, 0);

	sw->outstanding_position = True;
	sw->outpos_serial = serial;

	sw->outpos_xoffset = x;
	sw->outpos_yoffset = y;
	sw->outpos_width = wa.width;
	sw->outpos_height = wa.height;
}


/* Check if it's time to send our position */
static void
sw_check_timers(RDPCLIENT * This)
{
	seamless_window *sw;
	struct timeval now;

	gettimeofday(&now, NULL);
	for (sw = This->xwin.seamless_windows; sw; sw = sw->next)
	{
		if (timerisset(sw->position_timer) && timercmp(sw->position_timer, &now, <))
		{
			timerclear(sw->position_timer);
			sw_update_position(This, sw);
		}
	}
}


static void
sw_restack_window(RDPCLIENT * This, seamless_window * sw, unsigned long behind)
{
	seamless_window *sw_above;

	/* Remove window from stack */
	for (sw_above = This->xwin.seamless_windows; sw_above; sw_above = sw_above->next)
	{
		if (sw_above->behind == sw->id)
			break;
	}

	if (sw_above)
		sw_above->behind = sw->behind;

	/* And then add it at the new position */
	for (sw_above = This->xwin.seamless_windows; sw_above; sw_above = sw_above->next)
	{
		if (sw_above->behind == behind)
			break;
	}

	if (sw_above)
		sw_above->behind = sw->id;

	sw->behind = behind;
}


static void
sw_handle_restack(RDPCLIENT * This, seamless_window * sw)
{
	Status status;
	Window root, parent, *children;
	unsigned int nchildren, i;
	seamless_window *sw_below;

	status = XQueryTree(This->display, RootWindowOfScreen(This->xwin.screen),
			    &root, &parent, &children, &nchildren);
	if (!status || !nchildren)
		return;

	sw_below = NULL;

	i = 0;
	while (children[i] != sw->wnd)
	{
		i++;
		if (i >= nchildren)
			goto end;
	}

	for (i++; i < nchildren; i++)
	{
		sw_below = sw_get_window_by_wnd(This, children[i]);
		if (sw_below)
			break;
	}

	if (!sw_below && !sw->behind)
		goto end;
	if (sw_below && (sw_below->id == sw->behind))
		goto end;

	if (sw_below)
	{
		seamless_send_zchange(This, sw->id, sw_below->id, 0);
		sw_restack_window(This, sw, sw_below->id);
	}
	else
	{
		seamless_send_zchange(This, sw->id, 0, 0);
		sw_restack_window(This, sw, 0);
	}

      end:
	XFree(children);
}


static seamless_group *
sw_find_group(RDPCLIENT * This, unsigned long id, BOOL dont_create)
{
	seamless_window *sw;
	seamless_group *sg;
	XSetWindowAttributes attribs;

	for (sw = This->xwin.seamless_windows; sw; sw = sw->next)
	{
		if (sw->group->id == id)
			return sw->group;
	}

	if (dont_create)
		return NULL;

	sg = xmalloc(sizeof(seamless_group));

	sg->wnd =
		XCreateWindow(This->display, RootWindowOfScreen(This->xwin.screen), -1, -1, 1, 1, 0,
			      CopyFromParent, CopyFromParent, CopyFromParent, 0, &attribs);

	sg->id = id;
	sg->refcnt = 0;

	return sg;
}


static void
mwm_hide_decorations(RDPCLIENT * This, Window wnd)
{
	PropMotifWmHints motif_hints;
	Atom hintsatom;

	/* setup the property */
	motif_hints.flags = MWM_HINTS_DECORATIONS;
	motif_hints.decorations = 0;

	/* get the atom for the property */
	hintsatom = XInternAtom(This->display, "_MOTIF_WM_HINTS", False);
	if (!hintsatom)
	{
		warning("Failed to get atom _MOTIF_WM_HINTS: probably your window manager does not support MWM hints\n");
		return;
	}

	XChangeProperty(This->display, wnd, hintsatom, hintsatom, 32, PropModeReplace,
			(unsigned char *) &motif_hints, PROP_MOTIF_WM_HINTS_ELEMENTS);

}

#define SPLITCOLOUR15(colour, rv) \
{ \
	rv.red = ((colour >> 7) & 0xf8) | ((colour >> 12) & 0x7); \
	rv.green = ((colour >> 2) & 0xf8) | ((colour >> 8) & 0x7); \
	rv.blue = ((colour << 3) & 0xf8) | ((colour >> 2) & 0x7); \
}

#define SPLITCOLOUR16(colour, rv) \
{ \
	rv.red = ((colour >> 8) & 0xf8) | ((colour >> 13) & 0x7); \
	rv.green = ((colour >> 3) & 0xfc) | ((colour >> 9) & 0x3); \
	rv.blue = ((colour << 3) & 0xf8) | ((colour >> 2) & 0x7); \
} \

#define SPLITCOLOUR24(colour, rv) \
{ \
	rv.blue = (colour & 0xff0000) >> 16; \
	rv.green = (colour & 0x00ff00) >> 8; \
	rv.red = (colour & 0x0000ff); \
}

#define MAKECOLOUR(pc) \
	((pc.red >> This->xwin.red_shift_r) << This->xwin.red_shift_l) \
		| ((pc.green >> This->xwin.green_shift_r) << This->xwin.green_shift_l) \
		| ((pc.blue >> This->xwin.blue_shift_r) << This->xwin.blue_shift_l) \

#define BSWAP16(x) { x = (((x & 0xff) << 8) | (x >> 8)); }
#define BSWAP24(x) { x = (((x & 0xff) << 16) | (x >> 16) | (x & 0xff00)); }
#define BSWAP32(x) { x = (((x & 0xff00ff) << 8) | ((x >> 8) & 0xff00ff)); \
			x = (x << 16) | (x >> 16); }

/* The following macros output the same octet sequences
   on both BE and LE hosts: */

#define BOUT16(o, x) { *(o++) = x >> 8; *(o++) = x; }
#define BOUT24(o, x) { *(o++) = x >> 16; *(o++) = x >> 8; *(o++) = x; }
#define BOUT32(o, x) { *(o++) = x >> 24; *(o++) = x >> 16; *(o++) = x >> 8; *(o++) = x; }
#define LOUT16(o, x) { *(o++) = x; *(o++) = x >> 8; }
#define LOUT24(o, x) { *(o++) = x; *(o++) = x >> 8; *(o++) = x >> 16; }
#define LOUT32(o, x) { *(o++) = x; *(o++) = x >> 8; *(o++) = x >> 16; *(o++) = x >> 24; }

static uint32
translate_colour(RDPCLIENT * This, uint32 colour)
{
	PixelColour pc;
	switch (This->server_depth)
	{
		case 15:
			SPLITCOLOUR15(colour, pc);
			break;
		case 16:
			SPLITCOLOUR16(colour, pc);
			break;
		case 24:
			SPLITCOLOUR24(colour, pc);
			break;
		default:
			/* Avoid warning */
			pc.red = 0;
			pc.green = 0;
			pc.blue = 0;
			break;
	}
	return MAKECOLOUR(pc);
}

/* indent is confused by UNROLL8 */
/* *INDENT-OFF* */

/* repeat and unroll, similar to bitmap.c */
/* potentialy any of the following translate */
/* functions can use repeat but just doing */
/* the most common ones */

#define UNROLL8(stm) { stm stm stm stm stm stm stm stm }
/* 2 byte output repeat */
#define REPEAT2(stm) \
{ \
	while (out <= end - 8 * 2) \
		UNROLL8(stm) \
	while (out < end) \
		{ stm } \
}
/* 3 byte output repeat */
#define REPEAT3(stm) \
{ \
	while (out <= end - 8 * 3) \
		UNROLL8(stm) \
	while (out < end) \
		{ stm } \
}
/* 4 byte output repeat */
#define REPEAT4(stm) \
{ \
	while (out <= end - 8 * 4) \
		UNROLL8(stm) \
	while (out < end) \
		{ stm } \
}
/* *INDENT-ON* */

static void
translate8to8(RDPCLIENT * This, const uint8 * data, uint8 * out, uint8 * end)
{
	while (out < end)
		*(out++) = (uint8) This->xwin.colmap[*(data++)];
}

static void
translate8to16(RDPCLIENT * This, const uint8 * data, uint8 * out, uint8 * end)
{
	uint16 value;

	if (This->xwin.compatible_arch)
	{
		/* *INDENT-OFF* */
		REPEAT2
		(
			*((uint16 *) out) = This->xwin.colmap[*(data++)];
			out += 2;
		)
		/* *INDENT-ON* */
	}
	else if (This->xwin.xserver_be)
	{
		while (out < end)
		{
			value = (uint16) This->xwin.colmap[*(data++)];
			BOUT16(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			value = (uint16) This->xwin.colmap[*(data++)];
			LOUT16(out, value);
		}
	}
}

/* little endian - conversion happens when colourmap is built */
static void
translate8to24(RDPCLIENT * This, const uint8 * data, uint8 * out, uint8 * end)
{
	uint32 value;

	if (This->xwin.compatible_arch)
	{
		while (out < end)
		{
			value = This->xwin.colmap[*(data++)];
			BOUT24(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			value = This->xwin.colmap[*(data++)];
			LOUT24(out, value);
		}
	}
}

static void
translate8to32(RDPCLIENT * This, const uint8 * data, uint8 * out, uint8 * end)
{
	uint32 value;

	if (This->xwin.compatible_arch)
	{
		/* *INDENT-OFF* */
		REPEAT4
		(
			*((uint32 *) out) = This->xwin.colmap[*(data++)];
			out += 4;
		)
		/* *INDENT-ON* */
	}
	else if (This->xwin.xserver_be)
	{
		while (out < end)
		{
			value = This->xwin.colmap[*(data++)];
			BOUT32(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			value = This->xwin.colmap[*(data++)];
			LOUT32(out, value);
		}
	}
}

static void
translate15to16(RDPCLIENT * This, const uint16 * data, uint8 * out, uint8 * end)
{
	uint16 pixel;
	uint16 value;
	PixelColour pc;

	if (This->xwin.xserver_be)
	{
		while (out < end)
		{
			pixel = *(data++);
			if (This->xwin.host_be)
			{
				BSWAP16(pixel);
			}
			SPLITCOLOUR15(pixel, pc);
			value = MAKECOLOUR(pc);
			BOUT16(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			pixel = *(data++);
			if (This->xwin.host_be)
			{
				BSWAP16(pixel);
			}
			SPLITCOLOUR15(pixel, pc);
			value = MAKECOLOUR(pc);
			LOUT16(out, value);
		}
	}
}

static void
translate15to24(RDPCLIENT * This, const uint16 * data, uint8 * out, uint8 * end)
{
	uint32 value;
	uint16 pixel;
	PixelColour pc;

	if (This->xwin.compatible_arch)
	{
		/* *INDENT-OFF* */
		REPEAT3
		(
			pixel = *(data++);
			SPLITCOLOUR15(pixel, pc);
			*(out++) = pc.blue;
			*(out++) = pc.green;
			*(out++) = pc.red;
		)
		/* *INDENT-ON* */
	}
	else if (This->xwin.xserver_be)
	{
		while (out < end)
		{
			pixel = *(data++);
			if (This->xwin.host_be)
			{
				BSWAP16(pixel);
			}
			SPLITCOLOUR15(pixel, pc);
			value = MAKECOLOUR(pc);
			BOUT24(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			pixel = *(data++);
			if (This->xwin.host_be)
			{
				BSWAP16(pixel);
			}
			SPLITCOLOUR15(pixel, pc);
			value = MAKECOLOUR(pc);
			LOUT24(out, value);
		}
	}
}

static void
translate15to32(RDPCLIENT * This, const uint16 * data, uint8 * out, uint8 * end)
{
	uint16 pixel;
	uint32 value;
	PixelColour pc;

	if (This->xwin.compatible_arch)
	{
		/* *INDENT-OFF* */
		REPEAT4
		(
			pixel = *(data++);
			SPLITCOLOUR15(pixel, pc);
			*(out++) = pc.blue;
			*(out++) = pc.green;
			*(out++) = pc.red;
			*(out++) = 0;
		)
		/* *INDENT-ON* */
	}
	else if (This->xwin.xserver_be)
	{
		while (out < end)
		{
			pixel = *(data++);
			if (This->xwin.host_be)
			{
				BSWAP16(pixel);
			}
			SPLITCOLOUR15(pixel, pc);
			value = MAKECOLOUR(pc);
			BOUT32(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			pixel = *(data++);
			if (This->xwin.host_be)
			{
				BSWAP16(pixel);
			}
			SPLITCOLOUR15(pixel, pc);
			value = MAKECOLOUR(pc);
			LOUT32(out, value);
		}
	}
}

static void
translate16to16(RDPCLIENT * This, const uint16 * data, uint8 * out, uint8 * end)
{
	uint16 pixel;
	uint16 value;
	PixelColour pc;

	if (This->xwin.xserver_be)
	{
		if (This->xwin.host_be)
		{
			while (out < end)
			{
				pixel = *(data++);
				BSWAP16(pixel);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				BOUT16(out, value);
			}
		}
		else
		{
			while (out < end)
			{
				pixel = *(data++);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				BOUT16(out, value);
			}
		}
	}
	else
	{
		if (This->xwin.host_be)
		{
			while (out < end)
			{
				pixel = *(data++);
				BSWAP16(pixel);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				LOUT16(out, value);
			}
		}
		else
		{
			while (out < end)
			{
				pixel = *(data++);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				LOUT16(out, value);
			}
		}
	}
}

static void
translate16to24(RDPCLIENT * This, const uint16 * data, uint8 * out, uint8 * end)
{
	uint32 value;
	uint16 pixel;
	PixelColour pc;

	if (This->xwin.compatible_arch)
	{
		/* *INDENT-OFF* */
		REPEAT3
		(
			pixel = *(data++);
			SPLITCOLOUR16(pixel, pc);
			*(out++) = pc.blue;
			*(out++) = pc.green;
			*(out++) = pc.red;
		)
		/* *INDENT-ON* */
	}
	else if (This->xwin.xserver_be)
	{
		if (This->xwin.host_be)
		{
			while (out < end)
			{
				pixel = *(data++);
				BSWAP16(pixel);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				BOUT24(out, value);
			}
		}
		else
		{
			while (out < end)
			{
				pixel = *(data++);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				BOUT24(out, value);
			}
		}
	}
	else
	{
		if (This->xwin.host_be)
		{
			while (out < end)
			{
				pixel = *(data++);
				BSWAP16(pixel);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				LOUT24(out, value);
			}
		}
		else
		{
			while (out < end)
			{
				pixel = *(data++);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				LOUT24(out, value);
			}
		}
	}
}

static void
translate16to32(RDPCLIENT * This, const uint16 * data, uint8 * out, uint8 * end)
{
	uint16 pixel;
	uint32 value;
	PixelColour pc;

	if (This->xwin.compatible_arch)
	{
		/* *INDENT-OFF* */
		REPEAT4
		(
			pixel = *(data++);
			SPLITCOLOUR16(pixel, pc);
			*(out++) = pc.blue;
			*(out++) = pc.green;
			*(out++) = pc.red;
			*(out++) = 0;
		)
		/* *INDENT-ON* */
	}
	else if (This->xwin.xserver_be)
	{
		if (This->xwin.host_be)
		{
			while (out < end)
			{
				pixel = *(data++);
				BSWAP16(pixel);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				BOUT32(out, value);
			}
		}
		else
		{
			while (out < end)
			{
				pixel = *(data++);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				BOUT32(out, value);
			}
		}
	}
	else
	{
		if (This->xwin.host_be)
		{
			while (out < end)
			{
				pixel = *(data++);
				BSWAP16(pixel);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				LOUT32(out, value);
			}
		}
		else
		{
			while (out < end)
			{
				pixel = *(data++);
				SPLITCOLOUR16(pixel, pc);
				value = MAKECOLOUR(pc);
				LOUT32(out, value);
			}
		}
	}
}

static void
translate24to16(RDPCLIENT * This, const uint8 * data, uint8 * out, uint8 * end)
{
	uint32 pixel = 0;
	uint16 value;
	PixelColour pc;

	while (out < end)
	{
		pixel = *(data++) << 16;
		pixel |= *(data++) << 8;
		pixel |= *(data++);
		SPLITCOLOUR24(pixel, pc);
		value = MAKECOLOUR(pc);
		if (This->xwin.xserver_be)
		{
			BOUT16(out, value);
		}
		else
		{
			LOUT16(out, value);
		}
	}
}

static void
translate24to24(RDPCLIENT * This, const uint8 * data, uint8 * out, uint8 * end)
{
	uint32 pixel;
	uint32 value;
	PixelColour pc;

	if (This->xwin.xserver_be)
	{
		while (out < end)
		{
			pixel = *(data++) << 16;
			pixel |= *(data++) << 8;
			pixel |= *(data++);
			SPLITCOLOUR24(pixel, pc);
			value = MAKECOLOUR(pc);
			BOUT24(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			pixel = *(data++) << 16;
			pixel |= *(data++) << 8;
			pixel |= *(data++);
			SPLITCOLOUR24(pixel, pc);
			value = MAKECOLOUR(pc);
			LOUT24(out, value);
		}
	}
}

static void
translate24to32(RDPCLIENT * This, const uint8 * data, uint8 * out, uint8 * end)
{
	uint32 pixel;
	uint32 value;
	PixelColour pc;

	if (This->xwin.compatible_arch)
	{
		/* *INDENT-OFF* */
#ifdef NEED_ALIGN
		REPEAT4
		(
			*(out++) = *(data++);
			*(out++) = *(data++);
			*(out++) = *(data++);
			*(out++) = 0;
		)
#else
		REPEAT4
		(
		 /* Only read 3 bytes. Reading 4 bytes means reading beyond buffer. */
		 *((uint32 *) out) = *((uint16 *) data) + (*((uint8 *) data + 2) << 16);
		 out += 4;
		 data += 3;
		)
#endif
		/* *INDENT-ON* */
	}
	else if (This->xwin.xserver_be)
	{
		while (out < end)
		{
			pixel = *(data++) << 16;
			pixel |= *(data++) << 8;
			pixel |= *(data++);
			SPLITCOLOUR24(pixel, pc);
			value = MAKECOLOUR(pc);
			BOUT32(out, value);
		}
	}
	else
	{
		while (out < end)
		{
			pixel = *(data++) << 16;
			pixel |= *(data++) << 8;
			pixel |= *(data++);
			SPLITCOLOUR24(pixel, pc);
			value = MAKECOLOUR(pc);
			LOUT32(out, value);
		}
	}
}

static uint8 *
translate_image(RDPCLIENT * This, int width, int height, uint8 * data)
{
	int size;
	uint8 *out;
	uint8 *end;

	/*
	   If RDP depth and X Visual depths match,
	   and arch(endian) matches, no need to translate:
	   just return data.
	   Note: select_visual should've already ensured g_no_translate
	   is only set for compatible depths, but the RDP depth might've
	   changed during connection negotiations.
	 */
	if (This->xwin.no_translate_image)
	{
		if ((This->xwin.depth == 15 && This->server_depth == 15) ||
		    (This->xwin.depth == 16 && This->server_depth == 16) ||
		    (This->xwin.depth == 24 && This->server_depth == 24))
			return data;
	}

	size = width * height * (This->xwin.bpp / 8);
	out = (uint8 *) xmalloc(size);
	end = out + size;

	switch (This->server_depth)
	{
		case 24:
			switch (This->xwin.bpp)
			{
				case 32:
					translate24to32(This, data, out, end);
					break;
				case 24:
					translate24to24(This, data, out, end);
					break;
				case 16:
					translate24to16(This, data, out, end);
					break;
			}
			break;
		case 16:
			switch (This->xwin.bpp)
			{
				case 32:
					translate16to32(This, (uint16 *) data, out, end);
					break;
				case 24:
					translate16to24(This, (uint16 *) data, out, end);
					break;
				case 16:
					translate16to16(This, (uint16 *) data, out, end);
					break;
			}
			break;
		case 15:
			switch (This->xwin.bpp)
			{
				case 32:
					translate15to32(This, (uint16 *) data, out, end);
					break;
				case 24:
					translate15to24(This, (uint16 *) data, out, end);
					break;
				case 16:
					translate15to16(This, (uint16 *) data, out, end);
					break;
			}
			break;
		case 8:
			switch (This->xwin.bpp)
			{
				case 8:
					translate8to8(This, data, out, end);
					break;
				case 16:
					translate8to16(This, data, out, end);
					break;
				case 24:
					translate8to24(This, data, out, end);
					break;
				case 32:
					translate8to32(This, data, out, end);
					break;
			}
			break;
	}
	return out;
}

BOOL
get_key_state(RDPCLIENT * This, unsigned int state, uint32 keysym)
{
	int modifierpos, key, keysymMask = 0;
	int offset;

	KeyCode keycode = XKeysymToKeycode(This->display, keysym);

	if (keycode == NoSymbol)
		return False;

	for (modifierpos = 0; modifierpos < 8; modifierpos++)
	{
		offset = This->xwin.mod_map->max_keypermod * modifierpos;

		for (key = 0; key < This->xwin.mod_map->max_keypermod; key++)
		{
			if (This->xwin.mod_map->modifiermap[offset + key] == keycode)
				keysymMask |= 1 << modifierpos;
		}
	}

	return (state & keysymMask) ? True : False;
}

static void
calculate_shifts(uint32 mask, int *shift_r, int *shift_l)
{
	*shift_l = ffs(mask) - 1;
	mask >>= *shift_l;
	*shift_r = 8 - ffs(mask & ~(mask >> 1));
}

/* Given a mask of a colour channel (e.g. XVisualInfo.red_mask),
   calculates the bits-per-pixel of this channel (a.k.a. colour weight).
 */
static unsigned
calculate_mask_weight(uint32 mask)
{
	unsigned weight = 0;
	do
	{
		weight += (mask & 1);
	}
	while (mask >>= 1);
	return weight;
}

static BOOL
select_visual(RDPCLIENT * This)
{
	XPixmapFormatValues *pfm;
	int pixmap_formats_count, visuals_count;
	XVisualInfo *vmatches = NULL;
	XVisualInfo template;
	int i;
	unsigned red_weight, blue_weight, green_weight;

	red_weight = blue_weight = green_weight = 0;

	if (This->server_depth == -1)
	{
		This->server_depth = DisplayPlanes(This->display, DefaultScreen(This->display));
	}

	pfm = XListPixmapFormats(This->display, &pixmap_formats_count);
	if (pfm == NULL)
	{
		error("Unable to get list of pixmap formats from display.\n");
		XCloseDisplay(This->display);
		return False;
	}

	/* Search for best TrueColor visual */
	template.class = TrueColor;
	vmatches = XGetVisualInfo(This->display, VisualClassMask, &template, &visuals_count);
	This->xwin.visual = NULL;
	This->xwin.no_translate_image = False;
	This->xwin.compatible_arch = False;
	if (vmatches != NULL)
	{
		for (i = 0; i < visuals_count; ++i)
		{
			XVisualInfo *visual_info = &vmatches[i];
			BOOL can_translate_to_bpp = False;
			int j;

			/* Try to find a no-translation visual that'll
			   allow us to use RDP bitmaps directly as ZPixmaps. */
			if (!This->xwin.xserver_be && (((visual_info->depth == 15) &&
					       /* R5G5B5 */
					       (visual_info->red_mask == 0x7c00) &&
					       (visual_info->green_mask == 0x3e0) &&
					       (visual_info->blue_mask == 0x1f)) ||
					      ((visual_info->depth == 16) &&
					       /* R5G6B5 */
					       (visual_info->red_mask == 0xf800) &&
					       (visual_info->green_mask == 0x7e0) &&
					       (visual_info->blue_mask == 0x1f)) ||
					      ((visual_info->depth == 24) &&
					       /* R8G8B8 */
					       (visual_info->red_mask == 0xff0000) &&
					       (visual_info->green_mask == 0xff00) &&
					       (visual_info->blue_mask == 0xff))))
			{
				This->xwin.visual = visual_info->visual;
				This->xwin.depth = visual_info->depth;
				This->xwin.compatible_arch = !This->xwin.host_be;
				This->xwin.no_translate_image = (visual_info->depth == This->server_depth);
				if (This->xwin.no_translate_image)
					/* We found the best visual */
					break;
			}
			else
			{
				This->xwin.compatible_arch = False;
			}

			if (visual_info->depth > 24)
			{
				/* Avoid 32-bit visuals and likes like the plague.
				   They're either untested or proven to work bad
				   (e.g. nvidia's Composite 32-bit visual).
				   Most implementation offer a 24-bit visual anyway. */
				continue;
			}

			/* Only care for visuals, for whose BPPs (not depths!)
			   we have a translateXtoY function. */
			for (j = 0; j < pixmap_formats_count; ++j)
			{
				if (pfm[j].depth == visual_info->depth)
				{
					if ((pfm[j].bits_per_pixel == 16) ||
					    (pfm[j].bits_per_pixel == 24) ||
					    (pfm[j].bits_per_pixel == 32))
					{
						can_translate_to_bpp = True;
					}
					break;
				}
			}

			/* Prefer formats which have the most colour depth.
			   We're being truly aristocratic here, minding each
			   weight on its own. */
			if (can_translate_to_bpp)
			{
				unsigned vis_red_weight =
					calculate_mask_weight(visual_info->red_mask);
				unsigned vis_green_weight =
					calculate_mask_weight(visual_info->green_mask);
				unsigned vis_blue_weight =
					calculate_mask_weight(visual_info->blue_mask);
				if ((vis_red_weight >= red_weight)
				    && (vis_green_weight >= green_weight)
				    && (vis_blue_weight >= blue_weight))
				{
					red_weight = vis_red_weight;
					green_weight = vis_green_weight;
					blue_weight = vis_blue_weight;
					This->xwin.visual = visual_info->visual;
					This->xwin.depth = visual_info->depth;
				}
			}
		}
		XFree(vmatches);
	}

	if (This->xwin.visual != NULL)
	{
		This->owncolmap = False;
		calculate_shifts(This->xwin.visual->red_mask, &This->xwin.red_shift_r, &This->xwin.red_shift_l);
		calculate_shifts(This->xwin.visual->green_mask, &This->xwin.green_shift_r, &This->xwin.green_shift_l);
		calculate_shifts(This->xwin.visual->blue_mask, &This->xwin.blue_shift_r, &This->xwin.blue_shift_l);
	}
	else
	{
		template.class = PseudoColor;
		template.depth = 8;
		template.colormap_size = 256;
		vmatches =
			XGetVisualInfo(This->display,
				       VisualClassMask | VisualDepthMask | VisualColormapSizeMask,
				       &template, &visuals_count);
		if (vmatches == NULL)
		{
			error("No usable TrueColor or PseudoColor visuals on this display.\n");
			XCloseDisplay(This->display);
			XFree(pfm);
			return False;
		}

		/* we use a colourmap, so the default visual should do */
		This->owncolmap = True;
		This->xwin.visual = vmatches[0].visual;
		This->xwin.depth = vmatches[0].depth;
	}

	This->xwin.bpp = 0;
	for (i = 0; i < pixmap_formats_count; ++i)
	{
		XPixmapFormatValues *pf = &pfm[i];
		if (pf->depth == This->xwin.depth)
		{
			This->xwin.bpp = pf->bits_per_pixel;

			if (This->xwin.no_translate_image)
			{
				switch (This->server_depth)
				{
					case 15:
					case 16:
						if (This->xwin.bpp != 16)
							This->xwin.no_translate_image = False;
						break;
					case 24:
						/* Yes, this will force image translation
						   on most modern servers which use 32 bits
						   for R8G8B8. */
						if (This->xwin.bpp != 24)
							This->xwin.no_translate_image = False;
						break;
					default:
						This->xwin.no_translate_image = False;
						break;
				}
			}

			/* Pixmap formats list is a depth-to-bpp mapping --
			   there's just a single entry for every depth,
			   so we can safely break here */
			break;
		}
	}
	XFree(pfm);
	pfm = NULL;
	return True;
}

/*
static int
error_handler(RDPCLIENT * This, Display * dpy, XErrorEvent * eev)
{
	if ((eev->error_code == BadMatch) && (eev->request_code == X_ConfigureWindow))
	{
		fprintf(stderr, "Got \"BadMatch\" when trying to restack windows.\n");
		fprintf(stderr,
			"This is most likely caused by a broken window manager (commonly KWin).\n");
		return 0;
	}

	return This->xwin.old_error_handler(dpy, eev);
}
*/

BOOL
ui_init(RDPCLIENT * This)
{
	int screen_num;

	This->display = XOpenDisplay(NULL);
	if (This->display == NULL)
	{
		error("Failed to open display: %s\n", XDisplayName(NULL));
		return False;
	}

	{
		uint16 endianess_test = 1;
		This->xwin.host_be = !(BOOL) (*(uint8 *) (&endianess_test));
	}

	/*This->xwin.old_error_handler = XSetErrorHandler(error_handler);*/
	This->xwin.xserver_be = (ImageByteOrder(This->display) == MSBFirst);
	screen_num = DefaultScreen(This->display);
	This->xwin.x_socket = ConnectionNumber(This->display);
	This->xwin.screen = ScreenOfDisplay(This->display, screen_num);
	This->xwin.depth = DefaultDepthOfScreen(This->xwin.screen);

	if (!select_visual(This))
		return False;

	if (This->xwin.no_translate_image)
	{
		DEBUG(("Performance optimization possible: avoiding image translation (colour depth conversion).\n"));
	}

	if (This->server_depth > This->xwin.bpp)
	{
		warning("Remote desktop colour depth %d higher than display colour depth %d.\n",
			This->server_depth, This->xwin.bpp);
	}

	DEBUG(("RDP depth: %d, display depth: %d, display bpp: %d, X server BE: %d, host BE: %d\n",
	       This->server_depth, This->xwin.depth, This->xwin.bpp, This->xwin.xserver_be, This->xwin.host_be));

	if (!This->owncolmap)
	{
		This->xwin.xcolmap =
			XCreateColormap(This->display, RootWindowOfScreen(This->xwin.screen), This->xwin.visual,
					AllocNone);
		if (This->xwin.depth <= 8)
			warning("Display colour depth is %d bit: you may want to use -C for a private colourmap.\n", This->xwin.depth);
	}

	if ((!This->ownbackstore) && (DoesBackingStore(This->xwin.screen) != Always))
	{
		warning("External BackingStore not available. Using internal.\n");
		This->ownbackstore = True;
	}

	/*
	 * Determine desktop size
	 */
	if (This->fullscreen)
	{
		This->width = WidthOfScreen(This->xwin.screen);
		This->height = HeightOfScreen(This->xwin.screen);
		This->xwin.using_full_workarea = True;
	}
	else if (This->width < 0)
	{
		/* Percent of screen */
		if (-This->width >= 100)
			This->xwin.using_full_workarea = True;
		This->height = HeightOfScreen(This->xwin.screen) * (-This->width) / 100;
		This->width = WidthOfScreen(This->xwin.screen) * (-This->width) / 100;
	}
	else if (This->width == 0)
	{
		/* Fetch geometry from _NET_WORKAREA */
		uint32 x, y, cx, cy;
		if (get_current_workarea(This, &x, &y, &cx, &cy) == 0)
		{
			This->width = cx;
			This->height = cy;
			This->xwin.using_full_workarea = True;
		}
		else
		{
			warning("Failed to get workarea: probably your window manager does not support extended hints\n");
			This->width = WidthOfScreen(This->xwin.screen);
			This->height = HeightOfScreen(This->xwin.screen);
		}
	}

	/* make sure width is a multiple of 4 */
	This->width = (This->width + 3) & ~3;

	This->xwin.mod_map = XGetModifierMapping(This->display);

	xkeymap_init(This);

	if (This->enable_compose)
		This->xwin.IM = XOpenIM(This->display, NULL, NULL, NULL);

	xclip_init(This);
	ewmh_init(This);
	if (This->seamless_rdp)
		seamless_init(This);

	DEBUG_RDP5(("server bpp %d client bpp %d depth %d\n", This->server_depth, This->xwin.bpp, This->xwin.depth));

	return True;
}

void
ui_deinit(RDPCLIENT * This)
{
	while (This->xwin.seamless_windows)
	{
		XDestroyWindow(This->display, This->xwin.seamless_windows->wnd);
		sw_remove_window(This, This->xwin.seamless_windows);
	}

	xclip_deinit(This);

	if (This->xwin.IM != NULL)
		XCloseIM(This->xwin.IM);

	if (This->xwin.null_cursor != NULL)
		ui_destroy_cursor(This, This->xwin.null_cursor);

	XFreeModifiermap(This->xwin.mod_map);

	if (This->ownbackstore)
		XFreePixmap(This->display, This->xwin.backstore);

	XFreeGC(This->display, This->xwin.gc);
	XCloseDisplay(This->display);
	This->display = NULL;
}


static void
get_window_attribs(RDPCLIENT * This, XSetWindowAttributes * attribs)
{
	attribs->background_pixel = BlackPixelOfScreen(This->xwin.screen);
	attribs->background_pixel = WhitePixelOfScreen(This->xwin.screen);
	attribs->border_pixel = WhitePixelOfScreen(This->xwin.screen);
	attribs->backing_store = This->ownbackstore ? NotUseful : Always;
	attribs->override_redirect = This->fullscreen;
	attribs->colormap = This->xwin.xcolmap;
}

static void
get_input_mask(RDPCLIENT * This, long *input_mask)
{
	*input_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
		VisibilityChangeMask | FocusChangeMask | StructureNotifyMask;

	if (This->sendmotion)
		*input_mask |= PointerMotionMask;
	if (This->ownbackstore)
		*input_mask |= ExposureMask;
	if (This->fullscreen || This->grab_keyboard)
		*input_mask |= EnterWindowMask;
	if (This->grab_keyboard)
		*input_mask |= LeaveWindowMask;
}

BOOL
ui_create_window(RDPCLIENT * This)
{
	uint8 null_pointer_mask[1] = { 0x80 };
	uint8 null_pointer_data[24] = { 0x00 };

	XSetWindowAttributes attribs;
	XClassHint *classhints;
	XSizeHints *sizehints;
	int wndwidth, wndheight;
	long input_mask, ic_input_mask;
	XEvent xevent;

	wndwidth = This->fullscreen ? WidthOfScreen(This->xwin.screen) : This->width;
	wndheight = This->fullscreen ? HeightOfScreen(This->xwin.screen) : This->height;

	/* Handle -x-y portion of geometry string */
	if (This->xpos < 0 || (This->xpos == 0 && (This->pos & 2)))
		This->xpos = WidthOfScreen(This->xwin.screen) + This->xpos - This->width;
	if (This->ypos < 0 || (This->ypos == 0 && (This->pos & 4)))
		This->ypos = HeightOfScreen(This->xwin.screen) + This->ypos - This->height;

	get_window_attribs(This, &attribs);

	This->wnd = XCreateWindow(This->display, RootWindowOfScreen(This->xwin.screen), This->xpos, This->ypos, wndwidth,
			      wndheight, 0, This->xwin.depth, InputOutput, This->xwin.visual,
			      CWBackPixel | CWBackingStore | CWOverrideRedirect | CWColormap |
			      CWBorderPixel, &attribs);

	if (This->xwin.gc == NULL)
	{
		This->xwin.gc = XCreateGC(This->display, This->wnd, 0, NULL);
		ui_reset_clip(This);
	}

	if (This->xwin.create_bitmap_gc == NULL)
		This->xwin.create_bitmap_gc = XCreateGC(This->display, This->wnd, 0, NULL);

	if ((This->ownbackstore) && (This->xwin.backstore == 0))
	{
		This->xwin.backstore = XCreatePixmap(This->display, This->wnd, This->width, This->height, This->xwin.depth);

		/* clear to prevent rubbish being exposed at startup */
		XSetForeground(This->display, This->xwin.gc, BlackPixelOfScreen(This->xwin.screen));
		XFillRectangle(This->display, This->xwin.backstore, This->xwin.gc, 0, 0, This->width, This->height);
	}

	XStoreName(This->display, This->wnd, This->title);

	if (This->hide_decorations)
		mwm_hide_decorations(This, This->wnd);

	classhints = XAllocClassHint();
	if (classhints != NULL)
	{
		classhints->res_name = classhints->res_class = "rdesktop";
		XSetClassHint(This->display, This->wnd, classhints);
		XFree(classhints);
	}

	sizehints = XAllocSizeHints();
	if (sizehints)
	{
		sizehints->flags = PMinSize | PMaxSize;
		if (This->pos)
			sizehints->flags |= PPosition;
		sizehints->min_width = sizehints->max_width = This->width;
		sizehints->min_height = sizehints->max_height = This->height;
		XSetWMNormalHints(This->display, This->wnd, sizehints);
		XFree(sizehints);
	}

	if (This->embed_wnd)
	{
		XReparentWindow(This->display, This->wnd, (Window) This->embed_wnd, 0, 0);
	}

	get_input_mask(This, &input_mask);

	if (This->xwin.IM != NULL)
	{
		This->xwin.IC = XCreateIC(This->xwin.IM, XNInputStyle, (XIMPreeditNothing | XIMStatusNothing),
				 XNClientWindow, This->wnd, XNFocusWindow, This->wnd, NULL);

		if ((This->xwin.IC != NULL)
		    && (XGetICValues(This->xwin.IC, XNFilterEvents, &ic_input_mask, NULL) == NULL))
			input_mask |= ic_input_mask;
	}

	XSelectInput(This->display, This->wnd, input_mask);
	XMapWindow(This->display, This->wnd);

	/* wait for VisibilityNotify */
	do
	{
		XMaskEvent(This->display, VisibilityChangeMask, &xevent);
	}
	while (xevent.type != VisibilityNotify);
	This->Unobscured = xevent.xvisibility.state == VisibilityUnobscured;

	This->xwin.focused = False;
	This->xwin.mouse_in_wnd = False;

	/* handle the WM_DELETE_WINDOW protocol */
	This->xwin.protocol_atom = XInternAtom(This->display, "WM_PROTOCOLS", True);
	This->xwin.kill_atom = XInternAtom(This->display, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(This->display, This->wnd, &This->xwin.kill_atom, 1);

	/* create invisible 1x1 cursor to be used as null cursor */
	if (This->xwin.null_cursor == NULL)
		This->xwin.null_cursor = ui_create_cursor(This, 0, 0, 1, 1, null_pointer_mask, null_pointer_data);

	return True;
}

void
ui_resize_window(RDPCLIENT * This)
{
	XSizeHints *sizehints;
	Pixmap bs;

	sizehints = XAllocSizeHints();
	if (sizehints)
	{
		sizehints->flags = PMinSize | PMaxSize;
		sizehints->min_width = sizehints->max_width = This->width;
		sizehints->min_height = sizehints->max_height = This->height;
		XSetWMNormalHints(This->display, This->wnd, sizehints);
		XFree(sizehints);
	}

	if (!(This->fullscreen || This->embed_wnd))
	{
		XResizeWindow(This->display, This->wnd, This->width, This->height);
	}

	/* create new backstore pixmap */
	if (This->xwin.backstore != 0)
	{
		bs = XCreatePixmap(This->display, This->wnd, This->width, This->height, This->xwin.depth);
		XSetForeground(This->display, This->xwin.gc, BlackPixelOfScreen(This->xwin.screen));
		XFillRectangle(This->display, bs, This->xwin.gc, 0, 0, This->width, This->height);
		XCopyArea(This->display, This->xwin.backstore, bs, This->xwin.gc, 0, 0, This->width, This->height, 0, 0);
		XFreePixmap(This->display, This->xwin.backstore);
		This->xwin.backstore = bs;
	}
}

void
ui_destroy_window(RDPCLIENT * This)
{
	if (This->xwin.IC != NULL)
		XDestroyIC(This->xwin.IC);

	XDestroyWindow(This->display, This->wnd);
}

void
xwin_toggle_fullscreen(RDPCLIENT * This)
{
	Pixmap contents = 0;

	if (This->xwin.seamless_active)
		/* Turn off SeamlessRDP mode */
		ui_seamless_toggle(This);

	if (!This->ownbackstore)
	{
		/* need to save contents of window */
		contents = XCreatePixmap(This->display, This->wnd, This->width, This->height, This->xwin.depth);
		XCopyArea(This->display, This->wnd, contents, This->xwin.gc, 0, 0, This->width, This->height, 0, 0);
	}

	ui_destroy_window(This);
	This->fullscreen = !This->fullscreen;
	ui_create_window(This);

	XDefineCursor(This->display, This->wnd, This->xwin.current_cursor);

	if (!This->ownbackstore)
	{
		XCopyArea(This->display, contents, This->wnd, This->xwin.gc, 0, 0, This->width, This->height, 0, 0);
		XFreePixmap(This->display, contents);
	}
}

static void
handle_button_event(RDPCLIENT * This, XEvent xevent, BOOL down)
{
	uint16 button, flags = 0;
	This->last_gesturetime = xevent.xbutton.time;
	button = xkeymap_translate_button(xevent.xbutton.button);
	if (button == 0)
		return;

	if (down)
		flags = MOUSE_FLAG_DOWN;

	/* Stop moving window when button is released, regardless of cursor position */
	if (This->xwin.moving_wnd && (xevent.type == ButtonRelease))
		This->xwin.moving_wnd = False;

	/* If win_button_sizee is nonzero, enable single app mode */
	if (xevent.xbutton.y < This->win_button_size)
	{
		/*  Check from right to left: */
		if (xevent.xbutton.x >= This->width - This->win_button_size)
		{
			/* The close button, continue */
			;
		}
		else if (xevent.xbutton.x >= This->width - This->win_button_size * 2)
		{
			/* The maximize/restore button. Do not send to
			   server.  It might be a good idea to change the
			   cursor or give some other visible indication
			   that rdesktop inhibited this click */
			if (xevent.type == ButtonPress)
				return;
		}
		else if (xevent.xbutton.x >= This->width - This->win_button_size * 3)
		{
			/* The minimize button. Iconify window. */
			if (xevent.type == ButtonRelease)
			{
				/* Release the mouse button outside the minimize button, to prevent the
				   actual minimazation to happen */
				rdp_send_input(This, time(NULL), RDP_INPUT_MOUSE, button, 1, 1);
				XIconifyWindow(This->display, This->wnd, DefaultScreen(This->display));
				return;
			}
		}
		else if (xevent.xbutton.x <= This->win_button_size)
		{
			/* The system menu. Ignore. */
			if (xevent.type == ButtonPress)
				return;
		}
		else
		{
			/* The title bar. */
			if (xevent.type == ButtonPress)
			{
				if (!This->fullscreen && This->hide_decorations && !This->xwin.using_full_workarea)
				{
					This->xwin.moving_wnd = True;
					This->xwin.move_x_offset = xevent.xbutton.x;
					This->xwin.move_y_offset = xevent.xbutton.y;
				}
				return;
			}
		}
	}

	if (xevent.xmotion.window == This->wnd)
	{
		rdp_send_input(This, time(NULL), RDP_INPUT_MOUSE,
			       flags | button, xevent.xbutton.x, xevent.xbutton.y);
	}
	else
	{
		/* SeamlessRDP */
		rdp_send_input(This, time(NULL), RDP_INPUT_MOUSE,
			       flags | button, xevent.xbutton.x_root, xevent.xbutton.y_root);
	}
}


/* Process events in Xlib queue
   Returns 0 after user quit, 1 otherwise */
static int
xwin_process_events(RDPCLIENT * This)
{
	XEvent xevent;
	KeySym keysym;
	uint32 ev_time;
	char str[256];
	Status status;
	int events = 0;
	seamless_window *sw;

	while ((XPending(This->display) > 0) && events++ < 20)
	{
		XNextEvent(This->display, &xevent);

		if ((This->xwin.IC != NULL) && (XFilterEvent(&xevent, None) == True))
		{
			DEBUG_KBD(("Filtering event\n"));
			continue;
		}

		switch (xevent.type)
		{
			case VisibilityNotify:
				if (xevent.xvisibility.window == This->wnd)
					This->Unobscured =
						xevent.xvisibility.state == VisibilityUnobscured;

				break;
			case ClientMessage:
				/* the window manager told us to quit */
				if ((xevent.xclient.message_type == This->xwin.protocol_atom)
				    && ((Atom) xevent.xclient.data.l[0] == This->xwin.kill_atom))
					/* Quit */
					return 0;
				break;

			case KeyPress:
				This->last_gesturetime = xevent.xkey.time;
				if (This->xwin.IC != NULL)
					/* Multi_key compatible version */
				{
					XmbLookupString(This->xwin.IC,
							&xevent.xkey, str, sizeof(str), &keysym,
							&status);
					if (!((status == XLookupKeySym) || (status == XLookupBoth)))
					{
						error("XmbLookupString failed with status 0x%x\n",
						      status);
						break;
					}
				}
				else
				{
					/* Plain old XLookupString */
					DEBUG_KBD(("\nNo input context, using XLookupString\n"));
					XLookupString((XKeyEvent *) & xevent,
						      str, sizeof(str), &keysym, NULL);
				}

				DEBUG_KBD(("KeyPress for keysym (0x%lx, %s)\n", keysym,
					   get_ksname(keysym)));

				ev_time = time(NULL);
				if (handle_special_keys(This, keysym, xevent.xkey.state, ev_time, True))
					break;

				xkeymap_send_keys(This, keysym, xevent.xkey.keycode, xevent.xkey.state,
						  ev_time, True, 0);
				break;

			case KeyRelease:
				This->last_gesturetime = xevent.xkey.time;
				XLookupString((XKeyEvent *) & xevent, str,
					      sizeof(str), &keysym, NULL);

				DEBUG_KBD(("\nKeyRelease for keysym (0x%lx, %s)\n", keysym,
					   get_ksname(keysym)));

				ev_time = time(NULL);
				if (handle_special_keys(This, keysym, xevent.xkey.state, ev_time, False))
					break;

				xkeymap_send_keys(This, keysym, xevent.xkey.keycode, xevent.xkey.state,
						  ev_time, False, 0);
				break;

			case ButtonPress:
				handle_button_event(This, xevent, True);
				break;

			case ButtonRelease:
				handle_button_event(This, xevent, False);
				break;

			case MotionNotify:
				if (This->xwin.moving_wnd)
				{
					XMoveWindow(This->display, This->wnd,
						    xevent.xmotion.x_root - This->xwin.move_x_offset,
						    xevent.xmotion.y_root - This->xwin.move_y_offset);
					break;
				}

				if (This->fullscreen && !This->xwin.focused)
					XSetInputFocus(This->display, This->wnd, RevertToPointerRoot,
						       CurrentTime);

				if (xevent.xmotion.window == This->wnd)
				{
					rdp_send_input(This, time(NULL), RDP_INPUT_MOUSE, MOUSE_FLAG_MOVE,
						       xevent.xmotion.x, xevent.xmotion.y);
				}
				else
				{
					/* SeamlessRDP */
					rdp_send_input(This, time(NULL), RDP_INPUT_MOUSE, MOUSE_FLAG_MOVE,
						       xevent.xmotion.x_root,
						       xevent.xmotion.y_root);
				}
				break;

			case FocusIn:
				if (xevent.xfocus.mode == NotifyGrab)
					break;
				This->xwin.focused = True;
				reset_modifier_keys(This);
				if (This->grab_keyboard && This->xwin.mouse_in_wnd)
					XGrabKeyboard(This->display, This->wnd, True,
						      GrabModeAsync, GrabModeAsync, CurrentTime);

				sw = sw_get_window_by_wnd(This, xevent.xfocus.window);
				if (!sw)
					break;

				if (sw->id != This->xwin.seamless_focused)
				{
					seamless_send_focus(This, sw->id, 0);
					This->xwin.seamless_focused = sw->id;
				}
				break;

			case FocusOut:
				if (xevent.xfocus.mode == NotifyUngrab)
					break;
				This->xwin.focused = False;
				if (xevent.xfocus.mode == NotifyWhileGrabbed)
					XUngrabKeyboard(This->display, CurrentTime);
				break;

			case EnterNotify:
				/* we only register for this event when in fullscreen mode */
				/* or grab_keyboard */
				This->xwin.mouse_in_wnd = True;
				if (This->fullscreen)
				{
					XSetInputFocus(This->display, This->wnd, RevertToPointerRoot,
						       CurrentTime);
					break;
				}
				if (This->xwin.focused)
					XGrabKeyboard(This->display, This->wnd, True,
						      GrabModeAsync, GrabModeAsync, CurrentTime);
				break;

			case LeaveNotify:
				/* we only register for this event when grab_keyboard */
				This->xwin.mouse_in_wnd = False;
				XUngrabKeyboard(This->display, CurrentTime);
				break;

			case Expose:
				if (xevent.xexpose.window == This->wnd)
				{
					XCopyArea(This->display, This->xwin.backstore, xevent.xexpose.window,
						  This->xwin.gc,
						  xevent.xexpose.x, xevent.xexpose.y,
						  xevent.xexpose.width, xevent.xexpose.height,
						  xevent.xexpose.x, xevent.xexpose.y);
				}
				else
				{
					sw = sw_get_window_by_wnd(This, xevent.xexpose.window);
					if (!sw)
						break;
					XCopyArea(This->display, This->xwin.backstore,
						  xevent.xexpose.window, This->xwin.gc,
						  xevent.xexpose.x + sw->xoffset,
						  xevent.xexpose.y + sw->yoffset,
						  xevent.xexpose.width,
						  xevent.xexpose.height, xevent.xexpose.x,
						  xevent.xexpose.y);
				}

				break;

			case MappingNotify:
				/* Refresh keyboard mapping if it has changed. This is important for
				   Xvnc, since it allocates keycodes dynamically */
				if (xevent.xmapping.request == MappingKeyboard
				    || xevent.xmapping.request == MappingModifier)
					XRefreshKeyboardMapping(&xevent.xmapping);

				if (xevent.xmapping.request == MappingModifier)
				{
					XFreeModifiermap(This->xwin.mod_map);
					This->xwin.mod_map = XGetModifierMapping(This->display);
				}
				break;

				/* clipboard stuff */
			case SelectionNotify:
				xclip_handle_SelectionNotify(This, &xevent.xselection);
				break;
			case SelectionRequest:
				xclip_handle_SelectionRequest(This, &xevent.xselectionrequest);
				break;
			case SelectionClear:
				xclip_handle_SelectionClear(This);
				break;
			case PropertyNotify:
				xclip_handle_PropertyNotify(This, &xevent.xproperty);
				if (xevent.xproperty.window == This->wnd)
					break;
				if (xevent.xproperty.window == DefaultRootWindow(This->display))
					break;

				/* seamless */
				sw = sw_get_window_by_wnd(This, xevent.xproperty.window);
				if (!sw)
					break;

				if ((xevent.xproperty.atom == This->net_wm_state_atom)
				    && (xevent.xproperty.state == PropertyNewValue))
				{
					sw->state = ewmh_get_window_state(This, sw->wnd);
					seamless_send_state(This, sw->id, sw->state, 0);
				}

				if ((xevent.xproperty.atom == This->net_wm_desktop_atom)
				    && (xevent.xproperty.state == PropertyNewValue))
				{
					sw->desktop = ewmh_get_window_desktop(This, sw->wnd);
					sw_all_to_desktop(This, sw->wnd, sw->desktop);
				}

				break;
			case MapNotify:
				if (!This->xwin.seamless_active)
					rdp_send_client_window_status(This, 1);
				break;
			case UnmapNotify:
				if (!This->xwin.seamless_active)
					rdp_send_client_window_status(This, 0);
				break;
			case ConfigureNotify:
				if (!This->xwin.seamless_active)
					break;

				sw = sw_get_window_by_wnd(This, xevent.xconfigure.window);
				if (!sw)
					break;

				gettimeofday(sw->position_timer, NULL);
				if (sw->position_timer->tv_usec + SEAMLESSRDP_POSITION_TIMER >=
				    1000000)
				{
					sw->position_timer->tv_usec +=
						SEAMLESSRDP_POSITION_TIMER - 1000000;
					sw->position_timer->tv_sec += 1;
				}
				else
				{
					sw->position_timer->tv_usec += SEAMLESSRDP_POSITION_TIMER;
				}

				sw_handle_restack(This, sw);
				break;
		}
	}
	/* Keep going */
	return 1;
}

/* Returns 0 after user quit, 1 otherwise */
int
ui_select(RDPCLIENT * This, int rdp_socket)
{
	int n;
	fd_set rfds, wfds;
	struct timeval tv;
	BOOL s_timeout = False;

	while (True)
	{
		n = (rdp_socket > This->xwin.x_socket) ? rdp_socket : This->xwin.x_socket;
		/* Process any events already waiting */
		if (!xwin_process_events(This))
			/* User quit */
			return 0;

		if (This->xwin.seamless_active)
			sw_check_timers(This);

		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_SET(rdp_socket, &rfds);
		FD_SET(This->xwin.x_socket, &rfds);

#ifdef WITH_RDPSND
		/* FIXME: there should be an API for registering fds */
		if (This->dsp_busy)
		{
			FD_SET(This->dsp_fd, &wfds);
			n = (This->dsp_fd > n) ? This->dsp_fd : n;
		}
#endif
		/* default timeout */
		tv.tv_sec = 60;
		tv.tv_usec = 0;

		/* add redirection handles */
		rdpdr_add_fds(This, &n, &rfds, &wfds, &tv, &s_timeout);
		seamless_select_timeout(This, &tv);

		n++;

		switch (select(n, &rfds, &wfds, NULL, &tv))
		{
			case -1:
				error("select: %s\n", strerror(errno));

			case 0:
				/* Abort serial read calls */
				if (s_timeout)
					rdpdr_check_fds(This, &rfds, &wfds, (BOOL) True);
				continue;
		}

		rdpdr_check_fds(This, &rfds, &wfds, (BOOL) False);

		if (FD_ISSET(rdp_socket, &rfds))
			return 1;

#ifdef WITH_RDPSND
		if (This->dsp_busy && FD_ISSET(This->dsp_fd, &wfds))
			wave_out_play();
#endif
	}
}

void
ui_move_pointer(RDPCLIENT * This, int x, int y)
{
	XWarpPointer(This->display, This->wnd, This->wnd, 0, 0, 0, 0, x, y);
}

HBITMAP
ui_create_bitmap(RDPCLIENT * This, int width, int height, uint8 * data)
{
	XImage *image;
	Pixmap bitmap;
	uint8 *tdata;
	int bitmap_pad;

	if (This->server_depth == 8)
	{
		bitmap_pad = 8;
	}
	else
	{
		bitmap_pad = This->xwin.bpp;

		if (This->xwin.bpp == 24)
			bitmap_pad = 32;
	}

	tdata = (This->owncolmap ? data : translate_image(This, width, height, data));
	bitmap = XCreatePixmap(This->display, This->wnd, width, height, This->xwin.depth);
	image = XCreateImage(This->display, This->xwin.visual, This->xwin.depth, ZPixmap, 0,
			     (char *) tdata, width, height, bitmap_pad, 0);

	XPutImage(This->display, bitmap, This->xwin.create_bitmap_gc, image, 0, 0, 0, 0, width, height);

	XFree(image);
	if (tdata != data)
		xfree(tdata);
	return (HBITMAP) bitmap;
}

void
ui_paint_bitmap(RDPCLIENT * This, int x, int y, int cx, int cy, int width, int height, uint8 * data)
{
	XImage *image;
	uint8 *tdata;
	int bitmap_pad;

	if (This->server_depth == 8)
	{
		bitmap_pad = 8;
	}
	else
	{
		bitmap_pad = This->xwin.bpp;

		if (This->xwin.bpp == 24)
			bitmap_pad = 32;
	}

	tdata = (This->owncolmap ? data : translate_image(This, width, height, data));
	image = XCreateImage(This->display, This->xwin.visual, This->xwin.depth, ZPixmap, 0,
			     (char *) tdata, width, height, bitmap_pad, 0);

	if (This->ownbackstore)
	{
		XPutImage(This->display, This->xwin.backstore, This->xwin.gc, image, 0, 0, x, y, cx, cy);
		XCopyArea(This->display, This->xwin.backstore, This->wnd, This->xwin.gc, x, y, cx, cy, x, y);
		ON_ALL_SEAMLESS_WINDOWS(XCopyArea,
					(This->display, This->xwin.backstore, sw->wnd, This->xwin.gc, x, y, cx, cy,
					 x - sw->xoffset, y - sw->yoffset));
	}
	else
	{
		XPutImage(This->display, This->wnd, This->xwin.gc, image, 0, 0, x, y, cx, cy);
		ON_ALL_SEAMLESS_WINDOWS(XCopyArea,
					(This->display, This->wnd, sw->wnd, This->xwin.gc, x, y, cx, cy,
					 x - sw->xoffset, y - sw->yoffset));
	}

	XFree(image);
	if (tdata != data)
		xfree(tdata);
}

void
ui_destroy_bitmap(RDPCLIENT * This, HBITMAP bmp)
{
	XFreePixmap(This->display, (Pixmap) bmp);
}

HGLYPH
ui_create_glyph(RDPCLIENT * This, int width, int height, const uint8 * data)
{
	XImage *image;
	Pixmap bitmap;
	int scanline;

	scanline = (width + 7) / 8;

	bitmap = XCreatePixmap(This->display, This->wnd, width, height, 1);
	if (This->xwin.create_glyph_gc == 0)
		This->xwin.create_glyph_gc = XCreateGC(This->display, bitmap, 0, NULL);

	image = XCreateImage(This->display, This->xwin.visual, 1, ZPixmap, 0, (char *) data,
			     width, height, 8, scanline);
	image->byte_order = MSBFirst;
	image->bitmap_bit_order = MSBFirst;
	XInitImage(image);

	XPutImage(This->display, bitmap, This->xwin.create_glyph_gc, image, 0, 0, 0, 0, width, height);

	XFree(image);
	return (HGLYPH) bitmap;
}

void
ui_destroy_glyph(RDPCLIENT * This, HGLYPH glyph)
{
	XFreePixmap(This->display, (Pixmap) glyph);
}

HCURSOR
ui_create_cursor(RDPCLIENT * This, unsigned int x, unsigned int y, int width, int height,
		 uint8 * andmask, uint8 * xormask)
{
	HGLYPH maskglyph, cursorglyph;
	XColor bg, fg;
	Cursor xcursor;
	uint8 *cursor, *pcursor;
	uint8 *mask, *pmask;
	uint8 nextbit;
	int scanline, offset;
	int i, j;

	scanline = (width + 7) / 8;
	offset = scanline * height;

	cursor = (uint8 *) xmalloc(offset);
	memset(cursor, 0, offset);

	mask = (uint8 *) xmalloc(offset);
	memset(mask, 0, offset);

	/* approximate AND and XOR masks with a monochrome X pointer */
	for (i = 0; i < height; i++)
	{
		offset -= scanline;
		pcursor = &cursor[offset];
		pmask = &mask[offset];

		for (j = 0; j < scanline; j++)
		{
			for (nextbit = 0x80; nextbit != 0; nextbit >>= 1)
			{
				if (xormask[0] || xormask[1] || xormask[2])
				{
					*pcursor |= (~(*andmask) & nextbit);
					*pmask |= nextbit;
				}
				else
				{
					*pcursor |= ((*andmask) & nextbit);
					*pmask |= (~(*andmask) & nextbit);
				}

				xormask += 3;
			}

			andmask++;
			pcursor++;
			pmask++;
		}
	}

	fg.red = fg.blue = fg.green = 0xffff;
	bg.red = bg.blue = bg.green = 0x0000;
	fg.flags = bg.flags = DoRed | DoBlue | DoGreen;

	cursorglyph = ui_create_glyph(This, width, height, cursor);
	maskglyph = ui_create_glyph(This, width, height, mask);

	xcursor =
		XCreatePixmapCursor(This->display, (Pixmap) cursorglyph,
				    (Pixmap) maskglyph, &fg, &bg, x, y);

	ui_destroy_glyph(This, maskglyph);
	ui_destroy_glyph(This, cursorglyph);
	xfree(mask);
	xfree(cursor);
	return (HCURSOR) xcursor;
}

void
ui_set_cursor(RDPCLIENT * This, HCURSOR cursor)
{
	This->xwin.current_cursor = (Cursor) cursor;
	XDefineCursor(This->display, This->wnd, This->xwin.current_cursor);
	ON_ALL_SEAMLESS_WINDOWS(XDefineCursor, (This->display, sw->wnd, This->xwin.current_cursor));
}

void
ui_destroy_cursor(RDPCLIENT * This, HCURSOR cursor)
{
	XFreeCursor(This->display, (Cursor) cursor);
}

void
ui_set_null_cursor(RDPCLIENT * This)
{
	ui_set_cursor(This, This->xwin.null_cursor);
}

#define MAKE_XCOLOR(xc,c) \
		(xc)->red   = ((c)->red   << 8) | (c)->red; \
		(xc)->green = ((c)->green << 8) | (c)->green; \
		(xc)->blue  = ((c)->blue  << 8) | (c)->blue; \
		(xc)->flags = DoRed | DoGreen | DoBlue;


HCOLOURMAP
ui_create_colourmap(RDPCLIENT * This, COLOURMAP * colours)
{
	COLOURENTRY *entry;
	int i, ncolours = colours->ncolours;
	if (!This->owncolmap)
	{
		uint32 *map = (uint32 *) xmalloc(sizeof(*This->xwin.colmap) * ncolours);
		XColor xentry;
		XColor xc_cache[256];
		uint32 colour;
		int colLookup = 256;
		for (i = 0; i < ncolours; i++)
		{
			entry = &colours->colours[i];
			MAKE_XCOLOR(&xentry, entry);

			if (XAllocColor(This->display, This->xwin.xcolmap, &xentry) == 0)
			{
				/* Allocation failed, find closest match. */
				int j = 256;
				int nMinDist = 3 * 256 * 256;
				long nDist = nMinDist;

				/* only get the colors once */
				while (colLookup--)
				{
					xc_cache[colLookup].pixel = colLookup;
					xc_cache[colLookup].red = xc_cache[colLookup].green =
						xc_cache[colLookup].blue = 0;
					xc_cache[colLookup].flags = 0;
					XQueryColor(This->display,
						    DefaultColormap(This->display,
								    DefaultScreen(This->display)),
						    &xc_cache[colLookup]);
				}
				colLookup = 0;

				/* approximate the pixel */
				while (j--)
				{
					if (xc_cache[j].flags)
					{
						nDist = ((long) (xc_cache[j].red >> 8) -
							 (long) (xentry.red >> 8)) *
							((long) (xc_cache[j].red >> 8) -
							 (long) (xentry.red >> 8)) +
							((long) (xc_cache[j].green >> 8) -
							 (long) (xentry.green >> 8)) *
							((long) (xc_cache[j].green >> 8) -
							 (long) (xentry.green >> 8)) +
							((long) (xc_cache[j].blue >> 8) -
							 (long) (xentry.blue >> 8)) *
							((long) (xc_cache[j].blue >> 8) -
							 (long) (xentry.blue >> 8));
					}
					if (nDist < nMinDist)
					{
						nMinDist = nDist;
						xentry.pixel = j;
					}
				}
			}
			colour = xentry.pixel;

			/* update our cache */
			if (xentry.pixel < 256)
			{
				xc_cache[xentry.pixel].red = xentry.red;
				xc_cache[xentry.pixel].green = xentry.green;
				xc_cache[xentry.pixel].blue = xentry.blue;

			}

			map[i] = colour;
		}
		return map;
	}
	else
	{
		XColor *xcolours, *xentry;
		Colormap map;

		xcolours = (XColor *) xmalloc(sizeof(XColor) * ncolours);
		for (i = 0; i < ncolours; i++)
		{
			entry = &colours->colours[i];
			xentry = &xcolours[i];
			xentry->pixel = i;
			MAKE_XCOLOR(xentry, entry);
		}

		map = XCreateColormap(This->display, This->wnd, This->xwin.visual, AllocAll);
		XStoreColors(This->display, map, xcolours, ncolours);

		xfree(xcolours);
		return (HCOLOURMAP) map;
	}
}

void
ui_destroy_colourmap(RDPCLIENT * This, HCOLOURMAP map)
{
	if (!This->owncolmap)
		xfree(map);
	else
		XFreeColormap(This->display, (Colormap) map);
}

void
ui_set_colourmap(RDPCLIENT * This, HCOLOURMAP map)
{
	if (!This->owncolmap)
	{
		if (This->xwin.colmap)
			xfree(This->xwin.colmap);

		This->xwin.colmap = (uint32 *) map;
	}
	else
	{
		XSetWindowColormap(This->display, This->wnd, (Colormap) map);
		ON_ALL_SEAMLESS_WINDOWS(XSetWindowColormap, (This->display, sw->wnd, (Colormap) map));
	}
}

void
ui_set_clip(RDPCLIENT * This, int x, int y, int cx, int cy)
{
	This->xwin.clip_rectangle.x = x;
	This->xwin.clip_rectangle.y = y;
	This->xwin.clip_rectangle.width = cx;
	This->xwin.clip_rectangle.height = cy;
	XSetClipRectangles(This->display, This->xwin.gc, 0, 0, &This->xwin.clip_rectangle, 1, YXBanded);
}

void
ui_reset_clip(RDPCLIENT * This)
{
	This->xwin.clip_rectangle.x = 0;
	This->xwin.clip_rectangle.y = 0;
	This->xwin.clip_rectangle.width = This->width;
	This->xwin.clip_rectangle.height = This->height;
	XSetClipRectangles(This->display, This->xwin.gc, 0, 0, &This->xwin.clip_rectangle, 1, YXBanded);
}

void
ui_bell(RDPCLIENT * This)
{
	XBell(This->display, 0);
}

void
ui_destblt(RDPCLIENT * This, uint8 opcode,
	   /* dest */ int x, int y, int cx, int cy)
{
	SET_FUNCTION(opcode);
	FILL_RECTANGLE(x, y, cx, cy);
	RESET_FUNCTION(opcode);
}

static const uint8 hatch_patterns[] = {
	0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,	/* 0 - bsHorizontal */
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,	/* 1 - bsVertical */
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,	/* 2 - bsFDiagonal */
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,	/* 3 - bsBDiagonal */
	0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08,	/* 4 - bsCross */
	0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81	/* 5 - bsDiagCross */
};

void
ui_patblt(RDPCLIENT * This, uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
{
	Pixmap fill;
	uint8 i, ipattern[8];

	SET_FUNCTION(opcode);

	switch (brush->style)
	{
		case 0:	/* Solid */
			SET_FOREGROUND(fgcolour);
			FILL_RECTANGLE_BACKSTORE(x, y, cx, cy);
			break;

		case 2:	/* Hatch */
			fill = (Pixmap) ui_create_glyph(This, 8, 8,
							hatch_patterns + brush->pattern[0] * 8);
			SET_FOREGROUND(fgcolour);
			SET_BACKGROUND(bgcolour);
			XSetFillStyle(This->display, This->xwin.gc, FillOpaqueStippled);
			XSetStipple(This->display, This->xwin.gc, fill);
			XSetTSOrigin(This->display, This->xwin.gc, brush->xorigin, brush->yorigin);
			FILL_RECTANGLE_BACKSTORE(x, y, cx, cy);
			XSetFillStyle(This->display, This->xwin.gc, FillSolid);
			XSetTSOrigin(This->display, This->xwin.gc, 0, 0);
			ui_destroy_glyph(This, (HGLYPH) fill);
			break;

		case 3:	/* Pattern */
			for (i = 0; i != 8; i++)
				ipattern[7 - i] = brush->pattern[i];
			fill = (Pixmap) ui_create_glyph(This, 8, 8, ipattern);
			SET_FOREGROUND(bgcolour);
			SET_BACKGROUND(fgcolour);
			XSetFillStyle(This->display, This->xwin.gc, FillOpaqueStippled);
			XSetStipple(This->display, This->xwin.gc, fill);
			XSetTSOrigin(This->display, This->xwin.gc, brush->xorigin, brush->yorigin);
			FILL_RECTANGLE_BACKSTORE(x, y, cx, cy);
			XSetFillStyle(This->display, This->xwin.gc, FillSolid);
			XSetTSOrigin(This->display, This->xwin.gc, 0, 0);
			ui_destroy_glyph(This, (HGLYPH) fill);
			break;

		default:
			unimpl("brush %d\n", brush->style);
	}

	RESET_FUNCTION(opcode);

	if (This->ownbackstore)
		XCopyArea(This->display, This->xwin.backstore, This->wnd, This->xwin.gc, x, y, cx, cy, x, y);
	ON_ALL_SEAMLESS_WINDOWS(XCopyArea,
				(This->display, This->ownbackstore ? This->xwin.backstore : This->wnd, sw->wnd, This->xwin.gc,
				 x, y, cx, cy, x - sw->xoffset, y - sw->yoffset));
}

void
ui_screenblt(RDPCLIENT * This, uint8 opcode,
	     /* dest */ int x, int y, int cx, int cy,
	     /* src */ int srcx, int srcy)
{
	SET_FUNCTION(opcode);
	if (This->ownbackstore)
	{
		XCopyArea(This->display, This->Unobscured ? This->wnd : This->xwin.backstore,
			  This->wnd, This->xwin.gc, srcx, srcy, cx, cy, x, y);
		XCopyArea(This->display, This->xwin.backstore, This->xwin.backstore, This->xwin.gc, srcx, srcy, cx, cy, x, y);
	}
	else
	{
		XCopyArea(This->display, This->wnd, This->wnd, This->xwin.gc, srcx, srcy, cx, cy, x, y);
	}

	ON_ALL_SEAMLESS_WINDOWS(XCopyArea,
				(This->display, This->ownbackstore ? This->xwin.backstore : This->wnd,
				 sw->wnd, This->xwin.gc, x, y, cx, cy, x - sw->xoffset, y - sw->yoffset));

	RESET_FUNCTION(opcode);
}

void
ui_memblt(RDPCLIENT * This, uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* src */ HBITMAP src, int srcx, int srcy)
{
	SET_FUNCTION(opcode);
	XCopyArea(This->display, (Pixmap) src, This->wnd, This->xwin.gc, srcx, srcy, cx, cy, x, y);
	ON_ALL_SEAMLESS_WINDOWS(XCopyArea,
				(This->display, (Pixmap) src, sw->wnd, This->xwin.gc,
				 srcx, srcy, cx, cy, x - sw->xoffset, y - sw->yoffset));
	if (This->ownbackstore)
		XCopyArea(This->display, (Pixmap) src, This->xwin.backstore, This->xwin.gc, srcx, srcy, cx, cy, x, y);
	RESET_FUNCTION(opcode);
}

void
ui_triblt(RDPCLIENT * This, uint8 opcode,
	  /* dest */ int x, int y, int cx, int cy,
	  /* src */ HBITMAP src, int srcx, int srcy,
	  /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
{
	/* This is potentially difficult to do in general. Until someone
	   comes up with a more efficient way of doing it I am using cases. */

	switch (opcode)
	{
		case 0x69:	/* PDSxxn */
			ui_memblt(This, ROP2_XOR, x, y, cx, cy, src, srcx, srcy);
			ui_patblt(This, ROP2_NXOR, x, y, cx, cy, brush, bgcolour, fgcolour);
			break;

		case 0xb8:	/* PSDPxax */
			ui_patblt(This, ROP2_XOR, x, y, cx, cy, brush, bgcolour, fgcolour);
			ui_memblt(This, ROP2_AND, x, y, cx, cy, src, srcx, srcy);
			ui_patblt(This, ROP2_XOR, x, y, cx, cy, brush, bgcolour, fgcolour);
			break;

		case 0xc0:	/* PSa */
			ui_memblt(This, ROP2_COPY, x, y, cx, cy, src, srcx, srcy);
			ui_patblt(This, ROP2_AND, x, y, cx, cy, brush, bgcolour, fgcolour);
			break;

		default:
			unimpl("triblt 0x%x\n", opcode);
			ui_memblt(This, ROP2_COPY, x, y, cx, cy, src, srcx, srcy);
	}
}

void
ui_line(RDPCLIENT * This, uint8 opcode,
	/* dest */ int startx, int starty, int endx, int endy,
	/* pen */ PEN * pen)
{
	SET_FUNCTION(opcode);
	SET_FOREGROUND(pen->colour);
	XDrawLine(This->display, This->wnd, This->xwin.gc, startx, starty, endx, endy);
	ON_ALL_SEAMLESS_WINDOWS(XDrawLine, (This->display, sw->wnd, This->xwin.gc,
					    startx - sw->xoffset, starty - sw->yoffset,
					    endx - sw->xoffset, endy - sw->yoffset));
	if (This->ownbackstore)
		XDrawLine(This->display, This->xwin.backstore, This->xwin.gc, startx, starty, endx, endy);
	RESET_FUNCTION(opcode);
}

void
ui_rect(RDPCLIENT * This,
	       /* dest */ int x, int y, int cx, int cy,
	       /* brush */ int colour)
{
	SET_FOREGROUND(colour);
	FILL_RECTANGLE(x, y, cx, cy);
}

void
ui_polygon(RDPCLIENT * This, uint8 opcode,
	   /* mode */ uint8 fillmode,
	   /* dest */ POINT * point, int npoints,
	   /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
{
	uint8 style, i, ipattern[8];
	Pixmap fill;

	SET_FUNCTION(opcode);

	switch (fillmode)
	{
		case ALTERNATE:
			XSetFillRule(This->display, This->xwin.gc, EvenOddRule);
			break;
		case WINDING:
			XSetFillRule(This->display, This->xwin.gc, WindingRule);
			break;
		default:
			unimpl("fill mode %d\n", fillmode);
	}

	if (brush)
		style = brush->style;
	else
		style = 0;

	switch (style)
	{
		case 0:	/* Solid */
			SET_FOREGROUND(fgcolour);
			FILL_POLYGON((XPoint *) point, npoints);
			break;

		case 2:	/* Hatch */
			fill = (Pixmap) ui_create_glyph(This, 8, 8,
							hatch_patterns + brush->pattern[0] * 8);
			SET_FOREGROUND(fgcolour);
			SET_BACKGROUND(bgcolour);
			XSetFillStyle(This->display, This->xwin.gc, FillOpaqueStippled);
			XSetStipple(This->display, This->xwin.gc, fill);
			XSetTSOrigin(This->display, This->xwin.gc, brush->xorigin, brush->yorigin);
			FILL_POLYGON((XPoint *) point, npoints);
			XSetFillStyle(This->display, This->xwin.gc, FillSolid);
			XSetTSOrigin(This->display, This->xwin.gc, 0, 0);
			ui_destroy_glyph(This, (HGLYPH) fill);
			break;

		case 3:	/* Pattern */
			for (i = 0; i != 8; i++)
				ipattern[7 - i] = brush->pattern[i];
			fill = (Pixmap) ui_create_glyph(This, 8, 8, ipattern);
			SET_FOREGROUND(bgcolour);
			SET_BACKGROUND(fgcolour);
			XSetFillStyle(This->display, This->xwin.gc, FillOpaqueStippled);
			XSetStipple(This->display, This->xwin.gc, fill);
			XSetTSOrigin(This->display, This->xwin.gc, brush->xorigin, brush->yorigin);
			FILL_POLYGON((XPoint *) point, npoints);
			XSetFillStyle(This->display, This->xwin.gc, FillSolid);
			XSetTSOrigin(This->display, This->xwin.gc, 0, 0);
			ui_destroy_glyph(This, (HGLYPH) fill);
			break;

		default:
			unimpl("brush %d\n", brush->style);
	}

	RESET_FUNCTION(opcode);
}

void
ui_polyline(RDPCLIENT * This, uint8 opcode,
	    /* dest */ POINT * points, int npoints,
	    /* pen */ PEN * pen)
{
	/* TODO: set join style */
	SET_FUNCTION(opcode);
	SET_FOREGROUND(pen->colour);
	XDrawLines(This->display, This->wnd, This->xwin.gc, (XPoint *) points, npoints, CoordModePrevious);
	if (This->ownbackstore)
		XDrawLines(This->display, This->xwin.backstore, This->xwin.gc, (XPoint *) points, npoints,
			   CoordModePrevious);

	ON_ALL_SEAMLESS_WINDOWS(seamless_XDrawLines,
				(This, sw->wnd, (XPoint *) points, npoints, sw->xoffset, sw->yoffset));

	RESET_FUNCTION(opcode);
}

void
ui_ellipse(RDPCLIENT * This, uint8 opcode,
	   /* mode */ uint8 fillmode,
	   /* dest */ int x, int y, int cx, int cy,
	   /* brush */ BRUSH * brush, int bgcolour, int fgcolour)
{
	uint8 style, i, ipattern[8];
	Pixmap fill;

	SET_FUNCTION(opcode);

	if (brush)
		style = brush->style;
	else
		style = 0;

	switch (style)
	{
		case 0:	/* Solid */
			SET_FOREGROUND(fgcolour);
			DRAW_ELLIPSE(x, y, cx, cy, fillmode);
			break;

		case 2:	/* Hatch */
			fill = (Pixmap) ui_create_glyph(This, 8, 8,
							hatch_patterns + brush->pattern[0] * 8);
			SET_FOREGROUND(fgcolour);
			SET_BACKGROUND(bgcolour);
			XSetFillStyle(This->display, This->xwin.gc, FillOpaqueStippled);
			XSetStipple(This->display, This->xwin.gc, fill);
			XSetTSOrigin(This->display, This->xwin.gc, brush->xorigin, brush->yorigin);
			DRAW_ELLIPSE(x, y, cx, cy, fillmode);
			XSetFillStyle(This->display, This->xwin.gc, FillSolid);
			XSetTSOrigin(This->display, This->xwin.gc, 0, 0);
			ui_destroy_glyph(This, (HGLYPH) fill);
			break;

		case 3:	/* Pattern */
			for (i = 0; i != 8; i++)
				ipattern[7 - i] = brush->pattern[i];
			fill = (Pixmap) ui_create_glyph(This, 8, 8, ipattern);
			SET_FOREGROUND(bgcolour);
			SET_BACKGROUND(fgcolour);
			XSetFillStyle(This->display, This->xwin.gc, FillOpaqueStippled);
			XSetStipple(This->display, This->xwin.gc, fill);
			XSetTSOrigin(This->display, This->xwin.gc, brush->xorigin, brush->yorigin);
			DRAW_ELLIPSE(x, y, cx, cy, fillmode);
			XSetFillStyle(This->display, This->xwin.gc, FillSolid);
			XSetTSOrigin(This->display, This->xwin.gc, 0, 0);
			ui_destroy_glyph(This, (HGLYPH) fill);
			break;

		default:
			unimpl("brush %d\n", brush->style);
	}

	RESET_FUNCTION(opcode);
}

/* warning, this function only draws on wnd or backstore, not both */
void
ui_draw_glyph(RDPCLIENT * This, int mixmode,
	      /* dest */ int x, int y, int cx, int cy,
	      /* src */ HGLYPH glyph, int srcx, int srcy,
	      int bgcolour, int fgcolour)
{
	SET_FOREGROUND(fgcolour);
	SET_BACKGROUND(bgcolour);

	XSetFillStyle(This->display, This->xwin.gc,
		      (mixmode == MIX_TRANSPARENT) ? FillStippled : FillOpaqueStippled);
	XSetStipple(This->display, This->xwin.gc, (Pixmap) glyph);
	XSetTSOrigin(This->display, This->xwin.gc, x, y);

	FILL_RECTANGLE_BACKSTORE(x, y, cx, cy);

	XSetFillStyle(This->display, This->xwin.gc, FillSolid);
}

#define DO_GLYPH(ttext,idx) \
{\
  glyph = cache_get_font (This, font, ttext[idx]);\
  if (!(flags & TEXT2_IMPLICIT_X))\
  {\
    xyoffset = ttext[++idx];\
    if ((xyoffset & 0x80))\
    {\
      if (flags & TEXT2_VERTICAL)\
        y += ttext[idx+1] | (ttext[idx+2] << 8);\
      else\
        x += ttext[idx+1] | (ttext[idx+2] << 8);\
      idx += 2;\
    }\
    else\
    {\
      if (flags & TEXT2_VERTICAL)\
        y += xyoffset;\
      else\
        x += xyoffset;\
    }\
  }\
  if (glyph != NULL)\
  {\
    x1 = x + glyph->offset;\
    y1 = y + glyph->baseline;\
    XSetStipple(This->display, This->xwin.gc, (Pixmap) glyph->pixmap);\
    XSetTSOrigin(This->display, This->xwin.gc, x1, y1);\
    FILL_RECTANGLE_BACKSTORE(x1, y1, glyph->width, glyph->height);\
    if (flags & TEXT2_IMPLICIT_X)\
      x += glyph->width;\
  }\
}

void
ui_draw_text(RDPCLIENT * This, uint8 font, uint8 flags, uint8 opcode, int mixmode, int x, int y,
	     int clipx, int clipy, int clipcx, int clipcy,
	     int boxx, int boxy, int boxcx, int boxcy, BRUSH * brush,
	     int bgcolour, int fgcolour, uint8 * text, uint8 length)
{
	/* TODO: use brush appropriately */

	FONTGLYPH *glyph;
	int i, j, xyoffset, x1, y1;
	DATABLOB *entry;

	SET_FOREGROUND(bgcolour);

	/* Sometimes, the boxcx value is something really large, like
	   32691. This makes XCopyArea fail with Xvnc. The code below
	   is a quick fix. */
	if (boxx + boxcx > This->width)
		boxcx = This->width - boxx;

	if (boxcx > 1)
	{
		FILL_RECTANGLE_BACKSTORE(boxx, boxy, boxcx, boxcy);
	}
	else if (mixmode == MIX_OPAQUE)
	{
		FILL_RECTANGLE_BACKSTORE(clipx, clipy, clipcx, clipcy);
	}

	SET_FOREGROUND(fgcolour);
	SET_BACKGROUND(bgcolour);
	XSetFillStyle(This->display, This->xwin.gc, FillStippled);

	/* Paint text, character by character */
	for (i = 0; i < length;)
	{
		switch (text[i])
		{
			case 0xff:
				/* At least two bytes needs to follow */
				if (i + 3 > length)
				{
					warning("Skipping short 0xff command:");
					for (j = 0; j < length; j++)
						fprintf(stderr, "%02x ", text[j]);
					fprintf(stderr, "\n");
					i = length = 0;
					break;
				}
				cache_put_text(This, text[i + 1], text, text[i + 2]);
				i += 3;
				length -= i;
				/* this will move pointer from start to first character after FF command */
				text = &(text[i]);
				i = 0;
				break;

			case 0xfe:
				/* At least one byte needs to follow */
				if (i + 2 > length)
				{
					warning("Skipping short 0xfe command:");
					for (j = 0; j < length; j++)
						fprintf(stderr, "%02x ", text[j]);
					fprintf(stderr, "\n");
					i = length = 0;
					break;
				}
				entry = cache_get_text(This, text[i + 1]);
				if (entry->data != NULL)
				{
					if ((((uint8 *) (entry->data))[1] == 0)
					    && (!(flags & TEXT2_IMPLICIT_X)) && (i + 2 < length))
					{
						if (flags & TEXT2_VERTICAL)
							y += text[i + 2];
						else
							x += text[i + 2];
					}
					for (j = 0; j < entry->size; j++)
						DO_GLYPH(((uint8 *) (entry->data)), j);
				}
				if (i + 2 < length)
					i += 3;
				else
					i += 2;
				length -= i;
				/* this will move pointer from start to first character after FE command */
				text = &(text[i]);
				i = 0;
				break;

			default:
				DO_GLYPH(text, i);
				i++;
				break;
		}
	}

	XSetFillStyle(This->display, This->xwin.gc, FillSolid);

	if (This->ownbackstore)
	{
		if (boxcx > 1)
		{
			XCopyArea(This->display, This->xwin.backstore, This->wnd, This->xwin.gc, boxx,
				  boxy, boxcx, boxcy, boxx, boxy);
			ON_ALL_SEAMLESS_WINDOWS(XCopyArea,
						(This->display, This->xwin.backstore, sw->wnd, This->xwin.gc,
						 boxx, boxy,
						 boxcx, boxcy,
						 boxx - sw->xoffset, boxy - sw->yoffset));
		}
		else
		{
			XCopyArea(This->display, This->xwin.backstore, This->wnd, This->xwin.gc, clipx,
				  clipy, clipcx, clipcy, clipx, clipy);
			ON_ALL_SEAMLESS_WINDOWS(XCopyArea,
						(This->display, This->xwin.backstore, sw->wnd, This->xwin.gc,
						 clipx, clipy,
						 clipcx, clipcy, clipx - sw->xoffset,
						 clipy - sw->yoffset));
		}
	}
}

void
ui_desktop_save(RDPCLIENT * This, uint32 offset, int x, int y, int cx, int cy)
{
	Pixmap pix;
	XImage *image;

	if (This->ownbackstore)
	{
		image = XGetImage(This->display, This->xwin.backstore, x, y, cx, cy, AllPlanes, ZPixmap);
	}
	else
	{
		pix = XCreatePixmap(This->display, This->wnd, cx, cy, This->xwin.depth);
		XCopyArea(This->display, This->wnd, pix, This->xwin.gc, x, y, cx, cy, 0, 0);
		image = XGetImage(This->display, pix, 0, 0, cx, cy, AllPlanes, ZPixmap);
		XFreePixmap(This->display, pix);
	}

	offset *= This->xwin.bpp / 8;
	cache_put_desktop(This, offset, cx, cy, image->bytes_per_line, This->xwin.bpp / 8, (uint8 *) image->data);

	XDestroyImage(image);
}

void
ui_desktop_restore(RDPCLIENT * This, uint32 offset, int x, int y, int cx, int cy)
{
	XImage *image;
	uint8 *data;

	offset *= This->xwin.bpp / 8;
	data = cache_get_desktop(This, offset, cx, cy, This->xwin.bpp / 8);
	if (data == NULL)
		return;

	image = XCreateImage(This->display, This->xwin.visual, This->xwin.depth, ZPixmap, 0,
			     (char *) data, cx, cy, BitmapPad(This->display), cx * This->xwin.bpp / 8);

	if (This->ownbackstore)
	{
		XPutImage(This->display, This->xwin.backstore, This->xwin.gc, image, 0, 0, x, y, cx, cy);
		XCopyArea(This->display, This->xwin.backstore, This->wnd, This->xwin.gc, x, y, cx, cy, x, y);
		ON_ALL_SEAMLESS_WINDOWS(XCopyArea,
					(This->display, This->xwin.backstore, sw->wnd, This->xwin.gc,
					 x, y, cx, cy, x - sw->xoffset, y - sw->yoffset));
	}
	else
	{
		XPutImage(This->display, This->wnd, This->xwin.gc, image, 0, 0, x, y, cx, cy);
		ON_ALL_SEAMLESS_WINDOWS(XCopyArea,
					(This->display, This->wnd, sw->wnd, This->xwin.gc, x, y, cx, cy,
					 x - sw->xoffset, y - sw->yoffset));
	}

	XFree(image);
}

/* these do nothing here but are used in uiports */
void
ui_begin_update(RDPCLIENT * This)
{
}

void
ui_end_update(RDPCLIENT * This)
{
}


void
ui_seamless_begin(RDPCLIENT * This, BOOL hidden)
{
	if (!This->seamless_rdp)
		return;

	if (This->xwin.seamless_started)
		return;

	This->xwin.seamless_started = True;
	This->xwin.seamless_hidden = hidden;

	if (!hidden)
		ui_seamless_toggle(This);
}


void
ui_seamless_hide_desktop(RDPCLIENT * This)
{
	if (!This->seamless_rdp)
		return;

	if (!This->xwin.seamless_started)
		return;

	if (This->xwin.seamless_active)
		ui_seamless_toggle(This);

	This->xwin.seamless_hidden = True;
}


void
ui_seamless_unhide_desktop(RDPCLIENT * This)
{
	if (!This->seamless_rdp)
		return;

	if (!This->xwin.seamless_started)
		return;

	This->xwin.seamless_hidden = False;

	ui_seamless_toggle(This);
}


void
ui_seamless_toggle(RDPCLIENT * This)
{
	if (!This->seamless_rdp)
		return;

	if (!This->xwin.seamless_started)
		return;

	if (This->xwin.seamless_hidden)
		return;

	if (This->xwin.seamless_active)
	{
		/* Deactivate */
		while (This->xwin.seamless_windows)
		{
			XDestroyWindow(This->display, This->xwin.seamless_windows->wnd);
			sw_remove_window(This, This->xwin.seamless_windows);
		}
		XMapWindow(This->display, This->wnd);
	}
	else
	{
		/* Activate */
		XUnmapWindow(This->display, This->wnd);
		seamless_send_sync(This);
	}

	This->xwin.seamless_active = !This->xwin.seamless_active;
}


void
ui_seamless_create_window(RDPCLIENT * This, unsigned long id, unsigned long group, unsigned long parent,
			  unsigned long flags)
{
	Window wnd;
	XSetWindowAttributes attribs;
	XClassHint *classhints;
	XSizeHints *sizehints;
	XWMHints *wmhints;
	long input_mask;
	seamless_window *sw, *sw_parent;

	if (!This->xwin.seamless_active)
		return;

	/* Ignore CREATEs for existing windows */
	sw = sw_get_window_by_id(This, id);
	if (sw)
		return;

	get_window_attribs(This, &attribs);
	wnd = XCreateWindow(This->display, RootWindowOfScreen(This->xwin.screen), -1, -1, 1, 1, 0, This->xwin.depth,
			    InputOutput, This->xwin.visual,
			    CWBackPixel | CWBackingStore | CWColormap | CWBorderPixel, &attribs);

	XStoreName(This->display, wnd, "SeamlessRDP");
	ewmh_set_wm_name(This, wnd, "SeamlessRDP");

	mwm_hide_decorations(This, wnd);

	classhints = XAllocClassHint();
	if (classhints != NULL)
	{
		classhints->res_name = "rdesktop";
		classhints->res_class = "SeamlessRDP";
		XSetClassHint(This->display, wnd, classhints);
		XFree(classhints);
	}

	/* WM_NORMAL_HINTS */
	sizehints = XAllocSizeHints();
	if (sizehints != NULL)
	{
		sizehints->flags = USPosition;
		XSetWMNormalHints(This->display, wnd, sizehints);
		XFree(sizehints);
	}

	/* Parent-less transient windows */
	if (parent == 0xFFFFFFFF)
	{
		XSetTransientForHint(This->display, wnd, RootWindowOfScreen(This->xwin.screen));
		/* Some buggy wm:s (kwin) do not handle the above, so fake it
		   using some other hints. */
		ewmh_set_window_popup(This, wnd);
	}
	/* Normal transient windows */
	else if (parent != 0x00000000)
	{
		sw_parent = sw_get_window_by_id(This, parent);
		if (sw_parent)
			XSetTransientForHint(This->display, wnd, sw_parent->wnd);
		else
			warning("ui_seamless_create_window: No parent window 0x%lx\n", parent);
	}

	if (flags & SEAMLESSRDP_CREATE_MODAL)
	{
		/* We do this to support buggy wm:s (*cough* metacity *cough*)
		   somewhat at least */
		if (parent == 0x00000000)
			XSetTransientForHint(This->display, wnd, RootWindowOfScreen(This->xwin.screen));
		ewmh_set_window_modal(This, wnd);
	}

	/* FIXME: Support for Input Context:s */

	get_input_mask(This, &input_mask);
	input_mask |= PropertyChangeMask;

	XSelectInput(This->display, wnd, input_mask);

	/* handle the WM_DELETE_WINDOW protocol. FIXME: When killing a
	   seamless window, we could try to close the window on the
	   serverside, instead of terminating rdesktop */
	XSetWMProtocols(This->display, wnd, &This->xwin.kill_atom, 1);

	sw = xmalloc(sizeof(seamless_window));
	sw->wnd = wnd;
	sw->id = id;
	sw->behind = 0;
	sw->group = sw_find_group(This, group, False);
	sw->group->refcnt++;
	sw->xoffset = 0;
	sw->yoffset = 0;
	sw->width = 0;
	sw->height = 0;
	sw->state = SEAMLESSRDP_NOTYETMAPPED;
	sw->desktop = 0;
	sw->position_timer = xmalloc(sizeof(struct timeval));
	timerclear(sw->position_timer);

	sw->outstanding_position = False;
	sw->outpos_serial = 0;
	sw->outpos_xoffset = sw->outpos_yoffset = 0;
	sw->outpos_width = sw->outpos_height = 0;

	sw->next = This->xwin.seamless_windows;
	This->xwin.seamless_windows = sw;

	/* WM_HINTS */
	wmhints = XAllocWMHints();
	if (wmhints)
	{
		wmhints->flags = WindowGroupHint;
		wmhints->window_group = sw->group->wnd;
		XSetWMHints(This->display, sw->wnd, wmhints);
		XFree(wmhints);
	}
}


void
ui_seamless_destroy_window(RDPCLIENT * This, unsigned long id, unsigned long flags)
{
	seamless_window *sw;

	if (!This->xwin.seamless_active)
		return;

	sw = sw_get_window_by_id(This, id);
	if (!sw)
	{
		warning("ui_seamless_destroy_window: No information for window 0x%lx\n", id);
		return;
	}

	XDestroyWindow(This->display, sw->wnd);
	sw_remove_window(This, sw);
}


void
ui_seamless_destroy_group(RDPCLIENT * This, unsigned long id, unsigned long flags)
{
	seamless_window *sw, *sw_next;

	if (!This->xwin.seamless_active)
		return;

	for (sw = This->xwin.seamless_windows; sw; sw = sw_next)
	{
		sw_next = sw->next;

		if (sw->group->id == id)
		{
			XDestroyWindow(This->display, sw->wnd);
			sw_remove_window(This, sw);
		}
	}
}


void
ui_seamless_move_window(RDPCLIENT * This, unsigned long id, int x, int y, int width, int height, unsigned long flags)
{
	seamless_window *sw;

	if (!This->xwin.seamless_active)
		return;

	sw = sw_get_window_by_id(This, id);
	if (!sw)
	{
		warning("ui_seamless_move_window: No information for window 0x%lx\n", id);
		return;
	}

	/* We ignore server updates until it has handled our request. */
	if (sw->outstanding_position)
		return;

	if (!width || !height)
		/* X11 windows must be at least 1x1 */
		return;

	sw->xoffset = x;
	sw->yoffset = y;
	sw->width = width;
	sw->height = height;

	/* If we move the window in a maximized state, then KDE won't
	   accept restoration */
	switch (sw->state)
	{
		case SEAMLESSRDP_MINIMIZED:
		case SEAMLESSRDP_MAXIMIZED:
			return;
	}

	/* FIXME: Perhaps use ewmh_net_moveresize_window instead */
	XMoveResizeWindow(This->display, sw->wnd, sw->xoffset, sw->yoffset, sw->width, sw->height);
}


void
ui_seamless_restack_window(RDPCLIENT * This, unsigned long id, unsigned long behind, unsigned long flags)
{
	seamless_window *sw;

	if (!This->xwin.seamless_active)
		return;

	sw = sw_get_window_by_id(This, id);
	if (!sw)
	{
		warning("ui_seamless_restack_window: No information for window 0x%lx\n", id);
		return;
	}

	if (behind)
	{
		seamless_window *sw_behind;
		Window wnds[2];

		sw_behind = sw_get_window_by_id(This, behind);
		if (!sw_behind)
		{
			warning("ui_seamless_restack_window: No information for window 0x%lx\n",
				behind);
			return;
		}

		wnds[1] = sw_behind->wnd;
		wnds[0] = sw->wnd;

		XRestackWindows(This->display, wnds, 2);
	}
	else
	{
		XRaiseWindow(This->display, sw->wnd);
	}

	sw_restack_window(This, sw, behind);
}


void
ui_seamless_settitle(RDPCLIENT * This, unsigned long id, const char *title, unsigned long flags)
{
	seamless_window *sw;

	if (!This->xwin.seamless_active)
		return;

	sw = sw_get_window_by_id(This, id);
	if (!sw)
	{
		warning("ui_seamless_settitle: No information for window 0x%lx\n", id);
		return;
	}

	/* FIXME: Might want to convert the name for non-EWMH WMs */
	XStoreName(This->display, sw->wnd, title);
	ewmh_set_wm_name(This, sw->wnd, title);
}


void
ui_seamless_setstate(RDPCLIENT * This, unsigned long id, unsigned int state, unsigned long flags)
{
	seamless_window *sw;

	if (!This->xwin.seamless_active)
		return;

	sw = sw_get_window_by_id(This, id);
	if (!sw)
	{
		warning("ui_seamless_setstate: No information for window 0x%lx\n", id);
		return;
	}

	switch (state)
	{
		case SEAMLESSRDP_NORMAL:
		case SEAMLESSRDP_MAXIMIZED:
			ewmh_change_state(This, sw->wnd, state);
			XMapWindow(This->display, sw->wnd);
			break;
		case SEAMLESSRDP_MINIMIZED:
			/* EWMH says: "if an Application asks to toggle _NET_WM_STATE_HIDDEN
			   the Window Manager should probably just ignore the request, since
			   _NET_WM_STATE_HIDDEN is a function of some other aspect of the window
			   such as minimization, rather than an independent state." Besides,
			   XIconifyWindow is easier. */
			if (sw->state == SEAMLESSRDP_NOTYETMAPPED)
			{
				XWMHints *hints;
				hints = XGetWMHints(This->display, sw->wnd);
				if (hints)
				{
					hints->flags |= StateHint;
					hints->initial_state = IconicState;
					XSetWMHints(This->display, sw->wnd, hints);
					XFree(hints);
				}
				XMapWindow(This->display, sw->wnd);
			}
			else
				XIconifyWindow(This->display, sw->wnd, DefaultScreen(This->display));
			break;
		default:
			warning("SeamlessRDP: Invalid state %d\n", state);
			break;
	}

	sw->state = state;
}


void
ui_seamless_syncbegin(RDPCLIENT * This, unsigned long flags)
{
	if (!This->xwin.seamless_active)
		return;

	/* Destroy all seamless windows */
	while (This->xwin.seamless_windows)
	{
		XDestroyWindow(This->display, This->xwin.seamless_windows->wnd);
		sw_remove_window(This, This->xwin.seamless_windows);
	}
}


void
ui_seamless_ack(RDPCLIENT * This, unsigned int serial)
{
	seamless_window *sw;
	for (sw = This->xwin.seamless_windows; sw; sw = sw->next)
	{
		if (sw->outstanding_position && (sw->outpos_serial == serial))
		{
			sw->xoffset = sw->outpos_xoffset;
			sw->yoffset = sw->outpos_yoffset;
			sw->width = sw->outpos_width;
			sw->height = sw->outpos_height;
			sw->outstanding_position = False;

			/* Do a complete redraw of the window as part of the
			   completion of the move. This is to remove any
			   artifacts caused by our lack of synchronization. */
			XCopyArea(This->display, This->xwin.backstore,
				  sw->wnd, This->xwin.gc,
				  sw->xoffset, sw->yoffset, sw->width, sw->height, 0, 0);

			break;
		}
	}
}
