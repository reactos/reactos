//========================================================================
//
// UGooString.cc
//
// Unicode string
//
// Copyright 2005 Albert Astals Cid <aacid@kde.org>
//
//========================================================================

#include <string.h>
#include <assert.h>

#include "goo/gmem.h"
#include "goo/GooString.h"
#include "PDFDocEncoding.h"
#include "UGooString.h"

int inline UGooString::roundedSize(int len) {
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
void inline UGooString::resize(int newLength) {
  int curSize = roundedSize(length);
  int newSize = roundedSize(newLength);

  assert(s);

  if (curSize != newSize) {
    Unicode *sNew = sStatic;
    if (newSize != STR_STATIC_SIZE)
      sNew = new Unicode[newSize];

    // we had to re-allocate the memory, so copy the content of previous
    // buffer into a new buffer
    if (newLength < length) {
      memcpy(sNew, s, newLength * sizeof(Unicode));
    } else {
      memcpy(sNew, s, length * sizeof(Unicode));
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

UGooString::UGooString(void)
{
  s = sStatic;
  length = 0;
  resize(0);
}

UGooString::UGooString(GooString &str)
{
  s = sStatic;
  length = 0;
  if (str.hasUnicodeMarker())
  {
    resize((str.getLength() - 2) / 2);
    for (int j = 0; j < length; ++j) {
      s[j] = ((str.getChar(2 + 2*j) & 0xff) << 8) | (str.getChar(3 + 2*j) & 0xff);
    }
  } else
    Set(str.getCString(), str.getLength());
}

UGooString::UGooString(const UGooString &str)
{
  s = sStatic;
  length = 0;
  Set(str);
}

UGooString::UGooString(const char *str, int strLen)
{
  s = sStatic;
  length = 0;
  if (CALC_STRING_LEN == strLen)
    strLen = strlen(str);
  Set(str, strLen);
}

UGooString *UGooString::Set(const UGooString &str)
{
  resize(str.length);
  memcpy(s, str.s, length * sizeof(Unicode));
  return this;
}

UGooString* UGooString::Set(const char *str, int strLen)
{
  int  j;
  bool foundUnencoded = false;

  if (CALC_STRING_LEN == strLen)
    strLen = strlen(str);

  resize(strLen);
  for (j = 0; j < length; ++j) {
    s[j] = pdfDocEncoding[str[j] & 0xff];
    if (!s[j]) {
        foundUnencoded = true;
        break;
    }
  }
  if (foundUnencoded)
  {
    for (j = 0; j < length; ++j) {
      s[j] = str[j];
    }
  }
  return this;
}

UGooString *UGooString::clear(void)
{
    resize(0);
    return this;
}

UGooString::~UGooString()
{
  if (s != sStatic)
    delete[] s;
}

int UGooString::cmp(const UGooString &str) const
{
    return cmp(&str);
}

int UGooString::cmp(const UGooString *str) const
{
  int n1, n2, i, x;
  Unicode *p1, *p2;

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

int UGooString::cmp(const char *str, int strLen) const
{
  int n1, n2, i, x;
  Unicode u;
  Unicode *p1, *p2;

  assert(str);
  if (strLen == CALC_STRING_LEN) {
    n2 = 0;
    if (str)
      n2 = strlen(str);
  } else {
    n2 = strLen;
  }

  p2 = &u;
  n1 = length;
  for (i = 0, p1 = s; i < n1 && i < n2; ++i, ++p1) {
    u = pdfDocEncoding[str[i] & 0xff];
    if (!u)
      u = (unsigned char)str[i];
    x = *p1 - *p2;
    if (x != 0) {
      return x;
    }
  }
  return n1 - n2;
}

char *UGooString::getCStringCopy() const
{
  char *res = new char[length + 1];
  for (int i = 0; i < length; i++) res[i] = s[i];
  res[length] = '\0';
  return res;
}

