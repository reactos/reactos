/* --------------- edit.c ----------- */

#include "dflat.h"

extern DBOX PrintSetup;

char DFlatApplication[] = "Edit";

static char Untitled[] = "Untitled";

static int wndpos;

static int MemoPadProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
static void NewFile(DFWINDOW,char *);
static void SelectFile(DFWINDOW);
static void PadWindow(DFWINDOW, char *);
static void OpenPadWindow(DFWINDOW, char *,char *);
static void LoadFile(DFWINDOW);
static void PrintPad(DFWINDOW);
static void SaveFile(DFWINDOW, int);
static void EditDeleteFile(DFWINDOW);
static int EditorProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
static char *NameComponent(char *);
static int PrintSetupProc(DFWINDOW, DFMESSAGE, PARAM, PARAM);
static void FixTabMenu(void);
#ifndef TURBOC
void Calendar(DFWINDOW);
#endif
//void BarChart(DFWINDOW);
char **Argv;

#define CHARSLINE 80
#define LINESPAGE 66

int main (int argc, char *argv[])
{
	DFWINDOW wnd;
	FILE *fp;
	if (!init_messages())
		return 1;
	Argv = argv;
	LoadConfig ();
//	if (!LoadConfig())
//		cfg.ScreenLines = SCREENHEIGHT;
	wnd = DfCreateWindow (APPLICATION,
	                      "FreeDos Edit " VERSION,
	                      0, 0, -1, -1,
	                      &MainMenu,
	                      NULL,
	                      MemoPadProc,
//	                      MOVEABLE  |
//	                      SIZEABLE  |
//	                      HASBORDER |
//	                      MINMAXBOX |
	                      HASSTATUSBAR);

	LoadHelpFile ();
	DfSendMessage (wnd, SETFOCUS, TRUE, 0);

	// Load the files from args - if the file does not exist, open a new window....
	while (argc > 1)
	{
		// check if the file exists....
		if (( fp = fopen(argv[1],"r")) == NULL )
		{
			// file does not exist - create new window
			NewFile(wnd,argv[1]);
		}
		else
			PadWindow(wnd, argv[1]);
		--argc;
		argv++;
	}

	while (DfDispatchMessage ())
		;

	return 0;
}

/* ------ open text files and put them into editboxes ----- */
static void PadWindow(DFWINDOW wnd, char *FileName)
{
    int ax;
    struct _finddata_t ff;
    char path[MAX_PATH];
    char *cp;

    CreatePath(path, FileName, FALSE, FALSE);
    cp = path+strlen(path);
    CreatePath(path, FileName, TRUE, FALSE);
    ax = _findfirst(path, &ff);
    if (ax == -1)
        return;
    do
    {
        strcpy(cp, ff.name);
        OpenPadWindow(wnd, path,NULL);
    }
	while (_findnext(ax, &ff) == 0);
	_findclose (ax);
}

/* ------- window processing module for the
                    Edit application window ----- */
static int MemoPadProc(DFWINDOW wnd,DFMESSAGE msg,PARAM p1,PARAM p2)
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
		case DFM_COMMAND:
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

				case ID_DELETEFILE:
					EditDeleteFile(inFocus);
					return TRUE;

				case ID_PRINTSETUP:
					DfDialogBox(wnd, &PrintSetup, TRUE, PrintSetupProc);
					return TRUE;

				case ID_PRINT:
					PrintPad(inFocus);
					return TRUE;

				case ID_EXIT:
					if (!DfYesNoBox("Exit FreeDos Edit?"))
						return FALSE;
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

				case ID_CALENDAR:
					Calendar(wnd);
					return TRUE;

