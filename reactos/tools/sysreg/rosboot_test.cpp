
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
#include <signal.h>
 
			
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

	string RosBootTest::ROS_EMU_TYPE= _T("ROS_EMU_TYPE");
	string RosBootTest::EMU_TYPE_QEMU = _T("qemu");
	string RosBootTest::EMU_TYPE_VMWARE = _T("vmware");
	string RosBootTest::ROS_EMU_PATH = _T("ROS_EMU_PATH");
	string RosBootTest::ROS_HDD_IMAGE= _T("ROS_HDD_IMAGE");
	string RosBootTest::ROS_CD_IMAGE = _T("ROS_CD_IMAGE");
	string RosBootTest::ROS_MAX_TIME = _T("ROS_MAX_TIME");
	string RosBootTest::ROS_LOG_FILE = _T("ROS_LOG_FILE");
	string RosBootTest::ROS_SYM_DIR = _T("ROS_SYM_DIR");
	string RosBootTest::ROS_DELAY_READ = _T("ROS_DELAY_READ");
	string RosBootTest::ROS_SYSREG_CHECKPOINT = _T("SYSREG_CHECKPOINT:");
	string RosBootTest::ROS_CRITICAL_IMAGE = _T("ROS_CRITICAL_IMAGE");
	string RosBootTest::ROS_EMU_KILL = _T("ROS_EMU_KILL");
	string RosBootTest::ROS_EMU_MEM = _T("ROS_EMU_MEM");
	string RosBootTest::ROS_BOOT_CMD = _T("ROS_BOOT_CMD");

//---------------------------------------------------------------------------------------
	RosBootTest::RosBootTest() : m_MaxTime(0.0), m_DelayRead(0)
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
        if (m_DelayRead)
		{
			///
			/// delay reading until emulator is ready
			///

            OsSupport::sleep(m_DelayRead);
		}
    }

