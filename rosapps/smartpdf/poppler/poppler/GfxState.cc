//========================================================================
//
// GfxState.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stddef.h>
#include <math.h>
#include <string.h>
#include "goo/gmem.h"
#include "Error.h"
#include "Object.h"
#include "Array.h"
#include "Page.h"
#include "GfxState.h"
#include "GfxFont.h"
#include "UGooString.h"

//------------------------------------------------------------------------

static inline GfxColorComp clip01(GfxColorComp x) {
  return (x < 0) ? 0 : (x > gfxColorComp1) ? gfxColorComp1 : x;
}

static inline double clip01(double x) {
  return (x < 0) ? 0 : (x > 1) ? 1 : x;
}

GBool Matrix::invertTo(Matrix *other)
{
  double det;

  det = 1 / (m[0] * m[3] - m[1] * m[2]);
  other->m[0] = m[3] * det;
  other->m[1] = -m[1] * det;
  other->m[2] = -m[2] * det;
  other->m[3] = m[0] * det;
  other->m[4] = (m[2] * m[5] - m[3] * m[4]) * det;
  other->m[5] = (m[1] * m[4] - m[0] * m[5]) * det;

  return gTrue;
}

void Matrix::transform(double x, double y, double *tx, double *ty)
{
  double temp_x, temp_y;

  temp_x = m[0] * x + m[2] * y + m[4];
  temp_y = m[1] * x + m[3] * y + m[5];

  *tx = temp_x;
  *ty = temp_y;
}

//------------------------------------------------------------------------

struct gfxBlendModeName {
  char *name;
  GfxBlendMode mode;
};

static gfxBlendModeName gfxBlendModeNames[] = {
  { "Normal",     gfxBlendNormal },
  { "Compatible", gfxBlendNormal },
  { "Multiply",   gfxBlendMultiply },
  { "Screen",     gfxBlendScreen },
  { "Overlay",    gfxBlendOverlay },
  { "Darken",     gfxBlendDarken },
  { "Lighten",    gfxBlendLighten },
  { "ColorDodge", gfxBlendColorDodge },
  { "ColorBurn",  gfxBlendColorBurn },
  { "HardLight",  gfxBlendHardLight },
  { "SoftLight",  gfxBlendSoftLight },
  { "Difference", gfxBlendDifference },
  { "Exclusion",  gfxBlendExclusion },
  { "Hue",        gfxBlendHue },
  { "Saturation", gfxBlendSaturation },
  { "Color",      gfxBlendColor },
  { "Luminosity", gfxBlendLuminosity }
};

#define nGfxBlendModeNames \
          ((int)((sizeof(gfxBlendModeNames) / sizeof(gfxBlendModeName))))
	 
//------------------------------------------------------------------------
// 
// NB: This must match the GfxColorSpaceMode enum defined in
// GfxState.h
static char *gfxColorSpaceModeNames[] = {
  "DeviceGray",
  "CalGray",
  "DeviceRGB",
  "CalRGB",
  "DeviceCMYK",
  "Lab",
  "ICCBased",
  "Indexed",
  "Separation",
  "DeviceN",
  "Pattern"
};

#define nGfxColorSpaceModes ((sizeof(gfxColorSpaceModeNames) / sizeof(char *)))

//------------------------------------------------------------------------
// GfxColorSpace
//------------------------------------------------------------------------

GfxColorSpace::GfxColorSpace() {
}

GfxColorSpace::~GfxColorSpace() {
}

GfxColorSpace *GfxColorSpace::parse(Object *csObj) {
  GfxColorSpace *cs;
  Object obj1;

  cs = NULL;
  if (csObj->isName()) {
    if (csObj->isName("DeviceGray") || csObj->isName("G")) {
      cs = new GfxDeviceGrayColorSpace();
    } else if (csObj->isName("DeviceRGB") || csObj->isName("RGB")) {
      cs = new GfxDeviceRGBColorSpace();
    } else if (csObj->isName("DeviceCMYK") || csObj->isName("CMYK")) {
      cs = new GfxDeviceCMYKColorSpace();
    } else if (csObj->isName("Pattern")) {
      cs = new GfxPatternColorSpace(NULL);
    } else {
      error(-1, "Bad color space '%s'", csObj->getNameC());
    }
  } else if (csObj->isArray()) {
    csObj->arrayGet(0, &obj1);
    if (obj1.isName("DeviceGray") || obj1.isName("G")) {
      cs = new GfxDeviceGrayColorSpace();
    } else if (obj1.isName("DeviceRGB") || obj1.isName("RGB")) {
      cs = new GfxDeviceRGBColorSpace();
    } else if (obj1.isName("DeviceCMYK") || obj1.isName("CMYK")) {
      cs = new GfxDeviceCMYKColorSpace();
    } else if (obj1.isName("CalGray")) {
      cs = GfxCalGrayColorSpace::parse(csObj->getArray());
    } else if (obj1.isName("CalRGB")) {
      cs = GfxCalRGBColorSpace::parse(csObj->getArray());
    } else if (obj1.isName("Lab")) {
      cs = GfxLabColorSpace::parse(csObj->getArray());
    } else if (obj1.isName("ICCBased")) {
      cs = GfxICCBasedColorSpace::parse(csObj->getArray());
    } else if (obj1.isName("Indexed") || obj1.isName("I")) {
      cs = GfxIndexedColorSpace::parse(csObj->getArray());
    } else if (obj1.isName("Separation")) {
      cs = GfxSeparationColorSpace::parse(csObj->getArray());
    } else if (obj1.isName("DeviceN")) {
      cs = GfxDeviceNColorSpace::parse(csObj->getArray());
    } else if (obj1.isName("Pattern")) {
      cs = GfxPatternColorSpace::parse(csObj->getArray());
    } else {
      error(-1, "Bad color space");
    }
    obj1.free();
  } else {
    error(-1, "Bad color space - expected name or array");
  }
  return cs;
}

void GfxColorSpace::getDefaultRanges(double *decodeLow, double *decodeRange,
				     int maxImgPixel) {
  int i;

  for (i = 0; i < getNComps(); ++i) {
    decodeLow[i] = 0;
    decodeRange[i] = 1;
  }
}

int GfxColorSpace::getNumColorSpaceModes() {
  return nGfxColorSpaceModes;
}

char *GfxColorSpace::getColorSpaceModeName(int idx) {
  return gfxColorSpaceModeNames[idx];
}

void GfxColorSpace::getRGBLine(Guchar *in, unsigned int *out, int length) {
  int i, j, n;
  GfxColor color;
  GfxRGB rgb;

  n = getNComps();
  for (i = 0; i < length; i++) {
    
    for (j = 0; j < n; j++)
      color.c[j] = in[i * n + j] * 256;

    getRGB (&color, &rgb);
    out[i] =
	((int) colToByte(rgb.r) << 16) |
	((int) colToByte(rgb.g) << 8) |
	((int) colToByte(rgb.b) << 0);
  }
}

void GfxColorSpace::getGrayLine(Guchar *in, unsigned char *out, int length) {
  int i, j, n;
  GfxColor color;
  GfxGray gray;

  n = getNComps();
  for (i = 0; i < length; i++) {
    
    for (j = 0; j < n; j++)
      color.c[j] = in[i * n + j] * 256;

    getGray (&color, &gray);
    out[i] = colToByte(gray);
  }
}


//------------------------------------------------------------------------
// GfxDeviceGrayColorSpace
//------------------------------------------------------------------------

GfxDeviceGrayColorSpace::GfxDeviceGrayColorSpace() {
}

GfxDeviceGrayColorSpace::~GfxDeviceGrayColorSpace() {
}

GfxColorSpace *GfxDeviceGrayColorSpace::copy() {
  return new GfxDeviceGrayColorSpace();
}

void GfxDeviceGrayColorSpace::getGray(GfxColor *color, GfxGray *gray) {
  *gray = clip01(color->c[0]);
}

void GfxDeviceGrayColorSpace::getGrayLine(Guchar *in, Guchar *out, int length) {
  memcpy (out, in, length);
}

void GfxDeviceGrayColorSpace::getRGB(GfxColor *color, GfxRGB *rgb) {
  rgb->r = rgb->g = rgb->b = clip01(color->c[0]);
}

void GfxDeviceGrayColorSpace::getRGBLine(Guchar *in, unsigned int *out,
					 int length) {
  int i;

  for (i = 0; i < length; i++)
    out[i] = (in[i] << 16) | (in[i] << 8) | (in[i] << 0);
}

void GfxDeviceGrayColorSpace::getCMYK(GfxColor *color, GfxCMYK *cmyk) {
  cmyk->c = cmyk->m = cmyk->y = 0;
  cmyk->k = clip01(gfxColorComp1 - color->c[0]);
}

//------------------------------------------------------------------------
// GfxCalGrayColorSpace
//------------------------------------------------------------------------

GfxCalGrayColorSpace::GfxCalGrayColorSpace() {
  whiteX = whiteY = whiteZ = 1;
  blackX = blackY = blackZ = 0;
  gamma = 1;
}

GfxCalGrayColorSpace::~GfxCalGrayColorSpace() {
}

GfxColorSpace *GfxCalGrayColorSpace::copy() {
  GfxCalGrayColorSpace *cs;

  cs = new GfxCalGrayColorSpace();
  cs->whiteX = whiteX;
  cs->whiteY = whiteY;
  cs->whiteZ = whiteZ;
  cs->blackX = blackX;
  cs->blackY = blackY;
  cs->blackZ = blackZ;
  cs->gamma = gamma;
  return cs;
}

GfxColorSpace *GfxCalGrayColorSpace::parse(Array *arr) {
  GfxCalGrayColorSpace *cs;
  Object obj1, obj2, obj3;

  arr->get(1, &obj1);
  if (!obj1.isDict()) {
    error(-1, "Bad CalGray color space");
    obj1.free();
    return NULL;
  }
  cs = new GfxCalGrayColorSpace();
  if (obj1.dictLookup("WhitePoint", &obj2)->isArray() &&
      obj2.arrayGetLength() == 3) {
    obj2.arrayGet(0, &obj3);
    cs->whiteX = obj3.getNum();
    obj3.free();
    obj2.arrayGet(1, &obj3);
    cs->whiteY = obj3.getNum();
    obj3.free();
    obj2.arrayGet(2, &obj3);
    cs->whiteZ = obj3.getNum();
    obj3.free();
  }
  obj2.free();
  if (obj1.dictLookup("BlackPoint", &obj2)->isArray() &&
      obj2.arrayGetLength() == 3) {
    obj2.arrayGet(0, &obj3);
    cs->blackX = obj3.getNum();
    obj3.free();
    obj2.arrayGet(1, &obj3);
    cs->blackY = obj3.getNum();
    obj3.free();
    obj2.arrayGet(2, &obj3);
    cs->blackZ = obj3.getNum();
    obj3.free();
  }
  obj2.free();
  if (obj1.dictLookup("Gamma", &obj2)->isNum()) {
    cs->gamma = obj2.getNum();
  }
  obj2.free();
  obj1.free();
  return cs;
}

void GfxCalGrayColorSpace::getGray(GfxColor *color, GfxGray *gray) {
  *gray = clip01(color->c[0]);
}

void GfxCalGrayColorSpace::getGrayLine(Guchar *in, Guchar *out, int length) {
  memcpy (out, in, length);
}

void GfxCalGrayColorSpace::getRGB(GfxColor *color, GfxRGB *rgb) {
  rgb->r = rgb->g = rgb->b = clip01(color->c[0]);
}

void GfxCalGrayColorSpace::getRGBLine(Guchar *in, unsigned int *out,
				      int length) {
  int i;

  for (i = 0; i < length; i++)
    out[i] = (in[i] << 16) | (in[i] << 8) | (in[i] << 0);
}

void GfxCalGrayColorSpace::getCMYK(GfxColor *color, GfxCMYK *cmyk) {
  cmyk->c = cmyk->m = cmyk->y = 0;
  cmyk->k = clip01(gfxColorComp1 - color->c[0]);
}

//------------------------------------------------------------------------
// GfxDeviceRGBColorSpace
//------------------------------------------------------------------------

GfxDeviceRGBColorSpace::GfxDeviceRGBColorSpace() {
}

GfxDeviceRGBColorSpace::~GfxDeviceRGBColorSpace() {
}

GfxColorSpace *GfxDeviceRGBColorSpace::copy() {
  return new GfxDeviceRGBColorSpace();
}

void GfxDeviceRGBColorSpace::getGray(GfxColor *color, GfxGray *gray) {
  *gray = clip01((GfxColorComp)(0.3 * color->c[0] +
		 0.59 * color->c[1] +
		 0.11 * color->c[2] + 0.5));
}

void GfxDeviceRGBColorSpace::getGrayLine(Guchar *in, Guchar *out, int length) {
  int i;

  for (i = 0; i < length; i++) {
    out[i] = 
      (in[i * 3 + 0] * 19595 + 
       in[i * 3 + 1] * 38469 + 
       in[i * 3 + 2] * 7472) / 65536;
  }
}

void GfxDeviceRGBColorSpace::getRGB(GfxColor *color, GfxRGB *rgb) {
  rgb->r = clip01(color->c[0]);
  rgb->g = clip01(color->c[1]);
  rgb->b = clip01(color->c[2]);
}

void GfxDeviceRGBColorSpace::getRGBLine(Guchar *in, unsigned int *out,
					int length) {
  Guchar *p;
  int i;

  for (i = 0, p = in; i < length; i++, p += 3)
    out[i] = (p[0] << 16) | (p[1] << 8) | (p[2] << 0);
}

void GfxDeviceRGBColorSpace::getCMYK(GfxColor *color, GfxCMYK *cmyk) {
  GfxColorComp c, m, y, k;

  c = clip01(gfxColorComp1 - color->c[0]);
  m = clip01(gfxColorComp1 - color->c[1]);
  y = clip01(gfxColorComp1 - color->c[2]);
  k = c;
  if (m < k) {
    k = m;
  }
  if (y < k) {
    k = y;
  }
  cmyk->c = c - k;
  cmyk->m = m - k;
  cmyk->y = y - k;
  cmyk->k = k;
}

//------------------------------------------------------------------------
// GfxCalRGBColorSpace
//------------------------------------------------------------------------

GfxCalRGBColorSpace::GfxCalRGBColorSpace() {
  whiteX = whiteY = whiteZ = 1;
  blackX = blackY = blackZ = 0;
  gammaR = gammaG = gammaB = 1;
  mat[0] = 1; mat[1] = 0; mat[2] = 0;
  mat[3] = 0; mat[4] = 1; mat[5] = 0;
  mat[6] = 0; mat[7] = 0; mat[8] = 1;
}

GfxCalRGBColorSpace::~GfxCalRGBColorSpace() {
}

GfxColorSpace *GfxCalRGBColorSpace::copy() {
  GfxCalRGBColorSpace *cs;
  int i;

  cs = new GfxCalRGBColorSpace();
  cs->whiteX = whiteX;
  cs->whiteY = whiteY;
  cs->whiteZ = whiteZ;
  cs->blackX = blackX;
  cs->blackY = blackY;
  cs->blackZ = blackZ;
  cs->gammaR = gammaR;
  cs->gammaG = gammaG;
  cs->gammaB = gammaB;
  for (i = 0; i < 9; ++i) {
    cs->mat[i] = mat[i];
  }
  return cs;
}

GfxColorSpace *GfxCalRGBColorSpace::parse(Array *arr) {
  GfxCalRGBColorSpace *cs;
  Object obj1, obj2, obj3;
  int i;

  arr->get(1, &obj1);
  if (!obj1.isDict()) {
    error(-1, "Bad CalRGB color space");
    obj1.free();
    return NULL;
  }
  cs = new GfxCalRGBColorSpace();
  if (obj1.dictLookup("WhitePoint", &obj2)->isArray() &&
      obj2.arrayGetLength() == 3) {
    obj2.arrayGet(0, &obj3);
    cs->whiteX = obj3.getNum();
    obj3.free();
    obj2.arrayGet(1, &obj3);
    cs->whiteY = obj3.getNum();
    obj3.free();
    obj2.arrayGet(2, &obj3);
    cs->whiteZ = obj3.getNum();
    obj3.free();
  }
  obj2.free();
  if (obj1.dictLookup("BlackPoint", &obj2)->isArray() &&
      obj2.arrayGetLength() == 3) {
    obj2.arrayGet(0, &obj3);
    cs->blackX = obj3.getNum();
    obj3.free();
    obj2.arrayGet(1, &obj3);
    cs->blackY = obj3.getNum();
    obj3.free();
    obj2.arrayGet(2, &obj3);
    cs->blackZ = obj3.getNum();
    obj3.free();
  }
  obj2.free();
  if (obj1.dictLookup("Gamma", &obj2)->isArray() &&
      obj2.arrayGetLength() == 3) {
    obj2.arrayGet(0, &obj3);
    cs->gammaR = obj3.getNum();
    obj3.free();
    obj2.arrayGet(1, &obj3);
    cs->gammaG = obj3.getNum();
    obj3.free();
    obj2.arrayGet(2, &obj3);
    cs->gammaB = obj3.getNum();
    obj3.free();
  }
  obj2.free();
  if (obj1.dictLookup("Matrix", &obj2)->isArray() &&
      obj2.arrayGetLength() == 9) {
    for (i = 0; i < 9; ++i) {
      obj2.arrayGet(i, &obj3);
      cs->mat[i] = obj3.getNum();
      obj3.free();
    }
  }
  obj2.free();
  obj1.free();
  return cs;
}

