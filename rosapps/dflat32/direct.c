/* ---------- direct.c --------- */

#include <direct.h>
#include <io.h>

#include "dflat.h"

#define DRIVE		1
#define DIRECTORY	2
#define FILENAME	4
#define EXTENSION	8

static char path[MAX_PATH];
static char drive[_MAX_DRIVE] = " :";
static char dir[_MAX_DIR];
static char name[_MAX_FNAME];
static char ext[_MAX_EXT];

/* ----- Create unambiguous path from file spec, filling in the
     drive and directory if incomplete. Optionally change to
     the new drive and subdirectory ------ */
void CreatePath(char *path,char *fspec,int InclName,int Change)
{
    int cm = 0;
    char currdir[MAX_PATH];
    char *cp;

	/* save the current directory */
	if (!Change)
		GetCurrentDirectory (MAX_PATH, currdir);

    *drive = *dir = *name = *ext = '\0';
    _splitpath(fspec, drive, dir, name, ext);
    if (!InclName)
        *name = *ext = '\0';
    *drive = toupper(*drive);
    if (*ext)
        cm |= EXTENSION;
    if (InclName && *name)
        cm |= FILENAME;
    if (*dir)
        cm |= DIRECTORY;
    if (*drive)
        cm |= DRIVE;
    if (cm & DRIVE)
        _chdrive(*drive - '@');
    else
	{
        *drive = _getdrive();
        *drive += '@';
    }
    if (cm & DIRECTORY)
    {
        cp = dir+strlen(dir)-1;
        if (*cp == '\\')
            *cp = '\0';
        chdir(dir);
    }
    getcwd(dir, sizeof dir);
    memmove(dir, dir+2, strlen(dir+1));
    if (InclName)    {
        if (!(cm & FILENAME))
            strcpy(name, "*");
        if (!(cm & EXTENSION) && strchr(fspec, '.') != NULL)
            strcpy(ext, ".*");
    }
    else
        *name = *ext = '\0';
    if (dir[strlen(dir)-1] != '\\')
        strcat(dir, "\\");
    memset(path, 0, sizeof path);
    _makepath(path, drive, dir, name, ext);

	if (!Change)
		SetCurrentDirectory (currdir);
}


static int dircmp(const void *c1, const void *c2)
{
    return stricmp(*(char **)c1, *(char **)c2);
}


BOOL DfDlgDirList(DFWINDOW wnd, char *fspec,
                enum commands nameid, enum commands pathid,
                unsigned attrib)
{
    int ax, i = 0;
    struct _finddata_t ff;
    CTLWINDOW *ct = FindCommand(wnd->extension,nameid,LISTBOX);
    DFWINDOW lwnd;
    char **dirlist = NULL;

	CreatePath(path, fspec, TRUE, TRUE);
	if (ct != NULL)
	{
		lwnd = ct->wnd;
		DfSendMessage(ct->wnd, CLEARTEXT, 0, 0);

		if (attrib & 0x8000)
		{
			DWORD cd, dr;

			cd = GetLogicalDrives ();
			for (dr = 0; dr < 26; dr++)
			{
				if (cd & (1 << dr))
				{
					char drname[15];

					sprintf(drname, "[%c:\\]", (char)(dr+'A'));
#if 0
                    /* ---- test for network or RAM disk ---- */
                    regs.x.ax = 0x4409;     /* IOCTL func 9 */
                    regs.h.bl = dr+1;
                    int86(DOS, &regs, &regs);
                    if (!regs.x.cflag)    {
                        if (regs.x.dx & 0x1000)
                            strcat(drname, " (Network)");
                        else if (regs.x.dx == 0x0800)
                            strcat(drname, " (RAMdisk)");
                    }
#endif
					DfSendMessage(lwnd,ADDTEXT,(PARAM)drname,0);
				}
			}
			DfSendMessage(lwnd, PAINT, 0, 0);
		}
		ax = _findfirst(path, &ff);
		if (ax == -1)
			return FALSE;
		do
		{
            if (!((attrib & 0x4000) &&
                 (ff.attrib & (attrib & 0x3f)) == 0) &&
                 strcmp(ff.name, "."))
			{
                char fname[MAX_PATH+2];
                sprintf(fname, (ff.attrib & FILE_ATTRIBUTE_DIRECTORY) ?
                                "[%s]" : "%s" , ff.name);
                dirlist = DFrealloc(dirlist,
                                    sizeof(char *)*(i+1));
                dirlist[i] = DFmalloc(strlen(fname)+1);
                if (dirlist[i] != NULL)
                    strcpy(dirlist[i], fname);
                i++;
            }
        }
		while (_findnext(ax, &ff) == 0);
		_findclose(ax);
        if (dirlist != NULL)
		{
            int j;
            /* -- sort file/drive/directory list box data -- */
            qsort(dirlist, i, sizeof(void *), dircmp);

            /* ---- send sorted list to list box ---- */
            for (j = 0; j < i; j++)    {
                DfSendMessage(lwnd,ADDTEXT,(PARAM)dirlist[j],0);
                free(dirlist[j]);
            }
            free(dirlist);
        }
        DfSendMessage(lwnd, SHOW_WINDOW, 0, 0);
    }
    if (pathid)
	{
        _makepath(path, drive, dir, NULL, NULL);
        PutItemText(wnd, pathid, path);
    }
    return TRUE;
}

/* EOF */
