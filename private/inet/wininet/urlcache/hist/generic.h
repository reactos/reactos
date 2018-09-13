//generic.h

BOOL
ParseArgsDyn(
    LPSTR InBuffer,
    LPSTR **pArgv,
    LPDWORD pArgc
    );

DWORD 
AddArgvDyn (LPTSTR **pArgv, DWORD *pArgc, LPTSTR szNew);

LPBYTE
MemFind (LPBYTE lpB, DWORD cbB, LPBYTE lpP, DWORD cbP);
