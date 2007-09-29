//========================================================================
//
// Splash.cc
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
#include "SplashBitmap.h"
#include "SplashState.h"
#include "SplashPath.h"
#include "SplashXPath.h"
#include "SplashXPathScanner.h"
#include "SplashPattern.h"
#include "SplashScreen.h"
#include "SplashFont.h"
#include "SplashGlyphBitmap.h"
#include "Splash.h"

//------------------------------------------------------------------------

static void blendNormal(SplashColorPtr src, SplashColorPtr dest,
			SplashColorPtr blend, SplashColorMode cm) {
  int i;

  for (i = 0; i < splashColorModeNComps[cm]; ++i) {
    blend[i] = src[i];
  }
}

//------------------------------------------------------------------------
// Splash
//------------------------------------------------------------------------

Splash::Splash(SplashBitmap *bitmapA) {
  bitmap = bitmapA;
  state = new SplashState(bitmap->width, bitmap->height);
  softMask = NULL;
  clearModRegion();
  debugMode = gFalse;
}

Splash::~Splash() {
  while (state && state->next) {
    restoreState();
  }
  delete state;
  if (softMask) {
    delete softMask;
  }
}

//------------------------------------------------------------------------
// state read
//------------------------------------------------------------------------

SplashPattern *Splash::getStrokePattern() {
  return state->strokePattern;
}

SplashPattern *Splash::getFillPattern() {
  return state->fillPattern;
}

SplashScreen *Splash::getScreen() {
  return state->screen;
}

SplashBlendFunc Splash::getBlendFunc() {
  return state->blendFunc;
}

SplashCoord Splash::getStrokeAlpha() {
  return state->strokeAlpha;
}

SplashCoord Splash::getFillAlpha() {
  return state->fillAlpha;
}

SplashCoord Splash::getLineWidth() {
  return state->lineWidth;
}

int Splash::getLineCap() {
  return state->lineCap;
}

int Splash::getLineJoin() {
  return state->lineJoin;
}

SplashCoord Splash::getMiterLimit() {
  return state->miterLimit;
}

SplashCoord Splash::getFlatness() {
  return state->flatness;
}

SplashCoord *Splash::getLineDash() {
  return state->lineDash;
}

int Splash::getLineDashLength() {
  return state->lineDashLength;
}

SplashCoord Splash::getLineDashPhase() {
  return state->lineDashPhase;
}

SplashClip *Splash::getClip() {
  return state->clip;
}

//------------------------------------------------------------------------
// state write
//------------------------------------------------------------------------

void Splash::setStrokePattern(SplashPattern *strokePattern) {
  state->setStrokePattern(strokePattern);
}

void Splash::setFillPattern(SplashPattern *fillPattern) {
  state->setFillPattern(fillPattern);
}

void Splash::setScreen(SplashScreen *screen) {
  state->setScreen(screen);
}

void Splash::setBlendFunc(SplashBlendFunc func) {
  state->blendFunc = func;
}

void Splash::setStrokeAlpha(SplashCoord alpha) {
  state->strokeAlpha = alpha;
}

void Splash::setFillAlpha(SplashCoord alpha) {
  state->fillAlpha = alpha;
}

void Splash::setLineWidth(SplashCoord lineWidth) {
  state->lineWidth = lineWidth;
}

void Splash::setLineCap(int lineCap) {
  state->lineCap = lineCap;
}

void Splash::setLineJoin(int lineJoin) {
  state->lineJoin = lineJoin;
}

void Splash::setMiterLimit(SplashCoord miterLimit) {
  state->miterLimit = miterLimit;
}

void Splash::setFlatness(SplashCoord flatness) {
  if (flatness < 1) {
    state->flatness = 1;
  } else {
    state->flatness = flatness;
  }
}

void Splash::setLineDash(SplashCoord *lineDash, int lineDashLength,
			 SplashCoord lineDashPhase) {
  state->setLineDash(lineDash, lineDashLength, lineDashPhase);
}

void Splash::clipResetToRect(SplashCoord x0, SplashCoord y0,
			     SplashCoord x1, SplashCoord y1) {
  state->clip->resetToRect(x0, y0, x1, y1);
}

SplashError Splash::clipToRect(SplashCoord x0, SplashCoord y0,
			       SplashCoord x1, SplashCoord y1) {
  return state->clip->clipToRect(x0, y0, x1, y1);
}

SplashError Splash::clipToPath(SplashPath *path, GBool eo) {
  return state->clip->clipToPath(path, state->flatness, eo);
}

//------------------------------------------------------------------------
// state save/restore
//------------------------------------------------------------------------

void Splash::saveState() {
  SplashState *newState;

  newState = state->copy();
  newState->next = state;
  state = newState;
}

SplashError Splash::restoreState() {
  SplashState *oldState;

  if (!state->next) {
    return splashErrNoSave;
  }
  oldState = state;
  state = state->next;
  delete oldState;
  return splashOk;
}

//------------------------------------------------------------------------
// soft mask
//------------------------------------------------------------------------

void Splash::setSoftMask(SplashBitmap *softMaskA) {
  if (softMask) {
    delete softMask;
  }
  softMask = softMaskA;
}

//------------------------------------------------------------------------
// modified region
//------------------------------------------------------------------------

void Splash::clearModRegion() {
  modXMin = bitmap->getWidth();
  modYMin = bitmap->getHeight();
  modXMax = -1;
  modYMax = -1;
}

inline void Splash::updateModX(int x) {
  if (x < modXMin) {
    modXMin = x;
  }
  if (x > modXMax) {
    modXMax = x;
  }
}

inline void Splash::updateModY(int y) {
  if (y < modYMin) {
    modYMin = y;
  }
  if (y > modYMax) {
    modYMax = y;
  }
}

//------------------------------------------------------------------------
// drawing operations
//------------------------------------------------------------------------

void Splash::clear(SplashColorPtr color) {
  SplashColorPtr row, p;
  Guchar mono;
  int x, y;

  switch (bitmap->mode) {
  case splashModeMono1:
    mono = color[0] ? 0xff : 0x00;
    if (bitmap->rowSize < 0) {
      memset(bitmap->data + bitmap->rowSize * (bitmap->height - 1),
	     mono, -bitmap->rowSize * bitmap->height);
    } else {
      memset(bitmap->data, mono, bitmap->rowSize * bitmap->height);
    }
    break;
  case splashModeMono8:
    if (bitmap->rowSize < 0) {
      memset(bitmap->data + bitmap->rowSize * (bitmap->height - 1),
	     color[0], -bitmap->rowSize * bitmap->height);
    } else {
      memset(bitmap->data, color[0], bitmap->rowSize * bitmap->height);
    }
    break;
  case splashModeAMono8:
    if (color[0] == color[1]) {
      if (bitmap->rowSize < 0) {
	memset(bitmap->data + bitmap->rowSize * (bitmap->height - 1),
	       color[0], -bitmap->rowSize * bitmap->height);
      } else {
	memset(bitmap->data, color[0], bitmap->rowSize * bitmap->height);
      }
    } else {
      row = bitmap->data;
      for (y = 0; y < bitmap->height; ++y) {
	p = row;
	for (x = 0; x < bitmap->width; ++x) {
	  *p++ = color[0];
	  *p++ = color[1];
	}
	row += bitmap->rowSize;
      }
    }
    break;
  case splashModeRGB8:
  case splashModeBGR8:
    if (color[0] == color[1] && color[1] == color[2]) {
      if (bitmap->rowSize < 0) {
	memset(bitmap->data + bitmap->rowSize * (bitmap->height - 1),
	       color[0], -bitmap->rowSize * bitmap->height);
      } else {
	memset(bitmap->data, color[0], bitmap->rowSize * bitmap->height);
      }
    } else {
      row = bitmap->data;
      for (y = 0; y < bitmap->height; ++y) {
	p = row;
	for (x = 0; x < bitmap->width; ++x) {
	  *p++ = color[0];
	  *p++ = color[1];
	  *p++ = color[2];
	}
	row += bitmap->rowSize;
      }
    }
    break;
  case splashModeRGB8Qt:
    if (color[0] == color[1] && color[1] == color[2]) {
      if (bitmap->rowSize < 0) {
	memset(bitmap->data + bitmap->rowSize * (bitmap->height - 1),
	       color[0], -bitmap->rowSize * bitmap->height);
      } else {
	memset(bitmap->data, color[0], bitmap->rowSize * bitmap->height);
      }
    } else {
      row = bitmap->data;
      for (y = 0; y < bitmap->height; ++y) {
	p = row;
	for (x = 0; x < bitmap->width; ++x) {
	  *p++ = color[2];
	  *p++ = color[1];
	  *p++ = color[0];
	  *p++ = 255;
	}
	row += bitmap->rowSize;
      }
    }
    break;
  case splashModeARGB8:
  case splashModeBGRA8:
#if SPLASH_CMYK
  case splashModeCMYK8:
#endif
    if (color[0] == color[1] && color[1] == color[2] && color[2] == color[3]) {
      if (bitmap->rowSize < 0) {
	memset(bitmap->data + bitmap->rowSize * (bitmap->height - 1),
	       color[0], -bitmap->rowSize * bitmap->height);
      } else {
	memset(bitmap->data, color[0], bitmap->rowSize * bitmap->height);
      }
    } else {
      row = bitmap->data;
      for (y = 0; y < bitmap->height; ++y) {
	p = row;
	for (x = 0; x < bitmap->width; ++x) {
	  *p++ = color[0];
	  *p++ = color[1];
	  *p++ = color[2];
	  *p++ = color[3];
	}
	row += bitmap->rowSize;
      }
    }
    break;
#if SPLASH_CMYK
  case splashModeACMYK8:
    if (color[0] == color[1] && color[1] == color[2] &&
	color[2] == color[3] && color[3] == color[4]) {
      if (bitmap->rowSize < 0) {
	memset(bitmap->data + bitmap->rowSize * (bitmap->height - 1),
	       color[0], -bitmap->rowSize * bitmap->height);
      } else {
	memset(bitmap->data, color[0], bitmap->rowSize * bitmap->height);
      }
    } else {
      row = bitmap->data;
      for (y = 0; y < bitmap->height; ++y) {
	p = row;
	for (x = 0; x < bitmap->width; ++x) {
	  *p++ = color[0];
	  *p++ = color[1];
	  *p++ = color[2];
	  *p++ = color[3];
	  *p++ = color[4];
	}
	row += bitmap->rowSize;
      }
    }
    break;
#endif
  }

  updateModX(0);
  updateModY(0);
  updateModX(bitmap->width - 1);
  updateModY(bitmap->height - 1);
}

SplashError Splash::stroke(SplashPath *path) {
  SplashXPath *xPath, *xPath2;

  if (debugMode) {
    printf("stroke [dash:%d] [width:%.2f]:\n",
	   state->lineDashLength, (double)state->lineWidth);
    dumpPath(path);
  }
  opClipRes = splashClipAllOutside;
  if (path->length == 0) {
    return splashErrEmptyPath;
  }
  xPath = new SplashXPath(path, state->flatness, gFalse);
  if (xPath->length == 0) {
    delete xPath;
    return splashErrEmptyPath;
  }
  if (state->lineDashLength > 0) {
    xPath2 = makeDashedPath(xPath);
    delete xPath;
    xPath = xPath2;
  }
  if (state->lineWidth <= 1) {
    strokeNarrow(xPath);
  } else {
    strokeWide(xPath);
  }
  delete xPath;
  return splashOk;
}

