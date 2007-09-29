//========================================================================
//
// SplashFTFontEngine.cc
//
//========================================================================

#include <config.h>

#if HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "goo/gmem.h"
#include "goo/GooString.h"
#include "goo/gfile.h"
#include "fofi/FoFiTrueType.h"
#include "fofi/FoFiType1C.h"
#include "SplashFTFontFile.h"
#include "SplashFTFontEngine.h"

#ifdef VMS
#if (__VMS_VER < 70000000)
extern "C" int unlink(char *filename);
#endif
#endif

//------------------------------------------------------------------------

static void fileWrite(void *stream, char *data, int len) {
  fwrite(data, 1, len, (FILE *)stream);
}

//------------------------------------------------------------------------
// SplashFTFontEngine
//------------------------------------------------------------------------

SplashFTFontEngine::SplashFTFontEngine(GBool aaA, FT_Library libA) {
  FT_Int major, minor, patch;

  aa = aaA;
  lib = libA;

  // as of FT 2.1.8, CID fonts are indexed by CID instead of GID
  FT_Library_Version(lib, &major, &minor, &patch);
  useCIDs = major > 2 ||
            (major == 2 && (minor > 1 || (minor == 1 && patch > 7)));
}

SplashFTFontEngine *SplashFTFontEngine::init(GBool aaA) {
  FT_Library libA;

  if (FT_Init_FreeType(&libA)) {
    return NULL;
  }
  return new SplashFTFontEngine(aaA, libA);
}

SplashFTFontEngine::~SplashFTFontEngine() {
  FT_Done_FreeType(lib);
}

SplashFontFile *SplashFTFontEngine::loadType1Font(SplashFontFileID *idA,
						  SplashFontSrc *src,
						  char **enc) {
  return SplashFTFontFile::loadType1Font(this, idA, src, enc);
}

SplashFontFile *SplashFTFontEngine::loadType1CFont(SplashFontFileID *idA,
						   SplashFontSrc *src,
						   char **enc) {
  return SplashFTFontFile::loadType1Font(this, idA, src, enc);
}

SplashFontFile *SplashFTFontEngine::loadCIDFont(SplashFontFileID *idA,
						SplashFontSrc *src) {
  FoFiType1C *ff;
  Gushort *cidToGIDMap;
  int nCIDs;
  SplashFontFile *ret;

  // check for a CFF font
  if (useCIDs) {
    cidToGIDMap = NULL;
    nCIDs = 0;
  } else {
    if (src->isFile) {
      ff = FoFiType1C::load(src->fileName->getCString());
    } else {
      ff = new FoFiType1C(src->buf, src->bufLen, gFalse);
    }
    if (ff) {
    cidToGIDMap = ff->getCIDToGIDMap(&nCIDs);
    delete ff;
  } else {
    cidToGIDMap = NULL;
    nCIDs = 0;
  }
  }
  ret = SplashFTFontFile::loadCIDFont(this, idA, src, cidToGIDMap, nCIDs);
  if (!ret) {
    gfree(cidToGIDMap);
  }
  return ret;
}

SplashFontFile *SplashFTFontEngine::loadTrueTypeFont(SplashFontFileID *idA,
						     SplashFontSrc *src,
						     Gushort *codeToGID,
						     int codeToGIDLen,
						     int faceIndex) {
#if 0
  FoFiTrueType *ff;
  GooString *tmpFileName;
  FILE *tmpFile;
  SplashFontFile *ret;

  if (!(ff = FoFiTrueType::load(fileName, faceIndex))) {
    return NULL;
  }
  tmpFileName = NULL;
  if (!openTempFile(&tmpFileName, &tmpFile, "wb", NULL)) {
    delete ff;
    return NULL;
  }
  ff->writeTTF(&fileWrite, tmpFile);
  delete ff;
  fclose(tmpFile);
  ret = SplashFTFontFile::loadTrueTypeFont(this, idA,
					   tmpFileName->getCString(),
					   gTrue, codeToGID, codeToGIDLen,
					   faceIndex);
  if (ret) {
    if (deleteFile) {
      unlink(fileName);
    }
  } else {
    unlink(tmpFileName->getCString());
  }
  delete tmpFileName;
  return ret;
#else
  SplashFontFile *ret;
  ret = SplashFTFontFile::loadTrueTypeFont(this, idA, src,
					   codeToGID, codeToGIDLen,
					   faceIndex);
  return ret;
#endif
}

#endif // HAVE_FREETYPE_FREETYPE_H || HAVE_FREETYPE_H
