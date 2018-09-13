/*      File: D:\WACKER\term\res.h (Created: 26-Nov-1993)
 *
 *      Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *      All rights reserved
 *
 *      $Revision: 4 $
 *      $Date: 8/05/99 2:31p $
 */

//NOTE: Take this one out if it is defined in some CHICAGO build.
#if !(defined(DS_NONBOLD))
	#define DS_NONBOLD      4L
#endif

/*-----------------------------------------------------------------------------
 * Here are our naming conventions for IDs for dialog, menu, string,
 * accelerator, control and etc.  If you modify this file also modify the
 * corresponding copies (KOPYKAT.RH - KopyKat, HA5G.H - HA/Win, RES.H - Wacker).
 *
 * IDx_[yy_][module_]name
 *
 * where x is           D for dialogs
 *                                      C for controls
 *                                      M for menus
 *                                      A for accelerators
 *                                      S for strings
 *                                      U for RC data
 *                                      V for VERSIONINFO resource
 *
 *               yy is          BM for bitmap
 *                                      CB for combo box
 *                                      CK for check box
 *                                      EF for edit field
 *                                      FR for frame control
 *                                      GR for group box
 *                                      IC for icon
 *                                      LB for list box
 *                                      ML for multi-line edit box
 *                                      PB for push button
 *                                      RB for radio button
 *                                      SB for spin button
 *                                      SL for slider control
 *                                      ST for static text
 *                                      VU for vu meter (custom control)
 *
 *                                      (yy is only used if x is D)
 *
 *                                      NOTE: If you need to make up an id for a control not listed,
 *                                      assign it a unique two letter sequence and add it into
 *                                      the list in alphabetic order.
 *
 *               module         is provided to give us the ability to further distinguish
 *                                      controls.  It will allow for IDC_CB_VT100_CharacterSets
 *                                      and IDC_CB_VT52_CharacterSets to coexist.
 *
 *               name           is the control specific description.
 *
 *--------------------------------------------------------------------------- */

/* ----- Dialog Identifiers ----- */

