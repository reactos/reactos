// FIXME: Make these variables so we can also use modes like 800x600
#define SCREEN_X	640
#define SCREEN_Y	480

#define VGA_NORMAL 0
#define VGA_AND    8
#define VGA_OR     16
#define VGA_XOR    24

typedef struct { int quot, rem; } div_t;

int maskbit[640], y80[480], xconv[640], bit8[640], startmasks[8], endmasks[8];

char* vidmem;

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

static unsigned char saved_SEQ_mask;	/* 0x02 */
static unsigned char saved_GC_eSR;	/* 0x01 */
static unsigned char saved_GC_fun;	/* 0x03 */
static unsigned char saved_GC_rmap;	/* 0x04 */
static unsigned char saved_GC_mode;	/* 0x05 */
static unsigned char saved_GC_mask;	/* 0x08 */
static unsigned char leftMask;
static int byteCounter;
static unsigned char rightMask;

void get_masks(int x, int w);
