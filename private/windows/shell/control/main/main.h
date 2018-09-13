/** FILE: main.h *********** Module Header ********************************
 *
 *  Control panel utility library routines for use by control panel applets.
 *  Common definitions, resource ids, typedefs, external declarations and
 *  library routine function prototypes.
 *
 * History:
 *  15:30 on Thur  25 Apr 1991  -by-  Steve Cathcart   [stevecat]
 *        Took base code from Win 3.1 source
 *  10:30 on Tues  04 Feb 1992  -by-  Steve Cathcart   [stevecat]
 *        Updated code to latest Win 3.1 sources
 *  22:00 on Wed   17 Nov 1993  -by-  Steve Cathcart   [stevecat]
 *        Changes for product update
 *
 *  Copyright (C) 1990-1993 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                            Include Files
//==========================================================================
#include <windows.h>
#include "cphelp.h"
#include "uniconv.h"

//==========================================================================
//                        Definitions
//==========================================================================
#define CP_ACCEL    100
#define CP_MENU     200

/* Screen Saver Description String Resource ID */
#define SAVERDESC 1

#define MAX_PRINTERS 16

#define PRIVATEINIFILE 1
#define COMPRESSIONFILES 1

#define DRIVERNAME_LEN  130

#define WINDOWGRID  0

// RCDATA resource type identifiers for TimeZone info (date.c, timezone.rc)

#define NUMBER_STRUCTS  1
#define ZONE_INFO       2
#define STD_DATE        3
#define DST_DATE        4

/* Return codes for Copy() routine  NOTE: Make them less than the LZ return
   codes, for simplicity.
*/
#define COPY_CANCEL        0
#define COPY_SELF         -21
#define COPY_NOCREATE     -22
#define COPY_DRIVEOPEN    -23
#define COPY_NODISKSPACE  -24
#define COPY_NOMEMORY     -25

//
//  String Resource IDs
//


#define INITS                 0                     // Old string id values
#define CLASS               (INITS+16)              // 16
#define WIN_INI             (CLASS+4)               // 20
#define ERRORS              (WIN_INI+12)            // 32
#define CHILDREN            (ERRORS+16)             // 48
#define UTILS               (CHILDREN+16)           // 64
#define COLOR               (UTILS+8)               // 72
#define MYFONT              160                     // 110
#define PRN                 (MYFONT+56)             // 158
#define KBD                 (PRN+2)                 // 160
#define DATE                (KBD+10)                // 170
#define CONFLICT            (DATE+16)               // 186
#define INSTALLIT           (CONFLICT+10)           // 196
#define DESKTOP             (INSTALLIT+10)          // 206
#define STR386              (DESKTOP+22)            // 228
#define REMOVEMSG_COLOR     (STR386+8)              // 236
#define REMOVEMSG_PRN       (REMOVEMSG_COLOR+1)     // 237
#define REMOVEMSG_FONT      (REMOVEMSG_COLOR+2)     // 238
#define REMOVEMSG_PATTERN   (REMOVEMSG_COLOR+3)     // 239
#define MYPORT              (REMOVEMSG_COLOR+8)     // 244
#define DAYSOFWK            (MYPORT+20)             // 264
#define MON_OF_YR           (DAYSOFWK+16)           // 280
#define MOUSE               (MON_OF_YR+48)          // 328
#define INSTALL             (MOUSE+16)              // 344
#define NETWORK             (INSTALL+16)            // 360
#define SYSTEM              500
#define INFO                600
#define INTL                800  /* Int'l string must remain last to prevent
                                    problems when new countries are added */

#ifdef JAPAN
#define SJ_FONTSAMPLE           2950
#define ANSI_FONTSAMPLE         2951
#define HANGEUL_FONTSAMPLE      2952
#define CHINESE_FONTSAMPLE      2953
#define OEM_FONTSAMPLE          2954

/* 2970 - 2979 defined for Wife Font Driver dialog box  and messages used in it
*/
#define FONT_NODRVINSTALLED     2970
#define FONT_NODRVSEL           2971
#define FONT_MULDRVSEL          2972
#define FONT_REMOVEDRV          2973
#define FONT_NODRVFOUND         2974
#define FONT_DRVSREAD           2975
#define FONT_ILLEGALDRV         2976
#define FONT_DRVALRDYINS        2977
#define IDS_WIFE_DELFONTONDRV   2978

#define NOT_A_FONT              2980

#define IDS_WIFE_FF_IS_EXIST    2981
#define IDS_WIFE_CAN_NOT_REMOVE 2982
#define IDS_WIFE_FONTDRIVER_A   2983
#define IDS_WIFE_FONTDRIVER_B   2984

#define IDS_WIFE_ALREADYEXIST   2985
#define IDS_WIFE_UNABLE_CONVERT 2986
#define IDS_WIFE_UNABLE_INSTALL 2987

//#define DPD_ASK_USE           2988
//#define DPD_INTER_ERR         2989
//#define DPD_ASK_OVERWRITE     2990
//#define DPD_ABORTED           2991
//#define DPD_ITS_DPD           2992

#define FONTDRIVERSINF          2988
#define FONTSINF                2989
#define FONTPACKAGES            2997
#define FONTDRIVERS             2998
#define FONTFILESEARCHSPEC      2999

// Borrow these number from DPD_*

#define TTFONT_OUTLINEINF       2990
#define TTFONT_BITMAPSINF       2991
#define TTFONT_BITMAPFOR        2992
#define TTFONT_BITMAPMODE       2993

#define ID_INST_FONT_INSTPATH      3000
#define ID_INST_FONT_DESCRIPTION   3001
#define ID_INST_FONT_OK            3002
#define ID_INST_FONT_CANCEL        3003
#define ID_INST_FONT_BROWSE        3004
#define ID_INST_FONT_HELP          3005

// LONG_DATE_FORMAT
// number 4000 - 4099 is reserved for Japanese special date format
#define JaDAYSOFWK              4000
#define JaEMPERORYEAR           4020
#define SPACE4                  4030
#define DAYOFWEEKTAIL           4031
#endif

#define IDS_SYSSETCHANGE        1000
#define IDS_TRUETYPECHANGE      1001
#define IDS_COMCHANGE           1002
#define IDS_RESTART             1003
#define IDS_ALLFONTSFILTER      1004
#define IDS_NORMALFONTSFILTER   1005
#define IDS_VIRTUALMEMCHANGE    1006
#define IDS_RECOVERDLGCHANGE    1007
#define IDS_VIRTANDRECCHANGE    1008

#define IDS_TIMEZONES       2048

#define IDS_NETERROR        NETWORK + 1
#define NET_NOTSUPP         NETWORK + 2
#define NET_ERROR           NETWORK + 3
#define NET_MORE            NETWORK + 4
#define NET_POINTER         NETWORK + 5
#define NET_VALUE           NETWORK + 6
#define NET_PASSWORD        NETWORK + 7
#define NET_ACCESS          NETWORK + 8
#define NET_BUSY            NETWORK + 9
#define NET_WINDOWSERR      NETWORK + 10
#define NET_BADUSER         NETWORK + 11
#define NET_OUTOFMEM        NETWORK + 12
#define NET_BADCANCEL       NETWORK + 13

#define NET_JOBID       IDS_NETERROR+0x40
#define NET_JNF         IDS_NETERROR+0x41
#define NET_JNH         IDS_NETERROR+0x42
#define NET_BADQ        IDS_NETERROR+0x43
#define NET_BFH         IDS_NETERROR+0x44
#define NET_COPIES      IDS_NETERROR+0x45

#define WNNC_CON_All (WNNC_CON_AddConnection | WNNC_CON_CancelConnection | WNNC_CON_GetConnections)


