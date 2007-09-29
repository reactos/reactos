//========================================================================
//
// pdffonts.cc
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

#include "config.h"
#include <poppler-config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include "parseargs.h"
#include "goo/GooString.h"
#include "goo/gmem.h"
#include "GlobalParams.h"
#include "Error.h"
#include "Object.h"
#include "Dict.h"
#include "GfxFont.h"
#include "Annot.h"
#include "PDFDoc.h"
#include "UGooString.h"

static char *fontTypeNames[] = {
  "unknown",
  "Type 1",
  "Type 1C",
  "Type 3",
  "TrueType",
  "CID Type 0",
  "CID Type 0C",
  "CID TrueType"
};

static void scanFonts(Dict *resDict, PDFDoc *doc);
static void scanFont(GfxFont *font, PDFDoc *doc);

static int firstPage = 1;
static int lastPage = 0;
static char ownerPassword[33] = "\001";
static char userPassword[33] = "\001";
static char cfgFileName[256] = "";
static GBool printVersion = gFalse;
static GBool printHelp = gFalse;

static ArgDesc argDesc[] = {
  {"-f",      argInt,      &firstPage,     0,
   "first page to examine"},
  {"-l",      argInt,      &lastPage,      0,
   "last page to examine"},
  {"-opw",    argString,   ownerPassword,  sizeof(ownerPassword),
   "owner password (for encrypted files)"},
  {"-upw",    argString,   userPassword,   sizeof(userPassword),
   "user password (for encrypted files)"},
  {"-cfg",        argString,      cfgFileName,    sizeof(cfgFileName),
   "configuration file to use in place of .xpdfrc"},
  {"-v",      argFlag,     &printVersion,  0,
   "print copyright and version info"},
  {"-h",      argFlag,     &printHelp,     0,
   "print usage information"},
  {"-help",   argFlag,     &printHelp,     0,
   "print usage information"},
  {"--help",  argFlag,     &printHelp,     0,
   "print usage information"},
  {"-?",      argFlag,     &printHelp,     0,
   "print usage information"},
  {NULL}
};

static Ref *fonts;
static int fontsLen;
static int fontsSize;

int main(int argc, char *argv[]) {
  PDFDoc *doc;
  GooString *fileName;
  GooString *ownerPW, *userPW;
  GBool ok;
  Page *page;
  Dict *resDict;
  Annots *annots;
  Object obj1, obj2;
  int pg, i;
  int exitCode;

  exitCode = 99;

  // parse args
  ok = parseArgs(argDesc, &argc, argv);
  if (!ok || argc != 2 || printVersion || printHelp) {
    fprintf(stderr, "pdffonts version %s\n", xpdfVersion);
    fprintf(stderr, "%s\n", xpdfCopyright);
    if (!printVersion) {
      printUsage("pdffonts", "<PDF-file>", argDesc);
    }
    goto err0;
  }
  fileName = new GooString(argv[1]);

  // read config file
  globalParams = new GlobalParams(cfgFileName);

  // open PDF file
  if (ownerPassword[0] != '\001') {
    ownerPW = new GooString(ownerPassword);
  } else {
    ownerPW = NULL;
  }
  if (userPassword[0] != '\001') {
    userPW = new GooString(userPassword);
  } else {
    userPW = NULL;
  }

  if(fileName->cmp("-") != 0) {
      doc = new PDFDoc(fileName, ownerPW, userPW);
  } else {
      Object obj;

      obj.initNull();
      doc = new PDFDoc(new FileStream(stdin, 0, gFalse, 0, &obj), ownerPW, userPW);
  }

  if (userPW) {
    delete userPW;
  }
  if (ownerPW) {
    delete ownerPW;
  }
  if (!doc->isOk()) {
    exitCode = 1;
    goto err1;
  }

  // get page range
  if (firstPage < 1) {
    firstPage = 1;
  }
  if (lastPage < 1 || lastPage > doc->getNumPages()) {
    lastPage = doc->getNumPages();
  }

  // scan the fonts
  printf("name                                 type         emb sub uni object ID\n");
  printf("------------------------------------ ------------ --- --- --- ---------\n");
  fonts = NULL;
  fontsLen = fontsSize = 0;
  for (pg = firstPage; pg <= lastPage; ++pg) {
    page = doc->getCatalog()->getPage(pg);
    if ((resDict = page->getResourceDict())) {
      scanFonts(resDict, doc);
    }
    annots = new Annots(doc->getXRef(),
			doc->getCatalog(),
			page->getAnnots(&obj1));
    obj1.free();
    for (i = 0; i < annots->getNumAnnots(); ++i) {
      if (annots->getAnnot(i)->getAppearance(&obj1)->isStream()) {
	obj1.streamGetDict()->lookup("Resources", &obj2);
	if (obj2.isDict()) {
	  scanFonts(obj2.getDict(), doc);
	}
	obj2.free();
      }
      obj1.free();
    }
    delete annots;
  }

  exitCode = 0;

  // clean up
  gfree(fonts);
 err1:
  delete doc;
  delete globalParams;
 err0:

  // check for memory leaks
  Object::memCheck(stderr);
  gMemReport(stderr);

  return exitCode;
}

