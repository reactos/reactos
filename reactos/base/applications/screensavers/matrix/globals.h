#ifndef GLOBALS_INCLUDED
#define GLOBALS_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define APPNAME	_T("MATRIX ScreenSaver 2.0")

#define DENSITY			24
#define DENSITY_MAX		50
#define DENSITY_MIN		5

// constants inferred from matrix.bmp
#define MAX_INTENSITY	5			// number of intensity levels
#define NUM_GLYPHS		26			// number of "glyphs" in each level
#define GLYPH_WIDTH		14			// width  of each glyph (pixels)
#define GLYPH_HEIGHT	14			// height of each glyph (pixels)

#define SPEED_MAX		10
#define SPEED_MIN		1

#define MAXMSG_WIDTH	0x100
#define MAXMSG_HEIGHT	0x100
#define MAXMSG_LENGTH	64

#define MSGSPEED_MAX	500
#define MSGSPEED_MIN	50

#define MAX_MESSAGES	16

#define FONT_MIN	8
#define FONT_MAX	30

extern TCHAR	g_szMessages[MAX_MESSAGES][MAXMSG_LENGTH];
extern int		g_nFontSize;
extern TCHAR	g_szFontName[];
extern BOOL		g_fFontBold;
extern int		g_nNumMessages;
extern int		g_nCurrentMessage;
extern int		g_nMessageSpeed;
extern int		g_nMatrixSpeed;
extern int		g_nDensity;
extern BOOL		g_fRandomizeMessages;
extern HFONT	g_hFont;
extern BOOL		g_fScreenSaving;

void LoadSettings();
void SaveSettings();

BOOL ChangePassword(HWND hwnd);
BOOL VerifyPassword(HWND hwnd);

BOOL Configure(HWND hwndParent);
BOOL ScreenSaver(HWND hwndParent);

int crc_rand();

#ifdef __cplusplus
}
#endif

#endif
