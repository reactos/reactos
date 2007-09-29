//========================================================================
//
// Gfx.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include "goo/gmem.h"
#include "goo/GooTimer.h"
#include "goo/GooHash.h"
#include "GlobalParams.h"
#include "CharTypes.h"
#include "Object.h"
#include "Array.h"
#include "Dict.h"
#include "Stream.h"
#include "Lexer.h"
#include "Parser.h"
#include "GfxFont.h"
#include "GfxState.h"
#include "OutputDev.h"
#include "Page.h"
#include "Error.h"
#include "Gfx.h"
#include "ProfileData.h"
#include "UGooString.h"

// the MSVC math.h doesn't define this
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//------------------------------------------------------------------------
// constants
//------------------------------------------------------------------------

// Max recursive depth for a function shading fill.
#define functionMaxDepth 6

// Max delta allowed in any color component for a function shading fill.
#define functionColorDelta (dblToCol(1 / 256.0))

// Max number of splits along the t axis for an axial shading fill.
#define axialMaxSplits 256

// Max delta allowed in any color component for an axial shading fill.
#define axialColorDelta (dblToCol(1 / 256.0))

// Max number of splits along the t axis for a radial shading fill.
#define radialMaxSplits 256

// Max delta allowed in any color component for a radial shading fill.
#define radialColorDelta (dblToCol(1 / 256.0))

// Max recursive depth for a Gouraud triangle shading fill.
#define gouraudMaxDepth 4

// Max delta allowed in any color component for a Gouraud triangle
// shading fill.
#define gouraudColorDelta (dblToCol(1 / 256.0))

// Max recursive depth for a patch mesh shading fill.
#define patchMaxDepth 6

// Max delta allowed in any color component for a patch mesh shading
// fill.
#define patchColorDelta (dblToCol(1 / 256.0))

//------------------------------------------------------------------------
// Operator table
//------------------------------------------------------------------------

#ifdef WIN32 // this works around a bug in the VC7 compiler
#  pragma optimize("",off)
#endif

Operator Gfx::opTab[] = {
  {"\"",  3, {tchkNum,    tchkNum,    tchkString},
          &Gfx::opMoveSetShowText},
  {"'",   1, {tchkString},
          &Gfx::opMoveShowText},
  {"B",   0, {tchkNone},
          &Gfx::opFillStroke},
  {"B*",  0, {tchkNone},
          &Gfx::opEOFillStroke},
  {"BDC", 2, {tchkName,   tchkProps},
          &Gfx::opBeginMarkedContent},
  {"BI",  0, {tchkNone},
          &Gfx::opBeginImage},
  {"BMC", 1, {tchkName},
          &Gfx::opBeginMarkedContent},
  {"BT",  0, {tchkNone},
          &Gfx::opBeginText},
  {"BX",  0, {tchkNone},
          &Gfx::opBeginIgnoreUndef},
  {"CS",  1, {tchkName},
          &Gfx::opSetStrokeColorSpace},
  {"DP",  2, {tchkName,   tchkProps},
          &Gfx::opMarkPoint},
  {"Do",  1, {tchkName},
          &Gfx::opXObject},
  {"EI",  0, {tchkNone},
          &Gfx::opEndImage},
  {"EMC", 0, {tchkNone},
          &Gfx::opEndMarkedContent},
  {"ET",  0, {tchkNone},
          &Gfx::opEndText},
  {"EX",  0, {tchkNone},
          &Gfx::opEndIgnoreUndef},
  {"F",   0, {tchkNone},
          &Gfx::opFill},
  {"G",   1, {tchkNum},
          &Gfx::opSetStrokeGray},
  {"ID",  0, {tchkNone},
          &Gfx::opImageData},
  {"J",   1, {tchkInt},
          &Gfx::opSetLineCap},
  {"K",   4, {tchkNum,    tchkNum,    tchkNum,    tchkNum},
          &Gfx::opSetStrokeCMYKColor},
  {"M",   1, {tchkNum},
          &Gfx::opSetMiterLimit},
  {"MP",  1, {tchkName},
          &Gfx::opMarkPoint},
  {"Q",   0, {tchkNone},
          &Gfx::opRestore},
  {"RG",  3, {tchkNum,    tchkNum,    tchkNum},
          &Gfx::opSetStrokeRGBColor},
  {"S",   0, {tchkNone},
          &Gfx::opStroke},
  {"SC",  -4, {tchkNum,   tchkNum,    tchkNum,    tchkNum},
          &Gfx::opSetStrokeColor},
  {"SCN", -5, {tchkSCN,   tchkSCN,    tchkSCN,    tchkSCN,
	       tchkSCN},
          &Gfx::opSetStrokeColorN},
  {"T*",  0, {tchkNone},
          &Gfx::opTextNextLine},
  {"TD",  2, {tchkNum,    tchkNum},
          &Gfx::opTextMoveSet},
  {"TJ",  1, {tchkArray},
          &Gfx::opShowSpaceText},
  {"TL",  1, {tchkNum},
          &Gfx::opSetTextLeading},
  {"Tc",  1, {tchkNum},
          &Gfx::opSetCharSpacing},
  {"Td",  2, {tchkNum,    tchkNum},
          &Gfx::opTextMove},
  {"Tf",  2, {tchkName,   tchkNum},
          &Gfx::opSetFont},
  {"Tj",  1, {tchkString},
          &Gfx::opShowText},
  {"Tm",  6, {tchkNum,    tchkNum,    tchkNum,    tchkNum,
	      tchkNum,    tchkNum},
          &Gfx::opSetTextMatrix},
  {"Tr",  1, {tchkInt},
          &Gfx::opSetTextRender},
  {"Ts",  1, {tchkNum},
          &Gfx::opSetTextRise},
  {"Tw",  1, {tchkNum},
          &Gfx::opSetWordSpacing},
  {"Tz",  1, {tchkNum},
          &Gfx::opSetHorizScaling},
  {"W",   0, {tchkNone},
          &Gfx::opClip},
  {"W*",  0, {tchkNone},
          &Gfx::opEOClip},
  {"b",   0, {tchkNone},
          &Gfx::opCloseFillStroke},
  {"b*",  0, {tchkNone},
          &Gfx::opCloseEOFillStroke},
  {"c",   6, {tchkNum,    tchkNum,    tchkNum,    tchkNum,
	      tchkNum,    tchkNum},
          &Gfx::opCurveTo},
  {"cm",  6, {tchkNum,    tchkNum,    tchkNum,    tchkNum,
	      tchkNum,    tchkNum},
          &Gfx::opConcat},
  {"cs",  1, {tchkName},
          &Gfx::opSetFillColorSpace},
  {"d",   2, {tchkArray,  tchkNum},
          &Gfx::opSetDash},
  {"d0",  2, {tchkNum,    tchkNum},
          &Gfx::opSetCharWidth},
  {"d1",  6, {tchkNum,    tchkNum,    tchkNum,    tchkNum,
	      tchkNum,    tchkNum},
          &Gfx::opSetCacheDevice},
  {"f",   0, {tchkNone},
          &Gfx::opFill},
  {"f*",  0, {tchkNone},
          &Gfx::opEOFill},
  {"g",   1, {tchkNum},
          &Gfx::opSetFillGray},
  {"gs",  1, {tchkName},
          &Gfx::opSetExtGState},
  {"h",   0, {tchkNone},
          &Gfx::opClosePath},
  {"i",   1, {tchkNum},
          &Gfx::opSetFlat},
  {"j",   1, {tchkInt},
          &Gfx::opSetLineJoin},
  {"k",   4, {tchkNum,    tchkNum,    tchkNum,    tchkNum},
          &Gfx::opSetFillCMYKColor},
  {"l",   2, {tchkNum,    tchkNum},
          &Gfx::opLineTo},
  {"m",   2, {tchkNum,    tchkNum},
          &Gfx::opMoveTo},
  {"n",   0, {tchkNone},
          &Gfx::opEndPath},
  {"q",   0, {tchkNone},
          &Gfx::opSave},
  {"re",  4, {tchkNum,    tchkNum,    tchkNum,    tchkNum},
          &Gfx::opRectangle},
  {"rg",  3, {tchkNum,    tchkNum,    tchkNum},
          &Gfx::opSetFillRGBColor},
  {"ri",  1, {tchkName},
          &Gfx::opSetRenderingIntent},
  {"s",   0, {tchkNone},
          &Gfx::opCloseStroke},
  {"sc",  -4, {tchkNum,   tchkNum,    tchkNum,    tchkNum},
          &Gfx::opSetFillColor},
  {"scn", -5, {tchkSCN,   tchkSCN,    tchkSCN,    tchkSCN,
	       tchkSCN},
          &Gfx::opSetFillColorN},
  {"sh",  1, {tchkName},
          &Gfx::opShFill},
  {"v",   4, {tchkNum,    tchkNum,    tchkNum,    tchkNum},
          &Gfx::opCurveTo1},
  {"w",   1, {tchkNum},
          &Gfx::opSetLineWidth},
  {"y",   4, {tchkNum,    tchkNum,    tchkNum,    tchkNum},
          &Gfx::opCurveTo2},
};

#ifdef WIN32 // this works around a bug in the VC7 compiler
#  pragma optimize("",on)
#endif

#define numOps (sizeof(opTab) / sizeof(Operator))

//------------------------------------------------------------------------
// GfxResources
//------------------------------------------------------------------------

GfxResources::GfxResources(XRef *xref, Dict *resDict, GfxResources *nextA) {
  Object obj1, obj2;
  Ref r;

  if (resDict) {

    // build font dictionary
    fonts = NULL;
    resDict->lookupNF("Font", &obj1);
    if (obj1.isRef()) {
      obj1.fetch(xref, &obj2);
      if (obj2.isDict()) {
	r = obj1.getRef();
	fonts = new GfxFontDict(xref, &r, obj2.getDict());
      }
      obj2.free();
    } else if (obj1.isDict()) {
      fonts = new GfxFontDict(xref, NULL, obj1.getDict());
    }
    obj1.free();

    // get XObject dictionary
    resDict->lookup("XObject", &xObjDict);

    // get color space dictionary
    resDict->lookup("ColorSpace", &colorSpaceDict);

    // get pattern dictionary
    resDict->lookup("Pattern", &patternDict);

    // get shading dictionary
    resDict->lookup("Shading", &shadingDict);

    // get graphics state parameter dictionary
    resDict->lookup("ExtGState", &gStateDict);

  } else {
    fonts = NULL;
    xObjDict.initNull();
    colorSpaceDict.initNull();
    patternDict.initNull();
    shadingDict.initNull();
    gStateDict.initNull();
  }

  next = nextA;
}

GfxResources::~GfxResources() {
  if (fonts) {
    delete fonts;
  }
  xObjDict.free();
  colorSpaceDict.free();
  patternDict.free();
  shadingDict.free();
  gStateDict.free();
}

GfxFont *GfxResources::lookupFont(char *name) {
  GfxFont *font;
  GfxResources *resPtr;

  for (resPtr = this; resPtr; resPtr = resPtr->next) {
    if (resPtr->fonts) {
      if ((font = resPtr->fonts->lookup(name)))
	return font;
    }
  }
  error(-1, "Unknown font tag '%s'", name);
  return NULL;
}

GBool GfxResources::lookupXObject(char *name, Object *obj) {
  GfxResources *resPtr;

  for (resPtr = this; resPtr; resPtr = resPtr->next) {
    if (resPtr->xObjDict.isDict()) {
      if (!resPtr->xObjDict.dictLookup(name, obj)->isNull())
	return gTrue;
      obj->free();
    }
  }
  error(-1, "XObject '%s' is unknown", name);
  return gFalse;
}

GBool GfxResources::lookupXObjectNF(char *name, Object *obj) {
  GfxResources *resPtr;

  for (resPtr = this; resPtr; resPtr = resPtr->next) {
    if (resPtr->xObjDict.isDict()) {
      if (!resPtr->xObjDict.dictLookupNF(name, obj)->isNull())
	return gTrue;
      obj->free();
    }
  }
  error(-1, "XObject '%s' is unknown", name);
  return gFalse;
}

void GfxResources::lookupColorSpace(char *name, Object *obj) {
  GfxResources *resPtr;

  for (resPtr = this; resPtr; resPtr = resPtr->next) {
    if (resPtr->colorSpaceDict.isDict()) {
      if (!resPtr->colorSpaceDict.dictLookup(name, obj)->isNull()) {
	return;
      }
      obj->free();
    }
  }
  obj->initNull();
}

GfxPattern *GfxResources::lookupPattern(char *name) {
  GfxResources *resPtr;
  GfxPattern *pattern;
  Object obj;

  for (resPtr = this; resPtr; resPtr = resPtr->next) {
    if (resPtr->patternDict.isDict()) {
      if (!resPtr->patternDict.dictLookup(name, &obj)->isNull()) {
	pattern = GfxPattern::parse(&obj);
	obj.free();
	return pattern;
      }
      obj.free();
    }
  }
  error(-1, "Unknown pattern '%s'", name);
  return NULL;
}

GfxShading *GfxResources::lookupShading(char *name) {
  GfxResources *resPtr;
  GfxShading *shading;
  Object obj;

  for (resPtr = this; resPtr; resPtr = resPtr->next) {
    if (resPtr->shadingDict.isDict()) {
      if (!resPtr->shadingDict.dictLookup(name, &obj)->isNull()) {
	shading = GfxShading::parse(&obj);
	obj.free();
	return shading;
      }
      obj.free();
    }
  }
  error(-1, "Unknown shading '%s'", name);
  return NULL;
}

GBool GfxResources::lookupGState(char *name, Object *obj) {
  GfxResources *resPtr;

  for (resPtr = this; resPtr; resPtr = resPtr->next) {
    if (resPtr->gStateDict.isDict()) {
      if (!resPtr->gStateDict.dictLookup(name, obj)->isNull()) {
	return gTrue;
      }
      obj->free();
    }
  }
  error(-1, "ExtGState '%s' is unknown", name);
  return gFalse;
}

//------------------------------------------------------------------------
// Gfx
//------------------------------------------------------------------------

