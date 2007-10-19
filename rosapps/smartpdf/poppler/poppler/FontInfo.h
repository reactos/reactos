#ifndef FONT_INFO_H
#define FONT_INFO_H

#include "goo/gtypes.h"
#include "goo/GooList.h"

class FontInfo {
public:
  enum Type {
    unknown,
    Type1,
    Type1C,
    Type3,
    TrueType,
    CIDType0,
    CIDType0C,
    CIDTrueType
  };

  // Constructor.
  FontInfo(GfxFont *fontA, PDFDoc *doc);
  // Copy constructor
  FontInfo(FontInfo& f);
  // Destructor.
  ~FontInfo();

  GooString *getName()      { return name; };
  GooString *getFile()      { return file; };
  Type       getType()      { return type; };
  GBool      getEmbedded()  { return emb; };
  GBool      getSubset()    { return subset; };
  GBool      getToUnicode() { return hasToUnicode; };

private:
  GooString *name;
  GooString *file;
  Type type;
  GBool emb;
  GBool subset;
  GBool hasToUnicode;
  Ref fontRef;
};

class FontInfoScanner {
public:

  // Constructor.
  FontInfoScanner(PDFDoc *doc);
  // Destructor.
  ~FontInfoScanner();

  GooList *scan(int nPages);

private:

  PDFDoc *doc;
  int currentPage;
  Ref *fonts;
  int fontsLen;
  int fontsSize;

  void scanFonts(Dict *resDict, GooList *fontsList);
};

#endif
