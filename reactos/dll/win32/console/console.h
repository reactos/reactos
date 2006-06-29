#ifndef CONSOLE_H__
#define CONSOLE_H__

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include "resource.h"

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

#endif /* CONSOLE_H__ */