static void scanFonts(Dict *resDict, PDFDoc *doc) {
  Object obj1, obj2, xObjDict, xObj, resObj;
  Ref r;
  GfxFontDict *gfxFontDict;
  GfxFont *font;
  int i;

  // scan the fonts in this resource dictionary
  gfxFontDict = NULL;
  resDict->lookupNF("Font", &obj1);
  if (obj1.isRef()) {
    obj1.fetch(doc->getXRef(), &obj2);
    if (obj2.isDict()) {
      r = obj1.getRef();
      gfxFontDict = new GfxFontDict(doc->getXRef(), &r, obj2.getDict());
    }
    obj2.free();
  } else if (obj1.isDict()) {
    gfxFontDict = new GfxFontDict(doc->getXRef(), NULL, obj1.getDict());
  }
  if (gfxFontDict) {
    for (i = 0; i < gfxFontDict->getNumFonts(); ++i) {
      if ((font = gfxFontDict->getFont(i))) {
	scanFont(font, doc);
      }
    }
    delete gfxFontDict;
  }
  obj1.free();

  // recursively scan any resource dictionaries in objects in this
  // resource dictionary
  resDict->lookup("XObject", &xObjDict);
  if (xObjDict.isDict()) {
    for (i = 0; i < xObjDict.dictGetLength(); ++i) {
      xObjDict.dictGetVal(i, &xObj);
      if (xObj.isStream()) {
	xObj.streamGetDict()->lookup("Resources", &resObj);
	if (resObj.isDict()) {
	  scanFonts(resObj.getDict(), doc);
	}
	resObj.free();
      }
      xObj.free();
    }
  }
  xObjDict.free();
}

static void scanFont(GfxFont *font, PDFDoc *doc) {
  Ref fontRef, embRef;
  Object fontObj, toUnicodeObj;
  GooString *name;
  GBool emb, subset, hasToUnicode;
  int i;

  fontRef = *font->getID();

  // check for an already-seen font
  for (i = 0; i < fontsLen; ++i) {
    if (fontRef.num == fonts[i].num && fontRef.gen == fonts[i].gen) {
      return;
    }
  }

  // font name
  name = font->getOrigName();

  // check for an embedded font
  if (font->getType() == fontType3) {
    emb = gTrue;
  } else {
    emb = font->getEmbeddedFontID(&embRef);
  }

  // look for a ToUnicode map
  hasToUnicode = gFalse;
  if (doc->getXRef()->fetch(fontRef.num, fontRef.gen, &fontObj)->isDict()) {
    hasToUnicode = fontObj.dictLookup("ToUnicode", &toUnicodeObj)->isStream();
    toUnicodeObj.free();
  }
  fontObj.free();

  // check for a font subset name: capital letters followed by a '+'
  // sign
  subset = gFalse;
  if (name) {
    for (i = 0; i < name->getLength(); ++i) {
      if (name->getChar(i) < 'A' || name->getChar(i) > 'Z') {
	break;
      }
    }
    subset = i > 0 && i < name->getLength() && name->getChar(i) == '+';
  }

  // print the font info
  printf("%-36s %-12s %-3s %-3s %-3s",
	 name ? name->getCString() : "[none]",
	 fontTypeNames[font->getType()],
	 emb ? "yes" : "no",
	 subset ? "yes" : "no",
	 hasToUnicode ? "yes" : "no");
  if (fontRef.gen >= 100000) {
    printf(" [none]\n");
  } else {
    printf(" %6d %2d\n", fontRef.num, fontRef.gen);
  }

  // add this font to the list
  if (fontsLen == fontsSize) {
    fontsSize += 32;
    fonts = (Ref *)grealloc(fonts, fontsSize * sizeof(Ref));
  }
  fonts[fontsLen++] = *font->getID();
}
