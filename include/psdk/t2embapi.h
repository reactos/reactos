#ifndef __T2EMBED_API_H
#define __T2EMBED_API_H

#ifndef CHARSET_UNICODE
#define CHARSET_UNICODE    1
#define CHARSET_DEFAULT    1
#define CHARSET_SYMBOL     2
#define CHARSET_GLYPHIDX   3
#endif

#ifndef EMBED_PREVIEWPRINT
#define EMBED_PREVIEWPRINT 1
#define EMBED_EDITABLE     2
#define EMBED_INSTALLABLE  3
#define EMBED_NOEMBEDDING  4
#endif

#ifndef LICENSE_INSTALLABLE
#define LICENSE_INSTALLABLE   0x0
#define LICENSE_DEFAULT       0x0
#define LICENSE_NOEMBEDDING   0x2
#define LICENSE_PREVIEWPRINT  0x4
#define LICENSE_EDITABLE      0x8
#endif

#ifndef TTEMBED_RAW
#define TTEMBED_RAW                       0x0
#define TTEMBED_SUBSET                    0x1
#define TTEMBED_TTCOMPRESSED              0x4
#define TTEMBED_FAILIFVARIATIONSIMULATED  0x10
#define TTEMBED_EMBEDEUDC                 0x20
#define TTEMBED_VALIDATIONTESTS           0x40
#define TTEMBED_WEBOBJECT                 0x80
#define TTEMBED_ENCRYPTDATA               0x10000000
#endif

#ifndef E_NONE
#define E_NONE 0x0
#endif

#ifndef E_CHARCODECOUNTINVALID
#define E_CHARCODECOUNTINVALID     0x2
#define E_CHARCODESETINVALID       0x3
#define E_DEVICETRUETYPEFONT       0x4
#define E_HDCINVALID               0x6
#define E_NOFREEMEMORY             0x7
#define E_FONTREFERENCEINVALID     0x8
#define E_NOTATRUETYPEFONT         0xA
#define E_ERRORACCESSINGFONTDATA   0xC
#define E_ERRORACCESSINGFACENAME   0xD
#define E_ERRORUNICODECONVERSION   0x11
#define E_ERRORCONVERTINGCHARS     0x12
#define E_EXCEPTION                0x13
#define E_RESERVEDPARAMNOTNULL     0x14
#define E_CHARSETINVALID           0x15
#define E_WIN32S_NOTSUPPORTED      0x16
#define E_FILE_NOT_FOUND           0x17
#define E_TTC_INDEX_OUT_OF_RANGE   0x18
#define E_INPUTPARAMINVALID        0x19
#endif

#ifndef E_ERRORCOMPRESSINGFONTDATA
#define E_ERRORCOMPRESSINGFONTDATA    0x100
#define E_FONTDATAINVALID             0x102
#define E_NAMECHANGEFAILED            0x103
#define E_FONTNOTEMBEDDABLE           0x104
#define E_PRIVSINVALID                0x105
#define E_SUBSETTINGFAILED            0x106
#define E_READFROMSTREAMFAILED        0x107
#define E_SAVETOSTREAMFAILED          0x108
#define E_NOOS2                       0x109
#define E_T2NOFREEMEMORY              0x10A
#define E_ERRORREADINGFONTDATA        0x10B
#define E_FLAGSINVALID                0x10C
#define E_ERRORCREATINGFONTFILE       0x10D
#define E_FONTALREADYEXISTS           0x10E
#define E_FONTNAMEALREADYEXISTS       0x10F
#define E_FONTINSTALLFAILED           0x110
#define E_ERRORDECOMPRESSINGFONTDATA  0x111
#define E_ERRORACCESSINGEXCLUDELIST   0x112
#define E_FACENAMEINVALID             0x113
#define E_STREAMINVALID               0x114
#define E_STATUSINVALID               0x115
#define E_PRIVSTATUSINVALID           0x116
#define E_PERMISSIONSINVALID          0x117
#define E_PBENABLEDINVALID            0x118
#define E_SUBSETTINGEXCEPTION         0x119
#define E_SUBSTRING_TEST_FAIL         0x11A
#define E_FONTVARIATIONSIMULATED      0x11B
#define E_FONTVALIDATEFAIL            0x11C
#define E_FONTFAMILYNAMENOTINFULL     0x11D
#endif

#ifndef E_ADDFONTFAILED
#define E_ADDFONTFAILED             0x200
#define E_COULDNTCREATETEMPFILE     0x201
#define E_FONTFILECREATEFAILED      0x203
#define E_WINDOWSAPI                0x204
#define E_FONTFILENOTFOUND          0x205
#define E_RESOURCEFILECREATEFAILED  0x206
#define E_ERROREXPANDINGFONTDATA    0x207
#define E_ERRORGETTINGDC            0x208
#define E_EXCEPTIONINDECOMPRESSION  0x209
#define E_EXCEPTIONINCOMPRESSION    0x20A
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long(WINAPIV *WRITEEMBEDPROC)
(
    void* lpvWriteStream,
    const void* lpvBuffer, 
    const unsigned long cbBuffer
);

