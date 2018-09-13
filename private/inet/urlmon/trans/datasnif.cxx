//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       datasnif.cxx
//
//  Contents:   Stream Mime type checking (attempts to guess the MIME type
//              of a buffer by simple pattern matching).
//
//  Classes:    CContentAnalyzer
//
//  Functions:  private:
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
//                CContentAnalyzer::FindMimeFromData
//                ::FindMimeFromData
//
//
//  History:    05-25-96   AdriaanC (Adriaan Canter) Created
//              07-16-96   AdriaanC (Adriaan Canter) Modified
//              08-06-96   AdriaanC (Adriaan Canter) Modified
//              08-14-96   AdriaanC (Adriaan Canter) Modified
//
//----------------------------------------------------------------------------

#include <trans.h>
#include "datasnif.hxx"
#include <shlwapip.h>
#ifdef UNIX
#include <mainwin.h>
#endif

PerfDbgTag(tagDataSniff, "Urlmon", "Log DataSniff", DEB_DATA);

// Max no. bytes to look at
#define SAMPLE_SIZE 256

// Registry Key for app/fileext associations
#define szApplicationRegistryKey "\\Shell\\Open\\Command"
#define szApplicationRegistryKey2 "\\Shell\\Connect To\\Command"
#define szMimeRegistryKey        "MIME\\Database\\Content Type\\"

// Magic header words
#define AU_SUN_MAGIC                    0x2e736e64
#define AU_SUN_INV_MAGIC                0x646e732e
#define AU_DEC_MAGIC                    0x2e736400
#define AU_DEC_INV_MAGIC                0x0064732e
#define AIFF_MAGIC                      0x464f524d
#define AIFF_INV_MAGIC                  0x4d524f46
#define AIFF_MAGIC_MORE_1               'AIFF'
#define AIFF_MAGIC_MORE_2               'AIFC'
#define RIFF_MAGIC                      0x52494646
#define AVI_MAGIC                       0x41564920
#define WAV_MAGIC                       0x57415645
#define JAVA_MAGIC                      0xcafebabe
#define MPEG_MAGIC                      0x000001b3
#define MPEG_MAGIC_2                    0x000001ba
#define EMF_MAGIC_1                     0x01000000
#define EMF_MAGIC_2                     0x20454d46
#define WMF_MAGIC                       0xd7cdc69a
#define JPEG_MAGIC_1                    0xFF
#define JPEG_MAGIC_2                    0xD8

// Magic header text
CHAR vszRichTextMagic[] =                "{\\rtf";
CHAR vszPostscriptMagic[] =              "%!";
CHAR vszBinHexMagic[] =                  "onverted with BinHex";
CHAR vszBase64Magic[] =                  "begin";
CHAR vszGif87Magic[] =                   "GIF87";
CHAR vszGif89Magic[] =                   "GIF89";
CHAR vszTiffMagic[] =                    "MM";
CHAR vszBmpMagic[] =                     "BM";
CHAR vszZipMagic[] =                     "PK";
CHAR vszExeMagic[] =                     "MZ";
CHAR vszPngMagic[] =                     "\211PNG\r\n\032\n";
CHAR vszCompressMagic[] =                "\037\235";
CHAR vszGzipMagic[] =                    "\037\213";
CHAR vszXbmMagic1[] =                    "define";
CHAR vszXbmMagic2[] =                    "width";
CHAR vszXbmMagic3[] =                    "bits";
CHAR vszPdfMagic[] =                     "%PDF";
CHAR vszJGMagic[] =                      "JG";
CHAR vszMIDMagic[] =                     "MThd";

// null MIME type
WCHAR vwzNULL[] =                        L"(null)";

// 7 bit MIME Types
WCHAR vwzTextPlain[] =                   L"text/plain";
WCHAR vwzTextRichText[] =                L"text/richtext";
WCHAR vwzImageXBitmap[] =                L"image/x-xbitmap";
WCHAR vwzApplicationPostscript[] =       L"application/postscript";
WCHAR vwzApplicationBase64[] =           L"application/base64";
WCHAR vwzApplicationMacBinhex[] =        L"application/macbinhex40";
WCHAR vwzApplicationPdf[] =              L"application/pdf";
WCHAR vwzApplicationCDF[] =              L"application/x-cdf";
WCHAR vwzApplicationNETCDF[] =           L"application/x-netcdf";
WCHAR vwzmultipartmixedreplace[] =       L"multipart/x-mixed-replace";
WCHAR vwzmultipartmixed[] =              L"multipart/mixed";
WCHAR vwzTextScriptlet[] =               L"text/scriptlet";
WCHAR vwzTextComponent[] =               L"text/x-component";
WCHAR vwzTextXML[] =                     L"text/xml";
WCHAR vwzApplicationHTA[] =              L"application/hta";

