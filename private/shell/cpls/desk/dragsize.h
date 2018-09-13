#define SETWINDOWPOINTER(hwnd, name, p) SetWindowLong(hwnd, 0, (LONG)p)
#define GETWINDOWPOINTER(hwnd, name)	((name)GetWindowLong(hwnd, 0))

#define ALLOCWINDOWPOINTER(name, size)	((name)LocalAlloc(LPTR, size))
#define FREEWINDOWPOINTER(p)		LocalFree((HLOCAL)p)

#define GETWINDOWID(hwnd)               GetWindowLong(hwnd, GWL_ID)

#define DRAGSIZECLASSNAME               TEXT("DragSizeWindow")

HWND WINAPI CreateDragSizeWindow(LONG style, int x, int y, int wid, int hgt, HWND hwndParent, LONG wID);

BOOL FAR PASCAL InitDragSizeClass(void);

#define DSM_DRAGPOS	(WM_USER)

#define DSN_BEGINDRAG	(0)
#define DSN_DRAGGING	(1)
#define DSN_ENDDRAG	(2)
#define DSN_NCCREATE	(3)
