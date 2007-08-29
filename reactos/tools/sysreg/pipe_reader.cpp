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
	using std::vector;
//---------------------------------------------------------------------------------------
	PipeReader::PipeReader() : m_File(NULL)
	{

	}

//---------------------------------------------------------------------------------------
	PipeReader::~PipeReader()
	{

	}

//---------------------------------------------------------------------------------------

	bool PipeReader::openSource(string const & PipeCmd)
	{
		if (m_File != NULL)
		{
			cerr << "PipeReader::openPipe> pipe already open" << endl;
			return false;
		}

		cerr << "cmd>" << PipeCmd << endl;

		m_File = popen(PipeCmd.c_str(), "r"); //AccessMode.c_str());
		if (m_File)
		{
			cerr << "PipeReader::openPipe> successfully opened pipe" << endl;
			return true;
		}

		cerr << "PipeReader::openPipe> failed to open pipe " << PipeCmd << endl;
		return false;
	}

//---------------------------------------------------------------------------------------

	bool PipeReader::closeSource() 
	{
		if (!m_File)
		{
			cerr << "PipeReader::closePipe> pipe is not open" << endl;
			return false;
		}

		int res = pclose(m_File);
		
		if (res == INT_MAX)
		{
			cerr << "Error: _pclose failed " <<endl;
			return false;
		}

		m_File = NULL;
		return true;
	}

//---------------------------------------------------------------------------------------

	bool PipeReader::isSourceOpen()
	{
		return feof(m_File);
	}

//---------------------------------------------------------------------------------------

	bool PipeReader::readSource(vector<string> & lines)
	{
		TCHAR * buf = (TCHAR*)malloc(100 * sizeof(TCHAR));
//#ifdef NDEBUG
		memset(buf, 0x0, sizeof(TCHAR) * 100);
//#endif

		TCHAR * res = _fgetts(buf, 100, m_File);
		if (!res)
		{
			//cerr << "Error: PipeReader::readPipe failed" << endl;
			free(buf);
			return false;
		}
		string line(buf);
		lines.push_back(line);
		return true;
	}

} // end of namespace System_
