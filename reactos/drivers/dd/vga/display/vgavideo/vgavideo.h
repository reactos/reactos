// FIXME: Make these variables so we can also use modes like 800x600
#define SCREEN_X	640
#define SCREEN_Y	480

#define VGA_NORMAL 0
#define VGA_AND    8
#define VGA_OR     16
#define VGA_XOR    24

//This is in mingw standard headers
//typedef struct { int quot, rem; } div_t;

extern int maskbit[640];
extern int y80[480];
extern int xconv[640];
extern int bit8[640];
extern int startmasks[8];
extern int endmasks[8];

extern UCHAR PreCalcReverseByte[256];

extern char* vidmem;

#define MISC     0x3c2
#define SEQ      0x3c4
#define CRTC     0x3d4
#define GRAPHICS 0x3ce
#define FEATURE  0x3da
#define ATTRIB   0x3c0
#define STATUS   0x3da

typedef struct _VideoMode {
  unsigned short VidSeg;
  unsigned char  Misc;
  unsigned char  Feature;
  unsigned char  Seq[5];
  unsigned char  Crtc[25];
  unsigned char  Gfx[9];
  unsigned char  Attrib[21];
} VideoMode;

VOID vgaPreCalc();
VOID vgaPutPixel(INT x, INT y, UCHAR c);
VOID vgaPutByte(INT x, INT y, UCHAR c);
VOID vgaGetByte(ULONG offset,
                UCHAR *b, UCHAR *g,
                UCHAR *r, UCHAR *i);
INT vgaGetPixel(INT x, INT y);
BOOL vgaHLine(INT x, INT y, INT len, UCHAR c);
BOOL vgaVLine(INT x, INT y, INT len, UCHAR c);
INT abs(INT nm);
BOOL VGADDIIntersectRect(PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2);

#define SEQ_I   0x3C4           /* Sequencer Index */
#define SEQ_D   0x3C5           /* Sequencer Data Register */

#define GRA_I   0x3CE           /* Graphics Controller Index */
#define GRA_D   0x3CF           /* Graphics Controller Data Register */

#define LowByte(w)  (*((unsigned char *)&(w) + 0))
#define HighByte(w) (*((unsigned char *)&(w) + 1))

#define ASSIGNVP4(x, y, vp) vp = vidmem /* VBUF */ + (((x) + (y)*SCREEN_X) >> 3);
#define ASSIGNMK4(x, y, mask) mask = 0x80 >> ((x) & 7);

void get_masks(int x, int w);
