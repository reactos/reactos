/****************************************************************************
Module name  : SendKeys.H
Description  : Include File for SendKeys utility functions.
*****************************************************************************/

#ifndef _INC_SENDKEYS
#define _INC_SENDKEYS		/* #defined if SendKeys.h has been included */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

#define ARRAY_LEN(Array)			(sizeof(Array) / sizeof(Array[0]))
#define INRANGE(low, val, high) ((low <= val) && (val <= high))
#define TOUPPER(Char)		((BYTE) (DWORD) AnsiUpper((LPSTR) MAKEINTRESOURCE(Char)))



// ************************ Function Prototypes ******************************
typedef enum {
	SK_NOERROR, SK_MISSINGCLOSEBRACE, SK_INVALIDKEY,
	SK_MISSINGCLOSEPAREN, SK_INVALIDCOUNT, SK_STRINGTOOLONG,
	SK_CANTINSTALLHOOK
} SENDKEYSERR;

SENDKEYSERR WINAPI _export SendKeys (LPCSTR szKeys);
SENDKEYSERR WINAPI VMSendKeys (LPCSTR szKeys);
void WINAPI PostVirtualKeyEvent (BYTE bVirtKey, BOOL fUp);

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */
#endif  /* _INC_SENDKEYS */


