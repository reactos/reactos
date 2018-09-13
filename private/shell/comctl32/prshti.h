//
//  Internal data structures for property sheet support.
//

//
//  We are such morons.  Wiz97 underwent a redesign between IE4 and IE5
//  so we have to treat them as two unrelated wizard styles that happen to
//  have frighteningly similar names.  So prsht.h contains both
//  PSH_WIZARD97IE4 and PSH_WIZARD97IE5, and defines PSH_WIZARD97 to be
//  the one appropriate to the version of the header file being included.
//
//  We redefine PSH_WIZARD97 to mean "Any form of Wizard97",
//
#undef  PSH_WIZARD97
#define PSH_WIZARD97        (PSH_WIZARD97IE4 | PSH_WIZARD97IE5)

//
//  The history of PROPSHEETHEADER
//
//  PROPSHEETHEADERSIZE_BETA
//
//      This is the property sheet header that shipped in an early
//      Win95 beta (sometime between Sep 1993 and Sep 1994, maybe M5).
//
//      It is just like the shipping Win95 property sheet header,
//      except that it lacks the PFNPROPSHEETCALLBACK at the end.
//      We grudgingly accept it but don't publicize the fact.
//
//      For some reason, we have always supported this wacky
//      ancient unreleased PROPSHEETHEADER, so there's no point
//      in dropping support for it now...  If you think it's not
//      worth retaining support for this ancient structure,
//      feel free to nuke it.  But you become responsible for the
//      potential app compat bugs from Norton Utilities for
//      Windows 95 v1.0.
//
//  PROPSHEETHEADERSIZE_V1
//
//      This is the property sheet header that shipped in Win95,
//      NT4, and IE3.  It is documented and lots of people use it.
//
//  PROPSHEETHEADERSIZE_V1a
//
//      This is an interim property sheet header that never shipped.
//      Support for it has been broken for a long time, so I dropped
//      the support altogether for IE5.
//
//  PROPSHEETHEADERSIZE_V2
//
//      This is the property sheet header that shipped in IE4.
//
#define PROPSHEETHEADERSIZE_BETA    CCSIZEOF_STRUCT(PROPSHEETHEADER, H_ppsp)
#define PROPSHEETHEADERSIZE_V1      CCSIZEOF_STRUCT(PROPSHEETHEADER, pfnCallback)
#define PROPSHEETHEADERSIZE_V2      CCSIZEOF_STRUCT(PROPSHEETHEADER, H_pszbmHeader)

#define IsValidPROPSHEETHEADERSIZE(dwSize) \
       ((dwSize) == PROPSHEETHEADERSIZE_BETA || \
        (dwSize) == PROPSHEETHEADERSIZE_V1   || \
        (dwSize) == PROPSHEETHEADERSIZE_V2)

// PropertySheetPage structure sizes:

#define MINPROPSHEETPAGESIZEA PROPSHEETPAGEA_V1_SIZE
#define MINPROPSHEETPAGESIZEW PROPSHEETPAGEW_V1_SIZE
#define MINPROPSHEETPAGESIZE  PROPSHEETPAGE_V1_SIZE

