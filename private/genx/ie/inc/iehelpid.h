//+-------------------------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//
//  Contents:   Helpids for iexplore.hlp.  iexplore.hlp only contains help for the binaries 
//  distributed with a browser-only install of IE, such as:
//
//      shdocvw.dll
//      browseui.dll
//      inetcpl.cpl
//
//  Pure shell components, such as shell32.dll, use other .hlp files, such as shell.hlp,
//  and have their help ids defined in other headers (see shellids.h and help.h).
//+-------------------------------------------------------------------------------------------

#define IDH_IE_RUN_COMMAND              50000 // was 0x3000

// For Add To Favorites
#define IDH_BROWSELIST                  50001 // was 0x3001
#define IDH_NEWFOLDER                   50002 // was 0x3002
#define IDH_CREATEIN                    50003 // was 0x3003
#define IDH_NAMEEDIT                    50004 // was 0x3004

// For Open dialog
#define IDH_RUNBROWSE                   50005 // was 0x3005

// For Organize Favorites
#define IDH_ORG_FAVORITES_MOVE          50006 // was 0x3050
#define IDH_ORG_FAVORITES_RENAME        50007 // was 0x3051
#define IDH_ORG_FAVORITES_DELETE        50008 // was 0x3052
#define IDH_ORG_FAVORITES_OPEN          50009 // was 0x3053
#define IDH_ORG_FAVORITES_CLOSE         50010 // was 0x3054

//For Volvo QFE
#define IDH_AUTOCONFIG_BUTTON           50011 // was 0x3055
#define IDH_AUTOCONFIG_TEXT             50012 // was 0x3056
#define IDH_OPTS_CONNX_AUTOCNCT_REFRESH 50013 // was 0x3070

//Trident print features
#define IDH_PRINT_SCREEN                50014 // was 0x3057
#define IDH_PRINT_SEL_FRAME             50015 // was 0x3058
#define IDH_PRINT_ALL_FRAME             50016 // was 0x3059
#define IDH_PRINT_LINKS                 50017 // was 0x3060
#define IDH_PRINT_SHORTCUTS             50020 // was 5002



//Find Text dialog box
#define IDH_FIND_WHOLEA                  50022 // was 0x3063
#define IDH_FIND_CASEA                   50023 // was 0x3064
#define IDH_FIND_UPA                     50024 // was 0x3065
#define IDH_FIND_DOWNA                   50025 // was 0x3066
#define IDH_FIND_NEXTA                   50026 // was 0x3067
#define IDH_FIND_WHATA                   50027 // was 0x3068


//General Tab, Language button
#define IDH_OPTS_GEN_LANG_BUT           50028 // was 0x3071

//Language Preferences dialog box
#define IDH_LANG_PREFS_LIST             50029 // was 0x3075 //Language label and list box
#define IDH_LANG_PREFS_UP               50030 // was 0x3076 //Move up button
#define IDH_LANG_PREFS_DOWN             50031 // was 0x3077 //Move Down button
#define IDH_LANG_PREFS_REMOVE           50032 // was 0x3078 //Remove button
#define IDH_LANG_PREFS_ADD              50033 // was 0x3070 //Add button




//Content tab, Personal Information: Personal Profile and Microsoft Wallet
#define IDH_EDIT_PROFILE_BTN            50198 // Edit Profiles button
#define IDH_OPTS_PROG_ADDRMGR_BUT       50035 // was 0x3073
#define IDH_OPTS_PROG_PAYMENTMGR_BUT    50036 // was 0x3074

#define IDH_ADD_LANGUAGE_LANGUAGE_LIST  50037 // was 0x3102 //in Add Languages dialog box, Language list
#define IDH_ADD_LANGUAGE_USER_DEFINED   50038 // was 0x3103  //in Add Languages dialog box, User Defined label and text box

//Client Authentication dialog box
#define  IDH_CLIENT_AUTHENTICATION_LIST 50039 // was 0x3100         //Identification label, text, and list area
#define IDH_CLIENT_AUTHENTICATION_CERT_PROPS    50040  // was 0x3101 //View button

#define IDH_GROUPBOX                    50041 // was 1

///////////////////******************       Advanced tab, Advanced list


