/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     file reading support 
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */

#include "file_reader.h"
#include <assert.h>

namespace System_
{
//---------------------------------------------------------------------------------------
	FileReader::FileReader() : m_File(NULL)
	{
	}
//---------------------------------------------------------------------------------------
	FileReader::~FileReader()
	{
	}
//---------------------------------------------------------------------------------------
	bool FileReader::openFile(TCHAR const * filename)
	{
#ifdef UNICODE
		m_File = _tfopen(filename, _T("rb,ccs=UNICODE"));
#else
		m_File = _tfopen(filename, _T("rb"));
#endif

		if (m_File)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
//---------------------------------------------------------------------------------------
	bool FileReader::closeFile()
	{
		if (!m_File)
		{
			return false;
		}

		if (!fclose(m_File))
		{
			m_File = NULL;
			return true;
		}

		return false;
	}
//---------------------------------------------------------------------------------------
	bool FileReader::readFile(vector<string> & lines)
	{
		if (!m_File)
		{
			return false;
		}
		
		bool ret = true;
		size_t total_length = 0;
		size_t line_count = lines.size();
		size_t num = 0;
		char szBuffer[256];
		int readoffset = 0;

#ifdef UNICODE
		wchar_t wbuf[512];
		int wbuf_offset = 0;

		if (m_BufferedLines.length ())
		{
			wcscpy(wbuf, m_BufferedLines.c_str ());
			wbuf_offset = m_BufferedLines.length ();
		}
#else
		if (m_BufferedLines.length())
		{
			strcpy(szBuffer, m_BufferedLines.c_str());
			readoffset = m_BufferedLines.length();
		}
#endif

		do
		{
			if (total_length < num)
			{
#ifdef UNICODE
				memmove(wbuf, &wbuf[total_length], (num - total_length) * sizeof(wchar_t));
				wbuf_offset = num - total_length;
#else
				memmove(szBuffer, &szBuffer[total_length], num - total_length);
				readoffset = num - total_length;
#endif
			}

			num = fread(&szBuffer[readoffset], 
				        sizeof(char), sizeof(szBuffer)/sizeof(char) - (readoffset+1) * sizeof(char),
						m_File);

			szBuffer[num] = L'\0';

			if (!num)
			{
				if (line_count == lines.size ())
				{
					ret = false;
				}
				break;
			}
			TCHAR * ptr;
#ifdef UNICODE
			int i = 0;
			int conv;
			while((conv = mbtowc(&wbuf[wbuf_offset+i], &szBuffer[i], num)))
			{
				i += conv;
				if (i == num)
					break;

				assert(wbuf_offset + i < 512);
			}
			wbuf[wbuf_offset + num] = L'\0';

			TCHAR * offset = wbuf;
#else

			TCHAR * offset = szBuffer;
#endif
			total_length = 0;
			while((ptr = _tcsstr(offset, _T("\x0D\x0A"))) != NULL)
			{
				long long length = ((long long)ptr - (long long)offset);
				length /= sizeof(TCHAR);

				offset[length] = _T('\0');

				string line = offset;
				lines.push_back (line);

				offset += length + 2;
				total_length += length + 2;

				if (total_length == num)
				{
					break;
				}
			}
		}while(num );

		if (total_length < num)
		{
#ifdef UNICODE
		m_BufferedLines = &wbuf[total_length];
#else
		m_BufferedLines = &szBuffer[total_length];
#endif
		}

		return ret;
	}

} // end of namespace System_
