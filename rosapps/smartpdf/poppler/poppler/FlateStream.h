//========================================================================
//
// FlateStream.h
//
// Copyright (C) 2005, Jeff Muizelaar
//
//========================================================================

#ifndef FLATESTREAM_H
#define FLATESTREAM_H
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
#include <zlib.h>
}

class FlateStream: public FilterStream {
public:

  FlateStream(Stream *strA, int predictor, int columns, int colors, int bits);
  virtual ~FlateStream();
  virtual StreamKind getKind() { return strFlate; }
  virtual void reset();
  virtual int getChar();
  virtual int lookChar();
  virtual GooString *getPSFilter(int psLevel, char *indent);
  virtual GBool isBinary(GBool last = gTrue);

  int getRawChar() {
    if (fill_buffer())
      return EOF;
  
    return out_buf[out_pos++];
  }

private:
  int fill_buffer(void);
  z_stream d_stream;
  StreamPredictor *pred;
  int status;
  /* in_buf currently needs to be 1 or we over read from EmbedStreams */
  unsigned char in_buf[1];
  unsigned char out_buf[4096];
  int out_pos;
  int out_buf_len;
};

#endif
