//========================================================================
//
// OutputDev.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef OUTPUTDEV_H
#define OUTPUTDEV_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <poppler-config.h>
#include "goo/gtypes.h"
#include "CharTypes.h"

class Dict;
class GooHash;
class GooString;
class GfxState;
class GfxColorSpace;
class GfxImageColorMap;
class GfxFunctionShading;
class GfxAxialShading;
class GfxRadialShading;
class Stream;
class Link;
class Catalog;

//------------------------------------------------------------------------
// OutputDev
//------------------------------------------------------------------------

class OutputDev {
public:

  // Constructor.
  OutputDev() { profileHash = NULL; }

  // Destructor.
  virtual ~OutputDev() {}

  //----- get info about output device

  // Does this device use upside-down coordinates?
  // (Upside-down means (0,0) is the top left corner of the page.)
  virtual GBool upsideDown() = 0;

  // Does this device use drawChar() or drawString()?
  virtual GBool useDrawChar() = 0;

  // Does this device use tilingPatternFill()?  If this returns false,
  // tiling pattern fills will be reduced to a series of other drawing
  // operations.
  virtual GBool useTilingPatternFill() { return gFalse; }

  // Does this device use functionShadedFill(), axialShadedFill(), and
  // radialShadedFill()?  If this returns false, these shaded fills
  // will be reduced to a series of other drawing operations.
  virtual GBool useShadedFills() { return gFalse; }

  // Does this device use beginType3Char/endType3Char?  Otherwise,
  // text in Type 3 fonts will be drawn with drawChar/drawString.
  virtual GBool interpretType3Chars() = 0;

  // Does this device need non-text content?
  virtual GBool needNonText() { return gTrue; }

  //----- initialization and control

  // Set default transform matrix.
  virtual void setDefaultCTM(double *ctm);

  // Start a page.
  virtual void startPage(int /*pageNum*/, GfxState * /*state*/) {}

  // End a page.
  virtual void endPage() {}

  // Dump page contents to display.
  virtual void dump() {}

  //----- coordinate conversion

  // Convert between device and user coordinates.
  virtual void cvtDevToUser(double dx, double dy, double *ux, double *uy);
  virtual void cvtUserToDev(double ux, double uy, int *dx, int *dy);

  double *getDefCTM() { return defCTM; }
  double *getDefICTM() { return defICTM; }

  //----- link borders
  virtual void drawLink(Link * /*link*/, Catalog * /*catalog*/) {}

  //----- save/restore graphics state
  virtual void saveState(GfxState * /*state*/) {}
  virtual void restoreState(GfxState * /*state*/) {}

  //----- update graphics state
  virtual void updateAll(GfxState *state);
  virtual void updateCTM(GfxState * /*state*/, double /*m11*/, double /*m12*/,
			 double /*m21*/, double /*m22*/, double /*m31*/, double /*m32*/) {}
  virtual void updateLineDash(GfxState * /*state*/) {}
  virtual void updateFlatness(GfxState * /*state*/) {}
  virtual void updateLineJoin(GfxState * /*state*/) {}
  virtual void updateLineCap(GfxState * /*state*/) {}
  virtual void updateMiterLimit(GfxState * /*state*/) {}
  virtual void updateLineWidth(GfxState * /*state*/) {}
  virtual void updateFillColorSpace(GfxState * /*state*/) {}
  virtual void updateStrokeColorSpace(GfxState * /*state*/) {}
  virtual void updateFillColor(GfxState * /*state*/) {}
  virtual void updateStrokeColor(GfxState * /*state*/) {}
  virtual void updateBlendMode(GfxState * /*state*/) {}
  virtual void updateFillOpacity(GfxState * /*state*/) {}
  virtual void updateStrokeOpacity(GfxState * /*state*/) {}
  virtual void updateFillOverprint(GfxState * /*state*/) {}
  virtual void updateStrokeOverprint(GfxState * /*state*/) {}

