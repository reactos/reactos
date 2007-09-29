//========================================================================
//
// GlobalParams.cc
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#ifdef ENABLE_PLUGINS
#  ifndef WIN32
#    include <dlfcn.h>
#  endif
#endif
#ifdef WIN32
#  include <shlobj.h>
#endif
#ifdef HAVE_FONTCONFIG_H
#  include <fontconfig/fontconfig.h>
#endif
#include "goo/gmem.h"
#include "goo/GooString.h"
#include "goo/GooList.h"
#include "goo/GooHash.h"
#include "goo/gfile.h"
#include "Error.h"
#include "NameToCharCode.h"
#include "CharCodeToUnicode.h"
#include "UnicodeMap.h"
#include "CMap.h"
#include "BuiltinFontTables.h"
#include "FontEncodingTables.h"
#ifdef ENABLE_PLUGINS
#  include "XpdfPluginAPI.h"
#endif
#include "GlobalParams.h"
#include "GfxFont.h"

#if MULTITHREADED
#  define lockGlobalParams            gLockMutex(&mutex)
#  define lockUnicodeMapCache         gLockMutex(&unicodeMapCacheMutex)
#  define lockCMapCache               gLockMutex(&cMapCacheMutex)
#  define unlockGlobalParams          gUnlockMutex(&mutex)
#  define unlockUnicodeMapCache       gUnlockMutex(&unicodeMapCacheMutex)
#  define unlockCMapCache             gUnlockMutex(&cMapCacheMutex)
#else
#  define lockGlobalParams
#  define lockUnicodeMapCache
#  define lockCMapCache
#  define unlockGlobalParams
#  define unlockUnicodeMapCache
#  define unlockCMapCache
#endif

#ifndef FC_WEIGHT_BOOK
#define FC_WEIGHT_BOOK 75
#endif

#include "NameToUnicodeTable.h"
#include "UnicodeMapTables.h"
#include "UTF8.h"

#ifdef ENABLE_PLUGINS
#  ifdef WIN32
extern XpdfPluginVecTable xpdfPluginVecTable;
#  endif
#endif

//------------------------------------------------------------------------

#define cidToUnicodeCacheSize     4
#define unicodeToUnicodeCacheSize 4

//------------------------------------------------------------------------

GlobalParams *globalParams = NULL;

//------------------------------------------------------------------------
// DisplayFontParam
//------------------------------------------------------------------------

DisplayFontParam::DisplayFontParam(GooString *nameA,
				   DisplayFontParamKind kindA) {
  name = nameA;
  kind = kindA;
  switch (kind) {
  case displayFontT1:
    t1.fileName = NULL;
    break;
  case displayFontTT:
    tt.fileName = NULL;
    break;
  }
}

DisplayFontParam::~DisplayFontParam() {
  delete name;
  switch (kind) {
  case displayFontT1:
    if (t1.fileName) {
      delete t1.fileName;
    }
    break;
  case displayFontTT:
    if (tt.fileName) {
      delete tt.fileName;
    }
    break;
  }
}

//------------------------------------------------------------------------
// PSFontParam
//------------------------------------------------------------------------

PSFontParam::PSFontParam(GooString *pdfFontNameA, int wModeA,
			 GooString *psFontNameA, GooString *encodingA) {
  pdfFontName = pdfFontNameA;
  wMode = wModeA;
  psFontName = psFontNameA;
  encoding = encodingA;
}

PSFontParam::~PSFontParam() {
  delete pdfFontName;
  delete psFontName;
  if (encoding) {
    delete encoding;
  }
}

#ifdef ENABLE_PLUGINS
//------------------------------------------------------------------------
// Plugin
//------------------------------------------------------------------------

class Plugin {
public:

  static Plugin *load(char *type, char *name);
  ~Plugin();

private:

#ifdef WIN32
  Plugin(HMODULE libA);
  HMODULE lib;
#else
  Plugin(void *dlA);
  void *dl;
#endif
};

Plugin *Plugin::load(char *type, char *name) {
  GooString *path;
  Plugin *plugin;
  XpdfPluginVecTable *vt;
  XpdfBool (*xpdfInitPlugin)(void);
#ifdef WIN32
  HMODULE libA;
#else
  void *dlA;
#endif

  path = globalParams->getBaseDir();
  appendToPath(path, "plugins");
  appendToPath(path, type);
  appendToPath(path, name);

#ifdef WIN32
  path->append(".dll");
  if (!(libA = LoadLibrary(path->getCString()))) {
    error(-1, "Failed to load plugin '%s'",
	  path->getCString());
    goto err1;
  }
  if (!(vt = (XpdfPluginVecTable *)
	         GetProcAddress(libA, "xpdfPluginVecTable"))) {
    error(-1, "Failed to find xpdfPluginVecTable in plugin '%s'",
	  path->getCString());
    goto err2;
  }
#else
  //~ need to deal with other extensions here
  path->append(".so");
  if (!(dlA = dlopen(path->getCString(), RTLD_NOW))) {
    error(-1, "Failed to load plugin '%s': %s",
	  path->getCString(), dlerror());
    goto err1;
  }
  if (!(vt = (XpdfPluginVecTable *)dlsym(dlA, "xpdfPluginVecTable"))) {
    error(-1, "Failed to find xpdfPluginVecTable in plugin '%s'",
	  path->getCString());
    goto err2;
  }
#endif

  if (vt->version != xpdfPluginVecTable.version) {
    error(-1, "Plugin '%s' is wrong version", path->getCString());
    goto err2;
  }
  memcpy(vt, &xpdfPluginVecTable, sizeof(xpdfPluginVecTable));

#ifdef WIN32
  if (!(xpdfInitPlugin = (XpdfBool (*)(void))
	                     GetProcAddress(libA, "xpdfInitPlugin"))) {
    error(-1, "Failed to find xpdfInitPlugin in plugin '%s'",
	  path->getCString());
    goto err2;
  }
#else
  if (!(xpdfInitPlugin = (XpdfBool (*)(void))dlsym(dlA, "xpdfInitPlugin"))) {
    error(-1, "Failed to find xpdfInitPlugin in plugin '%s'",
	  path->getCString());
    goto err2;
  }
#endif

  if (!(*xpdfInitPlugin)()) {
    error(-1, "Initialization of plugin '%s' failed",
	  path->getCString());
    goto err2;
  }

#ifdef WIN32
  plugin = new Plugin(libA);
#else
  plugin = new Plugin(dlA);
#endif

  delete path;
  return plugin;

 err2:
#ifdef WIN32
  FreeLibrary(libA);
#else
  dlclose(dlA);
#endif
 err1:
  delete path;
  return NULL;
}

#ifdef WIN32
Plugin::Plugin(HMODULE libA) {
  lib = libA;
}
#else
Plugin::Plugin(void *dlA) {
  dl = dlA;
}
#endif

Plugin::~Plugin() {
  void (*xpdfFreePlugin)(void);

#ifdef WIN32
  if ((xpdfFreePlugin = (void (*)(void))
                            GetProcAddress(lib, "xpdfFreePlugin"))) {
    (*xpdfFreePlugin)();
  }
  FreeLibrary(lib);
#else
  if ((xpdfFreePlugin = (void (*)(void))dlsym(dl, "xpdfFreePlugin"))) {
    (*xpdfFreePlugin)();
  }
  dlclose(dl);
#endif
}

#endif // ENABLE_PLUGINS

//------------------------------------------------------------------------
// parsing
//------------------------------------------------------------------------

