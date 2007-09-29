//========================================================================
//
// pdftotext.cc
//
// Copyright 1997-2003 Glyph & Cog, LLC
//
// Modified for Debian by Hamish Moffatt, 22 May 2002.
//
//========================================================================

#include "config.h"
#include <poppler-config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "parseargs.h"
#include "goo/GooString.h"
#include "goo/gmem.h"
#include "GlobalParams.h"
#include "Object.h"
#include "Stream.h"
#include "Array.h"
#include "Dict.h"
#include "XRef.h"
#include "Catalog.h"
#include "Page.h"
#include "PDFDoc.h"
#include "TextOutputDev.h"
#include "CharTypes.h"
#include "UnicodeMap.h"
#include "Error.h"
#include "UGooString.h"

static void printInfoString(FILE *f, Dict *infoDict, char *key,
			    char *text1, char *text2, UnicodeMap *uMap);
static void printInfoDate(FILE *f, Dict *infoDict, char *key, char *fmt);

static int firstPage = 1;
static int lastPage = 0;
static GBool physLayout = gFalse;
static GBool rawOrder = gFalse;
static GBool htmlMeta = gFalse;
static char textEncName[128] = "";
static char textEOL[16] = "";
static GBool noPageBreaks = gFalse;
static char ownerPassword[33] = "\001";
static char userPassword[33] = "\001";
static GBool quiet = gFalse;
static char cfgFileName[256] = "";
static GBool printVersion = gFalse;
static GBool printHelp = gFalse;

static ArgDesc argDesc[] = {
  {"-f",       argInt,      &firstPage,     0,
   "first page to convert"},
  {"-l",       argInt,      &lastPage,      0,
   "last page to convert"},
  {"-layout",  argFlag,     &physLayout,    0,
   "maintain original physical layout"},
  {"-raw",     argFlag,     &rawOrder,      0,
   "keep strings in content stream order"},
  {"-htmlmeta", argFlag,   &htmlMeta,       0,
   "generate a simple HTML file, including the meta information"},
  {"-enc",     argString,   textEncName,    sizeof(textEncName),
   "output text encoding name"},
  {"-eol",     argString,   textEOL,        sizeof(textEOL),
   "output end-of-line convention (unix, dos, or mac)"},
  {"-nopgbrk", argFlag,     &noPageBreaks,  0,
   "don't insert page breaks between pages"},
  {"-opw",     argString,   ownerPassword,  sizeof(ownerPassword),
   "owner password (for encrypted files)"},
  {"-upw",     argString,   userPassword,   sizeof(userPassword),
   "user password (for encrypted files)"},
  {"-q",       argFlag,     &quiet,         0,
   "don't print any messages or errors"},
  {"-cfg",     argString,   cfgFileName,    sizeof(cfgFileName),
   "configuration file to use in place of .xpdfrc"},
  {"-v",       argFlag,     &printVersion,  0,
   "print copyright and version info"},
  {"-h",       argFlag,     &printHelp,     0,
   "print usage information"},
  {"-help",    argFlag,     &printHelp,     0,
   "print usage information"},
  {"--help",   argFlag,     &printHelp,     0,
   "print usage information"},
  {"-?",       argFlag,     &printHelp,     0,
   "print usage information"},
  {NULL}
};

int main(int argc, char *argv[]) {
  PDFDoc *doc;
  GooString *fileName;
  GooString *textFileName;
  GooString *ownerPW, *userPW;
  TextOutputDev *textOut;
  FILE *f;
  UnicodeMap *uMap;
  Object info;
  GBool ok;
  char *p;
  int exitCode;

  exitCode = 99;

  // parse args
  ok = parseArgs(argDesc, &argc, argv);
  if (!ok || argc < 2 || argc > 3 || printVersion || printHelp) {
    fprintf(stderr, "pdftotext version %s\n", xpdfVersion);
    fprintf(stderr, "%s\n", xpdfCopyright);
    if (!printVersion) {
      printUsage("pdftotext", "<PDF-file> [<text-file>]", argDesc);
    }
    goto err0;
  }
  fileName = new GooString(argv[1]);

  // read config file
  globalParams = new GlobalParams(cfgFileName);
  if (textEncName[0]) {
    globalParams->setTextEncoding(textEncName);
  }
  if (textEOL[0]) {
    if (!globalParams->setTextEOL(textEOL)) {
      fprintf(stderr, "Bad '-eol' value on command line\n");
    }
  }
  if (noPageBreaks) {
    globalParams->setTextPageBreaks(gFalse);
  }
  if (quiet) {
    globalParams->setErrQuiet(quiet);
  }

  // get mapping to output encoding
  if (!(uMap = globalParams->getTextEncoding())) {
    error(-1, "Couldn't get text encoding");
    delete fileName;
    goto err1;
  }

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
    goto err2;
  }

#ifdef ENFORCE_PERMISSIONS
  // check for copy permission
  if (!doc->okToCopy()) {
    error(-1, "Copying of text from this document is not allowed.");
    exitCode = 3;
    goto err2;
  }
