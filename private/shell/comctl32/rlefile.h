//
//  handle AVI RLE files with custom code.
//
//  use this code to deal with .AVI files without the MCIAVI runtime
//
//  restrictions:
//
//          AVI file must be a native DIB format (RLE or none)
//          AVI file must fit into memory.
//

#define FOURCC DWORD
#if defined(WIN32) && !defined(WINNT)
#include <vfw.h>
#else
// HACK to build for now under NT
#include <avifmt.h>
#endif

#ifdef WIN32
#define PTR
#else
#define PTR _huge   /* or FAR */
#endif

typedef struct _RLEFILE {
    int                 NumFrames;      // number of frames
    int                 Width;          // width in pixels
    int                 Height;         // height in pixels
    int                 Rate;           // mSec per frame

    HPALETTE            hpal;           // palete for drawing

    HANDLE              hRes;           // resource handle
    LPVOID              pFile;          // bits of file.

    int                 iFrame;         // current frame
    int                 iKeyFrame;      // nearest key
    int                 nFrame;         // index pos of frame.
    LPVOID              pFrame;         // current frame data
    DWORD               cbFrame;        // size in bytes of frame

    DWORD               FullSizeImage;  // full-frame size
    BITMAPINFOHEADER    bi;             // DIB format
    DWORD               rgbs[256];      // the colors
    MainAVIHeader PTR  *pMainHeader;    // main header
    int                 iStream;        // stream number of video
    AVIStreamHeader PTR*pStream;        // video stream
    LPBITMAPINFOHEADER  pFormat;        // format of video stream
    LPVOID              pMovie;         // movie chunk
    UNALIGNED AVIINDEXENTRY PTR * pIndex; // master index

}   RLEFILE;

extern BOOL RleFile_OpenFromFile(RLEFILE *prle, LPCTSTR szFile);
extern BOOL RleFile_OpenFromResource(RLEFILE *prle, HINSTANCE hInstance, LPCTSTR szName, LPCTSTR szType);
extern BOOL RleFile_Close(RLEFILE  *prle);
extern BOOL RleFile_SetColor(RLEFILE  *prle, int iColor, COLORREF rgb);
extern BOOL RleFile_ChangeColor(RLEFILE  *prle, COLORREF rgbS, COLORREF rgbD);
extern BOOL RleFile_Seek(RLEFILE  *prle, int iFrame);
extern BOOL RleFile_Paint(RLEFILE  *prle, HDC hdc, int iFrame, int x, int y);
extern BOOL RleFile_Draw(RLEFILE  *prle, HDC hdc, int iFrame, int x, int y);

#define RleFile_New()       ((RLEFILE *)LocalAlloc(LPTR, sizeof(RLEFILE)))
#define RleFile_Free(pavi)  (RleFile_Close(pavi), LocalFree((HLOCAL)(pavi)))

#define RleFile_NumFrames(prle)     ((prle)->NumFrames)
#define RleFile_Width(prle)         ((prle)->Width)
#define RleFile_Height(prle)        ((prle)->Height)
#define RleFile_Rate(prle)          ((prle)->Rate)
