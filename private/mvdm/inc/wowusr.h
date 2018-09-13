/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WOWUSR.H
 *  16-bit User API argument structures
 *
 *  History:
 *  Created 02-Feb-1991 by Jeff Parsons (jeffpar)
 *  Added Win 3.1 APIs on 19-March-1992 Chandan S. Chauhan (ChandanC)
 *
--*/

/* User API IDs
 */
#define FUN_ADJUSTWINDOWRECT        102 //
#define FUN_ADJUSTWINDOWRECTEX      454 //
#define FUN_ANSILOWER               432 //
#define FUN_ANSILOWERBUFF           438 //
#define FUN_ANSINEXT                472 //
#define FUN_ANSIPREV                473 //
#define FUN_ANSIUPPER               431 //
#define FUN_ANSIUPPERBUFF           437 //
#define FUN_ANYPOPUP                52  //
#define FUN_APPENDMENU              411 //
#define FUN_ARRANGEICONICWINDOWS    170 // Internal, proto
#define FUN_BEGINDEFERWINDOWPOS     259 // Internal, proto
#define FUN_BEGINPAINT              39  //
#define FUN_BRINGWINDOWTOTOP        45  //
#define FUN_BROADCASTMESSAGE        355 // Internal
#define FUN_BUILDCOMMDCB            213 //
#define FUN_BUTTONWNDPROC           303 // Internal
#define FUN_CALCCHILDSCROLL         462 // Internal
#define FUN_CALLMSGFILTER           123 //
#define FUN_CALLWINDOWPROC          122 //
#define FUN_CARETBLINKPROC          311 // Internal
#define FUN_CASCADECHILDWINDOWS     198 // Internal
#define FUN_CHANGECLIPBOARDCHAIN    149 //
#define FUN_CHANGEMENU              153 //
#define FUN_CHECKDLGBUTTON          97  //
#define FUN_CHECKMENUITEM           154 //
#define FUN_CHECKRADIOBUTTON        96  //
#define FUN_CHILDWINDOWFROMPOINT    191 //
#define FUN_CLEARCOMMBREAK          211 //
#define FUN_CLIENTTOSCREEN          28  //
#define FUN_CLIPCURSOR              16  //
#define FUN_CLOSECLIPBOARD          138 //
#define FUN_CLOSECOMM               207 //
#define FUN_CLOSEWINDOW             43  //
#define FUN_COMBOBOXCTLWNDPROC      344 // Internal
#define FUN_COMPUPDATERECT          316 // Internal
#define FUN_COMPUPDATERGN           317 // Internal
#define FUN_CONTROLPANELINFO        273 // Internal
#define FUN_CONTSCROLL              310 // Internal
#define FUN_COPYRECT                74  //
#define FUN_COUNTCLIPBOARDFORMATS   143 //
#define FUN_CREATECARET             163 //
#define FUN_CREATECURSOR            406 //
#define FUN_CREATECURSORICONINDIRECT 408 // Internal
#define FUN_CREATEDIALOG            89  //
#define FUN_CREATEDIALOGINDIRECT    219 //
#define FUN_CREATEDIALOGINDIRECTPARAM 242 //
#define FUN_CREATEDIALOGPARAM       241 //
#define FUN_CREATEICON              407 //
#define FUN_CREATEMENU              151 //
#define FUN_CREATEPOPUPMENU         415 //
#define FUN_CREATEWINDOW            41  //
#define FUN_CREATEWINDOWEX          452 //
#define FUN_DEFDLGPROC              308 //
#define FUN_DEFERWINDOWPOS          260 // Internal, proto
#define FUN_DEFFRAMEPROC            445 //
#define FUN_DEFHOOKPROC             235 //
#define FUN_DEFMDICHILDPROC         447 //
#define FUN_DEFWINDOWPROC           107 //
#define FUN_DELETEMENU              413 //
#define FUN_DESKTOPWNDPROC          305 // Internal
#define FUN_DESTROYCARET            164 //
#define FUN_DESTROYCURSOR           458 //
#define FUN_DESTROYICON             457 //
#define FUN_DESTROYMENU             152 //
#define FUN_DESTROYWINDOW           53  //
#define FUN_DIALOGBOX               87  //
#define FUN_DIALOGBOXINDIRECT       218 //
#define FUN_DIALOGBOXINDIRECTPARAM  240 //
#define FUN_DIALOGBOXPARAM          239 //
#define FUN_DISABLEOEMLAYER         4   // Internal
#define FUN_DISPATCHMESSAGE         114 //
#define FUN_DLGDIRLIST              100 //
#define FUN_DLGDIRLISTCOMBOBOX      195 //
#define FUN_DLGDIRSELECT            99  //
#define FUN_DLGDIRSELECTCOMBOBOX    194 //
#define FUN_DRAGDETECT              465 // Internal
#define FUN_DRAGOBJECT              464 // Internal
#define FUN_DRAWFOCUSRECT           466 //
#define FUN_DRAWICON                84  //
#define FUN_DRAWMENUBAR             160 //
#define FUN_DRAWTEXT                85  //
#define FUN_DUMPICON                459 // Internal
#define FUN_EDITWNDPROC             301 // Internal
#define FUN_EMPTYCLIPBOARD          139 //
#define FUN_ENABLEHARDWAREINPUT     331 //
#define FUN_ENABLEMENUITEM          155 //
#define FUN_ENABLEOEMLAYER          3   // Internal
#define FUN_ENABLEWINDOW            34  //
#define FUN_ENDDEFERWINDOWPOS       261 // Internal, proto
#define FUN_ENDDIALOG               88  //
#define FUN_ENDMENU                 187 // Internal
#define FUN_ENDPAINT                40  //
#define FUN_ENUMCHILDWINDOWS        55  //
#define FUN_ENUMCLIPBOARDFORMATS    144 //
#define FUN_ENUMPROPS               27  //
#define FUN_ENUMTASKWINDOWS         225 //
#define FUN_ENUMWINDOWS             54  //
#define FUN_EQUALRECT               244 //
#define FUN_ESCAPECOMMFUNCTION      214 //
#define FUN_EXCLUDEUPDATERGN        238 //
#define FUN_EXITWINDOWS             7   // Internal, proto
#define FUN_FARCALLNETDRIVER        500 // Internal
#define FUN_FILEPORTDLGPROC         346 // Internal
#define FUN_FILLRECT                81  //
#define FUN_FILLWINDOW              324 // Internal
#define FUN_FINALUSERINIT           400 // Internal
#define FUN_FINDWINDOW              50  //
#define FUN_FLASHWINDOW             105 //
#define FUN_FLUSHCOMM               215 //
#define FUN_FRAMERECT               83  //
#define FUN_GETACTIVEWINDOW         60  //
#define FUN_GETASYNCKEYSTATE        249 //
#define FUN_GETCAPTURE              236 //
#define FUN_GETCARETBLINKTIME       169 //
#define FUN_GETCARETPOS             183 //
#define FUN_GETCLASSINFO            404 //
#define FUN_GETCLASSLONG            131 //
#define FUN_GETCLASSNAME            58  //
#define FUN_GETCLASSWORD            129 //
#define FUN_GETCLIENTRECT           33  //
#define FUN_GETCLIPBOARDDATA        142 //
#define FUN_GETCLIPBOARDFORMATNAME  146 //
#define FUN_GETCLIPBOARDOWNER       140 //
#define FUN_GETCLIPBOARDVIEWER      148 //
#define FUN_GETCOMMERROR            203 //
#define FUN_GETCOMMEVENTMASK        209 //
#define FUN_GETCOMMSTATE            202 //
#define FUN_GETCONTROLBRUSH         326 // Internal
#define FUN_GETCURRENTTIME          15  //
#define FUN_GETSYSTEMMSECCOUNT      15  //  This system.drv rtn gets thunked to GetCurrentTime
#define FUN_GETCURSORPOS            17  //
#define FUN_GETDC                   66  //
#define FUN_GETDESKTOPHWND          278 // Internal, proto
#define FUN_GETDESKTOPWINDOW        286 //
#define FUN_GETDIALOGBASEUNITS      243 //
#define FUN_GETDLGCTRLID            277 // Internal, proto
#define FUN_GETDLGITEM              91  //
#define FUN_GETDLGITEMINT           95  //
#define FUN_GETDLGITEMTEXT          93  //
#define FUN_GETDOUBLECLICKTIME      21  //
#define FUN_GETFILEPORTNAME         343 // Internal
#define FUN_GETFOCUS                23  //
#define FUN_GETICONID               455 // Internal
#define FUN_GETINPUTSTATE           335 //
#define FUN_GETINTERNALWINDOWPOS    460 // Internal
#define FUN_GETKEYBOARDSTATE        222 //
#define FUN_GETKEYSTATE             106 //
#define FUN_GETLASTACTIVEPOPUP      287 //
#define FUN_GETMENU                 157 //
#define FUN_GETMENUCHECKMARKDIMENSIONS  417 //
#define FUN_GETMENUITEMCOUNT        263 //
#define FUN_GETMENUITEMID           264 //
#define FUN_GETMENUSTATE            250 //
#define FUN_GETMENUSTRING           161 //
#define FUN_GETMESSAGE              108 //
#define FUN_GETMESSAGE2             323 // Internal
#define FUN_GETMESSAGEPOS           119 //
#define FUN_GETMESSAGETIME          120 //
#define FUN_GETMOUSEEVENTPROC       337 // Internal
#define FUN_GETNEXTDLGGROUPITEM     227 //
#define FUN_GETNEXTDLGTABITEM       228 //
#define FUN_GETNEXTQUEUEWINDOW      274 // Internal
#define FUN_GETNEXTWINDOW           230 //
#define FUN_GETPARENT               46  //
#define FUN_GETPRIORITYCLIPBOARDFORMAT  402 //
#define FUN_GETPROP                 25  //
#define FUN_GETQUEUESTATUS          334 // Internal
#define FUN_GETSCROLLPOS            63  //
#define FUN_GETSCROLLRANGE          65  //
#define FUN_GETSUBMENU              159 //
#define FUN_GETSYSCOLOR             180 //
#define FUN_GETSYSMODALWINDOW       189 //
#define FUN_GETSYSTEMMENU           156 //
#define FUN_GETSYSTEMMETRICS        179 //
#define FUN_GETTABBEDTEXTEXTENT     197 //
#define FUN_GETTICKCOUNT            13  //
#define FUN_GETTIMERRESOLUTION      14  // Internal
#define FUN_GETTOPWINDOW            229 //
#define FUN_GETUPDATERECT           190 //
#define FUN_GETUPDATERGN            237 //
#define FUN_GETWC2                  318 // Internal
#define FUN_GETWINDOW               262 //
#define FUN_GETWINDOWDC             67  //
#define FUN_GETWINDOWLONG           135 //
#define FUN_GETWINDOWRECT           32  //
#define FUN_GETWINDOWTASK           224 //
#define FUN_GETWINDOWTEXT           36  //
#define FUN_GETWINDOWTEXTLENGTH     38  //
#define FUN_GETWINDOWWORD           133 //
#define FUN_GLOBALADDATOM           268 //
#define FUN_GLOBALDELETEATOM        269 //
#define FUN_GLOBALFINDATOM          270 //
#define FUN_GLOBALGETATOMNAME       271 //
#define FUN_GRAYSTRING              185 //
#define FUN_HIDECARET               166 //
#define FUN_HILITEMENUITEM          162 //
#define FUN_ICONSIZE                86  // Internal
#define FUN_INFLATERECT             78  //
#define FUN_INITAPP                 5   // No proto
#define FUN_INSENDMESSAGE           192 //
#define FUN_INSERTMENU              410 //
#define FUN_INTERSECTRECT           79  //
#define FUN_INVALIDATERECT          125 //
#define FUN_INVALIDATERGN           126 //
#define FUN_INVERTRECT              82  //
#define FUN_ISCHARALPHA             433 //
#define FUN_ISCHARALPHANUMERIC      434 //
#define FUN_ISCHARLOWER             436 //
#define FUN_ISCHARUPPER             435 //
#define FUN_ISCHILD                 48  //
#define FUN_ISCLIPBOARDFORMATAVAILABLE  193 //
#define FUN_ISDIALOGMESSAGE         90  //
#define FUN_ISDLGBUTTONCHECKED      98  //
#define FUN_ISICONIC                31  //
#define FUN_ISRECTEMPTY             75  //
#define FUN_ISTWOBYTECHARPREFIX     51  // Internal, proto
#define FUN_ISUSERIDLE              333 // Internal
#define FUN_ISWINDOW                47  //
#define FUN_ISWINDOWENABLED         35  //
#define FUN_ISWINDOWVISIBLE         49  //
#define FUN_ISZOOMED                272 //
#define FUN_KILLSYSTEMTIMER         182 // Internal
#define FUN_KILLTIMER               12  //
#define FUN_KILLTIMER2              327 // Internal
#define FUN_LBOXCARETBLINKER        453 // Internal
#define FUN_LBOXCTLWNDPROC          307 // Internal
#define FUN_LOADACCELERATORS        177 //
#define FUN_LOADBITMAP              175 //
#define FUN_LOADCURSOR              173 //
#define FUN_LOADCURSORICONHANDLER   336 // Internal
#define FUN_LOADDIBCURSORHANDLER    356 // Internal
#define FUN_LOADDIBICONHANDLER      357 // Internal
#define FUN_LOADICON                174 //
#define FUN_LOADICONHANDLER         456 // Internal
#define FUN_LOADMENU                150 //
#define FUN_LOADMENUINDIRECT        220 //
#define FUN_LOADSTRING              176 //
#define FUN_LOCKMYTASK              276 // Internal
#define FUN_LOOKUPMENUHANDLE        217 // Internal
#define FUN_LSTRCMP                 430 //
#define FUN_LSTRCMPI                471 //
#define FUN_MAPDIALOGRECT           103 //
#define FUN_MB_DLGPROC              409 // Internal
#define FUN_MDICLIENTWNDPROC        444 // Internal
#define FUN_MENUITEMSTATE           329 // Internal
#define FUN_MENUWNDPROC             306 // Internal
#define FUN_MESSAGEBEEP             104 //
#define FUN_MESSAGEBOX              1   //
#define FUN_MODIFYMENU              414 //
#define FUN_MOVEWINDOW              56  //
#define FUN_OFFSETRECT              77  //
#define FUN_OLDEXITWINDOWS          2   // Internal
#define FUN_OPENCLIPBOARD           137 //
#define FUN_OPENCOMM                200 //
#define FUN_OPENICON                44  //
#define FUN_PAINTRECT               325 // Internal
#define FUN_PEEKMESSAGE             109 //
#define FUN_POSTAPPMESSAGE          116 //
#define FUN_POSTMESSAGE             110 //
#define FUN_POSTMESSAGE2            313 // Internal
#define FUN_POSTQUITMESSAGE         6   //
#define FUN_PTINRECT                76  //
#define FUN_READCOMM                204 //
#define FUN_REALIZEPALETTE          283 //
#define FUN_REGISTERCLASS           57  //
#define FUN_REGISTERCLIPBOARDFORMAT 145 //
#define FUN_REGISTERWINDOWMESSAGE   118 //
#define FUN_RELEASECAPTURE          19  //
#define FUN_RELEASEDC               68  //
#define FUN_REMOVEMENU              412 //
#define FUN_REMOVEPROP              24  //
#define FUN_REPAINTSCREEN           275 // No proto
#define FUN_REPLYMESSAGE            115 //
#define FUN_SBWNDPROC               304 // Internal
#define FUN_SCREENTOCLIENT          29  //
#define FUN_SCROLLCHILDREN          463 // Internal
#define FUN_SCROLLDC                221 //
#define FUN_SCROLLWINDOW            61  //
#define FUN_SELECTPALETTE           282 //
#define FUN_SENDDLGITEMMESSAGE      101 //
#define FUN_SENDMESSAGE             111 //
#define FUN_SENDMESSAGE2            312 // Internal
#define FUN_SETACTIVEWINDOW         59  //
#define FUN_SETCAPTURE              18  //
#define FUN_SETCARETBLINKTIME       168 //
#define FUN_SETCARETPOS             165 //
#define FUN_SETCLASSLONG            132 //
#define FUN_SETCLASSWORD            130 //
#define FUN_SETCLIPBOARDDATA        141 //
#define FUN_SETCLIPBOARDVIEWER      147 //
#define FUN_SETCOMMBREAK            210 //
#define FUN_SETCOMMEVENTMASK        208 //
#define FUN_SETCOMMSTATE            201 //
#define FUN_SETCURSOR               69  //
#define FUN_SETCURSORPOS            70  //
#define FUN_SETDESKPATTERN          279 // Internal
#define FUN_SETDESKWALLPAPER        285 // Internal
#define FUN_SETDLGITEMINT           94  //
#define FUN_SETDLGITEMTEXT          92  //
#define FUN_SETDOUBLECLICKTIME      20  //
#define FUN_SETEVENTHOOK            321 // Internal
#define FUN_SETFOCUS                22  //
#define FUN_SETGETKBDSTATE          330 // Internal
#define FUN_SETGRIDGRANULARITY      284 // Internal
#define FUN_SETINTERNALWINDOWPOS    461 // Internal
#define FUN_SETKEYBOARDSTATE        223 //
#define FUN_SETMENU                 158 //
#define FUN_SETMENUITEMBITMAPS      418 //
#define FUN_SETMESSAGEQUEUE         266 //
#define FUN_SETPARENT               233 //
#define FUN_SETPROP                 26  //
#define FUN_SETRECT                 72  //
#define FUN_SETRECTEMPTY            73  //
#define FUN_SETSCROLLPOS            62  //
#define FUN_SETSCROLLRANGE          64  //
#define FUN_SETSYSCOLORS            181 //
#define FUN_SETSYSMODALWINDOW       188 //
#define FUN_SETSYSTEMMENU           280 // Internal
#define FUN_SETSYSTEMTIMER          11  // Internal
#define FUN_SETTIMER                10  //
#define FUN_SETTIMER2               328 // Internal
#define FUN_SETWC2                  319 // Internal
#define FUN_SETWINDOWLONG           136 //
#define FUN_SETWINDOWPOS            232 //
#define FUN_SETWINDOWSHOOKINTERNAL  121 // Internal
#define FUN_SETWINDOWTEXT           37  //
#define FUN_SETWINDOWWORD           134 //
#define FUN_SHOWCARET               167 //
#define FUN_SHOWCURSOR              71  //
#define FUN_SHOWOWNEDPOPUPS         265 //
#define FUN_SHOWSCROLLBAR           267 //
#define FUN_SHOWWINDOW              42  //
#define FUN_SIGNALPROC              314 // Internal
#define FUN_SNAPWINDOW              281 // Internal
#define FUN_STATICWNDPROC           302 // Internal
#define FUN_STRINGFUNC              470 // Internal
#define FUN_SWAPMOUSEBUTTON         186 //
#define FUN_SWITCHTOTHISWINDOW      172 // Internal
#define FUN_SWITCHWNDPROC           347 // Internal
#define FUN_SYSERRORBOX             320 // Internal
#define FUN_TABBEDTEXTOUT           196 //
#define FUN_TABTHETEXTOUTFORWIMPS   354 // Internal
#define FUN_TILECHILDWINDOWS        199 // Internal
#define FUN_TITLEWNDPROC            345 // Internal
#define FUN_TRACKPOPUPMENU          416 //
#define FUN_TRANSLATEACCELERATOR    178 //
#define FUN_TRANSLATEMDISYSACCEL    451 //
#define FUN_TRANSLATEMESSAGE        113 //
#define FUN_TRANSMITCOMMCHAR        206 //
#define FUN_UNGETCOMMCHAR           212 //
#define FUN_UNHOOKWINDOWSHOOK       234 //
#define FUN_UNIONRECT               80  //
#define FUN_UNREGISTERCLASS         403 //
#define FUN_UPDATEWINDOW            124 //
#define FUN_USERYIELD               332 // Internal
#define FUN_VALIDATERECT            127 //
#define FUN_VALIDATERGN             128 //
#define FUN_WAITMESSAGE             112 //
#define FUN_WINDOWFROMPOINT         30  //
#define FUN_WINFARFRAME             340 // Internal
#define FUN_WINHELP                 171 //
#define FUN_WINOLDAPPHACKOMATIC     322 // Internal
#define FUN_WNETADDCONNECTION       517 // Internal
#define FUN_WNETBROWSEDIALOG        515 // Internal
#define FUN_WNETCANCELCONNECTION    518 // Internal
#define FUN_WNETCANCELJOB           506 // Internal
#define FUN_WNETCLOSEJOB            502 // Internal
#define FUN_WNETDEVICEMODE          514 // Internal
#define FUN_WNETGETCAPS             513 // Internal
#define FUN_WNETGETCONNECTION       512 // Internal
#define FUN_WNETGETERROR            519 // Internal
#define FUN_WNETGETERRORTEXT        520 // Internal
#define FUN_WNETGETUSER             516 // Internal
#define FUN_WNETHOLDJOB             504 // Internal
#define FUN_WNETLOCKQUEUEDATA       510 // Internal
#define FUN_WNETOPENJOB             501 // Internal
#define FUN_WNETRELEASEJOB          505 // Internal
#define FUN_WNETSETJOBCOPIES        507 // Internal
#define FUN_WNETUNLOCKQUEUEDATA     511 // Internal
#define FUN_WNETUNWATCHQUEUE        509 // Internal
#define FUN_WNETWATCHQUEUE          508 // Internal
#define FUN_WRITECOMM               205 //
#define FUN_WVSPRINTF               421 //
#define FUN_XCSTODS                 315 // Internal
#define FUN__FFFE_FARFRAME          341 // No proto
#define FUN__WSPRINTF               420 //
#define FUN_SETWINDOWSHOOKEX        291 // win31 api
#define FUN_UNHOOKWINDOWSHOOKEX     292 // win31 api
#define FUN_CALLNEXTHOOKEX          293 // win31 api
#define FUN_CLOSEDRIVER             253
#define FUN_COPYCURSOR              369
#define FUN_COPYICON                368
#define FUN_DEFDRIVERPROC           255
#define FUN_ENABLESCROLLBAR         482
#define FUN_GETCLIPCURSOR           309
#define FUN_GETCURSOR               247
#define FUN_GETDCEX                 359
#define FUN_GETDRIVERMODULEHANDLE   254
#define FUN_GETDRIVERINFO           256
#define FUN_GETFREESYSTEMRESOURCES  284
#define FUN_GETMESSAGEEXTRAINFO         288
#define FUN_GETNEXTDRIVER               257
#define FUN_GETOPENCLIPBOARDWINDOW      248
#define FUN_GETQUEUESTATUS              334
#define FUN_GETSYSTEMDEBUGSTATE         231
#define FUN_GETTIMERRESOLUTION          14
#define FUN_GETWINDOWPLACEMENT          370
#define FUN_ISMENU                      358
#define FUN_LOCKINPUT                   226
#define FUN_LOCKWINDOWUPDATE            294
#define FUN_MAPWINDOWPOINTS             258
#define FUN_OPENDRIVER                  252
#define FUN_QUERYSENDMESSAGE            184
#define FUN_REDRAWWINDOW                290
#define FUN_SCROLLWINDOWEX              319
#define FUN_SENDDRIVERMESSAGE           251
#define FUN_SETWINDOWPLACEMENT          371
#define FUN_SUBTRACTRECT                373
#define FUN_SYSTEMPARAMETERSINFO        483

