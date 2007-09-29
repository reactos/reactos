 //========================================================================
//
// XRef.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
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
#include "goo/gmem.h"
#include "Object.h"
#include "Stream.h"
#include "Lexer.h"
#include "Parser.h"
#include "Dict.h"
#include "Error.h"
#include "ErrorCodes.h"
#include "UGooString.h"
#include "XRef.h"

//------------------------------------------------------------------------

#define xrefSearchSize 1024	// read this many bytes at end of file
                                //   to look for 'startxref'

//------------------------------------------------------------------------
// Permission bits
// Note that the PDF spec uses 1 base (eg bit 3 is 1<<2)
//------------------------------------------------------------------------

#define permPrint         (1<<2)  // bit 3
#define permChange        (1<<3)  // bit 4
#define permCopy          (1<<4)  // bit 5
#define permNotes         (1<<5)  // bit 6
#define permFillForm      (1<<8)  // bit 9
#define permAccessibility (1<<9)  // bit 10
#define permAssemble      (1<<10) // bit 11
#define permHighResPrint  (1<<11) // bit 12
#define defPermFlags 0xfffc

//------------------------------------------------------------------------
// ObjectStream
//------------------------------------------------------------------------

class ObjectStream {
public:

  // Create an object stream, using object number <objStrNum>,
  // generation 0.
  ObjectStream(XRef *xref, int objStrNumA);

  ~ObjectStream();

  // Return the object number of this object stream.
  int getObjStrNum() { return objStrNum; }

  // Get the <objIdx>th object from this stream, which should be
  // object number <objNum>, generation 0.
  Object *getObject(int objIdx, int objNum, Object *obj);

private:

  int objStrNum;		// object number of the object stream
  int nObjects;			// number of objects in the stream
  Object *objs;			// the objects (length = nObjects)
  int *objNums;			// the object numbers (length = nObjects)
};

ObjectStream::ObjectStream(XRef *xref, int objStrNumA) {
  Stream *str;
  Parser *parser;
  int *offsets;
  Object objStr, obj1, obj2;
  int first, i;

  objStrNum = objStrNumA;
  nObjects = 0;
  objs = NULL;
  objNums = NULL;

  if (!xref->fetch(objStrNum, 0, &objStr)->isStream()) {
    goto err1;
  }

  if (!objStr.streamGetDict()->lookup("N", &obj1)->isInt()) {
    obj1.free();
    goto err1;
  }
  nObjects = obj1.getInt();
  obj1.free();
  if (nObjects <= 0) {
    goto err1;
  }

  if (!objStr.streamGetDict()->lookup("First", &obj1)->isInt()) {
    obj1.free();
    goto err1;
  }
  first = obj1.getInt();
  obj1.free();
  if (first < 0) {
    goto err1;
  }

  if (nObjects*(int)sizeof(int)/sizeof(int) != nObjects) {
    error(-1, "Invalid 'nObjects'");
    goto err1;
  }
 
  objs = new Object[nObjects];
  objNums = (int *)gmallocn(nObjects, sizeof(int));
  offsets = (int *)gmallocn(nObjects, sizeof(int));

  // parse the header: object numbers and offsets
  objStr.streamReset();
  obj1.initNull();
  str = new EmbedStream(objStr.getStream(), &obj1, gTrue, first);
  parser = new Parser(xref, new Lexer(xref, str));
  for (i = 0; i < nObjects; ++i) {
    parser->getObj(&obj1);
    parser->getObj(&obj2);
    if (!obj1.isInt() || !obj2.isInt()) {
      obj1.free();
      obj2.free();
      delete parser;
      gfree(offsets);
      goto err1;
    }
    objNums[i] = obj1.getInt();
    offsets[i] = obj2.getInt();
    obj1.free();
    obj2.free();
    if (objNums[i] < 0 || offsets[i] < 0 ||
        (i > 0 && offsets[i] < offsets[i-1])) {
      delete parser;
      gfree(offsets);
      goto err1;
    }
  }
  while (str->getChar() != EOF) ;
  delete parser;

  // skip to the first object - this shouldn't be necessary because
  // the First key is supposed to be equal to offsets[0], but just in
  // case...
  for (i = first; i < offsets[0]; ++i) {
    objStr.getStream()->getChar();
  }

  // parse the objects
  for (i = 0; i < nObjects; ++i) {
    obj1.initNull();
    if (i == nObjects - 1) {
      str = new EmbedStream(objStr.getStream(), &obj1, gFalse, 0);
    } else {
      str = new EmbedStream(objStr.getStream(), &obj1, gTrue,
                            offsets[i+1] - offsets[i]);
    }
    parser = new Parser(xref, new Lexer(xref, str));
    parser->getObj(&objs[i]);
    while (str->getChar() != EOF) ;
    delete parser;
  }

  gfree(offsets);

 err1:
  objStr.free();
  return;
}

