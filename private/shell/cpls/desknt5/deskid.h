///////////////////////////////////////////////////////////////////////////////
// Icons

#define IDI_DISPLAY              100


///////////////////////////////////////////////////////////////////////////////
// Strings

#define IDS_DISPLAY_TITLE        100
#define IDS_DISPLAY_DISABLED     102
#define IDS_DISPLAYFORMAT        104
#define IDS_UNKNOWNMONITOR       105
#define IDS_UNKNOWNDEVICE        106
#define IDS_ADVDIALOGTITLE       107
#define IDS_MULTIPLEMONITORS     108

#define IDC_STATIC  -1


#define DLG_SCREENSAVER          150
#define DLG_BACKGROUND           151
#define DLG_APPEARANCE           152
#define DLG_CUSTOMFONT           153
#define DLG_COLORPICK            154
#define DLG_PATTERN              155
#define DLG_MULTIMONITOR         156
#define DLG_SINGLEMONITOR        157
#define DLG_KEEPNEW              158
#define DLG_SAVESCHEME           159
#define DLG_ASKDYNACDS           160
#define DLG_GENERAL              161
#define DLG_FAKE_SETTINGS        163
#define DLG_WIZPAGE              164

#define MENU_MONITOR             667

#define IDS_ICON                  40
#define IDS_NAME                  41
#define IDS_INFO                  42

#define IDB_COLOR4               120
#define IDB_COLOR8               121
#define IDB_ENERGYSTAR           122
#define IDB_COLOR4DITHER         123
#define IDB_COLOR16              124
#define IDB_COLOR24              125

#define IDC_NO_HELP_1            200     // Used in place of IDC_STATIC when context Help
#define IDC_NO_HELP_2            201     // should be disabled for a control

// screen saver controls
#define IDC_CHOICES             1000
#define IDC_METHOD              1001
#define IDC_BIGICON             1002
#define IDC_SETTING             1003
#define IDC_TEST                1004
#define IDC_ENERGYSTAR_BMP      1005
#define IDC_SCREENSAVEDELAY     1006
#define IDC_SCREENSAVEARROW     1007
#define IDC_LOWPOWERCONFIG      1014

#define IDC_DEMO                1015
#define IDC_SSDELAYLABEL        1016
#define IDC_ENERGY_TEXT         1017
#define IDC_ENERGY_TEXT2        1018
#define IDC_ENERGY_TEXT3        1019
#define IDC_USEPASSWORD         1020
#define IDC_SETPASSWORD         1021
#define IDC_SSDELAYSCALE        1022

// background controls
#define IDC_PATLIST             1100
#define IDC_WALLLIST            1101
#define IDC_EDITPAT             1102
#define IDC_BROWSEWALL          1103
#define IDC_CENTER              1104
#define IDC_TILE                1105
#define IDC_PATTERN             1106
#define IDC_WALLPAPER           1107
#define IDC_BACKPREV            1108
#define IDC_TXT_DISPLAY         1109

// background dialog strings
#define IDS_NONE                1100
#define IDS_UNLISTEDPAT         1101
#define IDS_BITMAPOPENERR       1102
#define IDS_DIB_NOOPEN          1103
#define IDS_DIB_INVALID         1104
#define IDS_BADWALLPAPER        1106
#define IDS_BROWSETITLE         1107
#define IDS_BROWSEFILTER        1108

// appearance controls
#define IDC_SCHEMES             1400
#define IDC_SAVESCHEME          1401
#define IDC_DELSCHEME           1402
#define IDC_ELEMENTS            1403
#define IDC_MAINSIZE            1404
#define IDC_FONTNAME            1407
#define IDC_FONTSIZE            1408
#define IDC_FONTBOLD            1409
#define IDC_FONTITAL            1410
#define IDC_SIZEARROWS          1411
#define IDC_MAINCOLOR           1412
#define IDC_TEXTCOLOR           1413

#define IDC_SIZELABEL           1450
#define IDC_COLORLABEL          1451
#define IDC_FONTLABEL           1452
#define IDC_FNCOLORLABEL        1453
#define IDC_FONTSIZELABEL       1454
#define IDC_GRADIENTLABEL       1455

