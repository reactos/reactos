/*  VARS.C  */

//#define     WINVER 0x0300

#include	"windows.h"
//#include "winstric.h"                        /* added for win 3.1 compatibility 1/92 */
#include "drivers.h"
#include "vars1.h"
#include "gide.h"

void 	*aliasStack[MAXVECTORSTACK];
void	*vectorStack[MAXVECTORSTACK];

int	stackPointer;
unsigned char lastCode;
void	(*serialVector)();
void	(*codeVector)();
int	(*commandVector)();
struct aliasTable *aliasPtr;

struct aliasTable nullTable[] = 
{
	{ "",		0	},
};

struct listTypes tempList, keyHoldList, keyLockList;
char cAliasString[MAXALIASLEN];
int nullCount;
int blockCount;

char buf[CODEBUFFERLEN];
int spos,rpos;

int passAll, fatalErrorFlag, stdErrorFlag, waitForClear, beginOK;

int mouseX, mouseY;

MOUSEKEYSPARAM mouData = {2, 0, 0, 0};
MOUSEKEYSPARAM *mouseDataPtr = &mouData;
int requestButton1, requestButton2, requestButton3 = FALSE;
int button1Status, button2Status, button3Status = FALSE;
