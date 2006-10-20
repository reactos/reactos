
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
#include <time.h>
#include <float.h>

namespace Sysreg_
{
#ifdef UNICODE

	using std::wcerr;
	using std::endl;

#define cerr wcerr

#else

	using std::cerr;
	using std::endl;

#endif


	using System_::PipeReader;

	string RosBootTest::VARIABLE_NAME = _T("ROSBOOT_CMD");
	string RosBootTest::CLASS_NAME = _T("rosboot");
	string RosBootTest::DEBUG_PORT = _T("ROSBOOT_DEBUG_PORT");
	string RosBootTest::DEBUG_FILE = _T("ROSBOOT_DEBUG_FILE");
	string RosBootTest::TIME_OUT = _T("ROSBOOT_TIME_OUT");

//---------------------------------------------------------------------------------------
	RosBootTest::RosBootTest() : RegressionTest(RosBootTest::CLASS_NAME),  m_Timeout(60.0)
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
		string timeout;
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
		
		if (conf_parser.getStringValue(RosBootTest::TIME_OUT, timeout))
		{
			TCHAR * stop;
			m_Timeout = _tcstod(timeout.c_str (), &stop);
			if (_isnan(m_Timeout) || m_Timeout == 0.0)
			{
				cerr << "Warning: overriding timeout with default of 60 sec" << endl;
				m_Timeout = 60.0;
			}
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
			cerr <<"Error: unknown debug port " << debug_port <<endl
				<<" Currently only file|pipe is supported" <<endl;
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

//		cerr << debug_data << endl;
		return true;

	}
//---------------------------------------------------------------------------------------
	bool RosBootTest::isTimeout(double max_timeout)
	{
		static time_t start = 0;

		if (!start)
		{
			time(&start);
			return false;
		}

		time_t stop;
		time(&stop);

		double elapsed = difftime(stop, start);
		if (elapsed > max_timeout)
		{
			return true;
		}
		else
		{
			return false;
		}

	}
//---------------------------------------------------------------------------------------
	bool RosBootTest::fetchDebugByPipe(string boot_cmd)
	{
		struct timeval ts;
		PipeReader pipe_reader;

		if (!pipe_reader.openPipe(boot_cmd, string(_T("rt"))))
		{
			cerr << "Error: failed to open pipe with cmd: " << boot_cmd <<endl;
			return false;
		}
		string Buffer;
		Buffer.reserve (10000);

		bool ret = true;

		while(!pipe_reader.isEof ())
		{
			if (isTimeout(m_Timeout))
			{
				break;
			}

			string::size_type size = pipe_reader.readPipe (Buffer);
			cerr << "XXXsize_type " << size <<endl;


			if (!checkDebugData(Buffer))
			{
				ret = false;
				break;
			}
		}
		pipe_reader.closePipe ();
		return ret;
	}
//---------------------------------------------------------------------------------------
	bool RosBootTest::fetchDebugByFile(Sysreg_::string boot_cmd, Sysreg_::string debug_log)
	{
		PipeReader pipe_reader;

		_tremove(debug_log.c_str ());
		if (!pipe_reader.openPipe(boot_cmd, string(_T("rt"))))
		{
			cerr << "Error: failed to open pipe with cmd: " << boot_cmd << endl;
			return false;
		}
		FILE * file = _tfopen(debug_log.c_str (), _T("rt"));
		if (!file)
		{
			cerr << "Error: failed to open debug log " << debug_log << endl;
			pipe_reader.closePipe ();
			return false;
		}

		TCHAR szBuffer[500];
		bool ret = true;

		while(!pipe_reader.isEof ())
		{
			if (_fgetts(szBuffer, sizeof(szBuffer) / sizeof(TCHAR), file))
			{
				string buffer = szBuffer;
				if (!checkDebugData(buffer))
				{
					ret = false;
					break;
				}
				if (isTimeout(m_Timeout))
				{
					break;
				}
			}
		}

		pipe_reader.closePipe ();		
		return ret;
	}

} // end of namespace Sysreg_