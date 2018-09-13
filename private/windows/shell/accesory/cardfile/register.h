#define KEYNAMESIZE     300
#define CLASSES (TEXT(".classes"))

VOID FAR GetClassId(HWND hwnd, LPTSTR lpstrClass);
INT  FAR MakeFilterSpec(LPTSTR lpstrClass, LPTSTR lpstrExt, LPTSTR lpstrFilterSpec);
VOID FAR RegInit(VOID);
VOID FAR RegTerm(VOID);
