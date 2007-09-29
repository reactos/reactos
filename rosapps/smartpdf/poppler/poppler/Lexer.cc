//========================================================================
//
// Lexer.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
// Copyright 2006 Krzysztof Kowalczyk (http://blog.kowalczyk.info)
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "Lexer.h"
#include "Error.h"
#include "XRef.h"

//------------------------------------------------------------------------

// A '1' in this array means the character is white space.  A '1' or
// '2' means the character ends a name or command.
static char specialChars[256] = {
  1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0,   // 0x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 1x
  1, 0, 0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 2,   // 2x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0,   // 3x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 4x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0,   // 5x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 6x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0,   // 7x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 8x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 9x
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // ax
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // bx
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // cx
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // dx
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // ex
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0    // fx
};

#define IS_DIGIT(c) (((c >= '0') && (c <= '9')) ? 1 : 0)

//------------------------------------------------------------------------
// Lexer
//------------------------------------------------------------------------

Lexer::Lexer(XRef *xrefA, Stream *str) {
  Object obj;

  lookCharLastValueCached = LOOK_VALUE_NOT_CACHED;
  xref = xrefA;

  buf = NULL;
  bufCurPos = NULL;
  bufSize = 0;
  bufLeft = 0;
  curStr.initStream(str);
  streams = new Array(xref);
  streams->add(curStr.copy(&obj));
  strPtr = 0;
  freeArray = gTrue;
  curStr.streamReset();
  curStrHasGetBuf = curStr.getStream()->hasGetBuf();
}

Lexer::Lexer(XRef *xrefA, Object *obj) {
  Object obj2;

  lookCharLastValueCached = LOOK_VALUE_NOT_CACHED;
  xref = xrefA;
  curStrHasGetBuf = gFalse;
  buf = NULL;
  bufCurPos = NULL;
  bufSize = 0;
  bufLeft = 0;

  if (obj->isStream()) {
    streams = new Array(xref);
    freeArray = gTrue;
    streams->add(obj->copy(&obj2));
  } else {
    assert(obj->isArray());
    streams = obj->getArray();
    freeArray = gFalse;
  }
  strPtr = 0;
  if (streams->getLength() > 0) {
    streams->get(strPtr, &curStr);
    curStr.streamReset();
    curStrHasGetBuf = curStr.getStream()->hasGetBuf();
  }
}

Lexer::~Lexer() {
  if (!curStr.isNone()) {
    curStr.streamClose();
    curStr.free();
  }
  if (freeArray) {
    delete streams;
  }
}

GBool Lexer::fillBuf() {
  assert(curStrHasGetBuf);
  assert(curStr.getStream()->hasGetBuf());
  assert(0 == bufLeft);

  GBool hasData = curStr.getStream()->getBuf(&buf, &bufSize);
  if (!hasData)
    return gFalse;
  assert(bufSize > 0);
  bufCurPos = buf;
  bufLeft = bufSize;
  return gTrue;
}

void Lexer::nextStream() {
  curStr.streamClose();
  curStr.free();
  ++strPtr;
  curStrHasGetBuf = gFalse; // important for getChar() while (curStrHasGetBuf) correct break
  if (strPtr < streams->getLength()) {
    streams->get(strPtr, &curStr);
    curStr.streamReset();
    curStrHasGetBuf = curStr.getStream()->hasGetBuf();
  }
}

Stream *Lexer::getStream() 
{
  if (curStr.isNone())
    return (Stream *)NULL;

  if (bufLeft > 0) {
    // the caller expects sequential stream but we have buffered some stuff
    // so we have to give it back to the stream
    assert(curStrHasGetBuf);
    curStr.getStream()->ungetBuf(bufLeft);
    bufLeft = 0;
    lookCharLastValueCached = LOOK_VALUE_NOT_CACHED;
  }
  return curStr.getStream();
}

int Lexer::getPos() {
  Guint pos;

  if (curStr.isNone())
    return -1;

  pos = curStr.streamGetPos();
  if (curStrHasGetBuf) {
    pos -= bufLeft;
  }

  if (LOOK_VALUE_NOT_CACHED != lookCharLastValueCached)
    --pos;
  return (int)pos;
}

void Lexer::setPos(Guint pos, int dir) {
  if (curStr.isNone())
    return;
  curStr.streamSetPos(pos, dir);
  lookCharLastValueCached = Lexer::LOOK_VALUE_NOT_CACHED;
  bufLeft = 0;
}

