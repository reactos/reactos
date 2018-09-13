//
// Module: globals.c
//
// Global variables for the Object Packager.
//
//

#include "packager.h"


INT gcxIcon;
INT gcyIcon;
INT gcxArrange;                     // Icon text wrap boundary
INT gcyArrange;
INT giXppli = DEF_LOGPIXELSX;       // Number of pixels per logical
INT giYppli = DEF_LOGPIXELSY;       // inch along width and height

BOOL gfEmbObjectOpen = FALSE;
BOOL gfBlocked = FALSE;
BOOL gfEmbedded = FALSE;            // Editing an embedded object?
BOOL gfInvisible = FALSE;           // Editing invisibly?
BOOL gfOleClosed = FALSE;           // Should we send Ole_Closed or not?
BOOL gfEmbeddedFlag = FALSE;        // Editing with /Embedded flag?
BOOL gfDocCleared = FALSE;
BOOL gfServer = FALSE;              // Is the server loaded?
BOOL gfDocExists = FALSE;

HANDLE ghInst;                      // Unique instance identifier
HACCEL ghAccTable;                  // Application specific accelerator table
HBRUSH ghbrBackground = NULL;       // Fill brush used to paint background
HFONT ghfontTitle = NULL;
HFONT ghfontChild;                  // Font for caption bar
HCURSOR ghcurWait;                  // Hourglass cursor

HWND ghwndFrame;                    // Main window
HWND ghwndBar[CCHILDREN];
HWND ghwndPane[CCHILDREN];
HWND ghwndPict;
HWND ghwndError = NULL;             // Parent window when Error popup happens

INT gnCmdShowSave;                  // Show flags; saved if started invisibly
UINT gcOleWait = 0;                 // OLE asynchronous transaction counter
LHCLIENTDOC glhcdoc = 0;	   // Handle to client document "link"
LPSAMPDOC gvlptempdoc = NULL;
LPAPPSTREAM glpStream = NULL;
LPOLECLIENT glpclient = NULL;
LPVOID glpobj[CCHILDREN];
LPVOID glpobjUndo[CCHILDREN];
HANDLE ghServer = NULL;             // Handle to server memory block
LPSAMPSRVR glpsrvr = NULL;          // Pointer to OLE server memory
LPSAMPDOC glpdoc = NULL;            // Pointer to current OLE document
DWORD gcbObject;
PANETYPE gpty[CCHILDREN];
PANETYPE gptyUndo[CCHILDREN];

OLECLIPFORMAT gcfFileName = 0;      // Clipboard format "FileName"
OLECLIPFORMAT gcfLink = 0;          // Clipboard format "ObjectLink"
OLECLIPFORMAT gcfNative = 0;        // Clipboard format "Native"
OLECLIPFORMAT gcfOwnerLink = 0;     // Clipboard format "OwnerLink"

CHAR gszClientName[CCLIENTNAMEMAX]; // Name of the client application
CHAR gszFileName[CBPATHMAX];
CHAR gszCaption[CCHILDREN][CBMESSAGEMAX];
CHAR gszProtocol[] = "StdFileEditing";
CHAR gszSProtocol[] = "Static";
CHAR gszTemp[] = "Fake Object";
CHAR gszAppClassName[] = "Package"; // Not NLS specific

CHAR szAppName[CBMESSAGEMAX];       // Application name
CHAR szUntitled[CBMESSAGEMAX];      // "(Untitled)" string BUGBUG this is misnamed and used funny
CHAR szUndo[CBSHORTSTRING];         // "Undo %s" string
CHAR szContent[CBMESSAGEMAX];
CHAR szAppearance[CBMESSAGEMAX];
CHAR szDummy[CBSHORTSTRING];
