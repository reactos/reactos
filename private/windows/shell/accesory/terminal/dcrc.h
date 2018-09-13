/* ------- itrc.h defines dialog item lists and resource names --------- */

#define ICO_STOP           1
#define ICO_NOTE           2
#define ICO_CAUTION        3
#define ICO_QUESTION       4
#define ICO_DYNACOMM       255


/* String table constants */

#define STR_APPNAME        0x0001
#define STR_DEVELOPER      0x0002
#define STR_TERMINAL       0x0003
#define STR_SCRIPT         0x0004
#define STR_MEMO           0x0005
#define STR_ERRCAPTION     0x0006
#define STR_FATALERROR     0x0007
#define STR_TASKERROR      0x0008
#define STR_CREATEFILE     0x0009
#define STR_APPNAME_PRIVATE 0x000A /* jtf terminal */
#define STR_EXTRAINFO      0x000B

#define STR_VERSION        0x0021
#define STR_ICONLIBFILE    0x0022
#define STR_AUTOEXEC       0x0023
#define STR_AUTOLOAD       0x0024
#define STR_HELPFILE       0x0025
#define STR_BUILDFILE      0x0026
#define STR_TEMPFILE       0x0027
#define STR_COM            0x0028
#define STR_PATH           0x0029            /* mbbx 1.04: REZ... */
#define STR_COMMAND        0x002A
#define STR_MORE           0x002B

#define STR_PHONE          0x0041
#define STR_DIALPREFIX     0x0042
#define STR_DIALSUFFIX     0x0043
#define STR_HANGPREFIX     0x0044
#define STR_HANGSUFFIX     0x0045
#define STR_ANSWER         0x0046
#define STR_ORIGINATE      0x0047
#define STR_XFERLINESTR    0x0048
#define STR_ANSWERBACK     0x0049

#define STR_HIDETERMINAL   0x0061
#define STR_SHOWTERMINAL   0x0062
#define STR_HIDEFKEYS      0x0063
#define STR_SHOWFKEYS      0x0064
#define STR_STOP           0x0065            /* mbbx 2.00: xfer ctrls... */
#define STR_BREAK          0x0066
#define STR_PAUSE          0x0067
#define STR_RESUME         0x0068

#define STR_SAVE           0x0081
#define STR_SAVEAS         0x0082
#define STR_DELETE         0x0083
#define STR_PRINT          0x0084
#define STR_MERGE          0x0085
#define STR_SENDTEXTFILE   0x0086
#define STR_VIEWTEXTFILE   0x0087
#define STR_COMPILE        0x0088
#define STR_EXECUTE        0x0089
#define STR_EDIT           0x008A            /* mbbx 1.04: REZ... */
#define STR_LOAD           0x008B
#define STR_CREATE         0x008C

#define STR_RETRIES        0x00A1
#define STR_BYTECOUNT      0x00A2
#define STR_SENDING        0x00A3
#define STR_RECEIVING      0x00A4
#define STR_VIEWING        0x00A5
#define STR_FILENAME       0x00A6            /* mbbx 1.04: REZ... */
#define STR_DIRECTORY      0x00A7
#define STR_TO             0x00A8
#define STR_TOPIC          0x00A9
#define STR_LEVEL          0x00AA  /* jtf 3.Final */

#define STR_STOPTASK       0x0101
#define STR_STOPXFER       0x0102
#define STR_SAVECHANGES    0x0103
#define STR_OTHERCOM       0x0104
#define STR_OVERWRITEFILE  0x0105
#define STR_DELETING       0x0106            /* mbbx 1.04: REZ... */
#define STR_AREYOUSURE     0x0107
#define STR_XOFFSTATE      0x0108
#define STR_COMPILEOK	   0x0109
#define STR_TELNETFAIL	   0x010A // sdj: added this msg if telnet open fails
#define STR_SETCOMFAIL	   0x010B // sdj: added this msg if setcomstate fails

