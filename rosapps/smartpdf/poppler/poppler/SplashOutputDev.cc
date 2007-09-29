//========================================================================
//
// SplashOutputDev.cc
//
// Copyright 2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <string.h>
#include <math.h>
#include "goo/gfile.h"
#include "GlobalParams.h"
#include "Error.h"
#include "Object.h"
#include "GfxFont.h"
#include "Link.h"
#include "CharCodeToUnicode.h"
#include "FontEncodingTables.h"
#include "fofi/FoFiTrueType.h"
#include "splash/SplashBitmap.h"
#include "splash/SplashGlyphBitmap.h"
#include "splash/SplashPattern.h"
#include "splash/SplashScreen.h"
#include "splash/SplashPath.h"
#include "splash/SplashState.h"
#include "splash/SplashErrorCodes.h"
#include "splash/SplashFontEngine.h"
#include "splash/SplashFont.h"
#include "splash/SplashFontFile.h"
#include "splash/SplashFontFileID.h"
#include "splash/Splash.h"
#include "SplashOutputDev.h"

//------------------------------------------------------------------------
// Blend functions
//------------------------------------------------------------------------

static void splashOutBlendMultiply(SplashColorPtr src, SplashColorPtr dest,
                                   SplashColorPtr blend, SplashColorMode cm) {
  int i;

  for (i = 0; i < splashColorModeNComps[cm]; ++i) {
    // note: floor(x / 255) = x >> 8 (for 16-bit x)
    blend[i] = (dest[i] * src[i]) >> 8;
  }
}

static void splashOutBlendScreen(SplashColorPtr src, SplashColorPtr dest,
                                 SplashColorPtr blend, SplashColorMode cm) {
  int i;

  for (i = 0; i < splashColorModeNComps[cm]; ++i) {
    // note: floor(x / 255) = x >> 8 (for 16-bit x)
    blend[i] = dest[i] + src[i] - ((dest[i] * src[i]) >> 8);
  }
}

static void splashOutBlendOverlay(SplashColorPtr src, SplashColorPtr dest,
                                  SplashColorPtr blend, SplashColorMode cm) {
  int i;

  //~ not sure if this is right
  for (i = 0; i < splashColorModeNComps[cm]; ++i) {
    // note: floor(x / 255) = x >> 8 (for 16-bit x)
    blend[i] = dest[i] < 0x80 ? ((dest[i] * src[i]) >> 8)
                              : dest[i] + src[i] - ((dest[i] * src[i]) >> 8);
  }
}

static void splashOutBlendDarken(SplashColorPtr src, SplashColorPtr dest,
                                 SplashColorPtr blend, SplashColorMode cm) {
  int i;

  for (i = 0; i < splashColorModeNComps[cm]; ++i) {
    blend[i] = dest[i] < src[i] ? dest[i] : src[i];
  }
}

static void splashOutBlendLighten(SplashColorPtr src, SplashColorPtr dest,
                                  SplashColorPtr blend, SplashColorMode cm) {
  int i;

  for (i = 0; i < splashColorModeNComps[cm]; ++i) {
    blend[i] = dest[i] > src[i] ? dest[i] : src[i];
  }
}

static void splashOutBlendColorDodge(SplashColorPtr src, SplashColorPtr dest,
                                     SplashColorPtr blend,
                                     SplashColorMode cm) {
  int i, x;

  for (i = 0; i < splashColorModeNComps[cm]; ++i) {
    x = dest[i] + src[i];
    blend[i] = x <= 255 ? x : 255;
  }
}

static void splashOutBlendColorBurn(SplashColorPtr src, SplashColorPtr dest,
                                    SplashColorPtr blend, SplashColorMode cm) {
  int i, x;

  for (i = 0; i < splashColorModeNComps[cm]; ++i) {
    x = dest[i] - (255 - src[i]);
    blend[i] = x >= 0 ? x : 0;
  }
}

static void splashOutBlendHardLight(SplashColorPtr src, SplashColorPtr dest,
                                    SplashColorPtr blend, SplashColorMode cm) {
  int i;

  //~ not sure if this is right
  for (i = 0; i < splashColorModeNComps[cm]; ++i) {
    // note: floor(x / 255) = x >> 8 (for 16-bit x)
    blend[i] = src[i] < 0x80
                 ? ((dest[i] * (src[i] * 2)) >> 8)
                 : 0xff - (((0xff - dest[i]) * (0x1ff - src[i] * 2)) >> 8);
  }
}

static void splashOutBlendSoftLight(SplashColorPtr src, SplashColorPtr dest,
                                    SplashColorPtr blend, SplashColorMode cm) {
  int i, x;

  //~ not sure if this is right
  for (i = 0; i < splashColorModeNComps[cm]; ++i) {
    if (src[i] < 0x80) {
      x = dest[i] - (0x80 - src[i]);
      blend[i] = x >= 0 ? x : 0;
    } else {
      x = dest[i] + (src[i] - 0x80);
      blend[i] = x <= 255 ? x : 255;
    }
  }
}

static void splashOutBlendDifference(SplashColorPtr src, SplashColorPtr dest,
                                     SplashColorPtr blend,
                                     SplashColorMode cm) {
  int i;

  for (i = 0; i < splashColorModeNComps[cm]; ++i) {
    blend[i] = dest[i] < src[i] ? src[i] - dest[i] : dest[i] - src[i];
  }
}

static void splashOutBlendExclusion(SplashColorPtr src, SplashColorPtr dest,
                                    SplashColorPtr blend, SplashColorMode cm) {
  int i;

  //~ not sure what this is supposed to do
  for (i = 0; i < splashColorModeNComps[cm]; ++i) {
    blend[i] = dest[i] < src[i] ? src[i] - dest[i] : dest[i] - src[i];
  }
}

static void cvtRGBToHSV(Guchar r, Guchar g, Guchar b, int *h, int *s, int *v) {
  int cmax, cmid, cmin, x;

  if (r >= g) {
    if (g >= b)      { x = 0; cmax = r; cmid = g; cmin = b; }
    else if (b >= r) { x = 4; cmax = b; cmid = r; cmin = g; }
    else             { x = 5; cmax = r; cmid = b; cmin = g; }
  } else {
    if (r >= b)      { x = 1; cmax = g; cmid = r; cmin = b; }
    else if (g >= b) { x = 2; cmax = g; cmid = b; cmin = r; }
    else             { x = 3; cmax = b; cmid = g; cmin = r; }
  }
  if (cmax == cmin) {
    *h = *s = 0;
  } else {
    *h = x * 60;
    if (x & 1) {
      *h += ((cmax - cmid) * 60) / (cmax - cmin);
    } else {
      *h += ((cmid - cmin) * 60) / (cmax - cmin);
    }
    *s = (255 * (cmax - cmin)) / cmax;
  }
  *v = cmax;
}

static void cvtHSVToRGB(int h, int s, int v, Guchar *r, Guchar *g, Guchar *b) {
  int x, f, cmax, cmid, cmin;

  if (s == 0) {
    *r = *g = *b = v;
  } else {
    x = h / 60;
    f = h % 60;
    cmax = v;
    if (x & 1) {
      cmid = (v * 255 - ((s * f) / 60)) >> 8;
    } else {
      cmid = (v * (255 - ((s * (60 - f)) / 60))) >> 8;
    }
    // note: floor(x / 255) = x >> 8 (for 16-bit x)
    cmin = (v * (255 - s)) >> 8;
    switch (x) {
    case 0: *r = cmax; *g = cmid; *b = cmin; break;
    case 1: *g = cmax; *r = cmid; *b = cmin; break;
    case 2: *g = cmax; *b = cmid; *r = cmin; break;
    case 3: *b = cmax; *g = cmid; *r = cmin; break;
    case 4: *b = cmax; *r = cmid; *g = cmin; break;
    case 5: *r = cmax; *b = cmid; *g = cmin; break;
    }
  }
}

static void splashOutBlendHue(SplashColorPtr src, SplashColorPtr dest,
                              SplashColorPtr blend, SplashColorMode cm) {
  int hs, ss, vs, hd, sd, vd;
#if SPLASH_CMYK
  Guchar r, g, b;
#endif

  switch (cm) {
  case splashModeMono1:
  case splashModeMono8:
    blend[0] = dest[0];
    break;
  case splashModeRGB8:
  case splashModeRGB8Qt:
    cvtRGBToHSV(src[0], src[1], src[2], &hs, &ss, &vs);
    cvtRGBToHSV(dest[0], dest[1], dest[2], &hd, &sd, &vd);
    cvtHSVToRGB(hs, sd, vd, &blend[0], &blend[1], &blend[2]);
    break;
  case splashModeBGR8:
    cvtRGBToHSV(src[2], src[1], src[0], &hs, &ss, &vs);
    cvtRGBToHSV(dest[2], dest[1], dest[0], &hd, &sd, &vd);
    cvtHSVToRGB(hs, sd, vd, &blend[2], &blend[1], &blend[0]);
    break;
#if SPLASH_CMYK
  case splashModeCMYK8:
    //~ (0xff - ...) should be clipped
    cvtRGBToHSV(0xff - (src[0] + src[3]),
                0xff - (src[1] + src[3]),
                0xff - (src[2] + src[3]), &hs, &ss, &vs);
    cvtRGBToHSV(0xff - (dest[0] + dest[3]),
                0xff - (dest[1] + dest[3]),
                0xff - (dest[2] + dest[3]), &hd, &sd, &vd);
    cvtHSVToRGB(hs, sd, vd, &r, &g, &b);
    //~ should do black generation
    blend[0] = 0xff - r;
    blend[0] = 0xff - g;
    blend[0] = 0xff - b;
    blend[3] = 0;
    break;
#endif
  default:
    //~ unimplemented
    break;
  }
}

