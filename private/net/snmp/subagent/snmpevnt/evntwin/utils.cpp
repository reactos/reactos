
#include "stdafx.h"
#include "utils.h"
#include "globals.h"
#include "trapreg.h"

//***************************************************************************
//
//  MapEventToSeverity
//
//  Extract the severity field from the event ID and convert it to a
//  string equivallent.
//
//  Parameters:
//		DWORD dwEvent
//			The full event ID
//
//		CString& sResult
//			The severity code string is returned here.
//
//  Returns:
//		The severity code string is returned via sResult.
//
//  Status:
//
//***************************************************************************
void MapEventToSeverity(DWORD dwEvent, CString& sResult)
{
	//
	//  Values are 32 bit event ID values layed out as follows:
	//
	//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
	//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
	//  +---+-+-+-----------------------+-------------------------------+
	//  |Sev|C|R|     Facility          |               Code            |
	//  +---+-+-+-----------------------+-------------------------------+
	//
	//  where
	//
	//      Sev - is the severity code
	//
	//          00 - Success
	//          01 - Informational
	//          10 - Warning
	//          11 - Error
	//
	//      C - is the Customer code flag
	//
	//      R - is a reserved bit
	//
	//      Facility - is the facility code
	//
	//      Code - is the facility's status code
	//
	//
	// Define the facility codes

	static UINT auiResource[4] =
		{IDS_EVENT_SEV_SUCCESS,
		 IDS_EVENT_SEV_INFORMATIONAL,
		 IDS_EVENT_SEV_WARNING,
		 IDS_EVENT_SEV_ERROR
		 };
	
	int iSeverity = (dwEvent >> 30) & 3;
	sResult.LoadString(auiResource[iSeverity]);
}




//********************************************************************************
// FindSubstring
//
// Find a substring in some text and return an index to the starting
// position of the string if it is found.
//
// Parameters:
//		LPCSTR pszTemplate
//			Pointer to the string to find.
//
//		LPCSTR pszText
//			Pointer to the text that will be searched
//			for the template string.
//
// Returns:
//		An index to the location of the substring within the text if it
//		is found, otherwise -1.
//
//********************************************************************************
LONG FindSubstring(LPCTSTR pszTemplate, LPCTSTR pszText)
{
	if (*pszTemplate == 0) {
		// An empty template string matches anything, so return zero.
		// This should never really happen since it doesn't make much
		// sense to search for nothing.
		return 0;
	}

	LPCTSTR pszTextStart = pszText;
	while (*pszText) {
		// Iterate through the character positions in the text
		// and return the index to the starting char in the string
		// if it is found.
		LPCTSTR pch1 = pszTemplate;
		LPCTSTR pch2 = pszText;

        while (*pch1 && (*pch1 == *pch2))
        {
			++pch1;
			++pch2;
		}

		if (*pch1 == 0) {
			// We reached the end of the template string, so there
			// must have been a match.
			return (LONG)(pszText - pszTextStart);
		}

		++pszText;
	}
	// Failed to find the substring
	return -1;
}


//********************************************************************************
// FindWholeWord
//
// Find a whole word in some text and return an index to the starting
// position of the whole word if it is found.  Whole word means the
// specified template string followed by a whitespace or end of string.
//
// Parameters:
//		LPCSTR pszTemplate
//			Pointer to the "whole word" string to find.
//
//		LPCSTR pszText
//			Pointer to the text that will be searched
//			for the template string.
//
// Returns:
//		An index to the location of the "whole word" substring within the text if it
//		is found, otherwise -1.
//
//********************************************************************************
LONG FindWholeWord(LPCTSTR pszTemplate, LPCTSTR pszText)
{
	if (*pszTemplate == 0) {
		// An empty search string matches anything, so return the index
		// of the first character.
		return 0;
	}


	// Iterate through each character position checking for a whole-word
	// match at each position.
	LONG nchTemplate = _tcslen(pszTemplate);
	LPCTSTR pszTextStart = pszText;
	LPCTSTR pchTextLimit = pszText + (_tcslen(pszText) - nchTemplate);
	while (pszText <= pchTextLimit) {

		// Check to see if the word is contained anywhere within the text
		INT iPos = FindSubstring(pszTemplate, pszText);
		if (iPos == -1) {
			return -1;
		}

		// Point at the location of the template string within the text
		pszText += iPos;

		// Get the prefix character
		INT ichPrefix;
		if (pszText == pszTextStart) {
			// Beginning of line counts as white space.
			ichPrefix = _T(' ');
		}
		else {
			ichPrefix = *(pszText - 1);
		}

		// Get the suffix character.
		INT ichSuffix = pszText[nchTemplate];
		if (ichSuffix == 0) {
			// End of line counts as whitespace
			ichSuffix = _T(' ');
		}

		// To match a whole word, the word must be bounded on either side
		// by whitespace.
		if (isspace(ichPrefix) && isspace(ichSuffix)) {
			return (LONG)(pszText - pszTextStart);
		}

		// Bump the text pointer to the next position so we don't do the
		// same thing all over again.
		++pszText;
	}
	return -1;
}
	

