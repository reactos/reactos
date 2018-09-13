// Copyright (c) 1996-1999 Microsoft Corporation

#define STR_CARETNAME                   100

#define STR_CURSORNAMEFIRST             110

#define STR_TITLEBAR_NAME               140
#define STR_TITLEBAR_IMEBUTTON_NAME     141
#define STR_TITLEBAR_MINBUTTON_NAME     142
#define STR_TITLEBAR_MAXBUTTON_NAME     143
#define STR_TITLEBAR_HELPBUTTON_NAME    144
#define STR_TITLEBAR_CLOSEBUTTON_NAME   145
#define STR_TITLEBAR_RESTOREBUTTON_NAME 146

#define STR_TITLEBAR_DESCRIPTION        150
#define STR_TITLEBAR_SHORTCUT           160

#define STR_SCROLLBAR_NAME              180
#define STR_SCROLLBAR_DESCRIPTION       200

#define STR_MENUBAR_NAME                250
#define STR_SYSMENU_NAME                251
#define STR_MENUBAR_DESCRIPTION         252
#define STR_SYSMENUBAR_DESCRIPTION      253
#define STR_MENU_SHORTCUT               254
#define STR_MENU_SHORTCUT_FORMAT        255
#define STR_SYSMENU_KEY                 256
#define STR_CHILDSYSMENU_KEY            257
#define STR_EXECUTE                     258

#define STR_SHIFT                       259
#define STR_CONTROL                     260
#define STR_ALT                         STR_MENU_SHORTCUT
#define STR_CHILDSYSMENU_NAME           261
#define STR_STARTBUTTON_SHORTCUT		262
#define STR_CONTEXT_MENU                263
#define STR_DOCMENU_NAME				264
#define STR_DOUBLE_CLICK                265
#define STR_CLICK                       266

#define STR_WINDOW_NAME                 270
#define STR_STARTBUTTON                 271
#define STR_SPIN_GREATER                272
#define STR_SPIN_LESSER                 273
#define STR_TRAY                        274
#define STR_HOTKEY_NONE                 275

#define STR_COMBOBOX_LIST_SHORTCUT      280
#define STR_DROPDOWN_SHOW               285
#define STR_DROPDOWN_HIDE               286


#define STR_ALTTAB_NAME                 290
#define STR_ALTTAB_DESCRIPTION          291
#define STR_TAB_SWITCH                  292
#define STR_MDICLI_NAME                 293
#define STR_DESKTOP_NAME                294
#define STR_PERCENTAGE_FORMAT           295

#define STR_TREE_EXPAND                 305
#define STR_TREE_COLLAPSE               306

#define STR_HTML_JUMP                   307
#define STR_BUTTON_PUSH                 308
#define STR_BUTTON_CHECK                309
#define STR_BUTTON_UNCHECK              310
#define STR_BUTTON_HALFCHECK            311


#define STR_CLASSFIRST                  500
#define STR_LISTBOX                     500
#define STR_MENUPOPUP                   501
#define STR_BUTTON                      502
#define STR_STATIC                      503
#define STR_EDIT                        504
#define STR_COMBOBOX                    505
#define STR_DIALOG                      506
#define STR_SWITCH                      507
#define STR_MDICLIENT                   508
#define STR_DESKTOP                     509
#define STR_SCROLLCTL                   510
#define STR_COMCTL32_STATUSBAR          511
#define STR_COMCTL32_TOOLBAR            512
#define STR_COMCTL32_PROGRESSBAR        513
#define STR_COMCTL32_ANIMATED           514
#define STR_COMCTL32_TABCONTROL         515
#define STR_COMCTL32_HOTKEY             516
#define STR_COMCTL32_HEADER             517
#define STR_COMCTL32_SLIDER             518
#define STR_COMCTL32_LISTVIEW           519
#define STR_SMD96_LISTVIEW				520
#define STR_COMCTL32_UPDOWN             521
#define STR_COMCTL32_UPDOWN32           522
#define STR_COMCTL32_TOOLTIPS           523
#define STR_COMCTL32_TOOLTIPS32         524
#define STR_COMCTL32_OUTLINEVIEW        525
#define STR_COMCTL32_CALENDAR           526
#define STR_COMCTL32_DATETIME           527
#define STR_COMCTL32_HTML               528
#define STR_RICHEDIT                    529
#define STR_RICHEDIT20A                 530
#define STR_RICHEDIT20W                 531
#define STR_SDM95_WORD1                 532
#define STR_SDM95_WORD2                 533
#define STR_SDM95_WORD3                 534
#define STR_SDM95_WORD4                 535
#define STR_SDM95_WORD5                 536
#define STR_SDM95_EXCEL1                537
#define STR_SDM95_EXCEL2                538
#define STR_SDM95_EXCEL3                539
#define STR_SDM95_EXCEL4                540
#define STR_SDM95_EXCEL5                541
#define STR_SDM96_WORD1                 542
#define STR_SDM96_WORD2                 543
#define STR_SDM96_WORD3                 544
#define STR_SDM96_WORD4                 545
#define STR_SDM96_WORD5                 546
#define STR_SDM31_WORD1                 547
#define STR_SDM31_WORD2                 548
#define STR_SDM31_WORD3                 549
#define STR_SDM31_WORD4                 550
#define STR_SDM31_WORD5                 551
#define STR_SDM96_OFFICE1               552
#define STR_SDM96_OFFICE2               553
#define STR_SDM96_OFFICE3               554
#define STR_SDM96_OFFICE4               555
#define STR_SDM96_OFFICE5               556
#define STR_SDM96_EXCEL1                557
#define STR_SDM96_EXCEL2                558
#define STR_SDM96_EXCEL3                559
#define STR_SDM96_EXCEL4                560
#define STR_SDM96_EXCEL5                561
#define STR_CLASSLAST                   561

#define CSTR_CLIENT_CLASSES             (STR_CLASSLAST - STR_CLASSFIRST + 1)
#define CSTR_WINDOW_CLASSES             2

// The first 32 class names can referred to by index values
// when sending  WM_GETOBJECT/OBJID_QUERYCLASSNAMEIDX.
#define CSTR_QUERYCLASSNAME_CLASSES     32
// We actually use (index + 65536) - to keep the return value
// out of the way of apps which return small intergers for
// WM_GETOBJECT even though they shouldn't (ie. Notes)
#define CSTR_QUERYCLASSNAME_BASE        65536

#define STR_STATEFIRST                  1000
#define STR_ROLEFIRST                   1100
