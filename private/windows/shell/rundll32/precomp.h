#ifndef STRICT
#define STRICT
#endif

#define _INC_OLE
#include <windows.h>
#undef _INC_OLE

#ifdef WIN32
#include <shell2.h>
#else
#include <shell.h>
#endif

#define IDI_DEFAULT     100

#define IDS_UNKNOWNERROR        0x100

#define IDS_LOADERR             0x300

#define IDS_GETPROCADRERR       0x400
#define IDS_CANTLOADDLL         0x401

//#include "port32.h"
