//========================================================================
//
// Catalog.cc
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stddef.h>
#include <stdlib.h>
#include "goo/gmem.h"
#include "Object.h"
#include "XRef.h"
#include "Array.h"
#include "Dict.h"
#include "Page.h"
#include "Error.h"
#include "Link.h"
#include "PageLabelInfo.h"
#include "UGooString.h"
#include "Catalog.h"

// This define is used to limit the depth of recursive readPageTree calls
// This is needed because the page tree nodes can reference their parents
// leaving us in an infinite loop
// Most sane pdf documents don't have a call depth higher than 10
#define MAX_CALL_DEPTH 1000

//------------------------------------------------------------------------
// Catalog
//------------------------------------------------------------------------

Catalog::Catalog(XRef *xrefA) {
  Object catDict, pagesDict;
  Object obj, obj2;
  int numPages0;
  int i;

  ok = gTrue;
  xref = xrefA;
  pages = NULL;
  pageRefs = NULL;
  numPages = pagesSize = 0;
  baseURI = NULL;
  pageLabelInfo = NULL;

  xref->getCatalog(&catDict);
  if (!catDict.isDict()) {
    error(-1, "Catalog object is wrong type (%s)", catDict.getTypeName());
    goto err1;
  }

  // read page tree
  catDict.dictLookup("Pages", &pagesDict);
  // This should really be isDict("Pages"), but I've seen at least one
  // PDF file where the /Type entry is missing.
  if (!pagesDict.isDict()) {
    error(-1, "Top-level pages object is wrong type (%s)",
	  pagesDict.getTypeName());
    goto err2;
  }
  pagesDict.dictLookup("Count", &obj);
  // some PDF files actually use real numbers here ("/Count 9.0")
  if (!obj.isNum()) {
    error(-1, "Page count in top-level pages object is wrong type (%s)",
	  obj.getTypeName());
    goto err3;
  }
  pagesSize = numPages0 = (int)obj.getNum();
  obj.free();
  pages = (Page **)gmallocn(pagesSize, sizeof(Page *));
  pageRefs = (Ref *)gmallocn(pagesSize, sizeof(Ref));
  for (i = 0; i < pagesSize; ++i) {
    pages[i] = NULL;
    pageRefs[i].num = -1;
    pageRefs[i].gen = -1;
  }
  numPages = readPageTree(pagesDict.getDict(), NULL, 0, 0);
  if (numPages != numPages0) {
    error(-1, "Page count in top-level pages object is incorrect");
  }
  pagesDict.free();

  // read named destination dictionary
  catDict.dictLookup("Dests", &dests);

  // read root of named destination tree - PDF1.6 table 3.28
  if (catDict.dictLookup("Names", &obj)->isDict()) {
    obj.dictLookup("Dests", &obj2);
    destNameTree.init(xref, &obj2);
    obj2.free();
    obj.dictLookup("EmbeddedFiles", &obj2);
    embeddedFileNameTree.init(xref, &obj2);
    obj2.free();
  }
  obj.free();

  if (catDict.dictLookup("PageLabels", &obj)->isDict())
    pageLabelInfo = new PageLabelInfo(&obj, numPages);
  obj.free();

  // read page mode
  pageMode = pageModeNone;
  if (catDict.dictLookup("PageMode", &obj)->isName()) {
    if (obj.isName("UseNone"))
      pageMode = pageModeNone;
    else if (obj.isName("UseOutlines"))
      pageMode = pageModeOutlines;
    else if (obj.isName("UseThumbs"))
      pageMode = pageModeThumbs;
    else if (obj.isName("FullScreen"))
      pageMode = pageModeFullScreen;
    else if (obj.isName("UseOC"))
      pageMode = pageModeOC;
    else if (obj.isName("UseAttachments"))
      pageMode = pageModeAttach;
  }
  obj.free();

  pageLayout = pageLayoutNone;
  if (catDict.dictLookup("PageLayout", &obj)->isName()) {
    if (obj.isName("SinglePage"))
      pageLayout = pageLayoutSinglePage;
    if (obj.isName("OneColumn"))
      pageLayout = pageLayoutOneColumn;
    if (obj.isName("TwoColumnLeft"))
      pageLayout = pageLayoutTwoColumnLeft;
    if (obj.isName("TwoColumnRight"))
      pageLayout = pageLayoutTwoColumnRight;
    if (obj.isName("TwoPageLeft"))
      pageLayout = pageLayoutTwoPageLeft;
    if (obj.isName("TwoPageRight"))
      pageLayout = pageLayoutTwoPageRight;
  }
  obj.free();

  // read base URI
  if (catDict.dictLookup("URI", &obj)->isDict()) {
    if (obj.dictLookup("Base", &obj2)->isString()) {
      baseURI = obj2.getString()->copy();
    }
    obj2.free();
  }
  obj.free();

  // get the metadata stream
  catDict.dictLookup("Metadata", &metadata);

  // get the structure tree root
  catDict.dictLookup("StructTreeRoot", &structTreeRoot);

  // get the outline dictionary
  catDict.dictLookup("Outlines", &outline);

  // get the AcroForm dictionary
  catDict.dictLookup("AcroForm", &acroForm);

  catDict.free();
  return;

 err3:
  obj.free();
 err2:
  pagesDict.free();
 err1:
  catDict.free();
  dests.initNull();
  ok = gFalse;
}

