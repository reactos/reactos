/* ------------ menu.h ------------- */

#ifndef MENU_H
#define MENU_H

#define DF_MAXPULLDOWNS 15
#define DF_MAXSELECTIONS 20
#define DF_MAXCASCADES 3  /* nesting level of cascaded menus */

/* ----------- popdown menu selection structure
       one for each selection on a popdown menu --------- */
struct DfPopDown {
    unsigned char *SelectionTitle; /* title of the selection */
    int ActionId;          /* the command executed        */
    int Accelerator;       /* the accelerator key         */
    int Attrib;  /* DF_INACTIVE | DF_CHECKED | DF_TOGGLE | DF_CASCADED*/
    char *help;            /* Help mnemonic               */
};

/* ----------- popdown menu structure
       one for each popdown menu on the menu bar -------- */
typedef struct DfMenu {
    char *Title;           /* title on the menu bar       */
    void (*PrepMenu)(void *, struct DfMenu *); /* function  */
	char *StatusText;      /* text for the status bar     */
	int CascadeId;   /* command id of cascading selection */
    int Selection;         /* most recent selection       */
    struct DfPopDown Selections[DF_MAXSELECTIONS+1];
} DF_MENU;

/* ----- one for each menu bar ----- */
typedef struct DfMenuBar {
	int ActiveSelection;
	DF_MENU PullDown[DF_MAXPULLDOWNS+1];
} DF_MBAR;

/* --------- macros to define a menu bar with
                 popdowns and selections ------------- */
#define DF_SEPCHAR "\xc4"
#define DF_DEFMENU(m) DF_MBAR m = {-1,{
#define DF_POPDOWN(ttl,func,stat)     {ttl,func,stat,-1,0,{
#define DF_CASCADED_POPDOWN(id,func)  {NULL,func,NULL,id,0,{
#define DF_SELECTION(stxt,acc,id,attr)   {stxt,acc,id,attr,#acc},
#define DF_SEPARATOR                     {DF_SEPCHAR},
#define DF_ENDPOPDOWN                    {NULL}}},
#define DF_ENDMENU                {(void *)-1} }};

/* -------- menu selection attributes -------- */
#define DF_INACTIVE    1
#define DF_CHECKED     2
#define DF_TOGGLE      4
#define DF_CASCADED    8    

/* --------- the standard menus ---------- */
extern DF_MBAR DfMainMenu;
extern DF_MBAR DfSystemMenu;
extern DF_MBAR *DfActiveMenuBar;

int DfMenuHeight(struct DfPopDown *);
int DfMenuWidth(struct DfPopDown *);

#endif

