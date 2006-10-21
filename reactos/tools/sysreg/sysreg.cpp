
/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     app initialization
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */


#include "sysreg.h"

#ifdef UNICODE

	using std::wcerr;
	using std::endl;
	using std::wcout;

#define cerr wcerr
#define cout wcout

#else

	using std::cerr;
	using std::cout;
	using std::endl;

#endif


typedef std::basic_string<TCHAR> string;


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
		cerr << "Error: the requested regression test does not exist" << endl;
		return -1;
	}
	
	string envvar;
	string ros = _T("ROS_OUTPUT");
	config.getStringValue (ros, envvar);

	SymbolFile::initialize (config, envvar);

	if (regtest->execute (config))
	{
		cout << "The regression test " << regtest->getName () << " completed successfully" << endl;
	}
	else
	{
		cout << "The regression test " << regtest->getName () << " failed" << endl;
	}

	return 0;
}