//				case ID_BARCHART:
//					BarChart(wnd);
//					return TRUE;

				case ID_ABOUT:
                    DfMessageBox(
                         "About D-Flat and FreeDos Edit",
                        "   ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿\n"
                        "   ³    ÜÜÜ   ÜÜÜ     Ü    ³\n"
                        "   ³    Û  Û  Û  Û    Û    ³\n"
                        "   ³    Û  Û  Û  Û    Û    ³\n"
                        "   ³    Û  Û  Û  Û Û  Û    ³\n"
                        "   ³    ßßß   ßßß   ßß     ³\n"
                        "   RÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄU\n"
                        "D-Flat implements the SAA/CUA\n"
                        "interface in a public domain\n"
                        "C language library originally\n"
                        "published in Dr. Dobb's Journal\n"
                        "    ------------------------ \n"
                        "FreeDos Edit is a clone of MSDOS\n"
                        "editor for the FREEDOS Project");
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

/* --- The New command. Open an empty editor window --- */
static void NewFile(DFWINDOW wnd, char *FileName)
{
	OpenPadWindow(wnd, Untitled,FileName);
}

/* --- The Open... command. Select a file  --- */
static void SelectFile(DFWINDOW wnd)
{
	char FileName[MAX_PATH];

	if (OpenFileDialogBox("*.*", FileName))
	{
		/* see if the document is already in a window */
		DFWINDOW wnd1 = FirstWindow(wnd);
		while (wnd1 != NULL)
		{
			if (wnd1->extension &&
				stricmp(FileName, wnd1->extension) == 0)
			{
				DfSendMessage(wnd1, SETFOCUS, TRUE, 0);
				DfSendMessage(wnd1, RESTORE, 0, 0);
				return;
			}
			wnd1 = NextWindow(wnd1);
		}
		OpenPadWindow(wnd, FileName, NULL);
	}
}

/* --- open a document window and load a file --- */
static void OpenPadWindow(DFWINDOW wnd, char *FileName,char *NewFileName)
{
	static DFWINDOW wnd1 = NULL;
	DFWINDOW wwnd;
	struct stat sb;
	char *Fname = FileName;
	char *Fnewname = NewFileName;
	char *ermsg;

	if (strcmp(FileName, Untitled))
	{
		if (stat(FileName, &sb))
		{
			ermsg = DFmalloc(strlen(FileName)+20);
			strcpy(ermsg, "No such file as\n");
			strcat(ermsg, FileName);
			DfErrorMessage(ermsg);
			free(ermsg);
			return;
		}

		Fname = NameComponent(FileName);

		// check file size
		if (sb.st_size > 64000)
		{
			ermsg = DFmalloc(strlen(FileName)+20);
			strcpy(ermsg, "File too large for this version of Edit\n");
			DfErrorMessage(ermsg);
			free(ermsg);
			return;
		}
	}

	wwnd = WatchIcon();
	wndpos += 2;

	if (NewFileName != NULL)
		Fname = NameComponent(NewFileName);

	if (wndpos == 20)
		wndpos = 2;

	wnd1 = DfCreateWindow(EDITBOX,
	                      Fname,
	                      (wndpos-1)*2, wndpos, 10, 40,
	                      NULL, wnd, EditorProc,
	                      SHADOW |
	                      MINMAXBOX |
	                      CONTROLBOX |
	                      VSCROLLBAR |
	                      HSCROLLBAR |
	                      MOVEABLE |
	                      HASBORDER |
	                      SIZEABLE |
	                      MULTILINE);

	if (strcmp(FileName, Untitled))
	{
		wnd1->extension = DFmalloc(strlen(FileName)+1);
		strcpy(wnd1->extension, FileName);
		LoadFile(wnd1);
	}
	DfSendMessage(wwnd, CLOSE_WINDOW, 0, 0);
	DfSendMessage(wnd1, SETFOCUS, TRUE, 0);
	DfSendMessage(wnd1, MAXIMIZE, 0, 0);
}

/* --- Load the notepad file into the editor text buffer --- */
static void LoadFile(DFWINDOW wnd)
{
	char *Buf = NULL;
	int recptr = 0;
	FILE *fp;

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
		if (Buf != NULL)
		{
			DfSendMessage(wnd, SETTEXT, (PARAM) Buf, 0);
			free(Buf);
		}
	}
}

