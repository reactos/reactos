#ifndef PIPE_READER_H__
#define PIPE_READER_H__

/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     file reading support
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */



#include "user_types.h"
#include "data_source.h"
#include <cstdio>
//#include <stdlib.h>

namespace System_
{
//---------------------------------------------------------------------------------------
///
/// class PipeReader
///
/// Description: this class implements a pipe reader. It uses _popen to perform opening of
///              pipe / _pclose

    class PipeReader : public DataSource
	{
	public:

//---------------------------------------------------------------------------------------
///
/// PipeReader
///
/// Description: constructor of class PipeReader

		PipeReader();

//---------------------------------------------------------------------------------------
///
/// virtual ~PipeReader
///
/// Description: destructor of class PipeReader

		virtual ~PipeReader();

//---------------------------------------------------------------------------------------
///
/// openPipe
///
/// Description: this function attempts to open a pipe. If an pipe is already open or
/// it fails to open a pipe, the function returns false
///
/// @param PipeCmd command of the pipe to open
/// @param AccessMode on how to open the pipe
///        accepted modes are t ... text mode   - not compatible with b
///                           b ... binary mode - not compatible with t
///                           r ... allows reading from the pipe
///                           w ... allows writing to the pipe
/// @return bool

		virtual bool openSource(const string & PipeCmd);

//---------------------------------------------------------------------------------------
///
/// closePipe
///
/// Description: closes a pipe. Returns true on success
///
/// @return bool

		virtual bool closeSource();

//---------------------------------------------------------------------------------------
///
/// readPipe
///
/// Description: attempts to read from the pipe. Returns true on success. If it returns
/// false, call PipeReader::isEoF() to determine if the pipe should be closed
///
/// @param Buffer to be written to
/// @return string::size_type

		bool readSource(std::vector<string> & lines);

//---------------------------------------------------------------------------------------
///
/// isEof
///
/// Description: returns true if the pipe has reached end of file. The caller should call
/// closePipe if this function returns true

	virtual bool isSourceOpen();

protected:
	FILE * m_File;

	}; // end of class PipeReader

} // end of namespace System_



#endif /* end of PIPE_READER_H__ */
