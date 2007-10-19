//========================================================================
//
// Object.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef OBJECT_H
#define OBJECT_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <stdio.h>
#include <string.h>
#include "goo/gtypes.h"
#include "goo/gmem.h"
#include "goo/GooString.h"
#include "UGooString.h"

class XRef;
class Array;
class Dict;
class Stream;

extern GooStringCache g_objectStringCache;

//------------------------------------------------------------------------
// Ref
//------------------------------------------------------------------------

struct Ref {
  int num;			// object number
  int gen;			// generation number
};

//------------------------------------------------------------------------
// object types
//------------------------------------------------------------------------

enum ObjType {
  // simple objects
  objBool,			// boolean
  objInt,			// integer
  objReal,			// real
  objString,			// string
  objName,			// name
  objNull,			// null

  // complex objects
  objArray,			// array
  objDict,			// dictionary
  objStream,			// stream
  objRef,			// indirect reference

  // special objects
  objCmd,			// command name
  objError,			// error return from Lexer
  objEOF,			// end of file return from Lexer
  objNone			// uninitialized object
};

#define numObjTypes 14		// total number of object types

//------------------------------------------------------------------------
// Object
//------------------------------------------------------------------------

#ifdef DEBUG_MEM
#define initObj(t) ++numAlloc[type = t]
#else
#define initObj(t) type = t
#endif

class Object {
public:

  // Default constructor.
  Object():
    type(objNone) {}

  // Initialize an object.
  Object *initBool(GBool boolnA)
    { initObj(objBool); booln = boolnA; return this; }
  Object *initInt(int intgA)
    { initObj(objInt); intg = intgA; return this; }
  Object *initReal(double realA)
    { initObj(objReal); real = realA; return this; }
  Object *initString(GooString *stringA) {
        initObj(objString);
        string = stringA;
        return this; }
  Object *initName(char *nameA) {
    initObj(objName);
    name = g_objectStringCache.alloc(nameA);
    return this;
  }

  Object *initNull()
    { initObj(objNull); return this; }
  Object *initArray(XRef *xref);
  Object *initDict(XRef *xref);
  Object *initDict(Dict *dictA);
  Object *initStream(Stream *streamA);
  Object *initRef(int numA, int genA)
    { initObj(objRef); ref.num = numA; ref.gen = genA; return this; }
  Object *initCmd(char *cmdA) {
    initObj(objCmd);
    cmd = g_objectStringCache.alloc(cmdA);
    return this;
  }
  Object *initError()
    { initObj(objError); return this; }
  Object *initEOF()
    { initObj(objEOF); return this; }

  // Copy an object.
  Object *copy(Object *obj);
  Object *shallowCopy(Object *obj) {
    *obj = *this;
    return obj;
  }

  // If object is a Ref, fetch and return the referenced object.
  // Otherwise, return a copy of the object.
  Object *fetch(XRef *xref, Object *obj);

  // Free object contents.
  void free();

  // Type checking.
  ObjType getType() { return type; }
  GBool isBool() { return type == objBool; }
  GBool isInt() { return type == objInt; }
  GBool isReal() { return type == objReal; }
  GBool isNum() { return type == objInt || type == objReal; }
  GBool isString() { return type == objString; }
  GBool isName() { return type == objName; }
  GBool isNull() { return type == objNull; }
  GBool isArray() { return type == objArray; }
  GBool isDict() { return type == objDict; }
  GBool isStream() { return type == objStream; }
  GBool isRef() { return type == objRef; }
  GBool isCmd() { return type == objCmd; }
  GBool isError() { return type == objError; }
  GBool isEOF() { return type == objEOF; }
  GBool isNone() { return type == objNone; }

  // Special type checking.
  GBool isName(char *nameA)
    { return type == objName && (0 == name->cmp(nameA)); }
  GBool isDict(char *dictType);
  GBool isStream(char *dictType);
  GBool isCmd(char *cmdA)
    { return type == objCmd && (0 == cmd->cmp(cmdA)); }

  // Accessors.  NB: these assume object is of correct type.
  GBool getBool() { return booln; }
  int getInt() { return intg; }
  double getReal() { return real; }
  double getNum() { return type == objInt ? (double)intg : real; }
  GooString *getString() { return string; }
  GooString *getName() { return name; }
  char *getNameC() { return name->getCString(); }
  Array *getArray() { return array; }
  Dict *getDict() { return dict; }
  Stream *getStream() { return stream; }
  Ref getRef() { return ref; }
  int getRefNum() { return ref.num; }
  int getRefGen() { return ref.gen; }
  GooString *getCmd() { return cmd; }

  // Array accessors.
  int arrayGetLength();
  void arrayAdd(Object *elem);
  Object *arrayGet(int i, Object *obj);
  Object *arrayGetNF(int i, Object *obj);

