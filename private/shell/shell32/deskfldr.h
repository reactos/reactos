//
// the lowest sort order number goes at the top left of the desktop
//
#include "views.h"

// array indexes into g_asDesktopReqItems

#define CDESKTOP_REGITEM_DRIVES         0
#define CDESKTOP_REGITEM_NETWORK        1
#define CDESKTOP_REGITEM_INTERNET       2

EXTERN_C REQREGITEM g_asDesktopReqItems[];

//
// CAUTION: _CompareIDsOriginal() function in RegFldr.cpp has code that assumes that all 
// the "old" sort order values were <= 0x40. So, when it comes across a bOrder <= 0x40,
// it calls _GetOrder() function to get the "new" bOrder value. The following values have
// been bumped up to be above 0x40 sothat for all "new" values, we don't have to make that call. 
//
#define SORT_ORDER_MYDOCS       0x48    // coded in shell\ext\mydocs2\selfreg.inf
#define SORT_ORDER_DRIVES       0x50
#define SORT_ORDER_NETWORK      0x58
#define SORT_ORDER_RECYCLEBIN   0x60    // coded in shell32\selfreg.inx
#define SORT_ORDER_INETROOT     0x68