Gfx::Gfx(XRef *xrefA, OutputDev *outA, int pageNum, Dict *resDict,
	 double hDPI, double vDPI, PDFRectangle *box,
	 PDFRectangle *cropBox, int rotate,
	 GBool (*abortCheckCbkA)(void *data),
	 void *abortCheckCbkDataA) {
  int i;

  xref = xrefA;
  subPage = gFalse;
  printCommands = globalParams->getPrintCommands();
  profileCommands = globalParams->getProfileCommands();

  // start the resource stack
  res = new GfxResources(xref, resDict, NULL);

  // initialize
  out = outA;
  state = new GfxState(hDPI, vDPI, box, rotate, out->upsideDown());
  fontChanged = gFalse;
  clip = clipNone;
  ignoreUndef = 0;
  out->startPage(pageNum, state);
  out->setDefaultCTM(state->getCTM());
  out->updateAll(state);
  for (i = 0; i < 6; ++i) {
    baseMatrix[i] = state->getCTM()[i];
  }
  formDepth = 0;
  abortCheckCbk = abortCheckCbkA;
  abortCheckCbkData = abortCheckCbkDataA;

  // set crop box
  if (cropBox) {
    state->moveTo(cropBox->x1, cropBox->y1);
    state->lineTo(cropBox->x2, cropBox->y1);
    state->lineTo(cropBox->x2, cropBox->y2);
    state->lineTo(cropBox->x1, cropBox->y2);
    state->closePath();
    state->clip();
    out->clip(state);
    state->clearPath();
  }
}

Gfx::Gfx(XRef *xrefA, OutputDev *outA, Dict *resDict,
	 PDFRectangle *box, PDFRectangle *cropBox,
	 GBool (*abortCheckCbkA)(void *data),
	 void *abortCheckCbkDataA) {
  int i;

  xref = xrefA;
  subPage = gTrue;
  printCommands = globalParams->getPrintCommands();

  // start the resource stack
  res = new GfxResources(xref, resDict, NULL);

  // initialize
  out = outA;
  state = new GfxState(72, 72, box, 0, gFalse);
  fontChanged = gFalse;
  clip = clipNone;
  ignoreUndef = 0;
  for (i = 0; i < 6; ++i) {
    baseMatrix[i] = state->getCTM()[i];
  }
  formDepth = 0;
  abortCheckCbk = abortCheckCbkA;
  abortCheckCbkData = abortCheckCbkDataA;

  // set crop box
  if (cropBox) {
    state->moveTo(cropBox->x1, cropBox->y1);
    state->lineTo(cropBox->x2, cropBox->y1);
    state->lineTo(cropBox->x2, cropBox->y2);
    state->lineTo(cropBox->x1, cropBox->y2);
    state->closePath();
    state->clip();
    out->clip(state);
    state->clearPath();
  }
}

Gfx::~Gfx() {
  while (state && state->hasSaves()) {
    restoreState();
  }
  if (!subPage) {
    out->endPage();
  }
  while (res) {
    popResources();
  }
  delete state;
}

void Gfx::display(Object *obj, GBool topLevel) {
  Object obj2;
  int i;

  if (obj->isArray()) {
    for (i = 0; i < obj->arrayGetLength(); ++i) {
      obj->arrayGet(i, &obj2);
      if (!obj2.isStream()) {
        error(-1, "Weird page contents");
        obj2.free();
        return;
      }
      obj2.free();
    }
  } else if (!obj->isStream()) {
    error(-1, "Weird page contents");
    return;
  }
  parser = new Parser(xref, new Lexer(xref, obj));
  go(topLevel);
  delete parser;
  parser = NULL;
}

void Gfx::go(GBool topLevel) {
  Object obj;
  Object args[maxArgs];
  int numArgs, i;
  int lastAbortCheck;
#ifdef HAVE_GETTIMEOFDAY
  GooTimer *timer;
#endif

  // scan a sequence of objects
  updateLevel = lastAbortCheck = 0;
  numArgs = 0;
  parser->getObj(&obj);
  while (!obj.isEOF()) {

    // got a command - execute it
    if (obj.isCmd()) {
      if (printCommands) {
        obj.print(stdout);
        for (i = 0; i < numArgs; ++i) {
          printf(" ");
          args[i].print(stdout);
        }
        printf("\n");
        fflush(stdout);
      }
#ifdef HAVE_GETTIMEOFDAY
      if (profileCommands) 
        timer = new GooTimer ();
#endif

      // Run the operation
      execOp(&obj, args, numArgs);

#ifdef HAVE_GETTIMEOFDAY
      // Update the profile information
      if (profileCommands) {
        GooHash *hash;

        hash = out->getProfileHash ();
        if (hash) {
          GooString *cmd_g;
          ProfileData *data_p;

          cmd_g = new GooString (obj.getCmd());
          data_p = (ProfileData *)hash->lookup (cmd_g);
          if (data_p == NULL) {
            data_p = new ProfileData();
            hash->add (cmd_g, data_p);
          }
          
          data_p->addElement (timer->getElapsed ());
        }
        delete (timer);
      }
#endif
      obj.free();
      for (i = 0; i < numArgs; ++i)
        args[i].free();
      numArgs = 0;

      // periodically update display
      if (++updateLevel >= 20000) {
        out->dump();
        updateLevel = 0;
      }

      // check for an abort
      if (abortCheckCbk) {
        if (updateLevel - lastAbortCheck > 10) {
          if ((*abortCheckCbk)(abortCheckCbkData)) {
            break;
          }
          lastAbortCheck = updateLevel;
        }
      }

    // got an argument - save it
    } else if (numArgs < maxArgs) {
      args[numArgs++] = obj;

    // too many arguments - something is wrong
    } else {
      error(getPos(), "Too many args in content stream");
      if (printCommands) {
        printf("throwing away arg: ");
        obj.print(stdout);
        printf("\n");
        fflush(stdout);
      }
      obj.free();
    }

    // grab the next object
    parser->getObj(&obj);
  }
  obj.free();

  // args at end with no command
  if (numArgs > 0) {
    error(getPos(), "Leftover args in content stream");
    if (printCommands) {
      printf("%d leftovers:", numArgs);
      for (i = 0; i < numArgs; ++i) {
        printf(" ");
        args[i].print(stdout);
      }
      printf("\n");
      fflush(stdout);
    }
    for (i = 0; i < numArgs; ++i)
      args[i].free();
  }

  // update display
  if (topLevel && updateLevel > 0) {
    out->dump();
  }
}

void Gfx::execOp(Object *cmd, Object args[], int numArgs) {
  Operator *op;
  char *name;
  Object *argPtr;
  int i;

  // find operator
  name = cmd->getCmd()->getCString();
  if (!(op = findOp(name))) {
    if (ignoreUndef == 0)
      error(getPos(), "Unknown operator '%s'", name);
    return;
  }

  // type check args
  argPtr = args;
  if (op->numArgs >= 0) {
    if (numArgs < op->numArgs) {
      error(getPos(), "Too few (%d) args to '%s' operator", numArgs, name);
      return;
    }
    if (numArgs > op->numArgs) {
#if 0
      error(getPos(), "Too many (%d) args to '%s' operator", numArgs, name);
#endif
      argPtr += numArgs - op->numArgs;
      numArgs = op->numArgs;
    }
  } else {
    if (numArgs > -op->numArgs) {
      error(getPos(), "Too many (%d) args to '%s' operator",
        numArgs, name);
      return;
    }
  }
  for (i = 0; i < numArgs; ++i) {
    if (!checkArg(&argPtr[i], op->tchk[i])) {
      error(getPos(), "Arg #%d to '%s' operator is wrong type (%s)",
        i, name, argPtr[i].getTypeName());
      return;
    }
  }

  // do it
  (this->*op->func)(argPtr, numArgs);
}

Operator *Gfx::findOp(char *name) {
  int a, b, m, cmp;

  a = -1;
  b = numOps;
  // invariant: opTab[a] < name < opTab[b]
  while (b - a > 1) {
    m = (a + b) / 2;
    cmp = strcmp(opTab[m].name, name);
    if (cmp < 0)
      a = m;
    else if (cmp > 0)
      b = m;
    else
      a = b = m;
  }
  if (cmp != 0)
    return NULL;
  return &opTab[a];
}

GBool Gfx::checkArg(Object *arg, TchkType type) {
  switch (type) {
  case tchkBool:   return arg->isBool();
  case tchkInt:    return arg->isInt();
  case tchkNum:    return arg->isNum();
  case tchkString: return arg->isString();
  case tchkName:   return arg->isName();
  case tchkArray:  return arg->isArray();
  case tchkProps:  return arg->isDict() || arg->isName();
  case tchkSCN:    return arg->isNum() || arg->isName();
  case tchkNone:   return gFalse;
  }
  return gFalse;
}

int Gfx::getPos() {
  return parser ? parser->getPos() : -1;
}

//------------------------------------------------------------------------
// graphics state operators
//------------------------------------------------------------------------

void Gfx::opSave(Object args[], int numArgs) {
  saveState();
}

void Gfx::opRestore(Object args[], int numArgs) {
  restoreState();
}

void Gfx::opConcat(Object args[], int numArgs) {
  state->concatCTM(args[0].getNum(), args[1].getNum(),
                   args[2].getNum(), args[3].getNum(),
                   args[4].getNum(), args[5].getNum());
  out->updateCTM(state, args[0].getNum(), args[1].getNum(),
                 args[2].getNum(), args[3].getNum(),
                 args[4].getNum(), args[5].getNum());
  fontChanged = gTrue;
}

void Gfx::opSetDash(Object args[], int numArgs) {
  Array *a;
  int length;
  Object obj;
  double *dash;
  int i;

  a = args[0].getArray();
  length = a->getLength();
  if (length == 0) {
    dash = NULL;
  } else {
    dash = (double *)gmallocn(length, sizeof(double));
    for (i = 0; i < length; ++i) {
      dash[i] = a->get(i, &obj)->getNum();
      obj.free();
    }
  }
  state->setLineDash(dash, length, args[1].getNum());
  out->updateLineDash(state);
}

void Gfx::opSetFlat(Object args[], int numArgs) {
  state->setFlatness((int)args[0].getNum());
  out->updateFlatness(state);
}

void Gfx::opSetLineJoin(Object args[], int numArgs) {
  state->setLineJoin(args[0].getInt());
  out->updateLineJoin(state);
}

void Gfx::opSetLineCap(Object args[], int numArgs) {
  state->setLineCap(args[0].getInt());
  out->updateLineCap(state);
}

void Gfx::opSetMiterLimit(Object args[], int numArgs) {
  state->setMiterLimit(args[0].getNum());
  out->updateMiterLimit(state);
}

void Gfx::opSetLineWidth(Object args[], int numArgs) {
  state->setLineWidth(args[0].getNum());
  out->updateLineWidth(state);
}

void Gfx::opSetExtGState(Object args[], int numArgs) {
  Object obj1, obj2;
  GfxBlendMode mode;
  GBool haveFillOP;

  if (!res->lookupGState(args[0].getNameC(), &obj1)) {
    return;
  }
  if (!obj1.isDict()) {
    error(getPos(), "ExtGState '%s' is wrong type", args[0].getNameC());
    obj1.free();
    return;
  }

  // transparency support: blend mode, fill/stroke opacity
  if (!obj1.dictLookup("BM", &obj2)->isNull()) {
    if (state->parseBlendMode(&obj2, &mode)) {
      state->setBlendMode(mode);
      out->updateBlendMode(state);
    } else {
      error(getPos(), "Invalid blend mode in ExtGState");
    }
  }
  obj2.free();
  if (obj1.dictLookup("ca", &obj2)->isNum()) {
    state->setFillOpacity(obj2.getNum());
    out->updateFillOpacity(state);
  }
  obj2.free();
  if (obj1.dictLookup("CA", &obj2)->isNum()) {
    state->setStrokeOpacity(obj2.getNum());
    out->updateStrokeOpacity(state);
  }
  obj2.free();

  // fill/stroke overprint
  if ((haveFillOP = (obj1.dictLookup("op", &obj2)->isBool()))) {
    state->setFillOverprint(obj2.getBool());
    out->updateFillOverprint(state);
  }
  obj2.free();
  if (obj1.dictLookup("OP", &obj2)->isBool()) {
    state->setStrokeOverprint(obj2.getBool());
    out->updateStrokeOverprint(state);
    if (!haveFillOP) {
      state->setFillOverprint(obj2.getBool());
      out->updateFillOverprint(state);
    }
  }
  obj2.free();

  obj1.free();
}

void Gfx::opSetRenderingIntent(Object args[], int numArgs) {
}

//------------------------------------------------------------------------
// color operators
//------------------------------------------------------------------------

void Gfx::opSetFillGray(Object args[], int numArgs) {
  GfxColor color;

  state->setFillPattern(NULL);
  state->setFillColorSpace(new GfxDeviceGrayColorSpace());
  out->updateFillColorSpace(state);
  color.c[0] = dblToCol(args[0].getNum());
  state->setFillColor(&color);
  out->updateFillColor(state);
}

void Gfx::opSetStrokeGray(Object args[], int numArgs) {
  GfxColor color;

  state->setStrokePattern(NULL);
  state->setStrokeColorSpace(new GfxDeviceGrayColorSpace());
  out->updateStrokeColorSpace(state);
  color.c[0] = dblToCol(args[0].getNum());
  state->setStrokeColor(&color);
  out->updateStrokeColor(state);
}

void Gfx::opSetFillCMYKColor(Object args[], int numArgs) {
  GfxColor color;
  int i;

  state->setFillPattern(NULL);
  state->setFillColorSpace(new GfxDeviceCMYKColorSpace());
  out->updateFillColorSpace(state);
  for (i = 0; i < 4; ++i) {
    color.c[i] = dblToCol(args[i].getNum());
  }
  state->setFillColor(&color);
  out->updateFillColor(state);
}

void Gfx::opSetStrokeCMYKColor(Object args[], int numArgs) {
  GfxColor color;
  int i;

  state->setStrokePattern(NULL);
  state->setStrokeColorSpace(new GfxDeviceCMYKColorSpace());
  out->updateStrokeColorSpace(state);
  for (i = 0; i < 4; ++i) {
    color.c[i] = dblToCol(args[i].getNum());
  }
  state->setStrokeColor(&color);
  out->updateStrokeColor(state);
}

void Gfx::opSetFillRGBColor(Object args[], int numArgs) {
  GfxColor color;
  int i;

  state->setFillPattern(NULL);
  state->setFillColorSpace(new GfxDeviceRGBColorSpace());
  out->updateFillColorSpace(state);
  for (i = 0; i < 3; ++i) {
    color.c[i] = dblToCol(args[i].getNum());
  }
  state->setFillColor(&color);
  out->updateFillColor(state);
}

void Gfx::opSetStrokeRGBColor(Object args[], int numArgs) {
  GfxColor color;
  int i;

  state->setStrokePattern(NULL);
  state->setStrokeColorSpace(new GfxDeviceRGBColorSpace());
  out->updateStrokeColorSpace(state);
  for (i = 0; i < 3; ++i) {
    color.c[i] = dblToCol(args[i].getNum());
  }
  state->setStrokeColor(&color);
  out->updateStrokeColor(state);
}

