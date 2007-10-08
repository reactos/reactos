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
#include <cstdio>
namespace System_
{
//---------------------------------------------------------------------------------------
    FileReader::FileReader() : DataSource(),  m_File(NULL)
	{
	}
//---------------------------------------------------------------------------------------
	FileReader::~FileReader()
	{
	}
//---------------------------------------------------------------------------------------
    bool FileReader::openSource(const string & filename)
	{
		m_File = fopen((char*)filename.c_str(), (char*)"rb");

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
	bool FileReader::closeSource()
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
	bool FileReader::readSource(vector<string> & lines)
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

		if (m_BufferedLines.length())
		{
			strcpy(szBuffer, m_BufferedLines.c_str());
			readoffset = m_BufferedLines.length();
		}

		do
		{
			if (total_length < num)
			{
				memmove(szBuffer, &szBuffer[total_length], num - total_length);
				readoffset = num - total_length;
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
			char * ptr;
			char * offset = szBuffer;

			total_length = 0;
			while((ptr = strstr(offset, "\x0D\x0A")) != NULL)
			{
				long length = ((long )ptr - (long)offset);
				length /= sizeof(char);

				offset[length] = '\0';

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
		    m_BufferedLines = &szBuffer[total_length];
		}

		return ret;
	}

} // end of namespace System_
