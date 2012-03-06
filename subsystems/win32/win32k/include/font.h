#pragma once

// FIXME: duplicated from ntoskrnl, should got to NDK or something
#define InterlockedIncrementUL(Addend) \
    (ULONG)InterlockedIncrement((PLONG)(Addend))

#define InterlockedDecrementUL(Addend) \
    (ULONG)InterlockedDecrement((PLONG)(Addend))

#define LFONT_GetObject FontGetObject

#define ENGAPI

#define PENALTY_Max                    127979
#define PENALTY_CharSet                65000
#define PENALTY_OutputPrecision        19000
#define PENALTY_FixedPitch             15000
#define PENALTY_FaceName               10000
#define PENALTY_Family                  9000
#define PENALTY_FamilyUnknown           8000
#define PENALTY_HeightBigger             600
#define PENALTY_FaceNameSubst            500
#define PENALTY_PitchVariable            350
#define PENALTY_HeightSmaller            150
#define PENALTY_HeightBiggerDifference   150
#define PENALTY_FamilyUnlikely            50
#define PENALTY_Width                     50
#define PENALTY_SizeSynth                 50
#define PENALTY_Aspect                    30
#define PENALTY_IntSizeSynth              20
#define PENALTY_UnevenSizeSynth            4
#define PENALTY_Italic                     4
#define PENALTY_NotTrueType                4
#define PENALTY_Weight                     3
#define PENALTY_Underline                  3
#define PENALTY_StrikeOut                  3
#define PENALTY_VectorHeightSmaller        2
#define PENALTY_DeviceFavor                2
#define PENALTY_ItalicSim                  1
#define PENALTY_DefaultPitchFixed          1
#define PENALTY_SmallPenalty               1
#define PENALTY_VectorHeightBigger         1


#define FD_MAX_FILES 4

extern PEPROCESS gpepCSRSS;

typedef struct _FONTSUBSTITUTE
{
    WCHAR awcSubstName[LF_FACESIZE];
    WCHAR awcCapSubstName[LF_FACESIZE];
    ULONG iHashValue;
} FONTSUBSTITUTE, *PFONTSUBSTITUTE;


typedef struct _POINTEF
{
    FLOATOBJ x;
    FLOATOBJ y;
} POINTEF, *PPOINTEF;

typedef struct _RFONT *PRFONT;

typedef enum _FLPFE
{
    PFE_DEVICEFONT  = 0x0001,
    PFE_DEADSTATE   = 0x0002,
    PFE_REMOTEFONT  = 0x0004,
    PFE_EUDC        = 0x0008,
    PFE_SBCS_SYSTEM = 0x0010,
    PFE_UFIMATCH    = 0x0020,
    PFE_MEMORYFONT  = 0x0040,
    PFE_DBCS_FONT   = 0x0080,
    PFE_VERT_FACE   = 0x0100,
} FLPFE;

typedef struct _PFE
{
    struct _PFF * pPFF;
    ULONG_PTR iFont;
    FLPFE flPFE;
    FD_GLYPHSET *pfdg;
    ULONG_PTR idfdg;
    IFIMETRICS * pifi;
    ULONG_PTR idifi;
    FD_KERNINGPAIR *pkp;
    ULONG_PTR idkp;
    ULONG ckp;
    ULONG iOrientation;
    ULONG cjEfdwPFE;
    //GISET * pgiset;
    ULONG ulTimeStamp;
    UNIVERSAL_FONT_ID ufi;
    DWORD pid;
    DWORD tid;
    LIST_ENTRY ql;
    void * pFlEntry;
    ULONG cAlt;
    ULONG cPfdgRef;
    ULONG aiFamilyName[1];
} PFE, *PPFE;

typedef struct _PFEOBJ
{
    BASEOBJECT baseobj;
    PPFE ppfe;
} PFEOBJ, *PPFEOBJ;

typedef struct _PFF
{
    ULONG sizeofThis;
    struct _PFF *pPFFNext;
    struct _PFF *pPFFPrev;
    PWSTR pwszPathname;
    ULONG cwc;
    ULONG iFileNameHash;
    ULONG cFiles;
    FLONG flState;
    ULONG cLoaded;
    ULONG cNotEnum;
    ULONG cRFONT;
    PRFONT prfntList;
    HFF hff;
    HDEV hdev;
    DHPDEV dhpdev;
    void *pfhFace;
    void *pfhFamily;
    void *pfhUFI;
    struct _PFT *pPFT;
    ULONG ulCheckSum;
    ULONG cFonts;
    void *pPvtDataHead;
    PFONTFILEVIEW apffv[FD_MAX_FILES];
    PFE apfe[];
} PFF, *PPFF;

typedef struct _PFELINK
{
    struct _PFELINK *ppfelNext;
    PPFE ppfe;
} PFELINK, *PPFELINK;

typedef struct _HASHBUCKET
{
    struct _HASHBUCKET *pbktCollision;
    PFELINK *ppfelEnumHead;
    PFELINK *ppfelEnumTail;
    ULONG cTrueType;
    ULONG iHashValue;
    FLONG fl;
    struct _HASHBUCKET * pbktPrev;
    struct _HASHBUCKET * pbktNext;
    ULONG ulTime;
    union
    {
        WCHAR wcCapName[LF_FACESIZE]; // LF_FULLFACESIZE? dynamic?
        UNIVERSAL_FONT_ID ufi;
    } u;
} HASHBUCKET, *PHASHBUCKET;