void Splash::strokeNarrow(SplashXPath *xPath) {
  SplashXPathSeg *seg;
  int x0, x1, x2, x3, y0, y1, x, y, t;
  SplashCoord dx, dy, dxdy;
  SplashClipResult clipRes;
  int nClipRes[3] = {0, 0, 0};
  int i;

  for (i = 0, seg = xPath->segs; i < xPath->length; ++i, ++seg) {

    x0 = splashFloor(seg->x0);
    x1 = splashFloor(seg->x1);
    y0 = splashFloor(seg->y0);
    y1 = splashFloor(seg->y1);

    // horizontal segment
    if (y0 == y1) {
      if (x0 > x1) {
	t = x0; x0 = x1; x1 = t;
      }
      if ((clipRes = state->clip->testSpan(x0, x1, y0))
	  != splashClipAllOutside) {
	drawSpan(x0, x1, y0, state->strokePattern, state->strokeAlpha,
		 clipRes == splashClipAllInside);
      }

    // segment with |dx| > |dy|
    } else if (splashAbs(seg->dxdy) > 1) {
      dx = seg->x1 - seg->x0;
      dy = seg->y1 - seg->y0;
      dxdy = seg->dxdy;
      if (y0 > y1) {
	t = y0; y0 = y1; y1 = t;
	t = x0; x0 = x1; x1 = t;
	dx = -dx;
	dy = -dy;
      }
      if ((clipRes = state->clip->testRect(x0 <= x1 ? x0 : x1, y0,
					   x0 <= x1 ? x1 : x0, y1))
	  != splashClipAllOutside) {
	if (dx > 0) {
	  x2 = x0;
	  x3 = splashFloor(seg->x0 + ((SplashCoord)y0 + 1 - seg->y0) * dxdy);
	  drawSpan(x2, (x2 <= x3 - 1) ? x3 - 1 : x2, y0, state->strokePattern,
		   state->strokeAlpha, clipRes == splashClipAllInside);
	  x2 = x3;
	  for (y = y0 + 1; y <= y1 - 1; ++y) {
	    x3 = splashFloor(seg->x0 + ((SplashCoord)y + 1 - seg->y0) * dxdy);
	    drawSpan(x2, x3 - 1, y, state->strokePattern,
		     state->strokeAlpha, clipRes == splashClipAllInside);
	    x2 = x3;
	  }
	  drawSpan(x2, x2 <= x1 ? x1 : x2, y1, state->strokePattern,
		   state->strokeAlpha, clipRes == splashClipAllInside);
	} else {
	  x2 = x0;
	  x3 = splashFloor(seg->x0 + ((SplashCoord)y0 + 1 - seg->y0) * dxdy);
	  drawSpan((x3 + 1 <= x2) ? x3 + 1 : x2, x2, y0, state->strokePattern,
		   state->strokeAlpha, clipRes == splashClipAllInside);
	  x2 = x3;
	  for (y = y0 + 1; y <= y1 - 1; ++y) {
	    x3 = splashFloor(seg->x0 + ((SplashCoord)y + 1 - seg->y0) * dxdy);
	    drawSpan(x3 + 1, x2, y, state->strokePattern,
		     state->strokeAlpha, clipRes == splashClipAllInside);
	    x2 = x3;
	  }
	  drawSpan(x1, (x1 <= x2) ? x2 : x1, y1, state->strokePattern,
		   state->strokeAlpha, clipRes == splashClipAllInside);
	}
      }

    // segment with |dy| > |dx|
    } else {
      dxdy = seg->dxdy;
      if (y0 > y1) {
	t = x0; x0 = x1; x1 = t;
	t = y0; y0 = y1; y1 = t;
      }
      if ((clipRes = state->clip->testRect(x0 <= x1 ? x0 : x1, y0,
					   x0 <= x1 ? x1 : x0, y1))
	  != splashClipAllOutside) {
	drawPixel(x0, y0, state->strokePattern, state->strokeAlpha,
		  clipRes == splashClipAllInside);
	for (y = y0 + 1; y <= y1 - 1; ++y) {
	  x = splashFloor(seg->x0 + ((SplashCoord)y - seg->y0) * dxdy);
	  drawPixel(x, y, state->strokePattern, state->strokeAlpha,
		    clipRes == splashClipAllInside);
	}
	drawPixel(x1, y1, state->strokePattern, state->strokeAlpha,
		  clipRes == splashClipAllInside);
      }
    }
    ++nClipRes[clipRes];
  }
  if (nClipRes[splashClipPartial] ||
      (nClipRes[splashClipAllInside] && nClipRes[splashClipAllOutside])) {
    opClipRes = splashClipPartial;
  } else if (nClipRes[splashClipAllInside]) {
    opClipRes = splashClipAllInside;
  } else {
    opClipRes = splashClipAllOutside;
  }
}

void Splash::strokeWide(SplashXPath *xPath) {
  SplashXPathSeg *seg, *seg2;
  SplashPath *widePath;
  SplashCoord d, dx, dy, wdx, wdy, dxPrev, dyPrev, wdxPrev, wdyPrev;
  SplashCoord dotprod, miter;
  int i, j;

  dx = dy = wdx = wdy = 0; // make gcc happy
  dxPrev = dyPrev = wdxPrev = wdyPrev = 0; // make gcc happy

  for (i = 0, seg = xPath->segs; i < xPath->length; ++i, ++seg) {

    // save the deltas for the previous segment; if this is the first
    // segment on a subpath, compute the deltas for the last segment
    // on the subpath (which may be used to draw a line join)
    if (seg->flags & splashXPathFirst) {
      for (j = i + 1, seg2 = &xPath->segs[j]; j < xPath->length; ++j, ++seg2) {
	if (seg2->flags & splashXPathLast) {
	  d = splashDist(seg2->x0, seg2->y0, seg2->x1, seg2->y1);
	  if (d == 0) {
	    //~ not clear what the behavior should be for joins with d==0
	    dxPrev = 0;
	    dyPrev = 1;
	  } else {
	    d = (SplashCoord)1 / d;
	    dxPrev = d * (seg2->x1 - seg2->x0);
	    dyPrev = d * (seg2->y1 - seg2->y0);
	  }
	  wdxPrev = (SplashCoord)0.5 * state->lineWidth * dxPrev;
	  wdyPrev = (SplashCoord)0.5 * state->lineWidth * dyPrev;
	  break;
	}
      }
    } else {
      dxPrev = dx;
      dyPrev = dy;
      wdxPrev = wdx;
      wdyPrev = wdy;
    }

    // compute deltas for this line segment
    d = splashDist(seg->x0, seg->y0, seg->x1, seg->y1);
    if (d == 0) {
      // we need to draw end caps on zero-length lines
      //~ not clear what the behavior should be for splashLineCapButt with d==0
      dx = 0;
      dy = 1;
    } else {
      d = (SplashCoord)1 / d;
      dx = d * (seg->x1 - seg->x0);
      dy = d * (seg->y1 - seg->y0);
    }
    wdx = (SplashCoord)0.5 * state->lineWidth * dx;
    wdy = (SplashCoord)0.5 * state->lineWidth * dy;

    // initialize the path (which will be filled)
    widePath = new SplashPath();
    widePath->moveTo(seg->x0 - wdy, seg->y0 + wdx);

    // draw the start cap
    if (seg->flags & splashXPathEnd0) {
      switch (state->lineCap) {
      case splashLineCapButt:
	widePath->lineTo(seg->x0 + wdy, seg->y0 - wdx);
	break;
      case splashLineCapRound:
	widePath->arcCWTo(seg->x0 + wdy, seg->y0 - wdx, seg->x0, seg->y0);
	break;
      case splashLineCapProjecting:
	widePath->lineTo(seg->x0 - wdx - wdy, seg->y0 + wdx - wdy);
	widePath->lineTo(seg->x0 - wdx + wdy, seg->y0 - wdx - wdy);
	widePath->lineTo(seg->x0 + wdy, seg->y0 - wdx);
	break;
      }
    } else {
      widePath->lineTo(seg->x0 + wdy, seg->y0 - wdx);
    }

    // draw the left side of the segment
    widePath->lineTo(seg->x1 + wdy, seg->y1 - wdx);

    // draw the end cap
    if (seg->flags & splashXPathEnd1) {
      switch (state->lineCap) {
      case splashLineCapButt:
	widePath->lineTo(seg->x1 - wdy, seg->y1 + wdx);
	break;
      case splashLineCapRound:
	widePath->arcCWTo(seg->x1 - wdy, seg->y1 + wdx, seg->x1, seg->y1);
	break;
      case splashLineCapProjecting:
	widePath->lineTo(seg->x1 + wdx + wdy, seg->y1 - wdx + wdy);
	widePath->lineTo(seg->x1 + wdx - wdy, seg->y1 + wdx + wdy);
	widePath->lineTo(seg->x1 - wdy, seg->y1 + wdx);
	break;
      }
    } else {
      widePath->lineTo(seg->x1 - wdy, seg->y1 + wdx);
    }

    // draw the right side of the segment
    widePath->lineTo(seg->x0 - wdy, seg->y0 + wdx);

    // fill the segment
    fillWithPattern(widePath, gTrue, state->strokePattern, state->strokeAlpha);
    delete widePath;

    // draw the line join
    if (!(seg->flags & splashXPathEnd0)) {
      widePath = NULL;
      switch (state->lineJoin) {
      case splashLineJoinMiter:
	dotprod = -(dx * dxPrev + dy * dyPrev);
	if (splashAbs(splashAbs(dotprod) - 1) > 0.01) {
	  widePath = new SplashPath();
	  widePath->moveTo(seg->x0, seg->y0);
	  miter = (SplashCoord)2 / ((SplashCoord)1 - dotprod);
	  if (splashSqrt(miter) <= state->miterLimit) {
	    miter = splashSqrt(miter - 1);
	    if (dy * dxPrev > dx * dyPrev) {
	      widePath->lineTo(seg->x0 + wdyPrev, seg->y0 - wdxPrev);
	      widePath->lineTo(seg->x0 + wdy - miter * wdx,
			       seg->y0 - wdx - miter * wdy);
	      widePath->lineTo(seg->x0 + wdy, seg->y0 - wdx);
	    } else {
	      widePath->lineTo(seg->x0 - wdyPrev, seg->y0 + wdxPrev);
	      widePath->lineTo(seg->x0 - wdy - miter * wdx,
			       seg->y0 + wdx - miter * wdy);
	      widePath->lineTo(seg->x0 - wdy, seg->y0 + wdx);
	    }
	  } else {
	    if (dy * dxPrev > dx * dyPrev) {
	      widePath->lineTo(seg->x0 + wdyPrev, seg->y0 - wdxPrev);
	      widePath->lineTo(seg->x0 + wdy, seg->y0 - wdx);
	    } else {
	      widePath->lineTo(seg->x0 - wdyPrev, seg->y0 + wdxPrev);
	      widePath->lineTo(seg->x0 - wdy, seg->y0 + wdx);
	    }
	  }
	}
	break;
      case splashLineJoinRound:
	widePath = new SplashPath();
	widePath->moveTo(seg->x0 + wdy, seg->y0 - wdx);
	widePath->arcCWTo(seg->x0 + wdy, seg->y0 - wdx, seg->x0, seg->y0);
	break;
      case splashLineJoinBevel:
	widePath = new SplashPath();
	widePath->moveTo(seg->x0, seg->y0);
	if (dy * dxPrev > dx * dyPrev) {
	  widePath->lineTo(seg->x0 + wdyPrev, seg->y0 - wdxPrev);
	  widePath->lineTo(seg->x0 + wdy, seg->y0 - wdx);
	} else {
	  widePath->lineTo(seg->x0 - wdyPrev, seg->y0 + wdxPrev);
	  widePath->lineTo(seg->x0 - wdy, seg->y0 + wdx);
	}
	break;
      }
      if (widePath) {
	fillWithPattern(widePath, gTrue, state->strokePattern,
			state->strokeAlpha);
	delete widePath;
      }
    }
  }
}

SplashXPath *Splash::makeDashedPath(SplashXPath *xPath) {
  SplashXPath *dPath;
  GBool lineDashStartOn, lineDashOn;
  GBool atSegStart, atSegEnd, atDashStart, atDashEnd;
  int lineDashStartIdx, lineDashIdx, subpathStart;
  SplashCoord lineDashTotal, lineDashStartPhase, lineDashDist;
  int segIdx;
  SplashXPathSeg *seg;
  SplashCoord sx0, sy0, sx1, sy1, ax0, ay0, ax1, ay1, dist;
  int i;

  dPath = new SplashXPath();

  lineDashTotal = 0;
  for (i = 0; i < state->lineDashLength; ++i) {
    lineDashTotal += state->lineDash[i];
  }
  lineDashStartPhase = state->lineDashPhase;
  i = splashFloor(lineDashStartPhase / lineDashTotal);
  lineDashStartPhase -= (SplashCoord)i * lineDashTotal;
  lineDashStartOn = gTrue;
  lineDashStartIdx = 0;
  while (lineDashStartPhase >= state->lineDash[lineDashStartIdx]) {
    lineDashStartOn = !lineDashStartOn;
    lineDashStartPhase -= state->lineDash[lineDashStartIdx];
    ++lineDashStartIdx;
  }

  segIdx = 0;
  seg = xPath->segs;
  sx0 = seg->x0;
  sy0 = seg->y0;
  sx1 = seg->x1;
  sy1 = seg->y1;
  dist = splashDist(sx0, sy0, sx1, sy1);
  lineDashOn = lineDashStartOn;
  lineDashIdx = lineDashStartIdx;
  lineDashDist = state->lineDash[lineDashIdx] - lineDashStartPhase;
  atSegStart = gTrue;
  atDashStart = gTrue;
  subpathStart = dPath->length;

  while (segIdx < xPath->length) {

    ax0 = sx0;
    ay0 = sy0;
    if (dist <= lineDashDist) {
      ax1 = sx1;
      ay1 = sy1;
      lineDashDist -= dist;
      dist = 0;
      atSegEnd = gTrue;
      atDashEnd = lineDashDist == 0 || (seg->flags & splashXPathLast);
    } else {
      ax1 = sx0 + (lineDashDist / dist) * (sx1 - sx0);
      ay1 = sy0 + (lineDashDist / dist) * (sy1 - sy0);
      sx0 = ax1;
      sy0 = ay1;
      dist -= lineDashDist;
      lineDashDist = 0;
      atSegEnd = gFalse;
      atDashEnd = gTrue;
    }

    if (lineDashOn) {
      dPath->addSegment(ax0, ay0, ax1, ay1,
			atDashStart, atDashEnd,
			atDashStart, atDashEnd);
      // end of closed subpath
      if (atSegEnd &&
	  (seg->flags & splashXPathLast) &&
	  !(seg->flags & splashXPathEnd1)) {
	dPath->segs[subpathStart].flags &= ~splashXPathEnd0;
	dPath->segs[dPath->length - 1].flags &= ~splashXPathEnd1;
      }
    }

    if (atDashEnd) {
      lineDashOn = !lineDashOn;
      if (++lineDashIdx == state->lineDashLength) {
	lineDashIdx = 0;
      }
      lineDashDist = state->lineDash[lineDashIdx];
      atDashStart = gTrue;
    } else {
      atDashStart = gFalse;
    }
    if (atSegEnd) {
      if (++segIdx < xPath->length) {
	++seg;
	sx0 = seg->x0;
	sy0 = seg->y0;
	sx1 = seg->x1;
	sy1 = seg->y1;
	dist = splashDist(sx0, sy0, sx1, sy1);
	if (seg->flags & splashXPathFirst) {
	  lineDashOn = lineDashStartOn;
	  lineDashIdx = lineDashStartIdx;
	  lineDashDist = state->lineDash[lineDashIdx] - lineDashStartPhase;
	  atDashStart = gTrue;
	  subpathStart = dPath->length;
	}
      }
      atSegStart = gTrue;
    } else {
      atSegStart = gFalse;
    }
  }

  return dPath;
}