ObjectStream::~ObjectStream() {
  int i;

  if (objs) {
    for (i = 0; i < nObjects; ++i) {
      objs[i].free();
    }
    delete[] objs;
  }
  gfree(objNums);
}

Object *ObjectStream::getObject(int objIdx, int objNum, Object *obj) {
  if (objIdx < 0 || objIdx >= nObjects || objNum != objNums[objIdx]) {
    return obj->initNull();
  }
  return objs[objIdx].copy(obj);
}

//------------------------------------------------------------------------
// XRef
//------------------------------------------------------------------------

XRef::XRef(BaseStream *strA) {
  Guint pos;
  Object obj;

  ok = gTrue;
  errCode = errNone;
  size = 0;
  entries = NULL;
  streamEnds = NULL;
  streamEndsLen = 0;
  objStr = NULL;

  encrypted = gFalse;
  permFlags = defPermFlags;
  ownerPasswordOk = gFalse;

  // read the trailer
  str = strA;
  start = str->getStart();
  pos = getStartXref();

  // if there was a problem with the 'startxref' position, try to
  // reconstruct the xref table
  if (pos == 0) {
    if (!(ok = constructXRef())) {
      errCode = errDamaged;
      return;
    }

  // read the xref table
  } else {
    while (readXRef(&pos)) ;

    // if there was a problem with the xref table,
    // try to reconstruct it
    if (!ok) {
      if (!(ok = constructXRef())) {
        errCode = errDamaged;
        return;
      }
    }
  }

  // get the root dictionary (catalog) object
  trailerDict.dictLookupNF("Root", &obj);
  if (obj.isRef()) {
    rootNum = obj.getRefNum();
    rootGen = obj.getRefGen();
    obj.free();
  } else {
    obj.free();
    if (!(ok = constructXRef())) {
      errCode = errDamaged;
      return;
    }
  }

  // now set the trailer dictionary's xref pointer so we can fetch
  // indirect objects from it
  trailerDict.getDict()->setXRef(this);
}

XRef::~XRef() {
  gfree(entries);
  trailerDict.free();
  if (streamEnds) {
    gfree(streamEnds);
  }
  if (objStr) {
    delete objStr;
  }
}

// Read the 'startxref' position.
Guint XRef::getStartXref() {
  char buf[xrefSearchSize+1];
  char *p;
  int c, n, i;

  // read last xrefSearchSize bytes
  str->setPos(xrefSearchSize, -1);
  for (n = 0; n < xrefSearchSize; ++n) {
    if ((c = str->getChar()) == EOF) {
      break;
    }
    buf[n] = c;
  }
  buf[n] = '\0';

  // find startxref
  for (i = n - 9; i >= 0; --i) {
    if (!strncmp(&buf[i], "startxref", 9)) {
      break;
    }
  }
  if (i < 0) {
    return 0;
  }
  for (p = &buf[i+9]; isspace(*p); ++p) ;
  lastXRefPos = strToUnsigned(p);

  return lastXRefPos;
}

// Read one xref table section.  Also reads the associated trailer
// dictionary, and returns the prev pointer (if any).
GBool XRef::readXRef(Guint *pos) {
  Parser *parser;
  Object obj;
  GBool more;

  // start up a parser, parse one token
  obj.initNull();
  parser = new Parser(NULL,
             new Lexer(NULL,
               str->makeSubStream(start + *pos, gFalse, 0, &obj)));
  parser->getObj(&obj);

  // parse an old-style xref table
  if (obj.isCmd("xref")) {
    obj.free();
    more = readXRefTable(parser, pos);

  // parse an xref stream
  } else if (obj.isInt()) {
    obj.free();
    if (!parser->getObj(&obj)->isInt()) {
      goto err1;
    }
    obj.free();
    if (!parser->getObj(&obj)->isCmd("obj")) {
      goto err1;
    }
    obj.free();
    if (!parser->getObj(&obj)->isStream()) {
      goto err1;
    }
    more = readXRefStream(obj.getStream(), pos);
    obj.free();

  } else {
    goto err1;
  }

  delete parser;
  return more;

 err1:
  obj.free();
  delete parser;
  ok = gFalse;
  return gFalse;
}