static void splashOutBlendSaturation(SplashColorPtr src, SplashColorPtr dest,
                                     SplashColorPtr blend,
                                     SplashColorMode cm) {
  int hs, ss, vs, hd, sd, vd;
#if SPLASH_CMYK
  Guchar r, g, b;
#endif

  switch (cm) {
  case splashModeMono1:
  case splashModeMono8:
    blend[0] = dest[0];
    break;
  case splashModeRGB8:
  case splashModeRGB8Qt:
    cvtRGBToHSV(src[0], src[1], src[2], &hs, &ss, &vs);
    cvtRGBToHSV(dest[0], dest[1], dest[2], &hd, &sd, &vd);
    cvtHSVToRGB(hd, ss, vd, &blend[0], &blend[1], &blend[2]);
    break;
  case splashModeBGR8:
    cvtRGBToHSV(src[2], src[1], src[0], &hs, &ss, &vs);
    cvtRGBToHSV(dest[2], dest[1], dest[0], &hd, &sd, &vd);
    cvtHSVToRGB(hd, ss, vd, &blend[2], &blend[1], &blend[0]);
    break;
#if SPLASH_CMYK
  case splashModeCMYK8:
    //~ (0xff - ...) should be clipped
    cvtRGBToHSV(0xff - (src[0] + src[3]),
                0xff - (src[1] + src[3]),
                0xff - (src[2] + src[3]), &hs, &ss, &vs);
    cvtRGBToHSV(0xff - (dest[0] + dest[3]),
                0xff - (dest[1] + dest[3]),
                0xff - (dest[2] + dest[3]), &hd, &sd, &vd);
    cvtHSVToRGB(hd, ss, vd, &r, &g, &b);
    //~ should do black generation
    blend[0] = 0xff - r;
    blend[0] = 0xff - g;
    blend[0] = 0xff - b;
    blend[3] = 0;
    break;
#endif
  default:
    //~ unimplemented
    break;
  }
}

static void splashOutBlendColor(SplashColorPtr src, SplashColorPtr dest,
                                SplashColorPtr blend, SplashColorMode cm) {
  int hs, ss, vs, hd, sd, vd;
#if SPLASH_CMYK
  Guchar r, g, b;
#endif

  switch (cm) {
  case splashModeMono1:
  case splashModeMono8:
    blend[0] = dest[0];
    break;
  case splashModeRGB8:
  case splashModeRGB8Qt:
    cvtRGBToHSV(src[0], src[1], src[2], &hs, &ss, &vs);
    cvtRGBToHSV(dest[0], dest[1], dest[2], &hd, &sd, &vd);
    cvtHSVToRGB(hs, ss, vd, &blend[0], &blend[1], &blend[2]);
    break;
  case splashModeBGR8:
    cvtRGBToHSV(src[2], src[1], src[0], &hs, &ss, &vs);
    cvtRGBToHSV(dest[2], dest[1], dest[0], &hd, &sd, &vd);
    cvtHSVToRGB(hs, ss, vd, &blend[2], &blend[1], &blend[0]);
    break;
#if SPLASH_CMYK
  case splashModeCMYK8:
    //~ (0xff - ...) should be clipped
    cvtRGBToHSV(0xff - (src[0] + src[3]),
                0xff - (src[1] + src[3]),
                0xff - (src[2] + src[3]), &hs, &ss, &vs);
    cvtRGBToHSV(0xff - (dest[0] + dest[3]),
                0xff - (dest[1] + dest[3]),
                0xff - (dest[2] + dest[3]), &hd, &sd, &vd);
    cvtHSVToRGB(hs, ss, vd, &r, &g, &b);
    //~ should do black generation
    blend[0] = 0xff - r;
    blend[0] = 0xff - g;
    blend[0] = 0xff - b;
    blend[3] = 0;
    break;
#endif
  default:
    //~ unimplemented
    break;
  }
}

static void splashOutBlendLuminosity(SplashColorPtr src, SplashColorPtr dest,
                                     SplashColorPtr blend,
                                     SplashColorMode cm) {
  int hs, ss, vs, hd, sd, vd;
#if SPLASH_CMYK
  Guchar r, g, b;
#endif

  switch (cm) {
  case splashModeMono1:
  case splashModeMono8:
    blend[0] = dest[0];
    break;
  case splashModeRGB8:
  case splashModeRGB8Qt:
    cvtRGBToHSV(src[0], src[1], src[2], &hs, &ss, &vs);
    cvtRGBToHSV(dest[0], dest[1], dest[2], &hd, &sd, &vd);
    cvtHSVToRGB(hd, sd, vs, &blend[0], &blend[1], &blend[2]);
    break;
  case splashModeBGR8:
    cvtRGBToHSV(src[2], src[1], src[0], &hs, &ss, &vs);
    cvtRGBToHSV(dest[2], dest[1], dest[0], &hd, &sd, &vd);
    cvtHSVToRGB(hd, sd, vs, &blend[2], &blend[1], &blend[0]);
    break;
#if SPLASH_CMYK
  case splashModeCMYK8:
    //~ (0xff - ...) should be clipped
    cvtRGBToHSV(0xff - (src[0] + src[3]),
                0xff - (src[1] + src[3]),
                0xff - (src[2] + src[3]), &hs, &ss, &vs);
    cvtRGBToHSV(0xff - (dest[0] + dest[3]),
                0xff - (dest[1] + dest[3]),
                0xff - (dest[2] + dest[3]), &hd, &sd, &vd);
    cvtHSVToRGB(hd, sd, vs, &r, &g, &b);
    //~ should do black generation
    blend[0] = 0xff - r;
    blend[0] = 0xff - g;
    blend[0] = 0xff - b;
    blend[3] = 0;
    break;
#endif
  default:
    //~ unimplemented
    break;
  }
}

// NB: This must match the GfxBlendMode enum defined in GfxState.h.
SplashBlendFunc splashOutBlendFuncs[] = {
  NULL,
  &splashOutBlendMultiply,
  &splashOutBlendScreen,
  &splashOutBlendOverlay,
  &splashOutBlendDarken,
  &splashOutBlendLighten,
  &splashOutBlendColorDodge,
  &splashOutBlendColorBurn,
  &splashOutBlendHardLight,
  &splashOutBlendSoftLight,
  &splashOutBlendDifference,
  &splashOutBlendExclusion,
  &splashOutBlendHue,
  &splashOutBlendSaturation,
  &splashOutBlendColor,
  &splashOutBlendLuminosity
};

//------------------------------------------------------------------------
// SplashOutFontFileID
//------------------------------------------------------------------------

class SplashOutFontFileID: public SplashFontFileID {
public:

  SplashOutFontFileID(Ref *rA) { r = *rA; }

  ~SplashOutFontFileID() {}

  GBool matches(SplashFontFileID *id) {
    return ((SplashOutFontFileID *)id)->r.num == r.num &&
           ((SplashOutFontFileID *)id)->r.gen == r.gen;
  }

private:

  Ref r;
};

//------------------------------------------------------------------------
// T3FontCache
//------------------------------------------------------------------------

struct T3FontCacheTag {
  Gushort code;
  Gushort mru;			// valid bit (0x8000) and MRU index
};

class T3FontCache {
public:

  T3FontCache(Ref *fontID, double m11A, double m12A,
              double m21A, double m22A,
              int glyphXA, int glyphYA, int glyphWA, int glyphHA,
              GBool aa);
  ~T3FontCache();
  GBool matches(Ref *idA, double m11A, double m12A,
                double m21A, double m22A)
    { return fontID.num == idA->num && fontID.gen == idA->gen &&
             m11 == m11A && m12 == m12A && m21 == m21A && m22 == m22A; }

  Ref fontID;			// PDF font ID
  double m11, m12, m21, m22;	// transform matrix
  int glyphX, glyphY;		// pixel offset of glyph bitmaps
  int glyphW, glyphH;		// size of glyph bitmaps, in pixels
  int glyphSize;		// size of glyph bitmaps, in bytes
  int cacheSets;		// number of sets in cache
  int cacheAssoc;		// cache associativity (glyphs per set)
  Guchar *cacheData;		// glyph pixmap cache
  T3FontCacheTag *cacheTags;	// cache tags, i.e., char codes
};

T3FontCache::T3FontCache(Ref *fontIDA, double m11A, double m12A,
                         double m21A, double m22A,
                         int glyphXA, int glyphYA, int glyphWA, int glyphHA,
                         GBool aa) {
  int i;

  fontID = *fontIDA;
  m11 = m11A;
  m12 = m12A;
  m21 = m21A;
  m22 = m22A;
  glyphX = glyphXA;
  glyphY = glyphYA;
  glyphW = glyphWA;
  glyphH = glyphHA;
  if (aa) {
    glyphSize = glyphW * glyphH;
  } else {
    glyphSize = ((glyphW + 7) >> 3) * glyphH;
  }
  cacheAssoc = 8;
  if (glyphSize <= 256) {
    cacheSets = 8;
  } else if (glyphSize <= 512) {
    cacheSets = 4;
  } else if (glyphSize <= 1024) {
    cacheSets = 2;
  } else {
    cacheSets = 1;
  }
  cacheData = (Guchar *)gmallocn(cacheSets * cacheAssoc, glyphSize);
  cacheTags = (T3FontCacheTag *)gmallocn(cacheSets * cacheAssoc,
                                         sizeof(T3FontCacheTag));
  for (i = 0; i < cacheSets * cacheAssoc; ++i) {
    cacheTags[i].mru = i & (cacheAssoc - 1);
  }
}

T3FontCache::~T3FontCache() {
  gfree(cacheData);
  gfree(cacheTags);
}

struct T3GlyphStack {
  Gushort code;			// character code
  double x, y;			// position to draw the glyph

  //----- cache info
  T3FontCache *cache;		// font cache for the current font
  T3FontCacheTag *cacheTag;	// pointer to cache tag for the glyph
  Guchar *cacheData;		// pointer to cache data for the glyph

  //----- saved state
  SplashBitmap *origBitmap;
  Splash *origSplash;
  double origCTM4, origCTM5;

  T3GlyphStack *next;		// next object on stack
};

//------------------------------------------------------------------------
// SplashOutputDev
//------------------------------------------------------------------------

SplashOutputDev::SplashOutputDev(SplashColorMode colorModeA,
                                 int bitmapRowPadA,
                                 GBool reverseVideoA,
                                 SplashColorPtr paperColorA,
                                 GBool bitmapTopDownA,
                                 GBool allowAntialiasA) {
  colorMode = colorModeA;
  bitmapRowPad = bitmapRowPadA;
  bitmapTopDown = bitmapTopDownA;
  allowAntialias = allowAntialiasA;
  reverseVideo = reverseVideoA;
  splashColorCopy(paperColor, paperColorA);

  xref = NULL;

  bitmap = new SplashBitmap(1, 1, bitmapRowPad, colorMode, bitmapTopDown);
  splash = new Splash(bitmap);
  splash->clear(paperColor);

  fontEngine = NULL;

  nT3Fonts = 0;
  t3GlyphStack = NULL;

  font = NULL;
  needFontUpdate = gFalse;
  textClipPath = NULL;
}

