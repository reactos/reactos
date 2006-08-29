//
// EnumDirs.h
//

#ifndef __ENUMDIRS_H
#define __ENUMDIRS_H

//#include "win.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef BOOL (*MYENUMDIRSPROC) ( PWIN32_FIND_DATA, long );
BOOL EnumDirs ( const TCHAR* szDirectory, const TCHAR* szFileSpec, MYENUMDIRSPROC pProc, long lParam );

#endif//__ENUMDIRS_H