GBool XRef::readXRefTable(Parser *parser, Guint *pos) {
  XRefEntry entry;
  GBool more;
  Object obj, obj2;
  Guint pos2;
  int first, n, newSize, i;

  while (1) {
    parser->getObj(&obj);
    if (obj.isCmd("trailer")) {
      obj.free();
      break;
    }
    if (!obj.isInt()) {
      goto err1;
    }
    first = obj.getInt();
    obj.free();
    if (!parser->getObj(&obj)->isInt()) {
      goto err1;
    }
    n = obj.getInt();
    obj.free();
    if (first < 0 || n < 0 || first + n < 0) {
      goto err1;
    }
    if (first + n > size) {
      for (newSize = size ? 2 * size : 1024;
           first + n > newSize && newSize > 0;
           newSize <<= 1) ;
      if (newSize < 0) {
        goto err1;
      }
      if (newSize*(int)sizeof(XRefEntry)/sizeof(XRefEntry) != newSize) {
        error(-1, "Invalid 'obj' parameters'");
        goto err1;
      }
 
      entries = (XRefEntry *)greallocn(entries, newSize, sizeof(XRefEntry));
      for (i = size; i < newSize; ++i) {
        entries[i].offset = 0xffffffff;
        entries[i].type = xrefEntryFree;
      }
      size = newSize;
    }
    for (i = first; i < first + n; ++i) {
      if (!parser->getObj(&obj)->isInt()) {
        goto err1;
      }
      entry.offset = (Guint)obj.getInt();
      obj.free();
      if (!parser->getObj(&obj)->isInt()) {
        goto err1;
      }
      entry.gen = obj.getInt();
      obj.free();
      parser->getObj(&obj);
      if (obj.isCmd("n")) {
        entry.type = xrefEntryUncompressed;
      } else if (obj.isCmd("f")) {
        entry.type = xrefEntryFree;
      } else {
        goto err1;
      }
      obj.free();
      if (entries[i].offset == 0xffffffff) {
        entries[i] = entry;
        // PDF files of patents from the IBM Intellectual Property
        // Network have a bug: the xref table claims to start at 1
        // instead of 0.
        if (i == 1 && first == 1 &&
            entries[1].offset == 0 && entries[1].gen == 65535 &&
            entries[1].type == xrefEntryFree) {
          i = first = 0;
          entries[0] = entries[1];
          entries[1].offset = 0xffffffff;
        }
      }
    }
  }

  // read the trailer dictionary
  if (!parser->getObj(&obj)->isDict()) {
    goto err1;
  }

  // get the 'Prev' pointer
  obj.getDict()->lookupNF("Prev", &obj2);
  if (obj2.isInt()) {
    *pos = (Guint)obj2.getInt();
    more = gTrue;
  } else if (obj2.isRef()) {
    // certain buggy PDF generators generate "/Prev NNN 0 R" instead
    // of "/Prev NNN"
    *pos = (Guint)obj2.getRefNum();
    more = gTrue;
  } else {
    more = gFalse;
  }
  obj2.free();

  // save the first trailer dictionary
  if (trailerDict.isNone()) {
    obj.copy(&trailerDict);
  }

  // check for an 'XRefStm' key
  if (obj.getDict()->lookup("XRefStm", &obj2)->isInt()) {
    pos2 = (Guint)obj2.getInt();
    readXRef(&pos2);
    if (!ok) {
      obj2.free();
      goto err1;
    }
  }
  obj2.free();

  obj.free();
  return more;

 err1:
  obj.free();
  ok = gFalse;
  return gFalse;
}

