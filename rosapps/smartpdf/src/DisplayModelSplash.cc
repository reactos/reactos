/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
#include "DisplayModelSplash.h"
#include "str_util.h"

#include "GlobalParams.h"
#include "GooMutex.h"
#include "GooString.h"
#include "Link.h"
#include "Object.h" /* must be included before SplashOutputDev.h because of sloppiness in SplashOutputDev.h */
#include "PDFDoc.h"
#include "SplashBitmap.h"
#include "SplashOutputDev.h"
#include "TextOutputDev.h"

#ifdef __GNUC__
#include <ctype.h>
#endif

#include <assert.h>
#include <stdlib.h> /* malloc etc. */

#define ACTION_NEXT_PAGE    "NextPage"
#define ACTION_PREV_PAGE    "PrevPage"
#define ACTION_FIRST_PAGE   "FirstPage"
#define ACTION_LAST_PAGE    "LastPage"

void CDECL error(int pos, char *msg, ...) {
    va_list args;
    char        buf[4096], *p = buf;

    // NB: this can be called before the globalParams object is created
    if (globalParams && globalParams->getErrQuiet()) {
        return;
    }

    if (pos >= 0) {
        p += _snprintf(p, sizeof(buf)-1, "Error (%d): ", pos);
        *p   = '\0';
        OutputDebugString((TCHAR*)p); /* @note: TCHAR* cast */
    } else {
        OutputDebugString(TEXT("Error: ")); /* @note: TEXT() cast */
    }

    p = buf;
    va_start(args, msg);
    p += _vsnprintf(p, sizeof(buf) - 1, msg, args);
    while ( p > buf  &&  isspace(p[-1]) )
            *--p = '\0';
    *p++ = '\r';
    *p++ = '\n';
    *p   = '\0';
    OutputDebugString((TCHAR*)buf); /* @note: TCHAR* cast */
    va_end(args);
}

#if 0
static Links *GetLinksForPage(PDFDoc *doc, int pageNo)
{
    Object obj;
    Catalog *catalog = doc->getCatalog();
    Page *page = catalog->getPage(pageNo);
    Links *links = new Links(page->getAnnots(&obj), catalog->getBaseURI());
    obj.free();
    return links;
}

static int GetPageRotation(PDFDoc *doc, int pageNo)
{
    Catalog *catalog = doc->getCatalog();
    Page *page = catalog->getPage(pageNo);
    int rotation = page->getRotate();
    return rotation;
}

static const char * GetLinkActionKindName(LinkActionKind kind) {
    switch (kind) {
        case (actionGoTo):
            return "actionGoTo";
        case actionGoToR:
            return "actionGoToR";
        case actionLaunch:
            return "actionLaunch";
        case actionURI:
            return "actionURI";
        case actionNamed:
            return "actionNamed";
        case actionMovie:
            return "actionMovie";
        case actionUnknown:
            return "actionUnknown";
        default:
            assert(0);
            return "unknown action";
    }
}

static void DumpLinks(DisplayModelSplash *dm, PDFDoc *doc)
{
    Links *     links = NULL;
    int         pagesCount, linkCount;
    int         pageRotation;
    Link *      link;
    LinkURI *   linkUri;
    GBool       upsideDown;

    DBG_OUT("DumpLinks() started\n");
    if (!doc)
        return;

    upsideDown = dm->outputDevice->upsideDown();
    pagesCount = doc->getNumPages();
    for (int pageNo = 1; pageNo < pagesCount; ++pageNo) {
        delete links;
        links = GetLinksForPage(doc, pageNo);
        if (!links)
            goto Exit;
        linkCount = links->getNumLinks();
        if (linkCount > 0)
            DBG_OUT(" :page %d linkCount = %d\n", pageNo, linkCount);
        pageRotation = GetPageRotation(doc, pageNo);
        for (int i=0; i<linkCount; i++) {
            link = links->getLink(i);
            LinkAction *action = link->getAction();
            LinkActionKind actionKind = action->getKind();
            double xs, ys, xe, ye;
            link->getRect(&xs, &ys, &xe, &ye);
            DBG_OUT( "   link %d: pageRotation=%d upsideDown=%d, action=%d (%s), (xs=%d,ys=%d - xe=%d,ye=%d)\n", i,
                pageRotation, (int)upsideDown,
                (int)actionKind,
                GetLinkActionKindName(actionKind),
                (int)xs, (int)ys, (int)xe, (int)ye);
            if (actionURI == actionKind) {
                linkUri = (LinkURI*)action;
                DBG_OUT("   uri=%s\n", linkUri->getURI()->getCString());
            }
        }
    }
Exit:
    delete links;
    DBG_OUT("DumpLinks() finished\n");
}
#endif

