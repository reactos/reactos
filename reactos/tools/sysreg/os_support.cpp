/* $Id: os_support.h 24643 2006-10-24 11:45:21Z janderwald $
 *
 * PROJECT:     System regression tool for ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        tools/sysreg/conf_parser.h
 * PURPOSE:     operating systems specific code
 * PROGRAMMERS: Johannes Anderwald (johannes.anderwald at sbox tugraz at)
 */

#include "os_support.h"

namespace System_
{
#ifndef __LINUX__
	bool OsSupport::terminateProcess(OsSupport::ProcessID pid)
	{
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
		if (!hProcess)
		{
			return false;
		}

		bool ret = TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
		return ret;
	}

	OsSupport::ProcessID OsSupport::createProcess(TCHAR *procname, int procargsnum, TCHAR **procargs)
	{
		STARTUPINFO siStartInfo;
		PROCESS_INFORMATION piProcInfo; 
		OsSupport::ProcessID pid;

		UNREFERENCED_PARAMETER(procargsnum);
		UNREFERENCED_PARAMETER(procargs);

		ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
		ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

		siStartInfo.cb = sizeof(STARTUPINFO);
		siStartInfo.wShowWindow = SW_SHOWNORMAL;
		siStartInfo.dwFlags = STARTF_USESHOWWINDOW;

		LPTSTR command = _tcsdup(procname);

		if (!CreateProcess(NULL, procname, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &siStartInfo, &piProcInfo))
		{
			cerr << "Error: CreateProcess failed " << command <<endl;
			pid = 0;
		}
		else
		{
			pid = piProcInfo.dwProcessId;
			CloseHandle(piProcInfo.hProcess);
			CloseHandle(piProcInfo.hThread);
		}
		free(command);
		return pid;
	}
   	void OsSupport::sleep(long value)
    	{
        	Sleep(value);
    	}
#else
/********************************************************************************************************************/
	OsSupport::ProcessID OsSupport::createProcess(TCHAR *procname, int procargsnum, TCHAR **procargs)
	{
		ProcessID pid;

		if ((pid = fork()) < 0)
		{
			cerr << "OsSupport::createProcess> fork failed" << endl;
			return 0;
		}
		if (pid == 0)
		{
			execv(procname, procargs);
			return 0;
		}
		
		return pid;
	}

	bool OsSupport::terminateProcess(OsSupport::ProcessID pid)
	{
		kill(pid, SIGKILL);
		return true;
	}

    	void OsSupport::sleep(long value)
    	{
        	sleep(value);
    	}


#endif

} // end of namespace System_
