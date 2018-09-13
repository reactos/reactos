//*****************************************************************************
//
// LoadAccelerator - compatibility support.
//
//
// 23-Jul-92  NanduriR   Created.
//*****************************************************************************


typedef struct _ACCELALIAS {
    struct _ACCELALIAS FAR *lpNext;
    struct _ACCELALIAS FAR *lpPrev;
    HAND16  hInst;
    HTASK16 hTask16;
    WORD    f16;            // WORD only for padding, could be BOOL,DWORD
    HAND16  h16;
    HAND32  h32;
} ACCELALIAS, FAR *LPACCELALIAS;


ULONG FASTCALL WU32LoadAccelerators(VPVOID vpData);
LPACCELALIAS SetupAccelAlias(HAND16 hInstance, HAND16 hAccel16, HAND32 hAccel32, BOOL f16);
BOOL  DestroyAccelAlias(WORD hTask16);
LPACCELALIAS FindAccelAlias(HANDLE hAccel, UINT fSize);
HAND32 GetAccelHandle32(HAND16 h16);
HAND16 GetAccelHandle16(HAND32 h32);
HACCEL CreateAccel32(VPVOID vpAccel16, DWORD cbAccel16);
HAND16 CreateAccel16(HACCEL hAccel32);


#define HANDLE_16BIT       0x01
#define HANDLE_32BIT       0x02
#define HACCEL32(h16)      GetAccelHandle32(h16)
#define GETHACCEL16(h32)   GetAccelHandle16(h32)
