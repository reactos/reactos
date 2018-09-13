/* FONT DEFINITIONS
    Written by Ed Mills, 1984
    Edited by McGregor, 1/10/85
*/

/* this flag, defined in kernel.h tells us whether or not we should
** be loading fonts above the line.  if it is returned by GetCurPID,
** then we need to use the fonts-above-the-line scheme
*/
#define EMSF_LIB_CODE   0x0004          /* Library code segs above the line */

/* Include file to define types and constants needed by the font manager
   and font routines. */

#define SMART_FONT_VERSION  0x0200
#define FONT_MAN_VERSION    0x0100

/* The name of the font resource file. */
#define FONTRESOURCEFILE    TEXT("FONTS.FON")

/* The error code we pass to FatalExit when we die. */
#define FONTFATALEXITCODE   -2

/* Define the number of rows to add when we grow the physical font table and
   physical font instance table. */
#define PFT_GROW_SIZE   2
#define PFI_GROW_SIZE   5
#define RT_GROW_SIZE    1

#define WAIT_FOR_PFI_READ()
#define RELEASE_PFI_READ()
#define WAIT_FOR_PFI_WRITE()
#define RELEASE_PFI_WRITE()
#define WAIT_FOR_PFT_READ()
#define RELEASE_PFT_READ()
#define WAIT_FOR_PFT_WRITE()
#define RELEASE_PFT_WRITE()
#define WAIT_FOR_RT_READ()
#define RELEASE_RT_READ()
#define WAIT_FOR_RT_WRITE()
#define RELEASE_RT_WRITE()


/* The number of physical font instances in the instance table. */
extern WORD     Num_PF_Instances;

/* The number of physical font entries in physical font table. */
extern WORD     Num_Phys_Fonts;

/* The number of Resources loaded in the resource table. */
extern WORD     Num_Resources;

extern WORD     Size_PF_Inst_Tab;       /* size of instance table */
extern WORD     Size_PFont_Tab;         /* size of font table */
extern WORD     Size_Res_Tab;           /* size of resource table */

extern LOCALHANDLE      PF_Inst_Tab;    /* Handle to the instance table. */
extern GLOBALHANDLE     PFont_Table;    /* Handle to the font table. */
extern LOCALHANDLE      Res_Table;      /* Handle to the resource table. */

/* The begining of a font file. */
typedef     struct  {
    SHORT       Version;
    DWORD       Size;
    CHAR        Copyright[60];   //This MUST be ANSI for compatibility
} Font_Header_Type;

/* PFont_Tab_Type */
/* For an exact description of the fields in this structure see 'Font
   Files' in section 6.4 of the Windows Adaptation Guide. */
typedef     struct  {
    HANDLE          ResHandle;       /* resource module handle */
    WORD            ResIndex;        /* resource file index word */
    GLOBALHANDLE    PF_Handle;   /* font as formatted for STRBLT */
    GLOBALHANDLE    PID_Handle;  /* handle to creator's Process ID */
    WORD            AboveLineCopy;       /* ack */
    WORD            DC_Use_Count;    /* num of DC's with this font selected*/
    SHORT           Use_Count;       /* PFI's using this font */
    ATOM            DN_Index;        /* device name atom */
    ATOM            FN_Index;        /* face name atom */
    SHORT           Type;                /* font type (i.e. raster, vector) */
    SHORT           Points;              /* point size */
    SHORT           VertRes;         /* vertical digitization */
    SHORT           HorizRes;        /* horizontal digitization */
    SHORT           Ascent;              /* baseline offset from top */
    SHORT           InternalLeading; /* Internal leading included in font */
    SHORT           ExternalLeading; /* Prefered extra space between lines */
    BYTE            Italic;              /* flag for italic */
    BYTE            Underline;       /* flag for underlining */
    BYTE            StrikeOut;       /* flag for strikeout */
    SHORT           Weight;              /* weight of font */
    BYTE            CharSet;         /* character set */
    SHORT           PixWidth;        /* font bitmap width */
    SHORT           PixHeight;       /* font bitmap height */
    BYTE            PitchAndFamily;  /* fixed or variable pitch and family */
    SHORT           AvgWidth;        /* average character width */
    SHORT           MaxWidth;        /* maximum character width */
    BYTE            FirstChar;       /* first valid character */
    BYTE            LastChar;        /* last valid character */
    BYTE            DefaultChar;     /* displayed for invalid chars */
    BYTE            BreakChar;       /* defines wordbreaks */
}PFont_Tab_Type;


/* PF_Inst_Type */
typedef     struct  {
    DWORD           PDeviceObjCount;    /* PDevice object count. */
    SHORT           WndExtX;
    SHORT           WndExtY;
    SHORT           VprtExtX;
    SHORT           VprtExtY;
    DWORD           LogFontObjCount;    /* LogFont object count. */
    GLOBALHANDLE    ProcID_Handle;  /* creator's Process ID */
    GLOBALHANDLE    PFont_Handle;   /* physical font */
    LOCALHANDLE     FTrans_Handle;  /* font xform */
    WORD            PFont_Index;        /* index in PFont_Table */
}PF_Inst_Type;


#define DEVICE_FONT        (((WORD)0xFFFF) / sizeof(PFont_Tab_Type))   /* Realized by the device. */
#define MAX_PFT_INDEX   (DEVICE_FONT-2)
#define BAD_PFT_INDEX   (DEVICE_FONT-1)         /* Map_Font failed. */


/* Res_Tab_Type */
typedef     struct  {
    HANDLE          ResHandle;      /* resource module handle */
    SHORT           Use_Count;      /* PFT's using this resource */
    SHORT           Add_Count;      /* How many times it's been added. */
    WORD            First;              /* PFT index of first and last */
    WORD            Last;               /* entry for fonts from this resource. */
}Res_Tab_Type;


/* font file header (Adaptation Guide section 6.4) */
typedef struct {
    WORD        dfVersion;                  /* not in FONTINFO */
    DWORD       dfSize;                     /* not in FONTINFO */
    BYTE        dfCopyright[60];        /* not in FONTINFO */
    WORD        dfType;
    WORD        dfPoints;
    WORD        dfVertRes;
    WORD        dfHorizRes;
    WORD        dfAscent;
    WORD        dfInternalLeading;
    WORD        dfExternalLeading;
    BYTE        dfItalic;
    BYTE        dfUnderline;
    BYTE        dfStrikeOut;
    WORD        dfWeight;
    BYTE        dfCharSet;
    WORD        dfPixWidth;
    WORD        dfPixHeight;
    BYTE        dfPitchAndFamily;
    WORD        dfAvgWidth;
    WORD        dfMaxWidth;
    BYTE        dfFirstChar;
    BYTE        dfLastChar;
    BYTE        dfDefaultCHar;
    BYTE        dfBreakChar;
    WORD        dfWidthBytes;
    DWORD       dfDevice;               /* See Adaptation Guide 6.3.10 and 6.4 */
    DWORD       dfFace;                 /* See Adaptation Guide 6.3.10 and 6.4 */
    DWORD       dfBitsPointer;      /* See Adaptation Guide 6.3.10 and 6.4 */
}FFH;

typedef FFH        *PFFH;
typedef FFH    FAR *LPFFH;

#define DF_MAPSIZE 1

typedef struct {
     FFH    fhHeader;
     DWORD  dfBitsOffset;
     BYTE   dfMaps[DF_MAPSIZE];
}FFHEADER;

typedef FFHEADER        *PFFHEADER;
typedef FFHEADER    FAR *LPFFHEADER;

#define FONT_FIXED sizeof(FFH)

