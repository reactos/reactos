/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
#include "DisplayModel.h"

#include "str_util.h"

// TODO: get rid of the need for GooString and UGooString in common code
#include "GooString.h"
#include "UGooString.h"
// TODO: get rid of the need for GooMutex.h ?
#include "GooMutex.h"

#include <assert.h>
#include <stdlib.h>

#ifdef _WIN32
#define PREDICTIVE_RENDER 1
#endif

#define MAX_BITMAPS_CACHED 256
static GooMutex             cacheMutex;
static BitmapCacheEntry *   gBitmapCache[MAX_BITMAPS_CACHED] = {0};
static int                  gBitmapCacheCount = 0;

static MutexAutoInitDestroy gAutoCacheMutex(&cacheMutex);

DisplaySettings gDisplaySettings = {
  PADDING_PAGE_BORDER_TOP_DEF,
  PADDING_PAGE_BORDER_BOTTOM_DEF,
  PADDING_PAGE_BORDER_LEFT_DEF,
  PADDING_PAGE_BORDER_RIGHT_DEF,
  PADDING_BETWEEN_PAGES_X_DEF,
  PADDING_BETWEEN_PAGES_Y_DEF
};

bool validZoomReal(double zoomReal)
{
    if ((zoomReal < ZOOM_MIN) || (zoomReal > ZOOM_MAX)) {
        DBG_OUT("validZoomReal() invalid zoom: %.4f\n", zoomReal);
        return false;
    }
    return true;
}

bool displayModeFacing(DisplayMode displayMode)
{
    if ((DM_SINGLE_PAGE == displayMode) || (DM_CONTINUOUS == displayMode))
        return false;
    else if ((DM_FACING == displayMode) || (DM_CONTINUOUS_FACING == displayMode))
        return true;
    assert(0);
    return false;
}

bool displayModeContinuous(DisplayMode displayMode)
{
    if ((DM_SINGLE_PAGE == displayMode) || (DM_FACING == displayMode))
        return false;
    else if ((DM_CONTINUOUS == displayMode) || (DM_CONTINUOUS_FACING == displayMode))
        return true;
    assert(0);
    return false;
}

int columnsFromDisplayMode(DisplayMode displayMode)
{
    if (DM_SINGLE_PAGE == displayMode) {
        return 1;
    } else if (DM_FACING == displayMode) {
        return 2;
    } else if (DM_CONTINUOUS == displayMode) {
        return 1;
    } else if (DM_CONTINUOUS_FACING == displayMode) {
        return 2;
    } else
        assert(0);
    return 1;
}

DisplaySettings *globalDisplaySettings(void)
{
    return &gDisplaySettings;
}

bool rotationFlipped(int rotation)
{
    assert(validRotation(rotation));
    normalizeRotation(&rotation);
    if ((90 == rotation) || (270 == rotation))
        return true;
    return false;
}

bool displayStateFromDisplayModel(DisplayState *ds, DisplayModel *dm)
{
    ds->filePath = str_escape(dm->fileName());
    if (!ds->filePath)
        return FALSE;
    ds->displayMode = dm->displayMode();
    ds->fullScreen = dm->fullScreen();
    ds->pageNo = dm->currentPageNo();
    ds->rotation = dm->rotation();
    ds->zoomVirtual = dm->zoomVirtual();
    ds->scrollX = (int)dm->areaOffset.x;
    if (displayModeContinuous(dm->displayMode())) {
        /* TODO: should be offset of top page */
        ds->scrollY = 0;
    } else {
        ds->scrollY = (int)dm->areaOffset.y;
    }
    ds->windowDx = dm->drawAreaSize.dxI();
    ds->windowDy = dm->drawAreaSize.dyI();
    ds->windowX = 0;
    ds->windowY = 0;
    return TRUE;
}

/* Given 'pageInfo', which should contain correct information about
   pageDx, pageDy and rotation, return a page size after applying a global
   rotation */
void pageSizeAfterRotation(PdfPageInfo *pageInfo, int rotation,
    double *pageDxOut, double *pageDyOut)
{
    assert(pageInfo && pageDxOut && pageDyOut);
    if (!pageInfo || !pageDxOut || !pageDyOut)
        return;

    *pageDxOut = pageInfo->pageDx;
    *pageDyOut = pageInfo->pageDy;

    rotation = rotation + pageInfo->rotation;
    normalizeRotation(&rotation);
    if (rotationFlipped(rotation))
        swap_double(pageDxOut, pageDyOut);
}

DisplayModel::DisplayModel(DisplayMode displayMode)
{
    _displayMode = displayMode;
    _rotation = INVALID_ROTATION;
    _zoomVirtual = INVALID_ZOOM;
    _fullScreen = false;
    _startPage = INVALID_PAGE_NO;
    _appData = NULL;
    _pdfEngine = NULL;
    pagesInfo = NULL;

    _linkCount = 0;
    _links = NULL;

    searchHitPageNo = INVALID_PAGE_NO;
    searchState.searchState = eSsNone;
    searchState.str = new GooString();
    searchState.strU = new UGooString();
}

DisplayModel::~DisplayModel()
{
    delete _pdfEngine;
}

PdfPageInfo *DisplayModel::getPageInfo(int pageNo) const
{
    assert(validPageNo(pageNo));
    assert(pagesInfo);
    if (!pagesInfo) return NULL;
    return &(pagesInfo[pageNo-1]);
}

bool DisplayModel::load(const char *fileName, int startPage, WindowInfo *win) 
{ 
    assert(fileName);
    if (!_pdfEngine->load(fileName, win))
        return false;

    if (validPageNo(startPage))
        _startPage = startPage;
    else
        _startPage = 1;

    if (!buildPagesInfo())
        return false;
    return true;
}

