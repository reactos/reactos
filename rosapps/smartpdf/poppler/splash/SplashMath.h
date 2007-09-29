//========================================================================
//
// SplashMath.h
//
//========================================================================

#ifndef SPLASHMATH_H
#define SPLASHMATH_H

#if USE_FIXEDPONT
#include "FixedPoint.h"
#else
#include <math.h>
#endif
#include "SplashTypes.h"

static inline SplashCoord splashAbs(SplashCoord x) {
#if USE_FIXEDPOINT
  return FixedPoint::abs(x);
#else
  return fabs(x);
#endif
}

static inline int splashFloor(SplashCoord x) {
  #if USE_FIXEDPOINT
    return FixedPoint::floor(x);
  #else
    return (int)floor(x);
  #endif
}

static inline int splashCeil(SplashCoord x) {
#if USE_FIXEDPOINT
  return FixedPoint::ceil(x);
#else
  return (int)ceil(x);
#endif
}

static inline int splashRound(SplashCoord x) {
#if USE_FIXEDPOINT
  return FixedPoint::round(x);
#else
  return (int)floor(x + 0.5);
#endif
}

static inline SplashCoord splashSqrt(SplashCoord x) {
#if USE_FIXEDPOINT
  return FixedPoint::sqrt(x);
#else
  return sqrt(x);
#endif
}

static inline SplashCoord splashPow(SplashCoord x, SplashCoord y) {
#if USE_FIXEDPOINT
  return FixedPoint::pow(x, y);
#else
  return pow(x, y);
#endif
}

static inline SplashCoord splashDist(SplashCoord x0, SplashCoord y0,
				     SplashCoord x1, SplashCoord y1) {
  SplashCoord dx, dy;
  dx = x1 - x0;
  dy = y1 - y0;
#if USE_FIXEDPOINT
  return FixedPoint::sqrt(dx * dx + dy * dy);
#else
  return sqrt(dx * dx + dy * dy);
#endif
}

#endif
