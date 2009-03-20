/*
App Software Licence
--------------------
This package includes software which is copyright (c) L. Patrick.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. You may not sell this software package.
4. You may include this software in a distribution of other software,
   and you may charge a nominal fee for the media used.
5. You may sell derivative programs, providing that such programs
   simply use this software in a compiled form.
6. You may sell derivative programs which use a compiled, modified
   version of this software, provided that you have attempted as
   best as you can to propagate all modifications made to the source
   code files of this software package back to the original author(s)
   of this package.

THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS AS IS, AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
*/
/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.

   http://enchantia.com/software/graphapp/
   http://www.it.usyd.edu.au/~graphapp/
*/
/*
    Modified for ReactOS
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>


#define DEGREES_TO_RADIANS(deg) ((deg)*2*M_PI/360)

typedef struct _Rect
{
	int x, y;		/* top-left point inside rect */
	int width, height;	/* width and height of rect */
} Rect, *PRect;

int FASTCALL IntFillRect(DC *dc, INT XLeft, INT YLeft, INT Width, INT Height, PGDIBRUSHOBJ BrushObj, BOOL Pen);
int FASTCALL app_fill_rect(DC *dc, Rect r, PGDIBRUSHOBJ BrushObj, BOOL Pen);

static
POINT
INTERNAL_CALL
app_new_point(int x, int y)
{
	POINT p;	
	p.x = x;
	p.y = y;
	return p;
}
#define pt(x,y)       app_new_point((x),(y))

static
Rect
INTERNAL_CALL
rect(int x, int y, int width, int height)
{
  Rect r;
  r.x = x;
  r.y = y;
  r.width = width;
  r.height = height;
  return r;
}


/*
 *  app_window_fill_rect:
 *
 *  Fill a rectangle with colour, in a window.
 *
 *  This function implements client-side clipping, so that
 *  we never rely on the GDI system to do clipping, except if
 *  the destination is a window which is partially obscured.
 *  In that situation we must rely on the GDI system because there
 *  is no way for the program to know which portions of the
 *  window are currently obscured.
 */
#define app_fill_rect( dc, r, BrushObj, Pen) \
        IntFillRect(dc, r.x, r.y, r.width, r.height, BrushObj, Pen)

/*
 *  Drawing an ellipse with a certain line thickness.
 *  Use an inner and and outer ellipse and fill the spaces between.
 *  The inner ellipse uses all UPPERCASE letters, the outer lowercase.
 *
 *  This algorithm is based on the fill_ellipse algorithm presented
 *  above, but uses two ellipse calculations, and some fix-up code
 *  to avoid pathological cases where the inner ellipse is almost
 *  the same size as the outer (in which case the border of the
 *  elliptical curve might otherwise have appeared broken).
 */
