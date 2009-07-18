/*
 * Server-side region objects. Based on the X11 implementation.
 *
 * Copyright 1993, 1994, 1995, 2004 Alexandre Julliard
 * Modifications and additions: Copyright 1998 Huw Davies
 *					  1999 Alex Korobka
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Note:
 *  This is a simplified version of the code, without all the explanations.
 *  Check the equivalent GDI code to make sense of it.
 */

/************************************************************************

Copyright (c) 1987, 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

************************************************************************/

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "request.h"
#include "user.h"

struct region
{
    int size;
    int num_rects;
    rectangle_t *rects;
    rectangle_t extents;
};


#define RGN_DEFAULT_RECTS 2

#define EXTENTCHECK(r1, r2) \
    ((r1)->right > (r2)->left && \
    (r1)->left < (r2)->right && \
    (r1)->bottom > (r2)->top && \
    (r1)->top < (r2)->bottom)

typedef int (*overlap_func_t)( struct region *reg, const rectangle_t *r1, const rectangle_t *r1End,
                               const rectangle_t *r2, const rectangle_t *r2End, int top, int bottom );
typedef int (*non_overlap_func_t)( struct region *reg, const rectangle_t *r,
                                   const rectangle_t *rEnd, int top, int bottom );

static const rectangle_t empty_rect;  /* all-zero rectangle for empty regions */

/* add a rectangle to a region */
static inline rectangle_t *add_rect( struct region *reg )
{
    if (reg->num_rects >= reg->size - 1)
    {
        rectangle_t *new_rect = realloc( reg->rects, 2 * sizeof(rectangle_t) * reg->size );
        if (!new_rect)
        {
            set_error( STATUS_NO_MEMORY );
            return NULL;
        }
        reg->rects = new_rect;
        reg->size *= 2;
    }
    return reg->rects + reg->num_rects++;
}

/* make sure all the rectangles are valid and that the region is properly y-x-banded */
static inline int validate_rectangles( const rectangle_t *rects, unsigned int nb_rects )
{
    const rectangle_t *ptr, *end;

    for (ptr = rects, end = rects + nb_rects; ptr < end; ptr++)
    {
        if (ptr->left >= ptr->right || ptr->top >= ptr->bottom) return 0;  /* empty rectangle */
        if (ptr == end - 1) break;
        if (ptr[0].top == ptr[1].top)  /* same band */
        {
            if (ptr[0].bottom != ptr[1].bottom) return 0;  /* not same y extent */
            if (ptr[0].right >= ptr[1].left) return 0;  /* not properly x ordered */
        }
        else  /* new band */
        {
            if (ptr[0].bottom > ptr[1].top) return 0;  /* not properly y ordered */
        }
    }
    return 1;
}

/* attempt to merge the rects in the current band with those in the */
/* previous one. Used only by region_op. */
static int coalesce_region( struct region *pReg, int prevStart, int curStart )
{
    int curNumRects;
    rectangle_t *pRegEnd = &pReg->rects[pReg->num_rects];
    rectangle_t *pPrevRect = &pReg->rects[prevStart];
    rectangle_t *pCurRect = &pReg->rects[curStart];
    int prevNumRects = curStart - prevStart;
    int bandtop = pCurRect->top;

    for (curNumRects = 0;
         (pCurRect != pRegEnd) && (pCurRect->top == bandtop);
         curNumRects++)
    {
        pCurRect++;
    }

    if (pCurRect != pRegEnd)
    {
        pRegEnd--;
        while (pRegEnd[-1].top == pRegEnd->top) pRegEnd--;
        curStart = pRegEnd - pReg->rects;
        pRegEnd = pReg->rects + pReg->num_rects;
    }

    if ((curNumRects == prevNumRects) && (curNumRects != 0))
    {
        pCurRect -= curNumRects;
        if (pPrevRect->bottom == pCurRect->top)
        {
            do
            {
                if ((pPrevRect->left != pCurRect->left) ||
                    (pPrevRect->right != pCurRect->right)) return curStart;
                pPrevRect++;
                pCurRect++;
                prevNumRects -= 1;
            } while (prevNumRects != 0);

            pReg->num_rects -= curNumRects;
            pCurRect -= curNumRects;
            pPrevRect -= curNumRects;

            do
            {
                pPrevRect->bottom = pCurRect->bottom;
                pPrevRect++;
                pCurRect++;
                curNumRects -= 1;
            } while (curNumRects != 0);

            if (pCurRect == pRegEnd) curStart = prevStart;
            else do { *pPrevRect++ = *pCurRect++; } while (pCurRect != pRegEnd);

        }
    }
    return curStart;
}

