//---------------------------------------------------------------------------
// Helper routines for an owner draw menu showing the contents of a directory.
//---------------------------------------------------------------------------
#include "shellprv.h"
#include <fsmenu.h>
#include "ids.h"

#define DF_HOOK         0x80000000

// Cursor IDs from shdocvw

#define IDC_MENUMOVE            100
#define IDC_MENUCOPY            101
#define IDC_MENUDENY            102

#define IEPlaySound(szEvent, fSysCommand)   SHPlaySound(szEvent)

#define g_himlIcons         g_himlFSIcons
#define g_himlIconsSmall    g_himlFSIconsSmall

#include "..\lib\fsmenu.cpp"
