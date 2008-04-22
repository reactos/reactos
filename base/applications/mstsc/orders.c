/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   RDP order processing
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

#include <precomp.h>


extern uint8 *g_next_packet;
static RDP_ORDER_STATE g_order_state;
extern BOOL g_use_rdp5;

/* Read field indicating which parameters are present */
static void
rdp_in_present(STREAM s, uint32 * present, uint8 flags, int size)
{
	uint8 bits;
	int i;

	if (flags & RDP_ORDER_SMALL)
	{
		size--;
	}

	if (flags & RDP_ORDER_TINY)
	{
		if (size < 2)
			size = 0;
		else
			size -= 2;
	}

	*present = 0;
	for (i = 0; i < size; i++)
	{
		in_uint8(s, bits);
		*present |= bits << (i * 8);
	}
}

/* Read a co-ordinate (16-bit, or 8-bit delta) */
static void
rdp_in_coord(STREAM s, sint16 * coord, BOOL delta)
{
	sint8 change;

	if (delta)
	{
		in_uint8(s, change);
		*coord += change;
	}
	else
	{
		in_uint16_le(s, *coord);
	}
}

/* Parse a delta co-ordinate in polyline/polygon order form */
static int
parse_delta(uint8 * buffer, int *offset)
{
	int value = buffer[(*offset)++];
	int two_byte = value & 0x80;

	if (value & 0x40)	/* sign bit */
		value |= ~0x3f;
	else
		value &= 0x3f;

	if (two_byte)
		value = (value << 8) | buffer[(*offset)++];

	return value;
}

/* Read a colour entry */
static void
rdp_in_colour(STREAM s, uint32 * colour)
{
	uint32 i;
	in_uint8(s, i);
	*colour = i;
	in_uint8(s, i);
	*colour |= i << 8;
	in_uint8(s, i);
	*colour |= i << 16;
}

/* Parse bounds information */
static BOOL
rdp_parse_bounds(STREAM s, BOUNDS * bounds)
{
	uint8 present;

	in_uint8(s, present);

	if (present & 1)
		rdp_in_coord(s, &bounds->left, False);
	else if (present & 16)
		rdp_in_coord(s, &bounds->left, True);

	if (present & 2)
		rdp_in_coord(s, &bounds->top, False);
	else if (present & 32)
		rdp_in_coord(s, &bounds->top, True);

	if (present & 4)
		rdp_in_coord(s, &bounds->right, False);
	else if (present & 64)
		rdp_in_coord(s, &bounds->right, True);

	if (present & 8)
		rdp_in_coord(s, &bounds->bottom, False);
	else if (present & 128)
		rdp_in_coord(s, &bounds->bottom, True);

	return s_check(s);
}

/* Parse a pen */
static BOOL
rdp_parse_pen(STREAM s, PEN * pen, uint32 present)
{
	if (present & 1)
		in_uint8(s, pen->style);

	if (present & 2)
		in_uint8(s, pen->width);

	if (present & 4)
		rdp_in_colour(s, &pen->colour);

	return s_check(s);
}

/* Parse a brush */
static BOOL
rdp_parse_brush(STREAM s, BRUSH * brush, uint32 present)
{
	if (present & 1)
		in_uint8(s, brush->xorigin);

	if (present & 2)
		in_uint8(s, brush->yorigin);

	if (present & 4)
		in_uint8(s, brush->style);

	if (present & 8)
		in_uint8(s, brush->pattern[0]);

	if (present & 16)
		in_uint8a(s, &brush->pattern[1], 7);

	return s_check(s);
}

/* Process a destination blt order */
static void
process_destblt(STREAM s, DESTBLT_ORDER * os, uint32 present, BOOL delta)
{
	if (present & 0x01)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x02)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x04)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x08)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x10)
		in_uint8(s, os->opcode);

	DEBUG(("DESTBLT(op=0x%x,x=%d,y=%d,cx=%d,cy=%d)\n",
	       os->opcode, os->x, os->y, os->cx, os->cy));

	ui_destblt(ROP2_S(os->opcode), os->x, os->y, os->cx, os->cy);
}

/* Process a pattern blt order */
static void
process_patblt(STREAM s, PATBLT_ORDER * os, uint32 present, BOOL delta)
{
	if (present & 0x0001)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x0002)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x0004)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x0008)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x0010)
		in_uint8(s, os->opcode);

	if (present & 0x0020)
		rdp_in_colour(s, &os->bgcolour);

	if (present & 0x0040)
		rdp_in_colour(s, &os->fgcolour);

	rdp_parse_brush(s, &os->brush, present >> 7);

	DEBUG(("PATBLT(op=0x%x,x=%d,y=%d,cx=%d,cy=%d,bs=%d,bg=0x%x,fg=0x%x)\n", os->opcode, os->x,
	       os->y, os->cx, os->cy, os->brush.style, os->bgcolour, os->fgcolour));

	ui_patblt(ROP2_P(os->opcode), os->x, os->y, os->cx, os->cy,
		  &os->brush, os->bgcolour, os->fgcolour);
}

