//========================================================================
//
// SplashFont.cc
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <string.h>
#include "goo/gmem.h"
#include "SplashMath.h"
#include "SplashGlyphBitmap.h"
#include "SplashFontFile.h"
#include "SplashFont.h"

//------------------------------------------------------------------------

struct SplashFontCacheTag {
  int c;
  short xFrac, yFrac;		// x and y fractions
  int mru;			// valid bit (0x80000000) and MRU index
  int x, y, w, h;		// offset and size of glyph
};

//------------------------------------------------------------------------
// SplashFont
//------------------------------------------------------------------------

SplashFont::SplashFont(SplashFontFile *fontFileA, SplashCoord *matA,
		       GBool aaA) {
  fontFile = fontFileA;
  fontFile->incRefCnt();
  mat[0] = matA[0];
  mat[1] = matA[1];
  mat[2] = matA[2];
  mat[3] = matA[3];
  aa = aaA;

  cache = NULL;
  cacheTags = NULL;

  xMin = yMin = xMax = yMax = 0;
}

void SplashFont::initCache() {
  int i;

  // this should be (max - min + 1), but we add some padding to
  // deal with rounding errors
  glyphW = xMax - xMin + 3;
  glyphH = yMax - yMin + 3;
  if (aa) {
    glyphSize = glyphW * glyphH;
  } else {
    glyphSize = ((glyphW + 7) >> 3) * glyphH;
  }

  // set up the glyph pixmap cache
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
  cache = (Guchar *)gmallocn(cacheSets* cacheAssoc, glyphSize);
  cacheTags = (SplashFontCacheTag *)gmallocn(cacheSets * cacheAssoc,
					     sizeof(SplashFontCacheTag));
  for (i = 0; i < cacheSets * cacheAssoc; ++i) {
    cacheTags[i].mru = i & (cacheAssoc - 1);
  }
}

SplashFont::~SplashFont() {
  fontFile->decRefCnt();
  if (cache) {
    gfree(cache);
  }
  if (cacheTags) {
    gfree(cacheTags);
  }
}

GBool SplashFont::getGlyph(int c, int xFrac, int yFrac,
			   SplashGlyphBitmap *bitmap) {
  SplashGlyphBitmap bitmap2;
  int size;
  Guchar *p;
  int i, j, k;

  // no fractional coordinates for large glyphs or non-anti-aliased
  // glyphs
  if (!aa || glyphH > 50) {
    xFrac = yFrac = 0;
  }

  // check the cache
  i = (c & (cacheSets - 1)) * cacheAssoc;
  for (j = 0; j < cacheAssoc; ++j) {
    if ((cacheTags[i+j].mru & 0x80000000) &&
	cacheTags[i+j].c == c &&
	(int)cacheTags[i+j].xFrac == xFrac &&
	(int)cacheTags[i+j].yFrac == yFrac) {
      bitmap->x = cacheTags[i+j].x;
      bitmap->y = cacheTags[i+j].y;
      bitmap->w = cacheTags[i+j].w;
      bitmap->h = cacheTags[i+j].h;
      for (k = 0; k < cacheAssoc; ++k) {
	if (k != j &&
	    (cacheTags[i+k].mru & 0x7fffffff) <
	      (cacheTags[i+j].mru & 0x7fffffff)) {
	  ++cacheTags[i+k].mru;
	}
      }
      cacheTags[i+j].mru = 0x80000000;
      bitmap->aa = aa;
      bitmap->data = cache + (i+j) * glyphSize;
      bitmap->freeData = gFalse;
      return gTrue;
    }
  }

  // generate the glyph bitmap
  if (!makeGlyph(c, xFrac, yFrac, &bitmap2)) {
    return gFalse;
  }

  // if the glyph doesn't fit in the bounding box, return a temporary
  // uncached bitmap
  if (bitmap2.w > glyphW || bitmap2.h > glyphH) {
    *bitmap = bitmap2;
    return gTrue;
  }

  // insert glyph pixmap in cache
  if (aa) {
    size = bitmap2.w * bitmap2.h;
  } else {
    size = ((bitmap2.w + 7) >> 3) * bitmap2.h;
  }
  p = NULL; // make gcc happy
  for (j = 0; j < cacheAssoc; ++j) {
    if ((cacheTags[i+j].mru & 0x7fffffff) == cacheAssoc - 1) {
      cacheTags[i+j].mru = 0x80000000;
      cacheTags[i+j].c = c;
      cacheTags[i+j].xFrac = (short)xFrac;
      cacheTags[i+j].yFrac = (short)yFrac;
      cacheTags[i+j].x = bitmap2.x;
      cacheTags[i+j].y = bitmap2.y;
      cacheTags[i+j].w = bitmap2.w;
      cacheTags[i+j].h = bitmap2.h;
      p = cache + (i+j) * glyphSize;
      memcpy(p, bitmap2.data, size);
    } else {
      ++cacheTags[i+j].mru;
    }
  }
  *bitmap = bitmap2;
  bitmap->data = p;
  bitmap->freeData = gFalse;
  if (bitmap2.freeData) {
    gfree(bitmap2.data);
  }
  return gTrue;
}
