//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cdutil.hxx
//
//  Contents:   Forms^3 utilities
//
//----------------------------------------------------------------------------

#ifndef I_PUTIL_HXX_
#define I_PUTIL_HXX_
#pragma INCMSG("--- Beg 'putil.hxx'")

#ifndef X_DBLLINK_HXX_
#define X_DBLLINK_HXX_
#include "dbllink.hxx"
#endif

#define PRINTMODE_NO_TRANSPARENCY 0x01

class CPrintDoc;    // declared in formkrnl.hxx
class CBodyElement; // declared in ebody.hxx
//+------------------------------------------------------------------------
//
//  Common Print structures
//
//-------------------------------------------------------------------------

MtExtern(PRINTINFOBAG)

struct PRINTINFOBAG
{
    DECLARE_MEMALLOC_NEW_DELETE(Mt(PRINTINFOBAG))

    PRINTINFOBAG();
    HGLOBAL                 hDevMode;           // Device mode handle
    HGLOBAL                 hDevNames;          // Device name handle
    POINT                   ptPaperSize;        // Paper size
    RECT                    rtMargin;           // Margins
    HDC                     hdc;                // Device context handle
    HDC                     hic;                // Information context handle
    DWORD                   dwThreadId;         // ID of enqueuing thread
    WORD                    nCopies;            // nr of copies
    DVTARGETDEVICE          *ptd;               // ptargetdevice for root printjob (owner is printjob)
    CStr                    strHeader;         // header string
    CStr                    strFooter;         // footer string
    TCHAR                   achPrintToFileName[MAX_PATH];
    IStream *               pstmOEHeader;       // Stream of Outlook Express mail header HTML document
    CPrintDoc *             pPrintDocOEHeader;  // Print doc holding the Outlook Express mail header
    WORD                    nFromPage;
    WORD                    nToPage;
    int                     iFontScaling;
    DWORD                   dwPrintMode;        // Driver specific print mode (e.g. to suppress SRCAND in image draw code)
    CBodyElement *          pBodyActive;        // the active frames body (for drawing focus rect)

    unsigned int            fCollate:1;         // Collate the copies
    unsigned int            fShortcutTable:1;   // Print table of shortcuts at end
    unsigned int            fPrintLinked:1;     // Print recursively
    unsigned int            fBackground:1;      // Print background color or bitmap
    unsigned int            fRootDocumentHasFrameset:1; // Does the root document have a frameset?
    unsigned int            fPrintActiveFrame:1;// Shall we print the active frame or all frames
    unsigned int            fPrintSelection:1;  // print just the selection
    unsigned int            fAllPages:1;        // print all pages ?
    unsigned int            fSpoolerEmptyBeforeEnqueue:1; // Was the spooler empty before this job was enqueued? (internal)
    unsigned int            fDrtPrint:1;        // are we printing for the DRT?
    unsigned int            fPagesSelected:1;   // from - to Pages was selected ?
    unsigned int            fPrintToFile:1;     // dialog - print to file was checked ?
    unsigned int            fPrintAsShown:1;    // print wysiwyg
    unsigned int            fPrintOEHeader:1;   // print Outlook Express mail header
    unsigned int            fConsiderBitmapFonts:1; // consider having the driver download fonts as bitmaps to avoid HP driver bug 25436
    unsigned int            fXML:1;             // originally an xml file that generated temporary html


        PRINTINFOBAG& operator=(const PRINTINFOBAG &otherbag)
        {
        CStr    Header;
        CStr    Footer;
        hDevMode = otherbag.hDevMode;
        hDevNames = otherbag.hDevNames;
        ptPaperSize = otherbag.ptPaperSize;
        rtMargin = otherbag.rtMargin;
        hdc = otherbag.hdc;
        hic = otherbag.hic;
        dwThreadId = otherbag.dwThreadId;
        nCopies = otherbag.nCopies;
        ptd = otherbag.ptd;
        Header.Set(otherbag.strHeader);
        strHeader.Set(Header);
        Footer.Set(otherbag.strFooter);
        strFooter.Set(Footer);
        _tcscpy(achPrintToFileName, otherbag.achPrintToFileName);
        pstmOEHeader = otherbag.pstmOEHeader;
        pPrintDocOEHeader = otherbag.pPrintDocOEHeader;
        pBodyActive = otherbag.pBodyActive;
        fCollate = otherbag.fCollate;
        fShortcutTable = otherbag.fShortcutTable;
        fPrintLinked = otherbag.fPrintLinked;
        fBackground = otherbag.fBackground;
        fRootDocumentHasFrameset = otherbag.fRootDocumentHasFrameset;
        fPrintActiveFrame = otherbag.fPrintActiveFrame;
        fPrintSelection = otherbag.fPrintSelection;
        fAllPages = otherbag.fAllPages;
        nFromPage = otherbag.nFromPage;
        nToPage = otherbag.nToPage;
        fSpoolerEmptyBeforeEnqueue = otherbag.fSpoolerEmptyBeforeEnqueue;
        fDrtPrint = otherbag.fDrtPrint;
        fPagesSelected = otherbag.fPagesSelected;
        iFontScaling = otherbag.iFontScaling;
        fPrintToFile = otherbag.fPrintToFile;
        fPrintAsShown = otherbag.fPrintAsShown;
        fPrintOEHeader = otherbag.fPrintOEHeader;
        fConsiderBitmapFonts = otherbag.fConsiderBitmapFonts;
        fXML = otherbag.fXML;
        return *this;
        };
}; // tag: pibag