//  - COMPATIBILITY CONSTRAINT -
//
//  Shell32 prior to IE5 knows the internal structure of the HPROPSHEETPAGE,
//  so we have to give it what it wants.  (shell32\bitbuck.c, docfind2.c)
//
//  Win95 Golden - Shell32 expects the HPROPSHEETPAGE to be equal to
//  the lParam that is passed to the dialog proc's WM_INITDIALOG.
//  No special flags are passed in the PROPSHEETPAGE to indicate
//  that this assumption is being made.
//
//  Win95 IE4 Integrated - Same as Win95 Golden, except that
//  the shell sets the PSP_SHPAGE bit in the PROPSHEETPAGE.dwFlags
//  to indicate that it wants this wacky behavior.
//
//  WinNT Golden - Shell32 expects the HPROPSHEETPAGE to be equal
//  to a pointer to the internal PSP structure used by WinNT golden.
//  The internal PSP structure looked like a PROPSHEETPAGE, except
//  that it had two fields stuck in front.  (One DWORD and one pointer.)
//  The NT shell passes the PSP_SHPAGE flag.
//
//  WinNT IE4 Integrated - Same as WinNT Golden.
//
//  Furthermore, all versions of Shell32 prior to IE5 call the internal
//  CreatePage function (shell32\docfind2.c)
//
//  - Summary -
//
//                passes       expected      expects
//              PSP_SHPAGE       PSP         CLASSICPREFIX
//              ---------      -------       -------------
//  95 Gold                      ANSI
//  95/IE4          *            ANSI
//  NT Gold         *            UNI           *
//  NT/IE4          *            UNI           *
//
//  Note that Win95 Gold does not set the PSP_SHPAGE flag, so we have
//  to assume that any ANSI caller might be a Win95 Gold shell32.
//
//  WinNT is easier.  We return the WinNT Golden UNICODE version of
//  the PSP if (and only if) the PSP_SHPAGE flag is set.
//
//  The PSP_SHPAGE flag has been removed from the header file so nobody
//  can pass it ever again.
//
//  So our structures look like this.  The bracketed section is the
//  the memory block passed by the app to CreatePropertySheetPage
//  and whose layout cannot be altered.
//
//  ANSI Comctl32, ANSI application:
//
//            +---------------+
//            | PAGEPREFIX    |
//  hpage95->/+---------------+
//          / | PROPSHEETPAGE |
//          | |  (ANSI)       |
//          | |               |
//          | +---------------+
//          | |               |
//          | | app goo       |
//          \ |               |
//           \+---------------+
//
//  UNICODE Comctl32, ANSI application.
//
//
//          The authoritative page      The shadow page
//
//            +---------------+         + - - - - - - - +
//            | PAGEPREFIX    |         | PAGEPREFIX    |
//            +---------------+         +---------------+ <- hpageNT
//            | CLASSICPREFIX |<-\/-----| CLASSICPREFIX |
//            +---------------+<-/\---->+---------------+\<- hpage95
//            | PROPSHEETPAGE |         | PROPSHEETPAGE | \
//            |  (UNICODE)    |         |  (ANSI)       | |
//            |               |         |               | |
//            +---------------+         +---------------+ |
//                                      |               | |
//                                      | app goo       | |
//                                      |               | /
//                                      +---------------+/
//
//                                  (The dotted line around PAGEPREFIX
//                                   means that it is allocated but unused.)
//
//
//  UNICODE Comctl32, UNICODE application.
//
//            +---------------+
//            | PAGEPREFIX    |
//  hpageNT-> +---------------+
//            | CLASSICPREFIX |
//  hpage95->/+---------------+
//          / | PROPSHEETPAGE |
//          | |  (UNICODE)    |
//          | |               |
//          | +---------------+
//          | |               |
//          | | app goo       |
//          \ |               |
//           \+---------------+
//
//  Are we confused yet?  Let's try to explain.
//
//  REQUIREMENT
//
//      The app goo must be kept in the structure
//      corresponding to the character set of the application.
//
//      Notice that if the application is ANSI, then the app goo
//      is kept with the ANSI version of PROPSHEETPAGE.  If the
//      application is UNICODE, then the app goo is kept with the
//      UNICODE version of the PROPSHEETPAGE.
//
//      It doesn't hurt to "accidentally" put a copy of the app goo
//      on the version the app doesn't use; that just wastes memory.
//
//  REQUIREMENT
//
//      If a UNICODE app passed PSP_SHPAGE, then the hpage must
//      point to the CLASSICPREFIX structure.
//
//      To simplify matters (like HPROPSHEETPAGE validation), we
//      apply this rule even if the app didn't pass PSP_SHPAGE.
//
//  DESIGN
//
//      If the app is ANSI and we support UNICODE, then we create
//      a UNICODE copy of the ANSI property sheet structure,
//      and the ANSI PROPSHEETPAGE becomes a "shadow".
//      The UNICODE copy does not need to carry the app goo
//      since it will never be seen by the app.
//
//  REQUIREMENT
//
//      If an ANSI app creates a property sheet page, then the hpage
//      must point to the ANSI version of the PROPSHEETPAGE.
//      (Because the app might be shell32.)
//
//  >> CAUTION <<
//
//      The requirements on hpages rule means that any time an hpage
//      comes in from the outside world, we need to sniff it and decide
//      if it's the UNICODE version or the ANSI version; if it's
//      the ANSI version, then we switch the pointer to point to
//      the UNICODE version instead.
//
//  REMARK
//
//      Internally, we use only the UNICODE version of the PROPSHEETPAGE.
//      (Unless we're building Win95 ANSI-only, duh.)  The ANSI version
//      (the "shadow") is just for show to keep the app happy.  It is
//      the UNICODE version that is authoritative.
//
//      Only the authoritative PROPSHEETPAGE needs to have the PAGEPREFIX,
//      but we put one on both sides to simplify memory management,
//      because it means that all PROPSHEETPAGEs look the same (both
//      authoritative and shadow).

