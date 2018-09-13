//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1996-1997               **
//*********************************************************************

//;begin_internal
/***********************************************************************************************

  This is a distributed SDK component - do not put any #includes or other directives that rely
  upon files not dropped. If in doubt - build iedev

  If you add comments please include either ;BUGBUG at the beginning of a single line OR
  enclose in a ;begin_internal, ;end_internal block - such as this one!

 ***********************************************************************************************/
//;end_internal

//;begin_internal
#ifndef __COREDISP_H__
#define __COREDISP_H__
//;end_internal

//;begin_internal
//
// The following dispid must be the smallest possible dispid so that it
// always ends up first in our attr array.
// It does not need to be exposed to the outside world
#define DISPID_AAHEADER                 MINLONG             // DISPID is 0x80000000
#define DISPID_RECALC_INFO              MINLONG+1
//;end_internal


#define DISPID_XOBJ_MIN                 0x80010000
#define DISPID_XOBJ_MAX                 0x8001FFFF
#define DISPID_XOBJ_BASE                DISPID_XOBJ_MIN
#define DISPID_HTMLOBJECT               (DISPID_XOBJ_BASE   + 500)
#define DISPID_ELEMENT                  (DISPID_HTMLOBJECT  + 500)
#define DISPID_SITE                     (DISPID_ELEMENT     + 1000)
#define DISPID_OBJECT                   (DISPID_SITE        + 1000)
#define DISPID_STYLE                    (DISPID_OBJECT      + 1000)
#define DISPID_ATTRS                    (DISPID_STYLE       + 1000)
#define DISPID_EVENTS                   (DISPID_ATTRS       + 1000)
#define DISPID_XOBJ_EXPANDO             (DISPID_EVENTS      + 1000)
#define DISPID_XOBJ_ORDINAL             (DISPID_XOBJ_EXPANDO+ 1000)

//;begin_internal
// Expandos for ActiveX controls, note these are very limited compared to
// normal expandos on an element.

#define DISPID_ACTIVEX_EXPANDO_BASE      DISPID_XOBJ_EXPANDO
#define DISPID_ACTIVEX_EXPANDO_MAX       (DISPID_ACTIVEX_EXPANDO_BASE + 999)

#define DISPID_OBJECT_ORDINAL_BASE       DISPID_XOBJ_ORDINAL
#define DISPID_OBJECT_ORDINAL_MAX       (DISPID_OBJECT_ORDINAL_BASE + 999)

#define DISPID_COLLECTION_MIN           1000000
#define DISPID_COLLECTION_MAX           2999999

// Divide collection dispid space into "named member" half and "ordinal access" half
// for stylesheets collection.
#define DISPID_STYLESHEETSCOLLECTION_NAMED_BASE        (DISPID_COLLECTION_MIN)
#define DISPID_STYLESHEETSCOLLECTION_NAMED_MAX         (DISPID_COLLECTION_MIN+((DISPID_COLLECTION_MAX-DISPID_COLLECTION_MIN)/2))
#define DISPID_STYLESHEETSCOLLECTION_ORDINAL_BASE      (DISPID_STYLESHEETSCOLLECTION_NAMED_MAX+1)
#define DISPID_STYLESHEETSCOLLECTION_ORDINAL_MAX       (DISPID_COLLECTION_MAX)

// DISPID range for expandos not associated with an ActiveX control
#define DISPID_EXPANDO_BASE             3000000
#define DISPID_EXPANDO_MAX              3999999

#define IsStandardDispid(dispid)        (dispid <= 0)
#define IsExpandoDispid(dispid)         (DISPID_EXPANDO_BASE <= dispid && dispid <= DISPID_EXPANDO_MAX)

#define DISPID_EVENTHOOK_SENSITIVE_BASE   4000000
#define DISPID_EVENTHOOK_SENSITIVE_MAX    4499999
#define DISPID_EVENTHOOK_INSENSITIVE_BASE 4500000
#define DISPID_EVENTHOOK_INSENSITIVE_MAX  4999999

#define DISPID_PEER_HOLDER_BASE         5000000

#define IsPeerDispid(dispid)            (DISPID_PEER_HOLDER_BASE <= dispid)

//;end_internal

//;begin_internal
//
// IE 4 dispids that no longer exist
//
//;end_internal
#define DISPID_HTMLOPTIONBUTTONELEMENTEVENTS_ONCHANGE       DISPID_HTMLINPUTTEXTELEMENTEVENTS_ONCHANGE

//;begin_internal
//
// Standard control properties
//
//;end_internal

//;BUGBUG: rgardner - why do we use these names ???
#define DISPID_CommonCtrl_FONTNAME        1
#define DISPID_CommonCtrl_FONTSIZE        2
#define DISPID_CommonCtrl_FONTBOLD        3
#define DISPID_CommonCtrl_FONTITAL        4
#define DISPID_CommonCtrl_FONTUNDER       5
#define DISPID_CommonCtrl_FONTSTRIKE      6
#define DISPID_CommonCtrl_FONTWEIGHT      7
#define DISPID_CommonCtrl_FONTCHARSET     8
#define DISPID_CommonCtrl_FONTSUPERSCRIPT 9
#define DISPID_CommonCtrl_FONTSUBSCRIPT   10

// Data Binding DISPID's
#define DISPID_MSDATASRCINTERFACE       (-3900)
#define DISPID_ADVISEDATASRCCHANGEEVENT (-3901)


//;begin_internal
// DISPID values for HTML Dialogs files per interface
//;end_internal

#define DISPID_HTMLDLG                          25000
#define DISPID_HTMLDLGMODEL                     26000

//;begin_internal
// DISPID values for HTML Application files per interface
//;end_internal

#define DISPID_HTMLAPP                          5000

//;begin_internal
//----------------------------------------------------------------------------
//
//  Semi-standard x-object properties.
//
//  These values match those used by VB and are for the benefit of controls
//  with hard coded knowledge of VB.
//
//----------------------------------------------------------------------------
//;end_internal

#define STDPROPID_XOBJ_NAME                 (DISPID_XOBJ_BASE + 0x0)
#define STDPROPID_XOBJ_INDEX                (DISPID_XOBJ_BASE + 0x1)
//;begin_internal
// for IE3 compatibility

#define STDPROPID_IE3XOBJ_OBJECTALIGN     (DISPID_XOBJ_BASE + 0x1) 

