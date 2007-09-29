//========================================================================
//
// ArthurOutputDev.cc
//
// Copyright 2003 Glyph & Cog, LLC
// Copyright 2004 Red Hat, Inc
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
#include "GfxState.h"
#include "GfxFont.h"
#include "Link.h"
#include "CharCodeToUnicode.h"
#include "FontEncodingTables.h"
#include <fofi/FoFiTrueType.h>
#include "ArthurOutputDev.h"

#include <QtCore/QtDebug>
#include <QtGui/QPainterPath>
//------------------------------------------------------------------------

#include "splash/SplashFontFileID.h"
#include "splash/SplashFontFile.h"
#include "splash/SplashFontEngine.h"
#include "splash/SplashFont.h"
#include "splash/SplashMath.h"
#include "splash/SplashPath.h"
#include "splash/SplashGlyphBitmap.h"
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
// ArthurOutputDev
//------------------------------------------------------------------------

ArthurOutputDev::ArthurOutputDev(QPainter *painter):
  m_painter(painter)
{
  m_currentBrush = QBrush(Qt::SolidPattern);
  m_fontEngine = 0;
}

ArthurOutputDev::~ArthurOutputDev()
{
}

void ArthurOutputDev::startDoc(XRef *xrefA) {
  int i;

  xref = xrefA;
  if (m_fontEngine) {
    delete m_fontEngine;
  }
  m_fontEngine = new SplashFontEngine(
#if HAVE_T1LIB_H
				    globalParams->getEnableT1lib(),
#endif
#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H
				    globalParams->getEnableFreeType(),
#endif
				    globalParams->getAntialias());
}

void ArthurOutputDev::startPage(int pageNum, GfxState *state)
{
  // fill page with white background.
  int w = static_cast<int>(state->getPageWidth());
  int h = static_cast<int>(state->getPageHeight());
  QColor fillColour(Qt::white);
  QBrush fill(fillColour);
  m_painter->save();
  m_painter->setPen(fillColour);
  m_painter->setBrush(fill);
  m_painter->drawRect(0, 0, w, h);
  m_painter->restore();
}

void ArthurOutputDev::endPage() {
}

void ArthurOutputDev::drawLink(Link *link, Catalog *catalog)
{
}

void ArthurOutputDev::saveState(GfxState *state)
{
  m_painter->save();
}

void ArthurOutputDev::restoreState(GfxState *state)
{
  m_painter->restore();
}

void ArthurOutputDev::updateAll(GfxState *state)
{
  updateLineDash(state);
  updateLineJoin(state);
  updateLineCap(state);
  updateLineWidth(state);
  updateFlatness(state);
  updateMiterLimit(state);
  updateFillColor(state);
  updateStrokeColor(state);
  updateFillOpacity(state);
  updateStrokeOpacity(state);
  m_needFontUpdate = gTrue;
}

// This looks wrong - why aren't adjusting the matrix?
void ArthurOutputDev::updateCTM(GfxState *state, double m11, double m12,
				double m21, double m22,
				double m31, double m32)
{
  updateLineDash(state);
  updateLineJoin(state);
  updateLineCap(state);
  updateLineWidth(state);
}

void ArthurOutputDev::updateLineDash(GfxState *state)
{
  // qDebug() << "updateLineDash";
}

void ArthurOutputDev::updateFlatness(GfxState *state)
{
  // qDebug() << "updateFlatness";
}

void ArthurOutputDev::updateLineJoin(GfxState *state)
{
  switch (state->getLineJoin()) {
  case 0:
    m_currentPen.setJoinStyle(Qt::MiterJoin);
    break;
  case 1:
    m_currentPen.setJoinStyle(Qt::RoundJoin);
    break;
  case 2:
    m_currentPen.setJoinStyle(Qt::BevelJoin);
    break;
  }
  m_painter->setPen(m_currentPen);
}

void ArthurOutputDev::updateLineCap(GfxState *state)
{
  switch (state->getLineCap()) {
  case 0:
    m_currentPen.setCapStyle(Qt::FlatCap);
    break;
  case 1:
    m_currentPen.setCapStyle(Qt::RoundCap);
    break;
  case 2:
    m_currentPen.setCapStyle(Qt::SquareCap);
    break;
  }
  m_painter->setPen(m_currentPen);
}