MtExtern(DUMMYJOB)

struct DUMMYJOB : public CIntrusiveDblLinkedListNode
{
    DECLARE_MEMALLOC_NEW_DELETE(Mt(DUMMYJOB))

    DWORD               dwDummyJobId;      // Unique dummy job id (different from print job id)
    HANDLE              hPrinter;          // Printer handle needed for lifetime of dummy print job

    DUMMYJOB& operator=(const DUMMYJOB &other)
    {
        dwDummyJobId = other.dwDummyJobId;
        hPrinter = other.hPrinter;
        return *this;
    }
}; // tag: dj

typedef TIntrusiveDblLinkedListOfStruct<DUMMYJOB> DummyJobList;

MtExtern(PRINTINFO)

struct PRINTINFO : public CIntrusiveDblLinkedListNode
{
    DECLARE_MEMALLOC_NEW_DELETE(Mt(PRINTINFO))

    PRINTINFOBAG *          ppibagRootDocument; // Root document print info bag pointer

    ULONG                   ulPrintJobId;       // Unique print job id
    CStr                    cstrTempFileName;   // Temporary file name (only used with fTempFile)
    CStr                    cstrBaseUrl;        // Print job's absolute URL or filename
    CStr                    cstrPrettyUrl;      // Print job's pretty URL (absolute) used when printing framesets
    EVENT_HANDLE            hEvent;             // event to signal when printing is done, we don't own this

    DUMMYJOB                djDummyJob;         // temporary dummy entry in Windows spooler until this job can print

    unsigned int            nDepth;             // Print depth for recursion
    unsigned int            fRootDocument:1;    // Is this an original, enqueued job (i.e. not generated)
    unsigned int            fTempFile:1;        // Is there a temporary file provided for this job
    unsigned int            grfPrintObjectType:2;// Is the print job a CPrintDoc, IPrint, or IOleCommandTarget
                                                // (used internally)
    unsigned int            fCancelled:1;       // Is this job cancelled (used internally)
    unsigned int            fFrameChild:1;      // Is this job a frame (child) of a previous job
    unsigned int            fPrintHtmlOnly:1;   // Do we know ahead of time that we are we dealing with a Trident compatible format?
    unsigned int            fLoadFinishedEarly:1;// Did load of this job finish before previous job started printing?
    unsigned int            fOverrideDocUrl:1;  // Should the url of the print doc be overwritten by the PRINTINFO cstrBaseUrl (error docs)?
    unsigned int            fInitiatingLoading:1;// Set as long as CSpooler::InitiateLoading() function executes.  Used to identify syncronous loads.
    unsigned int            fAbortLoading:1;    // Was this jobs cancelled after loading started?
    unsigned int            fDontRunScripts:1;  // Don't run scripts.
    unsigned int            fUsePrettyUrl:1;    // Are we using pretty Urls (when printing framesets in 'print all frames' mode)
    unsigned int            fAlternateDoc:1;    // Are we printing an alternate print document?
    unsigned int            fExtJobsComplete:1; // If this job is an IPrint document, was it already processed in the UI thread?  For Office docs in framesets.
    unsigned int            fNeedsDummyJob:1;   // Don't queue a dummy job in the windows print spooler
}; // tag: pi

typedef TIntrusiveDblLinkedListOfStruct<PRINTINFO> PrintQueue;


