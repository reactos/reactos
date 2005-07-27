#ifndef __WIN32K_PATH_H
#define __WIN32K_PATH_H

typedef enum tagGdiPathState
{
   PATH_Null,
   PATH_Open,
   PATH_Closed
} GdiPathState;

typedef struct tagGdiPath
{
   GdiPathState state;
   POINT      *pPoints;
   BYTE         *pFlags;
   int          numEntriesUsed, numEntriesAllocated;
   BOOL       newStroke;
} GdiPath;

#define PATH_IsPathOpen(path) ((path).state==PATH_Open)

BOOL STDCALL NtGdiAbortPath(HDC  hDC);

BOOL STDCALL NtGdiBeginPath(HDC  hDC);

BOOL STDCALL NtGdiCloseFigure(HDC  hDC);

BOOL STDCALL NtGdiEndPath(HDC  hDC);

BOOL STDCALL NtGdiFillPath(HDC  hDC);

BOOL STDCALL NtGdiFlattenPath(HDC  hDC);

BOOL STDCALL NtGdiGetMiterLimit(HDC  hDC,
                        PFLOAT  Limit);

INT STDCALL NtGdiGetPath(HDC  hDC,
                 LPPOINT  Points,
                 LPBYTE  Types,
                 INT  nSize);

HRGN STDCALL NtGdiPathToRegion(HDC  hDC);

BOOL STDCALL NtGdiSetMiterLimit(HDC  hDC,
                        FLOAT  NewLimit,
                        PFLOAT  OldLimit);

BOOL STDCALL NtGdiStrokeAndFillPath(HDC  hDC);

BOOL STDCALL NtGdiStrokePath(HDC  hDC);

BOOL STDCALL NtGdiWidenPath(HDC  hDC);

#endif
