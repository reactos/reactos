;*************************************************************************
;
;       WINDOWS.INC - Windows assembly language structures & constants
;
; Copyright (c) 1985-1994, Microsoft Corp.	All rights reserved.
;*************************************************************************
;
; Conditional Block includes:   (True states)
;     NOTEXT - don't include TextMetric struc & text drawing modes & stock objs.
;     NORASTOPS - don't include binary and ternary raster ops.
;     NOVK      - don't include virtual key definitions
;     NOMB      - don't include message box definitions
;     NOWM      - don't include window messages
;
;
FALSE   =       0
TRUE    =       1
NULL    =       0

;*******************************************************************
;
;       Rectangle
;
;*******************************************************************

RECT    struc
        rcLeft          dw      ?
        rcTop           dw      ?
        rcRight         dw      ?
        rcBottom        dw      ?
RECT    ends

;*******************************************************************
;
;  Window Class structure
;
;*******************************************************************

WNDCLASS struc
        clsStyle        dw      ?       ; class style
        clsLpfnWndProc  dd      ?
        clsCbClsExtra   dw      ?
        clsCbWndExtra   dw      ?
        clsHInstance    dw      ?       ; instance handle
        clsHIcon        dw      ?       ; class icon handle
        clsHCursor      dw      ?       ; class cursor handle
        clsHbrBackground dw     ?       ; class background brush
        clsLpszMenuName dd      ?       ; menu name
        clsLpszClassName dd     ?       ; far ptr to class name
WNDCLASS ends

IFNDEF NOTEXT
TEXTMETRIC struc
    tmHeight	    dw	    ?
    tmAscent	    dw	    ?
    tmDescent	    dw	    ?
    tmIntLeading    dw	    ?
    tmExtLeading    dw	    ?
    tmAveCharWidth  dw	    ?
    tmMaxCharWidth  dw	    ?
    tmWeight	    dw	    ?
    tmItalic	    db	    ?
    tmUnderlined    db	    ?
    tmStruckOut     db	    ?
    tmFirstChar     db	    ?
    tmLastChar	    db	    ?
    tmDefaultChar   db	    ?
    tmBreakChar     db	    ?
    tmPitch	    db	    ?
    tmCharSet	    db	    ?
    tmOverhang	    dw	    ?
    tmAspectX	    dw	    ?
    tmAspectY	    dw	    ?
TEXTMETRIC ends

LF_FACESIZE	EQU	32

LOGFONT struc
    lfHeight	      dw   ?
    lfWidth	      dw   ?
    lfEscapement      dw   ?
    lfOrientation     dw   ?
    lfWeight	      dw   ?
    lfItalic	      db   ?
    lfUnderline       db   ?
    lfStrikeOut       db   ?
    lfCharSet	      db   ?
    lfOutPrecision    db   ?
    lfClipPrecision   db   ?
    lfQuality	      db   ?
    lfPitchAndFamily  db   ?
    lfFaceName	      db   LF_FACESIZE dup(?)
LOGFONT ends

LOGBRUSH struc
    lbStyle         dw ?
    lbColor         dd ?
    lbHatch         dw ?
LOGBRUSH ends

;
;  Text Drawing modes
;
TRANSPARENT     = 1
OPAQUE          = 2
;
; Mapping Modes
;
MM_TEXT         =   1
MM_LOMETRIC     =   2
MM_HIMETRIC     =   3
MM_LOENGLISH    =   4
MM_HIENGLISH    =   5
MM_TWIPS        =   6
MM_ISOTROPIC    =   7
MM_ANISOTROPIC  =   8
;
; Coordinate Modes
;
ABSOLUTE        =   1
RELATIVE        =   2
;
;  Stock Logical Objects
;
WHITE_BRUSH         =  0
LTGRAY_BRUSH        =  1
GRAY_BRUSH          =  2
DKGRAY_BRUSH        =  3
BLACK_BRUSH         =  4
NULL_BRUSH          =  5
HOLLOW_BRUSH        =  5
WHITE_PEN           =  6
BLACK_PEN           =  7
NULL_PEN            =  8
DOT_MARKER          =  9
OEM_FIXED_FONT      = 10
ANSI_FIXED_FONT     = 11
ANSI_VAR_FONT       = 12
SYSTEM_FONT         = 13
DEVICE_DEFAULT_FONT = 14
DEFAULT_PALETTE     = 15
SYSTEM_FIXED_FONT   = 16
ENDIF
;
; Brush Styles
;
BS_SOLID        =   0
BS_NULL         =   1
BS_HOLLOW       =   BS_NULL
BS_HATCHED      =   2
BS_PATTERN      =   3
BS_INDEXED      =   4
BS_DIBPATTERN	=   5
;
; Hatch Styles
;
HS_HORIZONTAL   =   0       ; -----
HS_VERTICAL     =   1       ; |||||
HS_FDIAGONAL    =   2       ; \\\\\
HS_BDIAGONAL    =   3       ; /////
HS_CROSS        =   4       ; +++++
HS_DIAGCROSS    =   5       ; xxxxx
;
; Pen Styles
;
PS_SOLID        =   0
PS_DASH         =   1       ; -------
PS_DOT          =   2       ; .......
PS_DASHDOT      =   3       ; _._._._
PS_DASHDOTDOT   =   4       ; _.._.._
PS_NULL         =   5
PS_INSIDEFRAME  =   6
;
; Device Parameters for GetDeviceCaps()
;
DRIVERVERSION =0     ; Device driver version
TECHNOLOGY    =2     ; Device classification
HORZSIZE      =4     ; Horizontal size in millimeters
VERTSIZE      =6     ; Vertical size in millimeters
HORZRES       =8     ; Horizontal width in pixels
VERTRES       =10    ; Vertical width in pixels
BITSPIXEL     =12    ; Number of bits per pixel
PLANES        =14    ; Number of planes
NUMBRUSHES    =16    ; Number of brushes the device has
NUMPENS       =18    ; Number of pens the device has
NUMMARKERS    =20    ; Number of markers the device has
NUMFONTS      =22    ; Number of fonts the device has
NUMCOLORS     =24    ; Number of colors the device supports
PDEVICESIZE   =26    ; Size required for device descriptor
CURVECAPS     =28    ; Curve capabilities
LINECAPS      =30    ; Line capabilities
POLYGONALCAPS =32    ; Polygonal capabilities
TEXTCAPS      =34    ; Text capabilities
CLIPCAPS      =36    ; Clipping capabilities
RASTERCAPS    =38    ; Bitblt capabilities
ASPECTX       =40    ; Length of the X leg
ASPECTY       =42    ; Length of the Y leg
ASPECTXY      =44    ; Length of the hypotenuse

LOGPIXELSX    =88    ; Logical pixels/inch in X
LOGPIXELSY    =90    ; Logical pixels/inch in Y

SIZEPALETTE   =104   ; Number of entries in physical palette
NUMRESERVED   =106   ; Number of reserved entries in palette
COLORRES      =108   ; Actual color resolution
;
ifndef NOGDICAPMASKS
;
; Device Capability Masks:
;
; Device Technologies
DT_PLOTTER       =   0  ; /* Vector plotter                   */
DT_RASDISPLAY    =   1  ; /* Raster display                   */
DT_RASPRINTER    =   2  ; /* Raster printer                   */
DT_RASCAMERA     =   3  ; /* Raster camera                    */
DT_CHARSTREAM    =   4  ; /* Character-stream, PLP            */
DT_METAFILE      =   5  ; /* Metafile, VDM                    */
DT_DISPFILE      =   6  ; /* Display-file                     */
;
; Curve Capabilities
CC_NONE          =   0  ; /* Curves not supported             */
CC_CIRCLES       =   1  ; /* Can do circles                   */
CC_PIE           =   2  ; /* Can do pie wedges                */
CC_CHORD         =   4  ; /* Can do chord arcs                */
CC_ELLIPSES      =   8  ; /* Can do ellipese                  */
CC_WIDE          =   16 ; /* Can do wide lines                */
CC_STYLED        =   32 ; /* Can do styled lines              */
CC_WIDESTYLED    =   64 ; /* Can do wide styled lines         */
CC_INTERIORS     =   128; /* Can do interiors                 */
;
; Line Capabilities
LC_NONE          =   0  ; /* Lines not supported              */
LC_POLYLINE      =   2  ; /* Can do polylines                 */
LC_MARKER        =   4  ; /* Can do markers                   */
LC_POLYMARKER    =   8  ; /* Can do polymarkers               */
LC_WIDE          =   16 ; /* Can do wide lines                */
LC_STYLED        =   32 ; /* Can do styled lines              */
LC_WIDESTYLED    =   64 ; /* Can do wide styled lines         */
LC_INTERIORS     =   128; /* Can do interiors                 */
;
; Polygonal Capabilities
PC_NONE          =   0  ; /* Polygonals not supported         */
PC_POLYGON       =   1  ; /* Can do polygons                  */
PC_RECTANGLE     =   2  ; /* Can do rectangles                */
PC_WINDPOLYGON   =   4  ; /* Can do winding polygons          */
PC_TRAPEZOID     =   4  ; /* Can do trapezoids                */
PC_SCANLINE      =   8  ; /* Can do scanlines                 */
PC_WIDE          =   16 ; /* Can do wide borders              */
PC_STYLED        =   32 ; /* Can do styled borders            */
PC_WIDESTYLED    =   64 ; /* Can do wide styled borders       */
PC_INTERIORS     =   128; /* Can do interiors                 */
PC_POLYPOLYGON   =   256; /* Can do PolyPolygons              */

;
; Polygonal Capabilities */
CP_NONE          =   0  ; /* No clipping of output            */
CP_RECTANGLE     =   1  ; /* Output clipped to rects          */
;
; Text Capabilities
TC_OP_CHARACTER  =   0001h ; /* Can do OutputPrecision   CHARACTER      */
TC_OP_STROKE     =   0002h ; /* Can do OutputPrecision   STROKE         */
TC_CP_STROKE     =   0004h ; /* Can do ClipPrecision     STROKE         */
TC_CR_90         =   0008h ; /* Can do CharRotAbility    90             */
TC_CR_ANY        =   0010h ; /* Can do CharRotAbility    ANY            */
TC_SF_X_YINDEP   =   0020h ; /* Can do ScaleFreedom      X_YINDEPENDENT */
TC_SA_DOUBLE     =   0040h ; /* Can do ScaleAbility      DOUBLE         */
TC_SA_INTEGER    =   0080h ; /* Can do ScaleAbility      INTEGER        */
TC_SA_CONTIN     =   0100h ; /* Can do ScaleAbility      CONTINUOUS     */
TC_EA_DOUBLE     =   0200h ; /* Can do EmboldenAbility   DOUBLE         */
TC_IA_ABLE       =   0400h ; /* Can do ItalisizeAbility  ABLE           */
TC_UA_ABLE       =   0800h ; /* Can do UnderlineAbility  ABLE           */
TC_SO_ABLE       =   1000h ; /* Can do StrikeOutAbility  ABLE           */
TC_RA_ABLE       =   2000h ; /* Can do RasterFontAble    ABLE           */
TC_VA_ABLE       =   4000h ; /* Can do VectorFontAble    ABLE           */
TC_RESERVED      =   8000h
;
; Raster Capabilities
RC_BITBLT        =   1      ; /* Can do standard BLT.             */
RC_BANDING       =   2      ; /* Device requires banding support  */
RC_SCALING       =   4      ; /* Device requires scaling support  */
RC_BITMAP64      =   8      ; /* Device can support >64K bitmap   */
RC_GDI20_OUTPUT  =   0010h  ; /* has 2.0 output calls         */
RC_DI_BITMAP     =   0080h  ; /* supports DIB to memory       */
RC_PALETTE       =   0100h  ; /* supports a palette           */
RC_DIBTODEV      =   0200h  ; /* supports DIBitsToDevice      */
RC_BIGFONT       =   0400h  ; /* supports >64K fonts          */
RC_STRETCHBLT    =   0800h  ; /* supports StretchBlt          */
RC_FLOODFILL     =   1000h  ; /* supports FloodFill           */
RC_STRETCHDIB    =   2000h  ; /* supports StretchDIBits       */

endif       ;NOGDICAPMASKS

