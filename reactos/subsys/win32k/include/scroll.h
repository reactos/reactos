#ifndef _WIN32K_SCROLL_H
#define _WIN32K_SCROLL_H

DWORD FASTCALL IntCreateScrollBar(PWINDOW_OBJECT Window, LONG idObject);
BOOL FASTCALL IntDestroyScrollBar(PWINDOW_OBJECT Window, LONG idObject);

#endif /* _WIN32K_SCROLL_H */
