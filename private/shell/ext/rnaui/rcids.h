//****************************************************************************
//
//  Module:     RNAUI.DLL
//  File:       rcids.h
//  Content:    This file contains all the constant declaration for the
//              RNA UI resources.
//  History:
//      Tue 23-Feb-1993 14:08:25  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#ifndef _RCIDS_H_
#define _RCIDS_H_


//*****************************************************************************
// General values
//*****************************************************************************

#define MAXRESTEXT      80
#define MAXMENUNAME     32
#define MAXDISPNAME     25
//#define CCH_MENUMAX     128  // max length of context menu strings
#define CCH_COLUMN      20
#define OFSTCMD_OPEN    0    // 0-based

//*****************************************************************************
// Icon ID number section
//*****************************************************************************

#define IDI_ICON        100             // ************************************
#define IDI_REMOTEFLD   100             //
#define IDI_REMOTE      101             // RC compiler does not want a
#define IDI_NEWREMOTE   102             // nested constant definition
#define IDI_REMOTE_FILE 103             //
#define IDI_DEL_CONN    104             // ************************************
#define IDI_DEL_MCONN   105
#define IDI_WELCOME_LIGHTBULB 106

// Icon orders
//
#define RMTFLD_ICON     0
#define CONN_ICON       1
#define NEWOBJ_ICON     2

#define IDB_REM_TB_SMALL    1000
#define IDB_WELCOME_MON     1001

//*****************************************************************************
// Menu ID number section
//*****************************************************************************

#define MENU_REMOTE             101
#define POPUP_CONTEXT           102
#define POPUP_DROP              103
#define MENU_SERVER             104

#define ID_MAIN_SUBMENU         0
#define ID_VIEW_SUBMENU         1
#define ID_HELP_SUBMENU         2

//*****************************************************************************
// Remote Folder ID number
//*****************************************************************************

#define ID_GENERIC_START        0x1000

#define IDM_GENERIC_START	(ID_GENERIC_START+0x0000)
#define IDM_GENERIC_DEFAULT	(IDM_GENERIC_START+0x0000)

// String IDs
//
#define IDS_GENERIC_START       (ID_GENERIC_START+0x0100)
#define IDS_CAP_REMOTE          (IDS_GENERIC_START+0x0002)

// Remote menu
#define RSVIDM_FIRST            0x0000
#define RSVIDM_REMOTE           (RSVIDM_FIRST + 0x0000)
#define RSVIDM_CONNECT          (RSVIDM_FIRST + 0x0001)
#define RSVIDM_CREATE           (RSVIDM_FIRST + 0x0002)
#define RSVIDM_DIALIN           (RSVIDM_FIRST + 0x0003)
#define RSVIDM_LINK             (RSVIDM_FIRST + 0x0004)
#define RSVIDM_DELETE           (RSVIDM_FIRST + 0x0005)
#define RSVIDM_PROPERTIES       (RSVIDM_FIRST + 0x0006)
#define RSVIDM_SETTING          (RSVIDM_FIRST + 0x0007)
#define RSVIDM_STATUS           (RSVIDM_FIRST + 0x0008)
#define RSVIDM_DISCONNECT       (RSVIDM_FIRST + 0x0009)
#define RSVIDM_RENAME           (RSVIDM_FIRST + 0x000A)

// Menu hints
#define IDS_MH_FIRST            (IDS_GENERIC_START+0x0300)
#define IDS_MH_REMOTE           (IDS_MH_FIRST + RSVIDM_REMOTE)
#define IDS_MH_CONNECT          (IDS_MH_FIRST + RSVIDM_CONNECT)
#define IDS_MH_CREATE           (IDS_MH_FIRST + RSVIDM_CREATE)
#define IDS_MH_DIALIN           (IDS_MH_FIRST + RSVIDM_DIALIN)
#define IDS_MH_LINK             (IDS_MH_FIRST + RSVIDM_LINK)
#define IDS_MH_DELETE           (IDS_MH_FIRST + RSVIDM_DELETE)
#define IDS_MH_PROPERTIES       (IDS_MH_FIRST + RSVIDM_PROPERTIES)
#define IDS_MH_SETTING          (IDS_MH_FIRST + RSVIDM_SETTING)
#define IDS_MH_STATUS           (IDS_MH_FIRST + RSVIDM_STATUS)
#define IDS_MH_DISCONNECT       (IDS_MH_FIRST + RSVIDM_DISCONNECT)
#define IDS_MH_RENAME           (IDS_MH_FIRST + RSVIDM_RENAME)