#define PSP_SHPAGE                 0x00000200  // Ewww; see above

#ifdef UNICODE
//
//  CLASSICPREFIX
//
//  This structure is allocated ahead of the PROPSHEETPAGE when we
//  create an HPROPSHEETPAGE.  See the diagrams above.  Sometimes
//  the HPROPSHEETPAGE points to this structure, sometimes it doesn't.
//  See the diagrams above.
//
//  This structure can never change, due to backwards compatibility
//  constraints described above.  (Okay, you can change it once you
//  decide to drop support for versions of NT less than 5, like that'll
//  ever happen.)

//
//  pispMain
//
//      Points to the main copy of the HPROPSHEETPAGE.
//
//  pispShadow
//
//      Points to that shadow copy of the HPROPSHEETPAGE, or NULL if
//      there is no shadow copy.

typedef struct CLASSICPREFIX {
    union ISP *pispMain;
    union ISP *pispShadow;
} CLASSICPREFIX, *PCLASSICPREFIX;

#endif // UNICODE

//
//  PAGEPREFIX
//
//  Stuff that we track which isn't part of the CLASSICPREFIX.
//
//  hpage is the HPROPSHEETPAGE that we give out to applications.
//

typedef struct PAGEPREFIX {
    HPROPSHEETPAGE hpage;
    DWORD dwInternalFlags;
    SIZE siz;                           // Page ideal size
} PAGEPREFIX, *PPAGEPREFIX;

//
//  Flag values for dwInternalFlags
//

#define PSPI_WX86               1
#define PSPI_FETCHEDICON        2       // For debugging (GetPageInfoEx)

//
//  _PSP
//
//  This is the structure than the compiler thinks an HPROPSHEETPAGE
//  points to.  To make sure all our code goes through
//  InternalizeHPROPSHEETPAGE on the way in and
//  ExternalizeHPROPSHEETPAGE on the way out, we intentionally leave
//  it undefined.

typedef struct _PSP PSP, *PPSP;

//
//  ISP - Internal Sheet Page
//
//  Our internal structure for tracking property sheet pages.  This
//  is also what a native-character set HPROPSHEETPAGE points to.
//
//  Note the "union with an array of one element that we index with
//  the value -1 in order to access it at negative offsets" trick.
//
//  Note also that the CLASSICPREFIX goes above the HPROPSHEETPAGE
//  on Win95, but below it on WinNT.  See discussion at the top of this
//  file.
//
//  To save all the typing of union names and [-1]'s, access to fields
//  of an IPSP are encapsulated inside the _psp, _cpfx, and _pfx macros.

typedef union ISP {
    struct {
        PAGEPREFIX pfx;             // lives above the HPROPSHEETPAGE
        #ifdef UNICODE_WIN9x
        CLASSICPREFIX cpfx;         // lives above the HPROPSHEETPAGE
        #endif
    } above[1];
    struct {
        #ifdef WINNT
        CLASSICPREFIX cpfx;         // lives below the HPROPSHEETPAGE
        #endif
        PROPSHEETPAGE psp;          // lives below the HPROPSHEETPAGE
    } below;
} ISP, *PISP;

#define _pfx    above[-1].pfx
#define _psp    below.psp
#ifdef WINNT
#define _cpfx   below.cpfx
#else
#define _cpfx   above[-1].cpfx
#endif

#define PropSheetBase(pisp)     ((LPBYTE)(pisp) - sizeof((PISP)pisp)->above)

//
//  Converting an HPROPSHEETPAGE into a PSP means sniffing at the
//  _cpfx.dwFlags and seeing if it's an ANSI page or a UNICODE page.
//

