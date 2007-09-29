//========================================================================
//
// Gfx.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef GFX_H
#define GFX_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include "goo/gtypes.h"
#include "Object.h"

class GooString;
class XRef;
class Array;
class Stream;
class Parser;
class Dict;
class OutputDev;
class GfxFontDict;
class GfxFont;
class GfxPattern;
class GfxTilingPattern;
class GfxShadingPattern;
class GfxShading;
class GfxFunctionShading;
class GfxAxialShading;
class GfxRadialShading;
class GfxGouraudTriangleShading;
class GfxPatchMeshShading;
struct GfxPatch;
class GfxState;
struct GfxColor;
class Gfx;
class PDFRectangle;

//------------------------------------------------------------------------
// Gfx
//------------------------------------------------------------------------

enum GfxClipType {
  clipNone,
  clipNormal,
  clipEO
};

enum TchkType {
  tchkBool,			// boolean
  tchkInt,			// integer
  tchkNum,			// number (integer or real)
  tchkString,			// string
  tchkName,			// name
  tchkArray,			// array
  tchkProps,			// properties (dictionary or name)
  tchkSCN,			// scn/SCN args (number of name)
  tchkNone			// used to avoid empty initializer lists
};

#define maxArgs 8

struct Operator {
  char name[4];
  int numArgs;
  TchkType tchk[maxArgs];
  void (Gfx::*func)(Object args[], int numArgs);
};

class GfxResources {
public:

  GfxResources(XRef *xref, Dict *resDict, GfxResources *nextA);
  ~GfxResources();

  GfxFont *lookupFont(char *name);
  GBool lookupXObject(char *name, Object *obj);
  GBool lookupXObjectNF(char *name, Object *obj);
  void lookupColorSpace(char *name, Object *obj);
  GfxPattern *lookupPattern(char *name);
  GfxShading *lookupShading(char *name);
  GBool lookupGState(char *name, Object *obj);

  GfxResources *getNext() { return next; }

private:

  GfxFontDict *fonts;
  Object xObjDict;
  Object colorSpaceDict;
  Object patternDict;
  Object shadingDict;
  Object gStateDict;
  GfxResources *next;
};

class Gfx {
public:

  // Constructor for regular output.
  Gfx(XRef *xrefA, OutputDev *outA, int pageNum, Dict *resDict,
      double hDPI, double vDPI, PDFRectangle *box,
      PDFRectangle *cropBox, int rotate,
      GBool (*abortCheckCbkA)(void *data) = NULL,
      void *abortCheckCbkDataA = NULL);

  // Constructor for a sub-page object.
  Gfx(XRef *xrefA, OutputDev *outA, Dict *resDict,
      PDFRectangle *box, PDFRectangle *cropBox,
      GBool (*abortCheckCbkA)(void *data) = NULL,
      void *abortCheckCbkDataA = NULL);

  ~Gfx();

  // Interpret a stream or array of streams.
  void display(Object *obj, GBool topLevel = gTrue);

  // Display an annotation, given its appearance (a Form XObject) and
  // bounding box (in default user space).
  void doAnnot(Object *str, double xMin, double yMin,
	       double xMax, double yMax);

  // Save graphics state.
  void saveState();

  // Restore graphics state.
  void restoreState();

  // Get the current graphics state object.
  GfxState *getState() { return state; }

private:

  XRef *xref;			// the xref table for this PDF file
  OutputDev *out;		// output device
  GBool subPage;		// is this a sub-page object?
  GBool printCommands;		// print the drawing commands (for debugging)
  GBool profileCommands;	// profile the drawing commands (for debugging)
  GfxResources *res;		// resource stack
  int updateLevel;

  GfxState *state;		// current graphics state
  GBool fontChanged;		// set if font or text matrix has changed
  GfxClipType clip;		// do a clip?
  int ignoreUndef;		// current BX/EX nesting level
  double baseMatrix[6];		// default matrix for most recent
				//   page/form/pattern
  int formDepth;

  Parser *parser;		// parser for page content stream(s)

  GBool				// callback to check for an abort
    (*abortCheckCbk)(void *data);
  void *abortCheckCbkData;

  static Operator opTab[];	// table of operators

  void go(GBool topLevel);
  void execOp(Object *cmd, Object args[], int numArgs);
  Operator *findOp(char *name);
  GBool checkArg(Object *arg, TchkType type);
  int getPos();

  // graphics state operators
  void opSave(Object args[], int numArgs);
  void opRestore(Object args[], int numArgs);
  void opConcat(Object args[], int numArgs);
  void opSetDash(Object args[], int numArgs);
  void opSetFlat(Object args[], int numArgs);
  void opSetLineJoin(Object args[], int numArgs);
  void opSetLineCap(Object args[], int numArgs);
  void opSetMiterLimit(Object args[], int numArgs);
  void opSetLineWidth(Object args[], int numArgs);
  void opSetExtGState(Object args[], int numArgs);
  void opSetRenderingIntent(Object args[], int numArgs);

