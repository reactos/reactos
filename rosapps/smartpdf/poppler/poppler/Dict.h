//========================================================================
//
// Dict.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef DICT_H
#define DICT_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "Object.h"
#include "UGooString.h"

//------------------------------------------------------------------------
// Dict
//------------------------------------------------------------------------

struct DictEntry {
  UGooString *key;
  Object val;
};

class Dict {
public:

  Dict(XRef *xrefA);

  ~Dict();

  // Reference counting.
  int incRef() { return ++ref; }
  int decRef() { return --ref; }

  // Get number of entries.
  int getLength() const { return length; }

  // Add an entry
  void addOwnKeyVal(UGooString *key, Object *val);
  // FIXME: should also be renamed to addOwnVal()
  void add(const UGooString &key, Object *val) {
    addOwnKeyVal(new UGooString(key), val);
  }
  void addOwnVal(const char *key, Object *val) {
    addOwnKeyVal(new UGooString(key), val);
  }

  // Check if dictionary is of specified type.
  GBool is(char *type) const;

  // Look up an entry and return the value.  Returns a null object
  // if <key> is not in the dictionary.
  Object *lookup(const UGooString &key, Object *obj) const;
  Object *lookupNF(const UGooString &key, Object *obj) const;
  Object *lookupRefNoFetch(const UGooString &key) const;
  Object *lookup(const char *key, Object *obj, int keyLen=UGooString::CALC_STRING_LEN) const;
  Object *lookupNF(const char *key, Object *obj, int keyLen=UGooString::CALC_STRING_LEN) const;
  Object *lookupRefNoFetch(const char *key, int keyLen=UGooString::CALC_STRING_LEN) const;

  GBool lookupBool(const char *key, const char *alt_key, GBool *value) const;
  GBool lookupInt(const char *key, const char *alt_key, int *value) const;

  // Iterative accessors.
  UGooString *getKey(int i) const;
  Object *getVal(int i, Object *obj) const;
  Object *getValNF(int i, Object *obj) const;

  // Set the xref pointer.  This is only used in one special case: the
  // trailer dictionary, which is read before the xref table is
  // parsed.
  void setXRef(XRef *xrefA) { xref = xrefA; }

private:

  XRef *xref;			// the xref table for this PDF file
  DictEntry *entries;		// array of entries
  int size;			// size of <entries> array
  int length;			// number of entries in dictionary
  int ref;			// reference count

  DictEntry *find(const UGooString &key) const;
  DictEntry *find(const char *key, int keyLen) const;
};

#endif
