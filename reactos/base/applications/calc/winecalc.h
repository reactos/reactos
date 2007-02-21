/*
 * WineCalc (winecalc.h)
 *
 * Copyright 2003 James Briggs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

//////////////////////////////////////////////////////////////////

#ifdef UNICODE
#define CF_TTEXT CF_UNICODETEXT
#else
#define CF_TTEXT CF_TEXT
#endif

// numerics are defined here for easier porting

typedef double calcfloat;
#define FMT_DESC_FLOAT TEXT("%g")
#define FMT_DESC_EXP TEXT("%e")

#define CALC_ATOF(x) atof(x)

#define CONST_PI 3.1415926535897932384626433832795

/////////////////////////////////////////////////////////////////

#define CALC_BUF_SIZE 128

// statistics dialog dimensions

#define CALC_STA_X 235
#define CALC_STA_Y 180

// sentinel for differentiating Return from Ctrl+M events

#define NUMBER_OF_THE_BEAST 666

#define CALC_COLOR_BLUE    0
#define CALC_COLOR_RED     1
#define CALC_COLOR_GRAY    2
#define CALC_COLOR_MAGENTA 3

// gray hilite on rectangle owner-drawn controls RGB(CALC_GRAY, CALC_GRAY, CALC_GRAY)

#define CALC_GRAY 132

// count of buttons needing special toggle states depending on number base

#define TOGGLE_COUNT 23

// there are 3 window menus, standard, decimal measurement system and word size menus

#define COUNT_MENUS 3
#define MENU_STD    0
#define MENU_SCIMS  1
#define MENU_SCIWS  2

// count of buttons

#define CALC_BUTTONS_STANDARD   28
#define CALC_BUTTONS_SCIENTIFIC 73

// winecalc window outer dimensions

#define CALC_STANDARD_WIDTH    260
#define CALC_STANDARD_HEIGHT   252
#define CALC_SCIENTIFIC_WIDTH  480
#define CALC_SCIENTIFIC_HEIGHT 310

// winecalc private ids for events

#define ID_CALC_ZERO         0
#define ID_CALC_ONE          1
#define ID_CALC_TWO          2
#define ID_CALC_THREE        3
#define ID_CALC_FOUR         4
#define ID_CALC_FIVE         5
#define ID_CALC_SIX          6
#define ID_CALC_SEVEN        7
#define ID_CALC_EIGHT        8
#define ID_CALC_NINE         9
#define ID_CALC_BACKSPACE   20
#define ID_CALC_CLEAR_ENTRY 21
#define ID_CALC_CLEAR_ALL   22
#define ID_CALC_MEM_CLEAR   23
#define ID_CALC_DIVIDE      24
#define ID_CALC_SQRT        25
#define ID_CALC_MEM_RECALL  26
#define ID_CALC_MULTIPLY    27
#define ID_CALC_PERCENT     28
#define ID_CALC_MEM_STORE   29
#define ID_CALC_MINUS       30
#define ID_CALC_RECIPROCAL  31
#define ID_CALC_MEM_PLUS    32
#define ID_CALC_SIGN        33
#define ID_CALC_DECIMAL     34
#define ID_CALC_PLUS        35
#define ID_CALC_EQUALS      36
#define ID_CALC_STA         37
#define ID_CALC_FE          38
#define ID_CALC_LEFTPAREN   39
#define ID_CALC_RIGHTPAREN  40
#define ID_CALC_MOD         41
#define ID_CALC_AND         42
#define ID_CALC_OR          43
#define ID_CALC_XOR         44
#define ID_CALC_SUM         45
#define ID_CALC_SIN         46
#define ID_CALC_LOG10       47
#define ID_CALC_LSH         48
#define ID_CALC_NOT         49
#define ID_CALC_S           50
#define ID_CALC_COS         52
#define ID_CALC_FACTORIAL   53
#define ID_CALC_INT         54
#define ID_CALC_DAT         55
#define ID_CALC_TAN         56
#define ID_CALC_SQUARE      57
#define ID_CALC_A           58
#define ID_CALC_B           59
#define ID_CALC_C           60
#define ID_CALC_D           61
#define ID_CALC_E           62
#define ID_CALC_F           63
#define ID_CALC_AVE         64
#define ID_CALC_DMS         65
#define ID_CALC_EXP         66
#define ID_CALC_LN          67
#define ID_CALC_PI          68
#define ID_CALC_CUBE        69
#define ID_CALC_POWER       51

// Number System Radio Buttons

#define CALC_NS_COUNT         4
#define ID_CALC_NS_HEX     2000
#define ID_CALC_NS_DEC     2001
#define ID_CALC_NS_OCT     2002
#define ID_CALC_NS_BIN     2003

#define NBASE_DECIMAL         0
#define NBASE_BINARY          1
#define NBASE_OCTAL           2
#define NBASE_HEX             3

#define CALC_NS_OFFSET_X     15
#define CALC_NS_OFFSET_Y     37

#define SZ_RADIO_NS_X        50
#define SZ_RADIO_NS_Y        15

#define CALC_NS_HEX_LEFT      0
#define CALC_NS_HEX_TOP       0

#define CALC_NS_DEC_LEFT     50
#define CALC_NS_DEC_TOP       0

#define CALC_NS_OCT_LEFT     98
#define CALC_NS_OCT_TOP       0

#define CALC_NS_BIN_LEFT    148
#define CALC_NS_BIN_TOP       0

// Measurement System Radio Buttons

#define CALC_MS_COUNT         3
#define ID_CALC_MS_DEGREES 2010
#define ID_CALC_MS_RADIANS 2011
#define ID_CALC_MS_GRADS   2012

#define TRIGMODE_DEGREES      0
#define TRIGMODE_RADIANS      1
#define TRIGMODE_GRADS        2

#define CALC_MS_OFFSET_X    225
#define CALC_MS_OFFSET_Y     37

#define SZ_RADIO_MS_X        75
#define SZ_RADIO_MS_Y        15

#define CALC_MS_DEGREES_LEFT  0
#define CALC_MS_DEGREES_TOP   0

#define CALC_MS_RADIANS_LEFT 82
#define CALC_MS_RADIANS_TOP   0

#define CALC_MS_GRADS_LEFT  162
#define CALC_MS_GRADS_TOP     0

// Inv and Hyp Checkboxes

#define CALC_CB_COUNT         2
#define ID_CALC_CB_INV     2020
#define ID_CALC_CB_HYP     2021

#define WORDSIZE_BYTE         1
#define WORDSIZE_WORD         2
#define WORDSIZE_DWORD        4
#define WORDSIZE_QWORD        8

#define CALC_CB_OFFSET_X     15
#define CALC_CB_OFFSET_Y     58

#define CALC_CB_INV_LEFT      0
#define CALC_CB_INV_TOP      10

#define SZ_RADIO_CB_X        50
#define SZ_RADIO_CB_Y        14

#define CALC_CB_HYP_LEFT     58
#define CALC_CB_HYP_TOP      10

// Word Size Radio Buttons

#define CALC_WS_COUNT         4
#define ID_CALC_WS_QWORD   2030
#define ID_CALC_WS_DWORD   2031
#define ID_CALC_WS_WORD    2032
#define ID_CALC_WS_BYTE    2033

#define CALC_WS_OFFSET_X    CALC_MS_OFFSET_X
#define CALC_WS_OFFSET_Y    CALC_MS_OFFSET_Y

#define CALC_WS_QWORD_LEFT    0
#define CALC_WS_QWORD_TOP     0

#define CALC_WS_DWORD_LEFT   57
#define CALC_WS_DWORD_TOP     0

#define CALC_WS_WORD_LEFT   120
#define CALC_WS_WORD_TOP      0

#define CALC_WS_BYTE_LEFT   175
#define CALC_WS_BYTE_TOP      0

#define SZ_RADIO_WS_X        50
#define SZ_RADIO_WS_Y        15

// drawing offsets

#define CALC_EDIT_HEIGHT 20

#define SZ_FILLER_X     32
#define SZ_FILLER_Y     30

#define SZ_BIGBTN_X     62
#define SZ_BIGBTN_Y     30

#define SZ_MEDBTN_X     36
#define SZ_MEDBTN_Y     30

#define MARGIN_LEFT      7
#define MARGIN_SMALL_X   3
#define MARGIN_SMALL_Y   3
#define MARGIN_BIG_X   274
#define MARGIN_STANDARD_BIG_X    11
#define MARGIN_BIG_Y     6

#define SZ_SPACER_X              11

// winecalc results display area

#define WDISPLAY_STANDARD_LEFT        MARGIN_LEFT + 4
#define WDISPLAY_STANDARD_TOP         5
#define WDISPLAY_STANDARD_RIGHT     244
#define WDISPLAY_STANDARD_BOTTOM     24
#define CALC_STANDARD_MARGIN_TOP     16

#define WDISPLAY_SCIENTIFIC_LEFT      MARGIN_LEFT + 4
#define WDISPLAY_SCIENTIFIC_TOP       5
#define WDISPLAY_SCIENTIFIC_RIGHT   465
#define WDISPLAY_SCIENTIFIC_BOTTOM   24
#define CALC_SCIENTIFIC_MARGIN_TOP   44

typedef struct tagCalcBtn {
	int id;            // private id
	HWND hBtn;         // button child window handle
	TCHAR label[80];    // text on buttonface
	int color;         // text color
	RECT r;            // location
	int enable;        // 1 = control enbabled, 0 = disabled
}
CALCBTN;

typedef struct tagPos {
	int x;
	int y;
}
POS;

typedef struct tagCalc {
	HINSTANCE hInst;   // this HINSTANCE
	HWND hWnd;         // main window's HWND

	POS pos;
	int numButtons;    // standard = 28, scientific = more

	TCHAR buffer [CALC_BUF_SIZE];  // current keyboard buffer
	TCHAR display[CALC_BUF_SIZE]; // display buffer before output

	calcfloat value;   // most recent computer value
	calcfloat memory;  // most recent stored memory value from display buffer
	int paren;         // current parentheses level

	int sciMode;       // standard = 1, scientific = 0
	int displayMode;   // 0 = float, 1 = scientific exponential notation like 1.0e+10

	TCHAR oper;         // most recent operator pushed
	calcfloat operand; // most recent operand pushed
	int newenter;      // track multiple =
	int next;          // binary operation flag
	int err;           // errror status for divide by zero, infinity, etc.
	int init;          // starting buffer

	int digitGrouping; // no separators = 0, separators = 1

	int trigMode;      // degrees = 0, radians = 1, grads = 2
	int numBase;       // 10 = decimal, 2 = binary, 8 = octal, 16 = hex
	int wordSize;      // 1 = byte, 2 = word, 4 = dword, 8 = qword
	int invMode;       // INV mode 0 = off, 1 = on
	int hypMode;       // HYP mode 0 = off, 1 = on

	int new;           // first time 0 = false, 1 = true
	CALCBTN cb[80];    // enough buttons for standard or scientific mode
}
CALC;

BOOL CALLBACK AboutDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI MainProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void InitLuts(void);
void InitMenus(HINSTANCE hInst);
void DestroyMenus();

void InitCalc (CALC *calc);
void DestroyCalc (CALC *calc);

void calc_buffer_format(CALC *calc);
void calc_buffer_display(CALC *calc);
TCHAR *calc_sep(TCHAR *s);

void DrawCalcText (HDC hdc, HDC hMemDC, PAINTSTRUCT *ps, CALC *calc, int object, TCHAR *s);
void CalcRect (HDC hdc, HDC hMemDC, PAINTSTRUCT *ps, CALC *calc, int object);
void DrawCalcRectSci(HDC hdc, HDC hMemDC, PAINTSTRUCT  *ps, CALC *calc, RECT *r);
void DrawCalc (HDC hdc, HDC hMemDC, PAINTSTRUCT  *ps, CALC *calc);

void calc_setmenuitem_radio(HMENU hMenu, UINT id);

void show_debug(CALC *calc, TCHAR *title, long wParam, long lParam);

calcfloat calc_atof(const TCHAR *s, int base);
void calc_ftoa(CALC *calc, calcfloat r, TCHAR *buf);
long factorial(long n);

calcfloat calc_convert_to_radians(CALC *calc);
calcfloat calc_convert_from_radians(CALC *calc);

