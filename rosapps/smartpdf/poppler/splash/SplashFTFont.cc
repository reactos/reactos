//========================================================================
//
// SplashFTFont.cc
//
//========================================================================

#include <config.h>

#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#define MAKE_VERSION( a,b,c ) (((a) << 16) | ((b) << 8) | (c))

#define FREETYPE_VERSION \
	MAKE_VERSION(FREETYPE_MAJOR,FREETYPE_MINOR,FREETYPE_PATCH)

#include <ft2build.h>
#include FT_OUTLINE_H
#include FT_SIZES_H
#include FT_GLYPH_H
#include "goo/gmem.h"
#include "SplashMath.h"
#include "SplashGlyphBitmap.h"
#include "SplashPath.h"
#include "SplashFTFontEngine.h"
#include "SplashFTFontFile.h"
#include "SplashFTFont.h"

//------------------------------------------------------------------------

#if ( FREETYPE_VERSION >= MAKE_VERSION(2,2,0) )
static int glyphPathMoveTo(const FT_Vector *pt, void *path);
static int glyphPathLineTo(const FT_Vector *pt, void *path);
static int glyphPathConicTo(const FT_Vector *ctrl, const FT_Vector *pt, void *path);
static int glyphPathCubicTo(const FT_Vector *ctrl1, const FT_Vector *ctrl2,
			    const FT_Vector *pt, void *path);
#else
static int glyphPathMoveTo(FT_Vector *pt, void *path);
static int glyphPathLineTo(FT_Vector *pt, void *path);
static int glyphPathConicTo(FT_Vector *ctrl, FT_Vector *pt, void *path);
static int glyphPathCubicTo(FT_Vector *ctrl1, FT_Vector *ctrl2,
			    FT_Vector *pt, void *path);
#endif

//------------------------------------------------------------------------
// SplashFTFont
//------------------------------------------------------------------------

SplashFTFont::SplashFTFont(SplashFTFontFile *fontFileA, SplashCoord *matA):
  SplashFont(fontFileA, matA, fontFileA->engine->aa)
{
  FT_Face face;
  double size, div;
  int x, y;

  face = fontFileA->face;
  if (FT_New_Size(face, &sizeObj)) {
    return;
  }
  face->size = sizeObj;
  size = splashSqrt(mat[2]*mat[2] + mat[3]*mat[3]);
  if (FT_Set_Pixel_Sizes(face, 0, (int)size)) {
    return;
  }

  div = face->bbox.xMax > 20000 ? 65536 : 1;

  // transform the four corners of the font bounding box -- the min
  // and max values form the bounding box of the transformed font
  x = (int)((mat[0] * face->bbox.xMin + mat[2] * face->bbox.yMin) /
	    (div * face->units_per_EM));
  xMin = xMax = x;
  y = (int)((mat[1] * face->bbox.xMin + mat[3] * face->bbox.yMin) /
	    (div * face->units_per_EM));
  yMin = yMax = y;
  x = (int)((mat[0] * face->bbox.xMin + mat[2] * face->bbox.yMax) /
	    (div * face->units_per_EM));
  if (x < xMin) {
    xMin = x;
  } else if (x > xMax) {
    xMax = x;
  }
  y = (int)((mat[1] * face->bbox.xMin + mat[3] * face->bbox.yMax) /
	    (div * face->units_per_EM));
  if (y < yMin) {
    yMin = y;
  } else if (y > yMax) {
    yMax = y;
  }
  x = (int)((mat[0] * face->bbox.xMax + mat[2] * face->bbox.yMin) /
	    (div * face->units_per_EM));
  if (x < xMin) {
    xMin = x;
  } else if (x > xMax) {
    xMax = x;
  }
  y = (int)((mat[1] * face->bbox.xMax + mat[3] * face->bbox.yMin) /
	    (div * face->units_per_EM));
  if (y < yMin) {
    yMin = y;
  } else if (y > yMax) {
    yMax = y;
  }
  x = (int)((mat[0] * face->bbox.xMax + mat[2] * face->bbox.yMax) /
	    (div * face->units_per_EM));
  if (x < xMin) {
    xMin = x;
  } else if (x > xMax) {
    xMax = x;
  }
  y = (int)((mat[1] * face->bbox.xMax + mat[3] * face->bbox.yMax) /
	    (div * face->units_per_EM));
  if (y < yMin) {
    yMin = y;
  } else if (y > yMax) {
    yMax = y;
  }
  // This is a kludge: some buggy PDF generators embed fonts with
  // zero bounding boxes.
  if (xMax == xMin) {
    xMin = 0;
    xMax = (int)size;
  }
  if (yMax == yMin) {
    yMin = 0;
    yMax = (int)((SplashCoord)1.2 * size);
  }

  // compute the transform matrix
#if USE_FIXEDPOINT
  matrix.xx = (FT_Fixed)((mat[0] / size).getRaw());
  matrix.yx = (FT_Fixed)((mat[1] / size).getRaw());
  matrix.xy = (FT_Fixed)((mat[2] / size).getRaw());
  matrix.yy = (FT_Fixed)((mat[3] / size).getRaw());
#else
  matrix.xx = (FT_Fixed)((mat[0] / size) * 65536);
  matrix.yx = (FT_Fixed)((mat[1] / size) * 65536);
  matrix.xy = (FT_Fixed)((mat[2] / size) * 65536);
  matrix.yy = (FT_Fixed)((mat[3] / size) * 65536);
#endif
}