#define IDC_LOOKPREV            1470

// these need to be all clumped together because they are treated as a group
#define IDC_STARTMAINCOLOR      1500
#define IDC_CUSTOMMAINCOLOR     1549
#define IDC_ENDMAINCOLOR        IDC_CUSTOMMAINCOLOR
#define IDC_STARTTEXTCOLOR      1550
#define IDC_CUSTOMTEXTCOLOR     1599
#define IDC_ENDTEXTCOLOR        IDC_CUSTOMTEXTCOLOR
#define IDC_GRADIENT            1600

// appearance elements
#define ELNAME_DESKTOP          1401
#define ELNAME_INACTIVECAPTION  1402
#define ELNAME_INACTIVEBORDER   1403
#define ELNAME_ACTIVECAPTION    1404
#define ELNAME_ACTIVEBORDER     1405
#define ELNAME_MENU             1406
#define ELNAME_MENUSELECTED     1407
#define ELNAME_WINDOW           1408
#define ELNAME_SCROLLBAR        1409
#define ELNAME_BUTTON           1410
#define ELNAME_SMALLCAPTION     1411
#define ELNAME_ICONTITLE        1412
#define ELNAME_CAPTIONBUTTON    1413
#define ELNAME_DISABLEDMENU     1414
#define ELNAME_MSGBOX           1415
#define ELNAME_SCROLLBUTTON     1416
#define ELNAME_APPSPACE         1417
#define ELNAME_SMCAPSYSBUT      1418
#define ELNAME_SMALLWINDOW      1419
#define ELNAME_DXICON           1420
#define ELNAME_DYICON           1421
#define ELNAME_INFO             1422
#define ELNAME_ICON             1423
#define ELNAME_SMICON           1424
#define ELNAME_HOTTRACKAREA     1425

// appearance strings for sample
#define IDS_ACTIVE              1450
#define IDS_INACTIVE            1451
#define IDS_MINIMIZED           1452
#define IDS_ICONTITLE           1453
#define IDS_NORMAL              1454
#define IDS_DISABLED            1455
#define IDS_SELECTED            1456
#define IDS_MSGBOX              1457
#define IDS_BUTTONTEXT          1458
#define IDS_SMCAPTION           1459
#define IDS_WINDOWTEXT          1460
#define IDS_MSGBOXTEXT          1461

#define IDS_BLANKNAME           1480
#define IDS_NOSCHEME2DEL        1481

// appearance preview menu
#define IDR_MENU                  1
#define IDM_NORMAL                10
#define IDM_DISABLED              11
#define IDM_SELECTED              12

// Fake settings page for compatibility with Win95 hooks
#define IDC_CUSTOMFONT          1211    // DONT CHANGE THIS MUST BE 1211 (Win95 compat)
#define IDC_FONTLIST            1212    // DONT CHANGE THIS MUST BE 1212 (Win95 compat)
#define IDC_CHANGEDRV           1213    // DONT CHANGE THIS MUST BE 1213 (Win95 compat)


#define BMP_MONITOR             1250


#define IDC_COUNTDOWN           1217
#define IDC_SHOWQUICKRES        1218

#define IDS_PAT_REMOVE          1231
#define IDS_PAT_CHANGE          1232
#define IDS_PAT_CREATE          1233
#define IDS_REMOVEPATCAPTION    1234
#define IDS_CHANGEPATCAPTION    1235

#define IDS_SCHEME_WARNING       1300
#define IDS_FONTCHANGE_IN_SCHEME 1301
#define IDS_COPYOF_SCHEME        1302



//controls & strings for custom font dialog
#define IDC_CUSTOMSAMPLE        1400
#define IDC_CUSTOMRULER         1501
#define IDC_CUSTOMCOMBO         1502

#define IDS_10PTSAMPLE          1500
#define IDS_RULERDIRECTION      1501
#define IDS_10PTSAMPLEFACENAME  1510


// color picker mini-dialog
#define IDC_16COLORS            1615
#define IDC_COLORCUST           1616
#define IDC_COLOROTHER          1617
#define IDC_COLORETCH           1618