void ArthurOutputDev::updateMiterLimit(GfxState *state)
{
  // We can't do mitre (or Miter) limit with Qt4 yet.
  // the limit is in state->getMiterLimit() when we get there
}

void ArthurOutputDev::updateLineWidth(GfxState *state)
{
  m_currentPen.setWidthF(state->getTransformedLineWidth());
  m_painter->setPen(m_currentPen);
}

void ArthurOutputDev::updateFillColor(GfxState *state)
{
  GfxRGB rgb;
  QColor brushColour = m_currentBrush.color();
  state->getFillRGB(&rgb);
  brushColour.setRgbF(colToDbl(rgb.r), colToDbl(rgb.g), colToDbl(rgb.b), brushColour.alphaF());
  m_currentBrush.setColor(brushColour);
}

void ArthurOutputDev::updateStrokeColor(GfxState *state)
{
  GfxRGB rgb;
  QColor penColour = m_currentPen.color();
  state->getStrokeRGB(&rgb);
  penColour.setRgbF(colToDbl(rgb.r), colToDbl(rgb.g), colToDbl(rgb.b), penColour.alphaF());
  m_currentPen.setColor(penColour);
  m_painter->setPen(m_currentPen);
}

void ArthurOutputDev::updateFillOpacity(GfxState *state)
{
  QColor brushColour= m_currentBrush.color();
  brushColour.setAlphaF(state->getFillOpacity());
  m_currentBrush.setColor(brushColour);
}

void ArthurOutputDev::updateStrokeOpacity(GfxState *state)
{
  QColor penColour= m_currentPen.color();
  penColour.setAlphaF(state->getStrokeOpacity());
  m_currentPen.setColor(penColour);
  m_painter->setPen(m_currentPen);
}

