/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    Cmdexec2.c

Abstract:

    This file contains the commands for examining and modifying
    debuggee data - memory and registers.

Author:

    Kent Forschmiedt (a-kentf) 20-Jul-92

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

//Prototypes

BOOL mismatch (ADDR addr0,LPBYTE lpBuf0,ADDR addr1,LPBYTE lpBuf1,int len);
BOOL bDisplayReg(RD);

BOOL
StringLogger(
    LPCSTR szStr,
    BOOL fFileLog,
    BOOL fSendRemote,
    BOOL fPrintLocal
    );


/************************** Data declaration    *************************/

/****** Publics ********/

extern LPPD    LppdCommand;
extern LPTD    LptdCommand;

extern ULONG   ulPseudo[];

/****** Locals ********/

static LPSTR lpszLastSearch = NULL;

struct dmfi {
    DWORD   cBytes;
    FMTTYPE fmtType;
    UINT    radix;
    UINT    cAcross;
    UINT    cchItem;
    UINT    cItems;
};

static  struct dmfi dmfi[] = {
// cBytes fmtType radix cAcross cchItem cItems
    { 1, fmtAscii,   0,   32,     1,    32*8},     // 0
    { 1, fmtUInt,   16,   16,     2,    16*8},     // 1
    { 2, fmtUInt,   16,    8,     4,     8*8},     // 2
    { 4, fmtUInt,   16,    4,     8,     4*8},     // 3
    { 4, fmtFloat,  10,    1,    14,     1*8},     // 4
    { 8, fmtFloat,  10,    1,    22,     1*8},     // 5
    {10, fmtFloat,  10,    1,    30,     1*8},     // 6
    { 2, fmtUnicode, 0,   32,     1,    32*8},     // 7
    {16, fmtFloat,  10,    1,    30,     1*8},     // 8
};

//
//  Command help text.
//
static char *HelpText[] = {
    "[__GENERAL__]",
    "    To display help for a specific command, enter the following: ",
    " ",
    "        help <command>",
    " ",
    "    Help is available for the following commands: ",
    " ",
    "    ?      Evaluate expression ",
    "    .      Dot command ",
    "    !      Execute function in extension DLL ",
    "    #      Search disassembly for regular expression ",
    "    %%     Change current context to the specified stack frame ",
    "    BA     Set memory breakpoint ",
    "    BC     Clear breakpoints ",
    "    BD     Disable breakpoints ",
    "    BE     Enable breakpoints ",
    "    BL     List breakpoints ",
    "    BP     Set a breakpoint ",
    "    C      Compare memory ranges ",
    "    D      Display memory contents ",
    "    E      Enter memory contents ",
    "    F      Freeze thread ",
    "    FI     Fill memory range ",
    "    FR     Display or modify FP registers ",
    "    G      Go ",
    "    GH     Go - exception handled ",
    "    GN     Go - exception not handled ",
    "    HELP   Help ",
    "    K      Display stack trace ",
    "    L      Restart debuggee ",
    "    LD     Defer symbol loading for module ",
    "    LM     List loaded modules",
    "    LMX    List loaded modules, with extended load information ",
    "    LN     List nearest symbol ",
    "    M      Move memory ",
    "    N      Set radix ",
    "    P      Program step ",
    "    Q      Quit debugger",
    "    R      Display or modify register ",
    "    REMOTE Start remote server ",
    "    RT     Toggle register display ",
    "    S      Search memory ",
    "    S+     Enable source mode ",
    "    S-     Disable source mode ",
    "    SE     Sets RIP break or warning level ",
    "    SX     List exception actions ",
    "    SXD    Disable exception actions ",
    "    SXN    Notify on exception ",
    "    SXE    Enable exception ",
    "    T      Trace into ",
    "    U      Unassemble ",
    "    X      Examine symbols ",
    "    Z      Thaw thread ",
    "   <addr>  Address expression ",
    "   <range> Range expression ",
    " ",
    "[ ?     ] ",
    "    ? <expression>      -   Evaluates an expression ",
    "    ?.                  -   Lists local variables for current context ",
    " ",
    "[ .     ] ",
    "    .<command>          -   Executes the specified dot command ",
    " ",
    "    For a list of available dot commands, type \".?\" ",
    " ",
    "[ !     ] ",
    "    ![dll.]<extension>  -   Executes the extension function in the specified extension DLL",
    " ",
    "[ #     ] ",
    "    # [pattern]         -   Searches for a pattern in the disassembly window",
    " ",
    "[ %     ] ",
    "    %%<frame-number>     -   Changes current context to the specified stack frame ",
    "                            Hint: Use the 'KN' command to get the frame number ",
    " ",
    "[ BA    ] ",
    "    BA <access> <size> <addr>  -   Sets a memory breakpoint ",
    " ",
    "[ BC    ] ",
    "    BC <id-list>        -   Clears the specified breakpoint(s) ",
    " ",
    "[ BD    ] ",
    "    BD <id-list>        -   Disables the specified breakpoint(s) ",
    " ",
    "[ BE    ] ",
    "    BE <id-list>        -   Enables the specified breakpoint(s) ",
    " ",
    "[ BL    ] ",
    "    BL <id-list>        -   Lists the specified breakpoints ",
    " ",
    "[ BP    ] ",
    "    BP[id] <condition> [<options>]    - Sets a breakpoint ",
    " ",
    "        id         - Assign given identifier to the breakpoint ",
    "        condition  - Set the break condition as follows: ",
    " ",
    "            [context]@<line>    - Break at source line ",
    "            ?<expression>       - Break if expression is true ",
    "            =<addr> [/R<size>]  - Break if memory has changed ",
    " ",
    "            [context]<addr> [msg-condition] \",  - Break at address ",
    " ",
    "              msg-condition - ",
    "                /M<msg-name>    - Check for message by name ",
    "                /M<msg-class>   - Check for message by class ",
    " ",
    "                   msg-class is a combination of the following: ",
    "                           M - Mouse ",
    "                           W - Window ",
    "                           N - Input ",
    "                           S - System ",
    "                           I - Init ",
    "                           C - Clipboard ",
    "                           D - DDE ",
    "                           Z - Nonclient ",
    " ",
    "        options    - Set options as follows: ",
    " ",
    "            /P<count>           - Skip first <count> times ",
    "            /Q                  - Suppress unresolved BP dialog box",
    "            /H<number>          - Attach BP to process <number> ",
    "            /T<number>          - Attach BP to thread <number> ",
    "            /C\"<cmd-list>\"      - Execute <cmd-list> when hit ",
    " ",
    "[ C     ] ",
    "    C [range] [addr]    -   Compare memory ranges",
    " ",
    "[ D     ] ",
    "    D<mode> <addr> | <range>    -   Displays memory contents",
    " ",
    "        mode    -   Can be one of the following: ",
    " ",
    "            C   -   Code (disassembly) ",
    "            A   -   ASCII characters",
    "            U   -   Unicode characters",
    "            B   -   Byte value (char) ",
    "            W   -   Word value",
    "            D   -   Double-word value",
    "            S   -   4-byte real value ",
    "            I   -   8-byte real value",
    "            T   -   10-byte real value",
    " ",
    "[ E     ] ",
    "    E<mode> <addr> [value-list] -   Enter memory contents",
    " ",
    "        mode    -   Can be one of the following: ",
    " ",
    "            A   -   ASCII ",
    "            U   -   Unicode ",
    "            B   -   Byte ",
    "            W   -   Word ",
    "            D   -   Doubleword ",
    "            S   -   4-byte real ",
    "            I   -   8-byte real ",
    "            T   -   10-byte real ",
    " ",
    "[ F     ] ",
    "[ Z     ] ",
    "    ~[.|*|<id>]F    -   Freezes the specified thread ",
    "    ~[.|*|<id>]Z ",
    " ",
    "[ FI    ] ",
    "    FI<mode> <range> <value-list>   -   Fills the memory range",
    " ",
    "        mode    -   Can be one of the following ",
    " ",
    "            A   -   ASCII ",
    "            U   -   Unicode ",
    "            B   -   Byte ",
    "            W   -   Word ",
    "            D   -   Doubleword ",
    "            S   -   4-byte real ",
    "            I   -   8-byte real ",
    "            T   -   10-byte real ",
    " ",
    "[ FR    ] ",
    "    FR [<reg>[=<value>]]    -   Displays or modifies a floating-point register ",
    " ",
    "[ G     ] ",
    "    G   [=<start>] [break]  -   Starts execution at entry and continues to break ",
    " ",
    "    GH                      -   Go, mark exception as handled ",
    " ",
    "    GN                      -   Go, mark exception as not handled ",
    " ",
    "[ Help  ] ",
    "   HELP [command]  -   Displays this list ",
    " ",
    "       Displays help for a specific command. For a list of all ",
    "       available commands, omit the command argument. ",
    " ",
    "[ K     ] ",
    "    K<BSVNT> [=frameaddr stackaddr programcounter] [<frames>]     -   Displays a stack trace ",
    " ",
    "        B   -   Stack trace includes 3 double-word values from the stack ",
    "        S   -   Stack trace includes source file and line numbers ",
    "        V   -   Stack trace includes run-time function information ",
    "                FPO [params, locals, registers] ",
    "        N   -   Stack trace includes frame numbers ",
    "        T   -   Stack trace includes the column header titles ",
    " ",
    "[ L     ] ",
    "    L   -   Restarts the debuggee ",
    " ",
    "[ LD    ] ",
    "    LD <module-name>    -   Defers symbol loading for the specified module ",
    " ",
    "[ LM    ] ",
    "    LM [/f|/s [/o]] [module-name]  - Lists loaded-module information ",
    " ",
    "        f   -   Lists flat modules ",
    "        s   -   Lists segmented modules ",
    "        o   -   Sorts segmented modules by selector ",
    " ",
    "[ LMX   ] ",
    "    LMX [/f|/s [/o]] [module-name]  - Lists loaded modules, with symbol-load information ",
    " ",
    "        f   -   Lists flat modules ",
    "        s   -   Lists segmented modules ",
    "        o   -   Sorts segmented modules by selector ",
    " ",
    "[ LN    ] ",
    "    LN <addr>   -   Lists the nearest symbol ",
    " ",
    "[ M     ] ",
    "    M <range> <addr>    -   Moves memory ",
    " ",
    "[ N     ] ",
    "    N<radix>    -   Sets the default radix ",
    " ",
    "        radix - Can be 8, 10, or 16 ",
    " ",
    "[ P     ] ",
    "    P [repeat]  -   Executes the specified number of instructions ",
    " ",
    "[ Q     ] ",
    "    Q           -   Quits WinDbg ",
    " ",
    "[ R     ] ",
    "    R [<reg>[=<value>]]    -   Displays or modifies the specified register ",
    " ",
    "[ REMOTE ] ",
    "    REMOTE <pipe name>  -   Starts the remote server for <pipe name> ",
    "    REMOTE              -   Displays the remote server status ",
    "    REMOTE stop         -   Terminates the remote server ",
    " ",
    "[ S     ] ",
    "    S <range> <pattern>  -   Searches memory for the specified pattern ",
    " ",
    "[ S+    ] ",
    "[ S-    ] ",
    "    S+  -   Enables source-mode debugging ",
    "    S-  -   Disables source-mode debugging ",
    " ",
    "[ SE    ] ",
    "    SE<B|W> [0-3]   -   Sets the RIP break or warning level ",
    " ",
    "        B   -   Sets the break level ",
    "        W   -   Sets the warning level ",
    " ",
    "[ SX    ] ",
    "    SX  -   Lists exception actions ",
    " ",
    "[ SXD   ] ",
    "    SXD <exception> [name]  -   Disables the specified exception action ",
    " ",
    "[ SXN   ] ",
    "    SXN <exception> [name]  -   Notifies on exception ",
    " ",
    "[ SXE   ] ",
    "    SXE <exception> [/C cmd] [name] - Enables exception action ",
    " ",
    "[ T     ] ",
    "    T [repeat]  -   Traces into subroutine ",
    " ",
    "[ U     ] ",
    "    U <addr>   - Unassembles instruction(s) ",
    " ",
    "[ X     ] ",
    "    X<scope> [context]<pattern>    - Find symbols within the given scope that",
    "                                     match the specified pattern ",
    " ",
    "        scope    -   A combination of the following values: ",
    " ",
    "            L   -   Lexical ",
    "            F   -   Function ",
    "            C   -   Class ",
    "            M   -   Module ",
    "            E   -   Exe ",
    "            P   -   Public ",
    "            G   -   Global ",
    "            *   -   All ",
    " ",
    "    Note:   The command \"x*!\" lists all loaded modules ",
    "            (it is equivalent to the LM command)",
    " ",
    "[ <addr> ] ",
    "    Any valid expression may be used where an address ",
    "    is required. WinDbg recognizes standard C/C++ expressions, ",
    "    so any of the following are syntactically valid: ",
    " ",
    "            0x55000 ",
    "            MyDll!MyFunc ",
    "            szBuffer+6 ",
    " ",
    "[ <range> ] ",
    "    A range expression is one of the following: ",
    " ",
    "            <addr1> <addr2> ",
    "               Addresses from <addr1> to <addr2>, inclusive ",
    " ",
    "            <addr> L <count> ",
    "               Addresses from <addr> extending for <count> items. The ",
    "               size of the item is determined by the command; for ",
    "               example, the dd command has a size of 4, while db has ",
    "               an item size of 1. "
    " ",
    "            <addr> I <integer expression> ",
    "               Specifies a count in instructions rather than bytes ",
    "               (only valid for the U command) ",
    " ",
    "[__END__]"
};


