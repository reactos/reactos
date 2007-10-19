//========================================================================
//
// Catalog.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef CATALOG_H
#define CATALOG_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

class XRef;
class Object;
class Page;
class PageAttrs;
struct Ref;
class LinkDest;
class UGooString;
class PageLabelInfo;

//------------------------------------------------------------------------
// NameTree
//------------------------------------------------------------------------

class NameTree {
public:
  NameTree();
  void init(XRef *xref, Object *tree);
  void parse(Object *tree);
  GBool lookup(UGooString *name, Object *obj);
  void free();
  int numEntries() { return length; };
  // iterator accessor
  Object getValue(int i);
  UGooString *getName(int i);

private:
  struct Entry {
    Entry(Array *array, int index);
    ~Entry();
    UGooString *name;
    Object value;
    void free();
    static int cmp(const void *key, const void *entry);
  };

  void addEntry(Entry *entry);

  XRef *xref;
  Object *root;
  Entry **entries;
  int size, length; // size is the number of entries in
                    // the array of Entry*
                    // length is the number of real Entry
};

class EmbFile {
public:
  EmbFile(GooString *name, GooString *description,
	  GooString *createDate,
	  GooString *modDate, Object objStr) :
    m_name(name),
    m_description(description),
    m_createDate(createDate),
    m_modDate(modDate)
  {
    objStr.copy(&m_objStr);
  }

  ~EmbFile()
  {
    delete m_name;
    delete m_description;
    delete m_modDate;
    delete m_createDate;
    m_objStr.free();
  }

  GooString *name() { return m_name; }
  GooString *description() { return m_description; }
  GooString *modDate() { return m_modDate; }
  GooString *createDate() { return m_createDate; }
  Object &streamObject() { return m_objStr; }

private:
  GooString *m_name;
  GooString *m_description;
  GooString *m_createDate;
  GooString *m_modDate;
  Object m_objStr;
};

//------------------------------------------------------------------------
// Catalog
//------------------------------------------------------------------------

class Catalog {
public:

  // Constructor.
  Catalog(XRef *xrefA);

  // Destructor.
  ~Catalog();

  // Is catalog valid?
  GBool isOk() { return ok; }

  // Get number of pages.
  int getNumPages() { return numPages; }

  // Get a page.
  Page *getPage(int i) { return pages[i-1]; }

  // Get the reference for a page object.
  Ref *getPageRef(int i) { return &pageRefs[i-1]; }

  // Return base URI, or NULL if none.
  GooString *getBaseURI() { return baseURI; }

  // Return the contents of the metadata stream, or NULL if there is
  // no metadata.
  GooString *readMetadata();

  // Return the structure tree root object.
  Object *getStructTreeRoot() { return &structTreeRoot; }

  // Find a page, given its object ID.  Returns page number, or 0 if
  // not found.
  int findPage(int num, int gen);

  // Find a named destination.  Returns the link destination, or
  // NULL if <name> is not a destination.
  LinkDest *findDest(UGooString *name);

  // Get the number of embedded files
  int numEmbeddedFiles() { return embeddedFileNameTree.numEntries(); }

  // Get the i'th file embedded (at the Document level) in the document
  EmbFile *embeddedFile(int i);

  // Convert between page indices and page labels.
  GBool labelToIndex(GooString *label, int *index);
  GBool indexToLabel(int index, GooString *label);

  Object *getOutline() { return &outline; }

  Object *getAcroForm() { return &acroForm; }

  enum PageMode {
    pageModeNone,
    pageModeOutlines,
    pageModeThumbs,
    pageModeFullScreen,
    pageModeOC,
    pageModeAttach
  };
  enum PageLayout {
    pageLayoutNone,
    pageLayoutSinglePage,
    pageLayoutOneColumn,
    pageLayoutTwoColumnLeft,
    pageLayoutTwoColumnRight,
    pageLayoutTwoPageLeft,
    pageLayoutTwoPageRight
  };

  // Returns the page mode.
  PageMode getPageMode() { return pageMode; }
  PageLayout getPageLayout() { return pageLayout; }

private:

  XRef *xref;			// the xref table for this PDF file
  Page **pages;			// array of pages
  Ref *pageRefs;		// object ID for each page
  int numPages;			// number of pages
  int pagesSize;		// size of pages array
  Object dests;			// named destination dictionary
  NameTree destNameTree;	// named destination name-tree
  NameTree embeddedFileNameTree;  // embedded file name-tree
  GooString *baseURI;		// base URI for URI-type links
  Object metadata;		// metadata stream
  Object structTreeRoot;	// structure tree root dictionary
  Object outline;		// outline dictionary
  Object acroForm;		// AcroForm dictionary
  GBool ok;			// true if catalog is valid
  PageLabelInfo *pageLabelInfo; // info about page labels
  PageMode pageMode;		// page mode
  PageLayout pageLayout;	// page layout

  int readPageTree(Dict *pages, PageAttrs *attrs, int start, int callDepth);
  Object *findDestInTree(Object *tree, GooString *name, Object *obj);
};

#endif