#define IDC_YESDYNA             1620
#define IDC_NODYNA              1621
#define IDC_SHUTUP              1622
#define IDC_DYNA                1623

// patern edit dialog

#define IDD_PATTERN             1700
#define IDD_PATTERNCOMBO        1701
#define IDD_ADDPATTERN          1702
#define IDD_CHANGEPATTERN       1703
#define IDD_DELPATTERN          1704
#define IDD_PATSAMPLE           1705
#define IDD_PATSAMPLE_TXT       1706
#define IDD_PATTERN_TXT         1707

// Multimonitor controls

#define IDC_DISPLAYLIST         1800
#define IDC_DISPLAYDESK         1801
#define IDC_DISPLAYPROPERTIES   1802
#define IDC_DISPLAYNAME         1803
#define IDC_DISPLAYMODE         1804
#define IDC_DISPLAYUSEME        1805
#define IDC_DISPLAYPRIME        1806
#define IDC_FLASH               1810
#define IDC_DISPLAYTEXT         1811
#define IDC_SCREENSAMPLE        1812
#define IDC_COLORSAMPLE         1813
#define IDC_RESXY               1814
#define IDC_RES_LESS            1815
#define IDC_RES_MORE            1816
#define IDC_COLORGROUPBOX       1817
#define IDC_RESGROUPBOX         1818
#define IDC_MULTIMONHELP        1819
#define IDC_DISPLAYLABEL        1820
#define IDC_TROUBLESHOOT        1821
#define IDC_IDENTIFY            1822

// Keep the old value
#define IDC_COLORBOX            1807 // used to be ID_DSP_COLORBOX
#define IDC_SCREENSIZE          1808 // used to be ID_DSP_AREA_SB

// general.cpp
#define IDC_FONTSIZEGRP         1852
#define IDC_FONT_SIZE           1853
#define IDC_CUSTFONTPER         1854
#define IDC_FONT_SIZE_STR       1855
#define IDC_DYNA_TEXT           1856




//
// String IDs
//

// general.cpp

#define IDS_UNKNOWN                         3010
#define IDS_CUSTFONTPER                     3011
#define IDS_CUSTFONTWARN                    3012
#define ID_DSP_TXT_CHANGE_FONT              3013
#define ID_DSP_TXT_ADMIN_INSTALL            3014
#define ID_DSP_CUSTOM_FONTS                 3015
#define ID_DSP_TXT_FONT_LATER               3016
#define ID_DSP_TXT_NEW_FONT                 3017
#define ID_DSP_TXT_FONT_IN_SETUP_MODE       3018
#define ID_DSP_NORMAL_FONTSIZE_TEXT         3019
#define ID_DSP_CUSTOM_FONTSIZE_TEXT         3020


// install2.c

#define ID_DSP_TXT_INSTALL_DRIVER           3030
#define ID_DSP_TXT_BAD_INF                  3031
#define ID_DSP_TXT_DRIVER_INSTALLED         3032
#define ID_DSP_TXT_DRIVER_INSTALLED_FAILED  3033
#define ID_DSP_TXT_NO_USE_UPGRADE           3034


// Multimon.cpp

