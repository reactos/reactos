/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        roshttpd.cpp
 * PURPOSE:     Main program
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH  01/09/2000 Created
 */
#include <debug.h>
#include <new>
#include <winsock2.h>
#include <stdio.h>
#include <config.h>
#include <error.h>
#include <httpd.h>

using namespace std;


VOID Run()
{
    InitWinsock();

    pDaemonThread = NULL;
	pConfiguration = NULL;

	try {
        // Create configuration object
		pConfiguration = new CConfig;
        pConfiguration->Default();

        // Create daemon object
        pDaemonThread = new CHttpDaemonThread;

		MSG Msg;
		BOOL bQuit = FALSE;
		while ((!bQuit) && (!pDaemonThread->Terminated())) {
		    bQuit = PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE);
			if (!bQuit)
			    DispatchMessage(&Msg);
        }

		delete pDaemonThread;

		if (pConfiguration != NULL)
		    delete pConfiguration;
	} catch (bad_alloc e) {
		if (pConfiguration != NULL)
			delete pConfiguration;
		ReportErrorStr(TS("Insufficient resources."));
	}

    DeinitWinsock();
}

/* Program entry point */
int main(int argc, char* argv[])
{
    printf("ReactOS HTTP Daemon\n");
    printf("Type Control-C to stop.\n");

    Run();

    printf("Daemon stopped.\n");
}
