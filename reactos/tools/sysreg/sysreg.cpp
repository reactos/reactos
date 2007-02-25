
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
using System_::ComponentFactoryTemplate;

using Sysreg_::ConfigParser;

//regression test classes
using Sysreg_::RegressionTest;
using Sysreg_::RosBootTest;

#if 0
using System_::SymbolFile;
#endif

typedef ComponentFactoryTemplate<RegressionTest, string> ComponentFactory;

static const TCHAR USAGE[] = 
_T("sysreg.exe -l | [conf_file] <testname>\n\n-l            - list available tests\nconf_file     - (optional) path to a configuration file (default: sysreg.cfg)\ntest_name     - name of test to execute\n");




int _tmain(int argc, TCHAR * argv[])
{
	ConfigParser config;
	ComponentFactory comp_factory;
	TCHAR DefaultConfig[] = _T("sysreg.cfg");
	TCHAR *ConfigFile;
	TCHAR * TestName;

	if ((argc != 3) && (argc != 2))
	{
		cerr << USAGE << endl;
		return -1;
	}

//---------------------------------------------------------------------------------------
	/// regression tests should be registered here
	comp_factory.registerComponent<RosBootTest>(RosBootTest::CLASS_NAME);

//---------------------------------------------------------------------------------------

	if (argc == 2)
	{
		if (_tcscmp(argv[1], _T("-l")) == 0)
		{
			comp_factory.listComponentIds();
			return -1;
		}
	}

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

	RegressionTest * regtest = comp_factory.createComponent (TestName);
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
		cout << "The regression test " << regtest->getName () << " completed successfully" << endl;
	}
	else
	{
		cout << "The regression test " << regtest->getName () << " failed" << endl;
		return -2;
	}

	return 0;
}
