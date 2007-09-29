//========================================================================
//
// Annot.cc
//
// Copyright 2000-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdlib.h>
#include "goo/gmem.h"
#include "Object.h"
#include "Catalog.h"
#include "Gfx.h"
#include "Lexer.h"
#include "UGooString.h"
#include "Annot.h"

//------------------------------------------------------------------------
// Annot
//------------------------------------------------------------------------

Annot::Annot(XRef *xrefA, Dict *acroForm, Dict *dict) {
  Object apObj, asObj, obj1, obj2;
  GBool regen, isTextField;
  double t;

  ok = gTrue;
  xref = xrefA;
  appearBuf = NULL;

  if (dict->lookup("Rect", &obj1)->isArray() &&
      obj1.arrayGetLength() == 4) {
    readArrayNum(&obj1, 0, &xMin);
    readArrayNum(&obj1, 1, &yMin);
    readArrayNum(&obj1, 2, &xMax);
    readArrayNum(&obj1, 3, &yMax);
    if (ok) {
    if (xMin > xMax) {
      t = xMin; xMin = xMax; xMax = t;
    }
    if (yMin > yMax) {
      t = yMin; yMin = yMax; yMax = t;
    }
  } else {
    //~ this should return an error
    xMin = yMin = 0;
    xMax = yMax = 1;
  }
  } else {
    //~ this should return an error
    xMin = yMin = 0;
    xMax = yMax = 1;
  }
  obj1.free();

  // check if field apperances need to be regenerated
  regen = gFalse;
  if (acroForm) {
    acroForm->lookup("NeedAppearances", &obj1);
    if (obj1.isBool() && obj1.getBool()) {
      regen = gTrue;
    }
    obj1.free();
  }

  // check for a text-type field
  isTextField = dict->lookup("FT", &obj1)->isName("Tx");
  obj1.free();

#if 0 //~ appearance stream generation is not finished yet
  if (regen && isTextField) {
    generateAppearance(acroForm, dict);
  } else {
#endif
    if (dict->lookup("AP", &apObj)->isDict()) {
      if (dict->lookup("AS", &asObj)->isName()) {
	if (apObj.dictLookup("N", &obj1)->isDict()) {
	  if (obj1.dictLookupNF(asObj.getNameC(), &obj2)->isRef()) {
	    obj2.copy(&appearance);
	    ok = gTrue;
	  } else {
	    obj2.free();
	    if (obj1.dictLookupNF("Off", &obj2)->isRef()) {
	      obj2.copy(&appearance);
	      ok = gTrue;
	    }
	  }
	  obj2.free();
	}
	obj1.free();
      } else {
	if (apObj.dictLookupNF("N", &obj1)->isRef()) {
	  obj1.copy(&appearance);
	  ok = gTrue;
	}
	obj1.free();
      }
      asObj.free();
    }
    apObj.free();
#if 0 //~ appearance stream generation is not finished yet
  }
#endif
}

void Annot::readArrayNum(Object *pdfArray, int key, double *value) {
  Object valueObject;

  pdfArray->arrayGet(key, &valueObject);
  if (valueObject.isNum()) {
    *value = valueObject.getNum();
  } else {
    *value = 0;
    ok = gFalse;
  }
  valueObject.free();
}

Annot::~Annot() {
  appearance.free();
  if (appearBuf) {
    delete appearBuf;
  }
}

