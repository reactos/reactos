//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//      INETREG.H - String literals for HKEYs in registry
//

//      HISTORY:
//      
//      6/21/96     t-gpease            Created.
//  6/24/96     t-gpease        changed from being library into #defines.
//                              turns out that the linker is stupid and
//                              doesn't remove unneeded functs, proc, etc...
//

#ifndef _INETREGSTRS_H_
#define _INETREGSTRS_H_


// 
// HKEY: HKEY_CURRENT_USER
//

//
// Top level defines
//
#define TSZMICROSOFTPATH                  TEXT("Software\\Microsoft")
#define TSZIEPATH        TSZMICROSOFTPATH TEXT("\\Internet Explorer")
#define TSZWINCURVERPATH TSZMICROSOFTPATH TEXT("\\windows\\CurrentVersion")
#define TSZWININETPATH   TSZWINCURVERPATH TEXT("\\Internet Settings")

// Windows : HKLM
#define REGSTR_PATH_RUNONCE_KEY TSZWINCURVERPATH TEXT("\\RunOnce")

// INETCPL : HKLM
#define REGSTR_PATH_INETCPL_PS_EXTENTIONS TSZWINCURVERPATH TEXT("\\Controls Folder\\Internet")

//
// Explorer : HKCU
//
#define REGSTR_PATH_IEXPLORER           TSZIEPATH

// Main
#define SZ_IE_MAIN                      "Main"
#define REGSTR_PATH_MAIN                TSZIEPATH TEXT( "\\") TEXT(SZ_IE_MAIN)
#define REGSTR_KEY_MAIN                 TEXT(SZ_IE_MAIN)

#define REGSTR_VAL_SMOOTHSCROLL         TEXT("SmoothScroll")
#define REGSTR_VAL_SMOOTHSCROLL_DEF     TRUE

#define REGSTR_VAL_SHOWTOOLBAR          TEXT("Show_ToolBar")
#define REGSTR_VAL_SHOWADDRESSBAR       TEXT("Show_URLToolBar")
#define REGSTR_VAL_STARTPAGE            TEXT("Start Page")
#define REGSTR_VAL_SEARCHPAGE           TEXT("Search Page")
#define REGSTR_VAL_LOCALPAGE            TEXT("Local Page")

#define REGSTR_VAL_USESTYLESHEETS       TEXT("Use Stylesheets")
#define REGSTR_VAL_USESTYLESHEETS_TYPE  REG_SZ  // "yes" or "no"
#define REGSTR_VAL_USESTYLESHEETS_DEF   TEXT("yes")

#define REGSTR_VAL_USEICM               TEXT("UseICM")
#define REGSTR_VAL_USEICM_DEF           FALSE

#define REGSTR_VAL_SHOWFOCUS            TEXT("Tabstop - MouseDown")
#define REGSTR_VAL_SHOWFOCUS_TYPE       REG_SZ  // "yes" or "no"
#define REGSTR_VAL_SHOWFOCUS_DEF        TEXT("no")

#define REGSTR_VAL_LOADIMAGES           TEXT("Display Inline Images")
#define REGSTR_VAL_PLAYSOUNDS           TEXT("Play_Background_Sounds")
#define REGSTR_VAL_PLAYVIDEOS           TEXT("Display Inline Videos")
#define REGSTR_VAL_ANCHORUNDERLINE      TEXT("Anchor Underline")
#define REGSTR_VAL_USEDLGCOLORS         TEXT("Use_DlgBox_Colors")
#define REGSTR_VAL_CHECKASSOC           TEXT("Check_Associations")
#define REGSTR_VAL_SHOWFULLURLS         TEXT("Show_FullURL")

// Settings 
#define SZ_IE_SETTINGS  "Settings"
#define REGSTR_PATH_IE_SETTINGS         TSZIEPATH TEXT("\\") TEXT(SZ_IE_SETTINGS)
#define REGSTR_KEY_IE_SETTINGS          TEXT(SZ_IE_SETTINGS)

#define REGSTR_VAL_IE_CUSTOMCOLORS      TEXT("Custom Colors")
#define REGSTR_VAL_IE_CUSTOMCOLORS_TYPE REG_BINARY

#define REGSTR_VAL_ANCHORCOLOR          TEXT("Anchor Color")
#define REGSTR_VAL_ANCHORCOLORVISITED   TEXT("Anchor Color Visited")
#define REGSTR_VAL_BACKGROUNDCOLOR      TEXT("Background Color")
#define REGSTR_VAL_TEXTCOLOR            TEXT("Text Color")