#define MSG_CONFIGURATION_PROBLEM           3040
#define MSG_INVALID_NEW_DRIVER              3041
#define MSG_INVALID_DEFAULT_DISPLAY_MODE    3042
#define MSG_INVALID_DISPLAY_DRIVER          3043
#define MSG_INVALID_OLD_DISPLAY_DRIVER      3044
#define MSG_INVALID_16COLOR_DISPLAY_MODE    3045
#define MSG_INVALID_DISPLAY_MODE            3046
#define MSG_INVALID_CONFIGURATION           3047
#define ID_DSP_TXT_8BIT_COLOR               3048
#define ID_DSP_TXT_4BIT_COLOR               3049
#define ID_DSP_TXT_15BIT_COLOR              3050
#define ID_DSP_TXT_16BIT_COLOR              3051
#define ID_DSP_TXT_TRUECOLOR24              3052
#define ID_DSP_TXT_TRUECOLOR32              3053
#define ID_DSP_TXT_XBYY                     3054
#define ID_DSP_TXT_SETTINGS                 3055
#define IDS_REVERTBACK                      3056
#define IDS_OLD_DRIVER                      3057
#define IDS_TURNONTITLE                     3058
#define IDS_TURNITON                        3059
#define IDS_TURNONMSG                       3060
#define IDS_WARNFLICK1                      3061
#define IDS_SETTINGS_INVALID                3062
#define IDS_SETTINGS_CANNOT_SAVE            3063
#define IDS_SETTINGS_FAILED_SAVE            3064
#define IDS_CHANGE_SETTINGS                 3065
#define IDS_CHANGESETTINGS_FAILED           3066
#define IDS_DYNAMIC_CHANGESETTINGS_FAILED   3067
// The string to execute the trouble shooter for display settings page.
#define IDS_TROUBLESHOOT_EXEC               3068
#define IDS_PRIMARY                         3069
#define IDS_SECONDARY                       3070
#define IDS_NOTATTACHED                     3071
#define IDS_FREQUENCY                       3072

// ocpage.cpp
#define IDC_DSP_CLRPALGRP                   3100 
#define IDC_DSP_COLORBAR                    3102 
#define IDC_DSP_DSKAREAGRP                  3103 
#define IDC_REFRESH_RATE                    3106   
#define IDC_MONITOR_BITMAP                  3108 

#define IDB_COLORBAR_BITMAP                 3109
#define IDB_MONITOR_BITMAP                  3110 
#define ID_DSP_TXT_COMPATABLE_DEV           3150
#define ID_DSP_TXT_ADMIN_CHANGE             3151
#define ID_DSP_TXT_COLOR                    3153
#define IDS_DEFFREQ                         3154
#define IDS_INTERLACED                      3155
#define IDS_FREQ                            3156
#define IDS_COLOR_RED                       3157
#define IDS_COLOR_GREEN                     3158
#define IDS_COLOR_BLUE                      3159
#define IDS_COLOR_YELLOW                    3160
#define IDS_COLOR_MAGENTA                   3161
#define IDS_COLOR_CYAN                      3162
#define IDS_PATTERN_HORZ                    3163
#define IDS_PATTERN_VERT                    3164
#define MSG_SETTING_KB                      3165
#define MSG_SETTING_MB                      3166
#define IDS_RED_SHADES                      3167
#define IDS_GREEN_SHADES                    3168
#define IDS_BLUE_SHADES                     3169
#define IDS_GRAY_SHADES                     3170
#define ID_DSP_TXT_TEST_MODE                3171
#define ID_DSP_TXT_DID_TEST_WARNING         3172
#define ID_DSP_TXT_DID_TEST_RESULT          3173
#define ID_DSP_TXT_TEST_FAILED              3174
#define IDS_DISPLAY_WIZ_TITLE               3175
#define IDS_DISPLAY_WIZ_SUBTITLE            3176
#define ID_DSP_TXT_DID_TEST_WARNING_LONG    3177

// ntreg.cpp

#define IDS_UNAVAILABLE                     3270


// setupact.log messages

