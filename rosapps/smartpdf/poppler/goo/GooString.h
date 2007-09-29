//========================================================================
//
// GooString.h
//
// Simple variable-length string type.
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef GSTRING_H
#define GSTRING_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <stdlib.h> // for NULL
#include "gtypes.h"
#include "GooMutex.h"

class GooString {
public:

  // Create an empty string.
  GooString();

  // Create a string from a C string.
  GooString(const char *sA);

  // Create a string from <lengthA> chars at <sA>.  This string
  // can contain null characters.
  GooString(const char *sA, int lengthA);

  // Create a string from <lengthA> chars at <idx> in <str>.
  GooString(GooString *str, int idx, int lengthA);

  // Set content of a string to concatination of <s1> and <s2>. They can both
  // be NULL. if <s1Len> or <s2Len> is CALC_STRING_LEN, then length of the string
  // will be calculated with strlen(). Otherwise we assume they are a valid
  // length of string (or its substring)

/* @note: gnu g++ fix */
  GooString* Set(const char *s1, int s1Len=CALC_STRING_LEN, const char *s2=NULL, int s2Len=CALC_STRING_LEN);

/* original code line:
 *  GooString* GooString::Set(const char *s1, int s1Len=CALC_STRING_LEN, const char *s2=NULL, int s2Len=CALC_STRING_LEN);
 */

  GooString(GooString *str);

  // Return a newly allocated copy of the string
  GooString *copy() { return new GooString(this); }

  // Concatenate two strings.
  GooString(GooString *str1, GooString *str2);

  // Convert an integer to a string.
  static GooString *fromInt(int x);

  ~GooString();

  // Get length.
  int getLength() const { return length; }

  // Get C string.
  char *getCString() const { return s; }

  // Get <i>th character.
  char getChar(int i) { return s[i]; }

  // Change <i>th character.
  void setChar(int i, char c) { s[i] = c; }

  // Clear string to zero length.
  GooString *clear();

  // Append a character or string.
  GooString *append(char c);
  GooString *append(GooString *str);
  GooString *append(const char *str, int lengthA=CALC_STRING_LEN);

  // Insert a character or string.
  GooString *insert(int i, char c);
  GooString *insert(int i, GooString *str);
  GooString *insert(int i, const char *str, int lengthA=CALC_STRING_LEN);

  // Delete a character or range of characters.
  GooString *del(int i, int n = 1);

  // Convert string to all-upper/all-lower case.
  GooString *upperCase();
  GooString *lowerCase();

  // Compare two strings:  -1:<  0:=  +1:>
  int cmp(GooString *str);
  int cmpN(GooString *str, int n);
  int cmp(const char *sA);
  int cmpN(const char *sA, int n);

  GBool hasUnicodeMarker(void);

  // a special value telling that the length of the string is not given
  // so it must be calculated from the strings
  static const int CALC_STRING_LEN = -1;

private:
  // you can tweak this number for a different speed/memory usage tradeoffs.
  // In libc malloc() rounding is 16 so it's best to choose a value that
  // results in sizeof(GooString) be a multiple of 16.
  // 24 makes sizeof(GooString) to be 32.
  static const int STR_STATIC_SIZE = 24;

  int  roundedSize(int len);

  char sStatic[STR_STATIC_SIZE];
  int length;
  char *s;

  void resize(int newLength);
};

//Uncomment if you want to gather stats on hit rate of the cache
//#define CALC_OBJECT_STRING_CACHE_STATS 1

/* A cache for GooString. You can think of it as a custom allocator 
   for GooString(). Use alloc() to get a new GooString() and free() to free
   existing GooString(). It keeps last GooStringCache::CACHE_SIZE free()ed
   strings in a cache (which is a stack) and satisfies the alloc()s from
   the cache first, thus saves free()/malloc() cycle.
   It's used by Object::free()/Object::init*() and works great for them
   because they recycle strings like crazy.
*/
class GooStringCache 
{
public:
  GooStringCache() {
    inCache = 0;
#ifdef CALC_OBJECT_STRING_CACHE_STATS
    totalAllocs = 0;
    allocsFromCache = 0;
#endif
#if MULTITHREADED
    gInitMutex(&mutex);
#endif
  }

  ~GooStringCache() {
    for (int i=0; i<inCache; i++) {
      delete stringsCached[i];
    }
#if MULTITHREADED
    gDestroyMutex(&mutex);
#endif
  }

  GooString *alloc(GooString *str) {
    return alloc(str->getCString(), str->getLength());
  }

  // alloc and free are called a lot, so make them inline
  GooString *alloc(const char *txt, int strLen = GooString::CALC_STRING_LEN) {
    GooString *res = NULL;
#if MULTITHREADED
    gLockMutex(&mutex);
#endif
#ifdef CALC_OBJECT_STRING_CACHE_STATS
    ++totalAllocs;
#endif
    if (inCache > 0) {
      // pop the value from the top of the stack
      res = stringsCached[inCache-1];
      res->Set(txt, strLen);
      --inCache;
#ifdef CALC_OBJECT_STRING_CACHE_STATS
      ++allocsFromCache;
#endif
      goto Exit;
    } else {
      res = new GooString(txt, strLen);
      goto Exit;
    }
Exit:
#if MULTITHREADED
    gUnlockMutex(&mutex);
#endif
    return res;
  }

  void free(GooString *str) {
#if MULTITHREADED
    gLockMutex(&mutex);
#endif
    if (inCache < CACHE_SIZE) {
      // put the value at the top of the stack
      stringsCached[inCache] = str;
      ++inCache;
    } else {
      // cache is full
      delete str;
    }
#if MULTITHREADED
    gUnlockMutex(&mutex);
#endif
  }
private:
  // CACHE_SIZE size affects 2 things:
  // - alloc() hit ratio i.e. how many alloc()s can be satisfied from cache
  //   as opposed to allocating new GooString() with generic malloc()
  //   This is a *very* effective cache. I get 99.98% alloc() hit ratio even
  //   with CACHE_SIZE of 8. 95% with CACHE_SIZE of 4
  // - how often we call delete on GooString(). When cache is full, we delete
  //   strings the usual way. When CACHE_SIZE grows, we hit delete less
  // 64 is chosen by gut feeling, might use some tweaking
  static const int CACHE_SIZE = 64;

#ifdef CALC_OBJECT_STRING_CACHE_STATS
  int totalAllocs;
  int allocsFromCache;
#endif
  int inCache;
  // you can think of it as a stack, we only add something to the top
  // or take it from the top
  GooString *stringsCached[CACHE_SIZE];
#if MULTITHREADED
  GooMutex mutex;
#endif
};

#endif