static
int
INTERNAL_CALL
app_draw_ellipse(DC *g, Rect r, PGDIBRUSHOBJ BrushObj)
{
	/* Outer ellipse: e(x,y) = b*b*x*x + a*a*y*y - a*a*b*b */

	int a = r.width / 2;
	int b = r.height / 2;
	int x = 0;
	int y = b;
	long a2 = a*a;
	long b2 = b*b;
	long xcrit = (3 * a2 / 4) + 1;
	long ycrit = (3 * b2 / 4) + 1;
	long t = b2 + a2 - 2*a2*b;	/* t = e(x+1,y-1) */
	long dxt = b2*(3+x+x);
	long dyt = a2*(3-y-y);
	int d2xt = b2+b2;
	int d2yt = a2+a2;

	int w = BrushObj->ptPenWidth.x;

	/* Inner ellipse: E(X,Y) = B*B*X*X + A*A*Y*Y - A*A*B*B */

	int A = a-w > 0 ? a-w : 0;
	int B = b-w > 0 ? b-w : 0;
	int X = 0;
	int Y = B;
	long A2 = A*A;
	long B2 = B*B;
	long XCRIT = (3 * A2 / 4) + 1;
	long YCRIT = (3 * B2 / 4) + 1;
	long T = B2 + A2 - 2*A2*B;	/* T = E(X+1,Y-1) */
	long DXT = B2*(3+X+X);
	long DYT = A2*(3-Y-Y);
	int D2XT = B2+B2;
	int D2YT = A2+A2;

	int movedown, moveout;
	int innerX = 0, prevx, prevy, W;
	Rect r1, r2;
	int result = 1;

//	START_DEBUG();

	if ((r.width <= 2) || (r.height <= 2))
		return app_fill_rect(g, r, BrushObj, TRUE);

	r1.x = r.x + a;
	r1.y = r.y;
	r1.width = r.width & 1; /* i.e. if width is odd */
	r1.height = 1;

	r2 = r1;
	r2.y = r.y + r.height - 1;

	prevx = r1.x;
	prevy = r1.y;

	while (y > 0)
	{
		while (Y == y)
		{
			innerX = X;

			if (T + A2*Y < XCRIT) /* E(X+1,Y-1/2) <= 0 */
			{
				/* move outwards to encounter edge */
				X += 1;
				T += DXT;
				DXT += D2XT;
			}
			else if (T - B2*X >= YCRIT) /* e(x+1/2,y-1) > 0 */
			{
				/* drop down one line */
				Y -= 1;
				T += DYT;
				DYT += D2YT;
			}
			else {
				/* drop diagonally down and out */
				X += 1;
				Y -= 1;
				T += DXT + DYT;
				DXT += D2XT;
				DYT += D2YT;
			}
		}

		movedown = moveout = 0;

		W = x - innerX;
		if (r1.x + W < prevx)
			W = prevx - r1.x;
		if (W < w)
			W = w;

		if (t + a2*y < xcrit) /* e(x+1,y-1/2) <= 0 */
		{
			/* move outwards to encounter edge */
			x += 1;
			t += dxt;
			dxt += d2xt;

			moveout = 1;
		}
		else if (t - b2*x >= ycrit) /* e(x+1/2,y-1) > 0 */
		{
			/* drop down one line */
			y -= 1;
			t += dyt;
			dyt += d2yt;

			movedown = 1;
		}
		else {
			/* drop diagonally down and out */
			x += 1;
			y -= 1;
			t += dxt + dyt;
			dxt += d2xt;
			dyt += d2yt;

			movedown = 1;
			moveout = 1;
		}

		if (movedown) {
			if (r1.width == 0) {
				r1.x -= 1; r1.width += 2;
				r2.x -= 1; r2.width += 2;
				moveout = 0;
			}

			if (r1.x < r.x)
				r1.x = r2.x = r.x;
			if (r1.width > r.width)
				r1.width = r2.width = r.width;
			if (r1.y == r2.y-1) {
				r1.x = r2.x = r.x;
				r1.width = r2.width = r.width;
			}

			if ((r1.y < r.y+w) || (r1.x+W >= r1.x+r1.width-W))
			{
				result &= app_fill_rect(g, r1, BrushObj, TRUE);
				result &= app_fill_rect(g, r2, BrushObj, TRUE);

				prevx = r1.x;
				prevy = r1.y;
			}
			else if (r1.y+r1.height < r2.y)
			{
				/* draw distinct rectangles */
				result &= app_fill_rect(g, rect(r1.x,r1.y,
						W,1), BrushObj, TRUE);
				result &= app_fill_rect(g, rect(
						r1.x+r1.width-W,r1.y,W,1), BrushObj, TRUE);
				result &= app_fill_rect(g, rect(r2.x,
						r2.y,W,1), BrushObj, TRUE);
				result &= app_fill_rect(g, rect(
						r2.x+r2.width-W,r2.y,W,1), BrushObj, TRUE);

				prevx = r1.x;
				prevy = r1.y;
			}

			/* move down */
			r1.y += 1;
			r2.y -= 1;
		}

		if (moveout) {
			/* move outwards */
			r1.x -= 1; r1.width += 2;
			r2.x -= 1; r2.width += 2;
		}
	}
	if ((x <= a) && (prevy < r2.y)) {
		/* draw final line */
		r1.height = r1.y+r1.height-r2.y;
		r1.y = r2.y;

		W = w;
		if (r.x + W != prevx)
			W = prevx - r.x;
		if (W < w)
			W = w;

		if (W+W >= r.width) {
			result &= app_fill_rect(g, rect(r.x, r1.y,
				r.width, r1.height), BrushObj, TRUE);
			return result;
		}

		result &= app_fill_rect(g, rect(r.x, r1.y, W, r1.height), BrushObj, TRUE);
		result &= app_fill_rect(g, rect(r.x+r.width-W, r1.y,
			W, r1.height), BrushObj, TRUE);
	}
	return result;
}

/*
 *  Draw an arc of an ellipse from start_angle anti-clockwise to
 *  end_angle. If the angles coincide, draw nothing; if they
 *  differ by 360 degrees or more, draw a full ellipse.
 *  The shape is drawn with the current line thickness,
 *  completely within the bounding rectangle. The shape is also
 *  axis-aligned, so that the ellipse would be horizontally and
 *  vertically symmetric is it was complete.
 *
 *  The draw_arc algorithm is based on draw_ellipse, but unlike
 *  that algorithm is not symmetric in the general case, since
 *  an angular portion is clipped from the shape. 
 *  This clipping is performed by keeping track of two hypothetical
 *  lines joining the centre point to the enclosing rectangle,
 *  at the angles start_angle and end_angle, using a line-intersection
 *  algorithm. Essentially the algorithm just fills the spaces
 *  which are within the arc and also between the angles, going
 *  in an anti-clockwise direction from start_angle to end_angle.
 *  In the top half of the ellipse, this amounts to drawing
 *  to the left of the start_angle line and to the right of
 *  the end_angle line, while in the bottom half of the ellipse,
 *  it involves drawing to the right of the start_angle and to
 *  the left of the end_angle.
 */

/*
 *  Fill a rectangle within an arc, given the centre point p0,
 *  and the two end points of the lines corresponding to the
 *  start_angle and the end_angle. This function takes care of
 *  the logic needed to swap the fill direction below
 *  the central point, and also performs the calculations
 *  needed to intersect the current Y value with each line.
 */
