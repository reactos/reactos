//*****************************************************************************
//
// Cursor and Icon compatibility Support -
//
//     Support for apps - which do a GlobalLock on Cursors and Icons to
//     create headaches for us.
//
//     A compatibility issue.
//
//
// 21-Apr-92  NanduriR   Created.
//
//*****************************************************************************

#define HANDLE_TYPE_UNKNOWN  0x01
#define HANDLE_TYPE_ICON     0x02
#define HANDLE_TYPE_CURSOR   0x04

#define HANDLE_TYPE_WOWGLOBAL 0x08

#define HANDLE_16BIT       0x01
#define HANDLE_32BIT       0x02
#define HANDLE_16BITRES    0x04

#define CIALIAS_TASKISGONE  0x01
#define CIALIAS_HMOD        0x02
#define CIALIAS_HTASK       0x04

typedef struct _CURSORICONALIAS {
    struct _CURSORICONALIAS FAR *lpNext;
    BYTE   fInUse;
    BYTE   flType;
    HAND16 h16;
    HAND32 h32;
    HAND16 hInst16;
    HAND16 hMod16;
    HTASK16 hTask16;
    WORD    hRes16;         // 16bit resource handle
    WORD    cbData;
    UINT    cLock;
    VPVOID  vpData;
    LPBYTE  pbDataOld;
    LPBYTE  pbDataNew;
    LPBYTE  lpszName;       // name of 16bit resource
} CURSORICONALIAS, FAR *LPCURSORICONALIAS;


#define PROBABLYCURSOR(BitsPixel, Planes) ((((BitsPixel) == 1) &&  \
                                                     ((Planes) == 1)) || \
                                            (BitsPixel) == 0 || (Planes) == 0)
#define BOGUSHANDLE(h) (~(h) & 0x4)

extern  UINT   cPendingCursorIconUpdates;

HANDLE W32CreateCursorIcon32(LPCURSORICONALIAS lpCIAlias);
HAND16 W32Create16BitCursorIcon(HAND16 hInst16, INT xHotSpot, INT yHotSpot,
         INT nWidth, INT nHeight, INT nPlanes, INT nBitsPixel,
         LPBYTE lpBitsAND, LPBYTE lpBitsXOR, INT   nBytesAND, INT nBytesXOR);

HANDLE GetCursorIconAlias32(HAND16 h16, UINT flType);
HAND16 GetCursorIconAlias16(HAND32 h32, UINT flType);
LPCURSORICONALIAS AllocCursorIconAlias();
LPCURSORICONALIAS FindCursorIconAlias(ULONG hCI, UINT flHandleSize);
BOOL DeleteCursorIconAlias(ULONG hCI, UINT flHandleSize);
BOOL FreeCursorIconAlias(HAND16 hand16, ULONG ulFLags);
HAND16 SetupCursorIconAlias(HAND16 hInst16, HAND32 h32, HAND16 h16, UINT flType,
                               LPBYTE lpResName, WORD hRes16);
HAND16 SetupResCursorIconAlias(HAND16 hInst16, HAND32 h32, LPBYTE lpResName, WORD hRes16, UINT flType);
ULONG SetCursorIconFlag(HAND16 h16, BOOL fSet);
BOOL  ReplaceCursorIcon(LPCURSORICONALIAS lpCIAliasIn);
BOOL FASTCALL  WK32WowCursorIconOp(PVDMFRAME pFrame);
VOID  UpdateCursorIcon(VOID);
HAND16 W32Create16BitCursorIconFrom32BitHandle(HANDLE h32, HAND16 hMod16,
                                                                 PUINT cbData);
BOOL  InitStdCursorIconAlias(VOID);

#if defined(FE_SB)
BOOL FindCursorIconAliasInUse(ULONG hCI);
#endif

#define HCURSOR32(hobj16)       GetCursorIconAlias32((HAND16)(hobj16), HANDLE_TYPE_CURSOR)
#define GETHCURSOR16(hobj32)    GetCursorIconAlias16((HAND32)(hobj32), HANDLE_TYPE_CURSOR)
#define FREEHCURSOR16(hobj16)   DeleteCursorIconAlias((ULONG)(hobj16), HANDLE_16BIT)

#define HICON32(hobj16)         GetCursorIconAlias32((HAND16)(hobj16), HANDLE_TYPE_ICON)
#define GETHICON16(hobj32)      GetCursorIconAlias16((HAND32)(hobj32), HANDLE_TYPE_ICON)
#define FREEHICON16(hobj16)     DeleteCursorIconAlias((ULONG)(hobj16), HANDLE_16BIT)

#define HICON32_REGCLASS(hobj16)  GetClassCursorIconAlias32((HAND16)(hobj16))
HANDLE GetClassCursorIconAlias32(HAND16 h16);
VOID InvalidateCursorIconAlias(LPCURSORICONALIAS lpT);

//
// In win32 USER
//

HANDLE WINAPI WOWLoadCursorIcon(HANDLE hInstance, LPCSTR lpIconName,
                                                 LPTSTR rt, LPHANDLE lphRes16);
HAND16 W32CheckIfAlreadyLoaded(VPVOID pData, WORD ResType);
