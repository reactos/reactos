#ifndef pack2_h__
#define pack2_h__


#define INC_OLE2        // Prevent windows.h from pulling in OLE 1
#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <shlobj.h>         // ;Internal
#include <shellapi.h>       // ;Internal
#include <shlwapi.h>
#include <shlwapip.h>       // ;Internal
#include <ole2.h>

#include <ole2ver.h>
// #include <crtfree.h>        // don't use CRT libs
#include <ccstock.h>
// #include <shsemip.h>	// in ccshell\inc
// #include <shellp.h>     // in ccshell\inc
// #include <debug.h>      // in ccshell\inc
// #include <shguidp.h>    // in ccshell\inc

#include "packutil.h"
#include "packguid.h"
#include "ids.h"


HRESULT CPackage_CreateInstnace(IUnknown ** ppunk);

//////////////////////////////////
// External Variables
//
extern UINT             g_cRefThisDll;		// per-instance
extern HINSTANCE        g_hinst;                
extern UINT             g_cfFileContents;       
extern UINT             g_cfFileDescriptor;
extern UINT		g_cfObjectDescriptor;
extern UINT		g_cfEmbedSource;
extern UINT             g_cfFileNameW;
extern INT              g_cxIcon;
extern INT              g_cyIcon;
extern INT              g_cxArrange;
extern INT              g_cyArrange;
extern HFONT            g_hfontTitle;


//////////////////////////////////
// Global Constants
//
#define HIMETRIC_PER_INCH       2540    // Number of HIMETRIC units per inch
#define DEF_LOGPIXELSX          96      // Default values for pixels per
#define DEF_LOGPIXELSY          96      // logical inch
#define CBCMDLINKMAX            500     // num chars in cmdline package
#define FILE_SHARE_READWRITE    (FILE_SHARE_READ | FILE_SHARE_WRITE)
#define OLEIVERB_EDITPACKAGE    (OLEIVERB_PRIMARY+1)
#define OLEIVERB_FIRST_CONTEXT  (OLEIVERB_PRIMARY+2)
#define OLEIVERB_LAST_CONTEXT   (OLEIVERB_PRIMARY+0xFFFF)
#define PACKWIZ_NUM_PAGES	3	// number of pages in our wizard


//////////////////////////////////
// Clipboard Formats
//
#define CF_FILECONTENTS     g_cfFileContents
#define CF_FILEDESCRIPTOR   g_cfFileDescriptor
#define CF_OBJECTDESCRIPTOR g_cfObjectDescriptor
#define CF_EMBEDSOURCE	    g_cfEmbedSource
#define CF_FILENAMEW        g_cfFileNameW
#define CFSTR_EMBEDSOURCE   TEXT("Embed Source")
#define CFSTR_OBJECTDESCRIPTOR TEXT("Object Descriptor")


//////////////////////////////////
// String constants
//
#define SZUSERTYPE              L"OLE Package"
#define SZCONTENTS              L"\001Ole10Native"
#define SZAPPNAME               TEXT("Object Packager")

//////////////////////////////////
// Old packager junk...
//
// NOTE: This enumeration is used to determine what kind of information
// is stored in a packager object.  Currently, the new packager on supports
// ICON and PEMBED.  We will probably want to implement CMDLINK and PICTURE
// to remain back compatible with the old packager.
//
typedef enum
{
    NOTHING,
    CMDLINK,
    ICON,
    PEMBED,
    PICTURE
} PANETYPE;

//////////////////////////////////
// Embedded File structure
//
// NOTE: This is similar to the structure used by the old packager to store
// information about the embedded file, however it is slightly different.
// Most notably, I use a FILEDESCRIPTOR structure to hold onto the filename
// and filesize so this information can be easily transferred in a GetData call
// Also, I've deleted some uncessary fields that the old packager used to 
// deal with OLE 1 ways of dealing with things.

typedef struct _EMBED           // embed
{
    FILEDESCRIPTOR fd;          // file descriptor of embedded file
    LPTSTR  pszTempName;         // temp. file name used when shellexec'ing
    HANDLE hTask;               // handle to task on shellexec'ed objects
    LPOLEOBJECT poo;            // oleobject interface on running contents
    BOOL   fIsOleFile;          // TRUE if OLE can activate this type of file
} EMBED, *LPEMBED;


//////////////////////////////////
// Command line structure
//
// NOTE: This is the structure the old packager used when implementing
// the command line packages.  It will be best to use this structure for
// the new packager for ease of use in reading and writing old packager
// formats.
//
typedef struct _CML             // cml
{
    BOOL fCmdIsLink;
    TCHAR szCommandLine[CBCMDLINKMAX];
} CML, *LPCML;

////////////////////////////////
// PackageInfo Structure
//
// NOTE: This structure is used by the Create New Package Wizard and the
// Edit Package dialogs.  We use it to hold onto package information, so that
// the CPackage Object can initialize/reinitialize itself after one of these
// calls.
//
typedef struct _packageInfo 
{
	TCHAR	szLabel[MAX_PATH];
	TCHAR	szFilename[MAX_PATH];
	TCHAR	szIconPath[MAX_PATH];
	int 	iIcon;			    // must be an int for PickIconDlg
} PACKAGER_INFO, *LPPACKAGER_INFO;


////////////////////////////////
// PersistStorage enumeration
//
typedef enum
{
        PSSTATE_UNINIT = 0,     //Uninitialized
        PSSTATE_SCRIBBLE,   //Scribble
        PSSTATE_ZOMBIE,     //No scribble
        PSSTATE_HANDSOFF    //Hand-off
} PSSTATE;

#endif

#include "debug.h"

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))      