#define STR_NOCOMMPORTS    0x0181
#define STR_COMMNOTREADY   0x0182
#define STR_HANGUP         0x0183
#define STR_RETRYCOUNT     0x0184
#define STR_ABORTSND       0x0185
#define STR_ABORTRCV       0x0186
#define STR_LOADEMUL       0x0187            /* mbbx 1.04: REZ... */
#define STR_OUTOFMEMORY    0x0188
#define STR_COULDNOTEXEC   0x0189

#define STR_MINTIME        0x018A            /* jtf 3.20 */
#define STR_NOMEMORY	      0x018B
#define STR_PORTDISCONNECT 0x018C     // -sdj for telnet-quit processing

#define STREWRERR          0x01E1
#define STRFNOTFOUND       0x01E2
#define STRFINVALIDNAME    0x01E3
#define STRFERROPEN        0x01E4
#define STRFERRREAD        0x01E5
#define STRFERRFILELENGTH  0x01E6
#define STRFERRCLOSE       0x01E7
#define STRERRHANGUP       0x01E8
#define STRERRNOFILE       0x01E9
#define STRERRNOTIMERS     0x01EA   /* rjs bugs 006 */
#define STR_PRINTERROR     0x01EB   /* rjs bugs 013 */
#define STRDRIVEDIR	      0X01EC	

#define STR_INI_WINDOWS          0x0221
#define STR_INI_DEVICE           0x0222
#define STR_INI_EXTENSIONS       0x0223
#define STR_INI_COLORS           0x0224
#define STR_INI_WINDOW           0x0225
#define STR_INI_BKGDCOLOR        0x0226
#define STR_INI_WINDOWTEXT       0x0227
#define STR_INI_TEXTCOLOR        0x0228
#define STR_INI_INTL             0x0229
#define STR_INI_ICOUNTRY         0x022A
#define STR_INI_IDATE            0x022B
#define STR_INI_ITIME            0x022C
#define STR_INI_S1159            0x022D
#define STR_INI_S2359            0x022E
#define STR_INI_SDATE            0x022F
#define STR_INI_STIME            0x0230
#define STR_INI_PORTS            0x0231
#define STR_INI_DEVICES          0x0232
#define STR_INI_ON               0x0233
#define STR_INI_DEVICEMODE       0x0234
#define STR_INI_POSITION         0x0235
#define STR_INI_MAXIMIZED        0x0236
#define STR_INI_PORT             0x0237
#define STR_INI_SWAP             0x0238
#define STR_INI_SYSTEM           0x0239      /* mbbx 2.01.157 */
#define STR_INI_SETTINGS         0x023A
#define STR_INI_SCRIPT           0x023B
#define STR_INI_TASK             0x023C
#define STR_INI_MEMO             0x023D
#define STR_INI_DATA             0x023E
#define STR_INI_BUFFER           0x023F
#define STR_INI_FONT             0x0240
#define STR_INI_FONTFACE         0x0241
#define STR_INI_POINTER          0x0242 /* jtf gold 070 */
#define STR_INI_CONNECTORS       0x0243      /* slc nova 012 bjw gold 027 */
#define STR_INI_EMULOTHER        0x0244      /* slc nova 048 */
#define STR_INI_LISTBOXFONT      0x0245      /* slc nova 049 */
#define STR_INI_DCFKEYFONT	 0x0246      /* slc swat */
#define	STR_INI_XPOSITION	 0x0247	     /* sdj: added these so that terminal   */
#define	STR_INI_YPOSITION	 0x0248	     /* sdj: can remember the prev pos/size */
#define	STR_INI_WIDTH		 0x0249	     /* sdj: set by the user and use them in*/
#define	STR_INI_HEIGHT		 0x0250      /* sdj: CreateWindow arguments	    */


