/* packager.h - Constants, types, and exports from the main module.
 */

#include <windows.h>

#ifdef STRICT
#   undef STRICT
#   define PACKGR_STRICT
#endif

#define SERVERONLY
#include <ole.h>

#ifdef PACKGR_STRICT
#   define STRICT
#   undef PACKGR_STRICT
#endif

#include "ids.h"


#define HIMETRIC_PER_INCH   2540    // Number of HIMETRIC units per inch
#define DEF_LOGPIXELSX      96      // Default values for pixels per
#define DEF_LOGPIXELSY      96      // logical inch

#define KEYNAMESIZE         300     // Maximum registration key length

#define CCLIENTNAMEMAX      50      // Maximum length of client app name
#define CBCMDLINKMAX        500
#define CBMESSAGEMAX        128
#define CBSTRINGMAX         256     // Maximum lenght of a string in the res.
#define CBSHORTSTRING       20
#define CBFILTERMAX         50      // Max # chars in a filter specification
#define CBPATHMAX           260     // Most chars in a fully qual. filename

#define CharCountOf(a)      (sizeof(a) / sizeof(a[0]))

#define CITEMSMAX           100

#define APPEARANCE          0
#define CONTENT             1
#define CCHILDREN           2       // Number of panes which precede

#define OLE_PLAY            0
#define OLE_EDIT            1

#define WM_FIXSCROLL        (WM_USER+100)
#define WM_REDRAW           (WM_USER+101)
#define WM_READEMBEDDED     (WM_USER+102)


#define CHAR_SPACE          TEXT(' ')
#define CHAR_QUOTE          TEXT('"')

#define SZ_QUOTE            TEXT("\"")

typedef enum
{
    NOTHING,
    CMDLINK,
    ICON,
    PEMBED,
    PICTURE
} PANETYPE;


typedef enum
{
    SOP_FILE,
    SOP_MEMORY
}
STREAMOP;


typedef struct _APPSTREAM
{
    LPOLESTREAMVTBL lpstbl;
    INT fh;
} APPSTREAM, *LPAPPSTREAM;


typedef struct _EMBED           // embed
{
    ATOM aFileName;
    ATOM aTempName;
    DWORD dwSize;
    HANDLE hContents;
    HANDLE hdata;
    HANDLE hTask;
    HANDLE hSvrInst;
    BOOL bOleSvrFile;
    LPOLECLIENT lpclient;       // At activation time we check whether the file
    LPOLEOBJECT lpLinkObj;      // is a OLE server file. If so, we will create
                                // a link to it, and activate it in OLE fashion
} EMBED, *LPEMBED;


typedef struct _CML             // cml
{
    HANDLE hdata;
    RECT rc;                    // HACK:  Same location as in PICT
    BOOL fCmdIsLink;
    CHAR szCommand[CBCMDLINKMAX];
} CML, *LPCML;


typedef struct _IC              // ic
{
    HANDLE hdata;
    HICON hDlgIcon;
    CHAR szIconPath[CBPATHMAX];
    CHAR szIconText[CBPATHMAX];
    INT iDlgIcon;
} IC, *LPIC;


typedef struct _PICT            // pict
{
    HANDLE hdata;
    RECT rc;                    // HACK:  Same location as in CML
    LPOLEOBJECT lpObject;
    BOOL fNotReady;             // TRUE if object creation is not complete
} PICT, *LPPICT;


typedef struct _SAMPSRVR        // srvr
{
    OLESERVER olesrvr;          // Server
    HANDLE hsrvr;               // Handle to server memory block
    LHSERVER lhsrvr;            // Registration handle
} PBSRVR, *LPSAMPSRVR;


typedef struct _SAMPDOC         // doc
{
    OLESERVERDOC oledoc;        // Document
    HANDLE hdoc;                // Handle to document memory block
    LHSERVERDOC lhdoc;          // Registration handle
    ATOM aName;                 // Document name atom
} PBDOC, *LPSAMPDOC;


typedef struct _SAMPITEM        // item
{
    OLEOBJECT oleobject;        // Object
    HANDLE hitem;               // Handle to item memory block
    LPOLECLIENT lpoleclient;
    INT ref;                    // # of references to document
    ATOM aName;                 // Item name atom
} ITEM, *LPSAMPITEM;


#include "globals.h"

#include "function.h"


#if DBG_PRNT
#define DPRINT(s)   OutputDebugString(TEXT(s) TEXT("\n"))
#else
#define DPRINT(s)
#endif