void Gfx::opSetFillColorSpace(Object args[], int numArgs) {
  Object obj;
  GfxColorSpace *colorSpace;
  GfxColor color;
  int i;

  state->setFillPattern(NULL);
  res->lookupColorSpace(args[0].getNameC(), &obj);
  if (obj.isNull()) {
    colorSpace = GfxColorSpace::parse(&args[0]);
  } else {
    colorSpace = GfxColorSpace::parse(&obj);
  }
  obj.free();
  if (colorSpace) {
    state->setFillColorSpace(colorSpace);
    out->updateFillColorSpace(state);
  } else {
    error(getPos(), "Bad color space (fill)");
  }
  for (i = 0; i < gfxColorMaxComps; ++i) {
    color.c[i] = 0;
  }
  state->setFillColor(&color);
  out->updateFillColor(state);
}

void Gfx::opSetStrokeColorSpace(Object args[], int numArgs) {
  Object obj;
  GfxColorSpace *colorSpace;
  GfxColor color;
  int i;

  state->setStrokePattern(NULL);
  res->lookupColorSpace(args[0].getNameC(), &obj);
  if (obj.isNull()) {
    colorSpace = GfxColorSpace::parse(&args[0]);
  } else {
    colorSpace = GfxColorSpace::parse(&obj);
  }
  obj.free();
  if (colorSpace) {
    state->setStrokeColorSpace(colorSpace);
    out->updateStrokeColorSpace(state);
  } else {
    error(getPos(), "Bad color space (stroke)");
  }
  for (i = 0; i < gfxColorMaxComps; ++i) {
    color.c[i] = 0;
  }
  state->setStrokeColor(&color);
  out->updateStrokeColor(state);
}

void Gfx::opSetFillColor(Object args[], int numArgs) {
  GfxColor color;
  int i;

  state->setFillPattern(NULL);
  for (i = 0; i < numArgs; ++i) {
    color.c[i] = dblToCol(args[i].getNum());
  }
  state->setFillColor(&color);
  out->updateFillColor(state);
}

void Gfx::opSetStrokeColor(Object args[], int numArgs) {
  GfxColor color;
  int i;

  state->setStrokePattern(NULL);
  for (i = 0; i < numArgs; ++i) {
    color.c[i] = dblToCol(args[i].getNum());
  }
  state->setStrokeColor(&color);
  out->updateStrokeColor(state);
}

void Gfx::opSetFillColorN(Object args[], int numArgs) {
  GfxColor color;
  GfxPattern *pattern;
  int i;

  if (state->getFillColorSpace()->getMode() == csPattern) {
    if (numArgs > 1) {
      for (i = 0; i < numArgs && i < 4; ++i) {
        if (args[i].isNum()) {
          color.c[i] = dblToCol(args[i].getNum());
        }
      }
      state->setFillColor(&color);
      out->updateFillColor(state);
    }
    if (args[numArgs-1].isName() &&
        (pattern = res->lookupPattern(args[numArgs-1].getNameC()))) {
      state->setFillPattern(pattern);
    }

  } else {
    state->setFillPattern(NULL);
    for (i = 0; i < numArgs && i < 4; ++i) {
      if (args[i].isNum()) {
        color.c[i] = dblToCol(args[i].getNum());
      }
    }
    state->setFillColor(&color);
    out->updateFillColor(state);
  }
}

void Gfx::opSetStrokeColorN(Object args[], int numArgs) {
  GfxColor color;
  GfxPattern *pattern;
  int i;

  if (state->getStrokeColorSpace()->getMode() == csPattern) {
    if (numArgs > 1) {
      for (i = 0; i < numArgs && i < 4; ++i) {
        if (args[i].isNum()) {
          color.c[i] = dblToCol(args[i].getNum());
        }
      }
      state->setStrokeColor(&color);
      out->updateStrokeColor(state);
    }
    if (args[numArgs-1].isName() &&
        (pattern = res->lookupPattern(args[numArgs-1].getNameC()))) {
      state->setStrokePattern(pattern);
    }

  } else {
    state->setStrokePattern(NULL);
    for (i = 0; i < numArgs && i < 4; ++i) {
      if (args[i].isNum()) {
        color.c[i] = dblToCol(args[i].getNum());
      }
    }
    state->setStrokeColor(&color);
    out->updateStrokeColor(state);
  }
}

//------------------------------------------------------------------------
// path segment operators
//------------------------------------------------------------------------

void Gfx::opMoveTo(Object args[], int numArgs) {
  state->moveTo(args[0].getNum(), args[1].getNum());
}

void Gfx::opLineTo(Object args[], int numArgs) {
  if (!state->isCurPt()) {
    error(getPos(), "No current point in lineto");
    return;
  }
  state->lineTo(args[0].getNum(), args[1].getNum());
}

void Gfx::opCurveTo(Object args[], int numArgs) {
  double x1, y1, x2, y2, x3, y3;

  if (!state->isCurPt()) {
    error(getPos(), "No current point in curveto");
    return;
  }
  x1 = args[0].getNum();
  y1 = args[1].getNum();
  x2 = args[2].getNum();
  y2 = args[3].getNum();
  x3 = args[4].getNum();
  y3 = args[5].getNum();
  state->curveTo(x1, y1, x2, y2, x3, y3);
}

void Gfx::opCurveTo1(Object args[], int numArgs) {
  double x1, y1, x2, y2, x3, y3;

  if (!state->isCurPt()) {
    error(getPos(), "No current point in curveto1");
    return;
  }
  x1 = state->getCurX();
  y1 = state->getCurY();
  x2 = args[0].getNum();
  y2 = args[1].getNum();
  x3 = args[2].getNum();
  y3 = args[3].getNum();
  state->curveTo(x1, y1, x2, y2, x3, y3);
}

void Gfx::opCurveTo2(Object args[], int numArgs) {
  double x1, y1, x2, y2, x3, y3;

  if (!state->isCurPt()) {
    error(getPos(), "No current point in curveto2");
    return;
  }
  x1 = args[0].getNum();
  y1 = args[1].getNum();
  x2 = args[2].getNum();
  y2 = args[3].getNum();
  x3 = x2;
  y3 = y2;
  state->curveTo(x1, y1, x2, y2, x3, y3);
}

void Gfx::opRectangle(Object args[], int numArgs) {
  double x, y, w, h;

  x = args[0].getNum();
  y = args[1].getNum();
  w = args[2].getNum();
  h = args[3].getNum();
  state->moveTo(x, y);
  state->lineTo(x + w, y);
  state->lineTo(x + w, y + h);
  state->lineTo(x, y + h);
  state->closePath();
}

void Gfx::opClosePath(Object args[], int numArgs) {
  if (!state->isCurPt()) {
    error(getPos(), "No current point in closepath");
    return;
  }
  state->closePath();
}

//------------------------------------------------------------------------
// path painting operators
//------------------------------------------------------------------------

void Gfx::opEndPath(Object args[], int numArgs) {
  doEndPath();
}

void Gfx::opStroke(Object args[], int numArgs) {
  if (!state->isCurPt()) {
    //error(getPos(), "No path in stroke");
    return;
  }
  if (state->isPath())
    out->stroke(state);
  doEndPath();
}

void Gfx::opCloseStroke(Object args[], int numArgs) {
  if (!state->isCurPt()) {
    //error(getPos(), "No path in closepath/stroke");
    return;
  }
  state->closePath();
  if (state->isPath()) {
    out->stroke(state);
  }
  doEndPath();
}

void Gfx::opFill(Object args[], int numArgs) {
  if (!state->isCurPt()) {
    //error(getPos(), "No path in fill");
    return;
  }
  if (state->isPath()) {
    if (state->getFillColorSpace()->getMode() == csPattern) {
      doPatternFill(gFalse);
    } else {
      out->fill(state);
    }
  }
  doEndPath();
}

void Gfx::opEOFill(Object args[], int numArgs) {
  if (!state->isCurPt()) {
    //error(getPos(), "No path in eofill");
    return;
  }
  if (state->isPath()) {
    if (state->getFillColorSpace()->getMode() == csPattern) {
      doPatternFill(gTrue);
    } else {
      out->eoFill(state);
    }
  }
  doEndPath();
}

void Gfx::opFillStroke(Object args[], int numArgs) {
  if (!state->isCurPt()) {
    //error(getPos(), "No path in fill/stroke");
    return;
  }
  if (state->isPath()) {
    if (state->getFillColorSpace()->getMode() == csPattern) {
      doPatternFill(gFalse);
    } else {
      out->fill(state);
    }
    out->stroke(state);
  }
  doEndPath();
}

void Gfx::opCloseFillStroke(Object args[], int numArgs) {
  if (!state->isCurPt()) {
    //error(getPos(), "No path in closepath/fill/stroke");
    return;
  }
  if (state->isPath()) {
    state->closePath();
    if (state->getFillColorSpace()->getMode() == csPattern) {
      doPatternFill(gFalse);
    } else {
      out->fill(state);
    }
    out->stroke(state);
  }
  doEndPath();
}

void Gfx::opEOFillStroke(Object args[], int numArgs) {
  if (!state->isCurPt()) {
    //error(getPos(), "No path in eofill/stroke");
    return;
  }
  if (state->isPath()) {
    if (state->getFillColorSpace()->getMode() == csPattern) {
      doPatternFill(gTrue);
    } else {
      out->eoFill(state);
    }
    out->stroke(state);
  }
  doEndPath();
}

void Gfx::opCloseEOFillStroke(Object args[], int numArgs) {
  if (!state->isCurPt()) {
    //error(getPos(), "No path in closepath/eofill/stroke");
    return;
  }
  if (state->isPath()) {
    state->closePath();
    if (state->getFillColorSpace()->getMode() == csPattern) {
      doPatternFill(gTrue);
    } else {
      out->eoFill(state);
    }
    out->stroke(state);
  }
  doEndPath();
}

void Gfx::doPatternFill(GBool eoFill) {
  GfxPattern *pattern;

  // this is a bit of a kludge -- patterns can be really slow, so we
  // skip them if we're only doing text extraction, since they almost
  // certainly don't contain any text
  if (!out->needNonText()) {
    return;
  }

  if (!(pattern = state->getFillPattern())) {
    return;
  }
  switch (pattern->getType()) {
  case 1:
    doTilingPatternFill((GfxTilingPattern *)pattern, eoFill);
    break;
  case 2:
    doShadingPatternFill((GfxShadingPattern *)pattern, eoFill);
    break;
  default:
    error(getPos(), "Unimplemented pattern type (%d) in fill",
          pattern->getType());
    break;
  }
}

void Gfx::doTilingPatternFill(GfxTilingPattern *tPat, GBool eoFill) {
  GfxPatternColorSpace *patCS;
  GfxColorSpace *cs;
  GfxPath *savedPath;
  double xMin, yMin, xMax, yMax, x, y, x1, y1;
  double cxMin, cyMin, cxMax, cyMax;
  int xi0, yi0, xi1, yi1, xi, yi;
  double *ctm, *btm, *ptm;
  double m[6], ictm[6], m1[6], imb[6];
  double det;
  double xstep, ystep;
  int i;

  // get color space
  patCS = (GfxPatternColorSpace *)state->getFillColorSpace();

  // construct a (pattern space) -> (current space) transform matrix
  ctm = state->getCTM();
  btm = baseMatrix;
  ptm = tPat->getMatrix();
  // iCTM = invert CTM
  det = 1 / (ctm[0] * ctm[3] - ctm[1] * ctm[2]);
  ictm[0] = ctm[3] * det;
  ictm[1] = -ctm[1] * det;
  ictm[2] = -ctm[2] * det;
  ictm[3] = ctm[0] * det;
  ictm[4] = (ctm[2] * ctm[5] - ctm[3] * ctm[4]) * det;
  ictm[5] = (ctm[1] * ctm[4] - ctm[0] * ctm[5]) * det;
  // m1 = PTM * BTM = PTM * base transform matrix
  m1[0] = ptm[0] * btm[0] + ptm[1] * btm[2];
  m1[1] = ptm[0] * btm[1] + ptm[1] * btm[3];
  m1[2] = ptm[2] * btm[0] + ptm[3] * btm[2];
  m1[3] = ptm[2] * btm[1] + ptm[3] * btm[3];
  m1[4] = ptm[4] * btm[0] + ptm[5] * btm[2] + btm[4];
  m1[5] = ptm[4] * btm[1] + ptm[5] * btm[3] + btm[5];
  // m = m1 * iCTM = (PTM * BTM) * (iCTM)
  m[0] = m1[0] * ictm[0] + m1[1] * ictm[2];
  m[1] = m1[0] * ictm[1] + m1[1] * ictm[3];
  m[2] = m1[2] * ictm[0] + m1[3] * ictm[2];
  m[3] = m1[2] * ictm[1] + m1[3] * ictm[3];
  m[4] = m1[4] * ictm[0] + m1[5] * ictm[2] + ictm[4];
  m[5] = m1[4] * ictm[1] + m1[5] * ictm[3] + ictm[5];

  // construct a (device space) -> (pattern space) transform matrix
  det = 1 / (m1[0] * m1[3] - m1[1] * m1[2]);
  imb[0] = m1[3] * det;
  imb[1] = -m1[1] * det;
  imb[2] = -m1[2] * det;
  imb[3] = m1[0] * det;
  imb[4] = (m1[2] * m1[5] - m1[3] * m1[4]) * det;
  imb[5] = (m1[1] * m1[4] - m1[0] * m1[5]) * det;

  // save current graphics state
  savedPath = state->getPath()->copy();
  saveState();

  // set underlying color space (for uncolored tiling patterns); set
  // various other parameters (stroke color, line width) to match
  // Adobe's behavior
  if (tPat->getPaintType() == 2 && (cs = patCS->getUnder())) {
    state->setFillColorSpace(cs->copy());
    out->updateFillColorSpace(state);
    state->setStrokeColorSpace(cs->copy());
    out->updateStrokeColorSpace(state);
    state->setStrokeColor(state->getFillColor());
  } else {
    state->setFillColorSpace(new GfxDeviceGrayColorSpace());
    out->updateFillColorSpace(state);
    state->setStrokeColorSpace(new GfxDeviceGrayColorSpace());
    out->updateStrokeColorSpace(state);
  }
  state->setFillPattern(NULL);
  out->updateFillColor(state);
  state->setStrokePattern(NULL);
  out->updateStrokeColor(state);
  state->setLineWidth(0);
  out->updateLineWidth(state);

  // clip to current path
  state->clip();
  if (eoFill) {
    out->eoClip(state);
  } else {
    out->clip(state);
  }
  state->clearPath();

  // get the clip region, check for empty
  state->getClipBBox(&cxMin, &cyMin, &cxMax, &cyMax);
  if (cxMin > cxMax || cyMin > cyMax) {
    goto err;
  }

  // transform clip region bbox to pattern space
  xMin = xMax = cxMin * imb[0] + cyMin * imb[2] + imb[4];
  yMin = yMax = cxMin * imb[1] + cyMin * imb[3] + imb[5];
  x1 = cxMin * imb[0] + cyMax * imb[2] + imb[4];
  y1 = cxMin * imb[1] + cyMax * imb[3] + imb[5];
  if (x1 < xMin) {
    xMin = x1;
  } else if (x1 > xMax) {
    xMax = x1;
  }
  if (y1 < yMin) {
    yMin = y1;
  } else if (y1 > yMax) {
    yMax = y1;
  }
  x1 = cxMax * imb[0] + cyMin * imb[2] + imb[4];
  y1 = cxMax * imb[1] + cyMin * imb[3] + imb[5];
  if (x1 < xMin) {
    xMin = x1;
  } else if (x1 > xMax) {
    xMax = x1;
  }
  if (y1 < yMin) {
    yMin = y1;
  } else if (y1 > yMax) {
    yMax = y1;
  }
  x1 = cxMax * imb[0] + cyMax * imb[2] + imb[4];
  y1 = cxMax * imb[1] + cyMax * imb[3] + imb[5];
  if (x1 < xMin) {
    xMin = x1;
  } else if (x1 > xMax) {
    xMax = x1;
  }
  if (y1 < yMin) {
    yMin = y1;
  } else if (y1 > yMax) {
    yMax = y1;
  }

  // draw the pattern
  //~ this should treat negative steps differently -- start at right/top
  //~ edge instead of left/bottom (?)
  xstep = fabs(tPat->getXStep());
  ystep = fabs(tPat->getYStep());
  xi0 = (int)floor((xMin - tPat->getBBox()[0]) / xstep);
  xi1 = (int)ceil((xMax - tPat->getBBox()[0]) / xstep);
  yi0 = (int)floor((yMin - tPat->getBBox()[1]) / ystep);
  yi1 = (int)ceil((yMax - tPat->getBBox()[1]) / ystep);
  for (i = 0; i < 4; ++i) {
    m1[i] = m[i];
  }
  if (out->useTilingPatternFill()) {
    m1[4] = m[4];
    m1[5] = m[5];
    out->tilingPatternFill(state, tPat->getContentStream(),
                           tPat->getPaintType(), tPat->getResDict(),
                           m1, tPat->getBBox(),
                           xi0, yi0, xi1, yi1, xstep, ystep);
  } else {
    for (yi = yi0; yi < yi1; ++yi) {
      for (xi = xi0; xi < xi1; ++xi) {
        x = xi * xstep;
        y = yi * ystep;
        m1[4] = x * m[0] + y * m[2] + m[4];
        m1[5] = x * m[1] + y * m[3] + m[5];
        doForm1(tPat->getContentStream(), tPat->getResDict(),
                m1, tPat->getBBox());
      }
    }
  }

  // restore graphics state
 err:
  restoreState();
  state->setPath(savedPath);
}

