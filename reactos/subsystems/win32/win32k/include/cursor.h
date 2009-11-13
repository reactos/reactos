
typedef struct _SYSTEM_CURSORINFO
{
  BOOL Enabled;
  RECT ClipRect;
  BOOL IsClipped;
  ICONINFO CurrentCursorObject;
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

extern SYSTEM_CURSORINFO CursorInfo;
