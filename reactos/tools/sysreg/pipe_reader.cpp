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
	using std::cout;
	using std::endl;
	using std::cerr;

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
#ifdef NDEBUG			
			cerr << "PipeReader::openPipe> pipe already open" << endl;
#endif
			return false;
		}
		// 
		m_File = _tpopen(PipeCmd.c_str(), AccessMode.c_str());
		if (m_File)
		{
#ifdef NDEBUG
			cerr << "PipeReader::openPipe> successfully opened pipe" << endl;
#endif
			return true;
		}

#ifdef NDEBUG
		cerr << "PipeReader::openPipe> failed to open pipe " << PipeCmd << endl;
#endif
		return false;
	}

//---------------------------------------------------------------------------------------

	bool PipeReader::closePipe() 
	{
		if (!m_File)
		{
#ifdef NDEBUG
			cerr << "PipeReader::closePipe> pipe is not open" << endl;
#endif
			return false;
		}

		int res = _pclose(m_File);
		
		if (res == UINT_MAX)
		{
#ifdef NDEBUG
			cerr << "Error: _pclose failed " <<endl;
#endif
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
#ifdef NDEBUG
			cerr << "Error: PipeReader::readPipe failed" << endl;
#endif
			return 0;
		}

#ifdef NDEBUG
		cerr << "PipeReader::readPipe> res: " << Buffer << endl;
#endif

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
