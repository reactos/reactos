#ifndef _WIN32K_PROP_H
#define _WIN32K_PROP_H

typedef struct _PROPERTY
{
  LIST_ENTRY PropListEntry;
  HANDLE Data;
  ATOM Atom;
} PROPERTY, *PPROPERTY;

#endif /* _WIN32K_PROP_H */

