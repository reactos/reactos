//========================================================================
//
// SplashFTFontFile.h
//
//========================================================================

#ifndef SPLASHFTFONTFILE_H
#define SPLASHFTFONTFILE_H

#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include "SplashFontFile.h"

class SplashFontFileID;
class SplashFTFontEngine;

//------------------------------------------------------------------------
// SplashFTFontFile
//------------------------------------------------------------------------

class SplashFTFontFile: public SplashFontFile {
public:

  static SplashFontFile *loadType1Font(SplashFTFontEngine *engineA,
				       SplashFontFileID *idA,
				       SplashFontSrc *src, char **encA);
  static SplashFontFile *loadCIDFont(SplashFTFontEngine *engineA,
					 SplashFontFileID *idA,
					 SplashFontSrc *src,
					 Gushort *codeToCIDA, int codeToGIDLenA);
  static SplashFontFile *loadTrueTypeFont(SplashFTFontEngine *engineA,
					  SplashFontFileID *idA,
					  SplashFontSrc *src,
					  Gushort *codeToGIDA,
					  int codeToGIDLenA,
					  int faceIndexA=0);

  virtual ~SplashFTFontFile();

  // Create a new SplashFTFont, i.e., a scaled instance of this font
  // file.
  virtual SplashFont *makeFont(SplashCoord *mat);

private:

  SplashFTFontFile(SplashFTFontEngine *engineA,
		   SplashFontFileID *idA,
		   SplashFontSrc *srcA,
		   FT_Face faceA,
		   Gushort *codeToGIDA, int codeToGIDLenA);

  SplashFTFontEngine *engine;
  FT_Face face;
  Gushort *codeToGID;
  int codeToGIDLen;

  friend class SplashFTFont;
};

#endif // HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H

#endif
