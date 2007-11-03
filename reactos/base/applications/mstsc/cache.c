/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Cache routines
   Copyright (C) Matthew Chapman 1999-2005
   Copyright (C) Jeroen Meijer 2005

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

#include "rdesktop.h"

/* BITMAP CACHE */
extern int g_pstcache_fd[];

#define NUM_ELEMENTS(array) (sizeof(array) / sizeof(array[0]))
#define IS_PERSISTENT(id) (g_pstcache_fd[id] > 0)
#define TO_TOP -1
#define NOT_SET -1
#define IS_SET(idx) (idx >= 0)

/*
 * TODO: Test for optimal value of BUMP_COUNT. TO_TOP gives lowest cpu utilisation but using
 * a positive value will hopefully result in less frequently used bitmaps having a greater chance
 * of being evicted from the cache, and therby reducing the need to load bitmaps from disk.
 * (Jeroen)
 */
#define BUMP_COUNT 40

struct bmpcache_entry
{
	RD_HBITMAP bitmap;
	sint16 previous;
	sint16 next;
};

static struct bmpcache_entry g_bmpcache[3][0xa00];
static RD_HBITMAP g_volatile_bc[3];

static int g_bmpcache_lru[3] = { NOT_SET, NOT_SET, NOT_SET };
static int g_bmpcache_mru[3] = { NOT_SET, NOT_SET, NOT_SET };
static int g_bmpcache_count[3];

/* Setup the bitmap cache lru/mru linked list */
void
cache_rebuild_bmpcache_linked_list(uint8 id, sint16 * idx, int count)
{
	int n = count, c = 0;
	sint16 n_idx;

	/* find top, skip evicted bitmaps */
	while (--n >= 0 && g_bmpcache[id][idx[n]].bitmap == NULL);
	if (n < 0)
	{
		g_bmpcache_mru[id] = g_bmpcache_lru[id] = NOT_SET;
		return;
	}

	g_bmpcache_mru[id] = idx[n];
	g_bmpcache[id][idx[n]].next = NOT_SET;
	n_idx = idx[n];
	c++;

	/* link list */
	while (n >= 0)
	{
		/* skip evicted bitmaps */
		while (--n >= 0 && g_bmpcache[id][idx[n]].bitmap == NULL);

		if (n < 0)
			break;

		g_bmpcache[id][n_idx].previous = idx[n];
		g_bmpcache[id][idx[n]].next = n_idx;
		n_idx = idx[n];
		c++;
	}

	g_bmpcache[id][n_idx].previous = NOT_SET;
	g_bmpcache_lru[id] = n_idx;

	if (c != g_bmpcache_count[id])
	{
		error("Oops. %d in bitmap cache linked list, %d in ui cache...\n", c,
		      g_bmpcache_count[id]);
		exit(1);
	}
}

/* Move a bitmap to a new position in the linked list. */
void
cache_bump_bitmap(uint8 id, uint16 idx, int bump)
{
	int p_idx, n_idx, n;

	if (!IS_PERSISTENT(id))
		return;

	if (g_bmpcache_mru[id] == idx)
		return;

	DEBUG_RDP5(("bump bitmap: id=%d, idx=%d, bump=%d\n", id, idx, bump));

	n_idx = g_bmpcache[id][idx].next;
	p_idx = g_bmpcache[id][idx].previous;

	if (IS_SET(n_idx))
	{
		/* remove */
		--g_bmpcache_count[id];
		if (IS_SET(p_idx))
			g_bmpcache[id][p_idx].next = n_idx;
		else
			g_bmpcache_lru[id] = n_idx;
		if (IS_SET(n_idx))
			g_bmpcache[id][n_idx].previous = p_idx;
		else
			g_bmpcache_mru[id] = p_idx;
	}
	else
	{
		p_idx = NOT_SET;
		n_idx = g_bmpcache_lru[id];
	}

	if (bump >= 0)
	{
		for (n = 0; n < bump && IS_SET(n_idx); n++)
		{
			p_idx = n_idx;
			n_idx = g_bmpcache[id][p_idx].next;
		}
	}
	else
	{
		p_idx = g_bmpcache_mru[id];
		n_idx = NOT_SET;
	}

	/* insert */
	++g_bmpcache_count[id];
	g_bmpcache[id][idx].previous = p_idx;
	g_bmpcache[id][idx].next = n_idx;

	if (p_idx >= 0)
		g_bmpcache[id][p_idx].next = idx;
	else
		g_bmpcache_lru[id] = idx;

	if (n_idx >= 0)
		g_bmpcache[id][n_idx].previous = idx;
	else
		g_bmpcache_mru[id] = idx;
}