typedef unsigned long(WINAPIV *READEMBEDPROC)
(
    void* lpvReadStream,
    void* lpvBuffer,
    const unsigned long cbBuffer
);


typedef struct
{ 
    unsigned long ulStructSize;
    long lTestFromSize;
    long lTestToSize;
    unsigned long ulCharSet;
    unsigned short usReserved1;
    unsigned short usCharCodeCount;
    unsigned short* pusCharCodeSet;
} TTVALIDATIONTESTPARAMS;

typedef struct
{ 
    unsigned long ulStructSize;
    long lTestFromSize;
    long lTestToSize;
    unsigned long ulCharSet;
    unsigned short usReserved1;
    unsigned short usCharCodeCount;
    unsigned long* pulCharCodeSet;
} TTVALIDATIONTESTPARAMSEX;

typedef struct
{ 
    unsigned short usStructSize;
    unsigned short usRootStrSize;
    unsigned short *pusRootStr;
} TTEMBEDINFO;

typedef struct
{ 
    unsigned short usStructSize;
    unsigned short usRefStrSize;
    unsigned short *pusRefStr;
} TTLOADINFO;

LONG
WINAPI
TTCharToUnicode(HDC hDC,
                UCHAR* pucCharCodes,
                ULONG ulCharCodeSize,
                USHORT* pusShortCodes,
                ULONG ulShortCodeSize,
                ULONG ulFlags);

LONG
WINAPI
TTDeleteEmbeddedFont(HANDLE hFontReference,
                     ULONG ulFlags,
                     ULONG* pulStatus);

LONG
WINAPI
TTEmbedFont(HDC hDC,
            ULONG ulFlags,
            ULONG ulCharSet,
            ULONG* pulPrivStatus,
            ULONG* pulStatus,
            WRITEEMBEDPROC lpfnWriteToStream,
            LPVOID lpvWriteStream,
            USHORT* pusCharCodeSet,
            USHORT usCharCodeCount,
            USHORT usLanguage,
            TTEMBEDINFO* pTTEmbedInfo);

LONG
WINAPI
TTEmbedFontFromFileA(HDC hDC,
                     LPCSTR szFontFileName,
                     USHORT usTTCIndex,
                     ULONG ulFlags,
                     ULONG ulCharSet,
                     ULONG* pulPrivStatus,
                     ULONG* pulStatus,
                     WRITEEMBEDPROC lpfnWriteToStream,
                     LPVOID lpvWriteStream,
                     USHORT* pusCharCodeSet,
                     USHORT usCharCodeCount,
                     USHORT usLanguage,
                     TTEMBEDINFO* pTTEmbedInfo);

LONG
WINAPI
TTEnableEmbeddingForFacename(LPSTR lpszFacename,
                             BOOL bEnable);

LONG
WINAPI
TTGetEmbeddedFontInfo(ULONG ulFlags,
                      ULONG* pulPrivStatus,
                      ULONG ulPrivs,
                      ULONG* pulStatus,
                      READEMBEDPROC lpfnReadFromStream,
                      LPVOID lpvReadStream,
                      TTLOADINFO* pTTLoadInfo);

LONG
WINAPI
TTGetEmbeddingType(HDC hDC,
                   ULONG* pulPrivStatus);

LONG
WINAPI
TTIsEmbeddingEnabled(HDC hDC,
                     BOOL* pbEnabled);

LONG
WINAPI
TTIsEmbeddingEnabledForFacename(LPSTR lpszFacename,
                                BOOL* pbEnabled);

LONG
WINAPI
TTLoadEmbeddedFont(HANDLE *phFontReference,
                   ULONG ulFlags,
                   ULONG* pulPrivStatus,
                   ULONG ulPrivs,
                   ULONG* pulStatus,
                   READEMBEDPROC lpfnReadFromStream,
                   LPVOID lpvReadStream,
                   LPWSTR szWinFamilyName,
                   LPSTR szMacFamilyName,
                   TTLOADINFO* pTTLoadInfo);

LONG
WINAPI
TTRunValidationTests(HDC hDC,
                     TTVALIDATIONTESTPARAMS* pTestParam);

LONG
WINAPI
TTEmbedFontEx(HDC hDC,
              ULONG ulFlags,
              ULONG ulCharSet,
              ULONG* pulPrivStatus,
              ULONG* pulStatus,
              WRITEEMBEDPROC lpfnWriteToStream,
              LPVOID lpvWriteStream,
              ULONG* pulCharCodeSet,
              USHORT usCharCodeCount,
              USHORT usLanguage,
              TTEMBEDINFO* pTTEmbedInfo);

LONG
WINAPI
TTRunValidationTestsEx(HDC hDC,
                       TTVALIDATIONTESTPARAMSEX* pTestParam);

LONG
WINAPI
TTGetNewFontName(HANDLE* phFontReference,
                 LPWSTR szWinFamilyName,
                 long cchMaxWinName,
                 LPSTR szMacFamilyName,
                 long cchMaxMacName);

#ifdef __cplusplus
    }
#endif

#endif /* __T2EMBED_API_H */