SplashOutputDev::~SplashOutputDev() {
  int i;

  for (i = 0; i < nT3Fonts; ++i) {
    delete t3FontCache[i];
  }
  if (fontEngine) {
    delete fontEngine;
  }
  if (splash) {
    delete splash;
  }
  if (bitmap) {
    delete bitmap;
  }
}

void SplashOutputDev::startDoc(XRef *xrefA) {
  int i;

  xref = xrefA;
  if (fontEngine) {
    delete fontEngine;
  }
  fontEngine = new SplashFontEngine(
#if HAVE_T1LIB_H
                                    globalParams->getEnableT1lib(),
#endif
#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H
                                    globalParams->getEnableFreeType(),
#endif
                                    allowAntialias &&
                                      globalParams->getAntialias() &&
                                      colorMode != splashModeMono1);
  for (i = 0; i < nT3Fonts; ++i) {
    delete t3FontCache[i];
  }
  nT3Fonts = 0;
}

void SplashOutputDev::startPage(int pageNum, GfxState *state) {
  int w, h;
  SplashColor color;

  w = state ? (int)(state->getPageWidth() + 0.5) : 1;
  h = state ? (int)(state->getPageHeight() + 0.5) : 1;
  if (splash) {
    delete splash;
    splash = NULL;
  }
  if (!bitmap || w != bitmap->getWidth() || h != bitmap->getHeight()) {
    if (bitmap) {
      delete bitmap;
    }
    bitmap = new SplashBitmap(w, h, bitmapRowPad, colorMode, bitmapTopDown);
  }
  splash = new Splash(bitmap);
  switch (colorMode) {
  case splashModeMono1:
  case splashModeMono8:
    color[0] = 0;
    break;
  case splashModeRGB8:
  case splashModeBGR8:
  case splashModeRGB8Qt:
    color[0] = color[1] = color[2] = 0;
    break;
  case splashModeAMono8:
    color[0] = 0xff;
    color[1] = 0;
    break;
  case splashModeARGB8:
    color[0] = 255;
    color[1] = color[2] = color[3] = 0;
    break;
  case splashModeBGRA8:
    color[0] = color[1] = color[2] = 0;
    color[3] = 255;
    break;
#if SPLASH_CMYK
  case splashModeCMYK8:
    color[0] = color[1] = color[2] = color[3] = 0;
    break;
  case splashModeACMYK8:
    color[0] = 255;
    color[1] = color[2] = color[3] = color[4] = 0;
    break;
#endif
  }
  splash->setStrokePattern(new SplashSolidColor(color));
  splash->setFillPattern(new SplashSolidColor(color));
  splash->setLineCap(splashLineCapButt);
  splash->setLineJoin(splashLineJoinMiter);
  splash->setLineDash(NULL, 0, 0);
  splash->setMiterLimit(10);
  splash->setFlatness(1);
  splash->clear(paperColor);
}

void SplashOutputDev::endPage() {
}

void SplashOutputDev::drawLink(Link *link, Catalog *catalog) {
  double x1, y1, x2, y2;
  LinkBorderStyle *borderStyle;
  double r, g, b;
  GfxRGB rgb;
  GfxGray gray;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif
  double *dash;
  int dashLength;
  SplashCoord dashList[20];
  SplashPath *path;
  int x, y, i;

  link->getRect(&x1, &y1, &x2, &y2);
  borderStyle = link->getBorderStyle();
  if (borderStyle->getWidth() > 0) {
    borderStyle->getColor(&r, &g, &b);
    rgb.r = dblToCol(r);
    rgb.g = dblToCol(g);
    rgb.b = dblToCol(b);
    gray = dblToCol(0.299 * r + 0.587 * g + 0.114 * b);
    if (gray > gfxColorComp1) {
      gray = gfxColorComp1;
    }
#if SPLASH_CMYK
    cmyk.c = gfxColorComp1 - rgb.r;
    cmyk.m = gfxColorComp1 - rgb.g;
    cmyk.y = gfxColorComp1 - rgb.b;
    cmyk.k = 0;
    splash->setStrokePattern(getColor(gray, &rgb, &cmyk));
#else
    splash->setStrokePattern(getColor(gray, &rgb));
#endif
    splash->setLineWidth((SplashCoord)borderStyle->getWidth());
    borderStyle->getDash(&dash, &dashLength);
    if (borderStyle->getType() == linkBorderDashed && dashLength > 0) {
      if (dashLength > 20) {
        dashLength = 20;
      }
      for (i = 0; i < dashLength; ++i) {
        dashList[i] = (SplashCoord)dash[i];
      }
      splash->setLineDash(dashList, dashLength, 0);
    }
    path = new SplashPath();
    if (borderStyle->getType() == linkBorderUnderlined) {
      cvtUserToDev(x1, y1, &x, &y);
      path->moveTo((SplashCoord)x, (SplashCoord)y);
      cvtUserToDev(x2, y1, &x, &y);
      path->lineTo((SplashCoord)x, (SplashCoord)y);
    } else {
      cvtUserToDev(x1, y1, &x, &y);
      path->moveTo((SplashCoord)x, (SplashCoord)y);
      cvtUserToDev(x2, y1, &x, &y);
      path->lineTo((SplashCoord)x, (SplashCoord)y);
      cvtUserToDev(x2, y2, &x, &y);
      path->lineTo((SplashCoord)x, (SplashCoord)y);
      cvtUserToDev(x1, y2, &x, &y);
      path->lineTo((SplashCoord)x, (SplashCoord)y);
      path->close();
    }
    splash->stroke(path);
    delete path;
  }
}

void SplashOutputDev::saveState(GfxState *state) {
  splash->saveState();
}

void SplashOutputDev::restoreState(GfxState *state) {
  splash->restoreState();
  needFontUpdate = gTrue;
}

void SplashOutputDev::updateAll(GfxState *state) {
  updateLineDash(state);
  updateLineJoin(state);
  updateLineCap(state);
  updateLineWidth(state);
  updateFlatness(state);
  updateMiterLimit(state);
  updateFillColor(state);
  updateStrokeColor(state);
  needFontUpdate = gTrue;
}

void SplashOutputDev::updateCTM(GfxState *state, double m11, double m12,
                                double m21, double m22,
                                double m31, double m32) {
  updateLineDash(state);
  updateLineJoin(state);
  updateLineCap(state);
  updateLineWidth(state);
}

void SplashOutputDev::updateLineDash(GfxState *state) {
  double *dashPattern;
  int dashLength;
  double dashStart;
  SplashCoord dash[20];
  SplashCoord phase;
  int i;

  state->getLineDash(&dashPattern, &dashLength, &dashStart);
  if (dashLength > 20) {
    dashLength = 20;
  }
  for (i = 0; i < dashLength; ++i) {
    dash[i] =  (SplashCoord)state->transformWidth(dashPattern[i]);
    if (dash[i] < 1) {
      dash[i] = 1;
    }
  }
  phase = (SplashCoord)state->transformWidth(dashStart);
  splash->setLineDash(dash, dashLength, phase);
}

void SplashOutputDev::updateFlatness(GfxState *state) {
  splash->setFlatness(state->getFlatness());
}

void SplashOutputDev::updateLineJoin(GfxState *state) {
  splash->setLineJoin(state->getLineJoin());
}

void SplashOutputDev::updateLineCap(GfxState *state) {
  splash->setLineCap(state->getLineCap());
}

void SplashOutputDev::updateMiterLimit(GfxState *state) {
  splash->setMiterLimit(state->getMiterLimit());
}

void SplashOutputDev::updateLineWidth(GfxState *state) {
  splash->setLineWidth(state->getTransformedLineWidth());
}

void SplashOutputDev::updateFillColor(GfxState *state) {
  GfxGray gray;
  GfxRGB rgb;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif

  state->getFillGray(&gray);
  state->getFillRGB(&rgb);
#if SPLASH_CMYK
  state->getFillCMYK(&cmyk);
  splash->setFillPattern(getColor(gray, &rgb, &cmyk));
#else
  splash->setFillPattern(getColor(gray, &rgb));
#endif
}

void SplashOutputDev::updateStrokeColor(GfxState *state) {
  GfxGray gray;
  GfxRGB rgb;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif

  state->getStrokeGray(&gray);
  state->getStrokeRGB(&rgb);
#if SPLASH_CMYK
  state->getStrokeCMYK(&cmyk);
  splash->setStrokePattern(getColor(gray, &rgb, &cmyk));
#else
  splash->setStrokePattern(getColor(gray, &rgb));
#endif
}

