//========================================================================
//
// SplashOutputDev.h
//
// Copyright 2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef SPLASHOUTPUTDEV_H
#define SPLASHOUTPUTDEV_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "goo/gtypes.h"
#include "splash/SplashTypes.h"
#include "poppler-config.h"
#include "OutputDev.h"
#include "GfxState.h"

class Gfx8BitFont;
class SplashBitmap;
class Splash;
class SplashPath;
class SplashPattern;
class SplashFontEngine;
class SplashFont;
class T3FontCache;
struct T3FontCacheTag;
struct T3GlyphStack;

//------------------------------------------------------------------------

// number of Type 3 fonts to cache
#define splashOutT3FontCacheSize 8

//------------------------------------------------------------------------
// SplashOutputDev
//------------------------------------------------------------------------

class SplashOutputDev: public OutputDev {
public:

  // Constructor.
  SplashOutputDev(SplashColorMode colorModeA, int bitmapRowPadA,
		  GBool reverseVideoA, SplashColorPtr paperColorA,
		  GBool bitmapTopDownA = gTrue,
		  GBool allowAntialiasA = gTrue);

  // Destructor.
  virtual ~SplashOutputDev();

  //----- get info about output device

  // Does this device use upside-down coordinates?
  // (Upside-down means (0,0) is the top left corner of the page.)
  virtual GBool upsideDown() { return gTrue; }

  // Does this device use drawChar() or drawString()?
  virtual GBool useDrawChar() { return gTrue; }

  // Does this device use beginType3Char/endType3Char?  Otherwise,
  // text in Type 3 fonts will be drawn with drawChar/drawString.
  virtual GBool interpretType3Chars() { return gTrue; }

  //----- initialization and control

  // Start a page.
  virtual void startPage(int pageNum, GfxState *state);

  // End a page.
  virtual void endPage();

  //----- link borders
  virtual void drawLink(Link *link, Catalog *catalog);

  //----- save/restore graphics state
  virtual void saveState(GfxState *state);
  virtual void restoreState(GfxState *state);

  //----- update graphics state
  virtual void updateAll(GfxState *state);
  virtual void updateCTM(GfxState *state, double m11, double m12,
			 double m21, double m22, double m31, double m32);
  virtual void updateLineDash(GfxState *state);
  virtual void updateFlatness(GfxState *state);
  virtual void updateLineJoin(GfxState *state);
  virtual void updateLineCap(GfxState *state);
  virtual void updateMiterLimit(GfxState *state);
  virtual void updateLineWidth(GfxState *state);
  virtual void updateFillColor(GfxState *state);
  virtual void updateStrokeColor(GfxState *state);
  virtual void updateBlendMode(GfxState *state);
  virtual void updateFillOpacity(GfxState *state);
  virtual void updateStrokeOpacity(GfxState *state);

  //----- update text state
  virtual void updateFont(GfxState *state);

  //----- path painting
  virtual void stroke(GfxState *state);
  virtual void fill(GfxState *state);
  virtual void eoFill(GfxState *state);

  //----- path clipping
  virtual void clip(GfxState *state);
  virtual void eoClip(GfxState *state);

  //----- text drawing
  virtual void drawChar(GfxState *state, double x, double y,
			double dx, double dy,
			double originX, double originY,
			CharCode code, int nBytes, Unicode *u, int uLen);
  virtual GBool beginType3Char(GfxState *state, double x, double y,
			       double dx, double dy,
			       CharCode code, Unicode *u, int uLen);
  virtual void endType3Char(GfxState *state);
  virtual void endTextObject(GfxState *state);

  //----- image drawing
  virtual void drawImageMask(GfxState *state, Object *ref, Stream *str,
			     int width, int height, GBool invert,
			     GBool inlineImg);
  virtual void drawImage(GfxState *state, Object *ref, Stream *str,
			 int width, int height, GfxImageColorMap *colorMap,
			 int *maskColors, GBool inlineImg);
  virtual void drawMaskedImage(GfxState *state, Object *ref, Stream *str,
			       int width, int height,
			       GfxImageColorMap *colorMap,
			       Stream *maskStr, int maskWidth, int maskHeight,
			       GBool maskInvert);
  virtual void drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str,
				   int width, int height,
				   GfxImageColorMap *colorMap,
				   Stream *maskStr,
				   int maskWidth, int maskHeight,
				   GfxImageColorMap *maskColorMap);

  //----- Type 3 font operators
  virtual void type3D0(GfxState *state, double wx, double wy);
  virtual void type3D1(GfxState *state, double wx, double wy,
		       double llx, double lly, double urx, double ury);

  //----- special access

  // Called to indicate that a new PDF document has been loaded.
  void startDoc(XRef *xrefA);

  void setPaperColor(SplashColorPtr paperColorA);

  GBool isReverseVideo() { return reverseVideo; }
  void setReverseVideo(GBool reverseVideoA) { reverseVideo = reverseVideoA; }

  // Get the bitmap and its size.
  SplashBitmap *getBitmap() { return bitmap; }
  int getBitmapWidth();
  int getBitmapHeight();

  // Returns the last rasterized bitmap, transferring ownership to the
  // caller.
  SplashBitmap *takeBitmap();

  // Get the Splash object.
  Splash *getSplash() { return splash; }

  // Get the modified region.
  void getModRegion(int *xMin, int *yMin, int *xMax, int *yMax);

  // Clear the modified region.
  void clearModRegion();

  // Set the Splash fill color.
  void setFillColor(int r, int g, int b);

  SplashFont *getCurrentFont() { return font; }

private:

#if SPLASH_CMYK
  SplashPattern *getColor(GfxGray gray, GfxRGB *rgb, GfxCMYK *cmyk);
#else
  SplashPattern *getColor(GfxGray gray, GfxRGB *rgb);
#endif
  SplashPath *convertPath(GfxState *state, GfxPath *path);
  void drawType3Glyph(T3FontCache *t3Font,
		      T3FontCacheTag *tag, Guchar *data,
		      double x, double y);
  static GBool imageMaskSrc(void *data, SplashColorPtr line);
  static GBool imageSrc(void *data, SplashColorPtr line);
  static GBool alphaImageSrc(void *data, SplashColorPtr line);
  static GBool maskedImageSrc(void *data, SplashColorPtr line);

  SplashColorMode colorMode;
  int bitmapRowPad;
  GBool bitmapTopDown;
  GBool allowAntialias;
  GBool reverseVideo;		// reverse video mode
  SplashColor paperColor;	// paper color

  XRef *xref;			// xref table for current document

  SplashBitmap *bitmap;
  Splash *splash;
  SplashFontEngine *fontEngine;

  T3FontCache *			// Type 3 font cache
    t3FontCache[splashOutT3FontCacheSize];
  int nT3Fonts;			// number of valid entries in t3FontCache
  T3GlyphStack *t3GlyphStack;	// Type 3 glyph context stack

  SplashFont *font;		// current font
  GBool needFontUpdate;		// set when the font needs to be updated
  SplashPath *textClipPath;	// clipping path built with text object
};

#endif
