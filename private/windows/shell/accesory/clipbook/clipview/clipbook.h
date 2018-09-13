
/*****************************************************************************

                        C L I P B O O K   H E A D E R

    Name:       clipbook.h
    Date:       21-Jan-1994
    Creator:    Unknown

    Description:
        This is the header file for all files in clipview directory.

    History:
        21-Jan-1994     John Fu, reformat, cleanup, and removed all externs.

*****************************************************************************/




#include   <commdlg.h>
#include   <ddeml.h>
#include   <nddeapi.h>
#include   "clpbkrc.h"
#include   "vclpbrd.h"






#define SetStatusBarText(x) if(hwndStatus)SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM)(LPSTR)(x));







/**********************

    Data structures

**********************/



/*
 *  Per MDI client data
 */

struct MdiInfo {
   HCONV    hExeConv;                       // Used for sync transaction only
   HCONV    hClpConv;                       // Used for async transaction only
   HSZ      hszClpTopic;
   HSZ      hszConvPartner;
   HSZ      hszConvPartnerNP;
   HWND     hWndListbox;
   WORD     DisplayMode;
   WORD     OldDisplayMode;
   DWORD    flags;

   TCHAR    szBaseName[ (MAX_COMPUTERNAME_LENGTH+1) * 2];

   TCHAR    szComputerName[MAX_COMPUTERNAME_LENGTH + 1];

   UINT     CurSelFormat;
   LONG     cyScrollLast;
   LONG     cyScrollNow;
   int      cxScrollLast;
   int      cxScrollNow;
   RECT     rcWindow;
   WORD     cyLine, cxChar, cxMaxCharWidth; // Size of a standard text char
   WORD     cxMargin, cyMargin;             // White border size around clip data area
   BOOL     fDisplayFormatChanged;

   PVCLPBRD pVClpbrd;
   HCONV    hVClpConv;
   HSZ      hszVClpTopic;

   // scrollbars, etc. for the damn paging control

   HWND     hwndVscroll;
   HWND     hwndHscroll;
   HWND     hwndSizeBox;
   HWND     hwndPgUp;
   HWND     hwndPgDown;
};





typedef struct MdiInfo   MDIINFO;
typedef struct MdiInfo * PMDIINFO;
typedef struct MdiInfo FAR * LPMDIINFO;










/*
 *  data request record
 */

#define      RQ_PREVBITMAP   10
#define      RQ_COPY         11
#define      RQ_SETPAGE      12
#define      RQ_EXECONV      13

struct DataRequest_tag
   {
   WORD   rqType;      // one of above defines
   HWND   hwndMDI;
   HWND   hwndList;
   UINT   iListbox;
   BOOL   fDisconnect;
   WORD   wFmt;
   WORD   wRetryCnt;
   };

typedef struct DataRequest_tag DATAREQ;
typedef struct DataRequest_tag * PDATAREQ;







/*
 *  Owner draw listbox data structure
 */

#define MAX_PAGENAME_LENGTH MAX_NDDESHARENAME

struct ListEntry_tag
   {
   TCHAR name[MAX_PAGENAME_LENGTH + 1];
   HBITMAP hbmp;
   BOOL fDelete;
   BOOL fTriedGettingPreview;
   };

typedef struct ListEntry_tag LISTENTRY;
typedef struct ListEntry_tag * PLISTENTRY;
typedef struct ListEntry_tag FAR * LPLISTENTRY;








/*
 *  Extra window data for MDI child registerclass
 *  contains a pointer to above MDIINFO struct
 */

#define GWL_MDIINFO     0
#define CBWNDEXTRA      sizeof(LONG_PTR)

// per MDI window flags - used for MDIINFO.flags

#define F_LOCAL         0x00000001
#define F_CLPBRD        0x00000002

// per MDI display mode - MDIINFO.DisplayMode

#define DSP_LIST        10
#define DSP_PREV        11
#define DSP_PAGE        12






/*
 *  Data structure used to pass share info to SedCallback
 */

typedef struct
   {
   SECURITY_INFORMATION si;
   WCHAR awchCName[MAX_COMPUTERNAME_LENGTH + 3];
   WCHAR awchSName[MAX_NDDESHARENAME + 1];
   }
   SEDCALLBACKCONTEXT;








/*
 *  Data structure passed to KeepAsDialogProc (Paste)
 */

typedef struct
    {
    BOOL    bAlreadyExist;
    BOOL    bAlreadyShared;
    TCHAR   ShareName[MAX_NDDESHARENAME +2];
    }
    KEEPASDLG_PARAM, *PKEEPASDLG_PARAM;






/*************

    Macros

*************/



#define PRIVATE_FORMAT(fmt)     ((fmt) >= 0xC000)
#define GETMDIINFO(x)           (x? (PMDIINFO)(GetWindowLongPtr((x),GWL_MDIINFO)): NULL)



// parameter codes for MyGetFormat()
#define GETFORMAT_LIE       200
#define GETFORMAT_DONTLIE   201


// default DDEML synchronous transaction timeouts
// note these should be generous
#define SHORT_SYNC_TIMEOUT  (24L*1000L)
#define LONG_SYNC_TIMEOUT   (60L*1000L)


// owner draw listbox and bitmap metrics constants
#define LSTBTDX             16              // width of folder ( with or without hand )
#define LSTBTDY             16              // height of folder ( with or without hand )

#define SHR_PICT_X          0               // offsets of shared folder bitmap
#define SHR_PICT_Y          0
#define SAV_PICT_X          16              // offsets of non-shared folder bitmap
#define SAV_PICT_Y          0

#define PREVBRD             4               // border around preview bitmaps

#define BTNBARBORDER        2
#define DEF_WIDTH           400             // initial app size
#define DEF_HEIGHT          300

#define SZBUFSIZ            MAX_DDE_EXEC




// combined styles for owner draw listbox variants
#define LBS_LISTVIEW        (LBS_OWNERDRAWFIXED|LBS_DISABLENOSCROLL)
#define LBS_PREVIEW         (LBS_MULTICOLUMN|LBS_OWNERDRAWFIXED)





//////////// compile options //////////////
#define MAX_ALLOWED_PAGES       127



#define WINDOW_MENU_INDEX       4
#define DISPLAY_MENU_INDEX      3   // submenu to put format entries i.e. "&Text"
#define SECURITY_MENU_INDEX     2

