//========================================================================
//
// SplashClip.cc
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdlib.h>
#include <string.h>
#include "goo/gmem.h"
#include "SplashErrorCodes.h"
#include "SplashMath.h"
#include "SplashPath.h"
#include "SplashXPath.h"
#include "SplashXPathScanner.h"
#include "SplashClip.h"

//------------------------------------------------------------------------
// SplashClip.flags
//------------------------------------------------------------------------

#define splashClipEO       0x01	// use even-odd rule

//------------------------------------------------------------------------
// SplashClip
//------------------------------------------------------------------------

SplashClip::SplashClip(SplashCoord x0, SplashCoord y0,
		       SplashCoord x1, SplashCoord y1) {
  if (x0 < x1) {
    xMin = splashFloor(x0);
    xMax = splashFloor(x1);
  } else {
    xMin = splashFloor(x1);
    xMax = splashFloor(x0);
  }
  if (y0 < y1) {
    yMin = splashFloor(y0);
    yMax = splashFloor(y1);
  } else {
    yMin = splashFloor(y1);
    yMax = splashFloor(y0);
  }
  paths = NULL;
  flags = NULL;
  scanners = NULL;
  length = size = 0;
}

SplashClip::SplashClip(SplashClip *clip) {
  int i;

  xMin = clip->xMin;
  yMin = clip->yMin;
  xMax = clip->xMax;
  yMax = clip->yMax;
  length = clip->length;
  size = clip->size;
  paths = (SplashXPath **)gmallocn(size, sizeof(SplashXPath *));
  flags = (Guchar *)gmallocn(size, sizeof(Guchar));
  scanners = (SplashXPathScanner **)
                 gmallocn(size, sizeof(SplashXPathScanner *));
  for (i = 0; i < length; ++i) {
    paths[i] = clip->paths[i]->copy();
    flags[i] = clip->flags[i];
    scanners[i] = new SplashXPathScanner(paths[i], flags[i] & splashClipEO);
  }
}

SplashClip::~SplashClip() {
  int i;

  for (i = 0; i < length; ++i) {
    delete paths[i];
    delete scanners[i];
  }
  gfree(paths);
  gfree(flags);
  gfree(scanners);
}

void SplashClip::grow(int nPaths) {
  if (length + nPaths > size) {
    if (size == 0) {
      size = 32;
    }
    while (size < length + nPaths) {
      size *= 2;
    }
    paths = (SplashXPath **)greallocn(paths, size, sizeof(SplashXPath *));
    flags = (Guchar *)greallocn(flags, size, sizeof(Guchar));
    scanners = (SplashXPathScanner **)
                   greallocn(scanners, size, sizeof(SplashXPathScanner *));
  }
}

void SplashClip::resetToRect(SplashCoord x0, SplashCoord y0,
			     SplashCoord x1, SplashCoord y1) {
  int i;

  for (i = 0; i < length; ++i) {
    delete paths[i];
    delete scanners[i];
  }
  gfree(paths);
  gfree(flags);
  gfree(scanners);
  paths = NULL;
  flags = NULL;
  scanners = NULL;
  length = size = 0;

  if (x0 < x1) {
    xMin = splashFloor(x0);
    xMax = splashFloor(x1);
  } else {
    xMin = splashFloor(x1);
    xMax = splashFloor(x0);
  }
  if (y0 < y1) {
    yMin = splashFloor(y0);
    yMax = splashFloor(y1);
  } else {
    yMin = splashFloor(y1);
    yMax = splashFloor(y0);
  }
}

SplashError SplashClip::clipToRect(SplashCoord x0, SplashCoord y0,
				   SplashCoord x1, SplashCoord y1) {
  int x0I, y0I, x1I, y1I;

  if (x0 < x1) {
    x0I = splashFloor(x0);
    x1I = splashFloor(x1);
  } else {
    x0I = splashFloor(x1);
    x1I = splashFloor(x0);
  }
  if (x0I > xMin) {
    xMin = x0I;
  }
  if (x1I < xMax) {
    xMax = x1I;
  }
  if (y0 < y1) {
    y0I = splashFloor(y0);
    y1I = splashFloor(y1);
  } else {
    y0I = splashFloor(y1);
    y1I = splashFloor(y0);
  }
  if (y0I > yMin) {
    yMin = y0I;
  }
  if (y1I < yMax) {
    yMax = y1I;
  }
  return splashOk;
}