void GfxCalRGBColorSpace::getGray(GfxColor *color, GfxGray *gray) {
  *gray = clip01((GfxColorComp)(0.299 * color->c[0] +
		 0.587 * color->c[1] +
		 0.114 * color->c[2] + 0.5));
}

void GfxCalRGBColorSpace::getGrayLine(Guchar *in, Guchar *out, int length) {
  int i;

  for (i = 0; i < length; i++) {
    out[i] = 
      (in[i * 3 + 0] * 19595 + 
       in[i * 3 + 1] * 38469 + 
       in[i * 3 + 2] * 7472) / 65536;
  }
}

void GfxCalRGBColorSpace::getRGB(GfxColor *color, GfxRGB *rgb) {
  rgb->r = clip01(color->c[0]);
  rgb->g = clip01(color->c[1]);
  rgb->b = clip01(color->c[2]);
}

void GfxCalRGBColorSpace::getRGBLine(Guchar *in, unsigned int *out,
				     int length) {
  Guchar *p;
  int i;

  for (i = 0, p = in; i < length; i++, p += 3)
    out[i] = (p[0] << 16) | (p[1] << 8) | (p[2] << 0);
}

void GfxCalRGBColorSpace::getCMYK(GfxColor *color, GfxCMYK *cmyk) {
  GfxColorComp c, m, y, k;

  c = clip01(gfxColorComp1 - color->c[0]);
  m = clip01(gfxColorComp1 - color->c[1]);
  y = clip01(gfxColorComp1 - color->c[2]);
  k = c;
  if (m < k) {
    k = m;
  }
  if (y < k) {
    k = y;
  }
  cmyk->c = c - k;
  cmyk->m = m - k;
  cmyk->y = y - k;
  cmyk->k = k;
}

//------------------------------------------------------------------------
// GfxDeviceCMYKColorSpace
//------------------------------------------------------------------------

GfxDeviceCMYKColorSpace::GfxDeviceCMYKColorSpace() {
}

GfxDeviceCMYKColorSpace::~GfxDeviceCMYKColorSpace() {
}

GfxColorSpace *GfxDeviceCMYKColorSpace::copy() {
  return new GfxDeviceCMYKColorSpace();
}

void GfxDeviceCMYKColorSpace::getGray(GfxColor *color, GfxGray *gray) {
  *gray = clip01((GfxColorComp)(gfxColorComp1 - color->c[3]
				- 0.3  * color->c[0]
				- 0.59 * color->c[1]
				- 0.11 * color->c[2] + 0.5));
}

void GfxDeviceCMYKColorSpace::getRGB(GfxColor *color, GfxRGB *rgb) {
  double c, m, y, k, c1, m1, y1, k1, r, g, b, x;
    
  c = colToDbl(color->c[0]);
  m = colToDbl(color->c[1]);
  y = colToDbl(color->c[2]);
  k = colToDbl(color->c[3]);
  c1 = 1 - c;
  m1 = 1 - m;
  y1 = 1 - y;
  k1 = 1 - k;
  // this is a matrix multiplication, unrolled for performance
  //                        C M Y K
  x = c1 * m1 * y1 * k1; // 0 0 0 0
  r = g = b = x;
  x = c1 * m1 * y1 * k;  // 0 0 0 1
  r += 0.1373 * x;
  g += 0.1216 * x;
  b += 0.1255 * x;
  x = c1 * m1 * y  * k1; // 0 0 1 0
  r += x;
  g += 0.9490 * x;
  x = c1 * m1 * y  * k;  // 0 0 1 1
  r += 0.1098 * x;
  g += 0.1020 * x;
  x = c1 * m  * y1 * k1; // 0 1 0 0
  r += 0.9255 * x;
  b += 0.5490 * x;
  x = c1 * m  * y1 * k;  // 0 1 0 1
  r += 0.1412 * x;
  x = c1 * m  * y  * k1; // 0 1 1 0
  r += 0.9294 * x;
  g += 0.1098 * x;
  b += 0.1412 * x;
  x = c1 * m  * y  * k;  // 0 1 1 1
  r += 0.1333 * x;
  x = c  * m1 * y1 * k1; // 1 0 0 0
  g += 0.6784 * x;
  b += 0.9373 * x;
  x = c  * m1 * y1 * k;  // 1 0 0 1
  g += 0.0588 * x;
  b += 0.1412 * x;
  x = c  * m1 * y  * k1; // 1 0 1 0
  g += 0.6510 * x;
  b += 0.3137 * x;
  x = c  * m1 * y  * k;  // 1 0 1 1
  g += 0.0745 * x;
  x = c  * m  * y1 * k1; // 1 1 0 0
  r += 0.1804 * x;
  g += 0.1922 * x;
  b += 0.5725 * x;
  x = c  * m  * y1 * k;  // 1 1 0 1
  b += 0.0078 * x;
  x = c  * m  * y  * k1; // 1 1 1 0
  r += 0.2118 * x;
  g += 0.2119 * x;
  b += 0.2235 * x;
  rgb->r = clip01(dblToCol(r));
  rgb->g = clip01(dblToCol(g));
  rgb->b = clip01(dblToCol(b));
}

void GfxDeviceCMYKColorSpace::getCMYK(GfxColor *color, GfxCMYK *cmyk) {
  cmyk->c = clip01(color->c[0]);
  cmyk->m = clip01(color->c[1]);
  cmyk->y = clip01(color->c[2]);
  cmyk->k = clip01(color->c[3]);
}

//------------------------------------------------------------------------
// GfxLabColorSpace
//------------------------------------------------------------------------

// This is the inverse of MatrixLMN in Example 4.10 from the PostScript
// Language Reference, Third Edition.
static double xyzrgb[3][3] = {
  {  3.240449, -1.537136, -0.498531 },
  { -0.969265,  1.876011,  0.041556 },
  {  0.055643, -0.204026,  1.057229 }
};

GfxLabColorSpace::GfxLabColorSpace() {
  whiteX = whiteY = whiteZ = 1;
  blackX = blackY = blackZ = 0;
  aMin = bMin = -100;
  aMax = bMax = 100;
}

GfxLabColorSpace::~GfxLabColorSpace() {
}

GfxColorSpace *GfxLabColorSpace::copy() {
  GfxLabColorSpace *cs;

  cs = new GfxLabColorSpace();
  cs->whiteX = whiteX;
  cs->whiteY = whiteY;
  cs->whiteZ = whiteZ;
  cs->blackX = blackX;
  cs->blackY = blackY;
  cs->blackZ = blackZ;
  cs->aMin = aMin;
  cs->aMax = aMax;
  cs->bMin = bMin;
  cs->bMax = bMax;
  cs->kr = kr;
  cs->kg = kg;
  cs->kb = kb;
  return cs;
}

GfxColorSpace *GfxLabColorSpace::parse(Array *arr) {
  GfxLabColorSpace *cs;
  Object obj1, obj2, obj3;

  arr->get(1, &obj1);
  if (!obj1.isDict()) {
    error(-1, "Bad Lab color space");
    obj1.free();
    return NULL;
  }
  cs = new GfxLabColorSpace();
  if (obj1.dictLookup("WhitePoint", &obj2)->isArray() &&
      obj2.arrayGetLength() == 3) {
    obj2.arrayGet(0, &obj3);
    cs->whiteX = obj3.getNum();
    obj3.free();
    obj2.arrayGet(1, &obj3);
    cs->whiteY = obj3.getNum();
    obj3.free();
    obj2.arrayGet(2, &obj3);
    cs->whiteZ = obj3.getNum();
    obj3.free();
  }
  obj2.free();
  if (obj1.dictLookup("BlackPoint", &obj2)->isArray() &&
      obj2.arrayGetLength() == 3) {
    obj2.arrayGet(0, &obj3);
    cs->blackX = obj3.getNum();
    obj3.free();
    obj2.arrayGet(1, &obj3);
    cs->blackY = obj3.getNum();
    obj3.free();
    obj2.arrayGet(2, &obj3);
    cs->blackZ = obj3.getNum();
    obj3.free();
  }
  obj2.free();
  if (obj1.dictLookup("Range", &obj2)->isArray() &&
      obj2.arrayGetLength() == 4) {
    obj2.arrayGet(0, &obj3);
    cs->aMin = obj3.getNum();
    obj3.free();
    obj2.arrayGet(1, &obj3);
    cs->aMax = obj3.getNum();
    obj3.free();
    obj2.arrayGet(2, &obj3);
    cs->bMin = obj3.getNum();
    obj3.free();
    obj2.arrayGet(3, &obj3);
    cs->bMax = obj3.getNum();
    obj3.free();
  }
  obj2.free();
  obj1.free();

  cs->kr = 1 / (xyzrgb[0][0] * cs->whiteX +
		xyzrgb[0][1] * cs->whiteY +
		xyzrgb[0][2] * cs->whiteZ);
  cs->kg = 1 / (xyzrgb[1][0] * cs->whiteX +
		xyzrgb[1][1] * cs->whiteY +
		xyzrgb[1][2] * cs->whiteZ);
  cs->kb = 1 / (xyzrgb[2][0] * cs->whiteX +
		xyzrgb[2][1] * cs->whiteY +
		xyzrgb[2][2] * cs->whiteZ);

  return cs;
}

void GfxLabColorSpace::getGray(GfxColor *color, GfxGray *gray) {
  GfxRGB rgb;

  getRGB(color, &rgb);
  *gray = clip01((GfxColorComp)(0.299 * rgb.r +
		 0.587 * rgb.g +
				0.114 * rgb.b + 0.5));
}

void GfxLabColorSpace::getRGB(GfxColor *color, GfxRGB *rgb) {
  double X, Y, Z;
  double t1, t2;
  double r, g, b;

  // convert L*a*b* to CIE 1931 XYZ color space
  t1 = (colToDbl(color->c[0]) + 16) / 116;
  t2 = t1 + colToDbl(color->c[1]) / 500;
  if (t2 >= (6.0 / 29.0)) {
    X = t2 * t2 * t2;
  } else {
    X = (108.0 / 841.0) * (t2 - (4.0 / 29.0));
  }
  X *= whiteX;
  if (t1 >= (6.0 / 29.0)) {
    Y = t1 * t1 * t1;
  } else {
    Y = (108.0 / 841.0) * (t1 - (4.0 / 29.0));
  }
  Y *= whiteY;
  t2 = t1 - colToDbl(color->c[2]) / 200;
  if (t2 >= (6.0 / 29.0)) {
    Z = t2 * t2 * t2;
  } else {
    Z = (108.0 / 841.0) * (t2 - (4.0 / 29.0));
  }
  Z *= whiteZ;

  // convert XYZ to RGB, including gamut mapping and gamma correction
  r = xyzrgb[0][0] * X + xyzrgb[0][1] * Y + xyzrgb[0][2] * Z;
  g = xyzrgb[1][0] * X + xyzrgb[1][1] * Y + xyzrgb[1][2] * Z;
  b = xyzrgb[2][0] * X + xyzrgb[2][1] * Y + xyzrgb[2][2] * Z;
  rgb->r = dblToCol(pow(clip01(r * kr), 0.5));
  rgb->g = dblToCol(pow(clip01(g * kg), 0.5));
  rgb->b = dblToCol(pow(clip01(b * kb), 0.5));
}

void GfxLabColorSpace::getCMYK(GfxColor *color, GfxCMYK *cmyk) {
  GfxRGB rgb;
  GfxColorComp c, m, y, k;

  getRGB(color, &rgb);
  c = clip01(gfxColorComp1 - rgb.r);
  m = clip01(gfxColorComp1 - rgb.g);
  y = clip01(gfxColorComp1 - rgb.b);
  k = c;
  if (m < k) {
    k = m;
  }
  if (y < k) {
    k = y;
  }
  cmyk->c = c - k;
  cmyk->m = m - k;
  cmyk->y = y - k;
  cmyk->k = k;
}

void GfxLabColorSpace::getDefaultRanges(double *decodeLow, double *decodeRange,
					int maxImgPixel) {
  decodeLow[0] = 0;
  decodeRange[0] = 100;
  decodeLow[1] = aMin;
  decodeRange[1] = aMax - aMin;
  decodeLow[2] = bMin;
  decodeRange[2] = bMax - bMin;
}

//------------------------------------------------------------------------
// GfxICCBasedColorSpace
//------------------------------------------------------------------------

GfxICCBasedColorSpace::GfxICCBasedColorSpace(int nCompsA, GfxColorSpace *altA,
					     Ref *iccProfileStreamA) {
  nComps = nCompsA;
  alt = altA;
  iccProfileStream = *iccProfileStreamA;
  rangeMin[0] = rangeMin[1] = rangeMin[2] = rangeMin[3] = 0;
  rangeMax[0] = rangeMax[1] = rangeMax[2] = rangeMax[3] = 1;
}

GfxICCBasedColorSpace::~GfxICCBasedColorSpace() {
  delete alt;
}

GfxColorSpace *GfxICCBasedColorSpace::copy() {
  GfxICCBasedColorSpace *cs;
  int i;

  cs = new GfxICCBasedColorSpace(nComps, alt->copy(), &iccProfileStream);
  for (i = 0; i < 4; ++i) {
    cs->rangeMin[i] = rangeMin[i];
    cs->rangeMax[i] = rangeMax[i];
  }
  return cs;
}

GfxColorSpace *GfxICCBasedColorSpace::parse(Array *arr) {
  GfxICCBasedColorSpace *cs;
  Ref iccProfileStreamA;
  int nCompsA;
  GfxColorSpace *altA;
  Dict *dict;
  Object obj1, obj2, obj3;
  int i;

  arr->getNF(1, &obj1);
  if (obj1.isRef()) {
    iccProfileStreamA = obj1.getRef();
  } else {
    iccProfileStreamA.num = 0;
    iccProfileStreamA.gen = 0;
  }
  obj1.free();
  arr->get(1, &obj1);
  if (!obj1.isStream()) {
    error(-1, "Bad ICCBased color space (stream)");
    obj1.free();
    return NULL;
  }
  dict = obj1.streamGetDict();
  if (!dict->lookup("N", &obj2)->isInt()) {
    error(-1, "Bad ICCBased color space (N)");
    obj2.free();
    obj1.free();
    return NULL;
  }
  nCompsA = obj2.getInt();
  obj2.free();
  if (nCompsA > gfxColorMaxComps) {
    error(-1, "ICCBased color space with too many (%d > %d) components",
	  nCompsA, gfxColorMaxComps);
    nCompsA = gfxColorMaxComps;
  }
  if (dict->lookup("Alternate", &obj2)->isNull() ||
      !(altA = GfxColorSpace::parse(&obj2))) {
    switch (nCompsA) {
    case 1:
      altA = new GfxDeviceGrayColorSpace();
      break;
    case 3:
      altA = new GfxDeviceRGBColorSpace();
      break;
    case 4:
      altA = new GfxDeviceCMYKColorSpace();
      break;
    default:
      error(-1, "Bad ICCBased color space - invalid N");
      obj2.free();
      obj1.free();
      return NULL;
    }
  }
  obj2.free();
  cs = new GfxICCBasedColorSpace(nCompsA, altA, &iccProfileStreamA);
  if (dict->lookup("Range", &obj2)->isArray() &&
      obj2.arrayGetLength() == 2 * nCompsA) {
    for (i = 0; i < nCompsA; ++i) {
      obj2.arrayGet(2*i, &obj3);
      cs->rangeMin[i] = obj3.getNum();
      obj3.free();
      obj2.arrayGet(2*i+1, &obj3);
      cs->rangeMax[i] = obj3.getNum();
      obj3.free();
    }
  }
  obj2.free();
  obj1.free();
  return cs;
}

void GfxICCBasedColorSpace::getGray(GfxColor *color, GfxGray *gray) {
  alt->getGray(color, gray);
}

void GfxICCBasedColorSpace::getRGB(GfxColor *color, GfxRGB *rgb) {
  alt->getRGB(color, rgb);
}

void GfxICCBasedColorSpace::getRGBLine(Guchar *in, unsigned int *out,
				       int length) {
  alt->getRGBLine(in, out, length);
}

void GfxICCBasedColorSpace::getCMYK(GfxColor *color, GfxCMYK *cmyk) {
  alt->getCMYK(color, cmyk);
}

void GfxICCBasedColorSpace::getDefaultRanges(double *decodeLow,
					     double *decodeRange,
					     int maxImgPixel) {
  alt->getDefaultRanges(decodeLow, decodeRange, maxImgPixel);

#if 0
  // this is nominally correct, but some PDF files don't set the
  // correct ranges in the ICCBased dict
  int i;

  for (i = 0; i < nComps; ++i) {
    decodeLow[i] = rangeMin[i];
    decodeRange[i] = rangeMax[i] - rangeMin[i];
  }
#endif
}

//------------------------------------------------------------------------
// GfxIndexedColorSpace
//------------------------------------------------------------------------

GfxIndexedColorSpace::GfxIndexedColorSpace(GfxColorSpace *baseA,
					   int indexHighA) {
  base = baseA;
  indexHigh = indexHighA;
  lookup = (Guchar *)gmallocn((indexHigh + 1) * base->getNComps(),
			      sizeof(Guchar));
}

