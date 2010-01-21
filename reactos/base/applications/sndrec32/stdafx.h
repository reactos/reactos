#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN


#ifdef _UNICODE
	#define _sntprintf_s    _snwprintf_s
#else
	#define _sntprintf_s    _snprintf_s
#endif


#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h>

 
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
