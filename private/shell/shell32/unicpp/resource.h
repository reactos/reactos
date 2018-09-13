// copied from shdocvw\resource.h
// values adjusted to not conflict with shell32\ids.h
//

// IE4 shipped with FCIDM_NEXTCTL as 0xA030 and we can not change it
// because we need to support IE5 browser only on top of IE4 integrated.
#define FCIDM_NEXTCTL       (FCIDM_BROWSERFIRST + 0x30) // explorer browseui shell32

// IE4 shipped with FCIDM_FINDFILES as 0xA0085 and we can not change it
// because we need to support IE5 browser only on top of IE4 integrated.
#define FCIDM_FINDFILES     (FCIDM_BROWSERFIRST + 0x85)

#define IDD_BACKGROUND                  0x7500
#define IDD_COMPONENTS                  0x7501
#define IDD_ADDCOMPONENT                0x7502
#define IDD_PATTERN                     0x7503
#define IDD_EDITPAT                     0x7504

#define IDD_FOLDEROPTIONS               0x7505 
#define IDD_ADVANCEDOPTIONS             0x7508

#define IDI_BACK_NONE                   200 // shell32 id.h goes to 173 for icons
#define IDI_FRAME                       0x0200

#define IDC_PAT_LIST                    0x7550
#define IDC_PAT_SAMPLE                  0x7551
#define IDC_PAT_EDIT                    0x7552
#define IDC_PAT_PATTERN                 0x7553
#define IDC_PAT_PREVIEW                 0x7554

#define IDC_EPAT_COMBO                  0x7555
#define IDC_EPAT_SAMPLE                 0x7556
#define IDC_EPAT_PATTERN                0x7557
#define IDC_EPAT_ADD                    0x7558
#define IDC_EPAT_CHANGE                 0x7559
#define IDC_EPAT_DEL                    0x755a
#define IDC_EPAT_NAME                   0x755b
#define IDC_EPAT_SAMPTEXT               0x755c
#define IDC_EPAT_PATTEXT                0x755d

#define IDC_BACK_WPLIST                 0x7560
#define IDC_BACK_BROWSE                 0x7561
#define IDC_BACK_PATTERN                0x7562
#define IDC_BACK_WPSTYLE                0x7563
#define IDC_BACK_PREVIEW                0x7565
// #define IDC_BACK_GROUP                  0x7566
#define IDC_BACK_DISPLAY                0x7567
#define IDC_BACK_SELECT                 0x7568

#define IDC_COMP_LIST                   0x7570
#define IDC_COMP_NEW                    0x7571
#define IDC_COMP_DELETE                 0x7572
#define IDC_COMP_PROPERTIES             0x7573
//#define IDC_COMP_RESET                  0x7574
#define IDC_COMP_PREVIEW                0x7575
//#define IDC_COMP_CHANNELS               0x7576
#define IDC_COMP_ENABLEAD               0x7577
//#define IDC_COMP_FOLDEROPT              0x7578

#define IDC_CPROP_COMPLIST              0x7579
#define IDC_CPROP_SOURCE                0x757a
#define IDC_CPROP_BROWSE                0x757b
#define IDC_GOTO_GALLERY                0x757c

#define IDC_KBSTART                     0x857c

// ids for IDD_FOLDEROPTIONS
// Warning: Do not change the order and sequence of the following IDs.
// The code assumes and asserts if it changes.
#define IDC_FCUS_WEB                    0x7590
#define IDC_FCUS_CLASSIC                (IDC_FCUS_WEB + 1)              // 0x7591
#define IDC_FCUS_WHENEVER_POSSIBLE      (IDC_FCUS_CLASSIC + 1)          // 0x7592
#define IDC_FCUS_WHEN_CHOOSE            (IDC_FCUS_WHENEVER_POSSIBLE + 1)// 0x7593
#define IDC_FCUS_SAME_WINDOW            (IDC_FCUS_WHEN_CHOOSE + 1)      // 0x7594
#define IDC_FCUS_SEPARATE_WINDOWS       (IDC_FCUS_SAME_WINDOW + 1)      // 0x7595
#define IDC_FCUS_SINGLECLICK            (IDC_FCUS_SEPARATE_WINDOWS + 1) // 0x7596
#define IDC_FCUS_DOUBLECLICK            (IDC_FCUS_SINGLECLICK + 1)      // 0x7597
#define IDC_FCUS_ICON_IE                (IDC_FCUS_DOUBLECLICK + 1)      // 0x7598
#define IDC_FCUS_ICON_HOVER             (IDC_FCUS_ICON_IE + 1)          // 0x7599
#define IDC_FCUS_ICON_MAX               (IDC_FCUS_ICON_HOVER + 1)
// End of Warning: Do not change the order and sequence of the above IDC_FCUS_* id values.

