/* vt100.c
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
 */

/* This is the main code body for my generic vt-100 emulator. This code
 * body provides parsing for most of the vt-100 escape sequences, but it
 * doesn't actually do anything with some of them. The idea behind this 
 * parser is to provide a generic front-end that you can initialize, then
 * send all of your output to. The output is parsed by the routines in this
 * program, then spit out to a back-end.
 *
 * What back-end you say? Well, the one you have to supply yourself. There's a
 * dozen or so routines you have to provide for character-based I/O, cursor 
 * movement, erasing and deleting text, and setting text and terminal attributes.
 *
 * For a list of the routines your back end must supply, read vt100.h closely.
 *
 * In case it's not obvious, these routines were written for a system running win32.
 * for vt100.c and vt100.h, most of the code should be easily portable to other
 * operating systems. Yeah, like they NEED a vt-100 emulator :p
 */

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "vt100.h"
  
 
/*  NOTE - many of the functions look like they should
 * take X-Y pairs. Bear in mind this is a text display,
 * so for this we're talking ROWS and COLUMNS. So parm
 * lists go (row, column), which is the opposite of (x, y).
 */

char    cBuffer[MAXVTBUFLEN+1]; /* terminal output buffer for unprocessed characters */
int     BufLen;                 /* Number of characters in cBuffer waiting for output */

/* List of all device-independant colors. These colors will be transmitted to a
 * back-end routine responsible for setting the foreground and background text
 * colors appropriately. Note that the color combinations are ordered to correspond
 * with combinations of red (0x1), green (0x2) and blue (0x4) bit flags.
 */

int ScColorTrans[8]= { 0,
                    SC_RED,
                    SC_GREEN,
                    SC_RED|SC_GREEN,
                    SC_BLUE,
                    SC_RED|SC_BLUE,
                    SC_RED|SC_GREEN,
                    SC_RED|SC_GREEN|SC_BLUE
                    };


/* List of terminal attributes which we track (used by <esc>[?#h and <esc>[?#l) */

int termAttrMode[NUM_TERM_ATTR_MODES] = { 0,
                    CURSORAPPL_MODE,
                    ANSI_MODE,
                    COL132_MODE,
                    SMOOTHSCROLL_MODE,
                    REVSCREEN_MODE,
                    ORIGINREL_MODE,
                    WRAP_MODE,
                    REPEAT_MODE,
                    };

/* FORWARD FUNCTION DECLARATIONS -
 * these functions are intended for use only in this module */

static int ProcessBracket(int Start);
static int ProcessEscape(int Start);
static int ProcessControl(int Start);
static int ProcessBuffer(void);

/* END FORWARD FUNCTION DECLARATIONS */



/* vtputs() - 
 * 
 * front-end 'puts()' substitute. Call this routine instead 
 * of 'puts()', and it'll pass the output through the vt100 emulator.
 */

vtputs(char *f)
{
    char cbuf[1024];

    strcpy(cbuf,f);
    strcat(cbuf,"\n");
    vtProcessedTextOut(cbuf, strlen(cbuf));

    return(0);
}


/* vtprintf -
 * 
 * This routine is a substitute for printf(). Call this routine with the 
 * same parameters you would use for printf, and output will be channelled
 * through the vt-100 emulator.
 */

vtprintf(char *format, ...)
{
    char cbuf[1024];
    va_list va;

    va_start(va, format);
    vsprintf(cbuf,format, va);
    va_end(va);

    vtProcessedTextOut(cbuf, strlen(cbuf));

    return(0);
}

/* vtInitVT100 -
 *
 * Set the initial state of the VT-100 emulator, and call the initialization
 * routine for the back end. This routine MUST be invoked before any other, or 
 * the VT-100 emulator will most likely roll over and die.
 */

vtInitVT100(void)
{
    int i=0;

    cBuffer[0]='\0';
    BufLen=0;

    beInitVT100Terminal(); /* call the back-end initialization. */

    return(0);
}


/* ProcessBracket -
 * 
 * Helper routine for processing escape sequences. By the time this
 * routine is invoked, '<esc>[' has already been read in the input
 * stream. 'Start' is an index in cBuffer to the '<esc>'.
 *
 * If an escape sequence is successfully parsed, return the index of the
 * first character AFTER the escape sequence. Otherwise, return 'Start'.
 *
 */

