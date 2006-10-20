
/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     ReactOS boot test
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */


#include "rosboot_test.h"
#include "pipe_reader.h"

#include <iostream>

namespace Sysreg_
{
	using std::cout;
	using std::endl;
	using std::cerr;

	using System_::PipeReader;

	string RosBootTest::VARIABLE_NAME = _T("ROSBOOT_CMD");
	string RosBootTest::CLASS_NAME = _T("rosboot");
	string RosBootTest::DEBUG_PORT = _T("ROSBOOT_DEBUG_PORT");
	string RosBootTest::DEBUG_FILE = _T("ROSBOOT_DEBUG_FILE");
//---------------------------------------------------------------------------------------
	RosBootTest::RosBootTest() : RegressionTest(RosBootTest::CLASS_NAME)
	{

	}

//---------------------------------------------------------------------------------------
	RosBootTest::~RosBootTest() 
	{

	}

//---------------------------------------------------------------------------------------
	bool RosBootTest::execute(ConfigParser &conf_parser) 
	{
		string boot_cmd;
		string debug_port;
		bool ret;

		if (!conf_parser.getStringValue (RosBootTest::DEBUG_PORT, debug_port))
		{
			cerr << "Error: ROSBOOT_DEBUG_TYPE is not set in configuration file" << endl;
			return false;
		}
		if (!conf_parser.getStringValue(RosBootTest::VARIABLE_NAME, boot_cmd))
		{
			cerr << "Error: ROSBOOT_CMD is not set in configuration file" << endl;
			return false;
		}

		if (!_tcscmp(debug_port.c_str(), _T("pipe")))
		{
			ret = fetchDebugByPipe(boot_cmd);
		}
		else if (!_tcscmp(debug_port.c_str(),  _T("file")))
		{
			string debug_file;
			if (!conf_parser.getStringValue (RosBootTest::DEBUG_FILE, debug_file))
			{
				cerr << "Error: ROSBOOT_DEBUG_FILE is not set in configuration file" << endl;
				return false;
			}
			ret = fetchDebugByFile(boot_cmd, debug_file);
		}
		else
		{
			_tprintf(_T("Error: unknown debug port %s Currently only file|pipe is supported\n"), debug_port);
			return false;
		}


		return ret;
	}
//---------------------------------------------------------------------------------------
	bool RosBootTest::checkDebugData(string debug_data)
	{
		///
		/// FIXME
		///
		/// parse debug_data and output STOP errors, UM exception
		/// as well as important stages i.e. ntoskrnl loaded 
		/// TBD the information needs to be written into an provided log object
		/// which writes the info into HTML/log / sends etc ....

		_tprintf(debug_data.c_str ());
		return true;

	}
//---------------------------------------------------------------------------------------
	bool RosBootTest::fetchDebugByPipe(string boot_cmd)
	{
		struct timeval ts;
		PipeReader pipe_reader;

		if (!pipe_reader.openPipe(boot_cmd, string(_T("rt"))))
		{
			_tprintf(_T("Error: failed to open pipe with cmd: %s\n"), boot_cmd.c_str ());
			return false;
		}
		string Buffer;
		Buffer.reserve (10000);

//		gettimeofday(&ts, NULL);

		while(!pipe_reader.isEof ())
		{
			pipe_reader.readPipe (Buffer);
			if (!checkDebugData(Buffer))
			{
				break;
			}
			if (hasTimeout(&ts, 60000)

		}
		pipe_reader.closePipe ();
		return true;
	}
//---------------------------------------------------------------------------------------
	bool RosBootTest::fetchDebugByFile(Sysreg_::string boot_cmd, Sysreg_::string debug_log)
	{
		PipeReader pipe_reader;

		if (!pipe_reader.openPipe(boot_cmd, string(_T("rt"))))
		{
			_tprintf(_T("Error: failed to open pipe with cmd: %s\n"), boot_cmd.c_str ());
			return false;
		}
		FILE * file = _tfopen(debug_log.c_str (), _T("rt"));
		if (!file)
		{
			_tprintf(_T("Error: failed to open debug log %s\n", debug_log.c_str ()));
			pipe_reader.closePipe ();
			return false;
		}

		TCHAR szBuffer[500];

		do
		{
			if (_fgetts(szBuffer, sizeof(szBuffer) / sizeof(TCHAR), file))
			{
				string buffer = szBuffer;
				if (!checkDebugData(buffer))
				{
					break;
				}
			}
		}while(!pipe_reader.isEof ());

		pipe_reader.closePipe ();		
	}

} // end of namespace Sysreg_