#define MEASUREMENTSYS       KBD + 4
#define MEASUREMENTSYSTEMS   2

/* color resources */
#define ALLELEMENTS         999
#define BACKGROUND           0
#define MDIWINDOW            1
#define CLIENT               2
#define CLIENTTEXT           3
#define MENUBAR              4
#define MENUTEXT             5
#define MYCAPTION            6
#define CAPTION2             7
#define CAPTIONTEXT          8
#define BORDER               9
#define BORDER2             10
#define WINDOWFRAME         11
#define SCROLLBARS          12
#define BUTTONFACE          13
#define BUTTONSHADOW        14
#define BUTTONTEXT          15
#define GRAYTEXT            16
#define HIGHLIGHT           17
#define HIGHLIGHTTEXT       18
#define CAPTION2TEXT        19
#define BUTTONHIGHLIGHT     20
#define ACTIVESTRING        21
#define INACTIVESTRING      22
#define MENUTEXTSTRING      23
#define WINDOWTEXTSTRING    24
#define GRAYTEXTSTRING      25
#define HIGHLIGHTTEXTSTRING 26
#define COLORSCHEMES        27
#define SCHEMEERR           (COLORSCHEMES+4)

#ifdef JAPAN
#define WINDOWTEXTSTRING13  (SCHEMEERR + 6)
#endif

#define EASY_ADD  0x01
#define FORCE_ADD 0x02

#define CONFIG_CANCEL -3
#define CONFIG_REMOVE -2

#define  NOSELECT -1        /* indices for int Selected; */
#define  HOUR      0        /* index into rDateTime, wDateTime, wRange */
#define  MINUTE   1
#define  SECOND   2
#define  MONTH      3
#define  DAY      4
#define  YEAR      5
#define  WEEKDAY  6

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
#define  JaDAY  7       // "Getsuyo-bi" etc...
#define  JaYEAR 8       // "Heisei/Syowa" etc..
#endif

#if 0
#define  UPTIME   6
#define  DOWNTIME 7
#define  UPDATE   8
#define  DOWNDATE 9
#endif

#define LDF_HOUR    (HOUR << 4)
#define LDF_MINUTE  (MINUTE << 4)
#define LDF_SECOND  (SECOND << 4)
#define LDF_MONTH   (MONTH << 4)
#define LDF_DAY     (DAY << 4)
#define LDF_YEAR    (YEAR << 4)

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
#define LDF_JaDAY   (JaDAY << 4)
#define LDF_JaYEAR  (JaYEAR << 4)
#define LDF_SEP     ((JaYEAR+1) << 4)
#else
#define LDF_SEP     ((YEAR+1) << 4)
#endif

#define MAX_DEC_DIGITS 6

#define MAX_LDF_SEP 6

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
#define MAX_SPACE_NUM 4
#endif

typedef struct tagLDF
{
  WORD  Leadin;
  TCHAR LeadinSep[MAX_LDF_SEP];
  WORD  Order[3];

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
// LONG_DATE_FORMAT
  // additional separator is required
  TCHAR Sep[3][MAX_LDF_SEP];
#else
  TCHAR Sep[2][MAX_LDF_SEP];
#endif

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
// LONG_DATE_FORMAT
  // Japanese date format will have day string (sunday..) to the tail
  WORD Trailin; // this is used as flag
#endif

} LDF;

typedef LDF FAR *LPLDF;
typedef LDF NEAR *PLDF;

#define CONTROLICON     21
#define NINEPINICON     23

#define FIRSTCHILDICON  24
#define COLORICON       24
#define PRNICON         25
#define FONTICON        26
#define INTLICON        27
#define PORTSICON       28
#define KEYBRDICON      29
#define MOUSEICON       30
#define DATETIMEICON    31
#define DESKTOPICON     32
#define SOUNDICON       33
#define NETICON         34
#define WIN386ICON      35
#define POWERICON       36
#define SYSTEMICON      37
#define TYPE1ICON       38

#define FIRSTCHILD          0
#define CHILD_COLOR         0
#define CHILD_PRINTER       1
#define CHILD_FONT          2
#define CHILD_INTL          3
#define CHILD_PORTS         4
#define CHILD_KEYBOARD      5
#define CHILD_MOUSE         6
#define CHILD_DATETIME      7
#define CHILD_DESKTOP       8
#define CHILD_SOUND         9
#define CHILD_NETWORK      10
#define CHILD_WIN386       11
#define CHILD_SYSTEM       12
#define LASTCHILD          CHILD_SYSTEM
#define CATAGORIES          LASTCHILD+1

#define DLG_COLOR         100
#define DLG_PRINTER       1
#define DLG_FONT          2
#define DLG_INTL          3
#define DLG_PORTS         4
#define DLG_KEYBOARD      5
#define DLG_MOUSE         6
#define DLG_DATETIME      7
#define DLG_DESKTOP       8
#define DLG_SOUND         9
#define DLG_NETWORK      10
#define DLG_WIN386       11

#define DLG_CONFIGURE    12
#define DLG_PRTSETUP     13
#define DLG_FONT2        14
#define DLG_INTLDATE     15
#define DLG_INTLTIME     16
#define DLG_INTLNUM      17
#define DLG_INTLCUR      18
#define DLG_PORTS2       19

#define DLG_PASSWD       20
#define DLG_NETPRN       21
#define DLG_LOGON        22
#define DLG_CONFLICT     23

#define DLG_RAINBOW      26
#define DLG_COLORSAVE    27
#define DLG_ADDFILE      28
#define DLG_UNLIST       29
#define DLG_INSTALL      30
#define DLG_REMOVEFONT   31
#define DLG_PREVIOUSCON  32
#define DLG_PORTS3       33
#define DLG_PATTERN      34
#define DLG_COLORDEFINE  35
#define DLG_TRUETYPE     36
#define DLG_RESTART      37
#define DLG_BROWSE       38
#define DLG_SYSTEM       39
#define DLG_ADDOS        40
#define DLG_VIRTUALMEM   41
#define DLG_TASKING      42
#ifdef JAPAN
#define DLG_INST_FONT    43
#define DLG_INST_FONT_BROWSE 44
#endif // JAPAN
#define DLG_COREDUMP      45
#define DLG_PROGRESS      46
#define DLG_INSTALL_PS    47
#define DLG_REMOVEFONT_PS 48


#define CONTROLABOUT     99

/* These constants serve a dual purpose:  They are both the menu ID
   as well as the value to be passed to WinHelp.  If these values are
   changed, change the code so it passes the appropriate ContextID
   when calling WinHelp.     15 Sept 1989  Clark R. Cyr               */

#define MENU_SCHHELP     33

#define MENU_INDHELP     40
#define MENU_USEHELP     41
#define MENU_ABOUT       50
#define MENU_EXIT        51

#define MYNUL     (LPTSTR) szNull

#define OUT_OF_MEM   -1         /* current undocumented DlgBox() out of mem
                                 return */
#define FOO -1                  /* for useless control ids */

#define  THE_LISTBOX    20           /* general use */
#define  PUSH_OK        IDOK
#define  PUSH_RESET     21
#define  PUSH_CANCEL    IDCANCEL
#define  PUSH_SETUP     23
#define  EDIT_PATH      26
#define  EDIT_FILE      28
#define  PUSH_SAVEIT    29

#define  CHILDBITMAPS   50             /* Bitmaps from 50 - 59 */

// World bitmap for TimeZones

#define  WORLD          51

#define COLORDEFS       21

