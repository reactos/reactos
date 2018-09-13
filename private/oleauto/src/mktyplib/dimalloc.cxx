/***
*dimalloc.cxx
*
*  Copyright (C) 1992-93, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This file contains a debug implementation of the IMalloc interface.
*
*  This implementation is basically a simple wrapping of the C runtime,
*  with additional work to detect memory leakage, and memory overwrite.
*
*  Leakage is detected by tracking each allocation in an address
*  instance table, and then checking to see if the table is empty
*  when the last reference to the allocator is released.
*
*  Memory overwrite is detected by placing a signature at the end
*  of every allocated block, and checking to make sure the signature
*  is unchanged when the block is freed.
*
*  This implementation also has additional param validation code, as
*  well as additional check make sure that instances that are passed
*  to Free() were actually allocated by the corresponding instance
*  of the allocator.
*
*
*  Creating an instance of this debug allocator that uses the default
*  output interface would look like the following,
*
*
*  BOOL init_application_instance()
*  {
*    HRESULT hresult;
*    IMalloc FAR* pmalloc;
*
*    pmalloc = NULL;
*
*    if((hresult = CreateDbAlloc(DBALLOC_NONE, NULL, &pmalloc)) != NOERROR)
*      goto LReturn;
*
*    hresult = OleInitialize(pmalloc);
*
*  LReturn:;
*    if(pmalloc != NULL)
*      pmalloc->Release();
*
*    return (hresult == NOERROR) ? TRUE : FALSE;
*  }
*
*
*  CONSIDER: could add an option to force error generation, something
*   like DBALLOC_ERRORGEN, that works along the lines of OB's 
*   DebErrorNow.
*
*  CONSIDER: add support for heap-checking. say for example,
*   DBALLOC_HEAPCHECK would do a heapcheck every free? every 'n'
*   calls to free? ...
*
*
*Revision History:
*
* [00]	25-Feb-92 bradlo: Created.
* [01]	03-Mar-93 rajivk: Added to ebapp.
*
*Implementation Notes:
*
*  The method IMalloc::DidAlloc() is allowed to always return
*  "Dont Know" (-1).  This method is called by Ole, and they take
*  some appropriate action when they get this answer. UNDONE -- elaborate.
*
*****************************************************************************/

#ifdef DEBUG	// entire file
#if 0
#include "pch.c"
#pragma hdrstop(PCHNAME)
#else	//0
#include "mktyplib.h"
#ifdef MAC
#define OE_MAC	TRUE
#endif
#ifdef WIN32
#define OE_WIN32 TRUE
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
#define OE_RISC TRUE
#endif
#endif
#ifdef WIN16
#define OE_WIN16 TRUE
#endif
#endif //0

#if OE_MAC
#include <macos\osutils.h>
#include <macos\sysequ.h>
#endif // OE_MAC

// Note: this file is designed to be stand-alone; it includes a
// carefully chosen, minimal set of headers.
//
// For conditional compilation we use the ole2 conventions,
//    _MAC      = mac
//    WIN32     = Win32 (NT really)
//    <nothing> = defaults to Win16
#ifdef _DEBUG
#undef _DEBUG
#endif
#define _DEBUG 1


#if !OE_WIN32
#include "ole2.h"
#if !OE_MAC
#include "compobj.h"
#endif
#endif //!OE_WIN32

#if OE_MAC
// include Mac stuff
#include "macos\memory.h"
#include "macos\errors.h"

typedef VOID* HSYS;
#define HSYS_Nil ((HSYS)NULL)


#endif

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>

#if 0
#include "ebapp.h"
#include "dimalloc.hxx"

// This global is defined in bind.c and contains the name of the
// log file specified by -o, if there is one.
//
extern "C" {
  CHAR g_szLogFile[];
}
#else //0

#if OE_RISC
        // UNDONE: RISC [jeffrob] Currently use same alignment for ALL risc
        // UNDONE: platforms.
	#define cbAlign 8
#endif	// OE_RISC

#if OE_MAC
  #include "macos\menus.h"
  #include "macos\dialogs.h"
  #include "stdio.h"
  #include "macos\files.h"
  #include "macos\lists.h"
#endif // OE_MAC


