#ifndef __MONSLCTL__H
#define __MONSLCTL__H

/* MONSL_MONINFO Flags */
#define MSL_MIF_DISABLED    0x1

typedef struct _MONSL_MONINFO
{
    POINT Position;
    SIZE Size;
    DWORD Flags; /* MSL_MIF_* */
    LPARAM lParam;
} MONSL_MONINFO, *PMONSL_MONINFO;

/*
 * MSLM_SETMONITORSINFO
 *   wParam: DWORD
 *           Count of MONSL_MONINFO structures provided as lParam.
 *   lParam: PMONSL_MONINFO
 *           Array of wParam MONSL_MONINFO structures.
 *
 *   Returns non-zero value if successful.
 */
#define MSLM_SETMONITORSINFO (WM_USER + 0x10)

/*
 * MSLM_GETMONITORSINFO
 *   wParam: DWORD
 *           Length of MONSL_MONINFO array buffer provided in lParam.
 *   lParam: PMONSL_MONINFO
 *           Array of wParam MONSL_MONINFO structures
 *
 *   Returns number of structures copied.
 */
#define MSLM_GETMONITORSINFO (WM_USER + 0x11)

/*
 * MSLM_GETMONITORINFOCOUNT
 *   wParam: Ignored.
 *   lParam: Ignored.
 *
 *   Returns number of monitors.
 */
#define MSLM_GETMONITORINFOCOUNT    (WM_USER + 0x12)

/*
 * MSLM_HITTEST
 *   wParam: PPOINT
 *           Pointer to a POINT structure specifying the coordinates
 *           relative to the client area of the control.
 *   lParam: Ignored.
 *
 *   Returns the index of the monitor at this point, or -1.
 */
#define MSLM_HITTEST    (WM_USER + 0x13)

/*
 * MSLM_SETCURSEL
 *   wParam: INT
 *           Selects the monitor with this index. Pass -1 to clear the selection.
 *   lParam: Ignored.
 *
 *   Returns a non-zero value if successful.
 */
#define MSLM_SETCURSEL  (WM_USER + 0x14)

/*
 * MSLM_GETCURSEL
 *   wParam: Ignored.
 *   lParam: Ignored.
 *
 *   Returns the index of the selected monitor, or -1 if none is currently selected.
 */
#define MSLM_GETCURSEL  (WM_USER + 0x15)

/*
 * MSLM_SETMONITORINFO
 *   wParam: INT
 *           Index of the monitor information that is queried.
 *   lParam: PMONSL_MONINFO
 *           Pointer to a MONSL_MONINFO structures.
 *
 *   Returns non-zero value if successful.
 */
#define MSLM_SETMONITORINFO (WM_USER + 0x16)

/*
 * MSLM_GETMONITORINFO
 *   wParam: INT
 *           Index of the monitor information to be changed.
 *   lParam: PMONSL_MONINFO
 *           Pointer to a MONSL_MONINFO structures.
 *
 *   Returns non-zero value if successful.
 */
#define MSLM_GETMONITORINFO (WM_USER + 0x17)

BOOL RegisterMonitorSelectionControl(IN HINSTANCE hInstance);
VOID UnregisterMonitorSelectionControl(IN HINSTANCE hInstance);

#endif /* __MONSLCTL__H */