#define IDC_FCUS_RESTORE_DEFAULTS       0x759B

// Warning: Do not change the order and sequence of the following IDs.
#define IDC_FCUS_ICON_ACTIVEDESKTOP     0x759C
#define IDC_FCUS_ICON_WEBVIEW           (IDC_FCUS_ICON_ACTIVEDESKTOP + 1)  // 0x759D
#define IDC_FCUS_ICON_WINDOW            (IDC_FCUS_ICON_WEBVIEW + 1)        // 0x759E           
#define IDC_FCUS_ICON_CLICKS            (IDC_FCUS_ICON_WINDOW + 1)         // 0x759F
// End of Warning: Do not change the order and sequence of the above IDC_FOCUS_* 

#define IDC_FCUS_WEBVIEW_GROUP_STATIC   0x7580

// ids for IDD_CHECKSINGLECLICK
#define IDC_SC_YES                      0x75A0
#define IDC_SC_NO                       0x75A1
#define IDC_SC_MOREINFO                 0x75A2

// ids for IDD_ADVANCEDOPTIONS
#define IDC_ADVO_ADVANCEDTREE           0x75A8
#define IDC_ADVO_ADV_RESTORE_DEF        0x75A9
#define IDC_ADVO_RESETTOORIGINAL        0x75AA
#define IDC_ADVO_USECURRENTFOLDER       0x75AB
#define IDC_ADVO_IMAGEFOLDER            0x75AC
#define IDC_ADVO_ADVANCEDTEXT           0x75AD
#define IDC_ADVO_GROUPBOX               0x75AE
#define IDC_ADVO_STATICTEXT             0x75AF

//
// The constants below came from DESKHTML.DLL
//
#define IDS_SUBSCRIBEDURL               0x7600
#define IDS_RESIZEABLE                  0x7601
#define IDS_BASE_TAG                    0x7602
#define IDS_COMMENT_BEGIN               0x7603
#define IDS_COMMENT1                    0x7604
#define IDS_COMMENT_END                 0x7605
#define IDS_HEADER_BEGIN                0x7606
#define IDS_BODY_BEGIN                  0x7607
#define IDS_DIV_START                   0x7608
#define IDS_DIV_SIZE                    0x7609
#define IDS_IMAGE_BEGIN                 0x760a
#define IDS_IMAGE_LOCATION              0x760b
#define IDS_IMAGE_SIZE                  0x760c
#define IDS_DIV_END                     0x760d
#define IDS_IFRAME_BEGIN                0x760e
#define IDS_IFRAME_SIZE                 0x760f
#define IDS_BODY_END                    0x7610
#define IDS_CONTROL_1                   0x7611
#define IDS_CONTROL_2                   0x7612
#define IDS_CONTROL_3                   0x7613
#define IDS_DIV_START2                  0x7614
#define IDS_DIV_START2W                 0x7615
#define IDS_IMAGE_BEGIN2                0x7616
#define IDS_IFRAME_BEGIN2               0x7617
#define IDS_BODY_BEGIN2                 0x7618
#define IDS_BODY_CENTER_WP              0x7619
#define IDS_BODY_PATTERN_AND_WP         0x761a
#define IDS_STRETCH_WALLPAPER           0x761b
#define IDS_WPSTYLE                     0x761c    // first style string
#define IDS_WPSTYLE_CENTER              (IDS_WPSTYLE)
#define IDS_WPSTYLE_TILE                (IDS_WPSTYLE+1)
#define IDS_WPSTYLE_STRETCH             (IDS_WPSTYLE+2)
#define IDS_PAT_UNLISTED                0x7620
#define IDS_EPAT_REMOVECAP              0x7621
#define IDS_EPAT_REMOVE                 0x7622
#define IDS_EPAT_CHANGECAP              0x7623
#define IDS_EPAT_CHANGE                 0x7624
#define IDS_EPAT_CREATE                 0x7625
#define IDS_COMP_BADURL                 0x7626
#define IDS_COMP_TITLE                  0x7627
#define IDS_COMP_EXISTS                 0x7628
#define IDS_COMP_SUBSCRIBED             0x7629
#define IDS_COMP_BADSUBSCRIBE           0x762a
#define IDS_SAMPLE_COMPONENT            0x762b
#define IDS_CHANNEL_BAR                 0x762c
#define IDS_COMP_CONFIRMDEL             0x762d
#define IDS_VALIDFN_FMT                 0x762e
#define IDS_VALIDFN_TITLE               0x762f
#define IDS_BACK_TYPE1                  0x7630
#define IDS_BACK_TYPE2                  0x7631
#define IDS_COMP_TYPE1                  0x7632
#define IDS_COMP_TYPE2                  0x7633
#define IDS_VISIT_URL                   0x7634
#define IDS_COMP_CONFIRMRESET           0x7635
#define IDS_BACK_FILETYPES              0x7636
#define IDS_COMP_FILETYPES              0x7637
#define IDS_ADDCOMP_ERROR_CDFNODTI      0x7638
#define IDS_ADDCOMP_ERROR_CDFINALID     0x7639
#define IDS_VISITGALLERY_TEXT           0x763a
#define IDS_VISITGALLERY_TITLE          0x763b
#define IDS_CONFIRM_ADI_REINSTALL       0x763d
#define IDS_ADDRBAND_ACCELLERATOR       0x763e
#define IDS_FOLDEROPT_TEXT              0x763f
#define IDS_FOLDEROPT_TITLE             0x7640
#define IDS_FOLDERVIEWS                 0x7643
#define IDS_LIKECURRENT_TEXT            0x7644
#define IDS_RESETALL_TEXT               0x7645
#define IDS_DIV_START3                  0x7646
#define IDS_BODY_PATTERN_AND_WP2        0x7647
#define IDS_BODY_CENTER_WP2             0x7648
#define IDS_BODY_END2                   0x7649
#define IDS_COMP_EXISTS_2               0x764a