static int LineCtr;
static int CharCtr;

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
			for (i = 0; i < cfg.TopMargin; i++)
				fputc('\n', prn);
			LineCtr = cfg.TopMargin;
		}
		CharCtr = 0;
		if (c == '\n')
			return;
	}
	if (CharCtr == 0)
	{
		for (i = 0; i < cfg.LeftMargin; i++)
		{
			fputc(' ', prn);
			CharCtr++;
		}
	}
	CharCtr++;
	fputc(c, prn);
}

/* --- print the current notepad --- */
static void PrintPad(DFWINDOW wnd)
{
	if (*cfg.PrinterPort)
	{
		FILE *prn;
		if ((prn = fopen(cfg.PrinterPort, "wt")) != NULL)
		{
			long percent;
			BOOL KeepPrinting = TRUE;
			unsigned char *text = GetText(wnd);
			unsigned oldpct = 100, cct = 0, len = strlen(text);
			DFWINDOW swnd = SliderBox(20, GetTitle(wnd), "Printing");
			/* ------- print the notepad text --------- */
			LineCtr = CharCtr = 0;
			while (KeepPrinting && *text)
			{
				PrintChar(prn, *text++);
				percent = ((long) ++cct * 100) / len;
				if ((int)percent != (int)oldpct)
				{
					oldpct = (int) percent;
					KeepPrinting = DfSendMessage(swnd, PAINT, 0, oldpct);
				}
			}
			if (KeepPrinting)
				/* ---- user did not cancel ---- */
				if (oldpct < 100)
					DfSendMessage(swnd, PAINT, 0, 100);
			/* ------- follow with a form feed? --------- */
			if (DfYesNoBox("Form Feed?"))
				fputc('\f', prn);
			fclose(prn);
		}
		else
			DfErrorMessage("Cannot open printer file");
	}
	else
		DfErrorMessage("No printer selected");
}

/* ---------- save a file to disk ------------ */
static void SaveFile(DFWINDOW wnd, int Saveas)
{
    FILE *fp;
    if (wnd->extension == NULL || Saveas)    {
        char FileName[MAX_PATH];
        if (SaveAsDialogBox(FileName))    {
            if (wnd->extension != NULL)
                free(wnd->extension);
            wnd->extension = DFmalloc(strlen(FileName)+1);
            strcpy(wnd->extension, FileName);
            AddTitle(wnd, NameComponent(FileName));
            DfSendMessage(wnd, BORDER, 0, 0);
        }
        else
            return;
    }
    if (wnd->extension != NULL)
    {
        DFWINDOW mwnd = MomentaryMessage("Saving the file");
        if ((fp = fopen(wnd->extension, "wt")) != NULL)
        {
            fwrite(GetText(wnd), strlen(GetText(wnd)), 1, fp);
            fclose(fp);
            wnd->TextChanged = FALSE;
        }
        DfSendMessage(mwnd, CLOSE_WINDOW, 0, 0);
    }
}
/* -------- delete a file ------------ */
static void EditDeleteFile(DFWINDOW wnd)
{
    if (wnd->extension != NULL)    {
        if (strcmp(wnd->extension, Untitled))    {
            char *fn = NameComponent(wnd->extension);
            if (fn != NULL)    {
                char msg[30];
                sprintf(msg, "Delete %s?", fn);
                if (DfYesNoBox(msg))    {
                    unlink(wnd->extension);
                    DfSendMessage(wnd, CLOSE_WINDOW, 0, 0);
                }
            }
        }
    }
}
/* ------ display the row and column in the statusbar ------ */
static void ShowPosition(DFWINDOW wnd)
{
    char status[30];
    sprintf(status, "Line:%4d  Column: %2d",
        wnd->CurrLine, wnd->CurrCol);
    DfSendMessage(GetParent(wnd), ADDSTATUS, (PARAM) status, 0);
}