#define STR_FI                   0x0261
#define STR_RF                   0x0262
#define STR_DF                   0x0263
#define STR_RI                   0x0264
#define STR_RE                   0x0265
#define STR_SI                   0x0266
#define STR_SE                   0x0267


#define STR_KER_BADPACKET     0x02D1         /* mbbx 1.04: REZ... */
#define STR_KER_BADPACKNUM    0x02D2
#define STR_KER_RETRYABORT    0x02D3
#define STR_KER_SNDABORT      0x02D4
#define STR_KER_RCVABORT      0x02D5
#define STR_KER_CREATEFILE    0x02D6
#define STR_KER_XFERABORT     0x02D7
#define STR_KER_BADPACKTYPE   0x02D8
#define STR_KER_BADHOSTCMD    0x02DA
#define STR_KER_NEWPATH       0x02DC
#define STR_KER_BADPATH       0x02DD
#define STR_KER_BADDIRSPEC    0x02DE
#define STR_KER_DISKSPACE     0x02DF
#define STR_KER_GETDISKSPACE  0x02E0
#define STR_KER_DELETED       0x02E1
#define STR_KER_DELETEFILE    0x02E2
#define STR_KER_RENAMED       0x02E3
#define STR_KER_RENAMEFILE    0x02E4
#define STR_KER_COPIED        0x02E5
#define STR_KER_COPYFILE      0x02E6
#define STR_KER_HELP          0x02E7
#define STR_KER_BADCOMMAND    0x02E8
#define STR_KER_FILECOUNT     0x02E9
#define STR_KER_WHO           0x02ED         /* rkhx 2.00 */


/*- Added 02/13/91 Doug Wickstrom- for win 3.1 common dialog support -----*/
#define STR_FILTERTRM		0X02EE
#define STR_FILTERTXT		0X02EF
#define STR_FILTERALL		0X02F0


/* 0x0300 - reserved for FILEOPEN.RC */
/* 0x0400 - reserved for EDITFILE.RC */

#define STR_ERC_LIST       0x0800
#define STR_ERT_LIST       0x0C00

#define STR_NERR_LIST      0x0D00            /* mbbx 2.00: network... */
#define STR_MYNERR_LIST    0x0F00

#define STR_DIRS_LIST      0x1000
#define STR_CMDS_LIST      0x1100

#define STR_DLGS_LIST      0x2000
#define STR_EDITS_LIST     0x2100
#define STR_FILES_LIST     0x2200
#define STR_KEYS_LIST      0x2300            /* mbbx 1.04 */
#define STR_KBDS_LIST      0x2400
#define STR_MENUS_LIST     0x2500
#define STR_NETS_LIST      0x2580            /* mbbx 2.00: network */
#define STR_PRTS_LIST      0x2600
#define STR_RECS_LIST      0x2700
#define STR_SLCTS_LIST     0x2800
#define STR_SETS_LIST      0x2900
#define STR_STNGS_LIST     0x2A00
#define STR_TBLS_LIST      0x2B00
#define STR_VIDS_LIST      0x2C00            /* mbbx 1.03 */
#define STR_WAITS_LIST     0x2D00
#define STR_WHENS_LIST     0x2E00
#define STR_WNDS_LIST      0x2F00

#define STR_BINS_LIST      0x3000
#define STR_CNCTS_LIST     0x3100            /* mbbx 2.00: network */
#define STR_ICSS_LIST      0x3200
#define STR_DOCS_LIST      0x3300
#define STR_MDMS_LIST      0x3400
#define STR_PTRS_LIST      0x3500
#define STR_TERMS_LIST     0x3600
#define STR_TEXTS_LIST     0x3700
#define STR_OPTS_LIST      0x3800

#define STR_STRS_LIST      0x4000
#define STR_INTS_LIST      0x4100
#define STR_BOOLS_LIST     0x4200
#define STR_MISCS_LIST     0x4300
#define STR_EVALS_LIST     0x4400            /* mbbx 1.04: REZ... */

