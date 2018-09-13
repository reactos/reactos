/**
**      Header for the New version of RC.EXE. This contains the structures
**      for new format of BITMAP files.
**/

/*  The width of the name field in the Data for the group resources */
#ifndef RC_INVOKED       // RC can't handle #pragmas
#pragma pack(2)

typedef struct tagBITMAPHEADER
  {
    DWORD   Size;
    WORD    Width;
    WORD    Height;
    WORD    Planes;
    WORD    BitCount;
  } BITMAPHEADER;

// IDIOTS!  WHY WASN'T THIS DEFINED TO BE SAME AS RESOURCE FORMAT?
// Image File header
typedef struct tagIMAGEFILEHEADER
{
    BYTE    cx;
    BYTE    cy;
    BYTE    nColors;
    BYTE    iUnused;
    WORD    xHotSpot;
    WORD    yHotSpot;
    DWORD   cbDIB;
    DWORD   offsetDIB;
} IMAGEFILEHEADER;

// File header
#define FT_ICON     1
#define FT_CURSOR   2

typedef struct tagICONFILEHEADER
{
        WORD iReserved;
        WORD iResourceType;
        WORD cresIcons;
        IMAGEFILEHEADER imh[1];
} ICONFILEHEADER;

typedef struct tagNEWHEADER {
    WORD    Reserved;
    WORD    ResType;
    WORD    ResCount;
} NEWHEADER, *LPNEWHEADER;

typedef struct tagICONDIR
{
        BYTE  Width;            /* 16, 32, 64 */
        BYTE  Height;           /* 16, 32, 64 */
        BYTE  ColorCount;       /* 2, 8, 16 */
        BYTE  reserved;
} ICONDIR;

// Format of resource directory (array of resources)

typedef struct tagRESDIR
{
        ICONDIR Icon;
        WORD    Planes;
        WORD    BitCount;
        DWORD   BytesInRes;
        WORD    idIcon;
} RESDIR, *LPRESDIR;

typedef struct tagRESDIRDISK
{
        struct  tagICONDIR  Icon;

        WORD   Reserved[2];
        DWORD  BytesInRes;
        DWORD  Offset;
} RESDIRDISK, *LPRESDIRDISK;

#pragma pack()
#endif // !RC_INVOKED
