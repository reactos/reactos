/* $Id$ */

#define SAVE_DIALOG         1
#define OPEN_DIALOG         2	

#if (WINVER >= 0x0500) && !defined (__OBJC__)
#include <objbase.h>
#endif

#if !defined (_MSC_VER)
#include_next <commdlg.h>
#endif
