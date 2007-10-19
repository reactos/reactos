//========================================================================
//
// Lexer.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
// Copyright 2006 Krzysztof Kowalczyk (http://blog.kowalczyk.info)
//
//========================================================================

#ifndef LEXER_H
#define LEXER_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <assert.h>
#include "Object.h"
#include "Stream.h"

class XRef;

#define tokBufSize 128		// size of token buffer

//------------------------------------------------------------------------
// Lexer
//------------------------------------------------------------------------

class Lexer {
public:

  // Construct a lexer for a single stream.  Deletes the stream when
  // lexer is deleted.
  Lexer(XRef *xrefA, Stream *str);

  // Construct a lexer for a stream or array of streams (assumes obj
  // is either a stream or array of streams).
  Lexer(XRef *xrefA, Object *obj);

  // Destructor.
  ~Lexer();

  // Get the next object from the input stream.
  Object *getObj(Object *obj, int objNum = -1);

  // Skip to the beginning of the next line in the input stream.
  void skipToNextLine();

  // Skip over one character.
  void skipChar();

  Stream *getStream();

  // Get current position in file.  This is only used for error
  // messages, so it returns an int instead of a Guint.
  int getPos();

  // Set position in file.
  void setPos(Guint pos, int dir = 0);

  // Returns true if <c> is a whitespace character.
  static GBool isSpace(int c);

private:

  // we really want lookChar() and getChar() to be inlined
  int lookChar() {
    if (LOOK_VALUE_NOT_CACHED != lookCharLastValueCached) {
      return lookCharLastValueCached;
    }
    lookCharLastValueCached = getChar();
    return lookCharLastValueCached;
  }

  int getChar() {
    int       c;
    GBool     hasMoreData;

    if (LOOK_VALUE_NOT_CACHED != lookCharLastValueCached) {
      c = lookCharLastValueCached;
      lookCharLastValueCached = LOOK_VALUE_NOT_CACHED;
      assert( (c >= LOOK_VALUE_NOT_CACHED) && (c < 256));
      return c;
    }

    while (curStrHasGetBuf) {
      if (bufLeft > 0) {
        c = *bufCurPos++ & 0xff;
        bufLeft--;
        return c;
      }
      hasMoreData = fillBuf();
      if (!hasMoreData)
        nextStream();
    }

    c = EOF;
    while (!curStr.isNone() && (c = curStr.streamGetChar()) == EOF) {
      nextStream();
    }
    return c;
  }

  void  nextStream();
  GBool fillBuf();

  Array *   streams;                // array of input streams
  int       strPtr;                 // index of current stream
  Object    curStr;                 // current stream
  GBool     freeArray;              // should lexer free the streams array?
  char      tokBuf[tokBufSize];     // temporary token buffer
  XRef *    xref;

  // often (e.g. ~30% on PDF Refernce 1.6 pdf file from Adobe site) getChar
  // is called right after lookChar. In order to avoid expensive re-doing
  // getChar() of underlying stream, we cache the last value found by
  // lookChar() in lookCharLastValueCached. A special value
  // LOOK_VALUE_NOT_CACHED that should never be part of stream indicates
  // that no value was cached
  static const int LOOK_VALUE_NOT_CACHED = -3;
  int       lookCharLastValueCached;

  GBool     curStrHasGetBuf;     // does current stream support GetBuf() ?
  char *    buf;
  char *    bufCurPos;
  int       bufSize;
  int       bufLeft;
};

#endif