/* window processing module for the editboxes */
static int EditorProc(DFWINDOW wnd,DFMESSAGE msg,PARAM p1,PARAM p2)
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
				DfSendMessage(GetParent(wnd), ADDSTATUS, 0, 0);
			else
				ShowPosition(wnd);
			return rtn;

		case KEYBOARD_CURSOR:
			rtn = DefaultWndProc(wnd, msg, p1, p2);
			ShowPosition(wnd);
			return rtn;

		case DFM_COMMAND:
			switch ((int) p1)
			{
				case ID_SEARCH:
					DfSearchText(wnd);
					return TRUE;
				case ID_REPLACE:
					DfReplaceText(wnd);
					return TRUE;
				case ID_SEARCHNEXT:
					DfSearchNext(wnd);
					return TRUE;
				case ID_CUT:
					CopyToClipboard(wnd);
					DfSendMessage(wnd, DFM_COMMAND, ID_DELETETEXT, 0);
					DfSendMessage(wnd, PAINT, 0, 0);
					return TRUE;
				case ID_COPY:
					CopyToClipboard(wnd);
					ClearTextBlock(wnd);
					DfSendMessage(wnd, PAINT, 0, 0);
					return TRUE;
				case ID_PASTE:
					PasteFromClipboard(wnd);
					DfSendMessage(wnd, PAINT, 0, 0);
					return TRUE;
				case ID_DELETETEXT:
				case ID_CLEAR:
					rtn = DefaultWndProc(wnd, msg, p1, p2);
					DfSendMessage(wnd, PAINT, 0, 0);
					return rtn;
				case ID_HELP:
					DisplayHelp(wnd, "MEMOPADDOC");
					return TRUE;
				case ID_WRAP:
					DfSendMessage(GetParent(wnd), DFM_COMMAND, ID_WRAP, 0);
					wnd->WordWrapMode = cfg.WordWrap;
					return TRUE;
				case ID_INSERT:
					DfSendMessage(GetParent(wnd), DFM_COMMAND, ID_INSERT, 0);
					wnd->InsertMode = cfg.InsertMode;
					DfSendMessage(NULL, SHOW_CURSOR, wnd->InsertMode, 0);
					return TRUE;
				default:
					break;
			}
			break;

		case CLOSE_WINDOW:
			if (wnd->TextChanged)
			{
				char *cp = DFmalloc(25+strlen(GetTitle(wnd)));
				DfSendMessage(wnd, SETFOCUS, TRUE, 0);
				strcpy(cp, GetTitle(wnd));
				strcat(cp, "\nText changed. Save it?");
				if (DfYesNoBox(cp))
					DfSendMessage(GetParent(wnd), DFM_COMMAND, ID_SAVE, 0);
				free(cp);
			}
			wndpos = 0;
			if (wnd->extension != NULL)
			{
				free(wnd->extension);
				wnd->extension = NULL;
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

static char *ports[] = {
	"Lpt1",	"Lpt2",	"Lpt3",
	"Com1",	"Com2",	"Com3",	"Com4",
	 NULL
};

static int PrintSetupProc(DFWINDOW wnd, DFMESSAGE msg, PARAM p1, PARAM p2)
{
	int rtn, i = 0, mar;
	char marg[10];
	DFWINDOW cwnd;

	switch (msg)
	{
		case CREATE_WINDOW:
			rtn = DefaultWndProc(wnd, msg, p1, p2);
			PutItemText(wnd, ID_PRINTERPORT, cfg.PrinterPort);
			while (ports[i] != NULL)
				PutComboListText(wnd, ID_PRINTERPORT, ports[i++]);
			for (mar = CHARSLINE; mar >= 0; --mar)
			{
				sprintf(marg, "%3d", mar);
				PutItemText(wnd, ID_LEFTMARGIN, marg);
				PutItemText(wnd, ID_RIGHTMARGIN, marg);
			}
			for (mar = LINESPAGE; mar >= 0; --mar)
			{
				sprintf(marg, "%3d", mar);
				PutItemText(wnd, ID_TOPMARGIN, marg);
				PutItemText(wnd, ID_BOTTOMMARGIN, marg);
			}
			cwnd = ControlWindow(&PrintSetup, ID_LEFTMARGIN);
			DfSendMessage(cwnd, LB_SETSELECTION,
				CHARSLINE-cfg.LeftMargin, 0);
			cwnd = ControlWindow(&PrintSetup, ID_RIGHTMARGIN);
			DfSendMessage(cwnd, LB_SETSELECTION,
				CHARSLINE-cfg.RightMargin, 0);
			cwnd = ControlWindow(&PrintSetup, ID_TOPMARGIN);
			DfSendMessage(cwnd, LB_SETSELECTION,
				LINESPAGE-cfg.TopMargin, 0);
			cwnd = ControlWindow(&PrintSetup, ID_BOTTOMMARGIN);
			DfSendMessage(cwnd, LB_SETSELECTION,
				LINESPAGE-cfg.BottomMargin, 0);
			return rtn;
		case DFM_COMMAND:
			if ((int) p1 == ID_OK && (int) p2 == 0)
			{
				GetItemText(wnd, ID_PRINTERPORT, cfg.PrinterPort, 4);
				cwnd = ControlWindow(&PrintSetup, ID_LEFTMARGIN);
				cfg.LeftMargin = CHARSLINE -
					DfSendMessage(cwnd, LB_CURRENTSELECTION, 0, 0);
				cwnd = ControlWindow(&PrintSetup, ID_RIGHTMARGIN);
				cfg.RightMargin = CHARSLINE -
					DfSendMessage(cwnd, LB_CURRENTSELECTION, 0, 0);
				cwnd = ControlWindow(&PrintSetup, ID_TOPMARGIN);
				cfg.TopMargin = LINESPAGE -
					DfSendMessage(cwnd, LB_CURRENTSELECTION, 0, 0);
				cwnd = ControlWindow(&PrintSetup, ID_BOTTOMMARGIN);
				cfg.BottomMargin = LINESPAGE -
					DfSendMessage(cwnd, LB_CURRENTSELECTION, 0, 0);
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
	char *p;

	if (cp != NULL)
	{
		p = strchr(cp, '(');
		if (p != NULL)
		{
//			*(p+1) = (char)(cfg.Tabs + '0');
//			if (GetClass(inFocus) == POPDOWNMENU)
//				DfSendMessage(inFocus, PAINT, 0, 0);
		}
	}
}

void PrepFileMenu(void *w, struct Menu *mnu)
{
	DFWINDOW wnd = w;
	DeactivateCommand(&MainMenu, ID_SAVE);
	DeactivateCommand(&MainMenu, ID_SAVEAS);
	DeactivateCommand(&MainMenu, ID_DELETEFILE);
	DeactivateCommand(&MainMenu, ID_PRINT);
	if (wnd != NULL && GetClass(wnd) == EDITBOX)
	{
		if (isMultiLine(wnd))
		{
			ActivateCommand(&MainMenu, ID_SAVE);
			ActivateCommand(&MainMenu, ID_SAVEAS);
			ActivateCommand(&MainMenu, ID_DELETEFILE);
			ActivateCommand(&MainMenu, ID_PRINT);
		}
	}
}

void PrepSearchMenu(void *w, struct Menu *mnu)
{
	DFWINDOW wnd = w;
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
	DFWINDOW wnd = w;
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
			if (!TestAttribute(wnd, READONLY) && Clipboard != NULL)
				ActivateCommand(&MainMenu, ID_PASTE);
			if (wnd->DeletedText != NULL)
				ActivateCommand(&MainMenu, ID_UNDO);
		}
	}
}

/* EOF */