GfxIndexedColorSpace::~GfxIndexedColorSpace() {
  delete base;
  gfree(lookup);
}

GfxColorSpace *GfxIndexedColorSpace::copy() {
  GfxIndexedColorSpace *cs;

  cs = new GfxIndexedColorSpace(base->copy(), indexHigh);
  memcpy(cs->lookup, lookup,
	 (indexHigh + 1) * base->getNComps() * sizeof(Guchar));
  return cs;
}

GfxColorSpace *GfxIndexedColorSpace::parse(Array *arr) {
  GfxIndexedColorSpace *cs;
  GfxColorSpace *baseA;
  int indexHighA;
  Object obj1;
  int x;
  char *s;
  int n, i, j;

  if (arr->getLength() != 4) {
    error(-1, "Bad Indexed color space");
    goto err1;
  }
  arr->get(1, &obj1);
  if (!(baseA = GfxColorSpace::parse(&obj1))) {
    error(-1, "Bad Indexed color space (base color space)");
    goto err2;
  }
  obj1.free();
  if (!arr->get(2, &obj1)->isInt()) {
    error(-1, "Bad Indexed color space (hival)");
    delete baseA;
    goto err2;
  }
  indexHighA = obj1.getInt();
  if (indexHighA < 0 || indexHighA > 255) {
    // the PDF spec requires indexHigh to be in [0,255] -- allowing
    // values larger than 255 creates a security hole: if nComps *
    // indexHigh is greater than 2^31, the loop below may overwrite
    // past the end of the array
    error(-1, "Bad Indexed color space (invalid indexHigh value)");
    delete baseA;
    goto err2;
  }
  obj1.free();
  cs = new GfxIndexedColorSpace(baseA, indexHighA);
  arr->get(3, &obj1);
  n = baseA->getNComps();
  if (obj1.isStream()) {
    obj1.streamReset();
    for (i = 0; i <= indexHighA; ++i) {
      for (j = 0; j < n; ++j) {
	if ((x = obj1.streamGetChar()) == EOF) {
	  error(-1, "Bad Indexed color space (lookup table stream too short)");
	  goto err3;
	}
	cs->lookup[i*n + j] = (Guchar)x;
      }
    }
    obj1.streamClose();
  } else if (obj1.isString()) {
    if (obj1.getString()->getLength() < (indexHighA + 1) * n) {
      error(-1, "Bad Indexed color space (lookup table string too short)");
      goto err3;
    }
    s = obj1.getString()->getCString();
    for (i = 0; i <= indexHighA; ++i) {
      for (j = 0; j < n; ++j) {
	cs->lookup[i*n + j] = (Guchar)*s++;
      }
    }
  } else {
    error(-1, "Bad Indexed color space (lookup table)");
    goto err3;
  }
  obj1.free();
  return cs;

 err3:
  delete cs;
 err2:
  obj1.free();
 err1:
  return NULL;
}

GfxColor *GfxIndexedColorSpace::mapColorToBase(GfxColor *color,
					       GfxColor *baseColor) {
  Guchar *p;
  double low[gfxColorMaxComps], range[gfxColorMaxComps];
  int n, i;

  n = base->getNComps();
  base->getDefaultRanges(low, range, indexHigh);
  p = &lookup[(int)(colToDbl(color->c[0]) + 0.5) * n];
  for (i = 0; i < n; ++i) {
    baseColor->c[i] = dblToCol(low[i] + (p[i] / 255.0) * range[i]);
  }
  return baseColor;
}

void GfxIndexedColorSpace::getGray(GfxColor *color, GfxGray *gray) {
  GfxColor color2;

  base->getGray(mapColorToBase(color, &color2), gray);
}

void GfxIndexedColorSpace::getRGB(GfxColor *color, GfxRGB *rgb) {
  GfxColor color2;

  base->getRGB(mapColorToBase(color, &color2), rgb);
}

void GfxIndexedColorSpace::getRGBLine(Guchar *in, unsigned int *out, int length) {
  Guchar *line;
  int i, j, n;

  n = base->getNComps();
  line = (Guchar *) gmalloc (length * n);
  for (i = 0; i < length; i++)
    for (j = 0; j < n; j++)
      line[i * n + j] = lookup[in[i] * n + j];

  base->getRGBLine(line, out, length);

  gfree (line);
}

void GfxIndexedColorSpace::getCMYK(GfxColor *color, GfxCMYK *cmyk) {
  GfxColor color2;

  base->getCMYK(mapColorToBase(color, &color2), cmyk);
}

void GfxIndexedColorSpace::getDefaultRanges(double *decodeLow,
					    double *decodeRange,
					    int maxImgPixel) {
  decodeLow[0] = 0;
  decodeRange[0] = maxImgPixel;
}

//------------------------------------------------------------------------
// GfxSeparationColorSpace
//------------------------------------------------------------------------

GfxSeparationColorSpace::GfxSeparationColorSpace(GooString *nameA,
						 GfxColorSpace *altA,
						 Function *funcA) {
  name = nameA;
  alt = altA;
  func = funcA;
}

GfxSeparationColorSpace::~GfxSeparationColorSpace() {
  delete name;
  delete alt;
  delete func;
}

GfxColorSpace *GfxSeparationColorSpace::copy() {
  return new GfxSeparationColorSpace(name->copy(), alt->copy(), func->copy());
}

//~ handle the 'All' and 'None' colorants
GfxColorSpace *GfxSeparationColorSpace::parse(Array *arr) {
  GfxSeparationColorSpace *cs;
  GooString *nameA;
  GfxColorSpace *altA;
  Function *funcA;
  Object obj1;

  if (arr->getLength() != 4) {
    error(-1, "Bad Separation color space");
    goto err1;
  }
  if (!arr->get(1, &obj1)->isName()) {
    error(-1, "Bad Separation color space (name)");
    goto err2;
  }
  nameA = new GooString(obj1.getName());
  obj1.free();
  arr->get(2, &obj1);
  if (!(altA = GfxColorSpace::parse(&obj1))) {
    error(-1, "Bad Separation color space (alternate color space)");
    goto err3;
  }
  obj1.free();
  arr->get(3, &obj1);
  if (!(funcA = Function::parse(&obj1))) {
    goto err4;
  }
  obj1.free();
  cs = new GfxSeparationColorSpace(nameA, altA, funcA);
  return cs;

 err4:
  delete altA;
 err3:
  delete nameA;
 err2:
  obj1.free();
 err1:
  return NULL;
}

void GfxSeparationColorSpace::getGray(GfxColor *color, GfxGray *gray) {
  double x;
  double c[gfxColorMaxComps];
  GfxColor color2;
  int i;

  x = colToDbl(color->c[0]);
  func->transform(&x, c);
  for (i = 0; i < alt->getNComps(); ++i) {
    color2.c[i] = dblToCol(c[i]);
  }
  alt->getGray(&color2, gray);
}

void GfxSeparationColorSpace::getRGB(GfxColor *color, GfxRGB *rgb) {
  double x;
  double c[gfxColorMaxComps];
  GfxColor color2;
  int i;

  x = colToDbl(color->c[0]);
  func->transform(&x, c);
  for (i = 0; i < alt->getNComps(); ++i) {
    color2.c[i] = dblToCol(c[i]);
  }
  alt->getRGB(&color2, rgb);
}

void GfxSeparationColorSpace::getCMYK(GfxColor *color, GfxCMYK *cmyk) {
  double x;
  double c[gfxColorMaxComps];
  GfxColor color2;
  int i;

  x = colToDbl(color->c[0]);
  func->transform(&x, c);
  for (i = 0; i < alt->getNComps(); ++i) {
    color2.c[i] = dblToCol(c[i]);
  }
  alt->getCMYK(&color2, cmyk);
}

//------------------------------------------------------------------------
// GfxDeviceNColorSpace
//------------------------------------------------------------------------

GfxDeviceNColorSpace::GfxDeviceNColorSpace(int nCompsA,
					   GfxColorSpace *altA,
					   Function *funcA) {
  nComps = nCompsA;
  alt = altA;
  func = funcA;
}

GfxDeviceNColorSpace::~GfxDeviceNColorSpace() {
  int i;

  for (i = 0; i < nComps; ++i) {
    delete names[i];
  }
  delete alt;
  delete func;
}

GfxColorSpace *GfxDeviceNColorSpace::copy() {
  GfxDeviceNColorSpace *cs;
  int i;

  cs = new GfxDeviceNColorSpace(nComps, alt->copy(), func->copy());
  for (i = 0; i < nComps; ++i) {
    cs->names[i] = names[i]->copy();
  }
  return cs;
}

//~ handle the 'None' colorant
GfxColorSpace *GfxDeviceNColorSpace::parse(Array *arr) {
  GfxDeviceNColorSpace *cs;
  int nCompsA;
  GooString *namesA[gfxColorMaxComps];
  GfxColorSpace *altA;
  Function *funcA;
  Object obj1, obj2;
  int i;

  if (arr->getLength() != 4 && arr->getLength() != 5) {
    error(-1, "Bad DeviceN color space");
    goto err1;
  }
  if (!arr->get(1, &obj1)->isArray()) {
    error(-1, "Bad DeviceN color space (names)");
    goto err2;
  }
  nCompsA = obj1.arrayGetLength();
  if (nCompsA > gfxColorMaxComps) {
    error(-1, "DeviceN color space with too many (%d > %d) components",
	  nCompsA, gfxColorMaxComps);
    nCompsA = gfxColorMaxComps;
  }
  for (i = 0; i < nCompsA; ++i) {
    if (!obj1.arrayGet(i, &obj2)->isName()) {
      error(-1, "Bad DeviceN color space (names)");
      obj2.free();
      goto err2;
    }
    namesA[i] = new GooString(obj2.getName());
    obj2.free();
  }
  obj1.free();
  arr->get(2, &obj1);
  if (!(altA = GfxColorSpace::parse(&obj1))) {
    error(-1, "Bad DeviceN color space (alternate color space)");
    goto err3;
  }
  obj1.free();
  arr->get(3, &obj1);
  if (!(funcA = Function::parse(&obj1))) {
    goto err4;
  }
  obj1.free();
  cs = new GfxDeviceNColorSpace(nCompsA, altA, funcA);
  for (i = 0; i < nCompsA; ++i) {
    cs->names[i] = namesA[i];
  }
  return cs;

 err4:
  delete altA;
 err3:
  for (i = 0; i < nCompsA; ++i) {
    delete namesA[i];
  }
 err2:
  obj1.free();
 err1:
  return NULL;
}

void GfxDeviceNColorSpace::getGray(GfxColor *color, GfxGray *gray) {
  double x[gfxColorMaxComps], c[gfxColorMaxComps];
  GfxColor color2;
  int i;

  for (i = 0; i < nComps; ++i) {
    x[i] = colToDbl(color->c[i]);
  }
  func->transform(x, c);
  for (i = 0; i < alt->getNComps(); ++i) {
    color2.c[i] = dblToCol(c[i]);
  }
  alt->getGray(&color2, gray);
}

void GfxDeviceNColorSpace::getRGB(GfxColor *color, GfxRGB *rgb) {
  double x[gfxColorMaxComps], c[gfxColorMaxComps];
  GfxColor color2;
  int i;

  for (i = 0; i < nComps; ++i) {
    x[i] = colToDbl(color->c[i]);
  }
  func->transform(x, c);
  for (i = 0; i < alt->getNComps(); ++i) {
    color2.c[i] = dblToCol(c[i]);
  }
  alt->getRGB(&color2, rgb);
}

void GfxDeviceNColorSpace::getCMYK(GfxColor *color, GfxCMYK *cmyk) {
  double x[gfxColorMaxComps], c[gfxColorMaxComps];
  GfxColor color2;
  int i;

  for (i = 0; i < nComps; ++i) {
    x[i] = colToDbl(color->c[i]);
  }
  func->transform(x, c);
  for (i = 0; i < alt->getNComps(); ++i) {
    color2.c[i] = dblToCol(c[i]);
  }
  alt->getCMYK(&color2, cmyk);
}

//------------------------------------------------------------------------
// GfxPatternColorSpace
//------------------------------------------------------------------------

GfxPatternColorSpace::GfxPatternColorSpace(GfxColorSpace *underA) {
  under = underA;
}

GfxPatternColorSpace::~GfxPatternColorSpace() {
  if (under) {
    delete under;
  }
}

GfxColorSpace *GfxPatternColorSpace::copy() {
  return new GfxPatternColorSpace(under ? under->copy() :
				          (GfxColorSpace *)NULL);
}

GfxColorSpace *GfxPatternColorSpace::parse(Array *arr) {
  GfxPatternColorSpace *cs;
  GfxColorSpace *underA;
  Object obj1;

  if (arr->getLength() != 1 && arr->getLength() != 2) {
    error(-1, "Bad Pattern color space");
    return NULL;
  }
  underA = NULL;
  if (arr->getLength() == 2) {
    arr->get(1, &obj1);
    if (!(underA = GfxColorSpace::parse(&obj1))) {
      error(-1, "Bad Pattern color space (underlying color space)");
      obj1.free();
      return NULL;
    }
    obj1.free();
  }
  cs = new GfxPatternColorSpace(underA);
  return cs;
}

void GfxPatternColorSpace::getGray(GfxColor *color, GfxGray *gray) {
  *gray = 0;
}

void GfxPatternColorSpace::getRGB(GfxColor *color, GfxRGB *rgb) {
  rgb->r = rgb->g = rgb->b = 0;
}

void GfxPatternColorSpace::getCMYK(GfxColor *color, GfxCMYK *cmyk) {
  cmyk->c = cmyk->m = cmyk->y = 0;
  cmyk->k = 1;
}

//------------------------------------------------------------------------
// Pattern
//------------------------------------------------------------------------

GfxPattern::GfxPattern(int typeA) {
  type = typeA;
}

GfxPattern::~GfxPattern() {
}

GfxPattern *GfxPattern::parse(Object *obj) {
  GfxPattern *pattern;
  Object obj1;

  if (obj->isDict()) {
    obj->dictLookup("PatternType", &obj1);
  } else if (obj->isStream()) {
    obj->streamGetDict()->lookup("PatternType", &obj1);
  } else {
    return NULL;
  }
  pattern = NULL;
  if (obj1.isInt() && obj1.getInt() == 1) {
    pattern = GfxTilingPattern::parse(obj);
  } else if (obj1.isInt() && obj1.getInt() == 2) {
    pattern = GfxShadingPattern::parse(obj);
  }
  obj1.free();
  return pattern;
}

//------------------------------------------------------------------------
// GfxTilingPattern
//------------------------------------------------------------------------

GfxTilingPattern *GfxTilingPattern::parse(Object *patObj) {
  GfxTilingPattern *pat;
  Dict *dict;
  int paintTypeA, tilingTypeA;
  double bboxA[4], matrixA[6];
  double xStepA, yStepA;
  Object resDictA;
  Object obj1, obj2;
  int i;

  if (!patObj->isStream()) {
    return NULL;
  }
  dict = patObj->streamGetDict();

  if (dict->lookup("PaintType", &obj1)->isInt()) {
    paintTypeA = obj1.getInt();
  } else {
    paintTypeA = 1;
    error(-1, "Invalid or missing PaintType in pattern");
  }
  obj1.free();
  if (dict->lookup("TilingType", &obj1)->isInt()) {
    tilingTypeA = obj1.getInt();
  } else {
    tilingTypeA = 1;
    error(-1, "Invalid or missing TilingType in pattern");
  }
  obj1.free();
  bboxA[0] = bboxA[1] = 0;
  bboxA[2] = bboxA[3] = 1;
  if (dict->lookup("BBox", &obj1)->isArray() &&
      obj1.arrayGetLength() == 4) {
    for (i = 0; i < 4; ++i) {
      if (obj1.arrayGet(i, &obj2)->isNum()) {
	bboxA[i] = obj2.getNum();
      }
      obj2.free();
    }
  } else {
    error(-1, "Invalid or missing BBox in pattern");
  }
  obj1.free();
  if (dict->lookup("XStep", &obj1)->isNum()) {
    xStepA = obj1.getNum();
  } else {
    xStepA = 1;
    error(-1, "Invalid or missing XStep in pattern");
  }
  obj1.free();
  if (dict->lookup("YStep", &obj1)->isNum()) {
    yStepA = obj1.getNum();
  } else {
    yStepA = 1;
    error(-1, "Invalid or missing YStep in pattern");
  }
  obj1.free();
  if (!dict->lookup("Resources", &resDictA)->isDict()) {
    resDictA.free();
    resDictA.initNull();
    error(-1, "Invalid or missing Resources in pattern");
  }
  matrixA[0] = 1; matrixA[1] = 0;
  matrixA[2] = 0; matrixA[3] = 1;
  matrixA[4] = 0; matrixA[5] = 0;
  if (dict->lookup("Matrix", &obj1)->isArray() &&
      obj1.arrayGetLength() == 6) {
    for (i = 0; i < 6; ++i) {
      if (obj1.arrayGet(i, &obj2)->isNum()) {
	matrixA[i] = obj2.getNum();
      }
      obj2.free();
    }
  }
  obj1.free();

  pat = new GfxTilingPattern(paintTypeA, tilingTypeA, bboxA, xStepA, yStepA,
			     &resDictA, matrixA, patObj);
  resDictA.free();
  return pat;
}

