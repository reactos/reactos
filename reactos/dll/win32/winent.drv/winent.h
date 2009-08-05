/* GDI escapes */

#define NTDRV_ESCAPE 6789
enum ntdrv_escape_codes
{
    NTDRV_GET_DISPLAY,      /* get X11 display for a DC */
    NTDRV_GET_DRAWABLE,     /* get current drawable for a DC */
    NTDRV_GET_FONT,         /* get current X font for a DC */
    NTDRV_SET_DRAWABLE,     /* set current drawable for a DC */
    NTDRV_START_EXPOSURES,  /* start graphics exposures */
    NTDRV_END_EXPOSURES,    /* end graphics exposures */
    NTDRV_GET_DCE,          /* no longer used */
    NTDRV_SET_DCE,          /* no longer used */
    NTDRV_GET_GLX_DRAWABLE, /* get current glx drawable for a DC */
    NTDRV_SYNC_PIXMAP,      /* sync the dibsection to its pixmap */
    NTDRV_FLUSH_GL_DRAWABLE /* flush changes made to the gl drawable */
};

struct ntdrv_escape_set_drawable
{
    enum ntdrv_escape_codes  code;         /* escape code (X11DRV_SET_DRAWABLE) */
    //Drawable                 drawable;     /* X drawable */
    int                      mode;         /* ClipByChildren or IncludeInferiors */
    RECT                     dc_rect;      /* DC rectangle relative to drawable */
    RECT                     drawable_rect;/* Drawable rectangle relative to screen */
    //XID                      fbconfig_id;  /* fbconfig id used by the GL drawable */
    //Drawable                 gl_drawable;  /* GL drawable */
    //Pixmap                   pixmap;       /* Pixmap for a GLXPixmap gl_drawable */
    int                      gl_copy;      /* whether the GL contents need explicit copying */
};

/* font.c */
VOID
FeSelectFont(NTDRV_PDEVICE *physDev, HFONT hFont);

BOOL
FeTextOut( NTDRV_PDEVICE *physDev, INT x, INT y, UINT flags,
           const RECT *lprect, LPCWSTR wstr, UINT count,
           const INT *lpDx );

void NTDRV_SendMouseInput( HWND hwnd, DWORD flags, DWORD x, DWORD y,
                              DWORD data, DWORD time, DWORD extra_info, UINT injected_flags );

void NTDRV_SendKeyboardInput( WORD wVk, WORD wScan, DWORD event_flags, DWORD time,
                                 DWORD dwExtraInfo, UINT injected_flags );

BOOL CDECL RosDrv_SetCursorPos( INT x, INT y );

LRESULT HOOK_CallHooks( INT id, INT code, WPARAM wparam, LPARAM lparam, BOOL unicode );

BOOL CDECL RosDrv_GetCursorPos( LPPOINT pt );
