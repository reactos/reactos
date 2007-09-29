//========================================================================
//
// Splash.h
//
//========================================================================

#ifndef SPLASH_H
#define SPLASH_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "SplashTypes.h"
#include "SplashClip.h"

class SplashBitmap;
struct SplashGlyphBitmap;
class SplashState;
class SplashPattern;
class SplashScreen;
class SplashPath;
class SplashXPath;
class SplashFont;

//------------------------------------------------------------------------

// Retrieves the next line of pixels in an image mask.  Normally,
// fills in *<line> and returns true.  If the image stream is
// exhausted, returns false.
typedef GBool (*SplashImageMaskSource)(void *data, SplashColorPtr pixel);

// Retrieves the next line of pixels in an image.  Normally, fills in
// *<line> and returns true.  If the image stream is exhausted,
// returns false.
typedef GBool (*SplashImageSource)(void *data, SplashColorPtr line);

//------------------------------------------------------------------------
// Splash
//------------------------------------------------------------------------

class Splash {
public:

  // Create a new rasterizer object.
  Splash(SplashBitmap *bitmapA);

  ~Splash();

  //----- state read

  SplashPattern *getStrokePattern();
  SplashPattern *getFillPattern();
  SplashScreen *getScreen();
  SplashBlendFunc getBlendFunc();
  SplashCoord getStrokeAlpha();
  SplashCoord getFillAlpha();
  SplashCoord getLineWidth();
  int getLineCap();
  int getLineJoin();
  SplashCoord getMiterLimit();
  SplashCoord getFlatness();
  SplashCoord *getLineDash();
  int getLineDashLength();
  SplashCoord getLineDashPhase();
  SplashClip *getClip();

  //----- state write

  void setStrokePattern(SplashPattern *strokeColor);
  void setFillPattern(SplashPattern *fillColor);
  void setScreen(SplashScreen *screen);
  void setBlendFunc(SplashBlendFunc func);
  void setStrokeAlpha(SplashCoord alpha);
  void setFillAlpha(SplashCoord alpha);
  void setLineWidth(SplashCoord lineWidth);
  void setLineCap(int lineCap);
  void setLineJoin(int lineJoin);
  void setMiterLimit(SplashCoord miterLimit);
  void setFlatness(SplashCoord flatness);
  // the <lineDash> array will be copied
  void setLineDash(SplashCoord *lineDash, int lineDashLength,
		   SplashCoord lineDashPhase);
  void clipResetToRect(SplashCoord x0, SplashCoord y0,
		       SplashCoord x1, SplashCoord y1);
  SplashError clipToRect(SplashCoord x0, SplashCoord y0,
			 SplashCoord x1, SplashCoord y1);
  SplashError clipToPath(SplashPath *path, GBool eo);

  //----- state save/restore

  void saveState();
  SplashError restoreState();

  //----- soft mask

  void setSoftMask(SplashBitmap *softMaskA);

  //----- drawing operations

  // Fill the bitmap with <color>.  This is not subject to clipping.
  void clear(SplashColorPtr color);

  // Stroke a path using the current stroke pattern.
  SplashError stroke(SplashPath *path);

  // Fill a path using the current fill pattern.
  SplashError fill(SplashPath *path, GBool eo);

  // Fill a path, XORing with the current fill pattern.
  SplashError xorFill(SplashPath *path, GBool eo);

  // Draw a character, using the current fill pattern.
  SplashError fillChar(SplashCoord x, SplashCoord y, int c, SplashFont *font);

  // Draw a glyph, using the current fill pattern.  This function does
  // not free any data, i.e., it ignores glyph->freeData.
  SplashError fillGlyph(SplashCoord x, SplashCoord y,
			SplashGlyphBitmap *glyph);

  // Draws an image mask using the fill color.  This will read <h>
  // lines of <w> pixels from <src>, starting with the top line.  "1"
  // pixels will be drawn with the current fill color; "0" pixels are
  // transparent.  The matrix:
  //    [ mat[0] mat[1] 0 ]
  //    [ mat[2] mat[3] 0 ]
  //    [ mat[4] mat[5] 1 ]
  // maps a unit square to the desired destination for the image, in
  // PostScript style:
  //    [x' y' 1] = [x y 1] * mat
  // Note that the Splash y axis points downward, and the image source
  // is assumed to produce pixels in raster order, starting from the
  // top line.
  SplashError fillImageMask(SplashImageMaskSource src, void *srcData,
			    int w, int h, SplashCoord *mat);

  // Draw an image.  This will read <h> lines of <w> pixels from
  // <src>, starting with the top line.  These pixels are assumed to
  // be in the source mode, <srcMode>.  The following combinations of
  // source and target modes are supported:
  //    source       target
  //    ------       ------
  //    Mono1        Mono1
  //    Mono8        Mono1   -- with dithering
  //    Mono8        Mono8
  //    RGB8         RGB8
  //    BGR8         BGR8
  //    ARGB8        RGB8    -- with source alpha (masking)
  //    BGRA8        BGR8    -- with source alpha (masking)
  // The matrix behaves as for fillImageMask.
  SplashError drawImage(SplashImageSource src, void *srcData,
			SplashColorMode srcMode,
			int w, int h, SplashCoord *mat);

  //----- misc

  // Return the associated bitmap.
  SplashBitmap *getBitmap() { return bitmap; }

  // Get a bounding box which includes all modifications since the
  // last call to clearModRegion.
  void getModRegion(int *xMin, int *yMin, int *xMax, int *yMax)
    { *xMin = modXMin; *yMin = modYMin; *xMax = modXMax; *yMax = modYMax; }

  // Clear the modified region bounding box.
  void clearModRegion();

  // Get clipping status for the last drawing operation subject to
  // clipping.
  SplashClipResult getClipRes() { return opClipRes; }

  // Toggle debug mode on or off.
  void setDebugMode(GBool debugModeA) { debugMode = debugModeA; }

private:

  void updateModX(int x);
  void updateModY(int y);
  void strokeNarrow(SplashXPath *xPath);
  void strokeWide(SplashXPath *xPath);
  SplashXPath *makeDashedPath(SplashXPath *xPath);
  SplashError fillWithPattern(SplashPath *path, GBool eo,
			      SplashPattern *pattern, SplashCoord alpha);
  void drawPixel(int x, int y, SplashColorPtr color,
		 SplashCoord alpha, GBool noClip);
  void drawPixel(int x, int y, SplashPattern *pattern,
		 SplashCoord alpha, GBool noClip);
  void drawSpan(int x0, int x1, int y, SplashPattern *pattern,
		SplashCoord alpha, GBool noClip);
  void xorSpan(int x0, int x1, int y, SplashPattern *pattern, GBool noClip);
  void dumpPath(SplashPath *path);
  void dumpXPath(SplashXPath *path);

  SplashBitmap *bitmap;
  SplashState *state;
  SplashBitmap *softMask;
  int modXMin, modYMin, modXMax, modYMax;
  SplashClipResult opClipRes;
  GBool debugMode;
};

#endif