// 8 bit MIME types
WCHAR vwzAudioAiff[] =                   L"audio/x-aiff";
WCHAR vwzAudioBasic[] =                  L"audio/basic";
WCHAR vwzAudioWav[] =                    L"audio/wav";
WCHAR vwzAudioMID[] =                    L"audio/mid";
WCHAR vwzImageGif[] =                    L"image/gif";
WCHAR vwzImagePJpeg[] =                  L"image/pjpeg";
WCHAR vwzImageJpeg[] =                   L"image/jpeg";
WCHAR vwzImageTiff[] =                   L"image/tiff";
WCHAR vwzImagePng[] =                    L"image/x-png";
WCHAR vwzImageBmp[] =                    L"image/bmp";
WCHAR vwzImageJG[] =                     L"image/x-jg";
WCHAR vwzImageEmf[] =                    L"image/x-emf";
WCHAR vwzImageWmf[] =                    L"image/x-wmf";
WCHAR vwzVideoAvi[] =                    L"video/avi";
WCHAR vwzVideoMpeg[] =                   L"video/mpeg";
WCHAR vwzApplicationCompressed[] =       L"application/x-compressed";
WCHAR vwzApplicationZipCompressed[] =    L"application/x-zip-compressed";
WCHAR vwzApplicationGzipCompressed[] =   L"application/x-gzip-compressed";
WCHAR vwzApplicationJava[] =             L"application/java";
WCHAR vwzApplicationMSDownload[] =       L"application/x-msdownload";

// 7 or 8 bit MIME types
WCHAR vwzTextHTML[] =                    L"text/html";
WCHAR vwzApplicationOctetStream[] =      L"application/octet-stream";




