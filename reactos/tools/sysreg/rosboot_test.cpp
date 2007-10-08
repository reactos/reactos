
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
#include "env_var.h"

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
//#include <io.h>

namespace Sysreg_
{
	using std::vector;
	using System_::PipeReader;
	using System_::NamedPipeReader;
/*	using System_::SymbolFile; */
	using System_::FileReader;
	using System_::OsSupport;
    using System_::EnvironmentVariable;

#ifdef UNICODE
	using std::wofstream;
	typedef wofstream ofstream;
#else
	using std::ofstream;
#endif

	string RosBootTest::ROS_EMU_TYPE= "ROS_EMU_TYPE";
	string RosBootTest::EMU_TYPE_QEMU = "qemu";
	string RosBootTest::EMU_TYPE_VMWARE = "vmware";
	string RosBootTest::ROS_HDD_IMAGE= "ROS_HDD_IMAGE";
	string RosBootTest::ROS_CD_IMAGE = "ROS_CD_IMAGE";
	string RosBootTest::ROS_MAX_TIME = "ROS_MAX_TIME";
	string RosBootTest::ROS_LOG_FILE = "ROS_LOG_FILE";
	string RosBootTest::ROS_SYM_DIR = "ROS_SYM_DIR";
	string RosBootTest::ROS_DELAY_READ = "ROS_DELAY_READ";
	string RosBootTest::ROS_SYSREG_CHECKPOINT = "SYSREG_CHECKPOINT:";
	string RosBootTest::ROS_CRITICAL_IMAGE = "ROS_CRITICAL_IMAGE";
	string RosBootTest::ROS_EMU_KILL = "ROS_EMU_KILL";
	string RosBootTest::ROS_EMU_MEM = "ROS_EMU_MEM";
	string RosBootTest::ROS_BOOT_CMD = "ROS_BOOT_CMD";

#ifdef __LINUX__
    string RosBootTest::ROS_EMU_PATH = "ROS_EMU_PATH_LIN";
#else
    string RosBootTest::ROS_EMU_PATH = "ROS_EMU_PATH_WIN";
#endif

//---------------------------------------------------------------------------------------
	RosBootTest::RosBootTest() : m_MaxTime(65), m_DelayRead(0)
	{

	}

//---------------------------------------------------------------------------------------
	RosBootTest::~RosBootTest() 
	{

	}
//---------------------------------------------------------------------------------------
    bool RosBootTest::executeBootCmd()
    {
        int numargs = 0;
        char * args[128];
        char * pBuf;
        char szBuffer[128];

        pBuf = (char*)m_BootCmd.c_str ();
        if (pBuf)
        {
            pBuf = strtok(pBuf, " ");
            while(pBuf != NULL)
            {
                if (!numargs)
                    strcpy(szBuffer, pBuf);

                args[numargs] = pBuf;
                numargs++;
                pBuf = strtok(NULL, " ");
            }
            args[numargs++] = 0;
        }
        else
        {
            strcpy(szBuffer, pBuf);
        }
        m_Pid = OsSupport::createProcess (szBuffer, numargs-1, args, false); 
        if (!m_Pid)
        {
            cerr << "Error: failed to launch boot cmd " << m_BootCmd << endl;
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

            OsSupport::delayExecution(m_DelayRead);
		}
    }
//---------------------------------------------------------------------------------------
    void RosBootTest::getDefaultHDDImage(string & img)
    {
        img = "output-i386";

        EnvironmentVariable::getValue("ROS_OUTPUT", img);
#ifdef __LINUX__
        img += "/ros.hd";
#else
        img += "\\ros.hd";
#endif
    }
//---------------------------------------------------------------------------------------
    bool RosBootTest::isFileExisting(string output)
    {
        FILE * file;
        file = fopen(output.c_str(), "r");
        
        if (file)
        {
            /* the file exists */
            fclose(file);
            return true;
        }
        return false;
    }

//---------------------------------------------------------------------------------------
    bool RosBootTest::isDefaultHDDImageExisting()
    {
        string img;

        getDefaultHDDImage(img);
        return isFileExisting(img);
    }

//---------------------------------------------------------------------------------------
    bool RosBootTest::createHDDImage(string image)
    {

        string qemuimgdir;
        if (!getQemuDir(qemuimgdir))
        {
            cerr << "Error: failed to retrieve qemu directory path" << endl;
            return false;
        }


#ifdef __LINUX__
        qemuimgdir += "/qemu-img";

#else
        qemuimgdir += "\\qemu-img.exe";
#endif  

       if (!isFileExisting(qemuimgdir))
        {
            cerr << "Error: ROS_EMU_PATH must contain the path to qemu and qemu-img " << qemuimgdir << endl;
            return false;
        }
       remove(image.c_str ());

        char * options[] = {NULL,
                             "create",
                             "-f",
#ifdef __LINUX__
                            "raw",
#else
                            "vmdk",
#endif
                            NULL,
                            "100M",
                            NULL
                            };


        options[0] = (char*)qemuimgdir.c_str();
        options[4] = (char*)image.c_str();
            
        cerr << "Creating HDD Image ..." << image << endl;
        OsSupport::createProcess ((char*)qemuimgdir.c_str(), 6, options, true);
        if (isFileExisting(image))
        {
            m_HDDImage = image;
            return true;
        }
        return false;
    }
//----------------------------------------------------------------------------------------
    bool RosBootTest::isQemuPathValid()
    {
        string qemupath;

        if (m_BootCmd.length())
        {
            /* the boot cmd is already provided 
             * check if path to qemu is valid
             */
            string::size_type pos = m_BootCmd.find_first_of(" ");
            if (pos == string::npos)
            {
                /* the bootcmd is certainly not valid */
                return false;
            }
            qemupath = m_BootCmd.substr(0, pos);
        }
        else
        {
            /* the qemu path is provided ROS_EMU_PATH variable */
            qemupath = m_EmuPath;
        }


        return isFileExisting(qemupath);
    }
//----------------------------------------------------------------------------------------
    bool RosBootTest::hasQemuNoRebootOption()
    {
        ///
        /// FIXME 
        /// extract version
        ///

        return true;
    }
//----------------------------------------------------------------------------------------
    bool RosBootTest::getQemuDir(string & qemupath)
    {
        string::size_type pos;

#ifdef __LINUX__
        pos = m_EmuPath.find_last_of("/");
#else       
        pos = m_EmuPath.find_last_of("\\");
#endif
        if (pos == string::npos)
        {
            cerr << "Error: ROS_EMU_PATH is invalid!!!" << endl;
            return false; 
        }
        qemupath = m_EmuPath.substr(0, pos);
        return true;
    }
//----------------------------------------------------------------------------------------
    bool RosBootTest::createBootCmd()
    {
        string pipe;
        string qemudir;
        char pipename[] = "qemuXXXXXX";
        if (m_MaxMem.length() == 0)
        {
            /* set default memory size to 64M */
            m_MaxMem = "64";
        }

#ifdef __LINUX__

        if (mktemp(pipename))
        {
            string temp = pipename;
            m_Src = "/tmp/" + temp;
            pipe = "pipe:" + m_Src;
        }
        else
        {
            pipe = "pipe:/tmp/qemu";
		    m_Src = "/tmp/qemu";
        }

        qemudir = "/usr/share/qemu";
        m_DebugPort = "pipe";
#else		
        if (_mktemp(pipename))
        {
            string temp = pipename;
    		pipe = "pipe:" + temp;
		    m_Src = "\\\\.\\pipe\\" + temp;
        }
        else
        {
    		pipe = "pipe:qemu";
		    m_Src = "\\\\.\\pipe\\qemu";
        }
        m_DebugPort = "pipe";

        if (!getQemuDir(qemudir))
        {
            return false;
        }
#endif	 

        
        m_BootCmd = m_EmuPath + " -L " + qemudir + " -m " + m_MaxMem + " -serial " + pipe;

        if (m_CDImage.length())
        {
            /* boot from cdrom */
            m_BootCmd +=  " -boot d -cdrom " + m_CDImage;

            if (m_HDDImage.length ())
            {
                /* add disk when specified */
                m_BootCmd += " -hda " + m_HDDImage;
            }

        }
        else if (m_HDDImage.length ())
        {
            /* boot from hdd */
            m_BootCmd += " -boot c -hda " + m_HDDImage;
        }
        else
        {
            /*
             * no boot device provided 
             */
            cerr << "Error: no bootdevice provided" << endl;
            return false;
        }

#ifdef __LINUX__
                /* on linux we need get pid in order to be able
                 * to terminate the emulator in case of errors
                 * on windows we can get pid as return of CreateProcess
                 */
        m_PidFile = "output-i386";
        EnvironmentVariable::getValue("ROS_OUTPUT", m_PidFile);
        m_PidFile += "/pid.txt";
        m_BootCmd += " -pidfile ";
        m_BootCmd += m_PidFile;
		m_BootCmd += " -vnc :0";
#else

        if (hasQemuNoRebootOption())
        {
            m_BootCmd += " -no-reboot ";
        }
#endif
        return true;
    }
//----------------------------------------------------------------------------------------
    bool RosBootTest::extractPipeFromBootCmd()
    {
		string::size_type pos = m_BootCmd.find("-serial");
        if (pos == string::npos)
        {
            /* no debug options provided */
            return false;
        }

        string pipe = m_BootCmd.substr(pos + 7, m_BootCmd.size() - pos -7);
	    pos = pipe.find("pipe:");
		if (pos == 0)
		{
		    pipe = pipe.substr(pos + 5, pipe.size() - pos - 5);
            pos = pipe.find(" ");
            if (pos != string::npos)
            {
                pipe = pipe.substr(0, pos);
            }
#ifdef __LINUX__
            m_Src = pipe;
#else
			m_Src = "\\\\.\\pipe\\" + pipe.substr(0, pos);
#endif
            m_DebugPort = "pipe";
            return true;
        }
        pos = pipe.find("stdio");
        if (pos == 0)
		{
#ifdef __LINUX__
			m_Src = m_BootCmd;
            m_DebugPort = "stdio";
            return true;
#else					
			cerr << "Error: reading from stdio is not supported for windows hosts - use pipes" << endl;
			return false;	
#endif				
		}

        cerr << "Error: no valid debug port specified - use stdio / pipes" << endl;
        return false;
    }
//----------------------------------------------------------------------------------------
    bool RosBootTest::configureHDDImage()
    {
        //cout << "configureHDDImage m_HDDImage " << m_HDDImage.length() << " m_CDImage " << m_CDImage.length() << " m_BootCmd: " << m_BootCmd.length() << endl;
        if (m_HDDImage.length())
        {
            /* check if ROS_HDD_IMAGE points to hdd image */
            if (!isFileExisting(m_HDDImage))
            {
                if (!m_CDImage.length ())
                {
                    cerr << "Error: HDD image is not existing and CDROM image not provided" << endl;
                    return false;
                }
                /* create it */
                return createHDDImage(m_HDDImage);
            }
            return true;
        }
        else if (!m_BootCmd.length ())
        {
            /* no hdd image provided
             * but also no override by 
             * ROS_BOOT_CMD
             */
            if (!m_CDImage.length ())
            {
                cerr << "Error: no HDD and CDROM image provided" << endl;
                return false;
            }

            getDefaultHDDImage(m_HDDImage);
            if (isFileExisting(m_HDDImage))
            {
                cerr << "Falling back to default hdd image " << m_HDDImage << endl;
                return true;
            }
            return createHDDImage(m_HDDImage);
        }
        /*
         * verify the provided ROS_BOOT_CMD for hdd image
         *
         */

        bool hdaboot = false;
        string::size_type pos = m_BootCmd.find ("-boot c");
        if (pos != string::npos)
        {
            hdaboot = true;
        }

        pos = m_BootCmd.find("-hda ");
        if (pos != string::npos)
        {
            string hdd = m_BootCmd.substr(pos + 5, m_BootCmd.length() - pos - 5);
            if (!hdd.length ())
            {
                cerr << "Error: ROS_BOOT_CMD misses value of -hda option" << endl;
                return false;
            }
            pos = m_BootCmd.find(" ");
            if (pos != string::npos)
            {
                /// FIXME
                /// sysreg assumes that the hdd image filename has no spaces
                ///
                hdd = hdd.substr(0, pos);
            }
            if (!isFileExisting(hdd))
            {
                if (hdaboot)
                {
                    cerr << "Error: ROS_BOOT_CMD specifies booting from hda but no valid hdd image " << hdd << " provided" << endl;
                    return false;
                }

                /* the file does not exist create it */
                return createHDDImage(hdd);
            }
            return true;
        }

        return false;
    }
//----------------------------------------------------------------------------------------
    bool RosBootTest::configureCDImage()
    {
        if (!m_BootCmd.length ())
        {
            if (m_CDImage.length())
            {
                /* we have a cd image lets check if its valid */
                if (isFileExisting(m_CDImage))
                {
                    cerr << "Using CDROM image " << m_CDImage << endl;
                    return true;
                }
            }
            if (isFileExisting("ReactOS-RegTest.iso"))
            {
                m_CDImage = "ReactOS-RegTest.iso";
                cerr << "Falling back to default CDROM image " << m_CDImage << endl;
                return true;
            }
            cerr << "No CDROM image found, boot device is HDD" << endl;
            m_CDImage = "";
            return true;
        }
        
        string::size_type pos = m_BootCmd.find("-boot ");
        if (pos == string::npos)
        {
            /* ROS_BOOT_CMD must provide a boot parameter*/
            cerr << "Error: ROS_BOOT_CMD misses boot parameter" << endl;
            return false;
        }

        string rest = m_BootCmd.substr(pos + 6, m_BootCmd.length() - pos - 6);
        if (rest.length() < 1)
        {
            /* boot parameter misses option where to boot from */
            cerr << "Error: ROS_BOOT_CMD misses boot parameter" << endl;
            return false;
        }
        if (rest[0] != 'c' && rest[0] != 'd')
        {
            cerr << "Error: ROS_BOOT_CMD has invalid boot parameter" << endl;
            return false;
        }

        if (rest[0] == 'c')
        {
            /* ROS_BOOT_CMD boots from hdd */
            return true;
        }

        pos = m_BootCmd.find("-cdrom ");
        if (pos == string::npos)
        {
            cerr << "Error: ROS_BOOT_CMD misses cdrom parameter" << endl;
            return false;
        }
        rest = m_BootCmd.substr(pos + 7, m_BootCmd.length() - pos - 7);
        if (!rest.length())
        {
            cerr << "Error: ROS_BOOT_CMD misses cdrom parameter" << endl;
            return false;
        }
        pos = rest.find(" ");
        if (pos != string::npos)
        {
            rest = rest.substr(0, pos);
        }
        
        if (!isFileExisting(rest))
        {
            cerr << "Error: cdrom image " << rest << " does not exist" << endl;
            return false;
        }
        else
        {
            m_CDImage = rest;
            return true;
        }
    }
//----------------------------------------------------------------------------------------
    bool RosBootTest::configureQemu()
    {
        if (!isQemuPathValid())
        {
            cerr << "Error: the path to qemu is not valid" << endl;
            return false;
        }

        if (!configureCDImage())
        {
            cerr << "Error: failed to set a valid cdrom configuration" << endl;
            return false;
        }

        if (!configureHDDImage())
        {
            cerr << "Error: failed to set a valid hdd configuration" << endl;
            return false;
        }

        if (m_BootCmd.length())
        {
            if (!extractPipeFromBootCmd())
            {
                return false;
            }
        }
        else
        {
            if (!createBootCmd())
            {
                return false;
            }
        }
        if (m_PidFile.length () && isFileExisting(m_PidFile))
        {
            cerr << "Deleting pid file " << m_PidFile << endl;
            remove(m_PidFile.c_str ());
        }

        cerr << "Opening Data Source:" << m_BootCmd << endl;
        m_DataSource = new NamedPipeReader();
        if (!executeBootCmd())
        {
            cerr << "Error: failed to launch emulator with: " << m_BootCmd << endl;
            return false;
        }
        
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
#if 0
        if (!conf_parser.getStringValue(RosBootTest::ROS_EMU_TYPE, m_EmuType))
        {
            cerr << "Error: ROS_EMU_TYPE is not set" << endl;
            return false;
        }
#endif

        if (!conf_parser.getStringValue(RosBootTest::ROS_EMU_PATH, m_EmuPath))
        {
            cerr << "Error: ROS_EMU_PATH is not set" << endl;
            return false;
        }
        if (!m_HDDImage.length())
        {
            /* only read image once */
            conf_parser.getStringValue (RosBootTest::ROS_HDD_IMAGE, m_HDDImage);
        }
        if (!m_CDImage.length())
        {
            /* only read cdimage once */
            conf_parser.getStringValue (RosBootTest::ROS_CD_IMAGE, m_CDImage);
        }
        /* reset boot cmd */
        m_BootCmd = "";


        conf_parser.getIntValue (RosBootTest::ROS_MAX_TIME, m_MaxTime);
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
        OsSupport::delayExecution(3);

        if (m_Pid)
        {
			OsSupport::terminateProcess (m_Pid, 0);
        }
		delete m_DataSource;
        m_DataSource = NULL;

        if (m_PidFile.length ())
        {
            remove(m_PidFile.c_str ());
        }
	}
    
//---------------------------------------------------------------------------------------
	bool RosBootTest::execute(ConfigParser &conf_parser) 
	{
		if (!readConfigurationValues(conf_parser))
        {
            return false;
        }
#if 0
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
#else
        if (!configureQemu())
        {
            cerr << "Error: failed to configure qemu" << endl;
            return false;
        }
#endif

        if (m_DelayRead)
        {
            cerr << "Delaying read for " << m_DelayRead << " seconds" << endl;
            OsSupport::delayExecution(m_DelayRead); 
        }

        if (!m_DataSource->openSource(m_Src))
        {
            cerr << "Error: failed to open data source with " << m_Src << endl;
            cleanup();
            return false;
        }
#ifdef __LINUX__
        /*
         * For linux systems we can only
         * check if the emulator runs by
         * opening pid.txt and lookthrough if
         * it exists
         */

        FILE * file = fopen(m_PidFile.c_str(), "r");
        if (!file)
        {
            cerr << "Error: failed to launch emulator" << endl;
            cleanup();
            return false;
        }
        char buffer[128];
        if (!fread(buffer, 1, sizeof(buffer), file))
        {
            cerr << "Error: pid file w/o pid!!! " << endl;
            fclose(file);
            cleanup();
            return false;
        }
        m_Pid = atoi(buffer);
        fclose(file);
#endif
        OsSupport::cancelAlarms();
#ifdef __LINUX__
        //OsSupport::setAlarm (m_MaxTime, m_Pid);
        //OsSupport::setAlarm(m_MaxTime, getpid());
#else
        OsSupport::setAlarm (m_MaxTime, m_Pid);
        OsSupport::setAlarm(m_MaxTime, GetCurrentProcessId());
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
				if (!strncmp(line.c_str (), m_Checkpoint.c_str (), m_Checkpoint.length ()))
				{
					state = DebugStateCPReached;
					break;
				}
				m_Checkpoints.push_back (line);
			}


			if (line.find ("*** Fatal System Error") != string::npos)
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
					cerr << "----------------------------------" << endl;
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
					cerr << "----------------------------------" << endl;
				}
				state = DebugStateBSODDetected;
				break;
			}
			else if (line.find ("Unhandled exception") != string::npos)
			{
				if (m_CriticalImage == "IGNORE")
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
				string::size_type pos = address.find_last_of (" ");
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
				
				pos = modulename.find_last_of ("\\");
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

				pos = appname.find_last_of (".");
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
	bool RosBootTest::analyzeDebugData()
	{
		vector<string> vect;
		size_t lines = 0;
		bool write_log;
		ofstream file;
        bool ret = true;

        if (m_DebugFile.length ())
		{
			remove(m_DebugFile.c_str ());
			file.open (m_DebugFile.c_str ());
		}

		write_log = file.is_open ();
		while(1)
		{
			size_t prev_count = vect.size ();
			if (!m_DataSource->readSource (vect))
			{
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
		if (write_log)
		{
			file.close();
		}

        return ret;
	}
} // end of namespace Sysreg_