#define COLOR_BOX1       32
#define COLOR_BOX2       33
#define COLOR_BOX3       34
#define COLOR_BOX4       35
#define COLOR_BOX5       36
#define COLOR_BOX6       37
#define COLOR_BOX7       38
#define COLOR_BOX8       39
#define COLOR_BOX9       40
#define COLOR_BOX10      41
#define COLOR_BOX11      42
#define COLOR_BOX12      43
#define COLOR_BOX13      44
#define COLOR_BOX14      45
#define COLOR_BOX15      46
#define COLOR_BOX16      47
#define COLOR_BOX17      48
#define COLOR_BOX18      49
#define COLOR_BOX19      50
#define COLOR_BOX20      51
#define COLOR_BOX21      52
#define COLOR_BOX22      53
#define COLOR_BOX23      54
#define COLOR_BOX24      55
#define COLOR_BOX25      56
#define COLOR_BOX26      57
#define COLOR_BOX27      58
#define COLOR_BOX28      59
#define COLOR_BOX29      60
#define COLOR_BOX30      61
#define COLOR_BOX31      62
#define COLOR_BOX32      63
#define COLOR_BOX33      64
#define COLOR_BOX34      65
#define COLOR_BOX35      66
#define COLOR_BOX36      67
#define COLOR_BOX37      68
#define COLOR_BOX38      69
#define COLOR_BOX39      70
#define COLOR_BOX40      71
#define COLOR_BOX41      72
#define COLOR_BOX42      73
#define COLOR_BOX43      74
#define COLOR_BOX44      75
#define COLOR_BOX45      76
#define COLOR_BOX46      77
#define COLOR_BOX47      78
#define COLOR_BOX48      79
#define COLOR_CUSTOM1    80
#define COLOR_CUSTOM2    81
#define COLOR_CUSTOM3    82
#define COLOR_CUSTOM4    83
#define COLOR_CUSTOM5    84
#define COLOR_CUSTOM6    85
#define COLOR_CUSTOM7    86
#define COLOR_CUSTOM8    87
#define COLOR_CUSTOM9    88
#define COLOR_CUSTOM10   89
#define COLOR_CUSTOM11   90
#define COLOR_CUSTOM12   91
#define COLOR_CUSTOM13   92
#define COLOR_CUSTOM14   93
#define COLOR_CUSTOM15   94
#define COLOR_CUSTOM16   95

#define COLORBOXES       64


#define COLOR_HUESCROLL   700           /* color dialog */
#define COLOR_SATSCROLL   701
#define COLOR_LUMSCROLL   702
#define COLOR_HUE         703
#define COLOR_SAT         704
#define COLOR_LUM         705
#define COLOR_RED         706
#define COLOR_GREEN       707
#define COLOR_BLUE        708
#define COLOR_CURRENT     709
#define COLOR_RAINBOW     710
#define COLOR_SAVE        711
#define COLOR_REMOVE      712
#define COLOR_RESET       713
#define COLOR_TUNE        714
#define COLOR_SCHEMES     715
#define COLOR_ELEMENT     716
#define COLOR_SAMPLES     717
#define COLOR_PALETTE     718
#define COLOR_MIX         719

/* Scroll id's are order dependent, and must match the HSL & RGB order */
#define HUESCROLL         720
#define SATSCROLL         721
#define LUMSCROLL         722
#define REDSCROLL         723
#define GREENSCROLL       724
#define BLUESCROLL        725

#define COLOR_ADD          731

#define VALID_ENTRY       0
#define INVALID_ENTRY     1
#define INVALID_ID        2

#define RESTART_TEXT      100

#define FIRST_COUNTRY     INTL+2       /* international strings */

#define INTL_COUNTRY        200        /* International dialog */
#define INTL_LANGUAGE       201
#define INTL_KEYBOARD       202
#define INTL_MEASUREMENT    203
#define INTL_RESET          204
#define INTL_LISTSEP        205
#define INTL_DATEACCEL      206
#define INTL_DATECHANGE     207
#define INTL_DATECHANGE2    208
#define INTL_DATESAMPLE     209
#define INTL_DATESAMPLE2    210
#define INTL_TIMEACCEL      211
#define INTL_TIMECHANGE     212
#define INTL_TIMECHANGE2    213
#define INTL_TIMESAMPLE     214
#define INTL_NUMACCEL       215
#define INTL_NUMCHANGE      216
#define INTL_NUMCHANGE2     217
#define INTL_NUMSAMPLE      218
#define INTL_NEGNUMSAMPLE       228    /* ! OUT OF ORDER ! */
#define INTL_CURACCEL       219
#define INTL_CURCHANGE      220
#define INTL_CURCHANGE2     221
#define INTL_CURSAMPLE      222
#define INTL_NEGSAMPLE      223
#define DATE_SEP            224

#define MDY               231
#define DMY               232
#define YMD               233
#define DAY_LEADINGZERO   234
#define MONTH_LEADINGZERO 235
#define CENTURY           236
#define LONG_MDY          237
#define LONG_DMY          238
#define LONG_YMD          239
#define DAYOFWEEK         240
#define DAYLONG           241
#define MONTHLONG         242
#define YEARLONG          243
#define SPACE1            244
#define SPACE2            245
#define SPACE3            246
#define LDATESAMPLE       247

#define TIME_AM           250
#define TIME_PM           251
#define TIME_12           252
#define TIME_24           253
#define TIME_NOHOUR0      254
#define TIME_HOUR0        255
#define TIME_SEP          256
#define TIME_MERIDIAN     257
#define TIME_MERIDIAN2    258

#define TIME_SUFFIX       10250
#define TIME_PREFIX       10251

#define NUM_1000SEP       260
#define NUM_DECSEP        261
#define NUM_DECDIGITS     262
#define NUM_NOLEAD0       263
#define NUM_LEAD0         264
#define NUM_NEG           265

#define CUR_FORMAT1       270
#define CUR_FORMAT2       271
#define CUR_FORMAT3       272
#define CUR_FORMAT4       273
#define CUR_NEG           274
#define CUR_SYMBOL        275
#define CUR_DECDIGITS     276
#define CUR_1000SEP       277
#define CUR_DECSEP        278

#define LBOX_PRINTER      300           /* printer dialog */
#define LBOX_ADDPRINTER   301
#define PRN_DEFAULT       302
#define PRN_DEFAULT2      303
#define PRN_ADD           304
#define PRN_CONFIGURE     305
#define PRN_PORTS         306
#define PRN_SETUP         307
#define PRN_REMOVE        308
#define PRN_ACTIVE        309
#define PRN_INACTIVE      310
#define PRN_SPOOLER       311
#define PRN_INSTALL       312
#define PRN_ADDPRNSTRING  313
#define PRN_NAME          314
#define PRN_DNS           315
#define PRN_TR            316
#define PRN_NETWORK       317
#define UNLIST_LB         318
#define PRN_DNSTXT        319
#define PRN_TRTXT         320
#define PRN_DOSPRINT      321
#define PRN_RESET         322
#define PRN_INSTRUCT      323

#define LBOX_INSTALLED    400           /* font dialog */
#define FONT_STATUS       401
#define FONT_SAMPLE       402
#define ADDFONT           403
#define DELFONT           404
#define SAMPLEFRAME       405
#define FONTNAME          406
#define INSTALLFONT       407
#define FONT_DISKSPACE    408
#define FONT_TRUETYPE     409

#define FONT2_FILENAME    410
#define FONT2_CURDIR      411
#define FONT2_DIRS        412
#define FONT2_FILESSINGLE 413
#define FONT2_FILESMULTI  414
#define FONT2_FILENAMELABEL   415
#define FONT2_FILESLABEL  416
#define FONT2_DISKSPACE   417
#define FONT_REMOVEMSG    418
#define FONT_REMOVECHECK  419
#define FONT_TRUETYPEONLY 420
#define FONT_CONVERT_PS   431
#define FONT_INSTALL_PS   432
#define FONT_COPY_PS      433
#define FONT_REMOVE_PS    434
#define FONT_INSTALLMSG   435

