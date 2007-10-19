//========================================================================
//
// UGooString.h
//
// Unicode string
//
// Copyright 2005 Albert Astals Cid <aacid@kde.org>
//
//========================================================================

#ifndef UGooString_H
#define UGooString_H

#include "CharTypes.h"

class GooString;

class UGooString
{
public:

  // Create empty unicode string
  UGooString(void);

  // Create a unicode string from <str>.
  UGooString(GooString &str);

  // Copy the unicode string
  UGooString(const UGooString &str);

  // Create a unicode string from <str>.
  UGooString(const char *str, int strLen = CALC_STRING_LEN);

  UGooString *Set(const char *str, int strLen = CALC_STRING_LEN);
  UGooString *Set(const UGooString &str);

  // Set the string to empty string, freeing all dynamically allocated memory
  // as a side effect
  UGooString *clear(void);

  ~UGooString();

  void resize(int newLength);

  int getLength() const { return length; }

  // Compare two strings:  -1:<  0:=  +1:>
  int cmp(const UGooString *str) const;
  int cmp(const UGooString &str) const;
  int cmp(const char *str, int strLen = CALC_STRING_LEN) const;

  // get the unicode
  Unicode *unicode() const { return s; }

  // Return a newly allocated copy of the string converted to
  // ascii (non-Unicode) format. Caller has to delete [] the result
  char *getCStringCopy() const;

  // a special value telling that the length of the string is not given
  // so it must be calculated from the strings
  static const int CALC_STRING_LEN = -1;

private:
  // you can tweak this number for a different speed/memory usage tradeoffs.
  // In libc malloc() rounding is 16 so it's best to choose a value that
  // results in sizeof(UGooString) be a multiple of 16.
  // 20 makes sizeof(UGooString) to be 48.
  static const int STR_STATIC_SIZE = 20;

  int  roundedSize(int len);
  void initChar(const char *str, int strLen);

  Unicode sStatic[STR_STATIC_SIZE];
  int length;
  Unicode *s;
};

#endif
