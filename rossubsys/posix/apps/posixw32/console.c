/* console.c
 *
 * AUTHOR:  John L. Miller, johnmil@cs.cmu.edu / johnmil@jprc.com
 * DATE:    8/4/96
 *
 * Copyright (c) 1996 John L. Miller
 *
 * Full permission is granted to use, modify and distribute
 * this code, provided:
 * 1) This comment field is included in its entirity
 * 2) No money is charged for any work including or based on
 *    portions of this code.
 *
 * If you're a nice person and find this useful, I'd appreciate a
 * note letting me know about it. e-mail is usually what spurs me
 * on to improve and support software I've written.
 *
 * This file contains functions intended to provide the back
 * end to a console window for my semi-vt100 emulator.
 */

/* Note - one HUGE difference between console windows and terminal
 * windows. Console windows displays start at (0,0). Terminal displays
 * start at (1,1). YUCK!
 */

#include <windows.h>
#include "vt100.h"

int topScrollRow=TOP_EDGE;
int bottomScrollRow=BOTTOM_EDGE;

/* This variable will contain terminal configuration flags, such as 
 * reverse/standard video, whether wrapping is enabled, and so on.
 */
int conTermMode;

/* Variable to hold the cursor position for save/restore cursor calls */
COORD cursorPosSave={1,1};

/* Handles to the current console for input and output */
HANDLE hConIn, hConOut;

/* Array of all the tabs which are currently set. Ironically, I think the
 * primary emulator can CLEAR tags, but not set them.
 */
int tabSet[132]={0};
int numTabs = 0;


/* This section contains console-specific color information. NT consoles can
 * have Red, blue, green, and intensity flags set. Hence, 4 con_colors.
 */
#define NUM_CON_COLORS 4

/* Foreground and background colors are separated out */
int conForeColors, conBackColors;

/* mapping between foreground and background console colors: needed
 * when reverse video is being used 
 */
int conColorMapping[NUM_CON_COLORS][2] = 
{
    {FOREGROUND_RED, BACKGROUND_RED},
    {FOREGROUND_BLUE, BACKGROUND_BLUE},
    {FOREGROUND_GREEN, BACKGROUND_GREEN},
    {FOREGROUND_INTENSITY, BACKGROUND_INTENSITY}
};


/* Device-independant foreground and background flags stored here.
 * probably a bad division of labor, but hey, since we don't use
 * all of their flags in our console stuff (and hence can't retrieve
 * them), the information has to live SOMEWHERE.
 */

int scForeFlags, scBackFlags;

/* Defines for array indexing for translation of flags */
#define SC_FLAG      0
#define CONSOLE_FLAG 1

/* Color mapping between SC (the vt-100 emulator device independant
 * flags) and NT console character specific flags. Flags which have no analog
 * are set to 0. Note that all global character attributes (character set
 * underline, bold, reverse) are all stored in foreground only 
 */
const int scForeMapping[NUM_SC_ATTRIBUTES][2] =
{
    {SC_RED,FOREGROUND_RED},
    {SC_GREEN,FOREGROUND_GREEN},
    {SC_BLUE,FOREGROUND_BLUE},
    {SC_BOLD,FOREGROUND_INTENSITY},
    {SC_UL,0},
    {SC_BL,0},
    {SC_RV,0},
    {SC_ASCII,0},
    {SC_G0,0},
    {SC_G1,0},
    {SC_GRAPHICS,0}
};

/* Background color mapping between SC and console */
const int scBackMapping[NUM_SC_ATTRIBUTES][2] =
{
    {SC_RED,BACKGROUND_RED},
    {SC_GREEN,BACKGROUND_GREEN},
    {SC_BLUE,BACKGROUND_BLUE},
    {SC_BOLD,BACKGROUND_INTENSITY},
    {SC_UL,0},
    {SC_BL,0},
    {SC_RV,0},
    {SC_ASCII,0},
    {SC_G0,0},
    {SC_G1,0},
    {SC_GRAPHICS,0}
};

/* These arrays map character vals 0-255 to new values.
 * Since the G0 and G1 character sets don't have a direct analog in
 * NT, I'm settling for replacing the ones I know what to set them
 * to.
 */
char G0Chars[256];
char G1Chars[256];