#define CLICKMIN          100      /* milliseconds */
#define CLICKMAX          900
#define CLICKSUM        (CLICKMIN+CLICKMAX)
#define CLICKRANGE      (CLICKMAX-CLICKMIN)

#define MOUSE_SWAP        500
#define MOUSE_LFRAME      501
#define MOUSE_RFRAME      502
#define MOUSE_LEFT        503
#define MOUSE_RIGHT       504
#define MOUSE_OUTLINE     505
#define MOUSE_LBUTTON     506
#define MOUSE_RBUTTON     507
#define MOUSE_NAME        508
#define MOUSE_TRAILS      509
#define MOUSE_SNAP        510

#define MOUSE_DBLCLKFRAME 529
#define MOUSE_DBLCLK      530
#define MOUSE_CLICKSCROLL 531
#define MOUSE_SPEEDSCROLL 532
#define MOUSE_GEARS       533
#define MOUSE_GEAR1       534
#define MOUSE_GEAR2       535
#define MOUSE_GEAR3       536
#define MOUSE_GEAR4       537
#define MOUSE_GEAR5       538
#define MOUSE_GEARSCROLL  539

#define KSPEED_EDIT       600
#define KSPEED_SCROLL     601
#define KSPEED_MODEL      602
#define KDELAY_SCROLL     603

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
#define KEYMODE_ROMAN     604
#define KEYMODE_KANJI     605
#endif

#define NET_USERNAME      610
#define NET_LOGON         611
#define NET_LOGOFF        612
#define NET_SETTINGS      613
#define NET_PASSWD        614
#define NET_PRNS          615

#define LOGON_USER        620
#define LOGON_PASSWD      621
#define LOGON_CLEAR       622

#define PASSWD_OLD        625
#define PASSWD_NEW        626
#define PASSWD_NEW2       627

#define NETPRN_LPT1       630
#define NETPRN_LPT2       631
#define NETPRN_LPT3       632
#define NETPRN_LPT4       633
#define NETPRN_PORT       634
#define NETPRN_PATH       635
#define NETPRN_PASSWD     636
#define NETPRN_CONNECT    637
#define NETPRN_DISCON     638
#define NETPRN_SHOWPASSWD 639
#define NETPRN_BROWSE     640
#define NETPRN_RECON      641
#define NETPRN_PREV       642
#define NETPRN_ADDPREV    643
#define NETPRN_DELPREV    644

/* Order of HOUR, MINUTE, SECOND, MONTH, DAY, YEAR critical */
#define DATETIME          700
#define DATETIME_HOUR     701
#define DATETIME_MINUTE   702
#define DATETIME_SECOND   703
#define DATETIME_MONTH    704
#define DATETIME_DAY      705
#define DATETIME_YEAR     706
#define DATETIME_DSEP1    707
#define DATETIME_DSEP2    708
#define DATETIME_TSEP1    709
#define DATETIME_TSEP2    710
#define DATETIME_DARROW   711
#define DATETIME_TARROW   712
#define DATETIME_AMPM     713
#define DATEBOX           714
#define TIMEBOX           715
#define IDD_TZ_DATE       716
#define IDD_TZ_TIME       717
#define DATETIME_MSG      718

#define IDD_TZ_SDATE      720
#define IDD_TZ_SD_MONTH   721
#define IDD_TZ_SD_DAY     722
#define IDD_TZ_SD_YEAR    723
#define IDD_TZ_SD_SEP1    724
#define IDD_TZ_SD_SEP2    725
#define IDD_TZ_SD_ARROW   726

#define IDD_TZ_EDATE      730
#define IDD_TZ_ED_MONTH   731
#define IDD_TZ_ED_DAY     732
#define IDD_TZ_ED_YEAR    733
#define IDD_TZ_ED_SEP1    734
#define IDD_TZ_ED_SEP2    735
#define IDD_TZ_ED_ARROW   736

#define IDD_TZ_TIMEZONES  740

#define IDD_TZ_DAYLIGHT   745

#define IDD_TZ_WORLD      750

#define WIN386_PORTS       900
#define WIN386_ALWAYS      901
#define WIN386_NEVER       902
#define WIN386_DELAY       903
#define WIN386_DELAYSEC    904
#define WIN386_DLYSCROLL   905
#define WIN386_MINSLICE    906
#define WIN386_SLICESCROLL 907
#define WIN386_FOREGRD     908
#define WIN386_FGSCROLL    909
#define WIN386_BACKGRD     910
#define WIN386_BGSCROLL    911
#define WIN386_EXCLUSIVE   912

#define DESKTOP_SS          950
#define DESKTOP_MINUTES     951
#define DESKTOP_MINSCROLL   952
#define DESKTOP_BEEP        953
#define DESKTOP_BORDER      954
#define DESKTOP_BDRSCROLL   955
#define DESKTOP_GRID        956
#define DESKTOP_GRIDSCROLL  957
#define DESKTOP_BLINK       958
#define DESKTOP_CURSOR      959
#define DESKTOP_SAVER       960
#define DESKTOP_SAVERTIME   961
#define DESKTOP_SAVERSCROLL 962
#define DESKTOP_TEST        963
#define DESKTOP_SETUP       964
#define DESKTOP_SAVERPASSWD 965

#define BLINK              1000

#define CURSORMIN           200
#define CURSORMAX          1200
#define CURSORSUM       (CURSORMIN+CURSORMAX)
#define CURSORRANGE     (CURSORMAX-CURSORMIN)

#define LEFTBOXX            16
#define LEFTBOXY            27
#define LEFTMOUSEX          22
#define LEFTMOUSEY          41
#define RIGHTBOXX           33
#define RIGHTBOXY           27
#define RIGHTMOUSEX         40
#define RIGHTMOUSEY         41

#define PORT_BAUDRATE   800
#define PORT_DATABITS   801
#define PORT_PARITY     802
#define PORT_STOPBITS   803
#define PORT_FLOWCTL    804
#define PORT_ADVANCED   805
#define PORT_BASEIO     806
#define PORT_IRQ        807
#define PORT_SPINNER    808

#define PORT_LB         810
#define PORT_ADD        811
#define PORT_FIFO       812
#define PORT_NUMBER     813
#define SERIAL_DBASE    815
#define PORT_DELETE     816

#define PORT_SETTING    828
#define PORT_TITLE      829

#define PORT_COM1RECT   830
#define PORT_COM2RECT   831
#define PORT_COM3RECT   832
#define PORT_COM4RECT   833

#define PORT_COM1       834
#define PORT_COM2       835
#define PORT_COM3       836
#define PORT_COM4       837

// NT System Applet
#define IDD_SYS_COMPUTERNAME   1100
#define IDD_SYS_OS             1101
#define IDD_SYS_SHOWLIST       1102
#define IDD_SL_TXT1            1103
#define IDD_SYS_SECONDS        1104
#define IDD_SYS_SECSCROLL      1105
#define IDD_SL_TXT2            1106
#define IDD_SYS_LB_SYSVARS     1107
#define IDD_SYS_UVLABEL        1108
#define IDD_SYS_LB_USERVARS    1109
#define IDD_SYS_VAR            1110
#define IDD_SYS_VALUE          1111
#define IDD_SYS_DELUV          1112
#define IDD_SYS_SETUV          1113
#define IDD_SYS_VMEM           1114
#define IDD_SYS_ENABLECOUNTDOWN 1115
#define IDD_SYS_COREDUMP        1116

#define IDD_SYS_ANS_NAME       1150
#define IDD_SYS_ANS_LOCATION   1151

