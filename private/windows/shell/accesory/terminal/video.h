/*****************************************************************************/
/* Constant Definitions                                                      */
/*****************************************************************************/

#define VID_CALCTEXT             0x8000      /* mbbx 1.03 ... */
#define VID_CALCBKGD             0x4000
#define VID_CALCATTR             0x2000

#define VID_BOLD                 0x0001
#define VID_REVERSE              0x0002
#define VID_ITALIC               0x0004
#define VID_UNDERLINE            0x0008
#define VID_STRIKEOUT            0x0010
#define VID_MASK                 (VID_BOLD | VID_REVERSE | VID_ITALIC | VID_UNDERLINE | VID_STRIKEOUT)

#define VID_RED                  0
#define VID_GREEN                1
#define VID_BLUE                 2

#define VID_MAXFONTCACHE         6           /* mbbx 1.04: per jtfx 1.1 ... */

#define VID_DRAW_TOP             0x01        /* mbbx 1.04: per jtfx 1.1 ... */
#define VID_DRAW_BOTTOM          0x02
#define VID_DRAW_LEFT            0x04
#define VID_DRAW_RIGHT           0x08
#define VID_DRAW_SCAN1           0x10
#define VID_DRAW_SCAN3           0x20
#define VID_DRAW_SCAN7           0x40
#define VID_DRAW_SCAN9           0x80


/*****************************************************************************/
/* Variable Declarations                                                     */
/*****************************************************************************/

struct                                       /* mbbx 1.03 ... */
{
   BYTE  text[3];
   BYTE  bkgd[3];
   WORD  flags;
} vidAttr[32];


struct                                       /* mbbx 1.04: per jtfx 1.1 ... */
{
   HANDLE   hFont;
   WORD     flags;
} vidFontCache[VID_MAXFONTCACHE];


struct
{
   BYTE  buffer;
   BYTE  display;
} vidGraphChars[64];


INT   vidCharWidths[256];