GBool XRef::readXRefStream(Stream *xrefStr, Guint *pos) {
  Dict *dict;
  int w[3];
  GBool more;
  Object obj, obj2, idx;
  int newSize, first, n, i;

  dict = xrefStr->getDict();

  if (!dict->lookupNF("Size", &obj)->isInt()) {
    goto err1;
  }
  newSize = obj.getInt();
  obj.free();
  if (newSize < 0) {
    goto err1;
  }
  if (newSize > size) {
    if (newSize * (int)sizeof(XRefEntry)/sizeof(XRefEntry) != newSize) {
      error(-1, "Invalid 'size' parameter.");
      return gFalse;
    }
    entries = (XRefEntry *)greallocn(entries, newSize, sizeof(XRefEntry));
    for (i = size; i < newSize; ++i) {
      entries[i].offset = 0xffffffff;
      entries[i].type = xrefEntryFree;
    }
    size = newSize;
  }

  if (!dict->lookupNF("W", &obj)->isArray() ||
      obj.arrayGetLength() < 3) {
    goto err1;
  }
  for (i = 0; i < 3; ++i) {
    if (!obj.arrayGet(i, &obj2)->isInt()) {
      obj2.free();
      goto err1;
    }
    w[i] = obj2.getInt();
    obj2.free();
    if (w[i] < 0 || w[i] > 4) {
      goto err1;
    }
  }
  obj.free();

  xrefStr->reset();
  dict->lookupNF("Index", &idx);
  if (idx.isArray()) {
    for (i = 0; i+1 < idx.arrayGetLength(); i += 2) {
      if (!idx.arrayGet(i, &obj)->isInt()) {
        idx.free();
        goto err1;
      }
      first = obj.getInt();
      obj.free();
      if (!idx.arrayGet(i+1, &obj)->isInt()) {
        idx.free();
        goto err1;
      }
      n = obj.getInt();
      obj.free();
      if (first < 0 || n < 0 ||
          !readXRefStreamSection(xrefStr, w, first, n)) {
        idx.free();
        goto err0;
      }
    }
  } else {
    if (!readXRefStreamSection(xrefStr, w, 0, newSize)) {
      idx.free();
      goto err0;
    }
  }
  idx.free();

  dict->lookupNF("Prev", &obj);
  if (obj.isInt()) {
    *pos = (Guint)obj.getInt();
    more = gTrue;
  } else {
    more = gFalse;
  }
  obj.free();
  if (trailerDict.isNone()) {
    trailerDict.initDict(dict);
  }

  return more;

 err1:
  obj.free();
 err0:
  ok = gFalse;
  return gFalse;
}

GBool XRef::readXRefStreamSection(Stream *xrefStr, int *w, int first, int n) {
  Guint offset;
  int type, gen, c, newSize, i, j;

  if (first + n < 0) {
    return gFalse;
  }
  if (first + n > size) {
    for (newSize = size ? 2 * size : 1024;
         first + n > newSize && newSize > 0;
         newSize <<= 1) ;
    if (newSize < 0) {
      return gFalse;
    }
    if (newSize*(int)sizeof(XRefEntry)/sizeof(XRefEntry) != newSize) {
      error(-1, "Invalid 'size' inside xref table.");
      return gFalse;
    }
    entries = (XRefEntry *)greallocn(entries, newSize, sizeof(XRefEntry));
    for (i = size; i < newSize; ++i) {
      entries[i].offset = 0xffffffff;
      entries[i].type = xrefEntryFree;
    }
    size = newSize;
  }
  for (i = first; i < first + n; ++i) {
    if (w[0] == 0) {
      type = 1;
    } else {
      for (type = 0, j = 0; j < w[0]; ++j) {
        if ((c = xrefStr->getChar()) == EOF) {
          return gFalse;
        }
        type = (type << 8) + c;
      }
    }
    for (offset = 0, j = 0; j < w[1]; ++j) {
      if ((c = xrefStr->getChar()) == EOF) {
        return gFalse;
      }
      offset = (offset << 8) + c;
    }
    for (gen = 0, j = 0; j < w[2]; ++j) {
      if ((c = xrefStr->getChar()) == EOF) {
        return gFalse;
      }
      gen = (gen << 8) + c;
    }
    if (entries[i].offset == 0xffffffff) {
      switch (type) {
      case 0:
        entries[i].offset = offset;
        entries[i].gen = gen;
        entries[i].type = xrefEntryFree;
        break;
      case 1:
        entries[i].offset = offset;
        entries[i].gen = gen;
        entries[i].type = xrefEntryUncompressed;
        break;
      case 2:
        entries[i].offset = offset;
        entries[i].gen = gen;
        entries[i].type = xrefEntryCompressed;
        break;
      default:
        return gFalse;
      }
    }
  }

  return gTrue;
}

