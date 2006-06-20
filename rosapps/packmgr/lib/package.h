////////////////////////////////////////////////
//
// package.hpp
//					Package C++ Header
////////////////////////////////////////////////

#include <windows.h>
#include "error.h"


/* Callbacks */

typedef int (*PML_AddItem)		(int id, const char* name, int parent, int icon);
typedef int (*PML_SetStatus)	(int status1, int status2, WCHAR* text);
typedef int (*PML_SetButton)	(DWORD dwID, BOOL state);
typedef int (*PML_SetIcon)		(int id, int icon);
typedef int (*PML_SetText)		(const char* text);
typedef int (*PML_Ask)			(const WCHAR* text);


/* Structs */

typedef void* pTree;

#define MAXNODES 10000


/* Prototypes */

void PML_Abort (void);
WCHAR* PML_TransError (int code, WCHAR* string, INT maxchar);

int PML_LoadTree (pTree*, char* url, PML_AddItem);
int PML_FindItem (pTree tree, const char* what);
int PML_LoadPackage (pTree, int id, PML_SetButton);
char* PML_GetDescription (pTree tree, int id);
int PML_SetAction (pTree, int package, int action, PML_SetIcon, PML_Ask);
int PML_DoIt (pTree, PML_SetStatus, PML_Ask);

void PML_CloseTree (pTree);


/* Version */ 

#define PACKMGR_VERSION_MAJOR		0
#define PACKMGR_VERSION_MINOR		3
#define PACKMGR_VERSION_PATCH_LEVEL	1
