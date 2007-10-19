//========================================================================
//
// HtmlOutputDev.h
//
// Copyright 1997 Derek B. Noonburg
//
// Changed 1999 by G.Ovtcharov
//========================================================================

#ifndef HTMLOUTPUTDEV_H
#define HTMLOUTPUTDEV_H

#ifdef __GNUC__
#pragma interface
#endif

#include <stdio.h>
#include "goo/gtypes.h"
#include "goo/GooList.h"
#include "GfxFont.h"
#include "OutputDev.h"
#include "HtmlLinks.h"
#include "HtmlFonts.h"
#include "Link.h"
#include "Catalog.h"
#include "UnicodeMap.h"


#ifdef WIN32
#  define SLASH '\\'
#else
#  define SLASH '/'
#endif

#define xoutRound(x) ((int)(x + 0.5))

#define DOCTYPE "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">"
#define DOCTYPE_FRAMES "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\"\n\"http://www.w3.org/TR/html4/frameset.dtd\">"

class GfxState;
class GooString;
//------------------------------------------------------------------------
// HtmlString
//------------------------------------------------------------------------

enum UnicodeTextDirection {
  textDirUnknown,
  textDirLeftRight,
  textDirRightLeft,
  textDirTopBottom
};


class HtmlString {
public:

  // Constructor.
  HtmlString(GfxState *state, double fontSize, HtmlFontAccu* fonts);

  // Destructor.
  ~HtmlString();

  // Add a character to the string.
  void addChar(GfxState *state, double x, double y,
	       double dx, double dy,
	       Unicode u);
  HtmlLink* getLink() { return link; }
  void endString(); // postprocessing

private:
// aender die text variable
  HtmlLink *link;
  double xMin, xMax;		// bounding box x coordinates
  double yMin, yMax;		// bounding box y coordinates
  int col;			// starting column
  Unicode *text;		// the text
  double *xRight;		// right-hand x coord of each char
  HtmlString *yxNext;		// next string in y-major order
  HtmlString *xyNext;		// next string in x-major order
  int fontpos;
  GooString* htext;
  int len;			// length of text and xRight
  int size;			// size of text and xRight arrays
  UnicodeTextDirection dir;	// direction (left to right/right to left)

  friend class HtmlPage;

};


//------------------------------------------------------------------------
// HtmlPage
//------------------------------------------------------------------------



class HtmlPage {
public:

  // Constructor.
  HtmlPage(GBool rawOrder, char *imgExtVal);

  // Destructor.
  ~HtmlPage();

  // Begin a new string.
  void beginString(GfxState *state, GooString *s);

  // Add a character to the current string.
  void addChar(GfxState *state, double x, double y,
	       double dx, double dy,
		double ox, double oy,
		Unicode *u, int uLen); //Guchar c);

  void updateFont(GfxState *state);

  // End the current string, sorting it into the list of strings.
  void endString();

  // Coalesce strings that look like parts of the same line.
  void coalesce();

  // Find a string.  If <top> is true, starts looking at top of page;
  // otherwise starts looking at <xMin>,<yMin>.  If <bottom> is true,
  // stops looking at bottom of page; otherwise stops looking at
  // <xMax>,<yMax>.  If found, sets the text bounding rectange and
  // returns true; otherwise returns false.


  // new functions
  void AddLink(const HtmlLink& x){
    links->AddLink(x);
  }

 void dump(FILE *f, int pageNum);

  // Clear the page.
  void clear();

  void conv();
private:
  HtmlFont* getFont(HtmlString *hStr) { return fonts->Get(hStr->fontpos); }

  double fontSize;		// current font size
  GBool rawOrder;		// keep strings in content stream order

  HtmlString *curStr;		// currently active string

  HtmlString *yxStrings;	// strings in y-major order
  HtmlString *xyStrings;	// strings in x-major order
  HtmlString *yxCur1, *yxCur2;	// cursors for yxStrings list

  void setDocName(char* fname);
  void dumpAsXML(FILE* f,int page);
  void dumpComplex(FILE* f, int page);

  // marks the position of the fonts that belong to current page (for noframes)
  int fontsPageMarker;
  HtmlFontAccu *fonts;
  HtmlLinks *links;