GlobalParams::GlobalParams(char *cfgFileName) {
  UnicodeMap *map;
  GooString *fileName;
  FILE *f;
  int i;

#ifdef HAVE_FONTCONFIG_H
  FcInit();
  FCcfg = FcConfigGetCurrent();
#endif

#if MULTITHREADED
  gInitMutex(&mutex);
  gInitMutex(&unicodeMapCacheMutex);
  gInitMutex(&cMapCacheMutex);
#endif

  initBuiltinFontTables();

  // scan the encoding in reverse because we want the lowest-numbered
  // index for each char name ('space' is encoded twice)
  macRomanReverseMap = new NameToCharCode();
  for (i = 255; i >= 0; --i) {
    if (macRomanEncoding[i]) {
      macRomanReverseMap->add(macRomanEncoding[i], (CharCode)i);
    }
  }

#ifdef WIN32
  // baseDir will be set by a call to setBaseDir
  baseDir = new GooString();
#else
  baseDir = appendToPath(getHomeDir(), ".xpdf");
#endif
  nameToUnicode = new NameToCharCode();
  cidToUnicodes = new GooHash(gTrue);
  unicodeToUnicodes = new GooHash(gTrue);
  residentUnicodeMaps = new GooHash();
  unicodeMaps = new GooHash(gTrue);
  cMapDirs = new GooHash(gTrue);
  toUnicodeDirs = new GooList();
  displayFonts = new GooHash();
  psPaperWidth = -1;
  psPaperHeight = -1;
  psImageableLLX = psImageableLLY = 0;
  psImageableURX = psPaperWidth;
  psImageableURY = psPaperHeight;
  psCrop = gTrue;
  psExpandSmaller = gFalse;
  psShrinkLarger = gTrue;
  psCenter = gTrue;
  psDuplex = gFalse;
  psLevel = psLevel2;
  psFile = NULL;
  psFonts = new GooHash();
  psNamedFonts16 = new GooList();
  psFonts16 = new GooList();
  psEmbedType1 = gTrue;
  psEmbedTrueType = gTrue;
  psEmbedCIDPostScript = gTrue;
  psEmbedCIDTrueType = gTrue;
  psOPI = gFalse;
  psASCIIHex = gFalse;
  textEncoding = new GooString("UTF-8");
#if defined(WIN32)
  textEOL = eolDOS;
#elif defined(MACOS)
  textEOL = eolMac;
#else
  textEOL = eolUnix;
#endif
  textPageBreaks = gTrue;
  textKeepTinyChars = gFalse;
  fontDirs = new GooList();
  initialZoom = new GooString("125");
  continuousView = gFalse;
  enableT1lib = gTrue;
  enableFreeType = gTrue;
  antialias = gTrue;
  urlCommand = NULL;
  movieCommand = NULL;
  mapNumericCharNames = gTrue;
  printCommands = gFalse;
  profileCommands = gFalse;
  errQuiet = gFalse;

  cidToUnicodeCache = new CharCodeToUnicodeCache(cidToUnicodeCacheSize);
  unicodeToUnicodeCache =
      new CharCodeToUnicodeCache(unicodeToUnicodeCacheSize);
  unicodeMapCache = new UnicodeMapCache();
  cMapCache = new CMapCache();

#ifdef ENABLE_PLUGINS
  plugins = new GooList();
  securityHandlers = new GooList();
#endif

  // set up the initial nameToUnicode table
  for (i = 0; nameToUnicodeTab[i].name; ++i) {
    nameToUnicode->add(nameToUnicodeTab[i].name, nameToUnicodeTab[i].u);
  }

  // set up the residentUnicodeMaps table
  map = new UnicodeMap("Latin1", gFalse,
		       latin1UnicodeMapRanges, latin1UnicodeMapLen);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("ASCII7", gFalse,
		       ascii7UnicodeMapRanges, ascii7UnicodeMapLen);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("Symbol", gFalse,
		       symbolUnicodeMapRanges, symbolUnicodeMapLen);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("ZapfDingbats", gFalse, zapfDingbatsUnicodeMapRanges,
		       zapfDingbatsUnicodeMapLen);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("UTF-8", gTrue, &mapUTF8);
  residentUnicodeMaps->add(map->getEncodingName(), map);
  map = new UnicodeMap("UCS-2", gTrue, &mapUCS2);
  residentUnicodeMaps->add(map->getEncodingName(), map);

  // look for a user config file, then a system-wide config file
  f = NULL;
  fileName = NULL;
  if (cfgFileName && cfgFileName[0]) {
    fileName = new GooString(cfgFileName);
    if (!(f = fopen(fileName->getCString(), "r"))) {
      delete fileName;
    }
  }
  if (!f) {
    fileName = appendToPath(getHomeDir(), xpdfUserConfigFile);
    if (!(f = fopen(fileName->getCString(), "r"))) {
      delete fileName;
    }
  }
  if (!f) {
#if defined(WIN32) && !defined(__CYGWIN32__)
    char buf[512];
    /* @note: TCHAR* cast */
    i = GetModuleFileName(NULL, (TCHAR*)buf, sizeof(buf));
    if (i <= 0 || i >= sizeof(buf)) {
      // error or path too long for buffer - just use the current dir
      buf[0] = '\0';
    }
    fileName = grabPath(buf);
    appendToPath(fileName, xpdfSysConfigFile);
#else
    fileName = new GooString(xpdfSysConfigFile);
#endif
    if (!(f = fopen(fileName->getCString(), "r"))) {
      delete fileName;
    }
  }
  if (f) {
    parseFile(fileName, f);
    delete fileName;
    fclose(f);
  }

  scanEncodingDirs();
}