//+---------------------------------------------------------------------------
//
//  Method:     CContentAnalyzer::SampleData
//
//  Synopsis:
//
//  Arguments:  (void)
//
//  Returns:    (void)
//
//  History:    5-25-96   AdriaanC (Adriaan Canter) Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CContentAnalyzer::SampleData()
{
    BOOL fFoundFirstXBitMapTag = FALSE;
    BOOL fFoundSecondXBitMapTag = FALSE;
    BOOL fFoundAsciiChar = FALSE;

    int nHTMLConfidence = 0;

    unsigned char *p = (unsigned char*) _pBuf;

    _cbNL = _cbCR = _cbFF = _cbText = _cbCtrl = _cbHigh = 0;

    // Count incidence of character types.
    for (int i = 0; i < _cbSample - 1; i++)
    {
        fFoundAsciiChar = FALSE;

        if (*p == '\n')           // new line
        {
            _cbNL++;
        }
        else if (*p == '\r')      // carriage return
        {
            _cbCR++;
        }
        else if (*p == '\f')      // form feed
        {
            _cbFF++;
        }
        else if (*p == '\t')      // tab
        {
            _cbText++;
        }
        else if (*p < 32)         // control character
        {
            _cbCtrl++;
        }
        else if (*p >= 32 && *p < 128)        // regular text
        {
            _cbText++;
            fFoundAsciiChar = TRUE;
        }
        else                      // extended text
        {
            _cbHigh++;
        }

        if (fFoundAsciiChar)
        {
            // check for html
            if (*p == '<')
            {
                if (!StrCmpNIC((char*) p+1, "?XML", sizeof("?XML") - 1) &&
                    (
                        (*(p+5) == ':') || 
                        (*(p+5) == ' ') || 
                        (*(p+5) == '\t')) )
                {
                    _fFoundXML = TRUE;
                    // don't break : for CDF
                }
                

                if (!StrCmpNIC((char*) p+1, "SCRIPTLET", sizeof("SCRIPTLET") - 1))
                {
                    _fFoundTextScriptlet = TRUE;
                    break;
                }

                if (!StrCmpNIC((char*) p+1, "HTML", sizeof("HTML") - 1)
                    || !StrCmpNIC((char*) p+1, "HEAD", sizeof("HEAD") - 1)
                    || !StrCmpNIC((char*) p+1, "TITLE", sizeof("TITLE") - 1)
                    || !StrCmpNIC((char*) p+1, "BODY", sizeof("BODY") - 1)
                    || !StrCmpNIC((char*) p+1, "SCRIPT", sizeof("SCRIPT") - 1)
                    || !StrCmpNIC((char*) p+1, "A HREF", sizeof("A HREF") - 1)
                    || !StrCmpNIC((char*) p+1, "PRE", sizeof("PRE") - 1)
                    || !StrCmpNIC((char*) p+1, "IMG", sizeof("IMG") - 1)
                    || !StrCmpNIC((char*) p+1, "PLAINTEXT", sizeof("PLAINTEXT") - 1)
                    || !StrCmpNIC((char*) p+1, "TABLE", sizeof("TABLE") - 1))
                {
                    _fFoundHTML = TRUE;
                    break;
                }
                else if (   !StrCmpNIC((char*) p+1, "HR", sizeof("HR") - 1)
                         || !StrCmpNIC((char*) p+1, "A", sizeof("A") - 1)
                         || !StrCmpNIC((char*) p+1, "/A", sizeof("/A") - 1)
                         || !StrCmpNIC((char*) p+1, "B", sizeof("B") - 1)
                         || !StrCmpNIC((char*) p+1, "/B", sizeof("/B") - 1)
                         || !StrCmpNIC((char*) p+1, "P", sizeof("P") - 1)
                         || !StrCmpNIC((char*) p+1, "/P", sizeof("/P") - 1)
                         || !StrCmpNIC((char*) p+1, "!--", sizeof("!--") - 1)
                        )
                {
                    //
                    // In order for this branch to identify this is HTML 
                    // We have to make sure:
                    //      1. some HTML control char exists
                    //      2. We've scanned the whole data block
                    //      3. 2/3 of the data should be text
                    //
                     
                    nHTMLConfidence += 50;
                    if (    nHTMLConfidence >= 100
                        &&  i == _cbSample - 1 
                        &&  _cbText >= ((_cbSample * 2) / 3)
                       )
                    {
                        _fFoundHTML = TRUE;
                        break;
                    }
                }
                if (!StrCmpNIC((char*) p+1, "CHANNEL", sizeof("CHANNEL") - 1))
                {
                    _fFoundCDF = TRUE;
                    break;
                }
        
            
            }
            else if (!StrCmpNIC((char*) p, "-->", sizeof("-->") - 1))
            {
                // comment begin
                // I really want to make sure that most of the 
                // char are printable 
                // potential issue: International code page?
                nHTMLConfidence += 50;
                if (   (nHTMLConfidence >= 100) 
                    && (i == _cbSample - 1 )
                    && (_cbText > (_cbSample * 2 /3) )
                   )
                {
                    _fFoundHTML = TRUE;
                    break;
                }
            }
            // check for xbitmap
            else if (*p == '#')
            {
                if (!StrCmpNC((char*) p+1, vszXbmMagic1, sizeof(vszXbmMagic1) - 1))
                    fFoundFirstXBitMapTag = TRUE;
            }
            else if (*p == '_' && fFoundSecondXBitMapTag)
            {
                if (!StrCmpNC((char*) p+1, vszXbmMagic3, sizeof(vszXbmMagic3) - 1))
                {
                    _fFoundXBitMap = TRUE;
                    break;
                }
            }
            else if (*p == '_' && fFoundFirstXBitMapTag)
            {
                if (!StrCmpNC((char*) p+1, vszXbmMagic2, sizeof(vszXbmMagic2) - 1))
                    fFoundSecondXBitMapTag = TRUE;
            }

            // MacBinhex
            else if (*p == 'c')
            {
                if (!StrCmpNC((char*) p+1, vszBinHexMagic, sizeof(vszBinHexMagic) - 1))
                {
                    _fFoundMacBinhex = TRUE;
                    break;
                }
            }

        }
        p++;
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CContentAnalyzer::IsBMP
//
//  Synopsis:
//
//  Arguments:  (void)
//
//  Returns:    BOOL
//
//  History:    5-25-96   AdriaanC (Adriaan Canter) Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CContentAnalyzer::IsBMP()
{
    BOOL bRetVal = TRUE;

    BITMAPFILEHEADER UNALIGNED *pBMFileHdr;

    if (_cbSample < 2)
    {
        bRetVal = FALSE;
    }

    // Check header
    if (StrCmpNC(_pBuf, vszBmpMagic, sizeof(vszBmpMagic) - 1))
    {
        bRetVal = FALSE;
    }

    // Sample size needs to be big enough.
    if (_cbSample < sizeof(BITMAPFILEHEADER))
    {
        bRetVal = FALSE;
    }

    pBMFileHdr = (BITMAPFILEHEADER*)(_pBuf);

#ifdef UNIX

    /* Use 14 on Unix, because we want the size without the padding
     * done on Unix. sizeof(BITMAPFILEHEADER) = 16 on Unix with padding
     */
    #define UNIX_BITMAP_HEADER_SIZE 14
    BITMAPFILEHEADER bmFileHeader;

    if(MwReadBITMAPFILEHEADER((LPBYTE)_pBuf, UNIX_BITMAP_HEADER_SIZE, &bmFileHeader))
        pBMFileHdr = &bmFileHeader;

#endif /* UNIX */

    // The reserved fields must be set to 0
    if (pBMFileHdr->bfReserved1!=0 || pBMFileHdr->bfReserved2!=0)
    {
        bRetVal = FALSE;
    }

    return bRetVal;
}

//+---------------------------------------------------------------------------
//
//  Method:     CContentAnalyzer::GetDataFormat
//
//  Synopsis:
//
//  Arguments:  (WCHAR* wzMimeType)
//
//  Returns:    BOOL dwDataFormat
//
//  History:    7-21-96   AdriaanC (Adriaan Canter) Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD CContentAnalyzer::GetDataFormat(LPCWSTR wzMimeType)
{
    CLIPFORMAT cfFormat;
    DATAFORMAT dwDataFormat;
    HRESULT hr;

    if (!wzMimeType)
    {
        return DATAFORMAT_AMBIGUOUS;
    }

    if( !_wcsicmp(wzMimeType, vwzNULL) )
    {
        return DATAFORMAT_AMBIGUOUS;
    }


    hr = FindMediaTypeFormat(wzMimeType, &cfFormat, (DWORD *)&dwDataFormat);

    if (hr == S_OK)
    {
        return dwDataFormat;
    }
    else
    {
        return DATAFORMAT_UNKNOWN;
    }

}

//+---------------------------------------------------------------------------
//
//  Method:     CContentAnalyzer::FormatAgreesWithData
//
//  Synopsis:
//
//  Arguments:  (void)
//
//  Returns:    BOOL
//
//  History:    8-14-96   AdriaanC (Adriaan Canter) Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CContentAnalyzer::FormatAgreesWithData(DWORD dwFormat)
{
    if (dwFormat == DATAFORMAT_TEXT && _fBinary == FALSE
        || dwFormat == DATAFORMAT_BINARY && _fBinary == TRUE
        || dwFormat == DATAFORMAT_TEXTORBINARY)
    {
        return TRUE;
    }
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Method:     CContentAnalyzer::MatchDWordAtOffset
//
//  Synopsis:   Determines if a given magic word is found at
//              the specified offset.
//
//  Arguments:  (DWORD magic, int offset)
//
//  Returns:    BOOL
//
//  History:    5-25-96   AdriaanC (Adriaan Canter) Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CContentAnalyzer::MatchDWordAtOffset(DWORD magic, int offset)
{
    BOOL bRetVal = TRUE;

    DWORD dwWord = 0;

    unsigned char* p = (unsigned char*) _pBuf;

    if (_cbSample < offset + (int) sizeof(DWORD))
    {
        return FALSE;
    }

    dwWord = (p[offset] << 24)
        | (p[offset+1] << 16)
        | (p[offset+2] << 8)
        |  p[offset+3];


    if (magic != dwWord)
    {
        bRetVal = FALSE;
    }

    return bRetVal;
}


//+---------------------------------------------------------------------------
//
//  Method:     CContentAnalyzer::FindAppFromExt
//
//  Synopsis:   Determines an associated application from
//              a given file extension
//
//  Arguments:  (LPSTR pszExt, LPSTR pszCommand (command line))
//
//  Returns:    BOOL (Associated Application is found or not)
//
//  History:    7-15-96   AdriaanC (Adriaan Canter) Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CContentAnalyzer::FindAppFromExt(LPSTR pszExt, LPSTR pszCommand, DWORD cbCommand)
{
    DWORD cbLen, dwType;
    CHAR szRegPath[MAX_PATH];
    BOOL fReturn = FALSE;
    HKEY hMimeKey = NULL;

    // BUGBUG - Is there a max registry path length?
    cbLen = MAX_PATH;

    // Should be a file extension
    TransAssert((pszExt[0] == '.'));

    // Open key on extension
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, pszExt, 0,
        KEY_QUERY_VALUE, &hMimeKey) == ERROR_SUCCESS)
    {
        // Find file type (txtfile, htmlfile, etc) .
        // These currently utilize a null key.
        if (RegQueryValueEx(hMimeKey, NULL, NULL, &dwType,
            (LPBYTE)szRegPath, &cbLen) == ERROR_SUCCESS)
        {
            strncat(szRegPath, szApplicationRegistryKey, MAX_PATH - strlen(szRegPath) - 1);

            HKEY hAppKey = NULL;
            cbLen = cbCommand;

            // szRegPath should now look similar to
            // "txtfile\Shell\Open\Command". Open key on szRegPath
            if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szRegPath, 0,
                KEY_QUERY_VALUE, &hAppKey) == ERROR_SUCCESS) 
            {
                // Find the application command line - again, null key.
                if (RegQueryValueEx(hMimeKey, NULL, NULL, &dwType,
                    (LPBYTE)pszCommand, &cbLen) == ERROR_SUCCESS)
                {
                    // Success
                    fReturn = TRUE;
                }
                RegCloseKey(hAppKey);
            }

            else 
            {   
                // check "Shell\\Connect To\command" key - used by SmartTerm 

                // dynamic allocate szRegPath2 so that it won't take
                // unnecessary stack space - after all, this is not a 
                // common case
                CHAR* szRegPath2 = NULL;
                HKEY hAppKey2 = NULL;

                szRegPath2 = new CHAR[MAX_PATH];
                if( szRegPath2 )
                {
                    if (RegQueryValueEx(hMimeKey, NULL, NULL, &dwType,
                        (LPBYTE)szRegPath2, &cbLen) == ERROR_SUCCESS)
                    {
                        strncat(szRegPath2, szApplicationRegistryKey2, 
                            MAX_PATH - strlen(szRegPath2) - 1);
                    }
                    else
                    {
                        // this should not happen at all 
                        delete [] szRegPath2;
                        szRegPath2 = NULL;
                    }
                }

                if (szRegPath2 && 
                    RegOpenKeyEx(HKEY_CLASSES_ROOT, szRegPath2, 0, KEY_QUERY_VALUE, &hAppKey2) == ERROR_SUCCESS) 
                {
                    if (RegQueryValueEx(hMimeKey, NULL, NULL, &dwType,
                        (LPBYTE)pszCommand, &cbLen) == ERROR_SUCCESS)
                    {
                        // Success
                        fReturn = TRUE;
                    }
                    RegCloseKey(hAppKey2);

                }

                delete [] szRegPath2;
            }

        }
        RegCloseKey(hMimeKey);
    }

    return fReturn;
}
//+---------------------------------------------------------------------------
//
//  Method:     CContentAnalyzer::CheckTextHeaders
//
//  Synopsis:
//
//
//  Arguments:  void
//
//  Returns:    void
//
//  History:    7-23-96   AdriaanC (Adriaan Canter) Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CContentAnalyzer::CheckTextHeaders()
{
    BOOL bRet = TRUE;
    // application/pdf (Acrobat)
    if (!StrCmpNC(_pBuf, vszPdfMagic, sizeof(vszPdfMagic) - 1))
    {
        _wzMimeType = vwzApplicationPdf;
    }

    // application/Postscript
    else if (!StrCmpNC(_pBuf, vszPostscriptMagic, sizeof(vszPostscriptMagic) - 1))
    {
        _wzMimeType = vwzApplicationPostscript;
    }

    // text/richtext
    else if (!StrCmpNC(_pBuf, vszRichTextMagic, sizeof(vszRichTextMagic) - 1))
    {
        _wzMimeType = vwzTextRichText;
    }

    // application/base64
    else if (!StrCmpNC(_pBuf, vszBase64Magic, sizeof(vszBase64Magic) - 1))
    {
        _wzMimeType = vwzApplicationBase64;
    }

    // No matches - assume plain text.
    else
    {
        //_wzMimeType = vwzTextPlain;
        bRet = FALSE;
    }

    return bRet;

}