static int ProcessBracket(int Start)
{
    int End;                 /* Current character being examined in cBuffer */
    int args[10], numargs=0; /* numerical args after <esc>[ */
    int iMode;               
    int i;
    int itmp=0;
    int left;
    int iForeground, iBackground;

    /* If there's no valid escape sequence, return as we were called. */

    if ((cBuffer[Start+1] != '[')||(Start+2 >= BufLen))
        return(Start);

    End = Start + 2;

    /* Loop through the buffer, hacking out all numeric
     * arguments (consecutive string of digits and
     * semicolons
     */

    do {
        itmp = 0; /* itmp will hold the current arguments integer value */

                  /* the semicolon is a delimiter */
        if (cBuffer[End] == ';')
        {
            End++;

            if (End >= BufLen)
                return(Start);
        }

        /*  Parse this argument into a number. */

        while (isdigit(cBuffer[End]))
        {
            itmp = itmp*10 + (cBuffer[End]-'0');
            End++;
            if (End >= BufLen)
                return(Start);
        }

        /* Save the numeric argument if we actually
         * parsed a number.
         */

        if (End != Start + 2)
            args[numargs++] = itmp;

    } while (cBuffer[End] == ';');

    /* At this point, we've come across a character that isn't a number
     * and isn't a semicolon. This means it is the command specifier.
     */

    /* Number of characters left in the input stream. I don't use 
     * this as rigorously as I should here.
     */

    left =  BufLen - End;

    /* Return if there's definitely not enough characters for a
     * full escape sequence.
     */

    if (left <= 0)
        return(Start);

    /* Giant switch statement, parsing the command specifier that followed
     * up <esc>[arg;arg;...arg
     */

    switch (cBuffer[End])
    {
    /*         Cursor Up:      Esc [ Pn A */
    case 'A':
        beOffsetCursor(numargs ? -args[0] : -1, 0);
        End += 1;
        break;

    /*         Cursor Down:    Esc [ Pn B */
    case 'B':
        beOffsetCursor(numargs ? args[0] : 1, 0);
        End += 1;
        break;

    /*         Cursor Right:    Esc [ Pn C */
    case 'C':
        beOffsetCursor(0, numargs ? args[0] : 1);
        End += 1;
        break;

    /*          Cursor Left:   Esc [ Pn D */
    case 'D':
        beOffsetCursor(0, numargs ? -args[0] : -1);
        End += 1;
        break;

    /*          Direct Addressing :  Esc [ Pn(row);Pn(col)H or
     *                               Esc [ Pn(row);Pn(col)f
     */
    case 'H':
    case 'f':
        if (numargs == 0)
            beAbsoluteCursor(1,1);
        else if (numargs == 1)
            beAbsoluteCursor(args[0] > 0 ? args[0] : 1,1);
        else if (numargs == 2)
            beAbsoluteCursor(args[0] > 0 ? args[0] : 1, args[1] > 0 ? args[1] : 1);

        End += 1;
        break;

    /*         Erase from Cursor to end of screen         Esc [ J
     *         Erase from Beginning of screen to cursor   Esc [ 1 J
     *         Erase Entire screen                        Esc [ 2 J
     */
    case 'J':
        if (numargs == 0)
            beEraseText(CUR_ROW, CUR_COL, BOTTOM_EDGE, RIGHT_EDGE);
        else if (args[0] == 1)
            beEraseText(TOP_EDGE, LEFT_EDGE, CUR_ROW, CUR_COL);
        else
            beEraseText(TOP_EDGE, LEFT_EDGE, BOTTOM_EDGE, RIGHT_EDGE);

        End += 1;
        break;

    /*         Erase from Cursor to end of line           Esc [ K
     *         Erase from Beginning of line to cursor     Esc [ 1 K
     *         Erase Entire line                          Esc [ 2 K
     */
    case 'K':
        if (numargs == 0)
            beEraseText(CUR_ROW, CUR_COL, CUR_ROW, RIGHT_EDGE);
        else if (args[0] == 1)
            beEraseText(CUR_ROW, LEFT_EDGE, CUR_ROW, CUR_COL);
        else
            beEraseText(CUR_ROW, LEFT_EDGE, CUR_ROW, RIGHT_EDGE);

        End += 1;
        break;


    /*  Set Graphics Rendition:
     *          ESC[#;#;....;#m
     *  The graphics rendition is basically foreground and background
     *  color and intensity.
     */
    case 'm':
        beGetTextAttributes(&iForeground, &iBackground);

        if (numargs < 1)
        {
            /* If we just get ESC[m, treat it as though
             * we should shut off all extra text
             * attributes
             */

            iForeground &= ~(SC_BOLD|SC_UL|SC_BL|SC_RV|SC_GRAPHICS|SC_G0|SC_G1);
            iForeground |= SC_ASCII;

            beSetTextAttributes(iForeground, iBackground);
            End += 1;
            break;
        }

        /* Loop through all the color specs we got, and combine them
         * together. Note that things like 'reverse video', 'bold',
         * and 'blink' are only applied to the foreground. The back end
         * is responsible for applying these properties to all text.
         */
        for (i=0; i < numargs; i++)
        {
            switch(args[i])
            {
            /* 0 for normal display */
            case 0:
                iForeground &= ~SC_BOLD;
                break;

            /* 1 for bold on */
            case 1:
                iForeground |= SC_BOLD;
                break;

            /* 4 underline (mono only) */
            case 4:
                iForeground |= SC_UL;
                break;

            /* 5 blink on */
            case 5:
                iForeground |= SC_BL;
                break;

            /* 7 reverse video on */
            case 7:
                iForeground |= SC_RV;
                break;

            /* 8 nondisplayed (invisible)  BUGBUG - not doing this. */


            /* 30-37 is bit combination of 30+ red(1) green(2) blue(4)
             * 30 black foreground
             */
            case 30:
            case 31:
            case 32:
            case 33:
            case 34:
            case 35:
            case 36:
            case 37:
                iForeground &= ~(SC_RED|SC_GREEN|SC_BLUE);
                iForeground |= ScColorTrans[args[i]-30];
                break;

            /* 40-47 is bit combo similar to 30-37, but for background. */
            case 40:
            case 41:
            case 42:
            case 43:
            case 44:
            case 45:
            case 46:
            case 47:
                iBackground &= ~(SC_RED|SC_GREEN|SC_BLUE);
                iBackground |= ScColorTrans[args[i]-30];
                break;
            }
        }

        beSetTextAttributes(iForeground, iBackground);

        End += 1;
        break;


    /*
     *         Set with        Esc [ Ps h
     *         Reset with      Esc [ Ps l
     * Mode name               Ps      Set             Reset
     * -------------------------------------------------------------------
     * Keyboard action         2       Locked          Unlocked
     * Insertion               4       Insert          Overwrite
     * Send - Receive          12      Full            Echo
     * Line feed/New line      20      New line        Line feed
     */
    case 'h':
    case 'l':
    /* BUGBUG - many of the terminal modes are set with '?' as the
     * first character rather than a number sign. These are dealt with
     * later in this switch statement because they must be. Most other
     * settings are just ignored, however.
     */
        End += 1;
        break;

    /*         Insert line             Esc [ Pn L */
    case 'L':
        beInsertRow(CUR_ROW);
        End += 1;
        break;

    /*         Delete line             Esc [ Pn M */
    case 'M':
        do {
            beDeleteText(CUR_ROW,LEFT_EDGE, CUR_ROW, RIGHT_EDGE);
        } while (--args[0] > 0);

        End += 1;
        break;

    /*         Delete character        Esc [ Pn P */
    case 'P':
        do {
            beDeleteText(CUR_ROW, CUR_COL, CUR_ROW, CUR_COL);
        } while (--args[0] > 0);
        End += 1;
        break;

    /*         Set the Scrolling region        Esc [ Pn(top);Pn(bot) r */
    case 'r':
        if (numargs == 0)
            beSetScrollingRows(TOP_EDGE,BOTTOM_EDGE);
        else if (numargs == 2)
            beSetScrollingRows(args[0],args[1]);
        End += 1;
        break;

    /*         Print screen or region  Esc [ i
     *         Print cursor line       Esc [ ? 1 i
     *         Enter auto print        Esc [ ? 5 i
     *         Exit auto print         Esc [ ? 4 i
     *         Enter print controller  Esc [ 5 i
     *         Exit print controller   Esc [ 4 i
     */
    case 'i':
        /* BUGBUG - print commands are not acted upon. */
        End += 1;
        break;


    /*         Clear tab at current column     Esc [ g
     *         Clear all tabs                  Esc [ 3 g
     */
    case 'g':
        if (numargs == 0)
            beClearTab(CUR_COL);
        else
            if ((numargs == 1) && (args[0] == 3))
                beClearTab(ALL_TABS);

        End += 1;
        break;

    /* BUGBUG - queries which require responses are ignored. */

    /*         Esc [ c         DA:Device Attributes
     *                         or
     *
     *         Esc [ <sol> x   DECREQTPARM: Request Terminal Parameters
     *                         * <sol> values other than 1 are ignored.  Upon
     *                           receipt of a <sol> value of 1, the following
     *                           response is sent:
     *                                 Esc[3;<par>;<nbits>;<xspeed>;<rspeed>;1;0x
     *
     *                                 * Where <par>, <nbits>, <xspeed>, and <rspeed>
     *                                   are as for VT100s with the following
     *                                   exceptions:
     *                                   <nbits>       Values of 5 and 6 bits per
     *                                                 character are sent as 7 bits.
     *                                   <xspeed>,<rspeed>
     *                                                 These two numbers will always
     *                                                 be the same.  9600 baud is
     *                                                 sent for 7200 baud.
     *
     *         Esc [ Ps n      DSR: Device Status Report
     *                         * Parameter values other than 5, 6, are ignored.
     *                           If the parameter value is 5, the sequence
     *                           Esc [ O n is returned.  If the parameter value is
     *                           6, the CPR: Cursor Position Report sequence
     *                           Esc [ Pn ; Pn R is returned with the Pn set to
     *                           cursor row and column numbers.
     *
     * Cursor Controls:
     *          ESC[#;#R                       Reports current cursor line & column
     */

    /*                         spec    <esc>[<spec>h   <esc>[<spec>l
     * Cursor key              ?1      Application     Cursor
     * ANSI/VT52               ?2      ANSI            VT52
     * Column                  ?3      132             80
     * Scrolling               ?4      Smooth          Jump
     * Screen                  ?5      Reverse         Normal
     * Origin                  ?6      Relative        Absolute
     * Wraparound              ?7      Wrap            Truncate
     * Auto key repeat         ?8      Repeating       No repeat
     */
    case '?':
        /* We didn't catch the numeric argument because the '?' stopped
         * it before. Parse it now. 
         */
        args[0]=0;
        while (isdigit(cBuffer[++End]))
        {
            if (End >= BufLen)
                return(Start);
            args[0] = 10*args[0] + (cBuffer[End]-'0');
        }

        /* If we don't handle this particular '?' command (and
         * there are plenty we don't) then just ignore it. 
         */
        if (  (args[0] > 8) 
            ||( (cBuffer[End] != 'l') && (cBuffer[End] != 'h'))
           )
        {
            End++;
            if (End >= BufLen)
                return(Start);
            break;
        }

        /* This command sets terminal status. Get the current status,
         * determine what needs to be changed, and send it back.
         */

        iMode = beGetTermMode();

        /* If we need a given mode and it's not already set, set it. */

        if ((cBuffer[End] == 'h') && (!(iMode & termAttrMode[args[0]])))
        {
            beSetTermMode(iMode | termAttrMode[args[0]]);
        }

        /* likewise, clear it as appropriate */
        if ((cBuffer[End] == 'l') && (iMode & termAttrMode[args[0]]))
        {
            beSetTermMode(iMode & ~termAttrMode[args[0]]);
        }

        End++;
        break;

    /* If it's an escape sequence we don't treat, pretend as though we never saw
     * it.
     */
    default:
        End += 1;
        break;
    }

    return(End);

}


