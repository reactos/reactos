#ifndef _WIN32K_PROP_H
#define _WIN32K_PROP_H

BOOL FASTCALL
W32kSetProp(struct _WINDOW_OBJECT* Wnd, ATOM Atom, HANDLE Data);

struct _PROPERTY* FASTCALL
W32kGetProp(struct _WINDOW_OBJECT* WindowObject, ATOM Atom);


#endif /* _WIN32K_PROP_H */

/* EOF */