// STDPROPID_XOBJ_BASEHREF is a constant used by IE3
//;end_internal
#define STDPROPID_XOBJ_BASEHREF             (DISPID_XOBJ_BASE + 0x2) 
#define STDPROPID_XOBJ_LEFT                 (DISPID_XOBJ_BASE + 0x3)
#define STDPROPID_XOBJ_TOP                  (DISPID_XOBJ_BASE + 0x4)
#define STDPROPID_XOBJ_WIDTH                (DISPID_XOBJ_BASE + 0x5)
#define STDPROPID_XOBJ_HEIGHT               (DISPID_XOBJ_BASE + 0x6)
#define STDPROPID_XOBJ_VISIBLE              (DISPID_XOBJ_BASE + 0x7)
#define STDPROPID_XOBJ_PARENT               (DISPID_XOBJ_BASE + 0x8)
#define STDPROPID_XOBJ_DRAGMODE             (DISPID_XOBJ_BASE + 0x9)
#define STDPROPID_XOBJ_DRAGICON             (DISPID_XOBJ_BASE + 0xA)
#define STDPROPID_XOBJ_TAG                  (DISPID_XOBJ_BASE + 0xB)
#define STDPROPID_XOBJ_TABSTOP              (DISPID_XOBJ_BASE + 0xE)
#define STDPROPID_XOBJ_TABINDEX             (DISPID_XOBJ_BASE + 0xF)
#define STDPROPID_XOBJ_HELPCONTEXTID        (DISPID_XOBJ_BASE + 0x32)
#define STDPROPID_XOBJ_DEFAULT              (DISPID_XOBJ_BASE + 0x37)
#define STDPROPID_XOBJ_CANCEL               (DISPID_XOBJ_BASE + 0x38)
#define STDPROPID_XOBJ_LEFTNORUN            (DISPID_XOBJ_BASE + 0x39)
#define STDPROPID_XOBJ_TOPNORUN             (DISPID_XOBJ_BASE + 0x3A)
#define STDPROPID_XOBJ_ALIGNPERSIST         (DISPID_XOBJ_BASE + 0x3C)
#define STDPROPID_XOBJ_LINKTIMEOUT          (DISPID_XOBJ_BASE + 0x3D)
#define STDPROPID_XOBJ_LINKTOPIC            (DISPID_XOBJ_BASE + 0x3E)
#define STDPROPID_XOBJ_LINKITEM             (DISPID_XOBJ_BASE + 0x3F)
#define STDPROPID_XOBJ_LINKMODE             (DISPID_XOBJ_BASE + 0x40)
#define STDPROPID_XOBJ_DATACHANGED          (DISPID_XOBJ_BASE + 0x41)
#define STDPROPID_XOBJ_DATAFIELD            (DISPID_XOBJ_BASE + 0x42)
#define STDPROPID_XOBJ_DATASOURCE           (DISPID_XOBJ_BASE + 0x43)
#define STDPROPID_XOBJ_WHATSTHISHELPID      (DISPID_XOBJ_BASE + 0x44)
#define STDPROPID_XOBJ_CONTROLTIPTEXT       (DISPID_XOBJ_BASE + 0x45)
#define STDPROPID_XOBJ_STATUSBARTEXT        (DISPID_XOBJ_BASE + 0x46)
#define STDPROPID_XOBJ_APPLICATION          (DISPID_XOBJ_BASE + 0x47)
#define STDPROPID_XOBJ_BLOCKALIGN           (DISPID_XOBJ_BASE + 0x48)
#define STDPROPID_XOBJ_CONTROLALIGN         (DISPID_XOBJ_BASE + 0x49)
#define STDPROPID_XOBJ_STYLE                (DISPID_XOBJ_BASE + 0x4A)
#define STDPROPID_XOBJ_COUNT                (DISPID_XOBJ_BASE + 0x4B)
#define STDPROPID_XOBJ_DISABLED             (DISPID_XOBJ_BASE + 0x4C)
#define STDPROPID_XOBJ_RIGHT                (DISPID_XOBJ_BASE + 0x4D)
#define STDPROPID_XOBJ_BOTTOM               (DISPID_XOBJ_BASE + 0x4E)

//;begin_internal
//----------------------------------------------------------------------------
//
//  Semi-standard x-object properties.
//
//  These are events that are fired for all sites
//----------------------------------------------------------------------------
//;end_internal

#define STDDISPID_XOBJ_ONBLUR                           (DISPID_XOBJ_BASE)
#define STDDISPID_XOBJ_ONFOCUS                          (DISPID_XOBJ_BASE + 1)
#define STDDISPID_XOBJ_BEFOREUPDATE                     (DISPID_XOBJ_BASE + 4)
#define STDDISPID_XOBJ_AFTERUPDATE                      (DISPID_XOBJ_BASE + 5)
#define STDDISPID_XOBJ_ONROWEXIT                        (DISPID_XOBJ_BASE + 6)
#define STDDISPID_XOBJ_ONROWENTER                       (DISPID_XOBJ_BASE + 7)
#define STDDISPID_XOBJ_ONMOUSEOVER                      (DISPID_XOBJ_BASE + 8)
#define STDDISPID_XOBJ_ONMOUSEOUT                       (DISPID_XOBJ_BASE + 9)
#define STDDISPID_XOBJ_ONHELP                           (DISPID_XOBJ_BASE + 10)
#define STDDISPID_XOBJ_ONDRAGSTART                      (DISPID_XOBJ_BASE + 11)
#define STDDISPID_XOBJ_ONSELECTSTART                    (DISPID_XOBJ_BASE + 12)
#define STDDISPID_XOBJ_ERRORUPDATE                      (DISPID_XOBJ_BASE + 13)
#define STDDISPID_XOBJ_ONDATASETCHANGED                 (DISPID_XOBJ_BASE + 14)
#define STDDISPID_XOBJ_ONDATAAVAILABLE                  (DISPID_XOBJ_BASE + 15)
#define STDDISPID_XOBJ_ONDATASETCOMPLETE                (DISPID_XOBJ_BASE + 16)
#define STDDISPID_XOBJ_ONFILTER                         (DISPID_XOBJ_BASE + 17)
#define STDDISPID_XOBJ_ONLOSECAPTURE                    (DISPID_XOBJ_BASE + 18)
#define STDDISPID_XOBJ_ONPROPERTYCHANGE                 (DISPID_XOBJ_BASE + 19)
#define STDDISPID_XOBJ_ONDRAG                           (DISPID_XOBJ_BASE + 20)
#define STDDISPID_XOBJ_ONDRAGEND                        (DISPID_XOBJ_BASE + 21)
#define STDDISPID_XOBJ_ONDRAGENTER                      (DISPID_XOBJ_BASE + 22)
#define STDDISPID_XOBJ_ONDRAGOVER                       (DISPID_XOBJ_BASE + 23)
#define STDDISPID_XOBJ_ONDRAGLEAVE                      (DISPID_XOBJ_BASE + 24)
#define STDDISPID_XOBJ_ONDROP                           (DISPID_XOBJ_BASE + 25)
#define STDDISPID_XOBJ_ONCUT                            (DISPID_XOBJ_BASE + 26)
#define STDDISPID_XOBJ_ONCOPY                           (DISPID_XOBJ_BASE + 27)
#define STDDISPID_XOBJ_ONPASTE                          (DISPID_XOBJ_BASE + 28)
#define STDDISPID_XOBJ_ONBEFORECUT                      (DISPID_XOBJ_BASE + 29)
#define STDDISPID_XOBJ_ONBEFORECOPY                     (DISPID_XOBJ_BASE + 30)
#define STDDISPID_XOBJ_ONBEFOREPASTE                    (DISPID_XOBJ_BASE + 31)
#define STDDISPID_XOBJ_ONROWSDELETE                     (DISPID_XOBJ_BASE + 32)
#define STDDISPID_XOBJ_ONROWSINSERTED                   (DISPID_XOBJ_BASE + 33)
#define STDDISPID_XOBJ_ONCELLCHANGE                     (DISPID_XOBJ_BASE + 34)

