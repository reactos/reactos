//========================================================================
//
// SplashT1FontFile.cc
//
//========================================================================

#include <config.h>

#if HAVE_T1LIB_H

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <string.h>
#include <t1lib.h>
#include "goo/gmem.h"
#include "SplashT1FontEngine.h"
#include "SplashT1Font.h"
#include "SplashT1FontFile.h"

//------------------------------------------------------------------------
// SplashT1FontFile
//------------------------------------------------------------------------

SplashFontFile *SplashT1FontFile::loadType1Font(SplashT1FontEngine *engineA,
						SplashFontFileID *idA,
						SplashFontSrc *src,
						char **encA) {
  int t1libIDA;
  char **encTmp;
  char *encStrTmp;
  int encStrSize;
  char *encPtr;
  int i;
  GString *fileNameA;
  SplashFontSrc *newsrc = NULL;
  SplashFontFile *ff;

  if (! src->isFile) {
    GString *tmpFileName;
    FILE *tmpFile;
    if (!openTempFile(&tmpFileName, &tmpFile, "wb", NULL))
      return NULL;
    fwrite(src->buf, 1, src->bufLen, tmpFile);
    fclose(tmpFile);
    newsrc = new SplashFontSrc;
    newsrc->setFile(tmpFileName, gTrue);
    src = newsrc;
    delete tmpFileName;
  }
  fileNameA = src->fileName;
  // load the font file
  if ((t1libIDA = T1_AddFont(fileNameA)) < 0) {
    if (newsrc)
      delete newsrc;
    return NULL;
  }
  T1_LoadFont(t1libIDA);

  // reencode it
  encStrSize = 0;
  for (i = 0; i < 256; ++i) {
    if (encA[i]) {
      encStrSize += strlen(encA[i]) + 1;
    }
  }
  encTmp = (char **)gmallocn(257, sizeof(char *));
  encStrTmp = (char *)gmallocn(encStrSize, sizeof(char));
  encPtr = encStrTmp;
  for (i = 0; i < 256; ++i) {
    if (encA[i]) {
      strcpy(encPtr, encA[i]);
      encTmp[i] = encPtr;
      encPtr += strlen(encPtr) + 1;
    } else {
      encTmp[i] = ".notdef";
    }
  }
  encTmp[256] = "custom";
  T1_ReencodeFont(t1libIDA, encTmp);

  ff = new SplashT1FontFile(engineA, idA, src,
			      t1libIDA, encTmp, encStrTmp);
  if (newsrc)
    newsrc->unref();
  return ff;
}

SplashT1FontFile::SplashT1FontFile(SplashT1FontEngine *engineA,
				   SplashFontFileID *idA,
				   SplashFontSrc *srcA,
				   int t1libIDA, char **encA, char *encStrA):
  SplashFontFile(idA, srcA)
{
  engine = engineA;
  t1libID = t1libIDA;
  enc = encA;
  encStr = encStrA;
}

SplashT1FontFile::~SplashT1FontFile() {
  gfree(encStr);
  gfree(enc);
  T1_DeleteFont(t1libID);
}

SplashFont *SplashT1FontFile::makeFont(SplashCoord *mat) {
  SplashFont *font;

  font = new SplashT1Font(this, mat);
  font->initCache();
  return font;
}

#endif // HAVE_T1LIB_H