SplashError Splash::fill(SplashPath *path, GBool eo) {
  if (debugMode) {
    printf("fill [eo:%d]:\n", eo);
    dumpPath(path);
  }
  return fillWithPattern(path, eo, state->fillPattern, state->fillAlpha);
}

SplashError Splash::fillWithPattern(SplashPath *path, GBool eo,
				    SplashPattern *pattern,
				    SplashCoord alpha) {
  SplashXPath *xPath;
  SplashXPathScanner *scanner;
  int xMinI, yMinI, xMaxI, yMaxI, x0, x1, y;
  SplashClipResult clipRes, clipRes2;

  if (path->length == 0) {
    return splashErrEmptyPath;
  }
  xPath = new SplashXPath(path, state->flatness, gTrue);
  xPath->sort();
  scanner = new SplashXPathScanner(xPath, eo);

  // get the min and max x and y values
  scanner->getBBox(&xMinI, &yMinI, &xMaxI, &yMaxI);

  // check clipping
  if ((clipRes = state->clip->testRect(xMinI, yMinI, xMaxI, yMaxI))
      != splashClipAllOutside) {

    // limit the y range
    if (yMinI < state->clip->getYMin()) {
      yMinI = state->clip->getYMin();
    }
    if (yMaxI > state->clip->getYMax()) {
      yMaxI = state->clip->getYMax();
    }

    // draw the spans
    for (y = yMinI; y <= yMaxI; ++y) {
      while (scanner->getNextSpan(y, &x0, &x1)) {
	if (clipRes == splashClipAllInside) {
	  drawSpan(x0, x1, y, pattern, alpha, gTrue);
	} else {
	  // limit the x range
	  if (x0 < state->clip->getXMin()) {
	    x0 = state->clip->getXMin();
	  }
	  if (x1 > state->clip->getXMax()) {
	    x1 = state->clip->getXMax();
	  }
	  clipRes2 = state->clip->testSpan(x0, x1, y);
	  drawSpan(x0, x1, y, pattern, alpha, clipRes2 == splashClipAllInside);
	}
      }
    }
  }
  opClipRes = clipRes;

  delete scanner;
  delete xPath;
  return splashOk;
}

SplashError Splash::xorFill(SplashPath *path, GBool eo) {
  SplashXPath *xPath;
  SplashXPathScanner *scanner;
  int xMinI, yMinI, xMaxI, yMaxI, x0, x1, y;
  SplashClipResult clipRes, clipRes2;

  if (path->length == 0) {
    return splashErrEmptyPath;
  }
  xPath = new SplashXPath(path, state->flatness, gTrue);
  xPath->sort();
  scanner = new SplashXPathScanner(xPath, eo);

  // get the min and max x and y values
  scanner->getBBox(&xMinI, &yMinI, &xMaxI, &yMaxI);

  // check clipping
  if ((clipRes = state->clip->testRect(xMinI, yMinI, xMaxI, yMaxI))
      != splashClipAllOutside) {

    // limit the y range
    if (yMinI < state->clip->getYMin()) {
      yMinI = state->clip->getYMin();
    }
    if (yMaxI > state->clip->getYMax()) {
      yMaxI = state->clip->getYMax();
    }

    // draw the spans
    for (y = yMinI; y <= yMaxI; ++y) {
      while (scanner->getNextSpan(y, &x0, &x1)) {
	if (clipRes == splashClipAllInside) {
	  xorSpan(x0, x1, y, state->fillPattern, gTrue);
	} else {
	  // limit the x range
	  if (x0 < state->clip->getXMin()) {
	    x0 = state->clip->getXMin();
	  }
	  if (x1 > state->clip->getXMax()) {
	    x1 = state->clip->getXMax();
	  }
	  clipRes2 = state->clip->testSpan(x0, x1, y);
	  xorSpan(x0, x1, y, state->fillPattern,
		  clipRes2 == splashClipAllInside);
	}
      }
    }
  }
  opClipRes = clipRes;

  delete scanner;
  delete xPath;
  return splashOk;
}

void Splash::drawPixel(int x, int y, SplashColorPtr color,
		       SplashCoord alpha, GBool noClip) {
  SplashBlendFunc blendFunc;
  SplashColorPtr p;
  SplashColor dest, blend;
  int alpha2, ialpha2;
  Guchar t;

  if (noClip || state->clip->test(x, y)) {
    if (alpha != 1 || softMask || state->blendFunc) {
      blendFunc = state->blendFunc ? state->blendFunc : &blendNormal;
      if (softMask) {
	alpha2 = (int)(alpha * softMask->data[y * softMask->rowSize + x]);
      } else {
	alpha2 = (int)(alpha * 255);
      }
      ialpha2 = 255 - alpha2;
      switch (bitmap->mode) {
      case splashModeMono1:
	p = &bitmap->data[y * bitmap->rowSize + (x >> 3)];
	dest[0] = (*p >> (7 - (x & 7))) & 1;
	(*blendFunc)(color, dest, blend, bitmap->mode);
	t = (alpha2 * blend[0] + ialpha2 * dest[0]) >> 8;
	if (t) {
	  *p |= 0x80 >> (x & 7);
	} else {
	  *p &= ~(0x80 >> (x & 7));
	}
	break;
      case splashModeMono8:
	p = &bitmap->data[y * bitmap->rowSize + x];
	(*blendFunc)(color, p, blend, bitmap->mode);
	// note: floor(x / 255) = x >> 8 (for 16-bit x)
	p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	break;
      case splashModeAMono8:
	p = &bitmap->data[y * bitmap->rowSize + 2 * x];
	(*blendFunc)(color, p, blend, bitmap->mode);
	p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	break;
      case splashModeRGB8:
      case splashModeBGR8:
	p = &bitmap->data[y * bitmap->rowSize + 3 * x];
	(*blendFunc)(color, p, blend, bitmap->mode);
	p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	p[2] = (alpha2 * blend[2] + ialpha2 * p[2]) >> 8;
	break;
      case splashModeRGB8Qt:
	p = &bitmap->data[y * bitmap->rowSize + 4 * x];
	(*blendFunc)(color, p, blend, bitmap->mode);
	p[0] = (alpha2 * blend[2] + ialpha2 * p[0]) >> 8;
	p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	p[2] = (alpha2 * blend[0] + ialpha2 * p[2]) >> 8;
	break;
      case splashModeARGB8:
      case splashModeBGRA8:
#if SPLASH_CMYK
      case splashModeCMYK8:
#endif
	p = &bitmap->data[y * bitmap->rowSize + 4 * x];
	(*blendFunc)(color, p, blend, bitmap->mode);
	p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	p[2] = (alpha2 * blend[2] + ialpha2 * p[2]) >> 8;
	p[3] = (alpha2 * blend[3] + ialpha2 * p[3]) >> 8;
	break;
#if SPLASH_CMYK
      case splashModeACMYK8:
	p = &bitmap->data[y * bitmap->rowSize + 5 * x];
	(*blendFunc)(color, p, blend, bitmap->mode);
	p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	p[2] = (alpha2 * blend[2] + ialpha2 * p[2]) >> 8;
	p[3] = (alpha2 * blend[3] + ialpha2 * p[3]) >> 8;
	p[4] = (alpha2 * blend[4] + ialpha2 * p[4]) >> 8;
	break;
#endif
      }
    } else {
      switch (bitmap->mode) {
      case splashModeMono1:
	p = &bitmap->data[y * bitmap->rowSize + (x >> 3)];
	if (color[0]) {
	  *p |= 0x80 >> (x & 7);
	} else {
	  *p &= ~(0x80 >> (x & 7));
	}
	break;
      case splashModeMono8:
	p = &bitmap->data[y * bitmap->rowSize + x];
	p[0] = color[0];
	break;
      case splashModeAMono8:
	p = &bitmap->data[y * bitmap->rowSize + 2 * x];
	p[0] = color[0];
	p[1] = color[1];
	break;
      case splashModeRGB8:
      case splashModeBGR8:
	p = &bitmap->data[y * bitmap->rowSize + 3 * x];
	p[0] = color[0];
	p[1] = color[1];
	p[2] = color[2];
	break;
      case splashModeRGB8Qt:
	p = &bitmap->data[y * bitmap->rowSize + 4 * x];
	p[0] = color[2];
	p[1] = color[1];
	p[2] = color[0];
	break;
      case splashModeARGB8:
      case splashModeBGRA8:
#if SPLASH_CMYK
      case splashModeCMYK8:
#endif
	p = &bitmap->data[y * bitmap->rowSize + 4 * x];
	p[0] = color[0];
	p[1] = color[1];
	p[2] = color[2];
	p[3] = color[3];
	break;
#if SPLASH_CMYK
      case splashModeACMYK8:
	p = &bitmap->data[y * bitmap->rowSize + 5 * x];
	p[0] = color[0];
	p[1] = color[1];
	p[2] = color[2];
	p[3] = color[3];
	p[4] = color[4];
	break;
#endif
      }
    }
    updateModX(x);
    updateModY(y);
  }
}

void Splash::drawPixel(int x, int y, SplashPattern *pattern,
		       SplashCoord alpha, GBool noClip) {
  SplashBlendFunc blendFunc;
  SplashColor color;
  SplashColorPtr p;
  SplashColor dest, blend;
  int alpha2, ialpha2;
  Guchar t;

  if (noClip || state->clip->test(x, y)) {
    if (alpha != 1 || softMask || state->blendFunc) {
      blendFunc = state->blendFunc ? state->blendFunc : &blendNormal;
      pattern->getColor(x, y, color);
      if (softMask) {
	alpha2 = (int)(alpha * softMask->data[y * softMask->rowSize + x]);
      } else {
	alpha2 = (int)(alpha * 255);
      }
      ialpha2 = 255 - alpha2;
      switch (bitmap->mode) {
      case splashModeMono1:
	p = &bitmap->data[y * bitmap->rowSize + (x >> 3)];
	dest[0] = (*p >> (7 - (x & 7))) & 1;
	(*blendFunc)(color, dest, blend, bitmap->mode);
	t = (alpha2 * blend[0] + ialpha2 * dest[0]) >> 8;
	if (t) {
	  *p |= 0x80 >> (x & 7);
	} else {
	  *p &= ~(0x80 >> (x & 7));
	}
	break;
      case splashModeMono8:
	p = &bitmap->data[y * bitmap->rowSize + x];
	(*blendFunc)(color, p, blend, bitmap->mode);
	// note: floor(x / 255) = x >> 8 (for 16-bit x)
	p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	break;
      case splashModeAMono8:
	p = &bitmap->data[y * bitmap->rowSize + 2 * x];
	(*blendFunc)(color, p, blend, bitmap->mode);
	p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	break;
      case splashModeRGB8:
      case splashModeBGR8:
	p = &bitmap->data[y * bitmap->rowSize + 3 * x];
	(*blendFunc)(color, p, blend, bitmap->mode);
	p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	p[2] = (alpha2 * blend[2] + ialpha2 * p[2]) >> 8;
	break;
      case splashModeRGB8Qt:
	p = &bitmap->data[y * bitmap->rowSize + 4 * x];
	(*blendFunc)(color, p, blend, bitmap->mode);
	p[0] = (alpha2 * blend[2] + ialpha2 * p[0]) >> 8;
	p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	p[2] = (alpha2 * blend[0] + ialpha2 * p[2]) >> 8;
	break;
      case splashModeARGB8:
      case splashModeBGRA8:
#if SPLASH_CMYK
      case splashModeCMYK8:
#endif
	p = &bitmap->data[y * bitmap->rowSize + 4 * x];
	(*blendFunc)(color, p, blend, bitmap->mode);
	p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	p[2] = (alpha2 * blend[2] + ialpha2 * p[2]) >> 8;
	p[3] = (alpha2 * blend[3] + ialpha2 * p[3]) >> 8;
	break;
#if SPLASH_CMYK
      case splashModeACMYK8:
	p = &bitmap->data[y * bitmap->rowSize + 5 * x];
	(*blendFunc)(color, p, blend, bitmap->mode);
	p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	p[2] = (alpha2 * blend[2] + ialpha2 * p[2]) >> 8;
	p[3] = (alpha2 * blend[3] + ialpha2 * p[3]) >> 8;
	p[4] = (alpha2 * blend[4] + ialpha2 * p[4]) >> 8;
	break;
#endif
      }
    } else {
      pattern->getColor(x, y, color);
      switch (bitmap->mode) {
      case splashModeMono1:
	p = &bitmap->data[y * bitmap->rowSize + (x >> 3)];
	if (color[0]) {
	  *p |= 0x80 >> (x & 7);
	} else {
	  *p &= ~(0x80 >> (x & 7));
	}
	break;
      case splashModeMono8:
	p = &bitmap->data[y * bitmap->rowSize + x];
	p[0] = color[0];
	break;
      case splashModeAMono8:
	p = &bitmap->data[y * bitmap->rowSize + 2 * x];
	p[0] = color[0];
	p[1] = color[1];
	break;
      case splashModeRGB8:
      case splashModeBGR8:
	p = &bitmap->data[y * bitmap->rowSize + 3 * x];
	p[0] = color[0];
	p[1] = color[1];
	p[2] = color[2];
	break;
      case splashModeRGB8Qt:
	p = &bitmap->data[y * bitmap->rowSize + 4 * x];
	p[0] = color[2];
	p[1] = color[1];
	p[2] = color[0];
	break;
      case splashModeARGB8:
      case splashModeBGRA8:
#if SPLASH_CMYK
      case splashModeCMYK8:
#endif
	p = &bitmap->data[y * bitmap->rowSize + 4 * x];
	p[0] = color[0];
	p[1] = color[1];
	p[2] = color[2];
	p[3] = color[3];
	break;
#if SPLASH_CMYK
      case splashModeACMYK8:
	p = &bitmap->data[y * bitmap->rowSize + 5 * x];
	p[0] = color[0];
	p[1] = color[1];
	p[2] = color[2];
	p[3] = color[3];
	p[4] = color[4];
	break;
#endif
      }
    }
    updateModX(x);
    updateModY(y);
  }
}