typedef enum _FONT_HASH_TYPE
{
    FHT_FACE = 0,
    FHT_FAMILY = 1,
    FHT_UFI = 2,
} FONT_HASH_TYPE;

typedef struct _FONTHASH
{
    DWORD id;
    FONT_HASH_TYPE fht;
    ULONG cBuckets;
    ULONG cUsed;
    ULONG cCollisions;
    PHASHBUCKET pbktFirst;
    PHASHBUCKET pbktLast;
    PHASHBUCKET apbkt[];
} FONTHASH, *PFONTHASH;

#define MAX_FONT_LIST 100
typedef struct _PFT
{
    PFONTHASH  pfhFamily;
    PFONTHASH  pfhFace;
    PFONTHASH  pfhUFI;
    ULONG      cBuckets;
    ULONG      cFiles;
    PPFF       apPFF[MAX_FONT_LIST];

    /* ROS specific */
    HSEMAPHORE hsem;
} PFT, *PPFT;

typedef struct _RFONTLINK
{
    PRFONT prfntPrev;
    PRFONT prfntNext;
} RFONTLINK, *PRFONTLINK;

typedef struct _CACHE
{
    GLYPHDATA *pgdNext;
    GLYPHDATA *pgdThreshold;
    PBYTE pjFirstBlockEnd;
    void *pdblBase;
    ULONG cMetrics;
    ULONG cjbbl;
    ULONG cBlocksMax;
    ULONG cBlocks;
    ULONG cGlyphs;
    ULONG cjTotal;
    void * pbblBase;
    void * pbblCur;
    GLYPHBITS * pgbNext;
    GLYPHBITS * pgbThreshold;
    PBYTE pjAuxCacheMem;
    ULONG cjAuxCacheMem;
    ULONG cjGlyphMax;
    BOOL bSmallMetrics;
    ULONG iMax;
    ULONG iFirst;
    ULONG cBits;
} CACHE;

typedef struct _RFONT
{
    FONTOBJ fobj;

    HDEV hdevProducer;
    HDEV hdevConsumer;
    DHPDEV dhpdev;

    PFE * ppfe;
    PFF * ppff;
    RFONTLINK rflPDEV;
    RFONTLINK rflPFF;
    PRFONT prfntSystemTT;
    PRFONT prfntSysEUDC;
    PRFONT prfntDefEUDC;
    PRFONT *paprfntFaceName;
    PRFONT aprfntQuickBuff[8];

    FLONG flType;
    FLONG flInfo;
    FLONG flEUDCState;
    ULONG ulContent;
    ULONG ulTimeStamp;
    ULONG ulNumLinks;
    ULONG iGraphicsMode;
    ULONG ulOrientation;
    ULONG cBitsPerPel;
    struct
    {
        ULONG bVertical:1;
        ULONG bDeviceFont:1;
        ULONG bIsSystemFont:1;
        ULONG bNeededPaths:1;
        ULONG bFilledEudcArray:1;
    };


    FD_XFORM fdx;
    FD_DEVICEMETRICS fddm;
    FIX fxMaxExtent;
    POINTFX ptfxMaxAscent;
    POINTFX ptfxMaxDescent;
    TEXTMETRICW *ptmw;
    FD_GLYPHSET *pfdg;
    HGLYPH *phg;
    HGLYPH hgBreak;
    FIX fxBreak;
    void *wcgp;


    MATRIX mxWorldToDevice;
    MATRIX mxForDDI;
    //EXFORMOBJ xoForDDI;

    POINTEF pteUnitAscent;
    FLOATOBJ efWtoDAscent;
    FLOATOBJ efDtoWAscent;

    POINTEF pteUnitBase;
    FLOATOBJ efWtoDBase;
    FLOATOBJ efDtoWBase;

    FLOATOBJ efWtoDEsc;
    FLOATOBJ efDtoWEsc;
    FLOATOBJ efEscToBase;
    FLOATOBJ efEscToAscent;
    FLOATOBJ efDtoWBase_31;
    FLOATOBJ efDtoWAscent_31;

    POINTEF eptflNtoWScale;

    ULONG bNtoWIdent;
    LONG lCharInc;
    LONG lMaxAscent;
    LONG lMaxHeight;
    LONG lAscent;
    LONG lEscapement;
    ULONG cSelected;

    HSEMAPHORE hsemCache;
    HSEMAPHORE hsemCache1;
    CACHE cache;

    POINTL ptlSim;
} RFONT;

typedef struct _LFONT
{
    BASEOBJECT baseobj;
    LFTYPE lft;
    FLONG fl;
    PPDEVOBJ ppdev;
    HGDIOBJ hPFE;
    ULONG iNameHash;
    WCHAR awchFace[LF_FACESIZE];
    ENUMLOGFONTEXDVW elfexw;
} LFONT, *PLFONT;