static
int
FASTCALL
app_fill_arc_rect(DC *g,
        Rect r,   // top, left, width, height
	POINT p0, // Center
	POINT p1, // Start
	POINT p2, // End
	int start_angle,
	int end_angle,
	PGDIBRUSHOBJ BrushObj,
	BOOL Pen)
{
	int x1, x2;
	int start_above, end_above;
	long rise1, run1, rise2, run2;

	rise1 = p1.y - p0.y;
	run1  = p1.x - p0.x;
	rise2 = p2.y - p0.y;
	run2  = p2.x - p0.x;

	if (r.y <= p0.y) // 
        {
		/* in top half of arc ellipse */

		if (p1.y <= r.y)
		{
			/* start_line is in the top half and is */
			/* intersected by the current Y scan line */
			if (rise1 == 0)
				x1 = p1.x;
			else
				x1 = p0.x + (r.y-p0.y)*run1/rise1;
			start_above = 1;
		}
		else if ((start_angle >= 0) && (start_angle <= 180))
		{
			/* start_line is above middle */
			x1 = p1.x;
			start_above = 1;
		}
		else
		{
			/* start_line is below middle */
			x1 = r.x + r.width;
			start_above = 0;
		}
		if (x1 < r.x)
			x1 = r.x;
		if (x1 > r.x+r.width)
			x1 = r.x+r.width;

		if (p2.y <= r.y)
		{
			/* end_line is in the top half and is */
			/* intersected by the current Y scan line */
			if (rise2 == 0)
				x2 = p2.x;
			else
				x2 = p0.x + (r.y-p0.y)*run2/rise2;
			end_above = 1;
		}
		else if ((end_angle >= 0) && (end_angle <= 180))
		{
			/* end_line is above middle */
			x2 = p2.x;
			end_above = 1;
		}
		else
		{
			/* end_line is below middle */
			x2 = r.x;
			end_above = 0;
		}

		if (x2 < r.x) x2 = r.x;

		if (x2 > r.x+r.width) x2 = r.x+r.width;

		if (start_above && end_above)
		{
			if (start_angle > end_angle)
			{
				/* fill outsides of wedge */
				if (! app_fill_rect(g, rect(r.x, r.y,
					x1-r.x, r.height), BrushObj, Pen))
					return 0;
				return app_fill_rect(g, rect(x2, r.y,
					r.x+r.width-x2, r.height), BrushObj, Pen);
			}
			else
			{
				/* fill inside of wedge */
				r.width = x1-x2;
				r.x = x2;
				return app_fill_rect(g, r, BrushObj, Pen);
			}
		}
		else if (start_above)
		{
			/* fill to the left of the start_line */
			r.width = x1-r.x;
			return app_fill_rect(g, r, BrushObj, Pen);
		}
		else if (end_above)
		{
			/* fill right of end_line */
			r.width = r.x+r.width-x2;
			r.x = x2;
			return app_fill_rect(g, r, BrushObj, Pen);
		}
		else
		{
			if (start_angle > end_angle)
				return app_fill_rect(g,r, BrushObj, Pen);
			else
				return 1;
		}
	}
	else
	{
		/* in lower half of arc ellipse */

		if (p1.y >= r.y)
		{
			/* start_line is in the lower half and is */
			/* intersected by the current Y scan line */
			if (rise1 == 0)
				x1 = p1.x;
			else
				x1 = p0.x + (r.y-p0.y)*run1/rise1;
			start_above = 0;
		}
		else if ((start_angle >= 180) && (start_angle <= 360))
		{
			/* start_line is below middle */
			x1 = p1.x;
			start_above = 0;
		}
		else
		{
			/* start_line is above middle */
			x1 = r.x;
			start_above = 1;
		}
		if (x1 < r.x)
			x1 = r.x;
		if (x1 > r.x+r.width)
			x1 = r.x+r.width;

		if (p2.y >= r.y)
		{
			/* end_line is in the lower half and is */
			/* intersected by the current Y scan line */
			if (rise2 == 0)
				x2 = p2.x;
			else
				x2 = p0.x + (r.y-p0.y)*run2/rise2;
			end_above = 0;
		}
		else if ((end_angle >= 180) && (end_angle <= 360))
		{
			/* end_line is below middle */
			x2 = p2.x;
			end_above = 0;
		}
		else
		{
			/* end_line is above middle */
			x2 = r.x + r.width;
			end_above = 1;
		}
		if (x2 < r.x)
			x2 = r.x;
		if (x2 > r.x+r.width)
			x2 = r.x+r.width;

		if (start_above && end_above)
		{
			if (start_angle > end_angle)
				return app_fill_rect(g,r, BrushObj, Pen);
			else
				return 1;
		}
		else if (start_above)
		{
			/* fill to the left of end_line */
			r.width = x2-r.x;
			return app_fill_rect(g,r, BrushObj, Pen);
		}
		else if (end_above)
		{
			/* fill right of start_line */
			r.width = r.x+r.width-x1;
			r.x = x1;
			return app_fill_rect(g,r, BrushObj, Pen);
		}
		else
		{
			if (start_angle > end_angle)
			{
				/* fill outsides of wedge */
				if (! app_fill_rect(g, rect(r.x, r.y,
					x2-r.x, r.height), BrushObj, Pen))
					return 0;
				return app_fill_rect(g, rect(x1, r.y,
					r.x+r.width-x1, r.height), BrushObj, Pen);
			}
			else
			{
				/* fill inside of wedge */
				r.width = x2-x1;
				r.x = x1;
				return app_fill_rect(g, r, BrushObj, Pen);
			}
		}
	}
}

/*
 *  To fill an axis-aligned ellipse, we use a scan-line algorithm.
 *  We walk downwards from the top Y co-ordinate, calculating
 *  the width of the ellipse using incremental integer arithmetic.
 *  To save calculation, we observe that the top and bottom halves
 *  of the ellipsoid are mirror-images, therefore we can draw the
 *  top and bottom halves by reflection. As a result, this algorithm
 *  draws rectangles inwards from the top and bottom edges of the
 *  bounding rectangle.
 *
 *  To save rendering time, draw as few rectangles as possible.
 *  Other ellipse-drawing algorithms assume we want to draw each
 *  line, using a draw_pixel operation, or a draw_horizontal_line
 *  operation. This approach is slower than it needs to be in
 *  circumstances where a fill_rect operation is more efficient
 *  (such as in X-Windows, where there is a communication overhead
 *  to the X-Server). For this reason, the algorithm accumulates
 *  rectangles on adjacent lines which have the same width into a
 *  single larger rectangle.
 *
 *  This algorithm forms the basis of the later, more complex,
 *  draw_ellipse algorithm, which renders the rectangular spaces
 *  between an outer and inner ellipse, and also the draw_arc and
 *  fill_arc operations which additionally clip drawing between
 *  a start_angle and an end_angle.
 *  
 */