static void TransfromUpsideDown(DisplayModelSplash *dm, int pageNo, double *y1, double *y2)
{
    assert(dm);
    if (!dm) return;

    PdfPageInfo *pageInfo = dm->getPageInfo(pageNo);
    double dy = pageInfo->pageDy;
    assert(*y1 <= dy);
    *y1 = dy - *y1;
    assert(*y2 <= dy);
    *y2 = dy - *y2;
}

DisplayModelSplash::DisplayModelSplash(DisplayMode displayMode) :
    DisplayModel(displayMode)
{
    _pdfEngine = new PdfEnginePoppler();
}

TextPage *DisplayModelSplash::GetTextPage(int pageNo)
{
    assert(pdfDoc);
    if (!pdfDoc) return NULL;
    assert(validPageNo(pageNo));
    assert(pagesInfo);
    if (!pagesInfo) return NULL;

    PdfPageInfo * pdfPageInfo = &(pagesInfo[pageNo-1]);
    if (!pdfPageInfo->textPage) {
        TextOutputDev *textOut = new TextOutputDev(NULL, gTrue, gFalse, gFalse);
        if (!textOut)
            return NULL;
        if (!textOut->isOk()) {
            delete textOut;
            return NULL;
        }
        pdfDoc->displayPage(textOut, pageNo, 72, 72, 0, gFalse, gTrue, gFalse);
        pdfPageInfo->textPage = textOut->takeText();
        delete textOut;
    }
    return pdfPageInfo->textPage;
}

void DisplayModelSplash::FreeLinks(void)
{
    for (int pageNo = 1; pageNo <= pageCount(); ++pageNo) {
        delete pagesInfo[pageNo-1].links;
        pagesInfo[pageNo-1].links = NULL;
    }
}

void DisplayModelSplash::FreeTextPages()
{
    for (int pageNo = 1; pageNo <= pageCount(); ++pageNo) {
        delete pagesInfo[pageNo-1].textPage;
        pagesInfo[pageNo-1].textPage = NULL;
    }
}

DisplayModelSplash::~DisplayModelSplash()
{
    RenderQueue_RemoveForDisplayModel(this);
    BitmapCache_FreeForDisplayModel(this);
    cancelRenderingForDisplayModel(this);
    FreeLinks();
    FreeTextPages();

    delete searchState.str;
    delete searchState.strU;
    free((void*)_links);
    free((void*)pagesInfo);
}

/* Map point <x>/<y> on the page <pageNo> to point on the screen. */
void DisplayModelSplash::cvtUserToScreen(int pageNo, double *x, double *y)
{
    double          xTmp = *x;
    double          yTmp = *y;
    double          ctm[6];
    double          dpi;
    int             rotationTmp;

    assert(pdfDoc);
    if (!pdfDoc) return;

    dpi = (double)PDF_FILE_DPI * _zoomReal * 0.01;
    rotationTmp = rotation();
    normalizeRotation(&rotationTmp);
    SplashOutputDev *outputDev = pdfEnginePoppler()->outputDevice();
    pdfDoc->getCatalog()->getPage(pageNo)->getDefaultCTM(ctm, dpi, dpi, rotationTmp, outputDev->upsideDown());

    PdfPageInfo *pageInfo = getPageInfo(pageNo);
    *x = ctm[0] * xTmp + ctm[2] * yTmp + ctm[4] + 0.5 + pageInfo->screenX - pageInfo->bitmapX;
    *y = ctm[1] * xTmp + ctm[3] * yTmp + ctm[5] + 0.5 + pageInfo->screenY - pageInfo->bitmapY;
}

/* Map point <x>/<y> on screen to user coordinates <pageNo>, <x>, <y>. 
 * If it's not possible (i.e. given point is out of any page), 
 * returns CONVERSION_IMPOSSIBLE as a <pageNo> */
