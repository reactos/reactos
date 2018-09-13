/* Header file for Print File */

#define PRINTREC         struct PrintType
#define LPPRINTREC       PRINTREC FAR *

LPPRINTREC    lpPrintFile;
HANDLE       hPrintFile;

VOID PrintFileString(LPSTR, LONG, BOOL);
BOOL PrintFileControl(WORD  msg, WPARAM wParam, DWORD lParam );
BOOL  APIENTRY PrintFileComm(BOOL);

struct PrintType
       {
       BOOL    active;         /* Printer active flag used for pause/resume */
       BOOL    selectPrintActive; /* Selection printing active */
       BOOL    filePrintActive;   /* File printing active */
       SHORT   openCount;      /* Counter to test for balance of open/close */
       INT     fileio;         /* File iochannel */
       HFONT   hFont;            /* current print font */  
       LOGFONT font;             /* Logical font sturcture */
       INT     point;            /* Point size */
       SHORT   nLineHeight;      /* Real line height includes leading */
       INT   pageWidth;
       INT   pageHeight;
       INT   pageLength;         /* Length of page in lines */
       INT   lineLength;         /* Length of line on a page */
       INT   prtLine;            /* Current line being printed */
       INT   charCount;          /* Current position in line */
       INT   pixCount;           /* Current position in pixels */
       INT   pixColCount;        /* Current col position of pixels */
       INT   tab;                /* Tab value */
       BOOL  CRtoLF;             /* True translate CR to LF */
       BYTE  title[STR255];         /* Title of document to print */
       BYTE  lineBuffer[STR255];    /* Current line buffer */
		 BYTE  tmpFile[STR255];    /* Temp file name jtfnew */
       BOOL  cancelAbort;        /* Cancel printing flag */
       HDC   hPrintDC;           /* Handle to printer DC */
       HWND  hAbortDlg;          /* Handle to abort dialog box */
       FARPROC lpAbortDlg;
       FARPROC lpabortDlgProc;
       };


#define CR     13               /* Carriage return              */
#define LF     10               /* Line Feed                    */
#define FF     12               /* Form Feed                    */
#define TAB    9                /* Tab                          */
#define TABMAX 20               /* Max tab size                 */


#define PRINTFILEBOLD         0x0001
#define PRINTFILEUNDERLINE    0x0002
#define PRINTFILEITALIC       0x0004
#define PRINTFILESTRIKEOUT    0x0008
#define PRINTFILEQUALITY      0x0010
#define PRINTFILECRTOLF       0x0020
#define PRINTFILENORMAL       0x0040
#define PRINTFILETAB          0x0080
#define PRINTFILESETFONT      0x0100
#define PRINTFILEFONTFACE     0x0200
