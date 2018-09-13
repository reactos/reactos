/*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*/

#include "precomp.h"
#pragma hdrstop
#pragma hdrstop
#define FOREGROUND_WHITE (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED)

typedef
BOOL
(*PTEST_ROUTINE)(
    IN HANDLE Handle
    );

BOOL
Scroll1(
    IN HANDLE Handle
    );
BOOL
Scroll2(
    IN HANDLE Handle
    );
BOOL
Scroll3(
    IN HANDLE Handle
    );
BOOL
Scroll4(
    IN HANDLE Handle
    );
BOOL
Scroll5(
    IN HANDLE Handle
    );

#define NUM_TESTS 5
PTEST_ROUTINE Tests[NUM_TESTS] = {Scroll1,Scroll2,Scroll3,Scroll4,Scroll5};

BOOL
Scroll5(
    IN HANDLE Handle
    )
{
    SMALL_RECT ScrollRectangle;
    CHAR_INFO Fill;
    COORD DestinationOrigin;
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;

    if (!GetConsoleScreenBufferInfo(Handle,&ScreenInfo)) {
        DbgPrint("ERROR: GetConsoleScreenBufferInfo failed\n");
    }

    ScrollRectangle = ScreenInfo.srWindow;
    Fill.Attributes = BACKGROUND_GREEN;
    Fill.Char.UnicodeChar = 'T';
    DestinationOrigin.X = 0;
    DestinationOrigin.Y = -(ScreenInfo.srWindow.Bottom - ScreenInfo.srWindow.Top + 1);
    if (!ScrollConsoleScreenBuffer(Handle,
                                   &ScrollRectangle,
                                   NULL,
                                   DestinationOrigin,
                                   &Fill)) {
        DbgPrint("ScrollConsoleScreenBuffer failed\n");
        return FALSE;
    }
    DbgPrint("scrolled entire screen\n");
    DbgBreakPoint();
    return TRUE;
}

BOOL
Scroll4(
    IN HANDLE Handle
    )
{
    CHAR_INFO CharBuffer[20][25];
    SMALL_RECT WriteRegion;
    COORD BufferSize;
    COORD BufferCoord;
    CHAR TextAttr;
    CHAR BackgroundAttr;
    int i,j;
    CHAR Char;
    SMALL_RECT ScrollRectangle,ClipRectangle;
    CHAR_INFO Fill;
    COORD DestinationOrigin;

    BackgroundAttr = BACKGROUND_BLUE;
    TextAttr = FOREGROUND_WHITE;
    for (i=0;i<20;i++) {
        for (Char='a',j=0;j<25;j++) {
            CharBuffer[i][j].Attributes = BackgroundAttr | TextAttr;
            CharBuffer[i][j].Char.AsciiChar = Char;
            Char++;
            if (Char > 'z') Char = 'a';
        }
    }

    //
    // write square to screen
    //

    BufferSize.X = 25;
    BufferSize.Y = 20;
    BufferCoord.X = 0;
    BufferCoord.Y = 0;
    WriteRegion.Left = 5;
    WriteRegion.Right = 15;
    WriteRegion.Top = 5;
    WriteRegion.Bottom = 15;
    if (!WriteConsoleOutput(Handle,
                            (PCHAR_INFO)CharBuffer,
                            BufferSize,
                            BufferCoord,
                            &WriteRegion
                           )) {
        DbgPrint("WriteConsoleOutput failed\n");
        return FALSE;
    }
    if (WriteRegion.Left != 5 || WriteRegion.Right != 15 ||
        WriteRegion.Top != 5 || WriteRegion.Bottom != 15) {
        DbgPrint("test 1: regions don't match\n");
        DbgPrint("WriteRegion is %ld\n",&WriteRegion);
    }
    DbgPrint("wrote rectangle to 5,5\n");
    DbgBreakPoint();

    ScrollRectangle.Left = 5;
    ScrollRectangle.Right = 15;
    ScrollRectangle.Top = 5;
    ScrollRectangle.Bottom = 15;
    ClipRectangle.Left = 9;
    ClipRectangle.Right = 25;
    ClipRectangle.Top = 9;
    ClipRectangle.Bottom = 25;
    Fill.Attributes = BACKGROUND_GREEN;
    Fill.Char.UnicodeChar = 'T';
    DestinationOrigin.X = 20;
    DestinationOrigin.Y = 20;
    if (!ScrollConsoleScreenBuffer(Handle,
                                   &ScrollRectangle,
                                   &ClipRectangle,
                                   DestinationOrigin,
                                   &Fill)) {
        DbgPrint("ScrollConsoleScreenBuffer failed\n");
        return FALSE;
    }
    DbgPrint("scrolled region (5,15)(5,15) to (20,20) with clip (9,9)(25,25)\n");
    DbgBreakPoint();
    return TRUE;
}

