//========================================================================
//
// SplashFontFile.cc
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "goo/gmem.h"
#include "goo/GooString.h"
#include "SplashFontFile.h"
#include "SplashFontFileID.h"

#ifdef VMS
#if (__VMS_VER < 70000000)
extern "C" int unlink(char *filename);
#endif
#endif

//------------------------------------------------------------------------
// SplashFontFile
//------------------------------------------------------------------------

SplashFontFile::SplashFontFile(SplashFontFileID *idA, SplashFontSrc *srcA) {
  id = idA;
  src = srcA;
  src->ref();
  refCnt = 0;
}

SplashFontFile::~SplashFontFile() {
  src->unref();
  delete id;
}

void SplashFontFile::incRefCnt() {
  ++refCnt;
}

void SplashFontFile::decRefCnt() {
  if (!--refCnt) {
    delete this;
  }
}

//

SplashFontSrc::SplashFontSrc() {
  isFile = gFalse;
  deleteSrc = gFalse;
  fileName = NULL;
  buf = NULL;
  refcnt = 1;
}

SplashFontSrc::~SplashFontSrc() {
  if (deleteSrc) {
    if (isFile) {
      if (fileName)
	unlink(fileName->getCString());
    } else {
      if (buf)
	gfree(buf);
    }
  }

  if (isFile && fileName)
    delete fileName;
}

void SplashFontSrc::ref() {
  refcnt++;
}

void SplashFontSrc::unref() {
  if (! --refcnt)
    delete this;
}

void SplashFontSrc::setFile(GooString *file, GBool del)
{
  isFile = gTrue;
  fileName = file->copy();
  deleteSrc = del;
}

void SplashFontSrc::setFile(const char *file, GBool del)
{
  isFile = gTrue;
  fileName = new GooString(file);
  deleteSrc = del;
}

void SplashFontSrc::setBuf(char *bufA, int bufLenA, GBool del)
{
  isFile = gFalse;
  buf = bufA;
  bufLen = bufLenA;
  deleteSrc = del;
}