#define IsTopicHeader(p)    (*(p) == '[')



// These are always stored in fixed-up form, never module relative.
static  ADDR    addrLastDumpStart;
static  ADDR    addrLastDumpEnd;
static  ADDR    addrLastEnterStart;
static  ADDR    addrLastEnterEnd;

ADDR    addrLastDisasmStart;
static  ADDR    addrLastDisasmEnd;

static  ADDR    addrAsm;          // for assemble and enter commands
static  int     nEnterType;       // data type for interactive enter


/****** Externs from ??? *******/

extern  CXF      CxfIp;

/**************************       Code          *************************/



/******************************************************************************
 *
 * Functions for displaying help
 *
 ******************************************************************************/

BOOL
TopicMatch (
    char *  Header,
    char *  Topic
    )
{
    char    HdrBuf[ MAX_PATH ];
    char    TopicBuf[ MAX_PATH ];
    char   *p;
    size_t  i;
    BOOL    Match = FALSE;

    i = strspn( Header, "[ \t" );
    p = Header+i;
    strcpy( HdrBuf, p );
    p = strpbrk( HdrBuf, " \t]" );

    if ( p ) {
        *p = 0;

        i = strspn( Topic, " \t" );
        p = Topic+i;
        strcpy( TopicBuf, p );
        p = strpbrk( TopicBuf, " \t" );

        if ( p ) {
            *p = 0;
        }

        Match = !_stricmp( HdrBuf, TopicBuf );
    }

    return Match;
}


char **
FindTopic(
    LPSTR   Topic
    )
{
    char **Text = HelpText;

    while ( Text ) {
        if ( IsTopicHeader( *Text ) ) {

            if ( TopicMatch( *Text, "__END__" ) ) {

                Text = NULL;
                break;

            } else if ( TopicMatch( *Text, Topic ) ) {

                while ( IsTopicHeader( *Text ) )  {
                    Text++;
                }
                break;
            }
        }
        Text++;
    }

    return Text;
}


BOOL
DisplayTopic(
    LPSTR   Topic
    )
{
    char **TopicText;
    BOOL    Found = FALSE;

    TopicText = FindTopic( Topic );

    if ( TopicText ) {
        Found = TRUE;
        while ( !IsTopicHeader( *TopicText ) ) {
            CmdLogFmt( *TopicText );
            CmdLogFmt( "\r\n" );
            TopicText++;
        }
    }

    return Found;
}


BOOL
CmdHelp (
    LPSTR   Topic
    )
/*++

Routine Description:

    Displays help, either general or for a specific topic

Arguments:

    Topic   - Supplies topic (NULL for general help)

Return Value:

    FALSE if topic not found.

--*/
{
    BOOL    Found = TRUE;

    if ( Topic ) {

        if ( !DisplayTopic( Topic) ) {
            CmdLogFmt( "No help available on %s\r\n", Topic );
            Found = FALSE;
        }

    } else {

        DisplayTopic( "__GENERAL__" );
    }

    return Found;
}



/******************************************************************************
 *
 * Helper and common functions
 *
 ******************************************************************************/

void
CmdAsmPrompt(
    BOOL fRemote,
    BOOL fLocal
    )
/*++

Routine Description:

    Print the address prompt string for assembler input.
    Set the printed area readonly.

Arguments:

    None

Return Value:

    None

--*/
{
    char szStr[100];
    uint flags;

    if (g_contWorkspace_WkSp.m_bShowSegVal) {
        flags = EEFMT_SEG;
    } else {
        flags = EEFMT_32;
        SYFixupAddr( &addrAsm );
    }

    CmdInsertInit();

    EEFormatAddress( &addrAsm, szStr, sizeof(szStr), flags);
    strcat(szStr, "> ");

    StringLogger(szStr, TRUE, fRemote, fLocal );
}                       /* CmdAsmPrompt */


BOOL
CmdAsmLine(
    LPSTR lpsz
    )
/*++

Routine Description:

    Assemble one line.

Arguments:

    lpsz    - Supplies string containing line to be assembled

Return Value:

    Always TRUE; this is a synchronous command.

--*/
{
    XOSD    xosd;

    /*
    ** We got only the part of the line that the user typed
    */

    lpsz = CPSkipWhitespace(lpsz);

    /*
    **  Check for blank line
    */

    AuxPrintf(0, "Asm: \"%s\"", lpsz);

    lpsz = CPSkipWhitespace( lpsz );
    if (*lpsz == 0) {

        CmdSetDefaultCmdProc();

    } else {

        /*
        **  Assemble up the line
        */

        ADDR addrT = addrAsm;
        xosd = OSDAssemble( LppdCur->hpid, LptdCur->htid, &addrT, lpsz);
        if (xosd != xosdNone) {
            CmdLogVar(ERR_Bad_Assembly);
        } else {
            addrAsm = addrT;
        }
    }
    return TRUE;
}                   /* CmdAsmLine() */


void
CmdEnterPrompt(
    BOOL fRemote,
    BOOL fLocal
    )
/*++

Routine Description:

    Print prompt line for interactive enter

Arguments:

    None

Return Value:

    None

--*/
{
    char bBuf[30];
    char szStr[200];
    LPSTR pStr;
    DWORD cb;
    XOSD xosd;

    CmdInsertInit();

    pStr = szStr;

    EEFormatAddress(&addrAsm, pStr, (DWORD)(sizeof(szStr) - (pStr - szStr)),
                 g_contWorkspace_WkSp.m_bShowSegVal * EEFMT_SEG);
    strcat(szStr, "  ");
    pStr += strlen(szStr);

    xosd = OSDReadMemory(LppdCur->hpid,
                         LptdCur->htid,
                         &addrAsm,
                         bBuf,
                         dmfi[nEnterType].cBytes,
                         &cb
                         );
    if (xosd != xosdNone) {
        cb = 0;
    }

    if (cb != dmfi[nEnterType].cBytes) {

        memset(pStr, '?', dmfi[nEnterType].cchItem);
        pStr[dmfi[nEnterType].cchItem] = 0;
        pStr += strlen(pStr);

    } else {

        int rdx = dmfi[nEnterType].radix;
        FMTTYPE fmt = dmfi[nEnterType].fmtType;
        int cch = dmfi[nEnterType].cchItem;
        if (rdx == 16) {
            rdx = radix;
            if (rdx == 8) {
                cch = (dmfi[nEnterType].cBytes * 8 + 2) / 3;
            } else if (rdx == 10) {
                // this is close...
                cch = (dmfi[nEnterType].cBytes * 8 + 1) / 3;
            }
        }
        if (rdx == 16 || rdx == 8) {
            fmt = fmt | fmtZeroPad;
        }
        CPFormatMemory(pStr,
                cch + 1,
                (PBYTE) bBuf,
                dmfi[nEnterType].cBytes*8,
                fmt,
                rdx);

        pStr += strlen(pStr);
    }

    strcat(pStr, "> ");
    StringLogger(szStr, TRUE, fRemote, fLocal);
}


BOOL
CmdEnterLine(
    LPSTR lpsz
    )
/*++

Routine Description:

    Handle a line entered by the user in interactive enter mode.  This takes
    one data item at a time, writes it to memory, and increments the memory
    pointer by the size of the data item.

Arguments:

    lpsz  - Supplies string to be parsed into a data item

Return Value:

    Always TRUE, signifying a synchronous command

--*/
{
    int err;

    if (*lpsz == '\0') {
        // empty line - all done.
        CmdSetDefaultCmdProc();
    }
    else if ( *(lpsz = CPSkipWhitespace(lpsz)) == '\0'
            || (*lpsz == '/' && *CPSkipWhitespace(lpsz+1) == '\0'))
    {
        // space(s) or '/' on empty line - keep old value and step
        GetAddrOff(addrAsm) += dmfi[nEnterType].cBytes;
        addrLastEnterEnd = addrAsm;
    }
    else
    {
        err = DoEnterMem(lpsz, &addrAsm, LOG_DM(nEnterType), FALSE);
        if (err == LOGERROR_UNKNOWN) {
            CmdLogVar(ERR_Edit_Failed);
        } else {
            addrLastEnterEnd = addrAsm;
        }
    }

    return TRUE;
}


LOGERR
DoEnterMem(
    LPSTR  lpsz,
    LPADDR lpAddr,
    LOG_DM type,
    BOOL   fMulti
    )
/*++

Routine Description:

    Executive for all styles of E commands.  Takes an argument list, decodes
    it and stores data.

Arguments:

    lpsz    - Supplies text of data to parse
    lpAddr  - Supplies address where data are to be stored
    type    - Supplies type of data expected
    fMulti  - Supplies flag for whether to allow multiple items

Return Value:

    LOGERR   code

--*/
{
    DWORD     cb;
    DWORD     dwcb;
    BYTE      buf[2 * MAX_USER_LINE];
    LOGERR    rVal;
    XOSD      xosd;

    rVal = GetValueList(lpsz, type, fMulti, buf, sizeof(buf), &cb);
    if (rVal == LOGERROR_NOERROR) {

        xosd = OSDWriteMemory(LppdCur->hpid,
                              LptdCur->htid,
                              lpAddr,
                              buf,
                              cb,
                              &dwcb
                              );
        UpdateDebuggerState(UPDATE_DATAWINS);

        if (xosd == xosdNone) {
            GetAddrOff(*lpAddr) += cb;
        } else {
            CmdLogVar(ERR_Edit_Failed);
            rVal = LOGERROR_QUIET;
        }
    }
    return rVal;
}                     /* DoEnterMem() */


LOGERR
GetValueList(
    LPSTR   lpsz,
    LOG_DM  type,
    BOOL    fMulti,
    LPBYTE  lpBuf,
    int     cchBuf,
    PDWORD  pcb
    )