//Advanced Tab, Advanced List
//Multimedia
#define IDH_MM_PIC                      50042 // was 3 //Show Pictures
#define IDH_MM_ANIM                     50176 //Play animations
#define IDH_MM_SOUND                    50043 // was 4 //Play Sounds
#define IDH_MM_VIDEO                    50044 // was 5 //play videos
#define IDH_APPEAR_COLOR                50045 // was 6
#define IDH_SMART_DITH                  50177 //Smart dithering
//Browsing
#define IDH_DOWNLOAD_COMP                 50492 //Notify when downloads complete
#define IDH_AUTOCOMP                    50180 //Use AutoComplete
#define IDH_ADD_URL                     50111 // was 79//show friendly urls
#define IDH_ADD_SMOOTH                  50107 // was 75//use smooth scrolling
#define IDH_ADD_LINK                    50112 // was 80//highlight links
#define IDH_NEW_PROCESS                 50178 //Browse in a new process
#define IDH_CHANNELLOG                  50179 //Enable page hit counting
#define IDH_ENABLE_SUB_UPDATES          50181 //Enable scheduled subscription updates
#define IDH_UPDATE_SUB_NEW_PROC         50182 //Update subscriptions in a new process
//Security
#define IDH_PCT_ALLOW                   50130 // was 99//pct 1.0
#define IDH_SSL2_ALLOW                  50128 // was 97//ssl 3.0
#define IDH_SSL3_ALLOW                  50129 // was 98// ssl 3.0
#define IDH_CRYPT_NOSAVE_SSL            50127 // was 96//do not save pages to disk
#define IDH_PRIV_VIEW                   50089 // was 54//warn if changing between secure and unsecure
#define IDH_CHK_CERT_REVOC              50184 // Check for publisher'scertificate revocation
#define IDH_PRIV_INVALID                50119 // was 87//Warn about invalid site certs
#define IDH_TAKE_COOKIES_ALWAYS         50189 //Always accept cookies// Now: Allow cookies that are stored on your computer
#define IDH_PRIV_COOKIE                 50090 // was 55// prompt before accepting cookies //Now: Allow per-session cookies (not stored)
#define IDH_DISABLE_COOKIES             50190 //Disable all cookies// obsolete

//Java VM
#define IDH_ADD_JAVA_COMP               50110 // was 78//JAVA JIT complier enabled
#define IDH_ADD_JAVA_LOG                50109 // was 77//java logging enabled

//Printing
#define IDH_PRINT_BKGRND                50191 //Print background colors and images

//Searching
#define IDH_AUTOSCAN                    50192 //Autoscan common root domains
#define IDH_SEARCH_URL_FAILS            50193 //Search when URL fails; never, always, always

//Toolbar
#define IDH_TOOL_STD                    50048 // was 9 //small icons




#define IDH_RESTORE_DEFS            50196 //Restore Defaults button at bottom of advanced tab

////////////////******       End of Advanced tab **********////////////////////////////

#define IDH_APPEAR_OPTION               50046 // was 7
#define IDH_APPEAR_LINK                 50047 // was 8
#define IDH_FONT_INT                    50055 // was 16
#define IDH_FONT_MIME                   50056 // was 17
#define IDH_OPTS_GEN_FONTS_FONTSIZE     50057 // was 5003
#define IDH_CHAR_SET                    50058 // was 18
#define IDH_INTL_DEFAULT                50059 // was 19
#define IDH_INTL_FONT_PROP              50060 // was 21
#define IDH_INTL_FONT_FIXED             50061 // was 22
#define IDH_DIAL_CON                    50062 // was 24
#define IDH_DIAL_USE                    50063 // connection tab, checkbox: dial the default connection when needed
#define IDH_DIAL_PROP                   50064 // dialup server properties, properties button
#define IDH_DIAL_DIS                    50065 // was 27
#define IDH_DIAL_ADD                    50066 // Add button in connections tab
#define IDH_DIAL_SYS                    50067 // was 29
#define IDH_PROX_SERV                   50068 // proxy server group box, Manual Proxy Server option
#define IDH_PROX_SETTINGS               50069 // LAN Settings button
#define IDH_SERV_INFO                   50070 // was 34
#define IDH_SERV_SAME                   50071 // was 35
#define IDH_EXCEPT_PROX                 50072 // was 37
#define IDH_EXCEPT_LOCAL                50073 // was 38
#define IDH_CUST_ADDRESS                50076 // was 41
#define IDH_CUST_DEF                    50077 // was 42
#define IDH_CUST_CURR                   50078 // was 43
#define IDH_HIST_NUM                    50079 // was 44
#define IDH_HIST_CLEAR                  50080 // was 45
#define IDH_MAIL                        50082 // was 47
#define IDH_NEWS                        50083 // was 48
#define IDH_IE_DEF                      50085 // was 50
#define IDH_CONNECTION_SHARING          50086

