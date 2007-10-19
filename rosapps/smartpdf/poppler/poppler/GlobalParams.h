//========================================================================
//
// GlobalParams.h
//
// Copyright 2001-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef GLOBALPARAMS_H
#define GLOBALPARAMS_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "poppler-config.h"
#include <assert.h>
#include <stdio.h>
#ifdef HAVE_FONTCONFIG_H
#include <fontconfig/fontconfig.h>
#endif
#include "goo/gtypes.h"
#include "CharTypes.h"

#if MULTITHREADED
#include <goo/GooMutex.h>
#endif

class GooString;
class GooList;
class GooHash;
class NameToCharCode;
class CharCodeToUnicode;
class CharCodeToUnicodeCache;
class UnicodeMap;
class UnicodeMapCache;
class CMap;
class CMapCache;
struct XpdfSecurityHandler;
class GlobalParams;
class GfxFont;

//------------------------------------------------------------------------

// The global parameters object.
extern GlobalParams *globalParams;

//------------------------------------------------------------------------

enum DisplayFontParamKind {
  displayFontT1,
  displayFontTT
};

struct DisplayFontParamT1 {
  GooString *fileName;
};

struct DisplayFontParamTT {
  GooString *fileName;
  int faceIndex;
};

class DisplayFontParam {
public:

  GooString *name;		// font name for 8-bit fonts and named
				//   CID fonts; collection name for
				//   generic CID fonts
  DisplayFontParamKind kind;
  union {
    DisplayFontParamT1 t1;
    DisplayFontParamTT tt;
  };

  DisplayFontParam(GooString *nameA, DisplayFontParamKind kindA);
  void setFileName(GooString *fileNameA) {
    if (displayFontT1 == kind)
        t1.fileName = fileNameA;
    else {
        assert(displayFontTT == kind);
        tt.fileName = fileNameA;
    }
  }
  ~DisplayFontParam();
};

//------------------------------------------------------------------------

class PSFontParam {
public:

  GooString *pdfFontName;		// PDF font name for 8-bit fonts and
				//   named 16-bit fonts; char collection
				//   name for generic 16-bit fonts
  int wMode;			// writing mode (0=horiz, 1=vert) for
				//   16-bit fonts
  GooString *psFontName;		// PostScript font name
  GooString *encoding;		// encoding, for 16-bit fonts only

  PSFontParam(GooString *pdfFontNameA, int wModeA,
	      GooString *psFontNameA, GooString *encodingA);
  ~PSFontParam();
};

//------------------------------------------------------------------------

enum PSLevel {
  psLevel1,
  psLevel1Sep,
  psLevel2,
  psLevel2Sep,
  psLevel3,
  psLevel3Sep
};

//------------------------------------------------------------------------

enum EndOfLineKind {
  eolUnix,			// LF
  eolDOS,			// CR+LF
  eolMac			// CR
};

//------------------------------------------------------------------------

class GlobalParams {
public:

  // Initialize the global parameters by attempting to read a config
  // file.
  GlobalParams(char *cfgFileName);

  ~GlobalParams();

  void setBaseDir(char *dir);

#ifndef HAVE_FONTCONFIG_H
  void setupBaseFonts(char *dir);
#endif

  //----- accessors

  CharCode getMacRomanCharCode(char *charName);

  GooString *getBaseDir();
  Unicode mapNameToUnicode(char *charName);
  UnicodeMap *getResidentUnicodeMap(GooString *encodingName);
  FILE *getUnicodeMapFile(GooString *encodingName);
  FILE *findCMapFile(GooString *collection, GooString *cMapName);
  FILE *findToUnicodeFile(GooString *name);
  DisplayFontParam *getDisplayFont(GfxFont *font);
  GooString *getPSFile();
  int getPSPaperWidth();
  int getPSPaperHeight();
  void getPSImageableArea(int *llx, int *lly, int *urx, int *ury);
  GBool getPSDuplex();
  GBool getPSCrop();
  GBool getPSExpandSmaller();
  GBool getPSShrinkLarger();
  GBool getPSCenter();
  PSLevel getPSLevel();
  PSFontParam *getPSFont(GooString *fontName);
  PSFontParam *getPSFont16(GooString *fontName, GooString *collection, int wMode);
  GBool getPSEmbedType1();
  GBool getPSEmbedTrueType();
  GBool getPSEmbedCIDPostScript();
  GBool getPSEmbedCIDTrueType();
  GBool getPSOPI();
  GBool getPSASCIIHex();
  GooString *getTextEncodingName();
  EndOfLineKind getTextEOL();
  GBool getTextPageBreaks();
  GBool getTextKeepTinyChars();
  GooString *findFontFile(GooString *fontName, char **exts);
  GooString *getInitialZoom();
  GBool getContinuousView();
  GBool getEnableT1lib();
  GBool getEnableFreeType();
  GBool getAntialias();
  GooString *getURLCommand() { return urlCommand; }
  GooString *getMovieCommand() { return movieCommand; }
  GBool getMapNumericCharNames();
  GBool getPrintCommands();
  GBool getProfileCommands();
  GBool getErrQuiet();

