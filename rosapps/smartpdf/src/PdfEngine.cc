/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
#include "base_util.h"
#include "PdfEngine.h"

#include "ErrorCodes.h"
#include "GooString.h"
#include "GooList.h"
#include "GlobalParams.h"
#include "SplashBitmap.h"
#include "Object.h" /* must be included before SplashOutputDev.h because of sloppiness in SplashOutputDev.h */
#include "SplashOutputDev.h"
#include "TextOutputDev.h"
#include "PDFDoc.h"
#include "SecurityHandler.h"
#include "Link.h"
#include "str_util.h"

// in SumatraPDF.cpp
extern "C" char *GetPasswordForFile(WindowInfo *win, const char *fileName);

const char* const LINK_ACTION_GOTO = "linkActionGoTo";
const char* const LINK_ACTION_GOTOR = "linkActionGoToR";
const char* const LINK_ACTION_LAUNCH = "linkActionLaunch";
const char* const LINK_ACTION_URI = "linkActionUri";
const char* const LINK_ACTION_NAMED = "linkActionNamed";
const char* const LINK_ACTION_MOVIE = "linkActionMovie";
const char* const LINK_ACTION_UNKNOWN = "linkActionUnknown";

static SplashColorMode gSplashColorMode = splashModeBGR8;

static SplashColor splashColRed;
static SplashColor splashColGreen;
static SplashColor splashColBlue;
static SplashColor splashColWhite;
static SplashColor splashColBlack;

#define SPLASH_COL_RED_PTR (SplashColorPtr)&(splashColRed[0])
#define SPLASH_COL_GREEN_PTR (SplashColorPtr)&(splashColGreen[0])
#define SPLASH_COL_BLUE_PTR (SplashColorPtr)&(splashColBlue[0])
#define SPLASH_COL_WHITE_PTR (SplashColorPtr)&(splashColWhite[0])
#define SPLASH_COL_BLACK_PTR (SplashColorPtr)&(splashColBlack[0])

static SplashColorPtr  gBgColor = SPLASH_COL_WHITE_PTR;

static void splashColorSet(SplashColorPtr col, Guchar red, Guchar green, Guchar blue, Guchar alpha)
{
    switch (gSplashColorMode)
    {
        case splashModeBGR8:
            col[0] = blue;
            col[1] = green;
            col[2] = red;
            break;
        case splashModeRGB8:
            col[0] = red;
            col[1] = green;
            col[2] = blue;
            break;
        default:
            assert(0);
            break;
    }
}

void SplashColorsInit(void)
{
    splashColorSet(SPLASH_COL_RED_PTR, 0xff, 0, 0, 0);
    splashColorSet(SPLASH_COL_GREEN_PTR, 0, 0xff, 0, 0);
    splashColorSet(SPLASH_COL_BLUE_PTR, 0, 0, 0xff, 0);
    splashColorSet(SPLASH_COL_BLACK_PTR, 0, 0, 0, 0);
    splashColorSet(SPLASH_COL_WHITE_PTR, 0xff, 0xff, 0xff, 0);
}

static HBITMAP createDIBitmapCommon(RenderedBitmap *bmp, HDC hdc)
{
    int bmpDx = bmp->dx();
    int bmpDy = bmp->dy();
    int bmpRowSize = bmp->rowSize();

    BITMAPINFOHEADER bmih;
    bmih.biSize = sizeof(bmih);
    bmih.biHeight = -bmpDy;
    bmih.biWidth = bmpDx;
    bmih.biPlanes = 1;
    bmih.biBitCount = 24;
    bmih.biCompression = BI_RGB;
    bmih.biSizeImage = bmpDy * bmpRowSize;;
    bmih.biXPelsPerMeter = bmih.biYPelsPerMeter = 0;
    bmih.biClrUsed = bmih.biClrImportant = 0;

    unsigned char* bmpData = bmp->data();
    HBITMAP hbmp = ::CreateDIBitmap(hdc, &bmih, CBM_INIT, bmpData, (BITMAPINFO *)&bmih , DIB_RGB_COLORS);
    return hbmp;
}

