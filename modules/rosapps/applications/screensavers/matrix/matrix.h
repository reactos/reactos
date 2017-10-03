#ifndef MATRIX_INCLUDED
#define MATRIX_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//
// Top BYTE of each glyph is used as flags
// (redraw state, intensity etc)
//
// Bottom BYTE of each glyph is the character value
//
// Bit:  15     14     13-8     |  7-0
//
//    [Redraw][Blank][Intensity] [Glyph]
//
typedef unsigned short GLYPH;

#define GLYPH_REDRAW 0x8000
#define GLYPH_BLANK  0x4000

//
//	The "matrix" is basically an array of these
//  column structures, positioned side-by-side
//
typedef struct
{
	BOOL	state;
	int		countdown;

	BOOL	started;
	int		runlen;

	int		blippos;
	int		bliplen;

	int		length;
	GLYPH	*glyph;

} MATRIX_COLUMN;

typedef struct
{
	int				width;
	int				height;
	int				numcols;
	int				numrows;

	// bitmap containing glyphs.
	HDC				hdcBitmap;
	HBITMAP			hbmBitmap;

	MATRIX_MESSAGE *message;

	MATRIX_COLUMN	column[1];

} MATRIX;

GLYPH RandomGlyph(int intensity);
void  DrawGlyph(MATRIX *matrix, HDC hdc, int xpos, int ypos, GLYPH glyph);

HWND CreateScreenSaveWnd(HWND hwndParent, RECT *rect);
void InitScreenSaveClass();

#ifdef __cplusplus
}
#endif

#endif