/* Evict the least-recently used bitmap from the cache */
void
cache_evict_bitmap(uint8 id)
{
	uint16 idx;
	int n_idx;

	if (!IS_PERSISTENT(id))
		return;

	idx = g_bmpcache_lru[id];
	n_idx = g_bmpcache[id][idx].next;
	DEBUG_RDP5(("evict bitmap: id=%d idx=%d n_idx=%d bmp=0x%x\n", id, idx, n_idx,
		    g_bmpcache[id][idx].bitmap));

	ui_destroy_bitmap(g_bmpcache[id][idx].bitmap);
	--g_bmpcache_count[id];
	g_bmpcache[id][idx].bitmap = 0;

	g_bmpcache_lru[id] = n_idx;
	g_bmpcache[id][n_idx].previous = NOT_SET;

	pstcache_touch_bitmap(id, idx, 0);
}

/* Retrieve a bitmap from the cache */
RD_HBITMAP
cache_get_bitmap(uint8 id, uint16 idx)
{
	if ((id < NUM_ELEMENTS(g_bmpcache)) && (idx < NUM_ELEMENTS(g_bmpcache[0])))
	{
		if (g_bmpcache[id][idx].bitmap || pstcache_load_bitmap(id, idx))
		{
			if (IS_PERSISTENT(id))
				cache_bump_bitmap(id, idx, BUMP_COUNT);

			return g_bmpcache[id][idx].bitmap;
		}
	}
	else if ((id < NUM_ELEMENTS(g_volatile_bc)) && (idx == 0x7fff))
	{
		return g_volatile_bc[id];
	}

	error("get bitmap %d:%d\n", id, idx);
	return NULL;
}

/* Store a bitmap in the cache */
void
cache_put_bitmap(uint8 id, uint16 idx, RD_HBITMAP bitmap)
{
	RD_HBITMAP old;

	if ((id < NUM_ELEMENTS(g_bmpcache)) && (idx < NUM_ELEMENTS(g_bmpcache[0])))
	{
		old = g_bmpcache[id][idx].bitmap;
		if (old != NULL)
			ui_destroy_bitmap(old);
		g_bmpcache[id][idx].bitmap = bitmap;

		if (IS_PERSISTENT(id))
		{
			if (old == NULL)
				g_bmpcache[id][idx].previous = g_bmpcache[id][idx].next = NOT_SET;

			cache_bump_bitmap(id, idx, TO_TOP);
			if (g_bmpcache_count[id] > BMPCACHE2_C2_CELLS)
				cache_evict_bitmap(id);
		}
	}
	else if ((id < NUM_ELEMENTS(g_volatile_bc)) && (idx == 0x7fff))
	{
		old = g_volatile_bc[id];
		if (old != NULL)
			ui_destroy_bitmap(old);
		g_volatile_bc[id] = bitmap;
	}
	else
	{
		error("put bitmap %d:%d\n", id, idx);
	}
}

/* Updates the persistent bitmap cache MRU information on exit */
void
cache_save_state(void)
{
	uint32 id = 0, t = 0;
	int idx;

	for (id = 0; id < NUM_ELEMENTS(g_bmpcache); id++)
		if (IS_PERSISTENT(id))
		{
			DEBUG_RDP5(("Saving cache state for bitmap cache %d...", id));
			idx = g_bmpcache_lru[id];
			while (idx >= 0)
			{
				pstcache_touch_bitmap((uint8) id, (uint16) idx, ++t);
				idx = g_bmpcache[id][idx].next;
			}
			DEBUG_RDP5((" %d stamps written.\n", t));
		}
}


