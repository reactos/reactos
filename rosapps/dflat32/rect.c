/* ------------- rect.c --------------- */

#include "dflat.h"

 /* --- Produce the vector end points produced by the overlap
        of two other vectors --- */
static void subVector(int *v1, int *v2,
                        int t1, int t2, int o1, int o2)
{
    *v1 = *v2 = -1;
    if (DfWithin(o1, t1, t2))    {
        *v1 = o1;
        if (DfWithin(o2, t1, t2))
            *v2 = o2;
        else
            *v2 = t2;
    }
    else if (DfWithin(o2, t1, t2))    {
        *v2 = o2;
        if (DfWithin(o1, t1, t2))
            *v1 = o1;
        else
            *v1 = t1;
    }
    else if (DfWithin(t1, o1, o2))    {
        *v1 = t1;
        if (DfWithin(t2, o1, o2))
            *v2 = t2;
        else
            *v2 = o2;
    }
    else if (DfWithin(t2, o1, o2))    {
        *v2 = t2;
        if (DfWithin(t1, o1, o2))
            *v1 = t1;
        else
            *v1 = o1;
    }
}

 /* --- Return the rectangle produced by the overlap
        of two other rectangles ---- */
DFRECT DfSubRectangle(DFRECT r1, DFRECT r2)
{
    DFRECT r = {0,0,0,0};
    subVector((int *) &DfRectLeft(r), (int *) &DfRectRight(r),
        DfRectLeft(r1), DfRectRight(r1),
        DfRectLeft(r2), DfRectRight(r2));
    subVector((int *) &DfRectTop(r), (int *) &DfRectBottom(r),
        DfRectTop(r1), DfRectBottom(r1),
        DfRectTop(r2), DfRectBottom(r2));
    if (DfRectRight(r) == -1 || DfRectTop(r) == -1)
        DfRectRight(r) =
        DfRectLeft(r) =
        DfRectTop(r) =
        DfRectBottom(r) = 0;
    return r;
}

/* ------- return the client rectangle of a window ------ */
DFRECT DfClientRect(void *wnd)
{
	DFRECT rc;

	if (wnd == NULL)
	{
		DfRectLeft(rc) = 1; // DfGetClientLeft((DFWINDOW)wnd);
		DfRectTop(rc) = 2; // DfGetClientTop((DFWINDOW)wnd);
		DfRectRight(rc) = DfGetScreenWidth () - 2; // DfGetClientRight((DFWINDOW)wnd);
		DfRectBottom(rc) = DfGetScreenHeight () - 2; // DfGetClientBottom((DFWINDOW)wnd);
		return rc;
	}

	DfRectLeft(rc) = DfGetClientLeft((DFWINDOW)wnd);
	DfRectTop(rc) = DfGetClientTop((DFWINDOW)wnd);
	DfRectRight(rc) = DfGetClientRight((DFWINDOW)wnd);
	DfRectBottom(rc) = DfGetClientBottom((DFWINDOW)wnd);

	return rc;
}

/* ----- return the rectangle relative to
            its window's screen position -------- */
DFRECT DfRelativeWindowRect(void *wnd, DFRECT rc)
{
    DfRectLeft(rc) -= DfGetLeft((DFWINDOW)wnd);
    DfRectRight(rc) -= DfGetLeft((DFWINDOW)wnd);
    DfRectTop(rc) -= DfGetTop((DFWINDOW)wnd);
    DfRectBottom(rc) -= DfGetTop((DFWINDOW)wnd);
    return rc;
}

/* ----- clip a rectangle to the parents of the window ----- */
DFRECT DfClipRectangle(void *wnd, DFRECT rc)
{
    DFRECT sr;
    DfRectLeft(sr) = DfRectTop(sr) = 0;
    DfRectRight(sr) = DfGetScreenWidth()-1;
    DfRectBottom(sr) = DfGetScreenHeight()-1;
    if (!DfTestAttribute((DFWINDOW)wnd, DF_NOCLIP))
        while ((wnd = DfGetParent((DFWINDOW)wnd)) != NULL)
            rc = DfSubRectangle(rc, DfClientRect(wnd));
    return DfSubRectangle(rc, sr);
}

/* EOF */
