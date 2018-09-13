//
// utils.h: Declares data, defines and struct types for common code
//            module.
//
//

#ifndef __UTILS_H__
#define __UTILS_H__



/////////////////////////////////////////////////////  DEFINES

#define BLOCK        
#define Unref(x)     x

#ifdef DEBUG
#define INLINE
#define DEBUG_CODE(x)   x
#else
#define INLINE          __inline
#define DEBUG_CODE(x)   
#endif


/////////////////////////////////////////////////////  MACROS

// Zero-initialize data-item
//
#define ZeroInit(pobj, type)        lmemset((LPSTR)pobj, 0, sizeof(type))

// Copy chunk of memory
//
#define BltByte(pdest, psrc, cb)    lmemmove((LPSTR)pdest, (LPSTR)psrc, cb)

// General flag macros
//
#define SetFlag(obj, f)             do {obj |= (f);} while (0)
#define ToggleFlag(obj, f)          do {obj ^= (f);} while (0)
#define ClearFlag(obj, f)           do {obj &= ~(f);} while (0)
#define IsFlagSet(obj, f)           (BOOL)(((obj) & (f)) == (f))  
#define IsFlagClear(obj, f)         (BOOL)(((obj) & (f)) != (f))  

//      void * GAlloc(DWORD cbBytes)
//          Alloc a chunk of memory, quickly, with no 64k limit on size of
//          individual objects or total object size.  Initialize to zero.
//
#define GAlloc(cbBytes)         GlobalAlloc(GPTR, cbBytes)

//      void * GReAlloc(void * pv, DWORD cbNewSize)
//          Realloc one of above.  If pv is NULL, then this function will do
//          an alloc for you.  Initializes new portion to zero.
//
#define GReAlloc(pv, cbNewSize) GlobalReAlloc(pv, GMEM_MOVEABLE | GMEM_ZEROINIT, cbNewSize)

//      void GFree(void *pv)
//          Free pv if it is nonzero.  Set pv to zero.  
//
#define GFree(pv)        do { (pv) ? GlobalFree(pv) : (void)0;  pv = NULL; } while (0)

//      DWORD GGetSize(void *pv)
//          Get the size of a block allocated by Alloc()
//
#define GGetSize(pv)            GlobalSize(pv)

//      type * GAllocType(type);                    (macro)
//          Alloc some memory the size of <type> and return pointer to <type>.
//
#define GAllocType(type)                (type *)GAlloc(sizeof(type))

//      type * GAllocArray(type, int cNum);         (macro)
//          Alloc an array of data the size of <type>.
//
#define GAllocArray(type, cNum)          (type *)GAlloc(sizeof(type) * (cNum))

//      type * GReAllocArray(type, void * pb, int cNum);
//
#define GReAllocArray(type, pb, cNum)    (type *)GReAlloc(pb, sizeof(type) * (cNum))

//      void Free(void _huge * pb);                      (macro)
//          Free pb if it is nonzero.  Set pb to zero.  (Overrides Free above.)
//
#define Free(pb)        do { (pb) ? Free(pb) : (void)0;  pb = NULL; } while (0)


// Color macros
//
#define ColorText(nState)   (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT)
#define ColorBk(nState)     (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW)
#define ColorMenuText(nState)   (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT)
#define ColorMenuBk(nState)     (((nState) & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_MENU)
#define GetImageDrawStyle(nState)   (((nState) & ODS_SELECTED) ? ILD_SELECTED : ILD_NORMAL)

// Sets the dialog handle in the given data struct on first
//  message that the dialog gets (WM_SETFONT).
//
#define SetDlgHandle(hwnd, msg, lp)     if((msg)==WM_SETFONT) (lp)->hdlg=(hwnd);


/////////////////////////////////////////////////////  TYPEDEFS

/////////////////////////////////////////////////////  EXPORTED DATA

/////////////////////////////////////////////////////  PUBLIC PROTOTYPES

int     NEAR PASCAL RuiUserMessage (HWND, UINT, UINT);
void    NEAR PASCAL ShortenName (LPSTR, LPSTR, DWORD);
BOOL    NEAR PASCAL IsServerInstalled ();

LPSTR   PUBLIC lmemset(LPSTR dst, char val, UINT count);
LPSTR   PUBLIC lmemmove(LPSTR dst, LPSTR src, int count);
int     PUBLIC AnsiToInt(LPCSTR pszString);

int     PUBLIC DoModal (HWND hwndParent, DLGPROC lpfnDlgProc, UINT uID, LPARAM lParam);

HMENU   PUBLIC LoadPopupMenu(UINT id, UINT uSubOffset);
UINT    PUBLIC MergePopupMenu(HMENU FAR *phMenu, UINT idResource, UINT uSubOffset, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast);
HMENU   PUBLIC GetMenuFromID(HMENU hmMain, UINT uID);

BOOL    PUBLIC RunDLLProcess (LPSTR pszCmdLine);
BOOL    PUBLIC RunDLLThread(HWND hwnd, LPCSTR pszCmdLine, int nCmdShow);


#endif // __UTILS_H__
