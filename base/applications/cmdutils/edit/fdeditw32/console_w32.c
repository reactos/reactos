/* ----------- console.c ---------- */

#include <windows.h>

#include "keys.h"

#define MAXSAVES    50
#define swap(a,b)   a^=b; b^=a; a^=b;

/* ----- table of alt keys for finding shortcut keys ----- */
static const int altconvert[] = {
    ALT_A,ALT_B,ALT_C,ALT_D,ALT_E,ALT_F,ALT_G,ALT_H,
    ALT_I,ALT_J,ALT_K,ALT_L,ALT_M,ALT_N,ALT_O,ALT_P,
    ALT_Q,ALT_R,ALT_S,ALT_T,ALT_U,ALT_V,ALT_W,ALT_X,
    ALT_Y,ALT_Z,ALT_0,ALT_1,ALT_2,ALT_3,ALT_4,ALT_5,
    ALT_6,ALT_7,ALT_8,ALT_9
};

/*
 Set text and background colors
 */
static const WORD col_fore[] = {
/*
 dark colors
 */
    0,                  /* BLACK */
    FOREGROUND_BLUE,    /* BLUE */
    FOREGROUND_GREEN,   /* GREEN */
    FOREGROUND_BLUE |   /* CYAN */
    FOREGROUND_GREEN,
    FOREGROUND_RED,     /* RED */
    FOREGROUND_BLUE |   /* MAGENTA */
    FOREGROUND_RED,
    FOREGROUND_RED |    /* BROWN */
    FOREGROUND_GREEN,
    FOREGROUND_RED |    /* LIGHTGRAY */
    FOREGROUND_GREEN |
    FOREGROUND_BLUE,
/*
 light colors
 */
    FOREGROUND_INTENSITY,/* DARKGRAY */
    FOREGROUND_BLUE |   /* LIGHTBLUE */
    FOREGROUND_INTENSITY,
    FOREGROUND_GREEN |  /* LIGHTGREEN */
    FOREGROUND_INTENSITY,
    FOREGROUND_BLUE |   /* LIGHTCYAN */
    FOREGROUND_GREEN |
    FOREGROUND_INTENSITY,
    FOREGROUND_RED |    /* LIGHTRED */
    FOREGROUND_INTENSITY,
    FOREGROUND_BLUE |   /* MAGENTA */
    FOREGROUND_RED |
    FOREGROUND_INTENSITY,
    FOREGROUND_RED |    /* YELLOW */
    FOREGROUND_GREEN |
    FOREGROUND_INTENSITY,
    FOREGROUND_RED |    /* WHITE */
    FOREGROUND_GREEN |
    FOREGROUND_BLUE |
    FOREGROUND_INTENSITY,
};

static const WORD col_back[] = {
/*
 dark colors
 */
    0,                  /* BLACK */
    BACKGROUND_BLUE,    /* BLUE */
    BACKGROUND_GREEN,   /* GREEN */
    BACKGROUND_BLUE |   /* CYAN */
    BACKGROUND_GREEN,
    BACKGROUND_RED,     /* RED */
    BACKGROUND_BLUE |   /* MAGENTA */
    BACKGROUND_RED,
    BACKGROUND_RED |    /* BROWN */
    BACKGROUND_GREEN,
    BACKGROUND_RED |    /* LIGHTGRAY */
    BACKGROUND_GREEN |
    BACKGROUND_BLUE,
/*
 light colors
 */
    BACKGROUND_INTENSITY,/* DARKGRAY */
    BACKGROUND_BLUE |   /* LIGHTBLUE */
    BACKGROUND_INTENSITY,
    BACKGROUND_GREEN |  /* LIGHTGREEN */
    BACKGROUND_INTENSITY,
    BACKGROUND_BLUE |   /* LIGHTCYAN */
    BACKGROUND_GREEN |
    BACKGROUND_INTENSITY,
    BACKGROUND_RED |    /* LIGHTRED */
    BACKGROUND_INTENSITY,
    BACKGROUND_BLUE |   /* MAGENTA */
    BACKGROUND_RED |
    BACKGROUND_INTENSITY,
    BACKGROUND_RED |    /* YELLOW */
    BACKGROUND_GREEN |
    BACKGROUND_INTENSITY,
    BACKGROUND_RED |    /* WHITE */
    BACKGROUND_GREEN |
    BACKGROUND_BLUE |
    BACKGROUND_INTENSITY,
};

