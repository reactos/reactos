///////////////////////////////////////////////////
//
// main.h
//				main.cpp's lumber room :)
///////////////////////////////////////////////////

#include <package.h>

int Argc;
char **Argv;
BOOL done = FALSE;

int Help (void);
int Install (void);
int Show (void);

int SetStatus (int status1, int status2, WCHAR* text);
