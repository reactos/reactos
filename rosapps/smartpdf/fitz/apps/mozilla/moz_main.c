#include "fitz.h"
#include "mupdf.h"

#include <windows.h>
#include <windowsx.h>

#include "npapi.h"
#include "npupp.h"

#define PAD 5

#define MSG(s) MessageBox(0,s,"MuPDF Debug",MB_OK)

typedef struct pdfmoz_s pdfmoz_t;
typedef struct page_s page_t;

struct page_s
{
    fz_obj *ref;
    fz_obj *obj;
    pdf_page *page;
    fz_pixmap *image;
    int w, h;	/* page size in units */
    int px; /* pixel height */
};

struct pdfmoz_s
{
    NPP inst;
    HWND hwnd;
    HWND sbar;
    WNDPROC winproc;
    HCURSOR arrow, hand, wait;
    BITMAPINFO *dibinf;
    HBRUSH graybrush;

    int scrollpage;	/* scrollbar -> page (n) */
    int scrollyofs;	/* scrollbar -> page offset in pixels */

    int pagecount;
    page_t *pages;

    char *filename;
    char *doctitle;

    pdf_xref *xref;
    fz_renderer *rast;

    char error[1024];	/* empty if no error has occured */
};

void pdfmoz_warn(pdfmoz_t *moz, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    va_end(ap);
    strcpy(moz->error, buf);
    InvalidateRect(moz->hwnd, NULL, FALSE);
    NPN_Status(moz->inst, moz->error);
}

void pdfmoz_error(pdfmoz_t *moz, fz_error *error)
{
    strcpy(moz->error, error->msg);
    InvalidateRect(moz->hwnd, NULL, FALSE);
    NPN_Status(moz->inst, moz->error);
}

void pdfmoz_open(pdfmoz_t *moz, char *filename)
{
    SCROLLINFO si;
    fz_error *error;
    fz_obj *obj;
    char *password = "";
    pdf_pagetree *pages;
    fz_irect bbox;
    int rot;
    int i;

    strcpy(moz->error, "");

    error = fz_newrenderer(&moz->rast, pdf_devicergb, 0, 1024 * 512);
    if (error)
	pdfmoz_error(moz, error);

    /*
     * Open PDF and load xref table
     */

    moz->filename = filename;

    error = pdf_newxref(&moz->xref);
    if (error)
	pdfmoz_error(moz, error);

    error = pdf_loadxref(moz->xref, filename);
    if (error)
    {
	if (!strncmp(error->msg, "ioerror", 7))
	    pdfmoz_error(moz, error);
	error = pdf_repairxref(moz->xref, filename);
	if (error)
	    pdfmoz_error(moz, error);
    }

    /*
     * Handle encrypted PDF files
     */

    error = pdf_decryptxref(moz->xref);
    if (error)
	pdfmoz_error(moz, error);

    if (moz->xref->crypt)
    {
	error = pdf_setpassword(moz->xref->crypt, password);
	//	while (error)
	//	{
	//		fz_droperror(error);
	//		password = winpassword(moz, filename);
	//		if (!password)
	//			exit(1);
	//		error = pdf_setpassword(moz->xref->crypt, password);
	if (error)
	    pdfmoz_warn(moz, "Invalid password.");
	//	}
    }

    /*
     * Load page tree
     */

    error = pdf_loadpagetree(&pages, moz->xref);
    if (error)
	pdfmoz_error(moz, error);

    moz->pagecount = pdf_getpagecount(pages);
    moz->pages = fz_malloc(sizeof(page_t) * moz->pagecount);

    for (i = 0; i < moz->pagecount; i++)
    {
	moz->pages[i].ref = fz_keepobj(pages->pref[i]);
	moz->pages[i].obj = fz_keepobj(pdf_getpageobject(pages, i));
	moz->pages[i].page = nil;
	moz->pages[i].image = nil;

	obj = fz_dictgets(moz->pages[i].obj, "CropBox");
	if (!obj)
	    obj = fz_dictgets(moz->pages[i].obj, "MediaBox");
	bbox = fz_roundrect(pdf_torect(obj));
	moz->pages[i].w = bbox.x1 - bbox.x0;
	moz->pages[i].h = bbox.y1 - bbox.y0;

	rot = fz_toint(fz_dictgets(moz->pages[i].obj, "Rotate"));
	if ((rot / 90) % 2)
	{
	    int t = moz->pages[i].w;
	    moz->pages[i].w = moz->pages[i].h;
	    moz->pages[i].h = t;
	}

	moz->pages[i].px = 1 + PAD;
    }

    pdf_droppagetree(pages);

    /*
     * Load meta information
     * TODO: move this into mupdf library
     */

    obj = fz_dictgets(moz->xref->trailer, "Root");
    if (!obj)
	pdfmoz_error(moz, fz_throw("syntaxerror: missing Root object"));

    error = pdf_loadindirect(&moz->xref->root, moz->xref, obj);
    if (error)
	pdfmoz_error(moz, error);

    obj = fz_dictgets(moz->xref->trailer, "Info");
    if (obj)
    {
	error = pdf_loadindirect(&moz->xref->info, moz->xref, obj);
	if (error)
	    pdfmoz_error(moz, error);
    }

    error = pdf_loadnametrees(moz->xref);
    if (error)
	pdfmoz_error(moz, error);

    moz->doctitle = filename;
    if (strrchr(moz->doctitle, '\\'))
	moz->doctitle = strrchr(moz->doctitle, '\\') + 1;
    if (strrchr(moz->doctitle, '/'))
	moz->doctitle = strrchr(moz->doctitle, '/') + 1;
    if (moz->xref->info)
    {
	obj = fz_dictgets(moz->xref->info, "Title");
	if (obj)
	{
	    error = pdf_toutf8(&moz->doctitle, obj);
	    if (error)
		pdfmoz_error(moz, error);
	}
    }

    /*
     * Start at first page
     */

    si.cbSize = sizeof(si);
    si.fMask = SIF_POS | SIF_RANGE; // XXX | SIF_PAGE;
    si.nPos = 0;
    si.nMin = 0;
    si.nMax = 1;
    si.nPage = 1;
    SetScrollInfo(moz->hwnd, SB_VERT, &si, TRUE);

    moz->scrollpage = 0;
    moz->scrollyofs = 0;

    InvalidateRect(moz->hwnd, NULL, FALSE);
}

