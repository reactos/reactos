
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
#include "sym_file.h"

#include <iostream>
#include <vector>
#include <time.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
 

#ifndef __LINUX__
#include <windows.h>
#endif


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

	using std::vector;
	using System_::PipeReader;
	using System_::SymbolFile;

	string RosBootTest::VARIABLE_NAME = _T("ROSBOOT_CMD");
	string RosBootTest::CLASS_NAME = _T("rosboot");
	string RosBootTest::DEBUG_PORT = _T("ROSBOOT_DEBUG_PORT");
	string RosBootTest::DEBUG_FILE = _T("ROSBOOT_DEBUG_FILE");
	string RosBootTest::TIME_OUT = _T("ROSBOOT_TIME_OUT");
	string RosBootTest::PID_FILE= _T("ROSBOOT_PID_FILE");

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

		conf_parser.getStringValue (RosBootTest::PID_FILE, m_PidFile);

		
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
	bool RosBootTest::checkDebugData(vector<string> & debug_data)
	{
		/// TBD the information needs to be written into an provided log object
		/// which writes the info into HTML/log / sends etc ....

		bool clear = true;

		for(size_t i = 0; i < debug_data.size();i++)
		{
			string line = debug_data[i];

			cerr << line << endl;

			if (line.find (_T("SYSREG_CHECKPOINT")) != string::npos)
			{
				line.erase (0, line.find (_T("SYSREG_CHECKPOINT")) + 19);
				if (!_tcsncmp(line.c_str (), _T("USETUP_COMPLETE"), 15))
				{
					///
					/// we need to stop the emulator to avoid
					/// looping again into USETUP (at least with bootcdregtest)

					return false;
				}

			}


			if (line.find (_T("*** Fatal System Error")) != string::npos)
			{
				cerr << "BSOD detected" <<endl;
				return false;
			}
			else if (line.find (_T("Unhandled exception")) != string::npos)
			{
				if (i + 3 >= debug_data.size ())
				{
					///
					/// missing information is cut off -> try reconstruct at next call
					///
					clear = false;
					break;
				}

				cerr << "UM detected" <<endl;

				///
				/// extract address from next line
				/// 

				string address = debug_data[i+2];
				string::size_type pos = address.find_last_of (_T(" "));
				address = address.substr (pos, address.length () - 1 - pos);

				///
				/// extract module name
				///
				string modulename = debug_data[i+3];
				pos = modulename.find_last_of (_T("\\"));
				modulename = modulename.substr (pos + 1, modulename.length () - pos);
				pos = modulename.find_last_of (_T("."));
				modulename = modulename.substr (0, pos);

				///
				/// resolve address
				///
				string result;
				result.reserve (200);

				SymbolFile::resolveAddress (modulename, address, result);
				cerr << result << endl;
				
				///
				/// TODO
				///
				/// resolve frame addresses 

				return false;
			}
		
		}

		if (clear && debug_data.size () > 5)
		{
			debug_data.clear ();
		}
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
		PipeReader pipe_reader;

		if (!pipe_reader.openPipe(boot_cmd, string(_T("rt"))))
		{
			cerr << "Error: failed to open pipe with cmd: " << boot_cmd <<endl;
			return false;
		}
		string Buffer;
		Buffer.reserve (500);

		bool ret = true;
		vector<string> vect;

		while(!pipe_reader.isEof ())
		{
			if (isTimeout(m_Timeout))
			{
				break;
			}

			pipe_reader.readPipe (Buffer);
			vect.push_back (Buffer);


			if (!checkDebugData(vect))
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
		_tremove(m_PidFile.c_str ());
		if (!pipe_reader.openPipe(boot_cmd, string(_T("rt"))))
		{
			cerr << "Error: failed to open pipe with cmd: " << boot_cmd << endl;
			return false;
		}
		// FIXXME
		// give the emulator some time to load freeloadr
		_sleep( (clock_t)4 * CLOCKS_PER_SEC );

		int pid = 0;

		if (m_PidFile != _T(""))
		{
			FILE * pidfile = _tfopen(m_PidFile.c_str (), _T("rt"));
			if (pidfile)
			{
				TCHAR szBuffer[20];
				if (_fgetts(szBuffer, sizeof(szBuffer) / sizeof(TCHAR), pidfile))
				{
					pid = _ttoi(szBuffer);
				}

			}
			fclose(pidfile);
		}

		FILE * file = _tfopen(debug_log.c_str (), _T("rt"));
		if (!file)
		{
			cerr << "Error: failed to open debug log " << debug_log << endl;
			pipe_reader.closePipe ();
			return false;
		}

		TCHAR szBuffer[1000];
		bool ret = true;
		vector<string> vect;

		while(!pipe_reader.isEof ())
		{
			if (_fgetts(szBuffer, sizeof(szBuffer) / sizeof(TCHAR), file))
			{

				string line = szBuffer;
				
				while(line.find (_T("\x10")) != string::npos)
				{
					line.erase(line.find(_T("\x10")), 1);
				}

				if (line[0] != _T('(') && vect.size() >=1)
				{
					string prev = vect[vect.size () -1];
					prev.insert (prev.length ()-1, line);
					vect.pop_back ();
					vect.push_back (prev);

				}
				else
				{
					vect.push_back (line);
				}

				if (!checkDebugData(vect))
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
		fclose(file);
		if (pid)
		{
#ifdef __LINUX__

#else
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
			if (hProcess)
			{
				TerminateProcess(hProcess, 0);
			}
			CloseHandle(hProcess);
#endif
		}
		pipe_reader.closePipe ();	
		return ret;
	}

} // end of namespace Sysreg_