/* These four sets of variables are just precomputed combinations of
 * all the possible flags to save time for masking.
 */
int allFore[2], allBack[2];
int bothFore[2], bothBack[2];


/* FORWARD DECLARATIONS */
int
RawPrintLine(
    char *text,
    int len,
    int scrollAtEnd
    );

int
Scroll(
    int row
    );
/* END FORWARD DECLARATIONS */



/* beInitVT100Terminal() - 
 *
 * This function is called by the VT100 emulator as soon as the 
 * front-end terminal is initialized. It's responsible for setting
 * initial state of the terminal, and initing our many wacky variables.
 */

int
beInitVT100Terminal()
{
    int i;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    /* Set tabs to every 8 spaces initially */
    numTabs = 0;
    for (numTabs=0; numTabs < 132/8; numTabs++)
        tabSet[numTabs] = (numTabs+1)*8;

    /* Init the cursor save position to HOME */
    cursorPosSave.X = 1;
    cursorPosSave.Y = 1;

    /* Disable scrolling window limits */
    topScrollRow=TOP_EDGE;
    bottomScrollRow=BOTTOM_EDGE;

    conTermMode = ANSI_MODE|WRAP_MODE|REPEAT_MODE;

    hConIn  = GetStdHandle(STD_INPUT_HANDLE);
    hConOut = GetStdHandle(STD_OUTPUT_HANDLE);

    /* Init our time-saving mask variables */
    allFore[SC_FLAG]       = allBack[SC_FLAG]       = 0;
    allFore[CONSOLE_FLAG]  = allBack[CONSOLE_FLAG]  = 0;
    bothFore[SC_FLAG]      = bothBack[SC_FLAG]      = 0;
    bothFore[CONSOLE_FLAG] = bothBack[CONSOLE_FLAG] = 0;

    for (i=0; i<NUM_SC_ATTRIBUTES; i++)
    {
        allFore[SC_FLAG]      |= scForeMapping[i][SC_FLAG];
        allFore[CONSOLE_FLAG] |= scForeMapping[i][CONSOLE_FLAG];
        allBack[SC_FLAG]      |= scBackMapping[i][SC_FLAG];
        allBack[CONSOLE_FLAG] |= scBackMapping[i][CONSOLE_FLAG];

        if (scForeMapping[i][SC_FLAG] && scForeMapping[i][CONSOLE_FLAG])
        {
            bothFore[SC_FLAG]      |= scForeMapping[i][SC_FLAG];
            bothFore[CONSOLE_FLAG] |= scForeMapping[i][CONSOLE_FLAG];
        }

        if (scBackMapping[i][SC_FLAG] && scBackMapping[i][CONSOLE_FLAG])
        {
            bothBack[SC_FLAG]      |= scBackMapping[i][SC_FLAG];
            bothBack[CONSOLE_FLAG] |= scBackMapping[i][CONSOLE_FLAG];
        }
    }

    conForeColors = conBackColors = 0;

    for (i=0; i<NUM_CON_COLORS; i++)
    {
        conForeColors |= conColorMapping[i][0];
        conBackColors |= conColorMapping[i][1];
    }


    /* Do initial settings for device-independant flags */
    scForeFlags = SC_ASCII;
    scBackFlags = 0;

    GetConsoleScreenBufferInfo(hConOut, &csbi);

    for (i=0; i<NUM_SC_ATTRIBUTES; i++)
    {
        if (csbi.wAttributes & scForeMapping[i][CONSOLE_FLAG])
            scForeFlags |= scForeMapping[i][SC_FLAG];

        if (csbi.wAttributes & scBackMapping[i][CONSOLE_FLAG])
            scBackFlags |= scBackMapping[i][SC_FLAG];
    }


    /* Since the G0/G1 character sets don't really map to
     * USASCII, So come as close as we can. By default, it'll
     * just print the ascii character. For the graphics characters
     * I was able to identify, change that mapping.
     */

    for (i=0; i<256; i++)
    {
        G0Chars[i] = i;
        G1Chars[i] = i;
    }

    G1Chars['a']=(char)177;
    G1Chars['f']=(char)248;
    G1Chars['g']=(char)241;
    G1Chars['j']=(char)217;
    G1Chars['k']=(char)191;
    G1Chars['l']=(char)218;
    G1Chars['m']=(char)192;
    G1Chars['n']=(char)197;
    G1Chars['o']=(char)196;
    G1Chars['p']=(char)196;
    G1Chars['q']=(char)196;
    G1Chars['r']=(char)196;
    G1Chars['s']=(char)196;
    G1Chars['t']=(char)195;
    G1Chars['u']=(char)180;
    G1Chars['v']=(char)193;
    G1Chars['w']=(char)194;
    G1Chars['x']=(char)179;
    G1Chars['y']=(char)243;
    G1Chars['z']=(char)242;

    return(0);
}