  CharCodeToUnicode *getCIDToUnicode(GooString *collection);
  CharCodeToUnicode *getUnicodeToUnicode(GooString *fontName);
  UnicodeMap *getUnicodeMap(GooString *encodingName);
  CMap *getCMap(GooString *collection, GooString *cMapName);
  UnicodeMap *getTextEncoding();
#ifdef ENABLE_PLUGINS
  GBool loadPlugin(char *type, char *name);
#endif

  //----- functions to set parameters

  void setPSFile(char *file);
  GBool setPSPaperSize(char *size);
  void setPSPaperWidth(int width);
  void setPSPaperHeight(int height);
  void setPSImageableArea(int llx, int lly, int urx, int ury);
  void setPSDuplex(GBool duplex);
  void setPSCrop(GBool crop);
  void setPSExpandSmaller(GBool expand);
  void setPSShrinkLarger(GBool shrink);
  void setPSCenter(GBool center);
  void setPSLevel(PSLevel level);
  void setPSEmbedType1(GBool embed);
  void setPSEmbedTrueType(GBool embed);
  void setPSEmbedCIDPostScript(GBool embed);
  void setPSEmbedCIDTrueType(GBool embed);
  void setPSOPI(GBool opi);
  void setPSASCIIHex(GBool hex);
  void setTextEncoding(char *encodingName);
  GBool setTextEOL(char *s);
  void setTextPageBreaks(GBool pageBreaks);
  void setTextKeepTinyChars(GBool keep);
  void setInitialZoom(char *s);
  void setContinuousView(GBool cont);
  GBool setEnableT1lib(char *s);
  GBool setEnableFreeType(char *s);
  GBool setAntialias(char *s);
  void setMapNumericCharNames(GBool map);
  void setPrintCommands(GBool printCommandsA);
  void setProfileCommands(GBool profileCommandsA);
  void setErrQuiet(GBool errQuietA);

  //----- security handlers

  void addSecurityHandler(XpdfSecurityHandler *handler);
  XpdfSecurityHandler *getSecurityHandler(char *name);

private:

  void parseFile(GooString *fileName, FILE *f);
  void parseNameToUnicode(GooString *name);
  void parseCIDToUnicode(GooList *tokens, GooString *fileName, int line);
  void parseUnicodeToUnicode(GooList *tokens, GooString *fileName, int line);
  void parseUnicodeMap(GooList *tokens, GooString *fileName, int line);
  void parseCMapDir(GooList *tokens, GooString *fileName, int line);
  void parseToUnicodeDir(GooList *tokens, GooString *fileName, int line);
  void parsePSFile(GooList *tokens, GooString *fileName, int line);
  void parsePSPaperSize(GooList *tokens, GooString *fileName, int line);
  void parsePSImageableArea(GooList *tokens, GooString *fileName, int line);
  void parsePSLevel(GooList *tokens, GooString *fileName, int line);
  void parsePSFont(GooList *tokens, GooString *fileName, int line);
  void parsePSFont16(char *cmdName, GooList *fontList,
		     GooList *tokens, GooString *fileName, int line);
  void parseTextEncoding(GooList *tokens, GooString *fileName, int line);
  void parseTextEOL(GooList *tokens, GooString *fileName, int line);
  void parseFontDir(GooList *tokens, GooString *fileName, int line);
  void parseInitialZoom(GooList *tokens, GooString *fileName, int line);
  void parseCommand(char *cmdName, GooString **val,
		    GooList *tokens, GooString *fileName, int line);
  void parseYesNo(char *cmdName, GBool *flag,
		  GooList *tokens, GooString *fileName, int line);
  GBool parseYesNo2(char *token, GBool *flag);
  UnicodeMap *getUnicodeMap2(GooString *encodingName);