void Splash::drawSpan(int x0, int x1, int y, SplashPattern *pattern,
		      SplashCoord alpha, GBool noClip) {
  SplashBlendFunc blendFunc;
  SplashColor color;
  SplashColorPtr p;
  SplashColor dest, blend;
  Guchar mask, t;
  int alpha2, ialpha2;
  int i, j, n;

  n = x1 - x0 + 1;

  if (noClip) {
    updateModX(x0);
    updateModX(x1);
    updateModY(y);
  }

  if (alpha != 1 || softMask || state->blendFunc) {
    blendFunc = state->blendFunc ? state->blendFunc : &blendNormal;
    if (softMask) {
      alpha2 = ialpha2 = 0; // make gcc happy
    } else {
      alpha2 = (int)(alpha * 255);
      ialpha2 = 255 - alpha2;
    }
    switch (bitmap->mode) {
    case splashModeMono1:
      p = &bitmap->data[y * bitmap->rowSize + (x0 >> 3)];
      i = 0;
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	if ((j = x0 & 7)) {
	  mask = 0x80 >> j;
	  for (; j < 8 && i < n; ++i, ++j) {
	    if (noClip || state->clip->test(x0 + i, y)) {
	      if (softMask) {
		alpha2 = (int)(alpha *
			       softMask->data[y * softMask->rowSize + x0 + i]);
		ialpha2 = 255 - alpha2;
	      }
	      dest[0] = (*p >> (7 - j)) & 1;
	      (*blendFunc)(color, dest, blend, bitmap->mode);
	      t = (alpha2 * blend[0] + ialpha2 * dest[0]) >> 8;
	      if (t) {
		*p |= mask;
	      } else {
		*p &= ~mask;
	      }
	      if (!noClip) {
		updateModX(x0 + i);
		updateModY(y);
	      }
	    }
	    mask >>= 1;
	  }
	  ++p;
	}
	while (i < n) {
	  mask = 0x80;
	  for (j = 0; j < 8 && i < n; ++i, ++j) {
	    if (noClip || state->clip->test(x0 + i, y)) {
	      if (softMask) {
		alpha2 = (int)(alpha *
			       softMask->data[y * softMask->rowSize + x0 + i]);
		ialpha2 = 255 - alpha2;
	      }
	      dest[0] = (*p >> (7 - j)) & 1;
	      (*blendFunc)(color, dest, blend, bitmap->mode);
	      t = (alpha2 * blend[0] + ialpha2 * dest[0]) >> 8;
	      if (t) {
		*p |= mask;
	      } else {
		*p &= ~mask;
	      }
	      if (!noClip) {
		updateModX(x0 + i);
		updateModY(y);
	      }
	    }
	    mask >>= 1;
	  }
	  ++p;
	}
      } else {
	if ((j = x0 & 7)) {
	  mask = 0x80 >> j;
	  for (; j < 8 && i < n; ++i, ++j) {
	    if (noClip || state->clip->test(x0 + i, y)) {
	      pattern->getColor(x0 + i, y, color);
	      if (softMask) {
		alpha2 = (int)(alpha *
			       softMask->data[y * softMask->rowSize + x0 + i]);
		ialpha2 = 255 - alpha2;
	      }
	      dest[0] = (*p >> (7 - j)) & 1;
	      (*blendFunc)(color, dest, blend, bitmap->mode);
	      t = (alpha2 * blend[0] + ialpha2 * dest[0]) >> 8;
	      if (t) {
		*p |= mask;
	      } else {
		*p &= ~mask;
	      }
	      if (!noClip) {
		updateModX(x0 + i);
		updateModY(y);
	      }
	    }
	    mask >>= 1;
	  }
	  ++p;
	}
	while (i < n) {
	  mask = 0x80;
	  for (j = 0; j < 8 && i < n; ++i, ++j) {
	    if (noClip || state->clip->test(x0 + i, y)) {
	      pattern->getColor(x0 + i, y, color);
	      if (softMask) {
		alpha2 = (int)(alpha *
			       softMask->data[y * softMask->rowSize + x0 + i]);
		ialpha2 = 255 - alpha2;
	      }
	      dest[0] = (*p >> (7 - j)) & 1;
	      (*blendFunc)(color, dest, blend, bitmap->mode);
	      t = (alpha2 * blend[0] + ialpha2 * dest[0]) >> 8;
	      if (t) {
		*p |= mask;
	      } else {
		*p &= ~mask;
	      }
	      if (!noClip) {
		updateModX(x0 + i);
		updateModY(y);
	      }
	    }
	    mask >>= 1;
	  }
	  ++p;
	}
      }
      break;

    case splashModeMono8:
      p = &bitmap->data[y * bitmap->rowSize + x0];
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    if (softMask) {
	      alpha2 = (int)(alpha *
			     softMask->data[y * softMask->rowSize + x0 + i]);
	      ialpha2 = 255 - alpha2;
	    }
	    (*blendFunc)(color, p, blend, bitmap->mode);
	    *p = (alpha2 * blend[0] + ialpha2 * *p) >> 8;
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  ++p;
	}
      } else {
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    pattern->getColor(x0 + i, y, color);
	    if (softMask) {
	      alpha2 = (int)(alpha *
			     softMask->data[y * softMask->rowSize + x0 + i]);
	      ialpha2 = 255 - alpha2;
	    }
	    (*blendFunc)(color, p, blend, bitmap->mode);
	    *p = (alpha2 * blend[0] + ialpha2 * *p) >> 8;
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  ++p;
	}
      }
      break;

    case splashModeAMono8:
      p = &bitmap->data[y * bitmap->rowSize + 2 * x0];
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    if (softMask) {
	      alpha2 = (int)(alpha *
			     softMask->data[y * softMask->rowSize + x0 + i]);
	      ialpha2 = 255 - alpha2;
	    }
	    (*blendFunc)(color, p, blend, bitmap->mode);
	    p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	    p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 2;
	}
      } else {
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    pattern->getColor(x0 + i, y, color);
	    if (softMask) {
	      alpha2 = (int)(alpha *
			     softMask->data[y * softMask->rowSize + x0 + i]);
	      ialpha2 = 255 - alpha2;
	    }
	    (*blendFunc)(color, p, blend, bitmap->mode);
	    p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	    p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 2;
	}
      }
      break;

    case splashModeRGB8:
    case splashModeBGR8:
      p = &bitmap->data[y * bitmap->rowSize + 3 * x0];
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    if (softMask) {
	      alpha2 = (int)(alpha *
			     softMask->data[y * softMask->rowSize + x0 + i]);
	      ialpha2 = 255 - alpha2;
	    }
	    (*blendFunc)(color, p, blend, bitmap->mode);
	    p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	    p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	    p[2] = (alpha2 * blend[2] + ialpha2 * p[2]) >> 8;
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 3;
	}
      } else {
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    pattern->getColor(x0 + i, y, color);
	    if (softMask) {
	      alpha2 = (int)(alpha *
			     softMask->data[y * softMask->rowSize + x0 + i]);
	      ialpha2 = 255 - alpha2;
	    }
	    (*blendFunc)(color, p, blend, bitmap->mode);
	    p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	    p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	    p[2] = (alpha2 * blend[2] + ialpha2 * p[2]) >> 8;
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 3;
	}
      }
      break;

    case splashModeRGB8Qt:
      p = &bitmap->data[y * bitmap->rowSize + 4 * x0];
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    if (softMask) {
	      alpha2 = (int)(alpha *
			     softMask->data[y * softMask->rowSize + x0 + i]);
	      ialpha2 = 255 - alpha2;
	    }
	    (*blendFunc)(color, p, blend, bitmap->mode);
	    p[0] = (alpha2 * blend[2] + ialpha2 * p[0]) >> 8;
	    p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	    p[2] = (alpha2 * blend[0] + ialpha2 * p[2]) >> 8;
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 4;
	}
      } else {
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    pattern->getColor(x0 + i, y, color);
	    if (softMask) {
	      alpha2 = (int)(alpha *
			     softMask->data[y * softMask->rowSize + x0 + i]);
	      ialpha2 = 255 - alpha2;
	    }
	    (*blendFunc)(color, p, blend, bitmap->mode);
	    p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	    p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	    p[2] = (alpha2 * blend[2] + ialpha2 * p[2]) >> 8;
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 4;
	}
      }
      break;

    case splashModeARGB8:
    case splashModeBGRA8:
#if SPLASH_CMYK
    case splashModeCMYK8:
#endif
      p = &bitmap->data[y * bitmap->rowSize + 4 * x0];
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    if (softMask) {
	      alpha2 = (int)(alpha *
			     softMask->data[y * softMask->rowSize + x0 + i]);
	      ialpha2 = 255 - alpha2;
	    }
	    (*blendFunc)(color, p, blend, bitmap->mode);
	    p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	    p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	    p[2] = (alpha2 * blend[2] + ialpha2 * p[2]) >> 8;
	    p[3] = (alpha2 * blend[3] + ialpha2 * p[3]) >> 8;
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 4;
	}
      } else {
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    pattern->getColor(x0 + i, y, color);
	    if (softMask) {
	      alpha2 = (int)(alpha *
			     softMask->data[y * softMask->rowSize + x0 + i]);
	      ialpha2 = 255 - alpha2;
	    }
	    (*blendFunc)(color, p, blend, bitmap->mode);
	    p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	    p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	    p[2] = (alpha2 * blend[2] + ialpha2 * p[2]) >> 8;
	    p[3] = (alpha2 * blend[3] + ialpha2 * p[3]) >> 8;
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 4;
	}
      }
      break;
#if SPLASH_CMYK
    case splashModeACMYK8:
      p = &bitmap->data[y * bitmap->rowSize + 5 * x0];
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    if (softMask) {
	      alpha2 = (int)(alpha *
			     softMask->data[y * softMask->rowSize + x0 + i]);
	      ialpha2 = 255 - alpha2;
	    }
	    (*blendFunc)(color, p, blend, bitmap->mode);
	    p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	    p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	    p[2] = (alpha2 * blend[2] + ialpha2 * p[2]) >> 8;
	    p[3] = (alpha2 * blend[3] + ialpha2 * p[3]) >> 8;
	    p[4] = (alpha2 * blend[4] + ialpha2 * p[4]) >> 8;
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 4;
	}
      } else {
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    pattern->getColor(x0 + i, y, color);
	    if (softMask) {
	      alpha2 = (int)(alpha *
			     softMask->data[y * softMask->rowSize + x0 + i]);
	      ialpha2 = 255 - alpha2;
	    }
	    (*blendFunc)(color, p, blend, bitmap->mode);
	    p[0] = (alpha2 * blend[0] + ialpha2 * p[0]) >> 8;
	    p[1] = (alpha2 * blend[1] + ialpha2 * p[1]) >> 8;
	    p[2] = (alpha2 * blend[2] + ialpha2 * p[2]) >> 8;
	    p[3] = (alpha2 * blend[3] + ialpha2 * p[3]) >> 8;
	    p[4] = (alpha2 * blend[4] + ialpha2 * p[4]) >> 8;
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 4;
	}
      }
      break;
#endif
    }

  } else {
    switch (bitmap->mode) {
    case splashModeMono1:
      p = &bitmap->data[y * bitmap->rowSize + (x0 >> 3)];
      i = 0;
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	if ((j = x0 & 7)) {
	  mask = 0x80 >> j;
	  for (; j < 8 && i < n; ++i, ++j) {
	    if (noClip || state->clip->test(x0 + i, y)) {
	      if (color[0]) {
		*p |= mask;
	      } else {
		*p &= ~mask;
	      }
	      if (!noClip) {
		updateModX(x0 + i);
		updateModY(y);
	      }
	    }
	    mask >>= 1;
	  }
	  ++p;
	}
	while (i < n) {
	  mask = 0x80;
	  for (j = 0; j < 8 && i < n; ++i, ++j) {
	    if (noClip || state->clip->test(x0 + i, y)) {
	      if (color[0]) {
		*p |= mask;
	      } else {
		*p &= ~mask;
	      }
	      if (!noClip) {
		updateModX(x0 + i);
		updateModY(y);
	      }
	    }
	    mask >>= 1;
	  }
	  ++p;
	}
      } else {
	if ((j = x0 & 7)) {
	  mask = 0x80 >> j;
	  for (; j < 8 && i < n; ++i, ++j) {
	    if (noClip || state->clip->test(x0 + i, y)) {
	      pattern->getColor(x0 + i, y, color);
	      if (color[0]) {
		*p |= mask;
	      } else {
		*p &= ~mask;
	      }
	      if (!noClip) {
		updateModX(x0 + i);
		updateModY(y);
	      }
	    }
	    mask >>= 1;
	  }
	  ++p;
	}
	while (i < n) {
	  mask = 0x80;
	  for (j = 0; j < 8 && i < n; ++i, ++j) {
	    if (noClip || state->clip->test(x0 + i, y)) {
	      pattern->getColor(x0 + i, y, color);
	      if (color[0]) {
		*p |= mask;
	      } else {
		*p &= ~mask;
	      }
	      if (!noClip) {
		updateModX(x0 + i);
		updateModY(y);
	      }
	    }
	    mask >>= 1;
	  }
	  ++p;
	}
      }
      break;

    case splashModeMono8:
      p = &bitmap->data[y * bitmap->rowSize + x0];
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    *p = color[0];
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  ++p;
	}
      } else {
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    pattern->getColor(x0 + i, y, color);
	    *p = color[0];
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  ++p;
	}
      }
      break;

    case splashModeAMono8:
      p = &bitmap->data[y * bitmap->rowSize + 2 * x0];
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    p[0] = color[0];
	    p[1] = color[1];
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 2;
	}
      } else {
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    pattern->getColor(x0 + i, y, color);
	    p[0] = color[0];
	    p[1] = color[1];
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 2;
	}
      }
      break;

    case splashModeRGB8:
    case splashModeBGR8:
      p = &bitmap->data[y * bitmap->rowSize + 3 * x0];
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    p[0] = color[0];
	    p[1] = color[1];
	    p[2] = color[2];
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 3;
	}
      } else {
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    pattern->getColor(x0 + i, y, color);
	    p[0] = color[0];
	    p[1] = color[1];
	    p[2] = color[2];
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 3;
	}
      }
      break;

    case splashModeRGB8Qt:
      p = &bitmap->data[y * bitmap->rowSize + 4 * x0];
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    p[0] = color[2];
	    p[1] = color[1];
	    p[2] = color[0];
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 4;
	}
      } else {
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    pattern->getColor(x0 + i, y, color);
	    p[0] = color[0];
	    p[1] = color[1];
	    p[2] = color[2];
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 4;
	}
      }
      break;

    case splashModeARGB8:
    case splashModeBGRA8:
#if SPLASH_CMYK
    case splashModeCMYK8:
#endif
      p = &bitmap->data[y * bitmap->rowSize + 4 * x0];
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    p[0] = color[0];
	    p[1] = color[1];
	    p[2] = color[2];
	    p[3] = color[3];
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 4;
	}
      } else {
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    pattern->getColor(x0 + i, y, color);
	    p[0] = color[0];
	    p[1] = color[1];
	    p[2] = color[2];
	    p[3] = color[3];
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 4;
	}
      }
      break;
#if SPLASH_CMYK
    case splashModeACMYK8:
      p = &bitmap->data[y * bitmap->rowSize + 5 * x0];
      if (pattern->isStatic()) {
	pattern->getColor(0, 0, color);
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    p[0] = color[0];
	    p[1] = color[1];
	    p[2] = color[2];
	    p[3] = color[3];
	    p[4] = color[4];
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 4;
	}
      } else {
	for (i = 0; i < n; ++i) {
	  if (noClip || state->clip->test(x0 + i, y)) {
	    pattern->getColor(x0 + i, y, color);
	    p[0] = color[0];
	    p[1] = color[1];
	    p[2] = color[2];
	    p[3] = color[3];
	    p[4] = color[4];
	    if (!noClip) {
	      updateModX(x0 + i);
	      updateModY(y);
	    }
	  }
	  p += 4;
	}
      }
      break;
#endif
    }
  }
}

void Splash::xorSpan(int x0, int x1, int y, SplashPattern *pattern,
		     GBool noClip) {
  SplashColor color;
  SplashColorPtr p;
  Guchar mask;
  int i, j, n;

  n = x1 - x0 + 1;

  if (noClip) {
    updateModX(x0);
    updateModX(x1);
    updateModY(y);
  }

  switch (bitmap->mode) {
  case splashModeMono1:
    p = &bitmap->data[y * bitmap->rowSize + (x0 >> 3)];
    i = 0;
    if ((j = x0 & 7)) {
      mask = 0x80 >> j;
      for (; j < 8 && i < n; ++i, ++j) {
	if (noClip || state->clip->test(x0 + i, y)) {
	  pattern->getColor(x0 + i, y, color);
	  if (color[0]) {
	    *p ^= mask;
	  }
	  if (!noClip) {
	    updateModX(x0 + i);
	    updateModY(y);
	  }
	}
	mask >>= 1;
      }
      ++p;
    }
    while (i < n) {
      mask = 0x80;
      for (j = 0; j < 8 && i < n; ++i, ++j) {
	if (noClip || state->clip->test(x0 + i, y)) {
	  pattern->getColor(x0 + i, y, color);
	  if (color[0]) {
	    *p ^= mask;
	  }
	  if (!noClip) {
	    updateModX(x0 + i);
	    updateModY(y);
	  }
	}
	mask >>= 1;
      }
      ++p;
    }
    break;

  case splashModeMono8:
    p = &bitmap->data[y * bitmap->rowSize + x0];
    for (i = 0; i < n; ++i) {
      if (noClip || state->clip->test(x0 + i, y)) {
	pattern->getColor(x0 + i, y, color);
	*p ^= color[0];
	if (!noClip) {
	  updateModX(x0 + i);
	  updateModY(y);
	}
      }
      ++p;
    }
    break;

  case splashModeAMono8:
    p = &bitmap->data[y * bitmap->rowSize + 2 * x0];
    for (i = 0; i < n; ++i) {
      if (noClip || state->clip->test(x0 + i, y)) {
	pattern->getColor(x0 + i, y, color);
	p[0] ^= color[0];
	p[1] ^= color[1];
	if (!noClip) {
	  updateModX(x0 + i);
	  updateModY(y);
	}
      }
      p += 2;
    }
    break;

  case splashModeRGB8:
  case splashModeBGR8:
    p = &bitmap->data[y * bitmap->rowSize + 3 * x0];
    for (i = 0; i < n; ++i) {
      if (noClip || state->clip->test(x0 + i, y)) {
	pattern->getColor(x0 + i, y, color);
	p[0] ^= color[0];
	p[1] ^= color[1];
	p[2] ^= color[2];
	if (!noClip) {
	  updateModX(x0 + i);
	  updateModY(y);
	}
      }
      p += 3;
    }
    break;

  case splashModeRGB8Qt:
    p = &bitmap->data[y * bitmap->rowSize + 4 * x0];
    for (i = 0; i < n; ++i) {
      if (noClip || state->clip->test(x0 + i, y)) {
	pattern->getColor(x0 + i, y, color);
	p[0] ^= color[2];
	p[1] ^= color[1];
	p[2] ^= color[0];
	if (!noClip) {
	  updateModX(x0 + i);
	  updateModY(y);
	}
      }
      p += 4;
    }
    break;

  case splashModeARGB8:
  case splashModeBGRA8:
#if SPLASH_CMYK
  case splashModeCMYK8:
#endif
    p = &bitmap->data[y * bitmap->rowSize + 4 * x0];
    for (i = 0; i < n; ++i) {
      if (noClip || state->clip->test(x0 + i, y)) {
	pattern->getColor(x0 + i, y, color);
	p[0] ^= color[0];
	p[1] ^= color[1];
	p[2] ^= color[2];
	p[3] ^= color[3];
	if (!noClip) {
	  updateModX(x0 + i);
	  updateModY(y);
	}
      }
      p += 4;
    }
    break;
#if SPLASH_CMYK
  case splashModeACMYK8:
    p = &bitmap->data[y * bitmap->rowSize + 5 * x0];
    for (i = 0; i < n; ++i) {
      if (noClip || state->clip->test(x0 + i, y)) {
	pattern->getColor(x0 + i, y, color);
	p[0] ^= color[0];
	p[1] ^= color[1];
	p[2] ^= color[2];
	p[3] ^= color[3];
	p[4] ^= color[4];
	if (!noClip) {
	  updateModX(x0 + i);
	  updateModY(y);
	}
      }
      p += 4;
    }
    break;
#endif
  }
}

SplashError Splash::fillChar(SplashCoord x, SplashCoord y,
			     int c, SplashFont *font) {
  SplashGlyphBitmap glyph;
  int x0, y0, xFrac, yFrac;
  SplashError err;

  if (debugMode) {
    printf("fillChar: x=%.2f y=%.2f c=%3d=0x%02x='%c'\n",
	   (double)x, (double)y, c, c, c);
  }
  x0 = splashFloor(x);
  xFrac = splashFloor((x - x0) * splashFontFraction);
  y0 = splashFloor(y);
  yFrac = splashFloor((y - y0) * splashFontFraction);
  if (!font->getGlyph(c, xFrac, yFrac, &glyph)) {
    return splashErrNoGlyph;
  }
  err = fillGlyph(x, y, &glyph);
  if (glyph.freeData) {
    gfree(glyph.data);
  }
  return err;
}