void DecString(CString& sValue, int iValue)
{
    // 32 bytes should be enough to hold any value
    TCHAR szValue[32];
    _itot(iValue, szValue, 10);
    sValue = szValue;
}


void DecString(CString& sValue, long lValue)
{
    // 32 bytes should be enough to hold any value
    TCHAR szValue[32];
    _ltot(lValue, szValue, 10);
    sValue = szValue;
}


void DecString(CString& sValue, DWORD dwValue)
{
    TCHAR szValue[32];
    _ultot(dwValue, szValue, 10);
    sValue = szValue;
}




CList::CList()
{
    m_pndPrev = this;
	m_pndNext = this;
}

void CList::Link(CList*& pndHead)
{
    if (pndHead == NULL)
        pndHead = this;
    else
    {

        m_pndNext = pndHead;
	    m_pndPrev = pndHead->m_pndPrev;
	    m_pndPrev->m_pndNext = this;
	    m_pndNext->m_pndPrev = this;
	}
}

void CList::Unlink(CList*& pndHead)
{
    if (pndHead == this)
	{
	    if (m_pndNext == this)
		    pndHead = NULL;
		else
	        pndHead = m_pndNext;
	}
	
    m_pndPrev->m_pndNext = m_pndNext;
	m_pndNext->m_pndPrev = m_pndPrev;
}




//***************************************************************
// GetFormattedValue
//
// Convert a value to ASCII and insert thousand separator characters
// into resulting value string.
//
// Parameters:
//      CString& sValueDst
//          The place to return the converted value.
//
//      LONG lValue
//          The value to convert.
//
//*****************************************************************
void GetFormattedValue(CString& sValueDst, LONG lValue)
{
    CString sValueSrc;
    DecString(sValueSrc, lValue);

    LONG nch = sValueSrc.GetLength();
    LPCTSTR pszSrc = sValueSrc;

    // Get a buffer as large as the source string plus the largest number of commas
    // plus one for the sign, one for the null terminator plus one character for slop.
    LPTSTR pszDst = sValueDst.GetBuffer(nch + nch / 3 + 3);

    // Copy any leading sign character.
    if ((*pszSrc == _T('+')) || (*pszSrc == _T('-'))) {
        *pszDst++ = *pszSrc++;
        --nch;
    }

    // Now copy the rest of the number and insert thousand separator characters in
    // the appropriate positions.
    LONG nchInitial = nch;
    while (nch > 0) {
        if ((nch % 3) == 0) {
            if (nch != nchInitial) {
                *pszDst++ = g_chThousandSep;
            }
        }
        *pszDst++ = *pszSrc++;
        --nch;
    }
    *pszDst = _T('\0');

    sValueDst.ReleaseBuffer();
}




//**************************************************************
// GenerateRangeMessage
//
// Generate a message indicating that the user should enter a value
// between some numbers nMin and nMax.
//
// Parameters:
//      CString& sMessage
//          The place to return the message.
//
//      LONG nMin
//          The minimum valid value in the range.
//
//      LONG nMax
//          The maximum valid value in the range.
//
//****************************************************************
void GenerateRangeMessage(CString& sMessage, LONG nMin, LONG nMax)
{
    CString sText;

    sMessage.LoadString(IDS_RANGE_MESSAGE_PREFIX);
    sMessage += _T(' ');

    GetFormattedValue(sText, nMin);
    sMessage += sText;
    sMessage += _T(' ');

    sText.LoadString(IDS_RANGE_VALUE_SEPARATOR);
    sMessage += sText;
    sMessage += _T(' ');


    GetFormattedValue(sText, nMax);
    sMessage += sText;

    sText.LoadString(IDS_SENTENCE_TERMINATOR);
    sMessage += sText;
}



