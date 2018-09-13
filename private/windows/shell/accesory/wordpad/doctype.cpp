// doctype.cpp
//
// Copyright (C) 1992-1999 Microsoft Corporation
// All rights reserved.

#include "stdafx.h"
#include "resource.h"
#include "strings.h"

#include "multconv.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static const BYTE byteRTFPrefix[5] = {'{', '\\', 'r', 't', 'f'};
static const BYTE byteWord2Prefix[4] = {0xDB, 0xA5, 0x2D, 0x00};
static const BYTE byteCompFilePrefix[8] = {0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1};
static const BYTE byteWrite1Prefix[2] = {0x31, 0xBE};
static const BYTE byteWrite2Prefix[2] = {0x32, 0xBE};
static const BYTE byteWord5JPrefix[2] = {0x94, 0xA6};
static const BYTE byteWord5KPrefix[2] = {0x95, 0xA6};
static const BYTE byteWord5TPrefix[2] = {0x96, 0xA6};

//
// Registry paths to converter information.  Note that the array sizes must be
// at least MAX_PATH because the contents of the array will be replaced with
// the filesystem path in ScanForConverters.
//

#define CONVERTER_PATH(x) TEXT("Software\\Microsoft\\Shared Tools\\")   \
                            TEXT("Text Converters\\Import\\") TEXT(x)

TCHAR szWordConverter[MAX_PATH] = CONVERTER_PATH("MSWord6.wpc");
TCHAR szWriteConverter[MAX_PATH] = CONVERTER_PATH("MSWinWrite.wpc");
TCHAR szWord97Converter[MAX_PATH] = CONVERTER_PATH("MSWord8");

#undef CONVERTER_PATH

int RD_DEFAULT = RD_RICHTEXT;

/////////////////////////////////////////////////////////////////////////////

static BOOL IsConverterFormat(LPCTSTR pszConverter, LPCTSTR pszPathName);
static BOOL IsWord6(LPCTSTR pszPathName);

DocType doctypes[NUM_DOC_TYPES] =
{
    DECLARE_DOCTYPE(WINWORD2, FALSE, FALSE, FALSE, NULL),
    DECLARE_DOCTYPE(WINWORD6, TRUE, FALSE, TRUE, szWordConverter),
    DECLARE_DOCTYPE(WORD97, TRUE, FALSE, FALSE, szWord97Converter),
    DECLARE_DOCTYPE_SYN(WORDPAD, WINWORD6, TRUE, TRUE, FALSE, szWordConverter),
    DECLARE_DOCTYPE(WRITE, TRUE, FALSE, FALSE, szWriteConverter),
    DECLARE_DOCTYPE(RICHTEXT, TRUE, TRUE, FALSE, NULL),
    DECLARE_DOCTYPE(TEXT, TRUE, TRUE, FALSE, NULL),
    DECLARE_DOCTYPE(OEMTEXT, TRUE, TRUE, FALSE, NULL),
    DECLARE_DOCTYPE(UNICODETEXT, TRUE, TRUE, FALSE, NULL),
    DECLARE_DOCTYPE(ALL, TRUE, FALSE, FALSE, NULL),
    DECLARE_DOCTYPE_NULL(EMBEDDED, FALSE, FALSE, FALSE, NULL)
};

CString DocType::GetString(int nID)
{
	ASSERT(idStr != NULL);
	CString str;
	VERIFY(str.LoadString(idStr));
	CString strSub;
	AfxExtractSubString(strSub, str, nID);
	return strSub;
}

static BOOL IsConverterFormat(LPCTSTR pszConverter, LPCTSTR pszPathName)
{
	CConverter conv(pszConverter);
	return conv.IsFormatCorrect(pszPathName);
}

static BOOL IsLeadMatch(CFile& file, const BYTE* pb, UINT nCount)
{
	// check for match at beginning of file
	BOOL b = FALSE;
	BYTE* buf = new BYTE[nCount];
	
	TRY
	{
		file.SeekToBegin();
		memset(buf, 0, nCount);
		file.Read(buf, nCount);
		if (memcmp(buf, pb, nCount) == 0)
			b = TRUE;
	}
	END_TRY

	delete [] buf;
	return b;
}

static BOOL IsWord6(LPCTSTR pszPathName)
{
        USES_CONVERSION;
        BOOL bRes = FALSE;
        // see who created it
        LPSTORAGE lpStorage;
        SCODE sc = StgOpenStorage(T2COLE(pszPathName), NULL,
                STGM_READ|STGM_SHARE_EXCLUSIVE, 0, 0, &lpStorage);
        if (sc == NOERROR)
        {
                LPSTREAM lpStream;
                sc = lpStorage->OpenStream(T2COLE(szSumInfo), NULL,
                        STGM_READ|STGM_SHARE_EXCLUSIVE, NULL, &lpStream);
                if (sc == NOERROR)
                {
                        lpStream->Release();
                        bRes = TRUE;
                }
                lpStorage->Release();
        }
        return bRes;
}



