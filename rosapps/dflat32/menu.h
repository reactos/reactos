/* ------------ menu.h ------------- */

#ifndef MENU_H
#define MENU_H

#define MAXPULLDOWNS 15
#define MAXSELECTIONS 20
#define MAXCASCADES 3  /* nesting level of cascaded menus */

/* ----------- popdown menu selection structure
       one for each selection on a popdown menu --------- */
struct PopDown {
    unsigned char *SelectionTitle; /* title of the selection */
    int ActionId;          /* the command executed        */
    int Accelerator;       /* the accelerator key         */
    int Attrib;  /* INACTIVE | CHECKED | TOGGLE | CASCADED*/
    char *help;            /* Help mnemonic               */
};

/* ----------- popdown menu structure
       one for each popdown menu on the menu bar -------- */
typedef struct Menu {
    char *Title;           /* title on the menu bar       */
    void (*PrepMenu)(void *, struct Menu *); /* function  */
	char *StatusText;      /* text for the status bar     */
	int CascadeId;   /* command id of cascading selection */
    int Selection;         /* most recent selection       */
    struct PopDown Selections[MAXSELECTIONS+1];
} MENU;

/* ----- one for each menu bar ----- */
typedef struct MenuBar {
	int ActiveSelection;
	MENU PullDown[MAXPULLDOWNS+1];
} MBAR;

/* --------- macros to define a menu bar with
                 popdowns and selections ------------- */
#define SEPCHAR "\xc4"
#define DEFMENU(m) MBAR m = {-1,{
#define POPDOWN(ttl,func,stat)     {ttl,func,stat,-1,0,{
#define CASCADED_POPDOWN(id,func)  {NULL,func,NULL,id,0,{
#define SELECTION(stxt,acc,id,attr)   {stxt,acc,id,attr,#acc},
#define SEPARATOR                     {SEPCHAR},
#define ENDPOPDOWN                    {NULL}}},
#define ENDMENU                {(void *)-1} }};

/* -------- menu selection attributes -------- */
#define INACTIVE    1
#define CHECKED     2
#define TOGGLE      4
#define CASCADED    8    

/* --------- the standard menus ---------- */
extern MBAR MainMenu;
extern MBAR SystemMenu;
extern MBAR *ActiveMenuBar;

int MenuHeight(struct PopDown *);
int MenuWidth(struct PopDown *);

#endif

