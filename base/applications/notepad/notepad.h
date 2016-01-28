#ifndef _NOTEPAD_H
#define _NOTEPAD_H

#ifndef STRSAFE_NO_DEPRECATE
#define STRSAFE_NO_DEPRECATE
#endif

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winnls.h>
#include <wingdi.h>
#include <shellapi.h>
#include <commdlg.h>
#include <tchar.h>
#include <malloc.h>

#include "main.h"
#include "dialog.h"

void UpdateWindowCaption(BOOL clearModifyAlert);

#endif /* _NOTEPAD_H */