static
int
FASTCALL
app_fill_ellipse(DC *g, Rect r, PGDIBRUSHOBJ BrushObj)
{
	/* e(x,y) = b*b*x*x + a*a*y*y - a*a*b*b */

	int a = r.width / 2;
	int b = r.height / 2;
	int x = 0;
	int y = b;
	long a2 = a*a;
	long b2 = b*b;
	long xcrit = (3 * a2 / 4) + 1;
	long ycrit = (3 * b2 / 4) + 1;
	long t = b2 + a2 - 2*a2*b;	/* t = e(x+1,y-1) */
	long dxt = b2*(3+x+x);
	long dyt = a2*(3-y-y);
	int d2xt = b2+b2;
	int d2yt = a2+a2;
	Rect r1, r2;
	int result = 1;

//	START_DEBUG();

	if ((r.width <= 2) || (r.height <= 2))
		return app_fill_rect(g, r, BrushObj, FALSE);

	r1.x = r.x + a;
	r1.y = r.y;
	r1.width = r.width & 1; /* i.e. if width is odd */
	r1.height = 1;

	r2 = r1;
	r2.y = r.y + r.height - 1;

	while (y > 0)
	{
		if (t + a2*y < xcrit) { /* e(x+1,y-1/2) <= 0 */
			/* move outwards to encounter edge */
			x += 1;
			t += dxt;
			dxt += d2xt;

			/* move outwards */
			r1.x -= 1; r1.width += 2;
			r2.x -= 1; r2.width += 2;
		}
		else if (t - b2*x >= ycrit) { /* e(x+1/2,y-1) > 0 */
			/* drop down one line */
			y -= 1;
			t += dyt;
			dyt += d2yt;

			/* enlarge rectangles */
			r1.height += 1;
			r2.height += 1; r2.y -= 1;
		}
		else {
			/* drop diagonally down and out */
			x += 1;
			y -= 1;
			t += dxt + dyt;
			dxt += d2xt;
			dyt += d2yt;

			if ((r1.width > 0) && (r1.height > 0))
			{
				/* draw rectangles first */

				if (r1.y+r1.height < r2.y) {
					/* distinct rectangles */
					result &= app_fill_rect(g, r1, BrushObj, FALSE);
					result &= app_fill_rect(g, r2, BrushObj, FALSE);
				}

				/* move down */
				r1.y += r1.height; r1.height = 1;
				r2.y -= 1;         r2.height = 1;
			}
			else {
				/* skipped pixels on initial diagonal */

				/* enlarge, rather than moving down */
				r1.height += 1;
				r2.height += 1; r2.y -= 1;
			}

			/* move outwards */
			r1.x -= 1; r1.width += 2;
			r2.x -= 1; r2.width += 2;
		}
	}
	if (r1.y < r2.y) {
		/* overlap */
		r1.x = r.x;
		r1.width = r.width;
		r1.height = r2.y+r2.height-r1.y;
		result &= app_fill_rect(g, r1, BrushObj, FALSE);
	}
	else if (x <= a) {
		/* crossover, draw final line */
		r1.x = r.x;
		r1.width = r.width;
		r1.height = r1.y+r1.height-r2.y;
		r1.y = r2.y;
		result &= app_fill_rect(g, r1, BrushObj, FALSE);
	}
	return result;
}

static
FASTCALL
POINT app_boundary_point(Rect r, int angle)
{
	int cx, cy;
	double tangent;

	cx = r.width;
	cx /= 2;
	cx += r.x;

	cy = r.height;
	cy /= 2;
	cy += r.y;

	if (angle == 0)
		return pt(r.x+r.width, cy);
	else if (angle == 45)
		return pt(r.x+r.width, r.y);
	else if (angle == 90)
		return pt(cx, r.y);
	else if (angle == 135)
		return pt(r.x, r.y);
	else if (angle == 180)
		return pt(r.x, cy);
	else if (angle == 225)
		return pt(r.x, r.y+r.height);
	else if (angle == 270)
		return pt(cx, r.y+r.height);
	else if (angle == 315)
		return pt(r.x+r.width, r.y+r.height);

	tangent = tan(DEGREES_TO_RADIANS(angle));

	if ((angle > 45) && (angle < 135))
		return pt((int)(cx+r.height/tangent/2), r.y);
	else if ((angle > 225) && (angle < 315))
		return pt((int)(cx-r.height/tangent/2), r.y+r.height);
	else if ((angle > 135) && (angle < 225))
		return pt(r.x, (int)(cy+r.width*tangent/2));
	else
		return pt(r.x+r.width, (int)(cy-r.width*tangent/2));
}