//----------------------------------------------------------------------------------------
    bool RosBootTest::configureQemu()
    {
        string qemupath;
        string::size_type pos;
 
        bool bootfromcd = false;
	bool bootcmdprovided = false;
        if (m_CDImage != _T("") )
        {
            bootfromcd = true;    
        }
        
        if (m_BootCmd != _T("") )
        {
        	///
        	/// boot cmd is provided already
        	///
        	bootcmdprovided = true;
        }

#ifdef __LINUX__
        pos = m_EmuPath.find_last_of(_T("/"));
#else       
        pos = m_EmuPath.find_last_of(_T("\\"));
#endif
        if (pos == string::npos)
        {
            cerr << "Error: ROS_EMU_PATH is invalid!!!" << endl;
            return false; 
        }
        qemupath = m_EmuPath.substr(0, pos);


        if (m_HDDImage == _T(""))
        {
            if(!bootfromcd)
            {
                cerr << "Error: ROS_CD_IMAGE and ROS_HDD_IMAGE is not set!!!" << endl;
                return false;
            }
            string qemuimgdir = qemupath;
            m_HDDImage = _T("ros.img");
#ifdef __LINUX___            
                qemuimgdir += _T("qemu-img");
#else
                qemuimgdir += _T("qemu-img.exe");
#endif  
            ///
            /// FIXME
            /// call qemu-img to create the tool
            ///
            cerr << "Creating HDD Image ..." << qemuimgdir << endl;
        }
        
        if (!bootcmdprovided)
        {
        	if (m_MaxMem == "")
        	{
            		// default to 64M
            		m_MaxMem = "64";
        	}
		string pipe;
#ifdef __LINUX__
		pipe = _T("stdio");
		m_Src = _T("");
#else		
		pipe = _T("pipe:qemu");
		m_Src = _T("\\\\.\\pipe\\qemu");
#endif	 
        

        	if (bootfromcd)
        	{
            		m_BootCmd = m_EmuPath + _T(" -serial ") + pipe + _T(" -m ") + m_MaxMem + _T(" -hda ") + m_HDDImage +  _T(" -boot d -cdrom ") + m_CDImage;
        	}
        	else
        	{
            		m_BootCmd = m_EmuPath + _T(" -L ") + qemupath + _T(" -m ") + m_MaxMem + _T(" -boot c -serial ") + pipe;
        	}

        	if (m_KillEmulator == _T("yes"))
        	{
#ifdef __LINUX__
            		m_BootCmd += _T(" -pidfile pid.txt");
#endif
        	}
        	else
        	{
            		m_BootCmd += _T(" -no-reboot ");
        	}
	}
	else
	{
		string::size_type pos = m_BootCmd.find(_T("-serial"));
		if (pos != string::npos)
		{
			string pipe = m_BootCmd.substr(pos + 7, m_BootCmd.size() - pos -7);
			pos = pipe.find(_T("pipe:"));
			if (pos == 0)
			{
#ifdef __LINUX__
				cerr << "Error: popen doesnot support reading from pipe" << endl;
				return false;
#else
				pipe = pipe.substr(pos + 5, pipe.size() - pos - 5);
				pos = pipe.find(_T(" "));
				if (pos != string::npos)
				{
					m_Src = _T("\\\\.\\pipe\\") + pipe.substr(0, pos);	
				}
				else
				{
					m_Src = _T("\\\\.\\pipe\\" + pipe;
				}
#endif				
			}
			else
			{
				pos = pipe.find(_T("stdio"));
				if (pos == 0)
				{
#ifdef __LINUX__
					m_Src = m_BootCmd;
#else					
					cerr << "Error: sysreg  doesnot support reading stdio" << endl;
					return false;	
#endif				
				
				}
				
				
			}
			
		}
	
	
	}

        cerr << "Opening Data Source:" << m_BootCmd << endl;


#ifdef __LINUX__
        m_DataSource = new PipeReader();
        m_Src = m_BootCmd;
#else
        m_DataSource = new NamedPipeReader();
        if (!executeBootCmd())
        {
            cerr << "Error: failed to launch emulator with: " << m_BootCmd << endl;
            return false;
        }
#endif
        
        return true;
    }

//---------------------------------------------------------------------------------------
    bool RosBootTest::configureVmWare()
    {
        cerr << "VmWare is currently not yet supported" << endl;
        return false;
    }
//---------------------------------------------------------------------------------------
    bool RosBootTest::readConfigurationValues(ConfigParser &conf_parser)
    {
        if (!conf_parser.getStringValue(RosBootTest::ROS_EMU_TYPE, m_EmuType))
        {
            cerr << "Error: ROS_EMU_TYPE is not set" << endl;
            return false;
        }

        if (!conf_parser.getStringValue(RosBootTest::ROS_EMU_PATH, m_EmuPath))
        {
            cerr << "Error: ROS_EMU_PATH is not set" << endl;
            return false;
        }

        conf_parser.getStringValue (RosBootTest::ROS_HDD_IMAGE, m_HDDImage);
        conf_parser.getStringValue (RosBootTest::ROS_CD_IMAGE, m_CDImage);
        conf_parser.getDoubleValue (RosBootTest::ROS_MAX_TIME, m_MaxTime);
		conf_parser.getStringValue (RosBootTest::ROS_LOG_FILE, m_DebugFile);
        conf_parser.getStringValue (RosBootTest::ROS_SYM_DIR, m_SymDir);
        conf_parser.getIntValue (RosBootTest::ROS_DELAY_READ, m_DelayRead);
        conf_parser.getStringValue (RosBootTest::ROS_SYSREG_CHECKPOINT, m_Checkpoint);
        conf_parser.getStringValue (RosBootTest::ROS_CRITICAL_IMAGE, m_CriticalImage);
        conf_parser.getStringValue (RosBootTest::ROS_EMU_KILL, m_KillEmulator);
        conf_parser.getStringValue (RosBootTest::ROS_EMU_MEM, m_MaxMem);
        conf_parser.getStringValue (RosBootTest::ROS_BOOT_CMD, m_BootCmd);

        return true;
    }
//---------------------------------------------------------------------------------------
	void RosBootTest::cleanup()
	{
        	m_DataSource->closeSource();
		if (m_KillEmulator == "yes")
		{
        		OsSupport::sleep(3 * CLOCKS_PER_SEC);
        		if (m_Pid)
        		{
		    		OsSupport::terminateProcess (m_Pid);
        		}
        	}	
		delete m_DataSource;

	}
    
//---------------------------------------------------------------------------------------
	bool RosBootTest::execute(ConfigParser &conf_parser) 
	{
		if (!readConfigurationValues(conf_parser))
        {
            return false;
        }

        if (m_EmuType == EMU_TYPE_QEMU)
        {
            if (!configureQemu())
            {
                cerr << "Error: failed to configure qemu" << endl;
                return false;
            }
        }
        else if (m_EmuType == EMU_TYPE_VMWARE)
        {
            if (!configureVmWare())
            {
                cerr << "Error: failed to configure vmware" << endl;
                return false;
            }
        }
        else
        {   
            ///
            /// unsupported emulator

            cerr << "Error: ROS_EMU_TYPE value is not supported:" << m_EmuType << "=" << EMU_TYPE_QEMU << endl;
            return false;
        }
#ifndef __LINUX__
        OsSupport::sleep(500);
#endif

	assert(m_DataSource != 0);
        if (!m_DataSource->openSource(m_Src))
        {
            cerr << "Error: failed to open data source with " << m_Src << endl;
            cleanup();
            return false;
        }

#ifndef __LINUX__
        OsSupport::sleep(3000); //FIXME
#endif
        bool ret = analyzeDebugData();
	cleanup();

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

			if (line.find (RosBootTest::ROS_SYSREG_CHECKPOINT) != string::npos)
			{
				line.erase (0, line.find (RosBootTest::ROS_SYSREG_CHECKPOINT) +
							  RosBootTest::ROS_SYSREG_CHECKPOINT.length ());
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
				if (m_CriticalImage == _T("IGNORE"))
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
				if (m_CriticalImage.find (appname) == string::npos && m_CriticalImage.length () > 1)
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
        if (max_timeout == 0)
        {
            // no timeout specified
            return false;
        }
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
			if (isTimeout(m_MaxTime))
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