//+---------------------------------------------------------------------------
//
//  Method:     CContentAnalyzer::CheckBinaryHeaders
//
//  Synopsis:
//
//
//  Arguments:  void
//
//  Returns:    void
//
//  History:    7-23-96   AdriaanC (Adriaan Canter) Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CContentAnalyzer::CheckBinaryHeaders()
{
    BOOL bRet = TRUE;
    // image/gif
    if (!StrCmpNIC(_pBuf, vszGif87Magic, sizeof(vszGif87Magic) - 1)
        || !StrCmpNIC(_pBuf, vszGif89Magic, sizeof(vszGif89Magic) - 1))
    {
        _wzMimeType = vwzImageGif;
    }

    // image/pjpeg
    else if ((BYTE)_pBuf[0] == JPEG_MAGIC_1 && (BYTE)_pBuf[1] == JPEG_MAGIC_2)
    {
        _wzMimeType = vwzImagePJpeg;
    }

    // img/bmp
    else if (IsBMP())
    {
        _wzMimeType = vwzImageBmp;
    }

    // audio/wav
    else if (MatchDWordAtOffset(RIFF_MAGIC, 0)
        && MatchDWordAtOffset(WAV_MAGIC, 8))
    {
        _wzMimeType = vwzAudioWav;
    }

    // audio/basic (.au files)
    else if (MatchDWordAtOffset(AU_DEC_MAGIC, 0)
           || MatchDWordAtOffset(AU_SUN_MAGIC, 0)
           || MatchDWordAtOffset(AU_DEC_INV_MAGIC, 0)
           || MatchDWordAtOffset(AU_SUN_INV_MAGIC, 0))
    {
       _wzMimeType = vwzAudioBasic;
    }

    // image/tiff
    else if (!StrCmpC(_pBuf, vszTiffMagic)) // "MM" followed by a \0
    {
        _wzMimeType = vwzImageTiff;
    }

    // application/x-msdownload
    else if (!StrCmpNC(_pBuf, vszExeMagic, sizeof(vszExeMagic) - 1))
    {
        _wzMimeType = vwzApplicationMSDownload;
    }

    // image/x-png
    else if (!StrCmpNC(_pBuf, vszPngMagic, sizeof(vszPngMagic) - 1))
    {
        _wzMimeType = vwzImagePng;
    }

    // image/x-jg
    else if (!StrCmpNC(_pBuf, vszJGMagic, sizeof(vszJGMagic) - 1)
        && (int) _pBuf[2] >= 3
        && (int) _pBuf[2] <= 31
        && _pBuf[4] == 0)
    {
        _wzMimeType = vwzImageJG;
    }

    // audio/x-aiff
    else if (MatchDWordAtOffset(AIFF_INV_MAGIC, 0))
    {
       _wzMimeType = vwzAudioAiff;
    }

    else if (MatchDWordAtOffset(AIFF_MAGIC, 0) &&
             ( MatchDWordAtOffset(AIFF_MAGIC_MORE_1, 8) ||
               MatchDWordAtOffset(AIFF_MAGIC_MORE_2, 8) ) )
    {
        //
        // according to DaveMay, the correct AIFF format would be:
        // 'FORM....AIFF' or 'FORM....AIFC'
        // Only check for 'FORM' is incorrect because .sc2 has the 
        // same sig
        //
       _wzMimeType = vwzAudioAiff;
    }

    // video/avi (or video/x-msvedio)
    else if (MatchDWordAtOffset(RIFF_MAGIC, 0)
        && MatchDWordAtOffset(AVI_MAGIC, 8))
    {
        _wzMimeType = vwzVideoAvi;
    }

    // video/mpeg
    else if (MatchDWordAtOffset(MPEG_MAGIC, 0)
            || MatchDWordAtOffset(MPEG_MAGIC_2, 0) )
    {
        _wzMimeType = vwzVideoMpeg;
    }

    // image/x-emf
    else if (MatchDWordAtOffset(EMF_MAGIC_1, 0)
        && MatchDWordAtOffset(EMF_MAGIC_2, 40))
    {
        _wzMimeType = vwzImageEmf;
    }

    // image/x-wmf
    else if (MatchDWordAtOffset(WMF_MAGIC, 0))
    {
        _wzMimeType = vwzImageWmf;
    }

    // application/java
    else if (MatchDWordAtOffset(JAVA_MAGIC, 0))
    {
        _wzMimeType = vwzApplicationJava;
    }

    // application/x-zip-compressed
    else if (!StrCmpNC(_pBuf, vszZipMagic, sizeof(vszZipMagic) - 1))
    {
        _wzMimeType = vwzApplicationZipCompressed;
    }

    // application/x-compress
    else if (!StrCmpNC(_pBuf, vszCompressMagic, sizeof(vszCompressMagic) - 1))
    {
        _wzMimeType = vwzApplicationCompressed;
    }

    // application/x-gzip
    else if (!StrCmpNC(_pBuf, vszGzipMagic, sizeof(vszGzipMagic) - 1))
    {
        _wzMimeType = vwzApplicationGzipCompressed;
    }

    // application/x-zip-compressed
    else if (!StrCmpNC(_pBuf, vszZipMagic, sizeof(vszZipMagic) - 1))
    {
        _wzMimeType = vwzApplicationZipCompressed;
    }

    // audio/mid
    else if (!StrCmpC(_pBuf, vszMIDMagic))
    {
        _wzMimeType = vwzAudioMID;
    }

    // application/pdf (Acrobat)
    else if (!StrCmpNC(_pBuf, vszPdfMagic, sizeof(vszPdfMagic) - 1))
    {
        _wzMimeType = vwzApplicationPdf;
    }

    // don't know what it is.
    else
    {
        //_wzMimeType = vwzApplicationOctetStream;
        bRet = FALSE;
    }

    return bRet;
}



