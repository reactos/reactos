/*  FreeDOS Editor

    Some portions copyright (c) Joe Cosentino 2000-2004.
    Modified by Eric Auer 2002.

*/

/* I N C L U D E S /////////////////////////////////////////////////////// */

#include "dflat.h"

#ifdef __REACTOS__
#define countof(x)  (sizeof(x)/sizeof(x[0]))

/* See system.h */
void fix_mbar(MBAR *mbar);
#endif

/* D E F I N E S ///////////////////////////////////////////////////////// */

#define CHARSLINE 80
#define LINESPAGE 66

/* G L O B A L S ///////////////////////////////////////////////////////// */

extern DBOX PrintSetup;
char DFlatApplication[] = "Edit";
static char Untitled[] = "Untitled";
#ifdef __REACTOS__
static unsigned int untitledCount = 0;
#endif
static int wndpos, LineCtr, CharCtr;
static char *ports[] = {
    "Lpt1", "Lpt2", "Lpt3",
    "Com1", "Com2", "Com3", "Com4",
    NULL
};

/* P R O T O T Y P E S /////////////////////////////////////////////////// */

int classify_args(int, char *[], char *[], char *[]);
static int MemoPadProc(WINDOW, MESSAGE, PARAM, PARAM);
static void NewFile(WINDOW,char *);
static void SelectFile(WINDOW);
static void PadWindow(WINDOW, char *);
static void OpenPadWindow(WINDOW, char *,char *);
static void LoadFile(WINDOW);
static void PrintPad(WINDOW);
static void SaveFile(WINDOW, int);
static int EditorProc(WINDOW, MESSAGE, PARAM, PARAM);
static char *NameComponent(char *);
static int PrintSetupProc(WINDOW, MESSAGE, PARAM, PARAM);
static void FixTabMenu(void);

/* F U N C T I O N S ///////////////////////////////////////////////////// */

int classify_args(int argc, char *rawargs[], char *fileargs[], char *optargs[])
{
    int index, jndex, kndex;
    char *argptr;

    /* skip index=0, aka argv[0]==edit.exe */
    for (index=1,jndex=0,kndex=0;index<argc;index++)
        {
        argptr=rawargs[index];
        if (*argptr == '/')
            {
            argptr++;
            optargs[kndex++]=argptr;
            } 
        else
            {
            fileargs[jndex++]=argptr;
            } 

        }

   return kndex;

} 

int main(int argc, char *argv[])
{
    WINDOW wnd;
    FILE *fp;
    char *fileargs[64], *optargs[64];
    int n_options, n_files, index, help_flag=0;

    n_options=classify_args(argc, argv, fileargs, optargs);
    n_files=argc-n_options-1/*argv[0]*/;
    for (index=0;index<n_options;index++)
        {
        if (optargs[index][0] == '?') help_flag=1;
        else if (optargs[index][0] == 'B' || optargs[index][0] == 'b') cfg.mono=1;
        else if (optargs[index][0] == 'H' || optargs[index][0] == 'h') cfg.ScreenLines=SCREENHEIGHT;
        else if (optargs[index][0] == 'R' || optargs[index][0] == 'r') cfg.read_only=1;
        else
            {
            printf("Invalid parameter - /%s\n", strupr(optargs[index]));
            return 1;
            }

        }

    if (help_flag)
        {
        printf("FreeDOS Editor\tVersion " VERSION ".\n\n"
               "Syntax: EDIT [/B] [/R] [/?] [file(s)]\n"
               "  /B     Forces monochrome mode\n"
               "  /R     Opens a file as read only\n"
               "  /?     Displays this help message\n"
               "  [file] Specifies initial file(s) to load.  Wildcards and multiple\n"
               "         files can be given\n");
        return 1;
        }

    if (!init_messages())
        return 1;
    init_videomode();

#ifdef ENABLEGLOBALARGV
    Argv = argv;
#endif

    if (!LoadConfig())
        cfg.ScreenLines = SCREENHEIGHT;

    fix_mbar(&MainMenu);


#ifdef __REACTOS__
    // Use an empty title string for CreateWindow() to not add HASTITLEBAR.
    wnd = CreateWindow(APPLICATION, NULL/*"FreeDOS Edit"*/, 0, 0, -1, -1, &MainMenu, NULL, MemoPadProc, /*MOVEABLE | SIZEABLE | HASBORDER | MINMAXBOX |*/ HASSTATUSBAR);
    SendMessage(wnd, MAXIMIZE, 0, 0); /* Put the application window maximized */
#else
    wnd = CreateWindow(APPLICATION, "FreeDOS Edit", 0, 0, -1, -1, &MainMenu, NULL, MemoPadProc, MOVEABLE | SIZEABLE | HASBORDER | MINMAXBOX | HASSTATUSBAR);
#endif
    LoadHelpFile(DFlatApplication);
    SendMessage(wnd, SETFOCUS, TRUE, 0);
    if (cfg.loadblank)
        NewFile(wnd,NULL);

    /* Load the files from args - if the file does not exist, open a new
       window.... */
    for (index=0;index<n_files;index++)
        {
        /*  Check if the file exists... */
        /* added by Eric: Do NOT try to open files with names */
        /* that contain wildcards, otherwise you may NewFile  */
        /* files with wildcards in their names. Sigh! 11/2002 */
        fp = NULL;
        if (((strchr(fileargs[index],'*') == NULL)) && ((strchr(fileargs[index],'?') == NULL)) && ((fp = fopen(fileargs[index],"rt")) == NULL))
            {
            NewFile(wnd,fileargs[index]);
            }
        else
            {
            if (fp != NULL)
                 fclose(fp);  /* don't leave open file handle [from test if exists in above if] */

            PadWindow(wnd, fileargs[index]);
            }

        }

    while (dispatch_message())
        TRAP_TO_SCHEDULER
    ;
    uninit_videomode();

    return 0;
}