GfxTilingPattern::GfxTilingPattern(int paintTypeA, int tilingTypeA,
				   double *bboxA, double xStepA, double yStepA,
				   Object *resDictA, double *matrixA,
				   Object *contentStreamA):
  GfxPattern(1)
{
  int i;

  paintType = paintTypeA;
  tilingType = tilingTypeA;
  for (i = 0; i < 4; ++i) {
    bbox[i] = bboxA[i];
  }
  xStep = xStepA;
  yStep = yStepA;
  resDictA->copy(&resDict);
  for (i = 0; i < 6; ++i) {
    matrix[i] = matrixA[i];
  }
  contentStreamA->copy(&contentStream);
}

GfxTilingPattern::~GfxTilingPattern() {
  resDict.free();
  contentStream.free();
}

GfxPattern *GfxTilingPattern::copy() {
  return new GfxTilingPattern(paintType, tilingType, bbox, xStep, yStep,
			      &resDict, matrix, &contentStream);
}

//------------------------------------------------------------------------
// GfxShadingPattern
//------------------------------------------------------------------------

GfxShadingPattern *GfxShadingPattern::parse(Object *patObj) {
  Dict *dict;
  GfxShading *shadingA;
  double matrixA[6];
  Object obj1, obj2;
  int i;

  if (!patObj->isDict()) {
    return NULL;
  }
  dict = patObj->getDict();

  dict->lookup("Shading", &obj1);
  shadingA = GfxShading::parse(&obj1);
  obj1.free();
  if (!shadingA) {
    return NULL;
  }

  matrixA[0] = 1; matrixA[1] = 0;
  matrixA[2] = 0; matrixA[3] = 1;
  matrixA[4] = 0; matrixA[5] = 0;
  if (dict->lookup("Matrix", &obj1)->isArray() &&
      obj1.arrayGetLength() == 6) {
    for (i = 0; i < 6; ++i) {
      if (obj1.arrayGet(i, &obj2)->isNum()) {
	matrixA[i] = obj2.getNum();
      }
      obj2.free();
    }
  }
  obj1.free();

  return new GfxShadingPattern(shadingA, matrixA);
}

GfxShadingPattern::GfxShadingPattern(GfxShading *shadingA, double *matrixA):
  GfxPattern(2)
{
  int i;

  shading = shadingA;
  for (i = 0; i < 6; ++i) {
    matrix[i] = matrixA[i];
  }
}

GfxShadingPattern::~GfxShadingPattern() {
  delete shading;
}

GfxPattern *GfxShadingPattern::copy() {
  return new GfxShadingPattern(shading->copy(), matrix);
}

//------------------------------------------------------------------------
// GfxShading
//------------------------------------------------------------------------

GfxShading::GfxShading(int typeA) {
  type = typeA;
  colorSpace = NULL;
}

GfxShading::GfxShading(GfxShading *shading) {
  int i;

  type = shading->type;
  colorSpace = shading->colorSpace->copy();
  for (i = 0; i < gfxColorMaxComps; ++i) {
    background.c[i] = shading->background.c[i];
  }
  hasBackground = shading->hasBackground;
  xMin = shading->xMin;
  yMin = shading->yMin;
  xMax = shading->xMax;
  yMax = shading->yMax;
  hasBBox = shading->hasBBox;
}

GfxShading::~GfxShading() {
  if (colorSpace) {
    delete colorSpace;
  }
}

GfxShading *GfxShading::parse(Object *obj) {
  GfxShading *shading;
  Dict *dict;
  int typeA;
  Object obj1;

  if (obj->isDict()) {
    dict = obj->getDict();
  } else if (obj->isStream()) {
    dict = obj->streamGetDict();
  } else {
    return NULL;
  }

  if (!dict->lookup("ShadingType", &obj1)->isInt()) {
    error(-1, "Invalid ShadingType in shading dictionary");
    obj1.free();
    return NULL;
  }
  typeA = obj1.getInt();
  obj1.free();

  switch (typeA) {
  case 1:
    shading = GfxFunctionShading::parse(dict);
    break;
  case 2:
    shading = GfxAxialShading::parse(dict);
    break;
  case 3:
    shading = GfxRadialShading::parse(dict);
    break;
  case 4:
    if (obj->isStream()) {
      shading = GfxGouraudTriangleShading::parse(4, dict, obj->getStream());
    } else {
      error(-1, "Invalid Type 4 shading object");
      goto err1;
    }
    break;
  case 5:
    if (obj->isStream()) {
      shading = GfxGouraudTriangleShading::parse(5, dict, obj->getStream());
    } else {
      error(-1, "Invalid Type 5 shading object");
      goto err1;
    }
    break;
  case 6:
    if (obj->isStream()) {
      shading = GfxPatchMeshShading::parse(6, dict, obj->getStream());
    } else {
      error(-1, "Invalid Type 6 shading object");
      goto err1;
    }
    break;
  case 7:
    if (obj->isStream()) {
      shading = GfxPatchMeshShading::parse(7, dict, obj->getStream());
    } else {
      error(-1, "Invalid Type 7 shading object");
      goto err1;
    }
    break;
  default:
    error(-1, "Unimplemented shading type %d", typeA);
    goto err1;
  }

  return shading;

 err1:
  return NULL;
}

GBool GfxShading::init(Dict *dict) {
  Object obj1, obj2;
  int i;

  dict->lookup("ColorSpace", &obj1);
  if (!(colorSpace = GfxColorSpace::parse(&obj1))) {
    error(-1, "Bad color space in shading dictionary");
    obj1.free();
    return gFalse;
  }
  obj1.free();

  for (i = 0; i < gfxColorMaxComps; ++i) {
    background.c[i] = 0;
  }
  hasBackground = gFalse;
  if (dict->lookup("Background", &obj1)->isArray()) {
    if (obj1.arrayGetLength() == colorSpace->getNComps()) {
      hasBackground = gTrue;
      for (i = 0; i < colorSpace->getNComps(); ++i) {
	background.c[i] = dblToCol(obj1.arrayGet(i, &obj2)->getNum());
	obj2.free();
      }
    } else {
      error(-1, "Bad Background in shading dictionary");
    }
  }
  obj1.free();

  xMin = yMin = xMax = yMax = 0;
  hasBBox = gFalse;
  if (dict->lookup("BBox", &obj1)->isArray()) {
    if (obj1.arrayGetLength() == 4) {
      hasBBox = gTrue;
      xMin = obj1.arrayGet(0, &obj2)->getNum();
      obj2.free();
      yMin = obj1.arrayGet(1, &obj2)->getNum();
      obj2.free();
      xMax = obj1.arrayGet(2, &obj2)->getNum();
      obj2.free();
      yMax = obj1.arrayGet(3, &obj2)->getNum();
      obj2.free();
    } else {
      error(-1, "Bad BBox in shading dictionary");
    }
  }
  obj1.free();

  return gTrue;
}

//------------------------------------------------------------------------
// GfxFunctionShading
//------------------------------------------------------------------------

GfxFunctionShading::GfxFunctionShading(double x0A, double y0A,
				       double x1A, double y1A,
				       double *matrixA,
				       Function **funcsA, int nFuncsA):
  GfxShading(1)
{
  int i;

  x0 = x0A;
  y0 = y0A;
  x1 = x1A;
  y1 = y1A;
  for (i = 0; i < 6; ++i) {
    matrix[i] = matrixA[i];
  }
  nFuncs = nFuncsA;
  for (i = 0; i < nFuncs; ++i) {
    funcs[i] = funcsA[i];
  }
}

GfxFunctionShading::GfxFunctionShading(GfxFunctionShading *shading):
  GfxShading(shading)
{
  int i;

  x0 = shading->x0;
  y0 = shading->y0;
  x1 = shading->x1;
  y1 = shading->y1;
  for (i = 0; i < 6; ++i) {
    matrix[i] = shading->matrix[i];
  }
  nFuncs = shading->nFuncs;
  for (i = 0; i < nFuncs; ++i) {
    funcs[i] = shading->funcs[i]->copy();
  }
}

GfxFunctionShading::~GfxFunctionShading() {
  int i;

  for (i = 0; i < nFuncs; ++i) {
    delete funcs[i];
  }
}

GfxFunctionShading *GfxFunctionShading::parse(Dict *dict) {
  GfxFunctionShading *shading;
  double x0A, y0A, x1A, y1A;
  double matrixA[6];
  Function *funcsA[gfxColorMaxComps];
  int nFuncsA;
  Object obj1, obj2;
  int i;

  x0A = y0A = 0;
  x1A = y1A = 1;
  if (dict->lookup("Domain", &obj1)->isArray() &&
      obj1.arrayGetLength() == 4) {
    x0A = obj1.arrayGet(0, &obj2)->getNum();
    obj2.free();
    y0A = obj1.arrayGet(1, &obj2)->getNum();
    obj2.free();
    x1A = obj1.arrayGet(2, &obj2)->getNum();
    obj2.free();
    y1A = obj1.arrayGet(3, &obj2)->getNum();
    obj2.free();
  }
  obj1.free();

  matrixA[0] = 1; matrixA[1] = 0;
  matrixA[2] = 0; matrixA[3] = 1;
  matrixA[4] = 0; matrixA[5] = 0;
  if (dict->lookup("Matrix", &obj1)->isArray() &&
      obj1.arrayGetLength() == 6) {
    matrixA[0] = obj1.arrayGet(0, &obj2)->getNum();
    obj2.free();
    matrixA[1] = obj1.arrayGet(1, &obj2)->getNum();
    obj2.free();
    matrixA[2] = obj1.arrayGet(2, &obj2)->getNum();
    obj2.free();
    matrixA[3] = obj1.arrayGet(3, &obj2)->getNum();
    obj2.free();
    matrixA[4] = obj1.arrayGet(4, &obj2)->getNum();
    obj2.free();
    matrixA[5] = obj1.arrayGet(5, &obj2)->getNum();
    obj2.free();
  }
  obj1.free();

  dict->lookup("Function", &obj1);
  if (obj1.isArray()) {
    nFuncsA = obj1.arrayGetLength();
    if (nFuncsA > gfxColorMaxComps) {
      error(-1, "Invalid Function array in shading dictionary");
      goto err1;
    }
    for (i = 0; i < nFuncsA; ++i) {
      obj1.arrayGet(i, &obj2);
      if (!(funcsA[i] = Function::parse(&obj2))) {
	goto err2;
      }
      obj2.free();
    }
  } else {
    nFuncsA = 1;
    if (!(funcsA[0] = Function::parse(&obj1))) {
      goto err1;
    }
  }
  obj1.free();

  shading = new GfxFunctionShading(x0A, y0A, x1A, y1A, matrixA,
				   funcsA, nFuncsA);
  if (!shading->init(dict)) {
    delete shading;
    return NULL;
  }
  return shading;

 err2:
  obj2.free();
 err1:
  obj1.free();
  return NULL;
}

GfxShading *GfxFunctionShading::copy() {
  return new GfxFunctionShading(this);
}

void GfxFunctionShading::getColor(double x, double y, GfxColor *color) {
  double in[2], out[gfxColorMaxComps];
  int i;

  // NB: there can be one function with n outputs or n functions with
  // one output each (where n = number of color components)
  for (i = 0; i < gfxColorMaxComps; ++i) {
    out[i] = 0;
  }
  in[0] = x;
  in[1] = y;
  for (i = 0; i < nFuncs; ++i) {
    funcs[i]->transform(in, &out[i]);
  }
  for (i = 0; i < gfxColorMaxComps; ++i) {
    color->c[i] = dblToCol(out[i]);
  }
}

//------------------------------------------------------------------------
// GfxAxialShading
//------------------------------------------------------------------------

GfxAxialShading::GfxAxialShading(double x0A, double y0A,
				 double x1A, double y1A,
				 double t0A, double t1A,
				 Function **funcsA, int nFuncsA,
				 GBool extend0A, GBool extend1A):
  GfxShading(2)
{
  int i;

  x0 = x0A;
  y0 = y0A;
  x1 = x1A;
  y1 = y1A;
  t0 = t0A;
  t1 = t1A;
  nFuncs = nFuncsA;
  for (i = 0; i < nFuncs; ++i) {
    funcs[i] = funcsA[i];
  }
  extend0 = extend0A;
  extend1 = extend1A;
}

GfxAxialShading::GfxAxialShading(GfxAxialShading *shading):
  GfxShading(shading)
{
  int i;

  x0 = shading->x0;
  y0 = shading->y0;
  x1 = shading->x1;
  y1 = shading->y1;
  t0 = shading->t0;
  y1 = shading->t1;
  nFuncs = shading->nFuncs;
  for (i = 0; i < nFuncs; ++i) {
    funcs[i] = shading->funcs[i]->copy();
  }
  extend0 = shading->extend0;
  extend1 = shading->extend1;
}

GfxAxialShading::~GfxAxialShading() {
  int i;

  for (i = 0; i < nFuncs; ++i) {
    delete funcs[i];
  }
}

GfxAxialShading *GfxAxialShading::parse(Dict *dict) {
  GfxAxialShading *shading;
  double x0A, y0A, x1A, y1A;
  double t0A, t1A;
  Function *funcsA[gfxColorMaxComps];
  int nFuncsA;
  GBool extend0A, extend1A;
  Object obj1, obj2;
  int i;

  x0A = y0A = x1A = y1A = 0;
  if (dict->lookup("Coords", &obj1)->isArray() &&
      obj1.arrayGetLength() == 4) {
    x0A = obj1.arrayGet(0, &obj2)->getNum();
    obj2.free();
    y0A = obj1.arrayGet(1, &obj2)->getNum();
    obj2.free();
    x1A = obj1.arrayGet(2, &obj2)->getNum();
    obj2.free();
    y1A = obj1.arrayGet(3, &obj2)->getNum();
    obj2.free();
  } else {
    error(-1, "Missing or invalid Coords in shading dictionary");
    goto err1;
  }
  obj1.free();

  t0A = 0;
  t1A = 1;
  if (dict->lookup("Domain", &obj1)->isArray() &&
      obj1.arrayGetLength() == 2) {
    t0A = obj1.arrayGet(0, &obj2)->getNum();
    obj2.free();
    t1A = obj1.arrayGet(1, &obj2)->getNum();
    obj2.free();
  }
  obj1.free();

  dict->lookup("Function", &obj1);
  if (obj1.isArray()) {
    nFuncsA = obj1.arrayGetLength();
    if (nFuncsA > gfxColorMaxComps) {
      error(-1, "Invalid Function array in shading dictionary");
      goto err1;
    }
    for (i = 0; i < nFuncsA; ++i) {
      obj1.arrayGet(i, &obj2);
      if (!(funcsA[i] = Function::parse(&obj2))) {
	obj1.free();
	obj2.free();
	goto err1;
      }
      obj2.free();
    }
  } else {
    nFuncsA = 1;
    if (!(funcsA[0] = Function::parse(&obj1))) {
      obj1.free();
      goto err1;
    }
  }
  obj1.free();

  extend0A = extend1A = gFalse;
  if (dict->lookup("Extend", &obj1)->isArray() &&
      obj1.arrayGetLength() == 2) {
    extend0A = obj1.arrayGet(0, &obj2)->getBool();
    obj2.free();
    extend1A = obj1.arrayGet(1, &obj2)->getBool();
    obj2.free();
  }
  obj1.free();

  shading = new GfxAxialShading(x0A, y0A, x1A, y1A, t0A, t1A,
				funcsA, nFuncsA, extend0A, extend1A);
  if (!shading->init(dict)) {
    delete shading;
    return NULL;
  }
  return shading;

 err1:
  return NULL;
}

GfxShading *GfxAxialShading::copy() {
  return new GfxAxialShading(this);
}

void GfxAxialShading::getColor(double t, GfxColor *color) {
  double out[gfxColorMaxComps];
  int i;

  // NB: there can be one function with n outputs or n functions with
  // one output each (where n = number of color components)
  for (i = 0; i < gfxColorMaxComps; ++i) {
    out[i] = 0;
  }
  for (i = 0; i < nFuncs; ++i) {
    funcs[i]->transform(&t, &out[i]);
  }
  for (i = 0; i < gfxColorMaxComps; ++i) {
    color->c[i] = dblToCol(out[i]);
  }
}

//------------------------------------------------------------------------
// GfxRadialShading
//------------------------------------------------------------------------

GfxRadialShading::GfxRadialShading(double x0A, double y0A, double r0A,
				   double x1A, double y1A, double r1A,
				   double t0A, double t1A,
				   Function **funcsA, int nFuncsA,
				   GBool extend0A, GBool extend1A):
  GfxShading(3)
{
  int i;

  x0 = x0A;
  y0 = y0A;
  r0 = r0A;
  x1 = x1A;
  y1 = y1A;
  r1 = r1A;
  t0 = t0A;
  t1 = t1A;
  nFuncs = nFuncsA;
  for (i = 0; i < nFuncs; ++i) {
    funcs[i] = funcsA[i];
  }
  extend0 = extend0A;
  extend1 = extend1A;
}

