/* ---------------- config.h -------------- */

#ifndef CONFIG_H
#define CONFIG_H

enum DfColorTypes {
	DF_STD_COLOR,
	DF_SELECT_COLOR,
	DF_FRAME_COLOR,
	DF_HILITE_COLOR
};

enum DfGrounds { DF_FG, DF_BG };

/* ----------- configuration parameters ----------- */
typedef struct DfConfig {
    char version[sizeof DF_VERSION];
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
    unsigned char clr[DF_CLASSCOUNT] [4] [2]; /* Colors         */
} DFCONFIG;

extern DFCONFIG DfCfg;
extern unsigned char DfColor[DF_CLASSCOUNT] [4] [2];
extern unsigned char DfBW[DF_CLASSCOUNT] [4] [2];
extern unsigned char DfReverse[DF_CLASSCOUNT] [4] [2];

BOOL DfLoadConfig(void);
void DfSaveConfig(void);
FILE *DfOpenConfig(char *);

#endif