#define IDD_TRANSFERSEND                100
#define IDD_TRANSFERRECEIVE             200
#define IDD_RECEIVEFILENAME2            300
#define IDD_RECEIVEFILENAME             400
#define IDD_FINDDIRECTORY               500
#define IDD_NEWCONNECTION               600
#define IDD_XFERZMDMSNDSTANDARDDOUBLE   700
#define IDD_XFERZMDMSNDSTANDARDSINGLE   800
#define IDD_XFERZMDMRECSTANDARDDOUBLE   900
#define IDD_XFERZMDMRECSTANDARDSINGLE  1000
#define IDD_XFERYMDMSNDSTANDARDDOUBLE  1100
#define IDD_XFERYMDMSNDSTANDARDSINGLE  1200
#define IDD_XFERYMDMRECSTANDARDDISPLAY 1300
#define IDD_XFERXMDMSNDSTANDARDDISPLAY 1400
#define IDD_XFERXMDMRECSTANDARDDISPLAY 1500
#define IDD_XFERKRMSNDSTANDARDDOUBLE   1600
#define IDD_XFERKRMSNDSTANDARDSINGLE   1700
#define IDD_XFERKRMRECSTANDARDDISPLAY  1800
#define IDD_XFERHPRSNDSTANDARDSINGLE   1900
#define IDD_XFERHPRSNDSTANDARDDOUBLE   2000
#define IDD_XFERHPRRECSTANDARDSINGLE   2100
#define IDD_XFERHPRRECSTANDARDDOUBLE   2200
#define IDD_CAPTURE                    2300
#define IDD_TAB_PHONENUMBER            2400
#define IDD_TAB_TERMINAL               2500
#define IDD_PRINTABORT                 2600
#define IDD_CUSTOM_PAGE_SETUP          2601
#define IDD_ASCII_SETUP                2700
#define IDD_ABOUT_DLG                  2750
#define IDD_UPGRADE_INFO               2751
#define IDR_UPGRADE_TEXT               2752
#define IDD_DEFAULT_TELNET             2753
#define IDD_KEYSUMMARYDLG              2754
#define IDD_KEYDLG                     2755
#define IDD_TERMINAL_SIZE_AND_COLORS   2756
#ifdef INCL_NAG_SCREEN
#define IDD_NAG_SCREEN                 2757
#define IDD_NAG_REGISTER               2758
#define IDD_NAG_PURCHASE               2759
#define IDD_REGISTRATION_REMINDER      2760
#endif
#define IDC_TERMINAL_SIZE_AND_COLORS    1005
#define IDC_EDIT_ROWS                   1006
#define IDC_EDIT_COLUMNS                1007
#ifdef INCL_NAG_SCREEN
#define IDC_NAG_CODE                    1008
#define IDC_NAG_PURCHASE                1009
#define IDC_NAG_TIME                    1010
#define IDC_NAG_ICON                    1011
#define IDC_REGISTER_EDIT               1012
#define IDC_PURCHASE_METHOD_WEB         1013
#define IDC_PURCHASE_METHOD_ORDER       1014
#define IDC_ORDER_INFO_NAME             1015
#define IDC_ORDER_INFO_COMPANY          1016
#define IDC_ORDER_INFO_ADDRESS          1017
#define IDC_ORDER_INFO_PHONE            1018
#define IDC_ORDER_INFO_EXTENSION        1019
#define IDC_CREDIT_CARD_MASTERCARD      1020
#define IDC_CREDIT_CARD_VISA            1021
#define IDC_CREDIT_CARD_AMERICAN_EXPRESS    1022
#define IDC_ORDER_INFO_CARD_NUMBER      1023
#define IDC_ORDER_INFO_CARD_EXPIRATION  1024
#define IDC_ORDER_EMAIL                 1025
#define IDC_ORDER_PRINT                 1027
#define IDC_ORDER_PHONE                 1028
#define IDC_ORDER_HOURS					1029
#define IDC_NAG_EXP_DAYS                1030
#endif
#define IDC_BUTTON3                     1026
#define IDC_STATIC5                     1112
#define IDC_STATIC6                     1113
#define IDC_STATIC7                     1114
#define IDC_TEXT_BLACK                  1303
#define IDC_TEXT_DARKGRAY               1304
#define IDC_TEXT_DARKBLUE               1305
#define IDC_TEXT_BLUE                   1306
#define IDC_TEXT_DARKGREEN              1307
#define IDC_TEXT_GREEN                  1308
#define IDC_TEXT_DARKCYAN               1309
#define IDC_TEXT_CYAN                   1310
#define IDC_TEXT_DARKRED                1311
#define IDC_TEXT_RED                    1312
#define IDC_TEXT_DARKMAGENTA            1313
#define IDC_TEXT_DARKYELLOW             1314
#define IDC_TEXT_LIGHTGRAY              1315
#define IDC_TEXT_MAGENTA                1316
#define IDC_TEXT_YELLOW                 1317
#define IDC_TEXT_WHITE                  1318
#define IDC_BACK_BLACK                  1319
#define IDC_BACK_DARKGRAY               1320
#define IDC_BACK_DARKBLUE               1321
#define IDC_BACK_BLUE                   1322
#define IDC_BACK_DARKGREEN              1323
#define IDC_BACK_GREEN                  1324
#define IDC_BACK_DARKCYAN               1325
#define IDC_BACK_CYAN                   1326
#define IDC_BACK_DARKRED                1327
#define IDC_BACK_RED                    1340
#define IDC_BACK_DARKMAGENTA            1341
#define IDC_BACK_DARKYELLOW             1342
#define IDC_BACK_LIGHTGRAY              1343
#define IDC_BACK_MAGENTA                1344
#define IDC_BACK_YELLOW                 1345
#define IDC_BACK_WHITE                  1346
#define IDC_TEXT_PREVIEW                1347
#define IDC_PREVIEW_GROUPBOX            1348
#define IDC_REGISTER_NOW                1349
#define IDC_REGISTER_LATER              1350
#define IDC_REGISTER_NOT                1351
#define IDC_STATIC                      -1

/* ---- from CNCTTAPI.RC ---- */

#define IDD_CNCT_CONFIRM               2800
#define IDD_CNCT_NEWPHONE              2900
#define IDD_DIALING                                3000
#define IDD_CNCT_PCMCIA                            3100

/* ---- from EMUDLGS.RC  ---- */

#define IDD_ANSI_SETTINGS				3500
#define IDD_VT100_SETTINGS				3600
#define IDD_VT100J_SETTINGS				3650
#define IDD_VT52_SETTINGS				3700
#define IDD_VT220_SETTINGS				3750
#define IDD_VT320_SETTINGS				3755
#define IDD_TTY_SETTINGS				3800
#define IDD_MINITEL_SETTINGS			3900
#define IDD_VIEWDATA_SETTINGS			3950

#define IDD_SCRN_COLOR_SETTINGS			3400

/* ----- Menu Identifiers ----- */

#define IDM_MENU_BASE                   40000

#define IDM_MAIN_MENU_FILE              0
#define IDM_MAIN_MENU_EDIT              1
#define IDM_MAIN_MENU_VIEW              2
#define IDM_MAIN_MENU_ACTIONS   3
#define IDM_MAIN_MENU_HELP              4

