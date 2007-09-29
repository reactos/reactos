//========================================================================
//
// DCTStream.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef DCTSTREAM_H
#define DCTSTREAM_H
#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif


#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <string.h>
#include <ctype.h>
#include "goo/gmem.h"
#include "goo/gfile.h"
#include "poppler-config.h"
#include "Error.h"
#include "Object.h"
#include "Decrypt.h"
#include "Stream.h"

extern "C" {
#include <jpeglib.h>
}

struct str_src_mgr {
    struct jpeg_source_mgr pub;
    JOCTET buffer;
    Stream *str;
    int index;
};


class DCTStream: public FilterStream {
public:

  DCTStream(Stream *strA);
  virtual ~DCTStream();
  virtual StreamKind getKind() { return strDCT; }
  virtual void reset();
  virtual int getChar();
  virtual int lookChar();
  virtual GooString *getPSFilter(int psLevel, char *indent);
  virtual GBool isBinary(GBool last = gTrue);
  Stream *getRawStream() { return str; }

private:
  void init();

  unsigned int x;
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  struct str_src_mgr src;
  JSAMPARRAY row_buffer;
};

#endif