#define FUN_TILECHILDWzINDOWS           199
#define FUN_USERSEEUSERDO               216
#define FUN_ENABLECOMMNOTIFICATION      245
#define FUN_EXITWINDOWSEXEC             246
#define FUN_OLDSETDESKPATTERN           279
#define FUN_OLDSETDESKWALLPAPER         285
#define FUN_KEYBD_EVENT                 289
#define FUN_MOUSE_EVENT                 299
#define FUN_BOZOSLIVEHERE               301
#define FUN_GETINTERNALICONHEADER       372
#define FUN_DLGDIRSELECTEX              422
#define FUN_DLGDIRSELECTCOMBOBOXEX      423
#define FUN_GETUSERLOCALOBJTYPE         480
#define FUN_HARDWARE_EVENT              481
#define FUN_DCHOOK                      362
#define FUN_WNETERRORTEXT               499
#define FUN_WNETABORTJOB                503
#define FUN_WNETENABLE                  521
#define FUN_WNETDISABLE                 522
#define FUN_WNETRESTORECONNECTION       523
#define FUN_WNETWRITEJOB                524
#define FUN_WNETCONNECTDIALOG           525
#define FUN_WNETDISCONNECTDIALOG        526
#define FUN_WNETCONNECTIONDIALOG        527
#define FUN_WNETVIEWQUEUEDIALOG         528
#define FUN_WNETPROPERTYDIALOG          529
#define FUN_WNETGETDIRECTORYTYPE        530
#define FUN_WNETDIRECTORYNOTIFY         531
#define FUN_WNETGETPROPERTYTEXT         532

/* New in Win95 user16 */

#define FUN_ACTIVATEKEYBOARDLAYOUT      562  // export 650
#define FUN_BROADCASTSYSTEMMESSAGE      554  // export 604
#define FUN_CALLMSGFILTER32             589  // export 823
#define FUN_CASCADEWINDOWS              429
#define FUN_CHANGEDISPLAYSETTINGS       557  // export 620
#define FUN_CHECKMENURADIOITEM          576  // export 666
#define FUN_CHILDWINDOWFROMPOINTEX      399
#define FUN_CHOOSECOLOR_CALLBACK16      584  // export 804
#define FUN_CHOOSEFONT_CALLBACK16       580  // export 800
#define FUN_COPYIMAGE                   390
#define FUN_CREATEICONFROMRESOURCEEX    450
#define FUN_DESTROYICON32               553  // export 610
#define FUN_DISPATCHINPUT               569  // export 658
#define FUN_DISPATCHMESSAGE32           588  // export 822
#define FUN_DLLENTRYPOINT               374
#define FUN_DOHOTKEYSTUFF               541  // export 601, export 541 NewSignalProc not thunked
#define FUN_DRAWANIMATEDRECTS           448
#define FUN_DRAWCAPTION                 571  // export 660
#define FUN_DRAWCAPTIONTEMP             568  // export 657
#define FUN_DRAWEDGE                    570  // export 659
#define FUN_DRAWFRAMECONTROL            567  // export 656
#define FUN_DRAWICONEX                  394
#define FUN_DRAWMENUBARTEMP             573  // export 662
#define FUN_DRAWSTATE                   449
#define FUN_DRAWTEXTEX                  375
#define FUN_ENUMDISPLAYSETTINGS         560  // export 621
#define FUN_FINDREPLACE_CALLBACK16      581  // export 801
#define FUN_FINDWINDOWEX                427
#define FUN_FORMATMESSAGE               556  // export 606
#define FUN_GETAPPVER                   498
#define FUN_GETCLASSINFOEX              398
#define FUN_GETFOREGROUNDWINDOW         558  // export 608
#define FUN_GETICONINFO                 395
#define FUN_GETKEYBOARDLAYOUT           563  // export 651
#define FUN_GETKEYBOARDLAYOUTLIST       564  // export 652
#define FUN_GETKEYBOARDLAYOUTNAME       477
#define FUN_GETMENUCONTEXTHELPID        385
#define FUN_GETMENUDEFAULTITEM          574  // export 663
#define FUN_GETMENUITEMINFO             443
#define FUN_GETMENUITEMRECT             575  // export 665
#define FUN_GETMESSAGE32                586  // export 820
#define FUN_GETPROPEX                   379
#define FUN_GETSCROLLINFO               476
#define FUN_GETSHELLWINDOW              540  // export 600
#define FUN_GETSYSCOLORBRUSH            281
#define FUN_GETWINDOWCONTEXTHELPID      383
#define FUN_GETWINDOWRGN                579  // export 669
#define FUN_HACKTASKMONITOR             555  // export 605
#define FUN_INITTHREADINPUT             409
#define FUN_INSERTMENUITEM              441
#define FUN_INSTALLIMT                  594  // export 890
#define FUN_ISDIALOGMESSAGE32           590  // export 824
#define FUN_LOADIMAGE                   389
#define FUN_LOADKEYBOARDLAYOUT          478
#define FUN_LOOKUPICONIDFROMDIRECTORYEX 364
#define FUN_MENUITEMFROMPOINT           479
#define FUN_MESSAGEBOXINDIRECT          593  // export 827
#define FUN_MSGWAITFORMULTIPLEOBJECTS   561  // export 640
#define FUN_OPENFILENAME_CALLBACK16     582  // export 802
#define FUN_PEEKMESSAGE32               585  // export 819
#define FUN_PLAYSOUNDEVENT              8
#define FUN_POSTMESSAGE32               591  // export 825
#define FUN_POSTPOSTEDMESSAGES          566  // export 655
#define FUN_POSTTHREADMESSAGE32         592  // export 826
#define FUN_PRINTDLG_CALLBACK16         583  // export 803
#define FUN_REGISTERCLASSEX             397
#define FUN_REMOVEPROPEX                380
#define FUN_SETCHECKCURSORTIMER         542  // export 602
#define FUN_SETFOREGROUNDWINDOW         559  // export 609
#define FUN_SETMENUCONTEXTHELPID        384
#define FUN_SETMENUDEFAULTITEM          543  // export 664
#define FUN_SETMENUITEMINFO             446
#define FUN_SETMESSAGEEXTRAINFO         376
#define FUN_SETPROPEX                   378
#define FUN_SETSCROLLINFO               475
#define FUN_SETSYSCOLORSTEMP            572  // export 661
#define FUN_SETWINDOWCONTEXTHELPID      382
#define FUN_SETWINDOWRGN                578  // export 668
#define FUN_SIGNALPROC32                391
#define FUN_TILEWINDOWS                 428
#define FUN_TRACKPOPUPMENUEX            577  // export 667
#define FUN_TRANSLATEMESSAGE32          587  // export 821
#define FUN_UNINSTALLIMT                595  // export 891
#define FUN_UNLOADINSTALLABLEDRIVERS    300
#define FUN_UNLOADKEYBOARDLAYOUT        565  // export 654
#define FUN_WINDOWFROMDC                117
#define FUN_WNETINITIALIZE              533
#define FUN_WNETLOGON                   534


/* WOW private thunks in USER */

#define FUN_NOTIFYWOW                   535
#define FUN_DEFDLGPROCTHUNK             536  // used by walias.c, not in thunk table
#define FUN_WOWWORDBREAKPROC            537
#define FUN_MOUSEEVENT                  538
#define FUN_KEYBDEVENT                  539

/* NotifyWOW ID's */
#define NW_LOADICON          1
#define NW_LOADCURSOR        2
#define NW_LOADACCELERATORS  3
#define NW_FINALUSERINIT     4
#define NW_KRNL386SEGS       5
#define NW_WINHELP           6


/* XLATOFF */
#pragma pack(2)
/* XLATON */

typedef struct _ADJUSTWINDOWRECT16 {        /* u102 */
    BOOL16  f3;
    LONG    f2;
    VPRECT16 f1;
} ADJUSTWINDOWRECT16;
typedef ADJUSTWINDOWRECT16 UNALIGNED *PADJUSTWINDOWRECT16;

typedef struct _ADJUSTWINDOWRECTEX16 {      /* u454 */
    DWORD   f4;
    BOOL16  f3;
    LONG    f2;
    VPRECT16 f1;
} ADJUSTWINDOWRECTEX16;
typedef ADJUSTWINDOWRECTEX16 UNALIGNED *PADJUSTWINDOWRECTEX16;

typedef struct _ANSILOWER16 {           /* u432 */
    VPSTR   f1;
} ANSILOWER16;
typedef ANSILOWER16 UNALIGNED *PANSILOWER16;

typedef struct _ANSILOWERBUFF16 {       /* u438 */
    WORD    f2;
    VPSTR   f1;
} ANSILOWERBUFF16;
typedef ANSILOWERBUFF16 UNALIGNED *PANSILOWERBUFF16;

typedef struct _ANSINEXT16 {            /* u472 */
    VPSTR   f1;
} ANSINEXT16;
typedef ANSINEXT16 UNALIGNED *PANSINEXT16;

typedef struct _ANSIPREV16 {            /* u473 */
    VPSTR   f2;
    VPSTR   f1;
} ANSIPREV16;
typedef ANSIPREV16 UNALIGNED *PANSIPREV16;

typedef struct _ANSIUPPER16 {           /* u431 */
    VPSTR   f1;
} ANSIUPPER16;
typedef ANSIUPPER16 UNALIGNED *PANSIUPPER16;

typedef struct _ANSIUPPERBUFF16 {       /* u437 */
    WORD    f2;
    VPSTR   f1;
} ANSIUPPERBUFF16;
typedef ANSIUPPERBUFF16 UNALIGNED *PANSIUPPERBUFF16;

#ifdef NULLSTRUCT
typedef struct _ANYPOPUP16 {            /* u52 */
} ANYPOPUP16;
typedef ANYPOPUP16 UNALIGNED *PANYPOPUP16;
#endif

typedef struct _APPENDMENU16 {          /* u411 */
    VPSTR   f4;
    WORD    f3;
    WORD    f2;
    HMENU16 f1;
} APPENDMENU16;
typedef APPENDMENU16 UNALIGNED *PAPPENDMENU16;

typedef struct _ARRANGEICONICWINDOWS16 {    /* u170 */
    HWND16  hwnd;
} ARRANGEICONICWINDOWS16;
typedef ARRANGEICONICWINDOWS16 UNALIGNED *PARRANGEICONICWINDOWS16;

typedef struct _BEGINDEFERWINDOWPOS16 {     /* u259 */
    SHORT   f1;
} BEGINDEFERWINDOWPOS16;
typedef BEGINDEFERWINDOWPOS16 UNALIGNED *PBEGINDEFERWINDOWPOS16;

typedef struct _BEGINPAINT16 {          /* u39 */
    VPPAINTSTRUCT16 vpPaint;
    HWND16  hwnd;
} BEGINPAINT16;
typedef BEGINPAINT16 UNALIGNED *PBEGINPAINT16;

typedef struct _BRINGWINDOWTOTOP16 {        /* u45 */
    HWND16  f1;
} BRINGWINDOWTOTOP16;
typedef BRINGWINDOWTOTOP16 UNALIGNED *PBRINGWINDOWTOTOP16;

typedef struct _BROADCASTMESSAGE16 {    /* u355 */
    LONG    f4;
    WORD    f3;
    WORD    f2;
    HWND16  f1;
} BROADCASTMESSAGE16;
typedef BROADCASTMESSAGE16 UNALIGNED *PBROADCASTMESSAGE16;

typedef struct _BUILDCOMMDCB16 {        /* u213 */
    VPDCB16 f2;
    VPSTR   f1;
} BUILDCOMMDCB16;
typedef BUILDCOMMDCB16 UNALIGNED *PBUILDCOMMDCB16;

typedef struct _CALCCHILDSCROLL16 {     /* u462 */
    WORD    f2;
    HWND16  f1;
} CALCCHILDSCROLL16;
typedef CALCCHILDSCROLL16 UNALIGNED *PCALCCHILDSCROLL16;

typedef struct _CALLMSGFILTER16 {       /* u123 */
    SHORT   f2;
    VPMSG16 f1;
} CALLMSGFILTER16;
typedef CALLMSGFILTER16 UNALIGNED *PCALLMSGFILTER16;

typedef struct _CALLWINDOWPROC16 {      /* u122 */
    LONG    f5;
    WORD    f4;
    WORD    f3;
    HWND16  f2;
    VPPROC f1;
} CALLWINDOWPROC16;
typedef CALLWINDOWPROC16 UNALIGNED *PCALLWINDOWPROC16;

typedef struct _CARETBLINKPROC16 {      /* u311 */
    DWORD   f4;
    WORD    f3;
    WORD    f2;
    HWND16  f1;
} CARETBLINKPROC16;
typedef CARETBLINKPROC16 UNALIGNED *PCARETBLINKPROC16;

typedef struct _CASCADECHILDWINDOWS16 { /* u198 */
    WORD    f2;
    HWND16  f1;
} CASCADECHILDWINDOWS16;
typedef CASCADECHILDWINDOWS16 UNALIGNED *PCASCADECHILDWINDOWS16;

typedef struct _CHANGECLIPBOARDCHAIN16 {    /* u149 */
    HWND16  f2;
    HWND16  f1;
} CHANGECLIPBOARDCHAIN16;
typedef CHANGECLIPBOARDCHAIN16 UNALIGNED *PCHANGECLIPBOARDCHAIN16;

typedef struct _CHANGEMENU16 {          /* u153 */
    WORD    f5;
    WORD    f4;
    VPSTR   f3;
    WORD    f2;
    HMENU16 f1;
} CHANGEMENU16;
typedef CHANGEMENU16 UNALIGNED *PCHANGEMENU16;

typedef struct _CHECKDLGBUTTON16 {      /* u97 */
    WORD    f3;
    SHORT   f2;
    HWND16  f1;
} CHECKDLGBUTTON16;
typedef CHECKDLGBUTTON16 UNALIGNED *PCHECKDLGBUTTON16;

typedef struct _CHECKMENUITEM16 {       /* u154 */
    WORD    f3;
    WORD    f2;
    HMENU16 f1;
} CHECKMENUITEM16;
typedef CHECKMENUITEM16 UNALIGNED *PCHECKMENUITEM16;

typedef struct _CHECKRADIOBUTTON16 {        /* u96 */
    SHORT   f4;
    SHORT   f3;
    SHORT   f2;
    HWND16  f1;
} CHECKRADIOBUTTON16;
typedef CHECKRADIOBUTTON16 UNALIGNED *PCHECKRADIOBUTTON16;

typedef struct _CHILDWINDOWFROMPOINT16 {    /* u191 */
    POINT16 f2;
    HWND16  f1;
} CHILDWINDOWFROMPOINT16;
typedef CHILDWINDOWFROMPOINT16 UNALIGNED *PCHILDWINDOWFROMPOINT16;

typedef struct _CLEARCOMMBREAK16 {      /* u211 */
    SHORT   f1;
} CLEARCOMMBREAK16;
typedef CLEARCOMMBREAK16 UNALIGNED *PCLEARCOMMBREAK16;

typedef struct _CLIENTTOSCREEN16 {      /* u28 */
    VPPOINT16 f2;
    HWND16  f1;
} CLIENTTOSCREEN16;
typedef CLIENTTOSCREEN16 UNALIGNED *PCLIENTTOSCREEN16;

typedef struct _CLIPCURSOR16 {          /* u16 */
    VPRECT16 f1;
} CLIPCURSOR16;
typedef CLIPCURSOR16 UNALIGNED *PCLIPCURSOR16;

#ifdef NULLSTRUCT
typedef struct _CLOSECLIPBOARD16 {      /* u138 */
} CLOSECLIPBOARD16;
typedef CLOSECLIPBOARD16 UNALIGNED *PCLOSECLIPBOARD16;
#endif

typedef struct _CLOSECOMM16 {           /* u207 */
    VPDWORD f2;  /* added for SetCommEventMask() support */
    SHORT   f1;
} CLOSECOMM16;
typedef CLOSECOMM16 UNALIGNED *PCLOSECOMM16;

typedef struct _CLOSEWINDOW16 {         /* u43 */
    HWND16  f1;
} CLOSEWINDOW16;
typedef CLOSEWINDOW16 UNALIGNED *PCLOSEWINDOW16;

typedef struct _COMPUPDATERECT16 {      /* u316 */
    WORD     f4;
    BOOL16   f3;
    VPRECT16 f2;
    HWND16   f1;
} COMPUPDATERECT16;
typedef COMPUPDATERECT16 UNALIGNED *PCOMPUPDATERECT16;

typedef struct _COMPUPDATERGN16 {       /* u317 */
    WORD     f4;
    BOOL16   f3;
    HRGN16   f2;
    HWND16   f1;
} COMPUPDATERGN16;
typedef COMPUPDATERGN16 UNALIGNED *PCOMPUPDATERGN16;

typedef struct _CONTROLPANELINFO16 {    /* u273 */
    VPVOID  f3;
    WORD    f2;
    WORD    f1;
} CONTROLPANELINFO16;
typedef CONTROLPANELINFO16 UNALIGNED *PCONTROLPANELINFO16;

typedef struct _CONTSCROLL16 {          /* u310 */
    DWORD   f4;
    WORD    f3;
    WORD    f2;
    HWND16  f1;
} CONTSCROLL16;
typedef CONTSCROLL16 UNALIGNED *PCONTSCROLL16;

typedef struct _COPYRECT16 {            /* u74 */
    VPRECT16 f2;
    VPRECT16 f1;
} COPYRECT16;
typedef COPYRECT16 UNALIGNED *PCOPYRECT16;

#ifdef NULLSTRUCT
typedef struct _COUNTCLIPBOARDFORMATS16 {   /* u143 */
} COUNTCLIPBOARDFORMATS16;
typedef COUNTCLIPBOARDFORMATS16 UNALIGNED *PCOUNTCLIPBOARDFORMATS16;
#endif

typedef struct _CREATECARET16 {         /* u163 */
    SHORT   f4;
    SHORT   f3;
    HBM16   f2;
    HWND16  f1;
} CREATECARET16;
typedef CREATECARET16 UNALIGNED *PCREATECARET16;

typedef struct _CREATECURSOR16 {        /* u406 */
    VPSTR   f7;
    VPSTR   f6;
    SHORT   f5;
    SHORT   f4;
    SHORT   f3;
    SHORT   f2;
    HAND16  f1;
} CREATECURSOR16;
typedef CREATECURSOR16 UNALIGNED *PCREATECURSOR16;

typedef struct _CREATECURSORICONINDIRECT16 { /* u408 */
    VPSTR   f4;
    VPSTR   f3;
    VPSTR   f2;
    HAND16  f1;
} CREATECURSORICONINDIRECT16;
typedef CREATECURSORICONINDIRECT16 UNALIGNED *PCREATECURSORICONINDIRECT16;

