//========================================================================
//
// BaseFile.h
//
// Copyright 1999 Derek B. Noonburg assigned by Michael Meeks.
//
//========================================================================

#ifndef BASEFILE_H
#define BASEFILE_H

#include <stdio.h>
#include <stdarg.h>

#include "Error.h"

typedef FILE * BaseFile;

static inline BaseFile
bxpdfopen (GooString *fileName1)
{
  GooString *fileName2;
  // try to open file
  fileName2 = NULL;
  BaseFile file;

#ifdef VMS
  if (!(file = fopen(fileName->getCString(), "rb", "ctx=stm"))) {
    error(-1, "Couldn't open file '%s'", fileName->getCString());
    return NULL;
  }
#else
  if (!(file = fopen(fileName1->getCString(), "rb"))) {
    fileName2 = fileName1->copy();
    fileName2->lowerCase();
    if (!(file = fopen(fileName2->getCString(), "rb"))) {
      fileName2->upperCase();
      if (!(file = fopen(fileName2->getCString(), "rb"))) {
	error(-1, "Couldn't open file '%s'", fileName1->getCString());
	delete fileName2;
	return NULL;
      }
    }
    delete fileName2;
  }
#endif
  return file;
}

static inline void
bfclose (BaseFile file)
{
  fclose (file);
}

static inline size_t
bfread (void *ptr, size_t size, size_t nmemb, BaseFile file)
{
  return fread (ptr, size, nmemb, file);
}

static inline int
bfseek (BaseFile file, long offset, int whence)
{
  return fseek (file, offset, whence);
}

static inline void
brewind (BaseFile file)
{
  rewind (file);
}

static inline long
bftell (BaseFile file)
{
  return ftell (file);
}*/

#endif /* BASEFILE_H */