#define IDS_HTMLDOCUMENT                0x764b
#define IDS_PICTURE                     0x764c
#define IDS_WEBSITE                     0x764d
#define IDS_ADDTODESKTOP                0x764e
#define IDS_EDITDESKTOPCOMP             0x764f
#define IDS_CDF_FILTER                  0x7650
#define IDS_URL_FILTER                  0x7651
#define IDS_TYPETHENAMESTRING           0x7652
#define IDS_IMAGES_FILTER               0x7653
#define IDS_HTMLDOC_FILTER              0x7654
#define IDS_CONFIRM_ADD                 0x7655
#define IDS_INTERNET_EXPLORER           0x7656
#define IDS_MHTML_FILTER                0x7657

#define IDS_NOHTML_SELECTED             0x7658
#define IDS_URL_EXTENSION               0x7659
#define IDS_CONFIRM_OVERWRITE_SUBSCR    0x765a
#define IDS_CONFIRM_RESET_SAFEMODE      0x765b
#define IDS_WPNONE                      0x765c
#define IDS_CONFIRM_TURNINGON_AD        0x765d // Not for the confirm close dialogs!
#define IDS_MENU_SUBSCRIBE_DESKCOMP     0x765e
#define IDS_YOULOSE                     0x765f

#define IDS_CMTF_COPYTO                 0x7660
#define IDS_CMTF_MOVETO                 0x7661
#define IDS_CMTF_COPY_DLG_TITLE         0x7662 //cmtf <=> CopyMoveToFolder
#define IDS_CMTF_MOVE_DLG_TITLE         0x7663
//#define IDS_CMTF_COPYORMOVE_DLG_TITLE 0x7677
#define IDS_CMTF_ERRORMSG               0x7664

#define IDS_CABINET                     0x7665
#define IDS_CANTFINDDIR                 0x7666

#define ACCEL_DESKTOP           3
#define IDS_MENU_RESET                  0x7667

#define IDS_SENDLINKTO                  0x7668
#define IDS_SENDPAGETO                  0x7669
#define IDS_SENDTO_ERRORMSG             0x766a

#define IDS_NEWMENU             0x766b
#define IDS_NEWFILEPREFIX       0x766c
#define IDS_NEWFOLDER           0x766d
#define IDS_NEWLINK             0x766e
#define IDS_FOLDERTEMPLATE      0x766f
#define IDS_FOLDERLONGPLATE     0x7670
#define IDS_NEWLINKTEMPLATE     0x7671
#define IDS_NEWFILE_ERROR_TITLE 0x7672
#define IDS_MY_CURRENT_HOMEPAGE 0x7673
#define IDS_FIND_MEUMONIC       0x7674  // Old Accelerator
#define IDS_CONNECTING          0x7675 
#define IDS_INFOTIP             0x7676  // Used by the shell automation object
#define IDS_CMTF_COPYORMOVE_DLG_TITLE   0x7677

#define IDS_COMP_ICW_ADD                0x7678
#define IDS_COMP_ICW_DISABLE            0x7679
#define IDS_COMP_ICW_TOGGLE             0x767A
#define IDS_COMP_ICW_TITLE              0x767B

#define IDS_NEWHELP_FIRST       0x767C
#define IDS_NEWHELP_FOLDER      0x767C
#define IDS_NEWHELP_LINK        0x767D