#define IDM_NEW                         100
#define IDM_OPEN                        101
#define IDM_SAVE                        102
#define IDM_SAVE_AS                     103
#define IDM_PAGESETUP           104
#define IDM_CHOOSEPRINT         105
#define IDM_PRINT                       106
#define IDM_PROPERTIES          107
#define IDM_EXIT                        108

#define IDM_COPY                        200
#define IDM_PASTE                       201
#define IDM_SELECT_ALL          202
#define IDM_CLEAR_BACKSCROLL    203
#define IDM_CLEAR_SCREEN        204

#define IDM_VIEW_TOOLBAR        300
#define IDM_VIEW_STATUS         301
#define IDM_VIEW_FONTS          302
#define IDM_VIEW_SNAP       303

#define IDM_ACTIONS_DIAL                    400
#define IDM_ACTIONS_HANGUP                  401
#define IDM_ACTIONS_SEND                    402
#define IDM_ACTIONS_RCV                     403
#define IDM_ACTIONS_CAP                     404
#define IDM_ACTIONS_PRINT                   405
#define IDM_ACTIONS_SEND_TEXT       406
#define IDM_ACTIONS_WAIT_FOR_CALL   407
#define IDM_ACTIONS_STOP_WAITING    408

#define IDM_CAPTURE_STOP                420
#define IDM_CAPTURE_PAUSE               421
#define IDM_CAPTURE_RESUME              422

#define IDM_PRNECHO_STOP                424
#define IDM_PRNECHO_PAUSE               425
#define IDM_PRNECHO_RESUME              426

//#define IDM_HELP                              500
#define IDM_HELPTOPICS          501
#define IDM_ABOUT               502
#define IDM_KEY_MACROS          503
#define IDM_PURCHASE_INFO       504
#define IDM_REG_CODE            505
#define IDM_REGISTER            506
#define IDM_DISCUSSION          507

/* --- Identifiers for Test Menu --- */

#define IDM_TEST_SAVEAS                 600
#define IDM_TEST_CLEARTERM              601
#define IDM_TEST_CLEARBACK              602
#define IDM_TEST_SELECTTERM     603
#define IDM_TEST_SELECTBACK     604
#define IDM_TEST_CONNECT                605
#define IDM_TEST_DISCONNECT     606
#define IDM_TEST_TESTFILE               610
#define IDM_TEST_BEZEL                  611
#define IDM_TEST_SNAP                   612
#define IDM_TEST_SHOWTIPS               613
#define IDM_TEST_NEW_CONNECTION 617
#define IDM_TEST_FLUSH_BACKSCRL 618

#define IDM_TEST_LOAD_EMU               619
#define IDM_TEST_LOAD_AUTO              620
#define IDM_TEST_LOAD_ANSI              621
#define IDM_TEST_LOAD_VT52              622
#define IDM_TEST_LOAD_VT100     623
#define IDM_TEST_LOAD_VIEWDATA  624
#define IDM_TEST_LOAD_MINITEL   625
#define IDM_TEST_LOAD_TTY               626

#define IDM_TEST_SESSNAME               627
#define IDM_TEST_LOAD_VT220     628

/* ---  --- */

#define IDM_CUST_TOOLBAR        700

/* --- Session Context Menu Identifiers --- */

#define IDM_CONTEXT_SEND                800
#define IDM_CONTEXT_RECEIVE             801
#define IDM_CONTEXT_PRINT               802
#define IDM_CONTEXT_COPY                803
#define IDM_CONTEXT_PASTE               804
#define IDM_CONTEXT_SELECT_ALL          805
#define IDM_CONTEXT_WHATS_THIS          806
#define IDM_CONTEXT_SNAP                807
#define IDM_CONTEXT_CLEAR_BACKSCROLL    808
#define IDM_CONTEXT_CLEAR_SCREEN        809

/* --- Identifiers for Minitel toolbar buttons --- */

#define IDM_MINITEL_INDEX                               900
#define IDM_MINITEL_CANCEL                              901
#define IDM_MINITEL_PREVIOUS                    902
#define IDM_MINITEL_REPEAT                              903
#define IDM_MINITEL_GUIDE                               904
#define IDM_MINITEL_CORRECT                     905
#define IDM_MINITEL_NEXT                                906
#define IDM_MINITEL_SEND                                907
#define IDM_MINITEL_CONFIN                              908

/* --- String identifiers --- */