//;begin_internal
//----------------------------------------------------------------------------
//
//  Base DISPIDs for each class.
//
//  Object and its base classes must use ids in the reserved x-object range.
//
//----------------------------------------------------------------------------
//;end_internal

#define DISPID_NORMAL_FIRST                     1000
#define DISPID_ANCHOR                           DISPID_NORMAL_FIRST
#define DISPID_BLOCK                            DISPID_NORMAL_FIRST
#define DISPID_BODY                             (DISPID_TEXTSITE + 1000)
#define DISPID_BR                               DISPID_NORMAL_FIRST
#define DISPID_BGSOUND                          DISPID_NORMAL_FIRST
#define DISPID_DD                               DISPID_NORMAL_FIRST
#define DISPID_DIR                              DISPID_NORMAL_FIRST
#define DISPID_DIV                              DISPID_NORMAL_FIRST
#define DISPID_DL                               DISPID_NORMAL_FIRST
#define DISPID_DT                               DISPID_NORMAL_FIRST
#define DISPID_EFONT                            DISPID_NORMAL_FIRST
#define DISPID_FORM                             DISPID_NORMAL_FIRST
#define DISPID_HEADER                           DISPID_NORMAL_FIRST
#define DISPID_HEDELEMS                         DISPID_NORMAL_FIRST
#define DISPID_HR                               DISPID_NORMAL_FIRST
#define DISPID_LABEL                            DISPID_NORMAL_FIRST
#define DISPID_LI                               DISPID_NORMAL_FIRST
#define DISPID_IMGBASE                          DISPID_NORMAL_FIRST
#define DISPID_IMG                              (DISPID_IMGBASE + 1000)
#define DISPID_INPUTIMAGE                       (DISPID_IMGBASE + 1000)
#define DISPID_INPUT                            (DISPID_TEXTSITE + 1000)
#define DISPID_INPUTTEXTBASE                    (DISPID_INPUT+1000)
#define DISPID_INPUTTEXT                        (DISPID_INPUTTEXTBASE+1000)
#define DISPID_MENU                             DISPID_NORMAL_FIRST
#define DISPID_OL                               DISPID_NORMAL_FIRST
#define DISPID_PARA                             DISPID_NORMAL_FIRST
#define DISPID_SELECT                           DISPID_NORMAL_FIRST
#define DISPID_SELECTOBJ                        DISPID_NORMAL_FIRST
#define DISPID_TABLE                            DISPID_NORMAL_FIRST
#define DISPID_TEXTSITE                         DISPID_NORMAL_FIRST
#define DISPID_TEXTAREA                         (DISPID_INPUTTEXT + 1000)
#define DISPID_MARQUEE                          (DISPID_TEXTAREA + 1000)
#define DISPID_RICHTEXT                         (DISPID_MARQUEE + 1000)
#define DISPID_BUTTON                           (DISPID_RICHTEXT + 1000)
#define DISPID_UL                               DISPID_NORMAL_FIRST
#define DISPID_PHRASE                           DISPID_NORMAL_FIRST
#define DISPID_UNKNOWNPDL                       DISPID_NORMAL_FIRST
#define DISPID_COMMENTPDL                       DISPID_NORMAL_FIRST
#define DISPID_TABLECELL                        (DISPID_TEXTSITE + 1000)
#define DISPID_RANGE                            DISPID_NORMAL_FIRST
#define DISPID_SELECTION                        DISPID_NORMAL_FIRST
#define DISPID_OPTION                           DISPID_NORMAL_FIRST
#define DISPID_1D                               (DISPID_TEXTSITE + 1000)
#define DISPID_MAP                              DISPID_NORMAL_FIRST
#define DISPID_AREA                             DISPID_NORMAL_FIRST
#define DISPID_PARAM                            DISPID_NORMAL_FIRST
#define DISPID_TABLESECTION                     DISPID_NORMAL_FIRST
#define DISPID_TABLEROW                         DISPID_NORMAL_FIRST
#define DISPID_TABLECOL                         DISPID_NORMAL_FIRST
#define DISPID_SCRIPT                           DISPID_NORMAL_FIRST
#define DISPID_STYLESHEET                       DISPID_NORMAL_FIRST
#define DISPID_STYLERULE                        DISPID_NORMAL_FIRST
#define DISPID_STYLESHEETS_COL                  DISPID_NORMAL_FIRST
#define DISPID_STYLERULES_COL                   DISPID_NORMAL_FIRST
#define DISPID_MIMETYPES_COL                    DISPID_NORMAL_FIRST
#define DISPID_PLUGINS_COL                      DISPID_NORMAL_FIRST
#define DISPID_2D                               DISPID_NORMAL_FIRST
#define DISPID_OMWINDOW                         DISPID_NORMAL_FIRST
#define DISPID_EVENTOBJ                         DISPID_NORMAL_FIRST
#define DISPID_PERSISTDATA                      DISPID_NORMAL_FIRST
#define DISPID_OLESITE                          DISPID_NORMAL_FIRST
#define DISPID_FRAMESET                         DISPID_NORMAL_FIRST
#define DISPID_LINK                             DISPID_NORMAL_FIRST
#define DISPID_STYLEELEMENT                     DISPID_NORMAL_FIRST
#define DISPID_FILTERS                          DISPID_NORMAL_FIRST
#define DISPID_TABLESECTION                     DISPID_NORMAL_FIRST
#define DISPID_OMRECT                           DISPID_NORMAL_FIRST
#define DISPID_DOMATTRIBUTE                     DISPID_NORMAL_FIRST
#define DISPID_DOMTEXTNODE                      DISPID_NORMAL_FIRST
#define DISPID_GENERIC                          DISPID_NORMAL_FIRST
#define DISPID_URN_COLL                         DISPID_NORMAL_FIRST

#define DISPID_HTMLDOCUMENT                     DISPID_NORMAL_FIRST
#define DISPID_OMDOCUMENT                       DISPID_NORMAL_FIRST
#define DISPID_DATATRANSFER                     DISPID_NORMAL_FIRST
#define DISPID_XMLDECL                          DISPID_NORMAL_FIRST
#define DISPID_DOCFRAG                          DISPID_NORMAL_FIRST
//;begin_internal
    // Special case for compatability with IE4 -> therefore the 1:
