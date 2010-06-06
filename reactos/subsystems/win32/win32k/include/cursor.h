
typedef struct _SYSTEM_CURSORINFO
{
  BOOL Enabled;
  RECT ClipRect;
  BOOL IsClipped;
  BOOL ShowingCursor;
  POINT CursorPos;
} SYSTEM_CURSORINFO, *PSYSTEM_CURSORINFO;

typedef struct _GDIPOINTER 
{
  BOOL     Enabled;
  SIZEL    Size;
  POINTL   HotSpot;
  XLATEOBJ *XlateObject;
  SURFACE  *psurfColor;
  SURFACE  *psurfMask;
  SURFACE  *psurfSave;
  RECTL    Exclude; /* required publicly for SPS_ACCEPT_EXCLUDE */
} GDIPOINTER, *PGDIPOINTER;

typedef struct _CURSORICONENTRY
{
    LIST_ENTRY Entry;
    ICONINFO   IconInfo;
    HANDLE     Self;
    HANDLE     hbmMaskUser; // temporary
    HANDLE     hbmColorUser; // temporary
} CURSORICONENTRY, *PCURSORICONENTRY;

extern SYSTEM_CURSORINFO CursorInfo;

PCURSORICONENTRY NTAPI USER_GetCursorIcon(HCURSOR Handle);