/* Process a screen blt order */
static void
process_screenblt(STREAM s, SCREENBLT_ORDER * os, uint32 present, BOOL delta)
{
	if (present & 0x0001)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x0002)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x0004)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x0008)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x0010)
		in_uint8(s, os->opcode);

	if (present & 0x0020)
		rdp_in_coord(s, &os->srcx, delta);

	if (present & 0x0040)
		rdp_in_coord(s, &os->srcy, delta);

	DEBUG(("SCREENBLT(op=0x%x,x=%d,y=%d,cx=%d,cy=%d,srcx=%d,srcy=%d)\n",
	       os->opcode, os->x, os->y, os->cx, os->cy, os->srcx, os->srcy));

	ui_screenblt(ROP2_S(os->opcode), os->x, os->y, os->cx, os->cy, os->srcx, os->srcy);
}

/* Process a line order */
static void
process_line(STREAM s, LINE_ORDER * os, uint32 present, BOOL delta)
{
	if (present & 0x0001)
		in_uint16_le(s, os->mixmode);

	if (present & 0x0002)
		rdp_in_coord(s, &os->startx, delta);

	if (present & 0x0004)
		rdp_in_coord(s, &os->starty, delta);

	if (present & 0x0008)
		rdp_in_coord(s, &os->endx, delta);

	if (present & 0x0010)
		rdp_in_coord(s, &os->endy, delta);

	if (present & 0x0020)
		rdp_in_colour(s, &os->bgcolour);

	if (present & 0x0040)
		in_uint8(s, os->opcode);

	rdp_parse_pen(s, &os->pen, present >> 7);

	DEBUG(("LINE(op=0x%x,sx=%d,sy=%d,dx=%d,dy=%d,fg=0x%x)\n",
	       os->opcode, os->startx, os->starty, os->endx, os->endy, os->pen.colour));

	if (os->opcode < 0x01 || os->opcode > 0x10)
	{
		error("bad ROP2 0x%x\n", os->opcode);
		return;
	}

	ui_line(ROP_MINUS_1(os->opcode), os->startx, os->starty, os->endx, os->endy, &os->pen);
}

/* Process an opaque rectangle order */
static void
process_rect(STREAM s, RECT_ORDER * os, uint32 present, BOOL delta)
{
	uint32 i;
	if (present & 0x01)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x02)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x04)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x08)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x10)
	{
		in_uint8(s, i);
		os->colour = (os->colour & 0xffffff00) | i;
	}

	if (present & 0x20)
	{
		in_uint8(s, i);
		os->colour = (os->colour & 0xffff00ff) | (i << 8);
	}

	if (present & 0x40)
	{
		in_uint8(s, i);
		os->colour = (os->colour & 0xff00ffff) | (i << 16);
	}

	DEBUG(("RECT(x=%d,y=%d,cx=%d,cy=%d,fg=0x%x)\n", os->x, os->y, os->cx, os->cy, os->colour));

	ui_rect(os->x, os->y, os->cx, os->cy, os->colour);
}

/* Process a desktop save order */
static void
process_desksave(STREAM s, DESKSAVE_ORDER * os, uint32 present, BOOL delta)
{
	int width, height;

	if (present & 0x01)
		in_uint32_le(s, os->offset);

	if (present & 0x02)
		rdp_in_coord(s, &os->left, delta);

	if (present & 0x04)
		rdp_in_coord(s, &os->top, delta);

	if (present & 0x08)
		rdp_in_coord(s, &os->right, delta);

	if (present & 0x10)
		rdp_in_coord(s, &os->bottom, delta);

	if (present & 0x20)
		in_uint8(s, os->action);

	DEBUG(("DESKSAVE(l=%d,t=%d,r=%d,b=%d,off=%d,op=%d)\n",
	       os->left, os->top, os->right, os->bottom, os->offset, os->action));

	width = os->right - os->left + 1;
	height = os->bottom - os->top + 1;

	if (os->action == 0)
		ui_desktop_save(os->offset, os->left, os->top, width, height);
	else
		ui_desktop_restore(os->offset, os->left, os->top, width, height);
}

