/* ------------- config.c ------------- */

#include "dflat.h"

/* ----- default colors for DfColor video system ----- */
unsigned char DfColor[DF_CLASSCOUNT] [4] [2] = {
    /* ------------ DF_NORMAL ------------ */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},/* DF_HILITE_COLOR */

    /* ---------- DF_APPLICATION --------- */
   {{LIGHTGRAY, BLUE},  /* DF_STD_COLOR    */
    {LIGHTGRAY, BLUE},  /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLUE}}, /* DF_HILITE_COLOR */

    /* ------------ DF_TEXTBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ------------ DF_LISTBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ----------- DF_EDITBOX ------------ */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLUE},  /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ---------- DF_MENUBAR ------------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {BLACK, CYAN},      /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {DARKGRAY, RED}},   /* DF_HILITE_COLOR
                          Inactive, Shortcut (both DF_FG) */

    /* ---------- DF_POPDOWNMENU --------- */
   {{BLACK, CYAN},      /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {BLACK, CYAN},      /* DF_FRAME_COLOR  */
    {DARKGRAY, RED}},   /* DF_HILITE_COLOR
                           Inactive ,Shortcut (both DF_FG) */

#ifdef INCLUDE_PICTUREBOX
    /* ------------ DF_PICTUREBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */
#endif

    /* ------------- DF_DIALOG ----------- */
   {{LIGHTGRAY, BLUE},  /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLUE}}, /* DF_HILITE_COLOR */

	/* ------------ DF_BOX --------------- */
   {{LIGHTGRAY, BLUE},  /* DF_STD_COLOR    */
    {LIGHTGRAY, BLUE},  /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLUE}}, /* DF_HILITE_COLOR */

    /* ------------ DF_BUTTON ------------ */
   {{BLACK, CYAN},      /* DF_STD_COLOR    */
    {WHITE, CYAN},      /* DF_SELECT_COLOR */
    {BLACK, CYAN},      /* DF_FRAME_COLOR  */
    {DARKGRAY, RED}},   /* DF_HILITE_COLOR
                           Inactive ,Shortcut (both DF_FG) */
    /* ------------ DF_COMBOBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK},  /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ------------- DF_TEXT ----------- */
   {{0xff, 0xff},  /* DF_STD_COLOR    */
    {0xff, 0xff},  /* DF_SELECT_COLOR */
    {0xff, 0xff},  /* DF_FRAME_COLOR  */
    {0xff, 0xff}}, /* DF_HILITE_COLOR */

    /* ------------- DF_RADIOBUTTON ----------- */
   {{LIGHTGRAY, BLUE},  /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLUE}}, /* DF_HILITE_COLOR */

    /* ------------- DF_CHECKBOX ----------- */
   {{LIGHTGRAY, BLUE},  /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLUE}}, /* DF_HILITE_COLOR */

    /* ------------ DF_SPINBUTTON ----------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ----------- DF_ERRORBOX ----------- */
   {{YELLOW, RED},      /* DF_STD_COLOR    */
    {YELLOW, RED},      /* DF_SELECT_COLOR */
    {YELLOW, RED},      /* DF_FRAME_COLOR  */
    {YELLOW, RED}},     /* DF_HILITE_COLOR */

    /* ----------- DF_MESSAGEBOX --------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ----------- DF_HELPBOX ------------ */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLUE},  /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {WHITE, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ---------- DF_STATUSBAR ------------- */
   {{BLACK, CYAN},      /* DF_STD_COLOR    */
    {BLACK, CYAN},      /* DF_SELECT_COLOR */
    {BLACK, CYAN},      /* DF_FRAME_COLOR  */
    {BLACK, CYAN}},     /* DF_HILITE_COLOR */

    /* ---------- DF_TITLEBAR ------------ */
   {{BLACK, CYAN},      /* DF_STD_COLOR    */
    {BLACK, CYAN},      /* DF_SELECT_COLOR */
    {BLACK, CYAN},      /* DF_FRAME_COLOR  */
    {WHITE, CYAN}},     /* DF_HILITE_COLOR */

    /* ------------ DF_DUMMY ------------- */
   {{GREEN, LIGHTGRAY}, /* DF_STD_COLOR    */
    {GREEN, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {GREEN, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {GREEN, LIGHTGRAY}} /* DF_HILITE_COLOR */
};

/* ----- default colors for mono video system ----- */
unsigned char DfBW[DF_CLASSCOUNT] [4] [2] = {
    /* ------------ DF_NORMAL ------------ */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},/* DF_HILITE_COLOR */

    /* ---------- DF_APPLICATION --------- */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},/* DF_HILITE_COLOR */

    /* ------------ DF_TEXTBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ------------ DF_LISTBOX ----------- */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ----------- DF_EDITBOX ------------ */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ---------- DF_MENUBAR ------------- */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {DARKGRAY, WHITE}}, /* DF_HILITE_COLOR
                           Inactive, Shortcut (both DF_FG) */

    /* ---------- DF_POPDOWNMENU --------- */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {DARKGRAY, WHITE}}, /* DF_HILITE_COLOR
                           Inactive ,Shortcut (both DF_FG) */