GfxRadialShading::GfxRadialShading(GfxRadialShading *shading):
  GfxShading(shading)
{
  int i;

  x0 = shading->x0;
  y0 = shading->y0;
  r0 = shading->r0;
  x1 = shading->x1;
  y1 = shading->y1;
  r1 = shading->r1;
  t0 = shading->t0;
  y1 = shading->t1;
  nFuncs = shading->nFuncs;
  for (i = 0; i < nFuncs; ++i) {
    funcs[i] = shading->funcs[i]->copy();
  }
  extend0 = shading->extend0;
  extend1 = shading->extend1;
}

GfxRadialShading::~GfxRadialShading() {
  int i;

  for (i = 0; i < nFuncs; ++i) {
    delete funcs[i];
  }
}

GfxRadialShading *GfxRadialShading::parse(Dict *dict) {
  GfxRadialShading *shading;
  double x0A, y0A, r0A, x1A, y1A, r1A;
  double t0A, t1A;
  Function *funcsA[gfxColorMaxComps];
  int nFuncsA;
  GBool extend0A, extend1A;
  Object obj1, obj2;
  int i;

  x0A = y0A = r0A = x1A = y1A = r1A = 0;
  if (dict->lookup("Coords", &obj1)->isArray() &&
      obj1.arrayGetLength() == 6) {
    x0A = obj1.arrayGet(0, &obj2)->getNum();
    obj2.free();
    y0A = obj1.arrayGet(1, &obj2)->getNum();
    obj2.free();
    r0A = obj1.arrayGet(2, &obj2)->getNum();
    obj2.free();
    x1A = obj1.arrayGet(3, &obj2)->getNum();
    obj2.free();
    y1A = obj1.arrayGet(4, &obj2)->getNum();
    obj2.free();
    r1A = obj1.arrayGet(5, &obj2)->getNum();
    obj2.free();
  } else {
    error(-1, "Missing or invalid Coords in shading dictionary");
    goto err1;
  }
  obj1.free();

  t0A = 0;
  t1A = 1;
  if (dict->lookup("Domain", &obj1)->isArray() &&
      obj1.arrayGetLength() == 2) {
    t0A = obj1.arrayGet(0, &obj2)->getNum();
    obj2.free();
    t1A = obj1.arrayGet(1, &obj2)->getNum();
    obj2.free();
  }
  obj1.free();

  dict->lookup("Function", &obj1);
  if (obj1.isArray()) {
    nFuncsA = obj1.arrayGetLength();
    if (nFuncsA > gfxColorMaxComps) {
      error(-1, "Invalid Function array in shading dictionary");
      goto err1;
    }
    for (i = 0; i < nFuncsA; ++i) {
      obj1.arrayGet(i, &obj2);
      if (!(funcsA[i] = Function::parse(&obj2))) {
	obj1.free();
	obj2.free();
	goto err1;
      }
      obj2.free();
    }
  } else {
    nFuncsA = 1;
    if (!(funcsA[0] = Function::parse(&obj1))) {
      obj1.free();
      goto err1;
    }
  }
  obj1.free();

  extend0A = extend1A = gFalse;
  if (dict->lookup("Extend", &obj1)->isArray() &&
      obj1.arrayGetLength() == 2) {
    extend0A = obj1.arrayGet(0, &obj2)->getBool();
    obj2.free();
    extend1A = obj1.arrayGet(1, &obj2)->getBool();
    obj2.free();
  }
  obj1.free();

  shading = new GfxRadialShading(x0A, y0A, r0A, x1A, y1A, r1A, t0A, t1A,
				 funcsA, nFuncsA, extend0A, extend1A);
  if (!shading->init(dict)) {
    delete shading;
    return NULL;
  }
  return shading;

 err1:
  return NULL;
}

GfxShading *GfxRadialShading::copy() {
  return new GfxRadialShading(this);
}

void GfxRadialShading::getColor(double t, GfxColor *color) {
  double out[gfxColorMaxComps];
  int i;

  // NB: there can be one function with n outputs or n functions with
  // one output each (where n = number of color components)
  for (i = 0; i < gfxColorMaxComps; ++i) {
    out[i] = 0;
  }
  for (i = 0; i < nFuncs; ++i) {
    funcs[i]->transform(&t, &out[i]);
  }
  for (i = 0; i < gfxColorMaxComps; ++i) {
    color->c[i] = dblToCol(out[i]);
  }
}

//------------------------------------------------------------------------
// GfxShadingBitBuf
//------------------------------------------------------------------------

class GfxShadingBitBuf {
public:

  GfxShadingBitBuf(Stream *strA);
  ~GfxShadingBitBuf();
  GBool getBits(int n, Guint *val);
  void flushBits();

private:

  Stream *str;
  int bitBuf;
  int nBits;
};

GfxShadingBitBuf::GfxShadingBitBuf(Stream *strA) {
  str = strA;
  str->reset();
  bitBuf = 0;
  nBits = 0;
}

GfxShadingBitBuf::~GfxShadingBitBuf() {
  str->close();
}

GBool GfxShadingBitBuf::getBits(int n, Guint *val) {
  int x;

  if (nBits >= n) {
    x = (bitBuf >> (nBits - n)) & ((1 << n) - 1);
    nBits -= n;
  } else {
    x = 0;
    if (nBits > 0) {
      x = bitBuf & ((1 << nBits) - 1);
      n -= nBits;
      nBits = 0;
    }
    while (n > 0) {
      if ((bitBuf = str->getChar()) == EOF) {
	nBits = 0;
	return gFalse;
      }
      if (n >= 8) {
	x = (x << 8) | bitBuf;
	n -= 8;
      } else {
	x = (x << n) | (bitBuf >> (8 - n));
	nBits = 8 - n;
	n = 0;
      }
    }
  }
  *val = x;
  return gTrue;
}

void GfxShadingBitBuf::flushBits() {
  bitBuf = 0;
  nBits = 0;
}

//------------------------------------------------------------------------
// GfxGouraudTriangleShading
//------------------------------------------------------------------------

GfxGouraudTriangleShading::GfxGouraudTriangleShading(
			       int typeA,
			       GfxGouraudVertex *verticesA, int nVerticesA,
			       int (*trianglesA)[3], int nTrianglesA,
			       Function **funcsA, int nFuncsA):
  GfxShading(typeA)
{
  int i;

  vertices = verticesA;
  nVertices = nVerticesA;
  triangles = trianglesA;
  nTriangles = nTrianglesA;
  nFuncs = nFuncsA;
  for (i = 0; i < nFuncs; ++i) {
    funcs[i] = funcsA[i];
  }
}

GfxGouraudTriangleShading::GfxGouraudTriangleShading(
			       GfxGouraudTriangleShading *shading):
  GfxShading(shading)
{
  int i;

  nVertices = shading->nVertices;
  vertices = (GfxGouraudVertex *)gmallocn(nVertices, sizeof(GfxGouraudVertex));
  memcpy(vertices, shading->vertices, nVertices * sizeof(GfxGouraudVertex));
  nTriangles = shading->nTriangles;
  triangles = (int (*)[3])gmallocn(nTriangles * 3, sizeof(int));
  memcpy(triangles, shading->triangles, nTriangles * 3 * sizeof(int));
  nFuncs = shading->nFuncs;
  for (i = 0; i < nFuncs; ++i) {
    funcs[i] = shading->funcs[i]->copy();
  }
}

GfxGouraudTriangleShading::~GfxGouraudTriangleShading() {
  int i;

  gfree(vertices);
  gfree(triangles);
  for (i = 0; i < nFuncs; ++i) {
    delete funcs[i];
  }
}

GfxGouraudTriangleShading *GfxGouraudTriangleShading::parse(int typeA,
							    Dict *dict,
							    Stream *str) {
  GfxGouraudTriangleShading *shading;
  Function *funcsA[gfxColorMaxComps];
  int nFuncsA;
  int coordBits, compBits, flagBits, vertsPerRow, nRows;
  double xMin, xMax, yMin, yMax;
  double cMin[gfxColorMaxComps], cMax[gfxColorMaxComps];
  double xMul, yMul;
  double cMul[gfxColorMaxComps];
  GfxGouraudVertex *verticesA;
  int (*trianglesA)[3];
  int nComps, nVerticesA, nTrianglesA, vertSize, triSize;
  Guint x, y, flag;
  Guint c[gfxColorMaxComps];
  GfxShadingBitBuf *bitBuf;
  Object obj1, obj2;
  int i, j, k, state;

  if (dict->lookup("BitsPerCoordinate", &obj1)->isInt()) {
    coordBits = obj1.getInt();
  } else {
    error(-1, "Missing or invalid BitsPerCoordinate in shading dictionary");
    goto err2;
  }
  obj1.free();
  if (dict->lookup("BitsPerComponent", &obj1)->isInt()) {
    compBits = obj1.getInt();
  } else {
    error(-1, "Missing or invalid BitsPerComponent in shading dictionary");
    goto err2;
  }
  obj1.free();
  flagBits = vertsPerRow = 0; // make gcc happy
  if (typeA == 4) {
    if (dict->lookup("BitsPerFlag", &obj1)->isInt()) {
      flagBits = obj1.getInt();
    } else {
      error(-1, "Missing or invalid BitsPerFlag in shading dictionary");
      goto err2;
    }
    obj1.free();
  } else {
    if (dict->lookup("VerticesPerRow", &obj1)->isInt()) {
      vertsPerRow = obj1.getInt();
    } else {
      error(-1, "Missing or invalid VerticesPerRow in shading dictionary");
      goto err2;
    }
    obj1.free();
  }
  if (dict->lookup("Decode", &obj1)->isArray() &&
      obj1.arrayGetLength() >= 6) {
    xMin = obj1.arrayGet(0, &obj2)->getNum();
    obj2.free();
    xMax = obj1.arrayGet(1, &obj2)->getNum();
    obj2.free();
    xMul = (xMax - xMin) / (pow(2.0, coordBits) - 1);
    yMin = obj1.arrayGet(2, &obj2)->getNum();
    obj2.free();
    yMax = obj1.arrayGet(3, &obj2)->getNum();
    obj2.free();
    yMul = (yMax - yMin) / (pow(2.0, coordBits) - 1);
    for (i = 0; 5 + 2*i < obj1.arrayGetLength() && i < gfxColorMaxComps; ++i) {
      cMin[i] = obj1.arrayGet(4 + 2*i, &obj2)->getNum();
      obj2.free();
      cMax[i] = obj1.arrayGet(5 + 2*i, &obj2)->getNum();
      obj2.free();
      cMul[i] = (cMax[i] - cMin[i]) / (double)((1 << compBits) - 1);
    }
    nComps = i;
  } else {
    error(-1, "Missing or invalid Decode array in shading dictionary");
    goto err2;
  }
  obj1.free();

  if (!dict->lookup("Function", &obj1)->isNull()) {
    if (obj1.isArray()) {
      nFuncsA = obj1.arrayGetLength();
      if (nFuncsA > gfxColorMaxComps) {
	error(-1, "Invalid Function array in shading dictionary");
	goto err1;
      }
      for (i = 0; i < nFuncsA; ++i) {
	obj1.arrayGet(i, &obj2);
	if (!(funcsA[i] = Function::parse(&obj2))) {
	  obj1.free();
	  obj2.free();
	  goto err1;
	}
	obj2.free();
      }
    } else {
      nFuncsA = 1;
      if (!(funcsA[0] = Function::parse(&obj1))) {
	obj1.free();
	goto err1;
      }
    }
  } else {
    nFuncsA = 0;
  }
  obj1.free();

  nVerticesA = nTrianglesA = 0;
  verticesA = NULL;
  trianglesA = NULL;
  vertSize = triSize = 0;
  state = 0;
  flag = 0; // make gcc happy
  bitBuf = new GfxShadingBitBuf(str);
  while (1) {
    if (typeA == 4) {
      if (!bitBuf->getBits(flagBits, &flag)) {
	break;
      }
    }
    if (!bitBuf->getBits(coordBits, &x) ||
	!bitBuf->getBits(coordBits, &y)) {
      break;
    }
    for (i = 0; i < nComps; ++i) {
      if (!bitBuf->getBits(compBits, &c[i])) {
	break;
      }
    }
    if (i < nComps) {
      break;
    }
    if (nVerticesA == vertSize) {
      vertSize = (vertSize == 0) ? 16 : 2 * vertSize;
      verticesA = (GfxGouraudVertex *)
	              greallocn(verticesA, vertSize, sizeof(GfxGouraudVertex));
    }
    verticesA[nVerticesA].x = xMin + xMul * (double)x;
    verticesA[nVerticesA].y = yMin + yMul * (double)y;
    for (i = 0; i < nComps; ++i) {
      verticesA[nVerticesA].color.c[i] =
	  dblToCol(cMin[i] + cMul[i] * (double)c[i]);
    }
    ++nVerticesA;
    bitBuf->flushBits();
    if (typeA == 4) {
      if (state == 0 || state == 1) {
	++state;
      } else if (state == 2 || flag > 0) {
	if (nTrianglesA == triSize) {
	  triSize = (triSize == 0) ? 16 : 2 * triSize;
	  trianglesA = (int (*)[3])
	                   greallocn(trianglesA, triSize * 3, sizeof(int));
	}
	if (state == 2) {
	  trianglesA[nTrianglesA][0] = nVerticesA - 3;
	  trianglesA[nTrianglesA][1] = nVerticesA - 2;
	  trianglesA[nTrianglesA][2] = nVerticesA - 1;
	  ++state;
	} else if (flag == 1) {
	  trianglesA[nTrianglesA][0] = trianglesA[nTrianglesA - 1][1];
	  trianglesA[nTrianglesA][1] = trianglesA[nTrianglesA - 1][2];
	  trianglesA[nTrianglesA][2] = nVerticesA - 1;
	} else { // flag == 2
	  trianglesA[nTrianglesA][0] = trianglesA[nTrianglesA - 1][0];
	  trianglesA[nTrianglesA][1] = trianglesA[nTrianglesA - 1][2];
	  trianglesA[nTrianglesA][2] = nVerticesA - 1;
	}
	++nTrianglesA;
      } else { // state == 3 && flag == 0
	state = 1;
      }
    }
  }
  delete bitBuf;
  if (typeA == 5) {
    nRows = nVerticesA / vertsPerRow;
    nTrianglesA = (nRows - 1) * 2 * (vertsPerRow - 1);
    trianglesA = (int (*)[3])gmallocn(nTrianglesA * 3, sizeof(int));
    k = 0;
    for (i = 0; i < nRows - 1; ++i) {
      for (j = 0; j < vertsPerRow - 1; ++j) {
	trianglesA[k][0] = i * vertsPerRow + j;
	trianglesA[k][1] = i * vertsPerRow + j+1;
	trianglesA[k][2] = (i+1) * vertsPerRow + j;
	++k;
	trianglesA[k][0] = i * vertsPerRow + j+1;
	trianglesA[k][1] = (i+1) * vertsPerRow + j;
	trianglesA[k][2] = (i+1) * vertsPerRow + j+1;
	++k;
      }
    }
  }

  shading = new GfxGouraudTriangleShading(typeA, verticesA, nVerticesA,
					  trianglesA, nTrianglesA,
					  funcsA, nFuncsA);
  if (!shading->init(dict)) {
    delete shading;
    return NULL;
  }
  return shading;

 err2:
  obj1.free();
 err1:
  return NULL;
}

GfxShading *GfxGouraudTriangleShading::copy() {
  return new GfxGouraudTriangleShading(this);
}

void GfxGouraudTriangleShading::getTriangle(
				    int i,
				    double *x0, double *y0, GfxColor *color0,
				    double *x1, double *y1, GfxColor *color1,
				    double *x2, double *y2, GfxColor *color2) {
  double in;
  double out[gfxColorMaxComps];
  int v, j;

  v = triangles[i][0];
  *x0 = vertices[v].x;
  *y0 = vertices[v].y;
  if (nFuncs > 0) {
    in = colToDbl(vertices[v].color.c[0]);
    for (j = 0; j < nFuncs; ++j) {
      funcs[j]->transform(&in, &out[j]);
    }
    for (j = 0; j < gfxColorMaxComps; ++j) {
      color0->c[j] = dblToCol(out[j]);
    }
  } else {
    *color0 = vertices[v].color;
  }
  v = triangles[i][1];
  *x1 = vertices[v].x;
  *y1 = vertices[v].y;
  if (nFuncs > 0) {
    in = colToDbl(vertices[v].color.c[0]);
    for (j = 0; j < nFuncs; ++j) {
      funcs[j]->transform(&in, &out[j]);
    }
    for (j = 0; j < gfxColorMaxComps; ++j) {
      color1->c[j] = dblToCol(out[j]);
    }
  } else {
    *color1 = vertices[v].color;
  }
  v = triangles[i][2];
  *x2 = vertices[v].x;
  *y2 = vertices[v].y;
  if (nFuncs > 0) {
    in = colToDbl(vertices[v].color.c[0]);
    for (j = 0; j < nFuncs; ++j) {
      funcs[j]->transform(&in, &out[j]);
    }
    for (j = 0; j < gfxColorMaxComps; ++j) {
      color2->c[j] = dblToCol(out[j]);
    }
  } else {
    *color2 = vertices[v].color;
  }
}

//------------------------------------------------------------------------
// GfxPatchMeshShading
//------------------------------------------------------------------------