  GooString *DocName;
  GooString *imgExt;
  int pageWidth;
  int pageHeight;
  static int pgNum;
  int firstPage;                // used to begin the numeration of pages

  friend class HtmlOutputDev;
};

//------------------------------------------------------------------------
// HtmlMetaVar
//------------------------------------------------------------------------
class HtmlMetaVar {
public:
    HtmlMetaVar(char *_name, char *_content);
    ~HtmlMetaVar();

    GooString* toString();

private:

    GooString *name;
    GooString *content;
};

//------------------------------------------------------------------------
// HtmlOutputDev
//------------------------------------------------------------------------

class HtmlOutputDev: public OutputDev {
public:

  // Open a text output file.  If <fileName> is NULL, no file is written
  // (this is useful, e.g., for searching text).  If <useASCII7> is true,
  // text is converted to 7-bit ASCII; otherwise, text is converted to
  // 8-bit ISO Latin-1.  <useASCII7> should also be set for Japanese
  // (EUC-JP) text.  If <rawOrder> is true, the text is kept in content
  // stream order.
  HtmlOutputDev(char *fileName, char *title,
	  char *author,
	  char *keywords,
	  char *subject,
	  char *date,
	  char *extension,
	  GBool rawOrder,
	  int firstPage = 1,
	  GBool outline = 0);

  // Destructor.
  virtual ~HtmlOutputDev();

  // Check if file was successfully created.
  virtual GBool isOk() { return ok; }

  //---- get info about output device

  // Does this device use upside-down coordinates?
  // (Upside-down means (0,0) is the top left corner of the page.)
  virtual GBool upsideDown() { return gTrue; }

  // Does this device use drawChar() or drawString()?
  virtual GBool useDrawChar() { return gTrue; }

  // Does this device use beginType3Char/endType3Char?  Otherwise,
  // text in Type 3 fonts will be drawn with drawChar/drawString.
  virtual GBool interpretType3Chars() { return gFalse; }

  // Does this device need non-text content?
  virtual GBool needNonText() { return gFalse; }

  //----- initialization and control

  // Start a page.
  virtual void startPage(int pageNum, GfxState *state);

  // End a page.
  virtual void endPage();

  //----- update text state
  virtual void updateFont(GfxState *state);

  //----- text drawing
  virtual void beginString(GfxState *state, GooString *s);
  virtual void endString(GfxState *state);
  virtual void drawChar(GfxState *state, double x, double y,
			double dx, double dy,
			double originX, double originY,
			CharCode code, int nBytes, Unicode *u, int uLen);

  virtual void drawImageMask(GfxState *state, Object *ref,
			     Stream *str,
			     int width, int height, GBool invert,
			     GBool inlineImg);
  virtual void drawImage(GfxState *state, Object *ref, Stream *str,
			  int width, int height, GfxImageColorMap *colorMap,
			 int *maskColors, GBool inlineImg);

  //new feature
  virtual int DevType() {return 1234;}
  virtual void drawLink(Link *link,Catalog *cat);

  int getPageWidth() { return maxPageWidth; }
  int getPageHeight() { return maxPageHeight; }

  GBool dumpDocOutline(Catalog* catalog);

  /*  char* getFileNameFromPath(char* c, int strlen); */

private:
  // convert encoding into a HTML standard, or encoding->getCString if not
  // recognized
  static char* mapEncodingToHtml(GooString* encoding);
  GooString* getLinkDest(Link *link,Catalog *catalog);
  void dumpMetaVars(FILE *);
  void doFrame(int firstPage);
  GBool newOutlineLevel(FILE *output, Object *node, Catalog* catalog, int level = 1);

  FILE *fContentsFrame;
  FILE *page;                   // html file
  //FILE *tin;                    // image log file
  //GBool write;
  GBool needClose;		// need to close the file?
  HtmlPage *pages;		// text for the current page
  GBool rawOrder;		// keep text in content stream order
  GBool doOutline;		// output document outline
  GBool ok;			// set up ok?
  GBool dumpJPEG;
  int pageNum;
  int maxPageWidth;
  int maxPageHeight;
  static int imgNum;
  GooString *Docname;
  GooString *docTitle;
  GooList *glMetaVars;
  friend class HtmlPage;
};

char* getFileNameFromPath(char* c, int strlen);

#endif