static void stretchDIBitsCommon(RenderedBitmap *bmp, HDC hdc, int leftMargin, int topMargin, int pageDx, int pageDy)
{
    int bmpDx = bmp->dx();
    int bmpDy = bmp->dy();
    int bmpRowSize = bmp->rowSize();

    BITMAPINFOHEADER bmih;
    bmih.biSize = sizeof(bmih);
    bmih.biHeight = -bmpDy;
    bmih.biWidth = bmpDx;
    bmih.biPlanes = 1;
    // we could create this dibsection in monochrome
    // if the printer is monochrome, to reduce memory consumption
    // but splash is currently setup to return a full colour bitmap
    bmih.biBitCount = 24;
    bmih.biCompression = BI_RGB;
    bmih.biSizeImage = bmpDy * bmpRowSize;;
    bmih.biXPelsPerMeter = bmih.biYPelsPerMeter = 0;
    bmih.biClrUsed = bmih.biClrImportant = 0;
    SplashColorPtr bmpData = bmp->data();

    ::StretchDIBits(hdc,
        // destination rectangle
        -leftMargin, -topMargin, pageDx, pageDy,
        // source rectangle
        0, 0, bmpDx, bmpDy,
        bmpData,
        (BITMAPINFO *)&bmih ,
        DIB_RGB_COLORS,
        SRCCOPY);
}

RenderedBitmapFitz::RenderedBitmapFitz(fz_pixmap *bitmap)
{
    _bitmap = bitmap;
}

RenderedBitmapFitz::~RenderedBitmapFitz()
{
    if (_bitmap)
        fz_droppixmap(_bitmap);
}

int RenderedBitmapFitz::rowSize()
{
    int rowSize = ((_bitmap->w * 3 + 3) / 4) * 4;
    return rowSize;
}

unsigned char *RenderedBitmapFitz::data()
{
#ifdef FITZ_HEAD
    unsigned char* bmpData = _bitmap->p;
#else
    unsigned char* bmpData = _bitmap->samples;
#endif
    return bmpData;
}

HBITMAP RenderedBitmapFitz::createDIBitmap(HDC hdc)
{
    return createDIBitmapCommon(this, hdc);
}

void RenderedBitmapFitz::stretchDIBits(HDC hdc, int leftMargin, int topMargin, int pageDx, int pageDy)
{
    stretchDIBitsCommon(this, hdc, leftMargin, topMargin, pageDx, pageDy);
}

RenderedBitmapSplash::RenderedBitmapSplash(SplashBitmap *bitmap)
{
    _bitmap = bitmap;
}

RenderedBitmapSplash::~RenderedBitmapSplash() {
    delete _bitmap;
}

int RenderedBitmapSplash::dx()
{ 
    return _bitmap->getWidth();
}

int RenderedBitmapSplash::dy()
{ 
    return _bitmap->getHeight();
}

int RenderedBitmapSplash::rowSize() 
{ 
    return _bitmap->getRowSize(); 
}
    
unsigned char *RenderedBitmapSplash::data()
{
    return _bitmap->getDataPtr();
}

HBITMAP RenderedBitmapSplash::createDIBitmap(HDC hdc)
{
    return createDIBitmapCommon(this, hdc);
}

void RenderedBitmapSplash::stretchDIBits(HDC hdc, int leftMargin, int topMargin, int pageDx, int pageDy)
{
    stretchDIBitsCommon(this, hdc, leftMargin, topMargin, pageDx, pageDy);
}

PdfEnginePoppler::PdfEnginePoppler() : 
    PdfEngine()
   , _pdfDoc(NULL)
   , _outputDev(NULL)
   , _linksForPage(NULL)
{
}

PdfEnginePoppler::~PdfEnginePoppler()
{
    delete _outputDev;
    delete _pdfDoc;
    for (int i = 0; (i < _pageCount) && _linksForPage; i++)
        delete _linksForPage[i];
    free(_linksForPage);
}