SplashError Splash::fillGlyph(SplashCoord x, SplashCoord y,
			      SplashGlyphBitmap *glyph) {
  SplashBlendFunc blendFunc;
  int alpha0, alpha, ialpha;
  Guchar *p;
  SplashColor fg, dest, blend;
  SplashColorPtr pix;
  SplashClipResult clipRes;
  GBool noClip;
  Guchar t;
  int x0, y0, x1, y1, xx, xx1, yy;

  x0 = splashFloor(x);
  y0 = splashFloor(y);

  if ((clipRes = state->clip->testRect(x0 - glyph->x,
				       y0 - glyph->y,
				       x0 - glyph->x + glyph->w - 1,
				       y0 - glyph->y + glyph->h - 1))
      != splashClipAllOutside) {
    noClip = clipRes == splashClipAllInside;

    if (noClip) {
      updateModX(x0 - glyph->x);
      updateModX(x0 - glyph->x + glyph->w - 1);
      updateModY(y0 - glyph->y);
      updateModY(y0 - glyph->y + glyph->h - 1);
    }

    //~ optimize this
    if (state->fillAlpha != 1 || softMask || state->blendFunc) {
      blendFunc = state->blendFunc ? state->blendFunc : &blendNormal;
      if (glyph->aa) {
	p = glyph->data;
	for (yy = 0, y1 = y0 - glyph->y; yy < glyph->h; ++yy, ++y1) {
	  for (xx = 0, x1 = x0 - glyph->x; xx < glyph->w; ++xx, ++x1) {
	    alpha = *p++;
	    if (softMask) {
	      alpha = (int)(alpha * (float)state->fillAlpha *
			    softMask->data[y1 * softMask->rowSize + x1]);
	    } else {
	      alpha = (int)(alpha * (float)state->fillAlpha);
	    }
	    if (alpha > 0) {
	      if (noClip || state->clip->test(x1, y1)) {
		ialpha = 255 - alpha;
		state->fillPattern->getColor(x1, y1, fg);
		switch (bitmap->mode) {
		case splashModeMono1:
		  pix = &bitmap->data[y1 * bitmap->rowSize + (x1 >> 3)];
		  dest[0] = (*pix >> (7 - (x1 & 7))) & 1;
		  (*blendFunc)(fg, dest, blend, bitmap->mode);
		  t = (alpha * blend[0] + ialpha * dest[0]) >> 8;
		  if (t) {
		    *pix |= 0x80 >> (x1 & 7);
		  } else {
		    *pix &= ~(0x80 >> (x1 & 7));
		  }
		  break;
		case splashModeMono8:
		  pix = &bitmap->data[y1 * bitmap->rowSize + x1];
		  (*blendFunc)(fg, pix, blend, bitmap->mode);
		  // note: floor(x / 255) = x >> 8 (for 16-bit x)
		  pix[0] = (alpha * blend[0] + ialpha * pix[0]) >> 8;
		  break;
		case splashModeAMono8:
		  pix = &bitmap->data[y1 * bitmap->rowSize + 2 * x1];
		  (*blendFunc)(fg, pix, blend, bitmap->mode);
		  pix[0] = (alpha * blend[0] + ialpha * pix[0]) >> 8;
		  pix[1] = (alpha * blend[1] + ialpha * pix[1]) >> 8;
		  break;
		case splashModeRGB8:
		case splashModeBGR8:
		  pix = &bitmap->data[y1 * bitmap->rowSize + 3 * x1];
		  (*blendFunc)(fg, pix, blend, bitmap->mode);
		  pix[0] = (alpha * blend[0] + ialpha * pix[0]) >> 8;
		  pix[1] = (alpha * blend[1] + ialpha * pix[1]) >> 8;
		  pix[2] = (alpha * blend[2] + ialpha * pix[2]) >> 8;
		  break;
		case splashModeRGB8Qt:
		  pix = &bitmap->data[y1 * bitmap->rowSize + 4 * x1];
		  (*blendFunc)(fg, pix, blend, bitmap->mode);
		  pix[0] = (alpha * blend[2] + ialpha * pix[0]) >> 8;
		  pix[1] = (alpha * blend[1] + ialpha * pix[1]) >> 8;
		  pix[2] = (alpha * blend[0] + ialpha * pix[2]) >> 8;
		  break;
		case splashModeARGB8:
		case splashModeBGRA8:
#if SPLASH_CMYK
		case splashModeCMYK8:
#endif
		  pix = &bitmap->data[y1 * bitmap->rowSize + 4 * x1];
		  (*blendFunc)(fg, pix, blend, bitmap->mode);
		  pix[0] = (alpha * blend[0] + ialpha * pix[0]) >> 8;
		  pix[1] = (alpha * blend[1] + ialpha * pix[1]) >> 8;
		  pix[2] = (alpha * blend[2] + ialpha * pix[2]) >> 8;
		  pix[3] = (alpha * blend[3] + ialpha * pix[3]) >> 8;
		  break;
#if SPLASH_CMYK
		case splashModeACMYK8:
		  pix = &bitmap->data[y1 * bitmap->rowSize + 5 * x1];
		  (*blendFunc)(fg, pix, blend, bitmap->mode);
		  pix[0] = (alpha * blend[0] + ialpha * pix[0]) >> 8;
		  pix[1] = (alpha * blend[1] + ialpha * pix[1]) >> 8;
		  pix[2] = (alpha * blend[2] + ialpha * pix[2]) >> 8;
		  pix[3] = (alpha * blend[3] + ialpha * pix[3]) >> 8;
		  pix[4] = (alpha * blend[4] + ialpha * pix[4]) >> 8;
		  break;
#endif
		}
		if (!noClip) {
		  updateModX(x1);
		  updateModY(y1);
		}
	      }
	    }
	  }
	}

      } else {
	p = glyph->data;
	for (yy = 0, y1 = y0 - glyph->y; yy < glyph->h; ++yy, ++y1) {
	  for (xx = 0, x1 = x0 - glyph->x; xx < glyph->w; xx += 8) {
	    alpha0 = *p++;
	    for (xx1 = 0; xx1 < 8 && xx + xx1 < glyph->w; ++xx1, ++x1) {
	      if (alpha0 & 0x80) {
		if (noClip || state->clip->test(x1, y1)) {
		  if (softMask) {
		    alpha = (int)(state->fillAlpha *
				  softMask->data[y1 * softMask->rowSize + x1]);
		  } else {
		    alpha = (int)(state->fillAlpha * 255);
		  }
		  ialpha = 255 - alpha;
		  state->fillPattern->getColor(x1, y1, fg);
		  switch (bitmap->mode) {
		  case splashModeMono1:
		    pix = &bitmap->data[y1 * bitmap->rowSize + (x1 >> 3)];
		    dest[0] = (*pix >> (7 - (x1 & 7))) & 1;
		    (*blendFunc)(fg, dest, blend, bitmap->mode);
		    t = (alpha * blend[0] + ialpha * dest[0]) >> 8;
		    if (t) {
		      *pix |= 0x80 >> (x1 & 7);
		    } else {
		      *pix &= ~(0x80 >> (x1 & 7));
		    }
		    break;
		  case splashModeMono8:
		    pix = &bitmap->data[y1 * bitmap->rowSize + x1];
		    (*blendFunc)(fg, pix, blend, bitmap->mode);
		    // note: floor(x / 255) = x >> 8 (for 16-bit x)
		    pix[0] = (alpha * blend[0] + ialpha * pix[0]) >> 8;
		    break;
		  case splashModeAMono8:
		    pix = &bitmap->data[y1 * bitmap->rowSize + 2 * x1];
		    (*blendFunc)(fg, pix, blend, bitmap->mode);
		    pix[0] = (alpha * blend[0] + ialpha * pix[0]) >> 8;
		    pix[1] = (alpha * blend[1] + ialpha * pix[1]) >> 8;
		    break;
		  case splashModeRGB8:
		  case splashModeBGR8:
		    pix = &bitmap->data[y1 * bitmap->rowSize + 3 * x1];
		    (*blendFunc)(fg, pix, blend, bitmap->mode);
		    pix[0] = (alpha * blend[0] + ialpha * pix[0]) >> 8;
		    pix[1] = (alpha * blend[1] + ialpha * pix[1]) >> 8;
		    pix[2] = (alpha * blend[2] + ialpha * pix[2]) >> 8;
		    break;
		  case splashModeRGB8Qt:
		    pix = &bitmap->data[y1 * bitmap->rowSize + 4 * x1];
		    (*blendFunc)(fg, pix, blend, bitmap->mode);
		    pix[0] = (alpha * blend[2] + ialpha * pix[0]) >> 8;
		    pix[1] = (alpha * blend[1] + ialpha * pix[1]) >> 8;
		    pix[2] = (alpha * blend[0] + ialpha * pix[2]) >> 8;
		    break;
		  case splashModeARGB8:
		  case splashModeBGRA8:
#if SPLASH_CMYK
		  case splashModeCMYK8:
#endif
		    pix = &bitmap->data[y1 * bitmap->rowSize + 4 * x1];
		    (*blendFunc)(fg, pix, blend, bitmap->mode);
		    pix[0] = (alpha * blend[0] + ialpha * pix[0]) >> 8;
		    pix[1] = (alpha * blend[1] + ialpha * pix[1]) >> 8;
		    pix[2] = (alpha * blend[2] + ialpha * pix[2]) >> 8;
		    pix[3] = (alpha * blend[3] + ialpha * pix[3]) >> 8;
		    break;
#if SPLASH_CMYK
		  case splashModeACMYK8:
		    pix = &bitmap->data[y1 * bitmap->rowSize + 5 * x1];
		    (*blendFunc)(fg, pix, blend, bitmap->mode);
		    pix[0] = (alpha * blend[0] + ialpha * pix[0]) >> 8;
		    pix[1] = (alpha * blend[1] + ialpha * pix[1]) >> 8;
		    pix[2] = (alpha * blend[2] + ialpha * pix[2]) >> 8;
		    pix[3] = (alpha * blend[3] + ialpha * pix[3]) >> 8;
		    pix[4] = (alpha * blend[4] + ialpha * pix[4]) >> 8;
		    break;
#endif
		  }
		  if (!noClip) {
		    updateModX(x1);
		    updateModY(y1);
		  }
		}
	      }
	      alpha0 <<= 1;
	    }
	  }
	}
      }

    } else {
      if (glyph->aa) {
	p = glyph->data;
	for (yy = 0, y1 = y0 - glyph->y; yy < glyph->h; ++yy, ++y1) {
	  for (xx = 0, x1 = x0 - glyph->x; xx < glyph->w; ++xx, ++x1) {
	    alpha = *p++;
	    if (alpha > 0) {
	      if (noClip || state->clip->test(x1, y1)) {
		ialpha = 255 - alpha;
		state->fillPattern->getColor(x1, y1, fg);
		switch (bitmap->mode) {
		case splashModeMono1:
		  if (alpha >= 0x80) {
		    pix = &bitmap->data[y1 * bitmap->rowSize + (x1 >> 3)];
		    if (fg[0]) {
		      *pix |= 0x80 >> (x1 & 7);
		    } else {
		      *pix &= ~(0x80 >> (x1 & 7));
		    }
		  }
		  break;
		case splashModeMono8:
		  pix = &bitmap->data[y1 * bitmap->rowSize + x1];
		  // note: floor(x / 255) = x >> 8 (for 16-bit x)
		  pix[0] = (alpha * fg[0] + ialpha * pix[0]) >> 8;
		  break;
		case splashModeAMono8:
		  pix = &bitmap->data[y1 * bitmap->rowSize + 2 * x1];
		  pix[0] = (alpha * fg[0] + ialpha * pix[0]) >> 8;
		  pix[1] = (alpha * fg[1] + ialpha * pix[1]) >> 8;
		  break;
		case splashModeRGB8:
		case splashModeBGR8:
		  pix = &bitmap->data[y1 * bitmap->rowSize + 3 * x1];
		  pix[0] = (alpha * fg[0] + ialpha * pix[0]) >> 8;
		  pix[1] = (alpha * fg[1] + ialpha * pix[1]) >> 8;
		  pix[2] = (alpha * fg[2] + ialpha * pix[2]) >> 8;
		  break;
		case splashModeRGB8Qt:
		  pix = &bitmap->data[y1 * bitmap->rowSize + 4 * x1];
		  pix[0] = (alpha * fg[2] + ialpha * pix[0]) >> 8;
		  pix[1] = (alpha * fg[1] + ialpha * pix[1]) >> 8;
		  pix[2] = (alpha * fg[0] + ialpha * pix[2]) >> 8;
		  break;
		case splashModeARGB8:
		case splashModeBGRA8:
#if SPLASH_CMYK
		case splashModeCMYK8:
#endif
		  pix = &bitmap->data[y1 * bitmap->rowSize + 4 * x1];
		  pix[0] = (alpha * fg[0] + ialpha * pix[0]) >> 8;
		  pix[1] = (alpha * fg[1] + ialpha * pix[1]) >> 8;
		  pix[2] = (alpha * fg[2] + ialpha * pix[2]) >> 8;
		  pix[3] = (alpha * fg[3] + ialpha * pix[3]) >> 8;
		  break;
#if SPLASH_CMYK
		case splashModeACMYK8:
		  pix = &bitmap->data[y1 * bitmap->rowSize + 5 * x1];
		  pix[0] = (alpha * fg[0] + ialpha * pix[0]) >> 8;
		  pix[1] = (alpha * fg[1] + ialpha * pix[1]) >> 8;
		  pix[2] = (alpha * fg[2] + ialpha * pix[2]) >> 8;
		  pix[3] = (alpha * fg[3] + ialpha * pix[3]) >> 8;
		  pix[4] = (alpha * fg[4] + ialpha * pix[4]) >> 8;
		  break;
#endif
		}
		if (!noClip) {
		  updateModX(x1);
		  updateModY(y1);
		}
	      }
	    }
	  }
	}

      } else {
	p = glyph->data;
	for (yy = 0, y1 = y0 - glyph->y; yy < glyph->h; ++yy, ++y1) {
	  for (xx = 0, x1 = x0 - glyph->x; xx < glyph->w; xx += 8) {
	    alpha0 = *p++;
	    for (xx1 = 0; xx1 < 8 && xx + xx1 < glyph->w; ++xx1, ++x1) {
	      if (alpha0 & 0x80) {
		if (noClip || state->clip->test(x1, y1)) {
		  state->fillPattern->getColor(x1, y1, fg);
		  switch (bitmap->mode) {
		  case splashModeMono1:
		    pix = &bitmap->data[y1 * bitmap->rowSize + (x1 >> 3)];
		    if (fg[0]) {
		      *pix |= 0x80 >> (x1 & 7);
		    } else {
		      *pix &= ~(0x80 >> (x1 & 7));
		    }
		    break;
		  case splashModeMono8:
		    pix = &bitmap->data[y1 * bitmap->rowSize + x1];
		    pix[0] = fg[0];
		    break;
		  case splashModeAMono8:
		    pix = &bitmap->data[y1 * bitmap->rowSize + 2 * x1];
		    pix[0] = fg[0];
		    pix[1] = fg[1];
		    break;
		  case splashModeRGB8:
		  case splashModeBGR8:
		    pix = &bitmap->data[y1 * bitmap->rowSize + 3 * x1];
		    pix[0] = fg[0];
		    pix[1] = fg[1];
		    pix[2] = fg[2];
		    break;
		  case splashModeRGB8Qt:
		    pix = &bitmap->data[y1 * bitmap->rowSize + 4 * x1];
		    pix[0] = fg[2];
		    pix[1] = fg[1];
		    pix[2] = fg[0];
		    break;
		  case splashModeARGB8:
		  case splashModeBGRA8:
#if SPLASH_CMYK
		  case splashModeCMYK8:
#endif
		    pix = &bitmap->data[y1 * bitmap->rowSize + 4 * x1];
		    pix[0] = fg[0];
		    pix[1] = fg[1];
		    pix[2] = fg[2];
		    pix[3] = fg[3];
		    break;
#if SPLASH_CMYK
		  case splashModeACMYK8:
		    pix = &bitmap->data[y1 * bitmap->rowSize + 5 * x1];
		    pix[0] = fg[0];
		    pix[1] = fg[1];
		    pix[2] = fg[2];
		    pix[3] = fg[3];
		    pix[4] = fg[4];
		    break;
#endif
		  }
		  if (!noClip) {
		    updateModX(x1);
		    updateModY(y1);
		  }
		}
	      }
	      alpha0 <<= 1;
	    }
	  }
	}
      }
    }
  }
  opClipRes = clipRes;

  return splashOk;
}

