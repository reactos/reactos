
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
#include "namedpipe_reader.h"
//#include "sym_file.h"
#include "file_reader.h"
#include "os_support.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <time.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
 
			
namespace Sysreg_
{
	using std::vector;
	using System_::PipeReader;
	using System_::NamedPipeReader;
/*	using System_::SymbolFile; */
	using System_::FileReader;
	using System_::OsSupport;

#ifdef UNICODE
	using std::wofstream;
	typedef wofstream ofstream;
#else
	using std::ofstream;
#endif

	string RosBootTest::VARIABLE_NAME = _T("ROSBOOT_CMD");
	string RosBootTest::CLASS_NAME = _T("rosboot");
	string RosBootTest::DEBUG_PORT = _T("ROSBOOT_DEBUG_PORT");
	string RosBootTest::DEBUG_FILE = _T("ROSBOOT_DEBUG_FILE");
	string RosBootTest::TIME_OUT = _T("ROSBOOT_TIME_OUT");
	string RosBootTest::PID_FILE= _T("ROSBOOT_PID_FILE");
	string RosBootTest::DELAY_READ = _T("ROSBOOT_DELAY_READ");
	string RosBootTest::CHECK_POINT = _T("ROSBOOT_CHECK_POINT");
	string RosBootTest::SYSREG_CHECKPOINT = _T("SYSREG_CHECKPOINT:");
	string RosBootTest::CRITICAL_APP = _T("ROSBOOT_CRITICAL_APP");

//---------------------------------------------------------------------------------------
	RosBootTest::RosBootTest() : RegressionTest(RosBootTest::CLASS_NAME),  m_Timeout(60.0), m_Delayread(0)
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
		string delayread;
		bool ret;

		///
		/// read required configuration arguments
		///

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