/* ------ open text files and put them into editboxes ----- */
static void PadWindow(WINDOW wnd, char *FileName)
{
    int ax,criterr=1;
    struct ffblk ff;
    char path[66],*cp;

    CreatePath(path, FileName, FALSE, FALSE);
    cp = path+strlen(path);
    CreatePath(path, FileName, TRUE, FALSE);
    while (criterr == 1)
        {
        ax = findfirst(path, &ff, 0);
        criterr = TestCriticalError();
        }

    while (ax == 0 && !criterr)
        {
        strcpy(cp, ff.ff_name);
        OpenPadWindow(wnd, path,NULL);
        ax = findnext(&ff);
        }

}

/* ------- window processing module for the Edit application window ----- */
static int MemoPadProc(WINDOW wnd,MESSAGE msg,PARAM p1,PARAM p2)
{
    int rtn;

    switch (msg)
        {
        case CREATE_WINDOW:
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            if (cfg.InsertMode)
                SetCommandToggle(&MainMenu, ID_INSERT);

            if (cfg.WordWrap)
                SetCommandToggle(&MainMenu, ID_WRAP);

            FixTabMenu();
            return rtn;
        case COMMAND:
            switch ((int)p1)
                {
                case ID_NEW:
                    NewFile(wnd,NULL);
                    return TRUE;
                case ID_OPEN:
                    SelectFile(wnd);
                    return TRUE;
                case ID_SAVE:
                    SaveFile(inFocus, FALSE);
                    return TRUE;
                case ID_SAVEAS:
                    SaveFile(inFocus, TRUE);
                    return TRUE;
                case ID_CLOSE:
                    SendMessage(inFocus, CLOSE_WINDOW, 0, 0);
                    SkipApplicationControls();
                    return TRUE;
                case ID_PRINTSETUP:
                    DialogBox(wnd, &PrintSetup, TRUE, PrintSetupProc);
                    return TRUE;
                case ID_PRINT:
                    PrintPad(inFocus);
                    return TRUE;
                case ID_EXIT:
                    break;
                case ID_WRAP:
                    cfg.WordWrap = GetCommandToggle(&MainMenu, ID_WRAP);
                    return TRUE;
                case ID_INSERT:
                    cfg.InsertMode = GetCommandToggle(&MainMenu, ID_INSERT);
                    return TRUE;
                case ID_TAB2:
                    cfg.Tabs = 2;
                    FixTabMenu();
                    return TRUE;
                case ID_TAB4:
                    cfg.Tabs = 4;
                    FixTabMenu();
                    return TRUE;
                case ID_TAB6:
                    cfg.Tabs = 6; 
                    FixTabMenu();
                    return TRUE;
                case ID_TAB8:
                    cfg.Tabs = 8;
                    FixTabMenu();
                    return TRUE;
#ifndef NOCALENDAR
                case ID_CALENDAR:
                    Calendar(wnd); 
                    return TRUE;
#endif
                case ID_ABOUT:
                    {
                    char aboutMsg[] =
                               "          FreeDOS Edit           \n"
                               "          Version @              \n"
                               "                                 \n"
                               "   FreeDOS Edit is based on the  \n"
                               "   D-Flat application published  \n"
                               "   in Dr. Dobb's Journal.        \n"
                               "                                 \n"
                               "    컴컴컴컴컴컴컴컴컴컴컴컴컴   \n"
                               "                                 \n"
                               "FreeDOS Edit is a clone of MS-DOS\n"
                               "editor for the FreeDOS Project   \n"
                               "Released under the GNU GPL License";
                    if (strchr(aboutMsg,'@') != NULL)
                        strncpy(strchr(aboutMsg,'@'), VERSION, strlen(VERSION));

                    MessageBox("About FreeDOS Edit", aboutMsg);
                    }
                    return TRUE;
                default:
                    break;

                }

            break;
        default:
            break;

        }

    return DefaultWndProc(wnd, msg, p1, p2);

}