static void decodescroll(pdfmoz_t *moz, int spos)
{
    int i, y = 0;
    moz->scrollpage = 0;
    moz->scrollyofs = 0;
    for (i = 0; i < moz->pagecount; i++)
    {
	if (spos >= y && spos < y + moz->pages[i].px)
	{
	    moz->scrollpage = i;
	    moz->scrollyofs = spos - y;
	    return;
	}
	y += moz->pages[i].px;
    }
}

fz_matrix pdfmoz_pagectm(pdfmoz_t *moz, int pagenum)
{
    page_t *page = moz->pages + pagenum;
    fz_matrix ctm;
    float zoom;
    RECT rc;

    GetClientRect(moz->hwnd, &rc);

    zoom = (rc.right - rc.left) / (float) page->w;

    ctm = fz_identity();
    ctm = fz_concat(ctm, fz_translate(0, -page->page->mediabox.y1));
    ctm = fz_concat(ctm, fz_scale(zoom, -zoom));
    ctm = fz_concat(ctm, fz_rotate(page->page->rotate));

    return ctm;
}

void pdfmoz_loadpage(pdfmoz_t *moz, int pagenum)
{
    page_t *page = moz->pages + pagenum;
    fz_error *error;

    if (page->page)
	return;

    error = pdf_loadpage(&page->page, moz->xref, page->obj);
    if (error)
	pdfmoz_error(moz, error);
}

void pdfmoz_drawpage(pdfmoz_t *moz, int pagenum)
{
    page_t *page = moz->pages + pagenum;
    fz_error *error;
    fz_matrix ctm;
    fz_rect bbox;

    if (page->image)
	return;

    ctm = pdfmoz_pagectm(moz, pagenum);
    bbox = fz_transformaabb(ctm, page->page->mediabox);

    error = fz_rendertree(&page->image, moz->rast, page->page->tree,
	    ctm, fz_roundrect(bbox), 1);
    if (error)
	pdfmoz_error(moz, error);
}