#define IDH_CERT_PERS                   50091 // was 56
#define IDH_CERT_SITE                   50092 // was 57
#define IDH_CERT_PUB                    50093 // was 58
#define IDH_ACT_CONTENT                 50097 // was 62
#define IDH_SAFE_EXPERT                 50099 // was 67
#define IDH_SAFE_NORM                   50100 // was 68
#define IDH_SAFE_NONE                   50101 // was 69
#define IDH_RATE_TOGGLE                 50102 // was 70
#define IDH_RATE_PROP                   50103 // was 71
#define IDH_TIF_VIEW                    50104 // was 72
#define IDH_TIF_SETTINGS                50105 // was 73


//Settings dialog
#define IDH_TEMP_EVERY                  50113 // was 81
#define IDH_TEMP_START                  50114 // was 82
#define IDH_TEMP_NEVER                  50115 // was 83
#define IDH_TEMP_AMOUNT                 50116 // was 84
#define IDH_TEMP_EMPTY                  50117 // was 85
#define IDH_TEMP_MOVE                   50118 // was 86
#define IDH_TEMP_AUTO			50491 // New for ie5

#define IDH_VIEW_CERT                   50124 // was 93
#define IDH_DEL_CERT                    50125 // was 94
#define IDH_LIST_CERT                   50126 // was 95



#define IDH_PAGESETUP_HEADER_LEFT       50136 // was      4129       // Page Setup dialog box, header and footer
#define IDH_CPL_GEN_USEBLANK            50137 // was    5100// Use Blank button
#define IDH_CPL_SEC_ZONE_DROPLIST       50138 // was    5110// Zone dropdown list
#define IDH_CPL_SEC_ADDSITES            50139 // was    5115// Add Sites button
#define IDH_CPL_SEC_CUSTOM_LEVEL        50140 // was    5140// Custom for expert users
#define IDH_CPL_SEC_SETTINGS            50141 // was    5135// Settings button
#define IDH_CPL_WEB_SITES_LIST          50142 // was    5145// Trusted Web sites list
#define IDH_CPL_WEB_SITES_REMOVE        50143 // was    5150// Web sites in- Remove button
#define IDH_CPL_REQ_VERIFICATION_CHKBOX 50144 // was    5155// Require server verification for all sites in this zone
#define IDH_CPL_WEB_SITES_ADD_THIS_TXT  50145 // was    5160// Add this web site text box
#define IDH_CPL_WEB_SITES_ADD_BUTTON    50146 // was    5165// Add button
#define IDH_CPL_SEC_SETTINGS_CURRENT    50147 // was    5170// Security: Current settings list box
#define IDH_CPL_SEC_SETTINGS_RESET      50148 // was    5175// Security: Reset to: text box

