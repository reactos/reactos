/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/cmdcons.c
 * PURPOSE:         Recovery console
 * PROGRAMMER:      Eric Kohl
 */

#include "usetup.h"

#define NDEBUG
#include <debug.h>


//#define FEATURE_HISTORY

typedef struct _CONSOLE_STATE
{
    SHORT maxx;
    SHORT maxy;
    BOOLEAN bInsert;
    BOOLEAN bExit;
} CONSOLE_STATE, *PCONSOLE_STATE;

typedef struct tagCOMMAND
{
    LPSTR name;
    INT flags;
    INT (*func)(PCONSOLE_STATE, LPSTR);
} COMMAND, *LPCOMMAND;


static
INT
CommandCls(
    PCONSOLE_STATE State,
    LPSTR param);

static
INT
CommandDumpSector(
    PCONSOLE_STATE State,
    LPSTR param);

static
INT
CommandExit(
    PCONSOLE_STATE State,
    LPSTR param);

static
INT
CommandHelp(
    PCONSOLE_STATE State,
    LPSTR param);

COMMAND
Commands[] =
{
    {"cls", 0, CommandCls},
    {"dumpsector", 0, CommandDumpSector},
    {"exit", 0, CommandExit},
    {"help", 0, CommandHelp},
    {NULL, 0, NULL}
};


static
VOID
freep(
    LPSTR *p)
{
    LPSTR *q;

    if (!p)
        return;

    q = p;
    while (*q)
        RtlFreeHeap(ProcessHeap, 0, *q++);

    RtlFreeHeap(ProcessHeap, 0, p);
}


static
VOID
StripQuotes(
    LPSTR in)
{
    LPSTR out = in;

    for (; *in; in++)
    {
        if (*in != '"')
            *out++ = *in;
    }

    *out = '\0';
}


BOOL
add_entry(
    LPINT ac,
    LPSTR **arg,
    LPCSTR entry)
{
    LPSTR q;
    LPSTR *oldarg;

    q = RtlAllocateHeap(ProcessHeap, 0, strlen(entry) + 1);
    if (q == NULL)
        return FALSE;

    strcpy(q, entry);
    oldarg = *arg;
    *arg = RtlReAllocateHeap(ProcessHeap, 0, oldarg, (*ac + 2) * sizeof(LPSTR));
    if (*arg == NULL)
    {
        RtlFreeHeap(ProcessHeap, 0, q);
        *arg = oldarg;
        return FALSE;
    }

    /* save new entry */
    (*arg)[*ac] = q;
    (*arg)[++(*ac)] = NULL;

    return TRUE;
}

static
LPSTR *
split(
    LPSTR s,
    LPINT args)
{
    LPSTR *arg;
    LPSTR start;
    LPSTR q;
    INT ac;
    INT_PTR len;
    BOOL bQuoted;

    arg = RtlAllocateHeap(ProcessHeap, 0 , sizeof(LPTSTR));
    if (arg == NULL)
        return NULL;

    *arg = NULL;

    ac = 0;
    while (*s)
    {
        bQuoted = FALSE;

        /* skip leading spaces */
        while (*s && (isspace(*s) || iscntrl(*s)))
            ++s;

        start = s;

        /* the first character can be '/' */
        if (*s == '/')
            s++;

        /* skip to next word delimiter or start of next option */
        while (isprint(*s))
        {
            /* if quote (") then set bQuoted */
            bQuoted ^= (*s == '\"');

            /* Check if we have unquoted text */
            if (!bQuoted)
            {
                /* check for separators */
                if (isspace(*s) || (*s == '/'))
                {
                    /* Make length at least one character */
                    if (s == start)
                        s++;
                    break;
                }
            }

            s++;
        }

        /* a word was found */
        if (s != start)
        {
            len = s - start;
            q = RtlAllocateHeap(ProcessHeap, 0, len + 1);
            if (q == NULL)
            {
                freep(arg);
                return NULL;
            }

            memcpy(q, start, len);
            q[len] = '\0';

            StripQuotes(q);

            if (!add_entry(&ac, &arg, q))
            {
                RtlFreeHeap(ProcessHeap, 0, q);
                freep(arg);
                return NULL;
            }

            RtlFreeHeap(ProcessHeap, 0, q);
        }
    }

    *args = ac;

    return arg;
}


