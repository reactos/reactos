#ifndef _WIN32K_PROP_H
#define _WIN32K_PROP_H

typedef struct _PROPERTY
{
  LIST_ENTRY PropListEntry;
  HANDLE Data;
  ATOM Atom;
} PROPERTY, *PPROPERTY;

#define IntLockWindowProperties(Window) \
  ExAcquireFastMutex(&Window->PropListLock)

#define IntUnLockWindowProperties(Window) \
  ExReleaseFastMutex(&Window->PropListLock)

#endif /* _WIN32K_PROP_H */