			if (isnan(m_Timeout) || m_Timeout == 0.0)
			{
				cerr << "Warning: overriding timeout with default of 60 sec" << endl;
				m_Timeout = 60.0;
			}
		}

		if (conf_parser.getStringValue(RosBootTest::DELAY_READ, delayread))
		{
			TCHAR * stop;
			m_Delayread = _tcstoul(delayread.c_str (), &stop, 10);
			if (m_Delayread > 60 || m_Delayread < 0)
			{
				cerr << "Warning: disabling delay read" << endl;
				m_Delayread = 0;
			}
		}

		///
		/// read optional arguments
		///

		conf_parser.getStringValue (RosBootTest::CHECK_POINT, m_Checkpoint);
		conf_parser.getStringValue (RosBootTest::CRITICAL_APP, m_CriticalApp);
		conf_parser.getStringValue (RosBootTest::DEBUG_FILE, m_DebugFile);

		if (conf_parser.getStringValue (RosBootTest::PID_FILE, m_PidFile))
		{
			_tremove(m_PidFile.c_str ());
		}
		
		if (!_tcscmp(debug_port.c_str(), _T("pipe")))
		{
			ret = fetchDebugByPipe(boot_cmd);
		}
		else if (!_tcscmp(debug_port.c_str(),  _T("file")))
		{
			if (!m_DebugFile.length())
			{
				cerr << "Error: ROSBOOT_DEBUG_FILE is not set in configuration file" << endl;
				return false;
			}
			ret = fetchDebugByFile(boot_cmd);
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
	void RosBootTest::dumpCheckpoints()
	{
		if (m_Checkpoints.size ())
		{
			cerr << "Dumping list of checkpoints: "<< endl;
			while(!m_Checkpoints.empty ())
			{
				cerr << m_Checkpoints[0] << endl;
				m_Checkpoints.erase (m_Checkpoints.begin ());

			}
		}
	}
//---------------------------------------------------------------------------------------
	RosBootTest::DebugState RosBootTest::checkDebugData(vector<string> & debug_data)
	{
		/// TBD the information needs to be written into an provided log object
		/// which writes the info into HTML/log / sends etc ....

		bool clear = true;
		DebugState state = DebugStateContinue;

		for(size_t i = 0; i < debug_data.size();i++)
		{
			string line = debug_data[i];

			cout << line << endl;

			if (line.find (RosBootTest::SYSREG_CHECKPOINT) != string::npos)
			{
				line.erase (0, line.find (RosBootTest::SYSREG_CHECKPOINT) + RosBootTest::SYSREG_CHECKPOINT.length ());
				if (!_tcsncmp(line.c_str (), m_Checkpoint.c_str (), m_Checkpoint.length ()))
				{
					state = DebugStateCPReached;
					break;
				}
				m_Checkpoints.push_back (line);
			}


			if (line.find (_T("*** Fatal System Error")) != string::npos)
			{
				cerr << "Blue Screen of Death detected" <<endl;
				if (m_Checkpoints.size ())
				{
					cerr << "dumping list of reached checkpoints:" << endl;
					do
					{
						string cp = m_Checkpoints[0];
						m_Checkpoints.erase (m_Checkpoints.begin ());
						cerr << cp << endl;
					}while(m_Checkpoints.size ());
					cerr << _T("----------------------------------") << endl;
				}
				if (i + 1 < debug_data.size () )
				{
					cerr << "dumping rest of debug log" << endl;
					while(i < debug_data.size ())
					{
						string data = debug_data[i];
						cerr << data << endl;
						i++;
					}
					cerr << _T("----------------------------------") << endl;
				}
				state = DebugStateBSODDetected;
				break;
			}
			else if (line.find (_T("Unhandled exception")) != string::npos)
			{
				if (m_CriticalApp == _T("IGNORE"))
				{
					///
					/// ignoring all user-mode exceptions
					///
					continue;
				}


				if (i + 3 >= debug_data.size ())
				{
					///
					/// missing information is cut off -> try reconstruct at next call
					///
					clear = false;
					break;
				}

				///
				/// extract address from next line
				/// 

				string address = debug_data[i+2];
				string::size_type pos = address.find_last_of (_T(" "));
				if (pos == string::npos)
				{
					cerr << "Error: trace is not available (corrupted debug info" << endl;
					continue;
				}


				address = address.substr (pos, address.length () - 1 - pos);

				///
				/// extract module name
				///
				string modulename = debug_data[i+3];
				
				pos = modulename.find_last_of (_T("\\"));
				if (pos == string::npos)
				{
					cerr << "Error: trace is not available (corrupted debug info" << endl;
					continue;
				}

				string appname = modulename.substr (pos + 1, modulename.length () - pos);
				if (m_CriticalApp.find (appname) == string::npos && m_CriticalApp.length () > 1)
				{
					/// the application is not in the list of 
					/// critical apps. Therefore we ignore the user-mode
					/// exception

					continue;
				}

				pos = appname.find_last_of (_T("."));
				if (pos == string::npos)
				{
					cerr << "Error: trace is not available (corrupted debug info" << endl;
					continue;
				}

				modulename = appname.substr (0, pos);

				cerr << "UM detected" <<endl;
				state = DebugStateUMEDetected;

				///
				/// resolve address
				///
				string result;
				result.reserve (200);
#if 0
				SymbolFile::resolveAddress (modulename, address, result);
				cerr << result << endl;
#endif				
				///
				/// TODO
				///
				/// resolve frame addresses 



				break;
			}
		}

		if (clear && debug_data.size () > 5)
		{
			debug_data.clear ();
		}
		return state;
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
		string pipecmd = _T("");
		bool ret = true;
		
		///
		/// FIXME
		/// split up arguments


		OsSupport::ProcessID pid = OsSupport::createProcess ((TCHAR*)boot_cmd.c_str (), 0, NULL); 
		string::size_type pipe_pos = boot_cmd.find (_T("serial pipe:"));

		NamedPipeReader namedpipe_reader;
		if (pipe_pos != string::npos)
		{
			pipe_pos += 12;
			string::size_type pipe_pos_end = boot_cmd.find (_T(" "), pipe_pos);
			if (pipe_pos != string::npos && pipe_pos > 0 && pipe_pos < boot_cmd.size())
			{
				pipecmd = _T("\\\\.\\pipe\\") + boot_cmd.substr (pipe_pos, pipe_pos_end - pipe_pos);
			}
		}

		if (!pipecmd.length ())
		{
			//FIXME
			pipecmd = _T("\\\\.\\pipe\\vmwaredebug");
		}

		if (m_Delayread)
		{
			///
			/// delay reading until emulator is ready
			///

			sleep( (clock_t)m_Delayread * CLOCKS_PER_SEC );
		}

		if (!namedpipe_reader.openPipe(pipecmd))
		{
			cerr << "Error: failed to open pipe with cmd: " << boot_cmd <<endl;
			return false;
		}
		string Buffer;
		Buffer.reserve (500);

		vector<string> vect;
		size_t lines = 0;
		bool write_log;
		ofstream file;
		if (m_DebugFile.length ())
		{
			_tremove(m_DebugFile.c_str ());
			file.open (m_DebugFile.c_str ());
		}

		write_log = file.is_open ();

		while(1)
		{
			if (isTimeout(m_Timeout))
			{
				break;
			}
			size_t prev_count = vect.size ();
			size_t line_count = namedpipe_reader.readPipe (vect);
			if (!line_count)
			{
				cerr << "No data read" << endl;
				continue;				
			}
			if (write_log)
			{
				for (size_t i = prev_count; i < vect.size (); i++)
				{
					string & line = vect[i];
					file << line;
				}
			}

			DebugState state = checkDebugData(vect);
			if (state == DebugStateBSODDetected || state == DebugStateUMEDetected)
			{
				ret = false;
				break;
			}
			else if (state == DebugStateCPReached)
			{
				break;
			}
			lines += line_count;
		}
		namedpipe_reader.closePipe ();
		if (write_log)
		{
			file.close();
		}
		_sleep(3* CLOCKS_PER_SEC);
		OsSupport::terminateProcess (pid);

		return ret;
	}
//---------------------------------------------------------------------------------------
	bool RosBootTest::fetchDebugByFile(string boot_cmd)
	{
		PipeReader pipe_reader;
		_tremove(m_DebugFile.c_str ());

		if (!pipe_reader.openPipe(boot_cmd, string(_T("rt"))))
		{
			cerr << "Error: failed to open pipe with cmd: " << boot_cmd << endl;
			return false;
		}

		if (m_Delayread)
		{
			///
			/// delay reading until emulator is ready
			///

			sleep( (clock_t)m_Delayread * CLOCKS_PER_SEC );
		}

		OsSupport::ProcessID pid = 0;

		if (m_PidFile != _T(""))
		{
			FileReader file;
			if (file.openFile(m_PidFile.c_str ()))
			{
				vector<string> lines;
				file.readFile(lines);
				if (lines.size())
				{
					string line = lines[0];
					pid = _ttoi(line.c_str ());
				}
				file.closeFile();
			}
		}
		FileReader file;
		if (!file.openFile (m_DebugFile.c_str ()))
		{
			cerr << "Error: failed to open debug log " << m_DebugFile<< endl;
			pipe_reader.closePipe ();
			return false;
		}

		vector<string> lines;
		bool ret = true;

		while(!pipe_reader.isEof ())
		{
			if (file.readFile (lines))
			{
				DebugState state = checkDebugData(lines);

				if (state == DebugStateBSODDetected || state == DebugStateUMEDetected)
				{
					ret = false;
					break;
				}
				else if (state == DebugStateCPReached)
				{
					break;
				}
			}
			if (isTimeout(m_Timeout))
			{
				///
				/// timeout has been reached
				///
				if (m_Checkpoint != _T(""))
				{
					///
					/// timeout was reached but
					/// the checkpoint was not reached
					/// we see this as a "hang"
					///
					ret = false;
				}
				break;
			}
		}
		file.closeFile ();

		if (pid)
		{
			OsSupport::terminateProcess (pid);
		}

		pipe_reader.closePipe ();	
		return ret;
	}

} // end of namespace Sysreg_
