//========================================================================
//
// Dict.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "goo/gmem.h"
#include "Error.h"
#include "Object.h"
#include "UGooString.h"
#include "XRef.h"
#include "Dict.h"

//------------------------------------------------------------------------
// Dict
//------------------------------------------------------------------------

Dict::Dict(XRef *xrefA) {
  xref = xrefA;
  entries = NULL;
  size = length = 0;
  ref = 1;
}

Dict::~Dict() {
  int i;

  for (i = 0; i < length; ++i) {
    delete entries[i].key;
    entries[i].val.free();
  }
  gfree(entries);
}

void Dict::addOwnKeyVal(UGooString *key, Object *val) {
  if (length == size) {
    if (length == 0) {
      size = 8;
    } else {
      size *= 2;
    }
    entries = (DictEntry *)greallocn(entries, size, sizeof(DictEntry));
  }
  entries[length].key = key;
  entries[length].val = *val;
  ++length;
}

inline DictEntry *Dict::find(const UGooString &key) const {
  int i;

  for (i = 0; i < length; ++i) {
    if (!key.cmp(entries[i].key))
      return &entries[i];
  }
  return NULL;
}

inline DictEntry *Dict::find(const char *key, int keyLen) const {
  int i;

  for (i = 0; i < length; ++i) {
    if (0 == entries[i].key->cmp(key, keyLen))
      return &entries[i];
  }
  return NULL;
}

// length of string "Type"
#define TYPE_STR_LEN 4

GBool Dict::is(char *type) const {
  DictEntry *e = find("Type", TYPE_STR_LEN);
  if (!e)
    return gFalse;
  return e->val.isName(type);
}

Object *Dict::lookup(const char *key, Object *obj, int keyLen) const {
  DictEntry *e = find(key, keyLen);
  if (!e)
    return obj->initNull();
  return e->val.fetch(xref, obj);
}

Object *Dict::lookupNF(const char *key, Object *obj, int keyLen) const {
  DictEntry *e = find(key, keyLen);
  if (!e)
    return obj->initNull();
  return e->val.copy(obj);
}

Object *Dict::lookup(const UGooString &key, Object *obj) const {
  DictEntry *e = find(key);
  if (!e)
    return obj->initNull();
  return e->val.fetch(xref, obj);
}

Object *Dict::lookupNF(const UGooString &key, Object *obj) const {
  DictEntry *e = find(key);
  if (!e)
    return obj->initNull();
  return e->val.copy(obj);
}

Object *Dict::lookupRefNoFetch(const char *key, int keyLen) const {
  DictEntry *e;
  e = find(key);
  if (!e)
    return NULL;
  return &(e->val);
}

Object *Dict::lookupRefNoFetch(const UGooString &key) const {
  DictEntry *e;
  e = find(key);
  if (!e)
    return NULL;
  return &(e->val);

}

GBool Dict::lookupBool(const char *key, const char *alt_key, GBool *value) const {
  Object *obj;

  // FIXME: handle ref objects as well
  obj = lookupRefNoFetch(key);
  if (obj && obj->isRef()) {
    error(-1, "ref object in Dict::lookupBool for %s", key);
    assert(0);
  }
  if (obj && obj->isBool()) {
    *value = obj->getBool();
    return gTrue;
  }

  if (!alt_key)
    return gFalse;
  obj = lookupRefNoFetch(alt_key);
  if (obj && obj->isRef()) {
    error(-1, "ref object in Dict::lookupBool for %s", alt_key);
    assert(0);
  }
  if (obj && obj->isBool()) {
    *value = obj->getBool();
    return gTrue;
  }

  return gFalse;
}

GBool Dict::lookupInt(const char *key, const char *alt_key, int *value) const {
  Object *obj;

  // FIXME: handle ref objects as well
  obj = lookupRefNoFetch(key);
  if (obj && obj->isRef()) {
    error(-1, "ref object in Dict::lookupInt for %s", key);
    assert(0);
  }
  if (obj && obj->isInt()) {
    *value = obj->getInt();
    return gTrue;
  }

  if (!alt_key)
    return gFalse;
  obj = lookupRefNoFetch(alt_key);
  if (obj && obj->isRef()) {
    error(-1, "ref object in Dict::lookupInt for %s", alt_key);
    assert(0);
  }
  if (obj && obj->isInt()) {
    *value = obj->getInt();
    return gTrue;
  }

  return gFalse;
}

UGooString *Dict::getKey(int i) const {
  return entries[i].key;
}

Object *Dict::getVal(int i, Object *obj) const {
  return entries[i].val.fetch(xref, obj);
}

Object *Dict::getValNF(int i, Object *obj) const {
  return entries[i].val.copy(obj);
}
