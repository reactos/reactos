
extern HANDLE ProcessHandle;
extern BOOL fKD;

#undef DECLARE_API

#undef d_printf
#undef GetExpression
#undef GetSymbol
#undef Disasm
#undef CheckControlC

#define d_printf         (ExtensionApis.lpOutputRoutine)
#define d_GetExpression   (ExtensionApis.lpGetExpressionRoutine)
#define d_GetSymbol       (ExtensionApis.lpGetSymbolRoutine)
#define d_Disasm          (ExtensionApis.lpGetDisasmRoutine)
#define d_CheckControlC   (ExtensionApis.lpCheckControlCRoutine)

#define DECLARE_API(s)                              \
        VOID                                        \
        s(                                          \
            HANDLE               hCurrentProcess,   \
            HANDLE               hCurrentThread,    \
            DWORD                dwCurrentPc,       \
            PWINDBG_EXTENSION_APIS pExtensionApis,  \
            LPSTR                lpArgumentString   \
            )

#define INIT_DPRINTF()    { if (!fKD) ExtensionApis = *pExtensionApis; ProcessHandle = hCurrentProcess; }

extern WINDBG_EXTENSION_APIS ExtensionApis;

#define MIN(x, y) ((x) < (y)) ? x:y

extern 
BOOL
GetData(IN DWORD dwAddress,  IN LPVOID ptr, IN ULONG size, IN PCSTR type );