//+---------------------------------------------------------------------------
//
//  Function:   GetDocTypeFromName
//
//  Synopsis:   Give a filename, determine what sort of document it is
//  
//  Parameters: [pszPathName]   -- The filename
//              [fe]            -- Exception that caused file.open to fail
//              [defaultToText] -- See notes below
//
//  Returns:    The file type or -1 for unknown/error
//
//  Notes:      The converters don't support Unicode but the filenames do.
//              This causes problems because even though the initial check
//              for file existance will succeed the converter will be unable
//              to load the file.  We get around this by trying to load the
//              file a second time using the shortname.  However, the behavior
//              if we can't load the file as it's native type is to load it as
//              a text file.  [defaultToText] is used to suppress this on the
//              first try so we know to try again.
//
//----------------------------------------------------------------------------

int GetDocTypeFromName(
        LPCTSTR pszPathName, 
        CFileException& fe, 
        bool defaultToText)
{
	CFile file;
	ASSERT(pszPathName != NULL);
	
	ScanForConverters();

	if (!file.Open(pszPathName, CFile::modeRead | CFile::shareDenyWrite, &fe))
		return -1;

	CFileStatus _stat;
	VERIFY(file.GetStatus(_stat));

	if (_stat.m_size == 0) // file is empty
	{
		CString ext = CString(pszPathName).Right(4);
		if (ext[0] != '.')
			return RD_TEXT;
		if (lstrcmpi(ext, _T(".doc"))==0)
			return RD_WORDPAD;
		if (lstrcmpi(ext, _T(".rtf"))==0)
			return RD_RICHTEXT;
		return RD_TEXT;
	}

	// RTF
	if (IsLeadMatch(file, byteRTFPrefix, sizeof(byteRTFPrefix)))
		return RD_RICHTEXT;

	// WORD 2
	if (IsLeadMatch(file, byteWord2Prefix, sizeof(byteWord2Prefix)))
		return RD_WINWORD2;
    
    // FarEast Word5, which is based on US Word 2
    if (IsLeadMatch(file, byteWord5JPrefix, sizeof(byteWord5JPrefix)) ||
        IsLeadMatch(file, byteWord5KPrefix, sizeof(byteWord5KPrefix)) ||
        IsLeadMatch(file, byteWord5TPrefix, sizeof(byteWord5TPrefix)))
    {
        return RD_FEWINWORD5;
    }
    
	// write file can start with 31BE or 32BE depending on whether it has
	// OLE objects in it or not
	if (IsLeadMatch(file, byteWrite1Prefix, sizeof(byteWrite1Prefix)) ||
		IsLeadMatch(file, byteWrite2Prefix, sizeof(byteWrite2Prefix)))
	{
		file.Close();
		if (IsConverterFormat(szWriteConverter, pszPathName))
			return RD_WRITE;
		else if (defaultToText)
			return RD_TEXT;
        else 
            return -1;
	}

	// test for compound file
	if (IsLeadMatch(file, byteCompFilePrefix, sizeof(byteCompFilePrefix)))
	{
		file.Close();

		if (IsConverterFormat(szWordConverter, pszPathName))
        {
            if (IsWord6(pszPathName))
                return RD_WINWORD6;
            else
                return RD_WORDPAD;
        }
		else if (IsConverterFormat(szWord97Converter, pszPathName))
        {
			return RD_WORD97;
        }
        else if (defaultToText)
        {
		    return RD_TEXT;
        }
        
        return -1;
	}

    //
    // If we get here we know the file exists but it is NOT any of the above
    // types.  Therefore it is either a text file or we need to open it as
    // a text file.  Either way we are justified in returning RD_TEXT
    // regardless of the defaultToText setting.
    //

    return RD_TEXT;
}

//+--------------------------------------------------------------------------
//
//  Function:   ScanForConverters
//
//  Synopsis:   Check for any text converters
//
//  Parameters: None
//  
//  Returns:    void
//
//  Notes:      This routine will update the entries in the global doctypes
//              structure.  It should be called before trying to use the
//              converters.  The code will only run once, even if it is called
//              multiple times.
//
//              The doctypes structure is expected to be initialized with the
//              registry path to the converter.  This path is replaced with 
//              the filesystem path or NULL if an error occurs.
//
//---------------------------------------------------------------------------

