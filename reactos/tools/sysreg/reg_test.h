#ifndef REG_TEST_H__
#define REG_TEST_H__

/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     regression test abstract class
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */

#include "user_types.h"
#include "conf_parser.h"

namespace Sysreg_
{
	using Sysreg_::ConfigParser;
//-------------------------------------------------------------------
///
/// class RegressionTest
///
/// Description: The class RegressionTest is an abstract base class. Every
/// regression test class derives from this class.


	class RegressionTest
	{
	public:

//---------------------------------------------------------------------------------------
///
/// RegressionTest
///
/// Description: constructor of class RegressionTest
///
/// @param RegName name of derriving subclass

		RegressionTest(const string RegName) : m_Name(RegName)
		{}

//---------------------------------------------------------------------------------------
///
/// ~RegressionTest
///
/// Description: destructor of class RegressionTest

		virtual ~RegressionTest()
		{}

//---------------------------------------------------------------------------------------
///
/// getName
///
/// Description: returns the value of the member m_Name

		const string & getName() const
		{ return m_Name; }

//---------------------------------------------------------------------------------------
///
/// execute
///
/// Description: the execute function passes an System_::ConfigParser object to the
/// derriving RegresssionTest class. The class can then read required options from the
/// configuration file. The RegressionTest shall then perform its specific regression test.
/// @see System_::PipeReader
/// @see System_::SymbolFile
///
/// Note :	if an error happens or the option cannot be found, this function
///         should return false.

	virtual bool execute(ConfigParser & conf_parser) = 0;

//---------------------------------------------------------------------------------------
///
/// checkForHang
///
///
///




	protected:
		string m_Name;

	}; // end of class RegressionTest


} // end of namespace Sysreg_

#endif