  // color operators
  void opSetFillGray(Object args[], int numArgs);
  void opSetStrokeGray(Object args[], int numArgs);
  void opSetFillCMYKColor(Object args[], int numArgs);
  void opSetStrokeCMYKColor(Object args[], int numArgs);
  void opSetFillRGBColor(Object args[], int numArgs);
  void opSetStrokeRGBColor(Object args[], int numArgs);
  void opSetFillColorSpace(Object args[], int numArgs);
  void opSetStrokeColorSpace(Object args[], int numArgs);
  void opSetFillColor(Object args[], int numArgs);
  void opSetStrokeColor(Object args[], int numArgs);
  void opSetFillColorN(Object args[], int numArgs);
  void opSetStrokeColorN(Object args[], int numArgs);

  // path segment operators
  void opMoveTo(Object args[], int numArgs);
  void opLineTo(Object args[], int numArgs);
  void opCurveTo(Object args[], int numArgs);
  void opCurveTo1(Object args[], int numArgs);
  void opCurveTo2(Object args[], int numArgs);
  void opRectangle(Object args[], int numArgs);
  void opClosePath(Object args[], int numArgs);

  // path painting operators
  void opEndPath(Object args[], int numArgs);
  void opStroke(Object args[], int numArgs);
  void opCloseStroke(Object args[], int numArgs);
  void opFill(Object args[], int numArgs);
  void opEOFill(Object args[], int numArgs);
  void opFillStroke(Object args[], int numArgs);
  void opCloseFillStroke(Object args[], int numArgs);
  void opEOFillStroke(Object args[], int numArgs);
  void opCloseEOFillStroke(Object args[], int numArgs);
  void doPatternFill(GBool eoFill);
  void doTilingPatternFill(GfxTilingPattern *tPat, GBool eoFill);
  void doShadingPatternFill(GfxShadingPattern *sPat, GBool eoFill);
  void opShFill(Object args[], int numArgs);
  void doFunctionShFill(GfxFunctionShading *shading);
  void doFunctionShFill1(GfxFunctionShading *shading,
			 double x0, double y0,
			 double x1, double y1,
			 GfxColor *colors, int depth);
  void doAxialShFill(GfxAxialShading *shading);
  void doRadialShFill(GfxRadialShading *shading);
  void doGouraudTriangleShFill(GfxGouraudTriangleShading *shading);
  void gouraudFillTriangle(double x0, double y0, GfxColor *color0,
			   double x1, double y1, GfxColor *color1,
			   double x2, double y2, GfxColor *color2,
			   int nComps, int depth);
  void doPatchMeshShFill(GfxPatchMeshShading *shading);
  void fillPatch(GfxPatch *patch, int nComps, int depth);
  void doEndPath();

  // path clipping operators
  void opClip(Object args[], int numArgs);
  void opEOClip(Object args[], int numArgs);

  // text object operators
  void opBeginText(Object args[], int numArgs);
  void opEndText(Object args[], int numArgs);

  // text state operators
  void opSetCharSpacing(Object args[], int numArgs);
  void opSetFont(Object args[], int numArgs);
  void opSetTextLeading(Object args[], int numArgs);
  void opSetTextRender(Object args[], int numArgs);
  void opSetTextRise(Object args[], int numArgs);
  void opSetWordSpacing(Object args[], int numArgs);
  void opSetHorizScaling(Object args[], int numArgs);

  // text positioning operators
  void opTextMove(Object args[], int numArgs);
  void opTextMoveSet(Object args[], int numArgs);
  void opSetTextMatrix(Object args[], int numArgs);
  void opTextNextLine(Object args[], int numArgs);

  // text string operators
  void opShowText(Object args[], int numArgs);
  void opMoveShowText(Object args[], int numArgs);
  void opMoveSetShowText(Object args[], int numArgs);
  void opShowSpaceText(Object args[], int numArgs);
  void doShowText(GooString *s);

  // XObject operators
  void opXObject(Object args[], int numArgs);
  void doImage(Object *ref, Stream *str, GBool inlineImg);
  void doForm(Object *str);
  void doForm1(Object *str, Dict *resDict, double *matrix, double *bbox);

  // in-line image operators
  void opBeginImage(Object args[], int numArgs);
  Stream *buildImageStream();
  void opImageData(Object args[], int numArgs);
  void opEndImage(Object args[], int numArgs);

  // type 3 font operators
  void opSetCharWidth(Object args[], int numArgs);
  void opSetCacheDevice(Object args[], int numArgs);

  // compatibility operators
  void opBeginIgnoreUndef(Object args[], int numArgs);
  void opEndIgnoreUndef(Object args[], int numArgs);

  // marked content operators
  void opBeginMarkedContent(Object args[], int numArgs);
  void opEndMarkedContent(Object args[], int numArgs);
  void opMarkPoint(Object args[], int numArgs);

  void pushResources(Dict *resDict);
  void popResources();
};

#endif