void GlobalParams::parseFile(GooString *fileName, FILE *f) {
  int line;
  GooList *tokens;
  GooString *cmd, *incFile;
  char *p1, *p2;
  char buf[512];
  FILE *f2;

  line = 1;
  while (getLine(buf, sizeof(buf) - 1, f)) {

    // break the line into tokens
    tokens = new GooList();
    p1 = buf;
    while (*p1) {
      for (; *p1 && isspace(*p1); ++p1) ;
      if (!*p1) {
	break;
      }
      if (*p1 == '"' || *p1 == '\'') {
	for (p2 = p1 + 1; *p2 && *p2 != *p1; ++p2) ;
	++p1;
      } else {
	for (p2 = p1 + 1; *p2 && !isspace(*p2); ++p2) ;
      }
      tokens->append(new GooString(p1, p2 - p1));
      p1 = *p2 ? p2 + 1 : p2;
    }

    if (tokens->getLength() > 0 &&
	((GooString *)tokens->get(0))->getChar(0) != '#') {
      cmd = (GooString *)tokens->get(0);
      if (!cmd->cmp("include")) {
	if (tokens->getLength() == 2) {
	  incFile = (GooString *)tokens->get(1);
	  if ((f2 = fopen(incFile->getCString(), "r"))) {
	    parseFile(incFile, f2);
	    fclose(f2);
	  } else {
	    error(-1, "Couldn't find included config file: '%s' (%s:%d)",
		  incFile->getCString(), fileName->getCString(), line);
	  }
	} else {
	  error(-1, "Bad 'include' config file command (%s:%d)",
		fileName->getCString(), line);
	}
      } else if (!cmd->cmp("nameToUnicode")) {
	if (tokens->getLength() != 2)
	  error(-1, "Bad 'nameToUnicode' config file command (%s:%d)",
		fileName->getCString(), line);
	else
	  parseNameToUnicode((GooString *) tokens->get(1));
      } else if (!cmd->cmp("cidToUnicode")) {
	parseCIDToUnicode(tokens, fileName, line);
      } else if (!cmd->cmp("unicodeToUnicode")) {
	parseUnicodeToUnicode(tokens, fileName, line);
      } else if (!cmd->cmp("unicodeMap")) {
	parseUnicodeMap(tokens, fileName, line);
      } else if (!cmd->cmp("cMapDir")) {
	parseCMapDir(tokens, fileName, line);
      } else if (!cmd->cmp("toUnicodeDir")) {
	parseToUnicodeDir(tokens, fileName, line);
      } else if (!cmd->cmp("displayFontT1")) {
        // deprecated
      } else if (!cmd->cmp("displayFontTT")) {
        // deprecated
      } else if (!cmd->cmp("displayNamedCIDFontT1")) {
        // deprecated
      } else if (!cmd->cmp("displayCIDFontT1")) {
        // deprecated
      } else if (!cmd->cmp("displayNamedCIDFontTT")) {
        // deprecated
      } else if (!cmd->cmp("displayCIDFontTT")) {
        // deprecated
      } else if (!cmd->cmp("psFile")) {
	parsePSFile(tokens, fileName, line);
      } else if (!cmd->cmp("psFont")) {
	parsePSFont(tokens, fileName, line);
      } else if (!cmd->cmp("psNamedFont16")) {
	parsePSFont16("psNamedFont16", psNamedFonts16,
		      tokens, fileName, line);
      } else if (!cmd->cmp("psFont16")) {
	parsePSFont16("psFont16", psFonts16, tokens, fileName, line);
      } else if (!cmd->cmp("psPaperSize")) {
	parsePSPaperSize(tokens, fileName, line);
      } else if (!cmd->cmp("psImageableArea")) {
	parsePSImageableArea(tokens, fileName, line);
      } else if (!cmd->cmp("psCrop")) {
	parseYesNo("psCrop", &psCrop, tokens, fileName, line);
      } else if (!cmd->cmp("psExpandSmaller")) {
	parseYesNo("psExpandSmaller", &psExpandSmaller,
		   tokens, fileName, line);
      } else if (!cmd->cmp("psShrinkLarger")) {
	parseYesNo("psShrinkLarger", &psShrinkLarger, tokens, fileName, line);
      } else if (!cmd->cmp("psCenter")) {
	parseYesNo("psCenter", &psCenter, tokens, fileName, line);
      } else if (!cmd->cmp("psDuplex")) {
	parseYesNo("psDuplex", &psDuplex, tokens, fileName, line);
      } else if (!cmd->cmp("psLevel")) {
	parsePSLevel(tokens, fileName, line);
      } else if (!cmd->cmp("psEmbedType1Fonts")) {
	parseYesNo("psEmbedType1", &psEmbedType1, tokens, fileName, line);
      } else if (!cmd->cmp("psEmbedTrueTypeFonts")) {
	parseYesNo("psEmbedTrueType", &psEmbedTrueType,
		   tokens, fileName, line);
      } else if (!cmd->cmp("psEmbedCIDPostScriptFonts")) {
	parseYesNo("psEmbedCIDPostScript", &psEmbedCIDPostScript,
		   tokens, fileName, line);
      } else if (!cmd->cmp("psEmbedCIDTrueTypeFonts")) {
	parseYesNo("psEmbedCIDTrueType", &psEmbedCIDTrueType,
		   tokens, fileName, line);
      } else if (!cmd->cmp("psOPI")) {
	parseYesNo("psOPI", &psOPI, tokens, fileName, line);
      } else if (!cmd->cmp("psASCIIHex")) {
	parseYesNo("psASCIIHex", &psASCIIHex, tokens, fileName, line);
      } else if (!cmd->cmp("textEncoding")) {
	parseTextEncoding(tokens, fileName, line);
      } else if (!cmd->cmp("textEOL")) {
	parseTextEOL(tokens, fileName, line);
      } else if (!cmd->cmp("textPageBreaks")) {
	parseYesNo("textPageBreaks", &textPageBreaks,
		   tokens, fileName, line);
      } else if (!cmd->cmp("textKeepTinyChars")) {
	parseYesNo("textKeepTinyChars", &textKeepTinyChars,
		   tokens, fileName, line);
      } else if (!cmd->cmp("fontDir")) {
	parseFontDir(tokens, fileName, line);
      } else if (!cmd->cmp("initialZoom")) {
	parseInitialZoom(tokens, fileName, line);
      } else if (!cmd->cmp("continuousView")) {
	parseYesNo("continuousView", &continuousView, tokens, fileName, line);
      } else if (!cmd->cmp("enableT1lib")) {
	parseYesNo("enableT1lib", &enableT1lib, tokens, fileName, line);
      } else if (!cmd->cmp("enableFreeType")) {
	parseYesNo("enableFreeType", &enableFreeType, tokens, fileName, line);
      } else if (!cmd->cmp("antialias")) {
	parseYesNo("antialias", &antialias, tokens, fileName, line);
      } else if (!cmd->cmp("urlCommand")) {
	parseCommand("urlCommand", &urlCommand, tokens, fileName, line);
      } else if (!cmd->cmp("movieCommand")) {
	parseCommand("movieCommand", &movieCommand, tokens, fileName, line);
      } else if (!cmd->cmp("mapNumericCharNames")) {
	parseYesNo("mapNumericCharNames", &mapNumericCharNames,
		   tokens, fileName, line);
      } else if (!cmd->cmp("printCommands")) {
	parseYesNo("printCommands", &printCommands, tokens, fileName, line);
      } else if (!cmd->cmp("errQuiet")) {
	parseYesNo("errQuiet", &errQuiet, tokens, fileName, line);
      } else {
	error(-1, "Unknown config file command '%s' (%s:%d)",
	      cmd->getCString(), fileName->getCString(), line);
	if (!cmd->cmp("displayFontX") ||
	    !cmd->cmp("displayNamedCIDFontX") ||
	    !cmd->cmp("displayCIDFontX")) {
	  error(-1, "-- Xpdf no longer supports X fonts");
	} else if (!cmd->cmp("t1libControl") || !cmd->cmp("freetypeControl")) {
	  error(-1, "-- The t1libControl and freetypeControl options have been replaced");
	  error(-1, "   by the enableT1lib, enableFreeType, and antialias options");
	} else if (!cmd->cmp("fontpath") || !cmd->cmp("fontmap")) {
	  error(-1, "-- the config file format has changed since Xpdf 0.9x");
	}
      }
    }

    deleteGooList(tokens, GooString);
    ++line;
  }
}

void GlobalParams::scanEncodingDirs() {
  GDir *dir;
  GDirEntry *entry;

  dir = new GDir(POPPLER_DATADIR "/nameToUnicode", gFalse);
  while (entry = dir->getNextEntry(), entry != NULL) {
    parseNameToUnicode(entry->getFullPath());
    delete entry;
  }
  delete dir;

  dir = new GDir(POPPLER_DATADIR "/cidToUnicode", gFalse);
  while (entry = dir->getNextEntry(), entry != NULL) {
    addCIDToUnicode(entry->getName(), entry->getFullPath());
    delete entry;
  }
  delete dir;

  dir = new GDir(POPPLER_DATADIR "/unicodeMap", gFalse);
  while (entry = dir->getNextEntry(), entry != NULL) {
    addUnicodeMap(entry->getName(), entry->getFullPath());
    delete entry;
  }
  delete dir;

  dir = new GDir(POPPLER_DATADIR "/cMap", gFalse);
  while (entry = dir->getNextEntry(), entry != NULL) {
    addCMapDir(entry->getName(), entry->getFullPath());
    toUnicodeDirs->append(entry->getFullPath()->copy());
    delete entry;
  }
  delete dir;
}

void GlobalParams::parseNameToUnicode(GooString *name) {
  char *tok1, *tok2;
  FILE *f;
  char buf[256];
  int line;
  Unicode u;

  if (!(f = fopen(name->getCString(), "r"))) {
    error(-1, "Couldn't open 'nameToUnicode' file '%s'",
	  name->getCString());
    return;
  }
  line = 1;
  while (getLine(buf, sizeof(buf), f)) {
    tok1 = strtok(buf, " \t\r\n");
    tok2 = strtok(NULL, " \t\r\n");
    if (tok1 && tok2) {
      sscanf(tok1, "%x", &u);
      nameToUnicode->add(tok2, u);
    } else {
      error(-1, "Bad line in 'nameToUnicode' file (%s:%d)",
	    name->getCString(), line);
    }
    ++line;
  }
  fclose(f);
}