/* apply an operation to two regions */
/* check the GDI version of the code for explanations */
static int region_op( struct region *newReg, const struct region *reg1, const struct region *reg2,
                      overlap_func_t overlap_func,
                      non_overlap_func_t non_overlap1_func,
                      non_overlap_func_t non_overlap2_func )
{
    int ybot, ytop, top, bot, prevBand, curBand;
    const rectangle_t *r1BandEnd, *r2BandEnd;

    const rectangle_t *r1 = reg1->rects;
    const rectangle_t *r2 = reg2->rects;
    const rectangle_t *r1End = r1 + reg1->num_rects;
    const rectangle_t *r2End = r2 + reg2->num_rects;

    rectangle_t *new_rects, *old_rects = newReg->rects;
    int new_size, ret = 0;

    new_size = max( reg1->num_rects, reg2->num_rects ) * 2;
    if (!(new_rects = mem_alloc( new_size * sizeof(*newReg->rects) ))) return 0;

    newReg->size = new_size;
    newReg->rects = new_rects;
    newReg->num_rects = 0;

    if (reg1->extents.top < reg2->extents.top)
        ybot = reg1->extents.top;
    else
        ybot = reg2->extents.top;

    prevBand = 0;

    do
    {
        curBand = newReg->num_rects;

        r1BandEnd = r1;
        while ((r1BandEnd != r1End) && (r1BandEnd->top == r1->top)) r1BandEnd++;

        r2BandEnd = r2;
        while ((r2BandEnd != r2End) && (r2BandEnd->top == r2->top)) r2BandEnd++;

        if (r1->top < r2->top)
        {
            top = max(r1->top,ybot);
            bot = min(r1->bottom,r2->top);

            if ((top != bot) && non_overlap1_func)
            {
                if (!non_overlap1_func( newReg, r1, r1BandEnd, top, bot )) goto done;
            }

            ytop = r2->top;
        }
        else if (r2->top < r1->top)
        {
            top = max(r2->top,ybot);
            bot = min(r2->bottom,r1->top);

            if ((top != bot) && non_overlap2_func)
            {
                if (!non_overlap2_func( newReg, r2, r2BandEnd, top, bot )) goto done;
            }

            ytop = r1->top;
        }
        else
        {
            ytop = r1->top;
        }

        if (newReg->num_rects != curBand)
            prevBand = coalesce_region(newReg, prevBand, curBand);

        ybot = min(r1->bottom, r2->bottom);
        curBand = newReg->num_rects;
        if (ybot > ytop)
        {
            if (!overlap_func( newReg, r1, r1BandEnd, r2, r2BandEnd, ytop, ybot )) goto done;
        }

        if (newReg->num_rects != curBand)
            prevBand = coalesce_region(newReg, prevBand, curBand);

        if (r1->bottom == ybot) r1 = r1BandEnd;
        if (r2->bottom == ybot) r2 = r2BandEnd;
    } while ((r1 != r1End) && (r2 != r2End));

    curBand = newReg->num_rects;
    if (r1 != r1End)
    {
        if (non_overlap1_func)
        {
            do
            {
                r1BandEnd = r1;
                while ((r1BandEnd < r1End) && (r1BandEnd->top == r1->top)) r1BandEnd++;
                if (!non_overlap1_func( newReg, r1, r1BandEnd, max(r1->top,ybot), r1->bottom ))
                    goto done;
                r1 = r1BandEnd;
            } while (r1 != r1End);
        }
    }
    else if ((r2 != r2End) && non_overlap2_func)
    {
        do
        {
            r2BandEnd = r2;
            while ((r2BandEnd < r2End) && (r2BandEnd->top == r2->top)) r2BandEnd++;
            if (!non_overlap2_func( newReg, r2, r2BandEnd, max(r2->top,ybot), r2->bottom ))
                goto done;
            r2 = r2BandEnd;
        } while (r2 != r2End);
    }

    if (newReg->num_rects != curBand) coalesce_region(newReg, prevBand, curBand);

    if ((newReg->num_rects < (newReg->size / 2)) && (newReg->size > 2))
    {
        new_size = max( newReg->num_rects, RGN_DEFAULT_RECTS );
        if ((new_rects = realloc( newReg->rects, sizeof(*newReg->rects) * new_size )))
        {
            newReg->rects = new_rects;
            newReg->size = new_size;
        }
    }
    ret = 1;
done:
    free( old_rects );
    return ret;
}