void Gfx::doShadingPatternFill(GfxShadingPattern *sPat, GBool eoFill) {
  GfxShading *shading;
  GfxPath *savedPath;
  double *ctm, *btm, *ptm;
  double m[6], ictm[6], m1[6];
  double xMin, yMin, xMax, yMax;
  double det;

  shading = sPat->getShading();

  // save current graphics state
  savedPath = state->getPath()->copy();
  saveState();

  // clip to bbox
  if (shading->getHasBBox()) {
    shading->getBBox(&xMin, &yMin, &xMax, &yMax);
    state->moveTo(xMin, yMin);
    state->lineTo(xMax, yMin);
    state->lineTo(xMax, yMax);
    state->lineTo(xMin, yMax);
    state->closePath();
    state->clip();
    out->clip(state);
    state->setPath(savedPath->copy());
  }

  // clip to current path
  state->clip();
  if (eoFill) {
    out->eoClip(state);
  } else {
    out->clip(state);
  }

  // set the color space
  state->setFillColorSpace(shading->getColorSpace()->copy());
  out->updateFillColorSpace(state);

  // background color fill
  if (shading->getHasBackground()) {
    state->setFillColor(shading->getBackground());
    out->updateFillColor(state);
    out->fill(state);
  }
  state->clearPath();

  // construct a (pattern space) -> (current space) transform matrix
  ctm = state->getCTM();
  btm = baseMatrix;
  ptm = sPat->getMatrix();
  // iCTM = invert CTM
  det = 1 / (ctm[0] * ctm[3] - ctm[1] * ctm[2]);
  ictm[0] = ctm[3] * det;
  ictm[1] = -ctm[1] * det;
  ictm[2] = -ctm[2] * det;
  ictm[3] = ctm[0] * det;
  ictm[4] = (ctm[2] * ctm[5] - ctm[3] * ctm[4]) * det;
  ictm[5] = (ctm[1] * ctm[4] - ctm[0] * ctm[5]) * det;
  // m1 = PTM * BTM = PTM * base transform matrix
  m1[0] = ptm[0] * btm[0] + ptm[1] * btm[2];
  m1[1] = ptm[0] * btm[1] + ptm[1] * btm[3];
  m1[2] = ptm[2] * btm[0] + ptm[3] * btm[2];
  m1[3] = ptm[2] * btm[1] + ptm[3] * btm[3];
  m1[4] = ptm[4] * btm[0] + ptm[5] * btm[2] + btm[4];
  m1[5] = ptm[4] * btm[1] + ptm[5] * btm[3] + btm[5];
  // m = m1 * iCTM = (PTM * BTM) * (iCTM)
  m[0] = m1[0] * ictm[0] + m1[1] * ictm[2];
  m[1] = m1[0] * ictm[1] + m1[1] * ictm[3];
  m[2] = m1[2] * ictm[0] + m1[3] * ictm[2];
  m[3] = m1[2] * ictm[1] + m1[3] * ictm[3];
  m[4] = m1[4] * ictm[0] + m1[5] * ictm[2] + ictm[4];
  m[5] = m1[4] * ictm[1] + m1[5] * ictm[3] + ictm[5];

  // set the new matrix
  state->concatCTM(m[0], m[1], m[2], m[3], m[4], m[5]);
  out->updateCTM(state, m[0], m[1], m[2], m[3], m[4], m[5]);

  // do shading type-specific operations
  switch (shading->getType()) {
  case 1:
    doFunctionShFill((GfxFunctionShading *)shading);
    break;
  case 2:
    doAxialShFill((GfxAxialShading *)shading);
    break;
  case 3:
    doRadialShFill((GfxRadialShading *)shading);
    break;
  case 4:
  case 5:
    doGouraudTriangleShFill((GfxGouraudTriangleShading *)shading);
    break;
  case 6:
  case 7:
    doPatchMeshShFill((GfxPatchMeshShading *)shading);
    break;
  }

  // restore graphics state
  restoreState();
  state->setPath(savedPath);
}

void Gfx::opShFill(Object args[], int numArgs) {
  GfxShading *shading;
  GfxPath *savedPath;
  double xMin, yMin, xMax, yMax;

  if (!(shading = res->lookupShading(args[0].getNameC()))) {
    return;
  }

  // save current graphics state
  savedPath = state->getPath()->copy();
  saveState();

  // clip to bbox
  if (shading->getHasBBox()) {
    shading->getBBox(&xMin, &yMin, &xMax, &yMax);
    state->moveTo(xMin, yMin);
    state->lineTo(xMax, yMin);
    state->lineTo(xMax, yMax);
    state->lineTo(xMin, yMax);
    state->closePath();
    state->clip();
    out->clip(state);
    state->clearPath();
  }

  // set the color space
  state->setFillColorSpace(shading->getColorSpace()->copy());
  out->updateFillColorSpace(state);

  // do shading type-specific operations
  switch (shading->getType()) {
  case 1:
    doFunctionShFill((GfxFunctionShading *)shading);
    break;
  case 2:
    doAxialShFill((GfxAxialShading *)shading);
    break;
  case 3:
    doRadialShFill((GfxRadialShading *)shading);
    break;
  case 4:
  case 5:
    doGouraudTriangleShFill((GfxGouraudTriangleShading *)shading);
    break;
  case 6:
  case 7:
    doPatchMeshShFill((GfxPatchMeshShading *)shading);
    break;
  }

  // restore graphics state
  restoreState();
  state->setPath(savedPath);

  delete shading;
}

void Gfx::doFunctionShFill(GfxFunctionShading *shading) {
  double x0, y0, x1, y1;
  GfxColor colors[4];

  if (out->useShadedFills()) {
    out->functionShadedFill(state, shading);
  } else {
  shading->getDomain(&x0, &y0, &x1, &y1);
  shading->getColor(x0, y0, &colors[0]);
  shading->getColor(x0, y1, &colors[1]);
  shading->getColor(x1, y0, &colors[2]);
  shading->getColor(x1, y1, &colors[3]);
  doFunctionShFill1(shading, x0, y0, x1, y1, colors, 0);
  }
}

void Gfx::doFunctionShFill1(GfxFunctionShading *shading,
                            double x0, double y0,
                            double x1, double y1,
                            GfxColor *colors, int depth) {
  GfxColor fillColor;
  GfxColor color0M, color1M, colorM0, colorM1, colorMM;
  GfxColor colors2[4];
  double *matrix;
  double xM, yM;
  int nComps, i, j;

  nComps = shading->getColorSpace()->getNComps();
  matrix = shading->getMatrix();

  // compare the four corner colors
  for (i = 0; i < 4; ++i) {
    for (j = 0; j < nComps; ++j) {
      if (abs(colors[i].c[j] - colors[(i+1)&3].c[j]) > functionColorDelta) {
        break;
      }
    }
    if (j < nComps) {
      break;
    }
  }

  // center of the rectangle
  xM = 0.5 * (x0 + x1);
  yM = 0.5 * (y0 + y1);

  // the four corner colors are close (or we hit the recursive limit)
  // -- fill the rectangle; but require at least one subdivision
  // (depth==0) to avoid problems when the four outer corners of the
  // shaded region are the same color
  if ((i == 4 && depth > 0) || depth == functionMaxDepth) {

    // use the center color
    shading->getColor(xM, yM, &fillColor);
    state->setFillColor(&fillColor);
    out->updateFillColor(state);

    // fill the rectangle
    state->moveTo(x0 * matrix[0] + y0 * matrix[2] + matrix[4],
                  x0 * matrix[1] + y0 * matrix[3] + matrix[5]);
    state->lineTo(x1 * matrix[0] + y0 * matrix[2] + matrix[4],
                  x1 * matrix[1] + y0 * matrix[3] + matrix[5]);
    state->lineTo(x1 * matrix[0] + y1 * matrix[2] + matrix[4],
                  x1 * matrix[1] + y1 * matrix[3] + matrix[5]);
    state->lineTo(x0 * matrix[0] + y1 * matrix[2] + matrix[4],
                  x0 * matrix[1] + y1 * matrix[3] + matrix[5]);
    state->closePath();
    out->fill(state);
    state->clearPath();

  // the four corner colors are not close enough -- subdivide the
  // rectangle
  } else {

    // colors[0]       colorM0       colors[2]
    //   (x0,y0)       (xM,y0)       (x1,y0)
    //         +----------+----------+
    //         |          |          |
    //         |    UL    |    UR    |
    // color0M |       colorMM       | color1M
    // (x0,yM) +----------+----------+ (x1,yM)
    //         |       (xM,yM)       |
    //         |    LL    |    LR    |
    //         |          |          |
    //         +----------+----------+
    // colors[1]       colorM1       colors[3]
    //   (x0,y1)       (xM,y1)       (x1,y1)

    shading->getColor(x0, yM, &color0M);
    shading->getColor(x1, yM, &color1M);
    shading->getColor(xM, y0, &colorM0);
    shading->getColor(xM, y1, &colorM1);
    shading->getColor(xM, yM, &colorMM);

    // upper-left sub-rectangle
    colors2[0] = colors[0];
    colors2[1] = color0M;
    colors2[2] = colorM0;
    colors2[3] = colorMM;
    doFunctionShFill1(shading, x0, y0, xM, yM, colors2, depth + 1);
    
    // lower-left sub-rectangle
    colors2[0] = color0M;
    colors2[1] = colors[1];
    colors2[2] = colorMM;
    colors2[3] = colorM1;
    doFunctionShFill1(shading, x0, yM, xM, y1, colors2, depth + 1);
    
    // upper-right sub-rectangle
    colors2[0] = colorM0;
    colors2[1] = colorMM;
    colors2[2] = colors[2];
    colors2[3] = color1M;
    doFunctionShFill1(shading, xM, y0, x1, yM, colors2, depth + 1);

    // lower-right sub-rectangle
    colors2[0] = colorMM;
    colors2[1] = colorM1;
    colors2[2] = color1M;
    colors2[3] = colors[3];
    doFunctionShFill1(shading, xM, yM, x1, y1, colors2, depth + 1);
  }
}

