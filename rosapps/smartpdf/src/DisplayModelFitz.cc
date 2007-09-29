/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
#include "DisplayModelFitz.h"

#include "str_util.h"

DisplayModelFitz::DisplayModelFitz(DisplayMode displayMode) :
    DisplayModel(displayMode)
{
    _pdfEngine = new PdfEngineFitz();
}

DisplayModelFitz::~DisplayModelFitz()
{
    RenderQueue_RemoveForDisplayModel(this);
    BitmapCache_FreeForDisplayModel(this);
    cancelRenderingForDisplayModel(this);
}

DisplayModelFitz *DisplayModelFitz_CreateFromFileName(
  const char *fileName,
  SizeD totalDrawAreaSize,
  int scrollbarXDy, int scrollbarYDx,
  DisplayMode displayMode, int startPage,
  WindowInfo *win)
{
    DisplayModelFitz *    dm = NULL;

    dm = new DisplayModelFitz(displayMode);
    if (!dm)
        goto Error;

    if (!dm->load(fileName, startPage, win))
        goto Error;

    dm->setScrollbarsSize(scrollbarXDy, scrollbarYDx);
    dm->setTotalDrawAreaSize(totalDrawAreaSize);

//    DBG_OUT("DisplayModelFitz_CreateFromPageTree() pageCount = %d, startPage=%d, displayMode=%d\n",
//        dm->pageCount(), (int)dm->startPage, (int)displayMode);
    return dm;
Error:
    delete dm;
    return NULL;
}

void DisplayModelFitz::cvtUserToScreen(int pageNo, double *x, double *y)
{
    pdf_page *page = pdfEngineFitz()->getPdfPage(pageNo);
    double zoom = zoomReal();
    int rot = rotation();
    fz_point p;

    normalizeRotation (&rot);
    zoom *= 0.01;

    p.x = *x;
    p.y = *y;

    fz_matrix ctm = pdfEngineFitz()->viewctm(page, zoom, rot);

    fz_point tp = fz_transformpoint (ctm, p);

    PdfPageInfo *pageInfo = getPageInfo(pageNo);

    double vx = 0, vy = 0;
    if (rot == 90 || rot == 180)
        vx += pageInfo->currDx;
    if (rot == 180 || rot == 270)
        vy += pageInfo->currDy;

    *x = tp.x + 0.5 + pageInfo->screenX - pageInfo->bitmapX + vx;
    *y = tp.y + 0.5 + pageInfo->screenY - pageInfo->bitmapY + vy;

}

void DisplayModelFitz::cvtScreenToUser(int *pageNo, double *x, double *y)
{
    double zoom = zoomReal();
    int rot = rotation();
    fz_point p;

    normalizeRotation (&rot);
    zoom *= 0.01;

    *pageNo = getPageNoByPoint (*x, *y);
    if (*pageNo == POINT_OUT_OF_PAGE) return;

    pdf_page *page = pdfEngineFitz()->getPdfPage(*pageNo);

    PdfPageInfo *pageInfo = getPageInfo(*pageNo);

    p.x = *x - 0.5 - pageInfo->screenX + pageInfo->bitmapX;
    p.y = *y - 0.5 - pageInfo->screenY + pageInfo->bitmapY;

    fz_matrix ctm = pdfEngineFitz()->viewctm(page, zoom, rot);
    fz_matrix invCtm = fz_invertmatrix(ctm);

    fz_point tp = fz_transformpoint (invCtm, p);

    double vx = 0, vy = 0;
    if (rot == 90 || rot == 180)
        vy -= pageInfo->pageDy;
    if (rot == 180 || rot == 270)
        vx += pageInfo->pageDx;

    *x = tp.x + vx;
    *y = tp.y + vy;
}

void DisplayModelFitz::handleLink(PdfLink *pdfLink)
{
    // TODO: implement me
}

/* Given <region> (in user coordinates ) on page <pageNo>, copies text in that region
 * to <buf>. Returnes number of copied characters */
int DisplayModelFitz::getTextInRegion(int pageNo, RectD *region, unsigned short *buf, int buflen)
{
    int             bxMin, bxMax, byMin, byMax;
    int             xMin, xMax, yMin, yMax;
    pdf_textline *  line, *ln;
    fz_error *      error;
    pdf_page *      page = pdfEngineFitz()->getPdfPage(pageNo);
    fz_tree *       tree = page->tree;
    double          rot = 0;
    double          zoom = 1;

    error = pdf_loadtextfromtree(&line, tree, fz_identity());
    if (error)
        return NULL;

    xMin = region->x;
    xMax = xMin + region->dx;
    yMin = region->y;
    yMax = yMin + region->dy;

    int p = 0;
    for (ln = line; ln; ln = ln->next) {
        int oldP = p;
        for (int i = 0; i < ln->len; i++) {
            bxMin = ln->text[i].bbox.x0;
            bxMax = ln->text[i].bbox.x1;
            byMin = ln->text[i].bbox.y0;
            byMax = ln->text[i].bbox.y1;
            int c = ln->text[i].c;
            if (c < 32)
                c = '?';
            if (bxMax >= xMin && bxMin <= xMax && byMax >= yMin && byMin <= yMax 
                    && p < buflen) {
                buf[p++] = c;
                //DBG_OUT("Char: %c : %d; ushort: %hu\n", (char)c, (int)c, (unsigned short)c);
                //DBG_OUT("Found char: %c : %hu; real %c : %hu\n", c, (unsigned short)(unsigned char) c, ln->text[i].c, ln->text[i].c);
            }
        }

        if (p > oldP && p < buflen - 1) {
#ifdef WIN32
            buf[p++] = '\r'; buf[p++] = 10;
            //DBG_OUT("Char: %c : %d; ushort: %hu\n", (char)buf[p], (int)(unsigned char)buf[p], buf[p]);
#else
            buf[p++] = '\n';
#endif
        }
    }

    pdf_droptextline(line);

    return p;
}
