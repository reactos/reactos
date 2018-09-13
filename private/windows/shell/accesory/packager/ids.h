/*
 * ids.h - Header file for OLE demo's resource file.
 */


/* Application resource ID */
#define ID_APPLICATION  1
#define SPLIT           2

/* File menu */
#define IDM_UPDATE      0x101
#define IDM_IMPORT      0x102
#define IDM_EXPORT      0x103
#define IDM_EXIT        0x104
#define IDM_NEW         0x106

/* Edit menu */
#define POS_EDITMENU    1
#define IDM_UNDO        0x200
#define IDM_CUT         0x201
#define IDM_COPY        0x202
#define IDM_PASTE       0x203
#define IDM_PASTELINK   0x204
#define IDM_CLEAR       0x206
#define IDM_LINKS       0x208
#define IDM_INSERTICON  0x209
#define IDM_LABEL       0x20a
#define IDM_COMMAND     0x20b
#define IDM_COPYPACKAGE 0x20d
#define IDM_PICT        0x20e
#define IDM_DESC        0x20f
#define IDM_NEXTWINDOW  0x210

/* Object popup menu */
#define POS_OBJECT      12      // position of Object item in Edit menu
#define IDM_OBJECT      0x220
#define IDM_VERBMIN     0x221
#define IDM_VERBMAX     0x230


/* Help menu */
#define IDM_INDEX       0x0280
#define IDM_SEARCH      0x0281
#define IDM_USINGHELP   0x0282
#define IDM_ABOUT       0x0283


#define IDM_LINKDONE    0x307


/* Pop up menu */
#define IDM_EMBEDFILE   0x2c0
#define IDM_LINKFILE    0x2c1

/* String table constants */
#define IDS_APPNAME         0x100
#define IDS_UNTITLED        0x101
#define IDS_MAYBESAVE       0x102
#define IDS_FILTER          0x106
#define IDS_CHANGELINK      0x108
#define IDS_ALLFILTER       0x109
#define IDS_CONTENT         0x10f
#define IDS_DESCRIPTION     0x110
#define IDS_PICTURE         0x111
#define IDS_APPEARANCE      0x112
#define IDS_INSERTICON      0x113
#define IDS_VIEW            0x114
#define IDS_LINKTOFILE      0x115
#define IDS_IMPORTFILE      0x116
#define IDS_EXPORTFILE      0x117
#define IDS_EMBEDFILE       0x118
#define IDS_MAYBEUPDATE     0x119
#define IDS_FROZEN          0x11a
#define IDS_OBJECT          0x11b
#define IDS_ASKCLOSETASK    0x120
#define IDS_OVERWRITE       0x121
#define IDS_PRIMARY_VERB    0x122
#define IDS_SECONDARY_VERB  0x123
#define IDS_FAILEDUPDATE    0x124
#define IDS_OBJECT_MENU     0x125
#define IDS_UNDO_MENU       0x126
#define IDS_CONTENT_OBJECT  0x127
#define IDS_APPEARANCE_OBJECT 0x128
#define IDS_GENERIC         0x129
#define IDS_EDIT            0x12a
#define IDS_EMBNAME_CONTENT 0x12b
#define IDS_INVALID_FILENAME 0x12c
#define IDS_POPUPVERBS        0x12d
#define IDS_SINGLEVERB        0x12e

/* Error messages */
#define E_FAILED_TO_READ_FILE           0x201
#define E_FAILED_TO_SAVE_FILE           0x202
#define E_FAILED_TO_READ_OBJECT         0x206
#define E_FAILED_TO_DELETE_OBJECT       0x207
#define E_CLIPBOARD_COPY_FAILED         0x209
#define E_GET_FROM_CLIPBOARD_FAILED     0x20a
#define E_FAILED_TO_CREATE_CHILD_WINDOW 0x20b
#define E_FAILED_TO_CREATE_OBJECT       0x20c
#define E_UNEXPECTED_RELEASE            0x20e
#define E_FAILED_TO_LAUNCH_SERVER       0x20f
#define E_FAILED_TO_UPDATE              0x210
#define E_FAILED_TO_FREEZE              0x211
#define E_FAILED_TO_UPDATE_LINK         0x212
#define E_FAILED_TO_REGISTER_SERVER     0x214
#define E_FAILED_TO_REGISTER_DOCUMENT   0x215
#define E_FAILED_TO_RECONNECT_OBJECT    0x217
#define E_FAILED_TO_EXECUTE_COMMAND     0x21a
#define E_FAILED_TO_FIND_ASSOCIATION    0x21b

#define W_STATIC_OBJECT                 0x301
#define W_FAILED_TO_CLONE_UNDO          0x302
#define W_FAILED_TO_NOTIFY              0x305

#define IDS_AUTO                        0x400
#define IDS_MANUAL                      0x401
#define IDS_CANCELED                    0x402

#define IDS_BROWSE                      0x500
#define IDS_CHNGICONPROGS               0x508
#define IDS_ACCESSDENIED                0x509
#define IDS_LOWMEM                      0x510
#define IDS_NOICONSTITLE                0x511
#define IDS_NOICONSMSG                  0x512
#define IDS_NOZEROSIZEFILES             0x513