void GlobalParams::addCIDToUnicode(GooString *collection,
				   GooString *fileName) {
  GooString *old;

  if ((old = (GooString *)cidToUnicodes->remove(collection))) {
    delete old;
  }
  cidToUnicodes->add(collection->copy(), fileName->copy());
}

void GlobalParams::parseCIDToUnicode(GooList *tokens, GooString *fileName,
				     int line) {
  if (tokens->getLength() != 3) {
    error(-1, "Bad 'cidToUnicode' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  addCIDToUnicode((GooString *)tokens->get(1), (GooString *)tokens->get(2));
}

void GlobalParams::parseUnicodeToUnicode(GooList *tokens, GooString *fileName,
					 int line) {
  GooString *font, *file, *old;

  if (tokens->getLength() != 3) {
    error(-1, "Bad 'unicodeToUnicode' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  font = (GooString *)tokens->get(1);
  file = (GooString *)tokens->get(2);
  if ((old = (GooString *)unicodeToUnicodes->remove(font))) {
    delete old;
  }
  unicodeToUnicodes->add(font->copy(), file->copy());
}

void GlobalParams::addUnicodeMap(GooString *encodingName, GooString *fileName)
{
  GooString *old;

  if ((old = (GooString *)unicodeMaps->remove(encodingName))) {
    delete old;
  }
  unicodeMaps->add(encodingName->copy(), fileName->copy());
}

void GlobalParams::parseUnicodeMap(GooList *tokens, GooString *fileName,
				   int line) {

  if (tokens->getLength() != 3) {
    error(-1, "Bad 'unicodeMap' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  addUnicodeMap((GooString *)tokens->get(1), (GooString *)tokens->get(2));
  }

void GlobalParams::addCMapDir(GooString *collection, GooString *dir) {
  GooList *list;

  if (!(list = (GooList *)cMapDirs->lookup(collection))) {
    list = new GooList();
    cMapDirs->add(collection->copy(), list);
  }
  list->append(dir->copy());
}

void GlobalParams::parseCMapDir(GooList *tokens, GooString *fileName, int line) {
  if (tokens->getLength() != 3) {
    error(-1, "Bad 'cMapDir' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  addCMapDir((GooString *)tokens->get(1), (GooString *)tokens->get(2));
}

void GlobalParams::parseToUnicodeDir(GooList *tokens, GooString *fileName,
				     int line) {
  if (tokens->getLength() != 2) {
    error(-1, "Bad 'toUnicodeDir' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  toUnicodeDirs->append(((GooString *)tokens->get(1))->copy());
}

void GlobalParams::parsePSPaperSize(GooList *tokens, GooString *fileName,
				    int line) {
  GooString *tok;

  if (tokens->getLength() == 2) {
    tok = (GooString *)tokens->get(1);
    if (!setPSPaperSize(tok->getCString())) {
      error(-1, "Bad 'psPaperSize' config file command (%s:%d)",
	    fileName->getCString(), line);
    }
  } else if (tokens->getLength() == 3) {
    tok = (GooString *)tokens->get(1);
    psPaperWidth = atoi(tok->getCString());
    tok = (GooString *)tokens->get(2);
    psPaperHeight = atoi(tok->getCString());
    psImageableLLX = psImageableLLY = 0;
    psImageableURX = psPaperWidth;
    psImageableURY = psPaperHeight;
  } else {
    error(-1, "Bad 'psPaperSize' config file command (%s:%d)",
	  fileName->getCString(), line);
  }
}

void GlobalParams::parsePSImageableArea(GooList *tokens, GooString *fileName,
					int line) {
  if (tokens->getLength() != 5) {
    error(-1, "Bad 'psImageableArea' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  psImageableLLX = atoi(((GooString *)tokens->get(1))->getCString());
  psImageableLLY = atoi(((GooString *)tokens->get(2))->getCString());
  psImageableURX = atoi(((GooString *)tokens->get(3))->getCString());
  psImageableURY = atoi(((GooString *)tokens->get(4))->getCString());
}

void GlobalParams::parsePSLevel(GooList *tokens, GooString *fileName, int line) {
  GooString *tok;

  if (tokens->getLength() != 2) {
    error(-1, "Bad 'psLevel' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(1);
  if (!tok->cmp("level1")) {
    psLevel = psLevel1;
  } else if (!tok->cmp("level1sep")) {
    psLevel = psLevel1Sep;
  } else if (!tok->cmp("level2")) {
    psLevel = psLevel2;
  } else if (!tok->cmp("level2sep")) {
    psLevel = psLevel2Sep;
  } else if (!tok->cmp("level3")) {
    psLevel = psLevel3;
  } else if (!tok->cmp("level3Sep")) {
    psLevel = psLevel3Sep;
  } else {
    error(-1, "Bad 'psLevel' config file command (%s:%d)",
	  fileName->getCString(), line);
  }
}

void GlobalParams::parsePSFile(GooList *tokens, GooString *fileName, int line) {
  if (tokens->getLength() != 2) {
    error(-1, "Bad 'psFile' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  if (psFile) {
    delete psFile;
  }
  psFile = ((GooString *)tokens->get(1))->copy();
}

void GlobalParams::parsePSFont(GooList *tokens, GooString *fileName, int line) {
  PSFontParam *param;

  if (tokens->getLength() != 3) {
    error(-1, "Bad 'psFont' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  param = new PSFontParam(((GooString *)tokens->get(1))->copy(), 0,
			  ((GooString *)tokens->get(2))->copy(), NULL);
  psFonts->add(param->pdfFontName, param);
}

void GlobalParams::parsePSFont16(char *cmdName, GooList *fontList,
				 GooList *tokens, GooString *fileName, int line) {
  PSFontParam *param;
  int wMode;
  GooString *tok;

  if (tokens->getLength() != 5) {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(2);
  if (!tok->cmp("H")) {
    wMode = 0;
  } else if (!tok->cmp("V")) {
    wMode = 1;
  } else {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  param = new PSFontParam(((GooString *)tokens->get(1))->copy(),
			  wMode,
			  ((GooString *)tokens->get(3))->copy(),
			  ((GooString *)tokens->get(4))->copy());
  fontList->append(param);
}

void GlobalParams::parseTextEncoding(GooList *tokens, GooString *fileName,
				     int line) {
  if (tokens->getLength() != 2) {
    error(-1, "Bad 'textEncoding' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  delete textEncoding;
  textEncoding = ((GooString *)tokens->get(1))->copy();
}

void GlobalParams::parseTextEOL(GooList *tokens, GooString *fileName, int line) {
  GooString *tok;

  if (tokens->getLength() != 2) {
    error(-1, "Bad 'textEOL' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(1);
  if (!tok->cmp("unix")) {
    textEOL = eolUnix;
  } else if (!tok->cmp("dos")) {
    textEOL = eolDOS;
  } else if (!tok->cmp("mac")) {
    textEOL = eolMac;
  } else {
    error(-1, "Bad 'textEOL' config file command (%s:%d)",
	  fileName->getCString(), line);
  }
}

void GlobalParams::parseFontDir(GooList *tokens, GooString *fileName, int line) {
  if (tokens->getLength() != 2) {
    error(-1, "Bad 'fontDir' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  fontDirs->append(((GooString *)tokens->get(1))->copy());
}

void GlobalParams::parseInitialZoom(GooList *tokens,
				    GooString *fileName, int line) {
  if (tokens->getLength() != 2) {
    error(-1, "Bad 'initialZoom' config file command (%s:%d)",
	  fileName->getCString(), line);
    return;
  }
  delete initialZoom;
  initialZoom = ((GooString *)tokens->get(1))->copy();
}

void GlobalParams::parseCommand(char *cmdName, GooString **val,
				GooList *tokens, GooString *fileName, int line) {
  if (tokens->getLength() != 2) {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  if (*val) {
    delete *val;
  }
  *val = ((GooString *)tokens->get(1))->copy();
}

void GlobalParams::parseYesNo(char *cmdName, GBool *flag,
			      GooList *tokens, GooString *fileName, int line) {
  GooString *tok;

  if (tokens->getLength() != 2) {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
    return;
  }
  tok = (GooString *)tokens->get(1);
  if (!parseYesNo2(tok->getCString(), flag)) {
    error(-1, "Bad '%s' config file command (%s:%d)",
	  cmdName, fileName->getCString(), line);
  }
}

GBool GlobalParams::parseYesNo2(char *token, GBool *flag) {
  if (!strcmp(token, "yes")) {
    *flag = gTrue;
  } else if (!strcmp(token, "no")) {
    *flag = gFalse;
  } else {
    return gFalse;
  }
  return gTrue;
}

GlobalParams::~GlobalParams() {
  GooHashIter *iter;
  GooString *key;
  GooList *list;

  freeBuiltinFontTables();

  delete macRomanReverseMap;

  delete baseDir;
  delete nameToUnicode;
  deleteGooHash(cidToUnicodes, GooString);
  deleteGooHash(unicodeToUnicodes, GooString);
  deleteGooHash(residentUnicodeMaps, UnicodeMap);
  deleteGooHash(unicodeMaps, GooString);
  deleteGooList(toUnicodeDirs, GooString);
  deleteGooHash(displayFonts, DisplayFontParam);
  if (psFile) {
    delete psFile;
  }
  deleteGooHash(psFonts, PSFontParam);
  deleteGooList(psNamedFonts16, PSFontParam);
  deleteGooList(psFonts16, PSFontParam);
  delete textEncoding;
  deleteGooList(fontDirs, GooString);
  delete initialZoom;
  if (urlCommand) {
    delete urlCommand;
  }
  if (movieCommand) {
    delete movieCommand;
  }

  cMapDirs->startIter(&iter);
  while (cMapDirs->getNext(&iter, &key, (void **)&list)) {
    deleteGooList(list, GooString);
  }
  delete cMapDirs;

  delete cidToUnicodeCache;
  delete unicodeToUnicodeCache;
  delete unicodeMapCache;
  delete cMapCache;

#ifdef ENABLE_PLUGINS
  delete securityHandlers;
  deleteGooList(plugins, Plugin);
#endif

#if MULTITHREADED
  gDestroyMutex(&mutex);
  gDestroyMutex(&unicodeMapCacheMutex);
  gDestroyMutex(&cMapCacheMutex);
#endif
}

//------------------------------------------------------------------------

void GlobalParams::setBaseDir(char *dir) {
  delete baseDir;
  baseDir = new GooString(dir);
}

//------------------------------------------------------------------------
// accessors
//------------------------------------------------------------------------

CharCode GlobalParams::getMacRomanCharCode(char *charName) {
  // no need to lock - macRomanReverseMap is constant
  return macRomanReverseMap->lookup(charName);
}

GooString *GlobalParams::getBaseDir() {
  GooString *s;

  lockGlobalParams;
  s = baseDir->copy();
  unlockGlobalParams;
  return s;
}

Unicode GlobalParams::mapNameToUnicode(char *charName) {
  // no need to lock - nameToUnicode is constant
  return nameToUnicode->lookup(charName);
}

UnicodeMap *GlobalParams::getResidentUnicodeMap(GooString *encodingName) {
  UnicodeMap *map;

  lockGlobalParams;
  map = (UnicodeMap *)residentUnicodeMaps->lookup(encodingName);
  unlockGlobalParams;
  if (map) {
    map->incRefCnt();
  }
  return map;
}

FILE *GlobalParams::getUnicodeMapFile(GooString *encodingName) {
  GooString *fileName;
  FILE *f;

  lockGlobalParams;
  if ((fileName = (GooString *)unicodeMaps->lookup(encodingName))) {
    f = fopen(fileName->getCString(), "r");
  } else {
    f = NULL;
  }
  unlockGlobalParams;
  return f;
}

FILE *GlobalParams::findCMapFile(GooString *collection, GooString *cMapName) {
  GooList *list;
  GooString *dir;
  GooString *fileName;
  FILE *f;
  int i;

  lockGlobalParams;
  if (!(list = (GooList *)cMapDirs->lookup(collection))) {
    unlockGlobalParams;
    return NULL;
  }
  for (i = 0; i < list->getLength(); ++i) {
    dir = (GooString *)list->get(i);
    fileName = appendToPath(dir->copy(), cMapName->getCString());
    f = fopen(fileName->getCString(), "r");
    delete fileName;
    if (f) {
      unlockGlobalParams;
      return f;
    }
  }
  unlockGlobalParams;
  return NULL;
}

FILE *GlobalParams::findToUnicodeFile(GooString *name) {
  GooString *dir, *fileName;
  FILE *f;
  int i;

  lockGlobalParams;
  for (i = 0; i < toUnicodeDirs->getLength(); ++i) {
    dir = (GooString *)toUnicodeDirs->get(i);
    fileName = appendToPath(dir->copy(), name->getCString());
    f = fopen(fileName->getCString(), "r");
    delete fileName;
    if (f) {
      unlockGlobalParams;
      return f;
    }
  }
  unlockGlobalParams;
  return NULL;
}

GBool findModifier(const char *name, const char *modifier, const char **start)
{
  const char *match;

  if (name == NULL)
    return gFalse;

  match = strstr(name, modifier);
  if (match) {
    if (*start == NULL || match < *start)
      *start = match;
    return gTrue;
  }
  else {
    return gFalse;
  }
}

#ifdef HAVE_FONTCONFIG_H
static FcPattern *buildFcPattern(GfxFont *font)
{
  int weight = FC_WEIGHT_NORMAL,
      slant = FC_SLANT_ROMAN,
      width = FC_WIDTH_NORMAL,
      spacing = FC_PROPORTIONAL;
  bool deleteFamily = false;
  char *family, *name, *lang, *modifiers;
  const char *start;
  FcPattern *p;

  // this is all heuristics will be overwritten if font had proper info
  name = font->getName()->getCString();
  
  modifiers = strchr (name, ',');
  if (modifiers == NULL)
    modifiers = strchr (name, '-');
  
  // remove the - from the names, for some reason, Fontconfig does not
  // understand "MS-Mincho" but does with "MS Mincho"
  int len = strlen(name);
  for (int i = 0; i < len; i++)
    name[i] = (name[i] == '-' ? ' ' : name[i]);

  start = NULL;
  findModifier(modifiers, "Regular", &start);
  findModifier(modifiers, "Roman", &start);
  
  if (findModifier(modifiers, "Oblique", &start))
    slant = FC_SLANT_OBLIQUE;
  if (findModifier(modifiers, "Italic", &start))
    slant = FC_SLANT_ITALIC;
  if (findModifier(modifiers, "Bold", &start))
    weight = FC_WEIGHT_BOLD;
  if (findModifier(modifiers, "Light", &start))
    weight = FC_WEIGHT_LIGHT;
  if (findModifier(modifiers, "Condensed", &start))
    width = FC_WIDTH_CONDENSED;
  
  if (start) {
    // There have been "modifiers" in the name, crop them to obtain
    // the family name
    family = new char[len+1];
    strcpy(family, name);
    int pos = (modifiers - name);
    family[pos] = '\0';
    deleteFamily = true;
  }
  else {
    family = name;
  }
  
  // use font flags
  if (font->isFixedWidth())
    spacing = FC_MONO;
  if (font->isBold())
    weight = FC_WEIGHT_BOLD;
  if (font->isItalic())
    slant = FC_SLANT_ITALIC;
  
  // if the FontDescriptor specified a family name use it
  if (font->getFamily()) {
    if (deleteFamily) {
      delete[] family;
      deleteFamily = false;
    }
    family = font->getFamily()->getCString();
  }
  
  // if the FontDescriptor specified a weight use it
  switch (font -> getWeight())
  {
    case GfxFont::W100: weight = FC_WEIGHT_EXTRALIGHT; break; 
    case GfxFont::W200: weight = FC_WEIGHT_LIGHT; break; 
    case GfxFont::W300: weight = FC_WEIGHT_BOOK; break; 
    case GfxFont::W400: weight = FC_WEIGHT_NORMAL; break; 
    case GfxFont::W500: weight = FC_WEIGHT_MEDIUM; break; 
    case GfxFont::W600: weight = FC_WEIGHT_DEMIBOLD; break; 
    case GfxFont::W700: weight = FC_WEIGHT_BOLD; break; 
    case GfxFont::W800: weight = FC_WEIGHT_EXTRABOLD; break; 
    case GfxFont::W900: weight = FC_WEIGHT_BLACK; break; 
    default: break; 
  }
  
  // if the FontDescriptor specified a width use it
  switch (font -> getStretch())
  {
    case GfxFont::UltraCondensed: width = FC_WIDTH_ULTRACONDENSED; break; 
    case GfxFont::ExtraCondensed: width = FC_WIDTH_EXTRACONDENSED; break; 
    case GfxFont::Condensed: width = FC_WIDTH_CONDENSED; break; 
    case GfxFont::SemiCondensed: width = FC_WIDTH_SEMICONDENSED; break; 
    case GfxFont::Normal: width = FC_WIDTH_NORMAL; break; 
    case GfxFont::SemiExpanded: width = FC_WIDTH_SEMIEXPANDED; break; 
    case GfxFont::Expanded: width = FC_WIDTH_EXPANDED; break; 
    case GfxFont::ExtraExpanded: width = FC_WIDTH_EXTRAEXPANDED; break; 
    case GfxFont::UltraExpanded: width = FC_WIDTH_ULTRAEXPANDED; break; 
    default: break; 
  }
  
  // find the language we want the font to support
  if (font->isCIDFont())
  {
    GooString *collection = ((GfxCIDFont *)font)->getCollection();
    if (collection)
    {
      if (strcmp(collection->getCString(), "Adobe-GB1") == 0)
        lang = "zh-cn"; // Simplified Chinese
      else if (strcmp(collection->getCString(), "Adobe-CNS1") == 0)
        lang = "zh-tw"; // Traditional Chinese
      else if (strcmp(collection->getCString(), "Adobe-Japan1") == 0)
        lang = "ja"; // Japanese
      else if (strcmp(collection->getCString(), "Adobe-Japan2") == 0)
        lang = "ja"; // Japanese
      else if (strcmp(collection->getCString(), "Adobe-Korea1") == 0)
        lang = "ko"; // Korean
      else if (strcmp(collection->getCString(), "Adobe-UCS") == 0)
        lang = "xx";
      else if (strcmp(collection->getCString(), "Adobe-Identity") == 0)
        lang = "xx";
      else
      {
        error(-1, "Unknown CID font collection, please report to poppler bugzilla.");
        lang = "xx";
      }
    }
    else lang = "xx";
  }
  else lang = "xx";
  
  p = FcPatternBuild(NULL,
                    FC_FAMILY, FcTypeString, family,
                    FC_SLANT, FcTypeInteger, slant, 
                    FC_WEIGHT, FcTypeInteger, weight,
                    FC_WIDTH, FcTypeInteger, width, 
                    FC_SPACING, FcTypeInteger, spacing,
                    FC_LANG, FcTypeString, lang,
                    NULL);
  if (deleteFamily)
    delete[] family;
  return p;
}
#endif

/* if you can't or don't want to use Fontconfig, you need to implement
   this function for your platform. For Windows, it's in GlobalParamsWin.cc
*/
#ifdef HAVE_FONTCONFIG_H
DisplayFontParam *GlobalParams::getDisplayFont(GfxFont *font) {
  DisplayFontParam *dfp;
  FcPattern *p=0;

  GooString *fontName = font->getName();
  if (!fontName) return NULL;
  
  lockGlobalParams;
  dfp = (DisplayFontParam *)displayFonts->lookup(fontName);
  if (!dfp)
  {
    FcChar8* s;
    char * ext;
    FcResult res;
    FcFontSet *set;
    int i;
    p = buildFcPattern(font);

    if (!p)
      goto fin;
    FcConfigSubstitute(FCcfg, p, FcMatchPattern);
    FcDefaultSubstitute(p);
    set = FcFontSort(FCcfg, p, FcFalse, NULL, &res);
    if (!set)
      goto fin;
    for (i = 0; i < set->nfont; ++i)
    {
      res = FcPatternGetString(set->fonts[i], FC_FILE, 0, &s);
      if (res != FcResultMatch || !s)
        continue;
      ext = strrchr((char*)s,'.');
      if (!ext)
        continue;
      if (!strncasecmp(ext,".ttf",4) || !strncasecmp(ext, ".ttc", 4))
      {
        dfp = new DisplayFontParam(fontName->copy(), displayFontTT);  
        dfp->tt.fileName = new GooString((char*)s);
        FcPatternGetInteger(set->fonts[i], FC_INDEX, 0, &(dfp->tt.faceIndex));
      }
      else if (!strncasecmp(ext,".pfa",4) || !strncasecmp(ext,".pfb",4)) 
      {
        dfp = new DisplayFontParam(fontName->copy(), displayFontT1);  
        dfp->t1.fileName = new GooString((char*)s);
      }
      else
        continue;
      displayFonts->add(dfp->name,dfp);
      break;
    }
    FcFontSetDestroy(set);
  }
fin:
  if (p)
    FcPatternDestroy(p);

  unlockGlobalParams;
  return dfp;
}
#endif

GooString *GlobalParams::getPSFile() {
  GooString *s;

  lockGlobalParams;
  s = psFile ? psFile->copy() : (GooString *)NULL;
  unlockGlobalParams;
  return s;
}

int GlobalParams::getPSPaperWidth() {
  int w;

  lockGlobalParams;
  w = psPaperWidth;
  unlockGlobalParams;
  return w;
}

int GlobalParams::getPSPaperHeight() {
  int h;

  lockGlobalParams;
  h = psPaperHeight;
  unlockGlobalParams;
  return h;
}

void GlobalParams::getPSImageableArea(int *llx, int *lly, int *urx, int *ury) {
  lockGlobalParams;
  *llx = psImageableLLX;
  *lly = psImageableLLY;
  *urx = psImageableURX;
  *ury = psImageableURY;
  unlockGlobalParams;
}

GBool GlobalParams::getPSCrop() {
  GBool f;

  lockGlobalParams;
  f = psCrop;
  unlockGlobalParams;
  return f;
}

GBool GlobalParams::getPSExpandSmaller() {
  GBool f;

  lockGlobalParams;
  f = psExpandSmaller;
  unlockGlobalParams;
  return f;
}

GBool GlobalParams::getPSShrinkLarger() {
  GBool f;

  lockGlobalParams;
  f = psShrinkLarger;
  unlockGlobalParams;
  return f;
}

GBool GlobalParams::getPSCenter() {
  GBool f;

  lockGlobalParams;
  f = psCenter;
  unlockGlobalParams;
  return f;
}

GBool GlobalParams::getPSDuplex() {
  GBool d;

  lockGlobalParams;
  d = psDuplex;
  unlockGlobalParams;
  return d;
}

PSLevel GlobalParams::getPSLevel() {
  PSLevel level;

  lockGlobalParams;
  level = psLevel;
  unlockGlobalParams;
  return level;
}

PSFontParam *GlobalParams::getPSFont(GooString *fontName) {
  PSFontParam *p;

  lockGlobalParams;
  p = (PSFontParam *)psFonts->lookup(fontName);
  unlockGlobalParams;
  return p;
}

PSFontParam *GlobalParams::getPSFont16(GooString *fontName,
				       GooString *collection, int wMode) {
  PSFontParam *p;
  int i;

  lockGlobalParams;
  p = NULL;
  if (fontName) {
    for (i = 0; i < psNamedFonts16->getLength(); ++i) {
      p = (PSFontParam *)psNamedFonts16->get(i);
      if (!p->pdfFontName->cmp(fontName) &&
	  p->wMode == wMode) {
	break;
      }
      p = NULL;
    }
  }
  if (!p && collection) {
    for (i = 0; i < psFonts16->getLength(); ++i) {
      p = (PSFontParam *)psFonts16->get(i);
      if (!p->pdfFontName->cmp(collection) &&
	  p->wMode == wMode) {
	break;
      }
      p = NULL;
    }
  }
  unlockGlobalParams;
  return p;
}

GBool GlobalParams::getPSEmbedType1() {
  GBool e;

  lockGlobalParams;
  e = psEmbedType1;
  unlockGlobalParams;
  return e;
}

GBool GlobalParams::getPSEmbedTrueType() {
  GBool e;

  lockGlobalParams;
  e = psEmbedTrueType;
  unlockGlobalParams;
  return e;
}

GBool GlobalParams::getPSEmbedCIDPostScript() {
  GBool e;

  lockGlobalParams;
  e = psEmbedCIDPostScript;
  unlockGlobalParams;
  return e;
}

GBool GlobalParams::getPSEmbedCIDTrueType() {
  GBool e;

  lockGlobalParams;
  e = psEmbedCIDTrueType;
  unlockGlobalParams;
  return e;
}

GBool GlobalParams::getPSOPI() {
  GBool opi;

  lockGlobalParams;
  opi = psOPI;
  unlockGlobalParams;
  return opi;
}

GBool GlobalParams::getPSASCIIHex() {
  GBool ah;

  lockGlobalParams;
  ah = psASCIIHex;
  unlockGlobalParams;
  return ah;
}

GooString *GlobalParams::getTextEncodingName() {
  GooString *s;

  lockGlobalParams;
  s = textEncoding->copy();
  unlockGlobalParams;
  return s;
}

EndOfLineKind GlobalParams::getTextEOL() {
  EndOfLineKind eol;

  lockGlobalParams;
  eol = textEOL;
  unlockGlobalParams;
  return eol;
}

GBool GlobalParams::getTextPageBreaks() {
  GBool pageBreaks;

  lockGlobalParams;
  pageBreaks = textPageBreaks;
  unlockGlobalParams;
  return pageBreaks;
}

GBool GlobalParams::getTextKeepTinyChars() {
  GBool tiny;

  lockGlobalParams;
  tiny = textKeepTinyChars;
  unlockGlobalParams;
  return tiny;
}

GooString *GlobalParams::findFontFile(GooString *fontName, char **exts) {
  GooString *dir, *fileName;
  char **ext;
  FILE *f;
  int i;

  lockGlobalParams;
  for (i = 0; i < fontDirs->getLength(); ++i) {
    dir = (GooString *)fontDirs->get(i);
    for (ext = exts; *ext; ++ext) {
      fileName = appendToPath(dir->copy(), fontName->getCString());
      fileName->append(*ext);
      if ((f = fopen(fileName->getCString(), "rb"))) {
	fclose(f);
	unlockGlobalParams;
	return fileName;
      }
      delete fileName;
    }
  }
  unlockGlobalParams;
  return NULL;
}

GooString *GlobalParams::getInitialZoom() {
  GooString *s;

  lockGlobalParams;
  s = initialZoom->copy();
  unlockGlobalParams;
  return s;
}

GBool GlobalParams::getContinuousView() {
  GBool f;

  lockGlobalParams;
  f = continuousView;
  unlockGlobalParams;
  return f;
}

GBool GlobalParams::getEnableT1lib() {
  GBool f;

  lockGlobalParams;
  f = enableT1lib;
  unlockGlobalParams;
  return f;
}

GBool GlobalParams::getEnableFreeType() {
  GBool f;

  lockGlobalParams;
  f = enableFreeType;
  unlockGlobalParams;
  return f;
}


GBool GlobalParams::getAntialias() {
  GBool f;

  lockGlobalParams;
  f = antialias;
  unlockGlobalParams;
  return f;
}

GBool GlobalParams::getMapNumericCharNames() {
  GBool map;

  lockGlobalParams;
  map = mapNumericCharNames;
  unlockGlobalParams;
  return map;
}

GBool GlobalParams::getPrintCommands() {
  GBool p;

  lockGlobalParams;
  p = printCommands;
  unlockGlobalParams;
  return p;
}

GBool GlobalParams::getProfileCommands() {
  GBool p;

  lockGlobalParams;
  p = profileCommands;
  unlockGlobalParams;
  return p;
}

GBool GlobalParams::getErrQuiet() {
  GBool q;

  lockGlobalParams;
  q = errQuiet;
  unlockGlobalParams;
  return q;
}

CharCodeToUnicode *GlobalParams::getCIDToUnicode(GooString *collection) {
  GooString *fileName;
  CharCodeToUnicode *ctu;

  lockGlobalParams;
  if (!(ctu = cidToUnicodeCache->getCharCodeToUnicode(collection))) {
    if ((fileName = (GooString *)cidToUnicodes->lookup(collection)) &&
	(ctu = CharCodeToUnicode::parseCIDToUnicode(fileName, collection))) {
      cidToUnicodeCache->add(ctu);
    }
  }
  unlockGlobalParams;
  return ctu;
}

CharCodeToUnicode *GlobalParams::getUnicodeToUnicode(GooString *fontName) {
  CharCodeToUnicode *ctu;
  GooHashIter *iter;
  GooString *fontPattern, *fileName;

  lockGlobalParams;
  fileName = NULL;
  unicodeToUnicodes->startIter(&iter);
  while (unicodeToUnicodes->getNext(&iter, &fontPattern, (void **)&fileName)) {
    if (strstr(fontName->getCString(), fontPattern->getCString())) {
      unicodeToUnicodes->killIter(&iter);
      break;
    }
    fileName = NULL;
  }
  if (fileName) {
    if (!(ctu = unicodeToUnicodeCache->getCharCodeToUnicode(fileName))) {
      if ((ctu = CharCodeToUnicode::parseUnicodeToUnicode(fileName))) {
	unicodeToUnicodeCache->add(ctu);
      }
    }
  } else {
    ctu = NULL;
  }
  unlockGlobalParams;
  return ctu;
}

UnicodeMap *GlobalParams::getUnicodeMap(GooString *encodingName) {
  return getUnicodeMap2(encodingName);
}

UnicodeMap *GlobalParams::getUnicodeMap2(GooString *encodingName) {
  UnicodeMap *map;

  if (!(map = getResidentUnicodeMap(encodingName))) {
    lockUnicodeMapCache;
    map = unicodeMapCache->getUnicodeMap(encodingName);
    unlockUnicodeMapCache;
  }
  return map;
}

CMap *GlobalParams::getCMap(GooString *collection, GooString *cMapName) {
  CMap *cMap;

  lockCMapCache;
  cMap = cMapCache->getCMap(collection, cMapName);
  unlockCMapCache;
  return cMap;
}

UnicodeMap *GlobalParams::getTextEncoding() {
  return getUnicodeMap2(textEncoding);
}

//------------------------------------------------------------------------
// functions to set parameters
//------------------------------------------------------------------------

void GlobalParams::setPSFile(char *file) {
  lockGlobalParams;
  if (psFile) {
    delete psFile;
  }
  psFile = new GooString(file);
  unlockGlobalParams;
}

GBool GlobalParams::setPSPaperSize(char *size) {
  lockGlobalParams;
  if (!strcmp(size, "match")) {
    psPaperWidth = psPaperHeight = -1;
  } else if (!strcmp(size, "letter")) {
    psPaperWidth = 612;
    psPaperHeight = 792;
  } else if (!strcmp(size, "legal")) {
    psPaperWidth = 612;
    psPaperHeight = 1008;
  } else if (!strcmp(size, "A4")) {
    psPaperWidth = 595;
    psPaperHeight = 842;
  } else if (!strcmp(size, "A3")) {
    psPaperWidth = 842;
    psPaperHeight = 1190;
  } else {
    unlockGlobalParams;
    return gFalse;
  }
  psImageableLLX = psImageableLLY = 0;
  psImageableURX = psPaperWidth;
  psImageableURY = psPaperHeight;
  unlockGlobalParams;
  return gTrue;
}

void GlobalParams::setPSPaperWidth(int width) {
  lockGlobalParams;
  psPaperWidth = width;
  psImageableLLX = 0;
  psImageableURX = psPaperWidth;
  unlockGlobalParams;
}

void GlobalParams::setPSPaperHeight(int height) {
  lockGlobalParams;
  psPaperHeight = height;
  psImageableLLY = 0;
  psImageableURY = psPaperHeight;
  unlockGlobalParams;
}

void GlobalParams::setPSImageableArea(int llx, int lly, int urx, int ury) {
  lockGlobalParams;
  psImageableLLX = llx;
  psImageableLLY = lly;
  psImageableURX = urx;
  psImageableURY = ury;
  unlockGlobalParams;
}

void GlobalParams::setPSCrop(GBool crop) {
  lockGlobalParams;
  psCrop = crop;
  unlockGlobalParams;
}

void GlobalParams::setPSExpandSmaller(GBool expand) {
  lockGlobalParams;
  psExpandSmaller = expand;
  unlockGlobalParams;
}

void GlobalParams::setPSShrinkLarger(GBool shrink) {
  lockGlobalParams;
  psShrinkLarger = shrink;
  unlockGlobalParams;
}

void GlobalParams::setPSCenter(GBool center) {
  lockGlobalParams;
  psCenter = center;
  unlockGlobalParams;
}

void GlobalParams::setPSDuplex(GBool duplex) {
  lockGlobalParams;
  psDuplex = duplex;
  unlockGlobalParams;
}

void GlobalParams::setPSLevel(PSLevel level) {
  lockGlobalParams;
  psLevel = level;
  unlockGlobalParams;
}

void GlobalParams::setPSEmbedType1(GBool embed) {
  lockGlobalParams;
  psEmbedType1 = embed;
  unlockGlobalParams;
}

void GlobalParams::setPSEmbedTrueType(GBool embed) {
  lockGlobalParams;
  psEmbedTrueType = embed;
  unlockGlobalParams;
}

void GlobalParams::setPSEmbedCIDPostScript(GBool embed) {
  lockGlobalParams;
  psEmbedCIDPostScript = embed;
  unlockGlobalParams;
}

void GlobalParams::setPSEmbedCIDTrueType(GBool embed) {
  lockGlobalParams;
  psEmbedCIDTrueType = embed;
  unlockGlobalParams;
}

void GlobalParams::setPSOPI(GBool opi) {
  lockGlobalParams;
  psOPI = opi;
  unlockGlobalParams;
}

void GlobalParams::setPSASCIIHex(GBool hex) {
  lockGlobalParams;
  psASCIIHex = hex;
  unlockGlobalParams;
}

void GlobalParams::setTextEncoding(char *encodingName) {
  lockGlobalParams;
  delete textEncoding;
  textEncoding = new GooString(encodingName);
  unlockGlobalParams;
}

GBool GlobalParams::setTextEOL(char *s) {
  lockGlobalParams;
  if (!strcmp(s, "unix")) {
    textEOL = eolUnix;
  } else if (!strcmp(s, "dos")) {
    textEOL = eolDOS;
  } else if (!strcmp(s, "mac")) {
    textEOL = eolMac;
  } else {
    unlockGlobalParams;
    return gFalse;
  }
  unlockGlobalParams;
  return gTrue;
}

void GlobalParams::setTextPageBreaks(GBool pageBreaks) {
  lockGlobalParams;
  textPageBreaks = pageBreaks;
  unlockGlobalParams;
}

void GlobalParams::setTextKeepTinyChars(GBool keep) {
  lockGlobalParams;
  textKeepTinyChars = keep;
  unlockGlobalParams;
}

void GlobalParams::setInitialZoom(char *s) {
  lockGlobalParams;
  delete initialZoom;
  initialZoom = new GooString(s);
  unlockGlobalParams;
}

void GlobalParams::setContinuousView(GBool cont) {
  lockGlobalParams;
  continuousView = cont;
  unlockGlobalParams;
}

GBool GlobalParams::setEnableT1lib(char *s) {
  GBool ok;

  lockGlobalParams;
  ok = parseYesNo2(s, &enableT1lib);
  unlockGlobalParams;
  return ok;
}

GBool GlobalParams::setEnableFreeType(char *s) {
  GBool ok;

  lockGlobalParams;
  ok = parseYesNo2(s, &enableFreeType);
  unlockGlobalParams;
  return ok;
}


GBool GlobalParams::setAntialias(char *s) {
  GBool ok;

  lockGlobalParams;
  ok = parseYesNo2(s, &antialias);
  unlockGlobalParams;
  return ok;
}

void GlobalParams::setMapNumericCharNames(GBool map) {
  lockGlobalParams;
  mapNumericCharNames = map;
  unlockGlobalParams;
}

void GlobalParams::setPrintCommands(GBool printCommandsA) {
  lockGlobalParams;
  printCommands = printCommandsA;
  unlockGlobalParams;
}

void GlobalParams::setProfileCommands(GBool profileCommandsA) {
  lockGlobalParams;
  profileCommands = profileCommandsA;
  unlockGlobalParams;
}

void GlobalParams::setErrQuiet(GBool errQuietA) {
  lockGlobalParams;
  errQuiet = errQuietA;
  unlockGlobalParams;
}

void GlobalParams::addSecurityHandler(XpdfSecurityHandler *handler) {
#ifdef ENABLE_PLUGINS
  lockGlobalParams;
  securityHandlers->append(handler);
  unlockGlobalParams;
#endif
}

XpdfSecurityHandler *GlobalParams::getSecurityHandler(char *name) {
#ifdef ENABLE_PLUGINS
  XpdfSecurityHandler *hdlr;
  int i;

  lockGlobalParams;
  for (i = 0; i < securityHandlers->getLength(); ++i) {
    hdlr = (XpdfSecurityHandler *)securityHandlers->get(i);
    if (!stricmp(hdlr->name, name)) {
      unlockGlobalParams;
      return hdlr;
    }
  }
  unlockGlobalParams;

  if (!loadPlugin("security", name)) {
    return NULL;
  }

  lockGlobalParams;
  for (i = 0; i < securityHandlers->getLength(); ++i) {
    hdlr = (XpdfSecurityHandler *)securityHandlers->get(i);
    if (!strcmp(hdlr->name, name)) {
      unlockGlobalParams;
      return hdlr;
    }
  }
  unlockGlobalParams;
#endif

  return NULL;
}

#ifdef ENABLE_PLUGINS
//------------------------------------------------------------------------
// plugins
//------------------------------------------------------------------------

GBool GlobalParams::loadPlugin(char *type, char *name) {
  Plugin *plugin;

  if (!(plugin = Plugin::load(type, name))) {
    return gFalse;
  }
  lockGlobalParams;
  plugins->append(plugin);
  unlockGlobalParams;
  return gTrue;
}

#endif // ENABLE_PLUGINS