#define IDD_VM_VOLUMES         1160
#define IDD_VM_SF_DRIVE        1161
#define IDD_VM_SF_SPACE        1162
#define IDD_VM_SF_SIZE         1163
#define IDD_VM_SF_SIZEMAX      1164
#define IDD_VM_SF_SET          1165
#define IDD_VM_MIN             1166
#define IDD_VM_RECOMMEND       1167
#define IDD_VM_ALLOCD          1168
#define IDD_VM_ST_INITSIZE     1169
#define IDD_VM_ST_MAXSIZE      1170
#define IDD_VMEM_ICON          1171
#define IDD_VMEM_MESSAGE       1172
#define IDD_VM_REG_SIZE_LIM    1173
#define IDD_VM_REG_SIZE_TXT    1174
#define IDD_VM_RSL_ALLOCD      1175


#define IDD_CDMP_LOG           1200
#define IDD_CDMP_SEND          1201
#define IDD_CDMP_WRITE         1202
#define IDD_CDMP_OVERWRITE     1203
#define IDD_CDMP_FILENAME      1204
#define IDD_CDMP_AUTOREBOOT    1205
#define IDD_CDMP_BROWSE        1206
#define IDD_CDMP_MESSAGE       1207
#define IDD_CDMP_ICON          1208



#define ID_INSTALLMSG           42
#define ID_PROGRESSMSG          43
#define ID_BAR                  44
#define ID_OVERALL              45

/* Flags for ReadLine */
#define RL_MORE_MEM       -1
#define RL_SECTION_END    -2

/* Flags for FillLBPorts */
#define LBPORT            0
#define CBPORT            1
#define COMPORTS          2
#define LPTPORTS          4
#define OTHERPORTS        8
#define NULLPORTFLAG   0x10
#define FANCY           0x20

/* indicies into the winini string array rglpszWinIni[]. 17-Sep-1987. */
#define WININIWINDOWS     0
#define WININICOLORS      1
#define WININIDEVICES     2
#define WININIFONTS       3
#define WININIPORTS       4
#define WININIINTL        5
#define WININIDESKTOP     6
#define WININITRUETYPE    7
#define CWININIENTRIES    8
#define CBWININIENTRIES   56

/* From dmdlgs.h (DeskMan project) */
#define IDD_TEXT             99
#define IDD_PATTERN         101
#define IDD_PATTERNCOMBO    102
#define IDD_EDITPATTERN     103
#define IDD_WALLPAPER       104
#define IDD_WALLCOMBO       105
#define IDD_WALLFILE        106
#define IDD_CENTER          107
#define IDD_TILE            108
#define IDD_ADDPATTERN      109
#define IDD_CHANGEPATTERN   110
#define IDD_DELPATTERN      111
#define IDD_PATH            112
#define IDD_GRIDGRAN        113
#define IDD_GRIDGRANSCROLL  114
#define IDD_PATSAMPLE       115
#define IDD_ICONSPACE       116
#define IDD_ICONSPACESCROLL 117
#define IDD_ICONWRAP        118
#define IDD_HELP            119
#define IDD_BROWSE          120
#define IDD_FASTSWITCH      121
#define IDD_YESALL          122
#define IDD_FULLDRAG        123
#define IDD_FS_NONE         124
#define IDD_FS              125
#define IDD_FS_ENHANCED     126
#define IDD_FS_GROUP        127

/* Special Messages for SETUP */
#define CP_SETFOCUS           (WM_USER + 0x0401)
#define CP_KILLFOCUS          (WM_USER + 0x0402)

/* Special Messages for SETUP */
#define CP_SETUPPRN           (WM_USER + 401)
#define CP_SETUPHELP          (WM_USER + 402)
#define CP_SETUPFONT          (WM_USER + 403)

/* wParams for CP_SETUPHELP message */
#define CPHELP_INSTALLED 1    // a printer has been installed
#define CPHELP_CONFIGIN  2    // now the Connections... dialog
#define CPHELP_CONFIGOUT 3    // we have returned from Connections...
#define CPHELP_SETUPIN   4    // user has pressed Setup...
#define CPHELP_SETUPOUT  5    // user has returned from Setup...

#define PATHMAX MAX_PATH   /* path length max - used for Get...Directory() calls */
#define DESCMAX 129          /* max description in newexe header */
#define MODNAMEMAX 20       /* max module name in newexe header */

#define  SEEK_BEG 0
#define  SEEK_CUR 1
#define  SEEK_END 2

/* Reboot switch for system dlg */
#define RET_ERROR               (-1)
#define RET_NO_CHANGE           0x0
#define RET_VIRTUAL_CHANGE      0x1
#define RET_RECOVER_CHANGE      0x2
#define RET_CHANGE_NO_REBOOT    0x4

#define RET_VIRT_AND_RECOVER (RET_VIRTUAL_CHANGE | RET_RECOVER_CHANGE)

#define IDSYSI_EXCLAMATION      (32515)

//
//  Font file types used in Fonts applet - installation
//

#define NOT_TT_OR_T1        0       //  Neither TrueType or Type 1 font (FALSE)
#define TRUETYPE_FONT       1       //  This is a TrueType font (TRUE)
#define TYPE1_FONT          2       //  This is an Adobe Type1 font
#define TYPE1_FONT_NC       3       //  Type1 font that cannot be converted to TT

//
//  Font file types used in Fonts applet - Main dlg "Installed Fonts" lbox
//

#define IF_OTHER            0       //  TrueType or Bitmap 1 font (FALSE)
#define IF_TYPE1            1       //  Adobe Type1 font
#define IF_TYPE1_TT         2       //  Matching TT font for Adobe Type1 font

#define T1_MAX_DATA     (2 * PATHMAX + 6)

//
//  Return codes from InstallT1Font routine
//

#define TYPE1_INSTALL_IDOK       IDOK        //  User pressed OK from MessageBox error
#define TYPE1_INSTALL_IDYES      IDYES       //  Same as IDOK
#define TYPE1_INSTALL_IDNO       IDNO        //  Font not installed - user pressed NO
#define TYPE1_INSTALL_IDCANCEL   IDCANCEL    //  Entire installation cancelled
#define TYPE1_INSTALL_PS_ONLY     10         //  Only the PS Font installed.
#define TYPE1_INSTALL_PS_AND_MTT  11         //  PostScript Font installed and matching
                                             //   TT font already installed.
#define TYPE1_INSTALL_TT_AND_PS   12         //  PS Font installed and converted to TT.
#define TYPE1_INSTALL_TT_ONLY     13         //  PS Font converted to TT only.
#define TYPE1_INSTALL_TT_AND_MPS  14         //  PS Font converted to TT and matching
                                             //   PS font already installed.


//==========================================================================
//                           Typedefs
//==========================================================================

typedef struct
{
    TCHAR name[PATHMAX];
    TCHAR desc[DESCMAX];
    TCHAR ModName[MODNAMEMAX];
    int ModType;
} BUFTYPE;
typedef BUFTYPE NEAR *PBUFTYPE, FAR *LPBUFTYPE;

/* Suffix length + NULL terminator */
#define TIMESUF_LEN   9

