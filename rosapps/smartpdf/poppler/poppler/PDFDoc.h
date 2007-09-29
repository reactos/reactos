//========================================================================
//
// PDFDoc.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef PDFDOC_H
#define PDFDOC_H

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <stdio.h>
#include "XRef.h"
#include "Catalog.h"
#include "Page.h"
#include "Annot.h"

class GooString;
class BaseStream;
class OutputDev;
class Links;
class LinkAction;
class LinkDest;
class Outline;

//------------------------------------------------------------------------
// PDFDoc
//------------------------------------------------------------------------

class PDFDoc {
public:

  PDFDoc(GooString *fileNameA, GooString *ownerPassword = NULL,
	 GooString *userPassword = NULL, void *guiDataA = NULL);

#ifdef WIN32
  PDFDoc(wchar_t *fileNameA, int fileNameLen, GooString *ownerPassword = NULL,
	 GooString *userPassword = NULL, void *guiDataA = NULL);
#endif

  PDFDoc(BaseStream *strA, GooString *ownerPassword = NULL,
	 GooString *userPassword = NULL, void *guiDataA = NULL);
  ~PDFDoc();

  // Was PDF document successfully opened?
  GBool isOk() { return ok; }

  // Get the error code (if isOk() returns false).
  int getErrorCode() { return errCode; }

  // Get file name.
  GooString *getFileName() { return fileName; }

  // Get the xref table.
  XRef *getXRef() { return xref; }

  // Get catalog.
  Catalog *getCatalog() { return catalog; }

  // Get base stream.
  BaseStream *getBaseStream() { return str; }

  // Get page parameters.
  double getPageMediaWidth(int page)
    { return catalog->getPage(page)->getMediaWidth(); }
  double getPageMediaHeight(int page)
    { return catalog->getPage(page)->getMediaHeight(); }
  double getPageCropWidth(int page)
    { return catalog->getPage(page)->getCropWidth(); }
  double getPageCropHeight(int page)
    { return catalog->getPage(page)->getCropHeight(); }
  int getPageRotate(int page)
    { return catalog->getPage(page)->getRotate(); }

  // Get number of pages.
  int getNumPages() { return catalog->getNumPages(); }

  // Return the contents of the metadata stream, or NULL if there is
  // no metadata.
  GooString *readMetadata() { return catalog->readMetadata(); }

  // Return the structure tree root object.
  Object *getStructTreeRoot() { return catalog->getStructTreeRoot(); }

  // Display a page.
  void displayPage(OutputDev *out, int page, double hDPI, double vDPI,
		   int rotate, GBool useMediaBox, GBool crop, GBool doLinks,
		   GBool (*abortCheckCbk)(void *data) = NULL,
		   void *abortCheckCbkData = NULL,
                   GBool (*annotDisplayDecideCbk)(Annot *annot, void *user_data) = NULL,
                   void *annotDisplayDecideCbkData = NULL);

  // Display a range of pages.
  void displayPages(OutputDev *out, int firstPage, int lastPage,
		    double hDPI, double vDPI, int rotate,
		    GBool useMediaBox, GBool crop, GBool doLinks,
		    GBool (*abortCheckCbk)(void *data) = NULL,
		    void *abortCheckCbkData = NULL,
                    GBool (*annotDisplayDecideCbk)(Annot *annot, void *user_data) = NULL,
                    void *annotDisplayDecideCbkData = NULL);

  // Display part of a page.
  void displayPageSlice(OutputDev *out, int page,
			double hDPI, double vDPI,
			int rotate, GBool useMediaBox, GBool crop, GBool doLinks,
			int sliceX, int sliceY, int sliceW, int sliceH,
			GBool (*abortCheckCbk)(void *data) = NULL,
			void *abortCheckCbkData = NULL,
                        GBool (*annotDisplayDecideCbk)(Annot *annot, void *user_data) = NULL,
                        void *annotDisplayDecideCbkData = NULL);

  // Find a page, given its object ID.  Returns page number, or 0 if
  // not found.
  int findPage(int num, int gen) { return catalog->findPage(num, gen); }

  // Returns the links for the current page, transferring ownership to
  // the caller.
  Links *takeLinks();

  // Find a named destination.  Returns the link destination, or
  // NULL if <name> is not a destination.
  LinkDest *findDest(UGooString *name)
    { return catalog->findDest(name); }

#ifndef DISABLE_OUTLINE
  // Return the outline object.
  Outline *getOutline() { return outline; }
#endif

  // Is the file encrypted?
  GBool isEncrypted() { return xref->isEncrypted(); }

  // Check various permissions.
  GBool okToPrint(GBool ignoreOwnerPW = gFalse)
    { return xref->okToPrint(ignoreOwnerPW); }
  GBool okToPrintHighRes(GBool ignoreOwnerPW = gFalse)
    { return xref->okToPrintHighRes(ignoreOwnerPW); }
  GBool okToChange(GBool ignoreOwnerPW = gFalse)
    { return xref->okToChange(ignoreOwnerPW); }
  GBool okToCopy(GBool ignoreOwnerPW = gFalse)
    { return xref->okToCopy(ignoreOwnerPW); }
  GBool okToAddNotes(GBool ignoreOwnerPW = gFalse)
    { return xref->okToAddNotes(ignoreOwnerPW); }
  GBool okToFillForm(GBool ignoreOwnerPW = gFalse)
    { return xref->okToFillForm(ignoreOwnerPW); }
  GBool okToAccessibility(GBool ignoreOwnerPW = gFalse)
    { return xref->okToAccessibility(ignoreOwnerPW); }
  GBool okToAssemble(GBool ignoreOwnerPW = gFalse)
    { return xref->okToAssemble(ignoreOwnerPW); }


  // Is this document linearized?
  GBool isLinearized();

  // Return the document's Info dictionary (if any).
  Object *getDocInfo(Object *obj) { return xref->getDocInfo(obj); }
  Object *getDocInfoNF(Object *obj) { return xref->getDocInfoNF(obj); }

  // Return the PDF version specified by the file.
  double getPDFVersion() { return pdfVersion; }

  // Save this file with another name.
  GBool saveAs(GooString *name);

  // Return a pointer to the GUI (XPDFCore or WinPDFCore object).
  void *getGUIData() { return guiData; }

private:

  GBool setup(GooString *ownerPassword, GooString *userPassword);
  GBool checkFooter();
  void checkHeader();
  GBool checkEncryption(GooString *ownerPassword, GooString *userPassword);
  void getLinks(Page *page);

  GooString *fileName;
  FILE *file;
  BaseStream *str;
  void *guiData;
  double pdfVersion;
  XRef *xref;
  Catalog *catalog;
  Links *links;
#ifndef DISABLE_OUTLINE
  Outline *outline;
#endif

  GBool ok;
  int errCode;
};

#endif
