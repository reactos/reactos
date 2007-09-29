//========================================================================
//
// SplashXPath.cc
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdlib.h>
#include <string.h>
#include "goo/gmem.h"
#include "SplashMath.h"
#include "SplashPath.h"
#include "SplashXPath.h"

//------------------------------------------------------------------------

#define maxCurveSplits (1 << 10)

//------------------------------------------------------------------------
// SplashXPath
//------------------------------------------------------------------------

SplashXPath::SplashXPath() {
  segs = NULL;
  length = size = 0;
}

SplashXPath::SplashXPath(SplashPath *path, SplashCoord flatness,
			 GBool closeSubpaths) {
  SplashCoord xc, yc, dx, dy, r, x0, y0, x1, y1;
  int quad0, quad1, quad;
  int curSubpath, n, i, j;

  segs = NULL;
  length = size = 0;

  i = 0;
  curSubpath = 0;
  while (i < path->length) {

    // first point in subpath - skip it
    if (path->flags[i] & splashPathFirst) {
      curSubpath = i;
      ++i;

    } else {

      // curve segment
      if (path->flags[i] & splashPathCurve) {
	addCurve(path->pts[i-1].x, path->pts[i-1].y,
		 path->pts[i  ].x, path->pts[i  ].y,
		 path->pts[i+1].x, path->pts[i+1].y,
		 path->pts[i+2].x, path->pts[i+2].y,
		 flatness,
		 (path->flags[i-1] & splashPathFirst),
		 (path->flags[i+2] & splashPathLast),
		 !closeSubpaths &&
		   (path->flags[i-1] & splashPathFirst) &&
		   !(path->flags[i-1] & splashPathClosed),
		 !closeSubpaths &&
		   (path->flags[i+2] & splashPathLast) &&
		   !(path->flags[i+2] & splashPathClosed));
	i += 3;

      // clockwise circular arc
      } else if (path->flags[i] & splashPathArcCW) {
	xc = path->pts[i].x;
	yc = path->pts[i].y;
	dx = path->pts[i+1].x - xc;
	dy = path->pts[i+1].y - yc;
	r = splashSqrt(dx * dx + dy * dy);
	if (path->pts[i-1].x < xc && path->pts[i-1].y <= yc) {
	  quad0 = 0;
	} else if (path->pts[i-1].x >= xc && path->pts[i-1].y < yc) {
	  quad0 = 1;
	} else if (path->pts[i-1].x > xc && path->pts[i-1].y >= yc) {
	  quad0 = 2;
	} else {
	  quad0 = 3;
	}
	if (path->pts[i+1].x <= xc && path->pts[i+1].y < yc) {
	  quad1 = 0;
	} else if (path->pts[i+1].x > xc && path->pts[i+1].y <= yc) {
	  quad1 = 1;
	} else if (path->pts[i+1].x >= xc && path->pts[i+1].y > yc) {
	  quad1 = 2;
	} else {
	  quad1 = 3;
	}
	n = 0; // make gcc happy
	if (quad0 == quad1) {
	  switch (quad0) {
	  case 0:
	  case 1: n = path->pts[i-1].x < path->pts[i+1].x ? 0 : 4; break;
	  case 2:
	  case 3: n = path->pts[i-1].x > path->pts[i+1].x ? 0 : 4; break;
	  }
	} else {
	  n = (quad1 - quad0) & 3;
	}
	x0 = path->pts[i-1].x;
	y0 = path->pts[i-1].y;
	x1 = y1 = 0; // make gcc happy
	quad = quad0;
	for (j = 0; j < n; ++j) {
	  switch (quad) {
	  case 0: x1 = xc;     y1 = yc - r; break;
	  case 1: x1 = xc + r; y1 = yc;     break;
	  case 2: x1 = xc;     y1 = yc + r; break;
	  case 3: x1 = xc - r; y1 = yc;     break;
	  }
	  addArc(x0, y0, x1, y1,
		 xc, yc, r, quad, flatness,
		 quad == quad0 && (path->flags[i-1] & splashPathFirst),
		 gFalse,
		 quad == quad0 && !closeSubpaths &&
		   (path->flags[i-1] & splashPathFirst) &&
		   !(path->flags[i-1] & splashPathClosed),
		 gFalse);
	  x0 = x1;
	  y0 = y1;
	  quad = (quad + 1) & 3;
	}
	addArc(x0, y0, path->pts[i+1].x, path->pts[i+1].y,
	       xc, yc, r, quad, flatness,
	       quad == quad0 && (path->flags[i-1] & splashPathFirst),
	       (path->flags[i+1] & splashPathLast),
	       quad == quad0 && !closeSubpaths &&
	         (path->flags[i-1] & splashPathFirst) &&
	         !(path->flags[i-1] & splashPathClosed),
	       !closeSubpaths &&
	         (path->flags[i+1] & splashPathLast) &&
	         !(path->flags[i+1] & splashPathClosed));
	i += 2;

      // line segment
      } else {
	addSegment(path->pts[i-1].x, path->pts[i-1].y,
		   path->pts[i].x, path->pts[i].y,
		   path->flags[i-1] & splashPathFirst,
		   path->flags[i] & splashPathLast,
		   !closeSubpaths &&
		     (path->flags[i-1] & splashPathFirst) &&
		     !(path->flags[i-1] & splashPathClosed),
		   !closeSubpaths &&
		     (path->flags[i] & splashPathLast) &&
		     !(path->flags[i] & splashPathClosed));
	++i;
      }

      // close a subpath
      if (closeSubpaths &&
	  (path->flags[i-1] & splashPathLast) &&
	  (path->pts[i-1].x != path->pts[curSubpath].x ||
	   path->pts[i-1].y != path->pts[curSubpath]. y)) {
	addSegment(path->pts[i-1].x, path->pts[i-1].y,
		   path->pts[curSubpath].x, path->pts[curSubpath].y,
		   gFalse, gTrue, gFalse, gFalse);
      }
    }
  }
}

