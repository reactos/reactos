#ifndef ENV_VAR_H__
#define ENV_VAR_H__

/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     environment variable lookup
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */

#include "user_types.h"
#include <map>


namespace System_
{
	using std::map;
//---------------------------------------------------------------------------------------
///
/// class EnvironmentVariable
///
/// Description: this class performs looking up environment variable from the system
/// The result is also stored in its internal map to cache results

	class EnvironmentVariable
	{
	public:
		typedef map<string, string> EnvironmentMap;
//---------------------------------------------------------------------------------------
///
/// ~EnvironmentVariable
///
/// Description: destructor of class EnvironmentVariable

		virtual ~EnvironmentVariable();

//---------------------------------------------------------------------------------------
///
/// getValue
///
/// Description: this function returns the value of an associated environment variable. if
/// the variable is set it returns true and the value in the parameter EnvValue. On error
/// it returns false and the param EnvValue remains untouched
///
/// @param EnvName name of environment variable to retrieve
/// @param EnvValue value of the environment variable
/// @return bool

		static bool getValue(const string & EnvName, string & EnvValue);


	protected:

//---------------------------------------------------------------------------------------
///
/// EnvironmentVariable
///
/// Description: constructor of class EnvironmentVariable

		EnvironmentVariable();

		static EnvironmentMap m_Map;

	}; // end of class EnvironmentVariable

} // end of namespace System_

#endif /* end of ENV_VAR_H__ */
