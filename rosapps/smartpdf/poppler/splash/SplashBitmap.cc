//========================================================================
//
// SplashBitmap.cc
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#include "goo/gmem.h"
#include "SplashErrorCodes.h"
#include "SplashBitmap.h"

//------------------------------------------------------------------------
// SplashBitmap
//------------------------------------------------------------------------

SplashBitmap::SplashBitmap(int widthA, int heightA, int rowPad,
			   SplashColorMode modeA, GBool topDown) {
  width = widthA;
  height = heightA;
  mode = modeA;
  switch (mode) {
  case splashModeMono1:
    rowSize = (width + 7) >> 3;
    break;
  case splashModeMono8:
    rowSize = width;
    break;
  case splashModeAMono8:
    rowSize = width * 2;
    break;
  case splashModeRGB8:
  case splashModeBGR8:
    rowSize = width * 3;
    break;
  case splashModeRGB8Qt:
  case splashModeARGB8:
  case splashModeBGRA8:
#if SPLASH_CMYK
  case splashModeCMYK8:
#endif
    rowSize = width * 4;
    break;
#if SPLASH_CMYK
  case splashModeACMYK8:
    rowSize = width * 5;
    break;
#endif
  }
  rowSize += rowPad - 1;
  rowSize -= rowSize % rowPad;
  data = NULL;
  data = (SplashColorPtr)gmalloc(rowSize * height);
  if (!data)
    return;
  if (!topDown) {
    data += (height - 1) * rowSize;
    rowSize = -rowSize;
  }
}


SplashBitmap::~SplashBitmap() {
  if (rowSize < 0) {
    gfree(data + (height - 1) * rowSize);
  } else {
    gfree(data);
  }
}

SplashError SplashBitmap::writePNMFile(char *fileName) {
  FILE *f;
  SplashColorPtr row, p;
  int x, y;

  if (!(f = fopen(fileName, "wb"))) {
    return splashErrOpenFile;
  }

  switch (mode) {

  case splashModeMono1:
    fprintf(f, "P4\n%d %d\n", width, height);
    row = data;
    for (y = 0; y < height; ++y) {
      p = row;
      for (x = 0; x < width; x += 8) {
	fputc(*p ^ 0xff, f);
	++p;
      }
      row += rowSize;
    }
    break;

  case splashModeMono8:
    fprintf(f, "P5\n%d %d\n255\n", width, height);
    row = data;
    for (y = 0; y < height; ++y) {
      p = row;
      for (x = 0; x < width; ++x) {
	fputc(*p, f);
	++p;
      }
      row += rowSize;
    }
    break;

  case splashModeAMono8:
    fprintf(f, "P5\n%d %d\n255\n", width, height);
    row = data;
    for (y = 0; y < height; ++y) {
      p = row;
      for (x = 0; x < width; ++x) {
	fputc(splashAMono8M(p), f);
	p += 2;
      }
      row += rowSize;
    }
    break;

  case splashModeRGB8:
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    row = data;
    for (y = 0; y < height; ++y) {
      p = row;
      for (x = 0; x < width; ++x) {
	fputc(splashRGB8R(p), f);
	fputc(splashRGB8G(p), f);
	fputc(splashRGB8B(p), f);
	p += 3;
      }
      row += rowSize;
    }
    break;

  case splashModeBGR8:
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    row = data;
    for (y = 0; y < height; ++y) {
      p = row;
      for (x = 0; x < width; ++x) {
	fputc(splashBGR8R(p), f);
	fputc(splashBGR8G(p), f);
	fputc(splashBGR8B(p), f);
	p += 3;
      }
      row += rowSize;
    }
    break;

 case splashModeRGB8Qt:
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    row = data;
    for (y = 0; y < height; ++y) {
      p = row;
      for (x = 0; x < width; ++x) {
	fputc(splashRGB8R(p), f);
	fputc(splashRGB8G(p), f);
	fputc(splashRGB8B(p), f);
	p += 4;
      }
      row += rowSize;
    }
    break;

  case splashModeARGB8:
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    row = data;
    for (y = 0; y < height; ++y) {
      p = row;
      for (x = 0; x < width; ++x) {
	fputc(splashARGB8R(p), f);
	fputc(splashARGB8G(p), f);
	fputc(splashARGB8B(p), f);
	p += 4;
      }
      row += rowSize;
    }
    break;

  case splashModeBGRA8:
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    row = data;
    for (y = 0; y < height; ++y) {
      p = row;
      for (x = 0; x < width; ++x) {
	fputc(splashBGRA8R(p), f);
	fputc(splashBGRA8G(p), f);
	fputc(splashBGRA8B(p), f);
	p += 4;
      }
      row += rowSize;
    }
    break;

#if SPLASH_CMYK
  case splashModeCMYK8:
  case splashModeACMYK8:
    // PNM doesn't support CMYK
    break;
#endif
  }

  fclose(f);
  return splashOk;
}

void SplashBitmap::getPixel(int x, int y, SplashColorPtr pixel) {
  SplashColorPtr p;

  if (y < 0 || y >= height || x < 0 || x >= width) {
    return;
  }
  switch (mode) {
  case splashModeMono1:
    p = &data[y * rowSize + (x >> 3)];
    pixel[0] = (p[0] >> (7 - (x & 7))) & 1;
    break;
  case splashModeMono8:
    p = &data[y * rowSize + x];
    pixel[0] = p[0];
    break;
  case splashModeAMono8:
    p = &data[y * rowSize + 2 * x];
    pixel[0] = p[0];
    pixel[1] = p[1];
    break;
  case splashModeRGB8:
  case splashModeBGR8:
    p = &data[y * rowSize + 3 * x];
    pixel[0] = p[0];
    pixel[1] = p[1];
    pixel[2] = p[2];
    break;
  case splashModeRGB8Qt:
    p = &data[y * rowSize + 4 * x];
    pixel[0] = p[2];
    pixel[1] = p[1];
    pixel[2] = p[0];
    break;
  case splashModeARGB8:
  case splashModeBGRA8:
#if SPLASH_CMYK
  case splashModeCMYK8:
#endif
    p = &data[y * rowSize + 4 * x];
    pixel[0] = p[0];
    pixel[1] = p[1];
    pixel[2] = p[2];
    pixel[3] = p[3];
    break;
#if SPLASH_CMYK
  case splashModeACMYK8:
    p = &data[y * rowSize + 5 * x];
    pixel[0] = p[0];
    pixel[1] = p[1];
    pixel[2] = p[2];
    pixel[3] = p[3];
    pixel[4] = p[4];
    break;
#endif
  }
}
