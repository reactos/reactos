
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
	RosBootTest::RosBootTest() : m_Timeout(60.0), m_Delayread(0)
	{

	}

//---------------------------------------------------------------------------------------
	RosBootTest::~RosBootTest() 
	{

	}
//---------------------------------------------------------------------------------------
    bool RosBootTest::executeBootCmd()
    {
        m_Pid = OsSupport::createProcess ((TCHAR*)m_BootCmd.c_str(), 0, NULL); 
        if (!m_Pid)
        {
            cerr << "Error: failed to launch boot cmd" << m_BootCmd << endl;
            return false;
        }

        return true;
    }
//---------------------------------------------------------------------------------------
    void RosBootTest::delayRead()
    {
        if (m_Delayread)
		{
			///
			/// delay reading until emulator is ready
			///

            OsSupport::sleep(m_Delayread * CLOCKS_PER_SEC );
		}
    }


//---------------------------------------------------------------------------------------
    bool RosBootTest::configurePipe()
    {
        if (!_tcscmp(m_DebugPort.c_str(), _T("qemupipe")))
        {
            string::size_type pipe_pos = m_BootCmd.find(_T("-serial pipe:"));
            if (pipe_pos != string::npos)
            {
			    pipe_pos += 12;
			    string::size_type pipe_pos_end = m_BootCmd.find (_T(" "), pipe_pos);
                if (pipe_pos != string::npos && pipe_pos_end != string::npos)
			    {
    				m_Pipe = _T("\\\\.\\pipe\\") + m_BootCmd.substr (pipe_pos, pipe_pos_end - pipe_pos);
#ifdef __LINUX__
                    m_DataSource = new PipeReader();
#else
                    m_DataSource = new NamedPipeReader();
#endif
                    return true;
	    		}
            }
        }
        else if (!_tcscmp(m_DebugPort.c_str(), _T("vmwarepipe")))
        {
            cerr << "VmWare debug pipe is currently fixed to \\\\.\\pipe\\vmwaredebug" << endl;
		    m_Pipe = _T("\\\\.\\pipe\\vmwaredebug");
#ifdef __LINUX__
                    m_DataSource = new PipeReader();
#else
                    m_DataSource = new NamedPipeReader();
#endif

            return true;
        }

        //
        // FIXME
        // support other emulators
        
        return false;
    }
//---------------------------------------------------------------------------------------
    bool RosBootTest::configureFile()
    {
        if (!_tcscmp(m_DebugPort.c_str(), _T("qemufile")))
        {
            string::size_type file_pos = m_BootCmd.find(_T("-serial file:"));
            if (file_pos != string::npos)
            {
                file_pos += 12;
			    string::size_type file_pos_end = m_BootCmd.find (_T(" "), file_pos);
                if (file_pos != string::npos && file_pos_end != string::npos)
			    {
    				m_File = m_BootCmd.substr (file_pos, file_pos_end - file_pos);
                    m_DataSource = new FileReader();
                    return true;
	    		}
                cerr << "Error: missing space at end of option" << endl;
            }
        }
        else if (!_tcscmp(m_DebugPort.c_str(), _T("vmwarefile")))
        {
            cerr << "VmWare debug file is currently fixed to debug.log" << endl;
            m_File = "debug.log";
            m_DataSource = new FileReader();
            return true;
        }

        // 
        // FIXME
        // support other emulators

        return false;
    }
//---------------------------------------------------------------------------------------
    bool RosBootTest::readConfigurationValues(ConfigParser &conf_parser)
    {
		///
		/// read required configuration arguments
		///



		if (!conf_parser.getStringValue (RosBootTest::DEBUG_PORT, m_DebugPort))
		{
			cerr << "Error: ROSBOOT_DEBUG_TYPE is not set in configuration file" << endl;
			return false;
		}
		if (!conf_parser.getStringValue(RosBootTest::VARIABLE_NAME, m_BootCmd))
		{
			cerr << "Error: ROSBOOT_CMD is not set in configuration file" << endl;
			return false;
		}
		
        	string timeout;
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

        	string delayread;
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

        return true;
    }
//---------------------------------------------------------------------------------------
	bool RosBootTest::execute(ConfigParser &conf_parser) 
	{

		if (!readConfigurationValues(conf_parser))
        {
            return false;
        }
        
        string src;

        if (m_DebugPort.find(_T("pipe")) != string::npos)
        {
            if (!configurePipe())
            {
                cerr << "Error: failed to configure pipe" << endl;
                return false;
            }

#ifndef __LINUX__
            if (!executeBootCmd())
            {
                cerr << "Error: failed to launch emulator" << endl;
                return false;
            }
#endif
            src = m_Pipe;
        }
        else if (m_DebugPort.find(_T("file")) != string::npos)
        {
            if (!configureFile())
            {
                cerr << "Error: failed to configure pipe" << endl;
                return false;
            }
            if (!executeBootCmd())
            {
                cerr << "Error: failed to launch emulator" << endl;
                return false;
            }

            src = m_File;
        }
        
        if (!m_DataSource->openSource(src))
        {
            cerr << "Error: failed to open data source with " << src << endl;
            return false;
        }

        bool ret = analyzeDebugData();

        m_DataSource->closeSource();
        OsSupport::sleep(3 * CLOCKS_PER_SEC);
        if (m_Pid)
        {
		    OsSupport::terminateProcess (m_Pid);
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
				line.erase (0, line.find (RosBootTest::SYSREG_CHECKPOINT) +
							  RosBootTest::SYSREG_CHECKPOINT.length ());
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
	bool RosBootTest::analyzeDebugData()
	{
		vector<string> vect;
		size_t lines = 0;
		bool write_log;
		ofstream file;
        bool ret = true;

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

			if (!m_DataSource->readSource (vect))
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
			lines += (vect.size() -prev_count); //WTF?
        }
		m_DataSource->closeSource();
		if (write_log)
		{
			file.close();
		}

        return ret;
	}
} // end of namespace Sysreg_
