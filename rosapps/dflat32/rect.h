/* ----------- rect.h ------------ */
#ifndef RECT_H
#define RECT_H

typedef struct    {
    int lf,tp,rt,bt;
} DFRECT;
#define DfWithin(p,v1,v2)   ((p)>=(v1)&&(p)<=(v2))
#define DfRectTop(r)        (r.tp)
#define DfRectBottom(r)     (r.bt)
#define DfRectLeft(r)       (r.lf)
#define DfRectRight(r)      (r.rt)
#define DfInsideRect(x,y,r) (DfWithin((x),DfRectLeft(r),DfRectRight(r))\
                               &&                              \
                          DfWithin((y),DfRectTop(r),DfRectBottom(r)))
#define DfValidRect(r)      (DfRectRight(r) || DfRectLeft(r) || \
                           DfRectTop(r) || DfRectBottom(r))
#define DfRectWidth(r)      (DfRectRight(r)-DfRectLeft(r)+1)
#define DfRectHeight(r)     (DfRectBottom(r)-DfRectTop(r)+1)
DFRECT DfSubRectangle(DFRECT, DFRECT);
DFRECT DfClientRect(void *);
DFRECT DfRelativeWindowRect(void *, DFRECT);
DFRECT DfClipRectangle(void *, DFRECT);
#endif