  // Dict accessors.
  int dictGetLength();
  void dictAddOwnKeyVal(UGooString *key, Object *val);
  void dictAdd(const UGooString &key, Object *val);
  void dictAddOwnVal(const char *key, Object *val);
  GBool dictIs(char *dictType);
  Object *dictLookup(const UGooString &key, Object *obj);
  Object *dictLookupNF(const UGooString &key, Object *obj);
  Object *dictLookupRefNoFetch(const UGooString &key);
  Object *dictLookup(const char *key, Object *obj, int keyLen=UGooString::CALC_STRING_LEN);
  Object *dictLookupNF(const char *key, Object *obj, int keyLen=UGooString::CALC_STRING_LEN);
  Object *dictLookupRefNoFetch(const char *key, int keyLen=UGooString::CALC_STRING_LEN);
  GBool dictLookupBool(const char *key, const char *alt_key, GBool *value);
  GBool dictLookupInt(const char *key, const char *alt_key, int *value);

  UGooString *dictGetKey(int i);
  Object *dictGetVal(int i, Object *obj);
  Object *dictGetValNF(int i, Object *obj);

  // Stream accessors.
  GBool streamIs(char *dictType);
  void streamReset();
  void streamClose();
  int streamGetChar();
  int streamLookChar();
  char *streamGetLine(char *buf, int size);
  Guint streamGetPos();
  void streamSetPos(Guint pos, int dir = 0);
  Dict *streamGetDict();

  // Output.
  char *getTypeName();
  void print(FILE *f = stdout);

  // Memory testing.
  static void memCheck(FILE *f);

private:

  ObjType type;             // object type
  union {                   // value for each type:
    GBool booln;            //   boolean
    int intg;               //   integer
    double real;            //   real
    GooString *string;      //   string
    GooString *name;        //   name
    Array *array;           //   array
    Dict *dict;             //   dictionary
    Stream *stream;         //   stream
    Ref ref;                //   indirect reference
    GooString *cmd;         //   command
  };

#ifdef DEBUG_MEM
  static int                // number of each type of object
    numAlloc[numObjTypes];  //   currently allocated
#endif
};

//------------------------------------------------------------------------
// Array accessors.
//------------------------------------------------------------------------

#include "Array.h"

inline int Object::arrayGetLength()
  { return array->getLength(); }

inline void Object::arrayAdd(Object *elem)
  { array->add(elem); }

inline Object *Object::arrayGet(int i, Object *obj)
  { return array->get(i, obj); }

inline Object *Object::arrayGetNF(int i, Object *obj)
  { return array->getNF(i, obj); }

//------------------------------------------------------------------------
// Dict accessors.
//------------------------------------------------------------------------

#include "Dict.h"

inline int Object::dictGetLength()
  { return dict->getLength(); }

inline void Object::dictAdd(const UGooString &key, Object *val)
  { dict->add(key, val); }

inline void Object::dictAddOwnVal(const char *key, Object *val)
  { dict->addOwnVal(key, val); }

inline void Object::dictAddOwnKeyVal(UGooString *key, Object *val)
  { dict->addOwnKeyVal(key, val); }

inline GBool Object::dictIs(char *dictType)
  { return dict->is(dictType); }

inline GBool Object::isDict(char *dictType)
  { return type == objDict && dictIs(dictType); }

inline Object *Object::dictLookup(const UGooString &key, Object *obj)
  { return dict->lookup(key, obj); }

inline Object *Object::dictLookup(const char *key, Object *obj, int keyLen)
  { return dict->lookup(key, obj, keyLen); }

inline Object *Object::dictLookupNF(const UGooString &key, Object *obj)
  { return dict->lookupNF(key, obj); }

inline Object *Object::dictLookupNF(const char *key, Object *obj, int keyLen)
  { return dict->lookupNF(key, obj, keyLen); }

inline Object *Object::dictLookupRefNoFetch(const UGooString &key)
  { return dict->lookupRefNoFetch(key); }

inline Object *Object::dictLookupRefNoFetch(const char *key, int keyLen)
  { return dict->lookupRefNoFetch(key, keyLen); }

inline GBool Object::dictLookupBool(const char *key, const char *alt_key, GBool *value)
  { return dict->lookupBool(key, alt_key, value); }

inline GBool Object::dictLookupInt(const char *key, const char *alt_key, int *value)
  { return dict->lookupInt(key, alt_key, value); }

inline UGooString *Object::dictGetKey(int i)
  { return dict->getKey(i); }

inline Object *Object::dictGetVal(int i, Object *obj)
  { return dict->getVal(i, obj); }

inline Object *Object::dictGetValNF(int i, Object *obj)
  { return dict->getValNF(i, obj); }

//------------------------------------------------------------------------
// Stream accessors.
//------------------------------------------------------------------------

#include "Stream.h"

inline GBool Object::streamIs(char *dictType)
  { return stream->getDict()->is(dictType); }

inline GBool Object::isStream(char *dictType)
  { return type == objStream && streamIs(dictType); }

inline void Object::streamReset()
  { stream->reset(); }

inline void Object::streamClose()
  { stream->close(); }

inline int Object::streamGetChar(){
    return stream->getChar();
}

inline int Object::streamLookChar()
  { return stream->lookChar(); }

inline char *Object::streamGetLine(char *buf, int size)
  { return stream->getLine(buf, size); }

inline Guint Object::streamGetPos()
  { return stream->getPos(); }

inline void Object::streamSetPos(Guint pos, int dir)
  { stream->setPos(pos, dir); }

inline Dict *Object::streamGetDict()
  { return stream->getDict(); }

#endif