GfxPatchMeshShading::GfxPatchMeshShading(int typeA,
					 GfxPatch *patchesA, int nPatchesA,
					 Function **funcsA, int nFuncsA):
  GfxShading(typeA)
{
  int i;

  patches = patchesA;
  nPatches = nPatchesA;
  nFuncs = nFuncsA;
  for (i = 0; i < nFuncs; ++i) {
    funcs[i] = funcsA[i];
  }
}

GfxPatchMeshShading::GfxPatchMeshShading(GfxPatchMeshShading *shading):
  GfxShading(shading)
{
  int i;

  nPatches = shading->nPatches;
  patches = (GfxPatch *)gmallocn(nPatches, sizeof(GfxPatch));
  memcpy(patches, shading->patches, nPatches * sizeof(GfxPatch));
  nFuncs = shading->nFuncs;
  for (i = 0; i < nFuncs; ++i) {
    funcs[i] = shading->funcs[i]->copy();
  }
}

GfxPatchMeshShading::~GfxPatchMeshShading() {
  int i;

  gfree(patches);
  for (i = 0; i < nFuncs; ++i) {
    delete funcs[i];
  }
}

GfxPatchMeshShading *GfxPatchMeshShading::parse(int typeA, Dict *dict,
						Stream *str) {
  GfxPatchMeshShading *shading;
  Function *funcsA[gfxColorMaxComps];
  int nFuncsA;
  int coordBits, compBits, flagBits;
  double xMin, xMax, yMin, yMax;
  double cMin[gfxColorMaxComps], cMax[gfxColorMaxComps];
  double xMul, yMul;
  double cMul[gfxColorMaxComps];
  GfxPatch *patchesA, *p;
  int nComps, nPatchesA, patchesSize, nPts, nColors;
  Guint flag;
  double x[16], y[16];
  Guint xi, yi;
  GfxColorComp c[4][gfxColorMaxComps];
  Guint ci[4];
  GfxShadingBitBuf *bitBuf;
  Object obj1, obj2;
  int i, j;

  if (dict->lookup("BitsPerCoordinate", &obj1)->isInt()) {
    coordBits = obj1.getInt();
  } else {
    error(-1, "Missing or invalid BitsPerCoordinate in shading dictionary");
    goto err2;
  }
  obj1.free();
  if (dict->lookup("BitsPerComponent", &obj1)->isInt()) {
    compBits = obj1.getInt();
  } else {
    error(-1, "Missing or invalid BitsPerComponent in shading dictionary");
    goto err2;
  }
  obj1.free();
  if (dict->lookup("BitsPerFlag", &obj1)->isInt()) {
    flagBits = obj1.getInt();
  } else {
    error(-1, "Missing or invalid BitsPerFlag in shading dictionary");
    goto err2;
  }
  obj1.free();
  if (dict->lookup("Decode", &obj1)->isArray() &&
      obj1.arrayGetLength() >= 6) {
    xMin = obj1.arrayGet(0, &obj2)->getNum();
    obj2.free();
    xMax = obj1.arrayGet(1, &obj2)->getNum();
    obj2.free();
    xMul = (xMax - xMin) / (pow(2.0, coordBits) - 1);
    yMin = obj1.arrayGet(2, &obj2)->getNum();
    obj2.free();
    yMax = obj1.arrayGet(3, &obj2)->getNum();
    obj2.free();
    yMul = (yMax - yMin) / (pow(2.0, coordBits) - 1);
    for (i = 0; 5 + 2*i < obj1.arrayGetLength() && i < gfxColorMaxComps; ++i) {
      cMin[i] = obj1.arrayGet(4 + 2*i, &obj2)->getNum();
      obj2.free();
      cMax[i] = obj1.arrayGet(5 + 2*i, &obj2)->getNum();
      obj2.free();
      cMul[i] = (cMax[i] - cMin[i]) / (double)((1 << compBits) - 1);
    }
    nComps = i;
  } else {
    error(-1, "Missing or invalid Decode array in shading dictionary");
    goto err2;
  }
  obj1.free();

  if (!dict->lookup("Function", &obj1)->isNull()) {
    if (obj1.isArray()) {
      nFuncsA = obj1.arrayGetLength();
      if (nFuncsA > gfxColorMaxComps) {
	error(-1, "Invalid Function array in shading dictionary");
	goto err1;
      }
      for (i = 0; i < nFuncsA; ++i) {
	obj1.arrayGet(i, &obj2);
	if (!(funcsA[i] = Function::parse(&obj2))) {
	  obj1.free();
	  obj2.free();
	  goto err1;
	}
	obj2.free();
      }
    } else {
      nFuncsA = 1;
      if (!(funcsA[0] = Function::parse(&obj1))) {
	obj1.free();
	goto err1;
      }
    }
  } else {
    nFuncsA = 0;
  }
  obj1.free();

  nPatchesA = 0;
  patchesA = NULL;
  patchesSize = 0;
  bitBuf = new GfxShadingBitBuf(str);
  while (1) {
    if (!bitBuf->getBits(flagBits, &flag)) {
      break;
    }
    if (typeA == 6) {
      switch (flag) {
      case 0: nPts = 12; nColors = 4; break;
      case 1:
      case 2:
      case 3:
      default: nPts =  8; nColors = 2; break;
      }
    } else {
      switch (flag) {
      case 0: nPts = 16; nColors = 4; break;
      case 1:
      case 2:
      case 3:
      default: nPts = 12; nColors = 2; break;
      }
    }
    for (i = 0; i < nPts; ++i) {
      if (!bitBuf->getBits(coordBits, &xi) ||
	  !bitBuf->getBits(coordBits, &yi)) {
	break;
      }
      x[i] = xMin + xMul * (double)xi;
      y[i] = yMin + yMul * (double)yi;
    }
    if (i < nPts) {
      break;
    }
    for (i = 0; i < nColors; ++i) {
      for (j = 0; j < nComps; ++j) {
	if (!bitBuf->getBits(compBits, &ci[j])) {
	  break;
	}
	c[i][j] = dblToCol(cMin[j] + cMul[j] * (double)ci[j]);
      }
      if (j < nComps) {
	break;
      }
    }
    if (i < nColors) {
      break;
    }
    if (nPatchesA == patchesSize) {
      patchesSize = (patchesSize == 0) ? 16 : 2 * patchesSize;
      patchesA = (GfxPatch *)greallocn(patchesA,
				       patchesSize, sizeof(GfxPatch));
    }
    p = &patchesA[nPatchesA];
    if (typeA == 6) {
      switch (flag) {
      case 0:
	p->x[0][0] = x[0];
	p->y[0][0] = y[0];
	p->x[0][1] = x[1];
	p->y[0][1] = y[1];
	p->x[0][2] = x[2];
	p->y[0][2] = y[2];
	p->x[0][3] = x[3];
	p->y[0][3] = y[3];
	p->x[1][3] = x[4];
	p->y[1][3] = y[4];
	p->x[2][3] = x[5];
	p->y[2][3] = y[5];
	p->x[3][3] = x[6];
	p->y[3][3] = y[6];
	p->x[3][2] = x[7];
	p->y[3][2] = y[7];
	p->x[3][1] = x[8];
	p->y[3][1] = y[8];
	p->x[3][0] = x[9];
	p->y[3][0] = y[9];
	p->x[2][0] = x[10];
	p->y[2][0] = y[10];
	p->x[1][0] = x[11];
	p->y[1][0] = y[11];
	for (j = 0; j < nComps; ++j) {
	  p->color[0][0].c[j] = c[0][j];
	  p->color[0][1].c[j] = c[1][j];
	  p->color[1][1].c[j] = c[2][j];
	  p->color[1][0].c[j] = c[3][j];
	}
	break;
      case 1:
	p->x[0][0] = patchesA[nPatchesA-1].x[0][3];
	p->y[0][0] = patchesA[nPatchesA-1].y[0][3];
	p->x[0][1] = patchesA[nPatchesA-1].x[1][3];
	p->y[0][1] = patchesA[nPatchesA-1].y[1][3];
	p->x[0][2] = patchesA[nPatchesA-1].x[2][3];
	p->y[0][2] = patchesA[nPatchesA-1].y[2][3];
	p->x[0][3] = patchesA[nPatchesA-1].x[3][3];
	p->y[0][3] = patchesA[nPatchesA-1].y[3][3];
	p->x[1][3] = x[0];
	p->y[1][3] = y[0];
	p->x[2][3] = x[1];
	p->y[2][3] = y[1];
	p->x[3][3] = x[2];
	p->y[3][3] = y[2];
	p->x[3][2] = x[3];
	p->y[3][2] = y[3];
	p->x[3][1] = x[4];
	p->y[3][1] = y[4];
	p->x[3][0] = x[5];
	p->y[3][0] = y[5];
	p->x[2][0] = x[6];
	p->y[2][0] = y[6];
	p->x[1][0] = x[7];
	p->y[1][0] = y[7];
	for (j = 0; j < nComps; ++j) {
	  p->color[0][0].c[j] = patchesA[nPatchesA-1].color[0][1].c[j];
	  p->color[0][1].c[j] = patchesA[nPatchesA-1].color[1][1].c[j];
	  p->color[1][1].c[j] = c[0][j];
	  p->color[1][0].c[j] = c[1][j];
	}
	break;
      case 2:
	p->x[0][0] = patchesA[nPatchesA-1].x[3][3];
	p->y[0][0] = patchesA[nPatchesA-1].y[3][3];
	p->x[0][1] = patchesA[nPatchesA-1].x[3][2];
	p->y[0][1] = patchesA[nPatchesA-1].y[3][2];
	p->x[0][2] = patchesA[nPatchesA-1].x[3][1];
	p->y[0][2] = patchesA[nPatchesA-1].y[3][1];
	p->x[0][3] = patchesA[nPatchesA-1].x[3][0];
	p->y[0][3] = patchesA[nPatchesA-1].y[3][0];
	p->x[1][3] = x[0];
	p->y[1][3] = y[0];
	p->x[2][3] = x[1];
	p->y[2][3] = y[1];
	p->x[3][3] = x[2];
	p->y[3][3] = y[2];
	p->x[3][2] = x[3];
	p->y[3][2] = y[3];
	p->x[3][1] = x[4];
	p->y[3][1] = y[4];
	p->x[3][0] = x[5];
	p->y[3][0] = y[5];
	p->x[2][0] = x[6];
	p->y[2][0] = y[6];
	p->x[1][0] = x[7];
	p->y[1][0] = y[7];
	for (j = 0; j < nComps; ++j) {
	  p->color[0][0].c[j] = patchesA[nPatchesA-1].color[1][1].c[j];
	  p->color[0][1].c[j] = patchesA[nPatchesA-1].color[1][0].c[j];
	  p->color[1][1].c[j] = c[0][j];
	  p->color[1][0].c[j] = c[1][j];
	}
	break;
      case 3:
	p->x[0][0] = patchesA[nPatchesA-1].x[3][0];
	p->y[0][0] = patchesA[nPatchesA-1].y[3][0];
	p->x[0][1] = patchesA[nPatchesA-1].x[2][0];
	p->y[0][1] = patchesA[nPatchesA-1].y[2][0];
	p->x[0][2] = patchesA[nPatchesA-1].x[1][0];
	p->y[0][2] = patchesA[nPatchesA-1].y[1][0];
	p->x[0][3] = patchesA[nPatchesA-1].x[0][0];
	p->y[0][3] = patchesA[nPatchesA-1].y[0][0];
	p->x[1][3] = x[0];
	p->y[1][3] = y[0];
	p->x[2][3] = x[1];
	p->y[2][3] = y[1];
	p->x[3][3] = x[2];
	p->y[3][3] = y[2];
	p->x[3][2] = x[3];
	p->y[3][2] = y[3];
	p->x[3][1] = x[4];
	p->y[3][1] = y[4];
	p->x[3][0] = x[5];
	p->y[3][0] = y[5];
	p->x[2][0] = x[6];
	p->y[2][0] = y[6];
	p->x[1][0] = x[7];
	p->y[1][0] = y[7];
	for (j = 0; j < nComps; ++j) {
	  p->color[0][1].c[j] = patchesA[nPatchesA-1].color[1][0].c[j];
	  p->color[0][1].c[j] = patchesA[nPatchesA-1].color[0][0].c[j];
	  p->color[1][1].c[j] = c[0][j];
	  p->color[1][0].c[j] = c[1][j];
	}
	break;
  }
    } else {
      switch (flag) {
      case 0:
	p->x[0][0] = x[0];
	p->y[0][0] = y[0];
	p->x[0][1] = x[1];
	p->y[0][1] = y[1];
	p->x[0][2] = x[2];
	p->y[0][2] = y[2];
	p->x[0][3] = x[3];
	p->y[0][3] = y[3];
	p->x[1][3] = x[4];
	p->y[1][3] = y[4];
	p->x[2][3] = x[5];
	p->y[2][3] = y[5];
	p->x[3][3] = x[6];
	p->y[3][3] = y[6];
	p->x[3][2] = x[7];
	p->y[3][2] = y[7];
	p->x[3][1] = x[8];
	p->y[3][1] = y[8];
	p->x[3][0] = x[9];
	p->y[3][0] = y[9];
	p->x[2][0] = x[10];
	p->y[2][0] = y[10];
	p->x[1][0] = x[11];
	p->y[1][0] = y[11];
	p->x[1][1] = x[12];
	p->y[1][1] = y[12];
	p->x[1][2] = x[13];
	p->y[1][2] = y[13];
	p->x[2][2] = x[14];
	p->y[2][2] = y[14];
	p->x[2][1] = x[15];
	p->y[2][1] = y[15];
	for (j = 0; j < nComps; ++j) {
	  p->color[0][0].c[j] = c[0][j];
	  p->color[0][1].c[j] = c[1][j];
	  p->color[1][1].c[j] = c[2][j];
	  p->color[1][0].c[j] = c[3][j];
	}
	break;
      case 1:
	p->x[0][0] = patchesA[nPatchesA-1].x[0][3];
	p->y[0][0] = patchesA[nPatchesA-1].y[0][3];
	p->x[0][1] = patchesA[nPatchesA-1].x[1][3];
	p->y[0][1] = patchesA[nPatchesA-1].y[1][3];
	p->x[0][2] = patchesA[nPatchesA-1].x[2][3];
	p->y[0][2] = patchesA[nPatchesA-1].y[2][3];
	p->x[0][3] = patchesA[nPatchesA-1].x[3][3];
	p->y[0][3] = patchesA[nPatchesA-1].y[3][3];
	p->x[1][3] = x[0];
	p->y[1][3] = y[0];
	p->x[2][3] = x[1];
	p->y[2][3] = y[1];
	p->x[3][3] = x[2];
	p->y[3][3] = y[2];
	p->x[3][2] = x[3];
	p->y[3][2] = y[3];
	p->x[3][1] = x[4];
	p->y[3][1] = y[4];
	p->x[3][0] = x[5];
	p->y[3][0] = y[5];
	p->x[2][0] = x[6];
	p->y[2][0] = y[6];
	p->x[1][0] = x[7];
	p->y[1][0] = y[7];
	p->x[1][1] = x[8];
	p->y[1][1] = y[8];
	p->x[1][2] = x[9];
	p->y[1][2] = y[9];
	p->x[2][2] = x[10];
	p->y[2][2] = y[10];
	p->x[2][1] = x[11];
	p->y[2][1] = y[11];
	for (j = 0; j < nComps; ++j) {
	  p->color[0][0].c[j] = patchesA[nPatchesA-1].color[0][1].c[j];
	  p->color[0][1].c[j] = patchesA[nPatchesA-1].color[1][1].c[j];
	  p->color[1][1].c[j] = c[0][j];
	  p->color[1][0].c[j] = c[1][j];
	}
	break;
      case 2:
	p->x[0][0] = patchesA[nPatchesA-1].x[3][3];
	p->y[0][0] = patchesA[nPatchesA-1].y[3][3];
	p->x[0][1] = patchesA[nPatchesA-1].x[3][2];
	p->y[0][1] = patchesA[nPatchesA-1].y[3][2];
	p->x[0][2] = patchesA[nPatchesA-1].x[3][1];
	p->y[0][2] = patchesA[nPatchesA-1].y[3][1];
	p->x[0][3] = patchesA[nPatchesA-1].x[3][0];
	p->y[0][3] = patchesA[nPatchesA-1].y[3][0];
	p->x[1][3] = x[0];
	p->y[1][3] = y[0];
	p->x[2][3] = x[1];
	p->y[2][3] = y[1];
	p->x[3][3] = x[2];
	p->y[3][3] = y[2];
	p->x[3][2] = x[3];
	p->y[3][2] = y[3];
	p->x[3][1] = x[4];
	p->y[3][1] = y[4];
	p->x[3][0] = x[5];
	p->y[3][0] = y[5];
	p->x[2][0] = x[6];
	p->y[2][0] = y[6];
	p->x[1][0] = x[7];
	p->y[1][0] = y[7];
	p->x[1][1] = x[8];
	p->y[1][1] = y[8];
	p->x[1][2] = x[9];
	p->y[1][2] = y[9];
	p->x[2][2] = x[10];
	p->y[2][2] = y[10];
	p->x[2][1] = x[11];
	p->y[2][1] = y[11];
	for (j = 0; j < nComps; ++j) {
	  p->color[0][0].c[j] = patchesA[nPatchesA-1].color[1][1].c[j];
	  p->color[0][1].c[j] = patchesA[nPatchesA-1].color[1][0].c[j];
	  p->color[1][1].c[j] = c[0][j];
	  p->color[1][0].c[j] = c[1][j];
	}
	break;
      case 3:
	p->x[0][0] = patchesA[nPatchesA-1].x[3][0];
	p->y[0][0] = patchesA[nPatchesA-1].y[3][0];
	p->x[0][1] = patchesA[nPatchesA-1].x[2][0];
	p->y[0][1] = patchesA[nPatchesA-1].y[2][0];
	p->x[0][2] = patchesA[nPatchesA-1].x[1][0];
	p->y[0][2] = patchesA[nPatchesA-1].y[1][0];
	p->x[0][3] = patchesA[nPatchesA-1].x[0][0];
	p->y[0][3] = patchesA[nPatchesA-1].y[0][0];
	p->x[1][3] = x[0];
	p->y[1][3] = y[0];
	p->x[2][3] = x[1];
	p->y[2][3] = y[1];
	p->x[3][3] = x[2];
	p->y[3][3] = y[2];
	p->x[3][2] = x[3];
	p->y[3][2] = y[3];
	p->x[3][1] = x[4];
	p->y[3][1] = y[4];
	p->x[3][0] = x[5];
	p->y[3][0] = y[5];
	p->x[2][0] = x[6];
	p->y[2][0] = y[6];
	p->x[1][0] = x[7];
	p->y[1][0] = y[7];
	p->x[1][1] = x[8];
	p->y[1][1] = y[8];
	p->x[1][2] = x[9];
	p->y[1][2] = y[9];
	p->x[2][2] = x[10];
	p->y[2][2] = y[10];
	p->x[2][1] = x[11];
	p->y[2][1] = y[11];
	for (j = 0; j < nComps; ++j) {
	  p->color[0][0].c[j] = patchesA[nPatchesA-1].color[1][0].c[j];
	  p->color[0][1].c[j] = patchesA[nPatchesA-1].color[0][0].c[j];
	  p->color[1][1].c[j] = c[0][j];
	  p->color[1][0].c[j] = c[1][j];
	}
	break;
      }
    }
    ++nPatchesA;
    bitBuf->flushBits();
  }
  delete bitBuf;

  if (typeA == 6) {
    for (i = 0; i < nPatchesA; ++i) {
      p = &patchesA[i];
      p->x[1][1] = (-4 * p->x[0][0]
		    +6 * (p->x[0][1] + p->x[1][0])
		    -2 * (p->x[0][3] + p->x[3][0])
		    +3 * (p->x[3][1] + p->x[1][3])
		    - p->x[3][3]) / 9;
      p->y[1][1] = (-4 * p->y[0][0]
		    +6 * (p->y[0][1] + p->y[1][0])
		    -2 * (p->y[0][3] + p->y[3][0])
		    +3 * (p->y[3][1] + p->y[1][3])
		    - p->y[3][3]) / 9;
      p->x[1][2] = (-4 * p->x[0][3]
		    +6 * (p->x[0][2] + p->x[1][3])
		    -2 * (p->x[0][0] + p->x[3][3])
		    +3 * (p->x[3][2] + p->x[1][0])
		    - p->x[3][0]) / 9;
      p->y[1][2] = (-4 * p->y[0][3]
		    +6 * (p->y[0][2] + p->y[1][3])
		    -2 * (p->y[0][0] + p->y[3][3])
		    +3 * (p->y[3][2] + p->y[1][0])
		    - p->y[3][0]) / 9;
      p->x[2][1] = (-4 * p->x[3][0]
		    +6 * (p->x[3][1] + p->x[2][0])
		    -2 * (p->x[3][3] + p->x[0][0])
		    +3 * (p->x[0][1] + p->x[2][3])
		    - p->x[0][3]) / 9;
      p->y[2][1] = (-4 * p->y[3][0]
		    +6 * (p->y[3][1] + p->y[2][0])
		    -2 * (p->y[3][3] + p->y[0][0])
		    +3 * (p->y[0][1] + p->y[2][3])
		    - p->y[0][3]) / 9;
      p->x[2][2] = (-4 * p->x[3][3]
		    +6 * (p->x[3][2] + p->x[2][3])
		    -2 * (p->x[3][0] + p->x[0][3])
		    +3 * (p->x[0][2] + p->x[2][0])
		    - p->x[0][0]) / 9;
      p->y[2][2] = (-4 * p->y[3][3]
		    +6 * (p->y[3][2] + p->y[2][3])
		    -2 * (p->y[3][0] + p->y[0][3])
		    +3 * (p->y[0][2] + p->y[2][0])
		    - p->y[0][0]) / 9;
    }
  }

  shading = new GfxPatchMeshShading(typeA, patchesA, nPatchesA,
				    funcsA, nFuncsA);
  if (!shading->init(dict)) {
    delete shading;
    return NULL;
  }
  return shading;

 err2:
  obj1.free();
 err1:
  return NULL;
}

