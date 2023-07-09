#define IDS_SPACE   0x0400
#define IDS_PLUS    0x0401
#define IDS_NONE    0x0402
#define IDS_ELLIPSES 0x0403

/* System MenuHelp
 */
#define MH_SYSMENU  (0x8000U - MINSYSCOMMAND)
#define IDS_SYSMENU (MH_SYSMENU-16)
#define IDS_HEADER  (MH_SYSMENU-15)
#define IDS_HEADERADJ   (MH_SYSMENU-14)
#define IDS_TOOLBARADJ  (MH_SYSMENU-13)

/* Cursor ID's
 */
#define IDC_SPLIT   100
#define IDC_MOVEBUTTON  102

#define IDC_STOP    103
#define IDC_COPY    104
#define IDC_MOVE    105
#define IDC_DIVIDER     106
#define IDC_DIVOPEN     107
#define IDC_HAND        108

#define IDB_STDTB_SMALL_COLOR   120
#define IDB_STDTB_LARGE_COLOR   121

#define IDB_VIEWTB_SMALL_COLOR  124
#define IDB_VIEWTB_LARGE_COLOR  125


/* Icon ID's
 */
#define IDI_INSERT  150

/* AdjustDlgProc stuff
 */
#define ADJUSTDLG   200
#define IDC_BUTTONLIST  201
#define IDC_RESET   202
#define IDC_CURRENT 203
#define IDC_REMOVE  204
#define IDC_APPHELP 205
#define IDC_MOVEUP  206
#define IDC_MOVEDOWN    207


// property sheet stuff
#define DLG_PROPSHEET           1006
#define DLG_PROPSHEETTABS       1007


// wizard property sheet stuff
#define DLG_WIZARD      1020


// if this id changes, it needs to change in shelldll as well.
// we need to find a better way of dealing with this.
#define IDS_CLOSE       0x1040
#define IDS_OK          0x1041
#define IDS_PROPERTIESFOR       0x1042

#define IDD_PAGELIST            0x3020
#define IDD_APPLYNOW            0x3021
#define IDD_DLGFRAME        0x3022
#define IDD_BACK        0x3023
#define IDD_NEXT        0x3024
#define IDD_FINISH      0x3025
#define IDD_DIVIDER     0x3026