#define IDS_GNRL_APPNAME                100
#define IDS_GNRL_AUTHOR                 101
#define IDS_GNRL_HELPFILE               102
#define IDS_GNRL_PRSHT_TITLE    103
#define IDS_GNRL_NEW_CNCT               104
#define IDS_GNRL_HAS                    105
#define IDS_GNRL_CNFRM_SAVE     106
#define IDS_GNRL_CNFRM_OVER     107
#define IDS_GNRL_CNFRM_DCNCT    108
#define IDS_GNRL_ELLIPSIS               109
#define IDS_GNRL_CREATE_PATH    110
#define IDS_GNRL_INVALID_CHARS  111
#define IDS_GNRL_INVALID_NAME   112
#define IDS_GNRL_PROFILE_DIR    113
//mpt:8-22-97
#if defined(INCL_USE_HTML_HELP)
#define IDS_HTML_HELPFILE               114
#endif
#ifdef INCL_NAG_SCREEN
#define IDS_GNRL_UNREGISTERED   115
#endif

#define IDS_TTT_NEW        130
#define IDS_TTT_OPEN       131
#define IDS_TTT_DIAL       132
#define IDS_TTT_HANGUP     133
#define IDS_TTT_SEND       134
#define IDS_TTT_RECEIVE    135
#define IDS_TTT_PROPERTY   136
/* #define IDS_TTT_HELP           137 */

/* --- String identifiers for Minitel sidebar button strings --- */

#define IDS_SIDEBAR_INDEX          138
#define IDS_SIDEBAR_CANCEL         139
#define IDS_SIDEBAR_PREVIOUS   140
#define IDS_SIDEBAR_REPEAT         141
#define IDS_SIDEBAR_GUIDE          142
#define IDS_SIDEBAR_CORRECT    143
#define IDS_SIDEBAR_NEXT           144
#define IDS_SIDEBAR_MSEND          145
#define IDS_SIDEBAR_CONFIN         146

/* --- End Mintel string ids --- */

#define IDS_CMM_ALL_FILES1 200
#define IDS_CMM_ALL_FILES2 201
#define IDS_CMM_SEL_FILES  202
#define IDS_CMM_SEL_DIR    203

#define IDS_SND_DLG_DD     204
#define IDS_SND_DLG_FILE   205

#define IDS_CMM_SAVE_AS    206
#define IDS_CMM_LOAD_SESS  207
#define IDS_CMM_HAS_FILES1 208
#define IDS_CMM_HAS_FILES2 209
#define IDS_CMM_HAS_FILES3 210
#define IDS_CMM_HAS_FILES4 211
#define IDS_SND_TXT_FILE   212

#define IDS_RM_APPEND      220
#define IDS_RM_OVERWRITE   221
#define IDS_RM_REFUSE      222
#define IDS_RM_NEWER       223
#define IDS_RM_DATE        224
#define IDS_RM_SEQUENCE    225

#define IDS_CPF_DLG_FILE                230
#define IDS_CPF_FILES1                  231
#define IDS_CPF_FILES2                  232
#define IDS_CPF_FILES3                  233
#define IDS_CPF_FILES4                  234
#define IDS_CPF_CAP_OFF                 235
#define IDS_CPF_CAP_ON                  236
#define IDS_CPF_SELECT_FILE             237
#define IDS_CPF_CAP_FILE                238

#define IDS_XD_RECEIVE     240
#define IDS_XD_SEND        241
#define IDS_XD_ON          242
#define IDS_XD_OFF         243
#define IDS_XD_CRC         244
#define IDS_XD_CHECK       245
#define IDS_XD_STREAM      246
#define IDS_XD_CB          247
#define IDS_XD_BP          248
#define IDS_XD_INT         249
#define IDS_XD_I_OF_I      250
#define IDS_XD_KILO        251
#define IDS_XD_K_OF_K      252
#define IDS_XD_BPS         253
#define IDS_XD_CPS         254
#define IDS_XD_PERCENT     255
#define IDS_XD_RECV_TITLE  256
#define IDS_XD_SEND_TITLE  257
#define IDS_XD_ONLY_1      258

#define IDS_XD_PROTO_X_1   270
#define IDS_XD_PROTO_X     271
#define IDS_XD_PROTO_Y     272
#define IDS_XD_PROTO_Y_G   273
#define IDS_XD_PROTO_Z     274
#define IDS_XD_PROTO_K     275
#define IDS_XD_PROTO_Z_CR  276

#define IDS_MB_TITLE_WARN  300
#define IDS_MB_TITLE_ERR   301
#define IDS_MB_MSG_NO_FILE 310

#define IDS_TM_SD_ZERO                  365
#define IDS_TM_SD_ONE                   366
#define IDS_TM_SD_TWO                   367
#define IDS_TM_SD_THREE                 368
#define IDS_TM_SD_FOUR                  369
#define IDS_TM_SD_FIVE                  370
#define IDS_TM_SD_SIX                   371
#define IDS_TM_SD_SEVEN                 372
#define IDS_TM_SD_EIGHT                 373