// Tooltips
#define IDS_TT_FIRST            (IDS_GENERIC_START+0x0400)
#define IDS_TT_CONNECT          (IDS_TT_FIRST + RSVIDM_CONNECT)
#define IDS_TT_CREATE           (IDS_TT_FIRST + RSVIDM_CREATE)
#define IDS_TT_DIALIN           (IDS_TT_FIRST + RSVIDM_DIALIN)
#define IDS_TT_SETTING          (IDS_TT_FIRST + RSVIDM_SETTING)
#define IDS_TT_STATUS           (IDS_TT_FIRST + RSVIDM_STATUS)
#define IDS_TT_DISCONNECT       (IDS_TT_FIRST + RSVIDM_DISCONNECT)

#define IDM_DROP_START          (IDM_GENERIC_START + 0x0010)
#define IDM_DROP_COPY           (IDM_DROP_START+0x0000)
#define IDM_DROP_MOVE           (IDM_DROP_START+0x0001)
#define IDM_DROP_CANCEL         (IDM_DROP_START+0x0002)

// Default Objects
//
#define IDS_REMOTE_START	(IDS_GENERIC_START+0x0500)
#define IDS_NEWREMOTE		(IDS_REMOTE_START+0x00)
#define IDS_UNKNOWNERROR	(IDS_REMOTE_START+0x01)

// Details view
//
#define IDS_ICOL_BASE           (IDS_REMOTE_START+0x03)
#define IDS_ICOL_NAME           IDS_ICOL_BASE
#define IDS_ICOL_PHONE          IDS_ICOL_BASE+1
#define IDS_ICOL_DEVICE         IDS_ICOL_BASE+2
#define IDS_ICOL_STATUS         IDS_ICOL_BASE+3

// The next 0x20 values are reserved

//*****************************************************************************
// Connection Information dialog box
//*****************************************************************************

#define IDD_ABENTRY         1200
#define IDC_AB_ICON         (IDD_ABENTRY)
#define IDC_AB_ENTRY	    (IDD_ABENTRY+1)
//
#define IDC_AB_AREATXT      (IDD_ABENTRY+3)
#define IDC_AB_AREA         (IDD_ABENTRY+4)
#define IDC_AB_PHONETXT     (IDD_ABENTRY+5)
#define IDC_AB_PHONE        (IDD_ABENTRY+6)
#define IDC_AB_COUNTRYTXT   (IDD_ABENTRY+7)
#define IDC_AB_COUNTRY	    (IDD_ABENTRY+8)
#define IDC_AB_FULLPHONE    (IDD_ABENTRY+9)
//
#define IDC_AB_DEVICETXT    (IDD_ABENTRY+14)
#define IDC_AB_DEVICE       (IDD_ABENTRY+15)
#define IDC_AB_DEVICESET    (IDD_ABENTRY+16)
//
#define IDC_AB_DEVICO       (IDD_ABENTRY+18)
#define IDC_AB_HELP         (IDD_ABENTRY+19)
#define IDC_AB_ADVDEV       (IDD_ABENTRY+20)
#define IDC_AB_PHNGRP       (IDD_ABENTRY+21)

#define IDC_AB_MLGRP        (IDD_ABENTRY+22)
#define IDC_AB_MLCNTTXT     (IDD_ABENTRY+23)
#define IDC_AB_MLCNT        (IDD_ABENTRY+24)
#define IDC_AB_MLSET        (IDD_ABENTRY+25)

#define IDD_NEWCONN         1250
#define IDD_ABMLENTRY       1260

//**************************************************************************
//  Global settings
//**************************************************************************