/*++

Routine Description:

    This parses value lists for enter, fill, and search commands.
    This function wants to consume the entire string, and will print
    a message and return an error code if it cannot.

Arguments:

    lpsz   - Supplies string to be parsed

    type   - Supplies data type

    fMulti - Supplies multiple items allowed if TRUE

    lpBuf  - Returns parsed data

    pcb    - returns bytes count of result

Return Value:

    LOGERROR code

--*/
{
    CHAR     chQuote;
    CHAR     szCopyBuf[MAX_USER_LINE];
    CHAR     szMisc[500];
    LPBYTE   lpb;
    int      cch;
    DWORD    cb;
    int      i;
    DWORD    dw;

    //
    // get value list
    //
    switch (type) {
      default:
        return LOGERROR_UNKNOWN;

      case LOG_DM_ASCII:
      case LOG_DM_UNICODE:

        // accept a string of some length
        // M00UNICODE this assumes that console input is ANSI

        chQuote = *lpsz;
        cb = CPCopyString(&lpsz, szCopyBuf, '\\', chQuote == '\'' || chQuote == '"');
        if (cb == 0  ||  *(lpsz = CPSkipWhitespace(lpsz)) != '\0')
        {
            CmdLogVar(ERR_String_Invalid);
            return LOGERROR_QUIET;
        }
        if (chQuote == '"') {
            // include \0 in quoted strings
            cb++;
        }

        if (type == LOG_DM_ASCII) {
            memcpy(lpBuf, szCopyBuf, cb);
        } else {
            mbtowc((WCHAR *)lpBuf, szCopyBuf, cb);
            cb = 2 * MultiByteToWideChar(
                                CP_ACP,
                                0,
                                szCopyBuf,
                                cb,
                                (LPWSTR)lpBuf,
                                cchBuf);
        }

        break;


      case LOG_DM_BYTE:
      case LOG_DM_WORD:
      case LOG_DM_DWORD:
////////////////////////////////////////////////////
//                                                //
//  If you add e.g. LOG_DM_QWORD here, you must   //
//  change the CPGetCastNbr call and the          //
//  decoding code to use the largest integer      //
//  size supported!                               //
//                                                //
////////////////////////////////////////////////////

        lpb = lpBuf;
        i = 0;
        while (cch = CPCopyToken(&lpsz, szCopyBuf)) {

            if (CPGetCastNbr(szCopyBuf,
                             T_LONG,
                             radix,
                             fCaseSensitive,
                             &CxfIp,
                             (LPSTR) &dw,
                             szMisc,
                             g_contWorkspace_WkSp.m_bMasmEval
                             )
                     != CPNOERROR)
            {

                CmdLogVar(ERR_Expr_Invalid);
                return LOGERROR_QUIET;

            } else {

                switch (type) {
                  case LOG_DM_BYTE:
                    *lpb = (BYTE)dw;
                    break;

                  case LOG_DM_WORD:
                    *(WORD *)lpb = (WORD)dw;
                    break;

                  case LOG_DM_DWORD:
                    *(DWORD *)lpb = dw;
                    break;
                }

                i++;
                lpb += dmfi[type].cBytes;

            }
        }

        if (i > 1 && !fMulti) {
            CmdLogVar(ERR_Expr_Invalid);
            return LOGERROR_QUIET;
        }

        cb = i * dmfi[type].cBytes;

        break;
      case LOG_DM_4REAL:
      case LOG_DM_8REAL:
      case LOG_DM_TREAL:
        lpb = lpBuf;
        i = 0;
        while (cch = CPCopyToken(&lpsz, szCopyBuf)) {

            if (CPUnformatMemory(
                     lpb,
                     szCopyBuf,
                     8 * dmfi[type].cBytes,
                     dmfi[type].fmtType,
                     radix )
                != CPNOERROR)
            {

                CmdLogVar(ERR_Expr_Invalid);
                return LOGERROR_QUIET;

            } else {

                i++;
                lpb += dmfi[type].cBytes;

            }
        }

        if (i > 1 && !fMulti) {
            CmdLogVar(ERR_Expr_Invalid);
            return LOGERROR_QUIET;
        }

        cb = i * dmfi[type].cBytes;

        break;

    }

    *pcb = cb;

    return LOGERROR_NOERROR;
}                     /* GetValueList() */


LOG_DM
LetterToType(
    char c
    )
/*++

Routine Description:

    Parser helper function, returns type for d, e, f, s commands.

Arguments:

    c  - character to map to a type code

Return Value:

    type code

--*/
{
    LOG_DM type;

    switch (c) {
      default:
        type = LOG_DM_UNKNOWN;
        break;
      case 'b':
      case 'B':
        type = LOG_DM_BYTE;
        break;
      case 'w':
      case 'W':
        type = LOG_DM_WORD;
        break;
      case 'd':
      case 'D':
        type = LOG_DM_DWORD;
        break;
      case 'i':
      case 'I':
        type = LOG_DM_8REAL;
        break;
      case 's':
      case 'S':
        type = LOG_DM_4REAL;
        break;
      case 't':
      case 'T':
        type = LOG_DM_TREAL;
        break;
      case 'a':
      case 'A':
        type = LOG_DM_ASCII;
        break;
      case 'u':
      case 'U':
        type = LOG_DM_UNICODE;
        break;
    }
    return type;
}


BOOL
mismatch(
    ADDR   addr0,
    LPBYTE lpBuf0,
    ADDR   addr1,
    LPBYTE lpBuf1,
    int    len
    )
/*++

Routine Description:

    Helper for LogCompare().  Print addresses and bytes for any bytes not
    matching in the two buffers.

Arguments:

    addr0   - Supplies debuggee address that data in lpBuf0 came from.

    lpBuf0  - Supplies pointer to first data

    addr1   - Supplies debuggee address that data in lpBuf1 came from.

    lpBuf0  - Supplies pointer to second data

    len     - Supplies number of bytes to compare

Return Value:

    TRUE if buffers didn't match

--*/
{
    int i;
    char sza0[20];
    char sza1[20];
    BOOL fMismatch = FALSE;
    for (i = 0; i < len; i++) {
        if (lpBuf0[i] != lpBuf1[i]) {
            fMismatch = TRUE;
            EEFormatAddress(&addr0, sza0, sizeof(sza0),
                         g_contWorkspace_WkSp.m_bShowSegVal * EEFMT_SEG);
            EEFormatAddress(&addr1, sza1, sizeof(sza1),
                         g_contWorkspace_WkSp.m_bShowSegVal * EEFMT_SEG);
            CmdLogFmt("%s %02x - %s %02x\r\n", sza0, lpBuf0[i], sza1, lpBuf1[i]);
        }
        GetAddrOff(addr0) += 1;
        GetAddrOff(addr1) += 1;
    }
    return fMismatch;
}


//
// miniature stdio
//
typedef struct tagMSTREAM {
    BYTE *_ptr;     // next char in buffer
    int   _cnt;     // chars remaining in buffer
    BYTE *_base;    // base of buffer
    int   _flag;    // error flag
    ADDR  _addr;    // next read address
    int   _len;     // limit; bytes unread
} MSTREAM, FAR * LPMSTREAM;
static MSTREAM mstream;
#define getb(P) (--(P)->_cnt >= 0 ? *((P)->_ptr++) : filb(P))
#define errorb(P) ((P)->_flag)
#define MSE_END   1
#define MSE_FAIL  2

static LPMSTREAM
openb(
    LPADDR lpAddr,
    int    len
    )
/*++

Routine Description:

    Set up for stream input from debuggee memory at lpAddr, for maximum
    of len bytes.

Arguments:

    lpAddr  - Supplies pointer to ADDR struct for start address

    len     - Supplies maximum bytes to read

Return Value:

    None

--*/
{
    mstream._cnt = 0;
    mstream._addr = *lpAddr;
    mstream._flag = 0;
    mstream._len  = len;
    return &mstream;
}


static void
tellb(
    LPMSTREAM lpMstream,
    LPADDR    lpAddr
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    *lpAddr = lpMstream->_addr;
    GetAddrOff(*lpAddr) -= lpMstream->_cnt;
}


static int
filb(
    LPMSTREAM lpMstream
    )
{
    DWORD cb;
    XOSD xosd;

    if (lpMstream->_flag) {
        return -1;
    }
    if (lpMstream->_base == NULL) {
        lpMstream->_base = (PBYTE) malloc(MEMBUF_SIZE);
    }

    cb = lpMstream->_len < MEMBUF_SIZE ? lpMstream->_len : MEMBUF_SIZE;
    if (cb < 1) {
        lpMstream->_flag = MSE_END;
        return -1;
    }
    xosd = OSDReadMemory(LppdCur->hpid,
                         LptdCur->htid,
                         &lpMstream->_addr,
                         lpMstream->_base,
                         cb,
                         &cb
                         );
    if (xosd != xosdNone || cb < 1) {
        lpMstream->_flag = MSE_FAIL;
        return -1;
    }
    GetAddrOff(lpMstream->_addr) += cb;
    lpMstream->_len -= cb;
    lpMstream->_cnt = cb - 1;
    lpMstream->_ptr = lpMstream->_base;

    //
    // Lint fanatics:
    // this is NOT an error!
    //
    return *lpMstream->_ptr++;
}

#ifdef _ALPHA_
/******************************************************************************
 *
 * Command entry points
 *
 ******************************************************************************/

LOGERR
LogAssemble(
    LPSTR lpsz
    )
