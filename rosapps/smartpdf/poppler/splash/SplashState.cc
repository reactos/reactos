//========================================================================
//
// SplashState.cc
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <string.h>
#include "goo/gmem.h"
#include "SplashPattern.h"
#include "SplashScreen.h"
#include "SplashClip.h"
#include "SplashState.h"

//------------------------------------------------------------------------
// SplashState
//------------------------------------------------------------------------

// number of components in each color mode
int splashColorModeNComps[] = {
  1, 1, 2, 3, 3, 4, 4
};

SplashState::SplashState(int width, int height) {
  SplashColor color;

  memset(&color, 0, sizeof(SplashColor));
  strokePattern = new SplashSolidColor(color);
  fillPattern = new SplashSolidColor(color);
  screen = new SplashScreen(10);
  blendFunc = NULL;
  strokeAlpha = 1;
  fillAlpha = 1;
  lineWidth = 0;
  lineCap = splashLineCapButt;
  lineJoin = splashLineJoinMiter;
  miterLimit = 10;
  flatness = 1;
  lineDash = NULL;
  lineDashLength = 0;
  lineDashPhase = 0;
  clip = new SplashClip(0, 0, width - 1, height - 1);
  next = NULL;
}

SplashState::SplashState(SplashState *state) {
  strokePattern = state->strokePattern->copy();
  fillPattern = state->fillPattern->copy();
  screen = state->screen->copy();
  blendFunc = state->blendFunc;
  strokeAlpha = state->strokeAlpha;
  fillAlpha = state->fillAlpha;
  lineWidth = state->lineWidth;
  lineCap = state->lineCap;
  lineJoin = state->lineJoin;
  miterLimit = state->miterLimit;
  flatness = state->flatness;
  if (state->lineDash) {
    lineDashLength = state->lineDashLength;
    lineDash = (SplashCoord *)gmallocn(lineDashLength, sizeof(SplashCoord));
    memcpy(lineDash, state->lineDash, lineDashLength * sizeof(SplashCoord));
  } else {
    lineDash = NULL;
    lineDashLength = 0;
  }
  lineDashPhase = state->lineDashPhase;
  clip = state->clip->copy();
  next = NULL;
}

SplashState::~SplashState() {
  delete strokePattern;
  delete fillPattern;
  delete screen;
  gfree(lineDash);
  delete clip;
}

void SplashState::setStrokePattern(SplashPattern *strokePatternA) {
  delete strokePattern;
  strokePattern = strokePatternA;
}

void SplashState::setFillPattern(SplashPattern *fillPatternA) {
  delete fillPattern;
  fillPattern = fillPatternA;
}

void SplashState::setScreen(SplashScreen *screenA) {
  delete screen;
  screen = screenA;
}

void SplashState::setLineDash(SplashCoord *lineDashA, int lineDashLengthA,
			      SplashCoord lineDashPhaseA) {
  gfree(lineDash);
  lineDashLength = lineDashLengthA;
  if (lineDashLength > 0) {
    lineDash = (SplashCoord *)gmallocn(lineDashLength, sizeof(SplashCoord));
    memcpy(lineDash, lineDashA, lineDashLength * sizeof(SplashCoord));
  } else {
    lineDash = NULL;
  }
  lineDashPhase = lineDashPhaseA;
}
