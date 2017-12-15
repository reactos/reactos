/*
 * PROJECT:     Dr. Watson crash reporter
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Project header
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once


struct ModuleData
{
    std::string ModuleName;
    void *BaseAddress;
    DWORD Size;
    bool Unloaded;


    ModuleData(void* addr);
    void Update(HANDLE hProcess);
};

struct ThreadData
{
    HANDLE Handle;
    CONTEXT Context;

    ThreadData(HANDLE handle = NULL);

    void Update();
};

typedef std::vector<ModuleData> ModuleList;
typedef std::map<DWORD, ThreadData> ThreadMap;

class DumpData
{
public:
    std::string ProcessPath;
    std::string ProcessName;
    DWORD ProcessID;
    DWORD ThreadID;
    HANDLE ProcessHandle;
    ModuleList Modules;
    ThreadMap Threads;
    EXCEPTION_DEBUG_INFO ExceptionInfo;
    HANDLE Event;
    bool FirstBPHit;

    DumpData();
};

#define NEWLINE "\r\n"

/* main.cpp */
void xfprintf(FILE* stream, const char *fmt, ...);

/* drwtsn32.cpp */
bool UpdateFromEvent(DEBUG_EVENT& evt, DumpData& data);

/* sysinfo.cpp */
void PrintSystemInfo(FILE* output, DumpData& data);

/* stacktrace.cpp */
void BeginStackBacktrace(DumpData& data);
void PrintStackBacktrace(FILE* output, DumpData& data, ThreadData& thread);
void EndStackBacktrace(DumpData& data);

