//========================================================================
//
// GfxFont.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "goo/gmem.h"
#include "Error.h"
#include "Object.h"
#include "Dict.h"
#include "GlobalParams.h"
#include "CMap.h"
#include "CharCodeToUnicode.h"
#include "FontEncodingTables.h"
#include "BuiltinFontTables.h"
#include <fofi/FoFiType1.h>
#include <fofi/FoFiType1C.h>
#include <fofi/FoFiTrueType.h>
#include "UGooString.h"
#include "GfxFont.h"

//------------------------------------------------------------------------

struct StdFontMapEntry {
  char *altName;
  char *properName;
};

// Acrobat 4.0 and earlier substituted Base14-compatible fonts without
// providing Widths and a FontDescriptor, so we munge the names into
// the proper Base14 names.  This table is from implementation note 44
// in the PDF 1.4 spec, with some additions based on empirical
// evidence.
static StdFontMapEntry stdFontMap[] = {
  { "Arial",                        "Helvetica" },
  { "Arial,Bold",                   "Helvetica-Bold" },
  { "Arial,BoldItalic",             "Helvetica-BoldOblique" },
  { "Arial,Italic",                 "Helvetica-Oblique" },
  { "Arial-Bold",                   "Helvetica-Bold" },
  { "Arial-BoldItalic",             "Helvetica-BoldOblique" },
  { "Arial-BoldItalicMT",           "Helvetica-BoldOblique" },
  { "Arial-BoldMT",                 "Helvetica-Bold" },
  { "Arial-Italic",                 "Helvetica-Oblique" },
  { "Arial-ItalicMT",               "Helvetica-Oblique" },
  { "ArialMT",                      "Helvetica" },
  { "Courier,Bold",                 "Courier-Bold" },
  { "Courier,BoldItalic",           "Courier-BoldOblique" },
  { "Courier,Italic",               "Courier-Oblique" },
  { "CourierNew",                   "Courier" },
  { "CourierNew,Bold",              "Courier-Bold" },
  { "CourierNew,BoldItalic",        "Courier-BoldOblique" },
  { "CourierNew,Italic",            "Courier-Oblique" },
  { "CourierNew-Bold",              "Courier-Bold" },
  { "CourierNew-BoldItalic",        "Courier-BoldOblique" },
  { "CourierNew-Italic",            "Courier-Oblique" },
  { "CourierNewPS-BoldItalicMT",    "Courier-BoldOblique" },
  { "CourierNewPS-BoldMT",          "Courier-Bold" },
  { "CourierNewPS-ItalicMT",        "Courier-Oblique" },
  { "CourierNewPSMT",               "Courier" },
  { "Helvetica,Bold",               "Helvetica-Bold" },
  { "Helvetica,BoldItalic",         "Helvetica-BoldOblique" },
  { "Helvetica,Italic",             "Helvetica-Oblique" },
  { "Helvetica-BoldItalic",         "Helvetica-BoldOblique" },
  { "Helvetica-Italic",             "Helvetica-Oblique" },
  { "Symbol,Bold",                  "Symbol" },
  { "Symbol,BoldItalic",            "Symbol" },
  { "Symbol,Italic",                "Symbol" },
  { "TimesNewRoman",                "Times-Roman" },
  { "TimesNewRoman,Bold",           "Times-Bold" },
  { "TimesNewRoman,BoldItalic",     "Times-BoldItalic" },
  { "TimesNewRoman,Italic",         "Times-Italic" },
  { "TimesNewRoman-Bold",           "Times-Bold" },
  { "TimesNewRoman-BoldItalic",     "Times-BoldItalic" },
  { "TimesNewRoman-Italic",         "Times-Italic" },
  { "TimesNewRomanPS",              "Times-Roman" },
  { "TimesNewRomanPS-Bold",         "Times-Bold" },
  { "TimesNewRomanPS-BoldItalic",   "Times-BoldItalic" },
  { "TimesNewRomanPS-BoldItalicMT", "Times-BoldItalic" },
  { "TimesNewRomanPS-BoldMT",       "Times-Bold" },
  { "TimesNewRomanPS-Italic",       "Times-Italic" },
  { "TimesNewRomanPS-ItalicMT",     "Times-Italic" },
  { "TimesNewRomanPSMT",            "Times-Roman" },
  { "TimesNewRomanPSMT,Bold",       "Times-Bold" },
  { "TimesNewRomanPSMT,BoldItalic", "Times-BoldItalic" },
  { "TimesNewRomanPSMT,Italic",     "Times-Italic" }
};

//------------------------------------------------------------------------
// GfxFont
//------------------------------------------------------------------------

GfxFont *GfxFont::makeFont(XRef *xref, char *tagA, Ref idA, Dict *fontDict) {
  GooString *nameA;
  GfxFont *font;
  Object obj1;

  // get base font name
  nameA = NULL;
  fontDict->lookup("BaseFont", &obj1);
  if (obj1.isName()) {
    nameA = new GooString(obj1.getName());
  }
  obj1.free();

  // get font type
  font = NULL;
  fontDict->lookup("Subtype", &obj1);
  if (obj1.isName("Type1") || obj1.isName("MMType1")) {
    font = new Gfx8BitFont(xref, tagA, idA, nameA, fontType1, fontDict);
  } else if (obj1.isName("Type1C")) {
    font = new Gfx8BitFont(xref, tagA, idA, nameA, fontType1C, fontDict);
  } else if (obj1.isName("Type3")) {
    font = new Gfx8BitFont(xref, tagA, idA, nameA, fontType3, fontDict);
  } else if (obj1.isName("TrueType")) {
    font = new Gfx8BitFont(xref, tagA, idA, nameA, fontTrueType, fontDict);
  } else if (obj1.isName("Type0")) {
    font = new GfxCIDFont(xref, tagA, idA, nameA, fontDict);
  } else {
    error(-1, "Unknown font type: '%s'",
	  obj1.isName() ? obj1.getNameC() : "???");
    font = new Gfx8BitFont(xref, tagA, idA, nameA, fontUnknownType, fontDict);
  }
  obj1.free();

  return font;
}

GfxFont::GfxFont(char *tagA, Ref idA, GooString *nameA) {
  ok = gFalse;
  tag = new GooString(tagA);
  id = idA;
  name = nameA;
  origName = nameA;
  embFontName = NULL;
  extFontFile = NULL;
  family = NULL;
  stretch = StretchNotDefined;
  weight = WeightNotDefined;
  refCnt = 1;
}

GfxFont::~GfxFont() {
  delete tag;
  delete family;
  if (origName && origName != name) {
    delete origName;
  }
  if (name) {
    delete name;
  }
  if (embFontName) {
    delete embFontName;
  }
  if (extFontFile) {
    delete extFontFile;
  }
}

void GfxFont::incRefCnt() {
  refCnt++;
}

void GfxFont::decRefCnt() {
  if (--refCnt == 0)
    delete this;
}