int
FASTCALL
app_fill_arc(DC *g, Rect r, int start_angle, int end_angle, PGDIBRUSHOBJ BrushObj, BOOL Chord)
{
	/* e(x,y) = b*b*x*x + a*a*y*y - a*a*b*b */

	int a = r.width / 2;
	int b = r.height / 2;
	int x = 0;
	int y = b;
	long a2 = a*a;
	long b2 = b*b;
	long xcrit = (3 * a2 / 4) + 1;
	long ycrit = (3 * b2 / 4) + 1;
	long t = b2 + a2 - 2*a2*b;	/* t = e(x+1,y-1) */
	long dxt = b2*(3+x+x);
	long dyt = a2*(3-y-y);
	int d2xt = b2+b2;
	int d2yt = a2+a2;
	Rect r1, r2;
	int movedown, moveout;
	int result = 1;

	/* line descriptions */
	POINT p0, p1, p2;

//	START_DEBUG();

	/* if angles differ by 360 degrees or more, close the shape */
	if ((start_angle + 360 <= end_angle) ||
	    (start_angle - 360 >= end_angle))
	{
		return app_fill_ellipse(g, r, BrushObj);
	}

	/* make start_angle >= 0 and <= 360 */
	while (start_angle < 0)
		start_angle += 360;
	start_angle %= 360;

	/* make end_angle >= 0 and <= 360 */
	while (end_angle < 0)
		end_angle += 360;
	end_angle %= 360;

	/* draw nothing if the angles are equal */
	if (start_angle == end_angle)
		return 1;

	/* find arc wedge line end points */
	p1 = app_boundary_point(r, start_angle);
	p2 = app_boundary_point(r, end_angle);
	if (Chord)
	 p0 = pt((p1.x+p2.x)/2,(p1.y+p2.y)/2);
	else
	 p0 = pt(r.x + r.width/2, r.y + r.height/2);

	/* initialise rectangles to be drawn */
	r1.x = r.x + a;
	r1.y = r.y;
	r1.width = r.width & 1; /* i.e. if width is odd */
	r1.height = 1;

	r2 = r1;
	r2.y = r.y + r.height - 1;

	while (y > 0)
	{
		moveout = movedown = 0;

		if (t + a2*y < xcrit) { /* e(x+1,y-1/2) <= 0 */
			/* move outwards to encounter edge */
			x += 1;
			t += dxt;
			dxt += d2xt;

			moveout = 1;
		}
		else if (t - b2*x >= ycrit) { /* e(x+1/2,y-1) > 0 */
			/* drop down one line */
			y -= 1;
			t += dyt;
			dyt += d2yt;

			movedown = 1;
		}
		else {
			/* drop diagonally down and out */
			x += 1;
			y -= 1;
			t += dxt + dyt;
			dxt += d2xt;
			dyt += d2yt;

			moveout = 1;
			movedown = 1;
		}

		if (movedown) {
			if (r1.width == 0) {
				r1.x -= 1; r1.width += 2;
				r2.x -= 1; r2.width += 2;
				moveout = 0;
			}

			if (r1.x < r.x)
				r1.x = r2.x = r.x;
			if (r1.width > r.width)
				r1.width = r2.width = r.width;
			if (r1.y == r2.y-1) {
				r1.x = r2.x = r.x;
				r1.width = r2.width = r.width;
			}

			if ((r1.width > 0) && (r1.y+r1.height < r2.y)) {
				/* distinct rectangles */
				result &= app_fill_arc_rect(g, r1,
						p0, p1, p2,
						start_angle, end_angle, BrushObj, FALSE);
				result &= app_fill_arc_rect(g, r2,
						p0, p1, p2,
						start_angle, end_angle, BrushObj, FALSE);
			}

			/* move down */
			r1.y += 1;
			r2.y -= 1;
		}

		if (moveout) {
			/* move outwards */
			r1.x -= 1; r1.width += 2;
			r2.x -= 1; r2.width += 2;
		}
	}
	if (r1.y < r2.y) {
		/* overlap */
		r1.x = r.x;
		r1.width = r.width;
		r1.height = r2.y+r2.height-r1.y;
		while (r1.height > 0) {
			result &= app_fill_arc_rect(g,
				rect(r1.x, r1.y, r1.width, 1),
				p0, p1, p2, start_angle, end_angle, BrushObj, FALSE);
			r1.y += 1;
			r1.height -= 1;
		}
	}
	else if (x <= a) {
		/* crossover, draw final line */
		r1.x = r.x;
		r1.width = r.width;
		r1.height = r1.y+r1.height-r2.y;
		r1.y = r2.y;
		while (r1.height > 0) {
			result &= app_fill_arc_rect(g, 
				rect(r1.x, r1.y, r1.width, 1),
				p0, p1, p2, start_angle, end_angle, BrushObj, FALSE);
			r1.y += 1;
			r1.height -= 1;
		}
	}
	return result;
}

