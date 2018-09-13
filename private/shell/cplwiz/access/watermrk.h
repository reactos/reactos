


////////////////////////////////////
//
// Stuff used for watermarking
//

#define WIZPAGE_FULL_PAGE_WATERMARK 0x00000001
#define WIZPAGE_SEPARATOR_CREATED   0x00000002

BOOL
GetBitmapDataAndPalette(
    IN  HINSTANCE                hInst,
    IN  LPCTSTR                  Id,
    OUT HPALETTE                *Palette,
    OUT PUINT                    ColorCount,
    OUT CONST BITMAPINFOHEADER **BitmapData
    );

BOOL
WaterMarkWizardDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
PaintWatermark(
    IN HWND hdlg,
    IN HDC  DialogDC,
    IN UINT XOffset,
    IN UINT YOffset
    );


void WaterMarkSetup(
	IN HWND hwndWizard
	);

void Watermark_SetClipHeight(
	IN INT nWatermarkHeight
	);


void Watermark_InitWizardPage(
	IN HWND hwndWizardPage
	);

//
////////////////////////////////////
