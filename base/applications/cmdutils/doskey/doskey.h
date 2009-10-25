#define IDS_HELP 0
#define IDS_INVALID_MACRO_DEF 1

#ifndef RC_INVOKED

#ifdef UNICODE
#define TNAME(x) x##W
#else
#define TNAME(x) x##A
#endif

/* Missing from include/psdk/wincon.h */
#ifndef ENABLE_INSERT_MODE
#define ENABLE_INSERT_MODE 0x20
#endif
#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS 0x80
#endif

/* Undocumented APIs */
#ifndef AddConsoleAlias
BOOL WINAPI AddConsoleAliasA(LPSTR, LPSTR, LPSTR);
BOOL WINAPI AddConsoleAliasW(LPWSTR, LPWSTR, LPWSTR);
#define AddConsoleAlias TNAME(AddConsoleAlias)
#endif
#ifndef ExpungeConsoleCommandHistory
BOOL WINAPI ExpungeConsoleCommandHistoryA(LPSTR);
BOOL WINAPI ExpungeConsoleCommandHistoryW(LPWSTR);
#define ExpungeConsoleCommandHistory TNAME(ExpungeConsoleCommandHistory)
#endif
#ifndef GetConsoleAliases
DWORD WINAPI GetConsoleAliasesA(LPSTR, DWORD, LPSTR);
DWORD WINAPI GetConsoleAliasesW(LPWSTR, DWORD, LPWSTR);
#define GetConsoleAliases TNAME(GetConsoleAliases)
#endif
#ifndef GetConsoleAliasesLength
DWORD WINAPI GetConsoleAliasesLengthA(LPSTR); 
DWORD WINAPI GetConsoleAliasesLengthW(LPWSTR); 
#define GetConsoleAliasesLength TNAME(GetConsoleAliasesLength)
#endif
#ifndef GetConsoleAliasExes
DWORD WINAPI GetConsoleAliasExesA(LPSTR, DWORD);
DWORD WINAPI GetConsoleAliasExesW(LPWSTR, DWORD);
#define GetConsoleAliasExes TNAME(GetConsoleAliasExes)
#endif
#ifndef GetConsoleAliasExesLength
DWORD WINAPI GetConsoleAliasExesLengthA(VOID);
DWORD WINAPI GetConsoleAliasExesLengthW(VOID);
#define GetConsoleAliasExesLength TNAME(GetConsoleAliasExesLength)
#endif
#ifndef GetConsoleCommandHistory
DWORD WINAPI GetConsoleCommandHistoryA(LPSTR, DWORD, LPSTR);
DWORD WINAPI GetConsoleCommandHistoryW(LPWSTR, DWORD, LPWSTR);
#define GetConsoleCommandHistory TNAME(GetConsoleCommandHistory)
#endif
#ifndef GetConsoleCommandHistoryLength
DWORD WINAPI GetConsoleCommandHistoryLengthA(LPSTR);
DWORD WINAPI GetConsoleCommandHistoryLengthW(LPWSTR);
#define GetConsoleCommandHistoryLength TNAME(GetConsoleCommandHistoryLength)
#endif
#ifndef SetConsoleNumberOfCommands
BOOL WINAPI SetConsoleNumberOfCommandsA(DWORD, LPSTR);
BOOL WINAPI SetConsoleNumberOfCommandsW(DWORD, LPWSTR);
#define SetConsoleNumberOfCommands TNAME(SetConsoleNumberOfCommands)
#endif

#endif /* RC_INVOKED */