int app_draw_arc(DC *g, Rect r, int start_angle, int end_angle, PGDIBRUSHOBJ PenBrushObj, BOOL Chord)
{
	/* Outer ellipse: e(x,y) = b*b*x*x + a*a*y*y - a*a*b*b */

	int a = r.width / 2;
	int b = r.height / 2;
	int x = 0;
	int y = b;
	long a2 = a*a;
	long b2 = b*b;
	long xcrit = (3 * a2 / 4) + 1;
	long ycrit = (3 * b2 / 4) + 1;
	long t = b2 + a2 - 2*a2*b;	/* t = e(x+1,y-1) */
	long dxt = b2*(3+x+x);
	long dyt = a2*(3-y-y);
	int d2xt = b2+b2;
	int d2yt = a2+a2;

	int w = PenBrushObj->ptPenWidth.x;

	/* Inner ellipse: E(X,Y) = B*B*X*X + A*A*Y*Y - A*A*B*B */

	int A = a-w > 0 ? a-w : 0;
	int B = b-w > 0 ? b-w : 0;
	int X = 0;
	int Y = B;
	long A2 = A*A;
	long B2 = B*B;
	long XCRIT = (3 * A2 / 4) + 1;
	long YCRIT = (3 * B2 / 4) + 1;
	long T = B2 + A2 - 2*A2*B;	/* T = E(X+1,Y-1) */
	long DXT = B2*(3+X+X);
	long DYT = A2*(3-Y-Y);
	int D2XT = B2+B2;
	int D2YT = A2+A2;

	/* arc rectangle calculations */
	int movedown, moveout;
	int innerX = 0, prevx, prevy, W;
	Rect r1, r2;
	int result = 1;

	/* line descriptions */
	POINT p0, p1, p2;

//	START_DEBUG();

	/* if angles differ by 360 degrees or more, close the shape */
	if ((start_angle + 360 <= end_angle) ||
	    (start_angle - 360 >= end_angle))
	{
		return app_draw_ellipse(g, r, PenBrushObj);
	}

	/* make start_angle >= 0 and <= 360 */
	while (start_angle < 0)
		start_angle += 360;
	start_angle %= 360;

	/* make end_angle >= 0 and <= 360 */
	while (end_angle < 0)
		end_angle += 360;
	end_angle %= 360;

	/* draw nothing if the angles are equal */
	if (start_angle == end_angle)
		return 1;

	/* find arc wedge line end points */
	p1 = app_boundary_point(r, start_angle);
	p2 = app_boundary_point(r, end_angle);
	if (Chord)
	 p0 = pt((p1.x+p2.x)/2,(p1.y+p2.y)/2);
	else
	 p0 = pt(r.x + r.width/2, r.y + r.height/2);

	/* determine ellipse rectangles */
	r1.x = r.x + a;
	r1.y = r.y;
	r1.width = r.width & 1; /* i.e. if width is odd */
	r1.height = 1;

	r2 = r1;
	r2.y = r.y + r.height - 1;

	prevx = r1.x;
	prevy = r1.y;

	while (y > 0)
	{
		while (Y == y)
		{
			innerX = X;

			if (T + A2*Y < XCRIT) /* E(X+1,Y-1/2) <= 0 */
			{
				/* move outwards to encounter edge */
				X += 1;
				T += DXT;
				DXT += D2XT;
			}
			else if (T - B2*X >= YCRIT) /* e(x+1/2,y-1) > 0 */
			{
				/* drop down one line */
				Y -= 1;
				T += DYT;
				DYT += D2YT;
			}
			else {
				/* drop diagonally down and out */
				X += 1;
				Y -= 1;
				T += DXT + DYT;
				DXT += D2XT;
				DYT += D2YT;
			}
		}

		movedown = moveout = 0;

		W = x - innerX;
		if (r1.x + W < prevx)
			W = prevx - r1.x;
		if (W < w)
			W = w;

		if (t + a2*y < xcrit) /* e(x+1,y-1/2) <= 0 */
		{
			/* move outwards to encounter edge */
			x += 1;
			t += dxt;
			dxt += d2xt;

			moveout = 1;
		}
		else if (t - b2*x >= ycrit) /* e(x+1/2,y-1) > 0 */
		{
			/* drop down one line */
			y -= 1;
			t += dyt;
			dyt += d2yt;

			movedown = 1;
		}
		else {
			/* drop diagonally down and out */
			x += 1;
			y -= 1;
			t += dxt + dyt;
			dxt += d2xt;
			dyt += d2yt;

			movedown = 1;
			moveout = 1;
		}

		if (movedown) {
			if (r1.width == 0) {
				r1.x -= 1; r1.width += 2;
				r2.x -= 1; r2.width += 2;
				moveout = 0;
			}

			if (r1.x < r.x)
				r1.x = r2.x = r.x;
			if (r1.width > r.width)
				r1.width = r2.width = r.width;
			if (r1.y == r2.y-1) {
				r1.x = r2.x = r.x;
				r1.width = r2.width = r.width;
			}

			if ((r1.y < r.y+w) || (r1.x+W >= r1.x+r1.width-W))
			{
				result &= app_fill_arc_rect(g, r1,
						p0, p1, p2,
						start_angle, end_angle, PenBrushObj, TRUE);
				result &= app_fill_arc_rect(g, r2,
						p0, p1, p2,
						start_angle, end_angle, PenBrushObj, TRUE);

				prevx = r1.x;
				prevy = r1.y;
			}
			else if (r1.y+r1.height < r2.y)
			{
				/* draw distinct rectangles */
				result &= app_fill_arc_rect(g, rect(
						r1.x,r1.y,W,1),
						p0, p1, p2,
						start_angle, end_angle, PenBrushObj, TRUE);
				result &= app_fill_arc_rect(g, rect(
						r1.x+r1.width-W,r1.y,W,1),
						p0, p1, p2,
						start_angle, end_angle, PenBrushObj, TRUE);
				result &= app_fill_arc_rect(g, rect(
						r2.x,r2.y,W,1),
						p0, p1, p2,
						start_angle, end_angle, PenBrushObj, TRUE);
				result &= app_fill_arc_rect(g, rect(
						r2.x+r2.width-W,r2.y,W,1),
						 p0, p1, p2,
						start_angle, end_angle, PenBrushObj, TRUE);

				prevx = r1.x;
				prevy = r1.y;
			}

			/* move down */
			r1.y += 1;
			r2.y -= 1;
		}

		if (moveout) {
			/* move outwards */
			r1.x -= 1; r1.width += 2;
			r2.x -= 1; r2.width += 2;
		}
	}
	if ((x <= a) && (prevy < r2.y)) {
		/* draw final lines */
		r1.height = r1.y+r1.height-r2.y;
		r1.y = r2.y;

		W = w;
		if (r.x + W != prevx)
			W = prevx - r.x;
		if (W < w)
			W = w;

		if (W+W >= r.width) {
			while (r1.height > 0) {
				result &= app_fill_arc_rect(g, rect(r.x,
					r1.y, r.width, 1), p0, p1, p2,
					start_angle, end_angle, PenBrushObj, TRUE);
				r1.y += 1;
				r1.height -= 1;
			}
			return result;
		}

		while (r1.height > 0) {
			result &= app_fill_arc_rect(g, rect(r.x, r1.y,
					W, 1), p0, p1, p2,
					start_angle, end_angle, PenBrushObj, TRUE);
			result &= app_fill_arc_rect(g, rect(r.x+r.width-W,
					r1.y, W, 1), p0, p1, p2,
					start_angle, end_angle, PenBrushObj, TRUE);
			r1.y += 1;
			r1.height -= 1;
		}
	}

	return result;
}

