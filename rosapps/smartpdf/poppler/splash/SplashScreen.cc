//========================================================================
//
// SplashScreen.cc
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <string.h>
#include "goo/gmem.h"
#include "SplashMath.h"
#include "SplashScreen.h"

//------------------------------------------------------------------------
// SplashScreen
//------------------------------------------------------------------------

// This generates a 45 degree screen using a circular dot spot
// function.  DPI = resolution / ((size / 2) * sqrt(2)).
// Gamma correction (gamma = 1 / 1.33) is also computed here.
SplashScreen::SplashScreen(int sizeA) {
  SplashCoord *dist;
  SplashCoord u, v, d, val;
  int size2, x, y, x1, y1, i;

  size2 = sizeA >> 1;
  if (size2 < 1) {
    size2 = 1;
  }
  size = size2 << 1;

  // initialize the threshold matrix
  mat = (SplashCoord *)gmallocn(size * size, sizeof(SplashCoord));
  for (y = 0; y < size; ++y) {
    for (x = 0; x < size; ++x) {
      mat[y * size + x] = -1;
    }
  }

  // build the distance matrix
  dist = (SplashCoord *)gmallocn(size * size2, sizeof(SplashCoord));
  for (y = 0; y < size2; ++y) {
    for (x = 0; x < size2; ++x) {
      if (x + y < size2 - 1) {
	u = (SplashCoord)x + 0.5 - 0;
	v = (SplashCoord)y + 0.5 - 0;
      } else {
	u = (SplashCoord)x + 0.5 - (SplashCoord)size2;
	v = (SplashCoord)y + 0.5 - (SplashCoord)size2;
      }
      dist[y * size2 + x] = u*u + v*v;
    }
  }
  for (y = 0; y < size2; ++y) {
    for (x = 0; x < size2; ++x) {
      if (x < y) {
	u = (SplashCoord)x + 0.5 - 0;
	v = (SplashCoord)y + 0.5 - (SplashCoord)size2;
      } else {
	u = (SplashCoord)x + 0.5 - (SplashCoord)size2;
	v = (SplashCoord)y + 0.5 - 0;
      }
      dist[(size2 + y) * size2 + x] = u*u + v*v;
    }
  }

  // build the threshold matrix
  minVal = 1;
  maxVal = 0;
  x1 = y1 = 0; // make gcc happy
  for (i = 1; i <= size * size2; ++i) {
    d = size * size2;
    for (y = 0; y < size; ++y) {
      for (x = 0; x < size2; ++x) {
	if (mat[y * size + x] < 0 &&
	    dist[y * size2 + x] < d) {
	  x1 = x;
	  y1 = y;
	  d = dist[y1 * size2 + x1];
	}
      }
    }
    u = (SplashCoord)1 - (SplashCoord)i / (SplashCoord)(size * size2 + 1);
    val = splashPow(u, 1.33);
    if (val < minVal) {
      minVal = val;
    }
    if (val > maxVal) {
      maxVal = val;
    }
    mat[y1 * size + x1] = val;
    if (y1 < size2) {
      mat[(y1 + size2) * size + x1 + size2] = val;
    } else {
      mat[(y1 - size2) * size + x1 + size2] = val;
    }
  }

  gfree(dist);
}

SplashScreen::SplashScreen(SplashScreen *screen) {
  int n;

  size = screen->size;
  n = size * size * sizeof(SplashCoord);
  mat = (SplashCoord *)gmalloc(n);
  memcpy(mat, screen->mat, n);
  minVal = screen->minVal;
  maxVal = screen->maxVal;
}

SplashScreen::~SplashScreen() {
  gfree(mat);
}

int SplashScreen::test(int x, int y, SplashCoord value) {
  int xx, yy;

  if (value < minVal) {
    return 0;
  }
  if (value >= maxVal) {
    return 1;
  }
  if ((xx = x % size) < 0) {
    xx = -xx;
  }
  if ((yy = y % size) < 0) {
    yy = -yy;
  }
  return value < mat[yy * size + xx] ? 0 : 1;
}

GBool SplashScreen::isStatic(SplashCoord value) {
  return value < minVal || value >= maxVal;
}