static
INT
CommandCls(
    PCONSOLE_STATE State,
    LPSTR param)
{
#if 0
    HANDLE hOutput;
    COORD coPos;
    DWORD dwWritten;

#if 0
    if (!strncmp(param, "/?", 2))
    {
        ConOutResPaging(TRUE,STRING_CLS_HELP);
        return 0;
    }
#endif

    coPos.X = 0;
    coPos.Y = 0;

    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    FillConsoleOutputAttribute(hOutput, csbi.wAttributes,
                               State->maxx * State->maxy,
                               coPos, &dwWritten);
    FillConsoleOutputCharacter(hOutput, ' ',
                               State->maxx * State->maxy,
                               coPos, &dwWritten);
    SetConsoleCursorPosition(hOutput, coPos);
#endif

    CONSOLE_ClearScreen();
    CONSOLE_SetCursorXY(0, 0);

    return 0;
}


void HexDump(PUCHAR buffer, ULONG size)
{
    ULONG offset = 0;
    PUCHAR ptr;

    while (offset < (size & ~15))
    {
        ptr = (PUCHAR)((ULONG_PTR)buffer + offset);
        CONSOLE_ConOutPrintf("%04lx  %02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx %02hx\n",
               offset,
               ptr[0],
               ptr[1],
               ptr[2],
               ptr[3],
               ptr[4],
               ptr[5],
               ptr[6],
               ptr[7],
               ptr[8],
               ptr[9],
               ptr[10],
               ptr[11],
               ptr[12],
               ptr[13],
               ptr[14],
               ptr[15]);
        offset += 16;
    }

    if (offset < size)
    {
        ptr = (PUCHAR)((ULONG_PTR)buffer + offset);
        CONSOLE_ConOutPrintf("%04lx ", offset);
        while (offset < size)
        {
            CONSOLE_ConOutPrintf(" %02hx", *ptr);
            offset++;
            ptr++;
        }

        CONSOLE_ConOutPrintf("\n");
    }

    CONSOLE_ConOutPrintf("\n");
}

static
INT
CommandDumpSector(
    PCONSOLE_STATE State,
    LPSTR param)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING PathName;
    HANDLE hDisk = NULL;
    DISK_GEOMETRY DiskGeometry;
    NTSTATUS Status;

    LPTSTR *argv = NULL;
    INT argc = 0;
    WCHAR DriveName[40];
    ULONG ulDrive;
//    ULONG ulSector;
    LARGE_INTEGER Sector, SectorCount, Offset;
    PUCHAR Buffer = NULL;

    DPRINT1("param: %s\n", param);

    if (!strncmp(param, "/?", 2))
    {
        CONSOLE_ConOutPrintf("DUMPSECT DiskNumber Sector\n\nDumps a disk sector to the screen.\n\n");
        return 0;
    }

    argv = split(param, &argc);

    DPRINT1("argc: %d\n", argc);
    DPRINT1("argv: %p\n", argv);

    if (argc != 2)
    {
        goto done;
    }

    DPRINT1("Device: %s\n", argv[0]);
    DPRINT1("Sector: %s\n", argv[1]);

    ulDrive = strtoul(argv[0], NULL, 0);
//    ulSector = strtoul(argv[1], NULL, 0);
    Sector.QuadPart = _atoi64(argv[1]);

    /* Build full drive name */