/* ProcessEscape - 
 * 
 * At this point, <esc> has been seen. Start points to the escape
 * itself, and then this routine is responsible for parsing the
 * rest of the escape sequence, and either pawning off further parsing,
 * or acting upon it as appropriate.
 *
 * Note that the escape sequences being parsed are still contained in cBuffer.
 */

static int ProcessEscape(int Start)
{
    int End;
    int left;
    int fore, back;
    int i;

    /* if it's definitely not a full escape sequence, return that we haven't
     * seen it.
     */
    if ((cBuffer[Start] != 27)||(Start+1 >= BufLen))
        return(Start);

    End = Start + 1;
    
    /* At this point, if the sequence is <esc> x, 'End' points at
     * x
     */

    /* left = number of characters left unparsed in the buffer. */
    left =  BufLen - End -1;

    /* Main switch statement - parse the escape sequence according to the
     * next character we see.
     */

    switch (cBuffer[End])
    {
    /*         Hard Reset                      Esc c    BUGBUG - not imp'd. */
    case 'c':
        End += 1;
        break;

    /*         Cursor up               Esc A */
    case 'A':
        beOffsetCursor(-1,0);
        End += 1;
        break;

    /*         Cursor down             Esc B */
    case 'B':
        beOffsetCursor(1,0);
        End += 1;
        break;

    /*         Cursor right            Esc C */
    case 'C':
        beOffsetCursor(0,1);
        End += 1;
        break;

    /*         Cursor left             Esc D */
    case 'D':
        beOffsetCursor(0,-1);
        End += 1;
        break;

    /*         Newline command:       Esc E */
    case 'E':
        beRawTextOut("\n",strlen("\n"));
        End += 1;
        break;

    /*          Invoke the Graphics character set  Esc F */
    case 'F':
        beGetTextAttributes(&fore, &back);
        if (! (fore & SC_GRAPHICS))
        {
            fore &= ~(SC_ASCII|SC_G0|SC_G1);
            fore |= SC_GRAPHICS;
            beSetTextAttributes(fore, back);
        }
        End += 1;
        break;

    /*         Invoke the ASCII character set     Esc G */
    case 'G':
        beGetTextAttributes(&fore, &back);
        if (! (fore & SC_ASCII))
        {
            fore &= ~(SC_G0|SC_G1|SC_GRAPHICS);
            fore |= SC_ASCII;
            beSetTextAttributes(fore, back);
        }
        End += 1;
        break;

    /*          Move the cursor to (1,1): Home cursor             Esc H */
    case 'H':
        beAbsoluteCursor(TOP_EDGE,LEFT_EDGE);
        End += 1;
        break;

    /*         Reverse line feed       Esc I */
    case 'I':
        beOffsetCursor(-1,0);
        End += 1;
        break;

    /*         Erase to end of screen  Esc J   */
    case 'J':
        beEraseText(CUR_ROW, CUR_COL, BOTTOM_EDGE, RIGHT_EDGE);
        End += 1;
        break;

    /*         Erase to end of line    Esc K */
    case 'K':
        beEraseText(CUR_ROW, CUR_COL, CUR_ROW, RIGHT_EDGE);
        End += 1;
        break;

    /*         Reverse Line:   Esc M */
    case 'M':
        beAbsoluteCursor(CUR_ROW, LEFT_EDGE);
        beOffsetCursor(-1,0);
        End += 1;
        break;

    /* Switch to G1 graphics character set.         Esc N */
    case 'N':
        beGetTextAttributes(&fore, &back);
        if (! (fore & SC_G1))
        {
            fore &= ~(SC_G0|SC_ASCII|SC_GRAPHICS);
            fore |= SC_G1;
            beSetTextAttributes(fore, back);
        }
        End += 1;
        break;

    /* Switch to G0 graphics character set        Esc O */
    case 'O':
        beGetTextAttributes(&fore, &back);
        if (! (fore & SC_G0))
        {
            fore &= ~(SC_G1|SC_ASCII|SC_GRAPHICS);
            fore |= SC_G0;
            beSetTextAttributes(fore, back);
        }
        End += 1;
        break;

    /*         Print cursor line       Esc V   BUGBUG - unimp'd */
    case 'V':
        End += 1;
        break;

    /*          Enter print controller  Esc W  BUGBUG - unimp'd */
    case 'W':
        End += 1;
        break;

    /*         Exit print controller   Esc X    BUGBUG - unimp'd */
    case 'X':
        End += 1;
        break;

    /*         Cursor address          Esc Y row col BUGBUG - unimp'd and needed */
    case 'Y':
        End += 1;
        break;

    /*         Identify terminal type     Esc Z    */
    case 'Z':
        beTransmitText(ANSWERBACK_MESSAGE,strlen(ANSWERBACK_MESSAGE));
        End += 1;
        break;

    /* One of dozens of escape sequences starting <esc>[. Parse further */
    case '[':
        /* pass in the escape as the starting point */
        End = ProcessBracket(End-1);
        break;

    /*        Print screen            Esc ]     BUGBUG - unimp'd */
    case ']':
        End += 1;
        break;

    /*         Enter auto print        Esc ^    BUGBUG - unimpd' */
    case '^':
        End += 1;
        break;

    /*         Exit auto print         Esc -    BUGBUG - unimpd' */
    case '-':
        End += 1;
        break;

    /*         Alternate keypad        Esc =    BUGBUG - unimpd' */
    case '=':
        End += 1;
        break;

    /*         Numeric keypad          Esc >    BUGBUG - unimpd' */
    case '>':
        End += 1;
        break;

    /*         Enter ANSI mode         Esc <    BUGBUG - unimpd' */
    case '<':
        End += 1;
        break;

    /*         Save cursor position & Attributes:     Esc 7 */
    case '7':
        beSaveCursor();
        End += 1;
        break;

    /*         Restore cursor position & attributes:  Esc 8 */
    case '8':
        beRestoreCursor();
        End += 1;
        break;

    /*  Set character size - BUGBUG - unimp'd.
     * # 1             Double ht, single width top half chars
     * # 2             Double ht, single width lower half chars
     * # 3             Double ht, double width top half chars
     * # 4             Double ht, double width lower half chars
     * # 5             Single ht, single width chars
     * # 6             Single ht, double width chars    
     */
    case '#':
        End += 1;
        break;

    /* Select character set
     * ESC ( A             British 
     * ESC ( C             Finnish
     * ESC ( E             Danish or Norwegian
     * ESC ( H             Swedish
     * ESC ( K             German
     * ESC ( Q             French Canadian
     * ESC ( R             Flemish or French/Belgian
     * ESC ( Y             Italian
     * ESC ( Z             Spanish
     * ESC ( 1             Alternative Character
     * ESC ( 4             Dutch
     * ESC ( 5             Finnish
     * ESC ( 6             Danish or Norwegian
     * ESC ( 7             Swedish
     * ESC ( =             Swiss (French or German)
     */
    case '(':
    case ')':
        /* BUGBUG - most character sets aren't implemented. */
        beGetTextAttributes(&fore, &back);
        switch (cBuffer[++End])
        {
        case 'B':    /* ESC ( B             North American ASCII set */
            i=SC_ASCII;
            break;

        case '0':    /* ESC ( 0             Line Drawing */
            i=SC_G1;
            break;

        case '2':    /* ESC ( 2             Alternative Line drawing */
            i=SC_G0;
            break;

        default:
            /* Make sure the screen mode isn't set. */
            i = 0xFFFFFFFF;
            break;
        }

        if (! (fore & i))
        {
            fore &= ~(SC_ASCII|SC_G0|SC_G1|SC_GRAPHICS);
            fore |= i;
            beSetTextAttributes(fore, back);
        }

        End += 1;
        break;

    /* Unknown escape sequence */
    default:
        {
            char cbuf[80];
            sprintf(cbuf,"<esc>%c", cBuffer[End]);
            beRawTextOut(cbuf+Start,6);
            End += 1;
        }
    }

    return(End);
}