bool DisplayModel::buildPagesInfo(void)
{
    assert(!pagesInfo);
    int _pageCount = pageCount();

    pagesInfo = (PdfPageInfo*)calloc(1, _pageCount * sizeof(PdfPageInfo));
    if (!pagesInfo)
        return false;

    for (int pageNo = 1; pageNo <= _pageCount; pageNo++) {
        PdfPageInfo *pageInfo = getPageInfo(pageNo);
        SizeD pageSize = pdfEngine()->pageSize(pageNo);
        pageInfo->pageDx = pageSize.dx();
        pageInfo->pageDy = pageSize.dy();
        pageInfo->rotation = pdfEngine()->pageRotation(pageNo);

        pageInfo->links = NULL;
        pageInfo->textPage = NULL;

        pageInfo->visible = false;
        pageInfo->shown = false;
        if (displayModeContinuous(_displayMode)) {
            pageInfo->shown = true;
        } else {
            if ((pageNo >= _startPage) && (pageNo < _startPage + columnsFromDisplayMode(_displayMode))) {
                DBG_OUT("DisplayModelSplash::CreateFromPdfDoc() set page %d as shown\n", pageNo);
                pageInfo->shown = true;
            }
        }
    }
    return true;
}

bool DisplayModel::pageShown(int pageNo)
{
    PdfPageInfo *pageInfo = getPageInfo(pageNo);
    if (!pageInfo)
        return false;
    return pageInfo->shown;
}

bool DisplayModel::pageVisible(int pageNo)
{
    PdfPageInfo *pageInfo = getPageInfo(pageNo);
    if (!pageInfo)
        return false;
    return pageInfo->visible;
}

/* Return true if a page is visible or a page below or above is visible */
bool DisplayModel::pageVisibleNearby(int pageNo)
{
    /* TODO: should it check 2 pages above and below in facing mode? */
    if (pageVisible(pageNo))
        return true;
    if (validPageNo(pageNo-1) && pageVisible(pageNo-1))
        return true;
    if (validPageNo(pageNo+1) && pageVisible(pageNo+1))
        return true;
    return false;
}

/* Given a zoom level that can include a "virtual" zoom levels like ZOOM_FIT_WIDTH
   and ZOOM_FIT_PAGE, calculate an absolute zoom level */
double DisplayModel::zoomRealFromFirtualForPage(double zoomVirtual, int pageNo)
{
    double          _zoomReal, zoomX, zoomY, pageDx, pageDy;
    double          areaForPageDx, areaForPageDy;
    int             areaForPageDxInt;
    int             columns;

    assert(0 != drawAreaSize.dxI());
    assert(0 != drawAreaSize.dy());

    pageSizeAfterRotation(getPageInfo(pageNo), rotation(), &pageDx, &pageDy);

    assert(0 != (int)pageDx);
    assert(0 != (int)pageDy);

    columns = columnsFromDisplayMode(displayMode());
    areaForPageDx = (drawAreaSize.dx() - PADDING_PAGE_BORDER_LEFT - PADDING_PAGE_BORDER_RIGHT);
    areaForPageDx -= (PADDING_BETWEEN_PAGES_X * (columns - 1));
    areaForPageDxInt = (int)(areaForPageDx / columns);
    areaForPageDx = (double)areaForPageDxInt;
    areaForPageDy = drawAreaSize.dy() - PADDING_PAGE_BORDER_TOP - PADDING_PAGE_BORDER_BOTTOM;
    if (ZOOM_FIT_WIDTH == zoomVirtual) {
        /* TODO: should use gWinDx if we don't show scrollbarY */
        _zoomReal = (areaForPageDx * 100.0) / (double)pageDx;
    } else if (ZOOM_FIT_PAGE == zoomVirtual) {
        zoomX = (areaForPageDx * 100.0) / (double)pageDx;
        zoomY = (areaForPageDy * 100.0) / (double)pageDy;
        if (zoomX < zoomY)
            _zoomReal = zoomX;
        else
            _zoomReal= zoomY;
    } else
        _zoomReal = zoomVirtual;

    return _zoomReal;
}

int DisplayModel::firstVisiblePageNo(void) const
{
    assert(pagesInfo);
    if (!pagesInfo) return INVALID_PAGE_NO;

    for (int pageNo = 1; pageNo <= pageCount(); ++pageNo) {
        PdfPageInfo *pageInfo = getPageInfo(pageNo);
        if (pageInfo->visible)
            return pageNo;
    }
    assert(0);
    return INVALID_PAGE_NO;
}

int DisplayModel::currentPageNo(void) const
{
    if (displayModeContinuous(displayMode()))
        return firstVisiblePageNo();
    else
        return _startPage;
}

void DisplayModel::setZoomVirtual(double zoomVirtual)
{
    int     pageNo;
    double  minZoom = INVALID_BIG_ZOOM;
    double  thisPageZoom;

    assert(ValidZoomVirtual(zoomVirtual));
    _zoomVirtual = zoomVirtual;

    if ((ZOOM_FIT_WIDTH == zoomVirtual) || (ZOOM_FIT_PAGE == zoomVirtual)) {
        /* we want the same zoom for all pages, so use the smallest zoom
           across the pages so that the largest page fits. In most PDFs all
           pages are the same size anyway */
        for (pageNo = 1; pageNo <= pageCount(); pageNo++) {
            if (pageShown(pageNo)) {
                thisPageZoom = zoomRealFromFirtualForPage(this->zoomVirtual(), pageNo);
                assert(0 != thisPageZoom);
                if (minZoom > thisPageZoom)
                    minZoom = thisPageZoom;
            }
        }
        assert(minZoom != INVALID_BIG_ZOOM);
        this->_zoomReal = minZoom;
    } else
        this->_zoomReal = zoomVirtual;
}

/* Given pdf info and zoom/rotation, calculate the position of each page on a
   large sheet that is continous view. Needs to be recalculated when:
     * zoom changes
     * rotation changes
     * switching between display modes
     * navigating to another page in non-continuous mode */