#define STR_ICS_NAME       0x5000            /* mbbx 1.04: ics */
#define STR_ICS_DATA       0x5020            /* mbbx 1.04: ics */
#define STR_COM_CONNECT    0x5080            /* mbbx 2.00 ... */
#define STR_MDM_HAYES      0x5100            /* mbbx 1.10: CUA... */
#define STR_MDM_TELEBIT    0x5120
#define STR_MDM_MNP        0x5140
#define STR_MDM_NONE       0x5160

/*----------------------------- Menu Commands -----------------------*/

#define POPUPMENUCOUNT     6                 /* mbbx 2.00: CUA 8 -> 9 */

#define FILEMENU           0
#define FMNEW              0x0101
#define FMOPEN             0x0102
#define FMCLOSE            0x0103
#define FMSAVE             0x0111
#define FMSAVEAS           0x0112
#define FMDELETE           0x0113
#define FMPRINT            0x0121
#define FMPRINTSETUP       0x0122
#define FMPRINTER          0x0131
#define FMTIMER            0x0132
#define FMMONITOR          0x0133
#define FMEXIT             0x01FF            /* mbbx 1.10: CUA */

#define EDITMENU           1
#define EMUNDO             0x0201
#define EMSELECTALL        0x0211
#define EMCUT              0x0221
#define EMCOPY             0x0222
#define EMPASTE            0x0223
#define EMCLEAR            0x0224
#define EMCOPYTABLE	      0x0225
#define EMCOPYBITMAP       0x0226
#define EMCOPYSPECIAL      0x022F            /* mbbx 2.00: CUA */
#define EMSETMARGIN        0x0231
#define EMSETTABWIDTH      0x0232            /* mbbx 2.00: CUA */
#define EMREFORMAT         0x0233
#define EMALIGN            0x0234
#define EMCENTER           0x0235
#define EMMERGE            0x0236
#define EMCOPYTHENPASTE    0x0241
#define EMSAVESELECT       0x0242
#define EMPRINTSELECT      0x0243
#define EMSAVESCREEN       0x0251

#define SETTINGSMENU       2
#define SMPHONE            0x0401
#define SMEMULATE          0x0402
#define SMTERMINAL         0x0403
#define SMFUNCTIONKEYS     0x0404
#define SMTEXTXFERS        0x0405
#define SMBINXFERS         0x0406
#define SMCOMMUNICATIONS   0x0407
#define SMMODEM            0x0408
#define WMFKEYS            0x0812

#define PHONEMENU          3
#define PMDIAL             0x0501
#define PMHANGUP           0x0502
#define PMWAITFORCALL      0x0503
#define PMREMOTEKERMIT     0x0511

#define TRANSFERMENU       4
#define TMSENDTEXTFILE     0x0601
#define TMRCVTEXTFILE      0x0602
#define TMVIEWTEXTFILE     0x0603
#define TMSENDBINFILE      0x0611
#define TMRCVBINFILE       0x0612
#define TMPAUSE            0x0621
#define TMRESUME           0x0622
#define TMSTOP             0x0623

#define HELPMENU           5
#define HMINDEX            0xFFFF
#define HMKEYBOARD         0x001E
#define HMMOUSE            0x001F
#define HMCOMMANDS         0x0020
#define HMPROCEDURES       0x0021
#define HMHELP             0xFFFC
#define HMABOUT            0x09FF
#define HMSEARCH           0x0105


/*---------------------------- Dialog Boxes -------------------------*/

/* standard item numbers */
#define ITMOK       1
#define ITMCANCEL   2

#define IDDBABOUT    99

#define IDDBSIGNON          87

/* Dialog box for printing abort */
#define IDABORTDLG          88

/* Dialog Box for Serial Port Initialization */
#define IDDBPORTINIT        89


/* Dialing Dialog box */

#define IDDBDIALING         94

