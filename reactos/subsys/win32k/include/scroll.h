#ifndef _WIN32K_SCROLL_H
#define _WIN32K_SCROLL_H

typedef struct _WINDOW_SCROLLINFO
{
  SCROLLBARINFO ScrollBarInfo;
  SCROLLINFO ScrollInfo;
} WINDOW_SCROLLINFO, *PWINDOW_SCROLLINFO;

#define IntGetScrollbarInfoFromWindow(Window, i) \
  ((PSCROLLBARINFO)(&((Window)->Scroll + i)->ScrollBarInfo))

#define IntGetScrollInfoFromWindow(Window, i) \
  ((LPSCROLLINFO)(&((Window)->Scroll + i)->ScrollInfo))

#define SBOBJ_TO_SBID(Obj)	((Obj) - OBJID_HSCROLL)
#define SBID_IS_VAILD(id)	(id == SB_HORZ || id == SB_VERT)

BOOL FASTCALL IntCreateScrollBars(PWINDOW_OBJECT Window);
BOOL FASTCALL IntDestroyScrollBars(PWINDOW_OBJECT Window);

#endif /* _WIN32K_SCROLL_H */
