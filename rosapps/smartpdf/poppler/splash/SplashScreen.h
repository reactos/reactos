//========================================================================
//
// SplashScreen.h
//
//========================================================================

#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "SplashTypes.h"

//------------------------------------------------------------------------
// SplashScreen
//------------------------------------------------------------------------

class SplashScreen {
public:

  SplashScreen(int sizeA);
  SplashScreen(SplashScreen *screen);
  ~SplashScreen();

  SplashScreen *copy() { return new SplashScreen(this); }

  // Return the computed pixel value (0=black, 1=white) for the gray
  // level <value> at (<x>, <y>).
  int test(int x, int y, SplashCoord value);

  // Returns true if value is above the white threshold or below the
  // black threshold, i.e., if the corresponding halftone will be
  // solid white or black.
  GBool isStatic(SplashCoord value);

private:

  SplashCoord *mat;		// threshold matrix
  int size;			// size of the threshold matrix
  SplashCoord minVal;		// any pixel value below minVal generates
				//   solid black
  SplashCoord maxVal;		// any pixel value above maxVal generates
				//   solid white
};

#endif
