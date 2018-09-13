/*** unifunc.h  ***/

#define CharSizeOf(x)   (sizeof(x) / sizeof(*x))
#define ByteCountOf(x)  ((x) * sizeof(TCHAR))

#define ARRAYSIZE(x)   (sizeof(x) / sizeof(*x))

TCHAR *UToDecT( UINT value, TCHAR *sz);