/* Process a memory blt order */
static void
process_memblt(STREAM s, MEMBLT_ORDER * os, uint32 present, BOOL delta)
{
	HBITMAP bitmap;

	if (present & 0x0001)
	{
		in_uint8(s, os->cache_id);
		in_uint8(s, os->colour_table);
	}

	if (present & 0x0002)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x0004)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x0008)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x0010)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x0020)
		in_uint8(s, os->opcode);

	if (present & 0x0040)
		rdp_in_coord(s, &os->srcx, delta);

	if (present & 0x0080)
		rdp_in_coord(s, &os->srcy, delta);

	if (present & 0x0100)
		in_uint16_le(s, os->cache_idx);

	DEBUG(("MEMBLT(op=0x%x,x=%d,y=%d,cx=%d,cy=%d,id=%d,idx=%d)\n",
	       os->opcode, os->x, os->y, os->cx, os->cy, os->cache_id, os->cache_idx));

	bitmap = cache_get_bitmap(os->cache_id, os->cache_idx);
	if (bitmap == NULL)
		return;

	ui_memblt(ROP2_S(os->opcode), os->x, os->y, os->cx, os->cy, bitmap, os->srcx, os->srcy);
}

/* Process a 3-way blt order */
static void
process_triblt(STREAM s, TRIBLT_ORDER * os, uint32 present, BOOL delta)
{
	HBITMAP bitmap;

	if (present & 0x000001)
	{
		in_uint8(s, os->cache_id);
		in_uint8(s, os->colour_table);
	}

	if (present & 0x000002)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x000004)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x000008)
		rdp_in_coord(s, &os->cx, delta);

	if (present & 0x000010)
		rdp_in_coord(s, &os->cy, delta);

	if (present & 0x000020)
		in_uint8(s, os->opcode);

	if (present & 0x000040)
		rdp_in_coord(s, &os->srcx, delta);

	if (present & 0x000080)
		rdp_in_coord(s, &os->srcy, delta);

	if (present & 0x000100)
		rdp_in_colour(s, &os->bgcolour);

	if (present & 0x000200)
		rdp_in_colour(s, &os->fgcolour);

	rdp_parse_brush(s, &os->brush, present >> 10);

	if (present & 0x008000)
		in_uint16_le(s, os->cache_idx);

	if (present & 0x010000)
		in_uint16_le(s, os->unknown);

	DEBUG(("TRIBLT(op=0x%x,x=%d,y=%d,cx=%d,cy=%d,id=%d,idx=%d,bs=%d,bg=0x%x,fg=0x%x)\n",
	       os->opcode, os->x, os->y, os->cx, os->cy, os->cache_id, os->cache_idx,
	       os->brush.style, os->bgcolour, os->fgcolour));

	bitmap = cache_get_bitmap(os->cache_id, os->cache_idx);
	if (bitmap == NULL)
		return;

	ui_triblt(os->opcode, os->x, os->y, os->cx, os->cy,
		  bitmap, os->srcx, os->srcy, &os->brush, os->bgcolour, os->fgcolour);
}

/* Process a polygon order */
static void
process_polygon(STREAM s, POLYGON_ORDER * os, uint32 present, BOOL delta)
{
	int index, data, next;
	uint8 flags = 0;
	POINT *points;

	if (present & 0x01)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x02)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x04)
		in_uint8(s, os->opcode);

	if (present & 0x08)
		in_uint8(s, os->fillmode);

	if (present & 0x10)
		rdp_in_colour(s, &os->fgcolour);

	if (present & 0x20)
		in_uint8(s, os->npoints);

	if (present & 0x40)
	{
		in_uint8(s, os->datasize);
		in_uint8a(s, os->data, os->datasize);
	}

	DEBUG(("POLYGON(x=%d,y=%d,op=0x%x,fm=%d,fg=0x%x,n=%d,sz=%d)\n",
	       os->x, os->y, os->opcode, os->fillmode, os->fgcolour, os->npoints, os->datasize));

	DEBUG(("Data: "));

	for (index = 0; index < os->datasize; index++)
		DEBUG(("%02x ", os->data[index]));

	DEBUG(("\n"));

	if (os->opcode < 0x01 || os->opcode > 0x10)
	{
		error("bad ROP2 0x%x\n", os->opcode);
		return;
	}

	points = (POINT *) xmalloc((os->npoints + 1) * sizeof(POINT));
	memset(points, 0, (os->npoints + 1) * sizeof(POINT));

	points[0].x = os->x;
	points[0].y = os->y;

	index = 0;
	data = ((os->npoints - 1) / 4) + 1;
	for (next = 1; (next <= os->npoints) && (next < 256) && (data < os->datasize); next++)
	{
		if ((next - 1) % 4 == 0)
			flags = os->data[index++];

		if (~flags & 0x80)
			points[next].x = parse_delta(os->data, &data);

		if (~flags & 0x40)
			points[next].y = parse_delta(os->data, &data);

		flags <<= 2;
	}

	if (next - 1 == os->npoints)
		ui_polygon(ROP_MINUS_1(os->opcode), os->fillmode, points, os->npoints + 1, NULL, 0,
			   os->fgcolour);
	else
		error("polygon parse error\n");

	xfree(points);
}

