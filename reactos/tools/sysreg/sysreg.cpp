
/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     app initialization
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */


#include "sysreg.h"

using System_::EnvironmentVariable;

using Sysreg_::ConfigParser;

//regression test classes
using Sysreg_::RosBootTest;

#if 0
using System_::SymbolFile;
#endif

static const TCHAR USAGE[] = 
_T("sysreg.exe -l | [conf_file] <testname>\n\n-l            - list available tests\nconf_file     - (optional) path to a configuration file (default: sysreg.cfg)\ntest_name     - name of test to execute\n");




int _tmain(int argc, TCHAR * argv[])
{
	ConfigParser config;
	TCHAR DefaultConfig[] = _T("sysreg.cfg");
	TCHAR *ConfigFile;
	TCHAR * TestName;

	if ((argc != 3) && (argc != 2))
	{
		cerr << USAGE << endl;
		return -1;
	}

//---------------------------------------------------------------------------------------

	if (argc == 2)
	{
		ConfigFile = DefaultConfig;
		TestName = argv[1];
	}
	else
	{
		ConfigFile = argv[1];
		TestName = argv[2];
	}


	if (!config.parseFile (ConfigFile))
	{
		cerr << USAGE << endl;
		return -1;
	}

	RosBootTest * regtest = new RosBootTest();
	if (!regtest)
	{
		cerr << "Error: the requested regression test does not exist" << endl;
		return -1;
	}
	
	string envvar;
	string ros = _T("ROS_OUTPUT");
	config.getStringValue (ros, envvar);
#if 0
	SymbolFile::initialize (config, envvar);
#endif	
	if (regtest->execute (config))
	{
		cout << "The regression test completed successfully" << endl;
	}
	else
	{
		cout << "The regression test failed" << endl;
		return -2;
	}

	return 0;
}