static int cursorpos[MAXSAVES];
static int cursorshape[MAXSAVES];
static int cs;

static HANDLE hConsoleIn;
static HANDLE hConsoleOut;
static HANDLE hNewScreenBuffer;
static CONSOLE_SCREEN_BUFFER_INFO csbi;

static SMALL_RECT updateRect;
static COORD      updateCoord;
static COORD      updateCoordSrc;
static CHAR_INFO *chiBuffer;
static unsigned char *coiBuffer;
static COORD      lastCurPos;
static BOOL       cursor_state;
static COORD      origCurPos;
static BOOL       initialized_vmode=FALSE;
static int        shift_mask;

static COORD    c_mouse;
static int      b_mouse;
static int      s_mouse;

void SwapCursorStack(void)
{
    if (cs > 1) {
        swap(cursorpos[cs-2], cursorpos[cs-1]);
        swap(cursorshape[cs-2], cursorshape[cs-1]);
    }
}

/* ---- Test for keystroke ---- */
typedef struct {
    unsigned int in_key;
    unsigned int out_key;
    unsigned int buf[32];
} keyb_t;

typedef struct {
    WORD vk;
    WORD kb;
    WORD alt_kb;
    WORD ctrl_kb;
} key_tbl_t;

static const key_tbl_t key_tbl[] = {
    { 'A', 'a', ALT_A, CTRL_A, },
    { 'B', 'b', ALT_B, CTRL_B, },
    { 'C', 'c', ALT_C, CTRL_C, },
    { 'D', 'd', ALT_D, CTRL_D, },
    { 'E', 'e', ALT_E, CTRL_E, },
    { 'F', 'f', ALT_F, CTRL_F, },
    { 'G', 'g', ALT_G, CTRL_G, },
    { 'H', 'h', ALT_H, CTRL_H, },
    { 'I', 'i', ALT_I, CTRL_I, },
    { 'J', 'j', ALT_J, CTRL_J, },
    { 'K', 'k', ALT_K, CTRL_K, },
    { 'L', 'l', ALT_L, CTRL_L, },
    { 'M', 'm', ALT_M, CTRL_M, },
    { 'N', 'n', ALT_N, CTRL_N, },
    { 'O', 'o', ALT_O, CTRL_O, },
    { 'P', 'p', ALT_P, CTRL_P, },
    { 'Q', 'q', ALT_Q, CTRL_Q, },
    { 'R', 'r', ALT_R, CTRL_R, },
    { 'S', 's', ALT_S, CTRL_S, },
    { 'T', 't', ALT_T, CTRL_T, },
    { 'U', 'u', ALT_U, CTRL_U, },
    { 'V', 'v', ALT_V, CTRL_V, },
    { 'W', 'w', ALT_W, CTRL_W, },
    { 'X', 'x', ALT_X, CTRL_X, },
    { 'Y', 'y', ALT_Y, CTRL_Y, },
    { 'Z', 'z', ALT_Z, CTRL_Z, },
};

#define VK_APIC     0xDE

static const key_tbl_t key_tbl3[] = {
    { VK_F1,  F1,  ALT_F1,  CTRL_F1, },
    { VK_F2,  F2,  ALT_F2,  CTRL_F2, },
    { VK_F3,  F3,  ALT_F3,  CTRL_F3, },
    { VK_F4,  F4,  ALT_F4,  CTRL_F4, },
    { VK_F5,  F5,  ALT_F5,  CTRL_F5, },
    { VK_F6,  F6,  ALT_F6,  CTRL_F6, },
    { VK_F7,  F7,  ALT_F7,  CTRL_F7, },
    { VK_F8,  F8,  ALT_F8,  CTRL_F8, },
    { VK_F9,  F9,  ALT_F9,  CTRL_F9, },
    { VK_F10, F10, ALT_F10, CTRL_F10, },
/* other keys */
    { VK_UP,     UP,   -1,        -1,        },
    { VK_DOWN,   DN,   -1,        -1,        },
    { VK_LEFT,   /*BS*/LARROW,   ALT_BS,    CTRL_BS,   },
    { VK_RIGHT,  FWD,  -1,        -1,/*CTRL_FWD,*/  },
    { VK_PRIOR,  PGUP, -1,        CTRL_PGUP, },
    { VK_NEXT,   PGDN, -1,        CTRL_PGDN, },
    { VK_HOME,   HOME, -1,        CTRL_HOME, },
    { VK_END,    END,  -1,        CTRL_END,  },
    { VK_INSERT, INS,  -1,        CTRL_INS,  },
    { VK_DELETE, DEL,  -1, /*ALT_DEL,*/   -1,        },
};

