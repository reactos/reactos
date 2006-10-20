
/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     app initialization
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */


#include "sysreg.h"


typedef std::basic_string<TCHAR> string;

using std::cout;
using std::endl;
using std::cerr;


using System_::EnvironmentVariable;
using System_::ComponentFactoryTemplate;

using Sysreg_::ConfigParser;

//regression test classes
using Sysreg_::RegressionTest;
using Sysreg_::RosBootTest;

using System_::SymbolFile;

typedef ComponentFactoryTemplate<RegressionTest, string> ComponentFactory;

static const TCHAR USAGE[] = 
_T("sysreg.exe <conf_file> <testname>\n\nconf_file           ... path to a configuration file\ntest_name           ... name of test to execute\n");




int _tmain(int argc, TCHAR * argv[])
{
	ConfigParser config;
	ComponentFactory comp_factory;
	
	if (argc != 3)
	{
		cerr << USAGE << endl;
		return -1;
	}
//---------------------------------------------------------------------------------------
	/// regression tests should be registered here
	comp_factory.registerComponent<RosBootTest>(RosBootTest::CLASS_NAME);

//---------------------------------------------------------------------------------------

	if (!config.parseFile (argv[1]))
	{
		cerr << USAGE << endl;
		return -1;
	}

	RegressionTest * regtest = comp_factory.createComponent (argv[2]);
	if (!regtest)
	{
		_tprintf(_T("Error: the requested regression test does not exist"));
		return -1;
	}
	
	string envvar;
	string ros = _T("ROS_OUTPUT");
	config.getStringValue (ros, envvar);

	SymbolFile::initialize (config, envvar);

	if (regtest->execute (config))
	{
		_tprintf(_T("The regression test %s completed successfully\n"), regtest->getName ().c_str ());
	}
	else
	{
		_tprintf(_T("The regression test %s failed\n"), regtest->getName ().c_str ());
	}

	return 0;
}