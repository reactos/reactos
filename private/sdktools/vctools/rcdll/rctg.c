/****************************************************************************/
/*                                                                          */
/*  RCTG.C -                                                                */
/*                                                                          */
/*    Windows 3.0 Resource Compiler - Resource generation functions         */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

#include "rc.h"
#include "assert.h"


#define MAXCODE    128000 //AFX uses > 65000
#define DIBBITMAPFORMAT   0x4D42   /* 'BM' as in PM format */

#undef  min
#define min(a,b) ((a<b)?(a):(b))

PCHAR       CodeArray;      /* pointer to code buffer */
int         CodeSize;       /* size of code buffer */
int         CCount;         /* current code array address */
PFILE       fhCode;         /* file handle for remaining data */
static int  ItemCountLoc;   /* a patch location; this one for */
static int  ItemExtraLoc;   /* a patch location; this one for */

typedef struct {
    SHORT   csHotX;
    SHORT   csHotY;
    SHORT   csWidth;
    SHORT   csHeight;
    SHORT   csWidthBytes;
    SHORT   csColor;
} IconHeader;


typedef struct {
    UINT        dfVersion;              /* not in FONTINFO */
    DWORD       dfSize;                 /* not in FONTINFO */
    CHAR        dfCopyright[60];        /* not in FONTINFO */
    UINT        dfType;
    UINT        dfPoints;
    UINT        dfVertRes;
    UINT        dfHorizRes;
    UINT        dfAscent;
    UINT        dfInternalLeading;
    UINT        dfExternalLeading;
    BYTE        dfItalic;
    BYTE        dfUnderline;
    BYTE        dfStrikeOut;
    UINT        dfWeight;
    BYTE        dfCharSet;
    UINT        dfPixWidth;
    UINT        dfPixHeight;
    BYTE        dfPitchAndFamily;
    UINT        dfAvgWidth;
    UINT        dfMaxWidth;
    BYTE        dfFirstChar;
    BYTE        dfLastChar;
    BYTE        dfDefaultCHar;
    BYTE        dfBreakChar;
    UINT        dfWidthBytes;
    DWORD       dfDevice;           /* See Adaptation Guide 6.3.10 and 6.4 */
    DWORD       dfFace;             /* See Adaptation Guide 6.3.10 and 6.4 */
    DWORD       dfReserved;         /* See Adaptation Guide 6.3.10 and 6.4 */
} ffh;

#define FONT_FIXED sizeof(ffh)
#define FONT_ALL sizeof(ffh) + 64

struct MacCursor {
    char data[16];
    char mask[16];
    short hotSpotV;
    short hotSpotH;
};

typedef struct {
    unsigned short red;
    unsigned short green;
    unsigned short blue;
} RGBColor;

typedef struct {
    unsigned short value;
    RGBColor rgb;
} ColorSpec;


#define ccs2 2
ColorSpec rgcs2[ccs2] = {
    {0, {0xffff,0xffff,0xffff}},
    {1, {0x0000,0x0000,0x0000}}
};


#define ccs16 16
ColorSpec rgcs16[ccs16] = {
    {0x00, {0xffff,0xffff,0xffff}},
    {0x01, {0xfc00,0xf37d,0x052f}},
    {0x02, {0xffff,0x648a,0x028c}},
    {0x03, {0xdd6b,0x08c2,0x06a2}},
    {0x04, {0xf2d7,0x0856,0x84ec}},
    {0x05, {0x46e3,0x0000,0xa53e}},
    {0x06, {0x0000,0x0000,0xd400}},
    {0x07, {0x0241,0xab54,0xeaff}},
    {0x08, {0x1f21,0xb793,0x1431}},
    {0x09, {0x0000,0x64af,0x11b0}},
    {0x0a, {0x5600,0x2c9d,0x0524}},
    {0x0b, {0x90d7,0x7160,0x3a34}},
    {0x0c, {0xc000,0xc000,0xc000}},
    {0x0d, {0x8000,0x8000,0x8000}},
    {0x0e, {0x4000,0x4000,0x4000}},
    {0x0f, {0x0000,0x0000,0x0000}}
};


/*
 *  the 34 legal icon colors
 */
#define ccs256 34
ColorSpec rgcs256[ccs256] = {
    {0x01, {0xFFFF, 0xFFFF, 0xCCCC}},
    {0x08, {0xFFFF, 0xCCCC, 0x9999}},
    {0x33, {0xCCCC, 0x9999, 0x6666}},
    {0x16, {0xFFFF, 0x6666, 0x3333}},
    {0x92, {0x3333, 0xFFFF, 0x9999}},
    {0xE3, {0x0000, 0xBBBB, 0x0000}},
    {0x9F, {0x3333, 0x9999, 0x6666}},
    {0xA5, {0x3333, 0x6666, 0x6666}},
    {0x48, {0x9999, 0xFFFF, 0xFFFF}},
    {0xC0, {0x0000, 0x9999, 0xFFFF}},
    {0xEC, {0x0000, 0x0000, 0xDDDD}},
    {0xB0, {0x3333, 0x0000, 0x9999}},
    {0x2A, {0xCCCC, 0xCCCC, 0xFFFF}},
    {0x54, {0x9999, 0x9999, 0xFFFF}},
    {0x7F, {0x6666, 0x6666, 0xCCCC}},
    {0xAB, {0x3333, 0x3333, 0x6666}},
    {0x13, {0xFFFF, 0x6666, 0xCCCC}},
    {0x69, {0x9999, 0x0000, 0x6666}},
    {0x5C, {0x9999, 0x6666, 0x9999}},
    {0x00, {0xFFFF, 0xFFFF, 0xFFFF}},
    {0xF5, {0xEEEE, 0xEEEE, 0xEEEE}},
    {0xF6, {0xDDDD, 0xDDDD, 0xDDDD}},
    {0x2B, {0xCCCC, 0xCCCC, 0xCCCC}},
    {0xF7, {0xBBBB, 0xBBBB, 0xBBBB}},
    {0xF8, {0xAAAA, 0xAAAA, 0xAAAA}},
    {0xF9, {0x8888, 0x8888, 0x8888}},
    {0xFA, {0x7777, 0x7777, 0x7777}},
    {0xFB, {0x5555, 0x5555, 0x5555}},
    {0xFC, {0x4444, 0x4444, 0x4444}},
    {0xFD, {0x2222, 0x2222, 0x2222}},
    {0xFE, {0x1111, 0x1111, 0x1111}},
    {0xFF, {0x0000, 0x0000, 0x0000}},
    {0x05, {0xFFFF, 0xFFFF, 0x0000}},
    {0xD8, {0xDDDD, 0x0000, 0x0000}}
};

void ProcessMacIcons(RESINFO* pRes, int itBase, int ib1, int ib4, int ib8);
void ReadDIB(int ibDesc, struct tagDESCRIPTOR *pds, BITMAPINFOHEADER* pbmh, int* pcbWidth, void** ppBits, RGBQUAD** prgrgq, BOOL fIcon);
void CompactAndFlipIcon(BYTE* pBits, int cbRowCur, int cbRowMask, int cbRowNew, int cbRowMaskNew, int Height);
void WriteMacRsrc(void* pBits, int cbBits, RESINFO* pResBase, DWORD res);
void LookupIconColor(ColorSpec* rgcs, int ccs, RGBQUAD* pq);
long MungeResType(WCHAR *szType, short wOrd);
int IdUnique(TYPEINFO *ptype, RESINFO* pres);
RESINFO* LookupIconRes(TYPEINFO* ptypeIcon, RESINFO* pres);
void TranslateString(char* sz);
void TranslateBuffer(char* rgch, int cch);


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      GenWarning2() -                                                      */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID
GenWarning2(
    int iMsg,
    PCHAR arg
    )
{
    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(iMsg), curFile, token.row, arg);
    SendWarning(Msg_Text);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      GenWarning4() -                                                      */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID
GenWarning4(
    int iMsg,
    PCHAR arg1,
    PCHAR arg2,
    PCHAR arg3
    )
{
    SET_MSG(Msg_Text,
            sizeof(Msg_Text),
            GET_MSG(iMsg),
            curFile,
            token.row,
            arg1,
            arg2,
            arg3);
    SendWarning(Msg_Text);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      GenError2() -                                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/

void
GenError2(
    int iMsg,
    PCHAR arg
    )
{
    if (fhCode > 0)
        fclose(fhCode);

    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(iMsg), curFile, token.row, arg);
    SendError(Msg_Text);
    quit("\n");
}


/*---------------------------------------------------------------------------*/
/*                                                                                                                                                       */
/*      GenError1() -                                                                                                                    */
/*                                                                                                                                                       */
/*---------------------------------------------------------------------------*/

