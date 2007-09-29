//========================================================================
//
// SplashXPathScanner.cc
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdlib.h>
#include "goo/gmem.h"
#include "SplashMath.h"
#include "SplashXPath.h"
#include "SplashXPathScanner.h"

//------------------------------------------------------------------------

struct SplashIntersect {
  int x0, x1;			// intersection of segment with [y, y+1)
  int count;			// EO/NZWN counter increment
};

static int cmpIntersect(const void *p0, const void *p1) {
  return ((SplashIntersect *)p0)->x0 - ((SplashIntersect *)p1)->x0;
}

//------------------------------------------------------------------------
// SplashXPathScanner
//------------------------------------------------------------------------

SplashXPathScanner::SplashXPathScanner(SplashXPath *xPathA, GBool eoA) {
  SplashXPathSeg *seg;
  SplashCoord xMinFP, yMinFP, xMaxFP, yMaxFP;
  int i;

  xPath = xPathA;
  eo = eoA;

  // compute the bbox
  if (xPath->length == 0) {
    xMin = yMin = 1;
    xMax = yMax = 0;
  } else {
    seg = &xPath->segs[0];
    if (seg->x0 <= seg->x1) {
      xMinFP = seg->x0;
      xMaxFP = seg->x1;
    } else {
      xMinFP = seg->x1;
      xMaxFP = seg->x0;
    }
    if (seg->flags & splashXPathFlip) {
      yMinFP = seg->y1;
      yMaxFP = seg->y0;
    } else {
      yMinFP = seg->y0;
      yMaxFP = seg->y1;
    }
    for (i = 1; i < xPath->length; ++i) {
      seg = &xPath->segs[i];
      if (seg->x0 < xMinFP) {
	xMinFP = seg->x0;
      } else if (seg->x0 > xMaxFP) {
	xMaxFP = seg->x0;
      }
      if (seg->x1 < xMinFP) {
	xMinFP = seg->x1;
      } else if (seg->x1 > xMaxFP) {
	xMaxFP = seg->x1;
      }
      if (seg->flags & splashXPathFlip) {
	if (seg->y0 > yMaxFP) {
	  yMaxFP = seg->y0;
	}
      } else {
	if (seg->y1 > yMaxFP) {
	  yMaxFP = seg->y1;
	}
      }
    }
    xMin = splashFloor(xMinFP);
    xMax = splashFloor(xMaxFP);
    yMin = splashFloor(yMinFP);
    yMax = splashFloor(yMaxFP);
  }

  interY = yMin - 1;
  xPathIdx = 0;
  inter = NULL;
  interLen = interSize = 0;
}

SplashXPathScanner::~SplashXPathScanner() {
  gfree(inter);
}

void SplashXPathScanner::getSpanBounds(int y, int *spanXMin, int *spanXMax) {
  if (interY != y) {
    computeIntersections(y);
  }
  if (interLen > 0) {
    *spanXMin = inter[0].x0;
    *spanXMax = inter[interLen - 1].x1;
  } else {
    *spanXMin = xMax + 1;
    *spanXMax = xMax;
  }
}

GBool SplashXPathScanner::test(int x, int y) {
  int count, i;

  if (interY != y) {
    computeIntersections(y);
  }
  count = 0;
  for (i = 0; i < interLen && inter[i].x0 <= x; ++i) {
    if (x <= inter[i].x1) {
      return gTrue;
    }
    count += inter[i].count;
  }
  return eo ? (count & 1) : (count != 0);
}

GBool SplashXPathScanner::testSpan(int x0, int x1, int y) {
  int count, xx1, i;

  if (interY != y) {
    computeIntersections(y);
  }

  count = 0;
  for (i = 0; i < interLen && inter[i].x1 < x0; ++i) {
    count += inter[i].count;
  }

  // invariant: the subspan [x0,xx1] is inside the path
  xx1 = x0 - 1;
  while (xx1 < x1) {
    if (i >= interLen) {
      return gFalse;
    }
    if (inter[i].x0 > xx1 + 1 &&
	!(eo ? (count & 1) : (count != 0))) {
      return gFalse;
    }
    if (inter[i].x1 > xx1) {
      xx1 = inter[i].x1;
    }
    count += inter[i].count;
    ++i;
  }

  return gTrue;
}