void Gfx::doAxialShFill(GfxAxialShading *shading) {
  double xMin, yMin, xMax, yMax;
  double x0, y0, x1, y1;
  double dx, dy, mul;
  GBool dxZero, dyZero;
  double tMin, tMax, t, tx, ty;
  double s[4], sMin, sMax, tmp;
  double ux0, uy0, ux1, uy1, vx0, vy0, vx1, vy1;
  double t0, t1, tt;
  double ta[axialMaxSplits + 1];
  int next[axialMaxSplits + 1];
  GfxColor color0, color1;
  int nComps;
  int i, j, k, kk;

  if (out->useShadedFills()) {

    out->axialShadedFill(state, shading);

  } else {

  // get the clip region bbox
  state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);

  // compute min and max t values, based on the four corners of the
  // clip region bbox
  shading->getCoords(&x0, &y0, &x1, &y1);
  dx = x1 - x0;
  dy = y1 - y0;
    dxZero = fabs(dx) < 0.001;
    dyZero = fabs(dy) < 0.001;
  mul = 1 / (dx * dx + dy * dy);
  tMin = tMax = ((xMin - x0) * dx + (yMin - y0) * dy) * mul;
  t = ((xMin - x0) * dx + (yMax - y0) * dy) * mul;
  if (t < tMin) {
    tMin = t;
  } else if (t > tMax) {
    tMax = t;
  }
  t = ((xMax - x0) * dx + (yMin - y0) * dy) * mul;
  if (t < tMin) {
    tMin = t;
  } else if (t > tMax) {
    tMax = t;
  }
  t = ((xMax - x0) * dx + (yMax - y0) * dy) * mul;
  if (t < tMin) {
    tMin = t;
  } else if (t > tMax) {
    tMax = t;
  }
  if (tMin < 0 && !shading->getExtend0()) {
    tMin = 0;
  }
  if (tMax > 1 && !shading->getExtend1()) {
    tMax = 1;
  }

  // get the function domain
  t0 = shading->getDomain0();
  t1 = shading->getDomain1();

  // Traverse the t axis and do the shading.
  //
  // For each point (tx, ty) on the t axis, consider a line through
  // that point perpendicular to the t axis:
  //
  //     x(s) = tx + s * -dy   -->   s = (x - tx) / -dy
  //     y(s) = ty + s * dx    -->   s = (y - ty) / dx
  //
  // Then look at the intersection of this line with the bounding box
  // (xMin, yMin, xMax, yMax).  In the general case, there are four
  // intersection points:
  //
  //     s0 = (xMin - tx) / -dy
  //     s1 = (xMax - tx) / -dy
  //     s2 = (yMin - ty) / dx
  //     s3 = (yMax - ty) / dx
  //
  // and we want the middle two s values.
  //
  // In the case where dx = 0, take s0 and s1; in the case where dy =
  // 0, take s2 and s3.
  //
  // Each filled polygon is bounded by two of these line segments
  // perpdendicular to the t axis.
  //
  // The t axis is bisected into smaller regions until the color
  // difference across a region is small enough, and then the region
  // is painted with a single color.

  // set up: require at least one split to avoid problems when the two
  // ends of the t axis have the same color
  nComps = shading->getColorSpace()->getNComps();
  ta[0] = tMin;
  next[0] = axialMaxSplits / 2;
  ta[axialMaxSplits / 2] = 0.5 * (tMin + tMax);
  next[axialMaxSplits / 2] = axialMaxSplits;
  ta[axialMaxSplits] = tMax;

  // compute the color at t = tMin
  if (tMin < 0) {
    tt = t0;
  } else if (tMin > 1) {
    tt = t1;
  } else {
    tt = t0 + (t1 - t0) * tMin;
  }
  shading->getColor(tt, &color0);

  // compute the coordinates of the point on the t axis at t = tMin;
  // then compute the intersection of the perpendicular line with the
  // bounding box
  tx = x0 + tMin * dx;
  ty = y0 + tMin * dy;
    if (dxZero && dyZero) {
    sMin = sMax = 0;
    } if (dxZero) {
    sMin = (xMin - tx) / -dy;
    sMax = (xMax - tx) / -dy;
    if (sMin > sMax) { tmp = sMin; sMin = sMax; sMax = tmp; }
    } else if (dyZero) {
    sMin = (yMin - ty) / dx;
    sMax = (yMax - ty) / dx;
    if (sMin > sMax) { tmp = sMin; sMin = sMax; sMax = tmp; }
  } else {
    s[0] = (yMin - ty) / dx;
    s[1] = (yMax - ty) / dx;
    s[2] = (xMin - tx) / -dy;
    s[3] = (xMax - tx) / -dy;
    for (j = 0; j < 3; ++j) {
      kk = j;
      for (k = j + 1; k < 4; ++k) {
        if (s[k] < s[kk]) {
          kk = k;
        }
      }
      tmp = s[j]; s[j] = s[kk]; s[kk] = tmp;
    }
    sMin = s[1];
    sMax = s[2];
  }
  ux0 = tx - sMin * dy;
  uy0 = ty + sMin * dx;
  vx0 = tx - sMax * dy;
  vy0 = ty + sMax * dx;

  i = 0;
  while (i < axialMaxSplits) {

    // bisect until color difference is small enough or we hit the
    // bisection limit
    j = next[i];
    while (j > i + 1) {
      if (ta[j] < 0) {
        tt = t0;
      } else if (ta[j] > 1) {
        tt = t1;
      } else {
        tt = t0 + (t1 - t0) * ta[j];
      }
      shading->getColor(tt, &color1);
      for (k = 0; k < nComps; ++k) {
          if (abs(color1.c[k] - color0.c[k]) > axialColorDelta) {
          break;
        }
      }
      if (k == nComps) {
        break;
      }
      k = (i + j) / 2;
      ta[k] = 0.5 * (ta[i] + ta[j]);
      next[i] = k;
      next[k] = j;
      j = k;
    }

    // use the average of the colors of the two sides of the region
    for (k = 0; k < nComps; ++k) {
        color0.c[k] = (color0.c[k] + color1.c[k]) / 2;
    }

    // compute the coordinates of the point on the t axis; then
    // compute the intersection of the perpendicular line with the
    // bounding box
    tx = x0 + ta[j] * dx;
    ty = y0 + ta[j] * dy;
      if (dxZero && dyZero) {
      sMin = sMax = 0;
      } if (dxZero) {
      sMin = (xMin - tx) / -dy;
      sMax = (xMax - tx) / -dy;
      if (sMin > sMax) { tmp = sMin; sMin = sMax; sMax = tmp; }
      } else if (dyZero) {
      sMin = (yMin - ty) / dx;
      sMax = (yMax - ty) / dx;
      if (sMin > sMax) { tmp = sMin; sMin = sMax; sMax = tmp; }
    } else {
      s[0] = (yMin - ty) / dx;
      s[1] = (yMax - ty) / dx;
      s[2] = (xMin - tx) / -dy;
      s[3] = (xMax - tx) / -dy;
      for (j = 0; j < 3; ++j) {
        kk = j;
        for (k = j + 1; k < 4; ++k) {
          if (s[k] < s[kk]) {
            kk = k;
          }
        }
        tmp = s[j]; s[j] = s[kk]; s[kk] = tmp;
      }
      sMin = s[1];
      sMax = s[2];
    }
    ux1 = tx - sMin * dy;
    uy1 = ty + sMin * dx;
    vx1 = tx - sMax * dy;
    vy1 = ty + sMax * dx;

    // set the color
    state->setFillColor(&color0);
    out->updateFillColor(state);

    // fill the region
    state->moveTo(ux0, uy0);
    state->lineTo(vx0, vy0);
    state->lineTo(vx1, vy1);
    state->lineTo(ux1, uy1);
    state->closePath();
    out->fill(state);
    state->clearPath();

    // set up for next region
    ux0 = ux1;
    uy0 = uy1;
    vx0 = vx1;
    vy0 = vy1;
    color0 = color1;
    i = next[i];
  }
  }
}

void Gfx::doRadialShFill(GfxRadialShading *shading) {
  double sMin, sMax, xMin, yMin, xMax, yMax;
  double x0, y0, r0, x1, y1, r1, t0, t1;
  int nComps;
  GfxColor colorA, colorB;
  double xa, ya, xb, yb, ra, rb;
  double ta, tb, sa, sb;
  int ia, ib, k, n;
  double *ctm;
  double angle, t, d0, d1;

  if (out->useShadedFills()) {

    out->radialShadedFill(state, shading);

  } else {

  // get the shading info
  shading->getCoords(&x0, &y0, &r0, &x1, &y1, &r1);
  t0 = shading->getDomain0();
  t1 = shading->getDomain1();
  nComps = shading->getColorSpace()->getNComps();

  // compute the (possibly extended) s range
  sMin = 0;
  sMax = 1;
  if (shading->getExtend0()) {
    if (r0 < r1) {
      // extend the smaller end
      sMin = -r0 / (r1 - r0);
    } else {
      // extend the larger end
      state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);
      d0 = (x0 - xMin) * (x0 - xMin);
      d1 = (x0 - xMax) * (x0 - xMax);
      sMin = d0 > d1 ? d0 : d1;
      d0 = (y0 - yMin) * (y0 - yMin);
      d1 = (y0 - yMax) * (y0 - yMax);
      sMin += d0 > d1 ? d0 : d1;
      sMin = (sqrt(sMin) - r0) / (r1 - r0);
      if (sMin > 0) {
        sMin = 0;
      } else if (sMin < -20) {
        // sanity check
        sMin = -20;
      }
    }
  }
  if (shading->getExtend1()) {
    if (r1 < r0) {
      // extend the smaller end
      sMax = -r0 / (r1 - r0);
    } else if (r1 > r0) {
      // extend the larger end
      state->getUserClipBBox(&xMin, &yMin, &xMax, &yMax);
      d0 = (x1 - xMin) * (x1 - xMin);
      d1 = (x1 - xMax) * (x1 - xMax);
      sMax = d0 > d1 ? d0 : d1;
      d0 = (y1 - yMin) * (y1 - yMin);
      d1 = (y1 - yMax) * (y1 - yMax);
      sMax += d0 > d1 ? d0 : d1;
      sMax = (sqrt(sMax) - r0) / (r1 - r0);
      if (sMax < 1) {
          sMax = 1;
      } else if (sMax > 20) {
        // sanity check
        sMax = 20;
      }
    }
  }

  // compute the number of steps into which circles must be divided to
  // achieve a curve flatness of 0.1 pixel in device space for the
  // largest circle (note that "device space" is 72 dpi when generating
  // PostScript, hence the relatively small 0.1 pixel accuracy)
  ctm = state->getCTM();
  t = fabs(ctm[0]);
  if (fabs(ctm[1]) > t) {
    t = fabs(ctm[1]);
  }
  if (fabs(ctm[2]) > t) {
    t = fabs(ctm[2]);
  }
  if (fabs(ctm[3]) > t) {
    t = fabs(ctm[3]);
  }
  if (r0 > r1) {
    t *= r0;
  } else {
    t *= r1;
  }
  if (t < 1) {
    n = 3;
  } else {
    n = (int)(M_PI / acos(1 - 0.1 / t));
    if (n < 3) {
      n = 3;
    } else if (n > 200) {
      n = 200;
    }
  }

  // Traverse the t axis and do the shading.
  //
  // This generates and fills a series of rings.  Each ring is defined
  // by two circles:
  //   sa, ta, xa, ya, ra, colorA
  //   sb, tb, xb, yb, rb, colorB
  //
  // The s/t axis is divided into radialMaxSplits parts; these parts
  // are combined as much as possible while respecting the
  // radialColorDelta parameter.

  // setup for the start circle
  ia = 0;
  sa = sMin;
  ta = t0 + sa * (t1 - t0);
  xa = x0 + sa * (x1 - x0);
  ya = y0 + sa * (y1 - y0);
  ra = r0 + sa * (r1 - r0);
  if (ta < t0) {
    shading->getColor(t0, &colorA);
  } else if (ta > t1) {
    shading->getColor(t1, &colorA);
  } else {
    shading->getColor(ta, &colorA);
  }

  while (ia < radialMaxSplits) {

    // go as far along the t axis (toward t1) as we can, such that the
    // color difference is within the tolerance (radialColorDelta) --
    // this uses bisection (between the current value, t, and t1),
    // limited to radialMaxSplits points along the t axis; require at
    // least one split to avoid problems when the innermost and
    // outermost colors are the same
    ib = radialMaxSplits;
    sb = sMin + ((double)ib / (double)radialMaxSplits) * (sMax - sMin);
    tb = t0 + sb * (t1 - t0);
    if (tb < t0) {
      shading->getColor(t0, &colorB);
    } else if (tb > t1) {
      shading->getColor(t1, &colorB);
    } else {
      shading->getColor(tb, &colorB);
    }
    while (ib - ia > 1) {
      for (k = 0; k < nComps; ++k) {
          if (abs(colorB.c[k] - colorA.c[k]) > radialColorDelta) {
          break;
        }
      }
      if (k == nComps && ib < radialMaxSplits) {
        break;
      }
      ib = (ia + ib) / 2;
      sb = sMin + ((double)ib / (double)radialMaxSplits) * (sMax - sMin);
      tb = t0 + sb * (t1 - t0);
      if (tb < t0) {
        shading->getColor(t0, &colorB);
      } else if (tb > t1) {
        shading->getColor(t1, &colorB);
      } else {
        shading->getColor(tb, &colorB);
      }
    }

    // compute center and radius of the circle
    xb = x0 + sb * (x1 - x0);
    yb = y0 + sb * (y1 - y0);
    rb = r0 + sb * (r1 - r0);

    // use the average of the colors at the two circles
    for (k = 0; k < nComps; ++k) {
        colorA.c[k] = (colorA.c[k] + colorB.c[k]) / 2;
    }
    state->setFillColor(&colorA);
    out->updateFillColor(state);

    // construct path for first circle
    state->moveTo(xa + ra, ya);
    for (k = 1; k < n; ++k) {
      angle = ((double)k / (double)n) * 2 * M_PI;
      state->lineTo(xa + ra * cos(angle), ya + ra * sin(angle));
    }
    state->closePath();

    // construct and append path for second circle
    state->moveTo(xb + rb, yb);
    for (k = 1; k < n; ++k) {
      angle = ((double)k / (double)n) * 2 * M_PI;
      state->lineTo(xb + rb * cos(angle), yb + rb * sin(angle));
    }
    state->closePath();

    // fill the ring
    out->eoFill(state);
    state->clearPath();

    // step to the next value of t
    ia = ib;
    sa = sb;
    ta = tb;
    xa = xb;
    ya = yb;
    ra = rb;
    colorA = colorB;
  }
  }
}

void Gfx::doGouraudTriangleShFill(GfxGouraudTriangleShading *shading) {
  double x0, y0, x1, y1, x2, y2;
  GfxColor color0, color1, color2;
  int i;

  for (i = 0; i < shading->getNTriangles(); ++i) {
    shading->getTriangle(i, &x0, &y0, &color0,
                         &x1, &y1, &color1,
                         &x2, &y2, &color2);
    gouraudFillTriangle(x0, y0, &color0, x1, y1, &color1, x2, y2, &color2,
                        shading->getColorSpace()->getNComps(), 0);
  }
}

void Gfx::gouraudFillTriangle(double x0, double y0, GfxColor *color0,
                              double x1, double y1, GfxColor *color1,
                              double x2, double y2, GfxColor *color2,
                              int nComps, int depth) {
  double x01, y01, x12, y12, x20, y20;
  GfxColor color01, color12, color20;
  int i;

  for (i = 0; i < nComps; ++i) {
    if (abs(color0->c[i] - color1->c[i]) > gouraudColorDelta ||
        abs(color1->c[i] - color2->c[i]) > gouraudColorDelta) {
      break;
    }
  }
  if (i == nComps || depth == gouraudMaxDepth) {
    state->setFillColor(color0);
    out->updateFillColor(state);
    state->moveTo(x0, y0);
    state->lineTo(x1, y1);
    state->lineTo(x2, y2);
    state->closePath();
    out->fill(state);
    state->clearPath();
  } else {
    x01 = 0.5 * (x0 + x1);
    y01 = 0.5 * (y0 + y1);
    x12 = 0.5 * (x1 + x2);
    y12 = 0.5 * (y1 + y2);
    x20 = 0.5 * (x2 + x0);
    y20 = 0.5 * (y2 + y0);
    //~ if the shading has a Function, this should interpolate on the
    //~ function parameter, not on the color components
    for (i = 0; i < nComps; ++i) {
      color01.c[i] = (color0->c[i] + color1->c[i]) / 2;
      color12.c[i] = (color1->c[i] + color2->c[i]) / 2;
      color20.c[i] = (color2->c[i] + color0->c[i]) / 2;
    }
    gouraudFillTriangle(x0, y0, color0, x01, y01, &color01,
                        x20, y20, &color20, nComps, depth + 1);
    gouraudFillTriangle(x01, y01, &color01, x1, y1, color1,
                        x12, y12, &color12, nComps, depth + 1);
    gouraudFillTriangle(x01, y01, &color01, x12, y12, &color12,
                        x20, y20, &color20, nComps, depth + 1);
    gouraudFillTriangle(x20, y20, &color20, x12, y12, &color12,
                        x2, y2, color2, nComps, depth + 1);
  }
}

