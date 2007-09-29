//========================================================================
//
// pdftoppm.cc
//
// Copyright 2003 Glyph & Cog, LLC
//
//========================================================================

#include "config.h"
#include <poppler-config.h>
#include <stdio.h>
#include "parseargs.h"
#include "goo/gmem.h"
#include "goo/GooString.h"
#include "GlobalParams.h"
#include "Object.h"
#include "PDFDoc.h"
#include "splash/SplashBitmap.h"
#include "splash/Splash.h"
#include "SplashOutputDev.h"

static int firstPage = 1;
static int lastPage = 0;
static int resolution = 150;
static GBool mono = gFalse;
static GBool gray = gFalse;
static char enableT1libStr[16] = "";
static char enableFreeTypeStr[16] = "";
static char antialiasStr[16] = "";
static char ownerPassword[33] = "";
static char userPassword[33] = "";
static GBool quiet = gFalse;
static char cfgFileName[256] = "";
static GBool printVersion = gFalse;
static GBool printHelp = gFalse;

static ArgDesc argDesc[] = {
  {"-f",      argInt,      &firstPage,     0,
   "first page to print"},
  {"-l",      argInt,      &lastPage,      0,
   "last page to print"},
  {"-r",      argInt,      &resolution,    0,
   "resolution, in DPI (default is 150)"},
  {"-mono",   argFlag,     &mono,          0,
   "generate a monochrome PBM file"},
  {"-gray",   argFlag,     &gray,          0,
   "generate a grayscale PGM file"},
#if HAVE_T1LIB_H
  {"-t1lib",      argString,      enableT1libStr, sizeof(enableT1libStr),
   "enable t1lib font rasterizer: yes, no"},
#endif
#if HAVE_FREETYPE_FREETYPE_H | HAVE_FREETYPE_H
  {"-freetype",   argString,      enableFreeTypeStr, sizeof(enableFreeTypeStr),
   "enable FreeType font rasterizer: yes, no"},
#endif
  {"-aa",         argString,      antialiasStr,   sizeof(antialiasStr),
   "enable font anti-aliasing: yes, no"},
  {"-opw",    argString,   ownerPassword,  sizeof(ownerPassword),
   "owner password (for encrypted files)"},
  {"-upw",    argString,   userPassword,   sizeof(userPassword),
   "user password (for encrypted files)"},
  {"-q",      argFlag,     &quiet,         0,
   "don't print any messages or errors"},
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

int main(int argc, char *argv[]) {
  PDFDoc *doc;
  GooString *fileName;
  char *ppmRoot;
  char ppmFile[512];
  GooString *ownerPW, *userPW;
  SplashColor paperColor;
  SplashOutputDev *splashOut;
  GBool ok;
  int exitCode;
  int pg;

  exitCode = 99;

  // parse args
  ok = parseArgs(argDesc, &argc, argv);
  if (mono && gray) {
    ok = gFalse;
  }
  if (!ok || argc != 3 || printVersion || printHelp) {
    fprintf(stderr, "pdftoppm version %s\n", xpdfVersion);
    fprintf(stderr, "%s\n", xpdfCopyright);
    if (!printVersion) {
      printUsage("pdftoppm", "<PDF-file> <PPM-root>", argDesc);
    }
    goto err0;
  }
  fileName = new GooString(argv[1]);
  ppmRoot = argv[2];

  // read config file
  globalParams = new GlobalParams(cfgFileName);
  if (enableT1libStr[0]) {
    if (!globalParams->setEnableT1lib(enableT1libStr)) {
      fprintf(stderr, "Bad '-t1lib' value on command line\n");
    }
  }
  if (enableFreeTypeStr[0]) {
    if (!globalParams->setEnableFreeType(enableFreeTypeStr)) {
      fprintf(stderr, "Bad '-freetype' value on command line\n");
    }
  }
  if (antialiasStr[0]) {
    if (!globalParams->setAntialias(antialiasStr)) {
      fprintf(stderr, "Bad '-aa' value on command line\n");
    }
  }
  if (quiet) {
    globalParams->setErrQuiet(quiet);
  }

  // open PDF file
  if (ownerPassword[0]) {
    ownerPW = new GooString(ownerPassword);
  } else {
    ownerPW = NULL;
  }
  if (userPassword[0]) {
    userPW = new GooString(userPassword);
  } else {
    userPW = NULL;
  }
  doc = new PDFDoc(fileName, ownerPW, userPW);
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
  if (firstPage < 1)
    firstPage = 1;
  if (lastPage < 1 || lastPage > doc->getNumPages())
    lastPage = doc->getNumPages();

  // write PPM files
  paperColor[0] = 255;
  paperColor[1] = 255;
  paperColor[2] = 255;
  splashOut = new SplashOutputDev(mono ? splashModeMono1 :
				    gray ? splashModeMono8 :
				             splashModeRGB8, 4,
				  gFalse, paperColor);
  splashOut->startDoc(doc->getXRef());
  for (pg = firstPage; pg <= lastPage; ++pg) {
    doc->displayPage(splashOut, pg, resolution, resolution, 0, gTrue, gFalse, gFalse);
    sprintf(ppmFile, "%.*s-%06d.%s",
	    (int)sizeof(ppmFile) - 32, ppmRoot, pg,
	    mono ? "pbm" : gray ? "pgm" : "ppm");
    splashOut->getBitmap()->writePNMFile(ppmFile);
  }
  delete splashOut;

  exitCode = 0;

  // clean up
 err1:
  delete doc;
  delete globalParams;
 err0:

  // check for memory leaks
  Object::memCheck(stderr);
  gMemReport(stderr);

  return exitCode;
}