GfxShading *GfxPatchMeshShading::copy() {
  return new GfxPatchMeshShading(this);
}

//------------------------------------------------------------------------
// GfxImageColorMap
//------------------------------------------------------------------------

GfxImageColorMap::GfxImageColorMap(int bitsA, Object *decode,
				   GfxColorSpace *colorSpaceA) {
  GfxIndexedColorSpace *indexedCS;
  GfxSeparationColorSpace *sepCS;
  int maxPixel, indexHigh;
  Guchar *lookup2;
  Function *sepFunc;
  Object obj;
  double x[gfxColorMaxComps];
  double y[gfxColorMaxComps];
  int i, j, k, byte;
  double mapped;

  ok = gTrue;

  // bits per component and color space
  bits = bitsA;
  maxPixel = (1 << bits) - 1;
  colorSpace = colorSpaceA;

  // get decode map
  if (decode->isNull()) {
    nComps = colorSpace->getNComps();
    colorSpace->getDefaultRanges(decodeLow, decodeRange, maxPixel);
  } else if (decode->isArray()) {
    nComps = decode->arrayGetLength() / 2;
    if (nComps != colorSpace->getNComps()) {
      goto err1;
    }
    for (i = 0; i < nComps; ++i) {
      decode->arrayGet(2*i, &obj);
      if (!obj.isNum()) {
	goto err2;
      }
      decodeLow[i] = obj.getNum();
      obj.free();
      decode->arrayGet(2*i+1, &obj);
      if (!obj.isNum()) {
	goto err2;
      }
      decodeRange[i] = obj.getNum() - decodeLow[i];
      obj.free();
    }
  } else {
    goto err1;
  }

  // Construct a lookup table -- this stores pre-computed decoded
  // values for each component, i.e., the result of applying the
  // decode mapping to each possible image pixel component value.
  //
  // Optimization: for Indexed and Separation color spaces (which have
  // only one component), we store color values in the lookup table
  // rather than component values.
  for (k = 0; k < gfxColorMaxComps; ++k) {
    lookup[k] = NULL;
  }
  colorSpace2 = NULL;
  nComps2 = 0;
  if (colorSpace->getMode() == csIndexed) {
    // Note that indexHigh may not be the same as maxPixel --
    // Distiller will remove unused palette entries, resulting in
    // indexHigh < maxPixel.
    indexedCS = (GfxIndexedColorSpace *)colorSpace;
    colorSpace2 = indexedCS->getBase();
    indexHigh = indexedCS->getIndexHigh();
    nComps2 = colorSpace2->getNComps();
    lookup2 = indexedCS->getLookup();
    colorSpace2->getDefaultRanges(x, y, indexHigh);
    byte_lookup = (Guchar *)gmalloc ((maxPixel + 1) * nComps2);
    for (k = 0; k < nComps2; ++k) {
      lookup[k] = (GfxColorComp *)gmallocn(maxPixel + 1,
					   sizeof(GfxColorComp));
      for (i = 0; i <= maxPixel; ++i) {
	j = (int)(decodeLow[0] + (i * decodeRange[0]) / maxPixel + 0.5);
	if (j < 0) {
	  j = 0;
	} else if (j > indexHigh) {
	  j = indexHigh;
	}

	mapped = x[k] + (lookup2[j*nComps2 + k] / 255.0) * y[k];
	lookup[k][i] = dblToCol(mapped);
	byte_lookup[i * nComps2 + k] = (Guchar) (mapped * 255);
      }
    }
  } else if (colorSpace->getMode() == csSeparation) {
    sepCS = (GfxSeparationColorSpace *)colorSpace;
    colorSpace2 = sepCS->getAlt();
    nComps2 = colorSpace2->getNComps();
    sepFunc = sepCS->getFunc();
    byte_lookup = (Guchar *)gmallocn ((maxPixel + 1), nComps2);
    for (k = 0; k < nComps2; ++k) {
      lookup[k] = (GfxColorComp *)gmallocn(maxPixel + 1,
					   sizeof(GfxColorComp));
      for (i = 0; i <= maxPixel; ++i) {
	x[0] = decodeLow[0] + (i * decodeRange[0]) / maxPixel;
	sepFunc->transform(x, y);
	lookup[k][i] = dblToCol(y[k]);
	byte_lookup[i*nComps2 + k] = (Guchar) (y[k] * 255);
      }
    }
  } else {
    byte_lookup = (Guchar *)gmallocn ((maxPixel + 1), nComps);
    for (k = 0; k < nComps; ++k) {
      lookup[k] = (GfxColorComp *)gmallocn(maxPixel + 1,
					   sizeof(GfxColorComp));
      for (i = 0; i <= maxPixel; ++i) {
	mapped = decodeLow[k] + (i * decodeRange[k]) / maxPixel;
	lookup[k][i] = dblToCol(mapped);
	byte = (int) (mapped * 255.0 + 0.5);
	if (byte < 0)  
	  byte = 0;  
	else if (byte > 255)  
	  byte = 255;  
	byte_lookup[i * nComps + k] = byte;	
      }
    }
  }

  return;

 err2:
  obj.free();
 err1:
  ok = gFalse;
  for (k = 0; k < gfxColorMaxComps; ++k) {
    lookup[k] = NULL;
  }
  byte_lookup = NULL;
}

GfxImageColorMap::GfxImageColorMap(GfxImageColorMap *colorMap) {
  int n, i, k;

  colorSpace = colorMap->colorSpace->copy();
  bits = colorMap->bits;
  nComps = colorMap->nComps;
  nComps2 = colorMap->nComps2;
  colorSpace2 = NULL;
  for (k = 0; k < gfxColorMaxComps; ++k) {
    lookup[k] = NULL;
  }
  n = 1 << bits;
  if (colorSpace->getMode() == csIndexed) {
    colorSpace2 = ((GfxIndexedColorSpace *)colorSpace)->getBase();
    for (k = 0; k < nComps2; ++k) {
      lookup[k] = (GfxColorComp *)gmallocn(n, sizeof(GfxColorComp));
      memcpy(lookup[k], colorMap->lookup[k], n * sizeof(GfxColorComp));
    }
  } else if (colorSpace->getMode() == csSeparation) {
    colorSpace2 = ((GfxSeparationColorSpace *)colorSpace)->getAlt();
    for (k = 0; k < nComps2; ++k) {
      lookup[k] = (GfxColorComp *)gmallocn(n, sizeof(GfxColorComp));
      memcpy(lookup[k], colorMap->lookup[k], n * sizeof(GfxColorComp));
    }
  } else {
    for (k = 0; k < nComps; ++k) {
      lookup[k] = (GfxColorComp *)gmallocn(n, sizeof(GfxColorComp));
      memcpy(lookup[k], colorMap->lookup[k], n * sizeof(GfxColorComp));
    }
  }
  for (i = 0; i < nComps; ++i) {
    decodeLow[i] = colorMap->decodeLow[i];
    decodeRange[i] = colorMap->decodeRange[i];
  }
  ok = gTrue;
}

GfxImageColorMap::~GfxImageColorMap() {
  int i;

  delete colorSpace;
  for (i = 0; i < gfxColorMaxComps; ++i) {
    gfree(lookup[i]);
  }
  gfree(byte_lookup);
}

void GfxImageColorMap::getGray(Guchar *x, GfxGray *gray) {
  GfxColor color;
  int i;

  if (colorSpace2) {
    for (i = 0; i < nComps2; ++i) {
      color.c[i] = lookup[i][x[0]];
    }
    colorSpace2->getGray(&color, gray);
  } else {
    for (i = 0; i < nComps; ++i) {
      color.c[i] = lookup[i][x[i]];
    }
    colorSpace->getGray(&color, gray);
  }
}

void GfxImageColorMap::getRGB(Guchar *x, GfxRGB *rgb) {
  GfxColor color;
  int i;

  if (colorSpace2) {
    for (i = 0; i < nComps2; ++i) {
      color.c[i] = lookup[i][x[0]];
    }
    colorSpace2->getRGB(&color, rgb);
  } else {
    for (i = 0; i < nComps; ++i) {
      color.c[i] = lookup[i][x[i]];
    }
    colorSpace->getRGB(&color, rgb);
  }
}

void GfxImageColorMap::getGrayLine(Guchar *in, Guchar *out, int length) {
  int i, j;
  Guchar *inp, *tmp_line;

  switch (colorSpace->getMode()) {
  case csIndexed:
  case csSeparation:
    tmp_line = (Guchar *) gmalloc (length * nComps2);
    for (i = 0; i < length; i++) {
      for (j = 0; j < nComps2; j++) {
	tmp_line[i * nComps2 + j] = byte_lookup[in[i] * nComps2 + j];
      }
    }
    colorSpace2->getGrayLine(tmp_line, out, length);
    gfree (tmp_line);
    break;

  default:
    inp = in;
    for (j = 0; j < length; j++)
      for (i = 0; i < nComps; i++) {
	*inp = byte_lookup[*inp * nComps + i];
	inp++;
      }
    colorSpace->getGrayLine(in, out, length);
    break;
  }
}

void GfxImageColorMap::getRGBLine(Guchar *in, unsigned int *out, int length) {
  int i, j;
  Guchar *inp, *tmp_line;

  switch (colorSpace->getMode()) {
  case csIndexed:
  case csSeparation:
    tmp_line = (Guchar *) gmalloc (length * nComps2);
    for (i = 0; i < length; i++) {
      for (j = 0; j < nComps2; j++) {
	tmp_line[i * nComps2 + j] = byte_lookup[in[i] * nComps2 + j];
      }
    }
    colorSpace2->getRGBLine(tmp_line, out, length);
    gfree (tmp_line);
    break;

  default:
    inp = in;
    for (j = 0; j < length; j++)
      for (i = 0; i < nComps; i++) {
	*inp = byte_lookup[*inp * nComps + i];
	inp++;
      }
    colorSpace->getRGBLine(in, out, length);
    break;
  }

}

void GfxImageColorMap::getCMYK(Guchar *x, GfxCMYK *cmyk) {
  GfxColor color;
  int i;

  if (colorSpace2) {
    for (i = 0; i < nComps2; ++i) {
      color.c[i] = lookup[i][x[0]];
    }
    colorSpace2->getCMYK(&color, cmyk);
  } else {
    for (i = 0; i < nComps; ++i) {
      color.c[i] = lookup[i][x[i]];
    }
    colorSpace->getCMYK(&color, cmyk);
  }
}

void GfxImageColorMap::getColor(Guchar *x, GfxColor *color) {
  int maxPixel, i;

  maxPixel = (1 << bits) - 1;
  for (i = 0; i < nComps; ++i) {
    color->c[i] = dblToCol(decodeLow[i] + (x[i] * decodeRange[i]) / maxPixel);
  }
}

//------------------------------------------------------------------------
// GfxSubpath and GfxPath
//------------------------------------------------------------------------

GfxSubpath::GfxSubpath(double x1, double y1) {
  size = 16;
  x = (double *)gmallocn(size, sizeof(double));
  y = (double *)gmallocn(size, sizeof(double));
  curve = (GBool *)gmallocn(size, sizeof(GBool));
  n = 1;
  x[0] = x1;
  y[0] = y1;
  curve[0] = gFalse;
  closed = gFalse;
}

GfxSubpath::~GfxSubpath() {
  gfree(x);
  gfree(y);
  gfree(curve);
}

// Used for copy().
GfxSubpath::GfxSubpath(GfxSubpath *subpath) {
  size = subpath->size;
  n = subpath->n;
  x = (double *)gmallocn(size, sizeof(double));
  y = (double *)gmallocn(size, sizeof(double));
  curve = (GBool *)gmallocn(size, sizeof(GBool));
  memcpy(x, subpath->x, n * sizeof(double));
  memcpy(y, subpath->y, n * sizeof(double));
  memcpy(curve, subpath->curve, n * sizeof(GBool));
  closed = subpath->closed;
}

void GfxSubpath::lineTo(double x1, double y1) {
  if (n >= size) {
    size += 16;
    x = (double *)greallocn(x, size, sizeof(double));
    y = (double *)greallocn(y, size, sizeof(double));
    curve = (GBool *)greallocn(curve, size, sizeof(GBool));
  }
  x[n] = x1;
  y[n] = y1;
  curve[n] = gFalse;
  ++n;
}