SplashFTFont::~SplashFTFont() {
}

GBool SplashFTFont::getGlyph(int c, int xFrac, int yFrac,
			     SplashGlyphBitmap *bitmap) {
  return SplashFont::getGlyph(c, xFrac, 0, bitmap);
}

GBool SplashFTFont::makeGlyph(int c, int xFrac, int yFrac,
			      SplashGlyphBitmap *bitmap) {
  SplashFTFontFile *ff;
  FT_Vector offset;
  FT_GlyphSlot slot;
  FT_UInt gid;
  int rowSize;
  Guchar *p, *q;
  int i;

  ff = (SplashFTFontFile *)fontFile;

  ff->face->size = sizeObj;
  offset.x = (FT_Pos)(int)((SplashCoord)xFrac * splashFontFractionMul * 64);
  offset.y = 0;
  FT_Set_Transform(ff->face, &matrix, &offset);
  slot = ff->face->glyph;

  if (ff->codeToGID && c < ff->codeToGIDLen) {
    gid = (FT_UInt)ff->codeToGID[c];
  } else {
    gid = (FT_UInt)c;
  }

  // if we have the FT2 bytecode interpreter, autohinting won't be used
#ifdef TT_CONFIG_OPTION_BYTECODE_INTERPRETER
  if (FT_Load_Glyph(ff->face, gid,
		    aa ? FT_LOAD_NO_BITMAP : FT_LOAD_DEFAULT)) {
    return gFalse;
  }
#else
  // FT2's autohinting doesn't always work very well (especially with
  // font subsets), so turn it off if anti-aliasing is enabled; if
  // anti-aliasing is disabled, this seems to be a tossup - some fonts
  // look better with hinting, some without, so leave hinting on
  if (FT_Load_Glyph(ff->face, gid,
		    aa ? FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP
                       : FT_LOAD_DEFAULT)) {
    return gFalse;
  }
#endif
  if (FT_Render_Glyph(slot, aa ? ft_render_mode_normal
		               : ft_render_mode_mono)) {
    return gFalse;
  }

  bitmap->x = -slot->bitmap_left;
  bitmap->y = slot->bitmap_top;
  bitmap->w = slot->bitmap.width;
  bitmap->h = slot->bitmap.rows;
  bitmap->aa = aa;
  if (aa) {
    rowSize = bitmap->w;
  } else {
    rowSize = (bitmap->w + 7) >> 3;
  }
  bitmap->data = (Guchar *)gmalloc(rowSize * bitmap->h);
  bitmap->freeData = gTrue;
  for (i = 0, p = bitmap->data, q = slot->bitmap.buffer;
       i < bitmap->h;
       ++i, p += rowSize, q += slot->bitmap.pitch) {
    memcpy(p, q, rowSize);
  }

  return gTrue;
}

struct SplashFTFontPath {
  SplashPath *path;
  GBool needClose;
};