void Annot::generateAppearance(Dict *acroForm, Dict *dict) {
  MemStream *appearStream;
  Object daObj, vObj, drObj, appearDict, obj1, obj2;
  GooString *daStr, *daStr1, *vStr, *s;
  char buf[256];
  double fontSize;
  int c;
  int i0, i1;

  //~ DA can be inherited
  if (dict->lookup("DA", &daObj)->isString()) {
    daStr = daObj.getString();

    // look for a font size
    //~ may want to parse the DS entry in place of this (if it exists)
    daStr1 = NULL;
    fontSize = 10;
    for (i1 = daStr->getLength() - 2; i1 >= 0; --i1) {
      if (daStr->getChar(i1) == 'T' && daStr->getChar(i1+1) == 'f') {
	for (--i1; i1 >= 0 && Lexer::isSpace(daStr->getChar(i1)); --i1) ;
	for (i0 = i1; i0 >= 0 && !Lexer::isSpace(daStr->getChar(i0)); --i0) ;
	if (i0 >= 0) {
	  ++i0;
	  ++i1;
	  s = new GooString(daStr, i0, i1 - i0);
	  fontSize = atof(s->getCString());
	  delete s;

	  // autosize the font
	  if (fontSize == 0) {
	    fontSize = 0.67 * (yMax - yMin);
	    daStr1 = new GooString(daStr, 0, i0);
	    sprintf(buf, "%.2f", fontSize);
	    daStr1->append(buf);
	    daStr1->append(daStr->getCString() + i1,
			   daStr->getLength() - i1);
	  }
	}
	break;
      }
    }

    // build the appearance stream contents
    appearBuf = new GooString();
    appearBuf->append("/Tx BMC\n");
    appearBuf->append("q BT\n");
    appearBuf->append(daStr1 ? daStr1 : daStr)->append("\n");
    if (dict->lookup("V", &vObj)->isString()) {
      //~ handle quadding -- this requires finding the font and using
      //~   the encoding and char widths
      sprintf(buf, "1 0 0 1 %.2f %.2f Tm\n", 2.0, yMax - yMin - fontSize);
      appearBuf->append(buf);
      sprintf(buf, "%g TL\n", fontSize);
      appearBuf->append(buf);
      vStr = vObj.getString();
      i0 = 0;
      while (i0 < vStr->getLength()) {
	for (i1 = i0;
	     i1 < vStr->getLength() &&
	       vStr->getChar(i1) != '\n' && vStr->getChar(i1) != '\r';
	     ++i1) ;
	if (i0 > 0) {
	  appearBuf->append("T*\n");
	}
	appearBuf->append('(');
	for (; i0 < i1; ++i0) {
	  c = vStr->getChar(i0);
	  if (c == '(' || c == ')' || c == '\\') {
	    appearBuf->append('\\');
	    appearBuf->append(c);
	  } else if (c < 0x20 || c >= 0x80) {
	    sprintf(buf, "\\%03o", c);
	    appearBuf->append(buf);
	  } else {
	    appearBuf->append(c);
	  }
	}
	appearBuf->append(") Tj\n");
	if (i1 + 1 < vStr->getLength() &&
	    vStr->getChar(i1) == '\r' && vStr->getChar(i1 + 1) == '\n') {
	  i0 = i1 + 2;
	} else {
	  i0 = i1 + 1;
	}
      }
    }
    vObj.free();
    appearBuf->append("ET Q\n");
    appearBuf->append("EMC\n");

    // build the appearance stream dictionary
    appearDict.initDict(xref);
    appearDict.dictAdd("Length", obj1.initInt(appearBuf->getLength()));
    appearDict.dictAdd("Subtype", obj1.initName("Form"));
    obj1.initArray(xref);
    obj1.arrayAdd(obj2.initReal(0));
    obj1.arrayAdd(obj2.initReal(0));
    obj1.arrayAdd(obj2.initReal(xMax - xMin));
    obj1.arrayAdd(obj2.initReal(yMax - yMin));
    appearDict.dictAdd("BBox", &obj1);

    // find the resource dictionary
    dict->lookup("DR", &drObj);
    if (!drObj.isDict()) {
      dict->lookup("Parent", &obj1);
      while (obj1.isDict()) {
	drObj.free();
	obj1.dictLookup("DR", &drObj);
	if (drObj.isDict()) {
	  break;
	}
	obj1.dictLookup("Parent", &obj2);
	obj1.free();
	obj1 = obj2;
      }
      obj1.free();
      if (!drObj.isDict()) {
	if (acroForm) {
	  drObj.free();
	  acroForm->lookup("DR", &drObj);
	}
      }
    }
    if (drObj.isDict()) {
      appearDict.dictAdd("Resources", drObj.copy(&obj1));
    }
    drObj.free();

    // build the appearance stream
    appearStream = new MemStream(appearBuf->getCString(), 0,
				 appearBuf->getLength(), &appearDict);
    appearance.initStream(appearStream);
    ok = gTrue;

    if (daStr1) {
      delete daStr1;
    }
  }
  daObj.free();
}

void Annot::draw(Gfx *gfx) {
  Object obj;

  if (appearance.fetch(xref, &obj)->isStream()) {
    gfx->doAnnot(&obj, xMin, yMin, xMax, yMax);
  }
  obj.free();
}

//------------------------------------------------------------------------
// Annots
//------------------------------------------------------------------------

Annots::Annots(XRef *xref, Catalog *catalog, Object *annotsObj) {
  Dict *acroForm;
  Annot *annot;
  Object obj1;
  int size;
  int i;

  annots = NULL;
  size = 0;
  nAnnots = 0;

  acroForm = catalog->getAcroForm()->isDict() ?
               catalog->getAcroForm()->getDict() : NULL;
  if (annotsObj->isArray()) {
    for (i = 0; i < annotsObj->arrayGetLength(); ++i) {
      if (annotsObj->arrayGet(i, &obj1)->isDict()) {
	annot = new Annot(xref, acroForm, obj1.getDict());
	if (annot->isOk()) {
	  if (nAnnots >= size) {
	    size += 16;
	    annots = (Annot **)greallocn(annots, size, sizeof(Annot *));
	  }
	  annots[nAnnots++] = annot;
	} else {
	  delete annot;
	}
      }
      obj1.free();
    }
  }
}

Annots::~Annots() {
  int i;

  for (i = 0; i < nAnnots; ++i) {
    delete annots[i];
  }
  gfree(annots);
}