; palette entry flags
;
PC_RESERVED     = 1    ;/* palette index used for animation */
PC_EXPLICIT     = 2    ;/* palette index is explicit to device */
PC_NOCOLLAPSE	= 4    ;/* do not match color to system palette */

; DIB color table identifiers
;
DIB_RGB_COLORS  = 0    ;/* color table in RGBTriples */
DIB_PAL_COLORS  = 1    ;/* color table in palette indices */
;

;constants for Get/SetSystemPaletteUse()
;
SYSPAL_STATIC	= 1
SYSPAL_NOSTATIC	= 2

; constants for CreateDIBitmap
CBM_INIT        = 4    ;/* initialize bitmap */
;
; Bitmap format constants
BI_RGB          = 0
BI_RLE8         = 1
BI_RLE4         = 2
;
;
ANSI_CHARSET	    = 0
SYMBOL_CHARSET	    = 2
SHIFTJIS_CHARSET    = 128   ;/* Japan  */
HANGEUL_CHARSET     = 129   ;/* Korea  */
GB2312_CHARSET	    = 134   ;/* China  */
CHINESEBIG5_CHARSET = 136   ;/* Taiwan */
OEM_CHARSET	    = 255
;
;  styles for CombineRgn
;
RGN_AND  = 1
RGN_OR   = 2
RGN_XOR  = 3
RGN_DIFF = 4
RGN_COPY = 5
;
;  Predefined cursor & icon IDs
;
IDC_ARROW       = 32512
IDC_IBEAM       = 32513
IDC_WAIT        = 32514
IDC_CROSS       = 32515
IDC_UPARROW     = 32516
IDC_SIZE        = 32640
IDC_ICON        = 32641
IDC_SIZENWSE    = 32642
IDC_SIZENESW    = 32643
IDC_SIZEWE      = 32644
IDC_SIZENS      = 32645
IDC_APPSTARTING = 32650

IDI_APPLICATION = 32512
IDI_HAND        = 32513
IDI_QUESTION    = 32514
IDI_EXCLAMATION = 32515
IDI_ASTERISK    = 32516

;
; OEM Resource Ordinal Numbers */
;
OBM_CLOSE         =  32754
OBM_UPARROW       =  32753
OBM_DNARROW       =  32752
OBM_RGARROW       =  32751
OBM_LFARROW       =  32750
OBM_REDUCE        =  32749
OBM_ZOOM          =  32748
OBM_RESTORE       =  32747
OBM_REDUCED       =  32746
OBM_ZOOMD         =  32745
OBM_RESTORED      =  32744
OBM_UPARROWD      =  32743
OBM_DNARROWD      =  32742
OBM_RGARROWD      =  32741
OBM_LFARROWD      =  32740
OBM_MNARROW       =  32739
OBM_COMBO         =  32738
OBM_UPARROWI	  =  32737
OBM_DNARROWI	  =  32736
OBM_RGARROWI	  =  32735
OBM_LFARROWI	  =  32734

OBM_OLD_CLOSE     =  32767
OBM_SIZE          =  32766
OBM_OLD_UPARROW   =  32765
OBM_OLD_DNARROW   =  32764
OBM_OLD_RGARROW   =  32763
OBM_OLD_LFARROW   =  32762
OBM_BTSIZE        =  32761
OBM_CHECK         =  32760
OBM_CHECKBOXES    =  32759
OBM_BTNCORNERS    =  32758
OBM_OLD_REDUCE    =  32757
OBM_OLD_ZOOM      =  32756
OBM_OLD_RESTORE   =  32755

OCR_NORMAL        =  32512
OCR_IBEAM         =  32513
OCR_WAIT          =  32514
OCR_CROSS         =  32515
OCR_UP            =  32516
OCR_SIZE          =  32640
OCR_ICON          =  32641
OCR_SIZENWSE      =  32642
OCR_SIZENESW      =  32643
OCR_SIZEWE        =  32644
OCR_SIZENS        =  32645
OCR_SIZEALL       =  32646
OCR_ICOCUR        =  32647

OIC_SAMPLE        =  32512
OIC_HAND          =  32513
OIC_QUES          =  32514
OIC_BANG          =  32515
OIC_NOTE          =  32516

;
;   Scroll bar constants
;
SB_HORZ = 0
SB_VERT = 1
SB_CTL  = 2
SB_BOTH = 3
;
;   Scroll Commands
;
SB_LINEUP        = 0
SB_LINEDOWN      = 1
SB_PAGEUP        = 2
SB_PAGEDOWN      = 3
SB_THUMBPOSITION = 4
SB_THUMBTRACK    = 5
SB_TOP           = 6
SB_BOTTOM        = 7
SB_ENDSCROLL     = 8
;
;  MessageBox type flags
;
IFNDEF                  NOMB
MB_OK                   = 0000H
MB_OKCANCEL             = 0001H
MB_ABORTRETRYIGNORE     = 0002H
MB_YESNOCANCEL          = 0003H
MB_YESNO                = 0004H
MB_RETRYCANCEL          = 0005H

MB_ICONHAND             = 0010H
MB_ICONQUESTION         = 0020H
MB_ICONEXCLAMATION      = 0030H
MB_ICONASTERISK         = 0040H

MB_DEFBUTTON1           = 0000H
MB_DEFBUTTON2           = 0100H
MB_DEFBUTTON3           = 0200H

MB_APPLMODAL            = 0000H
MB_SYSTEMMODAL          = 1000H
MB_TASKMODAL            = 2000H

MB_NOFOCUS              = 8000H

;
;  Conventional dialog box and message box command IDs
;
IDOK     =   1
IDCANCEL =   2
IDABORT  =   3
IDRETRY  =   4
IDIGNORE =   5
IDYES    =   6
IDNO     =   7
;
;  Flags for OpenFile
;
OF_READ 	    = 0000H
OF_WRITE	    = 0001H
OF_READWRITE	    = 0002H
OF_SHARE_COMPAT	    = 0000H
OF_SHARE_EXCLUSIVE  = 0010H
OF_SHARE_DENY_WRITE = 0020H
OF_SHARE_DENY_READ  = 0030H
OF_SHARE_DENY_NONE  = 0040H
OF_PARSE	    = 0100H
OF_DELETE	    = 0200H
OF_VERIFY	    = 0400H     ; Used with OF_REOPEN
OF_SEARCH	    = 0400H     ; Used without OF_REOPEN
OF_CANCEL           = 0800H
OF_CREATE           = 1000H
OF_PROMPT           = 2000H
OF_EXIST            = 4000H
OF_REOPEN           = 8000H

TF_FORCEDRIVE   = 80H

OPENSTRUC       STRUC
opLen   db      ?
opDisk  db      ?
opXtra  dw      ?
opDate  dw      ?
opTime  dw      ?
opFile  db      120 dup (?)
OPENSTRUC       ENDS
;
;  DrawText format flags
;
DT_LEFT         = 00H
DT_CENTER       = 01H
DT_RIGHT        = 02H
DT_TOP          = 00H
DT_VCENTER      = 04H
DT_BOTTOM       = 08H
DT_WORDBREAK    = 10H
DT_SINGLELINE   = 20H
DT_EXPANDTABS   = 40H
DT_TABSTOP      = 80H
DT_NOCLIP       =    0100H
DT_EXTERNALLEADING = 0200H
DT_CALCRECT     =    0400H
DT_NOPREFIX     =    0800H
DT_INTERNAL     =    1000H
ENDIF

;
; ExtFloodFill style flags
;
FLOODFILLBORDER  =  0
FLOODFILLSURFACE =  1

;
; Memory manager flags
;
LMEM_FIXED      =   0000h
LMEM_MOVEABLE   =   0002h
LMEM_NOCOMPACT  =   0010H
LMEM_NODISCARD  =   0020H
LMEM_ZEROINIT   =   0040h
LMEM_MODIFY     =   0080H
LMEM_DISCARDABLE=   0F00h
LHND    =    LMEM_MOVEABLE+LMEM_ZEROINIT
LPTR    =    LMEM_FIXED+LMEM_ZEROINIT
; Flags returned by LocalFlags (in addition to LMEM_DISCARDABLE)
LMEM_DISCARDED  =   4000H
LMEM_LOCKCOUNT  =   00FFH

NONZEROLHND     =    LMEM_MOVEABLE
NONZEROLPTR     =    LMEM_FIXED

GMEM_FIXED      =   0000h
GMEM_MOVEABLE   =   0002h
GMEM_NOCOMPACT  =   0010h
GMEM_NODISCARD  =   0020h
GMEM_ZEROINIT   =   0040h
GMEM_MODIFY     =   0080h
GMEM_DISCARDABLE=   0100h
GMEM_NOT_BANKED =   1000h
GMEM_DDESHARE   =   2000h
GMEM_SHARE	=   2000h
GMEM_NOTIFY     =   4000h
GMEM_LOWER      =   GMEM_NOT_BANKED
GHND            =   GMEM_MOVEABLE+GMEM_ZEROINIT
GPTR            =   GMEM_FIXED+GMEM_ZEROINIT

; Flags returned by GlobalFlags (in addition to GMEM_DISCARDABLE)
GMEM_DISCARDED  =    4000h
GMEM_LOCKCOUNT  =    00FFh

; Flags returned by GetWinFlags

WF_PMODE        =    0001h
WF_CPU286       =    0002h
WF_CPU386       =    0004h
WF_CPU486       =    0008h
WF_STANDARD	=    0010h
WF_WIN286	=    0010h
WF_ENHANCED	=    0020h
WF_WIN386	=    0020h
WF_CPU086	=    0040h
WF_CPU186	=    0080h
WF_CPUR4000     =    0100h
WF_RESERVED200  =    0200h
WF_80x87	=    0400h
WF_PAGING	=    0800h
WF_DOSPAGING    =    1000h
WF_WINNT        =    4000h
WF_WLO          =    8000h
WF_CPUMASK      =    0FC00h	; mask against hiword of GetWinFlags
WF_CPU_X86      =    0
WF_CPU_R4000    =    1
WF_CPU_ALPHA    =    2
WF_CPU_CLIPPER  =    3

; WEP fSystemExit flag values
WEP_SYSTEM_EXIT	=	1
WEP_FREE_DLL	=	0

;  Virtual Keys, Standard Set

IFNDEF          NOVK
VK_LBUTTON      = 01H
VK_RBUTTON      = 02H
VK_CANCEL       = 03H
VK_BACK         = 08H
VK_TAB          = 09H
VK_CLEAR        = 0cH
VK_RETURN       = 0dH
VK_SHIFT        = 10H
VK_CONTROL      = 11H
VK_MENU         = 12H
VK_PAUSE        = 13H
VK_CAPITAL      = 14H
VK_ESCAPE       = 1bH
VK_SPACE        = 20H

VK_PRIOR        = 21H
VK_NEXT         = 22H
VK_END          = 23H
VK_HOME         = 24H
VK_LEFT         = 25H
VK_UP           = 26H
VK_RIGHT        = 27H
VK_DOWN         = 28H

;  VK_A thru VK_Z are the same as their ASCII equivalents: 'A' thru 'Z'
;  VK_0 thru VK_9 are the same as their ASCII equivalents: '0' thru '0'

VK_PRINT        = 2aH
VK_EXECUTE      = 2bH
VK_SNAPSHOT	= 2ch	; Printscreen key..
VK_INSERT       = 2dH
VK_DELETE       = 2eH
VK_HELP         = 2fH

VK_NUMPAD0      = 60H
VK_NUMPAD1      = 61H
VK_NUMPAD2      = 62H
VK_NUMPAD3      = 63H
VK_NUMPAD4      = 64H
VK_NUMPAD5      = 65H
VK_NUMPAD6      = 66H
VK_NUMPAD7      = 67H
VK_NUMPAD8      = 68H
VK_NUMPAD9      = 69H
VK_MULTIPLY     = 6AH
VK_ADD          = 6BH
VK_SEPARATER    = 6CH
VK_SUBTRACT     = 6DH
VK_DECIMAL      = 6EH
VK_DIVIDE       = 6FH