typedef struct _CREATEDIALOG16 {        /* u89 */
    VPPROC  f4;
    HWND16  f3;
    VPSTR   f2;
    HAND16  f1;
} CREATEDIALOG16;
typedef CREATEDIALOG16 UNALIGNED *PCREATEDIALOG16;

typedef struct _CREATEDIALOGINDIRECT16 {    /* u219 */
    VPPROC f4;
    HWND16  f3;
    VPSTR   f2;
    HAND16  f1;
} CREATEDIALOGINDIRECT16;
typedef CREATEDIALOGINDIRECT16 UNALIGNED *PCREATEDIALOGINDIRECT16;

typedef struct _CREATEDIALOGINDIRECTPARAM16 {   /* u242 */
    LONG    f5;
    VPPROC f4;
    HWND16  f3;
    VPSTR   f2;
    HAND16  f1;
} CREATEDIALOGINDIRECTPARAM16;
typedef CREATEDIALOGINDIRECTPARAM16 UNALIGNED *PCREATEDIALOGINDIRECTPARAM16;

typedef struct _CREATEDIALOGPARAM16 {       /* u241 */
    DWORD   f6;
    LONG    f5;
    VPPROC  f4;
    HWND16  f3;
    VPSTR   f2;
    HAND16  f1;
} CREATEDIALOGPARAM16;
typedef CREATEDIALOGPARAM16 UNALIGNED *PCREATEDIALOGPARAM16;

typedef struct _CREATEICON16 {          /* u407 */
    VPSTR   f7;
    VPSTR   f6;
    WORD    f5;
    WORD    f4;
    SHORT   f3;
    SHORT   f2;
    HAND16  f1;
} CREATEICON16;
typedef CREATEICON16 UNALIGNED *PCREATEICON16;

#ifdef NULLSTRUCT
typedef struct _CREATEMENU16 {          /* u151 */
} CREATEMENU16;
typedef CREATEMENU16 UNALIGNED *PCREATEMENU16;
#endif

#ifdef NULLSTRUCT
typedef struct _CREATEPOPUPMENU16 {     /* u415 */
} CREATEPOPUPMENU16;
typedef CREATEPOPUPMENU16 UNALIGNED *PCREATEPOPUPMENU16;
#endif

typedef struct _CREATEWINDOW16 {        /* u41 */
    VPBYTE  vpParam;
    HAND16  hInstance;
    HMENU16 hMenu;
    HWND16  hwndParent;
    SHORT   cy;
    SHORT   cx;
    SHORT   y;
    SHORT   x;
    DWORD   dwStyle;
    VPSTR   vpszWindow;
    VPSTR   vpszClass;
} CREATEWINDOW16;
typedef CREATEWINDOW16 UNALIGNED *PCREATEWINDOW16;

typedef struct _CREATEWINDOWEX16 {      /* u452 */
    VPSTR   f12;
    HAND16  f11;
    HMENU16 f10;
    HWND16  f9;
    SHORT   f8;
    SHORT   f7;
    SHORT   f6;
    SHORT   f5;
    DWORD   f4;
    VPSTR   f3;
    VPSTR   f2;
    DWORD   f1;
} CREATEWINDOWEX16;
typedef CREATEWINDOWEX16 UNALIGNED *PCREATEWINDOWEX16;

typedef struct _DCHOOK16 {              /* u362 */
    DWORD   f4;
    DWORD   f3;
    WORD    f2;
    HDC16   f1;
} DCHOOK16;
typedef DCHOOK16 UNALIGNED *PDCHOOK16;

typedef struct _DEFDLGPROC16 {          /* u308 */
    LONG    f4;
    WORD    f3;
    WORD    f2;
    HWND16  f1;
} DEFDLGPROC16;
typedef DEFDLGPROC16 UNALIGNED *PDEFDLGPROC16;

typedef struct _DEFERWINDOWPOS16 {      /* u260 */
    WORD    f8;
    SHORT   f7;
    SHORT   f6;
    SHORT   f5;
    SHORT   f4;
    HWND16  f3;
    HWND16  f2;
    HAND16  f1;
} DEFERWINDOWPOS16;
typedef DEFERWINDOWPOS16 UNALIGNED *PDEFERWINDOWPOS16;

typedef struct _DEFFRAMEPROC16 {        /* u445 */
    LONG    f5;
    WORD    f4;
    WORD    f3;
    HWND16  f2;
    HWND16  f1;
} DEFFRAMEPROC16;
typedef DEFFRAMEPROC16 UNALIGNED *PDEFFRAMEPROC16;

typedef struct _DEFHOOKPROC16 {         /* u235 */
    VPPROC f4;
    DWORD   f3;
    WORD    f2;
    SHORT   f1;
} DEFHOOKPROC16;
typedef DEFHOOKPROC16 UNALIGNED *PDEFHOOKPROC16;

typedef struct _DEFMDICHILDPROC16 {     /* u447 */
    LONG    f4;
    WORD    f3;
    WORD    f2;
    HWND16  f1;
} DEFMDICHILDPROC16;
typedef DEFMDICHILDPROC16 UNALIGNED *PDEFMDICHILDPROC16;

typedef struct _DEFWINDOWPROC16 {       /* u107 */
    LONG    lParam;
    WORD    wParam;
    WORD    wMsg;
    HWND16  hwnd;
} DEFWINDOWPROC16;
typedef DEFWINDOWPROC16 UNALIGNED *PDEFWINDOWPROC16;

typedef struct _DELETEMENU16 {          /* u413 */
    WORD    f3;
    WORD    f2;
    HMENU16 f1;
} DELETEMENU16;
typedef DELETEMENU16 UNALIGNED *PDELETEMENU16;

typedef struct _DESKTOPWNDPROC16 {      /* u305 */
    LONG    f4;
    WORD    f3;
    WORD    f2;
    HWND16  f1;
} DESKTOPWNDPROC16;
typedef DESKTOPWNDPROC16 UNALIGNED *PDESKTOPWNDPROC16;

#ifdef NULLSTRUCT
typedef struct _DESTROYCARET16 {        /* u164 */
} DESTROYCARET16;
typedef DESTROYCARET16 UNALIGNED *PDESTROYCARET16;
#endif

typedef struct _DESTROYCURSOR16 {       /* u458 */
    HCUR16  f1;
} DESTROYCURSOR16;
typedef DESTROYCURSOR16 UNALIGNED *PDESTROYCURSOR16;

typedef struct _DESTROYICON16 {         /* u457 */
    HICON16 f1;
} DESTROYICON16;
typedef DESTROYICON16 UNALIGNED *PDESTROYICON16;

typedef struct _DESTROYMENU16 {         /* u152 */
    HMENU16 f1;
} DESTROYMENU16;
typedef DESTROYMENU16 UNALIGNED *PDESTROYMENU16;

typedef struct _DESTROYWINDOW16 {       /* u53 */
    HWND16 f1;
} DESTROYWINDOW16;
typedef DESTROYWINDOW16 UNALIGNED *PDESTROYWINDOW16;

typedef struct _DIALOGBOX16 {           /* u87 */
    VPPROC f4;
    HWND16  f3;
    VPSTR   f2;
    HAND16  f1;
} DIALOGBOX16;
typedef DIALOGBOX16 UNALIGNED *PDIALOGBOX16;

typedef struct _DIALOGBOXINDIRECT16 {       /* u218 */
    VPPROC f4;
    HWND16  f3;
    HAND16  f2;
    HAND16  f1;
} DIALOGBOXINDIRECT16;
typedef DIALOGBOXINDIRECT16 UNALIGNED *PDIALOGBOXINDIRECT16;

typedef struct _DIALOGBOXINDIRECTPARAM16 {  /* u240 */
    LONG    f5;
    VPPROC f4;
    HWND16  f3;
    HAND16  f2;
    HAND16  f1;
} DIALOGBOXINDIRECTPARAM16;
typedef DIALOGBOXINDIRECTPARAM16 UNALIGNED *PDIALOGBOXINDIRECTPARAM16;

typedef struct _DIALOGBOXPARAM16 {      /* u239 */
    WORD    f7;
    DWORD   f6;
    LONG    f5;
    VPPROC  f4;
    HWND16  f3;
    VPSTR   f2;
    HAND16  f1;
} DIALOGBOXPARAM16;
typedef DIALOGBOXPARAM16 UNALIGNED *PDIALOGBOXPARAM16;

#ifdef NULLSTRUCT
typedef struct _DISABLEOEMLAYER16 {     /* u4 */
} DISABLEOEMLAYER16;
typedef DISABLEOEMLAYER16 UNALIGNED *PDISABLEOEMLAYER16;
#endif

typedef struct _DISPATCHMESSAGE16 {     /* u114 */
    VPMSG16 f1;
} DISPATCHMESSAGE16;
typedef DISPATCHMESSAGE16 UNALIGNED *PDISPATCHMESSAGE16;

typedef struct _DLGDIRLIST16 {          /* u100 */
    WORD    f5;
    SHORT   f4;
    SHORT   f3;
    VPSTR   f2;
    HWND16  f1;
} DLGDIRLIST16;
typedef DLGDIRLIST16 UNALIGNED *PDLGDIRLIST16;

typedef struct _DLGDIRLISTCOMBOBOX16 {      /* u195 */
    WORD    f5;
    SHORT   f4;
    SHORT   f3;
    VPSTR   f2;
    HWND16  f1;
} DLGDIRLISTCOMBOBOX16;
typedef DLGDIRLISTCOMBOBOX16 UNALIGNED *PDLGDIRLISTCOMBOBOX16;

typedef struct _DLGDIRSELECT16 {        /* u99 */
    SHORT   f3;
    VPSTR   f2;
    HWND16  f1;
} DLGDIRSELECT16;
typedef DLGDIRSELECT16 UNALIGNED *PDLGDIRSELECT16;

typedef struct _DLGDIRSELECTCOMBOBOX16 {    /* u194 */
    SHORT   f3;
    VPSTR   f2;
    HWND16  f1;
} DLGDIRSELECTCOMBOBOX16;
typedef DLGDIRSELECTCOMBOBOX16 UNALIGNED *PDLGDIRSELECTCOMBOBOX16;

typedef struct _DLGDIRSELECTEX16 {  /* u422 */
    SHORT   f4;
    SHORT   f3;
    VPSTR   f2;
    HWND16  f1;
} DLGDIRSELECTEX16;
typedef DLGDIRSELECTEX16 UNALIGNED *PDLGDIRSELECTEX16;

typedef struct _DLGDIRSELECTCOMBOBOXEX16 {    /* u423 */
    SHORT   f4;
    SHORT   f3;
    VPSTR   f2;
    HWND16  f1;
} DLGDIRSELECTCOMBOBOXEX16;
typedef DLGDIRSELECTCOMBOBOXEX16 UNALIGNED *PDLGDIRSELECTCOMBOBOXEX16;

typedef struct _DRAGDETECT16 {          /* u465 */
    POINT16 pt;
    HWND16  hwnd;
} DRAGDETECT16;
typedef DRAGDETECT16 UNALIGNED *PDRAGDETECT16;

typedef struct _DRAGOBJECT16 {          /* u464 */
    HAND16  f5;
    LONG    f4;
    WORD    f3;
    HWND16  f2;
    HWND16  f1;
} DRAGOBJECT16;
typedef DRAGOBJECT16 UNALIGNED *PDRAGOBJECT16;

typedef struct _DRAWFOCUSRECT16 {       /* u466 */
    VPRECT16 f2;
    HDC16   f1;
} DRAWFOCUSRECT16;
typedef DRAWFOCUSRECT16 UNALIGNED *PDRAWFOCUSRECT16;

typedef struct _DRAWICON16 {            /* u84 */
    HICON16 f4;
    SHORT   f3;
    SHORT   f2;
    HDC16   f1;
} DRAWICON16;
typedef DRAWICON16 UNALIGNED *PDRAWICON16;

typedef struct _DRAWMENUBAR16 {         /* u160 */
    HWND16  f1;
} DRAWMENUBAR16;
typedef DRAWMENUBAR16 UNALIGNED *PDRAWMENUBAR16;

typedef struct _DRAWTEXT16 {            /* u85 */
    WORD     wFormat;
    VPRECT16 vpRect;
    SHORT    nCount;
    VPSTR    vpString;
    HDC16    hdc;
} DRAWTEXT16;
typedef DRAWTEXT16 UNALIGNED *PDRAWTEXT16;

typedef struct _DUMPICON16 {            /* u459 */
    VPSTR   f4;
    VPSTR   f3;
    VPWORD  f2;
    VPSTR   f1;
} DUMPICON16;
typedef DUMPICON16 UNALIGNED *PDUMPICON16;

#ifdef NULLSTRUCT
typedef struct _EMPTYCLIPBOARD16 {      /* u139 */
} EMPTYCLIPBOARD16;
typedef EMPTYCLIPBOARD16 UNALIGNED *PEMPTYCLIPBOARD16;
#endif

typedef struct _ENABLECOMMNOTIFICATION16 {     /* u245 */
    SHORT   f4;
    SHORT   f3;
    HWND16  f2;
    SHORT   f1;
} ENABLECOMMNOTIFICATION16;
typedef ENABLECOMMNOTIFICATION16 UNALIGNED *PENABLECOMMNOTIFICATION16;


typedef struct _ENABLEHARDWAREINPUT16 {     /* u331 */
    BOOL16  f1;
} ENABLEHARDWAREINPUT16;
typedef ENABLEHARDWAREINPUT16 UNALIGNED *PENABLEHARDWAREINPUT16;

typedef struct _ENABLEMENUITEM16 {      /* u155 */
    WORD    f3;
    WORD    f2;
    HMENU16 f1;
} ENABLEMENUITEM16;
typedef ENABLEMENUITEM16 UNALIGNED *PENABLEMENUITEM16;

#ifdef NULLSTRUCT
typedef struct _ENABLEOEMLAYER16 {      /* u3 */
} ENABLEOEMLAYER16;
typedef ENABLEOEMLAYER16 UNALIGNED *PENABLEOEMLAYER16;
#endif

typedef struct _ENABLEWINDOW16 {        /* u34 */
    BOOL16  f2;
    HWND16  f1;
} ENABLEWINDOW16;
typedef ENABLEWINDOW16 UNALIGNED *PENABLEWINDOW16;

typedef struct _ENDDEFERWINDOWPOS16 {       /* u261 */
    HAND16  f1;
} ENDDEFERWINDOWPOS16;
typedef ENDDEFERWINDOWPOS16 UNALIGNED *PENDDEFERWINDOWPOS16;

typedef struct _ENDDIALOG16 {           /* u88 */
    SHORT   f2;
    HWND16  f1;
} ENDDIALOG16;
typedef ENDDIALOG16 UNALIGNED *PENDDIALOG16;

#ifdef NULLSTRUCT
typedef struct _ENDMENU16 {             /* u187 */
} ENDMENU16;
typedef ENDMENU16 UNALIGNED *PENDMENU16;
#endif

typedef struct _ENDPAINT16 {            /* u40 */
    VPPAINTSTRUCT16 vpPaint;
    HWND16  hwnd;
} ENDPAINT16;
typedef ENDPAINT16 UNALIGNED *PENDPAINT16;

typedef struct _ENUMCHILDWINDOWS16 {        /* u55 */
    LONG    f3;
    VPPROC f2;
    HWND16  f1;
} ENUMCHILDWINDOWS16;
typedef ENUMCHILDWINDOWS16 UNALIGNED *PENUMCHILDWINDOWS16;

typedef struct _ENUMCLIPBOARDFORMATS16 {    /* u144 */
    WORD    f1;
} ENUMCLIPBOARDFORMATS16;
typedef ENUMCLIPBOARDFORMATS16 UNALIGNED *PENUMCLIPBOARDFORMATS16;

typedef struct _ENUMPROPS16 {           /* u27 */
    VPPROC f2;
    HWND16 f1;
} ENUMPROPS16;
typedef ENUMPROPS16 UNALIGNED *PENUMPROPS16;

typedef struct _ENUMTASKWINDOWS16 {     /* u225 */
    LONG    f3;
    VPPROC f2;
    HAND16  f1;
} ENUMTASKWINDOWS16;
typedef ENUMTASKWINDOWS16 UNALIGNED *PENUMTASKWINDOWS16;

typedef struct _ENUMWINDOWS16 {         /* u54 */
    LONG    f2;
    VPPROC f1;
} ENUMWINDOWS16;
typedef ENUMWINDOWS16 UNALIGNED *PENUMWINDOWS16;

typedef struct _EQUALRECT16 {           /* u244 */
    VPRECT16 f2;
    VPRECT16 f1;
} EQUALRECT16;
typedef EQUALRECT16 UNALIGNED *PEQUALRECT16;

typedef struct _ESCAPECOMMFUNCTION16 {      /* u214 */
    SHORT   f2;
    SHORT   f1;
} ESCAPECOMMFUNCTION16;
typedef ESCAPECOMMFUNCTION16 UNALIGNED *PESCAPECOMMFUNCTION16;

typedef struct _EXCLUDEUPDATERGN16 {        /* u238 */
    HWND16  f2;
    HDC16   f1;
} EXCLUDEUPDATERGN16;
typedef EXCLUDEUPDATERGN16 UNALIGNED *PEXCLUDEUPDATERGN16;

typedef struct _EXITWINDOWS16 {         /* u7 */
    WORD    wReturnCode;
    DWORD   dwReserved;
} EXITWINDOWS16;
typedef EXITWINDOWS16 UNALIGNED *PEXITWINDOWS16;

typedef struct _EXITWINDOWSEXEC16 {     /* u246 */
    VPSTR   vpCmdLine;
    VPSTR   vpProgName;
} EXITWINDOWSEXEC16;
typedef EXITWINDOWSEXEC16 UNALIGNED *PEXITWINDOWSEXEC16;

#ifdef NULLSTRUCT
typedef struct _FARCALLNETDRIVER16 {    /* u500 */
} FARCALLNETDRIVER16;
typedef FARCALLNETDRIVER16 UNALIGNED *PFARCALLNETDRIVER16;
#endif

typedef struct _FILEPORTDLGPROC16 {     /* u346 */
    LONG    f4;
    WORD    f3;
    WORD    f2;
    HWND16  f1;
} FILEPORTDLGPROC16;
typedef FILEPORTDLGPROC16 UNALIGNED *PFILEPORTDLGPROC16;

typedef struct _FILLRECT16 {            /* u81 */
    HBRSH16 f3;
    VPRECT16 f2;
    HDC16 f1;
} FILLRECT16;
typedef FILLRECT16 UNALIGNED *PFILLRECT16;

#ifdef NULLSTRUCT
typedef struct _FINALUSERINIT16 {       /* u400 */
} FINALUSERINIT16;
typedef FINALUSERINIT16 UNALIGNED *PFINALUSERINIT16;
#endif

typedef struct _FINDWINDOW16 {          /* u50 */
    VPSTR   f2;
    VPSTR   f1;
} FINDWINDOW16;
typedef FINDWINDOW16 UNALIGNED *PFINDWINDOW16;

typedef struct _FLASHWINDOW16 {         /* u105 */
    BOOL16  f2;
    HWND16  f1;
} FLASHWINDOW16;
typedef FLASHWINDOW16 UNALIGNED *PFLASHWINDOW16;

typedef struct _FLUSHCOMM16 {           /* u215 */
    SHORT   f2;
    SHORT   f1;
} FLUSHCOMM16;
typedef FLUSHCOMM16 UNALIGNED *PFLUSHCOMM16;

typedef struct _FRAMERECT16 {           /* u83 */
    HBRSH16 f3;
    VPRECT16 f2;
    HDC16 f1;
} FRAMERECT16;
typedef FRAMERECT16 UNALIGNED *PFRAMERECT16;

#ifdef NULLSTRUCT
typedef struct _GETACTIVEWINDOW16 {     /* u60 */
} GETACTIVEWINDOW16;
typedef GETACTIVEWINDOW16 UNALIGNED *PGETACTIVEWINDOW16;
#endif

typedef struct _GETASYNCKEYSTATE16 {        /* u249 */
    SHORT   f1;
} GETASYNCKEYSTATE16;
typedef GETASYNCKEYSTATE16 UNALIGNED *PGETASYNCKEYSTATE16;

#ifdef NULLSTRUCT
typedef struct _GETCAPTURE16 {          /* u236 */
} GETCAPTURE16;
typedef GETCAPTURE16 UNALIGNED *PGETCAPTURE16;
#endif

#ifdef NULLSTRUCT
typedef struct _GETCARETBLINKTIME16 {       /* u169 */
} GETCARETBLINKTIME16;
typedef GETCARETBLINKTIME16 UNALIGNED *PGETCARETBLINKTIME16;
#endif

typedef struct _GETCARETPOS16 {         /* u183 */
    VPPOINT16 f1;
} GETCARETPOS16;
typedef GETCARETPOS16 UNALIGNED *PGETCARETPOS16;