void pdfmoz_gotouri(pdfmoz_t *moz, fz_obj *uri)
{
    char buf[2048];
    memcpy(buf, fz_tostrbuf(uri), fz_tostrlen(uri));
    buf[fz_tostrlen(uri)] = 0;
    NPN_GetURL(moz->inst, buf, "_blank");
}

int pdfmoz_getpagenum(pdfmoz_t *moz, fz_obj *obj)
{
    int oid = fz_tonum(obj);
    int i;
    for (i = 0; i < moz->pagecount; i++)
	if (fz_tonum(moz->pages[i].ref) == oid)
	    return i;
    return 0;
}

void pdfmoz_gotopage(pdfmoz_t *moz, fz_obj *obj)
{
    int oid = fz_tonum(obj);
    int i, y = 0;
    for (i = 0; i < moz->pagecount; i++)
    {
	if (fz_tonum(moz->pages[i].ref) == oid)
	{
	    SetScrollPos(moz->hwnd, SB_VERT, y, TRUE);
	    InvalidateRect(moz->hwnd, NULL, FALSE);
	    return;
	}
	y += moz->pages[i].px;
    }
}

void pdfmoz_onmouse(pdfmoz_t *moz, int x, int y, int click)
{
    char buf[512];
    pdf_link *link;
    fz_matrix ctm;
    fz_point p;
    int pi;
    int py;

    if (!moz->pages)
	return;

    pi = moz->scrollpage;
    py = -moz->scrollyofs;
    while (pi < moz->pagecount)
    {
	if (!moz->pages[pi].image)
	    return;
	if (y > py && y < moz->pages[pi].px)
	    break;
	py += moz->pages[pi].px;
	pi ++;
    }
    if (pi == moz->pagecount)
	return;

    p.x = x + moz->pages[pi].image->x;
    p.y = y + moz->pages[pi].image->y - py;

    ctm = pdfmoz_pagectm(moz, pi);
    ctm = fz_invertmatrix(ctm);

    p = fz_transformpoint(ctm, p);

    for (link = moz->pages[pi].page->links; link; link = link->next)
    {
	if (p.x >= link->rect.x0 && p.x <= link->rect.x1)
	    if (p.y >= link->rect.y0 && p.y <= link->rect.y1)
		break;
    }

    if (link)
    {
	SetCursor(moz->hand);
	if (click)
	{
	    if (fz_isstring(link->dest))
		pdfmoz_gotouri(moz, link->dest);
	    if (fz_isindirect(link->dest))
		pdfmoz_gotopage(moz, link->dest);
	    return;
	}
	else
	{
	    if (fz_isstring(link->dest))
	    {
		memcpy(buf, fz_tostrbuf(link->dest), fz_tostrlen(link->dest));
		buf[fz_tostrlen(link->dest)] = 0;
		NPN_Status(moz->inst, buf);
	    }
	    else if (fz_isindirect(link->dest))
	    {
		sprintf(buf, "Go to page %d",
			pdfmoz_getpagenum(moz, link->dest) + 1);
		NPN_Status(moz->inst, buf);
	    }
	    else
		NPN_Status(moz->inst, "Say what?");
	}
    }
    else
    {
	sprintf(buf, "Page %d of %d", moz->scrollpage + 1, moz->pagecount);
	NPN_Status(moz->inst, buf);
	SetCursor(moz->arrow);
    }
}

static void drawimage(HDC hdc, pdfmoz_t *moz, fz_pixmap *image, int yofs)
{
    int bmpstride = ((image->w * 3 + 3) / 4) * 4;
    char *bmpdata = fz_malloc(image->h * bmpstride);
    int x, y;

    if (!bmpdata)
	return;

    for (y = 0; y < image->h; y++)
    {
	char *p = bmpdata + y * bmpstride;
	char *s = image->samples + y * image->w * 4;
	for (x = 0; x < image->w; x++)
	{
	    p[x * 3 + 0] = s[x * 4 + 3];
	    p[x * 3 + 1] = s[x * 4 + 2];
	    p[x * 3 + 2] = s[x * 4 + 1];
	}
    }

    moz->dibinf->bmiHeader.biWidth = image->w;
    moz->dibinf->bmiHeader.biHeight = -image->h;
    moz->dibinf->bmiHeader.biSizeImage = image->h * bmpstride;
    SetDIBitsToDevice(hdc,
	    0, /* destx */
	    yofs, /* desty */
	    image->w, /* destw */
	    image->h, /* desth */
	    0, /* srcx */
	    0, /* srcy */
	    0, /* startscan */
	    image->h, /* numscans */
	    bmpdata, /* pBits */
	    moz->dibinf, /* pInfo */
	    DIB_RGB_COLORS /* color use flag */
		     );

    fz_free(bmpdata);
}