void DisplayModel::relayout(double zoomVirtual, int rotation)
{
    int         pageNo;
    PdfPageInfo*pageInfo = NULL;
    double      currPosX;
    double      pageDx=0, pageDy=0;
    int         currDxInt, currDyInt;
    double      totalAreaDx, totalAreaDy;
    double      areaPerPageDx;
    int         areaPerPageDxInt;
    double      thisRowDx;
    double      rowMaxPageDy;
    double      offX, offY;
    double      pageOffX;
    int         columnsLeft;
    int         pageInARow;
    int         columns;
    double      newAreaOffsetX;

    assert(pagesInfo);
    if (!pagesInfo)
        return;

    normalizeRotation(&rotation);
    assert(validRotation(rotation));

    _rotation = rotation;

    double currPosY = PADDING_PAGE_BORDER_TOP;
    double currZoomReal = _zoomReal;
    setZoomVirtual(zoomVirtual);

//    DBG_OUT("DisplayModel::relayout(), pageCount=%d, zoomReal=%.6f, zoomVirtual=%.2f\n", pageCount, dm->zoomReal, dm->zoomVirtual);
    totalAreaDx = 0;

    if (0 == currZoomReal)
        newAreaOffsetX = 0.0;
    else
        newAreaOffsetX = areaOffset.x * _zoomReal / currZoomReal;
    areaOffset.x = newAreaOffsetX;
    /* calculate the position of each page on the canvas, given current zoom,
       rotation, columns parameters. You can think of it as a simple
       table layout i.e. rows with a fixed number of columns. */
    columns = columnsFromDisplayMode(displayMode());
    columnsLeft = columns;
    currPosX = PADDING_PAGE_BORDER_LEFT;
    rowMaxPageDy = 0;
    for (pageNo = 1; pageNo <= pageCount(); ++pageNo) {
        pageInfo = getPageInfo(pageNo);
        if (!pageInfo->shown) {
            assert(!pageInfo->visible);
            continue;
        }
        pageSizeAfterRotation(pageInfo, rotation, &pageDx, &pageDy);
        currDxInt = (int)(pageDx * _zoomReal * 0.01 + 0.5);
        currDyInt = (int)(pageDy * _zoomReal * 0.01 + 0.5);
        pageInfo->currDx = (double)currDxInt;
        pageInfo->currDy = (double)currDyInt;

        if (rowMaxPageDy < pageInfo->currDy)
            rowMaxPageDy = pageInfo->currDy;

        pageInfo->currPosX = currPosX;
        pageInfo->currPosY = currPosY;
        /* set position of the next page to be after this page with padding.
           Note: for the last page we don't want padding so we'll have to
           substract it when we create new page */
        currPosX += (pageInfo->currDx + PADDING_BETWEEN_PAGES_X);

        --columnsLeft;
        assert(columnsLeft >= 0);
        if (0 == columnsLeft) {
            /* starting next row */
            currPosY += rowMaxPageDy + PADDING_BETWEEN_PAGES_Y;
            rowMaxPageDy = 0;
            thisRowDx = currPosX - PADDING_BETWEEN_PAGES_X + PADDING_PAGE_BORDER_RIGHT;
            if (totalAreaDx < thisRowDx)
                totalAreaDx = thisRowDx;
            columnsLeft = columns;
            currPosX = PADDING_PAGE_BORDER_LEFT;
        }
/*        DBG_OUT("  page = %3d, (x=%3d, y=%5d, dx=%4d, dy=%4d) orig=(dx=%d,dy=%d)\n",
            pageNo, (int)pageInfo->currPosX, (int)pageInfo->currPosY,
                    (int)pageInfo->currDx, (int)pageInfo->currDy,
                    (int)pageDx, (int)pageDy); */
    }

    if (columnsLeft < columns) {
        /* this is a partial row */
        currPosY += rowMaxPageDy + PADDING_BETWEEN_PAGES_Y;
        thisRowDx = currPosX + (pageInfo->currDx + PADDING_BETWEEN_PAGES_X) - PADDING_BETWEEN_PAGES_X + PADDING_PAGE_BORDER_RIGHT;
        if (totalAreaDx < thisRowDx)
            totalAreaDx = thisRowDx;
    }

    /* since pages can be smaller than the drawing area, center them in x axis */
    if (totalAreaDx < drawAreaSize.dx()) {
        areaOffset.x = 0.0;
        offX = (drawAreaSize.dx() - totalAreaDx) / 2.0 + PADDING_PAGE_BORDER_LEFT;
        assert(offX >= 0.0);
        areaPerPageDx = totalAreaDx - PADDING_PAGE_BORDER_LEFT - PADDING_PAGE_BORDER_RIGHT;
        areaPerPageDx = areaPerPageDx - (PADDING_BETWEEN_PAGES_X * (columns - 1));
        areaPerPageDxInt = (int)(areaPerPageDx / (double)columns);
        areaPerPageDx = (double)areaPerPageDxInt;
        totalAreaDx = drawAreaSize.dx();
        pageInARow = 0;
        for (pageNo = 1; pageNo <= pageCount(); ++pageNo) {
            pageInfo = getPageInfo(pageNo);
            if (!pageInfo->shown) {
                assert(!pageInfo->visible);
                continue;
            }
            pageOffX = (pageInARow * (PADDING_BETWEEN_PAGES_X + areaPerPageDx));
            pageOffX += (areaPerPageDx - pageInfo->currDx) / 2;
            assert(pageOffX >= 0.0);
            pageInfo->currPosX = pageOffX + offX;
            ++pageInARow;
            if (pageInARow == columns)
                pageInARow = 0;
        }
    }

    /* if after resizing we would have blank space on the right due to x offset
       being too much, make x offset smaller so that there's no blank space */
    if (drawAreaSize.dx() - (totalAreaDx - newAreaOffsetX) > 0) {
        newAreaOffsetX = totalAreaDx - drawAreaSize.dx();
        areaOffset.x = newAreaOffsetX;
    }

    /* if a page is smaller than drawing area in y axis, y-center the page */
    totalAreaDy = currPosY + PADDING_PAGE_BORDER_BOTTOM - PADDING_BETWEEN_PAGES_Y;
    if (totalAreaDy < drawAreaSize.dy()) {
        offY = PADDING_PAGE_BORDER_TOP + (drawAreaSize.dy() - totalAreaDy) / 2;
        DBG_OUT("  offY = %.2f\n", offY);
        assert(offY >= 0.0);
        totalAreaDy = drawAreaSize.dy();
        for (pageNo = 1; pageNo <= pageCount(); ++pageNo) {
            pageInfo = getPageInfo(pageNo);
            if (!pageInfo->shown) {
                assert(!pageInfo->visible);
                continue;
            }
            pageInfo->currPosY += offY;
            DBG_OUT("  page = %3d, (x=%3d, y=%5d, dx=%4d, dy=%4d) orig=(dx=%d,dy=%d)\n",
                pageNo, (int)pageInfo->currPosX, (int)pageInfo->currPosY,
                        (int)pageInfo->currDx, (int)pageInfo->currDy,
                        (int)pageDx, (int)pageDy);
        }
    }

    _canvasSize = SizeD(totalAreaDx, totalAreaDy);
}

