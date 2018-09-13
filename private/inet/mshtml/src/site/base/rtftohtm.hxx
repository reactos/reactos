#ifndef I_RTFTOHTM_HXX_
#define I_RTFTOHTM_HXX_
#pragma INCMSG("--- Beg 'rtftohtm.hxx'")

class   CDoc;

//+---------------------------------------------------------------
//
//  Class:      CRtfToHtmlConverter
//
//  Purpose:    Support conversion between RTF and HTML through
//              the use of Word's html32.cnv converter.
//
//---------------------------------------------------------------

MtExtern(CRtfToHtmlConverter)

class CRtfToHtmlConverter
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRtfToHtmlConverter))
    CRtfToHtmlConverter(CDoc *);
    ~CRtfToHtmlConverter(void);

    HRESULT InternalHtmlToExternalRtf(LPCTSTR pszRtfPath,
                                      LPCTSTR pszHtmlPath = NULL);
    HRESULT ExternalRtfToInternalHtml(TCHAR *);
    HRESULT InternalHtmlToStreamRtf(IStream *);
    HRESULT StringRtftoStringHtml(LPSTR, HGLOBAL *);
    HRESULT StringHtmltoStringRtf(LPSTR, HGLOBAL *);

private:
    BOOL        _fInitSuccessful;
    CDoc *      _pDoc;
    HANDLE      _hExternalFile;
    HINSTANCE   _hConverter;
    HANDLE      _hTransferBuffer;
    char *      _pchModuleName;

    short   (WINAPI * _pfnIsFormatCorrect)(
                    HANDLE haszFile,
                    HANDLE haszClass);
    short   (WINAPI * _pfnHtmlToRtf)(
                    HANDLE ghszFile,
                    IStorage * pstgForeign,
                    HANDLE ghBuf,
                    HANDLE ghszClass,
                    HANDLE ghszSubset,
                    short (FAR PASCAL *)(LONG, INT));
    short   (WINAPI * _pfnRtfToHtml)(
                    HANDLE ghszFile,
                    IStorage * pstgForeign,
                    HANDLE ghBuf,
                    HANDLE ghszClass,
                    short (FAR PASCAL *)(BOOL *, INT));
};

#pragma INCMSG("--- End 'rtftohtm.hxx'")
#else
#pragma INCMSG("*** Dup 'rtftohtm.hxx'")
#endif
