//
// comm.h: Declares data, defines and struct types for common code
//            module.
//
//

#ifndef __COMM_H__
#define __COMM_H__



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

#define CbFromCch(cch)              ((cch)*sizeof(TCHAR))
#define CCH_NUL                     (sizeof(TCHAR))


/////////////////////////////////////////////////////  MACROS

// Zero-initialize data-item
//
#define ZeroInit(pobj, type)        lmemset((CHAR *)pobj, 0, sizeof(type))

// Copy chunk of memory
//
#define BltByte(pdest, psrc, cb)    lmemmove((CHAR *)pdest, (CHAR *)psrc, cb)

// General flag macros
//
#define SetFlag(obj, f)             do {obj |= (f);} while (0)
#define ToggleFlag(obj, f)          do {obj ^= (f);} while (0)
#define ClearFlag(obj, f)           do {obj &= ~(f);} while (0)
#define IsFlagSet(obj, f)           (BOOL)(((obj) & (f)) == (f))  
#define IsFlagClear(obj, f)         (BOOL)(((obj) & (f)) != (f))  

#define InRange(id, idFirst, idLast)  ((UINT)(id-idFirst) <= (UINT)(idLast-idFirst))

//
// Non-shared memory allocation
//

//      void * GAlloc(DWORD cbBytes)
//          Alloc a chunk of memory, quickly, with no 64k limit on size of
//          individual objects or total object size.  Initialize to zero.
//
#define GAlloc(cbBytes)         GlobalAlloc(GPTR, cbBytes)

//      void * GReAlloc(void * pv, DWORD cbNewSize)
//          Realloc one of above.  If pv is NULL, then this function will do
//          an alloc for you.  Initializes new portion to zero.
//
#define GReAlloc(pv, cbNewSize) GlobalReAlloc(pv, cbNewSize, GMEM_MOVEABLE | GMEM_ZEROINIT)

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

// Copies psz into *ppszBuf and (re)allocates *ppszBuf accordingly
BOOL PUBLIC GSetString(LPTSTR * ppszBuf, LPCTSTR psz);

// Concatenates psz onto *ppszBuf and (re)allocates *ppszBuf accordingly
BOOL PUBLIC GCatString(LPTSTR * ppszBuf, LPCTSTR psz);


// FileInfo struct that contains file time/size info
//
typedef struct _FileInfo
    {
    HICON   hicon;
    FILETIME ftMod;
    DWORD   dwSize;         // size of the file
    DWORD   dwAttributes;   // attributes
    LPARAM  lParam;
    LPTSTR   pszDisplayName; // points to the display name
    TCHAR    szPath[1];      
    } FileInfo;

#define FIGetSize(pfi)          ((pfi)->dwSize)
#define FIGetPath(pfi)          ((pfi)->szPath)
#define FIGetDisplayName(pfi)   ((pfi)->pszDisplayName)
#define FIGetAttributes(pfi)    ((pfi)->dwAttributes)
#define FIIsFolder(pfi)         (IsFlagSet((pfi)->dwAttributes, SFGAO_FOLDER))

// Flags for FICreate
#define FIF_DEFAULT     0x0000
#define FIF_ICON        0x0001
#define FIF_DONTTOUCH   0x0002
#define FIF_FOLDER      0x0004

HRESULT PUBLIC FICreate(LPCTSTR pszPath, FileInfo ** ppfi, UINT uFlags);
BOOL    PUBLIC FISetPath(FileInfo ** ppfi, LPCTSTR pszPathNew, UINT uFlags);
BOOL    PUBLIC FIGetInfoString(FileInfo * pfi, LPTSTR pszBuf, int cchBuf);
void    PUBLIC FIFree(FileInfo * pfi);

void    PUBLIC FileTimeToDateTimeString(LPFILETIME pft, LPTSTR pszBuf, int cchBuf);


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

#define DECLAREHOURGLASS    HCURSOR hcurSavHourglass
#define SetHourglass()      hcurSavHourglass = SetCursor(LoadCursor(NULL, IDC_WAIT))
#define ResetHourglass()    SetCursor(hcurSavHourglass)

// UNICODE WARNING: These must stay as CHARS for the math to be right

CHAR *   PUBLIC lmemset(CHAR * dst, CHAR val, UINT count);
CHAR *   PUBLIC lmemmove(CHAR * dst, CHAR * src, int count);

int     PUBLIC AnsiToInt(LPCTSTR pszString);

INT_PTR PUBLIC DoModal (HWND hwndParent, DLGPROC lpfnDlgProc, UINT uID, LPARAM lParam);

VOID PUBLIC SetRectFromExtent(HDC hdc, LPRECT lprc, LPCTSTR lpcsz);

// Flags for MyDrawText()
#define MDT_DRAWTEXT        0x00000001                                  
#define MDT_ELLIPSES        0x00000002                                  
#define MDT_LINK            0x00000004                                  
#define MDT_SELECTED        0x00000008                                  
#define MDT_DESELECTED      0x00000010                                  
#define MDT_DEPRESSED       0x00000020                                  
#define MDT_EXTRAMARGIN     0x00000040                                  
#define MDT_TRANSPARENT     0x00000080
#define MDT_LEFT            0x00000100
#define MDT_RIGHT           0x00000200
#define MDT_CENTER          0x00000400
#define MDT_VCENTER         0x00000800
#define MDT_CLIPPED         0x00001000

void PUBLIC MyDrawText(HDC hdc, LPCTSTR pszText, RECT * prc, UINT flags, int cyChar, int cxEllipses, COLORREF clrText, COLORREF clrTextBk);

DWORD PUBLIC MsgWaitObjectsSendMessage(DWORD cObjects, LPHANDLE phObjects, DWORD dwTimeout);

HCURSOR PUBLIC SetCursorRemoveWigglies(HCURSOR hcur);

LPTSTR PUBLIC _ConstructMessageString(HINSTANCE hinst, LPCTSTR pszMsg, va_list *ArgList);

BOOL PUBLIC ConstructMessage(LPTSTR * ppsz, HINSTANCE hinst, LPCTSTR pszMsg, ...);

#endif // __COMM_H__