/* The New command. Open an empty editor window */
static void NewFile(WINDOW wnd, char *FileName)
{
    OpenPadWindow(wnd, Untitled,FileName);
}

/* --- The Open... command. Select a file  --- */
static void SelectFile(WINDOW wnd)
{
    char FileName[64];

    if (OpenFileDialogBox("*.*", FileName))
        {
        /* See if the document is already in a window */
        WINDOW wnd1 = FirstWindow(wnd);

        while (wnd1 != NULL)
            {
            if (wnd1->extension && stricmp(FileName, wnd1->extension) == 0)
                {
                SendMessage(wnd1, SETFOCUS, TRUE, 0);
                SendMessage(wnd1, RESTORE, 0, 0);
                return;
                }

            wnd1 = NextWindow(wnd1);
            }

        OpenPadWindow(wnd, FileName,NULL);
        }

}

/* --- open a document window and load a file --- */
static void OpenPadWindow(WINDOW wnd, char *FileName,char *NewFileName)
{
    static WINDOW wnd1 = NULL;
    WINDOW wwnd;
    struct stat sb;
    char *Fname = FileName;
#if !defined(_WIN32)
    char *ermsg;
#endif
#ifdef __REACTOS__
    /* Buffer big enough to hold the NULL-terminated string L"4294967295",
     * corresponding to the literal 0xFFFFFFFF (MAXULONG) in decimal. */
    char newUntitled[countof(Untitled) + countof(" (4294967295)")];
#endif

    if (strcmp(FileName, Untitled))
        {
        if (stat(FileName, &sb))
            {
            NewFile(wnd,FileName);
            return;
            }

        Fname = NameComponent(FileName);

/* This limit doesn't exist in protected mode!!! */
#if !defined(_WIN32)
        /* check file size */
        if (sb.st_size > 64000UL)
            {
            ermsg = DFmalloc(strlen(FileName)+100);
            strcpy(ermsg, "File too large for this version of Edit\n");
            ErrorMessage(ermsg);
            free(ermsg);
            return;
            }
#endif
        }
#ifdef __REACTOS__
    else if (NewFileName == NULL)
    {
        untitledCount++;
        if (untitledCount > 1)
        {
            sprintf(newUntitled, "%s (%lu)", Untitled, untitledCount);
            Fname = newUntitled;
        }
        // else { Fname = Untitled; }
    }
#endif

    wwnd = WatchIcon();
    wndpos += 2;
    if (NewFileName != NULL)
        Fname = NameComponent(NewFileName);

    if (wndpos == 20)
        wndpos = 2;

    wnd1 = CreateWindow(EDITBOX, Fname,
        (wndpos-1)*2, wndpos, 10, 40,
        NULL, wnd, EditorProc,
        SHADOW     |
        MINMAXBOX  |
        CONTROLBOX |
        VSCROLLBAR |
        HSCROLLBAR |
        MOVEABLE   |
        HASBORDER  |
        SIZEABLE   |
        MULTILINE);

    if (NewFileName != NULL)
        {
        /* Either a command line new file or one that's on the 
        disk to load - Either way, must set the extension
        to the given filename */
        wnd1->extension = DFmalloc(strlen(NewFileName) + 1);
        strcpy(wnd1->extension,NewFileName);
        }
    else
        {
#ifdef __REACTOS__
        // assert(wnd1->extension == NULL);
        if (strcmp(FileName, Untitled))
        {
            wnd1->extension = DFmalloc(strlen(FileName)+1);
            strcpy(wnd1->extension, FileName);
        }
#else
        if (strcmp(FileName,Untitled) || wnd1->extension == NULL)
            wnd1->extension = DFmalloc(strlen(FileName)+1);

        strcpy(wnd1->extension, FileName);
#endif
        LoadFile(wnd1);                 /* Only load if not a new file */
        }

    SendMessage(wwnd, CLOSE_WINDOW, 0, 0);
    SendMessage(wnd1, SETFOCUS, TRUE, 0);
    SendMessage(wnd1, MAXIMIZE, 0, 0); 

}

