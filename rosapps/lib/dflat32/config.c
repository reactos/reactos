/* ------------- config.c ------------- */

#include "dflat32/dflat.h"

/* ----- default colors for color video system ----- */
unsigned char color[CLASSCOUNT] [4] [2] = {
    /* ------------ NORMAL ------------ */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},/* HILITE_COLOR */

    /* ---------- APPLICATION --------- */
   {{LIGHTGRAY, BLUE},  /* STD_COLOR    */
    {LIGHTGRAY, BLUE},  /* SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* FRAME_COLOR  */
    {LIGHTGRAY, BLUE}}, /* HILITE_COLOR */

    /* ------------ TEXTBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ------------ LISTBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ----------- EDITBOX ------------ */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLUE},  /* SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ---------- MENUBAR ------------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {BLACK, CYAN},      /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {DARKGRAY, RED}},   /* HILITE_COLOR
                          Inactive, Shortcut (both FG) */

    /* ---------- POPDOWNMENU --------- */
   {{BLACK, CYAN},      /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {BLACK, CYAN},      /* FRAME_COLOR  */
    {DARKGRAY, RED}},   /* HILITE_COLOR
                           Inactive ,Shortcut (both FG) */

#ifdef INCLUDE_PICTUREBOX
    /* ------------ PICTUREBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */
#endif

    /* ------------- DIALOG ----------- */
   {{LIGHTGRAY, BLUE},  /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* FRAME_COLOR  */
    {LIGHTGRAY, BLUE}}, /* HILITE_COLOR */

	/* ------------ BOX --------------- */
   {{LIGHTGRAY, BLUE},  /* STD_COLOR    */
    {LIGHTGRAY, BLUE},  /* SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* FRAME_COLOR  */
    {LIGHTGRAY, BLUE}}, /* HILITE_COLOR */

    /* ------------ BUTTON ------------ */
   {{BLACK, CYAN},      /* STD_COLOR    */
    {WHITE, CYAN},      /* SELECT_COLOR */
    {BLACK, CYAN},      /* FRAME_COLOR  */
    {DARKGRAY, RED}},   /* HILITE_COLOR
                           Inactive ,Shortcut (both FG) */
    /* ------------ COMBOBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK},  /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ------------- TEXT ----------- */
   {{0xff, 0xff},  /* STD_COLOR    */
    {0xff, 0xff},  /* SELECT_COLOR */
    {0xff, 0xff},  /* FRAME_COLOR  */
    {0xff, 0xff}}, /* HILITE_COLOR */

    /* ------------- RADIOBUTTON ----------- */
   {{LIGHTGRAY, BLUE},  /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* FRAME_COLOR  */
    {LIGHTGRAY, BLUE}}, /* HILITE_COLOR */

    /* ------------- CHECKBOX ----------- */
   {{LIGHTGRAY, BLUE},  /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {LIGHTGRAY, BLUE},  /* FRAME_COLOR  */
    {LIGHTGRAY, BLUE}}, /* HILITE_COLOR */

    /* ------------ SPINBUTTON ----------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ----------- ERRORBOX ----------- */
   {{YELLOW, RED},      /* STD_COLOR    */
    {YELLOW, RED},      /* SELECT_COLOR */
    {YELLOW, RED},      /* FRAME_COLOR  */
    {YELLOW, RED}},     /* HILITE_COLOR */

    /* ----------- MESSAGEBOX --------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ----------- HELPBOX ------------ */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLUE},  /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {WHITE, LIGHTGRAY}},/* HILITE_COLOR */

    /* ---------- STATUSBAR ------------- */
   {{BLACK, CYAN},      /* STD_COLOR    */
    {BLACK, CYAN},      /* SELECT_COLOR */
    {BLACK, CYAN},      /* FRAME_COLOR  */
    {BLACK, CYAN}},     /* HILITE_COLOR */

    /* ---------- TITLEBAR ------------ */
   {{BLACK, CYAN},      /* STD_COLOR    */
    {BLACK, CYAN},      /* SELECT_COLOR */
    {BLACK, CYAN},      /* FRAME_COLOR  */
    {WHITE, CYAN}},     /* HILITE_COLOR */

    /* ------------ DUMMY ------------- */
   {{GREEN, LIGHTGRAY}, /* STD_COLOR    */
    {GREEN, LIGHTGRAY}, /* SELECT_COLOR */
    {GREEN, LIGHTGRAY}, /* FRAME_COLOR  */
    {GREEN, LIGHTGRAY}} /* HILITE_COLOR */
};