/* ProcessControl -
 *
 * Process a probable escape or control sequence
 * starting at the supplied index. Return the index
 * of the first character *after* the escape sequence we
 * process.
 * In the case of an incomplete sequence, ie one
 * that isn't completed by the end of the buffer, return
 * 'Start'.
 * In the case of an invalid sequence,
 * note the invalid escape sequence, and return Start+1
 */

static int ProcessControl(int Start)
{
    int fore, back;
    int End = Start;

    /* Check to make sure we at least have enough characters
     * for a control character
     */

    if (Start >= BufLen)
        return(Start);

    switch (cBuffer[Start])
    {
    /*  NUL     0       Fill character; ignored on input.
     *  DEL     127     Fill character; ignored on input.
     */
    case 0:
    case 127:
        End += 1;
        break;

    /* ENQ     5       Transmit answerback message. */
    case 5:
        beTransmitText(ANSWERBACK_MESSAGE,strlen(ANSWERBACK_MESSAGE));
        End += 1;
        break;

    /* BEL     7       Ring the bell. */
    case 7:
        beRingBell();
        End += 1;
        break;

    /* BS      8       Move cursor left. */
    case 8:
        beOffsetCursor(0,-1);
        End += 1;
        break;

    /* HT      9       Move cursor to next tab stop. */
    case 9:
        beAdvanceToTab();
        End += 1;
        break;

    /* LF      10      Line feed; causes print if in autoprint. */
    case 10:
        beOffsetCursor(1,0);
        End += 1;
        break;

    /* VT      11      Same as LF. 
     * FF      12      Same as LF.
     */
    case 11:
    case 12:
        beOffsetCursor(1,0);
        End += 1;
        break;

    /* CR      13      Move cursor to left margin or newline. */
    case 13:
        beOffsetCursor(1,0);
        beAbsoluteCursor(CUR_ROW,LEFT_EDGE);
        End += 1;
        break;

    /* SO      14      Invoke G1 character set. */
    case 14:
        beGetTextAttributes(&fore, &back);
        if (! (fore & SC_G1))
        {
            fore &= ~(SC_ASCII|SC_G0|SC_G1|SC_GRAPHICS);
            fore |= SC_G1;
            beSetTextAttributes(fore, back);
        }
        End += 1;
        break;

    /* SI      15      Invoke G0 character set. */
    case 15:
        beGetTextAttributes(&fore, &back);
        if (! (fore & SC_G0))
        {
            fore &= ~(SC_ASCII|SC_G0|SC_G1|SC_GRAPHICS);
            fore |= SC_G0;
            beSetTextAttributes(fore, back);
        }
        End += 1;
        break;

    /* CAN     24      Cancel escape sequence and display checkerboard. BUGBUG - not imp'd.
     * SUB     26      Same as CAN.
     */
    case 24:
        End += 1;
        break;

    /* ESC     27      Introduce a control sequence. */
    case 27:
        End = ProcessEscape(Start);
        break;

    /* Print any other control character received. */
    default:
        {
            char buf[4];
            sprintf(buf,"^%c",'A' + cBuffer[Start] - 1);
            beRawTextOut(buf, 2);
            End += 1;
        }
        break;
    }

    return(End);
}