/* Process a polygon2 order */
static void
process_polygon2(STREAM s, POLYGON2_ORDER * os, uint32 present, BOOL delta)
{
	int index, data, next;
	uint8 flags = 0;
	POINT *points;

	if (present & 0x0001)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x0002)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x0004)
		in_uint8(s, os->opcode);

	if (present & 0x0008)
		in_uint8(s, os->fillmode);

	if (present & 0x0010)
		rdp_in_colour(s, &os->bgcolour);

	if (present & 0x0020)
		rdp_in_colour(s, &os->fgcolour);

	rdp_parse_brush(s, &os->brush, present >> 6);

	if (present & 0x0800)
		in_uint8(s, os->npoints);

	if (present & 0x1000)
	{
		in_uint8(s, os->datasize);
		in_uint8a(s, os->data, os->datasize);
	}

	DEBUG(("POLYGON2(x=%d,y=%d,op=0x%x,fm=%d,bs=%d,bg=0x%x,fg=0x%x,n=%d,sz=%d)\n",
	       os->x, os->y, os->opcode, os->fillmode, os->brush.style, os->bgcolour, os->fgcolour,
	       os->npoints, os->datasize));

	DEBUG(("Data: "));

	for (index = 0; index < os->datasize; index++)
		DEBUG(("%02x ", os->data[index]));

	DEBUG(("\n"));

	if (os->opcode < 0x01 || os->opcode > 0x10)
	{
		error("bad ROP2 0x%x\n", os->opcode);
		return;
	}

	points = (POINT *) xmalloc((os->npoints + 1) * sizeof(POINT));
	memset(points, 0, (os->npoints + 1) * sizeof(POINT));

	points[0].x = os->x;
	points[0].y = os->y;

	index = 0;
	data = ((os->npoints - 1) / 4) + 1;
	for (next = 1; (next <= os->npoints) && (next < 256) && (data < os->datasize); next++)
	{
		if ((next - 1) % 4 == 0)
			flags = os->data[index++];

		if (~flags & 0x80)
			points[next].x = parse_delta(os->data, &data);

		if (~flags & 0x40)
			points[next].y = parse_delta(os->data, &data);

		flags <<= 2;
	}

	if (next - 1 == os->npoints)
		ui_polygon(ROP_MINUS_1(os->opcode), os->fillmode, points, os->npoints + 1,
			   &os->brush, os->bgcolour, os->fgcolour);
	else
		error("polygon2 parse error\n");

	xfree(points);
}

/* Process a polyline order */
static void
process_polyline(STREAM s, POLYLINE_ORDER * os, uint32 present, BOOL delta)
{
	int index, next, data;
	uint8 flags = 0;
	PEN pen;
	POINT *points;

	if (present & 0x01)
		rdp_in_coord(s, &os->x, delta);

	if (present & 0x02)
		rdp_in_coord(s, &os->y, delta);

	if (present & 0x04)
		in_uint8(s, os->opcode);

	if (present & 0x10)
		rdp_in_colour(s, &os->fgcolour);

	if (present & 0x20)
		in_uint8(s, os->lines);

	if (present & 0x40)
	{
		in_uint8(s, os->datasize);
		in_uint8a(s, os->data, os->datasize);
	}

	DEBUG(("POLYLINE(x=%d,y=%d,op=0x%x,fg=0x%x,n=%d,sz=%d)\n",
	       os->x, os->y, os->opcode, os->fgcolour, os->lines, os->datasize));

	DEBUG(("Data: "));

	for (index = 0; index < os->datasize; index++)
		DEBUG(("%02x ", os->data[index]));

	DEBUG(("\n"));

	if (os->opcode < 0x01 || os->opcode > 0x10)
	{
		error("bad ROP2 0x%x\n", os->opcode);
		return;
	}

	points = (POINT *) xmalloc((os->lines + 1) * sizeof(POINT));
	memset(points, 0, (os->lines + 1) * sizeof(POINT));

	points[0].x = os->x;
	points[0].y = os->y;
	pen.style = pen.width = 0;
	pen.colour = os->fgcolour;

	index = 0;
	data = ((os->lines - 1) / 4) + 1;
	for (next = 1; (next <= os->lines) && (data < os->datasize); next++)
	{
		if ((next - 1) % 4 == 0)
			flags = os->data[index++];

		if (~flags & 0x80)
			points[next].x = parse_delta(os->data, &data);

		if (~flags & 0x40)
			points[next].y = parse_delta(os->data, &data);

		flags <<= 2;
	}

	if (next - 1 == os->lines)
		ui_polyline(ROP_MINUS_1(os->opcode), points, os->lines + 1, &pen);
	else
		error("polyline parse error\n");

	xfree(points);
}