#ifdef INCLUDE_PICTUREBOX
    /* ------------ DF_PICTUREBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */
#endif

    /* ------------- DF_DIALOG ----------- */
   {{LIGHTGRAY, BLACK},  /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY},  /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK},  /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLACK}}, /* DF_HILITE_COLOR */

	/* ------------ DF_BOX --------------- */
   {{LIGHTGRAY, BLACK},  /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK},  /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK},  /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLACK}}, /* DF_HILITE_COLOR */

    /* ------------ DF_BUTTON ------------ */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {WHITE, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {DARKGRAY, WHITE}}, /* DF_HILITE_COLOR
                           Inactive ,Shortcut (both DF_FG) */
    /* ------------ DF_COMBOBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ------------- DF_TEXT ----------- */
   {{0xff, 0xff},  /* DF_STD_COLOR    */
    {0xff, 0xff},  /* DF_SELECT_COLOR */
    {0xff, 0xff},  /* DF_FRAME_COLOR  */
    {0xff, 0xff}}, /* DF_HILITE_COLOR */

    /* ------------- DF_RADIOBUTTON ----------- */
   {{LIGHTGRAY, BLACK},  /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY},  /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK},  /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLACK}}, /* DF_HILITE_COLOR */

    /* ------------- DF_CHECKBOX ----------- */
   {{LIGHTGRAY, BLACK},  /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY},  /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK},  /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLACK}}, /* DF_HILITE_COLOR */

    /* ------------ DF_SPINBUTTON ----------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ----------- DF_ERRORBOX ----------- */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},/* DF_HILITE_COLOR */

    /* ----------- DF_MESSAGEBOX --------- */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},/* DF_HILITE_COLOR */

    /* ----------- DF_HELPBOX ------------ */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {WHITE, BLACK},     /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {WHITE, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ---------- DF_STATUSBAR ------------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ---------- DF_TITLEBAR ------------ */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ------------ DF_DUMMY ------------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}} /* DF_HILITE_COLOR */
};
/* ----- default colors for DfReverse mono video ----- */
unsigned char DfReverse[DF_CLASSCOUNT] [4] [2] = {
    /* ------------ DF_NORMAL ------------ */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ---------- DF_APPLICATION --------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ------------ DF_TEXTBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ------------ DF_LISTBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ----------- DF_EDITBOX ------------ */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ---------- DF_MENUBAR ------------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {DARKGRAY, WHITE}}, /* DF_HILITE_COLOR
                           Inactive, Shortcut (both DF_FG) */

    /* ---------- DF_POPDOWNMENU --------- */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {DARKGRAY, WHITE}}, /* DF_HILITE_COLOR
                           Inactive ,Shortcut (both DF_FG) */

#ifdef INCLUDE_PICTUREBOX
    /* ------------ DF_PICTUREBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */
