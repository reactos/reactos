/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     environment variable lookup
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */


#include "env_var.h"
#include <iostream>

namespace System_
{

	EnvironmentVariable::EnvironmentMap EnvironmentVariable::m_Map;

//---------------------------------------------------------------------------------------
	EnvironmentVariable::EnvironmentVariable()
	{

	}

//---------------------------------------------------------------------------------------
	EnvironmentVariable::~EnvironmentVariable()
	{
		m_Map.clear ();
	}

//---------------------------------------------------------------------------------------

	bool EnvironmentVariable::getValue( string const &EnvName, string &EnvValue)
	{
		EnvironmentMap::const_iterator it = m_Map.find (EnvName);
		if (it != m_Map.end())
		{
			EnvValue = it->second;
			return true;
		}

		char * value = getenv(EnvName.c_str ());

		if (!value)
		{
			cerr << "EnvironmentVariable::getValue found no value for " << EnvName << endl;
			return false;
		}

		if (!strlen(value))
		{
			cerr << "EnvironmentVariable::getValue found no value for " << EnvName << endl;
			return false;
		}

		EnvValue = value;
		m_Map[EnvName] = EnvValue;
		return true;
	}


} // end of namespace System_