LRESULT CALLBACK
MozWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    pdfmoz_t *moz = (pdfmoz_t*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    char buf[256];

    int x = (signed short) LOWORD(lParam);
    int y = (signed short) HIWORD(lParam);
    int i, h;

    SCROLLINFO si;
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc;
    RECT pad;
    WORD sendmsg;
    float zoom;

    GetClientRect(hwnd, &rc);
    h = rc.bottom - rc.top;

    if (strlen(moz->error))
    {
	if (msg == WM_PAINT)
	{
	    hdc = BeginPaint(hwnd, &ps);
	    FillRect(hdc, &rc, GetStockBrush(WHITE_BRUSH));
	    rc.top += 10;
	    rc.bottom -= 10;
	    rc.left += 10;
	    rc.right -= 10;
	    DrawText(hdc, moz->error, strlen(moz->error), &rc, 0);
		// DT_SINGLELINE|DT_CENTER|DT_VCENTER);
	    EndPaint(hwnd, &ps);
	}
	if (msg == WM_MOUSEMOVE)
	{
	    SetCursor(moz->arrow);
	}
	return 0;
    }

    switch (msg)
    {

    case WM_PAINT:
	GetClientRect(moz->hwnd, &rc);

	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hwnd, SB_VERT, &si);

	decodescroll(moz, si.nPos);

	/* evict out-of-range images and pages */
	for (i = 0; i < moz->pagecount; i++)
	{
	    if (i < moz->scrollpage - 2 || i > moz->scrollpage + 6)
	    {
		if (moz->pages[i].page)
		{
		    pdf_droppage(moz->pages[i].page);
		    moz->pages[i].page = nil;
		}
	    }
	    if (i < moz->scrollpage - 1 || i > moz->scrollpage + 3)
	    {
		if (moz->pages[i].image)
		{
		    fz_droppixmap(moz->pages[i].image);
		    moz->pages[i].image = nil;
		}
	    }
	}

	i = moz->scrollpage;

	pdfmoz_loadpage(moz, i);
	if (moz->error[0]) return 0;

	pdfmoz_drawpage(moz, i);
	if (moz->error[0]) return 0;

	y = -moz->scrollyofs;
	while (y < h && i < moz->pagecount)
	{
	    pdfmoz_loadpage(moz, i);
	    if (moz->error[0]) return 0;
	    pdfmoz_drawpage(moz, i);
	    if (moz->error[0]) return 0;
	    y += moz->pages[i].image->h;
	    i ++;
	}

	hdc = BeginPaint(hwnd, &ps);

	pad.left = rc.left;
	pad.right = rc.right;

	i = moz->scrollpage;
	y = -moz->scrollyofs;
	while (y < h && i < moz->pagecount)
	{
	    drawimage(hdc, moz, moz->pages[i].image, y);
	    y += moz->pages[i].image->h;
	    i ++;

	    pad.top = y;
	    pad.bottom = y + PAD;
	    FillRect(hdc, &pad, moz->graybrush);
	    y += PAD;
	}

	if (y < h)
	{
	    pad.top = y;
	    pad.bottom = h;
	    FillRect(hdc, &pad, moz->graybrush);
	}

	EndPaint(hwnd, &ps);

	return 0;

    case WM_SIZE:
	ShowScrollBar(moz->hwnd, SB_VERT, TRUE);
	GetClientRect(moz->hwnd, &rc);

	si.cbSize = sizeof(si);
	si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
	si.nPos = 0;
	si.nMin = 0;
	si.nMax = 0;
	// si.nPage = MAX(30, rc.bottom - rc.top - 30);
	si.nPage = rc.bottom - rc.top;

	for (i = 0; i < moz->pagecount; i++)
	{
	    zoom = (rc.right - rc.left) / (float) moz->pages[i].w;
	    moz->pages[i].px = zoom * moz->pages[i].h + PAD;

	    if (moz->scrollpage == i)
	    {
		si.nPos = si.nMax;
		if (moz->pages[i].image)
		{
		    si.nPos +=
			moz->pages[i].px *
			moz->scrollyofs /
			moz->pages[i].image->h + 1;
		}
	    }

	    if (moz->pages[i].image)
	    {
		fz_droppixmap(moz->pages[i].image);
		moz->pages[i].image = nil;
	    }

	    si.nMax += moz->pages[i].px;
	}

	si.nMax --;

	SetScrollInfo(moz->hwnd, SB_VERT, &si, TRUE);

	break;

    case WM_MOUSEMOVE:
	pdfmoz_onmouse(moz, x, y, 0);
	break;

    case WM_LBUTTONDOWN:
	SetFocus(hwnd);
	pdfmoz_onmouse(moz, x, y, 1);
	break;

    case WM_VSCROLL:

	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	GetScrollInfo(hwnd, SB_VERT, &si);

	switch (LOWORD(wParam))
	{
	case SB_BOTTOM: si.nPos = si.nMax; break;
	case SB_TOP: si.nPos = 0; break;
	case SB_LINEUP: si.nPos -= 50; break;
	case SB_LINEDOWN: si.nPos += 50; break;
	case SB_PAGEUP: si.nPos -= si.nPage; break;
	case SB_PAGEDOWN: si.nPos += si.nPage; break;
	case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
	case SB_THUMBPOSITION: si.nPos = si.nTrackPos; break;
	}

	si.fMask = SIF_POS;
	si.nPos = MAX(0, MIN(si.nPos, si.nMax));
	SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

	InvalidateRect(moz->hwnd, NULL, FALSE);

	decodescroll(moz, si.nPos);
	sprintf(buf, "Page %d of %d", moz->scrollpage + 1, moz->pagecount);
	NPN_Status(moz->inst, buf);

	return 0;

    case WM_MOUSEWHEEL:
	if ((signed short)HIWORD(wParam) > 0)
	    SendMessage(hwnd, WM_VSCROLL, MAKELONG(SB_LINEUP, 0), 0);
	else
	    SendMessage(hwnd, WM_VSCROLL, MAKELONG(SB_LINEDOWN, 0), 0);
	break;

    case WM_KEYDOWN:
	sendmsg = 0xFFFF;

	switch (wParam)
	{
	case VK_UP: sendmsg = SB_LINEUP; break;
	case VK_PRIOR: sendmsg = SB_PAGEUP; break;
	case ' ':
	case VK_NEXT: sendmsg = SB_PAGEDOWN; break;
	case '\r':
	case VK_DOWN: sendmsg = SB_LINEDOWN; break;
	case VK_HOME: sendmsg = SB_TOP; break;
	case VK_END: sendmsg = SB_BOTTOM; break;
	}

	if (sendmsg != 0xFFFF)
	    SendMessage(hwnd, WM_VSCROLL, MAKELONG(sendmsg, 0), 0);

	/* ick! someone eats events instead of bubbling... not my fault! */

	break;

    default:
	break;

    }

    return moz->winproc(hwnd, msg, wParam, lParam);
}

