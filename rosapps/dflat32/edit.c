/* --------------- edit.c ----------- */

#include "dflat.h"

extern DF_DBOX PrintSetup;

char DFlatApplication[] = "Edit";

static char Untitled[] = "Untitled";

static int wndpos;

static int MemoPadProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
static void NewFile(DFWINDOW,char *);
static void SelectFile(DFWINDOW);
static void PadWindow(DFWINDOW, char *);
static void OpenPadWindow(DFWINDOW, char *,char *);
static void LoadFile(DFWINDOW);
static void PrintPad(DFWINDOW);
static void SaveFile(DFWINDOW, int);
static void EditDeleteFile(DFWINDOW);
static int EditorProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
static char *NameComponent(char *);
static int PrintSetupProc(DFWINDOW, DFMESSAGE, DF_PARAM, DF_PARAM);
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

	if (DfInitialize () == FALSE)
		return 1;

	Argv = argv;
	DfLoadConfig ();
//	if (!DfLoadConfig())
//		DfCfg.ScreenLines = DF_SCREENHEIGHT;
	wnd = DfDfCreateWindow (DF_APPLICATION,
	                      "FreeDos Edit " DF_VERSION,
	                      0, 0, -1, -1,
	                      &DfMainMenu,
	                      NULL,
	                      MemoPadProc,
//	                      DF_MOVEABLE  |
//	                      DF_SIZEABLE  |
//	                      DF_HASBORDER |
//	                      DF_MINMAXBOX |
	                      DF_HASSTATUSBAR);

	DfLoadHelpFile ();
	DfSendMessage (wnd, DFM_SETFOCUS, TRUE, 0);

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

	DfTerminate ();

	return 0;
}