#define IDDIALING                11
#define IDDIALTIME               12
#define IDDIALRETRY              13


/*---------------------------------------------------------------------------*/

#define IDDBMYCONTROLS      98

#define IDFK1               10
#define IDFK2               11
#define IDFK3               12
#define IDFK4               13
#define IDFK5               14
#define IDFK6               15
#define IDFK7               16
#define IDFK8               17
#define IDTIMER              9
#define IDMORE              24

#define IDFK9               18
#define IDFK10              19


/*---------------------------------------------------------------------------*/

#define IDDBXFERCTRLS       24               /* mbbx 1.04 ... */

#define IDGRAYBACK           0

#define IDSTOP          0x4000               /* mbbx 2.00 ... */
#define IDPAUSE         0x2000
#define IDFORK          0x1000
#define IDSCALE         0x0800
/* #define IDREMAIN        0x0400 */
#define IDSENDING       0x0200
#define IDBERRORS       0x0100


/* ----------------------- File : Type Selection  ------------------------- */

#define IDDBFILETYPE     20
#define ITMSETTINGS      3
#define ITMSCRIPT        4
#define ITMMEMO          5


/* File:PrinterSetup --------------------------------------------------------*/

#define IDDBPRTSETUP             25          /* mbbx/jtfprt... */

#define IDPRINTNAME              11
#define IDPRTSETUP               21


/* File:PrinterSetup --------------------------------------------------------*/

#define IDDBCOPYSPECIAL          26          /* mbbx 2.00 ... */

#define ITMPRINTER               11
#define ITMFILE                  12
#define ITMCLIPBOARD             13

#define ITMTEXT                  21
#define ITMTABLE                 22
#define ITMBITMAP                23


/* Edit:SetMargin, Edit:SetTabWidth, & Search:GotoLine ----------------------*/

#define IDDBSETMARGIN            21
#define IDDBSETTABWIDTH          22
#define IDDBGOTOLINE             23

#define IDNUMITEM                11
#define IDBOOLITEM               12


/* Settings:Phone Number ----------------------------------------------------*/

#define IDDBPHON                 1

#define ITMPHONENUMBER           11
#define ITMRETRY                 12
#define ITMRETRYTIME             13
#define ITMSIGNAL                14


/* Settings:Terminal Emulation ----------------------------------------------*/

#define IDDBEMUL                 2
// sdj 02jun92 it is ADDS 12 TTY 11, can change the order to get rid
// of the unknown control id problem, only tty,vt52,vt100 are in RC
// but lets not do this now, before checking the impact on dcutil1.c
// FindResource code where 1000 1002 1003 are the .bin resources
// maybe the order is important!
#define ITMTTY			 11
#define ITMADDS 		 12
#define ITMVT52                  13
#define ITMVT100                 14
#define ITMVT220                 15
#define ITMIBM3101               16
#define ITMTVI925                17
#define ITMVIDTEX                18
#define ITMDELTA                 19

#define ITMTERMFIRST		 ITMTTY
#define ITMTERMLAST		 ITMVIDTEX
// sdj 02jun02 possible fix for unknown control id problem
//#define ITMTERMLAST		  ITMVT100

#define ITMCTRLGRP               20
#define ITM7BITCTRLS             21
#define ITM8BITCTRLS             22


/* Settings:Terminal Preferences --------------------------------------------*/

#define IDDBTERM                 3

#define ITMLINEWRAP              11
#define ITMLOCALECHO             12
#define ITMSOUND                 13

#define ITMINCRLF                21
#define ITMOUTCRLF               22

#define ITM80COL                 31
#define ITM132COL                32

#define ITMBLKCURSOR             41
#define ITMUNDCURSOR             42
#define ITMBLINKCURSOR           48

#define ITMFONTFACE              51
#define ITMFONTSIZE              52

#define ITMTRANSLATE             91
#define ITMBUFFERLINES           92
#define ITMSCROLLBARS            93
#define ITMIBMXANSI              94          /* rjs swat */
#define ITMWINCTRL               95