#endif

  // construct text file name
  if (argc == 3) {
    textFileName = new GooString(argv[2]);
  } else {
    p = fileName->getCString() + fileName->getLength() - 4;
    if (!strcmp(p, ".pdf") || !strcmp(p, ".PDF")) {
      textFileName = new GooString(fileName->getCString(),
				 fileName->getLength() - 4);
    } else {
      textFileName = fileName->copy();
    }
    textFileName->append(htmlMeta ? ".html" : ".txt");
  }

  // get page range
  if (firstPage < 1) {
    firstPage = 1;
  }
  if (lastPage < 1 || lastPage > doc->getNumPages()) {
    lastPage = doc->getNumPages();
  }

  // write HTML header
  if (htmlMeta) {
    if (!textFileName->cmp("-")) {
      f = stdout;
    } else {
      if (!(f = fopen(textFileName->getCString(), "wb"))) {
	error(-1, "Couldn't open text file '%s'", textFileName->getCString());
	exitCode = 2;
	goto err3;
      }
    }
    fputs("<html>\n", f);
    fputs("<head>\n", f);
    doc->getDocInfo(&info);
    if (info.isDict()) {
      printInfoString(f, info.getDict(), "Title", "<title>", "</title>\n",
		      uMap);
      printInfoString(f, info.getDict(), "Subject",
		      "<meta name=\"Subject\" content=\"", "\">\n", uMap);
      printInfoString(f, info.getDict(), "Keywords",
		      "<meta name=\"Keywords\" content=\"", "\">\n", uMap);
      printInfoString(f, info.getDict(), "Author",
		      "<meta name=\"Author\" content=\"", "\">\n", uMap);
      printInfoString(f, info.getDict(), "Creator",
		      "<meta name=\"Creator\" content=\"", "\">\n", uMap);
      printInfoString(f, info.getDict(), "Producer",
		      "<meta name=\"Producer\" content=\"", "\">\n", uMap);
      printInfoDate(f, info.getDict(), "CreationDate",
		    "<meta name=\"CreationDate\" content=\"\">\n");
      printInfoDate(f, info.getDict(), "LastModifiedDate",
		    "<meta name=\"ModDate\" content=\"\">\n");
    }
    info.free();
    fputs("</head>\n", f);
    fputs("<body>\n", f);
    fputs("<pre>\n", f);
    if (f != stdout) {
      fclose(f);
    }
  }

  // write text file
  textOut = new TextOutputDev(textFileName->getCString(),
			      physLayout, rawOrder, htmlMeta);
  if (textOut->isOk()) {
      doc->displayPages(textOut, firstPage, lastPage, 72, 72, 0,
			gTrue, gFalse, gFalse);
  } else {
    delete textOut;
    exitCode = 2;
    goto err3;
  }
  delete textOut;

  // write end of HTML file
  if (htmlMeta) {
    if (!textFileName->cmp("-")) {
      f = stdout;
    } else {
      if (!(f = fopen(textFileName->getCString(), "ab"))) {
	error(-1, "Couldn't open text file '%s'", textFileName->getCString());
	exitCode = 2;
	goto err3;
      }
    }
    fputs("</pre>\n", f);
    fputs("</body>\n", f);
    fputs("</html>\n", f);
    if (f != stdout) {
      fclose(f);
    }
  }

  exitCode = 0;

  // clean up
 err3:
  delete textFileName;
 err2:
  delete doc;
  uMap->decRefCnt();
 err1:
  delete globalParams;
 err0:

  // check for memory leaks
  Object::memCheck(stderr);
  gMemReport(stderr);

  return exitCode;
}

static void printInfoString(FILE *f, Dict *infoDict, char *key,
			    char *text1, char *text2, UnicodeMap *uMap) {
  Object obj;
  GooString *s1;
  GBool isUnicode;
  Unicode u;
  char buf[8];
  int i, n;

  if (infoDict->lookup(key, &obj)->isString()) {
    fputs(text1, f);
    s1 = obj.getString();
    if ((s1->getChar(0) & 0xff) == 0xfe &&
	(s1->getChar(1) & 0xff) == 0xff) {
      isUnicode = gTrue;
      i = 2;
    } else {
      isUnicode = gFalse;
      i = 0;
    }
    while (i < obj.getString()->getLength()) {
      if (isUnicode) {
	u = ((s1->getChar(i) & 0xff) << 8) |
	    (s1->getChar(i+1) & 0xff);
	i += 2;
      } else {
	u = s1->getChar(i) & 0xff;
	++i;
      }
      n = uMap->mapUnicode(u, buf, sizeof(buf));
      fwrite(buf, 1, n, f);
    }
    fputs(text2, f);
  }
  obj.free();
}

static void printInfoDate(FILE *f, Dict *infoDict, char *key, char *fmt) {
  Object obj;
  char *s;

  if (infoDict->lookup(key, &obj)->isString()) {
    s = obj.getString()->getCString();
    if (s[0] == 'D' && s[1] == ':') {
      s += 2;
    }
    fprintf(f, fmt, s);
  }
  obj.free();
}