VK_F1           = 70H
VK_F2           = 71H
VK_F3           = 72H
VK_F4           = 73H
VK_F5           = 74H
VK_F6           = 75H
VK_F7           = 76H
VK_F8           = 77H
VK_F9           = 78H
VK_F10          = 79H
VK_F11          = 7aH
VK_F12          = 7bH
VK_F13          = 7cH
VK_F14          = 7dH
VK_F15          = 7eH
VK_F16          = 7fH
VK_F17		= 80H
VK_F18		= 81H
VK_F19		= 82H
VK_F20		= 83H
VK_F21		= 84H
VK_F22		= 85H
VK_F23		= 86H
VK_F24		= 87H

VK_NUMLOCK      = 90H
VK_SCROLL       = 91H
ENDIF

IFNDEF NOWH

; SetWindowsHook() codes
WH_MSGFILTER       = (-1)
WH_JOURNALRECORD   = 0
WH_JOURNALPLAYBACK = 1
WH_KEYBOARD        = 2
WH_GETMESSAGE      = 3
WH_CALLWNDPROC     = 4
IFNDEF NOWIN31
WH_CBT             = 5
WH_SYSMSGFILTER    = 6
WH_MOUSE	   = 7
WH_HARDWARE	   = 8
WH_DEBUG	   = 9
ENDIF
;
; Hook Codes
HC_GETLPLPFN	   = (-3)
HC_LPLPFNNEXT      = (-2)
HC_LPFNNEXT        = (-1)
HC_ACTION          = 0
HC_GETNEXT         = 1
HC_SKIP            = 2
HC_NOREM           = 3
HC_NOREMOVE        = 3
HC_SYSMODALON      = 4
HC_SYSMODALOFF     = 5
;
; CBT Hook Codes
HCBT_MOVESIZE      = 0
HCBT_MINMAX        = 1
HCBT_QS            = 2
HCBT_CREATEWND	   = 3
HCBT_DESTROYWND	   = 4
HCBT_ACTIVATE	   = 5
HCBT_CLICKSKIPPED  = 6
HCBT_KEYSKIPPED    = 7
HCBT_SYSCOMMAND	   = 8
HCBT_SETFOCUS	   = 9

;
; WH_MSGFILTER Filter Proc Codes
MSGF_DIALOGBOX     = 0
MSGF_MENU          = 2
MSGF_MOVE          = 3
MSGF_SIZE          = 4
MSGF_SCROLLBAR     = 5
MSGF_NEXTWINDOW    = 6
;
; Window Manager Hook Codes
WC_INIT            = 1
WC_SWP             = 2
WC_DEFWINDOWPROC   = 3
WC_MINMAX          = 4
WC_MOVE            = 5
WC_SIZE            = 6
WC_DRAWCAPTION     = 7
;

; Message Structure used in Journaling
EVENTMSG    struc
    message     dw ?
    paramL      dw ?
    paramH      dw ?
    time        dd ?
EVENTMSG    ends

ENDIF ;NOWH

; Window field offsets for GetWindowLong() and GetWindowWord()
GWL_WNDPROC       =  (-4)
GWW_HINSTANCE     =  (-6)
GWW_HWNDPARENT    =  (-8)
GWW_ID            =  (-12)
GWL_STYLE         =  (-16)
GWL_EXSTYLE       =  (-20)

; GetWindow() Constants
GW_HWNDFIRST	  =  0
GW_HWNDLAST	  =  1
GW_HWNDNEXT	  =  2
GW_HWNDPREV	  =  3
GW_OWNER	  =  4
GW_CHILD	  =  5

; Class field offsets for GetClassLong() and GetClassWord()
GCL_MENUNAME      =  (-8)
GCW_HBRBACKGROUND =  (-10)
GCW_HCURSOR       =  (-12)
GCW_HICON         =  (-14)
GCW_HMODULE       =  (-16)
GCW_CBWNDEXTRA    =  (-18)
GCW_CBCLSEXTRA    =  (-20)
GCL_WNDPROC       =  (-24)
GCW_STYLE         =  (-26)

; WinWhere() Area Codes
HTERROR           =  (-2)
HTTRANSPARENT     =  (-1)
HTNOWHERE         =  0
HTCLIENT          =  1
HTCAPTION         =  2
HTSYSMENU         =  3
HTGROWBOX         =  4
HTSIZE            =  HTGROWBOX
HTMENU            =  5
HTHSCROLL         =  6
HTVSCROLL         =  7
HTREDUCE          =  8
HTZOOM            =  9
HTLEFT            =  10
HTRIGHT           =  11
HTTOP             =  12
HTTOPLEFT         =  13
HTTOPRIGHT        =  14
HTBOTTOM          =  15
HTBOTTOMLEFT      =  16
HTBOTTOMRIGHT     =  17
HTSIZEFIRST       =  HTLEFT
HTSIZELAST        =  HTBOTTOMRIGHT



;*************************************************************************
;
;       Misc structures & constants
;
;*************************************************************************

IFNDEF  NOMST
POINT   struc
        ptX             dw      ?
        ptY             dw      ?
POINT   ends

LOGPEN struc
    lopnStyle       dw ?
    lopnWidth       db (SIZE POINT) DUP(?)
    lopnColor       dd ?
LOGPEN ends


BITMAP STRUC
        bmType         DW ?
        bmWidth        DW ?
        bmHeight       DW ?
        bmWidthBytes   DW ?
        bmPlanes       DB ?
        bmBitsPixel    DB ?
        bmBits         DD ?
BITMAP ENDS

RGBTRIPLE	struc
	rgbBlue		db ?
	rgbGreen	db ?
	rgbRed		db ?
RGBTRIPLE	ends

RGBQUAD         struc
        rgbqBlue        db ?
        rgbqGreen       db ?
        rgbqRed         db ?
        rgbqReserved    db ?
RGBQUAD         ends

; structures for defining DIBs
BITMAPCOREHEADER struc
        bcSize      dd ?
        bcWidth     dw ?
        bcHeight    dw ?
        bcPlanes    dw ?
        bcBitCount  dw ?
BITMAPCOREHEADER ends

BITMAPINFOHEADER struc
        biSize           dd ?
        biWidth          dd ?
        biHeight         dd ?
        biPlanes         dw ?
        biBitCount       dw ?

        biCompression    dd ?
        biSizeImage      dd ?
        biXPelsPerMeter  dd ?
        biYPelsPerMeter  dd ?
        biClrUsed        dd ?
        biClrImportant   dd ?
BITMAPINFOHEADER ends

BITMAPINFO  struc
    bmiHeader   db (SIZE BITMAPINFOHEADER) DUP (?)
    bmiColors   db ?            ; array of RGBQUADs
BITMAPINFO  ends

BITMAPCOREINFO  struc
    bmciHeader  db (SIZE BITMAPCOREHEADER) DUP (?)
    bmciColors  db ?            ; array of RGBTRIPLEs
BITMAPCOREINFO  ends

BITMAPFILEHEADER struc
    bfType          dw ?
    bfSize          dd ?
    bfReserved1     dw ?
    bfReserved2     dw ?
    bfOffBits       dd ?
BITMAPFILEHEADER ends


WNDSTRUC struc
        WSwndStyle        dd      ?
        WSwndID           dw      ?
        WSwndText         dw      ?
        WSwndParent       dw      ?
        WSwndInstance     dw      ?
        WSwndClassProc    dd      ?
WNDSTRUC ends
;
;  Message structure
;
MSGSTRUCT       struc
msHWND          dw      ?
msMESSAGE       dw      ?
msWPARAM        dw      ?
msLPARAM        dd      ?
msTIME          dd      ?
msPT            dd      ?
MSGSTRUCT       ends

NEWPARMS struc
        nprmHwnd        dw      ?
        nprmCmd         db      ?
NEWPARMS ends
ENDIF

PAINTSTRUCT STRUC
    PShdc         DW ?
    PSfErase      DW ?
    PSrcPaint     DB size RECT dup(?)
    PSfRestore    DW ?
    PSfIncUpdate  DW ?
    PSrgbReserved DB 16 dup(?)
PAINTSTRUCT ENDS


CREATESTRUCT struc
    cs_lpCreateParams  dd ?
    cs_hInstance       dw ?
    cs_hMenu           dw ?
    cs_hwndParent      dw ?
    cs_cy              dw ?
    cs_cx              dw ?
    cs_y               dw ?
    cs_x               dw ?
    cs_style           dd ?
    cs_lpszName        dd ?
    cs_lpszClass       dd ?
    cs_dwExStyle       dd ?
CREATESTRUCT  ends
;
;       PostError constants
;
WARNING     = 0           ; command codes
MINOR_ERROR = 1
FATAL_ERROR = 2

IGNORE      = 0           ; response codes
RETRY       = 1
ABORT       = 2
;
; GDI-related constants & commands
;
ERRORREGION     = 0
NULLREGION      = 1
SIMPLEREGION    = 2
COMPLEXREGION   = 3

IFNDEF NORASTOPS
;
; Binary raster ops
;
R2_BLACK        =  1
R2_NOTMERGEPEN  =  2
R2_MASKNOTPEN   =  3
R2_NOTCOPYPEN   =  4
R2_MASKPENNOT   =  5
R2_NOT          =  6
R2_XORPEN       =  7
R2_NOTMASKPEN   =  8
R2_MASKPEN      =  9
R2_NOTXORPEN    = 10
R2_NOP          = 11
R2_MERGENOTPEN  = 12
R2_COPYPEN      = 13
R2_MERGEPENNOT  = 14
R2_MERGEPEN     = 15
R2_WHITE        = 16
;
; Ternary raster ops
;
SRCCOPY_L     = 0020h   ;dest=source
SRCCOPY_H     = 00CCh
SRCPAINT_L    = 0086h   ;dest=source OR dest
SRCPAINT_H    = 00EEh
SRCAND_L      = 00C6h   ;dest=source AND   dest
SRCAND_H      = 0088h
SRCINVERT_L   = 0046h   ;dest= source XOR      dest
SRCINVERT_H   = 0066h
SRCERASE_L    = 0328h   ;dest= source AND (not dest )
SRCERASE_H    = 0044h
NOTSRCCOPY_L  = 0008h   ;dest= (not source)
NOTSRCCOPY_H  = 0033h
NOTSRCERASE_L = 00A6h   ;dest= (not source) AND (not dest)
NOTSRCERASE_H = 0011h
MERGECOPY_L   = 00CAh   ;dest= (source AND pattern)
MERGECOPY_H   = 00C0h
MERGEPAINT_L  = 0226h   ;dest= (source AND pattern) OR dest
MERGEPAINT_H  = 00BBh
PATCOPY_L     = 0021h   ;dest= pattern
PATCOPY_H     = 00F0h
PATPAINT_L    = 0A09h   ;DPSnoo
PATPAINT_H    = 00FBh
PATINVERT_L   = 0049h   ;dest= pattern XOR     dest
PATINVERT_H   = 005Ah
DSTINVERT_L   = 0009h   ;dest= (not dest)
DSTINVERT_H   = 0055h
BLACKNESS_L   = 0042h   ;dest= BLACK
BLACKNESS_H   = 0000h
WHITENESS_L   = 0062h   ;dest= WHITE
WHITENESS_H   = 00FFh
;
; StretchBlt modes
;
BLACKONWHITE    = 1
WHITEONBLACK    = 2
COLORONCOLOR    = 3
;
; New StretchBlt modes
;
STRETCH_ANDSCANS    = 1
STRETCH_ORSCANS     = 2
STRETCH_DELETESCANS = 3
;
; PolyFill modes
;
ALTERNATE       = 1
WINDING         = 2
ENDIF
;
; Text Alignment Options
;
TA_NOUPDATECP   =  0
TA_UPDATECP     =  1

TA_LEFT         =  0
TA_RIGHT        =  2
TA_CENTER       =  6

TA_TOP          =  0
TA_BOTTOM       =  8
TA_BASELINE     =  24

ETO_GRAYED      =  1
ETO_OPAQUE      =  2
ETO_CLIPPED     =  4

ASPECT_FILTERING = 1

ifndef NOMETAFILE

