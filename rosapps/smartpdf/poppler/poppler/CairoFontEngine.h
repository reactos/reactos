//========================================================================
//
// CairoFontEngine.h
//
//========================================================================

#ifndef CAIROFONTENGINE_H
#define CAIROFONTENGINE_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "goo/gtypes.h"
#include <cairo-ft.h>

#include "GfxFont.h"

class CairoFont {
public:
  static CairoFont *create(GfxFont *gfxFont, XRef *xref, FT_Library lib, GBool useCIDs);
  ~CairoFont();

  GBool matches(Ref &other);
  cairo_font_face_t *getFontFace(void);
  unsigned long getGlyph(CharCode code, Unicode *u, int uLen);
private:
  CairoFont(Ref ref, cairo_font_face_t *cairo_font_face, FT_Face face,
      Gushort *codeToGID, int codeToGIDLen);
  Ref ref;
  cairo_font_face_t *cairo_font_face;
  FT_Face face;

  Gushort *codeToGID;
  int codeToGIDLen;
};

//------------------------------------------------------------------------

#define cairoFontCacheSize 64

//------------------------------------------------------------------------
// CairoFontEngine
//------------------------------------------------------------------------

class CairoFontEngine {
public:

  // Create a font engine.
  CairoFontEngine(FT_Library libA);
  ~CairoFontEngine();

  CairoFont *getFont(GfxFont *gfxFont, XRef *xref);

private:
  CairoFont *fontCache[cairoFontCacheSize];
  FT_Library lib;
  GBool useCIDs;
};

#endif
