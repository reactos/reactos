//========================================================================
//
// CMap.h
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef CMAP_H
#define CMAP_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "poppler-config.h"
#include "goo/gtypes.h"
#include "CharTypes.h"

#if MULTITHREADED
#include <goo/GooMutex.h>
#endif

class GooString;
struct CMapVectorEntry;
class CMapCache;

//------------------------------------------------------------------------

class CMap {
public:

  // Create the CMap specified by <collection> and <cMapName>.  Sets
  // the initial reference count to 1.  Returns NULL on failure.
  static CMap *parse(CMapCache *cache, GooString *collectionA,
		     GooString *cMapNameA);

  ~CMap();

  void incRefCnt();
  void decRefCnt();

  // Return collection name (<registry>-<ordering>).
  GooString *getCollection() { return collection; }

  // Return true if this CMap matches the specified <collectionA>, and
  // <cMapNameA>.
  GBool match(GooString *collectionA, GooString *cMapNameA);

  // Return the CID corresponding to the character code starting at
  // <s>, which contains <len> bytes.  Sets *<nUsed> to the number of
  // bytes used by the char code.
  CID getCID(char *s, int len, int *nUsed);

  // Return the writing mode (0=horizontal, 1=vertical).
  int getWMode() { return wMode; }

private:

  CMap(GooString *collectionA, GooString *cMapNameA);
  CMap(GooString *collectionA, GooString *cMapNameA, int wModeA);
  void useCMap(CMapCache *cache, char *useName);
  void copyVector(CMapVectorEntry *dest, CMapVectorEntry *src);
  void addCodeSpace(CMapVectorEntry *vec, Guint start, Guint end,
		    Guint nBytes);
  void addCIDs(Guint start, Guint end, Guint nBytes, CID firstCID);
  void freeCMapVector(CMapVectorEntry *vec);

  GooString *collection;
  GooString *cMapName;
  int wMode;			// writing mode (0=horizontal, 1=vertical)
  CMapVectorEntry *vector;	// vector for first byte (NULL for
				//   identity CMap)
  int refCnt;
#if MULTITHREADED
  GooMutex mutex;
#endif
};

//------------------------------------------------------------------------

#define cMapCacheSize 4

class CMapCache {
public:

  CMapCache();
  ~CMapCache();

  // Get the <cMapName> CMap for the specified character collection.
  // Increments its reference count; there will be one reference for
  // the cache plus one for the caller of this function.  Returns NULL
  // on failure.
  CMap *getCMap(GooString *collection, GooString *cMapName);

private:

  CMap *cache[cMapCacheSize];
};

#endif