/* ----- default colors for mono video system ----- */
unsigned char bw[CLASSCOUNT] [4] [2] = {
    /* ------------ NORMAL ------------ */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},/* HILITE_COLOR */

    /* ---------- APPLICATION --------- */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},/* HILITE_COLOR */

    /* ------------ TEXTBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ------------ LISTBOX ----------- */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ----------- EDITBOX ------------ */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ---------- MENUBAR ------------- */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {DARKGRAY, WHITE}}, /* HILITE_COLOR
                           Inactive, Shortcut (both FG) */

    /* ---------- POPDOWNMENU --------- */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {DARKGRAY, WHITE}}, /* HILITE_COLOR
                           Inactive ,Shortcut (both FG) */

#ifdef INCLUDE_PICTUREBOX
    /* ------------ PICTUREBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */
#endif

    /* ------------- DIALOG ----------- */
   {{LIGHTGRAY, BLACK},  /* STD_COLOR    */
    {BLACK, LIGHTGRAY},  /* SELECT_COLOR */
    {LIGHTGRAY, BLACK},  /* FRAME_COLOR  */
    {LIGHTGRAY, BLACK}}, /* HILITE_COLOR */

	/* ------------ BOX --------------- */
   {{LIGHTGRAY, BLACK},  /* STD_COLOR    */
    {LIGHTGRAY, BLACK},  /* SELECT_COLOR */
    {LIGHTGRAY, BLACK},  /* FRAME_COLOR  */
    {LIGHTGRAY, BLACK}}, /* HILITE_COLOR */

    /* ------------ BUTTON ------------ */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {WHITE, LIGHTGRAY}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {DARKGRAY, WHITE}}, /* HILITE_COLOR
                           Inactive ,Shortcut (both FG) */
    /* ------------ COMBOBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ------------- TEXT ----------- */
   {{0xff, 0xff},  /* STD_COLOR    */
    {0xff, 0xff},  /* SELECT_COLOR */
    {0xff, 0xff},  /* FRAME_COLOR  */
    {0xff, 0xff}}, /* HILITE_COLOR */

    /* ------------- RADIOBUTTON ----------- */
   {{LIGHTGRAY, BLACK},  /* STD_COLOR    */
    {BLACK, LIGHTGRAY},  /* SELECT_COLOR */
    {LIGHTGRAY, BLACK},  /* FRAME_COLOR  */
    {LIGHTGRAY, BLACK}}, /* HILITE_COLOR */

    /* ------------- CHECKBOX ----------- */
   {{LIGHTGRAY, BLACK},  /* STD_COLOR    */
    {BLACK, LIGHTGRAY},  /* SELECT_COLOR */
    {LIGHTGRAY, BLACK},  /* FRAME_COLOR  */
    {LIGHTGRAY, BLACK}}, /* HILITE_COLOR */

    /* ------------ SPINBUTTON ----------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ----------- ERRORBOX ----------- */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},/* HILITE_COLOR */

    /* ----------- MESSAGEBOX --------- */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},/* HILITE_COLOR */

    /* ----------- HELPBOX ------------ */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {WHITE, BLACK},     /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {WHITE, LIGHTGRAY}},/* HILITE_COLOR */

    /* ---------- STATUSBAR ------------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ---------- TITLEBAR ------------ */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ------------ DUMMY ------------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}} /* HILITE_COLOR */
};
/* ----- default colors for reverse mono video ----- */
unsigned char reverse[CLASSCOUNT] [4] [2] = {
    /* ------------ NORMAL ------------ */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ---------- APPLICATION --------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ------------ TEXTBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ------------ LISTBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ----------- EDITBOX ------------ */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ---------- MENUBAR ------------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {DARKGRAY, WHITE}}, /* HILITE_COLOR
                           Inactive, Shortcut (both FG) */

    /* ---------- POPDOWNMENU --------- */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {DARKGRAY, WHITE}}, /* HILITE_COLOR
                           Inactive ,Shortcut (both FG) */

#ifdef INCLUDE_PICTUREBOX
    /* ------------ PICTUREBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */
#endif

    /* ------------- DIALOG ----------- */
   {{BLACK, LIGHTGRAY},  /* STD_COLOR    */
    {LIGHTGRAY, BLACK},  /* SELECT_COLOR */
    {BLACK, LIGHTGRAY},  /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}}, /* HILITE_COLOR */

	/* ------------ BOX --------------- */
   {{BLACK, LIGHTGRAY},  /* STD_COLOR    */
    {BLACK, LIGHTGRAY},  /* SELECT_COLOR */
    {BLACK, LIGHTGRAY},  /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}}, /* HILITE_COLOR */

    /* ------------ BUTTON ------------ */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {WHITE, BLACK},     /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {DARKGRAY, WHITE}}, /* HILITE_COLOR
                           Inactive ,Shortcut (both FG) */
    /* ------------ COMBOBOX ----------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ------------- TEXT ----------- */
   {{0xff, 0xff},  /* STD_COLOR    */
    {0xff, 0xff},  /* SELECT_COLOR */
    {0xff, 0xff},  /* FRAME_COLOR  */
    {0xff, 0xff}}, /* HILITE_COLOR */

    /* ------------- RADIOBUTTON ----------- */
   {{BLACK, LIGHTGRAY},  /* STD_COLOR    */
    {LIGHTGRAY, BLACK},  /* SELECT_COLOR */
    {BLACK, LIGHTGRAY},  /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}}, /* HILITE_COLOR */

    /* ------------- CHECKBOX ----------- */
   {{BLACK, LIGHTGRAY},  /* STD_COLOR    */
    {LIGHTGRAY, BLACK},  /* SELECT_COLOR */
    {BLACK, LIGHTGRAY},  /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}}, /* HILITE_COLOR */

    /* ------------ SPINBUTTON ----------- */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ----------- ERRORBOX ----------- */
   {{BLACK, LIGHTGRAY},      /* STD_COLOR    */
    {BLACK, LIGHTGRAY},      /* SELECT_COLOR */
    {BLACK, LIGHTGRAY},      /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},     /* HILITE_COLOR */

    /* ----------- MESSAGEBOX --------- */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {BLACK, LIGHTGRAY}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {BLACK, LIGHTGRAY}},/* HILITE_COLOR */

    /* ----------- HELPBOX ------------ */
   {{BLACK, LIGHTGRAY}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {BLACK, LIGHTGRAY}, /* FRAME_COLOR  */
    {WHITE, LIGHTGRAY}},/* HILITE_COLOR */

    /* ---------- STATUSBAR ------------- */
   {{LIGHTGRAY, BLACK},      /* STD_COLOR    */
    {LIGHTGRAY, BLACK},      /* SELECT_COLOR */
    {LIGHTGRAY, BLACK},      /* FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},     /* HILITE_COLOR */

    /* ---------- TITLEBAR ------------ */
   {{LIGHTGRAY, BLACK},      /* STD_COLOR    */
    {LIGHTGRAY, BLACK},      /* SELECT_COLOR */
    {LIGHTGRAY, BLACK},      /* FRAME_COLOR  */
    {LIGHTGRAY, BLACK}},     /* HILITE_COLOR */

    /* ------------ DUMMY ------------- */
   {{LIGHTGRAY, BLACK}, /* STD_COLOR    */
    {LIGHTGRAY, BLACK}, /* SELECT_COLOR */
    {LIGHTGRAY, BLACK}, /* FRAME_COLOR  */
    {LIGHTGRAY, BLACK}} /* HILITE_COLOR */
};

/* ------ default configuration values ------- */
CONFIG cfg = {
    VERSION,
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

void BuildFileName(char *path, char *ext)
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

FILE *OpenConfig(char *mode)
{
	char path[64];
	BuildFileName(path, ".cfg");
	return fopen(path, mode);
}

/* ------ load a configuration file from disk ------- */
BOOL LoadConfig(void)
{
	static BOOL ConfigLoaded = FALSE;
	if (ConfigLoaded == FALSE)	{
	    FILE *fp = OpenConfig("rb");
    	if (fp != NULL)    {
        	fread(cfg.version, sizeof cfg.version+1, 1, fp);
        	if (strcmp(cfg.version, VERSION) == 0)    {
            	fseek(fp, 0L, SEEK_SET);
            	fread(&cfg, sizeof(CONFIG), 1, fp);
 		       	fclose(fp);
        	}
        	else	{
				char path[64];
				BuildFileName(path, ".cfg");
	        	fclose(fp);
				unlink(path);
            	strcpy(cfg.version, VERSION);
			}
			ConfigLoaded = TRUE;
    	}
	}
    return ConfigLoaded;
}

/* ------ save a configuration file to disk ------- */
void SaveConfig(void)
{
    FILE *fp = OpenConfig("wb");
    if (fp != NULL)    {
        fwrite(&cfg, sizeof(CONFIG), 1, fp);
        fclose(fp);
    }
}

/* --------- set window colors --------- */
void SetStandardColor(DFWINDOW wnd)
{
    foreground = WndForeground(wnd);
    background = WndBackground(wnd);
}

void SetReverseColor(DFWINDOW wnd)
{
    foreground = SelectForeground(wnd);
    background = SelectBackground(wnd);
}

/* EOF */