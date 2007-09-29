//========================================================================
//
// OutputDev.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stddef.h>
#include "Object.h"
#include "Stream.h"
#include "GfxState.h"
#include "OutputDev.h"
#include "goo/GooHash.h"

//------------------------------------------------------------------------
// OutputDev
//------------------------------------------------------------------------

void OutputDev::setDefaultCTM(double *ctm) {
  int i;
  double det;

  for (i = 0; i < 6; ++i) {
    defCTM[i] = ctm[i];
  }
  det = 1 / (defCTM[0] * defCTM[3] - defCTM[1] * defCTM[2]);
  defICTM[0] = defCTM[3] * det;
  defICTM[1] = -defCTM[1] * det;
  defICTM[2] = -defCTM[2] * det;
  defICTM[3] = defCTM[0] * det;
  defICTM[4] = (defCTM[2] * defCTM[5] - defCTM[3] * defCTM[4]) * det;
  defICTM[5] = (defCTM[1] * defCTM[4] - defCTM[0] * defCTM[5]) * det;
}

void OutputDev::cvtDevToUser(double dx, double dy, double *ux, double *uy) {
  *ux = defICTM[0] * dx + defICTM[2] * dy + defICTM[4];
  *uy = defICTM[1] * dx + defICTM[3] * dy + defICTM[5];
}

void OutputDev::cvtUserToDev(double ux, double uy, int *dx, int *dy) {
  *dx = (int)(defCTM[0] * ux + defCTM[2] * uy + defCTM[4] + 0.5);
  *dy = (int)(defCTM[1] * ux + defCTM[3] * uy + defCTM[5] + 0.5);
}

void OutputDev::updateAll(GfxState *state) {
  updateLineDash(state);
  updateFlatness(state);
  updateLineJoin(state);
  updateLineCap(state);
  updateMiterLimit(state);
  updateLineWidth(state);
  updateFillColorSpace(state);
  updateFillColor(state);
  updateStrokeColorSpace(state);
  updateStrokeColor(state);
  updateBlendMode(state);
  updateFillOpacity(state);
  updateStrokeOpacity(state);
  updateFillOverprint(state);
  updateStrokeOverprint(state);
  updateFont(state);
}

GBool OutputDev::beginType3Char(GfxState *state, double x, double y,
				double dx, double dy,
				CharCode code, Unicode *u, int uLen) {
  return gFalse;
}

void OutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
			      int width, int height, GBool invert,
			      GBool inlineImg) {
  int i, j;

  if (inlineImg) {
    str->reset();
    j = height * ((width + 7) / 8);
    for (i = 0; i < j; ++i)
      str->getChar();
    str->close();
  }
}

void OutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
			  int width, int height, GfxImageColorMap *colorMap,
			  int *maskColors, GBool inlineImg) {
  int i, j;

  if (inlineImg) {
    str->reset();
    j = height * ((width * colorMap->getNumPixelComps() *
		   colorMap->getBits() + 7) / 8);
    for (i = 0; i < j; ++i)
      str->getChar();
    str->close();
  }
}

void OutputDev::drawMaskedImage(GfxState *state, Object *ref, Stream *str,
				int width, int height,
				GfxImageColorMap *colorMap,
				Stream *maskStr,
				int maskWidth, int maskHeight,
				GBool maskInvert) {
  drawImage(state, ref, str, width, height, colorMap, NULL, gFalse);
}

void OutputDev::drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str,
				    int width, int height,
				    GfxImageColorMap *colorMap,
				    Stream *maskStr,
				    int maskWidth, int maskHeight,
				    GfxImageColorMap *maskColorMap) {
  drawImage(state, ref, str, width, height, colorMap, NULL, gFalse);
}

void OutputDev::endMarkedContent() {
}

void OutputDev::beginMarkedContent(char *name) {
}

void OutputDev::beginMarkedContent(char *name, Dict *properties) {
}

void OutputDev::markPoint(char *name) {
}

void OutputDev::markPoint(char *name, Dict *properties) {
}


#if OPI_SUPPORT
void OutputDev::opiBegin(GfxState *state, Dict *opiDict) {
}

void OutputDev::opiEnd(GfxState *state, Dict *opiDict) {
}
#endif

void OutputDev::startProfile() {
  if (profileHash)
    delete profileHash;

  profileHash = new GooHash (true);
}
 
GooHash *OutputDev::endProfile() {
  GooHash *profile = profileHash;

  profileHash = NULL;

  return profile;
}