void DisplayModel::changeStartPage(int startPage)
{
    assert(validPageNo(startPage));
    assert(!displayModeContinuous(displayMode()));

    int columns = columnsFromDisplayMode(displayMode());
    _startPage = startPage;
    for (int pageNo = 1; pageNo <= pageCount(); pageNo++) {
        PdfPageInfo *pageInfo = getPageInfo(pageNo);
        if (displayModeContinuous(displayMode()))
            pageInfo->shown = true;
        else
            pageInfo->shown = false;
        if ((pageNo >= startPage) && (pageNo < startPage + columns)) {
            //DBG_OUT("DisplayModel::changeStartPage() set page %d as shown\n", pageNo);
            pageInfo->shown = true;
        }
        pageInfo->visible = false;
    }
    relayout(zoomVirtual(), rotation());
}

/* Given positions of each page in a large sheet that is continous view and
   coordinates of a current view into that large sheet, calculate which
   parts of each page is visible on the screen.
   Needs to be recalucated after scrolling the view. */
void DisplayModel::recalcVisibleParts(void)
{
    int             pageNo;
    RectI           drawAreaRect;
    RectI           pageRect;
    RectI           intersect;
    PdfPageInfo*    pageInfo;
    int             visibleCount;

    assert(pagesInfo);
    if (!pagesInfo)
        return;

    drawAreaRect.x = (int)areaOffset.x;
    drawAreaRect.y = (int)areaOffset.y;
    drawAreaRect.dx = drawAreaSize.dxI();
    drawAreaRect.dy = drawAreaSize.dyI();

//    DBG_OUT("DisplayModel::recalcVisibleParts() draw area         (x=%3d,y=%3d,dx=%4d,dy=%4d)\n",
//        drawAreaRect.x, drawAreaRect.y, drawAreaRect.dx, drawAreaRect.dy);
    visibleCount = 0;
    for (pageNo = 1; pageNo <= pageCount(); ++pageNo) {
        pageInfo = getPageInfo(pageNo);
        if (!pageInfo->shown) {
            assert(!pageInfo->visible);
            continue;
        }
        pageRect.x = (int)pageInfo->currPosX;
        pageRect.y = (int)pageInfo->currPosY;
        pageRect.dx = (int)pageInfo->currDx;
        pageRect.dy = (int)pageInfo->currDy;
        pageInfo->visible = false;
        if (RectI_Intersect(&pageRect, &drawAreaRect, &intersect)) {
            pageInfo->visible = true;
            visibleCount += 1;
            pageInfo->bitmapX = (int) ((double)intersect.x - pageInfo->currPosX);
            assert(pageInfo->bitmapX >= 0);
            pageInfo->bitmapY = (int) ((double)intersect.y - pageInfo->currPosY);
            assert(pageInfo->bitmapY >= 0);
            pageInfo->bitmapDx = intersect.dx;
            pageInfo->bitmapDy = intersect.dy;
            pageInfo->screenX = (int) ((double)intersect.x - areaOffset.x);
            assert(pageInfo->screenX >= 0);
            assert(pageInfo->screenX <= drawAreaSize.dx());
            pageInfo->screenY = (int) ((double)intersect.y - areaOffset.y);
            assert(pageInfo->screenX >= 0);
            assert(pageInfo->screenY <= drawAreaSize.dy());
/*            DBG_OUT("                                  visible page = %d, (x=%3d,y=%3d,dx=%4d,dy=%4d) at (x=%d,y=%d)\n",
                pageNo, pageInfo->bitmapX, pageInfo->bitmapY,
                          pageInfo->bitmapDx, pageInfo->bitmapDy,
                          pageInfo->screenX, pageInfo->screenY); */
        }
    }

    assert(visibleCount > 0);
}

/* Map rectangle <r> on the page <pageNo> to point on the screen. */
void DisplayModel::rectCvtUserToScreen(int pageNo, RectD *r)
{
    double          sx, sy, ex, ey;

    sx = r->x;
    sy = r->y;
    ex = r->x + r->dx;
    ey = r->y + r->dy;

    cvtUserToScreen(pageNo, &sx, &sy);
    cvtUserToScreen(pageNo, &ex, &ey);
    RectD_FromXY(r, sx, ex, sy, ey);
}

/* Map rectangle <r> on the page <pageNo> to point on the screen. */
void DisplayModel::rectCvtScreenToUser(int *pageNo, RectD *r)
{
    double          sx, sy, ex, ey;

    sx = r->x;
    sy = r->y;
    ex = r->x + r->dx;
    ey = r->y + r->dy;

    cvtScreenToUser(pageNo, &sx, &sy);
    cvtScreenToUser(pageNo, &ex, &ey);
    RectD_FromXY(r, sx, ex, sy, ey);
}

int DisplayModel::getPageNoByPoint (double x, double y) 
{
    for (int pageNo = 1; pageNo <= pageCount(); ++pageNo) {
        PdfPageInfo *pageInfo = getPageInfo(pageNo);
        if (!pageInfo->visible)
            continue;
        assert(pageInfo->shown);
        if (!pageInfo->shown)
            continue;

        RectI pageOnScreen;
        pageOnScreen.x = pageInfo->screenX;
        pageOnScreen.y = pageInfo->screenY;
        pageOnScreen.dx = pageInfo->bitmapDx;
        pageOnScreen.dy = pageInfo->bitmapDy;

        if (RectI_Inside (&pageOnScreen, (int)x, (int)y)) /* @note: int casts */
            return pageNo;
    }
    return POINT_OUT_OF_PAGE;
}