void DisplayModelSplash::cvtScreenToUser(int *pageNo, double *x, double *y)
{
    double          xTmp = *x;
    double          yTmp = *y;
    double          ctm[6], invCtm[6];
    double          dpi, invDpi;
    int             rotationTmp, invRotationTmp;

    assert(pdfDoc);
    if (!pdfDoc) return;

    /* find page number of point <x>/<y> */
    *pageNo = getPageNoByPoint (*x, *y);

    if (*pageNo == POINT_OUT_OF_PAGE) return;

    dpi = (double)PDF_FILE_DPI * _zoomReal * 0.01;
    invDpi = (double)PDF_FILE_DPI * 1/(_zoomReal * 0.01);
    rotationTmp = rotation();
    normalizeRotation(&rotationTmp);
    invRotationTmp = rotationTmp == 0 ? 0 : 360 - rotationTmp;
    SplashOutputDev *outputDev = pdfEnginePoppler()->outputDevice();
    pdfDoc->getCatalog()->getPage(*pageNo)->getDefaultCTM(ctm, dpi, dpi, rotationTmp, outputDev->upsideDown());
    pdfDoc->getCatalog()->getPage(*pageNo)->getDefaultCTM(invCtm, invDpi, invDpi, invRotationTmp, outputDev->upsideDown());

    //DBG_OUT("Rotation = %d\n", rotationTmp);

    int sign;
    if (rotationTmp == 0 || rotationTmp == 180)
        sign = 1;
    else //rotationTmp == 90 || rotationTmp == 270
        sign = -1;

    PdfPageInfo *pageInfo = getPageInfo(*pageNo);
    *x = invCtm[0] * sign*(xTmp - ctm[4] - 0.5 - pageInfo->screenX + pageInfo->bitmapX)
        + invCtm[2] * sign*(yTmp - ctm[5] - 0.5 - pageInfo->screenY + pageInfo->bitmapY);
    *y = invCtm[1] * sign*(xTmp - ctm[4] - 0.5 - pageInfo->screenX + pageInfo->bitmapX)
        + invCtm[3] * sign*(yTmp - ctm[5] - 0.5 - pageInfo->screenY + pageInfo->bitmapY);
}

/* Given <region> (in user coordinates ) on page <pageNo>, return a text in that
   region or NULL if no text */
int DisplayModelSplash::getTextInRegion(int pageNo, RectD *region, unsigned short *buf, int buflen)
{
    GooString *         txt = NULL;
    double              xMin, yMin, xMax, yMax;
    GBool               useMediaBox = gFalse;
    GBool               crop = gTrue;
    GBool               doLinks = gFalse;
    PDFRectangle        selection;
    PdfPageInfo *       pageInfo;

    assert(pdfDoc);
    if (!pdfDoc) return NULL;

    double dpi = (double)PDF_FILE_DPI;

    /* TODO: cache textOut? */
    TextOutputDev *textOut = new TextOutputDev(NULL, gTrue, gFalse, gFalse);
    if (!textOut->isOk()) {
        delete textOut;
        return 0;
    }
    /* TODO: make sure we're not doing background threading */
    pageInfo = getPageInfo(pageNo);
    xMin = region->x;
    yMin = pageInfo->pageDy - (region->y + region->dy);//since coordinates are upside-dwon
    xMax = xMin + region->dx;
    yMax = yMin + region->dy;
    pdfDoc->displayPage(textOut, pageNo, dpi, dpi, 0, useMediaBox, crop, doLinks);

    txt = textOut->getText(xMin, yMin, xMax, yMax);

    int length = 0;
    if (txt && (txt->getLength() > 0)) {
        //DBG_OUT("DisplayModelSplash::getTextInRegion() found text '%s' on pageNo=%d, (x=%d, y=%d), (dx=%d, dy=%d)\n",
            //txt->getCString(), pageNo, (int)region->x, (int)region->y, (int)region->dx, (int)region->dy);
        length = min (txt->getLength(), buflen);
        for (int i = 0; i < length; i++) {
            buf[i] = (unsigned short)(unsigned char)txt->getChar(i);
            //DBG_OUT("Char: %c : %d; ushort: %hu\n", txt->getChar(i), (int)(unsigned char)txt->getChar(i), (unsigned short)(unsigned char)txt->getChar(i));
        }
    } else {
        DBG_OUT("DisplayModelSplash::getTextInRegion() didn't find text on pageNo=%d, (x=%d, y=%d), (dx=%d, dy=%d)\n",
            pageNo, (int)region->x, (int)region->y, (int)region->dx, (int)region->dy);
        if (txt) {
            delete txt;
            txt = NULL;
        }
    }
    delete textOut;
    return length;
}