SplashError SplashClip::clipToPath(SplashPath *path, SplashCoord flatness,
				   GBool eo) {
  SplashXPath *xPath;

  xPath = new SplashXPath(path, flatness, gTrue);

  // check for an empty path
  if (xPath->length == 0) {
    xMax = xMin - 1;
    yMax = yMin - 1;
    delete xPath;

  // check for a rectangle
  } else if (xPath->length == 4 &&
      ((xPath->segs[0].x0 == xPath->segs[0].x1 &&
	xPath->segs[0].x0 == xPath->segs[1].x0 &&
	xPath->segs[0].x0 == xPath->segs[3].x1 &&
	xPath->segs[2].x0 == xPath->segs[2].x1 &&
	xPath->segs[2].x0 == xPath->segs[1].x1 &&
	xPath->segs[2].x0 == xPath->segs[3].x0 &&
	xPath->segs[1].y0 == xPath->segs[1].y1 &&
	xPath->segs[1].y0 == xPath->segs[0].y1 &&
	xPath->segs[1].y0 == xPath->segs[2].y0 &&
	xPath->segs[3].y0 == xPath->segs[3].y1 &&
	xPath->segs[3].y0 == xPath->segs[0].y0 &&
	xPath->segs[3].y0 == xPath->segs[2].y1) ||
       (xPath->segs[0].y0 == xPath->segs[0].y1 &&
	xPath->segs[0].y0 == xPath->segs[1].y0 &&
	xPath->segs[0].y0 == xPath->segs[3].y1 &&
	xPath->segs[2].y0 == xPath->segs[2].y1 &&
	xPath->segs[2].y0 == xPath->segs[1].y1 &&
	xPath->segs[2].y0 == xPath->segs[3].y0 &&
	xPath->segs[1].x0 == xPath->segs[1].x1 &&
	xPath->segs[1].x0 == xPath->segs[0].x1 &&
	xPath->segs[1].x0 == xPath->segs[2].x0 &&
	xPath->segs[3].x0 == xPath->segs[3].x1 &&
	xPath->segs[3].x0 == xPath->segs[0].x0 &&
	xPath->segs[3].x0 == xPath->segs[2].x1))) {
    clipToRect(xPath->segs[0].x0, xPath->segs[0].y0,
	       xPath->segs[2].x0, xPath->segs[2].y0);
    delete xPath;

  } else {
    grow(1);
    xPath->sort();
    paths[length] = xPath;
    flags[length] = eo ? splashClipEO : 0;
    scanners[length] = new SplashXPathScanner(xPath, eo);
    ++length;
  }

  return splashOk;
}

GBool SplashClip::test(int x, int y) {
  int i;

  // check the rectangle
  if (x < xMin || x > xMax || y < yMin || y > yMax) {
    return gFalse;
  }

  // check the paths
  for (i = 0; i < length; ++i) {
    if (!scanners[i]->test(x, y)) {
      return gFalse;
    }
  }

  return gTrue;
}

SplashClipResult SplashClip::testRect(int rectXMin, int rectYMin,
				      int rectXMax, int rectYMax) {
  if (rectXMax < xMin || rectXMin > xMax ||
      rectYMax < yMin || rectYMin > yMax) {
    return splashClipAllOutside;
  }
  if (rectXMin >= xMin && rectXMax <= xMax &&
      rectYMin >= yMin && rectYMax <= yMax &&
      length == 0) {
    return splashClipAllInside;
  }
  return splashClipPartial;
}

SplashClipResult SplashClip::testSpan(int spanXMin, int spanXMax, int spanY) {
  int i;

  if (spanXMax < xMin || spanXMin > xMax ||
      spanY < yMin || spanY > yMax) {
    return splashClipAllOutside;
  }
  if (!(spanXMin >= xMin && spanXMax <= xMax &&
	spanY >= yMin && spanY <= yMax)) {
    return splashClipPartial;
  }
  for (i = 0; i < length; ++i) {
    if (!scanners[i]->testSpan(xMin, xMax, spanY)) {
      return splashClipPartial;
    }
  }
  return splashClipAllInside;
}