//;end_internal
#define DISPID_WINDOW                           1
#define DISPID_SCREEN                           DISPID_NORMAL_FIRST
#define DISPID_HISTORY                          1
#define DISPID_LOCATION                         1
#define DISPID_NAVIGATOR                        1
#define DISPID_COLLECTION                       (DISPID_NORMAL_FIRST+500)
#define DISPID_OPTIONS_COL                      (DISPID_NORMAL_FIRST+500)

#define DISPID_CHECKBOX                         DISPID_NORMAL_FIRST
#define DISPID_RADIO                            (DISPID_CHECKBOX + 1000)

#define DISPID_FRAMESITE                        (DISPID_SITE        + 1000)
#define DISPID_FRAME                            (DISPID_FRAMESITE   + 1000)
#define DISPID_IFRAME                           (DISPID_FRAMESITE   + 1000)


//;begin_internal
//----------------------------------------------------------------------------
//
//  Reserved negative DISPIDs
//
//----------------------------------------------------------------------------
//;end_internal

#define DISPID_WINDOWOBJECT                     (-5500)
#define DISPID_LOCATIONOBJECT                   (-5506)
#define DISPID_HISTORYOBJECT                    (-5507)
#define DISPID_NAVIGATOROBJECT                  (-5508)
#define DISPID_SECURITYCTX                      (-5511)
#define DISPID_AMBIENT_DLCONTROL                (-5512)
#define DISPID_AMBIENT_USERAGENT                (-5513)
#define DISPID_SECURITYDOMAIN                   (-5514)
#define DLCTL_DLIMAGES                          0x00000010
#define DLCTL_VIDEOS                            0x00000020
#define DLCTL_BGSOUNDS                          0x00000040
#define DLCTL_NO_SCRIPTS                        0x00000080
#define DLCTL_NO_JAVA                           0x00000100
#define DLCTL_NO_RUNACTIVEXCTLS                 0x00000200
#define DLCTL_NO_DLACTIVEXCTLS                  0x00000400
#define DLCTL_DOWNLOADONLY                      0x00000800
#define DLCTL_NO_FRAMEDOWNLOAD                  0x00001000
#define DLCTL_RESYNCHRONIZE                     0x00002000
#define DLCTL_PRAGMA_NO_CACHE                   0x00004000
#define DLCTL_NO_BEHAVIORS                   	0x00008000
#define DLCTL_NO_METACHARSET                    0x00010000
#define DLCTL_URL_ENCODING_DISABLE_UTF8         0x00020000
#define DLCTL_URL_ENCODING_ENABLE_UTF8          0x00040000
#define DLCTL_FORCEOFFLINE                      0x10000000
#define DLCTL_NO_CLIENTPULL                     0x20000000
#define DLCTL_SILENT                            0x40000000
#define DLCTL_OFFLINEIFNOTCONNECTED             0x80000000
#define DLCTL_OFFLINE                           DLCTL_OFFLINEIFNOTCONNECTED

//;begin_internal
//----------------------------------------------------------------------------
//
//  DISPID for each non xobject event
//
//----------------------------------------------------------------------------
//;end_internal

#define DISPID_ONABORT                          (DISPID_NORMAL_FIRST)
#define DISPID_ONCHANGE                         (DISPID_NORMAL_FIRST + 1)
#define DISPID_ONERROR                          (DISPID_NORMAL_FIRST + 2)
#define DISPID_ONLOAD                           (DISPID_NORMAL_FIRST + 3)
#define DISPID_ONSELECT                         (DISPID_NORMAL_FIRST + 6)
#define DISPID_ONSUBMIT                         (DISPID_NORMAL_FIRST + 7)
#define DISPID_ONUNLOAD                         (DISPID_NORMAL_FIRST + 8)
#define DISPID_ONBOUNCE                         (DISPID_NORMAL_FIRST + 9)
#define DISPID_ONFINISH                         (DISPID_NORMAL_FIRST + 10)
#define DISPID_ONSTART                          (DISPID_NORMAL_FIRST + 11)
#define DISPID_ONLAYOUT                         (DISPID_NORMAL_FIRST + 13)
#define DISPID_ONSCROLL                         (DISPID_NORMAL_FIRST + 14)
#define DISPID_ONRESET                          (DISPID_NORMAL_FIRST + 15)
#define DISPID_ONRESIZE                         (DISPID_NORMAL_FIRST + 16)
#define DISPID_ONBEFOREUNLOAD                   (DISPID_NORMAL_FIRST + 17)
#define DISPID_ONCHANGEFOCUS                    (DISPID_NORMAL_FIRST + 18)
#define DISPID_ONCHANGEBLUR                     (DISPID_NORMAL_FIRST + 19)
#define DISPID_ONPERSIST                        (DISPID_NORMAL_FIRST + 20)
#define DISPID_ONPERSISTSAVE                    (DISPID_NORMAL_FIRST + 21)
#define DISPID_ONPERSISTLOAD                    (DISPID_NORMAL_FIRST + 22)
#define DISPID_ONCONTEXTMENU                    (DISPID_NORMAL_FIRST + 23)
#define DISPID_ONBEFOREPRINT                    (DISPID_NORMAL_FIRST + 24)
#define DISPID_ONAFTERPRINT                     (DISPID_NORMAL_FIRST + 25)
#define DISPID_ONSTOP                           (DISPID_NORMAL_FIRST + 26)
#define DISPID_ONBEFOREEDITFOCUS                (DISPID_NORMAL_FIRST + 27)
#define DISPID_ONMOUSEHOVER                     (DISPID_NORMAL_FIRST + 28)

//;begin_internal
//----------------------------------------------------------------------------
//
//  DISPID for each unique HtmlAttribute/CssAttribute
//
//----------------------------------------------------------------------------
//;end_internal

#define DISPID_A_FIRST                          DISPID_ATTRS
#define DISPID_A_MIN                            DISPID_ATTRS
#define DISPID_A_MAX                            (DISPID_ATTRS+999)

#define DISPID_A_BACKGROUNDIMAGE                (DISPID_A_FIRST+1)
#define DISPID_A_COLOR                          (DISPID_A_FIRST+2)
#define DISPID_A_TEXTTRANSFORM                  (DISPID_A_FIRST+4)
#define DISPID_A_NOWRAP                         (DISPID_A_FIRST+5)
#define DISPID_A_LINEHEIGHT                     (DISPID_A_FIRST+6)
#define DISPID_A_TEXTINDENT                     (DISPID_A_FIRST+7)
#define DISPID_A_LETTERSPACING                  (DISPID_A_FIRST+8)
#define DISPID_A_LANG                           (DISPID_A_FIRST+9)
#define DISPID_A_OVERFLOW                       (DISPID_A_FIRST+10)