void GfxFont::readFontDescriptor(XRef *xref, Dict *fontDict) {
  Object obj1, obj2, obj3, obj4;
  double t;
  int i;

  // assume Times-Roman by default (for substitution purposes)
  flags = fontSerif;

  embFontID.num = -1;
  embFontID.gen = -1;
  missingWidth = 0;

  if (fontDict->lookup("FontDescriptor", &obj1)->isDict()) {

    // get flags
    if (obj1.dictLookup("Flags", &obj2)->isInt()) {
      flags = obj2.getInt();
    }
    obj2.free();

    // get name
    obj1.dictLookup("FontName", &obj2);
    if (obj2.isName()) {
      embFontName = new GooString(obj2.getName());
    }
    obj2.free();

    // get family
    obj1.dictLookup("FontFamily", &obj2);
    if (obj2.isString()) family = new GooString(obj2.getString());
    obj2.free();

    // get stretch
    obj1.dictLookup("FontStretch", &obj2);
    if (obj2.isName()) {
      char *name = obj2.getNameC();
      if (strcmp(name, "UltraCondensed") == 0) stretch = UltraCondensed;
      else if (strcmp(name, "ExtraCondensed") == 0) stretch = ExtraCondensed;
      else if (strcmp(name, "Condensed") == 0) stretch = Condensed;
      else if (strcmp(name, "SemiCondensed") == 0) stretch = SemiCondensed;
      else if (strcmp(name, "Normal") == 0) stretch = Normal;
      else if (strcmp(name, "SemiExpanded") == 0) stretch = SemiExpanded;
      else if (strcmp(name, "Expanded") == 0) stretch = Expanded;
      else if (strcmp(name, "ExtraExpanded") == 0) stretch = ExtraExpanded;
      else if (strcmp(name, "UltraExpanded") == 0) stretch = UltraExpanded;
      else error(-1, "Invalid Font Stretch");
    }
    obj2.free();
    
    // get weight
    obj1.dictLookup("FontWeight", &obj2);
    if (obj2.isNum()) {
      if (obj2.getNum() == 100) weight = W100;
      else if (obj2.getNum() == 200) weight = W200;
      else if (obj2.getNum() == 300) weight = W300;
      else if (obj2.getNum() == 400) weight = W400;
      else if (obj2.getNum() == 500) weight = W500;
      else if (obj2.getNum() == 600) weight = W600;
      else if (obj2.getNum() == 700) weight = W700;
      else if (obj2.getNum() == 800) weight = W800;
      else if (obj2.getNum() == 900) weight = W900;
      else error(-1, "Invalid Font Weight");
    }
    obj2.free();

    // look for embedded font file
    if (obj1.dictLookupNF("FontFile", &obj2)->isRef()) {
      embFontID = obj2.getRef();
      if (type != fontType1) {
	error(-1, "Mismatch between font type and embedded font file");
	type = fontType1;
      }
    }
    obj2.free();
    if (embFontID.num == -1 &&
	obj1.dictLookupNF("FontFile2", &obj2)->isRef()) {
      embFontID = obj2.getRef();
      if (type != fontTrueType && type != fontCIDType2) {
	error(-1, "Mismatch between font type and embedded font file");
	type = type == fontCIDType0 ? fontCIDType2 : fontTrueType;
      }
    }
    obj2.free();
    if (embFontID.num == -1 &&
	obj1.dictLookupNF("FontFile3", &obj2)->isRef()) {
      if (obj2.fetch(xref, &obj3)->isStream()) {
	obj3.streamGetDict()->lookup("Subtype", &obj4);
	if (obj4.isName("Type1")) {
	    embFontID = obj2.getRef();
	  if (type != fontType1) {
	    error(-1, "Mismatch between font type and embedded font file");
	    type = fontType1;
	  }
	} else if (obj4.isName("Type1C")) {
	    embFontID = obj2.getRef();
	  if (type != fontType1 && type != fontType1C) {
	    error(-1, "Mismatch between font type and embedded font file");
	  }
	  type = fontType1C;
	} else if (obj4.isName("TrueType")) {
	    embFontID = obj2.getRef();
	  if (type != fontTrueType) {
	    error(-1, "Mismatch between font type and embedded font file");
	    type = fontTrueType;
	  }
	} else if (obj4.isName("CIDFontType0C")) {
	    embFontID = obj2.getRef();
	  if (type != fontCIDType0) {
	    error(-1, "Mismatch between font type and embedded font file");
	  }
	  type = fontCIDType0C;
	} else {
	  error(-1, "Unknown embedded font type '%s'",
		obj4.isName() ? obj4.getNameC() : "???");
	}
	obj4.free();
      }
      obj3.free();
    }
    obj2.free();

    // look for MissingWidth
    obj1.dictLookup("MissingWidth", &obj2);
    if (obj2.isNum()) {
      missingWidth = obj2.getNum();
    }
    obj2.free();

    // get Ascent and Descent
    obj1.dictLookup("Ascent", &obj2);
    if (obj2.isNum()) {
      t = 0.001 * obj2.getNum();
      // some broken font descriptors set ascent and descent to 0
      if (t != 0) {
	ascent = t;
      }
    }
    obj2.free();
    obj1.dictLookup("Descent", &obj2);
    if (obj2.isNum()) {
      t = 0.001 * obj2.getNum();
      // some broken font descriptors set ascent and descent to 0
      if (t != 0) {
	descent = t;
      }
      // some broken font descriptors specify a positive descent
      if (descent > 0) {
	descent = -descent;
      }
    }
    obj2.free();

    // font FontBBox
    if (obj1.dictLookup("FontBBox", &obj2)->isArray()) {
      for (i = 0; i < 4 && i < obj2.arrayGetLength(); ++i) {
	if (obj2.arrayGet(i, &obj3)->isNum()) {
	  fontBBox[i] = 0.001 * obj3.getNum();
	}
	obj3.free();
      }
    }
    obj2.free();

  }
  obj1.free();
}

CharCodeToUnicode *GfxFont::readToUnicodeCMap(Dict *fontDict, int nBits,
					      CharCodeToUnicode *ctu) {
  GooString *buf;
  Object obj1;
  int c;

  if (!fontDict->lookup("ToUnicode", &obj1)->isStream()) {
    obj1.free();
    return NULL;
  }
  buf = new GooString();
  obj1.streamReset();
  while ((c = obj1.streamGetChar()) != EOF) {
    buf->append(c);
  }
  obj1.streamClose();
  obj1.free();
  if (ctu) {
    ctu->mergeCMap(buf, nBits);
  } else {
    ctu = CharCodeToUnicode::parseCMap(buf, nBits);
  }
  delete buf;
  return ctu;
}

void GfxFont::findExtFontFile() {
  static char *type1Exts[] = { ".pfa", ".pfb", ".ps", "", NULL };
  static char *ttExts[] = { ".ttf", ".ttc", NULL };

  if (name) {
    if (type == fontType1) {
      extFontFile = globalParams->findFontFile(name, type1Exts);
    } else if (type == fontTrueType) {
      extFontFile = globalParams->findFontFile(name, ttExts);
    }
  }
}