#define IDS_SETUPLOG_MSG_000                3300
#define IDS_SETUPLOG_MSG_001                3301
#define IDS_SETUPLOG_MSG_002                3302
#define IDS_SETUPLOG_MSG_003                3303
#define IDS_SETUPLOG_MSG_004                3304
#define IDS_SETUPLOG_MSG_005                3305
#define IDS_SETUPLOG_MSG_006                3306
#define IDS_SETUPLOG_MSG_007                3307
#define IDS_SETUPLOG_MSG_008                3308
#define IDS_SETUPLOG_MSG_009                3309
#define IDS_SETUPLOG_MSG_010                3310
#define IDS_SETUPLOG_MSG_011                3311
#define IDS_SETUPLOG_MSG_012                3312
#define IDS_SETUPLOG_MSG_013                3313
#define IDS_SETUPLOG_MSG_014                3314
#define IDS_SETUPLOG_MSG_015                3315
#define IDS_SETUPLOG_MSG_016                3316
#define IDS_SETUPLOG_MSG_017                3317
#define IDS_SETUPLOG_MSG_018                3318
#define IDS_SETUPLOG_MSG_019                3319
#define IDS_SETUPLOG_MSG_020                3320
#define IDS_SETUPLOG_MSG_021                3321
#define IDS_SETUPLOG_MSG_022                3322
#define IDS_SETUPLOG_MSG_023                3323
#define IDS_SETUPLOG_MSG_024                3324
#define IDS_SETUPLOG_MSG_025                3325
#define IDS_SETUPLOG_MSG_026                3326
#define IDS_SETUPLOG_MSG_027                3327
#define IDS_SETUPLOG_MSG_028                3328
#define IDS_SETUPLOG_MSG_029                3329
#define IDS_SETUPLOG_MSG_030                3330
#define IDS_SETUPLOG_MSG_031                3331
#define IDS_SETUPLOG_MSG_032                3332
#define IDS_SETUPLOG_MSG_033                3333
#define IDS_SETUPLOG_MSG_034                3334
#define IDS_SETUPLOG_MSG_035                3335
#define IDS_SETUPLOG_MSG_036                3336
#define IDS_SETUPLOG_MSG_037                3337
#define IDS_SETUPLOG_MSG_038                3338
#define IDS_SETUPLOG_MSG_039                3339
#define IDS_SETUPLOG_MSG_040                3340
#define IDS_SETUPLOG_MSG_041                3341
#define IDS_SETUPLOG_MSG_042                3342
#define IDS_SETUPLOG_MSG_043                3343
#define IDS_SETUPLOG_MSG_044                3344
#define IDS_SETUPLOG_MSG_045                3345
#define IDS_SETUPLOG_MSG_046                3346
#define IDS_SETUPLOG_MSG_047                3347
#define IDS_SETUPLOG_MSG_048                3348
#define IDS_SETUPLOG_MSG_049                3349
#define IDS_SETUPLOG_MSG_050                3350
#define IDS_SETUPLOG_MSG_051                3351
#define IDS_SETUPLOG_MSG_052                3352
#define IDS_SETUPLOG_MSG_053                3353
#define IDS_SETUPLOG_MSG_054                3354
#define IDS_SETUPLOG_MSG_055                3355
#define IDS_SETUPLOG_MSG_056                3356
#define IDS_SETUPLOG_MSG_057                3357
#define IDS_SETUPLOG_MSG_058                3358
#define IDS_SETUPLOG_MSG_059                3359
#define IDS_SETUPLOG_MSG_060                3360
#define IDS_SETUPLOG_MSG_061                3361
#define IDS_SETUPLOG_MSG_062                3362
#define IDS_SETUPLOG_MSG_063                3363
#define IDS_SETUPLOG_MSG_064                3364
#define IDS_SETUPLOG_MSG_065                3365
#define IDS_SETUPLOG_MSG_066                3366
#define IDS_SETUPLOG_MSG_067                3367
#define IDS_SETUPLOG_MSG_068                3368
#define IDS_SETUPLOG_MSG_069                3369
#define IDS_SETUPLOG_MSG_070                3370
#define IDS_SETUPLOG_MSG_071                3371
#define IDS_SETUPLOG_MSG_072                3372
#define IDS_SETUPLOG_MSG_073                3373
#define IDS_SETUPLOG_MSG_074                3374
#define IDS_SETUPLOG_MSG_075                3375
#define IDS_SETUPLOG_MSG_076                3376
#define IDS_SETUPLOG_MSG_077                3377
#define IDS_SETUPLOG_MSG_078                3378
#define IDS_SETUPLOG_MSG_079                3379
#define IDS_SETUPLOG_MSG_080                3380
#define IDS_SETUPLOG_MSG_081                3381
#define IDS_SETUPLOG_MSG_082                3382
#define IDS_SETUPLOG_MSG_083                3383
#define IDS_SETUPLOG_MSG_084                3384
#define IDS_SETUPLOG_MSG_085                3385
#define IDS_SETUPLOG_MSG_086                3386
#define IDS_SETUPLOG_MSG_087                3387
#define IDS_SETUPLOG_MSG_088                3388
#define IDS_SETUPLOG_MSG_089                3389
#define IDS_SETUPLOG_MSG_090                3390
#define IDS_SETUPLOG_MSG_091                3391
#define IDS_SETUPLOG_MSG_092                3392
#define IDS_SETUPLOG_MSG_093                3393
#define IDS_SETUPLOG_MSG_094                3394
#define IDS_SETUPLOG_MSG_095                3395
#define IDS_SETUPLOG_MSG_096                3396
#define IDS_SETUPLOG_MSG_097                3397
#define IDS_SETUPLOG_MSG_098                3398
#define IDS_SETUPLOG_MSG_099                3399