/* recalculate the extents of a region */
static void set_region_extents( struct region *region )
{
    rectangle_t *pRect, *pRectEnd;

    if (region->num_rects == 0)
    {
        region->extents.left = 0;
        region->extents.top = 0;
        region->extents.right = 0;
        region->extents.bottom = 0;
        return;
    }

    pRect = region->rects;
    pRectEnd = &pRect[region->num_rects - 1];

    region->extents.left = pRect->left;
    region->extents.top = pRect->top;
    region->extents.right = pRectEnd->right;
    region->extents.bottom = pRectEnd->bottom;

    while (pRect <= pRectEnd)
    {
        if (pRect->left < region->extents.left) region->extents.left = pRect->left;
        if (pRect->right > region->extents.right) region->extents.right = pRect->right;
        pRect++;
    }
}

/* handle an overlapping band for intersect_region */
static int intersect_overlapping( struct region *pReg,
                                  const rectangle_t *r1, const rectangle_t *r1End,
                                  const rectangle_t *r2, const rectangle_t *r2End,
                                  int top, int bottom )

{
    int left, right;

    while ((r1 != r1End) && (r2 != r2End))
    {
        left = max(r1->left, r2->left);
        right = min(r1->right, r2->right);

        if (left < right)
        {
            rectangle_t *rect = add_rect( pReg );
            if (!rect) return 0;
            rect->left = left;
            rect->top = top;
            rect->right = right;
            rect->bottom = bottom;
        }

        if (r1->right < r2->right) r1++;
        else if (r2->right < r1->right) r2++;
        else
        {
            r1++;
            r2++;
        }
    }
    return 1;
}

/* handle a non-overlapping band for subtract_region */
static int subtract_non_overlapping( struct region *pReg, const rectangle_t *r,
                                  const rectangle_t *rEnd, int top, int bottom )
{
    while (r != rEnd)
    {
        rectangle_t *rect = add_rect( pReg );
        if (!rect) return 0;
        rect->left = r->left;
        rect->top = top;
        rect->right = r->right;
        rect->bottom = bottom;
        r++;
    }
    return 1;
}

