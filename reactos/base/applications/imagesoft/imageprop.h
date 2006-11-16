
typedef struct _IMAGEADJUST
{
    PMAIN_WND_INFO Info;
    HWND hPicPrev;
    HBITMAP hBitmap;
    RECT ImageRect;
    DWORD OldTrackPos;
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

VOID AdjustBrightness(PIMAGEADJUST pImgAdj,
                      HDC hdcMem);
