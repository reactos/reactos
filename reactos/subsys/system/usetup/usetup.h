/*
*/

#ifndef __CONSOLE_H__
#define __CONSOLE_H__


#define DPRINT1(args...) do { DbgPrint("(%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT1 do { DbgPrint("%s:%d\n",__FILE__,__LINE__); } while(0);

#define DPRINT(args...)
#define CHECKPOINT


extern HANDLE ProcessHeap;


/* console.c */

NTSTATUS
AllocConsole(VOID);

VOID
FreeConsole(VOID);


NTSTATUS
ReadConsoleOutputCharacters(LPSTR lpCharacter,
			    ULONG nLength,
			    COORD dwReadCoord,
			    PULONG lpNumberOfCharsRead);

NTSTATUS
ReadConsoleOutputAttributes(PUSHORT lpAttribute,
			    ULONG nLength,
			    COORD dwReadCoord,
			    PULONG lpNumberOfAttrsRead);

NTSTATUS
WriteConsoleOutputCharacters(LPCSTR lpCharacter,
			     ULONG nLength,
			     COORD dwWriteCoord);

NTSTATUS
WriteConsoleOutputAttributes(CONST USHORT *lpAttribute,
			     ULONG nLength,
			     COORD dwWriteCoord,
			     PULONG lpNumberOfAttrsWritten);

#if 0
NTSTATUS
SetConsoleMode(HANDLE hConsoleHandle,
	       ULONG dwMode);
#endif

VOID
ConInKey(PINPUT_RECORD Buffer);

VOID
ConOutChar(CHAR c);

VOID
ConOutPuts(LPSTR szText);

VOID
ConOutPrintf(LPSTR szFormat, ...);

SHORT
GetCursorX(VOID);

SHORT
GetCursorY(VOID);

VOID
GetScreenSize(SHORT *maxx,
	      SHORT *maxy);


VOID
SetCursorType(BOOL bInsert,
	      BOOL bVisible);

VOID
SetCursorXY(SHORT x,
	    SHORT y);


VOID
ClearScreen(VOID);

VOID
SetStatusText(PCHAR Text);

VOID
SetTextXY(SHORT x, SHORT y, PCHAR Text);

VOID
PrintTextXY(SHORT x, SHORT y, char* fmt,...);

#endif /* __CONSOLE_H__*/

/* EOF */
