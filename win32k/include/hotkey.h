#ifndef _WIN32K_HOTKEY_H
#define _WIN32K_HOTKEY_H

#include <include/winsta.h>
#include <include/window.h>

typedef struct _HOT_KEY_ITEM
{
  LIST_ENTRY ListEntry;
  struct _ETHREAD *Thread;
  HWND hWnd;
  int id;
  UINT fsModifiers;
  UINT vk;
} HOT_KEY_ITEM, *PHOT_KEY_ITEM;


#endif /* _WIN32K_HOTKEY_H */

/* EOF */
