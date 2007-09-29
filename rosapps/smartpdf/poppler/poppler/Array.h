//========================================================================
//
// Array.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef ARRAY_H
#define ARRAY_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "Object.h"

class XRef;

//------------------------------------------------------------------------
// Array
//------------------------------------------------------------------------

class Array {
public:

  // Constructor.
  Array(XRef *xrefA);

  // Destructor.
  ~Array();

  // Reference counting.
  int incRef() { return ++ref; }
  int decRef() { return --ref; }

  // Get number of elements.
  int getLength() { return length; }

  // Add an element.
  void add(Object *elem);

  // Accessors.
  Object *get(int i, Object *obj);
  Object *getNF(int i, Object *obj);
  GBool getString(int i, GooString *string);

private:

  XRef *xref;			// the xref table for this PDF file
  Object *elems;		// array of elements
  int size;			// size of <elems> array
  int length;			// number of elements in array
  int ref;			// reference count
};

#endif