#define IDS_TM_XFER_ZERO        375
#define IDS_TM_XFER_ONE         376
#define IDS_TM_XFER_TWO         377
#define IDS_TM_XFER_THREE       378
#define IDS_TM_XFER_FOUR        379
#define IDS_TM_XFER_FIVE        380
#define IDS_TM_XFER_SIX         381
#define IDS_TM_XFER_SEVEN       382
#define IDS_TM_XFER_EIGHT       383
#define IDS_TM_XFER_NINE        384
#define IDS_TM_XFER_TEN         385
#define IDS_TM_XFER_ELEVEN      386
#define IDS_TM_XFER_TWELVE      387
#define IDS_TM_XFER_THIRTEEN    388
#define IDS_TM_XFER_FOURTEEN    389
#define IDS_TM_XFER_FIFTEEN     390
#define IDS_TM_XFER_SIXTEEN     391
#define IDS_TM_XFER_SEVENTEEN   392
#define IDS_TM_XFER_EIGHTEEN    393
#define IDS_TM_XFER_NINETEEN    394
#define IDS_TM_XFER_TWENTY      395
#define IDS_TM_XFER_TWENTYONE   396
#define IDS_TM_XFER_TWENTYTWO   397
#define IDS_TM_XFER_TWENTYTHREE 398
#define IDS_TM_XFER_TWENTYFOUR  399

#define IDS_TM_SZ_ZERO          400
#define IDS_TM_SZ_ONE           401
#define IDS_TM_SZ_TWO           402
#define IDS_TM_SZ_THREE         403
#define IDS_TM_SZ_FOUR          404
#define IDS_TM_SZ_FIVE          405
#define IDS_TM_SZ_SIX           406
#define IDS_TM_SZ_SEVEN         407
#define IDS_TM_SZ_EIGHT         408
#define IDS_TM_SZ_NINE          409
#define IDS_TM_SZ_TEN           410
#define IDS_TM_SZ_ELEVEN        411
#define IDS_TM_SZ_TWELVE        412
#define IDS_TM_SZ_THIRTEEN      413
#define IDS_TM_SZ_FOURTEEN      414
#define IDS_TM_SZ_FIFTEEN       415
#define IDS_TM_SZ_SIXTEEN       416
#define IDS_TM_SZ_SEVENTEEN     417
#define IDS_TM_SZ_EIGHTEEN      418
#define IDS_TM_SZ_NINETEEN      419
#define IDS_TM_SZ_TWENTY        420
#define IDS_TM_SZ_TWENTYONE     421
#define IDS_TM_SZ_TWENTYTWO     422
#define IDS_TM_SZ_TWENTYTHREE   423
#define IDS_TM_SZ_TWENTYFOUR    424
#define IDS_TM_SZ_TWENTYFIVE    425
#define IDS_TM_SZ_TWENTYSIX     426

#define IDS_TM_RX_ZERO                  430
#define IDS_TM_RX_ONE                   431
#define IDS_TM_RX_TWO                   432
#define IDS_TM_RX_THREE                 433
#define IDS_TM_RX_FOUR                  434
#define IDS_TM_RX_FIVE                  435
#define IDS_TM_RX_SIX                   436
#define IDS_TM_RX_SEVEN                 437
#define IDS_TM_RX_EIGHT                 438
#define IDS_TM_RX_NINE                  439
#define IDS_TM_RX_TEN                   440
#define IDS_TM_RX_ELEVEN                441
#define IDS_TM_RX_TWELVE                442
#define IDS_TM_RX_THIRTEEN              443
#define IDS_TM_RX_FOURTEEN              444

#define IDS_TM_RS_ZERO                  450
#define IDS_TM_RS_ONE                   451
#define IDS_TM_RS_TWO                   452
#define IDS_TM_RS_THREE                 453
#define IDS_TM_RS_FOUR                  454

#define IDS_TM_RE_ZERO                  455
#define IDS_TM_RE_ONE                   456
#define IDS_TM_RE_TWO                   457
#define IDS_TM_RE_THREE                 458
#define IDS_TM_RE_FOUR                  459
#define IDS_TM_RE_FIVE                  460
#define IDS_TM_RE_SIX                   461
#define IDS_TM_RE_SEVEN                 462
#define IDS_TM_RE_EIGHT                 463
#define IDS_TM_RE_NINE                  464
#define IDS_TM_RE_TEN                   465
#define IDS_TM_RE_ELEVEN                466
#define IDS_TM_RE_TWELVE                467
#define IDS_TM_RE_THIRTEEN              468
#define IDS_TM_RE_FOURTEEN              469
#define IDS_TM_RE_FIFTEEN               470
#define IDS_TM_RE_SIXTEEN               471
#define IDS_TM_RE_SEVENTEEN             472
#define IDS_TM_RE_EIGHTEEN              473

