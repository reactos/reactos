#ifndef PACKUTIL_H__
#define PACKUTIL_H__

#define CHAR_SPACE          TEXT(' ')
#define CHAR_QUOTE          TEXT('"')
#define SZ_QUOTE            TEXT("\"")
#define BUFFERSIZE          4096    // 4k buffer size for copy operations

/////////////////////////////////
// Icon structure
//
typedef struct _IC                      // ic
{
    HICON hDlgIcon;                     // handle to icon
    TCHAR szIconPath[MAX_PATH];        // path to icon
    TCHAR szIconText[MAX_PATH];        // text for icon
    INT iDlgIcon;                       // index of icon in a resource
    RECT rc;                            // bounding rect of icon and text
} IC, *LPIC;
    

VOID ReplaceExtension(LPTSTR lpstrTempFile,LPTSTR lpstrOrigFile);
LPIC IconCreate(void);
LPIC IconCreateFromFile(LPTSTR);
BOOL IconCalcSize(LPIC lpic);
VOID IconDraw(LPIC,HDC,LPRECT);
VOID GetCurrentIcon(LPIC lpic);
VOID GetDisplayName(LPTSTR, LPCTSTR);

HRESULT CopyStreamToFile(IStream*, LPTSTR);
HRESULT CopyFileToStream(LPTSTR, IStream*);
HRESULT StringReadFromStream(IStream* pstm, LPSTR pszBuffer, UINT cchChar);
HRESULT StringWriteToStream(IStream* pstm, LPCSTR pszBuffer, DWORD *pdwWrite);
BOOL PathSeparateArgs(LPTSTR pszPath, LPTSTR pszArgs);

#endif