//+---------------------------------------------------------------------------
//
//  Method:     CContentAnalyzer::FindMimeFromData
//
//  Synopsis:   Attempts to guess MIME type from buffer
//
//
//  Arguments:  pBuf, cbSample, wzSuggestedMimeType
//
//  Returns:    LPCWSTR (the MIME type guessed)
//
//  History:    5-25-96   AdriaanC (Adriaan Canter) Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPCWSTR CContentAnalyzer::FindMimeFromData(LPCWSTR wzFileName, char* pBuf,
    int cbSample, LPCWSTR wzSuggestedMimeType, DWORD grfFlags)
{
    BOOL fSampledData = FALSE;
    BOOL fFoundMimeTypeFromExt = FALSE;

    CHAR* szFileExt = 0;
    CHAR szFileName[MAX_PATH];
    CHAR szMimeTypeFromExt[SZMIMESIZE_MAX];
    CHAR szCommand[MAX_PATH];
    CHAR cLastByte;

    DWORD dwMimeLen = SZMIMESIZE_MAX;
    DWORD dwExtMimeTypeDataFormat;
    DWORD dwSuggestedMimeTypeDataFormat;
    DWORD dwMimeTypeDataFormat;
    DWORD cbCommand = MAX_PATH;
    BOOL  fExtensionChecked = FALSE;

    _grfFlags = grfFlags;

    // BUGBUG - we can use this information for DBCS.
    // Remove any info appended to the suggested mime type
    // such as charset information. This is identified by ';'

    if (wzSuggestedMimeType)
    {
        WCHAR* wptr = wcsstr(wzSuggestedMimeType, L";");
        if (wptr)
        {
            *wptr = L'\0';
        }
    }

    // Check to see if the server is suggesting an unknown mime type
    dwSuggestedMimeTypeDataFormat = GetDataFormat(wzSuggestedMimeType);
    if (dwSuggestedMimeTypeDataFormat == DATAFORMAT_UNKNOWN)
    {
        // server push returns "multipart" content type 
        // this is not the real mimetype, so we have to sniff 
        // to find out the truth 
        if(    wcsicmp(wzSuggestedMimeType, vwzmultipartmixed)
            && wcsicmp(wzSuggestedMimeType, vwzmultipartmixedreplace) )
        {
            // If so, return the suggested mime type.
            _wzMimeType = (WCHAR*) wzSuggestedMimeType;
            return _wzMimeType;
        }
    }

/*****
    // check if we got an extension and extension mime
    // matches the suggested mime - only for text/plain
    if (    wzSuggestedMimeType 
        &&  wzFileName
        && !wcscmp(wzSuggestedMimeType,vwzTextPlain))
         
    {
        fExtensionChecked = TRUE;
        fFoundMimeTypeFromExt = FindMimeFromExt(
                                        wzFileName,
                                        szFileName,
                                        szMimeTypeFromExt,
                                        &dwExtMimeTypeDataFormat,
                                        &szFileExt
                                        );

        // If there is a mime type associated with the file
        // extension then return it.
        if (   fFoundMimeTypeFromExt
            && (dwExtMimeTypeDataFormat == dwSuggestedMimeTypeDataFormat)
            && !wcscmp(wzSuggestedMimeType,_wzMimeTypeFromExt)
            )
        {
            // If so, return the suggested mime type.
            _wzMimeType = (WCHAR*) wzSuggestedMimeType;
            return _wzMimeType;
        }
    }
*****/

    // Not enough data to tell anything
    if (!pBuf || cbSample <= 0)
    {
        _wzMimeType = (WCHAR*) wzSuggestedMimeType;
        return _wzMimeType;
    }

    _pBuf = pBuf;
    _cbSample = (cbSample <= SAMPLE_SIZE) ? cbSample : SAMPLE_SIZE;

    // Save off last character. Null terminate the buffer.
    cLastByte = _pBuf[_cbSample - 1];
    _pBuf[_cbSample - 1] = '\0';


    // Common cases first - check the server indicated mime type
    // for text/html, image/gif or image/[p]jpeg.
    if (   wzSuggestedMimeType
        && !StrCmpICW(wzSuggestedMimeType, vwzTextHTML))
    {
        // Sample the data. This routine also checks for the following
        // mime types which require extended scanning through the buffer:
        // text/html, image/x-xbitmap, application/macbinhex
        SampleData();
        fSampledData = TRUE;

        if (_fFoundHTML)
        {
            _wzMimeType = vwzTextHTML;
           goto exit;
        }
    }

    // image/gif
    else if (wzSuggestedMimeType
        && !wcsicmp(wzSuggestedMimeType, vwzImageGif))
    {
        if (!StrCmpNIC(_pBuf, vszGif87Magic, sizeof(vszGif87Magic) - 1)
           || !StrCmpNIC(_pBuf, vszGif89Magic, sizeof(vszGif89Magic) - 1))
        {
            _wzMimeType = vwzImageGif;
            goto exit;
        }
    }

    // image/jpeg or image/pjpeg
    else if (wzSuggestedMimeType
        && (!wcsicmp(wzSuggestedMimeType, vwzImagePJpeg)
        || !wcsicmp(wzSuggestedMimeType, vwzImageJpeg)))
    {
        if ((BYTE)_pBuf[0] == JPEG_MAGIC_1 && (BYTE)_pBuf[1] == JPEG_MAGIC_2)
        {
            _wzMimeType = vwzImagePJpeg;
            goto exit;
        }
    }


    //
    // ********************** BEGIN HACK ******************************* 
    //
    // we will remove this once tridents defined the unique signature
    // for .hta and .htc format
    //
    // DanpoZ (98.08.12) - refer to IE5 SUPERHOT bug 35478
    //
    if (wzFileName )
    {

        CHAR* szExt;
        CHAR szFile[MAX_PATH];
        W2A(wzFileName, szFile, MAX_PATH);

        if( grfFlags & FMFD_URLASFILENAME )
        {
            //
            // remove teh security context '\1' and replace it with '\0'
            // but only do this when we are using URL to replace the filename
            //
            CHAR* pch = StrChr(szFile, '\1');
            if (pch)
            {
                *pch = '\0';
            }
        }

        szExt = FindFileExtension(szFile);
        if( szExt && 
            ( !StrCmpNIC(szExt, ".hta", sizeof(".hta") - 1) ||
              !StrCmpNIC(szExt, ".htc", sizeof(".htc") - 1)  ) )
        {
            fExtensionChecked = TRUE;
            fFoundMimeTypeFromExt = FindMimeFromExt(
                                            wzFileName,
                                            szFileName,
                                            szMimeTypeFromExt,
                                            &dwExtMimeTypeDataFormat,
                                            &szFileExt
                                            );

            // If there is a mime type associated with the file
            // extension then return it.
            if (fFoundMimeTypeFromExt)
            {
                _wzMimeType = _wzMimeTypeFromExt;
                goto exit;
            }
        }
    }
    //
    // ********************** END HACK ********************************* 
    //

    // One of the following is true:

    // 1) The server indicated a common mime type (html, gif or jpeg),
    //    however, verification failed.
    // 2) The server indicated an ambiguous mime type or
    //    a known, but uncommon mime type.

    // If not done so already, sample the data.
    if (!fSampledData)
    {
        SampleData();
        fSampledData = TRUE;
    }

    // Return any mime type that was positively
    // identified during the data sampling
    if( _fFoundCDF )
    {
        _wzMimeType = vwzApplicationCDF;
        goto exit;
    }
    else if( _fFoundXML)
    {
        _wzMimeType = vwzTextXML;
        goto exit;
    }
    else if (_fFoundHTML)
    {
        _wzMimeType = vwzTextHTML;
        goto exit;
    }
    else if (_fFoundXBitMap)
    {
        _wzMimeType = vwzImageXBitmap;
        goto exit;
    }
    else if (_fFoundMacBinhex)
    {
        _wzMimeType = vwzApplicationMacBinhex;
        goto exit;
    }
    else if( _fFoundTextScriptlet )
    {
        _wzMimeType = vwzTextScriptlet;
        goto exit;
    }

    if(    !_fFoundCDF  
        && wzSuggestedMimeType
        && !wcsicmp(wzSuggestedMimeType, vwzApplicationNETCDF) 
      ) 
    {
        // only overwrite application/x-netcdf with aplication/x-cdf
        _wzMimeType = vwzApplicationNETCDF; 
        goto exit;
    }
    

    // Decide if buffer is primarily text or binary. Conduct
    // pattern matching to determine a mime type depending on the
    // finding.
    if (!_cbCtrl || _cbText + _cbFF >= 16 * (_cbCtrl + _cbHigh))
    {
        _fBinary = FALSE;
        if( !CheckTextHeaders() )
        {
            if( !CheckBinaryHeaders() )
            {
                _wzMimeType = vwzTextPlain;
            }
        }
    }
    else
    {
        _fBinary = TRUE;
        if( !CheckBinaryHeaders() )
        {
            if( !CheckTextHeaders() )
            {
                _wzMimeType = vwzApplicationOctetStream;
            }
        }
    }

    // Determine format of the mime type from data
    dwMimeTypeDataFormat = GetDataFormat(_wzMimeType);

    // If the format of the mime type found from examining the data
    // is not ambiguous, then return this mime type.
    if (dwMimeTypeDataFormat != DATAFORMAT_AMBIGUOUS)
    {
        goto exit;
    }

    // Examination of data is inconclusive.
    else
    {
        // If the suggested mime type is not ambiguous and does
        // not conflict with the data format then return it.
        if (dwSuggestedMimeTypeDataFormat != DATAFORMAT_AMBIGUOUS
            && FormatAgreesWithData(dwSuggestedMimeTypeDataFormat))
        {
            _wzMimeType = (WCHAR*) wzSuggestedMimeType;
            goto exit;
        }

        // Otherwise, attempt to obtain a mime type from any
        // file extension. If none is found, but an application
        // is registered for the file extension, return
        // application/octet-stream.


        // If there is a file extension, find any
        // associated mime type.
        if (wzFileName && !fExtensionChecked)
        {
            fExtensionChecked = TRUE;

            fFoundMimeTypeFromExt = FindMimeFromExt(
                                            wzFileName,
                                            szFileName,
                                            szMimeTypeFromExt,
                                            &dwExtMimeTypeDataFormat,
                                            &szFileExt
                                            );
        }

        // If there is a mime type associated with the file
        // extension then return it.
        if (fFoundMimeTypeFromExt)
        {
            if (dwExtMimeTypeDataFormat == DATAFORMAT_UNKNOWN)
            {
                _wzMimeType = _wzMimeTypeFromExt;
                goto exit;
            }
            else
            {
                goto exit;
            }
        }

        // Otherwise, check to see if there is an associated application.
        if (szFileExt && FindAppFromExt(szFileExt, szCommand, cbCommand))
        {
            // Found an associated application.
            _wzMimeType = vwzApplicationOctetStream;
            goto exit;
        }

        // No suggested mime type, no mime type from file extension
        // and no registered application found. Fall through and return
        // mime type found from the data
    }


    exit:
        // Replace the null termination with
        // the original character.
        _pBuf[_cbSample - 1] = cLastByte;

        return _wzMimeType;
}