#endif

    /* ------------- DF_DIALOG ----------- */
   {{BLACK, LIGHTGRAY},  /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK},  /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY},  /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}}, /* DF_HILITE_COLOR */

	/* ------------ DF_BOX --------------- */
   {{BLACK, LIGHTGRAY},  /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY},  /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY},  /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}}, /* DF_HILITE_COLOR */

    /* ------------ DF_BUTTON ------------ */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {WHITE, BLACK},     /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {DARKGRAY, WHITE}}, /* DF_HILITE_COLOR
                           Inactive ,Shortcut (both DF_FG) */
    /* ------------ DF_COMBOBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ------------- DF_TEXT ----------- */
   {{0xff, 0xff},  /* DF_STD_COLOR    */
    {0xff, 0xff},  /* DF_SELECT_COLOR */
    {0xff, 0xff},  /* DF_FRAME_COLOR  */
    {0xff, 0xff}}, /* DF_HILITE_COLOR */

    /* ------------- DF_RADIOBUTTON ----------- */
   {{BLACK, LIGHTGRAY},  /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK},  /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY},  /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}}, /* DF_HILITE_COLOR */

    /* ------------- DF_CHECKBOX ----------- */
   {{BLACK, LIGHTGRAY},  /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK},  /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY},  /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}}, /* DF_HILITE_COLOR */

    /* ------------ DF_SPINBUTTON ----------- */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ----------- DF_ERRORBOX ----------- */
   {{BLACK, LIGHTGRAY},      /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY},      /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY},      /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},     /* DF_HILITE_COLOR */

    /* ----------- DF_MESSAGEBOX --------- */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ----------- DF_HELPBOX ------------ */
   {{BLACK, LIGHTGRAY}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* DF_FRAME_COLOR  */
    {WHITE, LIGHTGRAY}},/* DF_HILITE_COLOR */

    /* ---------- DF_STATUSBAR ------------- */
   {{LIGHTGRAY, BLACK},      /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK},      /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK},      /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},     /* DF_HILITE_COLOR */

    /* ---------- DF_TITLEBAR ------------ */
   {{LIGHTGRAY, BLACK},      /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK},      /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK},      /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},     /* DF_HILITE_COLOR */

    /* ------------ DF_DUMMY ------------- */
   {{LIGHTGRAY, BLACK}, /* DF_STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* DF_SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* DF_FRAME_COLOR  */
    {LIGHTGRAY, BLACK}} /* DF_HILITE_COLOR */
};

/* ------ default configuration values ------- */
DFCONFIG DfCfg = {
    DF_VERSION,
    TRUE,            /* Editor Insert Mode          */
    4,               /* Editor tab stops            */
    TRUE,            /* Editor word wrap            */
#ifdef INCLUDE_WINDOWOPTIONS
    TRUE,            /* Application Border          */
    TRUE,            /* Application Title           */
    TRUE,            /* Status Bar                  */
    TRUE,            /* Textured application window */
#endif
//    25,              /* Number of screen lines      */
	"Lpt1",			 /* Printer Port                */
	66,              /* Lines per printer page      */
	80,				 /* characters per printer line */
	6,				 /* Left printer margin			*/
	70,				 /* Right printer margin		*/
	3,				 /* Top printer margin			*/
	55				 /* Bottom printer margin		*/
};

void DfBuildFileName(char *path, char *ext)
{
	extern char **Argv;
    char *cp;

	strcpy(path, Argv[0]);
	cp = strrchr(path, '\\');
	if (cp == NULL)
		cp = path;
	else 
		cp++;
	strcpy(cp, DFlatApplication);
	strcat(cp, ext);
}

FILE *DfOpenConfig(char *mode)
{
	char path[64];
	DfBuildFileName(path, ".DfCfg");
	return fopen(path, mode);
}

/* ------ load a configuration file from disk ------- */
BOOL DfLoadConfig(void)
{
	static BOOL ConfigLoaded = FALSE;
	if (ConfigLoaded == FALSE)	{
	    FILE *fp = DfOpenConfig("rb");
    	if (fp != NULL)    {
        	fread(DfCfg.version, sizeof DfCfg.version+1, 1, fp);
        	if (strcmp(DfCfg.version, DF_VERSION) == 0)    {
            	fseek(fp, 0L, SEEK_SET);
            	fread(&DfCfg, sizeof(DFCONFIG), 1, fp);
 		       	fclose(fp);
        	}
        	else	{
				char path[64];
				DfBuildFileName(path, ".DfCfg");
	        	fclose(fp);
				unlink(path);
            	strcpy(DfCfg.version, DF_VERSION);
			}
			ConfigLoaded = TRUE;
    	}
	}
    return ConfigLoaded;
}

/* ------ save a configuration file to disk ------- */
void DfSaveConfig(void)
{
    FILE *fp = DfOpenConfig("wb");
    if (fp != NULL)    {
        fwrite(&DfCfg, sizeof(DFCONFIG), 1, fp);
        fclose(fp);
    }
}

/* --------- set window colors --------- */
void DfSetStandardColor(DFWINDOW wnd)
{
    DfForeground = DfWndForeground(wnd);
    DfBackground = DfWndBackground(wnd);
}

void DfSetReverseColor(DFWINDOW wnd)
{
    DfForeground = DfSelectForeground(wnd);
    DfBackground = DfSelectBackground(wnd);
}

/* EOF */
