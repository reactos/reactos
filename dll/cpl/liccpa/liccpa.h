#ifndef _LICCPA_H
#define _LICCPA_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <wingdi.h>
#include <wincon.h>
#include <devguid.h>
#include <shlobj.h>
#include <cpl.h>
#include <regstr.h>

#include "resource.h"

typedef struct
{
  int idIcon;
  int idName;
  int idDescription;
  APPLET_PROC AppletProc;
} APPLET, *PAPPLET;

extern HINSTANCE hApplet;

#endif /* _LICCPA_H */

/* EOF */