typedef struct _GETCLASSINFO16 {        /* u404 */
    VPWNDCLASS16 f3;
    VPSTR   f2;
    HAND16  f1;
} GETCLASSINFO16;
typedef GETCLASSINFO16 UNALIGNED *PGETCLASSINFO16;

typedef struct _GETCLASSLONG16 {        /* u131 */
    SHORT   f2;
    HWND16  f1;
} GETCLASSLONG16;
typedef GETCLASSLONG16 UNALIGNED *PGETCLASSLONG16;

typedef struct _GETCLASSNAME16 {        /* u58 */
    SHORT   f3;
    VPSTR   f2;
    HWND16  f1;
} GETCLASSNAME16;
typedef GETCLASSNAME16 UNALIGNED *PGETCLASSNAME16;

typedef struct _GETCLASSWORD16 {        /* u129 */
    SHORT   f2;
    HWND16  f1;
} GETCLASSWORD16;
typedef GETCLASSWORD16 UNALIGNED *PGETCLASSWORD16;

typedef struct _GETCLIENTRECT16 {       /* u33 */
    VPRECT16 vpRect;
    HWND16   hwnd;
} GETCLIENTRECT16;
typedef GETCLIENTRECT16 UNALIGNED *PGETCLIENTRECT16;

typedef struct _GETCLIPBOARDDATA16 {        /* u142 */
    WORD    f1;
} GETCLIPBOARDDATA16;
typedef GETCLIPBOARDDATA16 UNALIGNED *PGETCLIPBOARDDATA16;

typedef struct _GETCLIPBOARDFORMATNAME16 {  /* u146 */
    SHORT   f3;
    VPSTR   f2;
    WORD    f1;
} GETCLIPBOARDFORMATNAME16;
typedef GETCLIPBOARDFORMATNAME16 UNALIGNED *PGETCLIPBOARDFORMATNAME16;

#ifdef NULLSTRUCT
typedef struct _GETCLIPBOARDOWNER16 {       /* u140 */
} GETCLIPBOARDOWNER16;
typedef GETCLIPBOARDOWNER16 UNALIGNED *PGETCLIPBOARDOWNER16;
#endif

#ifdef NULLSTRUCT
typedef struct _GETCLIPBOARDVIEWER16 {      /* u148 */
} GETCLIPBOARDVIEWER16;
typedef GETCLIPBOARDVIEWER16 UNALIGNED *PGETCLIPBOARDVIEWER16;
#endif

typedef struct _GETCOMMERROR16 {        /* u203 */
    VPCOMSTAT16 f2;
    SHORT   f1;
} GETCOMMERROR16;
typedef GETCOMMERROR16 UNALIGNED *PGETCOMMERROR16;

typedef struct _GETCOMMEVENTMASK16 {        /* u209 */
    SHORT   f2;
    SHORT   f1;
} GETCOMMEVENTMASK16;
typedef GETCOMMEVENTMASK16 UNALIGNED *PGETCOMMEVENTMASK16;

typedef struct _GETCOMMSTATE16 {        /* u202 */
    VPDCB16 f2;
    SHORT   f1;
} GETCOMMSTATE16;
typedef GETCOMMSTATE16 UNALIGNED *PGETCOMMSTATE16;

typedef struct _GETCONTROLBRUSH16 {     /* u326 */
    WORD    f3;
    HDC16   f2;
    HWND16  f1;
} GETCONTROLBRUSH16;
typedef GETCONTROLBRUSH16 UNALIGNED *PGETCONTROLBRUSH16;

#ifdef NULLSTRUCT
typedef struct _GETCURRENTTIME16 {      /* u15 */
} GETCURRENTTIME16;
typedef GETCURRENTTIME16 UNALIGNED *PGETCURRENTTIME16;
#endif

typedef struct _GETCURSORPOS16 {        /* u17 */
    VPPOINT16 f1;
} GETCURSORPOS16;
typedef GETCURSORPOS16 UNALIGNED *PGETCURSORPOS16;

typedef struct _GETDC16 {           /* u66 */
    HWND16  f1;
} GETDC16;
typedef GETDC16 UNALIGNED *PGETDC16;

#ifdef NULLSTRUCT
typedef struct _GETDESKTOPHWND16 {      /* u278 */
} GETDESKTOPHWND16;
typedef GETDESKTOPHWND16 UNALIGNED *PGETDESKTOPHWND16;
#endif

#ifdef NULLSTRUCT
typedef struct _GETDESKTOPWINDOW16 {        /* u286 */
} GETDESKTOPWINDOW16;
typedef GETDESKTOPWINDOW16 UNALIGNED *PGETDESKTOPWINDOW16;
#endif

#ifdef NULLSTRUCT
typedef struct _GETDIALOGBASEUNITS16 {      /* u243 */
} GETDIALOGBASEUNITS16;
typedef GETDIALOGBASEUNITS16 UNALIGNED *PGETDIALOGBASEUNITS16;
#endif

typedef struct _GETDLGCTRLID16 {        /* u277 */
    HWND16  f1;
} GETDLGCTRLID16;
typedef GETDLGCTRLID16 UNALIGNED *PGETDLGCTRLID16;

typedef struct _GETDLGITEM16 {          /* u91 */
    SHORT   f2;
    HWND16  f1;
} GETDLGITEM16;
typedef GETDLGITEM16 UNALIGNED *PGETDLGITEM16;

typedef struct _GETDLGITEMINT16 {       /* u95 */
    BOOL16  f4;
    VPBOOL16 f3;
    SHORT   f2;
    HWND16  f1;
} GETDLGITEMINT16;
typedef GETDLGITEMINT16 UNALIGNED *PGETDLGITEMINT16;

typedef struct _GETDLGITEMTEXT16 {      /* u93 */
    SHORT   f4;
    VPSTR   f3;
    SHORT   f2;
    HWND16  f1;
} GETDLGITEMTEXT16;
typedef GETDLGITEMTEXT16 UNALIGNED *PGETDLGITEMTEXT16;

#ifdef NULLSTRUCT
typedef struct _GETDOUBLECLICKTIME16 {      /* u21 */
} GETDOUBLECLICKTIME16;
typedef GETDOUBLECLICKTIME16 UNALIGNED *PGETDOUBLECLICKTIME16;
#endif

typedef struct _GETFILEPORTNAME16 {     /* u343 */
    VPSTR   f1;
} GETFILEPORTNAME16;
typedef GETFILEPORTNAME16 UNALIGNED *PGETFILEPORTNAME16;

#ifdef NULLSTRUCT
typedef struct _GETFOCUS16 {            /* u23 */
} GETFOCUS16;
typedef GETFOCUS16 UNALIGNED *PGETFOCUS16;
#endif

typedef struct _GETICONID16 {           /* u455 */
    VPSTR  f2;
    HAND16 f1;
} GETICONID16;
typedef GETICONID16 UNALIGNED *PGETICONID16;

#ifdef NULLSTRUCT
typedef struct _GETINPUTSTATE16 {       /* u335 */
} GETINPUTSTATE16;
typedef GETINPUTSTATE16 UNALIGNED *PGETINPUTSTATE16;
#endif

typedef struct _GETINTERNALICONHEADER16 {   /* u372 */
    VPSTR   f2;
    VPSTR   f1;
} GETINTERNALICONHEADER16;
typedef GETINTERNALICONHEADER16 UNALIGNED *PGETINTERNALICONHEADER16;

typedef struct _GETINTERNALWINDOWPOS16 {    /* u460 */
    VPPOINT16 f3;
    VPRECT16  f2;
    HWND16    f1;
} GETINTERNALWINDOWPOS16;
typedef GETINTERNALWINDOWPOS16 UNALIGNED *PGETINTERNALWINDOWPOS16;

typedef struct _GETKEYBOARDSTATE16 {        /* u222 */
    VPBYTE  f1;
} GETKEYBOARDSTATE16;
typedef GETKEYBOARDSTATE16 UNALIGNED *PGETKEYBOARDSTATE16;

typedef struct _GETKEYSTATE16 {         /* u106 */
    SHORT   f1;
} GETKEYSTATE16;
typedef GETKEYSTATE16 UNALIGNED *PGETKEYSTATE16;

typedef struct _GETLASTACTIVEPOPUP16 {      /* u287 */
    HWND16  f1;
} GETLASTACTIVEPOPUP16;
typedef GETLASTACTIVEPOPUP16 UNALIGNED *PGETLASTACTIVEPOPUP16;

typedef struct _GETMENU16 {         /* u157 */
    HWND16  f1;
} GETMENU16;
typedef GETMENU16 UNALIGNED *PGETMENU16;

#ifdef NULLSTRUCT
typedef struct _GETMENUCHECKMARKDIMENSIONS16 {  /* u417 */
} GETMENUCHECKMARKDIMENSIONS16;
typedef GETMENUCHECKMARKDIMENSIONS16 UNALIGNED *PGETMENUCHECKMARKDIMENSIONS16;
#endif

typedef struct _GETMENUITEMCOUNT16 {        /* u263 */
    HMENU16 f1;
} GETMENUITEMCOUNT16;
typedef GETMENUITEMCOUNT16 UNALIGNED *PGETMENUITEMCOUNT16;

typedef struct _GETMENUITEMID16 {       /* u264 */
    SHORT   f2;
    HMENU16 f1;
} GETMENUITEMID16;
typedef GETMENUITEMID16 UNALIGNED *PGETMENUITEMID16;

typedef struct _GETMENUSTATE16 {        /* u250 */
    WORD    f3;
    WORD    f2;
    HMENU16 f1;
} GETMENUSTATE16;
typedef GETMENUSTATE16 UNALIGNED *PGETMENUSTATE16;

typedef struct _GETMENUSTRING16 {       /* u161 */
    WORD    f5;
    SHORT   f4;
    VPSTR   f3;
    WORD    f2;
    HMENU16 f1;
} GETMENUSTRING16;
typedef GETMENUSTRING16 UNALIGNED *PGETMENUSTRING16;

typedef struct _GETMESSAGE16 {          /* u108 */
    WORD    wMax;
    WORD    wMin;
    HWND16  hwnd;
    VPMSG16 vpMsg;
} GETMESSAGE16;
typedef GETMESSAGE16 UNALIGNED *PGETMESSAGE16;

typedef struct _GETMESSAGE216 {         /* u323 */
    BOOL16  f6;
    WORD    f5;
    WORD    f4;
    WORD    f3;
    HWND16  f2;
    VPMSG16 f1;
} GETMESSAGE216;
typedef GETMESSAGE216 UNALIGNED *PGETMESSAGE216;

#ifdef NULLSTRUCT
typedef struct _GETMESSAGEPOS16 {       /* u119 */
} GETMESSAGEPOS16;
typedef GETMESSAGEPOS16 UNALIGNED *PGETMESSAGEPOS16;
#endif

#ifdef NULLSTRUCT
typedef struct _GETMESSAGETIME16 {      /* u120 */
} GETMESSAGETIME16;
typedef GETMESSAGETIME16 UNALIGNED *PGETMESSAGETIME16;
#endif

typedef struct _GETNEXTDLGGROUPITEM16 {     /* u227 */
    BOOL16  f3;
    HWND16  f2;
    HWND16  f1;
} GETNEXTDLGGROUPITEM16;
typedef GETNEXTDLGGROUPITEM16 UNALIGNED *PGETNEXTDLGGROUPITEM16;

typedef struct _GETNEXTDLGTABITEM16 {       /* u228 */
    BOOL16  f3;
    HWND16  f2;
    HWND16  f1;
} GETNEXTDLGTABITEM16;
typedef GETNEXTDLGTABITEM16 UNALIGNED *PGETNEXTDLGTABITEM16;

typedef struct _GETNEXTQUEUEWINDOW16 {  /* u274 */
    BOOL16  f2;
    HWND16  f1;
} GETNEXTQUEUEWINDOW16;
typedef GETNEXTQUEUEWINDOW16 UNALIGNED *PGETNEXTQUEUEWINDOW16;

typedef struct _GETNEXTWINDOW16 {       /* u230 */
    WORD    f2;
    HWND16  f1;
} GETNEXTWINDOW16;
typedef GETNEXTWINDOW16 UNALIGNED *PGETNEXTWINDOW16;

typedef struct _GETPARENT16 {           /* u46 */
    HWND16  f1;
} GETPARENT16;
typedef GETPARENT16 UNALIGNED *PGETPARENT16;

typedef struct _GETPRIORITYCLIPBOARDFORMAT16 {  /* u402 */
    SHORT   f2;
    VPWORD  f1;
} GETPRIORITYCLIPBOARDFORMAT16;
typedef GETPRIORITYCLIPBOARDFORMAT16 UNALIGNED *PGETPRIORITYCLIPBOARDFORMAT16;

typedef struct _GETPROP16 {         /* u25 */
    VPSTR   f2;
    HWND16  f1;
} GETPROP16;
typedef GETPROP16 UNALIGNED *PGETPROP16;

typedef struct _GETSCROLLPOS16 {        /* u63 */
    SHORT   f2;
    HWND16  f1;
} GETSCROLLPOS16;
typedef GETSCROLLPOS16 UNALIGNED *PGETSCROLLPOS16;

typedef struct _GETSCROLLRANGE16 {      /* u65 */
    VPSHORT f4;
    VPSHORT f3;
    SHORT   f2;
    HWND16  f1;
} GETSCROLLRANGE16;
typedef GETSCROLLRANGE16 UNALIGNED *PGETSCROLLRANGE16;

typedef struct _GETSUBMENU16 {          /* u159 */
    SHORT   f2;
    HMENU16 f1;
} GETSUBMENU16;
typedef GETSUBMENU16 UNALIGNED *PGETSUBMENU16;

typedef struct _GETSYSCOLOR16 {         /* u180 */
    SHORT   f1;
} GETSYSCOLOR16;
typedef GETSYSCOLOR16 UNALIGNED *PGETSYSCOLOR16;

#ifdef NULLSTRUCT
typedef struct _GETSYSMODALWINDOW16 {       /* u189 */
} GETSYSMODALWINDOW16;
typedef GETSYSMODALWINDOW16 UNALIGNED *PGETSYSMODALWINDOW16;
#endif

typedef struct _GETSYSTEMMENU16 {       /* u156 */
    BOOL16  f2;
    HWND16  f1;
} GETSYSTEMMENU16;
typedef GETSYSTEMMENU16 UNALIGNED *PGETSYSTEMMENU16;

typedef struct _GETSYSTEMMETRICS16 {        /* u179 */
    SHORT   f1;
} GETSYSTEMMETRICS16;
typedef GETSYSTEMMETRICS16 UNALIGNED *PGETSYSTEMMETRICS16;

typedef struct _GETTABBEDTEXTEXTENT16 {     /* u197 */
    VPSHORT f5;
    SHORT   f4;
    SHORT   f3;
    VPSTR   f2;
    HDC16   f1;
} GETTABBEDTEXTEXTENT16;
typedef GETTABBEDTEXTEXTENT16 UNALIGNED *PGETTABBEDTEXTEXTENT16;

#ifdef NULLSTRUCT
typedef struct _GETTICKCOUNT16 {        /* u13 */
} GETTICKCOUNT16;
typedef GETTICKCOUNT16 UNALIGNED *PGETTICKCOUNT16;
#endif

#ifdef NULLSTRUCT
typedef struct _GETTIMERRESOLUTION16 {  /* u14 */
} GETTIMERRESOLUTION16;
typedef GETTIMERRESOLUTION16 UNALIGNED *PGETTIMERRESOLUTION16;
#endif

typedef struct _GETTOPWINDOW16 {        /* u229 */
    HWND16  f1;
} GETTOPWINDOW16;
typedef GETTOPWINDOW16 UNALIGNED *PGETTOPWINDOW16;

typedef struct _GETUPDATERECT16 {       /* u190 */
    BOOL16  f3;
    VPRECT16 f2;
    HWND16  f1;
} GETUPDATERECT16;
typedef GETUPDATERECT16 UNALIGNED *PGETUPDATERECT16;

typedef struct _GETUPDATERGN16 {        /* u237 */
    BOOL16  f3;
    HRGN16  f2;
    HWND16  f1;
} GETUPDATERGN16;
typedef GETUPDATERGN16 UNALIGNED *PGETUPDATERGN16;

typedef struct _GETUSERLOCALOBJTYPE16 { /* u480 */
    HAND16  f1;
} GETUSERLOCALOBJTYPE16;
typedef GETUSERLOCALOBJTYPE16 UNALIGNED *PGETUSERLOCALOBJTYPE16;

typedef struct _GETWC216 {              /* u318 */
    SHORT   f2;
    HWND16  f1;
} GETWC216;
typedef GETWC216 UNALIGNED *PGETWC216;

typedef struct _GETWINDOW16 {           /* u262 */
    WORD    f2;
    HWND16  f1;
} GETWINDOW16;
typedef GETWINDOW16 UNALIGNED *PGETWINDOW16;

typedef struct _GETWINDOWDC16 {         /* u67 */
    HWND16  f1;
} GETWINDOWDC16;
typedef GETWINDOWDC16 UNALIGNED *PGETWINDOWDC16;

typedef struct _GETWINDOWLONG16 {       /* u135 */
    SHORT   f2;
    HWND16  f1;
} GETWINDOWLONG16;
typedef GETWINDOWLONG16 UNALIGNED *PGETWINDOWLONG16;

typedef struct _GETWINDOWRECT16 {       /* u32 */
    VPRECT16 f2;
    HWND16  f1;
} GETWINDOWRECT16;
typedef GETWINDOWRECT16 UNALIGNED *PGETWINDOWRECT16;

typedef struct _GETWINDOWTASK16 {       /* u224 */
    HWND16  f1;
} GETWINDOWTASK16;
typedef GETWINDOWTASK16 UNALIGNED *PGETWINDOWTASK16;

typedef struct _GETWINDOWTEXT16 {       /* u36 */
    WORD    f3;
    VPSTR   f2;
    HWND16  f1;
} GETWINDOWTEXT16;
typedef GETWINDOWTEXT16 UNALIGNED *PGETWINDOWTEXT16;

typedef struct _GETWINDOWTEXTLENGTH16 {     /* u38 */
    HWND16  f1;
} GETWINDOWTEXTLENGTH16;
typedef GETWINDOWTEXTLENGTH16 UNALIGNED *PGETWINDOWTEXTLENGTH16;

typedef struct _GETWINDOWWORD16 {       /* u133 */
    SHORT   f2;
    HWND16  f1;
} GETWINDOWWORD16;
typedef GETWINDOWWORD16 UNALIGNED *PGETWINDOWWORD16;

typedef struct _GLOBALADDATOM16 {       /* u268 */
    VPSTR   f1;
} GLOBALADDATOM16;
typedef GLOBALADDATOM16 UNALIGNED *PGLOBALADDATOM16;

typedef struct _GLOBALDELETEATOM16 {        /* u269 */
    ATOM    f1;
} GLOBALDELETEATOM16;
typedef GLOBALDELETEATOM16 UNALIGNED *PGLOBALDELETEATOM16;

typedef struct _GLOBALFINDATOM16 {      /* u270 */
    VPSTR   f1;
} GLOBALFINDATOM16;
typedef GLOBALFINDATOM16 UNALIGNED *PGLOBALFINDATOM16;

typedef struct _GLOBALGETATOMNAME16 {       /* u271 */
    SHORT   f3;
    VPSTR   f2;
    ATOM    f1;
} GLOBALGETATOMNAME16;
typedef GLOBALGETATOMNAME16 UNALIGNED *PGLOBALGETATOMNAME16;

typedef struct _GRAYSTRING16 {          /* u185 */
    SHORT   f9;
    SHORT   f8;
    SHORT   f7;
    SHORT   f6;
    SHORT   f5;
    DWORD   f4;
    VPPROC f3;
    HBRSH16 f2;
    HDC16   f1;
} GRAYSTRING16;
typedef GRAYSTRING16 UNALIGNED *PGRAYSTRING16;

#ifdef NULLSTRUCT
typedef struct _HARDWARE_EVENT16 {      /* u481 */
} HARDWARE_EVENT16;
typedef HARDWARE_EVENT16 UNALIGNED *PHARDWARE_EVENT16;
#endif

typedef struct _HIDECARET16 {           /* u166 */
    HWND16 f1;
} HIDECARET16;
typedef HIDECARET16 UNALIGNED *PHIDECARET16;

typedef struct _HILITEMENUITEM16 {      /* u162 */
    WORD f4;
    WORD f3;
    HMENU16 f2;
    HWND16 f1;
} HILITEMENUITEM16;
typedef HILITEMENUITEM16 UNALIGNED *PHILITEMENUITEM16;

#ifdef NULLSTRUCT
typedef struct _ICONSIZE16 {            /* u86 */
} ICONSIZE16;
typedef ICONSIZE16 UNALIGNED *PICONSIZE16;
#endif

typedef struct _INFLATERECT16 {         /* u78 */
    SHORT f3;
    SHORT f2;
    VPRECT16 f1;
} INFLATERECT16;
typedef INFLATERECT16 UNALIGNED *PINFLATERECT16;

typedef struct _INITAPP16 {         /* u5 */
    HAND16  hInstance;
} INITAPP16;
typedef INITAPP16 UNALIGNED *PINITAPP16;