// Start Menu Info Tips.
#define IDS_PROGRAMS_TIP        0x7680
#define IDS_FAVORITES_TIP       0x7681
#define IDS_RECENT_TIP          0x7682
#define IDS_SETTINGS_TIP        0x7683
#define IDS_FIND_TIP            0x7684
#define IDS_HELP_TIP            0x7685
#define IDS_RUN_TIP             0x7686
#define IDS_LOGOFF_TIP          0x7687
#define IDS_EJECT_TIP           0x7688
#define IDS_SHUTDOWN_TIP        0x7689
#define IDS_CONTROL_TIP         0x768A
#define IDS_PRINTERS_TIP        0x768B
#define IDS_TRAYPROP_TIP        0x768C
#define IDS_MYDOCS_TIP          0x768D
#define IDS_NETCONNECT_TIP      0x768E
#define IDS_CHEVRONTIPTITLE     0x768F
#define IDS_CHEVRONTIP          0x7690
#define IDS_ALL_PICTURES        0x76A0
#define IDS_ALL_HTML            0x76A1
#define IDS_HTMLDOC             0x76A2


// Still more constants moved (and renumbered) from shdocvw\resource.h

//#define IDS_COMPSETTINGS              xxxxxx // already in shell32\resource.h

// More constants moved (and renumbered) from shdocvw\resource.h

#define MENU_DESKCOMP_CONTEXTMENU       400
#define MENU_STARTMENU_MYDOCS           401
#define MENU_STARTMENU_OPENFOLDER       402
// BUGBUG - raymondc - This used to be at FCIDM_BROWSER_EXPLORE+0x240.
// What does that mean?  Is that number special?  I just gave it a new
// number.
#define IDM_DCCM_FIRST          0xA100
#define DCCM_MENUITEMS          20  //  reserved space for Desktop Component Menu items
#define IDM_DCCM_OPEN           (IDM_DCCM_FIRST+0x01)
#define IDM_DCCM_OFFLINE        (IDM_DCCM_FIRST+0x02)
#define IDM_DCCM_SYNCHRONIZE    (IDM_DCCM_FIRST+0x03)
#define IDM_DCCM_PROPERTIES     (IDM_DCCM_FIRST+0x04)
#define IDM_DCCM_CUSTOMIZE      (IDM_DCCM_FIRST+0x08)
#define IDM_DCCM_CLOSE          (IDM_DCCM_FIRST+0x09)
#define IDM_DCCM_FULLSCREEN     (IDM_DCCM_FIRST+0x0a)
#define IDM_DCCM_SPLIT          (IDM_DCCM_FIRST+0x0b)
#define IDM_DCCM_RESTORE        (IDM_DCCM_FIRST+0x0c)
#define IDM_DCCM_LASTCOMPITEM   (IDM_DCCM_FIRST+DCCM_MENUITEMS)

#define IDM_MYDOCUMENTS         516
#define IDM_OPEN_FOLDER         517

// Bitmap IDs

#define IDB_MONITOR             0x135
#define IDB_WIZARD              0x136

// These numbers are for CShellDispatch (sdmain.cpp) to send
// messages to the tray.

//The following value is taken from  shdocvw\rcids.h
#ifndef FCIDM_REFRESH
#define FCIDM_REFRESH  0xA220
#endif // FCIDM_REFRESH

#define FCIDM_BROWSER_VIEW      (FCIDM_BROWSERFIRST+0x0060)
#define FCIDM_BROWSER_TOOLS     (FCIDM_BROWSERFIRST+0x0080)

#define FCIDM_STOP              (FCIDM_BROWSER_VIEW + 0x001a)
#define FCIDM_ADDTOFAVNOUI      (FCIDM_BROWSER_VIEW + 0x0021)
#define FCIDM_VIEWITBAR         (FCIDM_BROWSER_VIEW + 0x0022)
#define FCIDM_VIEWSEARCH        (FCIDM_BROWSER_VIEW + 0x0017)
#define FCIDM_CUSTOMIZEFOLDER   (FCIDM_BROWSER_VIEW + 0x0018)
#define FCIDM_VIEWFONTS         (FCIDM_BROWSER_VIEW + 0x0019)
// 1a is FCIDM_STOP
#define FCIDM_THEATER           (FCIDM_BROWSER_VIEW + 0x001b)        
#define FCIDM_JAVACONSOLE       (FCIDM_BROWSER_VIEW + 0x001c)

#define FCIDM_BROWSER_EDIT      (FCIDM_BROWSERFIRST+0x0040)
#define FCIDM_MOVE              (FCIDM_BROWSER_EDIT+0x0001)
#define FCIDM_COPY              (FCIDM_BROWSER_EDIT+0x0002)
#define FCIDM_PASTE             (FCIDM_BROWSER_EDIT+0x0003)
#define FCIDM_SELECTALL         (FCIDM_BROWSER_EDIT+0x0004)
#define FCIDM_LINK              (FCIDM_BROWSER_EDIT+0x0005)     // create shortcut
#define FCIDM_EDITPAGE          (FCIDM_BROWSER_EDIT+0x0006)