// Security
#define SZ_IE_SECURITY  "Security"
#define REGSTR_PATH_IE_SECURITY         TSZIEPATH TEXT("\\") TEXT(SZ_IE_SECURITY)
#define REGSTR_KEY_IE_SECURITY          TEXT(SZ_IE_SECURITY)

#define REGSTR_VAL_SAFETYWARNINGLEVEL   TEXT("Safety Warning Level")


//
// Internet : HKCU
//
// path to global internet settings (also under HKEY_CURRENT_USER)
#define REGSTR_PATH_INTERNETSETTINGS    TSZWININETPATH
#define REGSTR_PATH_INTERNET_SETTINGS   REGSTR_PATH_INTERNETSETTINGS

// string value under HKCU\REGSTR_PATH_REMOTEACCESS that contains name of
// connectoid used to connect to internet
#define REGSTR_VAL_INTERNETENTRY        TEXT("InternetProfile")
#define REGSTR_VAL_INTERNETPROFILE      REGSTR_VAL_INTERNETENTRY

#define REGSTR_VAL_INTERNETENTRYBKUP    TEXT("BackupInternetProfile")

#define REGSTR_VAL_CODEDOWNLOAD         TEXT("Code Download")
#define REGSTR_VAL_CODEDOWNLOAD_DEF     TEXT("yes")
#define REGSTR_VAL_CODEDOWNLOAD_TYPE    REG_SZ // "yes" or "no"

//
//  Cache
//
#define REGSTR_PATH_CACHE  \
    REGSTR_PATH_INTERNETSETTINGS TEXT("\\Cache")

#define REGSTR_PATH_CACHE_PATHS \
    REGSTR_PATH_CACHE TEXT("\\Paths")



#define REGSTR_PATH_CACHE_SPECIAL_PATHS \
	REGSTR_PATH_CACHE TEXT("Special Paths")

#define REGSTR_VAL_DIRECTORY           TEXT("Directory")
#define REGSTR_VAL_DIRECTORY_TYPE            REG_EXPAND_SZ

#define REGSTR_VAL_NEWDIRECTORY         TEXT("NewDirectory")
#define REGSTR_VAL_NEWDIRECTORY_TYPE    REG_EXPAND_SZ

#define REGSTR_VAL_CACHEPREFIX              TEXT("CachePrefix")
#define REGSTR_VAL_CACHEPREFIX_TYPE     REG_SZ

#define REGSTR_PATH_URLHISTORY \
    REGSTR_PATH_INTERNETSETTINGS TEXT("\\Url History")


//
// Access Medium
//
// access medium (modem, LAN, [etc?])
#define REGSTR_VAL_ACCESSMEDIUM         TEXT("AccessMedium")
// access type (MSN, other)
#define REGSTR_VAL_ACCESSTYPE           TEXT("AccessType")

//
// AutoDial
//
// name of connectoid-specific autodial handler dll and function
#define REGSTR_VAL_AUTODIALDLLNAME      TEXT("AutodialDllName")
#define REGSTR_VAL_AUTODIALFCNNAME      TEXT("AutodialFcnName")
// class name for window to receive Winsock activity messages
#define REGSTR_VAL_AUTODIAL_MONITORCLASSNAME    TEXT("MS_AutodialMonitor")
#define REGSTR_VAL_AUTODIAL_TRYONLYONCE         TEXT("TryAutodialOnce")

//
// Remote Access
//
// path to RNA values (under HKEY_CURRENT_USER)
#define REGSTR_PATH_REMOTEACCESS        TEXT("RemoteAccess")
// BUGBUG (scotth): remove this when inetcpl has fixed its typo
#define REGSTR_PATH_REMOTEACESS         REGSTR_PATH_REMOTEACCESS
// this is under HKLM... we are using this to determine if RNA is installed
// or not. We can't rely on finding the DLL since removing this component
// with the control panel's "Add/Remove Software" does not remove the RNAdll.
#define REGSTR_PATH_RNACOMPONENT    TSZWINCURVERPATH    TEXT("\\Setup\\OptionalComponents\\RNA")
#define REGSTR_VAL_RNAINSTALLED     TEXT("Installed")

// values under HKCU\REGSTR_PATH_INTERNET_SETTINGS

// 4-byte REG_BINARY, autodialing is enabled if this value is present and
// non-zero, disabled otherwise
#define REGSTR_VAL_ENABLEAUTODIAL               TEXT("EnableAutodial")