void DisplayModel::recalcSearchHitCanvasPos(void)
{
    int             pageNo;
    RectD           rect;

    pageNo = searchHitPageNo;
    if (INVALID_PAGE_NO == pageNo) return;
    rect = searchHitRectPage;
    rectCvtUserToScreen(pageNo, &rect);
    searchHitRectCanvas.x = (int)rect.x;
    searchHitRectCanvas.y = (int)rect.y;
    searchHitRectCanvas.dx = (int)rect.dx;
    searchHitRectCanvas.dy = (int)rect.dy;
}

/* Recalculates the position of each link on the canvas i.e. applies current
   rotation and zoom level and offsets it by the offset of each page in
   the canvas.
   TODO: applying rotation and zoom level could be split into a separate
         function for speedup, since it only has to change after rotation/zoomLevel
         changes while this function has to be called after each scrolling.
         But I'm not sure if this would be a significant speedup */
void DisplayModel::recalcLinksCanvasPos(void)
{
    PdfLink *       pdfLink;
    PdfPageInfo *   pageInfo;
    int             linkNo;
    RectD           rect;

    // TODO: calling it here is a bit of a hack
    recalcSearchHitCanvasPos();

    DBG_OUT("DisplayModel::recalcLinksCanvasPos() linkCount=%d\n", _linkCount);

    if (0 == _linkCount)
        return;
    assert(_links);
    if (!_links)
        return;

    for (linkNo = 0; linkNo < _linkCount; linkNo++) {
        pdfLink = link(linkNo);
        pageInfo = getPageInfo(pdfLink->pageNo);
        if (!pageInfo->visible) {
            /* hack: make the links on pages that are not shown invisible by
                     moving it off canvas. A better solution would probably be
                     not adding those links in the first place */
            pdfLink->rectCanvas.x = -100;
            pdfLink->rectCanvas.y = -100;
            pdfLink->rectCanvas.dx = 0;
            pdfLink->rectCanvas.dy = 0;
            continue;
        }

        rect = pdfLink->rectPage;
        rectCvtUserToScreen(pdfLink->pageNo, &rect);

#if 0 // this version is correct but needs to be made generic, not specific to poppler
        /* hack: in PDFs that have a crop-box (like treo700psprint_UG.pdf)
           we need to shift links by the offset of crop-box. Since we do it
           after conversion involving ctm, we need to apply current zoom and
           rotation. This is probably not the best place to be doing this
           but it's the only one we managed to make work */
        double offX = dm->pdfDoc->getCatalog()->getPage(pdfLink->pageNo)->getCropBox()->x1;
        double offY = dm->pdfDoc->getCatalog()->getPage(pdfLink->pageNo)->getCropBox()->y1;
        if (flippedRotation(dm->rotation)) {
            double tmp = offX;
            offX = offY;
            offY = tmp;
        }
        offX = offX * dm->zoomReal * 0.01;
        offY = offY * dm->zoomReal * 0.01;
#else
        pdfLink->rectCanvas.x = (int)rect.x;
        pdfLink->rectCanvas.y = (int)rect.y;
        pdfLink->rectCanvas.dx = (int)rect.dx;
        pdfLink->rectCanvas.dy = (int)rect.dy;
#endif
#if 0
        DBG_OUT("  link on page (x=%d, y=%d, dx=%d, dy=%d),\n",
            (int)pdfLink->rectPage.x, (int)pdfLink->rectPage.y,
            (int)pdfLink->rectPage.dx, (int)pdfLink->rectPage.dy);
        DBG_OUT("        screen (x=%d, y=%d, dx=%d, dy=%d)\n",
                (int)rect.x, (int)rect.y,
                (int)rect.dx, (int)rect.dy);
#endif
    }
}

void DisplayModel::clearSearchHit(void)
{
    DBG_OUT("DisplayModel::clearSearchHit()\n");
    searchHitPageNo = INVALID_PAGE_NO;
}

void DisplayModel::setSearchHit(int pageNo, RectD *hitRect)
{
    //DBG_OUT("DisplayModel::setSearchHit() page=%d at pos (%.2f, %.2f)-(%.2f,%.2f)\n", pageNo, xs, ys, xe, ye);
    searchHitPageNo = pageNo;
    searchHitRectPage = *hitRect;
    recalcSearchHitCanvasPos();
}

/* Given position 'x'/'y' in the draw area, returns a structure describing
   a link or NULL if there is no link at this position.
   Note: DisplayModelSplash owns this memory so it should not be changed by the
   caller and caller should not reference it after it has changed (i.e. process
   it immediately since it will become invalid after each _relayout()).
   TODO: this function is called frequently from UI code so make sure that
         it's fast enough for a decent number of link.
         Possible speed improvement: remember which links are visible after
         scrolling and skip the _Inside test for those invisible.
         Another way: build another list with only those visible, so we don't
         even have to travers those that are invisible.
   */
PdfLink *DisplayModel::linkAtPosition(int x, int y)
{
    if (0 == _linkCount) return NULL;
    assert(_links);
    if (!_links) return NULL;

    int canvasPosX = x + (int)areaOffset.x;
    int canvasPosY = y + (int)areaOffset.y;
    for (int i = 0; i < _linkCount; i++) {
        PdfLink *currLink = link(i);

        if (RectI_Inside(&(currLink->rectCanvas), canvasPosX, canvasPosY))
            return currLink;
    }
    return NULL;
}

/* Send the request to render a given page to a rendering thread */
void DisplayModel::startRenderingPage(int pageNo)
{
    RenderQueue_Add(this, pageNo);
}

