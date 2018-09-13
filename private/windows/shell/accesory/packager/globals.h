//
// Module: globals.h
//
// Global variable declarations for the Object Packager.
//
//


extern INT gcxIcon;
extern INT gcyIcon;
extern INT gcxArrange;              // Icon text wrap boundary
extern INT gcyArrange;
extern INT giXppli;                 // Number of pixels per logical
extern INT giYppli;                 // inch along width and height

extern BOOL gfEmbObjectOpen;
extern BOOL gfBlocked;
extern BOOL gfEmbedded;             // Editing an embedded object?
extern BOOL gfInvisible;            // Editing invisibly?
extern BOOL gfOleClosed;            // Should we send Ole_Closed or not?
extern BOOL gfEmbeddedFlag;         // Editing with /Embedded flag?
extern BOOL gfDocCleared;
extern BOOL gfServer;               // Is the server loaded?
extern BOOL gfDocExists;
extern BOOL gbDBCS;                 // Are we running in DBCS mode?

extern HANDLE ghInst;               // Unique instance identifier
extern HACCEL ghAccTable;           // Application specific accelerator table
extern HBRUSH ghbrBackground;       // Fill brush used to paint background
extern HFONT ghfontTitle;
extern HFONT ghfontChild;           // Font for caption bar
extern HCURSOR ghcurWait;           // Hourglass cursor

extern HWND ghwndFrame;             // Main window
extern HWND ghwndBar[];
extern HWND ghwndPane[];
extern HWND ghwndPict;
extern HWND ghwndError;             // Parent window when Error popup happens

extern INT gnCmdShowSave;           // Show flags; saved if started invisibly
extern UINT gcOleWait;              // OLE asynchronous transaction counter
extern LHCLIENTDOC glhcdoc;         // Handle to client document "link"
extern LPSAMPDOC gvlptempdoc;
extern LPAPPSTREAM glpStream;
extern LPOLECLIENT glpclient;
extern LPVOID glpobj[];
extern LPVOID glpobjUndo[];
extern HANDLE ghServer;             // Handle to server memory block
extern LPSAMPSRVR glpsrvr;          // Pointer to OLE server memory
extern LPSAMPDOC glpdoc;            // Pointer to current OLE document
extern DWORD gcbObject;
extern PANETYPE gpty[];
extern PANETYPE gptyUndo[];

extern OLECLIPFORMAT gcfFileName;   // Clipboard format "FileName"
extern OLECLIPFORMAT gcfLink;       // Clipboard format "ObjectLink"
extern OLECLIPFORMAT gcfNative;     // Clipboard format "Native"
extern OLECLIPFORMAT gcfOwnerLink;  // Clipboard format "OwnerLink"

extern CHAR gszClientName[];        // Name of the client application
extern CHAR gszFileName[];
extern CHAR gszCaption[][CBMESSAGEMAX];
extern CHAR gszProtocol[];
extern CHAR gszSProtocol[];
extern CHAR gszTemp[];
extern CHAR gszAppClassName[];      // Not NLS specific

extern CHAR szAppName[];            // Application name
extern CHAR szUntitled[];           // "(Untitled)" string BUGBUG this is misnamed and used funny
extern CHAR szUndo[];               // "Undo %s" string
extern CHAR szContent[];
extern CHAR szAppearance[];
extern CHAR szDummy[];

