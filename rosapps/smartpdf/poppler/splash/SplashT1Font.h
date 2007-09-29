//========================================================================
//
// SplashT1Font.h
//
//========================================================================

#ifndef SPLASHT1FONT_H
#define SPLASHT1FONT_H

#if HAVE_T1LIB_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "SplashFont.h"

class SplashT1FontFile;

//------------------------------------------------------------------------
// SplashT1Font
//------------------------------------------------------------------------

class SplashT1Font: public SplashFont {
public:

  SplashT1Font(SplashT1FontFile *fontFileA, SplashCoord *matA);

  virtual ~SplashT1Font();

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

  int t1libID;			// t1lib font ID
  float size;
};

#endif // HAVE_T1LIB_H

#endif
