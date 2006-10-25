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
		string::size_type size = Buffer.capacity();
		DWORD cbRead;
		BOOL fSuccess;

//#ifdef NDEBUG
		memset(buf, 0x0, sizeof(TCHAR) * size);
//#endif

#ifdef __LINUX__

#else
		do 
		{ 
			fSuccess = ReadFile( 
				h_Pipe,
				buf,
				size,
				&cbRead,
				NULL);
		 
			if (! fSuccess && GetLastError() != ERROR_MORE_DATA) 
				break; 
		 
			_tprintf( TEXT("%s\n"), buf ); 
		} while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

		if (!fSuccess)
			return 0;
#endif
		
		return _tcslen(buf);
	}


} // end of namespace System_