BOOL
Scroll3(
    IN HANDLE Handle
    )
{
    CHAR_INFO CharBuffer[20][25];
    SMALL_RECT WriteRegion;
    COORD BufferSize;
    COORD BufferCoord;
    CHAR TextAttr;
    CHAR BackgroundAttr;
    int i,j;
    CHAR Char;
    SMALL_RECT ScrollRectangle,ClipRectangle;
    CHAR_INFO Fill;
    COORD DestinationOrigin;

    BackgroundAttr = BACKGROUND_BLUE;
    TextAttr = FOREGROUND_WHITE;
    for (i=0;i<20;i++) {
        for (Char='a',j=0;j<25;j++) {
            CharBuffer[i][j].Attributes = BackgroundAttr | TextAttr;
            CharBuffer[i][j].Char.AsciiChar = Char;
            Char++;
            if (Char > 'z') Char = 'a';
        }
    }

    //
    // write square to screen
    //

    BufferSize.X = 25;
    BufferSize.Y = 20;
    BufferCoord.X = 0;
    BufferCoord.Y = 0;
    WriteRegion.Left = 5;
    WriteRegion.Right = 15;
    WriteRegion.Top = 5;
    WriteRegion.Bottom = 15;
    if (!WriteConsoleOutput(Handle,
                            (PCHAR_INFO)CharBuffer,
                            BufferSize,
                            BufferCoord,
                            &WriteRegion
                           )) {
        DbgPrint("WriteConsoleOutput failed\n");
        return FALSE;
    }
    if (WriteRegion.Left != 5 || WriteRegion.Right != 15 ||
        WriteRegion.Top != 5 || WriteRegion.Bottom != 15) {
        DbgPrint("test 1: regions don't match\n");
        DbgPrint("WriteRegion is %ld\n",&WriteRegion);
    }
    DbgPrint("wrote rectangle to 5,5\n");
    DbgBreakPoint();

    ScrollRectangle.Left = 5;
    ScrollRectangle.Right = 15;
    ScrollRectangle.Top = 5;
    ScrollRectangle.Bottom = 15;
    ClipRectangle.Left = 5;
    ClipRectangle.Right = 15;
    ClipRectangle.Top = 5;
    ClipRectangle.Bottom = 15;
    Fill.Attributes = BACKGROUND_GREEN;
    Fill.Char.UnicodeChar = 'T';
    DestinationOrigin.X = 5;
    DestinationOrigin.Y = 4;
    if (!ScrollConsoleScreenBuffer(Handle,
                                   &ScrollRectangle,
                                   &ClipRectangle,
                                   DestinationOrigin,
                                   &Fill)) {
        DbgPrint("ScrollConsoleScreenBuffer failed\n");
        return FALSE;
    }
    DbgPrint("scrolled region (5,15)(5,15) to (5,4)\n");
    DbgBreakPoint();
    return TRUE;
}

BOOL
Scroll2(
    IN HANDLE Handle
    )
{
    CHAR_INFO CharBuffer[20][25];
    SMALL_RECT WriteRegion;
    COORD BufferSize;
    COORD BufferCoord;
    CHAR TextAttr;
    CHAR BackgroundAttr;
    int i,j;
    CHAR Char;
    SMALL_RECT ScrollRectangle,ClipRectangle;
    CHAR_INFO Fill;
    COORD DestinationOrigin;

    BackgroundAttr = BACKGROUND_BLUE;
    TextAttr = FOREGROUND_WHITE;
    for (i=0;i<20;i++) {
        for (Char='a',j=0;j<25;j++) {
            CharBuffer[i][j].Attributes = BackgroundAttr | TextAttr;
            CharBuffer[i][j].Char.AsciiChar = Char;
            Char++;
            if (Char > 'z') Char = 'a';
        }
    }

    //
    // write square to screen
    //

    BufferSize.X = 25;
    BufferSize.Y = 20;
    BufferCoord.X = 0;
    BufferCoord.Y = 0;
    WriteRegion.Left = 5;
    WriteRegion.Right = 15;
    WriteRegion.Top = 5;
    WriteRegion.Bottom = 15;
    if (!WriteConsoleOutput(Handle,
                            (PCHAR_INFO)CharBuffer,
                            BufferSize,
                            BufferCoord,
                            &WriteRegion
                           )) {
        DbgPrint("WriteConsoleOutput failed\n");
        return FALSE;
    }
    if (WriteRegion.Left != 5 || WriteRegion.Right != 15 ||
        WriteRegion.Top != 5 || WriteRegion.Bottom != 15) {
        DbgPrint("test 1: regions don't match\n");
        DbgPrint("WriteRegion is %ld\n",&WriteRegion);
    }
    DbgPrint("wrote rectangle to 5,5\n");
    DbgBreakPoint();

    ScrollRectangle.Left = 5;
    ScrollRectangle.Right = 15;
    ScrollRectangle.Top = 5;
    ScrollRectangle.Bottom = 15;
    ClipRectangle.Left = 10;
    ClipRectangle.Right = 25;
    ClipRectangle.Top = 10;
    ClipRectangle.Bottom = 25;
    Fill.Attributes = BACKGROUND_GREEN;
    Fill.Char.UnicodeChar = 'T';
    DestinationOrigin.X = 10;
    DestinationOrigin.Y = 10;
    if (!ScrollConsoleScreenBuffer(Handle,
                                   &ScrollRectangle,
                                   NULL,
                                   DestinationOrigin,
                                   &Fill)) {
        DbgPrint("ScrollConsoleScreenBuffer failed\n");
        return FALSE;
    }
    DbgPrint("scrolled region (5,15)(5,15) to (10,10)\n");
    DbgBreakPoint();
    return TRUE;
}

