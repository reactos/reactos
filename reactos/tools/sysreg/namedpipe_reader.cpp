/* $Id: pipe_reader.cpp 24643 2006-10-24 11:45:21Z janderwald $
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/namedpipe_reader.cpp
 * PURPOSE:     pipe reader support
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 *              Christoph von Wittich (Christoph_vW@ReactOS.org)
 */

#include "namedpipe_reader.h"
#include "unicode.h"

#include <iostream>
#include <assert.h>

namespace System_
{
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

	using std::vector;
//---------------------------------------------------------------------------------------
    NamedPipeReader::NamedPipeReader() : DataSource(), h_Pipe(NULLVAL), m_Buffer(0)
	{
#ifdef UNICODE
        m_WBuffer = 0;
#endif
	}

//---------------------------------------------------------------------------------------
	NamedPipeReader::~NamedPipeReader()
	{
        if (m_Buffer)
            free(m_Buffer);
#ifdef UNICODE
        if (m_WBuffer)
            free(m_WBuffer);
#endif
	}

	bool NamedPipeReader::isSourceOpen()
	{
		return true;
	}

//---------------------------------------------------------------------------------------

	bool NamedPipeReader::openSource(const string & PipeCmd)
	{
		if (h_Pipe != NULLVAL)
		{
			cerr << "NamedPipeReader::openPipe> pipe already open" << endl;
			return false;
		}
#ifndef __LINUX__
		h_Pipe = CreateFile("\\\\.\\pipe\\qemu", //PipeCmd.c_str(),
			                GENERIC_WRITE | GENERIC_READ,
			                0,
			                NULL,
			                OPEN_EXISTING,
			                FILE_ATTRIBUTE_NORMAL,
			                (HANDLE)
			                NULL);

		if(INVALID_HANDLE_VALUE == h_Pipe) {
            cerr << "NamedPipeReader::openPipe> failed to open pipe " << PipeCmd << " Error:" << GetLastError() << endl;
			h_Pipe = NULLVAL;
			return false;
		}
		else
		{
			cout << "NamedPipeReader::openPipe> successfully opened pipe" << endl;
            m_BufferLength = 100;
            m_Buffer = (char*)malloc(sizeof(char) * m_BufferLength);
#ifdef UNICODE
            m_WBuffer = (WCHAR*)malloc(sizeof(WCHAR) * m_BufferLength);
#endif
		    ConnectNamedPipe(h_Pipe,
                 			 0);
            return true;
		}
#else
		h_Pipe = open(PipeCmd.c_str(), O_RDONLY);

		if(INVALID_HANDLE_VALUE == h_Pipe) {
			cerr << "NamedPipeReader::openPipe> failed to open pipe " << PipeCmd << endl;
			h_Pipe = NULLVAL;
			return false;
		}
		else
		{
            cout << "NamedPipeReader::openPipe> successfully opened pipe handle: "<< h_Pipe << endl << "Src: " << PipeCmd << endl;
            m_BufferLength = 100;
            m_Buffer = (char*)malloc(sizeof(char) * m_BufferLength);
			return true;
		}
#endif
	}

//---------------------------------------------------------------------------------------

	bool NamedPipeReader::closeSource() 
	{
		if (h_Pipe == NULLVAL)
		{
			cerr << "NamedPipeReader::closePipe> pipe is not open" << endl;
			return false;
		}
#ifdef __LINUX__
        close(h_Pipe);
#else
		DisconnectNamedPipe(h_Pipe);
		CloseHandle(h_Pipe);
#endif
		h_Pipe = NULLVAL;
		return true;
	}
//---------------------------------------------------------------------------------------
	void NamedPipeReader::extractLines(TCHAR * buffer, std::vector<string> & vect, bool & append_line, unsigned long cbRead)
	{
		TCHAR * offset = _tcschr(buffer, _T('\x0D'));
		DWORD buf_offset = 0;
		while(offset)
		{
			///
			/// HACKHACK
			/// due to some mysterious reason, _tcschr / _tcsstr sometimes returns
			/// not always the offset to the CR character but to the next LF
			/// in MSVC 2005 (Debug Modus)

			if (offset[0] == _T('\x0A'))
			{
				if (buf_offset)
				{
					offset--;
				}
				else
				{
					//TODO
					// implement me special case
				}
			}

			if (offset[0] == _T('\x0D'))
			{
				buf_offset += 2;
				offset[0] = _T('\0');
				offset +=2;
			}
			else
			{
				///
				/// BUG detected in parsing code
				///
				abort();
			}

			string line = buffer;
			if (append_line)
			{
				assert(vect.empty () == false);
				string prev_line = vect[vect.size () -1];
				prev_line += line;
				vect.pop_back ();
				vect.push_back (prev_line);
				append_line = false;
			}
			else
			{
				vect.push_back (line);
			}

			buf_offset += line.length();
			if (buf_offset >= cbRead)
			{
				break;
			}
			buffer = offset;

			offset = _tcsstr(buffer, _T("\n"));
		}
		if (buf_offset < cbRead)
		{
			buffer[cbRead - buf_offset] = _T('\0');
			string line = buffer;
			if (append_line)
			{
				assert(vect.empty () == false);
				string prev_line = vect[vect.size () -1];
				vect.pop_back ();
				prev_line += line;
				vect.push_back (prev_line);
			}
			else
			{
				vect.push_back (line);
				append_line = true;
			}
		}
		else
		{
			append_line = false;
		}
	}
//---------------------------------------------------------------------------------------
    bool NamedPipeReader::readPipe(char * buffer, int bufferlength, long & bytesread)
    {
        
#ifdef __LINUX__
        long cbRead = read(h_Pipe,
                           buffer,
                           (bufferlength-1) * sizeof(char));
#else
        DWORD cbRead = 0;
        BOOL fSuccess = ReadFile(h_Pipe,
						         buffer,
						         (bufferlength-1) * sizeof(char),
						         &cbRead,
						         NULL);
				 
		if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
            return false;
#endif

        bytesread = cbRead;
        return true;
    }
//---------------------------------------------------------------------------------------

	bool NamedPipeReader::readSource(vector<string> & vect)
	{
		size_t lines = vect.size ();

        if (h_Pipe == NULLVAL)
        {
            cerr << "Error: pipe is not open" << endl;
            return false;
        }

        if (!m_Buffer)
        {
            cerr << "Error: no memory" << endl;
        }

#ifdef UNICODE
        if (!m_WBuffer)
        {
            cerr << "Error: no memory" << endl;
        }
#endif

		bool append_line = false;
		do
		{
            memset(m_Buffer, 0x0, m_BufferLength * sizeof(char));
            long cbRead = 0;
                    
            if (!readPipe(m_Buffer, m_BufferLength-1, cbRead))
                break;

#ifdef UNICODE
            memset(m_WBuffer, 0x0, m_BufferLength * sizeof(WCHAR));
            if (!UnicodeConverter::ansi2Unicode(m_Buffer, m_WBuffer, cbRead))
                break;
            extractLines(m_WBuffer, vect, append_line, cbRead);
#else
            extractLines(m_Buffer, vect, append_line, cbRead);
#endif

        }while (append_line);

    return (vect.size () - lines);
}


} // end of namespace System_
