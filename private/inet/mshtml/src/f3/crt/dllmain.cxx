#ifdef __cplusplus
extern "C" {
#endif

extern CRITICAL_SECTION s_cs;
extern CRITICAL_SECTION g_csHeap;
extern void __cdecl _cinit();          /* crt0dat.c */
extern void __cdecl _cexit();
extern int  __cdecl __sbh_process_detach();
extern int _C_Termination_Done;        /* termination done flag */
extern int __proc_attached;
extern HANDLE g_hProcessHeap;
extern BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved);

BOOL WINAPI DLL_MAIN_FUNCTION_NAME(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
    BOOL retcode = TRUE;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            __proc_attached++;

            InitializeCriticalSection(&s_cs);
            InitializeCriticalSection(&g_csHeap);
            g_hProcessHeap = GetProcessHeap();

            DLL_MAIN_PRE_CINIT

#ifndef UNIX
            _cinit();               /* do C data initialize */
#endif
            
            retcode = DllMain(hDllHandle, dwReason, lpreserved);

            break;
        }

        case DLL_PROCESS_DETACH:
        {
            if (__proc_attached <= 0)
            {
                /*
                 * no prior process attach notification. just return
                 * without doing anything.
                 */
                return FALSE;
            }

            DLL_MAIN_PRE_CEXIT

            __proc_attached--;

            retcode = DllMain(hDllHandle, dwReason, lpreserved);

#ifndef UNIX
            if (_C_Termination_Done == FALSE)
            {
                _cexit();
            }
#endif

            DLL_MAIN_POST_CEXIT

#if DBG==1
            if (!__sbh_process_detach())
            {
				char ach[1024];
                wsprintfA(ach, "Small block heap not empty at DLL_PROCESS_DETACH\r\nFile: %s; Line %ld\r\n", __FILE__, __LINE__);
				OutputDebugStringA(ach);
            }
#else
            __sbh_process_detach();
#endif

            DeleteCriticalSection(&g_csHeap);
            DeleteCriticalSection(&s_cs);

            break;
        }

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        {
            retcode = DllMain(hDllHandle, dwReason, lpreserved);
            break;
        }
    }

    return retcode;
}

#ifdef __cplusplus
}
#endif
