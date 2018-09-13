//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       packager.hxx
//
//  Contents:   Save as Office9 'Thicket' format implementation classes.
//
//----------------------------------------------------------------------------

#ifndef _PACKAGER_HXX_
#define _PACKAGER_HXX_

#define MAX_SAVING_STATUS_TEXT                128
#define MAX_BUFFER_LEN                        512
#define REGPATH_MSIE_MAIN                     TEXT("Software\\Microsoft\\Internet Explorer\\Main")
#define REGVALUE_DOWNLOAD_IMAGES              "Display Inline Images"
#define URL_ABOUT_BLANK                       TEXT("about:blank")

class CWebArchive;

class CThicketProgress
{
public:
    CThicketProgress( HWND hDlg );
    ~CThicketProgress(void);

    void SetPercent( ULONG ulPct );
    void SetSaving( LPCTSTR szFile, LPCTSTR szDst );
    void SetSaveText(LPCTSTR szText);

protected:
    HWND    m_hDlg;
    HWND    m_hwndProg;
    TCHAR*  m_pszSavingFmt;
    int     m_cchSavingFmt;
    TCHAR*  m_pszPctFmt;
    ULONG   m_ulPct;
};

/*
 *  CDocumentPackager - master packager class.
 */

// Packaging styles. 
// NOTE: These need to match the order in the format filter string.
enum {
    PACKAGE_THICKET = 1,
    PACKAGE_MHTML,
    PACKAGE_HTML,
    PACKAGE_TEXT
};

class CDocumentPackager
{
public:
    CDocumentPackager(UINT iPackageStyle) {  m_iPackageStyle = iPackageStyle;
                                             m_ptp = NULL; }
    ~CDocumentPackager(void) {}

    HRESULT PackageDocument(IHTMLDocument2 *pDoc, LPCTSTR lpstrDoc,
                            BOOL *pfCancel, CThicketProgress *ptprog,
                            ULONG progLow, ULONG progHigh,
                            UINT cpDst,
                            CWebArchive *pwa = NULL );

    CWebArchive *GetFrameDocArchive( CWebArchive *pwaSrc );

protected:
    friend class CFramesPackager;

    UINT    m_iPackageStyle;

    HRESULT _PackageDocument(IHTMLDocument2 *pDoc, LPCTSTR lpstrDoc,
                             BOOL *pfCancel, CThicketProgress *ptprog,
                             ULONG progLow, ULONG progHigh,
                             UINT cpDst,
                             CWebArchive *pwa,
                             CDocumentPackager *pdpFrames,
                             BOOL fFrameDoc);

    HRESULT _GetDesignDoc( IHTMLDocument2 *pDocSrc, IHTMLDocument2 **ppDocDesign, 
                           BOOL *pfCancel, CThicketProgress *ptp, UINT cp );
private:
    CThicketProgress                 *m_ptp;
};


#endif // _PACKAGER_HXX_
