/* $Id: commdlg.h,v 1.1.2.1 2004/10/24 23:10:55 ion Exp $ */

#define SAVE_DIALOG         1
#define OPEN_DIALOG         2	

#if (WINVER >= 0x0500) && !defined (__OBJC__)
#include <objbase.h>
#endif

#include_next <commdlg.h>