#define REGSTR_VAL_ENABLEAUTODIALDISCONNECT     TEXT("EnableAutodisconnect")
#define REGSTR_VAL_ENABLEAUTODISCONNECT         REGSTR_VAL_ENABLEAUTODIALDISCONNECT

#define REGSTR_VAL_ENABLESECURITYCHECK          TEXT("EnableSecurityCheck")
// 4-byte REG_BINARY containing number of minutes of idle time to allow
// before autodisconnect.  Autodisconnect is disabled if this value is zero
// or not present.
#define REGSTR_VAL_DISCONNECTIDLETIME   TEXT("DisconnectIdleTime")

//
// MOS
//
#define REGSTR_PATH_MOSDISCONNECT       TSZMICROSOFTPATH TEXT("\\MOS\\Preferences")
#define REGSTR_VAL_MOSDISCONNECT        TEXT("DisconnectTimeout")

//
// Proxy : These are under REGSTR_PATH_INTERNETSETTINGS
//
#define REGSTR_VAL_PROXYENABLE          TEXT("ProxyEnable")
#define REGSTR_VAL_PROXYSERVER          TEXT("ProxyServer")
#define REGSTR_VAL_PROXYOVERRIDE        TEXT("ProxyOverride")



//
// Security : HKCU\\WININETPATH
//
#define SZTRUSTWARNLEVEL                    "Trust Warning Level"
#define REGSTR_KEY_TRUSTWARNINGLEVEL        TSZWININETPATH  TEXT(SZTRUSTWARNLEVEL)
#define REGSTR_VAL_TRUSTWARNINGLEVEL        TEXT(SZTRUSTWARNLEVEL) //"none" will turn off WinVerifyTrust warnings.
#define REGSTR_VAL_TRUSTWARNINGLEVEL_TYPE   REG_SZ  // "none" "norm" or "high"?
#define REGSTR_VAL_TRUSTWARNINGLEVEL_HIGH   TEXT("High")
#define REGSTR_VAL_TRUSTWARNINGLEVEL_MED    TEXT("Medium")
#define REGSTR_VAL_TRUSTWARNINGLEVEL_LOW    TEXT("No Security")
// default depends on MSHTML's prefs nSafetyWarningLevel

#define REGSTR_VAL_SECURITYWARNONSEND       TEXT("WarnOnPost")
#define REGSTR_VAL_SECURITYWARNONSEND_TYPE  REG_BINARY
#define REGSTR_VAL_SECURITYWARNONSEND_DEF   TRUE

#define REGSTR_VAL_SECURITYWARNONSENDALWAYS         TEXT("WarnAlwaysOnPost")
#define REGSTR_VAL_SECURITYWARNONSENDALWAYS_TYPE    REG_BINARY // FALSE-Only if... TRUE-Always
#define REGSTR_VAL_SECURITYWARNONSENDALWAYS_DEF     TRUE

#define REGSTR_VAL_SECURITYWARNONVIEW       TEXT("WarnOnView")
#define REGSTR_VAL_SECURITYWARNONVIEW_TYPE  REG_BINARY
#define REGSTR_VAL_SECURITYWARNONVIEW_DEF   TRUE

#define REGSTR_VAL_SECURITYALLOWCOOKIES         TEXT("AllowCookies")
#define REGSTR_VAL_SECURITYALLOWCOOKIES_TYPE    REG_BINARY
#define REGSTR_VAL_SECURITYALLOWCOOKIES_DEF     TRUE

#define REGSTR_VAL_SECURITYWARNONZONECROSSING       TEXT("WarnOnZoneCrossing")
#define REGSTR_VAL_SECURITYWARNONZONECROSSING_TYPE  REG_BINARY
#define REGSTR_VAL_SECURITYWARNONZONECROSSING_DEF   TRUE

#define REGSTR_VAL_SECURITYWARNONBADCERTVIEWING         TEXT("WarnOnBadCertRecving")
#define REGSTR_VAL_SECURITYWARNONBADCERTVIEWING_TYPE    REG_BINARY
#define REGSTR_VAL_SECURITYWARNONBADCERTVIEWING_DEF     TRUE

#define REGSTR_VAL_SECURITYWARNONBADCERTSENDING         TEXT("WarnOnBadCertSending")
#define REGSTR_VAL_SECURITYWARNONBADCERTSENDING_TYPE    REG_BINARY
#define REGSTR_VAL_SECURITYWARNONBADCERTSENDING_DEF     TRUE

