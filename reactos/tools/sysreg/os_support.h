#ifndef OS_SUPPORT_H__
#define OS_SUPPORT_H__

/* $Id: os_support.h 24643 2006-10-24 11:45:21Z janderwald $
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     operating systems specific code
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */

#ifndef __LINUX__
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#endif

#include "user_types.h"
#include <ctime>
#include <vector>
#include <sys/time.h>

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

namespace System_
{
//---------------------------------------------------------------------------------------
///
/// class OsSupport
///
/// Description: this class encapsulates operating system specific functions
///
///

	class OsSupport
	{
	public:
#ifdef WIN32
		typedef DWORD ProcessID;
#else
		typedef pid_t ProcessID;
#endif

//---------------------------------------------------------------------------------------
///
/// OsSupport
///
/// Description: constructor of class OsSupport

		virtual ~OsSupport()
		{}

//---------------------------------------------------------------------------------------
///
/// createProcess
///
/// Description: this functions creates a new process and returns its pid on success
///
/// @param procname name of the file to execute
/// @param procargsnum num of arguments for the new process
/// @param procargs arguments for the new process
///
///

		static ProcessID createProcess(TCHAR * procname, int procargsnum, TCHAR ** procargs, bool wait);

//---------------------------------------------------------------------------------------
///
/// terminateProcess
///
/// Description: this function terminates a process given by its pid
///
/// Note: returns true if the process with the given pid was terminated
///
/// @param pid process id of the process to terminate
/// @param exitcode for the killed process

	static bool terminateProcess(ProcessID pid, int exitcode);


//----------------------------------------------------------------------------------------
///
/// delayExecution
///
/// Description: this function sleeps the current process for the amount given in seconds
///
/// @param sec amount of seconds to sleep

    static void delayExecution(long sec);

///---------------------------------------------------------------------------------------
///
/// initializeTimer
///
/// Description: this function initializes an alarm. When the alarm expires, it will terminate
/// the given process

    static void setAlarm(long sec, OsSupport::ProcessID pid);

///---------------------------------------------------------------------------------------
///
/// cancelAlarms
///
/// Description: this function cancels all available timers

    static void cancelAlarms();

///---------------------------------------------------------------------------------------
///
/// checkAlarms
///
/// Description: this function checks for expired alarams

    static void checkAlarms();


	protected:
//---------------------------------------------------------------------------------------
///
/// OsSupport
///
/// Description: constructor of class OsSupport

		OsSupport()
		{}

#ifdef __LINUX__
        static struct sigaction s_sact;
#else
        static HANDLE s_hThread;
#endif

     typedef struct
     {
        struct timeval tm;
        ProcessID pid;
     }TIME_ENTRY, *PTIME_ENTRY;
     typedef std::vector<PTIME_ENTRY> TimeEntryVector;
     static TimeEntryVector s_Entries;
	}; // end of class OsSupport

} // end of namespace System_

#endif /* end of OS_SUPPORT_H__ */
