//
// dobj.h: Declares data, defines and struct types for RecAct
//          module.
//
//

#ifndef __DOBJ_H__
#define __DOBJ_H__

// DOBJ is the draw object structure for drawing listbox entries
//
// DOBJ kinds
//
#define DOK_ICON        1   // lpvObject is the HICON
#define DOK_STRING      2   // lpvObject is the LPCSTR
#define DOK_BITMAP      3   // lpvObject is the HBITMAP
#define DOK_SIDEITEM    4   // lpvObject points to LPSIDEITEM
#define DOK_IMAGE       5   // 
#define DOK_IDS         6   // lpvObject is the resource ID

// DOBJ item styles
//
#define DOF_LEFT        0x0000
#define DOF_CENTER      0x0001
#define DOF_RIGHT       0x0002
#define DOF_DIFFER      0x0004  // This item's appearance is affected by uState
#define DOF_MENU        0x0008  // Use menu colors
#define DOF_DISABLED    0x0010
#define DOF_IGNORESEL   0x0020  // Ignore selection state
#define DOF_USEIDS      0x0040  // lpvObject is a resource string ID
#define DOF_NODRAW      0x1000  // Don't draw 

typedef struct tagDOBJ
    {
    UINT    uKind;          // One of DOK_* ordinals
    UINT    uFlags;         // One of DOF_* styles
    LPVOID  lpvObject;      // ptr or handle
    HIMAGELIST himl;        // 
    int     iImage;         // 
    int     x;
    int     y;
    RECT    rcBounding;     // Bounding rect of entire object
    union 
        {
        RECT rcSrc;         // DOK_BITMAP: source rect to blt from
        RECT rcClip;        // Clipping rect
        RECT rcLabel;       // Clipping rect for label
        };

    } DOBJ,  * LPDOBJ;


void PUBLIC Dobj_Draw(HDC hdc, LPDOBJ pdobj, int cItems, UINT uState, int cxEllipses, int cyText, COLORREF clrBkgnd);

void PUBLIC ComputeImageRects(LPCTSTR psz, HDC hdc, LPPOINT ppt, LPRECT prcIcon, LPRECT prcLabel, int cxIcon, int cyIcon, int cxIconSpacing, int cyText);

#endif // __DOBJ_H__

