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
RECT subRectangle(RECT r1, RECT r2)
{
    RECT r = {0,0,0,0};
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
RECT ClientRect(void *wnd)
{
    RECT rc;

    RectLeft(rc) = GetClientLeft((WINDOW)wnd);
    RectTop(rc) = GetClientTop((WINDOW)wnd);
    RectRight(rc) = GetClientRight((WINDOW)wnd);
    RectBottom(rc) = GetClientBottom((WINDOW)wnd);
    return rc;
}

/* ----- return the rectangle relative to
            its window's screen position -------- */
RECT RelativeWindowRect(void *wnd, RECT rc)
{
    RectLeft(rc) -= GetLeft((WINDOW)wnd);
    RectRight(rc) -= GetLeft((WINDOW)wnd);
    RectTop(rc) -= GetTop((WINDOW)wnd);
    RectBottom(rc) -= GetTop((WINDOW)wnd);
    return rc;
}

/* ----- clip a rectangle to the parents of the window ----- */
RECT ClipRectangle(void *wnd, RECT rc)
{
    RECT sr;
    RectLeft(sr) = RectTop(sr) = 0;
    RectRight(sr) = SCREENWIDTH-1;
    RectBottom(sr) = SCREENHEIGHT-1;
    if (!TestAttribute((WINDOW)wnd, NOCLIP))
        while ((wnd = GetParent((WINDOW)wnd)) != NULL)
            rc = subRectangle(rc, ClientRect(wnd));
    return subRectangle(rc, sr);
}