SplashXPath::SplashXPath(SplashXPath *xPath) {
  length = xPath->length;
  size = xPath->size;
  segs = (SplashXPathSeg *)gmallocn(size, sizeof(SplashXPathSeg));
  memcpy(segs, xPath->segs, length * sizeof(SplashXPathSeg));
}

SplashXPath::~SplashXPath() {
  gfree(segs);
}

// Add space for <nSegs> more segments
void SplashXPath::grow(int nSegs) {
  if (length + nSegs > size) {
    if (size == 0) {
      size = 32;
    }
    while (size < length + nSegs) {
      size *= 2;
    }
    segs = (SplashXPathSeg *)greallocn(segs, size, sizeof(SplashXPathSeg));
  }
}

void SplashXPath::addCurve(SplashCoord x0, SplashCoord y0,
			   SplashCoord x1, SplashCoord y1,
			   SplashCoord x2, SplashCoord y2,
			   SplashCoord x3, SplashCoord y3,
			   SplashCoord flatness,
			   GBool first, GBool last, GBool end0, GBool end1) {
  SplashCoord cx[maxCurveSplits + 1][3];
  SplashCoord cy[maxCurveSplits + 1][3];
  int cNext[maxCurveSplits + 1];
  SplashCoord xl0, xl1, xl2, xr0, xr1, xr2, xr3, xx1, xx2, xh;
  SplashCoord yl0, yl1, yl2, yr0, yr1, yr2, yr3, yy1, yy2, yh;
  SplashCoord dx, dy, mx, my, d1, d2, flatness2;
  int p1, p2, p3;

  flatness2 = flatness * flatness;

  // initial segment
  p1 = 0;
  p2 = maxCurveSplits;
  cx[p1][0] = x0;  cy[p1][0] = y0;
  cx[p1][1] = x1;  cy[p1][1] = y1;
  cx[p1][2] = x2;  cy[p1][2] = y2;
  cx[p2][0] = x3;  cy[p2][0] = y3;
  cNext[p1] = p2;

  while (p1 < maxCurveSplits) {

    // get the next segment
    xl0 = cx[p1][0];  yl0 = cy[p1][0];
    xx1 = cx[p1][1];  yy1 = cy[p1][1];
    xx2 = cx[p1][2];  yy2 = cy[p1][2];
    p2 = cNext[p1];
    xr3 = cx[p2][0];  yr3 = cy[p2][0];

    // compute the distances from the control points to the
    // midpoint of the straight line (this is a bit of a hack, but
    // it's much faster than computing the actual distances to the
    // line)
    mx = (xl0 + xr3) * 0.5;
    my = (yl0 + yr3) * 0.5;
    dx = xx1 - mx;
    dy = yy1 - my;
    d1 = dx*dx + dy*dy;
    dx = xx2 - mx;
    dy = yy2 - my;
    d2 = dx*dx + dy*dy;

    // if the curve is flat enough, or no more subdivisions are
    // allowed, add the straight line segment
    if (p2 - p1 == 1 || (d1 <= flatness2 && d2 <= flatness2)) {
      addSegment(xl0, yl0, xr3, yr3,
		 p1 == 0 && first,
		 p2 == maxCurveSplits && last,
		 p1 == 0 && end0,
		 p2 == maxCurveSplits && end1);
      p1 = p2;

    // otherwise, subdivide the curve
    } else {
      xl1 = (xl0 + xx1) * 0.5;
      yl1 = (yl0 + yy1) * 0.5;
      xh = (xx1 + xx2) * 0.5;
      yh = (yy1 + yy2) * 0.5;
      xl2 = (xl1 + xh) * 0.5;
      yl2 = (yl1 + yh) * 0.5;
      xr2 = (xx2 + xr3) * 0.5;
      yr2 = (yy2 + yr3) * 0.5;
      xr1 = (xh + xr2) * 0.5;
      yr1 = (yh + yr2) * 0.5;
      xr0 = (xl2 + xr1) * 0.5;
      yr0 = (yl2 + yr1) * 0.5;
      // add the new subdivision points
      p3 = (p1 + p2) / 2;
      cx[p1][1] = xl1;  cy[p1][1] = yl1;
      cx[p1][2] = xl2;  cy[p1][2] = yl2;
      cNext[p1] = p3;
      cx[p3][0] = xr0;  cy[p3][0] = yr0;
      cx[p3][1] = xr1;  cy[p3][1] = yr1;
      cx[p3][2] = xr2;  cy[p3][2] = yr2;
      cNext[p3] = p2;
    }
  }
}

