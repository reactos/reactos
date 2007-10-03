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

    OsSupport::TimeEntryVector OsSupport::s_Entries;

    void OsSupport::checkAlarms()
    {
        struct timeval tm;
		size_t i;
        gettimeofday(&tm, 0);
        for (i = 0; i < s_Entries.size(); i++)
        {
            long diffsec = s_Entries[i]->tm.tv_sec - tm.tv_sec;
            if (diffsec < 0)
            {
                cout << "terminating process pid:" << s_Entries[i]->pid << endl;
                terminateProcess(s_Entries[i]->pid, -2); 
                free(s_Entries[i]);
                s_Entries.erase (s_Entries.begin () + i);
                i = MAX(0, i-1);
            }
        }

#ifdef __LINUX__
        if (s_Entries.size())
        {
            long secs = s_Entries[i]->tm.tv_sec - tm.tv_sec;
            alarm(secs);
        }
#endif
    }

    void OsSupport::cancelAlarms()
    {

#ifndef __LINUX__
        if (s_hThread)
        {
            TerminateThread(s_hThread, 0);
            s_hThread = 0;
        }
#endif

        for(size_t i = 0; i < s_Entries.size(); i++)
        {
            free(s_Entries[i]);
        }

        s_Entries.clear();


    }


#ifndef __LINUX__

    HANDLE OsSupport::s_hThread = 0;
    static HANDLE hTimer;
	bool OsSupport::terminateProcess(OsSupport::ProcessID pid, int exitcode)
	{
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
		if (!hProcess)
		{
			return false;
		}

		bool ret = TerminateProcess(hProcess, exitcode);
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

    DWORD WINAPI AlarmThread(LPVOID param)
    {
        LARGE_INTEGER   liDueTime;

        hTimer = CreateWaitableTimer(NULL, TRUE, _T("SysRegTimer"));
        if (!hTimer)
        {
            return 0;
        }
        liDueTime.QuadPart = -100000000LL;
        while(1)
        {
            SetWaitableTimer(hTimer, &liDueTime, 5000, NULL, NULL, FALSE);
            WaitForSingleObject(hTimer, INFINITE);
            OsSupport::checkAlarms ();
        }
        return 0;
    }

    void OsSupport::setAlarm(long secs, OsSupport::ProcessID pid)
    {

        PTIME_ENTRY entry = (PTIME_ENTRY) malloc(sizeof(TIME_ENTRY));
        if (entry)
        {
            cout << "secs: " << secs << endl;
            struct timeval tm;
            gettimeofday(&tm, 0);
            tm.tv_sec += secs;

            entry->tm = tm;
            entry->pid = pid;
            s_Entries.push_back(entry);
            if (s_Entries.size () == 1)
            {
                s_hThread = CreateThread(NULL, 0, AlarmThread, 0, 0, NULL);
            }
        }
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
                waitpid(pid, NULL, 0);
            }
        }
		return pid;
	}

	bool OsSupport::terminateProcess(OsSupport::ProcessID pid, int exitcode)
	{
		kill(pid, SIGKILL);
		return true;
	}

    void OsSupport::delayExecution(long value)
    {
        sleep( value );
    }

    void handleSignal(int sig)
    {
        if (sig == SIGALRM)
        {
            OsSupport::checkAlarms();
        }
    }
    void setAlarm(long secs, OsSupport::ProcessID pid)
    {
        sigemptyset( &sact.sa_mask );
        s_sact.sa_flags = 0;
        s_sact.sa_handler = catcher;
        sigaction( SIGALRM, &sact, NULL );

        alarm(timeout);

        PTIME_ENTRY entry = (PTIME_ENTRY) malloc(sizeof(TIME_ENTRY));
        if (entry)
        {
            struct timeval tm;
            gettimeofday(&tm, 0);
            tm.tv_sec += secs;

            entry->tm = tm;
            entry->pid = pid;
            for(int i = 0; i < s_Entries.size(); i++)
            {
                if (tm.tv_sec < s_Entries[i]->tm.tv_sec && tm.tv_usec < s_Entries[i]->tm.tv_usec)
                {
                    if (i == 0)
                    {
                        /* adjust alarm timer to new period */
                        alarm(secs);
                    }
                    s_Entries.insert(s_Entries.begin() + i, entry);
                    return;
                }
            }
            s_Entries.push_back(entry);
            alarm(secs);
        }
    }
#endif

} // end of namespace System_