typedef struct              /* International section description */
{
    TCHAR  sCountry[80];    /* Country name */
    int    iCountry;        /* Country code (phone ID) */
    int    iDate;           /* Date mode (0:MDY, 1:DMY, 2:YMD) */
    int    iTime;           /* Time mode (0: 12 hour clock, 1: 24 ) */
    int    iTLZero;         /* Leading zeros for hour (0: no, 1: yes) */
    int    iCurFmt;         /* Currency mode(0: prefix, no separation
                                             1: suffix, no separation
                                             2: prefix, 1 char separation
                                             3: suffix, 1 char separation) */

    int    iCurDec;         /* Currency Decimal Place */
    int    iNegCur;         /* Negative currency pattern:
                                 ($1.23), -$1.23, $-1.23, $1.23-, etc. */
    int    iLzero;          /* Leading zeros of decimal (0: no, 1: yes) */
    int    iDigits;         /* Significant decimal digits */
    int    iMeasure;        /* Metric 0; British 1 */
    TCHAR  s1159[TIMESUF_LEN];  /* Trailing string from 0:00 to 11:59 */
    TCHAR  s2359[TIMESUF_LEN];  /* Trailing string from 12:00 to 23:59 */
    TCHAR  sCurrency[6];    /* Currency symbol string */
    TCHAR  sThousand[6];    /* Thousand separator string */
    TCHAR  sDecimal[6];     /* Decimal separator string */
    TCHAR  sDateSep[6];     /* Date separator string */
    TCHAR  sTime[6];        /* Time separator string */
    TCHAR  sList[6];        /* List separator string */
    TCHAR  sLongDate[80];   /* Long date picture string */
    TCHAR  sShortDate[80];  /* Short date picture string */
    TCHAR  sLanguage[6];    /* Language name */
    short  iDayLzero;       /* Day Leading zero for Short Date format */
    short  iMonLzero;       /* Month Leading zero for Short Date format */
    short  iCentury;        /* Display full century in Short Date format */
    short  iLDate;          /* Long Date mode (0:MDY, 1:DMY, 2:YMD) */
    LCID   lcid;            /* NT NLS Language/Locale Identifier */
    TCHAR  sTimeFormat[80]; /* Time format picture string */
    int    iTimeMarker;     /* Time marker position (0: suffix, 1: prefix) */
    int    iNegNumber;      /* Negative number pattern:
                                 (1.1), -1.1, - 1.1, 1.1-, 1.1 -   */
    TCHAR  sMonThousand[6]; /* Monetary Thousand separator string */
    TCHAR  sMonDecimal[6];  /* Monetary Decimal separator string */

} INTLSTRUCT;
typedef INTLSTRUCT FAR *LPINTL;
typedef INTLSTRUCT NEAR *PINTL;

#ifndef NOARROWS
typedef struct
{
    short lineup;             /* lineup/down, pageup/down are relative */
    short linedown;           /* changes.  top/bottom and the thumb    */
    short pageup;             /* elements are absolute locations, with */
    short pagedown;           /* top & bottom used as limits.          */
    short top;
    short bottom;
    short thumbpos;
    short thumbtrack;
    BYTE  flags;              /* flags set on return                   */
} ARROWVSCROLL;
typedef ARROWVSCROLL NEAR     *NPARROWVSCROLL;
typedef ARROWVSCROLL FAR      *LPARROWVSCROLL;

#define UNKNOWNCOMMAND 1
#define OVERFLOW       2
#define UNDERFLOW      4

#endif

typedef int (*PFNGETNAME)(LPTSTR pszName, LPTSTR pszInf);

/* date.c */
#define TZNAME_SIZE  32
#define TZDISPLAYZ   65

typedef struct _APPLET_TIME_ZONE_INFORMATION
{
    TCHAR      szRegKey[80];
    TCHAR      szDisplayName[TZDISPLAYZ];
    WCHAR      szStandardName[TZNAME_SIZE];
    WCHAR      szDaylightName[TZNAME_SIZE];
    LONG       Bias;
    LONG       StandardBias;
    LONG       DaylightBias;
    SYSTEMTIME StandardDate;
    SYSTEMTIME DaylightDate;
} APPLET_TIME_ZONE_INFORMATION, *PAPPLET_TIME_ZONE_INFORMATION;

//==========================================================================
//                              Macros
//==========================================================================
#define GSM(SM) GetSystemMetrics(SM)
#define GDC(dc, index) GetDeviceCaps(dc, index)

#define LPMIS LPMEASUREITEMSTRUCT
#define LPDIS LPDRAWITEMSTRUCT
#define LPCIS LPCOMPAREITEMSTRUCT

#define LONG2POINT(l, pt)   (pt.y = (int) HIWORD(l),  pt.x = (int) LOWORD(l))

#define IsDBCSLeadByte(x) (FALSE)

//==========================================================================
//                         External Declarations
//==========================================================================
//  DATA
/* exported from cpl.c  */
extern HANDLE hModule;

extern UINT     wHelpMessage;           // stuff for help
extern UINT     wBrowseMessage;         // stuff for help
extern UINT     wBrowseDoneMessage;     // stuff for browse
extern WORD     wMenuID;
extern DWORD    dwMenuBits;
extern DWORD    dwContext;
extern FARPROC  lpfpNextHook;
extern BOOL     bSetup;                 // TRUE if running under Setup

/* Globals for file installation
 */
extern TCHAR pszWinDir[];
extern TCHAR pszSysDir[];
extern TCHAR pszClose[];
extern TCHAR pszContinue[];

extern TCHAR szSharedDir[PATHMAX];
extern char  szFontsDirA[PATHMAX];      // ANSI String!

extern TCHAR szIntl[];
extern TCHAR szFonts[];
extern TCHAR szDesktop[];
extern TCHAR szBoot[];
extern TCHAR szSYSTEMINI[];
extern TCHAR szMOUSEDRV[];
extern TCHAR szSETUPINF[];
extern TCHAR szCONTROLINF[];
extern TCHAR szSCRNSAVEEXE[];
extern TCHAR szScreenSaveActive[];
extern TCHAR szDevices[];
extern TCHAR szPorts[];
extern TCHAR szWindows[];
extern HANDLE hModule;
extern BOOL bMouse, bMouseCapture;
extern BOOL bCursorLock;
extern TCHAR szGenErr[133];
extern TCHAR szErrMem[133];
extern TCHAR szCtlPanel[30];
extern TCHAR szSetupInfPath[PATHMAX];
extern TCHAR szDefNullPort[20];
extern TCHAR szOnString[10], szNull[1];
extern TCHAR szFON[];
extern TCHAR szFOT[];
extern TCHAR szTTF[];
extern TCHAR szTrueType[];
extern TCHAR szComma[];
extern TCHAR szDot[];
extern TCHAR szSpace[];

extern TCHAR szSetupDir[PATHMAX];
extern TCHAR szCtlIni[];
extern TCHAR szSystemIniPath[];


extern short nDisk;
extern TCHAR szDrv[130];
extern TCHAR szDirOfSrc[PATHMAX];
extern short wDateTime[7];
extern short wPrevDateTime[7];
extern HWND hSetup;

extern INTLSTRUCT Current;
extern INTLSTRUCT IntlDef;

#define NUM_NEGNUM_PAT 5
#define NUM_NEG_PAT    16
#define NUM_CUR_PAT    2
#define NUM_SYM_PAT    4

extern TCHAR *pszNegNumPat[NUM_NEGNUM_PAT];
extern TCHAR *pszCurPat[NUM_CUR_PAT];
extern TCHAR *pszNegCurPat[NUM_NEG_PAT];
extern TCHAR *pszSymPlacement[NUM_SYM_PAT];

extern TCHAR szYes[];
extern TCHAR szNo[];