void ScanForConverters()
{
#ifdef _DEBUG
#define TRACE_ERROR(error, api, string)                                     \
        {if (ERROR_SUCCESS != error)                                        \
                TRACE(                                                      \
                    TEXT("Wordpad: error 0x%08x from %s looking for")       \
                                    TEXT("\r\n\t%s\r\n"),                   \
                    error,                                                  \
                    api,                                                    \
                    string);                                                \
        }
#else // !_DEBUG
#define TRACE_ERROR(error, api, string)
#endif // !_DEBUG

    static BOOL bScanned = FALSE;

    if (bScanned)
        return;
    
    TCHAR   szConverterPath[MAX_PATH];
    TCHAR   szExpandedPath[MAX_PATH];
    DWORD   error;

    for (int i = 0; i < NUM_DOC_TYPES; i++)
    {
		//
		// If this type is a duplicate of some other type don't try to search
		// for the converter twice
		//

		if (doctypes[i].bDup)
			continue;

        LPCTSTR pszConverterKey = doctypes[i].pszConverterName;
        DWORD   cbConverterPath = sizeof(szConverterPath);
        HKEY    keyConverter;

        if (NULL != pszConverterKey)
        {
            error = RegOpenKey(
                            HKEY_LOCAL_MACHINE, 
                            pszConverterKey, 
                            &keyConverter);
    
            TRACE_ERROR(error, TEXT("RegOpenKey"), pszConverterKey);

            if (ERROR_SUCCESS == error)
            {                                
                error = RegQueryValueEx(
                                    keyConverter,
                                    TEXT("Path"),
                                    NULL,
                                    NULL,
                                    (LPBYTE) szConverterPath,
                                    &cbConverterPath);

                TRACE_ERROR(error, TEXT("RegQueryValueEx"), TEXT("Path"));

                RegCloseKey(keyConverter);

                if (ERROR_SUCCESS == error)
                {
                    if (0 == ExpandEnvironmentStrings(
                                        szConverterPath,
                                        szExpandedPath,
                                        MAX_PATH))
                    {
                        error = GetLastError();
                        TRACE_ERROR(
                                error, 
                                TEXT("ExpandEnvironmentStrings"),
                                szConverterPath);
                    }
                }
                                    
                if (ERROR_SUCCESS == error)
                {
                    //
                    // The FILE_ATTRIBUTE_DIRECTORY bit will also be set if an
                    // error occurs - like file not found.
                    //

                    error = GetFileAttributes(szExpandedPath)
                                & FILE_ATTRIBUTE_DIRECTORY;

                    TRACE_ERROR(
                        error, 
                        TEXT("GetFileAttribytes"), 
                        szExpandedPath);
                }
            }

            if (ERROR_SUCCESS == error)
                _tcscpy(doctypes[i].pszConverterName, szExpandedPath);   
            else
                doctypes[i].pszConverterName = NULL;
        }
    }

    bScanned = TRUE;
}

CString GetExtFromType(int nDocType)
{
	ScanForConverters();

	CString str = doctypes[nDocType].GetString(DOCTYPE_EXT);
	if (!str.IsEmpty())
	{
		ASSERT(str.GetLength() == 5); // "*.ext"
		ASSERT(str[1] == '.');
		return str.Right(str.GetLength()-1);
	}
	return str;
}

// returns an RD_* from an index into the openfile dialog types
int GetTypeFromIndex(int nIndex, BOOL bOpen)
{
	ScanForConverters();

    //
    // Word97 is excluded from the list of open file types in GetFileTypes.
    // Make up for it here.
    //

    if (bOpen)
        ++nIndex;

	int nCnt = 0;
	for (int i=0;i<NUM_DOC_TYPES;i++)
	{
		if (!doctypes[i].bDup &&
			(bOpen ? doctypes[i].bRead : doctypes[i].bWrite))
		{
			if (nCnt == nIndex)
				return i;
			nCnt++;
		}
	}
	ASSERT(FALSE);
	return -1;
}

// returns an index into the openfile dialog types for the RD_* type
int GetIndexFromType(int nType, BOOL bOpen)
{
	ScanForConverters();

	int nCnt = 0;
	for (int i=0;i<NUM_DOC_TYPES;i++)
	{
		if (!doctypes[i].bDup &&
			(bOpen ? doctypes[i].bRead : doctypes[i].bWrite))
		{
			if (i == nType)
				return nCnt;
			nCnt++;
		}
	}
	return -1;
}

CString GetFileTypes(BOOL bOpen)
{
	ScanForConverters();

	CString str;
	for (int i=0;i<NUM_DOC_TYPES;i++)
	{
		if (bOpen && doctypes[i].bRead 
			&& !doctypes[i].bDup 
			&& !(RD_WORD97 == doctypes[i].nID))
		{
			str += doctypes[i].GetString(DOCTYPE_DESC);
			str += (TCHAR)NULL;
			str += doctypes[i].GetString(DOCTYPE_EXT);
			str += (TCHAR)NULL;
		}
		else if (!bOpen && doctypes[i].bWrite && !doctypes[i].bDup)
		{
			str += doctypes[i].GetString(DOCTYPE_DOCTYPE);
			str += (TCHAR)NULL;
			str += doctypes[i].GetString(DOCTYPE_EXT);
			str += (TCHAR)NULL;
		}
	}
	return str;
}