void DisplayModel::renderVisibleParts(void)
{
    int             pageNo;
    PdfPageInfo*    pageInfo;
    int             lastVisible = 0;

//    DBG_OUT("DisplayModel::renderVisibleParts()\n");
    for (pageNo = 1; pageNo <= pageCount(); ++pageNo) {
        pageInfo = getPageInfo(pageNo);
        if (pageInfo->visible) {
            assert(pageInfo->shown);
            startRenderingPage(pageNo);
            lastVisible = pageNo;
        }
    }
    assert(0 != lastVisible);
#ifdef PREDICTIVE_RENDER
    if (lastVisible != pageCount())
        startRenderingPage(lastVisible+1);
#endif
}

void DisplayModel::changeTotalDrawAreaSize(SizeD totalDrawAreaSize)
{
    int     newPageNo;
    int     currPageNo;

    currPageNo = currentPageNo();

    setTotalDrawAreaSize(totalDrawAreaSize);

    relayout(zoomVirtual(), rotation());
    recalcVisibleParts();
    recalcLinksCanvasPos();
    renderVisibleParts();
    setScrollbarsState();
    newPageNo = currentPageNo();
    if (newPageNo != currPageNo)
        pageChanged();
    repaintDisplay(true);
}

void DisplayModel::goToPage(int pageNo, int scrollY, int scrollX)
{
    assert(validPageNo(pageNo));
    if (!validPageNo(pageNo))
        return;

    /* in facing mode only start at odd pages (odd because page
       numbering starts with 1, so odd is really an even page) */
    if (displayModeFacing(displayMode()))
      pageNo = ((pageNo-1) & ~1) + 1;

    if (!displayModeContinuous(displayMode())) {
        /* in single page mode going to another page involves recalculating
           the size of canvas */
        changeStartPage(pageNo);
    }
    //DBG_OUT("DisplayModel::goToPage(pageNo=%d, scrollY=%d)\n", pageNo, scrollY);
    if (-1 != scrollX)
        areaOffset.x = (double)scrollX;
    PdfPageInfo * pageInfo = getPageInfo(pageNo);

    /* Hack: if an image is smaller in Y axis than the draw area, then we center
       the image by setting pageInfo->currPosY in RecalcPagesInfo. So we shouldn't
       scroll (adjust areaOffset.y) there because it defeats the purpose.
       TODO: is there a better way of y-centering?
       TODO: it probably doesn't work in continuous mode (but that's a corner
             case, I hope) */
    if (!displayModeContinuous(displayMode()))
        areaOffset.y = (double)scrollY;
    else
        areaOffset.y = pageInfo->currPosY - PADDING_PAGE_BORDER_TOP + (double)scrollY;
    /* TODO: prevent scrolling too far */

    recalcVisibleParts();
    recalcLinksCanvasPos();
    renderVisibleParts();
    setScrollbarsState();
    pageChanged();
    repaintDisplay(true);
}


void DisplayModel::changeDisplayMode(DisplayMode displayMode)
{
    if (_displayMode == displayMode)
        return;

    _displayMode = displayMode;
    int currPageNo = currentPageNo();
    if (displayModeContinuous(displayMode)) {
        /* mark all pages as shown but not yet visible. The equivalent code
           for non-continuous mode is in DisplayModel::changeStartPage() called
           from DisplayModel::goToPage() */
        for (int pageNo = 1; pageNo <= pageCount(); pageNo++) {
            PdfPageInfo *pageInfo = &(pagesInfo[pageNo-1]);
            pageInfo->shown = true;
            pageInfo->visible = false;
        }
        relayout(zoomVirtual(), rotation());
    }
    goToPage(currPageNo, 0);
}

/* given 'columns' and an absolute 'pageNo', return the number of the first
   page in a row to which a 'pageNo' belongs e.g. if 'columns' is 2 and we
   have 5 pages in 3 rows:
   (1,2)
   (3,4)
   (5)
   then, we return 1 for pages (1,2), 3 for (3,4) and 5 for (5).
   This is 1-based index, not 0-based. */
static int FirstPageInARowNo(int pageNo, int columns)
{
    int row = ((pageNo - 1) / columns); /* 0-based row number */
    int firstPageNo = row * columns + 1; /* 1-based page in a row */
    return firstPageNo;
}

/* In continuous mode just scrolls to the next page. In single page mode
   rebuilds the display model for the next page.
   Returns true if advanced to the next page or false if couldn't advance
   (e.g. because already was at the last page) */
bool DisplayModel::goToNextPage(int scrollY)
{
    int columns = columnsFromDisplayMode(displayMode());
    int currPageNo = currentPageNo();
    int firstPageInCurrRow = FirstPageInARowNo(currPageNo, columns);
    int newPageNo = currPageNo + columns;
    int firstPageInNewRow = FirstPageInARowNo(newPageNo, columns);

//    DBG_OUT("DisplayModel::goToNextPage(scrollY=%d), currPageNo=%d, firstPageInNewRow=%d\n", scrollY, currPageNo, firstPageInNewRow);
    if ((firstPageInNewRow > pageCount()) || (firstPageInCurrRow == firstPageInNewRow)) {
        /* we're on a last row or after it, can't go any further */
        return FALSE;
    }
    goToPage(firstPageInNewRow, scrollY);
    return TRUE;
}

bool DisplayModel::goToPrevPage(int scrollY)
{
    int columns = columnsFromDisplayMode(displayMode());
    int currPageNo = currentPageNo();
    DBG_OUT("DisplayModel::goToPrevPage(scrollY=%d), currPageNo=%d\n", scrollY, currPageNo);
    if (currPageNo <= columns) {
        /* we're on a first page, can't go back */
        return FALSE;
    }
    goToPage(currPageNo - columns, scrollY);
    return TRUE;
}

bool DisplayModel::goToLastPage(void)
{
    DBG_OUT("DisplayModel::goToLastPage()\n");

    int columns = columnsFromDisplayMode(displayMode());
    int currPageNo = currentPageNo();
    int firstPageInLastRow = FirstPageInARowNo(pageCount(), columns);

    if (currPageNo != firstPageInLastRow) { /* are we on the last page already ? */
        goToPage(firstPageInLastRow, 0);
        return TRUE;
    }
    return FALSE;
}