#define IDH_CPL_CNX_WIZARD              50149 // was    5230// Wizard button in Connection tab/Connection groupbox
#define IDH_CPL_CNX_SETTINGS            50150 // was    5235// Connection settings button
#define IDH_CPL_CNX_ACCESSBYLAN         50151 // was    5240// Access the Internet via a local area network
#define IDH_CPL_CNX_PROXY_ADDR_PORT     50152 // was    5180// Proxy server address and port
#define IDH_CPL_DUN_ATTEMPT_X_TIMES     50153 // was    5200// Dial-Up Settings: Attempt to connect x times
#define IDH_CPL_DUN_WAIT_X_SECS         50154 // was    5205// DUN: Wait x seconds between each attempt
#define IDH_CPL_DUN_SEND_MY_LOGIN       50155 // Do not allow Internet applications to use this connection
#define IDH_CPL_DUN_USERNAME            50156 // was    5215// DUN: User
#define IDH_CPL_DUN_PASSWORD            50157 // was    5220// DUN: Password
#define IDH_CPL_DUN_DOMAIN              50158 // was    5225// DUN: Domain
#define IDH_CPL_PROGRAMS_CAL            50159 // was    5250// Programs: Calendar
#define IDH_CPL_PROGRAMS_CONTACTS       50160 // was    5255// Programs: Contact List
#define IDH_CPL_PROGRAMS_INTERNET_CALL  50161 // was    5260// Programs: Internet Call
#define IDH_CPL_ADV_COLORS              50162 // was    5270// Colors button
#define IDH_CPL_ADV_ACCESSIBILITY       50163 // was    5275// Accessibility button
#define IDH_CPL_ACCESS_USE_MY_COLORS    50164 // was    5290// Accessibility: Always use my color settings
#define IDH_CPL_ACCESS_USE_MY_F_STYLE   50165 // was    5295// Accessibility: Always use my font style settings
#define IDH_CPL_ACCESS_USE_MY_F_SIZE    50166 // was    5300// Accessibility: Always use my font size settings
#define IDH_CPL_ACCESS_USE_MY_STYLESHEETS 50167 // was  5305// Accessibility: User Stylesheet check box and text box

//Security dialog box help
#define IDH_SEC_ENTER_SSL               50168 //Entering a secure site
#define IDH_SEC_ENTER_SSL_W_INVALIDCERT 50169 //Entering a secure site with an invalid certificate
#define IDH_SEC_SEND_N_REC_COOKIES      50171 //Sending and Receiving Information About Your Browsing
#define IDH_SEC_SIGNED_N_INVALID        50173 //Signed and Invalid ActiveX/Java Download
#define IDH_SEC_MIXED_DOWNLOAD_FROM_SSL 50175 //Insecure content download from a secure Web site
#define IDH_SEC_ENTER_NON_SECURE_SITE   50226 //Entering non-secure Web site without a cert, from a secure web site



//CERTIFICATE PROPERTIES DIALOG BOX
#define IDH_CERTVWPROP_GEN_FINEPRINT          50228
#define IDH_CERTVWPROP_DET_ISSUER_CERT        50229
#define IDH_CERTVWPROP_DET_FRIENDLY           50230
#define IDH_CERTVWPROP_DET_STATUS             50231
#define IDH_CERTVWPROP_TRUST_PURPOSE          50232
#define IDH_CERTVWPROP_TRUST_HIERAR           50233
#define IDH_CERTVWPROP_TRUST_VIEWCERT         50234
#define IDH_CERTVWPROP_TRUST_INHERIT          50235
#define IDH_CERTVWPROP_TRUST_EXPLICIT_TRUST   50236
#define IDH_CERTVWPROP_TRUST_EXPLICIT_DISTRUST 50237
#define IDH_CERTVWPROP_ADV_FIELD              50238
#define IDH_CERTVWPROP_ADV_DETAILS            50239

#define IDH_HOVERCOLOR                        50240//color dialog box
#define IDH_JAVA_PERMISSIONS                  50241//security tab, custom settings list

#define IDH_ENABLE_TRANSITIONS                50242//Advanced tab, inetcpl
#define IDH_UNDERLINE_LINKS                   50243//advanced tab, inetcpl
#define IDH_SECURITY_RESET_ZONE_DEFAULTS      50244//security tab, zones
#define IDH_SECURITY_RESET_LEVEL_DEFAULTS     50245//security tab, security settings, Reset custom settings

//Profile Assistant confirmation dialog box
#define IDH_PA_OPS_REQUEST                    50246//name and url of site making request
#define IDH_PA_OPS_LIST                       50247//Profile information requested--label and listbox
#define IDH_PA_USAGE_STRING                   50248//Usage label and text area
#define IDH_PA_VIEW_CERT                      50249//View Certificate button
#define IDH_PA_CONNECTION_SECURITY            50250//Connection Security label and text
#define IDH_PA_ALWAYS_SHARE                   50251//Always share text box

#define IDH_DISABLE_SCRIPT_DEBUG              50252//Advanced tab, intecpl

#define IDH_TEMP_INTERNET_VIEW_OBJECTS_BTN  50254 //Temporary Internet Files settings dialog, View Objects button