/* Create display info from a 'fileName' etc. 'pdfDoc' will be owned by DisplayInfo
   from now on, so the caller should not delete it itself. Not very good but
   alternative is worse.
   */
DisplayModelSplash *DisplayModelSplash_CreateFromFileName(
  const char *fileName,
  SizeD totalDrawAreaSize,
  int scrollbarXDy, int scrollbarYDx,
  DisplayMode displayMode, int startPage,
  WindowInfo *win)
{
    DisplayModelSplash * dm = new DisplayModelSplash(displayMode);
    if (!dm)
        goto Error;

    if (!dm->load(fileName, startPage, win))
        goto Error;

    dm->setScrollbarsSize(scrollbarXDy, scrollbarYDx);
    dm->setTotalDrawAreaSize(totalDrawAreaSize);

    dm->pdfDoc = dm->pdfEnginePoppler()->pdfDoc();

    DBG_OUT("DisplayModelSplash::CreateFromPdfDoc() pageCount = %d, startPage=%d, displayMode=%d\n",
        dm->pageCount(), (int)dm->startPage(), (int)displayMode);
    return dm;
Error:
    delete dm;
    return NULL;
}

/* Make sure that search hit is visible on the screen */
void DisplayModelSplash::EnsureSearchHitVisible()
{
    int             pageNo;
    int             yStart, yEnd;
    int             xStart, xEnd;
    int             xNewPos, yNewPos;
    BOOL            needScroll = FALSE;

    pageNo = searchHitPageNo;
    yStart = searchHitRectCanvas.y;
    yEnd = yStart + searchHitRectCanvas.dy + 24; /* TODO: 24 to account for the find ui bar */

    xStart = searchHitRectCanvas.x;
    xEnd = xStart + searchHitRectCanvas.dx;

    //DBG_OUT("DisplayModelSplash::EnsureSearchHitVisible(), (yStart=%d, yEnd=%d)\n", yStart, yEnd);

    // TODO: this logic is flawed
    yNewPos = (int)areaOffset.y;

    if (yStart < (int)areaOffset.y) {
        yNewPos -= ((int)areaOffset.y - yStart);
        needScroll = TRUE;
    }
    if (yEnd > (int)(areaOffset.y + drawAreaSize.dy())) {
        yNewPos -= ((int)(areaOffset.y + drawAreaSize.dy()) - yEnd);  
        needScroll = TRUE;
    }

    xNewPos = (int)areaOffset.x;
    if (xStart < (int)areaOffset.x) {
        xNewPos -= ((int)areaOffset.x - xStart);
        needScroll = TRUE;
    }
    if (xEnd > (int)(areaOffset.x + drawAreaSize.dx())) {
        xNewPos -= ((int)(areaOffset.x + drawAreaSize.dx()) - xEnd);  
        needScroll = TRUE;
    }

    PdfPageInfo *pageInfo = getPageInfo(pageNo);
    bool pageRendered = BitmapCache_Exists(this, pageNo, _zoomReal, _rotation);
    if (!pageInfo->visible || needScroll || !pageRendered)
        goToPage(pageNo, yNewPos, xNewPos);
}

/* Recalcualte 'linkCount' and 'links' out of 'pagesInfo' data.
   Should only be called if link data has chagned in 'pagesInfo'. */
void DisplayModelSplash::RecalcLinks(void)
{
    int             pageNo;
    int             i;
    PdfPageInfo *   pageInfo;
    Link *          popplerLink;
    PdfLink *       currPdfLink;
    int             currPdfLinkNo;
    double          xs, ys, xe, ye;

    DBG_OUT("DisplayModelSplash::RecalcLinks()\n");

    free((void*)_links);
    _links = NULL;
    _linkCount = 0;

    /* calculate number of links */
    for (pageNo = 1; pageNo <= pageCount(); ++pageNo) {
        pageInfo = getPageInfo(pageNo);
        if (!pageInfo->links)
            continue;
        _linkCount += pageInfo->links->getNumLinks();
    }

    assert(_linkCount > 0);
    _links = (PdfLink*)malloc(_linkCount * sizeof(PdfLink));
    if (!_links)
        return;

    /* build links info */
    currPdfLinkNo = 0;
    for (pageNo = 1; pageNo <= pageCount(); ++pageNo) {
        pageInfo = getPageInfo(pageNo);
        if (!pageInfo->links)
            continue;
        for (i = 0; i < pageInfo->links->getNumLinks(); i++) {
            currPdfLink = link(currPdfLinkNo);
            popplerLink = pageInfo->links->getLink(i);
            popplerLink->getRect(&xs, &ys, &xe, &ye);
            /* note: different param order than getRect() is intentional */
            RectD_FromXY(&currPdfLink->rectPage, xs, xe, ys, ye);
            assert(currPdfLink->rectPage.dx >= 0);
            assert(currPdfLink->rectPage.dy >= 0);
            currPdfLink->pageNo = pageNo;
            currPdfLink->link = popplerLink;
            ++currPdfLinkNo;
        }
    }
    assert(_linkCount == currPdfLinkNo);
    recalcLinksCanvasPos();
    DBG_OUT("DisplayModelSplash::RecalcLinks() new link count: %d\n", _linkCount);
}

