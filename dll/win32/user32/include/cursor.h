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


BOOL get_icon_size(HICON hIcon, SIZE *size);