/* beAbsoluteCursor -
 *
 * Given an input row and column, move the cursor to the 
 * absolute screen coordinates requested. Note that if the
 * display window has scrollbars, the column is adjusted
 * to take that into account, but the row is not. This allows
 * for large scrollback in terminal windows.
 *
 * ROW must be able to accept CUR_ROW, TOP_EDGE, BOTTOM_EDGE,
 * or a row number.
 *
 * COLUMN must be able to accept CUR_COL, LEFT_EDGE, RIGHT_EDGE,
 * or a column number.
 */

int
beAbsoluteCursor(
    int row,
    int col
    )
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD cursorPos;

    GetConsoleScreenBufferInfo(hConOut, &csbi);

    if (row == CUR_ROW)
        row = csbi.dwCursorPosition.Y;
    else if (row == TOP_EDGE)
        row = csbi.srWindow.Top;
    else if (row == BOTTOM_EDGE)
        row = csbi.srWindow.Bottom;
    else
        row += csbi.srWindow.Top - 1;

    if (col == CUR_COL)
        col = csbi.dwCursorPosition.X;
    else if (col == LEFT_EDGE)
        col = 0;
    else if (col == RIGHT_EDGE)
        col = csbi.dwSize.X-1;
    else
        col -= 1;

    cursorPos.X = col;
    cursorPos.Y = row;

    SetConsoleCursorPosition(hConOut, cursorPos);

    return(0);
}


/* beOffsetCursor -
 * 
 * Given an input row and column offset, move the cursor by that
 * many positions. For instance, row=0 and column=-1 would move
 * the cursor left a single column.
 *
 * If the cursor can't move the requested amount, results are 
 * unpredictable.
 */

int
beOffsetCursor(
    int row,
    int column
    )
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD cursorPos;

    GetConsoleScreenBufferInfo(hConOut, &csbi);

    cursorPos = csbi.dwCursorPosition;
    cursorPos.X += column;
    cursorPos.Y += row;

    if (cursorPos.X < 0)
        cursorPos.X = 0;

    if (cursorPos.X >= csbi.dwSize.X)
    {
        cursorPos.X -= csbi.dwSize.X;
        cursorPos.Y += 1;
    }

    if (cursorPos.Y < 0)
        cursorPos.Y = 0;

    SetConsoleCursorPosition(hConOut, cursorPos);

    return(0);
}


/* beRestoreCursor -
 * 
 * Saved cursor position should be stored in a static 
 * variable in the back end. This function restores the 
 * cursor to the position stored in that variable.
 */

int
beRestoreCursor(void)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD cursorPos;

    GetConsoleScreenBufferInfo(hConOut, &csbi);

    cursorPos = csbi.dwCursorPosition;

    cursorPos.Y += cursorPosSave.Y;

    SetConsoleCursorPosition(hConOut, cursorPos);

    return(0);
}

/* beSaveCursor -
 *
 * The back-end should maintain a static variable with the
 * last STORED cursor position in it. This function replaces
 * the contents of that variable with the current cursor position.
 * The cursor may be restored to this position by using the
 * beRestoreCursor function.
 */

int
beSaveCursor(void)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(hConOut, &csbi);
    cursorPosSave = csbi.dwCursorPosition;

    cursorPosSave.Y -= csbi.srWindow.Top;

    return(0);
}


/* beGetTextAttributes -
 *
 * given a pointer to 'fore'ground and 'back'ground ints,
 * fill them with a device-independant description of the
 * current foreground and background colors, as well as any
 * font information in the foreground variable.
 */

