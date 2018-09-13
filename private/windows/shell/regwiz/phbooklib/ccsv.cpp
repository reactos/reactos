// ############################################################################
// INCLUDES
#include "pch.hpp"

#include "ccsv.h"
#include "debug.h"

// ############################################################################
// DEFINES
#define chComma ','
#define chNewline '\n'
#define chReturn '\r'

// ############################################################################
//
// CCSVFile - simple file i/o for CSV files
//
CCSVFile::CCSVFile()
{
	m_hFile = 0;
	m_iLastRead = 0;
	m_pchLast = m_pchBuf = NULL;
}

// ############################################################################
CCSVFile::~CCSVFile()
{
	AssertSz(!m_hFile,"CCSV file is still open");
}

// ############################################################################
BOOLEAN CCSVFile::Open(LPCSTR pszFileName)
{
	AssertSz(!m_hFile, "a file is already open.");
		
	m_hFile = CreateFile((LPCTSTR)pszFileName, 
							GENERIC_READ, FILE_SHARE_READ, 
							0, OPEN_EXISTING, 0, 0);
	if (INVALID_HANDLE_VALUE == m_hFile)
	{
		return FALSE;
	}
	m_pchLast = m_pchBuf = NULL;
	return TRUE;
}

// ############################################################################
BOOLEAN CCSVFile::ReadToken(LPSTR psz, DWORD cbMax)
{
	LPSTR	pszLast;
	char		ch;

	ch = (char) ChNext();
	if (-1 == ch)
		{
		return FALSE;
		}

	pszLast = psz + (cbMax - 1);
	while (psz < pszLast && chComma != ch && chNewline != ch && -1 != ch)
		{
		*psz++ = ch;
		ch = (char) ChNext(); //Read in the next character
		}

	*psz++ = '\0';

	return TRUE;
}

// ############################################################################
void CCSVFile::Close(void)
{
	if (m_hFile)
		CloseHandle(m_hFile);
#ifdef DEBUG
	if (!m_hFile) Dprintf("CCSVFile::Close was called, but m_hFile was already 0\n");
#endif
	m_hFile = 0;
}

// ############################################################################
BOOL CCSVFile::FReadInBuffer(void)
{
	//Read another buffer
#ifdef WIN16
	if ((m_cchAvail = _read(m_hFile, m_rgchBuf, CCSVFILE_BUFFER_SIZE)) <= 0)
		return FALSE;
#else
	if (!ReadFile(m_hFile, m_rgchBuf, CCSVFILE_BUFFER_SIZE, &m_cchAvail, NULL) || !m_cchAvail)
		{
		return FALSE;	 //nothing more to read
		}
#endif

	m_pchBuf = m_rgchBuf;
	m_pchLast = m_pchBuf + m_cchAvail;
	
	return TRUE; //success
}

// ############################################################################
inline int CCSVFile::ChNext(void)
{

LNextChar:
	if (m_pchBuf >= m_pchLast && !FReadInBuffer())  //implies that we finished reading the buffer. Read in some more.
		return -1;	 //nothing more to read

	m_iLastRead = *m_pchBuf++;
	if (chReturn == m_iLastRead)
		goto LNextChar;		//faster to NOT make extra function call

	return m_iLastRead;
}
