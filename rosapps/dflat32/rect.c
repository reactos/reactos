/* ------------- rect.c --------------- */

#include "dflat.h"

 /* --- Produce the vector end points produced by the overlap
        of two other vectors --- */
static void subVector(int *v1, int *v2,
                        int t1, int t2, int o1, int o2)
{
    *v1 = *v2 = -1;
    if (within(o1, t1, t2))    {
        *v1 = o1;
        if (within(o2, t1, t2))
            *v2 = o2;
        else
            *v2 = t2;
    }
    else if (within(o2, t1, t2))    {
        *v2 = o2;
        if (within(o1, t1, t2))
            *v1 = o1;
        else
            *v1 = t1;
    }
    else if (within(t1, o1, o2))    {
        *v1 = t1;
        if (within(t2, o1, o2))
            *v2 = t2;
        else
            *v2 = o2;
    }
    else if (within(t2, o1, o2))    {
        *v2 = t2;
        if (within(t1, o1, o2))
            *v1 = t1;
        else
            *v1 = o1;
    }
}

 /* --- Return the rectangle produced by the overlap
        of two other rectangles ---- */
DFRECT subRectangle(DFRECT r1, DFRECT r2)
{
    DFRECT r = {0,0,0,0};
    subVector((int *) &RectLeft(r), (int *) &RectRight(r),
        RectLeft(r1), RectRight(r1),
        RectLeft(r2), RectRight(r2));
    subVector((int *) &RectTop(r), (int *) &RectBottom(r),
        RectTop(r1), RectBottom(r1),
        RectTop(r2), RectBottom(r2));
    if (RectRight(r) == -1 || RectTop(r) == -1)
        RectRight(r) =
        RectLeft(r) =
        RectTop(r) =
        RectBottom(r) = 0;
    return r;
}

/* ------- return the client rectangle of a window ------ */
DFRECT ClientRect(void *wnd)
{
	DFRECT rc;

	if (wnd == NULL)
	{
		RectLeft(rc) = 1; // GetClientLeft((DFWINDOW)wnd);
		RectTop(rc) = 2; // GetClientTop((DFWINDOW)wnd);
		RectRight(rc) = DfGetScreenWidth () - 2; // GetClientRight((DFWINDOW)wnd);
		RectBottom(rc) = DfGetScreenHeight () - 2; // GetClientBottom((DFWINDOW)wnd);
		return rc;
	}

	RectLeft(rc) = GetClientLeft((DFWINDOW)wnd);
	RectTop(rc) = GetClientTop((DFWINDOW)wnd);
	RectRight(rc) = GetClientRight((DFWINDOW)wnd);
	RectBottom(rc) = GetClientBottom((DFWINDOW)wnd);

	return rc;
}

/* ----- return the rectangle relative to
            its window's screen position -------- */
DFRECT RelativeWindowRect(void *wnd, DFRECT rc)
{
    RectLeft(rc) -= GetLeft((DFWINDOW)wnd);
    RectRight(rc) -= GetLeft((DFWINDOW)wnd);
    RectTop(rc) -= GetTop((DFWINDOW)wnd);
    RectBottom(rc) -= GetTop((DFWINDOW)wnd);
    return rc;
}

/* ----- clip a rectangle to the parents of the window ----- */
DFRECT ClipRectangle(void *wnd, DFRECT rc)
{
    DFRECT sr;
    RectLeft(sr) = RectTop(sr) = 0;
    RectRight(sr) = DfGetScreenWidth()-1;
    RectBottom(sr) = DfGetScreenHeight()-1;
    if (!TestAttribute((DFWINDOW)wnd, NOCLIP))
        while ((wnd = GetParent((DFWINDOW)wnd)) != NULL)
            rc = subRectangle(rc, ClientRect(wnd));
    return subRectangle(rc, sr);
}

/* EOF */