bool PdfEnginePoppler::load(const char *fileName, WindowInfo *win)
{
    setFileName(fileName);
    _windowInfo = win;
    /* note: don't delete fileNameStr since PDFDoc takes ownership and deletes them itself */
    GooString *fileNameStr = new GooString(fileName);
    if (!fileNameStr) return false;

    _pdfDoc = new PDFDoc(fileNameStr, NULL, NULL, (void*)win);
    if (!_pdfDoc->isOk()) {
        return false;
    }
    _pageCount = _pdfDoc->getNumPages();
    _linksForPage = (Links**)malloc(_pageCount * sizeof(Links*));
    if (!_linksForPage) return false;
    for (int i=0; i < _pageCount; i++)
        _linksForPage[i] = NULL;
    return true;
}

int PdfEnginePoppler::pageRotation(int pageNo)
{
    assert(validPageNo(pageNo));
    return pdfDoc()->getPageRotate(pageNo);
}

SizeD PdfEnginePoppler::pageSize(int pageNo)
{
    double dx = pdfDoc()->getPageCropWidth(pageNo);
    double dy = pdfDoc()->getPageCropHeight(pageNo);
    return SizeD(dx, dy);
}

SplashOutputDev * PdfEnginePoppler::outputDevice() {
    if (!_outputDev) {
        GBool bitmapTopDown = gTrue;
        _outputDev = new SplashOutputDev(gSplashColorMode, 4, gFalse, gBgColor, bitmapTopDown);
        if (_outputDev)
            _outputDev->startDoc(_pdfDoc->getXRef());
    }
    return _outputDev;
}

RenderedBitmap *PdfEnginePoppler::renderBitmap(
                           int pageNo, double zoomReal, int rotation,
                           BOOL (*abortCheckCbkA)(void *data),
                           void *abortCheckCbkDataA)
{
    assert(outputDevice());
    if (!outputDevice()) return NULL;

    //DBG_OUT("PdfEnginePoppler::RenderBitmap(pageNo=%d) rotate=%d, zoomReal=%.2f%%\n", pageNo, rotation, zoomReal);

    double hDPI = (double)PDF_FILE_DPI * zoomReal * 0.01;
    double vDPI = (double)PDF_FILE_DPI * zoomReal * 0.01;
    GBool  useMediaBox = gFalse;
    GBool  crop        = gTrue;
    GBool  doLinks     = gTrue;
    _pdfDoc->displayPage(_outputDev, pageNo, hDPI, vDPI, rotation, useMediaBox, crop, doLinks,
        abortCheckCbkA, abortCheckCbkDataA);

#if 0
    PdfPageInfo *pageInfo = getPageInfo(pageNo);
    if (!pageInfo->links) {
        /* displayPage calculates links for this page (if doLinks is true)
           and puts inside pdfDoc */
        pageInfo->links = pdfDoc->takeLinks();
        if (pageInfo->links->getNumLinks() > 0)
            RecalcLinks();
    }
#endif
    SplashBitmap* bmp = _outputDev->takeBitmap();
    if (bmp)
        return new RenderedBitmapSplash(bmp);

    return NULL;
}

bool PdfEnginePoppler::printingAllowed()
{
    if (_pdfDoc->okToPrint())
        return true;
    return false;
}

Links* PdfEnginePoppler::getLinksForPage(int pageNo)
{
    if (!_linksForPage)
        return NULL;
    if (_linksForPage[pageNo-1])
        return NULL;

    Object obj;
    Catalog *catalog = _pdfDoc->getCatalog();
    Page *page = catalog->getPage(pageNo);
    Links *links = new Links(page->getAnnots(&obj), catalog->getBaseURI());
    obj.free();
    _linksForPage[pageNo-1] = links;
    return _linksForPage[pageNo-1];
}

int PdfEnginePoppler::linkCount(int pageNo) {
    Links *links = getLinksForPage(pageNo);
    if (!links) return 0;
    return links->getNumLinks();
}

