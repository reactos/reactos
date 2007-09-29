//========================================================================
//
// SplashFont.h
//
//========================================================================

#ifndef SPLASHFONT_H
#define SPLASHFONT_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "goo/gtypes.h"
#include "SplashTypes.h"

struct SplashGlyphBitmap;
struct SplashFontCacheTag;
class SplashFontFile;
class SplashPath;

//------------------------------------------------------------------------

// Fractional positioning uses this many bits to the right of the
// decimal points.
#define splashFontFractionBits 2
#define splashFontFraction     (1 << splashFontFractionBits)
#define splashFontFractionMul \
                       ((SplashCoord)1 / (SplashCoord)splashFontFraction)

//------------------------------------------------------------------------
// SplashFont
//------------------------------------------------------------------------

class SplashFont {
public:

  SplashFont(SplashFontFile *fontFileA, SplashCoord *matA, GBool aaA);

  // This must be called after the constructor, so that the subclass
  // constructor has a chance to compute the bbox.
  void initCache();

  virtual ~SplashFont();

  SplashFontFile *getFontFile() { return fontFile; }

  // Return true if <this> matches the specified font file and matrix.
  GBool matches(SplashFontFile *fontFileA, SplashCoord *matA) {
    return fontFileA == fontFile &&
           matA[0] == mat[0] && matA[1] == mat[1] &&
           matA[2] == mat[2] && matA[3] == mat[3];
  }

  // Get a glyph - this does a cache lookup first, and if not found,
  // creates a new bitmap and adds it to the cache.  The <xFrac> and
  // <yFrac> values are splashFontFractionBits bits each, representing
  // the numerators of fractions in [0, 1), where the denominator is
  // splashFontFraction = 1 << splashFontFractionBits.  Subclasses
  // should override this to zero out xFrac and/or yFrac if they don't
  // support fractional coordinates.
  virtual GBool getGlyph(int c, int xFrac, int yFrac,
			 SplashGlyphBitmap *bitmap);

  // Rasterize a glyph.  The <xFrac> and <yFrac> values are the same
  // as described for getGlyph.
  virtual GBool makeGlyph(int c, int xFrac, int yFrac,
			  SplashGlyphBitmap *bitmap) = 0;

  // Return the path for a glyph.
  virtual SplashPath *getGlyphPath(int c) = 0;

  // Return the font transform matrix.
  SplashCoord *getMatrix() { return mat; }

  // Return the glyph bounding box.
  void getBBox(int *xMinA, int *yMinA, int *xMaxA, int *yMaxA)
    { *xMinA = xMin; *yMinA = yMin; *xMaxA = xMax; *yMaxA = yMax; }

protected:

  SplashFontFile *fontFile;
  SplashCoord mat[4];		// font transform matrix
  GBool aa;			// anti-aliasing
  int xMin, yMin, xMax, yMax;	// glyph bounding box
  Guchar *cache;		// glyph bitmap cache
  SplashFontCacheTag *		// cache tags
    cacheTags;
  int glyphW, glyphH;		// size of glyph bitmaps
  int glyphSize;		// size of glyph bitmaps, in bytes
  int cacheSets;		// number of sets in cache
  int cacheAssoc;		// cache associativity (glyphs per set)
};

#endif