//download objects folder properties
#define IDH_DLOAD_TYPE                    50255
#define IDH_DLOAD_CREATED                 50256
#define IDH_DLOAD_LASTACC                 50257
#define IDH_DLOAD_TOTALSIZE               50258
#define IDH_DLOAD_ID                      50259
#define IDH_DLOAD_STATUS                  50260
#define IDH_DLOAD_CODEBASE                50261
#define IDH_DLOAD_FILE_DEP                50262
#define IDH_DLOAD_JAVAPKG_DEP             50263
#define IDH_DLOAD_VERSION                 50264
#define IDH_DLOAD_DESC                    50265
#define IDH_DLOAD_COMPANY                 50266
#define IDH_DLOAD_LANG                    50267
#define IDH_DLOAD_COPYRIGHT               50268

//Local Internet sites
#define IDH_ADD_SITES_ADVANCED_BTN                      50269 
#define IDH_LOCAL_INTRA_INCLUDE_ALL_NOT_LISTED          50270
#define IDH_LOCAL_INTRA_INCLUDE_ALL_THAT_BYPASS_PROXY   50271
#define IDH_LOCAL_INTRA_INCLUDE_ALL_UNCS                50272

#define IDH_SITE_CERTS_ISSUER_TYPE        50273  //Site Certificates dialog box, Issuer Type drop down
 
#define IDH_JAVA_CUST_SETTINGS_BTN        50274  //security custom settings, Java Custom Settings button

//Add 
#define IDH_SUBSCR_TO                     50021 //Favorites full subscription
#define IDH_SUBSCR_TO_CHANNEL             50275 //channel full subscription
#define IDH_SUBSCR_ADDTO_FAVS             50276 //Favorites no subscription
#define IDH_SUBSCR_PARTIAL                50278 //Favorites partial subscription
#define IDH_CHANNEL_ADDTO_CHANNELS        50279 //channels not subscription
#define IDH_CHANNEL_PARTIAL               50280 //channels partial subscription
#define IDH_CHANNEL_SUBSCR_CUST_BUTTON    50281 //Customization button

#define IDH_SOFTWARE_CHANNEL_PERMISSIONS  50282

#define IDH_USER_AUTHENTICATION           50283

#define IDH_DLOAD_OBJNAME                 50284

#define IDH_RESET_SHARING_OPS             50285 //Content tab
#define IDH_ADV_PROFILE_ASSISTANT         50286 //Advanced tab

#define IDH_ALWAYS_LAUNCH_FULL_SCREEN     50287 //Advanced tab, Browser, Always launch fullscreen browser
#define IDH_WARN_IF_FORMS_REDIRECTED      50288 //Advanced tab, Security, Warn if forms submit is being redirected
#define IDH_SHOW_FONT_BUTTON              50289 //Advanced tab, Toolbar, Show font button


//fixes for IE 4.01, New context-sensitive help topics for advanced tab and client authentication personal certificates import and export buttons
#define IDH_SHOW_IE_ON_DESKTOP   50290
#define IDH_SHOW_WELCOME_EACH_LOGON      50291
#define IDH_LAUNCH_CHANNELSIN_FULL             50292
#define IDH_DELETE_HISTORY_WHEN_CLOSING    50293
#define IDH_CLIENTAUTH_IMPORT                          50294
#define IDH_CLIENTAUTH_EXPORT                       50295
#define IDH_USEHTTP11_THRU_PROXY              50296
#define IDH_USEHTTP11                                     50297
#define IDH_ALWAYS_EXPAND_ALT_TEXT    50298
#define IDH_MOVE_SYSTEM_CARET                 50299
#define IDH_CHANNEL_ON_WITHOUT_ACT_DTP     50300
#define IDH_ENABLE_JAVA_CONSOLE             50301

//50302 to 50399 reserved for IEUNIX

#define IDH_MATCH_DIACRITIC 50401 //find dialog box
#define IDH_MATCH_KASHIDA 50402 //find dialog box
#define IDH_MATCH_ALEF_HAMZA 50403 //find dialog box



