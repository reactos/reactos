/****************************************************************************
 *                                                                          *
 *  FILE        : SHOWDIB.H                                                 *
 *                                                                          *
 *  DESCRIPTION : Header/include file for ShowDIB example.                  *
 *                                                                          *
 ****************************************************************************/



// Macro to restrict a given value to an upper or lower boundary value
#define BOUND(x,min,max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))


// Macro to swap two values
#define SWAP(x,y)   ((x)^=(y)^=(x)^=(y))


// Macro to find the minimum of two values
#define MIN(x,y) (((x) <= (y)) : x ? y)


// Macros to display/remove hourglass cursor for lengthy operations
#define StartWait() hcurSave = SetCursor(LoadCursor(NULL,IDC_WAIT))
#define EndWait()   SetCursor(hcurSave)



#define MINBAND         50     // Minimum band size used by the program
#define BANDINCREMENT   20     // Decrement for band size while trying
                               // to determine optimum band size.


#define ISDIB(bft)      ((bft) == BFT_BITMAP)   // macro to determine if resource is a DIB


#define ALIGNULONG(i)   ((i+3)/4*4)     // Align to the closest DWORD (unsigned long )


#define WIDTHBYTES(i)   ((i+31)/32*4)   // Round off to the closest byte


#define PALVERSION      0x300
#define MAXPALETTE      256             // max. # supported palette entries

/***************** GLOBAL VARIABLES *************************/

extern  char        achFileName[128];   // File pathname
extern  DWORD       dwOffset;           // Current position if DIB file pointer
extern  RECT        rcClip;             // Current clip rectangle.
extern  BOOL        fPalColors;         // TRUE if the current DIB's color table
                                        // contains palette indexes not rgb values
extern  BOOL        bDIBToDevice;       // Use SetDIBitsToDevice() to BLT data.
extern  BOOL        bLegitDraw;         // We have a valid bitmap to draw
extern  WORD        wTransparent;       // Mode of DC
extern  HPALETTE    hpalCurrent;        // Handle to current palette
extern  HANDLE      hdibCurrent;        // Handle to current memory DIB
extern  HBITMAP     hbmCurrent;         // Handle to current memory BITMAP
extern  HANDLE      hbiCurrent;         // Handle to current bitmap info struct
extern  DWORD       dwStyle;            // Style bits of the App. window




/***********************************************************/
/* Declarations of functions used in dib.c module          */
/***********************************************************/

WORD            PaletteSize (VOID FAR * pv);
WORD            DibNumColors (VOID FAR * pv);
HANDLE          DibFromBitmap (HBITMAP hbm, DWORD biStyle, WORD biBits, HPALETTE hpal);
HBITMAP         BitmapFromDib (HANDLE hdib, HPALETTE hpal);
