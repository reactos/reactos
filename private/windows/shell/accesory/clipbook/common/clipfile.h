
/******************************************************************************

                    C L I P F I L E   H E A D E R

    Name:       clipfile.h
    Date:       1/20/94
    Creator:    John Fu

    Description:
        This is the header file for clipfile.c

******************************************************************************/




#define READFILE_SUCCESS         0
#define READFILE_IMPROPERFORMAT  1
#define READFILE_OPENCLIPBRDFAIL 2





extern  BOOL    fAnythingToRender;

extern  TCHAR   szFileSpecifier[];
extern  TCHAR   szFileName[MAX_PATH+1];
extern  TCHAR   szSaveFileName[MAX_PATH+1];     // Saved filename for delayed render

extern  BOOL    fNTReadFileFormat;
extern  BOOL    fNTSaveFileFormat;

extern  UINT    cf_link;
extern  UINT    cf_objectlink;
extern  UINT    cf_linkcopy;
extern  UINT    cf_objectlinkcopy;



extern TCHAR szCaptionName[];




extern  HANDLE RenderFormat(FORMATHEADER *, register HANDLE);


// winball additions

extern  BOOL AddNetInfoToClipboard (TCHAR *);
extern  BOOL AddPreviewFormat (VOID);
extern  BOOL AddCopiedFormat (UINT ufmtOriginal, UINT ufmtCopy);
extern  BOOL AddDIBtoDDB(VOID);

// end winball





// Functions


unsigned ReadFileHeader(
    HANDLE  fh);


BOOL ReadFormatHeader(
    HANDLE          fh,
    FORMATHEADER    *pfh,
    unsigned        iFormat);


short ReadClipboardFromFile(
    HWND    hwnd,
    HANDLE  fh);


DWORD OpenClipboardFile(
    HWND    hwnd,
    LPTSTR  szName);


HANDLE RenderFormatFromFile(
    LPTSTR  szFile,
    WORD    wFormat);


HANDLE RenderAllFromFile(
    LPTSTR  szFile);


BOOL IsWriteable(
    WORD Format);


int Count16BitClipboardFormats(void);


DWORD WriteFormatBlock(
    HANDLE  fh,
    DWORD   offset,
    DWORD   DataOffset,
    DWORD   DataLen,
    UINT    Format,
    LPWSTR  wszName);


DWORD WriteDataBlock(
    register HANDLE hFile,
    DWORD           offset,
    WORD            Format);


void GetClipboardNameW(
    register int    fmt,
    LPWSTR          wszName,
    register int    iSize);


DWORD SaveClipboardData(
    HWND    hwnd,
    LPTSTR  szFileName,
    BOOL    fPage);


DWORD SaveClipboardToFile(
    HWND    hwnd,
    TCHAR   *szShareName,
    TCHAR   *szFileName,
    BOOL    fPage);


BOOL AddPreviewFormat (VOID);


BOOL AddCopiedFormat (
    UINT    ufmtOriginal,
    UINT    ufmtCopy);


BOOL AddNetInfoToClipboard (
    TCHAR   *szShareName );