void DisplayModelSplash::GoToDest(LinkDest *linkDest)
{
    Ref             pageRef;
    int             newPage = INVALID_PAGE_NO;
    int             left, top;
    int             scrollY = 0;

    assert(linkDest);
    if (!linkDest) return;

    if (linkDest->isPageRef()) {
        pageRef = linkDest->getPageRef();
        newPage = pdfDoc->findPage(pageRef.num, pageRef.gen);
    } else {
        newPage = linkDest->getPageNum();
    }

    if (newPage <= 0 || newPage > pdfDoc->getNumPages()) {
        newPage = 1;
    }

    left = (int)linkDest->getLeft();
    top = (int)linkDest->getTop();
    /* TODO: convert left/top coordinates to window space */

    /* TODO: this logic needs to be implemented */
    switch (linkDest->getKind()) {
        case destXYZ:
            break;
        case destFitR:
            break;
        default:
            break;
    }
    goToPage( newPage, scrollY);
}

void DisplayModelSplash::GoToNamedDest(UGooString *dest)
{
    LinkDest *d;

    assert(dest);
    if (!dest) return;

    d = pdfDoc->findDest(dest);
    assert(d);
    if (!d) return;
    GoToDest(d);
    delete d;
}

void DisplayModelSplash::handleLinkGoTo(LinkGoTo *linkGoTo)
{
    LinkDest *      linkDest;
    UGooString *    linkNamedDest;

    assert(linkGoTo);
    if (!linkGoTo) return;

    linkDest = linkGoTo->getDest();
    linkNamedDest = linkGoTo->getNamedDest();
    if (linkDest) {
        assert(!linkNamedDest);
        GoToDest(linkDest);
    } else {
        assert(linkNamedDest);
        GoToNamedDest(linkNamedDest);
    }
}

void DisplayModelSplash::handleLinkGoToR(LinkGoToR *linkGoToR)
{
    LinkDest *      linkDest;
    UGooString *    linkNamedDest;
    GooString *     fileName;

    assert(linkGoToR);
    if (!linkGoToR) return;

    linkDest = linkGoToR->getDest();
    linkNamedDest = linkGoToR->getNamedDest();

    fileName = linkGoToR->getFileName();
    /* TODO: see if a file exists, if not show a dialog box. If exists,
       load it and go to a destination in this file.
       Should also search currently opened files. */
    /* Test file: C:\kjk\test_pdfs\pda\palm\dev tools\sdk\user_interface.pdf, page 633 */
}

void DisplayModelSplash::handleLinkURI(LinkURI *linkURI)
{
    const char *uri;

    uri = linkURI->getURI()->getCString();
    if (str_empty(uri))
        return;
    //LaunchBrowser(uri);
		MessageBox(NULL,(TCHAR*)uri, TEXT("Extern link blocked"), 0); /* @note: work-around to solve linking issue */

}

void DisplayModelSplash::handleLinkLaunch(LinkLaunch* linkLaunch)
{
    assert(linkLaunch);
    if (!linkLaunch) return;

    /* Launching means executing another application. It's not supported
       due to security and portability reasons */
}

