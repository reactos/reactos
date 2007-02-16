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
	NamedPipeReader::NamedPipeReader() : h_Pipe(NULLVAL)
	{

	}

//---------------------------------------------------------------------------------------
	NamedPipeReader::~NamedPipeReader()
	{

	}

//---------------------------------------------------------------------------------------

	bool NamedPipeReader::openPipe(string const & PipeCmd)
	{
		if (h_Pipe != NULLVAL)
		{
			cerr << "NamedPipeReader::openPipe> pipe already open" << endl;
			return false;
		}

#ifdef WIN32
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
			h_Pipe = NULLVAL;
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
#else
		h_Pipe = open("PipeCmd.c_str()", O_RDONLY);

		if(INVALID_HANDLE_VALUE == h_Pipe) {
			cerr << "NamedPipeReader::openPipe> failed to open pipe " << PipeCmd << endl;
			h_Pipe = NULLVAL;
			return false;
		}
		else
		{
			cout << "NamedPipeReader::openPipe> successfully opened pipe" << endl;
			return true;
		}
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


#ifndef WIN32
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
#endif /* UNICODE */
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
#endif /* UNICODE */

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
#else /* UNICODE */
					extractLines(localbuf, vect, append_line, cbRead);
#endif /* UNICODE */

				} while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 
			} while (append_line);

			if (!fSuccess)
				return 0;

			HeapFree(GetProcessHeap(), 0, localbuf);
#ifdef UNICODE
			HeapFree(GetProcessHeap(), 0, wbuf);
#endif /* UNICODE */
		}
		else
		{
			return 0;
		}

#else /* WIN32 */

	localbuf = (char*) malloc(localsize * sizeof(char));		
	if (localbuf != NULL)
	{

		DWORD cbRead;

		bool append_line = false;
		do
		{
			do 
			{ 
				memset(localbuf, 0, localsize * sizeof(char));

				cbRead = read(h_Pipe,
						localbuf,
						(localsize-1) * sizeof(char));
				 
				if (cbRead > 0)
				{
					extractLines(localbuf, vect, append_line, cbRead);
				}

			} while (!cbRead);  // repeat loop as long as there is data to read 
		} while (append_line);

		if (cbRead < 0)
			return 0;


		free(localbuf);
	}
	else
	{
		return 0;
	}

#endif /* WIN32 */

	return (vect.size () - lines);
	}


} // end of namespace System_