#ifdef NULLSTRUCT
typedef struct _INSENDMESSAGE16 {       /* u192 */
} INSENDMESSAGE16;
typedef INSENDMESSAGE16 UNALIGNED *PINSENDMESSAGE16;
#endif

typedef struct _INSERTMENU16 {          /* u410 */
    VPSTR f5;
    WORD f4;
    WORD f3;
    WORD f2;
    HMENU16 f1;
} INSERTMENU16;
typedef INSERTMENU16 UNALIGNED *PINSERTMENU16;

typedef struct _INTERSECTRECT16 {       /* u79 */
    VPRECT16 f3;
    VPRECT16 f2;
    VPRECT16 f1;
} INTERSECTRECT16;
typedef INTERSECTRECT16 UNALIGNED *PINTERSECTRECT16;

typedef struct _INVALIDATERECT16 {      /* u125 */
    BOOL16 f3;
    VPRECT16 f2;
    HWND16 f1;
} INVALIDATERECT16;
typedef INVALIDATERECT16 UNALIGNED *PINVALIDATERECT16;

typedef struct _INVALIDATERGN16 {       /* u126 */
    BOOL16 f3;
    HRGN16 f2;
    HWND16 f1;
} INVALIDATERGN16;
typedef INVALIDATERGN16 UNALIGNED *PINVALIDATERGN16;

typedef struct _INVERTRECT16 {          /* u82 */
    VPRECT16 f2;
    HDC16 f1;
} INVERTRECT16;
typedef INVERTRECT16 UNALIGNED *PINVERTRECT16;

typedef struct _ISCHARALPHA16 {         /* u433 */
    SHORT f1;
} ISCHARALPHA16;
typedef ISCHARALPHA16 UNALIGNED *PISCHARALPHA16;

typedef struct _ISCHARALPHANUMERIC16 {      /* u434 */
    SHORT f1;
} ISCHARALPHANUMERIC16;
typedef ISCHARALPHANUMERIC16 UNALIGNED *PISCHARALPHANUMERIC16;

typedef struct _ISCHARLOWER16 {         /* u436 */
    SHORT f1;
} ISCHARLOWER16;
typedef ISCHARLOWER16 UNALIGNED *PISCHARLOWER16;

typedef struct _ISCHARUPPER16 {         /* u435 */
    SHORT f1;
} ISCHARUPPER16;
typedef ISCHARUPPER16 UNALIGNED *PISCHARUPPER16;

typedef struct _ISCHILD16 {         /* u48 */
    HWND16 f2;
    HWND16 f1;
} ISCHILD16;
typedef ISCHILD16 UNALIGNED *PISCHILD16;

typedef struct _ISCLIPBOARDFORMATAVAILABLE16 {  /* u193 */
    WORD f1;
} ISCLIPBOARDFORMATAVAILABLE16;
typedef ISCLIPBOARDFORMATAVAILABLE16 UNALIGNED *PISCLIPBOARDFORMATAVAILABLE16;

typedef struct _ISDIALOGMESSAGE16 {     /* u90 */
    VPMSG16 f2;
    HWND16 f1;
} ISDIALOGMESSAGE16;
typedef ISDIALOGMESSAGE16 UNALIGNED *PISDIALOGMESSAGE16;

typedef struct _ISDLGBUTTONCHECKED16 {      /* u98 */
    SHORT f2;
    HWND16 f1;
} ISDLGBUTTONCHECKED16;
typedef ISDLGBUTTONCHECKED16 UNALIGNED *PISDLGBUTTONCHECKED16;

typedef struct _ISICONIC16 {            /* u31 */
    HWND16 f1;
} ISICONIC16;
typedef ISICONIC16 UNALIGNED *PISICONIC16;

typedef struct _ISRECTEMPTY16 {         /* u75 */
    VPRECT16 f1;
} ISRECTEMPTY16;
typedef ISRECTEMPTY16 UNALIGNED *PISRECTEMPTY16;

typedef struct _ISTWOBYTECHARPREFIX16 {     /* u51 */
    SHORT f1;
} ISTWOBYTECHARPREFIX16;
typedef ISTWOBYTECHARPREFIX16 UNALIGNED *PISTWOBYTECHARPREFIX16;

#ifdef NULLSTRUCT
typedef struct _ISUSERIDLE16 {          /* u59 */
} ISUSERIDLE16;
typedef ISUSERIDLE16 UNALIGNED *PISUSERIDLE16;
#endif

typedef struct _ISWINDOW16 {            /* u47 */
    HWND16 f1;
} ISWINDOW16;
typedef ISWINDOW16 UNALIGNED *PISWINDOW16;

typedef struct _ISWINDOWENABLED16 {     /* u35 */
    HWND16 f1;
} ISWINDOWENABLED16;
typedef ISWINDOWENABLED16 UNALIGNED *PISWINDOWENABLED16;

typedef struct _ISWINDOWVISIBLE16 {     /* u49 */
    HWND16 f1;
} ISWINDOWVISIBLE16;
typedef ISWINDOWVISIBLE16 UNALIGNED *PISWINDOWVISIBLE16;

typedef struct _ISZOOMED16 {            /* u272 */
    HWND16 f1;
} ISZOOMED16;
typedef ISZOOMED16 UNALIGNED *PISZOOMED16;

#ifdef NULLSTRUCT
typedef struct _KEYBD_EVENT16 {         /* u289 */
} KEYBD_EVENT16;
typedef KEYBD_EVENT16 UNALIGNED *PKEYBD_EVENT16;
#endif

typedef struct _KEYBDEVENT16 {          /* u539 */
    DWORD dwExtraInfo;
    WORD bScanCode;
    WORD bVirtualKey;
} KEYBDEVENT16;
typedef KEYBDEVENT16 UNALIGNED *PKEYBDEVENT16;

typedef struct _KILLSYSTEMTIMER16 {     /* u182 */
    SHORT  f2;
    HWND16 f1;
} KILLSYSTEMTIMER16;
typedef KILLSYSTEMTIMER16 UNALIGNED *PKILLSYSTEMTIMER16;

typedef struct _KILLTIMER16 {           /* u12 */
    SHORT f2;
    HWND16 f1;
} KILLTIMER16;
typedef KILLTIMER16 UNALIGNED *PKILLTIMER16;

typedef struct _KILLTIMER216 {          /* u327 */
    SHORT  f2;
    HWND16 f1;
} KILLTIMER216;
typedef KILLTIMER216 UNALIGNED *PKILLTIMER216;

typedef struct _LBOXCARETBLINKER16 {    /* u453 */
    DWORD   f4;
    WORD    f3;
    WORD    f2;
    HWND16  f1;
} LBOXCARETBLINKER16;
typedef LBOXCARETBLINKER16 UNALIGNED *PLBOXCARETBLINKER16;

typedef struct _LOADACCELERATORS16 {        /* u177 */
    VPSTR f2;
    HAND16 f1;
} LOADACCELERATORS16;
typedef LOADACCELERATORS16 UNALIGNED *PLOADACCELERATORS16;

typedef struct _LOADBITMAP16 {          /* u175 */
    DWORD  f4;
    DWORD  f3;
    VPSTR  f2;
    HAND16 f1;
} LOADBITMAP16;
typedef LOADBITMAP16 UNALIGNED *PLOADBITMAP16;

typedef struct _LOADCURSOR16 {		/* u173 */
    WORD   f7;
    WORD   f6;
    WORD   f5;
    DWORD  f4;
    DWORD  f3;
    VPSTR  f2;
    HAND16 f1;
} LOADCURSOR16;
typedef LOADCURSOR16 UNALIGNED *PLOADCURSOR16;

typedef struct _LOADCURSORICONHANDLER16 { /* u336 */
    HAND16  f3;
    HAND16  f2;
    HAND16  f1;
} LOADCURSORICONHANDLER16;
typedef LOADCURSORICONHANDLER16 UNALIGNED *PLOADCURSORICONHANDLER16;

typedef struct _LOADDIBCURSORHANDLER16 {  /* u356 */
    HAND16  f3;
    HAND16  f2;
    HAND16  f1;
} LOADDIBCURSORHANDLER16;
typedef LOADDIBCURSORHANDLER16 UNALIGNED *PLOADDIBCURSORHANDLER16;

typedef struct _LOADDIBICONHANDLER16 {    /* u357 */
    HAND16  f3;
    HAND16  f2;
    HAND16  f1;
} LOADDIBICONHANDLER16;
typedef LOADDIBICONHANDLER16 UNALIGNED *PLOADDIBICONHANDLER16;

typedef struct _LOADICON16 {		/* u174 */
    WORD   f6;
    WORD   f5;
    DWORD  f4;
    DWORD  f3;
    VPSTR  f2;
    HAND16 f1;
} LOADICON16;
typedef LOADICON16 UNALIGNED *PLOADICON16;

typedef struct _LOADICONHANDLER16 {     /* u456 */
    BOOL16  f2;
    HICON16 f1;
} LOADICONHANDLER16;
typedef LOADICONHANDLER16 UNALIGNED *PLOADICONHANDLER16;

typedef struct _LOADMENU16 {            /* u150 */
    WORD   f5;
    DWORD  f4;
    DWORD  f3;
    VPSTR  f2;
    HAND16 f1;
} LOADMENU16;
typedef LOADMENU16 UNALIGNED *PLOADMENU16;

typedef struct _LOADMENUINDIRECT16 {        /* u220 */
    VPSTR f1;
} LOADMENUINDIRECT16;
typedef LOADMENUINDIRECT16 UNALIGNED *PLOADMENUINDIRECT16;

typedef struct _LOADSTRING16 {          /* u176 */
    SHORT f4;
    VPSTR f3;
    WORD f2;
    HAND16 f1;
} LOADSTRING16;
typedef LOADSTRING16 UNALIGNED *PLOADSTRING16;

typedef struct _LOCKMYTASK16 {          /* u276 */
    BOOL16  f1;
} LOCKMYTASK16;
typedef LOCKMYTASK16 UNALIGNED *PLOCKMYTASK16;

typedef struct _LOOKUPMENUHANDLE16 {    /* u217 */
    WORD    f2;
    HMENU16 f1;
} LOOKUPMENUHANDLE16;
typedef LOOKUPMENUHANDLE16 UNALIGNED *PLOOKUPMENUHANDLE16;

typedef struct _LSTRCMP16 {         /* u430 */
    VPSTR  f2;
    VPSTR f1;
} LSTRCMP16;
typedef LSTRCMP16 UNALIGNED *PLSTRCMP16;

typedef struct _LSTRCMPI16 {            /* u471 */
    VPSTR  f2;
    VPSTR f1;
} LSTRCMPI16;
typedef LSTRCMPI16 UNALIGNED *PLSTRCMPI16;

typedef struct _MAPDIALOGRECT16 {       /* u103 */
    VPRECT16 f2;
    HWND16 f1;
} MAPDIALOGRECT16;
typedef MAPDIALOGRECT16 UNALIGNED *PMAPDIALOGRECT16;

typedef struct _MB_DLGPROC16 {          /* u409 */
    LONG    f4;
    WORD    f3;
    WORD    f2;
    HWND16  f1;
} MB_DLGPROC16;
typedef MB_DLGPROC16 UNALIGNED *PMB_DLGPROC16;

typedef struct _MENUITEMSTATE16 {       /* u329 */
    WORD    f3;
    WORD    f2;
    HMENU16 f1;
} MENUITEMSTATE16;
typedef MENUITEMSTATE16 UNALIGNED *PMENUITEMSTATE16;

typedef struct _MESSAGEBEEP16 {         /* u104 */
    WORD f1;
} MESSAGEBEEP16;
typedef MESSAGEBEEP16 UNALIGNED *PMESSAGEBEEP16;

typedef struct _MESSAGEBOX16 {          /* u1 */
    WORD f4;
    VPSTR f3;
    VPSTR f2;
    HWND16 f1;
} MESSAGEBOX16;
typedef MESSAGEBOX16 UNALIGNED *PMESSAGEBOX16;

typedef struct _MODIFYMENU16 {          /* u414 */
    VPSTR f5;
    WORD f4;
    WORD f3;
    WORD f2;
    HMENU16 f1;
} MODIFYMENU16;
typedef MODIFYMENU16 UNALIGNED *PMODIFYMENU16;

#ifdef NULLSTRUCT
typedef struct _MOUSE_EVENT16 {         /* u299 */
} MOUSE_EVENT16;
typedef MOUSE_EVENT16 UNALIGNED *PMOUSE_EVENT16;
#endif

typedef struct _MOUSEEVENT16 {          /* u538 */
    DWORD dwExtraInfo;
    WORD cButtons;
    WORD dy;
    WORD dx;
    WORD wFlags;
} MOUSEEVENT16;
typedef MOUSEEVENT16 UNALIGNED *PMOUSEEVENT16;


typedef struct _MOVEWINDOW16 {          /* u56 */
    BOOL16 f6;
    SHORT f5;
    SHORT f4;
    SHORT f3;
    SHORT f2;
    HWND16 f1;
} MOVEWINDOW16;
typedef MOVEWINDOW16 UNALIGNED *PMOVEWINDOW16;

typedef struct _OFFSETRECT16 {          /* u77 */
    SHORT f3;
    SHORT f2;
    VPRECT16 f1;
} OFFSETRECT16;
typedef OFFSETRECT16 UNALIGNED *POFFSETRECT16;

typedef struct _OPENCLIPBOARD16 {       /* u137 */
    HWND16 f1;
} OPENCLIPBOARD16;
typedef OPENCLIPBOARD16 UNALIGNED *POPENCLIPBOARD16;

typedef struct _OPENCOMM16 {            /* u200 */
    DWORD f4;  /* added for SetCommEventMask() support */
    WORD  f3;
    WORD  f2;
    VPSTR f1;
} OPENCOMM16;
typedef OPENCOMM16 UNALIGNED *POPENCOMM16;

typedef struct _OPENICON16 {            /* u44 */
    HWND16 f1;
} OPENICON16;
typedef OPENICON16 UNALIGNED *POPENICON16;

typedef struct _PAINTRECT16 {           /* u325 */
    VPRECT16 f5;
    HBRSH16  f4;
    HDC16    f3;
    HWND16   f2;
    HWND16   f1;
} PAINTRECT16;
typedef PAINTRECT16 UNALIGNED *PPAINTRECT16;

typedef struct _PEEKMESSAGE16 {         /* u109 */
    WORD f5;
    WORD f4;
    WORD f3;
    HWND16 f2;
    VPMSG16 f1;
} PEEKMESSAGE16;
typedef PEEKMESSAGE16 UNALIGNED *PPEEKMESSAGE16;

typedef struct _POSTAPPMESSAGE16 {      /* u116 */
    LONG f4;
    WORD f3;
    WORD f2;
    HAND16 f1;
} POSTAPPMESSAGE16;
typedef POSTAPPMESSAGE16 UNALIGNED *PPOSTAPPMESSAGE16;

typedef struct _POSTMESSAGE16 {         /* u110 */
    LONG f4;
    WORD f3;
    WORD f2;
    HWND16 f1;
} POSTMESSAGE16;
typedef POSTMESSAGE16 UNALIGNED *PPOSTMESSAGE16;

typedef struct _POSTMESSAGE216 {        /* u313 */
    LONG    f4;
    WORD    f3;
    WORD    f2;
    HWND16  f1;
} POSTMESSAGE216;
typedef POSTMESSAGE216 UNALIGNED *PPOSTMESSAGE216;

typedef struct _POSTQUITMESSAGE16 {     /* u6 */
    SHORT   wExitCode;
} POSTQUITMESSAGE16;
typedef POSTQUITMESSAGE16 UNALIGNED *PPOSTQUITMESSAGE16;

typedef struct _PTINRECT16 {            /* u76 */
    POINT16 f2;
    VPRECT16 f1;
} PTINRECT16;
typedef PTINRECT16 UNALIGNED *PPTINRECT16;

typedef struct _READCOMM16 {            /* u204 */
    SHORT f3;
    VPSTR f2;
    SHORT f1;
} READCOMM16;
typedef READCOMM16 UNALIGNED *PREADCOMM16;

typedef struct _REALIZEPALETTE16 {      /* u283 */
    HDC16 f1;
} REALIZEPALETTE16;
typedef REALIZEPALETTE16 UNALIGNED *PREALIZEPALETTE16;

typedef struct _REGISTERCLASS16 {       /* u57 */
    VPWNDCLASS16 vpWndClass;
} REGISTERCLASS16;
typedef REGISTERCLASS16 UNALIGNED *PREGISTERCLASS16;

typedef struct _REGISTERCLIPBOARDFORMAT16 { /* u145 */
    VPSTR f1;
} REGISTERCLIPBOARDFORMAT16;
typedef REGISTERCLIPBOARDFORMAT16 UNALIGNED *PREGISTERCLIPBOARDFORMAT16;

typedef struct _REGISTERWINDOWMESSAGE16 {   /* u118 */
    VPSTR f1;
} REGISTERWINDOWMESSAGE16;
typedef REGISTERWINDOWMESSAGE16 UNALIGNED *PREGISTERWINDOWMESSAGE16;

#ifdef NULLSTRUCT
typedef struct _RELEASECAPTURE16 {      /* u19 */
} RELEASECAPTURE16;
typedef RELEASECAPTURE16 UNALIGNED *PRELEASECAPTURE16;
#endif

typedef struct _RELEASEDC16 {           /* u68 */
    HDC16 f2;
    HWND16 f1;
} RELEASEDC16;
typedef RELEASEDC16 UNALIGNED *PRELEASEDC16;

typedef struct _REMOVEMENU16 {          /* u412 */
    WORD f3;
    WORD f2;
    HMENU16 f1;
} REMOVEMENU16;
typedef REMOVEMENU16 UNALIGNED *PREMOVEMENU16;

typedef struct _REMOVEPROP16 {          /* u24 */
    VPSTR f2;
    HWND16 f1;
} REMOVEPROP16;
typedef REMOVEPROP16 UNALIGNED *PREMOVEPROP16;

#ifdef NULLSTRUCT
typedef struct _REPAINTSCREEN16 {       /* u275 */
} REPAINTSCREEN16;
typedef REPAINTSCREEN16 UNALIGNED *PREPAINTSCREEN16;
#endif

typedef struct _REPLYMESSAGE16 {        /* u115 */
    LONG f1;
} REPLYMESSAGE16;
typedef REPLYMESSAGE16 UNALIGNED *PREPLYMESSAGE16;

typedef struct _SCREENTOCLIENT16 {      /* u29 */
    VPPOINT16 f2;
    HWND16 f1;
} SCREENTOCLIENT16;
typedef SCREENTOCLIENT16 UNALIGNED *PSCREENTOCLIENT16;

typedef struct _SCROLLCHILDREN16 {      /* u463 */
    LONG    f4;
    WORD    f3;
    WORD    f2;
    HWND16  f1;
} SCROLLCHILDREN16;
typedef SCROLLCHILDREN16 UNALIGNED *PSCROLLCHILDREN16;

typedef struct _SCROLLDC16 {            /* u221 */
    VPRECT16 f7;
    HRGN16 f6;
    VPRECT16 f5;
    VPRECT16 f4;
    SHORT f3;
    SHORT f2;
    HDC16 f1;
} SCROLLDC16;
typedef SCROLLDC16 UNALIGNED *PSCROLLDC16;

typedef struct _SCROLLWINDOW16 {        /* u61 */
    VPRECT16 f5;
    VPRECT16 f4;
    SHORT f3;
    SHORT f2;
    HWND16 f1;
} SCROLLWINDOW16;
typedef SCROLLWINDOW16 UNALIGNED *PSCROLLWINDOW16;

typedef struct _SELECTPALETTE16 {       /* u282 */
    BOOL16 f3;
    HPAL16 f2;
    HDC16 f1;
} SELECTPALETTE16;
typedef SELECTPALETTE16 UNALIGNED *PSELECTPALETTE16;

typedef struct _SENDDLGITEMMESSAGE16 {      /* u101 */
    LONG f5;
    WORD f4;
    WORD f3;
    SHORT f2;
    HWND16 f1;
} SENDDLGITEMMESSAGE16;
typedef SENDDLGITEMMESSAGE16 UNALIGNED *PSENDDLGITEMMESSAGE16;

typedef struct _SENDMESSAGE16 {         /* u111 */
    LONG f4;
    WORD f3;
    WORD f2;
    HWND16 f1;
} SENDMESSAGE16;
typedef SENDMESSAGE16 UNALIGNED *PSENDMESSAGE16;

typedef struct _SENDMESSAGE216 {        /* u312 */
    LONG   f4;
    WORD   f3;
    WORD   f2;
    HWND16 f1;
} SENDMESSAGE216;
typedef SENDMESSAGE216 UNALIGNED *PSENDMESSAGE216;

typedef struct _SETACTIVEWINDOW16 {     /* u59 */
    HWND16 f1;
} SETACTIVEWINDOW16;
typedef SETACTIVEWINDOW16 UNALIGNED *PSETACTIVEWINDOW16;