/* ProcessBuffer -
 * 
 * Process the current contents of the terminal character buffer.
 */
static int ProcessBuffer(void)
{
    int Start=0,End=0;

    /* Null-terminate the buffer. Why? Heck, why not? */

    cBuffer[BufLen] = '\0'; 

    /* Loop through the entire buffer. Start will be incremented
     * to point at the start of unprocessed text in the buffer as
     * we go.
     */
    while (Start < BufLen)
    {
        End = Start;

        /* Since we null-terminated, null < 27 and we have a termination condition */
        while ((cBuffer[End] > 27)||(cBuffer[End] == 10)||(cBuffer[End] == 13))
            End++;

        /* At this point, if Start < End, we have a string of characters which
         * doesn't have any control sequences, and should be printed raw.
         */

        if (End > Start)
            beRawTextOut(cBuffer+Start, End-Start);

        if (End >= BufLen)
        {
            break;
        }

        /* At this point, 'End' points to the beginning of an escape
         * sequence. We'll reset 'start' to be AFTER parsing the
         * escape sequence. Note that if the escape sequence
         * is incomplete, ProcessControl should return the
         * same value passed in. Otherwise, it'll return the
         * index of the first character after the valid
         * escape sequence.
         */

        Start = ProcessControl(End);

        if (Start == End)
        {
            /* The escape sequence was incomplete.
             * Move the unprocessed portion of the input buffer
             * to start at the beginning of the buffer, then 
             * return.
             */

            while (End < BufLen)
            {
                cBuffer[End-Start] = cBuffer[End];
                End += 1;
            }

            BufLen = End-Start;
            return(0);
        }
    }

    /* If we made it this far, Start == Buflen, and so we've finished
     * processing the buffer completely.
     */
    BufLen = 0;
    cBuffer[BufLen] = '\0';
         
    return(0);
}


/* vtProcessedTextOut -
 *
 * Output characters to terminal, passing them through the vt100 emulator.
 * Return -1 on error 
 */
int 
vtProcessedTextOut(char *cbuf, int count)
{
    /* If we have a buffer overflow, error out if we haven't already crashed. */

    if ((count + BufLen) > MAXVTBUFLEN)
    {
        beRawTextOut("ERROR: VT-100 internal buffer overflow!",39);
        return(-1);
    }

    /* Otherwise, add our requested information to the
     * output buffer, and attempt to parse it.
     */

    memcpy(cBuffer + BufLen, cbuf, count);
    BufLen += count;

    return(ProcessBuffer());
}