const char* linkActionKindToLinkType(LinkActionKind kind) {
    switch (kind) {
        case (actionGoTo):
            return LINK_ACTION_GOTO;
        case actionGoToR:
            return LINK_ACTION_GOTOR;
        case actionLaunch:
            return LINK_ACTION_LAUNCH;
        case actionURI:
            return LINK_ACTION_URI;
        case actionNamed:
            return LINK_ACTION_NAMED;
        case actionMovie:
            return LINK_ACTION_MOVIE;
        case actionUnknown:
            return LINK_ACTION_UNKNOWN;
        default:
            assert(0);
            return LINK_ACTION_UNKNOWN;
    }
}

const char* PdfEnginePoppler::linkType(int pageNo, int linkNo) {
    Links *links = getLinksForPage(pageNo);
    if (!links) return 0;
    int linkCount = links->getNumLinks();
    assert(linkNo < linkCount);
    Link *link = links->getLink(linkNo-1);
    LinkAction *    action = link->getAction();
    LinkActionKind  actionKind = action->getKind();
    return linkActionKindToLinkType(actionKind);
}

fz_matrix PdfEngineFitz::viewctm (pdf_page *page, float zoom, int rotate)
{
    fz_matrix ctm;
    ctm = fz_identity();
    ctm = fz_concat(ctm, fz_translate(0, -page->mediabox.y1));
    ctm = fz_concat(ctm, fz_scale(zoom, -zoom));
    ctm = fz_concat(ctm, fz_rotate(rotate + page->rotate));
    return ctm;
}

PdfEngineFitz::PdfEngineFitz() : 
        PdfEngine()
        , _popplerEngine(NULL)
        , _xref(NULL)
        , _outline(NULL)
        , _pageTree(NULL)
        , _pages(NULL)
        , _rast(NULL)
{
    _getPageSem = CreateSemaphore(NULL, 1, 1, NULL);
}

PdfEngineFitz::~PdfEngineFitz()
{
    if (_pages) {
        for (int i=0; i < _pageCount; i++) {
            if (_pages[i])
                pdf_droppage(_pages[i]);
        }
        free(_pages);
    }

    if (_pageTree)
        pdf_droppagetree(_pageTree);

    if (_outline)
        pdf_dropoutline(_outline);

    if (_xref) {
        if (_xref->store)
            pdf_dropstore(_xref->store);
        _xref->store = 0;
        pdf_closexref(_xref);
    }

    if (_rast) {
#ifdef FITZ_HEAD
        fz_dropgraphics(_rast);
#else
        fz_droprenderer(_rast);
#endif
    }

    CloseHandle(_getPageSem);

    delete _popplerEngine;        
}

bool PdfEngineFitz::load(const char *fileName, WindowInfo *win)
{
    assert(!_popplerEngine);
    _windowInfo = win;
    setFileName(fileName);
    fz_error *error = pdf_newxref(&_xref);
    if (error)
        goto Error;

    error = pdf_loadxref(_xref, (char*)fileName);
    if (error) {
        if (!strncmp(error->msg, "ioerror", 7))
            goto Error;
        error = pdf_repairxref(_xref, (char*)fileName);
        if (error)
            goto TryPoppler;
    }

    error = pdf_decryptxref(_xref);
    if (error)
        goto Error;

    if (_xref->crypt) {
#ifdef FITZ_HEAD
        int okay = pdf_setpassword(_xref->crypt, "");
        if (!okay)
            goto Error;
        if (!win) {
            // win might not be given if called from pdfbench.cc
            goto Error;
        }
        for (int i=0; i<3; i++) {
            char *pwd = GetPasswordForFile(win, fileName);
            okay = pdf_setpassword(_xref->crypt, pwd);
            free(pwd);
            if (okay)
                goto DecryptedOk;
        }
        goto Error;
#else
        error = pdf_setpassword(_xref->crypt, "");
        if (!error)
            goto DecryptedOk;
        if (!win) {
            // win might not be given if called from pdfbench.cc
            goto Error;
        }
        for (int i=0; i<3; i++) {
            char *pwd = GetPasswordForFile(win, fileName);
            // dialog box was cancelled
            if (!pwd)
                goto Error;
            error = pdf_setpassword(_xref->crypt, pwd);
            free(pwd);
            if (!error)
                goto DecryptedOk;
        }
        goto Error;
#endif
    }

DecryptedOk:
    error = pdf_loadpagetree(&_pageTree, _xref);
    if (error)
        goto Error;

    error = pdf_loadoutline(&_outline, _xref);
    // TODO: can I ignore this error?
    if (error)
        goto Error;

    _pageCount = _pageTree->count;
    _pages = (pdf_page**)malloc(sizeof(pdf_page*) * _pageCount);
    for (int i = 0; i < _pageCount; i++)
        _pages[i] = NULL;
    return true;
Error:
    return false;
TryPoppler:
    _popplerEngine = new PdfEnginePoppler();
    if (!_popplerEngine)
        return false;
    bool fok = _popplerEngine->load(fileName, win);
    if (!fok)
        goto ErrorPoppler;
    return true;

ErrorPoppler:
    delete _popplerEngine;
    return false;
}