#if SPLASH_CMYK
SplashPattern *SplashOutputDev::getColor(GfxGray gray, GfxRGB *rgb,
                                         GfxCMYK *cmyk) {
#else
SplashPattern *SplashOutputDev::getColor(GfxGray gray, GfxRGB *rgb) {
#endif
  SplashPattern *pattern;
  SplashColor color0, color1;
  GfxColorComp r, g, b;

  if (reverseVideo) {
    gray = gfxColorComp1 - gray;
    r = gfxColorComp1 - rgb->r;
    g = gfxColorComp1 - rgb->g;
    b = gfxColorComp1 - rgb->b;
  } else {
    r = rgb->r;
    g = rgb->g;
    b = rgb->b;
  }

  pattern = NULL; // make gcc happy
  switch (colorMode) {
  case splashModeMono1:
    color0[0] = 0;
    color1[0] = 1;
    pattern = new SplashHalftone(color0, color1,
                                 splash->getScreen()->copy(),
                                 (SplashCoord)colToDbl(gray));
    break;
  case splashModeMono8:
    color1[0] = colToByte(gray);
    pattern = new SplashSolidColor(color1);
    break;
  case splashModeAMono8:
    color1[0] = 255;
    color1[1] = colToByte(gray);
    pattern = new SplashSolidColor(color1);
    break;
  case splashModeRGB8:
  case splashModeRGB8Qt:
    color1[0] = colToByte(r);
    color1[1] = colToByte(g);
    color1[2] = colToByte(b);
    pattern = new SplashSolidColor(color1);
    break;
  case splashModeBGR8:
    color1[2] = colToByte(r);
    color1[1] = colToByte(g);
    color1[0] = colToByte(b);
    pattern = new SplashSolidColor(color1);
    break;
  case splashModeARGB8:
    color1[0] = 255;
    color1[1] = colToByte(r);
    color1[2] = colToByte(g);
    color1[3] = colToByte(b);
    pattern = new SplashSolidColor(color1);
    break;
  case splashModeBGRA8:
    color1[3] = 255;
    color1[2] = colToByte(r);
    color1[1] = colToByte(g);
    color1[0] = colToByte(b);
    pattern = new SplashSolidColor(color1);
    break;
#if SPLASH_CMYK
  case splashModeCMYK8:
    color1[0] = colToByte(cmyk->c);
    color1[1] = colToByte(cmyk->m);
    color1[2] = colToByte(cmyk->y);
    color1[3] = colToByte(cmyk->k);
    pattern = new SplashSolidColor(color1);
    break;
  case splashModeACMYK8:
    color1[0] = 255;
    color1[1] = colToByte(cmyk->c);
    color1[2] = colToByte(cmyk->m);
    color1[3] = colToByte(cmyk->y);
    color1[4] = colToByte(cmyk->k);
    pattern = new SplashSolidColor(color1);
    break;
#endif
  }

  return pattern;
}

void SplashOutputDev::updateBlendMode(GfxState *state) {
  splash->setBlendFunc(splashOutBlendFuncs[state->getBlendMode()]);
}

void SplashOutputDev::updateFillOpacity(GfxState *state) {
  splash->setFillAlpha((SplashCoord)state->getFillOpacity());
}

void SplashOutputDev::updateStrokeOpacity(GfxState *state) {
  splash->setStrokeAlpha((SplashCoord)state->getStrokeOpacity());
}

void SplashOutputDev::updateFont(GfxState *state) {
  GfxFont *gfxFont;
  GfxFontType fontType;
  SplashOutFontFileID *id;
  SplashFontFile *fontFile;
  SplashFontSrc *fontsrc = NULL;
  FoFiTrueType *ff;
  Ref embRef;
  Object refObj, strObj;
  GooString *fileName;
  char *tmpBuf;
  int tmpBufLen;
  Gushort *codeToGID;
  DisplayFontParam *dfp;
  CharCodeToUnicode *ctu;
  double m11, m12, m21, m22;
  SplashCoord mat[4];
  Unicode uBuf[8];
  int substIdx, n, code, cmap;
  int faceIndex = 0;

  needFontUpdate = gFalse;
  font = NULL;
  fileName = NULL;
  tmpBuf = NULL;
  tmpBufLen = 0;
  substIdx = -1;
  dfp = NULL;

  if (!(gfxFont = state->getFont())) {
    goto err1;
  }
  fontType = gfxFont->getType();
  if (fontType == fontType3) {
    goto err1;
  }

  // check the font file cache
  id = new SplashOutFontFileID(gfxFont->getID());
  if ((fontFile = fontEngine->getFontFile(id))) {
    delete id;

  } else {

    // if there is an embedded font, write it to disk
    if (gfxFont->getEmbeddedFontID(&embRef)) {
      tmpBuf = gfxFont->readEmbFontFile(xref, &tmpBufLen);
      if (! tmpBuf)
        goto err2;
    // if there is an external font file, use it
    } else if (!(fileName = gfxFont->getExtFontFile())) {

      // look for a display font mapping or a substitute font
      dfp = NULL;
      if (gfxFont->getName()) {
        dfp = globalParams->getDisplayFont(gfxFont);
      }
      if (!dfp) {
        error(-1, "Couldn't find a font for '%s'",
              gfxFont->getName() ? gfxFont->getName()->getCString()
                                 : "(unnamed)");
        goto err2;
      }
      switch (dfp->kind) {
      case displayFontT1:
        fileName = dfp->t1.fileName;
        fontType = gfxFont->isCIDFont() ? fontCIDType0 : fontType1;
        break;
      case displayFontTT:
        fileName = dfp->tt.fileName;
        fontType = gfxFont->isCIDFont() ? fontCIDType2 : fontTrueType;
        faceIndex = dfp->tt.faceIndex;
        break;
      }
    }

    fontsrc = new SplashFontSrc;
    if (fileName)
      fontsrc->setFile(fileName, gFalse);
    else
      fontsrc->setBuf(tmpBuf, tmpBufLen, gTrue);

    // load the font file
    switch (fontType) {
    case fontType1:
      fontFile = fontEngine->loadType1Font(id, fontsrc, 
                                           ((Gfx8BitFont *)gfxFont)->getEncoding());
      if (! fontFile) {
        error(-1, "Couldn't create a font for '%s'",
              gfxFont->getName() ? gfxFont->getName()->getCString()
                                 : "(unnamed)");
        goto err2;
      }
      break;
    case fontType1C:
      fontFile = fontEngine->loadType1CFont(id, fontsrc,
                                            ((Gfx8BitFont *)gfxFont)->getEncoding());
      if (! fontFile) {
        error(-1, "Couldn't create a font for '%s'",
              gfxFont->getName() ? gfxFont->getName()->getCString()
                                 : "(unnamed)");
        goto err2;
      }
      break;
    case fontTrueType:
        if (fileName)
         ff = FoFiTrueType::load(fileName->getCString());
        else
         ff = new FoFiTrueType(tmpBuf, tmpBufLen, gFalse);
        if (ff) {
          codeToGID = ((Gfx8BitFont *)gfxFont)->getCodeToGIDMap(ff);
            n = 256;
          delete ff;
        } else {
          codeToGID = NULL;
          n = 0;
        }
        if (!(fontFile = fontEngine->loadTrueTypeFont(
                             id,
                             fontsrc,
                             codeToGID, n))) {
          error(-1, "Couldn't create a font for '%s'",
                gfxFont->getName() ? gfxFont->getName()->getCString()
                                   : "(unnamed)");
          goto err2;
        }
        break;
    case fontCIDType0:
    case fontCIDType0C:
      fontFile = fontEngine->loadCIDFont(id, fontsrc);
      if (! fontFile) {
        error(-1, "Couldn't create a font for '%s'",
              gfxFont->getName() ? gfxFont->getName()->getCString()
                                 : "(unnamed)");
        goto err2;
      }
      break;
    case fontCIDType2:
      codeToGID = NULL;
      n = 0;
      if (dfp) {
        // create a CID-to-GID mapping, via Unicode
        if ((ctu = ((GfxCIDFont *)gfxFont)->getToUnicode())) {
                if (fileName)
                 ff = FoFiTrueType::load(fileName->getCString());
                else
                 ff = new FoFiTrueType(tmpBuf, tmpBufLen, gFalse);
                if (ff) {
            // look for a Unicode cmap
            for (cmap = 0; cmap < ff->getNumCmaps(); ++cmap) {
              if ((ff->getCmapPlatform(cmap) == 3 &&
                   ff->getCmapEncoding(cmap) == 1) ||
                  ff->getCmapPlatform(cmap) == 0) {
                break;
              }
            }
            if (cmap < ff->getNumCmaps()) {
              // map CID -> Unicode -> GID
              n = ctu->getLength();
              codeToGID = (Gushort *)gmallocn(n, sizeof(Gushort));
              for (code = 0; code < n; ++code) {
                if (ctu->mapToUnicode(code, uBuf, 8) > 0) {
                  codeToGID[code] = ff->mapCodeToGID(cmap, uBuf[0]);
                } else {
                  codeToGID[code] = 0;
                }
              }
            }
            delete ff;
          }
          ctu->decRefCnt();
        } else {
          error(-1, "Couldn't find a mapping to Unicode for font '%s'",
                gfxFont->getName() ? gfxFont->getName()->getCString()
                                   : "(unnamed)");
        }
      } else {
        if (((GfxCIDFont *)gfxFont)->getCIDToGID()) {
      n = ((GfxCIDFont *)gfxFont)->getCIDToGIDLen();
        if (n) {
                codeToGID = (Gushort *)gmallocn(n, sizeof(Gushort));
                memcpy(codeToGID, ((GfxCIDFont *)gfxFont)->getCIDToGID(),
                        n * sizeof(Gushort));
        } else {
                if (fileName)
                 ff = FoFiTrueType::load(fileName->getCString());
                else
                 ff = new FoFiTrueType(tmpBuf, tmpBufLen, gFalse);
                if (! ff)
                 goto err2;
                codeToGID = ((GfxCIDFont *)gfxFont)->getCodeToGIDMap(ff, &n);
                delete ff;
        }
        }
      }
      if (!(fontFile = fontEngine->loadTrueTypeFont(
                           id,
                           fontsrc,
                           codeToGID,
                           n,
                           faceIndex))) {
        error(-1, "Couldn't create a font for '%s'",
              gfxFont->getName() ? gfxFont->getName()->getCString()
                                 : "(unnamed)");
        goto err2;
      }
      break;
    default:
      // this shouldn't happen
      goto err2;
    }
  }

  // get the font matrix
  state->getFontTransMat(&m11, &m12, &m21, &m22);
  m11 *= state->getHorizScaling();
  m12 *= state->getHorizScaling();

  // create the scaled font
  mat[0] = m11;  mat[1] = -m12;
  mat[2] = m21;  mat[3] = -m22;
  if (fabs(mat[0] * mat[3] - mat[1] * mat[2]) < 0.01) {
    // avoid a singular (or close-to-singular) matrix
    mat[0] = 0.01;  mat[1] = 0;
    mat[2] = 0;     mat[3] = 0.01;
  }
  font = fontEngine->getFont(fontFile, mat);

  if (fontsrc && !fontsrc->isFile)
      fontsrc->unref();
  return;

 err2:
  delete id;
 err1:
  if (fontsrc && !fontsrc->isFile)
      fontsrc->unref();
  return;
}

void SplashOutputDev::stroke(GfxState *state) {
  SplashPath *path;

  path = convertPath(state, state->getPath());
  splash->stroke(path);
  delete path;
}

void SplashOutputDev::fill(GfxState *state) {
  SplashPath *path;

  path = convertPath(state, state->getPath());
  splash->fill(path, gFalse);
  delete path;
}

void SplashOutputDev::eoFill(GfxState *state) {
  SplashPath *path;

  path = convertPath(state, state->getPath());
  splash->fill(path, gTrue);
  delete path;
}

void SplashOutputDev::clip(GfxState *state) {
  SplashPath *path;

  path = convertPath(state, state->getPath());
  splash->clipToPath(path, gFalse);
  delete path;
}

void SplashOutputDev::eoClip(GfxState *state) {
  SplashPath *path;

  path = convertPath(state, state->getPath());
  splash->clipToPath(path, gTrue);
  delete path;
}

SplashPath *SplashOutputDev::convertPath(GfxState *state, GfxPath *path) {
  SplashPath *sPath;
  GfxSubpath *subpath;
  double x1, y1, x2, y2, x3, y3;
  int i, j;

  sPath = new SplashPath();
  for (i = 0; i < path->getNumSubpaths(); ++i) {
    subpath = path->getSubpath(i);
    if (subpath->getNumPoints() > 0) {
      state->transform(subpath->getX(0), subpath->getY(0), &x1, &y1);
      sPath->moveTo((SplashCoord)x1, (SplashCoord)y1);
      j = 1;
      while (j < subpath->getNumPoints()) {
        if (subpath->getCurve(j)) {
          state->transform(subpath->getX(j), subpath->getY(j), &x1, &y1);
          state->transform(subpath->getX(j+1), subpath->getY(j+1), &x2, &y2);
          state->transform(subpath->getX(j+2), subpath->getY(j+2), &x3, &y3);
          sPath->curveTo((SplashCoord)x1, (SplashCoord)y1,
                         (SplashCoord)x2, (SplashCoord)y2,
                         (SplashCoord)x3, (SplashCoord)y3);
          j += 3;
        } else {
          state->transform(subpath->getX(j), subpath->getY(j), &x1, &y1);
          sPath->lineTo((SplashCoord)x1, (SplashCoord)y1);
          ++j;
        }
      }
      if (subpath->isClosed()) {
        sPath->close();
      }
    }
  }
  return sPath;
}

void SplashOutputDev::drawChar(GfxState *state, double x, double y,
                               double dx, double dy,
                               double originX, double originY,
                               CharCode code, int nBytes,
                               Unicode *u, int uLen) {
  double x1, y1;
  SplashPath *path;
  int render;

  if (needFontUpdate) {
    updateFont(state);
  }
  if (!font) {
    return;
  }

  // check for invisible text -- this is used by Acrobat Capture
  render = state->getRender();
  if (render == 3) {
    return;
  }

  x -= originX;
  y -= originY;
  state->transform(x, y, &x1, &y1);

  // fill
  if (!(render & 1)) {
    splash->fillChar((SplashCoord)x1, (SplashCoord)y1, code, font);
  }

  // stroke
  if ((render & 3) == 1 || (render & 3) == 2) {
    if ((path = font->getGlyphPath(code))) {
      path->offset((SplashCoord)x1, (SplashCoord)y1);
      splash->stroke(path);
      delete path;
    }
  }

  // clip
  if (render & 4) {
    path = font->getGlyphPath(code);
    path->offset((SplashCoord)x1, (SplashCoord)y1);
    if (textClipPath) {
      textClipPath->append(path);
      delete path;
    } else {
      textClipPath = path;
    }
  }
}

GBool SplashOutputDev::beginType3Char(GfxState *state, double x, double y,
                                      double dx, double dy,
                                      CharCode code, Unicode *u, int uLen) {
  GfxFont *gfxFont;
  Ref *fontID;
  double *ctm, *bbox;
  T3FontCache *t3Font;
  T3GlyphStack *t3gs;
  double x1, y1, xMin, yMin, xMax, yMax, xt, yt;
  int i, j;

  if (!(gfxFont = state->getFont())) {
    return gFalse;
  }
  fontID = gfxFont->getID();
  ctm = state->getCTM();
  state->transform(0, 0, &xt, &yt);

  // is it the first (MRU) font in the cache?
  if (!(nT3Fonts > 0 &&
        t3FontCache[0]->matches(fontID, ctm[0], ctm[1], ctm[2], ctm[3]))) {

    // is the font elsewhere in the cache?
    for (i = 1; i < nT3Fonts; ++i) {
      if (t3FontCache[i]->matches(fontID, ctm[0], ctm[1], ctm[2], ctm[3])) {
        t3Font = t3FontCache[i];
        for (j = i; j > 0; --j) {
          t3FontCache[j] = t3FontCache[j - 1];
        }
        t3FontCache[0] = t3Font;
        break;
      }
    }
    if (i >= nT3Fonts) {

      // create new entry in the font cache
      if (nT3Fonts == splashOutT3FontCacheSize) {
        delete t3FontCache[nT3Fonts - 1];
        --nT3Fonts;
      }
      for (j = nT3Fonts; j > 0; --j) {
        t3FontCache[j] = t3FontCache[j - 1];
      }
      ++nT3Fonts;
      bbox = gfxFont->getFontBBox();
      if (bbox[0] == 0 && bbox[1] == 0 && bbox[2] == 0 && bbox[3] == 0) {
        // broken bounding box -- just take a guess
        xMin = xt - 5;
        xMax = xMin + 30;
        yMax = yt + 15;
        yMin = yMax - 45;
      } else {
        state->transform(bbox[0], bbox[1], &x1, &y1);
        xMin = xMax = x1;
        yMin = yMax = y1;
        state->transform(bbox[0], bbox[3], &x1, &y1);
        if (x1 < xMin) {
          xMin = x1;
        } else if (x1 > xMax) {
          xMax = x1;
        }
        if (y1 < yMin) {
          yMin = y1;
        } else if (y1 > yMax) {
          yMax = y1;
        }
        state->transform(bbox[2], bbox[1], &x1, &y1);
        if (x1 < xMin) {
          xMin = x1;
        } else if (x1 > xMax) {
          xMax = x1;
        }
        if (y1 < yMin) {
          yMin = y1;
        } else if (y1 > yMax) {
          yMax = y1;
        }
        state->transform(bbox[2], bbox[3], &x1, &y1);
        if (x1 < xMin) {
          xMin = x1;
        } else if (x1 > xMax) {
          xMax = x1;
        }
        if (y1 < yMin) {
          yMin = y1;
        } else if (y1 > yMax) {
          yMax = y1;
        }
      }
      t3FontCache[0] = new T3FontCache(fontID, ctm[0], ctm[1], ctm[2], ctm[3],
                                       (int)floor(xMin - xt),
                                       (int)floor(yMin - yt),
                                       (int)ceil(xMax) - (int)floor(xMin) + 3,
                                       (int)ceil(yMax) - (int)floor(yMin) + 3,
                                       colorMode != splashModeMono1);
    }
  }
  t3Font = t3FontCache[0];

  // is the glyph in the cache?
  i = (code & (t3Font->cacheSets - 1)) * t3Font->cacheAssoc;
  for (j = 0; j < t3Font->cacheAssoc; ++j) {
    if ((t3Font->cacheTags[i+j].mru & 0x8000) &&
        t3Font->cacheTags[i+j].code == code) {
      drawType3Glyph(t3Font, &t3Font->cacheTags[i+j],
                     t3Font->cacheData + (i+j) * t3Font->glyphSize,
                     xt, yt);
      return gTrue;
    }
  }

  // push a new Type 3 glyph record
  t3gs = new T3GlyphStack();
  t3gs->next = t3GlyphStack;
  t3GlyphStack = t3gs;
  t3GlyphStack->code = code;
  t3GlyphStack->x = xt;
  t3GlyphStack->y = yt;
  t3GlyphStack->cache = t3Font;
  t3GlyphStack->cacheTag = NULL;
  t3GlyphStack->cacheData = NULL;

  return gFalse;
}

void SplashOutputDev::endType3Char(GfxState *state) {
  T3GlyphStack *t3gs;
  double *ctm;

  if (t3GlyphStack->cacheTag) {
    memcpy(t3GlyphStack->cacheData, bitmap->getDataPtr(),
           t3GlyphStack->cache->glyphSize);
    delete bitmap;
    delete splash;
    bitmap = t3GlyphStack->origBitmap;
    splash = t3GlyphStack->origSplash;
    ctm = state->getCTM();
    state->setCTM(ctm[0], ctm[1], ctm[2], ctm[3],
                  t3GlyphStack->origCTM4, t3GlyphStack->origCTM5);
    drawType3Glyph(t3GlyphStack->cache,
                   t3GlyphStack->cacheTag, t3GlyphStack->cacheData,
                   t3GlyphStack->x, t3GlyphStack->y);
  }
  t3gs = t3GlyphStack;
  t3GlyphStack = t3gs->next;
  delete t3gs;
}

void SplashOutputDev::type3D0(GfxState *state, double wx, double wy) {
}

void SplashOutputDev::type3D1(GfxState *state, double wx, double wy,
                              double llx, double lly, double urx, double ury) {
  double *ctm;
  T3FontCache *t3Font;
  SplashColor color;
  double xt, yt, xMin, xMax, yMin, yMax, x1, y1;
  int i, j;

  t3Font = t3GlyphStack->cache;

  // check for a valid bbox
  state->transform(0, 0, &xt, &yt);
  state->transform(llx, lly, &x1, &y1);
  xMin = xMax = x1;
  yMin = yMax = y1;
  state->transform(llx, ury, &x1, &y1);
  if (x1 < xMin) {
    xMin = x1;
  } else if (x1 > xMax) {
    xMax = x1;
  }
  if (y1 < yMin) {
    yMin = y1;
  } else if (y1 > yMax) {
    yMax = y1;
  }
  state->transform(urx, lly, &x1, &y1);
  if (x1 < xMin) {
    xMin = x1;
  } else if (x1 > xMax) {
    xMax = x1;
  }
  if (y1 < yMin) {
    yMin = y1;
  } else if (y1 > yMax) {
    yMax = y1;
  }
  state->transform(urx, ury, &x1, &y1);
  if (x1 < xMin) {
    xMin = x1;
  } else if (x1 > xMax) {
    xMax = x1;
  }
  if (y1 < yMin) {
    yMin = y1;
  } else if (y1 > yMax) {
    yMax = y1;
  }
  if (xMin - xt < t3Font->glyphX ||
      yMin - yt < t3Font->glyphY ||
      xMax - xt > t3Font->glyphX + t3Font->glyphW ||
      yMax - yt > t3Font->glyphY + t3Font->glyphH) {
    error(-1, "Bad bounding box in Type 3 glyph");
    return;
  }

  // allocate a cache entry
  i = (t3GlyphStack->code & (t3Font->cacheSets - 1)) * t3Font->cacheAssoc;
  for (j = 0; j < t3Font->cacheAssoc; ++j) {
    if ((t3Font->cacheTags[i+j].mru & 0x7fff) == t3Font->cacheAssoc - 1) {
      t3Font->cacheTags[i+j].mru = 0x8000;
      t3Font->cacheTags[i+j].code = t3GlyphStack->code;
      t3GlyphStack->cacheTag = &t3Font->cacheTags[i+j];
      t3GlyphStack->cacheData = t3Font->cacheData + (i+j) * t3Font->glyphSize;
    } else {
      ++t3Font->cacheTags[i+j].mru;
    }
  }

  // save state
  t3GlyphStack->origBitmap = bitmap;
  t3GlyphStack->origSplash = splash;
  ctm = state->getCTM();
  t3GlyphStack->origCTM4 = ctm[4];
  t3GlyphStack->origCTM5 = ctm[5];

  // create the temporary bitmap
  if (colorMode == splashModeMono1) {
    bitmap = new SplashBitmap(t3Font->glyphW, t3Font->glyphH, 1,
                              splashModeMono1);
    splash = new Splash(bitmap);
    color[0] = 0;
    splash->clear(color);
    color[0] = 1;
  } else {
    bitmap = new SplashBitmap(t3Font->glyphW, t3Font->glyphH, 1,
                              splashModeMono8);
    splash = new Splash(bitmap);
    color[0] = 0x00;
    splash->clear(color);
    color[0] = 0xff;
  }
  splash->setFillPattern(new SplashSolidColor(color));
  splash->setStrokePattern(new SplashSolidColor(color));
  //~ this should copy other state from t3GlyphStack->origSplash?
  state->setCTM(ctm[0], ctm[1], ctm[2], ctm[3],
                -t3Font->glyphX, -t3Font->glyphY);
}

void SplashOutputDev::drawType3Glyph(T3FontCache *t3Font,
                                     T3FontCacheTag *tag, Guchar *data,
                                     double x, double y) {
  SplashGlyphBitmap glyph;

  glyph.x = -t3Font->glyphX;
  glyph.y = -t3Font->glyphY;
  glyph.w = t3Font->glyphW;
  glyph.h = t3Font->glyphH;
  glyph.aa = colorMode != splashModeMono1;
  glyph.data = data;
  glyph.freeData = gFalse;
  splash->fillGlyph((SplashCoord)x, (SplashCoord)y, &glyph);
}

void SplashOutputDev::endTextObject(GfxState *state) {
  if (textClipPath) {
    splash->clipToPath(textClipPath, gFalse);
    delete textClipPath;
    textClipPath = NULL;
  }
}

struct SplashOutImageMaskData {
  ImageStream *imgStr;
  GBool invert;
  int width, height, y;
};

GBool SplashOutputDev::imageMaskSrc(void *data, SplashColorPtr line) {
  SplashOutImageMaskData *imgMaskData = (SplashOutImageMaskData *)data;
  Guchar *p;
  SplashColorPtr q;
  int x;

  if (imgMaskData->y == imgMaskData->height) {
    return gFalse;
  }
  for (x = 0, p = imgMaskData->imgStr->getLine(), q = line;
       x < imgMaskData->width;
       ++x) {
    *q++ = *p++ ^ imgMaskData->invert;
  }
  ++imgMaskData->y;
  return gTrue;
}

void SplashOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
                                    int width, int height, GBool invert,
                                    GBool inlineImg) {
  double *ctm;
  SplashCoord mat[6];
  SplashOutImageMaskData imgMaskData;

  ctm = state->getCTM();
  mat[0] = ctm[0];
  mat[1] = ctm[1];
  mat[2] = -ctm[2];
  mat[3] = -ctm[3];
  mat[4] = ctm[2] + ctm[4];
  mat[5] = ctm[3] + ctm[5];

  imgMaskData.imgStr = new ImageStream(str, width, 1, 1);
  imgMaskData.imgStr->reset();
  imgMaskData.invert = invert ? 0 : 1;
  imgMaskData.width = width;
  imgMaskData.height = height;
  imgMaskData.y = 0;

  splash->fillImageMask(&imageMaskSrc, &imgMaskData, width, height, mat);
  if (inlineImg) {
    while (imgMaskData.y < height) {
      imgMaskData.imgStr->getLine();
      ++imgMaskData.y;
    }
  }

  delete imgMaskData.imgStr;
  str->close();
}

