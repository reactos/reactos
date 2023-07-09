/* special characters for handling symbol information */
/* note that we use characters in the private use area, as */
/* these will never be emitted (hopefully) by MultiByteToWideChar */
#define SYMDEFSTART 0xe000
#define SYMUSESTART 0xe001
#define SYMDELIMIT  0xe002
#define USR_RESOURCE 0
#define SYS_RESOURCE 1
#define IGN_RESOURCE 2
#define WIN_RESOURCE 3

extern int afxReadOnlySymbols;
extern int afxHiddenSymbols;
extern WCHAR* afxSzReadOnlySymbols;
extern WCHAR* afxSzHiddenSymbols;