/* --- Load the notepad file into the editor text buffer --- */
static void LoadFile(WINDOW wnd)
{
    char *Buf = NULL;
    unsigned int recptr = 0;
    FILE *fp;

#ifdef __REACTOS__
    if (wnd->extension == NULL)
#else
    if (!strcmp(wnd->extension, Untitled)) /* Not a real file load */
#endif
        {
        SendMessage(wnd, SETTEXT, (PARAM) "", 0);
        return;
        }

    if ((fp = fopen(wnd->extension, "rt")) != NULL)
        {
        while (!feof(fp))
            {
            handshake();
            Buf = DFrealloc(Buf, recptr+150);
            memset(Buf+recptr, 0, 150);
            fgets(Buf+recptr, 150, fp);
            recptr += strlen(Buf+recptr);
            }

        fclose(fp);
        if (Buf[strlen(Buf) - 1] != '\n')
            {
            Buf = DFrealloc(Buf, strlen(Buf)+2);
            Buf[strlen(Buf)] = '\n';
            Buf[strlen(Buf)] = 0;
            }

        if (Buf != NULL)
            {
            SendMessage(wnd, SETTEXT, (PARAM) Buf, 0);
            free(Buf);
            }

        }
    else
        {
        char fMsg[200];
        sprintf(fMsg, "Could not load %s", (strlen(wnd->extension)>120) ? "file" : (char *)(wnd->extension));
        ErrorMessage(fMsg);
        }

}

/* ------- print a character -------- */
static void PrintChar(FILE *prn, int c)
{
    int i;

    if (c == '\n' || CharCtr == cfg.RightMargin)
        {
        fputs("\r\n", prn);
        LineCtr++;
        if (LineCtr == cfg.BottomMargin)
            {
            fputc('\f', prn);
            for (i=0;i<cfg.TopMargin;i++)
                fputc('\n', prn);

            LineCtr=cfg.TopMargin;
            }

        CharCtr=0;
        if (c == '\n')
            return;

        }

    if (CharCtr == 0)
        {
        for (i=0;i<cfg.LeftMargin;i++)
            {
            fputc(' ', prn);
            CharCtr++;
            }

        }

    CharCtr++;
    fputc(c, prn);

}

/* --- print the current notepad --- */
static void PrintPad(WINDOW wnd)
{
    if (*cfg.PrinterPort)
        {
        FILE *prn;

        if ((prn = fopen(cfg.PrinterPort, "wt")) != NULL)
            {
            unsigned int percent;
            BOOL KeepPrinting=TRUE;
            unsigned char *text=GetText(wnd);
            unsigned int oldpct=100, cct=0, len=strlen(text);
            WINDOW swnd=SliderBox(20, GetTitle(wnd), "Printing");

            /* Print the notepad text */
            LineCtr=CharCtr=0;
            while (KeepPrinting && *text)
                {
                PrintChar(prn, *text++);
                percent=((unsigned long) ++cct * 100) / len;
                if (percent != oldpct)
                    {
                    oldpct=percent;
                    KeepPrinting=SendMessage(swnd, PAINT, 0, oldpct);
                    }

                }

            if (KeepPrinting)           /* User did not cancel */
                if (oldpct < 100)
                    SendMessage(swnd, PAINT, 0, 100);

            /* Follow with a form feed? */
            if (YesNoBox("Form Feed?"))
                fputc('\f', prn);

            fclose(prn);
            }
        else
            ErrorMessage("Cannot open printer file");

        }
    else
        ErrorMessage("No printer selected");

}