#ifdef __cplusplus
extern "C" {
#endif

// Assertion macro.
#define DebAssert(fExpr, szComment) \
    if (!(fExpr)) \
	DebAssertShow(__FILE__, __LINE__, szComment); \
    else 0 /* useless statement */

void DebAssertShow(LPSTR szFileName, UINT uLine, LPSTR szComment);


#define DebAssertNum(fExpr, szComment, nErr) \
    if (!(fExpr)) \
	DebAssertShow(__FILE__, __LINE__, szComment); \
    else 0 /* useless statement */

#if 0
// Macros for error checking:
#define IfErrExit(s) { if (eberr = (s)) goto Exit; }
#define IfNullExit(s) { if ( !(s) ) { eberr = EBERR_OutOfMemory; goto Exit;} }
#define IfErrGo(s) { if (eberr = (s)) goto Error; }
#define IfErrGoTo(s, label) { if (eberr = (s)) goto label; }
#endif //0

#ifndef WIN32
#ifndef _INC_WINDOWS
 #define LOWORD(l)	     ((WORD)(DWORD)(l))
 #define HIWORD(l)	     ((WORD)((((DWORD)(l)) >> 16) & 0xFFFF))
#endif //_INC_WINDOWS
#endif //!WIN32

#if 0
// Maintains the hproj/docfile/project substorage relationship.
typedef struct STGREC {
    IStorage FAR *pstgFile;
    IStorage FAR *pstgProj;
    HPROJECT hproj; // The hproject associated with this storage if known.
    LPSTR bstrName; // The name of the file.
    BOOL isTemp;    // TRUE if this is a temp file that should be deleted
		    // when closed.
} STGREC;

STGREC FAR *CreateTmpStorage(void);
EBERR  CopyTmpFileToDest(HPROJECT hproj, LPSTR lpstrFileName);
void PASCAL FreeStorage(void);
STGREC FAR *FindStgRecByName(LPSTR szName);
void CloseProjectStorage(HPROJECT hproj);

#if OE_WIN
extern HINSTANCE g_hInst;	 // host-app instance handle
extern HWND g_hwndSrchDlg;	// modeless search dialog
extern HWND g_hwndMDISpace;	// window handle of host app's MDI space
extern HWND g_hwndParent;	// this instance's top-level window.
extern char g_szProject[];	// project window class name
extern char g_szDebugWnd[];	// debug window class name
extern char g_szPolyWnd[];	// polygon window class name
#endif //OE_WIN

#if OE_MAC
extern BOOL fMacHelpLoaded;  //TRUE if the mac help file is laoded
extern FSSpec fsHelpFile;    //FSSpec for the help file
#endif

extern char *g_szTitleText;	// default title bar text
extern HWND g_hwndEbActive;	// Active O.B. window.
extern BOOL g_fAppActive;	// is this application active?
extern BOOL g_fParentActive;	// is the parent (frame) window active?
extern LPSTR  g_lpszHelpFile;	// help file if F1 pressed
extern DWORD  g_dwHelpContext;	// help context if F1 pressed
extern UINT   g_cMsgLoop;	// How many nested message loops?
extern INT  g_fNoAllocs;	// Are allocations allowed?

extern int g_cxScroll;	// width of scrollbar
extern int g_cyScroll;	// heigth of scrollbar
// Errros Used by Ebapp only.
#define EBERR_IdNotFound	    0xffff
extern int g_fDbcs;   // is DBCS support enabled?


LPDISPATCH EBCALL Bind(LPVARIANT lpvarRngName);
LPDISPATCH EBCALL NewPoint(void);
LPDISPATCH EBCALL NewPoly(void);
LPDISPATCH EBCALL GetProjCollection(void);
LPDISPATCH EBCALL NewList(void);
LPDISPATCH EBCALL GetAppObj(void);
void AppObjTerm();

void CallEbNotifyWindow(HWND hwnd, EBMSG ebmsg, LONG lparam);
void QuitEbApp();
void DebAssertWrite(char *szFileName, char *szMsg);

LPVOID EBCALL CreateNewObject(LPSTR szProjName, LPSTR szClassName);
BOOL GetDebIMalloc(IMalloc FAR* FAR* ppmalloc);

VOID FAR* EBCALL CreateInstance(BSTR szTlibName, UINT uIndex);
VOID FAR* CreateInst(BSTR szTlibName, UINT uIndex);

BOOL GetStringDlg(char **pszString);
BOOL RenameDlg(char **pszNewName);

#if OE_MAC
Boolean CheckMacHelpEvent(EventRecord *evtPtr);
OSErr GetWDInfoTemp(short wdRefNum, short *pvRefNum, long *pdirid);
BOOL MacGetFullPath(long dirid, short vRefNum, LPSTR szPath);
void InitializeGrafport();
BOOL FCalcTextHeight(CHAR *szText, SHORT cx, SHORT *cy);
void RectToRECT (const Rect *prcm, RECT *prc);
void RECTToRect (const RECT *prc, Rect *prcm);
BOOL PaintWindow(EventRecord *pevt);
#endif // OE_MAC

DEFINE_GUID (IID_CVARCOLLECT, 0x2d736941, 0xc370, 0x1068, 0xb3, 0x69, 0x08, 0x00, 0x2b, 0x2b, 0x37, 0x87);
#endif //0

#ifdef __cplusplus
}
#endif

#if OE_MAC
#ifdef _fstrcpy
#undef _fstrcpy
#endif
#ifdef _fstrncpy
#undef _fstrncpy
#endif
#ifdef _fstrncat
#undef _fstrncat
#endif
#ifdef _fstrcat
#undef _fstrcat
#endif
#ifdef _fstrchr
#undef _fstrchr
#endif

#define _fstrcpy      strcpy
#define _fstrncpy     strncpy
#define _fstrncat     strncat
#define _fstrcat      strcat
#define _fstrchr      strchr
#endif	//OE_MAC

#if OE_MAC
#define STDOLE_FILE	 "mstdole.tlb"
#define APPOBJ_FILE	 "mappobj"
#else	//!OE_MAC

#if OE_WIN32
#define STDOLE_FILE	 "stdole32.tlb"
#define APPOBJ_FILE	 "apo"
#else	//OE_WIN16
#define STDOLE_FILE	 "stdole.tlb"
#define APPOBJ_FILE	 "apobj"
#endif	//OE_WIN16

#endif	//!OE_MAC

#define VBAOLB_FILE	 "vba"

#endif

#ifndef DBALLOC_H_INCLUDED /* { */
#define DBALLOC_H_INCLUDED


interface IDbOutput : public IUnknown
{
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void FAR* FAR* ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    STDMETHOD_(void, Printf)(THIS_
      char FAR* szFmt, ...) PURE;

    STDMETHOD_(void, Assertion)(THIS_
      BOOL cond,
      char FAR* szExpr,
      char FAR* szFile,
      UINT uLine,
      char FAR* szMsg) PURE;
};


STDAPI CreateDbAlloc(
    ULONG options,
    IDbOutput FAR* pdbout,
    IMalloc FAR* FAR* ppmalloc);

// dballoc option flags - these are set at create time.

#define DBALLOC_NONE	0


#endif /* } DBALLOC_H_INCLUDED */

#define OOB_SELECTOROF(p)   ((USHORT) ((ULONG) (p) >> 16))
#define DIM(X) (sizeof(X)/sizeof((X)[0]))
#define UNREACHED 0
static char szSigMsg[] = "Signature Check Failed";


  // This defn switches in array handling code that knows how to
  // handle non-aligned & non-power-of-2 arrays.
  // Defined & used in hugearry.asm, dimalloc.cxx & exvarg.cxx
  // In dimalloc.cxx it enables signature prefixing for huge
  // arrays thus allowing testing of the array-adjust code in VBA.
#define	ADJUST_ARRAYS 1


#if OE_WIN16
#define MAX_SIZE 64000
#else
#define MAX_SIZE UINT_MAX
#endif

#define Max
VOID FAR * HugeAlloc(DWORD  bch);
VOID FAR *HugeRealloc(VOID FAR *pv, DWORD  bchNew);
VOID FAR *HugeFree(VOID FAR *  pv);




#if defined(WIN32)

# define MEMCMP(PV1, PV2, CB)	memcmp((PV1), (PV2), (CB))
# define MEMCPY(PV1, PV2, CB)	memcpy((PV1), (PV2), (CB))
# define MEMSET(PV,  VAL, CB)	memset((PV),  (VAL), (CB))
# define MALLOC(CB)		malloc(CB)
# define REALLOC(PV, CB)	realloc((PV), (CB))
# define FREE(PV)		free(PV)
# define MSIZE(PV)		_msize(PV)
//UNDONE: NT has no heapmin() function?
//# define HEAPMIN()		_heapmin()

#elif OE_MAC