__inline
PISP
InternalizeHPROPSHEETPAGE(HPROPSHEETPAGE hpage)
{
    PISP pisp = (PISP)hpage;
#ifdef UNICODE
    return pisp->_cpfx.pispMain;
#else // !UNICODE
    //  ANSI Comctl32's HPROPSHEETPAGE is the real thing
    return pisp;
#endif
}

#ifdef UNICODE
#define ExternalizeHPROPSHEETPAGE(pisp) ((pisp)->_pfx.hpage)
#else
#define ExternalizeHPROPSHEETPAGE(pisp) ((HPROPSHEETPAGE)(pisp))
#endif

//
// Used for GetPageInfo(), prpage.c
//
typedef struct {
    short     PointSize;
    WCHAR     szFace[LF_FACESIZE];
    BOOL      bItalic;
    int       iCharset;
} PAGEFONTDATA, * PPAGEFONTDATA;

//
//  PROPDATA
//
//  The state of a property sheet.
//

typedef struct
{
    HWND hDlg;          // the dialog for this instance data
    PROPSHEETHEADER psh;

    HWND hwndCurPage;   // current page hwnd
    HWND hwndTabs;      // tab control window
    int nCurItem;       // index of current item in tab control
    int idDefaultFallback; // the default id to set as DEFID if page doesn't have one

    int nReturn;
    UINT nRestart;

    int xSubDlg, ySubDlg;       // dimensions of sub dialog
    int cxSubDlg, cySubDlg;

    BOOL fFlags;
    BOOL fFlipped;      // Property sheet not mirrored but with flipped buttons

    // Wizard97 IE4 vs. IE5 discrepancy:
    //
    //  Wizard 97 IE4 - "watermark" refers to the bitmap that is used to
    //                  paint the background of the dialog.
    //  Wizard 97 IE5 - "watermark" refers to the bitmap that goes on
    //                  on the left-hand side of Welcome/Finish screens.
    //
  
    HBITMAP hbmWatermark;
    HBRUSH  hbrWatermark;
    HPALETTE hplWatermark;

    int cyHeaderHeight;
    HFONT hFontBold;
    HBITMAP hbmHeader;
    HBRUSH  hbrHeader;
    int ySubTitle;      // The subtitle's starting Y position
    BOOL fAllowApply;

    // These fields are used by MLUI
    LANGID wFrameLang;      // langid of propsheet frame
    int iFrameCharset;      // charset of propsheet frame

    // These fields cache font metric information
    PAGEFONTDATA    pfdCache;           // Cached font descriptor
    SIZE            sizCache;           // Cached height and width go here
    SIZE            sizMin;             // Smallest we allow pages to get

    HPROPSHEETPAGE rghpage[MAXPROPPAGES];

} PROPDATA, *LPPROPDATA;
// defines for fFlags
#define PD_NOERASE       0x0001
#define PD_CANCELTOCLOSE 0x0002
#define PD_DESTROY       0x0004
#define PD_WX86          0x0008
#define PD_FREETITLE     0x0010
#define PD_SHELLFONT     0x0020         // Is the frame using SHELLFONT?
#define PD_NEEDSHADOW    0x0040

//
//  Helper macros
//
//  UNIX does not support dummy unions, so we have to say
//  DUMMYUNION<n>_MEMBER all over the place, which sucks.
//  So all these underscore macros do the grunky work for us.
//
//  H_blah means "the field in the PROPSHEETHEADER named blah".
//  P_blah means "the field in the PROPSHEETPAGE   named blah".
//
//
#define H_hIcon             DUMMYUNION_MEMBER(hIcon)
#define H_pszIcon           DUMMYUNION_MEMBER(pszIcon)
#define H_nStartPage        DUMMYUNION2_MEMBER(nStartPage)
#define H_pStartPage        DUMMYUNION2_MEMBER(pStartPage)
#define H_phpage            DUMMYUNION3_MEMBER(phpage)
#define H_ppsp              DUMMYUNION3_MEMBER(ppsp)
#define H_hbmWatermark      DUMMYUNION4_MEMBER(hbmWatermark)
#define H_pszbmWatermark    DUMMYUNION4_MEMBER(pszbmWatermark)
#define H_hbmHeader         DUMMYUNION5_MEMBER(hbmHeader)
#define H_pszbmHeader       DUMMYUNION5_MEMBER(pszbmHeader)