#define IDH_SUBPROPS_SCHEDTAB_CUSTOM_SCHEDULE 50412
#define IDH_SUBPROPS_SCHEDTAB_MANUAL_SCHEDULE 50414
#define IDH_SUBPROPS_SCHEDTAB_SCHEDDESC 50415
#define IDH_SUBPROPS_SCHED_DONTUPDATE 50417
#define IDH_SUBPROPS_RECTAB_ADVANCED 50420
#define IDH_SUBPROPS_RECTAB_EMAIL_NOTIFICATION 50421
#define IDH_SUBPROPS_RECTAB_MAILOPTS_EMAIL_ADDRESS 50423
#define IDH_SUBPROPS_RECTAB_MAILOPTS_EMAIL_SERVER 50424
#define IDH_SUBPROPS_RECTAB_CHANNEL_LOGIN 50425
#define IDH_SUBPROPS_RECTAB_LOGINOPTS_USER_ID 50426
#define IDH_SUBPROPS_RECTAB_LOGINOPTS_PASSWORD 50427
#define IDH_SUBPROPS_RECTAB_ADVOPTS_DOWNLOAD_PAGES_DEEP 50428
#define IDH_SUBPROPS_RECTAB_ADVOPTS_FOLLOW_LINKS  50429
#define IDH_SUBPROPS_RECTAB_ADVOPTS_DOWNLOAD_IMAGES 50430
#define IDH_SUBPROPS_RECTAB_ADVOPTS_DOWNLOAD_SOUND 50431
#define IDH_SUBPROPS_RECTAB_ADVOPTS_DOWNLOAD_ACTIVEX 50432
#define IDH_SUBPROPS_RECTAB_ADVOPTS_MAX_DOWNLOAD 50433


#define IDH_CONNECTION_TAB_CONNECTOID_LIST  50434 //Dial-up connection list in connection tab
#define IDH_CONNECTION_TAB_REMOVE_CONNECTOID  50435 //Remove button in connection tab
#define IDH_BYPASS_AUTOCFG  50436
#define IDH_DISCONNECT_ON_IEEXIT  50437 //advanced dialup dialog box, checkbox: disconnect when all internet applications exit
#define IDH_USE_THIS_CNX_FOR_INTRANETS  50438
#define IDH_USETHIS_CNX_4_INTERNET  50439
#define IDH_FORTEZZA_ALLOW  50440 

#define IDH_EDIT_OFFLINE_SCHED	50441
#define IDH_MAKE_AVAIL_OFFLINE	50442
#define IDH_NEWSCHED_EVERY_AT_TIME	50443
#define IDH_NEWSCHED_NAME	50444
#define IDH_NEW_OFFLINE_SCHED	50445
#define IDH_REMOVE_OFFLINE_SCHED	50446

#define IDH_CHANNEL_DOWNLOAD_ALL	50447
#define IDH_CHANNEL_DOWNLOAD_COVER_N_TOC	50448
#define IDH_SUBPROPS_SUBTAB_SUBSCRIBED_NAME	50449
#define IDH_SUBPROPS_SUBTAB_SUBSCRIBED_URL	50450
#define IDH_WEBDOC_HOTKEY	50451
#define IDH_WEBDOC_VISITS	50452
#define IDH_SUBPROPS_SUBTAB_LAST	50453
#define IDH_SUBPROPS_DLSIZE	50454
#define IDH_SUBPROPS_SUBTAB_RESULT	50455

//#define IDH_JIT_COMPONENT	50456
#define IDH_JIT_SIZE	50457
#define IDH_JIT_DOWNLOAD_TIME	50458
#define IDH_JIT_NEVER_DOWNLOAD_THESE_COMPONENTS	50459
//#define IDH_JIT_PROGRESS_INDICATOR	50460
#define IDH_JIT_DOWNLOAD_BUTTON	50461
#define IDH_JIT_CANCEL_BUTTON	50462
#define IDH_JIT_VRML	50463
#define IDH_JIT_JAVAVMJIT	50464
#define IDH_JIT_IELPKJA	50465
#define IDH_JIT_IELPKKO	50466
#define IDH_JIT_IELPKPE	50467
#define IDH_JIT_IELPKZHT	50468
#define IDH_JIT_IELPKZHC	50469
#define IDH_JIT_IELPKTH	50470
#define IDH_JIT_IELPKIW	50471
#define IDH_JIT_IELPKVI	50472
#define IDH_JIT_IELPKAR	50473
#define IDH_JIT_IELPKAD	50474
#define IDH_JIT_MEDIAPLAYER	50475
#define IDH_JIT_MEDIAFILTER	50476
#define IDH_JIT_ACTIVEPAYMENT	50477
#define IDH_JIT_AOLSUPP	50478
#define IDH_JIT_MOBILEPK	50479
#define IDH_JIT_ICW	50480
#define IDH_JIT_USP10	50481

