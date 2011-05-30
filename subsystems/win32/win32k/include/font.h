
#define LFONT_GetObject FontGetObject

#define ENGAPI

#define FD_MAX_FILES 4

extern PEPROCESS gpepCSRSS;

typedef struct _POINTEF
{
    FLOATOBJ x;
    FLOATOBJ y;
} POINTEF, *PPOINTEF;

typedef struct _RFONT *PRFONT;

typedef struct _PFE
{
    struct _PFF * pPFF;
    ULONG_PTR iFont;
    FLONG flPFE;
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

typedef struct _PFF
{
    ULONG sizeofThis;
    LIST_ENTRY leLink;
    PWSTR pwszPathname;
    ULONG cwc;
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
    PFE apfe[1];
} PFF, *PPFF;

typedef struct
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
    BOOL bVertical;
    BOOL bDeviceFont;
    BOOL bIsSystemFont;
    BOOL bNeededPaths;
    BOOL bFilledEudcArray;


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
    WCHAR awchFace[LF_FACESIZE];
    ENUMLOGFONTEXDVW elfexw;
} LFONT, *PLFONT;

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
    ULONG cFiles,
    PFONTFILEVIEW *ppffv,
    DESIGNVECTOR *pdv,
    ULONG ulCheckSum);

INT
NTAPI
GreAddFontResourceInternal(
    IN PWCHAR apwszFiles[],
    IN ULONG cFiles,
    IN FLONG fl,
    IN DWORD dwPidTid,
    IN OPTIONAL DESIGNVECTOR *pdv);