Catalog::~Catalog() {
  int i;

  if (pages) {
    for (i = 0; i < pagesSize; ++i) {
      if (pages[i]) {
	delete pages[i];
      }
    }
    gfree(pages);
    gfree(pageRefs);
  }
  dests.free();
  destNameTree.free();
  embeddedFileNameTree.free();
  if (baseURI) {
    delete baseURI;
  }
  delete pageLabelInfo;
  metadata.free();
  structTreeRoot.free();
  outline.free();
  acroForm.free();
}

GooString *Catalog::readMetadata() {
  GooString *s;
  Dict *dict;
  Object obj;
  int c;

  if (!metadata.isStream()) {
    return NULL;
  }
  dict = metadata.streamGetDict();
  if (!dict->lookup("Subtype", &obj)->isName("XML")) {
    error(-1, "Unknown Metadata type: '%s'",
	  obj.isName() ? obj.getNameC() : "???");
  }
  obj.free();
  s = new GooString();
  metadata.streamReset();
  while ((c = metadata.streamGetChar()) != EOF) {
    s->append(c);
  }
  metadata.streamClose();
  return s;
}

int Catalog::readPageTree(Dict *pagesDict, PageAttrs *attrs, int start, int callDepth) {
  Object kids;
  Object kid;
  Object kidRef;
  PageAttrs *attrs1, *attrs2;
  Page *page;
  int i, j;

  attrs1 = new PageAttrs(attrs, pagesDict);
  pagesDict->lookup("Kids", &kids);
  if (!kids.isArray()) {
    error(-1, "Kids object (page %d) is wrong type (%s)",
	  start+1, kids.getTypeName());
    goto err1;
  }
  for (i = 0; i < kids.arrayGetLength(); ++i) {
    kids.arrayGet(i, &kid);
    if (kid.isDict("Page")) {
      attrs2 = new PageAttrs(attrs1, kid.getDict());
      page = new Page(xref, start+1, kid.getDict(), attrs2);
      if (!page->isOk()) {
	++start;
	goto err3;
      }
      if (start >= pagesSize) {
	pagesSize += 32;
	pages = (Page **)greallocn(pages, pagesSize, sizeof(Page *));
	pageRefs = (Ref *)greallocn(pageRefs, pagesSize, sizeof(Ref));
	for (j = pagesSize - 32; j < pagesSize; ++j) {
	  pages[j] = NULL;
	  pageRefs[j].num = -1;
	  pageRefs[j].gen = -1;
	}
      }
      pages[start] = page;
      kids.arrayGetNF(i, &kidRef);
      if (kidRef.isRef()) {
	pageRefs[start].num = kidRef.getRefNum();
	pageRefs[start].gen = kidRef.getRefGen();
      }
      kidRef.free();
      ++start;
    // This should really be isDict("Pages"), but I've seen at least one
    // PDF file where the /Type entry is missing.
    } else if (kid.isDict()) {
      if (callDepth > MAX_CALL_DEPTH) {
        error(-1, "Limit of %d recursive calls reached while reading the page tree. If your document is correct and not a test to try to force a crash, please report a bug.", MAX_CALL_DEPTH);
      } else {
        if ((start = readPageTree(kid.getDict(), attrs1, start, callDepth + 1))
	  < 0)
	goto err2;
      }
    } else {
      error(-1, "Kid object (page %d) is wrong type (%s)",
	    start+1, kid.getTypeName());
    }
    kid.free();
  }
  delete attrs1;
  kids.free();
  return start;

 err3:
  delete page;
 err2:
  kid.free();
 err1:
  kids.free();
  delete attrs1;
  ok = gFalse;
  return -1;
}