/* Process an ellipse order */
static void
process_ellipse(STREAM s, ELLIPSE_ORDER * os, uint32 present, BOOL delta)
{
	if (present & 0x01)
		rdp_in_coord(s, &os->left, delta);

	if (present & 0x02)
		rdp_in_coord(s, &os->top, delta);

	if (present & 0x04)
		rdp_in_coord(s, &os->right, delta);

	if (present & 0x08)
		rdp_in_coord(s, &os->bottom, delta);

	if (present & 0x10)
		in_uint8(s, os->opcode);

	if (present & 0x20)
		in_uint8(s, os->fillmode);

	if (present & 0x40)
		rdp_in_colour(s, &os->fgcolour);

	DEBUG(("ELLIPSE(l=%d,t=%d,r=%d,b=%d,op=0x%x,fm=%d,fg=0x%x)\n", os->left, os->top,
	       os->right, os->bottom, os->opcode, os->fillmode, os->fgcolour));

	ui_ellipse(ROP_MINUS_1(os->opcode), os->fillmode, os->left, os->top, os->right - os->left,
		   os->bottom - os->top, NULL, 0, os->fgcolour);
}

/* Process an ellipse2 order */
static void
process_ellipse2(STREAM s, ELLIPSE2_ORDER * os, uint32 present, BOOL delta)
{
	if (present & 0x0001)
		rdp_in_coord(s, &os->left, delta);

	if (present & 0x0002)
		rdp_in_coord(s, &os->top, delta);

	if (present & 0x0004)
		rdp_in_coord(s, &os->right, delta);

	if (present & 0x0008)
		rdp_in_coord(s, &os->bottom, delta);

	if (present & 0x0010)
		in_uint8(s, os->opcode);

	if (present & 0x0020)
		in_uint8(s, os->fillmode);

	if (present & 0x0040)
		rdp_in_colour(s, &os->bgcolour);

	if (present & 0x0080)
		rdp_in_colour(s, &os->fgcolour);

	rdp_parse_brush(s, &os->brush, present >> 8);

	DEBUG(("ELLIPSE2(l=%d,t=%d,r=%d,b=%d,op=0x%x,fm=%d,bs=%d,bg=0x%x,fg=0x%x)\n",
	       os->left, os->top, os->right, os->bottom, os->opcode, os->fillmode, os->brush.style,
	       os->bgcolour, os->fgcolour));

	ui_ellipse(ROP_MINUS_1(os->opcode), os->fillmode, os->left, os->top, os->right - os->left,
		   os->bottom - os->top, &os->brush, os->bgcolour, os->fgcolour);
}

/* Process a text order */
static void
process_text2(STREAM s, TEXT2_ORDER * os, uint32 present, BOOL delta)
{
	int i;

	if (present & 0x000001)
		in_uint8(s, os->font);

	if (present & 0x000002)
		in_uint8(s, os->flags);

	if (present & 0x000004)
		in_uint8(s, os->opcode);

	if (present & 0x000008)
		in_uint8(s, os->mixmode);

	if (present & 0x000010)
		rdp_in_colour(s, &os->fgcolour);

	if (present & 0x000020)
		rdp_in_colour(s, &os->bgcolour);

	if (present & 0x000040)
		in_uint16_le(s, os->clipleft);

	if (present & 0x000080)
		in_uint16_le(s, os->cliptop);

	if (present & 0x000100)
		in_uint16_le(s, os->clipright);

	if (present & 0x000200)
		in_uint16_le(s, os->clipbottom);

	if (present & 0x000400)
		in_uint16_le(s, os->boxleft);

	if (present & 0x000800)
		in_uint16_le(s, os->boxtop);

	if (present & 0x001000)
		in_uint16_le(s, os->boxright);

	if (present & 0x002000)
		in_uint16_le(s, os->boxbottom);

	rdp_parse_brush(s, &os->brush, present >> 14);

	if (present & 0x080000)
		in_uint16_le(s, os->x);

	if (present & 0x100000)
		in_uint16_le(s, os->y);

	if (present & 0x200000)
	{
		in_uint8(s, os->length);
		in_uint8a(s, os->text, os->length);
	}

	DEBUG(("TEXT2(x=%d,y=%d,cl=%d,ct=%d,cr=%d,cb=%d,bl=%d,bt=%d,br=%d,bb=%d,bs=%d,bg=0x%x,fg=0x%x,font=%d,fl=0x%x,op=0x%x,mix=%d,n=%d)\n", os->x, os->y, os->clipleft, os->cliptop, os->clipright, os->clipbottom, os->boxleft, os->boxtop, os->boxright, os->boxbottom, os->brush.style, os->bgcolour, os->fgcolour, os->font, os->flags, os->opcode, os->mixmode, os->length));

	DEBUG(("Text: "));

	for (i = 0; i < os->length; i++)
		DEBUG(("%02x ", os->text[i]));

	DEBUG(("\n"));

	ui_draw_text(os->font, os->flags, ROP_MINUS_1(os->opcode), os->mixmode, os->x, os->y,
		     os->clipleft, os->cliptop, os->clipright - os->clipleft,
		     os->clipbottom - os->cliptop, os->boxleft, os->boxtop,
		     os->boxright - os->boxleft, os->boxbottom - os->boxtop,
		     &os->brush, os->bgcolour, os->fgcolour, os->text, os->length);
}

