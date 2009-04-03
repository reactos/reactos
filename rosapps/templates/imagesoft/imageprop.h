
typedef struct _IMAGEADJUST
{
    PMAIN_WND_INFO Info;
    HWND hPicPrev;
    HBITMAP hBitmap;
    HBITMAP hPreviewBitmap;
    RECT ImageRect;
    INT RedVal;
    INT GreenVal;
    INT BlueVal;
} IMAGEADJUST, *PIMAGEADJUST;


INT_PTR CALLBACK ImagePropDialogProc(HWND hDlg,
                                     UINT message,
                                     WPARAM wParam,
                                     LPARAM lParam);

INT_PTR CALLBACK BrightnessProc(HWND hDlg,
                                UINT message,
                                WPARAM wParam,
                                LPARAM lParam);

INT_PTR CALLBACK ContrastProc(HWND hDlg,
                              UINT message,
                              WPARAM wParam,
                              LPARAM lParam);

VOID AdjustBrightness(HBITMAP hOrigBitmap,
                      HBITMAP hNewBitmap,
                      HWND hwnd,
                      HDC hdcMem,
                      INT RedVal,
                      INT GreenVal,
                      INT BlueVal);

BOOL DisplayBlackAndWhite(HWND hwnd,
                          HDC hdcMem,
                          HBITMAP hBitmap);
BOOL DisplayInvertedColors(HWND hwnd,
                           HDC hdcMem,
                           HBITMAP hBitmap);
BOOL DisplayBlur(HWND hwnd,
                 HDC hdcMem,
                 HBITMAP hBitmap);
BOOL DisplaySharpness(HWND hwnd,
                      HDC hdcMem,
                      HBITMAP hBitmap);