// Attempt to construct an xref table for a damaged file.
GBool XRef::constructXRef() {
  Parser *parser;
  Object newTrailerDict, obj;
  char buf[256];
  Guint pos;
  int num, gen;
  int newSize;
  int streamEndsSize;
  char *p;
  int i;
  GBool gotRoot;

  gfree(entries);
  size = 0;
  entries = NULL;

  error(0, "PDF file is damaged - attempting to reconstruct xref table...");
  gotRoot = gFalse;
  streamEndsLen = streamEndsSize = 0;

  str->reset();
  while (1) {
    pos = str->getPos();
    if (!str->getLine(buf, 256)) {
      break;
    }
    p = buf;

    // got trailer dictionary
    if (!strncmp(p, "trailer", 7)) {
      obj.initNull();
      parser = new Parser(NULL,
                 new Lexer(NULL,
                   str->makeSubStream(pos + 7, gFalse, 0, &obj)));
      parser->getObj(&newTrailerDict);
      if (newTrailerDict.isDict()) {
        newTrailerDict.dictLookupNF("Root", &obj);
        if (obj.isRef()) {
          rootNum = obj.getRefNum();
          rootGen = obj.getRefGen();
          if (!trailerDict.isNone()) {
            trailerDict.free();
          }
          newTrailerDict.copy(&trailerDict);
          gotRoot = gTrue;
        }
        obj.free();
      }
      newTrailerDict.free();
      delete parser;

    // look for object
    } else if (isdigit(*p)) {
      num = atoi(p);
      if (num > 0) {
        do {
          ++p;
        } while (*p && isdigit(*p));
        if (isspace(*p)) {
          do {
            ++p;
          } while (*p && isspace(*p));
          if (isdigit(*p)) {
            gen = atoi(p);
            do {
              ++p;
            } while (*p && isdigit(*p));
            if (isspace(*p)) {
              do {
                ++p;
              } while (*p && isspace(*p));
              if (!strncmp(p, "obj", 3)) {
                if (num >= size) {
                  newSize = (num + 1 + 255) & ~255;
                  if (newSize < 0) {
                    error(-1, "Bad object number");
                    return gFalse;
                  }
                  if (newSize*(int)sizeof(XRefEntry)/sizeof(XRefEntry) != newSize) {
                    error(-1, "Invalid 'obj' parameters.");
                    return gFalse;
                  }
                  entries = (XRefEntry *)
                      greallocn(entries, newSize, sizeof(XRefEntry));
                  for (i = size; i < newSize; ++i) {
                    entries[i].offset = 0xffffffff;
                    entries[i].type = xrefEntryFree;
                  }
                  size = newSize;
                }
                if (entries[num].type == xrefEntryFree ||
                    gen >= entries[num].gen) {
                  entries[num].offset = pos - start;
                  entries[num].gen = gen;
                  entries[num].type = xrefEntryUncompressed;
                }
              }
            }
          }
        }
      }

    } else if (!strncmp(p, "endstream", 9)) {
      if (streamEndsLen == streamEndsSize) {
        streamEndsSize += 64;
        if (streamEndsSize*(int)sizeof(int)/sizeof(int) != streamEndsSize) {
          error(-1, "Invalid 'endstream' parameter.");
          return gFalse;
        }
        streamEnds = (Guint *)greallocn(streamEnds,
                                        streamEndsSize, sizeof(int));
      }
      streamEnds[streamEndsLen++] = pos;
    }
  }

  if (gotRoot)
    return gTrue;

  error(-1, "Couldn't find trailer dictionary");
  return gFalse;
}

void XRef::setEncryption(int permFlagsA, GBool ownerPasswordOkA,
                         Guchar *fileKeyA, int keyLengthA,
                         int encVersionA, int encRevisionA) {
  int i;

  encrypted = gTrue;
  permFlags = permFlagsA;
  ownerPasswordOk = ownerPasswordOkA;
  if (keyLengthA <= 16) {
    keyLength = keyLengthA;
  } else {
    keyLength = 16;
  }
  for (i = 0; i < keyLength; ++i) {
    fileKey[i] = fileKeyA[i];
  }
  encVersion = encVersionA;
  encRevision = encRevisionA;
}

GBool XRef::okToPrint(GBool ignoreOwnerPW) {
  return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permPrint);
}

