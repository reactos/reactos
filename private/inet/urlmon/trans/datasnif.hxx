#ifndef _DATASNIF_HXX
#define _DATASNIF_HXX
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       datasnif.hxx
//
//  Contents:   Stream Mime type checking (attempts to guess the MIME type
//              of a buffer by simple pattern matching).
//
//  Classes:    ContentAnalyzer
//
//  Functions:  private:
//
//                CContentAnalyzer::SampleData
//                CContentAnalyzer::IsBMP
//                CContentAnalyzer::GetDataFormat
//                CContentAnalyzer::FormatAgreesWithData
//                CContentAnalyzer::MatchDWordAtOffset
//                CContentAnalyzer::FindAppFromExt
//                CContentAnalyzer::CheckTextHeaders
//                CContentAnalyzer::CheckBinaryHeaders
//
//              public:
//                ContentAnalyzer::FindMimeFromData
//                ::FindMimeFromData
//
//
//              05-26-96   AdriaanC (Adriaan Canter) Created
//              07-16-96   AdriaanC (Adriaan Canter) Modified
//              08-07-96   AdriaanC (Adriaan Canter) Modified
//              08-14-96   AdriaanC (Adriaan Canter) Modified
//
//----------------------------------------------------------------------------



class CContentAnalyzer
{
private:

    int _cbSample;
    int _cbText;
    int _cbNL;
    int _cbCR;
    int _cbFF;
    int _cbCtrl;
    int _cbHigh;

    char* _pBuf;
    WCHAR* _wzEncoding;
    WCHAR* _wzMimeType;
    DWORD _grfFlags;
    BOOL _fBinary;
    BOOL _fFoundHTML;               // Patterns requiring searching through buffer
    BOOL _fFoundXBitMap;            //
    BOOL _fFoundMacBinhex;          //
    BOOL _fFoundCDF;
    BOOL _fFoundTextScriptlet;
    BOOL _fFoundXML;
    WCHAR _wzMimeTypeFromExt[SZMIMESIZE_MAX];

private:
    inline void SampleData();
    inline BOOL IsBMP();
    inline DWORD GetDataFormat(LPCWSTR);
    inline BOOL FormatAgreesWithData(DWORD);
    inline BOOL MatchDWordAtOffset(DWORD, int);
    inline BOOL FindAppFromExt(LPSTR, LPSTR, DWORD);
    inline BOOL CheckTextHeaders();
    inline BOOL CheckBinaryHeaders();
public:
    CContentAnalyzer() : _cbSample(0), _cbText(0), _cbNL(0), _cbCR(0),
        _cbFF(0), _cbCtrl(0), _cbHigh(0), _pBuf(0), _grfFlags(0), 
        _fFoundHTML(FALSE), _fFoundXBitMap(FALSE), _fFoundMacBinhex(FALSE), 
        _fBinary(FALSE), _fFoundCDF(FALSE), _fFoundTextScriptlet(FALSE),
        _fFoundXML(FALSE) {}

    
    LPCWSTR FindMimeFromData(LPCWSTR, char*, int, LPCWSTR, DWORD);
    BOOL    FindMimeFromExt(
                        LPCWSTR wzFileName, 
                        CHAR *szFileName,
                        CHAR *szMimeTypeFromExt,
                        DWORD *pdwExtMimeTypeDataFormat,
                        CHAR  **ppszFileExt);
   


};

    LPCWSTR FindMimeFromData(LPCWSTR pwzFilename, LPVOID pBuffer,
        DWORD cbBufferSize, LPCWSTR pwzMimeProposed, DWORD grfFlags);

// private flags for bad header 

#endif // _DATASNIF_HXX