/* FONT CACHE */
static FONTGLYPH g_fontcache[12][256];

/* Retrieve a glyph from the font cache */
FONTGLYPH *
cache_get_font(uint8 font, uint16 character)
{
	FONTGLYPH *glyph;

	if ((font < NUM_ELEMENTS(g_fontcache)) && (character < NUM_ELEMENTS(g_fontcache[0])))
	{
		glyph = &g_fontcache[font][character];
		if (glyph->pixmap != NULL)
			return glyph;
	}

	error("get font %d:%d\n", font, character);
	return NULL;
}

/* Store a glyph in the font cache */
void
cache_put_font(uint8 font, uint16 character, uint16 offset,
	       uint16 baseline, uint16 width, uint16 height, RD_HGLYPH pixmap)
{
	FONTGLYPH *glyph;

	if ((font < NUM_ELEMENTS(g_fontcache)) && (character < NUM_ELEMENTS(g_fontcache[0])))
	{
		glyph = &g_fontcache[font][character];
		if (glyph->pixmap != NULL)
			ui_destroy_glyph(glyph->pixmap);

		glyph->offset = offset;
		glyph->baseline = baseline;
		glyph->width = width;
		glyph->height = height;
		glyph->pixmap = pixmap;
	}
	else
	{
		error("put font %d:%d\n", font, character);
	}
}


/* TEXT CACHE */
static DATABLOB g_textcache[256];

/* Retrieve a text item from the cache */
DATABLOB *
cache_get_text(uint8 cache_id)
{
	DATABLOB *text;

	text = &g_textcache[cache_id];
	return text;
}

/* Store a text item in the cache */
void
cache_put_text(uint8 cache_id, void *data, int length)
{
	DATABLOB *text;

	text = &g_textcache[cache_id];
	if (text->data != NULL)
		xfree(text->data);
	text->data = xmalloc(length);
	text->size = length;
	memcpy(text->data, data, length);
}


/* DESKTOP CACHE */
static uint8 g_deskcache[0x38400 * 4];

/* Retrieve desktop data from the cache */
uint8 *
cache_get_desktop(uint32 offset, int cx, int cy, int bytes_per_pixel)
{
	int length = cx * cy * bytes_per_pixel;

	if (offset > sizeof(g_deskcache))
		offset = 0;

	if ((offset + length) <= sizeof(g_deskcache))
	{
		return &g_deskcache[offset];
	}

	error("get desktop %d:%d\n", offset, length);
	return NULL;
}

/* Store desktop data in the cache */
void
cache_put_desktop(uint32 offset, int cx, int cy, int scanline, int bytes_per_pixel, uint8 * data)
{
	int length = cx * cy * bytes_per_pixel;

	if (offset > sizeof(g_deskcache))
		offset = 0;

	if ((offset + length) <= sizeof(g_deskcache))
	{
		cx *= bytes_per_pixel;
		while (cy--)
		{
			memcpy(&g_deskcache[offset], data, cx);
			data += scanline;
			offset += cx;
		}
	}
	else
	{
		error("put desktop %d:%d\n", offset, length);
	}
}


/* CURSOR CACHE */
static RD_HCURSOR g_cursorcache[0x20];

/* Retrieve cursor from cache */
RD_HCURSOR
cache_get_cursor(uint16 cache_idx)
{
	RD_HCURSOR cursor;

	if (cache_idx < NUM_ELEMENTS(g_cursorcache))
	{
		cursor = g_cursorcache[cache_idx];
		if (cursor != NULL)
			return cursor;
	}

	error("get cursor %d\n", cache_idx);
	return NULL;
}

/* Store cursor in cache */
void
cache_put_cursor(uint16 cache_idx, RD_HCURSOR cursor)
{
	RD_HCURSOR old;

	if (cache_idx < NUM_ELEMENTS(g_cursorcache))
	{
		old = g_cursorcache[cache_idx];
		if (old != NULL)
			ui_destroy_cursor(old);

		g_cursorcache[cache_idx] = cursor;
	}
	else
	{
		error("put cursor %d\n", cache_idx);
	}
}
