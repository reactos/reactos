/**************************************************/
/*					                              */
/*					                              */
/*	EudcEditor Utillity funcs	                  */
/*					                              */
/*					                              */
/* Copyright (c) 1997-1999 Microsoft Corporation. */
/**************************************************/

int  OutputMessageBox( HWND hWnd, UINT TitleID, UINT MessgID, BOOL OkFlag);
#ifdef BUILD_ON_WINNT
int  OutputMessageBoxEx( HWND hWnd, UINT TitleID, UINT MessgID, BOOL OkFlag, ...);
#endif // BUILD_ON_WINNT
void GetStringRes( LPTSTR lpStr, UINT sID);
void ConvStringRes( LPTSTR lpStr, CString String);

#ifdef UNICODE
#define Mytcsrchr wcsrchr
#define Mytcschr wcschr
#define Mytcstok wcstok
#define Mytcstol wcstol
#define Mytcsstr wcsstr
#define Myttoi _wtoi
#else
char * Mystrrchr(char *pszString, char ch);
char * Mystrchr(char *pszString, char ch);
#define Mytcsrchr Mystrrchr
#define Mytcschr Mystrchr
#define Mytcstok strtok
#define Mytcstol strtol
#define Mytcsstr strstr
#define Myttoi atoi
#endif

