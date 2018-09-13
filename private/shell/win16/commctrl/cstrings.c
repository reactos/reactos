#include "ctlspriv.h"

#pragma data_seg(DATASEG_READONLY)

char const FAR c_szNULL[] = "";
char const FAR c_szSpace[] = " ";
char const FAR c_szTabControlClass[] = WC_TABCONTROL;
char const FAR c_szListViewClass[] = WC_LISTVIEW;
char const FAR c_szHeaderClass[] = WC_HEADER;
char const FAR c_szTreeViewClass[] = WC_TREEVIEW;
char const FAR c_szStatusClass[] = STATUSCLASSNAME;
char const FAR c_szSToolTipsClass[] = TOOLTIPS_CLASS;
char const FAR c_szToolbarClass[] = TOOLBARCLASSNAME;
#ifdef IEWIN31_25
char const FAR c_szReBarClass[] = REBARCLASSNAME;
#endif
char const FAR c_szEllipses[MAX_ELLIPSE];     // = "..."; loaded from IDS_ELLIPSES
char const FAR c_szShell[] = "Shell";

const char FAR s_szUpdownClass[] = UPDOWN_CLASS;
#ifndef WIN32
#ifdef WANT_SUCKY_HEADER
const char FAR s_szHeaderClass[] = HEADERCLASSNAME;
#endif
const char FAR s_szBUTTONLISTBOX[] = BUTTONLISTBOX;
#endif
const char FAR s_szHOTKEY_CLASS[] = HOTKEY_CLASS;
const char FAR s_szSTrackBarClass[] = TRACKBAR_CLASS;
const char FAR s_szPROGRESS_CLASS[] = PROGRESS_CLASS;

const char FAR c_szTTSubclass[] = "TTSubclass";

#ifdef IEWIN31_25
const char FAR c_szCC32Subclass[] = "CC32SubclassInfo";
#endif


#pragma data_seg()