SplashError Splash::fillImageMask(SplashImageMaskSource src, void *srcData,
				  int w, int h, SplashCoord *mat) {
  GBool rot;
  SplashCoord xScale, yScale, xShear, yShear, yShear1;
  int tx, tx2, ty, ty2, scaledWidth, scaledHeight, xSign, ySign;
  int ulx, uly, llx, lly, urx, ury, lrx, lry;
  int ulx1, uly1, llx1, lly1, urx1, ury1, lrx1, lry1;
  int xMin, xMax, yMin, yMax;
  SplashClipResult clipRes, clipRes2;
  int yp, yq, yt, yStep, lastYStep;
  int xp, xq, xt, xStep, xSrc;
  int k1, spanXMin, spanXMax, spanY;
  SplashColorPtr pixBuf, p;
  int pixAcc;
  SplashCoord alpha;
  int x, y, x1, x2, y2;
  SplashCoord y1;
  int n, m, i, j;

  if (debugMode) {
    printf("fillImageMask: w=%d h=%d mat=[%.2f %.2f %.2f %.2f %.2f %.2f]\n",
	   w, h, (double)mat[0], (double)mat[1], (double)mat[2],
	   (double)mat[3], (double)mat[4], (double)mat[5]);
  }
  if (w == 0 && h == 0) return splashErrZeroImage;

  // check for singular matrix
  if (splashAbs(mat[0] * mat[3] - mat[1] * mat[2]) < 0.000001) {
    return splashErrSingularMatrix;
  }

  // compute scale, shear, rotation, translation parameters
  rot = splashAbs(mat[1]) > splashAbs(mat[0]);
  if (rot) {
    xScale = -mat[1];
    yScale = mat[2] - (mat[0] * mat[3]) / mat[1];
    xShear = -mat[3] / yScale;
    yShear = -mat[0] / mat[1];
  } else {
    xScale = mat[0];
    yScale = mat[3] - (mat[1] * mat[2]) / mat[0];
    xShear = mat[2] / yScale;
    yShear = mat[1] / mat[0];
  }
  // the +/-0.01 in these computations is to avoid floating point
  // precision problems which can lead to gaps between image stripes
  // (it can cause image stripes to overlap, but that's a much less
  // visible problem)
  if (xScale >= 0) {
    tx = splashRound(mat[4] - 0.01);
    tx2 = splashRound(mat[4] + xScale + 0.01) - 1;
  } else {
    tx = splashRound(mat[4] + 0.01) - 1;
    tx2 = splashRound(mat[4] + xScale - 0.01);
  }
  scaledWidth = abs(tx2 - tx) + 1;
  if (scaledWidth == 0) {
    // technically, this should draw nothing, but it generally seems
    // better to draw a one-pixel-wide stripe rather than throwing it
    // away
    scaledWidth = 1;
  }
  if (yScale >= 0) {
    ty = splashRound(mat[5] - 0.01);
    ty2 = splashRound(mat[5] + yScale + 0.01) - 1;
  } else {
    ty = splashRound(mat[5] + 0.01) - 1;
    ty2 = splashRound(mat[5] + yScale - 0.01);
  }
  scaledHeight = abs(ty2 - ty) + 1;
  if (scaledHeight == 0) {
    // technically, this should draw nothing, but it generally seems
    // better to draw a one-pixel-wide stripe rather than throwing it
    // away
    scaledHeight = 1;
  }
  xSign = (xScale < 0) ? -1 : 1;
  ySign = (yScale < 0) ? -1 : 1;
  yShear1 = (SplashCoord)xSign * yShear;

  // clipping
  ulx1 = 0;
  uly1 = 0;
  urx1 = xSign * (scaledWidth - 1);
  ury1 = (int)(yShear * urx1);
  llx1 = splashRound(xShear * ySign * (scaledHeight - 1));
  lly1 = ySign * (scaledHeight - 1) + (int)(yShear * llx1);
  lrx1 = xSign * (scaledWidth - 1) +
           splashRound(xShear * ySign * (scaledHeight - 1));
  lry1 = ySign * (scaledHeight - 1) + (int)(yShear * lrx1);
  if (rot) {
    ulx = tx + uly1;    uly = ty - ulx1;
    urx = tx + ury1;    ury = ty - urx1;
    llx = tx + lly1;    lly = ty - llx1;
    lrx = tx + lry1;    lry = ty - lrx1;
  } else {
    ulx = tx + ulx1;    uly = ty + uly1;
    urx = tx + urx1;    ury = ty + ury1;
    llx = tx + llx1;    lly = ty + lly1;
    lrx = tx + lrx1;    lry = ty + lry1;
  }
  xMin = (ulx < urx) ? (ulx < llx) ? (ulx < lrx) ? ulx : lrx
                                   : (llx < lrx) ? llx : lrx
		     : (urx < llx) ? (urx < lrx) ? urx : lrx
                                   : (llx < lrx) ? llx : lrx;
  xMax = (ulx > urx) ? (ulx > llx) ? (ulx > lrx) ? ulx : lrx
                                   : (llx > lrx) ? llx : lrx
		     : (urx > llx) ? (urx > lrx) ? urx : lrx
                                   : (llx > lrx) ? llx : lrx;
  yMin = (uly < ury) ? (uly < lly) ? (uly < lry) ? uly : lry
                                   : (lly < lry) ? lly : lry
		     : (ury < lly) ? (ury < lry) ? ury : lry
                                   : (lly < lry) ? lly : lry;
  yMax = (uly > ury) ? (uly > lly) ? (uly > lry) ? uly : lry
                                   : (lly > lry) ? lly : lry
		     : (ury > lly) ? (ury > lry) ? ury : lry
                                   : (lly > lry) ? lly : lry;
  clipRes = state->clip->testRect(xMin, yMin, xMax, yMax);
  opClipRes = clipRes;

  // compute Bresenham parameters for x and y scaling
  yp = h / scaledHeight;
  yq = h % scaledHeight;
  xp = w / scaledWidth;
  xq = w % scaledWidth;

  // allocate pixel buffer
  pixBuf = (SplashColorPtr)gmalloc((yp + 1) * w);

  // init y scale Bresenham
  yt = 0;
  lastYStep = 1;

  for (y = 0; y < scaledHeight; ++y) {

    // y scale Bresenham
    yStep = yp;
    yt += yq;
    if (yt >= scaledHeight) {
      yt -= scaledHeight;
      ++yStep;
    }

    // read row(s) from image
    n = (yp > 0) ? yStep : lastYStep;
    if (n > 0) {
      p = pixBuf;
      for (i = 0; i < n; ++i) {
	(*src)(srcData, p);
	p += w;
      }
    }
    lastYStep = yStep;

    // loop-invariant constants
    k1 = splashRound(xShear * ySign * y);

    // clipping test
    if (clipRes != splashClipAllInside &&
	!rot &&
	(int)(yShear * k1) ==
	  (int)(yShear * (xSign * (scaledWidth - 1) + k1))) {
      if (xSign > 0) {
	spanXMin = tx + k1;
	spanXMax = spanXMin + (scaledWidth - 1);
      } else {
	spanXMax = tx + k1;
	spanXMin = spanXMax - (scaledWidth - 1);
      }
      spanY = ty + ySign * y + (int)(yShear * k1);
      clipRes2 = state->clip->testSpan(spanXMin, spanXMax, spanY);
      if (clipRes2 == splashClipAllOutside) {
	continue;
      }
    } else {
      clipRes2 = clipRes;
    }

    // init x scale Bresenham
    xt = 0;
    xSrc = 0;

    // x shear
    x1 = k1;

    // y shear
    y1 = (SplashCoord)ySign * y + yShear * x1;
    // this is a kludge: if yShear1 is negative, then (int)y1 would
    // change immediately after the first pixel, which is not what we
    // want
    if (yShear1 < 0) {
      y1 += 0.999;
    }

    // loop-invariant constants
    n = yStep > 0 ? yStep : 1;

    for (x = 0; x < scaledWidth; ++x) {

      // x scale Bresenham
      xStep = xp;
      xt += xq;
      if (xt >= scaledWidth) {
	xt -= scaledWidth;
	++xStep;
      }

      // rotation
      if (rot) {
	x2 = (int)y1;
	y2 = -x1;
      } else {
	x2 = x1;
	y2 = (int)y1;
      }

      // compute the alpha value for (x,y) after the x and y scaling
      // operations
      m = xStep > 0 ? xStep : 1;
      p = pixBuf + xSrc;
      pixAcc = 0;
      for (i = 0; i < n; ++i) {
	for (j = 0; j < m; ++j) {
	  pixAcc += *p++;
	}
	p += w - m;
      }

      // blend fill color with background
      if (pixAcc != 0) {
	if (pixAcc == n * m) {
	  drawPixel(tx + x2, ty + y2, state->fillPattern, state->fillAlpha,
		    clipRes2 == splashClipAllInside);
	} else {
	  alpha = (SplashCoord)pixAcc / (SplashCoord)(n * m);
	  drawPixel(tx + x2, ty + y2, state->fillPattern,
		    state->fillAlpha * alpha,
		    clipRes2 == splashClipAllInside);
	}
      }

      // x scale Bresenham
      xSrc += xStep;

      // x shear
      x1 += xSign;

      // y shear
      y1 += yShear1;
    }
  }

  // free memory
  gfree(pixBuf);

  return splashOk;
}