/* handle an overlapping band for subtract_region */
static int subtract_overlapping( struct region *pReg,
                                 const rectangle_t *r1, const rectangle_t *r1End,
                                 const rectangle_t *r2, const rectangle_t *r2End,
                                 int top, int bottom )
{
    int left = r1->left;

    while ((r1 != r1End) && (r2 != r2End))
    {
        if (r2->right <= left) r2++;
        else if (r2->left <= left)
        {
            left = r2->right;
            if (left >= r1->right)
            {
                r1++;
                if (r1 != r1End)
                    left = r1->left;
            }
            else r2++;
        }
        else if (r2->left < r1->right)
        {
            rectangle_t *rect = add_rect( pReg );
            if (!rect) return 0;
            rect->left = left;
            rect->top = top;
            rect->right = r2->left;
            rect->bottom = bottom;
            left = r2->right;
            if (left >= r1->right)
            {
                r1++;
                if (r1 != r1End)
                    left = r1->left;
            }
            else r2++;
        }
        else
        {
            if (r1->right > left)
            {
                rectangle_t *rect = add_rect( pReg );
                if (!rect) return 0;
                rect->left = left;
                rect->top = top;
                rect->right = r1->right;
                rect->bottom = bottom;
            }
            r1++;
            left = r1->left;
        }
    }

    while (r1 != r1End)
    {
        rectangle_t *rect = add_rect( pReg );
        if (!rect) return 0;
        rect->left = left;
        rect->top = top;
        rect->right = r1->right;
        rect->bottom = bottom;
        r1++;
        if (r1 != r1End) left = r1->left;
    }
    return 1;
}

/* handle a non-overlapping band for union_region */
static int union_non_overlapping( struct region *pReg, const rectangle_t *r,
                                  const rectangle_t *rEnd, int top, int bottom )
{
    while (r != rEnd)
    {
        rectangle_t *rect = add_rect( pReg );
        if (!rect) return 0;
        rect->left = r->left;
        rect->top = top;
        rect->right = r->right;
        rect->bottom = bottom;
        r++;
    }
    return 1;
}

/* handle an overlapping band for union_region */
static int union_overlapping( struct region *pReg,
                              const rectangle_t *r1, const rectangle_t *r1End,
                              const rectangle_t *r2, const rectangle_t *r2End,
                              int top, int bottom )
{
#define MERGERECT(r) \
    if ((pReg->num_rects != 0) &&  \
        (pReg->rects[pReg->num_rects-1].top == top) &&  \
        (pReg->rects[pReg->num_rects-1].bottom == bottom) &&  \
        (pReg->rects[pReg->num_rects-1].right >= r->left))  \
    {  \
        if (pReg->rects[pReg->num_rects-1].right < r->right)  \
        {  \
            pReg->rects[pReg->num_rects-1].right = r->right;  \
        }  \
    }  \
    else  \
    {  \
        rectangle_t *rect = add_rect( pReg ); \
        if (!rect) return 0; \
        rect->top = top;  \
        rect->bottom = bottom;  \
        rect->left = r->left;  \
        rect->right = r->right;  \
    }  \
    r++;

    while ((r1 != r1End) && (r2 != r2End))
    {
        if (r1->left < r2->left)
        {
            MERGERECT(r1);
        }
        else
        {
            MERGERECT(r2);
        }
    }

    if (r1 != r1End)
    {
        do
        {
            MERGERECT(r1);
        } while (r1 != r1End);
    }
    else while (r2 != r2End)
    {
        MERGERECT(r2);
    }
    return 1;
#undef MERGERECT
}


/* create an empty region */
struct region *create_empty_region(void)
{
    struct region *region;

    if (!(region = mem_alloc( sizeof(*region) ))) return NULL;
    if (!(region->rects = mem_alloc( RGN_DEFAULT_RECTS * sizeof(*region->rects) )))
    {
        free( region );
        return NULL;
    }
    region->size = RGN_DEFAULT_RECTS;
    region->num_rects = 0;
    region->extents.left = 0;
    region->extents.top = 0;
    region->extents.right = 0;
    region->extents.bottom = 0;
    return region;
}

/* create a region from request data */
struct region *create_region_from_req_data( const void *data, data_size_t size )
{
    unsigned int alloc_rects;
    struct region *region;
    const rectangle_t *rects = data;
    int nb_rects = size / sizeof(rectangle_t);

    /* special case: empty region can be specified by a single all-zero rectangle */
    if (nb_rects == 1 && !memcmp( rects, &empty_rect, sizeof(empty_rect) )) nb_rects = 0;