pdf_page *PdfEngineFitz::getPdfPage(int pageNo)
{
    if (!_pages)
        return NULL;

    WaitForSingleObject(_getPageSem, INFINITE);
    pdf_page* page = _pages[pageNo-1];
    if (page) {
        if (!ReleaseSemaphore(_getPageSem, 1, NULL))
            DBG_OUT("Fitz: ReleaseSemaphore error!\n");
        return page;
    }
    // TODO: should check for error from pdf_getpageobject?
    fz_obj * obj = pdf_getpageobject(_pageTree, pageNo - 1);
    fz_error * error = pdf_loadpage(&page, _xref, obj);
    assert (!error);
    if (error) {
        if (!ReleaseSemaphore(_getPageSem, 1, NULL))
            DBG_OUT("Fitz: ReleaseSemaphore error!\n");
        fz_droperror(error);
        return NULL;
    }
    _pages[pageNo-1] = page;
    if (!ReleaseSemaphore(_getPageSem, 1, NULL))
        DBG_OUT("Fitz: ReleaseSemaphore error!\n");
    return page;
}

void PdfEngineFitz::dropPdfPage(int pageNo)
{
    assert(_pages);
    if (!_pages) return;
    pdf_page* page = _pages[pageNo-1];
    assert(page);
    if (!page) return;
    pdf_droppage(page);
    _pages[pageNo-1] = NULL;
}

int PdfEngineFitz::pageRotation(int pageNo)
{
    if (_popplerEngine)
        return _popplerEngine->pageRotation(pageNo);

    assert(validPageNo(pageNo));
    fz_obj *dict = pdf_getpageobject(pages(), pageNo - 1);
    int rotation;
    fz_error *error = pdf_getpageinfo(_xref, dict, NULL, &rotation);
    if (error)
        return INVALID_ROTATION;
    return rotation;
}

SizeD PdfEngineFitz::pageSize(int pageNo)
{
    if (_popplerEngine)
        return _popplerEngine->pageSize(pageNo);

    assert(validPageNo(pageNo));
    fz_obj *dict = pdf_getpageobject(pages(), pageNo - 1);
    fz_rect bbox;
    fz_error *error = pdf_getpageinfo(_xref, dict, &bbox, NULL);
    if (error)
        return SizeD(0,0);
    return SizeD(fabs(bbox.x1 - bbox.x0), fabs(bbox.y1 - bbox.y0));
}

bool PdfEngineFitz::printingAllowed()
{
    assert(_xref);
    int permissionFlags = PDF_DEFAULT_PERM_FLAGS;
    if (_xref && _xref->crypt)
        permissionFlags = _xref->crypt->p;
    if (permissionFlags & PDF_PERM_PRINT)
        return true;
    return false;
}

