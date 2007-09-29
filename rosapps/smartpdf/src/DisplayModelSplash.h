/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
#ifndef _DISPLAY_MODEL_SPLASH_H_
#define _DISPLAY_MODEL_SPLASH_H_
/* How to think of display logic: physical screen of size
   drawAreaSize is a window into (possibly much larger)
   total area (canvas) of size canvasSize.

   In DM_SINGLE_PAGE mode total area is the size of currently displayed page
   given current zoomLovel and rotation.
   In DM_CONTINUOUS mode total area consist of all pages rendered sequentially
   with a given zoomLevel and rotation. totalAreaDy is the sum of heights
   of all pages plus spaces between them and totalAreaDx is the size of
   the widest page.

   A possible configuration could look like this:

 -----------------------------------
 |                                 |
 |          -------------          |
 |          | screen    |          |
 |          | i.e.      |          |
 |          | drawArea  |          |
 |          -------------          |
 |                                 |
 |                                 |
 |    canvas                       |
 |                                 |
 |                                 |
 |                                 |
 |                                 |
 -----------------------------------

  We calculate the total area size and position of each page we display on the
  canvas. Canvas has to be >= draw area.

  Changing zoomLevel or rotation requires recalculation of total area and
  position of pdf pages in it.

  We keep the offset of draw area relative to total area. The offset changes
  due to scrolling (with keys or using scrollbars).

  To draw we calculate which part of each page overlaps draw area, we render
  those pages to a bitmap and display those bitmaps.
*/

#include "DisplayState.h"
#include "DisplayModel.h"

class GooString;
class Link;
class LinkDest;
class LinkGoTo;
class LinkGoToR;
class LinkLaunch;
class LinkNamed;
class LinkURI;
class Links;
class PDFDoc;
class SplashBitmap;
class TextOutputDev;
class TextPage;
class UGooString;

/* Information needed to drive the display of a given PDF document on a screen.
   You can think of it as a model in the MVC pardigm.
   All the display changes should be done through changing this model via
   API and re-displaying things based on new display information */
class DisplayModelSplash : public DisplayModel
{
public:
    DisplayModelSplash(DisplayMode displayMode);
    virtual ~DisplayModelSplash();

    PdfEnginePoppler * pdfEnginePoppler() { return (PdfEnginePoppler*)pdfEngine(); }

    virtual void  handleLink(PdfLink *pdfLink);

    TextPage *    GetTextPage(int pageNo);

    void          EnsureSearchHitVisible();

    void          handleLinkGoTo(LinkGoTo *linkGoTo);
    void          handleLinkGoToR(LinkGoToR *linkGoToR);
    void          handleLinkURI(LinkURI *linkURI);
    void          handleLinkLaunch(LinkLaunch* linkLaunch);
    void          handleLinkNamed(LinkNamed *linkNamed);
    BOOL          CanGoToNextPage();
    BOOL          CanGoToPrevPage();

    void          FindInit(int startPageNo);
    BOOL          FindNextForward();
    BOOL          FindNextBackward();


    void          FreeTextPages(void);
    void          RecalcLinks(void);
    void          GoToDest(LinkDest *linkDest);
    void          GoToNamedDest(UGooString *dest);
    void          FreeLinks(void);

    virtual int   getTextInRegion(int pageNo, RectD *region, unsigned short *buf, int buflen);
    virtual void  cvtUserToScreen(int pageNo, double *x, double *y);
    virtual void  cvtScreenToUser(int *pageNo, double *x, double *y);

public:
    PDFDoc *            pdfDoc;
};

DisplayModelSplash *DisplayModelSplash_CreateFromFileName(const char *fileName,
                                            SizeD totalDrawAreaSize,
                                            int scrollbarXDy, int scrollbarYDx,
                                            DisplayMode displayMode, int startPage,
                                            WindowInfo *win);
#endif
