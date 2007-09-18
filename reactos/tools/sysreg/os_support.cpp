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

	OsSupport::ProcessID OsSupport::createProcess(TCHAR *procname, int procargsnum, TCHAR **procargs, bool wait)
	{
		STARTUPINFO siStartInfo;
		PROCESS_INFORMATION piProcInfo; 
		OsSupport::ProcessID pid;
        DWORD length = 0;
        TCHAR * szBuffer;
        TCHAR * cmd;
		ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
		ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

		siStartInfo.cb = sizeof(STARTUPINFO);
		siStartInfo.wShowWindow = SW_SHOWNORMAL;
		siStartInfo.dwFlags = STARTF_USESHOWWINDOW;
        
        if (procargsnum)
        {
            for (int i = 1; i < procargsnum; i++)
            {
                length += _tcslen(procargs[i]);
            }

            length += procargsnum;
            szBuffer = (TCHAR*)malloc(length * sizeof(TCHAR));
            length = 0;
            for (int i = 1; i < procargsnum; i++)
            {
                _tcscpy(&szBuffer[length], procargs[i]);
                length += _tcslen(procargs[i]);
                szBuffer[length] = _T(' ');
                length++;
            }
            length = _tcslen(procname) + _tcslen(szBuffer) + 2;
            cmd = (TCHAR*)malloc(length * sizeof(TCHAR));
            _tcscpy(cmd, procname);
            _tcscat(cmd, _T(" "));
            _tcscat(cmd, szBuffer);
            free(szBuffer);
        }
        else
        {
            cmd = _tcsdup(procname);

        }
		if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &siStartInfo, &piProcInfo))
		{
            cerr << "Error: CreateProcess failed " << cmd << endl;
			pid = 0;
		}
		else
		{
			pid = piProcInfo.dwProcessId;
            if (wait)
            {
                WaitForSingleObject(piProcInfo.hThread, INFINITE);
            }
			CloseHandle(piProcInfo.hProcess);
			CloseHandle(piProcInfo.hThread);
		}
		free(cmd);
		return pid;
	}
   	void OsSupport::delayExecution(long value)
    	{
        	Sleep(value * 1000);
    	}
#else
/********************************************************************************************************************/
	OsSupport::ProcessID OsSupport::createProcess(TCHAR *procname, int procargsnum, TCHAR **procargs, bool bWait)
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
        else
        {
            /* parent process */
            if (bWait)
            {
                waitpid(pid, NULL, WNOHANG);
            }
        }
		return pid;
	}

	bool OsSupport::terminateProcess(OsSupport::ProcessID pid)
	{
		kill(pid, SIGKILL);
		return true;
	}

    	void OsSupport::delayExecution(long value)
    	{
			sleep( value );
    	}


#endif

} // end of namespace System_