#define DISPID_A_PADDING                        (DISPID_A_FIRST+11)
#define DISPID_A_PADDINGTOP                     (DISPID_A_FIRST+12)
#define DISPID_A_PADDINGRIGHT                   (DISPID_A_FIRST+13)
#define DISPID_A_PADDINGBOTTOM                  (DISPID_A_FIRST+14)
#define DISPID_A_PADDINGLEFT                    (DISPID_A_FIRST+15)

#define DISPID_A_CLEAR                          (DISPID_A_FIRST+16)
#define DISPID_A_LISTTYPE                       (DISPID_A_FIRST+17)
#define DISPID_A_FONTFACE                       (DISPID_A_FIRST+18)
#define DISPID_A_FONTSIZE                       (DISPID_A_FIRST+19)

#define DISPID_A_TEXTDECORATIONLINETHROUGH      (DISPID_A_FIRST+20)
#define DISPID_A_TEXTDECORATIONUNDERLINE        (DISPID_A_FIRST+21)
#define DISPID_A_TEXTDECORATIONBLINK            (DISPID_A_FIRST+22)
#define DISPID_A_TEXTDECORATIONNONE             (DISPID_A_FIRST+23)


#define DISPID_A_FONTSTYLE                      (DISPID_A_FIRST+24)
#define DISPID_A_FONTVARIANT                    (DISPID_A_FIRST+25)
#define DISPID_A_BASEFONT                       (DISPID_A_FIRST+26)
#define DISPID_A_FONTWEIGHT                     (DISPID_A_FIRST+27)

#define DISPID_A_TABLEBORDERCOLOR               (DISPID_A_FIRST+28)
#define DISPID_A_TABLEBORDERCOLORLIGHT          (DISPID_A_FIRST+29)
#define DISPID_A_TABLEBORDERCOLORDARK           (DISPID_A_FIRST+30)
#define DISPID_A_TABLEVALIGN                    (DISPID_A_FIRST+31)

#define DISPID_A_BACKGROUND                     (DISPID_A_FIRST+32)
#define DISPID_A_BACKGROUNDPOSX                 (DISPID_A_FIRST+33)
#define DISPID_A_BACKGROUNDPOSY                 (DISPID_A_FIRST+34)

#define DISPID_A_TEXTDECORATION                 (DISPID_A_FIRST+35)

#define DISPID_A_MARGIN                         (DISPID_A_FIRST+36)
#define DISPID_A_MARGINTOP                      (DISPID_A_FIRST+37)
#define DISPID_A_MARGINRIGHT                    (DISPID_A_FIRST+38)
#define DISPID_A_MARGINBOTTOM                   (DISPID_A_FIRST+39)
#define DISPID_A_MARGINLEFT                     (DISPID_A_FIRST+40)

#define DISPID_A_FONT                           (DISPID_A_FIRST+41)
#define DISPID_A_FONTSIZEKEYWORD                (DISPID_A_FIRST+42)
#define DISPID_A_FONTSIZECOMBINE                (DISPID_A_FIRST+43)

#define DISPID_A_BACKGROUNDREPEAT               (DISPID_A_FIRST+44)
#define DISPID_A_BACKGROUNDATTACHMENT           (DISPID_A_FIRST+45)
#define DISPID_A_BACKGROUNDPOSITION             (DISPID_A_FIRST+46)
#define DISPID_A_WORDSPACING                    (DISPID_A_FIRST+47)
#define DISPID_A_VERTICALALIGN                  (DISPID_A_FIRST+48)
#define DISPID_A_BORDER                         (DISPID_A_FIRST+49)
#define DISPID_A_BORDERTOP                      (DISPID_A_FIRST+50)
#define DISPID_A_BORDERRIGHT                    (DISPID_A_FIRST+51)
#define DISPID_A_BORDERBOTTOM                   (DISPID_A_FIRST+52)
#define DISPID_A_BORDERLEFT                     (DISPID_A_FIRST+53)
#define DISPID_A_BORDERCOLOR                    (DISPID_A_FIRST+54)
#define DISPID_A_BORDERTOPCOLOR                 (DISPID_A_FIRST+55)
#define DISPID_A_BORDERRIGHTCOLOR               (DISPID_A_FIRST+56)
#define DISPID_A_BORDERBOTTOMCOLOR              (DISPID_A_FIRST+57)
#define DISPID_A_BORDERLEFTCOLOR                (DISPID_A_FIRST+58)
#define DISPID_A_BORDERWIDTH                    (DISPID_A_FIRST+59)
#define DISPID_A_BORDERTOPWIDTH                 (DISPID_A_FIRST+60)
#define DISPID_A_BORDERRIGHTWIDTH               (DISPID_A_FIRST+61)
#define DISPID_A_BORDERBOTTOMWIDTH              (DISPID_A_FIRST+62)
#define DISPID_A_BORDERLEFTWIDTH                (DISPID_A_FIRST+63)
#define DISPID_A_BORDERSTYLE                    (DISPID_A_FIRST+64)
#define DISPID_A_BORDERTOPSTYLE                 (DISPID_A_FIRST+65)
#define DISPID_A_BORDERRIGHTSTYLE               (DISPID_A_FIRST+66)
#define DISPID_A_BORDERBOTTOMSTYLE              (DISPID_A_FIRST+67)
#define DISPID_A_BORDERLEFTSTYLE                (DISPID_A_FIRST+68)
#define DISPID_A_TEXTDECORATIONOVERLINE         (DISPID_A_FIRST+69)
#define DISPID_A_FLOAT                          (DISPID_A_FIRST+70)
#define DISPID_A_DISPLAY                        (DISPID_A_FIRST+71)
#define DISPID_A_LISTSTYLETYPE                  (DISPID_A_FIRST+72)
#define DISPID_A_LISTSTYLEPOSITION              (DISPID_A_FIRST+73)
#define DISPID_A_LISTSTYLEIMAGE                 (DISPID_A_FIRST+74)
#define DISPID_A_LISTSTYLE                      (DISPID_A_FIRST+75)
#define DISPID_A_WHITESPACE                     (DISPID_A_FIRST+76)
#define DISPID_A_PAGEBREAKBEFORE                (DISPID_A_FIRST+77)
#define DISPID_A_PAGEBREAKAFTER                 (DISPID_A_FIRST+78)
#define DISPID_A_SCROLL                         (DISPID_A_FIRST+79)
#define DISPID_A_VISIBILITY                     (DISPID_A_FIRST+80)
//;begin_internal
// This dispid is available
#define DISPID_A_HIDDEN                         (DISPID_A_FIRST+81)
//;end_internal
#define DISPID_A_FILTER                         (DISPID_A_FIRST+82)

#define DISPID_DEFAULTVALUE                     (DISPID_A_FIRST+83)

#define DISPID_A_BORDERCOLLAPSE                 (DISPID_A_FIRST+84)