//+---------------------------------------------------------------------------
//
//  Method:     CContentAnalyzer::FindMimeFromExt
//
//  Synopsis:
//
//  Arguments:  [wzFileName] --
//              [szFileName] --
//              [szMimeTypeFromExt] --
//              [pdwExtMimeTypeDataFormat] --
//
//  Returns:
//
//  History:    5-25-96   AdriaanC (Adriaan Canter)
//              1-28-1997   JohannP (Johann Posch)   made separate function
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CContentAnalyzer::FindMimeFromExt(
                        LPCWSTR wzFileName,
                        CHAR *szFileName,
                        CHAR *szMimeTypeFromExt,
                        DWORD *pdwExtMimeTypeDataFormat,
                        CHAR  **ppszFileExt)
{
    BOOL fFoundMimeTypeFromExt = FALSE;
    UrlMkAssert((wzFileName && szFileName && pdwExtMimeTypeDataFormat));
    DWORD dwMimeLen = SZMIMESIZE_MAX;
    CHAR* szFileExt = 0;

    // If there is a file extension, find any
    // associated mime type.
    W2A(wzFileName, szFileName, MAX_PATH);
    szFileExt = FindFileExtension(szFileName);
    if (szFileExt && GetMimeFromExt(szFileExt,
        szMimeTypeFromExt, &dwMimeLen) == ERROR_SUCCESS)
    {
        fFoundMimeTypeFromExt = TRUE;
        A2W(szMimeTypeFromExt, _wzMimeTypeFromExt, SZMIMESIZE_MAX);
        *pdwExtMimeTypeDataFormat = GetDataFormat(_wzMimeTypeFromExt);
    }
    if (szFileExt && ppszFileExt)
    {
        *ppszFileExt = szFileExt;
    }

    return fFoundMimeTypeFromExt;
}