int
beGetTextAttributes(
    int *fore,
    int *back
    )
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int i;

    /* Since it's entirely possible that the text attributes were
     * changed without our terminal being notified, we might as well
     * make sure they're accurate.
     */

    /* First, strip out everything in the screen buffer variables
     * that we can detect
     */

    scForeFlags &= ~bothFore[SC_FLAG];
    scBackFlags &= ~bothBack[SC_FLAG];

    /* Now, find out what the real settings are, and set the
     * flag values accordingly.
     */

    GetConsoleScreenBufferInfo(hConOut, &csbi);


    /* If reverse video is set, we need to reverse our color mappings
     * before any calculations get made.
     */

    if (scForeFlags & SC_RV)
    {
        int tmpFore, tmpBack;

        tmpFore = csbi.wAttributes & conForeColors;
        tmpBack = csbi.wAttributes & conBackColors;

        csbi.wAttributes &= ~(conForeColors | conBackColors);

        for (i=0; i<NUM_CON_COLORS; i++)
        {
            if (tmpFore & conColorMapping[i][0])
                csbi.wAttributes |= conColorMapping[i][1];

            if (tmpBack & conColorMapping[i][1])
                csbi.wAttributes |= conColorMapping[i][0];
        }
    }

    /* Now, do the actual translation between our detectable
     * console text attributes and the corresponding device-independant 
     * attributes.
     */

    for (i=0; i<NUM_SC_ATTRIBUTES; i++)
    {
        if (csbi.wAttributes & scForeMapping[i][CONSOLE_FLAG])
            scForeFlags |= scForeMapping[i][SC_FLAG];

        if (csbi.wAttributes & scBackMapping[i][CONSOLE_FLAG])
            scBackFlags |= scBackMapping[i][SC_FLAG];
    }

    /* Finally, copy our updated sc flags into the variables
     * passed in
     */

    if (fore != NULL)
        *fore = scForeFlags;

    if (back != NULL)
        *back = scBackFlags;

    return(0);
}


/* beSetTextAttributes -
 *
 * Given a foreground and a background device independant (SC) color and font
 * specification, apply these to the display, and save the state in the 
 * static screen variables.
 *
 * Note that many font-specific constants (bold/underline/reverse, G0/G1/ASCII)
 * are stored ONLY in the foreground specification.
 */

int
beSetTextAttributes(
    int fore,
    int back
    )
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int i;
    WORD wAttributes;

    /* First off, let's assign these settings into our
     * device-independant holder.
     */

    scForeFlags = fore;
    scBackFlags = back;

    /* Next, determine the console's actual current settings */

    GetConsoleScreenBufferInfo(hConOut, &csbi);

    /* Mask out any of the attributes which can be set via
     * our device-independant options. Since the console settings
     * have additional options, we need to retain those so we don't
     * do something unpleasant to our I/O abilities, for instance.
     */

    wAttributes = csbi.wAttributes;

    wAttributes &= ~(bothFore[CONSOLE_FLAG] | bothBack[CONSOLE_FLAG]);

    /* Now, loop through the device-independant possibilities for
     * flags, and modify our console flags as appropriate.
     */

    for (i=0; i<NUM_SC_ATTRIBUTES; i++)
    {
        if (scForeFlags & scForeMapping[i][SC_FLAG])
            wAttributes |= scForeMapping[i][CONSOLE_FLAG];

        if (scBackFlags & scBackMapping[i][SC_FLAG])
            wAttributes |= scBackMapping[i][CONSOLE_FLAG];
    }

    /* One last unpleasantry: if reverse video is set, then we should
     * reverse the foreground and background colors 
     */

    if (scForeFlags & SC_RV)
    {
        int tmpFore, tmpBack;

        tmpFore = wAttributes & conForeColors;
        tmpBack = wAttributes & conBackColors;

        wAttributes &= ~(conForeColors | conBackColors);

        for (i=0; i<NUM_CON_COLORS; i++)
        {
            if (tmpFore & conColorMapping[i][0])
                wAttributes |= conColorMapping[i][1];

            if (tmpBack & conColorMapping[i][1])
                wAttributes |= conColorMapping[i][0];
        }
    }

    /* The appropriate colors, etc. should be set in
     * the wAttributes variable now. Apply them to the
     * current console.
     */

    SetConsoleTextAttribute(hConOut, wAttributes);

    return(0);
}