/* ReactOS Interface *********************************************************/

int
FASTCALL
IntFillRect( DC *dc,
             INT XLeft,
             INT YLeft,
             INT Width,
             INT Height,
             PGDIBRUSHOBJ BrushObj,
             BOOL Pen)
{
  DWORD ROP = PATCOPY;
  RECTL DestRect;
  SURFACE *psurf;
  GDIBRUSHINST BrushInst;
  POINTL BrushOrigin;
  BOOL Ret = TRUE;
  PDC_ATTR pdcattr;

  ASSERT(BrushObj);

  psurf = SURFACE_LockSurface(dc->rosdc.hBitmap);
  if (psurf == NULL)
  {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
  }

  if (!(BrushObj->flAttrs & GDIBRUSH_IS_NULL))
  {
     pdcattr = dc->pdcattr;

     /* fix negative spaces */
     if (Width < 0)
     {
	XLeft += Width;
	Width = 0 - Width;
     }
     if (Height < 0)
     {
	YLeft += Height;
	Height = 0 - Height;
     }

     DestRect.left = XLeft;
     DestRect.right = XLeft + Width;

     DestRect.top = YLeft;
     DestRect.bottom = YLeft + Height;

     BrushOrigin.x = BrushObj->ptOrigin.x;
     BrushOrigin.y = BrushObj->ptOrigin.y;

     if (pdcattr->jROP2 == R2_XORPEN)
        ROP = PATINVERT;
     else
        ROP = PATCOPY;

     if (Pen)
        IntGdiInitBrushInstance(&BrushInst, BrushObj, dc->rosdc.XlatePen);
     else
        IntGdiInitBrushInstance(&BrushInst, BrushObj, dc->rosdc.XlateBrush);

     Ret = IntEngBitBlt(
         &psurf->SurfObj,
         NULL,
         NULL,
         dc->rosdc.CombinedClip,
         NULL,
         &DestRect,
         NULL,
         NULL,
         &BrushInst.BrushObject, // use pDC->eboFill
         &BrushOrigin,
         ROP3_TO_ROP4(ROP));
  }

  SURFACE_UnlockSurface(psurf);
  return (int)Ret;
}

BOOL
FASTCALL
IntFillArc( PDC dc,
            INT XLeft,
            INT YLeft,
            INT Width,
            INT Height,
            double StartArc,
            double EndArc,
            ARCTYPE arctype)
{
  PDC_ATTR pdcattr;
  PGDIBRUSHOBJ FillBrushObj;
  int Start = ceill(StartArc);
  int End   = ceill(EndArc);
  BOOL Chord = (arctype == GdiTypeChord), ret;

  pdcattr = dc->pdcattr;

  FillBrushObj = BRUSHOBJ_LockBrush(pdcattr->hbrush);
  if (NULL == FillBrushObj)   
  {
      DPRINT1("FillArc Fail\n");
      SetLastWin32Error(ERROR_INTERNAL_ERROR);
      return FALSE;
  }
  // Sort out alignment here.
  ret = app_fill_arc(dc, rect( XLeft, YLeft, Width, Height),
                    (dc->dclevel.flPath & DCPATH_CLOCKWISE) ? -End : -Start,
                    (dc->dclevel.flPath & DCPATH_CLOCKWISE) ? -Start : -End,
                     FillBrushObj, Chord);

  BRUSHOBJ_UnlockBrush(FillBrushObj);    
  return ret;
}

BOOL
FASTCALL
IntDrawArc( PDC dc,
            INT XLeft,
            INT YLeft,
            INT Width,
            INT Height,
            double StartArc,
            double EndArc,
            ARCTYPE arctype,
            PGDIBRUSHOBJ PenBrushObj)
{
  int Start = ceill(StartArc);
  int End   = ceill(EndArc);
  BOOL Chord = (arctype == GdiTypeChord);
  // Sort out alignment here.
  return app_draw_arc(dc, rect( XLeft, YLeft, Width, Height),
                     (dc->dclevel.flPath & DCPATH_CLOCKWISE) ? -End : -Start,
                     (dc->dclevel.flPath & DCPATH_CLOCKWISE) ? -Start : -End,
                      PenBrushObj, Chord);
}

BOOL
FASTCALL
IntDrawEllipse( PDC dc,
                INT XLeft,
                INT YLeft,
                INT Width,
                INT Height,
                PGDIBRUSHOBJ PenBrushObj)
{
  return (BOOL)app_draw_ellipse(dc, rect( XLeft, YLeft, Width, Height), PenBrushObj);
}

BOOL
FASTCALL
IntFillEllipse( PDC dc,
                INT XLeft,
                INT YLeft,
                INT Width,
                INT Height, 
                PGDIBRUSHOBJ FillBrushObj)
{
  return (BOOL)app_fill_ellipse(dc, rect( XLeft, YLeft, Width, Height), FillBrushObj);
}