struct SplashOutImageData {
  ImageStream *imgStr;
  GfxImageColorMap *colorMap;
  SplashColorPtr lookup;
  int *maskColors;
  SplashColorMode colorMode;
  int width, height, y;
};

GBool SplashOutputDev::imageSrc(void *data, SplashColorPtr line) {
  SplashOutImageData *imgData = (SplashOutImageData *)data;
  Guchar *p;
  SplashColorPtr q, col;
  GfxRGB rgb;
  GfxGray gray;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif
  int nComps, x;

  if (imgData->y == imgData->height) {
    return gFalse;
  }

  nComps = imgData->colorMap->getNumPixelComps();

  if (imgData->lookup) {
    switch (imgData->colorMode) {
  case splashModeMono1:
  case splashModeMono8:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
           x < imgData->width;
           ++x, ++p) {
        *q++ = imgData->lookup[*p];
      }
    break;
  case splashModeRGB8:
    case splashModeBGR8:
    case splashModeRGB8Qt:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
           x < imgData->width;
           ++x, ++p) {
        col = &imgData->lookup[3 * *p];
        *q++ = col[0];
        *q++ = col[1];
        *q++ = col[2];
      }
    break;
#if SPLASH_CMYK
    case splashModeCMYK8:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
           x < imgData->width;
           ++x, ++p) {
        col = &imgData->lookup[4 * *p];
        *q++ = col[0];
        *q++ = col[1];
        *q++ = col[2];
        *q++ = col[3];
      }
      break;
