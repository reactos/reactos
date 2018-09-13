typedef struct {
    BYTE    bMSB;
    BYTE    bLSB;
} MWORD;

typedef MWORD *LPMWORD;

#define MWORD2INT(mw)  ((mw).bMSB * 256 + (mw).bLSB)

typedef struct {
    MWORD   mwidPlatform;
    MWORD   mwidEncoding;
    MWORD   mwidLang;
    MWORD   mwidName;
    MWORD   mwcbString;
    MWORD   mwoffString;
} TTNAMEREC;

typedef TTNAMEREC *PTTNAMEREC;

typedef struct {
    MWORD   mwiFmtSel;
    MWORD   mwcNameRec;
    MWORD   mwoffStrings;
    TTNAMEREC   anrNames[1];
} TTNAMETBL;

typedef TTNAMETBL *PTTNAMETBL;


#define     TT_TBL_NAME         0x656D616E      // 'name'

#define     TTID_PLATFORM_MAC   1
#define     TTID_PLATFORM_MS    3

#define     TTID_MS_UNDEFINED   0
#define     TTID_MS_UNICODE     1
#define     TTID_MS_SHIFTJIS    2
#define     TTID_MS_GB          3
#define     TTID_MS_BIG5        4
#define     TTID_MS_WANSUNG     5

#define     TTID_NAME_COPYRIGHT  0
#define     TTID_NAME_FONTFAMILY 1
#define     TTID_NAME_FONTSUBFAM 2
#define     TTID_NAME_UNIQFONTID 3
#define     TTID_NAME_FULLFONTNM 4
#define     TTID_NAME_VERSIONSTR 5
#define     TTID_NAME_PSFONTNAME 6
#define     TTID_NAME_TRADEMARK  7