NPError
NPP_New(NPMIMEType mime, NPP inst, uint16 mode,
	int16 argc, char *argn[], char *argv[], NPSavedData *saved)
{
    pdfmoz_t *moz;

    //MSG("NPP_New");

    moz = fz_malloc(sizeof(pdfmoz_t));
    if (!moz)
	return NPERR_OUT_OF_MEMORY_ERROR;

    memset(moz, 0, sizeof(pdfmoz_t));

    sprintf(moz->error, "MuPDF is loading the file...");

    moz->inst = inst;

    moz->arrow = LoadCursor(NULL, IDC_ARROW);
    moz->hand = LoadCursor(NULL, IDC_HAND);
    moz->wait = LoadCursor(NULL, IDC_WAIT);

    moz->dibinf = fz_malloc(sizeof(BITMAPINFO) + 12);
    if (!moz->dibinf)
	return NPERR_OUT_OF_MEMORY_ERROR;

    moz->dibinf->bmiHeader.biSize = sizeof(moz->dibinf->bmiHeader);
    moz->dibinf->bmiHeader.biPlanes = 1;
    moz->dibinf->bmiHeader.biBitCount = 24;
    moz->dibinf->bmiHeader.biCompression = BI_RGB;
    moz->dibinf->bmiHeader.biXPelsPerMeter = 2834;
    moz->dibinf->bmiHeader.biYPelsPerMeter = 2834;
    moz->dibinf->bmiHeader.biClrUsed = 0;
    moz->dibinf->bmiHeader.biClrImportant = 0;
    moz->dibinf->bmiHeader.biClrUsed = 0;

    moz->graybrush = CreateSolidBrush(RGB(0x70,0x70,0x70));

    inst->pdata = moz;

    return NPERR_NO_ERROR;
}

