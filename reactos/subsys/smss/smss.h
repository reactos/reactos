
#ifndef _SMSS_H_INCLUDED_
#define _SMSS_H_INCLUDED_


#define CHILD_CSRSS     0
#define CHILD_WINLOGON  1


/* GLOBAL VARIABLES ****/

extern HANDLE SmApiPort;


/* FUNCTIONS ***********/

/* init.c */
BOOL InitSessionManager (HANDLE Children[]);


/* smss.c */
void DisplayString (LPCWSTR lpwString);
void PrintString (char* fmt,...);


/* smapi.c */
VOID STDCALL SmApiThread(HANDLE Port);



#endif /* _SMSS_H_INCLUDED_ */

/* EOF */