#define IDD_RNA_SETTING        1300
#define IDC_SET_REDIAL_LABEL   IDD_RNA_SETTING
#define IDC_SET_REDIAL         (IDD_RNA_SETTING+1)
#define IDC_SET_RDCNTLABEL     (IDD_RNA_SETTING+2)
#define IDC_SET_RDCNT          (IDD_RNA_SETTING+3)
#define IDC_SET_RDCNT_ARRW     (IDD_RNA_SETTING+4)
#define IDC_SET_RDC_UNIT       (IDD_RNA_SETTING+5)
#define IDC_SET_RDW_LABEL      (IDD_RNA_SETTING+6)
#define IDC_SET_RDWMIN         (IDD_RNA_SETTING+7)
#define IDC_SET_RDWMIN_ARRW    (IDD_RNA_SETTING+8)
#define IDC_SET_RDW_UNIT1      (IDD_RNA_SETTING+9)
#define IDC_SET_RDWSEC         (IDD_RNA_SETTING+10)
#define IDC_SET_RDWSEC_ARRW    (IDD_RNA_SETTING+11)
#define IDC_SET_RDW_UNIT2      (IDD_RNA_SETTING+12)
#define IDC_SET_IMPLICIT_LABEL (IDD_RNA_SETTING+13)
#define IDC_SET_ENIMPLICIT     (IDD_RNA_SETTING+14)
#define IDC_SET_DISIMPLICIT    (IDD_RNA_SETTING+15)
#define IDC_SET_DIAL_LABEL     (IDD_RNA_SETTING+16)
#define IDC_SET_TRAY           (IDD_RNA_SETTING+17)
#define IDC_SET_PROMPT         (IDD_RNA_SETTING+18)
#define IDC_SET_CONFIRM        (IDD_RNA_SETTING+19)

//**************************************************************************
//  Scripting
//**************************************************************************

#define IDD_SCRIPT            1400
#define IDC_SCRIPT_GRP        (IDD_SCRIPT+1)
#define IDC_SCRIPT_NAME       (IDD_SCRIPT+2)
#define IDC_EDIT              (IDD_SCRIPT+3)
#define IDC_SCRIPT_BROWSE     (IDD_SCRIPT+4)
#define IDC_MINIMIZED         (IDD_SCRIPT+5)
#define IDC_DEBUG             (IDD_SCRIPT+6)
#define IDC_SCRIPT_HELP       (IDD_SCRIPT+7)

//**************************************************************************
//  Multilink
//**************************************************************************

#define IDD_ML                1450
#define IDC_ML_DISABLE        (IDD_ML+1)
#define IDC_ML_ENABLE         (IDD_ML+2)
#define IDC_ML_GRP            (IDD_ML+3)
#define IDC_ML_FRAME          (IDD_ML+4)
#define IDC_ML_LIST           (IDD_ML+5)
#define IDC_ML_SEL_TXT        (IDD_ML+6)
#define IDC_ML_SEL            (IDD_ML+7)
#define IDC_ML_ADD            (IDD_ML+8)
#define IDC_ML_DEL            (IDD_ML+9)
#define IDC_ML_EDIT           (IDD_ML+10)

#define IDD_EDIT_MLI          1480
#define IDC_ML_DEVICE         (IDD_EDIT_MLI+1)
#define IDC_ML_PHONE          (IDC_AB_PHONE)

//*****************************************************************************
// Deletion Confirmation Dialog box
//*****************************************************************************

#define IDD_DELETE_CONN             1500
#define IDC_DEL_TEXT                IDD_DELETE_CONN+1

#define IDD_DELETE_MULTIPLE         1510

//*****************************************************************************
// Connection Confirmation Dialog box
//*****************************************************************************