#define IDS_TM_SS_ZERO                  475
#define IDS_TM_SS_ONE                   476
#define IDS_TM_SS_TWO                   477
#define IDS_TM_SS_THREE                 478
#define IDS_TM_SS_FOUR                  479
#define IDS_TM_SS_FIVE                  480

#define IDS_TM_SE_ZERO                  481
#define IDS_TM_SE_ONE                   482
#define IDS_TM_SE_TWO                   483
#define IDS_TM_SE_THREE                 484
#define IDS_TM_SE_FOUR                  485
#define IDS_TM_SE_FIVE                  486
#define IDS_TM_SE_SIX                   487
#define IDS_TM_SE_SEVEN                 488
#define IDS_TM_SE_EIGHT                 489
#define IDS_TM_SE_NINE                  490
#define IDS_TM_SE_TEN                   491
#define IDS_TM_SE_ELEVEN                492
#define IDS_TM_SE_TWELVE                493
#define IDS_TM_SE_THIRTEEN              494

#define IDS_TM_KRM_CANT_OPEN    497
#define IDS_TM_KRM_CANT_WRITE   498
#define IDS_TM_KRM_VIRUS_DETECT 499

#define IDS_TM_K_ZERO                   500
#define IDS_TM_K_ONE                    501
#define IDS_TM_K_TWO                    502
#define IDS_TM_K_THREE                  503
#define IDS_TM_K_FOUR                   504
#define IDS_TM_K_FIVE                   505
#define IDS_TM_K_SIX                    506
#define IDS_TM_K_SEVEN                  507
#define IDS_TM_K_EIGHT                  508
#define IDS_TM_K_NINE                   509

/* --- Statusbar Messages --- */

#define IDS_STATUSBR_CONNECT                    510
#define IDS_STATUSBR_CONNECT_FORMAT     511
#define IDS_STATUSBR_CONNECT_FORMAT_X   512
#define IDS_STATUSBR_DISCONNECT                 513
#define IDS_STATUSBR_CONNECTING                 514
#define IDS_STATUSBR_DISCONNECTING              515
#define IDS_STATUSBR_SCRL                               516
#define IDS_STATUSBR_CAPL                               517
#define IDS_STATUSBR_NUML                               518
#define IDS_STATUSBR_CAPTUREON                  519
#define IDS_STATUSBR_PRINTECHOON                520
#define IDS_STATUSBR_COM                                521
#define IDS_STATUSBR_AUTODETECT                 522
#define IDS_STATUSBR_COM_TCPIP          523
#define IDS_WINSOCK_SETTINGS_STR        524
#define IDS_STATUSBR_ANSWERING          525

/* --- Error Messages --- */

//#define IDS_ER_CNCT_FIND_DLL    600
//#define IDS_ER_CNCT_LOAD_DLL    601
#define IDS_ER_CNCT_BADCNTRYCDE 602
#define IDS_ER_CNCT_BADAREACODE 603
#define IDS_ER_CNCT_BADPHONENUM 604
#define IDS_ER_CNCT_BADLINE     605
#define IDS_ER_CNCT_NOMODEMS    606
#define IDS_ER_CNCT_TAPIFAILED  607
#define IDS_ER_CNCT_TAPINOMEM   608
#define IDS_ER_CNCT_CALLUNAVAIL 609
#define IDS_ER_CNCT_PORTFAILED  610
#define IDS_ER_TAPI_INIFILE     611
#define IDS_ER_TAPI_NODRIVER    612
#define IDS_ER_TAPI_NOMULTI     613
#define IDS_ER_TAPI_CONFIG              614
#define IDS_ER_CNCT_BADADDRESS  615
#define IDS_ER_TAPI_NEEDS_INFO  616
#define IDS_ER_TCPIP_FAILURE    617
#define IDS_ER_TCPIP_BADADDR    618
#define IDS_ER_TCPIP_MISSING_ADDR 619
#define IDS_OPEN_FAILED         620
#define IDS_ER_INVALID_TELNETID 621
#define IDS_ER_TAPI_UNKNOWN     622

#define IDS_PRINT_NOMEM                 702
#define IDS_PRINT_CANCEL                703
#define IDS_PRINT_ERROR                 704
#define IDS_PRINT_NOW_PRINTING  705
#define IDS_PRINT_OF_DOC                706
#define IDS_PRINT_ON_DEV                707
#define IDS_ER_NO_BITMAP                708
#define IDS_ER_TITLE                    709
#define IDS_ER_BAD_SESSION              710
#define IDS_PRINT_NO_PRINTER    711
#define IDS_PRINT_TITLE                 712
#define IDS_ER_REINIT                   713
#define IDS_ER_TAPI_REINIT      714
#define IDS_ER_TAPI_REINIT2     715
#define IDS_PRINT_FILTER_1              716
#define IDS_PRINT_FILTER_2              717
#define IDS_PRINT_FILTER_3              718
#define IDS_PRINT_FILTER_4              719
#define IDS_PRINT_TOFILE                720
#define IDS_PRINT_FILENAME              721
#define IDS_PRINT_CAPTURE_DOC			722