//
// Help defines
//

// Appearance Tab
#define IDH_DISPLAY_APPEARANCE_SCHEME                   4120
#define IDH_DISPLAY_APPEARANCE_SAVEAS_BUTTON            4121
#define IDH_DISPLAY_APPEARANCE_SAVEAS_DIALOG_TEXTBOX    4170
#define IDH_DISPLAY_APPEARANCE_DELETE_BUTTON            4122
#define IDH_DISPLAY_APPEARANCE_GRAPHIC                  4123
#define IDH_DISPLAY_APPEARANCE_ITEM_SIZE                4124
#define IDH_DISPLAY_APPEARANCE_FONT_BOLD                4125
#define IDH_DISPLAY_APPEARANCE_FONT_SIZE                4126
#define IDH_DISPLAY_APPEARANCE_FONT_COLOR               4127
#define IDH_DISPLAY_APPEARANCE_FONT_ITALIC              4128
#define IDH_DISPLAY_APPEARANCE_ITEM_COLOR               4129
#define IDH_DISPLAY_APPEARANCE_ITEM_LIST                4130
#define IDH_DISPLAY_APPEARANCE_FONT_LIST                4131
//#define IDH_DISPLAY_APPEARANCE_GRADIENT               4132    //Obsolete in NT5
#define IDH_DISPLAY_APPEARANCE_ITEM_COLOR2              4133

// Custom font size dialog box
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_CUSTOMFONT_LISTBOX    4085
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_CUSTOMFONT_RULER      4086
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_CUSTOMFONT_SAMPLE     4087

// general tab
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_FONTSIZE      4080
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_DYNA          4081
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_RESTART       4082
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_DONT_RESTART  4083 
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_ASK_ME        4084

// Screen saver tab
#define IDH_DISPLAY_SCREENSAVER_SCREENSAVER_PASSWORD_CHECKBOX 4810
#define IDH_DISPLAY_SCREENSAVER_SCREENSAVER_LISTBOX           4111
#define IDH_DISPLAY_SCREENSAVER_SCREENSAVER_WAIT              4112
#define IDH_DISPLAY_SCREENSAVER_SCREENSAVER_PREVIEW           4113
#define IDH_DISPLAY_SCREENSAVER_SCREENSAVER_SETTINGS          4114
#define IDH_DISPLAY_SCREENSAVER_SCREENSAVER_MONITOR           4115
#define IDH_DISPLAY_SCREENSAVER_ENERGYSAVE_GRAPHIC            4116
#define IDH_DISPLAY_SCREENSAVER_POWER_BUTTON                  4117

// Settings tab
#define IDH_DISPLAY_SETTINGS_MONITOR_GRAPHIC            4064  
#define IDH_DISPLAY_SETTINGS_DISPLAY_LIST               4065  
#define IDH_DISPLAY_SETTINGS_COLORBOX                   4066 
#define IDH_DISPLAY_SETTINGS_SCREENAREA                 4067
#define IDH_DISPLAY_SETTINGS_EXTEND_DESKTOP_CHECKBOX    4068  
#define IDH_DISPLAY_SETTINGS_ADVANCED_BUTTON            4069 
#define IDH_DISPLAY_SETTINGS_USE_PRIMARY_CHECKBOX       4072
#define IDH_DISPLAY_SETTINGS_IDENTIFY_BUTTON            4073
#define IDH_DISPLAY_SETTINGS_TROUBLE_BUTTON             4074
