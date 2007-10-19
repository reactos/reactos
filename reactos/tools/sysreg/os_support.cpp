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

    //int gettimeofday(struct timeval *tv, void * tz);
//------------------------------------------------------------------------
    void OsSupport::checkAlarms()
    {
        struct timeval tm;
		size_t i;
#if 0
//        gettimeofday(&tm, 0);
#endif
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
    bool OsSupport::hasAlarms ()
    {
        return (s_Entries.size () != 0);
    }
//------------------------------------------------------------------------
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
#if 0
__inline int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    FILETIME        ft;
    LARGE_INTEGER   li;
    __int64         t;
    static int      tzflag;

    if (tv)
    {
        GetSystemTimeAsFileTime(&ft);
        li.LowPart  = ft.dwLowDateTime;
        li.HighPart = ft.dwHighDateTime;
        t  = li.QuadPart;       /* In 100-nanosecond intervals */
        t -= EPOCHFILETIME;     /* Offset to the Epoch time */
        t /= 10;                /* In microseconds */
        tv->tv_sec  = (long)(t / 1000000);
        tv->tv_usec = (long)(t % 1000000);
    }

    if (tz)
    {
        if (!tzflag)
        {
            _tzset();
            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}
#endif
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

	OsSupport::ProcessID OsSupport::createProcess(char *procname, int procargsnum, char **procargs, bool wait)
	{
		STARTUPINFO siStartInfo;
		PROCESS_INFORMATION piProcInfo;
		OsSupport::ProcessID pid;
        DWORD length = 0;
        char * szBuffer;
        char * cmd;
		ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
		ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

		siStartInfo.cb = sizeof(STARTUPINFO);
		siStartInfo.wShowWindow = SW_SHOWNORMAL;
		siStartInfo.dwFlags = STARTF_USESHOWWINDOW;

        if (procargsnum)
        {
            for (int i = 1; i < procargsnum; i++)
            {
                length += strlen(procargs[i]);
            }

            length += procargsnum;
            szBuffer = (char*)malloc(length * sizeof(char));
            length = 0;
            for (int i = 1; i < procargsnum; i++)
            {
                strcpy(&szBuffer[length], procargs[i]);
                length += strlen(procargs[i]);
                if (i + 1 < procargsnum)
                {
                    szBuffer[length] = ' ';
                }
                else
                {
                    szBuffer[length] = '\0';
                }
                length++;
            }
            length = strlen(procname) + strlen(szBuffer) + 2;
            cmd = (char*)malloc(length * sizeof(char));
            strcpy(cmd, procname);
            strcat(cmd, " ");
            strcat(cmd, szBuffer);
            free(szBuffer);
        }
        else
        {
            cmd = _strdup(procname);

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

        hTimer = CreateWaitableTimer(NULL, TRUE, "SysRegTimer");
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
        return;
        PTIME_ENTRY entry = (PTIME_ENTRY) malloc(sizeof(TIME_ENTRY));
        if (entry)
        {
            cout << "secs: " << secs << endl;
            struct timeval tm;
#if 0
            gettimeofday(&tm, 0);
#else
#endif
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


	struct sigaction OsSupport::s_sact;


	OsSupport::ProcessID OsSupport::createProcess(char *procname, int procargsnum, char **procargs, bool bWait)
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
        else if (sig == SIGCHLD)
        {
            if (OsSupport::hasAlarms())
            {
                ///
                /// FIXME
                ///
                /// there are expiriation alarms active and a child died unexpectly
                /// lets commit suicide
                exit(-2);
            }
        }
    }
    void OsSupport::setAlarm(long secs, OsSupport::ProcessID pid)
    {
        sigemptyset( &s_sact.sa_mask );
        s_sact.sa_flags = 0;
        s_sact.sa_handler = handleSignal;
        sigaction( SIGALRM, &s_sact, NULL );

        PTIME_ENTRY entry = (PTIME_ENTRY) malloc(sizeof(TIME_ENTRY));
        if (entry)
        {
            struct timeval tm;
            gettimeofday(&tm, 0);
            tm.tv_sec += secs;

            entry->tm = tm;
            entry->pid = pid;
            for(size_t i = 0; i < s_Entries.size(); i++)
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