#endif
    case splashModeAMono8:
    case splashModeARGB8:
    case splashModeBGRA8:
#if SPLASH_CMYK
    case splashModeACMYK8:
#endif
      //~ unimplemented
    break;
  }
  } else {
    switch (imgData->colorMode) {
    case splashModeMono1:
    case splashModeMono8:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
           x < imgData->width;
           ++x, p += nComps) {
        imgData->colorMap->getGray(p, &gray);
        *q++ = colToByte(gray);
      }
        break;
    case splashModeRGB8:
    case splashModeRGB8Qt:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
           x < imgData->width;
           ++x, p += nComps) {
        imgData->colorMap->getRGB(p, &rgb);
        *q++ = colToByte(rgb.r);
        *q++ = colToByte(rgb.g);
        *q++ = colToByte(rgb.b);
      }
      break;
    case splashModeBGR8:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
           x < imgData->width;
           ++x, p += nComps) {
        imgData->colorMap->getRGB(p, &rgb);
        *q++ = colToByte(rgb.b);
        *q++ = colToByte(rgb.g);
        *q++ = colToByte(rgb.r);
      }
      break;
#if SPLASH_CMYK
    case splashModeCMYK8:
      for (x = 0, p = imgData->imgStr->getLine(), q = line;
           x < imgData->width;
           ++x, p += nComps) {
        imgData->colorMap->getCMYK(p, &cmyk);
        *q++ = colToByte(cmyk.c);
        *q++ = colToByte(cmyk.m);
        *q++ = colToByte(cmyk.y);
        *q++ = colToByte(cmyk.k);
      }
      break;
#endif
    case splashModeAMono8:
    case splashModeARGB8:
    case splashModeBGRA8:
#if SPLASH_CMYK
    case splashModeACMYK8:
#endif
      //~ unimplemented
      break;
    }
  }

  ++imgData->y;
  return gTrue;
}

GBool SplashOutputDev::alphaImageSrc(void *data, SplashColorPtr line) {
  SplashOutImageData *imgData = (SplashOutImageData *)data;
  Guchar *p;
  SplashColorPtr q, col;
  GfxRGB rgb;
  GfxGray gray;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif
  Guchar alpha;
  int nComps, x, i;

  if (imgData->y == imgData->height) {
    return gFalse;
  }

  nComps = imgData->colorMap->getNumPixelComps();

  for (x = 0, p = imgData->imgStr->getLine(), q = line;
       x < imgData->width;
       ++x, p += nComps) {
    alpha = 0;
    for (i = 0; i < nComps; ++i) {
      if (p[i] < imgData->maskColors[2*i] ||
          p[i] > imgData->maskColors[2*i+1]) {
        alpha = 0xff;
        break;
      }
    }
    if (imgData->lookup) {
      switch (imgData->colorMode) {
      case splashModeMono1:
      case splashModeMono8:
        *q++ = alpha;
        *q++ = imgData->lookup[*p];
        break;
      case splashModeRGB8:
      case splashModeRGB8Qt:
        *q++ = alpha;
        col = &imgData->lookup[3 * *p];
        *q++ = col[0];
        *q++ = col[1];
        *q++ = col[2];
        break;
      case splashModeBGR8:
        col = &imgData->lookup[3 * *p];
        *q++ = col[0];
        *q++ = col[1];
        *q++ = col[2];
        *q++ = alpha;
        break;
#if SPLASH_CMYK
      case splashModeCMYK8:
        *q++ = alpha;
        col = &imgData->lookup[4 * *p];
        *q++ = col[0];
        *q++ = col[1];
        *q++ = col[2];
        *q++ = col[3];
        break;
#endif
      case splashModeAMono8:
      case splashModeARGB8:
      case splashModeBGRA8:
#if SPLASH_CMYK
      case splashModeACMYK8:
#endif
        //~ unimplemented
        break;
      }
    } else {
      switch (imgData->colorMode) {
      case splashModeMono1:
      case splashModeMono8:
        imgData->colorMap->getGray(p, &gray);
        *q++ = alpha;
        *q++ = colToByte(gray);
        break;
      case splashModeRGB8:
      case splashModeRGB8Qt:
        imgData->colorMap->getRGB(p, &rgb);
        *q++ = alpha;
        *q++ = colToByte(rgb.r);
        *q++ = colToByte(rgb.g);
        *q++ = colToByte(rgb.b);
        break;
      case splashModeBGR8:
        imgData->colorMap->getRGB(p, &rgb);
        *q++ = colToByte(rgb.b);
        *q++ = colToByte(rgb.g);
        *q++ = colToByte(rgb.r);
        *q++ = alpha;
        break;
#if SPLASH_CMYK
      case splashModeCMYK8:
        imgData->colorMap->getCMYK(p, &cmyk);
        *q++ = alpha;
        *q++ = colToByte(cmyk.c);
        *q++ = colToByte(cmyk.m);
        *q++ = colToByte(cmyk.y);
        *q++ = colToByte(cmyk.k);
        break;
#endif
      case splashModeAMono8:
      case splashModeARGB8:
      case splashModeBGRA8:
#if SPLASH_CMYK
      case splashModeACMYK8:
#endif
        //~ unimplemented
        break;
      }
    }
  }

  ++imgData->y;
  return gTrue;
}

void SplashOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
                                int width, int height,
                                GfxImageColorMap *colorMap,
                                int *maskColors, GBool inlineImg) {
  double *ctm;
  SplashCoord mat[6];
  SplashOutImageData imgData;
  SplashColorMode srcMode;
  SplashImageSource src;
  GfxGray gray;
  GfxRGB rgb;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif
  Guchar pix;
  int n, i;

  ctm = state->getCTM();
  mat[0] = ctm[0];
  mat[1] = ctm[1];
  mat[2] = -ctm[2];
  mat[3] = -ctm[3];
  mat[4] = ctm[2] + ctm[4];
  mat[5] = ctm[3] + ctm[5];

  imgData.imgStr = new ImageStream(str, width,
                                   colorMap->getNumPixelComps(),
                                   colorMap->getBits());
  imgData.imgStr->reset();
  imgData.colorMap = colorMap;
  imgData.maskColors = maskColors;
  imgData.colorMode = colorMode;
  imgData.width = width;
  imgData.height = height;
  imgData.y = 0;

  // special case for one-channel (monochrome/gray/separation) images:
  // build a lookup table here
  imgData.lookup = NULL;
  if (colorMap->getNumPixelComps() == 1) {
    n = 1 << colorMap->getBits();
    switch (colorMode) {
    case splashModeMono1:
    case splashModeMono8:
      imgData.lookup = (SplashColorPtr)gmalloc(n);
      for (i = 0; i < n; ++i) {
        pix = (Guchar)i;
        colorMap->getGray(&pix, &gray);
        imgData.lookup[i] = colToByte(gray);
      }
      break;
    case splashModeRGB8:
    case splashModeRGB8Qt:
      imgData.lookup = (SplashColorPtr)gmalloc(3 * n);
      for (i = 0; i < n; ++i) {
        pix = (Guchar)i;
        colorMap->getRGB(&pix, &rgb);
        imgData.lookup[3*i] = colToByte(rgb.r);
        imgData.lookup[3*i+1] = colToByte(rgb.g);
        imgData.lookup[3*i+2] = colToByte(rgb.b);
      }
      break;
    case splashModeBGR8:
      imgData.lookup = (SplashColorPtr)gmalloc(3 * n);
      for (i = 0; i < n; ++i) {
        pix = (Guchar)i;
        colorMap->getRGB(&pix, &rgb);
        imgData.lookup[3*i] = colToByte(rgb.b);
        imgData.lookup[3*i+1] = colToByte(rgb.g);
        imgData.lookup[3*i+2] = colToByte(rgb.r);
      }
      break;
#if SPLASH_CMYK
    case splashModeCMYK8:
      imgData.lookup = (SplashColorPtr)gmalloc(4 * n);
      for (i = 0; i < n; ++i) {
        pix = (Guchar)i;
        colorMap->getCMYK(&pix, &cmyk);
        imgData.lookup[4*i] = colToByte(cmyk.c);
        imgData.lookup[4*i+1] = colToByte(cmyk.m);
        imgData.lookup[4*i+2] = colToByte(cmyk.y);
        imgData.lookup[4*i+3] = colToByte(cmyk.k);
      }
      break;
#endif
    default:
      //~ unimplemented
      break;
    }
  }

  switch (colorMode) {
  case splashModeMono1:
  case splashModeMono8:
    srcMode = maskColors ? splashModeAMono8 : splashModeMono8;
    break;
  case splashModeRGB8:
  case splashModeRGB8Qt:
    srcMode = maskColors ? splashModeARGB8 : splashModeRGB8;
    break;
  case splashModeBGR8:
    srcMode = maskColors ? splashModeBGRA8 : splashModeBGR8;
    break;
#if SPLASH_CMYK
  case splashModeCMYK8:
    srcMode = maskColors ? splashModeACMYK8 : splashModeCMYK8;
    break;
#endif
  default:
    //~ unimplemented
    srcMode = splashModeRGB8;
    break;
  }  
  src = maskColors ? &alphaImageSrc : &imageSrc;
  splash->drawImage(src, &imgData, srcMode, width, height, mat);
  if (inlineImg) {
    while (imgData.y < height) {
      imgData.imgStr->getLine();
      ++imgData.y;
    }
  }

  gfree(imgData.lookup);
  delete imgData.imgStr;
  str->close();
}

struct SplashOutMaskedImageData {
  ImageStream *imgStr;
  GfxImageColorMap *colorMap;
  SplashBitmap *mask;
  SplashColorPtr lookup;
  SplashColorMode colorMode;
  int width, height, y;
};

GBool SplashOutputDev::maskedImageSrc(void *data, SplashColorPtr line) {
  SplashOutMaskedImageData *imgData = (SplashOutMaskedImageData *)data;
  Guchar *p;
  SplashColor maskColor;
  SplashColorPtr q, col;
  GfxRGB rgb;
  GfxGray gray;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif
  Guchar alpha;
  int nComps, x;

  if (imgData->y == imgData->height) {
    return gFalse;
  }

  nComps = imgData->colorMap->getNumPixelComps();

  for (x = 0, p = imgData->imgStr->getLine(), q = line;
       x < imgData->width;
       ++x, p += nComps) {
    imgData->mask->getPixel(x, imgData->y, maskColor);
    alpha = maskColor[0] ? 0xff : 0x00;
    if (imgData->lookup) {
      switch (imgData->colorMode) {
      case splashModeMono1:
      case splashModeMono8:
        *q++ = alpha;
        *q++ = imgData->lookup[*p];
        break;
      case splashModeRGB8:
      case splashModeRGB8Qt:
        *q++ = alpha;
        col = &imgData->lookup[3 * *p];
        *q++ = col[0];
        *q++ = col[1];
        *q++ = col[2];
        break;
      case splashModeBGR8:
        col = &imgData->lookup[3 * *p];
        *q++ = col[0];
        *q++ = col[1];
        *q++ = col[2];
        *q++ = alpha;
        break;
#if SPLASH_CMYK
      case splashModeCMYK8:
        *q++ = alpha;
        col = &imgData->lookup[4 * *p];
        *q++ = col[0];
        *q++ = col[1];
        *q++ = col[2];
        *q++ = col[3];
        break;
#endif
      case splashModeAMono8:
      case splashModeARGB8:
      case splashModeBGRA8:
#if SPLASH_CMYK
      case splashModeACMYK8:
#endif
        //~ unimplemented
        break;
      }
    } else {
      switch (imgData->colorMode) {
      case splashModeMono1:
      case splashModeMono8:
        imgData->colorMap->getGray(p, &gray);
        *q++ = alpha;
        *q++ = colToByte(gray);
        break;
      case splashModeRGB8:
      case splashModeRGB8Qt:
        imgData->colorMap->getRGB(p, &rgb);
        *q++ = alpha;
        *q++ = colToByte(rgb.r);
        *q++ = colToByte(rgb.g);
        *q++ = colToByte(rgb.b);
        break;
      case splashModeBGR8:
        imgData->colorMap->getRGB(p, &rgb);
        *q++ = colToByte(rgb.b);
        *q++ = colToByte(rgb.g);
        *q++ = colToByte(rgb.r);
        *q++ = alpha;
        break;
#if SPLASH_CMYK
      case splashModeCMYK8:
        imgData->colorMap->getCMYK(p, &cmyk);
        *q++ = alpha;
        *q++ = colToByte(cmyk.c);
        *q++ = colToByte(cmyk.m);
        *q++ = colToByte(cmyk.y);
        *q++ = colToByte(cmyk.k);
        break;
#endif
      case splashModeAMono8:
      case splashModeARGB8:
      case splashModeBGRA8:
#if SPLASH_CMYK
      case splashModeACMYK8:
#endif
        //~ unimplemented
        break;
      }
    }
  }

  ++imgData->y;
  return gTrue;
}

