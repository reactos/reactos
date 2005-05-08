/* $Id: commdlg.h 12852 2005-01-06 13:58:04Z mf $ */

#define SAVE_DIALOG         1
#define OPEN_DIALOG         2	

#if (WINVER >= 0x0500) && !defined (__OBJC__)
#include <objbase.h>
#endif

#include_next <commdlg.h>