//    swprintf(DriveName, L"\\\\.\\PHYSICALDRIVE%lu", ulDrive);
    swprintf(DriveName, L"\\Device\\Harddisk%lu\\Partition0", ulDrive);

    RtlInitUnicodeString(&PathName,
                         DriveName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &PathName,
                               OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
                               NULL,
                               NULL);

    Status = NtOpenFile(&hDisk,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE | FILE_RANDOM_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateFile failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    Status = NtDeviceIoControlFile(hDisk,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                   NULL,
                                   0,
                                   &DiskGeometry,
                                   sizeof(DISK_GEOMETRY));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDeviceIoControlFile failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    DPRINT1("Drive number: %lu\n", ulDrive);
    DPRINT1("Cylinders: %I64u\nMediaType: %x\nTracksPerCylinder: %lu\n"
            "SectorsPerTrack: %lu\nBytesPerSector: %lu\n\n",
            DiskGeometry.Cylinders.QuadPart,
            DiskGeometry.MediaType,
            DiskGeometry.TracksPerCylinder,
            DiskGeometry.SectorsPerTrack,
            DiskGeometry.BytesPerSector);

    DPRINT1("Sector: %I64u\n", Sector.QuadPart);

    SectorCount.QuadPart = DiskGeometry.Cylinders.QuadPart *
                           DiskGeometry.TracksPerCylinder,
                           DiskGeometry.SectorsPerTrack;
    if (Sector.QuadPart >= SectorCount.QuadPart)
    {
        CONSOLE_ConOutPrintf("Invalid sector number! Valid range: [0 - %I64u]\n", SectorCount.QuadPart - 1);
        goto done;
    }

    Buffer = RtlAllocateHeap(ProcessHeap, 0, DiskGeometry.BytesPerSector);
    if (Buffer == NULL)
    {
        DPRINT1("Buffer allocation failed\n");
        goto done;
    }


    Offset.QuadPart = Sector.QuadPart * DiskGeometry.BytesPerSector;
    DPRINT1("Offset: %I64u\n", Offset.QuadPart);

    Status = NtReadFile(hDisk,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        Buffer,
                        DiskGeometry.BytesPerSector,
                        &Offset,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtReadFile failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    HexDump(Buffer, DiskGeometry.BytesPerSector);

done:
    if (Buffer != NULL)
        RtlFreeHeap(ProcessHeap, 0, Buffer);

    if (hDisk != NULL)
        NtClose(hDisk);

    freep(argv);

    return 0;
}


static
INT
CommandExit(
    PCONSOLE_STATE State,
    LPSTR param)
{
#if 0
    if (!strncmp(param, "/?", 2))
    {
        ConOutResPaging(TRUE,STRING_EXIT_HELP);
        /* Just make sure */
        bExit = FALSE;
        /* Dont exit */
        return 0;
    }
#endif

    State->bExit = TRUE;

    return 0;
}


static
INT
CommandHelp(
    PCONSOLE_STATE State,
    LPSTR param)
{
    CONSOLE_ConOutPrintf("CLS\n");
    CONSOLE_ConOutPrintf("DUMPSECTOR\n");
    CONSOLE_ConOutPrintf("EXIT\n");
    CONSOLE_ConOutPrintf("HELP\n");
    CONSOLE_ConOutPrintf("\n");

    return 0;
}


static
VOID
ClearCommandLine(
    LPSTR str,
    INT maxlen,
    SHORT orgx,
    SHORT orgy)
{
    INT count;

    CONSOLE_SetCursorXY(orgx, orgy);
    for (count = 0; count < (INT)strlen(str); count++)
        CONSOLE_ConOutChar(' ');
    memset(str, 0, maxlen);
    CONSOLE_SetCursorXY(orgx, orgy);
}


static
BOOL
ReadCommand(
    PCONSOLE_STATE State,
    LPSTR str,
    INT maxlen)
{
    SHORT orgx;     /* origin x/y */
    SHORT orgy;
    SHORT curx;     /*current x/y cursor position*/
    SHORT cury;
    SHORT tempscreen;
    INT   count;    /*used in some for loops*/
    INT   current = 0;  /*the position of the cursor in the string (str)*/
    INT   charcount = 0;/*chars in the string (str)*/
    INPUT_RECORD ir;
    CHAR  ch;
    BOOL bReturn = FALSE;
    BOOL bCharInput;
#ifdef FEATURE_HISTORY
    //BOOL bContinue=FALSE;/*is TRUE the second case will not be executed*/
    CHAR PreviousChar;
#endif


    CONSOLE_GetCursorXY(&orgx, &orgy);
    curx = orgx;
    cury = orgy;

    memset(str, 0, maxlen * sizeof(CHAR));

    CONSOLE_SetCursorType(State->bInsert, TRUE);

    do
    {
        bReturn = FALSE;
        CONSOLE_ConInKey(&ir);

        if (ir.Event.KeyEvent.dwControlKeyState &
            (RIGHT_ALT_PRESSED |LEFT_ALT_PRESSED|
             RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED) )
        {
            switch (ir.Event.KeyEvent.wVirtualKeyCode)
            {
#ifdef FEATURE_HISTORY
                case 'K':
                    /*add the current command line to the history*/
                    if (ir.Event.KeyEvent.dwControlKeyState &
                        (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
                    {
                        if (str[0])
                            History(0,str);

                        ClearCommandLine (str, maxlen, orgx, orgy);
                        current = charcount = 0;
                        curx = orgx;
                        cury = orgy;
                        //bContinue=TRUE;
                        break;
                    }

                case 'D':
                    /*delete current history entry*/
                    if (ir.Event.KeyEvent.dwControlKeyState &
                        (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
                    {
                        ClearCommandLine (str, maxlen, orgx, orgy);
                        History_del_current_entry(str);
                        current = charcount = strlen (str);
                        ConOutPrintf("%s", str);
                        GetCursorXY(&curx, &cury);
                        //bContinue=TRUE;
                        break;
                    }

#endif /*FEATURE_HISTORY*/
            }
        }

        bCharInput = FALSE;

        switch (ir.Event.KeyEvent.wVirtualKeyCode)
        {
            case VK_BACK:
                /* <BACKSPACE> - delete character to left of cursor */
                if (current > 0 && charcount > 0)
                {
                    if (current == charcount)
                    {
                        /* if at end of line */
                        str[current - 1] = L'\0';
                        if (CONSOLE_GetCursorX () != 0)
                        {
                            CONSOLE_ConOutPrintf("\b \b");
                            curx--;
                        }
                        else
                        {
                            CONSOLE_SetCursorXY((SHORT)(State->maxx - 1), (SHORT)(CONSOLE_GetCursorY () - 1));
                            CONSOLE_ConOutChar(' ');
                            CONSOLE_SetCursorXY((SHORT)(State->maxx - 1), (SHORT)(CONSOLE_GetCursorY () - 1));
                            cury--;
                            curx = State->maxx - 1;
                        }
                    }
                    else
                    {
                        for (count = current - 1; count < charcount; count++)
                            str[count] = str[count + 1];
                        if (CONSOLE_GetCursorX () != 0)
                        {
                            CONSOLE_SetCursorXY ((SHORT)(CONSOLE_GetCursorX () - 1), CONSOLE_GetCursorY ());
                            curx--;
                        }
                        else
                        {
                            CONSOLE_SetCursorXY ((SHORT)(State->maxx - 1), (SHORT)(CONSOLE_GetCursorY () - 1));
                            cury--;
                            curx = State->maxx - 1;
                        }
                        CONSOLE_GetCursorXY(&curx, &cury);
                        CONSOLE_ConOutPrintf("%s ", &str[current - 1]);
                        CONSOLE_SetCursorXY(curx, cury);
                    }
                    charcount--;
                    current--;
                }
                break;

            case VK_INSERT:
                /* toggle insert/overstrike mode */
                State->bInsert ^= TRUE;
                CONSOLE_SetCursorType(State->bInsert, TRUE);
                break;

            case VK_DELETE:
                /* delete character under cursor */
                if (current != charcount && charcount > 0)
                {
                    for (count = current; count < charcount; count++)
                        str[count] = str[count + 1];
                    charcount--;
                    CONSOLE_GetCursorXY(&curx, &cury);
                    CONSOLE_ConOutPrintf("%s ", &str[current]);
                    CONSOLE_SetCursorXY(curx, cury);
                }
                break;

            case VK_HOME:
                /* goto beginning of string */
                if (current != 0)
                {
                    CONSOLE_SetCursorXY(orgx, orgy);
                    curx = orgx;
                    cury = orgy;
                    current = 0;
                }
                break;

            case VK_END:
                /* goto end of string */
                if (current != charcount)
                {
                    CONSOLE_SetCursorXY(orgx, orgy);
                    CONSOLE_ConOutPrintf("%s", str);
                    CONSOLE_GetCursorXY(&curx, &cury);
                    current = charcount;
                }
                break;

            case 'M':
            case 'C':
                /* ^M does the same as return */
                bCharInput = TRUE;
                if (!(ir.Event.KeyEvent.dwControlKeyState &
                    (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED)))
                {
                    break;
                }

            case VK_RETURN:
                /* end input, return to main */
#ifdef FEATURE_HISTORY
                /* add to the history */
                if (str[0])
                    History (0, str);
#endif
                str[charcount++] = '\n';
                str[charcount] = '\0';
                CONSOLE_ConOutChar('\n');
                bReturn = TRUE;
                break;

            case VK_ESCAPE:
                /* clear str  Make this callable! */
                ClearCommandLine (str, maxlen, orgx, orgy);
                curx = orgx;
                cury = orgy;
                current = charcount = 0;
                break;

#ifdef FEATURE_HISTORY
            case VK_F3:
                History_move_to_bottom();
#endif
            case VK_UP:
#ifdef FEATURE_HISTORY
                /* get previous command from buffer */
                ClearCommandLine (str, maxlen, orgx, orgy);
                History (-1, str);
                current = charcount = strlen (str);
                if (((charcount + orgx) / maxx) + orgy > maxy - 1)
                    orgy += maxy - ((charcount + orgx) / maxx + orgy + 1);
                CONSOLE_ConOutPrintf("%s", str);
                CONSOLE_GetCursorXY(&curx, &cury);
#endif
                break;

            case VK_DOWN:
#ifdef FEATURE_HISTORY
                /* get next command from buffer */
                ClearCommandLine (str, maxlen, orgx, orgy);
                History (1, str);
                current = charcount = strlen (str);
                if (((charcount + orgx) / maxx) + orgy > maxy - 1)
                    orgy += maxy - ((charcount + orgx) / maxx + orgy + 1);
                CONSOLE_ConOutPrintf("%s", str);
                CONSOLE_GetCursorXY(&curx, &cury);
#endif
                break;

            case VK_LEFT:
                /* move cursor left */
                if (current > 0)
                {
                    current--;
                    if (CONSOLE_GetCursorX() == 0)
                    {
                        CONSOLE_SetCursorXY((SHORT)(State->maxx - 1), (SHORT)(CONSOLE_GetCursorY () - 1));
                        curx = State->maxx - 1;
                        cury--;
                    }
                    else
                    {
                        CONSOLE_SetCursorXY((SHORT)(CONSOLE_GetCursorX () - 1), CONSOLE_GetCursorY ());
                        curx--;
                    }
                }
                break;

            case VK_RIGHT:
                /* move cursor right */
                if (current != charcount)
                {
                    current++;
                    if (CONSOLE_GetCursorX() == State->maxx - 1)
                    {
                        CONSOLE_SetCursorXY(0, (SHORT)(CONSOLE_GetCursorY () + 1));
                        curx = 0;
                        cury++;
                    }
                    else
                    {
                        CONSOLE_SetCursorXY((SHORT)(CONSOLE_GetCursorX () + 1), CONSOLE_GetCursorY ());
                        curx++;
                    }
                }
#ifdef FEATURE_HISTORY
                else
                {
                    LPCSTR last = PeekHistory(-1);
                    if (last && charcount < (INT)strlen (last))
                    {
                        PreviousChar = last[current];
                        CONSOLE_ConOutChar(PreviousChar);
                        CONSOLE_GetCursorXY(&curx, &cury);
                        str[current++] = PreviousChar;
                        charcount++;
                    }
                }
#endif
                break;

            default:
                /* This input is just a normal char */
                bCharInput = TRUE;

        }

        ch = ir.Event.KeyEvent.uChar.UnicodeChar;
        if (ch >= 32 && (charcount != (maxlen - 2)) && bCharInput)
        {
            /* insert character into string... */
            if (State->bInsert && current != charcount)
            {
                /* If this character insertion will cause screen scrolling,
                 * adjust the saved origin of the command prompt. */
                tempscreen = strlen(str + current) + curx;
                if ((tempscreen % State->maxx) == (State->maxx - 1) &&
                    (tempscreen / State->maxx) + cury == (State->maxy - 1))
                {
                    orgy--;
                    cury--;
                }

                for (count = charcount; count > current; count--)
                    str[count] = str[count - 1];
                str[current++] = ch;
                if (curx == State->maxx - 1)
                    curx = 0, cury++;
                else
                    curx++;
                CONSOLE_ConOutPrintf("%s", &str[current - 1]);
                CONSOLE_SetCursorXY(curx, cury);
                charcount++;
            }
            else
            {
                if (current == charcount)
                    charcount++;
                str[current++] = ch;
                if (CONSOLE_GetCursorX () == State->maxx - 1 && CONSOLE_GetCursorY () == State->maxy - 1)
                    orgy--, cury--;
                if (CONSOLE_GetCursorX () == State->maxx - 1)
                    curx = 0, cury++;
                else
                    curx++;
                CONSOLE_ConOutChar(ch);
            }
        }
    }
    while (!bReturn);

    CONSOLE_SetCursorType(State->bInsert, TRUE);

    return TRUE;
}


static
BOOL
IsDelimiter(
    CHAR c)
{
    return (c == '/' || c == '=' || c == '\0' || isspace(c));
}


static
VOID
DoCommand(
    PCONSOLE_STATE State,
    LPSTR line)
{
    CHAR com[MAX_PATH]; /* the first word in the command */
    LPSTR cp = com;
//    LPSTR cstart;
    LPSTR rest = line; /* pointer to the rest of the command line */
//    INT cl;
    LPCOMMAND cmdptr;

    DPRINT1("DoCommand: (\'%s\')\n", line);

    /* Skip over initial white space */
    while (isspace(*rest))
        rest++;

//    cstart = rest;

    /* Anything to do ? */
    if (*rest)
    {
        /* Copy over 1st word as lower case */
        while (!IsDelimiter(*rest))
            *cp++ = tolower(*rest++);

        /* Terminate first word */
        *cp = '\0';

        /* Skip over whitespace to rest of line */
        while (isspace (*rest))
            rest++;

        /* Scan internal command table */
        for (cmdptr = Commands; ; cmdptr++)
        {
            /* If end of table execute ext cmd */
            if (cmdptr->name == NULL)
            {
                CONSOLE_ConOutPuts("Unknown command. Enter HELP to get a list of commands.");
                break;
            }

            if (strcmp(com, cmdptr->name) == 0)
            {
                cmdptr->func(State, rest);
                break;
            }

#if 0
            /* The following code handles the case of commands like CD which
             * are recognised even when the command name and parameter are
             * not space separated.
             *
             * e.g dir..
             * cd\freda
             */

            /* Get length of command name */
            cl = strlen(cmdptr->name);

            if ((cmdptr->flags & CMD_SPECIAL) &&
                (!strncmp (cmdptr->name, com, cl)) &&
                (strchr("\\.-", *(com + cl))))
            {
                /* OK its one of the specials...*/

                /* Call with new rest */
                cmdptr->func(State, cstart + cl);
                break;
            }
#endif
        }
    }
}


VOID
RecoveryConsole(VOID)
{
    CHAR szInputBuffer[256];
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    CONSOLE_STATE State;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    /* get screen size */
    State.maxx = csbi.dwSize.X;
    State.maxy = csbi.dwSize.Y;
    State.bInsert = TRUE;
    State.bExit = FALSE;

    CONSOLE_ClearScreen();
    CONSOLE_SetCursorXY(0, 0);

    CONSOLE_ConOutPrintf("ReactOS Recovery Console\n\nEnter HELP to get a list of commands.\n\n");

    while (!State.bExit)
    {
        /* Prompt */
        CONSOLE_ConOutPrintf(">");

        ReadCommand(&State, szInputBuffer, 256);
DPRINT1("%s\n", szInputBuffer);

        DoCommand(&State, szInputBuffer);

//        Cmd = ParseCommand(NULL);
//        if (!Cmd)
//            continue;

//        ExecuteCommand(Cmd);
//        FreeCommand(Cmd);
    }
}

/* EOF */
