/* $Id: commdlg.h,v 1.1.2.1 2004/10/25 01:25:26 ion Exp $ */

#define SAVE_DIALOG         1
#define OPEN_DIALOG         2	

#if (WINVER >= 0x0500) && !defined (__OBJC__)
#include <objbase.h>
#endif

#include_next <commdlg.h>
