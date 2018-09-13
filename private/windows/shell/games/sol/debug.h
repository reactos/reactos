#include <stdlib.h>

typedef struct
	{
	VOID *pgmcol;
	WORD lvl;
	INT msg;
	WPARAM wp1;
	LPARAM wp2;
	LRESULT wResult;
	} MDBG;



#define imdbgMax 500

VOID PrintCardMacs( GM * );
BOOL FSetGameNo( VOID );
VOID InitDebug( VOID );