    if (!validate_rectangles( rects, nb_rects ))
    {
        set_error( STATUS_INVALID_PARAMETER );
        return NULL;
    }

    if (!(region = mem_alloc( sizeof(*region) ))) return NULL;

    alloc_rects = max( nb_rects, RGN_DEFAULT_RECTS );
    if (!(region->rects = mem_alloc( alloc_rects * sizeof(*region->rects) )))
    {
        free( region );
        return NULL;
    }
    region->size = alloc_rects;
    region->num_rects = nb_rects;
    memcpy( region->rects, rects, nb_rects * sizeof(*rects) );
    set_region_extents( region );
    return region;
}

/* free a region */
void free_region( struct region *region )
{
    free( region->rects );
    free( region );
}

/* set region to a simple rectangle */
void set_region_rect( struct region *region, const rectangle_t *rect )
{
    if (rect->left < rect->right && rect->top < rect->bottom)
    {
        region->num_rects = 1;
        region->rects[0] = region->extents = *rect;
    }
    else
    {
        region->num_rects = 0;
        region->extents.left = 0;
        region->extents.top = 0;
        region->extents.right = 0;
        region->extents.bottom = 0;
    }
}

/* retrieve the region data for sending to the client */
rectangle_t *get_region_data( const struct region *region, data_size_t max_size, data_size_t *total_size )
{
    const rectangle_t *data = region->rects;

    if (!(*total_size = region->num_rects * sizeof(rectangle_t)))
    {
        /* return a single empty rect for empty regions */
        *total_size = sizeof(empty_rect);
        data = &empty_rect;
    }
    if (max_size >= *total_size) return memdup( data, *total_size );
    set_error( STATUS_BUFFER_OVERFLOW );
    return NULL;
}

/* retrieve the region data for sending to the client and free the region at the same time */
rectangle_t *get_region_data_and_free( struct region *region, data_size_t max_size, data_size_t *total_size )
{
    rectangle_t *ret = region->rects;

    if (!(*total_size = region->num_rects * sizeof(rectangle_t)))
    {
        /* return a single empty rect for empty regions */
        *total_size = sizeof(empty_rect);
        if (max_size >= sizeof(empty_rect))
        {
            ret = memdup( &empty_rect, sizeof(empty_rect) );
            free( region->rects );
        }
    }

    if (max_size < *total_size)
    {
        free( region->rects );
        set_error( STATUS_BUFFER_OVERFLOW );
        ret = NULL;
    }
    free( region );
    return ret;
}

/* check if a given region is empty */
int is_region_empty( const struct region *region )
{
    return region->num_rects == 0;
}


/* get the extents rect of a region */
void get_region_extents( const struct region *region, rectangle_t *rect )
{
    *rect = region->extents;
}

/* add an offset to a region */
void offset_region( struct region *region, int x, int y )
{
    rectangle_t *rect, *end;

    if (!region->num_rects) return;
    for (rect = region->rects, end = rect + region->num_rects; rect < end; rect++)
    {
        rect->left += x;
        rect->right += x;
        rect->top += y;
        rect->bottom += y;
    }
    region->extents.left += x;
    region->extents.right += x;
    region->extents.top += y;
    region->extents.bottom += y;
}

/* make a copy of a region; returns dst or NULL on error */
struct region *copy_region( struct region *dst, const struct region *src )
{
    if (dst == src) return dst;

    if (dst->size < src->num_rects)
    {
        rectangle_t *rect = realloc( dst->rects, src->num_rects * sizeof(*rect) );
        if (!rect)
        {
            set_error( STATUS_NO_MEMORY );
            return NULL;
        }
        dst->rects = rect;
        dst->size = src->num_rects;
    }
    dst->num_rects = src->num_rects;
    dst->extents = src->extents;
    memcpy( dst->rects, src->rects, src->num_rects * sizeof(*dst->rects) );
    return dst;
}

