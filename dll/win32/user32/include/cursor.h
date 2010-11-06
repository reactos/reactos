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

