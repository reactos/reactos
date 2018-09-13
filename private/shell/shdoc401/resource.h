//
// Things needed by fsmenu.cpp...
//

//
//  unicpp\resource.h contains no cursors so we can safely start at 1.
//
#define IDC_MENUMOVE            1
#define IDC_MENUCOPY            2
#define IDC_MENUDENY            3

//
//  unicpp\resource.h starts allocating strings at 0x7600.
//
#define IDS_NONE                0x0001


//
// ids to disable context Help
//
#define IDC_NO_HELP_1                   650

//
// Icon indexes copied from shell32
//
#define IDI_SYSFILE             154      // system file (.54, .vxd, ...)

//
// global ids
//
#define IDC_GROUPBOX                    300

// Internet shortcut-related IDs
#define IDS_SHORT_NEW_INTSHCUT              0x2730
#define IDS_NEW_INTSHCUT                    0x2731
#define IDS_INVALID_URL_SYNTAX              0x2732
#define IDS_UNREGISTERED_PROTOCOL           0x2733
#define IDS_SHORTCUT_ERROR_TITLE            0x2734
#define IDS_IS_EXEC_FAILED                  0x2735
#define IDS_IS_EXEC_OUT_OF_MEMORY           0x2736
#define IDS_IS_EXEC_UNREGISTERED_PROTOCOL   0x2737
#define IDS_IS_EXEC_INVALID_SYNTAX          0x2738
#define IDS_IS_LOADFROMFILE_FAILED          0x2739
#define IDS_INTERNET_SHORTCUT               0x273E
#define IDS_URL_DESC_FORMAT                 0x273F
#define IDS_FAV_LASTVISIT                   0x2740
#define IDS_FAV_LASTMOD                     0x2741
#define IDS_FAV_WHATSNEW                    0x2742
#define IDS_IS_APPLY_FAILED                 0x2744
#define IDS_FAV_STRING                      0x2745


// File Type strings
#define IDS_FT                              0x2760
#define IDS_ADDNEWFILETYPE                  0x2761
#define IDS_FT_EDITTITLE                    0x2762
#define IDS_FT_CLOSE                        0x2763
#define IDS_FT_EXEFILE                      0x2764
#define IDS_FT_MB_EXTTEXT                   0x2765
#define IDS_FT_MB_NOEXT                     0x2766
#define IDS_FT_MB_NOACTION                  0x2767
#define IDS_FT_MB_EXETEXT                   0x2768
#define IDS_FT_MB_REMOVETYPE                0x2769
#define IDS_FT_MB_REMOVEACTION              0x276A
#define IDS_EXTTYPETEMPLATE                 0x276B
#define IDS_CAP_OPENAS                      0x276C
#define IDS_EXE                             0x276D
#define IDS_PROGRAMSFILTER_NT               0x276E
#define IDS_PROGRAMSFILTER_WIN95            0x276F
#define IDS_FT_EXTALREADYUSE                0x2770
#define IDS_FT_MB_REPLEXTTEXT               0x2771


// File Type dialogs
// Note: Don't duplicate IDS between these dialogs
// as code is in place to know which help file is
// used...

#define DLG_FILETYPEOPTIONS             1054
#define IDC_FT_PROP_LV_FILETYPES        1000
#define IDC_FT_PROP_NEW                 1001
#define IDC_FT_PROP_REMOVE              1002
#define IDC_FT_PROP_EDIT                1003
#define IDC_FT_PROP_DOCICON             1004
#define IDC_FT_PROP_DOCEXTRO_TXT        1005
#define IDC_FT_PROP_DOCEXTRO            1006
#define IDC_FT_PROP_OPENICON            1007
#define IDC_FT_PROP_OPENEXE_TXT         1008
#define IDC_FT_PROP_OPENEXE             1009
#define IDC_FT_PROP_CONTTYPERO_TXT      1011
#define IDC_FT_PROP_CONTTYPERO          1012
#define IDC_FT_PROP_COMBO               1013
#define IDC_FT_PROP_FINDEXT_TXT         1014

#define IDS_FOLDEROPTIONS               1030

#define IDS_DESKTOP                     1040

#define DLG_FILETYPEOPTIONSEDIT         1055
#define IDC_FT_EDIT_DOCICON             1100
#define IDC_FT_EDIT_CHANGEICON          1101
#define IDC_FT_EDIT_DESC                1103
#define IDC_FT_EDIT_EXTTEXT             1104
#define IDC_FT_EDIT_EXT                 1105
#define IDC_FT_EDIT_LV_CMDSTEXT         1106
#define IDC_FT_EDIT_LV_CMDS             1107
#define IDC_FT_EDIT_NEW                 1108
#define IDC_FT_EDIT_EDIT                1109
#define IDC_FT_EDIT_REMOVE              1110
#define IDC_FT_EDIT_DEFAULT             1111
#define IDC_FT_EDIT_QUICKVIEW           1112
#define IDC_FT_EDIT_SHOWEXT             1113
#define IDC_FT_EDIT_DESCTEXT            1114
#define IDC_FT_COMBO_CONTTYPETEXT       1115
#define IDC_FT_COMBO_CONTTYPE           1116
#define IDC_FT_COMBO_DEFEXTTEXT         1117
#define IDC_FT_COMBO_DEFEXT             1118
#define IDC_FT_EDIT_CONFIRM_OPEN        1119
#define IDC_FT_EDIT_BROWSEINPLACE       1120

#define DLG_FILETYPEOPTIONSCMD          1056
#define IDC_FT_CMD_ACTION               1200
#define IDC_FT_CMD_EXETEXT              1201
#define IDC_FT_CMD_EXE                  1202
#define IDC_FT_CMD_BROWSE               1203
#define IDC_FT_CMD_DDEGROUP             1204
#define IDC_FT_CMD_USEDDE               1205
#define IDC_FT_CMD_DDEMSG               1206
#define IDC_FT_CMD_DDEAPPNOT            1207
#define IDC_FT_CMD_DDETOPIC             1208
#define IDC_FT_CMD_DDEAPP               1209

