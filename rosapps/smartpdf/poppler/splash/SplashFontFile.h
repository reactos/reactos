//========================================================================
//
// SplashFontFile.h
//
//========================================================================

#ifndef SPLASHFONTFILE_H
#define SPLASHFONTFILE_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "goo/gtypes.h"
#include "SplashTypes.h"

class GooString;
class SplashFontEngine;
class SplashFont;
class SplashFontFileID;

//------------------------------------------------------------------------
// SplashFontFile
//------------------------------------------------------------------------

class SplashFontSrc {
public:
  SplashFontSrc();
  ~SplashFontSrc();

  void setFile(GooString *file, GBool del);
  void setFile(const char *file, GBool del);
  void setBuf(char *bufA, int buflenA, GBool del);

  void ref();
  void unref();

  GBool isFile;
  GooString *fileName;
  char *buf;
  int bufLen;
  GBool deleteSrc;
  int refcnt;
};

class SplashFontFile {
public:

  virtual ~SplashFontFile();

  // Create a new SplashFont, i.e., a scaled instance of this font
  // file.
  virtual SplashFont *makeFont(SplashCoord *mat) = 0;

  // Get the font file ID.
  SplashFontFileID *getID() { return id; }

  // Increment the reference count.
  void incRefCnt();

  // Decrement the reference count.  If the new value is zero, delete
  // the SplashFontFile object.
  void decRefCnt();

protected:

  SplashFontFile(SplashFontFileID *idA, SplashFontSrc *srcA);

  SplashFontFileID *id;
  SplashFontSrc *src;
  int refCnt;

  friend class SplashFontEngine;
};

#endif