#define REGSTR_VAL_SECURITYDISABLECACHINGOFSSLPAGES       TEXT("DisableCachingOfSSLPages")
#define REGSTR_VAL_SECURITYDISABLECACHINGOFSSLPAGES_TYPE  REG_DWORD
#define REGSTR_VAL_SECURITYDISABLECACHINGOFSSLPAGES_DEF   FALSE


//
// Run/Show ActiveX / Java : These are under REGSTR_PATH_INTERNETSETTINGS
//
#define REGSTR_VAL_SECURITYACTIVEX              TEXT("Security_RunActiveXControls")
#define REGSTR_VAL_SECURITYACTIVEX_TYPE         REG_BINARY  // TRUE or FALSE
#define REGSTR_VAL_SECURITYACTIVEX_DEF          TRUE

#define REGSTR_VAL_SECURITYACTICEXSCRIPTS       TEXT("Security_RunScripts")
#define REGSTR_VAL_SECURITYACTICEXSCRIPTS_TYPE  REG_BINARY  // TRUE or FALSE
#define REGSTR_VAL_SECURITYACTICEXSCRIPTS_DEF   TRUE

#define REGSTR_VAL_SECURITYJAVA                 TEXT("Security_RunJavaApplets")
#define REGSTR_VAL_SECURITYJAVA_TYPE            REG_BINARY  // TRUE or FALSE
#define REGSTR_VAL_SECURITYJAVA_DEF             TRUE

//
// Java VM exclusively : HKCU
//
#define SZJAVAVMPATH                            "\\Java VM"
#define REGSTR_PATH_JAVAVM                      TSZMICROSOFTPATH TEXT(SZJAVAVMPATH)

#define REGSTR_VAL_JAVAJIT                      TEXT("EnableJIT")
#define REGSTR_VAL_JAVAJIT_TYPE                 REG_DWORD   // TRUE or FALSE
#define REGSTR_VAL_JAVAJIT_DEF                  FALSE

#define REGSTR_VAL_JAVALOGGING                   TEXT("EnableLogging")
#define REGSTR_VAL_JAVALOGGING_TYPE              REG_DWORD   // TRUE or FALSE
#define REGSTR_VAL_JAVALOGGING_DEF               FALSE


//
// QuickLinks
//
// this is where custom quicklinks are stored
#define SZTOOLBAR               "\\Toolbar"
#define TSZTOOLBAR              TEXT(SZTOOLBAR)
#define REGSTR_PATH_TOOLBAR     TSZIEPATH TEXT(SZTOOLBAR)
#define REGSTR_KEY_QUICKLINKS   TSZIEPATH TSZTOOLBAR TEXT("\\Links")
#define REGSTR_VAL_DAYSTOKEEP   TEXT("DaysToKeep")

#define SZNOTEXT                "NoText"
#define REGSTR_VAL_NOTEXT       TEXT(SZNOTEXT)
#define REGSTR_KEY_NOTEXT       TSZIEPATH TSZTOOLBAR TEXT("\\") TEXT(SZNOTEXT)
#define SZVISIBLE               "VisibleBands"
#define REGSTR_VAL_VISIBLE      TEXT(SZVISIBLE)
#define REGSTR_KEY_VISIBLE      TSZIEPATH TSZTOOLBAR TEXT("\\") TEXT(SZVISIBLE)


#define REGSTR_VAL_VISIBLEBANDS         TEXT("VisibleBands")
#define REGSTR_VAL_VISIBLEBANDS_TYPE    REG_DWORD   // 3 bits (see below)
#define REGSTR_VAL_VISIBLEBANDS_DEF     0x7         // all three bands
#define TOOLSBAND                       0x1
#define ADDRESSBAND                     0x2
#define LINKSBAND                       0x4

#define SZBACKBITMAP          "BackBitmap"
#define REGSTR_VAL_BACKBITMAP       TEXT("BackBitmap")
#define REGSTR_VAL_BACKBITMAP_TYPE  REG_SZ  
// "" = no bitmap or fillin with valid path, delete for default

#define REGSTR_KEY_BACKBITMAP   TSZIEPATH TSZTOOLBAR TEXT("\\") TEXT(SZBACKBITMAP)

// 
// Schannel Settings: HKLM
//

#define TSZSCHANNELPATH             TEXT("SYSTEM\\CurrentControlSet\\Control\\SecurityProviders\\SCHANNEL")
#define TSZSCHANNELPROTOCOLSPATH    TSZSCHANNELPATH TEXT("\\Protocols")

