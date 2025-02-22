/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 dll/opengl/opengl32/dllmain.c
 * PURPOSE:              OpenGL32 DLL
 */

#include "opengl32.h"

BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD Reason, LPVOID Reserved)
{
    switch ( Reason )
    {
        case DLL_PROCESS_ATTACH:
            /* Initialize Context list */
            InitializeListHead(&ContextListHead);
            /* no break */
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            /* Set NULL context for this thread */
            wglMakeCurrent(NULL, NULL);
        break;
        case DLL_PROCESS_DETACH:
            /* Clean up */
            if (!Reserved)
            {
                /* The process is not shutting down: release everything */
                wglMakeCurrent(NULL, NULL);
                IntDeleteAllContexts();
                IntDeleteAllICDs();
            }
            break;
    }

    return TRUE;
}