void ArthurOutputDev::updateFont(GfxState *state)
{
  GfxFont *gfxFont;
  GfxFontType fontType;
  SplashOutFontFileID *id;
  SplashFontFile *fontFile;
  SplashFontSrc *fontsrc;
  FoFiTrueType *ff;
  Ref embRef;
  Object refObj, strObj;
  GooString *fileName, *substName;
  char *tmpBuf;
  int tmpBufLen;
  Gushort *codeToGID;
  DisplayFontParam *dfp;
  double m11, m12, m21, m22, w1, w2;
  SplashCoord mat[4];
  char *name;
  int c, substIdx, n, code;

  m_needFontUpdate = false;
  m_font = NULL;
  fileName = NULL;
  tmpBuf = NULL;
  substIdx = -1;

  if (!(gfxFont = state->getFont())) {
    goto err1;
  }
  fontType = gfxFont->getType();
  if (fontType == fontType3) {
    goto err1;
  }

  // check the font file cache
  id = new SplashOutFontFileID(gfxFont->getID());
  if ((fontFile = m_fontEngine->getFontFile(id))) {
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
	break;
      }
    }

    fontsrc = new SplashFontSrc;
    if (fileName)
      fontsrc->setFile(fileName, gFalse);
    else
      fontsrc->setBuf(tmpBuf, tmpBufLen, gFalse);

    // load the font file
    switch (fontType) {
    case fontType1:
      if (!(fontFile = m_fontEngine->loadType1Font(
			   id,
			   fontsrc,
			   ((Gfx8BitFont *)gfxFont)->getEncoding()))) {
	error(-1, "Couldn't create a font for '%s'",
	      gfxFont->getName() ? gfxFont->getName()->getCString()
	                         : "(unnamed)");
	goto err2;
      }
      break;
    case fontType1C:
      if (!(fontFile = m_fontEngine->loadType1CFont(
			   id,
			   fontsrc,
			   ((Gfx8BitFont *)gfxFont)->getEncoding()))) {
	error(-1, "Couldn't create a font for '%s'",
	      gfxFont->getName() ? gfxFont->getName()->getCString()
	                         : "(unnamed)");
	goto err2;
      }
      break;
    case fontTrueType:
      if (!(ff = FoFiTrueType::load(fileName->getCString()))) {
	goto err2;
      }
      codeToGID = ((Gfx8BitFont *)gfxFont)->getCodeToGIDMap(ff);
      delete ff;
      if (!(fontFile = m_fontEngine->loadTrueTypeFont(
			   id,
			   fontsrc,
			   codeToGID, 256))) {
	error(-1, "Couldn't create a font for '%s'",
	      gfxFont->getName() ? gfxFont->getName()->getCString()
	                         : "(unnamed)");
	goto err2;
      }
      break;
    case fontCIDType0:
    case fontCIDType0C:
      if (!(fontFile = m_fontEngine->loadCIDFont(
			   id,
			   fontsrc))) {
	error(-1, "Couldn't create a font for '%s'",
	      gfxFont->getName() ? gfxFont->getName()->getCString()
	                         : "(unnamed)");
	goto err2;
      }
      break;
    case fontCIDType2:
      n = ((GfxCIDFont *)gfxFont)->getCIDToGIDLen();
      codeToGID = (Gushort *)gmallocn(n, sizeof(Gushort));
      memcpy(codeToGID, ((GfxCIDFont *)gfxFont)->getCIDToGID(),
	     n * sizeof(Gushort));
      if (!(fontFile = m_fontEngine->loadTrueTypeFont(
			   id,
			   fontsrc,
			   codeToGID, n))) {
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
  m_font = m_fontEngine->getFont(fontFile, mat);

  return;

 err2:
  delete id;
 err1:
  return;
}

static QPainterPath convertPath(GfxState *state, GfxPath *path, Qt::FillRule fillRule)
{
  GfxSubpath *subpath;
  double x1, y1, x2, y2, x3, y3;
  int i, j;

  QPainterPath qPath;
  qPath.setFillRule(fillRule);
  for (i = 0; i < path->getNumSubpaths(); ++i) {
    subpath = path->getSubpath(i);
    if (subpath->getNumPoints() > 0) {
      state->transform(subpath->getX(0), subpath->getY(0), &x1, &y1);
      qPath.moveTo(x1, y1);
      j = 1;
      while (j < subpath->getNumPoints()) {
	if (subpath->getCurve(j)) {
	  state->transform(subpath->getX(j), subpath->getY(j), &x1, &y1);
	  state->transform(subpath->getX(j+1), subpath->getY(j+1), &x2, &y2);
	  state->transform(subpath->getX(j+2), subpath->getY(j+2), &x3, &y3);
	  qPath.cubicTo( x1, y1, x2, y2, x3, y3);
	  j += 3;
	} else {
	  state->transform(subpath->getX(j), subpath->getY(j), &x1, &y1);
	  qPath.lineTo(x1, y1);
	  ++j;
	}
      }
      if (subpath->isClosed()) {
	qPath.closeSubpath();
      }
    }
  }
  return qPath;
}

void ArthurOutputDev::stroke(GfxState *state)
{
  m_painter->drawPath( convertPath( state, state->getPath(), Qt::OddEvenFill ) );
}

void ArthurOutputDev::fill(GfxState *state)
{
  m_painter->fillPath( convertPath( state, state->getPath(), Qt::WindingFill ), m_currentBrush );
}

void ArthurOutputDev::eoFill(GfxState *state)
{
  m_painter->fillPath( convertPath( state, state->getPath(), Qt::OddEvenFill ), m_currentBrush );
}

void ArthurOutputDev::clip(GfxState *state)
{
  m_painter->setClipPath(convertPath( state, state->getPath(), Qt::WindingFill ) );
}

void ArthurOutputDev::eoClip(GfxState *state)
{
  m_painter->setClipPath(convertPath( state, state->getPath(), Qt::OddEvenFill ) );
}

void ArthurOutputDev::drawChar(GfxState *state, double x, double y,
			       double dx, double dy,
			       double originX, double originY,
			       CharCode code, int nBytes, Unicode *u, int uLen) {
  double x1, y1;
//   SplashPath *path;
  int render;

  if (m_needFontUpdate) {
    updateFont(state);
  }
  if (!m_font) {
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
    int x0, y0, xFrac, yFrac;
    SplashGlyphBitmap glyph;

    x0 = static_cast<int>(floor(x1));
    xFrac = splashFloor((x1 - x0) * splashFontFraction);
    y0 = static_cast<int>(floor(y1));
    yFrac = splashFloor((y1 - y0) * splashFontFraction);
    SplashPath * fontPath;
    fontPath = m_font->getGlyphPath(code);
    if (fontPath) {
      QPainterPath qPath;
      qPath.setFillRule(Qt::WindingFill);
      for (int i = 0; i < fontPath->length; ++i) {
	if (fontPath->flags[i] & splashPathFirst) {
	  qPath.moveTo(x0+fontPath->pts[i].x, y0+fontPath->pts[i].y);
	} else if (fontPath->flags[i] & splashPathCurve) {
	  qPath.quadTo(x0+fontPath->pts[i].x, y0+fontPath->pts[i].y,
		       x0+fontPath->pts[i+1].x, y0+fontPath->pts[i+1].y);
	  ++i;
	} else if (fontPath->flags[i] & splashPathArcCW) {
	  qDebug() << "Need to implement arc";
	} else {
	  qPath.lineTo(x0+fontPath->pts[i].x, y0+fontPath->pts[i].y);
	}
	if (fontPath->flags[i] & splashPathLast) {
	  qPath.closeSubpath();
	}
      }
      m_painter->save();
      GfxRGB rgb;
      QColor brushColour = m_currentBrush.color();
      state->getFillRGB(&rgb);
      brushColour.setRgbF(colToDbl(rgb.r), colToDbl(rgb.g), colToDbl(rgb.b), state->getFillOpacity());
      m_painter->setBrush(brushColour);
      QColor penColour = m_currentPen.color();
      state->getStrokeRGB(&rgb);
      penColour.setRgbF(colToDbl(rgb.r), colToDbl(rgb.g), colToDbl(rgb.b), state->getStrokeOpacity());
      m_painter->setPen(penColour);
      m_painter->drawPath( qPath );
      m_painter->restore();
    }
  }

  // stroke
  if ((render & 3) == 1 || (render & 3) == 2) {
    qDebug() << "no stroke";
    /*
    if ((path = m_font->getGlyphPath(code))) {
      path->offset((SplashCoord)x1, (SplashCoord)y1);
      splash->stroke(path);
      delete path;
    }
    */
  }

  // clip
  if (render & 4) {
    qDebug() << "no clip";
    /*
    path = m_font->getGlyphPath(code);
    path->offset((SplashCoord)x1, (SplashCoord)y1);
    if (textClipPath) {
      textClipPath->append(path);
      delete path;
    } else {
      textClipPath = path;
    }
    */
  }
}

GBool ArthurOutputDev::beginType3Char(GfxState *state, double x, double y,
				      double dx, double dy,
				      CharCode code, Unicode *u, int uLen)
{
  return gFalse;
}

void ArthurOutputDev::endType3Char(GfxState *state)
{
}

void ArthurOutputDev::type3D0(GfxState *state, double wx, double wy)
{
}

void ArthurOutputDev::type3D1(GfxState *state, double wx, double wy,
			      double llx, double lly, double urx, double ury)
{
}

void ArthurOutputDev::endTextObject(GfxState *state)
{
}


void ArthurOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
				    int width, int height, GBool invert,
				    GBool inlineImg)
{
  qDebug() << "drawImageMask";
#if 0
  unsigned char *buffer;
  unsigned char *dest;
  cairo_surface_t *image;
  cairo_pattern_t *pattern;
  int x, y;
  ImageStream *imgStr;
  Guchar *pix;
  double *ctm;
  cairo_matrix_t matrix;
  int invert_bit;
  int row_stride;

  row_stride = (width + 3) & ~3;
  buffer = (unsigned char *) malloc (height * row_stride);
  if (buffer == NULL) {
    error(-1, "Unable to allocate memory for image.");
    return;
  }

  /* TODO: Do we want to cache these? */
  imgStr = new ImageStream(str, width, 1, 1);
  imgStr->reset();

  invert_bit = invert ? 1 : 0;

  for (y = 0; y < height; y++) {
    pix = imgStr->getLine();
    dest = buffer + y * row_stride;
    for (x = 0; x < width; x++) {

      if (pix[x] ^ invert_bit)
	*dest++ = 0;
      else
	*dest++ = 255;
    }
  }

  image = cairo_image_surface_create_for_data (buffer, CAIRO_FORMAT_A8,
					  width, height, row_stride);
  if (image == NULL)
    return;
  pattern = cairo_pattern_create_for_surface (image);
  if (pattern == NULL)
    return;

  ctm = state->getCTM();
  LOG (printf ("drawImageMask %dx%d, matrix: %f, %f, %f, %f, %f, %f\n",
	       width, height, ctm[0], ctm[1], ctm[2], ctm[3], ctm[4], ctm[5]));
  matrix.xx = ctm[0] / width;
  matrix.xy = -ctm[2] / height;
  matrix.yx = ctm[1] / width;
  matrix.yy = -ctm[3] / height;
  matrix.x0 = ctm[2] + ctm[4];
  matrix.y0 = ctm[3] + ctm[5];
  cairo_matrix_invert (&matrix);
  cairo_pattern_set_matrix (pattern, &matrix);

  cairo_pattern_set_filter (pattern, CAIRO_FILTER_BEST);
  /* FIXME: Doesn't the image mask support any colorspace? */
  cairo_set_source_rgb (cairo, fill_color.r, fill_color.g, fill_color.b);
  cairo_mask (cairo, pattern);

  cairo_pattern_destroy (pattern);
  cairo_surface_destroy (image);
  free (buffer);
  delete imgStr;
#endif
}

