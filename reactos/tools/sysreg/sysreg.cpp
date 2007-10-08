
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

static const char USAGE[] = 
"sysreg.exe [conf_file]\nconfiguration file (default: sysreg.cfg)";




int main(int argc, char * argv[])
{
	ConfigParser config;
	char DefaultConfig[] = "sysreg.cfg";
	char *ConfigFile;

	if ((argc > 2))
	{
		cerr << USAGE << endl;
		return -1;
	}

//---------------------------------------------------------------------------------------

	if (argc == 2)
	{
		ConfigFile = argv[1];
	}
	else
	{
		ConfigFile = DefaultConfig;
	}

	if (!config.parseFile (ConfigFile))
	{
		cerr << USAGE << endl;
		return -1;
	}

	RosBootTest * regtest = new RosBootTest();
	if (!regtest)
	{
		cerr << "Error: failed to create regression test" << endl;
		return -1;
	}
	
	string envvar;
	string ros = "ROS_OUTPUT";
	config.getStringValue (ros, envvar);
#if 0
	SymbolFile::initialize (config, envvar);
#endif	

    for(int i = 0; i < 3; i++)
    {
        bool ret = regtest->execute(config);
        if (!ret)
        {
            cout << "The regression test has failed at stage: " << i << endl;
            return -2;
        }
    }
	cout << "The regression test completed successfully" << endl;
	return 0;
}