typedef struct _SETCAPTURE16 {          /* u18 */
    HWND16 f1;
} SETCAPTURE16;
typedef SETCAPTURE16 UNALIGNED *PSETCAPTURE16;

typedef struct _SETCARETBLINKTIME16 {       /* u168 */
    WORD f1;
} SETCARETBLINKTIME16;
typedef SETCARETBLINKTIME16 UNALIGNED *PSETCARETBLINKTIME16;

typedef struct _SETCARETPOS16 {         /* u165 */
    SHORT f2;
    SHORT f1;
} SETCARETPOS16;
typedef SETCARETPOS16 UNALIGNED *PSETCARETPOS16;

typedef struct _SETCLASSLONG16 {        /* u132 */
    LONG f3;
    SHORT f2;
    HWND16 f1;
} SETCLASSLONG16;
typedef SETCLASSLONG16 UNALIGNED *PSETCLASSLONG16;

typedef struct _SETCLASSWORD16 {        /* u130 */
    WORD f3;
    SHORT f2;
    HWND16 f1;
} SETCLASSWORD16;
typedef SETCLASSWORD16 UNALIGNED *PSETCLASSWORD16;

typedef struct _SETCLIPBOARDDATA16 {        /* u141 */
    HAND16 f2;
    WORD f1;
} SETCLIPBOARDDATA16;
typedef SETCLIPBOARDDATA16 UNALIGNED *PSETCLIPBOARDDATA16;

typedef struct _SETCLIPBOARDVIEWER16 {      /* u147 */
    HWND16 f1;
} SETCLIPBOARDVIEWER16;
typedef SETCLIPBOARDVIEWER16 UNALIGNED *PSETCLIPBOARDVIEWER16;

typedef struct _SETCOMMBREAK16 {        /* u210 */
    SHORT f1;
} SETCOMMBREAK16;
typedef SETCOMMBREAK16 UNALIGNED *PSETCOMMBREAK16;

typedef struct _SETCOMMEVENTMASK16 {        /* u208 */
    WORD  f2;
    SHORT f1;
} SETCOMMEVENTMASK16;
typedef SETCOMMEVENTMASK16 UNALIGNED *PSETCOMMEVENTMASK16;

typedef struct _SETCOMMSTATE16 {        /* u201 */
    VPDCB16 f1;
} SETCOMMSTATE16;
typedef SETCOMMSTATE16 UNALIGNED *PSETCOMMSTATE16;

typedef struct _SETCURSOR16 {           /* u69 */
    HCUR16 f1;
} SETCURSOR16;
typedef SETCURSOR16 UNALIGNED *PSETCURSOR16;

typedef struct _SETCURSORPOS16 {        /* u70 */
    SHORT f2;
    SHORT f1;
} SETCURSORPOS16;
typedef SETCURSORPOS16 UNALIGNED *PSETCURSORPOS16;

typedef struct _SETDESKPATTERN16 {      /* u279 */
    VPSTR   f1;
} SETDESKPATTERN16;
typedef SETDESKPATTERN16 UNALIGNED *PSETDESKPATTERN16;

typedef struct _SETDESKWALLPAPER16 {    /* u285 */
    VPSTR   f1;
} SETDESKWALLPAPER16;
typedef SETDESKWALLPAPER16 UNALIGNED *PSETDESKWALLPAPER16;

typedef struct _SETDLGITEMINT16 {       /* u94 */
    BOOL16 f4;
    WORD f3;
    SHORT f2;
    HWND16 f1;
} SETDLGITEMINT16;
typedef SETDLGITEMINT16 UNALIGNED *PSETDLGITEMINT16;

typedef struct _SETDLGITEMTEXT16 {      /* u92 */
    VPSTR f3;
    SHORT f2;
    HWND16 f1;
} SETDLGITEMTEXT16;
typedef SETDLGITEMTEXT16 UNALIGNED *PSETDLGITEMTEXT16;

typedef struct _SETDOUBLECLICKTIME16 {      /* u20 */
    WORD f1;
} SETDOUBLECLICKTIME16;
typedef SETDOUBLECLICKTIME16 UNALIGNED *PSETDOUBLECLICKTIME16;

typedef struct _SETFOCUS16 {            /* u22 */
    HWND16 f1;
} SETFOCUS16;
typedef SETFOCUS16 UNALIGNED *PSETFOCUS16;

typedef struct _SETGETKBDSTATE16 {      /* u330 */
    VPBYTE  f1;
} SETGETKBDSTATE16;
typedef SETGETKBDSTATE16 UNALIGNED *PSETGETKBDSTATE16;

typedef struct _SETINTERNALWINDOWPOS16 {    /* u461 */
    VPPOINT16 f4;
    VPRECT16  f3;
    WORD      f2;
    HWND16    f1;
} SETINTERNALWINDOWPOS16;
typedef SETINTERNALWINDOWPOS16 UNALIGNED *PSETINTERNALWINDOWPOS16;

typedef struct _SETKEYBOARDSTATE16 {        /* u223 */
    VPBYTE f1;
} SETKEYBOARDSTATE16;
typedef SETKEYBOARDSTATE16 UNALIGNED *PSETKEYBOARDSTATE16;

typedef struct _SETMENU16 {         /* u158 */
    HMENU16 f2;
    HWND16 f1;
} SETMENU16;
typedef SETMENU16 UNALIGNED *PSETMENU16;

typedef struct _SETMENUITEMBITMAPS16 {      /* u418 */
    HBM16 f5;
    HBM16 f4;
    WORD f3;
    WORD f2;
    HMENU16 f1;
} SETMENUITEMBITMAPS16;
typedef SETMENUITEMBITMAPS16 UNALIGNED *PSETMENUITEMBITMAPS16;

typedef struct _SETMESSAGEQUEUE16 {     /* u266 */
    SHORT f1;
} SETMESSAGEQUEUE16;
typedef SETMESSAGEQUEUE16 UNALIGNED *PSETMESSAGEQUEUE16;

typedef struct _SETPARENT16 {           /* u233 */
    HWND16 f2;
    HWND16 f1;
} SETPARENT16;
typedef SETPARENT16 UNALIGNED *PSETPARENT16;

typedef struct _SETPROP16 {         /* u26 */
    HAND16 f3;
    VPSTR f2;
    HWND16 f1;
} SETPROP16;
typedef SETPROP16 UNALIGNED *PSETPROP16;

typedef struct _SETRECT16 {         /* u72 */
    SHORT f5;
    SHORT f4;
    SHORT f3;
    SHORT f2;
    VPRECT16 f1;
} SETRECT16;
typedef SETRECT16 UNALIGNED *PSETRECT16;

typedef struct _SETRECTEMPTY16 {        /* u73 */
    VPRECT16 f1;
} SETRECTEMPTY16;
typedef SETRECTEMPTY16 UNALIGNED *PSETRECTEMPTY16;

typedef struct _SETSCROLLPOS16 {        /* u62 */
    BOOL16 f4;
    SHORT f3;
    SHORT f2;
    HWND16 f1;
} SETSCROLLPOS16;
typedef SETSCROLLPOS16 UNALIGNED *PSETSCROLLPOS16;

typedef struct _SETSCROLLRANGE16 {      /* u64 */
    BOOL16 f5;
    SHORT f4;
    SHORT f3;
    SHORT f2;
    HWND16 f1;
} SETSCROLLRANGE16;
typedef SETSCROLLRANGE16 UNALIGNED *PSETSCROLLRANGE16;

typedef struct _SETSYSCOLORS16 {        /* u181 */
    VPLONG f3;
    VPSHORT f2;
    SHORT f1;
} SETSYSCOLORS16;
typedef SETSYSCOLORS16 UNALIGNED *PSETSYSCOLORS16;

typedef struct _SETSYSMODALWINDOW16 {       /* u188 */
    HWND16 f1;
} SETSYSMODALWINDOW16;
typedef SETSYSMODALWINDOW16 UNALIGNED *PSETSYSMODALWINDOW16;

typedef struct _SETSYSTEMMENU16 {       /* u280 */
    HMENU16 f2;
    HWND16  f1;
} SETSYSTEMMENU16;
typedef SETSYSTEMMENU16 UNALIGNED *PSETSYSTEMMENU16;

typedef struct _SETSYSTEMTIMER16 {      /* u11 */
    VPPROC  f4;
    WORD    f3;
    SHORT   f2;
    HWND16  f1;
} SETSYSTEMTIMER16;
typedef SETSYSTEMTIMER16 UNALIGNED *PSETSYSTEMTIMER16;

typedef struct _SETTIMER16 {            /* u10 */
    VPPROC f4;
    WORD f3;
    SHORT f2;
    HWND16 f1;
} SETTIMER16;
typedef SETTIMER16 UNALIGNED *PSETTIMER16;

typedef struct _SETTIMER216 {           /* u328 */
    VPPROC  f4;
    WORD    f3;
    SHORT   f2;
    HWND16  f1;
} SETTIMER216;
typedef SETTIMER216 UNALIGNED *PSETTIMER216;

typedef struct _SETWINDOWLONG16 {       /* u136 */
    LONG f3;
    SHORT f2;
    HWND16 f1;
} SETWINDOWLONG16;
typedef SETWINDOWLONG16 UNALIGNED *PSETWINDOWLONG16;

typedef struct _SETWINDOWPOS16 {        /* u232 */
    WORD f7;
    SHORT f6;
    SHORT f5;
    SHORT f4;
    SHORT f3;
    HWND16 f2;
    HWND16 f1;
} SETWINDOWPOS16;
typedef SETWINDOWPOS16 UNALIGNED *PSETWINDOWPOS16;

typedef struct _SETWINDOWSHOOKINTERNAL16 {      /* u121 */
    VPPROC f3;
    SHORT f2;
    SHORT f1;
} SETWINDOWSHOOKINTERNAL16;
typedef SETWINDOWSHOOKINTERNAL16 UNALIGNED *PSETWINDOWSHOOKINTERNAL16;

typedef struct _SETWINDOWTEXT16 {       /* u37 */
    VPSTR f2;
    HWND16 f1;
} SETWINDOWTEXT16;
typedef SETWINDOWTEXT16 UNALIGNED *PSETWINDOWTEXT16;

typedef struct _SETWINDOWWORD16 {       /* u134 */
    WORD f3;
    SHORT f2;
    HWND16 f1;
} SETWINDOWWORD16;
typedef SETWINDOWWORD16 UNALIGNED *PSETWINDOWWORD16;

typedef struct _SHOWCARET16 {           /* u167 */
    HWND16 f1;
} SHOWCARET16;
typedef SHOWCARET16 UNALIGNED *PSHOWCARET16;

typedef struct _SHOWCURSOR16 {          /* u71 */
    BOOL16 f1;
} SHOWCURSOR16;
typedef SHOWCURSOR16 UNALIGNED *PSHOWCURSOR16;

typedef struct _SHOWOWNEDPOPUPS16 {     /* u265 */
    BOOL16 f2;
    HWND16 f1;
} SHOWOWNEDPOPUPS16;
typedef SHOWOWNEDPOPUPS16 UNALIGNED *PSHOWOWNEDPOPUPS16;

typedef struct _SHOWSCROLLBAR16 {       /* u267 */
    BOOL16 f3;
    WORD f2;
    HWND16 f1;
} SHOWSCROLLBAR16;
typedef SHOWSCROLLBAR16 UNALIGNED *PSHOWSCROLLBAR16;

typedef struct _SHOWWINDOW16 {          /* u42 */
    SHORT f2;
    HWND16 f1;
} SHOWWINDOW16;
typedef SHOWWINDOW16 UNALIGNED *PSHOWWINDOW16;

typedef struct _SIGNALPROC16 {          /* u314 */
    LONG f4;
    WORD f3;
    WORD f2;
    HTASK16 f1;
} SIGNALPROC16;
typedef SIGNALPROC16 UNALIGNED *PSIGNALPROC16;

typedef struct _SNAPWINDOW16 {          /* u281 */
    HWND16  f1;
} SNAPWINDOW16;
typedef SNAPWINDOW16 UNALIGNED *PSNAPWINDOW16;

typedef struct _SWAPMOUSEBUTTON16 {     /* u186 */
    BOOL16 f1;
} SWAPMOUSEBUTTON16;
typedef SWAPMOUSEBUTTON16 UNALIGNED *PSWAPMOUSEBUTTON16;

typedef struct _SWITCHTOTHISWINDOW16 {  /* u172 */
    BOOL16  f2;
    HWND16  f1;
} SWITCHTOTHISWINDOW16;
typedef SWITCHTOTHISWINDOW16 UNALIGNED *PSWITCHTOTHISWINDOW16;

typedef struct _SWITCHWNDPROC16 {       /* u347 */
    LONG    f4;
    WORD    f3;
    WORD    f2;
    HWND16  f1;
} SWITCHWNDPROC16;
typedef SWITCHWNDPROC16 UNALIGNED *PSWITCHWNDPROC16;

typedef struct _SYSERRORBOX16 {         /* u320 */
    SHORT sBtn3;
    SHORT sBtn2;
    SHORT sBtn1;
    VPSZ  vpszCaption;
    VPSZ  vpszText;
} SYSERRORBOX16;
typedef SYSERRORBOX16 UNALIGNED *PSYSERRORBOX16;

typedef struct _TABBEDTEXTOUT16 {       /* u196 */
    SHORT f8;
    VPSHORT f7;
    SHORT f6;
    SHORT f5;
    VPSTR f4;
    SHORT f3;
    SHORT f2;
    HDC16 f1;
} TABBEDTEXTOUT16;
typedef TABBEDTEXTOUT16 UNALIGNED *PTABBEDTEXTOUT16;

typedef struct _TABTHETEXTOUTFORWIMPS16 { /* u354 */
    BOOL16   f9;
    SHORT    f8;
    VPSHORT  f7;
    SHORT    f6;
    SHORT    f5;
    VPSTR    f4;
    SHORT    f3;
    SHORT    f2;
    HDC16    f1;
} TABTHETEXTOUTFORWIMPS16;
typedef TABTHETEXTOUTFORWIMPS16 UNALIGNED *PTABTHETEXTOUTFORWIMPS16;

typedef struct _TILECHILDWINDOWS16 {    /* u199 */
    WORD    f2;
    HWND16  f1;
} TILECHILDWINDOWS16;
typedef TILECHILDWINDOWS16 UNALIGNED *PTILECHILDWINDOWS16;

typedef struct _TITLEWNDPROC16 {        /* u345 */
    LONG    f4;
    WORD    f3;
    WORD    f2;
    HWND16  f1;
} TITLEWNDPROC16;
typedef TITLEWNDPROC16 UNALIGNED *PTITLEWNDPROC16;

typedef struct _TRACKPOPUPMENU16 {      /* u416 */
    VPRECT16 f7;
    HWND16 f6;
    SHORT f5;
    SHORT f4;
    SHORT f3;
    WORD f2;
    HMENU16 f1;
} TRACKPOPUPMENU16;
typedef TRACKPOPUPMENU16 UNALIGNED *PTRACKPOPUPMENU16;

typedef struct _TRANSLATEACCELERATOR16 {    /* u178 */
    VPMSG16 f3;
    HAND16 f2;
    HWND16 f1;
} TRANSLATEACCELERATOR16;
typedef TRANSLATEACCELERATOR16 UNALIGNED *PTRANSLATEACCELERATOR16;

typedef struct _TRANSLATEMDISYSACCEL16 {    /* u451 */
    VPMSG16 f2;
    HWND16 f1;
} TRANSLATEMDISYSACCEL16;
typedef TRANSLATEMDISYSACCEL16 UNALIGNED *PTRANSLATEMDISYSACCEL16;

typedef struct _TRANSLATEMESSAGE16 {        /* u113 */
    VPMSG16 f1;
} TRANSLATEMESSAGE16;
typedef TRANSLATEMESSAGE16 UNALIGNED *PTRANSLATEMESSAGE16;

typedef struct _TRANSMITCOMMCHAR16 {        /* u206 */
    SHORT f2;
    SHORT f1;
} TRANSMITCOMMCHAR16;
typedef TRANSMITCOMMCHAR16 UNALIGNED *PTRANSMITCOMMCHAR16;

typedef struct _UNGETCOMMCHAR16 {       /* u212 */
    SHORT f2;
    SHORT f1;
} UNGETCOMMCHAR16;
typedef UNGETCOMMCHAR16 UNALIGNED *PUNGETCOMMCHAR16;

typedef struct _UNHOOKWINDOWSHOOK16 {       /* u234 */
    VPPROC f2;
    SHORT f1;
} UNHOOKWINDOWSHOOK16;
typedef UNHOOKWINDOWSHOOK16 UNALIGNED *PUNHOOKWINDOWSHOOK16;

typedef struct _UNIONRECT16 {           /* u80 */
    VPRECT16 f3;
    VPRECT16 f2;
    VPRECT16 f1;
} UNIONRECT16;
typedef UNIONRECT16 UNALIGNED *PUNIONRECT16;

typedef struct _UNREGISTERCLASS16 {     /* u403 */
    HAND16  hInstance;
    VPSTR   vpszClass;
} UNREGISTERCLASS16;
typedef UNREGISTERCLASS16 UNALIGNED *PUNREGISTERCLASS16;

typedef struct _UPDATEWINDOW16 {        /* u124 */
    HWND16 f1;
} UPDATEWINDOW16;
typedef UPDATEWINDOW16 UNALIGNED *PUPDATEWINDOW16;

typedef struct _USERSEEUSERDO16 {       /* u216 */
    LONG    f3;
    WORD    f2;
    WORD    f1;
} USERSEEUSERDO16;
typedef USERSEEUSERDO16 UNALIGNED *PUSERSEEUSERDO16;

#ifdef NULLSTRUCT
typedef struct _USERYIELD16 {           /* u332 */
} USERYIELD16;
typedef USERYIELD16 UNALIGNED *PUSERYIELD16;
#endif

typedef struct _VALIDATERECT16 {        /* u127 */
    VPRECT16 f2;
    HWND16 f1;
} VALIDATERECT16;
typedef VALIDATERECT16 UNALIGNED *PVALIDATERECT16;

typedef struct _VALIDATERGN16 {         /* u128 */
    HRGN16 f2;
    HWND16 f1;
} VALIDATERGN16;
typedef VALIDATERGN16 UNALIGNED *PVALIDATERGN16;

#ifdef NULLSTRUCT
typedef struct _WAITMESSAGE16 {         /* u112 */
} WAITMESSAGE16;
typedef WAITMESSAGE16 UNALIGNED *PWAITMESSAGE16;
#endif

typedef struct _WINDOWFROMPOINT16 {     /* u30 */
    POINT16 f1;
} WINDOWFROMPOINT16;
typedef WINDOWFROMPOINT16 UNALIGNED *PWINDOWFROMPOINT16;

typedef struct _WINHELP16 {         /* u171 */
    DWORD f4;
    WORD f3;
    VPSTR f2;
    HWND16 f1;
} WINHELP16;
typedef WINHELP16 UNALIGNED *PWINHELP16;

typedef struct _WINOLDAPPHACKOMATIC16 { /* u322 */
    LONG    f1;
} WINOLDAPPHACKOMATIC16;
typedef WINOLDAPPHACKOMATIC16 UNALIGNED *PWINOLDAPPHACKOMATIC16;

typedef struct _WRITECOMM16 {           /* u205 */
    SHORT f3;
    VPSTR f2;
    SHORT f1;
} WRITECOMM16;
typedef WRITECOMM16 UNALIGNED *PWRITECOMM16;

typedef struct _WSPRINTF16 {            /* u420 */
    VPSTR f2;
    VPSTR f1;
} WSPRINTF16;
typedef WSPRINTF16 UNALIGNED *PWSPRINTF16;

typedef struct _WVSPRINTF16 {           /* u421 */
    VPSTR f3;
    VPSTR f2;
    VPSTR f1;
} WVSPRINTF16;
typedef WVSPRINTF16 UNALIGNED *PWVSPRINTF16;

#ifdef NULLSTRUCT
typedef struct _XCSTODS16 {             /* u315 */
} XCSTODS16;
typedef XCSTODS16 UNALIGNED *PXCSTODS16;
#endif

typedef struct _SETWINDOWSHOOKEX16 {      /* u291 */
    HTASK16 f4;
    HAND16  f3;
    VPPROC  f2;
    SHORT   f1;
} SETWINDOWSHOOKEX16;
typedef SETWINDOWSHOOKEX16 UNALIGNED *PSETWINDOWSHOOKEX16;

typedef struct _UNHOOKWINDOWSHOOKEX16 {       /* u292 */
    HHOOK16 f1;
} UNHOOKWINDOWSHOOKEX16;
typedef UNHOOKWINDOWSHOOKEX16 UNALIGNED *PUNHOOKWINDOWSHOOKEX16;

typedef struct _CALLNEXTHOOKEX16 {         /* u293 */
    DWORD   f4;
    WORD    f3;
    SHORT   f2;
    HHOOK16 f1;
} CALLNEXTHOOKEX16;
typedef CALLNEXTHOOKEX16 UNALIGNED *PCALLNEXTHOOKEX16;

