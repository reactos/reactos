
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
  SURFOBJ  *psurfColor;
  SURFOBJ  *psurfMask;
  SURFOBJ  *psurfSave;
  RECTL    Exclude; /* required publicly for SPS_ACCEPT_EXCLUDE */
} GDIPOINTER, *PGDIPOINTER;