/* beRawTextOut-
 *
 * The name of this function is misleading. Given a pointer to
 * ascii text and a count of bytes to print, print them to the
 * display device. If wrapping is enabled, wrap text. If there is a
 * scrolling region set and the cursor is in it,
 * scroll only within that region. 'beRawTextOut' means that it's guaranteed
 * not to have control sequences within the text. 
 */

int
beRawTextOut(
    char *text,
    int len
    )
{
    int i,j;

    /* If there's no work to do, return immediately. */
    if ((text == NULL)||(len == 0))
        return(0);

    i=0;

    /* Otherwise, loop through the text until all of it has been output */
    while (i < len)
    {
        /* This inner loop serves to divide the raw text to output into
         * explicit lines. While the 'RawPrintLine' may still have to
         * break lines to do text wrapping, explicit line breaks are
         * handled right here. 
         */
        j=i;
        while ((text[j] != '\n')&&(j<len))
        {
            j++;
        }

        RawPrintLine(text+i, j-i, (text[j] == '\n'));

        i = j+1;
    }

    return(0);
}


/* RawPrintLine -
 *
 * This routine is a helper for beRawTextOut. It is given a
 * line of text which is guaranteed not to have any newlines
 * or control characters (which need to be interpreted) in it.
 * It prints out the text, wrapping if necessary, and handles
 * scrolling or truncation.
 *
 * If scrollAtEnd is true, an extra carriage return (scroll) is
 * performed after the text has been printed out.
 */

int
RawPrintLine(
    char *text,
    int len,
    int scrollAtEnd
    )
{
    int i, end;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwWritten;

    if ((scrollAtEnd == FALSE) && ((text == NULL)||(len == 0)))
        return(0);

    GetConsoleScreenBufferInfo(hConOut, &csbi);

    /* find out how far to the first tab or end of text */

    if (text != NULL)
    {
        for (end=0; end<len; end++)
        {
            if (text[end] == '\t')
                break;
        }

        if (end > (csbi.dwSize.X - csbi.dwCursorPosition.X))
            end = (csbi.dwSize.X - csbi.dwCursorPosition.X);

        /* If we're in non-ascii mode, we need to do a little
         * magic to get the right characters out.
         */

        if (scForeFlags & SC_G1)
        {
            for (i=0; i<end; i++)
            {
                text[i] = G1Chars[text[i]];
            }
        }

        /* actually print out the text. */

        WriteConsole(hConOut,text,(DWORD)end,&dwWritten,NULL);

        if (end == (csbi.dwSize.X - csbi.dwCursorPosition.X))
            Scroll(CUR_ROW);

        if (   (!(conTermMode & WRAP_MODE))
            && (end == (csbi.dwSize.X - csbi.dwCursorPosition.X))
           )
            end = len;

        if (end != len)
        {
            if (text[end] == '\t')
            {
                beAdvanceToTab();
                RawPrintLine(text+end+1,len - (end+1), FALSE);
            }
            else
            {
                RawPrintLine(text+end, len-end, FALSE);
                beAbsoluteCursor(CUR_ROW,1);
                beOffsetCursor(1,0);
            }
        }
    }

    /* Now that we've printed this line, scroll if we need to.
     * Note that a scroll implies a newline.
     */

    if (scrollAtEnd)
    {
        Scroll(CUR_ROW);
        beAbsoluteCursor(CUR_ROW,1);
        beOffsetCursor(1,0);
    }

    return(0);
}


/* Scroll -
 *
 * Given a row specification, calculate a scroll executed in that
 * row. It could be within a scroll range, or outside of it.
 *
 * For some ungodly reason, I made this routine handle the TOP_EDGE,
 * BOTTOM_EDGE, and CUR_ROW specifiers as well as a real row.
 */