void SplashXPath::addArc(SplashCoord x0, SplashCoord y0,
			 SplashCoord x1, SplashCoord y1,
			 SplashCoord xc, SplashCoord yc,
			 SplashCoord r, int quad,
			 SplashCoord flatness,
			 GBool first, GBool last, GBool end0, GBool end1) {
  SplashCoord px[maxCurveSplits + 1];
  SplashCoord py[maxCurveSplits + 1];
  int pNext[maxCurveSplits + 1];
  SplashCoord r2, flatness2;
  SplashCoord xx0, yy0, xx1, yy1, xm, ym, t, dx, dy;
  int p1, p2, p3;

  r2 = r * r;
  flatness2 = flatness * flatness;

  // initial segment
  p1 = 0;
  p2 = maxCurveSplits;
  px[p1] = x0;  py[p1] = y0;
  px[p2] = x1;  py[p2] = y1;
  pNext[p1] = p2;

  while (p1 < maxCurveSplits) {

    // get the next segment
    xx0 = px[p1];  yy0 = py[p1];
    p2 = pNext[p1];
    xx1 = px[p2];  yy1 = py[p2];

    // compute the arc midpoint
    t = (xx0 - xc) * (xx1 - xc) - (yy0 - yc) * (yy1 - yc);
    xm = splashSqrt((SplashCoord)0.5 * (r2 + t));
    ym = splashSqrt((SplashCoord)0.5 * (r2 - t));
    switch (quad) {
    case 0: xm = xc - xm;  ym = yc - ym;  break;
    case 1: xm = xc + xm;  ym = yc - ym;  break;
    case 2: xm = xc + xm;  ym = yc + ym;  break;
    case 3: xm = xc - xm;  ym = yc + ym;  break;
    }

    // compute distance from midpoint of straight segment to midpoint
    // of arc
    dx = (SplashCoord)0.5 * (xx0 + xx1) - xm;
    dy = (SplashCoord)0.5 * (yy0 + yy1) - ym;

    // if the arc is flat enough, or no more subdivisions are allowed,
    // add the straight line segment
    if (p2 - p1 == 1 || dx * dx + dy * dy <= flatness2) {
      addSegment(xx0, yy0, xx1, yy1,
		 p1 == 0 && first,
		 p2 == maxCurveSplits && last,
		 p1 == 0 && end0,
		 p2 == maxCurveSplits && end1);
      p1 = p2;

    // otherwise, subdivide the arc
    } else {
      p3 = (p1 + p2) / 2;
      px[p3] = xm;
      py[p3] = ym;
      pNext[p1] = p3;
      pNext[p3] = p2;
    }
  }
}