BOOL
Scroll1(
    IN HANDLE Handle
    )
{
    CHAR_INFO CharBuffer[20][25];
    SMALL_RECT WriteRegion;
    COORD BufferSize;
    COORD BufferCoord;
    CHAR TextAttr;
    CHAR BackgroundAttr;
    int i,j;
    CHAR Char;
    SMALL_RECT ScrollRectangle,ClipRectangle;
    CHAR_INFO Fill;
    COORD DestinationOrigin;

    BackgroundAttr = BACKGROUND_BLUE;
    TextAttr = FOREGROUND_WHITE;
    for (i=0;i<20;i++) {
        for (Char='a',j=0;j<25;j++) {
            CharBuffer[i][j].Attributes = BackgroundAttr | TextAttr;
            CharBuffer[i][j].Char.AsciiChar = Char;
            Char++;
            if (Char > 'z') Char = 'a';
        }
    }

    //
    // write square to screen
    //

    BufferSize.X = 25;
    BufferSize.Y = 20;
    BufferCoord.X = 0;
    BufferCoord.Y = 0;
    WriteRegion.Left = 0;
    WriteRegion.Right = 10;
    WriteRegion.Top = 0;
    WriteRegion.Bottom = 10;
    if (!WriteConsoleOutput(Handle,
                            (PCHAR_INFO)CharBuffer,
                            BufferSize,
                            BufferCoord,
                            &WriteRegion
                           )) {
        DbgPrint("WriteConsoleOutput failed\n");
        return FALSE;
    }
    if (WriteRegion.Left != 0 || WriteRegion.Right != 10 ||
        WriteRegion.Top != 0 || WriteRegion.Bottom != 10) {
        DbgPrint("test 1: regions don't match\n");
        DbgPrint("WriteRegion is %ld\n",&WriteRegion);
    }
    DbgPrint("wrote rectangle to 0,0\n");
    DbgBreakPoint();

    ScrollRectangle.Left = 0;
    ScrollRectangle.Right = 10;
    ScrollRectangle.Top = 0;
    ScrollRectangle.Bottom = 10;
    ClipRectangle.Left = 25;
    ClipRectangle.Right = 30;
    ClipRectangle.Top = 25;
    ClipRectangle.Bottom = 30;
    Fill.Attributes = BACKGROUND_GREEN;
    Fill.Char.UnicodeChar = 'T';
    DestinationOrigin.X = 20;
    DestinationOrigin.Y = 20;
    if (!ScrollConsoleScreenBuffer(Handle,
                                   &ScrollRectangle,
                                   &ClipRectangle,
                                   DestinationOrigin,
                                   &Fill)) {
        DbgPrint("ScrollConsoleScreenBuffer failed\n");
        return FALSE;
    }
    DbgPrint("scrolled region (0,10)(0,10) to (20,20)\n");
    DbgBreakPoint();
    return TRUE;
}


DWORD
main(
    int argc,
    char *argv[]
    )
{
    SECURITY_ATTRIBUTES SecurityAttributes;
    HANDLE Handle;
    int i;

    DbgPrint( "TSCROLL: Entering Scrolling Test Program\n" );

    for (i=0;i<NUM_TESTS;i++) {

        if (argc==2) {
            i = atoi(argv[1]);
        }

        //
        // open a new console
        //

        SecurityAttributes.bInheritHandle = FALSE;
        SecurityAttributes.lpSecurityDescriptor = NULL;
        SecurityAttributes.nLength = sizeof (SECURITY_ATTRIBUTES);
        Handle = CreateFile("CONOUT$",
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            &SecurityAttributes,
                            CREATE_ALWAYS,
                            0,
                            (HANDLE)-1
                           );
        if (Handle == (HANDLE)-1) {
            DbgPrint("CreateFile failed\n");
            return FALSE;
        }

        //
        // make it current
        //

        if (!SetConsoleActiveScreenBuffer(Handle)) {
            DbgPrint("SetConsoleActiveScreenBuffer failed\n");
            return FALSE;
        }

        if (!Tests[i](Handle)) {
            DbgPrint("Scroll%d failed\n",i+1);
        }
        else {
            DbgPrint("Scroll%d succeeded\n",i+1);
        }

        if (!SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE))) {
            DbgPrint("SetConsoleActiveScreenBuffer failed\n");
            return FALSE;
        }

        if (!CloseHandle(Handle)) {
            DbgPrint("CloseHandle failed\n");
            return FALSE;
        }
        if (argc==2) {
            break;
        }
    }

    DbgPrint( "TCON: Exiting Test Program\n" );

    return TRUE;
}
