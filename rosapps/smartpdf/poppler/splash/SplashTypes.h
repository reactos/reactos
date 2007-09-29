//========================================================================
//
// SplashTypes.h
//
//========================================================================

#ifndef SPLASHTYPES_H
#define SPLASHTYPES_H

#include "goo/gtypes.h"

//------------------------------------------------------------------------
// coordinates
//------------------------------------------------------------------------

#if USE_FIXEDPOINT
#include "goo/FixedPoint.h"
typedef FixedPoint SplashCoord;
#else
typedef double SplashCoord;
#endif

//------------------------------------------------------------------------
// colors
//------------------------------------------------------------------------

enum SplashColorMode {
  splashModeMono1,		// 1 bit per component, 8 pixels per byte,
				//   MSbit is on the left
  splashModeMono8,		// 1 byte per component, 1 byte per pixel
  splashModeAMono8,		// 1 byte per component, 2 bytes per pixel:
				//   AMAM...
  splashModeRGB8,		// 1 byte per component, 3 bytes per pixel:
				//   RGBRGB...
  splashModeBGR8,		// 1 byte per component, 3 bytes per pixel:
				//   BGRBGR...
  splashModeARGB8,		// 1 byte per component, 4 bytes per pixel:
				//   ARGBARGB...
  splashModeRGB8Qt,		// 1 byte per component, 4 bytes per pixel:
				//   Specially hacked to use in Qt frontends
  splashModeBGRA8		// 1 byte per component, 4 bytes per pixel:
				//   BGRABGRA...
#if SPLASH_CMYK
  ,
  splashModeCMYK8,		// 1 byte per component, 4 bytes per pixel:
				//   CMYKCMYK...
  splashModeACMYK8		// 1 byte per component, 5 bytes per pixel:
				//   ACMYKACMYK
#endif
};

// number of components in each color mode
// (defined in SplashState.cc)
extern int splashColorModeNComps[];

// max number of components in any SplashColor
#if SPLASH_CMYK
#  define splashMaxColorComps 5
#else
#  define splashMaxColorComps 4
#endif

typedef Guchar SplashColor[splashMaxColorComps];
typedef Guchar *SplashColorPtr;

// AMono8
static inline Guchar splashAMono8A(SplashColorPtr am8) { return am8[0]; }
static inline Guchar splashAMono8M(SplashColorPtr am8) { return am8[1]; }

// RGB8
static inline Guchar splashRGB8R(SplashColorPtr rgb8) { return rgb8[0]; }
static inline Guchar splashRGB8G(SplashColorPtr rgb8) { return rgb8[1]; }
static inline Guchar splashRGB8B(SplashColorPtr rgb8) { return rgb8[2]; }

// BGR8
static inline Guchar splashBGR8R(SplashColorPtr bgr8) { return bgr8[2]; }
static inline Guchar splashBGR8G(SplashColorPtr bgr8) { return bgr8[1]; }
static inline Guchar splashBGR8B(SplashColorPtr bgr8) { return bgr8[0]; }

// ARGB8
static inline Guchar splashARGB8A(SplashColorPtr argb8) { return argb8[0]; }
static inline Guchar splashARGB8R(SplashColorPtr argb8) { return argb8[1]; }
static inline Guchar splashARGB8G(SplashColorPtr argb8) { return argb8[2]; }
static inline Guchar splashARGB8B(SplashColorPtr argb8) { return argb8[3]; }

// ARGB8
static inline Guchar splashBGRA8A(SplashColorPtr bgra8) { return bgra8[3]; }
static inline Guchar splashBGRA8R(SplashColorPtr bgra8) { return bgra8[2]; }
static inline Guchar splashBGRA8G(SplashColorPtr bgra8) { return bgra8[1]; }
static inline Guchar splashBGRA8B(SplashColorPtr bgra8) { return bgra8[0]; }

#if SPLASH_CMYK
// CMYK8
static inline Guchar splashCMYK8C(SplashColorPtr cmyk8) { return cmyk8[0]; }
static inline Guchar splashCMYK8M(SplashColorPtr cmyk8) { return cmyk8[1]; }
static inline Guchar splashCMYK8Y(SplashColorPtr cmyk8) { return cmyk8[2]; }
static inline Guchar splashCMYK8K(SplashColorPtr cmyk8) { return cmyk8[3]; }

// ACMYK8
static inline Guchar splashACMYK8A(SplashColorPtr acmyk8) { return acmyk8[0]; }
static inline Guchar splashACMYK8C(SplashColorPtr acmyk8) { return acmyk8[1]; }
static inline Guchar splashACMYK8M(SplashColorPtr acmyk8) { return acmyk8[2]; }
static inline Guchar splashACMYK8Y(SplashColorPtr acmyk8) { return acmyk8[3]; }
static inline Guchar splashACMYK8K(SplashColorPtr acmyk8) { return acmyk8[4]; }
#endif

static inline void splashColorCopy(SplashColorPtr dest, SplashColorPtr src) {
  dest[0] = src[0];
  dest[1] = src[1];
  dest[2] = src[2];
  dest[3] = src[3];
#if SPLASH_CMYK
  dest[4] = src[4];
#endif
}

static inline void splashColorXor(SplashColorPtr dest, SplashColorPtr src) {
  dest[0] ^= src[0];
  dest[1] ^= src[1];
  dest[2] ^= src[2];
  dest[3] ^= src[3];
#if SPLASH_CMYK
  dest[4] ^= src[4];
#endif
}

//------------------------------------------------------------------------
// blend functions
//------------------------------------------------------------------------

typedef void (*SplashBlendFunc)(SplashColorPtr src, SplashColorPtr dest,
				SplashColorPtr blend, SplashColorMode cm);

//------------------------------------------------------------------------
// error results
//------------------------------------------------------------------------

typedef int SplashError;

#endif