NPError
NPP_Destroy(NPP inst, NPSavedData **saved)
{
    pdfmoz_t *moz = inst->pdata;
    int i;

    //MSG("NPP_Destroy");

    inst->pdata = NULL;

    DeleteObject(moz->graybrush);

    DestroyCursor(moz->arrow);
    DestroyCursor(moz->hand);
    DestroyCursor(moz->wait);

    fz_free(moz->dibinf);

    for (i = 0; i < moz->pagecount; i++)
    {
	if (moz->pages[i].obj)
	    fz_dropobj(moz->pages[i].obj);
	if (moz->pages[i].page)
	    pdf_droppage(moz->pages[i].page);
	if (moz->pages[i].image)
	    fz_droppixmap(moz->pages[i].image);
    }

    fz_free(moz->pages);

    if (moz->xref)
    {
	if (moz->xref->store)
	{
	    pdf_dropstore(moz->xref->store);
	    moz->xref->store = nil;
	}

	pdf_closexref(moz->xref);
    }

    fz_free(moz);

    return NPERR_NO_ERROR;
}

NPError
NPP_SetWindow(NPP inst, NPWindow *npwin)
{
    pdfmoz_t *moz = inst->pdata;

    if (moz->hwnd != npwin->window)
    {
	moz->hwnd = npwin->window;
	SetWindowLongPtr(moz->hwnd, GWLP_USERDATA, (LONG_PTR)moz);
	moz->winproc = (WNDPROC)
	    SetWindowLongPtr(moz->hwnd, GWLP_WNDPROC, (LONG_PTR)MozWinProc);
    }

    SetFocus(moz->hwnd);

    return NPERR_NO_ERROR;
}

NPError
NPP_NewStream(NPP inst, NPMIMEType type,
	NPStream* stream, NPBool seekable,
	uint16* stype)
{
    //MSG("NPP_NewStream");
    *stype = NP_ASFILE;
    return NPERR_NO_ERROR;
}

NPError
NPP_DestroyStream(NPP inst, NPStream* stream, NPReason reason)
{
    //MSG("NPP_DestroyStream");
    return NPERR_NO_ERROR;
}

int32
NPP_WriteReady(NPP inst, NPStream* stream)
{
    //MSG("NPP_WriteReady");
    return 2147483647;
}

int32
NPP_Write(NPP inst, NPStream* stream, int32 offset, int32 len, void* buffer)
{
    //MSG("NPP_Write");
    return len;
}

void
NPP_StreamAsFile(NPP inst, NPStream* stream, const char* fname)
{
    pdfmoz_t *moz = inst->pdata;
    //MSG("NPP_StreamAsFile");
    pdfmoz_open(moz, (char*)fname);
}

void
NPP_Print(NPP inst, NPPrint* platformPrint)
{
    MSG("Sorry, printing is not supported.");
}

int16
NPP_HandleEvent(NPP inst, void* event)
{
    MSG("handle event\n");
    return 0;
}

void
NPP_URLNotify(NPP inst, const char* url,
	NPReason reason, void* notifyData)
{
    MSG("notify url\n");
}

NPError
NPP_GetValue(void* inst, NPPVariable variable, void *value)
{
    return NPERR_NO_ERROR;
}

NPError
NPP_SetValue(void* inst, NPNVariable variable, void *value)
{
    return NPERR_NO_ERROR;
}

void* NPP_GetJavaClass(void)
{
    return 0;
}

NPError
NPP_Initialize(void)
{
    //	MSG("NPP_Initialize");
    return NPERR_NO_ERROR;
}

void
NPP_Shutdown(void)
{
    //	MSG("NPP_Shutdown");
}


