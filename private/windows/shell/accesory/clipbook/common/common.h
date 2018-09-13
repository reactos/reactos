
/*****************************************************************************

                        C O M M O N   H E A D E R

    Name:       common.h
    Date:       19-Apr-1994
    Creator:    John Fu

    Description:
        This is the header common to clipview and datasrv.

*****************************************************************************/



#define   PREVBMPSIZ   64   // dim of preview bitmap ( x and y )

// non localized control strings common to Clipsrv.exe and clipview.exe
#define     SZ_SRV_NAME         "ClipSrv"
#define     SZ_FORMAT_LIST      TEXT("FormatList")

#define     SZCMD_INITSHARE     TEXT("[initshare]")
#define     SZCMD_EXIT          TEXT("[exit]")
#define     SZCMD_PASTESHARE    TEXT("[pasteshare]")
#define     SZCMD_DELETE        TEXT("[delete]")
#define     SZCMD_SHARE         TEXT("[markshared]")
#define     SZCMD_UNSHARE       TEXT("[markunshared]")
#define     SZCMD_PASTE         TEXT("[paste]")
#define     SZCMD_SAVEAS        TEXT("[saveas]")
#define     SZCMD_OPEN          TEXT("[open]")
#define     SZCMD_DEBUG         TEXT("[debug]")



#define     MAX_CMD_LEN         30
#define     MAX_DDE_EXEC        (MAX_PATH +MAX_CMD_LEN +1)






// These commands are new for NT clipbook.
///////////////////////////////////////////////////////////////////////

// Requesting for error code after an XTYP_EXECUTE xtransaction
#define     SZ_ERR_REQUEST      TEXT("ErrorRequest")

#define     XERRT_MASK          0xF0        // use to mask the XERR types
#define     XERRT_SYS           0x10        // XERR type, a GetLastError error code
#define     XERRT_NDDE          0x20        // XERR type, a NDde error code
#define     XERRT_DDE           0x30        // XERR type, a DDE error code
#define     XERR_FORMAT         "%x %x"     // XERR format string, "error_type error_code"


// Save clipbrd file in Win 3.1 format
#define     SZCMD_SAVEASOLD     TEXT("[saveasold]")


// Version request - NT product 1 clipsrv will return 0x3010
#define     SZCMD_VERSION       TEXT("[Version]")


// Security information
#define     SZCMD_SECURITY      TEXT("[Security]")

#define     SHR_CHAR            TEXT('$')
#define     UNSHR_CHAR          TEXT('*')
#define     BOGUS_CHAR          TEXT('?')

#define     SZPREVNAME          TEXT("Clipbook Preview")
#define     SZLINK              TEXT("Link")
#define     SZLINKCOPY          TEXT("LnkCpy")
#define     SZOBJECTLINK        TEXT("ObjectLink")
#define     SZOBJECTLINKCOPY    TEXT("ObjLnkCpy")
#define     LSZOBJECTLINK       L"ObjectLink"
#define     LSZLINK             L"Link"

// The viewer and the server use this mutex name to avoid opening
// the clipboard at the same time.
#define     SZMUTEXCLP          TEXT("ClipbrdMutex")



// The Common globals

extern      HINSTANCE           hInst;
extern      UINT                cf_preview;
extern      HWND                hwndApp;




// added for winball - clausgi
extern UINT cf_link;
extern UINT cf_objectlink;
extern UINT cf_linkcopy;
extern UINT cf_objectlinkcopy;

// end additions

#define PRIVATE_FORMAT(fmt) ((fmt) >= 0xC000)




/* Dialogbox resource id */
#define ABOUTBOX        1
#define CONFIRMBOX  2



/* Other constants */
#define CDEFFMTS        8       /* Count of predifined clipboard formats    */
#define VPOSLAST        100     /* Highest vert scroll bar value */
#define HPOSLAST        100     /* Highest horiz scroll bar value */
#define CCHFMTNAMEMAX   79      /* Longest clipboard data fmt name, including
                                   terminator */
#define cLineAlwaysShow 3       /* # of "standard text height" lines to show
                                   when maximally scrolled down */
#define BUFFERLEN       160      /* String buffer length */
#define SMALLBUFFERLEN  90
#define IDSABOUT        1

#define CBMENU      1   /* Number for the Clipboard main menu  */

#define FILTERMAX   100     /* max len. of File/Open filter string */
#define CAPTIONMAX  30      /* len of caption text for above dlg.  */
#define PATHMAX     128     /* max. len of DOS pathname        */




/*  Last parameter to SetDIBits() and GetDIBits() calls */

#define  DIB_RGB_COLORS   0
#define  DIB_PAL_COLORS   1

#define  IDCLEAR    IDOK




/* Structures for saving/loading clipboard data from disk */

#define      CLP_ID  0xC350
#define   CLP_NT_ID  0xC351
#define CLPBK_NT_ID  0xC352

typedef struct
   {
   WORD        magic;
   WORD        FormatCount;
   } FILEHEADER;


// Format header
typedef struct
   {
   DWORD FormatID;
   DWORD DataLen;
   DWORD DataOffset;
   WCHAR  Name[CCHFMTNAMEMAX];
   } FORMATHEADER;

// Windows 3.1-type structures - Win31 packed on byte boundaries.
#pragma pack(1)
typedef struct
   {
   WORD FormatID;
   DWORD DataLen;
   DWORD DataOffset;
   char Name[CCHFMTNAMEMAX];
   } OLDFORMATHEADER;

// Windows 3.1 BITMAP struct - used to save Win 3.1 .CLP files
typedef struct {
   WORD bmType;
   WORD bmWidth;
   WORD bmHeight;
   WORD bmWidthBytes;
   BYTE bmPlanes;
   BYTE bmBitsPixel;
   LPVOID bmBits;
   } WIN31BITMAP;

// Windows 3.1 METAFILEPICT struct
typedef struct {
   WORD mm;
   WORD xExt;
   WORD yExt;
   WORD hMF;
   } WIN31METAFILEPICT;

#pragma pack()





/*****************************  global data  *******************************/
// extern OFSTRUCT  ofStruct;
extern HWND  hwndMain;
extern TCHAR szAppName[];
extern TCHAR szFileSpecifier[];

/* variables for the new File Open,File SaveAs and Find Text dialogs */

extern TCHAR  szSaveFileName [];
extern TCHAR  szLastDir  [];
extern TCHAR  szFilterSpec [];    /* default filter spec. for above  */
extern int    wHlpMsg;            /* message used to invoke Help     */
extern TCHAR  szOpenCaption [];   /* File open dialog caption text   */
extern TCHAR  szSaveCaption [];   /* File Save as dialog caption text  */












//
//  Common function prototypes that are
//  not defined in common lib
//


BOOL SyncOpenClipboard(
    HWND    hwnd);

BOOL SyncCloseClipboard(void);