#define DISPID_A_POSITION                       (DISPID_A_FIRST+90)
#define DISPID_A_ZINDEX                         (DISPID_A_FIRST+91)
#define DISPID_A_CLIP                           (DISPID_A_FIRST+92)
#define DISPID_A_CLIPRECTTOP                    (DISPID_A_FIRST+93)
#define DISPID_A_CLIPRECTRIGHT                  (DISPID_A_FIRST+94)
#define DISPID_A_CLIPRECTBOTTOM                 (DISPID_A_FIRST+95)
#define DISPID_A_CLIPRECTLEFT                   (DISPID_A_FIRST+96)

#define DISPID_A_FONTFACESRC                    (DISPID_A_FIRST+97)
#define DISPID_A_TABLELAYOUT                    (DISPID_A_FIRST+98)

//;begin_internal
// The style as a text string
//;end_internal
#define DISPID_A_STYLETEXT                      (DISPID_A_FIRST+99)

//;begin_internal
// Known attributes that have special meaning
//;end_internal
#define DISPID_A_LANGUAGE                       (DISPID_A_FIRST+100)

#define DISPID_A_VALUE                          (DISPID_A_FIRST+101)
#define DISPID_A_CURSOR                         (DISPID_A_FIRST+102)


//;begin_internal
//+-----------------------------------------------------------------------
//  A couple of dispids that are used internally for firing
//  events and prop notifies.
// Keep all the internal dispid's together, otherwise we'll trip up 

#define DISPID_A_EVENTSINK                      (DISPID_A_FIRST+103)
#define DISPID_A_PROPNOTIFYSINK                 (DISPID_A_FIRST+104)
#define DISPID_A_ROWSETNOTIFYSINK               (DISPID_A_FIRST+105)
#define DISPID_INTERNAL_INLINESTYLEAA           (DISPID_A_FIRST+106) // In line style Attr Array
#define DISPID_INTERNAL_CSTYLEPTRCACHE          (DISPID_A_FIRST+107) // Cached CStyle Ptr
#define DISPID_INTERNAL_CRUNTIMESTYLEPTRCACHE	(DISPID_A_FIRST+108) // runtime style ptr obj
#define DISPID_INTERNAL_INVOKECONTEXT           (DISPID_A_FIRST+109) // Cached Invoke context

#define DISPID_A_BGURLIMGCTXCACHEINDEX          (DISPID_A_FIRST+110)
#define DISPID_A_LIURLIMGCTXCACHEINDEX          (DISPID_A_FIRST+111)
#define DISPID_A_ROWSETASYNCHNOTIFYSINK         (DISPID_A_FIRST+112)
#define DISPID_INTERNAL_FILTERPTRCACHE          (DISPID_A_FIRST+113) // FilterCollection in AttrArray
#define DISPID_A_ROWPOSITIONCHANGESINK          (DISPID_A_FIRST+114)
#define DISPID_A_BEHAVIOR                       (DISPID_A_FIRST+115) // xtags
#define DISPID_A_READYSTATE                     (DISPID_A_FIRST+116) // ready state
//;end_internal

#define DISPID_A_DIR                            (DISPID_A_FIRST+117) // Complex Text support for bidi
#define DISPID_A_UNICODEBIDI                    (DISPID_A_FIRST+118) // Complex Text support for CSS2 unicode-bidi
#define DISPID_A_DIRECTION                      (DISPID_A_FIRST+119) // Complex Text support for CSS2 direction

#define DISPID_A_IMEMODE                        (DISPID_A_FIRST+120) 

#define DISPID_A_RUBYALIGN                      (DISPID_A_FIRST+121)
#define DISPID_A_RUBYPOSITION                   (DISPID_A_FIRST+122)
#define DISPID_A_RUBYOVERHANG                   (DISPID_A_FIRST+123)

//;begin_internal
#define DISPID_INTERNAL_ONBEHAVIOR_CONTENTREADY  (DISPID_A_FIRST+124)
#define DISPID_INTERNAL_ONBEHAVIOR_DOCUMENTREADY (DISPID_A_FIRST+125)
#define DISPID_INTERNAL_CDOMCHILDRENPTRCACHE     (DISPID_A_FIRST+126)
//;end_internal

#define DISPID_A_LAYOUTGRIDCHAR                 (DISPID_A_FIRST+127)
#define DISPID_A_LAYOUTGRIDLINE                 (DISPID_A_FIRST+128)
#define DISPID_A_LAYOUTGRIDMODE                 (DISPID_A_FIRST+129)
#define DISPID_A_LAYOUTGRIDTYPE                 (DISPID_A_FIRST+130)
#define DISPID_A_LAYOUTGRID                     (DISPID_A_FIRST+131)

#define DISPID_A_TEXTAUTOSPACE                  (DISPID_A_FIRST+132)

#define DISPID_A_LINEBREAK                      (DISPID_A_FIRST+133)
#define DISPID_A_WORDBREAK                      (DISPID_A_FIRST+134)

#define DISPID_A_TEXTJUSTIFY                    (DISPID_A_FIRST+135)
#define DISPID_A_TEXTJUSTIFYTRIM                (DISPID_A_FIRST+136)
#define DISPID_A_TEXTKASHIDA                    (DISPID_A_FIRST+137)

#define DISPID_A_OVERFLOWX                      (DISPID_A_FIRST+139)
#define DISPID_A_OVERFLOWY                      (DISPID_A_FIRST+140)

#define DISPID_A_HTCDISPATCHITEM_VALUE          (DISPID_A_FIRST+141)
#define DISPID_A_DOCFRAGMENT                    (DISPID_A_FIRST+142)

#define DISPID_A_HTCDD_ELEMENT                  (DISPID_A_FIRST+143)
#define DISPID_A_HTCDD_CREATEEVENTOBJECT        (DISPID_A_FIRST+144)

#define DISPID_A_URNATOM                        (DISPID_A_FIRST+145)
#define DISPID_A_UNIQUEPEERNUMBER               (DISPID_A_FIRST+146)

#define DISPID_A_ACCELERATOR                    (DISPID_A_FIRST+147)

//;begin_internal
#define DISPID_INTERNAL_ONBEHAVIOR_APPLYSTYLE       (DISPID_A_FIRST+148)
#define DISPID_INTERNAL_RUNTIMESTYLEAA              (DISPID_A_FIRST+149)
#define DISPID_A_HTCDISPATCHITEM_VALUE_SCRIPTSONLY  (DISPID_A_FIRST+150)
//;end_internal

//;begin_internal
//------------------------------------------------------------------------
//
//  Event property and method dispids
//
//------------------------------------------------------------------------
//;end_internal