//organize favorites new UI
#define IDH_ORGFAVS_UP	50482
#define IDH_ORGFAVS_DOWN	50483
#define IDH_ORGFAVS_NEW_FOLDER	50484
#define IDH_ORGFAVS_SORT	50485
#define IDH_ORGFAVS_SYNCHRO	50486
#define IDH_ORGFAVS_IMPORT	50487
#define IDH_ORGFAVS_EXPORT	50488
#define IDH_ORGFAVS_PROPERTIES	50489
#define IDH_ORGFAVS_LIST	50490
#define IDH_SAVEAS_TYPE	50493 //FileSave, Save As Type
#define IDH_CHAR_SET_SAVE_AS 	50494//FileSave, Language

#define IDH_AUTH_SAVE_PASSWORD     50495 //client authentication login
#define IDH_AUTH_REALM     50496 //client authentication login
#define IDH_AUTH_DOMAIN     50497 //client authentication login
#define IDH_AUTH_SERVER_FIREWALL     50498 //client authentication login
#define IDH_CPL_SEC_SETTINGS_CURRENT_ADMINAPPROVED     50499 //Active X security settings
#define IDH_SUBPROPS_RECTAB_ADVOPTS_ONLY_HTML_LINKS   50500 // Advanced tab, 
#define IDH_ADV_NOTIFY_DWNLD_COMPLETE     50501//Advanced tab: Notify when downloads complete
#define IDH_ADV_DISABLE_JIT     50502//Advanced tab: Enable Install on demand
#define IDH_ADV_DISABLE_CZECH_4NEWER_IE     50503//Advanced tab: Automatically check for Internet Explorer updates

//New JIT topics:
#define IDH_JIT_DIRECTANIMATION   50504
#define IDH_JIT_DIRECTOR   50505
#define IDH_JIT_HELPCONT   50506
#define IDH_JIT_MSN_AUTH   50507
#define IDH_JIT_TRIDATA   50508
#define IDH_JIT_EXTRAPACK   50509
#define IDH_JIT_OK_BUTTON   50510

#define IDH_TLS_PROTOCOL   50511 //Advanced tab, Transport Layer Security protocol checkbox

//New language ids, Yutakan
#define IDH_COMBO_UILANG   50512 // The dropdown listbox on the Menus and Dialogs dialog.
#define IDH_LANG_ADDSPK    50513 // Add satellite pack button: This button leads to the satellite pack downloading web site.
#define IDH_LANG_UI_PREF   50514 //the Change button on the Language Preference dialog, which invokes the Menus and Dialogs dialog.

//New Security tab ids, Lorenk
#define IDH_SEC_LVL_SLIDER 50515 //hook up to the description bullets text also
#define IDH_SEC_ZONE_LIST 50516  //the list of icons at the top of the security tab.

//Content tab,
#define IDH_OPTS_PROG_AUTOSUGGEST_BUT 50517 //Autosuggest button
#define IDH_INTELLIFORM 50524 //Autosuggest forms option//See 50180 for autosuggest address options
#define IDH_CLEAR_INTELLIFORM  50525 //Autosuggest forms clear history button

#define IDH_OPTS_PROG_WALLET_BUT 50518  //Wallet button

//Proxy server settings
#define IDH_PROX_SERV_AUTO  50519 //In proxy server group box (dialup settings) or Lan settings proxy server dialog box: Automatic discovery of proxy server
#define IDH_PROX_SERV_NONE  50520 //In proxy server group box (dialup settings) or Lan settings proxy server dialog box: No proxy server
#define IDH_PROX_SETTINGS_ADV 50521 ////In proxy server group box (dialup settings) or Lan settings proxy server dialog box: Advanced button
#define IDH_DIAL_PROP_ADV  50522 //In dial-up settings group box, Advanced button (for connection attempts and disconnecting settings

//Connections tab

#define IDH_DIAL_DEFAULT  50523 //Set Default button, makes selected connection your default