; Metafile Functions */
META_SETBKCOLOR            =  0201h
META_SETBKMODE             =  0102h
META_SETMAPMODE            =  0103h
META_SETROP2               =  0104h
META_SETRELABS             =  0105h
META_SETPOLYFILLMODE       =  0106h
META_SETSTRETCHBLTMODE     =  0107h
META_SETTEXTCHAREXTRA      =  0108h
META_SETTEXTCOLOR          =  0209h
META_SETTEXTJUSTIFICATION  =  020Ah
META_SETWINDOWORG          =  020Bh
META_SETWINDOWEXT          =  020Ch
META_SETVIEWPORTORG        =  020Dh
META_SETVIEWPORTEXT        =  020Eh
META_OFFSETWINDOWORG       =  020Fh
META_SCALEWINDOWEXT        =  0400h
META_OFFSETVIEWPORTORG     =  0211h
META_SCALEVIEWPORTEXT      =  0412h
META_LINETO                =  0213h
META_MOVETO                =  0214h
META_EXCLUDECLIPRECT       =  0415h
META_INTERSECTCLIPRECT     =  0416h
META_ARC                   =  0817h
META_ELLIPSE               =  0418h
META_FLOODFILL             =  0419h
META_PIE                   =  081Ah
META_RECTANGLE             =  041Bh
META_ROUNDRECT             =  061Ch
META_PATBLT                =  061Dh
META_SAVEDC                =  001Eh
META_SETPIXEL              =  041Fh
META_OFFSETCLIPRGN         =  0220h
META_TEXTOUT               =  0521h
META_BITBLT                =  0922h
META_STRETCHBLT            =  0B23h
META_POLYGON               =  0324h
META_POLYLINE              =  0325h
META_ESCAPE                =  0626h
META_RESTOREDC             =  0127h
META_FILLREGION            =  0228h
META_FRAMEREGION           =  0429h
META_INVERTREGION          =  012Ah
META_PAINTREGION           =  012Bh
META_SELECTCLIPREGION      =  012Ch
META_SELECTOBJECT          =  012Dh
META_SETTEXTALIGN          =  012Eh
META_DRAWTEXT              =  062Fh

META_CHORD		   =  0830h
META_SETMAPPERFLAGS	   =  0231h
META_EXTTEXTOUT		   =  0a32h
META_SETDIBTODEV	   =  0d33h
META_SELECTPALETTE	   =  0234h
META_REALIZEPALETTE	   =  0035h
META_ANIMATEPALETTE	   =  0436h
META_SETPALENTRIES	   =  0037h
META_POLYPOLYGON	   =  0538h
META_RESIZEPALETTE	   =  0139h

META_DIBBITBLT		   =  0940h
META_DIBSTRETCHBLT	   =  0b41h
META_DIBCREATEPATTERNBRUSH =  0142h
META_STRETCHDIB		   =  0f43h

META_DELETEOBJECT	   =  01f0h

META_CREATEPALETTE	   =  00f7h
META_CREATEBRUSH           =  00F8h
META_CREATEPATTERNBRUSH    =  01F9h
META_CREATEPENINDIRECT     =  02FAh
META_CREATEFONTINDIRECT    =  02FBh
META_CREATEBRUSHINDIRECT   =  02FCh
META_CREATEBITMAPINDIRECT  =  02FDh
META_CREATEBITMAP          =  06FEh
META_CREATEREGION          =  06FFh

; /* Clipboard Metafile Picture Structure */
HANDLETABLE struc
    ht_objectHandle  dw      ?
HANDLETABLE ends

METARECORD struc
    mr_rdSize	     dd      ?
    mr_rdFunction    dw      ?
    mr_rdParm	     dw      ?
METARECORD ends

METAFILEPICT struc
    mfp_mm	dw	?
    mfp_xExt	dw	?
    mfp_yExt	dw	?
    mfp_hMF	dw	?
METAFILEPICT ends

METAHEADER struc
  mtType	dw	?
  mtHeaderSize	dw	?
  mtVersion	dw	?
  mtSize	dd	?
  mtNoObjects	dw	?
  mtMaxRecord	dd	?
  mtNoParameters dw	?
METAHEADER ends

endif ; NOMETAFILE

; GDI Escapes
NEWFRAME                  =   1
ABORTDOC                  =   2
NEXTBAND                  =   3
SETCOLORTABLE             =   4
GETCOLORTABLE             =   5
FLUSHOUTPUT               =   6
DRAFTMODE                 =   7
QUERYESCSUPPORT           =   8
SETABORTPROC              =   9
STARTDOC                  =   10
;; This value conflicts with a std WIN386 MACRO definition
;;ENDDOC		    =	11
GETPHYSPAGESIZE           =   12
GETPRINTINGOFFSET         =   13
GETSCALINGFACTOR          =   14
MFCOMMENT                 =   15
GETPENWIDTH               =   16
SETCOPYCOUNT              =   17
SELECTPAPERSOURCE         =   18
DEVICEDATA                =   19
PASSTHROUGH               =   19
GETTECHNOLGY		  =   20
GETTECHNOLOGY		  =   20
SETENDCAP                 =   21
SETLINEJOIN               =   22
SETMITERLIMIT             =   23
BANDINFO                  =   24
DRAWPATTERNRECT           =   25
GETVECTORPENSIZE          =   26
GETVECTORBRUSHSIZE        =   27
ENABLEDUPLEX              =   28
ENABLEMANUALFEED	  =   29
GETSETPAPERBINS 	  =   29
GETSETPRINTORIENT	  =   30
ENUMPAPERBINS		  =   31

GETEXTENDEDTEXTMETRICS    =   256
GETEXTENTTABLE            =   257
GETPAIRKERNTABLE          =   258
GETTRACKKERNTABLE         =   259

EXTTEXTOUT                =   512

ENABLERELATIVEWIDTHS      =   768
ENABLEPAIRKERNING         =   769
SETKERNTRACK              =   770
SETALLJUSTVALUES	  =   771
SETCHARSET		  =   772

GETSETSCREENPARAMS        =   3072

STRETCHBLT                =   2048


; Spooler Error Codes
SP_NOTREPORTED            =   4000h
SP_ERROR                  =   (-1)
SP_APPABORT               =   (-2)
SP_USERABORT              =   (-3)
SP_OUTOFDISK              =   (-4)
SP_OUTOFMEMORY            =   (-5)

PR_JOBSTATUS              =   0000

; Object Definitions for EnumObjects()
OBJ_PEN                   =   1
OBJ_BRUSH                 =   2

;
; Menu flags for Change/Check/Enable MenuItem
;
MF_INSERT       =   0000h
MF_CHANGE       =   0080h
MF_APPEND       =   0100h
MF_DELETE       =   0200h
MF_REMOVE       =   1000h

MF_BYCOMMAND    =   0000h
MF_BYPOSITION   =   0400h

MF_SEPARATOR    =   0800h

MF_ENABLED      =   0000h
MF_GRAYED       =   0001h
MF_DISABLED     =   0002h

MF_UNCHECKED    =   0000h
MF_CHECKED      =   0008h
MF_USECHECKBITMAPS= 0200h

MF_STRING       =   0000h
MF_BITMAP       =   0004h
MF_OWNERDRAW    =   0100h

MF_POPUP        =   0010h
MF_MENUBARBREAK =   0020h
MF_MENUBREAK    =   0040h

MF_UNHILITE     =   0000h
MF_HILITE       =   0080h

MF_SYSMENU      =   2000h
MF_HELP         =   4000h
MF_MOUSESELECT  =   8000h


;
;  System Menu Command Values
;
SC_SIZE        = 0F000h
SC_MOVE        = 0F010h
SC_MINIMIZE    = 0F020h
SC_MAXIMIZE    = 0F030h
SC_NEXTWINDOW  = 0F040h
SC_PREVWINDOW  = 0F050h
SC_CLOSE       = 0F060h
SC_VSCROLL     = 0F070h
SC_HSCROLL     = 0F080h
SC_MOUSEMENU   = 0F090h
SC_KEYMENU     = 0F100h
SC_ARRANGE     = 0F110h
SC_RESTORE     = 0F120h
SC_TASKLIST    = 0F130h
SC_SCREENSAVE  = 0F140h
SC_HOTKEY      = 0F150h

SC_ICON        = SC_MINIMIZE
SC_ZOOM        = SC_MAXIMIZE

;
;  Window State Messages
;
IFNDEF  NOWM
WM_STATE            = 0000H

WM_NULL             = 0000h
WM_CREATE           = 0001h
WM_DESTROY          = 0002h
WM_MOVE             = 0003h
WM_SIZE             = 0005h
WM_ACTIVATE         = 0006h
WM_SETFOCUS         = 0007h
WM_KILLFOCUS        = 0008h
WM_ENABLE           = 000Ah
WM_SETREDRAW        = 000Bh
WM_SETTEXT          = 000Ch
WM_GETTEXT          = 000Dh
WM_GETTEXTLENGTH    = 000Eh
WM_PAINT            = 000Fh

WM_CLOSE            = 0010h
WM_QUERYENDSESSION  = 0011h
WM_QUIT             = 0012h
WM_QUERYOPEN        = 0013h
WM_ERASEBKGND       = 0014h
WM_SYSCOLORCHANGE   = 0015h
WM_ENDSESSION       = 0016h
WM_SHOWWINDOW       = 0018h
WM_CTLCOLOR         = 0019h
WM_WININICHANGE     = 001Ah
WM_DEVMODECHANGE    = 001Bh
WM_ACTIVATEAPP      = 001Ch
WM_FONTCHANGE       = 001Dh
WM_TIMECHANGE       = 001Eh
WM_CANCELMODE       = 001Fh

WM_SETCURSOR        = 0020h
WM_MOUSEACTIVATE    = 0021h
WM_CHILDACTIVATE    = 0022h
WM_QUEUESYNC        = 0023h
WM_GETMINMAXINFO    = 0024h
WM_HELP             = 0025h
WM_ICONERASEBKGND   = 0027h
WM_NEXTDLGCTL       = 0028h
WM_SPOOLERSTATUS    = 002Ah
WM_DRAWITEM         = 002Bh
WM_MEASUREITEM      = 002Ch
WM_DELETEITEM       = 002Dh
WM_VKEYTOITEM       = 002Eh
WM_CHARTOITEM       = 002Fh

WM_SETFONT          = 0030h
WM_GETFONT          = 0031h
WM_QUERYDRAGICON    = 0037h
;WM_WINHELP         = 0038h ;Win32
WM_COMPAREITEM	    = 0039h
;WM_FULLSCREEN      = 003Ah ;Win32
;WM_CLIENTSHUTDOWN  = 003Bh ;Win32
;WM_DDEMLEVENT      = 003Ch ;Win32
;MM_CALCSCROLL      = 003Fh ;Win32

WM_COMPACTING       = 0041h
;WM_OTHERWINDOWCREATED = 0042h ;Win32
;WM_OTHERWINDOWDESTROYED = 0043h ;Win32
WM_COMMNOTIFY       = 0044h
WM_WINDOWPOSCHANGING= 0046h
WM_WINDOWPOSCHANGED = 0047h
WM_POWER            = 0048h
;WM_COPYGLOBALDATA  = 0049h ;Win32
;WM_COPYDATA        = 004Ah ;Win32
;WM_CANCELJOURNAL   = 004Bh ;Win32
;WM_LOGONNOTIFY     = 004Ch ;Win32
WM_NOTIFY           = 004Eh
;WM_ACCESS_WINDOW   = 004Fh ;Win32

WM_TCARD	    = 0052h
WM_HELP 	    = 0053h


;WM_FINALDESTROY    = 0070h

WM_NCCREATE         = 0081h
WM_NCDESTROY        = 0082h
WM_NCCALCSIZE       = 0083h
WM_NCHITTEST        = 0084h
WM_NCPAINT          = 0085h
WM_NCACTIVATE       = 0086h
WM_GETDLGCODE       = 0087h

WM_NCMOUSEMOVE      = 00A0h
WM_NCLBUTTONDOWN    = 00A1h
WM_NCLBUTTONUP      = 00A2h
WM_NCLBUTTONDBLCLK  = 00A3h
WM_NCRBUTTONDOWN    = 00A4h
WM_NCRBUTTONUP      = 00A5h
WM_NCRBUTTONDBLCLK  = 00A6h
WM_NCMBUTTONDOWN    = 00A7h
WM_NCMBUTTONUP      = 00A8h
WM_NCMBUTTONDBLCLK  = 00A9h