void Gfx::doPatchMeshShFill(GfxPatchMeshShading *shading) {
  int start, i;

  if (shading->getNPatches() > 128) {
    start = 3;
  } else if (shading->getNPatches() > 64) {
    start = 2;
  } else if (shading->getNPatches() > 16) {
    start = 1;
  } else {
    start = 0;
  }
  for (i = 0; i < shading->getNPatches(); ++i) {
    fillPatch(shading->getPatch(i), shading->getColorSpace()->getNComps(),
              start);
  }
}

void Gfx::fillPatch(GfxPatch *patch, int nComps, int depth) {
  GfxPatch patch00, patch01, patch10, patch11;
  double xx[4][8], yy[4][8];
  double xxm, yym;
  int i;

  for (i = 0; i < nComps; ++i) {
    if (abs(patch->color[0][0].c[i] - patch->color[0][1].c[i])
          > patchColorDelta ||
        abs(patch->color[0][1].c[i] - patch->color[1][1].c[i])
          > patchColorDelta ||
        abs(patch->color[1][1].c[i] - patch->color[1][0].c[i])
          > patchColorDelta ||
        abs(patch->color[1][0].c[i] - patch->color[0][0].c[i])
          > patchColorDelta) {
      break;
    }
  }
  if (i == nComps || depth == patchMaxDepth) {
    state->setFillColor(&patch->color[0][0]);
    out->updateFillColor(state);
    state->moveTo(patch->x[0][0], patch->y[0][0]);
    state->curveTo(patch->x[0][1], patch->y[0][1],
                   patch->x[0][2], patch->y[0][2],
                   patch->x[0][3], patch->y[0][3]);
    state->curveTo(patch->x[1][3], patch->y[1][3],
                   patch->x[2][3], patch->y[2][3],
                   patch->x[3][3], patch->y[3][3]);
    state->curveTo(patch->x[3][2], patch->y[3][2],
                   patch->x[3][1], patch->y[3][1],
                   patch->x[3][0], patch->y[3][0]);
    state->curveTo(patch->x[2][0], patch->y[2][0],
                   patch->x[1][0], patch->y[1][0],
                   patch->x[0][0], patch->y[0][0]);
    state->closePath();
    out->fill(state);
    state->clearPath();
  } else {
    for (i = 0; i < 4; ++i) {
      xx[i][0] = patch->x[i][0];
      yy[i][0] = patch->y[i][0];
      xx[i][1] = 0.5 * (patch->x[i][0] + patch->x[i][1]);
      yy[i][1] = 0.5 * (patch->y[i][0] + patch->y[i][1]);
      xxm = 0.5 * (patch->x[i][1] + patch->x[i][2]);
      yym = 0.5 * (patch->y[i][1] + patch->y[i][2]);
      xx[i][6] = 0.5 * (patch->x[i][2] + patch->x[i][3]);
      yy[i][6] = 0.5 * (patch->y[i][2] + patch->y[i][3]);
      xx[i][2] = 0.5 * (xx[i][1] + xxm);
      yy[i][2] = 0.5 * (yy[i][1] + yym);
      xx[i][5] = 0.5 * (xxm + xx[i][6]);
      yy[i][5] = 0.5 * (yym + yy[i][6]);
      xx[i][3] = xx[i][4] = 0.5 * (xx[i][2] + xx[i][5]);
      yy[i][3] = yy[i][4] = 0.5 * (yy[i][2] + yy[i][5]);
      xx[i][7] = patch->x[i][3];
      yy[i][7] = patch->y[i][3];
    }
    for (i = 0; i < 4; ++i) {
      patch00.x[0][i] = xx[0][i];
      patch00.y[0][i] = yy[0][i];
      patch00.x[1][i] = 0.5 * (xx[0][i] + xx[1][i]);
      patch00.y[1][i] = 0.5 * (yy[0][i] + yy[1][i]);
      xxm = 0.5 * (xx[1][i] + xx[2][i]);
      yym = 0.5 * (yy[1][i] + yy[2][i]);
      patch10.x[2][i] = 0.5 * (xx[2][i] + xx[3][i]);
      patch10.y[2][i] = 0.5 * (yy[2][i] + yy[3][i]);
      patch00.x[2][i] = 0.5 * (patch00.x[1][i] + xxm);
      patch00.y[2][i] = 0.5 * (patch00.y[1][i] + yym);
      patch10.x[1][i] = 0.5 * (xxm + patch10.x[2][i]);
      patch10.y[1][i] = 0.5 * (yym + patch10.y[2][i]);
      patch00.x[3][i] = 0.5 * (patch00.x[2][i] + patch10.x[1][i]);
      patch00.y[3][i] = 0.5 * (patch00.y[2][i] + patch10.y[1][i]);
      patch10.x[0][i] = patch00.x[3][i];
      patch10.y[0][i] = patch00.y[3][i];
      patch10.x[3][i] = xx[3][i];
      patch10.y[3][i] = yy[3][i];
    }
    for (i = 4; i < 8; ++i) {
      patch01.x[0][i-4] = xx[0][i];
      patch01.y[0][i-4] = yy[0][i];
      patch01.x[1][i-4] = 0.5 * (xx[0][i] + xx[1][i]);
      patch01.y[1][i-4] = 0.5 * (yy[0][i] + yy[1][i]);
      xxm = 0.5 * (xx[1][i] + xx[2][i]);
      yym = 0.5 * (yy[1][i] + yy[2][i]);
      patch11.x[2][i-4] = 0.5 * (xx[2][i] + xx[3][i]);
      patch11.y[2][i-4] = 0.5 * (yy[2][i] + yy[3][i]);
      patch01.x[2][i-4] = 0.5 * (patch01.x[1][i-4] + xxm);
      patch01.y[2][i-4] = 0.5 * (patch01.y[1][i-4] + yym);
      patch11.x[1][i-4] = 0.5 * (xxm + patch11.x[2][i-4]);
      patch11.y[1][i-4] = 0.5 * (yym + patch11.y[2][i-4]);
      patch01.x[3][i-4] = 0.5 * (patch01.x[2][i-4] + patch11.x[1][i-4]);
      patch01.y[3][i-4] = 0.5 * (patch01.y[2][i-4] + patch11.y[1][i-4]);
      patch11.x[0][i-4] = patch01.x[3][i-4];
      patch11.y[0][i-4] = patch01.y[3][i-4];
      patch11.x[3][i-4] = xx[3][i];
      patch11.y[3][i-4] = yy[3][i];
    }
    //~ if the shading has a Function, this should interpolate on the
    //~ function parameter, not on the color components
    for (i = 0; i < nComps; ++i) {
      patch00.color[0][0].c[i] = patch->color[0][0].c[i];
      patch00.color[0][1].c[i] = (patch->color[0][0].c[i] +
                                  patch->color[0][1].c[i]) / 2;
      patch01.color[0][0].c[i] = patch00.color[0][1].c[i];
      patch01.color[0][1].c[i] = patch->color[0][1].c[i];
      patch01.color[1][1].c[i] = (patch->color[0][1].c[i] +
                                  patch->color[1][1].c[i]) / 2;
      patch11.color[0][1].c[i] = patch01.color[1][1].c[i];
      patch11.color[1][1].c[i] = patch->color[1][1].c[i];
      patch11.color[1][0].c[i] = (patch->color[1][1].c[i] +
                                  patch->color[1][0].c[i]) / 2;
      patch10.color[1][1].c[i] = patch11.color[1][0].c[i];
      patch10.color[1][0].c[i] = patch->color[1][0].c[i];
      patch10.color[0][0].c[i] = (patch->color[1][0].c[i] +
                                  patch->color[0][0].c[i]) / 2;
      patch00.color[1][0].c[i] = patch10.color[0][0].c[i];
      patch00.color[1][1].c[i] = (patch00.color[1][0].c[i] +
                                  patch01.color[1][1].c[i]) / 2;
      patch01.color[1][0].c[i] = patch00.color[1][1].c[i];
      patch11.color[0][0].c[i] = patch00.color[1][1].c[i];
      patch10.color[0][1].c[i] = patch00.color[1][1].c[i];
    }
    fillPatch(&patch00, nComps, depth + 1);
    fillPatch(&patch10, nComps, depth + 1);
    fillPatch(&patch01, nComps, depth + 1);
    fillPatch(&patch11, nComps, depth + 1);
  }
}

void Gfx::doEndPath() {
  if (state->isCurPt() && clip != clipNone) {
    state->clip();
    if (clip == clipNormal) {
      out->clip(state);
    } else {
      out->eoClip(state);
    }
  }
  clip = clipNone;
  state->clearPath();
}

//------------------------------------------------------------------------
// path clipping operators
//------------------------------------------------------------------------

void Gfx::opClip(Object args[], int numArgs) {
  clip = clipNormal;
}

void Gfx::opEOClip(Object args[], int numArgs) {
  clip = clipEO;
}

//------------------------------------------------------------------------
// text object operators
//------------------------------------------------------------------------

void Gfx::opBeginText(Object args[], int numArgs) {
  state->setTextMat(1, 0, 0, 1, 0, 0);
  state->textMoveTo(0, 0);
  out->updateTextMat(state);
  out->updateTextPos(state);
  fontChanged = gTrue;
}

void Gfx::opEndText(Object args[], int numArgs) {
  out->endTextObject(state);
}

//------------------------------------------------------------------------
// text state operators
//------------------------------------------------------------------------

void Gfx::opSetCharSpacing(Object args[], int numArgs) {
  state->setCharSpace(args[0].getNum());
  out->updateCharSpace(state);
}

void Gfx::opSetFont(Object args[], int numArgs) {
  GfxFont *font;

  if (!(font = res->lookupFont(args[0].getNameC()))) {
    return;
  }
  if (printCommands) {
    printf("  font: tag=%s name='%s' %g\n",
           font->getTag()->getCString(),
           font->getName() ? font->getName()->getCString() : "???",
           args[1].getNum());
    fflush(stdout);
  }

  font->incRefCnt();
  state->setFont(font, args[1].getNum());
  fontChanged = gTrue;
}

void Gfx::opSetTextLeading(Object args[], int numArgs) {
  state->setLeading(args[0].getNum());
}

void Gfx::opSetTextRender(Object args[], int numArgs) {
  state->setRender(args[0].getInt());
  out->updateRender(state);
}

void Gfx::opSetTextRise(Object args[], int numArgs) {
  state->setRise(args[0].getNum());
  out->updateRise(state);
}

void Gfx::opSetWordSpacing(Object args[], int numArgs) {
  state->setWordSpace(args[0].getNum());
  out->updateWordSpace(state);
}

void Gfx::opSetHorizScaling(Object args[], int numArgs) {
  state->setHorizScaling(args[0].getNum());
  out->updateHorizScaling(state);
  fontChanged = gTrue;
}

//------------------------------------------------------------------------
// text positioning operators
//------------------------------------------------------------------------

void Gfx::opTextMove(Object args[], int numArgs) {
  double tx, ty;

  tx = state->getLineX() + args[0].getNum();
  ty = state->getLineY() + args[1].getNum();
  state->textMoveTo(tx, ty);
  out->updateTextPos(state);
}

void Gfx::opTextMoveSet(Object args[], int numArgs) {
  double tx, ty;

  tx = state->getLineX() + args[0].getNum();
  ty = args[1].getNum();
  state->setLeading(-ty);
  ty += state->getLineY();
  state->textMoveTo(tx, ty);
  out->updateTextPos(state);
}

void Gfx::opSetTextMatrix(Object args[], int numArgs) {
  state->setTextMat(args[0].getNum(), args[1].getNum(),
                    args[2].getNum(), args[3].getNum(),
                    args[4].getNum(), args[5].getNum());
  state->textMoveTo(0, 0);
  out->updateTextMat(state);
  out->updateTextPos(state);
  fontChanged = gTrue;
}

void Gfx::opTextNextLine(Object args[], int numArgs) {
  double tx, ty;

  tx = state->getLineX();
  ty = state->getLineY() - state->getLeading();
  state->textMoveTo(tx, ty);
  out->updateTextPos(state);
}

//------------------------------------------------------------------------
// text string operators
//------------------------------------------------------------------------

void Gfx::opShowText(Object args[], int numArgs) {
  if (!state->getFont()) {
    error(getPos(), "No font in show");
    return;
  }
  if (fontChanged) {
    out->updateFont(state);
    fontChanged = gFalse;
  }
  out->beginStringOp(state);
  doShowText(args[0].getString());
  out->endStringOp(state);
}

void Gfx::opMoveShowText(Object args[], int numArgs) {
  double tx, ty;

  if (!state->getFont()) {
    error(getPos(), "No font in move/show");
    return;
  }
  if (fontChanged) {
    out->updateFont(state);
    fontChanged = gFalse;
  }
  tx = state->getLineX();
  ty = state->getLineY() - state->getLeading();
  state->textMoveTo(tx, ty);
  out->updateTextPos(state);
  out->beginStringOp(state);
  doShowText(args[0].getString());
  out->endStringOp(state);
}

void Gfx::opMoveSetShowText(Object args[], int numArgs) {
  double tx, ty;

  if (!state->getFont()) {
    error(getPos(), "No font in move/set/show");
    return;
  }
  if (fontChanged) {
    out->updateFont(state);
    fontChanged = gFalse;
  }
  state->setWordSpace(args[0].getNum());
  state->setCharSpace(args[1].getNum());
  tx = state->getLineX();
  ty = state->getLineY() - state->getLeading();
  state->textMoveTo(tx, ty);
  out->updateWordSpace(state);
  out->updateCharSpace(state);
  out->updateTextPos(state);
  out->beginStringOp(state);
  doShowText(args[2].getString());
  out->endStringOp(state);
}

void Gfx::opShowSpaceText(Object args[], int numArgs) {
  Array *a;
  Object obj;
  int wMode;
  int i;

  if (!state->getFont()) {
    error(getPos(), "No font in show/space");
    return;
  }
  if (fontChanged) {
    out->updateFont(state);
    fontChanged = gFalse;
  }
  out->beginStringOp(state);
  wMode = state->getFont()->getWMode();
  a = args[0].getArray();
  for (i = 0; i < a->getLength(); ++i) {
    a->get(i, &obj);
    if (obj.isNum()) {
      // this uses the absolute value of the font size to match
      // Acrobat's behavior
      if (wMode) {
        state->textShift(0, -obj.getNum() * 0.001 *
                        fabs(state->getFontSize()));
      } else {
        state->textShift(-obj.getNum() * 0.001 *
                         fabs(state->getFontSize()), 0);
      }
      out->updateTextShift(state, obj.getNum());
    } else if (obj.isString()) {
      doShowText(obj.getString());
    } else {
      error(getPos(), "Element of show/space array must be number or string");
    }
    obj.free();
  }
  out->endStringOp(state);
}

