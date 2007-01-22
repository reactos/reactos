#ifndef ROSBOOT_TEST_H__
#define ROSBOOT_TEST_H__

/* $Id$
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     ReactOS boot test
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */


#include "reg_test.h"
#include <vector>
#ifndef WIN32
#include <unistd.h>
#define _sleep sleep
#endif

namespace Sysreg_
{
	using std::vector;

//---------------------------------------------------------------------------------------
///
/// class RosBootTest
///
/// Description: this class attempts to boot ReactOS in an emulator with console logging enabled.
/// It 

	class RosBootTest : public RegressionTest
	{
	public:
		static string VARIABLE_NAME;
		static string CLASS_NAME;
		static string DEBUG_PORT;
		static string DEBUG_FILE;
		static string TIME_OUT;
		static string PID_FILE;
		static string CHECK_POINT;
		static string SYSREG_CHECKPOINT;
		static string DELAY_READ;
		static string CRITICAL_APP;

//---------------------------------------------------------------------------------------
///
/// RosBootTest
///
/// Description: constructor of class RosBootTest
///

		RosBootTest();

//---------------------------------------------------------------------------------------
///
/// ~RosBootTest
///
/// Description: destructor of class RosBootTest
///

		virtual ~RosBootTest();

//---------------------------------------------------------------------------------------
///
/// execute
///
/// Description: this function performs a ReactOS boot test. It reads the variable
/// ROSBOOT_CMD and executes this specific command. This command shall contain the path
/// to an emulator (i.e. qemu) and the required arguments (-serial switch, hdd img etc) to be able
/// to read from console. If an error is detected, it attempts to resolve the faulting
/// module and address.

	virtual bool execute(ConfigParser & conf_parser);

	protected:
//---------------------------------------------------------------------------------------
///
/// fetchDebugByPipe
///
/// Description: this functions debugs ReactOS by PipeReader class
///
/// Note: if an error occurs, this function returns false
///
/// @param BootCmd the command which is passed to PipeReader class
/// @return bool

	bool fetchDebugByPipe(string BootCmd);

//---------------------------------------------------------------------------------------
///
/// fetchDebugByFile
///
/// Description: this functions fetches debug info by reading a debug log
///
/// Note: if an error occurs, this function returns false
///
/// @param BootCmd the command which is passed to PipeReader class
/// @return bool

	bool fetchDebugByFile(string BootCmd);

//---------------------------------------------------------------------------------------
///
/// dumpCheckpoints
///
/// Description: prints a list of all reached checkpoints so far

	void dumpCheckpoints();

typedef enum DebugState
{
	DebugStateContinue = 1, /* continue debugging */
	DebugStateBSODDetected, /* bsod detected */
	DebugStateUMEDetected, /* user-mode exception detected */
	DebugStateCPReached /* check-point reached */
};

//---------------------------------------------------------------------------------------
///
/// checkDebugData
///
/// Description: this function parses the given debug data for BSOD, UM exception etc
///              If it detects an fatal error, it should return false
///
/// Note: the received debug information should be written to an internal log object
/// to facilate post-processing of the results

	DebugState checkDebugData(vector<string> & debug_data);

//---------------------------------------------------------------------------------------
///
/// checkTimeOut
///
/// Description: this function checks if the ReactOS has run longer than the maximum available
/// time

	bool isTimeout(double max_timeout);

protected:

	double m_Timeout;
	string m_PidFile;
	string m_Checkpoint;
	string m_CriticalApp;
	string m_DebugFile;
	vector <string> m_Checkpoints;
	unsigned long m_Delayread;

	}; // end of class RosBootTest

} // end of namespace Sysreg_

#endif /* end of ROSBOOT_H__ */