/* compute the intersection of two regions into dst, which can be one of the source regions */
struct region *intersect_region( struct region *dst, const struct region *src1,
                                 const struct region *src2 )
{
    if (!src1->num_rects || !src2->num_rects || !EXTENTCHECK(&src1->extents, &src2->extents))
    {
        dst->num_rects = 0;
        dst->extents.left = 0;
        dst->extents.top = 0;
        dst->extents.right = 0;
        dst->extents.bottom = 0;
        return dst;
    }
    if (!region_op( dst, src1, src2, intersect_overlapping, NULL, NULL )) return NULL;
    set_region_extents( dst );
    return dst;
}

/* compute the subtraction of two regions into dst, which can be one of the source regions */
struct region *subtract_region( struct region *dst, const struct region *src1,
                                const struct region *src2 )
{
    if (!src1->num_rects || !src2->num_rects || !EXTENTCHECK(&src1->extents, &src2->extents))
        return copy_region( dst, src1 );

    if (!region_op( dst, src1, src2, subtract_overlapping,
                    subtract_non_overlapping, NULL )) return NULL;
    set_region_extents( dst );
    return dst;
}

/* compute the union of two regions into dst, which can be one of the source regions */
struct region *union_region( struct region *dst, const struct region *src1,
                             const struct region *src2 )
{
    if (src1 == src2) return copy_region( dst, src1 );
    if (!src1->num_rects) return copy_region( dst, src2 );
    if (!src2->num_rects) return copy_region( dst, src1 );

    if ((src1->num_rects == 1) &&
        (src1->extents.left <= src2->extents.left) &&
        (src1->extents.top <= src2->extents.top) &&
        (src1->extents.right >= src2->extents.right) &&
        (src1->extents.bottom >= src2->extents.bottom))
        return copy_region( dst, src1 );

    if ((src2->num_rects == 1) &&
        (src2->extents.left <= src1->extents.left) &&
        (src2->extents.top <= src1->extents.top) &&
        (src2->extents.right >= src1->extents.right) &&
        (src2->extents.bottom >= src1->extents.bottom))
        return copy_region( dst, src2 );

    if (!region_op( dst, src1, src2, union_overlapping,
                    union_non_overlapping, union_non_overlapping )) return NULL;

    dst->extents.left = min(src1->extents.left, src2->extents.left);
    dst->extents.top = min(src1->extents.top, src2->extents.top);
    dst->extents.right = max(src1->extents.right, src2->extents.right);
    dst->extents.bottom = max(src1->extents.bottom, src2->extents.bottom);
    return dst;
}

/* compute the exclusive or of two regions into dst, which can be one of the source regions */
struct region *xor_region( struct region *dst, const struct region *src1,
                           const struct region *src2 )
{
    struct region *tmp = create_empty_region();

    if (!tmp) return NULL;

    if (!subtract_region( tmp, src1, src2 ) ||
        !subtract_region( dst, src2, src1 ) ||
        !union_region( dst, dst, tmp ))
        dst = NULL;

    free_region( tmp );
    return dst;
}

/* check if the given point is inside the region */
int point_in_region( struct region *region, int x, int y )
{
    const rectangle_t *ptr, *end;

    for (ptr = region->rects, end = region->rects + region->num_rects; ptr < end; ptr++)
    {
        if (ptr->top > y) return 0;
        if (ptr->bottom <= y) continue;
        /* now we are in the correct band */
        if (ptr->left > x) return 0;
        if (ptr->right <= x) continue;
        return 1;
    }
    return 0;
}

/* check if the given rectangle is (at least partially) inside the region */
int rect_in_region( struct region *region, const rectangle_t *rect )
{
    const rectangle_t *ptr, *end;

    for (ptr = region->rects, end = region->rects + region->num_rects; ptr < end; ptr++)
    {
        if (ptr->top >= rect->bottom) return 0;
        if (ptr->bottom <= rect->top) continue;
        if (ptr->left >= rect->right) continue;
        if (ptr->right <= rect->left) continue;
        return 1;
    }
    return 0;
}
