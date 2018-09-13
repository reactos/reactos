#ifndef _IDS_H_
#define _IDS_H_
// IDs for common UI resources (note that these MUST BE decimal numbers)

#define IDI_SECURITY    1

#define IDS_HOUR    0x100
#define IDS_MIN     0x101
#define IDS_SEC     0x102

#define IDS_BYTES   0x103
#define IDS_ORDERKB 0x104
#define IDS_ORDERMB 0x105
#define IDS_ORDERGB 0x106
#define IDS_ORDERTB 0x107
#define IDS_ORDERPB 0x108
#define IDS_ORDEREB 0x109

#define IDS_LONGDATE_SEP    0x10a

#define IDS_SIDEBYSIDE 0x10F

// strings for GetRelativeDateFormat (note: sun-sat must be kept in order)
#define IDS_DAYSOFTHEWEEK               0x110
#define IDS_SUNDAY                      (IDS_DAYSOFTHEWEEK)
#define IDS_MONDAY                      (IDS_DAYSOFTHEWEEK+1)
#define IDS_TUESDAY                     (IDS_DAYSOFTHEWEEK+2)
#define IDS_WEDNESDAY                   (IDS_DAYSOFTHEWEEK+3)
#define IDS_THURSDAY                    (IDS_DAYSOFTHEWEEK+4)
#define IDS_FRIDAY                      (IDS_DAYSOFTHEWEEK+5)
#define IDS_SATURDAY                    (IDS_DAYSOFTHEWEEK+6)
#define IDS_TODAY                       0x117
#define IDS_YESTERDAY                   0x118

// strings for Zone stuff (security.cpp)
#define IDS_MIXED                       0x119
#define IDS_INTERNETSECURITY            0x11A

// strings for SHRestrictedMessageBox (util.cpp)
#define IDS_RESTRICTIONS                0x11B
#define IDS_RESTRICTIONSTITLE           0x11C
 
// Free spaces 0x11D thru 0x11F

#define IDS_DRIVENOTREADY               0x120
#define IDS_UNFORMATTED                 0x121
#define IDS_NOFMT                       0x122
#define IDS_NOSUCHDRIVE                 0x123
#define IDS_FILEERROR                   0x124
#define IDS_FILEERRORMOVE               0x124  // Same as IDS_FILEERROR because (IDS_FILEERROR + FO_MOVE) or FO_COPY, FO_DELETE, FO_RENAME
#define IDS_FILEERRORCOPY               0x125
#define IDS_FILEERRORDEL                0x126
#define IDS_FILEERRORREN                0x127
#define IDS_SECURITY                    0x128
#define IDS_TEMPLATENOTSECURE           0x129



#define DLG_MESSAGEBOXCHECK     0x1200
#define IDC_MBC_TEXT            0x1201
#define IDC_MBC_CHECK           0x1202
#define IDC_MBC_ICON            0x1203

#endif // _IDS_H_
