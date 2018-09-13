//---------------------------------------------------------------------------
// Helper routines for an owner draw menu showing the contents of a directory.
//---------------------------------------------------------------------------
#include "priv.h"
#include <fsmenu.h>
#include "resource.h"

// Macros, manifest constants, and functions needed to get fsmenu.c to
// compile in shdoc401.

#define _WIN32_WINDOWS      0x0400
#define WINVER              0x0400
#include "shlobj.h"
#undef WINSHELLAPI
#define WINSHELLAPI
#include "fsmenu.h"
#include "vdate.h"
#define _FSMENU_H
#include "shellp.h"
#include "shsemip.h"

#define DF_HOOK         0x80000000

#if 0
#define TF_MENU         TF_CUSTOM1
#else
#define TF_MENU         0
#endif

#include "..\lib\fsmenu.cpp"