  void scanEncodingDirs();
  void addCIDToUnicode(GooString *collection, GooString *fileName);
  void addUnicodeMap(GooString *encodingName, GooString *fileName);
  void addCMapDir(GooString *collection, GooString *dir);

  //----- static tables

  NameToCharCode *		// mapping from char name to
    macRomanReverseMap;		//   MacRomanEncoding index

  //----- user-modifiable settings

  GooString *baseDir;		// base directory - for plugins, etc.
  NameToCharCode *		// mapping from char name to Unicode
    nameToUnicode;
  GooHash *cidToUnicodes;		// files for mappings from char collections
				//   to Unicode, indexed by collection name
				//   [GooString]
  GooHash *unicodeToUnicodes;	// files for Unicode-to-Unicode mappings,
				//   indexed by font name pattern [GooString]
  GooHash *residentUnicodeMaps;	// mappings from Unicode to char codes,
				//   indexed by encoding name [UnicodeMap]
  GooHash *unicodeMaps;		// files for mappings from Unicode to char
				//   codes, indexed by encoding name [GooString]
  GooHash *cMapDirs;		// list of CMap dirs, indexed by collection
				//   name [GooList[GooString]]
  GooList *toUnicodeDirs;		// list of ToUnicode CMap dirs [GooString]
  GooHash *displayFonts;		// display font info, indexed by font name
				//   [DisplayFontParam]
  GooString *psFile;		// PostScript file or command (for xpdf)
  int psPaperWidth;		// paper size, in PostScript points, for
  int psPaperHeight;		//   PostScript output
  int psImageableLLX,		// imageable area, in PostScript points,
      psImageableLLY,		//   for PostScript output
      psImageableURX,
      psImageableURY;
  GBool psCrop;			// crop PS output to CropBox
  GBool psExpandSmaller;	// expand smaller pages to fill paper
  GBool psShrinkLarger;		// shrink larger pages to fit paper
  GBool psCenter;		// center pages on the paper
  GBool psDuplex;		// enable duplexing in PostScript?
  PSLevel psLevel;		// PostScript level to generate
  GooHash *psFonts;		// PostScript font info, indexed by PDF
				//   font name [PSFontParam]
  GooList *psNamedFonts16;	// named 16-bit fonts [PSFontParam]
  GooList *psFonts16;		// generic 16-bit fonts [PSFontParam]
  GBool psEmbedType1;		// embed Type 1 fonts?
  GBool psEmbedTrueType;	// embed TrueType fonts?
  GBool psEmbedCIDPostScript;	// embed CID PostScript fonts?
  GBool psEmbedCIDTrueType;	// embed CID TrueType fonts?
  GBool psOPI;			// generate PostScript OPI comments?
  GBool psASCIIHex;		// use ASCIIHex instead of ASCII85?
  GooString *textEncoding;	// encoding (unicodeMap) to use for text
				//   output
  EndOfLineKind textEOL;	// type of EOL marker to use for text
				//   output
  GBool textPageBreaks;		// insert end-of-page markers?
  GBool textKeepTinyChars;	// keep all characters in text output
  GooList *fontDirs;		// list of font dirs [GooString]
  GooString *initialZoom;		// initial zoom level
  GBool continuousView;		// continuous view mode
  GBool enableT1lib;		// t1lib enable flag
  GBool enableFreeType;		// FreeType enable flag
  GBool antialias;		// anti-aliasing enable flag
  GooString *urlCommand;		// command executed for URL links
  GooString *movieCommand;	// command executed for movie annotations
  GBool mapNumericCharNames;	// map numeric char names (from font subsets)?
  GBool printCommands;		// print the drawing commands
  GBool profileCommands;	// profile the drawing commands
  GBool errQuiet;		// suppress error messages?

  CharCodeToUnicodeCache *cidToUnicodeCache;
  CharCodeToUnicodeCache *unicodeToUnicodeCache;
  UnicodeMapCache *unicodeMapCache;
  CMapCache *cMapCache;

#ifdef HAVE_FONTCONFIG_H
  FcConfig *FCcfg;
#endif

#ifdef ENABLE_PLUGINS
  GList *plugins;		// list of plugins [Plugin]
  GList *securityHandlers;	// list of loaded security handlers
				//   [XpdfSecurityHandler]
#endif

#if MULTITHREADED
  GooMutex mutex;
  GooMutex unicodeMapCacheMutex;
  GooMutex cMapCacheMutex;
#endif
};

#endif
