//========================================================================
//
// GooString.cc
//
// Simple variable-length string type.
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
#include <assert.h>
#include "gtypes.h"
#include "GooString.h"

int inline GooString::roundedSize(int len) {
  int delta;
  if (len <= STR_STATIC_SIZE-1)
      return STR_STATIC_SIZE;
  delta = len < 256 ? 7 : 255;
  return ((len + 1) + delta) & ~delta;
}

// Make sure that the buffer is big enough to contain <newLength> characters
// plus terminating 0.
// We assume that if this is being called from the constructor, <s> was set
// to NULL and <length> was set to 0 to indicate unused string before calling us.
void inline GooString::resize(int newLength) {
  int curSize = roundedSize(length);
  int newSize = roundedSize(newLength);

  assert(s);

  if (curSize != newSize) {
    char *sNew = sStatic;
    if (newSize != STR_STATIC_SIZE)
      sNew = new char[newSize];

    // we had to re-allocate the memory, so copy the content of previous
    // buffer into a new buffer
    if (newLength < length) {
      memcpy(sNew, s, newLength);
    } else {
      memcpy(sNew, s, length);
    }
    if (s != sStatic) {
      assert(curSize != STR_STATIC_SIZE);
      delete[] s;
    }
    s = sNew;
  }

  length = newLength;
  s[length] = '\0';
}

GooString* GooString::Set(const char *s1, int s1Len, const char *s2, int s2Len)
{
    int newLen = 0;
    char *p;

    if (s1) {
        if (CALC_STRING_LEN == s1Len) {
            s1Len = strlen(s1);
        } else
            assert(s1Len >= 0);
        newLen += s1Len;
    }

    if (s2) {
        if (CALC_STRING_LEN == s2Len) {
            s2Len = strlen(s2);
        } else
            assert(s2Len >= 0);
        newLen += s2Len;
    }

    resize(newLen);
    p = s;
    if (s1) {
        memcpy(p, s1, s1Len);
        p += s1Len;
    }
    if (s2) {
        memcpy(p, s2, s2Len);
        p += s2Len;
    }
    return this;
}

GooString::GooString() {
  s = sStatic;
  length = 0;
  Set(NULL);
}

GooString::GooString(const char *sA) {
  s = sStatic;
  length = 0;
  Set(sA, CALC_STRING_LEN);
}

GooString::GooString(const char *sA, int lengthA) {
  s = sStatic;
  length = 0;
  Set(sA, lengthA);
}

GooString::GooString(GooString *str, int idx, int lengthA) {
  s = sStatic;
  length = 0;
  assert(idx + lengthA < str->length);
  Set(str->getCString() + idx, lengthA);
}

GooString::GooString(GooString *str) {
  s = sStatic;
  length = 0;
  Set(str->getCString(), str->length);
}

GooString::GooString(GooString *str1, GooString *str2) {
  s = sStatic;
  length = 0;
  Set(str1->getCString(), str1->length, str2->getCString(), str2->length);
}

GooString *GooString::fromInt(int x) {
  char buf[24]; // enough space for 64-bit ints plus a little extra
  GBool neg;
  Guint y;
  int i;

  i = 24;
  if (x == 0) {
    buf[--i] = '0';
  } else {
    if ((neg = x < 0)) {
      y = (Guint)-x;
    } else {
      y = (Guint)x;
    }
    while (i > 0 && y > 0) {
      buf[--i] = '0' + y % 10;
      y /= 10;
    }
    if (neg && i > 0) {
      buf[--i] = '-';
    }
  }
  return new GooString(buf + i, 24 - i);
}

GooString::~GooString() {
  if (s != sStatic)
    delete[] s;
}

GooString *GooString::clear() {
  resize(0);
  return this;
}

GooString *GooString::append(char c) {
  return append((const char*)&c, 1);
}

GooString *GooString::append(GooString *str) {
  return append(str->getCString(), str->getLength());
}

GooString *GooString::append(const char *str, int lengthA) {
  int prevLen = length;
  if (CALC_STRING_LEN == lengthA)
    lengthA = strlen(str);
  resize(length + lengthA);
  memcpy(s + prevLen, str, lengthA);
  return this;
}

GooString *GooString::insert(int i, char c) {
  return insert(i, (const char*)&c, 1);
}

GooString *GooString::insert(int i, GooString *str) {
  return insert(i, str->getCString(), str->getLength());
}

GooString *GooString::insert(int i, const char *str, int lengthA) {
  int j;
  int prevLen = length;
  if (CALC_STRING_LEN == lengthA)
    lengthA = strlen(str);

  resize(length + lengthA);
  for (j = prevLen; j >= i; --j)
    s[j+lengthA] = s[j];
  memcpy(s+i, str, lengthA);
  return this;
}

GooString *GooString::del(int i, int n) {
  int j;

  if (n > 0) {
    if (i + n > length) {
      n = length - i;
    }
    for (j = i; j <= length - n; ++j) {
      s[j] = s[j + n];
    }
    resize(length - n);
  }
  return this;
}

GooString *GooString::upperCase() {
  int i;

  for (i = 0; i < length; ++i) {
    if (islower(s[i]))
      s[i] = toupper(s[i]);
  }
  return this;
}

GooString *GooString::lowerCase() {
  int i;

  for (i = 0; i < length; ++i) {
    if (isupper(s[i]))
      s[i] = tolower(s[i]);
  }
  return this;
}

int GooString::cmp(GooString *str) {
  int n1, n2, i, x;
  char *p1, *p2;

  n1 = length;
  n2 = str->length;
  for (i = 0, p1 = s, p2 = str->s; i < n1 && i < n2; ++i, ++p1, ++p2) {
    x = *p1 - *p2;
    if (x != 0) {
      return x;
    }
  }
  return n1 - n2;
}

int GooString::cmpN(GooString *str, int n) {
  int n1, n2, i, x;
  char *p1, *p2;

  n1 = length;
  n2 = str->length;
  for (i = 0, p1 = s, p2 = str->s;
       i < n1 && i < n2 && i < n;
       ++i, ++p1, ++p2) {
    x = *p1 - *p2;
    if (x != 0) {
      return x;
    }
  }
  if (i == n) {
    return 0;
  }
  return n1 - n2;
}

int GooString::cmp(const char *sA) {
  int n1, i, x;
  const char *p1, *p2;

  n1 = length;
  for (i = 0, p1 = s, p2 = sA; i < n1 && *p2; ++i, ++p1, ++p2) {
    x = *p1 - *p2;
    if (x != 0) {
      return x;
    }
  }
  if (i < n1) {
    return 1;
  }
  if (*p2) {
    return -1;
  }
  return 0;
}

int GooString::cmpN(const char *sA, int n) {
  int n1, i, x;
  const char *p1, *p2;

  n1 = length;
  for (i = 0, p1 = s, p2 = sA; i < n1 && *p2 && i < n; ++i, ++p1, ++p2) {
    x = *p1 - *p2;
    if (x != 0) {
      return x;
    }
  }
  if (i == n) {
    return 0;
  }
  if (i < n1) {
    return 1;
  }
  if (*p2) {
    return -1;
  }
  return 0;
}

GBool GooString::hasUnicodeMarker(void)
{
    return (s[0] & 0xff) == 0xfe && (s[1] & 0xff) == 0xff;
}