/* Process a raw bitmap cache order */
static void
process_raw_bmpcache(STREAM s)
{
	HBITMAP bitmap;
	uint16 cache_idx, bufsize;
	uint8 cache_id, width, height, bpp, Bpp;
	uint8 *data, *inverted;
	int y;

	in_uint8(s, cache_id);
	in_uint8s(s, 1);	/* pad */
	in_uint8(s, width);
	in_uint8(s, height);
	in_uint8(s, bpp);
	Bpp = (bpp + 7) / 8;
	in_uint16_le(s, bufsize);
	in_uint16_le(s, cache_idx);
	in_uint8p(s, data, bufsize);

	DEBUG(("RAW_BMPCACHE(cx=%d,cy=%d,id=%d,idx=%d)\n", width, height, cache_id, cache_idx));
	inverted = (uint8 *) xmalloc(width * height * Bpp);
	for (y = 0; y < height; y++)
	{
		memcpy(&inverted[(height - y - 1) * (width * Bpp)], &data[y * (width * Bpp)],
		       width * Bpp);
	}

	bitmap = ui_create_bitmap(width, height, inverted);
	xfree(inverted);
	cache_put_bitmap(cache_id, cache_idx, bitmap);
}

/* Process a bitmap cache order */
static void
process_bmpcache(STREAM s)
{
	HBITMAP bitmap;
	uint16 cache_idx, size;
	uint8 cache_id, width, height, bpp, Bpp;
	uint8 *data, *bmpdata;
	uint16 bufsize, pad2, row_size, final_size;
	uint8 pad1;

	pad2 = row_size = final_size = 0xffff;	/* Shut the compiler up */

	in_uint8(s, cache_id);
	in_uint8(s, pad1);	/* pad */
	in_uint8(s, width);
	in_uint8(s, height);
	in_uint8(s, bpp);
	Bpp = (bpp + 7) / 8;
	in_uint16_le(s, bufsize);	/* bufsize */
	in_uint16_le(s, cache_idx);

	if (g_use_rdp5)
	{
		size = bufsize;
	}
	else
	{

		/* Begin compressedBitmapData */
		in_uint16_le(s, pad2);	/* pad */
		in_uint16_le(s, size);
		/*      in_uint8s(s, 4);  *//* row_size, final_size */
		in_uint16_le(s, row_size);
		in_uint16_le(s, final_size);

	}
	in_uint8p(s, data, size);

	DEBUG(("BMPCACHE(cx=%d,cy=%d,id=%d,idx=%d,bpp=%d,size=%d,pad1=%d,bufsize=%d,pad2=%d,rs=%d,fs=%d)\n", width, height, cache_id, cache_idx, bpp, size, pad1, bufsize, pad2, row_size, final_size));

	bmpdata = (uint8 *) xmalloc(width * height * Bpp);

	if (bitmap_decompress(bmpdata, width, height, data, size, Bpp))
	{
		bitmap = ui_create_bitmap(width, height, bmpdata);
		cache_put_bitmap(cache_id, cache_idx, bitmap);
	}
	else
	{
		DEBUG(("Failed to decompress bitmap data\n"));
	}

	xfree(bmpdata);
}