void Gfx::doShowText(GooString *s) {
  GfxFont *font;
  int wMode;
  double riseX, riseY;
  CharCode code;
  Unicode u[8];
  double x, y, dx, dy, dx2, dy2, curX, curY, tdx, tdy, lineX, lineY;
  double originX, originY, tOriginX, tOriginY;
  double oldCTM[6], newCTM[6];
  double *mat;
  Object charProc;
  Dict *resDict;
  Parser *oldParser;
  char *p;
  int len, n, uLen, nChars, nSpaces, i;

  font = state->getFont();
  wMode = font->getWMode();

  if (out->useDrawChar()) {
    out->beginString(state, s);
  }

  // handle a Type 3 char
  if (font->getType() == fontType3 && out->interpretType3Chars()) {
    mat = state->getCTM();
    for (i = 0; i < 6; ++i) {
      oldCTM[i] = mat[i];
    }
    mat = state->getTextMat();
    newCTM[0] = mat[0] * oldCTM[0] + mat[1] * oldCTM[2];
    newCTM[1] = mat[0] * oldCTM[1] + mat[1] * oldCTM[3];
    newCTM[2] = mat[2] * oldCTM[0] + mat[3] * oldCTM[2];
    newCTM[3] = mat[2] * oldCTM[1] + mat[3] * oldCTM[3];
    mat = font->getFontMatrix();
    newCTM[0] = mat[0] * newCTM[0] + mat[1] * newCTM[2];
    newCTM[1] = mat[0] * newCTM[1] + mat[1] * newCTM[3];
    newCTM[2] = mat[2] * newCTM[0] + mat[3] * newCTM[2];
    newCTM[3] = mat[2] * newCTM[1] + mat[3] * newCTM[3];
    newCTM[0] *= state->getFontSize();
    newCTM[1] *= state->getFontSize();
    newCTM[2] *= state->getFontSize();
    newCTM[3] *= state->getFontSize();
    newCTM[0] *= state->getHorizScaling();
    newCTM[2] *= state->getHorizScaling();
    state->textTransformDelta(0, state->getRise(), &riseX, &riseY);
    curX = state->getCurX();
    curY = state->getCurY();
    lineX = state->getLineX();
    lineY = state->getLineY();
    oldParser = parser;
    p = s->getCString();
    len = s->getLength();
    while (len > 0) {
      n = font->getNextChar(p, len, &code,
                            u, (int)(sizeof(u) / sizeof(Unicode)), &uLen,
                            &dx, &dy, &originX, &originY);
      dx = dx * state->getFontSize() + state->getCharSpace();
      if (n == 1 && *p == ' ') {
        dx += state->getWordSpace();
      }
      dx *= state->getHorizScaling();
      dy *= state->getFontSize();
      state->textTransformDelta(dx, dy, &tdx, &tdy);
      state->transform(curX + riseX, curY + riseY, &x, &y);
      saveState();
      state->setCTM(newCTM[0], newCTM[1], newCTM[2], newCTM[3], x, y);
      //~ out->updateCTM(???)
      if (!out->beginType3Char(state, curX + riseX, curY + riseY, tdx, tdy,
                               code, u, uLen)) {
        ((Gfx8BitFont *)font)->getCharProc(code, &charProc);
        if ((resDict = ((Gfx8BitFont *)font)->getResources())) {
          pushResources(resDict);
        }
        if (charProc.isStream()) {
          display(&charProc, gFalse);
        } else {
          error(getPos(), "Missing or bad Type3 CharProc entry");
        }
        out->endType3Char(state);
        if (resDict) {
          popResources();
        }
        charProc.free();
      }
      restoreState();
      // GfxState::restore() does *not* restore the current position,
      // so we deal with it here using (curX, curY) and (lineX, lineY)
      curX += tdx;
      curY += tdy;
      state->moveTo(curX, curY);
      state->textSetPos(lineX, lineY);
      p += n;
      len -= n;
    }
    parser = oldParser;

  } else if (out->useDrawChar()) {
    state->textTransformDelta(0, state->getRise(), &riseX, &riseY);
    p = s->getCString();
    len = s->getLength();
    while (len > 0) {
      n = font->getNextChar(p, len, &code,
			    u, (int)(sizeof(u) / sizeof(Unicode)), &uLen,
			    &dx, &dy, &originX, &originY);
      if (wMode) {
	dx *= state->getFontSize();
	dy = dy * state->getFontSize() + state->getCharSpace();
	if (n == 1 && *p == ' ') {
	  dy += state->getWordSpace();
	}
      } else {
	dx = dx * state->getFontSize() + state->getCharSpace();
	if (n == 1 && *p == ' ') {
	  dx += state->getWordSpace();
	}
	dx *= state->getHorizScaling();
	dy *= state->getFontSize();
      }
      state->textTransformDelta(dx, dy, &tdx, &tdy);
      originX *= state->getFontSize();
      originY *= state->getFontSize();
      state->textTransformDelta(originX, originY, &tOriginX, &tOriginY);
      out->drawChar(state, state->getCurX() + riseX, state->getCurY() + riseY,
		    tdx, tdy, tOriginX, tOriginY, code, n, u, uLen);
      state->shift(tdx, tdy);
      p += n;
      len -= n;
    }

  } else {
    dx = dy = 0;
    p = s->getCString();
    len = s->getLength();
    nChars = nSpaces = 0;
    while (len > 0) {
      n = font->getNextChar(p, len, &code,
			    u, (int)(sizeof(u) / sizeof(Unicode)), &uLen,
			    &dx2, &dy2, &originX, &originY);
      dx += dx2;
      dy += dy2;
      if (n == 1 && *p == ' ') {
	++nSpaces;
      }
      ++nChars;
      p += n;
      len -= n;
    }
    if (wMode) {
      dx *= state->getFontSize();
      dy = dy * state->getFontSize()
	   + nChars * state->getCharSpace()
	   + nSpaces * state->getWordSpace();
    } else {
      dx = dx * state->getFontSize()
	   + nChars * state->getCharSpace()
	   + nSpaces * state->getWordSpace();
      dx *= state->getHorizScaling();
      dy *= state->getFontSize();
    }
    state->textTransformDelta(dx, dy, &tdx, &tdy);
    out->drawString(state, s);
    state->shift(tdx, tdy);
  }

  if (out->useDrawChar()) {
    out->endString(state);
  }

  updateLevel += 10 * s->getLength();
}

//------------------------------------------------------------------------
// XObject operators
//------------------------------------------------------------------------

void Gfx::opXObject(Object args[], int numArgs) {
  Object obj1, obj2, obj3, refObj;
#if OPI_SUPPORT
  Object opiDict;
#endif

  if (!res->lookupXObject(args[0].getNameC(), &obj1)) {
    return;
  }
  if (!obj1.isStream()) {
    error(getPos(), "XObject '%s' is wrong type", args[0].getNameC());
    obj1.free();
    return;
  }
#if OPI_SUPPORT
  obj1.streamGetDict()->lookup("OPI", &opiDict);
  if (opiDict.isDict()) {
    out->opiBegin(state, opiDict.getDict());
  }
#endif
  obj1.streamGetDict()->lookup("Subtype", &obj2);
  if (obj2.isName("Image")) {
    if (out->needNonText()) {
    res->lookupXObjectNF(args[0].getNameC(), &refObj);
    doImage(&refObj, obj1.getStream(), gFalse);
    refObj.free();
    }
  } else if (obj2.isName("Form")) {
    doForm(&obj1);
  } else if (obj2.isName("PS")) {
    obj1.streamGetDict()->lookup("Level1", &obj3);
    out->psXObject(obj1.getStream(),
                   obj3.isStream() ? obj3.getStream() : (Stream *)NULL);
  } else if (obj2.isName()) {
    error(getPos(), "Unknown XObject subtype '%s'", obj2.getNameC());
  } else {
    error(getPos(), "XObject subtype is missing or wrong type");
  }
  obj2.free();
#if OPI_SUPPORT
  if (opiDict.isDict()) {
    out->opiEnd(state, opiDict.getDict());
  }
  opiDict.free();
#endif
  obj1.free();
}

void Gfx::doImage(Object *ref, Stream *str, GBool inlineImg) {
  Dict *dict, *maskDict;
  int width, height;
  int bits, maskBits;
  StreamColorSpaceMode csMode;
  GBool mask;
  GBool invert;
  GfxColorSpace *colorSpace, *maskColorSpace;
  GfxImageColorMap *colorMap, *maskColorMap;
  Object maskObj, smaskObj;
  GBool haveColorKeyMask, haveExplicitMask, haveSoftMask;
  int maskColors[2*gfxColorMaxComps];
  int maskWidth, maskHeight;
  GBool maskInvert;
  Stream *maskStr;
  Object obj1, obj2;
  int i;

  // get info from the stream
  bits = 0;
  csMode = streamCSNone;
  str->getImageParams(&bits, &csMode);

  // get stream dict
  dict = str->getDict();

  // get size
  dict->lookup("Width", &obj1);
  if (obj1.isNull()) {
    obj1.free();
    dict->lookup("W", &obj1);
  }
  if (!obj1.isInt())
    goto err2;
  width = obj1.getInt();
  obj1.free();
  dict->lookup("Height", &obj1);
  if (obj1.isNull()) {
    obj1.free();
    dict->lookup("H", &obj1);
  }
  if (!obj1.isInt())
    goto err2;
  height = obj1.getInt();
  obj1.free();

  // image or mask?
  dict->lookup("ImageMask", &obj1);
  if (obj1.isNull()) {
    obj1.free();
    dict->lookup("IM", &obj1);
  }
  mask = gFalse;
  if (obj1.isBool())
    mask = obj1.getBool();
  else if (!obj1.isNull())
    goto err2;
  obj1.free();

  // bit depth
  if (bits == 0) {
  dict->lookup("BitsPerComponent", &obj1);
  if (obj1.isNull()) {
    obj1.free();
    dict->lookup("BPC", &obj1);
  }
  if (obj1.isInt()) {
    bits = obj1.getInt();
  } else if (mask) {
    bits = 1;
  } else {
    goto err2;
  }
  obj1.free();
  }

  // display a mask
  if (mask) {

    // check for inverted mask
    if (bits != 1)
      goto err1;
    invert = gFalse;
    dict->lookup("Decode", &obj1);
    if (obj1.isNull()) {
      obj1.free();
      dict->lookup("D", &obj1);
    }
    if (obj1.isArray()) {
      obj1.arrayGet(0, &obj2);
      if (obj2.isInt() && obj2.getInt() == 1)
	invert = gTrue;
      obj2.free();
    } else if (!obj1.isNull()) {
      goto err2;
    }
    obj1.free();

    // draw it
    out->drawImageMask(state, ref, str, width, height, invert, inlineImg);

  } else {

    // get color space and color map
    dict->lookup("ColorSpace", &obj1);
    if (obj1.isNull()) {
      obj1.free();
      dict->lookup("CS", &obj1);
    }
    if (obj1.isName()) {
      res->lookupColorSpace(obj1.getNameC(), &obj2);
      if (!obj2.isNull()) {
	obj1.free();
	obj1 = obj2;
      } else {
	obj2.free();
      }
    }
    if (!obj1.isNull()) {
    colorSpace = GfxColorSpace::parse(&obj1);
    } else if (csMode == streamCSDeviceGray) {
      colorSpace = new GfxDeviceGrayColorSpace();
    } else if (csMode == streamCSDeviceRGB) {
      colorSpace = new GfxDeviceRGBColorSpace();
    } else if (csMode == streamCSDeviceCMYK) {
      colorSpace = new GfxDeviceCMYKColorSpace();
    } else {
      colorSpace = NULL;
    }
    obj1.free();
    if (!colorSpace) {
      goto err1;
    }
    dict->lookup("Decode", &obj1);
    if (obj1.isNull()) {
      obj1.free();
      dict->lookup("D", &obj1);
    }
    colorMap = new GfxImageColorMap(bits, &obj1, colorSpace);
    obj1.free();
    if (!colorMap->isOk()) {
      delete colorMap;
      goto err1;
    }

    // get the mask
    haveColorKeyMask = haveExplicitMask = haveSoftMask = gFalse;
    maskStr = NULL; // make gcc happy
    maskWidth = maskHeight = 0; // make gcc happy
    maskInvert = gFalse; // make gcc happy
    maskColorMap = NULL; // make gcc happy
    dict->lookup("Mask", &maskObj);
    dict->lookup("SMask", &smaskObj);
    if (smaskObj.isStream()) {
      // soft mask
      if (inlineImg) {
	goto err1;
      }
      maskStr = smaskObj.getStream();
      maskDict = smaskObj.streamGetDict();
      maskDict->lookup("Width", &obj1);
      if (obj1.isNull()) {
	obj1.free();
	maskDict->lookup("W", &obj1);
      }
      if (!obj1.isInt()) {
	goto err2;
      }
      maskWidth = obj1.getInt();
      obj1.free();
      maskDict->lookup("Height", &obj1);
      if (obj1.isNull()) {
	obj1.free();
	maskDict->lookup("H", &obj1);
      }
      if (!obj1.isInt()) {
	goto err2;
      }
      maskHeight = obj1.getInt();
      obj1.free();
      maskDict->lookup("BitsPerComponent", &obj1);
      if (obj1.isNull()) {
	obj1.free();
	maskDict->lookup("BPC", &obj1);
      }
      if (!obj1.isInt()) {
	goto err2;
      }
      maskBits = obj1.getInt();
      obj1.free();
      maskDict->lookup("ColorSpace", &obj1);
      if (obj1.isNull()) {
	obj1.free();
	maskDict->lookup("CS", &obj1);
      }
      if (obj1.isName()) {
	res->lookupColorSpace(obj1.getNameC(), &obj2);
	if (!obj2.isNull()) {
	  obj1.free();
	  obj1 = obj2;
	} else {
	  obj2.free();
	}
      }
      maskColorSpace = GfxColorSpace::parse(&obj1);
      obj1.free();
      if (!maskColorSpace || maskColorSpace->getMode() != csDeviceGray) {
	goto err1;
      }
      maskDict->lookup("Decode", &obj1);
      if (obj1.isNull()) {
	obj1.free();
	maskDict->lookup("D", &obj1);
      }
      maskColorMap = new GfxImageColorMap(maskBits, &obj1, maskColorSpace);
      obj1.free();
      if (!maskColorMap->isOk()) {
	delete maskColorMap;
	goto err1;
      }
      //~ handle the Matte entry
      haveSoftMask = gTrue;
    } else if (maskObj.isArray()) {
      // color key mask
      for (i = 0;
	   i < maskObj.arrayGetLength() && i < 2*gfxColorMaxComps;
	   ++i) {
	maskObj.arrayGet(i, &obj1);
	maskColors[i] = obj1.getInt();
	obj1.free();
      }
      haveColorKeyMask = gTrue;
    } else if (maskObj.isStream()) {
      // explicit mask
      if (inlineImg) {
	goto err1;
      }
      maskStr = maskObj.getStream();
      maskDict = maskObj.streamGetDict();
      maskDict->lookup("Width", &obj1);
      if (obj1.isNull()) {
	obj1.free();
	maskDict->lookup("W", &obj1);
      }
      if (!obj1.isInt()) {
	goto err2;
      }
      maskWidth = obj1.getInt();
      obj1.free();
      maskDict->lookup("Height", &obj1);
      if (obj1.isNull()) {
	obj1.free();
	maskDict->lookup("H", &obj1);
      }
      if (!obj1.isInt()) {
	goto err2;
      }
      maskHeight = obj1.getInt();
      obj1.free();
      maskDict->lookup("ImageMask", &obj1);
      if (obj1.isNull()) {
	obj1.free();
	maskDict->lookup("IM", &obj1);
      }
      if (!obj1.isBool() || !obj1.getBool()) {
	goto err2;
      }
      obj1.free();
      maskInvert = gFalse;
      maskDict->lookup("Decode", &obj1);
      if (obj1.isNull()) {
	obj1.free();
	maskDict->lookup("D", &obj1);
      }
      if (obj1.isArray()) {
	obj1.arrayGet(0, &obj2);
	if (obj2.isInt() && obj2.getInt() == 1) {
	  maskInvert = gTrue;
	}
	obj2.free();
      } else if (!obj1.isNull()) {
	goto err2;
      }
      obj1.free();
      haveExplicitMask = gTrue;
    }

    // draw it
    if (haveSoftMask) {
      out->drawSoftMaskedImage(state, ref, str, width, height, colorMap,
			       maskStr, maskWidth, maskHeight, maskColorMap);
      delete maskColorMap;
    } else if (haveExplicitMask) {
      out->drawMaskedImage(state, ref, str, width, height, colorMap,
			   maskStr, maskWidth, maskHeight, maskInvert);
    } else {
    out->drawImage(state, ref, str, width, height, colorMap,
		     haveColorKeyMask ? maskColors : (int *)NULL, inlineImg);
    }
    delete colorMap;

    maskObj.free();
    smaskObj.free();
  }

  if ((i = width * height) > 1000) {
    i = 1000;
  }
  updateLevel += i;

  return;

 err2:
  obj1.free();
 err1:
  error(getPos(), "Bad image parameters");
}

