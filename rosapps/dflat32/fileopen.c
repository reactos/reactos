/* ----------- fileopen.c ------------- */

#include "dflat.h"

static BOOL DlgFileOpen(char *, char *, DBOX *);
static int DlgFnOpen(DFWINDOW, DFMESSAGE, PARAM, PARAM);
static void InitDlgBox(DFWINDOW);
static void StripPath(char *);
static BOOL IncompleteFilename(char *);

static char *OrigSpec;
static char *FileSpec;
static char *FileName;
static char *NewFileName;

static BOOL Saving;
extern DBOX FileOpen;
extern DBOX SaveAs;

/*
 * Dialog Box to select a file to open
 */
BOOL OpenFileDialogBox(char *Fpath, char *Fname)
{
    return DlgFileOpen(Fpath, Fname, &FileOpen);
}

/*
 * Dialog Box to select a file to save as
 */
BOOL SaveAsDialogBox(char *Fname)
{
    return DlgFileOpen(NULL, Fname, &SaveAs);
}

/* --------- generic file open ---------- */
static BOOL DlgFileOpen(char *Fpath, char *Fname, DBOX *db)
{
    BOOL rtn;
    char savedir[MAX_PATH];
    char OSpec[80];
    char FSpec[80];
    char FName[80];
    char NewFName[80];

    OrigSpec = OSpec;
    FileSpec = FSpec;
    FileName = FName;
    NewFileName = NewFName;

    GetCurrentDirectory (MAX_PATH, savedir);

    if (Fpath != NULL)    {
        strncpy(FileSpec, Fpath, 80);
        Saving = FALSE;
    }
    else    {
        *FileSpec = '\0';
        Saving = TRUE;
    }
    strcpy(FileName, FileSpec);
    strcpy(OrigSpec, FileSpec);

    if ((rtn = DfDialogBox(NULL, db, TRUE, DlgFnOpen)) != FALSE)
        strcpy(Fname, NewFileName);
    else
        *Fname = '\0';

    SetCurrentDirectory (savedir);

    return rtn;
}

/*
 *  Process dialog box messages
 */
static int DlgFnOpen(DFWINDOW wnd,DFMESSAGE msg,PARAM p1,PARAM p2)
{
	int rtn;
	DBOX *db;
	DFWINDOW cwnd;

    switch (msg)
    {
        case CREATE_WINDOW:
            rtn = DefaultWndProc(wnd, msg, p1, p2);
            db = wnd->extension;
            cwnd = ControlWindow(db, ID_FILENAME);
            DfSendMessage(cwnd, SETTEXTLENGTH, 64, 0);
            return rtn;

        case INITIATE_DIALOG:
            InitDlgBox(wnd);
            break;

        case DFM_COMMAND:
            switch ((int) p1)
			{
                case ID_FILENAME:
                    if (p2 != ENTERFOCUS)
					{
                        /* allow user to modify the file spec */
                        GetItemText(wnd, ID_FILENAME,
                                FileName, 65);
                        if (IncompleteFilename(FileName) || Saving)
						{
                            strcpy(OrigSpec, FileName);
                            StripPath(OrigSpec);
                        }
                        if (p2 != LEAVEFOCUS)
                            DfSendMessage(wnd, DFM_COMMAND, ID_OK, 0);
                    }
                    return TRUE;

                case ID_OK:
                    if (p2 != 0)
                        break;
                    GetItemText(wnd, ID_FILENAME,
                            FileName, 65);
                    strcpy(FileSpec, FileName);
                    if (IncompleteFilename(FileName))
					{
                        /* no file name yet */
                        InitDlgBox(wnd);
                        strcpy(OrigSpec, FileSpec);
                        return TRUE;
                    }
                    else    {
                        GetItemText(wnd, ID_PATH, FileName, 65);
                        strcat(FileName, FileSpec);
                        strcpy(NewFileName, FileName);
                    }
                    break;

                case ID_FILES:
                    switch ((int) p2)
					{
                        case ENTERFOCUS:
                        case LB_SELECTION:
                            /* selected a different filename */
                            GetDlgListText(wnd, FileName,
                                        ID_FILES);
                            PutItemText(wnd, ID_FILENAME,
                                            FileName);
                            break;
                        case LB_CHOOSE:
                            /* chose a file name */
                            GetDlgListText(wnd, FileName,
                                    ID_FILES);
                            DfSendMessage(wnd, DFM_COMMAND, ID_OK, 0);
                            break;
                        default:
                            break;
                    }
                    return TRUE;

                case ID_DRIVE:
                    switch ((int) p2)    {
                        case ENTERFOCUS:
                            if (Saving)
                                *FileSpec = '\0';
                            break;
                        case LEAVEFOCUS:
                            if (Saving)
                                strcpy(FileSpec, FileName);
                            break;

						case LB_SELECTION:
							{
								char dd[25];
								/* selected different drive/dir */
								GetDlgListText(wnd, dd, ID_DRIVE);
								if (*(dd+2) == ':')
									*(dd+3) = '\0';
								else
									*(dd+strlen(dd)-1) = '\0';
								strcpy(FileName, dd+1);
								if (*(dd+2) != ':' && *OrigSpec != '\\')
									strcat(FileName, "\\");
								strcat(FileName, OrigSpec);
								if (*(FileName+1) != ':' && *FileName != '.')
								{
									GetItemText(wnd, ID_PATH, FileSpec, 65);
									strcat(FileSpec, FileName);
								}
								else 
									strcpy(FileSpec, FileName);
							}
							break;

                        case LB_CHOOSE:
                            /* chose drive/dir */
                            if (Saving)
                                PutItemText(wnd, ID_FILENAME, "");
                            InitDlgBox(wnd);
                            return TRUE;
                        default:
                            break;
                    }
                    PutItemText(wnd, ID_FILENAME, FileSpec);
                    return TRUE;


                default:
                    break;
            }
        default:
            break;
    }
    return DefaultWndProc(wnd, msg, p1, p2);
}

/*
 *  Initialize the dialog box
 */
static void InitDlgBox(DFWINDOW wnd)
{
    if (*FileSpec && !Saving)
        PutItemText(wnd, ID_FILENAME, FileSpec);
    if (DfDlgDirList(wnd, FileSpec, ID_FILES, ID_PATH, 0))
    {
        StripPath(FileSpec);
        DfDlgDirList(wnd, "*.*", ID_DRIVE, 0, 0xc010);
    }
}

/*
 * Strip the drive and path information from a file spec
 */
static void StripPath(char *filespec)
{
    char *cp, *cp1;

    cp = strchr(filespec, ':');
    if (cp != NULL)
        cp++;
    else
        cp = filespec;
    while (TRUE)    {
        cp1 = strchr(cp, '\\');
        if (cp1 == NULL)
            break;
        cp = cp1+1;
    }
    strcpy(filespec, cp);
}


static BOOL IncompleteFilename(char *s)
{
    int lc = strlen(s)-1;
    if (strchr(s, '?') || strchr(s, '*') || !*s)
        return TRUE;
    if (*(s+lc) == ':' || *(s+lc) == '\\')
        return TRUE;
    return FALSE;
}

/* EOF */