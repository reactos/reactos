//========================================================================
//
// Object.cc
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
#include "Error.h"
#include "Stream.h"
#include "UGooString.h"
#include "XRef.h"

GooStringCache g_objectStringCache;

//------------------------------------------------------------------------
// Object
//------------------------------------------------------------------------

char *objTypeNames[numObjTypes] = {
  "boolean",
  "integer",
  "real",
  "string",
  "name",
  "null",
  "array",
  "dictionary",
  "stream",
  "ref",
  "cmd",
  "error",
  "eof",
  "none"
};

#ifdef DEBUG_MEM
int Object::numAlloc[numObjTypes] =
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif

Object *Object::initArray(XRef *xref) {
  initObj(objArray);
  array = new Array(xref);
  return this;
}

Object *Object::initDict(XRef *xref) {
  initObj(objDict);
  dict = new Dict(xref);
  return this;
}

Object *Object::initDict(Dict *dictA) {
  initObj(objDict);
  dict = dictA;
  dict->incRef();
  return this;
}

Object *Object::initStream(Stream *streamA) {
  initObj(objStream);
  stream = streamA;
  return this;
}

Object *Object::copy(Object *obj) {
  *obj = *this;
  switch (type) {
  case objString:
    obj->string = g_objectStringCache.alloc(string);
    break;
  case objName:
    obj->name = g_objectStringCache.alloc(name);
    break;
  case objArray:
    array->incRef();
    break;
  case objDict:
    dict->incRef();
    break;
  case objStream:
    stream->incRef();
    break;
  case objCmd:
    obj->cmd = g_objectStringCache.alloc(cmd);
    break;
  default:
    break;
  }
#ifdef DEBUG_MEM
  ++numAlloc[type];
#endif
  return obj;
}

Object *Object::fetch(XRef *xref, Object *obj) {
  return (type == objRef && xref) ?
         xref->fetch(ref.num, ref.gen, obj) : copy(obj);
}

void Object::free() {
  switch (type) {
  case objString:
    g_objectStringCache.free(string);
    break;
  case objName:
    g_objectStringCache.free(name);
    break;
  case objArray:
    if (0 == array->decRef()) {
      delete array;
    }
    break;
  case objDict:
    if (0 == dict->decRef()) {
      delete dict;
    }
    break;
  case objStream:
    if (0 == stream->decRef()) {
      delete stream;
    }
    break;
  case objCmd:
    g_objectStringCache.free(cmd);
    break;
  default:
    break;
  }
#ifdef DEBUG_MEM
  --numAlloc[type];
#endif
  type = objNone;
}

char *Object::getTypeName() {
  return objTypeNames[type];
}

void Object::print(FILE *f) {
  Object obj;
  int i;

  switch (type) {
  case objBool:
    fprintf(f, "%s", booln ? "true" : "false");
    break;
  case objInt:
    fprintf(f, "%d", intg);
    break;
  case objReal:
    fprintf(f, "%g", real);
    break;
  case objString:
    fprintf(f, "(");
    fwrite(string->getCString(), 1, string->getLength(), f);
    fprintf(f, ")");
    break;
  case objName:
    fprintf(f, "/%s", name);
    break;
  case objNull:
    fprintf(f, "null");
    break;
  case objArray:
    fprintf(f, "[");
    for (i = 0; i < arrayGetLength(); ++i) {
      if (i > 0)
        fprintf(f, " ");
      arrayGetNF(i, &obj);
      obj.print(f);
      obj.free();
    }
    fprintf(f, "]");
    break;
  case objDict:
    fprintf(f, "<<");
    for (i = 0; i < dictGetLength(); ++i) {
      char *key = dictGetKey(i)->getCStringCopy();
      fprintf(f, " /%s ", key);
      delete[] key;
      dictGetValNF(i, &obj);
      obj.print(f);
      obj.free();
    }
    fprintf(f, " >>");
    break;
  case objStream:
    fprintf(f, "<stream>");
    break;
  case objRef:
    fprintf(f, "%d %d R", ref.num, ref.gen);
    break;
  case objCmd:
    fprintf(f, "%s", cmd->getCString());
    break;
  case objError:
    fprintf(f, "<error>");
    break;
  case objEOF:
    fprintf(f, "<EOF>");
    break;
  case objNone:
    fprintf(f, "<none>");
    break;
  }
}

void Object::memCheck(FILE *f) {
#ifdef DEBUG_MEM
  int i;
  int t;

  t = 0;
  for (i = 0; i < numObjTypes; ++i)
    t += numAlloc[i];
  if (t > 0) {
    fprintf(f, "Allocated objects:\n");
    for (i = 0; i < numObjTypes; ++i) {
      if (numAlloc[i] > 0)
        fprintf(f, "  %-20s: %6d\n", objTypeNames[i], numAlloc[i]);
    }
  }
#endif
}
