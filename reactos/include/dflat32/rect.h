/* ----------- rect.h ------------ */
#ifndef RECT_H
#define RECT_H

typedef struct    {
    int lf,tp,rt,bt;
} DFRECT;
#define within(p,v1,v2)   ((p)>=(v1)&&(p)<=(v2))
#define RectTop(r)        (r.tp)
#define RectBottom(r)     (r.bt)
#define RectLeft(r)       (r.lf)
#define RectRight(r)      (r.rt)
#define InsideRect(x,y,r) (within((x),RectLeft(r),RectRight(r))\
                               &&                              \
                          within((y),RectTop(r),RectBottom(r)))
#define ValidRect(r)      (RectRight(r) || RectLeft(r) || \
                           RectTop(r) || RectBottom(r))
#define RectWidth(r)      (RectRight(r)-RectLeft(r)+1)
#define RectHeight(r)     (RectBottom(r)-RectTop(r)+1)
DFRECT subRectangle(DFRECT, DFRECT);
DFRECT ClientRect(void *);
DFRECT RelativeWindowRect(void *, DFRECT);
DFRECT ClipRectangle(void *, DFRECT);
#endif