#define DISPID_EVPROP_ONMOUSEOVER           (DISPID_EVENTS +  0)
#define DISPID_EVMETH_ONMOUSEOVER            STDDISPID_XOBJ_ONMOUSEOVER
#define DISPID_EVPROP_ONMOUSEOUT            (DISPID_EVENTS +  1)
#define DISPID_EVMETH_ONMOUSEOUT             STDDISPID_XOBJ_ONMOUSEOUT
#define DISPID_EVPROP_ONMOUSEDOWN           (DISPID_EVENTS +  2)
#define DISPID_EVMETH_ONMOUSEDOWN            DISPID_MOUSEDOWN
#define DISPID_EVPROP_ONMOUSEUP             (DISPID_EVENTS +  3)
#define DISPID_EVMETH_ONMOUSEUP              DISPID_MOUSEUP
#define DISPID_EVPROP_ONMOUSEMOVE           (DISPID_EVENTS +  4)
#define DISPID_EVMETH_ONMOUSEMOVE            DISPID_MOUSEMOVE
#define DISPID_EVPROP_ONKEYDOWN             (DISPID_EVENTS +  5)
#define DISPID_EVMETH_ONKEYDOWN              DISPID_KEYDOWN
#define DISPID_EVPROP_ONKEYUP               (DISPID_EVENTS +  6)
#define DISPID_EVMETH_ONKEYUP                DISPID_KEYUP
#define DISPID_EVPROP_ONKEYPRESS            (DISPID_EVENTS +  7)
#define DISPID_EVMETH_ONKEYPRESS             DISPID_KEYPRESS
#define DISPID_EVPROP_ONCLICK               (DISPID_EVENTS +  8)
#define DISPID_EVMETH_ONCLICK                DISPID_CLICK
#define DISPID_EVPROP_ONDBLCLICK            (DISPID_EVENTS +  9)
#define DISPID_EVMETH_ONDBLCLICK             DISPID_DBLCLICK
#define DISPID_EVPROP_ONSELECT              (DISPID_EVENTS + 10)
#define DISPID_EVMETH_ONSELECT               DISPID_ONSELECT
#define DISPID_EVPROP_ONSUBMIT              (DISPID_EVENTS + 11)
#define DISPID_EVMETH_ONSUBMIT               DISPID_ONSUBMIT
#define DISPID_EVPROP_ONRESET               (DISPID_EVENTS + 12)
#define DISPID_EVMETH_ONRESET                DISPID_ONRESET
#define DISPID_EVPROP_ONHELP                (DISPID_EVENTS + 13)
#define DISPID_EVMETH_ONHELP                 STDDISPID_XOBJ_ONHELP
#define DISPID_EVPROP_ONFOCUS               (DISPID_EVENTS + 14)
#define DISPID_EVMETH_ONFOCUS                STDDISPID_XOBJ_ONFOCUS
#define DISPID_EVPROP_ONBLUR                (DISPID_EVENTS + 15)
#define DISPID_EVMETH_ONBLUR                 STDDISPID_XOBJ_ONBLUR
#define DISPID_EVPROP_ONROWEXIT             (DISPID_EVENTS + 18)
#define DISPID_EVMETH_ONROWEXIT              STDDISPID_XOBJ_ONROWEXIT
#define DISPID_EVPROP_ONROWENTER            (DISPID_EVENTS + 19)
#define DISPID_EVMETH_ONROWENTER             STDDISPID_XOBJ_ONROWENTER
#define DISPID_EVPROP_ONBOUNCE              (DISPID_EVENTS + 20)
#define DISPID_EVMETH_ONBOUNCE               DISPID_ONBOUNCE
#define DISPID_EVPROP_ONBEFOREUPDATE        (DISPID_EVENTS + 21)
#define DISPID_EVMETH_ONBEFOREUPDATE         STDDISPID_XOBJ_BEFOREUPDATE
#define DISPID_EVPROP_ONAFTERUPDATE         (DISPID_EVENTS + 22)
#define DISPID_EVMETH_ONAFTERUPDATE          STDDISPID_XOBJ_AFTERUPDATE
#define DISPID_EVPROP_ONBEFOREDRAGOVER      (DISPID_EVENTS + 23)
#define DISPID_EVMETH_ONBEFOREDRAGOVER       EVENTID_CommonCtrlEvent_BeforeDragOver
#define DISPID_EVPROP_ONBEFOREDROPORPASTE   (DISPID_EVENTS + 24)
#define DISPID_EVMETH_ONBEFOREDROPORPASTE    EVENTID_CommonCtrlEvent_BeforeDropOrPaste
#define DISPID_EVPROP_ONREADYSTATECHANGE    (DISPID_EVENTS + 25)
#define DISPID_EVMETH_ONREADYSTATECHANGE     DISPID_READYSTATECHANGE
#define DISPID_EVPROP_ONFINISH              (DISPID_EVENTS + 26)
#define DISPID_EVMETH_ONFINISH               DISPID_ONFINISH
#define DISPID_EVPROP_ONSTART               (DISPID_EVENTS + 27)
#define DISPID_EVMETH_ONSTART                DISPID_ONSTART
#define DISPID_EVPROP_ONABORT               (DISPID_EVENTS + 28)
#define DISPID_EVMETH_ONABORT                DISPID_ONABORT
#define DISPID_EVPROP_ONERROR               (DISPID_EVENTS + 29)
#define DISPID_EVMETH_ONERROR                DISPID_ONERROR
#define DISPID_EVPROP_ONCHANGE              (DISPID_EVENTS + 30)
#define DISPID_EVMETH_ONCHANGE               DISPID_ONCHANGE
#define DISPID_EVPROP_ONSCROLL              (DISPID_EVENTS + 31)
#define DISPID_EVMETH_ONSCROLL               DISPID_ONSCROLL
#define DISPID_EVPROP_ONLOAD                (DISPID_EVENTS + 32)
#define DISPID_EVMETH_ONLOAD                 DISPID_ONLOAD
#define DISPID_EVPROP_ONUNLOAD              (DISPID_EVENTS + 33)
#define DISPID_EVMETH_ONUNLOAD               DISPID_ONUNLOAD
#define DISPID_EVPROP_ONLAYOUT              (DISPID_EVENTS + 34)
#define DISPID_EVMETH_ONLAYOUT               DISPID_ONLAYOUT
#define DISPID_EVPROP_ONDRAGSTART           (DISPID_EVENTS + 35)
#define DISPID_EVMETH_ONDRAGSTART            STDDISPID_XOBJ_ONDRAGSTART
#define DISPID_EVPROP_ONRESIZE              (DISPID_EVENTS + 36)
#define DISPID_EVMETH_ONRESIZE               DISPID_ONRESIZE
#define DISPID_EVPROP_ONSELECTSTART         (DISPID_EVENTS + 37)
#define DISPID_EVMETH_ONSELECTSTART          STDDISPID_XOBJ_ONSELECTSTART
#define DISPID_EVPROP_ONERRORUPDATE         (DISPID_EVENTS + 38)
#define DISPID_EVMETH_ONERRORUPDATE          STDDISPID_XOBJ_ERRORUPDATE
#define DISPID_EVPROP_ONBEFOREUNLOAD        (DISPID_EVENTS + 39)
#define DISPID_EVMETH_ONBEFOREUNLOAD         DISPID_ONBEFOREUNLOAD
#define DISPID_EVPROP_ONDATASETCHANGED      (DISPID_EVENTS + 40)
#define DISPID_EVMETH_ONDATASETCHANGED       STDDISPID_XOBJ_ONDATASETCHANGED
#define DISPID_EVPROP_ONDATAAVAILABLE       (DISPID_EVENTS + 41)
#define DISPID_EVMETH_ONDATAAVAILABLE        STDDISPID_XOBJ_ONDATAAVAILABLE
#define DISPID_EVPROP_ONDATASETCOMPLETE     (DISPID_EVENTS + 42)
#define DISPID_EVMETH_ONDATASETCOMPLETE      STDDISPID_XOBJ_ONDATASETCOMPLETE
#define DISPID_EVPROP_ONFILTER              (DISPID_EVENTS + 43)
#define DISPID_EVMETH_ONFILTER               STDDISPID_XOBJ_ONFILTER
#define DISPID_EVPROP_ONCHANGEFOCUS         (DISPID_EVENTS + 44)
#define DISPID_EVMETH_ONCHANGEFOCUS          DISPID_ONCHANGEFOCUS
#define DISPID_EVPROP_ONCHANGEBLUR          (DISPID_EVENTS + 45)
#define DISPID_EVMETH_ONCHANGEBLUR           DISPID_ONCHANGEBLUR
#define DISPID_EVPROP_ONLOSECAPTURE         (DISPID_EVENTS + 46)
#define DISPID_EVMETH_ONLOSECAPTURE          STDDISPID_XOBJ_ONLOSECAPTURE
#define DISPID_EVPROP_ONPROPERTYCHANGE      (DISPID_EVENTS + 47)
#define DISPID_EVMETH_ONPROPERTYCHANGE       STDDISPID_XOBJ_ONPROPERTYCHANGE
#define DISPID_EVPROP_ONPERSISTSAVE         (DISPID_EVENTS + 48)
#define DISPID_EVMETH_ONPERSISTSAVE          DISPID_ONPERSISTSAVE
#define DISPID_EVPROP_ONDRAG                (DISPID_EVENTS + 49)
#define DISPID_EVMETH_ONDRAG                 STDDISPID_XOBJ_ONDRAG
#define DISPID_EVPROP_ONDRAGEND             (DISPID_EVENTS + 50)
#define DISPID_EVMETH_ONDRAGEND              STDDISPID_XOBJ_ONDRAGEND
#define DISPID_EVPROP_ONDRAGENTER           (DISPID_EVENTS + 51)
#define DISPID_EVMETH_ONDRAGENTER            STDDISPID_XOBJ_ONDRAGENTER
#define DISPID_EVPROP_ONDRAGOVER            (DISPID_EVENTS + 52)
#define DISPID_EVMETH_ONDRAGOVER             STDDISPID_XOBJ_ONDRAGOVER
#define DISPID_EVPROP_ONDRAGLEAVE           (DISPID_EVENTS + 53)
#define DISPID_EVMETH_ONDRAGLEAVE            STDDISPID_XOBJ_ONDRAGLEAVE
#define DISPID_EVPROP_ONDROP                (DISPID_EVENTS + 54)
#define DISPID_EVMETH_ONDROP                 STDDISPID_XOBJ_ONDROP
#define DISPID_EVPROP_ONCUT                 (DISPID_EVENTS + 55)
#define DISPID_EVMETH_ONCUT                  STDDISPID_XOBJ_ONCUT
#define DISPID_EVPROP_ONCOPY                (DISPID_EVENTS + 56)
#define DISPID_EVMETH_ONCOPY                 STDDISPID_XOBJ_ONCOPY
#define DISPID_EVPROP_ONPASTE               (DISPID_EVENTS + 57)
#define DISPID_EVMETH_ONPASTE                STDDISPID_XOBJ_ONPASTE
#define DISPID_EVPROP_ONBEFORECUT           (DISPID_EVENTS + 58)
#define DISPID_EVMETH_ONBEFORECUT            STDDISPID_XOBJ_ONBEFORECUT
#define DISPID_EVPROP_ONBEFORECOPY          (DISPID_EVENTS + 59)
#define DISPID_EVMETH_ONBEFORECOPY           STDDISPID_XOBJ_ONBEFORECOPY
#define DISPID_EVPROP_ONBEFOREPASTE         (DISPID_EVENTS + 60)
#define DISPID_EVMETH_ONBEFOREPASTE          STDDISPID_XOBJ_ONBEFOREPASTE
#define DISPID_EVPROP_ONPERSISTLOAD         (DISPID_EVENTS + 61)
#define DISPID_EVMETH_ONPERSISTLOAD          DISPID_ONPERSISTLOAD
#define DISPID_EVPROP_ONROWSDELETE          (DISPID_EVENTS + 62)
#define DISPID_EVMETH_ONROWSDELETE           STDDISPID_XOBJ_ONROWSDELETE
#define DISPID_EVPROP_ONROWSINSERTED        (DISPID_EVENTS + 63)
#define DISPID_EVMETH_ONROWSINSERTED         STDDISPID_XOBJ_ONROWSINSERTED
#define DISPID_EVPROP_ONCELLCHANGE          (DISPID_EVENTS + 64)
#define DISPID_EVMETH_ONCELLCHANGE           STDDISPID_XOBJ_ONCELLCHANGE
#define DISPID_EVPROP_ONCONTEXTMENU         (DISPID_EVENTS + 65)
#define DISPID_EVMETH_ONCONTEXTMENU          DISPID_ONCONTEXTMENU
#define DISPID_EVPROP_ONBEFOREPRINT         (DISPID_EVENTS + 66)
#define DISPID_EVMETH_ONBEFOREPRINT          DISPID_ONBEFOREPRINT
#define DISPID_EVPROP_ONAFTERPRINT          (DISPID_EVENTS + 67)
#define DISPID_EVMETH_ONAFTERPRINT           DISPID_ONAFTERPRINT
#define DISPID_EVPROP_ONSTOP                (DISPID_EVENTS + 68)
#define DISPID_EVMETH_ONSTOP                DISPID_ONSTOP
#define DISPID_EVPROP_ONBEFOREEDITFOCUS     (DISPID_EVENTS + 69)
#define DISPID_EVMETH_ONBEFOREEDITFOCUS      DISPID_ONBEFOREEDITFOCUS
#define DISPID_EVPROP_ONATTACHEVENT         (DISPID_EVENTS + 70)
#define DISPID_EVPROP_ONMOUSEHOVER          (DISPID_EVENTS + 71)
#define DISPID_EVMETH_ONMOUSEHOVER           DISPID_ONMOUSEHOVER
#define DISPID_EVPROPS_COUNT                (                72)


//;begin_internal
#endif // __COREDISP_H__
//;end_internal

