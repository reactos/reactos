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

#include <iostream>
#include <assert.h>

namespace System_
{
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

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

	string::size_type NamedPipeReader::readPipe(string &Buffer)
	{
		TCHAR * buf = (TCHAR *)Buffer.c_str();
		string::size_type buffer_size = Buffer.capacity();
		string::size_type bytes_read = 0;
		DWORD cbRead;
		BOOL fSuccess;
		TCHAR * localbuf;
		DWORD localsize = MIN(100, buffer_size);

//#ifdef NDEBUG
		memset(buf, 0x0, sizeof(TCHAR) * buffer_size);
//#endif

#ifdef __LINUX__

#else
		localbuf = (TCHAR*) HeapAlloc(GetProcessHeap(), 0, localsize * sizeof(TCHAR));
		if (localbuf != NULL)
		{

			do
			{
				do 
				{ 
					ZeroMemory(localbuf, localsize * sizeof(TCHAR));

					fSuccess = ReadFile( 
						h_Pipe,
						localbuf,
						localsize * sizeof(TCHAR),
						&cbRead,
						NULL);
				 
					if (! fSuccess && GetLastError() != ERROR_MORE_DATA) 
						break; 
					
					if(bytes_read + cbRead > buffer_size)
					{
						Buffer.reserve(bytes_read + localsize * 3);
						buf = (TCHAR *)Buffer.c_str();
						buffer_size = Buffer.capacity();
					}

					memcpy(&buf[bytes_read], localbuf, cbRead);
					bytes_read += cbRead;

				} while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 
			} while (localbuf[_tcslen(localbuf)-1] != '\n');

			if (!fSuccess)
				return 0;

			HeapFree(GetProcessHeap(), 0, localbuf);
		}
		else
		{
			return 0;
		}
#endif
		buf[_tcslen(buf)-_tcslen(_T("\n"))-1] = '\0';
		return _tcslen(buf);
	}


} // end of namespace System_