static void ConvertPixmapForWindows(fz_pixmap *image)
{
    int bmpstride = ((image->w * 3 + 3) / 4) * 4;
    unsigned char *bmpdata = (unsigned char*)fz_malloc(image->h * bmpstride);
    if (!bmpdata)
        return;

    for (int y = 0; y < image->h; y++)
    {
        unsigned char *p = bmpdata + y * bmpstride;
#ifdef FITZ_HEAD
        unsigned char *s = image->p + y * image->w * 4;
#else
        unsigned char *s = image->samples + y * image->w * 4;
#endif
        for (int x = 0; x < image->w; x++)
        {
            p[x * 3 + 0] = s[x * 4 + 3];
            p[x * 3 + 1] = s[x * 4 + 2];
            p[x * 3 + 2] = s[x * 4 + 1];
        }
    }
#ifdef FITZ_HEAD
    fz_free(image->p);
    image->p = bmpdata;
#else
    fz_free(image->samples);
    image->samples = bmpdata;
#endif
}

RenderedBitmap *PdfEngineFitz::renderBitmap(
                           int pageNo, double zoomReal, int rotation,
                           BOOL (*abortCheckCbkA)(void *data),
                           void *abortCheckCbkDataA)
{
    fz_error* error;
    fz_matrix ctm;
    fz_rect bbox;

    if (_popplerEngine)
        return _popplerEngine->renderBitmap(pageNo, zoomReal, rotation, abortCheckCbkA, abortCheckCbkDataA);

    if (!_rast) {
#ifdef FITZ_HEAD
        error = fz_newgraphics(&_rast, 1024 * 512);
#else
        error = fz_newrenderer(&_rast, pdf_devicergb, 0, 1024 * 512);
#endif
    }

    fz_pixmap* image = NULL;
    pdf_page* page = getPdfPage(pageNo);
    if (!page)
        goto TryPoppler;
    zoomReal = zoomReal / 100.0;
    ctm = viewctm(page, zoomReal, rotation);
    bbox = fz_transformaabb(ctm, page->mediabox);
#ifdef FITZ_HEAD
    error = fz_drawtree(&image, _rast, page->tree, ctm, pdf_devicergb, fz_roundrect(bbox), 1);
#else
    error = fz_rendertree(&image, _rast, page->tree, ctm, fz_roundrect(bbox), 1);
#endif
#if CONSERVE_MEMORY
    dropPdfPage(pageNo);
#endif
    if (error)
        goto TryPoppler;
    ConvertPixmapForWindows(image);
    return new RenderedBitmapFitz(image);
TryPoppler:
    _popplerEngine = new PdfEnginePoppler();
    if (!_popplerEngine)
        return false;
    bool fok = _popplerEngine->load(fileName(), _windowInfo);
    if (!fok)
        goto ErrorPoppler;
    return _popplerEngine->renderBitmap(pageNo, zoomReal, rotation, abortCheckCbkA, abortCheckCbkDataA);
ErrorPoppler:
    return NULL;
}

static int getLinkCount(pdf_link *currLink) {
    if (!currLink)
        return 0;
    int count = 1;
    while (currLink->next) {
        ++count;
        currLink = currLink->next;
    }
    return count;
}

int PdfEngineFitz::linkCount(int pageNo) {
    pdf_page* page = getPdfPage(pageNo);
    if (!page)
        return 0;
    return getLinkCount(page->links);
}

static const char *linkToLinkType(pdf_link *link) {
    switch (link->kind) {
        case PDF_LGOTO:
            return LINK_ACTION_GOTO;
        case PDF_LURI:
            return LINK_ACTION_URI;
        case PDF_LUNKNOWN: /* @note: add unhandled value */
            OutputDebugString(L"Link unknown");
            break;
    }
    return LINK_ACTION_UNKNOWN;
}

const char* PdfEngineFitz::linkType(int pageNo, int linkNo) {
    pdf_page* page = getPdfPage(pageNo);
    pdf_link* currLink = page->links;
    for (int i = 0; i < linkNo; i++) {
        assert(currLink);
        if (!currLink)
            return NULL;
        currLink = currLink->next;
    }
    return linkToLinkType(currLink);
}

