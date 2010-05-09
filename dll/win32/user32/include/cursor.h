HCURSOR
CursorIconToCursor(HICON hIcon,
                   BOOL SemiTransparent);

HICON CreateCursorIconFromData(PVOID ImageData,
                               ICONIMAGE* IconImage,
                               int cxDesired,
                               int cyDesired,
                               int xHotspot,
                               int yHotspot,
                               BOOL fIcon);

/*
 *  The following macro function accounts for the irregularities of
 *   accessing cursor and icon resources in files and resource entries.
 */
typedef BOOL
(*fnGetCIEntry)(LPVOID dir, int n, int *width, int *height, int *bits );

int
CURSORICON_FindBestCursor(LPVOID dir,
                          fnGetCIEntry get_entry,
                          int Width,
                          int Height,
                          int ColorBits);
int
CURSORICON_FindBestIcon(LPVOID dir,
                        fnGetCIEntry get_entry,
                        int Width,
                        int Height,
                        int ColorBits);

