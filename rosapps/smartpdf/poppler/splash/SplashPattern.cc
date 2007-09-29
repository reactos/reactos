//========================================================================
//
// SplashPattern.cc
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include "SplashMath.h"
#include "SplashScreen.h"
#include "SplashPattern.h"

//------------------------------------------------------------------------
// SplashPattern
//------------------------------------------------------------------------

SplashPattern::SplashPattern() {
}

SplashPattern::~SplashPattern() {
}

//------------------------------------------------------------------------
// SplashSolidColor
//------------------------------------------------------------------------

SplashSolidColor::SplashSolidColor(SplashColorPtr colorA) {
  splashColorCopy(color, colorA);
}

SplashSolidColor::~SplashSolidColor() {
}

void SplashSolidColor::getColor(int x, int y, SplashColorPtr c) {
  splashColorCopy(c, color);
}

//------------------------------------------------------------------------
// SplashHalftone
//------------------------------------------------------------------------

SplashHalftone::SplashHalftone(SplashColorPtr color0A, SplashColorPtr color1A,
			       SplashScreen *screenA, SplashCoord valueA) {
  splashColorCopy(color0, color0A);
  splashColorCopy(color1, color1A);
  screen = screenA;
  value = valueA;
}

SplashPattern *SplashHalftone::copy() {
  return new SplashHalftone(color0, color1, screen->copy(), value);
}

SplashHalftone::~SplashHalftone() {
  delete screen;
}

void SplashHalftone::getColor(int x, int y, SplashColorPtr c) {
  splashColorCopy(c, screen->test(x, y, value) ? color1 : color0);
}

GBool SplashHalftone::isStatic() {
  return screen->isStatic(value);
}