WM_KEYFIRST         = 0100h
WM_KEYDOWN          = 0100h
WM_KEYUP            = 0101h
WM_CHAR             = 0102h
WM_DEADCHAR         = 0103h
WM_SYSKEYDOWN       = 0104h
WM_SYSKEYUP         = 0105h
WM_SYSCHAR          = 0106h
WM_SYSDEADCHAR      = 0107h
WM_KEYLAST          = 0108h
WM_INTERIM          = 010Ch

WM_INITDIALOG       = 0110h
WM_COMMAND          = 0111h
WM_SYSCOMMAND       = 0112h
WM_TIMER            = 0113h
WM_HSCROLL          = 0114h
WM_VSCROLL          = 0115h
WM_INITMENU         = 0116h
WM_INITMENUPOPUP    = 0117h
WM_MENUSELECT       = 011Fh

WM_MENUCHAR         = 0120h
WM_ENTERIDLE        = 0121h

;WM_CTLCOLORFIRST       = 0132h ;Win32
;WM_CTLCOLORMSGBOX      = 0132h ;Win32
;WM_CTLCOLOREDIT        = 0133h ;Win32
;WM_CTLCOLORLISTBOX     = 0134h ;Win32
;WM_CTLCOLORBTN         = 0135h ;Win32
;WM_CTLCOLORDLG         = 0136h ;Win32
;WM_CTLCOLORSCROLLBAR   = 0137h ;Win32
;WM_CTLCOLORSTATIC      = 0138h ;Win32
;WM_CTLCOLORLAST        = 0138h ;Win32

;CB_GETEDITSEL          = 0140h ;Win32
;CB_LIMITTEXT           = 0141h ;Win32
;CB_SETEDITSEL          = 0142h ;Win32
;CB_ADDSTRING           = 0143h ;Win32
;CB_DELETESTRING        = 0144h ;Win32
;CB_DIR                 = 0145h ;Win32
;CB_GETCOUNT            = 0146h ;Win32
;CB_GETCURSEL           = 0147h ;Win32
;CB_GETLBTEXT           = 0148h ;Win32
;CB_GETLBTEXTLEN        = 0149h ;Win32
;CB_INSERTSTRING        = 014Ah ;Win32
;CB_RESETCONTENT        = 014Bh ;Win32
;CB_FINDSTRING          = 014Ch ;Win32
;CB_SELECTSTRING        = 014Dh ;Win32
;CB_SETCURSEL           = 014Eh ;Win32
;CB_SHOWDROPDOWN        = 014Fh ;Win32

;CB_GETITEMDATA         = 0150h ;Win32
;CB_SETITEMDATA         = 0151h ;Win32
;CB_GETDROPPEDCONTROLRECT   = 0152h ;Win32
;CB_SETITEMHEIGHT       = 0153h ;Win32
;CB_GETITEMHEIGHT       = 0154h ;Win32
;CB_SETEXTENDEDUI       = 0155h ;Win32
;CB_GETEXTENDEDUI       = 0156h ;Win32
;CB_GETDROPPEDSTATE     = 0157h ;Win32
;CB_FINDSTRINGEXACT     = 0158h ;Win32
;CB_SETLOCALE           = 0159h ;Win32
;CB_GETLOCALE           = 015ah ;Win32

;STM_SETICON            = 0170h ;Win32
;STM_GETICON            = 0171h ;Win32

;LB_ADDSTRING           = 0180h ;Win32
;LB_INSERTSTRING        = 0181h ;Win32
;LB_DELETESTRING        = 0182h ;Win32
;LB_SELITEMRANGEEX      = 0183h ;Win32
;LB_RESETCONTENT        = 0184h ;Win32
;LB_SETSEL              = 0185h ;Win32
;LB_SETCURSEL           = 0186h ;Win32
;LB_GETSEL              = 0187h ;Win32
;LB_GETCURSEL           = 0188h ;Win32
;LB_GETTEXT             = 0189h ;Win32
;LB_GETTEXTLEN          = 018Ah ;Win32
;LB_GETCOUNT            = 018Bh ;Win32
;LB_SELECTSTRING        = 018Ch ;Win32
;LB_DIR                 = 018Dh ;Win32
;LB_GETTOPINDEX         = 018Eh ;Win32
;LB_FINDSTRING          = 018Fh ;Win32

;LB_GETSELCOUNT         = 0190h ;Win32
;LB_GETSELITEMS         = 0191h ;Win32
;LB_SETTABSTOPS         = 0192h ;Win32
;LB_GETHORIZONTALEXTENT = 0193h ;Win32
;LB_SETHORIZONTALEXTENT = 0194h ;Win32
;LB_SETCOLUMNWIDTH      = 0195h ;Win32
;LB_ADDFILE             = 0196h ;Win32
;LB_SETTOPINDEX         = 0197h ;Win32
;LB_GETITEMRECT         = 0198h ;Win32
;LB_GETITEMDATA         = 0199h ;Win32
;LB_SETITEMDATA         = 019Ah ;Win32
;LB_SELITEMRANGE        = 019Bh ;Win32
;LB_SETANCHORINDEX      = 019Ch ;Win32
;LB_GETANCHORINDEX      = 019Dh ;Win32
;LB_SETCARETINDEX       = 019Eh ;Win32
;LB_GETCARETINDEX       = 019Fh ;Win32

;LB_SETITEMHEIGHT       = 01A0h ;Win32
;LB_GETITEMHEIGHT       = 01A1h ;Win32
;LB_FINDSTRINGEXACT     = 01A2h ;Win32
;LBCB_CARETON           = 01A3h ;Win32
;LBCB_CARETOFF          = 01A4h ;Win32
;LB_SETLOCALE           = 01A5h ;Win32
;LB_GETLOCALE           = 01A6h ;Win32
;LB_SETCOUNT            = 01A7h ;Win32

;MN_SETHMENU            = 01E0h ;Win32
;MN_GETHMENU            = 01E1h ;Win32
;MN_SIZEWINDOW          = 01E2h ;Win32
;MN_OPENHIERARCHY       = 01E3h ;Win32
;MN_CLOSEHIERARCHY      = 01E4h ;Win32
;MN_SELECTITEM          = 01E5h ;Win32
;MN_CANCELMENUS         = 01E6h ;Win32
;MN_SELECTFIRSTVALIDITEM    = 01E7h ;Win32
;MN_GETPPOPUPMENU       = 01EAh ;Win32
;MN_FINDMENUWINDOWFROMPOINT = 01EBh ;Win32
;MN_SHOWPOPUPWINDOW     = 01ECh ;Win32
;MN_BUTTONDOWN          = 01EDh ;Win32
;MN_MOUSEMOVE           = 01EEh ;Win32
;MN_BUTTONUP            = 01EFh ;Win32
;MN_SETTIMERTOOPENHIERARCHY = 01F0 ;Win32

WM_MOUSEFIRST       = 0200h
WM_MOUSEMOVE        = 0200h
WM_LBUTTONDOWN      = 0201h
WM_LBUTTONUP        = 0202h
WM_LBUTTONDBLCLK    = 0203h
WM_RBUTTONDOWN      = 0204h
WM_RBUTTONUP        = 0205h
WM_RBUTTONDBLCLK    = 0206h
WM_MBUTTONDOWN      = 0207h
WM_MBUTTONUP        = 0208h
WM_MBUTTONDBLCLK    = 0209h
WM_MOUSELAST        = 0209h

WM_PARENTNOTIFY     = 0210h

WM_MDICREATE        = 0220h
WM_MDIDESTROY       = 0221h
WM_MDIACTIVATE      = 0222h
WM_MDIRESTORE       = 0223h
WM_MDINEXT          = 0224h
WM_MDIMAXIMIZE      = 0225h
WM_MDITILE          = 0226h
WM_MDICASCADE       = 0227h
WM_MDIICONARRANGE   = 0228h
WM_MDIGETACTIVE     = 0229h

WM_MDISETMENU       = 0230h
WM_DROPFILES	    = 0233h
;WM_MDIREFRESHMENU  = 0234h ;Win32

WM_HANGEULLAST      = 029Fh ;Win32

WM_CUT              = 0300h
WM_COPY             = 0301h
WM_PASTE            = 0302h
WM_CLEAR            = 0303h
WM_UNDO             = 0304h
WM_RENDERFORMAT     = 0305h
WM_RENDERALLFORMATS = 0306h
WM_DESTROYCLIPBOARD = 0307h
WM_DRAWCLIPBOARD    = 0308h
WM_PAINTCLIPBOARD   = 0309h
WM_VSCROLLCLIPBOARD = 030Ah
WM_SIZECLIPBOARD    = 030Bh
WM_ASKCBFORMATNAME  = 030Ch
WM_CHANGECBCHAIN    = 030Dh
WM_HSCROLLCLIPBOARD = 030Eh
WM_QUERYNEWPALETTE  = 030Fh
WM_PALETTEISCHANGING = 0310h
WM_PALETTECHANGED   = 0311h
;WM_HOTKEY          = 0312h     ;Win32
;WM_HOOKMSG         = 0314h     ;Win32
;WM_EXITPROCESS     = 0315h     ;Win32
WM_PRINT            = 0317h
WM_PRINTCLIENT      = 0318h

WM_PENWINFIRST      equ 0380h
WM_PENWINLAST       equ 038Fh

WM_COALESCE_FIRST  equ 0390h
WM_COALESCE_LAST   equ 039Fh

;  private window messages start here
WM_USER             = 0400H

;  private app messages start here
WM_APP              = 8000H
ENDIF           ; NOWM

; WM_MOUSEACTIVATE Return Codes
MA_ACTIVATE       =  1
MA_ACTIVATEANDEAT =  2
MA_NOACTIVATE     =  3

; Size message commands
SIZENORMAL       = 0
SIZEICONIC       = 1
SIZEFULLSCREEN   = 2
SIZEZOOMSHOW     = 3
SIZEZOOMHIDE     = 4

; ShowWindow() Commands
SW_HIDE            = 0
SW_SHOWNORMAL      = 1
SW_NORMAL          = 1
SW_SHOWMINIMIZED   = 2
SW_SHOWMAXIMIZED   = 3
SW_MAXIMIZE        = 3
SW_SHOWNOACTIVATE  = 4
SW_SHOW            = 5
SW_MINIMIZE        = 6
SW_SHOWMINNOACTIVE = 7
SW_SHOWNA          = 8
SW_RESTORE         = 9

; Old ShowWindow() Commands
HIDE_WINDOW        = 0
SHOW_OPENWINDOW    = 1
SHOW_ICONWINDOW    = 2
SHOW_FULLSCREEN    = 3
SHOW_OPENNOACTIVATE= 4

;  identifiers for the WM_SHOWWINDOW message
SW_PARENTCLOSING =  1
SW_OTHERZOOM     =  2
SW_PARENTOPENING =  3
SW_OTHERUNZOOM   =  4
;
; Key state masks for mouse messages
;
MK_LBUTTON       = 0001h
MK_RBUTTON       = 0002h
MK_SHIFT         = 0004h
MK_CONTROL       = 0008h
MK_MBUTTON       = 0010h
;
; Class styles
;
CS_VREDRAW         = 0001h
CS_HREDRAW         = 0002h
CS_KEYCVTWINDOW    = 0004H
CS_DBLCLKS         = 0008h
; 		     0010h reserved
CS_OWNDC           = 0020h
CS_CLASSDC         = 0040h
CS_PARENTDC        = 0080h
CS_NOKEYCVT        = 0100h
CS_SAVEBITS        = 0800h
CS_NOCLOSE         = 0200h
CS_BYTEALIGNCLIENT = 1000h
CS_BYTEALIGNWINDOW = 2000h
CS_GLOBALCLASS     = 4000h    ; Global window class

;
; Special CreateWindow position value
;
CW_USEDEFAULT	EQU    8000h