void Gfx::doForm(Object *str) {
  Dict *dict;
  Object matrixObj, bboxObj;
  double m[6], bbox[6];
  Object resObj;
  Dict *resDict;
  Object obj1;
  int i;

  // check for excessive recursion
  if (formDepth > 20) {
    return;
  }

  // get stream dict
  dict = str->streamGetDict();

  // check form type
  dict->lookup("FormType", &obj1);
  if (!(obj1.isNull() || (obj1.isInt() && obj1.getInt() == 1))) {
    error(getPos(), "Unknown form type");
  }
  obj1.free();

  // get bounding box
  dict->lookup("BBox", &bboxObj);
  if (!bboxObj.isArray()) {
    matrixObj.free();
    bboxObj.free();
    error(getPos(), "Bad form bounding box");
    return;
  }
  for (i = 0; i < 4; ++i) {
    bboxObj.arrayGet(i, &obj1);
    bbox[i] = obj1.getNum();
    obj1.free();
  }
  bboxObj.free();

  // get matrix
  dict->lookup("Matrix", &matrixObj);
  if (matrixObj.isArray()) {
    for (i = 0; i < 6; ++i) {
      matrixObj.arrayGet(i, &obj1);
      m[i] = obj1.getNum();
      obj1.free();
    }
  } else {
    m[0] = 1; m[1] = 0;
    m[2] = 0; m[3] = 1;
    m[4] = 0; m[5] = 0;
  }
  matrixObj.free();

  // get resources
  dict->lookup("Resources", &resObj);
  resDict = resObj.isDict() ? resObj.getDict() : (Dict *)NULL;

  // draw it
  ++formDepth;
  doForm1(str, resDict, m, bbox);
  --formDepth;

  resObj.free();
}

void Gfx::doAnnot(Object *str, double xMin, double yMin,
		  double xMax, double yMax) {
  Dict *dict, *resDict;
  Object matrixObj, bboxObj, resObj;
  Object obj1;
  double m[6], bbox[6], ictm[6];
  double *ctm;
  double formX0, formY0, formX1, formY1;
  double annotX0, annotY0, annotX1, annotY1;
  double det, x, y, sx, sy;
  int i;

  // get stream dict
  dict = str->streamGetDict();

  // get the form bounding box
  dict->lookup("BBox", &bboxObj);
  if (!bboxObj.isArray()) {
    bboxObj.free();
    error(getPos(), "Bad form bounding box");
    return;
  }
  for (i = 0; i < 4; ++i) {
    bboxObj.arrayGet(i, &obj1);
    bbox[i] = obj1.getNum();
    obj1.free();
  }
  bboxObj.free();

  // get the form matrix
  dict->lookup("Matrix", &matrixObj);
  if (matrixObj.isArray()) {
    for (i = 0; i < 6; ++i) {
      matrixObj.arrayGet(i, &obj1);
      m[i] = obj1.getNum();
      obj1.free();
    }
  } else {
    m[0] = 1; m[1] = 0;
    m[2] = 0; m[3] = 1;
    m[4] = 0; m[5] = 0;
  }
  matrixObj.free();

  // transform the form bbox from form space to user space
  formX0 = bbox[0] * m[0] + bbox[1] * m[2] + m[4];
  formY0 = bbox[0] * m[1] + bbox[1] * m[3] + m[5];
  formX1 = bbox[2] * m[0] + bbox[3] * m[2] + m[4];
  formY1 = bbox[2] * m[1] + bbox[3] * m[3] + m[5];

  // transform the annotation bbox from default user space to user
  // space: (bbox * baseMatrix) * iCTM
  ctm = state->getCTM();
  det = 1 / (ctm[0] * ctm[3] - ctm[1] * ctm[2]);
  ictm[0] = ctm[3] * det;
  ictm[1] = -ctm[1] * det;
  ictm[2] = -ctm[2] * det;
  ictm[3] = ctm[0] * det;
  ictm[4] = (ctm[2] * ctm[5] - ctm[3] * ctm[4]) * det;
  ictm[5] = (ctm[1] * ctm[4] - ctm[0] * ctm[5]) * det;
  x = baseMatrix[0] * xMin + baseMatrix[2] * yMin + baseMatrix[4];
  y = baseMatrix[1] * xMin + baseMatrix[3] * yMin + baseMatrix[5];
  annotX0 = ictm[0] * x + ictm[2] * y + ictm[4];
  annotY0 = ictm[1] * x + ictm[3] * y + ictm[5];
  x = baseMatrix[0] * xMax + baseMatrix[2] * yMax + baseMatrix[4];
  y = baseMatrix[1] * xMax + baseMatrix[3] * yMax + baseMatrix[5];
  annotX1 = ictm[0] * x + ictm[2] * y + ictm[4];
  annotY1 = ictm[1] * x + ictm[3] * y + ictm[5];

  // swap min/max coords
  if (formX0 > formX1) {
    x = formX0; formX0 = formX1; formX1 = x;
  }
  if (formY0 > formY1) {
    y = formY0; formY0 = formY1; formY1 = y;
  }
  if (annotX0 > annotX1) {
    x = annotX0; annotX0 = annotX1; annotX1 = x;
  }
  if (annotY0 > annotY1) {
    y = annotY0; annotY0 = annotY1; annotY1 = y;
  }

  // scale the form to fit the annotation bbox
  if (formX1 == formX0) {
    // this shouldn't happen
    sx = 1;
  } else {
    sx = (annotX1 - annotX0) / (formX1 - formX0);
  }
  if (formY1 == formY0) {
    // this shouldn't happen
    sy = 1;
  } else {
    sy = (annotY1 - annotY0) / (formY1 - formY0);
  }
  m[0] *= sx;
  m[2] *= sx;
  m[4] = (m[4] - formX0) * sx + annotX0;
  m[1] *= sy;
  m[3] *= sy;
  m[5] = (m[5] - formY0) * sy + annotY0;

  // get resources
  dict->lookup("Resources", &resObj);
  resDict = resObj.isDict() ? resObj.getDict() : (Dict *)NULL;

  // draw it
  doForm1(str, resDict, m, bbox);

  resObj.free();
  bboxObj.free();
}

void Gfx::doForm1(Object *str, Dict *resDict, double *matrix, double *bbox) {
  Parser *oldParser;
  double oldBaseMatrix[6];
  int i;

  // push new resources on stack
  pushResources(resDict);

  // save current graphics state
  saveState();

  // kill any pre-existing path
  state->clearPath();

  // save current parser
  oldParser = parser;

  // set form transformation matrix
  state->concatCTM(matrix[0], matrix[1], matrix[2],
		   matrix[3], matrix[4], matrix[5]);
  out->updateCTM(state, matrix[0], matrix[1], matrix[2],
		 matrix[3], matrix[4], matrix[5]);

  // set new base matrix
  for (i = 0; i < 6; ++i) {
    oldBaseMatrix[i] = baseMatrix[i];
    baseMatrix[i] = state->getCTM()[i];
  }

  // set form bounding box
  state->moveTo(bbox[0], bbox[1]);
  state->lineTo(bbox[2], bbox[1]);
  state->lineTo(bbox[2], bbox[3]);
  state->lineTo(bbox[0], bbox[3]);
  state->closePath();
  state->clip();
  out->clip(state);
  state->clearPath();

  // draw the form
  display(str, gFalse);

  // restore base matrix
  for (i = 0; i < 6; ++i) {
    baseMatrix[i] = oldBaseMatrix[i];
  }

  // restore parser
  parser = oldParser;

  // restore graphics state
  restoreState();

  // pop resource stack
  popResources();

  return;
}

//------------------------------------------------------------------------
// in-line image operators
//------------------------------------------------------------------------

void Gfx::opBeginImage(Object args[], int numArgs) {
  Stream *str;
  int c1, c2;

  // build dict/stream
  str = buildImageStream();

  // display the image
  if (str) {
    doImage(NULL, str, gTrue);
  
    // skip 'EI' tag
    c1 = str->getBaseStream()->getChar();
    c2 = str->getBaseStream()->getChar();
    while (!(c1 == 'E' && c2 == 'I') && c2 != EOF) {
      c1 = c2;
      c2 = str->getBaseStream()->getChar();
    }
    delete str;
  }
}

Stream *Gfx::buildImageStream() {
  Object dict;
  Object obj;
  Object dictObj;
  Stream *str;
  GBool isEOF = gFalse;

  dict.initDict(xref);
  while (1) {
    parser->getObj(&obj);
    isEOF = obj.isEOF();
    if (isEOF || obj.isCmd("ID"))
        break;
    if (!obj.isName()) {
      error(getPos(), "Inline image dictionary key must be a name object");
    } else {
      parser->getObj(&dictObj);
      isEOF = dictObj.isEOF();
      if (isEOF)
        break;
      if (dictObj.isError()) {
        // FIXME: we probably should not build a stream in this case either
        // but this behaviour was in poppler for a while so maybe it's ok
        dictObj.free();
        break;
      }
      dict.dictAddOwnVal(obj.getNameC(), &dictObj);
    }
    obj.free();
  }

  obj.free();
  if (isEOF) {
    error(getPos(), "End of file in inline image");
    dict.free();
    return NULL;
  }

  str = new EmbedStream(parser->getStream(), &dict, gFalse, 0);
  str = str->addFilters(&dict);

  return str;
}

void Gfx::opImageData(Object args[], int numArgs) {
  error(getPos(), "Internal: got 'ID' operator");
}

void Gfx::opEndImage(Object args[], int numArgs) {
  error(getPos(), "Internal: got 'EI' operator");
}

//------------------------------------------------------------------------
// type 3 font operators
//------------------------------------------------------------------------

void Gfx::opSetCharWidth(Object args[], int numArgs) {
  out->type3D0(state, args[0].getNum(), args[1].getNum());
}

void Gfx::opSetCacheDevice(Object args[], int numArgs) {
  out->type3D1(state, args[0].getNum(), args[1].getNum(),
	       args[2].getNum(), args[3].getNum(),
	       args[4].getNum(), args[5].getNum());
}

//------------------------------------------------------------------------
// compatibility operators
//------------------------------------------------------------------------

void Gfx::opBeginIgnoreUndef(Object args[], int numArgs) {
  ++ignoreUndef;
}

void Gfx::opEndIgnoreUndef(Object args[], int numArgs) {
  if (ignoreUndef > 0)
    --ignoreUndef;
}

//------------------------------------------------------------------------
// marked content operators
//------------------------------------------------------------------------

void Gfx::opBeginMarkedContent(Object args[], int numArgs) {
  if (printCommands) {
    printf("  marked content: %s ", args[0].getNameC());
    if (numArgs == 2)
      args[2].print(stdout);
    printf("\n");
    fflush(stdout);
  }

  if(numArgs == 2) {
    out->beginMarkedContent(args[0].getNameC(),args[1].getDict());
  } else {
    out->beginMarkedContent(args[0].getNameC());
  }
}

void Gfx::opEndMarkedContent(Object args[], int numArgs) {
  out->endMarkedContent();
}

void Gfx::opMarkPoint(Object args[], int numArgs) {
  if (printCommands) {
    printf("  mark point: %s ", args[0].getNameC());
    if (numArgs == 2)
      args[2].print(stdout);
    printf("\n");
    fflush(stdout);
  }

  if(numArgs == 2) {
    out->markPoint(args[0].getNameC(),args[1].getDict());
  } else {
    out->markPoint(args[0].getNameC());
  }

}

//------------------------------------------------------------------------
// misc
//------------------------------------------------------------------------

void Gfx::saveState() {
  out->saveState(state);
  state = state->save();
}

void Gfx::restoreState() {
  state = state->restore();
  out->restoreState(state);
}

void Gfx::pushResources(Dict *resDict) {
  res = new GfxResources(xref, resDict, res);
}

void Gfx::popResources() {
  GfxResources *resPtr;

  resPtr = res->getNext();
  delete res;
  res = resPtr;
}
