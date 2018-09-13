//*****************************************************************************
//
// SUBCLASSING  -
//
//     Support for subclassing of 32bit standard (predefined) classes by
//     WOW apps.
//
//
// 01-10-92  NanduriR   Created.
//
//*****************************************************************************


typedef struct {
    DWORD Proc16;
    DWORD Proc32;
} THUNKWNDPROC, FAR *LPTHUNKWNDPROC;

DWORD GetStdClassThunkWindowProc(LPSTR lpstrClass, PWW pww, HANDLE h32);
DWORD IsStdClassThunkWindowProc(DWORD Proc16, PINT piClass);
DWORD GetStdClass32WindowProc( INT iWOWClass ) ;

#define THUNKWP_SIZE    0x30        /* Code size of a thunk */
#define THUNKWP_BLOCK   ((INT)(4096 / sizeof(TWPLIST)))   /* Number of thunks per block */

typedef struct _twpList {
    VPVOID      vpfn16;                 /* 16-bit proc address */
    VPVOID      vptwpNext;              /* Pointer to next proc in the list */
    HWND        hwnd32;                 /* 32-bit window handle */
    DWORD       dwMagic;                /* Magic identifier */
    INT         iClass;                 /* Class of original proc */
    DWORD       lpfn32;                 /* 32-bit proc address, 0 means available */
    BYTE        Code[THUNKWP_SIZE];     /* Code for the thunk */
} TWPLIST, *LPTWPLIST;

#define SUBCLASS_MAGIC  0x534C4353      /* "SCLS" Sub-Class magic value */

DWORD GetThunkWindowProc( DWORD Proc32, LPSTR lpszClass, PWW pww, HWND hwnd32 );
BOOL FreeThunkWindowProc( DWORD Proc16 );
void W32FreeThunkWindowProc( DWORD Proc32, DWORD Proc16 );
DWORD IsThunkWindowProc( DWORD Proc16, PINT piClass );