  //----- update text state
  virtual void updateFont(GfxState * /*state*/) {}
  virtual void updateTextMat(GfxState * /*state*/) {}
  virtual void updateCharSpace(GfxState * /*state*/) {}
  virtual void updateRender(GfxState * /*state*/) {}
  virtual void updateRise(GfxState * /*state*/) {}
  virtual void updateWordSpace(GfxState * /*state*/) {}
  virtual void updateHorizScaling(GfxState * /*state*/) {}
  virtual void updateTextPos(GfxState * /*state*/) {}
  virtual void updateTextShift(GfxState * /*state*/, double /*shift*/) {}

  //----- path painting
  virtual void stroke(GfxState * /*state*/) {}
  virtual void fill(GfxState * /*state*/) {}
  virtual void eoFill(GfxState * /*state*/) {}
  virtual void tilingPatternFill(GfxState * /*state*/, Object * /*str*/,
				 int /*paintType*/, Dict * /*resDict*/,
				 double * /*mat*/, double * /*bbox*/,
				 int /*x0*/, int /*y0*/, int /*x1*/, int /*y1*/,
				 double /*xStep*/, double /*yStep*/) {}
  virtual void functionShadedFill(GfxState * /*state*/,
				  GfxFunctionShading * /*shading*/) {}
  virtual void axialShadedFill(GfxState * /*state*/, GfxAxialShading * /*shading*/) {}
  virtual void radialShadedFill(GfxState * /*state*/, GfxRadialShading * /*shading*/) {}

  //----- path clipping
  virtual void clip(GfxState * /*state*/) {}
  virtual void eoClip(GfxState * /*state*/) {}

  //----- text drawing
  virtual void beginStringOp(GfxState * /*state*/) {}
  virtual void endStringOp(GfxState * /*state*/) {}
  virtual void beginString(GfxState * /*state*/, GooString * /*s*/) {}
  virtual void endString(GfxState * /*state*/) {}
  virtual void drawChar(GfxState * /*state*/, double /*x*/, double /*y*/,
			double /*dx*/, double /*dy*/,
			double /*originX*/, double /*originY*/,
			CharCode /*code*/, int /*nBytes*/, Unicode * /*u*/, int /*uLen*/) {}
  virtual void drawString(GfxState * /*state*/, GooString * /*s*/) {}
  virtual GBool beginType3Char(GfxState * /*state*/, double /*x*/, double /*y*/,
			       double /*dx*/, double /*dy*/,
			       CharCode /*code*/, Unicode * /*u*/, int /*uLen*/);
  virtual void endType3Char(GfxState * /*state*/) {}
  virtual void endTextObject(GfxState * /*state*/) {}

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

  //----- grouping operators

  virtual void endMarkedContent();
  virtual void beginMarkedContent(char *name);
  virtual void beginMarkedContent(char *name, Dict *properties);
  virtual void markPoint(char *name);
  virtual void markPoint(char *name, Dict *properties);
  
  

#if OPI_SUPPORT
  //----- OPI functions
  virtual void opiBegin(GfxState *state, Dict *opiDict);
  virtual void opiEnd(GfxState *state, Dict *opiDict);
#endif

  //----- Type 3 font operators
  virtual void type3D0(GfxState * /*state*/, double /*wx*/, double /*wy*/) {}
  virtual void type3D1(GfxState * /*state*/, double /*wx*/, double /*wy*/,
		       double /*llx*/, double /*lly*/, double /*urx*/, double /*ury*/) {}

  //----- PostScript XObjects
  virtual void psXObject(Stream * /*psStream*/, Stream * /*level1Stream*/) {}

  //----- Profiling
  virtual void startProfile();
  virtual GooHash *getProfileHash() {return profileHash; }
  virtual GooHash *endProfile();

  
private:

  double defCTM[6];		// default coordinate transform matrix
  double defICTM[6];		// inverse of default CTM
  GooHash *profileHash;
};

#endif