;
; Windows styles (the high words)
;
WS_OVERLAPPED   = 00000h
WS_ICONICPOPUP  = 0C000h
WS_POPUP        = 08000h
WS_CHILD        = 04000h
WS_MINIMIZE     = 02000h
WS_VISIBLE      = 01000h
WS_DISABLED     = 00800h
WS_CLIPSIBLINGS = 00400h
WS_CLIPCHILDREN = 00200h
WS_MAXIMIZE     = 00100h
WS_CAPTION      = 000C0h     ; WS_BORDER | WS_DLGFRAME
WS_BORDER       = 00080h
WS_DLGFRAME     = 00040h
WS_VSCROLL      = 00020h
WS_HSCROLL      = 00010h
WS_SYSMENU      = 00008h
WS_THICKFRAME   = 00004h
WS_HREDRAW      = 00002h
WS_VREDRAW      = 00001h
WS_GROUP        = 00002h
WS_TABSTOP      = 00001h
WS_MINIMIZEBOX  = 00002h
WS_MAXIMIZEBOX  = 00001h

; Common Window Styles

WS_OVERLAPPEDWINDOW = WS_OVERLAPPED + WS_CAPTION + WS_SYSMENU + WS_THICKFRAME + WS_MINIMIZEBOX + WS_MAXIMIZEBOX
WS_POPUPWINDOW  = WS_POPUP + WS_BORDER + WS_SYSMENU
WS_CHILDWINDOW  = WS_CHILD
WS_TILEDWINDOW  = WS_OVERLAPPEDWINDOW

WS_TILED        = WS_OVERLAPPED
WS_ICONIC       = WS_MINIMIZE
WS_SIZEBOX      = WS_THICKFRAME

; Extended Window Styles (low words)
WS_EX_DLGMODALFRAME  = 0001
WS_EX_DRAGOBJECT     = 0002
WS_EX_NOPARENTNOTIFY = 0004
WS_EX_TOPMOST        = 0008

;
; predefined clipboard formats
;
CF_TEXT         =  1
CF_BITMAP       =  2
CF_METAFILEPICT =  3
CF_SYLK         =  4
CF_DIF          =  5
CF_TIFF         =  6
CF_OEMTEXT      =  7
CF_DIB          =  8
CF_PALETTE      =  9
CF_PENDATA      = 10
CF_RIFF         = 11
CF_WAVE         = 12

CF_OWNERDISPLAY = 80h       ; owner display
CF_DSPTEXT      = 81h       ; display text
CF_DSPBITMAP    = 82h       ; display bitmap
CF_DSPMETAFILEPICT  = 83h   ; display metafile
;
; Private clipboard format range
;
CF_PRIVATEFIRST       = 200h       ; Anything in this range doesn't
CF_PRIVATELAST        = 2ffh       ; get GlobalFree'd
CF_GDIOBJFIRST        = 300h       ; Anything in this range gets
CF_GDIOBJLAST         = 3ffh       ; DeleteObject'ed


MAKEINTRESOURCE MACRO a
        mov     ax,a
        xor     dx,dx
        ENDM
;
;  Predefined resource types
;
RT_CURSOR       = 1              ; must be passed through MAKEINTRESOURCE
RT_BITMAP       = 2
RT_ICON         = 3
RT_MENU         = 4
RT_DIALOG       = 5
RT_STRING       = 6
RT_FONTDIR      = 7
RT_FONT         = 8
RT_ACCELERATOR  = 9
RT_RCDATA       = 10

;** NOTE: if any new resource types are introduced above this point, then the
;** value of DIFFERENCE must be changed.
;** (RT_GROUP_CURSOR - RT_CURSOR) must always be equal to DIFFERENCE
;** (RT_GROUP_ICON - RT_ICON) must always be equal to DIFFERENCE

DIFFERENCE       =   11

RT_GROUP_CURSOR  =   RT_CURSOR + DIFFERENCE
RT_GROUP_ICON    =   RT_ICON + DIFFERENCE



IFNDEF NOMDI
MDICREATESTRUCT     struc
    szClass	    dd ?
    szTitle	    dd ?
    hOwner	    dw ?
    x		    dw ?
    y		    dw ?
    cxc 	    dw ?
    cyc 	    dw ?
    style	    dd ?
MDICREATESTRUCT ends

CLIENTCREATESTRUCT  struc
    hWindowMenu     dw ?
    idFirstChild    dw ?
CLIENTCREATESTRUCT ends
ENDIF

; NOMDI


PALETTEENTRY	    struc
    peRed	    db ?
    peGreen	    db ?
    peBlue	    db ?
    peFlags	    db ?
PALETTEENTRY	    ends

; Logical Palette
LOGPALETTE	    struc
    palVersion	    dw ?
    palNumEntries   dw ?
    palPalEntry     db ?  ; array of PALETTEENTRY
LOGPALETTE	    ends

; DRAWITEMSTRUCT for ownerdraw
DRAWITEMSTRUCT	    struc
    drCtlType	      dw ?
    drCtlID	      dw ?
    dritemID	      dw ?
    dritemAction      dw ?
    dritemState       dw ?
    drhwndItem	      dw ?
    drhDC	      dw ?
    drrcItem	      DB size RECT dup(?)
    dritemData	      dd ?
DRAWITEMSTRUCT ends

; DELETEITEMSTRUCT for ownerdraw
DELETEITEMSTRUCT    struc
    deCtlType	      dw ?
    deCtlID	      dw ?
    deitemID	      dw ?
    dehwndItem	      dw ?
    deitemData	      dd ?
DELETEITEMSTRUCT ends

; MEASUREITEMSTRUCT for ownerdraw
MEASUREITEMSTRUCT   struc
    meCtlType	      dw ?
    meCtlID	      dw ?
    meitemID	      dw ?
    meitemWidth       dw ?
    meitemHeight      dw ?
    meitemData	      dd ?
MEASUREITEMSTRUCT ends

; COMPAREITEMSTUCT for ownerdraw sorting
COMPAREITEMSTRUCT   struc
    coCtlType   dw ?
    coCtlID     dw ?
    cohwndItem  dw ?
    coitemID1   dw ?
    coitemData1 dd ?
    coitemID2   dw ?
    coitemData2 dd ?
COMPAREITEMSTRUCT   ends

; Owner draw control types
ODT_MENU      =  1
ODT_LISTBOX   =  2
ODT_COMBOBOX  =  3
ODT_BUTTON    =  4

; Owner draw actions
ODA_DRAWENTIRE = 1
ODA_SELECT     = 2
ODA_FOCUS      = 4

; Owner draw state
ODS_SELECTED   = 0001h
ODS_GRAYED     = 0002h
ODS_DISABLED   = 0004h
ODS_CHECKED    = 0008h
ODS_FOCUS      = 0010h

; PeekMessage() Options
PM_NOREMOVE    = 0000h
PM_REMOVE      = 0001h
PM_NOYIELD     = 0002h

; SetWindowPos Flags
SWP_NOSIZE       =  0001h
SWP_NOMOVE       =  0002h
SWP_NOZORDER     =  0004h
SWP_NOREDRAW     =  0008h
SWP_NOACTIVATE   =  0010h
SWP_DRAWFRAME    =  0020h
SWP_SHOWWINDOW   =  0040h
SWP_HIDEWINDOW   =  0080h
SWP_NOCOPYBITS   =  0100h
SWP_NOREPOSITION =  0200h


IFNDEF NOWINMESSAGES

; Listbox messages
LB_ADDSTRING	       = (WM_USER+1)
LB_INSERTSTRING        = (WM_USER+2)
LB_DELETESTRING        = (WM_USER+3)
LB_RESETCONTENT        = (WM_USER+5)
LB_SETSEL	       = (WM_USER+6)
LB_SETCURSEL	       = (WM_USER+7)
LB_GETSEL	       = (WM_USER+8)
LB_GETCURSEL	       = (WM_USER+9)
LB_GETTEXT	       = (WM_USER+10)
LB_GETTEXTLEN	       = (WM_USER+11)
LB_GETCOUNT	       = (WM_USER+12)
LB_SELECTSTRING        = (WM_USER+13)
LB_DIR		       = (WM_USER+14)
LB_GETTOPINDEX	       = (WM_USER+15)
LB_FINDSTRING	       = (WM_USER+16)
LB_GETSELCOUNT	       = (WM_USER+17)
LB_GETSELITEMS	       = (WM_USER+18)
LB_SETTABSTOPS	       = (WM_USER+19)
LB_GETHORIZONTALEXTENT = (WM_USER+20)
LB_SETHORIZONTALEXTENT = (WM_USER+21)
LB_SETTOPINDEX	       = (WM_USER+24)
LB_GETITEMRECT	       = (WM_USER+25)
LB_GETITEMDATA	       = (WM_USER+26)
LB_SETITEMDATA	       = (WM_USER+27)
LB_SELITEMRANGE        = (WM_USER+28)
LB_SETCARETINDEX       = (WM_USER+31)
LB_GETCARETINDEX       = (WM_USER+32)
IFNDEF NOWIN31
LB_SETITEMHEIGHT       = (WM_USER+33)
LB_GETITEMHEIGHT       = (WM_USER+34)
LB_FINDSTRINGEXACT     = (WM_USER+35)
ENDIF

LB_SETLOCALE	       = (WM_USER+38)   ; Win NT
LB_GETLOCALE	       = (WM_USER+39)   ; Win NT
;; LB_SETCOUNT	       = (WM_USER+40)   ; Win NT
LB_INITSTORAGE	       = (WM_USER+41)   

ENDIF
; NOWINMESSAGES

; Listbox Styles
LBS_NOTIFY            = 0001h
LBS_SORT              = 0002h
LBS_NOREDRAW          = 0004h
LBS_MULTIPLESEL       = 0008h
LBS_OWNERDRAWFIXED    = 0010h
LBS_OWNERDRAWVARIABLE = 0020h
LBS_HASSTRINGS        = 0040h
LBS_USETABSTOPS       = 0080h
LBS_NOINTEGRALHEIGHT  = 0100h
LBS_MULTICOLUMN       = 0200h
LBS_WANTKEYBOARDINPUT = 0400h
LBS_EXTENDEDSEL	      = 0800h
LBS_STANDARD          = LBS_NOTIFY + LBS_SORT + WS_VSCROLL + WS_BORDER
LBS_DISABLENOSCROLL   = 1000h

; Listbox Notification Codes
LBN_ERRSPACE      =  (-2)
LBN_SELCHANGE     =  1
LBN_DBLCLK        =  2
LBN_SELCANCEL     =  3
LBN_SETFOCUS      =  4
LBN_KILLFOCUS     =  5

IFNDEF NOWINMESSAGES

; Edit Control Messages
EM_GETSEL              = (WM_USER+0)
EM_SETSEL              = (WM_USER+1)
EM_GETRECT             = (WM_USER+2)
EM_SETRECT             = (WM_USER+3)
EM_SETRECTNP           = (WM_USER+4)
EM_SCROLL              = (WM_USER+5)
EM_LINESCROLL          = (WM_USER+6)
EM_GETMODIFY           = (WM_USER+8)
EM_SETMODIFY           = (WM_USER+9)
EM_GETLINECOUNT        = (WM_USER+10)
EM_LINEINDEX           = (WM_USER+11)
EM_SETHANDLE           = (WM_USER+12)
EM_GETHANDLE           = (WM_USER+13)
EM_LINELENGTH          = (WM_USER+17)
EM_REPLACESEL          = (WM_USER+18)
EM_SETFONT             = (WM_USER+19)
EM_GETLINE             = (WM_USER+20)
EM_LIMITTEXT           = (WM_USER+21)
EM_CANUNDO             = (WM_USER+22)
EM_UNDO                = (WM_USER+23)
EM_FMTLINES            = (WM_USER+24)
EM_LINEFROMCHAR        = (WM_USER+25)
EM_SETWORDBREAK        = (WM_USER+26)
EM_SETTABSTOPS         = (WM_USER+27)
EM_SETPASSWORDCHAR     = (WM_USER+28)
EM_EMPTYUNDOBUFFER     = (WM_USER+29)
IFNDEF NOWIN31
EM_GETFIRSTVISIBLELINE = (WM_USER+30)
EM_SETREADONLY         = (WM_USER+31)
EM_SETWORDBREAKPROC    = (WM_USER+32)
EM_GETWORDBREAKPROC    = (WM_USER+33)
EM_GETPASSWORDCHAR     = (WM_USER+34)
ENDIF

ENDIF
; NOWINMESSAGES