void Lexer::skipChar() {
  getChar(); 
}

Object *Lexer::getObj(Object *obj, int objNum) {
  char *p;
  int c, c2;
  GBool comment, neg, done;
  int numParen;
  int xi;
  double xf, scale;
  GooString *s;
  int n, m;

  // skip whitespace and comments
  comment = gFalse;
  while (1) {
    if ((c = getChar()) == EOF) {
      return obj->initEOF();
    }
    if (comment) {
      if (c == '\r' || c == '\n')
        comment = gFalse;
    } else if (c == '%') {
      comment = gTrue;
    } else if (specialChars[c] != 1) {
      break;
    }
  }

  // start reading token
  switch (c) {

  // number
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
  case '-': case '.':
    neg = gFalse;
    xi = 0;
    if (c == '-') {
      neg = gTrue;
    } else if (c == '.') {
      goto doReal;
    } else {
      xi = c - '0';
    }
    while (1) {
      c = lookChar();
      if (IS_DIGIT(c)) {
        skipChar();
        xi = xi * 10 + (c - '0');
      } else if (c == '.') {
        skipChar();
        goto doReal;
      } else {
        break;
      }
    }
    if (neg)
      xi = -xi;
    obj->initInt(xi);
    break;
  doReal:
    xf = xi;
    scale = 0.1;
    while (1) {
      c = lookChar();
      if (c == '-') {
        // ignore minus signs in the middle of numbers to match
        // Adobe's behavior
        error(getPos(), "Badly formatted number");
        skipChar();
        continue;
      }
      if (!IS_DIGIT(c)) {
        break;
      }
      skipChar();
      xf = xf + scale * (c - '0');
      scale *= 0.1;
    }
    if (neg)
      xf = -xf;
    obj->initReal(xf);
    break;

  // string
  case '(':
    p = tokBuf;
    n = 0;
    numParen = 1;
    done = gFalse;
    s = NULL;
    do {
      c2 = EOF;
      switch (c = getChar()) {

      case EOF:
#if 0
      // This breaks some PDF files, e.g., ones from Photoshop.
      case '\r':
      case '\n':
#endif
        error(getPos(), "Unterminated string");
        done = gTrue;
        break;

      case '(':
        ++numParen;
        c2 = c;
        break;

      case ')':
        if (--numParen == 0) {
          done = gTrue;
        } else {
          c2 = c;
        }
        break;

      case '\\':
        switch (c = getChar()) {
        case 'n':
          c2 = '\n';
          break;
        case 'r':
          c2 = '\r';
          break;
        case 't':
          c2 = '\t';
          break;
        case 'b':
          c2 = '\b';
          break;
        case 'f':
          c2 = '\f';
          break;
        case '\\':
        case '(':
        case ')':
          c2 = c;
          break;
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
          c2 = c - '0';
          c = lookChar();
          if (c >= '0' && c <= '7') {
            skipChar();
            c2 = (c2 << 3) + (c - '0');
            c = lookChar();
            if (c >= '0' && c <= '7') {
              skipChar();
              c2 = (c2 << 3) + (c - '0');
            }
          }
          break;
        case '\r':
          c = lookChar();
          if (c == '\n') {
            skipChar();
          }
          break;
        case '\n':
          break;
        case EOF:
          error(getPos(), "Unterminated string");
          done = gTrue;
          break;
        default:
          c2 = c;
          break;
        }
        break;

      default:
        c2 = c;
        break;
      }

      if (c2 != EOF) {
        if (n == tokBufSize) {
          if (!s)
            s = new GooString(tokBuf, tokBufSize);
          else
            s->append(tokBuf, tokBufSize);
          p = tokBuf;
          n = 0;
          
          // we are growing see if the document is not malformed and we are growing too much
          if (objNum != -1)
          {
            int newObjNum = xref->getNumEntry(getPos());
            if (newObjNum != objNum)
            {
              error(getPos(), "Unterminated string");
              done = gTrue;
            }
          }
        }
        *p++ = (char)c2;
        ++n;
      }
    } while (!done);
    if (!s)
      s = new GooString(tokBuf, n);
    else
      s->append(tokBuf, n);
    obj->initString(s);
    break;

  // name
  case '/':
    p = tokBuf;
    n = 0;
    while ((c = lookChar()) != EOF && !specialChars[c]) {
      skipChar();
      if (c == '#') {
        c2 = lookChar();
        if (c2 >= '0' && c2 <= '9') {
          c = c2 - '0';
        } else if (c2 >= 'A' && c2 <= 'F') {
          c = c2 - 'A' + 10;
        } else if (c2 >= 'a' && c2 <= 'f') {
          c = c2 - 'a' + 10;
        } else {
          goto notEscChar;
        }
        skipChar();
        c <<= 4;
        c2 = getChar();
        if (c2 >= '0' && c2 <= '9') {
          c += c2 - '0';
        } else if (c2 >= 'A' && c2 <= 'F') {
          c += c2 - 'A' + 10;
        } else if (c2 >= 'a' && c2 <= 'f') {
          c += c2 - 'a' + 10;
        } else {
          error(getPos(), "Illegal digit in hex char in name");
        }
      }
     notEscChar:
      if (++n == tokBufSize) {
        error(getPos(), "Name token too long");
        break;
      }
      *p++ = c;
    }
    *p = '\0';
    obj->initName(tokBuf);
    break;

  // array punctuation
  case '[':
  case ']':
    tokBuf[0] = c;
    tokBuf[1] = '\0';
    obj->initCmd(tokBuf);
    break;

  // hex string or dict punctuation
  case '<':
    c = lookChar();

    // dict punctuation
    if (c == '<') {
      skipChar();
      tokBuf[0] = tokBuf[1] = '<';
      tokBuf[2] = '\0';
      obj->initCmd(tokBuf);

    // hex string
    } else {
      p = tokBuf;
      m = n = 0;
      c2 = 0;
      s = NULL;
      while (1) {
        c = getChar();
        if (c == '>') {
          break;
        } else if (c == EOF) {
          error(getPos(), "Unterminated hex string");
          break;
        } else if (specialChars[c] != 1) {
          c2 = c2 << 4;
          if (c >= '0' && c <= '9')
            c2 += c - '0';
          else if (c >= 'A' && c <= 'F')
            c2 += c - 'A' + 10;
          else if (c >= 'a' && c <= 'f')
            c2 += c - 'a' + 10;
          else
            error(getPos(), "Illegal character <%02x> in hex string", c);
          if (++m == 2) {
            if (n == tokBufSize) {
              if (!s)
                s = new GooString(tokBuf, tokBufSize);
              else
                s->append(tokBuf, tokBufSize);
              p = tokBuf;
              n = 0;
            }
            *p++ = (char)c2;
            ++n;
            c2 = 0;
            m = 0;
          }
        }
      }
      if (!s)
        s = new GooString(tokBuf, n);
      else
        s->append(tokBuf, n);
      if (m == 1)
        s->append((char)(c2 << 4));
      obj->initString(s);
    }
    break;

  // dict punctuation
  case '>':
    c = lookChar();
    if (c == '>') {
      skipChar();
      tokBuf[0] = tokBuf[1] = '>';
      tokBuf[2] = '\0';
      obj->initCmd(tokBuf);
    } else {
      error(getPos(), "Illegal character '>'");
      obj->initError();
    }
    break;

  // error
  case ')':
  case '{':
  case '}':
    error(getPos(), "Illegal character '%c'", c);
    obj->initError();
    break;

  // command
  default:
    p = tokBuf;
    *p++ = c;
    n = 1;
    while ((c = lookChar()) != EOF && !specialChars[c]) {
      skipChar();
      if (++n == tokBufSize) {
        error(getPos(), "Command token too long");
        break;
      }
      *p++ = c;
    }
    *p = '\0';
    if (tokBuf[0] == 't' && !strcmp(tokBuf, "true")) {
      obj->initBool(gTrue);
    } else if (tokBuf[0] == 'f' && !strcmp(tokBuf, "false")) {
      obj->initBool(gFalse);
    } else if (tokBuf[0] == 'n' && !strcmp(tokBuf, "null")) {
      obj->initNull();
    } else {
      obj->initCmd(tokBuf);
    }
    break;
  }

  return obj;
}

void Lexer::skipToNextLine() {
  int c;

  while (1) {
    c = getChar();
    if (c == EOF || c == '\n') {
      return;
    }
    if (c == '\r') {
      if ((c = lookChar()) == '\n') {
        skipChar();
      }
      return;
    }
  }
}

GBool Lexer::isSpace(int c) {
  return c >= 0 && c <= 0xff && specialChars[c] == 1;
}