int
Scroll(
    int row
    )
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    SMALL_RECT scrollRect;
    COORD dest;
    CHAR_INFO fillChar;

    GetConsoleScreenBufferInfo(hConOut, &csbi);

    if (row == TOP_EDGE)
        row = csbi.srWindow.Top;
    else if (row == BOTTOM_EDGE)
        row = csbi.srWindow.Bottom;
    else if (row == CUR_ROW)
        row = csbi.dwCursorPosition.Y;
    else
        row += csbi.srWindow.Top;

    /* Escape out if we don't really need to scroll */

    if ( (row < (csbi.dwSize.Y-1))
        && ((row-csbi.srWindow.Top + 1) < bottomScrollRow)
        )
        return(0);

    /* NT console requires a fill character for scrolling. */

    fillChar.Char.AsciiChar=' ';
    fillChar.Attributes = csbi.wAttributes;

    /* Determine the rectangle of text to scroll. Under NT this
     * is actually an overlap-safe block-copy.
     */
    scrollRect.Left = 0;
    scrollRect.Top = 1;
    scrollRect.Right = csbi.dwSize.X;
    scrollRect.Bottom = row;

    if (topScrollRow != TOP_EDGE)
    {
        scrollRect.Top = csbi.srWindow.Top + topScrollRow + 1;
    }

    if (   (bottomScrollRow != BOTTOM_EDGE)
        && ((csbi.srWindow.Top+bottomScrollRow) < scrollRect.Bottom)
       )
    {
        scrollRect.Bottom = csbi.srWindow.Top + bottomScrollRow;
    }

    dest.X = 0;
    dest.Y = scrollRect.Top - 1;

    ScrollConsoleScreenBuffer(hConOut,&scrollRect,NULL,
                              dest, &fillChar);

    return(0);
}


/* beEraseText -
 *
 * Given a 'from' and a 'to' position in display coordinates,
 * this function will fill in all characters between the two
 * (inclusive) with spaces. Note that the coordinates do NOT
 * specify a rectangle. erasing from (1,1) to (2,2) erases 
 * all of the first row, and the first two characters of the
 * second.
 *
 * Note that this routine must be able to handle TOP_EDGE, 
 * BOTTOM_EDGE, LEFT_EDGE, RIGHT_EDGE, CUR_ROW, and CUR_COL
 * in the appropriate parameters.
 */

int
beEraseText(
    int rowFrom,
    int colFrom,
    int rowTo,
    int colTo
    )
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD dest, saveCursor;
    DWORD nLength, dwWritten;

    GetConsoleScreenBufferInfo(hConOut, &csbi);

    saveCursor = csbi.dwCursorPosition;

    /* Convert the row and column specifications into
     * buffer coordinates
     */
    if (rowFrom == CUR_ROW)
        rowFrom = csbi.dwCursorPosition.Y;
    else if (rowFrom == TOP_EDGE)
        rowFrom = csbi.srWindow.Top;
    else 
        rowFrom += csbi.srWindow.Top -1;

    if (colFrom == CUR_COL)
        colFrom = csbi.dwCursorPosition.X;
    else if (colFrom == LEFT_EDGE)
        colFrom = 0;
    else
        colFrom -= 1;

    if (rowTo == CUR_ROW)
        rowTo = csbi.dwCursorPosition.Y;
    else if (rowTo == BOTTOM_EDGE)
        rowTo = csbi.srWindow.Bottom;
    else
        rowTo += csbi.srWindow.Top-1;

    if (colTo == CUR_COL)
        colTo = csbi.dwCursorPosition.X;
    else if (colTo == RIGHT_EDGE)
        colTo = csbi.dwSize.X;
    else
        colTo -= 1;

    /* We're going to erase by filling a continuous range of
     * character cells with spaces. Note that this has displeasing
     * asthetics under NT, as highlighting appears to be immune.
     */
    nLength = (rowTo - rowFrom)*csbi.dwSize.X;
    nLength += colTo - colFrom;

    dest.X = colFrom;
    dest.Y = rowFrom;

    FillConsoleOutputCharacter(hConOut, ' ', nLength, dest, &dwWritten);
    FillConsoleOutputAttribute(hConOut, csbi.wAttributes, nLength, dest, &dwWritten);

    SetConsoleCursorPosition(hConOut, saveCursor);

    return(0);
}


/* beDeleteText -
 *
 * Given a screen cursor 'from' and 'to' position, this function
 * will delete all text between the two. Text will be scrolled
 * up as appropriate to fill the deleted space. Note that, as in
 * beEraseText, the two coordinates don't specify a rectangle, but
 * rather a starting position and ending position. In other words,
 * deleting from (1,1) to (2,2) should move the text from (2,3) to the
 * end of the second row to (1,1), move line 3 up to line 2, and so on.
 *
 * This function must be able to process TOP_EDGE, BOTTOM_EDGE, LEFT_EDGE,
 * RIGHT_EDGE, CUR_ROW, and CUR_COL specifications in the appropriate 
 * variables as well as regular row and column specifications.
 */