; Edit Control Styles (low word)
ES_LEFT            = 0000h
ES_CENTER          = 0001h
ES_RIGHT           = 0002h
ES_MULTILINE       = 0004h
ES_UPPERCASE       = 0008h
ES_LOWERCASE       = 0010h
ES_PASSWORD        = 0020h
ES_AUTOVSCROLL     = 0040h
ES_AUTOHSCROLL     = 0080h
ES_NOHIDESEL       = 0100h
ES_OEMCONVERT      = 0400h
IFNDEF NOWIN31
ES_READONLY        = 0800h
ES_WANTRETURN      = 1000h
ENDIF


; Edit Control Notification Codes
EN_SETFOCUS        = 0100h
EN_KILLFOCUS       = 0200h
EN_CHANGE          = 0300h
EN_UPDATE          = 0400h
EN_ERRSPACE        = 0500h
EN_MAXTEXT         = 0501h
EN_HSCROLL         = 0601h
EN_VSCROLL         = 0602h

IFNDEF NOWINMESSAGES

; Button Control Messages
BM_GETCHECK	   = (WM_USER+0)
BM_SETCHECK	   = (WM_USER+1)
BM_GETSTATE	   = (WM_USER+2)
BM_SETSTATE	   = (WM_USER+3)
BM_SETSTYLE	   = (WM_USER+4)

ENDIF
; NOWINMESSAGES

; Button Control Styles (low word)
BS_PUSHBUTTON      = 00h
BS_DEFPUSHBUTTON   = 01h
BS_CHECKBOX        = 02h
BS_AUTOCHECKBOX    = 03h
BS_RADIOBUTTON     = 04h
BS_3STATE          = 05h
BS_AUTO3STATE      = 06h
BS_GROUPBOX        = 07h
BS_USERBUTTON      = 08h
BS_AUTORADIOBUTTON = 09h
BS_OWNERDRAW       = 0Bh
BS_LEFTTEXT        = 20h

; User Button Notification Codes
BN_CLICKED         = 0
BN_PAINT           = 1
BN_HILITE          = 2
BN_UNHILITE        = 3
BN_DISABLE         = 4
BN_DOUBLECLICKED   = 5

; Dialog Styles (low words)
DS_ABSALIGN        = 01h
DS_SYSMODAL        = 02h
DS_LOCALEDIT       = 20h  ;/* Edit items get Local storage. */
DS_SETFONT         = 40h  ;/* User specified font for Dlg controls */
DS_MODALFRAME      = 80h  ;/* Can be combined with WS_CAPTION  */
DS_NOIDLEMSG       = 100h ;/* WM_ENTERIDLE message will not be sent */

IFNDEF NOWINMESSAGES

; Dialog box messages
DM_GETDEFID	   = (WM_USER+0)
DM_SETDEFID	   = (WM_USER+1)

ENDIF   ;NOWINMESSAGES

; Dialog Codes
DLGC_WANTARROWS     = 0001h    ;  /* Control wants arrow keys         */
DLGC_WANTTAB        = 0002h    ;  /* Control wants tab keys           */
DLGC_WANTALLKEYS    = 0004h    ;  /* Control wants all keys           */
DLGC_WANTMESSAGE    = 0004h    ;  /* Pass message to control          */
DLGC_HASSETSEL      = 0008h    ;  /* Understands EM_SETSEL message    */
DLGC_DEFPUSHBUTTON  = 0010h    ;  /* Default pushbutton               */
DLGC_UNDEFPUSHBUTTON= 0020h    ;  /* Non-default pushbutton           */
DLGC_RADIOBUTTON    = 0040h    ;  /* Radio button                     */
DLGC_WANTCHARS      = 0080h    ;  /* Want WM_CHAR messages            */
DLGC_STATIC         = 0100h    ;  /* Static item: don't include       */
DLGC_BUTTON         = 2000h    ;  /* Button item: can be checked      */

; Combo Box return Values
CB_OKAY          =   0
CB_ERR           =   (-1)
CB_ERRSPACE      =   (-2)

; Combo Box Notification Codes
CBN_ERRSPACE     =   (-1)
CBN_SELCHANGE    =   1
CBN_DBLCLK       =   2
CBN_SETFOCUS     =   3
CBN_KILLFOCUS    =   4
CBN_EDITCHANGE   =   5
CBN_EDITUPDATE   =   6
CBN_DROPDOWN     =   7

; Combo Box styles (low words)
CBS_SIMPLE           = 0001h
CBS_DROPDOWN         = 0002h
CBS_DROPDOWNLIST     = 0003h
CBS_OWNERDRAWFIXED   = 0010h
CBS_OWNERDRAWVARIABLE= 0020h
CBS_AUTOHSCROLL      = 0040h
CBS_OEMCONVERT       = 0080h
CBS_SORT             = 0100h
CBS_HASSTRINGS       = 0200h
CBS_NOINTEGRALHEIGHT = 0400h

IFNDEF NOWINMESSAGES

; Combo Box messages
CB_GETEDITSEL	         = (WM_USER+0)
CB_LIMITTEXT	         = (WM_USER+1)
CB_SETEDITSEL	         = (WM_USER+2)
CB_ADDSTRING	         = (WM_USER+3)
CB_DELETESTRING          = (WM_USER+4)
CB_DIR		         = (WM_USER+5)
CB_GETCOUNT	         = (WM_USER+6)
CB_GETCURSEL	         = (WM_USER+7)
CB_GETLBTEXT	         = (WM_USER+8)
CB_GETLBTEXTLEN          = (WM_USER+9)
CB_INSERTSTRING          = (WM_USER+10)
CB_RESETCONTENT          = (WM_USER+11)
CB_FINDSTRING	         = (WM_USER+12)
CB_SELECTSTRING          = (WM_USER+13)
CB_SETCURSEL	         = (WM_USER+14)
CB_SHOWDROPDOWN          = (WM_USER+15)
CB_GETITEMDATA           = (WM_USER+16)
CB_SETITEMDATA           = (WM_USER+17)
IFNDEF NOWIN31
CB_GETDROPPEDCONTROLRECT = (WM_USER+18)
CB_SETITEMHEIGHT         = (WM_USER+19)
CB_GETITEMHEIGHT         = (WM_USER+20)
CB_SETEXTENDEDUI         = (WM_USER+21)
CB_GETEXTENDEDUI         = (WM_USER+22)
CB_GETDROPPEDSTATE       = (WM_USER+23)
CB_FINDSTRINGEXACT       = (WM_USER+24)
ENDIF

ENDIF ; NOWINMESSAGES

; Static Control styles (low word)
SS_LEFT            = 00h
SS_CENTER          = 01h
SS_RIGHT           = 02h
SS_ICON            = 03h
SS_BLACKRECT       = 04h
SS_GRAYRECT        = 05h
SS_WHITERECT       = 06h
SS_BLACKFRAME      = 07h
SS_GRAYFRAME       = 08h
SS_WHITEFRAME      = 09h
SS_SIMPLE          = 0Bh
SS_LEFTNOWORDWRAP  = 0Ch
SS_NOPREFIX        = 80h    ; Don't do "&" character translation

IFNDEF NOWIN31
IFNDEF NOWINMESSAGES

;Static Control Messages
STM_SETICON        = (WM_USER+0)
STM_GETICON        = (WM_USER+1)
ENDIF
ENDIF

; Scroll Bar Styles (low word)
SBS_HORZ                    = 0000h
SBS_VERT                    = 0001h
SBS_TOPALIGN                = 0002h
SBS_LEFTALIGN               = 0002h
SBS_BOTTOMALIGN             = 0004h
SBS_RIGHTALIGN              = 0004h
SBS_SIZEBOXTOPLEFTALIGN     = 0002h
SBS_SIZEBOXBOTTOMRIGHTALIGN = 0004h
SBS_SIZEBOX                 = 0008h

IFNDEF NOSYSMETRICS

; GetSystemMetrics() codes
SM_CXSCREEN           =  0
SM_CYSCREEN           =  1
SM_CXVSCROLL          =  2
SM_CYHSCROLL          =  3
SM_CYCAPTION          =  4
SM_CXBORDER           =  5
SM_CYBORDER           =  6
SM_CXDLGFRAME         =  7
SM_CYDLGFRAME         =  8
SM_CYVTHUMB           =  9
SM_CXHTHUMB           =  10
SM_CXICON             =  11
SM_CYICON             =  12
SM_CXCURSOR           =  13
SM_CYCURSOR           =  14
SM_CYMENU             =  15
SM_CXFULLSCREEN       =  16
SM_CYFULLSCREEN       =  17
SM_CYKANJIWINDOW      =  18
SM_MOUSEPRESENT       =  19
SM_CYVSCROLL          =  20
SM_CXHSCROLL          =  21
SM_DEBUG              =  22
SM_SWAPBUTTON         =  23
SM_RESERVED1          =  24
SM_RESERVED2          =  25
SM_RESERVED3          =  26
SM_RESERVED4          =  27
SM_CXMIN              =  28
SM_CYMIN              =  29
SM_CXSIZE             =  30
SM_CYSIZE             =  31
SM_CXFRAME            =  32
SM_CYFRAME            =  33
SM_CXMINTRACK         =  34
SM_CYMINTRACK         =  35

IFNDEF NOWIN31
SM_CXDOUBLECLK        =  36
SM_CYDOUBLECLK        =  37
SM_CXICONSPACING      =  38
SM_CYICONSPACING      =  39
SM_MENUDROPALIGNMENT  =  40
SM_PENWINDOWS         =  41
SM_DBCSENABLED        =  42
SM_CMOUSEBUTTONS      =  43
SM_SECURE             =  44
ENDIF
IFNDEF NOWIN40
SM_CXEDGE             =  45
SM_CYEDGE             =  46
SM_CXMINSPACING       =  47
SM_CYMINSPACING       =  48
SM_CXSMICON           =  49
SM_CYSMICON           =  50
SM_CYSMCAPTION        =  51
SM_CXSMSIZE           =  52
SM_CYSMSIZE           =  53
SM_CXMENUSIZE         =  54
SM_CYMENUSIZE         =  55
SM_ARRANGE            =  56
SM_USERTYPE           =  57
SM_XWORKAREA          =  58
SM_YWORKAREA          =  59
SM_CXWORKAREA         =  60
SM_CYWORKAREA         =  61
SM_CYCAPTIONICON      =  62
SM_CYSMCAPTIONICON    =  63
SM_CXMINIMIZED        =  64
SM_CYMINIMIZED        =  65
SM_CXMAXTRACK         =  66
SM_CYMAXTRACK         =  67
SM_CXMAXIMIZED        =  68
SM_CYMAXIMIZED        =  69
SM_SHOWSOUNDS         =  70
SM_KEYBOARDPREF       =  71
SM_HIGHCONTRAST       =  72
SM_SCREENREADER       =  73
SM_CURSORSIZE         =  74
SM_CLEANBOOT	      =  75
SM_CXDRAG             =  76
SM_CYDRAG             =  77
SM_NETWORK	          =  78
SM_CXMENUCHECK        =  79
SM_CYMENUCHECK        =  80
ENDIF
SM_MAX		          =  80
SM_CMETRICSMAX        =  81

ENDIF   ;NOSYSMETRICS

IFNDEF  NOCOLOR

COLOR_SCROLLBAR           = 0
COLOR_BACKGROUND          = 1
COLOR_ACTIVECAPTION       = 2
COLOR_INACTIVECAPTION     = 3
COLOR_MENU                = 4
COLOR_WINDOW              = 5
COLOR_WINDOWFRAME         = 6
COLOR_MENUTEXT            = 7
COLOR_WINDOWTEXT          = 8
COLOR_CAPTIONTEXT         = 9
COLOR_ACTIVEBORDER        = 10
COLOR_INACTIVEBORDER      = 11
COLOR_APPWORKSPACE        = 12
COLOR_HIGHLIGHT           = 13
COLOR_HIGHLIGHTTEXT       = 14
COLOR_BTNFACE             = 15
COLOR_BTNSHADOW           = 16
COLOR_GRAYTEXT            = 17
COLOR_BTNTEXT             = 18
IFNDEF NOWIN31
COLOR_INACTIVECAPTIONTEXT = 19
COLOR_BTNHILIGHT          = 20
ENDIF
ENDIF   ;NOCOLOR