//TODO: lots more work here.
void ArthurOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
				int width, int height,
				GfxImageColorMap *colorMap,
				int *maskColors, GBool inlineImg)
{
  unsigned char *buffer;
  unsigned int *dest;
  int x, y;
  ImageStream *imgStr;
  Guchar *pix;
  GfxRGB rgb;
  int alpha, i;
  double *ctm;
  QMatrix matrix;
  int is_identity_transform;
  
  buffer = (unsigned char *)gmalloc (width * height * 4);

  /* TODO: Do we want to cache these? */
  imgStr = new ImageStream(str, width,
			   colorMap->getNumPixelComps(),
			   colorMap->getBits());
  imgStr->reset();
  
  /* ICCBased color space doesn't do any color correction
   * so check its underlying color space as well */
  is_identity_transform = colorMap->getColorSpace()->getMode() == csDeviceRGB ||
		  colorMap->getColorSpace()->getMode() == csICCBased && 
		  ((GfxICCBasedColorSpace*)colorMap->getColorSpace())->getAlt()->getMode() == csDeviceRGB;

  if (maskColors) {
    for (y = 0; y < height; y++) {
      dest = (unsigned int *) (buffer + y * 4 * width);
      pix = imgStr->getLine();
      colorMap->getRGBLine (pix, dest, width);

      for (x = 0; x < width; x++) {
	for (i = 0; i < colorMap->getNumPixelComps(); ++i) {
	  
	  if (pix[i] < maskColors[2*i] * 255||
	      pix[i] > maskColors[2*i+1] * 255) {
	    *dest = *dest | 0xff000000;
	    break;
	  }
	}
	pix += colorMap->getNumPixelComps();
	dest++;
      }
    }

    m_image = new QImage(buffer, width, height, QImage::Format_ARGB32);
  }
  else {
    for (y = 0; y < height; y++) {
      dest = (unsigned int *) (buffer + y * 4 * width);
      pix = imgStr->getLine();
      colorMap->getRGBLine (pix, dest, width);
    }

    m_image = new QImage(buffer, width, height, QImage::Format_RGB32);
  }

  if (m_image == NULL || m_image->isNull()) {
    qDebug() << "Null image";
    return;
  }
  ctm = state->getCTM();
  matrix.setMatrix(ctm[0] / width, ctm[1] / width, -ctm[2] / height, -ctm[3] / height, ctm[2] + ctm[4], ctm[3] + ctm[5]);

  m_painter->setMatrix(matrix, true);
  m_painter->drawImage( QPoint(0,0), *m_image );
  free (buffer);
  delete imgStr;

}
