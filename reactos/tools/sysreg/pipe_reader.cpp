/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     pipe reader support
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */

#include "pipe_reader.h"

#include <iostream>
#include <assert.h>

namespace System_
{
#ifdef UNICODE

	using std::wcerr;
	using std::endl;

#define cerr wcerr

#else

	using std::cerr;
	using std::endl;

#endif

//---------------------------------------------------------------------------------------
	PipeReader::PipeReader() : m_File(NULL)
	{

	}

//---------------------------------------------------------------------------------------
	PipeReader::~PipeReader()
	{

	}

//---------------------------------------------------------------------------------------

	bool PipeReader::openPipe(const System_::string &PipeCmd, System_::string AccessMode)
	{
		if (m_File != NULL)
		{
			cerr << "PipeReader::openPipe> pipe already open" << endl;
			return false;
		}
		// 
		m_File = _tpopen(PipeCmd.c_str(), AccessMode.c_str());
		if (m_File)
		{
			cerr << "PipeReader::openPipe> successfully opened pipe" << endl;
			return true;
		}

		cerr << "PipeReader::openPipe> failed to open pipe " << PipeCmd << endl;
		return false;
	}

//---------------------------------------------------------------------------------------

	bool PipeReader::closePipe() 
	{
		if (!m_File)
		{
			cerr << "PipeReader::closePipe> pipe is not open" << endl;
			return false;
		}

		int res = _pclose(m_File);
		
		if (res == UINT_MAX)
		{
			cerr << "Error: _pclose failed " <<endl;
			return false;
		}

		m_File = NULL;
		return true;
	}

//---------------------------------------------------------------------------------------

	bool PipeReader::isEof() const 
	{
		return feof(m_File);
	}

//---------------------------------------------------------------------------------------

	string::size_type PipeReader::readPipe(System_::string &Buffer)
	{
		
		TCHAR * buf = (TCHAR *)Buffer.c_str();
		System_::string::size_type size = Buffer.capacity();

//#ifdef NDEBUG
		memset(buf, 0x0, sizeof(TCHAR) * size);
//#endif

		TCHAR * res = _fgetts(buf, size, m_File);
		if (!res)
		{
			cerr << "Error: PipeReader::readPipe failed" << endl;
			return 0;
		}
		return _tcslen(buf);
	}

//---------------------------------------------------------------------------------------

	bool PipeReader::writePipe(const string & Buffer)
	{
		//TODO
		// implement me
		cerr << "PipeReader::writePipe is not yet implemented" << endl;

		return false;
	}

} // end of namespace System_