void SplashXPath::addSegment(SplashCoord x0, SplashCoord y0,
			     SplashCoord x1, SplashCoord y1,
			     GBool first, GBool last, GBool end0, GBool end1) {
  grow(1);
  segs[length].x0 = x0;
  segs[length].y0 = y0;
  segs[length].x1 = x1;
  segs[length].y1 = y1;
  segs[length].flags = 0;
  if (first) {
    segs[length].flags |= splashXPathFirst;
  }
  if (last) {
    segs[length].flags |= splashXPathLast;
  }
  if (end0) {
    segs[length].flags |= splashXPathEnd0;
  }
  if (end1) {
    segs[length].flags |= splashXPathEnd1;
  }
  if (y1 == y0) {
    segs[length].dxdy = segs[length].dydx = 0;
    segs[length].flags |= splashXPathHoriz;
    if (x1 == x0) {
      segs[length].flags |= splashXPathVert;
    }
  } else if (x1 == x0) {
    segs[length].dxdy = segs[length].dydx = 0;
    segs[length].flags |= splashXPathVert;
  } else {
    segs[length].dxdy = (x1 - x0) / (y1 - y0);
    segs[length].dydx = (SplashCoord)1 / segs[length].dxdy;
  }
  if (y0 > y1) {
    segs[length].flags |= splashXPathFlip;
  }
  ++length;
}

static int cmpXPathSegs(const void *arg0, const void *arg1) {
  SplashXPathSeg *seg0 = (SplashXPathSeg *)arg0;
  SplashXPathSeg *seg1 = (SplashXPathSeg *)arg1;
  SplashCoord x0, y0, x1, y1;

  if (seg0->flags & splashXPathFlip) {
    x0 = seg0->x1;
    y0 = seg0->y1;
  } else {
    x0 = seg0->x0;
    y0 = seg0->y0;
  }
  if (seg1->flags & splashXPathFlip) {
    x1 = seg1->x1;
    y1 = seg1->y1;
  } else {
    x1 = seg1->x0;
    y1 = seg1->y0;
  }
  if (y0 != y1) {
    return (y0 > y1) ? 1 : -1;
  }
  if (x0 != x1) {
    return (x0 > x1) ? 1 : -1;
  }
  return 0;
}

void SplashXPath::sort() {
  qsort(segs, length, sizeof(SplashXPathSeg), &cmpXPathSegs);
}
