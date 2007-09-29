//========================================================================
//
// SplashBitmap.h
//
//========================================================================

#ifndef SPLASHBITMAP_H
#define SPLASHBITMAP_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "SplashTypes.h"

//------------------------------------------------------------------------
// SplashBitmap
//------------------------------------------------------------------------

class SplashBitmap {
public:

  // Create a new bitmap.  It will have <widthA> x <heightA> pixels in
  // color mode <modeA>.  Rows will be padded out to a multiple of
  // <rowPad> bytes.  If <topDown> is false, the bitmap will be stored
  // upside-down, i.e., with the last row first in memory.
  SplashBitmap(int widthA, int heightA, int rowPad,
	       SplashColorMode modeA, GBool topDown = gTrue);

  ~SplashBitmap();

  int getWidth() { return width; }
  int getHeight() { return height; }
  int getRowSize() { return rowSize; }
  SplashColorMode getMode() { return mode; }
  SplashColorPtr getDataPtr() { return data; }

  SplashError writePNMFile(char *fileName);

  void getPixel(int x, int y, SplashColorPtr pixel);

private:

  int width, height;		// size of bitmap
  int rowSize;			// size of one row of data, in bytes
				//   - negative for bottom-up bitmaps
  SplashColorMode mode;		// color mode
  SplashColorPtr data;		// pointer to row zero of the bitmap data

  friend class Splash;
};

#endif
