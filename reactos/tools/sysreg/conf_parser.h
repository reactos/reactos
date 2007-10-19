#ifndef CONF_PARSER_H__
#define CONF_PARSER_H__

/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     configuration parser
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */

#include "user_types.h"
#include <map>

namespace Sysreg_
{
	using std::map;
//---------------------------------------------------------------------------------------
///
/// class ConfigParser
///
/// Description: this class reads configuration entries from a configuration file
///              Each entry must have the form of VALUE=XXX. The entries are stored
///              in a map which can be queried after parsing the configuration file
///
/// Note: lines beginning with an ; are ignored
///
///	Usage: First, call parseFile with param to file. Finally, call getValue with the
///        appropiate type

	class ConfigParser
	{
	public:

	typedef map<string, string> ConfigMap;
//---------------------------------------------------------------------------------------
///
/// ConfigParser
///
/// Description: constructor of class ConfigParser

	ConfigParser();

//---------------------------------------------------------------------------------------
///
/// ~ConfigParser
///
/// Description: destructor of class ConfigParser

	virtual ~ConfigParser();

//---------------------------------------------------------------------------------------
///
/// parseFile
///
/// Description: this function takes as param a path to file. it attempts to parse this specific
/// file with the given syntax. On success it returns true.
///
/// Note: Everytime parseFile is called, the previous stored values are cleared
///
/// @param FileName path to configuration file
/// @return bool

	bool parseFile(char * FileName);

//--------------------------------------------------------------------------------------
///
/// getStringValue
///
/// Description: attempts to read a config variable from the configuration file
///              If the variable name is not found, it returns false and the param
///              ConfValue remains untouched. On success it returns the true.
///
/// @param ConfVariable name of the configuration variable to retrieve
/// @param ConfValue type of value to retrieve

	bool getStringValue(string & ConfVariable, string & ConfValue);

    bool getDoubleValue(string ConfVariable, double & value);
    bool getIntValue(string ConfVariable, long int & value);

	protected:
		ConfigMap m_Map;

	}; // end of class ConfigParser


} // end of namspace Sysreg_


#endif /* end of CONF_PARSER_H__ */