// we can print at high res if we are only doing security handler revision
// 2 (and we are allowed to print at all), or with security handler rev
// 3 and we are allowed to print, and bit 12 is set.
GBool XRef::okToPrintHighRes(GBool ignoreOwnerPW) {
  if (2 == encRevision) {
    return (okToPrint(ignoreOwnerPW));
  } else if (encRevision >= 3) {
    return (okToPrint(ignoreOwnerPW) && (permFlags & permHighResPrint));
  } else {
    // something weird - unknown security handler version
    return gFalse;
  }
}

GBool XRef::okToChange(GBool ignoreOwnerPW) {
  return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permChange);
}

GBool XRef::okToCopy(GBool ignoreOwnerPW) {
  return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permCopy);
}

GBool XRef::okToAddNotes(GBool ignoreOwnerPW) {
  return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permNotes);
}

GBool XRef::okToFillForm(GBool ignoreOwnerPW) {
  return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permFillForm);
}

GBool XRef::okToAccessibility(GBool ignoreOwnerPW) {
  return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permAccessibility);
}

GBool XRef::okToAssemble(GBool ignoreOwnerPW) {
  return (!ignoreOwnerPW && ownerPasswordOk) || (permFlags & permAssemble);
}

Object *XRef::fetch(int num, int gen, Object *obj) {
  XRefEntry *e;
  Parser *parser;
  Object obj1, obj2, obj3;

  // check for bogus ref - this can happen in corrupted PDF files
  if (num < 0 || num >= size) {
    goto err;
  }

  e = &entries[num];
  switch (e->type) {

  case xrefEntryUncompressed:
    if (e->gen != gen) {
      goto err;
    }
    obj1.initNull();
    parser = new Parser(this,
               new Lexer(this,
                 str->makeSubStream(start + e->offset, gFalse, 0, &obj1)));
    parser->getObj(&obj1);
    parser->getObj(&obj2);
    parser->getObj(&obj3);
    if (!obj1.isInt() || obj1.getInt() != num ||
        !obj2.isInt() || obj2.getInt() != gen ||
        !obj3.isCmd("obj")) {
      obj1.free();
      obj2.free();
      obj3.free();
      delete parser;
      goto err;
    }
    parser->getObj(obj, encrypted ? fileKey : (Guchar *)NULL, keyLength,
                   num, gen);
    obj1.free();
    obj2.free();
    obj3.free();
    delete parser;
    break;

  case xrefEntryCompressed:
    if (gen != 0) {
      goto err;
    }
    if (!objStr || objStr->getObjStrNum() != (int)e->offset) {
      if (objStr) {
        delete objStr;
      }
      objStr = new ObjectStream(this, e->offset);
    }
    objStr->getObject(e->gen, num, obj);
    break;

  default:
    goto err;
  }

  return obj;

 err:
  return obj->initNull();
}

Object *XRef::getDocInfo(Object *obj) {
  return trailerDict.dictLookup("Info", obj);
}

// Added for the pdftex project.
Object *XRef::getDocInfoNF(Object *obj) {
  return trailerDict.dictLookupNF("Info", obj);
}

GBool XRef::getStreamEnd(Guint streamStart, Guint *streamEnd) {
  int a, b, m;

  if (streamEndsLen == 0 ||
      streamStart > streamEnds[streamEndsLen - 1]) {
    return gFalse;
  }

  a = -1;
  b = streamEndsLen - 1;
  // invariant: streamEnds[a] < streamStart <= streamEnds[b]
  while (b - a > 1) {
    m = (a + b) / 2;
    if (streamStart <= streamEnds[m]) {
      b = m;
    } else {
      a = m;
    }
  }
  *streamEnd = streamEnds[b];
  return gTrue;
}

int XRef::getNumEntry(int offset) const
{
  if (size > 0)
  {
    int res = 0;
    Guint resOffset = entries[0].offset;
    XRefEntry e;
    for (int i = 1; i < size; ++i)
    {
      e = entries[i];
      if (e.offset < (Guint)offset && e.offset >= resOffset)
      {
        res = i;
        resOffset = e.offset;
      }
    }
    return res;
  }
  else return -1;
}

Guint XRef::strToUnsigned(char *s) {
  Guint x;
  char *p;
  int i;

  x = 0;
  for (p = s, i = 0; *p && isdigit(*p) && i < 10; ++p, ++i) {
    x = 10 * x + (*p - '0');
  }
  return x;
}