#define MEMCPY			memcpy
// #define REALLOC(PV, CB)	   SetPtrSize((char _near *)(PV), (CB))
// #define MALLOC(CB)		   NewPtr(CB)
// #define FREE(PV)		   DisposPtr((char _near *)(PV))
#define REALLOC(PV, CB) 	realloc((PV), (CB))
#define MALLOC(CB)		malloc(CB)
#define FREE(PV)		free(PV)
#define MEMCMP(PV1, PV2, CB)	memcmp((PV1), (PV2), (CB))
#define MEMSET(PV,  VAL, CB)	memset((PV),  (VAL), (CB))
// #define MSIZE(PV)		   GetPtrSize((char _near *)(PV))
#define MSIZE(PV)		_msize(PV)
#define HEAPMIN()		_heapmin()

#else

# define MEMCMP(PV1, PV2, CB)	_fmemcmp((PV1), (PV2), (CB))
# define MEMCPY(PV1, PV2, CB)	_fmemcpy((PV1), (PV2), (CB))
# define MEMSET(PV,  VAL, CB)	_fmemset((PV),  (VAL), (CB))
# define MALLOC(CB)		_fmalloc(CB)
# define REALLOC(PV, CB)	_frealloc(PV, CB)
# define FREE(PV)		_ffree(PV)
# define MSIZE(PV)		_fmsize(PV)
# define HEAPMIN()		_fheapmin()

#endif

class FAR CStdDbOutput : public IDbOutput {
public:
    static IDbOutput FAR* Create();

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);


    // IDbOutput methods

    STDMETHOD_(void, Printf)(char FAR* szFmt, ...);

    STDMETHOD_(void, Assertion)(
      BOOL cond,
      char FAR* szExpr,
      char FAR* szFile,
      UINT uLine,
      char FAR* szMsg);


    void FAR* operator new(size_t cb){
      return MALLOC(cb);
    }
    void operator delete(void FAR* pv){
      FREE(pv);
    }

    CStdDbOutput(){
      m_refs = 0;
    }

private:
    ULONG m_refs;

    char m_rgch[128]; // buffer for output formatting
};



//---------------------------------------------------------------------
//                implementation of the debug allocator
//---------------------------------------------------------------------

class FAR CAddrNode
{
public:
    void FAR*      m_pv;	// instance
    ULONG	   m_cb;	// size of allocation in BYTES
    ULONG          m_nAlloc;	// the allocation pass count
    CAddrNode FAR* m_next;

    void FAR* operator new(size_t cb){
      return MALLOC(cb);
    }
    void operator delete(void FAR* pv){
      FREE(pv);
    }
};


class FAR CDbAlloc : public IMalloc
{
public:
    static HRESULT Create(
      ULONG options, IDbOutput FAR* pdbout, IMalloc FAR* FAR* ppmalloc);

    // IUnknown methods