bool DisplayModel::goToFirstPage(void)
{
    DBG_OUT("DisplayModel::goToFirstPage()\n");

    if (displayModeContinuous(displayMode())) {
        if (0 == areaOffset.y) {
            return FALSE;
        }
    } else {
        assert(pageShown(_startPage));
        if (1 == _startPage) {
            /* we're on a first page already */
            return FALSE;
        }
    }
    goToPage(1, 0);
    return TRUE;
}

void DisplayModel::scrollXTo(int xOff)
{
    DBG_OUT("DisplayModel::scrollXTo(xOff=%d)\n", xOff);
    areaOffset.x = (double)xOff;
    recalcVisibleParts();
    recalcLinksCanvasPos();
    setScrollbarsState();
    repaintDisplay(false);
}

void DisplayModel::scrollXBy(int dx)
{
    DBG_OUT("DisplayModel::scrollXBy(dx=%d)\n", dx);

    double maxX = _canvasSize.dx() - drawAreaSize.dx();
    assert(maxX >= 0.0);
    double prevX = areaOffset.x;
    double newX = prevX + (double)dx;
    if (newX < 0.0)
        newX = 0.0;
    else
        if (newX > maxX)
            newX = maxX;

    if (newX == prevX)
        return;

    scrollXTo((int)newX);
}

void DisplayModel::scrollYTo(int yOff)
{
    DBG_OUT("DisplayModel::scrollYTo(yOff=%d)\n", yOff);

    int currPageNo = currentPageNo();
    areaOffset.y = (double)yOff;
    recalcVisibleParts();
    recalcLinksCanvasPos();
    renderVisibleParts();

    int newPageNo = currentPageNo();
    if (newPageNo != currPageNo)
        pageChanged();
    repaintDisplay(false);
}

/* Scroll the doc in y-axis by 'dy'. If 'changePage' is TRUE, automatically
   switch to prev/next page in non-continuous mode if we scroll past the edges
   of current page */
void DisplayModel::scrollYBy(int dy, bool changePage)
{
    PdfPageInfo *   pageInfo;
    int             currYOff = (int)areaOffset.y;
    int             newPageNo;
    int             currPageNo;

    DBG_OUT("DisplayModel::scrollYBy(dy=%d, changePage=%d)\n", dy, (int)changePage);
    assert(0 != dy);
    if (0 == dy) return;

    int newYOff = currYOff;

    if (!displayModeContinuous(displayMode()) && changePage) {
        if ((dy < 0) && (0 == currYOff)) {
            if (_startPage > 1) {
                newPageNo = _startPage-1;
                assert(validPageNo(newPageNo));
                pageInfo = getPageInfo(newPageNo);
                newYOff = (int)pageInfo->currDy - drawAreaSize.dyI();
                if (newYOff < 0)
                    newYOff = 0; /* TODO: center instead? */
                goToPrevPage(newYOff);
                return;
            }
        }

        /* see if we have to change page when scrolling forward */
        if ((dy > 0) && (_startPage < pageCount())) {
            if ((int)areaOffset.y + drawAreaSize.dyI() >= _canvasSize.dyI()) {
                goToNextPage(0);
                return;
            }
        }
    }

    newYOff += dy;
    if (newYOff < 0) {
        newYOff = 0;
    } else if (newYOff + drawAreaSize.dyI() > _canvasSize.dyI()) {
        newYOff = _canvasSize.dyI() - drawAreaSize.dyI();
    }

    if (newYOff == currYOff)
        return;

    currPageNo = currentPageNo();
    areaOffset.y = (double)newYOff;
    recalcVisibleParts();
    recalcLinksCanvasPos();
    renderVisibleParts();
    setScrollbarsState();
    newPageNo = currentPageNo();
    if (newPageNo != currPageNo)
        pageChanged();
    repaintDisplay(false);
}

void DisplayModel::scrollYByAreaDy(bool forward, bool changePage)
{
    int toScroll = drawAreaSize.dyI();
    if (forward)
        scrollYBy(toScroll, changePage);
    else
        scrollYBy(-toScroll, changePage);
}

void DisplayModel::zoomTo(double zoomVirtual)
{
    //DBG_OUT("DisplayModel::zoomTo() zoomVirtual=%.6f\n", _zoomVirtual);
    int currPageNo = currentPageNo();
    relayout(zoomVirtual, rotation());
    goToPage(currPageNo, 0);
}

void DisplayModel::zoomBy(double zoomFactor)
{
    double newZoom = _zoomReal * zoomFactor;
    //DBG_OUT("DisplayModel::zoomBy() zoomReal=%.6f, zoomFactor=%.2f, newZoom=%.2f\n", dm->zoomReal, zoomFactor, newZoom);
    if (newZoom > ZOOM_MAX)
        return;
    zoomTo(newZoom);
}

void DisplayModel::rotateBy(int newRotation)
{
    normalizeRotation(&newRotation);
    assert(0 != newRotation);
    if (0 == newRotation)
        return;
    assert(validRotation(newRotation));
    if (!validRotation(newRotation))
        return;

    newRotation += rotation();
    normalizeRotation(&newRotation);
    assert(validRotation(newRotation));
    if (!validRotation(newRotation))
        return;

    int currPageNo = currentPageNo();
    relayout(zoomVirtual(), newRotation);
    goToPage(currPageNo, 0);
}

