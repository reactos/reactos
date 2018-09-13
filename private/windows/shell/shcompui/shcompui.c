///////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  FILE: SHCOMPUI.C
//
//  DESCRIPTION:
//
//    This module provides the code for supporting NTFS file compression
//    in the NT Explorer user interface.  There are two interfaces to the
//    compression features.
//
//    The first interface is a shell context menu extension that adds
//    the options "Compress" and "Uncompress" to the context/file menu of
//    an object that has registered the extension.  This interface
//    uses the standard shell extension protocol of QueryContextMenu,
//    InvokeCommand etc.  InvokeCommand calls ShellChangeCompressionAttribute()
//    to do the actual compression/uncompression.
//
//    The shell extension CLSID is {764BF0E1-F219-11ce-972D-00AA00A14F56}
//    and is represented by the symbol CLSID_CompressMenuExt.
//
//    The second interface is provided for handling compression
//    requests through object property pages.  Property page action
//    code calls ShellChangeCompressionAttribute( ).
//
//    This way, through either interface, compression appears the same
//    to the user.
//
//    Note that a good portion of the actual compression code was taken
//    from the WinFile implementation.  Some changes were made to
//    eliminate redundant code and to produce the desired Explorer
//    compression behavior.
//
//    By convention, the comment string "BUGBUG" indicates a problem that
//    should be investigated and resolved.  The comment string "WARNING"
//    points out areas that are sensitive to maintenance activity.
//
//    This module is applicable only to the NT version of the shell.
//
//    REVISIONS:
//
//    Date       Description                                         Programmer
//    ---------- --------------------------------------------------- ----------
//    09/15/95   Initial creation.                                   brianau
//    09/20/95   Incorporated changes from 1st code review.          brianau
//    10/02/95   Added SCCA context structure.                       brianau
//               Changed all __TEXT() macros to TEXT()
//    10/13/95   Removed function Int64ToString and moved it to      brianau
//               util.c.
//    02/22/96   Check for shift-key before adding context menu      brianau
//               items.  No shift key, no items.
//               Also added call to SHChangeNotify to notify shell
//               that compression attribute changed on each file.
//    03/20/96   Added invalidation of drive type flags cache.       brianau
//               Replaced imported function declarations with
//               #include <shellprv.h>
//
///////////////////////////////////////////////////////////////////////////////
#ifdef WINNT
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlobjp.h>
#define INITGUID
#include <initguid.h>
#include "resids.h"     // SHCOMPUI Resource IDs.
#include "debug.h"      // DbgOut and ASSERT.
#include "shcompui.h"

#define Assert(f)
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

//
// Debug message control.
//
//#define TRACE_SHEXT 1        // Un-comment for shell extension tracing.
//#define TRACE_DLL   1        // Un-comment for DLL load/unload tracing.
//#define TRACE_COMPRESSION  1 // Un-comment for compression code tracing.
//#define SIM_DISK_FULL 1      // Un-comment to test disk-full condition.

//
// Define context menu position offsets for each option.
//
#define MENUOFS_FIRST      0     // For index range checking.
#define MENUOFS_COMPRESS   0     // Command index for "Compress" menu opt..
#define MENUOFS_UNCOMPRESS 1     // Command index for "Uncompress" menu opt.
#define MENUOFS_LAST       1     // For index range checking.

//
// Return values for compression confirmation dialog.
//
#define COMPRESS_CANCELLED 0     // User pressed Cancel.
#define COMPRESS_SUBSNO    1     // User pressed OK without box checked.
#define COMPRESS_SUBSYES   2     // User pressed OK with box checked.

//
//  Control values for DisplayCompressProgress( ) and
//  DisplayUncompressProgress( )
//
#define PROGRESS_UPD_FILENAME            1
#define PROGRESS_UPD_DIRECTORY           2
#define PROGRESS_UPD_FILEANDDIR          3
#define PROGRESS_UPD_DIRCNT              4
#define PROGRESS_UPD_FILECNT             5
#define PROGRESS_UPD_COMPRESSEDSIZE      6
#define PROGRESS_UPD_FILESIZE            7
#define PROGRESS_UPD_PERCENTAGE          8
#define PROGRESS_UPD_FILENUMBERS         9
#define PROGRESS_UPD_FINAL              10

//
//  Return values for CompressErrMessageBox routine.
//
#define RETRY_CREATE     1
#define RETRY_DEVIO      2

//
// Some text string and character constants.
// BUGBUG: These should probably come from some locale info.
//
const TCHAR c_szSTAR[]      = TEXT("*");
const TCHAR c_szDOT[]       = TEXT(".");
const TCHAR c_szDOTDOT[]    = TEXT("..");
const TCHAR c_szNTLDR[]     = TEXT("NTLDR");
const TCHAR c_szBACKSLASH[] = TEXT("\\");

#define CH_NULL       TEXT('\0')


//
// String length constants.
//
#define MAX_DLGTITLE_LEN    128             // Max length of dialog title.
#define MAX_MESSAGE_LEN     (_MAX_PATH * 3) // General dialog text message.
#define MAX_MENUITEM_LEN     40             // Max length of context menu item.
#define MAX_CMDVERB_LEN      40             // Max length of cmd verb.

typedef HRESULT (CALLBACK FAR * LPFNCREATEINSTANCE)(LPUNKNOWN pUnkOuter,
        REFIID riid, LPVOID FAR* ppvObject);


static INT_PTR CALLBACK CompressSubsConfirmDlgProc(HWND hDlg, UINT uMsg,
                                WPARAM wParam, LPARAM lParam);

static BOOL DoCompress(HWND hwndParent,
                                LPTSTR DirectorySpec, LPTSTR FileSpec);

static BOOL DoUncompress(HWND hwndParent,
                                LPTSTR DirectorySpec, LPTSTR FileSpec);

static int  CompressErrMessageBox(HWND hwndActive,
                                LPTSTR szFile, PHANDLE phFile);

static INT_PTR CALLBACK CompressErrDialogProc(HWND hDlg, UINT uMsg,
                                WPARAM wParam, LPARAM lParam);

static BOOL OpenFileForCompress(PHANDLE phFile, LPTSTR szFile);

static DWORD FormatStringWithArgs(LPCTSTR pszFormat, LPTSTR pszBuffer,
                                                       DWORD nSize, ...);

static DWORD LoadStringWithArgs(HINSTANCE hInstance, UINT uId,
                                LPTSTR pszBuffer, DWORD nSize, ...);

static VOID CompressProgressYield(void);

static VOID DisplayUncompressProgress(int iType);

static void UncompressDiskFullError(HWND hwndParent, HANDLE hFile);


//
// Structure used to communicate with the compression confirmation dialog.
//
typedef struct {
   BOOL  bCompress;               // TRUE = compress, FALSE = uncompress
   TCHAR szFileName[_MAX_PATH+1]; // File to be acted on.
} CompressionDesc;


//
// Macros for converting between interface and class pointers.
//
#define CMX_OFFSETOF(x)          ((UINT_PTR)(&((CContextMenuExt *)0)->x))
#define PVOID2PCMX(pv,offset)    ((CContextMenuExt *)(((LPBYTE)pv)-offset))
#define PCM2PCMX(pcmx)           PVOID2PCMX(pcmx, CMX_OFFSETOF(ctm))
#define PSEI2PCMX(psei)          PVOID2PCMX(psei, CMX_OFFSETOF(sei))

INT g_cRefThisDll        = 0;       // Reference count for this DLL.
HINSTANCE g_hmodThisDll  = NULL;    // Handle to the DLL.
HANDLE g_hProcessHeap    = NULL;    // Handle to the process heap.
HANDLE g_hdlgProgress    = NULL;    // Operation progress dialog.

TCHAR szMessage[MAX_MESSAGE_LEN+1]; // BUGBUG:  This need not be global.
TCHAR g_szByteCntFmt[10];           // Byte cnt disp fmt str ( "%1 bytes" ).

#define SZ_SEMAPHORE_NAME   TEXT("SHCOMPUI_SEMAPHORE")
HANDLE g_hSemaphore       = NULL;    // Re-entrancy semaphore.

LPSCCA_CONTEXT g_pContext = NULL;   // Ptr to current context structure.

INT g_iRecursionLevel = 0;          // Used to control shell change notifications.

//
//  Global variables to hold the User option information.
//
BOOL g_bDoSubdirectories = FALSE;   // Include all subdirectories ?
BOOL g_bShowProgress     = FALSE;   // Show operation progress dialog ?
BOOL g_bIgnoreAllErrors  = FALSE;   // User wants to ignore all errors ?
BOOL g_bDiskFull         = FALSE;   // Is disk full on uncompression ?

//
//  Global variables to hold compression statistics.
//
LONGLONG g_cTotalDirectories       = 0;
LONGLONG g_cTotalFiles             = 0;

//
// Compression ratio statistics values.
//
unsigned _int64 g_iTotalFileSize       = 0;
unsigned _int64 g_iTotalCompressedSize = 0;

//
// "Current" file and directory names are global.
//
TCHAR  g_szFile[_MAX_PATH + 1];
TCHAR  g_szDirectory[_MAX_PATH + 1];

//
// Directory text control in progress dialog.
//
HDC   g_hdcDirectoryTextCtrl = NULL; // Control handle.
DWORD g_cDirectoryTextCtrlWd = 0;    // Width of control.

//
// Number format locale information.
//
NUMBERFMT g_NumberFormat;
TCHAR g_szDecimalSep[5];
TCHAR g_szThousandSep[5];


//
// Context menu extension GUID generated with GUIDGEN.
//
// {764BF0E1-F219-11ce-972D-00AA00A14F56}
DEFINE_GUID(CLSID_CompressMenuExt,
0x764bf0e1, 0xf219, 0x11ce, 0x97, 0x2d, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x56);

//
// Structure representing the class factory for this in-process server.
//
typedef struct
{
   IClassFactory        cf;       // Pointer to class factory vtbl.
   UINT                 cRef;     // Interface reference counter.
   LPFNCREATEINSTANCE   pfnCI;    // Pointer to instance generator function.
} CClassFactory;