/* ---------- save a file to disk ------------ */
static void SaveFile(WINDOW wnd, int Saveas)
{
    FILE *fp;
    char FileName[64];
    struct ffblk ffblk;

    if (wnd->extension == NULL || Saveas)
        {
        if (SaveAsDialogBox("*.*", NULL, FileName))
            {
#ifdef __REACTOS__
            if (wnd->extension == NULL)
                untitledCount--;
#endif
            if (wnd->extension != NULL)
                free(wnd->extension);

            wnd->extension = DFmalloc(strlen(FileName)+1);
            strcpy(wnd->extension, FileName);
            SendMessage(wnd, BORDER, 0, 0);
            }
        else
            return;

        }

    if (wnd->extension != NULL)
        {
        WINDOW mwnd;
        tryagain:
        mwnd=MomentaryMessage("Saving...");

        if (findfirst(wnd->extension, &ffblk, 0) == 0)
            {
            char fMsg[200];

            sprintf(fMsg,"Replace existing file?");
            if (!YesNoBox(fMsg))
                {
                SendMessage(mwnd, CLOSE_WINDOW, 0, 0);
                return;
                }

            }

        if ((fp = fopen(wnd->extension, "wt")) != NULL)
            {
            size_t howmuch=strlen(GetText(wnd));

            howmuch=fwrite(GetText(wnd), howmuch, 1, fp);
            fclose(fp);
            SendMessage(mwnd, CLOSE_WINDOW, 0, 0);
            if (howmuch != 1)
                {
                ErrorMessage("Not enough room on the disk\n");
                return;
                }
            else
                {
                wnd->TextChanged = FALSE;
                }

            }
        else
            {
            char fMsg[200];

            SendMessage(mwnd, CLOSE_WINDOW, 0, 0);
            sprintf(fMsg,"   Could not save\n     '%s'\n     Try again?", (strlen(wnd->extension)>120) ? "file" : strupr(wnd->extension));
            if (YesNoBox(fMsg))
                goto tryagain;

            }

        }

}

/* ------ display the row and column in the statusbar ------ */
static void ShowPosition(WINDOW wnd)
{
    /* This is where we place the "INS" display */
    char status[64], *InsModeText, *ReadOnlyText;
    if (wnd->InsertMode)
        {
        InsModeText = "   ";            /* Not on */
        }
    else
        {
        InsModeText = "INS";            /* Insert is on */
        }

    if (cfg.read_only)
        {
        ReadOnlyText = "READ";
        }
    else
        {
        ReadOnlyText = "    ";
        }

    if (WindowWidth(wnd) < 50)          /* Auto-condense new in 0.8 */
        {
        sprintf(status, "%c Li:%6d Co:%d", ReadOnlyText[0], wnd->CurrLine, wnd->CurrCol);
        }
    else
        sprintf(status, "%4s    %3s  Line:%6d  Col:%d", ReadOnlyText, InsModeText, wnd->CurrLine+1, wnd->CurrCol+1);

    SendMessage(GetParent(wnd), ADDSTATUS, (PARAM) status, 0);

}