char *GfxFont::readExtFontFile(int *len) {
  FILE *f;
  char *buf;

  if (!(f = fopen(extFontFile->getCString(), "rb"))) {
    error(-1, "External font file '%s' vanished", extFontFile->getCString());
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  *len = (int)ftell(f);
  fseek(f, 0, SEEK_SET);
  buf = (char *)gmalloc(*len);
  if ((int)fread(buf, 1, *len, f) != *len) {
    error(-1, "Error reading external font file '%s'",
	  extFontFile->getCString());
  }
  fclose(f);
  return buf;
}

char *GfxFont::readEmbFontFile(XRef *xref, int *len) {
  char *buf;
  Object obj1, obj2;
  Stream *str;
  int c;
  int size, i;

  obj1.initRef(embFontID.num, embFontID.gen);
  obj1.fetch(xref, &obj2);
  if (!obj2.isStream()) {
    error(-1, "Embedded font file is not a stream");
    obj2.free();
    obj1.free();
    embFontID.num = -1;
    return NULL;
  }
  str = obj2.getStream();

  buf = NULL;
  i = size = 0;
  str->reset();
  while ((c = str->getChar()) != EOF) {
    if (i == size) {
      size += 4096;
      buf = (char *)grealloc(buf, size);
    }
    buf[i++] = c;
  }
  *len = i;
  str->close();

  obj2.free();
  obj1.free();

  return buf;
}

//------------------------------------------------------------------------
// Gfx8BitFont
//------------------------------------------------------------------------

Gfx8BitFont::Gfx8BitFont(XRef *xref, char *tagA, Ref idA, GooString *nameA,
			 GfxFontType typeA, Dict *fontDict):
  GfxFont(tagA, idA, nameA)
{
  GooString *name2;
  BuiltinFont *builtinFont;
  char **baseEnc;
  GBool baseEncFromFontFile;
  char *buf;
  int len;
  FoFiType1 *ffT1;
  FoFiType1C *ffT1C;
  int code, code2;
  char *charName;
  GBool missing, hex;
  Unicode toUnicode[256];
  CharCodeToUnicode *utu, *ctu2;
  Unicode uBuf[8];
  double mul;
  int firstChar, lastChar;
  Gushort w;
  Object obj1, obj2, obj3;
  int n, i, a, b, m;

  refCnt = 1;
  type = typeA;
  ctu = NULL;

  // do font name substitution for various aliases of the Base 14 font
  // names
  if (name) {
    name2 = name->copy();
    i = 0;
    while (i < name2->getLength()) {
      if (name2->getChar(i) == ' ') {
	name2->del(i);
      } else {
	++i;
      }
    }
    a = 0;
    b = sizeof(stdFontMap) / sizeof(StdFontMapEntry);
    // invariant: stdFontMap[a].altName <= name2 < stdFontMap[b].altName
    while (b - a > 1) {
      m = (a + b) / 2;
      if (name2->cmp(stdFontMap[m].altName) >= 0) {
	a = m;
      } else {
	b = m;
      }
    }
    if (!name2->cmp(stdFontMap[a].altName)) {
      name = new GooString(stdFontMap[a].properName);
    }
    delete name2;
  }

  // is it a built-in font?
  builtinFont = NULL;
  if (name) {
    for (i = 0; i < nBuiltinFonts; ++i) {
      if (!name->cmp(builtinFonts[i].name)) {
	builtinFont = &builtinFonts[i];
	break;
      }
    }
  }

  // default ascent/descent values
  if (builtinFont) {
    ascent = 0.001 * builtinFont->ascent;
    descent = 0.001 * builtinFont->descent;
    fontBBox[0] = 0.001 * builtinFont->bbox[0];
    fontBBox[1] = 0.001 * builtinFont->bbox[1];
    fontBBox[2] = 0.001 * builtinFont->bbox[2];
    fontBBox[3] = 0.001 * builtinFont->bbox[3];
  } else {
    ascent = 0.95;
    descent = -0.35;
    fontBBox[0] = fontBBox[1] = fontBBox[2] = fontBBox[3] = 0;
  }

  // get info from font descriptor
  readFontDescriptor(xref, fontDict);

  // for non-embedded fonts, don't trust the ascent/descent/bbox
  // values from the font descriptor
  if (builtinFont && embFontID.num < 0) {
    ascent = 0.001 * builtinFont->ascent;
    descent = 0.001 * builtinFont->descent;
    fontBBox[0] = 0.001 * builtinFont->bbox[0];
    fontBBox[1] = 0.001 * builtinFont->bbox[1];
    fontBBox[2] = 0.001 * builtinFont->bbox[2];
    fontBBox[3] = 0.001 * builtinFont->bbox[3];
  }

  // look for an external font file
  findExtFontFile();

  // get font matrix
  fontMat[0] = fontMat[3] = 1;
  fontMat[1] = fontMat[2] = fontMat[4] = fontMat[5] = 0;
  if (fontDict->lookup("FontMatrix", &obj1)->isArray()) {
    for (i = 0; i < 6 && i < obj1.arrayGetLength(); ++i) {
      if (obj1.arrayGet(i, &obj2)->isNum()) {
	fontMat[i] = obj2.getNum();
      }
      obj2.free();
    }
  }
  obj1.free();

  // get Type 3 bounding box, font definition, and resources
  if (type == fontType3) {
    if (fontDict->lookup("FontBBox", &obj1)->isArray()) {
      for (i = 0; i < 4 && i < obj1.arrayGetLength(); ++i) {
	if (obj1.arrayGet(i, &obj2)->isNum()) {
	  fontBBox[i] = obj2.getNum();
	}
	obj2.free();
      }
    }
    obj1.free();
    if (!fontDict->lookup("CharProcs", &charProcs)->isDict()) {
      error(-1, "Missing or invalid CharProcs dictionary in Type 3 font");
      charProcs.free();
    }
    if (!fontDict->lookup("Resources", &resources)->isDict()) {
      resources.free();
    }
  }

  //----- build the font encoding -----

  // Encodings start with a base encoding, which can come from
  // (in order of priority):
  //   1. FontDict.Encoding or FontDict.Encoding.BaseEncoding
  //        - MacRoman / MacExpert / WinAnsi / Standard
  //   2. embedded or external font file
  //   3. default:
  //        - builtin --> builtin encoding
  //        - TrueType --> WinAnsiEncoding
  //        - others --> StandardEncoding
  // and then add a list of differences (if any) from
  // FontDict.Encoding.Differences.

  // check FontDict for base encoding
  hasEncoding = gFalse;
  usesMacRomanEnc = gFalse;
  baseEnc = NULL;
  baseEncFromFontFile = gFalse;
  fontDict->lookup("Encoding", &obj1);
  if (obj1.isDict()) {
    obj1.dictLookup("BaseEncoding", &obj2);
    if (obj2.isName("MacRomanEncoding")) {
      hasEncoding = gTrue;
      usesMacRomanEnc = gTrue;
      baseEnc = macRomanEncoding;
    } else if (obj2.isName("MacExpertEncoding")) {
      hasEncoding = gTrue;
      baseEnc = macExpertEncoding;
    } else if (obj2.isName("WinAnsiEncoding")) {
      hasEncoding = gTrue;
      baseEnc = winAnsiEncoding;
    }
    obj2.free();
  } else if (obj1.isName("MacRomanEncoding")) {
    hasEncoding = gTrue;
    usesMacRomanEnc = gTrue;
    baseEnc = macRomanEncoding;
  } else if (obj1.isName("MacExpertEncoding")) {
    hasEncoding = gTrue;
    baseEnc = macExpertEncoding;
  } else if (obj1.isName("WinAnsiEncoding")) {
    hasEncoding = gTrue;
    baseEnc = winAnsiEncoding;
  }

  // check embedded or external font file for base encoding
  // (only for Type 1 fonts - trying to get an encoding out of a
  // TrueType font is a losing proposition)
  ffT1 = NULL;
  ffT1C = NULL;
  buf = NULL;
  if (type == fontType1 && (extFontFile || embFontID.num >= 0)) {
    if (extFontFile) {
      ffT1 = FoFiType1::load(extFontFile->getCString());
    } else {
      buf = readEmbFontFile(xref, &len);
      ffT1 = FoFiType1::make(buf, len);
    }
    if (ffT1) {
      if (ffT1->getName()) {
	if (embFontName) {
	  delete embFontName;
	}
	embFontName = new GooString(ffT1->getName());
      }
      if (!baseEnc) {
	baseEnc = ffT1->getEncoding();
	baseEncFromFontFile = gTrue;
      }
    }
  } else if (type == fontType1C && (extFontFile || embFontID.num >= 0)) {
    if (extFontFile) {
      ffT1C = FoFiType1C::load(extFontFile->getCString());
    } else {
      buf = readEmbFontFile(xref, &len);
      ffT1C = FoFiType1C::make(buf, len);
    }
    if (ffT1C) {
      if (ffT1C->getName()) {
	if (embFontName) {
	  delete embFontName;
	}
	embFontName = new GooString(ffT1C->getName());
      }
      if (!baseEnc) {
	baseEnc = ffT1C->getEncoding();
	baseEncFromFontFile = gTrue;
      }
    }
  }
  if (buf) {
    gfree(buf);
  }

  // get default base encoding
  if (!baseEnc) {
    if (builtinFont && embFontID.num < 0) {
      baseEnc = builtinFont->defaultBaseEnc;
      hasEncoding = gTrue;
    } else if (type == fontTrueType) {
      baseEnc = winAnsiEncoding;
    } else {
      baseEnc = standardEncoding;
    }
  }

  // copy the base encoding
  for (i = 0; i < 256; ++i) {
    enc[i] = baseEnc[i];
    if ((encFree[i] = baseEncFromFontFile) && enc[i]) {
      enc[i] = copyString(baseEnc[i]);
    }
  }

  // some Type 1C font files have empty encodings, which can break the
  // T1C->T1 conversion (since the 'seac' operator depends on having
  // the accents in the encoding), so we fill in any gaps from
  // StandardEncoding
  if (type == fontType1C && (extFontFile || embFontID.num >= 0) &&
      baseEncFromFontFile) {
    for (i = 0; i < 256; ++i) {
      if (!enc[i] && standardEncoding[i]) {
	enc[i] = standardEncoding[i];
	encFree[i] = gFalse;
      }
    }
  }

  // merge differences into encoding
  if (obj1.isDict()) {
    obj1.dictLookup("Differences", &obj2);
    if (obj2.isArray()) {
      hasEncoding = gTrue;
      code = 0;
      for (i = 0; i < obj2.arrayGetLength(); ++i) {
	obj2.arrayGet(i, &obj3);
	if (obj3.isInt()) {
	  code = obj3.getInt();
	} else if (obj3.isName()) {
	  if (code >= 0 && code < 256) {
	    if (encFree[code]) {
	      gfree(enc[code]);
	    }
	    enc[code] = copyString(obj3.getNameC());
	    encFree[code] = gTrue;
	  }
	  ++code;
	} else {
	  error(-1, "Wrong type in font encoding resource differences (%s)",
		obj3.getTypeName());
	}
	obj3.free();
      }
    }
    obj2.free();
  }
  obj1.free();
  if (ffT1) {
    delete ffT1;
  }
  if (ffT1C) {
    delete ffT1C;
  }

  //----- build the mapping to Unicode -----

  // pass 1: use the name-to-Unicode mapping table
  missing = hex = gFalse;
  for (code = 0; code < 256; ++code) {
    if ((charName = enc[code])) {
      if (!(toUnicode[code] = globalParams->mapNameToUnicode(charName)) &&
	  strcmp(charName, ".notdef")) {
	// if it wasn't in the name-to-Unicode table, check for a
	// name that looks like 'Axx' or 'xx', where 'A' is any letter
	// and 'xx' is two hex digits
	if ((strlen(charName) == 3 &&
	     isalpha(charName[0]) &&
	     isxdigit(charName[1]) && isxdigit(charName[2]) &&
	     ((charName[1] >= 'a' && charName[1] <= 'f') ||
	      (charName[1] >= 'A' && charName[1] <= 'F') ||
	      (charName[2] >= 'a' && charName[2] <= 'f') ||
	      (charName[2] >= 'A' && charName[2] <= 'F'))) ||
	    (strlen(charName) == 2 &&
	     isxdigit(charName[0]) && isxdigit(charName[1]) &&
	     ((charName[0] >= 'a' && charName[0] <= 'f') ||
	      (charName[0] >= 'A' && charName[0] <= 'F') ||
	      (charName[1] >= 'a' && charName[1] <= 'f') ||
	      (charName[1] >= 'A' && charName[1] <= 'F')))) {
	  hex = gTrue;
	}
	missing = gTrue;
      }
    } else {
      toUnicode[code] = 0;
    }
  }

  // pass 2: try to fill in the missing chars, looking for names of
  // the form 'Axx', 'xx', 'Ann', 'ABnn', or 'nn', where 'A' and 'B'
  // are any letters, 'xx' is two hex digits, and 'nn' is 2-4
  // decimal digits
  if (missing && globalParams->getMapNumericCharNames()) {
    for (code = 0; code < 256; ++code) {
      if ((charName = enc[code]) && !toUnicode[code] &&
	  strcmp(charName, ".notdef")) {
	n = strlen(charName);
	code2 = -1;
	if (hex && n == 3 && isalpha(charName[0]) &&
	    isxdigit(charName[1]) && isxdigit(charName[2])) {
	  sscanf(charName+1, "%x", &code2);
	} else if (hex && n == 2 &&
		   isxdigit(charName[0]) && isxdigit(charName[1])) {
	  sscanf(charName, "%x", &code2);
	} else if (!hex && n >= 2 && n <= 4 &&
		   isdigit(charName[0]) && isdigit(charName[1])) {
	  code2 = atoi(charName);
	} else if (n >= 3 && n <= 5 &&
		   isdigit(charName[1]) && isdigit(charName[2])) {
	  code2 = atoi(charName+1);
	} else if (n >= 4 && n <= 6 &&
		   isdigit(charName[2]) && isdigit(charName[3])) {
	  code2 = atoi(charName+2);
	}
	if (code2 >= 0 && code2 <= 0xff) {
	  toUnicode[code] = (Unicode)code2;
	}
      }
    }
  }

  // construct the char code -> Unicode mapping object
  ctu = CharCodeToUnicode::make8BitToUnicode(toUnicode);

  // merge in a ToUnicode CMap, if there is one -- this overwrites
  // existing entries in ctu, i.e., the ToUnicode CMap takes
  // precedence, but the other encoding info is allowed to fill in any
  // holes
  readToUnicodeCMap(fontDict, 8, ctu);

  // look for a Unicode-to-Unicode mapping
  if (name && (utu = globalParams->getUnicodeToUnicode(name))) {
    for (i = 0; i < 256; ++i) {
      toUnicode[i] = 0;
    }
    ctu2 = CharCodeToUnicode::make8BitToUnicode(toUnicode);
    for (i = 0; i < 256; ++i) {
      n = ctu->mapToUnicode((CharCode)i, uBuf, 8);
      if (n >= 1) {
	n = utu->mapToUnicode((CharCode)uBuf[0], uBuf, 8);
	if (n >= 1) {
	  ctu2->setMapping((CharCode)i, uBuf, n);
	}
      }
    }
    utu->decRefCnt();
    delete ctu;
    ctu = ctu2;
  }

  //----- get the character widths -----

  // initialize all widths
  for (code = 0; code < 256; ++code) {
    widths[code] = missingWidth * 0.001;
  }

  // use widths from font dict, if present
  fontDict->lookup("FirstChar", &obj1);
  firstChar = obj1.isInt() ? obj1.getInt() : 0;
  obj1.free();
  if (firstChar < 0 || firstChar > 255) {
    firstChar = 0;
  }
  fontDict->lookup("LastChar", &obj1);
  lastChar = obj1.isInt() ? obj1.getInt() : 255;
  obj1.free();
  if (lastChar < 0 || lastChar > 255) {
    lastChar = 255;
  }
  mul = (type == fontType3) ? fontMat[0] : 0.001;
  fontDict->lookup("Widths", &obj1);
  if (obj1.isArray()) {
    flags |= fontFixedWidth;
    if (obj1.arrayGetLength() < lastChar - firstChar + 1) {
      lastChar = firstChar + obj1.arrayGetLength() - 1;
    }
    for (code = firstChar; code <= lastChar; ++code) {
      obj1.arrayGet(code - firstChar, &obj2);
      if (obj2.isNum()) {
	widths[code] = obj2.getNum() * mul;
	if (widths[code] != widths[firstChar]) {
	  flags &= ~fontFixedWidth;
	}
      }
      obj2.free();
    }

  // use widths from built-in font
  } else if (builtinFont) {
    // this is a kludge for broken PDF files that encode char 32
    // as .notdef
    if (builtinFont->widths->getWidth("space", &w)) {
      widths[32] = 0.001 * w;
    }
    for (code = 0; code < 256; ++code) {
      if (enc[code] && builtinFont->widths->getWidth(enc[code], &w)) {
	widths[code] = 0.001 * w;
      }
    }

  // couldn't find widths -- use defaults 
  } else {
    // this is technically an error -- the Widths entry is required
    // for all but the Base-14 fonts -- but certain PDF generators
    // apparently don't include widths for Arial and TimesNewRoman
    if (isFixedWidth()) {
      i = 0;
    } else if (isSerif()) {
      i = 8;
    } else {
      i = 4;
    }
    if (isBold()) {
      i += 2;
    }
    if (isItalic()) {
      i += 1;
    }
    builtinFont = builtinFontSubst[i];
    // this is a kludge for broken PDF files that encode char 32
    // as .notdef
    if (builtinFont->widths->getWidth("space", &w)) {
      widths[32] = 0.001 * w;
    }
    for (code = 0; code < 256; ++code) {
      if (enc[code] && builtinFont->widths->getWidth(enc[code], &w)) {
	widths[code] = 0.001 * w;
      }
    }
  }
  obj1.free();

  ok = gTrue;
}

Gfx8BitFont::~Gfx8BitFont() {
  int i;

  for (i = 0; i < 256; ++i) {
    if (encFree[i] && enc[i]) {
      gfree(enc[i]);
    }
  }
  ctu->decRefCnt();
  if (charProcs.isDict()) {
    charProcs.free();
  }
  if (resources.isDict()) {
    resources.free();
  }
}

int Gfx8BitFont::getNextChar(char *s, int len, CharCode *code,
			     Unicode *u, int uSize, int *uLen,
			     double *dx, double *dy, double *ox, double *oy) {
  CharCode c;

  *code = c = (CharCode)(*s & 0xff);
  *uLen = ctu->mapToUnicode(c, u, uSize);
  *dx = widths[c];
  *dy = *ox = *oy = 0;
  return 1;
}

CharCodeToUnicode *Gfx8BitFont::getToUnicode() {
  ctu->incRefCnt();
  return ctu;
}

Gushort *Gfx8BitFont::getCodeToGIDMap(FoFiTrueType *ff) {
  Gushort *map;
  int cmapPlatform, cmapEncoding;
  int unicodeCmap, macRomanCmap, msSymbolCmap, cmap;
  GBool useMacRoman, useUnicode;
  char *charName;
  Unicode u;
  int code, i, n;

  map = (Gushort *)gmallocn(256, sizeof(Gushort));
  for (i = 0; i < 256; ++i) {
    map[i] = 0;
  }

  // To match up with the Adobe-defined behaviour, we choose a cmap
  // like this:
  // 1. If the PDF font has an encoding:
  //    1a. If the PDF font specified MacRomanEncoding and the
  //        TrueType font has a Macintosh Roman cmap, use it, and
  //        reverse map the char names through MacRomanEncoding to
  //        get char codes.
  //    1b. If the TrueType font has a Microsoft Unicode cmap or a
  //        non-Microsoft Unicode cmap, use it, and use the Unicode
  //        indexes, not the char codes.
  //    1c. If the PDF font is symbolic and the TrueType font has a
  //        Microsoft Symbol cmap, use it, and use char codes
  //        directly (possibly with an offset of 0xf000).
  //    1d. If the TrueType font has a Macintosh Roman cmap, use it,
  //        as in case 1a.
  // 2. If the PDF font does not have an encoding or the PDF font is
  //    symbolic:
  //    2a. If the TrueType font has a Macintosh Roman cmap, use it,
  //        and use char codes directly (possibly with an offset of
  //        0xf000).
  //    2b. If the TrueType font has a Microsoft Symbol cmap, use it,
  //        and use char codes directly (possible with an offset of
  //        0xf000).
  // 3. If none of these rules apply, use the first cmap and hope for
  //    the best (this shouldn't happen).
  unicodeCmap = macRomanCmap = msSymbolCmap = -1;
  for (i = 0; i < ff->getNumCmaps(); ++i) {
    cmapPlatform = ff->getCmapPlatform(i);
    cmapEncoding = ff->getCmapEncoding(i);
    if ((cmapPlatform == 3 && cmapEncoding == 1) ||
	cmapPlatform == 0) {
      unicodeCmap = i;
    } else if (cmapPlatform == 1 && cmapEncoding == 0) {
      macRomanCmap = i;
    } else if (cmapPlatform == 3 && cmapEncoding == 0) {
      msSymbolCmap = i;
    }
  }
  cmap = 0;
  useMacRoman = gFalse;
  useUnicode = gFalse;
  if (hasEncoding) {
    if (usesMacRomanEnc && macRomanCmap >= 0) {
      cmap = macRomanCmap;
      useMacRoman = gTrue;
    } else if (unicodeCmap >= 0) {
      cmap = unicodeCmap;
      useUnicode = gTrue;
    } else if ((flags & fontSymbolic) && msSymbolCmap >= 0) {
      cmap = msSymbolCmap;
    } else if ((flags & fontSymbolic) && macRomanCmap >= 0) {
      cmap = macRomanCmap;
    } else if (macRomanCmap >= 0) {
      cmap = macRomanCmap;
      useMacRoman = gTrue;
    }
  } else {
    if (macRomanCmap >= 0) {
      cmap = macRomanCmap;
    } else if (msSymbolCmap >= 0) {
      cmap = msSymbolCmap;
    }
  }

  // reverse map the char names through MacRomanEncoding, then map the
  // char codes through the cmap
  if (useMacRoman) {
    for (i = 0; i < 256; ++i) {
      if ((charName = enc[i])) {
	if ((code = globalParams->getMacRomanCharCode(charName))) {
	  map[i] = ff->mapCodeToGID(cmap, code);
	}
      }
    }

  // map Unicode through the cmap
  } else if (useUnicode) {
    for (i = 0; i < 256; ++i) {
      if (((charName = enc[i]) &&
	   (u = globalParams->mapNameToUnicode(charName))) ||
	  (n = ctu->mapToUnicode((CharCode)i, &u, 1))) {
	map[i] = ff->mapCodeToGID(cmap, u);
      }
    }

  // map the char codes through the cmap, possibly with an offset of
  // 0xf000
  } else {
    for (i = 0; i < 256; ++i) {
      if (!(map[i] = ff->mapCodeToGID(cmap, i))) {
	map[i] = ff->mapCodeToGID(cmap, 0xf000 + i);
      }
    }
  }

  // try the TrueType 'post' table to handle any unmapped characters
  for (i = 0; i < 256; ++i) {
    if (!map[i] && (charName = enc[i])) {
      map[i] = (Gushort)(int)ff->mapNameToGID(charName);
    }
  }

  return map;
}

Dict *Gfx8BitFont::getCharProcs() {
  return charProcs.isDict() ? charProcs.getDict() : (Dict *)NULL;
}

Object *Gfx8BitFont::getCharProc(int code, Object *proc) {
  if (enc[code] && charProcs.isDict()) {
    charProcs.dictLookup(enc[code], proc);
  } else {
    proc->initNull();
  }
  return proc;
}

Dict *Gfx8BitFont::getResources() {
  return resources.isDict() ? resources.getDict() : (Dict *)NULL;
}

//------------------------------------------------------------------------
// GfxCIDFont
//------------------------------------------------------------------------

static int CDECL cmpWidthExcep(const void *w1, const void *w2) {
  return ((GfxFontCIDWidthExcep *)w1)->first -
         ((GfxFontCIDWidthExcep *)w2)->first;
}

static int CDECL cmpWidthExcepV(const void *w1, const void *w2) {
  return ((GfxFontCIDWidthExcepV *)w1)->first -
         ((GfxFontCIDWidthExcepV *)w2)->first;
}

GfxCIDFont::GfxCIDFont(XRef *xref, char *tagA, Ref idA, GooString *nameA,
		       Dict *fontDict):
  GfxFont(tagA, idA, nameA)
{
  Dict *desFontDict;
  GooString *collection, *cMapName;
  Object desFontDictObj;
  Object obj1, obj2, obj3, obj4, obj5, obj6;
  CharCodeToUnicode *utu;
  CharCode c;
  Unicode uBuf[8];
  int c1, c2;
  int excepsSize, i, j, k, n;

  refCnt = 1;
  ascent = 0.95;
  descent = -0.35;
  fontBBox[0] = fontBBox[1] = fontBBox[2] = fontBBox[3] = 0;
  cMap = NULL;
  ctu = NULL;
  widths.defWidth = 1.0;
  widths.defHeight = -1.0;
  widths.defVY = 0.880;
  widths.exceps = NULL;
  widths.nExceps = 0;
  widths.excepsV = NULL;
  widths.nExcepsV = 0;
  cidToGID = NULL;
  cidToGIDLen = 0;

  // get the descendant font
  if (!fontDict->lookup("DescendantFonts", &obj1)->isArray()) {
    error(-1, "Missing DescendantFonts entry in Type 0 font");
    obj1.free();
    goto err1;
  }
  if (!obj1.arrayGet(0, &desFontDictObj)->isDict()) {
    error(-1, "Bad descendant font in Type 0 font");
    goto err3;
  }
  obj1.free();
  desFontDict = desFontDictObj.getDict();

  // font type
  if (!desFontDict->lookup("Subtype", &obj1)) {
    error(-1, "Missing Subtype entry in Type 0 descendant font");
    goto err3;
  }
  if (obj1.isName("CIDFontType0")) {
    type = fontCIDType0;
  } else if (obj1.isName("CIDFontType2")) {
    type = fontCIDType2;
  } else {
    error(-1, "Unknown Type 0 descendant font type '%s'",
	  obj1.isName() ? obj1.getNameC() : "???");
    goto err3;
  }
  obj1.free();

  // get info from font descriptor
  readFontDescriptor(xref, desFontDict);

  // look for an external font file
  findExtFontFile();

  //----- encoding info -----

  // char collection
  if (!desFontDict->lookup("CIDSystemInfo", &obj1)->isDict()) {
    error(-1, "Missing CIDSystemInfo dictionary in Type 0 descendant font");
    goto err3;
  }
  obj1.dictLookup("Registry", &obj2);
  obj1.dictLookup("Ordering", &obj3);
  if (!obj2.isString() || !obj3.isString()) {
    error(-1, "Invalid CIDSystemInfo dictionary in Type 0 descendant font");
    goto err4;
  }
  collection = obj2.getString()->copy()->append('-')->append(obj3.getString());
  obj3.free();
  obj2.free();
  obj1.free();

  // look for a ToUnicode CMap
  if (!(ctu = readToUnicodeCMap(fontDict, 16, NULL))) {

    // the "Adobe-Identity" and "Adobe-UCS" collections don't have
    // cidToUnicode files
    if (collection->cmp("Adobe-Identity") &&
	collection->cmp("Adobe-UCS")) {

      // look for a user-supplied .cidToUnicode file
      if (!(ctu = globalParams->getCIDToUnicode(collection))) {
	error(-1, "Unknown character collection '%s'",
	      collection->getCString());
	// fall-through, assuming the Identity mapping -- this appears
	// to match Adobe's behavior
      }
    }
  }

  // look for a Unicode-to-Unicode mapping
  if (name && (utu = globalParams->getUnicodeToUnicode(name))) {
    if (ctu) {
      for (c = 0; c < ctu->getLength(); ++c) {
	n = ctu->mapToUnicode(c, uBuf, 8);
	if (n >= 1) {
	  n = utu->mapToUnicode((CharCode)uBuf[0], uBuf, 8);
	  if (n >= 1) {
	    ctu->setMapping(c, uBuf, n);
      }
    }
  }
      utu->decRefCnt();
    } else {
      ctu = utu;
    }
  }

  // encoding (i.e., CMap)
  //~ need to handle a CMap stream here
  //~ also need to deal with the UseCMap entry in the stream dict
  if (!fontDict->lookup("Encoding", &obj1)->isName()) {
    error(-1, "Missing or invalid Encoding entry in Type 0 font");
    delete collection;
    goto err3;
  }
  cMapName = new GooString(obj1.getName());
  obj1.free();
  if (!(cMap = globalParams->getCMap(collection, cMapName))) {
    error(-1, "Unknown CMap '%s' for character collection '%s'",
	  cMapName->getCString(), collection->getCString());
    delete collection;
    delete cMapName;
    goto err2;
  }
  delete collection;
  delete cMapName;

  // CIDToGIDMap (for embedded TrueType fonts)
  if (type == fontCIDType2) {
    desFontDict->lookup("CIDToGIDMap", &obj1);
    if (obj1.isStream()) {
      cidToGIDLen = 0;
      i = 64;
      cidToGID = (Gushort *)gmallocn(i, sizeof(Gushort));
      obj1.streamReset();
      while ((c1 = obj1.streamGetChar()) != EOF &&
	     (c2 = obj1.streamGetChar()) != EOF) {
	if (cidToGIDLen == i) {
	  i *= 2;
	  cidToGID = (Gushort *)greallocn(cidToGID, i, sizeof(Gushort));
	}
	cidToGID[cidToGIDLen++] = (Gushort)((c1 << 8) + c2);
      }
    } else if (!obj1.isName("Identity") && !obj1.isNull()) {
      error(-1, "Invalid CIDToGIDMap entry in CID font");
    }
    obj1.free();
  }

  //----- character metrics -----

  // default char width
  if (desFontDict->lookup("DW", &obj1)->isInt()) {
    widths.defWidth = obj1.getInt() * 0.001;
  }
  obj1.free();

  // char width exceptions
  if (desFontDict->lookup("W", &obj1)->isArray()) {
    excepsSize = 0;
    i = 0;
    while (i + 1 < obj1.arrayGetLength()) {
      obj1.arrayGet(i, &obj2);
      obj1.arrayGet(i + 1, &obj3);
      if (obj2.isInt() && obj3.isInt() && i + 2 < obj1.arrayGetLength()) {
	if (obj1.arrayGet(i + 2, &obj4)->isNum()) {
	  if (widths.nExceps == excepsSize) {
	    excepsSize += 16;
	    widths.exceps = (GfxFontCIDWidthExcep *)
	      greallocn(widths.exceps,
			excepsSize, sizeof(GfxFontCIDWidthExcep));
	  }
	  widths.exceps[widths.nExceps].first = obj2.getInt();
	  widths.exceps[widths.nExceps].last = obj3.getInt();
	  widths.exceps[widths.nExceps].width = obj4.getNum() * 0.001;
	  ++widths.nExceps;
	} else {
	  error(-1, "Bad widths array in Type 0 font");
	}
	obj4.free();
	i += 3;
      } else if (obj2.isInt() && obj3.isArray()) {
	if (widths.nExceps + obj3.arrayGetLength() > excepsSize) {
	  excepsSize = (widths.nExceps + obj3.arrayGetLength() + 15) & ~15;
	  widths.exceps = (GfxFontCIDWidthExcep *)
	    greallocn(widths.exceps,
		      excepsSize, sizeof(GfxFontCIDWidthExcep));
	}
	j = obj2.getInt();
	for (k = 0; k < obj3.arrayGetLength(); ++k) {
	  if (obj3.arrayGet(k, &obj4)->isNum()) {
	    widths.exceps[widths.nExceps].first = j;
	    widths.exceps[widths.nExceps].last = j;
	    widths.exceps[widths.nExceps].width = obj4.getNum() * 0.001;
	    ++j;
	    ++widths.nExceps;
	  } else {
	    error(-1, "Bad widths array in Type 0 font");
	  }
	  obj4.free();
	}
	i += 2;
      } else {
	error(-1, "Bad widths array in Type 0 font");
	++i;
      }
      obj3.free();
      obj2.free();
    }
    qsort(widths.exceps, widths.nExceps, sizeof(GfxFontCIDWidthExcep),
	  &cmpWidthExcep);
  }
  obj1.free();

  // default metrics for vertical font
  if (desFontDict->lookup("DW2", &obj1)->isArray() &&
      obj1.arrayGetLength() == 2) {
    if (obj1.arrayGet(0, &obj2)->isNum()) {
      widths.defVY = obj2.getNum() * 0.001;
    }
    obj2.free();
    if (obj1.arrayGet(1, &obj2)->isNum()) {
      widths.defHeight = obj2.getNum() * 0.001;
    }
    obj2.free();
  }
  obj1.free();

  // char metric exceptions for vertical font
  if (desFontDict->lookup("W2", &obj1)->isArray()) {
    excepsSize = 0;
    i = 0;
    while (i + 1 < obj1.arrayGetLength()) {
      obj1.arrayGet(i, &obj2);
      obj1.arrayGet(i+ 1, &obj3);
      if (obj2.isInt() && obj3.isInt() && i + 4 < obj1.arrayGetLength()) {
	if (obj1.arrayGet(i + 2, &obj4)->isNum() &&
	    obj1.arrayGet(i + 3, &obj5)->isNum() &&
	    obj1.arrayGet(i + 4, &obj6)->isNum()) {
	  if (widths.nExcepsV == excepsSize) {
	    excepsSize += 16;
	    widths.excepsV = (GfxFontCIDWidthExcepV *)
	      greallocn(widths.excepsV,
			excepsSize, sizeof(GfxFontCIDWidthExcepV));
	  }
	  widths.excepsV[widths.nExcepsV].first = obj2.getInt();
	  widths.excepsV[widths.nExcepsV].last = obj3.getInt();
	  widths.excepsV[widths.nExcepsV].height = obj4.getNum() * 0.001;
	  widths.excepsV[widths.nExcepsV].vx = obj5.getNum() * 0.001;
	  widths.excepsV[widths.nExcepsV].vy = obj6.getNum() * 0.001;
	  ++widths.nExcepsV;
	} else {
	  error(-1, "Bad widths (W2) array in Type 0 font");
	}
	obj6.free();
	obj5.free();
	obj4.free();
	i += 5;
      } else if (obj2.isInt() && obj3.isArray()) {
	if (widths.nExcepsV + obj3.arrayGetLength() / 3 > excepsSize) {
	  excepsSize =
	    (widths.nExcepsV + obj3.arrayGetLength() / 3 + 15) & ~15;
	  widths.excepsV = (GfxFontCIDWidthExcepV *)
	    greallocn(widths.excepsV,
		      excepsSize, sizeof(GfxFontCIDWidthExcepV));
	}
	j = obj2.getInt();
	for (k = 0; k < obj3.arrayGetLength(); k += 3) {
	  if (obj3.arrayGet(k, &obj4)->isNum() &&
	      obj3.arrayGet(k+1, &obj5)->isNum() &&
	      obj3.arrayGet(k+2, &obj6)->isNum()) {
	    widths.excepsV[widths.nExceps].first = j;
	    widths.excepsV[widths.nExceps].last = j;
	    widths.excepsV[widths.nExceps].height = obj4.getNum() * 0.001;
	    widths.excepsV[widths.nExceps].vx = obj5.getNum() * 0.001;
	    widths.excepsV[widths.nExceps].vy = obj6.getNum() * 0.001;
	    ++j;
	    ++widths.nExcepsV;
	  } else {
	    error(-1, "Bad widths (W2) array in Type 0 font");
	  }
	  obj6.free();
	  obj5.free();
	  obj4.free();
	}
	i += 2;
      } else {
	error(-1, "Bad widths (W2) array in Type 0 font");
	++i;
      }
      obj3.free();
      obj2.free();
    }
    qsort(widths.excepsV, widths.nExcepsV, sizeof(GfxFontCIDWidthExcepV),
	  &cmpWidthExcepV);
  }
  obj1.free();

  desFontDictObj.free();
  ok = gTrue;
  return;

 err4:
  obj3.free();
  obj2.free();
 err3:
  obj1.free();
 err2:
  desFontDictObj.free();
 err1:;
}

GfxCIDFont::~GfxCIDFont() {
  if (cMap) {
    cMap->decRefCnt();
  }
  if (ctu) {
    ctu->decRefCnt();
  }
  gfree(widths.exceps);
  gfree(widths.excepsV);
  if (cidToGID) {
    gfree(cidToGID);
  }
}

int GfxCIDFont::getNextChar(char *s, int len, CharCode *code,
			    Unicode *u, int uSize, int *uLen,
			    double *dx, double *dy, double *ox, double *oy) {
  CID cid;
  double w, h, vx, vy;
  int n, a, b, m;

  if (!cMap) {
    *code = 0;
    *uLen = 0;
    *dx = *dy = 0;
    return 1;
  }

  *code = (CharCode)(cid = cMap->getCID(s, len, &n));
  if (ctu) {
    *uLen = ctu->mapToUnicode(cid, u, uSize);
  } else {
    *uLen = 0;
  }

  // horizontal
  if (cMap->getWMode() == 0) {
    w = widths.defWidth;
    h = vx = vy = 0;
    if (widths.nExceps > 0 && cid >= widths.exceps[0].first) {
      a = 0;
      b = widths.nExceps;
      // invariant: widths.exceps[a].first <= cid < widths.exceps[b].first
      while (b - a > 1) {
	m = (a + b) / 2;
	if (widths.exceps[m].first <= cid) {
	  a = m;
	} else {
	  b = m;
	}
      }
      if (cid <= widths.exceps[a].last) {
	w = widths.exceps[a].width;
      }
    }

  // vertical
  } else {
    w = 0;
    h = widths.defHeight;
    vx = widths.defWidth / 2;
    vy = widths.defVY;
    if (widths.nExcepsV > 0 && cid >= widths.excepsV[0].first) {
      a = 0;
      b = widths.nExcepsV;
      // invariant: widths.excepsV[a].first <= cid < widths.excepsV[b].first
      while (b - a > 1) {
	m = (a + b) / 2;
	if (widths.excepsV[m].last <= cid) {
	  a = m;
	} else {
	  b = m;
	}
      }
      if (cid <= widths.excepsV[a].last) {
	h = widths.excepsV[a].height;
	vx = widths.excepsV[a].vx;
	vy = widths.excepsV[a].vy;
      }
    }
  }

  *dx = w;
  *dy = h;
  *ox = vx;
  *oy = vy;

  return n;
}

int GfxCIDFont::getWMode() {
  return cMap ? cMap->getWMode() : 0;
}

CharCodeToUnicode *GfxCIDFont::getToUnicode() {
  if (ctu) {
    ctu->incRefCnt();
  }
  return ctu;
}

GooString *GfxCIDFont::getCollection() {
  return cMap ? cMap->getCollection() : (GooString *)NULL;
}

Gushort *GfxCIDFont::getCodeToGIDMap(FoFiTrueType *ff, int *mapsizep) {
  Gushort *map;
  int cmapPlatform, cmapEncoding;
  int cmap;
  Unicode u;
  int i;
  CharCode mapsize;
  CharCode cidlen;

  *mapsizep = 0;
  if (!ctu) return NULL;

  /* we use only unicode cmap */
  cmap = -1;
  for (i = 0; i < ff->getNumCmaps(); ++i) {
    cmapPlatform = ff->getCmapPlatform(i);
    cmapEncoding = ff->getCmapEncoding(i);
    if ((cmapPlatform == 3 && cmapEncoding == 1) || cmapPlatform == 0)
      cmap = i;
  }
  if (cmap < 0)
    return NULL;

  cidlen = 0;
  mapsize = 64;
  map = (Gushort *)gmalloc(mapsize * sizeof(Gushort));

  while (cidlen < ctu->getLength()) {
    int n;
    if ((n = ctu->mapToUnicode((CharCode)cidlen, &u, 1)) == 0) {
      cidlen++;
      continue;
    }
    if (cidlen >= mapsize) {
      while (cidlen >= mapsize)
        mapsize *= 2;
      map = (Gushort *)grealloc(map, mapsize * sizeof(Gushort));
    }
    map[cidlen] = ff->mapCodeToGID(cmap, u);
    cidlen++;
  }

  *mapsizep = cidlen;
  return map;
}

//------------------------------------------------------------------------
// GfxFontDict
//------------------------------------------------------------------------

GfxFontDict::GfxFontDict(XRef *xref, Ref *fontDictRef, Dict *fontDict) {
  int i;
  Object obj1, obj2;
  Ref r;

  numFonts = fontDict->getLength();
  fonts = (GfxFont **)gmallocn(numFonts, sizeof(GfxFont *));
  for (i = 0; i < numFonts; ++i) {
    fontDict->getValNF(i, &obj1);
    obj1.fetch(xref, &obj2);
    if (obj2.isDict()) {
      if (obj1.isRef()) {
	r = obj1.getRef();
      } else {
	// no indirect reference for this font, so invent a unique one
	// (legal generation numbers are five digits, so any 6-digit
	// number would be safe)
	r.num = i;
	if (fontDictRef) {
	  r.gen = 100000 + fontDictRef->num;
	} else {
	  r.gen = 999999;
	}
      }
      char *aux = fontDict->getKey(i)->getCStringCopy();
      fonts[i] = GfxFont::makeFont(xref, aux,
				   r, obj2.getDict());
      delete[] aux;
      if (fonts[i] && !fonts[i]->isOk()) {
	delete fonts[i];
	fonts[i] = NULL;
      }
    } else {
      error(-1, "font resource is not a dictionary");
      fonts[i] = NULL;
    }
    obj1.free();
    obj2.free();
  }
}

GfxFontDict::~GfxFontDict() {
  int i;

  for (i = 0; i < numFonts; ++i) {
    if (fonts[i]) {
      fonts[i]->decRefCnt();
    }
  }
  gfree(fonts);
}

GfxFont *GfxFontDict::lookup(char *tag) {
  int i;

  for (i = 0; i < numFonts; ++i) {
    if (fonts[i] && fonts[i]->matches(tag)) {
      return fonts[i];
    }
  }
  return NULL;
}
