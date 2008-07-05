#define MONOCHROMEBITS  1
#define GREYSCALEBITS   8
#define PALLETEBITS     8
#define TRUECOLORBITS   24

#define PIXELS      0
#define CENTIMETERS 1
#define INCHES      2


/* generic definitions and forward declarations */
struct _MAIN_WND_INFO;
struct _EDIT_WND_INFO;


typedef enum _MDI_EDITOR_TYPE {
    metUnknown = 0,
    metImageEditor,
} MDI_EDITOR_TYPE, *PMDI_EDITOR_TYPE;

typedef enum
{
    tSelect = 0,
    tMove,
    tLasso,
    tZoom,
    tMagicWand,
    tBrush,
    tEraser,
    tPencil,
    tColorPick,
    tStamp,
    tFill,
    tLine,
    tPolyline,
    tRectangle,
    tRoundRectangle,
    tPolygon,
    tElipse,
} TOOL;

typedef struct _OPEN_IMAGE_EDIT_INFO
{
    BOOL CreateNew;
    union
    {
        struct
        {
            LONG Width;
            LONG Height;
        } New;
        struct
        {
            LPTSTR lpImagePath;
        } Open;
    };
    LPTSTR lpImageName;
    USHORT Type;
    LONG Resolution;
} OPEN_IMAGE_EDIT_INFO, *POPEN_IMAGE_EDIT_INFO;

typedef struct _EDIT_WND_INFO
{
    MDI_EDITOR_TYPE MdiEditorType; /* Must be first member! */

    HWND hSelf;
    HBITMAP hBitmap;
    HDC hDCMem;
    PBITMAPINFO pbmi;
    PBYTE pBits;
    struct _MAIN_WND_INFO *MainWnd;
    struct _EDIT_WND_INFO *Next;
    POINT ScrollPos;
    USHORT Zoom;
    DWORD Tool;

    POPEN_IMAGE_EDIT_INFO OpenInfo; /* Only valid during initialization */

    /* Canvas properties */
    USHORT Type;
    LONG Resolution;
    /* size of drawing area */
    LONG Width;
    LONG Height;

} EDIT_WND_INFO, *PEDIT_WND_INFO;


BOOL CreateImageEditWindow(struct _MAIN_WND_INFO *MainWnd,
                           POPEN_IMAGE_EDIT_INFO OpenInfo);
VOID SetImageEditorEnvironment(PEDIT_WND_INFO Info,
                               BOOL Setup);
BOOL InitImageEditWindowImpl(VOID);
VOID UninitImageEditWindowImpl(VOID);
