#define IDC_STATIC -1

/* Main Windows */
#define IDC_TOOLBAR             10
#define IDC_STATUSBAR           11
#define IDC_MAIN_MDI            12
/* these need to be kept consecutive */
#define IDS_FLT_TOOLS           20
#define IDS_FLT_COLORS          21
#define IDS_FLT_HISTORY         22

/* Program icon */
#define IDI_IMAGESOFTICON       50

/* Menus */
#define IDR_MAINMENU            102
#define IDR_POPUP               103

/* COMMANDS */

/* main */
#define ID_NEW                  2000
#define ID_OPEN                 2001
#define ID_CLOSE                2002
#define ID_CLOSEALL             2003
#define ID_SAVE                 2004
#define ID_SAVEAS               2005
#define ID_PRINTPRE             2006
#define ID_PRINT                2007
#define ID_PROP                 2008
#define ID_CUT                  2009
#define ID_COPY                 2010
#define ID_PASTE                2011
#define ID_PASTENEWIMAGE        2012
#define ID_UNDO                 2013
#define ID_REDO                 2014
#define ID_SELALL               2015
#define ID_EXIT                 2016
#define ID_EDITCOLOURS          2017
#define ID_TOOLS                2018
#define ID_COLOR                2019
#define ID_HISTORY              2020
#define ID_STATUSBAR            2021
#define ID_BLANK                2022

#define ID_BACK                 2025
#define ID_FORWARD              2026
#define ID_DELETE               2027

/* text */
#define ID_BOLD                 2030
#define ID_ITALIC               2031
#define ID_ULINE                2032
#define ID_TXTLEFT              2033
#define ID_TXTCENTER            2034
#define ID_TXTRIGHT             2035
#define ID_TXTFONTNAME          2036
#define ID_TXTFONTSIZE          2037

/* tools */
#define ID_CLONESTAMP           2050
#define ID_COLORPICKER          2051
#define ID_ECLIPSE              2052
#define ID_ECLIPSESEL           2053
#define ID_ERASER               2054
#define ID_FREEFORM             2055
#define ID_LASOO                2056
#define ID_LINE                 2057
#define ID_MAGICWAND            2058
#define ID_MOVE                 2059
#define ID_MOVESEL              2060
#define ID_PAINTBRUSH           2061
#define ID_PAINTBUCKET          2062
#define ID_PENCIL               2063
#define ID_RECOLORING           2064
#define ID_RECTANGLE            2065
#define ID_RECTSEL              2066
#define ID_ROUNDRECT            2067
#define ID_TEXT                 2068
#define ID_ZOOM                 2069

/* Adjust */
#define ID_BRIGHTNESS           2100
#define ID_CONTRAST             2101
#define ID_BLACKANDWHITE        2102
#define ID_INVERTCOLORS         2103
#define ID_BLUR                 2104
#define ID_SHARPEN              2105

#define ID_ABOUT                2400

#define ID_REFRESH              3000
#define ID_HELP                 3001
#define ID_WINDOW_TILE_HORZ     3002
#define ID_WINDOW_TILE_VERT     3003
#define ID_WINDOW_CASCADE       3004
#define ID_WINDOW_NEXT          3005
#define ID_WINDOW_ARRANGE       3006

/* menu hints */
#define IDS_HINT_BLANK          20000
#define IDS_HINT_NEW            20001
#define IDS_HINT_OPEN           20002
#define IDS_HINT_CLOSE          21006
#define IDS_HINT_CLOSEALL       21007
#define IDS_HINT_SAVE           20003
#define IDS_HINT_SAVEAS         20004
#define IDS_HINT_PRINT          20005
#define IDS_HINT_PRINTPRE       20006
#define IDS_HINT_PROP           20007
#define IDS_HINT_EXIT           20008

#define IDS_HINT_TOOLS          20020
#define IDS_HINT_COLORS         20021
#define IDS_HINT_HISTORY        20022
#define IDS_HINT_STATUS         20023

#define IDS_HINT_CASCADE        21009
#define IDS_HINT_TILE_HORZ      21010
#define IDS_HINT_TILE_VERT      21011
#define IDS_HINT_ARRANGE        21012
#define IDS_HINT_NEXT           21013

/* system menu hints */
#define IDS_HINT_SYS_RESTORE    21001
#define IDS_HINT_SYS_MOVE       21002
#define IDS_HINT_SYS_SIZE       21003
#define IDS_HINT_SYS_MINIMIZE   21004
#define IDS_HINT_SYS_MAXIMIZE   21005
#define IDS_HINT_SYS_CLOSE      21006


/* TOOLBAR BUTTON BITMAPS
 * These must be numbered consecutively
 * See loop in InitImageList (misc.c)
 */

/* standard */
#define TBICON_NEW              0
#define TBICON_OPEN             1
#define TBICON_SAVE             2
#define TBICON_PRINT            3
#define TBICON_PRINTPRE         4
#define TBICON_CUT              5
#define TBICON_COPY             6
#define TBICON_PASTE            7
#define TBICON_UNDO             8
#define TBICON_REDO             9

#define IDB_MAINNEW             10000
#define IDB_MAINOPEN            10001
#define IDB_MAINSAVE            10002
#define IDB_MAINPRINT           10003
#define IDB_MAINPRINTPRE        10004
#define IDB_MAINCUT             10005
#define IDB_MAINCOPY            10006
#define IDB_MAINPASTE           10007
#define IDB_MAINUNDO            10008
#define IDB_MAINREDO            10009