void DisplayModelSplash::handleLinkNamed(LinkNamed *linkNamed)
{
    GooString * name;
    char *      nameTxt;

    assert(linkNamed);
    if (!linkNamed) return;

    name = linkNamed->getName();
    if (!name)
      return;
    nameTxt = name->getCString();
    if (str_eq(ACTION_NEXT_PAGE, nameTxt)) {
        goToNextPage(0);
    } else if (str_eq(ACTION_PREV_PAGE, nameTxt)) {
        goToPrevPage(0);
    } else if (str_eq(ACTION_LAST_PAGE, nameTxt)) {
        goToLastPage();
    } else if (str_eq(ACTION_FIRST_PAGE, nameTxt)) {
        goToFirstPage();
    } else {
        /* not supporting: "GoBack", "GoForward", "Quit" */
    }
}

/* Return TRUE if can go to previous page (i.e. is not on the first page) */
BOOL DisplayModelSplash::CanGoToPrevPage(void)
{
    if (1 == currentPageNo())
        return FALSE;
    return TRUE;
}

/* Return TRUE if can go to next page (i.e. doesn't already show last page) */
BOOL DisplayModelSplash::CanGoToNextPage(void)
{
    if (displayModeFacing(displayMode())) {
        if (currentPageNo()+1 >= pageCount())
            return FALSE;
    } else {
        if (pageCount() == currentPageNo())
            return FALSE;
    }
    return TRUE;
}

void DisplayModelSplash::FindInit(int startPageNo)
{
    assert(validPageNo(startPageNo));
    searchState.searchState = eSsNone;
    searchState.wrapped = FALSE;
    searchState.startPage = startPageNo;
}

BOOL DisplayModelSplash::FindNextBackward(void)
{
    GBool               startAtTop, stopAtBottom;
    GBool               startAtLast, stopAtLast;
    GBool               caseSensitive, backward;
    GBool               found;
    double              xs, ys, xe, ye;
    UGooString *        strU;
    TextPage *          textPage;
    int                 pageNo;
    RectD               hitRect;

    DBG_OUT("DisplayModelSplash::FindNextBackward()\n");

    // previous search didn't find it, so there's no point looking again
    if (eSsNotFound == searchState.searchState)
        return FALSE;

    // search string was not entered
    if (0 == searchState.str->getLength())
        return FALSE;

    // searching uses the same code as rendering and that code is not
    // thread safe, so we have to cancel all background rendering
    cancelRenderingForDisplayModel(this);

    showBusyCursor();

    backward = gTrue;
    caseSensitive = searchState.caseSensitive;
    strU = searchState.strU;

    if (eSsNone == searchState.searchState) {
        // starting a new search backward
        searchState.currPage = searchState.startPage;
        assert(FALSE == searchState.wrapped);
        startAtLast = gFalse;
        startAtTop = gFalse;
    } else {
        // continuing previous search backward or forward
        startAtLast = gTrue;
        startAtTop = gFalse;
    }
    stopAtBottom = gTrue;
    stopAtLast = gFalse;

    if ((eSsFoundNext == searchState.searchState) 
        || (eSsFoundNextWrapped == searchState.searchState)) {
        searchState.wrapped = FALSE;
    }

    for (;;) {
        pageNo = searchState.currPage;
        textPage = GetTextPage(pageNo);

        DBG_OUT("  search backward for %s, case sensitive=%d, page=%d\n", searchState.str->getCString(), caseSensitive, pageNo);
        found = textPage->findText(strU->unicode(), strU->getLength(), startAtTop, stopAtBottom,
            startAtLast, stopAtLast, caseSensitive, backward,
            &xs, &ys, &xe, &ye);

        if (found) {
            DBG_OUT(" found '%s' on page %d at pos (%.2f, %.2f)-(%.2f,%.2f)\n", 
                searchState.str->getCString(), pageNo, xs, ys, xe, ye);

            TransfromUpsideDown(this, pageNo, &ys, &ye);
            RectD_FromXY(&hitRect, xs, xe, ys, ye);
            if (searchState.wrapped) {
                searchState.wrapped = FALSE;
                searchState.searchState = eSsFoundPrevWrapped;
            } else {
                searchState.searchState = eSsFoundPrev;
            }
            setSearchHit(pageNo, &hitRect);
            EnsureSearchHitVisible();
            goto Exit;
        }

        if (1 == pageNo) {
            DBG_OUT(" wrapped\n");
            searchState.wrapped = TRUE;
            searchState.currPage = pageCount();
        } else
            searchState.currPage = pageNo - 1;

        // moved to another page, so starting from the top
        startAtTop = gTrue;
        startAtLast = gFalse;

        if (searchState.wrapped && 
            (searchState.currPage == searchState.startPage) &&
            (eSsNone == searchState.searchState) ) {
            searchState.searchState = eSsNotFound;
            clearSearchHit();
            goto Exit;
        }
    }
Exit:
    showNormalCursor();
    return found;
}

