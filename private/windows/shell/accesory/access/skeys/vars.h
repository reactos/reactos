/*  VARS.H  */

#include "vars1.h"

typedef  struct tagMouseKeysParam {
	int		NumButtons;		/* holds number of buttons on the mice	*/
	int		Delta_Y;		/* Relative Y motion sign extended		*/
	int		Delta_X;		/* Relative X motion sign extended		*/
	int		Status;			/* status of mouse buttons and motion	*/
} MOUSEKEYSPARAM;


#define TRUE 1
#define FALSE 0

#define TAB 9
#define LINEFEED 10
#define VERTICALTAB 11
#define FORMFEED 12
#define RETURN 13
#define SPACE 32
#define COMMA 44
#define PERIOD 46
#define ESC 27
#define ESCAPE 27

#define notOKstatus 0
#define okStatus 1
#define NOKEY 0

extern void *aliasStack[MAXVECTORSTACK];
extern void *vectorStack[MAXVECTORSTACK];
extern int stackPointer;
extern unsigned char lastCode;
extern void (*serialVector)(unsigned char);
extern void (*codeVector)(unsigned char);
extern void (*commandVector)(unsigned char);
extern struct aliasTable *aliasPtr;

extern struct listTypes tempList, keyHoldList, keyLockList;
extern char cAliasString[MAXALIASLEN];
extern int nullCount;
extern int blockCount;

extern char buf[CODEBUFFERLEN];
extern int spos,rpos;

extern int passAll, fatalErrorFlag, stdErrorFlag, waitForClear, beginOK;

extern int mouseX, mouseY;
extern MOUSEKEYSPARAM mouData;
extern MOUSEKEYSPARAM *mouseDataPtr;
extern int requestButton1, requestButton2, requestButton3;
extern int button1Status, button2Status, button3Status;

extern struct aliasTable nullTable[];