typedef struct _CLOSEDRIVER16 {    /* u253 */
    VPVOID f3;
    VPVOID f2;
    WORD f1;
} CLOSEDRIVER16;
typedef CLOSEDRIVER16 UNALIGNED *PCLOSEDRIVER16;

typedef struct _COPYCURSOR16 {     /* u369 */
    HAND16 f2;
    HAND16 f1;
} COPYCURSOR16;
typedef COPYCURSOR16 UNALIGNED *PCOPYCURSOR16;

typedef struct _COPYICON16 {       /* u368 */
    HAND16 f2;
    HAND16 f1;
} COPYICON16;
typedef COPYICON16 UNALIGNED *PCOPYICON16;

typedef struct _DEFDRIVERPROC16 {      /* u255 */
    VPVOID f5;
    VPVOID f4;
    WORD    f3;
    HAND16 f2;
    DWORD f1;
} DEFDRIVERPROC16;
typedef DEFDRIVERPROC16 UNALIGNED *PDEFDRIVERPROC16;

typedef struct _ENABLESCROLLBAR16 {    /* u482 */
    WORD f3;
    SHORT f2;
    HAND16 f1;
} ENABLESCROLLBAR16;
typedef ENABLESCROLLBAR16 UNALIGNED *PENABLESCROLLBAR16;

typedef struct _GETCLIPCURSOR16 {      /* u309 */
    VPRECT16 f1;
} GETCLIPCURSOR16;
typedef GETCLIPCURSOR16 UNALIGNED *PGETCLIPCURSOR16;

typedef struct _GETDCEX16 {    /* u359 */
    DWORD f3;
    WORD f2;
    HAND16 f1;
} GETDCEX16;
typedef GETDCEX16 UNALIGNED *PGETDCEX16;

typedef struct _GETDRIVERMODULEHANDLE16 {      /* u254 */
    HAND16 f1;
} GETDRIVERMODULEHANDLE16;
typedef GETDRIVERMODULEHANDLE16 UNALIGNED *PGETDRIVERMODULEHANDLE16;

typedef struct _GETDRIVERINFO16 {      /* u256 */
    VPVOID f2;
    HAND16 f1;
} GETDRIVERINFO16;
typedef GETDRIVERINFO16 UNALIGNED *PGETDRIVERINFO16;

typedef struct _GETFREESYSTEMRESOURCES16 {     /* u284 */
    WORD f1;
} GETFREESYSTEMRESOURCES16;
typedef GETFREESYSTEMRESOURCES16 UNALIGNED *PGETFREESYSTEMRESOURCES16;

typedef struct _GETNEXTDRIVER16 {      /* u257 */
    DWORD f2;
    HAND16 f1;
} GETNEXTDRIVER16;
typedef GETNEXTDRIVER16 UNALIGNED *PGETNEXTDRIVER16;

typedef struct _GETQUEUESTATUS16 {     /* u334 */
    WORD f1;
} GETQUEUESTATUS16;
typedef GETQUEUESTATUS16 UNALIGNED *PGETQUEUESTATUS16;

typedef struct _GETWINDOWPLACEMENT16 {     /* u370 */
    VPVOID f2;
    HAND16 f1;
} GETWINDOWPLACEMENT16;
typedef GETWINDOWPLACEMENT16 UNALIGNED *PGETWINDOWPLACEMENT16;

typedef struct _ISMENU16 {     /* u358 */
    HAND16 f1;
} ISMENU16;
typedef ISMENU16 UNALIGNED *PISMENU16;

typedef struct _LOCKINPUT16 {      /* u226 */
    BOOL16 f3;
    HAND16 f2;
    HAND16 f1;
} LOCKINPUT16;
typedef LOCKINPUT16 UNALIGNED *PLOCKINPUT16;

typedef struct _LOCKWINDOWUPDATE16 {       /* u294 */
    HAND16 f1;
} LOCKWINDOWUPDATE16;
typedef LOCKWINDOWUPDATE16 UNALIGNED *PLOCKWINDOWUPDATE16;

typedef struct _MAPWINDOWPOINTS16 {    /* u258 */
    WORD f4;
    VPVOID f3;
    HAND16 f2;
    HAND16 f1;
} MAPWINDOWPOINTS16;
typedef MAPWINDOWPOINTS16 UNALIGNED *PMAPWINDOWPOINTS16;

typedef struct _OPENDRIVER16 {     /* u252 */
    VPVOID f3;
    VPVOID f2;
    VPVOID f1;
} OPENDRIVER16;
typedef OPENDRIVER16 UNALIGNED *POPENDRIVER16;

typedef struct _QUERYSENDMESSAGE16 {       /* u184 */
    VPVOID f4;
    WORD f3;
    WORD f2;
    WORD f1;
} QUERYSENDMESSAGE16;
typedef QUERYSENDMESSAGE16 UNALIGNED *PQUERYSENDMESSAGE16;

typedef struct _REDRAWWWINDOW16 {      /* ux293 */
    WORD f4;
    WORD f3;
    VPVOID f2;
    HAND16 f1;
} REDRAWWWINDOW16;
typedef REDRAWWWINDOW16 UNALIGNED *PREDRAWWWINDOW16;

typedef struct _SCROLLWINDOWEX16 {     /* u319 */
    WORD f8;
    VPRECT16 f7;
    WORD f6;
    VPRECT16 f5;
    VPRECT16 f4;
    SHORT f3;
    SHORT f2;
    HAND16 f1;
} SCROLLWINDOWEX16;
typedef SCROLLWINDOWEX16 UNALIGNED *PSCROLLWINDOWEX16;

typedef struct _SENDDRIVERMESSAGE16 {      /* u251 */
    VPVOID f4;
    VPVOID f3;
    WORD f2;
    HAND16 f1;
} SENDDRIVERMESSAGE16;
typedef SENDDRIVERMESSAGE16 UNALIGNED *PSENDDRIVERMESSAGE16;

typedef struct _REDRAWWINDOW16 {       /* u290 */
    WORD f4;
    WORD f3;
    VPVOID f2;
    HAND16 f1;
} REDRAWWINDOW16;
typedef REDRAWWINDOW16 UNALIGNED *PREDRAWWINDOW16;


typedef struct _SETEVENTHOOK16 {     /* u321 */
    VPPROC f1;
} SETEVENTHOOK16;
typedef SETEVENTHOOK16 UNALIGNED *PSETEVENTHOOK16;

typedef struct _FILLWINDOW16 {     /* u324 */
    HBRSH16  f4;
    HDC16    f3;
    HWND16   f2;
    HWND16   f1;
} FILLWINDOW16;
typedef FILLWINDOW16 UNALIGNED *PFILLWINDOW16;

typedef struct _SETWINDOWPLACEMENT16 {     /* u371 */
    VPVOID f2;
    HAND16 f1;
} SETWINDOWPLACEMENT16;
typedef SETWINDOWPLACEMENT16 UNALIGNED *PSETWINDOWPLACEMENT16;

typedef struct _SUBTRACTRECT16 {       /* u373 */
    VPVOID f3;
    VPVOID f2;
    VPVOID f1;
} SUBTRACTRECT16;
typedef SUBTRACTRECT16 UNALIGNED *PSUBTRACTRECT16;

typedef struct _SYSTEMPARAMETERSINFO16 {       /* u483 */
    WORD f4;
    VPVOID f3;
    WORD f2;
    WORD f1;
} SYSTEMPARAMETERSINFO16;
typedef SYSTEMPARAMETERSINFO16 UNALIGNED *PSYSTEMPARAMETERSINFO16;

typedef struct _CURSORSHAPE16 { /* curs */
    SHORT xHotSpot;
    SHORT yHotSpot;
    SHORT cx;
    SHORT cy;
    SHORT cbWidth;  /* Bytes per row, accounting for word alignment. */
    BYTE Planes;
    BYTE BitsPixel;
} CURSORSHAPE16;
typedef CURSORSHAPE16 UNALIGNED *PCURSORSHAPE16;


typedef struct _MULTIKEYHELP16 { /* mkh */
    WORD    mkSize;
    BYTE    mkKeylist;
    BYTE    szKeyphrase[1];
} MULTIKEYHELP16;
typedef MULTIKEYHELP16 UNALIGNED *PMULTIKEYHELP16;


typedef struct _HELPWININFO16 { /* hwinfo */
    SHORT  wStructSize;
    SHORT  x;
    SHORT  y;
    SHORT  dx;
    SHORT  dy;
    SHORT  wMax;
    BYTE   rgchMember[2];
} HELPWININFO16;
typedef HELPWININFO16 UNALIGNED *PHELPWININFO16;

typedef struct _LOADACCEL16 {    /* ldaccel */
    WORD   hInst;
    WORD   hAccel;
    VPVOID pAccel;
    DWORD  cbAccel;
} LOADACCEL16;
typedef LOADACCEL16 UNALIGNED FAR *PLOADACCEL16;

typedef struct _NOTIFYWOW16 {           /* u535 */
    VPVOID pData;
    WORD   Id;
} NOTIFYWOW16;
typedef NOTIFYWOW16 UNALIGNED *PNOTIFYWOW16;


typedef struct _ICONCUR16 { /* iconcur */
    WORD   hInst;
    VPVOID lpStr;
} ICONCUR16;
typedef ICONCUR16 UNALIGNED *PICONCUR16;


typedef struct _WNETADDCONNECTION16 {         /* u517 */
    VPSTR f3;
    VPSTR f2;
    VPSTR f1;
} WNETADDCONNECTION16;
typedef WNETADDCONNECTION16 UNALIGNED *PWNETADDCONNECTION16;

typedef struct _WNETGETCONNECTION16 {         /* u512 */
    VPVOID f3;
    VPSTR f2;
    VPSTR f1;
} WNETGETCONNECTION16;
typedef WNETGETCONNECTION16 UNALIGNED *PWNETGETCONNECTION16;

typedef struct _WNETCANCELCONNECTION16 {         /* u518 */
    BOOL16 f2;
    VPSTR f1;
} WNETCANCELCONNECTION16;
typedef WNETCANCELCONNECTION16 UNALIGNED *PWNETCANCELCONNECTION16;

typedef struct _WINDOWPLACEMENT16 {              /* wp16wow32only */
    WORD    length;
    WORD    flags;
    WORD    showCmd;
    POINT16 ptMinPosition;
    POINT16 ptMaxPosition;
    RECT16  rcNormalPosition;
} WINDOWPLACEMENT16;
typedef WINDOWPLACEMENT16 UNALIGNED *LPWINDOWPLACEMENT16;


/* New in Win95 user16 */


typedef struct _ACTIVATEKEYBOARDLAYOUT16 {       /* u562 */
    WORD    wFlags;
    DWORD   lcid;
} ACTIVATEKEYBOARDLAYOUT16;
typedef ACTIVATEKEYBOARDLAYOUT16 UNALIGNED *PACTIVATEKEYBOARDLAYOUT16;

typedef struct _BROADCASTSYSTEMMESSAGE16 {       /* u554 */
    DWORD   lParam;
    WORD    wParam;
    WORD    wMsg;
    VPDWORD lpdwRecipients;
    DWORD   dwFlags;
} BROADCASTSYSTEMMESSAGE16;
typedef BROADCASTSYSTEMMESSAGE16 UNALIGNED *PBROADCASTSYSTEMMESSAGE16;

typedef struct _CALLMSGFILTER3216 {              /* u589 */
    WORD    fMsg32;
    WORD    wContext;
    VPVOID  lpMsg32;
} CALLMSGFILTER3216;
typedef CALLMSGFILTER3216 UNALIGNED *PCALLMSGFILTER3216;

typedef struct _CASCADEWINDOWS16 {               /* u429 */
    VPVOID   ahwnd;
    WORD     chwnd;
    VPRECT16 lpRect;
    WORD     wFlags;
    HWND16   hwndParent;
} CASCADEWINDOWS16;
typedef CASCADEWINDOWS16 UNALIGNED *PCASCADEWINDOWS16;

typedef struct _CHANGEDISPLAYSETTINGS16 {        /* u557 */
    DWORD       dwFlags;
    VPDEVMODE31 lpDevMode;
} CHANGEDISPLAYSETTINGS16;
typedef CHANGEDISPLAYSETTINGS16 UNALIGNED *PCHANGEDISPLAYSETTINGS16;

typedef struct _CHECKMENURADIOITEM16 {           /* u576 */
    WORD      wFlags;
    WORD      wIDCheck;
    WORD      wIDLast;
    WORD      wIDFirst;
    HMENU16   hmenu;
} CHECKMENURADIOITEM16;
typedef CHECKMENURADIOITEM16 UNALIGNED *PCHECKMENURADIOITEM16;

typedef struct _CHILDWINDOWFROMPOINTEX16 {              /* u399 */
    WORD      wFlags;
    POINT16   pt;
    HWND16    hwnd;
} CHILDWINDOWFROMPOINTEX16;
typedef CHILDWINDOWFROMPOINTEX16 UNALIGNED *PCHILDWINDOWFROMPOINTEX16;

typedef struct _CHOOSECOLOR_CALLBACK1616 {              /* u584 */
    DWORD     lParam;
    WORD      wParam;
    WORD      wMsg;
    HWND16    hwnd;
} CHOOSECOLOR_CALLBACK1616;
typedef CHOOSECOLOR_CALLBACK1616 UNALIGNED *PCHOOSECOLOR_CALLBACK1616;

typedef struct _CHOOSEFONT_CALLBACK1616 {              /* u580 */
    DWORD     lParam;
    WORD      wParam;
    WORD      wMsg;
    HWND16    hwnd;
} CHOOSEFONT_CALLBACK1616;
typedef CHOOSEFONT_CALLBACK1616 UNALIGNED *PCHOOSEFONT_CALLBACK1616;

typedef struct _COPYIMAGE16 {              /* u390 */
    WORD      wFlags;
    SHORT     cyNew;
    SHORT     cxNew;
    WORD      wType;
    HAND16    hImage;
    HINST16   hinstOwner;
} COPYIMAGE16;
typedef COPYIMAGE16 UNALIGNED *PCOPYIMAGE16;

typedef struct _CREATEICONFROMRESOURCEEX16 {              /* u450 */
    WORD      lrDesired;
    WORD      cyDesired;
    WORD      cxDesired;
    DWORD     dwVer;
    BOOL16    fIcon;
    DWORD     cbRes;
    VPVOID    lpRes;
} CREATEICONFROMRESOURCEEX16;
typedef CREATEICONFROMRESOURCEEX16 UNALIGNED *PCREATEICONFROMRESOURCEEX16;

typedef struct _DESTROYICON3216 {              /* u553 */
    WORD      wFlags;
    HICON16   hicon;
} DESTROYICON3216;
typedef DESTROYICON3216 UNALIGNED *PDESTROYICON3216;

#ifdef NULLSTRUCT
typedef struct _DISPATCHINPUT16 {              /* u569 */
} DISPATCHINPUT16;
typedef DISPATCHINPUT16 UNALIGNED *PDISPATCHINPUT16;
#endif

typedef struct _DISPATCHMESSAGE3216 {              /* u588 */
    BOOL16 fMsg32;
    VPVOID lpMsg32;
} DISPATCHMESSAGE3216;
typedef DISPATCHMESSAGE3216 UNALIGNED *PDISPATCHMESSAGE3216;

typedef struct _DLLENTRYPOINT16 {              /* u374 */
    WORD  f6;
    DWORD f5;
    WORD  f4;
    WORD  f3;
    WORD  f2;
    DWORD f1;
} DLLENTRYPOINT16;
typedef DLLENTRYPOINT16 UNALIGNED *PDLLENTRYPOINT16;

typedef struct _DOHOTKEYSTUFF16 {              /* u541 */
    WORD  fsModifiers;
    WORD  vk;
} DOHOTKEYSTUFF16;
typedef DOHOTKEYSTUFF16 UNALIGNED *PDOHOTKEYSTUFF16;

typedef struct _DRAWANIMATEDRECTS16 {              /* u448 */
    VPRECT16 lprcEnd;
    VPRECT16 lprcStart;
    SHORT    idAnimation;
    HWND16   hwndClip;
} DRAWANIMATEDRECTS16;
typedef DRAWANIMATEDRECTS16 UNALIGNED *PDRAWANIMATEDRECTS16;

typedef struct _DRAWCAPTION16 {              /* u571 */
    WORD     wFlags;
    VPRECT16 lprc;
    HDC16    hdc;
    HWND16   hwnd;
} DRAWCAPTION16;
typedef DRAWCAPTION16 UNALIGNED *PDRAWCAPTION16;

typedef struct _DRAWCAPTIONTEMP16 {              /* u568 */
    WORD     wFlags;
    VPSTR    lpText;
    HICON16  hicon;
    HFONT16  hfont;
    VPRECT16 lprc;
    HDC16    hdc;
    HWND16   hwnd;
} DRAWCAPTIONTEMP16;
typedef DRAWCAPTIONTEMP16 UNALIGNED *PDRAWCAPTIONTEMP16;

typedef struct _DRAWEDGE16 {              /* u570 */
    WORD     wFlags;
    WORD     wEdge;
    VPRECT16 lprc;
    HDC16    hdc;
} DRAWEDGE16;
typedef DRAWEDGE16 UNALIGNED *PDRAWEDGE16;

typedef struct _DRAWFRAMECONTROL16 {              /* u567 */
    WORD     wState;
    WORD     wType;
    VPRECT16 lprc;
    HDC16    hdc;
} DRAWFRAMECONTROL16;
typedef DRAWFRAMECONTROL16 UNALIGNED *PDRAWFRAMECONTROL16;

typedef struct _DRAWICONEX16 {              /* u394 */
    WORD     wFlags;
    HAND16   hbr;
    WORD     wStepIfAni;
    SHORT    cy;
    SHORT    cx;
    HICON16  hicon;
    SHORT    y;
    SHORT    x;
    HDC16    hdc;
} DRAWICONEX16;
typedef DRAWICONEX16 UNALIGNED *PDRAWICONEX16;

typedef struct _DRAWMENUBARTEMP16 {              /* u573 */
    HFONT16  hfont;
    HMENU16  hmenu;
    VPRECT16 lprc;
    HDC16    hdc;
    HWND16   hwnd;
} DRAWMENUBARTEMP16;
typedef DRAWMENUBARTEMP16 UNALIGNED *PDRAWMENUBARTEMP16;

typedef struct _DRAWSTATE16 {              /* u449 */
    WORD            uFlags;
    SHORT           cy;
    SHORT           cx;
    SHORT           y;
    SHORT           x;
    WORD            wData;
    DWORD           lData;
    VPPROC          pfnCallBack;
    HAND16          hbrFore;
    HDC16           hdcDraw;
} DRAWSTATE16;
typedef DRAWSTATE16 UNALIGNED *PDRAWSTATE16;


typedef struct _DRAWTEXTPARAMS16 {      /* dtp16 */
    WORD  cbSize;
    SHORT iTabLength;
    SHORT iLeftMargin;
    SHORT iRightMargin;
    WORD  uiLengthDrawn;
} DRAWTEXTPARAMS16;
typedef DRAWTEXTPARAMS16 UNALIGNED *PDRAWTEXTPARAMS16;

typedef struct _DRAWTEXTEX16 {              /* u375 */
    VPVOID            lpDTparams;     // see DRAWTEXTPARAMS16 above
    DWORD             dwDTformat;
    VPRECT16          lprc;
    SHORT             cchText;
    VPSTR             lpchText;
    HDC16             hdc;
} DRAWTEXTEX16;
typedef DRAWTEXTEX16 UNALIGNED *PDRAWTEXTEX16;

typedef struct _ENUMDISPLAYSETTINGS16 {              /* u560 */
    VPDEVMODE31   lpdm;
    DWORD         dwModeNum;
    VPSTR         lpszDeviceName;
} ENUMDISPLAYSETTINGS16;
typedef ENUMDISPLAYSETTINGS16 UNALIGNED *PENUMDISPLAYSETTINGS16;

typedef struct _FINDREPLACE_CALLBACK1616 {              /* u581 */
    DWORD     lParam;
    WORD      wParam;
    WORD      wMsg;
    HWND16    hwnd;
} FINDREPLACE_CALLBACK1616;
typedef FINDREPLACE_CALLBACK1616 UNALIGNED *PFINDREPLACE_CALLBACK1616;

typedef struct _FINDWINDOWEX16 {              /* u427 */
    VPSTR     lpszName;
    VPSTR     lpszClass;
    HWND16    hwndChild;
    HWND16    hwndParent;
} FINDWINDOWEX16;
typedef FINDWINDOWEX16 UNALIGNED *PFINDWINDOWEX16;

typedef struct _FORMATMESSAGE16 {              /* u556 */
    VPDWORD   rglArgs;
    WORD      cbResultMax;
    VPSTR     lpResult;
    WORD      idLanguage;
    WORD      idMessage;
    VPVOID    lpSource;
    DWORD     dwFlags;
} FORMATMESSAGE16;
typedef FORMATMESSAGE16 UNALIGNED *PFORMATMESSAGE16;