#define REGSTR_PATH_PCT1            TSZSCHANNELPROTOCOLSPATH TEXT("\\PCT 1.0\\Client")
#define REGSTR_PATH_SSL2            TSZSCHANNELPROTOCOLSPATH TEXT("\\SSL 2.0\\Client")
#define REGSTR_PATH_SSL3            TSZSCHANNELPROTOCOLSPATH TEXT("\\SSL 3.0\\Client")
#define REGSTR_PATH_UNIHELLO        TSZSCHANNELPROTOCOLSPATH TEXT("\\Multi-Protocol Unified Hello\\Client")

#define REGSTR_VAL_SCHANNELENABLEPROTOCOL         TEXT("Enabled")
#define REGSTR_VAL_SCHANNELENABLEPROTOCOL_TYPE    REG_DWORD
#define REGSTR_VAL_SCHANNELENABLEPROTOCOL_DEF     TRUE


//
// Mail and News: HKLM
//
#define TSZINTERNETCLIENTSPATH      TEXT("Software\\Clients")
#define REGSTR_PATH_MAILCLIENTS     TSZINTERNETCLIENTSPATH  TEXT("\\Mail")
#define REGSTR_PATH_NEWSCLIENTS     TSZINTERNETCLIENTSPATH  TEXT("\\News")
// this is under the mail and news paths
#define TSZPROTOCOLSPATH            TEXT("Protocols\\")
// and one these is under the protocols path
#define TSZMAILTOPROTOCOL           TEXT("mailto")
#define TSZNEWSPROTOCOL             TEXT("news")

//
// International and Fonts: HKCU\\TSZIEPATH
//
#define REGSTR_PATH_INTERNATIONAL   TSZIEPATH   TEXT("\\International")

#define REGSTR_VAL_DEFAULT_CODEPAGE         TEXT("Default_CodePage")
#define REGSTR_VAL_DEFAULT_CODEPAGE_TYPE    REG_SZ  // code page
                   // will grab default from system if not found

// each CHARSET has a unique key under REGSTR_PATH_INTERNATIONAL
// which has the following values defined
#define REGSTR_VAL_FONT_SCRIPT           TEXT("Script")
#define REGSTR_VAL_FONT_SCRIPT_TYPE      REG_SZ  // friendly name of font if other than system
                                            // no default

#define REGSTR_VAL_DEF_ENCODING         TEXT("Default_Encoding")
#define REGSTR_VAL_DEF_ENCODING_TYPE    REG_SZ  // internal MIME table name
                                                // no default

#define REGSTR_VAL_FIXED_FONT       TEXT("IEFixedFontName")
#define REGSTR_VAL_FIXED_FONT_TYPE  REG_SZ  // must match a registered font name
                                            // no default

#define REGSTR_VAL_PROP_FONT        TEXT("IEPropFontName")
#define REGSTR_VAL_PROP_FONT_TYPE   REG_SZ  // must match a registered font name
                                            // no default

#define REGSTR_VAL_FONT_SIZE        TEXT("IEFontSize")
#define REGSTR_VAL_FONT_SIZE_TYPE   REG_BINARY
#define REGSTR_VAL_FONT_SIZE_DEF    2       // default size : Medium

#define REGSTR_VAL_AUTODETECT         TEXT("AutoDetect")
#define REGSTR_VAL_AUTODETECT_TYPE    REG_SZ

// MIME database charset extension
#define REGSTR_PATH_MIME_DATABASE            TEXT("MIME\\Database")
#define REGSTR_KEY_MIME_DATABASE_CHARSET     REGSTR_PATH_MIME_DATABASE TEXT("\\Charset")
#define REGSTR_KEY_MIME_DATABASE_CODEPAGE    REGSTR_PATH_MIME_DATABASE TEXT("\\CodePage")

#define REGSTR_VAL_INETENCODING       TEXT("InternetEncoding")
#define REGSTR_VAL_INETENCODING_TYPE  REG_DWORD

#define REGSTR_VAL_ALIASTO            TEXT("AliasForCharset")
#define REGSTR_VAL_ALIASTO_TYPE       REG_SZ

#define REGSTR_VAL_WEBCHARSET         TEXT("WebCharset")
#define REGSTR_VAL_WEBCHARSET_TYPE    REG_SZ

#define REGSTR_VAL_ENCODENAME         TEXT("EncodingName")
#define REGSTR_VAL_ENCODENAME_TYPE    REG_SZ

#define REGSTR_VAL_CODEPAGE           TEXT("CodePage")
#define REGSTR_VAL_CODEPAGE_TYPE      REG_DWORD

#endif // _INETREGSTRS_H_

