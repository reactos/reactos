#ifndef _WIN32K_PROP_H
#define _WIN32K_PROP_H

typedef struct _PROPERTY
{
  LIST_ENTRY PropListEntry;
  HANDLE Data;
  ATOM Atom;
} PROPERTY, *PPROPERTY;

BOOL FASTCALL
IntSetProp(PWINDOW_OBJECT Wnd, ATOM Atom, HANDLE Data);

PPROPERTY FASTCALL
IntGetProp(PWINDOW_OBJECT WindowObject, ATOM Atom);

#endif /* _WIN32K_PROP_H */

