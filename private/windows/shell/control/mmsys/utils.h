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
#define ZeroInit(pobj, type)        lmemset((LPTSTR)pobj, 0, sizeof(type))

// Copy chunk of memory
//
#define BltByte(pdest, psrc, cb)    lmemmove((LPTSTR)pdest, (LPTSTR)psrc, cb)

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



#endif // __UTILS_H__
#ifndef __STRING_H__
#define __STRING_H__


/////////////////////////////////////////////////////  INCLUDES

/////////////////////////////////////////////////////  MACROS

#define Bltbyte(rgbSrc,rgbDest,cb)  _fmemmove(rgbDest, rgbSrc, cb)

// Model independent, language-independent (DBCS aware) macros
//  taken from rcsys.h in Pen project and modified.
//
#define IsSzEqual(sz1, sz2)         (BOOL)(lstrcmpi(sz1, sz2) == 0)
#define IsCaseSzEqual(sz1, sz2)     (BOOL)(lstrcmp(sz1, sz2) == 0)
#define SzFromInt(sz, n)            (wsprintf((LPTSTR)sz, (LPTSTR)TEXT("%d"), n), (LPTSTR)sz)

#define IsLink(sz, szLnk)			(!lstrcmpi((LPTSTR)(sz+lstrlen(sz)-4), szLnk))


/////////////////////////////////////////////////////  PROTOTYPES

LPTSTR PUBLIC SzStrTok(LPTSTR string, LPCTSTR control);
LPCTSTR PUBLIC SzStrCh(LPCTSTR string, char ch);

LPTSTR PUBLIC SzFromIDS (UINT ids, LPTSTR pszBuf, int cbBuf);

/////////////////////////////////////////////////////  MORE INCLUDES

#endif // __STRING_H__


typedef struct _PROC_INFO
{
    LPCSTR  Name;
    FARPROC Address;
}
PROC_INFO, *PPROC_INFO;

#define PROCS_LOADED( pProcInfo ) ( (pProcInfo)[0].Address != NULL )
#define LOAD_IF_NEEDED( Library, ProcInfo ) ( PROCS_LOADED( ProcInfo ) ||   \
                                    LoadLibraryAndProcs( Library, ProcInfo ) )


extern PROC_INFO ACMProcs[];
extern PROC_INFO VFWProcs[];
extern PROC_INFO AVIProcs[];
extern PROC_INFO VERSIONProcs[];

BOOL LoadACM();
BOOL FreeACM();
BOOL LoadAVI();
BOOL FreeAVI();
BOOL LoadVFW();
BOOL FreeVFW();
BOOL LoadVERSION();
BOOL FreeVERSION();

//#define DEBUG_BUILT_LINKED
#ifndef DEBUG_BUILT_LINKED

#define acmFormatDetailsW            	(*ACMProcs[0].Address)
#define acmFormatTagDetailsW         	(*ACMProcs[1].Address)
#define acmDriverDetailsW            	(*ACMProcs[2].Address)
#define acmDriverMessage            	(*ACMProcs[3].Address)
#define acmDriverAddW            		(*ACMProcs[4].Address)
#define acmDriverEnum            		(*ACMProcs[5].Address)
#define acmDriverPriority            	(*ACMProcs[6].Address)
#define acmDriverRemove            		(*ACMProcs[7].Address)
#define acmMetrics            			(*ACMProcs[8].Address)
#define acmFormatChooseW            	(*ACMProcs[9].Address)

#define ICClose			            	(*VFWProcs[0].Address)
#define ICGetInfo		            	(*VFWProcs[1].Address)
#define ICLocate		            	(*VFWProcs[2].Address)
#define MCIWndCreateW	            	(*VFWProcs[3].Address)

#define AVIFileRelease 	            	(*AVIProcs[0].Address)
#define AVIStreamRelease	           	(*AVIProcs[1].Address)
#define AVIStreamSampleToTime			(*AVIProcs[2].Address)
#define AVIStreamStart					(*AVIProcs[3].Address)
#define AVIStreamLength					(*AVIProcs[4].Address)
#define AVIStreamReadFormat				(*AVIProcs[5].Address)
#define AVIStreamInfoW					(*AVIProcs[6].Address)
#define AVIFileGetStream				(*AVIProcs[7].Address)
#define AVIFileOpenW   	            	(*AVIProcs[8].Address)
#define AVIFileInit   	            	(*AVIProcs[9].Address)
#define AVIFileExit   	            	(*AVIProcs[10].Address)

#define VerQueryValueW	            	(*VERSIONProcs[0].Address)
#define GetFileVersionInfoW            	(*VERSIONProcs[1].Address)
#define GetFileVersionInfoSizeW        	(*VERSIONProcs[2].Address)

#endif