/* text */
#define TBICON_BOLD             0
#define TBICON_ITALIC           1
#define TBICON_ULINE            2
#define TBICON_TXTLEFT          3
#define TBICON_TXTCENTER        4
#define TBICON_TXTRIGHT         5

#define IDB_TEXTBOLD            10020
#define IDB_TEXTITALIC          10021
#define IDB_TEXTULINE           10022
#define IDB_TEXTLEFT            10023
#define IDB_TEXTCENTER          10024
#define IDB_TEXTRIGHT           10025


/* tools */
#define TBICON_RECTSEL          0
#define TBICON_MOVESEL          1
#define TBICON_LASOO            2
#define TBICON_MOVE             3
#define TBICON_ECLIPSESEL       4
#define TBICON_ZOOM             5
#define TBICON_MAGICWAND        6
#define TBICON_TEXT             7
#define TBICON_PAINTBRUSH       8
#define TBICON_ERASER           9
#define TBICON_PENCIL           10
#define TBICON_COLORPICKER      11
#define TBICON_CLONESTAMP       12
#define TBICON_RECOLORING       13
#define TBICON_PAINTBUCKET      14
#define TBICON_LINE             15
#define TBICON_RECTANGLE        16
#define TBICON_ROUNDRECT        17
#define TBICON_ECLIPSE          18
#define TBICON_FREEFORM         19

#define IDB_TOOLSRECTSEL        10030
#define IDB_TOOLSMOVESEL        10031
#define IDB_TOOLSLASOO          10032
#define IDB_TOOLSMOVE           10033
#define IDB_TOOLSECLIPSESEL     10034
#define IDB_TOOLSZOOM           10035
#define IDB_TOOLSMAGICWAND      10036
#define IDB_TOOLSTEXT           10037
#define IDB_TOOLSPAINTBRUSH     10038
#define IDB_TOOLSERASER         10039
#define IDB_TOOLSPENCIL         10040
#define IDB_TOOLSCOLORPICKER    10041
#define IDB_TOOLSCLONESTAMP     10042
#define IDB_TOOLSRECOLORING     10043
#define IDB_TOOLSPAINTBUCKET    10044
#define IDB_TOOLSLINE           10045
#define IDB_TOOLSRECTANGLE      10046
#define IDB_TOOLSROUNDRECT      10047
#define IDB_TOOLSECLIPSE        10048
#define IDB_TOOLSFREEFORM       10049


/* history */
#define TBICON_BACKSM           0
#define TBICON_UNDOSM           1
#define TBICON_REDOSM           2
#define TBICON_FORWARDSM        3
#define TBICON_DELETESM         4

#define IDB_HISTBACK            10060
#define IDB_HISTUNDO            10061
#define IDB_HISTREDO            10062
#define IDB_HISTFORWARD         10063
#define IDB_HISTDELETE          10064

#define IDB_COLORSMORE          10080
#define IDB_COLORSLESS          10081


/* tooltips */
#define IDS_TOOLTIP_NEW         6000
#define IDS_TOOLTIP_OPEN        6001
#define IDS_TOOLTIP_SAVE        6002
#define IDS_TOOLTIP_PRINTPRE    6003
#define IDS_TOOLTIP_PRINT       6004
#define IDS_TOOLTIP_CUT         6005
#define IDS_TOOLTIP_COPY        6006
#define IDS_TOOLTIP_PASTE       6007
#define IDS_TOOLTIP_UNDO        6008
#define IDS_TOOLTIP_REDO        6009

/* cursors */
#define IDC_PAINTBRUSHCURSOR    20001
#define IDC_PAINTBRUSHCURSORMOUSEDOWN 20002


/* DIALOGS */
#define IDC_PICPREVIEW          2999

/* brightness dialog */
#define IDD_BRIGHTNESS          3000
#define IDC_BRI_GROUP           3001
#define IDC_BRI_FULL            3002
#define IDC_BRI_RED             3003
#define IDC_BRI_GREEN           3004
#define IDC_BRI_BLUE            3005
#define IDC_BRI_EDIT            3006
#define IDC_BRI_TRACKBAR        3007

/* image property dialog */
#define IDD_IMAGE_PROP          4000
#define IDC_IMAGE_NAME_EDIT     4001
#define IDC_IMAGETYPE           4003
#define IDC_WIDTH_EDIT          4004
#define IDC_WIDTH_STAT          4005
#define IDC_HEIGHT_EDIT         4006
#define IDC_HEIGHT_STAT         4007
#define IDC_RES_EDIT            4008
#define IDC_RES_STAT            4009
#define IDC_UNIT                4010
#define IDC_IMAGE_SIZE          4011
#define IDS_IMAGE_MONOCHROME    4100
#define IDS_IMAGE_GREYSCALE     4101
#define IDS_IMAGE_PALETTE       4102
#define IDS_IMAGE_TRUECOLOR     4103
#define IDS_UNIT_CM             4104
#define IDS_UNIT_INCHES         4105
#define IDS_UNIT_PIXELS         4106
#define IDS_UNIT_DOTSCM         4107
#define IDS_UNIT_DPI            4108
#define IDS_UNIT_MB             4109
#define IDS_UNIT_KB             4110

/* about box info */
#define IDD_ABOUTBOX            4200
#define IDC_LICENSE_EDIT        4201
#define IDS_APPNAME             4202
#define IDS_VERSION             4203
#define IDS_LICENSE             4204
#define IDS_READY               4205
#define IDS_TOOLBAR_STANDARD    4206
#define IDS_TOOLBAR_TEST        4207
#define IDS_TOOLBAR_TEXT        4208
#define IDS_IMAGE_NAME          4209