/* ------ open text files and put them into editboxes ----- */
static void PadWindow(DFWINDOW wnd, char *FileName)
{
    int ax;
    struct _finddata_t ff;
    char path[MAX_PATH];
    char *cp;

    DfCreatePath(path, FileName, FALSE, FALSE);
    cp = path+strlen(path);
    DfCreatePath(path, FileName, TRUE, FALSE);
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
static int MemoPadProc(DFWINDOW wnd,DFMESSAGE msg,DF_PARAM p1,DF_PARAM p2)
{
	int rtn;
	switch (msg)
	{
		case DFM_CREATE_WINDOW:
			rtn = DfDefaultWndProc(wnd, msg, p1, p2);
			if (DfCfg.InsertMode)
				DfSetCommandToggle(&DfMainMenu, DF_ID_INSERT);
			if (DfCfg.WordWrap)
				DfSetCommandToggle(&DfMainMenu, DF_ID_WRAP);
			FixTabMenu();
			return rtn;
		case DFM_COMMAND:
			switch ((int)p1)
			{
				case DF_ID_NEW:
					NewFile(wnd,NULL);
					return TRUE;

				case DF_ID_OPEN:
					SelectFile(wnd);
					return TRUE;

				case DF_ID_SAVE:
					SaveFile(DfInFocus, FALSE);
					return TRUE;

				case DF_ID_SAVEAS:
					SaveFile(DfInFocus, TRUE);
					return TRUE;

				case DF_ID_DELETEFILE:
					EditDeleteFile(DfInFocus);
					return TRUE;

				case DF_ID_PRINTSETUP:
					DfDialogBox(wnd, &PrintSetup, TRUE, PrintSetupProc);
					return TRUE;

				case DF_ID_PRINT:
					PrintPad(DfInFocus);
					return TRUE;

				case DF_ID_EXIT:
					if (!DfYesNoBox("Exit FreeDos Edit?"))
						return FALSE;
					break;

				case DF_ID_WRAP:
					DfCfg.WordWrap = DfGetCommandToggle(&DfMainMenu, DF_ID_WRAP);
					return TRUE;

				case DF_ID_INSERT:
					DfCfg.InsertMode = DfGetCommandToggle(&DfMainMenu, DF_ID_INSERT);
					return TRUE;

				case DF_ID_TAB2:
					DfCfg.Tabs = 2;
					FixTabMenu();
					return TRUE;

				case DF_ID_TAB4:
					DfCfg.Tabs = 4;
					FixTabMenu();
					return TRUE;

				case DF_ID_TAB6:
					DfCfg.Tabs = 6;
					FixTabMenu();
					return TRUE;

				case DF_ID_TAB8:
					DfCfg.Tabs = 8;
					FixTabMenu();
					return TRUE;

				case DF_ID_CALENDAR:
					Calendar(wnd);
					return TRUE;

//				case DF_ID_BARCHART:
//					BarChart(wnd);
//					return TRUE;

				case DF_ID_ABOUT:
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

	return DfDefaultWndProc(wnd, msg, p1, p2);
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

	if (DfOpenFileDialogBox("*.*", FileName))
	{
		/* see if the document is already in a window */
		DFWINDOW wnd1 = DfFirstWindow(wnd);
		while (wnd1 != NULL)
		{
			if (wnd1->extension &&
				stricmp(FileName, wnd1->extension) == 0)
			{
				DfSendMessage(wnd1, DFM_SETFOCUS, TRUE, 0);
				DfSendMessage(wnd1, DFM_RESTORE, 0, 0);
				return;
			}
			wnd1 = DfNextWindow(wnd1);
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
			ermsg = DfMalloc(strlen(FileName)+20);
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
			ermsg = DfMalloc(strlen(FileName)+20);
			strcpy(ermsg, "File too large for this version of Edit\n");
			DfErrorMessage(ermsg);
			free(ermsg);
			return;
		}
	}

	wwnd = DfWatchIcon();
	wndpos += 2;

	if (NewFileName != NULL)
		Fname = NameComponent(NewFileName);

	if (wndpos == 20)
		wndpos = 2;

	wnd1 = DfDfCreateWindow(DF_EDITBOX,
	                      Fname,
	                      (wndpos-1)*2, wndpos, 10, 40,
	                      NULL, wnd, EditorProc,
	                      DF_SHADOW |
	                      DF_MINMAXBOX |
	                      DF_CONTROLBOX |
	                      DF_VSCROLLBAR |
	                      DF_HSCROLLBAR |
	                      DF_MOVEABLE |
	                      DF_HASBORDER |
	                      DF_SIZEABLE |
	                      DF_MULTILINE);

	if (strcmp(FileName, Untitled))
	{
		wnd1->extension = DfMalloc(strlen(FileName)+1);
		strcpy(wnd1->extension, FileName);
		LoadFile(wnd1);
	}
	DfSendMessage(wwnd, DFM_CLOSE_WINDOW, 0, 0);
	DfSendMessage(wnd1, DFM_SETFOCUS, TRUE, 0);
	DfSendMessage(wnd1, DFM_MAXIMIZE, 0, 0);
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
			DfHandshake();
			Buf = DfRealloc(Buf, recptr+150);
			memset(Buf+recptr, 0, 150);
			fgets(Buf+recptr, 150, fp);
			recptr += strlen(Buf+recptr);
		}
		fclose(fp);
		if (Buf != NULL)
		{
			DfSendMessage(wnd, DFM_SETTEXT, (DF_PARAM) Buf, 0);
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

	if (c == '\n' || CharCtr == DfCfg.RightMargin)
	{
		fputs("\r\n", prn);
		LineCtr++;
		if (LineCtr == DfCfg.BottomMargin)
		{
			fputc('\f', prn);
			for (i = 0; i < DfCfg.TopMargin; i++)
				fputc('\n', prn);
			LineCtr = DfCfg.TopMargin;
		}
		CharCtr = 0;
		if (c == '\n')
			return;
	}
	if (CharCtr == 0)
	{
		for (i = 0; i < DfCfg.LeftMargin; i++)
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
	if (*DfCfg.PrinterPort)
	{
		FILE *prn;
		if ((prn = fopen(DfCfg.PrinterPort, "wt")) != NULL)
		{
			long percent;
			BOOL KeepPrinting = TRUE;
			unsigned char *text = DfGetText(wnd);
			unsigned oldpct = 100, cct = 0, len = strlen(text);
			DFWINDOW swnd = DfSliderBox(20, DfGetTitle(wnd), "Printing");
			/* ------- print the notepad text --------- */
			LineCtr = CharCtr = 0;
			while (KeepPrinting && *text)
			{
				PrintChar(prn, *text++);
				percent = ((long) ++cct * 100) / len;
				if ((int)percent != (int)oldpct)
				{
					oldpct = (int) percent;
					KeepPrinting = DfSendMessage(swnd, DFM_PAINT, 0, oldpct);
				}
			}
			if (KeepPrinting)
				/* ---- user did not cancel ---- */
				if (oldpct < 100)
					DfSendMessage(swnd, DFM_PAINT, 0, 100);
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
        if (DfSaveAsDialogBox(FileName))    {
            if (wnd->extension != NULL)
                free(wnd->extension);
            wnd->extension = DfMalloc(strlen(FileName)+1);
            strcpy(wnd->extension, FileName);
            DfAddTitle(wnd, NameComponent(FileName));
            DfSendMessage(wnd, DFM_BORDER, 0, 0);
        }
        else
            return;
    }
    if (wnd->extension != NULL)
    {
        DFWINDOW mwnd = DfMomentaryMessage("Saving the file");
        if ((fp = fopen(wnd->extension, "wt")) != NULL)
        {
            fwrite(DfGetText(wnd), strlen(DfGetText(wnd)), 1, fp);
            fclose(fp);
            wnd->TextChanged = FALSE;
        }
        DfSendMessage(mwnd, DFM_CLOSE_WINDOW, 0, 0);
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
                    DfSendMessage(wnd, DFM_CLOSE_WINDOW, 0, 0);
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
    DfSendMessage(DfGetParent(wnd), DFM_ADDSTATUS, (DF_PARAM) status, 0);
}

/* window processing module for the editboxes */
static int EditorProc(DFWINDOW wnd,DFMESSAGE msg,DF_PARAM p1,DF_PARAM p2)
{
	int rtn;

	switch (msg)
	{
		case DFM_SETFOCUS:
			if ((int)p1)
			{
				wnd->InsertMode = DfGetCommandToggle(&DfMainMenu, DF_ID_INSERT);
				wnd->WordWrapMode = DfGetCommandToggle(&DfMainMenu, DF_ID_WRAP);
			}
			rtn = DfDefaultWndProc(wnd, msg, p1, p2);
			if ((int)p1 == FALSE)
				DfSendMessage(DfGetParent(wnd), DFM_ADDSTATUS, 0, 0);
			else
				ShowPosition(wnd);
			return rtn;

		case DFM_KEYBOARD_CURSOR:
			rtn = DfDefaultWndProc(wnd, msg, p1, p2);
			ShowPosition(wnd);
			return rtn;

		case DFM_COMMAND:
			switch ((int) p1)
			{
				case DF_ID_SEARCH:
					DfSearchText(wnd);
					return TRUE;
				case DF_ID_REPLACE:
					DfReplaceText(wnd);
					return TRUE;
				case DF_ID_SEARCHNEXT:
					DfSearchNext(wnd);
					return TRUE;
				case DF_ID_CUT:
					DfCopyToClipboard(wnd);
					DfSendMessage(wnd, DFM_COMMAND, DF_ID_DELETETEXT, 0);
					DfSendMessage(wnd, DFM_PAINT, 0, 0);
					return TRUE;
				case DF_ID_COPY:
					DfCopyToClipboard(wnd);
					DfClearTextBlock(wnd);
					DfSendMessage(wnd, DFM_PAINT, 0, 0);
					return TRUE;
				case DF_ID_PASTE:
					DfPasteFromClipboard(wnd);
					DfSendMessage(wnd, DFM_PAINT, 0, 0);
					return TRUE;
				case DF_ID_DELETETEXT:
				case DF_ID_CLEAR:
					rtn = DfDefaultWndProc(wnd, msg, p1, p2);
					DfSendMessage(wnd, DFM_PAINT, 0, 0);
					return rtn;
				case DF_ID_HELP:
					DfDisplayHelp(wnd, "MEMOPADDOC");
					return TRUE;
				case DF_ID_WRAP:
					DfSendMessage(DfGetParent(wnd), DFM_COMMAND, DF_ID_WRAP, 0);
					wnd->WordWrapMode = DfCfg.WordWrap;
					return TRUE;
				case DF_ID_INSERT:
					DfSendMessage(DfGetParent(wnd), DFM_COMMAND, DF_ID_INSERT, 0);
					wnd->InsertMode = DfCfg.InsertMode;
					DfSendMessage(NULL, DFM_SHOW_CURSOR, wnd->InsertMode, 0);
					return TRUE;
				default:
					break;
			}
			break;

		case DFM_CLOSE_WINDOW:
			if (wnd->TextChanged)
			{
				char *cp = DfMalloc(25+strlen(DfGetTitle(wnd)));
				DfSendMessage(wnd, DFM_SETFOCUS, TRUE, 0);
				strcpy(cp, DfGetTitle(wnd));
				strcat(cp, "\nText changed. Save it?");
				if (DfYesNoBox(cp))
					DfSendMessage(DfGetParent(wnd), DFM_COMMAND, DF_ID_SAVE, 0);
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

	return DfDefaultWndProc(wnd, msg, p1, p2);
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

static int PrintSetupProc(DFWINDOW wnd, DFMESSAGE msg, DF_PARAM p1, DF_PARAM p2)
{
	int rtn, i = 0, mar;
	char marg[10];
	DFWINDOW cwnd;

	switch (msg)
	{
		case DFM_CREATE_WINDOW:
			rtn = DfDefaultWndProc(wnd, msg, p1, p2);
			DfPutItemText(wnd, DF_ID_PRINTERPORT, DfCfg.PrinterPort);
			while (ports[i] != NULL)
				DfPutComboListText(wnd, DF_ID_PRINTERPORT, ports[i++]);
			for (mar = CHARSLINE; mar >= 0; --mar)
			{
				sprintf(marg, "%3d", mar);
				DfPutItemText(wnd, DF_ID_LEFTMARGIN, marg);
				DfPutItemText(wnd, DF_ID_RIGHTMARGIN, marg);
			}
			for (mar = LINESPAGE; mar >= 0; --mar)
			{
				sprintf(marg, "%3d", mar);
				DfPutItemText(wnd, DF_ID_TOPMARGIN, marg);
				DfPutItemText(wnd, DF_ID_BOTTOMMARGIN, marg);
			}
			cwnd = DfControlWindow(&PrintSetup, DF_ID_LEFTMARGIN);
			DfSendMessage(cwnd, DFM_LB_SETSELECTION,
				CHARSLINE-DfCfg.LeftMargin, 0);
			cwnd = DfControlWindow(&PrintSetup, DF_ID_RIGHTMARGIN);
			DfSendMessage(cwnd, DFM_LB_SETSELECTION,
				CHARSLINE-DfCfg.RightMargin, 0);
			cwnd = DfControlWindow(&PrintSetup, DF_ID_TOPMARGIN);
			DfSendMessage(cwnd, DFM_LB_SETSELECTION,
				LINESPAGE-DfCfg.TopMargin, 0);
			cwnd = DfControlWindow(&PrintSetup, DF_ID_BOTTOMMARGIN);
			DfSendMessage(cwnd, DFM_LB_SETSELECTION,
				LINESPAGE-DfCfg.BottomMargin, 0);
			return rtn;
		case DFM_COMMAND:
			if ((int) p1 == DF_ID_OK && (int) p2 == 0)
			{
				DfGetItemText(wnd, DF_ID_PRINTERPORT, DfCfg.PrinterPort, 4);
				cwnd = DfControlWindow(&PrintSetup, DF_ID_LEFTMARGIN);
				DfCfg.LeftMargin = CHARSLINE -
					DfSendMessage(cwnd, DFM_LB_CURRENTSELECTION, 0, 0);
				cwnd = DfControlWindow(&PrintSetup, DF_ID_RIGHTMARGIN);
				DfCfg.RightMargin = CHARSLINE -
					DfSendMessage(cwnd, DFM_LB_CURRENTSELECTION, 0, 0);
				cwnd = DfControlWindow(&PrintSetup, DF_ID_TOPMARGIN);
				DfCfg.TopMargin = LINESPAGE -
					DfSendMessage(cwnd, DFM_LB_CURRENTSELECTION, 0, 0);
				cwnd = DfControlWindow(&PrintSetup, DF_ID_BOTTOMMARGIN);
				DfCfg.BottomMargin = LINESPAGE -
					DfSendMessage(cwnd, DFM_LB_CURRENTSELECTION, 0, 0);
			}
			break;
		default:
			break;
	}
	return DfDefaultWndProc(wnd, msg, p1, p2);
}

static void FixTabMenu(void)
{
	char *cp = DfGetCommandText(&DfMainMenu, DF_ID_TABS);
	char *p;

	if (cp != NULL)
	{
		p = strchr(cp, '(');
		if (p != NULL)
		{
//			*(p+1) = (char)(DfCfg.Tabs + '0');
//			if (DfGetClass(DfInFocus) == DF_POPDOWNMENU)
//				DfSendMessage(DfInFocus, DFM_PAINT, 0, 0);
		}
	}
}

void DfPrepFileMenu(void *w, struct DfMenu *mnu)
{
	DFWINDOW wnd = w;
	DfDeactivateCommand(&DfMainMenu, DF_ID_SAVE);
	DfDeactivateCommand(&DfMainMenu, DF_ID_SAVEAS);
	DfDeactivateCommand(&DfMainMenu, DF_ID_DELETEFILE);
	DfDeactivateCommand(&DfMainMenu, DF_ID_PRINT);
	if (wnd != NULL && DfGetClass(wnd) == DF_EDITBOX)
	{
		if (DfIsMultiLine(wnd))
		{
			DfActivateCommand(&DfMainMenu, DF_ID_SAVE);
			DfActivateCommand(&DfMainMenu, DF_ID_SAVEAS);
			DfActivateCommand(&DfMainMenu, DF_ID_DELETEFILE);
			DfActivateCommand(&DfMainMenu, DF_ID_PRINT);
		}
	}
}

void DfPrepSearchMenu(void *w, struct DfMenu *mnu)
{
	DFWINDOW wnd = w;
	DfDeactivateCommand(&DfMainMenu, DF_ID_SEARCH);
	DfDeactivateCommand(&DfMainMenu, DF_ID_REPLACE);
	DfDeactivateCommand(&DfMainMenu, DF_ID_SEARCHNEXT);
	if (wnd != NULL && DfGetClass(wnd) == DF_EDITBOX)
	{
		if (DfIsMultiLine(wnd))
		{
			DfActivateCommand(&DfMainMenu, DF_ID_SEARCH);
			DfActivateCommand(&DfMainMenu, DF_ID_REPLACE);
			DfActivateCommand(&DfMainMenu, DF_ID_SEARCHNEXT);
		}
	}
}

void DfPrepEditMenu(void *w, struct DfMenu *mnu)
{
	DFWINDOW wnd = w;
	DfDeactivateCommand(&DfMainMenu, DF_ID_CUT);
	DfDeactivateCommand(&DfMainMenu, DF_ID_COPY);
	DfDeactivateCommand(&DfMainMenu, DF_ID_CLEAR);
	DfDeactivateCommand(&DfMainMenu, DF_ID_DELETETEXT);
	DfDeactivateCommand(&DfMainMenu, DF_ID_PARAGRAPH);
	DfDeactivateCommand(&DfMainMenu, DF_ID_PASTE);
	DfDeactivateCommand(&DfMainMenu, DF_ID_UNDO);
	if (wnd != NULL && DfGetClass(wnd) == DF_EDITBOX)
	{
		if (DfIsMultiLine(wnd))
		{
			if (DfTextBlockMarked(wnd))
			{
				DfActivateCommand(&DfMainMenu, DF_ID_CUT);
				DfActivateCommand(&DfMainMenu, DF_ID_COPY);
				DfActivateCommand(&DfMainMenu, DF_ID_CLEAR);
				DfActivateCommand(&DfMainMenu, DF_ID_DELETETEXT);
			}
			DfActivateCommand(&DfMainMenu, DF_ID_PARAGRAPH);
			if (!DfTestAttribute(wnd, DF_READONLY) && DfClipboard != NULL)
				DfActivateCommand(&DfMainMenu, DF_ID_PASTE);
			if (wnd->DeletedText != NULL)
				DfActivateCommand(&DfMainMenu, DF_ID_UNDO);
		}
	}
}

/* EOF */
