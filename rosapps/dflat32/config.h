/* ---------------- config.h -------------- */

#ifndef CONFIG_H
#define CONFIG_H

enum colortypes {
	STD_COLOR,
	SELECT_COLOR,
	FRAME_COLOR,
	HILITE_COLOR
};

enum grounds { FG, BG };

/* ----------- configuration parameters ----------- */
typedef struct config {
    char version[sizeof VERSION];
    BOOL InsertMode;   /* Editor insert mode                 */
    int Tabs;          /* Editor tab stops                   */
    BOOL WordWrap;     /* True to word wrap editor           */
#ifdef INCLUDE_WINDOWOPTIONS
    BOOL Border;       /* True for application window border */
    BOOL Title;        /* True for application window title  */
	BOOL StatusBar;    /* True for appl'n window status bar  */
    BOOL Texture;      /* True for textured appl window      */
#endif
//    int ScreenLines;   /* Number of screen lines (25/43/50)  */
	char PrinterPort[5];
	int LinesPage;     /* Lines per printer page             */
	int CharsLine;	   /* Characters per printer line        */
	int LeftMargin;	   /* Printer margins                    */
	int RightMargin;
	int TopMargin;
	int BottomMargin;
    unsigned char clr[CLASSCOUNT] [4] [2]; /* Colors         */
} CONFIG;

extern CONFIG cfg;
extern unsigned char color[CLASSCOUNT] [4] [2];
extern unsigned char bw[CLASSCOUNT] [4] [2];
extern unsigned char reverse[CLASSCOUNT] [4] [2];

BOOL LoadConfig(void);
void SaveConfig(void);
FILE *OpenConfig(char *);

#endif