void SplashOutputDev::drawMaskedImage(GfxState *state, Object *ref,
                                      Stream *str, int width, int height,
                                      GfxImageColorMap *colorMap,
                                      Stream *maskStr, int maskWidth,
                                      int maskHeight, GBool maskInvert) {
  double *ctm;
  SplashCoord mat[6];
  SplashOutMaskedImageData imgData;
  SplashOutImageMaskData imgMaskData;
  SplashColorMode srcMode;
  SplashBitmap *maskBitmap;
  Splash *maskSplash;
  SplashColor maskColor;
  GfxGray gray;
  GfxRGB rgb;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif
  Guchar pix;
  int n, i;

  //----- scale the mask image to the same size as the source image

  mat[0] = (SplashCoord)width;
  mat[1] = 0;
  mat[2] = 0;
  mat[3] = (SplashCoord)height;
  mat[4] = 0;
  mat[5] = 0;
  imgMaskData.imgStr = new ImageStream(maskStr, maskWidth, 1, 1);
  imgMaskData.imgStr->reset();
  imgMaskData.invert = maskInvert ? 0 : 1;
  imgMaskData.width = maskWidth;
  imgMaskData.height = maskHeight;
  imgMaskData.y = 0;
  maskBitmap = new SplashBitmap(width, height, 1, splashModeMono1);
  maskSplash = new Splash(maskBitmap);
  maskColor[0] = 0;
  maskSplash->clear(maskColor);
  maskColor[0] = 1;
  maskSplash->setFillPattern(new SplashSolidColor(maskColor));
  maskSplash->fillImageMask(&imageMaskSrc, &imgMaskData,
                            maskWidth, maskHeight, mat);
  delete imgMaskData.imgStr;
  maskStr->close();
  delete maskSplash;

  //----- draw the source image

  ctm = state->getCTM();
  mat[0] = ctm[0];
  mat[1] = ctm[1];
  mat[2] = -ctm[2];
  mat[3] = -ctm[3];
  mat[4] = ctm[2] + ctm[4];
  mat[5] = ctm[3] + ctm[5];

  imgData.imgStr = new ImageStream(str, width,
                                   colorMap->getNumPixelComps(),
                                   colorMap->getBits());
  imgData.imgStr->reset();
  imgData.colorMap = colorMap;
  imgData.mask = maskBitmap;
  imgData.colorMode = colorMode;
  imgData.width = width;
  imgData.height = height;
  imgData.y = 0;

  // special case for one-channel (monochrome/gray/separation) images:
  // build a lookup table here
  imgData.lookup = NULL;
  if (colorMap->getNumPixelComps() == 1) {
    n = 1 << colorMap->getBits();
    switch (colorMode) {
    case splashModeMono1:
    case splashModeMono8:
      imgData.lookup = (SplashColorPtr)gmalloc(n);
      for (i = 0; i < n; ++i) {
        pix = (Guchar)i;
        colorMap->getGray(&pix, &gray);
        imgData.lookup[i] = colToByte(gray);
      }
      break;
    case splashModeRGB8:
    case splashModeRGB8Qt:
      imgData.lookup = (SplashColorPtr)gmalloc(3 * n);
      for (i = 0; i < n; ++i) {
        pix = (Guchar)i;
        colorMap->getRGB(&pix, &rgb);
        imgData.lookup[3*i] = colToByte(rgb.r);
        imgData.lookup[3*i+1] = colToByte(rgb.g);
        imgData.lookup[3*i+2] = colToByte(rgb.b);
      }
      break;
    case splashModeBGR8:
      imgData.lookup = (SplashColorPtr)gmalloc(3 * n);
      for (i = 0; i < n; ++i) {
        pix = (Guchar)i;
        colorMap->getRGB(&pix, &rgb);
        imgData.lookup[3*i] = colToByte(rgb.b);
        imgData.lookup[3*i+1] = colToByte(rgb.g);
        imgData.lookup[3*i+2] = colToByte(rgb.r);
      }
      break;
#if SPLASH_CMYK
    case splashModeCMYK8:
      imgData.lookup = (SplashColorPtr)gmalloc(4 * n);
      for (i = 0; i < n; ++i) {
        pix = (Guchar)i;
        colorMap->getCMYK(&pix, &cmyk);
        imgData.lookup[4*i] = colToByte(cmyk.c);
        imgData.lookup[4*i+1] = colToByte(cmyk.m);
        imgData.lookup[4*i+2] = colToByte(cmyk.y);
        imgData.lookup[4*i+3] = colToByte(cmyk.k);
      }
      break;
#endif
    default:
      //~ unimplemented
      break;
    }
  }

  switch (colorMode) {
  case splashModeMono1:
  case splashModeMono8:
    srcMode = splashModeAMono8;
    break;
  case splashModeRGB8:
  case splashModeRGB8Qt:
    srcMode = splashModeARGB8;
    break;
  case splashModeBGR8:
    srcMode = splashModeBGRA8;
    break;
#if SPLASH_CMYK
  case splashModeCMYK8:
    srcMode = splashModeACMYK8;
    break;
#endif
  default:
    //~ unimplemented
    srcMode = splashModeARGB8;
    break;
  }  
  splash->drawImage(&maskedImageSrc, &imgData, srcMode, width, height, mat);

  delete maskBitmap;
  gfree(imgData.lookup);
  delete imgData.imgStr;
  str->close();
}

void SplashOutputDev::drawSoftMaskedImage(GfxState *state, Object *ref,
                                          Stream *str, int width, int height,
                                          GfxImageColorMap *colorMap,
                                          Stream *maskStr,
                                          int maskWidth, int maskHeight,
                                          GfxImageColorMap *maskColorMap) {
  double *ctm;
  SplashCoord mat[6];
  SplashOutImageData imgData;
  SplashOutImageData imgMaskData;
  SplashColorMode srcMode;
  SplashBitmap *maskBitmap;
  Splash *maskSplash;
  SplashColor maskColor;
  GfxGray gray;
  GfxRGB rgb;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif
  Guchar pix;
  int n, i;

  ctm = state->getCTM();
  mat[0] = ctm[0];
  mat[1] = ctm[1];
  mat[2] = -ctm[2];
  mat[3] = -ctm[3];
  mat[4] = ctm[2] + ctm[4];
  mat[5] = ctm[3] + ctm[5];

  //----- set up the soft mask

  imgMaskData.imgStr = new ImageStream(maskStr, maskWidth,
                                       maskColorMap->getNumPixelComps(),
                                       maskColorMap->getBits());
  imgMaskData.imgStr->reset();
  imgMaskData.colorMap = maskColorMap;
  imgMaskData.maskColors = NULL;
  imgMaskData.colorMode = splashModeMono8;
  imgMaskData.width = maskWidth;
  imgMaskData.height = maskHeight;
  imgMaskData.y = 0;
  n = 1 << maskColorMap->getBits();
  imgMaskData.lookup = (SplashColorPtr)gmalloc(n);
  for (i = 0; i < n; ++i) {
    pix = (Guchar)i;
    maskColorMap->getGray(&pix, &gray);
    imgMaskData.lookup[i] = colToByte(gray);
  }
  maskBitmap = new SplashBitmap(bitmap->getWidth(), bitmap->getHeight(),
                                1, splashModeMono8);
  maskSplash = new Splash(maskBitmap);
  maskColor[0] = 0;
  maskSplash->clear(maskColor);
  maskSplash->drawImage(&imageSrc, &imgMaskData,
                        splashModeMono8, maskWidth, maskHeight, mat);
  delete imgMaskData.imgStr;
  maskStr->close();
  gfree(imgMaskData.lookup);
  delete maskSplash;
  splash->setSoftMask(maskBitmap);

  //----- draw the source image

  imgData.imgStr = new ImageStream(str, width,
                                   colorMap->getNumPixelComps(),
                                   colorMap->getBits());
  imgData.imgStr->reset();
  imgData.colorMap = colorMap;
  imgData.maskColors = NULL;
  imgData.colorMode = colorMode;
  imgData.width = width;
  imgData.height = height;
  imgData.y = 0;

  // special case for one-channel (monochrome/gray/separation) images:
  // build a lookup table here
  imgData.lookup = NULL;
  if (colorMap->getNumPixelComps() == 1) {
    n = 1 << colorMap->getBits();
    switch (colorMode) {
    case splashModeMono1:
    case splashModeMono8:
      imgData.lookup = (SplashColorPtr)gmalloc(n);
      for (i = 0; i < n; ++i) {
        pix = (Guchar)i;
        colorMap->getGray(&pix, &gray);
        imgData.lookup[i] = colToByte(gray);
      }
      break;
    case splashModeRGB8:
    case splashModeRGB8Qt:
      imgData.lookup = (SplashColorPtr)gmalloc(3 * n);
      for (i = 0; i < n; ++i) {
        pix = (Guchar)i;
        colorMap->getRGB(&pix, &rgb);
        imgData.lookup[3*i] = colToByte(rgb.r);
        imgData.lookup[3*i+1] = colToByte(rgb.g);
        imgData.lookup[3*i+2] = colToByte(rgb.b);
      }
      break;
    case splashModeBGR8:
      imgData.lookup = (SplashColorPtr)gmalloc(3 * n);
      for (i = 0; i < n; ++i) {
        pix = (Guchar)i;
        colorMap->getRGB(&pix, &rgb);
        imgData.lookup[3*i] = colToByte(rgb.b);
        imgData.lookup[3*i+1] = colToByte(rgb.g);
        imgData.lookup[3*i+2] = colToByte(rgb.r);
      }
      break;
#if SPLASH_CMYK
    case splashModeCMYK8:
      imgData.lookup = (SplashColorPtr)gmalloc(4 * n);
      for (i = 0; i < n; ++i) {
        pix = (Guchar)i;
        colorMap->getCMYK(&pix, &cmyk);
        imgData.lookup[4*i] = colToByte(cmyk.c);
        imgData.lookup[4*i+1] = colToByte(cmyk.m);
        imgData.lookup[4*i+2] = colToByte(cmyk.y);
        imgData.lookup[4*i+3] = colToByte(cmyk.k);
      }
      break;
#endif
    default:
      //~ unimplemented
      break;
    }
  }

  switch (colorMode) {
  case splashModeMono1:
  case splashModeMono8:
    srcMode = splashModeMono8;
    break;
  case splashModeRGB8:
  case splashModeRGB8Qt:
    srcMode = splashModeRGB8;
    break;
  case splashModeBGR8:
    srcMode = splashModeBGR8;
    break;
#if SPLASH_CMYK
  case splashModeCMYK8:
    srcMode = splashModeCMYK8;
    break;
#endif
  default:
    //~ unimplemented
    srcMode = splashModeRGB8;
    break;
  }  
  splash->drawImage(&imageSrc, &imgData, srcMode, width, height, mat);

  splash->setSoftMask(NULL);
  gfree(imgData.lookup);
  delete imgData.imgStr;
  str->close();
}

void SplashOutputDev::setPaperColor(SplashColorPtr paperColorA) {
  splashColorCopy(paperColor, paperColorA);
}

int SplashOutputDev::getBitmapWidth() {
  return bitmap->getWidth();
}

int SplashOutputDev::getBitmapHeight() {
  return bitmap->getHeight();
}

SplashBitmap *SplashOutputDev::takeBitmap() {
  SplashBitmap *ret;

  ret = bitmap;
  bitmap = new SplashBitmap(1, 1, bitmapRowPad, colorMode, bitmapTopDown);
  return ret;
}

void SplashOutputDev::getModRegion(int *xMin, int *yMin,
                                   int *xMax, int *yMax) {
  splash->getModRegion(xMin, yMin, xMax, yMax);
}

void SplashOutputDev::clearModRegion() {
  splash->clearModRegion();
}

void SplashOutputDev::setFillColor(int r, int g, int b) {
  GfxRGB rgb;
  GfxGray gray;
#if SPLASH_CMYK
  GfxCMYK cmyk;
#endif

  rgb.r = byteToCol(r);
  rgb.g = byteToCol(g);
  rgb.b = byteToCol(b);
  gray = (GfxColorComp)(0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b + 0.5);
  if (gray > gfxColorComp1) {
    gray = gfxColorComp1;
  }
#if SPLASH_CMYK
  cmyk.c = gfxColorComp1 - rgb.r;
  cmyk.m = gfxColorComp1 - rgb.g;
  cmyk.y = gfxColorComp1 - rgb.b;
  cmyk.k = 0;
  splash->setFillPattern(getColor(gray, &rgb, &cmyk));
#else
  splash->setFillPattern(getColor(gray, &rgb));
#endif
}