/* ----- window processing module for the editboxes ----- */
static int EditorProc(WINDOW wnd,MESSAGE msg,PARAM p1,PARAM p2)
{
    int rtn;

    switch (msg)
        {
        case SETFOCUS:
            if ((int)p1)
                {
                wnd->InsertMode = GetCommandToggle(&MainMenu, ID_INSERT);
                wnd->WordWrapMode = GetCommandToggle(&MainMenu, ID_WRAP);
                }

            rtn = DefaultWndProc(wnd, msg, p1, p2);
            if ((int)p1 == FALSE)
                SendMessage(GetParent(wnd), ADDSTATUS, 0, 0);
            else 
                ShowPosition(wnd);

            return rtn;

        case KEYBOARD_CURSOR:
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            ShowPosition(wnd);
            return rtn;

        case COMMAND:
            switch ((int) p1)
                {
                case ID_SEARCH:
                    SearchText(wnd);
                    return TRUE;
                case ID_REPLACE:
                    ReplaceText(wnd);
                    return TRUE;
                case ID_SEARCHNEXT:
                    SearchNext(wnd);
                    return TRUE;
                case ID_CUT:
                    CopyToClipboard(wnd);
                    SendMessage(wnd, COMMAND, ID_DELETETEXT, 0);
                    SendMessage(wnd, PAINT, 0, 0);
                    return TRUE;
                case ID_COPY:
                    CopyToClipboard(wnd);
                    ClearTextBlock(wnd);
                    SendMessage(wnd, PAINT, 0, 0);
                    return TRUE;
                case ID_PASTE:
                    PasteFromClipboard(wnd);
                    SendMessage(wnd, PAINT, 0, 0);
                    return TRUE;
                case ID_DELETETEXT:
                case ID_CLEAR:
                    rtn = DefaultWndProc(wnd, msg, p1, p2);
                    SendMessage(wnd, PAINT, 0, 0);
                    return rtn;
                case ID_HELP:
                    DisplayHelp(wnd, "MEMOPADDOC");
                    return TRUE;
                case ID_WRAP:
                    SendMessage(GetParent(wnd), COMMAND, ID_WRAP, 0);
                    wnd->WordWrapMode = cfg.WordWrap;
                    return TRUE;
                case ID_INSERT:
                    SendMessage(GetParent(wnd), COMMAND, ID_INSERT, 0);
                    wnd->InsertMode = cfg.InsertMode;
                    SendMessage(NULL, SHOW_CURSOR, wnd->InsertMode, 0);
                    return TRUE;
                default:
                    break;

                }
            break;
        case CLOSE_WINDOW:              /* If we're only closing, not exiting */
            if (cfg.read_only)
                {
                cfg.read_only=0;
                wnd->TextChanged=FALSE;
                }

#ifdef __REACTOS__
            if (wnd->extension == NULL)
                untitledCount--;
#endif

            if (wnd->TextChanged)
                {
                char *cp = DFmalloc(80+strlen(GetTitle(wnd)));
                SendMessage(wnd, SETFOCUS, TRUE, 0);
                strcpy(cp, "              The file\n            '");
                strcat(cp, GetTitle(wnd));
                strcat(cp, "'\nhas not been saved yet.  Save it now?");
                if (YesNoBox(cp))
                    SendMessage(GetParent(wnd), COMMAND, ID_SAVE, 0);

                free(cp);
                }

            wndpos=0;
            if (wnd->extension != NULL)
                {
                free(wnd->extension);
                wnd->extension=NULL;
                }
            break;
        default:
            break;

        }

    return DefaultWndProc(wnd, msg, p1, p2);

}

/* -- point to the name component of a file specification -- */
static char *NameComponent(char *FileName)
{
    char *Fname;
    if ((Fname = strrchr(FileName, '\\')) == NULL)
        if ((Fname = strrchr(FileName, ':')) == NULL)
            Fname = FileName-1;

    return Fname + 1;

}

static int PrintSetupProc(WINDOW wnd, MESSAGE msg, PARAM p1, PARAM p2)
{
    int rtn, i = 0, mar;
    char marg[10];
    WINDOW cwnd;

    switch (msg)
        {
        case CREATE_WINDOW:
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            PutItemText(wnd, ID_PRINTERPORT, cfg.PrinterPort);
            while (ports[i] != NULL)
                PutComboListText(wnd, ID_PRINTERPORT, ports[i++]);

            for (mar=CHARSLINE;mar>=0;--mar)
                {
                sprintf(marg, "%3d", mar);
                PutItemText(wnd, ID_LEFTMARGIN, marg);
                PutItemText(wnd, ID_RIGHTMARGIN, marg);
                }

            for (mar=LINESPAGE;mar>=0;--mar)
                {
                sprintf(marg, "%3d", mar);
                PutItemText(wnd, ID_TOPMARGIN, marg);
                PutItemText(wnd, ID_BOTTOMMARGIN, marg);
                }

            cwnd = ControlWindow(&PrintSetup, ID_LEFTMARGIN);
            SendMessage(cwnd, LB_SETSELECTION, CHARSLINE-cfg.LeftMargin, 0);
            cwnd = ControlWindow(&PrintSetup, ID_RIGHTMARGIN);
            SendMessage(cwnd, LB_SETSELECTION, CHARSLINE-cfg.RightMargin, 0);
            cwnd = ControlWindow(&PrintSetup, ID_TOPMARGIN);
            SendMessage(cwnd, LB_SETSELECTION, LINESPAGE-cfg.TopMargin, 0);
            cwnd = ControlWindow(&PrintSetup, ID_BOTTOMMARGIN);
            SendMessage(cwnd, LB_SETSELECTION, LINESPAGE-cfg.BottomMargin, 0);
            return rtn;

        case COMMAND:
            if ((int) p1 == ID_OK && (int) p2 == 0)
                {
                GetItemText(wnd, ID_PRINTERPORT, cfg.PrinterPort, 4);
                cwnd = ControlWindow(&PrintSetup, ID_LEFTMARGIN);
                cfg.LeftMargin = CHARSLINE - SendMessage(cwnd, LB_CURRENTSELECTION, 0, 0);
                cwnd = ControlWindow(&PrintSetup, ID_RIGHTMARGIN);
                cfg.RightMargin = CHARSLINE - SendMessage(cwnd, LB_CURRENTSELECTION, 0, 0);
                cwnd = ControlWindow(&PrintSetup, ID_TOPMARGIN);
                cfg.TopMargin = LINESPAGE - SendMessage(cwnd, LB_CURRENTSELECTION, 0, 0);
                cwnd = ControlWindow(&PrintSetup, ID_BOTTOMMARGIN);
                cfg.BottomMargin = LINESPAGE - SendMessage(cwnd, LB_CURRENTSELECTION, 0, 0);
                }

            break;
        default:
            break;

        }

    return DefaultWndProc(wnd, msg, p1, p2);

}