#ifdef NULLSTRUCT
typedef struct _GETAPPVER16 {              /* u498 */
} GETAPPVER16;
typedef GETAPPVER16 UNALIGNED *PGETAPPVER16;
#endif

typedef struct _GETCLASSINFOEX16 {              /* u398 */
    VPVOID    lpwc;
    VPSTR     lpszClassName;
    HINST16   hinst;
} GETCLASSINFOEX16;
typedef GETCLASSINFOEX16 UNALIGNED *PGETCLASSINFOEX16;

#ifdef NULLSTRUCT
typedef struct _GETFOREGROUNDWINDOW16 {              /* u558 */
} GETFOREGROUNDWINDOW16;
typedef GETFOREGROUNDWINDOW16 UNALIGNED *PGETFOREGROUNDWINDOW16;
#endif

typedef struct _ICONINFO16 {     /* ii16 */
   BOOL16  fIcon;
   SHORT   xHotspot;
   SHORT   yHotspot;
   HBM16   hbmMask;
   HBM16   hbmColor;
} ICONINFO16;
typedef ICONINFO16 UNALIGNED *PICONINFO16;

typedef struct _GETICONINFO16 {              /* u395 */
    VPVOID    lpiconinfo;          // see ICONINFO16 structure above
    HICON16   hicon;
} GETICONINFO16;
typedef GETICONINFO16 UNALIGNED *PGETICONINFO16;

typedef struct _GETKEYBOARDLAYOUT16 {              /* u563 */
    DWORD     dwThreadID;
} GETKEYBOARDLAYOUT16;
typedef GETKEYBOARDLAYOUT16 UNALIGNED *PGETKEYBOARDLAYOUT16;

typedef struct _GETKEYBOARDLAYOUTLIST16 {              /* u564 */
    VPDWORD   lpdwHandleArray;
    WORD      cElements;
} GETKEYBOARDLAYOUTLIST16;
typedef GETKEYBOARDLAYOUTLIST16 UNALIGNED *PGETKEYBOARDLAYOUTLIST16;

typedef struct _GETKEYBOARDLAYOUTNAME16 {              /* u477 */
    VPSTR lpszLayoutName;
} GETKEYBOARDLAYOUTNAME16;
typedef GETKEYBOARDLAYOUTNAME16 UNALIGNED *PGETKEYBOARDLAYOUTNAME16;

typedef struct _GETMENUCONTEXTHELPID16 {              /* u385 */
    HMENU16   hmenu;
} GETMENUCONTEXTHELPID16;
typedef GETMENUCONTEXTHELPID16 UNALIGNED *PGETMENUCONTEXTHELPID16;

typedef struct _GETMENUDEFAULTITEM16 {              /* u574 */
    WORD       wFlags;
    BOOL16     fByPosition;
    HMENU16    hmenu;
} GETMENUDEFAULTITEM16;
typedef GETMENUDEFAULTITEM16 UNALIGNED *PGETMENUDEFAULTITEM16;

typedef struct _MENUITEMINFO16 {  /* mii16 */
    DWORD   cbSize;
    DWORD   fMask;
    WORD    fType;
    WORD    fState;
    WORD    wID;
    HMENU16 hSubMenu;
    HBM16   hbmpChecked;
    HBM16   hbmpUnchecked;
    DWORD   dwItemData;
    VPSTR   dwTypeData;
    WORD    cch;
} MENUITEMINFO16;
typedef MENUITEMINFO16 UNALIGNED *PMENUITEMINFO16;

typedef struct _GETMENUITEMINFO16 {              /* u443 */
    VPVOID       lpmii;   // see MENUITEMINFO16 above
    BOOL16       fByPosition;
    WORD         wIndex;
    HMENU16      hmenu;
} GETMENUITEMINFO16;
typedef GETMENUITEMINFO16 UNALIGNED *PGETMENUITEMINFO16;

typedef struct _GETMENUITEMRECT16 {              /* u575 */
    VPRECT16     lprcScreen;
    WORD         wIndex;
    HMENU16      hmenu;
    HWND16       hwnd;
} GETMENUITEMRECT16;
typedef GETMENUITEMRECT16 UNALIGNED *PGETMENUITEMRECT16;

typedef struct _GETMESSAGE3216 {              /* u586 */
    WORD         fMsg32;
    WORD         wLast;
    WORD         wFirst;
    HWND16       hwnd16;
    VPVOID       lpMsg32;
} GETMESSAGE3216;
typedef GETMESSAGE3216 UNALIGNED *PGETMESSAGE3216;

typedef struct _GETPROPEX16 {              /* u379 */
    VPSTR        lpszKey;
    HWND16       hwnd;
} GETPROPEX16;
typedef GETPROPEX16 UNALIGNED *PGETPROPEX16;

typedef struct _GETSCROLLINFO16 {              /* u476 */
    VPVOID       lpsi;
    WORD         wCode;
    HWND16       hwnd;
} GETSCROLLINFO16;
typedef GETSCROLLINFO16 UNALIGNED *PGETSCROLLINFO16;

#ifdef NULLSTRUCT
typedef struct _GETSHELLWINDOW16 {              /* u540 */
} GETSHELLWINDOW16;
typedef GETSHELLWINDOW16 UNALIGNED *PGETSHELLWINDOW16;
#endif

typedef struct _GETSYSCOLORBRUSH16 {              /* u281 */
    WORD  wIndex;
} GETSYSCOLORBRUSH16;
typedef GETSYSCOLORBRUSH16 UNALIGNED *PGETSYSCOLORBRUSH16;

typedef struct _GETWINDOWCONTEXTHELPID16 {              /* u383 */
    HWND16       hwnd;
} GETWINDOWCONTEXTHELPID16;
typedef GETWINDOWCONTEXTHELPID16 UNALIGNED *PGETWINDOWCONTEXTHELPID16;

typedef struct _GETWINDOWRGN16 {              /* u579 */
    HRGN16       hrgn;
    HWND16       hwnd;
} GETWINDOWRGN16;
typedef GETWINDOWRGN16 UNALIGNED *PGETWINDOWRGN16;

typedef struct _HACKTASKMONITOR16 {              /* u555 */
    SHORT        iMonitor;
} HACKTASKMONITOR16;
typedef HACKTASKMONITOR16 UNALIGNED *PHACKTASKMONITOR16;

typedef struct _INITTHREADINPUT16 {              /* u409 */
    WORD         wFlags;
    HAND16       hq;
} INITTHREADINPUT16;
typedef INITTHREADINPUT16 UNALIGNED *PINITTHREADINPUT16;

typedef struct _INSERTMENUITEM16 {              /* u441 */
    VPVOID       lpmii;   // see MENUITEMINFO16 above
    BOOL16       fByPosition;
    WORD         wIndex;
    HMENU16      hmenu;
} INSERTMENUITEM16;
typedef INSERTMENUITEM16 UNALIGNED *PINSERTMENUITEM16;

typedef struct _INSTALLIMT16 {              /* u594 */
    WORD         wMsgHi;
    WORD         wMsgLo;
    VPPROC       pfnDispatcher;
    VPSTR        lpszClassName;
} INSTALLIMT16;
typedef INSTALLIMT16 UNALIGNED *PINSTALLIMT16;

typedef struct _ISDIALOGMESSAGE3216 {              /* u590 */
    BOOL16       fMsg32;
    VPVOID       lpMsg32;
    HWND16       hwnd;
} ISDIALOGMESSAGE3216;
typedef ISDIALOGMESSAGE3216 UNALIGNED *PISDIALOGMESSAGE3216;

typedef struct _LOADIMAGE16 {              /* u389 */
    WORD         wFlags;
    SHORT        cyDesired;
    SHORT        cxDesired;
    WORD         wType;
    VPSTR        lpszName;
    HINST16      hinst;
} LOADIMAGE16;
typedef LOADIMAGE16 UNALIGNED *PLOADIMAGE16;

typedef struct _LOADKEYBOARDLAYOUT16 {              /* u478 */
    WORD         wFlags;
    VPSTR        lpszLayoutName;
} LOADKEYBOARDLAYOUT16;
typedef LOADKEYBOARDLAYOUT16 UNALIGNED *PLOADKEYBOARDLAYOUT16;

typedef struct _LOOKUPICONIDFROMDIRECTORYEX16 {              /* u364 */
    WORD         lrDesired;
    SHORT        cyDesired;
    SHORT        cxDesired;
    BOOL16       fIcon;
    VPVOID       lpnh;
} LOOKUPICONIDFROMDIRECTORYEX16;
typedef LOOKUPICONIDFROMDIRECTORYEX16 UNALIGNED *PLOOKUPICONIDFROMDIRECTORYEX16;

typedef struct _MENUITEMFROMPOINT16 {              /* u479 */
    POINT16      ptScreen;
    HMENU16      hmenu;
    HWND16       hwnd;
} MENUITEMFROMPOINT16;
typedef MENUITEMFROMPOINT16 UNALIGNED *PMENUITEMFROMPOINT16;

typedef struct _MSGBOXPARAMS16 {                    /* mbp16 */
    DWORD       cbSize;
    HWND16      hwndOwner;
    HINST16     hInstance;
    VPSTR       lpszText;
    VPSTR       lpszCaption;
    DWORD       dwStyle;
    VPSTR       lpszIcon;
    DWORD       dwContextHelpId;
    DWORD       vpfnMsgBoxCallback;
    DWORD       dwLanguageId;
} MSGBOXPARAMS16;
typedef MSGBOXPARAMS16 UNALIGNED *PMSGBOXPARAMS16;

typedef struct _MESSAGEBOXINDIRECT16 {              /* u593 */
    VPVOID       lpmbp;
} MESSAGEBOXINDIRECT16;
typedef MESSAGEBOXINDIRECT16 UNALIGNED *PMESSAGEBOXINDIRECT16;

typedef struct _MSGWAITFORMULTIPLEOBJECTS16 {              /* u561 */
    DWORD   dwWakeMask;
    DWORD   dwMilliseconds;
    BOOL16  fWaitAll;
    VPDWORD lpHandles;
    DWORD   dwHandleCount;
} MSGWAITFORMULTIPLEOBJECTS16;
typedef MSGWAITFORMULTIPLEOBJECTS16 UNALIGNED *PMSGWAITFORMULTIPLEOBJECTS16;

typedef struct _OPENFILENAME_CALLBACK1616 {              /* u582 */
    DWORD     lParam;
    WORD      wParam;
    WORD      wMsg;
    HWND16    hwnd;
} OPENFILENAME_CALLBACK1616;
typedef OPENFILENAME_CALLBACK1616 UNALIGNED *POPENFILENAME_CALLBACK1616;

typedef struct _PEEKMESSAGE3216 {              /* u585 */
    BOOL16    fMsg32;
    WORD      wFlags;
    WORD      wLast;
    WORD      wFirst;
    HWND16    hwnd;
    VPVOID    lpMsg32;
} PEEKMESSAGE3216;
typedef PEEKMESSAGE3216 UNALIGNED *PPEEKMESSAGE3216;

typedef struct _PLAYSOUNDEVENT16 {              /* u8 */
    SHORT     iSoundId;
} PLAYSOUNDEVENT16;
typedef PLAYSOUNDEVENT16 UNALIGNED *PPLAYSOUNDEVENT16;

typedef struct _POSTMESSAGE3216 {              /* u591 */
    WORD      wParamHi;
    DWORD     lParam;
    WORD      wParamLo;
    WORD      wMsg;
    HWND16    hwnd;
} POSTMESSAGE3216;
typedef POSTMESSAGE3216 UNALIGNED *PPOSTMESSAGE3216;

#ifdef NULLSTRUCT
typedef struct _POSTPOSTEDMESSAGES16 {              /* u566 */
} POSTPOSTEDMESSAGES16;
typedef POSTPOSTEDMESSAGES16 UNALIGNED *PPOSTPOSTEDMESSAGES16;
#endif

typedef struct _POSTTHREADMESSAGE3216 {              /* u592 */
    WORD      wParamHi;
    DWORD     lParam;
    WORD      wParamLo;
    WORD      wMsg;
    DWORD     dwThreadID;
} POSTTHREADMESSAGE3216;
typedef POSTTHREADMESSAGE3216 UNALIGNED *PPOSTTHREADMESSAGE3216;

typedef struct _PRINTDLG_CALLBACK1616 {              /* u583 */
    DWORD     lParam;
    WORD      wParam;
    WORD      wMsg;
    HWND16    hwnd;
} PRINTDLG_CALLBACK1616;
typedef PRINTDLG_CALLBACK1616 UNALIGNED *PPRINTDLG_CALLBACK1616;

typedef struct _REGISTERCLASSEX16 {              /* u397 */
    VPVOID    lpwcex;
} REGISTERCLASSEX16;
typedef REGISTERCLASSEX16 UNALIGNED *PREGISTERCLASSEX16;

typedef struct _REMOVEPROPEX16 {              /* u380 */
    VPSTR     lpszKey;
    HWND16    hwnd;
} REMOVEPROPEX16;
typedef REMOVEPROPEX16 UNALIGNED *PREMOVEPROPEX16;

typedef struct _SETCHECKCURSORTIMER16 {              /* u542 */
    SHORT     iTime;
} SETCHECKCURSORTIMER16;
typedef SETCHECKCURSORTIMER16 UNALIGNED *PSETCHECKCURSORTIMER16;

typedef struct _SETFOREGROUNDWINDOW16 {              /* u559 */
    HWND16    hwnd;
} SETFOREGROUNDWINDOW16;
typedef SETFOREGROUNDWINDOW16 UNALIGNED *PSETFOREGROUNDWINDOW16;

typedef struct _SETMENUCONTEXTHELPID16 {              /* u384 */
    DWORD     dwContextHelpId;
    HMENU16   hmenu;
} SETMENUCONTEXTHELPID16;
typedef SETMENUCONTEXTHELPID16 UNALIGNED *PSETMENUCONTEXTHELPID16;

typedef struct _SETMENUDEFAULTITEM16 {              /* u543 */
    BOOL16    fByPosition;
    WORD      wIndex;
    HMENU16   hmenu;
} SETMENUDEFAULTITEM16;
typedef SETMENUDEFAULTITEM16 UNALIGNED *PSETMENUDEFAULTITEM16;

typedef struct _SETMENUITEMINFO16 {              /* u446 */
    VPVOID    lpmii;   // see MENUITEMINFO16 above
    BOOL16    fByPosition;
    WORD      wIndex;
    HMENU16   hmenu;
} SETMENUITEMINFO16;
typedef SETMENUITEMINFO16 UNALIGNED *PSETMENUITEMINFO16;

typedef struct _SETMESSAGEEXTRAINFO16 {              /* u376 */
    DWORD     dwExtraInfo;
} SETMESSAGEEXTRAINFO16;
typedef SETMESSAGEEXTRAINFO16 UNALIGNED *PSETMESSAGEEXTRAINFO16;

typedef struct _SETPROPEX16 {              /* u378 */
    DWORD     dwValue;
    VPSTR     lpszKey;
    HWND16    hwnd;
} SETPROPEX16;
typedef SETPROPEX16 UNALIGNED *PSETPROPEX16;

typedef struct _SETSCROLLINFO16 {              /* u475 */
    BOOL16    fRedraw;
    VPVOID    lpsi;
    SHORT     iCode;
    HWND16    hwnd;
} SETSCROLLINFO16;
typedef SETSCROLLINFO16 UNALIGNED *PSETSCROLLINFO16;

typedef struct _SETSYSCOLORSTEMP16 {              /* u572 */
    WORD      wBrushCount;
    VPWORD    lpBrushes;
    VPDWORD   lpRGBs;
} SETSYSCOLORSTEMP16;
typedef SETSYSCOLORSTEMP16 UNALIGNED *PSETSYSCOLORSTEMP16;

typedef struct _SETWINDOWCONTEXTHELPID16 {              /* u382 */
    DWORD     dwContextID;
    HWND16    hwnd;
} SETWINDOWCONTEXTHELPID16;
typedef SETWINDOWCONTEXTHELPID16 UNALIGNED *PSETWINDOWCONTEXTHELPID16;

typedef struct _SETWINDOWRGN16 {              /* u578 */
    BOOL16    fRedraw;
    HRGN16    hrgn;
    HWND16    hwnd;
} SETWINDOWRGN16;
typedef SETWINDOWRGN16 UNALIGNED *PSETWINDOWRGN16;

typedef struct _SIGNALPROC3216 {              /* u391 */
    DWORD     dwSignalID;
    DWORD     dwID;
    DWORD     dwFlags;
    HTASK16   htask;
} SIGNALPROC3216;
typedef SIGNALPROC3216 UNALIGNED *PSIGNALPROC3216;

typedef struct _TILEWINDOWS16 {              /* u428 */
    VPWORD    ahwnd;
    WORD      chwnd;
    VPRECT16  lprc;
    WORD      wFlags;
    HWND16    hwndParent;
} TILEWINDOWS16;
typedef TILEWINDOWS16 UNALIGNED *PTILEWINDOWS16;

typedef struct _TPMPARAMS16 {                     /* tpmp */
    WORD   cbSize;
    RECT16 rcExclude;
} TPMPARAMS16;
typedef TPMPARAMS16 UNALIGNED *PTPMPARAMS16;

typedef struct _TRACKPOPUPMENUEX16 {              /* u577 */
    VPVOID    lpTpm;                              /* see TPMPARAMS16 above */
    HWND16    hwndOwner;
    SHORT     y;
    SHORT     x;
    WORD      wFlags;
    HMENU16   hmenu;
} TRACKPOPUPMENUEX16;
typedef TRACKPOPUPMENUEX16 UNALIGNED *PTRACKPOPUPMENUEX16;

typedef struct _TRANSLATEMESSAGE3216 {              /* u587 */
    BOOL16    fMsg32;
    VPVOID    lpMsg32;
} TRANSLATEMESSAGE3216;
typedef TRANSLATEMESSAGE3216 UNALIGNED *PTRANSLATEMESSAGE3216;

typedef struct _UNINSTALLIMT16 {              /* u595 */
    WORD      wMsgHi;
    WORD      wMsgLo;
    VPPROC    pfnDispatcher;
    VPSTR     lpszClassName;
} UNINSTALLIMT16;
typedef UNINSTALLIMT16 UNALIGNED *PUNINSTALLIMT16;

typedef struct _UNLOADINSTALLABLEDRIVERS16 {              /* u300 */
    SHORT     iCode;
} UNLOADINSTALLABLEDRIVERS16;
typedef UNLOADINSTALLABLEDRIVERS16 UNALIGNED *PUNLOADINSTALLABLEDRIVERS16;

typedef struct _UNLOADKEYBOARDLAYOUT16 {              /* u565 */
    DWORD     lcid;
} UNLOADKEYBOARDLAYOUT16;
typedef UNLOADKEYBOARDLAYOUT16 UNALIGNED *PUNLOADKEYBOARDLAYOUT16;

typedef struct _WINDOWFROMDC16 {              /* u117 */
    HDC16     hdc;
} WINDOWFROMDC16;
typedef WINDOWFROMDC16 UNALIGNED *PWINDOWFROMDC16;

#ifdef NULLSTRUCT
typedef struct _WNETINITIALIZE16 {              /* u533 */
} WNETINITIALIZE16;
typedef WNETINITIALIZE16 UNALIGNED *PWNETINITIALIZE16;
#endif

typedef struct _WNETLOGON16 {              /* u534 */
    HWND16     hwndOwner;
    VPSTR      lpszProvider;
} WNETLOGON16;
typedef WNETLOGON16 UNALIGNED *PWNETLOGON16;


/* WOW private thunks in USER */


typedef struct _WOWWORDBREAKPROC16 {   /* u537 */
    SHORT   action;
    SHORT   cbEditText;
    SHORT   ichCurrentWord;
    VPVOID  lpszEditText;
} WOWWORDBREAKPROC16;
typedef WOWWORDBREAKPROC16 UNALIGNED *PWOWWORDBREAKPROC16;



// NOTE: these structs are also in mvdm\wow16\user\init.c 
//       UserInit16 && Krnl386Segs
//       - they must be the same!!!
typedef struct _USERCLIENTGLOBALS {   /* uclg */
    WORD             hInstance;
    BYTE UNALIGNED **lpgpsi;
    BYTE UNALIGNED **lpCsrFlag;
    DWORD            dwBldInfo;
    VPWORD           lpwMaxDWPMsg;
    VPSTR            lpDWPBits;
    WORD             cbDWPBits;
    WORD             wUnusedPadding;
    DWORD            pfnGetProcModule;
    DWORD UNALIGNED *lpHighestAddress;
} USERCLIENTGLOBALS;
typedef USERCLIENTGLOBALS UNALIGNED *PUSERCLIENTGLOBALS;

typedef struct _KRNL386SEGS {   /* uclg */
    WORD CodeSeg1;
    WORD CodeSeg2;
    WORD CodeSeg3;
    WORD DataSeg1;
} KRNL386SEGS;
typedef KRNL386SEGS UNALIGNED *PKRNL386SEGS;

/* XLATOFF */
#pragma pack()
/* XLATON */