static keyb_t keyb = { 0, 0, { 0 } };

int w32_getkey()
{
    DWORD dwInputs;
    INPUT_RECORD inRec;
    unsigned int x;
    int          c;

    if (!GetNumberOfConsoleInputEvents(hConsoleIn, &dwInputs))
        return -1;
    if (!dwInputs)
        return -1;
    if (!ReadConsoleInput(hConsoleIn, &inRec, 1, &dwInputs))
        return -1;
    if (inRec.EventType == KEY_EVENT) {
        if (inRec.Event.KeyEvent.bKeyDown) {
            if (inRec.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
                shift_mask |= ALTKEY;
            if (inRec.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
                shift_mask |= CTRLKEY;
            if ((inRec.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED))
                shift_mask |= LEFTSHIFT;

            c = inRec.Event.KeyEvent.uChar.AsciiChar;
            if (!c) {
                c = -1;
/* special cases */

/* test for extended keys */
                for (x=0; x<sizeof(key_tbl3)/sizeof(key_tbl3[0]); x++) {
                    if (key_tbl3[x].vk == inRec.Event.KeyEvent.wVirtualKeyCode) {
                        if (inRec.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
                            c = key_tbl3[x].alt_kb;
                        else
                        if (inRec.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
                            c = key_tbl3[x].ctrl_kb;
                        else
                            c = key_tbl3[x].kb;
                        break;
                    }
                }
            } else {
                /* special case: Shift-TAB */
                if (c == 9 && (inRec.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED))
                    return SHIFT_HT;

                for (x=0; x<sizeof(key_tbl)/sizeof(key_tbl[0]); x++) {
                    if (key_tbl[x].vk == inRec.Event.KeyEvent.wVirtualKeyCode) {
                        if (inRec.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
                            c = key_tbl[x].alt_kb;
                        else
                        if (inRec.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
                            c = key_tbl[x].ctrl_kb;
                        break;
                    }
                }
            }
            return c;
        } else {
            if (!(inRec.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)))
                shift_mask &= ~ALTKEY;
            if (!(inRec.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)))
                shift_mask &= ~CTRLKEY;
            if (!(inRec.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED))
                shift_mask &= ~LEFTSHIFT;
        }
    }
    if (inRec.EventType == MOUSE_EVENT) {
        int old_mask = b_mouse;

        c_mouse = inRec.Event.MouseEvent.dwMousePosition;
        b_mouse = 0;
        if (inRec.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
            b_mouse |= 1;
        if (inRec.Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED)
            b_mouse |= 2;
        s_mouse |= old_mask ^ b_mouse;

        if (inRec.Event.MouseEvent.dwEventFlags & MOUSE_WHEELED) {
            int wheel = ( (int)inRec.Event.MouseEvent.dwButtonState ) >> 16;
            if (wheel < 0) return DN;
            if (wheel > 0) return UP;
        }

    }
    return -1;
}

void update_key_buffer(void)
{
    int c;

    while ((c=w32_getkey()) != -1) {
        keyb.buf[keyb.in_key] = c;
        keyb.in_key = (keyb.in_key+1) % (sizeof(keyb.buf)/sizeof(keyb.buf[0]));
    }
}

void clearBIOSbuffer(void)
{
/* not a good idea... */
/*
    keyb.in_key = keyb.out_key;
    FlushConsoleInputBuffer(hConsoleIn);
*/
}

BOOL keyhit(void)
{
    update_key_buffer();
    return (keyb.in_key != keyb.out_key);
}

/* ---- Read a keystroke ---- */
int getkey(void)
{
    int c;
    while (keyhit() == FALSE)
        Sleep(10);
    c = keyb.buf[keyb.out_key];
    keyb.out_key = (keyb.out_key+1) % (sizeof(keyb.buf)/sizeof(keyb.buf[0]));
    return c;
}

/* ---------- read the keyboard shift status --------- */
int getshift(void)
{
    return shift_mask;
}

/* -------- sound a buzz tone ---------- */
void beep(void)
{
}

/* -------- get the video mode and page from BIOS -------- */
void videomode(void)
{
/*
    regs.h.ah = 15;
    int86(VIDEO, &regs, &regs);
    video_mode = regs.h.al;
    video_page = regs.x.bx;
    video_page &= 0xff00;
    video_mode &= 0x7f;
*/
}

void init_videomode(void)
{
    CONSOLE_CURSOR_INFO curInfo;
    DWORD dwMode;
    int x,y;

    if (initialized_vmode)
        return;
    initialized_vmode = TRUE;

    hConsoleIn  = GetStdHandle(STD_INPUT_HANDLE);
    hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
    hNewScreenBuffer = CreateConsoleScreenBuffer( 
       GENERIC_READ |           /* read/write access */
       GENERIC_WRITE, 
       0,                       /* not shared */
       NULL,                    /* default security attributes */
       CONSOLE_TEXTMODE_BUFFER, /* must be TEXTMODE */
       NULL);

    GetConsoleScreenBufferInfo(hConsoleOut, &csbi);
    origCurPos = csbi.dwCursorPosition;
    SetConsoleActiveScreenBuffer(hNewScreenBuffer);
    updateCoord.X = csbi.dwSize.X;
/*    updateCoord.Y = csbi.dwSize.Y; */
    updateCoord.Y = 25;
    chiBuffer = (CHAR_INFO *)malloc(updateCoord.X*updateCoord.Y*sizeof(CHAR_INFO));
    coiBuffer = (unsigned char *)malloc(updateCoord.X*updateCoord.Y);
    updateCoordSrc.X = 0;
    updateCoordSrc.Y = 0;
    updateRect.Top = 0;
    updateRect.Left = 0;
    updateRect.Bottom = updateCoord.Y-1;
    updateRect.Right = updateCoord.X-1;

    for (y=0; y<updateCoord.Y; y++) {
        for (x=0; x<updateCoord.X; x++) {
            chiBuffer[x+y*updateCoord.X].Char.AsciiChar = ' ';
            chiBuffer[x+y*updateCoord.X].Attributes  = 0;
        }
    }

    WriteConsoleOutput( 
        hNewScreenBuffer,
        chiBuffer,
        updateCoord,
        updateCoordSrc,
        &updateRect);

    GetConsoleMode(hConsoleIn, &dwMode);
/*
 1) Without ENABLE_PROCESSED_INPUT we can use CTRL-C too.
 2) Sometimes, for some unknown reasons, WinXP didnt enable the mouse
    into the console, so its better to always enable it manually.
 */
    dwMode = (dwMode & ~ENABLE_PROCESSED_INPUT) | ENABLE_MOUSE_INPUT;
    SetConsoleMode(hConsoleIn, dwMode);

    lastCurPos.X = 0x7FFF;
    lastCurPos.Y = 0x7FFF;

    /* cursor is off by default into our screen buffer */
    cursor_state = FALSE;

    memset(&curInfo, 0, sizeof(CONSOLE_CURSOR_INFO));
    curInfo.dwSize = 1;
    curInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hNewScreenBuffer, &curInfo);

    /* mouse */
    c_mouse.X = 0;
    c_mouse.Y = 0;
    b_mouse = 0;
    s_mouse = 0;
}

/* ------ position the cursor ------ */
void cursor(int x, int y)
{
    COORD point;

    point.X = x; point.Y = y;
    if (memcmp(&point, &lastCurPos, sizeof(COORD))) {
        lastCurPos = point;
        SetConsoleCursorPosition(hNewScreenBuffer, point);
    }
}

/* ------- get the current cursor position ------- */
void curr_cursor(int *x, int *y)
{
    GetConsoleScreenBufferInfo(hNewScreenBuffer, &csbi);
    *x = csbi.dwCursorPosition.X;
    *y = csbi.dwCursorPosition.Y;
}

/* ------ save the current cursor configuration ------ */
void savecursor(void)
{
    if (cs < MAXSAVES)    {
/*
        getcursor();
        cursorshape[cs] = regs.x.cx;
        cursorpos[cs] = regs.x.dx;
*/
        cs++;
    }
}

/* ---- restore the saved cursor configuration ---- */
void restorecursor(void)
{
    if (cs)    {
        --cs;
/*
        videomode();
        regs.x.dx = cursorpos[cs];
        regs.h.ah = SETCURSOR;
        regs.x.bx = video_page;
        int86(VIDEO, &regs, &regs);
        set_cursor_type(cursorshape[cs]);
*/
    }
}

/* ------ make a normal cursor ------ */
void normalcursor(void)
{
/*    set_cursor_type(0x0607); */
}

/* ------ hide the cursor ------ */
void hidecursor(void)
{
    CONSOLE_CURSOR_INFO curInfo;

    if (cursor_state == TRUE) {
        cursor_state = FALSE;
        memset(&curInfo, 0, sizeof(CONSOLE_CURSOR_INFO));
        curInfo.dwSize = 1;
        curInfo.bVisible = FALSE;
        SetConsoleCursorInfo(hNewScreenBuffer, &curInfo);
    }
}

/* ------ unhide the cursor ------ */
void unhidecursor(void)
{
    CONSOLE_CURSOR_INFO curInfo;

    if (cursor_state == FALSE) {
        cursor_state = TRUE;
        memset(&curInfo, 0, sizeof(CONSOLE_CURSOR_INFO));
        curInfo.dwSize = 1;
        curInfo.bVisible = TRUE;
        SetConsoleCursorInfo(hNewScreenBuffer, &curInfo);
    }
}

/* ---- use BIOS to set the cursor type ---- */
void set_cursor_type(unsigned t)
{
/*
    videomode();
    regs.h.ah = SETCURSORTYPE;
    regs.x.bx = video_page;
    regs.x.cx = t;
    int86(VIDEO, &regs, &regs);
*/
}

/* ---- test for EGA -------- */
BOOL isEGA(void)
{
    return 0;
}

/* ---- test for VGA -------- */
BOOL isVGA(void)
{
    return 1;
}

/* ---------- set 25 line mode ------- */
void Set25(void)
{
}

/* ---------- set 43 line mode ------- */
void Set43(void)
{
}

/* ---------- set 50 line mode ------- */
void Set50(void)
{
}

/* ------ convert an Alt+ key to its letter equivalent ----- */
int AltConvert(int c)
{
    int i, a = 0;
    for (i = 0; i < 36; i++)
        if (c == altconvert[i])
            break;
    if (i < 26)
        a = 'a' + i;
    else if (i < 36)
        a = '0' + i - 26;
    return a;
}

void w32_put_video(int x, int y, int ch_att)
{
    SMALL_RECT srctRect;
    COORD coordBufCoord;

    chiBuffer[x+y*updateCoord.X].Char.AsciiChar = ch_att & 0xFF;
    ch_att >>= 8;
    coiBuffer[x+y*updateCoord.X] = ch_att;
    chiBuffer[x+y*updateCoord.X].Attributes  = col_fore[ch_att & 0xF] |
                                               col_back[ch_att >> 4];
    srctRect.Top = y;
    srctRect.Left = x;
    srctRect.Bottom = y;
    srctRect.Right = x;
 
    coordBufCoord.X = x; 
    coordBufCoord.Y = y;

    WriteConsoleOutput( 
        hNewScreenBuffer,
        chiBuffer,
        updateCoord,
        coordBufCoord,
        &srctRect);
}

int w32_get_video(int x, int y)
{
    int n = x+y*updateCoord.X;
    return (chiBuffer[n].Char.AsciiChar & 0xFF) | (coiBuffer[n] << 8);
}

void w32_scroll_up(int x1,int y1,int x2,int y2,int attr)
{
    int x,y,addr1,addr2,ch_att;
    SMALL_RECT srctRect;
    COORD coordBufCoord;

    ch_att = col_fore[attr & 0xF] | col_back[attr >> 4];
    addr1 = y1*updateCoord.X;
    addr2 = addr1+updateCoord.X;
    for (y=y1+1; y<=y2; y++,addr1=addr2,addr2+=updateCoord.X) {
        for (x=x1; x<=x2; x++) {
           chiBuffer[addr1+x].Char = chiBuffer[addr2+x].Char;
           chiBuffer[addr1+x].Attributes = chiBuffer[addr2+x].Attributes;
           coiBuffer[addr1+x] = coiBuffer[addr2+x];
        }
    }
    for (x=x1; x<=x2; x++) {
       chiBuffer[addr1+x].Char.AsciiChar = ' ';
       chiBuffer[addr1+x].Attributes = ch_att;
       coiBuffer[addr1+x] = attr;
    }

    srctRect.Top = y1;
    srctRect.Left = x1;
    srctRect.Bottom = y2;
    srctRect.Right = x2;
 
    coordBufCoord.X = x1; 
    coordBufCoord.Y = y1;

    WriteConsoleOutput( 
        hNewScreenBuffer,
        chiBuffer,
        updateCoord,
        coordBufCoord,
        &srctRect);
}

void w32_scroll_dw(int x1,int y1,int x2,int y2,int attr)
{
    int x,y,addr1,addr2,ch_att;
    SMALL_RECT srctRect;
    COORD coordBufCoord;

    ch_att = col_fore[attr & 0xF] | col_back[attr >> 4];
    addr1 = y2*updateCoord.X;
    addr2 = addr1-updateCoord.X;
    for (y=y2-1; y>=y1; y--,addr1=addr2,addr2-=updateCoord.X) {
        for (x=x1; x<=x2; x++) {
           chiBuffer[addr1+x].Char = chiBuffer[addr2+x].Char;
           chiBuffer[addr1+x].Attributes = chiBuffer[addr2+x].Attributes;
           coiBuffer[addr1+x] = coiBuffer[addr2+x];
        }
    }
    for (x=x1; x<=x2; x++) {
       chiBuffer[addr1+x].Char.AsciiChar = ' ';
       chiBuffer[addr1+x].Attributes = ch_att;
       coiBuffer[addr1+x] = attr;
    }

    srctRect.Top = y1;
    srctRect.Left = x1;
    srctRect.Bottom = y2;
    srctRect.Right = x2;
 
    coordBufCoord.X = x1; 
    coordBufCoord.Y = y1;

    WriteConsoleOutput( 
        hNewScreenBuffer,
        chiBuffer,
        updateCoord,
        coordBufCoord,
        &srctRect);
}

void movetoscreen(void *bf, int offset, int len)
{
    int x;
    unsigned char *cbf = (unsigned char *)bf;
    SMALL_RECT srctRect;
    COORD coordBufCoord;

    len >>= 1;
    offset >>= 1;
    for (x=0; x<len; x++,cbf+=2) {
       chiBuffer[offset+x].Char.AsciiChar = cbf[0];
       chiBuffer[offset+x].Attributes = col_fore[cbf[1] & 0xF] | col_back[cbf[1] >> 4];
       coiBuffer[offset+x] = cbf[1];
    }
    srctRect.Top = offset/updateCoord.X;
    srctRect.Left = offset%updateCoord.X;
    srctRect.Bottom = (offset+len-1)/updateCoord.X;
    srctRect.Right = (offset+len-1)%updateCoord.X;
 
    coordBufCoord.X = srctRect.Left; 
    coordBufCoord.Y = srctRect.Top;

    WriteConsoleOutput( 
        hNewScreenBuffer,
        chiBuffer,
        updateCoord,
        coordBufCoord,
        &srctRect);
}

void movefromscreen(void *bf, int offset, int len)
{
    int x;
    unsigned char *cbf = (unsigned char *)bf;

    len >>= 1;
    offset >>= 1;
    for (x=0; x<len; x++, cbf+=2) {
       cbf[0] = chiBuffer[offset+x].Char.AsciiChar;
       cbf[1] = coiBuffer[offset+x];
    }
}

int w32_screenwidth()
{
    return updateCoord.X;
}

int w32_screenheight()
{
    return updateCoord.Y;
}

void uninit_videomode(void)
{
    free(chiBuffer);
    free(coiBuffer);
    SetConsoleActiveScreenBuffer(hConsoleOut);
    CloseHandle(hNewScreenBuffer);
}

void w32_trap_to_sched(void)
{
    Sleep(10);
}

/* ---------- reset the mouse ---------- */
void resetmouse(void)
{
}

/* ----- test to see if the mouse driver is installed ----- */
BOOL mouse_installed(void)
{
    return 1;
}

/* ------ return true if mouse buttons are pressed ------- */
int mousebuttons(void)
{
    update_key_buffer();
    return b_mouse;
}

/* ---------- return mouse coordinates ---------- */
void get_mouseposition(int *x, int *y)
{
    update_key_buffer();
    *x = c_mouse.X;
    *y = c_mouse.Y;
}

/* -------- position the mouse cursor -------- */
void set_mouseposition(int x, int y)
{
}

/* --------- display the mouse cursor -------- */
void show_mousecursor(void)
{
}

/* --------- hide the mouse cursor ------- */
void hide_mousecursor(void)
{
}

/* --- return true if a mouse button has been released --- */
int button_releases(void)
{
    int ret;
    update_key_buffer();
    ret = s_mouse & ~b_mouse;
    s_mouse = 0;
    return ret;
}

/* ----- set mouse travel limits ------- */
void set_mousetravel(int minx, int maxx, int miny, int maxy)
{
    minx=maxx=miny=maxy=0;
}
