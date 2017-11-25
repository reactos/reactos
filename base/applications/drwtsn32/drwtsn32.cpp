#include "precomp.h"
#include <psapi.h>

ModuleData::ModuleData(void* addr)
{
    BaseAddress = addr;
    Size = 0;
    Unloaded = false;
}

void ModuleData::Update(HANDLE hProcess)
{
    MODULEINFO mi = {0};
    GetModuleInformation(hProcess, (HMODULE)BaseAddress, &mi, sizeof(mi));
    assert(BaseAddress == mi.lpBaseOfDll);
    Size = mi.SizeOfImage;

    ModuleName.resize(MAX_PATH);
    DWORD dwLen = GetModuleFileNameExA(hProcess, (HMODULE)BaseAddress, &ModuleName[0], ModuleName.size());
    ModuleName.resize(dwLen);
}


ThreadData::ThreadData(HANDLE handle = NULL)
    : Handle(handle)
{
    memset(&Context, 0, sizeof(Context));
}

void ThreadData::Update()
{
    Context.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL | CONTEXT_DEBUG_REGISTERS;
    GetThreadContext(Handle, &Context);
}

DumpData::DumpData()
    :ProcessID(0)
    ,ProcessHandle(NULL)
{
    memset(&ExceptionInfo, 0, sizeof(ExceptionInfo));
}


bool UpdateFromEvent(DEBUG_EVENT& evt, DumpData& data)
{
    switch(evt.dwDebugEventCode)
    {
    case CREATE_PROCESS_DEBUG_EVENT:
    {
        data.ProcessPath.resize(MAX_PATH);
        DWORD len = GetModuleFileNameExA(evt.u.CreateProcessInfo.hProcess, NULL, &data.ProcessPath[0], data.ProcessPath.size());
        if (len)
        {
            data.ProcessPath.resize(len);
            std::string::size_type pos = data.ProcessPath.find_last_of("\\/");
            if (pos != std::string::npos)
                data.ProcessName = data.ProcessPath.substr(pos+1);
        }
        else
        {
            data.ProcessPath = "??";
        }
        if (data.ProcessName.empty())
            data.ProcessName = data.ProcessPath;

        CloseHandle(evt.u.CreateProcessInfo.hFile);
        data.ProcessID = evt.dwProcessId;
        data.ProcessHandle = evt.u.CreateProcessInfo.hProcess;
    }
        break;
    case CREATE_THREAD_DEBUG_EVENT:
        data.Threads[evt.dwThreadId] = ThreadData(evt.u.CreateThread.hThread);
        break;
    case EXIT_THREAD_DEBUG_EVENT:
    {
        ThreadMap::iterator it = data.Threads.find(evt.dwThreadId);
        if (it != data.Threads.end())
        {
            data.Threads.erase(it);
        }
    }
        break;
    case LOAD_DLL_DEBUG_EVENT:
        CloseHandle(evt.u.LoadDll.hFile);
        for (size_t n = 0; n < data.Modules.size(); ++n)
        {
            if (data.Modules[n].BaseAddress == evt.u.LoadDll.lpBaseOfDll)
            {
                data.Modules[n].Unloaded = false;
                return true;
            }
        }
        data.Modules.push_back(ModuleData(evt.u.LoadDll.lpBaseOfDll));
        break;
    case UNLOAD_DLL_DEBUG_EVENT:
        for (size_t n = 0; n < data.Modules.size(); ++n)
        {
            if (data.Modules[n].BaseAddress == evt.u.UnloadDll.lpBaseOfDll)
                data.Modules[n].Unloaded = true;
        }
        break;
    case OUTPUT_DEBUG_STRING_EVENT: // ignore
        break;
    case EXCEPTION_DEBUG_EVENT:
        data.ExceptionInfo = evt.u.Exception;
        return false;
    case EXIT_PROCESS_DEBUG_EVENT:
        //assert(FALSE);
        return false;
    case RIP_EVENT:
        //assert(FALSE);
        return false;
    default:
        assert(false);
    }
    return true;
}

