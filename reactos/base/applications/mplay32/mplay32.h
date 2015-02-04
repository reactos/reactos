#include <stdarg.h>
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winreg.h>
#include <commdlg.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <digitalv.h>
#include <commctrl.h>
#include <tchar.h>
#include <strsafe.h>

#include "resource.h"

#define IDT_PLAYTIMER 1000

#define UNSUPPORTED_FILE 0
#define WAVE_FILE        1
#define MIDI_FILE        2
#define AUDIOCD_FILE     3
#define AVI_FILE         4

typedef struct
{
    TCHAR szExt[MAX_PATH];
    UINT uType;
} TYPEBYEXT;