/* Settings:Function Keys ---------------------------------------------------*/

#define IDDBFKEY                 4

#define ITMFKEYTITLE             10
#define ITMFKEYTEXT              20

#define ITMLEVEL1                31
#define ITMLEVEL2                32
#define ITMLEVEL3                33
#define ITMLEVEL4                34

#define ITMSHOWFKEYS             91
#define ITMAUTOARRANGE           92


/* Settings:Text Transfers --------------------------------------------------*/

#define IDDBTXTX                 5

#define ITMSTD                   11          /* ITMSTD, ITMCHR, ITMLIN */
#define ITMCHR                   12
#define ITMLIN                   13

#define ITMSTDGRP                16
#define ITMSTDXON                17
#define ITMSTDHARD               18
#define ITMSTDNONE               19

#define ITMCHRGRP                20          /* ITMCHRDELAY, ITMCHRWAIT */
#define ITMCHRDELAY              21
#define ITMCHRWAIT               22
#define ITMCHRTIME               23
#define ITMCHRUNITS              24

#define ITMLINGRP                30          /* ITMLINDELAY, ITMLINWAIT */
#define ITMLINDELAY              31
#define ITMLINWAIT               32
#define ITMLINTIME               33
#define ITMLINUNITS              34
#define ITMLINSTR                35

#define ITMWORDWRAP              41
#define ITMWRAPCOL               42


/* Settings:Binary Transfers ------------------------------------------------*/

#define IDDBBINX                 6

#define ITMDYNACOMM              11          /* mbbx 2.00: remove XTalk... */
#define ITMKERMIT                12
#define ITMXMODEM                13
#define ITMYMODEM                14
#define ITMYTERM                 15

#if OLD_CODE                                 /* mbbx 2.00: remove XTalk... */
#define ITMDYNACOMM              11
#define ITMCROSSTALK             12
#define ITMKERMIT                13
#define ITMXMODEM                14
#define ITMYMODEM                15
#define ITMYTERM                 16
#endif


/* Settings:Communications --------------------------------------------------*/

#define IDDBCOMM                 7

#define ITMSETUP                 3           /* rjs bug2 */

#define ITMBD110                 11
#define ITMBD300                 12
#define ITMBD600                 13          /* mbbx 2.00: support 600 baud */
#define ITMBD120                 14
#define ITMBD240                 15
#define ITMBD480                 16
#define ITMBD960                 17
#define ITMBD144		 18
#define ITMBD192		 19
#define ITMBD384		 20
#define ITMBD576		 21
#define ITMBD1152		 22


#define ITMDATA4		 23
#define ITMDATA5		 24
#define ITMDATA6		 25
#define ITMDATA7		 26
#define ITMDATA8		 27

#define ITMSTOP1                 31
#define ITMSTOP5                 32
#define ITMSTOP2                 33

#define ITMNOPARITY              41
#define ITMODDPARITY             42
#define ITMEVENPARITY            43
#define ITMMARKPARITY            44
#define ITMSPACEPARITY           45

#define ITMXONFLOW               51
#define ITMHARDFLOW              52
#define ITMNOFLOW                53
#define ITMETXFLOW               54          /* rjs bug2 */
#define ITMXONSTOPTRANS          55          /* rjs bug2 */
#define ITMXONSTOPRECEP          56          /* rjs bug2 */

#define ITMCONNECTOR             61

#define ITMPARITY                91
#define ITMCARRIER               92


#define ITMNOCOM                 0
                                             /* mbbx 2.00: network... */
#define ITMWINCOM                1
#define ITMCOM1                  (ITMWINCOM+0)
#define ITMCOM2                  (ITMWINCOM+1)
#define ITMCOM3                  (ITMWINCOM+2)
#define ITMCOM4                  (ITMWINCOM+3)
#define ITMCOMLAST               ITMCOM2
                                             /* mbbx 2.00: network... */