    STDMETHOD(QueryInterface)(REFIID riid, void FAR* FAR* ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IMalloc methods

    STDMETHOD_(void FAR*, Alloc)(ULONG cb);
    STDMETHOD_(void FAR*, Realloc)(void FAR* pv, ULONG cb);
    STDMETHOD_(void, Free)(void FAR* pv);
    STDMETHOD_(ULONG, GetSize)(void FAR* pv);
    STDMETHOD_(int, DidAlloc)(void FAR* pv);
    STDMETHOD_(void, HeapMinimize)(void);

    VOID  IMallocHeapChecker();
    VOID  MemInstance();

    void FAR* operator new(size_t cb){
      return MALLOC(cb);
    }
    void operator delete(void FAR* pv){
      FREE(pv);
    }

private:

    ULONG m_refs;
    ULONG m_cAllocCalls;		// total count of allocation calls
    IDbOutput FAR* m_pdbout;		// output interface
    CAddrNode FAR* m_rganode[64];	// address instance table


    // instance table methods

    void AddInst(void FAR* pv, ULONG nAlloc, ULONG cb);
    void DelInst(void FAR* pv);
    CAddrNode FAR* GetInst(void FAR* pv);

    BOOL IsEmpty(void);
    void DumpInstTable(void);

    void DumpInst(CAddrNode FAR* pn);

    inline UINT HashInst(void FAR* pv) const {
      return ((UINT)((ULONG)pv >> 4)) % DIM(m_rganode);
    }


    // output method(s)

    inline void Assertion(
      BOOL cond,
      char FAR* szExpr,
      char FAR* szFile,
      UINT uLine,
      char FAR* szMsg)
    {
      m_pdbout->Assertion(cond, szExpr, szFile, uLine, szMsg);
    }

    #define ASSERT(X) Assertion(X, #X, __FILE__, __LINE__, NULL)

    #define ASSERTSZ(X, SZ) Assertion(X, #X, __FILE__, __LINE__, SZ)

#if OE_RISC
    char m_rgchSig[cbAlign];
#else
    char m_rgchSig[4];
#endif // !(OE_RISC)

public:
    CDbAlloc(){
      m_refs = 1;
      m_pdbout = NULL;
      m_cAllocCalls = 0;
      MEMSET(m_rganode, 0, sizeof(m_rganode));
      m_rgchSig[0] = m_rgchSig[2] = (char)0xBA;
      m_rgchSig[1] = m_rgchSig[3] = (char)0xBE;
#if OE_RISC
#if (cbAlign == 8)
      m_rgchSig[4] = m_rgchSig[6] = (char)0xBA;
      m_rgchSig[5] = m_rgchSig[7] = (char)0xBE;
#else
      #error Invalid cbAlign value.
#endif  // cbAlign
#endif	// OE_RISC
    }
};


/***
*HRESULT CreateDbAlloc(ULONG, IDbOutput*, IMalloc**)
*Purpose:
*  Create an instance of CDbAlloc -- a debug implementation
*  of IMalloc.
*
*Entry:
*  pdbout = optional IDbOutput interface to use for ouput
*    (if NULL, then the default debug output interface will be used)
*  options = 
*
*Exit:
*  return value = HRESULT
*
*  *ppmalloc = pointer to an IMalloc interface
*
***********************************************************************/
STDAPI
CreateDbAlloc(
    ULONG options,
    IDbOutput FAR* pdbout,
    IMalloc FAR* FAR* ppmalloc)
{
    return CDbAlloc::Create(options, pdbout, ppmalloc);
}

HRESULT
CDbAlloc::Create(
    ULONG options,
    IDbOutput FAR* pdbout,
    IMalloc FAR* FAR* ppmalloc)
{
    HRESULT hresult;
    CDbAlloc FAR* pmalloc;


    // default the instance of IDbOutput if the user didn't supply one
    if(pdbout == NULL && ((pdbout = CStdDbOutput::Create()) == NULL)){
      hresult = ResultFromScode(E_OUTOFMEMORY);
      goto LError0;
    }

    pdbout->AddRef();

    if((pmalloc = new FAR CDbAlloc()) == NULL){
      hresult = ResultFromScode(E_OUTOFMEMORY);
      goto LError1;
    }

    pmalloc->m_pdbout = pdbout;

    *ppmalloc = pmalloc;

    return NOERROR;

LError1:;
    pdbout->Release();
    pmalloc->Release();

LError0:;
    return hresult;
}

STDMETHODIMP
CDbAlloc::QueryInterface(REFIID riid, void FAR* FAR* ppv)
{
    HRESULT hresult;
#if OE_MAC
    long a5Save = SetA5(*((long *)(long)CurrentA5));
#endif // OE_MAC

    if(riid == IID_IUnknown ){
      *ppv = this;
      AddRef();
      hresult = NOERROR;
    }
    else
      hresult = ResultFromScode(E_NOINTERFACE);

#if OE_MAC
    SetA5(a5Save);
#endif // OE_MAC

    return hresult;
}

STDMETHODIMP_(ULONG)
CDbAlloc::AddRef()
{
    return ++m_refs;
}

STDMETHODIMP_(ULONG)
CDbAlloc::Release()
{
//    FILE *pfileLog;  // UNDONE stevenl -- not used right now

    if(--m_refs == 0){

#if OE_MAC
      long a5Save = SetA5(*((long *)(long)CurrentA5));
#endif // OE_MAC

      // check for memory leakage
      if(!IsEmpty()){
	m_pdbout->Printf("Memory Leak Detected,\n");
	DumpInstTable();
	ASSERTSZ(FALSE, "Memory leaked");
      }
      else {
	// No memory has leaked. If we're running a test script,
	// we want the line "No Memory Leaks." at the end of the
	// script, so that there will be a baseline failure if
	// memory has leaked. The global g_szLogFile has the name
	// of the file, if there is one.
	//
#if 0
	if (strlen(g_szLogFile)) {
#if 0
// UNDONE 20-May-93 stevenl:
// This is #ifdef'ed out until we feel like
// messing with all the baselines again.
//
	  pfileLog = fopen(g_szLogFile,"at");
	  if (pfileLog) {
	    fprintf(pfileLog, "No Memory Leaks.");
	    fclose(pfileLog);
	  }
#endif
	} // if
#endif //0
      } // else

      m_pdbout->Release();
      delete this;

#if OE_MAC
      SetA5(a5Save);
#endif // OE_MAC

      return 0;
    }

    return m_refs;
}

STDMETHODIMP_(void FAR*)
CDbAlloc::Alloc(ULONG cb)
{
    ULONG size;
    VOID FAR* pv;

#if OE_MAC
    long a5Save = SetA5(*((long *)(long)CurrentA5));
#endif // OE_MAC

#if 0
    // If allocations have been disabled (probably because we're in
    // the middle of a save), just return NULL.
    if (g_fNoAllocs)
      return NULL;
#endif //0

    IMallocHeapChecker();

    // ++m_cAllocCalls;
    MemInstance();

    size = (ULONG)cb;

    // Support for Huge Arrays
    if ((cb + 2*sizeof(m_rgchSig)) < MAX_SIZE) {

      if((pv = MALLOC((size_t)(size + 2*sizeof(m_rgchSig)))) == NULL)
	goto Error;
    }
    else {
      if ((pv = (VOID FAR *)HugeAlloc(size + 2*sizeof(m_rgchSig))) == NULL)
	goto Error;

    }

    // set allocated block to some non-zero value
    MEMSET(pv, -1, (size_t)(size + 2*sizeof(m_rgchSig)));

#if ADJUST_ARRAYS
    if ((cb + 2*sizeof(m_rgchSig)) < MAX_SIZE) {
      // put signature at end of allocated block
      MEMCPY((char FAR*)pv + size + sizeof(m_rgchSig), m_rgchSig, (size_t)sizeof(m_rgchSig));
    }

    // put signature at the head of the allocated block
    MEMCPY((char FAR*)pv , m_rgchSig, sizeof(m_rgchSig));
    pv = (char FAR*)pv + sizeof(m_rgchSig);

#else
    // We do not put the signature for huge memory allocation
    if ((cb + 2*sizeof(m_rgchSig)) < MAX_SIZE) {
      // put signature at end of allocated block
      MEMCPY((char FAR*)pv + size + sizeof(m_rgchSig), m_rgchSig, (size_t)sizeof(m_rgchSig));

      // put signature at the head of the allocated block
      MEMCPY((char FAR*)pv , m_rgchSig, sizeof(m_rgchSig));

    }

    // For Huge allocation return the pointer to the beginnig of the seg.
    if ((cb + 2*sizeof(m_rgchSig)) < MAX_SIZE) {
      // return the pointer to the beginning of the block to be returned
      pv = (char FAR*)pv + sizeof(m_rgchSig);
    }
#endif

    // save the address returned and it's size also.
    AddInst(pv, m_cAllocCalls, size);

    // FALL THROUGH!!!

Error:

#if OE_MAC
    SetA5(a5Save);
#endif // OE_MAC

    return pv;
}



STDMETHODIMP_(void FAR*)
CDbAlloc::Realloc(void FAR* pv, ULONG cb)
{
    ULONG size;
    ULONG sizeToFree;
    CAddrNode FAR* pn;

    if(pv == NULL){
      return Alloc(cb);
    }

#if OE_MAC
    long a5Save = SetA5(*((long *)(long)CurrentA5));
#endif // OE_MAC

    // ++m_cAllocCalls;
    MemInstance();

    pn = GetInst(pv);

    sizeToFree = pn->m_cb;
    ASSERT(pn != NULL);

    if(cb == 0){
      Free(pv);
      pv = NULL;
      goto Done;
    }

#if 0
    // If allocations have been disabled (probably because we're in
    // the middle of a save) and we're trying to increase the size of
    // the allocated block, just return NULL.  We allow decreases, since
    // that can't cause an out of memory error in a real allocator.
    if (cb > sizeToFree && g_fNoAllocs)
      return NULL;
#endif //0

    size = cb;

    // UNDONE : This does not handle the case when  we mix the huge alloc and
    //		and realloc.
    if (((sizeToFree + 2*sizeof(m_rgchSig)) < MAX_SIZE) &&
	 ((size + 2*sizeof(m_rgchSig)) < MAX_SIZE)) {

      // we delete the instance from the table using the address passed in.
      DelInst(pv);

      // get the address of the original memory allocated
      pv = (char FAR*)pv - sizeof(m_rgchSig);

      // allocte enough memory to put the signature also.
      if ((pv = REALLOC(pv, (size_t)(size + 2*sizeof(m_rgchSig)))) == NULL)
	goto Done;
    }
    else {
      if (((sizeToFree + 2*sizeof(m_rgchSig)) >= MAX_SIZE) &&
	      ((size + 2*sizeof(m_rgchSig)) >= MAX_SIZE))  {

	// we delete the instance from the table using the address passed in.
	DelInst(pv);

#if ADJUST_ARRAYS
	// get the address of the original memory allocated
	pv = (char FAR*)pv - sizeof(m_rgchSig);
#endif
	// allocte enough memory to put the signature also.
	if ((pv = HugeRealloc(pv, size + 2*sizeof(m_rgchSig))) == NULL)
	  goto Done;
      }
      else {
	VOID FAR *pvNew;
	ULONG cbCopy;


	if ((pvNew = Alloc(size)) == NULL) {
	  // if the memory to be free is < MAX_SIZE then adjust the pointer
	  if ((sizeToFree + 2*sizeof(m_rgchSig)) < MAX_SIZE) {
	    // get the address of the original memory allocated
	    pv = (char FAR*)pv - sizeof(m_rgchSig);
	  }

	  Free(pv);
	  pv = NULL;
	  goto Done;
	}

	cbCopy = (sizeToFree < size) ? sizeToFree : size;

	// copy the original contents
	MEMCPY((char FAR*)pvNew , (char FAR*)pv, (size_t)cbCopy);

#if ADJUST_ARRAYS
	pv = (char FAR*)pv - sizeof(m_rgchSig);
#else
	// if the memory to be free is < MAX_SIZE then adjust the pointer
	if ((sizeToFree + 2*sizeof(m_rgchSig)) < MAX_SIZE) {
	  // get the address of the original memory allocated
	  pv = (char FAR*)pv - sizeof(m_rgchSig);
	}

#endif

	Free((char FAR*)pv + sizeof(m_rgchSig));
	pv = pvNew;
	goto Done;

      }

    }

    // We do not put the signature at the tail for huge memory allocation
    if ((size + 2*sizeof(m_rgchSig)) < MAX_SIZE) {
      // put signature at end of allocated block
      // NOTE:- the signature for the header is already there.
      MEMCPY((char FAR*)pv + size + sizeof(m_rgchSig), m_rgchSig, sizeof(m_rgchSig));
    }

#if ADJUST_ARRAYS
    pv = (char FAR*)pv + sizeof(m_rgchSig);
#else
    // For Huge allocation return the pointer to the beginnig of the seg.
    if ((cb + 2*sizeof(m_rgchSig)) < MAX_SIZE) {
      // return the pointer to the beginning of the block (to be used
      // by the caller).
      pv = (char FAR*)pv + sizeof(m_rgchSig);
    }
#endif

    // save the address returned and it's size also.
    AddInst(pv, m_cAllocCalls, size);

Done:
#if OE_MAC
    SetA5(a5Save);
#endif // OE_MAC
    return pv;
}

STDMETHODIMP_(void)
CDbAlloc::Free(void FAR* pv)
{
    CAddrNode FAR* pn;
    ULONG sizeToFree;

#if OE_MAC
    long a5Save = SetA5(*((long *)(long)CurrentA5));
#endif // OE_MAC

	// STORAGE.DLL Calls Free(NULL) alot
	if (pv == NULL)
		goto Done;

    pn = GetInst(pv);

    // check for attempt to free an instance we didnt allocate
    if(pn == NULL){
      ASSERTSZ(FALSE, "pointer freed by wrong allocator");
      goto Done;
    }

    // We do not put the signature at the tail for huge memory allocation
    if ((pn->m_cb + 2*sizeof(m_rgchSig)) < MAX_SIZE) {
      // verify the signature  at the tail
      if(MEMCMP((char FAR*)pv + pn->m_cb, m_rgchSig, sizeof(m_rgchSig)) != 0){
	m_pdbout->Printf(szSigMsg); m_pdbout->Printf("\n");
	DumpInst(GetInst(pv));
	ASSERTSZ(FALSE, szSigMsg);
      }

#if ADJUST_ARRAYS
    }

    {
#endif
      // verify the signature  at the head
      if(MEMCMP((char FAR*)pv - sizeof(m_rgchSig), m_rgchSig, sizeof(m_rgchSig)) != 0){
	m_pdbout->Printf(szSigMsg); m_pdbout->Printf("\n");
	DumpInst(GetInst(pv));
	ASSERTSZ(FALSE, szSigMsg);
      }

    }


    sizeToFree = pn->m_cb;

    DelInst(pv);

#if ADJUST_ARRAYS
    pv = (char FAR*)pv - sizeof(m_rgchSig);
#else
    if ((sizeToFree + 2*sizeof(m_rgchSig)) < MAX_SIZE) {
      // get the address of the original memory allocated
      pv = (char FAR*)pv - sizeof(m_rgchSig);
    }
#endif

    // stomp on the contents of the block
    MEMSET(pv, 0xCC, (size_t)(sizeToFree + 2*sizeof(m_rgchSig)));

    if ((sizeToFree + 2*sizeof(m_rgchSig)) < MAX_SIZE) {

      FREE(pv);
    }
    else {
      HugeFree(pv);
    }

Done: ;
#if OE_MAC
    SetA5(a5Save);
#endif // OE_MAC
}

STDMETHODIMP_(ULONG)
CDbAlloc::GetSize(void FAR* pv)
{
    CAddrNode FAR* pn;
    ASSERT((pn = GetInst(pv)) != NULL);

    // dont count extra signature bytes in size
    return pn->m_cb;
}

VOID CDbAlloc::MemInstance()
{
    ++m_cAllocCalls;

}


/***
*PUBLIC HRESULT CDbAlloc::DidAlloc
*Purpose:
*  Answer if the given address belongs to a block allocated by
*  this allocator.
*  
*Entry:
*  pv = the instance to lookup
*
*Exit:
*  return value = int
*    1 - did alloc
*    0 - did *not* alloc
*   -1 - dont know (according to the ole2 spec it is always legal
*        for the allocator to answer "dont know")
*
***********************************************************************/
STDMETHODIMP_(int)
CDbAlloc::DidAlloc(void FAR* pv)
{
    return -1; // answer "I dont know"
}


STDMETHODIMP_(void)
CDbAlloc::HeapMinimize()
{
#if !OE_WIN32 //UNDONE: what does HeapMinimize mean for WIN32?
#if OE_MAC
    long a5Save = SetA5(*((long *)(long)CurrentA5));
#endif // OE_MAC
    HEAPMIN();
#if OE_MAC
    SetA5(a5Save);
#endif // OE_MAC
#endif
}


//---------------------------------------------------------------------
//                      instance table methods
//---------------------------------------------------------------------

/***
*PRIVATE CDbAlloc::AddInst
*Purpose:
*  Add the given instance to the address instance table.
*
*Entry:
*  pv = the instance to add
*  nAlloc = the allocation passcount of this instance
*
*Exit:
*  None
*
***********************************************************************/
void
CDbAlloc::AddInst(void FAR* pv, ULONG nAlloc, ULONG cb)
{
    UINT hash;
    CAddrNode FAR* pn;


    ASSERT(pv != NULL);

    pn = (CAddrNode FAR*)new FAR CAddrNode();

    ASSERT(pn != NULL);

    pn->m_pv = pv;
    pn->m_cb = cb;
    pn->m_nAlloc = nAlloc;

    hash = HashInst(pv);
    pn->m_next = m_rganode[hash];
    m_rganode[hash] = pn;
}


/***
*UNDONE
*Purpose:
*  Remove the given instance from the address instance table.
*
*Entry:
*  pv = the instance to remove
*
*Exit:
*  None
*
***********************************************************************/
void
CDbAlloc::DelInst(void FAR* pv)
{
    CAddrNode FAR* FAR* ppn, FAR* pnDead;

    for(ppn = &m_rganode[HashInst(pv)]; *ppn != NULL; ppn = &(*ppn)->m_next){
      if((*ppn)->m_pv == pv){
	pnDead = *ppn;
	*ppn = (*ppn)->m_next;
	delete pnDead;
	// make sure it doesnt somehow appear twice
	ASSERT(GetInst(pv) == NULL);
	return;
      }
    }

    // didnt find the instance
    ASSERT(UNREACHED);
}


CAddrNode FAR*
CDbAlloc::GetInst(void FAR* pv)
{
    CAddrNode FAR* pn;

    for(pn = m_rganode[HashInst(pv)]; pn != NULL; pn = pn->m_next){
      if(pn->m_pv == pv)
        return pn;
    }
    return NULL;
}


void
CDbAlloc::DumpInst(CAddrNode FAR* pn)
{
    m_pdbout->Printf("[%lp]  nAlloc=%ld  size=%ld\n",
      pn->m_pv, pn->m_nAlloc, GetSize(pn->m_pv));
}


/***
*PRIVATE BOOL IsEmpty
*Purpose:
*  Answer if the address instance table is empty.
*
*Entry:
*  None
*
*Exit:
*  return value = BOOL, TRUE if empty, FALSE otherwise
*
***********************************************************************/
BOOL
CDbAlloc::IsEmpty()
{
    UINT u;

    for(u = 0; u < DIM(m_rganode); ++u){
      if(m_rganode[u] != NULL) {
#if OE_MAC
	// UNDONE: (dougf) temporary code to work around bug in OleInitialize.
	// UNDONE: OleInitialize in the OLE2 Beta 1 build allocs 42 bytes of
	// UNDONE: memory in it's first alloc, and never frees it.  It
	// UNDONE: also randomly fails to dealloc 39 bytes in WriteClassStg.
	// UNDONE: Rip this code when we upgrade to a newer Mac OLE that has
	// UNDONE: these bugs fixed.
        CAddrNode FAR* pn;
	ULONG cb;

        pn = m_rganode[u];
	cb = GetSize(pn->m_pv);

        if (pn->m_nAlloc == 1 && cb == 42)
	  continue;	// ignore OleInitialize's memory leak
        if (cb == 39)
	  continue;	// ignore the random OLE memory leak in WriteClassStg
			// (happens when running cl\tbmacole.scr)
#endif	//OE_MAC

#if OE_WIN32 && 0
	// UNDONE: (dougf) temporary code to work around bug in OleInitialize.
	// UNDONE: OleInitialize allocs 320 bytes of memory in it's first
	// UNDONE: alloc, and 60 bytes in it's 3rd alloc, and never frees them.
	// UNDONE: OleUnitialize allocs 68 & 172 bytes and never frees them.
	// UNDONE: Rip this code when we upgrade
	// UNDONE: to a newer NT OLE that has these bugs fixed.
        CAddrNode FAR* pn;
	ULONG cb;

        pn = m_rganode[u];
	cb = GetSize(pn->m_pv);

        if (pn->m_nAlloc == 1 && cb == 320)
	  continue;	// ignore OleInitialize's first memory leak
        if (pn->m_nAlloc == 3 && cb == 60)
	  continue;	// ignore OleInitialize's second memory leak
        if (cb == 68)
	  continue;	// ignore OleUninitialize's first memory leak
        if (cb == 172)
	  continue;	// ignore OleUninitialize's second memory leak
#endif	//OE_WIN32

#if !FV_UNICODE_OLE 	// UNDONE: temporary (ignore the dstrmgr leaks)
	return FALSE;	// some other leak
#endif //!FV_UNICODE_OLE
      }
    }

    return TRUE;
}


/***
*PRIVATE CDbAlloc::Dump
*Purpose:
*  Print the current contents of the address instance table,
*
*Entry:
*  None
*
*Exit:
*  None
*
***********************************************************************/
void
CDbAlloc::DumpInstTable()
{
    UINT u;
    CAddrNode FAR* pn;

    for(u = 0; u < DIM(m_rganode); ++u){
      for(pn = m_rganode[u]; pn != NULL; pn = pn->m_next){
	DumpInst(pn);
      }
    }
}


/***********
* GetDebIMalloc()
*
* Purpose : Walks the Heap and verifies that the heap is not corrupted
*
**************************************************************************/
VOID CDbAlloc::IMallocHeapChecker()
{

    UINT u;
    CAddrNode FAR* pn;

    return;

    for(u = 0; u < DIM(m_rganode); ++u){

      for(pn = m_rganode[u]; pn != NULL; pn = pn->m_next){
	// Verify that the signature are the foot and the head of this
	// instance is correct.

	// We do not put the signature at the tail for huge memory allocation
	if ((pn->m_cb + 2*sizeof(m_rgchSig)) < MAX_SIZE) {
	  // verify the signature  at the tail
	  if (MEMCMP((char FAR*)pn->m_pv + pn->m_cb, m_rgchSig, sizeof(m_rgchSig)) != 0){
	    m_pdbout->Printf(szSigMsg); m_pdbout->Printf("\n");
	    DumpInst(GetInst(pn->m_pv));
	    ASSERTSZ(FALSE, szSigMsg);
	  }

	  // verify the signature  at the head
	  if (MEMCMP((char FAR*)pn->m_pv - sizeof(m_rgchSig), m_rgchSig, sizeof(m_rgchSig)) != 0){
	    m_pdbout->Printf(szSigMsg); m_pdbout->Printf("\n");
	    DumpInst(GetInst(pn->m_pv));
	    ASSERTSZ(FALSE, szSigMsg);
	  }
	}

      }  // for loop
    }  // for loop
}

//---------------------------------------------------------------------
//                implementation of CStdDbOutput
//---------------------------------------------------------------------

IDbOutput FAR*
CStdDbOutput::Create()
{
    return (IDbOutput FAR*)new FAR CStdDbOutput();
}

STDMETHODIMP
CStdDbOutput::QueryInterface(REFIID riid, void FAR* FAR* ppv)
{
    if(riid == IID_IUnknown){
      *ppv = this;
      AddRef();
      return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG)
CStdDbOutput::AddRef()
{
    return ++m_refs;
}

STDMETHODIMP_(ULONG)
CStdDbOutput::Release()
{
    if(--m_refs == 0){ 
      delete this;
      return 0;
    }
    return m_refs;
}

STDMETHODIMP_(void)
CStdDbOutput::Printf(char FAR* szFmt, ...)
{
    va_list args;
    char *pn, FAR* pf;
    char rgchFmtBuf[128];
    char rgchOutputBuf[128];

    // copy the 'far' format string to a near buffer so we can use
    // a medium model vsprintf, which only supports near data pointers.
    //
    pn = rgchFmtBuf, pf=szFmt;
    while(*pf != '\0')
      *pn++ = *pf++;
    *pn = '\0';

    va_start(args, szFmt);

    vsprintf(rgchOutputBuf, rgchFmtBuf, args);


#if !OE_MAC
    OutputDebugString(rgchOutputBuf);
#else
    strcat(rgchOutputBuf, ";g");
    DebugStr((const unsigned char *) c2pstr(rgchOutputBuf));
#endif

}

STDMETHODIMP_(void)
CStdDbOutput::Assertion(
    BOOL cond,
    char FAR* szExpr,
    char FAR* szFile,
    UINT uLine,
    char FAR* szMsg)
{
    if(cond)
      return;

    // following is from compobj.dll (ole2)
    // FnAssert(szExpr, szMsg, szFile, uLine);
    DebAssert(0, szMsg);
}

#ifdef __cplusplus
extern "C" {
#endif

/***********
* GetDebIMalloc()
*
* Purpose : Creates a debug version of IMalloc
*
**************************************************************************/
BOOL GetDebIMalloc(IMalloc FAR* FAR* ppmalloc)
{
    // For Debug version we want to provide our own implementation of
    // IMalloc.  But for release version we use the default.
    HRESULT hresult;
    IMalloc FAR* pmalloc;

    pmalloc = NULL;
    hresult = CreateDbAlloc(DBALLOC_NONE, NULL, &pmalloc);

    if (hresult != NOERROR)
      return FALSE;

    *ppmalloc = pmalloc;

    return TRUE;
}

#ifdef __cplusplus
}
#endif


/***
* HugeAlloc
*
* Purpose:
*   Allocate a system memblock of given size and return its handle.
*   Note: on Win16 dereferences handle and produce 32-bit
*	   address = selector:offset=0.
*
* Inputs:
*   bch     Allocation request.  Can be >64K.
*
* Outputs:
*   Returns an HSYS.  NULL if unsuccessful.
*
*****************************************************************************/

VOID FAR * HugeAlloc(DWORD bch)
{
#if OE_WIN16

    VOID FAR *pv;
    HANDLE hMem;

    if (((hMem = GlobalAlloc(GMEM_MOVEABLE, bch)) == NULL)) {
      return NULL;
    }
    else if ((pv = (VOID FAR *) GlobalLock(hMem)) == NULL) {
      return NULL;
    }
    else {
      return (VOID FAR *)pv;
    }

#elif OE_MACNATIVE

    Handle hMemBlock;
    THz    pCurrZone;
    OSErr  oserr;

    //--------------------------------------------------------
    //	    The following is a work-around to our bogus code that
    //	    caches pointers to moveable memory.  The basic idea
    // is that we allocate all such memory out of a sub-heap
    // inside the host application (or inside our own heap when
    // a .DLL).  The zone is switched in briefly to allocate,
    // then switched back to avoid the OS or CODE seg loads
    // from allocating there-in.
    //
    //	    The practice of caching pointers to moveable memory on
    // the mac should be fixed to gain optimal use of available
    // memory.	The sub-heap scheme pools available memory
    // and keeps it from being used for loading code when data
    // is small.   Visa-versa, we cannot flush more code out
    // when the data expands to the heap limit. 		    (jwc)
    //--------------------------------------------------------

    DebAssert(g_pOBZone != NULL, "OB Zone used before being allocated.");
    pCurrZone = GetZone();	   // save current heap zone.
    SetZone(g_pOBZone); 		   // set to OB's zone.

    // Allocate moveable mac memblock.
    hMemBlock = NewHandle(bch);
    // get the error before calling SetZone. 'Cos  SetZone can
    // change the memory error.
    oserr = MemError();

    SetZone(pCurrZone); 		   // always restore zone.

    if (oserr) {
      return NULL;
    }
    else {
      return (HSYS)hMemBlock;
    }

#elif OE_MAC

    Handle hMemBlock;
    OSErr  oserr;

    // Allocate moveable mac memblock.
    hMemBlock = NewHandle(bch);
    oserr = MemError();
    if (oserr) {
      return NULL;
    }
    else {
      return (VOID FAR*)hMemBlock;
    }

#elif OE_WIN32

    VOID FAR *pv = NULL;

    return (VOID FAR *)VirtualAlloc(pv, bch, MEM_COMMIT, PAGE_READWRITE);

#else
#error Bad OE
#endif
}


/***
* HsysReallocHsys
*
* Purpose:
*   Reallocate a  system memblock given handle to new size.
*   Shrinking won't move block.
*
* Inputs:
*   hsys    Handle to sys memblock they want to realloc.
*   bchNew  New size they want.   Can be >64K.
*
* Outputs:
*   Returns an HSYS.  NULL if unsuccessful.
*
*****************************************************************************/

VOID FAR *HugeRealloc(VOID FAR *pv, DWORD  bchNew)
{
#if OE_WIN16  // TEMPORARY
#if OE_WIN16

    HANDLE hMem, hMemNew;
    VOID FAR *pvNew;
    USHORT usSel;
    DWORD dwMem;
    DWORD dwNewSize = bchNew;
#if ID_DEBUG
    ULONG cbOld;
#endif // ID_DEBUG


    // Get selector
    usSel = OOB_SELECTOROF((void FAR*)pv);

    if ((dwMem = GlobalHandle((WORD)usSel)) == NULL) {
      return NULL;
    }
    else {
      // Extract the handle.
      hMem = (HANDLE) LOWORD(dwMem);

#if ID_DEBUG
      // get the size of the old block
      cbOld  = GlobalSize(hMem);
#endif // ID_DEBUG

      if (((hMemNew =
	      GlobalReAlloc(hMem, bchNew, GMEM_MOVEABLE)) == NULL)) {
	return NULL;
      }
      else if ((pvNew = GlobalLock(hMemNew)) == NULL) {
	return NULL;
      }
      else {
	return (VOID FAR *)pvNew;
      }
    }

#elif OE_MACNATIVE

    Handle  hMemBlock;
    THz     pCurrZone;
    OSErr   oserr;
#if ID_DEBUG
    ULONG cbOld;
#endif // ID_DEBUG

    hMemBlock = (Handle)hsys;

#if ID_DEBUG
      // get the size of the old block
      cbOld  = GetHandleSize(hMemBlock);
#endif // ID_DEBUG

    pCurrZone = GetZone();	       // save current zone
    SetZone(HandleZone((Handle)hsys)); // must set proper zone or
    SetHandleSize(hMemBlock, bchNew);  //  handle will likely
				       //  jump to curr zone if it moves.
    oserr = MemError();
    SetZone(pCurrZone); 	       // restore current zone.


    if (oserr == memFullErr) {
      // Out of memory
      return NULL;
    }

    DebAssert ((MemError() != nilHandleErr),
      "HsysReallocHsys: NIL master pointer ");

    DebAssert ((MemError() != memWZErr),
      "HsysReallocHsys: Attempt to operate on free Block");

    // anything else would be an undocumented error
    DebAssert (MemError() == noErr,
      "HsysReallocHsys: undocumented Mac error");

    return (VOID FAR *)hMemBlock;

#elif OE_MAC

    Handle  hMemBlock;
    OSErr   oserr;
#if ID_DEBUG
    ULONG cbOld;
#endif // ID_DEBUG

    hMemBlock = (Handle)hsys;

#if ID_DEBUG
      // get the size of the old block
      cbOld  = GetHandleSize(hMemBlock);
#endif // ID_DEBUG

    SetHandleSize(hMemBlock, bchNew);  //  realloc
    oserr = MemError();

    if (oserr == memFullErr) {
      // Out of memory
      return NULL;
    }

    DebAssert ((MemError() != nilHandleErr),
      "HsysReallocHsys: NIL master pointer ");

    DebAssert ((MemError() != memWZErr),
      "HsysReallocHsys: Attempt to operate on free Block");

    // anything else would be an undocumented error
    DebAssert (MemError() == noErr,
      "HsysReallocHsys: undocumented Mac error");

    return (HSYS)hMemBlock;

#elif OE_WIN32
    // UNDONE...
    DebHalt("HsysReallocHsys: UNDONE for OE_WIN32");
    return NULL;
#else
#error Bad OE
#endif
#endif // TEMPORARY
    return NULL;
}




/***
* FreeHsys
*
* Purpose:
*   Free the sys memblock given a handle.
*   Implementation:
*    On Win16, get selector part of hsys,
*     get its handle, unlock and finally free.
*    On Mac: Just use DisposHandle
*
* Inputs:
*   hsys    Handle to memblock they want to free.
*
* Outputs:
*   Returns NULL if successful, otherwise on failure
*    returns the input param.
*
*****************************************************************************/

VOID FAR *HugeFree(VOID FAR *  pv)
{
#if OE_WIN16

    HANDLE hMem;
    DWORD dwMem;
    USHORT usSel = OOB_SELECTOROF((VOID FAR *)pv);

    dwMem = GlobalHandle((WORD)usSel);
    if (dwMem == NULL) {
      // error
      return pv;
    }
    else {
      hMem = (HANDLE) LOWORD(dwMem);
      GlobalUnlock(hMem);   // Can't fail cos nondiscardable.
      if (GlobalFree(hMem) != NULL) {
	// error
	return pv;
      }
      else {
	// ok
	return NULL;
      }
    }

#elif OE_MACNATIVE
	 THz	pCurrZone;
#if ID_DEBUG
	 OSErr	oserr;
#endif

    pCurrZone = GetZone();					   // save current zone
    SetZone(HandleZone((Handle)hsys));		   // must set to proper zone to correctly update free list.

    DisposHandle((Handle)hsys);

#if ID_DEBUG
    oserr = MemError(); 			    // SetZone() will destroy MemError() result.
#endif

    SetZone(pCurrZone); 						   // restore zone.

    DebAssert (oserr != memWZErr,
      "FreeHsys: attempt to operate on already free block.");

    DebAssert(oserr == noErr,
      "FreeHsys: unexpected error.");

    return NULL;

#elif OE_MAC
    HSYS hsys = (HSYS) pv;
#if ID_DEBUG
	 OSErr	oserr;
#endif

    DisposHandle((Handle)hsys);

#if ID_DEBUG
    oserr = MemError(); 			    // SetZone() will destroy MemError() result.

    DebAssert (oserr != memWZErr,
      "FreeHsys: attempt to operate on already free block.");

    DebAssert(oserr == noErr,
      "FreeHsys: unexpected error.");
#endif

    return NULL;

#elif OE_WIN32
    // UNDONE...
    DebAssert(FALSE, "FreeHsys: UNDONE for OE_WIN32");
    return NULL;
#else
#error Bad OE
#endif // OE_WIN16
    return NULL;

}

#endif //DEBUG