#define P_pszTemplate       DUMMYUNION_MEMBER(pszTemplate)
#define P_pResource         DUMMYUNION_MEMBER(pResource)
#define P_hIcon             DUMMYUNION2_MEMBER(hIcon)
#define P_pszIcon           DUMMYUNION2_MEMBER(pszIcon)

//
//  HASCALLBACK - We should call the callback for this page.
//

#define HASCALLBACK(pisp) \
       (((pisp)->_psp.dwFlags & PSP_USECALLBACK) && \
         (pisp)->_psp.pfnCallback)

//
//  HASREFPARENT - We should adjust the pcRefParent for this page.
//

#define HASREFPARENT(pisp) \
       (((pisp)->_psp.dwFlags & PSP_USEREFPARENT) && \
         (pisp)->_psp.pcRefParent)

//
//  HASHEADERTITLE - We should display a header title for this page.
//

#define HASHEADERTITLE(pisp) \
       (((pisp)->_psp.dwFlags & PSP_USEHEADERTITLE) && \
         (pisp)->_psp.pszHeaderTitle)

//
//  HASHEADERSUBTITLE - We should display a header subtitle for this page.
//

#define HASHEADERSUBTITLE(pisp) \
       (((pisp)->_psp.dwFlags & PSP_USEHEADERSUBTITLE) && \
         (pisp)->_psp.pszHeaderSubTitle)

//
//  GETPISP - Obtain the PISP for this page.  Once they have been
//            placed into the H_phpage, all the HPROPSHEETPAGEs are
//            already internalized, so we can just cast them over.
//

#define GETPISP(ppd, i) ((PISP)(ppd)->psh.H_phpage[i])

//
//  SETPISP - Change the PISP for this page.
//

#define SETPISP(ppd, i, v) ((ppd)->psh.H_phpage[i] = (HPROPSHEETPAGE)(v))

//
//  GETHPAGE - Obtain the external HPROPSHEETPAGE for this page.
//
#define GETHPAGE(ppd, i) ExternalizeHPROPSHEETPAGE(GETPISP(ppd, i))

//
//  GETPPSP - Obtain the PPSP for this page.  We get the PISP
//            and then suck out the PROPSHEETHEADER part.
//

#define GETPPSP(ppd, i) (&GETPISP(ppd, i)->_psp)

//
//  HASANSISHADOW
//
//  Does this authoritative property sheet page have an ANSI shadow?
//
//  If we are built ANSI, then the canonical PSP is equal to the
//  ANSI version, so there is no shadow.  (It's already the real thing.)
//
#ifdef UNICODE
#define HASANSISHADOW(pisp) ((pisp)->_cpfx.pispShadow)
#else
#define HASANSISHADOW(pisp) FALSE
#endif

//
//  HIDEWIZ97HEADER
//
//      Nonzero if we are a WIZARD97 property sheet but we should
//      hide the header for this page.

#define HIDEWIZ97HEADER(ppd, i) \
        (((ppd)->psh.dwFlags & PSH_WIZARD97) && \
          (GETPPSP(ppd, i)->dwFlags & PSP_HIDEHEADER))

//
//  Stub macros so we don't have to put "#ifdef BIG_ENDIAN" everywhere.
//
#ifndef BIG_ENDIAN
#define MwReadDWORD(lpByte)   *(LPDWORD)(lpByte)
#define MwWriteDWORD(lpByte, dwValue)   *(LPDWORD)(lpByte) = dwValue
#endif

//
//  End of helper macros
//

//
//  Functions shared between prsht.c and prpage.c
//
PISP AllocPropertySheetPage(DWORD dwClientSize);
HWND _CreatePage(LPPROPDATA ppd, PISP pisp, HWND hwndParent, LANGID langidMUI);
HPROPSHEETPAGE WINAPI _CreatePropertySheetPage(LPCPROPSHEETPAGE psp, BOOL fNeedShadow, BOOL fWx86);
#ifdef UNICODE
HPROPSHEETPAGE WINAPI _Hijaak95Hack(LPPROPDATA ppd, HPROPSHEETPAGE hpage);
#else
#define _Hijaak95Hack(ppd, hpage) hpage
#endif

