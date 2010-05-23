
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

typedef struct _CURSORICONENTRY
{
    HANDLE     hbmMask;
    HANDLE     hbmColor;
    HANDLE     hUser;
    LIST_ENTRY Entry;
} CURSORICONENTRY, *PCURSORICONENTRY;

extern SYSTEM_CURSORINFO CursorInfo;

VOID NTAPI USER_InitCursorIcons();
VOID USER_LockCursorIcons();
VOID USER_UnlockCursorIcons();
PCURSORICONENTRY NTAPI USER_GetCursorIcon(HCURSOR Handle);