void DisplayModel::showNormalCursor(void)
{
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void DisplayModel::showBusyCursor(void)
{
    // TODO: what is the right cursor?
    // can I set it per-window only?
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void LockCache(void) {
    gLockMutex(&cacheMutex);
}

void UnlockCache(void) {
    gUnlockMutex(&cacheMutex);
}

static void BitmapCacheEntry_Free(BitmapCacheEntry *entry) {
    assert(entry);
    if (!entry) return;
    delete entry->bitmap;
    free((void*)entry);
}

void BitmapCache_FreeAll(void) {
    LockCache();
    for (int i=0; i < gBitmapCacheCount; i++) {
        BitmapCacheEntry_Free(gBitmapCache[i]);
        gBitmapCache[i] = NULL;
    }
    gBitmapCacheCount = 0;
    UnlockCache();
}

/* Free all bitmaps in the cache that are not visible. Returns true if freed
   at least one item. */
bool BitmapCache_FreeNotVisible(void) {
    LockCache();
    bool freedSomething = false;
    int cacheCount = gBitmapCacheCount;
    int curPos = 0;
    for (int i = 0; i < cacheCount; i++) {
        BitmapCacheEntry* entry = gBitmapCache[i];
        bool shouldFree = !entry->dm->pageVisibleNearby(entry->pageNo);
         if (shouldFree) {
            if (!freedSomething)
                DBG_OUT("BitmapCache_FreeNotVisible() ");
            DBG_OUT("freed %d ", entry->pageNo);
            freedSomething = true;
            BitmapCacheEntry_Free(gBitmapCache[i]);
            gBitmapCache[i] = NULL;
            --gBitmapCacheCount;
        }

        if (curPos != i)
            gBitmapCache[curPos] = gBitmapCache[i];

        if (!shouldFree)
             ++curPos;
    }
    UnlockCache();
    if (freedSomething)
        DBG_OUT("\n");
    return freedSomething;
}

static bool BitmapCache_FreePage(DisplayModel *dm, int pageNo) {
    LockCache();
    int cacheCount = gBitmapCacheCount;
    bool freedSomething = false;
    int curPos = 0;
    for (int i = 0; i < cacheCount; i++) {
        bool shouldFree = (gBitmapCache[i]->dm == dm) && (gBitmapCache[i]->pageNo == pageNo);
        if (shouldFree) {
            if (!freedSomething)
                DBG_OUT("BitmapCache_FreePage() ");
            DBG_OUT("freed %d ", gBitmapCache[i]->pageNo);
            freedSomething = true;
            BitmapCacheEntry_Free(gBitmapCache[i]);
            gBitmapCache[i] = NULL;
            --gBitmapCacheCount;
        }

        if (curPos != i)
            gBitmapCache[curPos] = gBitmapCache[i];

        if (!shouldFree)
            ++curPos;
    }
    UnlockCache();
    if (freedSomething)
        DBG_OUT("\n");
    return freedSomething;
}

/* Free all bitmaps cached for a given <dm>. Returns TRUE if freed
   at least one item. */
bool BitmapCache_FreeForDisplayModel(DisplayModel *dm) {
    LockCache();
    int cacheCount = gBitmapCacheCount;
    bool freedSomething = false;
    int curPos = 0;
    for (int i = 0; i < cacheCount; i++) {
        bool shouldFree = (gBitmapCache[i]->dm == dm);
        if (shouldFree) {
            if (!freedSomething)
                DBG_OUT("BitmapCache_FreeForDisplayModel() ");
            DBG_OUT("freed %d ", gBitmapCache[i]->pageNo);
            freedSomething = true;
            BitmapCacheEntry_Free(gBitmapCache[i]);
            gBitmapCache[i] = NULL;
            --gBitmapCacheCount;
        }

        if (curPos != i)
            gBitmapCache[curPos] = gBitmapCache[i];

        if (!shouldFree)
            ++curPos;
    }
    UnlockCache();
    if (freedSomething)
        DBG_OUT("\n");
    return freedSomething;
}

void BitmapCache_Add(DisplayModel *dm, int pageNo, double zoomLevel, int rotation, 
    RenderedBitmap *bitmap, double renderTime) {
    assert(gBitmapCacheCount <= MAX_BITMAPS_CACHED);
    assert(dm);
    assert(validRotation(rotation));

    normalizeRotation(&rotation);
    DBG_OUT("BitmapCache_Add(pageNo=%d, zoomLevel=%.2f%%, rotation=%d)\n", pageNo, zoomLevel, rotation);
    LockCache();

    /* It's possible there still is a cached bitmap with different zoomLevel/rotation */
    BitmapCache_FreePage(dm, pageNo);

    if (gBitmapCacheCount >= MAX_BITMAPS_CACHED - 1) {
        /* TODO: find entry that is not visible and remove it from cache to
           make room for new entry */
        delete bitmap;
	/* @note: crossing initialization of "BitmapCacheEntry* entry" not allowed in mingw */
        //goto UnlockAndExit; 
	UnlockCache();
	return;
    }
    BitmapCacheEntry* entry = (BitmapCacheEntry*)malloc(sizeof(BitmapCacheEntry));
    if (!entry) {
        delete bitmap;
        goto UnlockAndExit;
    }
    entry->dm = dm;
    entry->pageNo = pageNo;
    entry->zoomLevel = zoomLevel;
    entry->rotation = rotation;
    entry->bitmap = bitmap;
    entry->renderTime = renderTime;
    gBitmapCache[gBitmapCacheCount++] = entry;
UnlockAndExit:
    UnlockCache();
}

BitmapCacheEntry *BitmapCache_Find(DisplayModel *dm, int pageNo) {
    BitmapCacheEntry* entry;
    LockCache();
    for (int i = 0; i < gBitmapCacheCount; i++) {
        entry = gBitmapCache[i];
        if ( (dm == entry->dm) && (pageNo == entry->pageNo) ) {
             goto Exit;
        }
    }
    entry = NULL;
Exit:
    UnlockCache();
    return entry;
}

BitmapCacheEntry *BitmapCache_Find(DisplayModel *dm, int pageNo, double zoomLevel, int rotation) {
    BitmapCacheEntry *entry;
    normalizeRotation(&rotation);
    LockCache();
    for (int i = 0; i < gBitmapCacheCount; i++) {
        entry = gBitmapCache[i];
        if ( (dm == entry->dm) && (pageNo == entry->pageNo) && 
             (zoomLevel == entry->zoomLevel) && (rotation == entry->rotation)) {
             goto Exit;
        }
    }
    entry = NULL;
Exit:
    UnlockCache();
    return entry;
}

/* Return true if a bitmap for a page defined by <dm>, <pageNo>, <zoomLevel>
   and <rotation> exists in the cache */
bool BitmapCache_Exists(DisplayModel *dm, int pageNo, double zoomLevel, int rotation) {
    if (BitmapCache_Find(dm, pageNo, zoomLevel, rotation))
        return true;
    return false;
}