enum _ESTROBJ_FLAGS
{
    TO_MEM_ALLOCATED  = 0x0001,
    TO_ALL_PTRS_VALID = 0x0002,
    TO_VALID          = 0x0004,
    TO_ESC_NOT_ORIENT = 0x0008,
    TO_PWSZ_ALLOCATED = 0x0010,
    TSIM_UNDERLINE1   = 0x0020,
    TSIM_UNDERLINE2   = 0x0040,
    TSIM_STRIKEOUT    = 0x0080,
    TO_HIGHRESTEXT    = 0x0100,
    TO_BITMAPS        = 0x0200,
    TO_PARTITION_INIT = 0x0400,
    TO_ALLOC_FACENAME = 0x0800,
    TO_SYS_PARTITION  = 0x1000,
};

typedef struct _ESTROBJ
{
    STROBJ    stro; // Text string object header.
    ULONG     cgposCopied;
    ULONG     cgposPositionsEnumerated;
    PRFONT    prfnt;
    FLONG     flTO;
    PGLYPHPOS pgpos;
    POINTFIX  ptfxRef;
    POINTFIX  ptfxUpdate;
    POINTFIX  ptfxEscapement;
    RECTFX    rcfx;
    FIX       fxExtent;
    FIX       fxExtra;
    FIX       fxBreakExtra;
    DWORD     dwCodePage;
    ULONG     cExtraRects;
    RECTL     arclExtra[3];
    RECTL     rclBackGroundSave;
    PWCHAR    pwcPartition;
    PLONG     plPartition;
    PLONG     plNext;
    PGLYPHPOS pgpNext;
    PLONG     plCurrentFont;
    POINTL    ptlBaseLineAdjust;
    INT       cTTSysGlyphs;
    INT       cSysGlyphs;
    INT       cDefGlyphs;
    INT       cNumFaceNameGlyphs;
    PVOID     pacFaceNameGlyphs;
    ULONG     acFaceNameGlyphs[8];
} ESTROBJ, *PESTROBJ;

extern PRFONT gprfntSystemTT;
extern PFT gpftPublic;

FORCEINLINE
PLFONT
LFONT_ShareLockFont(HFONT hfont)
{
    return (PLFONT)GDIOBJ_ReferenceObjectByHandle(hfont, GDIObjType_LFONT_TYPE);
}

FORCEINLINE
VOID
LFONT_ShareUnlockFont(PLFONT plfnt)
{
    GDIOBJ_vDereferenceObject(&plfnt->baseobj);
}

PPFE
NTAPI
LFONT_ppfe(PLFONT plfnt);

PRFONT
NTAPI
LFONT_prfntFindLinkedRFONT(
    _In_ PLFONT plfnt,
    _In_ PMATRIX pmxWorldToDevice);

VOID
NTAPI
UpcaseString(
    OUT PWSTR pwszDest,
    IN PWSTR pwszSource,
    IN ULONG cwc);

ULONG
NTAPI
CalculateNameHash(
    PWSTR pwszName);

ULONG
NTAPI
PFE_ulQueryTrueTypeTable(
    PPFE ppfe,
    ULONG ulTableTag,
    PTRDIFF dpStart,
    ULONG cjBuffer,
    PVOID pvBuffer);

BOOL
NTAPI
PFT_bInit(
    PFT *ppft);

PPFE
NTAPI
PFT_ppfeFindBestMatch(
    _In_ PPFT ppft,
    _In_ PWSTR pwszCapName,
    _In_ ULONG iHashValue,
    _In_ LOGFONTW *plf,
    _Inout_ PULONG pulPenalty);

VOID
NTAPI
ESTROBJ_vInit(
    IN ESTROBJ *pestro,
    IN PWSTR pwsz,
    IN ULONG cwc);

HFONT
NTAPI
GreHfontCreate(
    IN ENUMLOGFONTEXDVW *pelfw,
    IN ULONG cjElfw,
    IN LFTYPE lft,
    IN FLONG  fl,
    IN PVOID pvCliData);

PRFONT
NTAPI
DC_prfnt(PDC pdc);

PPFF
NTAPI
EngLoadFontFileFD(
    IN WCHAR *pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    DESIGNVECTOR *pdv,
    ULONG ulCheckSum);

INT
NTAPI
GreAddFontResourceW(
    IN WCHAR *pwszFiles,
    IN ULONG cwc,
    IN ULONG cFiles,
    IN FLONG fl,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv);

BOOL
NTAPI
GreExtTextOutW(
    IN HDC hDC,
    IN INT XStart,
    IN INT YStart,
    IN UINT fuOptions,
    IN OPTIONAL PRECTL lprc,
    IN LPWSTR String,
    IN INT Count,
    IN OPTIONAL LPINT Dx,
    IN DWORD dwCodePage);

VOID
NTAPI
EngAcquireSemaphoreShared(
    IN HSEMAPHORE hsem);

BOOL
NTAPI
GreGetTextExtentW(
    HDC hdc,
    LPWSTR lpwsz,
    INT cwc,
    LPSIZE psize,
    UINT flOpts);

PRFONT
NTAPI
RFONT_AllocRFONT(void);