BOOL DisplayModelSplash::FindNextForward(void)
{
    GBool               startAtTop, stopAtBottom;
    GBool               startAtLast, stopAtLast;
    GBool               caseSensitive, backward;
    GBool               found;
    double              xs, ys, xe, ye;
    UGooString *        strU;
    TextPage *          textPage;
    int                 pageNo;
    RectD               hitRect;

    DBG_OUT("DisplayModelSplash::FindNextForward()\n");

    // previous search didn't find it, so there's no point looking again
    if (eSsNotFound == searchState.searchState)
        return FALSE;

    // search string was not entered
    if (0 == searchState.str->getLength())
        return FALSE;

    // searching uses the same code as rendering and that code is not
    // thread safe, so we have to cancel all background rendering
    cancelRenderingForDisplayModel(this);

    backward = gFalse;
    caseSensitive =searchState.caseSensitive;
    strU = searchState.strU;
    if (eSsNone == searchState.searchState) {
        // starting a new search forward
        DBG_OUT("  new search\n");
        searchState.currPage = searchState.startPage;
        assert(FALSE == searchState.wrapped);
        startAtLast = gFalse;
        startAtTop = gTrue;
    } else {
        // continuing previous search forward or backward
        DBG_OUT("  continue search\n");
        startAtLast = gTrue;
        startAtTop = gFalse;
    }
    stopAtBottom = gTrue;
    stopAtLast = gFalse;

    if ((eSsFoundPrev == searchState.searchState) 
        || (eSsFoundPrevWrapped == searchState.searchState)) {
        searchState.wrapped = FALSE;
    }

    for (;;) {
        pageNo = searchState.currPage;
        textPage = GetTextPage(pageNo);
        DBG_OUT("  search forward for '%s', case sensitive=%d, page=%d\n", searchState.str->getCString(), caseSensitive, pageNo);
        found = textPage->findText(strU->unicode(), strU->getLength(), startAtTop, stopAtBottom,
            startAtLast, stopAtLast, caseSensitive, backward,
            &xs, &ys, &xe, &ye);

        if (found) {
            DBG_OUT(" found '%s' on page %d at pos (%.2f, %.2f)-(%.2f,%.2f)\n", 
                searchState.str->getCString(), pageNo, xs, ys, xe, ye);

            TransfromUpsideDown(this, pageNo, &ys, &ye);
            RectD_FromXY(&hitRect, xs, xe, ys, ye);
            if (searchState.wrapped) {
                searchState.wrapped = FALSE;
                searchState.searchState = eSsFoundNextWrapped;
            } else {
                searchState.searchState = eSsFoundNext;
            }
            setSearchHit(pageNo, &hitRect);
            EnsureSearchHitVisible();
            goto Exit;
        }

        if (pageCount() == pageNo) {
            DBG_OUT(" wrapped\n");
            searchState.wrapped = TRUE;
            searchState.currPage = 1;
        } else
            searchState.currPage = pageNo + 1;

        startAtTop = gTrue;
        startAtLast = gFalse;

        if (searchState.wrapped && 
            (searchState.currPage == searchState.startPage) &&
            (eSsNone == searchState.searchState) ) {
            searchState.searchState = eSsNotFound;
            clearSearchHit();
            goto Exit;
        }
    }
Exit:
    showNormalCursor();
    return found;
}

void DisplayModelSplash::handleLink(PdfLink *pdfLink)
{
    Link *          link;
    LinkAction *    action;
    LinkActionKind  actionKind;

    assert(pdfLink);
    if (!pdfLink) return;

    link = pdfLink->link;
    assert(link);
    if (!link) return;

    action = link->getAction();
    actionKind = action->getKind();

    switch (actionKind) {
        case actionGoTo:
            handleLinkGoTo((LinkGoTo*)action);
            break;
        case actionGoToR:
            handleLinkGoToR((LinkGoToR*)action);
            break;
        case actionLaunch:
            handleLinkLaunch((LinkLaunch*)action);
            break;
        case actionURI:
            handleLinkURI((LinkURI*)action);
            break;
        case actionNamed:
            handleLinkNamed((LinkNamed *)action);
            break;
        default:
            /* other kinds are not supported */
            break;
    }
}


