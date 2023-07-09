/**
**      Header for the New version of RC.EXE. This contains the structures
**      for new format of BITMAP files.
**/

/*  The width of the name field in the Data for the group resources */

#define  NAMELEN    14

typedef struct tagNEWHEADER
{
    WORD    Reserved;
    WORD    ResType;
    WORD    ResCount;
} NEWHEADER, *PNEWHEADER;

typedef struct tagDESCRIPTOR
{
    BYTE    Width;      // 16, 32, 64
    BYTE    Height;     // 16, 32, 64
    BYTE    ColorCount; //  2,  8, 16
    BYTE    reserved;
    WORD    xHotSpot;
    WORD    yHotSpot;
    DWORD   BytesInRes;
    DWORD   OffsetToBits;
} DESCRIPTOR;

typedef struct tagICONRESDIR
{
    BYTE    Width;      // 16, 32, 64
    BYTE    Height;     // 16, 32, 64
    BYTE    ColorCount; //  2,  8, 16
    BYTE    reserved;
} ICONRESDIR;

typedef struct tagCURSORDIR
{
    WORD    Width;
    WORD    Height;
} CURSORDIR;

typedef struct tagRESDIR
{
    union
    {
        ICONRESDIR   Icon;
        CURSORDIR    Cursor;
    } ResInfo;
    WORD    Planes;
    WORD    BitCount;
    DWORD   BytesInRes;
} RESDIR;

typedef struct tagLOCALHEADER
{
    WORD    xHotSpot;
    WORD    yHotSpot;
} LOCALHEADER;

typedef struct tagBITMAPHEADER
{
    DWORD   biSize;
    DWORD   biWidth;
    DWORD   biHeight;
    WORD    biPlanes;
    WORD    biBitCount;
    DWORD   biStyle;
    DWORD   biSizeImage;
    DWORD   biXPelsPerMeter;
    DWORD   biYPelsPerMeter;
    DWORD   biClrUsed;
    DWORD   biClrImportant;
}  BITMAPHEADER;


#define BFOFFBITS(pbfh) MAKELONG(*(LPWORD)((LPWORD)pbfh+5), \
                                 *(LPWORD)((LPWORD)pbfh+6))

#define TOCORE(bi) (*(BITMAPCOREHEADER *)&(bi))

/****************************************************\
*                                                    *
*      Imported from asdf.h in windows\inc           *
*                                                    *
\****************************************************/

// RIFF chunk header.

typedef struct _RTAG {
    FOURCC ckID;
    DWORD ckSize;
} RTAG, *PRTAG;


// Valid TAG types.

// 'ANI ' - simple ANImation file

#define FOURCC_ACON  mmioFOURCC('A', 'C', 'O', 'N')


// 'anih' - ANImation Header
// Contains an ANIHEADER structure.

#define FOURCC_anih mmioFOURCC('a', 'n', 'i', 'h')


// 'rate' - RATE table (array of jiffies)
// Contains an array of JIFs.  Each JIF specifies how long the corresponding
// animation frame is to be displayed before advancing to the next frame.
// If the AF_SEQUENCE flag is set then the count of JIFs == anih.cSteps,
// otherwise the count == anih.cFrames.

#define FOURCC_rate mmioFOURCC('r', 'a', 't', 'e')


// 'seq ' - SEQuence table (array of frame index values)
// Countains an array of DWORD frame indices.  anih.cSteps specifies how
// many.

#define FOURCC_seq  mmioFOURCC('s', 'e', 'q', ' ')


// 'fram' - list type for the icon list that follows

#define FOURCC_fram mmioFOURCC('f', 'r', 'a', 'm')

// 'icon' - Windows ICON format image (replaces MPTR)

#define FOURCC_icon mmioFOURCC('i', 'c', 'o', 'n')


// Standard tags (but for some reason not defined in MMSYSTEM.H)

#define FOURCC_INFO mmioFOURCC('I', 'N', 'F', 'O')      // INFO list
#define FOURCC_IART mmioFOURCC('I', 'A', 'R', 'T')      // Artist
#define FOURCC_INAM mmioFOURCC('I', 'N', 'A', 'M')      // Name/Title

#if 0 //in winuser.w
typedef DWORD JIF;  // in winuser.w

typedef struct _ANIHEADER {     // anih
    DWORD cbSizeof;
    DWORD cFrames;
    DWORD cSteps;
    DWORD cx, cy;
    DWORD cBitCount, cPlanes;
    JIF   jifRate;
    DWORD fl;
} ANIHEADER, *PANIHEADER;

// If the AF_ICON flag is specified the fields cx, cy, cBitCount, and
// cPlanes are all unused.  Each frame will be of type ICON and will
// contain its own dimensional information.

#define AF_ICON     0x0001L     // Windows format icon/cursor animation

#define AF_SEQUENCE 0x0002L     // Animation is sequenced
#endif

/**************************\
*                          *
*  End Import from asdf.h  *
*                          *
\**************************/
