08/08/96 - John L. Miller, johnmil@cs.cmu.edu, johnmil@jprc.com

FILES INCLUDED:
        00readme.txt - this file
        VT100.H      - Definitions for VT-100 emulator.
        VT100.C      - Front end parsing code for VT-100 emulator
        CONSOLE.C    - Back-end code to allow VT-100 in WinNt/Win95 console

Many UNIX users take terminals for granted, as something you get for free
with the operating system. Unfortunately, this isn't the case for many
non-unix operating systems, especially PC-based ones. After a number of
projects, I decided it would be nice if there was source publicly available
for doing VT-100 emulation.

The files included with this distribution are not a complete implementation
of VT-100 terminal emulation, but do provide complete enough coverage to
use many vt-100 functions over the network. For instance, its enough to
use EMACS to edit, or to connect up to your favorite mud with ANSI color
and graphics characters.

The VT-100 emulator is broken into two parts. The first is the front end,
vt100.c and vt100.h. These files were written to be fairly device-independant,
though admittedly if you're running under a 16-bit operating system instead
of a 32-bit, you might need to change some of the 'int' values to 'long.'
Otherwise, it should work 'as-is'.

The second part is a back-end. The back-end is responsible for doing the
workhorse activities. The front-end parses a character stream, and decides
whether to clear a part of the screen, or move the cursor, or switch fonts.
Then it calls routines in the back-end to perform these activities.

The back-end functions are, for the most part, very straight forward, and
quite easy to implement compared to writing a vt-100 emulator from scratch.
CONSOLE.C is a back-end for use in console (command, dos) windows under
Windows 95 and Windows NT. This console vt-100 emulator is also being used
in my TINTIN-III port and kerberized encrypted telnet port.


TO USE THIS VT-100 EMULATOR:

First, it's intended to be linked directly into source code. You'll need
to change printf's and puts' in your source code to call vtprintf() and
vtputs() instead. You can add additional functions to vt100.c as you see
fit to handle other output functions like putchar() and write(). Another
routine you may want to use is vtProcessedTextOut(), which accepts a
buffer to output, and a count of characters in that buffer.

Second, you need to make sure that your source code calls vtInitVT100()
before it does ANYTHING else. This initializes the vt-100 emulator.

Third, if you want to use this VT-100 emulator with anything besides
Windows NT and Windows 95 consoles, you'll need to implement your own
back end. The list of functions you will need to supply, as well as what
they need to do is contained in vt100.h. The list (minus descriptions)
is as follows:

    int beInitVT100Terminal();
    int beAbsoluteCursor(int row, int col);
    int beOffsetCursor(int row, int column);
    int beRestoreCursor(void);
    int beSaveCursor(void);
    int beSetTextAttributes(int fore, int back);
    int beRawTextOut(char *text, int len);
    int beEraseText(int rowFrom, int colFrom, int rowTo, int colTo);
    int beDeleteText(int rowFrom, int colFrom, int rowTo, int colTo);
    int beInsertRow(int row);
    int beTransmitText(char *text, int len);
    int beAdvanceToTab(void);
    int beClearTab(int col);
    int beSetScrollingRows(int fromRow, int toRow);
    int beRingBell(void);
    int beGetTermMode();
    int beSetTermMode(int newMode);

For details on what each of these does, read the descriptions of each
function included in vt100.h, and read over CONSOLE.C for examples. I've
included copious comments in all of these files to try to make them as
easy to use as possible.

In any case, it should be easier than writing a VT-100 emulator from
scratch.

KNOWN BUGS -

o Many features of VT-100 emulation aren't implemented. This includes
  support for graphics character set 0 and many of the
  answerback functions.

Well, good luck!