/* Process a bitmap cache v2 order */
static void
process_bmpcache2(STREAM s, uint16 flags, BOOL compressed)
{
	HBITMAP bitmap;
	int y;
	uint8 cache_id, cache_idx_low, width, height, Bpp;
	uint16 cache_idx, bufsize;
	uint8 *data, *bmpdata, *bitmap_id;

	bitmap_id = NULL;	/* prevent compiler warning */
	cache_id = flags & ID_MASK;
	Bpp = ((flags & MODE_MASK) >> MODE_SHIFT) - 2;

	if (flags & PERSIST)
	{
		in_uint8p(s, bitmap_id, 8);
	}

	if (flags & SQUARE)
	{
		in_uint8(s, width);
		height = width;
	}
	else
	{
		in_uint8(s, width);
		in_uint8(s, height);
	}

	in_uint16_be(s, bufsize);
	bufsize &= BUFSIZE_MASK;
	in_uint8(s, cache_idx);

	if (cache_idx & LONG_FORMAT)
	{
		in_uint8(s, cache_idx_low);
		cache_idx = ((cache_idx ^ LONG_FORMAT) << 8) + cache_idx_low;
	}

	in_uint8p(s, data, bufsize);

	DEBUG(("BMPCACHE2(compr=%d,flags=%x,cx=%d,cy=%d,id=%d,idx=%d,Bpp=%d,bs=%d)\n",
	       compressed, flags, width, height, cache_id, cache_idx, Bpp, bufsize));

	bmpdata = (uint8 *) xmalloc(width * height * Bpp);

	if (compressed)
	{
		if (!bitmap_decompress(bmpdata, width, height, data, bufsize, Bpp))
		{
			DEBUG(("Failed to decompress bitmap data\n"));
			xfree(bmpdata);
			return;
		}
	}
	else
	{
		for (y = 0; y < height; y++)
			memcpy(&bmpdata[(height - y - 1) * (width * Bpp)],
			       &data[y * (width * Bpp)], width * Bpp);
	}

	bitmap = ui_create_bitmap(width, height, bmpdata);

	if (bitmap)
	{
		cache_put_bitmap(cache_id, cache_idx, bitmap);
		if (flags & PERSIST)
			pstcache_save_bitmap(cache_id, cache_idx, bitmap_id, width, height,
					     (uint16) (width * height * Bpp), bmpdata);
	}
	else
	{
		DEBUG(("process_bmpcache2: ui_create_bitmap failed\n"));
	}

	xfree(bmpdata);
}

/* Process a colourmap cache order */
static void
process_colcache(STREAM s)
{
	COLOURENTRY *entry;
	COLOURMAP map;
	RD_HCOLOURMAP hmap;
	uint8 cache_id;
	int i;

	in_uint8(s, cache_id);
	in_uint16_le(s, map.ncolours);

	map.colours = (COLOURENTRY *) xmalloc(sizeof(COLOURENTRY) * map.ncolours);

	for (i = 0; i < map.ncolours; i++)
	{
		entry = &map.colours[i];
		in_uint8(s, entry->blue);
		in_uint8(s, entry->green);
		in_uint8(s, entry->red);
		in_uint8s(s, 1);	/* pad */
	}

	DEBUG(("COLCACHE(id=%d,n=%d)\n", cache_id, map.ncolours));

	hmap = ui_create_colourmap(&map);

	if (cache_id)
		ui_set_colourmap(hmap);

	xfree(map.colours);
}

/* Process a font cache order */
static void
process_fontcache(STREAM s)
{
	RD_HGLYPH bitmap;
	uint8 font, nglyphs;
	uint16 character, offset, baseline, width, height;
	int i, datasize;
	uint8 *data;

	in_uint8(s, font);
	in_uint8(s, nglyphs);

	DEBUG(("FONTCACHE(font=%d,n=%d)\n", font, nglyphs));

	for (i = 0; i < nglyphs; i++)
	{
		in_uint16_le(s, character);
		in_uint16_le(s, offset);
		in_uint16_le(s, baseline);
		in_uint16_le(s, width);
		in_uint16_le(s, height);

		datasize = (height * ((width + 7) / 8) + 3) & ~3;
		in_uint8p(s, data, datasize);

		bitmap = ui_create_glyph(width, height, data);
		cache_put_font(font, character, offset, baseline, width, height, bitmap);
	}
}

