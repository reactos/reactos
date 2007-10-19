//========================================================================
//
// UnicodeTypeTable.h
//
// Copyright 2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef UNICODETYPETABLE_H
#define UNICODETYPETABLE_H

#include "goo/gtypes.h"

extern GBool unicodeTypeL(Unicode c);

extern GBool unicodeTypeR(Unicode c);

extern Unicode unicodeToUpper(Unicode c);

extern Unicode *unicodeNormalizeNFKC(Unicode *in, int len,
				     int *out_len, int **offsets);

#endif