#define ITMNETCOM                2
#define ITMCOMBIOS               (ITMNETCOM+0)
#define ITMNETBIOS               (ITMNETCOM+1)
#define ITMUBNETCI               (ITMNETCOM+2)
#define ITMDEVICE                (ITMNETCOM+3)
#define ITMDECLAT                (ITMNETCOM+4)  /* rjs bug2 */
#define ITMDLLCONNECT            (ITMNETCOM+5)  /* rjs bug2 */


#define IDDBCOMBIOS              31
#define IDDBNETBIOS              32
#define IDDBUBNETCI              33
#define IDDBDEVICE               34

#define ITMPORT1                 11
#define ITMPORT2                 12
#define ITMPORT3                 13
#define ITMPORT4                 14

#define ITMEXT_STANDARD          20          /* ((ITMNETCOM*10)+0) */
#define ITMCOMBIOS_EICONX25      21          /* ((ITMCOMBIOS*10)+1) */
#define ITMCOMBIOS_ETHERTERM     22          /* ((ITMCOMBIOS*10)+2) */
#define ITMNETBIOS_UB            31          /* ((ITMNETBIOS*10)+1) */
#define ITMNETBIOS_ATT           32          /* ((ITMNETBIOS*10)+2) */
#define ITMDEVICE_EICON          51          /* ((ITMDEVICE*10)+1) */

#define ITMNETBIOS_SERVER        35
#define ITMNETBIOS_LNAME         36
#define ITMNETBIOS_RNAME         37
#define ITMNETBIOS_RCVTO         38
#define ITMNETBIOS_SNDTO         39
#define ITMDEVICE_NAME           59


/* Settings:Modem Commands --------------------------------------------------*/

#define IDDBMODEM                8

#define ITMDIAL                  10
#define ITMHANGUP                12
#define ITMBINTX                 14
#define ITMBINRX                 16
#define ITMFASTQRY               18
#define ITMANSWER                20
#define ITMORIGIN                21

/* #define ITMDEFAULTS             31          mbbx 2.00 */

/* #define ITMFASTLINK             41          mbbx 2.00 */
#define ITMHAYES                 41
#define ITMMULTITECH             42
#define ITMTRAILBLAZER           43
#define ITMNOMODEM               44

#define ITMMODEMFIRST            ITMHAYES
/* #define ITMMODEMFIRST         ITMFASTLINK    mbbx 2.00 */
#define ITMMODEMLAST             ITMNOMODEM


/* Phone:Remote Kermit ------------------------------------------------------*/

#define IDDBKERREMOTE            9

#define ITMKERHLP                11
#define ITMKERTYPE               12
#define ITMKERDEL                13
#define ITMKERCOPY               14
#define ITMKERREN                15
#define ITMKERMESS               16          /* rkhx 2.00 */
#define ITMKERCHDIR              17
#define ITMKERDIR                18
#define ITMKERFREE               19
#define ITMKERFINISH             20
#define ITMKERLGOUT              21
#define ITMKERWHO                22          /* rkhx 2.00 */

#define ITMKERFIRST              ITMKERHLP
#define ITMKERLAST               ITMKERLGOUT

#define ITMKERDECR1              31
#define ITMKERFNAME1             32
#define ITMKERDECR2              33
#define ITMKERFNAME2             34
#define ITMKEREXECUTE            35
#define ITMKERCANCEL             36


/*- Script:Compile ----------------------------------------------------------*/

#define IDDBCOMPILE              69

#define ITMPHASE1                11
#define ITMPHASE2                12
#define ITMPROGRESS              13
#define ITMPHASE3                14

#define ITMRESULT1               21
#define ITMRESULT2               22


/*---------------------------------------------------------------------------*/

#define DC_RES_CCTL              "CCTL"      /* mbbx 1.04: REZ */