int Catalog::findPage(int num, int gen) {
  int i;

  for (i = 0; i < numPages; ++i) {
    if (pageRefs[i].num == num && pageRefs[i].gen == gen)
      return i + 1;
  }
  return 0;
}

LinkDest *Catalog::findDest(UGooString *name) {
  LinkDest *dest;
  Object obj1, obj2;
  GBool found;

  // try named destination dictionary then name tree
  found = gFalse;
  if (dests.isDict()) {
    if (!dests.dictLookup(*name, &obj1)->isNull())
      found = gTrue;
    else
      obj1.free();
  }
  if (!found) {
    if (destNameTree.lookup(name, &obj1))
      found = gTrue;
    else
      obj1.free();
  }
  if (!found)
    return NULL;

  // construct LinkDest
  dest = NULL;
  if (obj1.isArray()) {
    dest = new LinkDest(obj1.getArray());
  } else if (obj1.isDict()) {
    if (obj1.dictLookup("D", &obj2)->isArray())
      dest = new LinkDest(obj2.getArray());
    else
      error(-1, "Bad named destination value");
    obj2.free();
  } else {
    error(-1, "Bad named destination value");
  }
  obj1.free();
  if (dest && !dest->isOk()) {
    delete dest;
    dest = NULL;
  }

  return dest;
}

EmbFile *Catalog::embeddedFile(int i)
{
    Object efDict;
    Object fileSpec;
    Object fileDesc;
    Object paramDict;
    Object paramObj;
    Object strObj;
    Object obj, obj2;
    obj = embeddedFileNameTree.getValue(i);
    GooString *fileName = new GooString();
    char *descString = embeddedFileNameTree.getName(i)->getCStringCopy();
    GooString *desc = new GooString(descString);
    delete[] descString;
    GooString *createDate = new GooString();
    GooString *modDate = new GooString();
    Stream *efStream = NULL;
    if (obj.isRef()) {
	if (obj.fetch(xref, &efDict)->isDict()) {
	    // efDict matches Table 3.40 in the PDF1.6 spec
	    efDict.dictLookup("F", &fileSpec);
	    if (fileSpec.isString()) {
		delete fileName;
		fileName = new GooString(fileSpec.getString());
	    }
	    fileSpec.free();

	    // the logic here is that the description from the name
	    // dictionary is used if we don't have a more specific
	    // description - see the Note: on page 157 of the PDF1.6 spec
	    efDict.dictLookup("Desc", &fileDesc);
	    if (fileDesc.isString()) {
		delete desc;
		desc = new GooString(fileDesc.getString());
	    } else {
		efDict.dictLookup("Description", &fileDesc);
		if (fileDesc.isString()) {
		    delete desc;
		    desc = new GooString(fileDesc.getString());
		}
	    }
	    fileDesc.free();
	    
	    efDict.dictLookup("EF", &obj2);
	    if (obj2.isDict()) {
		// This gives us the raw data stream bytes

		obj2.dictLookup("F", &strObj);
		if (strObj.isStream()) {
		    efStream = strObj.getStream();
		}

                if (!efStream) {
                    delete desc;
                    delete modDate;
                    delete createDate;
                    delete fileName;
                    return NULL;
                }

		// dataDict corresponds to Table 3.41 in the PDF1.6 spec.
		Dict *dataDict = efStream->getDict();

		// subtype is normally mimetype. You can extract it with code like this:
		// Object subtypeName;
		// dataDict->lookup( "Subtype", &subtypeName );
		// It is optional, so this will sometimes return a null object
		// if (subtypeName.isName()) {
		//        std::cout << "got subtype name: " << subtypeName.getName() << std::endl;
		// }

		// paramDict corresponds to Table 3.42 in the PDF1.6 spec
		Object paramDict;
		dataDict->lookup( "Params", &paramDict );
		if (paramDict.isDict()) {
		    paramDict.dictLookup("ModDate", &paramObj);
		    if (paramObj.isString()) {
			delete modDate;
		        modDate = new GooString(paramObj.getString());
		    }
		    paramObj.free();
		    paramDict.dictLookup("CreationDate", &paramObj);
		    if (paramObj.isString()) {
			delete createDate;
		        createDate = new GooString(paramObj.getString());
		    }
		    paramObj.free();
		}
		paramDict.free();
	    }
	    efDict.free();
	    obj2.free();
	}
    }
    EmbFile *embeddedFile = new EmbFile(fileName, desc, createDate, modDate, strObj);
    strObj.free();
    return embeddedFile;
}

