// Drag the selected items in a ListView.  Other than hwndLV (the ListView
// window), the parameters are the same as those for ShellDragObjects.
//
void    LVUtil_ScreenToLV(HWND hwndLV, LPPOINT ppt);
void    LVUtil_ClientToLV(HWND hwndLV, LPPOINT ppt);
void    LVUtil_LVToClient(HWND hwndLV, LPPOINT ppt);
void    LVUtil_DragSelectItem(HWND hwndLV, int nItem);
void    LVUtil_MoveSelectedItems(HWND hwndLV, int dx, int dy, BOOL fAll);
LPARAM LVUtil_GetLParam(HWND hwndLV, int i);

BOOL DAD_IsDraggingImage(void);

#define DCID_INVALID    -1
#define DCID_NULL       0
#define DCID_NO         1
#define DCID_MOVE       2
#define DCID_COPY       3
#define DCID_LINK       4
#define DCID_MAX        5
void DAD_SetDragCursor(int idCursor);
STDAPI_(BOOL) DAD_IsDragging();
STDAPI_(BOOL) DAD_SetDragImageFromWindow(HWND hwnd, POINT* ppt, IDataObject* pDataObject);