/* color.c  */
extern BOOL    bTuning;
extern HRGN    hIconRgn;
extern HWND    hBox1;
extern HWND    hCustom1;
extern HWND    hSave;
extern DWORD   nCurDsp;
extern DWORD   nCurMix;
extern DWORD   nCurBox;
extern RECT    rColorBox[];
extern BOOL    bMouseCapture;
extern RECT    rSamples;
extern RECT    rSamplesCapture;
extern DWORD   nElementIndex;
extern DWORD   nBoxHeight;
extern DWORD   nBoxWidth;
extern DWORD   nDriverColors;
extern HWND    hRainbowDlg;
extern DWORD   rainbowRGB;
extern DWORD   rgbBoxColor[];
extern DWORD   lCurColors[];
extern DWORD   currentRGB;
extern DWORD   dwContext;
extern WORD    ElementLBItems[];
extern int     Xlat[];
extern TCHAR    szCurrent[];
extern TCHAR    szColorSchemes[];
extern TCHAR    szColorA[];
extern TCHAR    szCustomColors[];
extern TCHAR    szEqual[];
extern TCHAR    szDisplay[];
extern TCHAR    szOEMBIN[];
extern TCHAR    szColors[];
extern TCHAR    szSchemeName[];
extern BYTE    fChanged;
extern RECT    Orig;
extern RECT    rSamples, rSamplesCapture;
extern RECT    rBorderLeft, rBorderTop, rBorderRight, rBorderBottom;
extern RECT    rBorderLeftFrame, rBorderTopFrame;
extern RECT    rBorderRightFrame, rBorderBottomFrame;
extern RECT    rBorderOutline, rBorderInterior;
extern RECT    rBorderOutline2, rBorderInterior2;
extern RECT    rBorderLeft2, rBorderTop2;
extern RECT    rBorderRight2, rBorderBottom2;
extern RECT    rBorderTopFrame2, rBorderRightFrame2;
extern RECT    rBorderLeftFrame2, rBorderBottomFrame2;
extern RECT    rCaptionLeft, rCaptionText, rCaptionRight;
extern RECT    rCaptionLeft2, rCaptionText2, rCaptionRight2;
extern RECT    rMenuBar, rMenuFrame, rMenuBar2, rMenuFrame2, rMenuText;
extern RECT    rUpArrow, rScroll, rDownArrow;
extern RECT    rScrollFrame;
extern RECT    rMDIWindow2;
extern RECT    rMDIWindow, rClient, rClientFrame, rClientText;
extern RECT    rButton;
extern RECT    rPullDown,rPullInside,rGrayText,rHighlight;
extern int     cyCaption, cyBorder, cyIcon, cyMenu, cyVScroll, cyVThumb;
extern int     cxVScroll, cxBorder, cxSize;
extern TCHAR    szActive[40], szInactive[40], szMenu[40];
extern TCHAR    szWindow[40],szGrayText[40], szHighlightText[40];
extern DWORD   CharHeight, CharWidth;
extern DWORD   CharExternalLeading, CharDescent;
extern POINT   ptMenuText, ptTitleText, ptTitleText2;
extern WNDPROC lpprocStatic;
extern DWORD   lPrevColors[];
extern DWORD   rgbBoxColor[];
extern DWORD   rgbBoxColorDefault[];
extern HBITMAP hUpArrow, hDownArrow;
extern HDC     hDCBits;
extern TCHAR   *pszWinStrings[];
extern short   H,L,S;
extern WORD    currentHue;
extern WORD    currentSat;
extern WORD    currentLum;
extern WORD    nHuePos, nSatPos, nLumPos;
extern WORD    nHueWidth, nSatHeight, nLumHeight;
extern RECT    rLumPaint;
extern RECT    rColorSamples;
extern RECT    rLumScroll;
extern RECT    rLumCapture;
extern RECT    rLumPaint;
extern RECT    rLumScroll;
extern RECT    rRainbow;
extern RECT    rRainbowCapture;
extern HBITMAP hRainbowBitmap;
extern HWND    hHSLRGB[];
extern RECT    rCurrentColor;
extern RECT    rNearestPure;

/* conflict.c  */
extern WORD nConfID;

/* date.c  */
extern PAPPLET_TIME_ZONE_INFORMATION Tzi;
extern LONG  NumTimeZones;

/* intl.c  */
extern INTLSTRUCT IntlDef;

//==========================================================================
//                            Function Prototypes
//==========================================================================
/* arrow.c */
short ArrowVScrollProc (short wScroll, short nCurrent, LPARROWVSCROLL lpAVS);
BOOL  OddArrowWindow (HWND);

/* color.c */
BOOL  APIENTRY ColorDlg (HWND hWnd, UINT message, DWORD wParam, LONG lParam);
BOOL  RemoveMsgBox (HWND  hWnd, LPTSTR lpStr1, WORD  wString);
DWORD hexatol (LPTSTR psz);
void  HiLiteBox (HDC hDC, DWORD nBox, DWORD fStyle);
void  ChangeBoxSelection (HWND hWnd, DWORD nNewBox);
void  ChangeBoxFocus (HWND hWnd, DWORD nNewBox);
void  ChangeColorBox (HWND hWnd, DWORD dwRGBcolor);
void  RetractComboBox (HWND hWnd);
BOOL  ColorSchemeMatch (HDC hDC, LPTSTR pszScheme);
short SchemeSelection (HWND hWnd, LPTSTR pszScheme);
void  UpdateScheme (HWND hWnd);
BOOL  SaveScheme (HWND hWnd, UINT message, DWORD wParam, LONG lParam);
DWORD ColorStringFunc (LPTSTR pszScheme);
BOOL  BoxDrawItem (LPDRAWITEMSTRUCT lpDIS);
BOOL  ComboDrawItem (LPDRAWITEMSTRUCT lpDIS);
BOOL  ColorKeyDown (DWORD wParam, DWORD *id);
DWORD ElementFromPt (POINT pt);
void  PaintBox (HDC hDC, DWORD i);
BOOL  SetupScreenDiagram (HWND hWnd);
BOOL  InitTuning (HWND  hWnd);
BOOL  InitColor (HWND hWnd);
void  PaintArrow (HDC hDC, BOOL bArrow);
void  PaintElement (HWND hWnd, HDC hDC, DWORD nIndex);
void  ColorPaint (HWND hWnd, HDC hDC, LPRECT lpPaintRect);
void  StoreToWin (HWND hWnd);
void  CPHelp (HWND hwnd);
DWORD FillFromControlIni (HWND  hWnd, LPTSTR pszSection);


/* color2.c */
void  ChangeColorSettings (HWND  hWnd, DWORD dwRGBcolor);
void  LumArrowPaint (HDC hDC);
void  EraseLumArrow (HDC hDC);
void  EraseCrossHair (HDC hDC);
void  EraseCrossHair (HDC hDC);
void  CrossHairPaint (HDC hDC, DWORD x, DWORD y);
void  NearestSolid (HWND hDlg);
void  SetupRainbowCapture (HWND  hDlg);
BOOL  APIENTRY RainbowDlg (HWND hWnd, UINT message, DWORD wParam, LONG lParam);
void  HLSPostoHLS (DWORD nHLSEdit);
void  HLStoHLSPos (DWORD nHLSEdit);
void  SetHLSEdit (DWORD nHLSEdit);
void  SetRGBEdit (DWORD nRGBEdit);
BOOL  InitRainbow (HWND hWnd);
void  PaintRainbow (HDC hDC, LPRECT lpRect);
void  RainbowPaint (HDC hDC, LPRECT lpPaintRect);
void  RGBtoHLS (DWORD lRGBColor);
WORD  HueToRGB (WORD n1, WORD n2, WORD hue);
DWORD HLStoRGB (WORD hue, WORD lum, WORD sat);
BOOL  RGBEditChange (HWND  hWnd, DWORD nDlgID);

/* cpl.c */
extern void  CPHelp (HWND hwnd);

/* date.c */
VOID CentreWindow (HWND hwnd);
BOOL GetTimeZoneRes (HWND hDlg);
VOID SetTheTimezone (HWND hDlg, int DaylightOption, PAPPLET_TIME_ZONE_INFORMATION ptzi);

/* desktop.c */
BOOL CheckVal(HWND hDlg, WORD wID, WORD wMin, WORD wMax, WORD wMsgID);

/* font.c */
BOOL   DelSharedFile (HWND hDlg, LPTSTR pszFontName, LPTSTR pszFile,
                                 LPTSTR lpPathName, BOOL bCheckShared);