#define IDS_ER_XFER_NO_FILE     800
#define IDS_ER_OPEN_FAILED      801

/* --- Dialing messages --- */

#define IDS_DIAL_OFFERING               900
#define IDS_DIAL_DIALTONE               901
#define IDS_DIAL_DIALING                902
#define IDS_DIAL_RINGBACK               903
#define IDS_DIAL_BUSY                   904
#define IDS_DIAL_CONNECTED              905
#define IDS_DIAL_DISCONNECTED   906
#define IDS_DIAL_NOANSWER               907
#define IDS_DIAL_NODIALTONE     908
#define IDS_DIAL_REDIAL_IN      909

#define IDS_CNCT_OPEN                   910
#define IDS_CNCT_CLOSE                  911
#define IDS_CNCT_DIRECTCOM              912
#define IDS_CNCT_DELAYEDDIAL    913
#define IDS_CNCT_DIRECTCOM_NT   914

#define IDS_TERM_DEF_FONT               920
#define IDS_TERM_DEF_CHARSET    921
#define IDS_TERM_DEF_VGA_SIZE   922
#define IDS_TERM_DEF_NONVGA_SIZE 923
#define IDS_PRINT_DEF_FONT      924
#define IDS_UPGRADE_FONT                925
#define IDS_UPGRADE_FONT_SIZE   926
#define IDS_PRINT_DEF_CHARSET   927

/* Emulator names       */

#define IDS_EMUNAME_BASE		930
#define IDS_EMUNAME_AUTO		930
#define IDS_EMUNAME_ANSI		931
#define IDS_EMUNAME_MINI		932
#define IDS_EMUNAME_VIEW		933
#define IDS_EMUNAME_TTY 		934
#define IDS_EMUNAME_VT100		935
#define IDS_EMUNAME_VT52		936
#define IDS_EMUNAME_VT100J		937
#define IDS_EMUNAME_ANSIW		938
#define IDS_EMUNAME_VT220		939
#define IDS_EMUNAME_VT320		940

/* This is a special flag to make upgrade text localizable */
#define IDS_USE_RTF              999

/* Upgrade text, next 50 id's reserved for upgrade text */

#define IDS_UPGRADE             1000

/* Upgrade button parameters */

#define IDS_UPGRADE_INFO        1051

#ifdef INCL_KEY_MACROS
#define IDS_MACRO_CTRL          1061
#define IDS_MACRO_ALT           1062
#define IDS_MACRO_SHIFT         1063
#define IDS_MACRO_ENTER         1064
#define IDS_MACRO_TAB           1065
#define IDS_MACRO_BS            1066
#define IDS_MACRO_INSERT        1067
#define IDS_MACRO_DEL           1068
#define IDS_MACRO_HOME          1069
#define IDS_MACRO_END           1070
#define IDS_MACRO_UP            1071
#define IDS_MACRO_DOWN          1072
#define IDS_MACRO_PAGEUP        1073
#define IDS_MACRO_PAGEDOWN      1074
#define IDS_MACRO_LEFT          1075
#define IDS_MACRO_RIGHT         1076
#define IDS_MACRO_CENTER        1077
#define IDS_MACRO_F1            1078
#define IDS_MACRO_F2            1079
#define IDS_MACRO_F3            1080
#define IDS_MACRO_F4            1081
#define IDS_MACRO_F5            1082
#define IDS_MACRO_F6            1083
#define IDS_MACRO_F7            1084
#define IDS_MACRO_F8            1085
#define IDS_MACRO_F9            1086
#define IDS_MACRO_F10           1087
#define IDS_MACRO_F11           1088
#define IDS_MACRO_F12           1089
#define IDS_MACRO_F13           1090
#define IDS_MACRO_F14           1091
#define IDS_MACRO_F15           1092
#define IDS_MACRO_F16           1093
#define IDS_MACRO_F17           1094
#define IDS_MACRO_F18           1095
#define IDS_MACRO_F19           1096
#define IDS_MACRO_F20           1097
#define IDS_MACRO_F21           1098
#define IDS_MACRO_F22           1099
#define IDS_MACRO_F23           1100
#define IDS_MACRO_F24           1101
#define IDS_MACRO_BUTTON1       1102
#define IDS_MACRO_BUTTON2       1103
#define IDS_MACRO_BUTTON3       1104
#define IDS_MACRO_BREAK         1105
#define IDS_MACRO_BACKTAB       1106
#define IDS_MACRO_NEWLINE       1107
#define IDS_MACRO_ALTGRAF       1108
#define IDS_MACRO_PAUSE         1109
#define IDS_MACRO_CAPLOCK       1110
#define IDS_MACRO_ESC           1111
#define IDS_MACRO_PRNTSCREEN    1112
#define IDS_MACRO_CLEAR         1113
#define IDS_MACRO_EREOF         1114
#define IDS_MACRO_PA1           1115
#define IDS_MACRO_ADD           1116
#define IDS_MACRO_SUB           1117
#define IDS_MACRO_2             1118
#define IDS_MACRO_6             1119
#define IDS_MACRO_MINUS         1120
#define IDS_MACRO_NUMPAD0       1121
#define IDS_MACRO_NUMPAD1       1122
#define IDS_MACRO_NUMPAD2       1123
#define IDS_MACRO_NUMPAD3       1124
#define IDS_MACRO_NUMPAD4       1125
#define IDS_MACRO_NUMPAD5       1126
#define IDS_MACRO_NUMPAD6       1127
#define IDS_MACRO_NUMPAD7       1128
#define IDS_MACRO_NUMPAD8       1129
#define IDS_MACRO_NUMPAD9       1130
#define IDS_MACRO_NUMPADPERIOD  1131
#define IDS_MACRO_DECIMAL       1132
#define IDS_MACRO_FSLASH        1133
#define IDS_MACRO_MULTIPLY      1134
#define IDS_MACRO_NUMLOCK       1135
#define IDS_MACRO_SCRLLOCK      1136
#define IDS_MACRO_INTERRUPT     1137
#define IDS_MACRO_UNKNOWN       1138
#define IDS_MACRO_CAPSLOCK      1139

