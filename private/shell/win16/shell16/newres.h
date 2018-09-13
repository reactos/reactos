/**
**      Header for the New version of RC.EXE. This contains the structures
**      for new format of BITMAP files.
**/

/*  The width of the name field in the Data for the group resources */

typedef struct tagBITMAPHEADER
  {
    DWORD   Size;
    WORD    Width;
    WORD    Height;
    WORD    Planes;
    WORD    BitCount;
  } BITMAPHEADER;

typedef struct tagICONDIR
{
        BYTE  Width;            /* 16, 32, 64 */
        BYTE  Height;           /* 16, 32, 64 */
        BYTE  ColorCount;       /* 2, 8, 16 */
        BYTE  reserved;
} ICONDIR;

typedef struct tagRESDIR
{
        struct  tagICONDIR  Icon;

        WORD   Planes;
        WORD   BitCount;
        DWORD  BytesInRes;
        WORD   idIcon;
} RESDIR, FAR *LPRESDIR;


typedef struct tagRESDIRDISK
{
        struct  tagICONDIR  Icon;

        WORD   Reserved[2];
        DWORD  BytesInRes;
        DWORD  Offset;
} RESDIRDISK, FAR *LPRESDIRDISK;