//***************************************************************************
// GetThousandSeparator
//
// Get the thousand separator character for the current locale.
//
// Parameters:
//      TCHAR* pchThousandSep
//          Pointer to the place to return the thousand separator character.
//
// Returns:
//      SCODE
//          S_OK if the thousand separator was returned.
//          E_FAIL if the thousand separator was not returned.
//
//**************************************************************************
SCODE GetThousandSeparator(TCHAR* pchThousandSep)
{
// Digit + separator + 3 digits + decimal + two digits + null terminator  + 4 slop
#define MAX_CHARS_THOUSAND 12
    CString sValue;
    LPTSTR pszValue = sValue.GetBuffer(MAX_CHARS_THOUSAND);

    GetNumberFormat(NULL, 0, _T("1000"), NULL, pszValue, MAX_CHARS_THOUSAND);
    sValue.ReleaseBuffer();

    TCHAR ch = sValue[1];
    if (isdigit(ch)) {
        return E_FAIL;
    }
    *pchThousandSep = ch;
    return S_OK;
}



//***********************************************************************
// IsDecimalInteger
//
// This function tests a string to see whether or not it contains a
// valid integer expression.
//
// Parameters:
//      LPCTSTR pszValue
//          Pointer to the string to test.
//
// Returns:
//      BOOL
//          TRUE = The string contained a valid integer expression.
//          FALSE = The string did not contain a valid integer expression.
//
//***********************************************************************
BOOL IsDecimalInteger(LPCTSTR pszValue)
{
    // Accept leading white space
    while (iswspace(*pszValue)) {
        ++pszValue;
    }

    // Accept a leading plus or minus sign
    if ((*pszValue == _T('+'))  ||  (*pszValue == _T('-'))) {
        ++pszValue;
    }

    // Skip a string of consecutive digits with embedded thousand separators
    BOOL bSawThousandSep = FALSE;
    LONG nDigits = 0;
    while (TRUE) {
        if (*pszValue == g_chThousandSep) {
            if (nDigits > 3) {
                return FALSE;
            }

            bSawThousandSep = TRUE;
            nDigits = 0;
        }
        else if (isdigit(*pszValue)) {
            ++nDigits;
        }
        else {
            break;
        }
        ++pszValue;
    }

    if (bSawThousandSep && nDigits != 3) {
        // If a thousand separater was encountered, then there must be
        // three digits to the right of the last thousand separator.
        return FALSE;
    }


    // Accept trailing whitespace
    if (iswspace(*pszValue)) {
        ++pszValue;
    }


    if (*pszValue == _T('\0')) {
        // We reached the end of the string, so it must have been a decimal integer.
        return TRUE;
    }
    else {
        // We did not rech the end of the string, so it couldn't have been a valid
        // decimal integer value.
        return FALSE;
    }
}


//***************************************************************************
// AsciiToLong
//
// This function first validates a string to make sure that it is a properly
// formatted integer expression, and then converts it to a long.  Any embedded
// characters, such as the thousand separator, are stripped out before the
// conversion is done.
//
//
// Parameters:
//      LPCTSTR pszValue
//          Pointer to the string value to convert.
//
//      LONG* plResult
//          Pointer to the place to store the result.
//
// Returns:
//      SCODE
//          S_OK = The string contained a valid integer and the converted
//                 value was returned via plResult.
//          E_FAIL = The string did not contain a properly formatted integer
//                   expression.
//
//
//**************************************************************************
SCODE AsciiToLong(LPCTSTR pszValue, LONG* plResult)
{
    if (!IsDecimalInteger(pszValue)) {
        return E_FAIL;
    }


    // Strip out any superfluous characters, such as the thousand separator
    // before converting from ascii to long.
    CString sStrippedValue;
    LPTSTR pszDst = sStrippedValue.GetBuffer(_tcslen(pszValue) + 1);
    TCHAR ch;
    while (ch = *pszValue++) {
        if (isdigit(ch) || ch==_T('+') || ch==_T('-')) {
            *pszDst++ = ch;
        }
    }
    *pszDst = 0;
    sStrippedValue.ReleaseBuffer();

    *plResult = _ttol(sStrippedValue);
    return S_OK;
}