typedef LPTSTR (STDMETHODCALLTYPE *STRDUPPROC)(LPCTSTR ptsz);

BOOL CopyPropertyPageStrings(LPPROPSHEETPAGE ppsp, STRDUPPROC pfnStrDup);
void FreePropertyPageStrings(LPCPROPSHEETPAGE ppsp);

BOOL ThunkPropSheetHeaderAtoW (LPCPROPSHEETHEADERA ppshA,
                                LPPROPSHEETHEADERW ppsh);
void FreePropSheetHeaderW(LPPROPSHEETHEADERW ppsh);

STDAPI_(LPTSTR) StrDup_AtoW(LPCTSTR ptsz);


typedef struct 
{
    POINT pt;               // Dialog box dimensions (DLU)
    HICON hIcon;            // Page icon
    PAGEFONTDATA pfd;       // Font info
#ifdef WINDOWS_ME
    BOOL bRTL;              // If tab caption should be right to left reading
#endif
    BOOL bMirrored;            // if the page contains mirroring flags
    BOOL bDialogEx;         // Is it a DIALOGEX?
    DWORD dwStyle;          // Dialog style
    TCHAR szCaption[128 + 50];  // Caption as stored in template

} PAGEINFOEX;

//
//  These flags control which parts of the PAGEINFOEX get filled in.
//
#define GPI_PT          0x0000      // so cheap, we always fetch it
#define GPI_ICON        0x0001
#define GPI_FONT        0x0002      // PAGEFONTDATA
#define GPI_BRTL        0x0000      // so cheap, we always fetch it
#define GPI_BMIRROR     0x0000      // so cheap, we always fetch it
#define GPI_DIALOGEX    0x0000      // so cheap, we always fetch it
#define GPI_CAPTION     0x0004
#define GPI_ALL         0x0007

BOOL WINAPI GetPageInfoEx(LPPROPDATA ppd, PISP pisp, PAGEINFOEX *ppi, LANGID langidMUI, DWORD flags);

// SHELLFONT means that you are a DIALOGEX and have the DS_SHELLFONT bits set
// Although this is supported only on NT5, the flag is still meaningful on
// Win9x to indicate an implicit PSH_USEPAGEFONT.
#define IsPageInfoSHELLFONT(ppi) \
    ((ppi)->bDialogEx && DS_SHELLFONT == (DS_SHELLFONT & (ppi)->dwStyle))

// Prsht_PrepareTemplate operating systems types
// Used as array indices. Be careful !!

typedef enum {
    PSPT_OS_WIN95_BIDI,    // Win95  BiDi
    PSPT_OS_WIN98_BIDI,    // Win98  BiDi   (Or Higher)
    PSPT_OS_WINNT4_ENA,    // WinNT4 BiDi Ena, No Winnt4 BiDi loc
    PSPT_OS_WINNT5,        // WinNT5 (Or Higher)
    PSPT_OS_OTHER,         // Anything else ....
    PSPT_OS_MAX            
    } PSPT_OS;

// Prsht_PrepareTemplate property sheet type 
// Used as array indices. Be careful !!
typedef enum {
    PSPT_TYPE_MIRRORED,     // Mirrored first page OR mirrored Process
    PSPT_TYPE_ENABLED,      // First page Language is BiDi
    PSPT_TYPE_ENGLISH,      // Anything else ....
    PSPT_TYPE_MAX           
} PSPT_TYPE;

// Prsht_PrepareTemplate property sheet default behavior override
// Used as array indices. Be careful !!

typedef enum {
    PSPT_OVERRIDE_NOOVERRIDE,
    PSPT_OVERRIDE_USEPAGELANG,  // Overridden by PSH_USEPAGELANG
    PSPT_OVERRIDE_MAX
    } PSPT_OVERRIDE;

// Prsht_PrepareTemplate Preparation action
typedef enum {
    PSPT_ACTION_NOACTION,      // Don't touch whatever you've passed
    PSPT_ACTION_NOMIRRORING,   // Turn off mirroring
    PSPT_ACTION_FLIP,          // Turn off mirroring and flip
    PSPT_ACTION_LOADENGLISH,   // load English template
    PSPT_ACTION_WIN9XCOMPAT    // Tags the templae with DS_BIDI_RTL for Win9x compat
    } PSPT_ACTION;