SplashPath *SplashFTFont::getGlyphPath(int c) {
  static FT_Outline_Funcs outlineFuncs = {
    &glyphPathMoveTo,
    &glyphPathLineTo,
    &glyphPathConicTo,
    &glyphPathCubicTo,
    0, 0
  };
  SplashFTFontFile *ff;
  SplashFTFontPath path;
  FT_GlyphSlot slot;
  FT_UInt gid;
  FT_Glyph glyph;

  ff = (SplashFTFontFile *)fontFile;
  ff->face->size = sizeObj;
  FT_Set_Transform(ff->face, &matrix, NULL);
  slot = ff->face->glyph;
  if (ff->codeToGID && c < ff->codeToGIDLen) {
    gid = ff->codeToGID[c];
  } else {
    gid = (FT_UInt)c;
  }
  if (FT_Load_Glyph(ff->face, gid, FT_LOAD_NO_BITMAP)) {
    return NULL;
  }
  if (FT_Get_Glyph(slot, &glyph)) {
    return NULL;
  }
  path.path = new SplashPath();
  path.needClose = gFalse;
  FT_Outline_Decompose(&((FT_OutlineGlyph)glyph)->outline,
		       &outlineFuncs, &path);
  if (path.needClose) {
    path.path->close();
  }
  FT_Done_Glyph(glyph);
  return path.path;
}

#if ( FREETYPE_VERSION >= MAKE_VERSION(2,2,0) )
static int glyphPathMoveTo(const FT_Vector *pt, void *path)
#else
static int glyphPathMoveTo(FT_Vector *pt, void *path)
#endif
{
  SplashFTFontPath *p = (SplashFTFontPath *)path;

  if (p->needClose) {
    p->path->close();
    p->needClose = gFalse;
  }
  p->path->moveTo(pt->x / 64.0, -pt->y / 64.0);
  return 0;
}

#if ( FREETYPE_VERSION >= MAKE_VERSION(2,2,0) )
static int glyphPathLineTo(const FT_Vector *pt, void *path)
#else
static int glyphPathLineTo(FT_Vector *pt, void *path)
#endif
{
  SplashFTFontPath *p = (SplashFTFontPath *)path;

  p->path->lineTo(pt->x / 64.0, -pt->y / 64.0);
  p->needClose = gTrue;
  return 0;
}

#if ( FREETYPE_VERSION >= MAKE_VERSION(2,2,0) )
static int glyphPathConicTo(const FT_Vector *ctrl, const FT_Vector *pt, void *path)
#else
static int glyphPathConicTo(FT_Vector *ctrl, FT_Vector *pt, void *path)
#endif
{
  SplashFTFontPath *p = (SplashFTFontPath *)path;
  SplashCoord x0, y0, x1, y1, x2, y2, x3, y3, xc, yc;

  if (!p->path->getCurPt(&x0, &y0)) {
    return 0;
  }
  xc = ctrl->x / 64.0;
  yc = -ctrl->y / 64.0;
  x3 = pt->x / 64.0;
  y3 = -pt->y / 64.0;

  // A second-order Bezier curve is defined by two endpoints, p0 and
  // p3, and one control point, pc:
  //
  //     p(t) = (1-t)^2*p0 + t*(1-t)*pc + t^2*p3
  //
  // A third-order Bezier curve is defined by the same two endpoints,
  // p0 and p3, and two control points, p1 and p2:
  //
  //     p(t) = (1-t)^3*p0 + 3t*(1-t)^2*p1 + 3t^2*(1-t)*p2 + t^3*p3
  //
  // Applying some algebra, we can convert a second-order curve to a
  // third-order curve:
  //
  //     p1 = (1/3) * (p0 + 2pc)
  //     p2 = (1/3) * (2pc + p3)

  x1 = (SplashCoord)(1.0 / 3.0) * (x0 + (SplashCoord)2 * xc);
  y1 = (SplashCoord)(1.0 / 3.0) * (y0 + (SplashCoord)2 * yc);
  x2 = (SplashCoord)(1.0 / 3.0) * ((SplashCoord)2 * xc + x3);
  y2 = (SplashCoord)(1.0 / 3.0) * ((SplashCoord)2 * yc + y3);

  p->path->curveTo(x1, y1, x2, y2, x3, y3);
  p->needClose = gTrue;
  return 0;
}

#if ( FREETYPE_VERSION >= MAKE_VERSION(2,2,0) )
static int glyphPathCubicTo(const FT_Vector *ctrl1, const FT_Vector *ctrl2, const FT_Vector *pt, void *path)
#else
static int glyphPathCubicTo(FT_Vector *ctrl1, FT_Vector *ctrl2, FT_Vector *pt, void *path)
#endif
{
  SplashFTFontPath *p = (SplashFTFontPath *)path;

  p->path->curveTo(ctrl1->x / 64.0, -ctrl1->y / 64.0,
		   ctrl2->x / 64.0, -ctrl2->y / 64.0,
		   pt->x / 64.0, -pt->y / 64.0);
  p->needClose = gTrue;
  return 0;
}

#endif // HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H