SplashError Splash::drawImage(SplashImageSource src, void *srcData,
			      SplashColorMode srcMode,
			      int w, int h, SplashCoord *mat) {
  GBool ok, rot, halftone, srcAlpha;
  SplashCoord xScale, yScale, xShear, yShear, yShear1;
  int tx, tx2, ty, ty2, scaledWidth, scaledHeight, xSign, ySign;
  int ulx, uly, llx, lly, urx, ury, lrx, lry;
  int ulx1, uly1, llx1, lly1, urx1, ury1, lrx1, lry1;
  int xMin, xMax, yMin, yMax;
  SplashClipResult clipRes, clipRes2;
  int yp, yq, yt, yStep, lastYStep;
  int xp, xq, xt, xStep, xSrc;
  int k1, spanXMin, spanXMax, spanY;
  SplashColorPtr pixBuf, p;
  SplashColor pix;
#if SPLASH_CMYK
  int pixAcc0, pixAcc1, pixAcc2, pixAcc3;
#else
  int pixAcc0, pixAcc1, pixAcc2;
#endif
  int alphaAcc;
  SplashCoord pixMul, alphaMul, alpha;
  int x, y, x1, x2, y2;
  SplashCoord y1;
  int nComps, n, m, i, j;

  if (debugMode) {
    printf("drawImage: srcMode=%d w=%d h=%d mat=[%.2f %.2f %.2f %.2f %.2f %.2f]\n",
	   srcMode, w, h, (double)mat[0], (double)mat[1], (double)mat[2],
	   (double)mat[3], (double)mat[4], (double)mat[5]);
  }

  // check color modes
  ok = gFalse; // make gcc happy
  nComps = 0; // make gcc happy
  halftone = gFalse;
  srcAlpha = gFalse;
  switch (bitmap->mode) {
  case splashModeMono1:
    ok = srcMode == splashModeMono1 || srcMode == splashModeMono8 ||
         srcMode == splashModeAMono8;
    halftone = srcMode == splashModeMono8 || srcMode == splashModeAMono8;
    srcAlpha = srcMode == splashModeAMono8;
    nComps = srcAlpha ? 2 : 1;
    break;
  case splashModeMono8:
    ok = srcMode == splashModeMono8 || srcMode == splashModeAMono8;
    srcAlpha = srcMode == splashModeAMono8;
    nComps = srcAlpha ? 2 : 1;
    break;
  case splashModeAMono8:
    //~ not implemented yet
    ok = gFalse;
    nComps = 2;
    break;
  case splashModeRGB8Qt:
  case splashModeRGB8:
    ok = srcMode == splashModeRGB8 || srcMode == splashModeARGB8;
    srcAlpha = srcMode == splashModeARGB8;
    nComps = srcAlpha ? 4 : 3;
    break;
  case splashModeBGR8:
    ok = srcMode == splashModeBGR8 || srcMode == splashModeBGRA8;
    srcAlpha = srcMode == splashModeBGRA8;
    nComps = srcAlpha ? 4 : 3;
    break;
#if SPLASH_CMYK
  case splashModeCMYK8:
    ok = srcMode == splashModeCMYK8 || srcMode == splashModeACMYK8;
    srcAlpha = srcMode == splashModeACMYK8;
    nComps = srcAlpha ? 5 : 4;
    break;
#endif
  case splashModeARGB8:
  case splashModeBGRA8:
#if SPLASH_CMYK
  case splashModeACMYK8:
#endif
    //~ not implemented yet
    ok = gFalse;
    nComps = 4;
    break;
  }
  if (!ok) {
    return splashErrModeMismatch;
  }

  // check for singular matrix
  if (splashAbs(mat[0] * mat[3] - mat[1] * mat[2]) < 0.000001) {
    return splashErrSingularMatrix;
  }

  // compute scale, shear, rotation, translation parameters
  rot = splashAbs(mat[1]) > splashAbs(mat[0]);
  if (rot) {
    xScale = -mat[1];
    yScale = mat[2] - (mat[0] * mat[3]) / mat[1];
    xShear = -mat[3] / yScale;
    yShear = -mat[0] / mat[1];
  } else {
    xScale = mat[0];
    yScale = mat[3] - (mat[1] * mat[2]) / mat[0];
    xShear = mat[2] / yScale;
    yShear = mat[1] / mat[0];
  }
  // the +/-0.01 in these computations is to avoid floating point
  // precision problems which can lead to gaps between image stripes
  // (it can cause image stripes to overlap, but that's a much less
  // visible problem)
  if (xScale >= 0) {
    tx = splashRound(mat[4] - 0.01);
    tx2 = splashRound(mat[4] + xScale + 0.01) - 1;
  } else {
    tx = splashRound(mat[4] + 0.01) - 1;
    tx2 = splashRound(mat[4] + xScale - 0.01);
  }
  scaledWidth = abs(tx2 - tx) + 1;
  if (scaledWidth == 0) {
    // technically, this should draw nothing, but it generally seems
    // better to draw a one-pixel-wide stripe rather than throwing it
    // away
    scaledWidth = 1;
  }
  if (yScale >= 0) {
    ty = splashRound(mat[5] - 0.01);
    ty2 = splashRound(mat[5] + yScale + 0.01) - 1;
  } else {
    ty = splashRound(mat[5] + 0.01) - 1;
    ty2 = splashRound(mat[5] + yScale - 0.01);
  }
  scaledHeight = abs(ty2 - ty) + 1;
  if (scaledHeight == 0) {
    // technically, this should draw nothing, but it generally seems
    // better to draw a one-pixel-wide stripe rather than throwing it
    // away
    scaledHeight = 1;
  }
  xSign = (xScale < 0) ? -1 : 1;
  ySign = (yScale < 0) ? -1 : 1;
  yShear1 = (SplashCoord)xSign * yShear;

  // clipping
  ulx1 = 0;
  uly1 = 0;
  urx1 = xSign * (scaledWidth - 1);
  ury1 = (int)(yShear * urx1);
  llx1 = splashRound(xShear * ySign * (scaledHeight - 1));
  lly1 = ySign * (scaledHeight - 1) + (int)(yShear * llx1);
  lrx1 = xSign * (scaledWidth - 1) +
           splashRound(xShear * ySign * (scaledHeight - 1));
  lry1 = ySign * (scaledHeight - 1) + (int)(yShear * lrx1);
  if (rot) {
    ulx = tx + uly1;    uly = ty - ulx1;
    urx = tx + ury1;    ury = ty - urx1;
    llx = tx + lly1;    lly = ty - llx1;
    lrx = tx + lry1;    lry = ty - lrx1;
  } else {
    ulx = tx + ulx1;    uly = ty + uly1;
    urx = tx + urx1;    ury = ty + ury1;
    llx = tx + llx1;    lly = ty + lly1;
    lrx = tx + lrx1;    lry = ty + lry1;
  }
  xMin = (ulx < urx) ? (ulx < llx) ? (ulx < lrx) ? ulx : lrx
                                   : (llx < lrx) ? llx : lrx
		     : (urx < llx) ? (urx < lrx) ? urx : lrx
                                   : (llx < lrx) ? llx : lrx;
  xMax = (ulx > urx) ? (ulx > llx) ? (ulx > lrx) ? ulx : lrx
                                   : (llx > lrx) ? llx : lrx
		     : (urx > llx) ? (urx > lrx) ? urx : lrx
                                   : (llx > lrx) ? llx : lrx;
  yMin = (uly < ury) ? (uly < lly) ? (uly < lry) ? uly : lry
                                   : (lly < lry) ? lly : lry
		     : (ury < lly) ? (ury < lry) ? ury : lry
                                   : (lly < lry) ? lly : lry;
  yMax = (uly > ury) ? (uly > lly) ? (uly > lry) ? uly : lry
                                   : (lly > lry) ? lly : lry
		     : (ury > lly) ? (ury > lry) ? ury : lry
                                   : (lly > lry) ? lly : lry;
  clipRes = state->clip->testRect(xMin, yMin, xMax, yMax);
  opClipRes = clipRes;
  if (clipRes == splashClipAllOutside) {
    return splashOk;
  }

  // compute Bresenham parameters for x and y scaling
  yp = h / scaledHeight;
  yq = h % scaledHeight;
  xp = w / scaledWidth;
  xq = w % scaledWidth;

  // allocate pixel buffer
  pixBuf = (SplashColorPtr)gmalloc((yp + 1) * w * nComps);

  pixAcc0 = pixAcc1 = pixAcc2 = 0; // make gcc happy
#if SPLASH_CMYK
  pixAcc3 = 0; // make gcc happy
#endif

  if (srcAlpha) {

    // init y scale Bresenham
    yt = 0;
    lastYStep = 1;

    for (y = 0; y < scaledHeight; ++y) {

      // y scale Bresenham
      yStep = yp;
      yt += yq;
      if (yt >= scaledHeight) {
	yt -= scaledHeight;
	++yStep;
      }

      // read row(s) from image
      n = (yp > 0) ? yStep : lastYStep;
      if (n > 0) {
	p = pixBuf;
	for (i = 0; i < n; ++i) {
	  (*src)(srcData, p);
	  p += w * nComps;
	}
      }
      lastYStep = yStep;

      // loop-invariant constants
      k1 = splashRound(xShear * ySign * y);

      // clipping test
      if (clipRes != splashClipAllInside &&
	  !rot &&
	  (int)(yShear * k1) ==
	    (int)(yShear * (xSign * (scaledWidth - 1) + k1))) {
	if (xSign > 0) {
	  spanXMin = tx + k1;
	  spanXMax = spanXMin + (scaledWidth - 1);
	} else {
	  spanXMax = tx + k1;
	  spanXMin = spanXMax - (scaledWidth - 1);
	}
	spanY = ty + ySign * y + (int)(yShear * k1);
	clipRes2 = state->clip->testSpan(spanXMin, spanXMax, spanY);
	if (clipRes2 == splashClipAllOutside) {
	  continue;
	}
      } else {
	clipRes2 = clipRes;
      }

      // init x scale Bresenham
      xt = 0;
      xSrc = 0;

      // x shear
      x1 = k1;

      // y shear
      y1 = (SplashCoord)ySign * y + yShear * x1;
      // this is a kludge: if yShear1 is negative, then (int)y1 would
      // change immediately after the first pixel, which is not what
      // we want
      if (yShear1 < 0) {
	y1 += 0.999;
      }

      // loop-invariant constants
      n = yStep > 0 ? yStep : 1;

      for (x = 0; x < scaledWidth; ++x) {

	// x scale Bresenham
	xStep = xp;
	xt += xq;
	if (xt >= scaledWidth) {
	  xt -= scaledWidth;
	  ++xStep;
	}

	// rotation
	if (rot) {
	  x2 = (int)y1;
	  y2 = -x1;
	} else {
	  x2 = x1;
	  y2 = (int)y1;
	}

	// compute the filtered pixel at (x,y) after the x and y scaling
	// operations
	m = xStep > 0 ? xStep : 1;
	alphaAcc = 0;
	switch (srcMode) {
	case splashModeAMono8:
	  p = pixBuf + xSrc * 2;
	  pixAcc0 = 0;
	  for (i = 0; i < n; ++i) {
	    for (j = 0; j < m; ++j) {
	      alphaAcc += *p++;
	      pixAcc0 += *p++;
	    }
	    p += 2 * (w - m);
	  }
	  break;
	case splashModeARGB8:
	  p = pixBuf + xSrc * 4;
	  pixAcc0 = pixAcc1 = pixAcc2 = 0;
	  for (i = 0; i < n; ++i) {
	    for (j = 0; j < m; ++j) {
	      alphaAcc += *p++;
	      pixAcc0 += *p++;
	      pixAcc1 += *p++;
	      pixAcc2 += *p++;
	    }
	    p += 4 * (w - m);
	  }
	  break;
	case splashModeBGRA8:
	  p = pixBuf + xSrc * 4;
	  pixAcc0 = pixAcc1 = pixAcc2 = 0;
	  for (i = 0; i < n; ++i) {
	    for (j = 0; j < m; ++j) {
	      pixAcc0 += *p++;
	      pixAcc1 += *p++;
	      pixAcc2 += *p++;
	      alphaAcc += *p++;
	    }
	    p += 4 * (w - m);
	  }
	  break;
#if SPLASH_CMYK
	case splashModeACMYK8:
	  p = pixBuf + xSrc * 5;
	  pixAcc0 = pixAcc1 = pixAcc2 = pixAcc3 = 0;
	  for (i = 0; i < n; ++i) {
	    for (j = 0; j < m; ++j) {
	      alphaAcc += *p++;
	      pixAcc0 += *p++;
	      pixAcc1 += *p++;
	      pixAcc2 += *p++;
	      pixAcc3 += *p++;
	    }
	    p += 5 * (w - m);
	  }
	  break;
#endif
	default: // make gcc happy
	  break;
	}
	pixMul = (SplashCoord)1 / (SplashCoord)(n * m);
	alphaMul = pixMul * (1.0 / 256.0);
	alpha = (SplashCoord)alphaAcc * alphaMul;

	if (alpha > 0) {
	  // mono8 -> mono1 conversion, with halftoning
	  if (halftone) {
	    pix[0] = state->screen->test(tx + x2, ty + y2,
			    (SplashCoord)pixAcc0 * pixMul * (1.0 / 256.0));

	  // no conversion, no halftoning
	  } else {
	    switch (bitmap->mode) {
#if SPLASH_CMYK
	    case splashModeCMYK8:
	      pix[3] = (int)((SplashCoord)pixAcc3 * pixMul);
	      // fall through
#endif
	    case splashModeRGB8:
	    case splashModeBGR8:
	    case splashModeRGB8Qt:
	      pix[2] = (int)((SplashCoord)pixAcc2 * pixMul);
	      pix[1] = (int)((SplashCoord)pixAcc1 * pixMul);
	      // fall through
	    case splashModeMono1:
	    case splashModeMono8:
	      pix[0] = (int)((SplashCoord)pixAcc0 * pixMul);
	      break;
	    default: // make gcc happy
	      break;
	    }
	  }

	  // set pixel
	  drawPixel(tx + x2, ty + y2, pix, alpha * state->fillAlpha,
		    clipRes2 == splashClipAllInside);
	}

	// x scale Bresenham
	xSrc += xStep;

	// x shear
	x1 += xSign;

	// y shear
	y1 += yShear1;
      }
    }

  } else {

    // init y scale Bresenham
    yt = 0;
    lastYStep = 1;

    for (y = 0; y < scaledHeight; ++y) {

      // y scale Bresenham
      yStep = yp;
      yt += yq;
      if (yt >= scaledHeight) {
	yt -= scaledHeight;
	++yStep;
      }

      // read row(s) from image
      n = (yp > 0) ? yStep : lastYStep;
      if (n > 0) {
	p = pixBuf;
	for (i = 0; i < n; ++i) {
	  (*src)(srcData, p);
	  p += w * nComps;
	}
      }
      lastYStep = yStep;

      // loop-invariant constants
      k1 = splashRound(xShear * ySign * y);

      // clipping test
      if (clipRes != splashClipAllInside &&
	  !rot &&
	  (int)(yShear * k1) ==
	    (int)(yShear * (xSign * (scaledWidth - 1) + k1))) {
	if (xSign > 0) {
	  spanXMin = tx + k1;
	  spanXMax = spanXMin + (scaledWidth - 1);
	} else {
	  spanXMax = tx + k1;
	  spanXMin = spanXMax - (scaledWidth - 1);
	}
	spanY = ty + ySign * y + (int)(yShear * k1);
	clipRes2 = state->clip->testSpan(spanXMin, spanXMax, spanY);
	if (clipRes2 == splashClipAllOutside) {
	  continue;
	}
      } else {
	clipRes2 = clipRes;
      }

      // init x scale Bresenham
      xt = 0;
      xSrc = 0;

      // x shear
      x1 = k1;

      // y shear
      y1 = (SplashCoord)ySign * y + yShear * x1;
      // this is a kludge: if yShear1 is negative, then (int)y1 would
      // change immediately after the first pixel, which is not what
      // we want
      if (yShear1 < 0) {
	y1 += 0.999;
      }

      // loop-invariant constants
      n = yStep > 0 ? yStep : 1;

      for (x = 0; x < scaledWidth; ++x) {

	// x scale Bresenham
	xStep = xp;
	xt += xq;
	if (xt >= scaledWidth) {
	  xt -= scaledWidth;
	  ++xStep;
	}

	// rotation
	if (rot) {
	  x2 = (int)y1;
	  y2 = -x1;
	} else {
	  x2 = x1;
	  y2 = (int)y1;
	}

	// compute the filtered pixel at (x,y) after the x and y scaling
	// operations
	m = xStep > 0 ? xStep : 1;
	switch (srcMode) {
	case splashModeMono1:
	case splashModeMono8:
	  p = pixBuf + xSrc;
	  pixAcc0 = 0;
	  for (i = 0; i < n; ++i) {
	    for (j = 0; j < m; ++j) {
	      pixAcc0 += *p++;
	    }
	    p += w - m;
	  }
	  break;
	case splashModeRGB8:
	case splashModeBGR8:
	case splashModeRGB8Qt:
	  p = pixBuf + xSrc * 3;
	  pixAcc0 = pixAcc1 = pixAcc2 = 0;
	  for (i = 0; i < n; ++i) {
	    for (j = 0; j < m; ++j) {
	      pixAcc0 += *p++;
	      pixAcc1 += *p++;
	      pixAcc2 += *p++;
	    }
	    p += 3 * (w - m);
	  }
	  break;
#if SPLASH_CMYK
	case splashModeCMYK8:
	  p = pixBuf + xSrc * 4;
	  pixAcc0 = pixAcc1 = pixAcc2 = pixAcc3 = 0;
	  for (i = 0; i < n; ++i) {
	    for (j = 0; j < m; ++j) {
	      pixAcc0 += *p++;
	      pixAcc1 += *p++;
	      pixAcc2 += *p++;
	      pixAcc3 += *p++;
	    }
	    p += 4 * (w - m);
	  }
	  break;
#endif
	default: // make gcc happy
	  break;
	}
	pixMul = (SplashCoord)1 / (SplashCoord)(n * m);

	// mono8 -> mono1 conversion, with halftoning
	if (halftone) {
	  pix[0] = state->screen->test(tx + x2, ty + y2,
			  (SplashCoord)pixAcc0 * pixMul * (1.0 / 256.0));

	// no conversion, no halftoning
	} else {
	  switch (bitmap->mode) {
#if SPLASH_CMYK
	  case splashModeCMYK8:
	    pix[3] = (int)((SplashCoord)pixAcc3 * pixMul);
	    // fall through
#endif
	  case splashModeRGB8:
	  case splashModeRGB8Qt:
	  case splashModeBGR8:
	    pix[2] = (int)((SplashCoord)pixAcc2 * pixMul);
	    pix[1] = (int)((SplashCoord)pixAcc1 * pixMul);
	    // fall through
	  case splashModeMono1:
	  case splashModeMono8:
	    pix[0] = (int)((SplashCoord)pixAcc0 * pixMul);
	    break;
	  default: // make gcc happy
	    break;
	  }
	}

	// set pixel
	drawPixel(tx + x2, ty + y2, pix, state->fillAlpha,
		  clipRes2 == splashClipAllInside);

	// x scale Bresenham
	xSrc += xStep;

	// x shear
	x1 += xSign;

	// y shear
	y1 += yShear1;
      }
    }

  }

  gfree(pixBuf);

  return splashOk;
}

void Splash::dumpPath(SplashPath *path) {
  int i;

  for (i = 0; i < path->length; ++i) {
    printf("  %3d: x=%8.2f y=%8.2f%s%s%s%s%s\n",
	   i, (double)path->pts[i].x, (double)path->pts[i].y,
	   (path->flags[i] & splashPathFirst) ? " first" : "",
	   (path->flags[i] & splashPathLast) ? " last" : "",
	   (path->flags[i] & splashPathClosed) ? " closed" : "",
	   (path->flags[i] & splashPathCurve) ? " curve" : "",
	   (path->flags[i] & splashPathArcCW) ? " arcCW" : "");
  }
}

void Splash::dumpXPath(SplashXPath *path) {
  int i;

  for (i = 0; i < path->length; ++i) {
    printf("  %4d: x0=%8.2f y0=%8.2f x1=%8.2f y1=%8.2f %s%s%s%s%s%s%s\n",
	   i, (double)path->segs[i].x0, (double)path->segs[i].y0,
	   (double)path->segs[i].x1, (double)path->segs[i].y1,
	   (path->segs[i].flags	& splashXPathFirst) ? "F" : " ",
	   (path->segs[i].flags	& splashXPathLast) ? "L" : " ",
	   (path->segs[i].flags	& splashXPathEnd0) ? "0" : " ",
	   (path->segs[i].flags	& splashXPathEnd1) ? "1" : " ",
	   (path->segs[i].flags	& splashXPathHoriz) ? "H" : " ",
	   (path->segs[i].flags	& splashXPathVert) ? "V" : " ",
	   (path->segs[i].flags	& splashXPathFlip) ? "P" : " ");
  }
}