/* Process a secondary order */
static void
process_secondary_order(STREAM s)
{
	/* The length isn't calculated correctly by the server.
	 * For very compact orders the length becomes negative
	 * so a signed integer must be used. */
	uint16 length;
	uint16 flags;
	uint8 type;
	uint8 *next_order;

	in_uint16_le(s, length);
	in_uint16_le(s, flags);	/* used by bmpcache2 */
	in_uint8(s, type);

	next_order = s->p + (sint16) length + 7;

	switch (type)
	{
		case RDP_ORDER_RAW_BMPCACHE:
			process_raw_bmpcache(s);
			break;

		case RDP_ORDER_COLCACHE:
			process_colcache(s);
			break;

		case RDP_ORDER_BMPCACHE:
			process_bmpcache(s);
			break;

		case RDP_ORDER_FONTCACHE:
			process_fontcache(s);
			break;

		case RDP_ORDER_RAW_BMPCACHE2:
			process_bmpcache2(s, flags, False);	/* uncompressed */
			break;

		case RDP_ORDER_BMPCACHE2:
			process_bmpcache2(s, flags, True);	/* compressed */
			break;

		default:
			unimpl("secondary order %d\n", type);
	}

	s->p = next_order;
}

/* Process an order PDU */
void
process_orders(STREAM s, uint16 num_orders)
{
	RDP_ORDER_STATE *os = &g_order_state;
	uint32 present;
	uint8 order_flags;
	int size, processed = 0;
	BOOL delta;

	while (processed < num_orders)
	{
		in_uint8(s, order_flags);

		if (!(order_flags & RDP_ORDER_STANDARD))
		{
			error("order parsing failed\n");
			break;
		}

		if (order_flags & RDP_ORDER_SECONDARY)
		{
			process_secondary_order(s);
		}
		else
		{
			if (order_flags & RDP_ORDER_CHANGE)
			{
				in_uint8(s, os->order_type);
			}

			switch (os->order_type)
			{
				case RDP_ORDER_TRIBLT:
				case RDP_ORDER_TEXT2:
					size = 3;
					break;

				case RDP_ORDER_PATBLT:
				case RDP_ORDER_MEMBLT:
				case RDP_ORDER_LINE:
				case RDP_ORDER_POLYGON2:
				case RDP_ORDER_ELLIPSE2:
					size = 2;
					break;

				default:
					size = 1;
			}

			rdp_in_present(s, &present, order_flags, size);

			if (order_flags & RDP_ORDER_BOUNDS)
			{
				if (!(order_flags & RDP_ORDER_LASTBOUNDS))
					rdp_parse_bounds(s, &os->bounds);

				ui_set_clip(os->bounds.left,
					    os->bounds.top,
					    os->bounds.right -
					    os->bounds.left + 1,
					    os->bounds.bottom - os->bounds.top + 1);
			}

			delta = order_flags & RDP_ORDER_DELTA;

			switch (os->order_type)
			{
				case RDP_ORDER_DESTBLT:
					process_destblt(s, &os->destblt, present, delta);
					break;

				case RDP_ORDER_PATBLT:
					process_patblt(s, &os->patblt, present, delta);
					break;

				case RDP_ORDER_SCREENBLT:
					process_screenblt(s, &os->screenblt, present, delta);
					break;

				case RDP_ORDER_LINE:
					process_line(s, &os->line, present, delta);
					break;

				case RDP_ORDER_RECT:
					process_rect(s, &os->rect, present, delta);
					break;

				case RDP_ORDER_DESKSAVE:
					process_desksave(s, &os->desksave, present, delta);
					break;

				case RDP_ORDER_MEMBLT:
					process_memblt(s, &os->memblt, present, delta);
					break;

				case RDP_ORDER_TRIBLT:
					process_triblt(s, &os->triblt, present, delta);
					break;

				case RDP_ORDER_POLYGON:
					process_polygon(s, &os->polygon, present, delta);
					break;

				case RDP_ORDER_POLYGON2:
					process_polygon2(s, &os->polygon2, present, delta);
					break;

				case RDP_ORDER_POLYLINE:
					process_polyline(s, &os->polyline, present, delta);
					break;

				case RDP_ORDER_ELLIPSE:
					process_ellipse(s, &os->ellipse, present, delta);
					break;

				case RDP_ORDER_ELLIPSE2:
					process_ellipse2(s, &os->ellipse2, present, delta);
					break;

				case RDP_ORDER_TEXT2:
					process_text2(s, &os->text2, present, delta);
					break;

				default:
					unimpl("order %d\n", os->order_type);
					return;
			}

			if (order_flags & RDP_ORDER_BOUNDS)
				ui_reset_clip();
		}

		processed++;
	}
#if 0
	/* not true when RDP_COMPRESSION is set */
	if (s->p != g_next_packet)
		error("%d bytes remaining\n", (int) (g_next_packet - s->p));
#endif

}

/* Reset order state */
void
reset_order_state(void)
{
	memset(&g_order_state, 0, sizeof(g_order_state));
	g_order_state.order_type = RDP_ORDER_PATBLT;
}