#define IDD_CONFIRMCONNECT          1600
#define IDC_CC_NO_CONFIRM           (IDD_CONFIRMCONNECT+1)
#define IDC_CC_NAME                 (IDD_CONFIRMCONNECT+2)
#define IDC_CC_WHATSNEXT            (IDD_CONFIRMCONNECT+3)
#define IDC_WELCOME_BITMAP          (IDD_CONFIRMCONNECT+5)
#define IDC_WELCOME_TIPS2           (IDD_CONFIRMCONNECT+6)
#define IDC_WELCOME_TIPS            (IDD_CONFIRMCONNECT+7)
#define IDC_WELCOME_MON             (IDD_CONFIRMCONNECT+8)

#define IDC_STATIC                  -1

//*****************************************************************************
// Resource String
//*****************************************************************************

#define IDS_BASE                    100

#define IDS_RNAUI                   IDS_BASE

#define IDS_INI_SCRIPT_DIR          IDS_RNAUI+10
#define IDS_INI_SCRIPT_SHORTDIR     IDS_RNAUI+11
#define IDS_FILE_FILTER             IDS_RNAUI+12

#define IDS_SHORT_NAME              IDS_RNAUI+13

#define IDS_COUNTRY_FMT             IDS_RNAUI+14

#define IDS_OPENPORT                IDS_RNAUI+20
#define IDS_PORTOPENED              IDS_RNAUI+21
#define IDS_CONNECTDEVICE           IDS_RNAUI+22
#define IDS_DEVICECONNECTED         IDS_RNAUI+23
#define IDS_STARTAUTHENTICATION     IDS_RNAUI+24
#define IDS_AUTHENTICATE            IDS_RNAUI+25
#define IDS_CALLBACKPREP            IDS_RNAUI+26
#define IDS_WAITRESPOND             IDS_RNAUI+27
#define IDS_AUTHENTICATED           IDS_RNAUI+28
#define IDS_NETLOGON                IDS_RNAUI+29
#define IDS_CONNECTED               IDS_RNAUI+30
#define IDS_DISCONNECTED            IDS_RNAUI+31

#define IDS_DISP_PHONE_FMT          IDS_RNAUI+36
#define IDS_DISP_NA_PHONE_FMT       IDS_RNAUI+37

#define IDS_ML_COL_BASE             IDS_RNAUI+38
#define IDS_ML_COL_DEVICE           (IDS_ML_COL_BASE)
#define IDS_ML_COL_PHONE            (IDS_ML_COL_BASE+1)

//*****************************************************************************
// Error Messages
//*****************************************************************************

#define IDS_ERROR                    IDS_BASE+100
#define IDS_ERR_INVALID_PHONE        IDS_ERROR
#define IDS_ERR_INVALID_RENAME       IDS_ERROR+1

#define IDS_ERR_FILE_NOT_EXIST       (IDS_ERROR+2)

#define IDS_ERR_UNKNOWN_FORMAT       (IDS_ERROR+9)
#define IDS_ERR_BAD_PORT             (IDS_ERROR+10)
#define IDS_ERR_DEVICE_NOT_FOUND     (IDS_ERROR+11)
#define IDS_ERR_NO_DEVICE            (IDS_ERROR+12)
#define IDS_ERR_NAME_EXIST           (IDS_ERROR+13)

#define IDS_ERR_RESERVE_NAME         (IDS_ERROR+16)
#define IDS_ERR_DEVICE_INUSE         (IDS_ERROR+17)

#define IDS_ERR_INVALID_CONN         (IDS_ERROR+18)

#define IDS_ERR_CORRUPT_IMPORT       (IDS_ERROR+19)
#define IDS_ERR_NO_DEVICE_INSTALLED  (IDS_ERROR+20)

#define IDS_ERR_BAD_INSTALL          (IDS_ERROR+23)

#define IDS_ERR_NAME_TOO_LONG        (IDS_ERROR+25)

#define IDS_ERR_INV_RDCNT            (IDS_ERROR+26)
#define IDS_ERR_INV_RDWMIN           (IDS_ERROR+27)
#define IDS_ERR_INV_RDWSEC           (IDS_ERROR+28)

//*****************************************************************************
// Out of Memory Messages
//*****************************************************************************

#define IDS_OOM                     (IDS_BASE+200)

#define IDS_OOM_FILLSPACE           (IDS_OOM+1)


#endif  // _RCIDS_H_