void GfxSubpath::curveTo(double x1, double y1, double x2, double y2,
			 double x3, double y3) {
  if (n+3 > size) {
    size += 16;
    x = (double *)greallocn(x, size, sizeof(double));
    y = (double *)greallocn(y, size, sizeof(double));
    curve = (GBool *)greallocn(curve, size, sizeof(GBool));
  }
  x[n] = x1;
  y[n] = y1;
  x[n+1] = x2;
  y[n+1] = y2;
  x[n+2] = x3;
  y[n+2] = y3;
  curve[n] = curve[n+1] = gTrue;
  curve[n+2] = gFalse;
  n += 3;
}

void GfxSubpath::close() {
  if (x[n-1] != x[0] || y[n-1] != y[0]) {
    lineTo(x[0], y[0]);
  }
  closed = gTrue;
}

void GfxSubpath::offset(double dx, double dy) {
  int i;

  for (i = 0; i < n; ++i) {
    x[i] += dx;
    y[i] += dy;
  }
}

GfxPath::GfxPath() {
  justMoved = gFalse;
  size = 16;
  n = 0;
  firstX = firstY = 0;
  subpaths = (GfxSubpath **)gmallocn(size, sizeof(GfxSubpath *));
}

GfxPath::~GfxPath() {
  int i;

  for (i = 0; i < n; ++i)
    delete subpaths[i];
  gfree(subpaths);
}

// Used for copy().
GfxPath::GfxPath(GBool justMoved1, double firstX1, double firstY1,
		 GfxSubpath **subpaths1, int n1, int size1) {
  int i;

  justMoved = justMoved1;
  firstX = firstX1;
  firstY = firstY1;
  size = size1;
  n = n1;
  subpaths = (GfxSubpath **)gmallocn(size, sizeof(GfxSubpath *));
  for (i = 0; i < n; ++i)
    subpaths[i] = subpaths1[i]->copy();
}

void GfxPath::moveTo(double x, double y) {
  justMoved = gTrue;
  firstX = x;
  firstY = y;
}

void GfxPath::lineTo(double x, double y) {
  if (justMoved) {
    if (n >= size) {
      size += 16;
      subpaths = (GfxSubpath **)
	           greallocn(subpaths, size, sizeof(GfxSubpath *));
    }
    subpaths[n] = new GfxSubpath(firstX, firstY);
    ++n;
    justMoved = gFalse;
  }
  subpaths[n-1]->lineTo(x, y);
}

void GfxPath::curveTo(double x1, double y1, double x2, double y2,
	     double x3, double y3) {
  if (justMoved) {
    if (n >= size) {
      size += 16;
      subpaths = (GfxSubpath **) 
 	         greallocn(subpaths, size, sizeof(GfxSubpath *));
    }
    subpaths[n] = new GfxSubpath(firstX, firstY);
    ++n;
    justMoved = gFalse;
  }
  subpaths[n-1]->curveTo(x1, y1, x2, y2, x3, y3);
}

void GfxPath::close() {
  // this is necessary to handle the pathological case of
  // moveto/closepath/clip, which defines an empty clipping region
  if (justMoved) {
    if (n >= size) {
      size += 16;
      subpaths = (GfxSubpath **)
	greallocn(subpaths, size, sizeof(GfxSubpath *));
    }
    subpaths[n] = new GfxSubpath(firstX, firstY);
    ++n;
    justMoved = gFalse;
  }
  subpaths[n-1]->close();
}

void GfxPath::append(GfxPath *path) {
  int i;

  if (n + path->n > size) {
    size = n + path->n;
    subpaths = (GfxSubpath **)
                 greallocn(subpaths, size, sizeof(GfxSubpath *));
  }
  for (i = 0; i < path->n; ++i) {
    subpaths[n++] = path->subpaths[i]->copy();
  }
  justMoved = gFalse;
}

void GfxPath::offset(double dx, double dy) {
  int i;

  for (i = 0; i < n; ++i) {
    subpaths[i]->offset(dx, dy);
  }
}

//------------------------------------------------------------------------
// GfxState
//------------------------------------------------------------------------

GfxState::GfxState(double hDPI, double vDPI, PDFRectangle *pageBox,
		   int rotateA, GBool upsideDown) {
  double kx, ky;

  rotate = rotateA;
  px1 = pageBox->x1;
  py1 = pageBox->y1;
  px2 = pageBox->x2;
  py2 = pageBox->y2;
  kx = hDPI / 72.0;
  ky = vDPI / 72.0;
  if (rotate == 90) {
    ctm[0] = 0;
    ctm[1] = upsideDown ? ky : -ky;
    ctm[2] = kx;
    ctm[3] = 0;
    ctm[4] = -kx * py1;
    ctm[5] = ky * (upsideDown ? -px1 : px2);
    pageWidth = kx * (py2 - py1);
    pageHeight = ky * (px2 - px1);
  } else if (rotate == 180) {
    ctm[0] = -kx;
    ctm[1] = 0;
    ctm[2] = 0;
    ctm[3] = upsideDown ? ky : -ky;
    ctm[4] = kx * px2;
    ctm[5] = ky * (upsideDown ? -py1 : py2);
    pageWidth = kx * (px2 - px1);
    pageHeight = ky * (py2 - py1);
  } else if (rotate == 270) {
    ctm[0] = 0;
    ctm[1] = upsideDown ? -ky : ky;
    ctm[2] = -kx;
    ctm[3] = 0;
    ctm[4] = kx * py2;
    ctm[5] = ky * (upsideDown ? px2 : -px1);
    pageWidth = kx * (py2 - py1);
    pageHeight = ky * (px2 - px1);
  } else {
    ctm[0] = kx;
    ctm[1] = 0;
    ctm[2] = 0;
    ctm[3] = upsideDown ? -ky : ky;
    ctm[4] = -kx * px1;
    ctm[5] = ky * (upsideDown ? py2 : -py1);
    pageWidth = kx * (px2 - px1);
    pageHeight = ky * (py2 - py1);
  }

  fillColorSpace = new GfxDeviceGrayColorSpace();
  strokeColorSpace = new GfxDeviceGrayColorSpace();
  fillColor.c[0] = 0;
  strokeColor.c[0] = 0;
  fillPattern = NULL;
  strokePattern = NULL;
  blendMode = gfxBlendNormal;
  fillOpacity = 1;
  strokeOpacity = 1;
  fillOverprint = gFalse;
  strokeOverprint = gFalse;

  lineWidth = 1;
  lineDash = NULL;
  lineDashLength = 0;
  lineDashStart = 0;
  flatness = 1;
  lineJoin = 0;
  lineCap = 0;
  miterLimit = 10;

  font = NULL;
  fontSize = 0;
  textMat[0] = 1; textMat[1] = 0;
  textMat[2] = 0; textMat[3] = 1;
  textMat[4] = 0; textMat[5] = 0;
  charSpace = 0;
  wordSpace = 0;
  horizScaling = 1;
  leading = 0;
  rise = 0;
  render = 0;

  path = new GfxPath();
  curX = curY = 0;
  lineX = lineY = 0;

  clipXMin = 0;
  clipYMin = 0;
  clipXMax = pageWidth;
  clipYMax = pageHeight;

  saved = NULL;
}

GfxState::~GfxState() {
  if (fillColorSpace) {
    delete fillColorSpace;
  }
  if (strokeColorSpace) {
    delete strokeColorSpace;
  }
  if (fillPattern) {
    delete fillPattern;
  }
  if (strokePattern) {
    delete strokePattern;
  }
  gfree(lineDash);
  if (path) {
    // this gets set to NULL by restore()
    delete path;
  }
  if (saved) {
    delete saved;
  }
  if (font) {
    font->decRefCnt();
  }
}

// Used for copy();
GfxState::GfxState(GfxState *state) {
  memcpy(this, state, sizeof(GfxState));
  if (fillColorSpace) {
    fillColorSpace = state->fillColorSpace->copy();
  }
  if (strokeColorSpace) {
    strokeColorSpace = state->strokeColorSpace->copy();
  }
  if (fillPattern) {
    fillPattern = state->fillPattern->copy();
  }
  if (strokePattern) {
    strokePattern = state->strokePattern->copy();
  }
  if (lineDashLength > 0) {
    lineDash = (double *)gmallocn(lineDashLength, sizeof(double));
    memcpy(lineDash, state->lineDash, lineDashLength * sizeof(double));
  }
  if (font)
    font->incRefCnt();

  saved = NULL;
}

void GfxState::setPath(GfxPath *pathA) {
  delete path;
  path = pathA;
}

void GfxState::getUserClipBBox(double *xMin, double *yMin,
			       double *xMax, double *yMax) {
  double ictm[6];
  double xMin1, yMin1, xMax1, yMax1, det, tx, ty;

  // invert the CTM
  det = 1 / (ctm[0] * ctm[3] - ctm[1] * ctm[2]);
  ictm[0] = ctm[3] * det;
  ictm[1] = -ctm[1] * det;
  ictm[2] = -ctm[2] * det;
  ictm[3] = ctm[0] * det;
  ictm[4] = (ctm[2] * ctm[5] - ctm[3] * ctm[4]) * det;
  ictm[5] = (ctm[1] * ctm[4] - ctm[0] * ctm[5]) * det;

  // transform all four corners of the clip bbox; find the min and max
  // x and y values
  xMin1 = xMax1 = clipXMin * ictm[0] + clipYMin * ictm[2] + ictm[4];
  yMin1 = yMax1 = clipXMin * ictm[1] + clipYMin * ictm[3] + ictm[5];
  tx = clipXMin * ictm[0] + clipYMax * ictm[2] + ictm[4];
  ty = clipXMin * ictm[1] + clipYMax * ictm[3] + ictm[5];
  if (tx < xMin1) {
    xMin1 = tx;
  } else if (tx > xMax1) {
    xMax1 = tx;
  }
  if (ty < yMin1) {
    yMin1 = ty;
  } else if (ty > yMax1) {
    yMax1 = ty;
  }
  tx = clipXMax * ictm[0] + clipYMin * ictm[2] + ictm[4];
  ty = clipXMax * ictm[1] + clipYMin * ictm[3] + ictm[5];
  if (tx < xMin1) {
    xMin1 = tx;
  } else if (tx > xMax1) {
    xMax1 = tx;
  }
  if (ty < yMin1) {
    yMin1 = ty;
  } else if (ty > yMax1) {
    yMax1 = ty;
  }
  tx = clipXMax * ictm[0] + clipYMax * ictm[2] + ictm[4];
  ty = clipXMax * ictm[1] + clipYMax * ictm[3] + ictm[5];
  if (tx < xMin1) {
    xMin1 = tx;
  } else if (tx > xMax1) {
    xMax1 = tx;
  }
  if (ty < yMin1) {
    yMin1 = ty;
  } else if (ty > yMax1) {
    yMax1 = ty;
  }

  *xMin = xMin1;
  *yMin = yMin1;
  *xMax = xMax1;
  *yMax = yMax1;
}

double GfxState::transformWidth(double w) {
  double x, y;

  x = ctm[0] + ctm[2];
  y = ctm[1] + ctm[3];
  return w * sqrt(0.5 * (x * x + y * y));
}

double GfxState::getTransformedFontSize() {
  double x1, y1, x2, y2;

  x1 = textMat[2] * fontSize;
  y1 = textMat[3] * fontSize;
  x2 = ctm[0] * x1 + ctm[2] * y1;
  y2 = ctm[1] * x1 + ctm[3] * y1;
  return sqrt(x2 * x2 + y2 * y2);
}

void GfxState::getFontTransMat(double *m11, double *m12,
			       double *m21, double *m22) {
  *m11 = (textMat[0] * ctm[0] + textMat[1] * ctm[2]) * fontSize;
  *m12 = (textMat[0] * ctm[1] + textMat[1] * ctm[3]) * fontSize;
  *m21 = (textMat[2] * ctm[0] + textMat[3] * ctm[2]) * fontSize;
  *m22 = (textMat[2] * ctm[1] + textMat[3] * ctm[3]) * fontSize;
}

void GfxState::setCTM(double a, double b, double c,
		      double d, double e, double f) {
  int i;

  ctm[0] = a;
  ctm[1] = b;
  ctm[2] = c;
  ctm[3] = d;
  ctm[4] = e;
  ctm[5] = f;

  // avoid FP exceptions on badly messed up PDF files
  for (i = 0; i < 6; ++i) {
    if (ctm[i] > 1e10) {
      ctm[i] = 1e10;
    } else if (ctm[i] < -1e10) {
      ctm[i] = -1e10;
    }
  }
}

void GfxState::concatCTM(double a, double b, double c,
			 double d, double e, double f) {
  double a1 = ctm[0];
  double b1 = ctm[1];
  double c1 = ctm[2];
  double d1 = ctm[3];
  int i;

  ctm[0] = a * a1 + b * c1;
  ctm[1] = a * b1 + b * d1;
  ctm[2] = c * a1 + d * c1;
  ctm[3] = c * b1 + d * d1;
  ctm[4] = e * a1 + f * c1 + ctm[4];
  ctm[5] = e * b1 + f * d1 + ctm[5];

  // avoid FP exceptions on badly messed up PDF files
  for (i = 0; i < 6; ++i) {
    if (ctm[i] > 1e10) {
      ctm[i] = 1e10;
    } else if (ctm[i] < -1e10) {
      ctm[i] = -1e10;
    }
  }
}

void GfxState::setFillColorSpace(GfxColorSpace *colorSpace) {
  if (fillColorSpace) {
    delete fillColorSpace;
  }
  fillColorSpace = colorSpace;
}

void GfxState::setStrokeColorSpace(GfxColorSpace *colorSpace) {
  if (strokeColorSpace) {
    delete strokeColorSpace;
  }
  strokeColorSpace = colorSpace;
}

void GfxState::setFillPattern(GfxPattern *pattern) {
  if (fillPattern) {
    delete fillPattern;
  }
  fillPattern = pattern;
}

void GfxState::setStrokePattern(GfxPattern *pattern) {
  if (strokePattern) {
    delete strokePattern;
  }
  strokePattern = pattern;
}

void GfxState::setFont(GfxFont *fontA, double fontSizeA) {
  if (font)
    font->decRefCnt();

  font = fontA;
  fontSize = fontSizeA;
}

void GfxState::setLineDash(double *dash, int length, double start) {
  if (lineDash)
    gfree(lineDash);
  lineDash = dash;
  lineDashLength = length;
  lineDashStart = start;
}

void GfxState::clearPath() {
  delete path;
  path = new GfxPath();
}

void GfxState::clip() {
  double xMin, yMin, xMax, yMax, x, y;
  GfxSubpath *subpath;
  int i, j;

  xMin = xMax = yMin = yMax = 0; // make gcc happy
  for (i = 0; i < path->getNumSubpaths(); ++i) {
    subpath = path->getSubpath(i);
    for (j = 0; j < subpath->getNumPoints(); ++j) {
      transform(subpath->getX(j), subpath->getY(j), &x, &y);
      if (i == 0 && j == 0) {
	xMin = xMax = x;
	yMin = yMax = y;
      } else {
	if (x < xMin) {
	  xMin = x;
	} else if (x > xMax) {
	  xMax = x;
	}
	if (y < yMin) {
	  yMin = y;
	} else if (y > yMax) {
	  yMax = y;
	}
      }
    }
  }
  if (xMin > clipXMin) {
    clipXMin = xMin;
  }
  if (yMin > clipYMin) {
    clipYMin = yMin;
  }
  if (xMax < clipXMax) {
    clipXMax = xMax;
  }
  if (yMax < clipYMax) {
    clipYMax = yMax;
  }
}

void GfxState::textShift(double tx, double ty) {
  double dx, dy;

  textTransformDelta(tx, ty, &dx, &dy);
  curX += dx;
  curY += dy;
}

void GfxState::shift(double dx, double dy) {
  curX += dx;
  curY += dy;
}

GfxState *GfxState::save() {
  GfxState *newState;

  newState = copy();
  newState->saved = this;
  return newState;
}

GfxState *GfxState::restore() {
  GfxState *oldState;

  if (saved) {
    oldState = saved;

    // these attributes aren't saved/restored by the q/Q operators
    oldState->path = path;
    oldState->curX = curX;
    oldState->curY = curY;
    oldState->lineX = lineX;
    oldState->lineY = lineY;

    path = NULL;
    saved = NULL;
    delete this;

  } else {
    oldState = this;
  }

  return oldState;
}

GBool GfxState::parseBlendMode(Object *obj, GfxBlendMode *mode) {
  Object obj2;
  int i, j;

  if (obj->isName()) {
    for (i = 0; i < nGfxBlendModeNames; ++i) {
      if (!strcmp(obj->getNameC(), gfxBlendModeNames[i].name)) {
	*mode = gfxBlendModeNames[i].mode;
	return gTrue;
      }
    }
    return gFalse;
  } else if (obj->isArray()) {
    for (i = 0; i < obj->arrayGetLength(); ++i) {
      obj->arrayGet(i, &obj2);
      if (!obj2.isName()) {
	obj2.free();
	return gFalse;
      }
      for (j = 0; j < nGfxBlendModeNames; ++j) {
	if (!strcmp(obj2.getNameC(), gfxBlendModeNames[j].name)) {
	  obj2.free();
	  *mode = gfxBlendModeNames[j].mode;
	  return gTrue;
	}
      }
      obj2.free();
    }
    *mode = gfxBlendNormal;
    return gTrue;
  } else {
    return gFalse;
  }
}