//
// Structure representing the context menu extension.
//
typedef struct
{
   IContextMenu   ctm;            // Pointer to context menu interface.
   IShellExtInit  sei;            // Pointer to shell extension init interface.
   UINT           cRef;           // Interface reference counter.
   STGMEDIUM      medium;         // OLE data xfer storage medium descriptor.
   INT            cSelectedFiles; // Cnt of files selected. Must be signed.
   BOOL           bDriveSelected; // Are drives selected ?
   LPDATAOBJECT   pDataObj;       // Saved pointer to Data Object.
} CContextMenuExt;


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: PathRemoveTheBackslash
//
// DESCRIPTION:
//
//    Removes trailing backslash from path string if it exists.
//    This code was cut directly from path.c in shell32.dll and
//    renamed from PathRemoveBackslash to avoid linkage naming
//    conflicts.
//
// in:
//  lpszPath    (A:\, C:\foo\, etc)
//
// out:
//  lpszPath    (A:\, C:\foo, etc)
//
// returns:
//  ponter to NULL that replaced the backslash
//  or the pointer to the last character if it isn't a backslash.
//
///////////////////////////////////////////////////////////////////////////////
LPTSTR WINAPI PathRemoveTheBackslash(LPTSTR lpszPath)
{
  int len = lstrlen(lpszPath)-1;
  if (IsDBCSLeadByte(*((LPSTR)CharPrev(lpszPath,lpszPath+len+1))))
      len--;

  if (!PathIsRoot(lpszPath) && lpszPath[len] == TEXT('\\'))
      lpszPath[len] = TEXT('\0');

  return lpszPath + len;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: FileAttribString
//
// DESCRIPTION:
//
//    Formats a file's attribute word into a string of characters.
//    Used for program tracing and debugging only.
//
// ARGUMENTS:
//
//    dwAttrib
//       Attribute bits to be decoded.
//
//    pszDest
//       Address of destination string.
//       String must be at least 8 characters long.
//
// RETURNS:
//
//    Pointer to destination string.
//
///////////////////////////////////////////////////////////////////////////////
#ifdef TRACE_COMPRESSION
static LPTSTR FileAttribString(DWORD dwAttrib, LPTSTR pszDest)
{
   if (dwAttrib != (DWORD)-1)
   {
      wsprintf(pszDest, TEXT("%c%c%c%c%c%c%c"),
                  dwAttrib & FILE_ATTRIBUTE_ARCHIVE    ? TEXT('A') : TEXT('.'),
                  dwAttrib & FILE_ATTRIBUTE_COMPRESSED ? TEXT('C') : TEXT('.'),
                  dwAttrib & FILE_ATTRIBUTE_DIRECTORY  ? TEXT('D') : TEXT('.'),
                  dwAttrib & FILE_ATTRIBUTE_HIDDEN     ? TEXT('H') : TEXT('.'),
                  dwAttrib & FILE_ATTRIBUTE_NORMAL     ? TEXT('N') : TEXT('.'),
                  dwAttrib & FILE_ATTRIBUTE_READONLY   ? TEXT('R') : TEXT('.'),
                  dwAttrib & FILE_ATTRIBUTE_SYSTEM     ? TEXT('S') : TEXT('.'));
   }
   else
     lstrcpy(pszDest, TEXT("INVALID"));

   return pszDest;
}
#endif


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: ChgDllRefCnt
//
// DESCRIPTION:
//
//    Adds reference count tracking to the incrementing and decrementing of the
//    module reference count.
//
// ARGUMENTS:
//
//    n
//       Should be +1 or -1.
//
// RETURNS:
//
//    Nothing.
//
///////////////////////////////////////////////////////////////////////////////
__inline void ChgDllRefCnt(INT n)
{
   ASSERT(g_cRefThisDll >= 0 && g_cRefThisDll+(n) >= 0);

   g_cRefThisDll += (n);

#ifdef TRACE_DLL
   DbgOut(TEXT("SHCOMPUI: ChgDllRefCnt. Count = %d"), g_cRefThisDll);
#endif
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CClassFactory_QueryInterface
//
// DESCRIPTION:
//
//    Class factory support for IUnknown::QueryInterface( ).
//    Queries class factory object for a specific interface.
//
// ARGUMENTS:
//
//    pcf
//       Pointer to class factory interface.
//
//    riid
//       Reference to ID of interface being requested.
//
//    ppvOut
//       Destination for address of vtable for requested interface.
//
// RETURNS:
//
//    NOERROR        = Success.
//    E_NOINTERFACE  = Requested interface not supported.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClassFactory_QueryInterface(IClassFactory *pcf, REFIID riid,
                                                              LPVOID *ppvOut)
{
   CClassFactory *this = IToClass(CClassFactory, cf, pcf);
   HRESULT hResult     = E_NOINTERFACE;

   ASSERT(NULL != this);
   ASSERT(NULL != ppvOut);

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CClassFactory::QueryInterface"));
#endif

   *ppvOut = NULL;

   if (IsEqualIID(riid, &IID_IClassFactory) ||
       IsEqualIID(riid, &IID_IUnknown))
   {
      (LPCLASSFACTORY)*ppvOut = &this->cf;
      this->cRef++;
      hResult = NOERROR;
   }

   return hResult;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CClassFactory_AddRef
//
// DESCRIPTION:
//
//    Class factory support for IUnknown::AddRef( ).
//    Increments object reference count.
//
// ARGUMENTS:
//
//    pcf
//       Pointer to class factory interface.
//
// RETURNS:
//
//    Returns object reference count after it is incremented.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CClassFactory_AddRef(IClassFactory *pcf)
{
   CClassFactory *this = IToClass(CClassFactory, cf, pcf);

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CClassFactory::AddRef"));
#endif

   ASSERT(NULL != this);

   return ++this->cRef;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CClassFactory_Release
//
// DESCRIPTION:
//
//    Class factory support for IUnknown::Release( ).
//    Decrements object reference count.
//    Deletes object from memory when reference count reaches 0.
//
// ARGUMENTS:
//
//    pcf
//       Pointer to class factory interface.
//
// RETURNS:
//
//    Returns object reference count after it is incremented.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CClassFactory_Release(IClassFactory *pcf)
{
   CClassFactory *this = IToClass(CClassFactory, cf, pcf);
   ULONG refCnt = 0;

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CClassFactory::Release"));
#endif

   ASSERT(NULL != this);

   if ((refCnt = --this->cRef) == 0)
   {
      LocalFree((HLOCAL)this);
      ChgDllRefCnt(-1);
   }

   return refCnt;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CClassFactory_CreateInstance
//
// DESCRIPTION:
//
//    Generates an instance of the class factory.
//
// ARGUMENTS:
//
//    pcf
//       Pointer to class factory interface.
//
//    punkOuter
//       Pointer to outer object's IUnknown interface.  Only used when
//       object aggregation is requested.
//
//    riid
//       Reference to requested interface ID.
//
//    ppv
//       Destination for address of requested interface.
//
// RETURNS:
//
//
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClassFactory_CreateInstance(IClassFactory *pcf, IUnknown *punkOuter,
                                           REFIID riid, LPVOID *ppv)
{
   CClassFactory *this = IToClass(CClassFactory, cf, pcf);
   HRESULT hResult     = CLASS_E_NOAGGREGATION;

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CClassFactory::CreateInstance"));
#endif

   ASSERT(NULL != this);

   if (NULL == punkOuter)
   {
      hResult = this->pfnCI(punkOuter, riid, ppv);
   }

   return hResult;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CClassFactory_LockServer
//
// DESCRIPTION:
//
//    Explicitly locks the server from unloading regardless of the module
//    reference count.
//    Not used for DLL servers.  Therefore, this implementation is nul.
//
//
// ARGUMENTS:
//    pcf
//       Pointer to class factory object.
//
//    fLock
//       TRUE  = Lock server.
//       FALSE = Unlock server.
//
//
// RETURNS:
//
//    E_NOTIMPL  = Server locking not required for DLL server.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClassFactory_LockServer(IClassFactory *pcf, BOOL fLock)
{
#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CClassFactory::LockServer"));
#endif

   return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CContextMenuExt_QueryInterface
//
// DESCRIPTION:
//
//    Context menu extension support for IUnknown::QueryInterface( ).
//    Queries context menu extension object for a specific interface.
//
// ARGUMENTS:
//
//    pctm
//       Pointer to context menu interface.
//
//    riid
//       Reference to ID of interface being requested.
//
//    ppvOut
//       Destination for address of vtable for requested interface.
//
// RETURNS:
//
//    NOERROR        = Success.
//    E_NOINTERFACE  = Requested interface not supported.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CContextMenuExt_QueryInterface(IContextMenu *pctm, REFIID riid,
                                                                LPVOID *ppvOut)
{
   CContextMenuExt *this = IToClass(CContextMenuExt, ctm, pctm);
   HRESULT hResult       = NOERROR;

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CContextMenuExt::QueryInterface"));
#endif

   ASSERT(NULL != this);
   ASSERT(NULL != ppvOut);

   *ppvOut = NULL;

   if (IsEqualIID(riid, &IID_IContextMenu) || IsEqualIID(riid, &IID_IUnknown))
   {
      (IContextMenu *)*ppvOut = &this->ctm;
      this->cRef++;
   }
   else if (IsEqualIID(riid, &IID_IShellExtInit))
   {
      (IShellExtInit *)*ppvOut = &this->sei;
      this->cRef++;
   }
   else
      hResult = E_NOINTERFACE;

   return hResult;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CContextMenuExt_AddRef
//
// DESCRIPTION:
//
//    Context menu extension support for IUnknown::AddRef( ).
//    Increments object reference count.
//
// ARGUMENTS:
//
//    pctm
//       Pointer to context menu interface.
//
// RETURNS:
//
//    Returns object reference count after it is incremented.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CContextMenuExt_AddRef(IContextMenu *pctm)
{
   CContextMenuExt *this = IToClass(CContextMenuExt, ctm, pctm);

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CContextMenuExt::AddRef"));
#endif

   ASSERT(NULL != this);

   return ++this->cRef;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CContextMenuExt_Cleanup
//
// DESCRIPTION:
//
//    Does the cleanup associated with a previous IShellExtInit_Initialize call
//
// ARGUMENTS:
//
//    this
//       Pointer to context menu extension
//
// RETURNS:
//
//    -nothing-
//
///////////////////////////////////////////////////////////////////////////////
void CContextMenu_Cleanup( CContextMenuExt *this )
{
      if (this->pDataObj)
      {
         this->pDataObj->lpVtbl->Release(this->pDataObj);
      }
      //
      // Now release the stgmedium (BUGBUG - Replace this with OLE's ReleaseStgMedium
      //
      if (this->medium.pUnkForRelease)
      {
         this->medium.pUnkForRelease->lpVtbl->Release(this->medium.pUnkForRelease);
      }
      else
      {
          switch(this->medium.tymed)
          {
             case TYMED_HGLOBAL:
                GlobalFree(this->medium.hGlobal);
                break;

             case TYMED_ISTORAGE: // depends on pstm/pstg overlap in union
             case TYMED_ISTREAM:
                this->medium.pstm->lpVtbl->Release(this->medium.pstm);
                break;

             default:
                Assert(0);  // unknown type
          }
      }
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CContextMenuExt_Release
//
// DESCRIPTION:
//
//    Context menu extension support for IUnknown::Release( ).
//    Decrements object reference count.
//    Deletes object from memory when reference count reaches 0.
//
// ARGUMENTS:
//
//    pctm
//       Pointer to context menu interface.
//
// RETURNS:
//
//    Returns object reference count after it is decremented.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CContextMenuExt_Release(IContextMenu *pctm)
{
   CContextMenuExt *this = IToClass(CContextMenuExt, ctm, pctm);
   ULONG refCnt = 0;

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CContextMenuExt::Release"));
#endif

   ASSERT(NULL != this);

   if ((refCnt = --this->cRef) == 0)
   {
      CContextMenu_Cleanup(this);
      LocalFree((HLOCAL)this);

      ChgDllRefCnt(-1);
   }

   return refCnt;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CContextMenuExt_QueryContextMenu
//
// DESCRIPTION:
//
//    Called by NT Shell when requesting menu option text and command numbers.
//
// ARGUMENTS:
//
//    pctm
//       Pointer to context menu interface.
//
//    hMenu
//       Handle to context menu to be modified.
//
//    indexMenu
//       Index where first menu item may be inserted.
//
//    idCmdFirst
//       Lower bound of available menu command IDs.
//
//    idCmdLast
//       Upper bound of available menu command IDs.
//
//    uFlags
//       Flag indicating context of function call.
//
// RETURNS:
//
//    Returns the number of menu items added (excluding separators).
//
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CContextMenuExt_QueryContextMenu(IContextMenu *pctm, HMENU hMenu,
                 UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
   CContextMenuExt *this = IToClass(CContextMenuExt, ctm, pctm);
   BOOL bDisableCompress        = FALSE;
   BOOL bDisableUncompress      = FALSE;
   BOOL bDirectorySelected      = FALSE;
   BOOL bNonCompressibleVol     = FALSE;
   DWORD dwAttribTalley         = 0;
   INT i                        = 0;
   INT cMenuItemsAdded          = 0;
   HCURSOR hCursor              = NULL;
   DRAGINFO di;                      // Drag information.
   LPTSTR pDragFileName = NULL;      // Ptr into list of drag info names.
   LPTSTR pNextName = NULL;          // Lookahead pointer into name list.

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CContextMenuExt::QueryContextMenu"));
#endif

   ASSERT(NULL != this);


   //
   // Only allow addition of Compress/Uncompress when user is pressing
   // shift key.
   //
   // cSelectedFiles will be -1 if user selected only a virtual object.
   // i.e. Control Panel, Printer etc.
   //
   if (GetAsyncKeyState(VK_SHIFT) >= 0 || this->cSelectedFiles <= 0)
      return 0;

   //
   // Determine if we should hide both of the context menu items or
   // disable one of them.
   // If no selected device supports compression, HIDE both items.
   // If a directory is selected, show both menu items.
   // If a directory is not in the selected list, disable a menu item
   // if all of the selected files have the same compression state.
   // i.e.:  All items compressed - disable the "Compress" item.
   // A mix of compression states activates both items.
   //

   dwAttribTalley = 0;   // Attribute talley mask.

   //
   // Attribute talley mask bits.
   //
#define TALLEY_UNCOMPRESSED 0x0001  // Object is an uncompressed file.
#define TALLEY_COMPRESSED   0x0002  // Object is a compressed file.
#define TALLEY_DIRECTORY    0x0004  // Object is a directory.
#define TALLEY_NOCOMPSUPT   0x0008  // At least 1 file from FAT/HPFS drive.

   //
   // Display the hourglass cursor so that the user knows something is
   // happening.  This loop can take a long time on large selections.
   //
   if (hCursor = LoadCursor(NULL, IDC_WAIT))
      hCursor = SetCursor(hCursor);
   ShowCursor(TRUE);

   //
   // Determine if the user has selected drives.
   // Since you can't select a combination of drive(s) and folders/files,
   // if there are ANY drives selected, the first one must be a drive.
   //
   {
      TCHAR szFileName[_MAX_PATH + 1];
      TCHAR szRootName[_MAX_PATH + 1];

      DragQueryFile((HDROP)this->medium.hGlobal, 0, szFileName,
                                                    ARRAYSIZE(szFileName));
      lstrcpy(szRootName, szFileName);
      PathStripToRoot(szRootName);
      this->bDriveSelected = (lstrcmpi(szRootName, szFileName) == 0);
   }

   //
   // Get list of file names selected.
   // The string contains nul-terminated names.
   // The entire list is terminated with a double-nul.
   //
   // BUGBUG: We need to write ANSI/UNICODE thunking layer for
   //         DragQueryInfo().
   //
   di.uSize = sizeof(DRAGINFO);
   DragQueryInfo((HDROP)this->medium.hGlobal, &di);
   pDragFileName = pNextName = di.lpFileList;

   for (i = 0; i < this->cSelectedFiles; i++)
   {
      DWORD dwFlags      = 0;
      DWORD dwAttrib     = 0;

      //
      // Set bits in the attribute "talley" word.  This signals the existence of
      // each desired quantity for all selected files.
      //
      if ((dwAttrib = GetFileAttributes(pDragFileName)) != (DWORD)-1)
      {
         if ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
            dwAttribTalley |= TALLEY_DIRECTORY;

         if ((dwAttrib & FILE_ATTRIBUTE_COMPRESSED))
            dwAttribTalley |= TALLEY_COMPRESSED;
         else
            dwAttribTalley |= TALLEY_UNCOMPRESSED;
      }

      //
      // Save pointer to next name.
      //
      pNextName += lstrlen(pNextName) + 1;

      if (!(dwAttribTalley & TALLEY_NOCOMPSUPT))
      {
         //
         // Do we have at least one drive that DOES NOT support compression?
         // If so, we don't show the menu items.  This provides the INTERSECTION
         // of capabilities for the selected files per the UI design guide.
         // Note that if GetVolumeInformation fails for a drive, the drive is empty
         // and is therefore uncompressible.
         //
         PathStripToRoot(pDragFileName);
         // GetVolumeInformation requires a trailing backslash.  Append
         // one if this is a UNC path.
         if (PathIsUNC(pDragFileName))
         {
             lstrcat(pDragFileName, c_szBACKSLASH);
         }
         if (GetVolumeInformation(pDragFileName, NULL, 0, NULL, NULL, &dwFlags, NULL, 0))
            dwAttribTalley |= (dwFlags & FS_FILE_COMPRESSION) == 0 ?
                                                TALLEY_NOCOMPSUPT : 0;
         else
            dwAttribTalley |= TALLEY_NOCOMPSUPT;
      }

      //
      // If all of the flag bits are set, no need to check more files.
      // We have all the info we need to properly configure the UI.
      //
      if ( (dwAttribTalley & ((DWORD)-1) ) == (TALLEY_DIRECTORY    |
                                               TALLEY_COMPRESSED   |
                                               TALLEY_UNCOMPRESSED |
                                               TALLEY_NOCOMPSUPT))
      {
         break;
      }

      //
      // Advance name pointer to next name.
      //
      pDragFileName = pNextName;
   }

   //
   // Free the file name list we got through DragQueryInfo().
   //
   if (di.lpFileList)
      SHFree(di.lpFileList);

   //
   // Convert the settings of the talley flag bits to more meaningful names.
   //
   bNonCompressibleVol = (dwAttribTalley & TALLEY_NOCOMPSUPT) != 0;
   bDirectorySelected  = (dwAttribTalley & TALLEY_DIRECTORY) != 0;

   if (!bDirectorySelected)
   {
      bDisableUncompress = (dwAttribTalley &
            (TALLEY_COMPRESSED | TALLEY_UNCOMPRESSED)) == TALLEY_UNCOMPRESSED;
      bDisableCompress   = (dwAttribTalley &
            (TALLEY_COMPRESSED | TALLEY_UNCOMPRESSED)) == TALLEY_COMPRESSED;
   }

   switch(uFlags & 0x0F)  // Upper 28 bits are reserved.
   {
      case CMF_EXPLORE:
         //
         // Win32 SDK says we should only get this flag bit set when the user
         // has selected an object in the left pane of the Explorer. This is a
         // doc bug in the SDK verified by "satona".  We get it via selection
         // in either pane.
         //

      case CMF_NORMAL:

         if (!bNonCompressibleVol)
         {
            INT cchLoaded = 0;
            TCHAR szMenuItem[MAX_MENUITEM_LEN + 1];

            //
            // BUGBUG: Regarding item separators; can we always count on there being
            //         an item above and below our new items?  If not, we're
            //         in danger of adding a separator at the top or bottom of
            //         the menu.  BobDay says we'll always have something above
            //         and below us.
            //
            cchLoaded = LoadString(g_hmodThisDll,
                                   bDirectorySelected ? IDS_COMPRESS_MENUITEM_ELLIP :
                                                        IDS_COMPRESS_MENUITEM,
                                   szMenuItem, ARRAYSIZE(szMenuItem));
            ASSERT(cchLoaded > 0);

            InsertMenu(hMenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
            InsertMenu(hMenu, indexMenu++, MF_STRING | MF_BYPOSITION | (bDisableCompress ? MF_GRAYED : 0),
                                           idCmdFirst + MENUOFS_COMPRESS, szMenuItem);
            cMenuItemsAdded++;
            cchLoaded = LoadString(g_hmodThisDll,
                                   bDirectorySelected ? IDS_UNCOMPRESS_MENUITEM_ELLIP :
                                                        IDS_UNCOMPRESS_MENUITEM,
                                   szMenuItem, ARRAYSIZE(szMenuItem));
            ASSERT(cchLoaded > 0);

            InsertMenu(hMenu, indexMenu++, MF_STRING | MF_BYPOSITION | (bDisableUncompress ? MF_GRAYED : 0),
                                           idCmdFirst + MENUOFS_UNCOMPRESS, szMenuItem);
            cMenuItemsAdded++;
            InsertMenu(hMenu, indexMenu,   MF_SEPARATOR | MF_BYPOSITION, 0, NULL);

         }
         break;

      case CMF_DEFAULTONLY:
      case CMF_VERBSONLY:
         //
         // SDK Docs say Context Menu Extensions should ignore these.
         //
         break;

      default:
         break;
   }

   //
   // Restore original cursor and return number of menu items added.
   //
   if (hCursor)
      SetCursor(hCursor);
   ShowCursor(FALSE);

   return cMenuItemsAdded;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CContextMenuExt_InvokeCommand
//
// DESCRIPTION:
//
//    Called by the shell whenever the user selects one of the registered
//    items on the context menu.
//
//    Or may be called programmatically with one of the following verb
//    strings:
//               "COMPRESS"    to compress files.
//               "UNCOMPRESS"  to uncompress files.
//
//               These verb names are case-sensitive and language-
//               insensitive.
//
// ARGUMENTS:
//
//    pctm
//       Pointer to the context menu interface.
//
//    pici
//       Pointer to the command info structure associated with the selected
//       menu command.
//
// RETURNS:
//
//    NOERROR              = Success
//    OLEOBJ_E_INVALIDVERB = Invalid verb was specified in programatic
//                           invocation.
//    E_FAIL               = User aborted operation or an error occured during
//                           compression/uncompression.  We should have more
//                           descriptive failure return info.  The original
//                           implementation of compression in WinFile only
//                           returned TRUE/FALSE.  In the interest of time,
//                           that "feature" was retained.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CContextMenuExt_InvokeCommand(IContextMenu *pctm,
                                           LPCMINVOKECOMMANDINFO pici)
{
   HRESULT hResult   = NOERROR;
   INT i = 0;
   CContextMenuExt *this = IToClass(CContextMenuExt, ctm, pctm);
   BOOL bCompressing = FALSE;
   BOOL bShowUI      = FALSE;
   HWND hwndParent   = NULL;

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CContextMenuExt::InvokeCommand"));
#endif

   ASSERT(NULL != pici);

   //
   // Caller can request that no UI be activated.
   //
   bShowUI = (pici->fMask & CMIC_MASK_FLAG_NO_UI) == 0;

   //
   // Parent all displayed dialogs with the handle passed in from the caller.
   // Use the desktop as a default if one wasn't provided.
   //
   if ((hwndParent = pici->hwnd) == NULL)
      hwndParent = GetDesktopWindow();

   if (HIWORD(pici->lpVerb) == 0)
   {
      //
      // InvokeCommand was called through the context menu extension protocol.
      //
      bCompressing = LOWORD(pici->lpVerb) == MENUOFS_COMPRESS;
   }
   else
   {
      //
      // InvokeCommand was called programatically.
      // If lpVerb is "COMPRESS", compress the file(s).
      // If lpVerb is "UNCOMPRESS", uncompress the file(s).
      // Otherwise, do nothing and return OLE error code.
      //
      TCHAR szValidVerb[MAX_CMDVERB_LEN + 1];
      TCHAR szVerb[MAX_CMDVERB_LEN + 1];
      INT cchLoaded = 0;

      //
      // Convert verb string to unicode.
      //
      MultiByteToWideChar(CP_ACP, 0L, pici->lpVerb, -1, szVerb, ARRAYSIZE(szVerb));

      bCompressing = FALSE;

      cchLoaded = LoadString(g_hmodThisDll, IDS_COMPRESS_CMDVERB, szValidVerb,
                                                        ARRAYSIZE(szValidVerb));
      ASSERT(cchLoaded > 0);

      if (!lstrcmp(szValidVerb, szVerb))
      {
         bCompressing = TRUE;
      }
      else
      {
         cchLoaded = LoadString(g_hmodThisDll, IDS_UNCOMPRESS_CMDVERB, szValidVerb,
                                                            ARRAYSIZE(szValidVerb));
         ASSERT(cchLoaded > 0);
         if (!lstrcmp(szValidVerb, szVerb))
         {
            bCompressing = FALSE;
         }
         else
         {
            //
            // Verb isn't COMPRESS or UNCOMPRESS.
            //
            hResult = OLEOBJ_E_INVALIDVERB;
         }
      }
   }

   //
   // Do the compression/uncompression.
   //
   if (hResult == NOERROR)
   {
      SCCA_CONTEXT Context;        // Compression context structure.

      SCCA_CONTEXT_INIT(&Context); // Initialize context.

      for (i = 0; i < this->cSelectedFiles; i++)
      {
         TCHAR szFileName[_MAX_PATH + 1];

         //
         // Get the name of the file to compress.
         //
         DragQueryFile((HDROP)this->medium.hGlobal, i, szFileName, ARRAYSIZE(szFileName));

         //
         // Compress/uncompress file.  Return value of FALSE indicates either an error
         // occured or the user cancelled the operation.  In either case, don't process
         // any more files.
         //
         if (!ShellChangeCompressionAttribute(hwndParent, szFileName, &Context,
                                              bCompressing, bShowUI))
         {
            hResult = E_FAIL;
            break;
         }
      }
   }

   return hResult;
}



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CContextMenuExt_GetCommandString
//
// DESCRIPTION:
//
//    Called by the shell when it wants a text string associated with a menu
//    item.  Status bar text for example or a menu command verb.
//
// ARGUMENTS:
//
//    pctm
//       Pointer to the context menu interface.
//
//    idCmd
//       Integer identifier of the command in question.  The number is the
//       offset of the menu item in the set of added menu items, based 0.
//
//    uFlags
//       Indicates what type of service the shell is requesting.
//
//    pwReserved
//       Unused.
//
//    pszName
//       Destination for menu item character string.  Provided by shell.
//
//    cchMax
//       Max size of destination buffer.  Provided by shell.
//
// RETURNS:
//
//    NOERROR = Success.
//    E_FAIL  = Requested menu idCmd not recognized.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CContextMenuExt_GetCommandString(IContextMenu *pctm, UINT_PTR idCmd,
                                              UINT uFlags, UINT *pwReserved, LPSTR pszName,
                                              UINT cchMax)
{
   CContextMenuExt *this = IToClass(CContextMenuExt, ctm, pctm);
   HRESULT hResult       = NOERROR;

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CContextMenuExt::GetCommandString idCmd = %d"), idCmd);
#endif

   ASSERT(NULL != this);
   ASSERT(NULL != pszName);

   //
   // Start with destination buffer blank.
   //
   if (cchMax > 0)
      pszName[0] = TEXT('\0');


   if (idCmd >= MENUOFS_FIRST && idCmd <= MENUOFS_LAST)
   {
      DWORD *pStrIdArray = NULL;
      BOOL bUnicode      = FALSE;
      DWORD dwCmdVerbIds[]         = { IDS_COMPRESS_CMDVERB,
                                       IDS_UNCOMPRESS_CMDVERB };
      DWORD dwSbarTextIds[]        = { IDS_COMPRESS_SBARTEXT,
                                       IDS_UNCOMPRESS_SBARTEXT };
      DWORD dwSbarTextMultIds[]    = { IDS_COMPRESS_SBARTEXT_M,
                                       IDS_UNCOMPRESS_SBARTEXT_M };
      DWORD dwSbarDrvTextIds[]     = { IDS_COMPRESS_SBARTEXT_DRV,
                                       IDS_UNCOMPRESS_SBARTEXT_DRV };
      DWORD dwSbarDrvTextMultIds[] = { IDS_COMPRESS_SBARTEXT_DRV_M,
                                       IDS_UNCOMPRESS_SBARTEXT_DRV_M };

      switch(uFlags)
      {
         //
         // Provide help text for menu item.
         //
         case GCS_HELPTEXTW:
         case GCS_HELPTEXTA:
            //
            // If drives selected, use "Drive" strings. Otherwise, use "File"
            // strings. Also address multiplicity.
            //
            if (this->cSelectedFiles == 1)
               pStrIdArray = this->bDriveSelected ? dwSbarDrvTextIds : dwSbarTextIds;
            else
               pStrIdArray = this->bDriveSelected ? dwSbarDrvTextMultIds : dwSbarTextMultIds;

            bUnicode    = uFlags == GCS_HELPTEXTW;
            break;

         //
         // Provide command verb recognized by InvokeCommand( ).
         //
         case GCS_VERBW:
         case GCS_VERBA:
            pStrIdArray = dwCmdVerbIds;
            bUnicode    = uFlags == GCS_VERBW;
            break;

         //
         // Validate that the menu cmd exists.
         //
         case GCS_VALIDATE:
            hResult = NOERROR;
            break;

         default:
            break;
      }

      //
      // If we've identified what array to get the string resource ID from, load
      // the ANSI or UNICODE version of the string related to the command id.
      //
      if (NULL != pStrIdArray)
      {
         INT cchLoaded = 0;

         if (bUnicode)
            cchLoaded = LoadStringW(g_hmodThisDll, *(pStrIdArray + idCmd), (LPWSTR)pszName, cchMax);
         else
            cchLoaded = LoadStringA(g_hmodThisDll, *(pStrIdArray + idCmd), pszName, cchMax);

         ASSERT(cchLoaded > 0);
      }
   }
   else
      hResult = E_FAIL;

   return hResult;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CShellExtInit_QueryInterface
//
// DESCRIPTION:
//
//    Called by the shell to obtain an interface from the shell extension.
//
// ARGUMENTS:
//
//    psei
//       Pointer to the shell extension interface.
//
//    riid
//       Reference to the requested interface ID.
//
//    ppvOut
//       Address of destination for resulting interface pointer.
//
//
// RETURNS:
//
//    NOERROR
//    E_NOINTERFACE
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CShellExtInit_QueryInterface(IShellExtInit *psei, REFIID riid,
                                                               LPVOID *ppvOut)
{
   CContextMenuExt *this = PSEI2PCMX(psei);

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CShellExtInit::QueryInterface"));
#endif

   ASSERT(NULL != this);
   ASSERT(NULL != ppvOut);

   return CContextMenuExt_QueryInterface(&this->ctm, riid, ppvOut);
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CShellExtInit_AddRef
//
// DESCRIPTION:
//
//    Increments the reference count for the shell extension init
//    interface.
//
// ARGUMENTS:
//
//    psei
//       Pointer to the shell extension interface.
//
// RETURNS:
//
//    New reference counter value.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CShellExtInit_AddRef(IShellExtInit *psei)
{
   CContextMenuExt *this = PSEI2PCMX(psei);

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CShellExtInit::AddRef"));
#endif

   ASSERT(NULL != this);

   return CContextMenuExt_AddRef(&this->ctm);
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CShellExtInit_Release
//
// DESCRIPTION:
//
//    Decrements the reference count for the shell extension init
//    interface.  When count reaches 0, the interface object is
//    deleted.
//
// ARGUMENTS:
//
//    psei
//       Pointer to the shell extension interface.
//
// RETURNS:
//
//    New reference counter value.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CShellExtInit_Release(IShellExtInit *psei)
{
   CContextMenuExt *this = PSEI2PCMX(psei);
   ULONG refCnt          = 0;

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CShellExtInit::Release"));
#endif

   ASSERT(NULL != this);

   return CContextMenuExt_Release(&this->ctm);
}


///////////////////////////////////////////////////////////////////////////////
//
// CShellExtInit::Initialize
//
// Called by the shell to initialize the shell extension.
//
// Arguments:
//
//    psei
//       Pointer to the IShellExtInit interface.
//
//    pidlFolder
//       Pointer to item ID list of parent folder.
//
//    lpdobj
//       Pointer to data object containing selected file names.
//
//    hkeyProgID
//       Registry class of the file object that has focus.
//
// RETURNS:
//
//    S_OK
//    E_FAIL
//    E_INVALIDARG
//    E_UNEXPECTED
//    E_OUTOFMEMORY
//    DV_E_LINDEX
//    DV_E_FORMATETC
//    DV_E_TYMED
//    DV_E_DVASPECT
//    OLE_E_NOTRUNNING
//    STG_E_MEDIUMFULL
//
///////////////////////////////////////////////////////////////////////////////
static STDMETHODIMP CShellExtInit_Initialize(IShellExtInit *psei,
                 LPCITEMIDLIST pidlFolder, LPDATAOBJECT lpdobj, HKEY hkeyProgID)
{
   CContextMenuExt *this = PSEI2PCMX(psei);
   FORMATETC fe          = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
   HRESULT hResult       = NOERROR;

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CShellExtInit::Initialize"));
#endif

   ASSERT(NULL != this);

   //
   // According to Win32 SDK, Initialize can be called more than once.
   //
   CContextMenu_Cleanup(this);

   if (NULL != lpdobj)
   {
      this->pDataObj = lpdobj;
      lpdobj->lpVtbl->AddRef(lpdobj);

      //
      // If we are allowed to get data from the data object, save the medium
      // descriptor in our context menu extension object.  We'll use it to
      // iterate through the names of the selected files in
      // CContextMenuExt::InvokeCommand( ).
      //
      hResult = lpdobj->lpVtbl->GetData(lpdobj, &fe, &this->medium);
      if (NOERROR == hResult)
         this->cSelectedFiles = DragQueryFile((HDROP)this->medium.hGlobal, (DWORD)-1, NULL, 0);
      else
         this->cSelectedFiles = 0;
   }
   else
      hResult = E_FAIL;

   return hResult;
}

///////////////////////////////////////////////////////////////////////////////
//                              CLASS VTABLES
///////////////////////////////////////////////////////////////////////////////
#pragma data_seg(".text")

//
// Create the class factory vtbl in a read-only segment.
//
IClassFactoryVtbl c_vtblCClassFactory = {
   CClassFactory_QueryInterface,
   CClassFactory_AddRef,
   CClassFactory_Release,
   CClassFactory_CreateInstance,
   CClassFactory_LockServer
};

//
// Create the context menu extension vtbl in a read-only segment.
//
IContextMenuVtbl c_vtblContextMenuExt = {
   CContextMenuExt_QueryInterface,
   CContextMenuExt_AddRef,
   CContextMenuExt_Release,
   CContextMenuExt_QueryContextMenu,
   CContextMenuExt_InvokeCommand,
   CContextMenuExt_GetCommandString
};

//
// Create the shell extension initialization vtbl in a read-only segment.
//
IShellExtInitVtbl c_vtblShellExtInit = {
   CShellExtInit_QueryInterface,
   CShellExtInit_AddRef,
   CShellExtInit_Release,
   CShellExtInit_Initialize
};

#pragma data_seg()


///////////////////////////////////////////////////////////////////////////////
//
// CContextMenuExt_CreateInstance
//
// Context menu extension instance generator.  Creates a context menu extension
// object and returns a pointer to the requested interface.
//
// Arguments:
//
//    punkOuter
//       Not used for objects that don't support aggregation.  We don't support
//       aggregation so we don't use it.
//
//    riid
//       Reference to the requested interface ID.
//
//    ppvOut
//       Address of the destination for the interface pointer.
//
// RETURNS:
//
//    NOERROR                = Success.
//    E_OUTOFMEMORY          = Can't allocate extension object.
//    E_NOINTERFACE          = Interface not supported.
//    CLASS_E_NOAGGREGATION  = Aggregation not supported.
//
///////////////////////////////////////////////////////////////////////////////
static HRESULT CContextMenuExt_CreateInstance(IUnknown *punkOuter,
                                              REFIID riid, void **ppvOut)
{
   CContextMenuExt *pcmx = NULL;
   HRESULT hResult       = NOERROR;

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CContextMenuExt::CreateInstance"));
#endif

   ASSERT(NULL != ppvOut);

   *ppvOut = NULL;

   if (NULL == punkOuter)
   {
      pcmx = (CContextMenuExt *)LocalAlloc(LPTR, sizeof(CContextMenuExt));
      if (NULL != pcmx)
      {
         pcmx->ctm.lpVtbl = &c_vtblContextMenuExt;  // Context menu ext vtable ptr.
         pcmx->sei.lpVtbl = &c_vtblShellExtInit;    // Shell extention int vtable ptr.
         pcmx->cRef = 0;                            // Initialize reference counter.
         pcmx->medium.tymed = TYMED_NULL;           // Not yet initialized by shell.
         pcmx->medium.hGlobal = (HGLOBAL)NULL;      // Not yet initialized by shell.
         pcmx->medium.pUnkForRelease = NULL;        // Not yet initialized by shell.
         pcmx->cSelectedFiles = 0;                  // Not yet initialized by shell.
         pcmx->bDriveSelected = FALSE;              // No drives selected yet.
         pcmx->pDataObj = NULL;
         hResult = c_vtblContextMenuExt.QueryInterface(&pcmx->ctm, riid, ppvOut);
         ChgDllRefCnt(+1);
      }

      else
         hResult = E_OUTOFMEMORY;
   }
   else
      hResult = CLASS_E_NOAGGREGATION;  // Extension doesn't support aggregation.

   return hResult;
}

///////////////////////////////////////////////////////////////////////////////
//
// CreateClassObject
//
// Creates a class factory object returning a pointer to its IUnknown interface.
//
// Arguments:
//
//    riid
//       Reference to interface on class factory object.
//
//    pfcnCI
//       Pointer to instance creation function.
//       In this application, this is CContextMenuExt_CreateInstance.
//
//    ppvOut
//       Destination for pointer to class factory interface (Vtable).
//
// RETURNS:
//
//    NOERROR        = Success.
//    E_NOINTERFACE  = Interface not supported.
//    E_OUTOFMEMORY  = Can't create class factory object.
//
///////////////////////////////////////////////////////////////////////////////
STDAPI CreateClassObject(REFIID riid, LPFNCREATEINSTANCE pfnCI, LPVOID *ppvOut)
{
   HRESULT hResult = NOERROR;

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: CreateClassObject"));
#endif

   ASSERT(NULL != ppvOut);
   ASSERT(NULL != pfnCI);

   *ppvOut = NULL;                 // Initialize pointer transfer buffer.

   if (IsEqualIID(riid, &IID_IClassFactory))
   {
      //
      // Allocate the class factory structure.
      //
      CClassFactory *pcf = (CClassFactory *)LocalAlloc(LPTR, sizeof(CClassFactory));
      if (NULL != pcf)
      {
         pcf->cf.lpVtbl = &c_vtblCClassFactory;  // Assign ptr to vtbl.
         pcf->cRef++;                            // Increment interface ref count.
         pcf->pfnCI = pfnCI;                     // Assign ptr instance creation proc.
         (IClassFactory *)*ppvOut = &pcf->cf;    // Return ptr to vtbl (interface).
         ChgDllRefCnt(+1);
      }
      else
         hResult = E_OUTOFMEMORY;    // Cannot get or use shell memory allocator interface.
   }
   else
      hResult = E_NOINTERFACE;      // Cannot produce requested interface.

return hResult;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: DllGetClassObject
//
// DESCRIPTION:
//
//    Called by NT Shell to retrieve the interface to the Class factory.
//
// ARGUMENTS:
//
//    rclsid
//       Reference to class ID that identifies the type of object that the
//       class factory will be asked to create.
//
//    riid
//       Reference to interface ID on the class factory object.
//
//    ppvOut
//       Destination location for class factory object pointer after instantiation.
//
// RETURNS:
//
//    NOERROR                   = Success.
//    E_OUTOFMEMORY             = Can't create class factory object.
//    E_NOINTERFACE             = Interface not supported.
//    CLASS_E_CLASSNOTAVAILABLE = Context menu extension not available.
//
///////////////////////////////////////////////////////////////////////////////
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvOut)
{
   HRESULT hResult = CLASS_E_CLASSNOTAVAILABLE;

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: DllGetClassObject"));
#endif

   ASSERT(NULL != ppvOut);

   *ppvOut = NULL;

   //
   // Call the extension-provided creation function corresponding
   // to the class ID of the requested extension.
   //
   if (IsEqualIID(rclsid, &CLSID_CompressMenuExt ))
   {
      hResult = CreateClassObject(riid,
                (LPFNCREATEINSTANCE)CContextMenuExt_CreateInstance, ppvOut);
   }

#ifdef DBG
   if (hResult != NOERROR)
      DbgOut(TEXT("SHCOMPUI: Context menu extension [CLSID_CompressMenuExt] creation failed."));
#endif

   return hResult;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: DllCanUnloadNow
//
// DESCRIPTION:
//
//    Called by NT to determine if DLL can be unloaded.
//
// ARGUMENTS:
//
//    None.
//
// RETURNS:
//
//    S_FALSE   = Can't unload.
//    S_OK      = OK to unload.
//
///////////////////////////////////////////////////////////////////////////////
STDAPI DllCanUnloadNow(void)
{

#ifdef TRACE_SHEXT
   DbgOut(TEXT("SHCOMPUI: DllCanUnloadNow.  Dll reference count = %d"), g_cRefThisDll);
#endif

   ASSERT(g_cRefThisDll >= 0);

   //
   // I test for <= 0 so that the DLL can be unloaded even if the ref
   // count drops below 0 (reference count error).  The preceding
   // ASSERT( ) catches this during development.  This means that
   // the ref counter has to be signed.
   //
   return ResultFromScode((g_cRefThisDll <= 0) ? S_OK : S_FALSE);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: DllMain
//
// DESCRIPTION:
//
//    Dll entry point.
//
///////////////////////////////////////////////////////////////////////////////
int APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   TCHAR szLocaleInfo[20];

   switch(dwReason)
   {
      case DLL_PROCESS_ATTACH:
#ifdef TRACE_DLL
         DbgOut(TEXT("SHCOMPUI: DLL_PROCESS_ATTACH"));
#endif
         g_hmodThisDll  = hInstance;
         g_hProcessHeap = GetProcessHeap();

         DisableThreadLibraryCalls(hInstance);

         //
         // Create/Open semaphore to prevent re-entrancy.
         //
         g_hSemaphore = CreateSemaphore(NULL, 1, 1, SZ_SEMAPHORE_NAME);
         if (GetLastError() == ERROR_ALREADY_EXISTS)
            g_hSemaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SZ_SEMAPHORE_NAME);

         if (NULL == g_hSemaphore)
            return FALSE;       // Can't create/open semaphore object.

         //
         // Prepare number format info for current locale.
         // Used in progress dialog display of 64-bit integers.
         //
         g_NumberFormat.NumDigits    = 0;  // This is locale-insensitive.
         g_NumberFormat.LeadingZero	 = 0;  // So is this.

         GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szLocaleInfo, ARRAYSIZE(szLocaleInfo));
         g_NumberFormat.Grouping       = StrToLong(szLocaleInfo);

         GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, g_szDecimalSep, ARRAYSIZE(g_szDecimalSep));
         g_NumberFormat.lpDecimalSep   = g_szDecimalSep;

         GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, g_szThousandSep, ARRAYSIZE(g_szThousandSep));
         g_NumberFormat.lpThousandSep  = g_szThousandSep;

         GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER, szLocaleInfo, ARRAYSIZE(szLocaleInfo));
         g_NumberFormat.NegativeOrder  = StrToLong(szLocaleInfo);

         break;

      case DLL_PROCESS_DETACH:
#ifdef TRACE_DLL
         DbgOut(TEXT("SHCOMPUI: DLL_PROCESS_DETACH"));
#endif
         CloseHandle(g_hSemaphore);
         break;

      case DLL_THREAD_ATTACH:
#ifdef TRACE_DLL
         DbgOut(TEXT("SHCOMPUI: DLL_THREAD_ATTACH"));
#endif
         break;

      case DLL_THREAD_DETACH:
#ifdef TRACE_DLL
         DbgOut(TEXT("SHCOMPUI: DLL_THREAD_DETACH"));
#endif
         break;

      default:
         break;
   }
   return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: FormatStringWithArgs
//
// DESCRIPTION:
//
//    Formats a text string with variable arguments.
//
// ARGUMENTS:
//
//    pszFormat
//       Address of format text string using %1,%2 etc. format specifiers.
//
//    pszBuffer
//       Address of destination buffer.
//
//    nSize
//       Number of characters in destination buffer.
//
//    ...
//       Variable length list of replacement parameters.
//
// RETURNS:
//
//    Returns number of characters copied to buffer.
//    0 = Error.  GetLastError() if you're interested in why.
//
///////////////////////////////////////////////////////////////////////////////
static DWORD FormatStringWithArgs(LPCTSTR pszFormat, LPTSTR pszBuffer,
                                                       DWORD nSize, ...)
{
   DWORD dwCharCount   = 0;
   va_list args;

   ASSERT(NULL != pszBuffer);
   ASSERT(NULL != pszFormat);

   //
   // Format the resource string by replacing parameters if present.
   //
   va_start(args, nSize);
   dwCharCount = FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                          pszFormat,
                          0,
                          0,
                          pszBuffer,
                          nSize,
                          &args);
   va_end(args);

   return dwCharCount;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: LoadStringWithArgs
//
// DESCRIPTION:
//
//    Formats a resource string with variable arguments.
//
// ARGUMENTS:
//
//    hInstance
//       Instance handle for module containing resource string.
//
//    uId
//       Resource string ID.  String may contain embedded formatting characters
//       for replaceable parameters (i.e. "Delete file %1 ?")
//
//    szBuffer
//       Destination buffer.
//
//    nSize
//       Number of characters in destination buffer.
//
//    ...
//       Variable length list of replacement parameters.
//
// RETURNS:
//
//    Returns number of characters copied to buffer.
//    0 = Error.  GetLastError() if you're interested in why.
//    Function does set last error to E_OUTOFMEMORY on LocalAlloc fail.
//
///////////////////////////////////////////////////////////////////////////////
static DWORD LoadStringWithArgs(HINSTANCE hInstance, UINT uId,
                                           LPTSTR pszBuffer, DWORD nSize, ...)
{
   DWORD dwCharCount = 0;
   LPTSTR pszFormat  = NULL;
   va_list args;

   ASSERT(NULL != pszBuffer);

   //
   // Allocate a buffer for the resource string.
   //
   if ((pszFormat = LocalAlloc(LMEM_FIXED, nSize * sizeof(TCHAR))) != NULL)
   {
      //
      // Load the resource string from the specified module.
      //
      if (LoadString(hInstance, uId, pszFormat, nSize) != 0)
      {
         va_start(args, nSize);
         dwCharCount = FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                                pszFormat,
                                0,
                                0,
                                pszBuffer,
                                nSize,
                                &args);
         va_end(args);
      }
      LocalFree(pszFormat);
   }
   else
      SetLastError((DWORD)E_OUTOFMEMORY);

   return dwCharCount;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CenterWindowInParent
//
// DESCRIPTION:
//
//    Positions a window centered in its parent.
//    This function was taken from WinFile.
//
// ARGUMENTS:
//
//    hwnd
//       Handle of window to be centered.
//
//
// RETURNS:
//
//    Nothing.
//
///////////////////////////////////////////////////////////////////////////////
static VOID CenterWindowInParent(HWND hwnd)
{
    RECT    rect;
    RECT    rectParent;
    HWND    hwndParent;
    LONG    Style;

    //
    //  Get window rect.
    //
    GetWindowRect(hwnd, &rect);

    //
    //  Get parent rect.
    //
    Style = GetWindowLong(hwnd, GWL_STYLE);
    if ((Style & WS_CHILD) == 0)
    {
        hwndParent = GetDesktopWindow();
    }
    else
    {
        hwndParent = GetParent(hwnd);
        if (hwndParent == NULL)
        {
            hwndParent = GetDesktopWindow();
        }
    }
    GetWindowRect(hwndParent, &rectParent);

    //
    //  Center the child in the parent.
    //
    rect.left = rectParent.left + (((rectParent.right - rectParent.left) - (rect.right - rect.left)) >> 1);
    rect.top  = rectParent.top +  (((rectParent.bottom - rectParent.top) - (rect.bottom - rect.top)) >> 1);

    //
    //  Move the child into position.
    //
    SetWindowPos( hwnd,
                  NULL,
                  rect.left,
                  rect.top,
                  0,
                  0,
                  SWP_NOSIZE | SWP_NOZORDER );

    SetForegroundWindow(hwnd);
}



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CompressSubsConfirmDlgProc
//
// DESCRIPTION:
//
//    Message proc for compression UI confirmation dialog.  The dialog is
//    displayed whenever a selected directory is about to be compressed or
//    uncompressed.  The dialog includes a message stating that all files
//    in the selected directory are about to be compressed/uncompressed.
//    Included is a checkbox that may be checked to approve compression/
//    uncompression of all sub-folders.
//
// ARGUMENTS:
//
//    wParam
//       Unused.
//
//    lParam
//       Address of a "Compression Descriptor" (type CompressionDesc).
//       The descriptor contains the name of the file to be compresssed/
//       uncompressed along with a flag value indicating which operation
//       is being performed.
//
//
// RETURNS:
//
//    0  = User pressed Cancel button.  Don't compress this folder.
//          (COMPRESS_CANCELLED)
//    1  = User pressed OK button.  Sub-folder checkbox is unchecked.
//          (COMPRESS_SUBSNO)
//    2  = User pressed OK button.  Sub-folder checkbox is checked.
//          (COMPRESS_SUBSYES)
//
///////////////////////////////////////////////////////////////////////////////
static INT_PTR CALLBACK CompressSubsConfirmDlgProc(HWND hDlg, UINT uMsg,
                                                   WPARAM wParam, LPARAM lParam)
{
   //
   // Dialog message string resource IDs.  Array indexes map directly to values
   // of lParam.
   //
   UINT uDlgTextIds[] = { IDS_UNCOMPRESS_CONFIRMATION, IDS_COMPRESS_CONFIRMATION };
   UINT uCbxTextIds[] = { IDS_UNCOMPRESS_ALSO,         IDS_COMPRESS_ALSO         };
   UINT uActionTextIds[] = { IDS_UNCOMPRESS_ACTION,    IDS_COMPRESS_ACTION       };

   TCHAR szDlgText[40 + _MAX_PATH];         // Dialog text resource string buffer.
   CompressionDesc *cd = (CompressionDesc *)lParam;
   UINT cchLoaded = 0;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         ASSERT(NULL != (void *)lParam);

         //
         // Initialize the "Compress/Uncompress all files in..." message.
         //
         LoadStringWithArgs(g_hmodThisDll, uDlgTextIds[cd->bCompress], szDlgText,
                                ARRAYSIZE(szDlgText), cd->szFileName);
         SetDlgItemText(hDlg, IDC_COMPRESS_CONFIRM_TEXT, szDlgText);

         //
         // Initialize the "This action compresses..." message.
         //
         cchLoaded = LoadString(g_hmodThisDll, uActionTextIds[cd->bCompress],
                                              szDlgText, ARRAYSIZE(szDlgText));
         ASSERT(cchLoaded > 0);
         SetDlgItemText(hDlg, IDC_COMPRESS_ACTION_TEXT, szDlgText);

         //
         // Initialize the "also compress/uncompress subfolders" checkbox message.
         //
         cchLoaded = LoadString(g_hmodThisDll, uCbxTextIds[cd->bCompress], szDlgText,
                                                                 ARRAYSIZE(szDlgText));
         ASSERT(cchLoaded > 0);

         SetDlgItemText(hDlg, IDC_COMPRESS_SUBFOLDERS,  szDlgText);

         return TRUE;

      case WM_COMMAND:
         //
         // Handle user button selections.
         //
         switch(wParam)
         {
            case IDOK:
               EndDialog(hDlg, Button_GetCheck(GetDlgItem(hDlg, IDC_COMPRESS_SUBFOLDERS)) ?
                                                                COMPRESS_SUBSYES :
                                                                COMPRESS_SUBSNO);
               return TRUE;

            case IDCANCEL:
               g_pContext->uCompletionReason = SCCA_REASON_USERCANCEL;
               EndDialog(hDlg, COMPRESS_CANCELLED);

               //
               // Fall through to return TRUE.
               //

            case IDC_COMPRESS_SUBFOLDERS:
               //
               // Do nothing when the checkbox is selected.
               //
               return TRUE;
         }
      }

      return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CompressProgressYield
//
// DESCRIPTION:
//
//   Allow other messages including Dialog messages for Modeless dialog to be
//   processed while we are Compressing and Uncompressing files.  This message
//   loop is similar to "wfYield" in treectl.c except that it allows for the
//   processing of Modeless dialog messages also (specifically for the Progress
//   Dialogs).
//
//   Since the file/directory Compression/Uncompression is done on a single
//   thread (in order to keep it synchronous with the existing Set Attributes
//   processing) we need to provide a mechanism that will allow a user to
//   Cancel out of the operation and also allow window messages, like WM_PAINT,
//   to be processed by other Window Procedures.
//
//   Taken from WinFile.  Removed MDI-related processing.
//
// ARGUMENTS:
//
//    None.
//
// RETURNS:
//
//    Nothing.
//
///////////////////////////////////////////////////////////////////////////////
static VOID CompressProgressYield(void)
{
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (!g_hdlgProgress || !IsDialogMessage(g_hdlgProgress, &msg))
        {
           TranslateMessage(&msg);
           DispatchMessage(&msg);
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: DisplayUncompressProgress
//
// DESCRIPTION:
//
//  Update the progress of uncompressing files.
//
//   This routine uses the global variables to update the Dialog box items
//   which display the progress through the uncompression process.  The global
//   variables are updated by individual routines.  An ordinal value is sent
//   to this routine which determines which dialog box item to update.
//   Taken from WinFile.
//
//
// ARGUMENTS:
//
//    iType
//       Control value to determine what is to be updated.
//       Value names are self-descriptive.
//
// RETURNS:
//
//    Nothing.
//
///////////////////////////////////////////////////////////////////////////////
static VOID DisplayUncompressProgress(int iType)
{
    TCHAR szNum[30];

    if (!g_bShowProgress)
    {
        return;
    }

    switch (iType)
    {
        case ( PROGRESS_UPD_FILEANDDIR ) :
        case ( PROGRESS_UPD_FILENAME ) :
        {
            SetDlgItemText(g_hdlgProgress, IDC_UNCOMPRESS_FILE, g_szFile);
            if (iType != PROGRESS_UPD_FILEANDDIR)
            {
                break;
            }

            // else...fall thru
        }
        case ( PROGRESS_UPD_DIRECTORY ) :
        {
            RECT rect;
            //
            //  Preprocess the directory name to shorten it to fit
            //  into the alloted space.
            //
            GetWindowRect(GetDlgItem(g_hdlgProgress, IDC_UNCOMPRESS_DIR), &rect);
            DrawTextEx(g_hdcDirectoryTextCtrl, g_szDirectory, lstrlen(g_szDirectory), &rect, DT_MODIFYSTRING | DT_PATH_ELLIPSIS, NULL);
            SetDlgItemText(g_hdlgProgress, IDC_UNCOMPRESS_DIR, g_szDirectory);

            break;
        }
        case ( PROGRESS_UPD_DIRCNT ) :
        {
            AddCommas((DWORD)g_cTotalDirectories, szNum);
            SetDlgItemText(g_hdlgProgress, IDC_UNCOMPRESS_DIRCNT, szNum);

            break;
        }
        case ( PROGRESS_UPD_FILENUMBERS ) :
        case ( PROGRESS_UPD_FILECNT ) :
        {
            AddCommas((DWORD)g_cTotalFiles, szNum);
            SetDlgItemText(g_hdlgProgress, IDC_UNCOMPRESS_FILECNT, szNum);

            break;
        }
    }

    CompressProgressYield();
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: UncompressProgDlg
//
// DESCRIPTION:
//
//   Display progress information.
//   Taken from WinFile.
//
//   NOTE:  This is a modeless dialog and must be terminated with DestroyWindow
//          and NOT EndDialog
//
// ARGUMENTS:
//
//    Standard dialog proc args.
//
// RETURNS:
//
//    TRUE  = Message handled.
//    FALSE = Message not handled.
//
///////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY UncompressProgDlg(
    HWND hDlg,
    UINT nMsg,
    DWORD wParam,
    LONG lParam)
{
    TCHAR szTemp[120];
    RECT  rect;

    switch (nMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            CenterWindowInParent(hDlg);

            g_hdlgProgress = hDlg;

            //
            //  Clear Dialog items.
            //
            szTemp[0] = TEXT('\0');

            SetDlgItemText(hDlg, IDC_UNCOMPRESS_FILE,    szTemp);
            SetDlgItemText(hDlg, IDC_UNCOMPRESS_DIR,     szTemp);
            SetDlgItemText(hDlg, IDC_UNCOMPRESS_DIRCNT,  szTemp);
            SetDlgItemText(hDlg, IDC_UNCOMPRESS_FILECNT, szTemp);

            g_hdcDirectoryTextCtrl = GetDC(GetDlgItem(hDlg, IDC_UNCOMPRESS_DIR));
            GetClientRect(GetDlgItem(hDlg, IDC_UNCOMPRESS_DIR), &rect);
            g_cDirectoryTextCtrlWd = rect.right;

            EnableWindow(hDlg, TRUE);
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                case ( IDCANCEL ) :
                {
                    if (LOWORD(wParam) == IDCANCEL)
                       g_pContext->uCompletionReason = SCCA_REASON_USERCANCEL;

                    if (g_hdcDirectoryTextCtrl)
                    {
                        ReleaseDC(GetDlgItem(hDlg, IDC_UNCOMPRESS_DIR), g_hdcDirectoryTextCtrl);
                        g_hdcDirectoryTextCtrl = NULL;
                    }
                    DestroyWindow(hDlg);
                    g_hdlgProgress = NULL;
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: DisplayCompressProgress
//
// DESCRIPTION:
//
//  Update the progress of compressing files.
//
//   This routine uses the global variables to update the Dialog box items
//   which display the progress through the compression process.  The global
//   variables are updated by individual routines.  An ordinal value is sent
//   to this routine which determines which dialog box item to update.
//   Taken from WinFile.
//
//
// ARGUMENTS:
//
//    iType
//       Control value to determine what is to be updated.
//       Value names are self-descriptive.
//
// RETURNS:
//
//    Nothing.
//
///////////////////////////////////////////////////////////////////////////////
void DisplayCompressProgress(int iType)
{
    TCHAR szTemp[120];
    TCHAR szNum[30];
    unsigned _int64 Percentage;

    if (!g_bShowProgress)
    {
        return;
    }

    switch (iType)
    {
        case ( PROGRESS_UPD_FILEANDDIR ) :
        case ( PROGRESS_UPD_FILENAME ) :
        {
            SetDlgItemText(g_hdlgProgress, IDC_COMPRESS_FILE, g_szFile);
            if (iType != PROGRESS_UPD_FILEANDDIR)
            {
                break;
            }

            // else...fall thru
        }
        case ( PROGRESS_UPD_DIRECTORY ) :
        {
            RECT rect;
            //
            //  Preprocess the directory name to shorten it to fit
            //  into the alloted space.
            //
            GetWindowRect(GetDlgItem(g_hdlgProgress, IDC_COMPRESS_DIR), &rect);
            DrawTextEx(g_hdcDirectoryTextCtrl, g_szDirectory, lstrlen(g_szDirectory), &rect, DT_MODIFYSTRING | DT_PATH_ELLIPSIS, NULL);
            SetDlgItemText(g_hdlgProgress, IDC_COMPRESS_DIR, g_szDirectory);

            break;
        }
        case ( PROGRESS_UPD_DIRCNT ) :
        {
            AddCommas((DWORD)g_cTotalDirectories, szNum);
            SetDlgItemText(g_hdlgProgress, IDC_COMPRESS_DIRCNT, szNum);

            break;
        }
        case ( PROGRESS_UPD_FILENUMBERS ) :
        case ( PROGRESS_UPD_FILECNT ) :
        {
            AddCommas((DWORD)g_cTotalFiles, szNum);
            SetDlgItemText(g_hdlgProgress, IDC_COMPRESS_FILECNT, szNum);
            if (iType != PROGRESS_UPD_FILENUMBERS)
            {
                break;
            }

            // else...fall thru
        }
        case ( PROGRESS_UPD_COMPRESSEDSIZE ) :
        {
            Int64ToString(g_iTotalCompressedSize, szTemp, ARRAYSIZE(szTemp), TRUE, &g_NumberFormat, NUMFMT_ALL);
            FormatStringWithArgs(g_szByteCntFmt, szNum, ARRAYSIZE(szNum), szTemp);
            SetDlgItemText(g_hdlgProgress, IDC_COMPRESS_CSIZE, szNum);
            if (iType != PROGRESS_UPD_FILENUMBERS)
            {
                break;
            }

            // else...fall thru
        }
        case ( PROGRESS_UPD_FILESIZE ) :
        {
            Int64ToString(g_iTotalFileSize, szTemp, ARRAYSIZE(szTemp), TRUE, &g_NumberFormat, NUMFMT_ALL);
            FormatStringWithArgs(g_szByteCntFmt, szNum, ARRAYSIZE(szNum), szTemp);
            SetDlgItemText(g_hdlgProgress, IDC_COMPRESS_USIZE, szNum);
            if (iType != PROGRESS_UPD_FILENUMBERS)
            {
                break;
            }

            // else...fall thru
        }

        case ( PROGRESS_UPD_PERCENTAGE ) :
        {
            if (g_iTotalFileSize != 0)
            {
                //
                //  Percentage = 100 - ((CompressSize * 100) / FileSize)
                //
                Percentage = (g_iTotalCompressedSize * 100) / g_iTotalFileSize;

                if (Percentage > 100)
                {
                    Percentage = 100;
                }
                else
                    Percentage = 100 - Percentage;
            }
            else
            {
                Percentage = 0;
            }
            //
            // Note that percentage string is not formatted.
            // i.e. no commas or decimal places.
            //
            Int64ToString(Percentage, szTemp, ARRAYSIZE(szTemp), FALSE, NULL, 0);
            wsprintf(szNum, TEXT("%s%%"), szTemp);
            SetDlgItemText(g_hdlgProgress, IDC_COMPRESS_RATIO, szNum);

            break;
        }
    }

    CompressProgressYield();
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CompressProgDlg
//
// DESCRIPTION:
//
//   Display progress information.
//   Taken from WinFile.
//
//   NOTE:  This is a modeless dialog and must be terminated with DestroyWindow
//          and NOT EndDialog
//
// ARGUMENTS:
//
//    Standard dialog proc args.
//
// RETURNS:
//
//    TRUE  = Message handled.
//    FALSE = Message not handled.
//
///////////////////////////////////////////////////////////////////////////////
BOOL APIENTRY CompressProgDlg(
    HWND hDlg,
    UINT nMsg,
    DWORD wParam,
    LONG lParam)
{
    TCHAR szTemp[120];
    RECT  rect;

    switch (nMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            CenterWindowInParent(hDlg);

            g_hdlgProgress = hDlg;

            //
            //  Clear Dialog items.
            //
            szTemp[0] = TEXT('\0');

            SetDlgItemText(hDlg, IDC_COMPRESS_FILE, szTemp);
            SetDlgItemText(hDlg, IDC_COMPRESS_DIR,  szTemp);
            SetDlgItemText(hDlg, IDC_COMPRESS_DIRCNT, szTemp);
            SetDlgItemText(hDlg, IDC_COMPRESS_FILECNT, szTemp);
            SetDlgItemText(hDlg, IDC_COMPRESS_CSIZE, szTemp);
            SetDlgItemText(hDlg, IDC_COMPRESS_USIZE, szTemp);
            SetDlgItemText(hDlg, IDC_COMPRESS_RATIO, szTemp);

            g_hdcDirectoryTextCtrl = GetDC(GetDlgItem(hDlg, IDC_COMPRESS_DIR));
            GetClientRect(GetDlgItem(hDlg, IDC_COMPRESS_DIR), &rect);
            g_cDirectoryTextCtrlWd = rect.right;

            //
            // Set Dialog message text.
            //
            LoadString(g_hmodThisDll, IDS_COMPRESS_DIR, szTemp, ARRAYSIZE(szTemp));
            EnableWindow(hDlg, TRUE);

            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                case ( IDCANCEL ) :
                {
                    if (LOWORD(wParam) == IDCANCEL)
                       g_pContext->uCompletionReason = SCCA_REASON_USERCANCEL;

                    if (g_hdcDirectoryTextCtrl)
                    {
                        ReleaseDC(GetDlgItem(hDlg, IDC_COMPRESS_DIR), g_hdcDirectoryTextCtrl);
                        g_hdcDirectoryTextCtrl = NULL;
                    }
                    DestroyWindow(hDlg);
                    g_hdlgProgress = NULL;

                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: NotifyShellOfAttribChange
//
// DESCRIPTION:
//
//   If the item is visible or is a directory, tell the shell that it's
//   attributes have changed.  This will cause the shell to update any
//   compression-related display characteristics.
//
// ARGUMENTS:
//
//   pszPath
//      Fully-qualified path to file/directory that changed.
//
//   bIsDirectory
//      If TRUE, shell is notified.
//      If FALSE, shell is notified only if global recursion level counter
//          is 1.  This ensures that we don't send unnecessary notifications to
//          the shell for items that definitely are not visible in the shell view.
//
// RETURNS:
//
//    Nothing.
//
///////////////////////////////////////////////////////////////////////////////
void NotifyShellOfAttribChange(LPTSTR pszPath, BOOL bIsDirectory)
{
    //
    // Only notify shell if item is visible.
    // All items handled at recursion level 1 are visible by default.
    // Subdirectories must always be updated because they may be visible
    // the Explorer tree view.
    //
    if (1 == g_iRecursionLevel || bIsDirectory)
    {
        if (PathIsRoot(pszPath))
        {
            //
            // Invalidate the drive type flags cache for this drive.
            // Cache will be updated with new information on next call to
            // RealDriveTypeFlags( ).
            //
            InvalidateDriveType(PathGetDriveNumber(pszPath));
        }
        else
        {
            PathRemoveTheBackslash(pszPath);
        }
        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH, pszPath, NULL);

#ifdef TRACE_COMPRESSION
        DbgOut(TEXT("SHCOMPUI: Shell notified. %s"), pszPath);
#endif

    }
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: ShellChangeCompressionAttribute
//
// DESCRIPTION:
//
//   Main entry point for file compression/uncompression.
//
// ARGUMENTS:
//
//    hwndParent
//       Handle to window for parenting dialogs.
//
//    szFileSpec
//       Fully-qualified path of file to be compressed/uncompressed.
//
//    pContext
//       Address of a context structure as defined in shcompui.h
//       This structure is created by the caller so that we can maintain
//       information about the compression process across a series of
//       calls to this function.  In particular, the user can press the
//       "Ignore All Errors" button and we must remember this during
//       subsequent calls.  The structure also maintains error count
//       information and completion status.  This may be used to supplement
//       the TRUE/FALSE return mechanism to further discriminate a FALSE
//       return value.
//
//    bCompressing
//       Control variable.  TRUE = compress file.  FALSE = uncompress file.
//
//    bShowUI
//       Control of message box and dialog displays.
//       TRUE  = Show all dialogs and messages.
//       FALSE = Hide all dialogs and messages.  Used when compression is desired
//               without any user interactiion.
//
//          ***********************************************************
//                                    NOTE
//
//            The bShowUI argument has been introduced to support
//            programmatic invocation of shell compression in cases
//            where a UI display is not wanted.  The functionality
//            of preventing UI display is not complete at this time.
//            To meet the SUR beta deadline, this parameter is ignored.
//
//          ***********************************************************
//
// RETURNS:
//
//    TRUE  = Operation successful.  Continue if iterating
//            through set of files/directories.
//    FALSE = User aborted compression/uncompression or error occurred.
//            Stop if iterating through set of files/directories.
//            Query the context structure for additional completion information.
//
///////////////////////////////////////////////////////////////////////////////
BOOL ShellChangeCompressionAttribute(
    HWND hwndParent,
    LPTSTR szNameSpec,
    LPSCCA_CONTEXT pContext,
    BOOL bCompressing,
    BOOL bShowUI)
{
    TCHAR   szTitle[MAX_DLGTITLE_LEN+1];
    TCHAR   szTemp[MAX_MESSAGE_LEN+1];
    TCHAR   szFilespec[_MAX_PATH+1];
    BOOL    bCompressionAttrChange;
    BOOL    bIsDir       = FALSE;
    BOOL    bRet         = TRUE;
    HCURSOR hCursor      = NULL;
    DWORD   dwAttribs    = 0;
    DWORD   dwNewAttribs = 0;
    DWORD   dwFlags      = 0;

    ASSERT(hwndParent != NULL);
    ASSERT(szNameSpec != NULL);

    //
    //  Make sure we're not in the middle of another compression operation.
    //  If so, put up an error box warning the user that they need to wait
    //  to do another compression operation.
    //
    if (WaitForSingleObject(g_hSemaphore, 0L) == WAIT_TIMEOUT)
    {
        //
        // BUGBUG: Shouldn't assume the UI is being displayed on behalf of
        //         Explorer.  The code should be modified so that the app
        //         name is passed in through ShellChangeCompressionAtttribute.
        //
        LoadString(g_hmodThisDll, IDS_APP_NAME, szTitle, ARRAYSIZE(szTitle));
        LoadString(g_hmodThisDll, IDS_MULTI_COMPRESS_ERR, szMessage, ARRAYSIZE(szMessage));
        MessageBox(hwndParent, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION);

#ifdef TRACE_COMPRESSION
        DbgOut(TEXT("SHCOMPUI: Re-entrancy attempted and denied"));
#endif

        return (TRUE);
    }

    //
    // Give the context structure "file" scope.
    //
    g_pContext = pContext;

    //
    // Reset recursion level counter.
    //
    g_iRecursionLevel = 0;

    //
    //  Make sure the volume supports File Compression.
    //
    lstrcpy(szTemp, szNameSpec);
    PathStripToRoot(szTemp);
    // GetVolumeInformation requires a trailing backslash.  Append
    // one if this is a UNC path.
    if (PathIsUNC(szTemp))
    {
        lstrcat(szTemp, c_szBACKSLASH);
    }
    if (!GetVolumeInformation (szTemp, NULL, 0L, NULL, NULL, &dwFlags, NULL, 0L)
       || !(dwFlags & FS_FILE_COMPRESSION))
    {
        //
        //  The volume does not support file compression, so just
        //  quit out.  Do not return FALSE, since that will not
        //  allow any other attributes to be changed.
        //

#ifdef TRACE_COMPRESSION
        DbgOut(TEXT("SHCOMPUI: Volume %s doesn't support compression."), szTemp);
#endif
        ReleaseSemaphore(g_hSemaphore, 1, NULL);
        return (TRUE);
    }

    //
    //  Show the hour glass cursor.
    //
    if (hCursor = LoadCursor(NULL, IDC_WAIT))
    {
        hCursor = SetCursor(hCursor);
    }
    ShowCursor(TRUE);

    //
    //  Get the file attributes (current and new).
    //  On error, don't change the attribute.
    //
    if ((dwAttribs = GetFileAttributes(szNameSpec)) != (DWORD)-1)
    {
       if (bCompressing)
          dwNewAttribs = dwAttribs | FILE_ATTRIBUTE_COMPRESSED;
       else
          dwNewAttribs = dwAttribs & ~FILE_ATTRIBUTE_COMPRESSED;
   }

    //
    //  Determine if ATTR_COMPRESSED is changing state.
    //
    bCompressionAttrChange = ( (dwAttribs & FILE_ATTRIBUTE_COMPRESSED) !=
                               (dwNewAttribs & FILE_ATTRIBUTE_COMPRESSED) );

#ifdef TRACE_COMPRESSION
    {
       TCHAR szAttribStr[40];
       TCHAR szNewAttribStr[40];
       DbgOut(TEXT("SHCOMPUI: File = \"%-30s\"  Attr = [%s]  New = [%s]"), szNameSpec,
                               FileAttribString(dwAttribs, szAttribStr),
                               FileAttribString(dwNewAttribs, szNewAttribStr));
    }
#endif

    g_bShowProgress     = FALSE;
    g_bIgnoreAllErrors  = g_pContext->bIgnoreAllErrors;
    g_bDiskFull         = FALSE;
    g_pContext->cErrors = 0;          // Clear "current" error counter.


    //
    //  If the Compression attribute changed or if we're dealing with
    //  a directory, perform action.
    //
    bIsDir = PathIsDirectory(szNameSpec);
    if (bCompressionAttrChange || bIsDir)
    {
        INT cchLoaded = 0;

        //
        //  Reset globals before progress display.
        //
        g_cTotalDirectories       = 0;
        g_cTotalFiles             = 0;
        g_iTotalFileSize          = 0;
        g_iTotalCompressedSize    = 0;
        g_szFile[0]               = CH_NULL;
        g_szDirectory[0]          = CH_NULL;

        cchLoaded = LoadString(g_hmodThisDll, IDS_BYTECNT_FMT, g_szByteCntFmt, ARRAYSIZE(g_szByteCntFmt));
        ASSERT(cchLoaded > 0);

        if (bIsDir)
        {
            BOOL bIgnoreAll = FALSE;
            UINT_PTR uDlgResult = 0;
            CompressionDesc compdesc = { bCompressing, TEXT("") };

            lstrcpy(compdesc.szFileName, szNameSpec);
            uDlgResult = DialogBoxParam(g_hmodThisDll, MAKEINTRESOURCE(DLG_COMPRESS_CONFIRMATION),
                         hwndParent, CompressSubsConfirmDlgProc, (LPARAM)&compdesc);

            lstrcpy(szFilespec, c_szSTAR);
            g_bShowProgress = TRUE;
            switch(uDlgResult)
            {
               case COMPRESS_SUBSYES:
                  g_bDoSubdirectories = TRUE;
                  break;

               case COMPRESS_SUBSNO:
                  g_bDoSubdirectories = FALSE;
                  break;

               case COMPRESS_CANCELLED:
                  bRet = FALSE;
                  goto CancelCompress;
                  break;

               default:
                  ASSERT(0);
                  break;
            }

            if (g_bShowProgress)
            {
                g_hdlgProgress = CreateDialog(
                                   g_hmodThisDll,
                                   MAKEINTRESOURCE(bCompressing ? DLG_COMPRESS_PROGRESS : DLG_UNCOMPRESS_PROGRESS),
                                   hwndParent,
                                   (bCompressing ? (DLGPROC)CompressProgDlg :
                                                   (DLGPROC)UncompressProgDlg));


                ShowWindow(g_hdlgProgress, SW_SHOW);
            }
            PathAddBackslash(szNameSpec);
            lstrcpy(szTemp, szNameSpec);

            bRet = bCompressing ? DoCompress(hwndParent, szNameSpec, szFilespec) :
                                  DoUncompress(hwndParent, szNameSpec, szFilespec);

            //
            //  Set attribute on Directory if last call was successful.
            //
            if (bRet)
            {
                szFilespec[0] = TEXT('\0');
                g_bDoSubdirectories = FALSE;
                lstrcpy(szNameSpec, szTemp);
                bRet = bCompressing ? DoCompress(hwndParent, szNameSpec, szFilespec) :
                                      DoUncompress(hwndParent, szNameSpec, szFilespec);
            }

            //
            // If the progress dialog was being displayed, destroy it.
            //
            if (g_hdlgProgress)
            {
                if (g_hdcDirectoryTextCtrl)
                {
                    ReleaseDC( GetDlgItem(g_hdlgProgress,
                                          (bCompressing ? IDC_COMPRESS_DIR : IDC_UNCOMPRESS_DIR)),
                                          g_hdcDirectoryTextCtrl);
                    g_hdcDirectoryTextCtrl = NULL;
                }
                DestroyWindow(g_hdlgProgress);
                g_hdlgProgress = NULL;
            }
        }
        else
        {
            //
            //  Compress single file.
            //
            g_bDoSubdirectories = FALSE;
            lstrcpy(szFilespec, szNameSpec);

            PathStripPath(szFilespec);
            PathRemoveFileSpec(szNameSpec);

            PathAddBackslash(szNameSpec);

#ifdef TRACE_COMPRESSION
            DbgOut(TEXT("Compress/Uncompress single file %s%s"),szNameSpec,szFilespec);
#endif

            bRet = bCompressing ? DoCompress(hwndParent, szNameSpec, szFilespec) :
                                  DoUncompress(hwndParent, szNameSpec, szFilespec);
        }
    }

CancelCompress:

    //
    //  Reset the cursor.
    //
    if (hCursor)
    {
        SetCursor(hCursor);
    }
    ShowCursor(FALSE);

    //
    // Tell the shell that the free space on this volume has changed.
    //
    lstrcpy(szTemp, szNameSpec);
    PathStripToRoot(szTemp);
    if (PathIsUNC(szTemp))
    {
        lstrcat(szTemp, c_szBACKSLASH);
    }
    SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATH, szTemp, NULL);

    //
    //  Return the appropriate value.
    //
    g_pContext->bIgnoreAllErrors = g_bIgnoreAllErrors;
    ReleaseSemaphore(g_hSemaphore, 1, NULL);
    return (bRet);
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CompressFile
//
// DESCRIPTION:
//
//   Compress a single file.
//   Originally take from WinFile.
//
// ARGUMENTS:
//
//    Handle
//       Handle to file to be compressed.
//
//    FileSpec
//       Full file specification.
//       Required to obtain compressed file size.
//       FindData->cFileName[] doesn't include the path.
//
//    FindData
//       Pointer to file search context data.  Contains file name.
//
// RETURNS:
//
//    TRUE  = Success.
//    FALSE = Device IO error during compression.
//
///////////////////////////////////////////////////////////////////////////////
BOOL CompressFile(
    HANDLE Handle,
    LPTSTR FileSpec,
    PWIN32_FIND_DATA FindData)
{
    USHORT State;
    ULONG Length;
    LARGE_INTEGER TempLarge;


    //
    //  Print out the file name and then do the Ioctl to compress the
    //  file.  When we are done we'll print the okay message.
    //
    lstrcpy(g_szFile, FindData->cFileName);
    DisplayCompressProgress(PROGRESS_UPD_FILENAME);

    State = 1;

    if (!DeviceIoControl( Handle,
                          FSCTL_SET_COMPRESSION,
                          &State,
                          sizeof(USHORT),
                          NULL,
                          0,
                          &Length,
                          FALSE ))
    {
        g_pContext->cErrors++;
        g_pContext->cCummErrors++;

#ifdef TRACE_COMPRESSION
        DbgOut(TEXT("SHCOMPUI: CompressFile, DeviceIoControl failed with error 0x%08X"),
                       GetLastError());
#endif
        return (FALSE);
    }

    //
    //  Gather statistics (File size, compressed size and count).
    //
    TempLarge.LowPart  = FindData->nFileSizeLow;
    TempLarge.HighPart = FindData->nFileSizeHigh;
    g_iTotalFileSize += TempLarge.QuadPart;

    TempLarge.LowPart = GetCompressedFileSize(FileSpec, &(TempLarge.HighPart));
    g_iTotalCompressedSize += TempLarge.QuadPart;

    g_cTotalFiles++;

    DisplayCompressProgress(PROGRESS_UPD_FILENUMBERS);

    return (TRUE);
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: DoCompress
//
// DESCRIPTION:
//
//   Compress a directory and its subdirectories if necessary.
//   Originally take from WinFile.
//
// ARGUMENTS:
//
//   hwndParent
//      Window handle for parenting dialogs and message boxes.
//
//   DirectorySpec
//      Fully-qualified directory specification with backslash appended.
//
//   FileSpec
//      File name with directory path removed.
//
// RETURNS:
//
//    TRUE  = Success.
//    FALSE = Device IO error or user aborted compression.
//
///////////////////////////////////////////////////////////////////////////////
BOOL DoCompress(
    HWND hwndParent,
    LPTSTR DirectorySpec,
    LPTSTR FileSpec)
{
    LPTSTR DirectorySpecEnd;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    USHORT State;
    ULONG  Length;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    int MBRet;
    TCHAR szTitle[128];

    g_iRecursionLevel++;

#ifdef TRACE_COMPRESSION
    DbgOut(TEXT("SHCOMPUI: DoCompress with %s%s"), DirectorySpec, FileSpec);
    DbgOut(TEXT("          Recursion Level: %d -> %d"), g_iRecursionLevel-1, g_iRecursionLevel);
#endif

    //
    //  If the file spec is null, then set the compression bit for
    //  the directory spec and get out.
    //
    lstrcpy(g_szDirectory, DirectorySpec);
    g_szFile[0] = CH_NULL;
    DisplayCompressProgress(PROGRESS_UPD_FILEANDDIR);

    if (lstrlen(FileSpec) == 0)
    {
DoCompressRetryCreate:

        if (!OpenFileForCompress(&FileHandle, DirectorySpec))
        {
#ifdef TRACE_COMPRESSION
           DbgOut(TEXT("SHCOMPUI: DoCompress, OpenFileForCompress failed."));
#endif
            goto DoCompressError;
        }

DoCompressRetryDevIo:

        State = 1;
        if (!DeviceIoControl( FileHandle,
                              FSCTL_SET_COMPRESSION,
                              &State,
                              sizeof(USHORT),
                              NULL,
                              0,
                              &Length,
                              FALSE ))
        {
            g_pContext->cErrors++;
            g_pContext->cCummErrors++;

DoCompressError:

#ifdef TRACE_COMPRESSION
            DbgOut(TEXT("SHCOMPUI: DoCompress, DeviceIoControl failed with error 0x%08X"),
                       GetLastError());
#endif

            if (!g_bIgnoreAllErrors)
            {
                MBRet = CompressErrMessageBox( hwndParent,
                                               DirectorySpec,
                                               &FileHandle );
                if (MBRet == RETRY_CREATE)
                {
                    goto DoCompressRetryCreate;
                }
                else if (MBRet == RETRY_DEVIO)
                {
                    goto DoCompressRetryDevIo;
                }
                else if (MBRet == IDABORT)
                {
                    //
                    //  Return error.
                    //  File handle was closed by CompressErrMessageBox( ).
                    //
                    g_pContext->uCompletionReason = SCCA_REASON_USERABORT;
                    g_iRecursionLevel--;
                    return (FALSE);
                }
                //
                //  Else (MBRet == IDIGNORE)
                //  Continue on as if the error did not occur.
                //
            }
        }
        if (INVALID_HANDLE_VALUE != FileHandle)
        {
            CloseHandle(FileHandle);
            FileHandle = INVALID_HANDLE_VALUE;
        }

        g_cTotalDirectories++;
        g_cTotalFiles++;

        DisplayCompressProgress(PROGRESS_UPD_DIRCNT);
        DisplayCompressProgress(PROGRESS_UPD_FILECNT);

        NotifyShellOfAttribChange(DirectorySpec, TRUE);

        g_iRecursionLevel--;
        return (TRUE);
    }

    //
    //  Get a pointer to the end of the directory spec, so that we can
    //  keep appending names to the end of it.
    //
    DirectorySpecEnd = DirectorySpec + lstrlen(DirectorySpec);

    //
    //  List the directory that is being compressed and display
    //  its current compress attribute.
    //
    g_cTotalDirectories++;
    DisplayCompressProgress(PROGRESS_UPD_DIRCNT);

    //
    //  For every file in the directory that matches the file spec,
    //  open the file and compress it.
    //
    //  Setup the template for findfirst/findnext.
    //
    lstrcpy(DirectorySpecEnd, FileSpec);

    if ((FindHandle = FindFirstFile(DirectorySpec, &FindData)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            DWORD dwAttrib = FindData.dwFileAttributes;

            //
            //  Make sure the user hasn't hit cancel.
            //
            if (g_bShowProgress && !g_hdlgProgress)
            {
               g_iRecursionLevel--;
               FindClose(FindHandle);
               return FALSE;
            }

            //
            //  Skip over the . and .. entries.
            //
            if ( !lstrcmp(FindData.cFileName, c_szDOT) ||
                 !lstrcmp(FindData.cFileName, c_szDOTDOT) )
            {
                continue;
            }
            else if ((DirectorySpecEnd == (DirectorySpec + 3)) &&
                     !lstrcmpi(FindData.cFileName, c_szNTLDR))
            {
               //
               //  Do not allow \NTLDR to be compressed.
               //  Put up OK message box and then continue.
               //
               lstrcpy(DirectorySpecEnd, FindData.cFileName);
               LoadString(g_hmodThisDll, IDS_NTLDR_COMPRESS_ERR, szTitle, ARRAYSIZE(szTitle));
               wsprintf(szMessage, szTitle, DirectorySpec);
               LoadString(g_hmodThisDll, IDS_APP_NAME, szTitle, ARRAYSIZE(szTitle));
               MessageBox(g_hdlgProgress ? g_hdlgProgress : hwndParent, szMessage,
                                          szTitle, MB_OK | MB_ICONEXCLAMATION);

               continue;
            }
            else
            {
                //
                //  Append the found file to the directory spec and
                //  open the file.
                //
                lstrcpy(DirectorySpecEnd, FindData.cFileName);

                if ((dwAttrib & FILE_ATTRIBUTE_COMPRESSED) ||
                    (!g_bDoSubdirectories && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)))
                {
                    //
                    //  File is a directory or is already compressed.
                    //  So just skip it.
                    //
                    continue;
                }

CompressFileRetryCreate:

                if (!OpenFileForCompress(&FileHandle, DirectorySpec))
                {
#ifdef TRACE_COMPRESSION
                   DbgOut(TEXT("SHCOMPUI: DoCompress, OpenFileForCompress failed."),
                       GetLastError());
#endif
                    goto CompressFileError;
                }

CompressFileRetryDevIo:

                //
                //  Compress the file.
                //
                if (!CompressFile(FileHandle, DirectorySpec, &FindData))
                {
CompressFileError:

                    if (!g_bIgnoreAllErrors)
                    {
                        MBRet = CompressErrMessageBox( hwndParent,
                                                       DirectorySpec,
                                                       &FileHandle );
                        if (MBRet == RETRY_CREATE)
                        {
                            goto CompressFileRetryCreate;
                        }
                        else if (MBRet == RETRY_DEVIO)
                        {
                            goto CompressFileRetryDevIo;
                        }
                        else if (MBRet == IDABORT)
                        {
                            //
                            //  Return error.
                            //  File handle was closed by CompressErrMessageBox( ).
                            //
                            g_pContext->uCompletionReason = SCCA_REASON_USERABORT;
                            g_iRecursionLevel--;
                            FindClose(FindHandle);
                            return (FALSE);
                        }
                        //
                        //  Else (MBRet == IDIGNORE)
                        //  Continue on as if the error did not occur.
                        //
                    }
                }
                if (INVALID_HANDLE_VALUE != FileHandle)
                {
                    CloseHandle(FileHandle);
                    FileHandle = INVALID_HANDLE_VALUE;
                }
            }

            NotifyShellOfAttribChange(DirectorySpec,
                                     (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0);

            CompressProgressYield();

        } while (FindNextFile(FindHandle, &FindData));

        FindClose(FindHandle);
    }

    //
    //  If we are to do subdirectores, then look for every subdirectory
    //  and recursively call ourselves to list the subdirectory.
    //
    if (g_bDoSubdirectories)
    {
        //
        //  Setup findfirst/findnext to search the entire directory.
        //
        lstrcpy(DirectorySpecEnd, c_szSTAR);

        if ((FindHandle = FindFirstFile(DirectorySpec, &FindData)) != INVALID_HANDLE_VALUE)
        {
            do
            {
                //
                //  Skip over the . and .. entries, otherwise recurse.
                //
                if ( !lstrcmp(FindData.cFileName, c_szDOT) ||
                     !lstrcmp(FindData.cFileName, c_szDOTDOT) )
                {
                    continue;
                }
                else
                {
                    //
                    //  If the entry is for a directory, then tack
                    //  on the subdirectory name to the directory spec
                    //  and recurse.
                    //
                    if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        lstrcpy(DirectorySpecEnd, FindData.cFileName);
                        lstrcat(DirectorySpecEnd, c_szBACKSLASH);

                        if (!DoCompress(hwndParent, DirectorySpec, FileSpec))
                        {
                            g_iRecursionLevel--;
                            FindClose(FindHandle);
                            return (FALSE || g_bIgnoreAllErrors);
                        }
                    }
                }

            } while (FindNextFile(FindHandle, &FindData));

            FindClose(FindHandle);
        }
    }

    g_iRecursionLevel--;
    return (TRUE);
}



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: UncompressFile
//
// DESCRIPTION:
//
//   Uncompress a single file.
//   Originally take from WinFile.
//
// ARGUMENTS:
//
//    hwndParent
//       Handle to window for parenting any dialogs.
//
//    hFile
//       Handle to file to be compressed.
//
//    FindData
//       Pointer to file search context data.  Contains file name.
//
// RETURNS:
//
//    TRUE  = Success.
//    FALSE = Device IO error during compression.
//
///////////////////////////////////////////////////////////////////////////////
BOOL UncompressFile(HWND hwndParent, HANDLE hFile, PWIN32_FIND_DATA FindData)
{
    USHORT State;
    ULONG Length;

    //
    //  Print out the file name and then do the Ioctl to uncompress the
    //  file.  When we are done we'll print the okay message.
    //
    lstrcpy(g_szFile, FindData->cFileName);
    DisplayUncompressProgress(PROGRESS_UPD_FILENAME);

    State = 0;

    if (!DeviceIoControl( hFile,
                          FSCTL_SET_COMPRESSION,
                          &State,
                          sizeof(USHORT),
                          NULL,
                          0,
                          &Length,
                          FALSE )
#ifdef SIM_DISK_FULL
                          || TRUE
#endif
                          )
    {
        g_pContext->cErrors++;
        g_pContext->cCummErrors++;

#ifdef TRACE_COMPRESSION
        DbgOut(TEXT("SHCOMPUI: UncompressFile, DeviceIoControl failed with error 0x%08X"),
                       GetLastError());
#endif

#ifdef SIM_DISK_FULL
        SetLastError((DWORD)STATUS_DISK_FULL);
#endif

        if (GetLastError() == STATUS_DISK_FULL)
        {
           UncompressDiskFullError(hwndParent, hFile);
           g_bDiskFull = TRUE;
        }

        return (FALSE);
    }

    //
    //  Increment the running total.
    //
    g_cTotalFiles++;
    DisplayUncompressProgress(PROGRESS_UPD_FILENUMBERS);

    return (TRUE);
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: DoUncompress
//
// DESCRIPTION:
//
//   Uncompress a directory and its subdirectories if necessary.
//   Originally take from WinFile.
//
// ARGUMENTS:
//
//   hwndParent
//      Window handle for parenting dialogs and message boxes.
//
//   DirectorySpec
//      Fully-qualified directory specification with backslash appended.
//
//   FileSpec
//      File name with directory path removed.
//
// RETURNS:
//
//    TRUE  = Success.
//    FALSE = Device IO error or user aborted uncompression.
//
///////////////////////////////////////////////////////////////////////////////
BOOL DoUncompress(
    HWND hwndParent,
    LPTSTR DirectorySpec,
    LPTSTR FileSpec)
{
    LPTSTR DirectorySpecEnd;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    USHORT State;
    ULONG  Length;
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    int MBRet;

    g_iRecursionLevel++;

#ifdef TRACE_COMPRESSION
    DbgOut(TEXT("SHCOMPUI: DoUncompress with %s%s"), DirectorySpec, FileSpec);
    DbgOut(TEXT("          Recursion Level: %d -> %d"), g_iRecursionLevel-1, g_iRecursionLevel);
#endif

    //
    //  If the file spec is null, then clear the compression bit for
    //  the directory spec and get out.
    //
    lstrcpy(g_szDirectory, DirectorySpec);
    g_szFile[0] = CH_NULL;
    DisplayUncompressProgress(PROGRESS_UPD_FILEANDDIR);

    if (lstrlen(FileSpec) == 0)
    {
DoUncompressRetryCreate:

        if (!OpenFileForCompress(&FileHandle, DirectorySpec))
        {
#ifdef TRACE_COMPRESSION
        DbgOut(TEXT("SHCOMPUI: UncompressDirecory, OpenFileForCompress failed."));
#endif
            goto DoUncompressError;
        }

DoUncompressRetryDevIo:

        State = 0;
        if (!DeviceIoControl( FileHandle,
                              FSCTL_SET_COMPRESSION,
                              &State,
                              sizeof(USHORT),
                              NULL,
                              0,
                              &Length,
                              FALSE )
#ifdef SIM_DISK_FULL
                              || TRUE
#endif
                              )
        {
            g_pContext->cErrors++;
            g_pContext->cCummErrors++;

DoUncompressError:

            //
            // Handle disk-full error.
            //
#ifdef TRACE_COMPRESSION
        DbgOut(TEXT("SHCOMPUI: DoUncompress, DeviceIoControl failed with error 0x%08X"),
                       GetLastError());
#endif
#ifdef SIM_DISK_FULL
            SetLastError((DWORD)STATUS_DISK_FULL);
#endif
            if (GetLastError() == STATUS_DISK_FULL)
            {
               UncompressDiskFullError(hwndParent, FileHandle);
               g_bDiskFull = TRUE;
               CloseHandle(FileHandle);
               g_iRecursionLevel--;
               return FALSE;
            }

            if (!g_bIgnoreAllErrors)
            {
                MBRet = CompressErrMessageBox( hwndParent,
                                               DirectorySpec,
                                               &FileHandle );
                if (MBRet == RETRY_CREATE)
                {
                    goto DoUncompressRetryCreate;
                }
                else if (MBRet == RETRY_DEVIO)
                {
                    goto DoUncompressRetryDevIo;
                }
                else if (MBRet == IDABORT)
                {
                    //
                    //  Return error.
                    //  File handle was closed by CompressErrMessageBox.
                    //
                    g_pContext->uCompletionReason = SCCA_REASON_USERABORT;
                    g_iRecursionLevel--;
                    return (FALSE);
                }
                //
                //  Else (MBRet == IDIGNORE)
                //  Continue on as if the error did not occur.
                //
            }
        }
        if (INVALID_HANDLE_VALUE != FileHandle)
        {
            CloseHandle(FileHandle);
            FileHandle = INVALID_HANDLE_VALUE;
        }

        g_cTotalDirectories++;
        g_cTotalFiles++;

        DisplayUncompressProgress(PROGRESS_UPD_DIRCNT);
        DisplayUncompressProgress(PROGRESS_UPD_FILECNT);

        NotifyShellOfAttribChange(DirectorySpec, TRUE);

        g_iRecursionLevel--;
        return (TRUE);
    }

    //
    //  Get a pointer to the end of the directory spec, so that we can
    //  keep appending names to the end of it.
    //
    DirectorySpecEnd = DirectorySpec + lstrlen(DirectorySpec);

    g_cTotalDirectories++;
    DisplayUncompressProgress(PROGRESS_UPD_DIRCNT);

    //
    //  For every file in the directory that matches the file spec,
    //  open the file and uncompress it.
    //
    //  Setup the template for findfirst/findnext.
    //
    lstrcpy(DirectorySpecEnd, FileSpec);

    if ((FindHandle = FindFirstFile(DirectorySpec, &FindData)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            DWORD dwAttrib = FindData.dwFileAttributes;

            //
            //  Make sure the user hasn't hit cancel.
            //
            if (g_bShowProgress && !g_hdlgProgress)
            {
               g_iRecursionLevel--;
               FindClose(FindHandle);
               return FALSE;
            }

            //
            //  Skip over the . and .. entries.
            //
            if ( !lstrcmp(FindData.cFileName, c_szDOT) ||
                 !lstrcmp(FindData.cFileName, c_szDOTDOT) )
            {
                continue;
            }
            else
            {
                //
                //  Append the found file to the directory spec and
                //  open the file.
                //
                lstrcpy(DirectorySpecEnd, FindData.cFileName);

                if (!(dwAttrib & FILE_ATTRIBUTE_COMPRESSED) ||
                    (!g_bDoSubdirectories && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)))
                {
                    //
                    //  File is a directory or already uncompressed.
                    //  So skip it.
                    //
                    continue;
                }

UncompressFileRetryCreate:

                if (!OpenFileForCompress(&FileHandle, DirectorySpec))
                {
#ifdef TRACE_COMPRESSION
                    DbgOut(TEXT("SHCOMPUI: OpenFileForCompress failed."));
#endif
                    goto UncompressFileError;
                }

UncompressFileRetryDevIo:

                //
                //  Uncompress the file.
                //
                if (!UncompressFile(hwndParent, FileHandle, &FindData))
                {
UncompressFileError:

                    //
                    // If disk is full, UncompressFile( ) already handled the error.
                    // Don't handle it again.  Just return.
                    //
                    if (g_bDiskFull)
                    {
                       g_pContext->uCompletionReason = SCCA_REASON_DISKFULL;
                       CloseHandle(FileHandle);
                       FindClose(FindHandle);
                       g_iRecursionLevel--;
                       return FALSE;
                    }

                    if (!g_bIgnoreAllErrors)
                    {
                        MBRet = CompressErrMessageBox( hwndParent,
                                                       DirectorySpec,
                                                       &FileHandle );
                        if (MBRet == RETRY_CREATE)
                        {
                            goto UncompressFileRetryCreate;
                        }
                        else if (MBRet == RETRY_DEVIO)
                        {
                            goto UncompressFileRetryDevIo;
                        }
                        else if (MBRet == IDABORT)
                        {
                            //
                            //  Return error.
                            //  File handle was closed by CompressErrMessageBox.
                            //
                            g_pContext->uCompletionReason = SCCA_REASON_USERABORT;
                            g_iRecursionLevel--;
                            FindClose(FindHandle);
                            return (FALSE);
                        }
                        //
                        //  Else (MBRet == IDIGNORE)
                        //  Continue on as if the error did not occur.
                        //
                    }
                }
                if (INVALID_HANDLE_VALUE != FileHandle)
                {
                    CloseHandle(FileHandle);
                    FileHandle = INVALID_HANDLE_VALUE;
                }
            }

            NotifyShellOfAttribChange(DirectorySpec,
                                      (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0);

            CompressProgressYield();

        } while (FindNextFile(FindHandle, &FindData));

        FindClose(FindHandle);
    }

    //
    //  If we are to do subdirectores, then look for every subdirectory
    //  and recursively call ourselves to list the subdirectory.
    //
    if (g_bDoSubdirectories)
    {
        //
        //  Setup findfirst/findnext to search the entire directory.
        //
        lstrcpy(DirectorySpecEnd, c_szSTAR);

        if ((FindHandle = FindFirstFile(DirectorySpec, &FindData)) != INVALID_HANDLE_VALUE)
        {
            do
            {
                //
                //  Skip over the . and .. entries, otherwise recurse.
                //
                if ( !lstrcmp(FindData.cFileName, c_szDOT) ||
                     !lstrcmp(FindData.cFileName, c_szDOTDOT) )
                {
                    continue;
                }
                else
                {
                    //
                    //  If the entry is for a directory, then tack
                    //  on the subdirectory name to the directory spec
                    //  and recurse.
                    //
                    if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        lstrcpy(DirectorySpecEnd, FindData.cFileName);
                        lstrcat(DirectorySpecEnd, c_szBACKSLASH);

                        if (!DoUncompress(hwndParent, DirectorySpec, FileSpec))
                        {
                            g_iRecursionLevel--;
                            FindClose(FindHandle);
                            return (FALSE || g_bIgnoreAllErrors);
                        }
                    }
                }

            } while (FindNextFile(FindHandle, &FindData));

            FindClose(FindHandle);
        }
    }

    g_iRecursionLevel--;
    return (TRUE);
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CompressErrMessageBox
//
// DESCRIPTION:
//
//   Puts up the error message box when a file cannot be compressed or
//   uncompressed.  It also returns the user preference.
//
//   NOTE: The file handle is closed if the abort or ignore option is
//         chosen by the user.
//
// ARGUMENTS:
//
//
// RETURNS:
//
//    RETRY_CREATE            = User selected "Retry"
//    RETRY_DEVIO             = User selected "Retry"
//    IDABORT
//    IDIGNORE
//    IDC_COMPRESS_IGNOREALL
//
///////////////////////////////////////////////////////////////////////////////
int CompressErrMessageBox(
    HWND hwndActive,
    LPTSTR szFile,
    PHANDLE phFile)
{
    int rc;

    //
    //  Put up the error message box - ABORT, RETRY, IGNORE, IGNORE ALL.
    //
    rc = (int)DialogBoxParam( g_hmodThisDll,
                              (LPTSTR) MAKEINTRESOURCE(DLG_COMPRESS_ERROR),
                              g_hdlgProgress ? g_hdlgProgress : hwndActive,
                              CompressErrDialogProc,
                              (LPARAM)szFile );

    //
    //  Return the user preference.
    //
    if (rc == IDRETRY)
    {
        if (*phFile == INVALID_HANDLE_VALUE)
        {
            return (RETRY_CREATE);
        }
        else
        {
            return (RETRY_DEVIO);
        }
    }
    else
    {
        //
        //  IDABORT or IDIGNORE or IDC_COMPRESS_IGNOREALL
        //
        //  Close the file handle and return the message box result.
        //
        if (*phFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(*phFile);
            *phFile = INVALID_HANDLE_VALUE;
        }

        return (rc);
    }
}



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: CompressErrDialogProc
//
// DESCRIPTION:
//
//   Puts up a dialog to allow the user to Abort, Retry, Ignore, or
//   Ignore All when an error occurs during compression.
//
//
//   Taken from WinFile source wffile.c.  Modified control resource IDs to
//   be consistent with other Explorer ID naming conventions.
//
// ARGUMENTS:
//
//   Standard Dialog Proc args.
//
// RETURNS:
//
//   Standard Dialog Proc return values..
//
//   Returns through EndDialog( ):
//
//      IDABORT
//      IDRETRY
//      IDIGNORE
//      IDC_COMPRESS_IGNOREALL
//
///////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK CompressErrDialogProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    WORD IdControl = TRUE;
    TCHAR szTitle[MAX_DLGTITLE_LEN + 1];

    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            //
            // Modify very long path names so that they fit into the message box.
            // They are formatted as "c:\dir1\dir2\dir3\...\dir8\filename.ext"
            // DrawTextEx isn't drawing on anything.  Only it's formatting capabilities are
            // being used. The DT_CALCRECT flag prevents drawing.
            //
            HDC  hDC        = GetDC(hDlg);
            LONG iBaseUnits = GetDialogBaseUnits();
            RECT rc;
            TCHAR szFilePath[MAX_PATH];         // Local copy of path name string.
            const int MAX_PATH_DISPLAY_WD = 50; // Max characters to display in path name.
            const int MAX_PATH_DISPLAY_HT =  1; // Path name is 1 character high.

            rc.left   = 0;
            rc.top    = 0;
            rc.right  = MAX_PATH_DISPLAY_WD * LOWORD(iBaseUnits);
            rc.bottom = MAX_PATH_DISPLAY_HT * HIWORD(iBaseUnits);

            lstrcpyn(szFilePath, (LPCTSTR)lParam, ARRAYSIZE(szFilePath));
            DrawTextEx(hDC, szFilePath, ARRAYSIZE(szFilePath), &rc,
                                        DT_CALCRECT | DT_PATH_ELLIPSIS | DT_MODIFYSTRING, NULL);
            ReleaseDC(hDlg, hDC);

            //
            //  Set the dialog message text.
            //
            LoadString( g_hmodThisDll,
                        IDS_COMPRESS_ATTRIB_ERR,
                        szTitle,
                        ARRAYSIZE(szTitle) );

            wsprintf(szMessage, szTitle, szFilePath);
            SetDlgItemText(hDlg, IDC_COMPRESS_ERRTEXT, szMessage);
            EnableWindow (hDlg, TRUE);

            break;
        }
        case ( WM_COMMAND ) :
        {
            IdControl = GET_WM_COMMAND_ID(wParam, lParam);
            switch (IdControl)
            {
                case ( IDC_COMPRESS_IGNOREALL ) :
                {
                    g_bIgnoreAllErrors = TRUE;

                    //  fall thru...
                }
                case ( IDABORT ) :
                case ( IDRETRY ) :
                case ( IDIGNORE ) :
                {
                    EndDialog(hDlg, IdControl);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }
    return (IdControl);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: OpenFileForCompress
//
// DESCRIPTION:
//
//   Opens the file for compression.  It handles the case where a READONLY
//   file is trying to be compressed or uncompressed.  Since read only files
//   cannot be opened for WRITE_DATA, it temporarily resets the file to NOT
//   be READONLY in order to open the file, and then sets it back once the
//   file has been compressed.
//
//   Taken from WinFile module wffile.c without change.  Originally from
//   G. Kimura's compact.c.
//
// ARGUMENTS:
//
//   phFile
//      Address of file handle variable for handle of open file if
//      successful.
//
//   szFile
//      Name string of file to be opened.
//
// RETURNS:
//
//    TRUE  = File successfully opened.  Handle in *phFile.
//    FALSE = File couldn't be opened. *phFile == INVALID_HANDLE_VALUE
//
///////////////////////////////////////////////////////////////////////////////
BOOL OpenFileForCompress(
    PHANDLE phFile,
    LPTSTR szFile)
{
    HANDLE hAttr;
    BY_HANDLE_FILE_INFORMATION fi;

    //
    //  Try to open the file - READ_DATA | WRITE_DATA.
    //
    if ((*phFile = CreateFile( szFile,
                               FILE_READ_DATA | FILE_WRITE_DATA,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL )) != INVALID_HANDLE_VALUE)
    {
        //
        //  Successfully opened the file.
        //
        return (TRUE);
    }

    if (GetLastError() != ERROR_ACCESS_DENIED)
    {
        return (FALSE);
    }

    //
    //  Try to open the file - READ_ATTRIBUTES | WRITE_ATTRIBUTES.
    //
    if ((hAttr = CreateFile( szFile,
                             FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                             NULL )) == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    //
    //  See if the READONLY attribute is set.
    //
    if ( (!GetFileInformationByHandle(hAttr, &fi)) ||
         (!(fi.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) )
    {
        //
        //  If the file could not be open for some reason other than that
        //  the readonly attribute was set, then fail.
        //
        CloseHandle(hAttr);
        return (FALSE);
    }

    //
    //  Turn OFF the READONLY attribute.
    //
    fi.dwFileAttributes &= ~FILE_ATTRIBUTE_READONLY;
    if (!SetFileAttributes(szFile, fi.dwFileAttributes))
    {
        CloseHandle(hAttr);
        return (FALSE);
    }

    //
    //  Try again to open the file - READ_DATA | WRITE_DATA.
    //
    *phFile = CreateFile( szFile,
                          FILE_READ_DATA | FILE_WRITE_DATA,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
                          NULL );

    //
    //  Close the file handle opened for READ_ATTRIBUTE | WRITE_ATTRIBUTE.
    //
    CloseHandle(hAttr);

    //
    //  Make sure the open succeeded.  If it still couldn't be opened with
    //  the readonly attribute turned off, then fail.
    //
    if (*phFile == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    //
    //  Turn the READONLY attribute back ON.
    //
    fi.dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
    if (!SetFileAttributes(szFile, fi.dwFileAttributes))
    {
        CloseHandle(*phFile);
        *phFile = INVALID_HANDLE_VALUE;
        return (FALSE);
    }

    //
    //  Return success.  A valid file handle is in *phFile.
    //
    return (TRUE);
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: UncompressDiskFullError
//
// DESCRIPTION:
//
//    To be called when disk space is exhausted during uncompression.
//    The function displays a message box with an OK button.
//    NTFS leaves a file partially compressed when DeviceIoControl( )
//    returns STATUS_DISK_FULL in GetLastError( ).  It also leaves the
//    compressed attribute as "compressed".
//    Once the user acknowledges the message, we attempt to re-compress the
//    file so that the entire file is compressed and matches its
//    attribute setting.
//
// ARGUMENTS:
//
//    hFile
//       Handle to file being uncompressed at the time of the error.
//
// RETURNS:
//
//    Nothing.
//
///////////////////////////////////////////////////////////////////////////////
static void UncompressDiskFullError(HWND hwndParent, HANDLE hFile)
{
   TCHAR szTitle[MAX_DLGTITLE_LEN + 1];
   USHORT State = 1; // 1 = Compress.
   ULONG Length = 0;

   LoadString(g_hmodThisDll, IDS_APP_NAME, szTitle, ARRAYSIZE(szTitle));
   LoadString(g_hmodThisDll, IDS_UNCOMPRESS_DISKFULL, szMessage, ARRAYSIZE(szMessage));

   MessageBox(hwndParent, szMessage, szTitle, MB_OK | MB_ICONSTOP);

   //
   // Try to compress the file.
   // Don't worry about errors on the compression attempt.
   // At worst case, the file is left partially compressed with the attribute
   // set as "compressed".  According to the NTFS people, this is not
   // harmful and the file is still usable.  Besides, there isn't much else
   // we could do at this point.
   //
   DeviceIoControl( hFile,
                    FSCTL_SET_COMPRESSION,
                    &State,
                    sizeof(USHORT),
                    NULL,
                    0,
                    &Length,
                    FALSE );

}



#endif        // ifdef WINNT
