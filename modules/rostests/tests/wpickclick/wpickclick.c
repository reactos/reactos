/*----------------------------------------------------------------------------
** wpickclick.c
**  Utilty to pick clicks posted to Wine Windows
**
**
**---------------------------------------------------------------------------
**  Copyright 2004 Jozef Stefanka for CodeWeavers, Inc.
**  Copyright 2005 Francois Gouget for CodeWeavers, Inc.
**  Copyright 2005 Dmitry Timoshkov for CodeWeavers, Inc.
**
**     This program is free software; you can redistribute it and/or modify
**     it under the terms of the GNU General Public License as published by
**     the Free Software Foundation; either version 2 of the License, or
**     (at your option) any later version.
**
**     This program is distributed in the hope that it will be useful,
**     but WITHOUT ANY WARRANTY; without even the implied warranty of
**     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**     GNU General Public License for more details.
**
**     You should have received a copy of the GNU General Public License
**     along with this program; if not, write to the Free Software
**     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
**--------------------------------------------------------------------------*/
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>

#include "hook.h"


#define APP_NAME "wpickclick.exe"


static BOOL (WINAPI *pInstallHooks)(HMODULE hdll);
static void (WINAPI *pRemoveHooks)();
static action_t* (WINAPI *pGetAction)();
static void (WINAPI *pFreeAction)(action_t* action);


/*
 * Provide some basic debugging support.
 */
#ifdef __GNUC__
#define __PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))
#else
#define __PRINTF_ATTR(fmt,args)
#endif
static int debug_on=0;
static int init_debug()
{
    char* str=getenv("CXTEST_DEBUG");
    if (str && strstr(str, "+hook"))
        debug_on=1;
    return debug_on;
}

static void cxlog(const char* format, ...) __PRINTF_ATTR(1,2);
static void cxlog(const char* format, ...)
{
    va_list valist;

    if (debug_on)
    {
        va_start(valist, format);
        vfprintf(stderr, format, valist);
        va_end(valist);
    }
}

static HINSTANCE load_hook_dll()
{
    HINSTANCE hinstDll;
    char dllpath[MAX_PATH];
    char* p;

    hinstDll=LoadLibrary("hook.dll");
    if (hinstDll != NULL)
        return hinstDll;

    if (!GetModuleFileName(NULL,dllpath,sizeof(dllpath)))
        return NULL;

    p=strrchr(dllpath,'\\');
    if (!p)
        return NULL;
    *p='\0';
    p=strrchr(dllpath,'\\');
    if (!p)
        return NULL;
    *p='\0';
    strcat(dllpath,"\\hookdll\\hook.dll");
    hinstDll=LoadLibrary(dllpath);
    return hinstDll;
}

char* cleanup(char* str)
{
    char* s;

    while (*str==' ' || *str=='\t' || *str=='\r' || *str=='\n')
        str++;
    s=strchr(str,'\n');
    if (!s)
        s=str+strlen(str)-1;
    while (s>str && (*s==' ' || *s=='\t' || *s=='\r' || *s=='\n'))
        s--;
    *(s+1)='\0';
    return str;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    HINSTANCE hDll;
    action_t* action;

    init_debug();

    /* Our scripts expect Unix-style line ends */
    _setmode(1,_O_BINARY);
    _setmode(2,_O_BINARY);

    if (strstr(lpCmdLine,"--help"))
    {
        fprintf(stderr,"%s - Utility to print coordinates, component, window title, component class and window class name of a click\n", APP_NAME);
        fprintf(stderr,"----------------------------------------------\n");
        fprintf(stderr,"Usage: %s\n",APP_NAME);
        fprintf(stderr,"The options are as follows:\n");
        fprintf(stderr,"After starting you can\n");
        fprintf(stderr,"select where to click.  If we properly track the click, it will be reported\n");
        fprintf(stderr,"in the following format:\n");
        fprintf(stderr,"    button-name x y component_name window_name component_class_name window_class_name\n");
        fprintf(stderr,"Note that x and y can be negative; this typically happens if you click within the\n");
        fprintf(stderr,"window manager decorations of a given window.\n");
        fprintf(stderr,"On success, %s returns 0, non zero on some failure\n",APP_NAME);
        exit(0);
    };

    /* Load the hook library */
    hDll = load_hook_dll();
    if (!hDll)
    {
        fprintf(stderr, "Error: Unable to load 'hook.dll'\n");
        printf("failed\n");
        return 1;
    }

    pInstallHooks=(void*)GetProcAddress(hDll, "InstallHooks");
    pRemoveHooks=(void*)GetProcAddress(hDll, "RemoveHooks");
    pGetAction=(void*)GetProcAddress(hDll, "GetAction");
    pFreeAction=(void*)GetProcAddress(hDll, "FreeAction");
    if (!pInstallHooks || !pRemoveHooks || !pGetAction)
    {
        fprintf(stderr, "Error: Unable to get the hook.dll functions (%ld)\n",
                GetLastError());
        printf("failed\n");
        return 1;
    }

    if (!pInstallHooks(hDll))
    {
        fprintf(stderr, "Error: Unable to install the hooks (%ld)\n",
                GetLastError());
        printf("failed\n");
        return 1;
    }

    fprintf(stderr, "Ready for capture...\n");
    action=pGetAction();
    if (!action)
    {
        fprintf(stderr, "Error: GetAction() failed\n");
        printf("failed\n");
        return 1;
    }

    switch (action->action)
    {
    case ACTION_FAILED:
        printf("failed\n");
        break;
    case ACTION_NONE:
        printf("none\n");
        break;
    case ACTION_FIND:
        printf("find\n");
        break;
    case ACTION_BUTTON1:
    case ACTION_BUTTON2:
    case ACTION_BUTTON3:
        printf("button%d %ld %ld\n", action->action-ACTION_BUTTON1+1,
               action->x, action->y);
        break;
    default:
        fprintf(stderr, "Error: Unknown action %d\n",action->action);
        printf("%d\n", action->action);
        break;
    }
    printf("%s\n", action->window_class);
    printf("%s\n", action->window_title);
    printf("%ld\n", action->control_id);
    printf("%s\n", action->control_class);
    printf("%s\n", cleanup(action->control_caption));

    cxlog("\n%s: action=%d x=%ld y=%ld\n", __FILE__, action->action,
          action->x, action->y);
    cxlog("window_class='%s'\n", action->window_class);
    cxlog("window_title='%s'\n", action->window_title);
    cxlog("control_id=%ld\n", action->control_id);
    cxlog("control_class='%s'\n", action->control_class);
    cxlog("control_caption='%s'\n", action->control_caption);

    pFreeAction(action);
    pRemoveHooks();
    return 0;
}