NameTree::NameTree(void)
{
  size = 0;
  length = 0;
  entries = NULL;
}

NameTree::Entry::Entry(Array *array, int index) {
    GooString n;
    if (!array->getString(index, &n) || !array->getNF(index + 1, &value)) {
      Object aux;
      array->get(index, &aux);
      if (aux.isString() && array->getNF(index + 1, &value) )
      {
        n.append(aux.getString());
      }
      else
        error(-1, "Invalid page tree");
    }
    name = new UGooString(n);
}

NameTree::Entry::~Entry() {
  value.free();
  delete name;
}

void NameTree::addEntry(Entry *entry)
{
  if (length == size) {
    if (length == 0) {
      size = 8;
    } else {
      size *= 2;
    }
    entries = (Entry **) grealloc (entries, sizeof (Entry *) * size);
  }

  entries[length] = entry;
  ++length;
}

void NameTree::init(XRef *xrefA, Object *tree) {
  xref = xrefA;
  parse(tree);
}

void NameTree::parse(Object *tree) {
  Object names;
  Object kids, kid;
  int i;

  if (!tree->isDict())
    return;

  // leaf node
  if (tree->dictLookup("Names", &names)->isArray()) {
    for (i = 0; i < names.arrayGetLength(); i += 2) {
      NameTree::Entry *entry;

      entry = new Entry(names.getArray(), i);
      addEntry(entry);
    }
  }
  names.free();

  // root or intermediate node
  if (tree->dictLookup("Kids", &kids)->isArray()) {
    for (i = 0; i < kids.arrayGetLength(); ++i) {
      if (kids.arrayGet(i, &kid)->isDict())
	parse(&kid);
      kid.free();
    }
  }
  kids.free();
}

int NameTree::Entry::cmp(const void *voidKey, const void *voidEntry)
{
  UGooString *key = (UGooString *) voidKey;
  Entry *entry = *(NameTree::Entry **) voidEntry;

  return key->cmp(entry->name);
}

GBool NameTree::lookup(UGooString *name, Object *obj)
{
  Entry **entry;
  char *cname;

  entry = (Entry **) bsearch(name, entries,
			     length, sizeof(Entry *), Entry::cmp);
  if (entry != NULL) {
    (*entry)->value.fetch(xref, obj);
    return gTrue;
  } else {
    cname = name->getCStringCopy();
    printf("failed to look up %s\n", cname);
    delete[] cname;
    obj->initNull();
    return gFalse;
  }
}

Object NameTree::getValue(int index)
{
  if (index < length) {
    return entries[index]->value;
  } else {
    return Object();
  }
}

UGooString *NameTree::getName(int index)
{
    if (index < length) {
	return entries[index]->name;
    } else {
	return NULL;
    }
}

void NameTree::free()
{
  int i;

  for (i = 0; i < length; i++)
    delete entries[i];

  gfree(entries);
}

GBool Catalog::labelToIndex(GooString *label, int *index)
{
  char *end;

  if (pageLabelInfo != NULL) {
    if (!pageLabelInfo->labelToIndex(label, index))
      return gFalse;
  } else {
    *index = strtol(label->getCString(), &end, 10) - 1;
    if (*end != '\0')
      return gFalse;
  }

  if (*index < 0 || *index >= numPages)
    return gFalse;

  return gTrue;
}

GBool Catalog::indexToLabel(int index, GooString *label)
{
  char buffer[32];

  if (index < 0 || index >= numPages)
    return gFalse;

  if (pageLabelInfo != NULL) {
    return pageLabelInfo->indexToLabel(index, label);
  } else {
    snprintf(buffer, sizeof (buffer), "%d", index + 1);
    label->append(buffer);	      
    return gTrue;
  }
}
