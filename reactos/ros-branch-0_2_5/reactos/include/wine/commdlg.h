/* $Id: commdlg.h,v 1.2 2004/01/12 23:44:24 sedwards Exp $ */

#define SAVE_DIALOG         1
#define OPEN_DIALOG         2	

#if (WINVER >= 0x0500) && !defined (__OBJC__)
#include <objbase.h>
#endif

#include_next <commdlg.h>
