//========================================================================
//
// SplashT1FontFile.h
//
//========================================================================

#ifndef SPLASHT1FONTFILE_H
#define SPLASHT1FONTFILE_H

#if HAVE_T1LIB_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "SplashFontFile.h"

class SplashT1FontEngine;

//------------------------------------------------------------------------
// SplashT1FontFile
//------------------------------------------------------------------------

class SplashT1FontFile: public SplashFontFile {
public:

  static SplashFontFile *loadType1Font(SplashT1FontEngine *engineA,
				       SplashFontFileID *idA,
				       SplashFontSrc *src,
				       char **encA);

  virtual ~SplashT1FontFile();

  // Create a new SplashT1Font, i.e., a scaled instance of this font
  // file.
  virtual SplashFont *makeFont(SplashCoord *mat);

private:

  SplashT1FontFile(SplashT1FontEngine *engineA,
		   SplashFontFileID *idA,
		   SplashFontSrc *src,
		   int t1libIDA, char **encA, char *encStrA);

  SplashT1FontEngine *engine;
  int t1libID;			// t1lib font ID
  char **enc;
  char *encStr;

  friend class SplashT1Font;
};

#endif // HAVE_T1LIB_H

#endif