static void FixTabMenu(void)
{
    char *cp = GetCommandText(&MainMenu, ID_TABS);

    if (cp != NULL)
        {
        cp = strchr(cp, '(');
        if (cp != NULL)
            {
            *(cp+1) = cfg.Tabs + '0';
            if (inFocus) {
                if (GetClass(inFocus) == POPDOWNMENU)
                    SendMessage(inFocus, PAINT, 0, 0);

                }
            }
        }

}

void PrepFileMenu(void *w, struct Menu *mnu)
{
    WINDOW wnd=w; mnu=mnu;

    DeactivateCommand(&MainMenu, ID_SAVE);
    DeactivateCommand(&MainMenu, ID_SAVEAS);
    DeactivateCommand(&MainMenu, ID_CLOSE);
    DeactivateCommand(&MainMenu, ID_PRINT);
    if (wnd != NULL && GetClass(wnd) == EDITBOX)
        {
        if (isMultiLine(wnd))
            {
            if (!cfg.read_only)
                {
                ActivateCommand(&MainMenu, ID_SAVE);
                ActivateCommand(&MainMenu, ID_SAVEAS);
                }

            ActivateCommand(&MainMenu, ID_CLOSE);
            ActivateCommand(&MainMenu, ID_PRINT);
            }

        }

}

void PrepSearchMenu(void *w, struct Menu *mnu)
{
    WINDOW wnd=w; mnu=mnu;

    DeactivateCommand(&MainMenu, ID_SEARCH);
    DeactivateCommand(&MainMenu, ID_REPLACE);
    DeactivateCommand(&MainMenu, ID_SEARCHNEXT);
    if (wnd != NULL && GetClass(wnd) == EDITBOX)
        {
        if (isMultiLine(wnd))
            {
            ActivateCommand(&MainMenu, ID_SEARCH);
            ActivateCommand(&MainMenu, ID_REPLACE);
            ActivateCommand(&MainMenu, ID_SEARCHNEXT);
            }

        }

}

void PrepEditMenu(void *w, struct Menu *mnu)
{
    WINDOW wnd=w; mnu=mnu;

    DeactivateCommand(&MainMenu, ID_CUT);
    DeactivateCommand(&MainMenu, ID_COPY);
    DeactivateCommand(&MainMenu, ID_CLEAR);
    DeactivateCommand(&MainMenu, ID_DELETETEXT);
    DeactivateCommand(&MainMenu, ID_PARAGRAPH);
    DeactivateCommand(&MainMenu, ID_PASTE);
    DeactivateCommand(&MainMenu, ID_UNDO);
    if (wnd != NULL && GetClass(wnd) == EDITBOX)
        {
        if (isMultiLine(wnd))
            {
            if (TextBlockMarked(wnd))
                {
                ActivateCommand(&MainMenu, ID_CUT);
                ActivateCommand(&MainMenu, ID_COPY);
                ActivateCommand(&MainMenu, ID_CLEAR);
                ActivateCommand(&MainMenu, ID_DELETETEXT);
                }

            ActivateCommand(&MainMenu, ID_PARAGRAPH);
            if (!TestAttribute(wnd, READONLY) && ReadClipboard() != NULL)
                ActivateCommand(&MainMenu, ID_PASTE);

            if (wnd->DeletedText != NULL)
                ActivateCommand(&MainMenu, ID_UNDO);

            }

        }

}
