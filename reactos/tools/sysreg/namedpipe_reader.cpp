/* $Id: pipe_reader.cpp 24643 2006-10-24 11:45:21Z janderwald $
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/namedpipe_reader.cpp
 * PURPOSE:     pipe reader support
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 *				Christoph von Wittich (Christoph@ApiViewer.de)
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
	NamedPipeReader::NamedPipeReader() : h_Pipe(NULL)
	{

	}

//---------------------------------------------------------------------------------------
	NamedPipeReader::~NamedPipeReader()
	{

	}

//---------------------------------------------------------------------------------------

	bool NamedPipeReader::openPipe(string const & PipeCmd)
	{
		if (h_Pipe != NULL)
		{
			cerr << "NamedPipeReader::openPipe> pipe already open" << endl;
			return false;
		}

#ifdef __LINUX__

#else
		h_Pipe = CreateFile(PipeCmd.c_str(),
			GENERIC_WRITE | GENERIC_READ,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			(HANDLE)
			NULL);

		if(INVALID_HANDLE_VALUE == h_Pipe) {
			cerr << "NamedPipeReader::openPipe> failed to open pipe " << PipeCmd << GetLastError() << endl;
			return false;
		}
		else
		{
			cout << "NamedPipeReader::openPipe> successfully opened pipe" << endl;
			return true;
		}

		ConnectNamedPipe(
			h_Pipe,
			0);

#endif

	}

//---------------------------------------------------------------------------------------

	bool NamedPipeReader::closePipe() 
	{
		if (!h_Pipe)
		{
			cerr << "NamedPipeReader::closePipe> pipe is not open" << endl;
			return false;
		}


#ifdef __LINUX__

#else
		DisconnectNamedPipe(h_Pipe);
		CloseHandle(h_Pipe);
#endif

		h_Pipe = NULL;
		return true;
	}
//---------------------------------------------------------------------------------------
	void NamedPipeReader::extractLines(TCHAR * buffer, std::vector<string> & vect, bool & append_line, unsigned long cbRead)
	{
		TCHAR * offset = _tcsstr(buffer, _T("\x0D"));
		DWORD buf_offset = 0;
		while(offset)
		{
			offset[0] = _T('\0');
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

			offset += 2;
						
			buf_offset += line.length () + 2;
			if (buf_offset >= cbRead)
			{
				break;
			}
			buffer = offset;
			offset = _tcsstr(buffer, _T("\n"));
		}
		if (buf_offset < cbRead)
		{
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

	size_t NamedPipeReader::readPipe(vector<string> & vect)
	{
		char * localbuf;
		DWORD localsize = 100;
		size_t lines = vect.size ();

#ifdef WIN32
		BOOL fSuccess;
		localbuf = (char*) HeapAlloc(GetProcessHeap(), 0, localsize * sizeof(char));
#ifdef UNICODE
		wchar_t * wbuf = (WCHAR*) HeapAlloc(GetProcessHeap(), 0, localsize * sizeof(wchar_t));
#endif
		if (localbuf != NULL)
		{
			bool append_line = false;
			do
			{
				DWORD cbRead;
				do 
				{ 
					ZeroMemory(localbuf, localsize * sizeof(char));
#ifdef UNICODE
					ZeroMemory(wbuf, localsize * sizeof(wchar_t));
#endif

					fSuccess = ReadFile( 
						h_Pipe,
						localbuf,
						(localsize-1) * sizeof(char),
						&cbRead,
						NULL);
				 
					if (! fSuccess && GetLastError() != ERROR_MORE_DATA) 
						break; 

#ifdef UNICODE
					if (UnicodeConverter::ansi2Unicode(localbuf, wbuf, cbRead))
					{
						extractLines(wbuf, vect, append_line, cbRead);
					}
#else
					extractLines(localbuf, vect, append_line, cbRead);
#endif

				} while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 
			} while (localbuf[strlen(localbuf)-1] != '\n');

			if (!fSuccess)
				return 0;

			HeapFree(GetProcessHeap(), 0, localbuf);
#ifdef UNICODE
			HeapFree(GetProcessHeap(), 0, wbuf);
#endif
		}
		else
		{
			return 0;
		}
#endif

	return (vect.size () - lines);
	}


} // end of namespace System_