#ifdef WIN16
typedef UINT (APIENTRY * LPPAGEPAINTHOOK)( HWND, UINT, WPARAM, LPARAM );
typedef UINT (APIENTRY * LPPAGESETUPHOOK)( HWND, UINT, WPARAM, LPARAM );

typedef struct tagPSDW
{
    DWORD           lStructSize;
    HWND            hwndOwner;
    HGLOBAL         hDevMode;
    HGLOBAL         hDevNames;
    DWORD           Flags;
    POINT           ptPaperSize;
    RECT            rtMinMargin;
    RECT            rtMargin;
    HINSTANCE       hInstance;
    LPARAM          lCustData;
    LPPAGESETUPHOOK lpfnPageSetupHook;
    LPPAGEPAINTHOOK lpfnPagePaintHook;
    LPCWSTR         lpPageSetupTemplateName;
    HGLOBAL         hPageSetupTemplate;
} PAGESETUPDLGW, * LPPAGESETUPDLGW;

typedef PAGESETUPDLGW PAGESETUPDLG;
typedef LPPAGESETUPDLGW LPPAGESETUPDLG;
#endif // WIN16


struct DRIVERPRINTMODE
{
    char                    achDriverName[CCHDEVICENAME+1]; // Driver name
    DWORD                   dwPrintMode;                    // Driver specific print mode (e.g. to suppress SRCAND in image draw code)
};

//+------------------------------------------------------------------------
//
//  Helper for Print/PageSetup
//
//-------------------------------------------------------------------------

HRESULT
FormsPageSetup(
    PRINTINFOBAG * pPrintInfoBag, HWND hwndOwner, CDoc* pDoc);

HRESULT
FormsPrint(
        PRINTINFOBAG * pPrintInfoBag,
        HWND hwndOwner,
        CDoc * pDoc,
        BOOL  fInitDefaults=FALSE);

HRESULT
InitPrintHandles(
        HGLOBAL hDevMode,
        HGLOBAL hDevNames,
        DVTARGETDEVICE ** pptd,
        HDC * phic,
        DWORD * pdwPrintMode,
        HDC * phdc = NULL);

void
DeinitPrintHandles(
        DVTARGETDEVICE * ptd,
        HDC hic);

HDC
CreateDCFromDevNames(HGLOBAL hDevNames, HGLOBAL hDevMode);

typedef enum
{
    PRINTOPTSUBKEY_MAIN = 1,
    PRINTOPTSUBKEY_PAGESETUP = 2,
    PRINTOPTSUBKEY_UNIX = 3       // IEUNIX: UNIX key
}
PRINTOPTIONS_SUBKEY;

typedef enum
{
    PRINTOPT_HEADER,
    PRINTOPT_FOOTER,
    PRINTOPT_PRINT_BACKGROUND,
    PRINTOPT_SHORTCUT_TABLE
}
PRINTOPTION;

HRESULT GetRegPrintOptionsKey(PRINTOPTIONS_SUBKEY PrintSubKey, HKEY * pkeyPrintOptions);

#ifdef UNIX
HRESULT GetRegPrintNameCommandKey(PRINTOPTIONS_SUBKEY PrintSubKey, HKEY *pkeyPrintOptions);
HRESULT  GetDefPrintNameCommandKey(HKEY key, HKEY *pKey);
void WriteRegPrintNameCommand(HKEY keyNC, TCHAR * pszName, TCHAR * pszCommand);
void ReadRegPrintNameCommand(HKEY keyNC, TCHAR *pValue, TCHAR *pszName);
#endif // UNIX

CStr *  GetRegPrintOptionString(PRINTOPTION PrintOption);
HRESULT SetRegPrintOptionString(PRINTOPTION PrintOption, TCHAR * psz);

BOOL    GetRegPrintOptionBool(PRINTOPTION PrintOption);
HRESULT SetRegPrintOptionBool(PRINTOPTION PrintOption, BOOL f);

HRESULT ParseCMDLine(const TCHAR *pchIn, TCHAR **pchURL, HGLOBAL *pHDevNames, HGLOBAL *pHDevMode, HDC *pHDC);
HRESULT VerifyPrintFileName(TCHAR *pchFileName);  // filename is in/out

UINT    GetHTMLTempFileName(const TCHAR *pchPathName, const TCHAR *pchPrefixString, UINT uUnique, LPWSTR lpTempFileName, UINT uAttempts=10);

#pragma INCMSG("--- End 'putil.hxx'")
#else
#pragma INCMSG("*** Dup 'putil.hxx'")
#endif