GBool SplashXPathScanner::getNextSpan(int y, int *x0, int *x1) {
  int xx0, xx1;

  if (interY != y) {
    computeIntersections(y);
  }
  if (interIdx >= interLen) {
    return gFalse;
  }
  xx0 = inter[interIdx].x0;
  xx1 = inter[interIdx].x1;
  interCount += inter[interIdx].count;
  ++interIdx;
  while (interIdx < interLen &&
	 (inter[interIdx].x0 <= xx1 ||
	  (eo ? (interCount & 1) : (interCount != 0)))) {
    if (inter[interIdx].x1 > xx1) {
      xx1 = inter[interIdx].x1;
    }
    interCount += inter[interIdx].count;
    ++interIdx;
  }
  *x0 = xx0;
  *x1 = xx1;
  return gTrue;
}

void SplashXPathScanner::computeIntersections(int y) {
  SplashCoord xSegMin, xSegMax, ySegMin, ySegMax, xx0, xx1;
  SplashXPathSeg *seg;
  int i, j;

  // find the first segment that intersects [y, y+1)
  i = (y >= interY) ? xPathIdx : 0;
  while (i < xPath->length &&
	 xPath->segs[i].y0 < y && xPath->segs[i].y1 < y) {
    ++i;
  }
  xPathIdx = i;

  // find all of the segments that intersect [y, y+1) and create an
  // Intersect element for each one
  interLen = 0;
  for (j = i; j < xPath->length; ++j) {
    seg = &xPath->segs[j];
    if (seg->flags & splashXPathFlip) {
      ySegMin = seg->y1;
      ySegMax = seg->y0;
    } else {
      ySegMin = seg->y0;
      ySegMax = seg->y1;
    }

    // ensure that:      ySegMin < y+1
    //              y <= ySegMax
    if (ySegMin >= y + 1) {
      break;
    }
    if (ySegMax < y) {
      continue;
    }

    if (interLen == interSize) {
      if (interSize == 0) {
	interSize = 16;
      } else {
	interSize *= 2;
      }
      inter = (SplashIntersect *)greallocn(inter, interSize,
					   sizeof(SplashIntersect));
    }

    if (seg->flags & splashXPathHoriz) {
      xx0 = seg->x0;
      xx1 = seg->x1;
    } else if (seg->flags & splashXPathVert) {
      xx0 = xx1 = seg->x0;
    } else {
      if (seg->x0 < seg->x1) {
	xSegMin = seg->x0;
	xSegMax = seg->x1;
      } else {
	xSegMin = seg->x1;
	xSegMax = seg->x0;
      }
      // intersection with top edge
      xx0 = seg->x0 + ((SplashCoord)y - seg->y0) * seg->dxdy;
      // intersection with bottom edge
      xx1 = seg->x0 + ((SplashCoord)y + 1 - seg->y0) * seg->dxdy;
      // the segment may not actually extend to the top and/or bottom edges
      if (xx0 < xSegMin) {
	xx0 = xSegMin;
      } else if (xx0 > xSegMax) {
	xx0 = xSegMax;
      }
      if (xx1 < xSegMin) {
	xx1 = xSegMin;
      } else if (xx1 > xSegMax) {
	xx1 = xSegMax;
      }
    }
    if (xx0 < xx1) {
      inter[interLen].x0 = splashFloor(xx0);
      inter[interLen].x1 = splashFloor(xx1);
    } else {
      inter[interLen].x0 = splashFloor(xx1);
      inter[interLen].x1 = splashFloor(xx0);
    }
    if (ySegMin <= y &&
	(SplashCoord)y < ySegMax &&
	!(seg->flags & splashXPathHoriz)) {
      inter[interLen].count = eo ? 1
	                         : (seg->flags & splashXPathFlip) ? 1 : -1;
    } else {
      inter[interLen].count = 0;
    }
    ++interLen;
  }

  qsort(inter, interLen, sizeof(SplashIntersect), &cmpIntersect);

  interY = y;
  interIdx = 0;
  interCount = 0;
}