#define IDS_DELETE_KEY_MACRO       1140
#define IDS_DUPLICATE_KEY_MACRO    1141
#define IDS_KEY_MACRO_REDEFINITION 1142
#define IDS_MISSING_KEY_MACRO      1143
#endif


/* upper left corner of upgrade button in banner */
#define IDN_UPGRADE_BUTTON_X    102
#define IDN_UPGRADE_BUTTON_Y    228
#define IDN_UPGRADE_BUTTON_W    130
#define IDN_UPGRADE_BUTTON_H    28

/* --- Transfer Tables --- */

#define IDT_CSB_CRC_TABLE       100
#define IDT_CRC_32_TAB          101

/* --- Acclerator Table --- */

#define IDA_WACKER                      100
#define IDA_CONTEXT_MENU        WM_USER+0x300

/* --- Program termination code --- */

#define IDC_MISSING_FILE        1

/* --- Bitmaps used for buttons --- */

#define IDB_BUTTONS_LARGE         3000
#define IDB_BUTTONS_SMALL         3002

/* --- ICONS used for sessions --- */

/* Icons must be numbered in sequence from IDI_PROG because resource
 * compiler is to stupid to do arithmatic.
 */

#define IDI_PROG                        100

#define IDI_PROG1                       101
#define IDI_PROG2                       102
#define IDI_PROG3                       103
#define IDI_PROG4                       104
#define IDI_PROG5                       105
#define IDI_PROG6                       106
#define IDI_PROG7                       107
#define IDI_PROG8                       108
#define IDI_PROG9                       109
#define IDI_PROG10                      110
#define IDI_PROG11                      111
#define IDI_PROG12                      112
#define IDI_PROG13                      113
#define IDI_PROG14                      114
#define IDI_PROG15                      115
#define IDI_PROG16                      116
#define IDI_PROG17                      117
#define IDI_PROG18                      118
#define IDI_PROG19                      119
#define IDI_PROG20                      120
#define IDI_PROG21                      121
#define IDI_PROG22                      122
#define IDI_PROG23                      123
#define IDI_PROG24                      124
#define IDI_PROG25                      125
#define IDI_PROG26                      126
#define IDI_PROG27                      127
#define IDI_PROG28                      128
#define IDI_PROG29                      129
#define IDI_PROG30                      130
#define IDI_PROG31                      131
#define IDI_PROG32                      132
#define IDI_PROG33                      133
#define IDI_PROG34                      134
#define IDI_PROG35                      135
#define IDI_PROG36                      136
#define IDI_PROG37                      137
#define IDI_PROG38                      138
#define IDI_PROG39                      139
#define IDI_PROG40                      140

/* Actually, I didn't know where to put this but it's used with the ICONS */
#define IDI_PROG_ICON_CNT       17

/* --- ID for program banner --- */

#define IDD_BM_BANNER           3001
#define IDR_GLOBE_AVI       4000
