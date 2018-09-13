
extern BOOL    fWinspoolLoaded;


extern DWORD (WINAPI *lpfnDEVICECAPABILITIES)(LPSTR lpDriverName, LPSTR lpDeviceName,
    WORD nIndex, LPSTR lpOutput, LPDEVMODE lpDevMode);

extern BOOL (WINAPI *lpfnDEVICEMODE)(HWND hWnd, LPSTR lpDriverName, LPSTR lpDeviceName, LPSTR lpOutput);

extern DWORD (WINAPI *lpfnEXTDEVICEMODE)(HWND hWnd,LPSTR lpDriverName,
    LPDEVMODE lpDevModeOutput, LPSTR lpDeviceName, LPSTR lpPort,
    LPDEVMODE lpDevModeInput, LPSTR lpProfile, DWORD flMode);

extern BOOL (WINAPI *lpfnOpenPrinter)(LPSTR pPrinterName, LPHANDLE phPrinter,
                               VOID *pDefault);

extern DWORD (WINAPI *lpfnStartDocPrinter)(HANDLE hPrinter, DWORD Level,
                                    LPBYTE  pDocInfo);

extern BOOL (WINAPI *lpfnStartPagePrinter)(HANDLE hPrinter);
extern BOOL (WINAPI *lpfnEndPagePrinter)(HANDLE hPrinter);
extern BOOL (WINAPI *lpfnEndDocPrinter)(HANDLE hPrinter);
extern BOOL (WINAPI *lpfnClosePrinter)(HANDLE hPrinter);
extern BOOL (WINAPI *lpfnWritePrinter)(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf,
                                LPDWORD pcWritten);
extern BOOL (WINAPI *lpfnDeletePrinter)(HANDLE hPrinter);

BOOL LoadWinspoolAndGetProcAddresses();