/*++

Routine Description:

    This is the command used to start the assembler up.

Arguments:

    lpsz - arguments to assemble command

Return Value:

    log error code

--*/
{
    int      cch;
    LPPD     LppdT = LppdCur;
    LPTD     LptdT = LptdCur;
    LOGERR   rVal = LOGERROR_NOERROR;


    PDWildInvalid();
    TDWildInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;

    Assert( cmdView != -1);

    CmdInsertInit();

    if (!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    /*
    **  Check for an address argument
    */

    lpsz = CPSkipWhitespace(lpsz);

    if (*lpsz == 0) {
        OSDGetAddr(LppdCur->hpid, LptdCur->htid, adrPC, &addrAsm);
    }
    else {
        if (CPGetAddress(lpsz,
                         &cch,
                         &addrAsm,
                         radix,
                         &CxfIp,
                         fCaseSensitive,
                         g_contWorkspace_WkSp.m_bMasmEval
                         ) != 0) {
            CmdLogVar(ERR_AddrExpr_Invalid);
            rVal = LOGERROR_QUIET;
            goto done;
        }
        lpsz += cch;
    }

    lpsz = CPSkipWhitespace(lpsz);
    if (*lpsz != 0) {
        rVal = LOGERROR_UNKNOWN;
        goto done;
    }

    /*
    **  We have a working address now set up for doing the the assembly
    */

    // this is happening between the CmdDoLine() and
    // CmdDoPrompt() calls - we will get the right prompt.

    CmdSetCmdProc(CmdAsmLine, CmdAsmPrompt);
    CmdSetAutoHistOK(FALSE);
    CmdSetEatEOLWhitespace(FALSE);


done:
    LppdCur = LppdT;
    LptdCur = LptdT;
    return rVal;
}                   /* LogAssemble() */
#endif


LOGERR
LogCompare(
    LPSTR lpsz
    )
/*++

Routine Description:

    Compare command:
    c <range> <addr>

Arguments:

    lpsz  - command tail

Return Value:

    LOGERROR code

--*/
{
    int      err;
    int      cch;
    int      i;
    ADDR     addr0;
    ADDR     addr1;
    ULONG    Items;
    DWORD    dwLen;
    DWORD    dwBytes;
    DWORD    dwcb;
    LONG     nDW;
    DWORD    buf0[MEMBUF_SIZE/sizeof(DWORD)];
    DWORD    buf1[MEMBUF_SIZE/sizeof(DWORD)];
    BOOL     fMatched;
    LPPD     LppdT = LppdCur;
    LPTD     LptdT = LptdCur;
    LOGERR   rVal = LOGERROR_NOERROR;
    char     szStr[100];
    XOSD     xosd;


    CmdInsertInit();

    PDWildInvalid();
    TDWildInvalid();
    PreRunInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        UpdateDebuggerState(UPDATE_CONTEXT);
    }


    err = CPGetRange(lpsz,
                     &cch,
                     &addr0,
                     &addr1,
                     &Items,
                     radix,
                     0,
                     1,
                     &CxfIp,
                     fCaseSensitive,
                     g_contWorkspace_WkSp.m_bMasmEval,
                     NULL);
    //
    // no default here
    //
    if (err != CPNOERROR) {
        CmdLogVar(ERR_RangeExpr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    if (GetAddrOff(addr1) < GetAddrOff(addr0)) {
        CmdLogVar(ERR_RangeExpr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    dwLen = (DWORD)(GetAddrOff(addr1) - GetAddrOff(addr0));

    lpsz = CPSkipWhitespace(lpsz + cch);
    err = CPGetAddress(lpsz,
                       &cch,
                       &addr1,
                       radix,
                       &CxfIp,
                       fCaseSensitive,
                       g_contWorkspace_WkSp.m_bMasmEval);
    if (err != CPNOERROR) {
        CmdLogVar(ERR_AddrExpr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }
    SYFixupAddr(&addr1);

    fMatched = TRUE;

    while (dwLen) {

        dwBytes = (dwLen < MEMBUF_SIZE)? dwLen : MEMBUF_SIZE;

        xosd = OSDReadMemory( LppdCur->hpid,
                              LptdCur->htid,
                              &addr0,
                              buf0,
                              dwBytes,
                              &dwcb
                              );
        if (xosd != xosdNone || dwcb < 1) {
            EEFormatAddress(&addr0,
                            szStr,
                            sizeof(szStr),
                            g_contWorkspace_WkSp.m_bShowSegVal? EEFMT_SEG : 0
                            );
            CmdLogVar(ERR_Read_Failed_At, szStr);
            rVal = LOGERROR_QUIET;
            break;
        }

        //
        // only read as many bytes as previous read got:
        //
        xosd = OSDReadMemory(LppdCur->hpid,
                             LptdCur->htid,
                             &addr1,
                             buf1,
                             dwcb,
                             &dwcb
                             );
        if (dwcb < 1) {
            EEFormatAddress(&addr1, szStr, sizeof(szStr),
                         g_contWorkspace_WkSp.m_bShowSegVal * EEFMT_SEG);
            CmdLogVar(ERR_Read_Failed_At, szStr);
            rVal = LOGERROR_QUIET;
            break;
        }

        dwBytes = dwcb;

        SetCtrlCTrap();
        // dwords much faster than bytes, but watch out for endian bias:
        nDW = dwBytes / sizeof(DWORD);
        for (i = 0; i < nDW; i++) {
            if (CheckCtrlCTrap()) {
                rVal = LOGERROR_QUIET;
                break;
            }
            if (buf0[i] != buf1[i] &&
                  mismatch(addr0, (LPBYTE)&buf0[i], addr1, (LPBYTE)&buf1[i], sizeof(UINT))) {
                fMatched = FALSE;
            }
            GetAddrOff(addr0) += sizeof(DWORD);
            GetAddrOff(addr1) += sizeof(DWORD);
        }
        nDW = dwBytes % sizeof(DWORD);
        if (nDW) {
            if (CheckCtrlCTrap()) {
                rVal = LOGERROR_QUIET;
                break;
            }
            if (mismatch(addr0, (LPBYTE)&buf0[i], addr1, (LPBYTE)&buf1[i], nDW)) {
                fMatched = FALSE;
            }

            GetAddrOff(addr0) += nDW;
            GetAddrOff(addr1) += nDW;
        }

        dwLen -= dwBytes;
    }
    ClearCtrlCTrap();

done:
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        LppdCur = LppdT;
        LptdCur = LptdT;
        UpdateDebuggerState(UPDATE_CONTEXT);
    }
    return rVal;
}                   /* LogCompare() */


BOOL
MatchPattern(
    PUCHAR  String,
    PUCHAR  Pattern
    )
{
    UCHAR   c, p, l;

    for (; ;) {
        switch (p = *Pattern++) {
            case 0:                             // end of pattern
                return *String ? FALSE : TRUE;  // if end of string TRUE

            case '*':
                while (*String) {               // match zero or more char
                    if (MatchPattern (String++, Pattern))
                        return TRUE;
                }
                return MatchPattern (String, Pattern);

            case '?':
                if (*String++ == 0)             // match any one char
                    return FALSE;                   // not end of string
                break;

            case '[':
                if ( (c = *String++) == 0)      // match char set
                    return FALSE;                   // syntax

                c = (UCHAR)toupper(c);
                l = 0;
                while (p = *Pattern++) {
                    if (p == ']')               // if end of char set, then
                        return FALSE;           // no match found

                    if (p == '-') {             // check a range of chars?
                        p = *Pattern;           // get high limit of range
                        if (p == 0  ||  p == ']')
                            return FALSE;           // syntax

                        if (c >= l  &&  c <= p)
                            break;              // if in range, move on
                    }

                    l = p;
                    if (c == p)                 // if char matches this element
                        break;                  // move on
                }

                while (p  &&  p != ']')         // got a match in char set
                    p = *Pattern++;             // skip to end of set

                break;

            default:
                c = *String++;
                if (toupper(c) != p)            // check for exact char
                    return FALSE;                   // not a match

                break;
        }
    }
}


LOGERR
LogDisasm(
    LPSTR lpsz,
    BOOL  fSearch
    )
/*++

Routine Description:

    This function does the dump code command.

    Syntax:
        dc
        dc address [endaddr]]
        dc address l cBytes
        dc address I cInstrunctions

Arguments:

    lpsz    - argument list for dump code command

    fSearch - if TRUE, search for pattern in disassembly

Return Value:

    log error code

--*/
{
    SDI             sds;
    int             cch;
    ADDR            addr;
    ADDR            endAddr = {0};

    //
    // Indicates whether we should disassemble 'cLine' amount of
    // instructions (FALSE), or disassemble from 'addr' to 'endAddr'
    // regardless of how many lines of instructions it is (TRUE).
    //
    BOOL            bStopAtAddr = FALSE;

    int             err;
    int             x;
    DWORD           cLine = 8;
    LPPD            LppdT = LppdCur;
    LPTD            LptdT = LptdCur;
    LOGERR          rVal = LOGERROR_NOERROR;
    LPSTR           lpch;
    int             cb;
    LPSTR           currFunc = NULL;
    NEARESTSYM      nsym;
    LPSTR           p;
    CHAR            buf[256];
    MSG             msg;
    //BOOL            fNotFound = FALSE;
    BOOL            bSecondParamIsALength;


    // Keep lint from whining
    ZeroMemory( &nsym, sizeof(nsym) );


    CmdInsertInit();
    IsKdCmdAllowed();

    TDWildInvalid();
    PDWildInvalid();
    PreRunInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;

    if (!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    /*
    **  If no arguments are given use the current CS:IP.
    */

    if (*lpsz == 0) {

        if (addrLastDisasmEnd.addr.seg == 0 && addrLastDisasmEnd.addr.off == 0) {
            OSDGetAddr(LptdCur->lppd->hpid, 
                       LptdCur->htid, 
                       adrPC,
                       &addrLastDisasmEnd
                       );
            SYFixupAddr(&addrLastDisasmEnd);
        } else {
            SYSanitizeAddr(&addrLastDisasmEnd);
        }
        addr = addrLastDisasmEnd;

    } else {

        /*
        ** The argument must be an address -- so try and get it.
        */

        lpsz = CPSkipWhitespace(lpsz);

        err = CPGetRange(lpsz,
                         &cch,
                         &addr,
                         &endAddr,
                         &cLine,
                         radix,
                         8,
                         1,
                         &CxfIp,
                         fCaseSensitive,
                         g_contWorkspace_WkSp.m_bMasmEval,
                         &bSecondParamIsALength);

        if (err != CPNOERROR) {

            CmdLogVar(ERR_RangeExpr_Invalid);
            rVal = LOGERROR_QUIET;
            goto done;

        } else if (GetAddrOff(endAddr) < GetAddrOff(addr)) {

            CmdLogVar(ERR_RangeExpr_Invalid);
            rVal = LOGERROR_QUIET;
            goto done;

        } else if (!cLine || !bSecondParamIsALength) {
            // I don't think cLine is set to 0 anymore, but just to be on the safe side we left it in there.
            //
            // If the line count is zero, or the second parameter is an address, then disassemble up to endAddr.
            //
            bStopAtAddr = TRUE;
        }

        lpsz += cch;
    }

    addrLastDisasmStart = addr;

    /*
    **  Check that all characters are used
    */

    lpsz = CPSkipWhitespace(lpsz);
    if (*lpsz != 0) {
        rVal = LOGERROR_UNKNOWN;
        goto done;
    }

    /*
    **  Now do the dissassembly
    */

    sds.dop = (g_contWorkspace_WkSp.m_dopDisAsmOpts & ~(0x800)) | dopAddr | dopOpcode | dopOperands;
    sds.addr = addr;

    if ( !bStopAtAddr ) {
        endAddr = sds.addr;
    }

    SetCtrlCTrap();

    if (!fSearch) {
        ZeroMemory( &nsym, sizeof(nsym) );
        if (GetNearestSymbolInfo( &sds.addr, &nsym )) {
            if (nsym.hsymP) {
                currFunc = FormatSymbol( nsym.hsymP, &nsym.cxt );
                CmdLogFmt("%s+0x%I64x:\r\n", 
                          currFunc, 
                          GetAddrOff(sds.addr) - GetAddrOff(nsym.addrP) 
                          );
                free( currFunc );
            } else {
                //if (!fNotFound) {
                    //CmdLogFmt( "<unknown>\r\n" );
                //}
                //fNotFound = TRUE;
            }
        }
    }

    while (TRUE) {

        if (CheckCtrlCTrap()) {
            rVal = LOGERROR_QUIET;
            break;
        }

        if (bStopAtAddr) {
            if (GetAddrOff(endAddr) <= GetAddrOff(sds.addr)) {
                break;
            }
        } else {
            //
            //  Stop displaying when we are about to wrap around
            //
            if (GetAddrOff(endAddr) > GetAddrOff(sds.addr )) {
                break;
            }

            if ((!fSearch) && (!cLine)) {
                break;
            }

            cLine -= 1;
        }

        if (!fSearch && (GetAddrOff(sds.addr) >= GetAddrOff(nsym.addrN))) {
            ZeroMemory( &nsym, sizeof(nsym) );
            if (GetNearestSymbolInfo( &sds.addr, &nsym )) {
                if (nsym.hsymP) {
                    currFunc = FormatSymbol( nsym.hsymP, &nsym.cxt );
                    CmdLogFmt("%s+0x%I64x:\r\n", 
                              currFunc, 
                              GetAddrOff(sds.addr) - GetAddrOff(nsym.addrP) 
                              );
                    free( currFunc );
                } else {
                    //if (!fNotFound) {
                        //CmdLogFmt( "<unknown>\r\n" );
                    //}
                    //fNotFound = TRUE;
                }
            }
        }

        x = 0;
        if ( !bStopAtAddr ) {
            endAddr = sds.addr;
        }
        if (OSDUnassemble(LppdCur->hpid, LptdCur->htid, &sds) != xosdNone) {

            rVal = LOGERROR_UNKNOWN;
            break;

        } else {

            p = &buf[0];
            *p = 0;

            if (sds.ichAddr != -1) {
                sprintf(p, "%s  ", &sds.lpch[sds.ichAddr]);
                p += strlen(p);
            }
            cb = strlen(&sds.lpch[sds.ichAddr]) + 2;

            if (sds.ichBytes != -1) {
                lpch = sds.lpch + sds.ichBytes;
                //
                //v-vadimp breaks bundle bytes display on ia64
                //
                if(LppdCur->mptProcessorType != mptia64) {
                    while (strlen(lpch) > 16) {
                        sprintf(p, "%16.16s\r\n%*s", lpch, cb, " ");
                        p += strlen(p);
                        lpch += 16;
                    }
                }
                cb = 17 - strlen(lpch);
                sprintf(p, "%-17s", lpch);
                p += strlen(p);
            }

            if ((LppdCur->mptProcessorType == mptia64) && (sds.ichPreg != -1)) {
                    sprintf(p, "%-5s", &sds.lpch[sds.ichPreg]);
                    p += strlen(p);
            }

            if (sds.ichOpcode == -1) {
                sprintf(p, "%-12s ", "???");
            }
            else {
                sprintf(p, "%-12s ", &sds.lpch[sds.ichOpcode]);
            }
            p += strlen(p);

            if (sds.ichOperands != -1) {
                sprintf(p, "%-25s ", &sds.lpch[sds.ichOperands]);
                p += strlen(p);
            } else if (sds.ichComment != -1) {
                sprintf(p, "%25s ", " ");
                p += strlen(p);
            }

            if (sds.ichComment != -1) {
                sprintf(p, "%s", &sds.lpch[sds.ichComment]);
                p += strlen(p);
            }

            if (fSearch) {
                if (MatchPattern( (PBYTE) buf, (PBYTE) lpszLastSearch )) {
                    ZeroMemory( &nsym, sizeof(nsym) );
                    if (GetNearestSymbolInfo( &sds.addr, &nsym )) {
                        if (nsym.hsymP) {
                            currFunc = FormatSymbol( nsym.hsymP, &nsym.cxt );
                            CmdLogFmt("%s+0x%I64x:\r\n",
                                      currFunc,
                                      GetAddrOff(sds.addr) - GetAddrOff(nsym.addrP)
                                      );
                            free( currFunc );
                        } else {
                            //if (!fNotFound) {
                                //CmdLogFmt( "<unknown>\r\n" );
                            //}
                            //fNotFound = TRUE;
                        }
                    }
                    break;
                } else {
                    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                        ProcessQCQPMessage(&msg);
                    }
                }
            } else {
                CmdLogFmt( "%s\r\n", buf );
            }
        }
    }

    ClearCtrlCTrap();

    addrLastDisasmEnd = sds.addr;

done:
    LppdCur = LppdT;
    LptdCur = LptdT;
    return rVal;
}                   /* LogDisasm() */


LOGERR
LogSearchDisasm(
    LPSTR lpsz
    )
/*++

Routine Description:

    This function does a regular expression search of the disasm output.

Arguments:

    lpsz    - search pattern

Return Value:

    log error code

--*/
{
    CmdInsertInit();

    if ((!lpsz || !*lpsz) && (!lpszLastSearch || !*lpszLastSearch)) {
        CmdLogFmt( "You must specify a search pattern\r\n" );
        return LOGERROR_QUIET;
    }

    if (*lpsz) {
        if (lpszLastSearch) {
            free( lpszLastSearch );
        }
        lpsz = CPSkipWhitespace(lpsz);
        lpszLastSearch = (PSTR) malloc( strlen(lpsz) + 16 );
        if (!lpszLastSearch) {
            CmdLogFmt( "Out of memory doing search\r\n" );
            return LOGERROR_QUIET;
        }
        if (*lpsz != '*') {
            *lpszLastSearch = '*';
            strcpy( lpszLastSearch+1, lpsz );
        }
        if (lpszLastSearch[strlen(lpszLastSearch)-1] != '*') {
            strcat(lpszLastSearch,"*");
        }
        _strupr( lpszLastSearch );
    }

    return LogDisasm( "", TRUE );
}


LOGERR
LogDumpMem(
    char  chType,
    LPSTR lpsz
    )
/*++

Routine Description:

    This function is the generic function which is used to dump
    memory to the command window.

Arguments:

    lpsz    - Supplies argument list for memory dump command
    type    - Supplies type of memory to be dumpped

Return Value:

    log error code

--*/
{
    char    rgch[100];   // format into this string
    char    rgch3[100];  // ascii dump for db
    BYTE    rgb[100];    // bytes to be formatted
    int     i;
    int     j;
    DWORD   jj;
    ADDR    addr;        // address to format at
    ADDR    addr1;       // tmp
    ULONG   Items;
    int     cch;         // parser variable
    DWORD   cb;          // bytes read by OSDebug
    int     cItems;      // dmfi[].cItems
    LPPD    LppdT = LppdCur;
    LPTD    LptdT = LptdCur;
    LOGERR  rVal = LOGERROR_NOERROR;
    int     err;
    LOG_DM  type;
    BOOL    fQuit;
    XOSD    xosd;
    BOOL    bDBCS = FALSE;
    BYTE    chSave = 0;
    static  UINT uiCodePage = (UINT)-1;


    CmdInsertInit();
    IsKdCmdAllowed();

    PDWildInvalid();
    TDWildInvalid();
    PreRunInvalid();

    type = LetterToType(chType);
    if (type == LOG_DM_UNKNOWN) {
        return LOGERROR_UNKNOWN;
    }
    cItems = dmfi[type].cItems;

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        UpdateDebuggerState(UPDATE_CONTEXT);
    }

    /*
    **  Check the debugger is alive
    */

    if (!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    /*
    **  Get the address to start dumping memory at.
    **  Either this is specified in the command or it is a continue from
    **  a previous command.
    */

    lpsz = CPSkipWhitespace(lpsz);

    if (*lpsz == 0) {    // no arg

        // if we haven't dumped, look at what we just entered:
        if (addrLastDumpEnd.addr.seg == 0 && addrLastDumpEnd.addr.off == 0) {
            addrLastDumpEnd = addrLastEnterStart;
        }
        // if nothing is there, try what we just disassembled:
        if (addrLastDumpEnd.addr.seg == 0 && addrLastDumpEnd.addr.off == 0) {
            addrLastDumpEnd = addrLastDisasmStart;
        }
        if (addrLastDumpEnd.addr.seg == 0 && addrLastDumpEnd.addr.off == 0) {
            OSDGetAddr(LppdCur->hpid, LptdCur->htid, adrData, &addrLastDumpEnd);
        } else {
            SYSanitizeAddr(&addrLastDumpEnd);
        }
        addr = addrLastDumpEnd;

    } else {

        err = CPGetRange(lpsz,
                         &cch,
                         &addr,
                         &addr1,
                         &Items,
                         radix,
                         cItems,
                         dmfi[type].cBytes,
                         &CxfIp,
                         fCaseSensitive,
                         g_contWorkspace_WkSp.m_bMasmEval,
                         NULL);

        if (err != CPNOERROR) {
            CmdInsertInit();
            CmdLogVar(ERR_RangeExpr_Invalid);
            rVal = LOGERROR_QUIET;
            goto done;
        }

        if (GetAddrOff(addr1) < GetAddrOff(addr)) {
            CmdInsertInit();
            CmdLogVar(ERR_RangeExpr_Invalid);
            rVal = LOGERROR_QUIET;
            goto done;
        }
        if (Items) {
            cItems = Items;
        } else {
            cItems = (DWORD)(addr1.addr.off - addr.addr.off + dmfi[type].cBytes - 1) / dmfi[type].cBytes;
        }
        if (cItems < 1) {
            CmdInsertInit();
            CmdLogVar(ERR_RangeExpr_Invalid);
            rVal = LOGERROR_QUIET;
            goto done;
        }

        /*
        **  Must have used up the entire line or else it's an error
        */

        lpsz = CPSkipWhitespace(lpsz + cch);
        if (*lpsz != 0) {
            rVal = LOGERROR_UNKNOWN;
            goto done;
        }
    }

    /*
    **  Dump out the memory
    */

    addrLastDumpStart = addr;

    SetCtrlCTrap();

    //Initialize
    bDBCS = FALSE;

    //
    // Be sure to leave this loop normally, or ClearCtrlCTrap()
    //
    for (i = 0, j = 0, fQuit = 0; !fQuit && i < cItems; i++) {

        // if at beginning of line, get address, display it
        if (j == 0) {
            //
            // check for abort
            //   remainder of command line should be discarded
            //
            if (CheckCtrlCTrap()) {
                rVal = LOGERROR_QUIET;
                break;
            }

            EEFormatAddress(&addr, rgch, sizeof(rgch),
                         g_contWorkspace_WkSp.m_bShowSegVal * EEFMT_SEG);
            CmdLogFmt("%s%s", rgch, (type == LOG_DM_ASCII || type == LOG_DM_UNICODE)? "  " : " ");

            rgch3[0] = 0;
        }

        // format and display next data item

        xosd = OSDReadMemory( LppdCur->hpid,
                              LptdCur->htid,
                              &addr,
                              rgb,
                              dmfi[type].cBytes,
                              &cb
                              );

        if (cb != dmfi[type].cBytes) {

            memset(rgch, '?', sizeof(rgch));
            rgch[dmfi[type].cchItem] = 0;
            CmdLogFmt(" %s", rgch);

            if (type == LOG_DM_DWORD) {
                strcat(rgch3, "????");
            } else {
                strcat(rgch3, "?");
            }

        } else {

            switch (type) {

              case LOG_DM_4REAL:
              case LOG_DM_8REAL:
              case LOG_DM_TREAL:

                // this should never fail
                Dbg(CPFormatMemory(rgch,
                        2 * dmfi[type].cBytes + 1,
                        rgb,
                        dmfi[type].cBytes * 8,
                        fmtUInt | fmtZeroPad,
                        16) == CPNOERROR);
                CmdLogFmt(" %s", rgch);

                // let's not assert on FP formatting problems...
                CPFormatMemory(
                        rgch,
                        dmfi[type].cchItem + 1,
                        rgb,
                        dmfi[type].cBytes * 8,
                        dmfi[type].fmtType,
                        dmfi[type].radix);
                CmdLogFmt("  %s", rgch);

                break;

              case LOG_DM_ASCII:

                if (*(LPSTR)rgb == '\0') {
                    fQuit = TRUE;
                } else {
                    CPFormatMemory(rgch, dmfi[type].cchItem + 1, rgb, dmfi[type].cBytes*8,
                        dmfi[type].fmtType, dmfi[type].radix);
                    if (bDBCS) {
                        if (j == 0) {
                            //This means that current *pszSrc is the 2nd byte
                            //of a splited DBCS
                            rgch[0] = '.';
                        } else {
                            //This DBC is changed to '.' by CPFormatMemory().
                            //So I restore it.
                            rgch[0] = chSave;
                            rgch[1] = rgb[0];
                            rgch[2] = '\0';
                        }
                        CmdLogFmt("%s", rgch);
                        bDBCS = FALSE;
                    } else if (IsDBCSLeadByte((BYTE)(0x00ff & rgb[0]))) {
                        chSave = rgb[0];
                        bDBCS = TRUE;
                    }
                    else if ((BYTE)rgb[0] >= (BYTE)0xa1 && (BYTE)rgb[0] <= (BYTE)0xdf) {
                        //'Hankaku Kana' is changed to '.' by CPFormatMemory().
                        rgch[0] = rgb[0];
                        CmdLogFmt("%s", rgch);
                    }
                    else {
                        CmdLogFmt("%s", rgch);
                    }
                }
                break;

              case LOG_DM_UNICODE:

                if (*(LPWCH)rgb == '\0') {
                    fQuit = TRUE;
                } else {
                    /*
                     * This is too bad. This kind of process should be done
                     * in CPFormatMemory(). But eecan???.dll shouldn't use
                     * WideCharToMultiByte() of GetACP() somehow.
                     */
                    int iCount;

                    if (uiCodePage == (UINT)-1) {
                        uiCodePage = GetACP();
                    }
                    iCount = WideCharToMultiByte(
                                uiCodePage,         // CodePage
                                0,                  // dwFlags
                                (LPWCH)rgb,         // lpWideCharStr
                                1,                  // cchWideCharLength
                                rgch,               // lpMultiByteStr
                                MB_CUR_MAX,         // cchMultiByteLength
                                NULL,               // lpDefaultChar
                                NULL                // lpUseDefaultChar
                                );

                    //This criterion should be same as one in CPFormatMemory().
                    if (iCount == 0
                    || (rgch[0] < ' ' || rgch[0] > 0x7e)
                    &&  !IsDBCSLeadByte(rgch[0])) {
                        rgch[0] = '.';
                        iCount = 1;
                    }
                    rgch[iCount] = 0;
                    CmdLogFmt("%s", rgch);
                }
                break;

              case LOG_DM_BYTE:
              case LOG_DM_WORD:
              case LOG_DM_DWORD:

                if (type == LOG_DM_DWORD && i < 4) {
                    ulPseudo[i] = *(LPDWORD)&rgb[0];
                }

                CPFormatMemory(rgch, dmfi[type].cchItem + 1, rgb, dmfi[type].cBytes*8,
                    dmfi[type].fmtType | fmtZeroPad, dmfi[type].radix);
                CmdLogFmt(" %s", rgch);

                switch (type) {
                  case LOG_DM_BYTE:
                    CPFormatMemory(&rgch3[j], 1, rgb, 8, fmtAscii, 0);
                    if (bDBCS) {
                        if (j == 0) {
                            //This means that current *pszSrc is the 2nd byte
                            //of a splited DBCS
                            rgch3[j] = '.';
                        } else {
                            //This DBC is changed to '.' by CPFormatMemory().
                            //So I restore it.
                            rgch3[j] = rgb[0];
                        }
                        bDBCS = FALSE;
                    } else if (IsDBCSLeadByte((BYTE)(0x00ff & rgb[0]))) {
                        rgch3[j] = rgb[0];
                        bDBCS = TRUE;
                    }
                    else if ((BYTE)rgb[0] >= (BYTE)0xa1 && (BYTE)rgb[0] <= (BYTE)0xdf) {
                        //'Hankaku Kana' is changed to '.' by CPFormatMemory().
                        rgch3[j] = rgb[0];
                    }
                    rgch3[j+1] = 0;
                    break;

                  case LOG_DM_WORD:
#if defined(DW_COMMAND_SUPPORT)
                  /*
                   * This functionality shouldn't be supported.
                   * If you want support this functionarity,
                   * Just define "DW_COMMAND_SUPPORT".
                   */
                  {
                    /*
                     * This is too bad. These kind of process should be done
                     * in CPFormatMemory(). But eecan???.dll shouldn't use
                     * WideCharToMultiByte() of GetACP() somehow.
                     */
                    int iCount;

                    if (uiCodePage == (UINT)-1) {
                        uiCodePage = GetACP();
                    }
                    iCount = WideCharToMultiByte(
                                uiCodePage,         // CodePage
                                0,                  // dwFlags
                                (LPWCH)rgb,         // lpWideCharStr
                                1,                  // cchWideCharLength
                                &rgch3[j],          // lpMultiByteStr
                                MB_CUR_MAX,         // cchMultiByteLength
                                NULL,               // lpDefaultChar
                                NULL                // lpUseDefaultChar
                                );

                    //This criterion should be same as one in CPFormatMemory().
                    if (iCount == 0
                    || (rgch[0] < ' ' || rgch[0] > 0x7e)
                    && !IsDBCSLeadByte(rgch[0])) {
                        rgch3[j]   = '.';
                        rgch3[j+1] = ' ';
                    }
                    else if (iCount == 1) {
                        rgch3[j+1] = ' ';
                    }
                    rgch3[j+2] = 0;
                    j++;
                  }
#else
                    CPFormatMemory(&rgch3[j], 1, rgb, 16, fmtUnicode, 0);
                    rgch3[j+1] = 0;
#endif
                    break;

                  case LOG_DM_DWORD:
                    for (jj = 0; jj < dmfi[type].cBytes; jj++) {
                        CPFormatMemory(&rgch3[dmfi[type].cBytes * j + jj],
                                        1, &rgb[jj], 8, fmtAscii, 0);
                    }
                    rgch3[dmfi[type].cBytes * j + jj] = 0;
                    break;
                }
            }
        }

        // if at end of line or end of list:
        if ( fQuit || ++j >= (int)dmfi[type].cAcross  ||  i >= cItems - 1) {
            if (bDBCS) {
                //If DBC is separated by new line, add 2nd byte.
                if (type == LOG_DM_BYTE) {
                    GetAddrOff(addr) += dmfi[type].cBytes;
                    xosd = OSDReadMemory(LppdCur->hpid,
                                         LptdCur->htid,
                                         &addr,
                                         rgb,
                                         dmfi[type].cBytes,
                                         &cb
                                         );
                    GetAddrOff(addr) -= dmfi[type].cBytes;
                    rgch3[j] = rgb[0];
                    rgch3[j+1] = '\0';
                } else if (type == LOG_DM_ASCII) {
                    GetAddrOff(addr) += dmfi[type].cBytes;
                    xosd = OSDReadMemory(LppdCur->hpid,
                                         LptdCur->htid,
                                         &addr,
                                         rgb,
                                         dmfi[type].cBytes,
                                         &cb
                                         );
                    GetAddrOff(addr) -= dmfi[type].cBytes;
                    rgch[0] = chSave;
                    rgch[1] = rgb[0];
                    rgch[2] = '\0';
                    CmdLogFmt("%s", rgch);
                }
            }

            // if in a hex mode, fill line and display ASCII version

            if (type == LOG_DM_BYTE || type == LOG_DM_WORD || type == LOG_DM_DWORD) {
                //
                // fill row to justify right column
                //
                cb = (dmfi[type].cAcross - j) * (dmfi[type].cchItem + 1) + 1;
                CmdLogFmt("%*s%s", cb, " ", rgch3);
            }
            // display newline
            CmdLogFmt("\r\n");
            j = 0;
        }

        GetAddrOff(addr) += dmfi[type].cBytes;
        GetAddrOff(addr) = SEPtrTo64( GetAddrOff(addr) );

    }

    ClearCtrlCTrap();

    addrLastDumpEnd = addr;

done:
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        LppdCur = LppdT;
        LptdCur = LptdT;
        UpdateDebuggerState(UPDATE_CONTEXT);
    }
    return rVal;
}                   /* LogDumpMem() */


LOGERR
LogEnterMem(
    LPSTR lpsz
)
/*++

Routine Description:

    This function is used from the command line to do editing of memory.

Arguments

    lpsz - Supplies argument list for memory edit command

    type - Supplies type of memory to be edited

Returns:

    log error code

--*/
{
    int      cch;
    ADDR     addr;
    LPPD     LppdT = LppdCur;
    LPTD     LptdT = LptdCur;
    LOGERR   rVal = LOGERROR_NOERROR;
    LOG_DM   type;

    CmdInsertInit();
    IsKdCmdAllowed();

    PDWildInvalid();
    TDWildInvalid();
    PreRunInvalid();

    type = LetterToType(*lpsz++);
    if (type == LOG_DM_UNKNOWN) {
        return LOGERROR_UNKNOWN;
    }

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        UpdateDebuggerState(UPDATE_CONTEXT);
    }

    //
    //  Check the debugger is really alive
    //

    if (!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    //
    //  Get the starting address to edit at
    //

    lpsz = CPSkipWhitespace(lpsz);

    if (*lpsz == 0) {
        rVal = LOGERROR_UNKNOWN;
        goto done;
    }

    if (CPGetAddress( lpsz,
                      &cch,
                      &addr,
                      radix,
                      &CxfIp,
                      fCaseSensitive,
                      g_contWorkspace_WkSp.m_bMasmEval) == 0) {
        lpsz += cch;
    } else {
        CmdLogVar(ERR_AddrExpr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    SYFixupAddr(&addr);
    addrLastEnterStart = addr;

    lpsz = CPSkipWhitespace(lpsz);

    if (*lpsz != 0) {
        rVal = DoEnterMem(lpsz, &addr, type, TRUE);
    } else {
        //
        // Interactive enter:
        //
        // set global context:
        //
        LppdT = LppdCommand;
        LptdT = LptdCommand;

        addrAsm = addr;
        nEnterType = type;
        CmdSetCmdProc(CmdEnterLine, CmdEnterPrompt);
        CmdSetAutoHistOK(FALSE);
        CmdSetEatEOLWhitespace(FALSE);
    }

done:
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        LppdCur = LppdT;
        LptdCur = LptdT;
        UpdateDebuggerState(UPDATE_CONTEXT);
    }
    return rVal;
}                   /* LogEnterMem() */


LOGERR
LogFill(
    LPSTR lpsz
    )
/*++

Routine Description:

    fill command
    fi <range> {<byte list> | <quoted string>}
    fib <range> <byte list>
    fiw <range> <word list>
    fid <range> <dword list>
    fii <range> <real4 list>
    fis <range> <real8 list>
    fit <range> <real10 list>
    fia <range> <ascii string>
    fiu <range> <unicode string>

Arguments:

    lpsz  - Supplies pointer to command tail

Return Value:

    LOGERR code

--*/
{
    int      cch;
    LOG_DM   type;
    ADDR     addr0;
    ADDR     addr1;
    ULONG    Items;
    ULONG    lLen;
    BYTE     buf[2 * MAX_USER_LINE];
    DWORD    cb;
    int      err;
    XOSD     xosd;
    LOGERR   rVal = LOGERROR_NOERROR;
    LPPD     LppdT = LppdCur;
    LPTD     LptdT = LptdCur;
    BOOL     bSecondParamIsALength;

    CmdInsertInit();

    PDWildInvalid();
    TDWildInvalid();
    PreRunInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        UpdateDebuggerState(UPDATE_CONTEXT);
    }

    type = LetterToType(*lpsz);
    if (type != LOG_DM_UNKNOWN) {
        lpsz++;
    }

    //
    // Getting the range is tricky, because we have 2 distinct cases:
    //  (1) fid 0x0100 l5 0xaabbccdd - will write 20 bytes, because the end range is
    //            calculated by (object size) * (count)
    //      where (object size) = 4. FID fills double words which are 4 bytes values.
    //            (count) = 5. Since l5 was specified.
    //
    //  (2) fid 0x0100 0x0105 0xaabbccdd - will write 4 bytes, because the end range
    //            must be respected regardles of the object size.
    //

    // First try. We assume that the user specified the (1) case. See above.
    err = CPGetRange(
                lpsz,
                &cch,
                &addr0,
                &addr1,
                &Items,
                radix,
                0,
                dmfi[(type == LOG_DM_UNKNOWN) ? LOG_DM_BYTE : type].cBytes,
                &CxfIp,
                fCaseSensitive,
                g_contWorkspace_WkSp.m_bMasmEval,
                &bSecondParamIsALength);


    // We assumed incorrectly if the second parameter was not a length and the size of the object is not 1.
    // Assuming a byte is the smallest unit.
    //
    // In other words, if the size of the object is 1, we don't care whether a
    // length or an address was specified, both will work correctly. If the size is
    // greater than 1 and a memroy address was specified, then we need to do a recalc
    // because "CPGetRange" will sometimes round the memory address up to accomadate
    // whole objects.
    DAssert(1 == dmfi[LOG_DM_BYTE].cBytes);
    //
    if (CPNOERROR == err && !bSecondParamIsALength &&
        (1 != dmfi[(type == LOG_DM_UNKNOWN) ? LOG_DM_BYTE : type].cBytes) )
    {
        // Reevaluating everything is the safest method. This buffers us from segmented memory models, ect...
        //
        err = CPGetRange(
                    lpsz,
                    &cch,
                    &addr0,
                    &addr1,
                    &Items,
                    radix,
                    0,
                    1, // size in bytes
                    &CxfIp,
                    fCaseSensitive,
                    g_contWorkspace_WkSp.m_bMasmEval,
                    &bSecondParamIsALength);
    }


    if (err != CPNOERROR) {
        CmdLogVar(ERR_RangeExpr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    if (GetAddrOff(addr1) < GetAddrOff(addr0)) {
        CmdLogVar(ERR_RangeExpr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    lLen = (DWORD)(GetAddrOff(addr1) - GetAddrOff(addr0));

    lpsz = CPSkipWhitespace(lpsz + cch);

    if (type == LOG_DM_UNKNOWN) {
        type = (*lpsz == '"' || *lpsz == '\'') ? LOG_DM_ASCII : LOG_DM_BYTE;
    }

    if (dmfi[type].cBytes && (cb = lLen % dmfi[type].cBytes) != 0) {
        if (cb == 1) {
            CmdLogVar(DBG_Not_Exact_Fill);
        } else {
            CmdLogVar(DBG_Not_Exact_Fills, cb);
        }
        lLen -= cb;
    }

    err = GetValueList(lpsz, type, TRUE, buf, sizeof(buf), &cb);
    if (err != LOGERROR_NOERROR) {
        rVal = err;
        goto done;
    }
    if (cb == 0) {
        CmdLogVar(ERR_Expr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    SetCtrlCTrap();
    while (lLen) {
        if (CheckCtrlCTrap()) {
            rVal = LOGERROR_QUIET;
            break;
        }
        if (cb > lLen) {
            cb = lLen;
        }
        xosd = OSDWriteMemory(LppdCur->hpid,
                              LptdCur->htid,
                              &addr0,
                              buf,
                              cb,
                              &cb
                              );
        if (xosd != xosdNone) {
            EEFormatAddress(&addr0, (PSTR) buf, 30,
                         g_contWorkspace_WkSp.m_bShowSegVal * EEFMT_SEG);
            CmdLogVar(ERR_Write_Failed_At, buf);
            rVal = LOGERROR_QUIET;
            break;
        }
        lLen -= cb;
        GetAddrOff(addr0) += cb;
    }
    ClearCtrlCTrap();

done:
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        LppdCur = LppdT;
        LptdCur = LptdT;
        UpdateDebuggerState(UPDATE_CONTEXT);
    }
    return rVal;
}                   /* LogFill() */


LOGERR
LogMovemem(
    LPSTR lpsz
    )
{
    int      err;
    int      cch;
    XOSD     xosd;
    ADDR     addr0;
    ADDR     addr1;
    ULONG    Items;
    LONG     lLen;
    ULONG     lBytes;
    LONG     lWanted;
    BYTE     buf[MEMBUF_SIZE];
    char     szStr[100];
    int      nDir;
    BOOL     fDoUpdate = FALSE;

    LPPD     LppdT = LppdCur;
    LPTD     LptdT = LptdCur;
    LOGERR   rVal = LOGERROR_NOERROR;


    CmdInsertInit();

    PDWildInvalid();
    TDWildInvalid();
    PreRunInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        UpdateDebuggerState(UPDATE_CONTEXT);
    }


    err = CPGetRange(lpsz,
                     &cch,
                     &addr0,
                     &addr1,
                     &Items,
                     radix,
                     0,
                     1,
                     &CxfIp,
                     fCaseSensitive,
                     g_contWorkspace_WkSp.m_bMasmEval,
                     NULL);

    if (err != CPNOERROR) {
        CmdLogVar(ERR_RangeExpr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    if (GetAddrOff(addr1) < GetAddrOff(addr0)) {
        CmdLogVar(ERR_RangeExpr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    lLen = (DWORD)(GetAddrOff(addr1) - GetAddrOff(addr0));

    lpsz = CPSkipWhitespace(lpsz + cch);
    err = CPGetAddress(lpsz,
                       &cch,
                       &addr1,
                       radix,
                       &CxfIp,
                       fCaseSensitive,
                       g_contWorkspace_WkSp.m_bMasmEval
                       );
    if (err != CPNOERROR) {
        CmdLogVar(ERR_AddrExpr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    SYFixupAddr(&addr1);

    if (GetAddrOff(addr0) > GetAddrOff(addr1)) {
        nDir = 1;
    } else {
        nDir = -1;
        GetAddrOff(addr0) += lLen;
        GetAddrOff(addr1) += lLen;
    }

    lWanted = lLen;
    SetCtrlCTrap();
    while (lLen > 0) {

        if (CheckCtrlCTrap()) {
            rVal = LOGERROR_QUIET;
            break;
        }

        lBytes = (lLen < MEMBUF_SIZE)? lLen : MEMBUF_SIZE;

        if (nDir < 0) {
            GetAddrOff(addr0) -= lBytes;
            GetAddrOff(addr1) -= lBytes;
        }

        xosd = OSDReadMemory(LppdCur->hpid,
                             LptdCur->htid,
                             &addr0,
                             buf,
                             lBytes,
                             &lBytes
                             );
        if (xosd != xosdNone || lBytes < 1) {
            EEFormatAddress(&addr0, szStr, sizeof(szStr),
                         g_contWorkspace_WkSp.m_bShowSegVal * EEFMT_SEG);
            CmdLogVar(ERR_Read_Failed_At, szStr);
            rVal = LOGERROR_QUIET;
            break;
        }

        xosd = OSDWriteMemory(LppdCur->hpid,
                              LptdCur->htid,
                              &addr1,
                              buf,
                              lBytes,
                              &lBytes
                              );
        if (xosd != xosdNone) {
            EEFormatAddress(&addr1, szStr, sizeof(szStr),
                         g_contWorkspace_WkSp.m_bShowSegVal * EEFMT_SEG);
            CmdLogVar(ERR_Write_Failed_At, szStr);
            rVal = LOGERROR_QUIET;
            break;
        }

        lLen -= lBytes;

        if (nDir > 0) {
            GetAddrOff(addr0) += lBytes;
            GetAddrOff(addr1) += lBytes;
        }
    }
    ClearCtrlCTrap();

    CmdLogVar(DBG_Bytes_Copied, lWanted - lLen, lWanted);

    fDoUpdate = TRUE;

done:
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        LppdCur = LppdT;
        LptdCur = LptdT;
        UpdateDebuggerState(UPDATE_CONTEXT);
    }
    if (fDoUpdate) {
        UpdateDebuggerState(UPDATE_DATAWINS);
    }
    return rVal;
}                   /* LogMovemem() */


LOGERR
LogRegisters(
    LPSTR lpsz,
    BOOL fFP
    )
/*++

Routine Description:

    Display the contents of one or all registers, or modify the contents of
    a register.  The regular or floating point register set may be selected
    by the fFP flag.

Arguments:

    lpsz    - Supplies string with the command on it

    fFP     - Supplies TRUE to use FP regs, FALSE to use integer regs

Return Value:

    log error code

--*/
{
    RD      rd;
    RT      rtMask;
    RT      rtExclude;
    char    rgch[100];
    BYTE    ab[30];
    int     cRegs;
    DWORD   cRegWidth = 0;
    LPSTR   lpch;
    int     i;
    int     j;
    int     y;
    LONG    ul;
    char    ch;
    int     rdx;
    FMTTYPE fmt;
    DWORD   update;
    LPPD    LppdT = LppdCur;
    LPTD    LptdT = LptdCur;
    LOGERR  rVal = LOGERROR_NOERROR;

    //
    // static flag for rt command
    //
    static BOOL fAllRegs = FALSE;

    CmdInsertInit();
    IsKdCmdAllowed();

    TDWildInvalid();
    PDWildInvalid();
    PreRunInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;

    /*
    **  Check for a live debuggee
    */

    if (!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    //
    // Default is to exclude no groups:
    //

    rtExclude = 0;

    //
    //  Check for floating point register dump
    //

    if (fFP) {
        rtMask = rtFPU;
    } else {
        rtMask = rtCPU;
    }

    if (lpsz[0] == 't' && lpsz[1] == '\0') {
        fAllRegs = !fAllRegs;
        lpsz++;
    }

    if (g_contWorkspace_WkSp.m_bRegModeExt || fAllRegs) {
        rtMask |= rtExtended;
    } else {
        rtMask |= rtRegular;
    }

    if (g_contWorkspace_WkSp.m_bRegModeMMU || fAllRegs) {
        rtMask |= rtSpecial;
        if (OSDGetDebugMetric(LppdCur->hpid, LptdCur->htid, mtrcExtMMU, &ul) == xosdNone &&
                ul == 0) {
            rtExclude = rtKmode;
        }
    }

    /*
    **  If no arguments then just dump the registers
    */

    OSDGetDebugMetric(LppdCur->hpid, LptdCur->htid, mtrcCRegs, &ul);
    cRegs = (int) ul;

    if (*lpsz == 0) {

        y = 0;

        for (i=0; i<cRegs; i++) {

            Dbg( OSDGetRegDesc( LppdCur->hpid, LptdCur->htid, i, &rd) == xosdNone );

            if (((rd.rt & rtProcessMask) == (rtMask & rtProcessMask)) &&
                ((rd.rt & rtMask & rtGroupMask) != 0) &&
                bDisplayReg(rd) &&
                ((rd.rt & rtExclude) == 0)) {

                cRegWidth = (rd.dwcbits + 7)/ 8 * 2;
                if ((y > 0)  &&
                      (rd.rt & rtNewLine) ||
                      (y + 1 + strlen(rd.lszName) + 1 + cRegWidth) > 80) {
                    CmdLogFmt("\r\n");
                    y = 0;
                } else {
                    if (y != 0) {
                        CmdLogFmt("  ");
                        y += 1;
                    }
                }


                CmdLogFmt("%s=", rd.lszName);
                Dbg( OSDReadRegister(LppdCur->hpid, LptdCur->htid, rd.dwId, ab) == xosdNone );

                switch (rd.rt & rtFmtTypeMask) {
                    case rtInteger:
                        fmt = fmtUInt | fmtZeroPad;
                        break;

                    case rtFloat:
                        fmt = fmtFloat;
                        break;

                    default:
                    case rtAddress:
                        fmt = fmtAddress | fmtZeroPad;
                        break;
                }

                if (fmt != fmtFloat) {
                    rdx = 16;
                } else {
                    rdx = 10;
                    for (j = 0; j < LOG_DM_MAX; j++) {
                        if (dmfi[j].fmtType == fmt &&
                              dmfi[j].cBytes*8 == rd.dwcbits) {
                            cRegWidth = dmfi[j].cchItem;
                            break;
                        }
                    }
                    Assert(j < LOG_DM_MAX);
                }
                Assert(cRegWidth+1 <= sizeof(rgch));
                CPFormatMemory(rgch, cRegWidth+1, ab, rd.dwcbits, fmt, rdx);

                // v-vadimp - pick up the NAT bit on ia64
                if ((LppdCur->mptProcessorType == mptia64) && (rd.rt & rtNat)) {
                    UINT NatReg, NatBit;
                    ULONGLONG NatRegValue;
                    if (rd.dwId >= CV_IA64_IntZero && rd.dwId <= CV_IA64_IntT22) {
                        NatReg = CV_IA64_IntNats;
                        NatBit = rd.dwId - CV_IA64_IntZero;
                    } else if (rd.dwId >= CV_IA64_IntR32 && rd.dwId <= CV_IA64_IntR95) {
                        NatReg = CV_IA64_IntNats2;
                        NatBit = rd.dwId - CV_IA64_IntR32;
                    } else if (rd.dwId >= CV_IA64_IntR96 && rd.dwId <= CV_IA64_IntR127) {
                        NatReg = CV_IA64_IntNats3;
                        NatBit = rd.dwId - CV_IA64_IntR96;
                    } else {
                        DAssert(!"Unknown register with a NAT bit");
                    }
                    OSDReadRegister(LppdCur->hpid,LptdCur->htid,NatReg,&NatRegValue);
                    if ((NatRegValue >> NatBit) & 0x1) {
                        strcat(rgch," 1");
                    } else {
                        strcat(rgch," 0");
                    }
                }

                CmdLogFmt("%s", rgch);

                y += strlen(rd.lszName) + cRegWidth + 1;
            }
        }
        if (y != 0) {
            CmdLogFmt("\r\n");
        }

        rVal = LOGERROR_NOERROR;
        goto done;
    }

    /*
    **  Now check for a specific register to have been requested.
    */

    lpsz = CPSkipWhitespace(lpsz);

    for (lpch = lpsz; (*lpch != 0) && (*lpch != '=') && (*lpch != ' ');
                        lpch = CharNext(lpch));
    ch = *lpch;
    *lpch = 0;

    lpsz = CharUpper(lpsz);          // Convert to uppercase name

    for (i=0; i<cRegs; i++) {
        Dbg( OSDGetRegDesc( LppdCur->hpid, LptdCur->htid, i, &rd) == xosdNone );

        //
        // match name and CPU; all groups allowed.
        //

        if ((rd.rt & rtProcessMask) == (rtMask & rtProcessMask)) {
            if (_stricmp(lpsz, rd.lszName) == 0) {
                break;
            }
            //
            // custom for <reg name>(<softw conv name>) format on Merced
            //
            if (LppdCur->mptProcessorType == mptia64) {
                PCHAR p1 = _tcschr(rd.lszName,'(');
                PCHAR p2 = _tcschr(rd.lszName,')');
                if (p1 != NULL && p2 != NULL) {
                    if (_strnicmp(lpsz,rd.lszName,__max(strlen(lpsz),(size_t)(p1-rd.lszName))) == 0) {
                        //
                        // reg name matched
                        //
                        break;
                    }
                    if (_strnicmp(lpsz,p1+1,__max(strlen(lpsz),(size_t)(p2-p1-1))) == 0) {
                        //
                        // conv name matched
                        //
                        break;
                    }
                }
            }

        }
    }

    if (i >= cRegs) {
        CmdLogVar(ERR_Register_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    *lpch = ch;
    lpch = CPSkipWhitespace(lpch);
    if (*lpch && *lpch!='=') {
        CmdLogFmt("Missing assigment operator\r\n");
        rVal = LOGERROR_QUIET;
        goto done;
    }

    /*
    **  Only display of requested register
    */

    if (*lpch != '=') {

        cRegWidth = (rd.dwcbits + 7)/ 8 * 2;
        CmdLogFmt("%s: ", rd.lszName);
        Dbg(OSDReadRegister(LppdCur->hpid, LptdCur->htid, rd.dwId, ab) == xosdNone );

        switch (rd.rt & rtFmtTypeMask) {
            default:
            case rtInteger:
                fmt = fmtUInt | fmtZeroPad;
                break;

            case rtFloat:
                fmt = fmtFloat;
                break;

            case rtAddress:
                fmt = fmtAddress;
                break;
        }

        if ((fmt & fmtBasis) != fmtFloat) {
            rdx = 16;
        } else {
            rdx = 10;
            for (j = 0; j < LOG_DM_MAX; j++) {
                if (dmfi[j].fmtType == (fmt & fmtBasis)
                      && dmfi[j].cBytes*8 == rd.dwcbits) {
                    cRegWidth = dmfi[j].cchItem;
                    break;
                }
            }
            Assert(j < LOG_DM_MAX);
        }
        Assert(cRegWidth+1 <= sizeof(rgch));
        CPFormatMemory(rgch, cRegWidth+1, ab, rd.dwcbits, fmt, rdx);

        //
        // pick up the NAT bit on IA64
        //
        if ((LppdCur->mptProcessorType == mptia64) && (rd.rt & rtNat))
        {
            UINT NatReg, NatBit;
            ULONGLONG NatRegValue;
            if (rd.dwId >= CV_IA64_IntZero && rd.dwId <= CV_IA64_IntT22) {
                NatReg = CV_IA64_IntNats;
                NatBit = rd.dwId - CV_IA64_IntZero;
            } else if (rd.dwId >= CV_IA64_IntR32 && rd.dwId <= CV_IA64_IntR95) {
                NatReg = CV_IA64_IntNats2;
                NatBit = rd.dwId - CV_IA64_IntR32;
            } else if (rd.dwId >= CV_IA64_IntR96 && rd.dwId <= CV_IA64_IntR127) {
                NatReg = CV_IA64_IntNats3;
                NatBit = rd.dwId - CV_IA64_IntR96;
            } else {
                DAssert(!"Unknown register with a NAT bit");
            }
            OSDReadRegister(LppdCur->hpid,LptdCur->htid,NatReg,&NatRegValue);
            if ((NatRegValue >> NatBit) & 0x1) {
                strcat(rgch," 1");
            } else {
                strcat(rgch," 0");
            }
        }

        CmdLogFmt("%s\r\n", rgch);

        rVal = LOGERROR_NOERROR;
        goto done;
    }


    /*
    **  Change a register
    */

    lpch++;             // Skip over '='

    // make sure there is a value
    lpch = CPSkipWhitespace(lpch);
    if (!*lpch) {
        CmdLogVar(ERR_Expr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

// MBH -
// for the moment, just say T_QUAD (was T_LONG), but should
// be dependent on the number of bits in the register.
//
    if (rd.rt & rtCPU) {
        if (CPGetCastNbr(lpch,
                         T_QUAD,
                         radix,
                         fCaseSensitive,
                         &CxfIp,
                         (LPSTR) ab,
                         (LPSTR) rgch,
                         g_contWorkspace_WkSp.m_bMasmEval) != CPNOERROR) {
            CmdLogVar(ERR_Expr_Invalid);
            rVal = LOGERROR_QUIET;
            goto done;
        }
    } else if (rd.rt & rtFPU) {
        if (CPGetFPNbr(lpch,
                       rd.dwcbits,
                       radix,
                       fCaseSensitive,
                       &CxfIp,
                       (LPSTR)ab,
                       (LPSTR) rgch)
                != CPNOERROR) {
            CmdLogVar(ERR_Expr_Invalid);
            rVal = LOGERROR_QUIET;
            goto done;
        }
    } else {
        CmdLogVar(ERR_Cant_Assign_To_Reg);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    if (OSDWriteRegister(LppdCur->hpid, LptdCur->htid, rd.dwId, ab) != xosdNone) {
        rVal = LOGERROR_UNKNOWN;
    }

    update = UPDATE_DATAWINS;
    if (rd.rt & rtFrame) {
        update |= UPDATE_CONTEXT;
    }
    if (rd.rt & rtPC) {
        update |= UPDATE_SOURCE | UPDATE_DISASM | UPDATE_CONTEXT;
    }

    UpdateDebuggerState(update);


done:

    LppdCur = LppdT;
    LptdCur = LptdT;
    return rVal;
}                   /* LogRegisters() */


LOGERR
LogSearch(
    LPSTR lpsz
    )
/*++

Routine Description:

    Search command:
    s <range> <value list>

    Current implementation supports bytes or string for value list,
    or specific data types in command name, i.e. sb, sd, su

Arguments:

    lpsz   - Supplies pointer to tail of command

Return Value:

    LOGERR code

--*/
{
    int        cch;
    LOG_DM     type;
    ADDR       addr0;
    ADDR       addr1;
    ULONG      Items;
    LONG       lLen;
    BYTE       buf[2 * MAX_USER_LINE];
    DWORD      cb;
    int        c;
    BYTE     * pm1;
    BYTE     * pm9;
    BYTE     * pmm;
    DWORD      nm;
    int        err;
    LPMSTREAM  lpM;
    LOGERR     rVal = LOGERROR_NOERROR;
    LPPD       LppdT = LppdCur;
    LPTD       LptdT = LptdCur;
    char       szStr[100];


    CmdInsertInit();

    PDWildInvalid();
    TDWildInvalid();
    PreRunInvalid();

    /*
    **  Check for a live debuggee
    */

    if (!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
         UpdateDebuggerState(UPDATE_CONTEXT);
    }

    type = LetterToType(*lpsz);
    if (type != LOG_DM_UNKNOWN) {
        lpsz++;
    }
    //
    // get range
    //

    err = CPGetRange(lpsz,
                     &cch,
                     &addr0,
                     &addr1,
                     &Items,
                     radix,
                     0,
                     dmfi[(type == LOG_DM_UNKNOWN) ? LOG_DM_BYTE : type].cBytes,
                     &CxfIp,
                     fCaseSensitive,
                     g_contWorkspace_WkSp.m_bMasmEval,
                     NULL);
    //
    // default not allowed here
    //
    if (err != CPNOERROR) {
        CmdLogVar(ERR_RangeExpr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    if (GetAddrOff(addr1) < GetAddrOff(addr0)) {
        CmdLogVar(ERR_RangeExpr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    lLen = (DWORD)(GetAddrOff(addr1) - GetAddrOff(addr0));

    lpsz = CPSkipWhitespace(lpsz + cch);

    if (type == LOG_DM_UNKNOWN) {
        type = (*lpsz == '"' || *lpsz == '\'') ? LOG_DM_ASCII : LOG_DM_BYTE;
    }

    err = GetValueList(lpsz, type, TRUE, buf, sizeof(buf), &cb);
    if (err != LOGERROR_NOERROR) {
        rVal = err;
        goto done;
    }

    lpM = openb(&addr0, lLen);

#define next()  (pm1 < pm9 ? ((int)(UINT)*pm1++) : (pm9=buf,getb(lpM)))

    // use match string for pushback queue after partial match
    pm1 = buf;
    pm9 = buf;

    // match count, pointer to next unmatched char
    nm = 0;
    pmm = buf;

    SetCtrlCTrap();
    //
    // if a partial match failed, scan from begin+1 for new match
    //
    while ((c = next()) != -1) {
        //
        // this ^C checking could seriously damage performance.
        // maybe it should happen in the getb macro, only on
        // buffer reloads.
        //
        if (CheckCtrlCTrap()) {
            rVal = LOGERROR_QUIET;
            break;
        }
        if (c == (int)(UINT)*pmm) {
            nm++;
            pmm++;

            if (nm == cb) {
                //
                // Hit!
                // figure out address
                //
                tellb(lpM, &addr1);
                GetAddrOff(addr1) -= nm;
                EEFormatAddress(&addr1, szStr, sizeof(szStr),
                             g_contWorkspace_WkSp.m_bShowSegVal * EEFMT_SEG);
                CmdLogFmt("%s\r\n", szStr);

                //
                // keep searching as though this match failed;
                // this will score on overlapped matches.
                //
                pm9 = pmm;
                pm1 = buf + 1;

                nm = 0;
                pmm = buf;
            }

        } else if (nm) {
            //
            // A partial match just failed.
            // we have read nm characters; the match starting at the first
            // failed, so we need to start scanning again at the second.
            //
            pm9 = pmm;
            pm1 = buf + 1;
            nm = 0;
            pmm = buf;
        }
    }
    ClearCtrlCTrap();
#undef next

done:
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        LppdCur = LppdT;
        LptdCur = LptdT;
        UpdateDebuggerState(UPDATE_CONTEXT);
    }
    return rVal;
}                   /* LogSearch() */





LOGERR
LogHelp(
    LPSTR lpsz
    )
/*++

Routine Description:

    Display help

Arguments:

    lpsz   - Supplies pointer to tail of command

Return Value:

    LOGERR code

--*/
{
    LOGERR  rVal    = LOGERROR_NOERROR;
    char   *Tok;

    CmdInsertInit();

    if ( !_strnicmp( lpsz, "elp",3 ) ) {
        lpsz+=3;
    } else {
        rVal = LOGERROR_UNKNOWN;
    }

    if ( rVal == LOGERROR_NOERROR ) {

        if ( *lpsz != 0   &&  *lpsz != ' ' && *lpsz != '\t' ) {

            rVal = LOGERROR_UNKNOWN;

        } else {

            Tok = strtok( lpsz, " \t" );

            if ( Tok ) {

                do {
                    CmdHelp( Tok );
                } while ( Tok = strtok( NULL, " \t" ) );

            } else {

                CmdHelp( NULL );
            }
        }
    }

    return rVal;
}
