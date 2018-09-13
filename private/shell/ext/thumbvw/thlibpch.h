/* sample source code for IE4 view extension
 * 
 * Copyright Microsoft 1996
 *
 * This file implements a tear off Drag-Drop interface for Listview windows
 */

#include <windows.h>
#include <ole2.h>

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

// debugging support
#ifdef _DEBUG
#define Assert(x)   if (!x) DebugBreak();
#else
#define Assert(x)
#endif

#include "thumpriv.h"