VOID   FixupNulls        (LPTSTR);
void   FontSelChange     (HWND hDlg);
HANDLE MyOpenSystemFile  (LPTSTR lpName, LPTSTR lpPathName, WORD wFlags);
HANDLE OpenFileWithShare (LPTSTR lpszFile, LPTSTR lpPathName, WORD wFlags);
BOOL   TTEnabled         (void);

/* font2.c */
void   AddBackslash     (LPTSTR lpszFile);
HANDLE Careful          (LPTSTR lpFileName, LPTSTR lpDestDir);
HANDLE InspectFontFile  (LPTSTR szFontFile, int *pNumFonts);
HANDLE PassedInspection (HANDLE hLogicalFont, LPTSTR szFileName);

BOOL DeleteT1Install (HWND hDlg, LPTSTR pszDesc, BOOL bDeleteFiles);
BOOL EnumType1Fonts (HWND hLBox);
BOOL GetT1Install (HWND hDlg, LPTSTR pszDesc, LPTSTR pszPfmFile, LPTSTR pszPfbFile);
int  InstallT1Font (HWND hDlg, HWND hLbox, BOOL bCopyTTFile, BOOL bInSharedDir,
                    LPTSTR szPfmName, LPTSTR szDesc);

BOOL InitProgress (HWND hwnd);
BOOL IsPSFont (HWND hDlg, LPTSTR lpszKey, LPTSTR lpszDesc, LPTSTR lpszPfm, LPTSTR lpszPfb, BOOL *pbCreatedPFM, int *lpiFontType);
void Progress2 (int PercentDone, LPTSTR szDesc);
void RemoveDecoration (LPTSTR pszDesc, BOOL bDeleteTrailingSpace);
void ResetProgress ();
void TermProgress ();
void TermPSInstall ();
void UpdateProgress (int iTotalCount, int iFontInstalling, int iProgress);


/* font3.c */
extern LONG  FileLength (LPTSTR);

VOID   ConvertExtension (LPTSTR pszFile, LPTSTR szExt);
VOID   FilesToDescs     (VOID);
void   FontsDropped     (HWND hwnd, HANDLE hDrop);

BOOL APIENTRY FontHookProc (HWND hDlg, UINT iMessage, WPARAM wParam, LONG lParam);
BOOL UniqueFilename (LPTSTR lpszDst, LPTSTR lpszSrc, LPTSTR lpszDir);
BOOL ValidFontFile(LPTSTR szFile, LPTSTR szDesc, int *lpiFontType);

#ifdef  LATER
/* instfls.c */
typedef int (*INSTALL_PROC)(HWND hDlg, WORD wMsg, int i,
                                            LPTSTR *pszFiles, LPTSTR lpszDir);
#define IFF_CHECKINI  0x0001
#define IFF_SRCANDDST 0x0002

#define IF_ALREADY_INSTALLED 1
#define IF_ALREADY_RUNNING 2
#define IF_JUST_INSTALLED 3

LPTSTR  CopyString(LPTSTR szStr);
LPTSTR  MyLoadString(WORD wId);
LPTSTR CpyToChr(LPTSTR pDest, LPTSTR pSrc, TCHAR cChr, int iMax);
VOID  GetDiskAndFile(LPTSTR pszInf, int *nDsk, LPTSTR pszDriver, WORD wSize);
DWORD InstallFiles(HWND hwnd, LPTSTR *pszFiles, int nCount,
                        INSTALL_PROC lpfnNewFile, WORD wFlags);
#endif  //  LATER

/* icur.c */
BOOL APIENTRY CurIntlDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam);

/* idate.c */
BOOL APIENTRY DateIntlDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam);

/* intl.c */
VOID GetDataString (HWND hCB, int nCurrent, LPTSTR pszString, WORD wDataCmd);
int NameFromInf (LPTSTR pszName, LPTSTR pszInf);
void ParseLDF (LPTSTR pszLDate, PLDF pLDF);

BOOL APIENTRY IntlDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam);

int
GetLocaleValue(
    LCID lcid,
    LCTYPE lcType,
    TCHAR *pszStr,
    int size,
    LPTSTR pszDefault);

/* inum.c */
BOOL APIENTRY NumIntlDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam);
BOOL ExistDigits (TCHAR *pszString);

/* itime.c */
BOOL APIENTRY TimeIntlDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam);

/* memutil.c */
LPVOID AllocMem    (DWORD cb);
BOOL   FreeMem     (LPVOID pMem, DWORD  cb);
LPVOID ReallocMem  (LPVOID lpOldMem, DWORD cbOld, DWORD cbNew);
LPTSTR AllocStr    (LPTSTR lpStr);
BOOL   FreeStr     (LPTSTR lpStr);
BOOL   ReallocStr  (LPTSTR *plpStr, LPTSTR lpStr);

#ifdef ANSI_FUNCTIONS
LPTSTR AllocStrA   (LPSTR  lpStr);
BOOL   FreeStrA    (LPSTR  lpStr);
BOOL   ReallocStrA (LPSTR  *plpStr, LPSTR lpStr);
#endif  // ANSI_FUNCTIONS

/* ports.c */
int SetupCommPort(HWND hDlg, int i);
short FillLBWithPorts(HWND hLB, WORD wFlags);

/* utiltext.c */
void GetDate (void);
void GetTime (void);
void SetDate (void);
void SetTime (void);

void SetDateTime (void);                // [stevecat] - new functions
void GetDateTime (void);

DWORD  AddStringToObject (DWORD dwStringObject, LPTSTR lpszSrc, WORD wFlags);
LPTSTR BackslashTerm (LPTSTR pszPath);
void   BorderRect (HDC hDC, LPRECT lpRect, HBRUSH hBrush);
int    Copy (HWND hParent, TCHAR *szSrcFile, TCHAR *szDestFile);
void   ErrMemDlg (HWND hParent);
HANDLE FindRHSIni (LPTSTR pFile, LPTSTR pSection, LPTSTR pRHS);
int    GetSection(LPTSTR lpFile, LPTSTR lpSection, LPHANDLE hSection, LPINT pSize);
int    myatoi (LPTSTR pszInt);
HANDLE StringToLocalHandle (LPTSTR lpStr);

/* util.c */
int    DoDialogBoxParam (int nDlg, HWND hParent, DLGPROC lpProc,
                                        DWORD dwHelpContext, DWORD dwParam);
void   HourGlass (BOOL bOn);
int    MyMessageBox (HWND hWnd, DWORD wText, DWORD wCaption, DWORD wType, ...);
BOOL   RestartDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam);
void   SendWinIniChange (LPTSTR szSection);
int    strpos (LPTSTR,TCHAR);
TCHAR   *strscan (TCHAR *, TCHAR *);
void   StripBlanks (TCHAR * );
BOOL  APIENTRY WantArrows (HWND hWnd, UINT message, DWORD wParam, LONG lParam);

/* virtual.c */
BOOL APIENTRY VirtualMemDlg (HWND hDlg, UINT message, DWORD wParam, LONG lParam);
BOOL APIENTRY CoreDumpDlg( HWND hDlg, UINT message, DWORD wParam, LONG lParam );


/* prictl.c */
BOOL APIENTRY TaskingDlg (HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);


#if DBG
//#ifndef DbgPrint
//void  DbgPrint( char *, ... );
//#endif
#ifndef DbgBreakPoint
void  DbgBreakPoint( void );
#endif
#endif

#ifdef JAPAN    /* V-KeijiY  June.30.1992 */
// for intl.c
DWORD ConvertEraToJapaneseEra(WORD,WORD,WORD);
WORD ConvertStringToInteger( LPTSTR far * );
#endif

#define IDD_SYS_TASKING             96






