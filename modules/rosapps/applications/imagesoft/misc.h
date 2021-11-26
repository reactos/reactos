
INT AllocAndLoadString(OUT LPTSTR *lpTarget,
                       IN HINSTANCE hInst,
                       IN UINT uID);

DWORD LoadAndFormatString(IN HINSTANCE hInstance,
                          IN UINT uID,
                          OUT LPTSTR *lpTarget,
                          ...);

BOOL StatusBarLoadAndFormatString(IN HWND hStatusBar,
                                  IN INT PartId,
                                  IN HINSTANCE hInstance,
                                  IN UINT uID,
                                  ...);

BOOL StatusBarLoadString(IN HWND hStatusBar,
                         IN INT PartId,
                         IN HINSTANCE hInstance,
                         IN UINT uID);

INT GetTextFromEdit(OUT LPTSTR lpString,
                    IN HWND hDlg,
                    IN UINT Res);

VOID GetError(DWORD err);

BOOL ToolbarDeleteControlSpace(HWND hWndToolbar,
                               const TBBUTTON *ptbButton);

typedef VOID (*ToolbarChangeControlCallback)(HWND hWndToolbar,
                                             HWND hWndControl,
                                             BOOL Vert);
VOID ToolbarUpdateControlSpaces(HWND hWndToolbar,
                                ToolbarChangeControlCallback ChangeCallback);

BOOL ToolbarInsertSpaceForControl(HWND hWndToolbar,
                                  HWND hWndControl,
                                  INT Index,
                                  INT iCmd,
                                  BOOL HideVertical);

HIMAGELIST InitImageList(UINT StartResource,
                         UINT NumImages);
