//========================================================================
//
// SplashFTFont.h
//
//========================================================================

#ifndef SPLASHFTFONT_H
#define SPLASHFTFONT_H

#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include "SplashFont.h"

class SplashFTFontFile;

//------------------------------------------------------------------------
// SplashFTFont
//------------------------------------------------------------------------

class SplashFTFont: public SplashFont {
public:

  SplashFTFont(SplashFTFontFile *fontFileA, SplashCoord *matA);

  virtual ~SplashFTFont();

  // Munge xFrac and yFrac before calling SplashFont::getGlyph.
  virtual GBool getGlyph(int c, int xFrac, int yFrac,
			 SplashGlyphBitmap *bitmap);

  // Rasterize a glyph.  The <xFrac> and <yFrac> values are the same
  // as described for getGlyph.
  virtual GBool makeGlyph(int c, int xFrac, int yFrac,
			  SplashGlyphBitmap *bitmap);

  // Return the path for a glyph.
  virtual SplashPath *getGlyphPath(int c);

private:

  FT_Size sizeObj;
  FT_Matrix matrix;
};

#endif // HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H

#endif