; Commands to pass WinHelp()
HELP_CONTEXT	=0001h	;/* Display topic in ulTopic */
HELP_QUIT	=0002h	;/* Terminate help */
HELP_INDEX	=0003h	;/* Display index */
HELP_HELPONHELP =0004h	;/* Display help on using help */
HELP_SETINDEX	=0005h	;/* Set the current Index for multi index help */
HELP_KEY	=0101h	;/* Display topic for keyword in offabData */

IFNDEF NOCOMM

NOPARITY        =   0
ODDPARITY       =   1
EVENPARITY      =   2
MARKPARITY      =   3
SPACEPARITY     =   4

ONESTOPBIT      =   0
ONE5STOPBITS    =   1
TWOSTOPBITS     =   2

IGNORE          =   0      ; /* Ignore signal    */
INFINITE        =   0FFFFh ; /* Infinite timeout */

; Error Flags
CE_RXOVER       =    0001h ; /* Receive Queue overflow       */
CE_OVERRUN      =    0002h ; /* Receive Overrun Error        */
CE_RXPARITY     =    0004h ; /* Receive Parity Error         */
CE_FRAME        =    0008h ; /* Receive Framing error        */
CE_BREAK        =    0010h ; /* Break Detected               */
CE_CTSTO        =    0020h ; /* CTS Timeout                  */
CE_DSRTO        =    0040h ; /* DSR Timeout                  */
CE_RLSDTO       =    0080h ; /* RLSD Timeout                 */
CE_TXFULL       =    0100h ; /* TX Queue is full             */
CE_PTO          =    0200h ; /* LPTx Timeout                 */
CE_IOE          =    0400h ; /* LPTx I/O Error               */
CE_DNS          =    0800h ; /* LPTx Device not selected     */
CE_OOP          =    1000h ; /* LPTx Out-Of-Paper            */
CE_MODE         =    8000h ; /* Requested mode unsupported   */

IE_BADID        =    (-1)  ;  /* Invalid or unsupported id    */
IE_OPEN         =    (-2)  ;  /* Device Already Open          */
IE_NOPEN        =    (-3)  ;  /* Device Not Open              */
IE_MEMORY       =    (-4)  ;  /* Unable to allocate queues    */
IE_DEFAULT      =    (-5)  ;  /* Error in default parameters  */
IE_HARDWARE     =    (-10) ;  /* Hardware Not Present         */
IE_BYTESIZE     =    (-11) ;  /* Illegal Byte Size            */
IE_BAUDRATE     =    (-12) ;  /* Unsupported BaudRate         */

; Events
EV_RXCHAR       =    0001h ; /* Any Character received       */
EV_RXFLAG       =    0002h ; /* Received certain character   */
EV_TXEMPTY      =    0004h ; /* Transmitt Queue Empty        */
EV_CTS          =    0008h ; /* CTS changed state            */
EV_DSR          =    0010h ; /* DSR changed state            */
EV_RLSD         =    0020h ; /* RLSD changed state           */
EV_BREAK        =    0040h ; /* BREAK received               */
EV_ERR          =    0080h ; /* Line status error occurred   */
EV_RING         =    0100h ; /* Ring signal detected         */
EV_PERR         =    0200h ; /* Printer error occured        */
EV_CTSS         =    0400h ; /* CTS state                    */
EV_DSRS         =    0800h ; /* DSR state                    */
EV_RLSDS        =    1000h ; /* RLSD state                   */
EV_RingTe       =    2000h ; /* Ring Trailing Edge Indicator */


; Escape Functions
SETXOFF         =    1     ;  /* Simulate XOFF received       */
SETXON          =    2     ;  /* Simulate XON received        */
SETRTS          =    3     ;  /* Set RTS high                 */
CLRRTS          =    4     ;  /* Set RTS low                  */
SETDTR          =    5     ;  /* Set DTR high                 */
CLRDTR          =    6     ;  /* Set DTR low                  */
RESETDEV        =    7     ;  /* Reset device if possible     */

LPTx            =    80h   ; /* Set if ID is for LPT device  */

IFNDEF NOWIN31
; new escape functions
GETMAXLPT   equ  8	   ; Max supported LPT id
GETMAXCOM   equ  9	   ; Max supported COM id
GETBASEIRQ  equ 10	   ; Get port base & irq for a port

; Comm Baud Rate indices
CBR_110     equ 0FF10h
CBR_300     equ 0FF11h
CBR_600     equ 0FF12h
CBR_1200    equ 0FF13h
CBR_2400    equ 0FF14h
CBR_4800    equ 0FF15h
CBR_9600    equ 0FF16h
CBR_14400   equ 0FF17h
CBR_19200   equ 0FF18h
;		0FF19h	(reserved)
;		0FF1Ah	(reserved)
CBR_38400   equ 0FF1Bh
;		0FF1Ch	(reserved)
;		0FF1Dh	(reserved)
;		0FF1Eh	(reserved)
CBR_56000   equ 0FF1Fh
;		0FF20h	(reserved)
;		0FF21h	(reserved)
;		0FF22h	(reserved)
CBR_128000  equ 0FF23h
;		0FF24h	(reserved)
;		0FF25h	(reserved)
;		0FF26h	(reserved)
CBR_256000  equ 0FF27h

; notifications passed in low word of lParam on WM_COMMNOTIFY messages
CN_RECEIVE  equ 1	    ; bytes are available in the input queue
CN_TRANSMIT equ 2	    ; fewer than wOutTrigger bytes still
			    ; remain in the output queue waiting
			    ; to be transmitted.
CN_EVENT    equ 4	    ; an enabled event has occurred

ENDIF


DCB     struc
    DCB_Id             db ?  ; /* Internal Device ID              */
    DCB_BaudRate       dw ?  ; /* Baudrate at which runing        */
    DCB_ByteSize       db ?  ; /* Number of bits/byte, 4-8        */
    DCB_Parity         db ?  ; /* 0-4=None,Odd,Even,Mark,Space    */
    DCB_StopBits       db ?  ; /* 0,1,2 = 1, 1.5, 2               */
    DCB_RlsTimeout     dw ?  ; /* Timeout for RLSD to be set      */
    DCB_CtsTimeout     dw ?  ; /* Timeout for CTS to be set       */
    DCB_DsrTimeout     dw ?  ; /* Timeout for DSR to be set       */

    DCB_BitMask1       db ?

    ;   BYTE fBinary: 1;     /* Binary Mode (skip EOF check     */
    ;   BYTE fRtsDisable:1;  /* Don't assert RTS at init time   */
    ;   BYTE fParity: 1;     /* Enable parity checking          */
    ;   BYTE fOutxCtsFlow:1; /* CTS handshaking on output       */
    ;   BYTE fOutxDsrFlow:1; /* DSR handshaking on output       */
    ;   BYTE fDummy: 2;      /* Reserved                        */
    ;   BYTE fDtrDisable:1;  /* Don't assert DTR at init time   */

    DCB_BitMask2       db ?

    ;   BYTE fOutX: 1;       /* Enable output X-ON/X-OFF        */
    ;   BYTE fInX: 1;        /* Enable input X-ON/X-OFF         */
    ;   BYTE fPeChar: 1;     /* Enable Parity Err Replacement   */
    ;   BYTE fNull: 1;       /* Enable Null stripping           */
    ;   BYTE fChEvt: 1;      /* Enable Rx character event.      */
    ;   BYTE fDtrflow: 1;    /* DTR handshake on input          */
    ;   BYTE fRtsflow: 1;    /* RTS handshake on input          */
    ;   BYTE fDummy2: 1;

    DCB_XonChar        db ? ; /* Tx and Rx X-ON character        */
    DCB_XoffChar       db ? ; /* Tx and Rx X-OFF character       */
    DCB_XonLim         dw ? ; /* Transmit X-ON threshold         */
    DCB_XoffLim        dw ? ; /* Transmit X-OFF threshold        */
    DCB_PeChar         db ? ; /* Parity error replacement char   */
    DCB_EofChar        db ? ; /* End of Input character          */
    DCB_EvtChar        db ? ; /* Recieved Event character        */
    DCB_TxDelay        dw ? ; /* Amount of time between chars    */
DCB     ends

COMSTAT     struc
    COMS_BitMask1   db ?

;    BYTE fCtsHold: 1;   /* Transmit is on CTS hold         */
;    BYTE fDsrHold: 1;   /* Transmit is on DSR hold         */
;    BYTE fRlsdHold: 1;  /* Transmit is on RLSD hold        */
;    BYTE fXoffHold: 1;  /* Received handshake              */
;    BYTE fXoffSent: 1;  /* Issued handshake                */
;    BYTE fEof: 1;       /* End of file character found     */
;    BYTE fTxim: 1;      /* Character being transmitted     */


    COMS_cbInQue    dw ?  ;   /* count of characters in Rx Queue */
    COMS_cbOutQue   dw ?  ;   /* count of characters in Tx Queue */
COMSTAT     ends

ENDIF       ;NOCOM

;
; Installable Driver Support
;
; Driver Messages
DRV_LOAD	    = 0001h
DRV_ENABLE	    = 0002h
DRV_OPEN	    = 0003h
DRV_CLOSE	    = 0004h
DRV_DISABLE	    = 0005h
DRV_FREE	    = 0006h
DRV_CONFIGURE	    = 0007h
DRV_QUERYCONFIGURE  = 0008h
DRV_INSTALL	    = 0009h
DRV_REMOVE	    = 000Ah
DRV_EXITSESSION	    = 000Bh
DRV_POWER	    = 000Fh
DRV_QUERYENDSESSION = 0010h
DRV_EXITSESSIONCANCELLED = 0011h
DRV_RESERVED	    = 0800h
DRV_USER	    = 4000h

;LPARAM of DRV_CONFIGURE message and return values
DRVCONFIGINFO struc
    DRVCNF_dwDCISize	      dw ?
    DRVCNF_lpszDCISectionName dd ?
    DRVCNF_lpszDCIAliasName   dd ?
DRVCONFIGINFO ends

DRVCNF_CANCEL	    = 0000h
DRVCNF_OK	    = 0001h
DRVCNF_RESTART	    = 0002h




IFNDEF  NOKERNEL
;
; Common Kernel errors
; 
ERR_GALLOC	= 01030h	; GlobalAlloc Failed
ERR_GREALLOC	= 01031h	; GlobalReAlloc Failed
ERR_GLOCK	= 01032h	; GlobalLock Failed
ERR_LALLOC	= 01033h	; LocalAlloc Failed
ERR_LREALLOC	= 01034h	; LocalReAlloc Failed
ERR_LLOCK	= 01035h	; LocalLock Failed
ERR_ALLOCRES	= 01036h	; AllocResource Failed
ERR_LOCKRES	= 01037h	; LockResource Failed
ERR_LOADMODULE  = 01038h	; LoadModule failed

;
; Common User Errors 
;
ERR_CREATEDLG	     =	01045h ; /* Create Dlg failure due to LoadMenu failure */
ERR_CREATEDLG2	     =	01046h ; /* Create Dlg failure due to CreateWindow Failure */
ERR_REGISTERCLASS    =	01047h ; /* RegisterClass failure due to Class already registered */
ERR_DCBUSY	     =	01048h ; /* DC Cache is full */
ERR_CREATEWND	     =	01049h ; /* Create Wnd failed due to class not found */
ERR_STRUCEXTRA	     =  01050h ; /* Unallocated Extra space is used */
ERR_LOADSTR	     =	01051h ; /* LoadString() failed */
ERR_LOADMENU	     =	01052h ; /* LoadMenu Failed     */
ERR_NESTEDBEGINPAINT =  01053h ; /* Nested BeginPaint() calls */
ERR_BADINDEX	     =  01054h ; /* Bad index to Get/Set Class/Window Word/Long */
ERR_CREATEMENU	     =	01055h ; /* Error creating menu */

;
; Common GDI Errors
;
ERR_CREATEDC	    = 01070h	; /* CreateDC/CreateIC etc., failure */
ERR_CREATEMETA      = 01071h	; /* CreateMetafile failure */
ERR_DELOBJSELECTED  = 01072h	; /* Bitmap being deleted is selected into DC */
ERR_SELBITMAP	    = 01073h	; /* Bitmap being selected is already selected elsewhere */

ENDIF	    ;NOKERNEL
