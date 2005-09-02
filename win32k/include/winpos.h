#ifndef _WIN32K_WINPOS_H
#define _WIN32K_WINPOS_H

/* Undocumented flags. */
#define SWP_NOCLIENTMOVE          0x0800
#define SWP_NOCLIENTSIZE          0x1000

#define IntPtInWindow(WndObject,x,y) \
  ((x) >= (WndObject)->WindowRect.left && \
   (x) < (WndObject)->WindowRect.right && \
   (y) >= (WndObject)->WindowRect.top && \
   (y) < (WndObject)->WindowRect.bottom && \
   (!(WndObject)->WindowRegion || ((WndObject)->Style & WS_MINIMIZE) || \
    NtGdiPtInRegion((WndObject)->WindowRegion, (INT)((x) - (WndObject)->WindowRect.left), \
                    (INT)((y) - (WndObject)->WindowRect.top))))


#endif /* _WIN32K_WINPOS_H */
