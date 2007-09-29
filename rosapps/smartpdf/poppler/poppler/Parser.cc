//========================================================================
//
// Parser.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stddef.h>
#include "Object.h"
#include "Array.h"
#include "Dict.h"
#include "Parser.h"
#include "XRef.h"
#include "Error.h"
#include "Decrypt.h"
#include "UGooString.h"

Parser::Parser(XRef *xrefA, Lexer *lexerA) {
  xref = xrefA;
  lexer = lexerA;
  inlineImg = 0;
  lexer->getObj(&buf1);
  lexer->getObj(&buf2);
}

Parser::~Parser() {
  buf1.free();
  buf2.free();
  delete lexer;
}

Object *Parser::getObj(Object *obj,
                       Guchar *fileKey, int keyLength,
                       int objNum, int objGen) {
  Stream *str;
  Object obj2;
  int num;
  Decrypt *decrypt;
  GooString *s;
  char *p;
  int i;

  // refill buffer after inline image data
  if (inlineImg == 2) {
    buf1.free();
    buf2.free();
    lexer->getObj(&buf1);
    lexer->getObj(&buf2);
    inlineImg = 0;
  }

  // array
  if (buf1.isCmd("[")) {
    shift();
    obj->initArray(xref);
    while (!buf1.isCmd("]") && !buf1.isEOF())
      obj->arrayAdd(getObj(&obj2, fileKey, keyLength, objNum, objGen));
    if (buf1.isEOF())
      error(getPos(), "End of file inside array");
    shift();

  // dictionary or stream
  } else if (buf1.isCmd("<<")) {
    shift(objNum);
    obj->initDict(xref);
    while (!buf1.isCmd(">>") && !buf1.isEOF()) {
      if (!buf1.isName()) {
        error(getPos(), "Dictionary key must be a name object");
        shift();
      } else {
        // buf1 might go away in shift(), so construct the key
        UGooString *key = new UGooString(buf1.getNameC());
        shift();
        if (buf1.isEOF() || buf1.isError()) {
          delete key;
          break;
        }
        obj->dictAddOwnKeyVal(key, getObj(&obj2, fileKey, keyLength, objNum, objGen));
      }
    }
    if (buf1.isEOF())
      error(getPos(), "End of file inside dictionary");
    if (buf2.isCmd("stream")) {
      if ((str = makeStream(obj))) {
        obj->initStream(str);
        if (fileKey) {
          str->getBaseStream()->doDecryption(fileKey, keyLength,
                                             objNum, objGen);
        }
      } else {
        obj->free();
        obj->initError();
      }
    } else {
      shift();
    }

  // indirect reference or integer
  } else if (buf1.isInt()) {
    num = buf1.getInt();
    shift();
    if (buf1.isInt() && buf2.isCmd("R")) {
      obj->initRef(num, buf1.getInt());
      shift();
      shift();
    } else {
      obj->initInt(num);
    }

  // string
  } else if (buf1.isString() && fileKey) {
    buf1.copy(obj);
    s = obj->getString();
    decrypt = new Decrypt(fileKey, keyLength, objNum, objGen);
    for (i = 0, p = obj->getString()->getCString();
      i < s->getLength();
      ++i, ++p) {
      *p = decrypt->decryptByte(*p);
    }
    delete decrypt;
    shift();

  // simple object
  } else {
    // avoid re-allocating memory for complex objects like strings by
    // shallow copy of <buf1> to <obj> and nulling <buf1> so that
    // subsequent buf1.free() won't free this memory
    buf1.shallowCopy(obj);
    buf1.initNull();
    shift();
  }

  return obj;
}

Stream *Parser::makeStream(Object *dict) {
  Object obj;
  BaseStream *baseStr;
  Stream *str;
  Guint pos, endPos, length;

  // get stream start position
  lexer->skipToNextLine();
  pos = lexer->getPos();

  // get length
  dict->dictLookup("Length", &obj);
  if (obj.isInt()) {
    length = (Guint)obj.getInt();
    obj.free();
  } else {
    error(getPos(), "Bad 'Length' attribute in stream");
    obj.free();
    return NULL;
  }

  // check for length in damaged file
  if (xref && xref->getStreamEnd(pos, &endPos)) {
    length = endPos - pos;
  }

  // in badly damaged PDF files, we can run off the end of the input
  // stream immediately after the "stream" token
  if (!lexer->getStream()) {
    return NULL;
  }
  baseStr = lexer->getStream()->getBaseStream();
  lexer->setPos(pos + length);
  // refill token buffers and check for 'endstream'
  shift();  // kill '>>'
  shift();  // kill 'stream'
  if (buf1.isCmd("endstream")) {
    shift();
  } else {
    error(getPos(), "Missing 'endstream'");
    // kludge for broken PDF files: just add 5k to the length, and
    // hope its enough
    length += 5000;
  }

  // make base stream
  str = baseStr->makeSubStream(pos, gTrue, length, dict);

  // get filters
  str = str->addFilters(dict);

  return str;
}

void Parser::shift(int objNum) {
  if (inlineImg > 0) {
    if (inlineImg < 2) {
      ++inlineImg;
    } else {
      // in a damaged content stream, if 'ID' shows up in the middle
      // of a dictionary, we need to reset
      inlineImg = 0;
    }
  } else if (buf2.isCmd("ID")) {
    lexer->skipChar();		// skip char after 'ID' command
    inlineImg = 1;
  }
  buf1.free();
  buf2.shallowCopy(&buf1);
  if (inlineImg > 0)		// don't buffer inline image data
    buf2.initNull();
  else
    lexer->getObj(&buf2, objNum);
}
