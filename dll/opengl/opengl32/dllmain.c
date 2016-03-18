/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 dll/opengl/opengl32/dllmain.c
 * PURPOSE:              OpenGL32 DLL
 */

#include "opengl32.h"

#ifdef OPENGL32_USE_TLS
DWORD OglTlsIndex = 0xFFFFFFFF;
#endif

BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD Reason, LPVOID Reserved)
{
#ifdef OPENGL32_USE_TLS
    struct Opengl32_ThreadData* ThreadData;
#endif
    switch ( Reason )
    {
        case DLL_PROCESS_ATTACH:
#ifdef OPENGL32_USE_TLS
            OglTlsIndex = TlsAlloc();
            if(OglTlsIndex == TLS_OUT_OF_INDEXES)
                return FALSE;
#endif
            /* Initialize Context list */
            InitializeListHead(&ContextListHead);
            /* no break */
        case DLL_THREAD_ATTACH:
#ifdef OPENGL32_USE_TLS
            ThreadData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ThreadData));
            if(!ThreadData)
                return FALSE;
            TlsSetValue(OglTlsIndex, ThreadData);
            ThreadData->glDispatchTable = &StubTable.glDispatchTable;
            ThreadData->hglrc = NULL;
            ThreadData->hdc = NULL;
            ThreadData->dc_data = NULL;
#else
            NtCurrentTeb()->glTable = &StubTable.glDispatchTable;
#endif // defined(OPENGL32_USE_TLS)
            break;

        case DLL_THREAD_DETACH:
            /* Set NULL context for this thread */
            wglMakeCurrent(NULL, NULL);
            /* Clean up */
#ifdef OPENGL32_USE_TLS
            ThreadData = TlsGetValue(OglTlsIndex);
            if(ThreadData)
                HeapFree(GetProcessHeap(), 0, ThreadData);
#else
            NtCurrentTeb()->glTable = NULL;
#endif // defined(OPENGL32_USE_TLS)
        break;
        case DLL_PROCESS_DETACH:
            /* Clean up */
            if (!Reserved)
            {
                /* The process is not shutting down: release everything */
                wglMakeCurrent(NULL, NULL);
                IntDeleteAllContexts();
                IntDeleteAllICDs();
#ifdef OPENGL32_USE_TLS
                ThreadData = TlsGetValue(OglTlsIndex);
                if(ThreadData)
                    HeapFree(GetProcessHeap(), 0, ThreadData);
#endif
            }
#ifdef OPENGL32_USE_TLS
            TlsFree(OglTlsIndex);
#endif
            break;
    }

    return TRUE;
}