//advanced tab
#define IDH_ADV_ENABLE_SCRIPTERROR_NOTIFICATION 50526
#define IDH_ADV_ENABLE_SYNC_OF_OFFLINEITEMS_PER_SCHED 50527
#define IDH_ADV_CLOSE_UNUSED_FOLDERS 50528
#define IDH_AUTODISC_DET_ONCE 50529
#define IDH_AUTODISC_DET_DISABLE 50530
#define IDH_AUTODISC_DET_AFTER_NET_CHG 50531
#define IDH_AUTODISC_DET_FOR_ANY_SETTINGS_CHGS 50532

//web folders
#define IDH_WEB_FOLDERS_CKBOX   50533  //in FileOpen dialog box

//Programs tab
#define IDH_HTML_EDITOR 50534 //HTML Editor default

#define IDH_SENDURLS_AS_UTF8 50535 //Send URLS as UTF-8, Advanced tab

#define IDH_SHOW_GO_IN_ADDRESSBAR  50536

#define IDH_SHOW_FRIENDLY_HTTP_ERROR_MESSAGES  50537
#define IDH_INLINE_AUTOCOMP_WEB_ADDRESSES 50538
#define IDH_INLINE_AUTOCOMP_PATHS_N_SHARES 50539
#define IDH_INTELLIFORM_PW 50540
#define IDH_CLEAR_INTELLIFORM_PW 50541
#define IDH_CONNECT_TAB_PERFORM_SECUR_CHECKB4_DIALING 50542

#define IDH_ADD_COMP_RADIO 50543
#define IDH_REPAIR_CURRENT_INSTALLATION 50544
#define IDH_RESTORE_PREVIOUS_IE 50545
#define IDH_MAINT_ADV_BUT 50546
#define IDH_RESTORE_COMPONENT_LIST 50547
#define IDH_DELETE_BACKUP 50548
#define IDH_REMOVE_IE5_SETUP_FILES 50549

#define IDH_CHK_SITE_CERT_REVOC 50550 //Check for server certificate revocation

#define IDH_BROWSEUI_TB_TEXTOPTNS                           50551
#define IDH_BROWSEUI_TB_ICONOPTNS                           50552

#define IDH_JIT_FLASH     50553
#define IDH_JIT_WEBFLDRS     50554
#define IDH_JIT_MESSNGR    50555
#define IDH_JIT_VBSCRIPT    50556
#define IDH_JIT_VML     50557
#define IDH_JIT_WAB    50558

#define IDH_SHOW_PLACEHOLDERS     50559
#define IDH_WEBFTP_ON    50560
#define IDH_CNX_CURRENT_DEFAULT_LBL_N_DISP   50561

#define IDH_INTELLIFORM_PW_PROMPT   50562

//Outlook Express Fonts dialog box
#define IDH_INTL_DEFAULT_OE   50563
#define IDH_CHAR_SET_OE   50564
#define IDH_INTL_FONT_PROP_OE   50565
#define IDH_INTL_FONT_FIXED_OE   50566
#define IDH_FONT_MIME_OE   50567

//New connection radio buttons
#define IDH_NEVERDIAL   50568
#define IDH_DIALIF_NETCNX_GONE   50569

//New Autosearch options in advanced tab
#define IDH_ADDBAR_SRCH_GOTOBEST   50570
#define IDH_ADDBAR_SRCH_RESULTS_ONLY   50571
#define IDH_ADDBAR_DONT_SRCH   50572
#define IDH_ADDBAR_DISP_RESULTS_WHERE   50573

#define IDH_RESET_WEBSTGS_BUTTON 50574

//More Search Settings dialog box
#define IDH_MORESRCH_AVAIL_PROVIDERS  50575 // Available providers list
#define IDH_MORESRCH_PREFERREDPROVIDERS  50576 //Preferred providers list
#define IDH_MORESRCH_ADD_PROVID  50577 //Add button
#define IDH_MORESRCH_REMOVE_PROVID  50578 //Remove button
#define IDH_MORESRCH_PREFRD_MOVEUP  50579  //Move up button
#define IDH_MORESRCH_PREFRD_MOVEDN  50580//Move down button
#define IDH_MORESRCH_RESET  50581 //Reset button

//last number added 50581, start new ids with 50582