void
GenError1(
    int iMsg
    )
{
    if (fhCode > 0)
        fclose(fhCode);

    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(iMsg), curFile, token.row);
    SendError(Msg_Text);
    quit("\n");
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  CtlAlloc() -                                                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID
CtlAlloc(
    VOID
    )
{
    CodeSize = MAXCODE;
    CodeArray = (PCHAR) MyAlloc(MAXCODE);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  CtlInit() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID
CtlInit(
    VOID
    )
{
    CCount = 0;         /* absolute location in CodeArray */
    fhCode = NULL_FILE; /* don't copy file unless we need to */
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  CtlFile() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
PFILE
CtlFile(
    PFILE fh
    )
{
    if (fh != NULL_FILE)
        fhCode = fh;    /* set file handle to read remaining resource from */

    return(fhCode);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  CtlFree() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID
CtlFree(
    VOID
    )
{
    CodeSize = 0;
    MyFree(CodeArray);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetSpace() -                                                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/
PCHAR
GetSpace(
    WORD cb
    )
{
    PCHAR pch;

    if (CCount > (int) (CodeSize - cb)) {
        PVOID pv;
        if ((pv = HeapReAlloc(hHeap, HEAP_NO_SERIALIZE|HEAP_ZERO_MEMORY, ((PCHAR)CodeArray)-8, CodeSize+0x00010000+8)) == NULL) {
            GenError1(2168); //"Resource too large"
            /* GenError1 calls quit() and doesn't return! */
        }
        CodeArray = ((PCHAR)pv)+8;
        CodeSize += 0x00010000;
    }

    pch = CodeArray + CCount;
    CCount += cb;
    return(pch);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  WriteString() -                                                          */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID
WriteString(
    PWCHAR sz,
    BOOL fMacCP
    )
{
    /* add a string to the resource buffer */
    if (fMacRsrcs) {
        WriteMacString(sz, fMacCP, FALSE);
    } else {
        do {
            WriteWord(*sz);
        } while (*sz++ != 0);
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  WriteMacString() -                                                       */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID
WriteMacString(
    PWCHAR sz,
    BOOL fMacCP,
    BOOL fPascal
    )
{
    BYTE rgb[256];
    BYTE rgbExpand[256];
    BYTE* pb = rgb;
    BYTE* pbExpand = rgbExpand;
    int cch = 0;
    int cb = 0;

    if (sz != NULL)
    {
        UINT iCP;
        UINT nCP = uiCodePage;
        BOOL fAttemptTranslate = FALSE;
        static UINT rgnCP[] = {10029, 10007, 10000, 10006, 10081};    // Mac codepages
        static BOOL rgfCP[5];

        // If the codepage for the current resource text is one of the Windows
        // Latin 1, Greek, Cyrillic, Turkish, or Eastern European codepages, then
        // there exists a corresponding Macintosh codepage in Win32 and it is valid
        // to use WCToMB to map the Windows text to the Macintosh character set. If
        // the Windows text is in a different code page, we don't try to do any
        // mapping.
        iCP = uiCodePage - 1250;
        if (fMacCP && uiCodePage >= 1250 && uiCodePage <= 1254) {
            nCP = rgnCP[iCP];

            // unfortunately the Mac code pages are not supported under Windows 95.
            // To handle this, we check to see if the Mac code page we came up
            // with is actually available, and if it isn't, revert back to uiCodePage.

            if ((rgfCP[iCP] & 0x01) == 0) {  // is this fCP still uninitialized?
                rgfCP[iCP] |= 0x01;     // bit 0 set: has been initialized
                if (IsValidCodePage(nCP))
                    rgfCP[iCP] |= 0x02;     // bit 1 set: this CP is available
            }

            if ((rgfCP[iCP] & 0x02) == 0) {
                nCP = uiCodePage;
                fAttemptTranslate = TRUE;
            }
        }

        cch = wcslen(sz);

        cb = WideCharToMultiByte(nCP, 0, sz, cch, NULL, 0, NULL, NULL);
        if (cb > sizeof(rgb))
            pb = (BYTE *) MyAlloc(cb);
        WideCharToMultiByte(nCP, 0, sz, cch, (LPSTR) pb, cb, NULL, NULL);

        // if the Mac code page we wanted isn't available, try using our hard-coded tables
        if (fAttemptTranslate)
            TranslateBuffer((LPSTR) pb, cb);
    }

    if (fPascal) {
        WriteByte((char)cb);
        WriteBuffer((LPSTR) rgb, (USHORT)cb);
    } else {
        // at worst, we'll need one wide char for every single-byte char, plus a null terminator
        if (((cb + 1) * sizeof(WCHAR)) > sizeof(rgbExpand))
            pbExpand = (BYTE *) MyAlloc((cb + 1) * sizeof(WCHAR));

        cb = ExpandString(pb, cb, pbExpand);
        WriteBuffer((LPSTR) pbExpand, (USHORT)cb);

        if (pbExpand != rgbExpand)
            MyFree(pbExpand);
    }

    if (pb != rgb)
        MyFree(pb);
}


unsigned char* mpchchCodePage;

unsigned char mpchchLatin1ToMac[128] = {
    0x3f, /* 0x80 */
    0x3f, /* 0x81 */
    0xe2, /* 0x82 */
    0xc4, /* 0x83 */
    0xe3, /* 0x84 */
    0xc9, /* 0x85 */
    0xa0, /* 0x86 */
    0xe0, /* 0x87 */
    0xf6, /* 0x88 */
    0xe4, /* 0x89 */
    0x3f, /* 0x8a */
    0xdc, /* 0x8b */
    0xce, /* 0x8c */
    0x3f, /* 0x8d */
    0x3f, /* 0x8e */
    0x3f, /* 0x8f */
    0x3f, /* 0x90 */
    0xd4, /* 0x91 */
    0xd5, /* 0x92 */
    0xd2, /* 0x93 */
    0xd3, /* 0x94 */
    0xa5, /* 0x95 */
    0xd0, /* 0x96 */
    0xd1, /* 0x97 */
    0xf7, /* 0x98 */
    0x84, /* 0x99 */
    0x3f, /* 0x9a */
    0xdd, /* 0x9b */
    0xcf, /* 0x9c */
    0x3f, /* 0x9d */
    0x3f, /* 0x9e */
    0xd9, /* 0x9f */
    0xca, /* 0xa0 */
    0xc1, /* 0xa1 */
    0xa2, /* 0xa2 */
    0xa3, /* 0xa3 */
    0xdb, /* 0xa4 */
    0xb4, /* 0xa5 */
    0x3f, /* 0xa6 */
    0xa4, /* 0xa7 */
    0xac, /* 0xa8 */
    0xa9, /* 0xa9 */
    0xbb, /* 0xaa */
    0xc7, /* 0xab */
    0xc2, /* 0xac */
    0x3f, /* 0xad */
    0xa8, /* 0xae */
    0x3f, /* 0xaf */
    0xa1, /* 0xb0 */
    0xb1, /* 0xb1 */
    0x3f, /* 0xb2 */
    0x3f, /* 0xb3 */
    0xab, /* 0xb4 */
    0xb5, /* 0xb5 */
    0xa6, /* 0xb6 */
    0xe1, /* 0xb7 */
    0xfc, /* 0xb8 */
    0x3f, /* 0xb9 */
    0xbc, /* 0xba */
    0xc8, /* 0xbb */
    0x3f, /* 0xbc */
    0x3f, /* 0xbd */
    0x3f, /* 0xbe */
    0xc0, /* 0xbf */
    0xcb, /* 0xc0 */
    0xe7, /* 0xc1 */
    0xe5, /* 0xc2 */
    0xcc, /* 0xc3 */
    0x80, /* 0xc4 */
    0x81, /* 0xc5 */
    0xae, /* 0xc6 */
    0x82, /* 0xc7 */
    0xe9, /* 0xc8 */
    0x83, /* 0xc9 */
    0xe6, /* 0x3f */
    0xe8, /* 0xcb */
    0xed, /* 0xcc */
    0xea, /* 0xcd */
    0xeb, /* 0xce */
    0xec, /* 0xcf */
    0x3f, /* 0xd0 */
    0x84, /* 0xd1 */
    0xf1, /* 0xd2 */
    0xee, /* 0xd3 */
    0xef, /* 0xd4 */
    0xcd, /* 0xd5 */
    0x85, /* 0xd6 */
    0x3f, /* 0xd7 */
    0xaf, /* 0xd8 */
    0x84, /* 0xd9 */
    0xf2, /* 0xda */
    0xf3, /* 0xdb */
    0x86, /* 0xdc */
    0x3f, /* 0xdd */
    0x3f, /* 0xde */
    0xa7, /* 0xdf */
    0x88, /* 0xe0 */
    0x87, /* 0xe1 */
    0x89, /* 0xe2 */
    0x8b, /* 0xe3 */
    0x8a, /* 0xe4 */
    0x8c, /* 0xe5 */
    0xbe, /* 0xe6 */
    0x8d, /* 0xe7 */
    0x8f, /* 0xe8 */
    0x8e, /* 0xe9 */
    0x90, /* 0xea */
    0x91, /* 0xeb */
    0x93, /* 0xec */
    0x92, /* 0xed */
    0x94, /* 0xee */
    0x95, /* 0xef */
    0x3f, /* 0xf0 */
    0x96, /* 0xf1 */
    0x98, /* 0xf2 */
    0x97, /* 0xf3 */
    0x99, /* 0xf4 */
    0x9b, /* 0xf5 */
    0x9a, /* 0xf6 */
    0xd6, /* 0xf7 */
    0xbf, /* 0xf8 */
    0x9d, /* 0xf9 */
    0x9c, /* 0xfa */
    0x9e, /* 0xfb */
    0x9f, /* 0xfc */
    0x3f, /* 0xfd */
    0x3f, /* 0xfe */
    0xd8, /* 0xff */
};

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  BuildCodePage() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/
void
BuildCodePage(
    int cp
    )
{
    mpchchCodePage = cp == 1252 ? mpchchLatin1ToMac : NULL;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  TranslateString() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/
void
TranslateString(
    char* sz
    )
{
    TranslateBuffer(sz, strlen(sz)+1);
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  TranslateBuffer() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/
void
TranslateBuffer(
    char* rgch,
    int cch
    )
{
    if (mpchchCodePage == NULL)
        BuildCodePage(uiCodePage);
    if (mpchchCodePage == NULL)
        return;
    for (NULL; cch > 0; rgch++, cch--)
        if (*rgch & 0x80)
            *rgch = (char)mpchchCodePage[(unsigned char)(*rgch-0x80)];
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  ExpandString() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/
int
ExpandString(
    BYTE* pb,
    int cb,
    BYTE* pbExpand
    )
{
    int cbWide = 2; // for null terminator

    while (cb > 0) {
        if (IsDBCSLeadByteEx(uiCodePage, *pb)) {
            *pbExpand++ = *pb++;
            cb--;
        } else {
            *pbExpand++ = 0;
        }

        *pbExpand++ = *pb++;
        cbWide += 2;
        cb--;
    }

    *(WORD*) pbExpand++ = L'\0';
    return cbWide;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  AppendString() -                                                          */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID
AppendString(
    PWCHAR sz,
    BOOL fMacCP
    )
{
    PWCHAR psz;

    /* add a string to the resource buffer */
    psz = (PWCHAR) (&CodeArray[CCount]);
    if (*(psz-1) == L'\0')
        CCount -= sizeof(WCHAR);
    WriteString(sz, fMacCP);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  WriteAlign() -                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID
WriteAlign(
    VOID
    )
{
    WORD    i = CCount % 4;

    while (i--)
        WriteByte(0);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  WriteBuffer() -                                                          */
/*                                                                           */
/*---------------------------------------------------------------------------*/
VOID
WriteBuffer(
    PCHAR  pb,
    USHORT cb
    )
{
    while (cb--) {
        WriteByte(*pb++);
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/* WriteControl() -                                                         */
/*                                                                          */
/*  Parameters:                                                             */
/*      outfh  : The handle of the RES file.                                */
/*      Array  : Pointer to array from which some data is to be copied into */
/*               the .RES file.                                             */
/*               This is ignored if ArrayCount is zero.                     */
/*      ArrayCount : This is the number of bytes to be copied from "Array"  */
/*                   into the .RES file. This is zero if no copy is required*/
/*      FileCount  : This specifies the number of bytes to be copied from   */
/*                   fhCode into fhOut. If this is -1, the complete input   */
/*                   file is to be copied into fhOut.                       */
/**/
/**/

int
WriteControl(
    PFILE outfh,
    PCHAR Array,
    int ArrayCount,
    LONG FileCount
    )
{

    /* Check if the Array is to be written to .RES file */
    if (ArrayCount > 0)
        /* write the array (resource) to .RES file */
        MyWrite(outfh, Array, ArrayCount);

    /* copy the extra input file - opened by generator functions */
    if (fhCode != NULL_FILE) {
        /* Check if the complete input file is to be copied or not */
        if (FileCount == -1) {
            MyCopyAll(fhCode, outfh);
            fclose(fhCode);
        } else {
            /* Only a part of the input file is to be copied */
            MyCopy(fhCode, outfh, FileCount);

            /* Note that the fhCode is NOT closed in this case */
        }
    }

    return(ArrayCount);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  ResourceSize() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

LONG
ResourceSize (
    VOID
    )
{
    if (fhCode == NULL_FILE)
        return (LONG)CCount;            /* return size of array */
    else {
        /* note: currently all resources that use the fhCode file
         * compute their own resource sizes, and this shouldn't get
         * executed, but it is here in case of future modifications
         * which require it.
         */
        LONG lFPos = MySeek(fhCode, 0L, SEEK_CUR);
        LONG lcb = (LONG)CCount + MySeek(fhCode, 0L, SEEK_END) - lFPos;
        MySeek(fhCode, lFPos, SEEK_SET);
        return lcb;
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetIcon() -                                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

DWORD
GetIcon(
    LONG nbyFile
    )
{
    PFILE      infh = CtlFile(NULL_FILE);

    IconHeader header;
    LONG    nbyIconSize, nSeekLoc;
    LONG    nbyTransferred = 0;
    SHORT   IconID;
    int bHeaderWritten = FALSE;

    if (infh == NULL)
        return FALSE;
    /* read the header and find its size */
    MyRead( infh,  (PCHAR)&IconID, sizeof(SHORT));

    /* Check if the input file is in correct format */
    if (((CHAR)IconID != 1) && ((CHAR)IconID != 3))
        GenError2(2169, (PCHAR)tokenbuf); //"Resource file %ws is not in 2.03 format."

    MyRead( infh, (PCHAR)(IconHeader *) &header, sizeof(IconHeader));
    nbyIconSize = (header.csWidthBytes * 2) * header.csHeight;

    /* if pre-shrunk version exists at eof */
    if ((nSeekLoc = ( sizeof (SHORT) + nbyIconSize + sizeof(IconHeader))) < nbyFile) {
        /* mark as device dependant */
        *(((PCHAR)&IconID) + 1) = 0;
        MySeek(infh, (LONG)nSeekLoc, SEEK_SET);
        WriteWord(IconID);
    } else {   /* only canonical version exists */

        *(((PCHAR)&IconID) + 1) = 1;   /* mark as device independent */
        WriteWord(IconID);
        WriteBuffer((PCHAR)&header, sizeof(IconHeader));
        bHeaderWritten = TRUE;
    }

    nbyTransferred = nbyFile - MySeek(infh, 0L, SEEK_CUR);

    /* return number of bytes in the temporary file */
    return (nbyTransferred + (bHeaderWritten ? sizeof(IconHeader) : 0)
         + sizeof(SHORT));
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetNewBitmap() -                                                        */
/*                                                                          */
/*   This loads the new bitmaps in DIB format (PM format)                   */
/*--------------------------------------------------------------------------*/

DWORD
GetNewBitmap(
    VOID
    )
{
    PFILE infh = CtlFile(NULL_FILE);
    BITMAPFILEHEADER bf;
    BITMAPCOREHEADER bc;
    BITMAPINFOHEADER *pBitMapInfo;
    int cbColorTable;
    PCHAR pColorTable;
    LONG        cbImage;
    int nbits;
    DWORD BitmapSize;

    if (infh == NULL)
        return FALSE;
    MyRead(infh, (PCHAR)&bf, sizeof(bf));

    /* Check if it is in correct format */
    if (bf.bfType != DIBBITMAPFORMAT)
        GenError2(2170, (PCHAR)tokenbuf); //"Bitmap file %ws is not in 3.00 format."

    /* get the header -- assume old format */
    MyRead(infh, (PCHAR)&bc, sizeof(bc));

    BitmapSize = bc.bcSize;

    if (BitmapSize >= sizeof(BITMAPINFOHEADER)) {
        /* V3 or better format */
        pBitMapInfo = (BITMAPINFOHEADER *) MyAlloc(BitmapSize);

        memcpy(pBitMapInfo, &bc, sizeof(bc));

        MyRead(infh, ((PCHAR)pBitMapInfo) + sizeof(bc), BitmapSize - sizeof(bc));

        nbits = pBitMapInfo->biPlanes * pBitMapInfo->biBitCount;

        if ( pBitMapInfo->biCompression == BI_BITFIELDS ) {
            if( (pBitMapInfo->biBitCount <= 8) ||
                (pBitMapInfo->biBitCount == 24) )
            {
                GenError2(2170, (PCHAR)tokenbuf); //"Bitmap file %ws is not in 3.00 format."
            }

            cbColorTable = 3 * sizeof(DWORD);
        } else {
            // Only pBitMapInfo->biBitCount 1,4,8,24. biBitCount 16 and 32 MUST have BI_BITFIELD specified
            cbColorTable = (int)pBitMapInfo->biClrUsed * sizeof(RGBQUAD);
            if ((cbColorTable  == 0) && (pBitMapInfo->biBitCount<=8))
                cbColorTable = (1 << nbits) * sizeof(RGBQUAD);
        }

        if (fMacRsrcs) {
            pBitMapInfo->biSize = SwapLong(pBitMapInfo->biSize);
            pBitMapInfo->biWidth = SwapLong(pBitMapInfo->biWidth);
            pBitMapInfo->biHeight = SwapLong(pBitMapInfo->biHeight);
            pBitMapInfo->biPlanes = SwapWord(pBitMapInfo->biPlanes);
            pBitMapInfo->biBitCount = SwapWord(pBitMapInfo->biBitCount);
            pBitMapInfo->biCompression = SwapLong(pBitMapInfo->biCompression);
            pBitMapInfo->biSizeImage = SwapLong(pBitMapInfo->biSizeImage);
            pBitMapInfo->biXPelsPerMeter = SwapLong(pBitMapInfo->biXPelsPerMeter);
            pBitMapInfo->biYPelsPerMeter = SwapLong(pBitMapInfo->biYPelsPerMeter);
            pBitMapInfo->biClrUsed = SwapLong(pBitMapInfo->biClrUsed);
            pBitMapInfo->biClrImportant = SwapLong(pBitMapInfo->biClrImportant);
        }
        WriteBuffer((PCHAR)pBitMapInfo, (USHORT)BitmapSize);
        MyFree(pBitMapInfo);
    } else if (BitmapSize == sizeof(BITMAPCOREHEADER)) {
        nbits = bc.bcPlanes * bc.bcBitCount;

        /* old format */
        if (nbits == 24)
            cbColorTable = 0;
        else
            cbColorTable = (1 << nbits) * sizeof(RGBTRIPLE);

        if (fMacRsrcs) {
            bc.bcSize = SwapLong(bc.bcSize);
            bc.bcWidth = SwapWord(bc.bcWidth);
            bc.bcHeight = SwapWord(bc.bcHeight);
            bc.bcPlanes = SwapWord(bc.bcPlanes);
            bc.bcBitCount = SwapWord(bc.bcBitCount);
        }
        WriteBuffer((PCHAR)&bc, (USHORT)BitmapSize);
    } else {
        GenError1(2171); //"Unknown DIB header format"
    }

    if (cbColorTable) {
        pColorTable = (PCHAR) MyAlloc(cbColorTable);
        MyRead(infh, pColorTable, cbColorTable);
        WriteBuffer(pColorTable, (USHORT)cbColorTable);
        MyFree(pColorTable);
    }

    /* get the length of the bits */
    cbImage = MySeek(infh, 0L, SEEK_END) - BFOFFBITS(&bf) + BitmapSize + cbColorTable;

    /* seek to the beginning of the bits... */
    MySeek(infh, BFOFFBITS(&bf), SEEK_SET);

    return cbImage;
}

VOID
WriteOrdCode(
    void
    )
{
    WriteWord(0xFFFF);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  SetUpDlg() -                                                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID
SetUpDlg(
    PDLGHDR pDlg,
    BOOL fDlgEx
    )
{
    if (fDlgEx) {
        // Hack -- this is how we version switch the dialog
        WriteWord(0x0001);          // store wDlgVer
        WriteWord(0xFFFF);          // store wSignature
        WriteLong(pDlg->dwHelpID);
        WriteLong(pDlg->dwExStyle); // store exstyle
    }

    /* write the style bits to the resource buffer */
    WriteLong(pDlg->dwStyle);   /* store style */

    if (!fDlgEx)
        WriteLong(pDlg->dwExStyle);   /* store exstyle */

    ItemCountLoc = CCount;        /* global marker for location of item cnt. */

    /* skip place for num of items */
    WriteWord(0);

    /* output the dialog position and size */
    WriteWord(pDlg->x);
    WriteWord(pDlg->y);
    WriteWord(pDlg->cx);
    WriteWord(pDlg->cy);

    /* output the menu identifier */
    if (pDlg->fOrdinalMenu) {
        WriteOrdCode();
        WriteWord((USHORT)wcsatoi(pDlg->MenuName));
    } else {
        WriteString(pDlg->MenuName, FALSE);
    }

    /* output the class identifier */
    if (pDlg->fClassOrdinal) {
        WriteOrdCode();
        WriteWord((USHORT)wcsatoi(pDlg->Class));
    } else {
        WriteString(pDlg->Class, FALSE);
    }

    /* output the title */
    WriteString(pDlg->Title, TRUE);

    /* add the font information */
    if (pDlg->pointsize) {
        WriteWord(pDlg->pointsize);
        if (fDlgEx) {
            WriteWord(pDlg->wWeight);
            WriteByte(pDlg->bItalic);
            WriteByte(pDlg->bCharSet);
        }
        WriteString(pDlg->Font, FALSE);
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  SetUpItem() -                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID
SetUpItem(
    PCTRL LocCtl,
    BOOL fDlgEx
    )
{
    PWCHAR  tempptr;

    /* control dimensions, id, and style bits */
    WriteAlign();

    // control dimensions, id, and style bits
    if (fDlgEx) {
        WriteLong(LocCtl->dwHelpID);
        WriteLong(LocCtl->dwExStyle);
        WriteLong(LocCtl->dwStyle);
    } else {
        WriteLong(LocCtl->dwStyle);
        WriteLong(LocCtl->dwExStyle);
    }

    WriteWord(LocCtl->x);
    WriteWord(LocCtl->y);
    WriteWord(LocCtl->cx);
    WriteWord(LocCtl->cy);

    if (fDlgEx)
        WriteLong(LocCtl->id);
    else
        WriteWord(LOWORD(LocCtl->id));

    /* control class */
    tempptr = LocCtl->Class;
    if (*tempptr == 0xFFFF) {
        /* special class code follows */
        WriteWord(*tempptr++);
        WriteWord(*tempptr++);
    } else {
        WriteString(tempptr, FALSE);
    }

    /* text */
    if (LocCtl->fOrdinalText) {
        WriteOrdCode();
        WriteWord((USHORT)wcsatoi(LocCtl->text));
    } else {
        WriteString(LocCtl->text, TRUE);
    }

    if (fDlgEx)
        ItemExtraLoc = CCount;

    WriteWord(0);   /* zero CreateParams count */

    IncItemCount();

}


void
SetItemExtraCount(
    WORD wCount,
    BOOL fDlgEx
    )
{
    if (fDlgEx)
        *((WORD *) (CodeArray + ItemExtraLoc)) = wCount;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  IncItemCount() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* seemingly obscure way to increment # of items in a dialog */
/* ItemCountLoc indexes where we put the item count in the resource buffer, */
/* so we increment that counter when we add a control */

VOID
IncItemCount(
    VOID
    )
{
    PUSHORT     pus;

    pus = (PUSHORT)&CodeArray[ItemCountLoc];
    (*pus)++;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  SwapItemCount() -                                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/* when writing a Mac resource fork, we need to swap this count before writing */
VOID
SwapItemCount(
    VOID
    )
{
    PUSHORT     pus;

    pus = (PUSHORT)&CodeArray[ItemCountLoc];
    *pus = SwapWord(*pus);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  FixMenuPatch() -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID
FixMenuPatch(
    WORD wEndFlagLoc
    )
{
    if (fMacRsrcs)
        CodeArray[wEndFlagLoc + 1] |= MFR_END;
    else
        *((PWORD) (CodeArray + wEndFlagLoc)) |= MFR_END;
    // mark last menu item
//    CodeArray[wEndFlagLoc] |= MFR_END;
}


VOID
FixOldMenuPatch(
    WORD wEndFlagLoc
    )
{
    // mark last menu item
    if (fMacRsrcs)
        CodeArray[wEndFlagLoc + 1] |= OPENDMENU;
    else
        CodeArray[wEndFlagLoc] |= OPENDMENU;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  MarkAccelFlagsByte() -                                                   */
/*                                                                           */
/*---------------------------------------------------------------------------*/

/* set the place where the accel end bit is going to be set */

VOID
MarkAccelFlagsByte (
    VOID
    )
{
    /* set the location to the current position in the resource buffer */
    mnEndFlagLoc = CCount;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  PatchAccelEnd() -                                                        */
/*                                                                           */
/*---------------------------------------------------------------------------*/

VOID
PatchAccelEnd (
    VOID
    )
{
    if (fMacRsrcs)
        CodeArray[mnEndFlagLoc + 1] |= 0x80;
    else
        CodeArray[mnEndFlagLoc] |= 0x80;
}


// ----------------------------------------------------------------------------
//
//  SetUpMenu() -
//
// ----------------------------------------------------------------------------

WORD
SetUpMenu(
    PMENU pmn
    )
{
    WORD    wRes;

    WriteLong(pmn->dwType);
    WriteLong(pmn->dwState);
    WriteLong(pmn->dwID);

    // mark the last item added to the menu
    wRes = (WORD)CCount;

    WriteWord(pmn->wResInfo);
    WriteString(pmn->szText, TRUE);
    if (32)
        WriteAlign();
    if (pmn->wResInfo & MFR_POPUP)
        WriteLong(pmn->dwHelpID);

    return(wRes);
}


// ----------------------------------------------------------------------------
//
//  SetUpOldMenu() -
//
// ----------------------------------------------------------------------------

WORD
SetUpOldMenu(
    PMENUITEM mnTemp
    )
{
    WORD    wRes;

    /* mark the last item added to the menu */
    wRes = (WORD)CCount;

    /* write the menu flags */
    WriteWord(mnTemp->OptFlags);

    /* popup menus don't have id values */
    /* write ids of menuitems */
    if (!((mnTemp->OptFlags) & OPPOPUP))
        WriteWord(mnTemp->id);

    /* write text of selection */
    WriteString(mnTemp->szText, TRUE);

    return(wRes);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  GetRCData() -                                                          */
/*                                                                           */
/*---------------------------------------------------------------------------*/

WORD
GetRCData (
    PRESINFO pRes
    )
{
    PCHAR       pch, pchT;
    PWCHAR      pwch;
    WORD        nBytes = 0;
    ULONG       cb = 0;

    /* look for BEGIN (after id RCDATA memflags) */
    // 2134 -- "BEGIN expected in RCData"
    PreBeginParse(pRes, 2134);

    /* add the users data to the resource buffer until we see an END */
    while (token.type != END) {
    /* see explanation in rcl.c in GetStr() */
        if (token.type == LSTRLIT)
            token.type = token.realtype;

        switch (token.type) {
            case LSTRLIT:
                pwch = tokenbuf;
                while (token.val--) {
                    WriteWord(*pwch++);
                    nBytes += sizeof(WCHAR);
                }
                break;

            case STRLIT:
                cb = WideCharToMultiByte(uiCodePage, 0, tokenbuf,
                                            token.val, NULL, 0, NULL, NULL);
                pchT = pch = (PCHAR) MyAlloc(cb);
                WideCharToMultiByte(uiCodePage, 0, tokenbuf,
                                            token.val, pch, cb, NULL, NULL);
                while (cb--) {
                    WriteByte(*pch++);
                    nBytes += sizeof(CHAR);
                }
                MyFree(pchT);
                break;

            case NUMLIT:
                if (token.flongval) {
                    WriteLong(token.longval);
                    nBytes += sizeof(LONG);
                } else {
                    WriteWord(token.val);
                    nBytes += sizeof(WORD);
                }
                break;

            default:
                ParseError1(2164);
                return 0;
        }
        ICGetTok();
    }

    return(nBytes);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*  AddFontRes() -                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

BOOL
AddFontRes(
    PRESINFO pRes
    )
{
    PFILE    fpFont;
    BYTE     font[FONT_ALL];
    PCHAR    pEnd, pDev, pFace;
    DWORD    offset;
    SHORT    nbyFont;
    PFONTDIR pFont;
    PFONTDIR pFontSearch;

    /* get handle to font file */
    fpFont = CtlFile(NULL_FILE);
    if (fpFont == NULL)
        return FALSE;
    MySeek(fpFont, 0L, SEEK_SET);

    /* copy font information to the font directory */
    /*    name strings are ANSI (8-bit) */
    MyRead(fpFont, (PCHAR)&font[0], sizeof(ffh));
    pEnd = (PCHAR) (&font[0] + sizeof(ffh));    /* pointer to end of font buffer */
    offset = ((ffh * )(&font[0]))->dfDevice;
    if (offset != (LONG)0)  {
        MySeek(fpFont, (LONG)offset, SEEK_SET);        /* seek to device name */
        pDev = pEnd;
        do {
            MyRead( fpFont, pEnd, 1);              /* copy device name */
        } while (*pEnd++);
    } else {
        (*pEnd++ = '\0');
    }
    offset = ((ffh * )(&font[0]))->dfFace;
    MySeek(fpFont, (LONG)offset, SEEK_SET);         /* seek to face name */
    pFace = pEnd;
    do {                                /* copy face name */
        MyRead( fpFont, pEnd, 1);
    } while (*pEnd++);

    nbyFont = (SHORT)(pEnd - (PCHAR) &font[0]);

    pFont = (FONTDIR * )MyAlloc(sizeof(FONTDIR) + nbyFont);
    pFont->nbyFont = nbyFont;
    pFont->ordinal = pRes->nameord;
    pFont->next = NULL;
    memcpy((PCHAR)(pFont + 1), (PCHAR)font, nbyFont);

    if (!nFontsRead) {
        pFontList = pFontLast = pFont;
    } else {
        for (pFontSearch=pFontList ; pFontSearch!=NULL ; pFontSearch=pFontSearch->next) {
            if (pFont->ordinal == pFontSearch->ordinal) {
                SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(2181), curFile, token.row, pFont->ordinal);
                SendError(Msg_Text);
                MyFree(pFont);
                return FALSE;
            }
        }
        pFontLast = pFontLast->next = pFont;
    }

    /* rewind font file for SaveResFile() */
    MySeek(fpFont, 0L, SEEK_SET);
    return TRUE;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SaveResFile() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/


VOID
SaveResFile(
    PTYPEINFO pType,
    PRESINFO pRes
    )
{
    if (!fMacRsrcs)
        MyAlign(fhBin);

    AddResToResFile(pType, pRes, CodeArray, CCount, -1L);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetNewIconsCursors(ResType)                                     */
/*                                                                          */
/*      This reads all the different forms of icons/cursors in 3.00 format  */
/*      in the input file                                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
GetNewIconsCursors(
    PTYPEINFO pGroupType,
    PRESINFO pGroupRes,
    LPWSTR ResType
    )
{
    static SHORT  idIconUnique = 1;
    UINT          i;
    LONG          DescOffset;
    PTYPEINFO     pType;
    PRESINFO      pRes;
    NEWHEADER     NewHeader;
    DESCRIPTOR    Descriptor;
    BITMAPHEADER  BitMapHeader;
    RESDIR        ResDir;
    int           ArrayCount = 0;
    LOCALHEADER   LocHeader;

    /* Read the header of the bitmap file */
    MyRead(fhCode, (PCHAR)&NewHeader, sizeof(NEWHEADER));

    /* Check if the file is in correct format */
    if ((NewHeader.Reserved != 0) || ((NewHeader.ResType != 1) && (NewHeader.ResType != 2)))
        GenError2(2175, (PCHAR)tokenbuf); //"Resource file %ws is not in 3.00 format."
    /* Write the header into the Code array */
    WriteBuffer((PCHAR)&NewHeader, sizeof(NEWHEADER));

    /* Process all the forms one by one */
    for (i = 0; i < NewHeader.ResCount; i++) {
        /* Readin the Descriptor */
        MyRead(fhCode, (PCHAR)&Descriptor, sizeof(DESCRIPTOR));

        /* Save the current offset */
        DescOffset = MySeek(fhCode, 0L, SEEK_CUR);

        /* Seek to the Data */
        MySeek(fhCode, Descriptor.OffsetToBits, SEEK_SET);

        /* Get the bitcount and Planes data */
        MyRead(fhCode, (PCHAR)&BitMapHeader, sizeof(BITMAPHEADER));
        if (BitMapHeader.biSize != sizeof(BITMAPHEADER))
            GenError2(2176, (PCHAR)tokenbuf); //"Old DIB in %ws.  Pass it through SDKPAINT."

        ResDir.BitCount = BitMapHeader.biBitCount;
        ResDir.Planes = BitMapHeader.biPlanes;

        /* Seek to the Data */
        MySeek(fhCode, Descriptor.OffsetToBits, SEEK_SET);

        ArrayCount = 0;

        /* fill the fields of ResDir and LocHeader */
        switch (NewHeader.ResType) {
            case CURSORTYPE:

                LocHeader.xHotSpot = Descriptor.xHotSpot;
                LocHeader.yHotSpot = Descriptor.yHotSpot;
                ArrayCount = sizeof(LOCALHEADER);

                ResDir.ResInfo.Cursor.Width = (USHORT)BitMapHeader.biWidth;
                ResDir.ResInfo.Cursor.Height = (USHORT)BitMapHeader.biHeight;

                break;

            case ICONTYPE:

                ResDir.ResInfo.Icon.Width = Descriptor.Width;
                ResDir.ResInfo.Icon.Height = Descriptor.Height;
                ResDir.ResInfo.Icon.ColorCount = Descriptor.ColorCount;
                /* The following line is added to initialise the unused
                 * field "reserved".
                 * Fix for Bug #10382 --SANKAR-- 03-14-90
                 */
                ResDir.ResInfo.Icon.reserved = Descriptor.reserved;
                break;

        }

        ResDir.BytesInRes = Descriptor.BytesInRes + ArrayCount;


        /* Create a pRes with New name */
        pRes = (PRESINFO) MyAlloc(sizeof(RESINFO));
        pRes->language = language;
        pRes->version = version;
        pRes->characteristics = characteristics;
        pRes ->name = NULL;
        pRes ->nameord = idIconUnique++;

        /* The individual resources must have the same memory flags as the
            ** group.
            */
        pRes ->flags = pGroupRes ->flags;
        pRes ->size = Descriptor.BytesInRes + ArrayCount;

        /* Create a new pType, or find existing one */
        pType = AddResType(NULL, ResType);


        /* Put Resource Directory entry in CodeArray */
        WriteBuffer((PCHAR)&ResDir, sizeof(RESDIR));

        /*
         * Write the resource name ordinal.
         */
        WriteWord(pRes->nameord);

        MyAlign(fhBin);

        AddResToResFile(pType, pRes, (PCHAR)&LocHeader, ArrayCount,
            Descriptor.BytesInRes);

        /* Seek to the Next Descriptor */
        MySeek(fhCode, DescOffset, SEEK_SET);
    }

    pGroupRes ->size = sizeof(NEWHEADER) + NewHeader.ResCount * (sizeof(RESDIR) + sizeof(SHORT));

    /* If the group resource is marked as PRELOAD, then we should use
        ** the same flags. Otherwise, mark it as DISCARDABLE
        */
    if (!(pGroupRes ->flags & NSPRELOAD))
        pGroupRes ->flags = NSMOVE | NSPURE | NSDISCARD;

    /* Close the input file, nothing more to read */
    fclose(fhCode);
    fhCode = NULL_FILE;

    /* Copy the code array into RES file for Group items */
    SaveResFile(pGroupType, pGroupRes);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FileIsAnimated(LONG nbyFile)                                            */
/*                                                                          */
/*  This function checks to see if the file we have is 3.0 icon/cursor file */
/*  or an animated icon/cursor.                                             */
/*                                                                          */
/*  Returns RT_* of filetype.                                               */
/*--------------------------------------------------------------------------*/

DWORD
FileIsAnimated(
    LONG nbyFile
    )
{
    RTAG tag;
    LONG lRead;

    lRead = MyRead(fhCode, (PRTAG)&tag, sizeof(RTAG));
    MySeek(fhCode, 0L, SEEK_SET);             /* return to start of file */
    if (lRead != sizeof(RTAG))
        return FALSE;

    return tag.ckID == FOURCC_RIFF;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetAniIconsAniCursors(ResType)                                          */
/*                                                                          */
/*  This function check if the file we have is a valid animated icon.       */
/*  All the work performed here is purelly optional and is done to make     */
/*  sure that the image we write in the res file is in the proper format.   */
/*  Just returning the nbyFile would be enough to copy the file in the res  */
/*  file.                                                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

DWORD
GetAniIconsAniCursors(
    LONG nbyFile
    )
{
    RTAG tag;
    LONG lRead = nbyFile;

    /* Check if we have a RIFF file */
    lRead -= MyRead(fhCode, (PRTAG)&tag, sizeof(RTAG));

    if( tag.ckID!=FOURCC_RIFF )
        GenError2(2173, (PCHAR)tokenbuf);

    /* Read the next chunk */
    lRead -= MyRead(fhCode, (LPDWORD)&tag.ckID, sizeof(tag.ckID));
    if( tag.ckID!=FOURCC_ACON )
        GenError2(2173, (PCHAR)tokenbuf);

    /* so we have an animated icon file, make sure all the blocks are there */
    while( MyRead(fhCode, (PRTAG)&tag, sizeof(RTAG)) ) {
        lRead -= sizeof(RTAG)+tag.ckSize;
        MySeek(fhCode, tag.ckSize, SEEK_CUR);
    }

    if( lRead!=0 )
        GenError2(2173, (PCHAR)tokenbuf);

    /*
     * Now that we are sure this is a valid file, move the
     * file pointer back at the begining of the file.
     */
    MySeek(fhCode, 0L, SEEK_SET);

    return nbyFile;
}

/*  GetBufferLen
 *      Returns the current length of the buffer
 */

WORD
GetBufferLen(
    VOID
    )
{
    return (WORD)CCount;
}


USHORT
GetItemCount(
    int Index
    )
{
    return *((USHORT UNALIGNED*)(CodeArray + Index));
}

void
SetItemCount(
    int Index,
    USHORT wCount
    )
{
    *((USHORT UNALIGNED*)(CodeArray + Index)) = wCount;
}

DWORD
SwapLong(
    DWORD dw
    )
{
    return ((dw << 24) & 0xff000000L) |
    ((dw << 8) & 0x00ff0000L) |
    ((dw >> 8) & 0x0000ff00L) |
    ((dw >> 24) & 0x000000ffL);
}

WORD
SwapWord(
    WORD w
    )
{
    return ((w << 8) & 0xff00) |
        ((w >> 8) & 0x00ff);
}

enum {
    itNone = -1,

    itIcn_ = 0,
    itIcl4 = 1,
    itIcl8 = 2,

    itIcs_ = 3,
    itIcs4 = 4,
    itIcs8 = 5,

    itIcm_ = 6,
    itIcm4 = 7,
    itIcm8 = 8,

    itMax = 9
};

static DWORD mpitres[itMax] = {
    'ICN#',
    'icl4',
    'icl8',
    'ics#',
    'ics4',
    'ics8',
    'icm#',
    'icm4',
    'icm8'
};

enum {
    fitIcn_ = 1 << itIcn_,
    fitIcl4 = 1 << itIcl4,
    fitIcl8 = 1 << itIcl8,

    fitIcs_ = 1 << itIcs_,
    fitIcs4 = 1 << itIcs4,
    fitIcs8 = 1 << itIcs8,

    fitIcm_ = 1 << itIcm_,
    fitIcm4 = 1 << itIcm4,
    fitIcm8 = 1 << itIcm8
};

void
GetMacIcon(
    TYPEINFO *pType,
    RESINFO *pRes
    )
{
    struct tagNEWHEADER gh;
    struct tagDESCRIPTOR ds;
    BITMAPINFOHEADER bmh;
    int ibDescNext;
    int mpitib[itMax];
    int it;
    int ires;
    int fitFound;

    /* The input file has already been opened; read in the bitmap file header */

    MyRead(fhCode, (char*)&gh, sizeof(struct tagNEWHEADER));
    if (gh.Reserved != 0 || gh.ResType != 1)
        GenError2(2175, (PCHAR)tokenbuf);

    /* run through all the icons, keeping track of the useful ones */

    memset(mpitib, 0, sizeof(mpitib));
    ibDescNext = MySeek(fhCode, 0L, SEEK_CUR);
    fitFound = 0;
    for (ires = 0; ires < gh.ResCount; ires++) {
        /* Read in the descriptor */

        MySeek(fhCode, ibDescNext, SEEK_SET);
        MyRead(fhCode, (char*)&ds, sizeof(struct tagDESCRIPTOR));
        ibDescNext = MySeek(fhCode, 0L, SEEK_CUR);

        /* get bitmap header */

        MySeek(fhCode, ds.OffsetToBits, SEEK_SET);
        MyRead(fhCode, (char*)&bmh, sizeof(BITMAPINFOHEADER));
        if (bmh.biSize != sizeof(BITMAPINFOHEADER))
            GenError2(2176, (PCHAR)tokenbuf);

        /* find valid color cases */

        if (bmh.biPlanes != 1)
            continue;
        if (bmh.biBitCount == 1)
            it = itIcn_;
        else if (bmh.biBitCount == 4)
            it = itIcl4;
        else if (bmh.biBitCount == 8)
            it = itIcl8;
        else
            continue;

        /* find valid sizes */

        if (bmh.biWidth == 16 && bmh.biHeight == 24)
            it += itIcm_ - itIcn_;
        else if (bmh.biWidth == 16 && bmh.biHeight == 32)
            it += itIcs_ - itIcn_;
        else if (bmh.biWidth == 32 && bmh.biHeight == 64)
            it += itIcn_ - itIcn_;
        else
            continue;

        /* mark sizes we found */

        fitFound |= 1 << it;
        mpitib[it] = ibDescNext - sizeof(struct tagDESCRIPTOR);
    }

    /* if no usable icon found, bail out */

    if (fitFound == 0) {
        GenWarning2(4508, (PCHAR)tokenbuf);
    } else {
        if (fitFound & (fitIcn_|fitIcl4|fitIcl8))
            ProcessMacIcons(pRes, itIcn_, mpitib[itIcn_], mpitib[itIcl4], mpitib[itIcl8]);
        if (fitFound & (fitIcs_|fitIcs4|fitIcs8))
            ProcessMacIcons(pRes, itIcs_, mpitib[itIcs_], mpitib[itIcs4], mpitib[itIcs8]);
        if (fitFound & (fitIcm_|fitIcm4|fitIcm8))
            ProcessMacIcons(pRes, itIcm_, mpitib[itIcs_], mpitib[itIcs4], mpitib[itIcs8]);
    }

    fclose(fhCode);
    fhCode = NULL_FILE;
}


int
Luminance(
    RGBColor* prgb
    )
{
    return prgb->red/256*30 + prgb->green/256*59 + prgb->blue/256*11;
}

/* threshold = middle gray */

#define rThreshold 128
#define gThreshold 128
#define bThreshold 128
int lumThreshold = rThreshold*30 + gThreshold*59 + bThreshold*11;


void
ProcessMacIcons(
    RESINFO* pResBase,
    int itBase,
    int ib1,
    int ib4,
    int ib8
    )
{
    BITMAPINFOHEADER bmh;
    RGBQUAD* rgq;
    struct tagDESCRIPTOR ds;
    BYTE* pBits;
    int cbWidth, cbMask;
    int cbBits;
    int ib, iq;
    int y;
    BYTE *pbIn;
    BYTE* pbOut;
    BYTE bIn, bOut;

    /* create monochrome icon out of the best-looking icon */

    if (ib1 != 0) {
        /* read the DIB */

        ReadDIB(ib1, &ds, &bmh, &cbWidth, (void **) &pBits, &rgq, TRUE);

        /* invert bits, if color table is backwards */

        if (rgq[0].rgbReserved != 0)
            for (ib = cbWidth*bmh.biHeight/2; --ib >= 0; )
                pBits[ib] ^= 0xff;
    } else if (ib4 != 0) {
        /* read the DIB and create color-to-mono color mapping */

        ReadDIB(ib4, &ds, &bmh, &cbWidth, (void **) &pBits, &rgq, TRUE);
        for (iq = 0; iq < (int)bmh.biClrUsed; iq++)
            rgq[iq].rgbReserved =
                    Luminance(&rgcs16[rgq[iq].rgbReserved].rgb) < lumThreshold;

        /* map colors to black and white and convert to 1-bit/pixel */

        for (y = 0; y < (int)(bmh.biHeight/2); y++) {
            pbIn = pBits + y*cbWidth;
            pbOut = pbIn;
            assert(cbWidth % 4 == 0);   // we know it's 8 or 16 bytes wide
            for (ib = 0; ib < cbWidth; ) {
                bIn = *pbIn++;
                bOut = (bOut<<1) | rgq[bIn>>4].rgbReserved;
                bOut = (bOut<<1) | rgq[bIn&0xf].rgbReserved;
                ib++;
                if (ib % 4 == 0)
                    *pbOut++ = bOut;
            }
        }
    } else {
        /* read the DIB and create color-to-mono color mapping */

        ReadDIB(ib8, &ds, &bmh, &cbWidth, (void **) &pBits, &rgq, TRUE);
        for (iq = 0; iq < (int)bmh.biClrUsed; iq++)
            rgq[iq].rgbReserved =
                    Luminance(&rgcs256[rgq[iq].rgbReserved].rgb) < lumThreshold;

        /* map colors to black and white and convert to 1-bit/pixel */

        for (y = 0; y < (int)(bmh.biHeight/2); y++) {
            pbIn = pBits + y*cbWidth;
            pbOut = pbIn;
            assert(cbWidth % 8 == 0);   // we know it's 16 or 32 bytes wide
            for (ib = 0; ib < cbWidth; ) {
                bIn = *pbIn++;
                bOut = (bOut<<1) | rgq[bIn].rgbReserved;
                ib++;
                if (ib % 8 == 0)
                    *pbOut++ = bOut;
            }
        }
    }

    cbMask = (bmh.biWidth+31)/32*4;
    CompactAndFlipIcon(pBits, cbWidth, cbMask, bmh.biWidth/8, bmh.biWidth/8, bmh.biHeight/2);
    cbBits = bmh.biHeight * (bmh.biWidth/8);

    /* "xor" the mask back into image */
    pbOut = pBits; pbIn = pBits + cbBits/2;
    for (ib = cbBits/2; ib > 0; ib--)
        *pbOut++ ^= ~*pbIn++;

    /* and write out base icon */

    WriteMacRsrc(pBits, cbBits, pResBase, mpitres[itBase]);
    MyFree(pBits);
    MyFree(rgq);

    /* move over 16-color icon */

    if (ib4 != 0) {
        ReadDIB(ib4, &ds, &bmh, &cbWidth, (void **) &pBits, &rgq, TRUE);

        /* convert color table to mac standard palette */

        for (pbIn = pBits, ib = cbWidth*bmh.biHeight/2; ib > 0; pbIn++, ib--) {
            bIn = *pbIn;
            *pbIn = (rgq[bIn>>4].rgbReserved << 4) |
                    rgq[bIn&0x0f].rgbReserved;
        }

        /* compact and flip the image */

        cbMask = (bmh.biWidth+31)/32*4;
        CompactAndFlipIcon(pBits, cbWidth, cbMask, bmh.biWidth/2, bmh.biWidth/8, bmh.biHeight/2);
        cbBits = (bmh.biHeight/2) * (bmh.biWidth/2);

        /* "xor" the mask back into the image */

        pbOut = pBits; pbIn = pBits + cbBits;
        for (ib = 0; ib < cbBits; ib++, pbOut++) {
            if (ib % 4 == 0)
                bIn = *pbIn++;
            if ((bIn & 0x80) == 0)
                *pbOut ^= 0xf0;
            if ((bIn & 0x40) == 0)
                *pbOut ^= 0x0f;
            bIn <<= 2;
        }

        /* and write out the resource */

        WriteMacRsrc(pBits, cbBits, pResBase, mpitres[itBase+itIcs4-itIcs_]);
        MyFree(pBits);
        MyFree(rgq);
    }

    /* move over 256-color icon */

    if (ib8 != 0) {
        ReadDIB(ib8, &ds, &bmh, &cbWidth, (void **) &pBits, &rgq, TRUE);

        /* convert color table to mac standard palette */

        for (pbIn = pBits, ib = cbWidth*bmh.biHeight/2; ib > 0; pbIn++, ib--)
            *pbIn = rgq[*pbIn].rgbReserved;

        /* compact and flip the image */

        cbMask = (bmh.biWidth+31)/32*4;
        CompactAndFlipIcon(pBits, cbWidth, cbMask, bmh.biWidth, bmh.biWidth/8, bmh.biHeight/2);
        cbBits = (bmh.biHeight/2) * (bmh.biWidth);

        /* "xor" the mask back into the image */

        pbOut = pBits; pbIn = pBits + cbBits;
        for (ib = 0; ib < cbBits; ib++, pbOut++) {
            if (ib % 8 == 0)
                bIn = *pbIn++;
            if ((bIn & 0x80) == 0)
                *pbOut ^= 0xff;
            bIn <<= 1;
        }

        /* and write out the resource */

        WriteMacRsrc(pBits, cbBits, pResBase, mpitres[itBase+itIcs8-itIcs_]);

        MyFree(pBits);
        MyFree(rgq);
    }
}

void
WriteMacRsrc(
    void* pBits,
    int cbBits,
    RESINFO* pResBase,
    DWORD res
    )
{
    WCHAR sz[8];
    TYPEINFO* pType;
    RESINFO* pRes;

    sz[0] = (char)(res >> 24);
    sz[1] = (char)(res >> 16);
    sz[2] = (char)(res >> 8);
    sz[3] = (char)res;
    sz[4] = 0;
    pType = AddResType(sz, 0);

    pRes = (RESINFO *)MyAlloc(sizeof(RESINFO));
    *pRes = *pResBase;
    pRes->size = cbBits;

    AddResToResFile(pType, pRes, (PCHAR) pBits, (WORD)cbBits, 0);
}


void
CompactAndFlipIcon(
    BYTE* pBits,
    int cbRowCur,
    int cbRowMaskCur,
    int cbRowNew,
    int cbRowMaskNew,
    int Height
    )
{
    BYTE* pBitsNew;
    int y;
    BYTE* pbFrom, *pbTo;
    int cb;

    assert(cbRowCur >= cbRowNew);
    pBitsNew = (BYTE *) MyAlloc((WORD)(Height*(cbRowNew+cbRowMaskCur)));

    /* copy the bits over into the scratch space, compacting and
       flipping as we go */

    for (y = 0; y < Height; y++)
        memcpy(pBitsNew+y*cbRowNew, pBits+(Height-y-1)*cbRowCur, cbRowNew);

    /* copy over the mask, flipping and inverting as we go */

    for (y = 0; y < Height; y++) {
        pbTo = pBitsNew + cbRowNew*Height + y*cbRowMaskNew;
        pbFrom = pBits + cbRowCur*Height + (Height-y-1)*cbRowMaskCur;
        for (cb = cbRowMaskNew; cb > 0; cb--)
            *pbTo++ = ~*pbFrom++;
    }

    /* and move the result back to pBits */

    memcpy(pBits, pBitsNew, (cbRowNew+cbRowMaskCur)*Height);
    MyFree(pBitsNew);
}

void CrunchY(unsigned char* pbSrc, unsigned char* pbDst, int WidthBytes, int dySrc, int scale);
void CrunchX2(unsigned char* pbSrc, unsigned char* pbDst, int cbWidth, int dy);

void
GetMacCursor(
    TYPEINFO *pType,
    RESINFO *pRes
    )
{
    struct tagNEWHEADER gh;
    struct tagDESCRIPTOR ds;
    BITMAPINFOHEADER bmh;
    RGBQUAD *rgbq;
    short rgwMask[16];
    short rgwData[16];
    int xyBest;
    int xScale, yScale;
    char* pbBits;
    int ibDescNext, ibDescBest;
    int ires;
    int y, dy;
    int cbWidth;

    /* The input file has already been opened; read in the bitmap file header */

    MyRead(fhCode, (char*)&gh, sizeof(struct tagNEWHEADER));
    if (gh.Reserved != 0 || gh.ResType != 2)
        GenError2(2175, (PCHAR)tokenbuf);

    /* find the best-looking cursor */

    xyBest = 32767;
    ibDescBest = -1;
    ibDescNext = MySeek(fhCode, 0L, SEEK_CUR);
    for (ires = 0; ires < gh.ResCount; ires++) {
        /* Read in the descriptor */

        MySeek(fhCode, ibDescNext, SEEK_SET);
        MyRead(fhCode, (char*)&ds, sizeof(struct tagDESCRIPTOR));
        ibDescNext = MySeek(fhCode, 0L, SEEK_CUR);

        /* get bitmap header */

        MySeek(fhCode, ds.OffsetToBits, SEEK_SET);
        MyRead(fhCode, (char*)&bmh, sizeof(BITMAPINFOHEADER));
        if (bmh.biSize != sizeof(BITMAPINFOHEADER))
            GenError2(2176, (PCHAR)tokenbuf);
        /* !!! could we be smarter here about smaller cursors? */
        if (bmh.biBitCount != 1 || bmh.biPlanes != 1 ||
                bmh.biWidth % 16 != 0 || bmh.biHeight % 32 != 0)
            continue;
        xScale = bmh.biWidth / 16;
        yScale = bmh.biHeight / 32;
        if (xScale > 2)
            continue;
        if (xScale * yScale < xyBest) {
            xyBest = xScale * yScale;
            ibDescBest = ibDescNext - sizeof(struct tagDESCRIPTOR);
        }
    }

    /* if no usable cursor found, bail out */

    if (ibDescBest == -1) {
        GenWarning2(4507, (PCHAR)tokenbuf);
        return;
    }

    /* go back and get the best descriptor and bitmap header */

    ReadDIB(ibDescBest, &ds, &bmh, &cbWidth, (void **) &pbBits, &rgbq, FALSE);

    /* if our color table is backwards, invert the bits */

    if ((rgbq[0].rgbRed == 0xff) &&
        (rgbq[0].rgbGreen == 0xff) &&
        (rgbq[0].rgbBlue == 0xff))
    {
        int cb;
        for (cb = cbWidth * bmh.biHeight; cb > 0; cb--)
            pbBits[cb] = ~pbBits[cb];
    }

    /* if necessary, scale the bits down to 16x16 */

    if (xyBest != 1) {
        GenWarning2(4506, (PCHAR)tokenbuf);
        if (bmh.biWidth > 16) {
            assert(bmh.biWidth == 32);
            ds.xHotSpot /= (int)(bmh.biWidth / 16);
            CrunchX2((unsigned char *) pbBits, (unsigned char *) pbBits, cbWidth, bmh.biHeight);
            cbWidth = 2;
        }
        if (bmh.biHeight > 32) {
            ds.yHotSpot /= (int)(bmh.biHeight / 32);
            CrunchY((unsigned char *) pbBits, (unsigned char *) pbBits, cbWidth, bmh.biHeight, bmh.biHeight/32);
            bmh.biHeight = 32;
        }
    }

    /* now build the CURS resource mask and data */

    dy = bmh.biHeight/2;
    if (cbWidth == 1) {
        for (y = dy; y > 0; y--) {
            rgwMask[dy-y] = pbBits[y-1];
            rgwData[dy-y] = pbBits[dy+y-1] ^ ~rgwMask[dy-y];
        }
    } else {
        for (y = dy; y > 0; y--) {
            rgwMask[dy-y] = ~*(short*)&pbBits[(dy+y-1)*cbWidth];
            rgwData[dy-y] = *(short*)&pbBits[(y-1)*cbWidth] ^ rgwMask[dy-y];
        }
    }
    for (y = dy; y < 16; y++) {
        rgwMask[y] = 0;
        rgwData[y] = 0;
    }

    /* and write out the CURS resource data */

    WriteBuffer((char*)rgwData, 32);
    WriteBuffer((char*)rgwMask, 32);
    WriteWord(ds.yHotSpot);
    WriteWord(ds.xHotSpot);

    pRes->size = 32 + 32 + 2 + 2;

    /* and we're done - cleanup and return */

    MyFree(pbBits);
    MyFree(rgbq);
    fclose(fhCode);
    fhCode = NULL_FILE;
    AddResToResFile(pType, pRes, CodeArray, CCount, 0);
}


void
ReadDIB(
    int ibDesc,
    struct tagDESCRIPTOR* pds,
    BITMAPINFOHEADER* pbmh,
    int* pcbWidth,
    void** ppBits,
    RGBQUAD** prgq,
    BOOL fIcon
    )
{
    int cbBits;
    int iq;

    MySeek(fhCode, ibDesc, SEEK_SET);
    MyRead(fhCode, (char*)pds, sizeof(struct tagDESCRIPTOR));
    MySeek(fhCode, pds->OffsetToBits, SEEK_SET);
    MyRead(fhCode, (char *)pbmh, sizeof(BITMAPINFOHEADER));

    /* get the color table and map to macintosh color palette while we're
       looking at it */

    if (pbmh->biClrUsed == 0)
        pbmh->biClrUsed = 1 << pbmh->biBitCount;
    *prgq = (RGBQUAD*)MyAlloc((WORD)(pbmh->biClrUsed * sizeof(RGBQUAD)));
    MyRead(fhCode, (char *)*prgq, (WORD)(pbmh->biClrUsed*sizeof(RGBQUAD)));
    switch (pbmh->biBitCount) {
        case 1:
            for (iq = 0; iq < (int)pbmh->biClrUsed; iq++)
                LookupIconColor(rgcs2, ccs2, &(*prgq)[iq]);
            break;
        case 4:
            for (iq = 0; iq < (int)pbmh->biClrUsed; iq++)
                LookupIconColor(rgcs16, ccs16, &(*prgq)[iq]);
            break;
        case 8:
            // !!! should use 256-color palette
            for (iq = 0; iq < (int)pbmh->biClrUsed; iq++)
                LookupIconColor(rgcs256, ccs256, &(*prgq)[iq]);
            break;
        default:
            break;
    }

    /* allocate space for the bits, and load them in */

    *pcbWidth = (pbmh->biBitCount*pbmh->biWidth+31)/32*4;
    if (fIcon)
        cbBits = (*pcbWidth * pbmh->biHeight/2) + ((pbmh->biWidth+31)/32*4) * (pbmh->biHeight/2);
    else
        cbBits = *pcbWidth * pbmh->biHeight;
    *ppBits = MyAlloc((WORD)cbBits);
    MyRead(fhCode, *ppBits, (WORD)cbBits);
}


void
LookupIconColor(
    ColorSpec* rgcs,
    int ccs,
    RGBQUAD* pq
    )
{
    int ics, icsBest;
    int dred, dgreen, dblue;
    int drgb, drgbBest;

    drgbBest = 32767;
    icsBest = -1;
    for (ics = 0; ics < ccs; ics++) {
        dred = pq->rgbRed - (rgcs[ics].rgb.red>>8);
        dgreen = pq->rgbGreen - (rgcs[ics].rgb.green>>8);
        dblue = pq->rgbBlue - (rgcs[ics].rgb.blue>>8);
        drgb = abs(dred) + abs(dgreen) + abs(dblue);
        if (drgb < drgbBest) {
            drgbBest = drgb;
            icsBest = ics;
            if (drgbBest == 0)
                break;
        }
    }
    pq->rgbReserved = (BYTE)rgcs[icsBest].value;
}


BOOL
IsIcon(
    TYPEINFO* ptype
    )
{
    unsigned long rt;
    short it;

    if (ptype->type == 0)
        return FALSE;
    rt = res_type(ptype->type[0], ptype->type[1], ptype->type[2], ptype->type[3]);
    for (it = 0; it < itMax; it++)
        if (rt == mpitres[it])
            return TRUE;
    return FALSE;
}


void
CrunchX2(
    unsigned char* pbSrc,
    unsigned char* pbDst,
    int cbWidth,
    int dy
    )
{
    unsigned short cw, cwWidth;
    unsigned short w;
    unsigned char b = 0;
    short bit;

    assert(dy > 0);
    assert(cbWidth > 1);

    cwWidth = cbWidth / 2;
    do {
        cw = cwWidth;
        do {
            w = (*pbSrc << 8)|(*(pbSrc+1));
            pbSrc += 2;
            bit = 8;
            do {
                b >>= 1;
                if ((w & 3) == 3)   /* if both are white, keep white */
                    b += 0x80;
                w >>= 2;
            } while (--bit != 0);
            *pbDst++ = b;
        } while (--cw > 0);
        pbDst += cwWidth & 1;
    } while (--dy > 0);
}


void
CrunchY(
    unsigned char* pbSrc,
    unsigned char* pbDst,
    int WidthBytes,
    int dySrc,
    int scale
    )
{
    int cbGroup;
    int cwRow;
    int dyDst, dy;
    unsigned short w;
    unsigned char *pb;

    if (scale <= 1) {
        memcpy(pbDst, pbSrc, dySrc * WidthBytes);
        return;
    }
    dyDst = dySrc / scale;
    cbGroup = WidthBytes * (scale - 1);

    do {
        cwRow = WidthBytes / sizeof(unsigned short);
        do {
            pb = pbSrc;
            w = *(unsigned short*)pb;
            dy = scale - 1;
            do {
                pb += WidthBytes;
                w &= *(unsigned short*)pb;
            } while (--dy > 0);
            *((unsigned short*)pbDst) = w;
            pbDst += sizeof(unsigned short);
            pbSrc += sizeof(unsigned short);
        } while (--cwRow > 0);
        pbSrc += cbGroup;
    } while (--dyDst > 0);
}

/*  WriteMacMap
 *
 *  Writes out a macintosh resource map from the type and resource
 *  data stashed away in the type and resource lists
 *
 *  See Inside Mac, Volume I, for a fine description of the
 *  format of a macintosh resource file
 */
void
WriteMacMap(
    void
    )
{
    TYPEINFO *ptype;
    RESINFO *pres;
    int i;
    size_t cch;
    int cbNameTbl, ctype, cref, ibName;
    long cbData;
    int offRef;
    WCHAR *pch;
#define cbMacType 8
#define cbMacRef 12

    /* alright, we're done reading all this crap in, run through all
       our type lists and see what we've accumulated */

    cbData = MySeek(fhBin, 0L, 1) - MACDATAOFFSET;
    ctype = 0;
    cref = 0;
    cbNameTbl = 0;

    for (ptype = pTypInfo; ptype != 0; ptype = ptype->next) {
        if (ptype->nres == 0)
            continue;
        ctype++;
        cref += ptype->nres;
        for (pres = ptype->pres; pres != 0; pres = pres->next) {
            /* make sure each reference has a unique resource id */
            if (pres->nameord == 0)
                pres->nameord = (USHORT)IdUnique(ptype, pres);
            if (pres->name != 0)
                cbNameTbl += wcslen(pres->name)+1;
        }
    }

    /* write out the resource header at offset 0 in the file */

    MySeek(fhBin, 0L, 0);
    CtlInit();
    WriteLong((long)MACDATAOFFSET);
    WriteLong((long)MACDATAOFFSET + cbData);
    WriteLong(cbData);
    WriteLong((long)(16+4+2+2+2+2+2 + ctype*cbMacType + cref*cbMacRef + cbNameTbl));
    for (i = (MACDATAOFFSET - 16)/4; i-- > 0; )
        WriteLong(0);
    MyWrite(fhBin, (LPSTR)CodeArray, CCount);

    /* we've already written out all the data, now write out the
       beginning of the map part of the resource file */

    MySeek(fhBin, (long)MACDATAOFFSET + cbData, 0);
    CtlInit();
    /* 24 bytes of 0s */
    for (i = 6; i-- > 0; )
        WriteLong(0);
    /* offset to start of type list */
    WriteWord(28);
    /* offset to start of name list */
    WriteWord((USHORT)(28 + 2 + ctype * cbMacType + cref * cbMacRef));

    /* dump out type table of the resource map */

    WriteWord((USHORT)(ctype - 1));
    offRef = 2 + ctype * cbMacType;
    for (ptype = pTypInfo; ptype != 0; ptype = ptype->next) {
        long rt;
        TYPEINFO *ptypeX;

        if (ptype->nres == 0)
            continue;
        /* 32-bit resource name - verify name truncation didn't
           cause conflicts */
        rt = MungeResType(ptype->type, ptype->typeord);
        for (ptypeX = ptype->next; ptypeX != 0; ptypeX = ptypeX->next) {
            if (rt == MungeResType(ptypeX->type, ptypeX->typeord)) {
                char szMac[8], szType1[128], szType2[128];
                szMac[0] = (BYTE)(rt>>24);
                szMac[1] = (BYTE)(rt>>16);
                szMac[2] = (BYTE)(rt>>8);
                szMac[3] = (BYTE)(rt);
                szMac[4] = 0;
                if (ptype->typeord)
                    wsprintfA(szType1, "%d", ptype->typeord);
                else
                    wsprintfA(szType1, "%ws", ptype->type);
                if (ptypeX->typeord)
                    wsprintfA(szType2, "%d", ptypeX->typeord);
                else
                    wsprintfA(szType2, "%ws", ptypeX->type);
                GenWarning4(4509, szType1, szType2, szMac);
            }
        }
        WriteLong(rt);
        /* number of references of this type */
        WriteWord((USHORT)(ptype->nres-1));
        /*  offset to the reference list for this type */
        WriteWord((USHORT)offRef);
        offRef += ptype->nres * cbMacRef;
    }

    /* dump out reference table of the resource map */

    ibName = 0;
    for (ptype = pTypInfo; ptype != 0; ptype = ptype->next) {
        if (ptype->nres == 0)
            continue;
        for (pres = ptype->pres; pres != 0; pres = pres->next) {
            /* resource id */
            WriteWord(pres->nameord);
            /* offset to name in namelist */
            if (pres->name == 0) {
                WriteWord(0xffff);  /* unnamed, use -1 */
            } else {
                WriteWord((USHORT)ibName);
                ibName += wcslen(pres->name)+1;
            }
            /* attributes and resource data offset */
            WriteLong(pres->BinOffset);
            /* must be 0 */
            WriteLong(0L);
        }
    }

    /* and finally, dump out name table */

    /* note that we've implemented the Unicode=>ASCII conversion here by
       simply dumping out the low byte of each Unicode character. Effectively,
       we're assuming that resource names will be ASCII. Changing this would
       require changing the output code here and also changing a few places
       where we use wcslen to calculate the number of bytes that the ASCII
       resource name will require. If the resource name can contain 2-byte
       characters we would need to convert the Unicode to multi-byte and
       then count characters instead of just calling wcslen. */

    for (ptype = pTypInfo; ptype != 0; ptype = ptype->next) {
        if (ptype->nres == 0)
            continue;
        for (pres = ptype->pres; pres != 0; pres = pres->next) {
            if (pres->name == 0)
                continue;
            WriteByte(cch = wcslen(pres->name));
            for (pch = pres->name; cch--; )
                WriteByte((BYTE)*pch++);
        }
    }
    MyWrite(fhBin, (LPSTR)CodeArray, CCount);
}


long
MungeResType(
    WCHAR *szType,
    short wOrd
    )
{
    long rt;
    int ich;

    switch (wOrd) {
        case 0:
            assert(szType != NULL && *szType != 0);
            rt = 0;
            for (ich = 0; ich < 4; ich++) {
                rt <<= 8;
                if (*szType)
                    rt |= (BYTE) (*szType++);
                else
                    rt |= ' ';
            }
            break;
        case RT_CURSOR:
            rt = 'CURS';
            break;
        case RT_BITMAP:
            rt = 'WBMP';
            break;
        case RT_ICON:
            rt = 'WICO';
            break;
        case RT_MENU:
            rt = 'WMNU';
            break;
        case RT_DIALOG:
            rt = 'WDLG';
            break;
        case RT_STRING:
            rt = 'STR#';
            break;
        case RT_ACCELERATOR:
            rt = 'WACC';
            break;
        case RT_RCDATA:
        case RT_DLGINIT:
            rt = 'HEXA';
            break;
        case RT_TOOLBAR:
            rt = 'TLBR';
            break;
        case RT_GROUP_CURSOR:
            rt = 'CURS';
            break;
        case RT_GROUP_ICON:
            rt = 'WGIC';
            break;
        case RT_VERSION:
            rt = 'WVER';
            break;
        case RT_FONTDIR:
        case RT_FONT:
        //case RT_ERRTABLE:
        //case RT_NAMETABLE:
        default: {
            static char rgchHex[] = "0123456789ABCDEF";
            char ch4 = rgchHex[wOrd & 0x0f];
            char ch3 = rgchHex[(wOrd >> 4) & 0x0f];
            char ch2 = rgchHex[(wOrd >> 8) & 0x0f];
            char ch1 = 'M' + ((wOrd >> 12) & 0x0f);
            rt = res_type(ch1,ch2,ch3,ch4);
            break;
        }
    }

    return rt;
}


/*  IdUnique
 *
 *  Searches through the items of the given type looking for
 *  an unused resource id.  Returns the smallest resource id
 *  that is not currently used.
 *
 *  This routine handles icon families in a particular annoying
 *  way, using a particularly inefficient algorithm.  But it
 *  does keep icon ids synchronized if they have the same name.
 *
 *  Entry:
 *      ptype - type to search
 *      pres - resource type needing the unique id
 *
 *  Exit:
 *      retunrs - a unique resource id
 */
int
IdUnique(
    TYPEINFO *ptype,
    RESINFO *pres
    )
{
    int id;
    RESINFO *presScan;
    TYPEINFO* ptypeIcon;

    assert(ptype->pres != 0);

    if (IsIcon(ptype)) {
        /* see if we've already found an id for an icon with the same name */

        assert(pres->name != NULL);
        for (ptypeIcon = pTypInfo; ptypeIcon != NULL; ptypeIcon = ptypeIcon->next) {
            if (!IsIcon(ptypeIcon))
                continue;
            for (presScan = ptypeIcon->pres; presScan != NULL; presScan = presScan->next) {
                if (presScan->name == NULL || presScan->nameord == 0)
                    continue;
                if (wcscmp(presScan->name, pres->name) == 0)
                    return presScan->nameord;
            }
        }

        /* rats, didn't find it, gotta find one that's unique in *all* the
           icon types */

        for (id = idBase; ; ) {
            for (ptypeIcon = pTypInfo; ptypeIcon != NULL; ptypeIcon = ptypeIcon->next) {
                if (!IsIcon(ptypeIcon))
                    continue;
                for (presScan = ptypeIcon->pres; presScan != NULL; presScan = presScan->next) {
                    if (presScan->nameord == id)
                        goto NextId;
                }
            }
            return id;
NextId:
            id = (id+1) & 0xffff;
            if (id == 0)
                id = 1;
        }
    } else {
        for (id = idBase; ; ) {
            for (presScan = ptype->pres; presScan->nameord != id; ) {
                presScan = presScan->next;
                if (presScan == 0)
                    return id;
            }
            id = (id+1) & 0xffff;
            if (id == 0)
                id = 1;
        }
    }
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      GetToolbar -                                                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

SHORT
GetToolbarValue(
    void
    )
{
    SHORT sVal;

    if (!GetFullExpression(&sVal, GFE_ZEROINIT | GFE_SHORT))
        ParseError1(2250); //"expected numerical toolbar constant"

    return(sVal);
}

void
GetButtonSize(
    PSHORT cx,
    PSHORT cy
    )
{
    *cx= GetToolbarValue();
    if (token.type == COMMA)
        GetToken(TOKEN_NOEXPRESSION);
    *cy= GetToolbarValue();
}

int
GetToolbar(
    PRESINFO pRes
    )
{
    SHORT cx, cy;
    BOOL    bItemRead = FALSE;

    WriteWord(0x0001);  // Version 1 of this resource.

    GetButtonSize(&cx, &cy);

    WriteWord(cx);
    WriteWord(cy);

    ItemCountLoc = CCount;        /* global marker for location of item cnt. */

    /* skip place for num of items */
    WriteWord(0);

    PreBeginParse(pRes, 2251);

    while (token.type != END) {
        switch (token.type) {
            case TKSEPARATOR:
                bItemRead = TRUE;
                GetToken(TOKEN_NOEXPRESSION);
                WriteWord(0);
                break;

            case TKBUTTON:
                bItemRead = TRUE;
                GetToken(TRUE);
                if (token.type != NUMLIT)
                    ParseError1(2250); //"expected numerical toolbar constant"

                WriteSymbolUse(&token.sym);

                WriteWord(GetToolbarValue());
                break;

            case EOFMARK:
                ParseError1(2252); //"END expected in toolbar"
                quit("\n");
                break;

            default:
                ParseError1(2253); //"unknown toolbar item type"
                GetToken(TOKEN_NOEXPRESSION);   // try to continue
                continue;
        }

        IncItemCount();
    }

    /* make sure we have a toolbar item */
    if (!bItemRead)
        ParseError1(2254); //"empty toolbars not allowed"

    if (fMacRsrcs)
        SwapItemCount();

    return (TRUE);
}