int
beDeleteText(
    int rowFrom,
    int colFrom,
    int rowTo,
    int colTo
    )
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD dest, saveCursor;
    CHAR_INFO fillChar;

    GetConsoleScreenBufferInfo(hConOut, &csbi);

    saveCursor = csbi.dwCursorPosition;

    if (rowFrom == CUR_ROW)
        rowFrom = csbi.dwCursorPosition.Y;
    else if (rowFrom == TOP_EDGE)
        rowFrom = csbi.srWindow.Top;
    else 
        rowFrom += csbi.srWindow.Top -1;

    if (colFrom == CUR_COL)
        colFrom = csbi.dwCursorPosition.X;
    else if (colFrom == LEFT_EDGE)
        colFrom = 0;
    else
        colFrom -= 1;

    if (rowTo == CUR_ROW)
        rowTo = csbi.dwCursorPosition.Y;
    else if (rowTo == BOTTOM_EDGE)
        rowTo = csbi.srWindow.Bottom;
    else
        rowTo += csbi.srWindow.Top-1;

    if (colTo == CUR_COL)
        colTo = csbi.dwCursorPosition.X;
    else if (colTo == RIGHT_EDGE)
        colTo = csbi.dwSize.X;
    else
        colTo -= 1;

    fillChar.Char.AsciiChar=' ';
    fillChar.Attributes = csbi.wAttributes;

    /* Now that we've got the from and to positions
     * set correctly, we need to delete appropriate
     * rows and columns.
     */

    dest.X = colFrom;
    dest.Y = rowFrom;

    /* BUGBUG - need to implement this. What can I say, I'm lazy :) */

    return(0);
}


/* beInsertRow -
 *
 * Given a row number or CUR_ROW, TOP_EDGE or BOTTOM_EDGE as an input, 
 * this function will scroll all text from the current row down down by one,
 * and create a blank row under the cursor.
 */

int
beInsertRow(
    int row
    )
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD dest;
    CHAR_INFO fillChar;
    SMALL_RECT scrollRect;

    GetConsoleScreenBufferInfo(hConOut, &csbi);

    fillChar.Char.AsciiChar=' ';
    fillChar.Attributes = csbi.wAttributes;

    if (row == CUR_ROW)
        row = csbi.dwCursorPosition.Y;
    else if (row == TOP_EDGE)
        row = csbi.srWindow.Top;
    else if (row == BOTTOM_EDGE)
        row = csbi.srWindow.Bottom;
    else
        row += csbi.srWindow.Top-1;

    dest.X = 0;
    dest.Y = row+1;

    scrollRect.Top = row;
    scrollRect.Left = 0;
    scrollRect.Right = csbi.dwSize.X;
    scrollRect.Bottom = csbi.srWindow.Bottom;

    ScrollConsoleScreenBuffer(hConOut, &scrollRect, NULL, dest, &fillChar);

    return(0);
}


/* beTransmitText -
 *
 * Given a pointer to text and byte count, this routine should transmit data 
 * to whatever host made the request it's responding to. Typically this routin
 * should transmit data as though the user had typed it in.
 */

int
beTransmitText(
    char *text,
    int len
    )
{
    if ((text == NULL) || (len < 1))
        return(0);

    /* BUGBUG - need to implement this. */

    return(0);
}


/* beAdvanceToTab -
 *
 * This routine will destructively advance the cursor to the
 * next set tab, or to the end of the line if there are no
 * more tabs to the right of the cursor.
 */

int
beAdvanceToTab(void)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int i, col, tocol;
    COORD dest;
    DWORD dwWritten;

    GetConsoleScreenBufferInfo(hConOut,&csbi);

    col = csbi.dwCursorPosition.X+1;

    dest = csbi.dwCursorPosition;

    for(i=0; i<numTabs; i++)
    {
        if (col < tabSet[i])
        {
            tocol = tabSet[i];
            break;
        }
    }

    if (i == numTabs)
    {
        tocol = csbi.dwSize.X;
    }

    FillConsoleOutputCharacter(hConOut, ' ', tocol-col,
                               dest, &dwWritten);
    
    beOffsetCursor(0,tocol-col);

    return(0);
}