BOOL
FASTCALL
IntFillRoundRect( PDC dc,
                  INT Left,
                  INT Top,
                  INT Right,
                  INT Bottom,
                  INT Wellipse,
                  INT Hellipse, 
                  PGDIBRUSHOBJ FillBrushObj)
{
  Rect r;
  int rx, ry; /* radius in x and y directions */

  //           x    y          Width          Height
  r = rect( Left, Top, abs(Right-Left), abs(Bottom-Top));
  rx = Wellipse/2;
  ry = Hellipse/2;

  if (Wellipse > r.width)
  {
     if (Hellipse > r.height) // > W > H
        app_fill_ellipse(dc, r, FillBrushObj);
     else // > W < H
     {
       app_fill_arc(dc, rect( r.x, r.y, r.width - 1, Hellipse),
                        0, 180, FillBrushObj,FALSE);
       app_fill_arc(dc, rect(r.x, Bottom - Hellipse - 1, r.width - 1, Hellipse),
                        180, 360, FillBrushObj, FALSE);
     }
  }
  else if(Hellipse > r.height) // < W > H
  {
     app_fill_arc(dc, rect(r.x, r.y, Wellipse, r.height - 1),
                      90, 270, FillBrushObj, FALSE);
     app_fill_arc(dc, rect(Right - Wellipse - 1, r.y, Wellipse, r.height - 1),
                      270, 90, FillBrushObj,FALSE);
  }
  else // < W < H
  {
     app_fill_arc(dc, rect(r.x, r.y, rx+rx, ry+ry),
                      90, 180, FillBrushObj, FALSE);

     app_fill_arc(dc, rect(r.x, r.y+r.height-ry-ry, rx+rx, ry+ry),
                      180, 270, FillBrushObj, FALSE);

     app_fill_arc(dc, rect(r.x+r.width-rx-rx, r.y+r.height-ry-ry, rx+rx, ry+ry),
                      270, 360, FillBrushObj,FALSE);

     app_fill_arc(dc, rect(r.x+r.width-rx-rx, r.y, rx+rx, ry+ry),
                      0, 90, FillBrushObj,FALSE);   
  }
  if (Wellipse < r.width)
  {
     app_fill_rect(dc, rect(r.x+rx, r.y, r.width-rx-rx, ry+1), FillBrushObj, FALSE);
     app_fill_rect(dc, rect(r.x+rx, r.y+r.height-ry+1, r.width-rx-rx, ry-1), FillBrushObj, FALSE);
  }
  if (Hellipse < r.height)
  {
     app_fill_rect(dc, rect(r.x, r.y+ry+1, r.width, r.height-ry-ry), FillBrushObj, FALSE);
  }

  return TRUE;
}


BOOL
FASTCALL
IntDrawRoundRect( PDC dc,
                  INT Left,
                  INT Top,
                  INT Right,
                  INT Bottom,
                  INT Wellipse,
                  INT Hellipse,
                  PGDIBRUSHOBJ PenBrushObj)
{
  Rect r;
  int rx, ry; /* radius in x and y directions */
  int w = PenBrushObj->ptPenWidth.x;

  r = rect( Left, Top, abs(Right-Left), abs(Bottom-Top));
  rx = Wellipse/2;
  ry = Hellipse/2;
  
  if (Wellipse > r.width)
  {
     if (Hellipse > r.height) // > W > H
        app_draw_ellipse(dc, r, PenBrushObj);
     else // > W < H
     {
        app_draw_arc(dc, rect( r.x, r.y, r.width - 1, Hellipse - 1),
                         0, 180, PenBrushObj, FALSE);
        app_draw_arc(dc, rect(r.x, Bottom - Hellipse, r.width - 1, Hellipse - 1),
                         180, 360, PenBrushObj, FALSE);
     }
  }
  else if(Hellipse > r.height) // < W > H
  {
        app_draw_arc(dc, rect(r.x, r.y, Wellipse - 1, r.height - 1),
                         90, 270, PenBrushObj, FALSE);
        app_draw_arc(dc, rect(Right - Wellipse, r.y, Wellipse - 1, r.height - 1),
                         270, 90, PenBrushObj, FALSE);        
  }
  else // < W < H
  {
     app_draw_arc(dc, rect(r.x, r.y, rx+rx, ry+ry),
                      90, 180, PenBrushObj, FALSE);

     app_draw_arc(dc, rect(r.x,r.y+r.height-ry-ry,rx+rx,ry+ry),
                      180, 270, PenBrushObj, FALSE);

     app_draw_arc(dc, rect(r.x+r.width-rx-rx, r.y+r.height-ry-ry, rx+rx, ry+ry),
                      270, 360, PenBrushObj, FALSE);

     app_draw_arc(dc, rect(r.x+r.width-rx-rx,r.y,rx+rx,ry+ry),
                      0, 90, PenBrushObj, FALSE);
  }
  if ( Hellipse < r.height)
  {
     app_fill_rect(dc, rect(r.x, r.y+ry+1, w, r.height-ry-ry), PenBrushObj, TRUE);


     app_fill_rect(dc, rect(r.x+r.width-w, r.y+ry+1, w, r.height-ry-ry),
                       PenBrushObj, TRUE);
  }
  if ( Wellipse < r.width)
  {
     app_fill_rect(dc, rect(r.x+rx, r.y+r.height-w, r.width-rx-rx, w),
                       PenBrushObj, TRUE);

     app_fill_rect(dc, rect(r.x+rx, r.y, r.width-rx-rx, w), PenBrushObj, TRUE);
  }
 return TRUE;
}

