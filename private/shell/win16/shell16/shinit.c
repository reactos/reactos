#include "shprv.h"

#ifdef DEBUG
char ach[20];
void GetTaskName(HTASK hTask, LPSTR pname, int size)
{   _asm {
        push    ds
        les     di,pname
        mov     ds,hTask
        lea     si,ds:[0xF2]
        movsw
        movsw
        movsw
        movsw
        xor     al,al
        stosb
        pop     ds
}}
#endif

typedef struct _TASKINFO {
    struct _TASKINFO   *next;           // next link
    HTASK               hTask;          // task that owns this link
    HMODULE             hShell16;
}   TASKINFO;

TASKINFO *TaskInfoList;
HMODULE hShell16;

//
//  FindTask
//
TASKINFO * NEAR PASCAL FindTask()
{
    TASKINFO *pti;
    HTASK hTask = GetCurrentTask();

    for (pti=TaskInfoList; pti; pti=pti->next)
    {
        if (pti->hTask == hTask)
            return pti;
    }

    pti = (TASKINFO *)LocalAlloc(LPTR, sizeof(TASKINFO));

    if (pti == NULL)
        return NULL;

    pti->next  = TaskInfoList;
    pti->hTask = hTask;
    TaskInfoList = pti;
    return pti;
}

//
//  FreeTask
//
void NEAR PASCAL FreeTask()
{
    TASKINFO *pti, *p;
    HTASK hTask = GetCurrentTask();

    if (TaskInfoList == NULL)
        return;

    if (TaskInfoList->hTask == hTask)
    {
        pti = TaskInfoList;
        TaskInfoList = pti->next;
    }
    else
    {
        for (pti=TaskInfoList; pti; p=pti,pti=pti->next)
        {
            if (pti->hTask == hTask)
                break;
        }

        if (pti == NULL)
            return;

        p->next = pti->next;
    }

    LocalFree((HLOCAL)pti);
}

//
// load the passed library in the context of the calling app, only
// once.
//
HMODULE _loadds NEAR PASCAL LoadShell16()
{
    TASKINFO *pti;

    pti = FindTask();

    if (pti->hShell16 == NULL)
    {
#ifdef DEBUG
        GetTaskName(GetCurrentTask(), ach, sizeof(ach));
        DebugOutput(DBF_WARNING, "SHELL: loading SHELL16 for %ls", (LPSTR)ach);
#endif
        pti->hShell16 = LoadLibrary("SHELL16.DLL");

        //
        // because we do a GetProcAddress() only once per entry point
        // we cant ever *realy* free SHELL16, so we
        // load it a extra time to keep it in memory until our WEP
        //
        if (hShell16 == NULL)
            hShell16 = LoadLibrary("SHELL16.DLL");
    }

    return pti->hShell16;
}

//
//  FreeShell16
//
void NEAR PASCAL FreeShell16()
{
    TASKINFO *pti;

    pti = FindTask();

    if (pti->hShell16)
    {
#ifdef DEBUG
        GetTaskName(GetCurrentTask(), ach, sizeof(ach));
        DebugOutput(DBF_WARNING, "SHELL: freeing SHELL16 for %ls", (LPSTR)ach);
#endif
        FreeLibrary(pti->hShell16);
        pti->hShell16 = NULL;
    }
}

BOOL CALLBACK LibMain(HINSTANCE hinst, UINT wDS, DWORD unused)
{
    return TRUE;
}

BOOL CALLBACK _loadds WEP(BOOL fSystemExit)
{
    if (hShell16)
        FreeLibrary(hShell16);
    hShell16 = NULL;
    return TRUE;
}

#define DLL_PROCESS_ATTACH 1    
#define DLL_PROCESS_DETACH 0    

BOOL FAR PASCAL _loadds DllEntryPoint(DWORD dwReason, WORD  hInst, WORD  wDS, WORD wHeapSize, DWORD dwReserved1, WORD wReserved2)
{
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
#ifdef DEBUG
            GetTaskName(GetCurrentTask(), ach, sizeof(ach));
            DebugOutput(DBF_WARNING, "SHELL: process attach for %ls", (LPSTR)ach);
#endif
            break;

        case DLL_PROCESS_DETACH:
#ifdef DEBUG
            GetTaskName(GetCurrentTask(), ach, sizeof(ach));
            DebugOutput(DBF_WARNING, "SHELL: process detach for %ls", (LPSTR)ach);
#endif
            FreeShell16();
            FreeTask();
            break;
    }

    return TRUE;
}