/* beClearTab -
 *
 * This function accepts a constant, and will try to clear tabs
 * appropriately. Its argument is either
 * ALL_TABS, meaning all tabs should be removed
 * CUR_COL, meaning the tab in the current column should be wiped, or 
 * a column value, meaning if there's a tab there it should be wiped.
 *
 */

int
beClearTab(
    int col
    )
{
    int i, j;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    if (col == ALL_TABS)
    {
        tabSet[0] = 0;
        numTabs = 0;
        return(0);
    }

    if (col == CUR_COL)
    {
        GetConsoleScreenBufferInfo(hConOut,&csbi);
        col = csbi.dwCursorPosition.X+1;
    }

    for (i=0; i<numTabs; i++)
    {
        if (tabSet[i] == col)
        {
            numTabs -= 1;

            for (j=i; j<numTabs; j++)
                tabSet[j]=tabSet[j+1];

            break;
        }
    }

    return(0);
}

/* beSetScrollingRows -
 *
 * Given a pair of row numbers, this routine will set the scrolling
 * rows to those values. Note that this routine will accept
 * TOP_ROW and BOTTOM_ROW as values, meaning that scrolling should
 * be enabled for the entire display, regardless of resizing.
 */

int
beSetScrollingRows(
    int fromRow,
    int toRow
    )
{
    if (fromRow > toRow)
        return(-1);

    topScrollRow = fromRow;
    bottomScrollRow = toRow;

    return(0);
}


/* beRingBell -
 *
 *  Ring the system bell once.
 */

int
beRingBell(void)
{
    MessageBeep((UINT)-1);
    return(0);
}


/* beGetTermMode -
 * 
 * Return the value of conTermMode, which is the terminal settings which
 * can be queried/set by <esc>[?#h/l.
 */

int
beGetTermMode()
{
    return(conTermMode);
}


/* beSetTermMode -
 * 
 * Set the terminal as requested, assuming we can. Right now we only handle a 
 * couple of the possible flags, but we store many of the others. 
 */

int beSetTermMode(
    int newMode
    )
{
    int i, changes;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD newSize;
    SMALL_RECT newWindowRect;
    DWORD dwConMode;

    changes = conTermMode ^ newMode;

    /* For each bit set in 'changes', determine the
     * appropriate course of action.
     */

    for (i=0; i < NUM_TERM_ATTR_MODES; i++)
    {
        if (termAttrMode[i] & changes)
        {
            switch(termAttrMode[i])
            {
            case COL132_MODE:
                GetConsoleScreenBufferInfo(hConOut, &csbi);
                newSize.Y = csbi.dwSize.Y;
                newSize.X = (newMode & COL132_MODE) ? 132 : 80;
                if (newSize.X != csbi.dwSize.X)
                {
                    newWindowRect.Top = csbi.srWindow.Top;
                    newWindowRect.Bottom = csbi.srWindow.Bottom;
                    newWindowRect.Left = 0;
                    newWindowRect.Right = csbi.dwSize.X - 1;
                    SetConsoleScreenBufferSize(hConOut, newSize);
                    SetConsoleWindowInfo(hConOut, TRUE, &newWindowRect);
                }
                break;

            case WRAP_MODE:
                GetConsoleMode(hConOut,&dwConMode);
                if (   (newMode & WRAP_MODE) 
                    && (! (dwConMode & ENABLE_WRAP_AT_EOL_OUTPUT))
                   )
                {
                    dwConMode |= ENABLE_WRAP_AT_EOL_OUTPUT;
                    SetConsoleMode(hConOut, dwConMode);
                }
                if (   (!(newMode & WRAP_MODE))
                    && (dwConMode & ENABLE_WRAP_AT_EOL_OUTPUT)
                   )
                {
                    dwConMode &= ~ENABLE_WRAP_AT_EOL_OUTPUT;
                    SetConsoleMode(hConOut, dwConMode);
                }
                break;

            case CURSORAPPL_MODE:
            case ANSI_MODE:
            case SMOOTHSCROLL_MODE:
            case REVSCREEN_MODE:
            case ORIGINREL_MODE:
            case REPEAT_MODE:
                /* bugbug - we don't handle any of these. */
                break;
            }
        }
    }

    conTermMode = newMode;

    return(0);
}
